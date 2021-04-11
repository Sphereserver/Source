#include "graysvr.h"	// predef header

///////////////////////////////////////////////////////////
// CChar

void CChar::ClearNPC()
{
	ADDTOCALLSTACK("CChar::ClearNPC");
	if ( !m_pNPC )
		return;

	delete m_pNPC;
	m_pNPC = NULL;
}

void CChar::ClearPlayer()
{
	ADDTOCALLSTACK("CChar::ClearPlayer");
	if ( !m_pPlayer )
		return;

	m_pPlayer->m_pAccount->DetachChar(this);
	delete m_pPlayer;
	m_pPlayer = NULL;
}

bool CChar::SetPlayerAccount(CAccount *pAccount)
{
	ADDTOCALLSTACK("CChar::SetPlayerAccount");
	if ( !pAccount )
		return false;

	if ( m_pPlayer )
	{
		if ( m_pPlayer->m_pAccount == pAccount )
			return true;

		if ( m_pPlayer->m_pAccount )
			m_pPlayer->m_pAccount->DetachChar(this);
	}

	if ( m_pNPC )
		ClearNPC();

	m_pPlayer = new CCharPlayer(this, pAccount);
	pAccount->AttachChar(this);
	return true;
}

bool CChar::SetPlayerAccount(LPCTSTR pszAccName)
{
	ADDTOCALLSTACK("CChar::SetPlayerAccount");
	CAccount *pAccount = g_Accounts.Account_Find(pszAccName);
	if ( !pAccount )
	{
		DEBUG_ERR(("Can't attach char '%s' (UID=0%" FMTDWORDH ") to invalid account '%s'\n", GetName(), static_cast<DWORD>(GetUID()), pszAccName));
		return false;
	}
	return SetPlayerAccount(pAccount);
}

bool CChar::SetNPCBrain(NPCBRAIN_TYPE brain)
{
	ADDTOCALLSTACK("CChar::SetNPCBrain");
	if ( m_pPlayer )
		return false;

	if ( m_pNPC )
		m_pNPC->m_Brain = brain;
	else
		m_pNPC = new CCharNPC(this, brain);
	return true;
}

///////////////////////////////////////////////////////////
// CCharPlayer

CCharPlayer::CCharPlayer(CChar *pChar, CAccount *pAccount)
{
	m_pAccount = pAccount;
	m_timeLastUsed.Init();
	m_wMurders = 0;
	m_wDeaths = 0;
	m_speedMode = SPEEDMODE_DEFAULT;
	m_fRefuseTrades = false;
	m_fRefuseGlobalChatRequests = false;
	m_fKRToolbarEnabled = false;

	memset(m_SkillLock, 0, sizeof(m_SkillLock));
	memset(m_StatLock, 0, sizeof(m_StatLock));
	SetSkillClass(pChar, RESOURCE_ID(RES_SKILLCLASS));
}

CCharPlayer::~CCharPlayer()
{
	m_Speech.RemoveAll();
}

bool CCharPlayer::SetSkillClass(CChar *pChar, RESOURCE_ID rid)
{
	ADDTOCALLSTACK("CCharPlayer::SetSkillClass");
	CResourceDef *pDef = g_Cfg.ResourceGetDef(rid);
	if ( !pDef )
		return false;

	CSkillClassDef *pLink = static_cast<CSkillClassDef *>(pDef);
	if ( pLink == GetSkillClass() )
		return true;

	// Remove previous skillclass first
	size_t i = pChar->m_OEvents.FindResourceType(RES_SKILLCLASS);
	if ( i != pChar->m_OEvents.BadIndex() )
		pChar->m_OEvents.RemoveAt(i);

	// Set skillclass
	m_SkillClass.SetRef(pLink);
	pChar->m_OEvents.Add(pLink);
	return true;
}

