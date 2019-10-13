#include "graysvr.h"	// predef header
#include "../network/send.h"

#if !defined(_WIN32) || defined(_LIBEV)
	extern LinuxEv g_NetworkEvent;
#endif

///////////////////////////////////////////////////////////
// CClient

CClient::CClient(NetState *state)
{
	m_pChar = NULL;

	m_Env.SetInvalid();

	m_fUpdateStats = 0;

	m_iWalkTimeAvg = 100;
	m_iWalkStepCount = 0;
	m_timeWalkStep = GetTickCount64();

	m_ScreenSize.x = m_ScreenSize.y = 0;

	m_Prompt_Mode = CLIMODE_NORMAL;

	SetConnectType(CONNECT_UNK);	// don't know what sort of connect this is yet
	m_NetState = state;
	m_pAccount = NULL;

	m_FeatureFlags = 0;
	m_CharacterListFlags = 0;
	m_TooltipEnabled = false;
	m_ContainerGridEnabled = false;
	m_UseNewChatSystem = false;
	m_UseMultiSight = IsSetOF(OF_OSIMultiSight);

	m_timeLogin.Init();
	m_timeLastEvent = CServTime::GetCurrentTime();
	m_timeLastEventItemPickup = CServTime::GetCurrentTime();
	m_timeLastEventWarMode = CServTime::GetCurrentTime();
	m_timeLastEventWalk = CServTime::GetCurrentTime();
	m_timeNextEventWalk = 0;

	m_pGMPage = NULL;

	m_timeLastSkillThrowing.Init();
	m_pSkillThrowingTarg = NULL;
	m_SkillThrowingAnimID = ITEMID_NOTHING;
	m_SkillThrowingAnimHue = 0;
	m_SkillThrowingAnimRender = 0;

	m_Targ_Mode = CLIMODE_SETUP_CONNECTING;
	m_Targ_Timeout.Init();

	m_Crypt.SetClientVer(g_Serv.m_ClientVersion);

	m_pPopupPacket = NULL;

	m_zLastMessage[0] = 0;
	m_zLastObjMessage[0] = 0;

	m_pHouseDesign = NULL;

	// Update IP history
#ifdef _MTNETWORK
	HistoryIP &history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(GetPeer());
#else
	HistoryIP &history = g_NetworkIn.getIPHistoryManager().getHistoryForIP(GetPeer());
#endif
	++history.m_connecting;
	++history.m_connected;

	g_Log.Event(LOGM_CLIENTS_LOG, "%lx:Client connected [Total:%" FMTDWORD "] ('%s' %ld/%ld)\n", GetSocketID(), g_Serv.StatGet(SERV_STAT_CLIENTS), GetPeerStr(), history.m_connecting, history.m_connected);
}

CClient::~CClient()
{
	// Update IP history
#ifdef _MTNETWORK
	HistoryIP &history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(GetPeer());
#else
	HistoryIP &history = g_NetworkIn.getIPHistoryManager().getHistoryForIP(GetPeer());
#endif
	if ( GetConnectType() != CONNECT_GAME )
		--history.m_connecting;
	--history.m_connected;

	bool fWasChar = (m_pChar != NULL);
	CharDisconnect();	// am i a char in game ?
	if ( m_pGMPage )
		m_pGMPage->ClearHandler();

	// Clear containers (CTAG and TOOLTIP)
	m_TagDefs.Empty();
	m_TooltipData.Clean(true);

	if ( m_pAccount )
	{
		m_pAccount->OnLogout(this, fWasChar);
		m_pAccount = NULL;
	}

	if ( m_pPopupPacket )
	{
		delete m_pPopupPacket;
		m_pPopupPacket = NULL;
	}

	if ( !m_NetState->isClosed() )
		g_Log.EventError("Client being deleted without being safely removed from the network system\n");
}

bool CClient::CanInstantLogOut() const
{
	ADDTOCALLSTACK("CClient::CanInstantLogOut");
	if ( g_Serv.IsLoading() )
		return true;
	if ( !g_Cfg.m_iClientLingerTime || IsPriv(PRIV_GM) )
		return true;
	if ( !m_pChar || m_pChar->IsStatFlag(STATF_DEAD) )
		return true;
	if ( !m_pChar->m_pArea || m_pChar->m_pArea->IsFlag(REGION_FLAG_INSTA_LOGOUT) )
		return true;
	if ( m_pChar->m_pRoom && m_pChar->m_pRoom->IsFlag(REGION_FLAG_INSTA_LOGOUT) )
		return true;

	return false;
}

void CClient::CharDisconnect()
{
	ADDTOCALLSTACK("CClient::CharDisconnect");
	// Disconnect the CChar from the client.
	// Even tho the CClient might stay active.
	if ( !m_pChar )
		return;

	Announce(false);
	bool fCanInstaLogOut = CanInstantLogOut();
	int iLingerTime = g_Cfg.m_iClientLingerTime;

	// We are not a client anymore
	if ( m_bChatActive )
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

	if ( iLingerTime <= 0 )
		fCanInstaLogOut = true;

	// Gump memory cleanup, we don't want them on logged out players
	m_mapOpenedGumps.clear();

	// Layer dragging, moving it to backpack
	CItem *pItemDragging = m_pChar->LayerFind(LAYER_DRAGGING);
	if ( pItemDragging )
		m_pChar->ItemBounce(pItemDragging);

	// log out immediately ? (test before ClientDetach())
	if ( !fCanInstaLogOut )
	{
		// become an NPC for a little while
		CItem *pItemChange = CItem::CreateBase(ITEMID_RHAND_POINT_W);
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

	m_pAccount = NULL;
	m_pChar = NULL;
}

void CClient::SysMessage(LPCTSTR pszMsg) const	// system message (in lower left corner)
{
	ADDTOCALLSTACK("CClient::SysMessage");
	// Diff sorts of clients.
	if ( !pszMsg || !*pszMsg )
		return;

	switch ( GetConnectType() )
	{
		case CONNECT_CRYPT:
		case CONNECT_LOGIN:
		case CONNECT_GAME:
		{
			const_cast<CClient *>(this)->addSysMessage(pszMsg);
			return;
		}
		case CONNECT_HTTP:
		{
			const_cast<CClient *>(this)->m_Targ_Text = pszMsg;
			return;
		}
		case CONNECT_TELNET:
		case CONNECT_AXIS:
		{
			if ( ISINTRESOURCE(pszMsg) || (*pszMsg == '\0') )
				return;
			new PacketTelnet(this, pszMsg);
			return;
		}
		case CONNECT_UOG:
		{
			if ( ISINTRESOURCE(pszMsg) || (*pszMsg == '\0') )
				return;
			new PacketTelnet(this, pszMsg, true);
			return;
		}
	}
}

void CClient::Announce(bool fArrive) const
{
	ADDTOCALLSTACK("CClient::Announce");
	if ( !m_pAccount || !m_pChar || !m_pChar->m_pPlayer )
		return;

	// We have logged in or disconnected.
	// Annouce my arrival or departure.
	TCHAR *pszMsg = Str_GetTemp();
	if ( (g_Cfg.m_iArriveDepartMsg == 2) && (GetPrivLevel() > PLEVEL_Player) )		// notify of GMs
	{
		LPCTSTR pszTitle = m_pChar->Noto_GetFameTitle();
		sprintf(pszMsg, "@231 STAFF: %s%s logged %s.", pszTitle, m_pChar->GetName(), fArrive ? "in" : "out");
	}
	else if ( g_Cfg.m_iArriveDepartMsg == 1 )		// notify of players
	{
		const CRegionBase *pRegion = m_pChar->GetTopPoint().GetRegion(REGION_TYPE_AREA);
		sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MSG_ARRDEP_1), m_pChar->GetName(), g_Cfg.GetDefaultMsg(fArrive ? DEFMSG_MSG_ARRDEP_2 : DEFMSG_MSG_ARRDEP_3), pRegion ? pRegion->GetName() : g_Serv.GetName());
	}
	if ( pszMsg )
	{
		ClientIterator it;
		for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
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
		if ( fArrive )
		{
			// On client login, set active timer on murder memory
			pMurders->SetTimeout(static_cast<INT64>(pMurders->m_itEqMurderCount.m_Decay_Balance) * TICK_PER_SEC);
		}
		else
		{
			// Or make it inactive on logout
			pMurders->m_itEqMurderCount.m_Decay_Balance = static_cast<DWORD>(pMurders->GetTimerAdjusted());
			pMurders->SetTimeout(-1);
		}
	}
	else if ( fArrive )
	{
		// If there's no murder memory found, check if we need a new memory
		m_pChar->Noto_Murder();
	}
}

