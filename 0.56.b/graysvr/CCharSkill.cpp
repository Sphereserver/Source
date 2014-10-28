//
// CCharSkill.cpp
// Copyright Menace Software (www.menasoft.com).
//  CChar is either an NPC or a Player.
//

#include <cmath>
#include "graysvr.h"	// predef header.

//----------------------------------------------------------------------

void CChar::Action_StartSpecial( CREID_TYPE id )
{
	ADDTOCALLSTACK("CChar::Action_StartSpecial");
	// Take the special creature action.
	// lay egg, breath weapon (fire, lightning, acid, code, paralyze),
	//  create web, fire patch, fire ball,
	// steal, teleport, level drain, absorb magic, curse items,
	// rust items, stealing, charge, hiding, grab, regenerate, play dead.
	// Water = put out fire !

	if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOANIM ) )
	{
		UpdateAnimate( ANIM_CAST_AREA );
	}

	switch ( id )
	{
		case CREID_FIRE_ELEM:
			// leave a fire patch.
			{
				CItem * pItem = CItem::CreateScript( Calc_GetRandVal(2) ? ITEMID_FX_FIRE_F_EW : ITEMID_FX_FIRE_F_NS, this );
				ASSERT(pItem);
				pItem->SetType( IT_FIRE );
				pItem->m_itSpell.m_spell = SPELL_Fire_Field;
				pItem->m_itSpell.m_spelllevel = 100 + Calc_GetRandVal(500);
				pItem->m_itSpell.m_spellcharges = 1;
				pItem->m_uidLink = GetUID();	// Link it back to you
				pItem->MoveToDecay( GetTopPoint(), 30*TICK_PER_SEC + Calc_GetRandVal(60*TICK_PER_SEC));
			}
			break;

		case CREID_GIANT_SPIDER:
			// Leave a web patch.
			{
				static const WORD sm_Webs[] =
				{
					ITEMID_WEB1_1,
					ITEMID_WEB1_1 + 1,
					ITEMID_WEB1_1 + 2,
					ITEMID_WEB1_4
				};
				CItem * pItem = CItem::CreateScript( (ITEMID_TYPE) sm_Webs[ Calc_GetRandVal( COUNTOF(sm_Webs))], this );
				pItem->SetType(IT_WEB);
				pItem->MoveToCheck( GetTopPoint(), this );
				pItem->SetDecayTime( 3*60*TICK_PER_SEC );
			}
			break;

		default:
			SysMessage( "You have no special abilities" );
			return;	// No special ability.
	}

	// loss of stamina for a bit.
	UpdateStatVal( STAT_DEX, -( 5 + Calc_GetRandVal(5)));	// The cost in stam.
}


void CChar::Stat_AddMod( STAT_TYPE i, short iVal )
{
	ADDTOCALLSTACK("CChar::Stat_AddMod");
	ASSERT(i >= 0 && i < STAT_QTY);
	m_Stat[i].m_mod	+= iVal;
	UpdateStatsFlag();
}


void CChar::Stat_SetMod( STAT_TYPE i, short iVal )
{
	ADDTOCALLSTACK("CChar::Stat_SetMod");
	ASSERT(i >= 0 && i < STAT_QTY);
	int iStatVal = Stat_GetMod(static_cast<STAT_TYPE>(i));
	if ( IsTrigUsed(TRIGGER_STATCHANGE) )
	{
		if (i >= STAT_STR && i <= STAT_DEX)
		{
			CScriptTriggerArgs args;
			args.m_iN1 = i+8;	// Shift by 8 to indicate modSTR, modINT, modDEX
			args.m_iN2 = iStatVal;
			args.m_iN3 = iVal;
			if (OnTrigger(CTRIG_StatChange, this, &args) == TRIGRET_RET_TRUE)
				return;
			// do not restore argn1 to i, bad things will happen! leave i untouched. (matex)
			iVal = static_cast<short>(args.m_iN3);
		}
	}
	m_Stat[i].m_mod = iVal;
	UpdateStatsFlag();
}

short CChar::Stat_GetMod( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetMod");
	ASSERT(i >= 0 && i < STAT_QTY);
	return m_Stat[i].m_mod;
}

void CChar::Stat_SetVal( STAT_TYPE i, int iVal )
{
	ADDTOCALLSTACK("CChar::Stat_SetVal");
	if (i > STAT_BASE_QTY || i == STAT_FOOD) // Food must trigger Statchange. Redirect to Base value
	{
		Stat_SetBase(i, iVal);
		return;
	}
	ASSERT(i >= 0 && i < STAT_QTY); // allow for food
	m_Stat[i].m_val = iVal;
}

int CChar::Stat_GetVal( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetVal");
	if ( i > STAT_BASE_QTY || i == STAT_FOOD ) // Food must trigger Statchange. Redirect to Base value
		return Stat_GetBase(i);
	ASSERT(i >= 0 && i < STAT_QTY); // allow for food
	return m_Stat[i].m_val;
}


void CChar::Stat_SetMax( STAT_TYPE i, int iVal )
{
	ADDTOCALLSTACK("CChar::Stat_SetMax");
	ASSERT(i >= 0 && i < STAT_QTY); // allow for food

	if ( g_Cfg.m_iStatFlag && (
		( g_Cfg.m_iStatFlag&STAT_FLAG_DENYMAX ) ||
		( m_pPlayer && g_Cfg.m_iStatFlag&STAT_FLAG_DENYMAXP ) ||
		( m_pNPC && g_Cfg.m_iStatFlag&STAT_FLAG_DENYMAXN )
	   ))
	{
		m_Stat[i].m_max = 0;
	}
	else
	{
		int iStatVal = Stat_GetMax(static_cast<STAT_TYPE>(i));
		if ( IsTrigUsed(TRIGGER_STATCHANGE) )
		{
			// Only STR, DEX, INT, FOOD fire MaxHits, MaxMana, MaxStam, MaxFood for @StatChange
			if (i >= STAT_STR && i <= STAT_FOOD)
			{
				CScriptTriggerArgs args;
				args.m_iN1 = i + 4; // Shift by 4 to indicate MaxHits, etc..
				args.m_iN2 = iStatVal;
				args.m_iN3 = iVal;
				if (OnTrigger(CTRIG_StatChange, this, &args) == TRIGRET_RET_TRUE)
					return;
				// do not restore argn1 to i, bad things will happen! leave i untouched. (matex)
				iVal = static_cast<int>(args.m_iN3);
			}
		}
		m_Stat[i].m_max = iVal;
		switch ( i )
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
			default:
				break;
		}
	}
}

int CChar::Stat_GetMax( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetMax");
	int	val;
	ASSERT(i >= 0 && i < STAT_QTY); // allow for food
	if ( m_Stat[i].m_max < 1 )
	{
		if (i == STAT_FOOD)
		{
			CCharBase* pCharDef = Char_GetDef();
			ASSERT(pCharDef);
			val=pCharDef->m_MaxFood;
		}
		else
			val	= Stat_GetAdjusted(i);
		return (val < 0 ? (m_pPlayer ? 1 : 0) : val);
	}
	val	=  m_Stat[i].m_max;
	if ( i >= STAT_BASE_QTY )
		val += m_Stat[i].m_mod;

	if ( val < 0 )
		return ( m_pPlayer ? 1 : 0 );
	return val;
}

int CChar::Stat_GetSum() const
{
	ADDTOCALLSTACK("CChar::Stat_GetSum");
	int iStatSum = 0;

	for ( int i = 0; i < STAT_BASE_QTY; i++ )
	{
		iStatSum += Stat_GetBase(static_cast<STAT_TYPE>(i));
	}

	return( iStatSum );
}

short CChar::Stat_GetAdjusted( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetAdjusted");
	short val = Stat_GetBase(i) + Stat_GetMod(i);
	if ( i == STAT_FAME )
	{
		if ( val < 0 )
			val = 0;
		else if ( val > g_Cfg.m_iMaxFame )
			val = g_Cfg.m_iMaxFame;
	}
	else if ( i == STAT_KARMA )
	{
		if ( val < g_Cfg.m_iMinKarma )
			val = g_Cfg.m_iMinKarma;
		else if ( val > g_Cfg.m_iMaxKarma )
			val = g_Cfg.m_iMaxKarma;
	}
	return val;

}

short CChar::Stat_GetBase( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetBase");
	ASSERT(i >= 0 && i < STAT_QTY);

	if ( i == STAT_FAME && m_Stat[i].m_base < 0 )
		return 0;
	return m_Stat[i].m_base;
}


void CChar::Stat_AddBase( STAT_TYPE i, short iVal )
{
	ADDTOCALLSTACK("CChar::Stat_AddBase");
	Stat_SetBase( i, Stat_GetBase( i ) + iVal );
}

void CChar::Stat_SetBase( STAT_TYPE i, short iVal )
{
	ADDTOCALLSTACK("CChar::Stat_SetBase");
	ASSERT(i >= 0 && i < STAT_QTY);

	int iStatVal = Stat_GetBase(static_cast<STAT_TYPE>(i));
	if ( IsTrigUsed(TRIGGER_STATCHANGE) && ! g_Serv.IsLoading() )
	{
		// Only Str, Dex, Int, Food fire @Statchange here
		if (i >= STAT_STR && i <= STAT_FOOD)
		{
			CScriptTriggerArgs args;
			args.m_iN1 = i;
			args.m_iN2 = iStatVal;
			args.m_iN3 = iVal;
			if (OnTrigger(CTRIG_StatChange, this, &args) == TRIGRET_RET_TRUE)
				return;
			// do not restore argn1 to i, bad things will happen! leave i untouched. (matex)
			iVal = static_cast<short>(args.m_iN3);

			if (i != STAT_FOOD && m_Stat[i].m_max < 1) // MaxFood cannot depend on something, otherwise if the Stat depends on STR, INT, DEX, fire MaxHits, MaxMana, MaxStam
			{
				args.m_iN1 = i+4; // Shift by 4 to indicate MaxHits, MaxMana, MaxStam
				args.m_iN2 = iStatVal;
				args.m_iN3 = iVal;
				if (OnTrigger(CTRIG_StatChange, this, &args) == TRIGRET_RET_TRUE)
					return;
				// do not restore argn1 to i, bad things will happen! leave i untouched. (matex)
				iVal = static_cast<short>(args.m_iN3);
			}
		}
	}
	switch ( i )
	{
		case STAT_STR:
			{
				CCharBase * pCharDef = Char_GetDef();
		
				if ( pCharDef && !pCharDef->m_Str )
					pCharDef->m_Str = iVal;
			}
			break;
		case STAT_DEX:
			{
				CCharBase * pCharDef = Char_GetDef();
		
				if ( pCharDef && !pCharDef->m_Dex )
					pCharDef->m_Dex = iVal;
			}
			break;
		case STAT_INT:
			{
				CCharBase * pCharDef = Char_GetDef();
		
				if ( pCharDef && !pCharDef->m_Int )
					pCharDef->m_Int = iVal;
			}
			break;
		case STAT_KARMA:		// m_iMinKarma to m_iMaxKarma
			iVal = maximum(g_Cfg.m_iMinKarma, minimum(g_Cfg.m_iMaxKarma, iVal));
			break;
		case STAT_FAME:
			if ( iVal < 0 )
			{
				DEBUG_ERR(( "ID=0%x,UID=0%lx Fame set out of range %d\n", GetBaseID(), (DWORD) GetUID(), iVal ));
				iVal = 0;
			}
			break;
		case STAT_FOOD:
			break;
		default:
			throw CGrayError(LOGL_CRIT, 0, "Stat_SetBase: index out of range");
	}
	
	m_Stat[i].m_base = iVal;
	UpdateStatsFlag();
	if ( !g_Serv.IsLoading() && i == STAT_KARMA )
		NotoSave_Update();
}

short CChar::Stat_GetLimit( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetLimit");
	const CVarDefCont * pTagStorage = NULL;
	TemporaryString sStatName;

	if ( m_pPlayer )
	{
		const CSkillClassDef* pSkillClass = m_pPlayer->GetSkillClass();
		ASSERT(pSkillClass);
		if ( i == STAT_QTY )
		{
			pTagStorage = GetKey("OVERRIDE.STATSUM", true);
			return pTagStorage ? (static_cast<short>(pTagStorage->GetValNum())) : pSkillClass->m_StatSumMax;
		}
		ASSERT( i>=0 && i<STAT_BASE_QTY );

		sprintf(sStatName, "OVERRIDE.STATCAP_%d", static_cast<int>(i));
		int iStatMax;
		if ( (pTagStorage = GetKey(sStatName, true)) != NULL )
			iStatMax = static_cast<int>(pTagStorage->GetValNum());
		else
			iStatMax = pSkillClass->m_StatMax[i];

		if ( m_pPlayer->Stat_GetLock(i) >= SKILLLOCK_DOWN )
		{
			int iStatLevel = Stat_GetBase(i);
			if ( iStatLevel < iStatMax )
				iStatMax = iStatLevel;
		}
		return( iStatMax );
	}
	else
	{
		if ( i == STAT_QTY )
		{
			pTagStorage = GetKey("OVERRIDE.STATSUM", true);
			return pTagStorage ? static_cast<short>(pTagStorage->GetValNum()) : 300;
		}

		int iStatMax = 100;
		sprintf(sStatName, "OVERRIDE.STATCAP_%d", static_cast<int>(i));
		if ( (pTagStorage = GetKey(sStatName, true)) != NULL )
			iStatMax = static_cast<int>(pTagStorage->GetValNum());

		return iStatMax;
	}
}

//----------------------------------------------------------------------
// Skills

SKILL_TYPE CChar::Skill_GetBest( unsigned int iRank ) const // Which skill is the highest for character p
{
	ADDTOCALLSTACK("CChar::Skill_GetBest");
	// Get the top n best skills.

	if ( iRank >= g_Cfg.m_iMaxSkill )
		iRank = 0;

	DWORD * pdwSkills = new DWORD [iRank + 1];
	ASSERT(pdwSkills);
	memset(pdwSkills, 0, (iRank + 1) * sizeof(DWORD));

	DWORD dwSkillTmp;
	for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; i++)
	{
		if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(static_cast<SKILL_TYPE>(i)) )
			continue;

		dwSkillTmp = MAKEDWORD(i, Skill_GetBase(static_cast<SKILL_TYPE>(i)));
		for ( size_t j = 0; j <= iRank; j++ )
		{
			if ( HIWORD(dwSkillTmp) >= HIWORD(pdwSkills[j]) )
			{
				memmove( &pdwSkills[j + 1], &pdwSkills[j], (iRank - j) * sizeof(DWORD) );
				pdwSkills[j] = dwSkillTmp;
				break;
			}
		}
	}

	dwSkillTmp = pdwSkills[ iRank ];
	delete[] pdwSkills;
	return static_cast<SKILL_TYPE>(LOWORD( dwSkillTmp ));
}

unsigned short CChar::Skill_GetAdjusted( SKILL_TYPE skill ) const
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

	ASSERT( IsSkillBase( skill ));
	const CSkillDef * pSkillDef = g_Cfg.GetSkillDef( skill );
	int iAdjSkill = 0;

	if (pSkillDef != NULL)
	{
		int iPureBonus = ( pSkillDef->m_StatBonus[STAT_STR] * maximum(0,Stat_GetAdjusted( STAT_STR )) ) +
						 ( pSkillDef->m_StatBonus[STAT_INT] * maximum(0,Stat_GetAdjusted( STAT_INT )) ) +
						 ( pSkillDef->m_StatBonus[STAT_DEX] * maximum(0,Stat_GetAdjusted( STAT_DEX )) );

		iAdjSkill = IMULDIV( pSkillDef->m_StatPercent, iPureBonus, 10000 );
	}

	return( Skill_GetBase(static_cast<SKILL_TYPE>(skill)) + iAdjSkill );
}

void CChar::Skill_SetBase( SKILL_TYPE skill, int iValue )
{
	ADDTOCALLSTACK("CChar::Skill_SetBase");
	ASSERT( IsSkillBase(skill));
	if ( iValue < 0 ) iValue = 0;
	if ( IsTrigUsed(TRIGGER_SKILLCHANGE) )
	{
		CScriptTriggerArgs args;
		args.m_iN1 = static_cast<INT64>(skill);
		args.m_iN2 = iValue;
		if ( OnTrigger(CTRIG_SkillChange, this, &args) == TRIGRET_RET_TRUE )
			return;

		iValue = static_cast<int>(args.m_iN2);
	}
	m_Skill[skill] = iValue;
	if ( IsClient())
	{
		// Update the skills list
		m_pClient->addSkillWindow(skill);
	}
}

int CChar::Skill_GetMax( SKILL_TYPE skill ) const
{
	ADDTOCALLSTACK("CChar::Skill_GetMax");
	const CVarDefCont * pTagStorage = NULL;
	TemporaryString sSkillName;

	// What is my max potential in this skill ?
	if ( m_pPlayer )
	{
		const CSkillClassDef* pSkillClass = m_pPlayer->GetSkillClass();
		ASSERT(pSkillClass);

		if ( skill == SKILL_MAX )
		{
			pTagStorage = GetKey("OVERRIDE.SKILLSUM", true);
			return pTagStorage ? static_cast<int>(pTagStorage->GetValNum()) : pSkillClass->m_SkillSumMax;
		}

		ASSERT( IsSkillBase(skill) );

		sprintf(sSkillName, "OVERRIDE.SKILLCAP_%d", static_cast<int>(skill));
		int iSkillMax;
		if ( (pTagStorage = GetKey(sSkillName, true)) != NULL )
			iSkillMax = static_cast<int>(pTagStorage->GetValNum());
		else
			iSkillMax = pSkillClass->m_SkillLevelMax[skill];
		
		if ( m_pPlayer->Skill_GetLock(skill) >= SKILLLOCK_DOWN )
		{
			int iSkillLevel = Skill_GetBase(skill);
			if ( iSkillLevel < iSkillMax )
				iSkillMax = iSkillLevel;
		}

		return( iSkillMax );
	}
	else
	{
		if ( skill == SKILL_MAX )
		{
			pTagStorage = GetKey("OVERRIDE.SKILLSUM", true);
			return pTagStorage ? static_cast<int>(pTagStorage->GetValNum()) : (500 * g_Cfg.m_iMaxSkill);
		}

		int iSkillMax = 1000;
		sprintf(sSkillName, "OVERRIDE.SKILLCAP_%d", static_cast<int>(skill));
		if ( (pTagStorage = GetKey(sSkillName, true)) != NULL )
			iSkillMax = static_cast<int>(pTagStorage->GetValNum());

		return iSkillMax;
	}
}

