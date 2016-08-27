//  CChar is either an NPC or a Player.
#include <cmath>
#include "graysvr.h"	// predef header.
#include "../network/network.h"

bool CChar::IsResourceMatch( RESOURCE_ID_BASE rid, DWORD dwAmount )
{
	ADDTOCALLSTACK("CChar::IsResourceMatch");
	return IsResourceMatch(rid, dwAmount, 0);
}

bool CChar::IsResourceMatch( RESOURCE_ID_BASE rid, DWORD dwAmount, DWORD dwArgResearch )
{
	ADDTOCALLSTACK("CChar::IsResourceMatch");
	// Is the char a match for this test ?
	switch ( rid.GetResType() )
	{
		case RES_SKILL:			// do I have this skill level?
			if ( Skill_GetBase(static_cast<SKILL_TYPE>(rid.GetResIndex())) < dwAmount )
				return false;
			return true;

		case RES_CHARDEF:		// I'm this type of char?
		{
			CCharBase *pCharDef = Char_GetDef();
			ASSERT(pCharDef);
			if ( pCharDef->GetResourceID() == rid )
				return true;
			return false;
		}

		case RES_SPEECH:		// do I have this speech?
			if ( m_pNPC )
			{
				if ( m_pNPC->m_Speech.ContainsResourceID(rid) )
					return true;
				CCharBase *pCharDef = Char_GetDef();
				ASSERT(pCharDef);
				if ( pCharDef->m_Speech.ContainsResourceID(rid) )
					return true;
			}
			return false;

		case RES_EVENTS:		// do I have these events?
		{
			if ( m_OEvents.ContainsResourceID(rid) )
				return true;
			if ( m_pNPC )
			{
				CCharBase *pCharDef = Char_GetDef();
				ASSERT(pCharDef);
				if ( pCharDef->m_TEvents.ContainsResourceID(rid) )
					return true;
			}
			return false;
		}

		case RES_TYPEDEF:		// do I have these in my posession?
			if ( !ContentConsume(rid, dwAmount, true, dwArgResearch) )
				return true;
			return false;

		case RES_ITEMDEF:		// do I have these in my posession?
			if ( !ContentConsume(rid, dwAmount, true) )
				return true;
			return false;
		default:
			return false;
	}
}

CItemContainer *CChar::GetContainerCreate(LAYER_TYPE layer)
{
	ADDTOCALLSTACK("CChar::GetContainerCreate");
	// Get container on the given layer, or create if it doesn't exist yet

	CItem *pItemTest = LayerFind(layer);
	CItemContainer *pCont = dynamic_cast<CItemContainer *>(pItemTest);
	if ( pCont || g_Serv.IsLoading() )
		return pCont;

	// Add new container if not found
	ITEMID_TYPE id;
	switch ( layer )
	{
		case LAYER_VENDOR_STOCK:
		case LAYER_VENDOR_EXTRA:
		case LAYER_VENDOR_BUYS:
			if ( !NPC_IsVendor() )
				return NULL;
			id = ITEMID_VENDOR_BOX;
			break;

		case LAYER_BANKBOX:
			id = ITEMID_BANK_BOX;
			break;

		default:
			id = ITEMID_BACKPACK;
			break;
	}

	pCont = dynamic_cast<CItemContainer *>(CItem::CreateScript(id, this));
	ASSERT(pCont);
	if ( (layer == LAYER_PACK) || ((layer >= LAYER_VENDOR_STOCK) && (layer <= LAYER_BANKBOX)) )
		pCont->SetAttr(ATTR_NEWBIE|ATTR_MOVE_NEVER);
	LayerAdd(pCont, layer);
	return pCont;
}

CItem *CChar::GetBackpackItem(ITEMID_TYPE id)
{
	ADDTOCALLSTACK("CChar::GetBackpackItem");
	CItemContainer *pPack = GetContainer(LAYER_PACK);
	if ( pPack )
	{
		for ( CItem *pItem = pPack->GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
		{
			if ( pItem->GetID() == id )
				return pItem;
		}
	}
	return NULL;
}

CItem *CChar::LayerFind( LAYER_TYPE layer ) const
{
	ADDTOCALLSTACK("CChar::LayerFind");
	// Find an item i have equipped.

	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( pItem->GetEquipLayer() == layer )
			return pItem;
	}
	return NULL;
}

TRIGRET_TYPE CChar::OnCharTrigForLayerLoop( CScript &s, CTextConsole *pSrc, CScriptTriggerArgs *pArgs, CGString *pResult, LAYER_TYPE layer )
{
	ADDTOCALLSTACK("CChar::OnCharTrigForLayerLoop");
	CScriptLineContext StartContext = s.GetContext();
	CScriptLineContext EndContext = StartContext;

	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( pItem->GetEquipLayer() == layer )
		{
			TRIGRET_TYPE iRet = pItem->OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult);
			if ( iRet == TRIGRET_BREAK )
			{
				EndContext = StartContext;
				break;
			}
			if ( (iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE) )
				return iRet;
			if ( iRet == TRIGRET_CONTINUE )
				EndContext = StartContext;
			else
				EndContext = s.GetContext();
			s.SeekContext(StartContext);
		}
	}
	if ( EndContext.m_lOffset <= StartContext.m_lOffset )
	{
		// just skip to the end.
		TRIGRET_TYPE iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult);
		if ( iRet != TRIGRET_ENDIF )
			return iRet;
	}
	else
		s.SeekContext(EndContext);
	return TRIGRET_ENDIF;
}

int CChar::GetWeightLoadPercent( int iWeight ) const
{
	ADDTOCALLSTACK("CChar::GetWeightLoadPercent");
	// Get a percent of load.
	if ( IsPriv(PRIV_GM) )
		return 1;
	
	int	MaxCarry = g_Cfg.Calc_MaxCarryWeight(this);
	if ( !MaxCarry )
		return 1000;	// suppose self extra-overloaded

	return IMULDIV( iWeight, 100, MaxCarry);
}

bool CChar::CanCarry( const CItem *pItem ) const
{
	ADDTOCALLSTACK("CChar::CanCarry");
	if ( IsPriv(PRIV_GM) )
		return true;

	int iItemWeight = pItem->GetWeight();
	if ( pItem->GetEquipLayer() == LAYER_DRAGGING )		// if we're dragging the item, its weight is already added on char so don't count it again
		iItemWeight = 0;

	if ( GetTotalWeight() + iItemWeight > g_Cfg.Calc_MaxCarryWeight(this) )
		return false;

	return true;
}

bool CChar::CanEquipStr( CItem *pItem ) const
{
	ADDTOCALLSTACK("CChar::CanEquipStr");
	if ( IsPriv(PRIV_GM) )
		return true;

	CItemBase *pItemDef = pItem->Item_GetDef();
	LAYER_TYPE layer = pItemDef->GetEquipLayer();
	if ( !pItemDef->IsTypeEquippable() || !CItemBase::IsVisibleLayer(layer) )
		return true;

	if ( Stat_GetAdjusted(STAT_STR) >= pItemDef->m_ttEquippable.m_StrReq * (100 - pItem->GetDefNum("LOWERREQ", true)) / 100 )
		return true;

	return false;
}

LAYER_TYPE CChar::CanEquipLayer( CItem *pItem, LAYER_TYPE layer, CChar *pCharMsg, bool fTest )
{
	ADDTOCALLSTACK("CChar::CanEquipLayer");
	// This takes care of any conflicting items in the slot !
	// NOTE: Do not check to see if i can pick this up or steal this etc.
	// LAYER_NONE = can't equip this .

	CCharBase *pCharDef = Char_GetDef();
	ASSERT(pCharDef);
	ASSERT(pItem);

	if ( pItem->IsType(IT_SPELL) )	// spell memory conflicts are handled by CChar::Spell_Effect_Create()
		return layer;

	const CItemBase *pItemDef = pItem->Item_GetDef();
	if ( layer >= LAYER_QTY )
	{
		layer = pItemDef->GetEquipLayer();

		if ( (m_pPlayer || fTest) && !CanEquipStr(pItem) )
		{
			if ( m_pPlayer )	// message only players
			{
				SysMessagef("%s %s.", g_Cfg.GetDefaultMsg(DEFMSG_EQUIP_NOT_STRONG_ENOUGH), pItem->GetName());
				if ( pCharMsg && pCharMsg != this )
					pCharMsg->SysMessagef("%s %s.", g_Cfg.GetDefaultMsg(DEFMSG_EQUIP_NOT_STRONG_ENOUGH), pItem->GetName());
			}
			return LAYER_NONE;
		}
	}

	if ( pItem->GetParent() == this && pItem->GetEquipLayer() == layer )		// not a visible item LAYER_TYPE
		return layer;

	CItem *pItemPrev = NULL;
	bool fCantEquip = false;

	switch ( layer )
	{
		case LAYER_PACK:
		case LAYER_AUCTION:
			if ( !pItem->IsType(IT_CONTAINER) )
				fCantEquip = true;
			break;
		case LAYER_NONE:
		case LAYER_SPECIAL:
			switch ( pItem->GetType() )
			{
				case IT_EQ_TRADE_WINDOW:
				case IT_EQ_MEMORY_OBJ:
				case IT_EQ_SCRIPT:
					return LAYER_SPECIAL;	// we can have multiple items of these
				default:
					return LAYER_NONE;		// not legal!
			}
		case LAYER_HAIR:
			if ( !pItem->IsType(IT_HAIR) )
				fCantEquip = true;
			break;
		case LAYER_BEARD:
			if ( !pItem->IsType(IT_BEARD) )
				fCantEquip = true;
			break;
		case LAYER_BANKBOX:
			if ( !pItem->IsType(IT_EQ_BANK_BOX) )
				fCantEquip = true;
			break;
		case LAYER_HORSE:
			if ( !pItem->IsType(IT_EQ_HORSE) || !IsMountCapable() )
				fCantEquip = true;
			break;
		case LAYER_HAND1:
		case LAYER_HAND2:
		{
			if ( !pItemDef->IsTypeEquippable() || !pCharDef->Can(CAN_C_USEHANDS) )
			{
				fCantEquip = true;
				break;
			}

			if ( layer == LAYER_HAND2 )
			{
				// If it's a 2 handed weapon, unequip the other hand
				if ( pItem->IsTypeWeapon() || pItem->IsType(IT_FISH_POLE) )
					pItemPrev = LayerFind(LAYER_HAND1);
			}
			else
			{
				// Unequip 2 handed weapons if we must use the other hand
				pItemPrev = LayerFind(LAYER_HAND2);
				if ( pItemPrev && !pItemPrev->IsTypeWeapon() && !pItemPrev->IsType(IT_FISH_POLE) )
					pItemPrev = NULL;
			}
			break;
		}

		case LAYER_NEWLIGHT:
			// DEBUG_ERR(( "ALERT: Weird layer %d used for '%s' check this\n", LAYER_HIDDEN, pItem->GetResourceName()));
		case LAYER_COLLAR:
		case LAYER_RING:
		case LAYER_EARS:
		case LAYER_TALISMAN:
			break;

		default:
			if ( CItemBase::IsVisibleLayer(layer) )
			{
				if ( !pCharDef->Can(CAN_C_EQUIP) )
				{
					if ( m_pNPC )		// auto-patch NPC adding CAN equip flags
						pCharDef->m_Can |= CAN_C_EQUIP;
					else
						fCantEquip = true;
				}
			}
			break;
	}

	if ( fCantEquip )	// some creatures can equip certain special items ??? (orc lord?)
	{
		if ( pCharMsg )
			pCharMsg->SysMessageDefault(DEFMSG_EQUIP_CANNOT);
		return LAYER_NONE;
	}

	// Check for objects already in this slot. Deal with it.
	if ( !pItemPrev )
		pItemPrev = LayerFind(layer);

	if ( pItemPrev )
	{
		switch ( layer )
		{
			case LAYER_AUCTION:
			case LAYER_PACK:
				return LAYER_NONE;	// this should not happen
			case LAYER_HORSE:
			case LAYER_DRAGGING:
				if ( !fTest )
					ItemBounce(pItemPrev);
				break;
			case LAYER_BEARD:
			case LAYER_HAIR:
				if ( !fTest )
					pItemPrev->Delete();
				break;
			default:
			{
				if ( !CanMove(pItemPrev) )
					return LAYER_NONE;
				if ( !fTest )
					ItemBounce(pItemPrev);
				break;
			}
		}
	}

	return layer;
}

