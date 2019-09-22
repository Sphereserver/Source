// CChar is either an NPC or a Player
#include "graysvr.h"	// predef header.

///////////////////////////////////////////////////////////
// Stats

void CChar::Stat_AddMod(STAT_TYPE stat, int iVal)
{
	ADDTOCALLSTACK("CChar::Stat_AddMod");
	if ( (stat < STAT_STR) || (stat >= STAT_QTY) )
		return;

	m_Stat[stat].m_mod += iVal;

	int iMaxValue = Stat_GetMax(stat);		// make sure the current value is not higher than new max value
	if ( m_Stat[stat].m_val > iMaxValue )
		m_Stat[stat].m_val = iMaxValue;

	UpdateStatsFlag();
}

void CChar::Stat_SetMod(STAT_TYPE stat, int iVal)
{
	ADDTOCALLSTACK("CChar::Stat_SetMod");
	if ( (stat < STAT_STR) || (stat >= STAT_QTY) )
		return;
	if ( iVal > USHRT_MAX )
		iVal = USHRT_MAX;
	else if ( iVal < -USHRT_MAX )
		iVal = -USHRT_MAX;

	int iStatVal = Stat_GetMod(stat);
	if ( IsTrigUsed(TRIGGER_STATCHANGE) && !IsTriggerActive("CREATE") )
	{
		if ( (stat >= STAT_STR) && (stat <= STAT_DEX) )
		{
			CScriptTriggerArgs args;
			args.m_iN1 = static_cast<INT64>(stat) + 8;		// shift by 8 to indicate modSTR, modINT, modDEX
			args.m_iN2 = iStatVal;
			args.m_iN3 = iVal;
			if ( OnTrigger(CTRIG_StatChange, this, &args) == TRIGRET_RET_TRUE )
				return;
			// do not restore argn1 to i, bad things will happen! leave i untouched. (matex)
			iVal = static_cast<int>(args.m_iN3);
		}
	}

	m_Stat[stat].m_mod = iVal;

	if ( (stat == STAT_STR) && (iVal < iStatVal) )
	{
		// ModSTR is being decreased, so check if the char still have enough STR to use current equipped items
		CItem *pItemNext = NULL;
		for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
		{
			pItemNext = pItem->GetNext();
			if ( !CanEquipStr(pItem) )
			{
				SysMessagef("%s %s.", g_Cfg.GetDefaultMsg(DEFMSG_EQUIP_NOT_STRONG_ENOUGH), pItem->GetName());
				ItemBounce(pItem, false);
			}
		}
	}

	int iMaxValue = Stat_GetMax(stat);		// make sure the current value is not higher than new max value
	if ( m_Stat[stat].m_val > iMaxValue )
		m_Stat[stat].m_val = iMaxValue;

	UpdateStatsFlag();
}

int CChar::Stat_GetMod(STAT_TYPE stat) const
{
	ADDTOCALLSTACK("CChar::Stat_GetMod");
	if ( (stat < STAT_STR) || (stat >= STAT_QTY) )
		return 0;

	return m_Stat[stat].m_mod;
}

void CChar::Stat_SetVal(STAT_TYPE stat, int iVal)
{
	ADDTOCALLSTACK("CChar::Stat_SetVal");
	if ( (stat < STAT_STR) || (stat >= STAT_QTY) )
		return;
	if ( iVal > USHRT_MAX )
		iVal = USHRT_MAX;
	else if ( iVal < -USHRT_MAX )
		iVal = -USHRT_MAX;

	if ( stat >= STAT_BASE_QTY )	// food must trigger @StatChange, redirect to base value
	{
		Stat_SetBase(stat, iVal);
		return;
	}
	m_Stat[stat].m_val = iVal;
}

int CChar::Stat_GetVal(STAT_TYPE stat) const
{
	ADDTOCALLSTACK("CChar::Stat_GetVal");
	if ( (stat < STAT_STR) || (stat >= STAT_QTY) )
		return 0;
	if ( stat >= STAT_BASE_QTY )	// food must trigger @StatChange, redirect to base value
		return Stat_GetBase(stat);
	return m_Stat[stat].m_val;
}

void CChar::Stat_SetMax(STAT_TYPE stat, int iVal)
{
	ADDTOCALLSTACK("CChar::Stat_SetMax");
	if ( (stat < STAT_STR) || (stat >= STAT_QTY) )
		return;
	if ( iVal > USHRT_MAX )
		iVal = USHRT_MAX;
	else if ( iVal < -USHRT_MAX )
		iVal = -USHRT_MAX;

	if ( g_Cfg.m_iStatFlag && ((g_Cfg.m_iStatFlag & STAT_FLAG_DENYMAX) || (m_pPlayer && (g_Cfg.m_iStatFlag & STAT_FLAG_DENYMAXP)) || (m_pNPC && (g_Cfg.m_iStatFlag & STAT_FLAG_DENYMAXN))) )
		m_Stat[stat].m_max = 0;
	else
	{
		if ( IsTrigUsed(TRIGGER_STATCHANGE) && !IsTriggerActive("CREATE") )
		{
			if ( (stat >= STAT_STR) && (stat <= STAT_FOOD) )		// only STR, DEX, INT, FOOD fire MaxHits, MaxMana, MaxStam, MaxFood for @StatChange
			{
				CScriptTriggerArgs args;
				args.m_iN1 = static_cast<INT64>(stat) + 4;		// shift by 4 to indicate MaxHits, etc..
				args.m_iN2 = Stat_GetMax(stat);
				args.m_iN3 = iVal;
				if ( OnTrigger(CTRIG_StatChange, this, &args) == TRIGRET_RET_TRUE )
					return;
				// do not restore argn1 to i, bad things will happen! leave it untouched. (matex)
				iVal = static_cast<int>(args.m_iN3);
			}
		}
		m_Stat[stat].m_max = iVal;

		int iMaxValue = Stat_GetMax(stat);		// make sure the current value is not higher than new max value
		if ( m_Stat[stat].m_val > iMaxValue )
			m_Stat[stat].m_val = iMaxValue;

		switch ( stat )
		{
			case STAT_STR:
				UpdateHitsFlag();
				break;
			case STAT_INT:
				UpdateManaFlag();
				break;
			case STAT_DEX:
				UpdateStamFlag();
				break;
		}
	}
}

int CChar::Stat_GetMax(STAT_TYPE stat) const
{
	ADDTOCALLSTACK("CChar::Stat_GetMax");
	if ( (stat < STAT_STR) || (stat >= STAT_QTY) )
		return 0;

	int iVal;
	if ( m_Stat[stat].m_max < 1 )
	{
		if ( stat == STAT_FOOD )
		{
			CCharBase *pCharDef = Char_GetDef();
			ASSERT(pCharDef);
			iVal = pCharDef->m_MaxFood;
		}
		else
			iVal = Stat_GetAdjusted(stat);

		if ( stat == STAT_INT )
		{
			if ( (g_Cfg.m_iRacialFlags & RACIALF_ELF_WISDOM) && IsElf() )
				iVal += 20;		// elves always have +20 max mana (Wisdom racial trait)
		}

		if ( iVal < 0 )
			return m_pPlayer ? 1 : 0;
		return iVal;
	}

	iVal = m_Stat[stat].m_max;
	if ( stat >= STAT_BASE_QTY )
		iVal += m_Stat[stat].m_mod;

	if ( iVal < 0 )
		return m_pPlayer ? 1 : 0;
	return iVal;
}

int CChar::Stat_GetSum() const
{
	ADDTOCALLSTACK("CChar::Stat_GetSum");
	int iStatSum = 0;
	for ( STAT_TYPE i = STAT_STR; i < STAT_BASE_QTY; i = static_cast<STAT_TYPE>(i + 1) )
		iStatSum += Stat_GetBase(i);

	return iStatSum;
}

int CChar::Stat_GetAdjusted(STAT_TYPE stat) const
{
	ADDTOCALLSTACK("CChar::Stat_GetAdjusted");
	int iVal = Stat_GetBase(stat) + Stat_GetMod(stat);
	if ( stat == STAT_KARMA )
		iVal = maximum(g_Cfg.m_iMinKarma, minimum(g_Cfg.m_iMaxKarma, iVal));
	else if ( stat == STAT_FAME )
		iVal = maximum(0, minimum(g_Cfg.m_iMaxFame, iVal));

	return iVal;
}

int CChar::Stat_GetBase(STAT_TYPE stat) const
{
	ADDTOCALLSTACK("CChar::Stat_GetBase");
	if ( (stat < STAT_STR) || (stat >= STAT_QTY) )
		return 0;
	if ( (stat == STAT_FAME) && (m_Stat[stat].m_base < 0) )		// fame can't be negative
		return 0;
	return m_Stat[stat].m_base;
}

void CChar::Stat_SetBase(STAT_TYPE stat, int iVal)
{
	ADDTOCALLSTACK("CChar::Stat_SetBase");
	if ( (stat < STAT_STR) || (stat >= STAT_QTY) )
		return;
	if ( iVal > USHRT_MAX )
		iVal = USHRT_MAX;
	else if ( iVal < -USHRT_MAX )
		iVal = -USHRT_MAX;

	int iStatVal = Stat_GetBase(stat);
	if ( IsTrigUsed(TRIGGER_STATCHANGE) && !g_Serv.IsLoading() && !IsTriggerActive("CREATE") )
	{
		// Only Str, Dex, Int, Food fire @StatChange here
		if ( (stat >= STAT_STR) && (stat <= STAT_FOOD) )
		{
			CScriptTriggerArgs args;
			args.m_iN1 = stat;
			args.m_iN2 = iStatVal;
			args.m_iN3 = iVal;
			if ( OnTrigger(CTRIG_StatChange, this, &args) == TRIGRET_RET_TRUE )
				return;
			// do not restore argn1 to i, bad things will happen! leave i untouched. (matex)
			iVal = static_cast<int>(args.m_iN3);

			if ( (stat != STAT_FOOD) && (m_Stat[stat].m_max < 1) ) // MaxFood cannot depend on something, otherwise if the Stat depends on STR, INT, DEX, fire MaxHits, MaxMana, MaxStam
			{
				args.m_iN1 = static_cast<INT64>(stat) + 4; // Shift by 4 to indicate MaxHits, MaxMana, MaxStam
				args.m_iN2 = iStatVal;
				args.m_iN3 = iVal;
				if ( OnTrigger(CTRIG_StatChange, this, &args) == TRIGRET_RET_TRUE )
					return;
				// do not restore argn1 to i, bad things will happen! leave i untouched. (matex)
				iVal = static_cast<int>(args.m_iN3);
			}
		}
	}
	switch ( stat )
	{
		case STAT_STR:
		{
			CCharBase *pCharDef = Char_GetDef();
			if ( pCharDef && !pCharDef->m_Str )
				pCharDef->m_Str = iVal;
			break;
		}
		case STAT_INT:
		{
			CCharBase *pCharDef = Char_GetDef();
			if ( pCharDef && !pCharDef->m_Int )
				pCharDef->m_Int = iVal;
			break;
		}
		case STAT_DEX:
		{
			CCharBase *pCharDef = Char_GetDef();
			if ( pCharDef && !pCharDef->m_Dex )
				pCharDef->m_Dex = iVal;
			break;
		}
		case STAT_FOOD:
			break;
		case STAT_KARMA:
			iVal = maximum(g_Cfg.m_iMinKarma, minimum(g_Cfg.m_iMaxKarma, iVal));
			break;
		case STAT_FAME:
			iVal = maximum(0, minimum(g_Cfg.m_iMaxFame, iVal));
			break;
		default:
			throw CGrayError(LOGL_CRIT, 0, "Stat_SetBase: index out of range");
	}

	m_Stat[stat].m_base = iVal;

	if ( (stat == STAT_STR) && (iVal < iStatVal) )
	{
		// STR is being decreased, so check if the char still have enough STR to use current equipped items
		CItem *pItemNext = NULL;
		for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
		{
			pItemNext = pItem->GetNext();
			if ( !CanEquipStr(pItem) )
			{
				SysMessagef("%s %s.", g_Cfg.GetDefaultMsg(DEFMSG_EQUIP_NOT_STRONG_ENOUGH), pItem->GetName());
				ItemBounce(pItem, false);
			}
		}
	}

	int iMaxValue = Stat_GetMax(stat);		// make sure the current value is not higher than new max value
	if ( m_Stat[stat].m_val > iMaxValue )
		m_Stat[stat].m_val = iMaxValue;

	UpdateStatsFlag();
	if ( !g_Serv.IsLoading() && (stat == STAT_KARMA) )
		NotoSave_Update();
}

int CChar::Stat_GetLimit(STAT_TYPE stat) const
{
	ADDTOCALLSTACK("CChar::Stat_GetLimit");
	const CVarDefCont *pTagStorage = NULL;
	TemporaryString sStatName;

	if ( m_pPlayer )
	{
		const CSkillClassDef *pSkillClass = m_pPlayer->GetSkillClass();
		ASSERT(pSkillClass);
		if ( stat == STAT_QTY )
		{
			if ( (pTagStorage = GetKey("OVERRIDE.STATSUM", true)) != NULL )
				return static_cast<int>(pTagStorage->GetValNum());

			return pSkillClass->m_StatSumMax;
		}
		ASSERT((stat >= STAT_STR) && (stat < STAT_BASE_QTY));

		sprintf(sStatName, "OVERRIDE.STATCAP_%d", static_cast<int>(stat));
		int iStatMax;
		if ( (pTagStorage = GetKey(sStatName, true)) != NULL )
			iStatMax = static_cast<int>(pTagStorage->GetValNum());
		else
			iStatMax = pSkillClass->m_StatMax[stat];

		if ( m_pPlayer->Stat_GetLock(stat) >= SKILLLOCK_DOWN )
		{
			int iStatLevel = Stat_GetBase(stat);
			if ( iStatLevel < iStatMax )
				iStatMax = iStatLevel;
		}

		return iStatMax;
	}
	else
	{
		if ( stat == STAT_QTY )
		{
			if ( (pTagStorage = GetKey("OVERRIDE.STATSUM", true)) != NULL )
				return static_cast<int>(pTagStorage->GetValNum());

			return 300;
		}

		sprintf(sStatName, "OVERRIDE.STATCAP_%d", static_cast<int>(stat));
		if ( (pTagStorage = GetKey(sStatName, true)) != NULL )
			return static_cast<int>(pTagStorage->GetValNum());

		return 100;
	}
}

///////////////////////////////////////////////////////////
// Skills

SKILL_TYPE CChar::Skill_GetBest(unsigned int uRank) const
{
	ADDTOCALLSTACK("CChar::Skill_GetBest");
	// Get the top n best skills.

	if ( uRank >= g_Cfg.m_iMaxSkill )
		uRank = 0;

	DWORD *pdwSkills = new DWORD[uRank + 1];
	ASSERT(pdwSkills);
	memset(pdwSkills, 0, (uRank + 1) * sizeof(DWORD));

	DWORD dwSkillTmp;
	for ( unsigned int i = 0; i < g_Cfg.m_iMaxSkill; ++i )
	{
		if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(i) )
			continue;

		dwSkillTmp = MAKEDWORD(i, Skill_GetBase(static_cast<SKILL_TYPE>(i)));
		for ( size_t j = 0; j <= uRank; ++j )
		{
			if ( HIWORD(dwSkillTmp) >= HIWORD(pdwSkills[j]) )
			{
				memmove(&pdwSkills[j + 1], &pdwSkills[j], (uRank - j) * sizeof(DWORD));
				pdwSkills[j] = dwSkillTmp;
				break;
			}
		}
	}

	dwSkillTmp = pdwSkills[uRank];
	delete[] pdwSkills;
	return static_cast<SKILL_TYPE>(LOWORD(dwSkillTmp));
}

SKILL_TYPE CChar::Skill_GetMagicRandom(WORD wMinValue)
{
	ADDTOCALLSTACK("CChar::Skill_GetMagicRandom");
	SKILL_TYPE skills[SKILL_QTY];
	int iCount = 0;
	for ( unsigned int i = 0; i < g_Cfg.m_iMaxSkill; ++i )
	{
		SKILL_TYPE skill = static_cast<SKILL_TYPE>(i);
		if ( !g_Cfg.IsSkillFlag(skill, SKF_MAGIC) )
			continue;
		if ( Skill_GetBase(skill) < wMinValue )
			continue;

		skills[iCount] = skill;
		++iCount;
	}
	if ( iCount )
		return skills[Calc_GetRandVal(iCount)];

	return SKILL_NONE;
}

bool CChar::Skill_CanUse(SKILL_TYPE skill)
{
	ADDTOCALLSTACK("CChar::Skill_CanUse");
	if ( g_Cfg.IsSkillFlag(skill, SKF_DISABLED) )	// skill disabled
	{
		SysMessageDefault(DEFMSG_SKILL_NOSKILL);
		return false;
	}

	if ( IsStatFlag(STATF_Ridden) )		// ridden mounts can't use skills
		return false;

	// Expansion checks? different flags for NPCs/Players?
	return true;
}

