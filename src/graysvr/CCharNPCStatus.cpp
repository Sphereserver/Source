// Test things to judge what an NPC might be thinking. (want to do)
// But take no actions here.
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
			return CREID_MAN;
		case CREID_ELFMAN:
		case CREID_ELFWOMAN:
		case CREID_ELFGHOSTMAN:
		case CREID_ELFGHOSTWOMAN:
			return CREID_ELFMAN;
		case CREID_GARGMAN:
		case CREID_GARGWOMAN:
		case CREID_GARGGHOSTMAN:
		case CREID_GARGGHOSTWOMAN:
			return CREID_GARGMAN;
		case CREID_ETTIN:
		case CREID_ETTIN_AXE:
			return CREID_ETTIN;
		case CREID_ORC_LORD:
		case CREID_ORC:
		case CREID_ORC_CLUB:
			return CREID_ORC;
		case CREID_DAEMON:
		case CREID_DAEMON_SWORD:
			return CREID_DAEMON;
		case CREID_DRAGON_GREY:
		case CREID_DRAGON_RED:
		case CREID_DRAKE_GREY:
		case CREID_DRAKE_RED:
			return CREID_DRAGON_GREY;
		case CREID_LIZMAN:
		case CREID_LIZMAN_SPEAR:
		case CREID_LIZMAN_MACE:
			return CREID_LIZMAN;
		case CREID_RATMAN:
		case CREID_RATMAN_CLUB:
		case CREID_RATMAN_SWORD:
			return CREID_RATMAN;
		case CREID_SKELETON:
		case CREID_SKEL_AXE:
		case CREID_SKEL_SW_SH:
			return CREID_SKELETON;
		case CREID_TROLL_SWORD:
		case CREID_TROLL:
		case CREID_TROLL_MACE:
			return CREID_TROLL;
		case CREID_Tera_Warrior:
		case CREID_Tera_Drone:
		case CREID_Tera_Matriarch:
			return CREID_Tera_Drone;
		case CREID_Ophid_Mage:
		case CREID_Ophid_Warrior:
		case CREID_Ophid_Queen:
			return CREID_Ophid_Warrior;
		case CREID_HORSE1:
		case CREID_HORSE4:
		case CREID_HORSE2:
		case CREID_HORSE3:
		case CREID_HORSE_PACK:
			return CREID_HORSE1;
		case CREID_BrownBear:
		case CREID_GrizzlyBear:
		case CREID_PolarBear:
			return CREID_BrownBear;
		case CREID_Cow_BW:
		case CREID_Cow2:
		case CREID_Bull_Brown:
		case CREID_Bull2:
			return CREID_Bull_Brown;
		case CREID_Ostard_Desert:
		case CREID_Ostard_Frenz:
		case CREID_Ostard_Forest:
			return CREID_Ostard_Forest;
		case CREID_Sheep:
		case CREID_Sheep_Sheered:
			return CREID_Sheep;
		case CREID_Hart:
		case CREID_Deer:
			return CREID_Deer;
		case CREID_Pig:
		case CREID_Boar:
			return CREID_Pig;
		case CREID_Llama:
		case CREID_LLAMA_PACK:
			return CREID_Llama;
		default:
			return idTest;
	}
}

int CChar::NPC_GetVendorMarkup() const
{
	ADDTOCALLSTACK("CChar::NPC_GetVendorMarkup");
	// This vendor marks stuff up/down this percentage.
	// When vendor sells to players this is the markup value.
	//
	// RETURN:
	//  +100% = double price
	//  0% = default price
	//  -100% = free

	if ( IsStatFlag(STATF_Pet) )	// not on a hired vendor
		return 0;

	// Use char value
	CVarDefCont *pVar = m_TagDefs.GetKey("VENDORMARKUP");
	if ( pVar )
		return static_cast<int>(pVar->GetValNum());

	// Use region value
	if ( m_pArea )
	{
		pVar = m_pArea->m_TagDefs.GetKey("VENDORMARKUP");
		if ( pVar )
			return static_cast<int>(pVar->GetValNum());
	}

	// Use chardef value
	CCharBase *pCharDef = Char_GetDef();
	if ( pCharDef )
	{
		pVar = pCharDef->m_TagDefs.GetKey("VENDORMARKUP");
		if ( pVar )
			return static_cast<int>(pVar->GetValNum());
	}

	// Use default value
	return 15;
}