bool CChar::CheckCorpseCrime( const CItemCorpse *pCorpse, bool fLooting, bool fTest )
{
	ADDTOCALLSTACK("CChar::CheckCorpseCrime");
	// fLooting = looting as apposed to carving.
	// RETURN: true = criminal act !

	if ( !pCorpse || !g_Cfg.m_fLootingIsACrime )
		return false;

	CChar *pCharGhost = pCorpse->m_uidLink.CharFind();
	if ( !pCharGhost || pCharGhost == this )
		return false;

	if ( pCharGhost->Noto_GetFlag(this) == NOTO_GOOD )
	{
		if ( !fTest )
		{
			// Anyone saw me doing this?
			CheckCrimeSeen(SKILL_NONE, pCharGhost, pCorpse, fLooting ? g_Cfg.GetDefaultMsg(DEFMSG_LOOTING_CRIME) : NULL);
			Noto_Criminal();
		}
		return true;
	}
	return false;
}

CItemCorpse *CChar::FindMyCorpse( bool ignoreLOS, int iRadius ) const
{
	ADDTOCALLSTACK("CChar::FindMyCorpse");
	// If they are standing on their own corpse then res the corpse !
	CWorldSearch Area(GetTopPoint(), iRadius);
	for (;;)
	{
		CItem *pItem = Area.GetItem();
		if ( !pItem )
			break;
		if ( !pItem->IsType(IT_CORPSE) )
			continue;
		CItemCorpse *pCorpse = dynamic_cast<CItemCorpse*>(pItem);
		if ( !pCorpse || (pCorpse->m_uidLink != GetUID()) )
			continue;
		if ( pCorpse->m_itCorpse.m_BaseID != m_prev_id )	// not morphed type
			continue;
		if ( !ignoreLOS && !CanSeeLOS(pCorpse) )
			continue;
		return pCorpse;
	}
	return NULL;
}

int CChar::GetHealthPercent() const
{
	ADDTOCALLSTACK("CChar::GetHealthPercent");
	short str = Stat_GetAdjusted(STAT_STR);
	if ( !str )
		return 0;
	return IMULDIV(Stat_GetVal(STAT_STR), 100, str);
}

bool CChar::IsSwimming() const
{
	ADDTOCALLSTACK("CChar::IsSwimming");
	// Am i in/over/slightly under the water now ?
	// NOTE: This is a bit more complex because we need to test if we are slightly under water.

	CPointMap ptTop = GetTopPoint();

	CPointMap pt = g_World.FindItemTypeNearby(ptTop, IT_WATER);
	if ( !pt.IsValidPoint() )
		return false;

	signed char iDistZ = ptTop.m_z - pt.m_z;
	if ( iDistZ < -PLAYER_HEIGHT )	// far under the water somehow
		return false;

	// Is there a solid surface under us?
	DWORD wBlockFlags = GetMoveBlockFlags();
	signed char iSurfaceZ = g_World.GetHeightPoint2(ptTop, wBlockFlags, true);
	if ( (iSurfaceZ == pt.m_z) && (wBlockFlags & CAN_I_WATER) )
		return true;

	return false;
}

NPCBRAIN_TYPE CChar::GetNPCBrain(bool bGroupTypes) const
{
	ADDTOCALLSTACK("CChar::GetNPCBrain");
	if ( !m_pNPC )
		return NPCBRAIN_NONE;

	if ( bGroupTypes )
	{
		switch ( m_pNPC->m_Brain )
		{
			case NPCBRAIN_HEALER:
			case NPCBRAIN_GUARD:
			case NPCBRAIN_BANKER:
			case NPCBRAIN_VENDOR:
			case NPCBRAIN_STABLE:
				return NPCBRAIN_HUMAN;
			case NPCBRAIN_BERSERK:
			case NPCBRAIN_DRAGON:
				return NPCBRAIN_MONSTER;
			default:
				break;
		}
	}
	return m_pNPC->m_Brain;
}

LPCTSTR CChar::GetPronoun() const
{
	ADDTOCALLSTACK("CChar::GetPronoun");
	switch ( GetDispID() )
	{
		case CREID_MAN:
		case CREID_GHOSTMAN:
		case CREID_ELFMAN:
		case CREID_ELFGHOSTMAN:
		case CREID_GARGMAN:
		case CREID_GARGGHOSTMAN:
			return g_Cfg.GetDefaultMsg(DEFMSG_PRONOUN_HE);
		case CREID_WOMAN:
		case CREID_GHOSTWOMAN:
		case CREID_ELFWOMAN:
		case CREID_ELFGHOSTWOMAN:
		case CREID_GARGWOMAN:
		case CREID_GARGGHOSTWOMAN:
			return g_Cfg.GetDefaultMsg(DEFMSG_PRONOUN_SHE);
		default:
			return g_Cfg.GetDefaultMsg(DEFMSG_PRONOUN_IT);
	}
}

LPCTSTR CChar::GetPossessPronoun() const
{
	ADDTOCALLSTACK("CChar::GetPossessPronoun");
	switch ( GetDispID() )
	{
		case CREID_MAN:
		case CREID_GHOSTMAN:
		case CREID_ELFMAN:
		case CREID_ELFGHOSTMAN:
		case CREID_GARGMAN:
		case CREID_GARGGHOSTMAN:
			return g_Cfg.GetDefaultMsg(DEFMSG_POSSESSPRONOUN_HIS);
		case CREID_WOMAN:
		case CREID_GHOSTWOMAN:
		case CREID_ELFWOMAN:
		case CREID_ELFGHOSTWOMAN:
		case CREID_GARGWOMAN:
		case CREID_GARGGHOSTWOMAN:
			return g_Cfg.GetDefaultMsg(DEFMSG_POSSESSPRONOUN_HER);
		default:
			return g_Cfg.GetDefaultMsg(DEFMSG_POSSESSPRONOUN_ITS);
	}
}

BYTE CChar::GetModeFlag( const CClient *pViewer ) const
{
	ADDTOCALLSTACK("CChar::GetModeFlag");
	CCharBase *pCharDef = Char_GetDef();
	BYTE mode = 0;

	if ( IsStatFlag(STATF_Freeze|STATF_Stone) )
		mode |= CHARMODE_FREEZE;
	if ( pCharDef->IsFemale() )
		mode |= CHARMODE_FEMALE;

	if ( pViewer && (pViewer->GetNetState()->isClientVersion(MINCLIVER_SA) || pViewer->GetNetState()->isClientEnhanced()) )
	{
		if ( IsStatFlag(STATF_Hovering) )
			mode |= CHARMODE_FLYING;
	}
	else
	{
		if ( IsStatFlag(STATF_Poisoned) )
			mode |= CHARMODE_POISON;
	}

	if ( IsStatFlag(STATF_INVUL) )
		mode |= CHARMODE_YELLOW;
	if ( GetPrivLevel() > PLEVEL_Player )
		mode |= CHARMODE_IGNOREMOBS;
	if ( IsStatFlag(STATF_War) )
		mode |= CHARMODE_WAR;

	DWORD dwFlags = STATF_Sleeping;
	if ( !g_Cfg.m_iColorInvis )	//This is needed for Serv.ColorInvis to work, proper flags must be set
		dwFlags |= STATF_Insubstantial;
	if ( !g_Cfg.m_iColorHidden )	//serv.ColorHidden
		dwFlags |= STATF_Hidden;
	if ( !g_Cfg.m_iColorInvisSpell )	//serv.ColorInvisSpell
		dwFlags |= STATF_Invisible;
	if ( IsStatFlag(dwFlags) )	// Checking if I have any of these settings enabled on the ini and I have any of them, if so ... CHARMODE_INVIS is set and color applied.
		mode |= CHARMODE_INVIS;

	return mode;
}

BYTE CChar::GetLightLevel() const
{
	ADDTOCALLSTACK("CChar::GetLightLevel");
	// Get personal light level.

	if ( IsStatFlag(STATF_DEAD|STATF_Sleeping|STATF_NightSight) || IsPriv(PRIV_DEBUG) )
		return LIGHT_BRIGHT;
	if ( (g_Cfg.m_iRacialFlags & RACIALF_ELF_NIGHTSIGHT) && IsElf() )		// elves always have nightsight enabled (Night Sight racial trait)
		return LIGHT_BRIGHT;
	return GetTopSector()->GetLight();
}

CItem *CChar::GetSpellbook(SPELL_TYPE iSpell) const	// Retrieves a spellbook from the magic school given in iSpell
{
	ADDTOCALLSTACK("CChar::GetSpellbook");
	// Search for suitable book in hands first
	CItem *pReturn = NULL;
	for ( CItem *pBook = GetContentHead(); pBook != NULL; pBook = pBook->GetNext() )
	{
		if ( !pBook->IsTypeSpellbook() )
			continue;
		if ( (iSpell > pBook->m_itSpellbook.m_baseid) && (iSpell - (pBook->m_itSpellbook.m_baseid + 1) < 96) )
		{
			if ( pBook->IsSpellInBook(iSpell) )	//We found a book with this same spell, nothing more to do.
				return pBook;	
			else
				pReturn = pBook;	// We did not find the spell, but this book is of the same school ... we'll return this book if none better is found (NOTE: some book must be returned or the code will think that we don't have a book).
		}
	}

	// Then search in the top level of the pack
	CItemContainer *pPack = GetContainer(LAYER_PACK);
	if ( pPack )
	{
		for ( CItem *pBook = pPack->GetContentHead(); pBook != NULL; pBook = pBook->GetNext() )
		{
			if ( !pBook->IsTypeSpellbook() )
				continue;
			if ( (iSpell > pBook->m_itSpellbook.m_baseid) && (iSpell - (pBook->m_itSpellbook.m_baseid + 1) < 96) )
			{
				if ( pBook->IsSpellInBook(iSpell) )	//We found a book with this same spell, nothing more to do.
					return pBook;	
				else
					pReturn = pBook;	// We did not find the spell, but this book is of the same school ... we'll return this book if none better is found (NOTE: some book must be returned or the code will think that we don't have a book).
			}
		}
	}
	return pReturn;
}

int CChar::GetSpellbookExtra(CItem *pBooks[], int &count) const	// Retrieves a spellbook from the magic school given in iSpell
{
	ADDTOCALLSTACK("CChar::GetSpellbook");
	// Search for suitable book in hands first
	for ( CItem *pBook = GetContentHead(); pBook != NULL; pBook = pBook->GetNext() )
	{
		if ( pBook->GetType() != IT_SPELLBOOK_EXTRA )
			continue;
		if ( pBook->m_itSpellbook.m_baseid )
			pBooks[++count] = pBook;
	}

	// Then search in the top level of the pack
	CItemContainer *pPack = GetContainer(LAYER_PACK);
	if ( pPack )
	{
		for ( CItem *pBook = pPack->GetContentHead(); pBook != NULL; pBook = pBook->GetNext() )
		{
			if ( pBook->GetType() != IT_SPELLBOOK_EXTRA )
				continue;
			if ( pBook->m_itSpellbook.m_baseid )
				pBooks[++count] = pBook;
		}
	}
	return count;
}