int CChar::Skill_GetSum() const
{
	ADDTOCALLSTACK("CChar::Skill_GetSum");
	int iSkillSum = 0;

	for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; i++ )
	{
		iSkillSum += Skill_GetBase(static_cast<SKILL_TYPE>(i));
	}

	return( iSkillSum );
}

void CChar::Skill_Decay()
{
	ADDTOCALLSTACK("CChar::Skill_Decay");
	// Decay the character's skill levels.

	SKILL_TYPE skillDeduct = SKILL_NONE;
	int iSkillLevel = 0;

	// look for a skill to deduct from
	for (size_t i = 0; i < g_Cfg.m_iMaxSkill; ++i)
	{
		if ( g_Cfg.m_SkillIndexDefs.IsValidIndex(static_cast<SKILL_TYPE>(i)) == false )
			continue;

		// check that the skill is set to decrease and that it is not already at 0
		if ( Skill_GetLock(static_cast<SKILL_TYPE>(i)) != SKILLLOCK_DOWN || Skill_GetBase(static_cast<SKILL_TYPE>(i)) <= 0 )
			continue;

		// prefer to deduct from lesser skills
		if ( skillDeduct != SKILL_NONE && iSkillLevel > Skill_GetBase(static_cast<SKILL_TYPE>(i)))
			continue;

		skillDeduct = static_cast<SKILL_TYPE>(i);
		iSkillLevel = Skill_GetBase(skillDeduct);
	}

	// debug message
#ifdef _DEBUG
	if ( ( g_Cfg.m_wDebugFlags & DEBUGF_ADVANCE_STATS ) && IsPriv( PRIV_DETAIL ) && GetPrivLevel() >= PLEVEL_GM )
	{
		if ( skillDeduct == SKILL_NONE )
			SysMessage("No suitable skill to reduce.\n");
		else
			SysMessagef("Reducing %s=%d.%d\n", g_Cfg.GetSkillKey(skillDeduct), iSkillLevel/10, iSkillLevel%10);
	}
#endif

	// deduct a point from the chosen skill
	if ( skillDeduct != SKILL_NONE )
	{
		iSkillLevel--;

		Skill_SetBase(skillDeduct, iSkillLevel);
	}
}

void CChar::Skill_Experience( SKILL_TYPE skill, int difficulty )
{
	ADDTOCALLSTACK("CChar::Skill_Experience");
	// Give the char credit for using the skill.
	// More credit for the more difficult. or none if too easy
	//
	// ARGS:
	//  difficulty = skill target from 0-100
	//

	if ( !IsSkillBase(skill) || !g_Cfg.m_SkillIndexDefs.IsValidIndex(skill) )
		return;
	if ( m_pArea && m_pArea->IsFlag( REGION_FLAG_SAFE ))	// skills don't advance in safe areas.
		return;

	const CSkillDef * pSkillDef = g_Cfg.GetSkillDef(skill);
	if (pSkillDef == NULL)
		return;

	difficulty *= 10;

	int iSkillLevel = Skill_GetBase( skill );
	if ( difficulty < 0 )
	{
		// failure. Give a little experience for failure at low levels.
		if ( iSkillLevel < 300 )
		{
			difficulty = (( minimum( -difficulty, iSkillLevel )) / 2 ) - 8;

			if ( difficulty <= 0 )
				difficulty = 1;
		}
		else
		{
			difficulty = 0;
		}
	}
	if ( difficulty > 1000 )
		difficulty = 1000;

	if ( m_pPlayer )
	{
		int iSkillSum = 0;
		int iSkillSumMax = Skill_GetMax(SKILL_MAX);

		for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; i++ )
		{
			iSkillSum += Skill_GetBase(static_cast<SKILL_TYPE>(i));
		}

		if ( iSkillSum >= iSkillSumMax )
			difficulty = 0;
	}

	INT64 iSkillMax = Skill_GetMax(skill);	// max advance for this skill.

	// ex. ADV_RATE=2000,500,25 for ANATOMY (easy)
	// ex. ADV_RATE=8000,2000,100 for alchemy (hard)
	// assume 100 = a 1 for 1 gain.
	// ex: 8000 = we must use it 80 times to gain .1
	// Higher the number = the less probable to advance.
	// Extrapolate a place in the range.

	// give a bonus or a penalty if the task was too hard or too easy.
	int iSkillAdj = iSkillLevel + ( iSkillLevel - difficulty );

	// no gain at all if it was WAY TOO easy
	int iGainRadius = pSkillDef->m_GainRadius;

	if ((iGainRadius > 0) && ((difficulty + iGainRadius) < iSkillLevel))
	{
		if ( GetKeyNum("NOSKILLMSG", true) )
			SysMessage( g_Cfg.GetDefaultMsg(DEFMSG_GAINRADIUS_NOT_MET) );
		return;
	}
	INT64 iChance = pSkillDef->m_AdvRate.GetChancePercent( iSkillAdj );

	CScriptTriggerArgs pArgs( 0 , iChance, iSkillMax);
	if ( IsTrigUsed(TRIGGER_SKILLGAIN) )
	{	
		if ( Skill_OnCharTrigger( skill, CTRIG_SkillGain, &pArgs ) == TRIGRET_RET_TRUE )
			return;
	}
	if ( IsTrigUsed(TRIGGER_GAIN) )
	{	
		if ( Skill_OnTrigger( skill, SKTRIG_GAIN, &pArgs ) == TRIGRET_RET_TRUE )
			return;
	}
	pArgs.getArgNs(0,&iChance,&iSkillMax);

	if ( iChance <= 0 )
		return; // less than no chance ?

	int iRoll = Calc_GetRandVal(1000);

	if ( iSkillLevel < iSkillMax ) // Are we in position to gain skill ?
	{
		// slightly more chance of decay than gain
		if ( (iRoll * 3) <= (iChance * 4) )
			Skill_Decay();

		if ( difficulty > 0 )
		{
#ifdef _DEBUG
			if ( IsPriv( PRIV_DETAIL ) &&
				GetPrivLevel() >= PLEVEL_GM &&
				( g_Cfg.m_wDebugFlags & DEBUGF_ADVANCE_STATS ))
			{
				SysMessagef( "%s=%d.%d Difficult=%d Gain Chance=%d.%d%% Roll=%d%%",
					(LPCTSTR) pSkillDef->GetKey(),
					iSkillLevel/10,(iSkillLevel)%10,
					difficulty/10, iChance/10, iChance%10, iRoll/10 );
			}
#endif

			if ( iRoll <= iChance )
			{
				iSkillLevel++;

				Skill_SetBase( skill, iSkillLevel );
			}
		}
	}

	////////////////////////////////////////////////////////
	// Dish out any stat gains - even for failures.

	int iStatSum = 0;

	// Stat effects are unrelated to advance in skill !
	for ( int i=STAT_STR; i<STAT_BASE_QTY; i++ )
	{
		// Can't gain STR or DEX if morphed.
		if ( IsStatFlag( STATF_Polymorph ) && i != STAT_INT )
			continue;

		int iStatVal = Stat_GetBase(static_cast<STAT_TYPE>(i));
		if ( iStatVal <= 0 )	// some odd condition
			continue;
		iStatSum += iStatVal;

		int iStatMax = Stat_GetLimit(static_cast<STAT_TYPE>(i));
		if ( iStatVal >= iStatMax )
			continue;	// nothing grows past this. (even for NPC's)

		// You will tend toward these stat vals if you use this skill a lot.
		int iStatTarg = pSkillDef->m_Stat[i];
		if ( iStatVal >= iStatTarg )
			continue;		// you've got higher stats than this skill is good for.

		// ??? Building stats should consume food !!

		difficulty = IMULDIV( iStatVal, 1000, iStatTarg );
		iChance = g_Cfg.m_StatAdv[i].GetChancePercent( difficulty );

		// adjust the chance by the percent of this that the skill uses.
		if (pSkillDef->m_StatPercent)
			iChance = ( iChance * pSkillDef->m_StatBonus[i] * pSkillDef->m_StatPercent ) / 10000;

		iRoll = Calc_GetRandVal(1000);

#ifdef _DEBUG
		if ( IsPriv( PRIV_DETAIL ) &&
			GetPrivLevel() >= PLEVEL_GM &&
			( g_Cfg.m_wDebugFlags & DEBUGF_ADVANCE_STATS ))
		{
			SysMessagef( "%s Difficult=%d Gain Chance=%d.%d%% Roll=%d%%",
				(LPCTSTR) g_Stat_Name[i], difficulty, iChance/10, iChance%10, iRoll/10 );
		}
#endif

		if ( iRoll <= iChance )
		{
			Stat_SetBase(static_cast<STAT_TYPE>(i), iStatVal + 1);
			break;
		}
	}

	// Check for stats degrade.
	int iStatSumAvg = Stat_GetLimit( STAT_QTY );

	if ( m_pPlayer && iStatSum > iStatSumAvg && !IsPriv( PRIV_GM ) )
	{
		// We are at a point where our skills can degrade a bit.
		// In theory magical enhancements make us lazy !

		int iStatSumMax = iStatSumAvg + iStatSumAvg/4;
		int iChanceForLoss = Calc_GetSCurve( iStatSumMax - iStatSum, ( iStatSumMax - iStatSumAvg ) / 4 );
		iRoll = Calc_GetRandVal(1000);

#ifdef _DEBUG
		if ( IsPriv( PRIV_DETAIL ) &&
			GetPrivLevel() >= PLEVEL_GM &&
			( g_Cfg.m_wDebugFlags & DEBUGF_ADVANCE_STATS ))
		{
			SysMessagef( "Loss Diff=%d Chance=%d.%d%% Roll=%d%%",
				iStatSumMax - iStatSum,
				iChanceForLoss/10, iChanceForLoss%10, iRoll/10 );
		}
#endif

		if ( iRoll < iChanceForLoss )
		{
			// Find the stat that was used least recently and degrade it.
			int imin = STAT_STR;
			int iminval = INT_MAX;
			for ( int i=STAT_STR; i<STAT_BASE_QTY; i++ )
			{
				if ( iminval > pSkillDef->m_StatBonus[i] )
				{
					imin = i;
					iminval = pSkillDef->m_StatBonus[i];
				}
			}

			int iStatVal = Stat_GetBase(static_cast<STAT_TYPE>(imin));
			if ( iStatVal > 10 )
			{
				Stat_SetBase(static_cast<STAT_TYPE>(imin), iStatVal - 1);
			}
		}
	}
}


bool CChar::Skill_CheckSuccess( SKILL_TYPE skill, int difficulty ) const
{
	ADDTOCALLSTACK("CChar::Skill_CheckSuccess");
	// PURPOSE:
	//  Check a skill for success or fail.
	//  DO NOT give experience here.
	// ARGS:
	//  difficulty = 0-100 = The point at which the equiv skill level has a 50% chance of success.
	// RETURN:
	//	true = success in skill.
	//

	if ( ! IsSkillBase(skill) || IsPriv( PRIV_GM ))
		return( true );

	return( g_Cfg.Calc_SkillCheck( Skill_GetAdjusted(skill), difficulty ));
}

bool CChar::Skill_UseQuick( SKILL_TYPE skill, INT64 difficulty, bool bAllowGain )
{
	ADDTOCALLSTACK("CChar::Skill_UseQuick");
	// ARGS:
	//	skill		= skill to use
	//  difficulty	= 0-100
	//	bAllowGain	= can gain skill from this?
	// Use a skill instantly. No wait at all.
	// No interference with other skills.
	INT64 result = Skill_CheckSuccess( skill, static_cast<int>(difficulty) );
	CScriptTriggerArgs pArgs( 0 , difficulty, result);
	TRIGRET_TYPE ret = TRIGRET_RET_DEFAULT;

	if ( IsTrigUsed(TRIGGER_SKILLUSEQUICK) )
	{
		ret = Skill_OnCharTrigger( skill, CTRIG_SkillUseQuick, &pArgs );
		pArgs.getArgNs(0,&difficulty,&result);

		if ( ret == TRIGRET_RET_TRUE )
			return( true );
		else if ( ret == TRIGRET_RET_FALSE )
			return( false );
	}
	if ( IsTrigUsed(TRIGGER_USEQUICK) )
	{
		ret = Skill_OnTrigger( skill, SKTRIG_USEQUICK, &pArgs );
		pArgs.getArgNs(0,&difficulty,&result);

		if ( ret == TRIGRET_RET_TRUE )
			return( true );
		else if ( ret == TRIGRET_RET_FALSE )
			return( false );
	}
	
	if ( ! result )
	{
		if ( bAllowGain )
			Skill_Experience( skill, static_cast<int>(-difficulty) );

		return( false );
	}
	if ( bAllowGain )
		Skill_Experience( skill, static_cast<int>(difficulty) );

	return( true );
}

void CChar::Skill_Cleanup()
{
	ADDTOCALLSTACK("CChar::Skill_Cleanup");
	// We are done with the skill.
	// We may have succeeded, failed, or cancelled.
	m_Act_Difficulty = 0;
	m_Act_SkillCurrent = SKILL_NONE;

	SetTimeout( m_pPlayer ? -1 : TICK_PER_SEC ); // we should get a brain tick next time.
}

LPCTSTR CChar::Skill_GetName( bool fUse ) const
{
	ADDTOCALLSTACK("CChar::Skill_GetName");
	// Name the current skill we are doing.

	SKILL_TYPE skill = Skill_GetActive();
	if ( skill <= SKILL_NONE )
	{
		return( "Idling" );
	}
	if ( IsSkillBase(skill))
	{
		if ( ! fUse )
		{
			return( g_Cfg.GetSkillKey(skill));
		}

		TCHAR * pszText = Str_GetTemp();
		sprintf( pszText, "%s %s", g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_USING), g_Cfg.GetSkillKey(skill));
		return( pszText );
	}

	switch ( skill )
	{
		case NPCACT_FOLLOW_TARG: return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_FOLLOWING) );
		case NPCACT_STAY: return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_STAYING) );
		case NPCACT_GOTO: return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_GOINGTO) );
		case NPCACT_WANDER: return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_WANDERING) );
		case NPCACT_FLEE: return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_FLEEING) );
		case NPCACT_TALK: return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_TALKING) );
		case NPCACT_TALK_FOLLOW: return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_TALKFOLLOW) );
		case NPCACT_GUARD_TARG: return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_GUARDING) );
		case NPCACT_GO_HOME: return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_GOINGHOME) );
		case NPCACT_BREATH: return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_BREATHING) );
		case NPCACT_LOOTING: return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_LOOTING) );
		case NPCACT_THROWING: return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_THROWING) );
		case NPCACT_LOOKING: return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_LOOKING) );
		case NPCACT_TRAINING: return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_TRAINING) );
		case NPCACT_Napping: return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_NAPPING) );
		case NPCACT_FOOD: return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_SEARCHINGFOOD) );
		case NPCACT_RUNTO: return ( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_RUNNINGTO) );
		default: return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_THINKING) );
	}
}

void CChar::Skill_SetTimeout()
{
	ADDTOCALLSTACK("CChar::Skill_SetTimeout");
	SKILL_TYPE skill = Skill_GetActive();
	ASSERT( IsSkillBase(skill));

	const CSkillDef * pSkillDef = g_Cfg.GetSkillDef(skill);
	if (pSkillDef == NULL)
		return;

	int iSkillLevel = Skill_GetBase(skill);
	INT64 iDelay = static_cast<INT64>(pSkillDef->m_Delay.GetLinear( iSkillLevel ));
	SetTimeout(iDelay);
}

INT64 CChar::Skill_GetTimeout()
{
	ADDTOCALLSTACK("CChar::Skill_SetTimeout");
	SKILL_TYPE skill = Skill_GetActive();
	ASSERT( IsSkillBase(skill));

	const CSkillDef * pSkillDef = g_Cfg.GetSkillDef(skill);
	if (pSkillDef == NULL)
		return 0;

	int iSkillLevel = Skill_GetBase(skill);
	INT64 iDelay = static_cast<INT64>(pSkillDef->m_Delay.GetLinear( iSkillLevel ));
	return(iDelay);
}



