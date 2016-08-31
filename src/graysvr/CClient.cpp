#include "graysvr.h"	// predef header.
#include "CClient.h"
#include "../network/network.h"
#include "../network/send.h"

#if !defined(_WIN32) || defined(_LIBEV)
	extern LinuxEv g_NetworkEvent;
#endif

/////////////////////////////////////////////////////////////////
// -CClient stuff.

CClient::CClient(NetState* state)
{
	// This may be a web connection or Telnet ?
	m_NetState = state;
	SetConnectType( CONNECT_UNK );	// don't know what sort of connect this is yet.

	// update ip history
#ifndef _MTNETWORK
	HistoryIP& history = g_NetworkIn.getIPHistoryManager().getHistoryForIP(GetPeer());
#else
	HistoryIP& history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(GetPeer());
#endif
	history.m_connecting++;
	history.m_connected++;

	m_Crypt.SetClientVer( g_Serv.m_ClientVersion );
	m_pAccount = NULL;

	m_pChar = NULL;
	m_pGMPage = NULL;

	m_timeLogin.Init();
	m_timeLastEvent = CServTime::GetCurrentTime();
	m_timeLastEventItemPickup = CServTime::GetCurrentTime();
	m_timeLastEventWalk = CServTime::GetCurrentTime();
	m_timeNextEventWalk = 0;

	m_iWalkStepCount = 0;
	m_iWalkTimeAvg	= 100;
	m_timeWalkStep = GetTickCount64();

	m_Targ_Timeout.Init();
	m_Targ_Mode = CLIMODE_SETUP_CONNECTING;
	m_Prompt_Mode = CLIMODE_NORMAL;

	m_tmSetup.m_dwIP = 0;
	m_tmSetup.m_iConnect = 0;
	m_tmSetup.m_bNewSeed = false;

	m_Env.SetInvalid();

	g_Log.Event(LOGM_CLIENTS_LOG, "%lx:Client connected [Total:%lu] ('%s' %ld/%ld)\n",
		GetSocketID(), g_Serv.StatGet(SERV_STAT_CLIENTS), GetPeerStr(), history.m_connecting, history.m_connected);

	m_zLastMessage[0] = 0;
	m_zLastObjMessage[0] = 0;

	m_ScreenSize.x = m_ScreenSize.y = 0x0;
	m_pPopupPacket = NULL;
	m_pHouseDesign = NULL;
	m_fUpdateStats = 0;
}


CClient::~CClient()
{
	bool bWasChar;

	// update ip history
#ifndef _MTNETWORK
	HistoryIP& history = g_NetworkIn.getIPHistoryManager().getHistoryForIP(GetPeer());
#else
	HistoryIP& history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(GetPeer());
#endif
	if ( GetConnectType() != CONNECT_GAME )
		history.m_connecting--;
	history.m_connected--;

	bWasChar = ( m_pChar != NULL );
	CharDisconnect();	// am i a char in game ?
	Cmd_GM_PageClear();

	// Clear containers (CTAG and TOOLTIP)
	m_TagDefs.Empty();
	m_TooltipData.Clean(true);

	if ( m_pAccount )
	{
		m_pAccount->OnLogout(this, bWasChar);
		m_pAccount = NULL;
	}

	if (m_pPopupPacket != NULL)
	{
		delete m_pPopupPacket;
		m_pPopupPacket = NULL;
	}

	if (!m_NetState->isClosed())
		g_Log.EventError("Client being deleted without being safely removed from the network system\n");
}

bool CClient::CanInstantLogOut() const
{
	ADDTOCALLSTACK("CClient::CanInstantLogOut");
	if ( g_Serv.IsLoading())	// or exiting.
		return( true );
	if ( ! g_Cfg.m_iClientLingerTime )
		return true;
	if ( IsPriv( PRIV_GM ))
		return true;
	if ( m_pChar == NULL )
		return( true );
	if ( m_pChar->IsStatFlag(STATF_DEAD))
		return( true );

	const CRegionWorld * pArea = m_pChar->GetRegion();
	if ( pArea == NULL )
		return( true );
	if ( pArea->IsFlag( REGION_FLAG_INSTA_LOGOUT ))
		return( true );

	const CRegionBase * pRoom = m_pChar->GetRoom(); //Allows Room flag to work!
	if ( pRoom != NULL && pRoom->IsFlag( REGION_FLAG_INSTA_LOGOUT )) //sanity check for null rooms // Can C++ guarantee short-circuit evaluation for CRegionBase ?
		return( true );

	return( false );
}

void CClient::CharDisconnect()
{
	ADDTOCALLSTACK("CClient::CharDisconnect");
	// Disconnect the CChar from the client.
	// Even tho the CClient might stay active.
	if ( m_pChar == NULL )
		return;
	int	iLingerTime = g_Cfg.m_iClientLingerTime;

	Announce(false);
	bool fCanInstaLogOut = CanInstantLogOut();

	//	stoned chars cannot logout if they are not privileged of course
	if ( m_pChar->IsStatFlag(STATF_Stone) && ( GetPrivLevel() < PLEVEL_Counsel ))
	{
		iLingerTime = 60*60*TICK_PER_SEC;	// 1 hour of linger time
		fCanInstaLogOut = false;
	}

	//	we are not a client anymore
	if ( IsChatActive() )
		g_Serv.m_Chats.QuitChat(this);

	if ( m_pHouseDesign )
		m_pHouseDesign->EndCustomize(true);

	if ( IsTrigUsed(TRIGGER_LOGOUT) )
	{
		CScriptTriggerArgs Args(iLingerTime, fCanInstaLogOut);
		m_pChar->OnTrigger(CTRIG_LogOut, m_pChar, &Args);
		iLingerTime = static_cast<int>(Args.m_iN1);
		fCanInstaLogOut = (Args.m_iN2 != 0);
	}

	m_pChar->ClientDetach();	// we are not a client any more.

	if ( iLingerTime <= 0 ) fCanInstaLogOut = true;

	// Gump memory cleanup, we don't want them on logged out players
	m_mapOpenedGumps.clear();

	// Layer dragging, moving it to backpack
	CItem * pItemDragging = m_pChar->LayerFind(LAYER_DRAGGING);
	if ( pItemDragging )
	{
		m_pChar->ItemBounce(pItemDragging);
	}

	// log out immediately ? (test before ClientDetach())
	if ( ! fCanInstaLogOut )
	{
		// become an NPC for a little while
		CItem * pItemChange = CItem::CreateBase( ITEMID_RHAND_POINT_W );
		ASSERT(pItemChange);
		pItemChange->SetName("Client Linger");
		pItemChange->SetType(IT_EQ_CLIENT_LINGER);
		pItemChange->SetTimeout(iLingerTime);
		m_pChar->LayerAdd(pItemChange, LAYER_FLAG_ClientLinger);
	}
	else
	{
		// remove me from other clients screens now.
		m_pChar->SetDisconnected();
	}

	m_pChar = NULL;
}

void CClient::SysMessage( LPCTSTR pszMsg ) const // System message (In lower left corner)
{
	ADDTOCALLSTACK("CClient::SysMessage");
	// Diff sorts of clients.
	if ( !pszMsg || !*pszMsg ) return;

	switch ( GetConnectType() )
	{
		case CONNECT_TELNET:
		case CONNECT_AXIS:
			{
				if ( ISINTRESOURCE(pszMsg) || *pszMsg == '\0' ) return;

				new PacketTelnet(this, pszMsg);
			}
			return;
		case CONNECT_UOG:
			{
				if ( ISINTRESOURCE(pszMsg) || *pszMsg == '\0' ) return;

				new PacketTelnet(this, pszMsg, true);
			}
			return;
		case CONNECT_CRYPT:
		case CONNECT_LOGIN:
		case CONNECT_GAME:
			const_cast <CClient*>(this)->addSysMessage(pszMsg);
			return;

		case CONNECT_HTTP:
			const_cast <CClient*>(this)->m_Targ_Text = pszMsg;
			return;

		default:
			return;
	}
}