WORD CChar::Skill_GetAdjusted(SKILL_TYPE skill) const
{
	ADDTOCALLSTACK("CChar::Skill_GetAdjusted");
	// Get the skill adjusted for str,dex,int = 0-1000

	// m_SkillStat is used to figure out how much
	// of the total bonus comes from the stats
	// so if it's 80, then 20% (100% - 80%) comes from
	// the stat (str,int,dex) bonuses

	// example:

	// These are the cchar's stats:
	// m_Skill[x] = 50.0
	// m_Stat[str] = 50, m_Stat[int] = 30, m_Stat[dex] = 20

	// these are the skill "defs":
	// m_SkillStat = 80
	// m_StatBonus[str] = 50
	// m_StatBonus[int] = 50
	// m_StatBonus[dex] = 0

	// Pure bonus is:
	// 50% of str (25) + 50% of int (15) = 40

	// Percent of pure bonus to apply to raw skill is
	// 20% = 100% - m_SkillStat = 100 - 80

	// adjusted bonus is: 8 (40 * 0.2)

	// so the effective skill is 50 (the raw) + 8 (the bonus)
	// which is 58 in total.

	ASSERT(IsSkillBase(skill));
	const CSkillDef *pSkillDef = g_Cfg.GetSkillDef(skill);
	WORD wAdjSkill = 0;

	if ( pSkillDef )
	{
		int iPureBonus = (pSkillDef->m_StatBonus[STAT_STR] * maximum(0, Stat_GetAdjusted(STAT_STR))) + (pSkillDef->m_StatBonus[STAT_INT] * maximum(0, Stat_GetAdjusted(STAT_INT))) + (pSkillDef->m_StatBonus[STAT_DEX] * maximum(0, Stat_GetAdjusted(STAT_DEX)));
		wAdjSkill = static_cast<WORD>(IMULDIV(pSkillDef->m_StatPercent, iPureBonus, 10000));
	}

	return Skill_GetBase(skill) + wAdjSkill;
}

void CChar::Skill_SetBase(SKILL_TYPE skill, WORD wValue)
{
	ADDTOCALLSTACK("CChar::Skill_SetBase");
	ASSERT(IsSkillBase(skill));

	if ( IsTrigUsed(TRIGGER_SKILLCHANGE) )
	{
		CScriptTriggerArgs args;
		args.m_iN1 = static_cast<INT64>(skill);
		args.m_iN2 = static_cast<INT64>(wValue);
		if ( OnTrigger(CTRIG_SkillChange, this, &args) == TRIGRET_RET_TRUE )
			return;

		wValue = static_cast<WORD>(args.m_iN2);
	}
	m_Skill[skill] = wValue;

	if ( m_pClient )
		m_pClient->addSkillWindow(skill);	// update the skills list

	if ( g_Cfg.m_iCombatDamageEra )
	{
		if ( (skill == SKILL_ANATOMY) || (skill == SKILL_TACTICS) || (skill == SKILL_LUMBERJACKING) )
			UpdateStatsFlag();		// those skills are used to calculate the char damage bonus, so we must update the client status gump
	}
}

WORD CChar::Skill_GetMax(SKILL_TYPE skill, bool fIgnoreLock) const
{
	ADDTOCALLSTACK("CChar::Skill_GetMax");
	const CVarDefCont *pTagStorage = NULL;
	TemporaryString sSkillName;

	if ( m_pPlayer )
	{
		ASSERT(IsSkillBase(skill));
		sprintf(sSkillName, "OVERRIDE.SKILLCAP_%d", static_cast<int>(skill));

		WORD wSkillMax;
		if ( (pTagStorage = GetKey(sSkillName, true)) != NULL )
			wSkillMax = static_cast<WORD>(pTagStorage->GetValNum());
		else
		{
			const CSkillClassDef *pSkillClass = m_pPlayer->GetSkillClass();
			ASSERT(pSkillClass);
			wSkillMax = pSkillClass->m_SkillLevelMax[skill];
		}

		if ( !fIgnoreLock )
		{
			if ( m_pPlayer->Skill_GetLock(skill) >= SKILLLOCK_DOWN )
			{
				WORD wSkillLevel = Skill_GetBase(skill);
				if ( wSkillLevel < wSkillMax )
					wSkillMax = wSkillLevel;
			}
		}

		return wSkillMax;
	}
	else
	{
		if ( skill == static_cast<SKILL_TYPE>(g_Cfg.m_iMaxSkill) )
		{
			if ( (pTagStorage = GetKey("OVERRIDE.SKILLSUM", true)) != NULL )
				return static_cast<WORD>(pTagStorage->GetValNum());

			return static_cast<WORD>(g_Cfg.m_iMaxSkill) * 500;
		}

		sprintf(sSkillName, "OVERRIDE.SKILLCAP_%d", static_cast<int>(skill));
		if ( (pTagStorage = GetKey(sSkillName, true)) != NULL )
			return static_cast<WORD>(pTagStorage->GetValNum());

		return 1000;
	}
}

DWORD CChar::Skill_GetSumMax() const
{
	ADDTOCALLSTACK("CChar::Skill_GetSumMax");
	const CVarDefCont *pTagStorage = GetKey("OVERRIDE.SKILLSUM", true);
	if ( pTagStorage )
		return static_cast<DWORD>(pTagStorage->GetValNum());

	const CSkillClassDef *pSkillClass = m_pPlayer->GetSkillClass();
	if ( pSkillClass )
		return pSkillClass->m_SkillSumMax;

	return 0;
}

void CChar::Skill_Decay()
{
	ADDTOCALLSTACK("CChar::Skill_Decay");
	// Decay the character's skill levels.

	SKILL_TYPE skillDeduct = SKILL_NONE;
	WORD wSkillLevel = 0;

	// Look for a skill to deduct from
	for ( unsigned int i = 0; i < g_Cfg.m_iMaxSkill; ++i )
	{
		if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(i) )
			continue;

		// Check that the skill is set to decrease and that it is not already at 0
		if ( (Skill_GetLock(static_cast<SKILL_TYPE>(i)) != SKILLLOCK_DOWN) || (Skill_GetBase(static_cast<SKILL_TYPE>(i)) <= 0) )
			continue;

		// Prefer to deduct from lesser skills
		if ( (skillDeduct != SKILL_NONE) && (wSkillLevel > Skill_GetBase(static_cast<SKILL_TYPE>(i))) )
			continue;

		skillDeduct = static_cast<SKILL_TYPE>(i);
		wSkillLevel = Skill_GetBase(skillDeduct);
	}

	// deduct a point from the chosen skill
	if ( skillDeduct != SKILL_NONE )
	{
		--wSkillLevel;
		Skill_SetBase(skillDeduct, wSkillLevel);
	}
}

void CChar::Skill_Experience(SKILL_TYPE skill, int iDifficulty)
{
	ADDTOCALLSTACK("CChar::Skill_Experience");
	// Give the char credit for using the skill.
	// More credit for the more difficult. or none if too easy
	// ARGS:
	//  iDifficulty = skill target from 0-100

	if ( !IsSkillBase(skill) || !g_Cfg.m_SkillIndexDefs.IsValidIndex(skill) )
		return;
	if ( m_pArea && m_pArea->IsFlag(REGION_FLAG_SAFE) )	// skills don't advance in safe areas.
		return;

	const CSkillDef *pSkillDef = g_Cfg.GetSkillDef(skill);
	if ( !pSkillDef )
		return;

	iDifficulty *= 10;
	if ( iDifficulty < 1 )
		iDifficulty = 1;
	else if ( iDifficulty > 1000 )
		iDifficulty = 1000;

	if ( m_pPlayer && (GetSkillTotal() >= Skill_GetSumMax()) )
		iDifficulty = 0;

	// ex. ADV_RATE=2000,500,25 -> easy
	// ex. ADV_RATE=8000,2000,100 -> hard
	// Assume 100 = a 1 for 1 gain
	// ex: 8000 = we must use it 80 times to gain .1
	// Higher the number = the less probable to advance.
	// Extrapolate a place in the range.

	// give a bonus or a penalty if the task was too hard or too easy.
	// no gain at all if it was WAY TOO easy
	WORD wSkillLevel = Skill_GetBase(skill);
	WORD wSkillLevelFixed = maximum(50, wSkillLevel);
	int iGainRadius = pSkillDef->m_GainRadius;
	if ( (iGainRadius > 0) && ((iDifficulty + iGainRadius) < wSkillLevelFixed) )
	{
		if ( GetKeyNum("NOSKILLMSG") )
			SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_GAINRADIUS_NOT_MET));
		return;
	}

	INT64 iChance = pSkillDef->m_AdvRate.GetChancePercent(wSkillLevel);
	INT64 iSkillMax = Skill_GetMax(skill);	// max advance for this skill.

	CScriptTriggerArgs pArgs(0, iChance, iSkillMax);
	if ( IsTrigUsed(TRIGGER_SKILLGAIN) )
	{
		if ( Skill_OnCharTrigger(skill, CTRIG_SkillGain, &pArgs) == TRIGRET_RET_TRUE )
			return;
	}
	if ( IsTrigUsed(TRIGGER_GAIN) )
	{
		if ( Skill_OnTrigger(skill, SKTRIG_GAIN, &pArgs) == TRIGRET_RET_TRUE )
			return;
	}
	pArgs.getArgNs(0, &iChance, &iSkillMax);

	if ( iChance <= 0 )
		return;

	if ( wSkillLevelFixed < iSkillMax )	// are we in position to gain skill ?
	{
		// slightly more chance of decay than gain
		INT64 iRoll = Calc_GetRandVal(1000);
		if ( iRoll * 3 <= iChance * 4 )
			Skill_Decay();

		if ( iDifficulty > 0 )
		{
#ifdef _DEBUG
			if ( IsPriv(PRIV_DETAIL) && (GetPrivLevel() >= PLEVEL_GM) && (g_Cfg.m_wDebugFlags & DEBUGF_ADVANCE_STATS) )
				SysMessagef("%s=%d.%d Difficult=%d Gain Chance=%lld.%lld%% Roll=%d%%", pSkillDef->GetKey(), wSkillLevel / 10, wSkillLevel % 10, iDifficulty / 10, iChance / 10, iChance % 10, iRoll / 10);
#endif
			if ( iRoll <= iChance )
			{
				++wSkillLevel;
				Skill_SetBase(skill, wSkillLevel);
			}
		}
	}

	// Dish out any stat gains - even for failures
	int iStatSum = Stat_GetSum();
	int iStatCap = Stat_GetLimit(STAT_QTY);

	// Stat effects are unrelated to advance in skill !
	for ( STAT_TYPE i = STAT_STR; i < STAT_BASE_QTY; i = static_cast<STAT_TYPE>(i + 1) )
	{
		// Can't gain STR or DEX if morphed.
		if ( IsStatFlag(STATF_Polymorph) && (i != STAT_INT) )
			continue;

		if ( Stat_GetLock(i) != SKILLLOCK_UP )
			continue;

		int iStatVal = Stat_GetBase(i);
		if ( iStatVal <= 0 )	// some odd condition
			continue;

		if ( iStatSum > iStatCap )	// stat cap already reached
			break;

		int iStatMax = Stat_GetLimit(i);
		if ( iStatVal >= iStatMax )
			continue;	// nothing grows past this. (even for NPC's)

		// You will tend toward these stat vals if you use this skill a lot
		BYTE iStatTarg = pSkillDef->m_Stat[i];
		if ( iStatVal >= iStatTarg )
			continue;	// you've got higher stats than this skill is good for

		// Adjust the chance by the percent of this that the skill uses
		iDifficulty = IMULDIV(iStatVal, 1000, iStatTarg);
		iChance = g_Cfg.m_StatAdv[i].GetChancePercent(iDifficulty);
		if ( pSkillDef->m_StatPercent )
			iChance = (iChance * pSkillDef->m_StatBonus[i] * pSkillDef->m_StatPercent) / 10000;

		if ( iChance == 0 )
			continue;

		if ( Stat_Decrease(i, skill) )
		{
			iStatSum = Stat_GetSum();
			if ( iChance > Calc_GetRandVal(1000) )
			{
				Stat_SetBase(i, iStatVal + 1);
				break;
			}
		}
	}
}

bool CChar::Stats_Regen(INT64 iTimeDiff)
{
	ADDTOCALLSTACK("CChar::Stats_Regen");
	// Calling regens in all stats and checking REGEN%s/REGEN%sVAL where %s is hits/stam... to check values/delays
	// Food decay called here too.
	// calling @RegenStat for each stat if proceed.
	// iTimeDiff is the next tick the stats are going to regen.

	int iHitsHungerLoss = g_Cfg.m_iHitsHungerLoss ? g_Cfg.m_iHitsHungerLoss : 0;

	for ( STAT_TYPE i = STAT_STR; i <= STAT_FOOD; i = static_cast<STAT_TYPE>(i + 1) )
	{
		if ( g_Cfg.m_iRegenRate[i] <= 0 )
			continue;

		WORD wRate = Stats_GetRegenVal(i, true);
		if ( wRate <= 0 )
			continue;

		m_Stat[i].m_regen += static_cast<WORD>(iTimeDiff);
		if ( m_Stat[i].m_regen < wRate )
			continue;

		m_Stat[i].m_regen = 0;

		int iMod = Stats_GetRegenVal(i, false);
		if ( (i == STAT_STR) && (g_Cfg.m_iRacialFlags & RACIALF_HUMAN_TOUGH) && IsHuman() )
			iMod += 2;		// humans always have +2 hitpoint regeneration (Tough racial trait)

		int iStatLimit = Stat_GetMax(i);

		if ( IsTrigUsed(TRIGGER_REGENSTAT) )
		{
			CScriptTriggerArgs Args;
			Args.m_VarsLocal.SetNum("StatID", i, true);		// read-only
			Args.m_VarsLocal.SetNum("Value", iMod, true);
			Args.m_VarsLocal.SetNum("StatLimit", iStatLimit, true);
			if ( i == STAT_FOOD )
				Args.m_VarsLocal.SetNum("HitsHungerLoss", iHitsHungerLoss);

			if ( OnTrigger(CTRIG_RegenStat, this, &Args) == TRIGRET_RET_TRUE )
				continue;

			iMod = static_cast<int>(Args.m_VarsLocal.GetKeyNum("Value"));
			iStatLimit = static_cast<int>(Args.m_VarsLocal.GetKeyNum("StatLimit"));
			if ( i == STAT_FOOD )
				iHitsHungerLoss = static_cast<int>(Args.m_VarsLocal.GetKeyNum("HitsHungerLoss"));
		}
		if ( iMod == 0 )
			continue;

		if ( i == STAT_FOOD )
			OnTickFood(iMod, iHitsHungerLoss);
		else
			UpdateStatVal(i, iMod, iStatLimit);
	}
	return true;
}

WORD CChar::Stats_GetRegenVal(STAT_TYPE stat, bool fGetTicks)
{
	ADDTOCALLSTACK("CChar::Stats_GetRegenVal");
	// Return regen rates and regen val for the given stat.
	// fGetTicks = true returns the regen ticks
	// fGetTicks = false returns the values of regeneration.

	LPCTSTR pszStat = NULL;
	switch ( stat )
	{
		case STAT_STR:
			pszStat = "HITS";
			break;
		case STAT_INT:
			pszStat = "MANA";
			break;
		case STAT_DEX:
			pszStat = "STAM";
			break;
		case STAT_FOOD:
			pszStat = "FOOD";
			break;
		default:
			break;
	}

	if ( stat <= STAT_FOOD )
	{
		char chRegen[14];
		if ( fGetTicks )
		{
			sprintf(chRegen, "REGEN%s", pszStat);
			WORD wRate = static_cast<WORD>(maximum(0, GetDefNum(chRegen)));
			if ( wRate )
				return wRate * TICK_PER_SEC;

			return static_cast<WORD>(maximum(0, g_Cfg.m_iRegenRate[stat]));
		}
		else
		{
			sprintf(chRegen, "REGENVAL%s", pszStat);
			return static_cast<WORD>(maximum(1, GetDefNum(chRegen)));
		}
	}
	return 0;
}

bool CChar::Stat_Decrease(STAT_TYPE stat, SKILL_TYPE skill)
{
	// Stat to decrease
	// Skill = is this being called from Skill_Gain? if so we check this skill's bonuses.
	if ( !m_pPlayer )
		return false;

	// Check for stats degrade.
	int iStatSumAvg = Stat_GetLimit(STAT_QTY);
	int iStatSum = Stat_GetSum() + 1;	// +1 here assuming we are going to have +1 stat at some point thus we are calling this function
	if ( iStatSum <= iStatSumAvg )		// no need to lower any stat
		return true;

	int iMinVal;
	if ( skill )
		iMinVal = Stat_GetMax(stat);
	else
		iMinVal = Stat_GetAdjusted(stat);

	// We are at a point where our skills can degrade a bit.
	int iStatSumMax = iStatSumAvg + iStatSumAvg / 4;
	int iChanceForLoss = Calc_GetSCurve(iStatSumMax - iStatSum, (iStatSumMax - iStatSumAvg) / 4);
	if ( iChanceForLoss > Calc_GetRandVal(1000) )
	{
		// Find the stat that was used least recently and degrade it.
		STAT_TYPE iMin = STAT_NONE;
		int iVal = 0;
		for ( STAT_TYPE i = STAT_STR; i < STAT_BASE_QTY; i = static_cast<STAT_TYPE>(i + 1) )
		{
			if ( i == stat )
				continue;
			if ( Stat_GetLock(i) != SKILLLOCK_DOWN )
				continue;

			if ( skill )
			{
				const CSkillDef *pSkillDef = g_Cfg.GetSkillDef(skill);
				iVal = pSkillDef->m_StatBonus[i];
			}
			else
				iVal = Stat_GetBase(i);

			if ( iMinVal > iVal )
			{
				iMin = i;
				iMinVal = iVal;
			}
		}

		if ( iMin == STAT_NONE )
			return false;

		int iStatVal = Stat_GetBase(iMin);
		if ( iStatVal > 10 )
		{
			Stat_SetBase(iMin, iStatVal - 1);
			return true;
		}
	}
	return false;
}

bool CChar::Skill_CheckSuccess(SKILL_TYPE skill, int iDifficulty, bool fUseBellCurve) const
{
	ADDTOCALLSTACK("CChar::Skill_CheckSuccess");
	// PURPOSE:
	//  Check a skill for success or fail.
	//  DO NOT give experience here.
	// ARGS:
	//  iDifficulty		= 0-100 = The point at which the equiv skill level has a 50% chance of success.
	//  fUseBellCurve	= check skill success chance using bell curve or a simple percent check?
	// RETURN:
	//  true = success in skill.

	if ( IsPriv(PRIV_GM) && (skill != SKILL_PARRYING) )		// GM's can't always succeed Parrying or they won't receive any damage on combat even without STATF_Invul set
		return true;
	if ( !IsSkillBase(skill) || (iDifficulty < 0) )	// auto failure
		return false;

	iDifficulty *= 10;
	if ( fUseBellCurve )
		iDifficulty = Calc_GetSCurve(Skill_GetAdjusted(skill) - iDifficulty, SKILL_VARIANCE);

	return (iDifficulty >= Calc_GetRandVal(1000));
}

