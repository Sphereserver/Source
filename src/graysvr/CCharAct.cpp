//
// CCharAct.cpp
//

#include "graysvr.h"	// predef header.
#include "../network/send.h"

bool CChar::TeleportToObj(int iType, TCHAR *pszArgs)
{
	ADDTOCALLSTACK("CChar::TeleportToObj");

	DWORD dwUID = m_Act_Targ.GetObjUID() & ~UID_F_ITEM;
	DWORD dwTotal = g_World.GetUIDCount();
	DWORD dwCount = dwTotal - 1;

	int iArg = 0;
	if ( iType )
	{
		if ( pszArgs[0] && (iType == 1) )
			dwUID = 0;
		iArg = RES_GET_INDEX(Exp_GetVal(pszArgs));
	}

	while ( dwCount-- )
	{
		if ( ++dwUID >= dwTotal )
			dwUID = 1;

		CObjBase *pObj = g_World.FindUID(dwUID);
		if ( !pObj )
			continue;

		switch ( iType )
		{
			case 0:		// GONAME
			{
				MATCH_TYPE match = Str_Match(pszArgs, pObj->GetName());
				if ( match != MATCH_VALID )
					continue;
				break;
			}
			case 1:		// GOCHAR
			{
				if ( !pObj->IsChar() )
					continue;
				if ( iArg-- > 0 )
					continue;
				break;
			}
			case 2:		// GOTYPE
			{
				if ( !pObj->IsItem() )
					continue;
				CItem *pItem = static_cast<CItem *>(pObj);
				if ( !pItem->IsType(static_cast<IT_TYPE>(iArg)) )
					continue;
				break;
			}
			case 3:		// GOCHARID
			{
				if ( !pObj->IsChar() )
					continue;
				CChar *pChar = static_cast<CChar *>(pObj);
				if ( pChar->GetID() != static_cast<CREID_TYPE>(iArg) )
					continue;
				break;
			}
			case 4:		// GOITEMID
			{
				if ( !pObj->IsItem() )
					continue;
				CItem *pItem = static_cast<CItem *>(pObj);
				if ( pItem->GetID() != static_cast<ITEMID_TYPE>(iArg) )
					continue;
				break;
			}
		}

		CObjBaseTemplate *pObjTop = pObj->GetTopLevelObj();
		if ( !pObjTop || (pObjTop == this) )
			continue;
		if ( pObjTop->IsChar() )
		{
			if ( !CanDisturb(static_cast<CChar *>(pObjTop)) )
				continue;
		}

		m_Act_Targ = pObj->GetUID();
		Spell_Teleport(pObjTop->GetTopPoint(), true, false);
		return true;
	}
	return false;
}

bool CChar::TeleportToCli(int iType, int iArgs)
{
	ADDTOCALLSTACK("CChar::TeleportToCli");

	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( !iType )
		{
			if ( pClient->GetSocketID() != iArgs )
				continue;
		}
		CChar *pChar = pClient->GetChar();
		if ( !pChar )
			continue;
		if ( !CanDisturb(pChar) )
			continue;
		if ( iType )
		{
			if ( iArgs-- )
				continue;
		}
		m_Act_Targ = pChar->GetUID();
		Spell_Teleport(pChar->GetTopPoint(), true, false);
		return true;
	}
	return false;
}

void CChar::Jail(CTextConsole *pSrc, bool fSet, int iCell)
{
	ADDTOCALLSTACK("CChar::Jail");

	if ( IsTrigUsed(TRIGGER_JAILED) )
	{
		CScriptTriggerArgs Args(static_cast<INT64>(fSet), static_cast<INT64>(iCell));
		if ( OnTrigger(CTRIG_Jailed, pSrc, &Args) == TRIGRET_RET_TRUE )
			return;
	}

	if ( fSet )
	{
		TCHAR szJailName[16];
		if ( iCell )
			snprintf(szJailName, sizeof(szJailName), "jail%d", iCell);
		else
		{
			strncpy(szJailName, "jail", sizeof(szJailName) - 1);
			szJailName[sizeof(szJailName) - 1] = '\0';
		}

		CPointMap ptJail = g_Cfg.GetRegionPoint(szJailName);
		if ( ptJail.IsValidPoint() )
			Spell_Teleport(ptJail, true, false);

		if ( m_pPlayer )
		{
			CAccount *pAccount = m_pPlayer->m_pAccount;
			if ( !pAccount || pAccount->IsPriv(PRIV_JAILED) )
				return;

			pAccount->SetPrivFlags(PRIV_JAILED);
			pAccount->m_TagDefs.SetNum("JailCell", iCell, true);
			SysMessageDefault(DEFMSG_MSG_JAILED);
		}
	}
	else
	{
		if ( m_pPlayer )
		{
			CAccount *pAccount = m_pPlayer->m_pAccount;
			if ( !pAccount || !pAccount->IsPriv(PRIV_JAILED) )
				return;

			pAccount->ClearPrivFlags(PRIV_JAILED);
			pAccount->m_TagDefs.DeleteKey("JailCell");
			SysMessageDefault(DEFMSG_MSG_FORGIVEN);
		}
	}
}

// A vendor is giving me gold, put it in my pack or other place
void CChar::AddGoldToPack(DWORD dwAmount, CItemContainer *pPack, bool fSound)
{
	ADDTOCALLSTACK("CChar::AddGoldToPack");

	if ( !pPack )
		pPack = GetContainerCreate(LAYER_PACK);

	CItem *pGold = NULL;
	WORD wGoldStack = 0;
	while ( dwAmount > 0 )
	{
		wGoldStack = minimum(dwAmount, g_Cfg.m_iItemsMaxAmount);
		pGold = CItem::CreateScript(ITEMID_GOLD_C1, this);
		pGold->SetAmount(wGoldStack);
		pPack->ContentAdd(pGold);
		dwAmount -= wGoldStack;
	}

	if ( fSound && pGold )
		Sound(pGold->GetDropSound(pPack));
}

// Equip item on given layer
void CChar::LayerAdd(CItem *pItem, LAYER_TYPE layer)
{
	ADDTOCALLSTACK("CChar::LayerAdd");

	if ( !pItem )
		return;
	if ( (pItem->GetParent() == this) && (pItem->GetEquipLayer() == layer) )
		return;

	if ( layer == LAYER_DRAGGING )
	{
		pItem->RemoveSelf();	// remove from where I am before add so UNEQUIP effect takes
		// NOTE: CanEquipLayer may bounce an item. If it stacks with this we are in trouble
	}

	if ( !g_Serv.IsLoading() )
	{
		layer = CanEquipLayer(pItem, layer, NULL, false);
		if ( layer == LAYER_NONE )
		{
			// Can't equip the item on this layer, put it in pack instead
			ItemBounce(pItem);
			return;
		}

		if ( !pItem->IsTypeSpellable() && !pItem->m_itSpell.m_spell && !pItem->IsType(IT_WAND) )	// can this item have a spell effect? If so we do not send 
		{
			if ( IsTrigUsed(TRIGGER_MEMORYEQUIP) || IsTrigUsed(TRIGGER_ITEMMEMORYEQUIP) )
			{
				CScriptTriggerArgs pArgs;
				pArgs.m_iN1 = layer;
				if ( pItem->OnTrigger(ITRIG_MemoryEquip, this, &pArgs) == TRIGRET_RET_TRUE )
				{
					pItem->Delete();
					return;
				}
			}
		}
	}

	if ( layer == LAYER_SPECIAL )
	{
		if ( pItem->IsType(IT_EQ_TRADE_WINDOW) )
			layer = LAYER_NONE;
	}

	CContainer::ContentAddPrivate(pItem);
	pItem->SetEquipLayer(layer);

	switch ( layer )
	{
		case LAYER_HAND1:
		case LAYER_HAND2:
		{
			if ( pItem->IsTypeWeapon() )
			{
				m_uidWeapon = pItem->GetUID();
				m_uidWeaponLast = pItem->GetUID();
				if ( Fight_IsActive() )
				{
					m_atFight.m_Swing_NextAction = CServTime::GetCurrentTime() + g_Cfg.Calc_CombatAttackSpeed(this, pItem);
					Skill_Start(Fight_GetWeaponSkill());	// update char action
				}
			}
			else if ( pItem->IsType(IT_SHIELD) )
				StatFlag_Set(STATF_HasShield);
			break;
		}
		case LAYER_FLAG_Criminal:
		{
			StatFlag_Set(STATF_Criminal);
			NotoSave_Update();
			if ( m_pClient )
			{
				m_pClient->addBuff(BI_CRIMINALSTATUS, 1153802, 1153828);
				if ( !(g_Cfg.m_fGuardsOnMurderers && Noto_IsEvil()) )
					SysMessageDefault(DEFMSG_MSG_GUARDS);
			}
			return;
		}
		case LAYER_FLAG_SpiritSpeak:
		{
			StatFlag_Set(STATF_SpiritSpeak);
			return;
		}
		case LAYER_FLAG_Stuck:
		{
			StatFlag_Set(STATF_Freeze);
			if ( m_pClient )
			{
				m_pClient->addBuff(BI_PARALYZE, 1075827, 1075828, static_cast<WORD>(pItem->GetTimerAdjusted()));
				m_pClient->addCharMove(this);		// immediately tell the client that now he's unable to move (without this, it will be unable to move only on next tick update)
			}
			break;
		}
		default:
			break;
	}

	if ( layer != LAYER_DRAGGING )
	{
		switch ( pItem->GetType() )
		{
			case IT_EQ_SCRIPT:	// pure script
				break;
			case IT_EQ_MEMORY_OBJ:
			{
				CItemMemory *pMemory = dynamic_cast<CItemMemory *>(pItem);
				if ( pMemory && (pMemory->GetMemoryTypes() != MEMORY_NONE) )
					Memory_UpdateFlags(pMemory);
				break;
			}
			case IT_EQ_HORSE:
			{
				StatFlag_Set(STATF_OnHorse);
				break;
			}
			case IT_COMM_CRYSTAL:
			{
				StatFlag_Set(STATF_COMM_CRYSTAL);
				break;
			}
			default:
				break;
		}
	}

	pItem->Update();
}

// Unequip item
// This may be a delete etc. It can not FAIL!
void CChar::OnRemoveOb(CGObListRec *pObRec)	// override this = called when removed from list
{
	ADDTOCALLSTACK("CChar::OnRemoveOb");
	CItem *pItem = static_cast<CItem *>(pObRec);
	if ( !pItem )
		return;

	LAYER_TYPE layer = pItem->GetEquipLayer();
	if ( IsTrigUsed(TRIGGER_UNEQUIP) || IsTrigUsed(TRIGGER_ITEMUNEQUIP) )
	{
		if ( (layer != LAYER_DRAGGING) && !g_Serv.IsLoading() )
			static_cast<void>(pItem->OnTrigger(ITRIG_UNEQUIP, this));
	}

	CContainer::OnRemoveOb(pObRec);

	switch ( layer )
	{
		case LAYER_HAND1:
		case LAYER_HAND2:
		{
			if ( pItem->IsTypeWeapon() )
			{
				m_uidWeapon.InitUID();
				if ( Fight_IsActive() )
				{
					m_atFight.m_Swing_NextAction = CServTime::GetCurrentTime() + g_Cfg.Calc_CombatAttackSpeed(this, NULL);
					Skill_Start(Fight_GetWeaponSkill());	// update char action
				}
			}
			else if ( pItem->IsType(IT_SHIELD) )
				StatFlag_Clear(STATF_HasShield);

			if ( g_Cfg.IsSkillFlag(m_Act_SkillCurrent, SKF_GATHER) )
				Skill_Cleanup();

			if ( pItem->m_LastParryChance != 0 )
			{
				pItem->m_LastParryChance = 0;
				pItem->UpdatePropertyFlag();
			}
			break;
		}
		case LAYER_FLAG_Criminal:
		{
			StatFlag_Clear(STATF_Criminal);
			NotoSave_Update();
			if ( m_pClient )
			{
				m_pClient->removeBuff(BI_CRIMINALSTATUS);
				if ( !(g_Cfg.m_fGuardsOnMurderers && Noto_IsEvil()) )
					SysMessageDefault(DEFMSG_MSG_GUARDS_NOLONGER);
			}
			break;
		}
		case LAYER_FLAG_SpiritSpeak:
		{
			StatFlag_Clear(STATF_SpiritSpeak);
			break;
		}
		case LAYER_FLAG_Stuck:
		{
			StatFlag_Clear(STATF_Freeze);
			if ( m_pClient )
			{
				m_pClient->removeBuff(BI_PARALYZE);
				m_pClient->addCharMove(this);		// immediately tell the client that now he's able to move (without this, it will be able to move only on next tick update)
			}
			break;
		}
		default:
			break;
	}

	if ( layer != LAYER_DRAGGING )
	{
		switch ( pItem->GetType() )
		{
			case IT_COMM_CRYSTAL:
			{
				if ( !ContentFind(RESOURCE_ID(RES_TYPEDEF, IT_COMM_CRYSTAL), 0, 0) )
					StatFlag_Clear(STATF_COMM_CRYSTAL);
				break;
			}
			case IT_EQ_HORSE:
			{
				StatFlag_Clear(STATF_OnHorse);
				break;
			}
			case IT_EQ_MEMORY_OBJ:
			{
				CItemMemory *pMemory = dynamic_cast<CItemMemory *>(pItem);
				if ( pMemory )
				{
					if ( pMemory->GetMemoryTypes() & MEMORY_IPET )
						StatFlag_Clear(STATF_Pet);

					pMemory->SetMemoryTypes(MEMORY_NONE);
					Memory_UpdateFlags(pMemory);
				}
				break;
			}
			default:
				break;
		}

		if ( CItemBase::IsVisibleLayer(layer) )
		{
			if ( pItem->IsTypeArmor() )
			{
				if ( !IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
					m_defense = CalcArmorDefense();

				m_ResPhysical -= pItem->m_ResPhysical;
				m_ResPhysicalMax -= pItem->m_ResPhysicalMax;
				m_ResFire -= pItem->m_ResFire;
				m_ResFireMax -= pItem->m_ResFireMax;
				m_ResCold -= pItem->m_ResCold;
				m_ResColdMax -= pItem->m_ResColdMax;
				m_ResPoison -= pItem->m_ResPoison;
				m_ResPoisonMax -= pItem->m_ResPoisonMax;
				m_ResEnergy -= pItem->m_ResEnergy;
				m_ResEnergyMax -= pItem->m_ResEnergyMax;
			}
			else if ( pItem->IsTypeWeapon() )
			{
				m_DamPhysical -= pItem->m_DamPhysical;
				m_DamFire -= pItem->m_DamFire;
				m_DamCold -= pItem->m_DamCold;
				m_DamPoison -= pItem->m_DamPoison;
				m_DamEnergy -= pItem->m_DamEnergy;

				if ( pItem->m_MageWeapon != 0 )
				{
					WORD wSkillVal = Skill_GetBase(SKILL_MAGERY) + static_cast<WORD>(pItem->m_MageWeapon * 10);
					Skill_SetBase(SKILL_MAGERY, wSkillVal);
					m_TagDefs.DeleteKey("OVERRIDE.SKILLCAP_25");
				}

				const CItem *pCursedMemory = LayerFind(LAYER_SPELL_Curse_Weapon);
				if ( pCursedMemory )
					pItem->m_HitLifeLeech -= pCursedMemory->m_itSpell.m_spelllevel;
			}

			if ( pItem->m_StrengthBonus != 0 )
				Stat_SetMod(STAT_STR, Stat_GetMod(STAT_STR) - pItem->m_StrengthBonus);
			if ( pItem->m_DexterityBonus != 0 )
				Stat_SetMod(STAT_DEX, Stat_GetMod(STAT_DEX) - pItem->m_DexterityBonus);
			if ( pItem->m_IntelligenceBonus != 0 )
				Stat_SetMod(STAT_INT, Stat_GetMod(STAT_INT) - pItem->m_IntelligenceBonus);

			if ( pItem->m_HitpointIncrease != 0 )
				Stat_SetMax(STAT_STR, Stat_GetMax(STAT_STR) - pItem->m_HitpointIncrease);
			if ( pItem->m_StaminaIncrease != 0 )
				Stat_SetMax(STAT_DEX, Stat_GetMax(STAT_DEX) - pItem->m_StaminaIncrease);
			if ( pItem->m_ManaIncrease != 0 )
				Stat_SetMax(STAT_INT, Stat_GetMax(STAT_INT) - pItem->m_ManaIncrease);

			if ( pItem->m_SpellChanneling )
				m_FasterCasting += 1;

			m_Luck -= pItem->m_Luck;
			m_DamIncrease -= pItem->m_DamIncrease;
			m_SpellDamIncrease -= pItem->m_SpellDamIncrease;
			m_HitLifeLeech -= pItem->m_HitLifeLeech;
			m_HitManaDrain -= pItem->m_HitManaDrain;
			m_HitManaLeech -= pItem->m_HitManaLeech;
			m_HitStaminaLeech -= pItem->m_HitStaminaLeech;
			m_HitChanceIncrease -= pItem->m_HitChanceIncrease;
			m_DefChanceIncrease -= pItem->m_DefChanceIncrease;
			m_DefChanceIncreaseMax -= pItem->m_DefChanceIncreaseMax;
			m_SwingSpeedIncrease -= pItem->m_SwingSpeedIncrease;
			m_FasterCasting -= pItem->m_FasterCasting;
			m_FasterCastRecovery -= pItem->m_FasterCastRecovery;
			m_LowerManaCost -= pItem->m_LowerManaCost;
			m_LowerReagentCost -= pItem->m_LowerReagentCost;
			m_EnhancePotions -= pItem->m_EnhancePotions;
			if ( pItem->m_NightSight )
			{
				if ( m_NightSight )
				{
					StatFlag_Mod(STATF_NightSight, 0);
					if ( m_pClient )
					{
						m_pClient->addLight();
						m_pClient->removeBuff(BI_NIGHTSIGHT);
					}
				}
				m_NightSight -= pItem->m_NightSight;
			}
			m_ReflectPhysicalDamage -= pItem->m_ReflectPhysicalDamage;
		}

		// If items are magical then remove effect here
		Spell_Effect_Remove(pItem);
	}
}

// Drop all backpack items to an given container
void CChar::DropAll(CItemContainer *pDest, DWORD dwAttr)
{
	ADDTOCALLSTACK("CChar::DropAll");
	if ( IsStatFlag(STATF_Conjured) )
		return;

	CItemContainer *pPack = GetContainer(LAYER_PACK);
	if ( pPack )
	{
		if ( pDest )
			pPack->ContentsTransfer(pDest, true);
		else
			pPack->ContentsDump(GetTopPoint(), dwAttr);
	}

	// Transfer equipped items to corpse or your pack (if newbie)
	UnEquipAllItems(pDest);
}

// Drop all equipped items to an given container
void CChar::UnEquipAllItems(CItemContainer *pDest, bool fLeaveHands)
{
	ADDTOCALLSTACK("CChar::UnEquipAllItems");

	CItemContainer *pPack = GetContainerCreate(LAYER_PACK);
	bool fDestCorpse = (pDest && pDest->IsType(IT_CORPSE));

	CItem *pItemNext = NULL;
	LAYER_TYPE layer = LAYER_NONE;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		layer = pItem->GetEquipLayer();

		switch ( layer )
		{
			case LAYER_NONE:
			{
				pItem->Delete();	// get rid of any trades
				continue;
			}
			case LAYER_FLAG_Poison:
			case LAYER_FLAG_Hallucination:
			case LAYER_FLAG_Potion:
			case LAYER_FLAG_Drunk:
			case LAYER_FLAG_Stuck:
			case LAYER_FLAG_PotionUsed:
			{
				if ( IsStatFlag(STATF_DEAD) )
					pItem->Delete();
				continue;
			}
			case LAYER_PACK:
			case LAYER_HORSE:
				continue;
			case LAYER_HAIR:
			case LAYER_BEARD:
			{
				// Copy hair and beard to corpse
				if ( fDestCorpse )
				{
					CItem *pDupe = CItem::CreateDupeItem(pItem);
					pDest->ContentAdd(pDupe);
					pDupe->SetContainedLayer(static_cast<BYTE>(layer));
				}
				continue;
			}
			case LAYER_DRAGGING:
			{
				layer = LAYER_NONE;
				break;
			}
			case LAYER_HAND1:
			case LAYER_HAND2:
			{
				if ( fLeaveHands )
					continue;
				break;
			}
			default:
			{
				if ( !CItemBase::IsVisibleLayer(layer) )	// can't transfer this to corpse
					continue;
				break;
			}
		}

		// Move item to given dest container
		if ( pDest )
		{
			if ( fDestCorpse && pItem->IsAttr(ATTR_INSURED) )
			{
				pItem->ClrAttr(ATTR_INSURED);
				pItem->UpdatePropertyFlag();
			}
			else if ( !pItem->IsAttr(ATTR_NEWBIE|ATTR_MOVE_NEVER|ATTR_BLESSED) )
			{
				pDest->ContentAdd(pItem);
				if ( fDestCorpse )
					pItem->SetContainedLayer(static_cast<BYTE>(layer));
				continue;
			}
		}

		// Move item to char pack
		if ( pPack )
			pPack->ContentAdd(pItem);
	}
}

