//
// CClient.cpp
// Copyright Menace Software (www.menasoft.com).
//
#include "graysvr.h"	// predef header.
#include "CClient.h"

#ifndef _WIN32
	extern LinuxEv g_NetworkEvent;
#endif

/////////////////////////////////////////////////////////////////
// -CClient stuff.

CClient::CClient( SOCKET client ) :
	m_Socket( client )
{
	// This may be a web connection or Telnet ?
	m_PeerName		= m_Socket.GetPeerName(); // store ip address
	SetConnectType( CONNECT_UNK );	// don't know what sort of connect this is yet.
	UpdateLogIPConnecting( true );
	UpdateLogIPConnected( true );

	m_Crypt.SetClientVer( g_Serv.m_ClientVersion );
	m_pAccount = NULL;

	m_pChar = NULL;
	m_pGMPage = NULL;

	m_timeLogin.Init();
	m_timeLastSend =
	m_timeLastEvent = CServTime::GetCurrentTime();
	m_timeLastEventWalk = CServTime::GetCurrentTime();

	m_bin_PrvMsg = XCMD_QTY;

	m_wWalkCount = -1;
	m_iWalkStepCount = 0;
	m_iWalkTimeAvg	= 100;
	m_timeWalkStep = GetTickCount();
	m_fClosed = false;

	m_Targ_Timeout.Init();
	m_Targ_Mode = CLIMODE_SETUP_CONNECTING;
	m_Prompt_Mode = CLIMODE_NORMAL;

	m_tmSetup.m_dwIP = 0;
	m_tmSetup.m_iConnect = 0;
	m_tmSetup.m_bNewSeed = false;

	memset( m_Walk_LIFO, 0, sizeof( m_Walk_LIFO ));	// Initialize the fast walk killer stuff
	m_Walk_InvalidEchos = 0;
	m_Walk_CodeQty = -1;	// how many valid codes in here ?

	m_Env.SetInvalid();

	g_Serv.StatInc(SERV_STAT_CLIENTS);
	g_Serv.m_Clients.InsertHead(this);

	g_Log.Event(LOGM_CLIENTS_LOG, "%x:Client connected [Total:%i] ('%s' %i/%i)\n",
		m_Socket.GetSocket(), g_Serv.StatGet(SERV_STAT_CLIENTS), m_PeerName.GetAddrStr(),
		GetLogIPConnecting(), GetLogIPConnected());

	m_Socket.SetNonBlocking();

	// disable NAGLE algorythm for data compression/coalescing.
	// Send as fast as we can. we handle packing ourselves.
	BOOL nbool=TRUE;
	m_Socket.SetSockOpt(TCP_NODELAY, &nbool, sizeof(BOOL), IPPROTO_TCP);
	m_sendingData = false;
	
	if ( IsSetEF( EF_UseNetworkMulti ) )
	{
#ifndef _WIN32
		g_NetworkEvent.registerClient(this, LinuxEv::Write);
#endif
	}	

	m_zLastMessage[0] = 0;
	m_zLastObjMessage[0] = 0;
	m_tNextPickup.Init();
	m_reportedCliver = 0;
	m_bClient3d = false; // Client by default are 2d
	m_bClientKR = false;
	m_BfAntiCheat.lastvalue = m_BfAntiCheat.count = 0x0;
	m_ScreenSize.x = m_ScreenSize.y = 0x0;
	m_LastTooltipSend = 0;
	m_context_popup = -1;
	m_packetExceptions = 0;
	m_pHouseDesign = NULL;
	m_fUpdateStats = 0;
}


CClient::~CClient()
{
	g_Serv.StatDec( SERV_STAT_CLIENTS );
	bool bWasChar;

	if ( GetConnectType() != CONNECT_GAME )
		UpdateLogIPConnecting( false );
	UpdateLogIPConnected( false );

	g_Log.Event(LOGM_CLIENTS_LOG, "%x:Client disconnected [Total:%i] ('%s' %i/%i)\n",
		m_Socket.GetSocket(), g_Serv.StatGet(SERV_STAT_CLIENTS), m_PeerName.GetAddrStr(),
		GetLogIPConnecting(), GetLogIPConnected());

	bWasChar = ( m_pChar != NULL );
	CharDisconnect();	// am i a char in game ?
	Cmd_GM_PageClear();

	// Clear containers (CTAG and TOOLTIP)
	m_TagDefs.Empty();
	m_TooltipData.Clean(true);
	CleanTooltipQueue();

	CAccount * pAccount = GetAccount();
	if ( pAccount )
	{
		pAccount->OnLogout(this, bWasChar);
		m_pAccount = NULL;
	}

	xFlush();

	if ( m_Socket.IsOpen() )
	{
		if ( IsSetEF( EF_UseNetworkMulti ) )
		{
#ifdef _WIN32
			m_Socket.ClearAsync();
#else
			g_NetworkEvent.unregisterClient(this);
#endif
		}

		m_Socket.Close();
	}
}