CSkillClassDef *CCharPlayer::GetSkillClass() const
{
	ADDTOCALLSTACK("CCharPlayer::GetSkillClass");
	// NOTE: this should always return non-null
	CResourceLink *pLink = m_SkillClass.GetRef();
	return pLink ? static_cast<CSkillClassDef *>(pLink) : NULL;
}

SKILL_TYPE CCharPlayer::Skill_GetLockType(LPCTSTR pszKey) const
{
	ADDTOCALLSTACK("CCharPlayer::Skill_GetLockType");
	// Syntax accept 'SKILLLOCK[X]' and 'SKILLLOCK.X' (where X can be skill name or skill index)

	TCHAR pszArgs[40];
	strncpy(pszArgs, pszKey, sizeof(pszArgs));
	pszArgs[sizeof(pszArgs) - 1] = '\0';

	TCHAR *ppArgs[3];
	size_t iQty = Str_ParseCmds(pszArgs, ppArgs, COUNTOF(ppArgs), ".[]");
	if ( iQty <= 1 )
		return SKILL_NONE;

	iQty = IsDigit(ppArgs[1][0]) ? ATOI(ppArgs[1]) : g_Cfg.FindSkillKey(ppArgs[1]);
	SKILL_TYPE skill = static_cast<SKILL_TYPE>(iQty);
	return CChar::IsSkillBase(skill) ? skill : SKILL_NONE;
}

STAT_TYPE CCharPlayer::Stat_GetLockType(LPCTSTR pszKey) const
{
	ADDTOCALLSTACK("CCharPlayer::Stat_GetLockType");
	// Syntax accept 'STATLOCK[X]' and 'STATLOCK.X' (where X can be stat name or stat index)

	TCHAR pszArgs[40];
	strncpy(pszArgs, pszKey, sizeof(pszArgs));
	pszArgs[sizeof(pszArgs) - 1] = '\0';

	TCHAR *ppArgs[3];
	size_t iQty = Str_ParseCmds(pszArgs, ppArgs, COUNTOF(ppArgs), ".[]");
	if ( iQty <= 1 )
		return STAT_NONE;

	iQty = IsDigit(ppArgs[1][0]) ? ATOI(ppArgs[1]) : g_Cfg.FindStatKey(ppArgs[1]);
	STAT_TYPE stat = static_cast<STAT_TYPE>(iQty);
	return ((stat > STAT_NONE) && (stat < STAT_BASE_QTY)) ? stat : STAT_NONE;
}

enum CPC_TYPE
{
	#define ADD(a,b) CPC_##a,
	#include "../tables/CCharPlayer_props.tbl"
	#undef ADD
	CPC_QTY
};

const LPCTSTR CCharPlayer::sm_szLoadKeys[CPC_QTY + 1] =
{
	#define ADD(a,b) b,
	#include "../tables/CCharPlayer_props.tbl"
	#undef ADD
	NULL
};

enum CNC_TYPE
{
	#define ADD(a,b) CNC_##a,
	#include "../tables/CCharNpc_props.tbl"
	#undef ADD
	CNC_QTY
};

const LPCTSTR CCharNPC::sm_szLoadKeys[CNC_QTY + 1] =
{
	#define ADD(a,b) b,
	#include "../tables/CCharNpc_props.tbl"
	#undef ADD
	NULL
};

