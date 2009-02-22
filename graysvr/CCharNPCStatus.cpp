//
// CCharNPCStatus.cpp
// Copyright Menace Software (www.menasoft.com).
//
// Test things to judge what an NPC might be thinking. (want to do)
// But take no actions here.
//

#include "graysvr.h"	// predef header.

CREID_TYPE CChar::NPC_GetAllyGroupType(CREID_TYPE idTest)	// static
{
	ADDTOCALLSTACK("CChar::NPC_GetAllyGroupType");
	switch ( idTest )
	{
	case CREID_MAN:
	case CREID_WOMAN:
	case CREID_GHOSTMAN:
	case CREID_GHOSTWOMAN:
		return( CREID_MAN );
	case CREID_ELFMAN:
	case CREID_ELFWOMAN:
	case CREID_ELFGHOSTMAN:
	case CREID_ELFGHOSTWOMAN:
		return( CREID_ELFMAN );
	case CREID_ETTIN:
	case CREID_ETTIN_AXE:
		return( CREID_ETTIN );
	case CREID_ORC_LORD:
	case CREID_ORC:
	case CREID_ORC_CLUB:
		return( CREID_ORC );
	case CREID_DAEMON:
	case CREID_DAEMON_SWORD:
		return( CREID_DAEMON );
	case CREID_DRAGON_GREY:
	case CREID_DRAGON_RED:
	case CREID_DRAKE_GREY:
	case CREID_DRAKE_RED:
		return( CREID_DRAGON_GREY );
	case CREID_LIZMAN:
	case CREID_LIZMAN_SPEAR:
	case CREID_LIZMAN_MACE:
		return( CREID_LIZMAN );
	case CREID_RATMAN:
	case CREID_RATMAN_CLUB:
	case CREID_RATMAN_SWORD:
		return( CREID_RATMAN );
	case CREID_SKELETON:
	case CREID_SKEL_AXE:
	case CREID_SKEL_SW_SH:
		return( CREID_SKELETON );
	case CREID_TROLL_SWORD:
	case CREID_TROLL:
	case CREID_TROLL_MACE:
		return( CREID_TROLL );
	case CREID_Tera_Warrior:
	case CREID_Tera_Drone:
	case CREID_Tera_Matriarch:
		return( CREID_Tera_Drone );
	case CREID_Ophid_Mage:
	case CREID_Ophid_Warrior:
	case CREID_Ophid_Queen:
		return( CREID_Ophid_Warrior );
	case CREID_HORSE1:
	case CREID_HORSE4:
	case CREID_HORSE2:
	case CREID_HORSE3:
	case CREID_HORSE_PACK:
		return( CREID_HORSE1 );
	case CREID_BrownBear:
	case CREID_GrizzlyBear:
	case CREID_PolarBear:
		return( CREID_BrownBear );
	case CREID_Cow_BW:
	case CREID_Cow2:
	case CREID_Bull_Brown:
	case CREID_Bull2:
		return( CREID_Bull_Brown );
	case CREID_Ostard_Desert:
	case CREID_Ostard_Frenz:
	case CREID_Ostard_Forest:
		return( CREID_Ostard_Forest );
	case CREID_Sheep:
	case CREID_Sheep_Sheered:
		return( CREID_Sheep );
	case CREID_Hart:
	case CREID_Deer:
		return( CREID_Deer );
	case CREID_Pig:
	case CREID_Boar:
		return( CREID_Pig );
	case CREID_Llama:
	case CREID_LLAMA_PACK:
		return( CREID_Llama );
	}
	return( idTest );
}


