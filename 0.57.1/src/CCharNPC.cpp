#include "graysvr.h"
#include "CClient.h"
#include "quest.h"
#include "character/SkillClass.h"
#include "character/Profession.h"

enum CPC_TYPE
{
	#define ADD(a,b) CPC_##a,
	#include "tables/CCharPlayer_props.tbl"
	#undef ADD
	CPC_QTY,
};

LPCTSTR const CCharPlayer::sm_szLoadKeys[CPC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "tables/CCharPlayer_props.tbl"
	#undef ADD
	NULL,
};

enum CNC_TYPE
{
	#define ADD(a,b) CNC_##a,
	#include "tables/CCharNpc_props.tbl"
	#undef ADD
	CNC_QTY,
};

LPCTSTR const CCharNPC::sm_szLoadKeys[CNC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "tables/CCharNpc_props.tbl"
	#undef ADD
	NULL,
};

void CChar::ClearNPC()
{
	delete m_pNPC;
	m_pNPC = NULL;
}

void CChar::ClearPlayer()
{
	if ( m_pPlayer != NULL )
	{
		m_pPlayer->GetAccount()->DetachChar(this);
	}
	delete m_pPlayer;
	m_pPlayer = NULL;
}

bool CChar::SetPlayerAccount(CAccount *pAccount)
{
	// Set up the char as a Player.
	if ( !pAccount )
		return false;

	if ( m_pPlayer )
	{
		if ( m_pPlayer->GetAccount() == pAccount )
			return true;

		g_Log.Error("SetPlayerAccount '%s' already set '%s' != '%s'!\n", GetName(), m_pPlayer->GetAccount()->GetName(), pAccount->GetName());
		return false;
	}

	if ( m_pNPC )
	{
		// This could happen if the account is set manually through
		// scripts. (serv.newnpc=x -> new.account=y)
		delete m_pNPC;
		m_pNPC = NULL;
	}

	m_pPlayer = new CCharPlayer(this, pAccount);
	pAccount->AttachChar(this);
	return true;
}



bool CChar::SetPlayerAccount( LPCTSTR pszAccName )
{
	CAccountRef pAccount = g_Accounts.Account_FindCreate(pszAccName);
	if ( pAccount == NULL )
	{
		g_Log.Error("SetPlayerAccount '%s' can't find '%s'!\n", GetName(), pszAccName);
		return false;
	}
	return( SetPlayerAccount( pAccount ));
}