bool CChar::Skill_MakeItem_Success()
{
	ADDTOCALLSTACK("CChar::Skill_MakeItem_Success");
	// deliver the goods.
	int		quality	= 0;
	CItem * pItem = CItem::CreateTemplate( m_atCreate.m_ItemID, NULL, this );
	if ( pItem == NULL )
		return( false );

	// Cast the item to CItemVendable for setting quality and exp later
	CItemVendable * pItemVend = dynamic_cast<CItemVendable *> (pItem);

	TCHAR *pszMsg = Str_GetTemp();

	int iSkillLevel = Skill_GetBase( Skill_GetActive());	// primary skill value.
	if ( m_atCreate.m_Amount != 1 )
	{
		if ( pItem->IsType( IT_SCROLL ))
			pItem->m_itSpell.m_spelllevel = iSkillLevel;

		// Some item what is stackable? Then it is easy
		CItemBase * ptItemDef = CItemBase::FindItemBase( m_atCreate.m_ItemID );
		if ( ptItemDef->Can( CAN_I_PILE ) )
		{
			pItem->SetAmount( m_atCreate.m_Amount ); // Set the quantity if we are making bolts, arrows or shafts
		} 
		else 
		{
			CItem * ptItem;
			for (int n=1;n<m_atCreate.m_Amount;n++)
			{
				ptItem = CItem::CreateTemplate( m_atCreate.m_ItemID, NULL, this );
				ItemBounce( ptItem );
			}
		}
	}
	else if ( pItem->IsType( IT_SCROLL ))
	{
		// scrolls have the skill level of the inscriber ?
		pItem->m_itSpell.m_spelllevel = iSkillLevel;
	}
	else if ( pItem->IsType( IT_POTION ))
	{
		// Create the potion, set various properties,
	}
	else
	{
		// Only set the quality on single items.
		quality	= IMULDIV( iSkillLevel, 2, 10 );	// default value for quality.
		// Quality depends on the skill of the craftsman, and a random chance.
		// minimum quality is 1, maximum quality is 200.  100 is average.
		// How much variance?  This is the difference in quality levels from
		// what I can normally make.
		int variance = 2 - static_cast<int>(log10(static_cast<double>(Calc_GetRandVal( 250 ) + 1))); // this should result in a value between 0 and 2.
		// Determine if lower or higher quality
		if ( Calc_GetRandVal( 2 ))
		{
			// Better than I can normally make
		}
		else
		{
			// Worse than I can normally make
			variance = -(variance);
		}
		// The quality levels are as follows:
		// 1 - 25 Shoddy
		// 26 - 50 Poor
		// 51 - 75 Below Average
		// 76 - 125 Average
		// 125 - 150 Above Average
		// 151 - 175 Excellent
		// 175 - 200 Superior
		// Determine which range I'm in
		int qualityBase;
		if ( quality < 25 )
			qualityBase = 0;
		else if ( quality < 50 )
			qualityBase = 1;
		else if ( quality < 75 )
			qualityBase = 2;
		else if ( quality < 125 )
			qualityBase = 3;
		else if ( quality < 150 )
			qualityBase = 4;
		else if ( quality < 175 )
			qualityBase = 5;
		else
			qualityBase = 6;
		qualityBase += variance;
		if ( qualityBase < 0 )
			qualityBase = 0;
		if ( qualityBase > 6 )
			qualityBase = 6;

		switch ( qualityBase )
		{
			case 0:
				// Shoddy quality
				strcpy(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_MAKESUCCESS_1 ));
				quality = Calc_GetRandVal( 25 ) + 1;
				break;
			case 1:
				// Poor quality
				strcpy(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_MAKESUCCESS_2 ));
				quality = Calc_GetRandVal( 25 ) + 26;
				break;
			case 2:
				// Below average quality
				strcpy(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_MAKESUCCESS_3 ));
				quality = Calc_GetRandVal( 25 ) + 51;
				break;
			case 3:
				// Average quality
				quality = Calc_GetRandVal( 50 ) + 76;
				break;
			case 4:
				// Above average quality
				strcpy(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_MAKESUCCESS_4 ));
				quality = Calc_GetRandVal( 25 ) + 126;
				break;
			case 5:
				// Excellent quality
				strcpy(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_MAKESUCCESS_5 ));
				quality = Calc_GetRandVal( 25 ) + 151;
				break;
			case 6:
				// Superior quality
				strcpy(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_MAKESUCCESS_6 ));
				quality = Calc_GetRandVal( 25 ) + 176;
				break;
			default:
				// How'd we get here?
				quality = 1000;
				break;
		}

		if (pItemVend != NULL) // Check that the item is vendable before setting quality
			pItemVend->SetQuality(quality);

		if (( iSkillLevel > 999 && ( quality > 175 )) && ( !IsSetOF(OF_NoItemNaming)))
		{
			// A GM made this, and it is of high quality
			TCHAR * szNewName = Str_GetTemp();
			sprintf(szNewName, g_Cfg.GetDefaultMsg(DEFMSG_GRANDMASTER_MARK), static_cast<LPCTSTR>(pItem->GetName()), static_cast<LPCTSTR>(GetName()));
			pItem->SetName(szNewName);
		}
	}

	// TODO: Can Decay block decay on ground of items
	pItem->SetAttr(ATTR_MOVE_ALWAYS | ATTR_DECAY);	// Any made item is movable.

	// Item goes into ACT of player
	CGrayUID uidOldAct = m_Act_Targ;
	m_Act_Targ = pItem->GetUID();
	TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;
	if ( IsTrigUsed(TRIGGER_SKILLMAKEITEM) )
	{
		CScriptTriggerArgs Args( iSkillLevel, quality, uidOldAct.ObjFind() );
		iRet = OnTrigger( CTRIG_SkillMakeItem, this, &Args );
	}
	m_Act_Targ = uidOldAct;		// restore

	CObjBase * pItemCont = pItem->GetContainer();
	if ( iRet == TRIGRET_RET_TRUE )
	{
		if ( pItem->GetContainer() == pItemCont )
			pItem->Delete();
		return false;
	}
	else if ( iRet == TRIGRET_RET_DEFAULT )
	{
		if ( pItem->IsType( IT_POTION ))
		{
			if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
			{
				Sound( 0x240 );	// pouring noise.
			}
		}
		else if ( pItem->IsType( IT_MAP ))
		{
			if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
			{
				Sound( 0x255 );
			}
		}
		else if ( *pszMsg )
		{
			SysMessage(pszMsg);
		}
	}

	//	experience gain on craftings
	if ( g_Cfg.m_bExperienceSystem && ( g_Cfg.m_iExperienceMode&EXP_MODE_RAISE_CRAFT ))
	{
		int change = 0;
		// Calculate cost for buying this item if it is vendable
		if (pItemVend != NULL)
			change = pItemVend->GetVendorPrice(0)/100;

		//	item costing 1k will give 10 exp by default
		if ( change )
			ChangeExperience(change);
	}

	ItemBounce( pItem );
	return true;
}


int CChar::SkillResourceTest( const CResourceQtyArray * pResources )
{
	ADDTOCALLSTACK("CChar::SkillResourceTest");
	return pResources->IsResourceMatchAll( this );
}


bool CChar::Skill_MakeItem( ITEMID_TYPE id, CGrayUID uidTarg, SKTRIG_TYPE stage, bool fSkillOnly, int iReplicationQty )
{
	ADDTOCALLSTACK("CChar::Skill_MakeItem");
	// "MAKEITEM"
	//
	// SKILL_ALCHEMY
	// SKILL_BLACKSMITHING
	// SKILL_BOWCRAFT
	// SKILL_CARPENTRY
	// SKILL_COOKING
	// SKILL_INSCRIPTION
	// SKILL_TAILORING:
	// SKILL_TINKERING,
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
	//   true = success.
	//
	if ( LayerFind( LAYER_DRAGGING ) )
		ItemBounce(LayerFind( LAYER_DRAGGING));
	if ( id <= 0 )
		return( true );

	CItemBase * pItemDef = CItemBase::FindItemBase( id );
	if ( pItemDef == NULL )
		return( false );

	CItem * pItemTarg = uidTarg.ItemFind();
	if ( pItemTarg && stage == SKTRIG_SELECT )
	{
		if ( pItemDef->m_SkillMake.ContainsResourceMatch( pItemTarg ) == false &&
			pItemDef->m_BaseResources.ContainsResourceMatch( pItemTarg ) == false )
		{
			// Not intersect with the specified item
			return( false );
		}
	}

	if ( !SkillResourceTest( &(pItemDef->m_SkillMake) ) )
		return( false );
	if ( fSkillOnly )
		return( true );

	iReplicationQty = ResourceConsume( &(pItemDef->m_BaseResources), iReplicationQty, stage != SKTRIG_SUCCESS, pItemDef->GetResourceID().GetResIndex() );
	if ( ! iReplicationQty )
		return( false );

	// test or consume the needed resources.
	if ( stage == SKTRIG_FAIL )
	{
		// If fail only consume part of them.
		int iConsumePercent = -1;
		size_t i = pItemDef->m_SkillMake.FindResourceType( RES_SKILL );
		if ( i != pItemDef->m_SkillMake.BadIndex() )
		{
			const CSkillDef * pSkillDef = g_Cfg.GetSkillDef(static_cast<SKILL_TYPE>(pItemDef->m_SkillMake[i].GetResIndex()));
			if ( pSkillDef != NULL && pSkillDef->m_Effect.m_aiValues.GetCount() > 0 )
			{
				iConsumePercent = pSkillDef->m_Effect.GetRandom();
			}
		}

		if ( iConsumePercent < 0 )
			iConsumePercent = Calc_GetRandVal( 50 );

		ResourceConsumePart( &(pItemDef->m_BaseResources), iReplicationQty, iConsumePercent, false, pItemDef->GetResourceID().GetResIndex() );
		return( false );
	}

	if ( stage == SKTRIG_START )
	{
		// Start the skill.
		// Find the primary skill required.
		size_t i = pItemDef->m_SkillMake.FindResourceType( RES_SKILL );
		if ( i == pItemDef->m_SkillMake.BadIndex() )
			return( false );

		CResourceQty RetMainSkill = pItemDef->m_SkillMake[i];

		m_Act_Targ = uidTarg;	// targetted item to start the make process.
		m_atCreate.m_ItemID = id;
		m_atCreate.m_Amount = iReplicationQty;

		return Skill_Start(static_cast<SKILL_TYPE>(RetMainSkill.GetResIndex()), static_cast<int>(RetMainSkill.GetResQty() / 10));
	}

	if ( stage == SKTRIG_SUCCESS )
	{
		m_atCreate.m_Amount = iReplicationQty; // how much resources we really consumed
		return( Skill_MakeItem_Success() );
	}

	return( true );
}

int CChar::Skill_NaturalResource_Setup( CItem * pResBit )
{
	ADDTOCALLSTACK("CChar::Skill_NaturalResource_Setup");
	// RETURN: skill difficulty
	//  0-100
	ASSERT(pResBit);

	// Find the ore type located here based on color.
	const CRegionResourceDef * pOreDef = dynamic_cast <const CRegionResourceDef *>( g_Cfg.ResourceGetDef( pResBit->m_itResource.m_rid_res ));
	if ( pOreDef == NULL )
	{
		return( -1 );
	}

	return( pOreDef->m_Skill.GetRandom() / 10 );
}

CItem * CChar::Skill_NaturalResource_Create( CItem * pResBit, SKILL_TYPE skill )
{
	ADDTOCALLSTACK("CChar::Skill_NaturalResource_Create");
	// Create some natural resource item.
	// skill = Effects qty of items returned.
	// SKILL_MINING
	// SKILL_FISHING
	// SKILL_LUMBERJACKING

	ASSERT(pResBit);

	// Find the ore type located here based on color.
	CRegionResourceDef * pOreDef = dynamic_cast <CRegionResourceDef *>( g_Cfg.ResourceGetDef( pResBit->m_itResource.m_rid_res ));
	if ( pOreDef == NULL )
	{
		return( NULL );
	}

	// Skill effects how much of the ore i can get all at once.
	if ( pOreDef->m_ReapItem == ITEMID_NOTHING )
	{
		// I intended for there to be nothing here.
		return( NULL );
	}

	// Reap amount is semi-random
	int iAmount = pOreDef->m_ReapAmount.GetRandomLinear( Skill_GetBase(skill) );
	if ( ! iAmount)
	{
		int	maxAmount	= pResBit->GetAmount();
		// if REAPAMOUNT wasn't defined
		iAmount = pOreDef->m_Amount.GetRandomLinear( Skill_GetBase(skill) );
		iAmount	/= 2;
		if ( iAmount > maxAmount )	iAmount	= maxAmount;
		if ( iAmount < 1 ) 		iAmount	= 1;
	}

	//(Region)ResourceGather behaviour
	CScriptTriggerArgs	Args(0, 0, pResBit);
	Args.m_VarsLocal.SetNum("ResourceID",pOreDef->m_ReapItem);
	Args.m_iN1 = iAmount;
	TRIGRET_TYPE tRet = TRIGRET_RET_DEFAULT;
	if ( IsTrigUsed(TRIGGER_REGIONRESOURCEGATHER) )
		tRet = this->OnTrigger(CTRIG_RegionResourceGather, this, &Args);
	if ( IsTrigUsed(TRIGGER_RESOURCEGATHER) )
		tRet = pOreDef->OnTrigger("@ResourceGather", this, &Args);
	
	if ( tRet == TRIGRET_RET_TRUE )		return( NULL );

	iAmount = static_cast<int>(Args.m_iN1);
	//Creating the 'id' variable with the local given through->by the trigger(s) instead on top of method
	ITEMID_TYPE id = static_cast<ITEMID_TYPE>(RES_GET_INDEX( Args.m_VarsLocal.GetKeyNum("ResourceID")));

	iAmount = pResBit->ConsumeAmount( iAmount );	// amount i used up.
	if ( iAmount <= 0 )
	{
		return( NULL );
	}

	CItem * pItem = CItem::CreateScript( id, this );
	ASSERT(pItem);

	pItem->SetAmount( iAmount );
	return( pItem );
}

bool CChar::Skill_Mining_Smelt( CItem * pItemOre, CItem * pItemTarg )
{
	ADDTOCALLSTACK("CChar::Skill_Mining_Smelt");
	// SKILL_MINING
	// pItemTarg = forge or another pile of ore.
	// RETURN: true = success.
	if ( pItemOre == NULL || pItemOre == pItemTarg )
	{
		SysMessageDefault( DEFMSG_MINING_NOT_ORE );
		return( false );
	}

	// The ore is on the ground
	if ( ! CanUse( pItemOre, true ))
	{
		SysMessagef(g_Cfg.GetDefaultMsg( DEFMSG_MINING_REACH ), static_cast<LPCTSTR>(pItemOre->GetName()));
		return( false );
	}

	if ( pItemOre->IsType( IT_ORE ) &&
		pItemTarg != NULL &&
		pItemTarg->IsType( IT_ORE ))
	{
		// combine piles.
		if ( pItemTarg == pItemOre )
			return( false );
		if ( pItemTarg->GetID() != pItemOre->GetID())
			return( false );
		pItemTarg->SetAmountUpdate( pItemOre->GetAmount() + pItemTarg->GetAmount());
		pItemOre->Delete();
		return( true );
	}

#define SKILL_SMELT_FORGE_DIST 3

	if ( pItemTarg != NULL && pItemTarg->IsTopLevel() &&
		pItemTarg->IsType( IT_FORGE ))
	{
		m_Act_p = pItemTarg->GetTopPoint();
	}
	else
	{
		m_Act_p = g_World.FindItemTypeNearby( GetTopPoint(), IT_FORGE, SKILL_SMELT_FORGE_DIST, false );
	}

	if ( ! m_Act_p.IsValidPoint() || ! CanTouch(m_Act_p))
	{
		SysMessageDefault( DEFMSG_MINING_FORGE );
		return( false );
	}

	const CItemBase * pOreDef = pItemOre->Item_GetDef();
	if ( pOreDef->IsType( IT_INGOT ))
	{
		SysMessageDefault( DEFMSG_MINING_INGOTS );
		return false;
	}

	// Fire effect ?
	CItem * pItemEffect = CItem::CreateBase(ITEMID_FIRE);
	ASSERT(pItemEffect);
	CPointMap pt = m_Act_p;
	pt.m_z += 8;	// on top of the forge.
	pItemEffect->SetAttr( ATTR_MOVE_NEVER );
	pItemEffect->MoveToDecay( pt, TICK_PER_SEC );
	if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
	{
		Sound( 0x2b );
	}

	UpdateDir( m_Act_p );
	if ( pItemOre->IsAttr(ATTR_MAGIC|ATTR_BLESSED|ATTR_BLESSED2|ATTR_MOVE_ALWAYS))	// not magic items
	{
		SysMessageDefault( DEFMSG_MINING_FIRE );
		return false;
	}

	TCHAR * pszMsg = Str_GetTemp();
	sprintf(pszMsg, "%s %s", g_Cfg.GetDefaultMsg( DEFMSG_MINING_SMELT ), static_cast<LPCTSTR>(pItemOre->GetName()));
	Emote(pszMsg);

	int iMiningSkill = Skill_GetAdjusted(SKILL_MINING);
	int iOreQty = pItemOre->GetAmount();
	const CItemBase * pIngotDef = NULL;
	int iIngotQty = 0;

	if ( pOreDef->IsType( IT_ORE ))
	{
		ITEMID_TYPE idIngot = static_cast<ITEMID_TYPE>(RES_GET_INDEX( pOreDef->m_ttOre.m_IngotID));
		pIngotDef = CItemBase::FindItemBase(idIngot);
		iIngotQty = 1;	// ingots per ore.
	}
	else
	{
		// Smelting something like armor etc.
		// find the ingot type resources.
		for ( size_t i = 0; i < pOreDef->m_BaseResources.GetCount(); i++ )
		{
			RESOURCE_ID rid = pOreDef->m_BaseResources[i].GetResourceID();
			if ( rid.GetResType() != RES_ITEMDEF )
				continue;

			const CItemBase * pBaseDef = CItemBase::FindItemBase(static_cast<ITEMID_TYPE>(rid.GetResIndex()));
			if ( pBaseDef == NULL )
				continue;

			if ( pBaseDef->IsType( IT_GEM ))
			{
				// bounce the gems out of this.
				CItem * pGem = CItem::CreateScript(pBaseDef->GetID(), this);
				if ( pGem )
				{
					pGem->SetAmount(static_cast<unsigned int>(iOreQty * pBaseDef->m_BaseResources[i].GetResQty()));
					ItemBounce(pGem);
				}
				continue;
			}
			if ( pBaseDef->IsType( IT_INGOT ))
			{
				if ( iMiningSkill < pBaseDef->m_ttIngot.m_iSkillMin )
				{
					SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_MINING_SKILL ), static_cast<LPCTSTR>(pBaseDef->GetName()));
					continue;
				}
				pIngotDef = pBaseDef;
				iIngotQty = static_cast<int>(pOreDef->m_BaseResources[i].GetResQty());
			}
		}
	}

	if ( pIngotDef == NULL ||
		! pIngotDef->IsType(IT_INGOT))
	{
		SysMessageDefault( DEFMSG_MINING_CONSUMED );
		pItemOre->ConsumeAmount( iOreQty );
		return true;
	}

	iIngotQty *= iOreQty;	// max amount

	int iSkillRange = pIngotDef->m_ttIngot.m_iSkillMax - pIngotDef->m_ttIngot.m_iSkillMin;
	int iDifficulty = Calc_GetRandVal(iSkillRange);
	//iIngotQty = IMULDIV( iIngotQty, iDifficulty, iSkillRange );
	//if ( iIngotQty <= 0 )
	//	iIngotQty = 1;

	// try to make ingots.
	iDifficulty = ( pIngotDef->m_ttIngot.m_iSkillMin + iDifficulty ) / 10;
	if ( ! iIngotQty || ! Skill_UseQuick( SKILL_MINING, iDifficulty ))
	{
		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_MINING_NOTHING ), static_cast<LPCTSTR>(pItemOre->GetName()));
		// Lose up to half the resources.
		pItemOre->ConsumeAmount( Calc_GetRandVal( pItemOre->GetAmount() / 2 ) + 1 );
		return( false );
	}

	// Payoff - What do i get ?
	// This is the one
	CItem * pIngots = CItem::CreateScript( pIngotDef->GetID(), this );
	if ( pIngots == NULL )
	{
		// Either this is really iron, or there isn't an ingot defined for this guy
		SysMessageDefault( DEFMSG_MINING_NOTHING );
		return( false );
	}

	// give some random loss factor.
	pIngots->SetAmount( iIngotQty );
	pItemOre->ConsumeAmount( pItemOre->GetAmount());
	ItemBounce( pIngots );
	return true;
}