bool CCharPlayer::r_WriteVal(CChar *pChar, LPCTSTR pszKey, CGString &sVal)
{
	ADDTOCALLSTACK("CCharPlayer::r_WriteVal");
	EXC_TRY("WriteVal");

	if ( !pChar || !m_pAccount )
		return false;

	if ( !strnicmp(pszKey, "SKILLCLASS.", 11) )
		return GetSkillClass()->r_WriteVal(pszKey + 11, sVal, pChar);

	if ( !strnicmp(pszKey, "GUILD", 5) || !strnicmp(pszKey, "TOWN", 4) )
	{
		bool fGuild = !strnicmp(pszKey, "GUILD", 5);
		pszKey += fGuild ? 5 : 4;
		CItemStone *pStone = pChar->Guild_Find(fGuild ? MEMORY_GUILD : MEMORY_TOWN);
		if ( *pszKey == '\0' )
		{
			if ( pStone )
				sVal.FormatHex(static_cast<DWORD>(pStone->GetUID()));
			else
				sVal = "0";
			return true;
		}
		else if ( *pszKey == '.' )
		{
			++pszKey;
			if ( pStone )
				return pStone->r_WriteVal(pszKey, sVal, pChar);
		}
		return false;
	}

	switch ( FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case CPC_ACCOUNT:
			sVal = m_pAccount->GetName();
			return true;
		case CPC_DEATHS:
			sVal.FormatUVal(m_wDeaths);
			return true;
		case CPC_DSPEECH:
			m_Speech.WriteResourceRefList(sVal);
			return true;
		case CPC_ISDSPEECH:
			if ( pszKey[9] != '.' )
				return false;
			pszKey += 10;
			sVal.FormatVal(m_Speech.ContainsResourceName(RES_SPEECH, pszKey));
			return true;
		case CPC_KILLS:
			sVal.FormatUVal(m_wMurders);
			return true;
		case CPC_KRTOOLBARSTATUS:
			sVal.FormatVal(m_fKRToolbarEnabled);
			return true;
		case CPC_LASTUSED:
			sVal.FormatLLVal(-g_World.GetTimeDiff(m_timeLastUsed) / TICK_PER_SEC);
			return true;
		case CPC_PROFILE:
		{
			TCHAR szProfile[SCRIPT_MAX_LINE_LEN - 16];
			Str_MakeUnFiltered(szProfile, m_sProfile, sizeof(szProfile));
			sVal = szProfile;
			return true;
		}
		case CPC_REFUSEGLOBALCHATREQUESTS:
			sVal.FormatVal(m_fRefuseGlobalChatRequests);
			return true;
		case CPC_REFUSETRADES:
			sVal.FormatVal(m_fRefuseTrades);
			return true;
		case CPC_SKILLCLASS:
			sVal = GetSkillClass()->GetResourceName();
			return true;
		case CPC_SKILLLOCK:
			sVal.FormatVal(Skill_GetLock(Skill_GetLockType(pszKey)));
			return true;
		case CPC_SPEEDMODE:
			sVal.FormatVal(m_speedMode);
			return true;
		case CPC_STATLOCK:
			sVal.FormatVal(Stat_GetLock(Stat_GetLockType(pszKey)));
			return true;
		default:
			if ( FindTableSorted(pszKey, CCharNPC::sm_szLoadKeys, COUNTOF(CCharNPC::sm_szLoadKeys) - 1) >= 0 )
			{
				sVal = "0";
				return true;
			}
			return false;
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pChar);
	EXC_DEBUG_END;
	return false;
}