///////////////////////////////////////////////////////////

bool CClient::CanSee(const CObjBaseTemplate *pObj) const
{
	ADDTOCALLSTACK("CClient::CanSee");
	// Can player see item b
	if ( !m_pChar || !pObj )
		return false;

	if ( !IsPriv(PRIV_ALLSHOW) && pObj->IsChar() )
	{
		const CChar *pChar = static_cast<const CChar *>(pObj);
		if ( pChar->IsDisconnected() )
			return false;
	}
	return m_pChar->CanSee(pObj);
}

bool CClient::CanHear(const CObjBaseTemplate *pSrc, TALKMODE_TYPE mode) const
{
	ADDTOCALLSTACK("CClient::CanHear");
	// can we hear this text or sound.

	if ( !IsConnectTypePacket() )
		return false;
	if ( !pSrc )
		return true;
	if ( !m_pChar )
		return false;

	if ( IsPriv(PRIV_HEARALL) && pSrc->IsChar() && ((mode == TALKMODE_SAY) || (mode == TALKMODE_WHISPER) || (mode == TALKMODE_YELL)) )
	{
		const CChar *pChar = static_cast<const CChar *>(pSrc);
		if ( GetPrivLevel() >= pChar->GetPrivLevel() )
			return true;
	}

	return m_pChar->CanHear(pSrc, mode);
}

///////////////////////////////////////////////////////////

void CClient::addTargetVerb(LPCTSTR pszCmd, LPCTSTR pszArgs)
{
	ADDTOCALLSTACK("CClient::addTargetVerb");
	// Target a verb at some object

	ASSERT(pszCmd);
	GETNONWHITESPACE(pszCmd);
	SKIP_SEPARATORS(pszCmd);

	if ( !strlen(pszCmd) )
		pszCmd = pszArgs;

	if ( pszCmd == pszArgs )
	{
		GETNONWHITESPACE(pszCmd);
		SKIP_SEPARATORS(pszCmd);
		pszArgs = "";
	}

	// priv here
	PLEVEL_TYPE ilevel = g_Cfg.GetPrivCommandLevel(pszCmd);
	if ( ilevel > GetPrivLevel() )
		return;

	m_Targ_Text.Format("%s%s%s", pszCmd, (pszArgs[0] && pszCmd[0]) ? " " : "", pszArgs);
	TCHAR *pszMsg = Str_GetTemp();
	sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_TARGET_COMMAND), static_cast<LPCTSTR>(m_Targ_Text));
	addTarget(CLIMODE_TARG_OBJ_SET, pszMsg);
}

void CClient::addTargetFunctionMulti(LPCTSTR pszFunction, ITEMID_TYPE itemid, HUE_TYPE wHue, bool fAllowGround)
{
	ADDTOCALLSTACK("CClient::addTargetFunctionMulti");
	// Target a verb at some object
	ASSERT(pszFunction);
	GETNONWHITESPACE(pszFunction);
	SKIP_SEPARATORS(pszFunction);

	m_Targ_Text = pszFunction;
	if ( CItemBase::IsID_Multi(itemid) )	// a multi we get from multi.mul
	{
		SetTargMode(CLIMODE_TARG_OBJ_FUNC);
		new PacketAddTarget(this, fAllowGround ? PacketAddTarget::Ground : PacketAddTarget::Object, CLIMODE_TARG_OBJ_FUNC, PacketAddTarget::None, itemid, wHue);
	}
	addTargetFunction(pszFunction, fAllowGround, false);
}

void CClient::addTargetFunction(LPCTSTR pszFunction, bool fAllowGround, bool fCheckCrime)
{
	ADDTOCALLSTACK("CClient::addTargetFunction");
	// Target a verb at some object
	ASSERT(pszFunction);
	GETNONWHITESPACE(pszFunction);
	SKIP_SEPARATORS(pszFunction);

	m_Targ_Text = pszFunction;
	addTarget(CLIMODE_TARG_OBJ_FUNC, NULL, fAllowGround, fCheckCrime);
}

void CClient::addPromptConsoleFunction(LPCTSTR pszFunction, LPCTSTR pszArgs, bool fUnicode)
{
	ADDTOCALLSTACK("CClient::addPromptConsoleFunction");
	// Target a verb at some object
	ASSERT(pszFunction);
	m_Prompt_Text = pszFunction;
	addPromptConsole(CLIMODE_PROMPT_SCRIPT_VERB, pszArgs, 0, 0, fUnicode);
}