// Show the world that I'm picking/dropping this object
void CChar::UpdateDrag(CItem *pItem, CObjBase *pCont, CPointMap *pt)
{
	ADDTOCALLSTACK("CChar::UpdateDrag");

	if ( pCont && (pCont->GetTopLevelObj() == this) )		// moving to my own backpack
		return;
	if ( !pCont && !pt && (pItem->GetTopLevelObj() == this) )		// doesn't work for ground objects
		return;

	PacketDragAnimation *cmd = new PacketDragAnimation(this, pItem, pCont, pt);
	UpdateCanSee(cmd, m_pClient);
}


void CChar::UpdateStatsFlag() const
{
	ADDTOCALLSTACK("CChar::UpdateStatsFlag");
	if ( g_Serv.IsLoading() )
		return;

	if ( m_pClient )
		m_pClient->addUpdateStatsFlag();
}

void CChar::UpdateHitsFlag()
{
	ADDTOCALLSTACK("CChar::UpdateHitsFlag");
	if ( g_Serv.IsLoading() )
		return;

	m_fStatusUpdate |= SU_UPDATE_HITS;

	if ( m_pClient )
		m_pClient->addUpdateHitsFlag();
}

void CChar::UpdateModeFlag()
{
	ADDTOCALLSTACK("CChar::UpdateModeFlag");
	if ( g_Serv.IsLoading() )
		return;

	m_fStatusUpdate |= SU_UPDATE_MODE;
}

void CChar::UpdateManaFlag() const
{
	ADDTOCALLSTACK("CChar::UpdateManaFlag");
	if ( g_Serv.IsLoading() )
		return;

	if ( m_pClient )
		m_pClient->addUpdateManaFlag();
}

void CChar::UpdateStamFlag() const
{
	ADDTOCALLSTACK("CChar::UpdateStamFlag");
	if ( g_Serv.IsLoading() )
		return;

	if ( m_pClient )
		m_pClient->addUpdateStamFlag();
}

void CChar::UpdateRegenTimers(STAT_TYPE stat, WORD wVal)
{
	ADDTOCALLSTACK("CChar::UpdateRegenTimers");
	m_Stat[stat].m_regen = wVal;
}

void CChar::UpdateStatVal(STAT_TYPE stat, int iChange, int iLimit)
{
	ADDTOCALLSTACK("CChar::UpdateStatVal");
	int iValPrev = Stat_GetVal(stat);
	int iVal = iValPrev + iChange;
	if ( !iLimit )
		iLimit = Stat_GetMax(stat);

	if ( iVal < 0 )
		iVal = 0;
	else if ( iVal > iLimit )
		iVal = iLimit;

	if ( iVal == iValPrev )
		return;

	Stat_SetVal(stat, iVal);

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

// Get ANIM_TYPE value to use with UpdateAnimate()
ANIM_TYPE CChar::GenerateAnimate(ANIM_TYPE action, bool fTranslate)
{
	ADDTOCALLSTACK("CChar::UpdateAnimate");
	if ( (action < ANIM_WALK_UNARM) || (action >= ANIM_QTY) )
		return static_cast<ANIM_TYPE>(-1);
	if ( !fTranslate )
		return action;

	// Begin old client animation behaviour

	if ( action == ANIM_ATTACK_WEAPON )
	{
		CItem *pWeapon = m_uidWeapon.ItemFind();
		if ( pWeapon )
		{
			LAYER_TYPE layer = pWeapon->Item_GetDef()->GetEquipLayer();
			switch ( pWeapon->GetType() )
			{
				case IT_WEAPON_MACE_CROOK:
				case IT_WEAPON_MACE_SMITH:
				case IT_WEAPON_MACE_STAFF:
				case IT_WEAPON_MACE_SHARP:
					action = (layer == LAYER_HAND2) ? ANIM_ATTACK_2H_BASH : ANIM_ATTACK_1H_BASH;
					break;
				case IT_WEAPON_SWORD:
				case IT_WEAPON_AXE:
				case IT_WEAPON_MACE_PICK:
					action = (layer == LAYER_HAND2) ? ANIM_ATTACK_2H_SLASH : ANIM_ATTACK_1H_SLASH;
					break;
				case IT_WEAPON_FENCE:
					action = (layer == LAYER_HAND2) ? ANIM_ATTACK_2H_PIERCE : ANIM_ATTACK_1H_PIERCE;
					break;
				case IT_WEAPON_THROWING:
					action = ANIM_ATTACK_1H_SLASH;
					break;
				case IT_WEAPON_BOW:
					action = ANIM_ATTACK_BOW;
					break;
				case IT_WEAPON_XBOW:
					action = ANIM_ATTACK_XBOW;
					break;
				case IT_WEAPON_WHIP:
					action = ANIM_ATTACK_1H_BASH;
					break;
			}
		}
		else
			action = ANIM_ATTACK_WRESTLE;
	}

	if ( IsStatFlag(STATF_OnHorse) )
	{
		switch ( action )
		{
			case ANIM_WALK_UNARM:
			case ANIM_WALK_ARM:
			case ANIM_WALK_WAR:
				return ANIM_HORSE_RIDE_SLOW;
			case ANIM_RUN_UNARM:
			case ANIM_RUN_ARMED:
				return ANIM_HORSE_RIDE_FAST;
			case ANIM_STAND:
			case ANIM_STAND_WAR_1H:
			case ANIM_STAND_WAR_2H:
				return ANIM_HORSE_STAND;
			case ANIM_ATTACK_1H_SLASH:
			case ANIM_ATTACK_1H_PIERCE:
			case ANIM_ATTACK_1H_BASH:
			case ANIM_CAST_DIR:
			case ANIM_ATTACK_WRESTLE:
				return ANIM_HORSE_ATTACK;
			case ANIM_FIDGET1:
			case ANIM_FIDGET_YAWN:
			case ANIM_ATTACK_2H_BASH:
			case ANIM_ATTACK_2H_SLASH:
			case ANIM_ATTACK_2H_PIERCE:
			case ANIM_GET_HIT:
			case ANIM_BLOCK:
				return ANIM_HORSE_SLAP;
			case ANIM_CAST_AREA:
			case ANIM_ATTACK_BOW:
				return ANIM_HORSE_ATTACK_BOW;
			case ANIM_ATTACK_XBOW:
			case ANIM_BOW:
			case ANIM_SALUTE:
			case ANIM_EAT:
				return ANIM_HORSE_ATTACK_XBOW;
			default:
				return ANIM_HORSE_STAND;
		}
	}
	else if ( !IsPlayableCharacter() )		//( GetDispID() < CREID_MAN ) possible fix for anims not being displayed above 400
	{
		if ( GetDispID() >= CREID_HORSE1 )
		{
			switch ( action )
			{
				case ANIM_WALK_UNARM:
				case ANIM_WALK_ARM:
				case ANIM_WALK_WAR:
					return ANIM_ANI_WALK;
				case ANIM_RUN_UNARM:
				case ANIM_RUN_ARMED:
					return ANIM_ANI_RUN;
				case ANIM_STAND:
				case ANIM_STAND_WAR_1H:
				case ANIM_STAND_WAR_2H:
				case ANIM_FIDGET1:
					return ANIM_ANI_FIDGET1;
				case ANIM_FIDGET_YAWN:
					return ANIM_ANI_FIDGET2;
				case ANIM_CAST_DIR:
					return ANIM_ANI_ATTACK1;
				case ANIM_CAST_AREA:
				case ANIM_EAT:
					return ANIM_ANI_EAT;
				case ANIM_GET_HIT:
					return ANIM_ANI_GETHIT;
				case ANIM_ATTACK_1H_SLASH:
				case ANIM_ATTACK_1H_PIERCE:
				case ANIM_ATTACK_1H_BASH:
				case ANIM_ATTACK_2H_BASH:
				case ANIM_ATTACK_2H_SLASH:
				case ANIM_ATTACK_2H_PIERCE:
				case ANIM_ATTACK_BOW:
				case ANIM_ATTACK_XBOW:
				case ANIM_ATTACK_WRESTLE:
				{
					static const ANIM_TYPE sm_Anim_Ani_Attack[] = { ANIM_ANI_ATTACK1, ANIM_ANI_ATTACK2 };
					return sm_Anim_Ani_Attack[Calc_GetRandVal(COUNTOF(sm_Anim_Ani_Attack))];
				}
				case ANIM_DIE_BACK:
					return ANIM_ANI_DIE1;
				case ANIM_DIE_FORWARD:
					return ANIM_ANI_DIE2;
				case ANIM_BLOCK:
				case ANIM_BOW:
				case ANIM_SALUTE:
					return ANIM_ANI_SLEEP;
				default:
					return ANIM_WALK_UNARM;
			}
		}
		else
		{
			// Monsters don't have all the anims
			switch ( action )
			{
				case ANIM_CAST_DIR:
					return ANIM_MON_Stomp;
				case ANIM_CAST_AREA:
					return ANIM_MON_PILLAGE;
				case ANIM_DIE_BACK:
					return ANIM_MON_DIE1;
				case ANIM_DIE_FORWARD:
					return ANIM_MON_DIE2;
				case ANIM_GET_HIT:
				{
					static const ANIM_TYPE sm_Anim_Mon_GetHit[] = { ANIM_MON_GETHIT, ANIM_MON_BlockRight, ANIM_MON_BlockLeft };
					return sm_Anim_Mon_GetHit[Calc_GetRandVal(COUNTOF(sm_Anim_Mon_GetHit))];
				}
				case ANIM_ATTACK_1H_SLASH:
				case ANIM_ATTACK_1H_PIERCE:
				case ANIM_ATTACK_1H_BASH:
				case ANIM_ATTACK_2H_BASH:
				case ANIM_ATTACK_2H_PIERCE:
				case ANIM_ATTACK_2H_SLASH:
				case ANIM_ATTACK_BOW:
				case ANIM_ATTACK_XBOW:
				case ANIM_ATTACK_WRESTLE:
				{
					static const ANIM_TYPE sm_Anim_Mon_Attack[] = { ANIM_MON_ATTACK1, ANIM_MON_ATTACK2, ANIM_MON_ATTACK3 };
					return sm_Anim_Mon_Attack[Calc_GetRandVal(COUNTOF(sm_Anim_Mon_Attack))];
				}
				default:
					return ANIM_WALK_UNARM;
			}
		}
	}
	return action;
}

// NPC or character does a certain animation
// Translate the animation based on creature type
// ARGS:
//	fBackward = make the anim go in reverse
//	iFrameDelay = approx time, in seconds (0=fast, 1=slow)
bool CChar::UpdateAnimate(ANIM_TYPE action, bool fTranslate, bool fBackward, BYTE iFrameDelay, BYTE iAnimLen)
{
	ADDTOCALLSTACK("CChar::UpdateAnimate");
	if ( (action < 0) || (action >= ANIM_QTY) )
		return false;

	ANIM_TYPE_NEW subaction = static_cast<ANIM_TYPE_NEW>(-1);
	BYTE variation = 0;		// seems to have some effect for humans/elfs vs gargoyles
	if ( fTranslate )
		action = GenerateAnimate(action, true);
	ANIM_TYPE_NEW action1 = static_cast<ANIM_TYPE_NEW>(action);
	if ( IsPlayableCharacter() )		// perform these checks only for gargoyles or in enhanced client
	{
		CItem *pWeapon = m_uidWeapon.ItemFind();
		if ( pWeapon && (action == ANIM_ATTACK_WEAPON) )
		{
			if ( !IsGargoyle() )		// set variation to 1 for non gargoyle characters (humans/elfs using EC) in all fighting animations
				variation = 1;

			// Action depends on weapon type and 2 hand type
			LAYER_TYPE layer = pWeapon->Item_GetDef()->GetEquipLayer();
			action1 = NANIM_ATTACK;
			switch ( pWeapon->GetType() )
			{
				case IT_WEAPON_MACE_CROOK:
				case IT_WEAPON_MACE_PICK:
				case IT_WEAPON_MACE_SMITH:
				case IT_WEAPON_MACE_STAFF:
				case IT_WEAPON_MACE_SHARP:
					subaction = (layer == LAYER_HAND2) ? NANIM_ATTACK_2H_BASH : NANIM_ATTACK_1H_BASH;
					break;
				case IT_WEAPON_SWORD:
				case IT_WEAPON_AXE:
					subaction = (layer == LAYER_HAND2) ? NANIM_ATTACK_2H_PIERCE : NANIM_ATTACK_1H_PIERCE;
					break;
				case IT_WEAPON_FENCE:
					subaction = (layer == LAYER_HAND2) ? NANIM_ATTACK_2H_SLASH : NANIM_ATTACK_1H_SLASH;
					break;
				case IT_WEAPON_THROWING:
					subaction = NANIM_ATTACK_THROWING;
					break;
				case IT_WEAPON_BOW:
					subaction = NANIM_ATTACK_BOW;
					break;
				case IT_WEAPON_XBOW:
					subaction = NANIM_ATTACK_CROSSBOW;
					break;
				case IT_WEAPON_WHIP:
					subaction = NANIM_ATTACK_1H_BASH;
					break;
			}
		}
		else
		{
			switch ( action )
			{
				case ANIM_ATTACK_1H_SLASH:
					action1 = NANIM_ATTACK;
					subaction = NANIM_ATTACK_2H_BASH;
					break;
				case ANIM_ATTACK_1H_PIERCE:
					action1 = NANIM_ATTACK;
					subaction = NANIM_ATTACK_1H_SLASH;
					break;
				case ANIM_ATTACK_1H_BASH:
					action1 = NANIM_ATTACK;
					subaction = NANIM_ATTACK_1H_PIERCE;
					break;
				case ANIM_ATTACK_2H_PIERCE:
					action1 = NANIM_ATTACK;
					subaction = NANIM_ATTACK_2H_SLASH;
					break;
				case ANIM_ATTACK_2H_SLASH:
					action1 = NANIM_ATTACK;
					subaction = NANIM_ATTACK_2H_BASH;
					break;
				case ANIM_ATTACK_2H_BASH:
					action1 = NANIM_ATTACK;
					subaction = NANIM_ATTACK_2H_SLASH;
					break;
				case ANIM_CAST_DIR:
					action1 = NANIM_SPELL;
					subaction = NANIM_SPELL_NORMAL;
					break;
				case ANIM_CAST_AREA:
					action1 = NANIM_SPELL;
					subaction = NANIM_SPELL_SUMMON;
					break;
				case ANIM_ATTACK_BOW:
					subaction = NANIM_ATTACK_BOW;
					break;
				case ANIM_ATTACK_XBOW:
					subaction = NANIM_ATTACK_CROSSBOW;
					break;
				case ANIM_GET_HIT:
					action1 = NANIM_GETHIT;
					break;
				case ANIM_BLOCK:
					action1 = NANIM_BLOCK;
					variation = 1;
					break;
				case ANIM_ATTACK_WRESTLE:
					action1 = NANIM_ATTACK;
					subaction = NANIM_ATTACK_WRESTLING;
					break;
				/*case ANIM_BOW:		// these 2 anims doesn't showing properly when hovering/mounted, so we skip them
					action1 = NANIM_EMOTE;
					subaction = NANIM_EMOTE_BOW;
					break;
				case ANIM_SALUTE:
					action1 = NANIM_EMOTE;
					subaction = NANIM_EMOTE_SALUTE;
					break;*/
				case ANIM_EAT:
					action1 = NANIM_EAT;
					break;
			}
		}
	}

	// Other new animations than work on humans, elfs and gargoyles
	switch ( action )
	{
		case ANIM_DIE_BACK:
			variation = 1;		// variation makes characters die back
			action1 = NANIM_DEATH;
			break;
		case ANIM_DIE_FORWARD:
			action1 = NANIM_DEATH;
			break;
	}
	PacketAction *cmdOld = new PacketAction(this, action, 1, fBackward, iFrameDelay, iAnimLen);
	PacketActionNew *cmdNew = new PacketActionNew(this, action1, subaction, variation);

	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( !pClient->CanSee(this) )
			continue;
		if ( PacketActionNew::CanSendTo(pClient->m_NetState) && IsGargoyle() && (action1 >= 0) )
			cmdNew->send(pClient);
		else
			cmdOld->send(pClient);
	}
	delete cmdOld;
	delete cmdNew;
	return true;
}