bool CChar::Skill_UseQuick(SKILL_TYPE skill, int iDifficulty, bool fAllowGain, bool fUseBellCurve)
{
	ADDTOCALLSTACK("CChar::Skill_UseQuick");
	// ARGS:
	//  skill			= skill to use
	//  iDifficulty		= 0-100
	//  fAllowGain		= can gain skill from this?
	//  fUseBellCurve	= check skill success chance using bell curve or a simple percent check?
	// Use a skill instantly. No wait at all.
	// No interference with other skills.

	if ( g_Cfg.IsSkillFlag(skill, SKF_SCRIPTED) )
		return false;

	INT64 iResult = Skill_CheckSuccess(skill, iDifficulty, fUseBellCurve);
	INT64 iDiff = iDifficulty;
	CScriptTriggerArgs pArgs(0, iDiff, iResult);
	TRIGRET_TYPE ret = TRIGRET_RET_DEFAULT;

	if ( IsTrigUsed(TRIGGER_SKILLUSEQUICK) )
	{
		ret = Skill_OnCharTrigger(skill, CTRIG_SkillUseQuick, &pArgs);

		if ( ret == TRIGRET_RET_TRUE )
			return true;
		if ( ret == TRIGRET_RET_FALSE )
			return false;
		pArgs.getArgNs(0, &iDiff, &iResult);
	}
	if ( IsTrigUsed(TRIGGER_USEQUICK) )
	{
		ret = Skill_OnTrigger(skill, SKTRIG_USEQUICK, &pArgs);

		if ( ret == TRIGRET_RET_TRUE )
			return true;
		if ( ret == TRIGRET_RET_FALSE )
			return false;
		pArgs.getArgNs(0, &iDiff, &iResult);
	}

	if ( iResult > 0 )	// success
	{
		if ( fAllowGain )
			Skill_Experience(skill, static_cast<int>(iDiff));
		return true;
	}
	else				// fail
	{
		if ( fAllowGain )
			Skill_Experience(skill, static_cast<int>(-iDiff));
		return false;
	}
}

void CChar::Skill_Cleanup()
{
	ADDTOCALLSTACK("CChar::Skill_Cleanup");
	// We are done with the skill (succeeded / failed / aborted)
	m_Act_Difficulty = 0;
	m_Act_SkillCurrent = SKILL_NONE;
	SetTimeout(m_pPlayer ? -1 : TICK_PER_SEC);	// we should get a brain tick next time
}

LPCTSTR CChar::Skill_GetName(bool fUse) const
{
	ADDTOCALLSTACK("CChar::Skill_GetName");
	// Name the current skill we are doing.

	SKILL_TYPE skill = Skill_GetActive();
	if ( IsSkillBase(skill) )
	{
		if ( !fUse )
			return g_Cfg.GetSkillKey(skill);

		TCHAR *pszText = Str_GetTemp();
		sprintf(pszText, "%s %s", g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_USING), g_Cfg.GetSkillKey(skill));
		return pszText;
	}

	switch ( skill )
	{
		case NPCACT_FOLLOW_TARG:	return g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_FOLLOWING);
		case NPCACT_STAY:			return g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_STAYING);
		case NPCACT_GOTO:			return g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_GOINGTO);
		case NPCACT_WANDER:			return g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_WANDERING);
		case NPCACT_LOOKING:		return g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_LOOKING);
		case NPCACT_FLEE:			return g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_FLEEING);
		case NPCACT_TALK:			return g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_TALKING);
		case NPCACT_TALK_FOLLOW:	return g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_TALKFOLLOW);
		case NPCACT_GUARD_TARG:		return g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_GUARDING);
		case NPCACT_GO_HOME:		return g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_GOINGHOME);
		case NPCACT_BREATH:			return g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_BREATHING);
		case NPCACT_RIDDEN:			return g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_RIDDEN);
		case NPCACT_THROWING:		return g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_THROWING);
		case NPCACT_TRAINING:		return g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_TRAINING);
		case NPCACT_FOOD:			return g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_SEARCHINGFOOD);
		case NPCACT_RUNTO:			return g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_RUNNINGTO);
		default:					return g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_IDLING);
	}
}

void CChar::Skill_SetTimeout()
{
	ADDTOCALLSTACK("CChar::Skill_SetTimeout");
	SKILL_TYPE skill = Skill_GetActive();
	ASSERT(IsSkillBase(skill));

	const CSkillDef *pSkillDef = g_Cfg.GetSkillDef(skill);
	if ( !pSkillDef )
		return;

	SetTimeout(pSkillDef->m_Delay.GetLinear(Skill_GetBase(skill)));
}

INT64 CChar::Skill_GetTimeout()
{
	ADDTOCALLSTACK("CChar::Skill_SetTimeout");
	SKILL_TYPE skill = Skill_GetActive();
	ASSERT(IsSkillBase(skill));

	const CSkillDef *pSkillDef = g_Cfg.GetSkillDef(skill);
	if ( !pSkillDef )
		return 0;

	return pSkillDef->m_Delay.GetLinear(Skill_GetBase(skill));
}

int CChar::SkillResourceTest(const CResourceQtyArray *pResources)
{
	ADDTOCALLSTACK("CChar::SkillResourceTest");
	return pResources->IsResourceMatchAll(this);
}

bool CChar::Skill_MakeItem(ITEMID_TYPE id, CGrayUID uidTarg, SKTRIG_TYPE stage, bool fSkillOnly, DWORD dwReplicationQty)
{
	ADDTOCALLSTACK("CChar::Skill_MakeItem");
	// "MAKEITEM"
	//
	// SKILL_ALCHEMY
	// SKILL_BLACKSMITHING
	// SKILL_BOWCRAFT
	// SKILL_CARPENTRY
	// SKILL_CARTOGRAPHY
	// SKILL_COOKING
	// SKILL_INSCRIPTION
	// SKILL_TAILORING
	// SKILL_TINKERING
	//
	// Confer the new item.
	// Test for consumable items.
	// Fail = do a partial consume of the resources.
	//
	// ARGS:
	//  uidTarg = item targetted to try to make this . (this item should be used to make somehow)
	// Skill_GetActive()
	//
	// RETURN:
	//  true = success

	CItemBase *pItemDef = CItemBase::FindItemBase(id);
	if ( !pItemDef )
		return false;

	CItem *pItemTarg = uidTarg.ItemFind();
	if ( pItemTarg && (stage == SKTRIG_SELECT) )
	{
		if ( !pItemDef->m_SkillMake.ContainsResourceMatch(pItemTarg) && !pItemDef->m_BaseResources.ContainsResourceMatch(pItemTarg) )
			return false;
	}

	if ( !SkillResourceTest(&(pItemDef->m_SkillMake)) )
		return false;
	if ( fSkillOnly )
		return true;

	CItem *pItemDragging = LayerFind(LAYER_DRAGGING);
	if ( pItemDragging )
		ItemBounce(pItemDragging);

	dwReplicationQty = ResourceConsume(&(pItemDef->m_BaseResources), dwReplicationQty, stage != SKTRIG_SUCCESS, pItemDef->GetResourceID().GetResIndex());
	if ( !dwReplicationQty )
		return false;

	// Test or consume the needed resources.
	if ( stage == SKTRIG_FAIL )
	{
		// If fail only consume part of them.
		int iConsumePercent = -1;
		size_t i = pItemDef->m_SkillMake.FindResourceType(RES_SKILL);
		if ( i != pItemDef->m_SkillMake.BadIndex() )
		{
			CSkillDef *pSkillDef = g_Cfg.GetSkillDef(static_cast<SKILL_TYPE>(pItemDef->m_SkillMake[i].GetResIndex()));
			if ( pSkillDef && (pSkillDef->m_Effect.m_aiValues.GetCount() > 0) )
				iConsumePercent = pSkillDef->m_Effect.GetRandom();
		}

		if ( iConsumePercent < 0 )
			iConsumePercent = Calc_GetRandVal(50);

		ResourceConsumePart(&(pItemDef->m_BaseResources), dwReplicationQty, iConsumePercent, false, pItemDef->GetResourceID().GetResIndex());
		return false;
	}

	if ( stage == SKTRIG_START )
	{
		// Start the skill.
		// Find the primary skill required.
		size_t i = pItemDef->m_SkillMake.FindResourceType(RES_SKILL);
		if ( i == pItemDef->m_SkillMake.BadIndex() )
			return false;

		m_Act_Targ = uidTarg;	// targetted item to start the make process
		m_atCreate.m_ItemID = id;
		m_atCreate.m_Amount = static_cast<WORD>(dwReplicationQty);

		CResourceQty RetMainSkill = pItemDef->m_SkillMake[i];
		return Skill_Start(static_cast<SKILL_TYPE>(RetMainSkill.GetResIndex()));
	}

	if ( stage == SKTRIG_SUCCESS )
	{
		m_atCreate.m_Amount = static_cast<WORD>(dwReplicationQty);	// how much resources we really consumed
		return Skill_MakeItem_Success();
	}

	return true;
}

bool CChar::Skill_MakeItem_Success()
{
	ADDTOCALLSTACK("CChar::Skill_MakeItem_Success");
	// Deliver the goods

	CItem *pItem = CItem::CreateTemplate(m_atCreate.m_ItemID, NULL, this);
	if ( !pItem )
		return false;

	int iQuality = 0;
	TCHAR *pszMsg = Str_GetTemp();
	WORD wSkillLevel = Skill_GetBase(Skill_GetActive());					// primary skill value
	CItemVendable *pItemVend = dynamic_cast<CItemVendable *>(pItem);		// cast CItemVendable for setting quality and exp later

	if ( m_atCreate.m_Amount != 1 )
	{
		if ( pItem->IsType(IT_SCROLL) )
			pItem->m_itSpell.m_spelllevel = wSkillLevel;

		CItemBase *pItemDef = CItemBase::FindItemBase(m_atCreate.m_ItemID);
		if ( pItemDef && pItemDef->Can(CAN_I_PILE) )
			pItem->SetAmount(m_atCreate.m_Amount);
		else
		{
			for ( WORD i = 1; i < m_atCreate.m_Amount; ++i )
			{
				CItem *pItemNew = CItem::CreateTemplate(m_atCreate.m_ItemID, NULL, this);
				ItemBounce(pItemNew);
			}
		}
	}
	else if ( pItem->IsType(IT_SCROLL) )
	{
		// scrolls have the skill level of the inscriber ?
		pItem->m_itSpell.m_spelllevel = wSkillLevel;
	}
	else if ( pItem->IsType(IT_POTION) )
	{
		// Create the potion, set various properties,
	}
	else
	{
		// Only set the quality on single items.
		// Quality depends on the skill of the craftsman, and a random chance.
		// minimum quality is 1, maximum quality is 200.  100 is average.

		// How much variance? This is the difference in quality levels from what I can normally make.
		int iVariance = 2 - static_cast<int>(log10(static_cast<double>(Calc_GetRandVal(250)) + 1));	// this should result in a value between 0 and 2

		// Determine if lower or higher quality
		if ( !Calc_GetRandVal(2) )
			iVariance = -iVariance;		// worse than I can normally make

		// Determine which range I'm in
		iQuality = IMULDIV(wSkillLevel, 2, 10);	// default value for quality
		int iQualityBase;
		if ( iQuality < 25 )		// 1 - 25		Shoddy
			iQualityBase = 0;
		else if ( iQuality < 50 )	// 26 - 50		Poor
			iQualityBase = 1;
		else if ( iQuality < 75 )	// 51 - 75		Below Average
			iQualityBase = 2;
		else if ( iQuality < 125 )	// 76 - 125		Average
			iQualityBase = 3;
		else if ( iQuality < 150 )	// 125 - 150	Above Average
			iQualityBase = 4;
		else if ( iQuality < 175 )	// 151 - 175	Excellent
			iQualityBase = 5;
		else						// 175 - 200	Superior
			iQualityBase = 6;

		iQualityBase += iVariance;
		if ( iQualityBase < 0 )
			iQualityBase = 0;
		else if ( iQualityBase > 6 )
			iQualityBase = 6;

		switch ( iQualityBase )
		{
			case 0:		// shoddy quality
				strcpy(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MAKESUCCESS_1));
				iQuality = Calc_GetRandVal(25) + 1;
				break;
			case 1:		// poor quality
				strcpy(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MAKESUCCESS_2));
				iQuality = Calc_GetRandVal(25) + 26;
				break;
			case 2:		// below average quality
				strcpy(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MAKESUCCESS_3));
				iQuality = Calc_GetRandVal(25) + 51;
				break;
			case 3:		// average quality
				iQuality = Calc_GetRandVal(50) + 76;
				break;
			case 4:		// above average quality
				strcpy(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MAKESUCCESS_4));
				iQuality = Calc_GetRandVal(25) + 126;
				break;
			case 5:		// excellent quality
				strcpy(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MAKESUCCESS_5));
				iQuality = Calc_GetRandVal(25) + 151;
				break;
			case 6:		// superior quality
				strcpy(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MAKESUCCESS_6));
				iQuality = Calc_GetRandVal(25) + 176;
				break;
		}

		if ( pItemVend )	// only set quality property on vendable items
			pItemVend->SetQuality(static_cast<WORD>(iQuality));

		if ( (iQuality > 175) && (wSkillLevel > 999) && !IsSetOF(OF_NoItemNaming) )
		{
			// A GM made this, and it is of high quality
			TCHAR *pszNewName = Str_GetTemp();
			sprintf(pszNewName, g_Cfg.GetDefaultMsg(DEFMSG_GRANDMASTER_MARK), pItem->GetName(), GetName());
			pItem->SetName(pszNewName);
			//pItem->SetDefNum("CRAFTEDBY", GetUID());		// TO-DO: before enable this we must find away to properly clear CRAFTEDBY value on items when CRAFTEDBY char got deleted
		}
	}

	// Item goes into ACT of player
	CGrayUID uidOldAct = m_Act_Targ;
	m_Act_Targ = pItem->GetUID();
	TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;
	if ( IsTrigUsed(TRIGGER_SKILLMAKEITEM) )
	{
		CScriptTriggerArgs Args(wSkillLevel, iQuality, uidOldAct.ObjFind());
		iRet = OnTrigger(CTRIG_SkillMakeItem, this, &Args);
	}
	m_Act_Targ = uidOldAct;		// restore

	if ( iRet == TRIGRET_RET_TRUE )
	{
		pItem->Delete();
		return false;
	}
	if ( iRet == TRIGRET_RET_DEFAULT )
	{
		if ( !g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_NOSFX) && pItem->IsType(IT_POTION) )
			Sound(SOUND_LIQUID);
		if ( *pszMsg )
			SysMessage(pszMsg);
	}

	// Experience gain on craftings
	if ( g_Cfg.m_bExperienceSystem && (g_Cfg.m_iExperienceMode & EXP_MODE_RAISE_CRAFT) )
	{
		int iExp = 0;
		if ( pItemVend )
			iExp = pItemVend->GetVendorPrice(0) / 100;	// calculate cost for buying this item if it is vendable (gain = +1 exp each 100gp)
		if ( iExp )
			ChangeExperience(iExp);
	}

	ItemBounce(pItem);
	return true;
}

int CChar::Skill_NaturalResource_Setup(CItem *pResBit)
{
	ADDTOCALLSTACK("CChar::Skill_NaturalResource_Setup");
	// RETURN:
	//  difficulty = 0-100
	ASSERT(pResBit);

	// Find the resource type located here based on color.
	const CRegionResourceDef *pResourceDef = dynamic_cast<const CRegionResourceDef *>(g_Cfg.ResourceGetDef(pResBit->m_itResource.m_rid_res));
	if ( !pResourceDef )
		return -1;

	return pResourceDef->m_Skill.GetRandom() / 10;
}

CItem *CChar::Skill_NaturalResource_Create(CItem *pResBit, SKILL_TYPE skill)
{
	ADDTOCALLSTACK("CChar::Skill_NaturalResource_Create");
	// Create some natural resource item.
	// skill = Effects qty of items returned.
	// SKILL_MINING
	// SKILL_FISHING
	// SKILL_LUMBERJACKING
	ASSERT(pResBit);

	// Find the resource type located here based on color.
	CRegionResourceDef *pResourceDef = dynamic_cast<CRegionResourceDef *>(g_Cfg.ResourceGetDef(pResBit->m_itResource.m_rid_res));
	if ( !pResourceDef )
		return NULL;

	// Skill effects how much of the resource i can get all at once.
	if ( pResourceDef->m_ReapItem == ITEMID_NOTHING )
		return NULL;

	// Reap amount is semi-random
	WORD wAmount = static_cast<WORD>(pResourceDef->m_ReapAmount.GetRandomLinear(Skill_GetBase(skill)));
	if ( !wAmount )		// if REAPAMOUNT wasn't defined
	{
		wAmount = static_cast<WORD>(pResourceDef->m_Amount.GetRandomLinear(Skill_GetBase(skill)) / 2);
		WORD wMaxAmount = pResBit->GetAmount();
		if ( wAmount < 1 )
			wAmount = 1;
		if ( wAmount > wMaxAmount )
			wAmount = wMaxAmount;
	}

	// [Region]ResourceGather behavior
	CScriptTriggerArgs Args(0, 0, pResBit);
	Args.m_VarsLocal.SetNum("ResourceID", pResourceDef->m_ReapItem);
	Args.m_iN1 = wAmount;
	TRIGRET_TYPE tRet = TRIGRET_RET_DEFAULT;
	if ( IsTrigUsed(TRIGGER_REGIONRESOURCEGATHER) )
		tRet = OnTrigger(CTRIG_RegionResourceGather, this, &Args);
	if ( IsTrigUsed(TRIGGER_RESOURCEGATHER) )
		tRet = pResourceDef->OnTrigger("@ResourceGather", this, &Args);
	if ( tRet == TRIGRET_RET_TRUE )
		return NULL;

	wAmount = static_cast<WORD>(pResBit->ConsumeAmount(static_cast<DWORD>(Args.m_iN1)));	// amount i used up.
	if ( wAmount <= 0 )
		return NULL;

	// Create 'id' variable with the local given through->by the trigger(s) instead on top of method
	ITEMID_TYPE id = static_cast<ITEMID_TYPE>(RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("ResourceID")));
	CItem *pItem = CItem::CreateScript(id, this);
	ASSERT(pItem);
	pItem->SetAmount(wAmount);
	return pItem;
}

