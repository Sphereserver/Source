// The physics calculations of the world.
#include "graysvr.h"	// predef header.

///////////////////////////////////////////////////////////
// Movement

int CResource::Calc_MaxCarryWeight(const CChar *pChar) const
{
	ADDTOCALLSTACK("CResource::Calc_MaxCarryWeight");
	// How much weight can i carry before i can carry no more. (and move at all)
	// Amount of weight that can be carried Max:
	// based on str 
	// RETURN: 
	//  Weight in tenths of stones i should be able to carry.

	ASSERT(pChar);
	int iQty = 40 + (pChar->Stat_GetAdjusted(STAT_STR) * 35 / 10) + pChar->m_ModMaxWeight;
	if ( iQty < 0 )
		iQty = 0;
	if ( (m_iRacialFlags & RACIALF_HUMAN_STRONGBACK) && pChar->IsHuman() )
		iQty += 60;		// humans can always carry +60 stones (Strong Back racial trait)
	return iQty * WEIGHT_UNITS;
}

///////////////////////////////////////////////////////////
// Combat

int CResource::Calc_CombatAttackSpeed(CChar *pChar, CItem *pWeapon)
{
	ADDTOCALLSTACK("CResource::Calc_CombatAttackSpeed");
	// Calculate the swing speed value on chars
	// RETURN:
	//  Time in tenths of a sec. (for entire swing, not just time to hit)
	//  Should never return a value < 0 to avoid break combat timer/sequence

	ASSERT(pChar);
	if ( pChar->m_pNPC && (pChar->m_pNPC->m_Brain == NPCBRAIN_GUARD) && m_fGuardsInstantKill )
		return 1;

	BYTE bBaseSpeed = 50;	// base Wrestling speed (on ML formula it's 2.50)
	if ( pWeapon )
		bBaseSpeed = pWeapon->GetSpeed();

	switch ( g_Cfg.m_iCombatSpeedEra )
	{
		default:
		case 0:
		{
			// Sphere custom formula	(default m_iSpeedScaleFactor = 15000, uses DEX instead STAM and calculate delay using weapon WEIGHT if weapon SPEED is not set)
			if ( pWeapon && bBaseSpeed )
			{
				int iSwingSpeed = maximum(1, (pChar->Stat_GetAdjusted(STAT_DEX) + 100) * bBaseSpeed);
				iSwingSpeed = (g_Cfg.m_iSpeedScaleFactor * TICK_PER_SEC) / iSwingSpeed;
				if ( iSwingSpeed < 5 )
					iSwingSpeed = 5;
				return iSwingSpeed;
			}

			int iSwingSpeed = IMULDIV(100 - pChar->Stat_GetAdjusted(STAT_DEX), 40, 100);	// base speed is just the char DEX range (0 ~ 40)
			if ( iSwingSpeed < 5 )
				iSwingSpeed = 5;
			else
				iSwingSpeed += 5;

			if ( pWeapon )
			{
				int iWeightMod = (pWeapon->GetWeight() * 10) / (4 * WEIGHT_UNITS);
				if ( pWeapon->GetEquipLayer() == LAYER_HAND2 )	// 2-handed weapons are slower
					iWeightMod += iSwingSpeed / 2;
				iSwingSpeed += iWeightMod;
			}
			else
				iSwingSpeed += 2;
			return iSwingSpeed;
		}

		case 1:
		{
			// pre-AOS formula	(default m_iSpeedScaleFactor = 15000)
			int iSwingSpeed = maximum(1, (pChar->Stat_GetVal(STAT_DEX) + 100) * bBaseSpeed);
			iSwingSpeed = (g_Cfg.m_iSpeedScaleFactor * TICK_PER_SEC) / iSwingSpeed;
			if ( iSwingSpeed < 1 )
				iSwingSpeed = 1;
			return iSwingSpeed;
		}

		case 2:
		{
			// AOS formula		(default m_iSpeedScaleFactor = 40000)
			int iSwingSpeed = (pChar->Stat_GetVal(STAT_DEX) + 100) * bBaseSpeed;
			iSwingSpeed = maximum(1, iSwingSpeed * (100 + pChar->m_SwingSpeedIncrease) / 100);
			iSwingSpeed = ((g_Cfg.m_iSpeedScaleFactor * TICK_PER_SEC) / iSwingSpeed) / 2;
			if ( iSwingSpeed < 12 )		//1.25
				iSwingSpeed = 12;
			return iSwingSpeed;
		}

		case 3:
		{
			// SE formula		(default m_iSpeedScaleFactor = 80000)
			int iSwingSpeed = maximum(1, bBaseSpeed * (100 + pChar->m_SwingSpeedIncrease) / 100);
			iSwingSpeed = (g_Cfg.m_iSpeedScaleFactor / ((pChar->Stat_GetVal(STAT_DEX) + 100) * iSwingSpeed)) - 2;	// get speed in ticks of 0.25s each
			if ( iSwingSpeed < 5 )
				iSwingSpeed = 5;
			return (iSwingSpeed * TICK_PER_SEC) / 4;		// convert 0.25s ticks into ms
		}

		case 4:
		{
			// ML formula		(doesn't use m_iSpeedScaleFactor and it's only compatible with ML speed format eg. 0.25 ~ 5.00 instead 0 ~ 50)
			int iSwingSpeed = ((bBaseSpeed * 4) - (pChar->Stat_GetVal(STAT_DEX) / 30)) * (100 / (100 + pChar->m_SwingSpeedIncrease));	// get speed in ticks of 0.25s each
			if ( iSwingSpeed < 5 )
				iSwingSpeed = 5;
			return (iSwingSpeed * TICK_PER_SEC) / 4;		// convert 0.25s ticks into ms
		}
	}
}