bool CClient::IsSkillVisible(SKILL_TYPE skill)
{
	ADDTOCALLSTACK("CClient::IsSkillVisible");
	// Determine whether or not this client can see a skill in their
	// skill menu.

	// ML Clients can always see ML skills regardless of enabled features
	if ( skill >= SKILL_SPELLWEAVING && IsClientVersion( 0x500000 ) )
		return true;

	int iClientReq = 0x000000;	// The minimum client version required to see this skill
	bool bEnabled = true;		// Whether or not this skill is enabled in FEATURE_ options

	if ( skill < MAX_SKILL_T2A )
	{
		iClientReq = 0x000000;
	}
	else if ( skill < MAX_SKILL_LBR )
	{
		iClientReq = 0x300000;
	}
	else if ( skill < MAX_SKILL_AOS )
	{
		iClientReq = 0x300080;
		bEnabled = ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B );
	}
	else if ( skill < MAX_SKILL_SE )
	{
		iClientReq = 0x400010;
		bEnabled = ( g_Cfg.m_iFeatureSE & FEATURE_SE_NINJASAM );
	}
	else
	{
		iClientReq = 0x500000;
		bEnabled = ( g_Cfg.m_iFeatureML & FEATURE_ML_UPDATE );
	}

	// Check that the client has a valid client version
	if ( !IsClientVersion( iClientReq ) )
		return false;

	// Check that the skill is enabled
	if ( !bEnabled )
		return false;

	int iMaxSkill = MAX_SKILL;	// The highest skill id viewable by the client (+1)

	switch ( GetResDisp() )
	{
		case RDS_SA:
			iMaxSkill = MAX_SKILL_SA;
			break;

		case RDS_KR:
			iMaxSkill = MAX_SKILL_KR;
			break;

		case RDS_ML:
			iMaxSkill = MAX_SKILL_ML;
			break;

		case RDS_SE:
			iMaxSkill = MAX_SKILL_SE;
			break;

		case RDS_AOS:
			iMaxSkill = MAX_SKILL_AOS;
			break;

		case RDS_LBR:
			iMaxSkill = MAX_SKILL_LBR;
			break;

		case RDS_T2A:
			iMaxSkill = MAX_SKILL_T2A;
			break;
	}

	if ( skill >= iMaxSkill )
		return false;

	return true;
}

void CClient::CleanTooltipQueue()
{
	ADDTOCALLSTACK("CClient::CleanTooltipQueue");
	std::vector<CTooltipData *>::iterator i = m_TooltipQueue.begin();
	CTooltipData * pData = NULL;

	while ( i != m_TooltipQueue.end() )
	{
		pData = (*i);
		m_TooltipQueue.erase(i);
		i = m_TooltipQueue.begin();

		if ( pData )
		{
			delete pData;
		}
	}

	m_TooltipQueue.clear();
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

	CScriptTriggerArgs Args(iLingerTime, fCanInstaLogOut);
	m_pChar->OnTrigger(CTRIG_LogOut, m_pChar, &Args);
	iLingerTime = Args.m_iN1;
	fCanInstaLogOut = Args.m_iN2;

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
			{
				if ( ISINTRESOURCE(pszMsg) ) return;
				for ( ; *pszMsg != '\0'; pszMsg++ )
				{
					if ( *pszMsg == '\n' )	// translate.
						(const_cast <CClient*>(this))->xSendReady("\r", 1);
					(const_cast <CClient*>(this))->xSendReady(pszMsg, 1);
				}
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
	}
}