// Resend character after change status, polymorph, warmode, hide, etc
void CChar::Update(bool fFull, CClient *pClientExclude)
{
	ADDTOCALLSTACK("CChar::Update");

	if ( !pClientExclude )
		m_fStatusUpdate &= ~SU_UPDATE_MODE;

	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( pClient == pClientExclude )
			continue;
		if ( !pClient->GetChar() )
			continue;
		if ( GetTopPoint().GetDist(pClient->GetChar()->GetTopPoint()) > pClient->GetChar()->GetSight() )
			continue;
		if ( !pClient->CanSee(this) )
		{
			// In the case of "INVIS" used by GM's we must use this
			pClient->addObjectRemove(this);
			continue;
		}

		if ( fFull )
			pClient->addChar(this);
		else
		{
			pClient->addCharMove(this);
			pClient->addHealthBarUpdate(this);
		}
	}
}

// Character is moving (walk/run/teleport)
void CChar::UpdateMove(const CPointMap &ptOld, CClient *pExcludeClient)
{
	ADDTOCALLSTACK("CChar::UpdateMove");

	if ( !pExcludeClient )
		m_fStatusUpdate &= ~SU_UPDATE_MODE;

	EXC_TRY("UpdateMove");
	EXC_SET("FOR LOOP");
	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( pClient == pExcludeClient )
			continue;	// no need to see self move

		if ( pClient == m_pClient )
		{
			EXC_SET("AddPlayerView");
			pClient->addPlayerView(ptOld);
			continue;
		}

		EXC_SET("GetChar");
		CChar *pChar = pClient->GetChar();
		if ( !pChar )
			continue;

		bool fCouldSee = (ptOld.GetDist(pChar->GetTopPoint()) <= pChar->GetSight());
		EXC_SET("CanSee");
		if ( !pClient->CanSee(this) )
		{
			if ( fCouldSee )
			{
				EXC_SET("AddObjRem");
				pClient->addObjectRemove(this);		// this client can't see me anymore
			}
		}
		else if ( fCouldSee )
		{
			EXC_SET("AddcharMove");
			pClient->addCharMove(this);		// this client already saw me, just send the movement packet
		}
		else
		{
			EXC_SET("AddChar");
			pClient->addChar(this);			// first time this client has seen me, send complete packet
		}
	}
	EXC_CATCH;
}

// Character changed its facing direction
void CChar::UpdateDir(DIR_TYPE dir)
{
	ADDTOCALLSTACK("CChar::UpdateDir");

	if ( (dir != m_dirFace) && (dir > DIR_INVALID) && (dir < DIR_QTY) )
	{
		m_dirFace = dir;
		UpdateMove(GetTopPoint());
	}
}

// Character changed its facing direction
void CChar::UpdateDir(const CPointMap &pt)
{
	ADDTOCALLSTACK("CChar::UpdateDir");
	UpdateDir(GetTopPoint().GetDir(pt));
}

// Character changed its facing direction
void CChar::UpdateDir(const CObjBaseTemplate *pObj)
{
	ADDTOCALLSTACK("CChar::UpdateDir");
	if ( !pObj )
		return;

	pObj = pObj->GetTopLevelObj();
	if ( !pObj || (pObj == this) )		// in our own pack
		return;
	UpdateDir(pObj->GetTopPoint());
}

// Make this char generate some sound according to the given action
void CChar::SoundChar(CRESND_TYPE type)
{
	ADDTOCALLSTACK("CChar::SoundChar");
	if ( !g_Cfg.m_fGenericSounds )
		return;

	const CCharBase *pCharDef = Char_GetDef();

	if ( type == CRESND_RAND )
		type = Calc_GetRandVal(2) ? CRESND_IDLE : CRESND_NOTICE;

	switch ( type )
	{
		case CRESND_IDLE:
		{
			if ( pCharDef->m_soundIdle == static_cast<SOUND_TYPE>(-1) )
				return;
			else if ( pCharDef->m_soundIdle )
				return Sound(pCharDef->m_soundIdle);
			break;
		}
		case CRESND_NOTICE:
		{
			if ( pCharDef->m_soundNotice == static_cast<SOUND_TYPE>(-1) )
				return;
			else if ( pCharDef->m_soundNotice )
				return Sound(pCharDef->m_soundNotice);
			break;
		}
		case CRESND_HIT:
		{
			CItem *pWeapon = m_uidWeapon.ItemFind();
			if ( pWeapon )
			{
				CVarDefCont *pVar = pWeapon->GetDefKey("AMMOSOUNDHIT", true);
				if ( pVar )
					return Sound(static_cast<SOUND_TYPE>(pVar->GetValNum()));

				switch ( pWeapon->GetType() )
				{
					case IT_WEAPON_MACE_CROOK:
					case IT_WEAPON_MACE_PICK:
					case IT_WEAPON_MACE_SMITH:
					case IT_WEAPON_MACE_STAFF:
						return Sound(static_cast<SOUND_TYPE>(0x233));		//blunt01
					case IT_WEAPON_MACE_SHARP:
						return Sound(static_cast<SOUND_TYPE>(0x232));		//axe01
					case IT_WEAPON_SWORD:
					case IT_WEAPON_AXE:
						if ( pWeapon->Item_GetDef()->GetEquipLayer() == LAYER_HAND2 )	//if not two handed, don't break, just fall through and use same sound as fencing weapons
							return Sound(Calc_GetRandVal(2) ? static_cast<SOUND_TYPE>(0x236) : static_cast<SOUND_TYPE>(0x237));		//hvyswrd1 : hvyswrd4
					case IT_WEAPON_FENCE:
						return Sound(Calc_GetRandVal(2) ? static_cast<SOUND_TYPE>(0x23B) : static_cast<SOUND_TYPE>(0x23C));			//sword1 : sword7
					case IT_WEAPON_BOW:
					case IT_WEAPON_XBOW:
						return Sound(static_cast<SOUND_TYPE>(0x234));		//xbow
					case IT_WEAPON_THROWING:
						return Sound(static_cast<SOUND_TYPE>(0x5D2));		//throwH
					case IT_WEAPON_WHIP:
						return Sound(static_cast<SOUND_TYPE>(0x67E));		//whip01
					default:
						break;
				}
			}
			else if ( pCharDef->m_soundHit == static_cast<SOUND_TYPE>(-1) )
				return;
			else if ( pCharDef->m_soundHit )
				return Sound(pCharDef->m_soundHit);
			break;
		}
		case CRESND_GETHIT:
		{
			if ( pCharDef->m_soundGetHit )
				return Sound(pCharDef->m_soundGetHit);
			break;
		}
		case CRESND_DIE:
		{
			if ( pCharDef->m_soundDie == static_cast<SOUND_TYPE>(-1) )
				return;
			else if ( pCharDef->m_soundDie )
				return Sound(pCharDef->m_soundDie);
			break;
		}
	}

	SOUND_TYPE id = pCharDef->m_soundBase;
	if ( id == SOUND_NONE )
	{
		DEBUG_MSG(("CHARDEF %s has no base SOUND!\n", GetResourceName()));
		return;
	}
	else if ( id == SOUND_SPECIAL_HUMAN )	// Special (hardcoded) sound. Try to keep less hardcoded stuff possible, but this is useful
	{
		if ( type == CRESND_HIT )
		{
			// Same sound for every race/sex
			static const SOUND_TYPE sm_Snd_Hit[] = { 0x135, 0x137, 0x13B };
			id = sm_Snd_Hit[Calc_GetRandVal(COUNTOF(sm_Snd_Hit))];
		}
		else if ( pCharDef->IsFemale() )
		{
			switch ( type )
			{
				case CRESND_GETHIT:
				{
					static const SOUND_TYPE sm_Snd_Female_GetHit[] = { 0x14B, 0x14C, 0x14D, 0x14E, 0x14F };
					id = sm_Snd_Female_GetHit[Calc_GetRandVal(COUNTOF(sm_Snd_Female_GetHit))];
					break;
				}
				case CRESND_DIE:
				{
					static const SOUND_TYPE sm_Snd_Female_Die[] = { 0x150, 0x151, 0x152, 0x153 };
					id = sm_Snd_Female_Die[Calc_GetRandVal(COUNTOF(sm_Snd_Female_Die))];
					break;
				}
				default:
					break;
			}
		}
		else
		{
			switch ( type )
			{
				case CRESND_GETHIT:
				{
					static const SOUND_TYPE sm_Snd_Male_GetHit[] = { 0x154, 0x155, 0x156, 0x157, 0x158, 0x159 };
					id = sm_Snd_Male_GetHit[Calc_GetRandVal(COUNTOF(sm_Snd_Male_GetHit))];
					break;
				}
				case CRESND_DIE:
				{
					static const SOUND_TYPE sm_Snd_Male_Die[] = { 0x15A, 0x15B, 0x15C, 0x15D };
					id = sm_Snd_Male_Die[Calc_GetRandVal(COUNTOF(sm_Snd_Male_Die))];
					break;
				}
				default:
					break;
			}
		}
		// No idle/notice sounds for this
	}
	else if ( id < 0x4D6 )		// before Crane sound, sound IDs are ordered in a way...
	{
		id += static_cast<SOUND_TYPE>(type);
	}
	else if ( id < 0x5D5 )		// between Crane and Abyssal Infernal sounds, there's another scheme
	{
		switch ( type )
		{
			case CRESND_IDLE:	id += static_cast<SOUND_TYPE>(2);	break;
			case CRESND_NOTICE:	id += static_cast<SOUND_TYPE>(3);	break;
			case CRESND_HIT:	id += static_cast<SOUND_TYPE>(1);	break;
			case CRESND_GETHIT:	id += static_cast<SOUND_TYPE>(4);	break;
			default: break;
		}
	}
	else		//after Abyssal Infernal sound, there's another scheme (and NPCs have 4 sounds instead of 5)
	{
		switch ( type )
		{
			case CRESND_IDLE:	id += static_cast<SOUND_TYPE>(3);	break;
			case CRESND_NOTICE:	id += static_cast<SOUND_TYPE>(3);	break;
			case CRESND_GETHIT:	id += static_cast<SOUND_TYPE>(2);	break;
			case CRESND_DIE:	id += static_cast<SOUND_TYPE>(1);	break;
			default: break;
		}
	}
	Sound(id);
}

