//
// CResourceCalc.cpp
// Copyright Menace Software (www.menasoft.com).
// The physics calculations of the world.
//

#include "graysvr.h"	// predef header.

//********************************
// Movement

int CResource::Calc_MaxCarryWeight( const CChar * pChar ) const
{
	ADDTOCALLSTACK("CResource::Calc_MaxCarryWeight");
	// How much weight can i carry before i can carry no more. (and move at all)
	// Amount of weight that can be carried Max:
	// based on str 
	// RETURN: 
	//  Weight in tenths of stones i should be able to carry.

	ASSERT(pChar);
	signed int iQty = 40 + ( pChar->Stat_GetAdjusted(STAT_STR) * 35 / 10 ) + pChar->m_ModMaxWeight;
	if ( iQty < 0 )
		iQty = 0;
	if ( m_iFeatureML & FEATURE_ML_UPDATE && pChar->IsHuman())
		iQty += 60;		//Humans can always carry +60 stones (racial traits)
	return( iQty * WEIGHT_UNITS );
}

int CResource::Calc_WalkThroughChar( CChar * pCharMove, CChar * pCharObstacle )
{
	ADDTOCALLSTACK("CResource::Calc_WalkThroughChar");
	// Can't i push past this char ?
	// RETURN: 
	//  Stamina penalty
	//  -1 = i can't push through.
	//  0 = no penalty , just walk through. ie. a human past a bird or really small.

	if ( !pCharMove || !pCharObstacle )
		return -1;
	return ( pCharMove->Stat_GetAdjusted(STAT_DEX) / 5 );		// 20%
}

int CResource::Calc_DropStamWhileMoving( CChar * pChar, int iWeightLoadPercent )
{
	ADDTOCALLSTACK("CResource::Calc_DropStamWhileMoving");
	// I am now running/walking.
	// Should my stam drop ?
	// ARGS:
	//  iWeightLoadPercent = 0-100 percent of the weight i can carry.
	// RETURN:
	//  Stamina penalty
	//  -1 = can't move
	//  

	ASSERT(pChar);

	if ( pChar->IsStatFlag( STATF_DEAD ) )
		return 0;

	CVarDefCont * pVal = pChar->GetKey("OVERRIDE.RUNNINGPENALTY",true);

	if ( pChar->IsStatFlag( STATF_Fly|STATF_Hovering ))	// i'm running ?
	{
		iWeightLoadPercent += pVal ? static_cast<int>(pVal->GetValNum()) : m_iStamRunningPenalty;
	}

	// Chance to drop in Stam given a weight
	pVal = pChar->GetKey("OVERRIDE.STAMINALOSSATWEIGHT",true);

	int iChanceForStamLoss = pVal ? Calc_GetSCurve( iWeightLoadPercent - static_cast<int>(pVal->GetValNum()), 10 ) : Calc_GetSCurve( iWeightLoadPercent - m_iStaminaLossAtWeight, 10 );

	int iRoll = Calc_GetRandVal(1000);
	if ( iRoll <= iChanceForStamLoss )
		return 1;

	return( 0 );
}

//********************************
// Combat

int CResource::Calc_CombatAttackSpeed( CChar * pChar, CItem * pWeapon )
{
	ADDTOCALLSTACK("CResource::Calc_CombatAttackSpeed");
	// Combat: Speed of the attack
	// based on dex and weight of the weapon.
	// ? my tactics ?
	// ? my skill with weapon ?
	// ? How much weight i'm carrying ?
	// RETURN: 
	//  Time in tenths of a sec. (for entire swing, not just time to hit)

	ASSERT(pChar);

	if ( pWeapon != NULL )
	{
		CItemBase * pItemDef = dynamic_cast <CItemBase *> (pWeapon->Base_GetDef());
		if ( pItemDef )
		{
			BYTE speed;
			CVarDefCont * pTagStorage = NULL; 
			pTagStorage = pWeapon->GetKey("OVERRIDE.SPEED", true);
			if ( pTagStorage )
			{
				if ( pTagStorage->GetValNum())
				{
					speed = static_cast<int>(pTagStorage->GetValNum());
				}
				else
				{
					speed = pItemDef->GetSpeed();
				}
			}
			else
			{
				speed = pItemDef->GetSpeed();
			}

			if ( speed )
			{
				int iWaitTime = (TICK_PER_SEC * g_Cfg.m_iSpeedScaleFactor) / (maximum( (pChar->Stat_GetAdjusted(STAT_DEX) + 100), 1 ) * speed);
				return (iWaitTime > 5 ? iWaitTime : 5);
			}
		}
	}
	if ( pChar->m_pNPC &&
		pChar->m_pNPC->m_Brain == NPCBRAIN_GUARD &&
		m_fGuardsInstantKill )
		return( 1 );

	// Base speed is just your DEX range=40 to 0
	int iWaitTime = IMULDIV( 100 - pChar->Stat_GetAdjusted(STAT_DEX), 40, 100 );
	if ( iWaitTime < 5 )	// no-one needs to be this fast.
		iWaitTime = 5;
	else
		iWaitTime += 5;

	// Speed of the weapon as well effected by strength (minor).

	if ( pWeapon != NULL )
	{
		int iWeaponWait = (pWeapon->GetWeight() * 10 ) / ( 4 * WEIGHT_UNITS );	// tenths of a stone.
		if ( pWeapon->GetEquipLayer() == LAYER_HAND2 )	// 2 handed is slower
		{
			iWeaponWait += iWaitTime/2;
		}
		iWaitTime += iWeaponWait;
	}
	else
	{
		iWaitTime += 2;
	}

	return( iWaitTime );
}