int CResource::Calc_CombatChanceToHit(CChar *pChar, CChar *pCharTarg)
{
	ADDTOCALLSTACK("CResource::Calc_CombatChanceToHit");
	// Calculate the hit chance on combat
	// RETURN:
	//  0-100 percent chance to hit

	if ( !pCharTarg )	// must be a training dummy
		return 50;
	if ( pChar->m_pNPC && (pChar->m_pNPC->m_Brain == NPCBRAIN_GUARD) && m_fGuardsInstantKill )
		return 100;

	SKILL_TYPE skillAttacker = pChar->Fight_GetWeaponSkill();
	SKILL_TYPE skillTarget = pCharTarg->Fight_GetWeaponSkill();

	switch ( g_Cfg.m_iCombatHitChanceEra )
	{
		default:
		case 0:
		{
			// Sphere custom formula
			if ( pCharTarg->IsStatFlag(STATF_Sleeping|STATF_Freeze) )
				return Calc_GetRandVal(10);

			WORD wSkillVal = pChar->Skill_GetAdjusted(skillAttacker);

			// Offensive value mostly based on your skill and TACTICS (0 - 1000)
			WORD wAttackerSkill = (wSkillVal + pChar->Skill_GetAdjusted(SKILL_TACTICS)) / 2;

			// Defensive value mostly based on your tactics value and random DEX (0 - 1000)
			WORD wTargetSkill = pCharTarg->Skill_GetAdjusted(SKILL_TACTICS);

			// Make it easier to hit people having a bow or crossbow due to the fact that
			// its not a very "mobile" weapon, nor is it fast to change position while in
			// a fight etc. Just use 90% of the statvalue when defending so its easier to
			// hit than defend == more fun in combat.
			int iTargetStam = pCharTarg->Stat_GetVal(STAT_DEX);
			if ( g_Cfg.IsSkillFlag(skillTarget, SKF_RANGED) && !g_Cfg.IsSkillFlag(skillAttacker, SKF_RANGED) )
				wTargetSkill = (wTargetSkill + iTargetStam * 9) / 2;		// the defender uses ranged weapon and the attacker is not. Make just a bit easier to hit.
			else
				wTargetSkill = (wTargetSkill + iTargetStam * 10) / 2;		// the defender is using a nonranged, or they both use bows.

			int iDiff = (wAttackerSkill - wTargetSkill) / 5;
			iDiff = (wSkillVal - iDiff) / 10;
			if ( iDiff < 0 )
				iDiff = 0;		// very easy
			else if ( iDiff > 100 )
				iDiff = 100;	// very hard
			return Calc_GetRandVal(iDiff);
		}

		case 1:
		{
			// pre-AOS formula
			WORD wAttackerSkill = pChar->Skill_GetBase(skillAttacker) + 500;
			WORD wTargetSkill = pCharTarg->Skill_GetBase(skillTarget) + 500;

			int iChance = wAttackerSkill * 100 / (wTargetSkill * 2);
			if ( iChance < 0 )
				iChance = 0;
			else if ( iChance > 100 )
				iChance = 100;
			return iChance;
		}

		case 2:
		{
			// AOS formula
			WORD wAttackerSkill = pChar->Skill_GetBase(skillAttacker);
			WORD wTargetSkill = ((pCharTarg->Skill_GetBase(skillTarget) / 10) + 20) * (100 + minimum(pCharTarg->m_DefChanceIncrease, pCharTarg->m_DefChanceIncreaseMax));

			int iAttackerHitChance = pChar->m_HitChanceIncrease;
			if ( (g_Cfg.m_iRacialFlags & RACIALF_GARG_DEADLYAIM) && pChar->IsGargoyle() )
			{
				// Racial traits: Deadly Aim. Gargoyles always have +5 Hit Chance Increase and a minimum of 20.0 Throwing skill (not shown in skills gump)
				if ( (skillAttacker == SKILL_THROWING) && (wAttackerSkill < 200) )
					wAttackerSkill = 200;
				iAttackerHitChance += 5;
			}
			wAttackerSkill = ((wAttackerSkill / 10) + 20) * (100 + minimum(iAttackerHitChance, 45));

			int iChance = wAttackerSkill * 100 / (wTargetSkill * 2);
			if ( iChance < 2 )
				iChance = 2;	// minimum hit chance is 2%
			else if ( iChance > 100 )
				iChance = 100;
			return iChance;
		}
	}
}