bool CChar::Skill_Tracking( CGrayUID uidTarg, DIR_TYPE & dirPrv, int iDistMax )
{
	ADDTOCALLSTACK("CChar::Skill_Tracking");
	// SKILL_TRACKING
	const CObjBase * pObj = uidTarg.ObjFind();
	if ( pObj == NULL )
	{
		return false;
	}

	const CObjBaseTemplate * pObjTop = pObj->GetTopLevelObj();
	if ( pObjTop == NULL )
	{
		return( false );
	}

	int dist = GetTopDist3D( pObjTop );	// disconnect = SHRT_MAX
	if ( dist > iDistMax )
	{
		return false;
	}

	if ( pObjTop->IsChar() )
	{
		// prevent tracking of hidden staff
		const CChar *pChar = dynamic_cast<const CChar*>( pObjTop );
		if ( pChar )
		{
			if ( GetPrivLevel() < pChar->GetPrivLevel() && pChar->IsStatFlag(STATF_Insubstantial) )
				return false;
		}
	}

	DIR_TYPE dir = GetDir( pObjTop );
	if (( dirPrv != dir ) || ! Calc_GetRandVal(10))
	{
		dirPrv = dir;

		TCHAR *pszMsg = Str_GetTemp();
		LPCTSTR pszDef;
		// select tracking message based on distance
		if (dist <= 0)
			pszDef = g_Cfg.GetDefaultMsg( DEFMSG_TRACKING_RESULT_0 );
		else if (dist < 16)
			pszDef = g_Cfg.GetDefaultMsg( DEFMSG_TRACKING_RESULT_1 );
		else if (dist < 32)
			pszDef = g_Cfg.GetDefaultMsg( DEFMSG_TRACKING_RESULT_2 );
		else if (dist < 100)
			pszDef = g_Cfg.GetDefaultMsg( DEFMSG_TRACKING_RESULT_3 );
		else
			pszDef = g_Cfg.GetDefaultMsg( DEFMSG_TRACKING_RESULT_4 );

		ASSERT(dir >= 0 && static_cast<unsigned int>(dir) < COUNTOF(CPointBase::sm_szDirs));
		sprintf(pszMsg, pszDef, pObj->GetName(), pObjTop->IsDisconnected()? g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_RESULT_DISC) : "", static_cast<LPCTSTR>(CPointBase::sm_szDirs[dir]));
		ObjMessage(pszMsg, this);
	}

	return true;		// keep the skill active.
}

//************************************
// Skill handlers.

int CChar::Skill_Tracking( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Tracking");
	// SKILL_TRACKING
	// m_Act_Targ = what am i tracking ?
	// m_atTracking.m_PrvDir = the previous dir it was in.
	//

	if ( stage == SKTRIG_START )
	{
		// Already checked difficulty earlier.
		return( 0 );
	}
	if ( stage == SKTRIG_FAIL )
	{
		// This skill didn't fail, it just ended/went out of range, etc...
		ObjMessage( g_Cfg.GetDefaultMsg( DEFMSG_TRACKING_END ), this ); // say this instead of the failure message
		return( -SKTRIG_ABORT );
	}
	if ( stage == SKTRIG_STROKE )
	{
		int iSkillLevel = Skill_GetAdjusted(SKILL_TRACKING);
		if ( ! Skill_Tracking( m_Act_Targ, m_atTracking.m_PrvDir, iSkillLevel/20 + 10 ))
			return( -SKTRIG_ABORT );
		Skill_SetTimeout();		// next update.
		return( -SKTRIG_STROKE );	// keep it active.
	}

	return( -SKTRIG_ABORT );
}

int CChar::Skill_Alchemy( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Alchemy");
	// SKILL_ALCHEMY
	// m_atCreate.m_ItemID = potion we are making.
	// m_atCreate.m_Amount = amount of said item.

	if ( stage == SKTRIG_START )
	{
		if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
		{
			Sound( 0x242 );
		}
	}

	return( Skill_MakeItem( stage ));
}

int CChar::Skill_Mining( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Mining");
	// SKILL_MINING
	// m_Act_p = the point we want to mine at.
	// m_Act_TargPrv = Shovel
	//
	// Test the chance of precious ore.
	// resource check  to IT_ORE. How much can we get ?
	// RETURN:
	//  Difficulty 0-100

	if ( stage == SKTRIG_FAIL )
		return 0;

	if ( m_Act_p.m_x == -1 )
	{
		SysMessageDefault( DEFMSG_MINING_4 );
		return( -SKTRIG_QTY );
	}

	// Verify so we have a line of sight.
	if ( ! CanSeeLOS( m_Act_p, NULL, 2 ))
	{
		int Range;
		const CSkillDef * pSkillDef = g_Cfg.GetSkillDef(SKILL_MINING);
		Range = pSkillDef->m_Range;
		if ( ! pSkillDef->m_Range )
		{
			g_Log.EventError("Mining skill doesn't have a value for RANGE, defaulting to 2\n");
			Range = 2;
		}
		if ( GetTopPoint().GetDist( m_Act_p ) > Range )
			SysMessageDefault( DEFMSG_MINING_REACH );
		else
			SysMessageDefault( DEFMSG_MINING_LOS );
		return( -SKTRIG_QTY );
	}

	CItem * pShovel = m_Act_TargPrv.ItemFind();
	if ( pShovel == NULL )
	{
		SysMessageDefault( DEFMSG_MINING_TOOL );
		return( -SKTRIG_ABORT );
	}

	// resource check
	CItem * pResBit = g_World.CheckNaturalResource( m_Act_p,
		(IT_TYPE) GETINTRESOURCE(m_atResource.m_ridType), stage == SKTRIG_START, this );

	if ( pResBit == NULL )
	{
		SysMessageDefault( DEFMSG_MINING_1 );
		return( -SKTRIG_QTY );
	}
	if ( pResBit->GetAmount() == 0 )
	{
		SysMessageDefault( DEFMSG_MINING_2 );
		return( -SKTRIG_QTY );
	}

	if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOMINDIST ) )
	{
		if ( GetTopPoint().GetDist( m_Act_p ) < 1 )
		{
			SysMessageDefault( DEFMSG_MINING_CLOSE );
			return( -SKTRIG_QTY );
		}
	}

	if ( stage == SKTRIG_START )
	{
		m_atResource.m_Stroke_Count = Calc_GetRandVal( 5 ) + 2;
		return( Skill_NaturalResource_Setup( pResBit ));
	}

	// Useless now, strokes in SKF_GATHER are handled directly from Skill_Stage() and its code softcoded in their skill's triggers.
	// However I do not remove this block because of people not updating a shit!
	if ( stage == SKTRIG_STROKE )
	{
		UpdateDir( m_Act_p );
		if ( IsSetEF(EF_DamageTools) )
		{
			if ( pShovel->m_itWeapon.m_Hits_Cur )
				pShovel->OnTakeDamage( 1, this, DAMAGE_HIT_BLUNT );
			else
				pShovel->Delete();
		}

		if ( m_atResource.m_Stroke_Count )
		{
			int stroke = Skill_Stroke( true );
			if ( stroke == -SKTRIG_ABORT  || stroke == -SKTRIG_STROKE)
				return stroke;
		}

		return 0;
	}

	CItem * pItem = Skill_NaturalResource_Create( pResBit, SKILL_MINING );
	if ( pItem == NULL )
	{
		SysMessageDefault( DEFMSG_MINING_3 );
		return( -SKTRIG_FAIL );
	}

	if (m_atUnk.m_Arg2)
	{
		ItemBounce( pItem );
	}
	else
	{
		pItem->MoveToCheck( GetTopPoint(), this );	// put at my feet.
	}
	return 0;
}

int CChar::Skill_Fishing( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Fishing");
	// SKILL_FISHING
	// m_Act_p = where to fish.
	// NOTE: don't check LOS else you can't fish off boats.
	// Check that we dont stand too far away
	// Make sure we aren't in a house
	//
	// RETURN:
	//   difficulty = 0-100

	if ( stage == SKTRIG_FAIL )
		return 0;

	if ( m_Act_p.m_x == -1 )
	{
		SysMessageDefault( DEFMSG_FISHING_4 );
		return( -SKTRIG_QTY );
	}

	//Distance checks
	CRegionBase * pRegion = GetTopPoint().GetRegion(REGION_TYPE_MULTI);
	if (( pRegion ) && ( !pRegion->IsFlag(REGION_FLAG_SHIP) ))
	{
		// Are we in a house ?
		SysMessageDefault( DEFMSG_FISHING_3 );
		return( -SKTRIG_QTY );
	}

	if (m_Act_p.GetRegion(REGION_TYPE_MULTI)) // Do not allow him to fish through ship floor
	{
		SysMessageDefault( DEFMSG_FISHING_4 );
		return( -SKTRIG_QTY );
	}

	int Range;
	const CSkillDef * pSkillDef = g_Cfg.GetSkillDef(SKILL_FISHING);
	Range = pSkillDef->m_Range;
	if ( ! pSkillDef->m_Range )
	{
		g_Log.EventError("Fishing skill doesn't have a value for RANGE, defaulting to 4\n");
		Range = 4;
	}
	if (( m_pPlayer && g_Cfg.m_iAdvancedLos & ADVANCEDLOS_PLAYER ) || ( m_pNPC && g_Cfg.m_iAdvancedLos & ADVANCEDLOS_NPC ))
	{
		if ( ! CanSeeLOS( m_Act_p, NULL, 18, LOS_FISHING ))
		{
			SysMessageDefault( DEFMSG_FISHING_LOS );
			return( -SKTRIG_QTY );
		}

		if ( GetTopPoint().GetDist( m_Act_p ) > Range )	// Max distance for fishing.
		{
			SysMessageDefault( DEFMSG_FISHING_REACH );
			return( -SKTRIG_QTY );
		}
	}
	else
	{
		if ( GetTopPoint().GetDist( m_Act_p ) > Range ) // cast works for long distances.
		{
			SysMessageDefault( DEFMSG_FISHING_REACH );
			return( -SKTRIG_QTY );
		}
	}

	// resource check
	CItem * pResBit = g_World.CheckNaturalResource( m_Act_p,
			(IT_TYPE) GETINTRESOURCE(m_atResource.m_ridType), stage == SKTRIG_START, this );

	if ( pResBit == NULL )
	{
		SysMessageDefault( DEFMSG_FISHING_1 );
		return( -SKTRIG_QTY );
	}
	if ( pResBit->GetAmount() == 0 )
	{
		SysMessageDefault( DEFMSG_FISHING_2 );
		return( -SKTRIG_QTY );
	}

	if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOMINDIST ) )
	{
		if ( GetTopPoint().GetDist( m_Act_p ) < 2 ) // you cannot fish under your legs
		{
			SysMessageDefault( DEFMSG_FISHING_CLOSE );
			return( -SKTRIG_QTY );
		}
	}

	if ( stage == SKTRIG_START )
	{
		//m_atResource.m_Stroke_Count = 1;
		m_Act_Targ = pResBit->GetUID();
		return( Skill_NaturalResource_Setup( pResBit ));
	}

	if ( stage == SKTRIG_STROKE )	// Useless now, strokes in SKF_GATHER are handled directly from Skill_Stage() and its code softcoded in their skill's triggers.
	// I remain this code active however for 'old code is better, updates sux, fuck u updates makers, i'll cry and tell this is not working even if its correctly working muahahaha', u know.
	{
		UpdateDir( m_Act_p );
		m_Act_Targ = pResBit->GetUID();
		if ( m_atResource.m_Stroke_Count )
		{
			int stroke = Skill_Stroke( true );
			if ( stroke == -SKTRIG_ABORT  || stroke == -SKTRIG_STROKE)
				return stroke;
		}

		return 0 ;	// keep active.
	}

	CItem * pFish = Skill_NaturalResource_Create( pResBit, SKILL_FISHING );
	if ( pFish == NULL )
	{
		SysMessageDefault( DEFMSG_FISHING_2 );
		return( -SKTRIG_ABORT );
	}
	SysMessagef(g_Cfg.GetDefaultMsg( DEFMSG_FISHING_SUCCESS ), static_cast<LPCTSTR>(pFish->GetName()));

	if (m_atUnk.m_Arg2)
	{
		ItemBounce( pFish );
	}
	else
	{
		pFish->MoveToCheck( GetTopPoint(), this );	// put at my feet.
	}
	m_Act_Targ = pFish->GetUID();

	/*if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
		Sound( 0x364 );	// 0x027 old sound.
	CItem * pItemFX = CItem::CreateBase( ITEMID_FX_SPLASH );
	pItemFX->SetType(IT_WATER_WASH);	// can't fish here.
	pItemFX->MoveToDecay( m_Act_p, 1*TICK_PER_SEC );*/

	return 0;
}

int CChar::Skill_Lumberjack( SKTRIG_TYPE stage )
{	
	ADDTOCALLSTACK("CChar::Skill_Lumberjack");
	// SKILL_LUMBERJACK
	// m_Act_p = the point we want to chop/hack at.
	// m_Act_TargPrv = Axe/Dagger
	// NOTE: The skill is used for hacking with IT_FENCE (e.g. i_dagger) 
	//
	// RETURN:
	//   difficulty = 0-100

	if ( stage == SKTRIG_FAIL )
		return 0;

	CItem * pResBit = g_World.CheckNaturalResource( m_Act_p,
		(IT_TYPE) GETINTRESOURCE(m_atResource.m_ridType), stage == SKTRIG_START, this );

	CItem * pAxe = m_Act_TargPrv.ItemFind();

	/*if ( stage == SKTRIG_FAIL )
	{
		if ( (pAxe->IsType(IT_WEAPON_FENCE)) && ( pResBit->GetAmount() != 0 ) )
		{
			SysMessageDefault( DEFMSG_LUMBERJACKING_5 );
			ItemBounce( CItem::CreateScript( ITEMID_KINDLING1, this ));
			pResBit->ConsumeAmount(1);
		}
		return 0;
	}*/

	if ( m_Act_p.m_x == -1 )
	{
		SysMessageDefault( DEFMSG_LUMBERJACKING_6 );
		return( -SKTRIG_QTY );
	}

	// 3D distance check and LOS
	
	int Range;
	const CSkillDef * pSkillDef = g_Cfg.GetSkillDef(SKILL_MINING);
	Range = pSkillDef->m_Range;
	if ( ! pSkillDef->m_Range )
	{
		g_Log.EventError("Lumberjacking skill doesn't have a value for RANGE, defaulting to 2\n");
		Range = 2;
	}
	if ( GetTopPoint().GetDist3D( m_Act_p ) > Range )
	{
		SysMessageDefault( DEFMSG_LUMBERJACKING_REACH );
		return( -SKTRIG_QTY );
	}
	if ( ! CanSeeLOS( m_Act_p, NULL, 2 ))
	{
		SysMessageDefault( DEFMSG_LUMBERJACKING_LOS );
		return( -SKTRIG_QTY );
	}

	if ( pAxe == NULL )
	{
		SysMessageDefault( DEFMSG_LUMBERJACKING_TOOL );
		return( -SKTRIG_ABORT );
	}

	// resource check
	if ( pResBit == NULL )
	{
		if ( pAxe->IsType(IT_WEAPON_FENCE) ) //dagger
			SysMessageDefault( DEFMSG_LUMBERJACKING_3 );	// not a tree
		else
			SysMessageDefault( DEFMSG_LUMBERJACKING_1 );
		return( -SKTRIG_QTY );
	}
	if ( pResBit->GetAmount() == 0 )
	{
		if ( pAxe->IsType(IT_WEAPON_FENCE) ) //dagger
			SysMessageDefault( DEFMSG_LUMBERJACKING_4 );	// no wood to harvest
		else
			SysMessageDefault( DEFMSG_LUMBERJACKING_2 );
		return( -SKTRIG_QTY );
	}

	if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOMINDIST ) )
	{
		if ( GetTopPoint().GetDist( m_Act_p ) < 1 )
		{
			SysMessageDefault( DEFMSG_LUMBERJACKING_CLOSE );
			return( -SKTRIG_QTY );
		}
	}

	if ( stage == SKTRIG_START )
	{
		m_atResource.m_Stroke_Count = Calc_GetRandVal( 5 ) + 2;
		return( Skill_NaturalResource_Setup( pResBit ));
	}

	if ( stage == SKTRIG_STROKE )	// Useless now, strokes in SKF_GATHER are handled directly from Skill_Stage() and its code softcoded in their skill's triggers.
	{
		UpdateDir( m_Act_p );

		if (IsSetEF(EF_DamageTools) )
		{
			if ( pAxe->m_itWeapon.m_Hits_Cur )
				pAxe->OnTakeDamage( 1, this, DAMAGE_HIT_BLUNT );
			else
				pAxe->Delete();
		}

		if ( m_atResource.m_Stroke_Count )
		{
			int stroke = Skill_Stroke( true );
			if ( stroke == -SKTRIG_ABORT  || stroke == -SKTRIG_STROKE)
				return stroke;
		}
		return SKTRIG_SUCCESS;// keep active.
	}

	if ( pAxe->IsType(IT_WEAPON_FENCE) ) //dagger end
	{
		SysMessageDefault( DEFMSG_LUMBERJACKING_5 );
		ItemBounce( CItem::CreateScript( ITEMID_KINDLING1, this ));
		pResBit->ConsumeAmount(1);
		return 0;
	}
	CItem * pItem = Skill_NaturalResource_Create( pResBit, SKILL_LUMBERJACKING );
	if ( pItem == NULL )
	{
		SysMessageDefault( DEFMSG_LUMBERJACKING_2 );
		return( -SKTRIG_FAIL );
	}

	if (m_atUnk.m_Arg2)
	{
		ItemBounce( pItem );
	}
	else
	{
		pItem->MoveToCheck( GetTopPoint(), this );	// put at my feet.
	}
	return 0;
}