int CChar::NPC_GetVendorMarkup( const CChar * pChar ) const
{
	ADDTOCALLSTACK("CChar::NPC_GetVendorMarkup");
	// This vendor marks stuff up/down this percentage.
	// Base this on KARMA. Random is calculated at Restock time
	// When vendor sells to players this is the markup value.
	// fBuy: Client buying
	// RETURN:
	//  0-100

	if ( !pChar || IsStatFlag(STATF_Pet) )	// Not on a hired vendor.
		return( 0 );

	CVarDefCont	*pVar, *pVarCharDef;
	CCharBase * pCharDef = Char_GetDef();
	
	if ( pCharDef )
	{
		pVarCharDef = pCharDef->m_TagDefs.GetKey("VENDORMARKUP");
	}

	int iHostility = maximum(NPC_GetHostilityLevelToward(pChar), 0);
	iHostility = minimum(iHostility + 15, 100);

	pVar = m_TagDefs.GetKey("VENDORMARKUP");
	if ( pVar )
	{
		iHostility += pVar->GetValNum();
	}
	else
	{
		pVar = GetRegion()->m_TagDefs.GetKey("VENDORMARKUP");
		if ( pVar )
		{
			iHostility += pVar->GetValNum();
		}
		else
		{
			if ( pVarCharDef )
			{
				iHostility += pVarCharDef->GetValNum();
			}
		}
	}

	return( iHostility );
}


int CChar::NPC_OnHearName( LPCTSTR pszText ) const
{
	ADDTOCALLSTACK("CChar::NPC_OnHearName");
	// Did I just hear my name in this text ?
	// should be able to deal with "hi Dennis" in the future.
	// RETURN:
	//  index to skip past the name.

	LPCTSTR pszName = GetName();

	int i = FindStrWord( pszText, pszName );
	if ( i )
		return( i );

	if ( m_pNPC )
	{
		// My title ?
		if ( m_pNPC->m_Brain == NPCBRAIN_GUARD )
		{
			if ( ! strnicmp( pszText, "GUARD ", 6 ))
				return 6;
		}
		else if ( NPC_IsVendor())
		{
			if ( ! strnicmp( pszText, "VENDOR ", 7 ))
				return 7;
		}
	}

	CCharBase * pCharDef = Char_GetDef();

	// Named the chars type ? (must come first !)
	pszName = pCharDef->GetTradeName();
	for ( i=0; pszText[i] != '\0'; i++ )
	{
		if ( pszName[i] == '\0' )
		{
			// found name.
			while ( ISWHITESPACE( pszText[i] ))
				i ++;
			return( i );	// Char name found
		}
		if ( toupper( pszName[i] ) != toupper( pszText[i] ))	// not the name.
			break;
	}

	return( 0 );
}

bool CChar::NPC_CanSpeak() const
{
	ADDTOCALLSTACK("CChar::NPC_CanSpeak");
	//	players and chars with speech can
	if ( !m_pNPC || m_pNPC->m_Speech.GetCount() )
		return true;

	CCharBase	*pCharDef = Char_GetDef();
	return pCharDef ? pCharDef->m_Speech.GetCount() : false;
}

bool CChar::NPC_FightMayCast() const
{
	ADDTOCALLSTACK("CChar::NPC_FightMayCast");
	// This NPC could cast spells if they wanted to ?
	// check mana and anti-magic
	if ( Skill_GetBase(SKILL_MAGERY) < 300 )
		return false;
	if ( m_pArea && m_pArea->IsFlag(REGION_ANTIMAGIC_DAMAGE|REGION_FLAG_SAFE) )
		return false;
	if ( Stat_GetVal(STAT_INT) < 5 )
		return false;

	return true;
}

bool CChar::NPC_IsOwnedBy( const CChar * pChar, bool fAllowGM ) const
{
	ADDTOCALLSTACK("CChar::NPC_IsOwnedBy");
	// Is pChar my master ?
	// BESERK will not listen to commands tho.
	// fAllowGM = consider GM's to be owners of all NPC's

	if ( pChar == NULL )
		return( false );
	if ( this == pChar )
		return( true );

	if ( fAllowGM && pChar->IsPriv( PRIV_GM ))
		return( pChar->GetPrivLevel() > GetPrivLevel());

	if ( ! IsStatFlag( STATF_Pet ) || m_pPlayer )	// shortcut - i'm not a pet.
		return( false );
	if ( m_pNPC == NULL )
		return( false );
	if ( m_pNPC->m_Brain == NPCBRAIN_BERSERK )	// i cannot be commanded.
		return( false );

	return( Memory_FindObjTypes( pChar, MEMORY_IPET|MEMORY_FRIEND ) != NULL );
}