void CClient::Announce( bool fArrive ) const
{
	ADDTOCALLSTACK("CClient::Announce");
	if ( !m_pAccount || !GetChar() || !GetChar()->m_pPlayer )
		return;

	// We have logged in or disconnected.
	// Annouce my arrival or departure.
	TCHAR *pszMsg = Str_GetTemp();
	if ( (g_Cfg.m_iArriveDepartMsg == 2) && (GetPrivLevel() > PLEVEL_Player) )		// notify of GMs
	{
		LPCTSTR zTitle = m_pChar->Noto_GetFameTitle();
		sprintf(pszMsg, "@231 STAFF: %s%s logged %s.", zTitle, m_pChar->GetName(), (fArrive ? "in" : "out"));
	}
	else if ( g_Cfg.m_iArriveDepartMsg == 1 )		// notify of players
	{
		const CRegionBase *pRegion = m_pChar->GetTopPoint().GetRegion(REGION_TYPE_AREA);
		sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MSG_ARRDEP_1),
			m_pChar->GetName(),
			fArrive ? g_Cfg.GetDefaultMsg(DEFMSG_MSG_ARRDEP_2) : g_Cfg.GetDefaultMsg(DEFMSG_MSG_ARRDEP_3),
			pRegion != NULL ? pRegion->GetName() : g_Serv.GetName());
	}
	if ( pszMsg )
	{
		ClientIterator it;
		for (CClient *pClient = it.next(); pClient != NULL; pClient = it.next())
		{
			if ( (pClient == this) || (GetPrivLevel() > pClient->GetPrivLevel()) )
				continue;
			pClient->SysMessage(pszMsg);
		}

	}

	// Check murder decay timer
	CItem *pMurders = m_pChar->LayerFind(LAYER_FLAG_Murders);
	if ( pMurders )
	{
		if ( fArrive )	// on client login, set active timer on murder memory
			pMurders->SetTimeout(pMurders->m_itEqMurderCount.m_Decay_Balance * TICK_PER_SEC);
		else			// or make it inactive on logout
		{
			pMurders->m_itEqMurderCount.m_Decay_Balance = static_cast<DWORD>(pMurders->GetTimerAdjusted());
			pMurders->SetTimeout(-1);
		}
	}
	else if ( fArrive )
		m_pChar->Noto_Murder();		// if there's no murder memory found, check if we need a new memory
}

////////////////////////////////////////////////////

bool CClient::CanSee( const CObjBaseTemplate * pObj ) const
{
	ADDTOCALLSTACK("CClient::CanSee");
	// Can player see item b
	if ( m_pChar == NULL || pObj == NULL )
		return false;

	if (!IsPriv(PRIV_ALLSHOW) && pObj->IsChar())
	{
		const CChar *pChar = static_cast<const CChar*>(pObj);
		if (pChar->IsDisconnected())
			return false;
	}
	return( m_pChar->CanSee( pObj ));
}

bool CClient::CanHear( const CObjBaseTemplate * pSrc, TALKMODE_TYPE mode ) const
{
	ADDTOCALLSTACK("CClient::CanHear");
	// can we hear this text or sound.

	if ( !IsConnectTypePacket() )
		return false;
	if ( !pSrc || (mode == TALKMODE_BROADCAST) )
		return true;
	if ( !m_pChar )
		return false;

	if ( IsPriv(PRIV_HEARALL) && pSrc->IsChar() && (mode == TALKMODE_SYSTEM || mode == TALKMODE_SAY || mode == TALKMODE_WHISPER || mode == TALKMODE_YELL) )
	{
		const CChar *pCharSrc = dynamic_cast<const CChar *>(pSrc);
		ASSERT(pCharSrc);
		if ( pCharSrc && pCharSrc->m_pClient )
		{
			if ( pCharSrc->GetPrivLevel() <= GetPrivLevel() )
				return true;
		}
	}

	return m_pChar->CanHear(pSrc, mode);
}

////////////////////////////////////////////////////


void CClient::addTargetVerb( LPCTSTR pszCmd, LPCTSTR pszArg )
{
	ADDTOCALLSTACK("CClient::addTargetVerb");
	// Target a verb at some object .

	ASSERT(pszCmd);
	GETNONWHITESPACE(pszCmd);
	SKIP_SEPARATORS(pszCmd);

	if ( !strlen(pszCmd) )
		pszCmd = pszArg;

	if ( pszCmd == pszArg )
	{
		GETNONWHITESPACE(pszCmd);
		SKIP_SEPARATORS(pszCmd);
		pszArg = "";
	}

	// priv here
	PLEVEL_TYPE ilevel = g_Cfg.GetPrivCommandLevel( pszCmd );
	if ( ilevel > GetPrivLevel() )
		return;

	m_Targ_Text.Format( "%s%s%s", pszCmd, ( pszArg[0] && pszCmd[0] ) ? " " : "", pszArg );
	TCHAR * pszMsg = Str_GetTemp();
	sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_TARGET_COMMAND), static_cast<LPCTSTR>(m_Targ_Text));
	addTarget(CLIMODE_TARG_OBJ_SET, pszMsg);
}

void CClient::addTargetFunctionMulti( LPCTSTR pszFunction, ITEMID_TYPE itemid, HUE_TYPE color, bool bAllowGround )
{
	ADDTOCALLSTACK("CClient::addTargetFunctionMulti");
	// Target a verb at some object .
	ASSERT(pszFunction);
	GETNONWHITESPACE(pszFunction);
	SKIP_SEPARATORS(pszFunction);

	m_Targ_Text = pszFunction;
	if ( CItemBase::IsID_Multi(itemid) )	// a multi we get from multi.mul
	{
		SetTargMode(CLIMODE_TARG_OBJ_FUNC);
		new PacketAddTarget(this, bAllowGround ? PacketAddTarget::Ground : PacketAddTarget::Object, CLIMODE_TARG_OBJ_FUNC, PacketAddTarget::None, itemid, color);
	}
	addTargetFunction(pszFunction, bAllowGround, false);
}

void CClient::addTargetFunction( LPCTSTR pszFunction, bool bAllowGround, bool bCheckCrime )
{
	ADDTOCALLSTACK("CClient::addTargetFunction");
	// Target a verb at some object .
	ASSERT(pszFunction);
	GETNONWHITESPACE(pszFunction);
	SKIP_SEPARATORS(pszFunction);

	m_Targ_Text = pszFunction;
	addTarget(CLIMODE_TARG_OBJ_FUNC, NULL, bAllowGround, bCheckCrime);
}

void CClient::addPromptConsoleFunction( LPCTSTR pszFunction, LPCTSTR pszSysmessage, bool bUnicode )
{
	ADDTOCALLSTACK("CClient::addPromptConsoleFunction");
	// Target a verb at some object .
	ASSERT(pszFunction);
	m_Prompt_Text = pszFunction;
	addPromptConsole(CLIMODE_PROMPT_SCRIPT_VERB, pszSysmessage, 0, 0, bUnicode);
}

////////////////////////////////////////////////////

