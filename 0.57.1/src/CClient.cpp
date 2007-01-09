#include "graysvr.h"
#include "CClient.h"
#include "network/network.h"
#include "network/send.h"
#include "common/version.h"

/////////////////////////////////////////////////////////////////
// -CClient stuff.

CClient::CClient(NetState *state)
{
	m_net = state;
	SetConnectType(CONNECT_UNK);	// don't know what sort of connect this is yet.
	NetworkIn::HistoryIP *hist = &g_Network->getHistoryForIP(peer());
	hist->m_connecting++;
	hist->m_connected++;
	
	m_Crypt.SetClientVer(0);
	m_pAccount = NULL;

	m_pChar = NULL;
	m_pGMPage = NULL;

	m_timeLogin.Init();
	m_timeLastSend =
	m_timeLastEvent = CServTime::GetCurrentTime();

	m_iWalkStepCount = 0;
	m_iWalkTimeAvg	= 100;
	ClearTargMode();

	m_Env.invalidate();

	g_Log.Event(LOGM_CLIENTS_LOG, "%x:Client connected [Total:%d] ('%s' %d/%d)\n",
		socketId(), g_Serv.StatGet(SERV_STAT_CLIENTS), speer(), hist->m_connecting, hist->m_connected);

	m_zLastMessage[0] = 0;
	m_zLastObjMessage[0] = 0;
	m_tNextPickup.Init();
	m_reportedCliver = 0;
	m_bClient3d = false; // Client by default are 2d
	m_wScreenx = 0;
	m_wScreeny = 0;
}


CClient::~CClient()
{
	bool bWasChar = (m_pChar != NULL);

	NetworkIn::HistoryIP *hist = &g_Network->getHistoryForIP(peer());
	if ( GetConnectType() != CONNECT_GAME )
		hist->m_connecting--;
	hist->m_connected--;

	CharDisconnect();
	Cmd_GM_PageClear();

	// Calling that in explicit way (other ways in implicit form sometime seem to fail)
	m_TagDefs.Empty();

	CAccount * pAccount = GetAccount();
	if ( bWasChar && pAccount )
	{
		pAccount->OnLogout(this, bWasChar);
		m_pAccount = NULL;
	}

	if ( m_net->isValid(this) )
	{
		m_net->clear();
	}
}

bool CClient::CanInstantLogOut() const
{
	if ( g_Serv.IsLoading())	// or exiting.
		return true;
	if ( ! g_Cfg.m_iClientLingerTime )
		return true;
	if ( IsPriv( PRIV_GM ))
		return true;
	if ( m_pChar == NULL )
		return true;
	if ( m_pChar->IsStatFlag(STATF_DEAD))
		return true;

	const CRegionWorld * pArea = m_pChar->GetRegion();
	if ( pArea == NULL )
		return true;
	if ( pArea->IsFlag( REGION_FLAG_INSTA_LOGOUT ))
		return true;
	return false;
}

void CClient::CharDisconnect()
{
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
	m_pChar->ClientDetach();	// we are not a client any more.

	CScriptTriggerArgs	args(iLingerTime, fCanInstaLogOut);
	m_pChar->OnTrigger(CTRIG_LogOut, m_pChar, &args);
	iLingerTime = args.m_iN1;
	fCanInstaLogOut = args.m_iN2;

	if ( iLingerTime <= 0 ) fCanInstaLogOut = true;

	// Gump memory cleanup, we don't want them on logged out players
	m_pChar->Memory_ClearTypes(MEMORY_GUMPRECORD);

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
	// Different sorts of clients.
	if ( !pszMsg || !*pszMsg ) return;
	CClient *pClient = const_cast <CClient*>(this);

	switch ( GetConnectType() )
	{
	case CONNECT_TELNET:
		{
			if ( ISINTRESOURCE(pszMsg) ) return;

			TEMPSTRING(buf);
			long index = 0;
			for ( const char *p = pszMsg; *p; p++ )
			{
				if ( *p == '\n' )
					buf[index++] = '\r';
				buf[index++] = *p;
			}
			pClient->xSend(buf, index);
		}
		return;
	case CONNECT_CRYPT:
	case CONNECT_LOGIN:
	case CONNECT_GAME:
		pClient->addSysMessage(pszMsg);
		return;

	case CONNECT_HTTP:
		pClient->m_Targ_Text = pszMsg;
		return;
	}
}