CChar * CChar::NPC_PetGetOwner() const
{
	ADDTOCALLSTACK("CChar::NPC_PetGetOwner");
	// Assume i am a pet. Get my first (primary) owner. not just friends. used for blame .

	if ( !IsStatFlag(STATF_Pet) )
		return NULL;

	CItemMemory	*pMemory = Memory_FindTypes(MEMORY_IPET);
	if ( !pMemory )
		return NULL;

	return pMemory->m_uidLink.CharFind();
}

int CChar::NPC_GetTrainMax( const CChar * pStudent, SKILL_TYPE Skill ) const
{
	ADDTOCALLSTACK("CChar::NPC_GetTrainMax");
	// What is the max I can train to ?
	int iMax;
	int iMaxAllowed;

	CVarDefCont * pValue = GetKey("OVERRIDE.TRAINSKILLMAXPERCENT",true);
	if ( pValue ) 
	{
		iMax = IMULDIV( pValue->GetValNum(), Skill_GetBase(Skill), 100 );
	} else {
		iMax = IMULDIV( g_Cfg.m_iTrainSkillPercent, Skill_GetBase(Skill), 100 );
	}

	pValue = GetKey("OVERRIDE.TRAINSKILLMAX",true);
	if ( pValue ) 
	{
		iMaxAllowed = pValue->GetValNum();
	} else {
		iMaxAllowed = g_Cfg.m_iTrainSkillMax;
	}
	
	if ( iMax > iMaxAllowed )
		return minimum(iMaxAllowed, pStudent->Skill_GetMax(Skill));

	// Is this more that the student can take ?
	return minimum(iMax, pStudent->Skill_GetMax(Skill));
}

bool CChar::NPC_CheckWalkHere( const CPointBase & pt, const CRegionBase * pArea, WORD wBlockFlags ) const
{
	ADDTOCALLSTACK("CChar::NPC_CheckWalkHere");
	// Does the NPC want to walk here ? step on this item ?
	if ( !m_pNPC )
		return false;

	if ( m_pArea != NULL ) // most decisions are based on area.
	{
		if ( m_pNPC->m_Brain == NPCBRAIN_GUARD && ! IsStatFlag( STATF_War ))
		{
			// Guards will want to stay in guard range.
			if ( m_pArea->IsGuarded() && ! pArea->IsGuarded())
			{
				return( false );
			}
		}

		if ( m_pNPC->m_Brain == NPCBRAIN_UNDEAD )
		{
			CSector	*ptSector = pt.GetSector();
			CSector *ptopSector = GetTopSector();

			if ( !ptSector || !ptopSector )
				return false;

			// always avoid the light.
			bool fSafeThere = ( ptSector->IsDark() || ( wBlockFlags & CAN_I_ROOF ) || pArea->IsFlag(REGION_FLAG_UNDERGROUND));
			if ( ! fSafeThere )
			{
				// But was it safe here ?
				if ( ptopSector->IsDark() || IsStatFlag( STATF_InDoors ))
					return( false );
			}
		}

		if ( Noto_IsCriminal() && Stat_GetAdjusted(STAT_INT) > 20 )	// I'm evil
		{
			if ( ! m_pArea->IsGuarded() && pArea->IsGuarded())		// too smart for this.
			{
				return( false );
			}
		}
	}

	CCharBase * pCharDef = Char_GetDef();
	if ( !pt.IsValidXY() )
		return true;

	bool fAvoid = false;
	int iIntToAvoid = 10;	// how intelligent do i have to be to avoid this.

	// Is there a nasty object here that will hurt us ?
	CWorldSearch AreaItems( pt );
	while (true)
	{
		CItem * pItem = AreaItems.GetItem();
		if ( pItem == NULL )
			break;

		int zdiff = pItem->GetTopZ() - pt.m_z;
		if ( abs(zdiff) > 3 )
			continue;

		switch ( pItem->GetType() )
		{
		case IT_SHRINE:
			// always avoid.
			return( false );

		case IT_WEB:
			if ( GetDispID() == CREID_GIANT_SPIDER )
				continue;
			iIntToAvoid = 80;
			fAvoid = true;
			break;
		case IT_FIRE: // fire object hurts us ?
			if ( pCharDef->Can(CAN_C_FIRE_IMMUNE))	// i like fire.
				continue;
			iIntToAvoid = 20;	// most creatures recognize fire as bad.
			fAvoid = true;
			break;
		case IT_SPELL:
			switch ( pItem->m_itSpell.m_spell )
			{
			case SPELL_Fire_Field:		iIntToAvoid = 100;		break;
			case SPELL_Poison_Field:	iIntToAvoid = 100;		break;
			case SPELL_Paralyze_Field:	iIntToAvoid = 100;		break;
			default:					iIntToAvoid = 150;		break;
			}	
			fAvoid = true;
			break;
		case IT_TRAP:
			iIntToAvoid = 150;
			fAvoid = true;
			break;
		case IT_TRAP_ACTIVE:
			iIntToAvoid = 50;
			fAvoid = true;
			break;
		case IT_MOONGATE:
		case IT_TELEPAD:
			fAvoid = true;
			break;
		}
	}
	if ( fAvoid )
	{
		if ( Calc_GetRandVal( Stat_GetAdjusted(STAT_INT)) > Calc_GetRandVal( iIntToAvoid ))
			return( false );
	}
	return( true );
}