bool CChar::ItemPickup(CItem *pItem, WORD wAmount)
{
	ADDTOCALLSTACK("CChar::ItemPickup");
	// Char is picking up an item
	// If pickup succeed, item will be placed "up in the air" (LAYER_DRAGGING)
	if ( !pItem )
		return false;

	if ( pItem->GetParent() == this )
	{
		if ( pItem->GetEquipLayer() == LAYER_HORSE )
			return false;
		if ( pItem->GetEquipLayer() == LAYER_DRAGGING )
			return true;
	}

	if ( !CanTouch(pItem) || !CanMove(pItem, true) )
		return false;

	const CObjBaseTemplate *pObjTop = pItem->GetTopLevelObj();

	if ( m_pClient )
	{
		const CItem *pItemCont = dynamic_cast<const CItem *>(pItem->GetParent());
		if ( pItemCont )
		{
			// Don't allow taking items from the bank unless we opened it here
			if ( pItemCont->IsType(IT_EQ_BANK_BOX) && (pItemCont->m_itEqBankBox.m_ptOpen != GetTopPoint()) )
				return false;

			// Check sub containers too
			CChar *pCharTop = dynamic_cast<CChar *>(const_cast<CObjBaseTemplate *>(pObjTop));
			if ( pCharTop )
			{
				CItemContainer *pBank = pCharTop->GetContainerCreate(LAYER_BANKBOX);
				if ( pBank->IsItemInside(pItemCont) && (pBank->m_itEqBankBox.m_ptOpen != GetTopPoint()) )
					return false;
			}

			// Don't allow pick items from containers not opened (protection against ,snoop)
			bool isInOpenedContainer = false;
			CClient::OpenedContainerMap_t::iterator itContainerFound = m_pClient->m_openedContainers.find(pItemCont->GetUID().GetPrivateUID());
			if ( itContainerFound != m_pClient->m_openedContainers.end() )
			{
				DWORD dwTopContainerUID = (((*itContainerFound).second).first).first;
				DWORD dwTopMostContainerUID = (((*itContainerFound).second).first).second;
				CPointMap ptOpenedContainerPosition = ((*itContainerFound).second).second;

				DWORD dwTopContainerUID_ToCheck = 0;
				if ( pItemCont->GetParentObj() )
					dwTopContainerUID_ToCheck = pItemCont->GetParentObj()->GetUID().GetPrivateUID();
				else
					dwTopContainerUID_ToCheck = pObjTop->GetUID().GetPrivateUID();

				if ( (dwTopMostContainerUID == pObjTop->GetUID().GetPrivateUID()) && (dwTopContainerUID == dwTopContainerUID_ToCheck) )
				{
					if ( pCharTop )
						isInOpenedContainer = true;
					else
					{
						CItem *pItemTop = dynamic_cast<CItem *>(const_cast<CObjBaseTemplate *>(pObjTop));
						if ( pItemTop && (pItemTop->IsType(IT_SHIP_HOLD) || pItemTop->IsType(IT_SHIP_HOLD_LOCK)) && (pItemTop->GetTopPoint().GetRegion(REGION_TYPE_MULTI) == GetTopPoint().GetRegion(REGION_TYPE_MULTI)) )
							isInOpenedContainer = true;
						else if ( ptOpenedContainerPosition.GetDist(pObjTop->GetTopPoint()) <= 3 )
							isInOpenedContainer = true;
					}
				}
			}

			if ( !isInOpenedContainer )
				return false;
		}
	}

	const CChar *pChar = dynamic_cast<const CChar *>(pObjTop);
	if ( (pChar != this) && pItem->IsAttr(ATTR_OWNED) && (pItem->m_uidLink != GetUID()) && !IsPriv(PRIV_ALLMOVE|PRIV_GM) )
		return false;

	const CItemCorpse *pCorpse = dynamic_cast<const CItemCorpse *>(pObjTop);
	if ( pCorpse )
	{
		if ( pCorpse->m_uidLink == GetUID() )
		{
			if ( g_Cfg.m_iRevealFlags & REVEALF_LOOTINGSELF )
				Reveal();
		}
		else
		{
			CheckCorpseCrime(pCorpse, true, false);
			if ( g_Cfg.m_iRevealFlags & REVEALF_LOOTINGOTHERS )
				Reveal();
		}
	}

	WORD wAmountMax = pItem->GetAmount();
	if ( wAmountMax <= 0 )
		return false;

	if ( (wAmount <= 0) || (wAmount > wAmountMax) || !pItem->Item_GetDef()->IsStackableType() )
		wAmount = wAmountMax;

	ITRIG_TYPE trigger;
	if ( pChar )
	{
		bool fCanTake = false;
		if ( pChar == this )	// we can always take our own items
			fCanTake = true;
		else if ( pItem->GetParentObj() != pChar )		// our owners can take items from us
			fCanTake = pChar->NPC_IsOwnedBy(this);
		else
			fCanTake = (IsPriv(PRIV_GM) && (GetPrivLevel() > pChar->GetPrivLevel()));	// higher priv players can take items and undress us

		if ( !fCanTake )
			return false;

		trigger = pItem->IsItemEquipped() ? ITRIG_UNEQUIP : ITRIG_PICKUP_PACK;
	}
	else
		trigger = pItem->IsTopLevel() ? ITRIG_PICKUP_GROUND : ITRIG_PICKUP_PACK;

	if ( trigger == ITRIG_PICKUP_GROUND )
	{
		// Bug with taking static/movenever items -or- catching the spell effects
		if ( !IsPriv(PRIV_ALLMOVE|PRIV_GM) && (pItem->IsAttr(ATTR_STATIC|ATTR_MOVE_NEVER) || pItem->IsType(IT_SPELL)) )
			return false;
	}

	if ( trigger != ITRIG_UNEQUIP )		// unequip is done later
	{
		if ( IsTrigUsed(CItem::sm_szTrigName[trigger]) || IsTrigUsed(sm_szTrigName[(CTRIG_itemAfterClick - 1) + trigger]) )		//ITRIG_PICKUP_GROUND, ITRIG_PICKUP_PACK
		{
			CScriptTriggerArgs Args(wAmount);
			if ( pItem->OnTrigger(trigger, this, &Args) == TRIGRET_RET_TRUE )
				return false;
		}
		if ( (trigger == ITRIG_PICKUP_PACK) && (IsTrigUsed(TRIGGER_PICKUP_SELF) || IsTrigUsed(TRIGGER_ITEMPICKUP_SELF)) )
		{
			CItem *pContItem = dynamic_cast<CItem *>(pItem->GetParentObj());
			if ( pContItem )
			{
				CScriptTriggerArgs Args1(pItem);
				if ( pContItem->OnTrigger(ITRIG_PICKUP_SELF, this, &Args1) == TRIGRET_RET_TRUE )
					return false;
			}
		}
	}

	if ( pItem->Item_GetDef()->IsStackableType() && (wAmount < wAmountMax) )
	{
		// Create an leftover item when pick up only part of the stack
		CItem *pItemNew = pItem->UnStackSplit(wAmount, this);
		pItemNew->SetTimeout(pItem->GetTimerDAdjusted());

		if ( IsTrigUsed(TRIGGER_PICKUP_STACK) || IsTrigUsed(TRIGGER_ITEMPICKUP_STACK) )
		{
			CScriptTriggerArgs Args2(pItemNew);
			if ( pItem->OnTrigger(ITRIG_PICKUP_STACK, this, &Args2) == TRIGRET_RET_TRUE )
				return false;
		}
	}

	// Do stack dropping if items are stacked
	if ( (trigger == ITRIG_PICKUP_GROUND) && IsSetEF(EF_ItemStackDrop) )
	{
		signed char iItemHeight = static_cast<signed char>(pItem->GetHeight());
		if ( iItemHeight < 1 )
			iItemHeight = 1;

		signed char iStackMaxZ = GetTopZ() + 16;
		CItem *pStack = NULL;
		CPointMap ptNewPlace = pItem->GetTopPoint();
		CWorldSearch AreaItems(ptNewPlace);
		for (;;)
		{
			pStack = AreaItems.GetItem();
			if ( !pStack )
				break;
			if ( (pStack->GetTopZ() <= pItem->GetTopZ()) || (pStack->GetTopZ() > iStackMaxZ) )
				continue;
			if ( pStack->IsAttr(ATTR_MOVE_NEVER|ATTR_STATIC|ATTR_LOCKEDDOWN) )
				continue;

			ptNewPlace = pStack->GetTopPoint();
			ptNewPlace.m_z -= iItemHeight;
			pStack->MoveToUpdate(ptNewPlace);
		}
	}

	CItemSpawn *pSpawn = static_cast<CItemSpawn *>(pItem->m_uidSpawnItem.ItemFind());
	if ( pSpawn )
		pSpawn->DelObj(pItem->GetUID());

	UpdateDrag(pItem);

	// Remove the item from other clients view if the item is being taken from the ground by a hidden character to prevent lingering item
	if ( (trigger == ITRIG_PICKUP_GROUND) && IsStatFlag(STATF_Insubstantial|STATF_Invisible|STATF_Hidden) )
		pItem->RemoveFromView(m_pClient);

	// Pick it up
	pItem->SetDecayTime(-1);
	LayerAdd(pItem, LAYER_DRAGGING);
	return true;
}

// Bounce an item into backpack
bool CChar::ItemBounce(CItem *pItem, bool fDisplayMsg)
{
	ADDTOCALLSTACK("CChar::ItemBounce");
	if ( !pItem )
		return false;

	CItemContainer *pPack = GetContainerCreate(LAYER_PACK);
	if ( pItem->GetParent() == pPack )
		return true;

	LPCTSTR pszWhere = NULL;
	if ( pPack && CanCarry(pItem) )		// this can happen at load time
	{
		if ( IsTrigUsed(TRIGGER_DROPON_ITEM) )
		{
			CScriptTriggerArgs Args(pPack);
			static_cast<void>(pItem->OnTrigger(ITRIG_DROPON_ITEM, this, &Args));

			if ( pItem->IsDeleted() )	// the trigger had deleted the item
				return false;
		}

		pszWhere = g_Cfg.GetDefaultMsg(DEFMSG_MSG_BOUNCE_PACK);
		pItem->RemoveFromView();
		pPack->ContentAdd(pItem);		// drop it on pack
		Sound(pItem->GetDropSound(pPack));
	}
	else
	{
		if ( !GetTopPoint().IsValidPoint() )
		{
			DEBUG_ERR(("Deleted item '%s' (UID=0%" FMTDWORDH ") because parent char '%s' (UID=0%" FMTDWORDH ") not placed on world can't carry it and also can't drop it on ground\n", pItem->GetName(), static_cast<DWORD>(pItem->GetUID()), GetName(), static_cast<DWORD>(GetUID())));
			pItem->Delete();
			return false;
		}
		pszWhere = g_Cfg.GetDefaultMsg(DEFMSG_MSG_FEET);
		pItem->RemoveFromView();
		pItem->MoveToDecay(GetTopPoint(), pItem->GetDecayTime());	// drop it on ground
	}

	if ( fDisplayMsg )
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_ITEMPLACE), pItem->GetName(), pszWhere);
	return true;
}

// Char is dropping an item on the ground
bool CChar::ItemDrop(CItem *pItem, const CPointMap &pt)
{
	ADDTOCALLSTACK("CChar::ItemDrop");
	if ( !pItem )
		return false;

	if ( IsSetEF(EF_ItemStacking) )
	{
		CPointMap ptStack = pt;
		CItem *pStack = NULL;
		CWorldSearch AreaItems(ptStack);
		for (;;)
		{
			pStack = AreaItems.GetItem();
			if ( !pStack )
				break;
			if ( (pStack->GetTopZ() < pt.m_z) || (pStack->GetTopZ() > pt.m_z + WALL_HEIGHT) )
				continue;

			signed char iStackHeight = static_cast<signed char>(pStack->GetHeight());
			if ( iStackHeight < 1 )
				iStackHeight = 1;

			ptStack.m_z += iStackHeight;
		}

		if ( (ptStack.m_z - pt.m_z > WALL_HEIGHT - 4) || !CanSeeLOS(ptStack) )
			return false;
		return pItem->MoveToCheck(ptStack, this);	// don't flip the item if it got stacked
	}

	// Check if this item can flip
	CItemBase *pItemDef = pItem->Item_GetDef();
	if ( (g_Cfg.m_fFlipDroppedItems || pItemDef->Can(CAN_I_FLIP)) && pItem->IsMovable() && !pItemDef->IsStackableType() )
		pItem->SetDispID(pItemDef->GetNextFlipID(pItem->GetDispID()));

	return pItem->MoveToCheck(pt, this);
}