void CClient::Announce( bool fArrive ) const
{
	if ( !GetAccount() || !m_pChar || !m_pChar->m_pPlayer )
		return;

	// We have logged in or disconnected.
	// Annouce my arrival or departure.
	if ( GetPrivLevel() > PLEVEL_Player )		// notify of GMs
	{
		const char *zTitle;

		TEMPSTRING(zMsg);
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

		ClientIterator it;
		CClient *client;
		while ( client = it.next() )
		{
			if (( client == this ) || ( GetPrivLevel() > client->GetPrivLevel() ))
				continue;
			client->SysMessage(zMsg);
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
	m_pAccount->m_uidLastChar = m_pChar->GetUID();
}

////////////////////////////////////////////////////

bool CClient::CanSee( const CObjBaseTemplate * pObj ) const
{
	// Can player see item b
	if ( m_pChar == NULL )
		return false;
	return( m_pChar->CanSee( pObj ));
}

bool CClient::CanHear( const CObjBaseTemplate * pSrc, TALKMODE_TYPE mode ) const
{
	// can we hear this text or sound.

	if ( ! IsConnectTypePacket())
	{
		if ( GetConnectType() != CONNECT_TELNET )
			return false;
		if ( mode == TALKMODE_BROADCAST ) // && GetAccount()
			return true;
		return false;
	}

	if ( mode == TALKMODE_BROADCAST || pSrc == NULL )
		return true;
	if ( m_pChar == NULL )
		return false;

	if ( IsPriv( PRIV_HEARALL ) &&
		pSrc->IsChar()&&
		( mode == TALKMODE_SYSTEM || mode == TALKMODE_SAY || mode == TALKMODE_WHISPER || mode == TALKMODE_YELL ))
	{
		const CChar * pCharSrc = dynamic_cast <const CChar*> ( pSrc );
		if ( pCharSrc && pCharSrc->IsClient())
		{
			if ( pCharSrc->GetPrivLevel() <= GetPrivLevel())
			{
				return true;
			}
		}
	}

	return( m_pChar->CanHear( pSrc, mode ));
}

////////////////////////////////////////////////////

void CClient::addTargetVerb( LPCTSTR pszCmd, LPCTSTR pszArg )
{
	// Target a verb at some object .
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

	if ( GetPrivLevel() < g_Cfg.GetPrivCommandLevel(pszCmd) ) return;

	m_Targ_Text.Format("%s%s%s", pszCmd, (pszArg[0] && pszCmd[0]) ? " " : "", pszArg);
	TEMPSTRING(pszMsg);
	sprintf(pszMsg, "Select object to set/command '%s'", (LPCTSTR) m_Targ_Text);
	addTarget(CLIMODE_TARG_OBJ_SET, pszMsg);
}


void CClient::addTargetFunction( LPCTSTR pszFunction, bool fAllowGround, bool fCheckCrime )
{
	// Target a verb at some object .
	GETNONWHITESPACE(pszFunction);
	SKIP_SEPARATORS(pszFunction);

	m_Targ_Text.Format( pszFunction );
	addTarget( CLIMODE_TARG_OBJ_FUNC, "", fAllowGround, fCheckCrime );
}

void CClient::addPromptConsoleFunction( LPCTSTR pszFunction, LPCTSTR pszSysmessage )
{
	// Target a verb at some object .
	m_Targ_Text.Format( pszFunction );
	addPromptConsole( CLIMODE_PROMPT_SCRIPT_VERB, pszSysmessage );
}


enum CLIR_TYPE
{
	CLIR_ACCOUNT,
	CLIR_GMPAGEP,
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
	"PARTY",
	"TARG",
	"TARGPROP",
	"TARGPRV",
	NULL,
};

bool CClient::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
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
				return true;
			case CLIR_GMPAGEP:
				pRef = m_pGMPage;
				return true;
			case CLIR_PARTY:
				if ( !this->m_pChar->m_pParty )
					return false;
				pRef = this->m_pChar->m_pParty;
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
	return( CScriptObj::r_GetRef( pszKey, pRef ));
}

LPCTSTR const CClient::sm_szLoadKeys[CC_QTY+1] = // static
{
	#define ADD(a,b) b,
	#include "tables/CClient_props.tbl"
	#undef ADD
	NULL,
};

LPCTSTR const CClient::sm_szVerbKeys[CV_QTY+1] =	// static
{
	#define ADD(a,b) b,
	#include "tables/CClient_functions.tbl"
	#undef ADD
	NULL,
};

bool CClient::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	EXC_TRY("WriteVal");
	int index;
	
	if ( !strnicmp("CTAG.", pszKey, 5) )		//	CTAG.xxx - client tag
	{
		if ( pszKey[4] != '.' )
			return false;
		pszKey += 5;
		VariableList::Variable *vardef = m_TagDefs.GetKey(pszKey);
		sVal = vardef ? vardef->GetValStr() : "";
		return true;
	}

	if ( !strnicmp("CTAG0.", pszKey, 6) )		//	CTAG0.xxx - client tag
	{
		if ( pszKey[5] != '.' )
			return false;
		pszKey += 6;
		VariableList::Variable *vardef = m_TagDefs.GetKey(pszKey);
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
		case CC_PRIVSHOW:
			// Show my priv title.
			sVal.FormatVal( ! IsPriv( PRIV_PRIV_NOSHOW ));
			break;
		case CC_REPORTEDCLIVER:
			{
				pszKey += strlen(sm_szLoadKeys[index]);
				GETNONWHITESPACE( pszKey );

				int iCliVer = (m_reportedCliver&0xFFFFF0);
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
						sVal.Format( "%d", m_wScreenx );
					else if ( !strnicmp("Y", pszKey, 1) )
						sVal.Format( "%d", m_wScreeny );
					else
						return false;
				}
				else
					sVal.Format( "%d,%d", m_wScreenx, m_wScreeny );
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
	EXC_TRY("LoadVal");
	if ( GetAccount() == NULL )
		return false;

	LPCTSTR pszKey = s.GetKey();

	if ( s.IsKeyHead( "CTAG.", 5 ) || s.IsKeyHead( "CTAG0.", 6 ) )
	{
		bool fZero = s.IsKeyHead( "CTAG0.", 6 );
		bool fQuoted = false;

		pszKey = pszKey + (fZero ? 6 : 5);
		m_TagDefs.SetStr( pszKey, fQuoted, s.GetArgStr( &fQuoted ), fZero );
		return true;
	}
		
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 ))
	{
		case CC_ALLMOVE:
			GetAccount()->TogPrivFlags( PRIV_ALLMOVE, s.GetArgStr() );
			addReSync();
			break;
		case CC_ALLSHOW:
			addRemoveAll( true, true );
			GetAccount()->TogPrivFlags( PRIV_ALLSHOW, s.GetArgStr() );
			addReSync();
			break;
		case CC_DEBUG:
			GetAccount()->TogPrivFlags( PRIV_DEBUG, s.GetArgStr() );
			addRemoveAll( true, false );
			addReSync();
			break;
		case CC_DETAIL:
			GetAccount()->TogPrivFlags( PRIV_DETAIL, s.GetArgStr() );
			break;
		case CC_GM: // toggle your GM status on/off
			if ( GetPrivLevel() >= PLEVEL_GM )
			{
				GetAccount()->TogPrivFlags( PRIV_GM, s.GetArgStr() );
			}
			break;
		case CC_HEARALL:
			GetAccount()->TogPrivFlags( PRIV_HEARALL, s.GetArgStr() );
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
			return false;
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
	EXC_TRY("Verb");
	// NOTE: This can be called directly from a RES_WEBPAGE script.
	//  So do not assume we are a game client !
	// NOTE: Mostly called from CChar::r_Verb
	// NOTE: Little security here so watch out for dangerous scripts !
	LPCTSTR pszKey = s.GetKey();

	// Old ver
	if ( s.IsKeyHead( "SET", 3 ) && ( g_Cfg.m_Functions.FindKey( pszKey ) == -1 ) )
	{
		PLEVEL_TYPE ilevel = g_Cfg.GetPrivCommandLevel( "SET" );
		if ( ilevel > GetPrivLevel() )
			return false;

		addTargetVerb( pszKey+3, s.GetArgRaw());
		return true;
	}

	if ( toupper( pszKey[0] ) == 'X' && ( g_Cfg.m_Functions.FindKey( pszKey ) == -1 ) )
	{
		PLEVEL_TYPE ilevel = g_Cfg.GetPrivCommandLevel( "SET" );
		if ( ilevel > GetPrivLevel() )
			return false;

		// Target this command verb on some other object.
		addTargetVerb( pszKey+1, s.GetArgRaw());
		return true;
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
					Cmd_CreateChar((CREID_TYPE)rid.GetResIndex(), SPELL_Summon, false);
				}
				else
				{
					ITEMID_TYPE id = (ITEMID_TYPE)rid.GetResIndex();
					Cmd_CreateItem(id);
				}
			}
			else
			{
				Menu_Setup( g_Cfg.ResourceGetIDType( RES_MENU, "MENU_ADDITEM"));
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
				this->m_TooltipData.Add(new CClientTooltip(clilocid, LocArgs));
			}
			break;
		case CV_ARROWQUEST:
			{
				int piVal[2];
				int iQty = Str_ParseCmds( s.GetArgRaw(), piVal, COUNTOF(piVal));
				addArrowQuest( piVal[0], piVal[1] );
			}
			break;
		case CV_BADSPAWN:
			{
				//	Loop the world searching for bad spawns
				for ( int m = 0; m < 256; m++ )
				{
					if ( !g_MapList.m_maps[m] ) continue;

					for ( DWORD d = 0; d < g_MapList.GetSectorQty(m); d++ )
					{
						CSector	*pSector = g_World.GetSector(m, d);
						if ( !pSector ) continue;

						CItem	*pNext;
						CItem	*pItem = static_cast <CItem*>(pSector->m_Items_Timer.GetHead());
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
									SysMessagef("Bad spawn (0%lx, id=%s). Set as ACT", pItem->uid(), g_Cfg.ResourceGetName(rid));
									goto endbadspawn;
								}
							}
						}
					}
				}

				SysMessage("There are no found bad spawn points left.");
	endbadspawn:;
			}
			break;
		case CV_BANKSELF: // open my own bank
			addBankOpen( m_pChar, (LAYER_TYPE) s.GetArgVal());
			break;
		case CV_CAST:
			if ( IsSetMagicFlags( MAGICF_PRECAST ) )
			{
				m_tmSkillMagery.m_Spell = (SPELL_TYPE) g_Cfg.ResourceGetIndexType( RES_SPELL, s.GetArgStr());	// m_atMagery.m_Spell
				m_pChar->m_atMagery.m_Spell = (SPELL_TYPE) g_Cfg.ResourceGetIndexType( RES_SPELL, s.GetArgStr());
				m_Targ_UID = dynamic_cast <CObjBase *>(pSrc)->GetUID();	// default target.
				m_Targ_PrvUID = dynamic_cast <CObjBase *>(pSrc)->GetUID();
				m_pChar->Skill_Start( SKILL_MAGERY );
				break;
			}
			else
				Cmd_Skill_Magery( (SPELL_TYPE) g_Cfg.ResourceGetIndexType( RES_SPELL, s.GetArgStr()), dynamic_cast <CObjBase *>(pSrc));
			break;
		case CV_CHARLIST:
			{
				// ussually just a gm command
				CCommand cmd;
				cmd.CharList3.m_Cmd = XCMD_CharList3;
				cmd.CharList3.m_len = sizeof( cmd.CharList3 );
				cmd.CharList3.m_count = Setup_FillCharList( cmd.CharList3.m_char, m_pChar );
				cmd.CharList3.m_unk = 0;

				xSend(&cmd, sizeof(cmd.CharList3));

				CharDisconnect();	// since there is no undoing this in the client.
				SetTargMode( CLIMODE_SETUP_CHARLIST );
			}
			break;

		case CV_CTAGLIST:
			if ( !strcmpi(s.GetArgStr(), "log") )
				pSrc = (CTextConsole *)&g_Serv;
			m_TagDefs.DumpKeys(pSrc, "CTAG.");
			break;

		case CV_EVERBTARG:
			m_Targ_Text = s.GetArgStr();
			addPromptConsole( CLIMODE_PROMPT_TARG_VERB, m_Targ_Text.IsEmpty() ? "Enter the verb" : "Enter the text" );
			break;

		case CV_GMPAGE:
			m_Targ_Text = s.GetArgStr();
			if ( !m_Targ_Text.IsEmpty() && !strnicmp( m_Targ_Text, "ADD ", 4 ) )
			{
				Cmd_GM_Page( m_Targ_Text );
				break;
			}
			addPromptConsole( CLIMODE_PROMPT_GM_PAGE_TEXT, g_Cfg.GetDefaultMsg( DEFMSG_GMPAGE_PROMPT ) );
			break;
		case CV_GOTARG: // go to my (preselected) target.
			{
				CObjBase * pObj = m_Targ_UID.ObjFind();
				if ( pObj != NULL )
				{
					CPointMap po = pObj->GetTopLevelObj()->GetTopPoint();
					CPointMap pnt = po;
					pnt.MoveN( DIR_W, 3 );
					WORD wBlockFlags = m_pChar->GetMoveBlockFlags();
					pnt.m_z = g_World.GetHeightPoint( pnt, wBlockFlags );	// ??? Get Area
					m_pChar->m_dirFace = pnt.GetDir( po, m_pChar->m_dirFace ); // Face the player
					m_pChar->Spell_Teleport( pnt, true, false );
				}
			}
			break;
		case CV_INFO:
			// We could also get ground tile info.
			addTarget( CLIMODE_TARG_OBJ_INFO, "What would you like info on?", true, false );
			break;
		case CV_INFORMATION:
			SysMessage( g_Serv.GetStatusString( 0x22 ));
			SysMessage( g_Serv.GetStatusString( 0x24 ));
			break;
		case CV_LAST:
			// Fake Previous target.
			if ( GetTargMode() >= CLIMODE_MOUSE_TYPE )
			{
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
			return false;
		case CV_LINK:	// link doors
			m_Targ_UID.InitUID();
			addTarget( CLIMODE_TARG_LINK, "Select the item to link." );
			break;

		case CV_MENU:
			Menu_Setup( g_Cfg.ResourceGetIDType( RES_MENU, s.GetArgStr()));
			break;
		case CV_MIDILIST:
			{
				int piMidi[64];
				int iQty = Str_ParseCmds(s.GetArgStr(), piMidi, COUNTOF(piMidi));
				if ( iQty > 0 )
				{
					PacketPlayMusic *cmd = new PacketPlayMusic(piMidi[Calc_GetRandVal(iQty)], this);
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
				addTarget( CLIMODE_TARG_TILE, "Select area to Nudge", true );
			}
			break;

		case CV_NUKE:
			m_Targ_Text = s.GetArgRaw();
			m_tmTile.m_ptFirst.InitPoint(); // Clear this first
			m_tmTile.m_Code = CV_NUKE;	// set nuke code.
			addTarget( CLIMODE_TARG_TILE, "Select area to Nuke", true );
			break;
		case CV_NUKECHAR:
			m_Targ_Text = s.GetArgRaw();
			m_tmTile.m_ptFirst.InitPoint(); // Clear this first
			m_tmTile.m_Code = CV_NUKECHAR;	// set nuke code.
			addTarget( CLIMODE_TARG_TILE, "Select area to Nuke Chars", true );
			break;
		case CV_PAGE:
			Cmd_GM_PageCmd( s.GetArgStr());
			break;
		case CV_REPAIR:
			addTarget( CLIMODE_TARG_REPAIR, "What item do you want to repair?" );
			break;
		case CV_RESEND:
			addReSync();
			break;
		case CV_SENDPACKET:
			SendPacket( s.GetArgStr() );
			break;
		case CV_SELF:
			// Fake self target.
			if ( GetTargMode() >= CLIMODE_MOUSE_TYPE )
			{
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
			return false;
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
				int iArgQty = Str_ParseCmds( s.GetArgRaw(), ppArgs, COUNTOF(ppArgs) );
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
		case CV_TELE:
			if ( IsSetMagicFlags( MAGICF_PRECAST ) )
			{
				m_tmSkillMagery.m_Spell = SPELL_Teleport;
				m_pChar->m_atMagery.m_Spell = SPELL_Teleport;
				m_Targ_UID = dynamic_cast <CObjBase *>(pSrc)->GetUID();	// default target.
				m_Targ_PrvUID = dynamic_cast <CObjBase *>(pSrc)->GetUID();
				m_pChar->Skill_Start( SKILL_MAGERY );
				break;
			}
			else
				Cmd_Skill_Magery( SPELL_Teleport, dynamic_cast <CObjBase *>(pSrc));
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
		case CV_VERSION:
			SysMessage(SPHERE_FULL);
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
					return true;
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

long CClient::socketId()
{
	return m_net->id();
}

CSocketAddress &CClient::peer()
{
	return m_net->m_peerAddr;
}

LPCTSTR CClient::speer()
{
	return m_net->m_peerAddr.GetAddrStr();
}