bool CChar::Skill_SmeltOre(CItem *pOre)
{
	ADDTOCALLSTACK("CChar::Skill_SmeltOre");
	// Smelt ores into ingots
	if ( !pOre )
		return false;

	m_Act_p = g_World.FindItemTypeNearby(GetTopPoint(), IT_FORGE, 2, false, true);
	if ( !m_Act_p.IsValidPoint() || !CanTouch(m_Act_p) )
	{
		SysMessageDefault(DEFMSG_SMELT_NOFORGE);
		return false;
	}

	const CItemBase *pIngotDef = CItemBase::FindItemBase(static_cast<ITEMID_TYPE>(RES_GET_INDEX(pOre->Item_GetDef()->m_ttOre.m_IngotID)));
	if ( !pIngotDef || !pIngotDef->IsType(IT_INGOT) || pOre->IsType(IT_INGOT) )
	{
		SysMessageDefault(DEFMSG_SMELT_CANT);
		return false;
	}

	if ( Skill_GetAdjusted(SKILL_MINING) < static_cast<WORD>(pIngotDef->m_ttIngot.m_iSkillReq) )
	{
		SysMessageDefault(DEFMSG_SMELT_NOSKILL);
		return false;
	}
	// add fire effect like in 55i
	if (g_Cfg.m_iSmeltOreOnDclick)
	{
		CItem* pItemEffect = CItem::CreateBase(ITEMID_FIRE);
		ASSERT(pItemEffect);
		CPointMap pt = m_Act_p;
		pt.m_z += 8;	// on top of the forge.
		pItemEffect->SetAttr(ATTR_MOVE_NEVER);
		pItemEffect->MoveToDecay(pt, TICK_PER_SEC);
		if (!g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_NOSFX))
			Sound(0x2b);
	}


	UpdateDir(m_Act_p);

	if ( !Skill_UseQuick(SKILL_MINING, pIngotDef->m_ttIngot.m_iSkillReq / 10) )
	{
		pOre->ConsumeAmount(maximum(1, (pOre->GetAmount() / 2)));
		SysMessageDefault(DEFMSG_SMELT_FAIL);
		return false;
	}

	CItem *pIngot = CItem::CreateScript(pIngotDef->GetID(), this);
	if ( !pIngot )
	{
		SysMessageDefault(DEFMSG_SMELT_CANT);
		return false;
	}

	pOre->Delete();
	pIngot->SetAmount(pOre->GetAmount());
	ItemBounce(pIngot, false);
	SysMessageDefault(DEFMSG_SMELT_SUCCESS);
	return true;
}

bool CChar::Skill_SmeltItem(CItem *pItem)
{
	ADDTOCALLSTACK("CChar::Skill_SmeltItem");
	// Smelt armors/weapons into ingots
	if ( !pItem )
		return false;

	m_Act_p = g_World.FindItemTypeNearby(GetTopPoint(), IT_FORGE, 2, false, true);
	if ( !m_Act_p.IsValidPoint() || !CanTouch(m_Act_p) )
	{
		SysMessageDefault(DEFMSG_SMELT_NOFORGE);
		return false;
	}

	m_Act_p = g_World.FindItemTypeNearby(GetTopPoint(), IT_ANVIL, 2, false, true);
	if ( !m_Act_p.IsValidPoint() || !CanTouch(m_Act_p) )
	{
		SysMessageDefault(DEFMSG_SMELT_NOANVIL);
		return false;
	}

	const CItemBase *pItemDef = pItem->Item_GetDef();
	const CItemBase *pIngotDef = NULL;
	WORD wAmount = 0;
	for ( size_t i = 0; i < pItemDef->m_BaseResources.GetCount(); ++i )
	{
		RESOURCE_ID rid = pItemDef->m_BaseResources[i].GetResourceID();
		if ( rid.GetResType() != RES_ITEMDEF )
			continue;

		const CItemBase *pResourceDef = CItemBase::FindItemBase(static_cast<ITEMID_TYPE>(rid.GetResIndex()));
		if ( !pResourceDef || !pResourceDef->IsType(IT_INGOT) )
			continue;

		pIngotDef = pResourceDef;
		wAmount = static_cast<WORD>((pItemDef->m_BaseResources[i].GetResQty() * 2) / 3);
		break;
	}

	if ( !pIngotDef || pItemDef->IsType(IT_INGOT) )
	{
		SysMessageDefault(DEFMSG_SMELT_CANT);
		return false;
	}

	if ( Skill_GetAdjusted(SKILL_BLACKSMITHING) < static_cast<WORD>(pIngotDef->m_ttIngot.m_iSkillReq) )
	{
		SysMessageDefault(DEFMSG_BLACKSMITHING_NOSKILL);
		return false;
	}

	UpdateDir(m_Act_p);

	CItem *pIngot = CItem::CreateScript(pIngotDef->GetID(), this);
	if ( !pIngot )
	{
		SysMessageDefault(DEFMSG_SMELT_CANT);
		return false;
	}

	pItem->Delete();
	pIngot->SetAmount(pItem->GetAmount() * wAmount);
	ItemBounce(pIngot, false);
	Sound(SOUND_DRIP3);
	Sound(SOUND_LIQUID);
	SysMessageDefault(DEFMSG_SMELT_ITEM_SUCCESS);
	return true;
}

bool CChar::Skill_Tracking(CGrayUID uidTarg, int iDistMax)
{
	ADDTOCALLSTACK("CChar::Skill_Tracking");
	// SKILL_TRACKING

	if ( !m_pClient )		// abort action if the client get disconnected
		return false;

	const CObjBaseTemplate *pObj = uidTarg.ObjFind();
	if ( !pObj )
		return false;

	const CChar *pChar = uidTarg.CharFind();
	if ( !pChar )
		return false;

	int iDist = GetTopDist3D(pObj);	// disconnected = SHRT_MAX
	if ( iDist > iDistMax )
		return false;

	// Prevent track hidden GMs
	if ( pChar->IsStatFlag(STATF_Insubstantial) && (pChar->GetPrivLevel() > GetPrivLevel()) )
		return false;

	DIR_TYPE dir = GetDir(pObj);
	ASSERT((dir > DIR_INVALID) && (static_cast<size_t>(dir) < COUNTOF(CPointBase::sm_szDirs)));

	// Select tracking message based on distance
	LPCTSTR pszDef;
	if ( iDist <= 0 )
		pszDef = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_RESULT_0);
	else if ( iDist < 16 )
		pszDef = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_RESULT_1);
	else if ( iDist < 32 )
		pszDef = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_RESULT_2);
	else if ( iDist < 100 )
		pszDef = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_RESULT_3);
	else
		pszDef = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_RESULT_4);

	if ( pszDef[0] )
	{
		TCHAR *pszMsg = Str_GetTemp();
		sprintf(pszMsg, pszDef, pChar->GetName(), pChar->IsDisconnected() ? g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_RESULT_DISC) : CPointBase::sm_szDirs[dir]);
		ObjMessage(pszMsg, this);
	}
	return true;		// keep the skill active
}

///////////////////////////////////////////////////////////
// Skill handlers

int CChar::Skill_Tracking(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Tracking");
	// SKILL_TRACKING
	// m_Act_Targ = what am i tracking ?

	if ( stage == SKTRIG_START )
		return 0;	// already checked difficulty earlier

	if ( stage == SKTRIG_FAIL )
	{
		// This skill didn't fail, it just ended/went out of range, etc...
		ObjMessage(g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_END), this);		// say this instead of the failure message
		return -SKTRIG_ABORT;
	}

	if ( stage == SKTRIG_STROKE )
	{
		if ( Skill_Stroke(false) == -SKTRIG_ABORT )
			return -SKTRIG_ABORT;

		WORD wSkillLevel = Skill_GetAdjusted(SKILL_TRACKING);
		if ( (g_Cfg.m_iRacialFlags & RACIALF_HUMAN_JACKOFTRADES) && IsHuman() )
			wSkillLevel = maximum(wSkillLevel, 200);			// humans always have a 20.0 minimum skill (racial traits)

		if ( !Skill_Tracking(m_Act_Targ, wSkillLevel / 10 + 10) )
			return -SKTRIG_ABORT;

		Skill_SetTimeout();			// next update
		return -SKTRIG_STROKE;		// keep it active
	}

	return -SKTRIG_ABORT;
}

int CChar::Skill_Mining(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Mining");
	// SKILL_MINING
	// m_Act_p = target point to look for resources
	// m_Act_TargPrv = tool item (pickaxe / shovel)
	//
	// RETURN:
	//  Difficulty = 0-100

	if ( stage == SKTRIG_FAIL )
		return 0;

	if ( m_Act_p.m_x == -1 )
	{
		SysMessageDefault(DEFMSG_MINING_NORESOURCE);
		return -SKTRIG_QTY;
	}

	// Distance check
	CSkillDef *pSkillDef = g_Cfg.GetSkillDef(SKILL_MINING);
	if ( pSkillDef->m_Range <= 0 )
		pSkillDef->m_Range = 2;

	int iTargRange = GetTopPoint().GetDist(m_Act_p);
	int iMaxRange = pSkillDef->m_Range;
	if ( (iTargRange < 1) && !g_Cfg.IsSkillFlag(SKILL_MINING, SKF_NOMINDIST) )
	{
		SysMessageDefault(DEFMSG_MINING_CLOSE);
		return -SKTRIG_QTY;
	}
	if ( iTargRange > iMaxRange )
	{
		SysMessageDefault(DEFMSG_MINING_REACH);
		return -SKTRIG_QTY;
	}
	if ( !CanSeeLOS(m_Act_p, NULL, iMaxRange) )
	{
		SysMessageDefault(DEFMSG_MINING_LOS);
		return -SKTRIG_QTY;
	}

	// Resource check
	CItem *pResBit = g_World.CheckNaturalResource(m_Act_p, static_cast<IT_TYPE>(GETINTRESOURCE(m_atResource.m_ridType)), (stage == SKTRIG_START), this);
	if ( !pResBit || (pResBit->GetAmount() == 0) )
	{
		SysMessageDefault(DEFMSG_MINING_NORESOURCE);
		return -SKTRIG_QTY;
	}

	if ( stage == SKTRIG_START )
	{
		m_atResource.m_Stroke_Count = static_cast<WORD>(Calc_GetRandVal(5) + 2);
		return Skill_NaturalResource_Setup(pResBit);
	}

	// Create the item
	CItem *pItem = Skill_NaturalResource_Create(pResBit, SKILL_MINING);
	if ( !pItem )
	{
		SysMessageDefault(DEFMSG_MINING_NORESOURCE);
		return -SKTRIG_FAIL;
	}

	SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MINING_SUCCESS), pItem->GetName());
	if ( m_atResource.m_bounceItem )
		ItemBounce(pItem, false);
	else
		pItem->MoveToCheck(GetTopPoint(), this, true);
	return 0;
}

int CChar::Skill_Fishing(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Fishing");
	// SKILL_FISHING
	// m_Act_p = target point to look for resources
	// m_Act_TargPrv = tool item (fishing pole / fishing net)
	//
	// RETURN:
	//  Difficulty = 0-100

	if ( stage == SKTRIG_FAIL )
		return 0;

	if ( (m_Act_p.m_x == -1) || m_Act_p.GetRegion(REGION_TYPE_MULTI) )		// do not allow fishing through ship floor
	{
		SysMessageDefault(DEFMSG_FISHING_NOWATER);
		return -SKTRIG_QTY;
	}

	// Distance check
	CSkillDef *pSkillDef = g_Cfg.GetSkillDef(SKILL_FISHING);
	if ( pSkillDef->m_Range <= 0 )
		pSkillDef->m_Range = 4;

	int iTargRange = GetTopPoint().GetDist(m_Act_p);
	int iMaxRange = pSkillDef->m_Range;
	if ( (iTargRange < 1) && !g_Cfg.IsSkillFlag(SKILL_FISHING, SKF_NOMINDIST) )
	{
		SysMessageDefault(DEFMSG_FISHING_CLOSE);
		return -SKTRIG_QTY;
	}
	if ( iTargRange > iMaxRange )
	{
		SysMessageDefault(DEFMSG_FISHING_REACH);
		return -SKTRIG_QTY;
	}
	if ( (m_pPlayer && (g_Cfg.m_iAdvancedLos & ADVANCEDLOS_PLAYER)) || (m_pNPC && (g_Cfg.m_iAdvancedLos & ADVANCEDLOS_NPC)) )
	{
		// Only check LOS when AdvancedLOS is enabled, because standard LOS will always fail (it doesn't check LOS over water)
		if ( !CanSeeLOS(m_Act_p, NULL, iMaxRange, LOS_FISHING) )
		{
			SysMessageDefault(DEFMSG_FISHING_LOS);
			return -SKTRIG_QTY;
		}
	}

	// Resource check
	CItem *pResBit = g_World.CheckNaturalResource(m_Act_p, static_cast<IT_TYPE>(GETINTRESOURCE(m_atResource.m_ridType)), (stage == SKTRIG_START), this);
	if ( !pResBit || (pResBit->GetAmount() == 0) )
	{
		SysMessageDefault(DEFMSG_FISHING_NORESOURCE);
		return -SKTRIG_QTY;
	}

	if ( stage == SKTRIG_START )
	{
		m_atResource.m_Stroke_Count = 1;
		m_Act_Targ = pResBit->GetUID();
		return Skill_NaturalResource_Setup(pResBit);
	}

	// Create the item
	CItem *pItem = Skill_NaturalResource_Create(pResBit, SKILL_FISHING);
	if ( !pItem )
	{
		SysMessageDefault(DEFMSG_FISHING_NORESOURCE);
		return -SKTRIG_FAIL;
	}

	SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_FISHING_SUCCESS), pItem->GetName());
	if ( m_atResource.m_bounceItem )
		ItemBounce(pItem, false);
	else
		pItem->MoveToCheck(GetTopPoint(), this, true);
	return 0;
}

int CChar::Skill_Lumberjack(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Lumberjack");
	// SKILL_LUMBERJACK
	// m_Act_p = target point to look for resources
	// m_Act_TargPrv = tool item (axe / dagger)
	//
	// RETURN:
	//  Difficulty = 0-100

	if ( stage == SKTRIG_FAIL )
		return 0;

	if ( m_Act_p.m_x == -1 )
	{
		SysMessageDefault(DEFMSG_LUMBERJACKING_NORESOURCE);
		return -SKTRIG_QTY;
	}

	// Distance check
	CSkillDef *pSkillDef = g_Cfg.GetSkillDef(SKILL_MINING);
	if ( pSkillDef->m_Range <= 0 )
		pSkillDef->m_Range = 2;

	int iTargRange = GetTopPoint().GetDist(m_Act_p);
	int iMaxRange = pSkillDef->m_Range;
	if ( (iTargRange < 1) && !g_Cfg.IsSkillFlag(SKILL_LUMBERJACKING, SKF_NOMINDIST) )
	{
		SysMessageDefault(DEFMSG_LUMBERJACKING_CLOSE);
		return -SKTRIG_QTY;
	}
	if ( iTargRange > iMaxRange )
	{
		SysMessageDefault(DEFMSG_LUMBERJACKING_REACH);
		return -SKTRIG_QTY;
	}
	if ( !CanSeeLOS(m_Act_p, NULL, iMaxRange) )
	{
		SysMessageDefault(DEFMSG_LUMBERJACKING_LOS);
		return -SKTRIG_QTY;
	}

	// Resource check
	CItem *pResBit = g_World.CheckNaturalResource(m_Act_p, static_cast<IT_TYPE>(GETINTRESOURCE(m_atResource.m_ridType)), (stage == SKTRIG_START), this);
	if ( !pResBit || (pResBit->GetAmount() == 0) )
	{
		SysMessageDefault(DEFMSG_LUMBERJACKING_NORESOURCE);
		return -SKTRIG_QTY;
	}

	if ( stage == SKTRIG_START )
	{
		m_atResource.m_Stroke_Count = static_cast<WORD>(Calc_GetRandVal(5) + 2);
		return Skill_NaturalResource_Setup(pResBit);
	}

	// Create the item
	CItem *pItem = NULL;
	CItem *pTool = m_Act_TargPrv.ItemFind();
	if ( pTool && pTool->IsType(IT_WEAPON_FENCE) )
	{
		pItem = CItem::CreateScript(ITEMID_KINDLING1, this);
		if ( !pItem )
		{
			SysMessageDefault(DEFMSG_LUMBERJACKING_NORESOURCE);
			return -SKTRIG_FAIL;
		}
		pResBit->ConsumeAmount();
		SysMessageDefault(DEFMSG_LUMBERJACKING_KINDLING_SUCCESS);
		SysMessageDefault(DEFMSG_LUMBERJACKING_KINDLING_AXE);
	}
	else
	{
		pItem = Skill_NaturalResource_Create(pResBit, SKILL_LUMBERJACKING);
		if ( !pItem )
		{
			SysMessageDefault(DEFMSG_LUMBERJACKING_NORESOURCE);
			return -SKTRIG_FAIL;
		}
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_LUMBERJACKING_SUCCESS), pItem->GetName());
	}

	if ( m_atResource.m_bounceItem )
		ItemBounce(pItem, false);
	else
		pItem->MoveToCheck(GetTopPoint(), this, true);
	return 0;
}