// Char is equipping an item
bool CChar::ItemEquip(CItem *pItem, CChar *pCharMsg, bool fFromDClick)
{
	ADDTOCALLSTACK("CChar::ItemEquip");
	if ( !pItem )
		return false;
	if ( (pItem->GetParent() == this) && (pItem->GetEquipLayer() != LAYER_DRAGGING) )	// item is already equipped
		return true;

	if ( IsTrigUsed(TRIGGER_EQUIPTEST) || IsTrigUsed(TRIGGER_ITEMEQUIPTEST) )
	{
		if ( pItem->OnTrigger(ITRIG_EQUIPTEST, this) == TRIGRET_RET_TRUE )
		{
			if ( !pItem->IsDeleted() && (!pCharMsg || !pCharMsg->m_pClient) )
				ItemBounce(pItem);
			return false;
		}
	}

	if ( pItem->GetAmount() > 1 )
		pItem->UnStackSplit(1, this);

	LAYER_TYPE layer = CanEquipLayer(pItem, LAYER_QTY, pCharMsg, false);
	if ( layer == LAYER_NONE )		// can't equip this
	{
		if ( !pCharMsg || !pCharMsg->m_pClient )
			ItemBounce(pItem);
		return false;
	}

	pItem->RemoveSelf();		// remove it from the container so that nothing will be stacked with it if unequipped
	pItem->SetDecayTime(-1);
	LayerAdd(pItem, layer);
	if ( !pItem->IsItemEquipped() )
		return false;

	if ( IsTrigUsed(TRIGGER_EQUIP) || IsTrigUsed(TRIGGER_ITEMEQUIP) )
	{
		if ( pItem->OnTrigger(ITRIG_EQUIP, this) == TRIGRET_RET_TRUE )
		{
			if ( !pItem->IsDeleted() && (!pCharMsg || !pCharMsg->m_pClient) )
				ItemBounce(pItem);
			return false;
		}
	}

	if ( !pItem->IsItemEquipped() )
		return false;

	Spell_Effect_Add(pItem);

	if ( fFromDClick )
		pItem->ResendOnEquip();

	if ( CItemBase::IsVisibleLayer(layer) )
	{
		CVarDefCont *pVar = GetDefKey("EQUIPSOUND", true);
		Sound(pVar ? static_cast<SOUND_TYPE>(pVar->GetValNum()) : SOUND_USE_CLOTH);

		if ( pItem->IsTypeArmor() )
		{
			if ( !IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
				m_defense = CalcArmorDefense();

			m_ResPhysical += pItem->m_ResPhysical;
			m_ResPhysicalMax += pItem->m_ResPhysicalMax;
			m_ResFire += pItem->m_ResFire;
			m_ResFireMax += pItem->m_ResFireMax;
			m_ResCold += pItem->m_ResCold;
			m_ResColdMax += pItem->m_ResColdMax;
			m_ResPoison += pItem->m_ResPoison;
			m_ResPoisonMax += pItem->m_ResPoisonMax;
			m_ResEnergy += pItem->m_ResEnergy;
			m_ResEnergyMax += pItem->m_ResEnergyMax;
		}
		else if ( pItem->IsTypeWeapon() )
		{
			m_DamPhysical += pItem->m_DamPhysical;
			m_DamFire += pItem->m_DamFire;
			m_DamCold += pItem->m_DamCold;
			m_DamPoison += pItem->m_DamPoison;
			m_DamEnergy += pItem->m_DamEnergy;

			if ( pItem->m_MageWeapon != 0 )
			{
				WORD wSkillVal = Skill_GetBase(SKILL_MAGERY) - static_cast<WORD>(pItem->m_MageWeapon * 10);
				WORD wSkillCap = Skill_GetMax(SKILL_MAGERY) - static_cast<WORD>(pItem->m_MageWeapon * 10);
				Skill_SetBase(SKILL_MAGERY, wSkillVal);
				m_TagDefs.SetNum("OVERRIDE.SKILLCAP_25", wSkillCap, true);
			}

			const CItem *pCursedMemory = LayerFind(LAYER_SPELL_Curse_Weapon);
			if ( pCursedMemory )
				pItem->m_HitLifeLeech += pCursedMemory->m_itSpell.m_spelllevel;
		}

		if ( pItem->m_StrengthBonus != 0 )
			Stat_SetMod(STAT_STR, Stat_GetMod(STAT_STR) + pItem->m_StrengthBonus);
		if ( pItem->m_DexterityBonus != 0 )
			Stat_SetMod(STAT_DEX, Stat_GetMod(STAT_DEX) + pItem->m_DexterityBonus);
		if ( pItem->m_IntelligenceBonus != 0 )
			Stat_SetMod(STAT_INT, Stat_GetMod(STAT_INT) + pItem->m_IntelligenceBonus);

		if ( pItem->m_HitpointIncrease != 0 )
			Stat_SetMax(STAT_STR, Stat_GetMax(STAT_STR) + pItem->m_HitpointIncrease);
		if ( pItem->m_StaminaIncrease != 0 )
			Stat_SetMax(STAT_DEX, Stat_GetMax(STAT_DEX) + pItem->m_StaminaIncrease);
		if ( pItem->m_ManaIncrease != 0 )
			Stat_SetMax(STAT_INT, Stat_GetMax(STAT_INT) + pItem->m_ManaIncrease);

		if ( pItem->m_SpellChanneling )
			m_FasterCasting -= 1;

		m_Luck += pItem->m_Luck;
		m_DamIncrease += pItem->m_DamIncrease;
		m_SpellDamIncrease += pItem->m_SpellDamIncrease;
		m_HitLifeLeech += pItem->m_HitLifeLeech;
		m_HitManaDrain += pItem->m_HitManaDrain;
		m_HitManaLeech += pItem->m_HitManaLeech;
		m_HitStaminaLeech += pItem->m_HitStaminaLeech;
		m_HitChanceIncrease += pItem->m_HitChanceIncrease;
		m_DefChanceIncrease += pItem->m_DefChanceIncrease;
		m_DefChanceIncreaseMax += pItem->m_DefChanceIncreaseMax;
		m_SwingSpeedIncrease += pItem->m_SwingSpeedIncrease;
		m_FasterCasting += pItem->m_FasterCasting;
		m_FasterCastRecovery += pItem->m_FasterCastRecovery;
		m_LowerManaCost += pItem->m_LowerManaCost;
		m_LowerReagentCost += pItem->m_LowerReagentCost;
		m_EnhancePotions += pItem->m_EnhancePotions;
		if ( pItem->m_NightSight )
		{
			if ( !m_NightSight )
			{
				StatFlag_Mod(STATF_NightSight, 1);
				if ( m_pClient )
				{
					m_pClient->addLight();
					m_pClient->addBuff(BI_NIGHTSIGHT, 1075643, 1075644);
				}
			}
			m_NightSight += pItem->m_NightSight;
		}
		m_ReflectPhysicalDamage += pItem->m_ReflectPhysicalDamage;
	}
	return true;
}

// Char is eating something
void CChar::EatAnim(LPCTSTR pszName, int iQty)
{
	ADDTOCALLSTACK("CChar::EatAnim");
	static const SOUND_TYPE sm_Snd_Eat[] = { 0x3A, 0x3B, 0x3C };
	Sound(sm_Snd_Eat[Calc_GetRandVal(COUNTOF(sm_Snd_Eat))]);

	if ( !IsStatFlag(STATF_OnHorse) )
		UpdateAnimate(ANIM_EAT);

	TCHAR szMsg[EXPRESSION_MAX_KEY_LEN];
	snprintf(szMsg, sizeof(szMsg), g_Cfg.GetDefaultMsg(DEFMSG_MSG_EATSOME), pszName);
	Emote(szMsg);

	int iHits = 0;
	int iMana = 0;
	int iStam = Calc_GetRandVal(3, 6) + (iQty / 5);
	int iFood = iQty;
	int iStatsLimit = 0;
	if ( IsTrigUsed(TRIGGER_EAT) )
	{
		CScriptTriggerArgs Args;
		Args.m_VarsLocal.SetNumNew("Hits", iHits);
		Args.m_VarsLocal.SetNumNew("Mana", iMana);
		Args.m_VarsLocal.SetNumNew("Stam", iStam);
		Args.m_VarsLocal.SetNumNew("Food", iFood);
		Args.m_iN1 = iStatsLimit;
		if ( OnTrigger(CTRIG_Eat, this, &Args) == TRIGRET_RET_TRUE )
			return;

		iHits = static_cast<int>(Args.m_VarsLocal.GetKeyNum("Hits")) + Stat_GetVal(STAT_STR);
		iMana = static_cast<int>(Args.m_VarsLocal.GetKeyNum("Mana")) + Stat_GetVal(STAT_INT);
		iStam = static_cast<int>(Args.m_VarsLocal.GetKeyNum("Stam")) + Stat_GetVal(STAT_DEX);
		iFood = static_cast<int>(Args.m_VarsLocal.GetKeyNum("Food")) + Stat_GetVal(STAT_FOOD);
		iStatsLimit = static_cast<int>(Args.m_iN1);
	}

	if ( iHits )
		UpdateStatVal(STAT_STR, iHits, iStatsLimit);
	if ( iMana )
		UpdateStatVal(STAT_INT, iMana, iStatsLimit);
	if ( iStam )
		UpdateStatVal(STAT_DEX, iStam, iStatsLimit);
	if ( iFood )
		UpdateStatVal(STAT_FOOD, iFood, iStatsLimit);
}

// Invisible char is being revealed (-1 = reveal everything, also invisible GMs)
bool CChar::Reveal(DWORD dwFlags)
{
	ADDTOCALLSTACK("CChar::Reveal");
	if ( !IsStatFlag(dwFlags) )
		return false;
	if ( m_pClient && m_pClient->m_pHouseDesign )	// don't reveal while in house design mode
		return false;

	if ( (dwFlags & STATF_Invisible) && IsStatFlag(STATF_Invisible) )
	{
		CItem *pSpell = LayerFind(LAYER_SPELL_Invis);
		if ( pSpell && pSpell->IsType(IT_SPELL) && (pSpell->m_itSpell.m_spell == SPELL_Invis) )
		{
			pSpell->SetType(IT_NORMAL);		// setting it to IT_NORMAL avoid a second call to this function
			pSpell->Delete();
		}
		pSpell = LayerFind(LAYER_FLAG_Potion);
		if ( pSpell && pSpell->IsType(IT_SPELL) && (pSpell->m_itSpell.m_spell == SPELL_Invis) )
		{
			pSpell->SetType(IT_NORMAL);		// setting it to IT_NORMAL avoid a second call to this function
			pSpell->Delete();
		}
	}

	StatFlag_Clear(dwFlags);
	if ( m_pClient )
	{
		if ( !IsStatFlag(STATF_Hidden|STATF_Insubstantial) )
			m_pClient->removeBuff(BI_HIDDEN);
		if ( !IsStatFlag(STATF_Invisible) )
			m_pClient->removeBuff(BI_INVISIBILITY);
	}

	if ( IsStatFlag(STATF_Invisible|STATF_Hidden|STATF_Insubstantial) )
		return false;

	m_StepStealth = 0;
	Update();
	SysMessageDefault(DEFMSG_HIDING_REVEALED);
	return true;
}

void CChar::SpeakUTF8(LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang)
{
	ADDTOCALLSTACK("CChar::SpeakUTF8");
	// Ignore the font argument here

	if ( (mode != TALKMODE_SPELL) && (g_Cfg.m_iRevealFlags & REVEALF_SPEAK) )
		Reveal();

	CObjBase::SpeakUTF8(pszText, wHue, mode, font, lang);
}

void CChar::SpeakUTF8Ex(const NWORD *pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang)
{
	ADDTOCALLSTACK("CChar::SpeakUTF8Ex");
	// Ignore the font argument here

	if ( (mode != TALKMODE_SPELL) && (g_Cfg.m_iRevealFlags & REVEALF_SPEAK) )
		Reveal();

	CObjBase::SpeakUTF8Ex(pszText, wHue, mode, font, lang);
}

void CChar::Speak(LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font)
{
	ADDTOCALLSTACK("CChar::Speak");
	// Ignore the font argument here

	if ( IsStatFlag(STATF_Stone) )
		return;
	if ( (mode != TALKMODE_SPELL) && (g_Cfg.m_iRevealFlags & REVEALF_SPEAK) )
		Reveal();

	CObjBase::Speak(pszText, wHue, mode, font);
}

CItem *CChar::Make_Figurine(CGrayUID uidOwner, ITEMID_TYPE id)
{
	ADDTOCALLSTACK("CChar::Make_Figurine");
	if ( !m_pNPC || IsDisconnected() )
		return NULL;

	CCharBase *pCharDef = Char_GetDef();
	CItem *pItem = CItem::CreateScript((id == ITEMID_NOTHING) ? pCharDef->m_trackID : id, this);
	if ( !pItem )
		return NULL;

	pItem->SetType(IT_FIGURINE);
	pItem->SetName(GetName());
	pItem->SetHue(GetHue());
	pItem->m_itFigurine.m_ID = GetID();
	pItem->m_itFigurine.m_UID = GetUID();
	pItem->m_uidLink = uidOwner;

	if ( IsStatFlag(STATF_Insubstantial) )
		pItem->SetAttr(ATTR_INVIS);

	SoundChar(CRESND_IDLE);
	Skill_Start(NPCACT_RIDDEN);
	StatFlag_Set(STATF_Ridden);
	SetDisconnected();
	m_atRidden.m_FigurineUID = pItem->GetUID();
	return pItem;
}

CItem *CChar::NPC_Shrink()
{
	ADDTOCALLSTACK("CChar::NPC_Shrink");
	if ( !m_pNPC )
		return NULL;

	NPC_PetClearOwners();	// clear follower slots on pet owner

	if ( IsStatFlag(STATF_Conjured) )
	{
		Stat_SetVal(STAT_STR, 0);
		return NULL;
	}

	CItem *pItem = Make_Figurine();
	if ( !pItem )
		return NULL;

	pItem->SetAttr(ATTR_MAGIC);
	pItem->MoveToCheck(GetTopPoint());
	return pItem;
}

CItem *CChar::Horse_GetMountItem() const
{
	ADDTOCALLSTACK("CChar::Horse_GetMountItem");

	if ( !IsStatFlag(STATF_Ridden) )
		return NULL;

	CItem *pItem = m_atRidden.m_FigurineUID.ItemFind();
	if ( !pItem )
	{
		CItemMemory *pItemMem = Memory_FindTypes(MEMORY_IPET);
		if ( pItemMem )
		{
			CChar *pOwner = pItemMem->m_uidLink.CharFind();
			if ( pOwner )
			{
				CItem *pItemMount = pOwner->LayerFind(LAYER_HORSE);
				if ( pItemMount && (pItemMount->m_itNormal.m_more2 == GetUID()) )
				{
					const_cast<CGrayUIDBase &>(m_atRidden.m_FigurineUID) = pItemMount->GetUID();
					pItem = pItemMount;
					DEBUG_ERR(("UID=0%" FMTDWORDH ", id=0%x '%s', Fixed mount item UID=0%" FMTDWORDH ", id=0%x '%s'\n", static_cast<DWORD>(GetUID()), GetBaseID(), GetName(), static_cast<DWORD>(pItem->GetUID()), pItem->GetBaseID(), pItem->GetName()));
				}
			}
		}
	}

	if ( !pItem || (!pItem->IsType(IT_FIGURINE) && !pItem->IsType(IT_EQ_HORSE)) )
		return NULL;

	return pItem;
}

CChar *CChar::Horse_GetMountChar() const
{
	ADDTOCALLSTACK("CChar::Horse_GetMountChar");
	CItem *pItem = Horse_GetMountItem();
	if ( pItem )
		return dynamic_cast<CChar *>(pItem->GetTopLevelObj());
	return NULL;
}

bool CChar::Horse_Mount(CChar *pHorse)
{
	ADDTOCALLSTACK("CChar::Horse_Mount");

	if ( !CanTouch(pHorse) )
	{
		SysMessageDefault(DEFMSG_MSG_MOUNT_DIST);
		return false;
	}

	TCHAR szMountID[20];
	snprintf(szMountID, sizeof(szMountID), "mount_0x%x", pHorse->GetDispID());
	LPCTSTR pszMemoryID = g_Exp.m_VarDefs.GetKeyStr(szMountID);

	RESOURCE_ID rid = g_Cfg.ResourceGetID(RES_QTY, pszMemoryID);

	ITEMID_TYPE id = static_cast<ITEMID_TYPE>(rid.GetResIndex());
	if ( id <= ITEMID_NOTHING )
		return false;

	if ( m_pClient && m_pClient->m_pHouseDesign )	// can't mount while in house design mode
		return false;

	if ( pHorse->m_pNPC->m_bonded && pHorse->IsStatFlag(STATF_DEAD) )
	{
		SysMessageDefault(DEFMSG_MSG_BONDED_DEAD_CANTMOUNT);
		return false;
	}

	if ( !IsMountCapable() )
	{
		SysMessageDefault(DEFMSG_MSG_MOUNT_UNABLE);
		return false;
	}

	if ( !pHorse->NPC_IsOwnedBy(this) )
	{
		SysMessageDefault(DEFMSG_MSG_MOUNT_DONTOWN);
		return false;
	}

	if ( g_Cfg.m_iMountHeight )
	{
		if ( !IsVerticalSpace(GetTopPoint(), true) )	// is there space for the char + mount?
		{
			SysMessageDefault(DEFMSG_MSG_MOUNT_CEILING);
			return false;
		}
	}

	Horse_UnMount();	// unmount if already mounted

	if ( IsTrigUsed(TRIGGER_MOUNT) )
	{
		CScriptTriggerArgs Args(pHorse);
		if ( OnTrigger(CTRIG_Mount, this, &Args) == TRIGRET_RET_TRUE )
			return false;
	}

	if ( IsPriv(PRIV_GM) )
		pHorse->NPC_PetSetOwner(this);

	CItem *pItem = pHorse->Make_Figurine(GetUID(), id);
	if ( !pItem )
		return false;

	pItem->SetType(IT_EQ_HORSE);
	pItem->SetTimeout(TICK_PER_SEC);
	LayerAdd(pItem, LAYER_HORSE);		// equip the horse item
	pHorse->StatFlag_Set(STATF_Ridden);
	pHorse->Skill_Start(NPCACT_RIDDEN);
	return true;
}

bool CChar::Horse_UnMount()
{
	ADDTOCALLSTACK("CChar::Horse_UnMount");
	if ( !IsStatFlag(STATF_OnHorse) || (IsStatFlag(STATF_Stone) && !IsPriv(PRIV_GM)) || (m_pClient && m_pClient->m_pHouseDesign) )
		return false;

	CItem *pItem = LayerFind(LAYER_HORSE);
	if ( !pItem || pItem->IsDeleted() )
	{
		StatFlag_Clear(STATF_OnHorse);	// flag got out of sync
		return false;
	}

	if ( pItem->GetDispID() == ITEMID_MEMORY_SHIP_PILOT )
	{
		CItemShip *pShip = dynamic_cast<CItemShip *>(pItem->m_uidLink.ItemFind());
		if ( pShip )
			pShip->Ship_SetPilot(NULL);
		return true;
	}

	CChar *pHorse = pItem->m_itFigurine.m_UID.CharFind();
	if ( IsTrigUsed(TRIGGER_DISMOUNT) && pHorse && pHorse->IsDisconnected() && !pHorse->IsDeleted() )
	{
		CScriptTriggerArgs Args(pHorse);
		if ( OnTrigger(CTRIG_Dismount, this, &Args) == TRIGRET_RET_TRUE )
			return false;
	}

	Use_Figurine(pItem, false);
	pItem->Delete();
	return true;
}

// A timer expired for an item we are carrying
// RETURN:
//	false = delete it
bool CChar::OnTickEquip(CItem *pItem)
{
	ADDTOCALLSTACK("CChar::OnTickEquip");
	if ( !pItem )
		return false;

	switch ( pItem->GetEquipLayer() )
	{
		case LAYER_FLAG_Wool:
		{
			// Regen sheep wool
			if ( GetID() == CREID_Sheep_Sheered )
				SetID(CREID_Sheep);
			return false;
		}
		case LAYER_FLAG_ClientLinger:
		{
			// Remove me from other clients screens
			SetDisconnected();
			return false;
		}
		case LAYER_SPECIAL:
		{
			switch ( pItem->GetType() )
			{
				case IT_EQ_SCRIPT:	// pure script
					break;
				case IT_EQ_MEMORY_OBJ:
				{
					CItemMemory *pMemory = dynamic_cast<CItemMemory *>(pItem);
					if ( pMemory )
						return Memory_OnTick(pMemory);
					return false;
				}
				default:
					break;
			}
			break;
		}
		case LAYER_HORSE:
		{
			// Give my horse a tick (it still in the game!)
			CChar *pHorse = pItem->m_itFigurine.m_UID.CharFind();
			if ( !pHorse )
				return false;
			if ( pHorse != this )			// some scripts can force mounts to have as 'mount' the rider itself (like old ethereal scripts)
				return pHorse->OnTick();	// if we call OnTick() again on them we'll have an infinite loop
			pItem->SetTimeout(TICK_PER_SEC);
			return true;
		}
		case LAYER_FLAG_Murders:
		{
			// Decay the murder count
			if ( !m_pPlayer || (m_pPlayer->m_wMurders <= 0) )
				return false;

			CScriptTriggerArgs args;
			args.m_iN1 = m_pPlayer->m_wMurders - 1;
			args.m_iN2 = g_Cfg.m_iMurderDecayTime;

			if ( IsTrigUsed(TRIGGER_MURDERDECAY) )
			{
				OnTrigger(CTRIG_MurderDecay, this, &args);
				if ( args.m_iN1 < 0 )
					args.m_iN1 = 0;
				if ( args.m_iN2 < 1 )
					args.m_iN2 = g_Cfg.m_iMurderDecayTime;
			}

			m_pPlayer->m_wMurders = static_cast<WORD>(args.m_iN1);
			NotoSave_Update();
			if ( m_pPlayer->m_wMurders == 0 )
				return false;
			pItem->SetTimeout(args.m_iN2);
			return true;
		}
		default:
			break;
	}

	if ( pItem->IsType(IT_SPELL) )
		return Spell_Equip_OnTick(pItem);

	return pItem->OnTick();
}

// Cure an poisoned char
// ARGS:
//	iSkill = 0-1000
bool CChar::SetPoisonCure(int iSkill, bool fExtra)
{
	ADDTOCALLSTACK("CChar::SetPoisonCure");
	UNREFERENCED_PARAMETER(iSkill);

	CItem *pPoison = LayerFind(LAYER_FLAG_Poison);
	if ( pPoison )
		pPoison->Delete();

	if ( fExtra )
	{
		pPoison = LayerFind(LAYER_FLAG_Hallucination);
		if ( pPoison )
			pPoison->Delete();
	}

	UpdateModeFlag();
	return true;
}

// Char is getting poisoned
// ARGS:
//	iSkill = how bad the poison is (0-1000)
//	iTicks = how long to last (if MAGIFC_OSIFORMULAS is enabled it will be overrided by the formula result)
bool CChar::SetPoison(int iSkill, int iTicks, CChar *pCharSrc)
{
	ADDTOCALLSTACK("CChar::SetPoison");

	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(SPELL_Poison);
	if ( !pSpellDef )
		return false;

	if ( !pSpellDef->IsSpellType(SPELLFLAG_NOUNPARALYZE) )
	{
		CItem *pParalyze = LayerFind(LAYER_SPELL_Paralyze);
		if ( pParalyze )
			pParalyze->Delete();
	}

	CItem *pPoison = Spell_Effect_Create(SPELL_Poison, LAYER_FLAG_Poison, iSkill, 1 + Calc_GetRandVal(2) * TICK_PER_SEC, pCharSrc, false);
	if ( !pPoison )
		return false;
	LayerAdd(pPoison, LAYER_FLAG_Poison);

	if ( IsSetMagicFlags(MAGICF_OSIFORMULAS) )
	{
		int iMaxHits = Stat_GetMax(STAT_STR);
		if ( iSkill >= 1000 )
		{
			if ( (GetDist(pCharSrc) < 3) && !Calc_GetRandVal(10) )
			{
				// Lethal poison
				pPoison->m_itSpell.m_pattern = static_cast<BYTE>(IMULDIV(iMaxHits, Calc_GetRandVal(16, 33), 100));
				pPoison->m_itSpell.m_spelllevel = 4;
				pPoison->m_itSpell.m_spellcharges = 80;		//1 min, 20 sec
				pPoison->SetTimeout(50);
			}
			else
			{
				// Deadly poison
				pPoison->m_itSpell.m_pattern = static_cast<BYTE>(IMULDIV(iMaxHits, Calc_GetRandVal(15, 30), 100));
				pPoison->m_itSpell.m_spelllevel = 3;
				pPoison->m_itSpell.m_spellcharges = 60;
				pPoison->SetTimeout(50);
			}
		}
		else if ( iSkill >= 850 )
		{
			// Greater poison
			pPoison->m_itSpell.m_pattern = static_cast<BYTE>(IMULDIV(iMaxHits, Calc_GetRandVal(7, 15), 100));
			pPoison->m_itSpell.m_spelllevel = 2;
			pPoison->m_itSpell.m_spellcharges = 60;
			pPoison->SetTimeout(40);
		}
		else if ( iSkill >= 600 )
		{
			// Poison
			pPoison->m_itSpell.m_pattern = static_cast<BYTE>(IMULDIV(iMaxHits, Calc_GetRandVal(5, 10), 100));
			pPoison->m_itSpell.m_spelllevel = 1;
			pPoison->m_itSpell.m_spellcharges = 30;
			pPoison->SetTimeout(30);
		}
		else
		{
			// Lesser poison
			pPoison->m_itSpell.m_spelllevel = 0;
			pPoison->m_itSpell.m_spellcharges = 30;
			pPoison->SetTimeout(20);
		}

		if ( iTicks > 0 )
			pPoison->m_itSpell.m_spellcharges = iTicks;
	}
	else
	{
		pPoison->m_itSpell.m_spellcharges = iTicks;		// effect duration
		pPoison->SetTimeout((5 + Calc_GetRandLLVal(4)) * TICK_PER_SEC);
	}

	if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B )
	{
		CItem *pEvilOmen = LayerFind(LAYER_SPELL_Evil_Omen);
		if ( pEvilOmen )
		{
			++pPoison->m_itSpell.m_spelllevel;		// Effect 2: next poison will have one additional level of poison.
			pEvilOmen->Delete();
		}
	}

	if ( m_pClient && IsSetOF(OF_Buffs) )
	{
		m_pClient->removeBuff(BI_POISON);
		m_pClient->addBuff(BI_POISON, 1017383, 1070722, static_cast<WORD>(pPoison->m_itSpell.m_spellcharges));
	}

	SysMessageDefault(DEFMSG_JUST_BEEN_POISONED);
	StatFlag_Set(STATF_Poisoned);
	UpdateModeFlag();
	return true;
}