size_t CChar::NPC_OnHearName(LPCTSTR pszText) const
{
	ADDTOCALLSTACK("CChar::NPC_OnHearName");
	// Did I just hear my name in this text ?
	// should be able to deal with "hi Dennis" in the future.
	// RETURN:
	//  index to skip past the name.

	LPCTSTR pszName = GetName();

	size_t i = FindStrWord(pszText, pszName);
	if ( i )
		return i;

	// Named the chars type ? (must come first !)
	CCharBase *pCharDef = Char_GetDef();
	pszName = pCharDef->GetTradeName();
	for ( i = 0; pszText[i] != '\0'; i++ )
	{
		if ( pszName[i] == '\0' )
		{
			// found name
			while ( ISWHITESPACE(pszText[i]) )
				i++;
			return i;	// char name found
		}
		if ( toupper(pszName[i]) != toupper(pszText[i]) )	// not the name
			break;
	}

	return 0;
}

bool CChar::NPC_CanSpeak() const
{
	ADDTOCALLSTACK("CChar::NPC_CanSpeak");
	//	players and chars with speech can
	if ( !m_pNPC || (m_pNPC->m_Speech.GetCount() > 0) )
		return true;

	CCharBase *pCharDef = Char_GetDef();
	return (pCharDef && (pCharDef->m_Speech.GetCount() > 0));
}

bool CChar::NPC_FightMayCast(bool fCheckSkill) const
{
	ADDTOCALLSTACK("CChar::NPC_FightMayCast");
	// This NPC could cast spells if they wanted to ?
	// check mana and anti-magic
	// Dont check for skill if !fCheckSkill
	if ( !m_pNPC )
		return false;
	if ( fCheckSkill && !const_cast<CChar *>(this)->Skill_GetMagicRandom(300) )
		return false;
	if ( m_pArea && m_pArea->IsFlag(REGION_ANTIMAGIC_DAMAGE|REGION_FLAG_SAFE) )
		return false;
	if ( Stat_GetVal(STAT_INT) < 5 )
		return false;

	return true;
}

bool CChar::NPC_IsOwnedBy(const CChar *pChar, bool fAllowGM) const
{
	ADDTOCALLSTACK("CChar::NPC_IsOwnedBy");
	// Is pChar my master ?
	// BESERK will not listen to commands tho.
	// fAllowGM = consider GM's to be owners of all NPC's

	if ( !pChar )
		return false;
	if ( pChar == this )
		return true;
	if ( fAllowGM && pChar->IsPriv(PRIV_GM) )
		return (pChar->GetPrivLevel() > GetPrivLevel());
	if ( !m_pNPC || !IsStatFlag(STATF_Pet) )	// shortcut - i'm not a pet.
		return false;
	if ( m_pNPC->m_Brain == NPCBRAIN_BERSERK )	// i cannot be commanded.
		return false;

	return (Memory_FindObjTypes(pChar, MEMORY_IPET) != NULL);
}

CChar *CChar::NPC_PetGetOwner() const
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

