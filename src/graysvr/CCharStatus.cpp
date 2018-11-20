// CChar is either an NPC or a Player
#include "graysvr.h"	// predef header.
#include "../network/network.h"

bool CChar::IsResourceMatch(RESOURCE_ID_BASE rid, DWORD dwAmount)
{
	ADDTOCALLSTACK("CChar::IsResourceMatch");
	return IsResourceMatch(rid, dwAmount, 0);
}

bool CChar::IsResourceMatch(RESOURCE_ID_BASE rid, DWORD dwAmount, DWORD dwArgResearch)
{
	ADDTOCALLSTACK("CChar::IsResourceMatch");
	// Is the char a match for this test ?

	switch ( rid.GetResType() )
	{
		case RES_SKILL:			// do I have this skill level?
			return (Skill_GetBase(static_cast<SKILL_TYPE>(rid.GetResIndex())) >= dwAmount);

		case RES_CHARDEF:		// I'm this type of char?
		{
			CCharBase *pCharDef = Char_GetDef();
			ASSERT(pCharDef);
			return (pCharDef->GetResourceID() == rid);
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
			return !ContentConsume(rid, dwAmount, true, dwArgResearch);

		case RES_ITEMDEF:		// do I have these in my posession?
			return !ContentConsume(rid, dwAmount, true);

		default:
			return false;
	}
}

CItemContainer *CChar::GetContainerCreate(LAYER_TYPE layer)
{
	ADDTOCALLSTACK("CChar::GetContainerCreate");
	// Get container on the given layer, or create if it doesn't exist yet

	CItemContainer *pCont = dynamic_cast<CItemContainer *>(LayerFind(layer));
	if ( pCont || g_Serv.IsLoading() )
		return pCont;

	// Create new container if not found
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
	if ( !pCont )
		return NULL;

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

CItem *CChar::LayerFind(LAYER_TYPE layer) const
{
	ADDTOCALLSTACK("CChar::LayerFind");
	// Find the item equipped on given layer

	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( pItem->GetEquipLayer() == layer )
			return pItem;
	}
	return NULL;
}

TRIGRET_TYPE CChar::OnCharTrigForLayerLoop(CScript &s, CTextConsole *pSrc, CScriptTriggerArgs *pArgs, CGString *pResult, LAYER_TYPE layer)
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
		// Just skip to the end
		TRIGRET_TYPE iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult);
		if ( iRet != TRIGRET_ENDIF )
			return iRet;
	}
	else
		s.SeekContext(EndContext);
	return TRIGRET_ENDIF;
}

int CChar::GetWeightLoadPercent(int iWeight) const
{
	ADDTOCALLSTACK("CChar::GetWeightLoadPercent");
	if ( IsPriv(PRIV_GM) )
		return 1;

	int	iMaxCarry = g_Cfg.Calc_MaxCarryWeight(this);
	if ( !iMaxCarry )
		return 1000;	// suppose self extra-overloaded
	return IMULDIV(iWeight, 100, iMaxCarry);
}

bool CChar::CanCarry(const CItem *pItem) const
{
	ADDTOCALLSTACK("CChar::CanCarry");
	if ( IsPriv(PRIV_GM) )
		return true;

	int iItemWeight = 0;
	if ( pItem->GetEquipLayer() != LAYER_DRAGGING )		// if we're dragging the item, its weight is already added on char so don't count it again
		iItemWeight = pItem->GetWeight();

	return (GetTotalWeight() + iItemWeight <= g_Cfg.Calc_MaxCarryWeight(this));
}

bool CChar::CanEquipStr(CItem *pItem) const
{
	ADDTOCALLSTACK("CChar::CanEquipStr");
	if ( IsPriv(PRIV_GM) )
		return true;

	CItemBase *pItemDef = pItem->Item_GetDef();
	if ( !pItemDef->IsTypeEquippable() || !CItemBase::IsVisibleLayer(pItemDef->GetEquipLayer()) )
		return true;

	return (Stat_GetAdjusted(STAT_STR) >= pItemDef->m_ttEquippable.m_StrReq * (100 - pItem->m_LowerRequirements) / 100);
}

LAYER_TYPE CChar::CanEquipLayer(CItem *pItem, LAYER_TYPE layer, CChar *pCharMsg, bool fTest)
{
	ADDTOCALLSTACK("CChar::CanEquipLayer");
	// This takes care of any conflicting items in the slot !
	// NOTE: Do not check to see if i can pick this up or steal this etc.
	// LAYER_NONE = can't equip this

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
			if ( m_pClient )
			{
				SysMessagef("%s %s.", g_Cfg.GetDefaultMsg(DEFMSG_EQUIP_NOT_STRONG_ENOUGH), pItem->GetName());
				if ( pCharMsg && (pCharMsg != this) )
					pCharMsg->SysMessagef("%s %s.", g_Cfg.GetDefaultMsg(DEFMSG_EQUIP_NOT_STRONG_ENOUGH), pItem->GetName());
			}
			return LAYER_NONE;
		}
	}

	if ( (pItem->GetParent() == this) && (pItem->GetEquipLayer() == layer) )		// not a visible item LAYER_TYPE
		return layer;

	CItem *pItemPrev = NULL;
	bool fCanEquip = true;

	switch ( layer )
	{
		case LAYER_PACK:
		case LAYER_AUCTION:
			if ( !pItem->IsType(IT_CONTAINER) )
				fCanEquip = false;
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
				fCanEquip = false;
			break;
		case LAYER_BEARD:
			if ( !pItem->IsType(IT_BEARD) )
				fCanEquip = false;
			break;
		case LAYER_BANKBOX:
			if ( !pItem->IsType(IT_EQ_BANK_BOX) )
				fCanEquip = false;
			break;
		case LAYER_HORSE:
			if ( !pItem->IsType(IT_EQ_HORSE) || !IsMountCapable() )
				fCanEquip = false;
			break;
		case LAYER_HAND1:
		case LAYER_HAND2:
			if ( !pItemDef->IsTypeEquippable() || !pCharDef->Can(CAN_C_USEHANDS) )
			{
				if ( pItemDef->GetDispID() != ITEMID_LIGHT_SRC )	// this light source item is a memory equipped on LAYER_HAND2, so it's ok to equip it even without proper TYPE/CAN
				{
					fCanEquip = false;
					break;
				}
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
		case LAYER_FACE:
		case LAYER_COLLAR:
		case LAYER_RING:
		case LAYER_EARS:
		case LAYER_TALISMAN:
			break;
		default:
			if ( CItemBase::IsVisibleLayer(layer) && !pCharDef->Can(CAN_C_EQUIP) )
				fCanEquip = false;
			break;
	}

	if ( !fCanEquip )
	{
		if ( pCharMsg )
			pCharMsg->SysMessageDefault(DEFMSG_EQUIP_CANNOT);
		return LAYER_NONE;
	}

	// Check for objects already in this slot
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
				if ( !CanMove(pItemPrev) )
					return LAYER_NONE;
				if ( !fTest )
					ItemBounce(pItemPrev);
				break;
		}
	}

	return layer;
}