CItemVendable * CChar::NPC_FindVendableItem( CItemVendable * pVendItem, CItemContainer * pContBuy, CItemContainer * pContStock ) // static
{
	ADDTOCALLSTACK("CChar::NPC_FindVendableItem");
	// Does the NPC want to buy this item
	if ( !pVendItem || !pContBuy || !pVendItem->IsValidSaleItem(false) )
		return NULL;

	CItem * pItemTest = pContBuy->ContentFind( RESOURCE_ID(RES_ITEMDEF, pVendItem->GetID()));
	if ( pItemTest == NULL )
	{
		/*if ( pContStock )
		{
			pItemTest = pContStock->ContentFind( RESOURCE_ID(RES_ITEMDEF, pVendItem->GetID()));
		}
		if ( pItemTest == NULL )*/
			return NULL;
	}

	CItemVendable * pItemSell = dynamic_cast <CItemVendable *> (pItemTest);
	if ( pItemSell == NULL )
	{
		// This is odd. The vendor should not have had this Item !
		return( NULL );
	}

	if ( pVendItem->GetType() != pItemSell->GetType())	// sanity check.
		return( NULL );

	return( pItemSell );
}

////////////////////////////////////////////////////////////////////
// This stuff is still questionable.


int CChar::NPC_WantThisItem( CItem * pItem ) const
{
	ADDTOCALLSTACK("CChar::NPC_WantThisItem");
	//  This should be the ULTIMATE place to check if the NPC wants this in any way.
	//  May not want to use it but rather just put it in my pack.
	//
	// NOTE:
	//  Don't check if i can see this or i can reach it.
	//  Don't check if i can carry this ?
	//
	//  Also take into consideration that some items such as:
	// ex. i want to eat fruit off a fruit tree.
	// ex. i want raw food that may be cooked etc.
	// ex. I want a corpse that i can cut up and eat the parts.
	// ex. I want a chest to loot.
	//
	// RETURN:
	//  0-100 percent = how bad do we want it ?

	// a container i would loot.

	if ( ! CanMove( pItem, false ))
	{
		// Some items like fruit trees, chests and corpses might still be useful.
		return( 0 );
	}

	CCharBase * pCharDef = Char_GetDef();
	int iRet = pCharDef->m_Desires.FindResourceMatch(pItem);
	if ( iRet >= 0 )
	{
		return( pCharDef->m_Desires[iRet].GetResQty() );
	}

	// i'm hungry and this is food ?
	int iFoodLevel = Food_GetLevelPercent();
	if ( Food_CanEat( pItem ) && iFoodLevel < 100 )
	{
		return( 100 - iFoodLevel );
	}

	// If i'm a vendor. is it something i would buy ?
	if ( NPC_IsVendor())
	{
		CItemVendable * pItemSell = NPC_FindVendableItem(
			dynamic_cast <CItemVendable *> (pItem),
			const_cast <CChar*>(this)->GetBank( LAYER_VENDOR_BUYS ),
			const_cast <CChar*>(this)->GetBank( LAYER_VENDOR_STOCK ));
		if ( pItemSell )
			return( pItemSell->GetVendorPrice(0) );
	}

	bool bWantGold = ((( m_pNPC->m_Brain >= NPCBRAIN_HUMAN ) && ( m_pNPC->m_Brain <= NPCBRAIN_THIEF )) ||
						( m_pNPC->m_Brain == NPCBRAIN_VENDOR_OFFDUTY ));

	//	I need for gold
	if ( bWantGold )
	{
		//	wish for gold and valuables too (that can be sold)
		if ( pItem->IsType(IT_COIN) || pItem->IsType(IT_GOLD) ) return 100;
		else if (( Stat_GetAdjusted(STAT_INT) > 50 ) && dynamic_cast <CItemVendable*>(pItem)) return (Calc_GetRandVal(50)+50);
	}

	// I guess I don't want it
	return 0;
}

