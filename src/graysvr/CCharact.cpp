//
// CCharAct.cpp
//

#include "graysvr.h"	// predef header.
#include "CClient.h"
#include "../network/network.h"
#include "../network/send.h"

bool CChar::TeleportToObj( int iType, TCHAR * pszArgs )
{
	ADDTOCALLSTACK("CChar::TeleportToObj");
	// "GONAME", "GOTYPE", "GOCHAR"
	// 0 = object name
	// 1 = char
	// 2 = item type

	DWORD dwUID = m_Act_Targ.GetObjUID() &~ UID_F_ITEM;
	DWORD dwTotal = g_World.GetUIDCount();
	DWORD dwCount = dwTotal-1;

	int iArg = 0;
	if ( iType )
	{
		if ( pszArgs[0] && iType == 1 )
			dwUID = 0;
		iArg = RES_GET_INDEX( Exp_GetVal( pszArgs ));
	}

	while ( dwCount-- )
	{
		if ( ++dwUID >= dwTotal )
		{
			dwUID = 1;
		}
		CObjBase * pObj = g_World.FindUID(dwUID);
		if ( pObj == NULL )
			continue;

		switch ( iType )
		{
			case 0:
				{
					MATCH_TYPE match = Str_Match( pszArgs, pObj->GetName());
					if ( match != MATCH_VALID )
						continue;
				}
				break;
			case 1:	// char
				{
					if ( ! pObj->IsChar())
						continue;
					if ( iArg-- > 0 )
						continue;
				}
				break;
			case 2:	// item type
				{
					if ( ! pObj->IsItem())
						continue;
					CItem * pItem = dynamic_cast <CItem*>(pObj);
					if ( ! pItem->IsType(static_cast<IT_TYPE>(iArg)))
						continue;
				}
				break;
			case 3: // char id
				{
					if ( ! pObj->IsChar())
						continue;
					CChar * pChar = dynamic_cast <CChar*>(pObj);
					if ( pChar->GetID() != iArg )
						continue;
				}
				break;
			case 4:	// item id
				{
					if ( ! pObj->IsItem())
						continue;
					CItem * pItem = dynamic_cast <CItem*>(pObj);
					if ( pItem->GetID() != iArg )
						continue;
				}
				break;
		}

		CObjBaseTemplate * pObjTop = pObj->GetTopLevelObj();
		if ( pObjTop == NULL || pObjTop == this )
			continue;
		if ( pObjTop->IsChar() )
		{
			if ( ! CanDisturb( dynamic_cast<CChar*>(pObjTop)))
				continue;
		}

		m_Act_Targ = pObj->GetUID();
		Spell_Teleport( pObjTop->GetTopPoint(), true, false );
		return( true );
	}
	return( false );
}

bool CChar::TeleportToCli( int iType, int iArgs )
{
	ADDTOCALLSTACK("CChar::TeleportToCli");
	
	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != NULL; pClient = it.next())
	{
		if ( ! iType )
		{
			if ( pClient->GetSocketID() != iArgs )
				continue;
		}
		CChar * pChar = pClient->GetChar();
		if ( pChar == NULL )
			continue;
		if ( ! CanDisturb( pChar ))
			continue;
		if ( iType )
		{
			if ( iArgs-- )
				continue;
		}
		m_Act_Targ = pChar->GetUID();
		Spell_Teleport( pChar->GetTopPoint(), true, false );
		return( true );
	}
	return( false );
}

void CChar::Jail( CTextConsole * pSrc, bool fSet, int iCell )
{
	ADDTOCALLSTACK("CChar::Jail");

	CScriptTriggerArgs Args( fSet? 1 : 0, static_cast<INT64>(iCell), static_cast<INT64>(0));

	if ( fSet )	// set the jailed flag.
	{
		if ( IsTrigUsed(TRIGGER_JAILED) )
		{
			if ( OnTrigger( CTRIG_Jailed, pSrc, &Args ) == TRIGRET_RET_TRUE )
				return;
		}

		if ( m_pPlayer )	// allow setting of this to offline chars.
		{
			CAccount *pAccount = m_pPlayer->GetAccount();
			ASSERT(pAccount != NULL);

			pAccount->SetPrivFlags( PRIV_JAILED );
			pAccount->m_TagDefs.SetNum("JailCell", iCell, true);
		}
		if ( IsClient())
		{
			m_pClient->SetPrivFlags( PRIV_JAILED );
		}
		TCHAR szJailName[ 128 ];
		if ( iCell )
		{
			sprintf( szJailName, "jail%d", iCell );
		}
		else
		{
			strcpy( szJailName, "jail" );
		}
		Spell_Teleport( g_Cfg.GetRegionPoint( szJailName ), true, false );
		SysMessageDefault( DEFMSG_MSG_JAILED );
	}
	else	// forgive.
	{
		if ( IsTrigUsed(TRIGGER_JAILED) )
		{
			if ( OnTrigger( CTRIG_Jailed, pSrc, &Args ) == TRIGRET_RET_TRUE )
				return;
		}

		if ( IsClient())
		{
			if ( ! m_pClient->IsPriv( PRIV_JAILED ))
				return;
			m_pClient->ClearPrivFlags( PRIV_JAILED );
		}
		if ( m_pPlayer )
		{
			CAccount *pAccount = m_pPlayer->GetAccount();
			ASSERT(pAccount != NULL);

			pAccount->ClearPrivFlags( PRIV_JAILED );
			if ( pAccount->m_TagDefs.GetKey("JailCell") != NULL )
				pAccount->m_TagDefs.DeleteKey("JailCell");
		}
		SysMessageDefault( DEFMSG_MSG_FORGIVEN );
	}
}

void CChar::AddGoldToPack( int iAmount, CItemContainer * pPack )
{
	ADDTOCALLSTACK("CChar::AddGoldToPack");
	// A vendor is giving me gold. put it in my pack or other place.

	if ( pPack == NULL )
		pPack = GetPackSafe();

	while ( iAmount > 0 )
	{
		CItem * pGold = CItem::CreateScript( ITEMID_GOLD_C1, this );

		int iGoldStack = minimum( iAmount, USHRT_MAX );
		pGold->SetAmount( iGoldStack );

		Sound( pGold->GetDropSound( pPack ));
		pPack->ContentAdd( pGold );
		iAmount -= iGoldStack;
	}
}

void CChar::LayerAdd( CItem * pItem, LAYER_TYPE layer )
{
	ADDTOCALLSTACK("CChar::LayerAdd");
	// add equipped items.
	// check for item already in that layer ?
	// NOTE: This could be part of the Load as well so it may not truly be being "equipped" at this time.
	// OnTrigger for equip is done by ItemEquip()

	if ( pItem == NULL )
		return;
	if ( pItem->GetParent() == this &&
		pItem->GetEquipLayer() == layer )
	{
		return;
	}

	if ( layer == LAYER_DRAGGING )
	{
		pItem->RemoveSelf();	// remove from where i am before add so UNEQUIP effect takes.
		// NOTE: CanEquipLayer may bounce an item . If it stacks with this we are in trouble !
	}

	if ( g_Serv.IsLoading() == false )
	{
		// This takes care of any conflicting items in the slot !
		layer = CanEquipLayer(pItem, layer, NULL, false);
		if ( layer == LAYER_NONE )
		{
			// we should not allow non-layered stuff to be put here ?
			// Put in pack instead ?
			ItemBounce( pItem );
			return;
		}

		if (!pItem->IsTypeSpellable() && !pItem->m_itSpell.m_spell && !pItem->IsType(IT_WAND))	// can this item have a spell effect ? If so we do not send 
		{
			if ((IsTrigUsed(TRIGGER_MEMORYEQUIP)) || (IsTrigUsed(TRIGGER_ITEMMEMORYEQUIP)))
			{
				CScriptTriggerArgs pArgs;
				pArgs.m_iN1 = layer;
				if (pItem->OnTrigger(ITRIG_MemoryEquip, this, &pArgs) == TRIGRET_RET_TRUE)
				{
					pItem->Delete();
					return;
				}
			}
		}
	}

	if ( layer == LAYER_SPECIAL )
	{
		if ( pItem->IsType( IT_EQ_TRADE_WINDOW ))
			layer = LAYER_NONE;
	}

	CContainer::ContentAddPrivate( pItem );
	pItem->SetEquipLayer( layer );

	// update flags etc for having equipped this.
	switch ( layer )
	{
		case LAYER_HAND1:
		case LAYER_HAND2:
			// If weapon
			if ( pItem->IsTypeWeapon())
			{
				m_uidWeapon = pItem->GetUID();
				if ( Fight_IsActive() )
					Skill_Start(Fight_GetWeaponSkill());	// reset weapon swing timer
			}
			else if ( pItem->IsTypeArmor())
			{
				// Shield of some sort.
				m_defense = static_cast<WORD>(CalcArmorDefense());
				StatFlag_Set( STATF_HasShield );
				UpdateStatsFlag();
			}
			break;
		case LAYER_SHOES:
		case LAYER_PANTS:
		case LAYER_SHIRT:
		case LAYER_HELM:		// 6
		case LAYER_GLOVES:	// 7
		case LAYER_COLLAR:	// 10 = gorget or necklace.
		case LAYER_HALF_APRON:
		case LAYER_CHEST:	// 13 = armor chest
		case LAYER_TUNIC:	// 17 = jester suit
		case LAYER_ARMS:		// 19 = armor
		case LAYER_CAPE:		// 20 = cape
		case LAYER_ROBE:		// 22 = robe over all.
		case LAYER_SKIRT:
		case LAYER_LEGS:
			// If armor or clothing = change in defense rating.
			m_defense = static_cast<WORD>(CalcArmorDefense());
			UpdateStatsFlag();
			break;

			// These effects are not magical. (make them spells !)

		case LAYER_FLAG_Criminal:
			StatFlag_Set( STATF_Criminal );
			return;
		case LAYER_FLAG_SpiritSpeak:
			StatFlag_Set( STATF_SpiritSpeak );
			return;
		case LAYER_FLAG_Stuck:
			StatFlag_Set( STATF_Freeze );
			break;
		default:
			break;
	}

	if ( layer != LAYER_DRAGGING )
	{
		switch ( pItem->GetType())
		{
			case IT_EQ_SCRIPT:	// pure script.
				break;
			case IT_EQ_MEMORY_OBJ:
			{
				CItemMemory *pMemory = dynamic_cast<CItemMemory *>( pItem );
				if (pMemory != NULL)
					Memory_UpdateFlags(pMemory);
				break;
			}
			case IT_EQ_HORSE:
				StatFlag_Set(STATF_OnHorse);
				break;
			case IT_COMM_CRYSTAL:
				StatFlag_Set(STATF_COMM_CRYSTAL);
				break;
			default:
				break;
		}

		
	}

	pItem->Update();
}

void CChar::OnRemoveOb( CGObListRec* pObRec )	// Override this = called when removed from list.
{
	ADDTOCALLSTACK("CChar::OnRemoveOb");
	// Unequip the item.
	// This may be a delete etc. It can not FAIL !
	CItem * pItem = STATIC_CAST <CItem*>(pObRec);
	if ( !pItem )
		return;

	LAYER_TYPE layer = pItem->GetEquipLayer();
	if (( IsTrigUsed(TRIGGER_UNEQUIP) ) || ( IsTrigUsed(TRIGGER_ITEMUNEQUIP) ))
	{
		if ( layer != LAYER_DRAGGING && ! g_Serv.IsLoading())
			pItem->OnTrigger( ITRIG_UNEQUIP, this );
	}

	CContainer::OnRemoveOb( pObRec );

	// remove equipped items effects
	switch ( layer )
	{
		case LAYER_HAND1:
		case LAYER_HAND2:	// other hand = shield
			if ( pItem->IsTypeWeapon())
			{
				m_uidWeapon.InitUID();
				if ( Fight_IsActive() )
					Skill_Start(Fight_GetWeaponSkill());	// reset weapon swing timer
			}
			else if ( pItem->IsTypeArmor())
			{
				// Shield
				m_defense = static_cast<WORD>(CalcArmorDefense());
				StatFlag_Clear( STATF_HasShield );
				UpdateStatsFlag();
			}
			if (( this->m_Act_SkillCurrent == SKILL_MINING ) || ( this->m_Act_SkillCurrent == SKILL_FISHING ) || ( this->m_Act_SkillCurrent == SKILL_LUMBERJACKING ))
			{
				Skill_Cleanup();
			}
			break;
		case LAYER_SHOES:
		case LAYER_PANTS:
		case LAYER_SHIRT:
		case LAYER_HELM:		// 6
		case LAYER_GLOVES:	// 7
		case LAYER_COLLAR:	// 10 = gorget or necklace.
		case LAYER_CHEST:	// 13 = armor chest
		case LAYER_TUNIC:	// 17 = jester suit
		case LAYER_ARMS:		// 19 = armor
		case LAYER_CAPE:		// 20 = cape
		case LAYER_ROBE:		// 22 = robe over all.
		case LAYER_SKIRT:
		case LAYER_LEGS:
			m_defense = static_cast<WORD>(CalcArmorDefense());
			UpdateStatsFlag();
			break;

		case LAYER_FLAG_Criminal:
			StatFlag_Clear( STATF_Criminal );
			break;
		case LAYER_FLAG_SpiritSpeak:
			StatFlag_Clear( STATF_SpiritSpeak );
			break;
		case LAYER_FLAG_Stuck:
			StatFlag_Clear( STATF_Freeze );
			break;
		default:
			break;
	}

	// Items with magic effects.
	if ( layer != LAYER_DRAGGING )
	{
		switch ( pItem->GetType())
		{
			case IT_COMM_CRYSTAL:
				if ( ContentFind( RESOURCE_ID( RES_TYPEDEF,IT_COMM_CRYSTAL ), 0, 0 ) == NULL )
				{
					StatFlag_Clear(STATF_COMM_CRYSTAL);
				}
				break;
			case IT_EQ_HORSE:
				StatFlag_Clear(STATF_OnHorse);
				break;
			case IT_EQ_MEMORY_OBJ:
			{
				// Clear the associated flags.
				CItemMemory *pMemory = dynamic_cast<CItemMemory*>(pItem);
				if (pMemory != NULL)
					Memory_UpdateClearTypes( pMemory, 0xFFFF );
				break;
			}
			default:
				break;
		}

		if ( pItem->IsTypeArmorWeapon() )
		{
			SetDefNum("DAMPHYSICAL", GetDefNum("DAMPHYSICAL", true) - pItem->GetDefNum("DAMPHYSICAL", true, true));
			SetDefNum("DAMFIRE", GetDefNum("DAMFIRE", true) - pItem->GetDefNum("DAMFIRE", true, true));
			SetDefNum("DAMCOLD", GetDefNum("DAMCOLD", true) - pItem->GetDefNum("DAMCOLD", true, true));
			SetDefNum("DAMPOISON", GetDefNum("DAMPOISON", true) - pItem->GetDefNum("DAMPOISON", true, true));
			SetDefNum("DAMENERGY", GetDefNum("DAMENERGY", true) - pItem->GetDefNum("DAMENERGY", true, true));

			SetDefNum("RESPHYSICAL", GetDefNum("RESPHYSICAL", true) - pItem->GetDefNum("RESPHYSICAL", true, true));
			SetDefNum("RESFIRE", GetDefNum("RESFIRE", true) - pItem->GetDefNum("RESFIRE", true, true));
			SetDefNum("RESCOLD", GetDefNum("RESCOLD", true) - pItem->GetDefNum("RESCOLD", true, true));
			SetDefNum("RESPOISON", GetDefNum("RESPOISON", true) - pItem->GetDefNum("RESPOISON", true, true));
			SetDefNum("RESENERGY", GetDefNum("RESENERGY", true) - pItem->GetDefNum("RESENERGY", true, true));
		}

		if ( pItem->IsTypeWeapon() )
		{
			CItem * pCursedMemory = LayerFind(LAYER_SPELL_Curse_Weapon);	// Remove the cursed state from SPELL_Curse_Weapon.
			if (pCursedMemory)
				pItem->SetDefNum("HitLeechLife", pItem->GetDefNum("HitLeechLife", true) - pCursedMemory->m_itSpell.m_spelllevel, true);
		}

		short iStrengthBonus = static_cast<short>(pItem->GetDefNum("BONUSSTR", true, true));
		if (iStrengthBonus != 0)
			Stat_SetMod(STAT_STR, Stat_GetMod(STAT_STR) - iStrengthBonus);

		short iDexterityBonus = static_cast<short>(pItem->GetDefNum("BONUSDEX", true, true));
		if (iDexterityBonus != 0)
			Stat_SetMod(STAT_DEX, Stat_GetMod(STAT_DEX) - iDexterityBonus);

		short iIntelligenceBonus = static_cast<short>(pItem->GetDefNum("BONUSINT", true, true));
		if (iIntelligenceBonus != 0)
			Stat_SetMod(STAT_INT, Stat_GetMod(STAT_INT) - iIntelligenceBonus);

		short iHitpointIncrease = static_cast<short>(pItem->GetDefNum("BONUSHITS", true, true));
		if (iHitpointIncrease != 0)
			Stat_SetMax(STAT_STR, Stat_GetMax(STAT_STR) - iHitpointIncrease);

		int iStaminaIncrease = static_cast<int>(pItem->GetDefNum("BONUSSTAM", true, true));
		if (iStaminaIncrease != 0)
			Stat_SetMax(STAT_DEX, Stat_GetMax(STAT_DEX) - iStaminaIncrease);

		int iManaIncrease = static_cast<int>(pItem->GetDefNum("BONUSMANA", true, true));
		if (iManaIncrease != 0)
			Stat_SetMax(STAT_INT, Stat_GetMax(STAT_INT) - iManaIncrease);

		INT64 iDamageIncrease = pItem->GetDefNum("INCREASEDAM", true, true);
		if ( iDamageIncrease != 0 )
			SetDefNum("INCREASEDAM", GetDefNum("INCREASEDAM", true) - iDamageIncrease);

		INT64 iDefenseChanceIncrease = pItem->GetDefNum("INCREASEDEFCHANCE", true, true);
		if ( iDefenseChanceIncrease != 0 )
			SetDefNum("INCREASEDEFCHANCE", GetDefNum("INCREASEDEFCHANCE", true) - iDefenseChanceIncrease);

		INT64 iFasterCasting = pItem->GetDefNum("FASTERCASTING", true, true);
		if ( iFasterCasting != 0 )
			SetDefNum("FASTERCASTING", GetDefNum("FASTERCASTING", true) - iFasterCasting);

		INT64 iHitChanceIncrease = pItem->GetDefNum("INCREASEHITCHANCE", true, true);
		if ( iHitChanceIncrease != 0 )
			SetDefNum("INCREASEHITCHANCE", GetDefNum("INCREASEHITCHANCE", true) - iHitChanceIncrease);

		INT64 iSpellDamageIncrease = pItem->GetDefNum("INCREASESPELLDAM", true, true);
		if ( iSpellDamageIncrease != 0 )
			SetDefNum("INCREASESPELLDAM", GetDefNum("INCREASESPELLDAM", true) - iSpellDamageIncrease);

		INT64 iSwingSpeedIncrease = pItem->GetDefNum("INCREASESWINGSPEED", true, true);
		if ( iSwingSpeedIncrease != 0 )
			SetDefNum("INCREASESWINGSPEED", GetDefNum("INCREASESWINGSPEED", true) - iSwingSpeedIncrease);

		INT64 iEnhancePotions = pItem->GetDefNum("ENHANCEPOTIONS", true, true);
		if (iEnhancePotions != 0)
			SetDefNum("ENHANCEPOTIONS", GetDefNum("ENHANCEPOTIONS", true) - iEnhancePotions);

		INT64 iLowerManaCost = pItem->GetDefNum("LOWERMANACOST", true, true);
		if (iLowerManaCost != 0)
			SetDefNum("LOWERMANACOST", GetDefNum("LOWERMANACOST", true) - iLowerManaCost);

		INT64 iLuck = pItem->GetDefNum("LUCK", true, true);
		if ( iLuck != 0 )
			SetDefNum("LUCK", GetDefNum("LUCK", true) - iLuck);

		if ( pItem->GetDefNum("NIGHTSIGHT", true, true))
		{
			StatFlag_Mod( STATF_NightSight, 0 );
			if ( IsClient() )
				m_pClient->addLight();
		}

		// If items are magical then remove effect here.
		Spell_Effect_Remove(pItem);
	}
}

