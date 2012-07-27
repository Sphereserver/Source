//
// CCharNPC.CPP
// Copyright Menace Software (www.menasoft.com).
//
// Actions specific to an NPC.
//

#include "graysvr.h"	// predef header.
#include "CClient.h"

enum CNC_TYPE
{
	#define ADD(a,b) CNC_##a,
	#include "../tables/CCharNpc_props.tbl"
	#undef ADD
	CNC_QTY
};

LPCTSTR const CCharNPC::sm_szLoadKeys[CNC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CCharNpc_props.tbl"
	#undef ADD
	NULL
};

void CChar::ClearNPC()
{
	ADDTOCALLSTACK("CChar::ClearNPC");
	if ( m_pNPC == NULL )
		return;

	delete m_pNPC;
	m_pNPC = NULL;
}

void CChar::ClearPlayer()
{
	ADDTOCALLSTACK("CChar::ClearPlayer");
	if ( m_pPlayer == NULL )
		return;

	// unlink me from my account.
	if ( g_Serv.m_iModeCode != SERVMODE_Exiting )
	{
		if ( m_pPlayer->m_pAccount )
		{
			DEBUG_WARN(( "Player delete '%s' name '%s'\n",
				(LPCTSTR) m_pPlayer->GetAccount()->GetName(), (LPCTSTR) GetName()));
		}
		else
		{
			DEBUG_WARN(( "Player delete from account name '%s'\n",
				(LPCTSTR) GetName()));
		}
	}

	// Is this valid ?
	m_pPlayer->GetAccount()->DetachChar( this );
	delete m_pPlayer;
	m_pPlayer = NULL;
}

bool CChar::SetPlayerAccount(CAccount *pAccount)
{
	ADDTOCALLSTACK("CChar::SetPlayerAccount");
	// Set up the char as a Player.
	if ( !pAccount )
		return false;

	if ( m_pPlayer )
	{
		if ( m_pPlayer->GetAccount() == pAccount )
			return true;

		DEBUG_ERR(("SetPlayerAccount '%s' already set '%s' != '%s'!\n", GetName(), m_pPlayer->GetAccount()->GetName(), pAccount->GetName()));
		return false;
	}

	if ( m_pNPC )
	{
		// This could happen if the account is set manually through
		// scripts
		ClearNPC();
	}

	m_pPlayer = new CCharPlayer(this, pAccount);
	pAccount->AttachChar(this);
	return true;
}



bool CChar::SetPlayerAccount( LPCTSTR pszAccName )
{
	ADDTOCALLSTACK("CChar::SetPlayerAccount");
	CAccountRef pAccount = g_Accounts.Account_FindCreate( pszAccName, g_Serv.m_eAccApp == ACCAPP_Free );
	if ( pAccount == NULL )
	{
		DEBUG_ERR(( "SetPlayerAccount '%s' can't find '%s'!\n", GetName(), pszAccName ));
		return false;
	}
	return( SetPlayerAccount( pAccount ));
}



bool CChar::SetNPCBrain( NPCBRAIN_TYPE NPCBrain )
{
	ADDTOCALLSTACK("CChar::SetNPCBrain");
	// Set up the char as an NPC
	if ( NPCBrain == NPCBRAIN_NONE || IsClient())
	{
		DEBUG_ERR(( "SetNPCBrain NULL\n" ));
		return false;
	}
	if ( m_pPlayer != NULL )
	{
		if ( m_pPlayer->GetAccount() != NULL )
		{
			DEBUG_ERR(( "SetNPCBrain to Player Account '%s'\n", m_pPlayer->GetAccount()->GetName() ));
		}
		else
		{
			DEBUG_ERR(( "SetNPCBrain to Player Name '%s'\n", GetName()));
		}
		return false;
	}
	if ( m_pNPC == NULL )
	{
		m_pNPC = new CCharNPC( this, NPCBrain );
	}
	else
	{
		// just replace existing brain.
		m_pNPC->m_Brain = NPCBrain;
	}
	return( true );
}