int CResource::Calc_FameKill(CChar *pKill)
{
	ADDTOCALLSTACK("CResource::Calc_FameKill");
	// Fame chance on kill

	if ( pKill->m_pPlayer )
		return pKill->Stat_GetAdjusted(STAT_FAME) / 10;
	else
		return pKill->Stat_GetAdjusted(STAT_FAME) / 200;
}

int CResource::Calc_KarmaKill(CChar *pKill, NOTO_TYPE NotoThem)
{
	ADDTOCALLSTACK("CResource::Calc_KarmaKill");
	// Karma change on kill

	int iKarmaChange = -pKill->Stat_GetAdjusted(STAT_KARMA);
	if ( NotoThem >= NOTO_CRIMINAL )
	{
		// No bad karma for killing a criminal or my aggressor
		if ( iKarmaChange < 0 )
			iKarmaChange = 0;
	}

	if ( pKill->m_pPlayer )
	{
		// If killing a 'good' PC we should always loose at least 500 karma
		if ( (iKarmaChange < 0) && (iKarmaChange >= -5000) )
			iKarmaChange = -5000;
		return iKarmaChange / 10;
	}
	else
	{
		// Always loose at least 50 karma if you kill a 'good' NPC
		if ( (iKarmaChange < 0) && (iKarmaChange >= -1000) )
			iKarmaChange = -1000;
		return iKarmaChange / 20;
	}
}

int CResource::Calc_KarmaScale(int iKarma, int iKarmaChange)
{
	ADDTOCALLSTACK("CResource::Calc_KarmaScale");
	// Scale the karma based on the current level
	// Should be harder to gain karma than to loose it

	if ( iKarma > 0 )
	{
		// Your a good guy. Harder to be a good guy
		if ( iKarmaChange < 0 )
			iKarmaChange *= 2;	// counts worse against you
		else
			iKarmaChange /= 2;	// counts less for you
	}

	// Scale the karma at higher levels
	if ( (iKarmaChange > 0) && (iKarmaChange < iKarma / 64) )
		return 0;

	return iKarmaChange;
}