///////////////////////////////////////////////////////////

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
	// 0x002000		Enable extra roleplay face styles on character creation	(enhanced clients only)
	// 0x004000		Trial account
	// 0x008000		Live account (required on clients 4.0.0+, otherwise bits 3..14 will be ignored)
	// 0x010000		Enable SA features
	// 0x020000		Enable HS features
	// 0x040000		Enable gothic custom house tiles
	// 0x080000		Enable rustic custom house tiles
	// 0x100000		Enable jungle custom house tiles
	// 0x200000		Enable shadowguard custom house tiles
	// 0x400000		Enable TOL features
	// 0x800000		Free account (Endless Journey)

	if ( !m_pAccount )
		return;

	RESDISPLAY_VERSION iResDisp = static_cast<RESDISPLAY_VERSION>(m_pAccount->GetResDisp());
	BYTE bMaxChars = static_cast<BYTE>(maximum(m_pAccount->GetMaxChars(), m_pAccount->m_Chars.GetCharCount()));
	m_FeatureFlags = 0;

	// Expansion features
	if ( iResDisp >= RDS_T2A )
	{
		if ( g_Cfg.m_iFeatureT2A & FEATURE_T2A_UPDATE )
			m_FeatureFlags |= 0x4;
		if ( g_Cfg.m_iFeatureT2A & FEATURE_T2A_CHAT )
			m_FeatureFlags |= 0x1;
	}

	if ( iResDisp >= RDS_LBR )
	{
		if ( g_Cfg.m_iFeatureLBR & FEATURE_LBR_UPDATE )
			m_FeatureFlags |= 0x8;
		if ( g_Cfg.m_iFeatureLBR & FEATURE_LBR_SOUND )
			m_FeatureFlags |= 0x2;
	}

	if ( iResDisp >= RDS_AOS )
	{
		if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_A )
			m_FeatureFlags |= 0x10;
	}

	if ( iResDisp >= RDS_SE )
	{
		if ( g_Cfg.m_iFeatureSE & FEATURE_SE_UPDATE )
			m_FeatureFlags |= 0x40;
	}

	if ( iResDisp >= RDS_ML )
	{
		if ( g_Cfg.m_iFeatureML & FEATURE_ML_UPDATE )
			m_FeatureFlags |= 0x80;
	}

	if ( iResDisp >= RDS_SA )
	{
		if ( g_Cfg.m_iFeatureSA & FEATURE_SA_UPDATE )
			m_FeatureFlags |= 0x10000;
	}

	if ( iResDisp >= RDS_TOL )
	{
		if ( g_Cfg.m_iFeatureTOL & FEATURE_TOL_UPDATE )
			m_FeatureFlags |= 0x400000;
	}

	// Max character slots
	if ( bMaxChars > 6 )
		m_FeatureFlags |= 0x1000;
	if ( bMaxChars == 6 )
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

	// Misc
	if ( g_Cfg.m_iFeatureExtra & FEATURE_EXTRA_ROLEPLAYFACES )
		m_FeatureFlags |= 0x2000;
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

	RESDISPLAY_VERSION iResDisp = static_cast<RESDISPLAY_VERSION>(m_pAccount->GetResDisp());
	BYTE bMaxChars = static_cast<BYTE>(maximum(m_pAccount->GetMaxChars(), m_pAccount->m_Chars.GetCharCount()));
	m_CharacterListFlags = 0;

	// Expansion features
	if ( iResDisp >= RDS_AOS )
	{
		if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B )
			m_CharacterListFlags |= 0x20;
		if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_POPUP )
			m_CharacterListFlags |= 0x8;
	}

	if ( iResDisp >= RDS_SE )
	{
		if ( g_Cfg.m_iFeatureSE & FEATURE_SE_NINJASAM )
			m_CharacterListFlags |= 0x80;
	}

	if ( iResDisp >= RDS_ML )
	{
		if ( g_Cfg.m_iFeatureML & FEATURE_ML_UPDATE )
			m_CharacterListFlags |= 0x100;
	}

	if ( iResDisp >= RDS_KR )
	{
		if ( g_Cfg.m_iFeatureKR & FEATURE_KR_UPDATE )
			m_CharacterListFlags |= 0x200;
	}

	if ( iResDisp >= RDS_SA )
	{
		if ( g_Cfg.m_iFeatureSA & FEATURE_SA_MOVEMENT )
			m_CharacterListFlags |= 0x4000;
	}

	// Max character slots
	if ( bMaxChars > 6 )
		m_CharacterListFlags |= 0x1000;
	else if ( bMaxChars == 6 )
		m_CharacterListFlags |= 0x40;
	else if ( bMaxChars == 1 )
		m_CharacterListFlags |= (0x10|0x4);

	// Misc
	if ( m_NetState->isClientKR() )
		m_CharacterListFlags |= (0x800|0x400);
	else if ( m_NetState->isClientEnhanced() || m_NetState->isClientVersion(MINCLIVER_SA) )
		m_CharacterListFlags |= (0x2000|0x800|0x400);

	m_TooltipEnabled = ((m_CharacterListFlags & 0x20) || m_NetState->isClientKR() || m_NetState->isClientEnhanced());
	m_ContainerGridEnabled = (m_NetState->isClientVersion(MINCLIVER_CONTAINERGRID) || m_NetState->isClientKR() || m_NetState->isClientEnhanced());
	m_UseNewChatSystem = m_NetState->isClientVersion(MINCLIVER_NEWCHATSYSTEM);
}

///////////////////////////////////////////////////////////

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

LPCTSTR const CClient::sm_szRefKeys[CLIR_QTY + 1] =
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

bool CClient::r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef)
{
	ADDTOCALLSTACK("CClient::r_GetRef");
	int index = FindTableHeadSorted(pszKey, sm_szRefKeys, COUNTOF(sm_szRefKeys) - 1);
	if ( index >= 0 )
	{
		pszKey += strlen(sm_szRefKeys[index]);
		SKIP_SEPARATORS(pszKey);
		switch ( index )
		{
			case CLIR_ACCOUNT:
				if ( pszKey[-1] != '.' )	// only used as a ref !
					break;
				pRef = m_pAccount;
				return true;
			case CLIR_GMPAGEP:
				pRef = m_pGMPage;
				return true;
			case CLIR_HOUSEDESIGN:
				pRef = m_pHouseDesign;
				return true;
			case CLIR_PARTY:
				if ( !m_pChar->m_pParty )
				{
					LPCTSTR pszKeyOld = pszKey;
					if ( !strnicmp(pszKey, "CREATE", 7) )
						pszKey += 7;

					SKIP_SEPARATORS(pszKey);
					CChar *pChar = static_cast<CGrayUID>(Exp_GetLLSingle(pszKey)).CharFind();
					if ( !pChar || !pChar->m_pClient )
						return false;

					CPartyDef::AcceptEvent(pChar, m_pChar, true);
					if ( !m_pChar->m_pParty )
						return false;

					pszKey = pszKeyOld;	// restoring back to real pszKey, so we don't get errors for giving an uid instead of PDV_CREATE
				}
				pRef = m_pChar->m_pParty;
				return true;
			case CLIR_TARG:
				pRef = m_Targ_UID.ObjFind();
				return true;
			case CLIR_TARGPRV:
				pRef = m_Targ_PrvUID.ObjFind();
				return true;
			case CLIR_TARGPROP:
				pRef = m_Prop_UID.ObjFind();
				return true;
		}
	}
	return CScriptObj::r_GetRef(pszKey, pRef);
}

LPCTSTR const CClient::sm_szLoadKeys[CC_QTY + 1] =	// static
{
	#define ADD(a,b) b,
	#include "../tables/CClient_props.tbl"
	#undef ADD
	NULL
};

LPCTSTR const CClient::sm_szVerbKeys[CV_QTY + 1] =	// static
{
	#define ADD(a,b) b,
	#include "../tables/CClient_functions.tbl"
	#undef ADD
	NULL
};