CItem *CChar::GetSpellbookRandom(SPELL_TYPE iSpell) const	// Retrieves a spellbook from the magic school given in iSpell
{
	ADDTOCALLSTACK("CChar::GetSpellbook");
	CItem *pBook = NULL;
	CItem *pBooks[static_cast<int>(SKILL_QTY)];
	// Search for suitable book in hands first
	int count = 0;
	for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; i++ )
	{
		SKILL_TYPE skill = static_cast<SKILL_TYPE>(i);
		CSkillDef *pSkillDef = g_Cfg.GetSkillDef(skill);
		if ( !pSkillDef )
			continue;
		if ( g_Cfg.IsSkillFlag(skill, SKF_MAGIC) && (Skill_GetBase(skill) > 1) )	//only selecting skills with value > 0.1
		{
			pBook = GetSpellbook(iSpell ? iSpell : const_cast<CChar*>(this)->Spell_GetIndex(skill));
			if ( pBook )
				pBooks[++count] = pBook;
		}
	}
	GetSpellbookExtra(pBooks, count);	// add extra spellbooks to the list 
	if ( count > 0 )
	{
		int rand = Calc_GetRandVal2(1, count);
		return pBooks[rand];
	}
	return NULL;
}

short CChar::Food_GetLevelPercent() const
{
	ADDTOCALLSTACK("CChar::Food_GetLevelPercent");
	short max = Stat_GetMax(STAT_FOOD);
	if ( max == 0 )
		return 100;
	return static_cast<short>(IMULDIV(Stat_GetVal(STAT_FOOD), 100, max));
}

LPCTSTR CChar::Food_GetLevelMessage(bool fPet, bool fHappy) const
{
	ADDTOCALLSTACK("CChar::Food_GetLevelMessage");
	short max = Stat_GetMax(STAT_FOOD);
	if ( max == 0 )
		return g_Cfg.GetDefaultMsg(DEFMSG_PET_HAPPY_UNAFFECTED);

	size_t index = IMULDIV(Stat_GetVal(STAT_FOOD), 8, max);

	if ( fPet )
	{
		static LPCTSTR const sm_szPetHunger[] =
		{
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_HAPPY_1),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_HAPPY_2),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_HAPPY_3),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_HAPPY_4),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_HAPPY_5),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_HAPPY_6),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_HAPPY_7),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_HAPPY_8)
		};
		static LPCTSTR const sm_szPetHappy[] =
		{
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_FOOD_1),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_FOOD_2),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_FOOD_3),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_FOOD_4),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_FOOD_5),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_FOOD_6),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_FOOD_7),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_FOOD_8)
		};

		if ( index >= (COUNTOF(sm_szPetHunger) - 1) )
			index = COUNTOF(sm_szPetHunger) - 1;

		return fHappy ? sm_szPetHappy[index] : sm_szPetHunger[index];
	}

	static LPCTSTR const sm_szFoodLevel[] =
	{
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_1),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_2),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_3),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_4),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_5),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_6),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_7),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_8)
	};

	if ( index >= (COUNTOF(sm_szFoodLevel) - 1) )
		index = COUNTOF(sm_szFoodLevel) - 1;

	return sm_szFoodLevel[index];
}

short CChar::Food_CanEat( CObjBase *pObj ) const
{
	ADDTOCALLSTACK("CChar::Food_CanEat");
	// Would i want to eat this creature ? hehe
	// would i want to eat some of this item ?
	// 0 = not at all.
	// 10 = only if starving.
	// 20 = needs to be prepared.
	// 50 = not bad.
	// 75 = yummy.
	// 100 = my favorite (i will drop other things to get this).

	CCharBase *pCharDef = Char_GetDef();
	ASSERT(pCharDef);

	size_t iRet = pCharDef->m_FoodType.FindResourceMatch(pObj);
	if ( iRet != pCharDef->m_FoodType.BadIndex() )
		return static_cast<short>(pCharDef->m_FoodType[iRet].GetResQty());	// how bad do i want it?

	return 0;
}

LPCTSTR CChar::GetTradeTitle() const // Paperdoll title for character p (2)
{
	ADDTOCALLSTACK("CChar::GetTradeTitle");
	if ( !m_sTitle.IsEmpty() )
		return m_sTitle;

	TCHAR *pTemp = Str_GetTemp();
	CCharBase *pCharDef = Char_GetDef();
	ASSERT(pCharDef);

	if ( m_pNPC && (pCharDef->GetTypeName() != pCharDef->GetTradeName()) )
	{
		sprintf(pTemp, "%s %s", pCharDef->IsFemale() ? g_Cfg.GetDefaultMsg(DEFMSG_TRADETITLE_ARTICLE_FEMALE) : g_Cfg.GetDefaultMsg(DEFMSG_TRADETITLE_ARTICLE_MALE), pCharDef->GetTradeName());
		return pTemp;
	}

	// Only players can have skill titles
	if ( !m_pPlayer )
		return pTemp;

	int len;
	SKILL_TYPE skill = Skill_GetBest();
	if ( skill == SKILL_NINJITSU )
	{
		static const CValStr sm_SkillTitles[] =
		{
			{ "", INT_MIN },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NEOPHYTE), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_NEOPHYTE")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NOVICE), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_NOVICE")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_APPRENTICE), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_APPRENTICE")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_JOURNEYMAN), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_JOURNEYMAN")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_EXPERT), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_EXPERT")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_ADEPT), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_ADEPT")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_MASTER), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_MASTER")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_GRANDMASTER), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_GRANDMASTER")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_ELDER_NINJITSU), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_ELDER")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_LEGENDARY_NINJITSU), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_LEGENDARY")) },
			{ NULL, INT_MAX }
		};
		len = sprintf(pTemp, "%s ", sm_SkillTitles->FindName(Skill_GetBase(skill)));
	}
	else if ( skill == SKILL_BUSHIDO )
	{
		static const CValStr sm_SkillTitles[] =
		{
			{ "", INT_MIN },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NEOPHYTE), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_NEOPHYTE")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NOVICE), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_NOVICE")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_APPRENTICE), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_APPRENTICE")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_JOURNEYMAN), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_JOURNEYMAN")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_EXPERT), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_EXPERT")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_ADEPT), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_ADEPT")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_MASTER), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_MASTER")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_GRANDMASTER), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_GRANDMASTER")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_ELDER_BUSHIDO), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_ELDER")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_LEGENDARY_BUSHIDO), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_LEGENDARY")) },
			{ NULL, INT_MAX }
		};
		len = sprintf(pTemp, "%s ", sm_SkillTitles->FindName(Skill_GetBase(skill)));
	}
	else
	{
		static const CValStr sm_SkillTitles[] =
		{
			{ "", INT_MIN },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NEOPHYTE), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_NEOPHYTE")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NOVICE), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_NOVICE")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_APPRENTICE), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_APPRENTICE")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_JOURNEYMAN), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_JOURNEYMAN")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_EXPERT), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_EXPERT")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_ADEPT), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_ADEPT")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_MASTER), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_MASTER")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_GRANDMASTER), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_GRANDMASTER")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_ELDER), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_ELDER")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_LEGENDARY), static_cast<int>(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_LEGENDARY")) },
			{ NULL, INT_MAX }
		};
		len = sprintf(pTemp, "%s ", sm_SkillTitles->FindName(Skill_GetBase(skill)));
	}

	sprintf(pTemp + len, "%s", static_cast<LPCTSTR>(g_Cfg.GetSkillDef(skill)->m_sTitle));
	return pTemp;
}

bool CChar::CanDisturb( const CChar *pChar ) const
{
	ADDTOCALLSTACK("CChar::CanDisturb");
	// I can see/disturb only players with priv same/less than me.
	if ( !pChar )
		return false;
	if ( GetPrivLevel() < pChar->GetPrivLevel() )
		return !pChar->IsStatFlag(STATF_Insubstantial|STATF_INVUL);
	return true;
}

bool CChar::CanSeeAsDead( const CChar *pChar) const
{
	ADDTOCALLSTACK("CChar::CanSeeAsDead");
	int iDeadCannotSee = g_Cfg.m_fDeadCannotSeeLiving;
	if ( iDeadCannotSee && !pChar->IsStatFlag(STATF_DEAD) && !IsPriv(PRIV_GM) )
	{
		if ( pChar->m_pPlayer )
		{
			if ( iDeadCannotSee == 2 )
				return false;
		}
		else
		{
			if ( (pChar->NPC_PetGetOwner() != this) && (pChar->m_pNPC->m_Brain != NPCBRAIN_HEALER) )
				return false;
		}
	}
	return true;
}

bool CChar::CanSeeInContainer( const CItemContainer *pContItem ) const
{
	ADDTOCALLSTACK("CChar::CanSeeInContainer");
	// This is a container of some sort. Can i see inside it ?

	if ( !pContItem || IsPriv(PRIV_GM) )
		return true;

	if ( pContItem->IsSearchable() )	// not a bank box or locked box
		return true;

	// Not normally searchable.
	// Make some special cases for searchable.

	CChar *pChar = static_cast<CChar*>(pContItem->GetTopLevelObj());
	if ( !pChar )
		return false;

	if ( pContItem->IsType(IT_EQ_TRADE_WINDOW) )
	{
		if ( pChar == this )
			return true;

		CItem *pItemTrade = pContItem->m_uidLink.ItemFind();
		if ( pItemTrade )
		{
			CChar *pCharTrade = static_cast<CChar*>(pItemTrade->GetTopLevelObj());
			if ( pCharTrade == this )
				return true;
		}
		return false;
	}

	if ( !pChar->NPC_IsOwnedBy(this) )		// pets and player vendors
		return false;

	if ( pContItem->IsType(IT_EQ_VENDOR_BOX) || pContItem->IsType(IT_EQ_BANK_BOX) )
	{
		// must be in the same position i opened it legitimately.
		// see addBankOpen
		if ( pContItem->m_itEqBankBox.m_pntOpen != GetTopPoint() )
			return false;
	}
	return true;
}