bool CChar::CheckCorpseCrime(const CItemCorpse *pCorpse, bool fLooting, bool fTest)
{
	ADDTOCALLSTACK("CChar::CheckCorpseCrime");
	// fLooting = looting as apposed to carving.
	// RETURN: true = criminal act !

	if ( !pCorpse || !g_Cfg.m_fLootingIsACrime )
		return false;

	CChar *pCharGhost = pCorpse->m_uidLink.CharFind();
	if ( !pCharGhost || (pCharGhost == this) )
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

CItemCorpse *CChar::FindMyCorpse(bool fIgnoreLOS, int iRadius) const
{
	ADDTOCALLSTACK("CChar::FindMyCorpse");
	// Check if char corpse is nearby
	CWorldSearch Area(GetTopPoint(), iRadius);
	for (;;)
	{
		CItem *pItem = Area.GetItem();
		if ( !pItem )
			break;
		if ( !pItem->IsType(IT_CORPSE) )
			continue;

		CItemCorpse *pCorpse = dynamic_cast<CItemCorpse *>(pItem);
		if ( !pCorpse || (pCorpse->m_uidLink != GetUID()) )
			continue;
		if ( pCorpse->m_itCorpse.m_BaseID != m_prev_id )	// not morphed type
			continue;
		if ( !fIgnoreLOS && !CanSeeLOS(pCorpse) )
			continue;
		return pCorpse;
	}
	return NULL;
}

int CChar::GetHealthPercent() const
{
	ADDTOCALLSTACK("CChar::GetHealthPercent");
	int iStr = Stat_GetAdjusted(STAT_STR);
	if ( !iStr )
		return 0;
	return IMULDIV(Stat_GetVal(STAT_STR), 100, iStr);
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

	if ( ptTop.m_z - pt.m_z < -PLAYER_HEIGHT )	// far under the water somehow
		return false;

	// Is there a solid surface under us?
	DWORD dwBlockFlags = GetMoveBlockFlags();
	return ((g_World.GetHeightPoint2(ptTop, dwBlockFlags, true) == pt.m_z) && (dwBlockFlags & CAN_I_WATER));
}

NPCBRAIN_TYPE CChar::GetNPCBrain(bool fGroupTypes) const
{
	ADDTOCALLSTACK("CChar::GetNPCBrain");
	if ( !m_pNPC )
		return NPCBRAIN_NONE;

	if ( fGroupTypes )
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

BYTE CChar::GetModeFlag(const CClient *pViewer) const
{
	ADDTOCALLSTACK("CChar::GetModeFlag");
	CCharBase *pCharDef = Char_GetDef();
	BYTE bMode = 0;

	if ( IsStatFlag(STATF_Freeze|STATF_Stone) )
		bMode |= CHARMODE_FREEZE;
	if ( pCharDef->IsFemale() )
		bMode |= CHARMODE_FEMALE;

	if ( pViewer && (pViewer->m_NetState->isClientVersion(MINCLIVER_SA) || pViewer->m_NetState->isClientEnhanced()) )
	{
		if ( IsStatFlag(STATF_Hovering) )
			bMode |= CHARMODE_FLYING;
	}
	else
	{
		if ( IsStatFlag(STATF_Poisoned) )
			bMode |= CHARMODE_POISON;
	}

	if ( IsStatFlag(STATF_INVUL) )
		bMode |= CHARMODE_YELLOW;
	if ( GetPrivLevel() > PLEVEL_Player )
		bMode |= CHARMODE_IGNOREMOBS;
	if ( IsStatFlag(STATF_War) )
		bMode |= CHARMODE_WAR;

	DWORD dwFlags = STATF_Sleeping;
	if ( !g_Cfg.m_iColorInvis )		// this is needed for Serv.ColorInvis to work, proper flags must be set
		dwFlags |= STATF_Insubstantial;
	if ( !g_Cfg.m_iColorHidden )
		dwFlags |= STATF_Hidden;
	if ( !g_Cfg.m_iColorInvisSpell )
		dwFlags |= STATF_Invisible;
	if ( IsStatFlag(dwFlags) )		// checking if I have any of these settings enabled on the ini and I have any of them, if so ... CHARMODE_INVIS is set and color applied
		bMode |= CHARMODE_INVIS;

	return bMode;
}

BYTE CChar::GetLightLevel() const
{
	ADDTOCALLSTACK("CChar::GetLightLevel");
	// Get personal light level

	if ( IsStatFlag(STATF_DEAD|STATF_Sleeping|STATF_NightSight) || IsPriv(PRIV_DEBUG) )
		return LIGHT_BRIGHT;
	if ( (g_Cfg.m_iRacialFlags & RACIALF_ELF_NIGHTSIGHT) && IsElf() )		// elves always have nightsight enabled (Night Sight racial trait)
		return LIGHT_BRIGHT;
	return GetTopSector()->GetLight();
}

CItem *CChar::GetSpellbook(SPELL_TYPE spell) const
{
	ADDTOCALLSTACK("CChar::GetSpellbook");
	// Check if the char have any spellbook to cast the spell

	// Search in hands
	CItem *pReturn = NULL;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		CItemBase *pItemDef = pItem->Item_GetDef();
		if ( !pItemDef->IsTypeSpellbook(pItem->GetType()) )
			continue;
		if ( (spell < static_cast<SPELL_TYPE>(pItemDef->m_ttSpellbook.m_Offset)) || (spell > static_cast<SPELL_TYPE>(pItemDef->m_ttSpellbook.m_Offset + pItemDef->m_ttSpellbook.m_MaxSpells)) )
			continue;
		if ( pItem->IsSpellInBook(spell) )
			return pItem;

		pReturn = pItem;		// spellbook found, but it doesn't have the spell... return this book if nothing better is found
	}

	// Search in the top level of backpack
	CItemContainer *pPack = GetContainer(LAYER_PACK);
	if ( pPack )
	{
		for ( CItem *pItem = pPack->GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
		{
			CItemBase *pItemDef = pItem->Item_GetDef();
			if ( !pItemDef->IsTypeSpellbook(pItem->GetType()) )
				continue;
			if ( (spell < static_cast<SPELL_TYPE>(pItemDef->m_ttSpellbook.m_Offset)) || (spell > static_cast<SPELL_TYPE>(pItemDef->m_ttSpellbook.m_Offset + pItemDef->m_ttSpellbook.m_MaxSpells)) )
				continue;
			if ( pItem->IsSpellInBook(spell) )
				return pItem;

			pReturn = pItem;	// spellbook found, but it doesn't have the spell... return this book if nothing better is found
		}
	}
	return pReturn;
}

int CChar::Food_GetLevelPercent() const
{
	ADDTOCALLSTACK("CChar::Food_GetLevelPercent");
	int iMax = Stat_GetMax(STAT_FOOD);
	if ( !iMax )
		return 100;
	return IMULDIV(Stat_GetVal(STAT_FOOD), 100, iMax);
}

LPCTSTR CChar::Food_GetLevelMessage(bool fPet, bool fHappy) const
{
	ADDTOCALLSTACK("CChar::Food_GetLevelMessage");
	int iMax = Stat_GetMax(STAT_FOOD);
	if ( !iMax )
		return g_Cfg.GetDefaultMsg(DEFMSG_PET_HAPPY_UNAFFECTED);

	size_t index = IMULDIV(Stat_GetVal(STAT_FOOD), 8, iMax);

	if ( fPet )
	{
		if ( fHappy )
		{
			static LPCTSTR const sm_szPetHappyMsg[] =
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
			if ( index >= COUNTOF(sm_szPetHappyMsg) - 1 )
				index = COUNTOF(sm_szPetHappyMsg) - 1;
			return sm_szPetHappyMsg[index];
		}

		static LPCTSTR const sm_szPetHungerMsg[] =
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
		if ( index >= COUNTOF(sm_szPetHungerMsg) - 1 )
			index = COUNTOF(sm_szPetHungerMsg) - 1;
		return sm_szPetHungerMsg[index];
	}

	static LPCTSTR const sm_szFoodLevelMsg[] =
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
	if ( index >= (COUNTOF(sm_szFoodLevelMsg) - 1) )
		index = COUNTOF(sm_szFoodLevelMsg) - 1;
	return sm_szFoodLevelMsg[index];
}

WORD CChar::Food_CanEat(CObjBase *pObj) const
{
	ADDTOCALLSTACK("CChar::Food_CanEat");
	// Check if char would want to eat this object
	// RETURN:
	//  0 = not at all
	//  10 = only if starving
	//  20 = needs to be prepared
	//  50 = not bad
	//  75 = yummy
	//  100 = my favorite (I will drop other things to get this)

	CCharBase *pCharDef = Char_GetDef();
	ASSERT(pCharDef);

	size_t iRet = pCharDef->m_FoodType.FindResourceMatch(pObj);
	if ( iRet != pCharDef->m_FoodType.BadIndex() )
		return static_cast<WORD>(pCharDef->m_FoodType[iRet].GetResQty());	// how bad do I want it?

	return 0;
}

LPCTSTR CChar::GetTradeTitle() const
{
	ADDTOCALLSTACK("CChar::GetTradeTitle");
	if ( !m_sTitle.IsEmpty() )
		return m_sTitle;

	TCHAR *pszTemp = Str_GetTemp();

	if ( m_pNPC )
	{
		CCharBase *pCharDef = Char_GetDef();
		ASSERT(pCharDef);
		if ( pCharDef->GetTypeName() != pCharDef->GetTradeName() )
			sprintf(pszTemp, "%s %s", pCharDef->IsFemale() ? g_Cfg.GetDefaultMsg(DEFMSG_TRADETITLE_ARTICLE_FEMALE) : g_Cfg.GetDefaultMsg(DEFMSG_TRADETITLE_ARTICLE_MALE), pCharDef->GetTradeName());
		return pszTemp;
	}

	int iLen;
	SKILL_TYPE skill = Skill_GetBest();
	if ( skill == SKILL_BUSHIDO )
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
		iLen = sprintf(pszTemp, "%s ", sm_SkillTitles->FindName(Skill_GetBase(skill)));
	}
	else if ( skill == SKILL_NINJITSU )
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
		iLen = sprintf(pszTemp, "%s ", sm_SkillTitles->FindName(Skill_GetBase(skill)));
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
		iLen = sprintf(pszTemp, "%s ", sm_SkillTitles->FindName(Skill_GetBase(skill)));
	}

	sprintf(pszTemp + iLen, "%s", static_cast<LPCTSTR>(g_Cfg.GetSkillDef(skill)->m_sTitle));
	return pszTemp;
}

bool CChar::CanDisturb(const CChar *pChar) const
{
	ADDTOCALLSTACK("CChar::CanDisturb");
	// I can see/disturb only players with priv same/less than me.
	if ( !pChar )
		return false;

	if ( GetPrivLevel() < pChar->GetPrivLevel() )
		return !pChar->IsStatFlag(STATF_Insubstantial|STATF_INVUL);
	return true;
}

bool CChar::CanSeeAsDead(const CChar *pChar) const
{
	ADDTOCALLSTACK("CChar::CanSeeAsDead");
	// Check if an dead char can see pChar
	if ( !pChar )
		return false;

	if ( !g_Cfg.m_fDeadCannotSeeLiving )
		return true;
	if ( pChar->m_pPlayer && ((g_Cfg.m_fDeadCannotSeeLiving != 2) || pChar->IsStatFlag(STATF_DEAD) || pChar->IsPriv(PRIV_GM)) )
		return true;
	if ( pChar->m_pNPC && ((pChar->NPC_PetGetOwner() == this) || (pChar->m_pNPC->m_Brain == NPCBRAIN_HEALER)) )
		return true;

	return false;
}