bool CCharPlayer::r_LoadVal(CChar *pChar, CScript &s)
{
	ADDTOCALLSTACK("CCharPlayer::r_LoadVal");
	EXC_TRY("LoadVal");

	LPCTSTR pszKey = s.GetKey();
	if ( !strnicmp(pszKey, "GUILD", 5) || !strnicmp(pszKey, "TOWN", 4) )
	{
		bool fGuild = !strnicmp(pszKey, "GUILD", 5);
		pszKey += fGuild ? 5 : 4;
		if ( *pszKey == '.' )
		{
			++pszKey;
			CItemStone *pStone = pChar->Guild_Find(fGuild ? MEMORY_GUILD : MEMORY_TOWN);
			if ( pStone )
				return pStone->r_SetVal(pszKey, s.GetArgRaw());
		}
		return false;
	}

	switch ( FindTableHeadSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case CPC_DEATHS:
			m_wDeaths = static_cast<WORD>(s.GetArgVal());
			return true;
		case CPC_DSPEECH:
			return m_Speech.r_LoadVal(s, RES_SPEECH);
		case CPC_KILLS:
			m_wMurders = static_cast<WORD>(s.GetArgVal());
			pChar->NotoSave_Update();
			return true;
		case CPC_KRTOOLBARSTATUS:
			m_fKRToolbarEnabled = (s.GetArgVal() > 0);
			if ( pChar->m_pClient )
				pChar->m_pClient->addKRToolbar(m_fKRToolbarEnabled);
			return true;
		case CPC_LASTUSED:
			m_timeLastUsed = CServTime::GetCurrentTime() - (s.GetArgLLVal() * TICK_PER_SEC);
			return true;
		case CPC_PROFILE:
			m_sProfile = Str_MakeFiltered(s.GetArgStr());
			return true;
		case CPC_REFUSEGLOBALCHATREQUESTS:
			m_fRefuseGlobalChatRequests = (s.GetArgVal() > 0);
			return true;
		case CPC_REFUSETRADES:
			m_fRefuseTrades = (s.GetArgVal() > 0);
			return true;
		case CPC_SKILLCLASS:
			return SetSkillClass(pChar, g_Cfg.ResourceGetIDType(RES_SKILLCLASS, s.GetArgStr()));
		case CPC_SKILLLOCK:
		{
			SKILL_TYPE skill = Skill_GetLockType(s.GetKey());
			SKILLLOCK_TYPE state = static_cast<SKILLLOCK_TYPE>(s.GetArgVal());
			if ( (state < SKILLLOCK_UP) || (state > SKILLLOCK_LOCK) )
				return false;

			Skill_SetLock(skill, state);
			if ( pChar->m_pClient )
				pChar->m_pClient->addSkillWindow(skill);
			return true;
		}
		case CPC_SPEEDMODE:
		{
			SPEEDMODE_TYPE mode = static_cast<SPEEDMODE_TYPE>(s.GetArgVal());
			if ( (mode < SPEEDMODE_DEFAULT) || (mode > SPEEDMODE_GMTELEPORT) || ((mode == SPEEDMODE_GMTELEPORT) && !pChar->IsPriv(PRIV_GM)) )
				return false;

			m_speedMode = mode;
			if ( pChar->m_pClient )
				pChar->m_pClient->addSpeedMode(m_speedMode);
			return true;
		}
		case CPC_STATLOCK:
		{
			STAT_TYPE stat = Stat_GetLockType(s.GetKey());
			SKILLLOCK_TYPE state = static_cast<SKILLLOCK_TYPE>(s.GetArgVal());
			if ( (state < SKILLLOCK_UP) || (state > SKILLLOCK_LOCK) )
				return false;

			Stat_SetLock(stat, state);
			if ( pChar->m_pClient )
				pChar->m_pClient->addHealthBarInfo(pChar);
			return true;
		}
		default:
			// Just ignore any NPC type stuff
			return (FindTableSorted(s.GetKey(), CCharNPC::sm_szLoadKeys, COUNTOF(CCharNPC::sm_szLoadKeys) - 1) >= 0) ? true : false;
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

void CCharPlayer::r_WriteChar(CChar *pChar, CScript &s)
{
	ADDTOCALLSTACK("CCharPlayer::r_WriteChar");
	UNREFERENCED_PARAMETER(pChar);
	EXC_TRY("r_WriteChar");

	s.WriteKey("ACCOUNT", m_pAccount->GetName());

	if ( m_wDeaths )
		s.WriteKeyVal("DEATHS", m_wDeaths);
	if ( m_wMurders )
		s.WriteKeyVal("KILLS", m_wMurders);
	if ( GetSkillClass()->GetResourceID().GetResIndex() )
		s.WriteKey("SKILLCLASS", GetSkillClass()->GetResourceName());
	if ( m_speedMode )
		s.WriteKeyVal("SPEEDMODE", m_speedMode);
	if ( m_fRefuseTrades )
		s.WriteKeyVal("REFUSETRADES", m_fRefuseTrades);
	if ( m_fRefuseGlobalChatRequests )
		s.WriteKeyVal("REFUSEGLOBALCHATREQUESTS", m_fRefuseGlobalChatRequests);
	if ( m_fKRToolbarEnabled )
		s.WriteKeyVal("KRTOOLBARSTATUS", m_fKRToolbarEnabled);

	EXC_SET("saving dynamic speech");
	if ( m_Speech.GetCount() > 0 )
	{
		CGString sVal;
		m_Speech.WriteResourceRefList(sVal);
		s.WriteKey("DSPEECH", sVal);
	}

	EXC_SET("saving profile");
	if ( !m_sProfile.IsEmpty() )
	{
		TCHAR szProfile[SCRIPT_MAX_LINE_LEN - 16];
		Str_MakeUnFiltered(szProfile, m_sProfile, sizeof(szProfile));
		s.WriteKey("PROFILE", szProfile);
	}

	EXC_SET("saving stats locks");
	size_t iMaxStat = COUNTOF(m_StatLock);
	for ( size_t i = 0; i < iMaxStat; ++i )
	{
		if ( !m_StatLock[i] )
			continue;

		TCHAR szTemp[15];
		snprintf(szTemp, sizeof(szTemp), "STATLOCK[%" FMTSIZE_T "]", i);
		s.WriteKeyVal(szTemp, m_StatLock[i]);
	}

	EXC_SET("saving skill locks");
	size_t iMaxSkill = COUNTOF(m_SkillLock);
	for ( size_t i = 0; i < iMaxSkill; ++i )
	{
		if ( !m_SkillLock[i] )
			continue;

		TCHAR szTemp[15];
		snprintf(szTemp, sizeof(szTemp), "SKILLLOCK[%" FMTSIZE_T "]", i);
		s.WriteKeyVal(szTemp, m_SkillLock[i]);
	}
	EXC_CATCH;
}

enum CPV_TYPE
{
	#define ADD(a,b) CPV_##a,
	#include "../tables/CCharPlayer_functions.tbl"
	#undef ADD
	CPV_QTY
};

const LPCTSTR CCharPlayer::sm_szVerbKeys[CPV_QTY + 1] =
{
	#define ADD(a,b) b,
	#include "../tables/CCharPlayer_functions.tbl"
	#undef ADD
	NULL
};

bool CChar::Player_OnVerb(CScript &s, CTextConsole *pSrc)	// execute command from script
{
	ADDTOCALLSTACK("CChar::Player_OnVerb");
	if ( !m_pPlayer || !pSrc )
		return false;

	LPCTSTR pszKey = s.GetKey();
	int index = FindTableSorted(pszKey, CCharPlayer::sm_szVerbKeys, COUNTOF(CCharPlayer::sm_szVerbKeys) - 1);
	if ( index < 0 )
	{
		if ( !strnicmp(pszKey, "GUILD", 5) || !strnicmp(pszKey, "TOWN", 4) )
		{
			bool fGuild = !strnicmp(pszKey, "GUILD", 5);
			pszKey += fGuild ? 5 : 4;
			if ( *pszKey == '.' )
			{
				++pszKey;
				CItemStone *pStone = Guild_Find(fGuild ? MEMORY_GUILD : MEMORY_TOWN);
				if ( pStone )
				{
					CScript script(pszKey, s.GetArgRaw());
					return pStone->r_Verb(script, pSrc);
				}
			}
			return false;
		}
	}

	switch ( index )
	{
		case CPV_KICK:
		{
			if ( m_pClient )
				m_pClient->addKick(pSrc);
			return true;
		}
		case CPV_PASSWORD:
		{
			if ( (pSrc != this) && ((pSrc->GetPrivLevel() <= GetPrivLevel()) || (pSrc->GetPrivLevel() < PLEVEL_Admin)) )
			{
				pSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_PRIV));
				return true;
			}

			if ( m_pPlayer->m_pAccount->SetPassword(s.GetArgStr()) )
			{
				SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_ACCEPTPASS));
				g_Log.Event(LOGM_ACCOUNTS, "Account '%s' changed password\n", m_pPlayer->m_pAccount->GetName());
			}
			else
				SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_INVALIDPASS));
			return true;
		}
		default:
			return false;
	}
}