void CChar::DropAll( CItemContainer * pCorpse, DWORD dwAttr )
{
	ADDTOCALLSTACK("CChar::DropAll");
	// shrunk or died. (or sleeping)
	if ( IsStatFlag( STATF_Conjured ))
		return;	// drop nothing.

	CItemContainer * pPack = GetPack();
	if ( pPack != NULL )
	{
		if ( pCorpse == NULL )
		{
			pPack->ContentsDump( GetTopPoint(), dwAttr );
		}
		else
		{
			pPack->ContentsTransfer( pCorpse, true );
		}
	}

	// transfer equipped items to corpse or your pack (if newbie).
	UnEquipAllItems( pCorpse );
}

void CChar::UnEquipAllItems( CItemContainer * pDest, bool bLeaveHands )
{
	ADDTOCALLSTACK("CChar::UnEquipAllItems");
	// We morphed, sleeping, died or became a GM.
	// Pets can be told to "Drop All"
	// drop item that is up in the air as well.
	// pDest       = Container to place items in
	// bLeaveHands = true to leave items in hands; otherwise, false

	if ( GetCount() <= 0 )
		return;
	CItemContainer * pPack = NULL;

	CItem* pItemNext;
	CItem* pItem = GetContentHead();
	for ( ; pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		LAYER_TYPE layer = pItem->GetEquipLayer();

		switch ( layer )
		{
			case LAYER_NONE:
				pItem->Delete();	// Get rid of any trades.
				continue;
			case LAYER_FLAG_Poison:
			case LAYER_FLAG_Criminal:
			case LAYER_FLAG_Hallucination:
			case LAYER_FLAG_Potion:
			case LAYER_FLAG_Drunk:
			case LAYER_FLAG_Stuck:
			case LAYER_FLAG_PotionUsed:
				if ( IsStatFlag( STATF_DEAD ))
					pItem->Delete();
				continue;
			case LAYER_PACK:
			case LAYER_HORSE:
				continue;
			case LAYER_HAIR:	// leave this.
			case LAYER_BEARD:
				// Copy hair and beard to corpse.
				if ( pDest == NULL )
					continue;
				if ( pDest->IsType(IT_CORPSE))
				{
					CItem * pDupe = CItem::CreateDupeItem( pItem );
					pDest->ContentAdd( pDupe );	// add content
					// Equip layer only matters on a corpse.
					pDupe->SetContainedLayer(static_cast<unsigned char>(layer));
				}
				continue;
			case LAYER_DRAGGING:
				layer = LAYER_NONE;
				break;
			case LAYER_HAND1:
			case LAYER_HAND2:
				if (bLeaveHands)
					continue;
				break;
			default:
				// can't transfer this to corpse.
				if ( ! CItemBase::IsVisibleLayer( layer ))
					continue;
				break;
		}
		if ( pDest != NULL &&
			! pItem->IsAttr( ATTR_NEWBIE|ATTR_MOVE_NEVER|ATTR_BLESSED|ATTR_INSURED|ATTR_NODROPTRADE ))
		{	// Move item to dest. (corpse ussually)
			pDest->ContentAdd( pItem );
			if ( pDest->IsType(IT_CORPSE))
			{
				// Equip layer only matters on a corpse.
				pItem->SetContainedLayer(static_cast<unsigned char>(layer));
			}
		}
		else
		{	// Move item to chars' pack.
			if ( pPack == NULL )
				pPack = GetPackSafe();
			pPack->ContentAdd( pItem );
		}
	}
}

void CChar::UpdateDrag( CItem * pItem, CObjBase * pCont, CPointMap * pt )
{
	ADDTOCALLSTACK("CChar::UpdateDrag");
	// Show the world that I am picking up or putting down this object.
	// NOTE: This makes people disapear.

	if ( pCont && pCont->GetTopLevelObj() == this )		// moving to my own backpack
		return;
	if ( !pCont && !pt && pItem->GetTopLevelObj() == this )		// doesn't work for ground objects
		return;

	PacketDragAnimation* cmd = new PacketDragAnimation(this, pItem, pCont, pt);
	UpdateCanSee(cmd, m_pClient);
}


void CChar::UpdateStatsFlag() const
{
	ADDTOCALLSTACK("CChar::UpdateStatsFlag");
	// Push status change to all who can see us.
	// For Weight, AC, Gold must update all
	// Just flag the stats to be updated later if possible.
	if ( g_Serv.IsLoading() )
		return;

	if ( IsClient() )
		GetClient()->addUpdateStatsFlag();
}

// queue updates

void CChar::UpdateHitsFlag()
{
	ADDTOCALLSTACK("CChar::UpdateHitsFlag");
	if ( g_Serv.IsLoading() )
		return;

	m_fStatusUpdate |= SU_UPDATE_HITS;

	if ( IsClient() )
		GetClient()->addUpdateHitsFlag();
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

	if ( IsClient() )
		GetClient()->addUpdateManaFlag();
}

void CChar::UpdateStamFlag() const
{
	ADDTOCALLSTACK("CChar::UpdateStamFlag");
	if ( g_Serv.IsLoading() )
		return;

	if ( IsClient() )
		GetClient()->addUpdateStamFlag();
}

void CChar::UpdateRegenTimers(STAT_TYPE iStat, short iVal)
{
	ADDTOCALLSTACK("CChar::UpdateRegenTimers");
	m_Stat[iStat].m_regen = iVal;
}

void CChar::UpdateStatVal( STAT_TYPE type, int iChange, int iLimit )
{
	ADDTOCALLSTACK("CChar::UpdateStatVal");
	int iValPrev = Stat_GetVal(type);
	int iVal = iValPrev + iChange;
	if ( !iLimit )
		iLimit = Stat_GetMax( type );

	if ( iVal < 0 )
		iVal = 0;
	else if ( iVal > iLimit )
		iVal = iLimit;

	if ( iVal == iValPrev )
		return;

	Stat_SetVal(type, iVal);

	switch ( type )
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

ANIM_TYPE CChar::GenerateAnimate( ANIM_TYPE action, bool fTranslate, bool fBackward, BYTE iFrameDelay, BYTE iAnimLen )
{
	ADDTOCALLSTACK("CChar::UpdateAnimate");
	// NPC or character does a certain Animate
	// Translate the animation based on creature type.
	// ARGS:
	//   fBackward = make the anim go in reverse.
	//   iFrameDelay = in seconds (approx), 0=fastest, 1=slower
	UNREFERENCED_PARAMETER(iAnimLen);
	if ( action < 0 || action >= ANIM_QTY )
		return (ANIM_TYPE)-1;

	CCharBase* pCharDef = Char_GetDef();

	//Begin old client animation behaviour

	if ( fBackward && iFrameDelay )	// backwards and delayed just dont work ! = invis
		iFrameDelay = 0;

	if (fTranslate || IsStatFlag(STATF_OnHorse))
	{
		CItem * pWeapon = m_uidWeapon.ItemFind();
		if (pWeapon != NULL && action == ANIM_ATTACK_WEAPON)
		{
			// action depends on weapon type (skill) and 2 Hand type.
			LAYER_TYPE layer = pWeapon->Item_GetDef()->GetEquipLayer();
			switch (pWeapon->GetType())
			{
			case IT_WEAPON_MACE_CROOK:	// sheperds crook
			case IT_WEAPON_MACE_SMITH:	// smith/sledge hammer
			case IT_WEAPON_MACE_STAFF:
			case IT_WEAPON_MACE_SHARP:	// war axe can be used to cut/chop trees.
				action = (layer == LAYER_HAND2) ? ANIM_ATTACK_2H_BASH : ANIM_ATTACK_1H_BASH;
				break;
			case IT_WEAPON_SWORD:
			case IT_WEAPON_AXE:
			case IT_WEAPON_MACE_PICK:	// pickaxe
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
			default:
				break;
			}
			// Temporary disabled - it's causing weird animations on some weapons
			/*if ((Calc_GetRandVal(2)) && (pWeapon->GetType() != IT_WEAPON_BOW) && (pWeapon->GetType() != IT_WEAPON_XBOW) && (pWeapon->GetType() != IT_WEAPON_THROWING))
			{
				// add some style to the attacks.
				if (layer == LAYER_HAND2)
					action = static_cast<ANIM_TYPE>(ANIM_ATTACK_2H_BASH + Calc_GetRandVal(3));
				else
					action = static_cast<ANIM_TYPE>(ANIM_ATTACK_1H_SLASH + Calc_GetRandVal(3));
			}*/
		}

		if (IsStatFlag(STATF_OnHorse))	// on horse back.
		{
			// Horse back anims are dif.
			switch (action)
			{
			case ANIM_WALK_UNARM:
			case ANIM_WALK_ARM:
				return ANIM_HORSE_RIDE_SLOW;
			case ANIM_RUN_UNARM:
			case ANIM_RUN_ARMED:
				return ANIM_HORSE_RIDE_FAST;
			case ANIM_STAND:
				return ANIM_HORSE_STAND;
			case ANIM_FIDGET1:
			case ANIM_FIDGET_YAWN:
				return ANIM_HORSE_SLAP;
			case ANIM_STAND_WAR_1H:
			case ANIM_STAND_WAR_2H:
				return ANIM_HORSE_STAND;
			case ANIM_ATTACK_1H_SLASH:
			case ANIM_ATTACK_1H_PIERCE:
			case ANIM_ATTACK_1H_BASH:
				return ANIM_HORSE_ATTACK;
			case ANIM_ATTACK_2H_BASH:
			case ANIM_ATTACK_2H_SLASH:
			case ANIM_ATTACK_2H_PIERCE:
				return ANIM_HORSE_SLAP;
			case ANIM_WALK_WAR:
				return ANIM_HORSE_RIDE_SLOW;
			case ANIM_CAST_DIR:
				return ANIM_HORSE_ATTACK;
			case ANIM_CAST_AREA:
				return ANIM_HORSE_ATTACK_BOW;
			case ANIM_ATTACK_BOW:
				return ANIM_HORSE_ATTACK_BOW;
			case ANIM_ATTACK_XBOW:
				return ANIM_HORSE_ATTACK_XBOW;
			case ANIM_GET_HIT:
				return ANIM_HORSE_SLAP;
			case ANIM_BLOCK:
				return ANIM_HORSE_SLAP;
			case ANIM_ATTACK_WRESTLE:
				return ANIM_HORSE_ATTACK;
			case ANIM_BOW:
			case ANIM_SALUTE:
			case ANIM_EAT:
				return ANIM_HORSE_ATTACK_XBOW;
			default:
				return ANIM_HORSE_STAND;
			}
		}
		else if (!IsPlayableCharacter())  //( GetDispID() < CREID_MAN ) Possible fix for anims not being displayed above 400
		{
			// Animals have certain anims. Monsters have others.

			if (GetDispID() >= CREID_HORSE1)
			{
				// All animals have all these anims thankfully
				switch (action)
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
						switch (Calc_GetRandVal(2))
						{
						case 0: return ANIM_ANI_ATTACK1; break;
						case 1: return ANIM_ANI_ATTACK2; break;
						}

					case ANIM_DIE_BACK:
						return ANIM_ANI_DIE1;
					case ANIM_DIE_FORWARD:
						return ANIM_ANI_DIE2;
					case ANIM_BLOCK:
					case ANIM_BOW:
					case ANIM_SALUTE:
						return ANIM_ANI_SLEEP;
					case ANIM_EAT:
						return ANIM_ANI_EAT;
					default:
						break;
				}

				while (action != ANIM_WALK_UNARM && !(pCharDef->m_Anims & (1 << action)))
				{
					// This anim is not supported. Try to use one that is.
					switch (action)
					{
						case ANIM_ANI_SLEEP:	// All have this.
							return ANIM_ANI_EAT;
						default:
							return ANIM_WALK_UNARM;
					}
				}
			}
			else
			{
				// Monsters don't have all the anims.

				switch (action)
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
						switch (Calc_GetRandVal(3))
						{
						case 0: return ANIM_MON_GETHIT; break;
						case 1: return ANIM_MON_BlockRight; break;
						case 2: return ANIM_MON_BlockLeft; break;
						}
						break;
					case ANIM_ATTACK_1H_SLASH:
					case ANIM_ATTACK_1H_PIERCE:
					case ANIM_ATTACK_1H_BASH:
					case ANIM_ATTACK_2H_BASH:
					case ANIM_ATTACK_2H_PIERCE:
					case ANIM_ATTACK_2H_SLASH:
					case ANIM_ATTACK_BOW:
					case ANIM_ATTACK_XBOW:
					case ANIM_ATTACK_WRESTLE:
						switch (Calc_GetRandVal(3))
						{
						case 0: return ANIM_MON_ATTACK1; break;
						case 1: return ANIM_MON_ATTACK2; break;
						case 2: return ANIM_MON_ATTACK3; break;
						}
					default:
						return ANIM_WALK_UNARM;
				}
				// NOTE: Available actions depend HEAVILY on creature type !
				// ??? Monsters don't have all anims in common !
				// translate these !
				while (action != ANIM_WALK_UNARM && !(pCharDef->m_Anims & (1 << action)))
				{
					// This anim is not supported. Try to use one that is.
					switch (action)
					{
					case ANIM_MON_ATTACK1:	// All have this.
						DEBUG_ERR(("Anim 0%x This is wrong! Invalid SCP file data.\n", GetDispID()));
						return ANIM_WALK_UNARM;

					case ANIM_MON_ATTACK2:	// Dolphins, Eagles don't have this.
					case ANIM_MON_ATTACK3:
						return ANIM_MON_ATTACK1;	// ALL creatures have at least this attack.
					case ANIM_MON_Cast2:	// Trolls, Spiders, many others don't have this.
						return ANIM_MON_BlockRight;	// Birds don't have this !
					case ANIM_MON_BlockRight:
						return ANIM_MON_BlockLeft;
					case ANIM_MON_BlockLeft:
						return ANIM_MON_GETHIT;
					case ANIM_MON_GETHIT:
						if (pCharDef->m_Anims & (1 << ANIM_MON_Cast2))
							return ANIM_MON_Cast2;
						else
							return ANIM_WALK_UNARM;

					case ANIM_MON_Stomp:
						return ANIM_MON_PILLAGE;
					case ANIM_MON_PILLAGE:
						return ANIM_MON_ATTACK3;
					case ANIM_MON_AttackBow:
					case ANIM_MON_AttackXBow:
						return ANIM_MON_ATTACK3;
					case ANIM_MON_AttackThrow:
						return ANIM_MON_AttackXBow;

					default:
						DEBUG_ERR(("Anim Unsupported 0%x for 0%x\n", action, GetDispID()));
						return ANIM_WALK_UNARM;
					}
				}
			}
		}
	}

	return action;
}