bool CChar::CanSee( const CObjBaseTemplate *pObj ) const
{
	ADDTOCALLSTACK("CChar::CanSee");
	// Can I see this object (char or item)?
	if ( !pObj || IsDisconnected() || !pObj->GetTopLevelObj()->GetTopPoint().IsValidPoint() )
		return false;

	// First check the distance since if this will fail, we do not need to scan all subcontainers to find this result ;)
	int iVisualRange = GetSight();
	if ( pObj->GetTopLevelObj()->GetTopPoint().GetDistSight(GetTopPoint()) > iVisualRange )
		return false;

	if ( pObj->IsItem() )
	{
		const CItem *pItem = static_cast<const CItem*>(pObj);
		if ( !pItem || !CanSeeItem(pItem) )
			return false;

		CObjBase *pObjCont = pItem->GetParentObj();
		if ( pObjCont )
		{
			if ( !CanSeeInContainer(dynamic_cast<const CItemContainer*>(pObjCont)) )
				return false;

			if ( IsSetEF(EF_FixCanSeeInClosedConts) )
			{
				// A client cannot see the contents of someone else's container, unless they have opened it first
				if ( IsClient() && pObjCont->IsItem() && pObjCont->GetTopLevelObj() != this )
				{
					CClient *pClient = GetClient();
					if ( pClient && pClient->m_openedContainers.find(pObjCont->GetUID().GetPrivateUID()) == pClient->m_openedContainers.end() )
					{
#ifdef _DEBUG
						if ( CanSee(pObjCont) )
						{
#ifdef THREAD_TRACK_CALLSTACK
							StackDebugInformation::printStackTrace();
#endif
							g_Log.EventDebug("%lx:EF_FixCanSeeInClosedConts prevents %s, (0%lx, '%s') from seeing item uid=0%lx (%s, '%s') in container uid=0%lx (%s, '%s')\n",
								pClient->GetSocketID(), pClient->GetAccount()->GetName(), (DWORD)GetUID(), GetName(false),
								(DWORD)pItem->GetUID(), pItem->GetResourceName(), pItem->GetName(),
								(DWORD)pObjCont->GetUID(), pObjCont->GetResourceName(), pObjCont->GetName());
						}
#endif

						return false;
						}
					}
				}

			return CanSee(pObjCont);
		}
	}
	else
	{
		const CChar *pChar = static_cast<const CChar*>(pObj);
		if ( !pChar )
			return false;
		if ( pChar == this )
			return true;
		if ( IsPriv(PRIV_ALLSHOW) )
			return (GetPrivLevel() < pChar->GetPrivLevel()) ? false : true;

		if ( m_pNPC && pChar->IsStatFlag(STATF_DEAD) )
		{
			if ( m_pNPC->m_Brain != NPCBRAIN_HEALER && Skill_GetBase(SKILL_SPIRITSPEAK) < 1000 )
				return false;
		}
		else if ( pChar->IsStatFlag(STATF_Invisible|STATF_Insubstantial|STATF_Hidden) )
		{
			// Characters can be invisible, but not to GM's (true sight ?)
			// equal level can see each other if they are staff members or they return 1 in @SeeHidden
			if ( pChar->GetPrivLevel() <= PLEVEL_Player )
			{
				if ( IsTrigUsed(TRIGGER_SEEHIDDEN) )
				{
					CScriptTriggerArgs Args;
					Args.m_iN1 = GetPrivLevel() <= pChar->GetPrivLevel();
					CChar *pChar2 = const_cast<CChar*>(pChar);
					CChar *this2 = const_cast<CChar*>(this);
					this2->OnTrigger(CTRIG_SeeHidden, pChar2, &Args);
					return (Args.m_iN1 != 0);
				}
				if ( GetPrivLevel() <= pChar->GetPrivLevel() )
					return false;
			}
			else
			{
				if ( GetPrivLevel() < pChar->GetPrivLevel() )
					return false;
			}
		}

		if ( IsStatFlag(STATF_DEAD) && !CanSeeAsDead(pChar) )
			return false;

		if ( pChar->IsDisconnected() )
		{
			if ( pChar->IsStatFlag(STATF_Ridden) )
			{
				CChar *pCharRider = Horse_GetMountChar();
				if ( pCharRider )
					return CanSee(pCharRider);
				return false;
			}

			if ( !IsPriv(PRIV_GM) )		// only characters with GM or ALLSHOW priv should be able to see disconnected chars
				return false;
		}
	}

	if ( IsPriv(PRIV_ALLSHOW) && (pObj->IsTopLevel() || pObj->IsDisconnected()) )		// don't exclude for logged out and diff maps
		return (GetTopPoint().GetDistSightBase(pObj->GetTopPoint()) <= UO_MAP_VIEW_SIZE);

	return true;
}

bool CChar::CanSeeLOS( const CPointMap &ptDst, CPointMap *pptBlock, int iMaxDist, WORD wFlags ) const
{
	ADDTOCALLSTACK("CChar::CanSeeLOS");
	if ( (m_pPlayer && (g_Cfg.m_iAdvancedLos & ADVANCEDLOS_PLAYER)) || (m_pNPC && (g_Cfg.m_iAdvancedLos & ADVANCEDLOS_NPC)) )
		return CanSeeLOS_New(ptDst, pptBlock, iMaxDist, wFlags);
	
	// Max distance of iMaxDist
	// Line of sight check
	// NOTE: if not blocked. pptBlock is undefined.
	// 3D LOS later - real LOS, i.e. we can't shoot through the floor, but can shoot through the hole in it

	if ( IsPriv(PRIV_GM) )
		return true;

	CPointMap ptSrc = GetTopPoint();
	int iDist = ptSrc.GetDist(ptDst);
	if ( iDist > iMaxDist )
	{
blocked:
		if ( pptBlock )
			*pptBlock = ptSrc;
		return false;	// blocked
	}

	// Walk towards the object. If any spot is too far above our heads then we can not see what's behind it.
	int iDistTry = 0;
	while ( --iDist >= 0 )
	{
		DIR_TYPE dir = ptSrc.GetDir(ptDst);
		DWORD wBlockFlags;
		if ( dir % 2 && !IsSetEF(EF_NoDiagonalCheckLOS) )	// test only diagonal dirs
		{
			CPointMap ptTest = ptSrc;
			DIR_TYPE dirTest1 = static_cast<DIR_TYPE>(dir - 1);	// get 1st ortogonal
			DIR_TYPE dirTest2 = static_cast<DIR_TYPE>(dir + 1);	// get 2nd ortogonal
			if ( dirTest2 == DIR_QTY )		// roll over
				dirTest2 = DIR_N;

			ptTest.Move(dirTest1);
			wBlockFlags = CAN_C_SWIM|CAN_C_WALK|CAN_C_FLY;
			signed char z = g_World.GetHeightPoint2(ptTest, wBlockFlags, true);
			signed char zDiff = static_cast<signed char>(abs(z - ptTest.m_z));

			if ( (zDiff > PLAYER_HEIGHT) || (wBlockFlags & (CAN_I_BLOCK|CAN_I_DOOR)) )		// blocked
			{
				ptTest = ptSrc;
				ptTest.Move(dirTest2);
				{
					wBlockFlags = CAN_C_SWIM|CAN_C_WALK|CAN_C_FLY;
					z = g_World.GetHeightPoint2(ptTest, wBlockFlags, true);
					zDiff = static_cast<signed char>(abs(z - ptTest.m_z));
					if ( zDiff > PLAYER_HEIGHT )
						goto blocked;

					if ( wBlockFlags & (CAN_I_BLOCK|CAN_I_DOOR) )
					{
						ptSrc = ptTest;
						goto blocked;
					}
				}
			}
			ptTest.m_z = z;
		}

		if ( iDist )
		{
			ptSrc.Move(dir);	// NOTE: The dir is very coarse and can change slightly.
			wBlockFlags = CAN_C_SWIM|CAN_C_WALK|CAN_C_FLY;
			signed char z = g_World.GetHeightPoint2(ptSrc, wBlockFlags, true);
			signed char zDiff = static_cast<signed char>(abs(z - ptSrc.m_z));

			if ( (zDiff > PLAYER_HEIGHT) || (wBlockFlags & (CAN_I_BLOCK|CAN_I_DOOR)) || (iDistTry > iMaxDist) )
				goto blocked;

			ptSrc.m_z = z;
			iDistTry++;
		}
	}

	if ( abs(ptSrc.m_z - ptDst.m_z) >= 20 )
		return false;
	return true;	// made it all the way to the object with no obstructions.
}

// a - gradient < x < b + gradient
#define BETWEENPOINT(coord, coordt, coords) ( (coord > ((double)minimum(coordt, coords) - 0.5)) && (coord < ((double)maximum(coordt, coords) + 0.5)) )
#define APPROX(num) ((double)((num - floor(num)) > 0.5)? ceil(num) : floor(num))
//#define CALCITEMHEIGHT(num) num + ((pItemDef->GetTFlags() & 0x400)? pItemDef->GetHeight() / 2 : pItemDef->GetHeight())
#define WARNLOS(_x_)		if ( g_Cfg.m_wDebugFlags & DEBUGF_LOS ) { g_pLog->EventWarn _x_; }

bool inline CChar::CanSeeLOS_New_Failed( CPointMap *pptBlock, CPointMap &ptNow ) const
{
	ADDTOCALLSTACK("CChar::CanSeeLOS_New_Failed");
	if ( pptBlock )
		*pptBlock = ptNow;
	return false;
}