bool CChar::CanSeeInContainer(const CItemContainer *pContItem) const
{
	ADDTOCALLSTACK("CChar::CanSeeInContainer");
	// This is a container of some sort. Can I see inside it?

	if ( !pContItem || IsPriv(PRIV_GM) )
		return true;
	if ( pContItem->IsSearchable() )	// not a bank box or locked box
		return true;

	// Not normally searchable
	// Make some special cases for searchable

	CChar *pChar = dynamic_cast<CChar *>(pContItem->GetTopLevelObj());
	if ( !pChar )
		return false;

	if ( pContItem->IsType(IT_EQ_TRADE_WINDOW) )
	{
		if ( pChar == this )
			return true;

		CItem *pItemTrade = pContItem->m_uidLink.ItemFind();
		if ( pItemTrade )
		{
			CChar *pCharTrade = dynamic_cast<CChar *>(pItemTrade->GetTopLevelObj());
			if ( pCharTrade == this )
				return true;
		}
		return false;
	}

	if ( !pChar->NPC_IsOwnedBy(this) )		// pets and player vendors
		return false;

	if ( pContItem->IsType(IT_EQ_VENDOR_BOX) || pContItem->IsType(IT_EQ_BANK_BOX) )
	{
		// These containers can be used only on the same position where they got opened (see CClient::addBankOpen())
		if ( pContItem->m_itEqBankBox.m_ptOpen != GetTopPoint() )
			return false;
	}
	return true;
}