bool CChar::UpdateAnimate(ANIM_TYPE action, bool fTranslate, bool fBackward , BYTE iFrameDelay , BYTE iAnimLen)
{

	if (action < 0 || action >= ANIM_QTY)
		return false;

	ANIM_TYPE_NEW subaction = static_cast<ANIM_TYPE_NEW>(-1);
	BYTE variation = 0;		//Seems to have some effect for humans/elfs vs gargoyles
	if (fTranslate)
		action = GenerateAnimate( action, true, fBackward);
	ANIM_TYPE_NEW action1 = static_cast<ANIM_TYPE_NEW>(action);
	if (IsPlayableCharacter())		//Perform these checks only for Gargoyles or in Enhanced Client
	{
		CItem * pWeapon = m_uidWeapon.ItemFind();
		if (pWeapon != NULL && action == ANIM_ATTACK_WEAPON)
		{
			if (!IsGargoyle())		//Set variation to 1 for non gargoyle characters (Humans and Elfs using EC) in all fighting animations.
				variation = 1;
			// action depends on weapon type (skill) and 2 Hand type.
			LAYER_TYPE layer = pWeapon->Item_GetDef()->GetEquipLayer();
			action1 = NANIM_ATTACK; //Should be NANIM_ATTACK;
			switch (pWeapon->GetType())
			{
			case IT_WEAPON_MACE_CROOK:
			case IT_WEAPON_MACE_PICK:
			case IT_WEAPON_MACE_SMITH:	// Can be used for smithing ?
			case IT_WEAPON_MACE_STAFF:
			case IT_WEAPON_MACE_SHARP:	// war axe can be used to cut/chop trees.
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
			default:
				break;
			}
		}
		else {
			switch (action)
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
				/*case ANIM_BOW:		//I'm commenting these 2 because they are not showing properly when Hovering/Mounted, so we skip them.
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
				break;
			default:
				break;
			}
		}
	}//Other new animations than work on humans, elfs and gargoyles
	switch (action)
	{
	case ANIM_DIE_BACK:
		variation = 1;		//Variation makes characters die back
		action1 = NANIM_DEATH;
		break;
	case ANIM_DIE_FORWARD:
		action1 = NANIM_DEATH;
		break;
	}
	PacketActionBasic* cmdnew = new PacketActionBasic(this, action1, subaction, variation);
	PacketAction* cmd = new PacketAction(this, action, 1, fBackward, iFrameDelay, iAnimLen);

	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != NULL; pClient = it.next())
	{
		if (!pClient->CanSee(this))
			continue;
		if (pClient->GetNetState()->isClientSA() && pClient->GetNetState()->m_reportedVersion < 6700351)	//Enhanced client always used this packet, at least until ~ 4.0.35 (6700351)
			cmdnew->send(pClient);
		else if (pClient->GetNetState()->isClientVersion(MINCLIVER_NEWMOBILEANIM) && (IsGargoyle()) && (action1 >= 0))	// On classic clients only send new packets for gargoyles
			cmdnew->send(pClient);
		else
			cmd->send(pClient);
	}
	delete cmdnew;
	delete cmd;	
	return true;
}

void CChar::UpdateMode( CClient * pExcludeClient, bool fFull )
{
	ADDTOCALLSTACK("CChar::UpdateMode");
	// If character status has been changed
	// (Polymorph, war mode or hide), resend him

	// no need to update the mode in the next tick
	if ( pExcludeClient == NULL )
		m_fStatusUpdate &= ~SU_UPDATE_MODE;

	ClientIterator it;
	for ( CClient* pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( pExcludeClient == pClient )
			continue;
		if ( pClient->GetChar() == NULL )
			continue;
		if ( GetTopPoint().GetDistSight(pClient->GetChar()->GetTopPoint()) > UO_MAP_VIEW_SIZE )
			continue;
		if ( !pClient->CanSee(this) )
		{
			// In the case of "INVIS" used by GM's we must use this.
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

void CChar::UpdateSpeedMode()
{
	ADDTOCALLSTACK("CChar::UpdateSpeedMode");
	if ( g_Serv.IsLoading() || !m_pPlayer )
		return;

	if ( IsClient() )
		GetClient()->addSpeedMode( m_pPlayer->m_speedMode );
}

void CChar::UpdateVisualRange()
{
	ADDTOCALLSTACK("CChar::UpdateVisualRange");
	if ( g_Serv.IsLoading() || !m_pPlayer )
		return;

	DEBUG_WARN(("CChar::UpdateVisualRange called, m_iVisualRange is %d\n", m_iVisualRange));

	if ( IsClient() )
		GetClient()->addVisualRange( m_iVisualRange );
}

void CChar::UpdateMove( const CPointMap & ptOld, CClient * pExcludeClient, bool bFull )
{
	ADDTOCALLSTACK("CChar::UpdateMove");
	// Who now sees this char ?
	// Did they just see him move ?

	// no need to update the mode in the next tick
	if ( pExcludeClient == NULL )
		m_fStatusUpdate &= ~SU_UPDATE_MODE;

	EXC_TRY("UpdateMove");
	EXC_SET("FOR LOOP");
	ClientIterator it;
	for ( CClient* pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( pClient == pExcludeClient )
			continue;	// no need to see self move

		if ( pClient == m_pClient )
		{
			EXC_SET("AddPlayerView");
			pClient->addPlayerView(ptOld, bFull);
			continue;
		}

		EXC_SET("GetChar");
		CChar * pChar = pClient->GetChar();
		if ( pChar == NULL )
			continue;

		bool fCouldSee = (ptOld.GetDistSight(pChar->GetTopPoint()) <= pChar->GetSight());
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

void CChar::UpdateDir( DIR_TYPE dir )
{
	ADDTOCALLSTACK("CChar::UpdateDir");

	if ( dir != m_dirFace && dir > DIR_INVALID && dir < DIR_QTY )
	{
		m_dirFace = dir;	// face victim.
		UpdateMove(GetTopPoint());
	}
}

void CChar::UpdateDir( const CPointMap & pt )
{
	ADDTOCALLSTACK("CChar::UpdateDir");

	// Change in direction.
	UpdateDir(GetTopPoint().GetDir(pt));
}

void CChar::UpdateDir( const CObjBaseTemplate * pObj )
{
	ADDTOCALLSTACK("CChar::UpdateDir");
	if ( pObj == NULL )
		return;

	pObj = pObj->GetTopLevelObj();
	if ( pObj == NULL || pObj == this )		// in our own pack
		return;
	UpdateDir(pObj->GetTopPoint());
}

void CChar::Update(const CClient * pClientExclude ) // If character status has been changed (Polymorph), resend him
{
	ADDTOCALLSTACK("CChar::Update");
	// Or I changed looks.
	// I moved or somebody moved me  ?

	// no need to update the mode in the next tick
	if ( pClientExclude == NULL)
		m_fStatusUpdate &= ~SU_UPDATE_MODE;

	ClientIterator it;
	for ( CClient* pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( pClient == pClientExclude )
			continue;
		if ( pClient->GetChar() == NULL )
			continue;
		if ( GetTopPoint().GetDistSight(pClient->GetChar()->GetTopPoint()) > UO_MAP_VIEW_SIZE )
			continue;
		if ( !pClient->CanSee(this) )
		{
			// In the case of "INVIS" used by GM's we must use this.
			pClient->addObjectRemove(this);
			continue;
		}

		if ( pClient == m_pClient )
			pClient->addReSync();
		else
			pClient->addChar(this);
	}
}

void CChar::SoundChar( CRESND_TYPE type )
{
	ADDTOCALLSTACK("CChar::SoundChar");
	if ( !g_Cfg.m_fGenericSounds )
		return;

	SOUND_TYPE id;

	CCharBase* pCharDef = Char_GetDef();
	switch ( GetDispID() )
	{
		case CREID_BLADES:
			id = pCharDef->m_soundbase;
			break;

		case CREID_MAN:
		case CREID_WOMAN:
		case CREID_GHOSTMAN:
		case CREID_GHOSTWOMAN:
		case CREID_ELFMAN:
		case CREID_ELFWOMAN:
		case CREID_ELFGHOSTMAN:
		case CREID_ELFGHOSTWOMAN:
		case CREID_GARGMAN:
		case CREID_GARGWOMAN:
		case CREID_GARGGHOSTMAN:
		case CREID_GARGGHOSTWOMAN:
		{
			id = 0;

			static const SOUND_TYPE sm_Snd_Man_Die[] = { 0x15a, 0x15b, 0x15c, 0x15d };
			static const SOUND_TYPE sm_Snd_Man_Omf[] = { 0x154, 0x155, 0x156, 0x157, 0x158, 0x159 };
			static const SOUND_TYPE sm_Snd_Wom_Die[] = { 0x150, 0x151, 0x152, 0x153 };
			static const SOUND_TYPE sm_Snd_Wom_Omf[] = { 0x14b, 0x14c, 0x14d, 0x14e, 0x14f };

			if ( pCharDef->IsFemale())
			{
				switch ( type )
				{
					case CRESND_GETHIT:
						id = sm_Snd_Wom_Omf[ Calc_GetRandVal( COUNTOF(sm_Snd_Wom_Omf)) ];
						break;
					case CRESND_DIE:
						id = sm_Snd_Wom_Die[ Calc_GetRandVal( COUNTOF(sm_Snd_Wom_Die)) ];
						break;
					default:
						break;
				}
			}
			else
			{
				switch ( type )
				{
					case CRESND_GETHIT:
						id = sm_Snd_Man_Omf[ Calc_GetRandVal( COUNTOF(sm_Snd_Man_Omf)) ];
						break;
					case CRESND_DIE:
						id = sm_Snd_Man_Die[ Calc_GetRandVal( COUNTOF(sm_Snd_Man_Die)) ];
						break;
					default:
						break;
				}
			}
		} break;

		default:
		{
			id = static_cast<SOUND_TYPE>(pCharDef->m_soundbase + type);
			switch ( pCharDef->m_soundbase )	// some creatures have no base sounds.
			{
				case 0:
					id = SOUND_NONE;
					break;
				case 128: // old versions
				case 181:
				case 199:
					if ( type <= CRESND_RAND2 )
						id = SOUND_NONE;
					break;
				case 130: // ANIMALS_DEER3
				case 183: // ANIMALS_LLAMA3
				case 201: // ANIMALS_RABBIT3
					if ( type <= CRESND_RAND2 )
						id = SOUND_NONE;
					else
						id = static_cast<SOUND_TYPE>(id-2);
					break;
				default:
					break;
			}
		} break;
	}

	if ( type == CRESND_HIT )
	{
		CItem * pWeapon = m_uidWeapon.ItemFind();
		if ( pWeapon != NULL )
		{
			CVarDefCont * pTagStorage = NULL; 
			pTagStorage = pWeapon->GetKey("OVERRIDE.SOUND_HIT", true);
			if ( pTagStorage )
			{
				if ( pTagStorage->GetValNum() )
					id = static_cast<SOUND_TYPE>(pTagStorage->GetValNum());
			}
			else
			{
				// weapon type strike noise based on type of weapon and how hard hit.
				switch ( pWeapon->GetType() )
				{
					case IT_WEAPON_MACE_CROOK:
					case IT_WEAPON_MACE_PICK:
					case IT_WEAPON_MACE_SMITH:	// Can be used for smithing ?
					case IT_WEAPON_MACE_STAFF:
						// 0x233 = blunt01 (miss?)
						id = 0x233;
						break;
					case IT_WEAPON_MACE_SHARP:	// war axe can be used to cut/chop trees.
						// 0x232 = axe01 swing. (miss?)
						id = 0x232;
						break;
					case IT_WEAPON_THROWING:
					case IT_WEAPON_SWORD:
					case IT_WEAPON_AXE:
						if ( pWeapon->Item_GetDef()->GetEquipLayer() == LAYER_HAND2 )
						{
							// 0x236 = hvyswrd1 = (heavy strike)
							// 0x237 = hvyswrd4 = (heavy strike)
							id = Calc_GetRandVal( 2 ) ? 0x236 : 0x237;
							break;
						}
					case IT_WEAPON_FENCE:
						// 0x23b = sword1
						// 0x23c = sword7
						id = Calc_GetRandVal( 2 ) ? 0x23b : 0x23c;
						break;
					case IT_WEAPON_BOW:
					case IT_WEAPON_XBOW:
						// 0x234 = xbow (hit)
						id = 0x234;
						break;
					default:
						break;
				}
			}

		}
		else if ( id == 0 )
		{
			static const SOUND_TYPE sm_Snd_Hit[] =
			{
				0x135,	//= hit01 = (slap)
				0x137,	//= hit03 = (hit sand)
				0x13b	//= hit07 = (hit slap)
			};
			id = sm_Snd_Hit[ Calc_GetRandVal( COUNTOF( sm_Snd_Hit )) ];
		}
	}

	Sound(id);
}

int CChar::ItemPickup(CItem * pItem, int amount)
{
	ADDTOCALLSTACK("CChar::ItemPickup");
	// Pickup off the ground or remove my own equipment. etc..
	// This item is now "up in the air"
	// RETURN:
	//  amount we can pick up.
	//	-1 = we cannot pick this up.

	if (( amount < 0 ) || !pItem )
		return -1;
	if ( pItem->GetParent() == this && pItem->GetEquipLayer() == LAYER_HORSE )
		return -1;
	if (( pItem->GetParent() == this ) && ( pItem->GetEquipLayer() == LAYER_DRAGGING ))
		return pItem->GetAmount();
	if ( !CanTouch(pItem) || !CanMove(pItem, true) )
		return -1;

	const CObjBaseTemplate * pObjTop = pItem->GetTopLevelObj();

	if( IsClient() )
	{
		CClient * client = GetClient();
		const CItem * pItemCont	= dynamic_cast <const CItem*> (pItem->GetParent());
		
		if ( pItemCont != NULL )
		{
			// Don't allow taking items from the bank unless we opened it here
			if ( pItemCont->IsType( IT_EQ_BANK_BOX ) && ( pItemCont->m_itEqBankBox.m_pntOpen != GetTopPoint() ) )
				return -1;

			// Check sub containers too
			CChar * pCharTop = dynamic_cast<CChar *>(const_cast<CObjBaseTemplate *>(pObjTop));
			if ( pCharTop != NULL )
			{
				bool bItemContIsInsideBankBox = pCharTop->GetBank()->IsItemInside( pItemCont );
				if ( bItemContIsInsideBankBox && ( pCharTop->GetBank()->m_itEqBankBox.m_pntOpen != GetTopPoint() ))
					return -1;
			}

			// protect from ,snoop - disallow picking from not opened containers
			bool isInOpenedContainer = false;
			CClient::OpenedContainerMap_t::iterator itContainerFound = client->m_openedContainers.find( pItemCont->GetUID().GetPrivateUID() );
			if ( itContainerFound != client->m_openedContainers.end() )
			{
				DWORD dwTopContainerUID = (((*itContainerFound).second).first).first;
				DWORD dwTopMostContainerUID = (((*itContainerFound).second).first).second;
				CPointMap ptOpenedContainerPosition = ((*itContainerFound).second).second;

				DWORD dwTopContainerUID_ToCheck = 0;
				if ( pItemCont->GetContainer() )
					dwTopContainerUID_ToCheck = pItemCont->GetContainer()->GetUID().GetPrivateUID();
				else
					dwTopContainerUID_ToCheck = pObjTop->GetUID().GetPrivateUID();

				if ( ( dwTopMostContainerUID == pObjTop->GetUID().GetPrivateUID() ) && ( dwTopContainerUID == dwTopContainerUID_ToCheck ) )
				{
					if ( pCharTop != NULL )
					{
						isInOpenedContainer = true;
						// probably a pickup check from pack if pCharTop != this?
					}
					else
					{
						CItem * pItemTop = dynamic_cast<CItem *>(const_cast<CObjBaseTemplate *>(pObjTop));
						if ( pItemTop && (pItemTop->IsType(IT_SHIP_HOLD) || pItemTop->IsType(IT_SHIP_HOLD_LOCK)) && (pItemTop->GetTopPoint().GetRegion(REGION_TYPE_MULTI) == GetTopPoint().GetRegion(REGION_TYPE_MULTI)) )
						{
							isInOpenedContainer = true;
						}
						else if ( ptOpenedContainerPosition.GetDist( pObjTop->GetTopPoint() ) <= 3 )
						{
							isInOpenedContainer = true;
						}
					}
				}
			}

			if( !isInOpenedContainer )
				return -1;
		}
	}

	const CChar * pChar = dynamic_cast <const CChar*> (pObjTop);

	if ( pChar != this &&
		pItem->IsAttr(ATTR_OWNED) &&
		pItem->m_uidLink != GetUID() &&
		!IsPriv(PRIV_ALLMOVE|PRIV_GM))
	{
		SysMessageDefault(DEFMSG_MSG_STEAL);
		return -1;
	}

	const CItemCorpse * pCorpse = dynamic_cast<const CItemCorpse *>(pObjTop);
	if ( pCorpse && pCorpse->m_uidLink != GetUID() )
	{
		CheckCorpseCrime(pCorpse, true, false);
		Reveal();
	}

	int iAmountMax = pItem->GetAmount();
	if ( iAmountMax <= 0 )
		return -1;

	if ( !pItem->Item_GetDef()->IsStackableType() )
		amount = iAmountMax;	// it's not stackable, so we must pick up the entire amount
	else
		amount = maximum(1, minimum(amount, iAmountMax));

	//int iItemWeight = ( amount == iAmountMax ) ? pItem->GetWeight() : pItem->Item_GetDef()->GetWeight() * amount;
	int iItemWeight = pItem->GetWeight(static_cast<WORD>(amount));

	// Is it too heavy to even drag ?
	bool fDrop = false;
	if ( GetWeightLoadPercent(GetTotalWeight() + iItemWeight) > 300 )
	{
		SysMessageDefault(DEFMSG_MSG_HEAVY);
		if (( pChar == this ) && ( pItem->GetParent() == GetPack() ))
		{
			fDrop = true;	// we can always drop it out of own pack !
		}
		return -1;
	}

	ITRIG_TYPE trigger;
	if ( pChar != NULL )
	{
		bool bCanTake = false;
		if (pChar == this) // we can always take our own items
			bCanTake = true;
		else if (pItem->GetContainer() != pChar || g_Cfg.m_fCanUndressPets == true) // our owners can take items from us (with CanUndressPets=true, they can undress us too)
			bCanTake = pChar->NPC_IsOwnedBy(this);
		else  // higher priv players can take items and undress us
			bCanTake = IsPriv(PRIV_GM) && GetPrivLevel() > pChar->GetPrivLevel();

		if (bCanTake == false)
		{
			SysMessageDefault(DEFMSG_MSG_STEAL);
			return -1;
		}
		trigger = pItem->IsItemEquipped() ? ITRIG_UNEQUIP : ITRIG_PICKUP_PACK;
	}
	else
	{
		trigger = pItem->IsTopLevel() ? ITRIG_PICKUP_GROUND : ITRIG_PICKUP_PACK;
	}

	if ( trigger == ITRIG_PICKUP_GROUND )
	{
		//	bug with taking static/movenever items -or- catching the spell effects
		if ( IsPriv(PRIV_ALLMOVE|PRIV_GM) ) ;
		else if ( pItem->IsAttr(ATTR_STATIC|ATTR_MOVE_NEVER) || pItem->IsType(IT_SPELL) )
			return -1;
	}

	if ( trigger != ITRIG_UNEQUIP )	// unequip is done later.
	{
		if (( IsTrigUsed(CItem::sm_szTrigName[trigger]) ) || ( IsTrigUsed(sm_szTrigName[(CTRIG_itemAfterClick - 1) + trigger]) )) //ITRIG_PICKUP_GROUND, ITRIG_PICKUP_PACK
		{
			CScriptTriggerArgs Args( amount );
			if ( pItem->OnTrigger( trigger, this, &Args ) == TRIGRET_RET_TRUE )
				return( -1 );
		}
		if (( trigger == ITRIG_PICKUP_PACK ) && (( IsTrigUsed(TRIGGER_PICKUP_SELF) ) || ( IsTrigUsed(TRIGGER_ITEMPICKUP_SELF) )))
		{
			CItem * pContItem = dynamic_cast <CItem*> ( pItem->GetContainer() );
			if ( pContItem )
			{
				CScriptTriggerArgs Args1(pItem);
				if ( pContItem->OnTrigger(ITRIG_PICKUP_SELF, this, &Args1) == TRIGRET_RET_TRUE )
					return -1;
			}
		}
	}


	if ( pItem->Item_GetDef()->IsStackableType() && amount )
	{
		// Did we only pick up part of it ?
		// part or all of a pile. Only if pilable !
		if ( amount < iAmountMax )
		{
			// create left over item.
			CItem * pItemNew = pItem->UnStackSplit(amount, this);
			pItemNew->SetTimeout( pItem->GetTimerDAdjusted() ); //since this was commented in DupeCopy

			if (( IsTrigUsed(TRIGGER_PICKUP_STACK) ) || ( IsTrigUsed(TRIGGER_ITEMPICKUP_STACK) ))
			{
				CScriptTriggerArgs Args2(pItemNew);
				if ( pItem->OnTrigger(ITRIG_PICKUP_STACK, this, &Args2) == TRIGRET_RET_TRUE )
					return -1;
			}

		}
	}

	// Do stack dropping if items are stacked
	if (( trigger == ITRIG_PICKUP_GROUND ) && IsSetEF( EF_ItemStackDrop ))
	{
		signed char iItemHeight = maximum(pItem->GetHeight(), 1);
		signed char	iStackMaxZ = GetTopZ() + 16;
		CItem * pStack = NULL;
		CPointMap ptNewPlace = pItem->GetTopPoint();
		CWorldSearch AreaItems(ptNewPlace);
		for (;;)
		{
			pStack = AreaItems.GetItem();
			if ( pStack == NULL )
				break;
			if (( pStack->GetTopZ() <= pItem->GetTopZ()) || ( pStack->GetTopZ() > iStackMaxZ ))
				continue;
			if ( pStack->IsAttr(ATTR_MOVE_NEVER|ATTR_STATIC|ATTR_LOCKEDDOWN))
				continue;

			ptNewPlace = pStack->GetTopPoint();
			ptNewPlace.m_z -= iItemHeight;
			pStack->MoveToUpdate(ptNewPlace);
		}
	}

	if ( fDrop )
	{
		ItemDrop(pItem, GetTopPoint());
		return -1;
	}

	// do the dragging anim for everyone else to see.
	UpdateDrag(pItem);

	// Remove the item from other clients view if the item is
	// being taken from the ground by a hidden character to
	// prevent lingering item.
	if ( ( trigger == ITRIG_PICKUP_GROUND ) && (IsStatFlag( STATF_Insubstantial | STATF_Invisible | STATF_Hidden )) )
        pItem->RemoveFromView( m_pClient );

	// Pick it up.
	pItem->SetDecayTime(-1);	// Kill any decay timer.
	CItemSpawn * pSpawn = static_cast<CItemSpawn*>(pItem->m_uidSpawnItem.ItemFind());
	if (pSpawn)
		pSpawn->DelObj(pItem->GetUID());
	LayerAdd( pItem, LAYER_DRAGGING );

	return amount;
}

bool CChar::ItemBounce( CItem * pItem )
{
	ADDTOCALLSTACK("CChar::ItemBounce");
	// We can't put this where we want to
	// So put in my pack if i can. else drop.
	// don't check where this came from !
	if ( pItem == NULL )
		return false;

	CItemContainer * pPack = GetPackSafe();
	if ( pItem->GetParent() == pPack )
		return true;

	LPCTSTR pszWhere = NULL;
	if ( pPack && CanCarry(pItem) )		// this can happen at load time
	{
		pszWhere = g_Cfg.GetDefaultMsg( DEFMSG_MSG_BOUNCE_PACK );
		pItem->RemoveFromView();
		pPack->ContentAdd(pItem);		// add it to pack
		Sound(pItem->GetDropSound(pPack));
	}
	else
	{
		if ( !GetTopPoint().IsValidPoint() )
		{
			// NPC is being created and has no valid point yet.
			if ( pszWhere )
				DEBUG_ERR(("No pack to place loot item '%s' for NPC '%s'\n", pItem->GetResourceName(), GetResourceName()));
			else
				DEBUG_ERR(("Loot item %s too heavy for NPC %s\n", pItem->GetResourceName(), GetResourceName()));

			pItem->Delete();
			return false;
		}
		pszWhere = g_Cfg.GetDefaultMsg( DEFMSG_MSG_FEET );
		pItem->RemoveFromView();
		pItem->MoveToDecay(GetTopPoint(), pItem->GetDecayTime());	// drop it on ground
	}

	SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_MSG_ITEMPLACE ), pItem->GetName(), pszWhere );
	return true;
}

bool CChar::ItemDrop( CItem * pItem, const CPointMap & pt )
{
	ADDTOCALLSTACK("CChar::ItemDrop");
	// A char actively drops an item on the ground.
	if ( pItem == NULL )
		return( false );

	if ( IsSetEF( EF_ItemStacking ) )
	{
		CGrayMapBlockState block( CAN_C_WALK, pt.m_z, pt.m_z, pt.m_z, maximum(pItem->GetHeight(), 1) );
		//g_World.GetHeightPoint( pt, block, true );
		//DEBUG_ERR(("Drop: %d / Min: %d / Max: %d\n", pItem->GetFixZ(pt), block.m_Bottom.m_z, block.m_Top.m_z));

		CPointMap ptStack = pt;
		signed char iStackMaxZ = block.m_Top.m_z;	//pt.m_z + 16;
		CItem * pStack = NULL;
		CWorldSearch AreaItems(ptStack);
		for (;;)
		{
			pStack = AreaItems.GetItem();
			if ( pStack == NULL )
				break;
			if ( pStack->GetTopZ() < pt.m_z || pStack->GetTopZ() > iStackMaxZ )
				continue;

			ptStack.m_z += maximum(pStack->GetHeight(), 1);
			//DEBUG_ERR(("(%d > %d) || (%d > %d)\n", ptStack.m_z, iStackMaxZ, ptStack.m_z + maximum(pItem->GetHeight(), 1), iStackMaxZ + 3));
			if ( (ptStack.m_z > iStackMaxZ) || (ptStack.m_z + maximum(pItem->GetHeight(), 1) > iStackMaxZ + 3) )
			{
				ItemBounce( pItem );		// put the item on backpack (or drop it on ground if it's too heavy)
				return false;
			}
		}
		return( pItem->MoveToCheck( ptStack, this ));	// don't flip the item if it got stacked
	}

	// Does this item have a flipped version?
	CItemBase * pItemDef = pItem->Item_GetDef();
	if (( g_Cfg.m_fFlipDroppedItems || pItemDef->Can(CAN_I_FLIP)) && pItem->IsMovableType() && !pItemDef->IsStackableType())
		pItem->SetDispID( pItemDef->GetNextFlipID( pItem->GetDispID()));

	return( pItem->MoveToCheck( pt, this ));
}

bool CChar::ItemEquip( CItem * pItem, CChar * pCharMsg, bool fFromDClick )
{
	ADDTOCALLSTACK("CChar::ItemEquip");
	// Equip visible stuff. else throw into our pack.
	// Pay no attention to where this came from.
	// Bounce anything in the slot we want to go to. (if possible)
	// NOTE: This can be used from scripts as well to equip memories etc.
	// ASSUME this is ok for me to use. (movable etc)

	if ( !pItem )
		return false;

	// In theory someone else could be dressing me ?
	if ( !pCharMsg )
		pCharMsg = this;

	if ( pItem->GetParent() == this )
	{
		if ( pItem->GetEquipLayer() != LAYER_DRAGGING ) // already equipped.
			return true;
	}


	if (( IsTrigUsed(TRIGGER_EQUIPTEST) ) || ( IsTrigUsed(TRIGGER_ITEMEQUIPTEST) ))
	{
		TRIGRET_TYPE iRet = pItem->OnTrigger(ITRIG_EQUIPTEST, this);

		if ( pItem->IsDeleted() )
			return false;

		if ( iRet == TRIGRET_RET_TRUE )
		{
			if ( pItem->GetEquipLayer() == LAYER_DRAGGING ) // dragging? else just do nothing
			{
				pItem->RemoveSelf();
				ItemBounce(pItem);
			}
			return false;
		}
	}

	// strong enough to equip this . etc ?
	// Move stuff already equipped.
   	if ( pItem->GetAmount() > 1 )
		pItem->UnStackSplit(1, this);
	// remove it from the container so that nothing will be stacked with it if unequipped
	pItem->RemoveSelf();

	LAYER_TYPE layer = CanEquipLayer( pItem, LAYER_QTY, pCharMsg, false );

	if ( layer == LAYER_NONE )
	{
		ItemBounce(pItem);
		return false;
	}

	pItem->SetDecayTime(-1);	// Kill any decay timer.
	LayerAdd(pItem, layer);
	if ( !pItem->IsItemEquipped() )	// Equip failed ? (cursed?) Did it just go into pack ?
		return false;

	if (( IsTrigUsed(TRIGGER_EQUIP) ) || ( IsTrigUsed(TRIGGER_ITEMEQUIP) ))
	{
		if ( pItem->OnTrigger(ITRIG_EQUIP, this) == TRIGRET_RET_TRUE )
			return false;
	}

	if ( !pItem->IsItemEquipped() )	// Equip failed ? (cursed?) Did it just go into pack ?
		return false;

	Spell_Effect_Add(pItem);	// if it has a magic effect.

	if ( CItemBase::IsVisibleLayer(layer) )	// visible layer ?
		Sound(0x057);

	if ( fFromDClick )
		pItem->ResendOnEquip();

	if (pItem->IsTypeArmorWeapon())
	{
		SetDefNum("DAMPHYSICAL", GetDefNum("DAMPHYSICAL", true) + pItem->GetDefNum("DAMPHYSICAL", true, true));
		SetDefNum("DAMFIRE", GetDefNum("DAMFIRE", true) + pItem->GetDefNum("DAMFIRE", true, true));
		SetDefNum("DAMCOLD", GetDefNum("DAMCOLD", true) + pItem->GetDefNum("DAMCOLD", true, true));
		SetDefNum("DAMPOISON", GetDefNum("DAMPOISON", true) + pItem->GetDefNum("DAMPOISON", true, true));
		SetDefNum("DAMENERGY", GetDefNum("DAMENERGY", true) + pItem->GetDefNum("DAMENERGY", true, true));

		SetDefNum("RESPHYSICAL", GetDefNum("RESPHYSICAL", true) + pItem->GetDefNum("RESPHYSICAL", true, true));
		SetDefNum("RESFIRE", GetDefNum("RESFIRE", true) + pItem->GetDefNum("RESFIRE", true, true));
		SetDefNum("RESCOLD", GetDefNum("RESCOLD", true) + pItem->GetDefNum("RESCOLD", true, true));
		SetDefNum("RESPOISON", GetDefNum("RESPOISON", true) + pItem->GetDefNum("RESPOISON", true, true));
		SetDefNum("RESENERGY", GetDefNum("RESENERGY", true) + pItem->GetDefNum("RESENERGY", true, true));
	}

	if (pItem->IsTypeWeapon())
	{
		CItem * pCursedMemory = LayerFind(LAYER_SPELL_Curse_Weapon);	// Mark the weapon as cursed if SPELL_Curse_Weapon is active.
		if (pCursedMemory)
			pItem->SetDefNum("HitLeechLife", pItem->GetDefNum("HitLeechLife", true) + pCursedMemory->m_itSpell.m_spelllevel, true);
	}

	short iStrengthBonus = static_cast<short>(pItem->GetDefNum("BONUSSTR", true, true));
	if (iStrengthBonus != 0)
		Stat_SetMod(STAT_STR, Stat_GetMod(STAT_STR) + iStrengthBonus);

	short iDexterityBonus = static_cast<short>(pItem->GetDefNum("BONUSDEX", true, true));
	if (iDexterityBonus != 0)
		Stat_SetMod(STAT_DEX, Stat_GetMod(STAT_DEX) + iDexterityBonus);

	short iIntelligenceBonus = static_cast<short>(pItem->GetDefNum("BONUSINT", true, true));
	if (iIntelligenceBonus != 0)
		Stat_SetMod(STAT_INT, Stat_GetMod(STAT_INT) + iIntelligenceBonus);

	int iHitpointIncrease = static_cast<int>(pItem->GetDefNum("BONUSHITS", true, true));
	if (iHitpointIncrease != 0)
		Stat_SetMax(STAT_STR, Stat_GetMax(STAT_STR) + iHitpointIncrease);

	int iStaminaIncrease = static_cast<int>(pItem->GetDefNum("BONUSSTAM", true, true));
	if (iStaminaIncrease != 0)
		Stat_SetMax(STAT_DEX, Stat_GetMax(STAT_DEX) + iStaminaIncrease);

	int iManaIncrease = static_cast<int>(pItem->GetDefNum("BONUSMANA", true, true));
	if (iManaIncrease != 0)
		Stat_SetMax(STAT_INT, Stat_GetMax(STAT_INT) + iManaIncrease);

	INT64 iDamageIncrease = pItem->GetDefNum("INCREASEDAM", true, true);
	if (iDamageIncrease != 0)
		SetDefNum("INCREASEDAM", GetDefNum("INCREASEDAM", true) + iDamageIncrease);

	INT64 iDefenseChanceIncrease = pItem->GetDefNum("INCREASEDEFCHANCE", true, true);
	if (iDefenseChanceIncrease != 0)
		SetDefNum("INCREASEDEFCHANCE", GetDefNum("INCREASEDEFCHANCE", true) + iDefenseChanceIncrease);

	INT64 iFasterCasting = pItem->GetDefNum("FASTERCASTING", true, true);
	if (iFasterCasting != 0)
		SetDefNum("FASTERCASTING", GetDefNum("FASTERCASTING", true) + iFasterCasting);

	INT64 iHitChanceIncrease = pItem->GetDefNum("INCREASEHITCHANCE", true, true);
	if (iHitChanceIncrease != 0)
		SetDefNum("INCREASEHITCHANCE", GetDefNum("INCREASEHITCHANCE", true) + iHitChanceIncrease);

	INT64 iSpellDamageIncrease = pItem->GetDefNum("INCREASESPELLDAM", true, true);
	if (iSpellDamageIncrease != 0)
		SetDefNum("INCREASESPELLDAM", GetDefNum("INCREASESPELLDAM", true) + iSpellDamageIncrease);

	INT64 iSwingSpeedIncrease = pItem->GetDefNum("INCREASESWINGSPEED", true, true);
	if (iSwingSpeedIncrease != 0)
		SetDefNum("INCREASESWINGSPEED", GetDefNum("INCREASESWINGSPEED", true) + iSwingSpeedIncrease);

	INT64 iEnhancePotions = pItem->GetDefNum("ENHANCEPOTIONS", true, true);
	if (iEnhancePotions != 0)
		SetDefNum("ENHANCEPOTIONS", GetDefNum("ENHANCEPOTIONS", true) + iEnhancePotions);

	INT64 iLowerManaCost = pItem->GetDefNum("LOWERMANACOST", true, true);
	if (iLowerManaCost != 0)
		SetDefNum("LOWERMANACOST", GetDefNum("LOWERMANACOST", true) + iLowerManaCost);

	INT64 iLuck = pItem->GetDefNum("LUCK", true, true);
	if (iLuck != 0)
		SetDefNum("LUCK", GetDefNum("LUCK", true) + iLuck);

	if (pItem->GetDefNum("NIGHTSIGHT", true, true))
	{
		StatFlag_Mod(STATF_NightSight, 1);
		if (IsClient())
			m_pClient->addLight();
	}

	return true;
}

void CChar::EatAnim( LPCTSTR pszName, int iQty )
{
	ADDTOCALLSTACK("CChar::EatAnim");
	static const SOUND_TYPE sm_EatSounds[] = { 0x03a, 0x03b, 0x03c };
	Sound(sm_EatSounds[Calc_GetRandVal(COUNTOF(sm_EatSounds))]);

	if ( !IsStatFlag(STATF_OnHorse) )
		UpdateAnimate(ANIM_EAT);

	TCHAR * pszMsg = Str_GetTemp();
	sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EATSOME), static_cast<LPCTSTR>(pszName));
	Emote(pszMsg);

	int iHits = 0;
	int iMana = 0;
	int iStam = Calc_GetRandVal2(3, 6) + (iQty / 5);
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

		iHits = static_cast<int>(Args.m_VarsLocal.GetKeyNum("Hits", true)) + Stat_GetVal(STAT_STR);
		iMana = static_cast<int>(Args.m_VarsLocal.GetKeyNum("Mana", true)) + Stat_GetVal(STAT_INT);
		iStam = static_cast<int>(Args.m_VarsLocal.GetKeyNum("Stam", true)) + Stat_GetVal(STAT_DEX);
		iFood = static_cast<int>(Args.m_VarsLocal.GetKeyNum("Food", true)) + Stat_GetVal(STAT_FOOD);
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

bool CChar::Reveal( DWORD dwFlags )
{
	ADDTOCALLSTACK("CChar::Reveal");
	// Some outside influence may be revealing us.

	if ( !dwFlags )
		dwFlags = STATF_Invisible|STATF_Hidden|STATF_Sleeping;
	if ( !IsStatFlag(dwFlags) )
		return false;

	if ( IsClient() && GetClient()->m_pHouseDesign )
	{
		// No reveal whilst in house design (unless they somehow got out)
		if ( GetClient()->m_pHouseDesign->GetDesignArea().IsInside2d(GetTopPoint()) )
			return false;

		GetClient()->m_pHouseDesign->EndCustomize(true);
	}

	if ( (dwFlags & STATF_Sleeping) && IsStatFlag(STATF_Sleeping) )
		Wake();
	
	if ( (dwFlags & STATF_Invisible) && IsStatFlag(STATF_Invisible) )
	{
		CItem * pSpell = LayerFind(LAYER_SPELL_Invis);
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

	m_StepStealth = 0;
	StatFlag_Clear(dwFlags);
	CClient *pClient = GetClient();
	if ( pClient )
	{
		if ( !IsStatFlag(STATF_Hidden|STATF_Insubstantial) )
			pClient->removeBuff(BI_HIDDEN);
		if ( !IsStatFlag(STATF_Invisible) )
			pClient->removeBuff(BI_INVISIBILITY);
	}

	if ( IsStatFlag(STATF_Invisible|STATF_Hidden|STATF_Insubstantial|STATF_Sleeping) )
		return false;

	UpdateMode(NULL, true);
	SysMessageDefault(DEFMSG_HIDING_REVEALED);
	return true;
}

void CChar::SpeakUTF8( LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang )
{
	ADDTOCALLSTACK("CChar::SpeakUTF8");
	// Ignore the font argument here !

	if ( IsStatFlag(STATF_Stone) )
		return;
	if ( mode == TALKMODE_YELL && GetPrivLevel() >= PLEVEL_Counsel )
		mode = TALKMODE_BROADCAST;		// GM Broadcast (done if a GM yells something)
	if ( m_pNPC )
		wHue = m_pNPC->m_SpeechHue;

	Reveal();
	CObjBase::SpeakUTF8(pszText, wHue, mode, font, lang);
}

void CChar::SpeakUTF8Ex( const NWORD * pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang )
{
	ADDTOCALLSTACK("CChar::SpeakUTF8Ex");
	// Ignore the font argument here !

	if ( IsStatFlag(STATF_Stone) )
		return;
	if ( mode == TALKMODE_YELL && GetPrivLevel() >= PLEVEL_Counsel )
		mode = TALKMODE_BROADCAST;		// GM Broadcast (done if a GM yells something)
	if ( m_pNPC )
		wHue = m_pNPC->m_SpeechHue;

	Reveal();
	CObjBase::SpeakUTF8Ex(pszText, wHue, mode, font, lang);
}

void CChar::Speak( LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font )
{
	ADDTOCALLSTACK("CChar::Speak");
	// Speak to all clients in the area.
	// Ignore the font argument here !

	if ( IsStatFlag(STATF_Stone) )
		return;
	if ( mode == TALKMODE_YELL && GetPrivLevel() >= PLEVEL_Counsel )
		mode = TALKMODE_BROADCAST;		// GM Broadcast (done if a GM yells something)
	if ( m_pNPC )
		wHue = m_pNPC->m_SpeechHue;

	Reveal();
	CObjBase::Speak(pszText, wHue, mode, font);
}

CItem * CChar::Make_Figurine( CGrayUID uidOwner, ITEMID_TYPE id )
{
	ADDTOCALLSTACK("CChar::Make_Figurine");
	// Make me into a figurine
	if ( IsDisconnected() || m_pPlayer )
		return NULL;

	CCharBase* pCharDef = Char_GetDef();

	// turn creature into a figurine.
	CItem * pItem = CItem::CreateScript( ( id == ITEMID_NOTHING ) ? pCharDef->m_trackID : id, this );
	if ( !pItem )
		return NULL;

	pItem->SetType(IT_FIGURINE);
	pItem->SetName(GetName());
	pItem->SetHue(GetHue());
	pItem->m_itFigurine.m_ID = GetID();	// Base type of creature.
	pItem->m_itFigurine.m_UID = GetUID();
	pItem->m_uidLink = uidOwner;

	if ( IsStatFlag(STATF_Insubstantial) )
		pItem->SetAttr(ATTR_INVIS);

	SoundChar(CRESND_RAND1);	// Horse winny
	StatFlag_Set(STATF_Ridden);
	Skill_Start(NPCACT_RIDDEN);
	SetDisconnected();
	m_atRidden.m_FigurineUID = pItem->GetUID();

	return pItem;
}

CItem * CChar::NPC_Shrink()
{
	ADDTOCALLSTACK("CChar::NPC_Shrink");
	// This will just kill conjured creatures.
	if ( IsStatFlag(STATF_Conjured) )
	{
		Stat_SetVal(STAT_STR, 0);
		return NULL;
	}

	NPC_PetClearOwners();	// Clear follower slots on pet owner

	CItem * pItem = Make_Figurine(UID_CLEAR, ITEMID_NOTHING);
	if ( !pItem )
		return NULL;

	pItem->SetAttr(ATTR_MAGIC);
	pItem->MoveToCheck(GetTopPoint());
	return pItem;
}

CItem * CChar::Horse_GetMountItem() const
{
	ADDTOCALLSTACK("CChar::Horse_GetMountItem");
	// I am a horse.
	// Get my mount object. (attached to my rider)

	if ( ! IsStatFlag( STATF_Ridden ))
		return( NULL );

	CItem * pItem = m_atRidden.m_FigurineUID.ItemFind();

	if ( pItem == NULL )
	{
		CItemMemory* pItemMem = Memory_FindTypes( MEMORY_IPET );

		if ( pItemMem != NULL )
		{
			CChar* pOwner = pItemMem->m_uidLink.CharFind();

			if ( pOwner != NULL )
			{
				CItem* pItemMount = pOwner->LayerFind(LAYER_HORSE);

				if ( pItemMount != NULL && pItemMount->m_itNormal.m_more2 == GetUID() )
				{
					const_cast<CGrayUIDBase&>(m_atRidden.m_FigurineUID) = pItemMount->GetUID();
					pItem = pItemMount;

					DEBUG_ERR(("UID=0%lx, id=0%x '%s', Fixed mount item UID=0%lx, id=0%x '%s'\n",
						(DWORD)GetUID(), GetBaseID(), GetName(), (DWORD)(pItem->GetUID()), pItem->GetBaseID(), pItem->GetName()));
				}
			}
		}
	}

	if ( pItem == NULL ||
		( ! pItem->IsType( IT_FIGURINE ) && ! pItem->IsType( IT_EQ_HORSE )))
	{
		return( NULL );
	}
	return( pItem );
}

CChar * CChar::Horse_GetMountChar() const
{
	// Gets my riding character, if i'm being mounted.
	ADDTOCALLSTACK("CChar::Horse_GetMountChar");
	CItem * pItem = Horse_GetMountItem();
	if ( pItem == NULL )
		return( NULL );
	return( dynamic_cast <CChar*>( pItem->GetTopLevelObj()));
}

bool CChar::Horse_Mount(CChar *pHorse) // Remove horse char and give player a horse item
{
	ADDTOCALLSTACK("CChar::Horse_Mount");
	// RETURN:
	//  true = done mounting
	//  false = we can't mount this

	if ( !CanTouch(pHorse) )
	{
		if ( pHorse->m_pNPC->m_bonded && pHorse->IsStatFlag(STATF_DEAD) )
			SysMessageDefault(DEFMSG_MSG_BONDED_DEAD_CANTMOUNT);
		else
			SysMessageDefault(DEFMSG_MSG_MOUNT_DIST);
		return false;
	}

	ITEMID_TYPE id;
	TCHAR * sMountDefname = Str_GetTemp();
	sprintf(sMountDefname, "mount_0x%x", pHorse->GetDispID());
	id = static_cast<ITEMID_TYPE>(g_Exp.m_VarDefs.GetKeyNum(sMountDefname));
	if ( id <= ITEMID_NOTHING )
		return false;

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

	if ( IsTrigUsed(TRIGGER_MOUNT) )
	{
		CScriptTriggerArgs Args(pHorse);
   		if ( OnTrigger(CTRIG_Mount, this, &Args) == TRIGRET_RET_TRUE )
			return false;
	}

	CItem * pItem = pHorse->Make_Figurine(GetUID(), id);
	if ( !pItem )
		return false;

	// Set a new owner if it is not us (check first to prevent friends taking ownership)
	if ( !pHorse->NPC_IsOwnedBy(this, false) )
		pHorse->NPC_PetSetOwner(this);

	Horse_UnMount();					// unmount if already mounted
	pItem->SetType(IT_EQ_HORSE);
	pItem->SetTimeout(TICK_PER_SEC);	// the first time we give it immediately a tick, then give the horse a tick everyone once in a while.
	LayerAdd(pItem, LAYER_HORSE);		// equip the horse item
	pHorse->StatFlag_Set(STATF_Ridden);
	pHorse->Skill_Start(NPCACT_RIDDEN);
	return true;
}

bool CChar::Horse_UnMount() // Get off a horse (Remove horse item and spawn new horse)
{
	ADDTOCALLSTACK("CChar::Horse_UnMount");
	if ( !IsStatFlag(STATF_OnHorse) || (IsStatFlag(STATF_Stone) && !IsPriv(PRIV_GM)) )
		return false;

	CItem * pItem = LayerFind(LAYER_HORSE);
	if ( pItem == NULL || pItem->IsDeleted() )
	{
		StatFlag_Clear(STATF_OnHorse);	// flag got out of sync !
		return false;
	}

	CChar * pPet = pItem->m_itFigurine.m_UID.CharFind();
	if ( IsTrigUsed(TRIGGER_DISMOUNT) && pPet != NULL && pPet->IsDisconnected() && !pPet->IsDeleted() ) // valid horse for trigger
	{
		CScriptTriggerArgs Args(pPet);
		if ( OnTrigger(CTRIG_Dismount, this, &Args) == TRIGRET_RET_TRUE )
			return false;
	}

	Use_Figurine(pItem, false);
	pItem->Delete();
	return true;
}

bool CChar::OnTickEquip( CItem * pItem )
{
	ADDTOCALLSTACK("CChar::OnTickEquip");
	// A timer expired for an item we are carrying.
	// Does it periodically do something ?
	// RETURN:
	//  false = delete it.
	if ( ! pItem )
		return false;
	switch ( pItem->GetEquipLayer())
	{
		case LAYER_FLAG_Wool:
			// This will regen the sheep it was sheered from.
			// Sheared sheep regen wool on a new day.
			if ( GetID() != CREID_Sheep_Sheered )
				return false;

			// Is it a new day ? regen my wool.
			SetID( CREID_Sheep );
			return false;

		case LAYER_FLAG_ClientLinger:
			// remove me from other clients screens.
			SetDisconnected();
			return( false );

		case LAYER_SPECIAL:
			switch ( pItem->GetType())
			{
				case IT_EQ_SCRIPT:	// pure script.
					break;
				case IT_EQ_MEMORY_OBJ:
				{
					CItemMemory *pMemory = dynamic_cast<CItemMemory*>( pItem );
					if (pMemory)
						return Memory_OnTick(pMemory);

					return false;
				}
				default:
					break;
			}
			break;

		case LAYER_FLAG_Stuck:
			// Only allow me to try to damage the web so often
			// Non-magical. held by something.
			// IT_EQ_STUCK
			pItem->SetTimeout( -1 );
			return( true );

		case LAYER_HORSE:
			// Give my horse a tick. (It is still in the game !)
			// NOTE: What if my horse dies (poisoned?)
			{
				CChar * pHorse = pItem->m_itFigurine.m_UID.CharFind();
				if ( pHorse == NULL )
					return(false);
				if ( pHorse != this )				//Some scripts can force mounts to have as 'mount' the rider itself (like old ethereal scripts)
					return pHorse->OnTick();	// if we call OnTick again on them we'll have an infinite loop.
				pItem->SetTimeout( TICK_PER_SEC );
				return true;
			}

		case LAYER_FLAG_Criminal:
			// update char notoriety when criminal timer goes off
			StatFlag_Clear( STATF_Criminal );
			NotoSave_Update();
			return( false );

		case LAYER_FLAG_Murders:
			// decay the murder count.
			{
				if ( ! m_pPlayer || m_pPlayer->m_wMurders <= 0  )
					return( false );

				CScriptTriggerArgs	args;
				args.m_iN1 = m_pPlayer->m_wMurders-1;
				args.m_iN2 = g_Cfg.m_iMurderDecayTime;

				if ( IsTrigUsed(TRIGGER_MURDERDECAY) )
				{
					OnTrigger(CTRIG_MurderDecay, this, &args);
					if ( args.m_iN1 < 0 ) args.m_iN1 = 0;
					if ( args.m_iN2 < 1 ) args.m_iN2 = g_Cfg.m_iMurderDecayTime;
				}

				m_pPlayer->m_wMurders = static_cast<WORD>(args.m_iN1);
				NotoSave_Update();
				if ( m_pPlayer->m_wMurders == 0 ) return( false );
				pItem->SetTimeout(args.m_iN2);	// update it's decay time.
				return( true );
			}

		default:
			break;
	}

	if ( pItem->IsType( IT_SPELL ))
	{
		return Spell_Equip_OnTick(pItem);
	}

	return( pItem->OnTick());
}

bool CChar::SetPoisonCure( int iSkill, bool fExtra )
{
	ADDTOCALLSTACK("CChar::SetPoisonCure");
	UNREFERENCED_PARAMETER(iSkill);
	// Leave the antidote in your body for a while.
	// iSkill = 0-1000

	CItem * pPoison = LayerFind( LAYER_FLAG_Poison );
	if ( pPoison != NULL )
	{
		// Is it successful ???
		pPoison->Delete();
	}
	if ( fExtra )
	{
		pPoison = LayerFind( LAYER_FLAG_Hallucination );
		if ( pPoison != NULL )
		{
			// Is it successful ???
			pPoison->Delete();
		}
	}
	UpdateModeFlag();
	return( true );
}

bool CChar::SetPoison( int iSkill, int iTicks, CChar * pCharSrc )
{
	ADDTOCALLSTACK("CChar::SetPoison");
	// SPELL_Poison
	// iSkill = 0-1000 = how bad the poison is
	// iTicks = how long to last. Should be 0 with MAGIFC_OSIFORMULAS enabled to calculate defaults
	// Physical attack of poisoning.

	if ( IsStatFlag( STATF_Conjured ))
	{
		// conjured creatures cannot be poisoned.
		return false;
	}
	CItem * pPoison;
	if ( IsStatFlag( STATF_Poisoned ))
	{
		// strengthen the poison ?
		pPoison = LayerFind( LAYER_FLAG_Poison );
		if (pPoison && !IsSetMagicFlags(MAGICF_OSIFORMULAS))
		{
			pPoison->m_itSpell.m_spellcharges += iTicks;
		}
		return false;
	}

	SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_JUST_BEEN_POISONED ) );

	// Release if paralyzed ?
	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(SPELL_Poison);
	if (!pSpellDef->IsSpellType(SPELLFLAG_NOUNPARALYZE))
		StatFlag_Clear( STATF_Freeze );	// remove paralyze.

	// Might be a physical vs. Magical attack.
	pPoison = Spell_Effect_Create(SPELL_Poison, LAYER_FLAG_Poison, iSkill, 1 + Calc_GetRandVal(2)*TICK_PER_SEC, pCharSrc, false );
	if (IsSetMagicFlags(MAGICF_OSIFORMULAS))
	{
		// If caster have more than 100.0 in magery and poisoning and it's distance is lesser than 3 tiles, he has a 10% change to inflict lethal poison
		if (pCharSrc->Skill_GetBase(SKILL_MAGERY) > 1000 && pCharSrc->Skill_GetBase(SKILL_POISONING) > 1000 && GetDist(pCharSrc) < 3 && Calc_GetRandVal(10) == 1)
		{
			pPoison->m_itSpell.m_pattern = static_cast<unsigned char>(IMULDIV(Stat_GetMax(STAT_STR), Calc_GetRandVal2(16, 33), 100));
			pPoison->m_itSpell.m_spelllevel = 4;
			pPoison->m_itSpell.m_spellcharges = 80;	//1 min / 20 sec
		}
		else if (iSkill <= 600)	// Lesser
		{
			pPoison->m_itSpell.m_spelllevel = 0;
			pPoison->m_itSpell.m_spellcharges = 30;
		}
		else if (iSkill < 851) // Normal
		{
			pPoison->m_itSpell.m_pattern = static_cast<unsigned char>(IMULDIV(Stat_GetMax(STAT_STR), Calc_GetRandVal2(5, 10), 100));
			pPoison->m_itSpell.m_spelllevel = 1;
			pPoison->m_itSpell.m_spellcharges = 30;
		}
		else if (iSkill < 1000) // Greater
		{
			pPoison->m_itSpell.m_pattern = static_cast<unsigned char>(IMULDIV(Stat_GetMax(STAT_STR), Calc_GetRandVal2(7, 15), 100));
			pPoison->m_itSpell.m_spelllevel = 2;
			pPoison->m_itSpell.m_spellcharges = 60;
		}
		else	// Deadly.
		{
			pPoison->m_itSpell.m_pattern = static_cast<unsigned char>(IMULDIV(Stat_GetMax(STAT_STR), Calc_GetRandVal2(15, 30), 100));
			pPoison->m_itSpell.m_spelllevel = 3;
			pPoison->m_itSpell.m_spellcharges = 60;
		}
		if (iTicks > 0)
			pPoison->m_itSpell.m_spellcharges = iTicks;
	}
	else
	{
		pPoison->m_itSpell.m_spellcharges = iTicks;	// how long to last.
	}

	if (IsAosFlagEnabled(FEATURE_AOS_UPDATE_B))
	{
		CItem * pEvilOmen = LayerFind(LAYER_SPELL_Evil_Omen);
		if (pEvilOmen)
		{
			pPoison->m_itSpell.m_spelllevel++;	// Effect 2: next poison will have one additional level of poison.
			pEvilOmen->Delete();
		}
	}

	LayerAdd(pPoison, LAYER_FLAG_Poison);	// Creating after setting the whole memory
	UpdateStatsFlag();
	return( true );
}

void CChar::Wake()
{
	ADDTOCALLSTACK("CChar::Wake");
	if (!IsStatFlag(STATF_Sleeping))
		return;

	CItemCorpse *pCorpse = FindMyCorpse(true);
	if (pCorpse == NULL)
	{
		Stat_SetVal(STAT_STR, 0);		// death
		return;
	}

	RaiseCorpse(pCorpse);
	StatFlag_Clear(STATF_Sleeping);
	UpdateMode();
}

void CChar::SleepStart( bool fFrontFall )
{
	ADDTOCALLSTACK("CChar::SleepStart");
	if (IsStatFlag(STATF_DEAD|STATF_Sleeping|STATF_Polymorph))
		return;

	CItemCorpse *pCorpse = MakeCorpse(fFrontFall);
	if (pCorpse == NULL)
	{
		SysMessageDefault(DEFMSG_MSG_CANTSLEEP);
		return;
	}

	// Play death animation (fall on ground)
	UpdateCanSee(new PacketDeath(this, pCorpse), m_pClient);

	SetID(m_prev_id);
	StatFlag_Set(STATF_Sleeping);
	StatFlag_Clear(STATF_Hidden);
	UpdateMode();
}

CItemCorpse * CChar::MakeCorpse( bool fFrontFall )
{
	ADDTOCALLSTACK("CChar::MakeCorpse");
	// Create the char corpse when it die (STATF_DEAD) or fall asleep (STATF_Sleeping)
	// Summoned (STATF_Conjured) and some others creatures have no corpse.

	WORD wFlags = static_cast<WORD>(m_TagDefs.GetKeyNum("DEATHFLAGS", true));
	if (wFlags & DEATH_NOCORPSE)
		return( NULL );
	if (IsStatFlag(STATF_Conjured) && !(wFlags & (DEATH_NOCONJUREDEFFECT|DEATH_HASCORPSE)))
	{
		Effect(EFFECT_XYZ, ITEMID_FX_SPELL_FAIL, this, 1, 30);
		return( NULL );
	}

	CItemCorpse *pCorpse = dynamic_cast<CItemCorpse *>(CItem::CreateScript(ITEMID_CORPSE, this));
	if (pCorpse == NULL)	// weird internal error
		return( NULL );

	TCHAR *pszMsg = Str_GetTemp();
	sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MSG_CORPSE_OF), GetName());
	pCorpse->SetName(pszMsg);
	pCorpse->SetHue(GetHue());
	pCorpse->SetCorpseType(GetDispID());
	pCorpse->SetAttr(ATTR_MOVE_NEVER);
	pCorpse->m_itCorpse.m_BaseID = m_prev_id;	// id the corpse type here !
	pCorpse->m_itCorpse.m_facing_dir = m_dirFace;
	pCorpse->m_uidLink = GetUID();

	// TO-DO: Fix SRC (or 'this') always seeing the corpse to the same dir (it works fine
	// for nearby clients but not for SRC). Probably it could be something related to the
	// Update() function making the corpse always get back to same default dir.

	if (fFrontFall)
		pCorpse->m_itCorpse.m_facing_dir = static_cast<DIR_TYPE>(m_dirFace|0x80);

	int iDecayTimer = -1;	// never decay
	if (IsStatFlag(STATF_DEAD))
	{
		iDecayTimer = (m_pPlayer) ? g_Cfg.m_iDecay_CorpsePlayer : g_Cfg.m_iDecay_CorpseNPC;
		pCorpse->SetTimeStamp(CServTime::GetCurrentTime().GetTimeRaw());	// death time
		if (Attacker_GetLast())
			pCorpse->m_itCorpse.m_uidKiller = Attacker_GetLast()->GetUID();
		else
			pCorpse->m_itCorpse.m_uidKiller.InitUID();
	}
	else	// sleeping (not dead)
	{
		pCorpse->SetTimeStamp(0);
		pCorpse->m_itCorpse.m_uidKiller = GetUID();
	}

	if ((m_pNPC && m_pNPC->m_bonded) || IsStatFlag(STATF_Conjured|STATF_Sleeping))
		pCorpse->m_itCorpse.m_carved = 1;	// corpse of bonded and summoned creatures (or sleeping players) can't be carved

	if ( !(wFlags & DEATH_NOLOOTDROP) )		// move non-newbie contents of the pack to corpse
		DropAll( pCorpse );

	pCorpse->SetKeyNum("OVERRIDE.MAXWEIGHT", g_Cfg.Calc_MaxCarryWeight(this) / 10);		// set corpse maxweight to prevent weird exploits like when someone place many items on an player corpse just to make this player get stuck on resurrect
	pCorpse->MoveToDecay(GetTopPoint(), iDecayTimer);
	return( pCorpse );
}

bool CChar::RaiseCorpse( CItemCorpse * pCorpse )
{
	ADDTOCALLSTACK("CChar::RaiseCorpse");
	// We are creating a char from the current char and the corpse.
	// Move the items from the corpse back onto us.

	if ( !pCorpse )
		return false;

	if ( pCorpse->GetCount() > 0 )
	{
		CItemContainer * pPack = GetPackSafe();

		CItem * pItemNext;
		for ( CItem * pItem = pCorpse->GetContentHead(); pItem != NULL; pItem = pItemNext )
		{
			pItemNext = pItem->GetNext();
			if ( pItem->IsType(IT_HAIR) || pItem->IsType(IT_BEARD) )	// hair on corpse was copied!
				continue;

			if ( pItem->GetContainedLayer() )
				ItemEquip(pItem);
			else
				pPack->ContentAdd(pItem);
		}

		pCorpse->ContentsDump( GetTopPoint());		// drop left items on ground
	}

	UpdateAnimate((pCorpse->m_itCorpse.m_facing_dir & 0x80) ? ANIM_DIE_FORWARD : ANIM_DIE_BACK, true, true);
	pCorpse->Delete();
	return true;
}

bool CChar::Death()
{
	ADDTOCALLSTACK("CChar::Death");
	// RETURN: false = delete

	if ( IsStatFlag(STATF_DEAD|STATF_INVUL) )
		return true;

	if ( IsTrigUsed(TRIGGER_DEATH) )
	{
		if ( OnTrigger(CTRIG_Death, this) == TRIGRET_RET_TRUE )
			return true;
	}

	// Look through memories of who I was fighting (make sure they knew they where fighting me)
	CItem * pItemNext;
	CItem * pItem;
	for ( pItem = GetContentHead(); pItem; pItem = pItemNext)
	{
		pItemNext = pItem->GetNext();

		if ( pItem->IsType(IT_EQ_TRADE_WINDOW) )
		{
			CItemContainer * pCont = dynamic_cast<CItemContainer *>(pItem);
			if ( pCont )
			{
				pCont->Trade_Delete();
				continue;
			}
		}

		if ( pItem->IsMemoryTypes(MEMORY_HARMEDBY|MEMORY_WAR_TARG) )
			Memory_ClearTypes( static_cast<CItemMemory *>(pItem), 0xFFFF );
	}

	// Give credit for the kill to my attacker(s)
	int iKillers = 0;
	CChar * pKiller = NULL;
	TCHAR * pszKillStr = Str_GetTemp();
	int iKillStrLen = sprintf( pszKillStr, g_Cfg.GetDefaultMsg(DEFMSG_MSG_KILLED_BY), (m_pPlayer)? 'P':'N', GetNameWithoutIncognito() );
	for ( size_t count = 0; count < m_lastAttackers.size(); count++ )
	{
		pKiller = CGrayUID(m_lastAttackers.at(count).charUID).CharFind();
		if ( pKiller )
		{
			if ( IsTrigUsed(TRIGGER_KILL) )
			{
				CScriptTriggerArgs args(this);
				args.m_iN1 = Attacker();
				if ( pKiller->OnTrigger(CTRIG_Kill, pKiller, &args) == TRIGRET_RET_TRUE )
					continue;
			}

			pKiller->Noto_Kill( this, IsStatFlag(STATF_Pet), Attacker() );
			iKillStrLen += sprintf( pszKillStr+iKillStrLen, "%s%c'%s'", iKillers ? ", " : "", (pKiller->m_pPlayer) ? 'P':'N', pKiller->GetNameWithoutIncognito() );
			++iKillers;
		}
	}

	// Record the kill event for posterity
	if ( !iKillers )
		iKillStrLen += sprintf( pszKillStr+iKillStrLen, "accident" );
	if ( m_pPlayer )
		g_Log.Event( LOGL_EVENT|LOGM_KILLS, "%s\n", pszKillStr );
	if ( m_pParty )
		m_pParty->SysMessageAll( pszKillStr );

	Reveal();
	SoundChar(CRESND_DIE);
	StatFlag_Set(STATF_DEAD);
	StatFlag_Clear(STATF_Stone|STATF_Freeze|STATF_Hidden|STATF_Sleeping|STATF_Hovering);
	SetPoisonCure(0, true);
	Skill_Cleanup();
	Spell_Dispel(100);			// get rid of all spell effects (moved here to prevent double @Destroy trigger)
	m_lastAttackers.clear();	// clear list of attackers

	if ( m_pPlayer )		// if I'm NPC then my mount goes with me
		Horse_UnMount();

	// Create the corpse item
	CItemCorpse * pCorpse = MakeCorpse(Calc_GetRandVal(2) > 1 ? true : false);
	if ( pCorpse )
	{
		if ( IsTrigUsed(TRIGGER_DEATHCORPSE) )
		{
			CScriptTriggerArgs Args(pCorpse);
			OnTrigger(CTRIG_DeathCorpse, this, &Args);
		}
	}

	// Play death animation (fall on ground)
	UpdateCanSee(new PacketDeath(this, pCorpse), m_pClient);

	if ( m_pNPC )
	{
		if ( m_pNPC->m_bonded )
		{
			m_Can |= CAN_C_GHOST;
			UpdateMode(NULL, true);
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
		if ( !(m_TagDefs.GetKeyNum("DEATHFLAGS", true) & DEATH_NOFAMECHANGE) )
			Noto_Fame( -Stat_GetAdjusted(STAT_FAME)/10 );

		LPCTSTR pszGhostName = NULL;
		CCharBase *pCharDefPrev = CCharBase::FindCharBase( m_prev_id );
		switch ( m_prev_id )
		{
			case CREID_GARGMAN:
			case CREID_GARGWOMAN:
				pszGhostName = ( pCharDefPrev && pCharDefPrev->IsFemale() ? "c_garg_ghost_woman" : "c_garg_ghost_man" );
				break;
			case CREID_ELFMAN:
			case CREID_ELFWOMAN:
				pszGhostName = ( pCharDefPrev && pCharDefPrev->IsFemale() ? "c_elf_ghost_woman" : "c_elf_ghost_man" );
				break;
			default:
				pszGhostName = ( pCharDefPrev && pCharDefPrev->IsFemale() ? "c_ghost_woman" : "c_ghost_man" );
				break;
		}
		ASSERT(pszGhostName != NULL);

		if ( !IsStatFlag(STATF_War) )
			StatFlag_Set(STATF_Insubstantial);	// manifest war mode for ghosts

		m_pPlayer->m_wDeaths++;
		SetHue( HUE_DEFAULT );	// get all pale
		SetID( static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType( RES_CHARDEF, pszGhostName )) );
		LayerAdd( CItem::CreateScript( ITEMID_DEATHSHROUD, this ) );

		CClient * pClient = GetClient();
		if ( pClient )
		{
			// OSI uses PacketDeathMenu to update client screen on death.
			// If the user disable this packet, it must be updated using PacketPlayerPosition
			if ( g_Cfg.m_iPacketDeathAnimation )
			{
				// Display death animation to client ("You are dead")
				new PacketDeathMenu(pClient, PacketDeathMenu::ServerSent);
				new PacketDeathMenu(pClient, PacketDeathMenu::Ghost);
			}
			else
				new PacketPlayerPosition(pClient);
		}

		// Remove the characters which I can't see as dead from the screen
		if ( g_Cfg.m_fDeadCannotSeeLiving )
		{
			CWorldSearch AreaChars(GetTopPoint(), UO_MAP_VIEW_SIZE);
			AreaChars.SetSearchSquare(true);
			for (;;)
			{
				CChar *pChar = AreaChars.GetChar();
				if ( !pChar )
					break;
				if ( !CanSeeAsDead(pChar) )
					pClient->addObjectRemove(pChar);						
			}
		}
	}
	return true;
}


bool CChar::OnFreezeCheck()
{
	ADDTOCALLSTACK("CChar::OnFreezeCheck");
	// Check if we are held in place.
	// RETURN: true = held in place.

	if ( IsStatFlag(STATF_Freeze|STATF_Stone) && !IsPriv(PRIV_GM) )
		return true;
	if ( GetKeyNum("NoMoveTill", true) > g_World.GetCurrentTime().GetTimeRaw() )
		return true;

	if ( m_pPlayer )
	{
		if ( m_pPlayer->m_speedMode & 0x04 )	// speed mode '4' prevents movement
			return true;

		if ( IsSetMagicFlags(MAGICF_FREEZEONCAST) && g_Cfg.IsSkillFlag(m_Act_SkillCurrent, SKF_MAGIC) )		// casting magic spells
		{
			CSpellDef *pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_Spell);
			if ( pSpellDef && !pSpellDef->IsSpellType(SPELLFLAG_NOFREEZEONCAST) )
				return true;
		}
	}

	return false;
}

void CChar::Flip()
{
	ADDTOCALLSTACK("CChar::Flip");
	UpdateDir( GetDirTurn( m_dirFace, 1 ));
}

CRegionBase * CChar::CanMoveWalkTo( CPointBase & ptDst, bool fCheckChars, bool fCheckOnly, DIR_TYPE dir, bool fPathFinding )
{
	ADDTOCALLSTACK("CChar::CanMoveWalkTo");
	// For both players and NPC's
	// Walk towards this point as best we can.
	// Affect stamina as if we WILL move !
	// RETURN:
	//  ptDst.m_z = the new z
	//  NULL = failed to walk here.

	int iWeightLoadPercent = GetWeightLoadPercent(GetTotalWeight());
	if ( !fCheckOnly )
	{
		if ( OnFreezeCheck() )
		{
			SysMessageDefault(DEFMSG_MSG_FROZEN);
			return NULL;
		}

		if ( Stat_GetVal(STAT_DEX) <= 0 && !IsStatFlag(STATF_DEAD) )
		{
			SysMessageDefault(DEFMSG_MSG_FATIGUE);
			return NULL;
		}

		if ( iWeightLoadPercent > 200 )
		{
			SysMessageDefault(DEFMSG_MSG_OVERLOAD);
			return NULL;
		}
	}

	CClient *pClient = GetClient();
	if ( pClient && pClient->m_pHouseDesign )
	{
		if ( pClient->m_pHouseDesign->GetDesignArea().IsInside2d(ptDst) )
		{
			ptDst.m_z = GetTopZ();
			return ptDst.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA);
		}
		return NULL;
	}

	// ok to go here ? physical blocking objects ?
	int iStamReq = 0;
	WORD wBlockFlags = 0;
	height_t ClimbHeight = 0;
	CRegionBase *pArea = NULL;

	EXC_TRY("CanMoveWalkTo");

	EXC_SET("Check Valid Move");
	pArea = CheckValidMove(ptDst, &wBlockFlags, dir, &ClimbHeight, fPathFinding);
	if ( !pArea )
	{
		WARNWALK(("CheckValidMove failed\n"));
		return NULL;
	}

	EXC_SET("NPC's will");
	if ( !fCheckOnly && m_pNPC && !NPC_CheckWalkHere(ptDst, pArea, wBlockFlags) )	// does the NPC want to walk here?
		return NULL;

	EXC_SET("Creature bumping");
	if ( fCheckChars && !IsStatFlag(STATF_DEAD|STATF_Sleeping|STATF_Insubstantial) )
	{
		CItem * pPoly = LayerFind(LAYER_SPELL_Polymorph);
		CWorldSearch AreaChars( ptDst );
		for (;;)
		{
			CChar * pChar = AreaChars.GetChar();
			if ( pChar == NULL )
				break;
			if ( pChar == this || abs(pChar->GetTopZ() - ptDst.m_z) > 5 || pChar->IsStatFlag(STATF_Insubstantial) )
				continue;
			if ( m_pNPC && pChar->m_pNPC )	// NPCs can't walk over another NPC
				return NULL;

			iStamReq = 10;
			if ( IsPriv(PRIV_GM) || pChar->IsStatFlag(STATF_DEAD) )
				iStamReq = 0;
			else if ( pPoly && pPoly->m_itSpell.m_spell == SPELL_Wraith_Form && GetTopMap() == 0 )		// chars under Wraith Form effect can always walk through chars in Felucca
				iStamReq = 0;

			TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;
			if ( IsTrigUsed(TRIGGER_PERSONALSPACE) )
			{
				CScriptTriggerArgs Args(iStamReq);
				iRet = pChar->OnTrigger(CTRIG_PersonalSpace, this, &Args);
				iStamReq = static_cast<int>(Args.m_iN1);

				if ( iRet == TRIGRET_RET_TRUE )
					return NULL;
			}

			if ( iStamReq <= 0 )
				continue;
			if ( Stat_GetVal(STAT_DEX) < Stat_GetMax(STAT_DEX) )
				return NULL;

			TCHAR *pszMsg = Str_GetTemp();
			if ( Stat_GetVal(STAT_DEX) < iStamReq )		// check if we have enough stamina to push the char
			{
				sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MSG_CANTPUSH), pChar->GetName());
				SysMessage(pszMsg);
				return NULL;
			}
			else if ( pChar->IsStatFlag(STATF_Hidden) )
			{
				sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_HIDING_STUMBLE), pChar->GetName());
				pChar->Reveal(STATF_Hidden);
			}
			else if ( pChar->IsStatFlag(STATF_Sleeping) )
				sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MSG_STEPON_BODY), pChar->GetName());
			else
				sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MSG_PUSH), pChar->GetName());

			if ( iRet != TRIGRET_RET_FALSE )
				SysMessage(pszMsg);

			break;
		}
	}

	if ( !fCheckOnly )
	{
		EXC_SET("Stamina penalty");
		// Chance to drop more stamina if running or overloaded
		CVarDefCont * pVal = GetKey("OVERRIDE.RUNNINGPENALTY", true);
		if ( IsStatFlag(STATF_Fly|STATF_Hovering) )
			iWeightLoadPercent += pVal ? static_cast<int>(pVal->GetValNum()) : g_Cfg.m_iStamRunningPenalty;

		pVal = GetKey("OVERRIDE.STAMINALOSSATWEIGHT", true);
		int iChanceForStamLoss = Calc_GetSCurve(iWeightLoadPercent - (pVal ? static_cast<int>(pVal->GetValNum()) : g_Cfg.m_iStaminaLossAtWeight), 10);
		if ( iChanceForStamLoss > Calc_GetRandVal(1000) )
			iStamReq += 1;

		if ( iStamReq )
			UpdateStatVal(STAT_DEX, -iStamReq);

		StatFlag_Mod(STATF_InDoors, (wBlockFlags & CAN_I_ROOF) || pArea->IsFlag(REGION_FLAG_UNDERGROUND));
		m_zClimbHeight = (wBlockFlags & CAN_I_CLIMB) ? ClimbHeight : 0;
	}

	EXC_CATCH;
	return pArea;
}

void CChar::CheckRevealOnMove()
{
	ADDTOCALLSTACK("CChar::CheckRevealOnMove");
	// Are we going to reveal ourselves by moving ?

	if ( !IsStatFlag(STATF_Invisible|STATF_Hidden|STATF_Sleeping) )
		return;

	if ( IsTrigUsed(TRIGGER_STEPSTEALTH) )
		OnTrigger(CTRIG_StepStealth, this);

	m_StepStealth -= IsStatFlag(STATF_Fly|STATF_Hovering) ? 2 : 1;
	if ( m_StepStealth <= 0 )
		Reveal();
}

bool CChar::CheckLocation( bool fStanding )
{
	ADDTOCALLSTACK("CChar::CheckLocation");
	// We are at this location. What will happen?
	// This function is called at every second on ALL chars
	// (even walking or not), so avoid heavy codes here.
	// RETURN: true = we teleported.

	CClient *pClient = GetClient();
	if ( pClient && pClient->m_pHouseDesign )
	{
		// stepping on items doesn't trigger anything whilst in design mode
		if ( pClient->m_pHouseDesign->GetDesignArea().IsInside2d(GetTopPoint()) )
			return false;

		pClient->m_pHouseDesign->EndCustomize(true);
	}

	if ( !fStanding )
	{
		SKILL_TYPE iSkillActive	= Skill_GetActive();
		if ( g_Cfg.IsSkillFlag(iSkillActive, SKF_IMMOBILE) )
			Skill_Fail(false);
		else if ( g_Cfg.IsSkillFlag(iSkillActive, SKF_FIGHT) && g_Cfg.IsSkillFlag(iSkillActive, SKF_RANGED) && !IsSetCombatFlags(COMBAT_ARCHERYCANMOVE) && !IsStatFlag(STATF_ArcherCanMove) )
		{
			// Keep timer active holding the swing action until the char stops moving
			m_atFight.m_War_Swing_State = WAR_SWING_EQUIPPING;
			SetTimeout(TICK_PER_SEC);
		}

		// This could get REALLY EXPENSIVE !
		if ( IsTrigUsed(TRIGGER_STEP) )
		{
			if ( m_pArea->OnRegionTrigger( this, RTRIG_STEP ) == TRIGRET_RET_TRUE )
				return false;

			CRegionBase *pRoom = GetTopPoint().GetRegion(REGION_TYPE_ROOM);
			if ( pRoom && pRoom->OnRegionTrigger( this, RTRIG_STEP ) == TRIGRET_RET_TRUE )
				return false;
		}
	}

	bool fStepCancel = false;
	CWorldSearch AreaItems( GetTopPoint() );
	for (;;)
	{
		CItem *pItem = AreaItems.GetItem();
		if ( pItem == NULL )
			break;

		int zdiff = pItem->GetTopZ() - GetTopZ();
		int	height = pItem->Item_GetDef()->GetHeight();
		if ( height < 3 )
			height = 3;

		if ( zdiff > height || zdiff < -3 )
			continue;
		if ( IsTrigUsed(TRIGGER_STEP) || IsTrigUsed(TRIGGER_ITEMSTEP) )
		{
			CScriptTriggerArgs Args( fStanding? 1 : 0 );
			if ( pItem->OnTrigger( ITRIG_STEP, this , &Args ) == TRIGRET_RET_TRUE )
			{
				fStepCancel	= true;
				continue;
			}
		}

		bool bSpellHit = false;
		switch ( pItem->GetType() )
		{
			case IT_WEB:
				if ( fStanding )
					continue;
				if ( Use_Item_Web(pItem) )	// we got stuck in a spider web
					return false;
				continue;
			case IT_FIRE:
				{
					int iSkillLevel = pItem->m_itSpell.m_spelllevel;	// heat level (0-1000)
					iSkillLevel = Calc_GetRandVal2(iSkillLevel/2, iSkillLevel);
					if ( IsStatFlag(STATF_Fly) )
						iSkillLevel /= 2;

					OnTakeDamage( g_Cfg.GetSpellEffect(SPELL_Fire_Field, iSkillLevel), NULL, DAMAGE_FIRE|DAMAGE_GENERAL, 0, 100, 0, 0, 0 );
					Sound(0x15f);	// fire noise
				}
				continue;
			case IT_SPELL:
				// Workaround: only hit 1 spell on each loop. If we hit all spells (eg: multiple field spells)
				// it will allow weird exploits like cast many Fire Fields on the same spot to take more damage,
				// or Paralyze Field + Fire Field to make the target get stuck forever being damaged with no way
				// to get out of the field, since the damage won't allow cast any spell and the Paralyze Field
				// will immediately paralyze again with 0ms delay at each damage tick.
				// On OSI if the player cast multiple fields on the same tile, it will remove the previous field
				// tile that got overlapped. But Sphere doesn't use this method, so this workaround is needed.
				if ( !bSpellHit )
				{
					OnSpellEffect(static_cast<SPELL_TYPE>(RES_GET_INDEX(pItem->m_itSpell.m_spell)), pItem->m_uidLink.CharFind(), static_cast<int>(pItem->m_itSpell.m_spelllevel), pItem);
					bSpellHit = true;
				}
				continue;
			case IT_TRAP:
			case IT_TRAP_ACTIVE:
				OnTakeDamage( pItem->Use_Trap(), NULL, DAMAGE_HIT_BLUNT|DAMAGE_GENERAL );
				continue;
			case IT_SWITCH:
				if ( pItem->m_itSwitch.m_fStep )
					Use_Item(pItem);
				continue;
			case IT_MOONGATE:
			case IT_TELEPAD:
				if ( fStanding && !IsStatFlag(STATF_Freeze|STATF_Stone) )
					continue;
				Use_MoonGate(pItem);
				return true;
			case IT_SHIP_PLANK:
			case IT_ROPE:
				if ( !fStanding && !IsStatFlag(STATF_Hovering) )
				{
					// Check if we can go out of the ship (in the same direction of plank)
					if ( MoveToValidSpot(m_dirFace, g_Cfg.m_iMaxShipPlankTeleport, 1, true) )
					{
						//pItem->SetTimeout(5 * TICK_PER_SEC);	// autoclose the plank behind us
						return true;
					}
				}
				continue;
			default:
				continue;
		}
	}

	if ( fStanding || fStepCancel )
		return false;

	// Check the map teleporters in this CSector (if any)
	const CPointMap & pt = GetTopPoint();
	CSector *pSector = pt.GetSector();
	if ( !pSector )
		return false;

	const CTeleport *pTeleport = pSector->GetTeleport(pt);
	if ( !pTeleport )
		return false;

	if ( m_pNPC )
	{
		if ( !pTeleport->bNpc )
			return false;

		if ( m_pNPC->m_Brain == NPCBRAIN_GUARD )
		{
			// Guards won't gate into unguarded areas.
			const CRegionWorld *pArea = dynamic_cast<CRegionWorld*>(pTeleport->m_ptDst.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA));
			if ( !pArea || !pArea->IsGuarded() )
				return false;
		}
		if ( Noto_IsCriminal() )
		{
			// wont teleport to guarded areas.
			const CRegionWorld *pArea = dynamic_cast<CRegionWorld*>(pTeleport->m_ptDst.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA));
			if ( !pArea || pArea->IsGuarded() )
				return false;
		}
	}
	Spell_Teleport(pTeleport->m_ptDst, true, false, false);
	return true;
}

bool CChar::MoveToRegion( CRegionWorld * pNewArea, bool fAllowReject )
{
	ADDTOCALLSTACK("CChar::MoveToRegion");
	// Moving to a new region. or logging out (not in any region)
	// pNewArea == NULL = we are logging out.
	// RETURN:
	//  false = do not allow in this area.
	if ( m_pArea == pNewArea )
		return true;

	if ( ! g_Serv.IsLoading())
	{
		if ( fAllowReject && IsPriv( PRIV_GM ))
		{
			fAllowReject = false;
		}

		// Leaving region trigger. (may not be allowed to leave ?)
		if ( m_pArea )
		{
			if ( IsTrigUsed(TRIGGER_EXIT) )
			{
				if ( m_pArea->OnRegionTrigger( this, RTRIG_EXIT ) == TRIGRET_RET_TRUE )
				{
					if ( pNewArea && fAllowReject )
						return false;
				}
			}

			if ( IsTrigUsed(TRIGGER_REGIONLEAVE) )
			{
				CScriptTriggerArgs Args(m_pArea);
				if ( OnTrigger(CTRIG_RegionLeave, this, & Args) == TRIGRET_RET_TRUE )
				{
					if ( pNewArea && fAllowReject )
						return false;
				}
			}
		}

		if ( IsClient() && pNewArea )
		{
			if ( pNewArea->IsFlag(REGION_FLAG_ANNOUNCE) && !pNewArea->IsInside2d( GetTopPoint()) )	// new area.
			{
				CVarDefContStr * pVarStr = dynamic_cast <CVarDefContStr *>( pNewArea->m_TagDefs.GetKey("ANNOUNCEMENT"));
				SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_MSG_REGION_ENTER ),
					( pVarStr != NULL ) ? static_cast<LPCTSTR>(pVarStr->GetValStr()) : static_cast<LPCTSTR>(pNewArea->GetName()));
			}

			// Is it guarded / safe / non-pvp?
			else if ( m_pArea && !IsStatFlag(STATF_DEAD) )
			{
				bool redNew = ( pNewArea->m_TagDefs.GetKeyNum("RED", true) != 0 );
				bool redOld = ( m_pArea->m_TagDefs.GetKeyNum("RED", true) != 0 );
				if ( pNewArea->IsGuarded() != m_pArea->IsGuarded() )
				{
					if ( pNewArea->IsGuarded() )	// now under the protection
					{
						CVarDefContStr	*pVarStr = dynamic_cast <CVarDefContStr *>( pNewArea->m_TagDefs.GetKey("GUARDOWNER"));
						SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_MSG_REGION_GUARDS_1 ),
							( pVarStr != NULL ) ? static_cast<LPCTSTR>(pVarStr->GetValStr()) : g_Cfg.GetDefaultMsg( DEFMSG_MSG_REGION_GUARD_ART ) );
					}
					else							// have left the protection
					{
						CVarDefContStr	*pVarStr = dynamic_cast <CVarDefContStr *>( m_pArea->m_TagDefs.GetKey("GUARDOWNER"));
						SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_MSG_REGION_GUARDS_2 ),
							( pVarStr != NULL ) ? static_cast<LPCTSTR>(pVarStr->GetValStr()) : g_Cfg.GetDefaultMsg( DEFMSG_MSG_REGION_GUARD_ART ) );
					}
				}
				if ( redNew != redOld )
					SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_REGION_REDDEF), g_Cfg.GetDefaultMsg(redNew ? DEFMSG_MSG_REGION_REDENTER : DEFMSG_MSG_REGION_REDLEFT));
				/*else if ( redNew && ( redNew == redOld ))
				{
					SysMessage("You are still in the red region.");
				}*/
				if ( pNewArea->IsFlag(REGION_FLAG_NO_PVP) != m_pArea->IsFlag(REGION_FLAG_NO_PVP))
					SysMessageDefault(( pNewArea->IsFlag(REGION_FLAG_NO_PVP)) ? DEFMSG_MSG_REGION_PVPSAFE : DEFMSG_MSG_REGION_PVPNOT );
				if ( pNewArea->IsFlag(REGION_FLAG_SAFE) != m_pArea->IsFlag(REGION_FLAG_SAFE) )
					SysMessageDefault((pNewArea->IsFlag(REGION_FLAG_SAFE)) ? DEFMSG_MSG_REGION_SAFETYGET : DEFMSG_MSG_REGION_SAFETYLOSE);
			}
		}

		// Entering region trigger.
		if ( pNewArea )
		{
			if ( IsTrigUsed(TRIGGER_ENTER) )
			{
				if ( pNewArea->OnRegionTrigger( this, RTRIG_ENTER ) == TRIGRET_RET_TRUE )
				{
					if ( m_pArea && fAllowReject )
						return false;
				}
			}
			if ( IsTrigUsed(TRIGGER_REGIONENTER) )
			{
				CScriptTriggerArgs Args(pNewArea);
				if ( OnTrigger(CTRIG_RegionEnter, this, & Args) == TRIGRET_RET_TRUE )
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

bool CChar::MoveToRoom( CRegionBase * pNewRoom, bool fAllowReject)
{
	ADDTOCALLSTACK("CChar::MoveToRoom");
	// Moving to a new room.
	// RETURN:
	// false = do not allow in this room.

	if ( m_pRoom == pNewRoom )
		return true;

	if ( ! g_Serv.IsLoading())
	{
		if ( fAllowReject && IsPriv( PRIV_GM ))
		{
			fAllowReject = false;
		}

		// Leaving room trigger. (may not be allowed to leave ?)
		if ( m_pRoom )
		{
			if ( IsTrigUsed(TRIGGER_EXIT) )
			{
				if ( m_pRoom->OnRegionTrigger( this, RTRIG_EXIT ) == TRIGRET_RET_TRUE )
				{
					if (fAllowReject )
						return false;
				}
			}

			if ( IsTrigUsed(TRIGGER_REGIONLEAVE) )
			{
				CScriptTriggerArgs Args(m_pRoom);
				if ( OnTrigger(CTRIG_RegionLeave, this, & Args) == TRIGRET_RET_TRUE )
				{
					if (fAllowReject )
						return false;
				}
			}
		}

		// Entering room trigger
		if ( pNewRoom )
		{
			if ( IsTrigUsed(TRIGGER_ENTER) )
			{
				if ( pNewRoom->OnRegionTrigger( this, RTRIG_ENTER ) == TRIGRET_RET_TRUE )
				{
					if (fAllowReject )
						return false;
				}
			}
			if ( IsTrigUsed(TRIGGER_REGIONENTER) )
			{
				CScriptTriggerArgs Args(pNewRoom);
				if ( OnTrigger(CTRIG_RegionEnter, this, & Args) == TRIGRET_RET_TRUE )
				{
					if (fAllowReject )
						return false;
				}
			}
		}
	}

	m_pRoom = pNewRoom;
	return true;
}

bool CChar::MoveToChar(CPointMap pt, bool bForceFix)
{
	ADDTOCALLSTACK("CChar::MoveToChar");
	// Same as MoveTo
	// This could be us just taking a step or being teleported.
	// Low level: DOES NOT UPDATE DISPLAYS or container flags. (may be offline)
	// This does not check for gravity.

	if ( !pt.IsValidPoint() )
		return false;

	CClient *pClient = GetClient();
	if ( m_pPlayer && !pClient )	// moving a logged out client !
	{
		CSector *pSector = pt.GetSector();
		if ( !pSector )
			return false;

		// We cannot put this char in non-disconnect state.
		SetDisconnected();
		pSector->m_Chars_Disconnect.InsertHead(this);
		SetUnkPoint(pt);
		return true;
	}

	// Did we step into a new region ?
	CRegionWorld * pAreaNew = dynamic_cast<CRegionWorld *>(pt.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA));
	if ( !MoveToRegion(pAreaNew, true) )
		return false;

	CRegionBase * pRoomNew = pt.GetRegion(REGION_TYPE_ROOM);
	if ( !MoveToRoom(pRoomNew, true) )
		return false;

	CPointMap ptOld = GetUnkPoint();
	bool fSectorChange = pt.GetSector()->MoveCharToSector(this);
	SetTopPoint(pt);

	if ( !m_fClimbUpdated || bForceFix )
		FixClimbHeight();

	if ( fSectorChange && !g_Serv.IsLoading() )
	{
		if ( IsTrigUsed(TRIGGER_ENVIRONCHANGE) )
		{
			CScriptTriggerArgs	Args(ptOld.m_x, ptOld.m_y, ptOld.m_z << 16 | ptOld.m_map);
			OnTrigger(CTRIG_EnvironChange, this, &Args);
		}
	}

	return true;
}

bool CChar::MoveToValidSpot(DIR_TYPE dir, int iDist, int iDistStart, bool bFromShip)
{
	ADDTOCALLSTACK("CChar::MoveToValidSpot");
	// Move from here to a valid spot.
	// ASSUME "here" is not a valid spot. (even if it really is)

	CPointMap pt = GetTopPoint();
	pt.MoveN( dir, iDistStart );
	pt.m_z += PLAYER_HEIGHT;
	signed char startZ = pt.m_z;

	WORD wCan = static_cast<WORD>(GetMoveBlockFlags(true));	// CAN_C_SWIM
	for ( int i=0; i<iDist; ++i )
	{
		if ( pt.IsValidPoint() )
		{
			// Don't allow boarding of other ships (they may be locked)
			CRegionBase * pRegionBase = pt.GetRegion( REGION_TYPE_SHIP);
			if ( pRegionBase)
			{
				pt.Move( dir );
				continue;
			}

			DWORD wBlockFlags = wCan;
			// Reset Z back to start Z + PLAYER_HEIGHT so we don't climb buildings
			pt.m_z = startZ;
			// Set new Z so we don't end up floating or underground
			pt.m_z = g_World.GetHeightPoint( pt, wBlockFlags, true );

			// don't allow characters to pass through walls or other blocked
			// paths when they're disembarking from a ship
			if ( bFromShip && (wBlockFlags & CAN_I_BLOCK) && !(wCan & CAN_C_PASSWALLS) && (pt.m_z > startZ) )
			{
				break;
			}

			if ( ! ( wBlockFlags &~ wCan ))
			{
				// we can go here. (maybe)
				if ( Spell_Teleport(pt, true, !bFromShip, false) )
					return true;
			}
		}
		pt.Move( dir );
	}
	return false;
}

bool CChar::SetPrivLevel(CTextConsole * pSrc, LPCTSTR pszFlags)
{
	ADDTOCALLSTACK("CChar::SetPrivLevel");
	// "PRIVSET"
	// Set this char to be a GM etc. (or take this away)
	// NOTE: They can be off-line at the time.

	if (( pSrc->GetPrivLevel() < PLEVEL_Admin ) ||
		( pSrc->GetPrivLevel() < GetPrivLevel() ) ||
		!pszFlags[0] || !m_pPlayer )
		return false;

	CAccount * pAccount = m_pPlayer->GetAccount();
	PLEVEL_TYPE PrivLevel = CAccount::GetPrivLevelText(pszFlags);

	// Remove Previous GM Robe
	ContentConsume(RESOURCE_ID(RES_ITEMDEF,ITEMID_GM_ROBE), INT_MAX);

	if ( PrivLevel >= PLEVEL_Counsel )
	{
		pAccount->SetPrivFlags(PRIV_GM_PAGE|( PrivLevel >= PLEVEL_GM ? PRIV_GM : 0));

		UnEquipAllItems();

		CItem * pItem = CItem::CreateScript( ITEMID_GM_ROBE, this );
		if ( pItem )
		{
			pItem->SetAttr(ATTR_MOVE_NEVER|ATTR_NEWBIE|ATTR_MAGIC);
			pItem->m_itArmor.m_spelllevel = 1000;
			pItem->SetHue( static_cast<HUE_TYPE>(( PrivLevel >= PLEVEL_GM ) ? HUE_RED : HUE_BLUE_NAVY) /*, false, pSrc */ ); //call @Dye on equiped gm robes after plevel change?
			ItemEquip(pItem);
		}
	}
	else
	{
		// Revoke GM status.
		pAccount->ClearPrivFlags( PRIV_GM_PAGE|PRIV_GM );
	}

	if ( pAccount->GetPrivLevel() < PLEVEL_Admin && PrivLevel < PLEVEL_Admin )	// can't demote admin this way.
	{
		pAccount->SetPrivLevel( PrivLevel );
	}

	Update();
	return true;
}

TRIGRET_TYPE CChar::OnTrigger( LPCTSTR pszTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs )
{
	ADDTOCALLSTACK("CChar::OnTrigger");

	if (IsTriggerActive(pszTrigName)) //This should protect any char trigger from infinite loop
		return TRIGRET_RET_DEFAULT;

	// Attach some trigger to the cchar. (PC or NPC)
	// RETURN: true = block further action.
	CCharBase* pCharDef = Char_GetDef();
	if ( !pCharDef )
		return TRIGRET_RET_DEFAULT;

	CTRIG_TYPE iAction;
	if ( ISINTRESOURCE(pszTrigName))
	{
		iAction = (CTRIG_TYPE) GETINTRESOURCE(pszTrigName);
		pszTrigName = sm_szTrigName[iAction];
	}
	else
	{
		iAction = (CTRIG_TYPE) FindTableSorted( pszTrigName, sm_szTrigName, COUNTOF(sm_szTrigName)-1 );
	}
	SetTriggerActive(pszTrigName);

	TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;

	TemporaryString sCharTrigName;
	sprintf(sCharTrigName, "@char%s", pszTrigName+1);

	int iCharAction = (CTRIG_TYPE) FindTableSorted( sCharTrigName, sm_szTrigName, COUNTOF(sm_szTrigName)-1 );

	EXC_TRY("Trigger");

	// 1) Triggers installed on characters, sensitive to actions on all chars
	if (( IsTrigUsed(sCharTrigName) ) && ( iCharAction > XTRIG_UNKNOWN ))
	{
		CChar * pChar = pSrc->GetChar();
		if ( pChar != NULL && this != pChar )
		{
			EXC_SET("chardef");
			CGrayUID uidOldAct = pChar->m_Act_Targ;
			pChar->m_Act_Targ = GetUID();
			iRet = pChar->OnTrigger(sCharTrigName, pSrc, pArgs );
			pChar->m_Act_Targ = uidOldAct;
			if ( iRet == TRIGRET_RET_TRUE )
				goto stopandret;//return iRet;	// Block further action.
		}
	}

	//	2) EVENTS
	//
	// Go thru the event blocks for the NPC/PC to do events.
	//
	if ( IsTrigUsed(pszTrigName) )
	{
		EXC_SET("events");
		size_t origEvents = m_OEvents.GetCount();
		size_t curEvents = origEvents;
		for ( size_t i = 0; i < curEvents; ++i ) // EVENTS (could be modifyed ingame!)
		{
			CResourceLink * pLink = m_OEvents[i];
			if ( !pLink || !pLink->HasTrigger(iAction) )
				continue;
			CResourceLock s;
			if ( !pLink->ResourceLock(s) )
				continue;

			iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
			if ( iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT )
				goto stopandret;//return iRet;

			curEvents = m_OEvents.GetCount();
			if ( curEvents < origEvents ) // the event has been deleted, modify the counter for other trigs to work
			{
				--i;
				origEvents = curEvents;
			}
		}

		if ( m_pNPC != NULL )
		{
			// 3) TEVENTS
			EXC_SET("NPC triggers"); // TEVENTS (constant events of NPCs)
			for ( size_t i = 0; i < pCharDef->m_TEvents.GetCount(); ++i )
			{
				CResourceLink * pLink = pCharDef->m_TEvents[i];
				if ( !pLink || !pLink->HasTrigger(iAction) )
					continue;
				CResourceLock s;
				if ( !pLink->ResourceLock(s) )
					continue;
				iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
				if ( iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT )
					goto stopandret;//return iRet;
			}
		}

		// 4) CHARDEF triggers
		if ( m_pPlayer == NULL ) //	CHARDEF triggers (based on body type)
		{
			EXC_SET("chardef triggers");
			if ( pCharDef->HasTrigger(iAction) )
			{
				CResourceLock s;
				if ( pCharDef->ResourceLock(s) )
				{
					iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
					if (( iRet != TRIGRET_RET_FALSE ) && ( iRet != TRIGRET_RET_DEFAULT ))
						goto stopandret;//return iRet;
				}
			}
		}


		// 5) EVENTSPET triggers for npcs
		if (m_pNPC != NULL)
		{
			EXC_SET("NPC triggers - EVENTSPET"); // EVENTSPET (constant events of NPCs set from sphere.ini)
			for (size_t i = 0; i < g_Cfg.m_pEventsPetLink.GetCount(); ++i)
			{
				CResourceLink * pLink = g_Cfg.m_pEventsPetLink[i];
				if (!pLink || !pLink->HasTrigger(iAction))
					continue;
				CResourceLock s;
				if (!pLink->ResourceLock(s))
					continue;
				iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
				if (iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT)
					goto stopandret;//return iRet;
			}
		}
		// 5) EVENTSPLAYER triggers for players
		if ( m_pPlayer != NULL )
		{
			//	EVENTSPLAYER triggers (constant events of players set from sphere.ini)
			EXC_SET("chardef triggers - EVENTSPLAYER");
			for ( size_t i = 0; i < g_Cfg.m_pEventsPlayerLink.GetCount(); ++i )
			{
				CResourceLink	*pLink = g_Cfg.m_pEventsPlayerLink[i];
				if ( !pLink || !pLink->HasTrigger(iAction) )
					continue;
				CResourceLock s;
				if ( !pLink->ResourceLock(s) )
					continue;
				iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
				if ( iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT )
					goto stopandret;//return iRet;
			}
		}
	}
stopandret:
	{
		SetTriggerActive((LPCTSTR)0);
		return iRet;
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("trigger '%s' action '%d' [0%lx]\n", pszTrigName, iAction, (DWORD)GetUID());
	EXC_DEBUG_END;
	return iRet;
}

void CChar::OnTickStatusUpdate()
{
	ADDTOCALLSTACK("CChar::OnTickStatusUpdate");

	if ( IsClient() )
		GetClient()->UpdateStats();

	// process m_fStatusUpdate flags
	INT64 iTimeDiff = - g_World.GetTimeDiff( m_timeLastHitsUpdate );
	if ( g_Cfg.m_iHitsUpdateRate && ( iTimeDiff >= g_Cfg.m_iHitsUpdateRate ) )
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
		UpdateMode();
		m_fStatusUpdate &= ~SU_UPDATE_MODE;
	}

	CObjBase::OnTickStatusUpdate();
}

void CChar::OnTickFood(int iVal, int HitsHungerLoss)
{
	ADDTOCALLSTACK("CChar::OnTickFood");
	if ( IsStatFlag(STATF_Conjured) || !Stat_GetMax(STAT_FOOD) )
		return;	// No need for food.

	// This may be money instead of food
	if ( IsStatFlag(STATF_Pet) && !NPC_CheckHirelingStatus() )
		return;

	long lFood = Stat_GetVal(STAT_FOOD) - iVal;
	if (lFood < 0)
		lFood = 0;
	Stat_SetVal(STAT_FOOD, lFood);

	int  nFoodLevel = Food_GetLevelPercent();
	if ( nFoodLevel < 40) 	// start looking for food at 40%
 	{
		// Tell everyone we look hungry.
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_HUNGER), Food_GetLevelMessage(false, false));
		CHAR_OnTickFood( nFoodLevel , HitsHungerLoss );
	}
}