///////////////////////////////////////////////////////////
// CCharNPC

CCharNPC::CCharNPC(CChar *pChar, NPCBRAIN_TYPE brain)
{
	m_Brain = brain;
	m_Home_Dist_Wander = WORD_MAX;
	m_Act_Motivation = 0;
	m_bonded = false;

	m_timeRestock.Init();
	memset(m_nextX, 0, sizeof(m_nextX));
	memset(m_nextY, 0, sizeof(m_nextY));

	pChar->SetSight(UO_MAP_VIEW_SIGHT);
}

void CCharNPC::Spells_Add(SPELL_TYPE spell)
{
	ADDTOCALLSTACK("CCharNPC::Spells_Add");
	CSpellDef *pSpell = g_Cfg.GetSpellDef(spell);
	if ( !pSpell )
		return;

	for ( size_t i = 0; i < m_spells.size(); ++i )
	{
		Spells refSpell = m_spells.at(i);
		if ( refSpell.id == spell )		// spell is already added
			return;
	}

	Spells refSpell;
	refSpell.id = spell;
	m_spells.push_back(refSpell);
}

SPELL_TYPE CCharNPC::Spells_GetAt(size_t index)
{
	ADDTOCALLSTACK("CCharNPC::Spells_GetAt");
	if ( m_spells.empty() || (index >= m_spells.size()) )
		return SPELL_NONE;

	Spells refSpell = m_spells.at(index);
	return refSpell.id ? refSpell.id : SPELL_NONE;
}