bool CChar::SetNPCBrain( NPCBRAIN_TYPE NPCBrain )
{
	// Set up the char as an NPC
	if ( NPCBrain == NPCBRAIN_NONE || IsClient())
	{
		g_Log.Error("SetNPCBrain NULL\n");
		return false;
	}
	if ( m_pPlayer != NULL )
	{
		g_Log.Error("SetNPCBrain to Player '%s'\n", m_pPlayer->GetAccount());
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
	return true;
}

//////////////////////////
// -CCharPlayer

CCharPlayer::CCharPlayer(CChar *pChar, CAccount *pAccount) : m_pAccount(pAccount)
{
	m_wDeaths = m_wMurders = 0;
	m_pts = 0;
	m_curFollower = m_maxFollower = 0;
	m_resFire = m_resCold = m_resPoison = m_resEnergy = 0;
	m_luck = m_iTithingPoints = 0;
	m_speedMode = 0;
	memset(m_SkillLock, 0, sizeof(m_SkillLock));
	memset(m_StatLock, 0, sizeof(m_StatLock));
	SetSkillClass(pChar, RESOURCE_ID(RES_SKILLCLASS));
	SetProfession(pChar, RESOURCE_ID(RES_PROFESSION));
}

CCharPlayer::~CCharPlayer()
{
}

bool CCharPlayer::SetSkillClass( CChar * pChar, RESOURCE_ID rid )
{
	CResourceDef *pDef = g_Cfg.ResourceGetDef(rid);
	if ( !pDef ) return false;

	SkillClass* pLink = static_cast <SkillClass*>(pDef);
	if ( !pLink ) return false;

	if ( pLink != GetSkillClass() )
	{
		// remove any previous skill class from the events
		int i = pChar->m_OEvents.FindResourceType(RES_SKILLCLASS);
		if ( i >= 0 )
		{
			pChar->m_OEvents.RemoveAt(i);
		}

		m_SkillClass.SetRef(pLink);
		pChar->m_OEvents.Add(pLink);
	}
	return true;
}

SkillClass *CCharPlayer::GetSkillClass() const
{
	CResourceLink *link = m_SkillClass.GetRef();
	return static_cast <SkillClass*>(link);
}

bool CCharPlayer::SetProfession(CChar *pChar, RESOURCE_ID rid)
{
	CResourceDef *pDef = g_Cfg.ResourceGetDef(rid);
	if ( !pDef ) return false;

	Profession *pLink = static_cast <Profession*>(pDef);
	if ( !pLink ) return false;

	if ( pLink != GetProfession() )
	{
		// remove any previous professions from events
		int i = pChar->m_OEvents.FindResourceType(RES_PROFESSION);
		if ( i >= 0 )
		{
			pChar->m_OEvents.RemoveAt(i);
		}

		m_Profession.SetRef(pLink);
		pChar->m_OEvents.Add(pLink);
	}
	return true;
}

Profession *CCharPlayer::GetProfession() const
{
	CResourceLink *link = m_Profession.GetRef();
	return static_cast <Profession*>(link);
}

SKILL_TYPE CCharPlayer::Skill_GetLockType( LPCTSTR pszKey ) const
{
	// only players can have skill locks.

	TCHAR szTmpKey[128];
	strcpylen( szTmpKey, pszKey, sizeof(szTmpKey) );

	TCHAR * ppArgs[3];
	int i = Str_ParseCmds( szTmpKey, ppArgs, COUNTOF(ppArgs), ".[]" );
	if ( i <= 1 )
		return SKILL_NONE;

	if ( isdigit( ppArgs[1][0] ))
	{
		i = ATOI( ppArgs[1] );
	}
	else
	{
		i = g_Cfg.FindSkillKey( ppArgs[1] );
	}
	if ( i >= MAX_SKILL )
		return SKILL_NONE;
	return( (SKILL_TYPE) i );
}

STAT_TYPE CCharPlayer::Stat_GetLockType( LPCTSTR pszKey ) const
{
	// only players can have skill locks.

	TCHAR szTmpKey[128];
	strcpylen( szTmpKey, pszKey, sizeof(szTmpKey) );

	TCHAR * ppArgs[3];
	int i = Str_ParseCmds( szTmpKey, ppArgs, COUNTOF(ppArgs), ".[]" );
	if ( i <= 1 )
		return( STAT_NONE );

	if ( isdigit( ppArgs[1][0] ))
	{
		i = ATOI( ppArgs[1] );
	}
	else
	{
		i = g_Cfg.FindStatKey( ppArgs[1] );
	}
	if ( i >= STAT_BASE_QTY )
		return( STAT_NONE );
	return( (STAT_TYPE) i );
}

bool CCharPlayer::r_WriteVal( CChar * pChar, LPCTSTR pszKey, CGString & sVal )
{
	EXC_TRY("WriteVal");

	if ( !pChar || !GetAccount() )
		return false;

	if ( 0 ) ;
	else if ( !strnicmp(pszKey, "SKILLCLASS.", 11) )
	{
		return GetSkillClass()->r_WriteVal(pszKey + 11, sVal, pChar);
	}
	else if ( !strnicmp(pszKey, "PROFESSION.", 11) )
	{
		return GetProfession()->r_WriteVal(pszKey + 11, sVal, pChar);
	}
	else if ( !strnicmp(pszKey, "GUILD", 5) )
	{
		pszKey += 5;
		if ( *pszKey == 0 )
		{
			CItemStone *pMyGuild = pChar->Guild_Find(MEMORY_GUILD);
			if ( pMyGuild ) sVal.FormatVal(pMyGuild->uid());
			else sVal.FormatVal(0);
			return true;
		}
		else if ( *pszKey == '.' )
		{
			pszKey += 1;
			CItemStone *pMyGuild = pChar->Guild_Find(MEMORY_GUILD);
			if ( pMyGuild ) return pMyGuild->r_WriteVal(pszKey, sVal, pChar);
		}
		return false;
	}
	else if ( !strnicmp(pszKey, "QUEST.", 6) )
	{
		pszKey += 6;
		int id = Exp_GetVal(pszKey);
		SKIP_SEPARATORS(pszKey);
		if (( id >= 0 ) && ( id < m_quests.size() ))
		{
			return m_quests[id]->r_WriteVal(pszKey, sVal, pChar);
		}
		return false;
	}
	else if ( !strnicmp(pszKey, "QUESTHIST.", 10) )
	{
		pszKey += 10;
		int id = Exp_GetVal(pszKey);
		SKIP_SEPARATORS(pszKey);
		if (( id >= 0 ) && ( id < m_questsHistory.size() ))
		{
			return m_questsHistory[id]->r_WriteVal(pszKey, sVal, pChar);
		}
		return false;
	}

	switch ( FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case CPC_ACCOUNT:
			sVal = GetAccount()->GetName();
			return true;
		case CPC_CURFOLLOWER:
			sVal.FormatVal(m_curFollower);
			return( true );
		case CPC_DEATHS:
			sVal.FormatVal( m_wDeaths );
			return true;
		case CPC_KILLS:
			sVal.FormatVal( m_wMurders );
			return true;
		case CPC_LASTUSED:
			sVal.FormatVal( - g_World.GetTimeDiff( m_timeLastUsed ) / TICK_PER_SEC );
			return true;
		case CPC_LUCK:
			sVal.FormatVal(m_luck);
			break;
		case CPC_MAXFOLLOWER:
			sVal.FormatVal(m_maxFollower);
			return( true );
		case CPC_PROFILE:
			{
				TCHAR szLine[SCRIPT_MAX_LINE_LEN-16];
				Str_MakeUnFiltered( szLine, m_sProfile, sizeof(szLine));
				sVal = szLine;
			}
			return true;

		case CPC_PTS:
			sVal.FormatVal(m_pts);
			break;
	
		case CPC_QUESTHIST:
			sVal.FormatVal(m_questsHistory.size());
			break;
		case CPC_QUESTS:
			sVal.FormatVal(m_quests.size());
			break;
	
		case CPC_RESCOLD:
			sVal.FormatVal(m_resCold);
			break;
		case CPC_RESFIRE:
			sVal.FormatVal(m_resFire);
			break;
		case CPC_RESENERGY:
			sVal.FormatVal(m_resEnergy);
			break;
		case CPC_RESPOISON:
			sVal.FormatVal(m_resPoison);
			break;
	
		case CPC_SKILLCLASS:
			sVal = GetSkillClass()->GetResourceName();
			return true;
		
		case CPC_SKILLLOCK:
			{
				// "SkillLock[alchemy]"
				SKILL_TYPE skill = Skill_GetLockType( pszKey );
				if ( skill <= SKILL_NONE )
					return false;
				sVal.FormatVal( Skill_GetLock( skill ));
			}
			return true;
		case CPC_SPEEDMODE:
			sVal.FormatVal( m_speedMode );
			return( true );
		case CPC_STATLOCK:
			{
				// "StatLock[str]"
				STAT_TYPE stat = Stat_GetLockType( pszKey );
				if (( stat <= STAT_NONE ) || ( stat >= STAT_BASE_QTY ))
					return false;
				sVal.FormatVal( Stat_GetLock( stat ));
			} return true;
			
		case CPC_TITHING:
			sVal.FormatVal(m_iTithingPoints);
			return true;

		default:
			if ( FindTableSorted( pszKey, CCharNPC::sm_szLoadKeys, COUNTOF( CCharNPC::sm_szLoadKeys )-1 ) >= 0 )
			{
				sVal = "0";
				return true;
			}
			return false;
	}
	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pChar);
	EXC_DEBUG_END;
	return false;
}