int CResource::Calc_CombatChanceToHit( CChar * pChar, SKILL_TYPE skill, CChar * pCharTarg, CItem * pWeapon )
{
	ADDTOCALLSTACK("CResource::Calc_CombatChanceToHit");
	UNREFERENCED_PARAMETER(pWeapon);
	// AGRS:
	//  pChar = the attacker.
	//  pWeapon = NULL = wrestling.
	//
	// RETURN:
	//  0-100 percent chance to hit on a d100 roll.
	// ??? OR ???
	//  0-100 percent difficulty against my SKILL_* combat skill 
	//
	// NOTE: 
	// There should be size panlties of large creatures vs. small creatures in melee combat.

	// Chance to hit a target: (bow or melee)
	// Target speed (DEX) subtract out STAM lost?
	// Target Surprised ? (war mode) 
	// vs. 
	// Attackers Weapon skill
	// Attackers TACTICS
	// Attackers STAM/DEX
	// Size diff.
	//
	// NOTE: 
	//  How does this translate to experiencce in my combat skills ?

	// What is our chance of hitting our target?
	// This will be compared to our current weapon skill level.
	// There should always be a bit of a chance. (even if we suck)
	// Adjust the value for our experience curve level.
	// RETURN:
	//  0-100 difficulty.

	if ( pCharTarg == NULL )
	{
		return( Calc_GetRandVal(31) ); // must be a training dummy
	}

	if ( pChar->m_pNPC &&
		pChar->m_pNPC->m_Brain == NPCBRAIN_GUARD &&
		m_fGuardsInstantKill )
	{
		return( 0 );
	}

	// Frozen targets should be easy.
	if ( pCharTarg->IsStatFlag( STATF_Sleeping | STATF_Freeze | STATF_Stone ))
	{
		return( Calc_GetRandVal(10) );
	}

	int iSkillVal = pChar->Skill_GetAdjusted( skill );

	// Offensive value mostly based on your skill and TACTICS.
	// 0 - 1000
	int iSkillAttack = ( iSkillVal + pChar->Skill_GetAdjusted( SKILL_TACTICS )) / 2;
	// int iSkillAttack = ( iSkillVal * 3 + pChar->Skill_GetAdjusted( SKILL_TACTICS )) / 4;

	// Defensive value mostly based on your tactics value and random DEX,
	// 0 - 1000
	int iSkillDefend = pCharTarg->Skill_GetAdjusted( SKILL_TACTICS );

	// Make it easier to hit people havin a bow or crossbow due to the fact that its
	// not a very "mobile" weapon, nor is it fast to change position while in
	// a fight etc. Just use 90% of the statvalue when defending so its easier
	// to hit than defend == more fun in combat.
	int iStam = pCharTarg->Stat_GetVal(STAT_DEX);
	if ( g_Cfg.IsSkillRanged(pCharTarg->Skill_GetActive()) &&
		!g_Cfg.IsSkillRanged(skill) )
		// The defender uses ranged weapon and the attacker is not.
		// Make just a bit easier to hit.
		iSkillDefend = ( iSkillDefend + iStam*9 ) / 2;
	else
		// The defender is using a nonranged, or they both use bows.
		iSkillDefend = ( iSkillDefend + iStam*10 ) / 2;

	int iDiff = ( iSkillAttack - iSkillDefend ) / 5;

	iDiff = ( iSkillVal - iDiff ) / 10;
	if ( iDiff < 0 )
		iDiff = 0;	// just means it's very easy.
	else if ( iDiff > 100 )
		iDiff = 100;	// just means it's very hard.

	return( Calc_GetRandVal(iDiff) );	// always need to have some chance. );
}