void CClient::Announce( bool fArrive ) const
{
	ADDTOCALLSTACK("CClient::Announce");
	if ( !GetAccount() || !GetChar() || !GetChar()->m_pPlayer )
		return;

	// We have logged in or disconnected.
	// Annouce my arrival or departure.
	if (( g_Cfg.m_iArriveDepartMsg == 2 ) && ( GetPrivLevel() > PLEVEL_Player ))		// notify of GMs
	{
		char *zMsg = Str_GetTemp();
		const char *zTitle;

		switch ( GetPrivLevel() )
		{
			case PLEVEL_Owner:
			case PLEVEL_Admin:
				zTitle = "Admin";
				break;
			case PLEVEL_Seer:
				zTitle = "Seer";
				break;
			case PLEVEL_Counsel:
				zTitle = "Counselor";
				break;
			default:
				zTitle = "GM";
		}

		sprintf(zMsg, "@231 STAFF: %s %s logged %s.", zTitle, m_pChar->GetName(), ( fArrive ? "in" : "out" ));

		for ( CClient *pClient = g_Serv.GetClientHead(); pClient ; pClient = pClient->GetNext() )
		{
			if (( pClient == this ) || ( GetPrivLevel() > pClient->GetPrivLevel() ))
				continue;
			pClient->SysMessage(zMsg);
		}
	}
	else if ( g_Cfg.m_iArriveDepartMsg == 1 )		// notify of players
	{
		TCHAR *pszMsg = Str_GetTemp();
		for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
		{
			if ( pClient == this )
				continue;
			CChar * pChar = pClient->GetChar();
			if ( !pChar || ( GetPrivLevel() > pClient->GetPrivLevel() ))
				continue;
			if ( !pClient->IsPriv(PRIV_DETAIL|PRIV_HEARALL) )
				continue;

			if ( !*pszMsg )
			{
				const CRegionBase * pRegion = m_pChar->GetTopPoint().GetRegion( REGION_TYPE_AREA );
				sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_ARRDEP_1 ),
					(LPCTSTR) m_pChar->GetName(),
					(fArrive)? g_Cfg.GetDefaultMsg( DEFMSG_ARRDEP_2 ) : g_Cfg.GetDefaultMsg( DEFMSG_ARRDEP_3 ),
					pRegion ? (LPCTSTR) pRegion->GetName() : (LPCTSTR) g_Serv.GetName());
			}
			pClient->SysMessage(pszMsg);
		}
	}

	// re-Start murder decay if arriving

	if ( m_pChar->m_pPlayer->m_wMurders )
	{
		CItem * pMurders = m_pChar->LayerFind(LAYER_FLAG_Murders);
		if ( pMurders )
		{
			if ( fArrive )
			{
				// If the Memory exists, put it in the loop
				pMurders->SetTimeout( pMurders->m_itEqMurderCount.m_Decay_Balance * TICK_PER_SEC );
			}
			else
			{
				// Or save decay point if departing and remove from the loop
				pMurders->m_itEqMurderCount.m_Decay_Balance = pMurders->GetTimerAdjusted();
				pMurders->SetTimeout(-1); // turn off the timer til we log in again.
			}
		}
		else if ( fArrive )
		{
			// If not, see if we need one made
			m_pChar->Noto_Murder();
		}
	}

	if ( m_pChar )
		m_pAccount->m_uidLastChar = m_pChar->GetUID();
}

////////////////////////////////////////////////////

bool CClient::CanSee( const CObjBaseTemplate * pObj ) const
{
	ADDTOCALLSTACK("CClient::CanSee");
	// Can player see item b
	if ( m_pChar == NULL )
		return( false );
	if ( m_pHouseDesign && pObj->IsItem() )
	{
		const CItem * pItemConst = STATIC_CAST<const CItem *>( pObj );
		if ( pItemConst )
		{
			CItem * pItem = const_cast<CItem *>( pItemConst );
			if ( pItem && (pItem->GetTagDefs()->GetKeyNum("FIXTURE") == (DWORD)m_pHouseDesign->GetUID()) )
				return( false );
		}
	}
	return( m_pChar->CanSee( pObj ));
}

bool CClient::CanHear( const CObjBaseTemplate * pSrc, TALKMODE_TYPE mode ) const
{
	ADDTOCALLSTACK("CClient::CanHear");
	// can we hear this text or sound.

	if ( ! IsConnectTypePacket())
	{
		if ( GetConnectType() != CONNECT_TELNET )
			return( false );
		if ( mode == TALKMODE_BROADCAST ) // && GetAccount()
			return( true );
		return( false );
	}

	if ( mode == TALKMODE_BROADCAST || pSrc == NULL )
		return( true );
	if ( m_pChar == NULL )
		return( false );

	if ( IsPriv( PRIV_HEARALL ) &&
		pSrc->IsChar()&&
		( mode == TALKMODE_SYSTEM || mode == TALKMODE_SAY || mode == TALKMODE_WHISPER || mode == TALKMODE_YELL ))
	{
		const CChar * pCharSrc = dynamic_cast <const CChar*> ( pSrc );
		ASSERT(pCharSrc);
		if ( pCharSrc && pCharSrc->IsClient())
		{
			if ( pCharSrc->GetPrivLevel() <= GetPrivLevel())
			{
				return( true );
			}
		}
	}

	return( m_pChar->CanHear( pSrc, mode ));
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
	TCHAR *pszMsg = Str_GetTemp();
	sprintf(pszMsg, "Select object to set/command '%s'", (LPCTSTR) m_Targ_Text);
	addTarget(CLIMODE_TARG_OBJ_SET, pszMsg);
}