bool CCharPlayer::r_LoadVal( CChar * pChar, CScript &s )
{
	EXC_TRY("LoadVal");
	LPCTSTR pszKey = s.GetKey();
	LPCTSTR pszArgs = s.GetArgStr();
	if ( !strnicmp(pszKey, "GMPAGE", 6) )		//	GM pages
	{
		CGMPage	*pPage;
		pszKey += 6;
		if ( *pszKey == '.' )						//	SERV.GMPAGE.*
		{
			SKIP_SEPARATORS(pszKey);
			int iQty = Exp_GetVal(pszKey);
			if (( iQty < 0 ) || ( iQty >= g_World.m_GMPages.GetCount() )) return false;
			SKIP_SEPARATORS(pszKey);
			pPage = static_cast <CGMPage*> (g_World.m_GMPages.GetAt(iQty));
			if ( !pPage ) return false;

			if ( !strnicmp(pszKey, "HANDLE", 6) )
			{
				CChar *ppChar = pChar;

				if ( *pszArgs ) ppChar = static_cast <CChar*> (g_World.FindUID(s.GetArgVal()));
				if ( !pChar ) return false;
				CClient *pClient = pChar->GetClient();
				if ( !pClient ) return false;

				pClient->m_pGMPage = pPage;
				pPage->SetGMHandler(pClient);
			}
			else if ( !strnicmp(pszKey, "DELETE", 6) )
			{
				pPage->RemoveSelf();
			}
			else if ( pPage->FindGMHandler() )
			{
				CClient	*pClient = pChar->GetClient();
				if ( pClient ) pClient->Cmd_GM_PageCmd(pszKey);
			}
			else return false;
			return true;
		}
		return false;
	}
	else if ( !strnicmp(pszKey, "QUEST.", 6) )
	{
		pszKey += 6;
		int id = Exp_GetVal(pszKey);
		SKIP_SEPARATORS(pszKey);
		if (( id >= 0 ) && ( id < m_quests.size() ))
		{
			CScript script(pszKey, pszArgs);
			return m_quests[id]->r_LoadVal(script);
		}
		return false;
	}
	else if ( !strnicmp(pszKey, "QUESTHIST.", 10) )
	{
		pszKey += 10;
		int id = Exp_GetVal(pszKey);
		SKIP_SEPARATORS(pszKey);
		if (( id >= 0 ) && ( id < m_questsHistory.size() ))
		{
			pszKey++;
			CScript script(pszKey, pszArgs);
			return m_questsHistory[id]->r_LoadVal(script);
		}
		return false;
	}
	//	load-time save restoring quest states
	else if ( !strnicmp(pszKey, "QUEST", 5) )	// QUESTx=var1,var2,a:b:c,a1:b1:c1,a2:b2:c2
	{
		pszKey += 5;
		int id = Exp_GetVal(pszKey);
		pszKey++;

		if (( id < 0 ) || ( id >= g_Quests.size() ))
		{
			g_Log.Error("Quest #%d is undefined [0..%d] is allowed.\n", id, g_Quests.size());
			return false;
		}
		Quest *quest = new Quest(g_Quests[id], pChar);
		quest->m_failed = Exp_GetVal(pszKey);
		SKIP_ARGSEP(pszKey);
		quest->m_timeleft = Exp_GetVal(pszKey);
		SKIP_ARGSEP(pszKey);

		while ( *pszKey )
		{
			QuestDef::ResourceTyped *resource = new QuestDef::ResourceTyped();
			pszKey = resource->Load(pszKey);
			quest->m_objectives.push_back(resource);
			SKIP_ARGSEP(pszKey);
		}

		if ( quest->m_objectives.size() )
			m_quests.push_back(quest);
		else
			m_questsHistory.push_back(quest);
		return true;
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
		case CPC_KILLS:
			m_wMurders = s.GetArgVal();
			return true;
		case CPC_LASTUSED:
			m_timeLastUsed = CServTime::GetCurrentTime() - ( s.GetArgVal() * TICK_PER_SEC );
			return true;
		case CPC_LUCK:
			m_luck = s.GetArgVal();
			break;
		case CPC_MAXFOLLOWER:
			{
				m_maxFollower = s.GetArgVal();
				pChar->UpdateStatsFlag();
			} return( true );

		case CPC_PROFESSION:
			return SetProfession(pChar, g_Cfg.ResourceGetIDType(RES_PROFESSION, s.GetArgStr()));

		case CPC_PROFILE:
			m_sProfile = Str_MakeFiltered( s.GetArgStr());
			return true;
	
		case CPC_PTS:
			m_pts = s.GetArgVal();
			break;
	
		case CPC_RESCOLD:
			m_resCold = s.GetArgVal();
			pChar->UpdateStatsFlag();
			break;
		case CPC_RESFIRE:
			m_resFire = s.GetArgVal();
			pChar->UpdateStatsFlag();
			break;
		case CPC_RESENERGY:
			m_resEnergy = s.GetArgVal();
			pChar->UpdateStatsFlag();
			break;
		case CPC_RESPOISON:
			m_resPoison = s.GetArgVal();
			pChar->UpdateStatsFlag();
			break;
	
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
				Skill_SetLock(skill, (SKILLLOCK_TYPE)bState);
			}
			return true;
		case CPC_SPEEDMODE:
			{
				m_speedMode = s.GetArgVal();
				pChar->UpdateSpeedMode();
			}
			return true;
		case CPC_STATLOCK:
			{
				STAT_TYPE stat = Stat_GetLockType( s.GetKey());
				if (( stat <= STAT_NONE ) || ( stat >= STAT_BASE_QTY ))
					return false;
				int bState = s.GetArgVal();
				if ( bState < SKILLLOCK_UP || bState > SKILLLOCK_LOCK )
					return false;
				Stat_SetLock(stat, (SKILLLOCK_TYPE)bState);
			} return true;
		case CPC_TITHING:
			m_iTithingPoints = s.GetArgVal();
			break;
			
		default:
			// Just ignore any NPC type stuff.
			if ( FindTableSorted(s.GetKey(), CCharNPC::sm_szLoadKeys, COUNTOF(CCharNPC::sm_szLoadKeys)-1) == -1 )
				return false;
	}
	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