bool CChar::CHAR_OnTickFood( int nFoodLevel , int HitsHungerLoss )
{
	ADDTOCALLSTACK("CChar::CHAR_OnTickFood");
	// Check Food usage.
	// Are we hungry enough to take some new action ?
	// RETURN: true = we have taken an action.

	if ( HitsHungerLoss <= 0 )
		return false;
	if ( !m_pArea || m_pArea->IsFlag(REGION_FLAG_SAFE))
		return false;
	if ( IsStatFlag(STATF_INVUL|STATF_DEAD|STATF_Sleeping|STATF_Stone|STATF_Spawned))
		return false;

	bool bPet = IsStatFlag(STATF_Pet);
	char *pszMsg = Str_GetTemp();
	sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_LOOKS), Food_GetLevelMessage(bPet, false));
	CItem *pMountItem = Horse_GetMountItem();
	if ( pMountItem )
		pMountItem->Emote(pszMsg);
	else
		Emote(pszMsg);

	if ( nFoodLevel <= 0 )
	{
		OnTakeDamage(HitsHungerLoss, this, DAMAGE_FIXED);
		SoundChar(CRESND_RAND2);
		if ( bPet )
			NPC_PetDesert();
		return true;
	}

	return false;
}

bool CChar::OnTick()
{
	ADDTOCALLSTACK("CChar::OnTick");
	// Assume this is only called 1 time per sec.
	// Get a timer tick when our timer expires.
	// RETURN: false = delete this.

	TIME_PROFILE_INIT;
	if ( IsSetSpecific )
		TIME_PROFILE_START;

	EXC_TRY("Tick");
	INT64 iTimeDiff = -g_World.GetTimeDiff(m_timeLastRegen);
	if ( !iTimeDiff )
		return true;

	if ( iTimeDiff >= TICK_PER_SEC )		// don't bother with < 1 sec timers on the checks below
	{
		// Decay equipped items (memories/spells)
		CItem *pItem = GetContentHead();
		for ( size_t iCount = 0; pItem != NULL; pItem = GetAt(++iCount) )
		{
			EXC_TRYSUB("Ticking items");
			/*if ( pItem->IsType(IT_EQ_MEMORY_OBJ) && !pItem->m_uidLink.ObjFind() )		// always check the validity of the memory objects
			{
				pItem->Delete();
				continue;
			}
			pItem->OnTickStatusUpdate();*/
			if ( !pItem->IsTimerSet() || !pItem->IsTimerExpired() )
				continue;
			if ( !OnTickEquip(pItem) )
				pItem->Delete();
			EXC_CATCHSUB("Char");
		}
	}

	if ( IsDisconnected() )
		return true;

	CClient *pClient = GetClient();
	if ( pClient )
	{
		// Players have a silly "always run" flag that gets stuck on.
		if ( -g_World.GetTimeDiff(pClient->m_timeLastEventWalk) > TICK_PER_SEC )
			StatFlag_Clear(STATF_Fly);

		// Check targeting timeout, if set
		if ( pClient->m_Targ_Timeout.IsTimeValid() && g_World.GetTimeDiff(pClient->m_Targ_Timeout) <= 0 )
			pClient->addTargetCancel();
	}

	if ( iTimeDiff >= TICK_PER_SEC )		// don't bother with < 1 sec timers on the checks below
	{
		m_timeLastRegen = CServTime::GetCurrentTime();

		EXC_SET("last attackers");
		Attacker_CheckTimeout();

		EXC_SET("NOTO timeout");
		NotoSave_CheckTimeout();

		if ( !IsStatFlag(STATF_DEAD) )
		{
			EXC_SET("check location");
			CheckLocation(true);	// check location periodically for standing in fire fields, traps, etc

			EXC_SET("regen stats");
			Stats_Regen(iTimeDiff);
		}
	}

	EXC_SET("update stats");
	OnTickStatusUpdate();

	if ( !IsStatFlag(STATF_DEAD) && Stat_GetVal(STAT_STR) <= 0 )
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

		if ( IsStatFlag(STATF_War) )
		{
			EXC_SET("combat hit try");
			if ( m_atFight.m_War_Swing_State == WAR_SWING_READY )	// hit my current target (if I'm ready)
				Fight_HitTry();
		}

		if ( m_pNPC )	// do some AI action
		{
			ProfileTask aiTask(PROFILE_NPC_AI);
			EXC_SET("NPC action");
			NPC_OnTickAction();

			if ( !IsStatFlag(STATF_DEAD) )
			{
				int iAiFlags = NPC_GetAiFlags();
				if ( (iAiFlags & NPC_AI_FOOD) && !(iAiFlags & NPC_AI_INTFOOD) )
					NPC_Food();
				if ( iAiFlags & NPC_AI_EXTRA )
					NPC_ExtraAI();
			}
		}
	}

	EXC_CATCH;
	if ( IsSetSpecific )
	{
		TIME_PROFILE_END;
		DEBUG_ERR(("CChar::OnTick(%lx) took %lld.%lld to run\n", (DWORD)GetUID(), (INT64)TIME_PROFILE_GET_HI, (INT64)TIME_PROFILE_GET_LO));
	}
	return true;
}