void CClient::addTargetFunctionMulti( LPCTSTR pszFunction, ITEMID_TYPE itemid, bool fGround )
{
	ADDTOCALLSTACK("CClient::addTargetFunctionMulti");
	// Target a verb at some object .
	ASSERT(pszFunction);
	GETNONWHITESPACE(pszFunction);
	SKIP_SEPARATORS(pszFunction);

	m_Targ_Text.Format( pszFunction );
	if ( CItemBase::IsID_Multi( itemid ))	// a multi we get from Multi.mul
	{
		SetTargMode(CLIMODE_TARG_OBJ_FUNC, "");

		CCommand cmd;
		cmd.TargetMulti.m_Cmd = XCMD_TargetMulti;
		cmd.TargetMulti.m_fAllowGround = fGround;
		cmd.TargetMulti.m_context = CLIMODE_TARG_OBJ_FUNC;	// 5=my id code for action.
		memset( cmd.TargetMulti.m_zero6, 0, sizeof(cmd.TargetMulti.m_zero6));
		cmd.TargetMulti.m_id = itemid - ITEMID_MULTI;

		// Add any extra stuff attached to the multi. preview this.

		memset( cmd.TargetMulti.m_zero20, 0, sizeof(cmd.TargetMulti.m_zero20));

		xSendPkt( &cmd, sizeof( cmd.TargetMulti ));
	}
	addTargetFunction( pszFunction, fGround, false );
}

void CClient::addTargetFunction( LPCTSTR pszFunction, bool fAllowGround, bool fCheckCrime )
{
	ADDTOCALLSTACK("CClient::addTargetFunction");
	// Target a verb at some object .
	ASSERT(pszFunction);
	GETNONWHITESPACE(pszFunction);
	SKIP_SEPARATORS(pszFunction);

	m_Targ_Text.Format( pszFunction );
	addTarget( CLIMODE_TARG_OBJ_FUNC, "", fAllowGround, fCheckCrime );
}

void CClient::addPromptConsoleFunction( LPCTSTR pszFunction, LPCTSTR pszSysmessage, bool bUnicode )
{
	ADDTOCALLSTACK("CClient::addPromptConsoleFunction");
	// Target a verb at some object .
	ASSERT(pszFunction);
	m_Prompt_Text.Format( pszFunction );
	addPromptConsole( CLIMODE_PROMPT_SCRIPT_VERB, pszSysmessage, 0, 0, bUnicode );
}