//////////////////////////
// -CCharPlayer

enum CPC_TYPE	// Player char.
{
	#define ADD(a,b) CPC_##a,
	#include "../tables/CCharPlayer_props.tbl"
	#undef ADD
	CPC_QTY
};

LPCTSTR const CCharPlayer::sm_szLoadKeys[CPC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CCharPlayer_props.tbl"
	#undef ADD
	NULL
};

CCharPlayer::CCharPlayer(CChar *pChar, CAccount *pAccount) : m_pAccount(pAccount)
{
	m_wDeaths = m_wMurders = 0;
	m_curFollower = m_maxFollower = 0;
	m_luck = m_iTithingPoints = 0;
	m_speedMode = 0;
	m_pflag = 0;
	m_bKrToolbarEnabled = false;
	m_timeLastUsed.Init();

	memset(m_SkillLock, 0, sizeof(m_SkillLock));
	memset(m_StatLock, 0, sizeof(m_StatLock));
	SetSkillClass(pChar, RESOURCE_ID(RES_SKILLCLASS));
}

CCharPlayer::~CCharPlayer()
{
	m_Speech.RemoveAll();
}

CAccountRef CCharPlayer::GetAccount() const
{
	ASSERT( m_pAccount );
	return( m_pAccount );
}

bool CCharPlayer::getKrToolbarStatus()
{
	return m_bKrToolbarEnabled;
}

bool CCharPlayer::SetSkillClass( CChar * pChar, RESOURCE_ID rid )
{
	ADDTOCALLSTACK("CCharPlayer::SetSkillClass");
	CResourceDef * pDef = g_Cfg.ResourceGetDef(rid);
	if ( !pDef )
		return false;

	CSkillClassDef* pLink = STATIC_CAST <CSkillClassDef*>(pDef);
	if ( !pLink )
		return false;
	if ( pLink == GetSkillClass() )
		return true;

	// Remove any previous skillclass from the Events block.
	size_t i = pChar->m_OEvents.FindResourceType(RES_SKILLCLASS);
	if ( i != pChar->m_OEvents.BadIndex() )
		pChar->m_OEvents.RemoveAt(i);

	m_SkillClass.SetRef(pLink);

	// set it as m_Events block as well.
	pChar->m_OEvents.Add(pLink);
	return true;
}

CSkillClassDef * CCharPlayer::GetSkillClass() const
{
	ADDTOCALLSTACK("CCharPlayer::GetSkillClass");
	// This should always return NON-NULL.

	CResourceLink * pLink = m_SkillClass.GetRef();
	if ( pLink == NULL )
		return( NULL );
	return( STATIC_CAST <CSkillClassDef *>(pLink));	
}

SKILL_TYPE CCharPlayer::Skill_GetLockType( LPCTSTR pszKey ) const
{
	ADDTOCALLSTACK("CCharPlayer::Skill_GetLockType");
	// only players can have skill locks.

	TCHAR szTmpKey[128];
	strcpylen( szTmpKey, pszKey, COUNTOF(szTmpKey) );

	TCHAR * ppArgs[3];
	size_t i = Str_ParseCmds( szTmpKey, ppArgs, COUNTOF(ppArgs), ".[]" );
	if ( i <= 1 )
		return( SKILL_NONE );

	if ( IsDigit( ppArgs[1][0] ))
	{
		i = ATOI( ppArgs[1] );
	}
	else
	{
		i = g_Cfg.FindSkillKey( ppArgs[1] );
	}
	if ( i >= g_Cfg.m_iMaxSkill )
		return( SKILL_NONE );
	return static_cast<SKILL_TYPE>(i);
}

SKILLLOCK_TYPE CCharPlayer::Skill_GetLock( SKILL_TYPE skill ) const
{
	ASSERT( skill >= 0 && static_cast<size_t>(skill) < COUNTOF(m_SkillLock));
	return static_cast<SKILLLOCK_TYPE>(m_SkillLock[skill]);
}