int CResource::Calc_FameKill( CChar * pKill )
{
	ADDTOCALLSTACK("CResource::Calc_FameKill");
	// Translate the fame for a Kill.

	int iFameChange = pKill->Stat_GetAdjusted(STAT_FAME);

	// Check if the victim is a PC, then higher gain/loss.
	if ( pKill->m_pPlayer )
	{
		iFameChange /= 10;
	}
	else
	{
		iFameChange /= 200;
	}

	return( iFameChange );
}

int CResource::Calc_FameScale( int iFame, int iFameChange )
{
	ADDTOCALLSTACK("CResource::Calc_FameScale");
	UNREFERENCED_PARAMETER(iFame);
	// Scale the fame based on the current level.
	// Scale the fame gain at higher levels.
	//if ( iFameChange > 0 && iFameChange < iFame/32 )
	//	return 0;
	return( iFameChange );
}

int CResource::Calc_KarmaKill( CChar * pKill, NOTO_TYPE NotoThem )
{
	ADDTOCALLSTACK("CResource::Calc_KarmaKill");
	// Karma change on kill ?

	int iKarmaChange = -pKill->Stat_GetAdjusted(STAT_KARMA);

	if ( NotoThem >= NOTO_CRIMINAL )
	{
		// No bad karma for killing a criminal or my aggressor.
		if ( iKarmaChange < 0 )
			iKarmaChange = 0;
	}
		
	// Check if the victim is a PC, then higher gain/loss.
	if ( pKill->m_pPlayer )
	{
		// If killing a 'good' PC we should always loose at least
		// 500 karma
		if ( iKarmaChange < 0 && iKarmaChange >= -5000 )
		{
			iKarmaChange = -5000;
		}

		iKarmaChange /= 10;
	}
	else	// Or is it was a NPC, less gain/loss.
	{
		// Always loose at least 20 karma if you kill a 'good' NPC
		if ( iKarmaChange < 0 && iKarmaChange >= -1000 )
		{
			iKarmaChange = -1000;
		}

		iKarmaChange /= 20;	// Not as harsh penalty as with player chars.
	}

	return( iKarmaChange );
}

int CResource::Calc_KarmaScale( int iKarma, int iKarmaChange )
{
	ADDTOCALLSTACK("CResource::Calc_KarmaScale");
	// Scale the karma based on the current level.

	// Should be harder to gain karma than to loose it.

	if ( iKarma > 0 )
	{
		// Your a good guy. Harder to be a good guy.
		if ( iKarmaChange < 0 )
			iKarmaChange *= 2;	// counts worse against you.
		else
			iKarmaChange /= 2;	// counts less for you.
	}
	else
	{
		// Your a bad guy.
	}

	// Scale the karma at higher levels.
	if ( iKarmaChange > 0 && iKarmaChange < iKarma/64 )
		return 0;

	return( iKarmaChange );
}

//********************************
// Skill Checks and Stat changes.

bool CResource::Calc_SkillCheck( int iSkillLevel, int iDifficulty )
{
	ADDTOCALLSTACK("CResource::Calc_SkillCheck");
	// Chance to complete skill check given skill x and difficulty y
	// ARGS:
	//  iSkillLevel = 0-1000
	//  difficulty = 0-100
	// RETURN:
	//  true = success check.

	if ( iDifficulty < 0 || iSkillLevel < 0 )	// auto failure.
		return( false );

	int iChanceForSuccess = Calc_GetSCurve( iSkillLevel - ( iDifficulty * 10 ), SKILL_VARIANCE );
	int iRoll = Calc_GetRandVal(1000);

	return( iRoll <= iChanceForSuccess );
}

//Chance to rise in Stat
//	food ?

//Chance to rise in Skill

//Chance to decay in Skill or Stats

//********************************
// Stealing