bool CChar::CanSeeLOS_New( const CPointMap &ptDst, CPointMap *pptBlock, int iMaxDist, WORD flags ) const
{
	ADDTOCALLSTACK("CChar::CanSeeLOS_New");
	if ( IsPriv(PRIV_GM) )
	{
		WARNLOS(("GM Pass\n"));
		return true;
	}
		
	CPointMap ptSrc = GetTopPoint();
	CPointMap ptNow(ptSrc);

	if ( ptSrc.m_map != ptDst.m_map )
		return CanSeeLOS_New_Failed(pptBlock, ptNow);
	if ( ptSrc == ptDst )
		return true;

	ptSrc.m_z = minimum(ptSrc.m_z + GetHeightMount(true), UO_SIZE_Z);	//true - substract one from the height because of eyes height
	WARNLOS(("Total Z: %d\n", ptSrc.m_z));

	int dx, dy, dz;
	dx = ptDst.m_x - ptSrc.m_x;
	dy = ptDst.m_y - ptSrc.m_y;
	dz = ptDst.m_z - ptSrc.m_z;

	double dist2d, dist3d;
	dist2d = sqrt(static_cast<double>(dx*dx + dy*dy));
	if ( dz )
		dist3d = sqrt(static_cast<double>(dist2d*dist2d + dz*dz));
	else
		dist3d = dist2d;
	
	if ( APPROX(dist2d) > static_cast<double>(iMaxDist) )
	{
		WARNLOS(("( APPROX(dist2d)(%f) > ((double)iMaxDist)(%f) ) --> NOLOS\n", APPROX(dist2d), (double)iMaxDist));
		return CanSeeLOS_New_Failed(pptBlock, ptNow);
	}
	
	double dFactorX, dFactorY, dFactorZ;
	dFactorX = dx / dist3d;
	dFactorY = dy / dist3d;
	dFactorZ = dz / dist3d;
	
	double nPx, nPy, nPz;
	nPx = ptSrc.m_x;
	nPy = ptSrc.m_y;
	nPz = ptSrc.m_z;
	
	std::vector<CPointMap> path;
	for (;;)
	{
		if ( BETWEENPOINT(nPx, ptDst.m_x, ptSrc.m_x) && BETWEENPOINT(nPy, ptDst.m_y, ptSrc.m_y) && BETWEENPOINT(nPz, ptDst.m_z, ptSrc.m_z) )
		{
			dx = (int)APPROX(nPx);
			dy = (int)APPROX(nPy);
			dz = (int)APPROX(nPz);

			// Add point to vector
			if ( path.size() )
			{
				CPointMap ptEnd = path.at(path.size() - 1);
				if ( ptEnd.m_x != dx || ptEnd.m_y != dy || ptEnd.m_z != dz )
					path.push_back(CPointMap(static_cast<WORD>(dx), static_cast<WORD>(dy), static_cast<signed char>(dz), ptSrc.m_map));
			}
			else
			{
				path.push_back(CPointMap(static_cast<WORD>(dx), static_cast<WORD>(dy), static_cast<signed char>(dz), ptSrc.m_map));
			}
			WARNLOS(("PATH X:%d Y:%d Z:%d\n", dx, dy, dz));

			nPx += dFactorX;
			nPy += dFactorY;
			nPz += dFactorZ;
		}
		else
			break;
	}
	
	if ( path.size() )
	{
		if ( path.at(path.size() - 1) != ptDst )
			path.push_back(CPointMap(ptDst.m_x, ptDst.m_y, ptDst.m_z, ptDst.m_map));
	}
	else
	{
		path.clear();
		return CanSeeLOS_New_Failed(pptBlock, ptNow);
	}
	
	WARNLOS(("Path calculated %" FMTSIZE_T "\n", path.size()));
	// Ok now we should loop through all the points and checking for maptile, staticx, items, multis.
	// If something is in the way and it has the wrong flags LOS return false
	
	const CGrayMapBlock *pBlock			= NULL;		// Block of the map (for statics)
	const CUOStaticItemRec *pStatic		= NULL;		// Statics iterator (based on GrayMapBlock)
	const CGrayMulti *pMulti 			= NULL;		// Multi Def (multi check)
	const CUOMultiItemRec2 *pMultiItem	= NULL;		// Multi item iterator
	CRegionBase *pRegion				= NULL;		// Nulti regions
	CRegionLinks rlinks;							// Links to multi regions
	CItem *pItem						= NULL;
	CItemBase *pItemDef 				= NULL;
	CItemBaseDupe *pDupeDef				= NULL;

	DWORD wTFlags = 0;
	height_t Height = 0;
	WORD terrainid = 0;
	bool bPath = true;
	bool bNullTerrain = false;

	CRegionBase *pSrcRegion = ptSrc.GetRegion(REGION_TYPE_AREA|REGION_TYPE_ROOM|REGION_TYPE_MULTI);
	CRegionBase *pNowRegion = NULL;
	
	int lp_x = 0, lp_y = 0; 
	signed char min_z = 0, max_z = 0;
	
	for (size_t i = 0; i < path.size(); lp_x = ptNow.m_x, lp_y = ptNow.m_y, pItemDef = NULL, pStatic = NULL, pMulti = NULL, pMultiItem = NULL, min_z = 0, max_z = 0, ++i )
	{
		ptNow = path.at(i);
		WARNLOS(("---------------------------------------------\n"));
		WARNLOS(("Point %d,%d,%d \n", ptNow.m_x, ptNow.m_y, ptNow.m_z));

		pNowRegion = ptNow.GetRegion(REGION_TYPE_AREA|REGION_TYPE_ROOM|REGION_TYPE_MULTI);

		if ( (flags & LOS_NO_OTHER_REGION) && (pSrcRegion != pNowRegion) )
		{
			WARNLOS(("flags & 0200 and path is leaving my region - BLOCK\n"));
			bPath = false;
			break;
		}

		if ( (flags & LOS_NC_MULTI) && ptNow.GetRegion(REGION_TYPE_MULTI) && (ptNow.GetRegion(REGION_TYPE_MULTI) != ptSrc.GetRegion(REGION_TYPE_MULTI)) )
		{
			WARNLOS(("flags & 0400 and path is crossing another multi - BLOCK\n"));
			bPath = false;
			break;
		}

		if ( lp_x != ptNow.m_x || lp_y != ptNow.m_y )
		{
			WARNLOS(("\tLoading new map block.\n"));
			pBlock = g_World.GetMapBlock(ptNow);
		}

		if ( !pBlock ) // something is wrong
		{
			WARNLOS(("GetMapBlock Failed\n"));
			bPath = false;
			break;
		}

		if ( !(flags & LOS_NB_TERRAIN) )
		{
			if ( !((flags & LOS_NB_LOCAL_TERRAIN) && (pSrcRegion == pNowRegion)) )
			{
				// ------ MapX.mul Check ----------
				terrainid = pBlock->GetTerrain(UO_BLOCK_OFFSET(ptNow.m_x), UO_BLOCK_OFFSET(ptNow.m_y))->m_wTerrainIndex;
				WARNLOS(("Terrain %d\n", terrainid));

				if ( (flags & LOS_FISHING) && (ptSrc.GetDist(ptNow) >= 2) && (g_World.GetTerrainItemType(terrainid) != IT_WATER) && (g_World.GetTerrainItemType(terrainid) != IT_NORMAL) )
				{
					WARNLOS(("Terrain %d blocked - flags & LOS_FISHING, distance >= 2 and type of pItemDef is not IT_WATER\n", terrainid));
					WARNLOS(("ptSrc: %d,%d,%d; ptNow: %d,%d,%d; terrainid: %d; terrainid type: %d\n", ptSrc.m_x, ptSrc.m_y, ptSrc.m_z, ptNow.m_x, ptNow.m_y, ptNow.m_z, terrainid, g_World.GetTerrainItemType(terrainid)));
					bPath = false;
					break;
				}

				//#define MAPTILEMIN minimum(minimum(minimum(pBlock->GetTerrain(0,0)->m_z, pBlock->GetTerrain(0,1)->m_z), pBlock->GetTerrain(1,0)->m_z), pBlock->GetTerrain(1,1)->m_z)
				//#define MAPTILEMAX maximum(maximum(maximum(pBlock->GetTerrain(0,0)->m_z, pBlock->GetTerrain(0,1)->m_z), pBlock->GetTerrain(1,0)->m_z), pBlock->GetTerrain(1,1)->m_z);
				//#define MAPTILEZ pBlock->GetTerrain(UO_BLOCK_OFFSET(ptNow.m_x), UO_BLOCK_OFFSET(ptNow.m_y))->m_z;

				if ( (terrainid != TERRAIN_HOLE) && (terrainid != 475) )
				{
					if ( terrainid < 430 || terrainid > 437 )
					{
						/*this stuff should do some checking for surrounding items:
						aaa
						aXa
						aaa
						min_z is determined as a minimum of all a/X terrain, where X is ptNow
						*/
						BYTE pos_x = UO_BLOCK_OFFSET(ptNow.m_x) > 1 ? UO_BLOCK_OFFSET(ptNow.m_x - 1) : 0;
						BYTE pos_y = UO_BLOCK_OFFSET(ptNow.m_y) > 1 ? UO_BLOCK_OFFSET(ptNow.m_y - 1) : 0;
						const BYTE defx = UO_BLOCK_OFFSET(ptNow.m_x);
						const BYTE defy = UO_BLOCK_OFFSET(ptNow.m_y);
						min_z = pBlock->GetTerrain(pos_x, pos_y)->m_z;
						max_z = pBlock->GetTerrain(defx, defy)->m_z;
						for ( BYTE posy = pos_y; (abs(defx - UO_BLOCK_OFFSET(pos_x)) <= 1 && pos_x <= 7); ++pos_x )
						{
							for ( pos_y = posy; (abs(defy - UO_BLOCK_OFFSET(pos_y)) <= 1 && pos_y <= 7); ++pos_y )
								min_z = minimum(min_z, pBlock->GetTerrain(pos_x, pos_y)->m_z);
						}
						//min_z = MAPTILEZ;
						//max_z = MAPTILEZ;
						WARNLOS(("Terrain %d - m:%d M:%d\n", terrainid, min_z, max_z));
						if ( CUOMapMeter::IsTerrainNull(terrainid) )
							bNullTerrain = true; //what if there are some items on that hole?
						if ( (min_z <= ptNow.m_z && max_z >= ptNow.m_z) && (ptNow.m_x != ptDst.m_x || ptNow.m_y != ptDst.m_y || min_z > ptDst.m_z || max_z < ptDst.m_z) )
						{
							WARNLOS(("Terrain %d - m:%d M:%d - block\n", terrainid, min_z, max_z));
							bPath = false;
							break;
						}
						CGrayTerrainInfo land(terrainid);
						if ( (land.m_flags & UFLAG1_WATER) && (flags & LOS_NC_WATER) )
							bNullTerrain = true;
					}
				}
				//#undef MAPTILEMIN
				//#undef MAPTILEMAX
				//#undef MAPTILEZ
			}
		}

		// ------ StaticsX.mul Check --------
		if ( !(flags & LOS_NB_STATIC) )
		{
			if ( !((flags & LOS_NB_LOCAL_STATIC) && (pSrcRegion == pNowRegion)) )
			{
				for ( size_t s = 0; s < pBlock->m_Statics.GetStaticQty(); pStatic = NULL, pItemDef = NULL, ++s )
				{
					pStatic = pBlock->m_Statics.GetStatic(s);
					if ( pStatic->m_x + pBlock->m_x != ptNow.m_x || pStatic->m_y + pBlock->m_y != ptNow.m_y )
						continue;

					//Fix for Stacked items blocking view
					if ( (pStatic->m_x == ptDst.m_x) && (pStatic->m_y == ptDst.m_y) && (pStatic->m_z >= GetTopZ()) && (pStatic->m_z <= ptSrc.m_z) )
						continue;

					pItemDef = CItemBase::FindItemBase(pStatic->GetDispID());
					wTFlags = 0;
					Height = 0;
					bNullTerrain = false;

					if ( !pItemDef )
					{
						WARNLOS(("STATIC - Cannot get pItemDef for item (0%x)\n", pStatic->GetDispID()));
					}
					else
					{
						if ( (flags & LOS_FISHING) && (ptSrc.GetDist(ptNow) >= 2) && (pItemDef->GetType() != IT_WATER) && (pItemDef->Can(CAN_I_DOOR | CAN_I_PLATFORM | CAN_I_BLOCK | CAN_I_CLIMB | CAN_I_FIRE | CAN_I_ROOF) || (pItemDef->m_Can & CAN_I_BLOCKLOS)) )
						{
							WARNLOS(("pStatic blocked - flags & 0800, distance >= 2 and type of pItemDef is not IT_WATER\n"));
							bPath = false;
							break;
						}

						wTFlags = pItemDef->GetTFlags();
						Height = pItemDef->GetHeight();

						if ( pItemDef->GetID() != pStatic->GetDispID() ) //not a parent item
						{
							WARNLOS(("Not a parent item (STATIC)\n"));
							pDupeDef = CItemBaseDupe::GetDupeRef(static_cast<ITEMID_TYPE>(pStatic->GetDispID()));
							if ( !pDupeDef )
							{
								g_Log.EventDebug("Failed to get non-parent reference (static) (DispID 0%x) (X: %d Y: %d Z: %d)\n", pStatic->GetDispID(), ptNow.m_x, ptNow.m_y, pStatic->m_z);
								wTFlags = pItemDef->GetTFlags();
								Height = pItemDef->GetHeight();
							}
							else
							{
								wTFlags = pDupeDef->GetTFlags();
								Height = pDupeDef->GetHeight();
							}
						}
						else
						{
							WARNLOS(("Parent item (STATIC)\n"));
						}

						Height = (wTFlags & UFLAG2_CLIMBABLE) ? Height / 2 : Height;

						if ( ((wTFlags & (UFLAG1_WALL|UFLAG1_BLOCK|UFLAG2_PLATFORM)) || (pItemDef->m_Can & CAN_I_BLOCKLOS)) && !((wTFlags & UFLAG2_WINDOW) && (flags & LOS_NB_WINDOWS)) )
						{
							WARNLOS(("pStatic %0x %d,%d,%d - %d\n", pStatic->GetDispID(), pStatic->m_x, pStatic->m_y, pStatic->m_z, Height));
							min_z = pStatic->m_z;
							max_z = minimum(Height + min_z, UO_SIZE_Z);
							WARNLOS(("wTFlags(0%lx)\n", wTFlags));

							WARNLOS(("pStatic %0x Z check: %d,%d (Now: %d) (Dest: %d).\n", pStatic->GetDispID(), min_z, max_z, ptNow.m_z, ptDst.m_z));
							if ( min_z <= ptNow.m_z && max_z >= ptNow.m_z )
							{
								if ( ptNow.m_x != ptDst.m_x || ptNow.m_y != ptDst.m_y || min_z > ptDst.m_z || max_z < ptDst.m_z )
								{
									WARNLOS(("pStatic blocked - m:%d M:%d\n", min_z, max_z));
									bPath = false;
									break;
								}
							}
						}
					}
				}
			}
		}
		
		if ( !bPath )
			break;
		
		// --------- In game items ----------
		if ( !(flags & LOS_NB_DYNAMIC) )
		{
			if ( !((flags & LOS_NB_LOCAL_DYNAMIC) && (pSrcRegion == pNowRegion)) )
			{
				CWorldSearch AreaItems(ptNow, 0);
				for (;;)
				{
					pItem = AreaItems.GetItem();
					if ( !pItem )
						break;
					if ( pItem->GetTopPoint().m_x != ptNow.m_x || pItem->GetTopPoint().m_y != ptNow.m_y )
						continue;
					if ( !CanSeeItem(pItem) )
						continue;

					//Fix for Stacked items blocking view
					if ( (pItem->GetTopPoint().m_x == ptDst.m_x) && (pItem->GetTopPoint().m_y == ptDst.m_y) && (pItem->GetTopPoint().m_z >= GetTopZ()) && (pItem->GetTopPoint().m_z <= ptSrc.m_z) )
						continue;

					pItemDef = CItemBase::FindItemBase(pItem->GetDispID());
					wTFlags = 0;
					Height = 0;
					bNullTerrain = false;

					if ( !pItemDef )
					{
						WARNLOS(("DYNAMIC - Cannot get pItemDef for item (0%x)\n", pItem->GetDispID()));
					}
					else
					{
						if ( (flags & LOS_FISHING) && (ptSrc.GetDist(ptNow) >= 2) && (pItemDef->GetType() != IT_WATER) && (pItemDef->Can(CAN_I_DOOR | CAN_I_PLATFORM | CAN_I_BLOCK | CAN_I_CLIMB | CAN_I_FIRE | CAN_I_ROOF) || (pItemDef->m_Can & CAN_I_BLOCKLOS)) )
						{
							WARNLOS(("pItem blocked - flags & 0800, distance >= 2 and type of pItemDef is not IT_WATER\n"));
							bPath = false;
							break;
							//return CanSeeLOS_New_Failed(pptBlock, ptNow);
						}

						wTFlags = pItemDef->GetTFlags();
						Height = pItemDef->GetHeight();

						if ( pItemDef->GetID() != pItem->GetDispID() )	//not a parent item
						{
							WARNLOS(("Not a parent item (DYNAMIC)\n"));
							pDupeDef = CItemBaseDupe::GetDupeRef(static_cast<ITEMID_TYPE>(pItem->GetDispID()));
							if ( !pDupeDef )
							{
								g_Log.EventDebug("Failed to get non-parent reference (dynamic) (DispID 0%x) (X: %d Y: %d Z: %d)\n", pItem->GetDispID(), ptNow.m_x, ptNow.m_y, pItem->GetTopZ());
								wTFlags = pItemDef->GetTFlags();
								Height = pItemDef->GetHeight();
							}
							else
							{
								wTFlags = pDupeDef->GetTFlags();
								Height = pDupeDef->GetHeight();
							}
						}
						else
						{
							WARNLOS(("Parent item (DYNAMIC)\n"));
						}

						Height = (wTFlags & UFLAG2_CLIMBABLE) ? Height / 2 : Height;

						if ( ((wTFlags & (UFLAG1_WALL|UFLAG1_BLOCK|UFLAG2_PLATFORM)) || pItemDef->m_Can & CAN_I_BLOCKLOS) && !((wTFlags & UFLAG2_WINDOW) && (flags & LOS_NB_WINDOWS)) )
						{
							WARNLOS(("pItem %0lx(%0x) %d,%d,%d - %d\n", (DWORD)pItem->GetUID(), pItem->GetDispID(), pItem->GetTopPoint().m_x, pItem->GetTopPoint().m_y, pItem->GetTopPoint().m_z, Height));
							min_z = pItem->GetTopZ();
							max_z = minimum(Height + min_z, UO_SIZE_Z);
							WARNLOS(("wTFlags(0%lx)\n", wTFlags));

							WARNLOS(("pItem %0lx(%0x) Z check: %d,%d (Now: %d) (Dest: %d).\n", (DWORD)pItem->GetUID(), pItem->GetDispID(), min_z, max_z, ptNow.m_z, ptDst.m_z));
							if ( min_z <= ptNow.m_z && max_z >= ptNow.m_z )
							{
								if ( ptNow.m_x != ptDst.m_x || ptNow.m_y != ptDst.m_y || min_z > ptDst.m_z || max_z < ptDst.m_z )
								{
									WARNLOS(("pItem blocked - m:%d M:%d\n", min_z, max_z));
									bPath = false;
									break;
								}
							}
						}
					}
				}
			}
		}

		if ( !bPath )
			break;
		
		// ----------- Multis ---------------
		
		if ( !(flags & LOS_NB_MULTI) )
		{
			if ( !((flags & LOS_NB_LOCAL_MULTI) && (pSrcRegion == pNowRegion)) )
			{
				size_t iQtyr = ptNow.GetRegions(REGION_TYPE_MULTI, rlinks);
				if ( iQtyr > 0 )
				{
					for ( size_t ii = 0; ii < iQtyr; pMulti = NULL, ++ii, pItem = NULL, pRegion = NULL )
					{
						pRegion = rlinks.GetAt(ii);
						if ( pRegion )
							pItem = pRegion->GetResourceID().ItemFind();

						if ( !pItem )
							continue;

						pMulti = g_Cfg.GetMultiItemDefs(pItem);
						if ( !pMulti )
							continue;

						size_t iQty = pMulti->GetItemCount();
						for ( size_t iii = 0; iii < iQty; pItemDef = NULL, pMultiItem = NULL, ++iii )
						{
							pMultiItem = pMulti->GetItem(iii);
							if ( !pMultiItem )
								break;
							if ( !pMultiItem->m_visible )
								continue;
							if ( (pMultiItem->m_dx + pItem->GetTopPoint().m_x != ptNow.m_x) || (pMultiItem->m_dy + pItem->GetTopPoint().m_y != ptNow.m_y) )
								continue;

							pItemDef = CItemBase::FindItemBase(pMultiItem->GetDispID());
							wTFlags = 0;
							Height = 0;
							bNullTerrain = false;

							if ( !pItemDef )
							{
								WARNLOS(("MULTI - Cannot get pItemDef for item (0%x)\n", pMultiItem->GetDispID()));
							}
							else
							{
								if ( (flags & LOS_FISHING) && (ptSrc.GetDist(ptNow) >= 2) && (pItemDef->GetType() != IT_WATER) && (pItemDef->Can(CAN_I_DOOR | CAN_I_PLATFORM | CAN_I_BLOCK | CAN_I_CLIMB | CAN_I_FIRE | CAN_I_ROOF) || (pItemDef->m_Can & CAN_I_BLOCKLOS)) )
								{
									WARNLOS(("pMultiItem blocked - flags & 0800, distance >= 2 and type of pItemDef is not IT_WATER\n"));
									bPath = false;
									break;
									//return CanSeeLOS_New_Failed(pptBlock, ptNow);
								}

								wTFlags = pItemDef->GetTFlags();
								Height = pItemDef->GetHeight();

								if ( pItemDef->GetID() != pMultiItem->GetDispID() ) //not a parent item
								{
									WARNLOS(("Not a parent item (MULTI)\n"));
									pDupeDef = CItemBaseDupe::GetDupeRef(static_cast<ITEMID_TYPE>(pMultiItem->GetDispID()));
									if ( !pDupeDef )
									{
										g_Log.EventDebug("Failed to get non-parent reference (multi) (DispID 0%x) (X: %d Y: %d Z: %d)\n", pMultiItem->GetDispID(), ptNow.m_x, ptNow.m_y, pMultiItem->m_dz + pItem->GetTopPoint().m_z);
										wTFlags = pItemDef->GetTFlags();
										Height = pItemDef->GetHeight();
									}
									else
									{
										wTFlags = pDupeDef->GetTFlags();
										Height = pDupeDef->GetHeight();
									}
								}
								else
								{
									WARNLOS(("Parent item (MULTI)\n"));
								}

								Height = (wTFlags & UFLAG2_CLIMBABLE) ? Height / 2 : Height;

								if ( ((wTFlags & (UFLAG1_WALL|UFLAG1_BLOCK|UFLAG2_PLATFORM)) || (pItemDef->m_Can & CAN_I_BLOCKLOS)) && !((wTFlags & UFLAG2_WINDOW) && (flags & LOS_NB_WINDOWS)) )
								{
									WARNLOS(("pMultiItem %0x %d,%d,%d - %d\n", pMultiItem->GetDispID(), pMultiItem->m_dx, pMultiItem->m_dy, pMultiItem->m_dz, Height));
									min_z = static_cast<signed char>(pMultiItem->m_dz) + pItem->GetTopPoint().m_z;
									max_z = minimum(Height + min_z, UO_SIZE_Z);
									WARNLOS(("wTFlags(0%lx)\n", wTFlags));

									if ( min_z <= ptNow.m_z && max_z >= ptNow.m_z )
									{
										if ( ptNow.m_x != ptDst.m_x || ptNow.m_y != ptDst.m_y || min_z > ptDst.m_z || max_z < ptDst.m_z )
										{
											WARNLOS(("pMultiItem blocked - m:%d M:%d\n", min_z, max_z));
											bPath = false;
											break;
										}
									}
								}
							}
						}

						if ( !bPath )
							break;
					}
				}
			}
		}

		if ( bNullTerrain )
			bPath = false;

		if ( !bPath )
			break;
	}

	path.clear();

	if ( !bPath )
		return CanSeeLOS_New_Failed(pptBlock, ptNow);

	return true;
}