void CCharPlayer::Skill_SetLock( SKILL_TYPE skill, SKILLLOCK_TYPE state )
{
	ASSERT( skill >= 0 && static_cast<size_t>(skill) < COUNTOF(m_SkillLock));
	m_SkillLock[skill] = state;
}

STAT_TYPE CCharPlayer::Stat_GetLockType( LPCTSTR pszKey ) const
{
	ADDTOCALLSTACK("CCharPlayer::Stat_GetLockType");
	// only players can have skill locks.

	TCHAR szTmpKey[128];
	strcpylen( szTmpKey, pszKey, COUNTOF(szTmpKey) );

	TCHAR * ppArgs[3];
	size_t i = Str_ParseCmds( szTmpKey, ppArgs, COUNTOF(ppArgs), ".[]" );
	if ( i <= 1 )
		return( STAT_NONE );

	if ( IsDigit( ppArgs[1][0] ))
	{
		i = ATOI( ppArgs[1] );
	}
	else
	{
		i = g_Cfg.FindStatKey( ppArgs[1] );
	}
	if ( i >= STAT_BASE_QTY )
		return( STAT_NONE );
	return static_cast<STAT_TYPE>(i);
}

SKILLLOCK_TYPE CCharPlayer::Stat_GetLock( STAT_TYPE stat ) const
{
	ASSERT( stat >= 0 && static_cast<size_t>(stat) < COUNTOF(m_StatLock));
	return static_cast<SKILLLOCK_TYPE>(m_StatLock[stat]);
}

void CCharPlayer::Stat_SetLock( STAT_TYPE stat, SKILLLOCK_TYPE state )
{
	ASSERT( stat >= 0 && static_cast<size_t>(stat) < COUNTOF(m_StatLock));
	m_StatLock[stat] = state;
}