int CChar::Skill_DetectHidden( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_DetectHidden");
	// SKILL_DETECTINGHIDDEN
	// Look around for who is hiding.
	// Detect them based on skill diff.
	// ??? Hidden objects ?

	if ( stage == SKTRIG_START )
	{
		// Based on who is hiding ?
		return( 10 );
	}
	if ( stage == SKTRIG_FAIL )
	{
		return 0;
	}
	if ( stage == SKTRIG_STROKE )
	{
		return 0;
	}
	if ( stage != SKTRIG_SUCCESS )
	{
		ASSERT(0);
		return( -SKTRIG_QTY );
	}

	int iRadius = ( Skill_GetAdjusted(SKILL_DETECTINGHIDDEN) / 8 ) + 1;
	CWorldSearch Area( GetTopPoint(), iRadius );
	bool fFound = false;
	for (;;)
	{
		CChar * pChar = Area.GetChar();
		if ( pChar == NULL )
			break;
		if ( pChar == this )
			continue;
		if ( ! pChar->IsStatFlag( STATF_Invisible | STATF_Hidden ))
			continue;
		// Try to detect them.
		if ( pChar->IsStatFlag( STATF_Hidden ))
		{
			// If there hiding skill is much better than our detect then stay hidden
		}
		pChar->Reveal();
		SysMessagef(g_Cfg.GetDefaultMsg( DEFMSG_DETECTHIDDEN_SUCC ), static_cast<LPCTSTR>(pChar->GetName()));
		fFound = true;
	}

	if ( ! fFound )
	{
		return( -SKTRIG_FAIL );
	}

	return( 0 );
}

int CChar::Skill_Cartography( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Cartography");
	// SKILL_CARTOGRAPHY
	// m_atCreate.m_ItemID = map we are making.
	// m_atCreate.m_Amount = amount of said item.

	if ( stage == SKTRIG_START )
	{
		if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
		{
			Sound( 0x249 );
		}
	}

	return( Skill_MakeItem( stage ));
}

int CChar::Skill_Musicianship( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Musicianship");
	// m_Act_Targ = the intrument i targetted to play.

	if ( stage == SKTRIG_STROKE )
		return 0;
	if ( stage == SKTRIG_START )
	{
		// no instrument fail immediate
		return Use_PlayMusic( m_Act_Targ.ItemFind(), Calc_GetRandVal(90));
	}

	return( 0 );
}

int CChar::Skill_Peacemaking( SKTRIG_TYPE stage )
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
			{
				return 0;
			}

		case SKTRIG_SUCCESS:
			{
				int peace = Skill_GetAdjusted(SKILL_PEACEMAKING);
				int iRadius = ( peace / 100 ) + 2;	// 2..12
				CWorldSearch Area(GetTopPoint(), iRadius);
				for (;;)
				{
					CChar *pChar = Area.GetChar();
					if ( pChar == NULL )
						return -SKTRIG_FAIL;
					if (( pChar == this ) || !CanSee(pChar) )
						continue;

					if ( pChar->Skill_GetAdjusted(SKILL_PEACEMAKING) > peace )
						SysMessagef("%s %s.", pChar->GetName(),g_Cfg.GetDefaultMsg( DEFMSG_PEACEMAKING_IGNORE ));
					else if ( pChar->Skill_GetAdjusted(SKILL_PROVOCATION) > peace )
					{
						SysMessagef("%s %s.", pChar->GetName(),g_Cfg.GetDefaultMsg( DEFMSG_PEACEMAKING_DISOBEY ));
						if ( pChar->Noto_IsEvil() )
							pChar->Fight_Attack(this);
					}
					else
					{
						pChar->Fight_ClearAll();
					}

					break;
				}
				return 0;
			}
		default:
			break;
	}
	return -SKTRIG_QTY;
}

int CChar::Skill_Enticement( SKTRIG_TYPE stage )
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
			{
				return 0;
			}

		case SKTRIG_SUCCESS:
			{
				if ( pChar->m_pPlayer )
				{
					SysMessagef("%s", g_Cfg.GetDefaultMsg( DEFMSG_ENTICEMENT_PLAYER ));
					return -SKTRIG_ABORT;
				}
				else if ( pChar->IsStatFlag(STATF_War) )
				{
					SysMessagef("%s %s.", pChar->GetName(), g_Cfg.GetDefaultMsg(DEFMSG_ENTICEMENT_BATTLE));
					return -SKTRIG_ABORT;
				}

				pChar->m_Act_p = GetTopPoint();
				pChar->NPC_WalkToPoint( ( pChar->m_Act_p.GetDist(pChar->GetTopPoint()) > 6) );

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

	CChar	*pCharProv = m_Act_TargPrv.CharFind();
	CChar	*pCharTarg = m_Act_Targ.CharFind();

	if (
		!pCharProv || !pCharTarg ||
		( pCharProv == this ) || ( pCharTarg == this ) || ( pCharProv == pCharTarg ) ||
		pCharProv->IsStatFlag(STATF_Pet|STATF_Conjured|STATF_Stone|STATF_DEAD|STATF_INVUL) ||
		pCharTarg->IsStatFlag(STATF_Pet|STATF_Conjured|STATF_Stone|STATF_DEAD|STATF_INVUL) ||
		m_pArea->IsFlag(REGION_FLAG_SAFE|REGION_FLAG_GUARDED) ||
		pCharProv->m_pArea->IsFlag(REGION_FLAG_SAFE|REGION_FLAG_GUARDED) ||
		pCharTarg->m_pArea->IsFlag(REGION_FLAG_SAFE|REGION_FLAG_GUARDED) )
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
	{
		return -SKTRIG_ABORT;
	}

	switch ( stage )
	{
		case SKTRIG_START:
		{
			int iDifficulty = Use_PlayMusic(NULL, Calc_GetRandVal(40));
			if ( iDifficulty < -1 )	// no instrument fail immediate
				return -SKTRIG_ABORT;

			if ( !iDifficulty )
				iDifficulty = pCharProv->Stat_GetAdjusted(STAT_INT);	// Depend on evil of the creature.

			//	cannot provoke more experienced provoker
			if ( pCharProv->Skill_GetAdjusted(SKILL_PROVOCATION) >= Skill_GetAdjusted(SKILL_PROVOCATION) )
				iDifficulty = 0;

			return iDifficulty;
		}

		case SKTRIG_FAIL:
			pCharProv->Fight_Attack(this);
			return 0;

		case SKTRIG_SUCCESS:
			// They are just too good for this.
			if ( pCharProv->Stat_GetAdjusted(STAT_KARMA) >= Calc_GetRandVal2(1000, 10000) )
			{
				pCharProv->Emote(g_Cfg.GetDefaultMsg(DEFMSG_PROVOCATION_EMOTE_1));
				return -SKTRIG_ABORT;
			}

			pCharProv->Emote(g_Cfg.GetDefaultMsg(DEFMSG_PROVOCATION_EMOTE_2));

			// He realizes that you are the real bad guy as well.
			if ( !pCharTarg->OnAttackedBy(this, 1, true) )
				return -SKTRIG_ABORT;

			pCharProv->Memory_AddObjTypes(this, MEMORY_AGGREIVED|MEMORY_IRRITATEDBY);

			// If out of range we might get attacked ourself.
			if ( pCharProv->GetTopDist3D(pCharTarg) > UO_MAP_VIEW_SIGHT ||
				pCharProv->GetTopDist3D(this) > UO_MAP_VIEW_SIGHT )
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
				CheckCrimeSeen(SKILL_NONE, NULL, pCharProv, g_Cfg.GetDefaultMsg( DEFMSG_PROVOKING_CRIME ));
				return -SKTRIG_ABORT;
			}

			// If we provoke upon a good char we should go criminal for it
			// but skill still succeed.
			if ( pCharTarg->Noto_GetFlag(this) == NOTO_GOOD )
			{
				CheckCrimeSeen(SKILL_NONE, NULL, pCharTarg, "provoking");
			}

			pCharProv->Fight_Attack(pCharTarg); // Make the actual provoking.
			return 0;

		default:
			break;
	}
	return -SKTRIG_QTY;
}

int CChar::Skill_Poisoning( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Poisoning");
	// Act_TargPrv = poison this weapon/food
	// Act_Targ = with this poison.

	if ( stage == SKTRIG_STROKE )
	{
		return 0;
	}

	CItem * pPoison = m_Act_Targ.ItemFind();
	if ( pPoison == NULL ||
		! pPoison->IsType(IT_POTION))
	{
		return( -SKTRIG_ABORT );
	}

	if ( stage == SKTRIG_START )
	{
		return Calc_GetRandVal( 60 );
	}
	if ( stage == SKTRIG_FAIL )
	{
		// Lose the poison sometimes ?
		return( 0 );
	}

	if ( RES_GET_INDEX(pPoison->m_itPotion.m_Type) != SPELL_Poison )
	{
		return( -SKTRIG_ABORT );
	}

	CItem * pItem = m_Act_TargPrv.ItemFind();
	if ( pItem == NULL )
	{
		return( -SKTRIG_QTY );
	}

	if ( stage != SKTRIG_SUCCESS )
	{
		ASSERT(0);
		return( -SKTRIG_ABORT );
	}

	if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
	{
		Sound( 0x247 );	// powdering.
	}

	switch ( pItem->GetType() )
	{
		case IT_FRUIT:
		case IT_FOOD:
		case IT_FOOD_RAW:
		case IT_MEAT_RAW:
			pItem->m_itFood.m_poison_skill = static_cast<unsigned char>(pPoison->m_itPotion.m_skillquality / 10);
			break;
		case IT_WEAPON_MACE_SHARP:
		case IT_WEAPON_SWORD:		// 13 =
		case IT_WEAPON_FENCE:		// 14 = can't be used to chop trees. (make kindling)
			pItem->m_itWeapon.m_poison_skill = static_cast<unsigned char>(pPoison->m_itPotion.m_skillquality / 10);
			pItem->UpdatePropertyFlag(AUTOTOOLTIP_FLAG_POISON);
			break;
		default:
			SysMessageDefault( DEFMSG_POISONING_WITEM );
			return( -SKTRIG_QTY );
	}
	// skill + quality of the poison.
	SysMessageDefault( DEFMSG_POISONING_SUCCESS );
	pPoison->ConsumeAmount();
	return( 0 );
}

int CChar::Skill_Cooking( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Cooking");
	// m_atCreate.m_ItemID = create this item
	// m_Act_p = the heat source
	// m_Act_Targ = the skill tool

	int iMaxDist = 3;

	if ( stage == SKTRIG_START )
	{
		m_Act_p = g_World.FindItemTypeNearby( GetTopPoint(), IT_FIRE, iMaxDist, false );
		if ( ! m_Act_p.IsValidPoint())
		{
			m_Act_p = g_World.FindItemTypeNearby( GetTopPoint(), IT_FORGE, iMaxDist, false );
			if ( ! m_Act_p.IsValidPoint())
			{
				m_Act_p = g_World.FindItemTypeNearby( GetTopPoint(), IT_CAMPFIRE, iMaxDist, false );
				if ( ! m_Act_p.IsValidPoint())
				{
					SysMessageDefault( DEFMSG_COOKING_FIRE_SOURCE );
					return( -SKTRIG_QTY );
				}
			}
		}
		UpdateDir( m_Act_p );	// toward the fire source
	}

	if ( stage == SKTRIG_SUCCESS )
	{
		if ( GetTopPoint().GetDist( m_Act_p ) > iMaxDist )
		{
			return( -SKTRIG_FAIL );
		}
	}

	return( Skill_MakeItem( stage ));
}

int CChar::Skill_Taming( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Taming");
	// m_Act_Targ = creature to tame.
	// Check the min required skill for this creature.
	// Related to INT ?

	CChar * pChar = m_Act_Targ.CharFind();
	if ( pChar == NULL )
	{
		return( -SKTRIG_QTY );
	}
	if ( pChar == this )
	{
		SysMessageDefault( DEFMSG_TAMING_YMASTER );
		return( -SKTRIG_QTY );
	}
	if ( pChar->m_pPlayer )
	{
		SysMessageDefault( DEFMSG_TAMING_CANT );
		return( -SKTRIG_QTY );
	}
	if ( GetTopDist3D(pChar) > 10 )
	{
		SysMessageDefault( DEFMSG_TAMING_REACH );
		return -SKTRIG_QTY;
	}
	if ( ! CanSeeLOS( pChar ) )
	{
		SysMessageDefault( DEFMSG_TAMING_LOS );
		return -SKTRIG_QTY;
	}
	UpdateDir( pChar );

	ASSERT( pChar->m_pNPC );

	int iTameBase = pChar->Skill_GetBase(SKILL_TAMING);
	if ( ! IsPriv( PRIV_GM )) // if its a gm doing it, just check that its not
	{
		// Is it tamable ?
		if ( pChar->IsStatFlag( STATF_Pet ))
		{
			SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_TAMING_TAME ), static_cast<LPCTSTR>(pChar->GetName()));
			return( -SKTRIG_QTY );
		}

		// Too smart or not an animal.
		if ( ! iTameBase || pChar->Skill_GetBase(SKILL_ANIMALLORE))
		{
			SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_TAMING_TAMED ), static_cast<LPCTSTR>(pChar->GetName()));
			return( -SKTRIG_QTY );
		}
	}

	if ( stage == SKTRIG_START )
	{
		int iDifficulty = iTameBase/10;

		if ( pChar->Memory_FindObjTypes( this, MEMORY_FIGHT|MEMORY_HARMEDBY|MEMORY_IRRITATEDBY|MEMORY_AGGREIVED ))
		{
			// I've attacked it b4 ?
			iDifficulty += 50;
		}

		m_atTaming.m_Stroke_Count = Calc_GetRandVal( 4 ) + 2;
		return( iDifficulty );
	}

	if ( stage == SKTRIG_FAIL )
	{
		// chance of being attacked ?
		return( 0 );
	}

	if ( stage == SKTRIG_STROKE )
	{
		static LPCTSTR const sm_szTameSpeak[] =
		{
			g_Cfg.GetDefaultMsg( DEFMSG_TAMING_1 ),
			g_Cfg.GetDefaultMsg( DEFMSG_TAMING_2 ),
			g_Cfg.GetDefaultMsg( DEFMSG_TAMING_3 ),
			g_Cfg.GetDefaultMsg( DEFMSG_TAMING_4 )
		};

		if ( IsPriv( PRIV_GM ))
			return( 0 );
		if ( m_atTaming.m_Stroke_Count <= 0 )
			return( 0 );

		TCHAR * pszMsg = Str_GetTemp();
		sprintf(pszMsg, sm_szTameSpeak[ Calc_GetRandVal( COUNTOF( sm_szTameSpeak )) ], static_cast<LPCTSTR>(pChar->GetName()));
		Speak(pszMsg);

		// Keep trying and updating the animation
		m_atTaming.m_Stroke_Count --;
		Skill_SetTimeout();
		return -SKTRIG_STROKE;
	}

	ASSERT( stage == SKTRIG_SUCCESS );

	// Create the memory of being tamed to prevent lame macroers
	CItemMemory * pMemory = pChar->Memory_FindObjTypes( this, MEMORY_SPEAK );
	if ( pMemory && pMemory->m_itEqMemory.m_Action == NPC_MEM_ACT_TAMED)
	{
		// See if I tamed it before
		// I did, no skill to tame it again
		TCHAR * pszMsg = Str_GetTemp();
		sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_TAMING_REMEMBER ), static_cast<LPCTSTR>(pChar->GetName()));
		ObjMessage(pszMsg, pChar );

		pChar->NPC_PetSetOwner( this );
		pChar->Stat_SetVal(STAT_FOOD, 50);	// this is good for something.
		pChar->m_Act_Targ = GetUID();
		pChar->Skill_Start( NPCACT_FOLLOW_TARG );
		return -SKTRIG_QTY;	// no credit for this.
	}

	pChar->NPC_PetSetOwner( this );
	pChar->Stat_SetVal(STAT_FOOD, 50);	// this is good for something.
	pChar->m_Act_Targ = GetUID();
	pChar->Skill_Start( NPCACT_FOLLOW_TARG );
	SysMessageDefault( DEFMSG_TAMING_SUCCESS );

	// Create the memory of being tamed to prevent lame macroers
	pMemory = pChar->Memory_AddObjTypes(this, MEMORY_SPEAK);
	if ( pMemory )
		pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_TAMED;
		
	return 0;
}

int CChar::Skill_Lockpicking( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Lockpicking");
	// m_Act_Targ = the item to be picked.
	// m_Act_TargPrv = The pick.

	if ( stage == SKTRIG_STROKE )
	{
		return 0;
	}

	CItem * pPick = m_Act_TargPrv.ItemFind();
	if ( pPick == NULL || ! pPick->IsType( IT_LOCKPICK ))
	{
		SysMessageDefault( DEFMSG_LOCKPICKING_NOPICK );
		return -SKTRIG_QTY;
	}

	CItem * pLock = m_Act_Targ.ItemFind();
	if ( pLock == NULL )
	{
		SysMessageDefault( DEFMSG_LOCKPICKING_WITEM );
		return -SKTRIG_QTY;
	}

	if ( pPick->GetTopLevelObj() != this )	// the pick is gone !
	{
		SysMessageDefault( DEFMSG_LOCKPICKING_PREACH );
		return -SKTRIG_QTY;
	}

	if ( stage == SKTRIG_FAIL )
	{
		// Damage my pick
		pPick->OnTakeDamage( 1, this, DAMAGE_HIT_BLUNT );
		return( 0 );
	}

	if ( ! CanTouch( pLock ))	// we moved too far from the lock.
	{
		SysMessageDefault( DEFMSG_LOCKPICKING_REACH );
		return -SKTRIG_QTY;
	}

	if (  stage == SKTRIG_START )
	{
		return( pLock->Use_LockPick( this, true, false ));
	}

	ASSERT( stage == SKTRIG_SUCCESS );

	if ( pLock->Use_LockPick( this, false, false ) < 0 )
	{
		return -SKTRIG_FAIL;
	}
	return 0;
}