enum CLIR_TYPE
{
	CLIR_ACCOUNT,
	CLIR_GMPAGEP,
	CLIR_HOUSEDESIGN,
	CLIR_PARTY,
	CLIR_TARG,
	CLIR_TARGPROP,
	CLIR_TARGPRV,
	CLIR_QTY,
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
	NULL,
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
				pRef = GetAccount();
				return( true );
			case CLIR_GMPAGEP:
				pRef = m_pGMPage;
				return( true );
			case CLIR_HOUSEDESIGN:
				pRef = m_pHouseDesign;
				return( true );
			case CLIR_PARTY:
				if ( !this->m_pChar->m_pParty )
					return false;
				pRef = this->m_pChar->m_pParty;
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
			sVal.FormatVal( m_bClient3d );
			break;
		case CC_CLIENTISKR:
			sVal.FormatVal( m_bClientKR );
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
			sVal.FormatVal( m_timeLastEvent.GetTimeRaw() );
			break;
		case CC_PRIVSHOW:
			// Show my priv title.
			sVal.FormatVal( ! IsPriv( PRIV_PRIV_NOSHOW ));
			break;
		case CC_REPORTEDCLIVER:
			{
				pszKey += strlen(sm_szLoadKeys[index]);
				GETNONWHITESPACE( pszKey );

				int iCliVer = (m_reportedCliver&0xFFFFFF0);
				if ( pszKey && strlen(pszKey) )
					iCliVer = m_reportedCliver;

				TCHAR szVersion[ 128 ];
				sVal = CCrypt::WriteClientVerString( iCliVer, szVersion );
			}
			break;
		case CC_SCREENSIZE:
			{
				if ( pszKey[10] == '.' )
				{
					pszKey += strlen(sm_szLoadKeys[index]);
					SKIP_SEPARATORS(pszKey);

					if ( !strnicmp("X", pszKey, 1) )
						sVal.Format( "%d", m_ScreenSize.x );
					else if ( !strnicmp("Y", pszKey, 1) )
						sVal.Format( "%d", m_ScreenSize.y );
					else
						return( false );
				}
				else
					sVal.Format( "%d,%d", m_ScreenSize.x, m_ScreenSize.y );
			} break;
		case CC_TARG:
			sVal.FormatVal( m_Targ_UID );
			break;
		case CC_TARGP:
			if ( pszKey[5] == '.' )
			{
				return m_Targ_p.r_WriteVal( pszKey+6, sVal );
			}
			sVal = m_Targ_p.WriteUsed();
			break;
		case CC_TARGPROP:
			sVal.FormatVal( m_Prop_UID );
			break;
		case CC_TARGPRV:
			sVal.FormatVal( m_Targ_PrvUID );
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
	if ( GetAccount() == NULL )
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
			GetAccount()->TogPrivFlags( PRIV_ALLMOVE, s.GetArgStr() );
			if ( IsSetOF( OF_Command_Sysmsgs ) )
				m_pChar->SysMessage( IsPriv(PRIV_ALLMOVE)? "Allmove ON" : "Allmove OFF" );
			addReSync();
			break;
		case CC_ALLSHOW:
			addRemoveAll( true, true );
			GetAccount()->TogPrivFlags( PRIV_ALLSHOW, s.GetArgStr() );
			if ( IsSetOF( OF_Command_Sysmsgs ) )
				m_pChar->SysMessage( IsPriv(PRIV_ALLSHOW)? "Allshow ON" : "Allshow OFF" );
			addReSync();
			break;
		case CC_DEBUG:
			GetAccount()->TogPrivFlags( PRIV_DEBUG, s.GetArgStr() );
			if ( IsSetOF( OF_Command_Sysmsgs ) )
					m_pChar->SysMessage( IsPriv(PRIV_DEBUG)? "Debug ON" : "Debug OFF" );
			addRemoveAll( true, false );
			addReSync();
			break;
		case CC_DETAIL:
			GetAccount()->TogPrivFlags( PRIV_DETAIL, s.GetArgStr() );
			if ( IsSetOF( OF_Command_Sysmsgs ) )
					m_pChar->SysMessage( IsPriv(PRIV_DETAIL)? "Detail ON" : "Detail OFF" );
			break;
		case CC_GM: // toggle your GM status on/off
			if ( GetPrivLevel() >= PLEVEL_GM )
			{
				GetAccount()->TogPrivFlags( PRIV_GM, s.GetArgStr() );
				if ( IsSetOF( OF_Command_Sysmsgs ) )
					m_pChar->SysMessage( IsPriv(PRIV_GM)? "GM ON" : "GM OFF" );
			}
			break;
		case CC_HEARALL:
			GetAccount()->TogPrivFlags( PRIV_HEARALL, s.GetArgStr() );
			if ( IsSetOF( OF_Command_Sysmsgs ) )
					m_pChar->SysMessage( IsPriv(PRIV_HEARALL)? "Hearall ON" : "Hearall OFF" );
			break;
		case CC_PRIVSHOW:
			// Hide my priv title.
			if ( GetPrivLevel() >= PLEVEL_Counsel )
			{
				if ( ! s.HasArgs())
				{
					GetAccount()->TogPrivFlags( PRIV_PRIV_NOSHOW, NULL );
				}
				else if ( s.GetArgVal() )
				{
					GetAccount()->ClearPrivFlags( PRIV_PRIV_NOSHOW );
				}
				else
				{
					GetAccount()->SetPrivFlags( PRIV_PRIV_NOSHOW );
				}
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
	if ( s.IsKeyHead( "SET", 3 ) && ( g_Cfg.m_Functions.FindKey( pszKey ) == -1 ) )
	{
		PLEVEL_TYPE ilevel = g_Cfg.GetPrivCommandLevel( "SET" );
		if ( ilevel > GetPrivLevel() )
			return( false );

		ASSERT( m_pChar );
		addTargetVerb( pszKey+3, s.GetArgRaw());
		return( true );
	}

	if ( toupper( pszKey[0] ) == 'X' && ( g_Cfg.m_Functions.FindKey( pszKey ) == -1 ) )
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
			if ( s.HasArgs())
			{
				// FindItemName ???
				TCHAR * pszArgs = s.GetArgStr();
				RESOURCE_ID rid = g_Cfg.ResourceGetID( RES_ITEMDEF, (LPCTSTR&)pszArgs );

				if ( rid.GetResType() == RES_CHARDEF )
				{
					m_Targ_PrvUID.InitUID();
					return Cmd_CreateChar( (CREID_TYPE) rid.GetResIndex(), SPELL_Summon, false );
				}

				ITEMID_TYPE id = (ITEMID_TYPE) rid.GetResIndex();
				return Cmd_CreateItem( id );
			}
			else
			{
				Menu_Setup( g_Cfg.ResourceGetIDType( RES_MENU, "MENU_ADDITEM"));
			}
			break;
		case CV_ADDBUFF:
			{
				if ( !s.HasArgs() ) 
				{
					DEBUG_ERR(("No AddBuff arguments\n"));
					break;
				}
				TCHAR * ppArgs[7];
				int ArgCount;
				int iArgs[4];

				if( (ArgCount = Str_ParseCmds( s.GetArgStr(), ppArgs, COUNTOF( ppArgs ))) < 5) {
					DEBUG_ERR(("Too few addbuff arguments\n"));
					break;
				}
				BYTE MBUArg[18] = {0};
				for (char idx = 0; idx != 4; ++idx) {
					if (!IsStrNumeric(ppArgs[idx]) || IsStrEmpty(ppArgs[idx])) {
						DEBUG_ERR(("Invalid addbuff argument number %i\n",idx+1));
						return true;
					}
					iArgs[idx] = g_Exp.GetVal(ppArgs[idx]);
				}
				if (iArgs[0] < 1001 || iArgs[0] > 1048 || iArgs[0] == 0x3EB || iArgs[0] == 0x3EC ) {
					DEBUG_ERR(("Invalid Buff Icon ID\n"));
					break;
				}
				if (strlen(ppArgs[4]) > 3) {
					DEBUG_ERR(("Invalid addbuff argument number 5\n"));
					break;
				}
				CharToMultiByteNonNull(MBUArg, ppArgs[4], 3);
				if (ppArgs[5])
					CharToMultiByteNonNull(MBUArg + 6 * sizeof(BYTE), ppArgs[5], 3);
				if (ppArgs[6])
					CharToMultiByteNonNull(MBUArg + 12 * sizeof(BYTE), ppArgs[6], 3);
				addBuff( iArgs[0], iArgs[1], iArgs[2], iArgs[3], MBUArg);

			}
			break;
		case CV_REMOVEBUFF:
			{
				if ( !s.HasArgs() ) {
					DEBUG_ERR(("No removebuff arguments.\n"));
					break;
				}
				long IconId = s.GetArgVal();
				if (IconId < 1001 || IconId > 1048 || IconId == 0x3EB || IconId == 0x3EC) {
					DEBUG_ERR(("Invalid Buff Icon ID\n"));
					break;
				}
				removeBuff(static_cast<WORD>(IconId));
			}
			break;
		case CV_ADDCLILOC:
			// Add cliloc in @ClientTooltip trigger
			{
				TCHAR * ppLocArgs[256];
				int qty = Str_ParseCmds(s.GetArgRaw(), ppLocArgs, COUNTOF(ppLocArgs), ",");
				DWORD clilocid = Exp_GetVal(ppLocArgs[0]);

				CGString LocArgs;
				for ( int y = 1 ; y < qty; y++ )
				{
					if ( LocArgs.GetLength() )
						LocArgs += "\t";
					LocArgs += ( !strcmp(ppLocArgs[y], "NULL") ? " " : ppLocArgs[y] );
				}

				if ( g_Cfg.m_wDebugFlags & DEBUGF_SCRIPTS )
					g_Log.EventDebug("SCRIPT: addcliloc(%d,'%s')\n", clilocid, (LPCTSTR)LocArgs);
				this->m_TooltipData.Add(new CClientTooltip(clilocid, LocArgs));
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
				if (( !IsStrNumeric(ppLocArgs[0]) ) || ( !IsStrNumeric(ppLocArgs[1]) ) )
				{
					DEBUG_ERR(("Bad AddContextEntry usage: Check first 2 arguments!\n"));
					return true;
				}
				if (( !IsStrNumeric(ppLocArgs[2]) ) && ( !IsStrEmpty(ppLocArgs[2]) ))
				{
					DEBUG_ERR(("Bad AddContextEntry usage: 3rd parameter must be a number!\n"));
					return true;
				}
				if (( !IsStrNumeric(ppLocArgs[3]) ) && ( !IsStrEmpty(ppLocArgs[3]) ))
				{
					DEBUG_ERR(("Bad AddContextEntry usage: 4th parameter must be a number!\n"));
					return true;
				}
				int entrytag = Exp_GetVal(ppLocArgs[0]);
				if ( entrytag < 100 )
				{
					DEBUG_ERR(("Bad AddContextEntry usage: TextEntry < 100 is reserved for server usage!\n"));
					return true;
				}
				DWORD textid = Exp_GetVal(ppLocArgs[1]);

				if ( textid > 32767 )
				{
					if ( ( textid >= 3000000 ) && ( textid <= 3032767) )
					{
						textid -= 3000000;
					} else
					{
						DEBUG_ERR(("Illegal CliLoc number. Only values between 3,000,000 and 3,032,767 (or 0 and 32,767) are permitted.\n"));
						return true;
					}
				}

				AOSPopupMenuAdd( entrytag, int(textid), Exp_GetVal(ppLocArgs[2]), Exp_GetVal(ppLocArgs[3]) );
			}
			break;
		case CV_ARROWQUEST:
			{
				int piVal[2];
				int iQty = Str_ParseCmds( s.GetArgRaw(), piVal, COUNTOF(piVal));
				addArrowQuest( piVal[0], piVal[1] );
#ifdef _ALPHASPHERE
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
				//	Loop the world searching for bad spawns
				bool fFound = false;
				for ( int m = 0; m < 256; m++ )
				{
					if ( !g_MapList.m_maps[m] ) continue;

					for ( DWORD d = 0; d < g_MapList.GetSectorQty(m); d++ )
					{
						CSector	*pSector = g_World.GetSector(m, d);
						if ( !pSector ) continue;

						CItem	*pNext;
						CItem	*pItem = STATIC_CAST <CItem*>(pSector->m_Items_Timer.GetHead());
						for ( ; pItem != NULL; pItem = pNext )
						{
							pNext = pItem->GetNext();

							if ( pItem->IsType(IT_SPAWN_ITEM) || pItem->IsType(IT_SPAWN_CHAR) )
							{
								CResourceDef	*pDef = pItem->Spawn_FixDef();
								if ( !pDef )
								{
									RESOURCE_ID_BASE	rid = ( pItem->IsType(IT_SPAWN_ITEM) ? pItem->m_itSpawnItem.m_ItemID : pItem->m_itSpawnChar.m_CharID);

									CPointMap	pt = pItem->GetTopPoint();
									m_pChar->Spell_Teleport(pt, true, false);
									m_pChar->m_Act_Targ = pItem->GetUID();
									SysMessagef("Bad spawn (0%lx, id=%s). Set as ACT", (DWORD)pItem->GetUID(), g_Cfg.ResourceGetName(rid));
									fFound = true;
								}
							}
						}
					}
				}
				if ( ! fFound )
					SysMessage(g_Cfg.GetDefaultMsg( DEFMSG_NO_BAD_SPAWNS ));
			}
			break;
		case CV_BANKSELF: // open my own bank
			addBankOpen( m_pChar, (LAYER_TYPE) s.GetArgVal());
			break;
		case CV_CAST:
			{
				SPELL_TYPE spell = (SPELL_TYPE)g_Cfg.ResourceGetIndexType(RES_SPELL, s.GetArgStr());
				CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
				if (pSpellDef == NULL)
					return true;

				if ( IsSetMagicFlags( MAGICF_PRECAST ) && !pSpellDef->IsSpellType( SPELLFLAG_NOPRECAST ) )
				{
					int skill;
					if (!pSpellDef->GetPrimarySkill(&skill, NULL))
						return true;

					m_tmSkillMagery.m_Spell = spell;	// m_atMagery.m_Spell
					m_pChar->m_atMagery.m_Spell = spell;
					m_Targ_UID = dynamic_cast <CObjBase *>(pSrc)->GetUID();	// default target.
					m_Targ_PrvUID = dynamic_cast <CObjBase *>(pSrc)->GetUID();
					m_pChar->Skill_Start( (SKILL_TYPE)skill );
					break;
				}
				else
					Cmd_Skill_Magery(spell, dynamic_cast <CObjBase *>(pSrc));
			}
			break;

		case CV_CHARLIST:
			{
				// ussually just a gm command
				CCommand cmd;
				int charCount = Setup_FillCharList( cmd.CharList3.m_char, m_pChar );
				int len = sizeof(cmd.CharList3) + (sizeof(cmd.CharList3.m_char[0]) * (charCount - 1));

				cmd.CharList3.m_Cmd = XCMD_CharList3;
				cmd.CharList3.m_len = len;
				cmd.CharList3.m_count = charCount;
				cmd.CharList3.m_unk = 0;

				xSendPkt( &cmd, len);

				CharDisconnect();	// since there is no undoing this in the client.
				SetTargMode( CLIMODE_SETUP_CHARLIST );
			}
			break;

		case CV_CTAGLIST:
			if ( ! strcmpi( s.GetArgStr(), "log" ))
				pSrc = (CTextConsole *)&g_Serv;
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
					WORD wBlockFlags = m_pChar->GetMoveBlockFlags();
					if ( IsSetEF( EF_WalkCheck ) )
						pnt.m_z = g_World.GetHeightPoint_New( pnt, wBlockFlags );	// ??? Get Area
					else
						pnt.m_z = g_World.GetHeightPoint( pnt, wBlockFlags );	// ??? Get Area
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
					CEvent Event;
					CPointMap pt = pObj->GetUnkPoint();
					Event.Target.m_context = GetTargMode();
					Event.Target.m_x = pt.m_x;
					Event.Target.m_y = pt.m_y;
					Event.Target.m_z = pt.m_z;
					Event.Target.m_UID = pObj->GetUID();
					Event.Target.m_id = 0;
					Event_Target( &Event );
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
				int piMidi[64];
				int iQty = Str_ParseCmds( s.GetArgStr(), piMidi, COUNTOF(piMidi));
				if ( iQty > 0 )
				{
					addMusic( piMidi[ Calc_GetRandVal( iQty ) ] );
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
		case CV_PAGE:
			Cmd_GM_PageCmd( s.GetArgStr());
			break;
		case CV_REPAIR:
			addTarget( CLIMODE_TARG_REPAIR, g_Cfg.GetDefaultMsg( DEFMSG_SELECT_ITEM_REPAIR ) );
			break;
		case CV_FLUSH:
			xFlush();
			break;
		case CV_RESEND:
			addReSync();
			break;
		case CV_SAVE:
			g_World.Save(s.GetArgVal());
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
				CEvent Event;
				Event.Target.m_context = GetTargMode();
				CPointMap pt = m_pChar->GetTopPoint();
				Event.Target.m_x = pt.m_x;
				Event.Target.m_y = pt.m_y;
				Event.Target.m_z = pt.m_z;
				Event.Target.m_UID = m_pChar->GetUID();
				Event.Target.m_id = 0;
				Event_Target(&Event);
				break;
			}
			return( false );
		case CV_SHOWSKILLS:
			addSkillWindow((SKILL_TYPE)MAX_SKILL); // Reload the real skills
			break;
		case CV_SKILLMENU:				// Just put up another menu.
			Cmd_Skill_Menu( g_Cfg.ResourceGetIDType( RES_SKILLMENU, s.GetArgStr()));
			break;
		case CV_SKILLSELECT:
			Event_Skill_Use( g_Cfg.FindSkillKey( s.GetArgStr() ) );
			break;
		case CV_SUMMON:	// from the spell skill script.
			// m_Targ_PrvUID should already be set.
			return Cmd_CreateChar( (CREID_TYPE) g_Cfg.ResourceGetIndexType( RES_CHARDEF, s.GetArgStr()), SPELL_Summon, true );
		case CV_SMSG:
		case CV_SYSMESSAGE:
			SysMessage( s.GetArgStr() );
			break;
		case CV_SMSGU:
		case CV_SYSMESSAGEUA:
			{
				TCHAR * pszArgs[5];
				int iArgQty = Str_ParseCmds( s.GetArgRaw(), pszArgs, 5 );
				if ( iArgQty > 4 )
				{
					// Font and mode are actually ignored here, but they never made a difference
					// anyway.. I'd like to keep the syntax similar to SAYUA
			 		NCHAR szBuffer[ MAX_TALK_BUFFER ];
					CvtSystemToNUNICODE( szBuffer, COUNTOF(szBuffer), pszArgs[4], -1 );

					addBarkUNICODE( szBuffer, NULL, (HUE_TYPE)Exp_GetVal(pszArgs[0]), TALKMODE_SYSTEM, (FONT_TYPE)0x03, pszArgs[3] );
				}
			}
			break;
		case CV_SMSGL:
		case CV_SYSMESSAGELOC:
			{
				TCHAR * ppArgs[256];
				int iArgQty = Str_ParseCmds( s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), "," );
				if ( iArgQty > 1 )
				{
					int hue = -1;
					if ( ppArgs[0] )
						hue = Exp_GetVal( ppArgs[0] );
					int iClilocId = Exp_GetVal( ppArgs[1] );

					if ( hue == -1 )	hue = HUE_TEXT_DEF;

					CGString CArgs;
					for ( int i = 2; i < iArgQty; i++ )
					{
						if ( CArgs.GetLength() )
							CArgs += "\t";
						CArgs += ( !strcmp(ppArgs[i], "NULL") ? " " : ppArgs[i] );
					}

					addBarkLocalized( iClilocId, NULL, (HUE_TYPE)hue, TALKMODE_SYSTEM, FONT_NORMAL, (TCHAR*)CArgs.GetPtr() );
				}
			}
			break;
		case CV_SMSGLEX:
		case CV_SYSMESSAGELOCEX:
			{
				TCHAR * ppArgs[256];
				int iArgQty = Str_ParseCmds( s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), "," );
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
					for ( int i = 4; i < iArgQty; i++ )
					{
						if ( CArgs.GetLength() )
							CArgs += "\t";
						CArgs += ( !strcmp(ppArgs[i], "NULL") ? " " : ppArgs[i] );
					}