bool CCharPlayer::r_WriteVal( CChar * pChar, LPCTSTR pszKey, CGString & sVal )
{
	ADDTOCALLSTACK("CCharPlayer::r_WriteVal");
	EXC_TRY("WriteVal");

	if ( !pChar || !GetAccount() )
		return false;

	if ( !strnicmp(pszKey, "SKILLCLASS.", 11) )
	{
		return GetSkillClass()->r_WriteVal(pszKey + 11, sVal, pChar);
	}
	else if ( ( !strnicmp(pszKey, "GUILD", 5) ) || ( !strnicmp(pszKey, "TOWN", 4) ) )
	{
		bool bIsGuild = !strnicmp(pszKey, "GUILD", 5);
		pszKey += bIsGuild ? 5 : 4;
		if ( *pszKey == 0 )
		{
			CItemStone *pMyGuild = pChar->Guild_Find(bIsGuild ? MEMORY_GUILD : MEMORY_TOWN);
			if ( pMyGuild ) sVal.FormatHex((DWORD)pMyGuild->GetUID());
			else sVal.FormatVal(0);
			return true;
		}
		else if ( *pszKey == '.' )
		{
			pszKey += 1;
			CItemStone *pMyGuild = pChar->Guild_Find(bIsGuild ? MEMORY_GUILD : MEMORY_TOWN);
			if ( pMyGuild ) return pMyGuild->r_WriteVal(pszKey, sVal, pChar);
		}
		return false;
	}

	switch ( FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case CPC_ACCOUNT:
			sVal = GetAccount()->GetName();
			return( true );
		case CPC_CURFOLLOWER:
			sVal.FormatVal(m_curFollower);
			return( true );
		case CPC_DEATHS:
			sVal.FormatVal( m_wDeaths );
			return( true );
		case CPC_DSPEECH:
			m_Speech.WriteResourceRefList( sVal );
			return( true );
		case CPC_KILLS:
			sVal.FormatVal( m_wMurders );
			return( true );
		case CPC_KRTOOLBARSTATUS:
			sVal.FormatVal( m_bKrToolbarEnabled );
			return true;
		case CPC_ISDSPEECH:
			if ( pszKey[9] != '.' )
				return( false );
			pszKey += 10;
			sVal = m_Speech.ContainsResourceName(RES_SPEECH, pszKey) ? "1" : "0";
			return( true );
		case CPC_LASTUSED:
			sVal.FormatVal( - g_World.GetTimeDiff( m_timeLastUsed ) / TICK_PER_SEC );
			return( true );
		case CPC_LUCK:
			sVal.FormatVal(m_luck);
			return( true );
		case CPC_MAXFOLLOWER:
			sVal.FormatVal(m_maxFollower);
			return( true );
		case CPC_PFLAG:
			sVal.FormatVal(m_pflag);
			return( true );
		case CPC_PROFILE:
			{
				TCHAR szLine[SCRIPT_MAX_LINE_LEN-16];
				Str_MakeUnFiltered( szLine, m_sProfile, sizeof(szLine));
				sVal = szLine;
			}
			return( true );
		case CPC_SKILLCLASS:
			sVal = GetSkillClass()->GetResourceName();
			return( true );
		case CPC_SKILLLOCK:
			{
				// "SkillLock[alchemy]"
				SKILL_TYPE skill = Skill_GetLockType( pszKey );
				if ( skill <= SKILL_NONE )
					return( false );
				sVal.FormatVal( Skill_GetLock( skill ));
			} return( true );
		case CPC_SPEEDMODE:
			sVal.FormatVal( m_speedMode );
			return( true );
		case CPC_STATLOCK:
			{
				// "StatLock[str]"
				STAT_TYPE stat = Stat_GetLockType( pszKey );
				if (( stat <= STAT_NONE ) || ( stat >= STAT_BASE_QTY ))
					return( false );
				sVal.FormatVal( Stat_GetLock( stat ));
			} return( true );
			
		case CPC_TITHING:
			sVal.FormatVal(m_iTithingPoints);
			return( true );

		default:
			if ( FindTableSorted( pszKey, CCharNPC::sm_szLoadKeys, COUNTOF( CCharNPC::sm_szLoadKeys )-1 ) >= 0 )
			{
				sVal = "0";
				return( true );
			}
			return( false );
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pChar);
	EXC_DEBUG_END;
	return false;
}