int CChar::Skill_Hiding( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Hiding");
	// SKILL_Stealth = move while already hidden !
	// SKILL_Hiding
	// Skill required varies with terrain and situation ?
	// if we are carrying a light source then this should not work.

	// We shoud just stay in HIDING skill. ?
	if ( stage == SKTRIG_STROKE ) return 0;
	if ( stage == SKTRIG_FAIL )
	{
		Reveal( STATF_Hidden );
		return 0;
	}

	if ( stage == SKTRIG_SUCCESS )
	{
		if ( IsStatFlag( STATF_Hidden ))
		{
			// I was already hidden ? so un-hide.
			Reveal( STATF_Hidden );
			return( -SKTRIG_ABORT );
		}

		ObjMessage( g_Cfg.GetDefaultMsg( DEFMSG_HIDING_SUCCESS ), this );
		if ( IsClient() )
		{
			GetClient()->removeBuff( BI_HIDDEN );
			GetClient()->addBuff( BI_HIDDEN , 1075655, 1075656, 0 );
		}
		StatFlag_Set( STATF_Hidden );
		UpdateModeFlag();
		return( 0 );
	}

	if ( stage == SKTRIG_START )
	{
		// Make sure i am not carrying a light ?

		CItem * pItem = GetContentHead();
		for ( ; pItem; pItem = pItem->GetNext())
		{
			if ( ! CItemBase::IsVisibleLayer( pItem->GetEquipLayer()))
				continue;
			if ( pItem->Item_GetDef()->Can( CAN_I_LIGHT ))
			{
				SysMessageDefault( DEFMSG_HIDING_TOOLIT );
				return( -SKTRIG_QTY );
			}
		}

		return Calc_GetRandVal(70);
	}
	ASSERT(0);
	return( -SKTRIG_QTY );
}

int CChar::Skill_Herding( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Herding");
	// m_Act_Targ = move this creature.
	// m_Act_p = move to here.
	// How do I make them move fast ? or with proper speed ???

	if ( stage == SKTRIG_STROKE )
		return 0;

	CChar	*pChar = m_Act_Targ.CharFind();
	CItem	*pCrook = m_Act_TargPrv.ItemFind();
	if ( !pChar )
	{
		SysMessageDefault(DEFMSG_HERDING_LTARG);
		return -SKTRIG_QTY;
	}
	if ( !pCrook )
	{
		SysMessageDefault(DEFMSG_HERDING_NOCROOK);
		return -SKTRIG_QTY;
	}

	switch ( stage )
	{
		case SKTRIG_START:
		{
			if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOANIM ) )
			{
				UpdateAnimate(ANIM_ATTACK_WEAPON);
			}
			int iIntVal = pChar->Stat_GetAdjusted(STAT_INT) / 2;
			return iIntVal + Calc_GetRandVal(iIntVal);
		}

		case SKTRIG_FAIL:
		{
			return 0;
		}

		case SKTRIG_SUCCESS:
		{
			if ( IsPriv(PRIV_GM) )
			{
				if ( pChar->GetPrivLevel() > GetPrivLevel() )
				{
					SysMessagef("%s %s", pChar->GetName(), g_Cfg.GetDefaultMsg(DEFMSG_HERDING_PLAYER) );
					return -SKTRIG_ABORT;
				}

				pChar->Spell_Teleport(m_Act_p, true, false);
			}
			else
			{
				if ( pChar->m_pPlayer || !pChar->m_pNPC || pChar->m_pNPC->m_Brain != NPCBRAIN_ANIMAL )
				{
					SysMessagef("%s %s", pChar->GetName(), g_Cfg.GetDefaultMsg(DEFMSG_HERDING_PLAYER) );
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

int CChar::Skill_SpiritSpeak( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_SpiritSpeak");
	if ( stage == SKTRIG_FAIL )
	{
		// bring ghosts ? hehe
		return 0;
	}
	if ( stage == SKTRIG_STROKE )
	{
		return 0;
	}
	if ( stage == SKTRIG_START )
	{
		// difficulty based on spirits near ?
		return( Calc_GetRandVal( 90 ));
	}
	if ( stage == SKTRIG_SUCCESS )
	{
		if ( IsStatFlag( STATF_SpiritSpeak ))
			return( -SKTRIG_ABORT );
		SysMessageDefault( DEFMSG_SPIRITSPEAK_SUCCESS );
		if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
		{
			Sound( 0x24a );
		}
		Spell_Effect_Create( SPELL_NONE, LAYER_FLAG_SpiritSpeak, 1, 4*60*TICK_PER_SEC, this );
		return( 0 );
	}

	ASSERT(0);
	return( -SKTRIG_ABORT );
}

int CChar::Skill_Meditation( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Meditation");
	// SKILL_MEDITATION
	// Try to regen your mana even faster than normal.
	// Give experience only when we max out.

	if ( stage == SKTRIG_FAIL || stage == SKTRIG_ABORT )
	{
		return 0;
	}

	if ( stage == SKTRIG_START )
	{
		if ( Stat_GetVal(STAT_INT) >= Stat_GetMax(STAT_INT))
		{
			SysMessageDefault( DEFMSG_MEDITATION_PEACE_1 );
			return( -SKTRIG_QTY );
		}

		m_atTaming.m_Stroke_Count = 0;

		SysMessageDefault( DEFMSG_MEDITATION_TRY );

		return Calc_GetRandVal(100);	// how hard to get started ?
	}
	if ( stage == SKTRIG_STROKE )
	{
		return 0;
	}
	if ( stage == SKTRIG_SUCCESS )
	{
		if ( Stat_GetVal(STAT_INT) >= Stat_GetMax(STAT_INT))
		{
			SysMessageDefault( DEFMSG_MEDITATION_PEACE_2 );
			return( 0 );	// only give skill credit now.
		}

		if ( m_atTaming.m_Stroke_Count == 0 )
		{
			if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
			{
				Sound( 0x0f9 );
			}
		}
		m_atTaming.m_Stroke_Count++;

		UpdateStatVal( STAT_INT, 1 );

		// next update. (depends on skill)
		Skill_SetTimeout();

		// Set a new possibility for failure ?
		// iDifficulty = Calc_GetRandVal(100);
		return( -SKTRIG_STROKE );
	}
	return( -SKTRIG_QTY );
}

int CChar::Skill_Healing( SKTRIG_TYPE stage )
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
	{
		return 0;
	}

	CItem * pBandage = m_Act_TargPrv.ItemFind();
	if ( pBandage == NULL )
	{
		SysMessageDefault( DEFMSG_HEALING_NOAIDS );
		return -SKTRIG_QTY;
	}
	if ( ! pBandage->IsType(IT_BANDAGE))
	{
		SysMessageDefault( DEFMSG_HEALING_WITEM );
		return -SKTRIG_QTY;
	}

	CObjBase * pObj = m_Act_Targ.ObjFind();
	if ( ! CanTouch(pObj))
	{
		SysMessageDefault( DEFMSG_HEALING_REACH );
		return -SKTRIG_QTY;
	}

	CItemCorpse * pCorpse;	// resurrect by corpse.
	CChar * pChar;
	if ( pObj->IsItem())
	{
		// Corpse ?
		pCorpse = dynamic_cast <CItemCorpse*>(pObj);
		if ( pCorpse == NULL )
		{
			SysMessageDefault( DEFMSG_HEALING_NONCHAR );
			return -SKTRIG_QTY;
		}

		pChar = pCorpse->m_uidLink.CharFind();
	}
	else
	{
		pCorpse = NULL;
		pChar = m_Act_Targ.CharFind();
	}

	if ( pChar == NULL )
	{
		SysMessageDefault( DEFMSG_HEALING_BEYOND );
		return -SKTRIG_QTY;
	}

	if ( GetDist(pObj) > 2 )
	{
		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_HEALING_TOOFAR ), static_cast<LPCTSTR>(pObj->GetName()));
		if ( pChar != this )
		{
			pChar->SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_HEALING_ATTEMPT ),
				static_cast<LPCTSTR>(GetName()), (pCorpse != NULL? static_cast<LPCTSTR>(pCorpse->GetName()) : g_Cfg.GetDefaultMsg( DEFMSG_HEALING_WHO )));
		}
		return -SKTRIG_QTY;
	}

	if ( pCorpse )
	{
		if ( ! pCorpse->IsTopLevel())
		{
			SysMessageDefault( DEFMSG_HEALING_CORPSEG );
			return -SKTRIG_QTY;
		}
		CRegionBase * pRegion = pCorpse->GetTopPoint().GetRegion(REGION_TYPE_AREA|REGION_TYPE_MULTI);
		if ( pRegion == NULL )
		{
			return -SKTRIG_QTY;
		}
		if ( pRegion->IsFlag( REGION_ANTIMAGIC_ALL | REGION_ANTIMAGIC_RECALL_IN | REGION_ANTIMAGIC_TELEPORT ))
		{
			SysMessageDefault( DEFMSG_HEALING_AM );
			if ( pChar != this )
			{
				pChar->SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_HEALING_ATTEMPT ),
					static_cast<LPCTSTR>(GetName()), static_cast<LPCTSTR>(pCorpse->GetName()));
			}
			return -SKTRIG_QTY;
		}
	}
	else if ( pChar->IsStatFlag(STATF_DEAD))
	{
		SysMessageDefault( DEFMSG_HEALING_GHOST );
		return -SKTRIG_QTY;
	}

	if ( ! pChar->IsStatFlag( STATF_Poisoned|STATF_DEAD )
		&& pChar->Stat_GetVal(STAT_STR) >= pChar->Stat_GetMax(STAT_STR) )
	{
		if ( pChar == this )
		{
			SysMessageDefault( DEFMSG_HEALING_HEALTHY );
		}
		else
		{
			SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_HEALING_NONEED ), static_cast<LPCTSTR>(pChar->GetName()));
		}
		return -SKTRIG_QTY;
	}

	if ( stage == SKTRIG_FAIL )
	{
		// just consume the bandage on fail and give some credit for trying.
		pBandage->ConsumeAmount();

		if ( pChar != this )
		{
			pChar->SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_HEALING_ATTEMPTF ),
					static_cast<LPCTSTR>(GetName()), (pCorpse != NULL? static_cast<LPCTSTR>(pCorpse->GetName()) : g_Cfg.GetDefaultMsg( DEFMSG_HEALING_WHO )));
		}

		// Harm the creature ?
		return -SKTRIG_FAIL;
	}

	if ( stage == SKTRIG_START )
	{
		if ( pChar != this )
		{
			TCHAR * pszMsg = Str_GetTemp();
			sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_HEALING_TO ), pChar->GetName());
			Emote(pszMsg);
		}
		else
		{
			Emote( g_Cfg.GetDefaultMsg( DEFMSG_HEALING_SELF ) );
		}
		if ( pCorpse )
		{
			// resurrect.
			return( 85 + Calc_GetRandVal(25));
		}
		if ( pChar->IsStatFlag( STATF_Poisoned ))
		{
			// level of the poison ?
			return( 50 + Calc_GetRandVal(50));
		}
		return Calc_GetRandVal(80);
	}

	ASSERT( stage == SKTRIG_SUCCESS );

	pBandage->ConsumeAmount();

	CItem * pBloodyBandage = CItem::CreateScript(Calc_GetRandVal(2) ? ITEMID_BANDAGES_BLOODY1 : ITEMID_BANDAGES_BLOODY2, this );
	ItemBounce(pBloodyBandage);

	const CSkillDef * pSkillDef = g_Cfg.GetSkillDef(Skill_GetActive());
	if (pSkillDef == NULL)
		return -SKTRIG_QTY;

	int iSkillLevel = Skill_GetAdjusted( Skill_GetActive());

	if ( pCorpse )
	{
		pChar->Spell_Resurrection(pCorpse);
		return 0;
	}
	if ( pChar->IsStatFlag( STATF_Poisoned ))
	{
		if ( ! SetPoisonCure( iSkillLevel, true ))
			return -SKTRIG_ABORT;

		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_HEALING_CURE_1 ), (pChar == this) ? g_Cfg.GetDefaultMsg(DEFMSG_HEALING_YOURSELF) : ( pChar->GetName()));
		if ( pChar != this )
		{
			pChar->SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_HEALING_CURE_2 ), static_cast<LPCTSTR>(GetName()));
		}
		return 0;
	}

	// LAYER_FLAG_Bandage
	pChar->UpdateStatVal( STAT_STR, pSkillDef->m_Effect.GetLinear(iSkillLevel));
	return 0;
}

int CChar::Skill_RemoveTrap( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_RemoveTrap");
	// m_Act_Targ = trap
	// Is it a trap ?

	if ( stage == SKTRIG_STROKE )
	{
		return 0;
	}
	CItem * pTrap = m_Act_Targ.ItemFind();
	if ( pTrap == NULL || ! pTrap->IsType(IT_TRAP))
	{
		SysMessageDefault( DEFMSG_REMOVETRAPS_WITEM );
		return( -SKTRIG_QTY );
	}
	if ( ! CanTouch(pTrap))
	{
		SysMessageDefault( DEFMSG_REMOVETRAPS_REACH );
		return( -SKTRIG_QTY );
	}
	if ( stage == SKTRIG_START )
	{
		// How difficult ?
		return Calc_GetRandVal(95);
	}
	if ( stage == SKTRIG_FAIL )
	{
		Use_Item( pTrap );	// set it off ?
		return 0;
	}
	if ( stage == SKTRIG_SUCCESS )
	{
		// disable it.
		pTrap->SetTrapState( IT_TRAP_INACTIVE, ITEMID_NOTHING, 5*60 );
		return 0;
	}
	ASSERT(0);
	return( -SKTRIG_QTY );
}

int CChar::Skill_Begging( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Begging");
	// m_Act_Targ = Our begging target..

	CChar * pChar = m_Act_Targ.CharFind();
	if ( pChar == NULL || pChar == this )
	{
		return( -SKTRIG_QTY );
	}

	if ( stage == SKTRIG_START )
	{
		UpdateAnimate( ANIM_BOW );
		SysMessagef(g_Cfg.GetDefaultMsg( DEFMSG_BEGGING_START ), static_cast<LPCTSTR>(pChar->GetName()));
		return( pChar->Stat_GetAdjusted(STAT_INT));
	}
	if ( stage == SKTRIG_STROKE )
	{
		if ( m_pNPC )
			return -SKTRIG_STROKE;	// Keep it active.
		return( 0 );
	}
	if ( stage == SKTRIG_FAIL )
	{
		// Might they do something bad ?
		return( 0 );
	}
	if ( stage == SKTRIG_SUCCESS )
	{
		// Now what ? Not sure how to make begging successful.
		// Give something from my inventory ?

		return( 0 );
	}

	ASSERT(0);
	return( -SKTRIG_QTY );
}

int CChar::Skill_Magery( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Magery");
	// SKILL_MAGERY
	//  m_Act_p = location to cast to.
	//  m_Act_TargPrv = the source of the spell.
	//  m_Act_Targ = target for the spell.
	//  m_atMagery.m_Spell = the spell.

	if ( stage == SKTRIG_STROKE )
	{
		return( 0 );
	}
	if ( stage == SKTRIG_FAIL )
	{
		Spell_CastFail();
		return( 0 );
	}
	if ( stage == SKTRIG_SUCCESS )
	{
		const CSpellDef *		tSpell	= g_Cfg.GetSpellDef( m_atMagery.m_Spell );
		if (tSpell == NULL)
			return( 0 );

		if ( IsClient() && IsSetMagicFlags( MAGICF_PRECAST ) && !tSpell->IsSpellType( SPELLFLAG_NOPRECAST ))
		{
			this->GetClient()->Cmd_Skill_Magery( this->m_atMagery.m_Spell, this->GetClient()->m_Targ_PrvUID.ObjFind() );

			// Don't increase skill at this point. The client
			// should select a target first.
			return( -SKTRIG_QTY );
		}
		else
		{
			if ( ! Spell_CastDone())
			{
				return( -SKTRIG_ABORT );
			}
			return( 0 );
		}
	}
	if ( stage == SKTRIG_START )
	{
		// NOTE: this should call SetTimeout();
		return Spell_CastStart();
	}

	ASSERT(0);
	return( -SKTRIG_ABORT );
}

int CChar::Skill_Fighting( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Fighting");
	// SKILL_ARCHERY
	// SKILL_THROWING
	// m_Act_Targ = attack target.

	if ( stage == SKTRIG_START )
	{
		// When do we get our next shot?

		m_atFight.m_War_Swing_State = WAR_SWING_EQUIPPING;
		m_atFight.m_fMoved	= 0;
		int iDifficulty = g_Cfg.Calc_CombatChanceToHit( this, Skill_GetActive(), m_Act_Targ.CharFind(), m_uidWeapon.ItemFind());

		// Set the swing timer.
		int iWaitTime = Fight_GetWeaponSwingTimer()/2;	// start the anim immediately.
		if ( g_Cfg.IsSkillRanged(Skill_GetActive()) )	// anim is funny for archery
			iWaitTime /= 2;
		SetTimeout( iWaitTime );

		return( iDifficulty );
	}

	if ( stage == SKTRIG_STROKE )
	{
		// Hit or miss my current target.
		if ( ! IsStatFlag( STATF_War ))
			return -SKTRIG_ABORT;

		if ( m_atFight.m_War_Swing_State != WAR_SWING_SWINGING )
		{
			m_atFight.m_War_Swing_State = WAR_SWING_READY;  // Waited my recoil time. So I'm ready.
		}

		Fight_HitTry();	// this cleans up itself.
		return -SKTRIG_STROKE;	// Stay in the skill till we hit.
	}

	return -SKTRIG_QTY;
}