bool CChar::CanSee(const CObjBaseTemplate *pObj) const
{
	ADDTOCALLSTACK("CChar::CanSee");
	// Can I see this object (char or item)?
	if ( !pObj || IsDisconnected() || !pObj->GetTopLevelObj()->GetTopPoint().IsValidPoint() )
		return false;

	if ( pObj->IsItem() )
	{
		const CItem *pItem = static_cast<const CItem *>(pObj);
		if ( !CanSeeItem(pItem) )
			return false;

		int iDist = pItem->IsTypeMulti() ? UO_MAP_VIEW_RADAR : GetSight();
		if ( GetTopPoint().GetDist(pObj->GetTopLevelObj()->GetTopPoint()) > iDist )
			return false;

		CObjBase *pObjCont = pItem->GetParentObj();
		if ( pObjCont )
		{
			if ( !CanSeeInContainer(dynamic_cast<const CItemContainer *>(pObjCont)) )
				return false;

			if ( m_pClient && IsSetEF(EF_FixCanSeeInClosedConts) )
			{
				// Clients can't see the contents of someone else's container, unless they have opened it first
				if ( pObjCont->IsItem() && (pObjCont->GetTopLevelObj() != this) )
				{
					if ( m_pClient->m_openedContainers.find(pObjCont->GetUID().GetPrivateUID()) == m_pClient->m_openedContainers.end() )
						return false;
				}
			}

			return CanSee(pObjCont);
		}
	}
	else
	{
		const CChar *pChar = static_cast<const CChar *>(pObj);
		if ( pChar == this )
			return true;
		if ( GetTopPoint().GetDist(pChar->GetTopPoint()) > GetSight() )
			return false;
		if ( IsPriv(PRIV_ALLSHOW) )
			return (GetPrivLevel() >= pChar->GetPrivLevel());

		if ( m_pNPC && pChar->IsStatFlag(STATF_DEAD) )
		{
			if ( m_pNPC->m_Brain != NPCBRAIN_HEALER )
				return false;
		}
		else if ( pChar->IsStatFlag(STATF_Invisible|STATF_Insubstantial|STATF_Hidden) )
		{
			if ( pChar->GetPrivLevel() <= PLEVEL_Player )
			{
				if ( IsTrigUsed(TRIGGER_SEEHIDDEN) )
				{
					CChar *pThisConst = const_cast<CChar *>(this);
					CChar *pCharConst = const_cast<CChar *>(pChar);

					CScriptTriggerArgs Args;
					Args.m_iN1 = GetPrivLevel() <= pChar->GetPrivLevel();

					pThisConst->OnTrigger(CTRIG_SeeHidden, pCharConst, &Args);
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
				return CanSee(Horse_GetMountChar());
			if ( !IsPriv(PRIV_GM) )		// only characters with GM or ALLSHOW priv should be able to see disconnected chars
				return false;
		}
	}

	if ( IsPriv(PRIV_ALLSHOW) && (pObj->IsTopLevel() || pObj->IsDisconnected()) )		// don't exclude for logged out and diff maps
		return (GetTopPoint().GetDistBase(pObj->GetTopPoint()) <= GetSight());

	return true;
}

bool CChar::CanSeeLOS(const CPointMap &ptDst, CPointMap *pptBlock, int iMaxDist, WORD wFlags) const
{
	ADDTOCALLSTACK("CChar::CanSeeLOS");
	if ( (m_pPlayer && (g_Cfg.m_iAdvancedLos & ADVANCEDLOS_PLAYER)) || (m_pNPC && (g_Cfg.m_iAdvancedLos & ADVANCEDLOS_NPC)) )
		return CanSeeLOS_Adv(ptDst, pptBlock, iMaxDist, wFlags);

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
		DWORD dwBlockFlags;

		if ( (dir % 2) && !IsSetEF(EF_NoDiagonalCheckLOS) )		// test only diagonal dirs
		{
			CPointMap ptTest = ptSrc;
			DIR_TYPE dirTest1 = static_cast<DIR_TYPE>(dir - 1);		// get 1st ortogonal
			DIR_TYPE dirTest2 = static_cast<DIR_TYPE>(dir + 1);		// get 2nd ortogonal
			if ( dirTest2 == DIR_QTY )		// roll over
				dirTest2 = DIR_N;

			ptTest.Move(dirTest1);
			dwBlockFlags = CAN_C_SWIM|CAN_C_WALK|CAN_C_FLY;
			signed char z = g_World.GetHeightPoint2(ptTest, dwBlockFlags, true);

			if ( (abs(z - ptTest.m_z) > PLAYER_HEIGHT) || (dwBlockFlags & (CAN_I_BLOCK|CAN_I_DOOR)) )		// blocked
			{
				ptTest = ptSrc;
				ptTest.Move(dirTest2);

				dwBlockFlags = CAN_C_SWIM|CAN_C_WALK|CAN_C_FLY;
				z = g_World.GetHeightPoint2(ptTest, dwBlockFlags, true);

				if ( abs(z - ptTest.m_z) > PLAYER_HEIGHT )
					goto blocked;

				if ( dwBlockFlags & (CAN_I_BLOCK|CAN_I_DOOR) )
				{
					ptSrc = ptTest;
					goto blocked;
				}
			}
			ptTest.m_z = z;
		}

		if ( iDist )
		{
			ptSrc.Move(dir);	// NOTE: the dir is very coarse and can change slightly
			dwBlockFlags = CAN_C_SWIM|CAN_C_WALK|CAN_C_FLY;
			signed char z = g_World.GetHeightPoint2(ptSrc, dwBlockFlags, true);

			if ( (abs(z - ptSrc.m_z) > PLAYER_HEIGHT) || (dwBlockFlags & (CAN_I_BLOCK|CAN_I_DOOR)) || (iDistTry > iMaxDist) )
				goto blocked;

			ptSrc.m_z = z;
			iDistTry++;
		}
	}

	return (abs(ptSrc.m_z - ptDst.m_z) < 20);
}

// a - gradient < x < b + gradient
#define BETWEENPOINT(coord, coordt, coords)		((coord > ((double)minimum(coordt, coords) - 0.5)) && (coord < ((double)maximum(coordt, coords) + 0.5)))
#define APPROX(num)								((double)((num - floor(num)) > 0.5) ? ceil(num) : floor(num))
#define WARNLOS(_x_)							if ( g_Cfg.m_wDebugFlags & DEBUGF_LOS ) { g_pLog->EventWarn _x_; }

bool inline CChar::CanSeeLOS_New_Failed(CPointMap *pptBlock, CPointMap &ptNow) const
{
	ADDTOCALLSTACK("CChar::CanSeeLOS_New_Failed");
	if ( pptBlock )
		*pptBlock = ptNow;
	return false;
}

bool CChar::CanSeeLOS_Adv(const CPointMap &ptDst, CPointMap *pptBlock, int iMaxDist, WORD wFlags) const
{
	ADDTOCALLSTACK("CChar::CanSeeLOS_Adv");
	if ( IsPriv(PRIV_GM) )
	{
		WARNLOS(("GM Pass\n"));
		return true;
	}

	CPointMap ptSrc = GetTopPoint();
	CPointMap ptNow = ptSrc;

	if ( ptSrc.m_map != ptDst.m_map )
		return CanSeeLOS_New_Failed(pptBlock, ptNow);
	if ( ptSrc == ptDst )
		return true;

	ptSrc.m_z = minimum(ptSrc.m_z + GetHeightMount(), UO_SIZE_Z);
	WARNLOS(("Total Z: %hhd\n", ptSrc.m_z));

	signed short dx = ptDst.m_x - ptSrc.m_x;
	signed short dy = ptDst.m_y - ptSrc.m_y;
	signed char dz = ptDst.m_z - ptSrc.m_z;

	double dist2d = sqrt(static_cast<double>(dx * dx + dy * dy));
	double dist3d = dz ? sqrt(static_cast<double>(dist2d * dist2d + dz * dz)) : dist2d;

	if ( APPROX(dist2d) > static_cast<double>(iMaxDist) )
	{
		WARNLOS(("(APPROX(dist2d)(%f) > iMaxDist(%f)) --> NoLOS\n", APPROX(dist2d), static_cast<double>(iMaxDist)));
		return CanSeeLOS_New_Failed(pptBlock, ptNow);
	}

	double dFactorX = dx / dist3d;
	double dFactorY = dy / dist3d;
	double dFactorZ = dz / dist3d;

	double nPx = ptSrc.m_x;
	double nPy = ptSrc.m_y;
	double nPz = ptSrc.m_z;

	std::vector<CPointMap> path;
	for (;;)
	{
		if ( BETWEENPOINT(nPx, ptDst.m_x, ptSrc.m_x) && BETWEENPOINT(nPy, ptDst.m_y, ptSrc.m_y) && BETWEENPOINT(nPz, ptDst.m_z, ptSrc.m_z) )
		{
			dx = static_cast<signed short>(APPROX(nPx));
			dy = static_cast<signed short>(APPROX(nPy));
			dz = static_cast<signed char>(APPROX(nPz));

			// Add point to vector
			if ( path.size() )
			{
				CPointMap ptEnd = path.at(path.size() - 1);
				if ( (ptEnd.m_x != dx) || (ptEnd.m_y != dy) || (ptEnd.m_z != dz) )
					path.push_back(CPointMap(dx, dy, dz, ptSrc.m_map));
			}
			else
			{
				path.push_back(CPointMap(dx, dy, dz, ptSrc.m_map));
			}
			WARNLOS(("PATH X:%hd Y:%hd Z:%hhd\n", dx, dy, dz));

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

	const CGrayMapBlock *pBlock = NULL;			// Block of the map (for statics)
	const CUOStaticItemRec *pStatic = NULL;		// Statics iterator (based on GrayMapBlock)
	const CGrayMulti *pMulti = NULL;			// Multi Def (multi check)
	const CUOMultiItemRec2 *pMultiItem = NULL;	// Multi item iterator
	CRegionBase *pRegion = NULL;				// Multi regions
	CRegionLinks rlinks;						// Links to multi regions
	CItem *pItem = NULL;
	CItemBase *pItemDef = NULL;
	CItemBaseDupe *pDupeDef = NULL;

	DWORD dwTFlags = 0;
	height_t iHeight = 0;
	WORD wTerrainId = 0;
	bool fPath = true;
	bool fNullTerrain = false;

	CRegionBase *pSrcRegion = ptSrc.GetRegion(REGION_TYPE_AREA|REGION_TYPE_ROOM|REGION_TYPE_MULTI);
	CRegionBase *pNowRegion = NULL;

	int lp_x = 0, lp_y = 0;
	signed char min_z = 0, max_z = 0;

	for ( size_t i = 0; i < path.size(); lp_x = ptNow.m_x, lp_y = ptNow.m_y, pItemDef = NULL, pStatic = NULL, pMulti = NULL, pMultiItem = NULL, min_z = 0, max_z = 0, ++i )
	{
		ptNow = path.at(i);
		WARNLOS(("---------------------------------------------\n"));
		WARNLOS(("Point %hd,%hd,%hhd\n", ptNow.m_x, ptNow.m_y, ptNow.m_z));

		pNowRegion = ptNow.GetRegion(REGION_TYPE_AREA|REGION_TYPE_ROOM|REGION_TYPE_MULTI);

		if ( (wFlags & LOS_NO_OTHER_REGION) && (pSrcRegion != pNowRegion) )
		{
			WARNLOS(("wFlags & LOS_NO_OTHER_REGION and path is leaving my region - BLOCK\n"));
			fPath = false;
			break;
		}

		if ( (wFlags & LOS_NC_MULTI) && ptNow.GetRegion(REGION_TYPE_MULTI) && (ptNow.GetRegion(REGION_TYPE_MULTI) != ptSrc.GetRegion(REGION_TYPE_MULTI)) )
		{
			WARNLOS(("wFlags & LOS_NC_MULTI and path is crossing another multi - BLOCK\n"));
			fPath = false;
			break;
		}

		if ( (lp_x != ptNow.m_x) || (lp_y != ptNow.m_y) )
		{
			WARNLOS(("\tLoading new map block\n"));
			pBlock = g_World.GetMapBlock(ptNow);
		}

		if ( !pBlock )	// something is wrong
		{
			WARNLOS(("GetMapBlock Failed\n"));
			fPath = false;
			break;
		}

		if ( !(wFlags & LOS_NB_TERRAIN) )
		{
			if ( !((wFlags & LOS_NB_LOCAL_TERRAIN) && (pSrcRegion == pNowRegion)) )
			{
				// ------ MapX.mul Check ----------
				wTerrainId = pBlock->GetTerrain(UO_BLOCK_OFFSET(ptNow.m_x), UO_BLOCK_OFFSET(ptNow.m_y))->m_wTerrainIndex;
				WARNLOS(("Terrain %hu\n", wTerrainId));

				if ( (wFlags & LOS_FISHING) && (ptSrc.GetDist(ptNow) >= 2) && (g_World.GetTerrainItemType(wTerrainId) != IT_WATER) && (g_World.GetTerrainItemType(wTerrainId) != IT_NORMAL) )
				{
					WARNLOS(("Terrain %hu blocked - wFlags & LOS_FISHING, distance >= 2 and type of pItemDef is not IT_WATER\n", wTerrainId));
					WARNLOS(("ptSrc: %hd,%hd,%hhd; ptNow: %hd,%hd,%hhd; wTerrainId: %hu; wTerrainId type: %d\n", ptSrc.m_x, ptSrc.m_y, ptSrc.m_z, ptNow.m_x, ptNow.m_y, ptNow.m_z, wTerrainId, g_World.GetTerrainItemType(wTerrainId)));
					fPath = false;
					break;
				}

				if ( (wTerrainId != TERRAIN_HOLE) && (wTerrainId != 475) )
				{
					if ( (wTerrainId < 430) || (wTerrainId > 437) )
					{
						/*this stuff should do some checking for surrounding items:
						aaa
						aXa
						aaa
						min_z is determined as a minimum of all a/X terrain, where X is ptNow
						*/
						BYTE pos_x = (UO_BLOCK_OFFSET(ptNow.m_x) > 1) ? UO_BLOCK_OFFSET(ptNow.m_x - 1) : 0;
						BYTE pos_y = (UO_BLOCK_OFFSET(ptNow.m_y) > 1) ? UO_BLOCK_OFFSET(ptNow.m_y - 1) : 0;
						const BYTE defx = UO_BLOCK_OFFSET(ptNow.m_x);
						const BYTE defy = UO_BLOCK_OFFSET(ptNow.m_y);
						min_z = pBlock->GetTerrain(pos_x, pos_y)->m_z;
						max_z = pBlock->GetTerrain(defx, defy)->m_z;
						for ( BYTE posy = pos_y; (abs(defx - UO_BLOCK_OFFSET(pos_x)) <= 1) && (pos_x <= 7); ++pos_x )
						{
							for ( pos_y = posy; (abs(defy - UO_BLOCK_OFFSET(pos_y)) <= 1) && (pos_y <= 7); ++pos_y )
								min_z = minimum(min_z, pBlock->GetTerrain(pos_x, pos_y)->m_z);
						}

						WARNLOS(("Terrain %hu - min:%hhd max:%hhd\n", wTerrainId, min_z, max_z));
						if ( CUOMapMeter::IsTerrainNull(wTerrainId) )
							fNullTerrain = true;	// what if there are some items on that hole?
						if ( ((min_z <= ptNow.m_z) && (max_z >= ptNow.m_z)) && ((ptNow.m_x != ptDst.m_x) || (ptNow.m_y != ptDst.m_y) || (min_z > ptDst.m_z) || (max_z < ptDst.m_z)) )
						{
							WARNLOS(("Terrain %hu - min:%hhd max:%hhd - block\n", wTerrainId, min_z, max_z));
							fPath = false;
							break;
						}
						CGrayTerrainInfo land(wTerrainId);
						if ( (land.m_flags & UFLAG1_WATER) && (wFlags & LOS_NC_WATER) )
							fNullTerrain = true;
					}
				}
			}
		}

		// ------ StaticsX.mul Check --------
		if ( !(wFlags & LOS_NB_STATIC) )
		{
			if ( !((wFlags & LOS_NB_LOCAL_STATIC) && (pSrcRegion == pNowRegion)) )
			{
				for ( size_t s = 0; s < pBlock->m_Statics.GetStaticQty(); pStatic = NULL, pItemDef = NULL, ++s )
				{
					pStatic = pBlock->m_Statics.GetStatic(s);
					if ( (pStatic->m_x + pBlock->m_x != ptNow.m_x) || (pStatic->m_y + pBlock->m_y != ptNow.m_y) )
						continue;

					// Fix for stacked items blocking view
					if ( (pStatic->m_x == ptDst.m_x) && (pStatic->m_y == ptDst.m_y) && (pStatic->m_z >= GetTopZ()) && (pStatic->m_z <= ptSrc.m_z) )
						continue;

					pItemDef = CItemBase::FindItemBase(pStatic->GetDispID());
					dwTFlags = 0;
					iHeight = 0;
					fNullTerrain = false;

					if ( !pItemDef )
					{
						WARNLOS(("STATIC - Cannot get pItemDef for item (0%x)\n", pStatic->GetDispID()));
					}
					else
					{
						if ( (wFlags & LOS_FISHING) && (ptSrc.GetDist(ptNow) >= 2) && (pItemDef->GetType() != IT_WATER) && (pItemDef->Can(CAN_I_DOOR|CAN_I_PLATFORM|CAN_I_BLOCK|CAN_I_CLIMB|CAN_I_FIRE|CAN_I_ROOF) || (pItemDef->m_Can & CAN_I_BLOCKLOS)) )
						{
							WARNLOS(("pStatic blocked - wFlags & LOS_FISHING, distance >= 2 and type of pItemDef is not IT_WATER\n"));
							fPath = false;
							break;
						}

						dwTFlags = pItemDef->GetTFlags();
						iHeight = pItemDef->GetHeight();

						if ( pItemDef->GetID() != pStatic->GetDispID() )	// not a parent item
						{
							WARNLOS(("Not a parent item (STATIC)\n"));
							pDupeDef = CItemBaseDupe::GetDupeRef(pStatic->GetDispID());
							if ( pDupeDef )
							{
								dwTFlags = pDupeDef->GetTFlags();
								iHeight = pDupeDef->GetHeight();
							}
							else
							{
								g_Log.EventDebug("Failed to get non-parent reference (static) (DispID 0%x) (X:%hd Y:%hd Z:%hhd)\n", pStatic->GetDispID(), ptNow.m_x, ptNow.m_y, pStatic->m_z);
								dwTFlags = pItemDef->GetTFlags();
								iHeight = pItemDef->GetHeight();
							}
						}
						else
						{
							WARNLOS(("Parent item (STATIC)\n"));
						}

						iHeight = (dwTFlags & UFLAG2_CLIMBABLE) ? iHeight / 2 : iHeight;

						if ( ((dwTFlags & (UFLAG1_WALL|UFLAG1_BLOCK|UFLAG2_PLATFORM)) || (pItemDef->m_Can & CAN_I_BLOCKLOS)) && !((dwTFlags & UFLAG2_WINDOW) && (wFlags & LOS_NB_WINDOWS)) )
						{
							WARNLOS(("pStatic 0%x %hhu,%hhu,%hhd - %hhu\n", pStatic->GetDispID(), pStatic->m_x, pStatic->m_y, pStatic->m_z, iHeight));
							min_z = pStatic->m_z;
							max_z = minimum(iHeight + min_z, UO_SIZE_Z);
							WARNLOS(("dwTFlags(0%lx)\n", dwTFlags));

							WARNLOS(("pStatic 0%x Z check: %hhd,%hhd (now: %hhd) (dest: %hhd)\n", pStatic->GetDispID(), min_z, max_z, ptNow.m_z, ptDst.m_z));
							if ( (min_z <= ptNow.m_z) && (max_z >= ptNow.m_z) )
							{
								if ( (ptNow.m_x != ptDst.m_x) || (ptNow.m_y != ptDst.m_y) || (min_z > ptDst.m_z) || (max_z < ptDst.m_z) )
								{
									WARNLOS(("pStatic blocked - min:%hhd max:%hhd\n", min_z, max_z));
									fPath = false;
									break;
								}
							}
						}
					}
				}
			}
		}

		if ( !fPath )
			break;

		// --------- In game items ----------
		if ( !(wFlags & LOS_NB_DYNAMIC) )
		{
			if ( !((wFlags & LOS_NB_LOCAL_DYNAMIC) && (pSrcRegion == pNowRegion)) )
			{
				CWorldSearch AreaItems(ptNow, 0);
				for (;;)
				{
					pItem = AreaItems.GetItem();
					if ( !pItem )
						break;
					if ( (pItem->GetTopPoint().m_x != ptNow.m_x) || (pItem->GetTopPoint().m_y != ptNow.m_y) )
						continue;
					if ( !CanSeeItem(pItem) )
						continue;

					// Fix for stacked items blocking view
					if ( (pItem->GetTopPoint().m_x == ptDst.m_x) && (pItem->GetTopPoint().m_y == ptDst.m_y) && (pItem->GetTopPoint().m_z >= GetTopZ()) && (pItem->GetTopPoint().m_z <= ptSrc.m_z) )
						continue;

					pItemDef = CItemBase::FindItemBase(pItem->GetDispID());
					dwTFlags = 0;
					iHeight = 0;
					fNullTerrain = false;

					if ( !pItemDef )
					{
						WARNLOS(("DYNAMIC - Cannot get pItemDef for item (0%x)\n", pItem->GetDispID()));
					}
					else
					{
						if ( (wFlags & LOS_FISHING) && (ptSrc.GetDist(ptNow) >= 2) && (pItemDef->GetType() != IT_WATER) && (pItemDef->Can(CAN_I_DOOR|CAN_I_PLATFORM|CAN_I_BLOCK|CAN_I_CLIMB|CAN_I_FIRE|CAN_I_ROOF) || (pItemDef->m_Can & CAN_I_BLOCKLOS)) )
						{
							WARNLOS(("pItem blocked - wFlags & LOS_FISHING, distance >= 2 and type of pItemDef is not IT_WATER\n"));
							fPath = false;
							break;
							//return CanSeeLOS_New_Failed(pptBlock, ptNow);
						}

						dwTFlags = pItemDef->GetTFlags();
						iHeight = pItemDef->GetHeight();

						if ( pItemDef->GetID() != pItem->GetDispID() )	// not a parent item
						{
							WARNLOS(("Not a parent item (dynamic)\n"));
							pDupeDef = CItemBaseDupe::GetDupeRef(pItem->GetDispID());
							if ( !pDupeDef )
							{
								g_Log.EventDebug("Failed to get non-parent reference (dynamic) (DispID 0%x) (X:%hd Y:%hd Z:%hhd)\n", pItem->GetDispID(), ptNow.m_x, ptNow.m_y, pItem->GetTopZ());
								dwTFlags = pItemDef->GetTFlags();
								iHeight = pItemDef->GetHeight();
							}
							else
							{
								dwTFlags = pDupeDef->GetTFlags();
								iHeight = pDupeDef->GetHeight();
							}
						}
						else
						{
							WARNLOS(("Parent item (dynamic)\n"));
						}

						iHeight = (dwTFlags & UFLAG2_CLIMBABLE) ? iHeight / 2 : iHeight;

						if ( ((dwTFlags & (UFLAG1_WALL|UFLAG1_BLOCK|UFLAG2_PLATFORM)) || (pItemDef->m_Can & CAN_I_BLOCKLOS)) && !((dwTFlags & UFLAG2_WINDOW) && (wFlags & LOS_NB_WINDOWS)) )
						{
							WARNLOS(("pItem 0%lx(0%x) %hd,%hd,%hhd - %hhu\n", static_cast<DWORD>(pItem->GetUID()), pItem->GetDispID(), pItem->GetTopPoint().m_x, pItem->GetTopPoint().m_y, pItem->GetTopPoint().m_z, iHeight));
							min_z = pItem->GetTopZ();
							max_z = minimum(iHeight + min_z, UO_SIZE_Z);
							WARNLOS(("dwTFlags(0%lx)\n", dwTFlags));

							WARNLOS(("pItem 0%lx(0%x) Z check: %hhd,%hhd (now: %hhd) (dest: %hhd)\n", static_cast<DWORD>(pItem->GetUID()), pItem->GetDispID(), min_z, max_z, ptNow.m_z, ptDst.m_z));
							if ( (min_z <= ptNow.m_z) && (max_z >= ptNow.m_z) )
							{
								if ( (ptNow.m_x != ptDst.m_x) || (ptNow.m_y != ptDst.m_y) || (min_z > ptDst.m_z) || (max_z < ptDst.m_z) )
								{
									WARNLOS(("pItem blocked - min:%hhd max:%hhd\n", min_z, max_z));
									fPath = false;
									break;
								}
							}
						}
					}
				}
			}
		}

		if ( !fPath )
			break;

		// ----------- Multis ---------------

		if ( !(wFlags & LOS_NB_MULTI) )
		{
			if ( !((wFlags & LOS_NB_LOCAL_MULTI) && (pSrcRegion == pNowRegion)) )
			{
				size_t iQtyRegions = ptNow.GetRegions(REGION_TYPE_MULTI, rlinks);
				if ( iQtyRegions > 0 )
				{
					for ( size_t ii = 0; ii < iQtyRegions; pMulti = NULL, ++ii, pItem = NULL, pRegion = NULL )
					{
						pRegion = rlinks.GetAt(ii);
						if ( pRegion )
							pItem = pRegion->GetResourceID().ItemFind();

						if ( !pItem )
							continue;

						pMulti = g_Cfg.GetMultiItemDefs(pItem);
						if ( !pMulti )
							continue;

						size_t iQtyItems = pMulti->GetItemCount();
						for ( size_t iii = 0; iii < iQtyItems; pItemDef = NULL, pMultiItem = NULL, ++iii )
						{
							pMultiItem = pMulti->GetItem(iii);
							if ( !pMultiItem )
								break;
							if ( !pMultiItem->m_visible )
								continue;
							if ( (pMultiItem->m_dx + pItem->GetTopPoint().m_x != ptNow.m_x) || (pMultiItem->m_dy + pItem->GetTopPoint().m_y != ptNow.m_y) )
								continue;

							pItemDef = CItemBase::FindItemBase(pMultiItem->GetDispID());
							dwTFlags = 0;
							iHeight = 0;
							fNullTerrain = false;

							if ( !pItemDef )
							{
								WARNLOS(("MULTI - Cannot get pItemDef for item (0%x)\n", pMultiItem->GetDispID()));
							}
							else
							{
								if ( (wFlags & LOS_FISHING) && (ptSrc.GetDist(ptNow) >= 2) && (pItemDef->GetType() != IT_WATER) && (pItemDef->Can(CAN_I_DOOR|CAN_I_PLATFORM|CAN_I_BLOCK|CAN_I_CLIMB|CAN_I_FIRE|CAN_I_ROOF) || (pItemDef->m_Can & CAN_I_BLOCKLOS)) )
								{
									WARNLOS(("pMultiItem blocked - wFlags & LOS_FISHING, distance >= 2 and type of pItemDef is not IT_WATER\n"));
									fPath = false;
									break;
									//return CanSeeLOS_New_Failed(pptBlock, ptNow);
								}

								dwTFlags = pItemDef->GetTFlags();
								iHeight = pItemDef->GetHeight();

								if ( pItemDef->GetID() != pMultiItem->GetDispID() )		// not a parent item
								{
									WARNLOS(("Not a parent item (multi)\n"));
									pDupeDef = CItemBaseDupe::GetDupeRef(pMultiItem->GetDispID());
									if ( !pDupeDef )
									{
										g_Log.EventDebug("Failed to get non-parent reference (multi) (DispID 0%x) (X:%hd Y:%hd Z:%hhd)\n", pMultiItem->GetDispID(), ptNow.m_x, ptNow.m_y, static_cast<signed char>(pMultiItem->m_dz) + pItem->GetTopPoint().m_z);
										dwTFlags = pItemDef->GetTFlags();
										iHeight = pItemDef->GetHeight();
									}
									else
									{
										dwTFlags = pDupeDef->GetTFlags();
										iHeight = pDupeDef->GetHeight();
									}
								}
								else
								{
									WARNLOS(("Parent item (multi)\n"));
								}

								iHeight = (dwTFlags & UFLAG2_CLIMBABLE) ? iHeight / 2 : iHeight;

								if ( ((dwTFlags & (UFLAG1_WALL|UFLAG1_BLOCK|UFLAG2_PLATFORM)) || (pItemDef->m_Can & CAN_I_BLOCKLOS)) && !((dwTFlags & UFLAG2_WINDOW) && (wFlags & LOS_NB_WINDOWS)) )
								{
									WARNLOS(("pMultiItem 0%x %hd,%hd,%hd - %hhu\n", pMultiItem->GetDispID(), pMultiItem->m_dx, pMultiItem->m_dy, pMultiItem->m_dz, iHeight));
									min_z = static_cast<signed char>(pMultiItem->m_dz) + pItem->GetTopPoint().m_z;
									max_z = minimum(iHeight + min_z, UO_SIZE_Z);
									WARNLOS(("dwTFlags(0%lx)\n", dwTFlags));

									if ( (min_z <= ptNow.m_z) && (max_z >= ptNow.m_z) )
									{
										if ( (ptNow.m_x != ptDst.m_x) || (ptNow.m_y != ptDst.m_y) || (min_z > ptDst.m_z) || (max_z < ptDst.m_z) )
										{
											WARNLOS(("pMultiItem blocked - min:%hhd max:%hhd\n", min_z, max_z));
											fPath = false;
											break;
										}
									}
								}
							}
						}

						if ( !fPath )
							break;
					}
				}
			}
		}

		if ( fNullTerrain )
			fPath = false;

		if ( !fPath )
			break;
	}

	path.clear();

	if ( !fPath )
		return CanSeeLOS_New_Failed(pptBlock, ptNow);

	return true;
}

#undef BETWEENPOINT
#undef APPROX
#undef WARNLOS

bool CChar::CanSeeLOS(const CObjBaseTemplate *pObj, WORD wFlags) const
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
		const CChar *pChar = dynamic_cast<const CChar *>(pObj);
		if ( pChar )
			pt.m_z = minimum(pt.m_z + pChar->GetHeightMount(), UO_SIZE_Z);
		return CanSeeLOS_Adv(pt, NULL, UO_MAP_VIEW_SIGHT, wFlags);
	}
	return CanSeeLOS(pObj->GetTopPoint(), NULL, UO_MAP_VIEW_SIGHT, wFlags);
}

bool CChar::CanTouch(const CPointMap &pt) const
{
	ADDTOCALLSTACK("CChar::CanTouch");
	// Can I reach this pt from where I'm standing?

	return CanSeeLOS(pt, NULL, 6);
}

bool CChar::CanTouch(const CObjBase *pObj) const
{
	ADDTOCALLSTACK("CChar::CanTouch");
	// Can I reach this obj from where I'm standing?
	// Swords, spears, arms length = x units
	// Use or Grab. May be in snooped container
	// Check for blocking objects

	if ( !pObj )
		return false;

	const CItem *pItem = NULL;
	const CObjBaseTemplate *pObjTop = pObj->GetTopLevelObj();
	int iDist = GetTopDist3D(pObjTop);

	if ( pObj->IsItem() )	// some objects can be used anytime. (even by the dead.)
	{
		pItem = static_cast<const CItem *>(pObj);
		bool fDeathImmune = IsPriv(PRIV_GM);
		switch ( pItem->GetType() )
		{
			case IT_SIGN_GUMP:	// can be seen from a distance.
				return (iDist <= UO_MAP_VIEW_SIGHT);

			case IT_SHRINE:		// We can use shrines when dead !!
				fDeathImmune = true;
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

		if ( !fDeathImmune && IsStatFlag(STATF_DEAD|STATF_Sleeping|STATF_Freeze|STATF_Stone) )
			return false;
	}

	// Search up to top level object
	const CChar *pChar = NULL;
	if ( pObjTop != this )
	{
		if ( pObjTop->IsChar() )
		{
			pChar = static_cast<const CChar *>(pObjTop);
			if ( pChar == this )
				return true;
			if ( IsPriv(PRIV_GM) )
				return (GetPrivLevel() >= pChar->GetPrivLevel());
			if ( pChar->IsStatFlag(STATF_DEAD|STATF_Stone) )
				return false;
		}

		const CObjBase *pObjTest = pObj;
		CObjBase *pObjCont = NULL;
		for (;;)
		{
			pItem = dynamic_cast<const CItem *>(pObjTest);
			if ( !pItem )
				break;

			// What is this inside of ?
			pObjCont = pItem->GetParentObj();
			if ( !pObjCont )
				break;

			pObjTest = pObjCont;
			if ( !CanSeeInContainer(dynamic_cast<const CItemContainer *>(pObjTest)) )
				return false;
		}
	}

	if ( IsPriv(PRIV_GM) )
		return true;

	if ( !CanSeeLOS(pObjTop) )
	{
		if ( Char_GetDef()->Can(CAN_C_DCIGNORELOS) )
			return true;
		else if ( pChar && pChar->Char_GetDef()->Can(CAN_C_DCIGNORELOS) )
			return true;
		else if ( pItem && pItem->Item_GetDef()->Can(CAN_I_DCIGNORELOS) )
			return true;
		else
			return false;
	}
	if ( iDist > 2 )
	{
		if ( Char_GetDef()->Can(CAN_C_DCIGNOREDIST) )
			return true;
		else if ( pChar && pChar->Char_GetDef()->Can(CAN_C_DCIGNOREDIST) )
			return true;
		else if ( pItem && pItem->Item_GetDef()->Can(CAN_I_DCIGNOREDIST) )
			return true;
		else
			return false;
	}
	return true;
}

IT_TYPE CChar::CanTouchStatic(CPointMap &pt, ITEMID_TYPE id, CItem *pItem)
{
	ADDTOCALLSTACK("CChar::CanTouchStatic");
	// Might be a dynamic or a static
	// Set pt to the top level point
	// RETURN:
	//  IT_JUNK = too far away

	if ( pItem )
	{
		if ( !CanTouch(pItem) )
			return IT_JUNK;
		pt = GetTopLevelObj()->GetTopPoint();
		return pItem->GetType();
	}

	// It's a static
	CItemBase *pItemDef = CItemBase::FindItemBase(id);
	if ( !pItemDef )
		return IT_NORMAL;
	if ( !CanTouch(pt) )
		return IT_JUNK;

	// Is this static really here?
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

bool CChar::CanHear(const CObjBaseTemplate *pSrc, TALKMODE_TYPE mode) const
{
	ADDTOCALLSTACK("CChar::CanHear");
	// Check if char can hear pSrc (not necessarily understand it (ghost))
	// NOTE: assume m_pClient->CanHear() has already been called (if it matters)

	if ( !pSrc )	// must be broadcast I guess
		return true;

	const CChar *pCharSrc = dynamic_cast<const CChar *>(pSrc);

	int iHearRange = 0;
	bool fThrough = false;
	switch ( mode )
	{
		case TALKMODE_WHISPER:
			iHearRange = g_Cfg.m_iDistanceWhisper;
			break;
		case TALKMODE_YELL:
			iHearRange = (pCharSrc && pCharSrc->IsPriv(PRIV_GM)) ? INT_MAX : g_Cfg.m_iDistanceYell;
			fThrough = true;
			break;
		case TALKMODE_BROADCAST:
			return true;
		default:
			iHearRange = GetSight();
			break;
	}

	pSrc = pSrc->GetTopLevelObj();
	int iDist = GetTopDist3D(pSrc);
	if ( iDist > iHearRange )
		return false;

	// Check if it can be heard through house walls
	if ( IsSetOF(OF_NoHouseMuteSpeech) || fThrough || IsPriv(PRIV_GM) )
		return true;

	CRegionWorld *pSrcRegion;
	if ( pCharSrc )
	{
		pSrcRegion = pCharSrc->m_pArea;
		if ( pCharSrc->IsPriv(PRIV_GM) )
			return true;
	}
	else
		pSrcRegion = dynamic_cast<CRegionWorld *>(pSrc->GetTopPoint().GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA));

	if ( !m_pArea || !pSrcRegion )	// should not happen really
		return false;
	if ( m_pArea == pSrcRegion )	// same region is always ok
		return true;

	CItem *pItemRegion = pSrcRegion->GetResourceID().ItemFind();
	if ( pItemRegion && !pSrcRegion->IsFlag(REGION_FLAG_SHIP) && (pItemRegion->GetKeyNum("NOMUTESPEECH", false) <= 0) )
		return false;

	pItemRegion = m_pArea->GetResourceID().ItemFind();
	if ( pItemRegion && !m_pArea->IsFlag(REGION_FLAG_SHIP) && (pItemRegion->GetKeyNum("NOMUTESPEECH", false) <= 0) )
		return false;

	return true;
}

bool CChar::CanMove(CItem *pItem, bool fMsg) const
{
	ADDTOCALLSTACK("CChar::CanMove");
	// Check if char can move pItem
	// NOT: test if need to steal. CChar::IsTakeCrime()
	// NOT: test if close enough. CChar::CanTouch()

	if ( !pItem )
		return false;
	if ( IsPriv(PRIV_GM|PRIV_ALLMOVE|PRIV_DEBUG) )
		return true;
	if ( pItem->IsAttr(ATTR_MOVE_NEVER|ATTR_LOCKEDDOWN) && !pItem->IsAttr(ATTR_MOVE_ALWAYS) )
		return false;

	if ( IsStatFlag(STATF_DEAD|STATF_Freeze|STATF_Sleeping|STATF_Insubstantial|STATF_Stone) )
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
	else	// equipped or in a container
	{
		// Can't move items from the trade window (client limitation)
		if ( pItem->GetParentObj()->IsContainer() )
		{
			const CItemContainer *pItemCont = dynamic_cast<const CItemContainer *>(pItem);
			if ( pItemCont && pItemCont->IsItemInTrade() )
			{
				SysMessageDefault(DEFMSG_MSG_TRADE_CANTMOVE);
				return false;
			}
		}

		// Can't move/steal newbie items from another char
		if ( pItem->IsAttr(ATTR_NEWBIE|ATTR_CURSED|ATTR_CURSED2|ATTR_BLESSED2) )
		{
			const CObjBaseTemplate *pObjTop = pItem->GetTopLevelObj();
			if ( pObjTop->IsItem() )
			{
				const CItemCorpse *pCorpse = dynamic_cast<const CItemCorpse *>(pObjTop);
				if ( pCorpse )
				{
					CChar *pChar = pCorpse->IsCorpseSleeping();
					if ( pChar && (pChar != this) )
						return false;
				}
			}
			else if ( pObjTop->IsChar() && (pObjTop != this) )
			{
				if ( pItem->IsAttr(ATTR_NEWBIE) && g_Cfg.m_bAllowNewbTransfer )
				{
					const CChar *pPet = dynamic_cast<const CChar *>(pObjTop);
					if ( pPet && (pPet->NPC_PetGetOwner() != this) )
						return false;
				}
				if ( !pItem->IsItemEquipped() || (pItem->GetEquipLayer() != LAYER_DRAGGING) )
					return false;
			}

			if ( pItem->IsAttr(ATTR_CURSED|ATTR_CURSED2) )
			{
				if ( fMsg )
					SysMessagef("%s %s", pItem->GetName(), g_Cfg.GetDefaultMsg(DEFMSG_CANTMOVE_CURSED));
				pItem->SetAttr(ATTR_IDENTIFIED);
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
				case LAYER_FACE:
				case LAYER_BEARD:
				case LAYER_PACK:
					if ( !IsPriv(PRIV_GM) )
						return false;
					break;
				default:
					if ( !CItemBase::IsVisibleLayer(layer) && !IsPriv(PRIV_GM) )
						return false;
			}
		}
	}

	return pItem->IsMovable();
}

bool CChar::IsTakeCrime(const CItem *pItem, CChar **ppCharMark) const
{
	ADDTOCALLSTACK("CChar::IsTakeCrime");

	if ( !pItem || IsPriv(PRIV_GM|PRIV_ALLMOVE) )
		return false;

	CObjBaseTemplate *pObjTop = pItem->GetTopLevelObj();
	if ( pObjTop == this )
		return false;	// this is yours

	CChar *pCharMark = dynamic_cast<CChar *>(pObjTop);
	if ( ppCharMark )
		*ppCharMark = pCharMark;

	if ( !pCharMark )	// in some (or is) container
	{
		if ( pItem->IsAttr(ATTR_OWNED) && (pItem->m_uidLink != GetUID()) )
			return true;

		const CItemContainer *pCont = dynamic_cast<const CItemContainer *>(pObjTop);
		if ( pCont && pCont->IsAttr(ATTR_OWNED) )
			return true;

		// Taking stuff from someones corpse can be a crime
		const CItemCorpse *pCorpse = dynamic_cast<const CItemCorpse *>(pObjTop);
		if ( pCorpse )
			return const_cast<CChar *>(this)->CheckCorpseCrime(pCorpse, true, true);

		return false;
	}

	if ( pCharMark->NPC_IsOwnedBy(this) || pCharMark->Memory_FindObjTypes(this, MEMORY_FRIEND) )	// he let's you
		return false;

	// Pack animal has no owner?
	if ( (pCharMark->GetNPCBrain(false) == NPCBRAIN_ANIMAL) && !pCharMark->IsStatFlag(STATF_Pet) )
		return false;

	return true;
}

bool CChar::CanUse(CItem *pItem, bool fMoveOrConsume) const
{
	ADDTOCALLSTACK("CChar::CanUse");
	// Check if char can use (consume) the item where it is
	// NOTE: although this implies that we pick it up and use it

	if ( !CanTouch(pItem) )
		return false;

	if ( fMoveOrConsume )
	{
		if ( !CanMove(pItem) )
			return false;
		if ( IsTakeCrime(pItem) )
			return false;
	}
	else
	{
		if ( pItem->IsTopLevel() )		// just snooping I guess
			return true;
		if ( pItem->GetTopLevelObj() != this )		// the item is on another character
			return (IsPriv(PRIV_GM) || pItem->IsType(IT_CONTAINER) || pItem->IsType(IT_BOOK));
	}

	return true;
}

bool CChar::IsMountCapable() const
{
	ADDTOCALLSTACK("CChar::IsMountCapable");
	// Check if char is capable of mounting rides
	if ( IsStatFlag(STATF_DEAD) )
		return false;

	return (IsHuman() || IsElf() || Char_GetDef()->Can(CAN_C_MOUNT));
}

bool CChar::IsVerticalSpace(CPointMap ptDst, bool fForceMount)
{
	ADDTOCALLSTACK("CChar::IsVerticalSpace");
	if ( IsPriv(PRIV_GM|PRIV_ALLMOVE) || !ptDst.IsValidPoint() )
		return true;

	DWORD dwBlockFlags = GetMoveBlockFlags();
	if ( dwBlockFlags & CAN_C_WALK )
		dwBlockFlags |= CAN_I_CLIMB;

	height_t iCharHeight = GetHeight();
	CGrayMapBlockState block(dwBlockFlags, ptDst.m_z, ptDst.m_z + m_zClimbHeight + iCharHeight, ptDst.m_z + m_zClimbHeight + 2, iCharHeight);
	g_World.GetHeightPoint(ptDst, block, true);

	return (iCharHeight + ptDst.m_z + (fForceMount ? 4 : 0) < block.m_Top.m_z);
}

CRegionBase *CChar::CheckValidMove(CPointBase &ptDst, DWORD *pdwBlockFlags, DIR_TYPE dir, height_t *pClimbHeight, bool fPathFinding) const
{
	ADDTOCALLSTACK("CChar::CheckValidMove");
	// Check if its ok to move here
	// NOTE:
	//  Ignore other characters for now
	//  Fill in the proper ptDst.m_z value for this location (if walking)
	// ARGS:
	//  pwBlockFlags = what is blocking me (can be null = don't care)
	// RETURN:
	//  The new region we may be in

	// Test diagonal dirs by two others *only* when already having a normal location
	if ( !fPathFinding && GetTopPoint().IsValidPoint() && (dir % 2) )
	{
		CPointMap ptTest;
		DIR_TYPE dirTest1 = static_cast<DIR_TYPE>(dir - 1);		// get 1st ortogonal
		DIR_TYPE dirTest2 = static_cast<DIR_TYPE>(dir + 1);		// get 2nd ortogonal
		if ( dirTest2 == DIR_QTY )
			dirTest2 = DIR_N;

		ptTest = GetTopPoint();
		ptTest.Move(dirTest1);
		if ( !CheckValidMove(ptTest, pdwBlockFlags, DIR_QTY, pClimbHeight) )
			return NULL;

		ptTest = GetTopPoint();
		ptTest.Move(dirTest2);
		if ( !CheckValidMove(ptTest, pdwBlockFlags, DIR_QTY, pClimbHeight) )
			return NULL;
	}

	CRegionBase *pArea = ptDst.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA|REGION_TYPE_ROOM);
	if ( !pArea )
	{
		WARNWALK(("Failed to get region\n"));
		return NULL;
	}

	DWORD dwCan = GetMoveBlockFlags();
	WARNWALK(("GetMoveBlockFlags()(0%lx)\n", dwCan));
	if ( !(dwCan & (CAN_C_SWIM|CAN_C_WALK|CAN_C_FLY|CAN_C_RUN|CAN_C_HOVER)) )
		return NULL;	// cannot move at all, so WTF?

	DWORD dwBlockFlags = dwCan;
	if ( dwCan & CAN_C_WALK )
	{
		dwBlockFlags |= CAN_I_CLIMB;		// if we can walk than we can climb (ignore CAN_C_FLY here)
		WARNWALK(("dwBlockFlags(0%lx) dwCan(0%lx)\n", dwBlockFlags, dwCan));
	}

	height_t iCharHeight = GetHeight();
	if ( m_pPlayer )	// always consider mount height on players
		iCharHeight += 4;

	CGrayMapBlockState block(dwBlockFlags, ptDst.m_z, ptDst.m_z + m_zClimbHeight + iCharHeight, ptDst.m_z + m_zClimbHeight + 3, iCharHeight);
	WARNWALK(("\t\tCGrayMapBlockState block(0%lx, %hhd, %hhd, %hhd); ptDst.m_z(%hhd) m_zClimbHeight(%hhu)\n", dwBlockFlags, ptDst.m_z, ptDst.m_z + m_zClimbHeight + iCharHeight, ptDst.m_z + m_zClimbHeight + 2, ptDst.m_z, m_zClimbHeight));

	if ( !ptDst.IsValidPoint() )
	{
		DEBUG_ERR(("Character 0%lx on %hd,%hd,%hhd wants to move into an invalid location %hd,%hd,%hhd\n", GetUID().GetObjUID(), GetTopPoint().m_x, GetTopPoint().m_y, GetTopPoint().m_z, ptDst.m_x, ptDst.m_y, ptDst.m_z));
		return NULL;
	}

	g_World.GetHeightPoint(ptDst, block, true);
	dwBlockFlags = block.m_Bottom.m_dwBlockFlags;

	if ( block.m_Top.m_dwBlockFlags )
	{
		dwBlockFlags |= CAN_I_ROOF;		// we are covered by something

		WARNWALK(("block.m_Top.m_z(%hhd) > ptDst.m_z(%hhd) + m_zClimbHeight(%hhu) + (block.m_Top.m_dwTile(0x%lx) > TERRAIN_QTY ? PLAYER_HEIGHT : PLAYER_HEIGHT / 2)(%hhu)\n", block.m_Top.m_z, ptDst.m_z, m_zClimbHeight, block.m_Top.m_dwTile, ptDst.m_z - (m_zClimbHeight + (block.m_Top.m_dwTile > TERRAIN_QTY ? PLAYER_HEIGHT : PLAYER_HEIGHT / 2))));
		if ( block.m_Top.m_z < block.m_Bottom.m_z + (m_zClimbHeight + (block.m_Top.m_dwTile > TERRAIN_QTY ? iCharHeight : iCharHeight / 2)) )
			dwBlockFlags |= CAN_I_BLOCK;	// we can't fit under this
	}

	if ( (dwCan != ULONG_MAX) && (dwBlockFlags != 0x0) )
	{
		WARNWALK(("BottomItemID(0%lx) TopItemID(0%lx)\n", block.m_Bottom.m_dwTile - TERRAIN_QTY, block.m_Top.m_dwTile - TERRAIN_QTY));
		CCharBase *pCharDef = Char_GetDef();
		ASSERT(pCharDef);

		if ( (dwBlockFlags & CAN_I_DOOR) && !pCharDef->Can(CAN_C_GHOST) )
			dwBlockFlags |= CAN_I_BLOCK;
		else if ( (dwBlockFlags & CAN_I_ROOF) && !pCharDef->Can(CAN_C_INDOORS) )
			dwBlockFlags |= CAN_I_BLOCK;
		else if ( (dwBlockFlags & CAN_I_WATER) && !pCharDef->Can(CAN_C_SWIM) )
			dwBlockFlags |= CAN_I_BLOCK;
		else if ( (dwBlockFlags & CAN_I_HOVER) && !pCharDef->Can(CAN_C_HOVER) && !IsStatFlag(STATF_Hovering) )
			dwBlockFlags |= CAN_I_BLOCK;

		// If anything blocks us it should not be overridden by this
		if ( (dwBlockFlags & CAN_I_DOOR) && pCharDef->Can(CAN_C_GHOST) )
			dwBlockFlags &= ~CAN_I_BLOCK;
		else if ( (dwBlockFlags & CAN_I_ROOF) && pCharDef->Can(CAN_C_INDOORS) )
			dwBlockFlags &= ~CAN_I_BLOCK;
		else if ( (dwBlockFlags & CAN_I_WATER) && pCharDef->Can(CAN_C_SWIM) )
			dwBlockFlags &= ~CAN_I_BLOCK;
		else if ( (dwBlockFlags & CAN_I_PLATFORM) && pCharDef->Can(CAN_C_WALK) )
			dwBlockFlags &= ~CAN_I_BLOCK;
		else if ( (dwBlockFlags & CAN_I_HOVER) && (pCharDef->Can(CAN_C_HOVER) || IsStatFlag(STATF_Hovering)) )
			dwBlockFlags &= ~CAN_I_BLOCK;

		// Check if we can walk/climb this height
		if ( !pCharDef->Can(CAN_C_FLY) )
		{
			if ( !(dwBlockFlags & CAN_I_CLIMB) )	// we can climb anywhere
			{
				WARNWALK(("block.m_Lowest.m_z(%hhd) block.m_Bottom.m_z(%hhd) block.m_Top.m_z(%hhd)\n", block.m_Lowest.m_z, block.m_Bottom.m_z, block.m_Top.m_z));
				if ( block.m_Bottom.m_dwTile < TERRAIN_QTY )
				{
					// Stepping on map terrain
					if ( block.m_Bottom.m_z > ptDst.m_z + m_zClimbHeight + iCharHeight + 6 )
						return NULL;
				}
				else
				{
					// Stepping on dynamic item
					if ( block.m_Bottom.m_z > ptDst.m_z + m_zClimbHeight + 2 )
						return NULL;
				}
			}
		}

		// CAN_I_CLIMB is not releveant for moving as you would need CAN_C_FLY to negate it. All others seem to match and the above uncommented checks are redundant (even dont make sense(?))
		//DWORD dwMoveBlock = (dwBlockFlags & CAN_I_MOVEMASK) & ~(CAN_I_CLIMB);
		//DWORD dwMoveBlock = (dwBlockFlags & CAN_I_MOVEMASK) & ~(CAN_I_CLIMB|CAN_I_ROOF);
		//if ( dwMoveBlock &~ wCan )
		if ( (dwBlockFlags & CAN_I_BLOCK) && !pCharDef->Can(CAN_C_PASSWALLS) )
			return NULL;
		if ( (dwBlockFlags & CAN_I_HOVER) && !pCharDef->Can(CAN_C_FLY|CAN_C_HOVER) )
			return NULL;
		if ( block.m_Bottom.m_z >= UO_SIZE_Z )
			return NULL;
	}

	WARNWALK(("iCharHeight(%hhu) block.m_Top.m_z(%hhd) ptDst.m_z(%hhd)\n", iCharHeight, block.m_Top.m_z, ptDst.m_z));
	if ( g_Cfg.m_iMountHeight && (iCharHeight + ptDst.m_z >= block.m_Top.m_z) && !IsPriv(PRIV_GM|PRIV_ALLMOVE) )
	{
		SysMessageDefault(DEFMSG_MSG_MOUNT_CEILING);
		return NULL;
	}

	if ( pdwBlockFlags )
		*pdwBlockFlags = dwBlockFlags;
	if ( (dwBlockFlags & CAN_I_CLIMB) && pClimbHeight )
		*pClimbHeight = block.m_zClimbHeight;

	ptDst.m_z = block.m_Bottom.m_z;
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