void CClient::UpdateFeatureFlags()
{
	ADDTOCALLSTACK("CClient::UpdateFeatureFlags");
	// Update ingame features enabled on this client
	// NOTE: Flags > 0x10000 requires at least client 6.0.14.2

	// 0x000001		Enable T2A features
	// 0x000002		Enable Renaissance features (play MP3 instead midi, ...)
	// 0x000004		Enable TD features
	// 0x000008		Enable LBR features
	// 0x000010		Enable AOS features (necro/paladin, house customization engine, ...)
	// 0x000020		Sixth character slot
	// 0x000040		Enable SE features
	// 0x000080		Enable ML features
	// 0x000100		8th age
	// 0x000200		9th age (enable crystal/shadow custom house tiles)
	// 0x000400		10th age
	// 0x000800		Increased house/bank storage
	// 0x001000		Seventh character slot
	// 0x002000		KR custom character faces
	// 0x004000		Trial account
	// 0x008000		Live account (required on clients 4.0.0+, otherwise bits 3..14 will be ignored)
	// 0x010000		Enable SA features
	// 0x020000		Enable HS features
	// 0x040000		Enable gothic custom house tiles
	// 0x080000		Enable rustic custom house tiles
	// 0x100000		Enable jungle custom house tiles
	// 0x200000		Enable shadowguard custom house tiles
	// 0x400000		Enable TOL features

	if ( !m_pAccount )
		return;

	RESDISPLAY_VERSION iResdisp = static_cast<RESDISPLAY_VERSION>(m_pAccount->GetResDisp());
	BYTE iMaxChars = static_cast<BYTE>(maximum(m_pAccount->GetMaxChars(), m_pAccount->m_Chars.GetCharCount()));
	m_FeatureFlags = 0;

	// Expansion features
	if ( iResdisp >= RDS_T2A )
	{
		if ( g_Cfg.m_iFeatureT2A & FEATURE_T2A_UPDATE )
			m_FeatureFlags |= 0x4;
		if ( g_Cfg.m_iFeatureT2A & FEATURE_T2A_CHAT )
			m_FeatureFlags |= 0x1;
	}

	if ( iResdisp >= RDS_LBR )
	{
		if ( g_Cfg.m_iFeatureLBR & FEATURE_LBR_UPDATE )
			m_FeatureFlags |= 0x8;
		if ( g_Cfg.m_iFeatureLBR & FEATURE_LBR_SOUND )
			m_FeatureFlags |= 0x2;
	}

	if ( iResdisp >= RDS_AOS )
	{
		if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_A )
			m_FeatureFlags |= 0x8000|0x10;
	}

	if ( iResdisp >= RDS_SE )
	{
		if ( g_Cfg.m_iFeatureSE & FEATURE_SE_NINJASAM )
			m_FeatureFlags |= 0x40;
	}

	if ( iResdisp >= RDS_ML )
	{
		if ( g_Cfg.m_iFeatureML & FEATURE_ML_UPDATE )
			m_FeatureFlags |= 0x80;
	}

	if ( iResdisp >= RDS_SA )
	{
		if ( g_Cfg.m_iFeatureSA & FEATURE_SA_UPDATE )
			m_FeatureFlags |= 0x10000;
	}

	if ( iResdisp >= RDS_TOL )
	{
		if ( g_Cfg.m_iFeatureTOL & FEATURE_TOL_UPDATE )
			m_FeatureFlags |= 0x400000;
	}

	// Max character slots
	if ( iMaxChars > 6 )
		m_FeatureFlags |= 0x1000;
	if ( iMaxChars == 6 )
		m_FeatureFlags |= 0x20;

	// Extra housing features (enable expansion items on house customization engine)
	// PS: main expansion items are already enabled with flags 0x10 (AOS), 0x40 (SE), 0x80 (ML), 0x10000 (SA) and 0x400000 (TOL)
	if ( g_Cfg.m_iFeatureExtra & FEATURE_EXTRA_CRYSTAL )
		m_FeatureFlags |= 0x200;
	if ( g_Cfg.m_iFeatureExtra & FEATURE_EXTRA_GOTHIC )
		m_FeatureFlags |= 0x40000;
	if ( g_Cfg.m_iFeatureExtra & FEATURE_EXTRA_RUSTIC )
		m_FeatureFlags |= 0x80000;
	if ( g_Cfg.m_iFeatureExtra & FEATURE_EXTRA_JUNGLE )
		m_FeatureFlags |= 0x100000;
	if ( g_Cfg.m_iFeatureExtra & FEATURE_EXTRA_SHADOWGUARD )
		m_FeatureFlags |= 0x200000;
}

void CClient::UpdateCharacterListFlags()
{
	ADDTOCALLSTACK("CClient::UpdateCharacterListFlags");
	// Update character list features enabled on this client

	// 0x0001		Unknown
	// 0x0002		Overwrite config buttons
	// 0x0004		Single character slot
	// 0x0008		Context menus
	// 0x0010		Limit character slots
	// 0x0020		Enable AOS features (tooltip, fight book)
	// 0x0040		Sixth character slot
	// 0x0080		Enable SE features
	// 0x0100		Enable ML features
	// 0x0200		Unknown (KR)
	// 0x0400		Enable KR/SA packet 0xE1 at character list (possibly other unknown effects)
	// 0x0800		Unknown (KR/SA)
	// 0x1000		Seventh character slot
	// 0x2000		Unknown (SA)
	// 0x4000		Enable new SA movement packets 0xF0 / 0xF1 / 0xF2
	// 0x8000		New Felucca areas / faction strongholds (uses map0x.mul, statics0x.mul, etc) - client 7.0.6+

	if ( !m_pAccount )
		return;

	RESDISPLAY_VERSION iResdisp = static_cast<RESDISPLAY_VERSION>(m_pAccount->GetResDisp());
	BYTE iMaxChars = static_cast<BYTE>(maximum(m_pAccount->GetMaxChars(), m_pAccount->m_Chars.GetCharCount()));
	m_CharacterListFlags = 0;

	// Expansion features
	if ( iResdisp >= RDS_AOS )
	{
		if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B )
			m_CharacterListFlags |= 0x20;
		if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_POPUP )
			m_CharacterListFlags |= 0x8;
	}

	if ( iResdisp >= RDS_SE )
	{
		if ( g_Cfg.m_iFeatureSE & FEATURE_SE_UPDATE )
			m_CharacterListFlags |= 0x80;
	}

	if ( iResdisp >= RDS_ML )
	{
		if ( g_Cfg.m_iFeatureML & FEATURE_ML_UPDATE )
			m_CharacterListFlags |= 0x100;
	}

	if ( iResdisp >= RDS_KR )
	{
		if ( g_Cfg.m_iFeatureKR & FEATURE_KR_UPDATE )
			m_CharacterListFlags |= 0x200;
	}

	if ( iResdisp >= RDS_SA )
	{
		if ( g_Cfg.m_iFeatureSA & FEATURE_SA_MOVEMENT )
			m_CharacterListFlags |= 0x4000;
	}

	// Max character slots
	if ( iMaxChars > 6 )
		m_CharacterListFlags |= 0x1000;
	else if ( iMaxChars == 6 )
		m_CharacterListFlags |= 0x40;
	else if ( iMaxChars == 1 )
		m_CharacterListFlags |= 0x10|0x4;

	// Misc
	if ( m_NetState->isClientKR() || m_NetState->isClientEnhanced() )		// tooltips must be always enabled on enhanced clients
		m_CharacterListFlags |= 0x400|0x20;
	m_TooltipEnabled = (m_CharacterListFlags & 0x20);
	m_ContainerGridEnabled = (m_NetState->isClientVersion(MINCLIVER_CONTAINERGRID) || m_NetState->isClientKR() || m_NetState->isClientEnhanced());
}

////////////////////////////////////////////////////

enum CLIR_TYPE
{
	CLIR_ACCOUNT,
	CLIR_GMPAGEP,
	CLIR_HOUSEDESIGN,
	CLIR_PARTY,
	CLIR_TARG,
	CLIR_TARGPROP,
	CLIR_TARGPRV,
	CLIR_QTY
};

LPCTSTR const CClient::sm_szRefKeys[CLIR_QTY+1] =
{
	"ACCOUNT",
	"GMPAGEP",
	"HOUSEDESIGN",
	"PARTY",
	"TARG",
	"TARGPROP",
	"TARGPRV",
	NULL
};

bool CClient::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CClient::r_GetRef");
	int i = FindTableHeadSorted( pszKey, sm_szRefKeys, COUNTOF(sm_szRefKeys)-1 );
	if ( i >= 0 )
	{
		pszKey += strlen( sm_szRefKeys[i] );
		SKIP_SEPARATORS(pszKey);
		switch (i)
		{
			case CLIR_ACCOUNT:
				if ( pszKey[-1] != '.' )	// only used as a ref !
					break;
				pRef = m_pAccount;
				return( true );
			case CLIR_GMPAGEP:
				pRef = m_pGMPage;
				return( true );
			case CLIR_HOUSEDESIGN:
				pRef = m_pHouseDesign;
				return( true );
			case CLIR_PARTY:
				if ( !m_pChar->m_pParty )
				{
					LPCTSTR oldKey = pszKey;
					if ( !strnicmp(pszKey, "CREATE", 7) )
						pszKey +=7;
						
					SKIP_SEPARATORS(pszKey);
					CChar * pChar = static_cast<CChar*>(static_cast<CGrayUID>(Exp_GetSingle(pszKey)).CharFind());
					if ( !pChar )
						return false;
					if ( !pChar->m_pClient )
						return false;
					CPartyDef::AcceptEvent(pChar, GetChar()->GetUID(), true);
					if ( !m_pChar->m_pParty )
						return false;
					pszKey = oldKey;	// Restoring back to real pszKey, so we don't get errors for giving an uid instead of PDV_CREATE.
				}
				pRef = m_pChar->m_pParty;
				return true;
			case CLIR_TARG:
				pRef = m_Targ_UID.ObjFind();
				return( true );
			case CLIR_TARGPRV:
				pRef = m_Targ_PrvUID.ObjFind();
				return( true );
			case CLIR_TARGPROP:
				pRef = m_Prop_UID.ObjFind();
				return( true );
		}
	}
	return( CScriptObj::r_GetRef( pszKey, pRef ));
}