int CChar::Skill_MakeItem( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_MakeItem");
	// SKILL_ALCHEMY:
	// SKILL_BLACKSMITHING:
	// SKILL_BOWCRAFT:
	// SKILL_CARPENTRY:
	// SKILL_COOKING:
	// SKILL_INSCRIPTION:
	// SKILL_TAILORING:
	// SKILL_TINKERING:
	//
	// m_Act_Targ = the item we want to be part of this process.
	// m_atCreate.m_ItemID = new item we are making
	// m_atCreate.m_Amount = amount of said item.
	
	if ( stage == SKTRIG_START )
	{
		return m_Act_Difficulty;	// keep the already set difficulty
	}
	if ( stage == SKTRIG_SUCCESS )
	{
		if ( ! Skill_MakeItem( m_atCreate.m_ItemID, m_Act_Targ, SKTRIG_SUCCESS, false, (m_atCreate.m_Amount ? m_atCreate.m_Amount : 1) ))
			return( -SKTRIG_ABORT );
		return 0;
	}
	if ( stage == SKTRIG_FAIL )
	{
		Skill_MakeItem( m_atCreate.m_ItemID, m_Act_Targ, SKTRIG_FAIL );
		return( 0 );
	}
	ASSERT(0);
	return( -SKTRIG_QTY );
}

int CChar::Skill_Tailoring( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Tailoring");
	if ( stage == SKTRIG_SUCCESS )
	{
		if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
		{
			Sound( SOUND_SNIP );	// snip noise
		}
	}

	return( Skill_MakeItem( stage ));
}

int CChar::Skill_Inscription( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Inscription");
	if ( stage == SKTRIG_START )
	{
		// Can we even attempt to make this scroll ?
		// m_atCreate.m_ItemID = create this item
		if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
		{
			Sound( 0x249 );
		}
	}

	return( Skill_MakeItem( stage ));
}

int CChar::Skill_Bowcraft( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Bowcraft");
	// SKILL_BOWCRAFT
	// m_Act_Targ = the item we want to be part of this process.
	// m_atCreate.m_ItemID = new item we are making
	// m_atCreate.m_Amount = amount of said item.

	return( Skill_MakeItem( stage ));
}

int CChar::Skill_Blacksmith( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Blacksmith");
	// m_atCreate.m_ItemID = create this item
	// m_Act_p = the anvil.
	// m_Act_Targ = the hammer.

	int iMaxDist = 2;

	if ( stage == SKTRIG_START )
	{
		m_Act_p = g_World.FindItemTypeNearby( GetTopPoint(), IT_FORGE, iMaxDist, false );
		if ( ! m_Act_p.IsValidPoint())
		{
			SysMessageDefault( DEFMSG_SMITHING_FORGE );
			return( -SKTRIG_QTY );
		}
		UpdateDir( m_Act_p );	// toward the forge

		m_atCreate.m_Stroke_Count = /*Calc_GetRandVal( 4 ) +*/ 2;
	}

	if ( stage == SKTRIG_SUCCESS )
	{
		if ( GetTopPoint().GetDist( m_Act_p ) > iMaxDist )
		{
			return( -SKTRIG_FAIL );
		}
	}

	return( Skill_MakeItem( stage ));
}

int CChar::Skill_Carpentry( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Carpentry");
	// m_Act_Targ = the item we want to be part of this process.
	// m_atCreate.m_ItemID = new item we are making
	// m_atCreate.m_Amount = amount of said item.

	if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
	{
		Sound( 0x23d );
	}

	if ( stage == SKTRIG_START )
	{
		// m_atCreate.m_ItemID = create this item
		m_atCreate.m_Stroke_Count = /*Calc_GetRandVal( 3 ) +*/ 2;
	}

	return( Skill_MakeItem( stage ));
}

int CChar::Skill_Scripted( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Scripted");
	if ( stage == SKTRIG_FAIL )
	{
		return 0;
	}
	if ( stage == SKTRIG_STROKE )
	{
		return 0;
	}
	if ( stage == SKTRIG_START )
	{
		return 0;
	}
	if ( stage == SKTRIG_SUCCESS )
	{
		return 0;
	}

	return( -SKTRIG_QTY );	// something odd
}

int CChar::Skill_Information( SKTRIG_TYPE stage )
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

	if ( ! IsClient())	// purely informational
		return( -SKTRIG_QTY );

	if ( stage == SKTRIG_FAIL )
	{
		return 0;
	}
	if ( stage == SKTRIG_STROKE )
	{
		return 0;
	}

	SKILL_TYPE skill = Skill_GetActive();
	int iSkillLevel = Skill_GetAdjusted(skill);

	if ( stage == SKTRIG_START )
	{
		return GetClient()->OnSkill_Info( skill, m_Act_Targ, iSkillLevel, true );
	}
	if ( stage == SKTRIG_SUCCESS )
	{
		return GetClient()->OnSkill_Info( skill, m_Act_Targ, iSkillLevel, false );
	}

	ASSERT(0);
	return( -SKTRIG_QTY );
}

int CChar::Skill_Act_Napping( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Act_Napping");
	// NPCACT_Napping:
	// we are taking a small nap. keep napping til we wake. (or move)
	// AFK command

	if ( stage == SKTRIG_START )
	{
		// we are taking a small nap.
		SetTimeout( 2*TICK_PER_SEC );
		return( 0 );
	}

	if ( stage == SKTRIG_STROKE )
	{
		if ( m_Act_p != GetTopPoint())
			return( -SKTRIG_QTY );	// we moved.
		SetTimeout( 8*TICK_PER_SEC );
		Speak( "z", HUE_WHITE, TALKMODE_WHISPER );
		return -SKTRIG_STROKE;	// Stay in the skill till we hit.
	}

	return( -SKTRIG_QTY );	// something odd
}

int CChar::Skill_Act_Breath( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Act_Breath");
	// NPCACT_BREATH
	// A Dragon I assume.
	// m_Act_Targ = my target.

	if ( stage == SKTRIG_STROKE )
	{
		return 0;
	}
	if ( stage == SKTRIG_FAIL )
	{
		return 0;
	}

	CChar * pChar = m_Act_Targ.CharFind();
	if ( pChar == NULL )
	{
		return -SKTRIG_QTY;
	}

	m_Act_p = pChar->GetTopPoint();
	UpdateDir( m_Act_p );

	if ( stage == SKTRIG_START )
	{
		UpdateStatVal( STAT_DEX, -10 );
		if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOANIM ) )
		{
			UpdateAnimate( ANIM_MON_Stomp );
		}
		SetTimeout( 3*TICK_PER_SEC );
		return 0;
	}

	ASSERT( stage == SKTRIG_SUCCESS );

	CPointMap pntMe = GetTopPoint();
	if ( pntMe.GetDist( m_Act_p ) > UO_MAP_VIEW_SIGHT )
	{
		m_Act_p.StepLinePath( pntMe, UO_MAP_VIEW_SIGHT );
	}

	if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
	{
		Sound( 0x227 );
	}

	// Set damage value
	int iDamage = ( Stat_GetVal(STAT_STR) * 16 ) / 100;
	if ( iDamage > 200 )
		iDamage = 200;

	const CVarDefCont * pTagStorage = GetDefKey("BREATH.DAM", true);
	if ( pTagStorage )
	{
		if ( pTagStorage->GetValNum() )
			iDamage = static_cast<int>(pTagStorage->GetValNum());
	}
	g_World.Explode( this, m_Act_p, 3, iDamage, DAMAGE_FIRE | DAMAGE_GENERAL );
	return( 0 );
}

int CChar::Skill_Act_Looting( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Act_Looting");
	// NPCACT_LOOTING
	// m_Act_Targ = the item i want.

	if ( stage == SKTRIG_STROKE )
	{
		return 0;
	}
	if ( stage == SKTRIG_START )
	{
		if (( m_atLooting.m_iDistCurrent == 0 ) && ( IsTrigUsed(TRIGGER_NPCSEEWANTITEM) ))
		{
			CScriptTriggerArgs Args( m_Act_Targ.ItemFind());
			if ( OnTrigger( CTRIG_NPCSeeWantItem, this, &Args ) == TRIGRET_RET_TRUE )
				return( -SKTRIG_QTY );
		}

		SetTimeout( 1 * TICK_PER_SEC );
		return 0;
	}

	return( -SKTRIG_QTY );
}

int CChar::Skill_Act_Throwing( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Act_Throwing");
	// NPCACT_THROWING
	// m_Act_Targ = my target.

	if ( stage == SKTRIG_STROKE )
	{
		return 0;
	}
	CChar * pChar = m_Act_Targ.CharFind();
	if ( pChar == NULL )
	{
		return( -SKTRIG_QTY );
	}

	m_Act_p = pChar->GetTopPoint();
	UpdateDir( m_Act_p );

	if ( stage == SKTRIG_START )
	{
		UpdateStatVal( STAT_DEX, -( 4 + Calc_GetRandVal(6)));
		if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOANIM ) )
		{
			UpdateAnimate( ANIM_MON_Stomp );
		}
		return 0;
	}

	if ( stage != SKTRIG_SUCCESS )
	{
		return( -SKTRIG_QTY );
	}

	CPointMap pntMe = GetTopPoint();
	if ( pntMe.GetDist( m_Act_p ) > UO_MAP_VIEW_SIGHT )
	{
		m_Act_p.StepLinePath( pntMe, UO_MAP_VIEW_SIGHT );
	}
	SoundChar( CRESND_GETHIT );

	// a rock or a boulder ?
	ITEMID_TYPE id = ITEMID_NOTHING;
	int iDamage = 0;

	CVarDefCont * pDam = GetDefKey("THROWDAM",true);
	if ( pDam )
	{
		INT64 DVal[2];
		size_t iQty = Str_ParseCmds( const_cast<TCHAR*>(pDam->GetValStr()), DVal, COUNTOF(DVal));
		switch(iQty)
		{
			case 1:
				iDamage = static_cast<int>(DVal[0]);
				break;
			case 2:
				iDamage = static_cast<int>(DVal[0] + Calc_GetRandLLVal( DVal[1] - DVal[0] ));
				break;
		}
	}

	CVarDefCont * pRock = GetDefKey("THROWOBJ",true);
    if ( pRock )
	{
		LPCTSTR t_Str = pRock->GetValStr();
		RESOURCE_ID_BASE rid = static_cast<RESOURCE_ID_BASE>(g_Cfg.ResourceGetID( RES_ITEMDEF, t_Str ));
		id = static_cast<ITEMID_TYPE>(rid.GetResIndex());
		if (!iDamage)
			iDamage = Stat_GetVal(STAT_DEX)/4 + Calc_GetRandVal( Stat_GetVal(STAT_DEX)/4 );
	}
	else
	{
		if ( Calc_GetRandVal( 3 ) )
		{
			id = static_cast<ITEMID_TYPE>(ITEMID_ROCK_B_LO + Calc_GetRandVal(ITEMID_ROCK_B_HI-ITEMID_ROCK_B_LO));
			if (!iDamage)
				iDamage = Stat_GetVal(STAT_DEX)/4 + Calc_GetRandVal( Stat_GetVal(STAT_DEX)/4 );
		}
		else
		{
			id = static_cast<ITEMID_TYPE>(ITEMID_ROCK_2_LO + Calc_GetRandVal(ITEMID_ROCK_2_HI-ITEMID_ROCK_2_LO));
			if (!iDamage)
				iDamage = 2 + Calc_GetRandVal( Stat_GetVal(STAT_DEX)/4 );
		}
	}

	if ( id != ITEMID_NOTHING )
	{
		CItem *pRock = CItem::CreateScript(id, this);
		if ( pRock )
		{
			pRock->SetAttr(ATTR_DECAY);
			pRock->MoveToCheck(m_Act_p, this);
			pRock->Effect(EFFECT_BOLT, id, this);
		}
		// did it hit ?
		if ( ! Calc_GetRandVal( pChar->GetTopPoint().GetDist( m_Act_p )))
		{
			pChar->OnTakeDamage( iDamage, this, DAMAGE_HIT_BLUNT );
		}
	}
	
	return 0;
}

int CChar::Skill_Act_Training( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Act_Training");
	// NPCACT_TRAINING
	// finished some traing maneuver.

	if ( stage == SKTRIG_START )
	{
		SetTimeout( 1*TICK_PER_SEC );
		return 0;
	}
	if ( stage == SKTRIG_STROKE )
	{
		return 0;
	}
	if ( stage != SKTRIG_SUCCESS )
	{
		return( -SKTRIG_QTY );
	}

	if ( m_Act_TargPrv == m_uidWeapon )
	{
		CItem * pItem = m_Act_Targ.ItemFind();
		if ( pItem )
		{
			switch ( pItem->GetType())
			{
				case IT_TRAIN_DUMMY:	// Train dummy.
					Use_Train_Dummy(pItem, false);
					break;
				case IT_TRAIN_PICKPOCKET:
					Use_Train_PickPocketDip(pItem, false);
					break;
				case IT_ARCHERY_BUTTE:	// Archery Butte
					Use_Train_ArcheryButte(pItem, false);
					break;
				default:
					break;
			}
		}
	}

	return 0;
}

//************************************
// General skill stuff.
ANIM_TYPE CChar::Skill_GetAnim( SKILL_TYPE skill )
{
	ANIM_TYPE anim = (ANIM_TYPE)-1;
	switch ( skill )
	{
		/*case SKILL_FISHING:		//softcoded
			anim = ANIM_ATTACK_2H_DOWN;
			break;*/
		case SKILL_BLACKSMITHING:
			anim = ANIM_ATTACK_WEAPON;
			break;
		case SKILL_MINING:
			anim = ANIM_ATTACK_1H_DOWN;
			break;
		case SKILL_LUMBERJACKING:
			anim = ANIM_ATTACK_2H_WIDE;
			break;
		default:
			break;
	}
	return anim;
}

int CChar::Skill_GetSound( SKILL_TYPE skill )
{
	int sound = 0;
	switch ( skill )
	{
		/*case SKILL_FISHING:	//softcoded
			sound = 0x364;
			break;
		case SKILL_TINKERING:	//old value
			sound = 0x241;
			break;*/
		case SKILL_ALCHEMY:
			sound = 0x242;
			break;
		case SKILL_TAILORING:
			sound = 0x248;
			break;
		case SKILL_INSCRIPTION:
			sound = 0x249;
			break;
		case SKILL_BOWCRAFT:
			sound = 0x055;
			break;
		case SKILL_BLACKSMITHING:
			sound = 0x02a;
			break;
		case SKILL_CARPENTRY:
			sound = 0x23d;
			break;
		case SKILL_CARTOGRAPHY:
			sound = 0x249;
			break;
		case SKILL_MINING:
			sound = Calc_GetRandVal(2) ? 0x125 : 0x126;
			break;
		case SKILL_LUMBERJACKING:
			sound = 0x13e;
			break;
		default:
			break;
	}
	return sound;
}

int CChar::Skill_Stroke( bool fResource )
{
	// fResource means decreasing m_atResource.m_Stroke_Count instead of m_atCreate.m_Stroke_Count
	int sound = -1;
	ANIM_TYPE anim = static_cast<ANIM_TYPE>(-1);
	SKILL_TYPE skill = Skill_GetActive();
	if ( m_atCreate.m_Stroke_Count > 1 )
	{
		if ( !g_Cfg.IsSkillFlag( skill, SKF_NOSFX ) )
		{
			sound = Skill_GetSound( skill );
		}
		if ( !g_Cfg.IsSkillFlag( skill, SKF_NOANIM ) )
		{
			anim = Skill_GetAnim( skill );
		}
	}
	INT64 delay = Skill_GetTimeout();
	if ( IsTrigUsed(TRIGGER_SKILLSTROKE))
	{
		CScriptTriggerArgs args;
		args.m_VarsLocal.SetNum("Skill", skill);
		if ( fResource )
			args.m_VarsLocal.SetNum("Strokes",m_atResource.m_Stroke_Count);
		else
			args.m_VarsLocal.SetNum("Strokes", m_atCreate.m_Stroke_Count);
		args.m_VarsLocal.SetNum("Sound", sound);
		args.m_VarsLocal.SetNum("Delay",delay);
		args.m_VarsLocal.SetNum("Anim", static_cast<INT64>(anim));

		if ( OnTrigger( CTRIG_SkillStroke, this, &args ) == TRIGRET_RET_TRUE ) 
			return(-SKTRIG_ABORT);
		if ( Skill_OnTrigger( skill, SKTRIG_STROKE, &args ) == TRIGRET_RET_TRUE )
			return(-SKTRIG_ABORT);

		sound = static_cast<int>(args.m_VarsLocal.GetKeyNum("Sound",false));
		if ( fResource )
			m_atResource.m_Stroke_Count = static_cast<WORD>(args.m_VarsLocal.GetKeyNum("Strokes",false));
		else
			m_atCreate.m_Stroke_Count = static_cast<WORD>(args.m_VarsLocal.GetKeyNum("Strokes",false));
		delay = args.m_VarsLocal.GetKeyNum("Delay",true);
		anim = static_cast<ANIM_TYPE>(args.m_VarsLocal.GetKeyNum("Anim", true));
	}

	if ( sound )
		Sound(sound);

	// Keep trying and updating the animation
	if ( anim )
		UpdateAnimate( anim );

	if ( delay < 10)
		delay = 10;
	if ( fResource )
	{
		if ( m_atResource.m_Stroke_Count )
			m_atResource.m_Stroke_Count --;
		if ( m_atResource.m_Stroke_Count < 1 )
			return SKTRIG_SUCCESS;

	}else
	{
		if ( m_atCreate.m_Stroke_Count )
			m_atCreate.m_Stroke_Count --;
		if ( m_atCreate.m_Stroke_Count < 1 )
			return( SKTRIG_SUCCESS );
	}
	SetTimeout(delay);
	//Skill_SetTimeout();	//Old behaviour, removed to keep up dynamic delay coming in with the trigger @SkillStroke
	return( -SKTRIG_STROKE );	// keep active.
}