int CChar::Skill_DetectHidden(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_DetectHidden");
	// SKILL_DETECTINGHIDDEN
	// Look around for who is hiding.
	// Detect them based on skill diff.
	// ??? Hidden objects ?

	if ( stage == SKTRIG_START )
		return 10;		// based on who is hiding ?
	if ( (stage == SKTRIG_FAIL) || (stage == SKTRIG_STROKE) )
		return 0;

	if ( stage != SKTRIG_SUCCESS )
	{
		ASSERT(0);
		return -SKTRIG_QTY;
	}

	if ( !(g_Cfg.m_iRevealFlags & REVEALF_DETECTINGHIDDEN) )	// skill succeeded, but effect is disabled
		return 0;

	WORD wSkillLevel = Skill_GetAdjusted(SKILL_DETECTINGHIDDEN);
	int iRadius = wSkillLevel / 100;

	CWorldSearch Area(GetTopPoint(), iRadius);
	bool fFound = false;
	for (;;)
	{
		CChar *pChar = Area.GetChar();
		if ( !pChar )
			break;
		if ( (pChar == this) || !pChar->IsStatFlag(STATF_Invisible|STATF_Hidden) )
			continue;

		// Check chance to reveal the target
		WORD wSkillSrc = wSkillLevel + static_cast<WORD>(Calc_GetRandVal(210)) - 100;
		WORD wSkillTarg = pChar->Skill_GetAdjusted(SKILL_HIDING) + static_cast<WORD>(Calc_GetRandVal(210) - 100);
		if ( wSkillSrc < wSkillTarg )
			continue;

		pChar->Reveal();
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_DETECTHIDDEN_SUCC), pChar->GetName());
		fFound = true;
	}

	if ( !fFound )
		return -SKTRIG_FAIL;
	return 0;
}

int CChar::Skill_Musicianship(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Musicianship");
	// m_Act_Targ = the intrument i targetted to play.

	if ( stage == SKTRIG_STROKE )
		return 0;
	if ( stage == SKTRIG_START )
		return Use_PlayMusic(m_Act_Targ.ItemFind(), Calc_GetRandVal(90));	// no instrument fail immediate
	return 0;
}

int CChar::Skill_Peacemaking(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Peacemaking");
	// try to make all those listening peacable.
	// General area effect.
	// make peace if possible. depends on who is listening/fighting.

	if ( stage == SKTRIG_STROKE )
		return 0;

	switch ( stage )
	{
		case SKTRIG_START:
		{
			// Basic skill check.
			int iDifficulty = Use_PlayMusic(NULL, Calc_GetRandVal(40));
			if ( iDifficulty < -1 )	// no instrument fail immediate
				return -SKTRIG_FAIL;
			if ( !iDifficulty )
				iDifficulty = Calc_GetRandVal(40);	// Depend on evil of the creatures here.
			return iDifficulty;
		}

		case SKTRIG_FAIL:
			return 0;

		case SKTRIG_SUCCESS:
		{
			WORD wSkillLevel = Skill_GetAdjusted(SKILL_PEACEMAKING);
			int iRadius = (wSkillLevel / 100) + 2;	// 2..12
			CWorldSearch Area(GetTopPoint(), iRadius);
			for (;;)
			{
				CChar *pChar = Area.GetChar();
				if ( !pChar )
					return -SKTRIG_FAIL;
				if ( (pChar == this) || !CanSee(pChar) )
					continue;

				if ( pChar->Skill_GetAdjusted(SKILL_PEACEMAKING) > wSkillLevel )
					SysMessagef("%s %s.", pChar->GetName(), g_Cfg.GetDefaultMsg(DEFMSG_PEACEMAKING_IGNORE));
				else if ( pChar->Skill_GetAdjusted(SKILL_PROVOCATION) > wSkillLevel )
				{
					SysMessagef("%s %s.", pChar->GetName(), g_Cfg.GetDefaultMsg(DEFMSG_PEACEMAKING_DISOBEY));
					if ( pChar->Noto_IsEvil() )
						pChar->Fight_Attack(this);
				}
				else
					pChar->Fight_Clear();

				break;
			}
			return 0;
		}

		default:
			break;
	}
	return -SKTRIG_QTY;
}

int CChar::Skill_Enticement(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Enticement");
	// m_Act_Targ = my target
	// Just keep playing and trying to allure them til we can't
	// Must have a musical instrument.

	if ( stage == SKTRIG_STROKE )
		return 0;

	CChar *pChar = m_Act_Targ.CharFind();
	if ( !pChar || !CanSee(pChar) )
		return -SKTRIG_QTY;

	switch ( stage )
	{
		case SKTRIG_START:
		{
			// Basic skill check.
			int iDifficulty = Use_PlayMusic(NULL, Calc_GetRandVal(40));
			if ( iDifficulty < -1 )	// no instrument fail immediate
				return -SKTRIG_FAIL;
			if ( !iDifficulty )
				iDifficulty = Calc_GetRandVal(40);	// Depend on evil of the creatures here.
			return iDifficulty;
		}

		case SKTRIG_FAIL:
			return 0;

		case SKTRIG_SUCCESS:
		{
			if ( pChar->m_pPlayer )
			{
				SysMessageDefault(DEFMSG_ENTICEMENT_PLAYER);
				return -SKTRIG_ABORT;
			}
			else if ( pChar->IsStatFlag(STATF_War) )
			{
				SysMessagef("%s %s.", pChar->GetName(), g_Cfg.GetDefaultMsg(DEFMSG_ENTICEMENT_BATTLE));
				return -SKTRIG_ABORT;
			}

			pChar->m_Act_p = GetTopPoint();
			pChar->NPC_WalkToPoint((pChar->m_Act_p.GetDist(pChar->GetTopPoint()) > 3));
			return 0;
		}

		default:
			break;
	}
	return -SKTRIG_QTY;
}

int CChar::Skill_Provocation(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Provocation");
	// m_Act_TargPrv = provoke this person
	// m_Act_Targ = against this person.

	if ( stage == SKTRIG_STROKE )
		return 0;

	CChar *pCharProv = m_Act_TargPrv.CharFind();
	CChar *pCharTarg = m_Act_Targ.CharFind();

	if ( !pCharProv || !pCharTarg || (pCharProv == this) || (pCharTarg == this) || (pCharProv == pCharTarg) || pCharProv->IsStatFlag(STATF_Pet|STATF_Conjured|STATF_Stone|STATF_DEAD|STATF_INVUL) || pCharTarg->IsStatFlag(STATF_Pet|STATF_Conjured|STATF_Stone|STATF_DEAD|STATF_INVUL) )
	{
		SysMessageDefault(DEFMSG_PROVOCATION_UPSET);
		return -SKTRIG_QTY;
	}

	if ( pCharProv->m_pPlayer || pCharTarg->m_pPlayer )
	{
		SysMessageDefault(DEFMSG_PROVOCATION_PLAYER);
		return -SKTRIG_ABORT;
	}

	if ( !CanSee(pCharProv) || !CanSee(pCharTarg) )
		return -SKTRIG_ABORT;

	switch ( stage )
	{
		case SKTRIG_START:
		{
			int iDifficulty = Use_PlayMusic(NULL, Calc_GetRandVal(40));
			if ( iDifficulty < -1 )	// no instrument fail immediate
				return -SKTRIG_ABORT;
			if ( !iDifficulty )
				iDifficulty = pCharProv->Stat_GetAdjusted(STAT_INT);	// Depend on evil of the creature.
			if ( pCharProv->Skill_GetAdjusted(SKILL_PROVOCATION) >= Skill_GetAdjusted(SKILL_PROVOCATION) )	// cannot provoke more experienced provoker
				iDifficulty = 0;
			return iDifficulty;
		}

		case SKTRIG_FAIL:
		{
			pCharProv->Fight_Attack(this);
			return 0;
		}

		case SKTRIG_SUCCESS:
		{
			// They are just too good for this.
			if ( pCharProv->Stat_GetAdjusted(STAT_KARMA) >= Calc_GetRandVal2(1000, 10000) )
			{
				pCharProv->Emote(g_Cfg.GetDefaultMsg(DEFMSG_PROVOCATION_EMOTE_1));
				return -SKTRIG_ABORT;
			}

			pCharProv->Emote(g_Cfg.GetDefaultMsg(DEFMSG_PROVOCATION_EMOTE_2));

			// He realizes that you are the real bad guy as well.
			if ( !pCharTarg->OnAttackedBy(this, true) )
				return -SKTRIG_ABORT;

			pCharProv->Memory_AddObjTypes(this, MEMORY_AGGREIVED|MEMORY_IRRITATEDBY);

			// If out of range we might get attacked ourself.
			if ( (pCharProv->GetTopDist3D(pCharTarg) > UO_MAP_VIEW_SIGHT) || (pCharProv->GetTopDist3D(this) > UO_MAP_VIEW_SIGHT) )
			{
				// Check that only "evil" monsters attack provoker back
				if ( pCharProv->Noto_IsEvil() )
					pCharProv->Fight_Attack(this);

				return -SKTRIG_ABORT;
			}

			// or just the npcs are in the same ally groups so can both attack you
			if ( NPC_GetAllyGroupType(pCharProv->GetDispID()) == NPC_GetAllyGroupType(pCharTarg->GetDispID()) )
			{
				if ( pCharProv->Noto_IsEvil() )
				{
					pCharProv->Fight_Attack(this);
					pCharTarg->Fight_Attack(this);
				}

				return -SKTRIG_ABORT;
			}

			// If we are provoking against a "good" PC/NPC and the provoked NPC/PC is good,
			// we are flagged criminal for it and guards are called.
			if ( pCharProv->Noto_GetFlag(this) == NOTO_GOOD )
			{
				// lose some karma for this.
				CheckCrimeSeen(SKILL_PROVOCATION, NULL, pCharProv, g_Cfg.GetDefaultMsg(DEFMSG_PROVOKING_CRIME));
				return -SKTRIG_ABORT;
			}

			// If we provoke upon a good char we should go criminal for it
			// but skill still succeed.
			if ( pCharTarg->Noto_GetFlag(this) == NOTO_GOOD )
				CheckCrimeSeen(SKILL_PROVOCATION, NULL, pCharTarg, "provoking");

			pCharProv->Fight_Attack(pCharTarg); // Make the actual provoking.
			return 0;
		}

		default:
			break;
	}
	return -SKTRIG_QTY;
}

int CChar::Skill_Poisoning(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Poisoning");
	// Act_TargPrv = poison this weapon/food
	// Act_Targ = with this poison.

	if ( stage == SKTRIG_STROKE )
		return 0;

	CItem *pPoison = m_Act_Targ.ItemFind();
	if ( !pPoison || !pPoison->IsType(IT_POTION) )
		return -SKTRIG_ABORT;
	if ( stage == SKTRIG_START )
		return Calc_GetRandVal(60);
	if ( stage == SKTRIG_FAIL )
		return 0;	// lose the poison sometimes ?
	if ( RES_GET_INDEX(pPoison->m_itPotion.m_Type) != SPELL_Poison )
		return -SKTRIG_ABORT;

	CItem *pItem = m_Act_TargPrv.ItemFind();
	if ( !pItem )
		return -SKTRIG_QTY;

	if ( stage != SKTRIG_SUCCESS )
	{
		ASSERT(0);
		return -SKTRIG_ABORT;
	}

	if ( !g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_NOSFX) )
		Sound(SOUND_RUSTLE);	// powdering.

	switch ( pItem->GetType() )
	{
		case IT_FRUIT:
		case IT_FOOD:
		case IT_FOOD_RAW:
		case IT_MEAT_RAW:
			pItem->m_itFood.m_poison_skill = static_cast<BYTE>(pPoison->m_itPotion.m_skillquality / 10);
			break;
		case IT_WEAPON_MACE_SHARP:
		case IT_WEAPON_SWORD:
		case IT_WEAPON_FENCE:
			pItem->m_itWeapon.m_poison_skill = static_cast<BYTE>(pPoison->m_itPotion.m_skillquality / 10);
			pItem->UpdatePropertyFlag();
			break;
		default:
			SysMessageDefault(DEFMSG_POISONING_WITEM);
			return -SKTRIG_QTY;
	}

	// skill + quality of the poison.
	SysMessageDefault(DEFMSG_POISONING_SUCCESS);
	pPoison->ConsumeAmount();
	return 0;
}

int CChar::Skill_Cooking(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Cooking");
	// m_atCreate.m_ItemID = create this item
	// m_Act_p = the heat source
	// m_Act_Targ = the skill tool

	int iMaxDist = 3;
	if ( stage == SKTRIG_START )
	{
		m_Act_p = g_World.FindItemTypeNearby(GetTopPoint(), IT_FIRE, iMaxDist, false, true);
		if ( !m_Act_p.IsValidPoint() )
		{
			m_Act_p = g_World.FindItemTypeNearby(GetTopPoint(), IT_FORGE, iMaxDist, false, true);
			if ( !m_Act_p.IsValidPoint() )
			{
				m_Act_p = g_World.FindItemTypeNearby(GetTopPoint(), IT_CAMPFIRE, iMaxDist, false, true);
				if ( !m_Act_p.IsValidPoint() )
				{
					SysMessageDefault(DEFMSG_COOKING_FIRE_SOURCE);
					return -SKTRIG_QTY;
				}
			}
		}
		UpdateDir(m_Act_p);	// toward the fire source
	}

	if ( stage == SKTRIG_SUCCESS )
	{
		if ( GetTopPoint().GetDist(m_Act_p) > iMaxDist )
			return -SKTRIG_FAIL;
	}

	return Skill_MakeItem(stage);
}

int CChar::Skill_Taming(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Taming");
	// m_Act_Targ = creature to tame.
	// Check the min required skill for this creature.

	CChar *pChar = m_Act_Targ.CharFind();
	if ( !pChar )
		return -SKTRIG_QTY;

	if ( pChar == this )
	{
		SysMessageDefault(DEFMSG_TAMING_YMASTER);
		return -SKTRIG_QTY;
	}
	if ( pChar->m_pPlayer )
	{
		SysMessageDefault(DEFMSG_TAMING_CANT);
		return -SKTRIG_QTY;
	}

	CSkillDef *pSkillDef = g_Cfg.GetSkillDef(SKILL_TAMING);
	if ( pSkillDef->m_Range <= 0 )
		pSkillDef->m_Range = 7;

	if ( GetTopDist3D(pChar) > pSkillDef->m_Range )
	{
		SysMessageDefault(DEFMSG_TAMING_REACH);
		return -SKTRIG_QTY;
	}
	if ( !CanSeeLOS(pChar) )
	{
		SysMessageDefault(DEFMSG_TAMING_LOS);
		return -SKTRIG_QTY;
	}
	UpdateDir(pChar);

	ASSERT(pChar->m_pNPC);

	WORD wTameBase = pChar->Skill_GetBase(SKILL_TAMING);
	if ( !IsPriv(PRIV_GM) )
	{
		if ( pChar->IsStatFlag(STATF_Pet) )
		{
			SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_TAMING_TAME), pChar->GetName());
			return -SKTRIG_QTY;
		}
		if ( !wTameBase )
		{
			SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_TAMING_TAMED), pChar->GetName());
			return -SKTRIG_QTY;
		}
		if ( IsSetOF(OF_PetSlots) )
		{
			if ( !FollowersUpdate(pChar, pChar->m_FollowerSlots, true) )
			{
				SysMessageDefault(DEFMSG_PETSLOTS_TRY_TAMING);
				return -SKTRIG_QTY;
			}
		}
	}

	if ( stage == SKTRIG_START )
	{
		int iDifficulty = wTameBase / 10;
		if ( pChar->Memory_FindObjTypes(this, MEMORY_FIGHT|MEMORY_HARMEDBY|MEMORY_IRRITATEDBY|MEMORY_AGGREIVED) )	// I've attacked it before ?
			iDifficulty += 50;

		m_atTaming.m_Stroke_Count = static_cast<WORD>(Calc_GetRandVal(4) + 2);
		return iDifficulty;
	}

	if ( stage == SKTRIG_FAIL )
		return 0;

	if ( stage == SKTRIG_STROKE )
	{
		if ( (m_atTaming.m_Stroke_Count <= 0) || IsPriv(PRIV_GM) )
			return 0;

		static LPCTSTR const sm_szTameSpeak[] =
		{
			g_Cfg.GetDefaultMsg(DEFMSG_TAMING_1),
			g_Cfg.GetDefaultMsg(DEFMSG_TAMING_2),
			g_Cfg.GetDefaultMsg(DEFMSG_TAMING_3),
			g_Cfg.GetDefaultMsg(DEFMSG_TAMING_4)
		};

		TCHAR *pszMsg = Str_GetTemp();
		sprintf(pszMsg, sm_szTameSpeak[Calc_GetRandVal(COUNTOF(sm_szTameSpeak))], pChar->GetName());
		Speak(pszMsg);

		// Keep trying and updating the animation
		--m_atTaming.m_Stroke_Count;
		Skill_SetTimeout();
		return -SKTRIG_STROKE;
	}

	ASSERT(stage == SKTRIG_SUCCESS);

	// Check if I tamed it before
	CItemMemory *pMemory = pChar->Memory_FindObjTypes(this, MEMORY_SPEAK);
	if ( pMemory && (pMemory->m_itEqMemory.m_Action == NPC_MEM_ACT_TAMED) )
	{
		// I did, no skill to tame it again
		TCHAR *pszMsg = Str_GetTemp();
		sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_TAMING_REMEMBER), pChar->GetName());
		ObjMessage(pszMsg, pChar);

		pChar->NPC_PetSetOwner(this);
		pChar->m_Act_Targ = GetUID();
		pChar->Skill_Start(NPCACT_FOLLOW_TARG);
		return -SKTRIG_QTY;	// no credit for this.
	}

	pChar->NPC_PetSetOwner(this);
	pChar->m_Act_Targ = GetUID();
	pChar->Skill_Start(NPCACT_FOLLOW_TARG);
	SysMessageDefault(DEFMSG_TAMING_SUCCESS);

	// Create the memory of being tamed to prevent lame macroers
	pMemory = pChar->Memory_AddObjTypes(this, MEMORY_SPEAK);
	if ( pMemory )
		pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_TAMED;

	return 0;
}