bool CCharPlayer::r_LoadVal( CChar * pChar, CScript &s )
{
	ADDTOCALLSTACK("CCharPlayer::r_LoadVal");
	EXC_TRY("LoadVal");
	
	LPCTSTR pszKey = s.GetKey();
	LPCTSTR pszArgs = s.GetArgStr();

	if ( !strnicmp(pszKey, "GMPAGE", 6) )		//	GM pages
	{
		pszKey += 6;
		if ( *pszKey == '.' )						//	GMPAGE.*
		{
			SKIP_SEPARATORS(pszKey);
			size_t index = Exp_GetVal(pszKey);
			if ( index >= g_World.m_GMPages.GetCount() )
				return false;

			CGMPage* pPage = STATIC_CAST <CGMPage*> (g_World.m_GMPages.GetAt(index));
			if ( pPage == NULL )
				return false;

			SKIP_SEPARATORS(pszKey);
			if ( !strcmpi(pszKey, "HANDLE") )
			{
				CChar *ppChar = pChar;

				if ( *pszArgs )
					ppChar = dynamic_cast<CChar*>(g_World.FindUID(s.GetArgVal()));

				if ( ppChar == NULL )
					return false;

				CClient *pClient = ppChar->GetClient();
				if ( pClient == NULL )
					return false;

				pPage->SetGMHandler(pClient);
			}
			else if ( !strcmpi(pszKey, "DELETE") )
			{
				delete pPage;
			}
			else if ( pPage->FindGMHandler() )
			{
				CClient* pClient = pChar->GetClient();
				if ( pClient != NULL && pClient->GetChar() != NULL )
					pClient->Cmd_GM_PageCmd(pszKey);
			}
			else
			{
				return false;
			}

			return true;
		}
		return false;
	}
	else if ( ( !strnicmp(pszKey, "GUILD", 5) ) || ( !strnicmp(pszKey, "TOWN", 4) ) )
	{
		bool bIsGuild = !strnicmp(pszKey, "GUILD", 5);
		pszKey += bIsGuild ? 5 : 4;
		if ( *pszKey == '.' )
		{
			pszKey += 1;
			CItemStone *pMyGuild = pChar->Guild_Find(bIsGuild ? MEMORY_GUILD : MEMORY_TOWN);
			if ( pMyGuild ) return pMyGuild->r_SetVal(pszKey, s.GetArgRaw());
		}
		return false;
	}

	switch ( FindTableHeadSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case CPC_CURFOLLOWER:
			{
				m_curFollower = s.GetArgVal();
				pChar->UpdateStatsFlag();
			} return( true );
		case CPC_DEATHS:
			m_wDeaths = s.GetArgVal();
			return true;
		case CPC_DSPEECH:
			return( m_Speech.r_LoadVal( s, RES_SPEECH ));
		case CPC_KILLS:
			m_wMurders = s.GetArgVal();
			return true;
		case CPC_KRTOOLBARSTATUS:
			m_bKrToolbarEnabled = ( s.GetArgVal() != 0 );
			if ( pChar->GetClient() != NULL )
				pChar->GetClient()->addKRToolbar( m_bKrToolbarEnabled );
			return true;
		case CPC_LASTUSED:
			m_timeLastUsed = CServTime::GetCurrentTime() - ( s.GetArgVal() * TICK_PER_SEC );
			return( true );
		case CPC_LUCK:
			{
				m_luck = s.GetArgVal();
				pChar->UpdateStatsFlag();
			} return( true );
		case CPC_MAXFOLLOWER:
			{
				m_maxFollower = s.GetArgVal();
				pChar->UpdateStatsFlag();
			} return( true );
		case CPC_PFLAG:
			{
				m_pflag = s.GetArgVal();
			} return( true );
		case CPC_PROFILE:
			m_sProfile = Str_MakeFiltered( s.GetArgStr());
			return( true );
		case CPC_SKILLCLASS:
			return SetSkillClass( pChar, g_Cfg.ResourceGetIDType( RES_SKILLCLASS, s.GetArgStr()));
		case CPC_SKILLLOCK:
			{
				SKILL_TYPE skill = Skill_GetLockType( s.GetKey());
				if ( skill <= SKILL_NONE )
					return false;
				int bState = s.GetArgVal();
				if ( bState < SKILLLOCK_UP || bState > SKILLLOCK_LOCK )
					return false;
				Skill_SetLock(skill, static_cast<SKILLLOCK_TYPE>(bState));
			} return true;
		case CPC_SPEEDMODE:
			{
				m_speedMode = s.GetArgVal();
				pChar->UpdateSpeedMode();
			} return true;
		case CPC_STATLOCK:
			{
				STAT_TYPE stat = Stat_GetLockType( s.GetKey());
				if (( stat <= STAT_NONE ) || ( stat >= STAT_BASE_QTY ))
					return false;
				int bState = s.GetArgVal();
				if ( bState < SKILLLOCK_UP || bState > SKILLLOCK_LOCK )
					return false;
				Stat_SetLock(stat, static_cast<SKILLLOCK_TYPE>(bState));
			} return true;
		case CPC_TITHING:
			{
				m_iTithingPoints = s.GetArgVal();
				pChar->UpdateStatsFlag();
			} return( true );

		default:
			// Just ignore any NPC type stuff.
			if ( FindTableSorted( s.GetKey(), CCharNPC::sm_szLoadKeys, COUNTOF( CCharNPC::sm_szLoadKeys )-1 ) >= 0 )
			{
				return( true );
			}
			return( false );
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

void CCharPlayer::r_WriteChar( CChar * pChar, CScript & s ) 
{
	ADDTOCALLSTACK("CCharPlayer::r_WriteChar");
	UNREFERENCED_PARAMETER(pChar);
	EXC_TRY("r_WriteChar");

	s.WriteKey("ACCOUNT", GetAccount()->GetName());

	if ( m_curFollower )
		s.WriteKeyVal( "CURFOLLOWER", m_curFollower );
	if ( m_wDeaths )
		s.WriteKeyVal( "DEATHS", m_wDeaths );
	if ( m_wMurders )
		s.WriteKeyVal( "KILLS", m_wMurders );
	if ( GetSkillClass()->GetResourceID().GetResIndex() )
		s.WriteKey( "SKILLCLASS", GetSkillClass()->GetResourceName());
	if ( m_luck )
		s.WriteKeyVal("LUCK", m_luck);
	if ( m_maxFollower )
		s.WriteKeyVal( "MAXFOLLOWER", m_maxFollower );
	if ( m_pflag )
		s.WriteKeyVal( "PFLAG", m_pflag );
	if ( m_speedMode )
		s.WriteKeyVal("SPEEDMODE", m_speedMode);
	if ( m_iTithingPoints )
		s.WriteKeyVal("TITHING", m_iTithingPoints);
	if (( GetAccount()->GetResDisp() >= RDS_KR ) && m_bKrToolbarEnabled )
		s.WriteKeyVal("KRTOOLBARSTATUS", m_bKrToolbarEnabled);

	EXC_SET("saving dynamic speech");
	if ( m_Speech.GetCount() > 0 )
	{
		CGString sVal;
		m_Speech.WriteResourceRefList( sVal );
		s.WriteKey("DSPEECH", sVal);
	}

	EXC_SET("saving profile");
	if ( ! m_sProfile.IsEmpty())
	{
		TCHAR szLine[SCRIPT_MAX_LINE_LEN-16];
		Str_MakeUnFiltered( szLine, m_sProfile, sizeof(szLine));
		s.WriteKey( "PROFILE", szLine );
	}

	EXC_SET("saving stats locks");
	for ( int x = 0; x < STAT_BASE_QTY; x++)	// Don't write all lock states!
	{
		if ( ! m_StatLock[x] )
			continue;
		TCHAR szTemp[128];
		sprintf( szTemp, "StatLock[%d]", x );	// smaller storage space.
		s.WriteKeyVal( szTemp, m_StatLock[x] );
	}

	EXC_SET("saving skill locks");
	for ( size_t j = 0; j < g_Cfg.m_iMaxSkill; j++)	// Don't write all lock states!
	{
		ASSERT(j < COUNTOF(m_SkillLock));
		if ( ! m_SkillLock[j] )
			continue;
		TCHAR szTemp[128];
		sprintf( szTemp, "SkillLock[%" FMTSIZE_T "]", j );	// smaller storage space.
		s.WriteKeyVal( szTemp, m_SkillLock[j] );
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_DEBUG_END;
}

enum CPV_TYPE	// Player char.
{
	#define ADD(a,b) CPV_##a,
	#include "../tables/CCharPlayer_functions.tbl"
	#undef ADD
	CPV_QTY
};

LPCTSTR const CCharPlayer::sm_szVerbKeys[CPV_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CCharPlayer_functions.tbl"
	#undef ADD
	NULL
};

bool CChar::Player_OnVerb( CScript &s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CChar::Player_OnVerb");
	if ( !m_pPlayer )
		return false;

	LPCTSTR pszKey = s.GetKey();
	int cpVerb = FindTableSorted( pszKey, CCharPlayer::sm_szVerbKeys, COUNTOF(CCharPlayer::sm_szVerbKeys)-1 );

	if ( cpVerb <= -1 )
	{
		if ( ( !strnicmp(pszKey, "GUILD", 5) ) || ( !strnicmp(pszKey, "TOWN", 4) ) )
		{
			bool bIsGuild = !strnicmp(pszKey, "GUILD", 5);
			pszKey += bIsGuild ? 5 : 4;
			if ( *pszKey == '.' )
			{
				pszKey += 1;
				CItemStone *pMyGuild = Guild_Find(bIsGuild ? MEMORY_GUILD : MEMORY_TOWN);
                if ( pMyGuild )
                {
                        CScript sToParse(pszKey, s.GetArgRaw());
                        return pMyGuild->r_Verb(sToParse, pSrc);
                }
			}
			return false;
		}
	}

	switch ( cpVerb )
	{
		case CPV_KICK: // "KICK" = kick and block the account
			return (IsClient() && GetClient()->addKick(pSrc));

		case CPV_PASSWORD:	// "PASSWORD"
		{
			// Set/Clear the password
			if ( pSrc != this )
			{
				if ( pSrc->GetPrivLevel() <= GetPrivLevel() || pSrc->GetPrivLevel() < PLEVEL_Admin )
				{
					pSrc->SysMessage("You are not privileged to do this.");
					return false;
				}
			}

			CAccount * pAccount = m_pPlayer->GetAccount();
			ASSERT(pAccount != NULL);

			if ( !s.HasArgs() )
			{
				pAccount->ClearPassword();
				SysMessage("Password has been cleared.");
				SysMessage("Log out, then back in to set the new password.");
				g_Log.Event(LOGM_ACCOUNTS|LOGL_EVENT, "Account '%s', password cleared", pAccount->GetName());
			}
			else
			{
				if ( pAccount->SetPassword(s.GetArgStr()) )
				{
					SysMessage("Password has been set.");
					g_Log.Event(LOGM_ACCOUNTS|LOGL_EVENT, "Account '%s', password set to '%s'\n", pAccount->GetName(), s.GetArgStr());
					return true;
				}

				SysMessage("Invalid password.");
			}
			break;
		}

		default:
			return false;
	}

	return true;
}

//////////////////////////
// -CCharNPC

CCharNPC::CCharNPC( CChar * pChar, NPCBRAIN_TYPE NPCBrain )
{
	UNREFERENCED_PARAMETER(pChar);
	m_Brain = NPCBrain;
	m_Home_Dist_Wander = SHRT_MAX;	// as far as i want.
	m_Act_Motivation = 0;
	m_SpeechHue = HUE_TEXT_DEF;
#ifndef _WIN32
	for (int i_tmpN=0;i_tmpN < MAX_NPC_PATH_STORAGE_SIZE;i_tmpN++)
	{
		m_nextX[i_tmpN] = 0;
		m_nextY[i_tmpN] = 0;
	}
#else
	memset(m_nextX, 0, sizeof(m_nextX));
	memset(m_nextY, 0, sizeof(m_nextY));
#endif
	m_timeRestock.Init();
}

CCharNPC::~CCharNPC()
{
}

bool CCharNPC::r_LoadVal( CChar * pChar, CScript &s )
{
	EXC_TRY("LoadVal");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	case CNC_ACTPRI:
		m_Act_Motivation = s.GetArgVal();
		break;
	case CNC_NPC:
		m_Brain = static_cast<NPCBRAIN_TYPE>(s.GetArgVal());
		break;
	case CNC_HOMEDIST:
		if ( ! pChar->m_ptHome.IsValidPoint())
		{
			pChar->m_ptHome = pChar->GetTopPoint();
		}
		m_Home_Dist_Wander = s.GetArgVal();
		break;
	case CNC_NEED:
	case CNC_NEEDNAME:
		{
			TCHAR * pTmp = s.GetArgRaw();
			m_Need.Load(pTmp);
		}
		break;
	case CNC_SPEECH:
		return( m_Speech.r_LoadVal( s, RES_SPEECH ));
	case CNC_SPEECHCOLOR:
		m_SpeechHue = s.GetArgVal();
		break;

	case CNC_VENDCAP:
		{
			CItemContainer * pBank = pChar->GetBank();
			if ( pBank )
				pBank->m_itEqBankBox.m_Check_Restock = s.GetArgVal();
		}
		break;
	case CNC_VENDGOLD:
		{
			CItemContainer * pBank = pChar->GetBank();
			if ( pBank )
				pBank->m_itEqBankBox.m_Check_Amount = s.GetArgVal();
		}
		break;

	default:
		// Just ignore any player type stuff.
		if ( FindTableHeadSorted( s.GetKey(), CCharPlayer::sm_szLoadKeys, COUNTOF( CCharPlayer::sm_szLoadKeys )-1 ) >= 0 )
			return( true );
		return(false );
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CCharNPC::r_WriteVal( CChar * pChar, LPCTSTR pszKey, CGString & sVal )
{
	EXC_TRY("WriteVal");
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	case CNC_ACTPRI:
		sVal.FormatVal( m_Act_Motivation );
		break;
	case CNC_NPC:
		sVal.FormatVal( m_Brain );
		break;
	case CNC_HOMEDIST:
		sVal.FormatVal( m_Home_Dist_Wander );
		break;
	case CNC_NEED:
		{
			TCHAR *pszTmp = Str_GetTemp();
			m_Need.WriteKey( pszTmp );
			sVal = pszTmp;
		}
		break;
	case CNC_NEEDNAME:
		{
			TCHAR *pszTmp = Str_GetTemp();
			m_Need.WriteNameSingle( pszTmp );
			sVal = pszTmp;
		}
		break;
	case CNC_SPEECH:
		m_Speech.WriteResourceRefList( sVal );
		break;
	case CNC_SPEECHCOLOR:
		sVal.FormatVal( m_SpeechHue );
		break;

	case CNC_VENDCAP:
		{
			CItemContainer * pBank = pChar->GetBank();
			if ( pBank )
				sVal.FormatVal( pBank->m_itEqBankBox.m_Check_Restock );
		}
		break;
	case CNC_VENDGOLD:
		{
			CItemContainer * pBank = pChar->GetBank();
			if ( pBank )
				sVal.FormatVal( pBank->m_itEqBankBox.m_Check_Amount );
		}
		break;
	default:
		if ( FindTableHeadSorted( pszKey, CCharPlayer::sm_szLoadKeys, COUNTOF( CCharPlayer::sm_szLoadKeys )-1 ) >= 0 )
		{
			sVal = "0";
			return( true );
		}
		if ( FindTableSorted( pszKey, CClient::sm_szLoadKeys, CC_QTY ) >= 0 )
		{
			sVal = "0";
			return( true );
		}
		return(false );
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pChar);
	EXC_DEBUG_END;
	return false;
}

void CCharNPC::r_WriteChar( CChar * pChar, CScript & s )
{
	UNREFERENCED_PARAMETER(pChar);

	// This says we are an NPC.
	s.WriteKeyVal("NPC", m_Brain );

	if ( m_Home_Dist_Wander < SHRT_MAX )
		s.WriteKeyVal( "HOMEDIST", m_Home_Dist_Wander );
	if ( m_Act_Motivation )
		s.WriteKeyHex( "ACTPRI", m_Act_Motivation );

	m_Speech.r_Write( s, "SPEECH" );

	if ( m_SpeechHue != HUE_TEXT_DEF )
	{
		s.WriteKeyVal( "SPEECHCOLOR", m_SpeechHue );
	}

	if ( m_Need.GetResourceID().IsValidUID())
	{
		TemporaryString pszTmp;
		m_Need.WriteKey( pszTmp );
		s.WriteKey( "NEED", pszTmp );
	}
}