					addBarkLocalizedEx( iClilocId, NULL, (HUE_TYPE)hue, TALKMODE_SYSTEM, FONT_NORMAL, (AFFIX_TYPE)affix, ppArgs[3], (TCHAR*)CArgs.GetPtr() );
				}
			}
			break;
		case CV_TELE:
			{
				CSpellDef *pSpellDef = g_Cfg.GetSpellDef(SPELL_Teleport);
				if (pSpellDef == NULL)
					return true;

				if ( IsSetMagicFlags( MAGICF_PRECAST ) && !pSpellDef->IsSpellType( SPELLFLAG_NOPRECAST ) )
				{
					int skill;
					if (!pSpellDef->GetPrimarySkill(&skill, NULL))
						return true;

					m_tmSkillMagery.m_Spell = SPELL_Teleport;
					m_pChar->m_atMagery.m_Spell = SPELL_Teleport;
					m_Targ_UID = dynamic_cast <CObjBase *>(pSrc)->GetUID();	// default target.
					m_Targ_PrvUID = dynamic_cast <CObjBase *>(pSrc)->GetUID();
					m_pChar->Skill_Start( (SKILL_TYPE)skill );
					break;
				}
				else
					Cmd_Skill_Magery( SPELL_Teleport, dynamic_cast <CObjBase *>(pSrc));
			}
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