int CChar::NPC_GetWeaponUseScore( CItem * pWeapon )
{
	ADDTOCALLSTACK("CChar::NPC_GetWeaponUseScore");
	// How good would i be at this weapon ?

	SKILL_TYPE skill;

	if ( !pWeapon )
		skill = SKILL_WRESTLING;
	else
	{
		// Is it a weapon ?
		skill = pWeapon->Weapon_GetSkill();
		if ( skill == SKILL_WRESTLING )
			return( 0 );

		// I can't equip this anyhow.
		if ( CanEquipLayer( pWeapon, LAYER_QTY, NULL, true ) == LAYER_NONE )
			return( 0 );
		// How much damage could i do with this ?
	}

	int iDmg = Fight_CalcDamage( pWeapon, skill );
	int iSkillLevel = Skill_GetAdjusted( skill );

	return( iSkillLevel + iDmg * 50 );
}

int CChar::NPC_GetHostilityLevelToward( const CChar * pCharTarg ) const
{
	ADDTOCALLSTACK("CChar::NPC_GetHostilityLevelToward");
	// What is my general hostility level toward this type of creature ?
	//
	// based on:
	//  npc vs player, (evil npc's don't like players regurdless of align, xcept in town)
	//  karma (we are of different alignments)
	//  creature body type. (allie groups)
	//  hunger, (they could be food)
	//  memories of this creature.
	//
	// DO NOT consider:
	//   strength, he is far stronger or waeker than me.
	//	 health, i may be near death.
	//   location (guarded area), (xcept in the case that evil people like other evils in town)
	//   loot, etc.
	//
	// RETURN:
	//   100 = extreme hatred.
	//   0 = neutral.
	//   -100 = love them
	//


	if ( !pCharTarg || !m_pNPC )
		return 0;

	int iHostility = 0;

	// if it is a pet - register it the same as it's master.
	CChar * pCharOwn = pCharTarg->NPC_PetGetOwner();
	if ( pCharOwn != NULL && pCharOwn != this )
	{
		static int sm_iReentrant = 0;
		if (sm_iReentrant > 32)
		{
			DEBUG_ERR(("Too many owners (circular ownership?) to continue acquiring hostility level towards %s uid=0%x\n", pCharOwn->GetName(), pCharOwn->GetUID().GetPrivateUID()));
			return 0;
		}

		++sm_iReentrant;
		iHostility = NPC_GetHostilityLevelToward( pCharOwn );
		--sm_iReentrant;

		return iHostility;
	}

	int iKarma = Stat_GetAdjusted(STAT_KARMA);

	bool fDoMemBase = false;

	if ( Noto_IsEvil() &&	// i am evil.
		(m_pArea && !m_pArea->IsGuarded()) &&	// we are not in an evil town.
		pCharTarg->m_pPlayer )	// my target is a player.
	{
		// If i'm evil i give no benefit to players with bad karma.
		// I hate all players.
		// Unless i'm in a guarded area. then they are cool.
		iHostility = 51;
	}
	else if ( m_pNPC->m_Brain == NPCBRAIN_BERSERK )	// i'm beserk.
	{
		// beserks just hate everyone all the time.
		iHostility = 100;
	}
	else if ( pCharTarg->m_pNPC &&	// my target is an NPC
		pCharTarg->m_pNPC->m_Brain != NPCBRAIN_BERSERK &&	// ok to hate beserks.
		! g_Cfg.m_fMonsterFight )		// monsters are not supposed to fight other monsters !
	{
		iHostility = -50;
		fDoMemBase = true;	// set this low in case we are defending ourselves. but not attack for hunger.
	}
	else
	{
		// base hostillity on karma diff.

		int iKarmaTarg = pCharTarg->Stat_GetAdjusted(STAT_KARMA);
		int iKarmaDiff = iKarma - iKarmaTarg;

		if ( Noto_IsEvil())
		{
			// I'm evil.
			if ( iKarmaTarg > 0 )
			{
				iHostility += ( iKarmaTarg ) / 1024;
			}
		}
		else if ( iKarma > 300 )
		{
			// I'm good and my target is evil.
			if ( iKarmaTarg < -100 )
			{
				iHostility += ( -iKarmaTarg ) / 1024;
			}
		}
	}

	// Based on just creature type.

	if ( ! fDoMemBase )
	{
		if ( pCharTarg->m_pNPC )
		{
			// Human NPC's will attack humans .

			if ( GetDispID() == pCharTarg->GetDispID())
			{
				// I will never attack those of my own kind...even if starving
				iHostility -= 100;
			}
			else if ( NPC_GetAllyGroupType( GetDispID()) == NPC_GetAllyGroupType(pCharTarg->GetDispID()))
			{
				iHostility -= 50;
			}
			else if ( pCharTarg->m_pNPC->m_Brain == m_pNPC->m_Brain )	// My basic kind
			{
				// Won't attack other monsters. (unless very hungry)
				iHostility -= 30;
			}
		}
		else
		{
			// Not immediately hostile if looks the same as me.
			if ( ! IsHuman() && NPC_GetAllyGroupType( GetDispID()) == NPC_GetAllyGroupType(pCharTarg->GetDispID()))
			{
				iHostility -= 51;
			}
		}
	}

	// I have been attacked/angered by this creature before ?
	CItemMemory * pMemory = Memory_FindObjTypes( pCharTarg, MEMORY_FIGHT|MEMORY_HARMEDBY|MEMORY_IRRITATEDBY|MEMORY_SAWCRIME|MEMORY_AGGREIVED );
	if ( pMemory )
	{
		iHostility += 50;
	}

#ifdef _ALPHASPHERE
	/* 
		foes or allies? 
		Defined by tag.ff_group, tag.ff_foe and/or tag.ff_ally
	*/
	CVarDefCont	*pVar, *pVarTarg;
	int iffCounter=0, iffGroup=0;

	pVarTarg = pCharTarg->m_TagDefs.GetKey("FF_GROUP");
	if ( pVarTarg )
	{
		iffGroup = pVarTarg->GetValNum();
		pVar = m_TagDefs.GetKey("FF_FOE");
		if ( pVar )
		{
			iffCounter = pVar->GetValNum();
			if ( iffCounter & iffGroup )
			{
				// is an enemy
				iHostility += 70;
			}
		} else {
			pVar = m_TagDefs.GetKey("FF_ALLY");
			if ( pVar )
			{
				iffCounter = pVar->GetValNum();
				if ( iffCounter & iffGroup )
				{
					// is an ally
					iHostility -= 70;
				}
			}
		}

		if ( iHostility > 100 )
			iHostility = 100;
		else if ( iHostility < -100 )
			iHostility = -100;
		TCHAR *pszTmp = Str_GetTemp();
		sprintf(pszTmp,"char = %s, target = %s, iHostility = %d, iffGroup = %d, iffCounter = %d\n",GetName(),pCharTarg->GetName(),iHostility,iffGroup,iffCounter);
		DEBUG_WARN((pszTmp));
	}
#endif

	return( iHostility );
}