// Create char corpse when it dies
CItemCorpse *CChar::MakeCorpse(bool fFrontFall)
{
	ADDTOCALLSTACK("CChar::MakeCorpse");

	WORD wFlags = static_cast<WORD>(m_TagDefs.GetKeyNum("DEATHFLAGS"));
	if ( wFlags & DEATH_NOCORPSE )
		return NULL;
	if ( IsStatFlag(STATF_Conjured) && !(wFlags & (DEATH_NOCONJUREDEFFECT|DEATH_HASCORPSE)) )	// summoned creatures have no corpse
	{
		Effect(EFFECT_XYZ, ITEMID_FX_SPELL_FAIL, this, 1, 30);
		return NULL;
	}

	CItemCorpse *pCorpse = dynamic_cast<CItemCorpse *>(CItem::CreateScript(ITEMID_CORPSE, this));
	if ( !pCorpse )		// weird internal error
		return NULL;

	TCHAR szName[MAX_ITEM_NAME_SIZE];
	snprintf(szName, sizeof(szName), g_Cfg.GetDefaultMsg(m_pPlayer ? DEFMSG_MSG_CORPSE_PLAYER : DEFMSG_MSG_CORPSE_NPC), GetName());

	pCorpse->SetName(szName);
	pCorpse->SetHue(GetHue());
	pCorpse->SetAttr(ATTR_MOVE_NEVER);
	pCorpse->SetCorpseType(GetDispID());
	pCorpse->SetTimeStamp(CServTime::GetCurrentTime().GetTimeRaw());	// death timestamp
	pCorpse->m_uidLink = GetUID();
	pCorpse->m_ModMaxWeight = g_Cfg.Calc_MaxCarryWeight(this);		// set corpse maxweight to prevent weight exploit when someone place many items on an player corpse just to make this player get stuck on resurrect

	if ( m_pNPC && (m_pNPC->m_bonded || IsStatFlag(STATF_Conjured)) )
		pCorpse->m_itCorpse.m_carved = 1;	// corpse of bonded/summoned creatures can't be carved

	CChar *pKiller = Attacker_GetLowestElapsed();
	if ( pKiller )
		pCorpse->m_itCorpse.m_uidKiller = pKiller->GetUID();
	else
		pCorpse->m_itCorpse.m_uidKiller.InitUID();

	pCorpse->m_itCorpse.m_BaseID = m_prev_id;
	pCorpse->m_itCorpse.m_facing_dir = fFrontFall ? static_cast<DIR_TYPE>(m_dirFace|DIR_MASK_RUNNING) : m_dirFace;

	// Move char contents to the corpse on ground
	if ( !(wFlags & DEATH_NOLOOTDROP) )	
		DropAll(pCorpse);

	pCorpse->MoveToDecay(GetTopPoint(), m_pPlayer ? g_Cfg.m_iDecay_CorpsePlayer : g_Cfg.m_iDecay_CorpseNPC);
	return pCorpse;
}

// Raise the corpse back to the char
bool CChar::RaiseCorpse(CItemCorpse *pCorpse)
{
	ADDTOCALLSTACK("CChar::RaiseCorpse");

	if ( !pCorpse )
		return false;

	if ( pCorpse->GetCount() > 0 )
	{
		// Move corpse contents to the char
		CItemContainer *pPack = GetContainerCreate(LAYER_PACK);
		CItem *pItemNext = NULL;
		for ( CItem *pItem = pCorpse->GetContentHead(); pItem != NULL; pItem = pItemNext )
		{
			pItemNext = pItem->GetNext();
			if ( pItem->IsType(IT_HAIR) || pItem->IsType(IT_BEARD) )	// hair on corpse was copied!
				continue;

			if ( pItem->GetContainedLayer() )
				ItemEquip(pItem);
			else if ( pPack )
				pPack->ContentAdd(pItem);
		}
		pCorpse->ContentsDump(GetTopPoint());		// drop left items on ground
	}

	UpdateAnimate((pCorpse->m_itCorpse.m_facing_dir & DIR_MASK_RUNNING) ? ANIM_DIE_FORWARD : ANIM_DIE_BACK, true, true);
	pCorpse->Delete();
	return true;
}

// The char died
// RETURN: 
//	true = successfully died
//	false = something went wrong
bool CChar::Death()
{
	ADDTOCALLSTACK("CChar::Death");

	if ( IsStatFlag(STATF_DEAD|STATF_INVUL) )
		return true;

	if ( IsTrigUsed(TRIGGER_DEATH) )
	{
		if ( OnTrigger(CTRIG_Death, this) == TRIGRET_RET_TRUE )
			return true;
	}

	// Look through memories of who I was fighting (make sure they knew they where fighting me)
	CItem *pItemNext = NULL;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( pItem->IsType(IT_EQ_TRADE_WINDOW) )
		{
			CItemContainer *pCont = dynamic_cast<CItemContainer *>(pItem);
			if ( pCont )
			{
				pCont->Trade_Delete();
				continue;
			}
		}

		if ( pItem->IsMemoryTypes(MEMORY_HARMEDBY) )
			Memory_ClearTypes(static_cast<CItemMemory *>(pItem), 0xFFFF & ~MEMORY_IPET);
	}

	// Give kill credit to my attackers
	int iKillers = 0;
	CChar *pKiller = NULL;
	TCHAR szMsg[THREAD_STRING_LENGTH];
	size_t len = snprintf(szMsg, sizeof(szMsg), g_Cfg.GetDefaultMsg(DEFMSG_MSG_KILLED_BY), m_pPlayer ? 'P' : 'N', GetNameWithoutIncognito());

	for ( size_t i = 0; i < m_lastAttackers.size(); ++i )
	{
		LastAttackers &refAttacker = m_lastAttackers.at(i);
		pKiller = refAttacker.charUID.CharFind();
		if ( pKiller && (refAttacker.damage > 0) )
		{
			if ( IsTrigUsed(TRIGGER_KILL) )
			{
				CScriptTriggerArgs args(this);
				args.m_iN1 = static_cast<INT64>(m_lastAttackers.size());
				if ( pKiller->OnTrigger(CTRIG_Kill, pKiller, &args) == TRIGRET_RET_TRUE )
					continue;
			}

			pKiller->Noto_Kill(this, IsStatFlag(STATF_Pet), static_cast<int>(m_lastAttackers.size()));
			if ( len < sizeof(szMsg) )
				len += snprintf(szMsg + len, sizeof(szMsg) - len, "%s%c'%s'", iKillers ? ", " : "", pKiller->m_pPlayer ? 'P' : 'N', pKiller->GetNameWithoutIncognito());
			++iKillers;
		}
	}

	// Record the kill event for posterity
	if ( !iKillers )
		strncat(szMsg, "accident", sizeof(szMsg) - 1);
	if ( m_pPlayer )
		g_Log.Event(LOGM_KILLS, "%s\n", szMsg);
	if ( m_pParty )
		m_pParty->SysMessageAll(szMsg);

	Reveal();
	SoundChar(CRESND_DIE);
	StatFlag_Set(STATF_Insubstantial|STATF_DEAD);
	StatFlag_Clear(STATF_Hidden|STATF_Hovering|STATF_Stone|STATF_War|STATF_FreezeCast|STATF_Freeze);
	SetPoisonCure(0, true);
	Skill_Cleanup();
	Spell_Dispel(100);		// get rid of all spell effects (moved here to prevent double @Destroy trigger)

	if ( m_pPlayer )		// if I'm NPC then my mount goes with me
		Horse_UnMount();

	// Create the corpse item
	bool fFrontFall = (IsStatFlag(STATF_Fly) || Calc_GetRandVal(2));
	CItemCorpse *pCorpse = MakeCorpse(fFrontFall);
	if ( pCorpse )
	{
		if ( IsTrigUsed(TRIGGER_DEATHCORPSE) )
		{
			CScriptTriggerArgs Args(pCorpse);
			OnTrigger(CTRIG_DeathCorpse, this, &Args);
		}
	}
	m_lastAttackers.clear();	// clear list of attackers

	// Play death animation (fall on ground)
	UpdateCanSee(new PacketDeath(this, pCorpse, fFrontFall), m_pClient);

	if ( m_pNPC )
	{
		if ( m_pNPC->m_bonded && NPC_PetGetOwner() )
		{
			m_Can |= CAN_C_GHOST;
			Update();
			return true;
		}

		if ( pCorpse )
			pCorpse->m_uidLink.InitUID();

		NPC_PetClearOwners();
		return false;	// delete the NPC
	}

	if ( m_pPlayer )
	{
		ChangeExperience(-(static_cast<int>(m_exp) / 10), pKiller);
		if ( !(m_TagDefs.GetKeyNum("DEATHFLAGS") & DEATH_NOFAMECHANGE) )
			Noto_Fame(-Stat_GetAdjusted(STAT_FAME) / 10);

		LPCTSTR pszGhostName = NULL;
		CCharBase *pCharDefPrev = CCharBase::FindCharBase(m_prev_id);
		switch ( m_prev_id )
		{
			case CREID_GARGMAN:
			case CREID_GARGWOMAN:
				pszGhostName = (pCharDefPrev && pCharDefPrev->IsFemale()) ? "c_garg_ghost_woman" : "c_garg_ghost_man";
				break;
			case CREID_ELFMAN:
			case CREID_ELFWOMAN:
				pszGhostName = (pCharDefPrev && pCharDefPrev->IsFemale()) ? "c_elf_ghost_woman" : "c_elf_ghost_man";
				break;
			default:
				pszGhostName = (pCharDefPrev && pCharDefPrev->IsFemale()) ? "c_ghost_woman" : "c_ghost_man";
				break;
		}

		++m_pPlayer->m_wDeaths;
		SetHue(HUE_DEFAULT);	// get all pale
		SetID(static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType(RES_CHARDEF, pszGhostName)));
		LayerAdd(CItem::CreateScript(ITEMID_DEATHSHROUD, this));

		if ( m_pClient )
		{
			if ( g_Cfg.m_iPacketDeathAnimation || m_pClient->m_NetState->isClientKR() || m_pClient->m_NetState->isClientEnhanced() )
			{
				// Display the "You are dead" screen animation (this must be always enabled on enhanced clients)
				new PacketDeathMenu(m_pClient, DEATH_MODE_MANIFEST);
			}
			else
			{
				// OSI uses PacketDeathMenu to update client screen on death. If this packet is disabled,
				// the client must be updated manually using these others packets as workaround
				m_pClient->addPlayerUpdate();
			}

			m_pClient->addPlayerWarMode();
			m_pClient->addSeason(SEASON_Desolate);
			m_pClient->addMapWaypoint(pCorpse, MAPWAYPOINT_Corpse);		// add corpse map waypoint on enhanced clients

			// Force client to update backpack content
			CItem *pPack = LayerFind(LAYER_PACK);
			if ( pPack )
			{
				pPack->RemoveFromView();
				pPack->Update();
			}

			// Remove from screen the characters which I can't see as dead
			if ( g_Cfg.m_fDeadCannotSeeLiving )
			{
				CWorldSearch AreaChars(GetTopPoint(), GetSight());
				for (;;)
				{
					CChar *pChar = AreaChars.GetChar();
					if ( !pChar )
						break;
					if ( !CanSeeAsDead(pChar) )
						m_pClient->addObjectRemove(pChar);
				}
			}
		}
	}
	return true;
}

// Check if char is stuck
bool CChar::OnFreezeCheck()
{
	ADDTOCALLSTACK("CChar::OnFreezeCheck");

	if ( IsStatFlag(STATF_Freeze|STATF_FreezeCast|STATF_Stone) && !IsPriv(PRIV_GM) )
		return true;
	if ( static_cast<UINT64>(GetKeyNum("NoMoveTill")) > g_World.GetCurrentTime().GetTimeRaw() )
		return true;

	return false;
}

// Toggle gargoyle flying mode
void CChar::ToggleFlying()
{
	ADDTOCALLSTACK("CChar::ToggleFlying");

	if ( IsTrigUsed(TRIGGER_TOGGLEFLYING) )
	{
		if ( OnTrigger(CTRIG_ToggleFlying, this) == TRIGRET_RET_TRUE )
			return;
	}

	if ( IsStatFlag(STATF_Hovering) )
	{
		// Stop hovering
		StatFlag_Clear(STATF_Hovering);
		if ( m_pClient )
			m_pClient->removeBuff(BI_GARGOYLEFLY);
	}
	else
	{
		// Begin hovering
		StatFlag_Set(STATF_Hovering);
		if ( m_pClient )
			m_pClient->addBuff(BI_GARGOYLEFLY, 1112193, 1112567);

		// Float char up to the hover Z
		CPointMap ptHover = g_World.FindItemTypeNearby(GetTopPoint(), IT_HOVEROVER, 0);
		if ( ptHover.IsValidPoint() )
			MoveTo(ptHover);
	}

	// NANIM_TAKEOFF and NANIM_LANDING animations are only available on the new animation packet (PacketAnimationBasic),
	// so we must use it instead the old PacketAnimation which will translate these values into an wrong animation
	PacketActionNew *cmd = new PacketActionNew(this, IsStatFlag(STATF_Hovering) ? NANIM_TAKEOFF : NANIM_LANDING, static_cast<ANIM_TYPE_NEW>(0), static_cast<BYTE>(0));
	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( !PacketActionNew::CanSendTo(pClient->m_NetState) || !pClient->CanSee(this) )
			continue;
		pClient->addCharMove(this);
		cmd->send(pClient);
	}
	delete cmd;
}

// Flip char direction
void CChar::Flip()
{
	ADDTOCALLSTACK("CChar::Flip");
	UpdateDir(GetDirTurn(m_dirFace, 1));
}