bool CChar::NPC_CheckWalkHere(const CPointBase &pt, const CRegionBase *pArea) const
{
	ADDTOCALLSTACK("CChar::NPC_CheckWalkHere");
	// Does the NPC want to walk here ? step on this item ?
	if ( !m_pNPC )
		return false;
	if ( !pt.IsValidXY() )
		return true;

	if ( m_pArea )
	{
		if ( (m_pNPC->m_Brain == NPCBRAIN_GUARD) && !IsStatFlag(STATF_War) )	// guards will want to stay in guard range
		{
			if ( m_pArea->IsGuarded() && !pArea->IsGuarded() )
				return false;
		}

		if ( Noto_IsCriminal() )
		{
			if ( !m_pArea->IsGuarded() && pArea->IsGuarded() )
				return false;
		}
	}

	// Is there a nasty object here that will hurt us ?
	CWorldSearch AreaItems(pt);
	for (;;)
	{
		CItem *pItem = AreaItems.GetItem();
		if ( !pItem )
			break;

		if ( abs(pItem->GetTopZ() - pt.m_z) > 5 )
			continue;

		switch ( pItem->GetType() )
		{
			case IT_WEB:
				return (GetDispID() == CREID_GIANT_SPIDER) ? true : false;
			case IT_FIRE:
				return Can(CAN_C_FIRE_IMMUNE);
			case IT_TRAP:
			case IT_TRAP_ACTIVE:
			case IT_MOONGATE:
			case IT_TELEPAD:
				return false;
			default:
				break;
		}
	}
	return true;
}

CItemVendable *CChar::NPC_FindVendableItem(CItemVendable *pVendItem, CItemContainer *pContBuy)	// static
{
	ADDTOCALLSTACK("CChar::NPC_FindVendableItem");
	// Does the NPC want to buy this item?
	if ( !pVendItem || !pContBuy || !pVendItem->IsValidSaleItem(false) )
		return NULL;

	CItem *pItemTest = pContBuy->ContentFind(RESOURCE_ID(RES_ITEMDEF, pVendItem->GetID()));
	if ( !pItemTest )
		return NULL;

	CItemVendable *pItemSell = dynamic_cast<CItemVendable *>(pItemTest);
	if ( !pItemSell )	// the item is not vendable
		return NULL;
	if ( pVendItem->GetType() != pItemSell->GetType() )	// sanity check
		return NULL;

	return pItemSell;
}

////////////////////////////////////////////////////////////////////
// This stuff is still questionable.


int CChar::NPC_WantThisItem(CItem *pItem) const
{
	ADDTOCALLSTACK("CChar::NPC_WantThisItem");
	//  This should be the ULTIMATE place to check if the NPC wants this in any way.
	//  May not want to use it but rather just put it in my pack.
	//
	// NOTE:
	//  Don't check if i can see this or i can reach it.
	//  Don't check if i can carry this ?
	//
	// RETURN:
	//  0-100 percent = how bad do we want it ?

	if ( !CanMove(pItem, false) )
		return 0;

	CCharBase *pCharDef = Char_GetDef();
	ASSERT(pCharDef);
	size_t iRet = pCharDef->m_Desires.FindResourceMatch(pItem);
	if ( iRet != pCharDef->m_Desires.BadIndex() )
		return pCharDef->m_Desires[iRet].GetResQty();

	// I'm hungry and this is food ?
	int iFoodLevel = Food_GetLevelPercent();
	if ( (iFoodLevel < 100) && Food_CanEat(pItem) )
		return 100 - iFoodLevel;

	if ( NPC_IsVendor() )
	{
		// Vendors always want money
		if ( pItem->IsType(IT_GOLD) )
			return 100;

		// Is it something I would buy?
		CItemVendable *pItemSell = NPC_FindVendableItem(dynamic_cast<CItemVendable *>(pItem), const_cast<CChar *>(this)->GetContainerCreate(LAYER_VENDOR_BUYS));
		if ( pItemSell )
			return pItemSell->GetVendorPrice(0);
	}

	return 0;
}