int CChar::NPC_GetAttackContinueMotivation( CChar * pChar, int iMotivation ) const
{
	ADDTOCALLSTACK("CChar::NPC_GetAttackContinueMotivation");
	// I have seen fit to attack them.
	// How much do i want to continue an existing fight ? cowardice ?
	// ARGS:
	//  iMotivation = My base motivation toward this creature.
	//
	// RETURN:
	// -101 = ? dead meat. (run away)
	//
	// 0 = I'm have no interest.
	// 50 = even match.
	// 100 = he's a push over.
	if ( !m_pNPC )
		return 0;

	if ( pChar->IsStatFlag( STATF_DEAD | STATF_INVUL | STATF_Stone ))
		return( -100 );

	if ( m_pNPC->m_Brain == NPCBRAIN_BERSERK )
	{
		// Less interested the further away they are.
		return( iMotivation + 80 - GetDist( pChar ));
	}

	// Try to stay on one target.
	if ( Fight_IsActive() && m_Act_Targ == pChar->GetUID())
		iMotivation += 8;

	// Less interested the further away they are.
	iMotivation -= GetDist( pChar );

	// Undead are fearless.
	if ( m_pNPC->m_Brain == NPCBRAIN_UNDEAD	|| m_pNPC->m_Brain == NPCBRAIN_GUARD )
	{
		iMotivation += 90;
		return( iMotivation );
	}

	if ( ! g_Cfg.m_fMonsterFear )
	{
		return( iMotivation );
	}

	// I'm just plain stronger.
	iMotivation += ( Stat_GetAdjusted(STAT_STR) - pChar->Stat_GetAdjusted(STAT_STR));

	// I'm healthy.
	int iTmp = GetHealthPercent() - pChar->GetHealthPercent();
	if ( iTmp < -50 )
		iMotivation -= 50;
	else if ( iTmp > 50 )
		iMotivation += 50;

	// I'm smart and therefore more cowardly. (if injured)
	iMotivation -= Stat_GetAdjusted(STAT_INT) / 16;

	return( iMotivation );
}