// Check if char can walk to given pt
// RETURN:
//	ptDst.m_z = the new z
//	NULL = failed to walk here
CRegionBase *CChar::CanMoveWalkTo(CPointBase &ptDst, bool fCheckChars, bool fCheckOnly, DIR_TYPE dir, bool fPathFinding)
{
	ADDTOCALLSTACK("CChar::CanMoveWalkTo");

	if ( Can(CAN_C_NONMOVER) )
		return NULL;

	int iWeight = 0;
	int iMaxWeight = 0;
	if ( !IsPriv(PRIV_GM) )
	{
		iWeight = GetTotalWeight() / WEIGHT_UNITS;
		iMaxWeight = g_Cfg.Calc_MaxCarryWeight(this) / WEIGHT_UNITS;
	}

	if ( !fCheckOnly )
	{
		if ( OnFreezeCheck() )
		{
			SysMessageDefault(DEFMSG_MSG_FROZEN);
			return NULL;
		}
		else if ( (Stat_GetVal(STAT_DEX) <= 0) && !IsStatFlag(STATF_DEAD) )
		{
			SysMessageDefault((iWeight > iMaxWeight) ? DEFMSG_MSG_FATIGUE_WEIGHT : DEFMSG_MSG_FATIGUE);
			return NULL;
		}
	}

	if ( m_pClient && m_pClient->m_pHouseDesign )
	{
		if ( m_pClient->m_pHouseDesign->GetDesignArea().IsInside2d(ptDst) )
		{
			ptDst.m_z = GetTopZ();
			return ptDst.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA);
		}
		return NULL;
	}

	DWORD dwBlockFlags = 0;
	height_t ClimbHeight = 0;
	CRegionBase *pArea = NULL;

	EXC_TRY("CanMoveWalkTo");

	EXC_SET("Check valid move");
	pArea = CheckValidMove(ptDst, &dwBlockFlags, static_cast<DIR_TYPE>(dir & ~DIR_MASK_RUNNING), &ClimbHeight, fPathFinding);
	if ( !pArea )
		return NULL;

	EXC_SET("NPC's will");
	if ( !fCheckOnly && m_pNPC && !NPC_CheckWalkHere(ptDst, pArea) )	// does the NPC want to walk here?
		return NULL;

	EXC_SET("Creature bumping");
	int iStamReq = 0;
	if ( fCheckChars && !IsStatFlag(STATF_DEAD|STATF_Insubstantial) )
	{
		CItem *pPoly = LayerFind(LAYER_SPELL_Polymorph);
		CWorldSearch AreaChars(ptDst);
		for (;;)
		{
			CChar *pChar = AreaChars.GetChar();
			if ( !pChar )
				break;
			if ( (pChar == this) || (abs(pChar->GetTopZ() - ptDst.m_z) > 5) || pChar->IsStatFlag(STATF_Insubstantial) )
				continue;
			if ( m_pNPC && pChar->m_pNPC )	// NPCs can't walk over another NPC
				return NULL;

			iStamReq = 10;
			if ( IsPriv(PRIV_GM) || pChar->IsStatFlag(STATF_DEAD|STATF_Invisible|STATF_Hidden) )
				iStamReq = 0;
			else if ( pPoly && (pPoly->m_itSpell.m_spell == SPELL_Wraith_Form) )		// chars under Wraith Form effect can always walk through chars
				iStamReq = 0;

			TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;
			if ( IsTrigUsed(TRIGGER_PERSONALSPACE) )
			{
				CScriptTriggerArgs Args(iStamReq);
				iRet = pChar->OnTrigger(CTRIG_PersonalSpace, this, &Args);
				if ( iRet == TRIGRET_RET_TRUE )
					return NULL;

				iStamReq = static_cast<int>(Args.m_iN1);
				if ( iStamReq < 0 )
					continue;
			}

			if ( (iStamReq > 0) && (Stat_GetVal(STAT_DEX) < Stat_GetMax(STAT_DEX)) )
				return NULL;

			if ( Stat_GetVal(STAT_DEX) < iStamReq )		// check if we have enough stamina to push the char
			{
				SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_CANTPUSH), pChar->GetName());
				return NULL;
			}
			else if ( pChar->IsStatFlag(STATF_Invisible|STATF_Hidden) )
			{
				pChar->Reveal(STATF_Invisible|STATF_Hidden);
				if ( iRet != TRIGRET_RET_FALSE )
					SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_HIDING_STUMBLE), pChar->GetName());
			}
			else if ( iRet != TRIGRET_RET_FALSE )
				SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_PUSH), pChar->GetName());

			break;
		}
	}

	if ( !fCheckOnly )
	{
		EXC_SET("Stamina penalty");
		if ( iWeight > iMaxWeight )
		{
			int iWeightPenalty = g_Cfg.m_iStaminaLossAtWeight + ((iWeight - iMaxWeight) / 25);

			if ( IsStatFlag(STATF_OnHorse) )
				iWeightPenalty /= 3;

			if ( dir & DIR_MASK_RUNNING )
				iWeightPenalty += (iWeightPenalty * g_Cfg.m_iStamRunningPenalty) / 100;

			iStamReq += iWeightPenalty;
		}

		if ( iStamReq > 0 )
			UpdateStatVal(STAT_DEX, -iStamReq);

		StatFlag_Mod(STATF_InDoors, (dwBlockFlags & CAN_I_ROOF) || pArea->IsFlag(REGION_FLAG_UNDERGROUND));
		m_zClimbHeight = (dwBlockFlags & CAN_I_CLIMB) ? ClimbHeight : 0;
	}

	EXC_CATCH;
	return pArea;
}

void CChar::CheckRevealOnMove()
{
	ADDTOCALLSTACK("CChar::CheckRevealOnMove");

	if ( !IsStatFlag(STATF_Invisible|STATF_Hidden) )
		return;

	if ( IsTrigUsed(TRIGGER_STEPSTEALTH) )
		OnTrigger(CTRIG_StepStealth, this);

	m_StepStealth -= IsStatFlag(STATF_Fly|STATF_Hovering) ? 2 : 1;
	if ( m_StepStealth <= 0 )
		Reveal();
}

// Periodically check if the char is stepping on something
// This function is called at every second on ALL chars (walking or not) so avoid heavy codes here
// RETURN:
//	true = we can move there
//	false = we can't move there
//	default = we teleported
TRIGRET_TYPE CChar::CheckLocation(bool fStanding)
{
	ADDTOCALLSTACK("CChar::CheckLocation");

	if ( m_pClient && m_pClient->m_pHouseDesign )	// stepping on items while in house design mode doesn't trigger anything
		return TRIGRET_RET_TRUE;

	if ( !fStanding )
	{
		if ( g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_IMMOBILE) )
			Skill_Fail();

		if ( IsTrigUsed(TRIGGER_STEP) )
		{
			if ( m_pArea->OnRegionTrigger(this, RTRIG_STEP) == TRIGRET_RET_TRUE )
				return TRIGRET_RET_FALSE;

			CRegionBase *pRoom = GetTopPoint().GetRegion(REGION_TYPE_ROOM);
			if ( pRoom && pRoom->OnRegionTrigger(this, RTRIG_STEP) == TRIGRET_RET_TRUE )
				return TRIGRET_RET_FALSE;
		}
	}

	bool fStepCancel = false;
	bool fSpellHit = false;
	CWorldSearch AreaItems(GetTopPoint());
	for (;;)
	{
		CItem *pItem = AreaItems.GetItem();
		if ( !pItem )
			break;

		int zdiff = pItem->GetTopZ() - GetTopZ();
		int	height = pItem->Item_GetDef()->GetHeight();
		if ( height < 3 )
			height = 3;

		if ( (zdiff > height) || (zdiff < -3) )
			continue;
		if ( IsTrigUsed(TRIGGER_STEP) || IsTrigUsed(TRIGGER_ITEMSTEP) )
		{
			CScriptTriggerArgs Args(fStanding ? 1 : 0);
			TRIGRET_TYPE iRet = pItem->OnTrigger(ITRIG_STEP, this, &Args);
			if ( iRet == TRIGRET_RET_TRUE )		// block walk
			{
				fStepCancel = true;
				continue;
			}
			if ( iRet == TRIGRET_RET_HALFBAKED )	// allow walk, skipping hardcoded checks below
				continue;
		}

		switch ( pItem->GetType() )
		{
			case IT_WEB:
			{
				if ( fStanding )
					continue;
				if ( Use_Item_Web(pItem) )	// we got stuck in a spider web
					return TRIGRET_RET_FALSE;
				continue;
			}
			case IT_FIRE:
			{
				int iSkillLevel = pItem->m_itSpell.m_spelllevel;	// heat level (0-1000)
				iSkillLevel = Calc_GetRandVal(iSkillLevel / 2, iSkillLevel);
				if ( IsStatFlag(STATF_Fly) )
					iSkillLevel /= 2;

				if ( OnTakeDamage(g_Cfg.GetSpellEffect(SPELL_Fire_Field, iSkillLevel), NULL, DAMAGE_FIRE|DAMAGE_GENERAL, 0, 100, 0, 0, 0) != -1 )
				{
					Sound(SOUND_FLAMESTRIKE);
					if ( m_pNPC && fStanding )
					{
						m_Act_p = GetTopPoint();
						m_Act_p.Move(static_cast<DIR_TYPE>(Calc_GetRandVal(DIR_QTY)));
						NPC_WalkToPoint(true);		// run away from the threat
					}
				}
				continue;
			}
			case IT_SPELL:
			{
				// Workaround: only hit 1 spell on each loop. If we hit all spells (eg: multiple field spells)
				// it will allow weird exploits like cast many Fire Fields on the same spot to take more damage,
				// or Paralyze Field + Fire Field to make the target get stuck forever being damaged with no way
				// to get out of the field, because the damage won't allow cast any spell and the Paralyze Field
				// will immediately paralyze again with 0ms delay at each damage tick.
				// On OSI if the player cast multiple fields on the same tile, it will remove the previous field
				// tile that got overlapped. But Sphere doesn't use this method, so this workaround is needed.
				if ( fSpellHit )
					continue;

				if ( OnSpellEffect(static_cast<SPELL_TYPE>(RES_GET_INDEX(pItem->m_itSpell.m_spell)), pItem->m_uidLink.CharFind(), static_cast<int>(pItem->m_itSpell.m_spelllevel), pItem) )
				{
					fSpellHit = true;
					if ( m_pNPC && fStanding )
					{
						m_Act_p = GetTopPoint();
						m_Act_p.Move(static_cast<DIR_TYPE>(Calc_GetRandVal(DIR_QTY)));
						NPC_WalkToPoint(true);		// run away from the threat
					}
				}
				continue;
			}
			case IT_TRAP:
			case IT_TRAP_ACTIVE:
			{
				if ( OnTakeDamage(pItem->Use_Trap(), NULL, DAMAGE_HIT_BLUNT|DAMAGE_GENERAL) )
				{
					if ( m_pNPC && fStanding )
					{
						m_Act_p = GetTopPoint();
						m_Act_p.Move(static_cast<DIR_TYPE>(Calc_GetRandVal(DIR_QTY)));
						NPC_WalkToPoint(true);		// run away from the threat
					}
				}
				continue;
			}
			case IT_SWITCH:
			{
				if ( pItem->m_itSwitch.m_fStep )
					Use_Item(pItem);
				continue;
			}
			case IT_MOONGATE:
			case IT_TELEPAD:
			{
				if ( fStanding )
					continue;
				if ( Use_MoonGate(pItem) )
					return TRIGRET_RET_DEFAULT;
				continue;
			}
			case IT_SHIP_PLANK:
			case IT_ROPE:
			{
				if ( !fStanding && !IsStatFlag(STATF_Hovering) )
				{
					// Check if we can go out of the ship (in the same direction of plank)
					if ( MoveToValidSpot(m_dirFace, g_Cfg.m_iMaxShipPlankTeleport, 1, true) )
					{
						//pItem->SetTimeout(5 * TICK_PER_SEC);	// autoclose the plank behind us
						return TRIGRET_RET_TRUE;
					}
				}
				continue;
			}
			default:
				continue;
		}
	}

	if ( fStanding || fStepCancel )
		return TRIGRET_RET_FALSE;

	// Check if there's any map teleporter here
	const CPointMap &pt = GetTopPoint();
	CSector *pSector = pt.GetSector();
	if ( !pSector )
		return TRIGRET_RET_FALSE;

	const CTeleport *pTeleport = pSector->GetTeleport(pt);
	if ( !pTeleport )
		return TRIGRET_RET_TRUE;

	if ( m_pNPC )
	{
		if ( !pTeleport->m_fNPC )
			return TRIGRET_RET_FALSE;

		if ( m_pNPC->m_Brain == NPCBRAIN_GUARD )
		{
			// Guards won't teleport to unguarded areas
			CRegionWorld *pArea = dynamic_cast<CRegionWorld *>(pTeleport->m_ptDst.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA));
			if ( !pArea || !pArea->IsGuarded() )
				return TRIGRET_RET_FALSE;
		}
		if ( Noto_IsCriminal() )
		{
			// Criminals won't teleport to guarded areas
			CRegionWorld *pArea = dynamic_cast<CRegionWorld *>(pTeleport->m_ptDst.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA));
			if ( !pArea || pArea->IsGuarded() )
				return TRIGRET_RET_FALSE;
		}
	}
	Spell_Teleport(pTeleport->m_ptDst, true, false, false);
	return TRIGRET_RET_DEFAULT;
}

// Char is moving to a new region, or logging out (not in any region, pNewArea == NULL)
bool CChar::MoveToRegion(CRegionWorld *pNewArea, bool fAllowReject)
{
	ADDTOCALLSTACK("CChar::MoveToRegion");
	if ( m_pArea == pNewArea )
		return true;

	if ( !g_Serv.IsLoading() )
	{
		if ( fAllowReject && IsPriv(PRIV_GM) )
			fAllowReject = false;

		if ( m_pArea )
		{
			if ( IsTrigUsed(TRIGGER_EXIT) )
			{
				if ( m_pArea->OnRegionTrigger(this, RTRIG_EXIT) == TRIGRET_RET_TRUE )
				{
					if ( pNewArea && fAllowReject )
						return false;
				}
			}
			if ( IsTrigUsed(TRIGGER_REGIONLEAVE) )
			{
				CScriptTriggerArgs Args(m_pArea);
				if ( OnTrigger(CTRIG_RegionLeave, this, &Args) == TRIGRET_RET_TRUE )
				{
					if ( pNewArea && fAllowReject )
						return false;
				}
			}
		}

		if ( m_pClient && pNewArea )
		{
			if ( pNewArea->IsFlag(REGION_FLAG_ANNOUNCE) && !pNewArea->IsInside2d(GetTopPoint()) )
			{
				CVarDefContStr *pVarStr = dynamic_cast<CVarDefContStr *>(pNewArea->m_TagDefs.GetKey("ANNOUNCEMENT"));
				SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_REGION_ENTER), pVarStr ? pVarStr->GetValStr() : pNewArea->GetName());
			}
			else if ( m_pArea && !IsStatFlag(STATF_DEAD) )
			{
				if ( pNewArea->IsGuarded() != m_pArea->IsGuarded() )
				{
					if ( pNewArea->IsGuarded() )
					{
						CVarDefContStr *pVarStr = dynamic_cast<CVarDefContStr *>(pNewArea->m_TagDefs.GetKey("GUARDOWNER"));
						SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_REGION_GUARDS_1), pVarStr ? pVarStr->GetValStr() : g_Cfg.GetDefaultMsg(DEFMSG_MSG_REGION_GUARD_ART));
					}
					else
					{
						CVarDefContStr *pVarStr = dynamic_cast<CVarDefContStr *>(m_pArea->m_TagDefs.GetKey("GUARDOWNER"));
						SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_REGION_GUARDS_2), pVarStr ? pVarStr->GetValStr() : g_Cfg.GetDefaultMsg(DEFMSG_MSG_REGION_GUARD_ART));
					}
				}

				bool fRedRegionOld = (m_pArea->m_TagDefs.GetKeyNum("RED") != 0);
				bool fRedRegionNew = (pNewArea->m_TagDefs.GetKeyNum("RED") != 0);
				if ( fRedRegionOld != fRedRegionNew )
					SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_REGION_REDDEF), g_Cfg.GetDefaultMsg(fRedRegionNew ? DEFMSG_MSG_REGION_REDENTER : DEFMSG_MSG_REGION_REDLEFT));

				if ( pNewArea->IsFlag(REGION_FLAG_NO_PVP) != m_pArea->IsFlag(REGION_FLAG_NO_PVP) )
					SysMessageDefault((pNewArea->IsFlag(REGION_FLAG_NO_PVP)) ? DEFMSG_MSG_REGION_PVPSAFE : DEFMSG_MSG_REGION_PVPNOT);

				if ( pNewArea->IsFlag(REGION_FLAG_SAFE) != m_pArea->IsFlag(REGION_FLAG_SAFE) )
					SysMessageDefault((pNewArea->IsFlag(REGION_FLAG_SAFE)) ? DEFMSG_MSG_REGION_SAFETYGET : DEFMSG_MSG_REGION_SAFETYLOSE);
			}
		}

		if ( pNewArea )
		{
			if ( IsTrigUsed(TRIGGER_ENTER) )
			{
				if ( pNewArea->OnRegionTrigger(this, RTRIG_ENTER) == TRIGRET_RET_TRUE )
				{
					if ( m_pArea && fAllowReject )
						return false;
				}
			}
			if ( IsTrigUsed(TRIGGER_REGIONENTER) )
			{
				CScriptTriggerArgs Args(pNewArea);
				if ( OnTrigger(CTRIG_RegionEnter, this, &Args) == TRIGRET_RET_TRUE )
				{
					if ( m_pArea && fAllowReject )
						return false;
				}
			}
		}
	}

	m_pArea = pNewArea;
	return true;
}

// Char is moving to a new room
bool CChar::MoveToRoom(CRegionBase *pNewRoom, bool fAllowReject)
{
	ADDTOCALLSTACK("CChar::MoveToRoom");

	if ( m_pRoom == pNewRoom )
		return true;

	if ( !g_Serv.IsLoading() )
	{
		if ( fAllowReject && IsPriv(PRIV_GM) )
			fAllowReject = false;

		if ( m_pRoom )
		{
			if ( IsTrigUsed(TRIGGER_EXIT) )
			{
				if ( m_pRoom->OnRegionTrigger(this, RTRIG_EXIT) == TRIGRET_RET_TRUE )
				{
					if ( fAllowReject )
						return false;
				}
			}
			if ( IsTrigUsed(TRIGGER_REGIONLEAVE) )
			{
				CScriptTriggerArgs Args(m_pRoom);
				if ( OnTrigger(CTRIG_RegionLeave, this, &Args) == TRIGRET_RET_TRUE )
				{
					if ( fAllowReject )
						return false;
				}
			}
		}

		if ( pNewRoom )
		{
			if ( IsTrigUsed(TRIGGER_ENTER) )
			{
				if ( pNewRoom->OnRegionTrigger(this, RTRIG_ENTER) == TRIGRET_RET_TRUE )
				{
					if ( fAllowReject )
						return false;
				}
			}
			if ( IsTrigUsed(TRIGGER_REGIONENTER) )
			{
				CScriptTriggerArgs Args(pNewRoom);
				if ( OnTrigger(CTRIG_RegionEnter, this, &Args) == TRIGRET_RET_TRUE )
				{
					if ( fAllowReject )
						return false;
				}
			}
		}
	}

	m_pRoom = pNewRoom;
	return true;
}