#undef WARNLOS
#undef APPROX
#undef BETWEENPOINT
//#undef CALCITEMHEIGHT

bool CChar::CanSeeLOS( const CObjBaseTemplate *pObj, WORD wFlags ) const
{
	ADDTOCALLSTACK("CChar::CanSeeLOS");

	if ( !CanSee(pObj) )
		return false;

	pObj = pObj->GetTopLevelObj();
	if ( !pObj )
		return false;

	if ( (m_pPlayer && (g_Cfg.m_iAdvancedLos & ADVANCEDLOS_PLAYER)) || (m_pNPC && (g_Cfg.m_iAdvancedLos & ADVANCEDLOS_NPC)) )
	{
		CPointMap pt = pObj->GetTopPoint();
		const CChar *pChar = dynamic_cast<const CChar*>(pObj);
		if ( pChar )
			pt.m_z = minimum(pt.m_z + pChar->GetHeightMount(true), UO_SIZE_Z);
		return CanSeeLOS_New(pt, NULL, UO_MAP_VIEW_SIZE, wFlags);
	}
	else
		return CanSeeLOS(pObj->GetTopPoint(), NULL, UO_MAP_VIEW_SIZE, wFlags);
}

bool CChar::CanTouch( const CPointMap &pt ) const
{
	ADDTOCALLSTACK("CChar::CanTouch");
	// Can I reach this from where i am.
	// swords, spears, arms length = x units.
	// Use or Grab.
	// Check for blocking objects.
	// It this in a container we can't get to ?

	return CanSeeLOS(pt, NULL, 6);
}

bool CChar::CanTouch( const CObjBase *pObj ) const
{
	ADDTOCALLSTACK("CChar::CanTouch");
	// Can I reach this from where i am. swords, spears, arms length = x units.
	// Use or Grab. May be in snooped container.
	// Check for blocking objects. Is this in a container we can't get to ?

	if ( !pObj )
		return false;

	const CItem *pItem = NULL;
	const CObjBaseTemplate *pObjTop = pObj->GetTopLevelObj();
	int iDist = GetTopDist3D(pObjTop);

	if ( pObj->IsItem() )	// some objects can be used anytime. (even by the dead.)
	{
		pItem = static_cast<const CItem *>(pObj);
		if ( !pItem )
			return false;

		bool bDeathImmune = IsPriv(PRIV_GM);
		switch ( pItem->GetType() )
		{
			case IT_SIGN_GUMP:	// can be seen from a distance.
				return (iDist <= UO_MAP_VIEW_SIGHT);

			case IT_SHRINE:		// We can use shrines when dead !!
			case IT_TELESCOPE:
				bDeathImmune = true;
				break;

			case IT_SHIP_PLANK:
			case IT_SHIP_SIDE:
			case IT_SHIP_SIDE_LOCKED:
			case IT_ROPE:
				if ( IsStatFlag(STATF_Sleeping|STATF_Freeze|STATF_Stone) )
					break;
				return (iDist <= g_Cfg.m_iMaxShipPlankTeleport);

			default:
				break;
		}

		if ( !bDeathImmune && IsStatFlag(STATF_DEAD|STATF_Sleeping|STATF_Freeze|STATF_Stone) )
			return false;
	}

	//	search up to top level object
	const CChar *pChar = NULL;
	if ( pObjTop && (pObjTop != this) )
	{
		if ( pObjTop->IsChar() )
		{
			pChar = dynamic_cast<const CChar *>(pObjTop);
			if ( !pChar )
				return false;
			if ( pChar == this )
				return true;
			if ( IsPriv(PRIV_GM) )
				return (GetPrivLevel() >= pChar->GetPrivLevel());
			if ( pChar->IsStatFlag(STATF_DEAD|STATF_Stone) )
				return false;
		}

		CObjBase *pObjCont = NULL;
		const CObjBase *pObjTest = pObj;
		for (;;)
		{
			pItem = static_cast<const CItem *>(pObjTest);
			if ( !pItem )
				break;

			// What is this inside of ?
			pObjCont = pItem->GetParentObj();
			if ( !pObjCont )
				break;

			pObjTest = pObjCont;
			if ( !CanSeeInContainer(static_cast<const CItemContainer *>(pObjTest)) )
				return false;
		}
	}

	if ( IsPriv(PRIV_GM) )
		return true;

	if ( !CanSeeLOS(pObjTop) )
	{
		if ( GetAbilityFlags() & CAN_C_DCIGNORELOS )
			return true;
		else if ( pChar && (pChar->GetAbilityFlags() & CAN_C_DCIGNORELOS) )
			return true;
		else if ( pItem && (pItem->GetAbilityFlags() & CAN_I_DCIGNORELOS) )
			return true;
		else
			return false;
	}
	if ( iDist > 3 )
	{
		if ( GetAbilityFlags() & CAN_C_DCIGNOREDIST )
			return true;
		else if ( pChar && (pChar->GetAbilityFlags() & CAN_C_DCIGNOREDIST) )
			return true;
		else if ( pItem && (pItem->GetAbilityFlags() & CAN_I_DCIGNOREDIST) )
			return true;
		else
			return false;
	}
	return true;
}