void CCharPlayer::r_WriteChar( CChar * pChar, CScript & s ) 
{
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
	if ( GetProfession()->GetResourceID().GetResIndex() )
		s.WriteKey("PROFESSION", GetProfession()->GetResourceName());
	if ( m_luck )
		s.WriteKeyVal("LUCK", m_luck);
	if ( m_maxFollower )
		s.WriteKeyVal( "MAXFOLLOWER", m_maxFollower );
	if ( m_resCold )
		s.WriteKeyVal("RESCOLD", m_resCold);
	if ( m_resEnergy )
		s.WriteKeyVal("RESENERGY", m_resEnergy);
	if ( m_resFire )
		s.WriteKeyVal("RESFIRE", m_resFire);
	if ( m_resPoison )
		s.WriteKeyVal("RESPOISON", m_resPoison);
	if ( m_pts )
		s.WriteKeyVal("PTS", m_pts);
	if ( m_speedMode )
		s.WriteKeyVal("SPEEDMODE", m_speedMode);
	if ( m_iTithingPoints )
		s.WriteKeyVal("TITHING", m_iTithingPoints);

	if ( ! m_sProfile.IsEmpty())
	{
		TCHAR szLine[SCRIPT_MAX_LINE_LEN-16];
		Str_MakeUnFiltered( szLine, m_sProfile, sizeof(szLine));
		s.WriteKey( "PROFILE", szLine );
	}

	for ( int x=0;x<STAT_BASE_QTY;x++)	// Don't write all lock states!
	{
		if ( ! m_StatLock[x] )
			continue;
		TCHAR szTemp[128];
		sprintf( szTemp, "StatLock[%d]", x );	// smaller storage space.
		s.WriteKeyVal( szTemp, m_StatLock[x] );
	}

	for ( int j=0;j<MAX_SKILL;j++)	// Don't write all lock states!
	{
		if ( ! m_SkillLock[j] )
			continue;
		TCHAR szTemp[128];
		sprintf( szTemp, "SkillLock[%d]", j );	// smaller storage space.
		s.WriteKeyVal( szTemp, m_SkillLock[j] );
	}

	//	quests
	vector<Quest *>::iterator	it;
	for ( it = m_quests.begin(); it != m_quests.end(); it++ )
	{
		(*it)->r_Write(s);
	}
	//	historical quests
	for ( it = m_questsHistory.begin(); it != m_questsHistory.end(); it++ )
	{
		(*it)->r_Write(s);
	}
	EXC_CATCH;
}