// Same as MoveTo. The char is taking a step or being teleported
// Low level: DOES NOT update display or container flags (may be offline)
bool CChar::MoveToChar(CPointMap pt, bool fForceFix)
{
	ADDTOCALLSTACK("CChar::MoveToChar");

	if ( !pt.IsValidPoint() )
		return false;

	if ( m_pPlayer && !m_pClient )	// moving a logged out client
	{
		CSector *pSector = pt.GetSector();
		if ( !pSector )
			return false;

		// Can't put this char in non-disconnect state
		SetDisconnected();
		pSector->MoveDisconnectedCharToSector(this);
		SetUnkPoint(pt);
		return true;
	}

	CRegionWorld *pAreaNew = dynamic_cast<CRegionWorld *>(pt.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA));
	if ( !MoveToRegion(pAreaNew, true) )
		return false;

	CRegionBase *pRoomNew = pt.GetRegion(REGION_TYPE_ROOM);
	if ( !MoveToRoom(pRoomNew, true) )
		return false;

	CPointMap ptOld = GetTopPoint();
	bool fSectorChange = pt.GetSector()->MoveCharToSector(this);
	SetTopPoint(pt);

	if ( !m_fClimbUpdated || fForceFix )
		FixClimbHeight();

	if ( fSectorChange && !g_Serv.IsLoading() )
	{
		if ( IsTrigUsed(TRIGGER_ENVIRONCHANGE) )
		{
			CScriptTriggerArgs Args(ptOld.m_x, ptOld.m_y, ptOld.m_z << 16 | ptOld.m_map);
			OnTrigger(CTRIG_EnvironChange, this, &Args);
		}
	}
	return true;
}

// Move char from here to a valid spot (assume "here" is not a valid spot, even if it really is)
bool CChar::MoveToValidSpot(DIR_TYPE dir, int iDist, int iDistStart, bool fFromShip)
{
	ADDTOCALLSTACK("CChar::MoveToValidSpot");

	CPointMap pt = GetTopPoint();
	pt.MoveN(dir, iDistStart);
	pt.m_z += PLAYER_HEIGHT;
	signed char startZ = pt.m_z;

	DWORD dwCan = GetMoveBlockFlags(true);
	for ( int i = 0; i < iDist; ++i )
	{
		if ( pt.IsValidPoint() )
		{
			// Don't allow boarding of other ships (they may be locked)
			CRegionBase *pRegionBase = pt.GetRegion(REGION_TYPE_SHIP);
			if ( pRegionBase )
			{
				pt.Move(dir);
				continue;
			}

			// Reset Z back to start Z + PLAYER_HEIGHT so we don't climb buildings
			pt.m_z = startZ;

			// Set new Z so we don't end up floating or underground
			DWORD dwBlockFlags = dwCan;
			pt.m_z = g_World.GetHeightPoint(pt, dwBlockFlags, true);

			// Don't allow characters to pass through walls or other blocked paths when they're disembarking from a ship
			if ( fFromShip && (dwBlockFlags & CAN_I_BLOCK) && !(dwCan & CAN_C_PASSWALLS) && (pt.m_z > startZ) )
				break;

			if ( !(dwBlockFlags & ~dwCan) )
			{
				// We can go here
				if ( Spell_Teleport(pt, true, !fFromShip, false) )
					return true;
			}
		}
		pt.Move(dir);
	}
	return false;
}

// Set char priv level ("PRIVSET")
// NOTE: It can be offline at this time
bool CChar::SetPrivLevel(CTextConsole *pSrc, LPCTSTR pszFlags)
{
	ADDTOCALLSTACK("CChar::SetPrivLevel");

	if ( !m_pPlayer || !m_pPlayer->m_pAccount || !pszFlags[0] || (pSrc->GetPrivLevel() < PLEVEL_Admin) || (pSrc->GetPrivLevel() < GetPrivLevel()) )
		return false;

	PLEVEL_TYPE plevel = m_pPlayer->m_pAccount->GetPrivLevelText(pszFlags);
	m_pPlayer->m_pAccount->SetPrivLevel(plevel);

	if ( plevel >= PLEVEL_Counsel )
	{
		// Set GM status
		StatFlag_Set(STATF_INVUL);

		CItem *pRobe = ContentFind(RESOURCE_ID(RES_ITEMDEF, ITEMID_GM_ROBE));
		if ( !pRobe )
			pRobe = CItem::CreateScript(ITEMID_GM_ROBE, this);
		if ( pRobe )
		{
			pRobe->SetAttr(ATTR_NEWBIE|ATTR_MAGIC);
			pRobe->SetHue(static_cast<HUE_TYPE>((plevel >= PLEVEL_GM) ? HUE_RED : HUE_BLUE_NAVY));	// since sept/2014 OSI changed 'Counselor' plevel to 'Advisor', using GM Robe color 05f
			UnEquipAllItems();
			ItemEquip(pRobe);
		}
	}
	else
	{
		// Clear GM status
		StatFlag_Clear(STATF_INVUL);
		ContentConsume(RESOURCE_ID(RES_ITEMDEF, ITEMID_GM_ROBE));
	}
	NotoSave_Update();
	UpdatePropertyFlag();
	return true;
}

// Char fired an trigger
TRIGRET_TYPE CChar::OnTrigger(LPCTSTR pszTrigName, CTextConsole *pSrc, CScriptTriggerArgs *pArgs)
{
	ADDTOCALLSTACK("CChar::OnTrigger");

	if ( IsTriggerActive(pszTrigName) )		// this should protect any char trigger from infinite loop
		return TRIGRET_RET_DEFAULT;

	// Attach some trigger to the CChar
	// RETURN: true = block further action
	CCharBase *pCharDef = Char_GetDef();
	if ( !pCharDef )
		return TRIGRET_RET_DEFAULT;

	CTRIG_TYPE iAction;
	if ( ISINTRESOURCE(pszTrigName) )
	{
		iAction = static_cast<CTRIG_TYPE>(GETINTRESOURCE(pszTrigName));
		pszTrigName = sm_szTrigName[iAction];
	}
	else
		iAction = static_cast<CTRIG_TYPE>(FindTableSorted(pszTrigName, sm_szTrigName, COUNTOF(sm_szTrigName) - 1));

	SetTriggerActive(pszTrigName);
	TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;

	TemporaryString szTrigName;
	snprintf(szTrigName, 30, "@char%s", pszTrigName + 1);

	EXC_TRY("Trigger");

	// Triggers installed on characters, sensitive to actions on all chars
	if ( IsTrigUsed(szTrigName) && (FindTableSorted(szTrigName, sm_szTrigName, COUNTOF(sm_szTrigName) - 1) > XTRIG_UNKNOWN) )
	{
		CChar *pChar = pSrc->GetChar();
		if ( pChar && (pChar != this) )
		{
			EXC_SET("chardef");
			CGrayUID uidOldAct = pChar->m_Act_Targ;
			pChar->m_Act_Targ = GetUID();
			iRet = pChar->OnTrigger(szTrigName, pSrc, pArgs);
			pChar->m_Act_Targ = uidOldAct;
			if ( iRet == TRIGRET_RET_TRUE )
				goto stopandret;	//return iRet;
		}
	}

	// EVENTS
	if ( IsTrigUsed(pszTrigName) )
	{
		EXC_SET("events");
		size_t origEvents = m_OEvents.GetCount();
		size_t curEvents = origEvents;
		for ( size_t i = 0; i < curEvents; ++i )
		{
			CResourceLink *pLink = m_OEvents[i];
			if ( !pLink || !pLink->HasTrigger(iAction) )
				continue;
			CResourceLock s;
			if ( !pLink->ResourceLock(s) )
				continue;
			iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
			if ( (iRet != TRIGRET_RET_FALSE) && (iRet != TRIGRET_RET_DEFAULT) )
				goto stopandret;	//return iRet;

			curEvents = m_OEvents.GetCount();
			if ( curEvents < origEvents )	// the event has been deleted, modify the counter for other trigs to work
			{
				--i;
				origEvents = curEvents;
			}
		}

		// TEVENTS
		if ( m_pNPC )
		{
			EXC_SET("NPC triggers");
			for ( size_t i = 0; i < pCharDef->m_TEvents.GetCount(); ++i )
			{
				CResourceLink *pLink = pCharDef->m_TEvents[i];
				if ( !pLink || !pLink->HasTrigger(iAction) )
					continue;
				CResourceLock s;
				if ( !pLink->ResourceLock(s) )
					continue;
				iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
				if ( (iRet != TRIGRET_RET_FALSE) && (iRet != TRIGRET_RET_DEFAULT) )
					goto stopandret;	//return iRet;
			}
		}

		// CHARDEF
		if ( !m_pPlayer )
		{
			EXC_SET("chardef triggers");
			if ( pCharDef->HasTrigger(iAction) )
			{
				CResourceLock s;
				if ( pCharDef->ResourceLock(s) )
				{
					iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
					if ( (iRet != TRIGRET_RET_FALSE) && (iRet != TRIGRET_RET_DEFAULT) )
						goto stopandret;	//return iRet;
				}
			}
		}


		// EVENTSPET
		if ( m_pNPC )
		{
			EXC_SET("NPC triggers - EVENTSPET");
			for ( size_t i = 0; i < g_Cfg.m_pEventsPetLink.GetCount(); ++i )
			{
				CResourceLink *pLink = g_Cfg.m_pEventsPetLink[i];
				if ( !pLink || !pLink->HasTrigger(iAction) )
					continue;
				CResourceLock s;
				if ( !pLink->ResourceLock(s) )
					continue;
				iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
				if ( (iRet != TRIGRET_RET_FALSE) && (iRet != TRIGRET_RET_DEFAULT) )
					goto stopandret;	//return iRet;
			}
		}

		// EVENTSPLAYER
		if ( m_pPlayer )
		{
			EXC_SET("chardef triggers - EVENTSPLAYER");
			for ( size_t i = 0; i < g_Cfg.m_pEventsPlayerLink.GetCount(); ++i )
			{
				CResourceLink *pLink = g_Cfg.m_pEventsPlayerLink[i];
				if ( !pLink || !pLink->HasTrigger(iAction) )
					continue;
				CResourceLock s;
				if ( !pLink->ResourceLock(s) )
					continue;
				iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
				if ( (iRet != TRIGRET_RET_FALSE) && (iRet != TRIGRET_RET_DEFAULT) )
					goto stopandret;	//return iRet;
			}
		}
	}
stopandret:
	{
		SetTriggerActive(NULL);
		return iRet;
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("trigger '%s' action '%d' [0%" FMTDWORDH "]\n", pszTrigName, iAction, static_cast<DWORD>(GetUID()));
	EXC_DEBUG_END;
	return iRet;
}

void CChar::OnTickStatusUpdate()
{
	ADDTOCALLSTACK("CChar::OnTickStatusUpdate");

	if ( m_pClient )
		m_pClient->UpdateStats();

	if ( g_Cfg.m_iHitsUpdateRate && (-g_World.GetTimeDiff(m_timeLastHitsUpdate) >= g_Cfg.m_iHitsUpdateRate) )
	{
		if ( m_fStatusUpdate & SU_UPDATE_HITS )
		{
			PacketHealthUpdate *cmd = new PacketHealthUpdate(this, false);
			UpdateCanSee(cmd, m_pClient);		// send hits update to all nearby clients
			m_fStatusUpdate &= ~SU_UPDATE_HITS;
		}
		m_timeLastHitsUpdate = CServTime::GetCurrentTime();
	}

	if ( m_fStatusUpdate & SU_UPDATE_MODE )
	{
		Update(false);
		m_fStatusUpdate &= ~SU_UPDATE_MODE;
	}

	CObjBase::OnTickStatusUpdate();
}

void CChar::OnTickFood(int iVal, int iHitsHungerLoss)
{
	ADDTOCALLSTACK("CChar::OnTickFood");
	if ( IsStatFlag(STATF_DEAD|STATF_Conjured|STATF_Spawned) || !Stat_GetMax(STAT_FOOD) )
		return;
	if ( IsStatFlag(STATF_Pet) && !NPC_CheckHirelingStatus() )		// this may be money instead of food
		return;
	if ( IsPriv(PRIV_GM) )
		return;

	// Decrease food level
	int iFood = Stat_GetVal(STAT_FOOD) - iVal;
	if ( iFood < 0 )
		iFood = 0;
	Stat_SetVal(STAT_FOOD, iFood);

	// Show hunger message if food level is getting low
	int iFoodLevel = Food_GetLevelPercent();
	if ( iFoodLevel > 40 )
		return;
	if ( (iHitsHungerLoss <= 0) || IsStatFlag(STATF_Stone) )
		return;

	LPCTSTR pszMsgLevel = Food_GetLevelMessage();
	SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_HUNGER), pszMsgLevel);

	TCHAR szMsg[EXPRESSION_MAX_KEY_LEN];
	snprintf(szMsg, sizeof(szMsg), g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_LOOKS), pszMsgLevel);
	CItem *pMountItem = Horse_GetMountItem();
	if ( pMountItem )
		pMountItem->Emote(szMsg);
	else
		Emote(szMsg);

	// Get hunger damage if food level reach 0
	if ( iFoodLevel <= 0 )
	{
		OnTakeDamage(iHitsHungerLoss, this, DAMAGE_FIXED);
		if ( m_pNPC )
			NPC_PetRelease();
	}
}

// Assume this is only called 1 time per sec
// Get a timer tick when our timer expires
// RETURN: false = delete this
bool CChar::OnTick()
{
	ADDTOCALLSTACK("CChar::OnTick");

	EXC_TRY("Tick");
	INT64 iTimeDiff = -g_World.GetTimeDiff(m_timeLastTick);
	if ( !iTimeDiff )
		return true;

	EXC_SET("equipped items");
	CItem *pItemNext = NULL;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( !pItem->IsTimerSet() || !pItem->IsTimerExpired() )
			continue;
		if ( !OnTickEquip(pItem) )
			pItem->Delete();
	}

	if ( IsDisconnected() )
		return true;

	if ( m_pClient )
	{
		// Clear 'running' flag when the client stop running
		if ( -g_World.GetTimeDiff(m_pClient->m_timeLastEventWalk) > 2 )
			StatFlag_Clear(STATF_Fly);

		// Show returning anim for thowing weapons after throw it
		if ( m_pClient->m_timeLastSkillThrowing.IsTimeValid() && (-g_World.GetTimeDiff(m_pClient->m_timeLastSkillThrowing) > 2) )
		{
			m_pClient->m_timeLastSkillThrowing.Init();
			if ( m_pClient->m_pSkillThrowingTarg && m_pClient->m_pSkillThrowingTarg->IsValidUID() )
				Effect(EFFECT_BOLT, m_pClient->m_SkillThrowingAnimID, m_pClient->m_pSkillThrowingTarg, 18, 1, false, m_pClient->m_SkillThrowingAnimHue, m_pClient->m_SkillThrowingAnimRender);
		}
	}

	if ( iTimeDiff >= TICK_PER_SEC )		// don't bother with < 1 sec timers on the checks below
	{
		m_timeLastTick = CServTime::GetCurrentTime();

		EXC_SET("last attackers");
		if ( g_Cfg.m_iAttackerTimeout > 0 )
			Attacker_CheckTimeout();

		EXC_SET("NOTO timeout");
		if ( g_Cfg.m_iNotoTimeout > 0 )
			NotoSave_CheckTimeout();

		if ( !IsStatFlag(STATF_DEAD) )
		{
			EXC_SET("check location");
			CheckLocation(true);

			EXC_SET("regen stats");
			Stat_Regen(iTimeDiff);
		}

		if ( m_pClient && m_pClient->m_Targ_Timeout.IsTimeValid() && (g_World.GetTimeDiff(m_pClient->m_Targ_Timeout) <= 0) )
			m_pClient->addTargetCancel();
	}

	EXC_SET("update stats");
	OnTickStatusUpdate();

	if ( !IsStatFlag(STATF_DEAD) && (Stat_GetVal(STAT_STR) <= 0) )
	{
		EXC_SET("death");
		return Death();
	}

	if ( IsTimerSet() && IsTimerExpired() )
	{
		EXC_SET("timer expired");
		switch ( Skill_Done() )
		{
			case -SKTRIG_ABORT:	EXC_SET("skill abort");		Skill_Fail(true);	break;	// fail with no message or credit
			case -SKTRIG_FAIL:	EXC_SET("skill fail");		Skill_Fail(false);	break;
			case -SKTRIG_QTY:	EXC_SET("skill cleanup");	Skill_Cleanup();	break;
		}

		if ( m_pNPC )
		{
			ProfileTask aiTask(PROFILE_NPC_AI);
			EXC_SET("NPC action");
			if ( !IsStatFlag(STATF_Freeze) )
			{
				NPC_OnTickAction();

				if ( !IsStatFlag(STATF_DEAD) )
				{
					const int iFlags = m_pNPC->GetNpcAiFlags(this);
					if ( iFlags & (NPC_AI_FOOD|NPC_AI_INTFOOD) )
						NPC_Act_Food();
					if ( iFlags & NPC_AI_EXTRA )
						NPC_ExtraAI();
				}
			}
		}
	}

	EXC_CATCH;
	return true;
}