int CChar::Skill_Lockpicking(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Lockpicking");
	// m_Act_Targ = the item to be picked.
	// m_Act_TargPrv = The pick.

	if ( stage == SKTRIG_STROKE )
		return 0;

	CItem *pPick = m_Act_TargPrv.ItemFind();
	if ( !pPick || !pPick->IsType(IT_LOCKPICK) )
	{
		SysMessageDefault(DEFMSG_LOCKPICKING_NOPICK);
		return -SKTRIG_QTY;
	}

	CItem *pLock = m_Act_Targ.ItemFind();
	if ( !pLock )
	{
		SysMessageDefault(DEFMSG_LOCKPICKING_WITEM);
		return -SKTRIG_QTY;
	}

	if ( pPick->GetTopLevelObj() != this )	// the pick is gone !
	{
		SysMessageDefault(DEFMSG_LOCKPICKING_PREACH);
		return -SKTRIG_QTY;
	}

	if ( stage == SKTRIG_FAIL )
	{
		pPick->OnTakeDamage(1, this, DAMAGE_HIT_BLUNT);	// damage my pick
		return 0;
	}

	if ( !CanTouch(pLock) )	// we moved too far from the lock
	{
		SysMessageDefault(DEFMSG_LOCKPICKING_REACH);
		return -SKTRIG_QTY;
	}

	if ( stage == SKTRIG_START )
		return pLock->Use_LockPick(this, true, false);

	ASSERT(stage == SKTRIG_SUCCESS);

	if ( pLock->Use_LockPick(this, false, false) < 0 )
		return -SKTRIG_FAIL;

	return 0;
}

int CChar::Skill_Hiding(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Hiding");
	// SKILL_Hiding
	// Skill required varies with terrain and situation ?
	// if we are carrying a light source then this should not work.

	if ( stage == SKTRIG_STROKE )	// we shoud just stay in HIDING skill ?
		return 0;

	if ( stage == SKTRIG_FAIL )
	{
		Reveal();
		return 0;
	}

	if ( stage == SKTRIG_SUCCESS )
	{
		ObjMessage(g_Cfg.GetDefaultMsg(DEFMSG_HIDING_SUCCESS), this);
		StatFlag_Set(STATF_Hidden);
		Reveal(STATF_Invisible);	// clear previous invisibility spell effect (this will not reveal the char because STATF_Hidden still set)
		Update(false);
		if ( m_pClient )
		{
			m_pClient->removeBuff(BI_HIDDEN);
			m_pClient->addBuff(BI_HIDDEN, 1075655, 1075656);
		}
		return 0;
	}

	if ( stage == SKTRIG_START )
	{
		// Make sure I'm not carrying a light ?
		for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
		{
			if ( !CItemBase::IsVisibleLayer(pItem->GetEquipLayer()) )
				continue;
			if ( pItem->Item_GetDef()->Can(CAN_I_LIGHT) )
			{
				SysMessageDefault(DEFMSG_HIDING_TOOLIT);
				return -SKTRIG_QTY;
			}
		}
		return Calc_GetRandVal(70);
	}
	ASSERT(0);
	return -SKTRIG_QTY;
}

int CChar::Skill_Herding(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Herding");
	// m_Act_Targ = move this creature.
	// m_Act_p = move to here.
	// How do I make them move fast ? or with proper speed ???

	if ( stage == SKTRIG_STROKE )
		return 0;

	CChar *pChar = m_Act_Targ.CharFind();
	if ( !pChar )
	{
		SysMessageDefault(DEFMSG_HERDING_LTARG);
		return -SKTRIG_QTY;
	}

	CItem *pCrook = m_Act_TargPrv.ItemFind();
	if ( !pCrook )
	{
		SysMessageDefault(DEFMSG_HERDING_NOCROOK);
		return -SKTRIG_QTY;
	}

	switch ( stage )
	{
		case SKTRIG_START:
		{
			if ( !g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_NOANIM) )
				UpdateAnimate(ANIM_ATTACK_WEAPON);

			int iIntVal = pChar->Stat_GetAdjusted(STAT_INT);
			return Calc_GetRandVal2(iIntVal / 2, iIntVal);
		}

		case SKTRIG_FAIL:
			return 0;

		case SKTRIG_SUCCESS:
		{
			if ( IsPriv(PRIV_GM) )
			{
				if ( pChar->GetPrivLevel() > GetPrivLevel() )
				{
					SysMessagef("%s %s", pChar->GetName(), g_Cfg.GetDefaultMsg(DEFMSG_HERDING_PLAYER));
					return -SKTRIG_ABORT;
				}
				pChar->Spell_Teleport(m_Act_p, true, false);
			}
			else
			{
				if ( !pChar->m_pNPC || (pChar->m_pNPC->m_Brain != NPCBRAIN_ANIMAL) )
				{
					SysMessagef("%s %s", pChar->GetName(), g_Cfg.GetDefaultMsg(DEFMSG_HERDING_PLAYER));
					return -SKTRIG_ABORT;
				}
				pChar->m_Act_p = m_Act_p;
				pChar->Skill_Start(NPCACT_GOTO);
			}

			ObjMessage(g_Cfg.GetDefaultMsg(DEFMSG_HERDING_SUCCESS), pChar);
			return 0;
		}

		default:
			break;
	}
	return -SKTRIG_QTY;
}

int CChar::Skill_SpiritSpeak(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_SpiritSpeak");
	if ( (stage == SKTRIG_FAIL) || (stage == SKTRIG_STROKE) )
		return 0;

	if ( stage == SKTRIG_START )
		return Calc_GetRandVal(90);		// difficulty based on spirits near ?

	if ( stage == SKTRIG_SUCCESS )
	{
		if ( IsStatFlag(STATF_SpiritSpeak) )
			return -SKTRIG_ABORT;
		if ( !g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_NOSFX) )
			Sound(SOUND_SPIRITSPEAK);

		SysMessageDefault(DEFMSG_SPIRITSPEAK_SUCCESS);
		Spell_Effect_Create(SPELL_NONE, LAYER_FLAG_SpiritSpeak, 1, 4 * 60 * TICK_PER_SEC, this);
		return 0;
	}

	ASSERT(0);
	return -SKTRIG_ABORT;
}

int CChar::Skill_Meditation(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Meditation");
	// SKILL_MEDITATION
	// Try to regen your mana even faster than normal.
	// Give experience only when we max out.

	if ( (stage == SKTRIG_FAIL) || (stage == SKTRIG_ABORT) )
	{
		if ( m_pClient )
			m_pClient->removeBuff(BI_ACTIVEMEDITATION);
		return 0;
	}

	if ( stage == SKTRIG_START )
	{
		if ( Stat_GetVal(STAT_INT) >= Stat_GetMax(STAT_INT) )
		{
			SysMessageDefault(DEFMSG_MEDITATION_PEACE_1);
			return -SKTRIG_QTY;
		}

		m_atTaming.m_Stroke_Count = 0;
		SysMessageDefault(DEFMSG_MEDITATION_TRY);
		return Calc_GetRandVal(100);	// how hard to get started ?
	}

	if ( stage == SKTRIG_STROKE )
		return 0;

	if ( stage == SKTRIG_SUCCESS )
	{
		if ( Stat_GetVal(STAT_INT) >= Stat_GetMax(STAT_INT) )
		{
			if ( m_pClient )
				m_pClient->removeBuff(BI_ACTIVEMEDITATION);
			SysMessageDefault(DEFMSG_MEDITATION_PEACE_2);
			return 0;	// only give skill credit now.
		}

		if ( m_atTaming.m_Stroke_Count == 0 )
		{
			if ( m_pClient )
				m_pClient->addBuff(BI_ACTIVEMEDITATION, 1075657, 1075658);
			if ( !g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_NOSFX) )
				Sound(SOUND_SFX6);
		}
		++m_atTaming.m_Stroke_Count;
		UpdateStatVal(STAT_INT, 1);
		Skill_SetTimeout();		// next update (depends on skill)
		return -SKTRIG_STROKE;
	}
	return -SKTRIG_QTY;
}

int CChar::Skill_Healing(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Healing");
	// SKILL_VETERINARY:
	// SKILL_HEALING
	// m_Act_TargPrv = bandages.
	// m_Act_Targ = heal target.
	//
	// should depend on the severity of the wounds ?
	// should be just a fast regen over time ?
	// RETURN:
	//  = -3 = failure.

	if ( stage == SKTRIG_STROKE )
		return 0;

	CItem *pBandage = m_Act_TargPrv.ItemFind();
	if ( !pBandage )
	{
		SysMessageDefault(DEFMSG_HEALING_NOAIDS);
		return -SKTRIG_QTY;
	}
	if ( !pBandage->IsType(IT_BANDAGE) )
	{
		SysMessageDefault(DEFMSG_HEALING_WITEM);
		return -SKTRIG_QTY;
	}

	CObjBase *pObj = m_Act_Targ.ObjFind();
	if ( !CanTouch(pObj) )
	{
		SysMessageDefault(DEFMSG_HEALING_REACH);
		return -SKTRIG_QTY;
	}

	CItemCorpse *pCorpse = NULL;	// resurrect by corpse
	CChar *pChar = m_Act_Targ.CharFind();
	if ( pObj->IsItem() )
	{
		pCorpse = dynamic_cast<CItemCorpse *>(pObj);
		if ( !pCorpse )
		{
			SysMessageDefault(DEFMSG_HEALING_NONCHAR);
			return -SKTRIG_QTY;
		}
		pChar = pCorpse->m_uidLink.CharFind();
	}

	if ( !pChar )
	{
		SysMessageDefault(DEFMSG_HEALING_BEYOND);
		return -SKTRIG_QTY;
	}

	if ( GetDist(pObj) > UO_MAP_DIST_INTERACT )
	{
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_HEALING_TOOFAR), pObj->GetName());
		if ( pChar != this )
			pChar->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_HEALING_ATTEMPT), GetName(), pCorpse ? pCorpse->GetName() : g_Cfg.GetDefaultMsg(DEFMSG_HEALING_WHO));

		return -SKTRIG_QTY;
	}

	if ( pCorpse )
	{
		if ( !pCorpse->IsTopLevel() )
		{
			SysMessageDefault(DEFMSG_HEALING_CORPSEG);
			return -SKTRIG_QTY;
		}

		CRegionBase *pRegion = pCorpse->GetTopPoint().GetRegion(REGION_TYPE_AREA|REGION_TYPE_MULTI);
		if ( !pRegion )
			return -SKTRIG_QTY;

		if ( pRegion->IsFlag(REGION_ANTIMAGIC_ALL|REGION_ANTIMAGIC_RECALL_IN|REGION_ANTIMAGIC_TELEPORT) )
		{
			SysMessageDefault(DEFMSG_HEALING_AM);
			if ( pChar != this )
				pChar->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_HEALING_ATTEMPT), GetName(), pCorpse->GetName());

			return -SKTRIG_QTY;
		}
	}
	else if ( pChar->IsStatFlag(STATF_DEAD) )
	{
		SysMessageDefault(DEFMSG_HEALING_GHOST);
		return -SKTRIG_QTY;
	}

	if ( !pChar->IsStatFlag(STATF_Poisoned|STATF_DEAD) && (pChar->Stat_GetVal(STAT_STR) >= pChar->Stat_GetMax(STAT_STR)) )
	{
		if ( pChar == this )
			SysMessageDefault(DEFMSG_HEALING_HEALTHY);
		else
			SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_HEALING_NONEED), pChar->GetName());

		return -SKTRIG_QTY;
	}

	if ( stage == SKTRIG_FAIL )
	{
		// just consume the bandage on fail and give some credit for trying.
		pBandage->ConsumeAmount();

		if ( pChar != this )
			pChar->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_HEALING_ATTEMPTF), GetName(), pCorpse ? pCorpse->GetName() : g_Cfg.GetDefaultMsg(DEFMSG_HEALING_WHO));

		// Harm the creature ?
		return -SKTRIG_FAIL;
	}

	if ( stage == SKTRIG_START )
	{
		if ( pChar != this )
		{
			TCHAR *pszMsg = Str_GetTemp();
			sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_HEALING_TO), pChar->GetName());
			Emote(pszMsg);
		}
		else
		{
			Emote(g_Cfg.GetDefaultMsg(DEFMSG_HEALING_SELF));
		}
		if ( pCorpse )	// resurrect
			return 85 + Calc_GetRandVal(25);
		if ( pChar->IsStatFlag(STATF_Poisoned) )	// poison level
			return 50 + Calc_GetRandVal(50);

		return Calc_GetRandVal(80);
	}

	ASSERT(stage == SKTRIG_SUCCESS);
	pBandage->ConsumeAmount();

	CItem *pBloodyBandage = CItem::CreateScript(Calc_GetRandVal(2) ? ITEMID_BANDAGES_BLOODY1 : ITEMID_BANDAGES_BLOODY2, this);
	ItemBounce(pBloodyBandage);

	const CSkillDef *pSkillDef = g_Cfg.GetSkillDef(Skill_GetActive());
	if ( !pSkillDef )
		return -SKTRIG_QTY;

	if ( pCorpse )
	{
		pChar->Spell_Resurrection(pCorpse, this);
		return 0;
	}

	WORD wSkillLevel = Skill_GetAdjusted(Skill_GetActive());
	if ( pChar->IsStatFlag(STATF_Poisoned) )
	{
		if ( !SetPoisonCure(wSkillLevel, true) )
			return -SKTRIG_ABORT;

		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_HEALING_CURE_1), (pChar == this) ? g_Cfg.GetDefaultMsg(DEFMSG_HEALING_YOURSELF) : pChar->GetName());
		if ( pChar != this )
			pChar->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_HEALING_CURE_2), GetName());

		return 0;
	}

	// LAYER_FLAG_Bandage
	pChar->UpdateStatVal(STAT_STR, pSkillDef->m_Effect.GetLinear(wSkillLevel));
	return 0;
}

int CChar::Skill_RemoveTrap(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_RemoveTrap");
	// m_Act_Targ = trap
	// Is it a trap ?

	if ( stage == SKTRIG_STROKE )
		return 0;

	CItem *pTrap = m_Act_Targ.ItemFind();
	if ( !pTrap || !pTrap->IsType(IT_TRAP) )
	{
		SysMessageDefault(DEFMSG_REMOVETRAPS_WITEM);
		return -SKTRIG_QTY;
	}
	if ( !CanTouch(pTrap) )
	{
		SysMessageDefault(DEFMSG_REMOVETRAPS_REACH);
		return -SKTRIG_QTY;
	}
	if ( stage == SKTRIG_START )
	{
		// How difficult ?
		return Calc_GetRandVal(95);
	}
	if ( stage == SKTRIG_FAIL )
	{
		Use_Item(pTrap);	// set it off ?
		return 0;
	}
	if ( stage == SKTRIG_SUCCESS )
	{
		pTrap->SetTrapState(IT_TRAP_INACTIVE, ITEMID_NOTHING, 5 * 60);	// disable it
		return 0;
	}
	ASSERT(0);
	return -SKTRIG_QTY;
}

int CChar::Skill_Begging(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Begging");
	// m_Act_Targ = Our begging target..

	CChar *pChar = m_Act_Targ.CharFind();
	if ( !pChar || (pChar == this) )
		return -SKTRIG_QTY;

	if ( stage == SKTRIG_START )
	{
		UpdateAnimate(ANIM_BOW);
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_BEGGING_START), pChar->GetName());
		return pChar->Stat_GetAdjusted(STAT_INT);
	}
	if ( stage == SKTRIG_STROKE )
	{
		if ( m_pNPC )
			return -SKTRIG_STROKE;	// Keep it active.
		return 0;
	}
	if ( stage == SKTRIG_FAIL )
		return 0;	// Might they do something bad ?

	if ( stage == SKTRIG_SUCCESS )
		return 0;	// Now what ? Not sure how to make begging successful. Give something from my inventory ?

	ASSERT(0);
	return -SKTRIG_QTY;
}

int CChar::Skill_Magery(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Magery");
	// SKILL_MAGERY
	//  m_Act_p = location to cast to.
	//  m_Act_TargPrv = the source of the spell.
	//  m_Act_Targ = target for the spell.
	//  m_atMagery.m_Spell = the spell.

	if ( stage == SKTRIG_STROKE )
		return 0;

	if ( stage == SKTRIG_FAIL )
	{
		Spell_CastFail();
		return 0;
	}
	if ( stage == SKTRIG_SUCCESS )
	{
		const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_Spell);
		if ( !pSpellDef )
			return 0;

		if ( m_pClient && IsSetMagicFlags(MAGICF_PRECAST) && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST) )
		{
			m_pClient->Cmd_Skill_Magery(m_atMagery.m_Spell, m_pClient->m_Targ_PrvUID.ObjFind());
			return -SKTRIG_QTY;		// don't increase skill at this point. The client should select a target first.
		}
		else
		{
			if ( !Spell_CastDone() )
				return -SKTRIG_ABORT;
			return 0;
		}
	}
	if ( stage == SKTRIG_START )
		return Spell_CastStart();	// NOTE: this should call SetTimeout();

	ASSERT(0);
	return -SKTRIG_ABORT;
}