LPCTSTR const CCharPlayer::sm_szVerbKeys[] =
{
	"KICK",
	"PASSWORD",
	NULL,
};

bool CChar::Player_OnVerb( CScript &s, CTextConsole * pSrc ) // Execute command from script
{
	if ( !m_pPlayer )
		return false;

	switch ( FindTableSorted( s.GetKey(), m_pPlayer->sm_szVerbKeys, COUNTOF(m_pPlayer->sm_szVerbKeys)-1 ))
	{
		case 0: // "KICK" = kick and block the account
			return (IsClient() && GetClient()->addKick(pSrc));
		case 1:	// "PASSWORD"
			// Set/Clear the password
			if ( pSrc != this )
			{
				if ( pSrc->GetPrivLevel() <= GetPrivLevel() || pSrc->GetPrivLevel() < PLEVEL_Admin )
				{
					pSrc->SysMessage("You are not privileged to do this.");
					return false;
				}
			}
			if ( !s.HasArgs() )
			{
				m_pPlayer->GetAccount()->m_sCurPassword.Empty();
				SysMessage("Password has been cleared.");
				SysMessage("Log out, then back in to set the new password.");
				g_Log.Event(LOGM_ACCOUNTS|LOGL_EVENT, "Account '%s', password cleared", m_pPlayer->GetAccount()->GetName());
			}
			else
			{
				m_pPlayer->GetAccount()->SetPassword(s.GetArgStr());
				SysMessage("Password has been set");
				g_Log.Event(LOGM_ACCOUNTS|LOGL_EVENT, "Account '%s', password set to '%s'", m_pPlayer->GetAccount()->GetName(), s.GetArgStr());
			}
			break;

		default:
			return false;
	}

	return true;
}