bool CClient::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CClient::r_WriteVal");
	EXC_TRY("WriteVal");

	if ( !strnicmp("CTAG.", pszKey, 5) )
	{
		if ( pszKey[4] != '.' )
			return false;
		pszKey += 5;
		CVarDefCont *pVar = m_TagDefs.GetKey(pszKey);
		sVal = pVar ? pVar->GetValStr() : "";
		return true;
	}

	if ( !strnicmp("CTAG0.", pszKey, 6) )
	{
		if ( pszKey[5] != '.' )
			return false;
		pszKey += 6;
		CVarDefCont *pVar = m_TagDefs.GetKey(pszKey);
		sVal = pVar ? pVar->GetValStr() : "0";
		return true;
	}

	int index;
	if ( !strnicmp("TARGP", pszKey, 5) && ((pszKey[5] == '\0') || (pszKey[5] == '.')) )
		index = CC_TARGP;
	else if ( !strnicmp("SCREENSIZE", pszKey, 10) && ((pszKey[10] == '\0') || (pszKey[10] == '.')) )
		index = CC_SCREENSIZE;
	else if ( !strnicmp("REPORTEDCLIVER", pszKey, 14) && ((pszKey[14] == '\0') || (pszKey[14] == '.')) )
		index = CC_REPORTEDCLIVER;
	else
		index = FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);

	switch ( index )
	{
		case CC_ALLMOVE:
			sVal.FormatVal(IsPriv(PRIV_ALLMOVE));
			break;
		case CC_ALLSHOW:
			sVal.FormatVal(IsPriv(PRIV_ALLSHOW));
			break;
		case CC_CLIENTIS3D:
			sVal.FormatVal(m_NetState->isClient3D());
			break;
		case CC_CLIENTISKR:
			sVal.FormatVal(m_NetState->isClientKR());
			break;
		case CC_CLIENTISSA:
			sVal.FormatVal(m_NetState->isClientEnhanced());
			break;
		case CC_CLIENTVERSION:
		{
			TCHAR szVersion[128];
			sVal = m_Crypt.WriteClientVerString(m_Crypt.GetClientVer(), szVersion);
			break;
		}
		case CC_DEBUG:
			sVal.FormatVal(IsPriv(PRIV_DEBUG));
			break;
		case CC_DETAIL:
			sVal.FormatVal(IsPriv(PRIV_DETAIL));
			break;
		case CC_GM:
			sVal.FormatVal(IsPriv(PRIV_GM));
			break;
		case CC_HEARALL:
			sVal.FormatVal(IsPriv(PRIV_HEARALL));
			break;
		case CC_LASTEVENT:
			sVal.FormatLLVal(m_timeLastEvent.GetTimeRaw());
			break;
		case CC_PRIVSHOW:
			sVal.FormatVal(!IsPriv(PRIV_PRIV_NOSHOW));
			break;
		case CC_REPORTEDCLIVER:
		{
			pszKey += 14;
			GETNONWHITESPACE(pszKey);

			DWORD dwCliVer = m_NetState->getReportedVersion();
			if ( pszKey[0] == '\0' )
			{
				// Return full version string (eg: 5.0.2d)
				TCHAR szVersion[128];
				sVal = CCrypt::WriteClientVerString(dwCliVer, szVersion);
			}
			else
			{
				// Return raw version number (eg: 5.0.2d = 5000204)
				sVal.FormatUVal(dwCliVer);
			}
			break;
		}
		case CC_SCREENSIZE:
		{
			if ( pszKey[10] == '.' )
			{
				pszKey += 10;
				SKIP_SEPARATORS(pszKey);

				if ( !strnicmp("X", pszKey, 1) )
					sVal.Format("%hu", m_ScreenSize.x);
				else if ( !strnicmp("Y", pszKey, 1) )
					sVal.Format("%hu", m_ScreenSize.y);
				else
					return false;
			}
			else
				sVal.Format("%hu,%hu", m_ScreenSize.x, m_ScreenSize.y);
			break;
		}
		case CC_TARG:
			sVal.FormatHex(m_Targ_UID);
			break;
		case CC_TARGP:
		{
			if ( pszKey[5] == '.' )
				return m_Targ_p.r_WriteVal(pszKey + 6, sVal);
			sVal = m_Targ_p.WriteUsed();
			break;
		}
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
			return CScriptObj::r_WriteVal(pszKey, sVal, pSrc);
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CClient::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CClient::r_LoadVal");
	EXC_TRY("LoadVal");
	if ( !m_pAccount )
		return false;

	LPCTSTR pszKey = s.GetKey();

	if ( s.IsKeyHead("CTAG.", 5) || s.IsKeyHead("CTAG0.", 6) )
	{
		bool fZero = s.IsKeyHead("CTAG0.", 6);
		bool fQuoted = false;

		pszKey = pszKey + (fZero ? 6 : 5);
		m_TagDefs.SetStr(pszKey, fQuoted, s.GetArgStr(&fQuoted), fZero);
		return true;
	}

	switch ( FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case CC_ALLMOVE:
		{
			m_pAccount->TogPrivFlags(PRIV_ALLMOVE, s.GetArgStr());
			addPlayerSee(NULL);
			if ( IsSetOF(OF_Command_Sysmsgs) )
				m_pChar->SysMessagef("AllMove %s", g_Cfg.GetDefaultMsg(IsPriv(PRIV_ALLMOVE) ? DEFMSG_CMD_TOGGLE_ON : DEFMSG_CMD_TOGGLE_OFF));
			break;
		}
		case CC_ALLSHOW:
		{
			if ( IsPriv(PRIV_ALLSHOW) )
				addRemoveAll(false, true);
			m_pAccount->TogPrivFlags(PRIV_ALLSHOW, s.GetArgStr());
			addPlayerSee(NULL);
			if ( IsSetOF(OF_Command_Sysmsgs) )
				m_pChar->SysMessagef("AllShow %s", g_Cfg.GetDefaultMsg(IsPriv(PRIV_ALLSHOW) ? DEFMSG_CMD_TOGGLE_ON : DEFMSG_CMD_TOGGLE_OFF));
			break;
		}
		case CC_DEBUG:
		{
			addRemoveAll(true, false);
			m_pAccount->TogPrivFlags(PRIV_DEBUG, s.GetArgStr());
			addPlayerSee(NULL);
			if ( IsSetOF(OF_Command_Sysmsgs) )
				m_pChar->SysMessagef("Debug %s", g_Cfg.GetDefaultMsg(IsPriv(PRIV_DEBUG) ? DEFMSG_CMD_TOGGLE_ON : DEFMSG_CMD_TOGGLE_OFF));
			break;
		}
		case CC_DETAIL:
		{
			m_pAccount->TogPrivFlags(PRIV_DETAIL, s.GetArgStr());
			if ( IsSetOF(OF_Command_Sysmsgs) )
				m_pChar->SysMessagef("Detail %s", g_Cfg.GetDefaultMsg(IsPriv(PRIV_DETAIL) ? DEFMSG_CMD_TOGGLE_ON : DEFMSG_CMD_TOGGLE_OFF));
			break;
		}
		case CC_GM:
		{
			if ( GetPrivLevel() >= PLEVEL_GM )
			{
				m_pAccount->TogPrivFlags(PRIV_GM, s.GetArgStr());
				m_pChar->UpdatePropertyFlag();
				addPlayerSee(NULL);
				if ( IsSetOF(OF_Command_Sysmsgs) )
					m_pChar->SysMessagef("GM %s", g_Cfg.GetDefaultMsg(IsPriv(PRIV_GM) ? DEFMSG_CMD_TOGGLE_ON : DEFMSG_CMD_TOGGLE_OFF));
			}
			break;
		}
		case CC_HEARALL:
		{
			m_pAccount->TogPrivFlags(PRIV_HEARALL, s.GetArgStr());
			if ( IsSetOF(OF_Command_Sysmsgs) )
				m_pChar->SysMessagef("HearAll %s", g_Cfg.GetDefaultMsg(IsPriv(PRIV_HEARALL) ? DEFMSG_CMD_TOGGLE_ON : DEFMSG_CMD_TOGGLE_OFF));
			break;
		}
		case CC_PRIVSHOW:
			if ( GetPrivLevel() >= PLEVEL_Counsel )
			{
				if ( !s.HasArgs() )
					m_pAccount->TogPrivFlags(PRIV_PRIV_NOSHOW, NULL);
				else if ( s.GetArgVal() )
					m_pAccount->ClearPrivFlags(PRIV_PRIV_NOSHOW);
				else
					m_pAccount->SetPrivFlags(PRIV_PRIV_NOSHOW);

				m_pChar->UpdatePropertyFlag();
				if ( IsSetOF(OF_Command_Sysmsgs) )
					m_pChar->SysMessagef("PrivShow %s", g_Cfg.GetDefaultMsg(IsPriv(PRIV_PRIV_NOSHOW) ? DEFMSG_CMD_TOGGLE_OFF : DEFMSG_CMD_TOGGLE_ON));
			}
			break;
		case CC_TARG:
			m_Targ_UID = s.GetArgVal();
			break;
		case CC_TARGP:
			m_Targ_p.Read(s.GetArgRaw());
			if ( !m_Targ_p.IsValidPoint() )
			{
				m_Targ_p.ValidatePoint();
				SysMessagef("Invalid point: %s", s.GetArgStr());
			}
			break;
		case CC_TARGPROP:
			m_Prop_UID = s.GetArgVal();
			break;
		case CC_TARGPRV:
			m_Targ_PrvUID = s.GetArgVal();
			break;
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CClient::r_Verb(CScript &s, CTextConsole *pSrc) // Execute command from script
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
	if ( s.IsKeyHead("SET", 3) && !g_Cfg.m_Functions.ContainsKey(pszKey) )
	{
		if ( GetPrivLevel() < g_Cfg.GetPrivCommandLevel("SET") )
			return false;

		ASSERT(m_pChar);
		addTargetVerb(pszKey + 3, s.GetArgRaw());
		return true;
	}

	if ( (toupper(pszKey[0]) == 'X') && !g_Cfg.m_Functions.ContainsKey(pszKey) )
	{
		if ( GetPrivLevel() < g_Cfg.GetPrivCommandLevel("SET") )
			return false;

		// Target this command verb on some other object.
		ASSERT(m_pChar);
		addTargetVerb(pszKey + 1, s.GetArgRaw());
		return true;
	}

	int index = FindTableSorted(pszKey, sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1);
	switch ( index )
	{
		case CV_ADD:
		{
			if ( s.HasArgs() )
			{
				TCHAR *ppArgs[2];
				size_t iArgQty = Str_ParseCmds(s.GetArgStr(), ppArgs, COUNTOF(ppArgs));

				if ( !IsValidGameObjDef(static_cast<LPCTSTR>(ppArgs[0])) )
				{
					DEBUG_ERR(("%s: invalid argument '%s'\n", sm_szVerbKeys[index], ppArgs[0]));
					SysMessageDefault(DEFMSG_CMD_INVALID);
					return true;
				}

				RESOURCE_ID rid = g_Cfg.ResourceGetID(RES_QTY, const_cast<LPCTSTR &>(ppArgs[0]));
				m_tmAdd.m_id = rid.GetResIndex();
				m_tmAdd.m_amount = (iArgQty > 1) ? static_cast<WORD>(maximum(ATOI(ppArgs[1]), 1)) : 1;

				if ( (rid.GetResType() == RES_CHARDEF) || (rid.GetResType() == RES_SPAWN) )
				{
					m_Targ_PrvUID.InitUID();
					return addTargetChars(CLIMODE_TARG_ADDCHAR, static_cast<CREID_TYPE>(m_tmAdd.m_id), false);
				}
				else
					return addTargetItems(CLIMODE_TARG_ADDITEM, static_cast<ITEMID_TYPE>(m_tmAdd.m_id));
				break;
			}

			if ( IsValidDef("d_add") )
				Dialog_Setup(CLIMODE_DIALOG, g_Cfg.ResourceGetIDType(RES_DIALOG, "d_add"), 0, m_pChar);
			else
				SysMessageDefault(DEFMSG_CMD_INVALID);
			break;
		}
		case CV_ADDBUFF:
		{
			TCHAR *ppArgs[11];
			Str_ParseCmds(s.GetArgStr(), ppArgs, COUNTOF(ppArgs));

			int iArgs[4];
			for ( int i = 0; i < 4; ++i )
			{
				if ( !IsStrNumeric(ppArgs[i]) )
				{
					DEBUG_ERR(("%s: invalid argument '%s'\n", sm_szVerbKeys[index], ppArgs[i]));
					return true;
				}
				iArgs[i] = Exp_GetVal(ppArgs[i]);
			}
			if ( (iArgs[0] < 0) || (iArgs[0] > WORD_MAX) )
			{
				DEBUG_ERR(("%s: invalid icon '%d'\n", sm_szVerbKeys[index], iArgs[0]));
				break;
			}

			LPCTSTR pszArgs[7];
			size_t iArgQty = 0;
			for ( int i = 0; i < 7; ++i )
			{
				pszArgs[i] = ppArgs[i + 4];
				if ( pszArgs[i] != NULL )
					++iArgQty;
			}

			addBuff(static_cast<BUFF_ICONS>(iArgs[0]), static_cast<DWORD>(iArgs[1]), static_cast<DWORD>(iArgs[2]), static_cast<WORD>(iArgs[3]), pszArgs, iArgQty);
			break;
		}
		case CV_REMOVEBUFF:
		{
			BUFF_ICONS IconId = static_cast<BUFF_ICONS>(s.GetArgVal());
			if ( (IconId < 0) || (IconId > WORD_MAX) )
			{
				DEBUG_ERR(("%s: invalid icon '%d'\n", sm_szVerbKeys[index], IconId));
				break;
			}
			removeBuff(IconId);
			break;
		}
		case CV_ADDCLILOC:
		{
			// Add cliloc in @ClientTooltip trigger
			TCHAR *ppArgs[16];
			size_t iArgQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), ",");
			DWORD dwClilocId = static_cast<DWORD>(Exp_GetLLVal(ppArgs[0]));

			CGString sVal;
			for ( size_t i = 1; i < iArgQty; ++i )
			{
				if ( sVal.GetLength() )
					sVal += "\t";
				sVal += !strcmp(ppArgs[i], "NULL") ? " " : ppArgs[i];
			}

			m_TooltipData.Add(new CClientTooltip(dwClilocId, sVal));
			break;
		}
		case CV_ADDCONTEXTENTRY:
		{
			// Add context menu entry in @[Item/Char]ContextMenuRequest trigger
			if ( !m_pPopupPacket )
				return true;

			TCHAR *ppArgs[4];
			size_t iArgQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), ",");

			for ( size_t i = 0; i < iArgQty; ++i )
			{
				if ( (i > 1) && IsStrEmpty(ppArgs[i]) )
					continue;

				if ( !IsStrNumeric(ppArgs[i]) )
				{
					DEBUG_ERR(("%s: invalid argument '%s'\n", sm_szVerbKeys[index], ppArgs[i]));
					return true;
				}
			}

			int iTextEntry = Exp_GetVal(ppArgs[0]);
			if ( iTextEntry < 100 )
			{
				DEBUG_ERR(("%s: can't use entry '%d' (entries < 100 are reserved for internal server usage)\n", sm_szVerbKeys[index], iTextEntry));
				return true;
			}
			m_pPopupPacket->addOption(static_cast<WORD>(iTextEntry), static_cast<DWORD>(Exp_GetLLVal(ppArgs[1])), static_cast<WORD>(Exp_GetLLVal(ppArgs[2])), static_cast<WORD>(Exp_GetLLVal(ppArgs[3])));
			break;
		}
		case CV_ARROWQUEST:
		{
			INT64 piVal[3];
			Str_ParseCmds(s.GetArgRaw(), piVal, COUNTOF(piVal));
			addArrowQuest(static_cast<WORD>(piVal[0]), static_cast<WORD>(piVal[1]), static_cast<DWORD>(piVal[2]));
			break;
		}
		case CV_BADSPAWN:
		{
			// Loop the world searching for bad spawns
			bool fFound = false;
			CItem *pItem = NULL;
			CSector *pSector = NULL;
			CResourceDef *pSpawnDef = NULL;
			for ( size_t iMap = 0; (iMap < MAP_QTY) && !fFound; ++iMap )
			{
				if ( !g_MapList.m_maps[iMap] )
					continue;

				int iSectorQty = g_MapList.GetSectorQty(iMap);
				for ( int iSector = 0; (iSector < iSectorQty) && !fFound; ++iSector )
				{
					pSector = g_World.GetSector(iMap, iSector);
					if ( !pSector )
						continue;

					for ( pItem = static_cast<CItem *>(pSector->m_Items_Timer.GetHead()); (pItem != NULL) && !fFound; pItem = pItem->GetNext() )
					{
						if ( pItem->IsType(IT_SPAWN_ITEM) || pItem->IsType(IT_SPAWN_CHAR) )
						{
							pSpawnDef = static_cast<CItemSpawn *>(pItem)->FixDef();
							if ( !pSpawnDef )
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
			break;
		}
		case CV_BANKSELF:
		{
			addBankOpen(m_pChar, LAYER_BANKBOX);
			break;
		}
		case CV_CAST:
		{
			SPELL_TYPE spell = static_cast<SPELL_TYPE>(g_Cfg.ResourceGetIndexType(RES_SPELL, s.GetArgStr()));
			const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
			if ( !pSpellDef )
				return true;

			CObjBase *pObjSrc = dynamic_cast<CObjBase *>(pSrc);
			if ( IsSetMagicFlags(MAGICF_PRECAST) && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST) )
			{
				int iSkill;
				if ( !pSpellDef->GetPrimarySkill(&iSkill, NULL) )
					return true;

				m_tmSkillMagery.m_Spell = spell;	// m_atMagery.m_Spell
				m_pChar->m_atMagery.m_Spell = spell;
				if ( pObjSrc )
				{
					m_Targ_UID = pObjSrc->GetUID();	// default target.
					m_Targ_PrvUID = pObjSrc->GetUID();
				}
				else
				{
					m_Targ_UID.ClearUID();
					m_Targ_PrvUID.ClearUID();
				}
				m_pChar->Skill_Start(static_cast<SKILL_TYPE>(iSkill));
				break;
			}
			else
				Cmd_Skill_Magery(spell, pObjSrc);
			break;
		}
		case CV_CHANGEFACE:		// open 'face selection' dialog (enhanced clients only)
		{
			addGumpDialog(CLIMODE_DIALOG, NULL, 0, NULL, 0, 0, 0, m_pChar, CLIMODE_DIALOG_FACESELECTION);
			break;
		}
		case CV_CHARLIST:		// usually just a gm command
		{
			if ( !PacketChangeCharacter::CanSendTo(m_NetState) )
				break;
			new PacketChangeCharacter(this);
			CharDisconnect();	// since there is no undoing this in the client.
			SetTargMode(CLIMODE_SETUP_CHARLIST);
			break;
		}
		case CV_CTAGLIST:
		{
			if ( !strcmpi(s.GetArgStr(), "log") )
				pSrc = &g_Serv;
			m_TagDefs.DumpKeys(pSrc, "CTAG.");
			break;
		}
		case CV_CLEARCTAGS:
		{
			if ( s.HasArgs() )
			{
				LPCTSTR pszArgs = s.GetArgStr();
				SKIP_SEPARATORS(pszArgs);
				m_TagDefs.ClearKeys(pszArgs);
			}
			else
				m_TagDefs.ClearKeys();
			break;
		}
		case CV_CLEARSPECIALMOVE:
		{
			Event_SpecialMoveSelect(0);
			addSpecialMoveClear();
			break;
		}
		case CV_CLOSEPAPERDOLL:
		{
			const CChar *pChar = s.HasArgs() ? static_cast<CGrayUID>(s.GetArgVal()).CharFind() : m_pChar;
			if ( pChar )
				closeUIWindow(PacketCloseUIWindow::Paperdoll, pChar);
			break;
		}
		case CV_CLOSEPROFILE:
		{
			const CChar *pChar = s.HasArgs() ? static_cast<CGrayUID>(s.GetArgVal()).CharFind() : m_pChar;
			if ( pChar )
				closeUIWindow(PacketCloseUIWindow::Profile, pChar);
			break;
		}
		case CV_CLOSESTATUS:
		{
			const CChar *pChar = s.HasArgs() ? static_cast<CGrayUID>(s.GetArgVal()).CharFind() : m_pChar;
			if ( pChar )
				closeUIWindow(PacketCloseUIWindow::Status, pChar);
			break;
		}
		case CV_CODEXOFWISDOM:
		{
			INT64 piArgs[2];
			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), piArgs, COUNTOF(piArgs));
			if ( iArgQty < 1 )
			{
				SysMessage("Usage: CODEXOFWISDOM TopicID [ForceOpen]");
				break;
			}

			addCodexOfWisdom(static_cast<DWORD>(piArgs[0]), static_cast<bool>(piArgs[1]));
			break;
		}
		case CV_DYE:
		{
			const CObjBase *pObj = s.HasArgs() ? static_cast<CGrayUID>(s.GetArgVal()).ObjFind() : NULL;
			if ( pObj )
				addDyeOption(pObj);
			break;
		}
		case CV_EVERBTARG:
		{
			m_Prompt_Text = s.GetArgStr();
			addPromptConsole(CLIMODE_PROMPT_TARG_VERB, m_Targ_Text.IsEmpty() ? "Enter the verb" : "Enter the text", m_Targ_UID);
			break;
		}
		case CV_EXTRACT:
		{
			// sort of like EXPORT but for statics.
			// Opposite of the "UNEXTRACT" command
			TCHAR *ppArgs[2];
			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), ppArgs, COUNTOF(ppArgs));
			if ( iArgQty < 2 )
			{
				SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_EXTRACT_USAGE));
				break;
			}

			m_Targ_Text = ppArgs[0];				// point at the options (if any)
			m_tmTile.m_ptFirst.InitPoint();			// clear this first
			m_tmTile.m_Code = CV_EXTRACT;			// set extract code
			m_tmTile.m_id = Exp_GetVal(ppArgs[1]);	// extract id
			addTarget(CLIMODE_TARG_TILE, g_Cfg.GetDefaultMsg(DEFMSG_SELECT_EXTRACT_AREA), true);
			break;
		}
		case CV_UNEXTRACT:
		{
			// Create item from script.
			// Opposite of the "EXTRACT" command
			TCHAR *ppArgs[2];
			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), ppArgs, COUNTOF(ppArgs));
			if ( iArgQty < 2 )
			{
				SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_UNEXTRACT_USAGE));
				break;
			}

			m_Targ_Text = ppArgs[0];				// point at the options (if any)
			m_tmTile.m_ptFirst.InitPoint();			// clear this first
			m_tmTile.m_Code = CV_UNEXTRACT;			// set extract code
			m_tmTile.m_id = Exp_GetVal(ppArgs[1]);	// extract id
			addTarget(CLIMODE_TARG_UNEXTRACT, g_Cfg.GetDefaultMsg(DEFMSG_SELECT_MULTI_POS), true);
			break;
		}
		case CV_GMPAGE:
		{
			m_Targ_Text = s.GetArgStr();
			if ( !m_Targ_Text.IsEmpty() && !strnicmp(m_Targ_Text, "ADD ", 4) )
			{
				Event_PromptResp_GMPage(m_Targ_Text + 4);
				break;
			}
			addPromptConsole(CLIMODE_PROMPT_GM_PAGE_TEXT, g_Cfg.GetDefaultMsg(DEFMSG_GMPAGE_PROMPT));
			break;
		}
		case CV_GOTARG:		// go to my (preselected) target.
		{
			ASSERT(m_pChar);
			CObjBase *pObj = m_Targ_UID.ObjFind();
			if ( pObj )
			{
				CPointMap pt = pObj->GetTopLevelObj()->GetTopPoint();
				m_pChar->m_dirFace = m_pChar->GetDir(pObj, m_pChar->m_dirFace);
				m_pChar->Spell_Teleport(pt, true, false);
			}
			break;
		}
		case CV_INFO:
		{
			// We could also get ground tile info.
			addTarget(CLIMODE_TARG_OBJ_INFO, g_Cfg.GetDefaultMsg(DEFMSG_SELECT_ITEM_INFO), true, false);
			break;
		}
		case CV_INFORMATION:
		{
			SysMessage(g_Serv.GetStatusString(0x22));
			SysMessage(g_Serv.GetStatusString(0x24));
			break;
		}
		case CV_LAST:	// fake previous target
		{
			if ( GetTargMode() >= CLIMODE_MOUSE_TYPE )
			{
				ASSERT(m_pChar);
				CObjBase *pObj = m_pChar->m_Act_Targ.ObjFind();
				if ( pObj )
				{
					Event_Target(GetTargMode(), pObj->GetUID(), pObj->GetTopPoint());
					addTargetCancel();
				}
				break;
			}
			return false;
		}
		case CV_LINK:	// link doors
		{
			m_Targ_UID.InitUID();
			addTarget(CLIMODE_TARG_LINK, g_Cfg.GetDefaultMsg(DEFMSG_SELECT_LINK_ITEM));
			break;
		}
		case CV_MAPWAYPOINT:
		{
			INT64 piVal[2];
			size_t iArgQty = Str_ParseCmds(s.GetArgRaw(), piVal, COUNTOF(piVal));
			if ( iArgQty < 2 )
			{
				SysMessage("Usage: MAPWAYPOINT uid type");
				break;
			}
			CObjBase *pObj = static_cast<CGrayUID>(piVal[0]).ObjFind();
			addMapWaypoint(pObj, static_cast<MAPWAYPOINT_TYPE>(piVal[1]));
			break;
		}
		case CV_MENU:
		{
			Menu_Setup(g_Cfg.ResourceGetIDType(RES_MENU, s.GetArgStr()));
			break;
		}
		case CV_MIDILIST:
		{
			INT64 piMidi[64];
			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), piMidi, COUNTOF(piMidi));
			if ( iArgQty > 0 )
				addMusic(static_cast<MIDI_TYPE>(piMidi[Calc_GetRandVal(iArgQty)]));
			break;
		}
		case CV_NUDGE:
		{
			if ( !s.HasArgs() )
			{
				SysMessage("Usage: NUDGE dx dy dz");
				break;
			}
			m_Targ_Text = s.GetArgRaw();
			m_tmTile.m_ptFirst.InitPoint();		// clear this first
			m_tmTile.m_Code = CV_NUDGE;
			addTarget(CLIMODE_TARG_TILE, g_Cfg.GetDefaultMsg(DEFMSG_SELECT_NUDGE_AREA), true);
			break;
		}
		case CV_NUKE:
		{
			m_Targ_Text = s.GetArgRaw();
			m_tmTile.m_ptFirst.InitPoint();		// clear this first
			m_tmTile.m_Code = CV_NUKE;			// set nuke code
			addTarget(CLIMODE_TARG_TILE, g_Cfg.GetDefaultMsg(DEFMSG_SELECT_NUKE_AREA), true);
			break;
		}
		case CV_NUKECHAR:
		{
			m_Targ_Text = s.GetArgRaw();
			m_tmTile.m_ptFirst.InitPoint();		// clear this first
			m_tmTile.m_Code = CV_NUKECHAR;		// set nuke code
			addTarget(CLIMODE_TARG_TILE, g_Cfg.GetDefaultMsg(DEFMSG_SELECT_NUKE_CHAR_AREA), true);
			break;
		}
		case CV_OPENPAPERDOLL:
		{
			const CChar *pChar = s.HasArgs() ? static_cast<CGrayUID>(s.GetArgVal()).CharFind() : m_pChar;
			if ( pChar )
				addCharPaperdoll(pChar);
			break;
		}
		case CV_OPENTRADEWINDOW:
		{
			TCHAR *ppArgs[2];
			Str_ParseCmds(s.GetArgStr(), ppArgs, COUNTOF(ppArgs));

			CChar *pChar = ppArgs[0] ? static_cast<CGrayUID>(Exp_GetLLVal(ppArgs[0])).CharFind() : NULL;
			if ( pChar )
			{
				CItem *pItem = ppArgs[1] ? static_cast<CGrayUID>(Exp_GetLLVal(ppArgs[1])).ItemFind() : NULL;
				Cmd_SecureTrade(pChar, pItem);
			}
			break;
		}
		case CV_REPAIR:
			addTarget(CLIMODE_TARG_REPAIR, g_Cfg.GetDefaultMsg(DEFMSG_SELECT_ITEM_REPAIR));
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
			addScrollResource(s.GetArgStr(), SCROLL_Updates);
			break;
		case CV_SENDPACKET:
			SendPacket(s.GetArgStr());
			break;
		case CV_SELF:		// fake self target
			if ( GetTargMode() >= CLIMODE_MOUSE_TYPE )
			{
				ASSERT(m_pChar);
				Event_Target(GetTargMode(), m_pChar->GetUID(), m_pChar->GetTopPoint());
				addTargetCancel();
				break;
			}
			return false;
		case CV_SHOWSKILLS:
			addSkillWindow(static_cast<SKILL_TYPE>(g_Cfg.m_iMaxSkill));		// reload the real skills
			break;
		case CV_SKILLMENU:
			Cmd_Skill_Menu(g_Cfg.ResourceGetIDType(RES_SKILLMENU, s.GetArgStr()));
			break;
		case CV_SKILLSELECT:
			Event_Skill_Use(g_Cfg.FindSkillKey(s.GetArgStr()));
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

				LPCTSTR pszPrompt = !pSpellDef->m_sTargetPrompt.IsEmpty() ? static_cast<LPCTSTR>(pSpellDef->m_sTargetPrompt) : g_Cfg.GetDefaultMsg(DEFMSG_SELECT_MAGIC_TARGET);

				int iSpellTimeout = static_cast<int>(GetDefNum("SPELLTIMEOUT"));
				if ( !iSpellTimeout )
					iSpellTimeout = g_Cfg.m_iSpellTimeout * TICK_PER_SEC;

				addTarget(CLIMODE_TARG_SKILL_MAGERY, pszPrompt, pSpellDef->IsSpellType(SPELLFLAG_TARG_XYZ), pSpellDef->IsSpellType(SPELLFLAG_HARM), iSpellTimeout);
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
					int iSkill;
					if ( !pSpellDef->GetPrimarySkill(&iSkill, NULL) )
						return false;

					m_pChar->Skill_Start(static_cast<SKILL_TYPE>(iSkill));
				}
			}
			break;
		}
		case CV_SMSG:
		case CV_SYSMESSAGE:
			SysMessage(s.GetArgStr());
			break;
		case CV_SYSMESSAGEF:	// there is still an issue with numbers not resolving properly when %i,%d,or other numeric format code is in use
		{
			TCHAR *ppArgs[4];
			size_t iArgQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs));
			if ( iArgQty < 2 )
			{
				g_Log.EventError("%s: too few arguments\n", sm_szVerbKeys[index]);
				return false;
			}
			if ( iArgQty > 4 )
			{
				g_Log.EventError("%s: too many arguments\n", sm_szVerbKeys[index]);
				return false;
			}
			if ( *ppArgs[0] == '"' )	// skip quotes
				++ppArgs[0];
			for ( TCHAR *pEnd = ppArgs[0] + strlen(ppArgs[0]) - 1; pEnd >= ppArgs[0]; --pEnd )
			{
				if ( *pEnd == '"' )		// skip quotes
				{
					*pEnd = '\0';
					break;
				}
			}
			SysMessagef(ppArgs[0], ppArgs[1], ppArgs[2] ? ppArgs[2] : 0, ppArgs[3] ? ppArgs[3] : 0);
			break;
		}
		case CV_SMSGU:
		case CV_SYSMESSAGEUA:
		{
			TCHAR *ppArgs[5];
			size_t iArgQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs));
			if ( iArgQty > 4 )
			{
				// Font and mode are actually ignored here, but they never made a difference anyway.. I'd like to keep the syntax similar to SAYUA
				NCHAR szBuffer[MAX_TALK_BUFFER];
				CvtSystemToNUNICODE(szBuffer, COUNTOF(szBuffer), ppArgs[4], -1);

				addBarkUNICODE(szBuffer, NULL, static_cast<HUE_TYPE>(Exp_GetLLVal(ppArgs[0])), TALKMODE_SYSTEM, FONT_NORMAL, ppArgs[3]);
			}
			break;
		}
		case CV_SMSGL:
		case CV_SYSMESSAGELOC:
		{
			TCHAR *ppArgs[16];
			size_t iArgQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), ",");
			if ( iArgQty > 1 )
			{
				HUE_TYPE hue = (ATOI(ppArgs[0]) > 0) ? static_cast<HUE_TYPE>(Exp_GetLLVal(ppArgs[0])) : HUE_TEXT_DEF;
				DWORD dwClilocId = static_cast<DWORD>(Exp_GetLLVal(ppArgs[1]));

				CGString sVal;
				for ( size_t i = 2; i < iArgQty; ++i )
				{
					if ( sVal.GetLength() )
						sVal += "\t";
					sVal += !strcmp(ppArgs[i], "NULL") ? " " : ppArgs[i];
				}

				addBarkLocalized(dwClilocId, NULL, hue, TALKMODE_SYSTEM, FONT_NORMAL, sVal.GetPtr());
			}
			break;
		}
		case CV_SMSGLEX:
		case CV_SYSMESSAGELOCEX:
		{
			TCHAR *ppArgs[16];
			size_t iArgQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), ",");
			if ( iArgQty > 2 )
			{
				HUE_TYPE hue = (ATOI(ppArgs[0]) > 0) ? static_cast<HUE_TYPE>(Exp_GetLLVal(ppArgs[0])) : HUE_TEXT_DEF;
				DWORD dwClilocId = static_cast<DWORD>(Exp_GetLLVal(ppArgs[1]));
				AFFIX_TYPE affix = ppArgs[2] ? static_cast<AFFIX_TYPE>(Exp_GetLLVal(ppArgs[2])) : AFFIX_APPEND;

				CGString sVal;
				for ( size_t i = 4; i < iArgQty; ++i )
				{
					if ( sVal.GetLength() )
						sVal += "\t";
					sVal += !strcmp(ppArgs[i], "NULL") ? " " : ppArgs[i];
				}

				addBarkLocalizedEx(dwClilocId, NULL, hue, TALKMODE_SYSTEM, FONT_NORMAL, affix, ppArgs[3], sVal.GetPtr());
			}
			break;
		}
		case CV_TELE:
			Cmd_Skill_Magery(SPELL_Teleport, dynamic_cast<CObjBase *>(pSrc));
			break;
		case CV_TILE:
			if ( !s.HasArgs() )
			{
				SysMessage("Usage: TILE z-height item1 item2 itemX");
				break;
			}
			m_Targ_Text = s.GetArgStr();	// point at the options
			m_tmTile.m_ptFirst.InitPoint();	// clear this first
			m_tmTile.m_Code = CV_TILE;
			addTarget(CLIMODE_TARG_TILE, "Pick 1st corner:", true);
			break;
		case CV_VERSION:
			SysMessage(g_szServerDescription);
			break;
		case CV_WEBLINK:
			addWebLaunch(s.GetArgStr());
			break;
		default:
			if ( r_LoadVal(s) )
			{
				CGString sVal;
				if ( r_WriteVal(s.GetKey(), sVal, pSrc) )
					return true;
			}
			return CScriptObj::r_Verb(s, pSrc);		// used in the case of web pages to access server level things
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

CSocketAddress &CClient::GetPeer() const
{
	return m_NetState->m_peerAddress;
}

LPCTSTR CClient::GetPeerStr() const
{
	return m_NetState->m_peerAddress.GetAddrStr();
}