LPCTSTR const CClient::sm_szLoadKeys[CC_QTY+1] = // static
{
	#define ADD(a,b) b,
	#include "../tables/CClient_props.tbl"
	#undef ADD
	NULL,
};

LPCTSTR const CClient::sm_szVerbKeys[CV_QTY+1] =	// static
{
	#define ADD(a,b) b,
	#include "../tables/CClient_functions.tbl"
	#undef ADD
	NULL,
};

bool CClient::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CClient::r_WriteVal");
	EXC_TRY("WriteVal");
	int index;

	if ( !strnicmp("CTAG.", pszKey, 5) )		//	CTAG.xxx - client tag
	{
		if ( pszKey[4] != '.' )
			return( false );
		pszKey += 5;
		CVarDefCont *vardef = m_TagDefs.GetKey(pszKey);
		sVal = vardef ? vardef->GetValStr() : "";
		return true;
	}

	if ( !strnicmp("CTAG0.", pszKey, 6) )		//	CTAG0.xxx - client tag
	{
		if ( pszKey[5] != '.' )
			return( false );
		pszKey += 6;
		CVarDefCont *vardef = m_TagDefs.GetKey(pszKey);
		sVal = vardef ? vardef->GetValStr() : "0";
		return true;
	}

	if ( !strnicmp( "TARGP", pszKey, 5 ) && ( pszKey[5] == '\0' || pszKey[5] == '.' ) )
		index = CC_TARGP;
	else if ( !strnicmp( "SCREENSIZE", pszKey, 10 ) && ( pszKey[10] == '\0' || pszKey[10] == '.' ) )
		index = CC_SCREENSIZE;
	else if ( !strnicmp( "REPORTEDCLIVER", pszKey, 14 ) && ( pszKey[14] == '\0' || pszKey[14] == '.' ) )
		index = CC_REPORTEDCLIVER;
	else
		index	= FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 );

	switch (index)
	{
		case CC_ALLMOVE:
			sVal.FormatVal( IsPriv( PRIV_ALLMOVE ));
			break;
		case CC_ALLSHOW:
			sVal.FormatVal( IsPriv( PRIV_ALLSHOW ));
			break;
		case CC_CLIENTIS3D:
			sVal.FormatVal( m_NetState->isClient3D() );
			break;
		case CC_CLIENTISKR:
			sVal.FormatVal( m_NetState->isClientKR() );
			break;
		case CC_CLIENTISSA:
			sVal.FormatVal( m_NetState->isClientEnhanced() );
			break;
		case CC_CLIENTVERSION:
			{
				TCHAR szVersion[ 128 ];
				sVal = m_Crypt.WriteClientVer( szVersion );
			}
			break;
		case CC_DEBUG:
			sVal.FormatVal( IsPriv( PRIV_DEBUG ));
			break;
		case CC_DETAIL:
			sVal.FormatVal( IsPriv( PRIV_DETAIL ));
			break;
		case CC_GM:	// toggle your GM status on/off
			sVal.FormatVal( IsPriv( PRIV_GM ));
			break;
		case CC_HEARALL:
			sVal.FormatVal( IsPriv( PRIV_HEARALL ));
			break;
		case CC_LASTEVENT:
			sVal.FormatLLVal( m_timeLastEvent.GetTimeRaw() );
			break;
		case CC_PRIVSHOW:
			// Show my priv title.
			sVal.FormatVal( ! IsPriv( PRIV_PRIV_NOSHOW ));
			break;
		case CC_REPORTEDCLIVER:
			{
				pszKey += strlen(sm_szLoadKeys[index]);
				GETNONWHITESPACE(pszKey);

				DWORD iCliVer = m_NetState->getReportedVersion();
				if ( pszKey[0] == '\0' )
				{
					// Return full version string (eg: 5.0.2d)
					TCHAR szVersion[128];
					sVal = CCrypt::WriteClientVerString(iCliVer, szVersion);
				}
				else
				{
					// Return raw version number (eg: 5.0.2d = 5000204)
					sVal.FormatUVal(iCliVer);
				}
			}
			break;
		case CC_SCREENSIZE:
			{
				if ( pszKey[10] == '.' )
				{
					pszKey += strlen(sm_szLoadKeys[index]);
					SKIP_SEPARATORS(pszKey);

					if ( !strnicmp("X", pszKey, 1) )
						sVal.Format( "%lu", m_ScreenSize.x );
					else if ( !strnicmp("Y", pszKey, 1) )
						sVal.Format( "%lu", m_ScreenSize.y );
					else
						return( false );
				}
				else
					sVal.Format( "%lu,%lu", m_ScreenSize.x, m_ScreenSize.y );
			} break;
		case CC_TARG:
			sVal.FormatHex(m_Targ_UID);
			break;
		case CC_TARGP:
			if ( pszKey[5] == '.' )
			{
				return m_Targ_p.r_WriteVal( pszKey+6, sVal );
			}
			sVal = m_Targ_p.WriteUsed();
			break;
		case CC_TARGPROP:
			sVal.FormatHex(m_Prop_UID);
			break;
		case CC_TARGPRV:
			sVal.FormatHex(m_Targ_PrvUID);
			break;
		case CC_TARGTXT:
			sVal = m_Targ_Text;
			break;
		default:
			return( CScriptObj::r_WriteVal( pszKey, sVal, pSrc ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CClient::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CClient::r_LoadVal");
	EXC_TRY("LoadVal");
	if ( !m_pAccount )
		return( false );

	LPCTSTR pszKey = s.GetKey();

	if ( s.IsKeyHead( "CTAG.", 5 ) || s.IsKeyHead( "CTAG0.", 6 ) )
	{
		bool fZero = s.IsKeyHead( "CTAG0.", 6 );
		bool fQuoted = false;

		pszKey = pszKey + (fZero ? 6 : 5);
		m_TagDefs.SetStr( pszKey, fQuoted, s.GetArgStr( &fQuoted ), fZero );
		return( true );
	}

	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 ))
	{
		case CC_ALLMOVE:
			addRemoveAll(true, false);
			m_pAccount->TogPrivFlags( PRIV_ALLMOVE, s.GetArgStr() );
			if ( IsSetOF( OF_Command_Sysmsgs ) )
				m_pChar->SysMessage( IsPriv(PRIV_ALLMOVE)? "Allmove ON" : "Allmove OFF" );
			addPlayerView(NULL);
			break;
		case CC_ALLSHOW:
			addRemoveAll(false, true);
			m_pAccount->TogPrivFlags( PRIV_ALLSHOW, s.GetArgStr() );
			if ( IsSetOF( OF_Command_Sysmsgs ) )
				m_pChar->SysMessage( IsPriv(PRIV_ALLSHOW)? "Allshow ON" : "Allshow OFF" );
			addPlayerView(NULL);
			break;
		case CC_DEBUG:
			addRemoveAll(true, false);
			m_pAccount->TogPrivFlags( PRIV_DEBUG, s.GetArgStr() );
			if ( IsSetOF( OF_Command_Sysmsgs ) )
					m_pChar->SysMessage( IsPriv(PRIV_DEBUG)? "Debug ON" : "Debug OFF" );
			addPlayerView(NULL);
			break;
		case CC_DETAIL:
			m_pAccount->TogPrivFlags( PRIV_DETAIL, s.GetArgStr() );
			if ( IsSetOF( OF_Command_Sysmsgs ) )
					m_pChar->SysMessage( IsPriv(PRIV_DETAIL)? "Detail ON" : "Detail OFF" );
			break;
		case CC_GM: // toggle your GM status on/off
			if ( GetPrivLevel() >= PLEVEL_GM )
			{
				m_pAccount->TogPrivFlags( PRIV_GM, s.GetArgStr() );
				m_pChar->ResendTooltip();
				if ( IsSetOF( OF_Command_Sysmsgs ) )
					m_pChar->SysMessage( IsPriv(PRIV_GM)? "GM ON" : "GM OFF" );
			}
			break;
		case CC_HEARALL:
			m_pAccount->TogPrivFlags( PRIV_HEARALL, s.GetArgStr() );
			if ( IsSetOF( OF_Command_Sysmsgs ) )
					m_pChar->SysMessage( IsPriv(PRIV_HEARALL)? "Hearall ON" : "Hearall OFF" );
			break;
		case CC_PRIVSHOW:
			// Hide my priv title.
			if ( GetPrivLevel() >= PLEVEL_Counsel )
			{
				if ( ! s.HasArgs())
				{
					m_pAccount->TogPrivFlags( PRIV_PRIV_NOSHOW, NULL );
				}
				else if ( s.GetArgVal() )
				{
					m_pAccount->ClearPrivFlags( PRIV_PRIV_NOSHOW );
				}
				else
				{
					m_pAccount->SetPrivFlags( PRIV_PRIV_NOSHOW );
				}
				m_pChar->ResendTooltip();
				if ( IsSetOF( OF_Command_Sysmsgs ) )
					m_pChar->SysMessage( IsPriv(PRIV_PRIV_NOSHOW)? "Privshow OFF" : "Privshow ON" );
			}
			break;

		case CC_TARG:
			m_Targ_UID = s.GetArgVal();
			break;
		case CC_TARGP:
			m_Targ_p.Read( s.GetArgRaw());
			if ( !m_Targ_p.IsValidPoint() )
			{
				m_Targ_p.ValidatePoint();
				SysMessagef( "Invalid point: %s", s.GetArgStr() );
			}
			break;
		case CC_TARGPROP:
			m_Prop_UID = s.GetArgVal();
			break;
		case CC_TARGPRV:
			m_Targ_PrvUID = s.GetArgVal();
			break;
		default:
			return( false );
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CClient::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CClient::r_Verb");
	EXC_TRY("Verb");
	// NOTE: This can be called directly from a RES_WEBPAGE script.
	//  So do not assume we are a game client !
	// NOTE: Mostly called from CChar::r_Verb
	// NOTE: Little security here so watch out for dangerous scripts !

	ASSERT(pSrc);
	LPCTSTR pszKey = s.GetKey();

	// Old ver
	if ( s.IsKeyHead( "SET", 3 ) && ( g_Cfg.m_Functions.ContainsKey( pszKey ) == false ) )
	{
		PLEVEL_TYPE ilevel = g_Cfg.GetPrivCommandLevel( "SET" );
		if ( ilevel > GetPrivLevel() )
			return( false );

		ASSERT( m_pChar );
		addTargetVerb( pszKey+3, s.GetArgRaw());
		return( true );
	}

	if ( toupper( pszKey[0] ) == 'X' && ( g_Cfg.m_Functions.ContainsKey( pszKey ) == false ) )
	{
		PLEVEL_TYPE ilevel = g_Cfg.GetPrivCommandLevel( "SET" );
		if ( ilevel > GetPrivLevel() )
			return( false );

		// Target this command verb on some other object.
		ASSERT( m_pChar );
		addTargetVerb( pszKey+1, s.GetArgRaw());
		return( true );
	}

	int index = FindTableSorted( s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys)-1 );
	switch (index)
	{
		case CV_ADD:
			if ( s.HasArgs() )
			{
				TCHAR *ppArgs[2];
				size_t iQty = Str_ParseCmds(s.GetArgStr(), ppArgs, COUNTOF(ppArgs));

				if ( !IsValidGameObjDef(static_cast<LPCTSTR>(ppArgs[0])) )
				{
					g_Log.EventWarn("Invalid ADD argument '%s'\n", ppArgs[0]);
					SysMessageDefault(DEFMSG_CMD_INVALID);
					return true;
				}

				RESOURCE_ID rid = g_Cfg.ResourceGetID(RES_QTY, const_cast<LPCTSTR &>(ppArgs[0]));
				m_tmAdd.m_id = rid.GetResIndex();
				m_tmAdd.m_amount = (iQty > 1) ? static_cast<WORD>(maximum(ATOI(ppArgs[1]), 1)) : 1;

				if ( (rid.GetResType() == RES_CHARDEF) || (rid.GetResType() == RES_SPAWN) )
				{
					m_Targ_PrvUID.InitUID();
					return addTargetChars(CLIMODE_TARG_ADDCHAR, static_cast<CREID_TYPE>(m_tmAdd.m_id), false);
				}
				else
				{
					return addTargetItems(CLIMODE_TARG_ADDITEM, static_cast<ITEMID_TYPE>(m_tmAdd.m_id));
				}
			}
			else
			{
				if ( IsValidDef("d_add") )
					Dialog_Setup(CLIMODE_DIALOG, g_Cfg.ResourceGetIDType(RES_DIALOG, "d_add"), 0, GetChar());
				else
					Menu_Setup(g_Cfg.ResourceGetIDType(RES_MENU, "MENU_ADDITEM"));
			}
			break;
		case CV_ADDBUFF:
			{
				TCHAR * ppArgs[11];
				Str_ParseCmds( s.GetArgStr(), ppArgs, COUNTOF(ppArgs));

				int iArgs[4];
				for ( int idx = 0; idx < 4; ++idx )
				{
					if ( !IsStrNumeric(ppArgs[idx]))
					{
						DEBUG_ERR(("Invalid AddBuff argument number %u\n", idx+1));
						return true;
					}
					iArgs[idx] = Exp_GetVal( ppArgs[idx] );
				}
				if (iArgs[0] < BI_START || iArgs[0] > BI_QTY/* || iArgs[0] == 0x3EB || iArgs[0] == 0x3EC*/) {	// 0x3eb and 0x3ec among some others does not exists now, which doesn't mean they won't fill them and, since nothing happens when wrong id is sent, we can let them be sent.
					DEBUG_ERR(("Invalid AddBuff icon ID\n"));
					break;
				}

				LPCTSTR Args[7];
				size_t ArgsCount = 0;
				for ( int i = 0; i < 7; ++i )
				{
					Args[i] = ppArgs[i + 4];
					if ( Args[i] != NULL )
						ArgsCount++;
				}

				addBuff(static_cast<BUFF_ICONS>(iArgs[0]), iArgs[1], iArgs[2], static_cast<WORD>(iArgs[3]), Args, ArgsCount);
			}
			break;
		case CV_REMOVEBUFF:
			{
				BUFF_ICONS IconId = static_cast<BUFF_ICONS>(s.GetArgVal());
				if (IconId < BI_START || IconId > BI_QTY/* || IconId == 0x3EB || IconId == 0x3EC*/) {
					DEBUG_ERR(("Invalid RemoveBuff icon ID\n"));
					break;
				}
				removeBuff(IconId);
			}
			break;
		case CV_ADDCLILOC:
			// Add cliloc in @ClientTooltip trigger
			{
				TCHAR * ppLocArgs[256];
				size_t qty = Str_ParseCmds(s.GetArgRaw(), ppLocArgs, COUNTOF(ppLocArgs), ",");
				DWORD clilocid = Exp_GetVal(ppLocArgs[0]);

				CGString LocArgs;
				for ( size_t y = 1 ; y < qty; y++ )
				{
					if ( LocArgs.GetLength() )
						LocArgs += "\t";
					LocArgs += ( !strcmp(ppLocArgs[y], "NULL") ? " " : ppLocArgs[y] );
				}

				if ( g_Cfg.m_wDebugFlags & DEBUGF_SCRIPTS )
					g_Log.EventDebug("SCRIPT: addcliloc(%lu,'%s')\n", clilocid, static_cast<LPCTSTR>(LocArgs));
				m_TooltipData.Add(new CClientTooltip(clilocid, LocArgs));
			}
			break;
		case CV_ADDCONTEXTENTRY:
			{
				TCHAR * ppLocArgs[20];
				if ( Str_ParseCmds(s.GetArgRaw(), ppLocArgs, COUNTOF(ppLocArgs), ",") > 4 )
				{
					DEBUG_ERR(("Bad AddContextEntry usage: Function takes maximum of 4 arguments!\n"));
					return true;
				}

				if (m_pPopupPacket == NULL)
				{
					DEBUG_ERR(("Bad AddContextEntry usage: Not used under a @ContextMenuRequest/@itemContextMenuRequest trigger!\n"));
					return true;
				}

				for ( int i = 0; i < 4; i++ )
				{
					if ( i > 1 && IsStrEmpty(ppLocArgs[i]) )
						continue;

					if ( !IsStrNumeric(ppLocArgs[i]) )
					{
						DEBUG_ERR(("Bad AddContextEntry usage: Argument %d must be a number!\n", i+1));
						return true;
					}
				}

				int entrytag = Exp_GetVal(ppLocArgs[0]);
				if ( entrytag < 100 )
				{
					DEBUG_ERR(("Bad AddContextEntry usage: TextEntry < 100 is reserved for server usage!\n"));
					return true;
				}
				m_pPopupPacket->addOption(static_cast<WORD>(entrytag), static_cast<DWORD>(Exp_GetVal(ppLocArgs[1])), static_cast<WORD>(Exp_GetVal(ppLocArgs[2])), static_cast<WORD>(Exp_GetVal(ppLocArgs[3])));
			}
			break;
		case CV_ARROWQUEST:
			{
				INT64 piVal[3];
				Str_ParseCmds( s.GetArgRaw(), piVal, COUNTOF(piVal));
				addArrowQuest( static_cast<int>(piVal[0]), static_cast<int>(piVal[1]), static_cast<int>(piVal[2]) );
#ifdef _ALPHASPHERE
				// todo: should use a proper container for these, since the arrows are lost
				// when the client logs out, and also newer clients support multiple
				// arrows
				if ( piVal[0] && piVal[1] && m_pChar )
				{
					m_pChar->SetKeyNum("ARROWQUEST_X", piVal[0]);
					m_pChar->SetKeyNum("ARROWQUEST_Y", piVal[1]);
				} else {
					m_pChar->DeleteKey("ARROWQUEST_X");
					m_pChar->DeleteKey("ARROWQUEST_Y");
				}
#endif
			}
			break;
		case CV_BADSPAWN:
			{
				// Loop the world searching for bad spawns
				bool fFound = false;
				for ( int m = 0; m < 256 && !fFound; m++ )
				{
					if ( !g_MapList.m_maps[m] )
						continue;

					for ( int d = 0; d < g_MapList.GetSectorQty(m) && !fFound; d++ )
					{
						CSector *pSector = g_World.GetSector(m, d);
						if ( !pSector )
							continue;

						CItem *pNext;
						CItem *pItem = static_cast<CItem *>(pSector->m_Items_Inert.GetHead());
						for ( ; pItem != NULL && !fFound; pItem = pNext )
						{
							pNext = pItem->GetNext();

							if ( pItem->IsType(IT_SPAWN_ITEM) || pItem->IsType(IT_SPAWN_CHAR) )
							{
								CItemSpawn *pSpawn = static_cast<CItemSpawn*>(pItem);
								CResourceDef *pDef = pSpawn->FixDef();
								if ( !pDef )
								{
									RESOURCE_ID_BASE rid = pItem->IsType(IT_SPAWN_ITEM) ? pItem->m_itSpawnItem.m_ItemID : pItem->m_itSpawnChar.m_CharID;

									CPointMap pt = pItem->GetTopPoint();
									m_pChar->Spell_Teleport(pt, true, false);
									m_pChar->m_Act_Targ = pItem->GetUID();
									SysMessagef("Bad spawn (0%lx, id=%s). Set as ACT", static_cast<DWORD>(pItem->GetUID()), g_Cfg.ResourceGetName(rid));
									fFound = true;
								}
							}
						}
					}
				}
				if ( !fFound )
					SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NO_BAD_SPAWNS));
			}
			break;
		case CV_BANKSELF: // open my own bank
			addBankOpen(m_pChar, LAYER_BANKBOX);
			break;
		case CV_CAST:
			{
				SPELL_TYPE spell = static_cast<SPELL_TYPE>(g_Cfg.ResourceGetIndexType(RES_SPELL, s.GetArgStr()));
				const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
				if (pSpellDef == NULL)
					return true;

				CObjBase * pObjSrc = dynamic_cast<CObjBase *>(pSrc);

				if ( IsSetMagicFlags( MAGICF_PRECAST ) && !pSpellDef->IsSpellType( SPELLFLAG_NOPRECAST ) )
				{
					int skill;
					if (!pSpellDef->GetPrimarySkill(&skill, NULL))
						return true;

					m_tmSkillMagery.m_Spell = spell;	// m_atMagery.m_Spell
					m_pChar->m_atMagery.m_Spell = spell;
					if (pObjSrc != NULL)
					{
						m_Targ_UID = pObjSrc->GetUID();	// default target.
						m_Targ_PrvUID = pObjSrc->GetUID();
					}
					else
					{
						m_Targ_UID.ClearUID();
						m_Targ_PrvUID.ClearUID();
					}
					m_pChar->Skill_Start(static_cast<SKILL_TYPE>(skill));
					break;
				}
				else
					Cmd_Skill_Magery(spell, pObjSrc);
			}
			break;

		case CV_CHARLIST:
			{
				// usually just a gm command
				new PacketChangeCharacter(this);

				CharDisconnect();	// since there is no undoing this in the client.
				SetTargMode( CLIMODE_SETUP_CHARLIST );
			}
			break;

		case CV_CTAGLIST:
			if ( ! strcmpi( s.GetArgStr(), "log" ))
				pSrc = &g_Serv;
			m_TagDefs.DumpKeys(pSrc, "CTAG.");
			break;

		case CV_CLEARCTAGS:
			{
				if ( s.HasArgs() )
				{
					LPCTSTR pszArgs = s.GetArgStr();
					SKIP_SEPARATORS(pszArgs);
					m_TagDefs.ClearKeys(pszArgs);
				}
				else
				{
					m_TagDefs.ClearKeys();
				}
			} break;

		case CV_CLOSEPAPERDOLL:
			{
				CChar *pChar = m_pChar;
				if ( s.HasArgs() )
				{
					CGrayUID uid = s.GetArgVal();
					pChar = uid.CharFind();
				}
				if ( pChar )
					closeUIWindow(pChar, 0x01);
			}
			break;

		case CV_CLOSEPROFILE:
			{
				CChar *pChar = m_pChar;
				if ( s.HasArgs() )
				{
					CGrayUID uid = s.GetArgVal();
					pChar = uid.CharFind();
				}
				if ( pChar )
					closeUIWindow(pChar, 0x08);
			}
			break;

		case CV_CLOSESTATUS:
			{
				CChar *pChar = m_pChar;
				if ( s.HasArgs() )
				{
					CGrayUID uid = s.GetArgVal();
					pChar = uid.CharFind();
				}
				if ( pChar )
					closeUIWindow(pChar, 0x02);
			}
			break;

		case CV_DYE:
			if ( s.HasArgs() )
			{
				CGrayUID uid(s.GetArgVal());
				CObjBase *pObj = uid.ObjFind();
				if ( pObj )
					addDyeOption(pObj);
			}
			break;

		case CV_EVERBTARG:
			m_Prompt_Text = s.GetArgStr();
			addPromptConsole( CLIMODE_PROMPT_TARG_VERB, m_Targ_Text.IsEmpty() ? "Enter the verb" : "Enter the text",  m_Targ_UID );
			break;

		case CV_EXTRACT:
			// sort of like EXPORT but for statics.
			// Opposite of the "UNEXTRACT" command

			if ( ! s.HasArgs())
			{
				SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_EXTRACT_USAGE ) );
			}
			else
			{
				TCHAR * ppArgs[2];
				Str_ParseCmds( s.GetArgStr(), ppArgs, COUNTOF( ppArgs ));

				m_Targ_Text = ppArgs[0]; // Point at the options, if any
				m_tmTile.m_ptFirst.InitPoint(); // Clear this first
				m_tmTile.m_Code = CV_EXTRACT;	// set extract code.
				m_tmTile.m_id = Exp_GetVal(ppArgs[1]);	// extract id.
				addTarget( CLIMODE_TARG_TILE, g_Cfg.GetDefaultMsg( DEFMSG_SELECT_EXTRACT_AREA ), true );
			}
			break;

		case CV_UNEXTRACT:
			// Create item from script.
			// Opposite of the "EXTRACT" command
			if ( ! s.HasArgs())
			{
				SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_UNEXTRACT_USAGE ) );
			}
			else
			{
				TCHAR * ppArgs[2];
				Str_ParseCmds( s.GetArgStr(), ppArgs, COUNTOF( ppArgs ));

				m_Targ_Text = ppArgs[0]; // Point at the options, if any
				m_tmTile.m_ptFirst.InitPoint(); // Clear this first
				m_tmTile.m_Code = CV_UNEXTRACT;	// set extract code.
				m_tmTile.m_id = Exp_GetVal(ppArgs[1]);	// extract id.

				addTarget( CLIMODE_TARG_UNEXTRACT, g_Cfg.GetDefaultMsg( DEFMSG_SELECT_MULTI_POS ), true );
			}
			break;

		case CV_GMPAGE:
			m_Targ_Text = s.GetArgStr();
			if ( !m_Targ_Text.IsEmpty() && !strnicmp( m_Targ_Text, "ADD ", 4 ) )
			{
				Cmd_GM_Page( m_Targ_Text + 4 );
				break;
			}
			addPromptConsole( CLIMODE_PROMPT_GM_PAGE_TEXT, g_Cfg.GetDefaultMsg( DEFMSG_GMPAGE_PROMPT ) );
			break;
		case CV_GOTARG: // go to my (preselected) target.
			{
				ASSERT(m_pChar);
				CObjBase * pObj = m_Targ_UID.ObjFind();
				if ( pObj != NULL )
				{
					CPointMap po = pObj->GetTopLevelObj()->GetTopPoint();
					CPointMap pnt = po;
					pnt.MoveN( DIR_W, 3 );
					DWORD wBlockFlags = m_pChar->GetMoveBlockFlags();
					pnt.m_z = g_World.GetHeightPoint2( pnt, wBlockFlags );	// ??? Get Area
					m_pChar->m_dirFace = pnt.GetDir( po, m_pChar->m_dirFace ); // Face the player
					m_pChar->Spell_Teleport( pnt, true, false );
				}
			}
			break;
		case CV_INFO:
			// We could also get ground tile info.
			addTarget( CLIMODE_TARG_OBJ_INFO, g_Cfg.GetDefaultMsg( DEFMSG_SELECT_ITEM_INFO ), true, false );
			break;
		case CV_INFORMATION:
			SysMessage( g_Serv.GetStatusString( 0x22 ));
			SysMessage( g_Serv.GetStatusString( 0x24 ));
			break;
		case CV_LAST:
			// Fake Previous target.
			if ( GetTargMode() >= CLIMODE_MOUSE_TYPE )
			{
				ASSERT(m_pChar);
				CObjBase * pObj = m_pChar->m_Act_Targ.ObjFind();
				if ( pObj != NULL )
				{
					Event_Target(GetTargMode(), pObj->GetUID(), pObj->GetTopPoint());
					addTargetCancel();
				}
				break;
			}
			return( false );
		case CV_LINK:	// link doors
			m_Targ_UID.InitUID();
			addTarget( CLIMODE_TARG_LINK, g_Cfg.GetDefaultMsg( DEFMSG_SELECT_LINK_ITEM ) );
			break;

		case CV_MENU:
			Menu_Setup( g_Cfg.ResourceGetIDType( RES_MENU, s.GetArgStr()));
			break;
		case CV_MIDILIST:
			{
				INT64 piMidi[64];
				size_t iQty = Str_ParseCmds( s.GetArgStr(), piMidi, COUNTOF(piMidi));
				if ( iQty > 0 )
				{
					addMusic( static_cast<MIDI_TYPE>(piMidi[ Calc_GetRandVal( iQty ) ]) );
				}
			}
			break;
		case CV_NUDGE:
			if ( ! s.HasArgs())
			{
				SysMessage( "Usage: NUDGE dx dy dz" );
			}
			else
			{
				m_Targ_Text = s.GetArgRaw();
				m_tmTile.m_ptFirst.InitPoint(); // Clear this first
				m_tmTile.m_Code = CV_NUDGE;
				addTarget( CLIMODE_TARG_TILE, g_Cfg.GetDefaultMsg( DEFMSG_SELECT_NUDGE_AREA ), true );
			}
			break;

		case CV_NUKE:
			m_Targ_Text = s.GetArgRaw();
			m_tmTile.m_ptFirst.InitPoint(); // Clear this first
			m_tmTile.m_Code = CV_NUKE;	// set nuke code.
			addTarget( CLIMODE_TARG_TILE, g_Cfg.GetDefaultMsg( DEFMSG_SELECT_NUKE_AREA ), true );
			break;
		case CV_NUKECHAR:
			m_Targ_Text = s.GetArgRaw();
			m_tmTile.m_ptFirst.InitPoint(); // Clear this first
			m_tmTile.m_Code = CV_NUKECHAR;	// set nuke code.
			addTarget( CLIMODE_TARG_TILE, g_Cfg.GetDefaultMsg( DEFMSG_SELECT_NUKE_CHAR_AREA ), true );
			break;

		case CV_OPENPAPERDOLL:
		{
			CChar *pChar = m_pChar;
			if ( s.HasArgs() )
			{
				CGrayUID uid = s.GetArgVal();
				pChar = uid.CharFind();
			}
			if ( pChar )
				addCharPaperdoll(pChar);
			break;
		}

		case CV_OPENTRADEWINDOW:
		{
			TCHAR *ppArgs[2];
			Str_ParseCmds(s.GetArgStr(), ppArgs, COUNTOF(ppArgs));

			CChar *pChar = NULL;
			CItem *pItem = NULL;
			if ( ppArgs[0] )
			{
				CGrayUID uidChar = static_cast<CGrayUID>(Exp_GetVal(ppArgs[0]));
				pChar = uidChar.CharFind();
			}
			if ( ppArgs[1] )
			{
				CGrayUID uidItem = static_cast<CGrayUID>(Exp_GetVal(ppArgs[1]));
				pItem = uidItem.ItemFind();
			}
			if ( pChar )
				Cmd_SecureTrade(pChar, pItem);
			break;
		}

		case CV_PAGE:
			Cmd_GM_PageCmd( s.GetArgStr());
			break;
		case CV_REPAIR:
			addTarget( CLIMODE_TARG_REPAIR, g_Cfg.GetDefaultMsg( DEFMSG_SELECT_ITEM_REPAIR ) );
			break;
		case CV_FLUSH:
#ifndef _MTNETWORK
			g_NetworkOut.flush(this);
#else
			g_NetworkManager.flush(m_NetState);
#endif
			break;
		case CV_RESEND:
			addReSync();
			break;
		case CV_SAVE:
			g_World.Save(s.GetArgVal() != 0);
			break;
		case CV_SCROLL:
			// put a scroll up.
			addScrollResource( s.GetArgStr(), SCROLL_TYPE_UPDATES );
			break;
		case CV_SENDPACKET:
			SendPacket( s.GetArgStr() );
			break;
		case CV_SELF:
			// Fake self target.
			if ( GetTargMode() >= CLIMODE_MOUSE_TYPE )
			{
				ASSERT(m_pChar);
				Event_Target(GetTargMode(), m_pChar->GetUID(), m_pChar->GetTopPoint());
				addTargetCancel();
				break;
			}
			return( false );
		case CV_SHOWSKILLS:
			addSkillWindow(static_cast<SKILL_TYPE>(g_Cfg.m_iMaxSkill)); // Reload the real skills
			break;
		case CV_SKILLMENU:				// Just put up another menu.
			Cmd_Skill_Menu( g_Cfg.ResourceGetIDType( RES_SKILLMENU, s.GetArgStr()));
			break;
		case CV_SKILLSELECT:
			Event_Skill_Use( g_Cfg.FindSkillKey( s.GetArgStr() ) );
			break;
		case CV_SUMMON:
		{
			ASSERT(m_pChar);
			const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(SPELL_Summon);
			if ( !pSpellDef )
				return false;

			m_pChar->m_Act_Targ = m_pChar->GetUID();
			m_pChar->m_Act_TargPrv = m_pChar->GetUID();

			if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_OBJ|SPELLFLAG_TARG_XYZ) )
			{
				m_tmSkillMagery.m_Spell = SPELL_Summon;
				m_tmSkillMagery.m_SummonID = static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType(RES_CHARDEF, s.GetArgStr()));

				LPCTSTR pPrompt = g_Cfg.GetDefaultMsg(DEFMSG_SELECT_MAGIC_TARGET);
				if ( !pSpellDef->m_sTargetPrompt.IsEmpty() )
					pPrompt = pSpellDef->m_sTargetPrompt;

				int SpellTimeout = static_cast<int>(GetDefNum("SPELLTIMEOUT"));
				if ( !SpellTimeout )
					SpellTimeout = g_Cfg.m_iSpellTimeout * TICK_PER_SEC;

				addTarget(CLIMODE_TARG_SKILL_MAGERY, pPrompt, pSpellDef->IsSpellType(SPELLFLAG_TARG_XYZ), pSpellDef->IsSpellType(SPELLFLAG_HARM), SpellTimeout);
				break;
			}
			else
			{
				m_pChar->m_atMagery.m_Spell = SPELL_Summon;
				m_pChar->m_atMagery.m_SummonID = static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType(RES_CHARDEF, s.GetArgStr()));

				if ( IsSetMagicFlags(MAGICF_PRECAST) && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST) )
				{
					m_pChar->Spell_CastDone();
					break;
				}
				else
				{
					int skill;
					if ( !pSpellDef->GetPrimarySkill(&skill, NULL) )
						return false;

					m_pChar->Skill_Start(static_cast<SKILL_TYPE>(skill));
				}
			}
			break;
		}
		case CV_SMSG:
		case CV_SYSMESSAGE:
			SysMessage( s.GetArgStr() );
			break;
		case CV_SYSMESSAGEF: //There is still an issue with numbers not resolving properly when %i,%d,or other numeric format code is in use
			{
				TCHAR * pszArgs[4];
				size_t iArgQty = Str_ParseCmds( s.GetArgRaw(), pszArgs, COUNTOF(pszArgs) );
				if ( iArgQty < 2 )
				{
					g_Log.EventError("SysMessagef with less than 1 args for the given text\n");
					return false;
				}
				if ( iArgQty > 4 )
				{
					g_Log.EventError("Too many arguments given to SysMessagef (max = text + 3\n");
					return false;
				}
				//strip quotes if any
				if ( *pszArgs[0] == '"' )
					pszArgs[0]++;
				for (TCHAR * pEnd = pszArgs[0] + strlen( pszArgs[0] ) - 1; pEnd >= pszArgs[0]; pEnd-- )
				{
					if ( *pEnd == '"' )
					{
						*pEnd = '\0';
						break;
					}
				}
				SysMessagef( pszArgs[0], pszArgs[1], pszArgs[2] ? pszArgs[2] : 0, pszArgs[3] ? pszArgs[3] : 0);
			}break;
		case CV_SMSGU:
		case CV_SYSMESSAGEUA:
			{
				TCHAR * pszArgs[5];
				size_t iArgQty = Str_ParseCmds( s.GetArgRaw(), pszArgs, COUNTOF(pszArgs) );
				if ( iArgQty > 4 )
				{
					// Font and mode are actually ignored here, but they never made a difference
					// anyway.. I'd like to keep the syntax similar to SAYUA
			 		NCHAR szBuffer[ MAX_TALK_BUFFER ];
					CvtSystemToNUNICODE( szBuffer, COUNTOF(szBuffer), pszArgs[4], -1 );

					addBarkUNICODE( szBuffer, NULL, static_cast<HUE_TYPE>(Exp_GetVal(pszArgs[0])), TALKMODE_SYSTEM, FONT_NORMAL, pszArgs[3] );
				}
			}
			break;
		case CV_SMSGL:
		case CV_SYSMESSAGELOC:
			{
				TCHAR * ppArgs[256];
				size_t iArgQty = Str_ParseCmds( s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), "," );
				if ( iArgQty > 1 )
				{
					int hue = -1;
					if ( ppArgs[0] )
						hue = Exp_GetVal( ppArgs[0] );
					int iClilocId = Exp_GetVal( ppArgs[1] );

					if ( hue == -1 )	hue = HUE_TEXT_DEF;

					CGString CArgs;
					for ( size_t i = 2; i < iArgQty; i++ )
					{
						if ( CArgs.GetLength() )
							CArgs += "\t";
						CArgs += ( !strcmp(ppArgs[i], "NULL") ? " " : ppArgs[i] );
					}

					addBarkLocalized(iClilocId, NULL, static_cast<HUE_TYPE>(hue), TALKMODE_SYSTEM, FONT_NORMAL, CArgs.GetPtr());
				}
			}
			break;
		case CV_SMSGLEX:
		case CV_SYSMESSAGELOCEX:
			{
				TCHAR * ppArgs[256];
				size_t iArgQty = Str_ParseCmds( s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), "," );
				if ( iArgQty > 2 )
				{
					int hue = -1;
					int affix = 0;
					if ( ppArgs[0] )
						hue = Exp_GetVal( ppArgs[0] );
					int iClilocId = Exp_GetVal( ppArgs[1] );
					if ( ppArgs[2] )
						affix = Exp_GetVal( ppArgs[2] );

					if ( hue == -1 )	hue = HUE_TEXT_DEF;

					CGString CArgs;
					for ( size_t i = 4; i < iArgQty; i++ )
					{
						if ( CArgs.GetLength() )
							CArgs += "\t";
						CArgs += ( !strcmp(ppArgs[i], "NULL") ? " " : ppArgs[i] );
					}

					addBarkLocalizedEx( iClilocId, NULL, static_cast<HUE_TYPE>(hue), TALKMODE_SYSTEM, FONT_NORMAL, static_cast<AFFIX_TYPE>(affix), ppArgs[3], CArgs.GetPtr() );
				}
			}
			break;
		case CV_TELE:
			Cmd_Skill_Magery(SPELL_Teleport, dynamic_cast<CObjBase *>(pSrc));
			break;
		case CV_TILE:
			if ( ! s.HasArgs())
			{
				SysMessage( "Usage: TILE z-height item1 item2 itemX" );
			}
			else
			{
				m_Targ_Text = s.GetArgStr(); // Point at the options
				m_tmTile.m_ptFirst.InitPoint(); // Clear this first
				m_tmTile.m_Code = CV_TILE;
				addTarget( CLIMODE_TARG_TILE, "Pick 1st corner:", true );
			}
			break;
		case CV_VERSION:	// "SHOW VERSION"
			SysMessage(g_szServerDescription);
			break;
		case CV_WEBLINK:
			addWebLaunch( s.GetArgStr());
			break;
		default:
			if ( r_LoadVal( s ))
			{
				CGString sVal;
				if ( r_WriteVal( s.GetKey(), sVal, pSrc ))
				{
					// if ( !s.IsKeyHead( "CTAG.", 5 ) && !s.IsKeyHead( "CTAG0.", 6 ) ) // We don't want output related to ctag
					//	SysMessagef( "%s = %s", (LPCTSTR) s.GetKey(), (LPCTSTR) sVal );	// feedback on what we just did.

					return( true );
				}
			}
			return( CScriptObj::r_Verb( s, pSrc ));	// used in the case of web pages to access server level things..
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

long CClient::GetSocketID() const
{
	return m_NetState->id();
}

CSocketAddress &CClient::GetPeer()
{
	return m_NetState->m_peerAddress;
}

LPCTSTR CClient::GetPeerStr() const
{
	return m_NetState->m_peerAddress.GetAddrStr();
}