//////////////////////////
// -CCharNPC

CCharNPC::CCharNPC( CChar * pChar, NPCBRAIN_TYPE NPCBrain )
{
	m_Brain = NPCBrain;
	m_Home_Dist_Wander = SHRT_MAX;	// as far as i want.
	m_Act_Motivation = 0;
	m_SpeechHue = HUE_TEXT_DEF;
	memset(m_nextX, 0, sizeof(m_nextX));
	memset(m_nextY, 0, sizeof(m_nextY));
	m_timeRestock.Init();
	m_renamed = false;
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
		m_Brain = (NPCBRAIN_TYPE) s.GetArgVal();
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
		if ( FindTableHeadSorted( s.GetKey(), CCharPlayer::sm_szLoadKeys, COUNTOF(CCharPlayer::sm_szLoadKeys)-1 ) >= 0 )
			return true;
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
			TEMPSTRING(pszTmp);
			m_Need.WriteKey( pszTmp );
			sVal = pszTmp;
		}
		break;
	case CNC_NEEDNAME:
		{
			TEMPSTRING(pszTmp);
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
		if ( FindTableHeadSorted( pszKey, CCharPlayer::sm_szLoadKeys, COUNTOF(CCharPlayer::sm_szLoadKeys)-1 ) >= 0 )
		{
			sVal = "0";
			return true;
		}
		if ( FindTableSorted( pszKey, CClient::sm_szLoadKeys, CC_QTY ) >= 0 )
		{
			sVal = "0";
			return true;
		}
		return(false );
	}
	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pChar->GetUID());
	EXC_DEBUG_END;
	return false;
}

void CCharNPC::r_WriteChar( CChar * pChar, CScript & s )
{
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
		TEMPSTRING(pszTmp);
		m_Need.WriteKey( pszTmp );
		s.WriteKey( "NEED", pszTmp );
	}
}