int CCharNPC::GetNpcAiFlags(const CChar *pChar) const
{
	const CVarDefCont *pVar = pChar->GetKey("OVERRIDE.NPCAI", true);
	return pVar ? static_cast<int>(pVar->GetValNum()) : g_Cfg.m_iNpcAi;
}

bool CCharNPC::r_LoadVal(CChar *pChar, CScript &s)
{
	EXC_TRY("LoadVal");
	switch ( FindTableSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		// Set as string
		case CNC_THROWDAM:
		case CNC_THROWOBJ:
		case CNC_THROWRANGE:
		{
			bool fQuoted = false;
			pChar->SetDefStr(s.GetKey(), s.GetArgStr(&fQuoted), fQuoted);
			return true;
		}
		case CNC_BONDED:
			m_bonded = (s.GetArgVal() > 0);
			pChar->UpdatePropertyFlag();
			return true;
		case CNC_ACTPRI:
			m_Act_Motivation = static_cast<BYTE>(s.GetArgVal());
			return true;
		case CNC_NPC:
			m_Brain = static_cast<NPCBRAIN_TYPE>(s.GetArgVal());
			return true;
		case CNC_HOMEDIST:
			m_Home_Dist_Wander = static_cast<WORD>(s.GetArgVal());
			return true;
		case CNC_NEED:
		case CNC_NEEDNAME:
		{
			TCHAR *pszArgs = s.GetArgRaw();
			m_Need.Load(pszArgs);
			return true;
		}
		case CNC_SPEECH:
			return m_Speech.r_LoadVal(s, RES_SPEECH);
		case CNC_VENDCAP:
		{
			CItemContainer *pBank = pChar->GetContainerCreate(LAYER_BANKBOX);
			if ( pBank )
				pBank->m_itEqBankBox.m_Check_Restock = static_cast<DWORD>(s.GetArgVal());
			return true;
		}
		case CNC_VENDGOLD:
		{
			CItemContainer *pBank = pChar->GetContainerCreate(LAYER_BANKBOX);
			if ( pBank )
				pBank->m_itEqBankBox.m_Check_Amount = static_cast<DWORD>(s.GetArgVal());
			return true;
		}
		case CNC_SPELLADD:
		{
			INT64 piVal[SPELL_MAGERY_QTY];
			size_t iQty = Str_ParseCmds(s.GetArgStr(), piVal, COUNTOF(piVal));
			for ( size_t i = 0; i < iQty; ++i )
				Spells_Add(static_cast<SPELL_TYPE>(piVal[i]));
			return true;
		}
		default:
			// Just ignore any player type stuff
			return (FindTableHeadSorted(s.GetKey(), CCharPlayer::sm_szLoadKeys, COUNTOF(CCharPlayer::sm_szLoadKeys) - 1) >= 0) ? true : false;
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CCharNPC::r_WriteVal(CChar *pChar, LPCTSTR pszKey, CGString &sVal)
{
	EXC_TRY("WriteVal");
	switch ( FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{

		// Return as string or hex number or NULL if not set
		// On these ones, check BaseDef too if not found on dynamic
		case CNC_THROWDAM:
		case CNC_THROWOBJ:
		case CNC_THROWRANGE:
			sVal = pChar->GetDefStr(pszKey, false, true);
			return true;
		// Return as decimal number or 0 if not set
		// On these ones, check BaseDef if not found on dynamic
		case CNC_BONDED:
			sVal.FormatVal(m_bonded);
			return true;
		case CNC_ACTPRI:
			sVal.FormatUVal(m_Act_Motivation);
			return true;
		case CNC_NPC:
			sVal.FormatVal(m_Brain);
			return true;
		case CNC_HOMEDIST:
			sVal.FormatUVal(m_Home_Dist_Wander);
			return true;
		case CNC_NEED:
		{
			TCHAR *pszArgs = Str_GetTemp();
			m_Need.WriteKey(pszArgs);
			sVal = pszArgs;
			return true;
		}
		case CNC_NEEDNAME:
		{
			TCHAR *pszArgs = Str_GetTemp();
			m_Need.WriteNameSingle(pszArgs);
			sVal = pszArgs;
			return true;
		}
		case CNC_SPEECH:
			m_Speech.WriteResourceRefList(sVal);
			return true;
		case CNC_VENDCAP:
		{
			CItemContainer *pBank = pChar->GetContainerCreate(LAYER_BANKBOX);
			if ( pBank )
				sVal.FormatUVal(pBank->m_itEqBankBox.m_Check_Restock);
			return true;
		}
		case CNC_VENDGOLD:
		{
			CItemContainer *pBank = pChar->GetContainerCreate(LAYER_BANKBOX);
			if ( pBank )
				sVal.FormatUVal(pBank->m_itEqBankBox.m_Check_Amount);
			return true;
		}
		default:
			if ( (FindTableHeadSorted(pszKey, CCharPlayer::sm_szLoadKeys, COUNTOF(CCharPlayer::sm_szLoadKeys) - 1) >= 0) || (FindTableSorted(pszKey, CClient::sm_szLoadKeys, CC_QTY) >= 0) )
			{
				sVal = "0";
				return true;
			}
			return false;
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pChar);
	EXC_DEBUG_END;
	return false;
}

void CCharNPC::r_WriteChar(CChar *pChar, CScript &s)
{
	UNREFERENCED_PARAMETER(pChar);

	s.WriteKeyVal("NPC", m_Brain);

	if ( m_Home_Dist_Wander < WORD_MAX )
		s.WriteKeyVal("HOMEDIST", m_Home_Dist_Wander);
	if ( m_Act_Motivation )
		s.WriteKeyHex("ACTPRI", m_Act_Motivation);

	m_Speech.r_Write(s, "SPEECH");

	if ( m_bonded )
		s.WriteKeyVal("BONDED", m_bonded);

	if ( m_Need.GetResourceID().IsValidUID() )
	{
		TemporaryString pszArgs;
		m_Need.WriteKey(pszArgs);
		s.WriteKey("NEED", pszArgs);
	}
}