IT_TYPE CChar::CanTouchStatic( CPointMap &pt, ITEMID_TYPE id, CItem *pItem )
{
	ADDTOCALLSTACK("CChar::CanTouchStatic");
	// Might be a dynamic or a static.
	// RETURN:
	//  IT_JUNK = too far away.
	//  set pt to the top level point.

	if ( pItem )
	{
		if ( !CanTouch(pItem) )
			return IT_JUNK;
		pt = GetTopLevelObj()->GetTopPoint();
		return pItem->GetType();
	}

	// It's a static !
	CItemBase *pItemDef = CItemBase::FindItemBase(id);
	if ( !pItemDef )
		return IT_NORMAL;
	if ( !CanTouch(pt) )
		return IT_JUNK;

	// Is this static really here ?
	const CGrayMapBlock *pMapBlock = g_World.GetMapBlock(pt);
	if ( !pMapBlock )
		return IT_JUNK;

	int x2 = pMapBlock->GetOffsetX(pt.m_x);
	int y2 = pMapBlock->GetOffsetY(pt.m_y);

	size_t iQty = pMapBlock->m_Statics.GetStaticQty();
	for ( size_t i = 0; i < iQty; ++i )
	{
		if ( !pMapBlock->m_Statics.IsStaticPoint(i, x2, y2) )
			continue;
		const CUOStaticItemRec *pStatic = pMapBlock->m_Statics.GetStatic(i);
		if ( id == pStatic->GetDispID() )
			return pItemDef->GetType();
	}

	return IT_NORMAL;
}

bool CChar::CanHear( const CObjBaseTemplate *pSrc, TALKMODE_TYPE mode ) const
{
	ADDTOCALLSTACK("CChar::CanHear");
	// can we hear text or sound. (not necessarily understand it (ghost))
	// Can't hear TALKMODE_SAY through house walls.
	// NOTE: Assume pClient->CanHear() has already been called. (if it matters)

	if ( !pSrc )	// must be broadcast I guess
		return true;

	bool fThrough = false;
	int iHearRange = 0;
	switch ( mode )
	{
		case TALKMODE_YELL:
			if ( g_Cfg.m_iDistanceYell < 0 )
				return true;
			iHearRange = g_Cfg.m_iDistanceYell;
			fThrough = true;
			break;
		case TALKMODE_BROADCAST:
			return true;
		case TALKMODE_WHISPER:
			if ( g_Cfg.m_iDistanceWhisper < 0 )
				return true;
			iHearRange = g_Cfg.m_iDistanceWhisper;
			break;
		default:
			if ( g_Cfg.m_iDistanceTalk < 0 )
				return true;
			iHearRange = g_Cfg.m_iDistanceTalk;
			break;
	}

	pSrc = pSrc->GetTopLevelObj();
	int iDist = GetTopDist3D(pSrc);
	if ( iDist > iHearRange )	// too far away
		return false;
	if ( IsSetOF(OF_NoHouseMuteSpeech) )
		return true;
	if ( IsPriv(PRIV_GM) )
		return true;
	if ( fThrough )		// a yell goes through walls..
		return true;

	// sound can be blocked if in house.
	CRegionWorld *pSrcRegion;
	if ( pSrc->IsChar() )
	{
		const CChar *pCharSrc = dynamic_cast<const CChar*>(pSrc);
		ASSERT(pCharSrc);
		pSrcRegion = pCharSrc->GetRegion();
		if ( pCharSrc->IsPriv(PRIV_GM) )
			return true;
	}
	else
		pSrcRegion = dynamic_cast<CRegionWorld *>(pSrc->GetTopPoint().GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA));

	if ( m_pArea == pSrcRegion )	// same region is always ok.
		return true;
	if ( !pSrcRegion || !m_pArea ) // should not happen really.
		return false;

	bool fCanSpeech = false;
	CVarDefCont *pValue = pSrcRegion->GetResourceID().IsItem() ? pSrcRegion->GetResourceID().ItemFind()->GetKey("NOMUTESPEECH", false) : NULL;
	if ( pValue && pValue->GetValNum() > 0 )
		fCanSpeech = true;
	if ( pSrcRegion->GetResourceID().IsItem() && !pSrcRegion->IsFlag(REGION_FLAG_SHIP) && !fCanSpeech )
		return false;

	pValue = m_pArea->GetResourceID().IsItem() ? m_pArea->GetResourceID().ItemFind()->GetKey("NOMUTESPEECH", false) : NULL;
	if ( pValue && pValue->GetValNum() > 0 )
		fCanSpeech = true;
	if ( m_pArea->GetResourceID().IsItem() && !m_pArea->IsFlag(REGION_FLAG_SHIP) && !fCanSpeech )
		return false;

	return true;
}

bool CChar::CanMove( CItem *pItem, bool fMsg ) const
{
	ADDTOCALLSTACK("CChar::CanMove");
	// Is it possible that i could move this ?
	// NOT: test if need to steal. IsTakeCrime()
	// NOT: test if close enough. CanTouch()

	if ( IsPriv(PRIV_ALLMOVE|PRIV_DEBUG|PRIV_GM) )
		return true;
	if ( !pItem )
		return false;
	if ( pItem->IsAttr(ATTR_MOVE_NEVER|ATTR_LOCKEDDOWN) && !pItem->IsAttr(ATTR_MOVE_ALWAYS) )
		return false;

	if ( IsStatFlag(STATF_Stone|STATF_Freeze|STATF_Insubstantial|STATF_DEAD|STATF_Sleeping) )
	{
		if ( fMsg )
			SysMessageDefault(DEFMSG_CANTMOVE_DEAD);
		return false;
	}

	if ( pItem->IsTopLevel() )
	{
		if ( pItem->IsTopLevelMultiLocked() )
		{
			if ( fMsg )
				SysMessageDefault(DEFMSG_CANTMOVE_MULTI);

			return false;
		}
	}
	else	// equipped or in a container.
	{
		// Can't move items from the trade window (client limitation)
		if ( pItem->GetParentObj()->IsContainer() )
		{
			CItemContainer *pItemCont = dynamic_cast<CItemContainer *>(pItem->GetParentObj());
			if ( pItemCont && pItemCont->IsItemInTrade() )
			{
				SysMessageDefault(DEFMSG_MSG_TRADE_CANTMOVE);
				return false;
			}
		}

		// Can't move equipped cursed items
		if ( pItem->IsAttr(ATTR_CURSED|ATTR_CURSED2) && pItem->IsItemEquipped() )
		{
			pItem->SetAttr(ATTR_IDENTIFIED);
			if ( fMsg )
				SysMessagef("%s %s", pItem->GetName(), g_Cfg.GetDefaultMsg(DEFMSG_CANTMOVE_CURSED));

			return false;
		}

		// Can't steal/move newbie items on another cchar. (even if pet)
		if ( pItem->IsAttr(ATTR_NEWBIE|ATTR_BLESSED2|ATTR_CURSED|ATTR_CURSED2) )
		{
			const CObjBaseTemplate *pObjTop = pItem->GetTopLevelObj();
			if ( pObjTop->IsItem() )	// is this a corpse or sleeping person ?
			{
				const CItemCorpse *pCorpse = dynamic_cast<const CItemCorpse *>(pObjTop);
				if ( pCorpse )
				{
					CChar *pChar = pCorpse->IsCorpseSleeping();
					if ( pChar && pChar != this )
						return false;
				}
			}
			else if ( pObjTop->IsChar() && pObjTop != this )
			{
				if (( pItem->IsAttr(ATTR_NEWBIE) ) && ( g_Cfg.m_bAllowNewbTransfer ))
				{
					CChar *pPet = dynamic_cast<CChar*>( pItem->GetTopLevelObj());
					if (pPet->NPC_PetGetOwner() == this)
						return true;
				}
				if ( !pItem->IsItemEquipped() || pItem->GetEquipLayer() != LAYER_DRAGGING )
					return false;
			}
		}

		if ( pItem->IsItemEquipped() )
		{
			LAYER_TYPE layer = pItem->GetEquipLayer();
			switch ( layer )
			{
				case LAYER_DRAGGING:
					return true;
				case LAYER_HAIR:
				case LAYER_BEARD:
				case LAYER_PACK:
					if ( !IsPriv(PRIV_GM) )
						return false;
					break;
				default:
					if ( !CItemBase::IsVisibleLayer(layer) && ! IsPriv(PRIV_GM) )
						return false;
			}
		}
	}

	return pItem->IsMovable();
}

bool CChar::IsTakeCrime( const CItem *pItem, CChar ** ppCharMark ) const
{
	ADDTOCALLSTACK("CChar::IsTakeCrime");

	if ( IsPriv(PRIV_GM | PRIV_ALLMOVE) )
		return false;

	CObjBaseTemplate *pObjTop = pItem->GetTopLevelObj();
	CChar *pCharMark = dynamic_cast<CChar *>(pObjTop);
	if ( ppCharMark )
		*ppCharMark = pCharMark;

	if ( static_cast<const CChar*>(pObjTop) == this )
		return false;	// this is yours

	if ( pCharMark == NULL )	// In some (or is) container.
	{
		if ( pItem->IsAttr(ATTR_OWNED) && pItem->m_uidLink != GetUID() )
			return true;

		CItemContainer *pCont = dynamic_cast<CItemContainer *>(pObjTop);
		if ( pCont )
		{
			if ( pCont->IsAttr(ATTR_OWNED) )
				return true;
		}

		const CItemCorpse *pCorpseItem = dynamic_cast<const CItemCorpse *>(pObjTop);
		if ( pCorpseItem )		// taking stuff off someones corpse can be a crime
			return const_cast<CChar*>(this)->CheckCorpseCrime(pCorpseItem, true, true);

		return false;	// I guess it's not a crime
	}

	if ( pCharMark->NPC_IsOwnedBy(this) || pCharMark->Memory_FindObjTypes(this, MEMORY_FRIEND) )	// he let's you
		return false;

	// Pack animal has no owner ?
	if ( (pCharMark->GetNPCBrain(false) == NPCBRAIN_ANIMAL) && !pCharMark->IsStatFlag(STATF_Pet) )
		return false;

	return true;
}

bool CChar::CanUse( CItem *pItem, bool fMoveOrConsume ) const
{
	ADDTOCALLSTACK("CChar::CanUse");
	// Can the Char use ( CONSUME )  the item where it is ?
	// NOTE: Although this implies that we pick it up and use it.

	if ( !CanTouch(pItem) )
		return false;

	if ( fMoveOrConsume )
	{
		if ( !CanMove(pItem) )
			return false;	// ? using does not imply moving in all cases ! such as reading ?
		if ( IsTakeCrime(pItem) )
			return false;
	}
	else
	{
		// Just snooping i guess.
		if ( pItem->IsTopLevel() )
			return true;
		// The item is on another character ?
		const CObjBaseTemplate *pObjTop = pItem->GetTopLevelObj();
		if ( pObjTop != (static_cast<const CObjBaseTemplate*>(this)) )
		{
			if ( IsPriv(PRIV_GM) || pItem->IsType(IT_CONTAINER) || pItem->IsType(IT_BOOK) )
				return true;
			return false;
		}
	}

	return true;
}