int CChar::Skill_Fighting(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Fighting");
	// SKILL_ARCHERY
	// SKILL_SWORDSMANSHIP
	// SKILL_MACEFIGHTING
	// SKILL_FENCING
	// SKILL_WRESTLING
	// SKILL_THROWING

	if ( stage == SKTRIG_START )
	{
		m_atFight.m_Swing_State = WAR_SWING_EQUIPPING;
		INT64 iRemainingDelay = g_World.GetTimeDiff(m_atFight.m_Swing_NextAction);
		if ( (iRemainingDelay < 0) || (iRemainingDelay > 255) )
			iRemainingDelay = 0;

		SetTimeout(iRemainingDelay);
		return g_Cfg.Calc_CombatChanceToHit(this, m_Fight_Targ.CharFind());
	}

	if ( stage == SKTRIG_STROKE )
	{
		// Waited my recoil time, so I'm ready to hit my current target
		if ( !IsStatFlag(STATF_War) )
			return -SKTRIG_ABORT;

		if ( m_atFight.m_Swing_State != WAR_SWING_SWINGING )
			m_atFight.m_Swing_State = WAR_SWING_READY;

		Fight_HitTry();
		return -SKTRIG_STROKE;
	}

	return -SKTRIG_QTY;
}

int CChar::Skill_MakeItem(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_MakeItem");
	// SKILL_ALCHEMY
	// SKILL_BLACKSMITHING
	// SKILL_BOWCRAFT
	// SKILL_CARPENTRY
	// SKILL_CARTOGRAPHY
	// SKILL_COOKING
	// SKILL_INSCRIPTION
	// SKILL_TAILORING
	// SKILL_TINKERING
	//
	// m_Act_Targ = the item we want to be part of this process.
	// m_atCreate.m_ItemID = new item we are making
	// m_atCreate.m_Amount = amount of said item.

	if ( stage == SKTRIG_START )
		return m_Act_Difficulty;	// keep the already set difficulty

	if ( stage == SKTRIG_SUCCESS )
	{
		if ( !Skill_MakeItem(m_atCreate.m_ItemID, m_Act_Targ, SKTRIG_SUCCESS, false, m_atCreate.m_Amount ? m_atCreate.m_Amount : 1) )
			return -SKTRIG_ABORT;
		return 0;
	}
	if ( stage == SKTRIG_FAIL )
	{
		Skill_MakeItem(m_atCreate.m_ItemID, m_Act_Targ, SKTRIG_FAIL);
		return 0;
	}
	ASSERT(0);
	return -SKTRIG_QTY;
}

int CChar::Skill_Blacksmith(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Blacksmith");
	// m_atCreate.m_ItemID = create this item
	// m_Act_p = the anvil.
	// m_Act_Targ = the hammer.

	int iMaxDist = 2;
	if ( stage == SKTRIG_START )
	{
		m_Act_p = g_World.FindItemTypeNearby(GetTopPoint(), IT_FORGE, iMaxDist, false, true);
		if ( !m_Act_p.IsValidPoint() )
		{
			SysMessageDefault(DEFMSG_SMITHING_FORGE);
			return -SKTRIG_QTY;
		}
		UpdateDir(m_Act_p);				// toward the forge
		m_atCreate.m_Stroke_Count = 2;	// + Calc_GetRandVal(4)
	}

	if ( stage == SKTRIG_SUCCESS )
	{
		if ( GetTopPoint().GetDist(m_Act_p) > iMaxDist )
			return -SKTRIG_FAIL;
	}

	return Skill_MakeItem(stage);
}

int CChar::Skill_Carpentry(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Carpentry");
	// m_Act_Targ = the item we want to be part of this process.
	// m_atCreate.m_ItemID = new item we are making
	// m_atCreate.m_Amount = amount of said item.

	if ( stage == SKTRIG_START )
		m_atCreate.m_Stroke_Count = 2;	// + Calc_GetRandVal( 3 )

	return Skill_MakeItem(stage);
}

int CChar::Skill_Scripted(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Scripted");
	if ( (stage == SKTRIG_FAIL) || (stage == SKTRIG_START) || (stage == SKTRIG_STROKE) || (stage == SKTRIG_SUCCESS) )
		return 0;

	return -SKTRIG_QTY;	// something odd
}

int CChar::Skill_Information(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Information");
	// SKILL_ANIMALLORE:
	// SKILL_ARMSLORE:
	// SKILL_ANATOMY:
	// SKILL_ITEMID:
	// SKILL_EVALINT:
	// SKILL_FORENSICS:
	// SKILL_TASTEID:
	// Difficulty should depend on the target item !!!??
	// m_Act_Targ = target.

	if ( !m_pClient )	// purely informational
		return -SKTRIG_QTY;

	if ( (stage == SKTRIG_FAIL) || (stage == SKTRIG_STROKE) )
		return 0;

	SKILL_TYPE skill = Skill_GetActive();
	WORD wSkillLevel = Skill_GetAdjusted(skill);
	if ( stage == SKTRIG_START )
		return m_pClient->OnSkill_Info(skill, m_Act_Targ, wSkillLevel, true);
	if ( stage == SKTRIG_SUCCESS )
		return m_pClient->OnSkill_Info(skill, m_Act_Targ, wSkillLevel, false);

	ASSERT(0);
	return -SKTRIG_QTY;
}

int CChar::Skill_Act_Breath(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Act_Breath");
	// NPCACT_BREATH
	// A Dragon I assume.
	// m_Fight_Targ = my target.

	if ( (stage == SKTRIG_STROKE) || (stage == SKTRIG_FAIL) )
		return 0;

	CChar *pChar = m_Fight_Targ.CharFind();
	if ( !pChar )
		return -SKTRIG_QTY;

	m_Act_p = pChar->GetTopPoint();
	if ( !IsSetCombatFlags(COMBAT_NODIRCHANGE) )
		UpdateDir(m_Act_p);

	if ( stage == SKTRIG_START )
	{
		UpdateStatVal(STAT_DEX, -10);
		if ( !g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_NOANIM) )
			UpdateAnimate(ANIM_MON_Stomp);

		SetTimeout(3 * TICK_PER_SEC);
		return 0;
	}

	ASSERT(stage == SKTRIG_SUCCESS);

	if ( !CanSeeLOS(pChar) )
		return -SKTRIG_QTY;

	CPointMap pt = GetTopPoint();
	if ( pt.GetDist(m_Act_p) > UO_MAP_VIEW_SIGHT )
		m_Act_p.StepLinePath(pt, UO_MAP_VIEW_SIGHT);

	int iDamage = static_cast<int>(GetDefNum("BREATH.DAM"));
	if ( !iDamage )
	{
		iDamage = (Stat_GetVal(STAT_STR) * 5) / 100;
		if ( iDamage < 1 )
			iDamage = 1;
		else if ( iDamage > 200 )
			iDamage = 200;
	}

	HUE_TYPE hue = static_cast<HUE_TYPE>(GetDefNum("BREATH.HUE"));
	ITEMID_TYPE id = static_cast<ITEMID_TYPE>(GetDefNum("BREATH.ANIM"));
	EFFECT_TYPE effect = static_cast<EFFECT_TYPE>(GetDefNum("BREATH.TYPE"));
	if ( !id )
		id = ITEMID_FX_FIRE_BALL;
	if ( !effect )
		effect = EFFECT_BOLT;
	Sound(SOUND_FLAME5);
	pChar->Effect(effect, id, this, 20, 30, false, hue);
	pChar->OnTakeDamage(iDamage, this, DAMAGE_FIRE, 0, 100, 0, 0, 0);
	return 0;
}

int CChar::Skill_Act_Throwing(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Act_Throwing");
	// NPCACT_THROWING
	// m_Fight_Targ = my target.

	if ( stage == SKTRIG_STROKE )
		return 0;

	CChar *pChar = m_Fight_Targ.CharFind();
	if ( !pChar )
		return -SKTRIG_QTY;

	m_Act_p = pChar->GetTopPoint();
	if ( !IsSetCombatFlags(COMBAT_NODIRCHANGE) )
		UpdateDir(m_Act_p);

	if ( stage == SKTRIG_START )
	{
		UpdateStatVal(STAT_DEX, -Calc_GetRandVal2(4, 10));
		if ( !g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_NOANIM) )
			UpdateAnimate(ANIM_MON_Stomp);

		return 0;
	}

	if ( stage != SKTRIG_SUCCESS )
		return -SKTRIG_QTY;

	CPointMap pt = GetTopPoint();
	if ( pt.GetDist(m_Act_p) > UO_MAP_VIEW_SIGHT )
		m_Act_p.StepLinePath(pt, UO_MAP_VIEW_SIGHT);

	SoundChar(CRESND_GETHIT);

	int iDamage = 0;
	CVarDefCont *pDam = GetDefKey("THROWDAM", true);
	if ( pDam )
	{
		INT64 iVal[2];
		size_t iQty = Str_ParseCmds(const_cast<TCHAR *>(pDam->GetValStr()), iVal, COUNTOF(iVal));
		switch ( iQty )
		{
			case 1:
				iDamage = static_cast<int>(iVal[0]);
				break;
			case 2:
				iDamage = static_cast<int>(iVal[0] + Calc_GetRandLLVal(iVal[1] - iVal[0]));
				break;
		}
	}

	ITEMID_TYPE id = ITEMID_NOTHING;
	CVarDefCont *pRock = GetDefKey("THROWOBJ", true);
	if ( pRock )
	{
		LPCTSTR pszRock = pRock->GetValStr();
		RESOURCE_ID_BASE rid = static_cast<RESOURCE_ID_BASE>(g_Cfg.ResourceGetID(RES_ITEMDEF, pszRock));
		id = static_cast<ITEMID_TYPE>(rid.GetResIndex());
		if ( !iDamage )
			iDamage = Stat_GetVal(STAT_DEX) / 4 + Calc_GetRandVal(Stat_GetVal(STAT_DEX) / 4);
	}
	else
	{
		if ( Calc_GetRandVal(3) )
		{
			id = static_cast<ITEMID_TYPE>(ITEMID_ROCK_B_LO + Calc_GetRandVal(ITEMID_ROCK_B_HI - ITEMID_ROCK_B_LO));
			if ( !iDamage )
				iDamage = Stat_GetVal(STAT_DEX) / 4 + Calc_GetRandVal(Stat_GetVal(STAT_DEX) / 4);
		}
		else
		{
			id = static_cast<ITEMID_TYPE>(ITEMID_ROCK_2_LO + Calc_GetRandVal(ITEMID_ROCK_2_HI - ITEMID_ROCK_2_LO));
			if ( !iDamage )
				iDamage = 2 + Calc_GetRandVal(Stat_GetVal(STAT_DEX) / 4);
		}
	}

	if ( id != ITEMID_NOTHING )
	{
		CItem *pItemRock = CItem::CreateScript(id, this);
		if ( pItemRock )
		{
			pItemRock->MoveToCheck(m_Act_p, this, true);
			pItemRock->Effect(EFFECT_BOLT, id, this);
		}
		if ( !Calc_GetRandVal(pChar->GetTopPoint().GetDist(m_Act_p)) )
			pChar->OnTakeDamage(iDamage, this, DAMAGE_HIT_BLUNT);
	}
	return 0;
}

int CChar::Skill_Act_Training(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Act_Training");
	// NPCACT_TRAINING
	// finished some traing maneuver.

	if ( stage == SKTRIG_START )
	{
		SetTimeout(TICK_PER_SEC);
		return 0;
	}
	if ( stage == SKTRIG_STROKE )
		return 0;
	if ( stage != SKTRIG_SUCCESS )
		return -SKTRIG_QTY;

	if ( m_Act_TargPrv == m_uidWeapon )
	{
		CItem *pItem = m_Act_Targ.ItemFind();
		if ( pItem )
		{
			switch ( pItem->GetType() )
			{
				case IT_TRAIN_DUMMY:
					Use_Train_Dummy(pItem, false);
					break;
				case IT_TRAIN_PICKPOCKET:
					Use_Train_PickPocketDip(pItem, false);
					break;
				case IT_ARCHERY_BUTTE:
					Use_Train_ArcheryButte(pItem, false);
					break;
				default:
					break;
			}
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////
// General skill stuff

ANIM_TYPE CChar::Skill_GetAnim(SKILL_TYPE skill)
{
	switch ( skill )
	{
		/*case SKILL_FISHING:	// softcoded
			return ANIM_ATTACK_2H_BASH;*/
		case SKILL_BLACKSMITHING:
			return ANIM_ATTACK_1H_SLASH;
		case SKILL_MINING:
			return ANIM_ATTACK_1H_BASH;
		case SKILL_LUMBERJACKING:
			return ANIM_ATTACK_2H_SLASH;
		default:
			return static_cast<ANIM_TYPE>(-1);
	}
}

SOUND_TYPE CChar::Skill_GetSound(SKILL_TYPE skill)
{
	switch ( skill )
	{
		/*case SKILL_FISHING:	// softcoded
			return 0x364;*/
		case SKILL_ALCHEMY:
			return 0x242;
		case SKILL_TAILORING:
			return 0x248;
		case SKILL_CARTOGRAPHY:
		case SKILL_INSCRIPTION:
			return 0x249;
		case SKILL_BOWCRAFT:
			return 0x055;
		case SKILL_BLACKSMITHING:
			return 0x02a;
		case SKILL_CARPENTRY:
			return 0x23d;
		case SKILL_MINING:
			return Calc_GetRandVal(2) ? 0x125 : 0x126;
		case SKILL_LUMBERJACKING:
			return 0x13e;
		default:
			return SOUND_NONE;
	}
}

int CChar::Skill_Stroke(bool fResource)
{
	// fResource means decreasing m_atResource.m_Stroke_Count instead of m_atCreate.m_Stroke_Count
	SKILL_TYPE skill = Skill_GetActive();
	SOUND_TYPE sound = SOUND_NONE;
	INT64 delay = Skill_GetTimeout();
	ANIM_TYPE anim = ANIM_WALK_UNARM;
	if ( m_atCreate.m_Stroke_Count > 1 )
	{
		if ( !g_Cfg.IsSkillFlag(skill, SKF_NOSFX) )
			sound = Skill_GetSound(skill);
		if ( !g_Cfg.IsSkillFlag(skill, SKF_NOANIM) )
			anim = Skill_GetAnim(skill);
	}

	if ( IsTrigUsed(TRIGGER_SKILLSTROKE) || IsTrigUsed(TRIGGER_STROKE) )
	{
		CScriptTriggerArgs args;
		args.m_VarsLocal.SetNum("Skill", skill);
		args.m_VarsLocal.SetNum("Sound", sound);
		args.m_VarsLocal.SetNum("Delay", delay);
		args.m_VarsLocal.SetNum("Anim", anim);
		args.m_iN1 = 1;	//UpdateDir() ?
		if ( fResource )
			args.m_VarsLocal.SetNum("Strokes", m_atResource.m_Stroke_Count);
		else
			args.m_VarsLocal.SetNum("Strokes", m_atCreate.m_Stroke_Count);

		if ( OnTrigger(CTRIG_SkillStroke, this, &args) == TRIGRET_RET_TRUE )
			return -SKTRIG_ABORT;
		if ( Skill_OnTrigger(skill, SKTRIG_STROKE, &args) == TRIGRET_RET_TRUE )
			return -SKTRIG_ABORT;

		sound = static_cast<SOUND_TYPE>(args.m_VarsLocal.GetKeyNum("Sound"));
		delay = args.m_VarsLocal.GetKeyNum("Delay");
		anim = static_cast<ANIM_TYPE>(args.m_VarsLocal.GetKeyNum("Anim"));

		if ( args.m_iN1 == 1 )
			UpdateDir(m_Act_p);
		if ( fResource )
			m_atResource.m_Stroke_Count = static_cast<WORD>(args.m_VarsLocal.GetKeyNum("Strokes"));
		else
			m_atCreate.m_Stroke_Count = static_cast<WORD>(args.m_VarsLocal.GetKeyNum("Strokes"));
	}

	if ( sound )
		Sound(sound);
	if ( anim )
		UpdateAnimate(anim);	// keep trying and updating the animation

	if ( fResource )
	{
		if ( m_atResource.m_Stroke_Count )
			--m_atResource.m_Stroke_Count;
		if ( m_atResource.m_Stroke_Count < 1 )
			return SKTRIG_SUCCESS;
	}
	else
	{
		if ( m_atCreate.m_Stroke_Count )
			--m_atCreate.m_Stroke_Count;
		if ( m_atCreate.m_Stroke_Count < 1 )
			return SKTRIG_SUCCESS;
	}

	if ( delay < 10 )
		delay = 10;

	//Skill_SetTimeout();	// old behaviour, removed to keep up dynamic delay coming in with the trigger @SkillStroke
	SetTimeout(delay);
	return -SKTRIG_STROKE;	// keep active.
}

int CChar::Skill_Stage(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Stage");
	if ( stage == SKTRIG_STROKE )
	{
		if ( g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_CRAFT) )
			return Skill_Stroke(false);

		if ( g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_GATHER) )
			return Skill_Stroke(true);
	}

	if ( g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_SCRIPTED) )
		return Skill_Scripted(stage);
	else if ( g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_FIGHT) )
		return Skill_Fighting(stage);
	else if ( g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_MAGIC) )
		return Skill_Magery(stage);
	/*else if ( g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_CRAFT ) )
		return Skill_MakeItem(stage);*/
	else switch ( Skill_GetActive() )
	{
		case SKILL_NONE:	// idling.
		case SKILL_PARRYING:
		case SKILL_CAMPING:
		case SKILL_MAGICRESISTANCE:
		case SKILL_TACTICS:
		case SKILL_STEALTH:
			return 0;
		case SKILL_ALCHEMY:
		case SKILL_BOWCRAFT:
		case SKILL_CARTOGRAPHY:
		case SKILL_INSCRIPTION:
		case SKILL_TAILORING:
		case SKILL_TINKERING:
			return Skill_MakeItem(stage);
		case SKILL_ANATOMY:
		case SKILL_ANIMALLORE:
		case SKILL_ITEMID:
		case SKILL_ARMSLORE:
		case SKILL_EVALINT:
		case SKILL_FORENSICS:
		case SKILL_TASTEID:
			return Skill_Information(stage);
		case SKILL_BEGGING:
			return Skill_Begging(stage);
		case SKILL_BLACKSMITHING:
			return Skill_Blacksmith(stage);
		case SKILL_PEACEMAKING:
			return Skill_Peacemaking(stage);
		case SKILL_CARPENTRY:
			return Skill_Carpentry(stage);
		case SKILL_COOKING:
			return Skill_Cooking(stage);
		case SKILL_DETECTINGHIDDEN:
			return Skill_DetectHidden(stage);
		case SKILL_ENTICEMENT:
			return Skill_Enticement(stage);
		case SKILL_HEALING:
		case SKILL_VETERINARY:
			return Skill_Healing(stage);
		case SKILL_FISHING:
			return Skill_Fishing(stage);
		case SKILL_HERDING:
			return Skill_Herding(stage);
		case SKILL_HIDING:
			return Skill_Hiding(stage);
		case SKILL_PROVOCATION:
			return Skill_Provocation(stage);
		case SKILL_LOCKPICKING:
			return Skill_Lockpicking(stage);
		case SKILL_MAGERY:
		case SKILL_NECROMANCY:
		case SKILL_CHIVALRY:
		case SKILL_BUSHIDO:
		case SKILL_NINJITSU:
		case SKILL_SPELLWEAVING:
		case SKILL_MYSTICISM:
			return Skill_Magery(stage);
		case SKILL_SNOOPING:
			return Skill_Snooping(stage);
		case SKILL_MUSICIANSHIP:
			return Skill_Musicianship(stage);
		case SKILL_POISONING:
			return Skill_Poisoning(stage);
		case SKILL_ARCHERY:
		case SKILL_SWORDSMANSHIP:
		case SKILL_MACEFIGHTING:
		case SKILL_FENCING:
		case SKILL_WRESTLING:
		case SKILL_THROWING:
			return Skill_Fighting(stage);
		case SKILL_SPIRITSPEAK:
			return Skill_SpiritSpeak(stage);
		case SKILL_STEALING:
			return Skill_Stealing(stage);
		case SKILL_TAMING:
			return Skill_Taming(stage);
		case SKILL_TRACKING:
			return Skill_Tracking(stage);
		case SKILL_LUMBERJACKING:
			return Skill_Lumberjack(stage);
		case SKILL_MINING:
			return Skill_Mining(stage);
		case SKILL_MEDITATION:
			return Skill_Meditation(stage);
		case SKILL_REMOVETRAP:
			return Skill_RemoveTrap(stage);
		case NPCACT_BREATH:
			return Skill_Act_Breath(stage);
		case NPCACT_THROWING:
			return Skill_Act_Throwing(stage);
		case NPCACT_TRAINING:
			return Skill_Act_Training(stage);
		default:
			if ( !IsSkillBase(Skill_GetActive()) )
			{
				if ( stage == SKTRIG_STROKE )
					return -SKTRIG_STROKE;	// keep these active. (NPC modes)
				return 0;
			}
	}

	SysMessageDefault(DEFMSG_SKILL_NOSKILL);
	return -SKTRIG_QTY;
}