int CChar::NPC_GetAttackMotivation( CChar * pChar, int iMotivation ) const
{
	ADDTOCALLSTACK("CChar::NPC_GetAttackMotivation");
	// Some sort of monster.
	// Am I stronger than he is ? Should I continue fighting ?
	// Take into consideration AC, health, skills, etc..
	// RETURN:
	// <-1 = dead meat. (run away)
	// 0 = I'm have no interest.
	// 50 = even match.
	// 100 = he's a push over.

	if ( !m_pNPC || !pChar || !pChar->m_pArea )
		return 0;
	if ( Stat_GetVal(STAT_STR) <= 0 )
		return( -1 );	// I'm dead.
	// Is the target interesting ?
	if ( pChar->m_pArea->IsFlag( REGION_FLAG_SAFE ))	// universal
		return( 0 );

	// If the area is guarded then think better of this.
	if ( pChar->m_pArea->IsGuarded() && m_pNPC->m_Brain != NPCBRAIN_GUARD )		// too smart for this.
	{
		iMotivation -= Stat_GetAdjusted(STAT_INT) / 20;
	}

	// Owned by or is one of my kind ?

	iMotivation += NPC_GetHostilityLevelToward( pChar );

	if ( iMotivation > 0 )
	{
		// Am i injured etc ?
		iMotivation = NPC_GetAttackContinueMotivation( pChar, iMotivation );
	}
	return iMotivation;
}