int CChar::Skill_Stage( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Stage");
	if ( stage == SKTRIG_STROKE && g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_CRAFT ))
		return(Skill_Stroke( false ));
	else if ( stage == SKTRIG_STROKE && g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_GATHER ))
	{
		UpdateDir( m_Act_p );
		return(Skill_Stroke( true ));
	}

	if ( g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_SCRIPTED ) )
		return Skill_Scripted( stage );
	else if ( g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_FIGHT ) )
		return Skill_Fighting(stage);
	else if ( g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_MAGIC ) )
		return Skill_Magery(stage);
	/*else if ( g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_CRAFT ) )
		return Skill_MakeItem(stage);*/
	else switch ( Skill_GetActive() )
	{
		case SKILL_NONE:	// idling.
			return 0;
		case SKILL_ALCHEMY:
			return Skill_Alchemy(stage);
		case SKILL_ANATOMY:
		case SKILL_ANIMALLORE:
		case SKILL_ITEMID:
		case SKILL_ARMSLORE:
			return Skill_Information(stage);
		case SKILL_PARRYING:
			return 0;
		case SKILL_BEGGING:
			return Skill_Begging(stage);
		case SKILL_BLACKSMITHING:
			return Skill_Blacksmith(stage);
		case SKILL_BOWCRAFT:
			return Skill_Bowcraft(stage);
		case SKILL_PEACEMAKING:
			return Skill_Peacemaking(stage);
		case SKILL_CAMPING:
			return 0;
		case SKILL_CARPENTRY:
			return Skill_Carpentry(stage);
		case SKILL_CARTOGRAPHY:
			return Skill_Cartography(stage);
		case SKILL_COOKING:
			return Skill_Cooking(stage);
		case SKILL_DETECTINGHIDDEN:
			return Skill_DetectHidden(stage);
		case SKILL_ENTICEMENT:
			return Skill_Enticement(stage);
		case SKILL_EVALINT:
			return Skill_Information(stage);
		case SKILL_HEALING:
			return Skill_Healing(stage);
		case SKILL_FISHING:
			return Skill_Fishing(stage);
		case SKILL_FORENSICS:
			return Skill_Information(stage);
		case SKILL_HERDING:
			return Skill_Herding(stage);
		case SKILL_HIDING:
			return Skill_Hiding(stage);
		case SKILL_PROVOCATION:
			return Skill_Provocation(stage);
		case SKILL_INSCRIPTION:
			return Skill_Inscription(stage);
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
		case SKILL_MAGICRESISTANCE:
			return 0;
		case SKILL_TACTICS:
			return 0;
		case SKILL_SNOOPING:
			return Skill_Snooping(stage);
		case SKILL_MUSICIANSHIP:
			return Skill_Musicianship(stage);
		case SKILL_POISONING:	// 30
			return Skill_Poisoning(stage);
		case SKILL_THROWING:
		case SKILL_ARCHERY:
			return Skill_Fighting(stage);
		case SKILL_SPIRITSPEAK:
			return Skill_SpiritSpeak(stage);
		case SKILL_STEALING:
			return Skill_Stealing(stage);
		case SKILL_TAILORING:
			return Skill_Tailoring(stage);
		case SKILL_TAMING:
			return Skill_Taming(stage);
		case SKILL_TASTEID:
			return Skill_Information(stage);
		case SKILL_TINKERING:
			return Skill_MakeItem(stage);
		case SKILL_TRACKING:
			return Skill_Tracking(stage);
		case SKILL_VETERINARY:
			return Skill_Healing(stage);
		case SKILL_SWORDSMANSHIP:
		case SKILL_MACEFIGHTING:
		case SKILL_FENCING:
		case SKILL_WRESTLING:
			return Skill_Fighting(stage);
		case SKILL_LUMBERJACKING:
			return Skill_Lumberjack(stage);
		case SKILL_MINING:
			return Skill_Mining(stage);
		case SKILL_MEDITATION:
			return Skill_Meditation(stage);
		case SKILL_STEALTH:
			return Skill_Hiding(stage);
		case SKILL_REMOVETRAP:
			return Skill_RemoveTrap(stage);
		case NPCACT_BREATH:
			return Skill_Act_Breath(stage);
		case NPCACT_LOOTING:
			return Skill_Act_Looting(stage);
		case NPCACT_THROWING:
			return Skill_Act_Throwing(stage);
		case NPCACT_TRAINING:
			return Skill_Act_Training(stage);
		case NPCACT_Napping:
			return Skill_Act_Napping(stage);

		default:
			if ( ! IsSkillBase(Skill_GetActive()))
			{
				if ( stage == SKTRIG_STROKE )
					return( -SKTRIG_STROKE ); // keep these active. (NPC modes)
				return 0;
			}
	}

	SysMessageDefault( DEFMSG_SKILL_NOSKILL );
	return -SKTRIG_QTY;
}

void CChar::Skill_Fail( bool fCancel )
{
	ADDTOCALLSTACK("CChar::Skill_Fail");
	// This is the normal skill check failure.
	// Other types of failure don't come here.
	//
	// ARGS:
	//	fCancel = no credt.
	//  else We still get some credit for having tried.

	SKILL_TYPE skill = Skill_GetActive();
	if ( skill == SKILL_NONE )
		return;

	if ( ! IsSkillBase(skill))
	{
		Skill_Cleanup();
		return;
	}

	if ( m_Act_Difficulty > 0 )
	{
		m_Act_Difficulty = - m_Act_Difficulty;
	}

	if ( !fCancel )
	{
		if ( IsTrigUsed(TRIGGER_SKILLFAIL) )
		{
			if ( Skill_OnCharTrigger( skill, CTRIG_SkillFail ) == TRIGRET_RET_TRUE )
				fCancel	= true;
		}
		if (( IsTrigUsed(TRIGGER_FAIL) ) && ( !fCancel ))
		{
			if ( Skill_OnTrigger( skill, SKTRIG_FAIL ) == TRIGRET_RET_TRUE )
				fCancel	= true;
		}
	}
	else
	{
		if ( IsTrigUsed(TRIGGER_SKILLABORT) )
		{
			if ( Skill_OnCharTrigger( skill, CTRIG_SkillAbort ) == TRIGRET_RET_TRUE )
			{
				Skill_Cleanup();
				return;
			}
		}
		if ( IsTrigUsed(TRIGGER_ABORT) )
		{
			if ( Skill_OnTrigger( skill, SKTRIG_ABORT ) == TRIGRET_RET_TRUE )
			{
				Skill_Cleanup();
				return;
			}
		}
	}

	if ( Skill_Stage( SKTRIG_FAIL ) >= 0 )
	{
		// Get some experience for failure ?
		if ( !fCancel )
			Skill_Experience( skill, m_Act_Difficulty );
	}

	Skill_Cleanup();
}

TRIGRET_TYPE	CChar::Skill_OnTrigger( SKILL_TYPE skill, SKTRIG_TYPE  stage) 
{
	CScriptTriggerArgs pArgs;
	return Skill_OnTrigger(skill,stage,&pArgs);
}

TRIGRET_TYPE	CChar::Skill_OnCharTrigger( SKILL_TYPE skill, CTRIG_TYPE  stage) 
{
	CScriptTriggerArgs pArgs;
	return Skill_OnCharTrigger(skill,stage,&pArgs);
}


TRIGRET_TYPE	CChar::Skill_OnTrigger( SKILL_TYPE skill, SKTRIG_TYPE  stage, CScriptTriggerArgs * pArgs ) 
{
	ADDTOCALLSTACK("CChar::Skill_OnTrigger");
	if ( !IsSkillBase(skill) )
		return TRIGRET_RET_DEFAULT;

	if ( ! (stage == SKTRIG_SELECT || stage == SKTRIG_GAIN || stage == SKTRIG_USEQUICK || stage == SKTRIG_WAIT || stage == SKTRIG_TARGETCANCEL ) )
		m_Act_SkillCurrent = skill;

	pArgs->m_iN1 = skill;
	TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;

	CSkillDef* pSkillDef = g_Cfg.GetSkillDef(skill);
	if ( pSkillDef != NULL && pSkillDef->HasTrigger( stage ) )
	{
		// RES_SKILL
		CResourceLock s;
		if ( pSkillDef->ResourceLock( s ))
			iRet = CScriptObj::OnTriggerScript( s, CSkillDef::sm_szTrigName[stage], this, pArgs );
	}

	return iRet;
}

TRIGRET_TYPE	CChar::Skill_OnCharTrigger( SKILL_TYPE skill, CTRIG_TYPE ctrig, CScriptTriggerArgs * pArgs ) 
{
	ADDTOCALLSTACK("CChar::Skill_OnCharTrigger");
	if ( !IsSkillBase(skill) )
		return TRIGRET_RET_DEFAULT;

	if ( ! (ctrig == CTRIG_SkillSelect || ctrig == CTRIG_SkillGain || ctrig == CTRIG_SkillUseQuick || ctrig == CTRIG_SkillWait || ctrig == CTRIG_SkillTargetCancel ) )
		m_Act_SkillCurrent = skill;

	pArgs->m_iN1 = skill;

	return OnTrigger( ctrig, this, pArgs );
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
	//   0 = success
	//	 -SKTRIG_STROKE = stay in skill. (stroke)
	//   -SKTRIG_FAIL = we must print the fail msg. (credit for trying)
	//   -SKTRIG_ABORT = we must print the fail msg. (But get no credit, canceled )
	//   -SKTRIG_QTY = special failure. clean up the skill but say nothing. (no credit)

	SKILL_TYPE skill = Skill_GetActive();
	if ( skill == SKILL_NONE )	// we should not be coming here (timer should not have expired)
		return -SKTRIG_QTY;

	// multi stroke tried stuff here first.
	// or stuff that never really fails.
	int iRet = Skill_Stage(SKTRIG_STROKE);
	if ( iRet < 0 )
		return( iRet );

	if ( m_Act_Difficulty < 0 )
	{
		// Was Bound to fail. But we had to wait for the timer anyhow.
		return -SKTRIG_FAIL;
	}

	//ACTARG2 will override fishing/mining/lumberjacking packing/grounding resources
	//ACTARG2 = 1 //pack resources
	//ACTARG2 = 0 //put resources on ground
	//override those values on both @Success triggers to change the original behavior
	//ps: ACTARG3 is the spawned used to harvest
	if (skill == SKILL_FISHING)
	{
		m_atUnk.m_Arg2=0;
	}
	else if (skill == SKILL_MINING || skill == SKILL_LUMBERJACKING)
	{
		m_atUnk.m_Arg2=1;
	}


	if ( IsTrigUsed(TRIGGER_SKILLSUCCESS) )
	{
		if ( Skill_OnCharTrigger( skill, CTRIG_SkillSuccess ) == TRIGRET_RET_TRUE )
		{
			Skill_Cleanup();
			return -SKTRIG_ABORT;
		}
	}
	
	if ( IsTrigUsed(TRIGGER_SUCCESS) )
	{
		if ( Skill_OnTrigger( skill, SKTRIG_SUCCESS ) == TRIGRET_RET_TRUE )
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
	Skill_Experience( skill, m_Act_Difficulty );
	Skill_Cleanup();

	return( -SKTRIG_SUCCESS );
}

bool CChar::Skill_Wait( SKILL_TYPE skilltry )
{
	ADDTOCALLSTACK("CChar::Skill_Wait");
	// Some sort of push button skill.
	// We want to do some new skill. Can we ?
	// If this is the same skill then tell them to wait.
	CScriptTriggerArgs pArgs(skilltry, Skill_GetActive());

	if ( IsTrigUsed(TRIGGER_SKILLWAIT) )
	{
		switch ( Skill_OnCharTrigger( skilltry, CTRIG_SkillWait, &pArgs ))
		{
			case TRIGRET_RET_TRUE:
				return true;
			case TRIGRET_RET_FALSE:
				Skill_Fail( true );
				return false;
			default:
				break;
		}
	}
	if ( IsTrigUsed(TRIGGER_WAIT) )
	{
		switch ( Skill_OnTrigger( skilltry, SKTRIG_WAIT, &pArgs ))
		{
			case TRIGRET_RET_TRUE:
				return true;
			case TRIGRET_RET_FALSE:
				Skill_Fail( true );
				return false;
			default:
				break;
		}
	}

	if ( IsStatFlag( STATF_DEAD | STATF_Sleeping | STATF_Freeze | STATF_Stone ))
	{
		SysMessageDefault( DEFMSG_SKILLWAIT_1 );
		return( true );
	}

	SKILL_TYPE skill = Skill_GetActive();
	if ( skill == SKILL_NONE )	// not currently doing anything.
	{
		Reveal();
		return( false );
	}

	// What if we are in combat mode ?
	if ( IsStatFlag( STATF_War ))
	{
		SysMessageDefault( DEFMSG_SKILLWAIT_2 );
		return( true );
	}

	// Passive skills just cancel.
	// SKILL_SPIRITSPEAK ?
	if ( skilltry != skill )
	{
		if ( skill == SKILL_MEDITATION || skill == SKILL_HIDING || skill == SKILL_STEALTH )
		{
			Skill_Fail( true );
			return( false );
		}
	}

	SysMessageDefault( DEFMSG_SKILLWAIT_3 );
	return ( true );
}

bool CChar::Skill_Start( SKILL_TYPE skill, int iDifficulty )
{
	ADDTOCALLSTACK("CChar::Skill_Start");
	// We have all the info we need to do the skill. (targeting etc)
	// Set up how long we have to wait before we get the desired results from this skill.
	// Set up any animations/sounds in the mean time.
	// Calc if we will succeed or fail.
	// ARGS:
	//  iDifficulty = 0-100
	// RETURN:
	//  false = failed outright with no wait. "You have no chance of taming this"

	if ( g_Serv.IsLoading() )
	{
		if ( skill != SKILL_NONE && !IsSkillBase(skill) && !IsSkillNPC(skill) )
		{
			DEBUG_ERR(("UID:0%lx Bad Skill %d for '%s'\n", (DWORD)GetUID(), skill, GetName()));
			return false;
		}
		m_Act_SkillCurrent = skill;
		return true;
	}

	if ( Skill_GetActive() != SKILL_NONE )
	{
		Skill_Fail( true );	// Fail previous skill unfinished. (with NO skill gain!)
	}

	if ( skill != SKILL_NONE )
	{
		m_Act_SkillCurrent = skill;	// Start using a skill.
		m_Act_Difficulty = iDifficulty;

		// Some skill can start right away. Need no targetting.
		// 0-100 scale of Difficulty
		if ( IsTrigUsed(TRIGGER_SKILLPRESTART) )
		{
			if ( Skill_OnCharTrigger( skill, CTRIG_SkillPreStart ) == TRIGRET_RET_TRUE )
			{
				Skill_Cleanup();
				return false;
			}
		}
		if ( IsTrigUsed(TRIGGER_PRESTART) )
		{
			if ( Skill_OnTrigger( skill, SKTRIG_PRESTART ) == TRIGRET_RET_TRUE )
			{
				Skill_Cleanup();
				return false;
			}
		}
		m_Act_Difficulty = Skill_Stage(SKTRIG_START);

		// Execute the @START trigger and pass various craft parameters there
		CScriptTriggerArgs pArgs;
		bool bCraftSkill = IsSkillCraft(skill);
		bool bGatherSkill = IsSkillGather(skill);
		RESOURCE_ID pResBase(RES_ITEMDEF, bCraftSkill? m_atCreate.m_ItemID : 0, 0);

		if ( bCraftSkill == true )
		{		
			m_atCreate.m_Stroke_Count=1;		//This matches the new strokes amount used on OSI.
			// set crafting parameters
			pArgs.m_VarsLocal.SetNum("CraftItemdef",pResBase.GetPrivateUID());
			pArgs.m_VarsLocal.SetNum("CraftStrokeCnt",m_atCreate.m_Stroke_Count);
			pArgs.m_VarsLocal.SetNum("CraftAmount",m_atCreate.m_Amount);
		}
		if ( bGatherSkill == true )
			pArgs.m_VarsLocal.SetNum("GatherStrokeCnt",m_atResource.m_Stroke_Count);

		if ( IsTrigUsed(TRIGGER_SKILLSTART) )
		{
			if (( Skill_OnCharTrigger( skill, CTRIG_SkillStart, &pArgs ) == TRIGRET_RET_TRUE ) || ( m_Act_Difficulty < 0 ))
			{
				Skill_Cleanup();
				return false;
			}
		}

		if ( IsTrigUsed(TRIGGER_START) )
		{
			if (( Skill_OnTrigger( skill, SKTRIG_START, &pArgs ) == TRIGRET_RET_TRUE ) || ( m_Act_Difficulty < 0 ))
			{
				Skill_Cleanup();
				return false;
			}
		}

		if ( bCraftSkill == true )
		{
			// read crafting parameters
			pResBase.SetPrivateUID(static_cast<int>(pArgs.m_VarsLocal.GetKeyNum("CraftItemdef",true)));
			m_atCreate.m_ItemID = static_cast<ITEMID_TYPE>(pResBase.GetResIndex());
			m_atCreate.m_Stroke_Count = static_cast<WORD>(pArgs.m_VarsLocal.GetKeyNum("CraftStrokeCnt",true));
			if ( m_atCreate.m_Stroke_Count < 1)
				m_atCreate.m_Stroke_Count = 1;
			m_atCreate.m_Amount = static_cast<WORD>(pArgs.m_VarsLocal.GetKeyNum("CraftAmount",true));
			// Casting sound & animation when starting, Skill_Stroke() will do it the next times.
			if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
				Sound( Skill_GetSound( Skill_GetActive() ));

			if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOANIM ) )
				UpdateAnimate( Skill_GetAnim( Skill_GetActive() ) );
		}
		if ( bGatherSkill == true )
			m_atResource.m_Stroke_Count = static_cast<WORD>(pArgs.m_VarsLocal.GetKeyNum("GatherStrokeCnt",true));

		if ( IsSkillBase(skill) )
		{
			const CSkillDef* pSkillDef = g_Cfg.GetSkillDef(skill);
			if (pSkillDef != NULL)
			{
				int iWaitTime = pSkillDef->m_Delay.GetLinear( Skill_GetBase(skill) );
				if ( iWaitTime != 0 )
				{
					// How long before complete skill.
					SetTimeout( iWaitTime );
				}
			}
		}

		if ( IsTimerExpired())
		{
			// the skill should have set it's own delay!?
			SetTimeout( 1 );
		}

		if ( m_Act_Difficulty > 0 )
		{

			if ( ! Skill_CheckSuccess( skill, m_Act_Difficulty ))
			{
					m_Act_Difficulty = - m_Act_Difficulty; // will result in Failure ?
			}
		}
	}

	// emote the action i am taking.
	if (( g_Cfg.m_wDebugFlags & DEBUGF_NPC_EMOTE ) || IsStatFlag(STATF_EmoteAction))
	{
		Emote( Skill_GetName(true));
	}

	return( true );
}