void CChar::Skill_Fail(bool fCancel)
{
	ADDTOCALLSTACK("CChar::Skill_Fail");
	// This is the normal skill check failure.
	// Other types of failure don't come here.
	//
	// ARGS:
	//  fCancel = no credit (otherwise get some credit for having tried)

	SKILL_TYPE skill = Skill_GetActive();
	if ( skill == SKILL_NONE )
		return;

	if ( !IsSkillBase(skill) )
	{
		Skill_Cleanup();
		return;
	}

	if ( m_Act_Difficulty > 0 )
		m_Act_Difficulty = -m_Act_Difficulty;

	if ( !fCancel )
	{
		if ( IsTrigUsed(TRIGGER_SKILLFAIL) )
		{
			if ( Skill_OnCharTrigger(skill, CTRIG_SkillFail) == TRIGRET_RET_TRUE )
				fCancel = true;
		}
		if ( IsTrigUsed(TRIGGER_FAIL) && !fCancel )
		{
			if ( Skill_OnTrigger(skill, SKTRIG_FAIL) == TRIGRET_RET_TRUE )
				fCancel = true;
		}
	}
	else
	{
		if ( IsTrigUsed(TRIGGER_SKILLABORT) )
		{
			if ( Skill_OnCharTrigger(skill, CTRIG_SkillAbort) == TRIGRET_RET_TRUE )
			{
				Skill_Cleanup();
				return;
			}
		}
		if ( IsTrigUsed(TRIGGER_ABORT) )
		{
			if ( Skill_OnTrigger(skill, SKTRIG_ABORT) == TRIGRET_RET_TRUE )
			{
				Skill_Cleanup();
				return;
			}
		}
	}

	if ( Skill_Stage(SKTRIG_FAIL) >= 0 )
	{
		// Get some experience for failure ?
		if ( !fCancel )
			Skill_Experience(skill, m_Act_Difficulty);
	}

	Skill_Cleanup();
}

TRIGRET_TYPE CChar::Skill_OnTrigger(SKILL_TYPE skill, SKTRIG_TYPE stage)
{
	CScriptTriggerArgs pArgs;
	return Skill_OnTrigger(skill, stage, &pArgs);
}

TRIGRET_TYPE CChar::Skill_OnTrigger(SKILL_TYPE skill, SKTRIG_TYPE stage, CScriptTriggerArgs *pArgs)
{
	ADDTOCALLSTACK("CChar::Skill_OnTrigger");
	if ( !IsSkillBase(skill) )
		return TRIGRET_RET_DEFAULT;

	if ( !((stage == SKTRIG_SELECT) || (stage == SKTRIG_GAIN) || (stage == SKTRIG_USEQUICK) || (stage == SKTRIG_WAIT) || (stage == SKTRIG_TARGETCANCEL)) )
		m_Act_SkillCurrent = skill;

	pArgs->m_iN1 = skill;
	if ( g_Cfg.IsSkillFlag(skill, SKF_MAGIC) )
		pArgs->m_VarsLocal.SetNum("spell", m_atMagery.m_Spell, true);

	TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;

	CSkillDef *pSkillDef = g_Cfg.GetSkillDef(skill);
	if ( pSkillDef && pSkillDef->HasTrigger(stage) )
	{
		// RES_SKILL
		CResourceLock s;
		if ( pSkillDef->ResourceLock(s) )
			iRet = CScriptObj::OnTriggerScript(s, CSkillDef::sm_szTrigName[stage], this, pArgs);
	}

	return iRet;
}

TRIGRET_TYPE CChar::Skill_OnCharTrigger(SKILL_TYPE skill, CTRIG_TYPE ctrig)
{
	CScriptTriggerArgs pArgs;
	return Skill_OnCharTrigger(skill, ctrig, &pArgs);
}

TRIGRET_TYPE CChar::Skill_OnCharTrigger(SKILL_TYPE skill, CTRIG_TYPE ctrig, CScriptTriggerArgs *pArgs)
{
	ADDTOCALLSTACK("CChar::Skill_OnCharTrigger");
	if ( !IsSkillBase(skill) )
		return TRIGRET_RET_DEFAULT;

	if ( !((ctrig == CTRIG_SkillSelect) || (ctrig == CTRIG_SkillGain) || (ctrig == CTRIG_SkillUseQuick) || (ctrig == CTRIG_SkillWait) || (ctrig == CTRIG_SkillTargetCancel)) )
		m_Act_SkillCurrent = skill;

	pArgs->m_iN1 = skill;
	if ( g_Cfg.IsSkillFlag(skill, SKF_MAGIC) )
		pArgs->m_VarsLocal.SetNum("spell", m_atMagery.m_Spell, true);

	return OnTrigger(ctrig, this, pArgs);
}

int CChar::Skill_Done()
{
	ADDTOCALLSTACK("CChar::Skill_Done");
	// We just finished using a skill. ASYNC timer expired.
	// m_Act_Skill = the skill.
	// Consume resources that have not already been consumed.
	// Confer the benefits of the skill.
	// calc skill gain based on this.
	//
	// RETURN: Did we succeed or fail ?
	//  0 = success
	//  -SKTRIG_STROKE = stay in skill (stroke)
	//  -SKTRIG_FAIL = we must print the fail msg (credit for trying)
	//  -SKTRIG_ABORT = we must print the fail msg (But get no credit, canceled )
	//  -SKTRIG_QTY = special failure. clean up the skill but say nothing (no credit)

	SKILL_TYPE skill = Skill_GetActive();
	if ( skill == SKILL_NONE )	// we should not be coming here (timer should not have expired)
		return -SKTRIG_QTY;

	// multi stroke tried stuff here first.
	// or stuff that never really fails.
	int iRet = Skill_Stage(SKTRIG_STROKE);
	if ( iRet < 0 )
		return iRet;

	if ( m_Act_Difficulty < 0 )		// was Bound to fail, but we had to wait for the timer anyhow.
		return -SKTRIG_FAIL;

	if ( IsTrigUsed(TRIGGER_SKILLSUCCESS) )
	{
		if ( Skill_OnCharTrigger(skill, CTRIG_SkillSuccess) == TRIGRET_RET_TRUE )
		{
			Skill_Cleanup();
			return -SKTRIG_ABORT;
		}
	}
	if ( IsTrigUsed(TRIGGER_SUCCESS) )
	{
		if ( Skill_OnTrigger(skill, SKTRIG_SUCCESS) == TRIGRET_RET_TRUE )
		{
			Skill_Cleanup();
			return -SKTRIG_ABORT;
		}
	}

	// Success for the skill.
	iRet = Skill_Stage(SKTRIG_SUCCESS);
	if ( iRet < 0 )
		return iRet;

	// Success = Advance the skill
	Skill_Experience(skill, m_Act_Difficulty);
	Skill_Cleanup();

	return -SKTRIG_SUCCESS;
}

bool CChar::Skill_Wait(SKILL_TYPE skilltry)
{
	ADDTOCALLSTACK("CChar::Skill_Wait");
	// Some sort of push button skill.
	// We want to do some new skill. Can we ?
	// If this is the same skill then tell them to wait.

	SKILL_TYPE skill = Skill_GetActive();
	CScriptTriggerArgs pArgs(skilltry, skill);

	if ( IsTrigUsed(TRIGGER_SKILLWAIT) )
	{
		switch ( Skill_OnCharTrigger(skilltry, CTRIG_SkillWait, &pArgs) )
		{
			case TRIGRET_RET_TRUE:
				return true;
			case TRIGRET_RET_FALSE:
				Skill_Fail(true);
				return false;
			default:
				break;
		}
	}
	if ( IsTrigUsed(TRIGGER_WAIT) )
	{
		switch ( Skill_OnTrigger(skilltry, SKTRIG_WAIT, &pArgs) )
		{
			case TRIGRET_RET_TRUE:
				return true;
			case TRIGRET_RET_FALSE:
				Skill_Fail(true);
				return false;
			default:
				break;
		}
	}

	if ( IsStatFlag(STATF_DEAD|STATF_Freeze|STATF_Stone) )
	{
		SysMessageDefault(DEFMSG_SKILLWAIT_1);
		return true;
	}

	if ( skill == SKILL_NONE )	// not currently doing anything.
	{
		if ( skilltry != SKILL_STEALTH )
			Reveal();
		return false;
	}

	if ( IsStatFlag(STATF_War) )
	{
		SysMessageDefault(DEFMSG_SKILLWAIT_2);
		return true;
	}

	// Cancel passive actions
	if ( skilltry != skill )
	{
		if ( (skill == SKILL_MEDITATION) || (skill == SKILL_HIDING) || (skill == SKILL_STEALTH) )		// SKILL_SPIRITSPEAK ?
		{
			Skill_Fail(true);
			return false;
		}
	}

	SysMessageDefault(DEFMSG_SKILLWAIT_3);
	return true;
}

bool CChar::Skill_Start(SKILL_TYPE skill)
{
	ADDTOCALLSTACK("CChar::Skill_Start");
	// We have all the info we need to do the skill. (targeting etc)
	// Set up how long we have to wait before we get the desired results from this skill.
	// Set up any animations/sounds in the mean time.
	// Calc if we will succeed or fail.
	// RETURN:
	//  false = failed outright with no wait. "You have no chance of taming this"

	if ( g_Serv.IsLoading() )
	{
		if ( (skill != SKILL_NONE) && !IsSkillBase(skill) && !IsSkillNPC(skill) )
		{
			DEBUG_ERR(("UID:0%lx Bad Skill %d for '%s'\n", static_cast<DWORD>(GetUID()), skill, GetName()));
			return false;
		}
		m_Act_SkillCurrent = skill;
		return true;
	}

	if ( !Skill_CanUse(skill) )
		return false;

	SKILL_TYPE skillPrev = Skill_GetActive();
	if ( skillPrev != SKILL_NONE )
		Skill_Fail(true);		// fail previous skill unfinished. (with NO skill gain!)

	if ( skill != SKILL_NONE )
	{
		if ( IsTrigUsed(TRIGGER_SKILLPRESTART) )
		{
			if ( Skill_OnCharTrigger(skill, CTRIG_SkillPreStart) == TRIGRET_RET_TRUE )
				return false;
		}
		if ( IsTrigUsed(TRIGGER_PRESTART) )
		{
			if ( Skill_OnTrigger(skill, SKTRIG_PRESTART) == TRIGRET_RET_TRUE )
				return false;
		}

		m_Act_SkillCurrent = skill;
		m_Act_Difficulty = Skill_Stage(SKTRIG_START);

		CScriptTriggerArgs pArgs;
		bool fCraftSkill = g_Cfg.IsSkillFlag(skill, SKF_CRAFT);
		bool fGatherSkill = g_Cfg.IsSkillFlag(skill, SKF_GATHER);
		RESOURCE_ID pResBase(RES_ITEMDEF, fCraftSkill ? m_atCreate.m_ItemID : 0, 0);

		if ( fCraftSkill )
		{
			m_atCreate.m_Stroke_Count = 1;
			pArgs.m_VarsLocal.SetNum("CraftItemdef", pResBase.GetPrivateUID());
			pArgs.m_VarsLocal.SetNum("CraftStrokeCnt", m_atCreate.m_Stroke_Count);
			pArgs.m_VarsLocal.SetNum("CraftAmount", m_atCreate.m_Amount);
		}
		if ( fGatherSkill )
		{
			m_atResource.m_bounceItem = 1;
			pArgs.m_VarsLocal.SetNum("GatherStrokeCnt", m_atResource.m_Stroke_Count);
		}

		if ( IsTrigUsed(TRIGGER_SKILLSTART) )
		{
			if ( (Skill_OnCharTrigger(skill, CTRIG_SkillStart, &pArgs) == TRIGRET_RET_TRUE) || (m_Act_Difficulty < 0) )
			{
				Skill_Cleanup();
				return false;
			}
		}
		if ( IsTrigUsed(TRIGGER_START) )
		{
			if ( (Skill_OnTrigger(skill, SKTRIG_START, &pArgs) == TRIGRET_RET_TRUE) || (m_Act_Difficulty < 0) )
			{
				Skill_Cleanup();
				return false;
			}
		}

		if ( fCraftSkill )
		{
			// read crafting parameters
			pResBase.SetPrivateUID(static_cast<DWORD>(pArgs.m_VarsLocal.GetKeyNum("CraftItemdef")));
			m_atCreate.m_Stroke_Count = static_cast<WORD>(maximum(1, pArgs.m_VarsLocal.GetKeyNum("CraftStrokeCnt")));
			m_atCreate.m_ItemID = static_cast<ITEMID_TYPE>(pResBase.GetResIndex());
			m_atCreate.m_Amount = static_cast<WORD>(pArgs.m_VarsLocal.GetKeyNum("CraftAmount"));
		}
		if ( fGatherSkill )
			m_atResource.m_Stroke_Count = static_cast<WORD>(pArgs.m_VarsLocal.GetKeyNum("GatherStrokeCnt"));

		// Casting sound & animation when starting, Skill_Stroke() will do it the next times.
		if ( fCraftSkill || fGatherSkill )
		{
			if ( !g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_NOSFX) )
				Sound(Skill_GetSound(Skill_GetActive()));

			if ( !g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_NOANIM) )
				UpdateAnimate(Skill_GetAnim(Skill_GetActive()));
		}

		if ( IsSkillBase(skill) )
		{
			const CSkillDef *pSkillDef = g_Cfg.GetSkillDef(skill);
			if ( pSkillDef )
			{
				int iWaitTime = pSkillDef->m_Delay.GetLinear(Skill_GetBase(skill));
				if ( iWaitTime != 0 )
					SetTimeout(iWaitTime);		// How long before complete skill.
			}
		}
		else if ( m_pNPC )
		{
			if ( ((skillPrev != NPCACT_GUARD_TARG) && (Skill_GetActive() == NPCACT_GUARD_TARG)) || ((skillPrev == NPCACT_GUARD_TARG) && (Skill_GetActive() != NPCACT_GUARD_TARG)) )
				UpdatePropertyFlag();
		}

		if ( IsTimerExpired() )
			SetTimeout(1);		// the skill should have set it's own delay!?

		if ( m_Act_Difficulty > 0 )
		{
			if ( !Skill_CheckSuccess(skill, m_Act_Difficulty, !g_Cfg.IsSkillFlag(skill, SKF_FIGHT)) )
				m_Act_Difficulty = -m_Act_Difficulty;	// will result in failure
		}
	}

#ifdef _DEBUG
	if ( IsStatFlag(STATF_EmoteAction) || (g_Cfg.m_wDebugFlags & DEBUGF_NPC_EMOTE) )
		Emote(Skill_GetName(true));
#else
	if ( IsStatFlag(STATF_EmoteAction) )
		Emote(Skill_GetName(true));
#endif
	return true;
}