bool CChar::IsMountCapable() const
{
	ADDTOCALLSTACK("CChar::IsMountCapable");
	// Is the character capable of mounting rides?
	// RETURN:
	//  false = incapable of riding

	if ( IsStatFlag(STATF_DEAD) )
		return false;
	if ( IsHuman() || IsElf() || (GetAbilityFlags() & CAN_C_MOUNT) )
		return true;

	return false;
}

bool CChar::IsVerticalSpace( CPointMap ptDest, bool fForceMount )
{
	ADDTOCALLSTACK("CChar::IsVerticalSpace");
	if ( IsPriv(PRIV_GM | PRIV_ALLMOVE) || !ptDest.IsValidPoint() )
		return true;

	WORD wBlockFlags = static_cast<WORD>(GetMoveBlockFlags());
	if ( wBlockFlags & CAN_C_WALK )
		wBlockFlags |= CAN_I_CLIMB;

	height_t iCharHeight = GetHeight();
	CGrayMapBlockState block(wBlockFlags, ptDest.m_z, ptDest.m_z + m_zClimbHeight + iCharHeight, ptDest.m_z + m_zClimbHeight + 2, iCharHeight);
	g_World.GetHeightPoint(ptDest, block, true);

	if ( iCharHeight + ptDest.m_z + (fForceMount ? 4 : 0) >= block.m_Top.m_z )		// 4 is the mount height
		return false;
	return true;
}

CRegionBase *CChar::CheckValidMove( CPointBase &ptDest, WORD *pwBlockFlags, DIR_TYPE dir, height_t *pClimbHeight, bool fPathFinding ) const
{
	ADDTOCALLSTACK("CChar::CheckValidMove");
	// Is it ok to move here ? is it blocked ?
	// ignore other characters for now.
	// RETURN:
	//  The new region we may be in.
	//  Fill in the proper ptDest.m_z value for this location. (if walking)
	//  pwBlockFlags = what is blocking me. (can be null = don't care)

	//	test diagonal dirs by two others *only* when already having a normal location
	if ( GetTopPoint().IsValidPoint() && !fPathFinding && (dir % 2) )
	{
		CPointMap ptTest;
		DIR_TYPE dirTest1 = static_cast<DIR_TYPE>(dir - 1); // get 1st ortogonal
		DIR_TYPE dirTest2 = static_cast<DIR_TYPE>(dir + 1); // get 2nd ortogonal
		if ( dirTest2 == DIR_QTY )		// roll over
			dirTest2 = DIR_N;

		ptTest = GetTopPoint();
		ptTest.Move(dirTest1);
		if ( !CheckValidMove(ptTest, pwBlockFlags, DIR_QTY, pClimbHeight) )
			return NULL;

		ptTest = GetTopPoint();
		ptTest.Move(dirTest2);
		if ( !CheckValidMove(ptTest, pwBlockFlags, DIR_QTY, pClimbHeight) )
			return NULL;
	}

	CRegionBase *pArea = ptDest.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA|REGION_TYPE_ROOM);
	if ( !pArea )
	{
		WARNWALK(("Failed to get region\n"));
		return NULL;
	}

	WORD wCan = static_cast<WORD>(GetMoveBlockFlags());
	WARNWALK(("GetMoveBlockFlags() (0x%x)\n",wCan));
	if ( !(wCan & (CAN_C_SWIM|CAN_C_WALK|CAN_C_FLY|CAN_C_RUN|CAN_C_HOVER)) )
		return NULL;	// cannot move at all, so WTF?

	WORD wBlockFlags = wCan;
	if ( wCan & CAN_C_WALK )
	{
		wBlockFlags |= CAN_I_CLIMB;		// if we can walk than we can climb. Ignore CAN_C_FLY at all here
		WARNWALK(("wBlockFlags (0%x) wCan(0%x)\n", wBlockFlags, wCan));
	}

	height_t iCharHeight = GetHeight();
	if ( m_pPlayer )	// always consider mount height on players
		iCharHeight += 4;

	CGrayMapBlockState block(wBlockFlags, ptDest.m_z, ptDest.m_z + m_zClimbHeight + iCharHeight, ptDest.m_z + m_zClimbHeight + 3, iCharHeight);
	WARNWALK(("\t\tCGrayMapBlockState block( 0%x, %d, %d, %d );ptDest.m_z(%d) m_zClimbHeight(%d)\n", wBlockFlags, ptDest.m_z, ptDest.m_z + m_zClimbHeight + iCharHeight, ptDest.m_z + m_zClimbHeight + 2, ptDest.m_z, m_zClimbHeight));

	if ( !ptDest.IsValidPoint() )
	{
		DEBUG_ERR(("Character 0%lx on %d,%d,%d wants to move into an invalid location %d,%d,%d.\n", GetUID().GetObjUID(), GetTopPoint().m_x, GetTopPoint().m_y, GetTopPoint().m_z, ptDest.m_x, ptDest.m_y, ptDest.m_z));
		return NULL;
	}
	g_World.GetHeightPoint(ptDest, block, true);

	// Pass along my results.
	wBlockFlags = static_cast<WORD>(block.m_Bottom.m_dwBlockFlags);

	if ( block.m_Top.m_dwBlockFlags )
	{
		wBlockFlags |= CAN_I_ROOF;	// we are covered by something.

		WARNWALK(("block.m_Top.m_z (%d) > ptDest.m_z (%d) + m_zClimbHeight (%d) + (block.m_Top.m_dwTile (0x%lx) > TERRAIN_QTY ? PLAYER_HEIGHT : PLAYER_HEIGHT/2 )(%d)\n", block.m_Top.m_z, ptDest.m_z, m_zClimbHeight, block.m_Top.m_dwTile, ptDest.m_z - (m_zClimbHeight + (block.m_Top.m_dwTile > TERRAIN_QTY ? PLAYER_HEIGHT : PLAYER_HEIGHT / 2))));
		if ( block.m_Top.m_z < block.m_Bottom.m_z + (m_zClimbHeight + (block.m_Top.m_dwTile > TERRAIN_QTY ? iCharHeight : iCharHeight / 2)) )
			wBlockFlags |= CAN_I_BLOCK;		// we can't fit under this!
	}

	if ( (wCan != 0xFFFF) && (wBlockFlags != 0x0) )
	{
		WARNWALK(("BOTTOMitemID (0%lx) TOPitemID (0%lx)\n", (block.m_Bottom.m_dwTile - TERRAIN_QTY), (block.m_Top.m_dwTile - TERRAIN_QTY)));
		CCharBase *pCharDef = Char_GetDef();
		ASSERT(pCharDef);

		if ( (wBlockFlags & CAN_I_DOOR) && !pCharDef->Can(CAN_C_GHOST) )
			wBlockFlags |= CAN_I_BLOCK;
		else if ( (wBlockFlags & CAN_I_ROOF) && !pCharDef->Can(CAN_C_INDOORS) )
			wBlockFlags |= CAN_I_BLOCK;
		else if ( (wBlockFlags & CAN_I_WATER) && !pCharDef->Can(CAN_C_SWIM) )
			wBlockFlags |= CAN_I_BLOCK;
		else if ( (wBlockFlags & CAN_I_HOVER) && !pCharDef->Can(CAN_C_HOVER) && !IsStatFlag(STATF_Hovering) )
			wBlockFlags |= CAN_I_BLOCK;

		// If anything blocks us it should not be overridden by this.
		if ( (wBlockFlags & CAN_I_DOOR) && pCharDef->Can(CAN_C_GHOST) )
			wBlockFlags &= ~CAN_I_BLOCK;
		else if ( (wBlockFlags & CAN_I_ROOF) && pCharDef->Can(CAN_C_INDOORS) )
			wBlockFlags &= ~CAN_I_BLOCK;
		else if ( (wBlockFlags & CAN_I_WATER) && pCharDef->Can(CAN_C_SWIM) )
			wBlockFlags &= ~CAN_I_BLOCK;
		else if ( (wBlockFlags & CAN_I_PLATFORM) && pCharDef->Can(CAN_C_WALK) )
			wBlockFlags &= ~CAN_I_BLOCK;
		else if ( (wBlockFlags & CAN_I_HOVER) && (pCharDef->Can(CAN_C_HOVER) || IsStatFlag(STATF_Hovering)) )
			wBlockFlags &= ~CAN_I_BLOCK;

		// Check if we can walk/climb this height
		if ( !pCharDef->Can(CAN_C_FLY) )
		{
			if ( !(wBlockFlags & CAN_I_CLIMB) ) // we can climb anywhere
			{
				WARNWALK(("block.m_Lowest.m_z %d  block.m_Bottom.m_z %d  block.m_Top.m_z %d\n", block.m_Lowest.m_z, block.m_Bottom.m_z, block.m_Top.m_z));
				if ( block.m_Bottom.m_dwTile < TERRAIN_QTY )
				{
					// Stepping on map terrain
					if ( block.m_Bottom.m_z > ptDest.m_z + m_zClimbHeight + iCharHeight + 6 )
						return NULL;
				}
				else
				{
					// Stepping on dynamic item
					if ( block.m_Bottom.m_z > ptDest.m_z + m_zClimbHeight + 2 )
						return NULL;
				}
			}
		}

		// CAN_I_CLIMB is not releveant for moving as you would need CAN_C_FLY to negate it. All others seem to match
		// and the above uncommented checks are redundant (even dont make sense(?))
		//WORD wMoveBlock = (wBlockFlags & CAN_I_MOVEMASK) &~ (CAN_I_CLIMB);
		//WORD wMoveBlock = (wBlockFlags & CAN_I_MOVEMASK) &~ (CAN_I_CLIMB|CAN_I_ROOF);
		//if ( wMoveBlock &~ wCan )
		if ( (wBlockFlags & CAN_I_BLOCK) && !pCharDef->Can(CAN_C_PASSWALLS) )
			return NULL;
		if ( block.m_Bottom.m_z >= UO_SIZE_Z )
			return NULL;
	}

	WARNWALK(("iCharHeight %d  block.m_Top.m_z  %d ptDest.m_z  %d\n", iCharHeight, block.m_Top.m_z, ptDest.m_z));
	if ( g_Cfg.m_iMountHeight && (iCharHeight + ptDest.m_z >= block.m_Top.m_z) && !IsPriv(PRIV_GM|PRIV_ALLMOVE) )
	{
		SysMessageDefault(DEFMSG_MSG_MOUNT_CEILING);
		return NULL;
	}

	if ( pwBlockFlags )
		*pwBlockFlags = wBlockFlags;
	if ( (wBlockFlags & CAN_I_CLIMB) && pClimbHeight )
		*pClimbHeight = block.m_zClimbHeight;

	ptDest.m_z = block.m_Bottom.m_z;
	return pArea;
}

void CChar::FixClimbHeight()
{
	ADDTOCALLSTACK("CChar::FixClimbHeight");
	height_t iCharHeight = GetHeight();
	if ( m_pPlayer )	// always consider mount height on players
		iCharHeight += 4;

	CPointBase pt = GetTopPoint();
	CGrayMapBlockState block(CAN_I_CLIMB, pt.m_z, pt.m_z + iCharHeight + 6, pt.m_z + 2, iCharHeight);
	g_World.GetHeightPoint(pt, block, true);

	if ( (block.m_Bottom.m_z == pt.m_z) && (block.m_dwBlockFlags & CAN_I_CLIMB) )	// we are standing on stairs
		m_zClimbHeight = block.m_zClimbHeight;
	else
		m_zClimbHeight = 0;
	m_fClimbUpdated = true;
}