///////////////////////////////////////////////////////////
// Stealing

int CResource::Calc_StealingItem(CChar *pCharThief, CItem *pItem, CChar *pCharMark)
{
	ADDTOCALLSTACK("CResource::Calc_StealingItem");
	// Chance to steal and retrieve the item successfully
	// NOTE:
	//  Items on the ground can always be stolen. chance of being seen is another matter.
	// RETURN:
	//  0-100 percent chance to hit on a d100 roll
	//  0-100 percent difficulty against my SKILL_STEALING skill

	ASSERT(pCharThief);
	ASSERT(pCharMark);

	int iDexMark = pCharMark->Stat_GetAdjusted(STAT_DEX);
	int iDifficulty = (pCharMark->Skill_GetAdjusted(SKILL_STEALING) / 5) + Calc_GetRandVal(iDexMark / 2) + IMULDIV(pItem->GetWeight(), 4, WEIGHT_UNITS);

	if ( pItem->IsItemEquipped() )
		iDifficulty += iDexMark / 2 + pCharMark->Stat_GetAdjusted(STAT_INT);		// this is REALLY HARD to do
	if ( pCharThief->IsStatFlag(STATF_War) )
		iDifficulty += Calc_GetRandVal(iDexMark / 2);

	return iDifficulty / 2;
}

bool CResource::Calc_CrimeSeen(CChar *pCharThief, CChar *pCharViewer, SKILL_TYPE SkillToSee, bool fBonus)
{
	ADDTOCALLSTACK("CResource::Calc_CrimeSeen");
	// Chance to steal without being seen by a specific person

	if ( SkillToSee == SKILL_NONE )
		return true;

	ASSERT(pCharViewer);
	ASSERT(pCharThief);

	if ( pCharViewer->IsPriv(PRIV_GM) || pCharThief->IsPriv(PRIV_GM) )
	{
		if ( pCharViewer->GetPrivLevel() < pCharThief->GetPrivLevel() )
			return false;	// never seen
		if ( pCharViewer->GetPrivLevel() > pCharThief->GetPrivLevel() )
			return true;	// always seen
	}

	int iChanceToSee = 1000 + (pCharViewer->Skill_GetBase(SkillToSee) - pCharThief->Skill_GetBase(SkillToSee));		// snooping or stealing

	if ( fBonus )
	{
		// Up by 30 % if it's me
		iChanceToSee += iChanceToSee / 3;
		if ( iChanceToSee < 50 )	// always at least 5% chance
			iChanceToSee = 50;
	}
	else
	{
		// The bystanders chance of seeing
		if ( iChanceToSee < 10 )	// always at least 1% chance
			iChanceToSee = 10;
	}
	return (Calc_GetRandVal(1000) <= iChanceToSee);
}

LPCTSTR CResource::Calc_MaptoSextant(CPointMap pt)
{
	ADDTOCALLSTACK("CResource::Calc_MaptoSextant");
	// Conversion from map square to degrees, minutes
	char *z = Str_GetTemp();
	CPointMap zeroPoint;
	zeroPoint.Read(strcpy(z, g_Cfg.m_sZeroPoint));

	int iLat = (pt.m_y - zeroPoint.m_y) * 360 * 60 / g_MapList.GetY(zeroPoint.m_map);
	int iLong;
	if ( pt.m_map <= 1 )
		iLong = (pt.m_x - zeroPoint.m_x) * 360 * 60 / UO_SIZE_X_REAL;
	else
		iLong = (pt.m_x - zeroPoint.m_x) * 360 * 60 / g_MapList.GetX(pt.m_map);

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "%do %d'%s, %do %d'%s", abs(iLat / 60), abs(iLat % 60), (iLat <= 0) ? "N" : "S", abs(iLong / 60), abs(iLong % 60), (iLong >= 0) ? "E" : "W");
	return pszTemp;
}