int CResource::Calc_StealingItem( CChar * pCharThief, CItem * pItem, CChar * pCharMark )
{
	ADDTOCALLSTACK("CResource::Calc_StealingItem");
	// Chance to steal and retrieve the item successfully.
	// weight of the item
	//  heavier items should be more difficult.
	//	thiefs skill/ dex
	//  marks skill/dex
	//  marks war mode ?
	// NOTE:
	//  Items on the ground can always be stolen. chance of being seen is another matter.
	// RETURN:
	//  0-100 percent chance to hit on a d100 roll.
	//  0-100 percent difficulty against my SKILL_STEALING skill.

	ASSERT(pCharThief);
	ASSERT(pCharMark);

	int iDexMark = pCharMark->Stat_GetAdjusted(STAT_DEX);
	int iSkillMark = pCharMark->Skill_GetAdjusted( SKILL_STEALING );
	int iWeightItem = pItem->GetWeight();
	
	// int iDifficulty = iDexMark/2 + (iSkillMark/5) + Calc_GetRandVal(iDexMark/2) + IMULDIV( iWeightItem, 4, WEIGHT_UNITS );
	// Melt mod:
    	int iDifficulty = (iSkillMark/5) + Calc_GetRandVal(iDexMark/2) + IMULDIV( iWeightItem, 4, WEIGHT_UNITS );
	
	if ( pItem->IsItemEquipped())
	{
		// This is REALLY HARD to do.
		iDifficulty += iDexMark/2 + pCharMark->Stat_GetAdjusted(STAT_INT);
	}
	if ( pCharThief->IsStatFlag( STATF_War )) // all keyed up.
	{
		iDifficulty += Calc_GetRandVal( iDexMark/2 );
	}
	
	// return( iDifficulty );
	// Melt mod:
	return( iDifficulty / 2 );
}

bool CResource::Calc_CrimeSeen( CChar * pCharThief, CChar * pCharViewer, SKILL_TYPE SkillToSee, bool fBonus )
{
	ADDTOCALLSTACK("CResource::Calc_CrimeSeen");
	// Chance to steal without being seen by a specific person
	//	weight of the item
	//	distance from crime. (0=i am the mark)
	//	Thiefs skill/ dex
	//  viewers skill

	if ( SkillToSee == SKILL_NONE )	// takes no skill.
		return( true );

	ASSERT(pCharViewer);
	ASSERT(pCharThief);

	if ( pCharViewer->IsPriv(PRIV_GM) || pCharThief->IsPriv(PRIV_GM))
	{
		if ( pCharViewer->GetPrivLevel() < pCharThief->GetPrivLevel())
			return( false );	// never seen
		if ( pCharViewer->GetPrivLevel() > pCharThief->GetPrivLevel())
			return( true );	// always seen.
	}

	int iChanceToSee = ( pCharViewer->Stat_GetAdjusted(STAT_DEX) + pCharViewer->Stat_GetAdjusted(STAT_INT)) * 50;
	if ( SkillToSee != SKILL_NONE )
	{
		// Snooping or stealing.
		iChanceToSee = 1000+(pCharViewer->Skill_GetBase(SkillToSee) - pCharThief->Skill_GetBase(SkillToSee));
	}
	else
	{
		iChanceToSee += 400;
	}

	// the targets chance of seeing.
	if ( fBonus )
	{
		// Up by 30 % if it's me.
		iChanceToSee += iChanceToSee/3;
		if ( iChanceToSee < 50 ) // always atleast 5% chance.
			iChanceToSee=50;
	}
	else
	{
		// the bystanders chance of seeing.
		if ( iChanceToSee < 10 ) // always atleast 1% chance.
			iChanceToSee=10;
	}

	if ( Calc_GetRandVal(1000) > iChanceToSee )
		return( false );

	return( true );
}

LPCTSTR CResource::Calc_MaptoSextant( CPointMap pntCoords )
{
	ADDTOCALLSTACK("CResource::Calc_MaptoSextant");
	// Conversion from map square to degrees, minutes
	char *z = Str_GetTemp();
	CPointMap	zeroPoint;

	zeroPoint.Read(strcpy(z, g_Cfg.m_sZeroPoint));

	long lLat =  (pntCoords.m_y - zeroPoint.m_y) * 360 * 60 / g_MapList.GetY(zeroPoint.m_map);
	long lLong;
	if ( pntCoords.m_map <= 1 )
		lLong = (pntCoords.m_x - zeroPoint.m_x) * 360 * 60 / UO_SIZE_X_REAL;
	else
		lLong = (pntCoords.m_x - zeroPoint.m_x) * 360 * 60 / g_MapList.GetX(pntCoords.m_map);
	int iLatDeg =  lLat / 60;
	int iLatMin  = lLat % 60;
	int iLongDeg = lLong / 60;
	int iLongMin = lLong % 60;

	TCHAR * pTemp = Str_GetTemp();
	sprintf( pTemp, "%io%i'%s, %io%i'%s",
		abs(iLatDeg),  abs(iLatMin),  (lLat <= 0) ? "N" : "S",
		abs(iLongDeg), abs(iLongMin), (lLong >= 0) ? "E" : "W");

	return pTemp;
}