int CChar::NPC_GetWeaponUseScore(CItem *pWeapon)
{
	ADDTOCALLSTACK("CChar::NPC_GetWeaponUseScore");
	// How good would i be at this weapon ?

	SKILL_TYPE skill = SKILL_WRESTLING;
	if ( pWeapon )
	{
		skill = pWeapon->Weapon_GetSkill();
		if ( skill == SKILL_WRESTLING )
			return 0;
		if ( CanEquipLayer(pWeapon, LAYER_QTY, NULL, true) == LAYER_NONE )		// I can't equip this
			return 0;
	}

	return Skill_GetAdjusted(skill) + Fight_CalcDamage(pWeapon) * 50;
}

int CChar::NPC_GetHostilityLevelToward(const CChar *pCharTarg) const
{
	ADDTOCALLSTACK("CChar::NPC_GetHostilityLevelToward");
	// What is my general hostility level toward this type of creature ?
	//
	// based on:
	//  npc vs player (evil npc's don't like players)
	//  creature body type (allie groups)
	//
	// RETURN:
	//   100 = extreme hatred.
	//   0 = neutral.
	//   -100 = love them

	if ( !m_pNPC || !pCharTarg )
		return 0;

	// If it's a pet, inherit hostility from it's master
	CChar *pCharOwn = pCharTarg->NPC_PetGetOwner();
	if ( pCharOwn && (pCharOwn != this) )
	{
		static int sm_iReentrant = 0;
		if ( sm_iReentrant > 32 )
		{
			DEBUG_ERR(("Too many owners (circular ownership?) to continue acquiring hostility level towards %s UID=0%" FMTDWORDH "\n", pCharOwn->GetName(), pCharOwn->GetUID().GetPrivateUID()));
			return 0;
		}

		++sm_iReentrant;
		int iHostility = NPC_GetHostilityLevelToward(pCharOwn);
		--sm_iReentrant;
		return iHostility;
	}

	if ( m_pNPC->m_Brain == NPCBRAIN_BERSERK )		// Beserks always hate everyone
		return 100;
	if ( pCharTarg->m_pPlayer )
		return 100;

	if ( pCharTarg->m_pNPC )
	{
		if ( !g_Cfg.m_fMonsterFight )	// Monsters are not supposed to fight other monsters!
			return 0;
		if ( GetDispID() == pCharTarg->GetDispID() )	// I will never attack those of my own kind
			return -100;
		else if ( NPC_GetAllyGroupType(GetDispID()) == NPC_GetAllyGroupType(pCharTarg->GetDispID()) )
			return -50;
		else if ( m_pNPC->m_Brain == pCharTarg->m_pNPC->m_Brain )
			return -30;

		return 100;
	}

	return 0;
}

int CChar::NPC_GetAttackMotivation(CChar *pChar) const
{
	ADDTOCALLSTACK("CChar::NPC_GetAttackMotivation");
	// Some sort of monster.
	// Am I stronger than he is ? Should I continue fighting ?
	// Take into consideration AC, health, skills, etc..
	// RETURN:
	//   < 0 = dead meat. (run away)
	//   0 = I'm have no interest.
	//   50 = even match.
	//   100 = he's a push over.

	if ( !m_pNPC || !pChar || !pChar->m_pArea )
		return 0;
	if ( pChar->m_pArea->IsFlag(REGION_FLAG_SAFE) )
		return 0;

	int iMotivation = NPC_GetHostilityLevelToward(pChar);
	if ( iMotivation <= 0 )
		return iMotivation;

	if ( !pChar->Fight_IsAttackable() )
		return 0;
	if ( (m_pNPC->m_Brain == NPCBRAIN_BERSERK) || (m_pNPC->m_Brain == NPCBRAIN_GUARD) )
		return 100;

	// Try to stay on one target
	if ( Fight_IsActive() && (m_Act_Targ == pChar->GetUID()) )
		iMotivation += 10;

	// Less interested the further away they are
	iMotivation -= GetDist(pChar);

	if ( g_Cfg.m_fMonsterFear )
	{
		if ( GetHealthPercent() < 50 )
			iMotivation -= 50 + (Stat_GetAdjusted(STAT_INT) / 16);
	}
	return iMotivation;
}
