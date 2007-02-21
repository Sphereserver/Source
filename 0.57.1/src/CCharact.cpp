//
// CCharAct.cpp
//

#include "graysvr.h"
#include "CClient.h"
#include "quest.h"
#include "network/network.h"
#include "network/send.h"

bool CChar::TeleportToObj( int iType, TCHAR * pszArgs )
{
	// "GONAME", "GOTYPE", "GOCHAR"
	// 0 = object name
	// 1 = char
	// 2 = item type

	DWORD dwUID = m_Act_Targ.GetObjUID() &~ UID_F_ITEM;
	DWORD dwTotal = g_World.GetUIDCount();
	DWORD dwCount = dwTotal-1;

	int iArg;
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
				CChar * pChar = dynamic_cast <CChar*>(pObj);
				if ( iArg-- > 0 )
					continue;
			}
			break;
		case 2:	// item type
			{
				if ( ! pObj->IsItem())
					continue;
				CItem * pItem = dynamic_cast <CItem*>(pObj);
				if ( ! pItem->IsType( (IT_TYPE) iArg ))
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
		if ( pObjTop->IsChar())
		{
			if ( !CanDisturb( dynamic_cast<CChar*>(pObjTop)) )
				continue;
		}

		if ( pObjTop == this )
			continue;

		m_Act_Targ = pObj->GetUID();
		Spell_Teleport( pObjTop->GetTopPoint(), true, false );
		return true;
	}
	return false;
}

bool CChar::TeleportToCli( int iType, int iArgs )
{
	ClientIterator it;
	CClient *client;
	while ( client = it.next() )
	{
		if ( ! iType )
		{
			if ( client->socketId() != iArgs )
				continue;
		}
		CChar *pChar = client->GetChar();
		if ( !pChar || !CanDisturb(pChar) )
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

void CChar::Jail( CTextConsole * pSrc, bool fSet, int iCell )
{
	if ( fSet )	// set the jailed flag.
	{
		if ( m_pPlayer )	// allow setting of this to offline chars.
		{
			m_pPlayer->GetAccount()->SetPrivFlags( PRIV_JAILED );
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
		SysMessageDefault( DEFMSG_JAILED );
	}
	else	// forgive.
	{
		if ( IsClient())
		{
			if ( ! m_pClient->IsPriv( PRIV_JAILED ))
				return;
			m_pClient->ClearPrivFlags( PRIV_JAILED );
		}
		if ( m_pPlayer )
		{
			m_pPlayer->GetAccount()->ClearPrivFlags( PRIV_JAILED );
		}
		SysMessageDefault( DEFMSG_FORGIVEN );
	}
}

void CChar::AddGoldToPack( int iAmount, CItemContainer * pPack )
{
	// A vendor is giving me gold. put it in my pack or other place.

	if ( pPack == NULL )
		pPack = GetPackSafe();

	while ( iAmount > 0 )
	{
		CItem * pGold = CItem::CreateScript( ITEMID_GOLD_C1, this );

		int iGoldStack = min( iAmount, USHRT_MAX );
		pGold->SetAmount( iGoldStack );

		Sound( pGold->GetDropSound( pPack ));
		pPack->ContentAdd( pGold );
		iAmount -= iGoldStack;
	}
}

void CChar::LayerAdd( CItem * pItem, LAYER_TYPE layer )
{
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
		if ( IsDisconnected() )
			return;
		// NOTE: CanEquipLayer may bounce an item . If it stacks with this we are in trouble !
	}

	// This takes care of any conflicting items in the slot !
	layer = CanEquipLayer(pItem, layer, NULL, false);
	if ( layer == LAYER_NONE )
	{
		// we should not allow non-layered stuff to be put here ?
		// Put in pack instead ?
		ItemBounce( pItem );
		return;
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
				Fight_ResetWeaponSwingTimer();
			}
			else if ( pItem->IsTypeArmor())
			{
				// Shield of some sort.
				m_defense = CalcArmorDefense();
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
			m_defense = CalcArmorDefense();
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
	}

	if ( layer != LAYER_DRAGGING )
	{
		switch ( pItem->GetType())
		{
			case IT_EQ_SCRIPT:	// pure script.
				break;
			case IT_EQ_MEMORY_OBJ:
				Memory_UpdateFlags( dynamic_cast <CItemMemory *>(pItem) );
				break;
			case IT_EQ_HORSE:
				StatFlag_Set(STATF_OnHorse);
				break;
			case IT_COMM_CRYSTAL:
				StatFlag_Set(STATF_COMM_CRYSTAL);
				break;
		}
	}

	pItem->Update();
}

void CChar::OnToggleEquip(CItem *pItem, bool amEquipping)
{
	//	not identifyed items do not suit me
	if ( !pItem || !pItem->IsAttr(ATTR_IDENTIFIED) )
		return;

	CItemBase *pBase = pItem->Item_GetDef();

	//	we do not have any tags, so do not need to proceed
	if ( !pBase->m_TagDefs.GetCount() && !pItem->m_TagDefs.GetCount() )
		return;

	int i;
	int mult = ( amEquipping ? 1 : -1 );

	// skill modifications
	for ( i = 0 ; i < MAX_SKILL; i++ )
	{
		VariableList::Variable *pProperty;

		pProperty = pItem->m_TagDefs.GetKey(g_Cfg.GetSkillKey((SKILL_TYPE)i));
		if ( pProperty )
			Skill_SetBase((SKILL_TYPE)i, Skill_GetBase((SKILL_TYPE)i) + ( mult * pProperty->GetValNum() ));

		pProperty = pBase->m_TagDefs.GetKey(g_Cfg.GetSkillKey((SKILL_TYPE)i));
		if ( pProperty )
			Skill_SetBase((SKILL_TYPE)i, Skill_GetBase((SKILL_TYPE)i) + ( mult * pProperty->GetValNum() ));
	}

	// stats modifications
	for ( i = 0 ; i <= 2; i++ )
	{
		VariableList::Variable *pProperty;

		pProperty = pItem->m_TagDefs.GetKey(g_Stat_Name[(STAT_TYPE)i]);
		if ( pProperty )
			Stat_AddMod((STAT_TYPE)i, mult * pProperty->GetValNum());

		pProperty = pBase->m_TagDefs.GetKey(g_Stat_Name[(STAT_TYPE)i]);
		if ( pProperty )
			Stat_AddMod((STAT_TYPE)i, mult * pProperty->GetValNum());
	}
}

void CChar::OnRemoveOb( CGObListRec* pObRec )	// Override this = called when removed from list.
{
	// Unequip the item.
	// This may be a delete etc. It can not FAIL !
	CItem * pItem = static_cast <CItem*>(pObRec);
	if ( !pItem )
		return;

	LAYER_TYPE layer = pItem->GetEquipLayer();
	if (( layer != LAYER_DRAGGING ) && !g_Serv.IsLoading() )
	{
		pItem->OnTrigger(ITRIG_UNEQUIP, this);
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
				Fight_ResetWeaponSwingTimer();
			}
			else if ( pItem->IsTypeArmor())
			{
				// Shield
				m_defense = CalcArmorDefense();
				StatFlag_Clear( STATF_HasShield );
				UpdateStatsFlag();
			}
			if (( this->m_Act_SkillCurrent == SKILL_MINING ) || ( this->m_Act_SkillCurrent == SKILL_FISHING ) || ( this->m_Act_SkillCurrent == SKILL_LUMBERJACKING ))
			{
				Skill_Start( SKILL_NONE );
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
			m_defense = CalcArmorDefense();
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
				// Clear the associated flags.
				Memory_UpdateClearTypes( dynamic_cast<CItemMemory*>(pItem), 0xFFFF );
				break;
		}
		OnToggleEquip(pItem, false);

		// If items are magical then remove effect here.
		Spell_Effect_Remove(pItem);
	}
}

void CChar::DropAll( CItemContainer * pCorpse, WORD wAttr )
{
	// shrunk or died.
	if ( IsStatFlag( STATF_Conjured ))
		return;	// drop nothing.

	CItemContainer * pPack = GetPack();
	if ( pPack != NULL )
	{
		if ( pCorpse == NULL )
		{
			pPack->ContentsDump( GetTopPoint(), wAttr );
		}
		else
		{
			pPack->ContentsTransfer( pCorpse, true );
		}

		//	close inventory gump of course
		pPack->RemoveFromView();
		pPack->Update();
	}

	// transfer equipped items to corpse or your pack (if newbie).
	UnEquipAllItems( pCorpse );
}

void CChar::UnEquipAllItems( CItemContainer * pDest )
{
	// We morphed, died or became a GM.
	// Pets can be told to "Drop All"
	// drop item that is up in the air as well.

	if ( ! GetCount())
		return;
	CItemContainer * pPack = NULL;

	CItem* pItemNext;
	CItem* pItem=GetContentHead();
	for ( ; pItem!=NULL; pItem=pItemNext )
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
				pDupe->SetContainedLayer( layer );
			}
			continue;
		case LAYER_DRAGGING:
			layer = LAYER_NONE;
			break;
		default:
			// can't transfer this to corpse.
			if ( ! CItemBase::IsVisibleLayer( layer ))
				continue;
			break;
		}
		if (( pDest != NULL ) && !pItem->IsAttr(ATTR_NEWBIE|ATTR_MOVE_NEVER))
		{	// Move item to dest. (corpse ussually)
			pDest->ContentAdd( pItem );
			if ( pDest->IsType(IT_CORPSE))
			{
				// Equip layer only matters on a corpse.
				pItem->SetContainedLayer( layer );
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

void CChar::UpdateDrag( CItem * pItem, CObjBase * pCont, CPointMap * ppt )
{
	// Show the world that I am picking up or putting down this object.
	// NOTE: This makes people disapear.
	CCommand cmd;
	cmd.DragAnim.m_Cmd = XCMD_DragAnim;
	cmd.DragAnim.m_id = pItem->GetDispID();
	cmd.DragAnim.m_unk3 = 0;
	cmd.DragAnim.m_unk5 = 0;
	cmd.DragAnim.m_unk7 = 0;

	CPointMap ptThis = GetTopPoint();

	if ( pCont != NULL )
	{
		// I'm putting an object in a cont..
		CObjBaseTemplate * pObjTop = pCont->GetTopLevelObj();
		if ( pObjTop == this )
			return;	// move stuff in my own pack.

		CPointMap ptTop = pObjTop->GetTopPoint();

		cmd.DragAnim.m_srcUID = GetUID();
		cmd.DragAnim.m_src_x = ptThis.m_x;
		cmd.DragAnim.m_src_y = ptThis.m_y;
		cmd.DragAnim.m_src_x = ptThis.m_z;
		cmd.DragAnim.m_dstUID = pObjTop->GetUID();
		cmd.DragAnim.m_dst_x = ptTop.m_x;
		cmd.DragAnim.m_dst_y = ptTop.m_y;
		cmd.DragAnim.m_dst_z = ptTop.m_z;
	}
	else if ( ppt != NULL )
	{
		// putting on ground.
		cmd.DragAnim.m_srcUID = GetUID();
		cmd.DragAnim.m_src_x = ptThis.m_x;
		cmd.DragAnim.m_src_y = ptThis.m_y;
		cmd.DragAnim.m_src_x = ptThis.m_z;
		cmd.DragAnim.m_dstUID = 0;
		cmd.DragAnim.m_dst_x = ppt->m_x;
		cmd.DragAnim.m_dst_y = ppt->m_y;
		cmd.DragAnim.m_dst_z = ppt->m_z;
	}
	else
	{
		// I'm getting an object from where ever it is.

		// ??? Note: this doesn't work for ground objects !
		CObjBaseTemplate * pObjTop = pItem->GetTopLevelObj();
		if ( pObjTop == this )
			return;	// move stuff in my own pack.

		CPointMap ptTop = pObjTop->GetTopPoint();

		cmd.DragAnim.m_srcUID = (pObjTop==pItem) ? 0 : pObjTop->uid();
		cmd.DragAnim.m_src_x = ptTop.m_x;
		cmd.DragAnim.m_src_y = ptTop.m_y;
		cmd.DragAnim.m_src_z = ptTop.m_z;
		cmd.DragAnim.m_dstUID = 0; // GetUID();
		cmd.DragAnim.m_dst_x = ptThis.m_x;
		cmd.DragAnim.m_dst_y = ptThis.m_y;
		cmd.DragAnim.m_dst_x = ptThis.m_z;
	}

	UpdateCanSee( &cmd, sizeof(cmd.DragAnim), m_pClient );
}


void	CChar::UpdateStatsFlag() const
{
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
	if ( g_Serv.IsLoading() )
		return;

	m_fHitsUpdate = true;

	if ( IsClient() )
		GetClient()->addUpdateHitsFlag();
}

void CChar::UpdateManaFlag() const
{
	if ( g_Serv.IsLoading() )
		return;

	if ( IsClient() )
		GetClient()->addUpdateManaFlag();
}

void CChar::UpdateStamFlag() const
{
	if ( g_Serv.IsLoading() )
		return;

	if ( IsClient() )
		GetClient()->addUpdateStamFlag();
}

void CChar::UpdateHitsForOthers() const
{
	int iMaxHits = max(Stat_GetMax(STAT_STR),1);

	CCommand cmd;
	cmd.StatChng.m_Cmd = XCMD_StatChngStr;
	cmd.StatChng.m_UID = GetUID();
	cmd.StatChng.m_max = 50;
	cmd.StatChng.m_val = ( (Stat_GetVal(STAT_STR) * 50) / iMaxHits);
	UpdateCanSee( &cmd, sizeof(cmd.StatChng), m_pClient );
}

void CChar::UpdateStatVal( STAT_TYPE type, int iChange, int iLimit )
{
	int iVal = Stat_GetVal( type );

	if ( iChange )
	{
		if ( ! iLimit )
		{
			iLimit = Stat_GetMax( type );
		}
		if ( iChange < 0 )
		{
			iVal += iChange;
		}
		else if ( iVal > iLimit )
		{
			iVal -= iChange;
			if ( iVal < iLimit ) iVal = iLimit;
		}
		else
		{
			iVal += iChange;
			if ( iVal > iLimit ) iVal = iLimit;
		}
		if ( iVal < 0 ) iVal = 0;
		Stat_SetVal( type, iVal );
	}

	iLimit = Stat_GetMax(type);
	if ( iLimit < 0 )
		iLimit = 0;

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
	}
}

int CChar::translateAnimation(int action)
{
	if ( action > 0x80 ) return -1;

	CCharBase	*charDef = Char_GetDef();
	CItem		*weapon = m_uidWeapon.ItemFind();

	//	combat-specific animations depeding on the weapon
	if ( weapon && ( action == 0x09 ))
	{
		IT_TYPE weapType = weapon->GetType();
		LAYER_TYPE layer = weapon->Item_GetDef()->GetEquipLayer();

		if ( weapType == IT_WEAPON_BOW )		action = 0x12;
		else if ( weapType == IT_WEAPON_XBOW )	action = 0x13;
		else if ( Calc_GetRandVal(2) )
		{
			if ( layer == LAYER_HAND2 )	action = 0x0c + Calc_GetRandVal(3);
			else						action = 0x09 + Calc_GetRandVal(3);
		}
		else switch ( weapType )
		{
			case IT_WEAPON_SWORD:
			case IT_WEAPON_AXE:
				action = ( layer == LAYER_HAND2 ) ? 0x0d : 0x09;
				break;
			case IT_WEAPON_FENCE:
				action = ( layer == LAYER_HAND2 ) ? 0x0e : 0x0a;
				break;
			default:
				action = ( layer == LAYER_HAND2 ) ? 0x0c : 0x0b;
		}
	}

	//	on horse-specific animations
	if ( IsStatFlag(STATF_OnHorse) )
	{
		switch ( action )
		{
		case 0x00:
		case 0x01:
		case 0x0f:
			action = 0x17;
			break;
		case 0x02:
		case 0x03:
			action = 0x18;
			break;
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x10:
		case 0x1f:
			action = 0x1a;
			break;
		case 0x11:
			action = 0x1b;
			break;
		case 0x20:
		case 0x21:
		case 0x22:
		case 0x13:
			action = 0x1c;
			break;
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x14:
		case 0x1e:
		case 0x05:
		case 0x06:
			action = 0x1d;
			break;
		default:
			action = 0x19;
		}
	}

	if ( !IsHuman() )
	{
		return (charDef->m_Anims & (1<<action)) ? action : -1;
	}

	//	NPC-specific (animals and monsters) lies here
	if (( charDef->GetDispID() >= CREID_HORSE1 ) && ( charDef->GetDispID() < CREID_MAN ))
	{
		//	animal translations
		switch ( action )
		{
			case 0x00:
			case 0x01:
			case 0x0f:
				action = 0x00;
				break;
			case 0x02:
			case 0x03:
				action = 0x01;
				break;
			case 0x11:
			case 0x22:
				action = 0x03;
				break;
			case 0x09:
			case 0x0a:
			case 0x0b:
			case 0x0c:
			case 0x0d:
			case 0x0e:
			case 0x12:
			case 0x13:
			case 0x1f:
				action = 0x05 + Calc_GetRandVal(2);
				break;
			case 0x14:
				action = 0x07;
				break;
			case 0x15:
				action = 0x08;
				break;
			case 0x05:
				action = 0x09;
				break;
			case 0x06:
				action = 0x0a;
				break;
			case 0x1e:
			case 0x20:
			case 0x21:
				action = 0x0b;
				break;
			case 0x16:
				action = 0x0c;
				break;
			default:
				action = 0x02;
				break;
		}
	}
	else
	{
		//	monster translations
		switch ( action )
		{
			case 0x15:
				action = 0x02;
				break;
			case 0x16:
				action = 0x03;
				break;
			case 0x09:
			case 0x0a:
			case 0x0b:
			case 0x0c:
			case 0x0d:
			case 0x0e:
			case 0x12:
			case 0x13:
			case 0x1f:
				action = 0x04 + Calc_GetRandVal(3);
				break;
			case 0x14:
				switch ( Calc_GetRandVal(3))
				{
					case 0: action = 0x0a; break;
					case 1: action = 0x10; break;
					case 2: action = 0x11; break;
				}
				break;
			case 0x11:
				action = 0x0b;
				break;
			case 0x10:
				action = 0x0c;
				break;
			default:
				action = 0x00;
		}
	}
	return (charDef->m_Anims & (1<<action)) ? action : -1;
}

void CChar::UpdateAnimate(int action, bool fBackward, BYTE iFrameDelay, int frameCount)
{
	action = translateAnimation(action);
	if ( action < 0 ) return;

	CCommand cmd;
	cmd.CharAction.m_Cmd = XCMD_CharAction;
	cmd.CharAction.m_UID = GetUID();
	cmd.CharAction.m_action = action;
	cmd.CharAction.m_framecount = frameCount;
	cmd.CharAction.m_repeat = 1;		// 1, repeat count. 0=forever.
	cmd.CharAction.m_backward = fBackward ? 1 : 0;	// 0, backwards (0/1)
	cmd.CharAction.m_repflag = 0;		// 0=dont repeat. 1=repeat
	cmd.CharAction.m_framedelay = fBackward ? 0 : iFrameDelay;	// 1, 0=fastest.

	UpdateCanSee(&cmd, sizeof(cmd.CharAction));
}

void CChar::UpdateMode( CClient * pExcludeClient, bool fFull )
{
	// If character status has been changed (Polymorph, war mode or hide), resend him
	ClientIterator it;
	CClient *client;
	while ( client = it.next() )
	{
		if ( pExcludeClient == client )
			continue;
		if ( !client->CanSee(this) )
		{
			// In the case of "INVIS" used by GM's we must use this.
			if ( GetDist(client->GetChar()) <= UO_MAP_VIEW_SIZE )
				client->addObjectRemove(this);

			continue;
		}

		if ( !client->IsPriv(PRIV_DEBUG) )
		{
			if ( fFull )
				client->addChar(this);
			else
				client->addCharMove(this);
		}
	}
}


void CChar::UpdateMove( CPointMap pold, CClient * pExcludeClient, bool fFull )
{
	// Who now sees this char? Did they just see him move ?
	ClientIterator it;
	CClient *client;
	while ( client = it.next() )
	{
		if ( client == pExcludeClient )
			continue;
		if ( client == m_pClient && fFull )
		{
			// What do i now see ?
			client->addMap( (CPointMap*)(pold.IsValidPoint() ? &pold : NULL), true );
			client->addPlayerView( pold );
			continue;
		}
		CChar * pChar = client->GetChar();
		if ( pChar == NULL )
			continue;

		bool fCouldSee = ( pold.GetDist( pChar->GetTopPoint()) <= UO_MAP_VIEW_SIZE );

		if ( !client->CanSee(this) )
		{	// can't see me now.
			if ( fCouldSee )
				client->addObjectRemove(this);
		}
		else if ( fCouldSee )	// They see me move.
		{
			client->addCharMove(this);
		}
		else					// first time this client has seen me.
		{
			client->addChar(this);
		}
	}
}

void CChar::UpdateSpeedMode()
{
	if ( g_Serv.IsLoading() )
		return;

	if ( !m_pPlayer )
		return;

	if ( IsClient() )
		GetClient()->addSpeedMode( m_pPlayer->m_speedMode );
}

void CChar::UpdateDir( DIR_TYPE dir )
{
	if ( dir != m_dirFace && dir < DIR_QTY )
	{
		m_dirFace = dir;	// face victim.
		UpdateMove( GetTopPoint(), NULL, true );
	}
}

void CChar::UpdateDir( const CPointMap & pt )
{
	// Change in direction.
	UpdateDir( GetTopPoint().GetDir( pt ));
}

void CChar::UpdateDir( const CObjBaseTemplate * pObj )
{
	if ( pObj == NULL )
		return;
	pObj = pObj->GetTopLevelObj();
	if ( pObj == this )		// In our own pack.
		return;
	UpdateDir( pObj->GetTopPoint());
}

void CChar::Update( const CClient * pClientExclude ) // If character status has been changed (Polymorph), resend him
{
	// Or I changed looks. I moved or somebody moved me?
	ClientIterator it;
	CClient *client;
	while ( client = it.next() )
	{
		if ( client == pClientExclude )
			continue;
		if ( client == m_pClient )
		{
			client->addReSync();
		}
		else if ( client->CanSee(this) )
		{
			client->addChar(this);
		}
	}
}

void CChar::SoundChar( CRESND_TYPE type )
{
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
				}
			}
		} break;

		default:
		{
			id = pCharDef->m_soundbase + type;
			switch ( pCharDef->m_soundbase )	// some creatures have no base sounds.
			{
				case 128: // old versions
				case 181:
				case 199:
					if ( type <= CRESND_RAND2 )
						id = 0;
					break;
				case 130: // ANIMALS_DEER3
				case 183: // ANIMALS_LLAMA3
				case 201: // ANIMALS_RABBIT3
					if ( type <= CRESND_RAND2 )
						id = 0;
					else
						id -= 2;
					break;
			}
		} break;
	}

	if ( type == CRESND_HIT )
	{
		CItem * pWeapon = m_uidWeapon.ItemFind();
		if ( pWeapon != NULL )
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
					// 0x234 = xbow ( hit)
					id = 0x234;
					break;
			}
		}
		else if ( id == 0 )
		{
			static const SOUND_TYPE sm_Snd_Hit[] =
			{
				0x135, //= hit01 = (slap)
				0x137, //= hit03 = (hit sand)
				0x13b, //= hit07 = (hit slap)
			};
			id = sm_Snd_Hit[ Calc_GetRandVal( COUNTOF( sm_Snd_Hit )) ];
		}
	}

	Sound(id);
}

int CChar::ItemPickup(CItem * pItem, int amount)
{
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

	if ( IsClient() )
	{
		const CItem *pItemCont	= dynamic_cast <const CItem*> (pItem->GetParent());
		if ( pItemCont )
		{
			// Don't allow taking items from the bank unless we opened it here
			if ( pItemCont->IsType( IT_EQ_BANK_BOX ) && ( pItemCont->m_itEqBankBox.m_pntOpen != GetTopPoint() ) )
				return -1;

			// Check subcontainers too
			CChar * pCharTop = dynamic_cast <CChar*> (pItem->GetTopLevelObj());
			if (( pCharTop != NULL ) && ( pCharTop->GetBank()->IsItemInside( pItemCont ) ) && ( pCharTop->GetBank()->m_itEqBankBox.m_pntOpen != GetTopPoint() ))
				return -1;
		}
	}

	const CObjBaseTemplate * pObjTop = pItem->GetTopLevelObj();
	const CChar * pChar = dynamic_cast <const CChar*> (pObjTop);

	if ( pChar != this &&
		pItem->IsAttr(ATTR_OWNED) &&
		pItem->m_uidLink != GetUID() &&
		!IsPriv(PRIV_ALLMOVE|PRIV_GM))
	{
muststeal:
		SysMessageDefault(DEFMSG_STEAL);
		return -1;
	}

	const CItemCorpse * pCorpseItem = dynamic_cast <const CItemCorpse *>(pObjTop);
	if ( pCorpseItem )
	{
		// Taking stuff off someones corpse can be a crime !
		if ( CheckCorpseCrime(pCorpseItem, true, false) )
			SysMessageDefault(DEFMSG_GUARDS);
	}

	int iAmountMax = pItem->GetAmount();
	if ( iAmountMax <= 0 )
		return -1;
	amount = max(1, min(amount, iAmountMax));
	int iItemWeight = ( amount == 1 ) ? pItem->GetWeight() : pItem->Item_GetDef()->GetWeight() * amount;

	// Is it too heavy to even drag ?
	bool fDrop = false;
	if ( GetWeightLoadPercent(GetTotalWeight() + iItemWeight) > 300 )
	{
		SysMessageDefault(DEFMSG_HEAVY);
		if (( pChar == this ) && ( pItem->GetParent() == GetPack() ))
		{
			fDrop = true;	// we can always drop it out of own pack !
		}
		return -1;
	}

	ITRIG_TYPE trigger;
	if ( pChar != NULL )
	{
		if ( ! pChar->NPC_IsOwnedBy( this ))
			goto muststeal;
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
		CScriptTriggerArgs Args( amount );
		if ( pItem->OnTrigger( trigger, this, &Args ) == TRIGRET_RET_TRUE )
			return -1;
		if ( trigger == ITRIG_PICKUP_PACK )
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
		}
	}
	else
		amount = iAmountMax;

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
	if ( ( trigger == ITRIG_PICKUP_GROUND ) && IsStatFlag(STATF_Insubstantial|STATF_Invisible|STATF_Hidden) )
	{
        pItem->RemoveFromView(m_pClient);
	}

	// Pick it up.
	pItem->SetDecayTime(-1);	// Kill any decay timer.
	LayerAdd( pItem, LAYER_DRAGGING );

	return amount;
}

bool CChar::ItemBounce( CItem * pItem )
{
	// We can't put this where we want to
	// So put in my pack if i can. else drop.
	// don't check where this came from !


	if ( pItem == NULL )
		return false;

	CItemContainer * pPack = GetPackSafe();
	if ( pItem->GetParent() == pPack )
		return true;

	LPCTSTR pszWhere = NULL;
	if ( CanCarry( pItem ))
	{
		// if we can carry it
		pszWhere = g_Cfg.GetDefaultMsg( DEFMSG_BOUNCE_PACK );
		if ( pPack == NULL )
			goto dropit;	// this can happen at load time.

		pPack->ContentAdd( pItem ); // Add it to pack
		Sound( pItem->GetDropSound( pPack ));
	}
	else
	{
dropit:
		if ( !GetTopPoint().IsValidPoint() )
		{
			// NPC is being created and has no valid point yet.
			if (pszWhere)
				g_Log.Error("No pack to place loot item '%s' for NPC '%s'\n", pItem->GetResourceName(), GetResourceName());
			else
				g_Log.Error("Loot item %s too heavy for NPC %s\n", pItem->GetResourceName(), GetResourceName());
			pItem->Delete();
			return false;
		}
		pszWhere = g_Cfg.GetDefaultMsg(DEFMSG_FEET);
		ItemDrop(pItem, GetTopPoint());
	}

	SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_ITEMPLACE ), pItem->GetName(), pszWhere );
	return true;
}

bool CChar::ItemDrop( CItem * pItem, const CPointMap & pt )
{
	// A char actively drops an item on the ground.
	if ( pItem == NULL )
		return false;

	CItemBase * pItemDef = pItem->Item_GetDef();
	if (( g_Cfg.m_fFlipDroppedItems || pItemDef->Can(CAN_I_FLIP)) &&
		pItem->IsMovableType() &&
		! pItemDef->IsStackableType())
	{
		// Does this item have a flipped version.
		pItem->SetDispID( pItemDef->GetNextFlipID( pItem->GetDispID()));
	}

	return( pItem->MoveToCheck( pt, this ));
}

bool CChar::ItemEquip( CItem * pItem, CChar * pCharMsg )
{
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

	if ( IsTrigUsed(TRIGGER_EQUIPTEST) )
	{
		int iRet = pItem->OnTrigger(ITRIG_EQUIPTEST, this);

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
	if ( !pItem->IsItemEquipped() )	// Equip failed ? Did it just go into pack ?
		return false;

	if ( pItem->OnTrigger(ITRIG_EQUIP, this) == TRIGRET_RET_TRUE )
	{
		return false;
	}

	if ( !pItem->IsItemEquipped() )	// Equip failed ? (Did it just go into pack ?
		return false;

	OnToggleEquip(pItem, true);

	Spell_Effect_Add(pItem);	// if it has a magic effect.

	if ( CItemBase::IsVisibleLayer(layer) )	// visible layer ?
		Sound(0x057);

	return true;
}

void CChar::EatAnim( LPCTSTR pszName, int iQty )
{
	Stat_SetVal( STAT_FOOD, Stat_GetVal(STAT_FOOD) + iQty );

	static const SOUND_TYPE sm_EatSounds[] = { 0x03a, 0x03b, 0x03c };

	Sound( sm_EatSounds[ Calc_GetRandVal( COUNTOF(sm_EatSounds)) ] );
	UpdateAnimate(34);

	TEMPSTRING(pszMsg);
	sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_EATSOME), pszName);
	Emote(pszMsg);
}

bool CChar::Reveal( DWORD dwFlags )
{
	// Some outside influence may be revealing us.

	if ( !IsStatFlag(dwFlags) )
		return false;

	bool fInvis = false;
	if (( dwFlags & STATF_Invisible ) && IsStatFlag( STATF_Invisible  ))
	{
		fInvis = true;
		SetHue( m_prev_Hue );
	}

	StatFlag_Clear(dwFlags);
	if ( IsStatFlag(STATF_Invisible|STATF_Hidden|STATF_Insubstantial) )
		return false;

	if ( fInvis )
	{
		RemoveFromView();
		Update();
	}
	else
		UpdateMode(NULL, true);

	SysMessageDefault(DEFMSG_HIDING_REVEALED);
	return true;
}

void CChar::SpeakUTF8( LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang )
{
	// Ignore the font argument here !

	if ( IsStatFlag(STATF_Stone))
		return;
	Reveal(STATF_Hidden);
	if ( mode == TALKMODE_YELL && GetPrivLevel() >= PLEVEL_Counsel && g_Cfg.m_iDistanceYell > 0 )
	{	// Broadcast yell.
		mode = TALKMODE_BROADCAST;	// GM Broadcast (Done if a GM yells something)
	}
	CObjBase::SpeakUTF8( pszText, wHue, mode, font, lang );
}
void CChar::SpeakUTF8Ex( const NWORD * pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang )
{
	// Ignore the font argument here !

	if ( IsStatFlag(STATF_Stone))
		return;
	Reveal(STATF_Hidden);
	if ( mode == TALKMODE_YELL && GetPrivLevel() >= PLEVEL_Counsel && g_Cfg.m_iDistanceYell > 0 )
	{	// Broadcast yell.
		mode = TALKMODE_BROADCAST;	// GM Broadcast (Done if a GM yells something)
	}
	if ( m_pNPC )
	{
		wHue = m_pNPC->m_SpeechHue;
	}
	CObjBase::SpeakUTF8Ex( pszText, wHue, mode, font, lang );
}
void CChar::Speak( LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font )
{
	// Speak to all clients in the area.
	// Ignore the font argument here !

	if ( IsStatFlag(STATF_Stone))
		return;
	Reveal(STATF_Hidden);
	if ( mode == TALKMODE_YELL && GetPrivLevel() >= PLEVEL_Counsel && g_Cfg.m_iDistanceYell > 0 )
	{	// Broadcast yell.
		mode = TALKMODE_BROADCAST;	// GM Broadcast (Done if a GM yells something)
	}
	if ( m_pNPC )
	{
		wHue = m_pNPC->m_SpeechHue;
	}
	CObjBase::Speak( pszText, wHue, mode, font );
}

CItem * CChar::Make_Figurine( UID uidOwner, ITEMID_TYPE id )
{
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
	m_atRidden.m_FigurineUID = pItem->GetUID();
	StatFlag_Set(STATF_Ridden);
	Skill_Start(NPCACT_RIDDEN);
	SetDisconnected();

	return pItem;
}

CItem * CChar::NPC_Shrink()
{
	// This will just kill conjured creatures.
	if ( IsStatFlag(STATF_Conjured) )
	{
		Stat_SetVal(STAT_STR, 0);
		return NULL;
	}

	CItem * pItem = Make_Figurine(UID_CLEAR, ITEMID_NOTHING);
	if ( !pItem )
		return NULL;

	pItem->SetAttr(ATTR_MAGIC);
	pItem->MoveToCheck(GetTopPoint());
	return pItem;
}

CItem * CChar::Horse_GetMountItem() const
{
	// I am a horse.
	// Get my mount object. (attached to my rider)

	if ( ! IsStatFlag( STATF_Ridden ))
		return NULL;

	CItem * pItem = m_atRidden.m_FigurineUID.ItemFind();
	if ( pItem == NULL ||
		( ! pItem->IsType( IT_FIGURINE ) && ! pItem->IsType( IT_EQ_HORSE )))
	{
		return NULL;
	}
	return( pItem );
}

CChar * CChar::Horse_GetMountChar() const
{
	CItem * pItem = Horse_GetMountItem();
	if ( pItem == NULL )
		return NULL;
	return( dynamic_cast <CChar*>( pItem->GetTopLevelObj()));
}

bool CChar::Horse_Mount(CChar *pHorse) // Remove horse char and give player a horse item
{
	// RETURN:
	//  true = done mounting so take no more action.
	//  false = we can't mount this so do something else.

	if ( !CanTouch(pHorse) )
	{
		SysMessageDefault(DEFMSG_MOUNT_DIST);
		return false;
	}

	ITEMID_TYPE id;
	TEMPSTRING(sMountDefname);
	sprintf(sMountDefname, "mount_0x%x", pHorse->GetDispID());
	id = (ITEMID_TYPE)g_Exp.m_VarDefs.GetKeyNum(sMountDefname);
	if ( id <= ITEMID_NOTHING )
		return false;

	if ( IsStatFlag(STATF_DEAD) || ! IsHuman() )	// only humans can ride horses.
	{
		SysMessageDefault(DEFMSG_MOUNT_UNABLE);
		return false;
	}
	if ( pHorse->m_pPlayer || !pHorse->NPC_IsOwnedBy(this) )
	{
		SysMessageDefault(DEFMSG_MOUNT_DONTOWN);
		return false;
	}

	Horse_UnMount();	// unmount if already on a horse.

	CItem * pItem = pHorse->Make_Figurine(GetUID(), id);
	if ( !pItem )
		return false;

	pItem->SetType(IT_EQ_HORSE);
	pItem->SetTimeout(TICK_PER_SEC);// The first time we give it immediatly a tick, then give the horse a tick everyone once in a while.
	LayerAdd(pItem, LAYER_HORSE);	// equip the horse item
	return true;
}

bool CChar::Horse_UnMount() // Get off a horse (Remove horse item and spawn new horse)
{
	if ( ! IsStatFlag( STATF_OnHorse ))
		return false;

	CItem * pItem = LayerFind( LAYER_HORSE );
	if ( pItem == NULL )
	{
		StatFlag_Clear( STATF_OnHorse );	// flag got out of sync !
		return false;
	}

	// What creature is the horse item ?
	CChar *pHorse = Use_Figurine(pItem);
	pItem->Delete();

	if ( ( pHorse->Stat_GetVal( STAT_STR ) <= 0 ) || ( pHorse->IsStatFlag( STATF_DEAD ) ) )
	{
		// Horsie is dead!
		pHorse->StatFlag_Clear( STATF_DEAD );
		pHorse->Stat_SetVal( STAT_STR, pHorse->Stat_GetAdjusted( STAT_STR ) );
		pHorse->Death();
	}

	return true;
}

bool CChar::OnTickEquip( CItem * pItem )
{
	// A timer expired for an item we are carrying.
	// Does it periodically do something ?
	// REUTRN:
	//  false = delete it.

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
		return false;

	case LAYER_SPECIAL:
		switch ( pItem->GetType())
		{
		case IT_EQ_SCRIPT:	// pure script.
			break;
		case IT_EQ_MEMORY_OBJ:
			return Memory_OnTick( dynamic_cast <CItemMemory*>( pItem ));
		default:
			break;
		}
		break;

	case LAYER_FLAG_Stuck:
		// Only allow me to try to damage the web so often
		// Non-magical. held by something.
		// IT_EQ_STUCK
		pItem->SetTimeout( -1 );
		return true;

	case LAYER_HORSE:
		// Give my horse a tick. (It is still in the game !)
		// NOTE: What if my horse dies (poisoned?)
		{
			// NPCs with just an item equipped is fine
			// but still give ticks just in case
			if ( m_pNPC )
			{
				pItem->SetTimeout( 10 * TICK_PER_SEC );
				return true;
			}

			CChar * pHorse = pItem->m_itFigurine.m_UID.CharFind();
			if ( pHorse == NULL )
				return false;
			
			if ( ( pHorse->m_Stat[STAT_STR].m_val <= 0 ) || ( pHorse->IsStatFlag( STATF_DEAD ) ) )
			{
				DEBUG_ERR(( "Character %s (0%x) riding dead horse (0%x) - forcing death on horse\n", this->GetName(), (DWORD)this->GetUID(), (DWORD)pHorse->GetUID() ));
				this->Horse_UnMount();

				if ( pHorse != NULL )
					pHorse->Delete();

				return false;
			}

			pItem->SetTimeout( 10 * TICK_PER_SEC );

			if ( pHorse->Fight_IsActive() )
				pHorse->Fight_ClearAll();

			if ( pHorse->Skill_GetActive() != NPCACT_RIDDEN )
				pHorse->Skill_Start( NPCACT_RIDDEN );

			bool needDelete = !pHorse->OnTick();
			if ( needDelete )
				pHorse->Delete();

			return ( !needDelete );
		}

	case LAYER_FLAG_Murders:
		// decay the murder count.
		if ( ! m_pPlayer || m_pPlayer->m_wMurders <= 0  )
			return false;

		CScriptTriggerArgs	args;
		args.m_iN1 = m_pPlayer->m_wMurders-1;
		args.m_iN2 = g_Cfg.m_iMurderDecayTime;
		if ( IsTrigUsed(TRIGGER_MURDERDECAY) )
		{
			OnTrigger(CTRIG_MurderDecay, this, &args);
			if ( args.m_iN1 < 0 ) args.m_iN1 = 0;
			if ( args.m_iN2 < 1 ) args.m_iN2 = g_Cfg.m_iMurderDecayTime;
		}
		m_pPlayer->m_wMurders = args.m_iN1;
		if ( m_pPlayer->m_wMurders == 0 ) return false;
		pItem->SetTimeout(args.m_iN2);	// update it's decay time.
		return true;
	}

	if ( pItem->IsType( IT_SPELL ))
	{
		return Spell_Equip_OnTick(pItem);
	}

	return( pItem->OnTick());
}

bool CChar::SetPoisonCure( int iSkill, bool fExtra )
{
	// Leave the anitdote in your body for a while.
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
	Update();
	return true;
}

bool CChar::SetPoison( int iSkill, int iTicks, CChar * pCharSrc )
{
	// SPELL_Poison
	// iSkill = 0-1000 = how bad the poison is
	// iTicks = how long to last.
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
		if ( pPoison)
		{
			pPoison->m_itSpell.m_spellcharges += iTicks;
		}
		return false;
	}

	SysMessage( "You have been poisoned!" );

	// Release if paralyzed ?
	StatFlag_Clear( STATF_Freeze );	// remove paralyze.

	// Might be a physical vs. Magical attack.
	pPoison = Spell_Effect_Create( SPELL_Poison, LAYER_FLAG_Poison, iSkill, (1+Calc_GetRandVal(2))*TICK_PER_SEC, pCharSrc );
	pPoison->m_itSpell.m_spellcharges = iTicks;	// how long to last.
	UpdateStatsFlag();
	return true;
}

CItemCorpse * CChar::MakeCorpse( bool fFrontFall )
{
	// some creatures (Elems) have no corpses.

	bool fLoot = ! IsStatFlag( STATF_Conjured );

	int iDecayTime = -1;	// never default.
	CItemCorpse * pCorpse = NULL;

	if ( fLoot &&
		GetDispID() != CREID_WATER_ELEM &&
		GetDispID() != CREID_AIR_ELEM &&
		GetDispID() != CREID_FIRE_ELEM &&
		GetDispID() != CREID_VORTEX &&
		GetDispID() != CREID_BLADES )
	{
		if ( m_pPlayer )
		{
			Horse_UnMount(); // If i'm conjured then my horse goes with me.
		}

		CItem* pItemCorpse = CItem::CreateScript( ITEMID_CORPSE, this );
		pCorpse = dynamic_cast <CItemCorpse *>(pItemCorpse);
		if ( pCorpse == NULL )	// Weird internal error !
		{
			pItemCorpse->Delete();
			goto nocorpse;
		}

		TEMPSTRING(pszMsg);
		sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_CORPSE_OF), GetName());
		pCorpse->SetName(pszMsg);
		pCorpse->SetHue( GetHue());
		pCorpse->SetCorpseType(GetDispID());
		pCorpse->m_itCorpse.m_BaseID = m_prev_id;	// id the corpse type here !
		pCorpse->m_itCorpse.m_facing_dir = m_dirFace;
		pCorpse->SetAttr(ATTR_INVIS);	// Don't display til ready.

		pCorpse->m_itCorpse.m_timeDeath = CServTime::GetCurrentTime();	// death time.
		pCorpse->m_itCorpse.m_uidKiller = m_Act_Targ;
		iDecayTime = m_pPlayer ? g_Cfg.m_iDecay_CorpsePlayer : g_Cfg.m_iDecay_CorpseNPC;

		if ( m_pPlayer )	// not being deleted.
			pCorpse->m_uidLink = GetUID();
	}
	else
	{
nocorpse:
		// Some creatures can never sleep. (not corpse)
		if ( ! IsStatFlag( STATF_DEAD ))
			return NULL;

		if ( m_pPlayer )
		{
			StatFlag_Clear( STATF_Conjured );
			Horse_UnMount();
		}

		if ( IsHuman())
			return NULL;	// conjured humans just disapear.

		CItem * pItem = CItem::CreateScript(ITEMID_FX_SPELL_FAIL, this);
		if ( pItem )
			pItem->MoveToDecay(GetTopPoint(), 2*TICK_PER_SEC);
	}

	// can fall forward.
	DIR_TYPE dir = m_dirFace;
	if ( fFrontFall )
	{
		dir = (DIR_TYPE) ( dir | 0x80 );
		if ( pCorpse )
			pCorpse->m_itCorpse.m_facing_dir = dir;
	}

	PacketDeath *cmd = new PacketDeath(this, pCorpse);
	UpdateCanSee(cmd, m_pClient);

	// Move non-newbie contents of the pack to corpse. (before it is displayed)
	if ( fLoot )
	{
		DropAll( pCorpse );
	}
	if ( pCorpse )
	{
		pCorpse->ClrAttr(ATTR_INVIS);	// make visible.
		pCorpse->MoveToDecay(GetTopPoint(), iDecayTime);
	}

	return( pCorpse );
}

bool CChar::RaiseCorpse( CItemCorpse * pCorpse )
{
	// We are creating a char from the current char and the corpse.
	// Move the items from the corpse back onto us.

	// If NPC is disconnected then reconnect them.
	// If the player is off line then don't allow this !!!

	if ( !pCorpse )
		return false;

	if ( pCorpse->GetCount())
	{
		CItemContainer * pPack = GetPackSafe();

		CItem* pItemNext;
		for ( CItem * pItem = pCorpse->GetContentHead(); pItem!=NULL; pItem=pItemNext )
		{
			pItemNext = pItem->GetNext();
			if ( pItem->IsType( IT_HAIR ) ||
				pItem->IsType( IT_BEARD ) ||
				pItem->IsAttr( ATTR_MOVE_NEVER ))
				continue;	// Hair on corpse was copied!
			// Redress if equipped.
			if ( pItem->GetContainedLayer())
				ItemEquip( pItem );	// Equip the item.
			else
				pPack->ContentAdd( pItem );	// Toss into pack.
		}

		// Any items left just dump on the ground.
		pCorpse->ContentsDump( GetTopPoint());
	}

	if ( pCorpse->IsTopLevel() || pCorpse->IsItemInContainer())
	{
		// I should move to where my corpse is just in case.
		m_fClimbUpdated = false; // update climb height
		MoveToChar( pCorpse->GetTopLevelObj()->GetTopPoint());
	}

	// Corpse is now gone. 	// 0x80 = on face.
	Update();
	UpdateDir( (DIR_TYPE)( pCorpse->m_itCorpse.m_facing_dir &~ 0x80 ));
	UpdateAnimate( ( pCorpse->m_itCorpse.m_facing_dir & 0x80 ) ? 22 : 21, true, 2 );

	pCorpse->Delete();

	return true;
}

bool CChar::Death()
{
	// RETURN: false = delete

	if ( IsStatFlag(STATF_DEAD|STATF_INVUL) )
		return true;
	if ( m_pNPC )
	{
		//	leave no corpse if for some reason creature dies while mounted
		if ( IsStatFlag(STATF_Ridden) )
			StatFlag_Set(STATF_Conjured);
	}

	if ( OnTrigger(CTRIG_Death, this) == TRIGRET_RET_TRUE )
		return true;

	// I am dead and we need to give credit for the kill to my attacker(s).
	TEMPSTRING(pszKillStr);
	int iKillStrLen = sprintf(pszKillStr, g_Cfg.GetDefaultMsg(DEFMSG_KILLED_BY), (m_pPlayer)?'P':'N', GetName());
	int iKillers = 0;

	// Look through my memories of who i was fighting. (make sure they knew they where fighting me)
	CChar	*pKiller = NULL;
	CItem	*pItemNext;
	CItem	*pItem;
	int		killedBy = 0;
	int		warTargets = 0;
	for ( pItem = GetContentHead(); pItem; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();

		if ( pItem->IsMemoryTypes(MEMORY_HARMEDBY|MEMORY_AGGREIVED) )
		{
			if ( (static_cast <CItemMemory *>(pItem))->m_uidLink.CharFind() )
				killedBy++;
		}
		if ( pItem->IsMemoryTypes(MEMORY_FIGHT|MEMORY_WAR_TARG) )
		{
			if ( (static_cast <CItemMemory *>(pItem))->m_uidLink.CharFind() )
				warTargets++;
		}
	}

	CScriptTriggerArgs args(this);
	args.m_iN2 = killedBy;
	args.m_iN3 = warTargets;

	for ( pItem = GetContentHead(); pItem; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();

      	if ( pItem->IsType(IT_EQ_TRADE_WINDOW) )
		{
			CItemContainer* pCont = dynamic_cast <CItemContainer*> (pItem);
			if ( pCont )
			{
				pCont->Trade_Delete();
				continue;
			}
		}

		// Sets OBODY value to BODY if LAYER_Flag_Wool is found on an NPC
		// Fixes issue with woolly sheep giving wool resource when corpse is carved after being shorn.
		if ( m_pNPC && (pItem->GetEquipLayer() == LAYER_FLAG_Wool) )
		{
			this->m_prev_id = this->GetID();
		}

		// i was harmed by this killer. noto his as a killer
		if ( pItem->IsMemoryTypes(MEMORY_HARMEDBY|MEMORY_AGGREIVED) )
		{
			CItemMemory * pMemory = static_cast <CItemMemory *>(pItem);
			pKiller = pMemory->m_uidLink.CharFind();
			if ( pKiller )
			{
				int iRet = TRIGRET_RET_DEFAULT;

				if ( IsTrigUsed(TRIGGER_KILL) )
				{
					args.m_iN1 = 0;
					iRet = pKiller->OnTrigger(CTRIG_Kill, pKiller, &args);
				}

				if ( iRet != TRIGRET_RET_TRUE )
				{
					if ( !pKiller->m_pPlayer && pKiller->NPC_PetGetOwner() )
					{
						pKiller->Noto_Kill(this, false, killedBy-1);
						(pKiller->NPC_PetGetOwner())->Noto_Kill(this, true, killedBy-1);
					}
					else
						pKiller->Noto_Kill(this, false, killedBy-1);

					iKillStrLen += sprintf(pszKillStr+iKillStrLen, "%s%c'%s'", iKillers ? ", " : "", (pKiller->m_pPlayer)?'P':'N', pKiller->GetName());
					iKillers ++;
				}
			}
			Memory_ClearTypes(pMemory, 0xFFFF);
		}
		else if ( pItem->IsMemoryTypes(MEMORY_FIGHT|MEMORY_WAR_TARG) )
		{
			CItemMemory *pMemory = static_cast <CItemMemory *>(pItem);
			CChar		*pWar = pMemory->m_uidLink.CharFind();

			if ( pWar && IsTrigUsed(TRIGGER_KILL) )
			{
				args.m_iN1 = 1;
				pWar->OnTrigger(CTRIG_Kill, pWar, &args);
			}
		}
	}

	//	No aggressor/killer detected. Try detect person last hit me  from the act target
	if ( !pKiller )
	{
		CObjBase	*ob = g_World.FindUID(m_Act_Targ);
		if ( ob )
			pKiller = static_cast <CChar *>(ob);
	}

	//	Check if having active quests with this target and mark it complete if complete
	if ( pKiller && pKiller->m_pPlayer )
	{
		vector<Quest *>::iterator it;
		for ( it = pKiller->m_pPlayer->m_quests.begin(); it != pKiller->m_pPlayer->m_quests.end(); it++ )
		{
			vector<QuestDef::ResourceTyped *>::iterator res;
			for ( res = (*it)->m_objectives.begin(); res != (*it)->m_objectives.end(); res++ )
			{
				if ( (*res)->m_type == 2 )
				{
					CCharBase *pCharBase = Char_GetDef();
					if ( (*res)->m_ref == GetBaseID() )
					{
						(*res)->m_amount--;
						if ( (*res)->m_amount <= 0 )
						{
							(*it)->m_objectives.erase(res);
							delete *res;
							pKiller->SysMessage("You've completed part of one of your quests.");
						}
						goto endofquest;
					}
				}
			}
		}
endofquest:
		;
	}

	// record the kill event for posterity.

	iKillStrLen += sprintf( pszKillStr+iKillStrLen, ( iKillers ) ? ".\n" : "accident.\n" );
	if ( m_pPlayer ) g_Log.Event(LOGL_EVENT|LOGM_KILLS, pszKillStr);

	if ( m_pParty )
	{
		m_pParty->SysMessageAll( pszKillStr );
	}

	NPC_PetClearOwners();	// Forgot who owns me. dismount my master if ridden.
	Reveal();
	SoundChar( CRESND_DIE );
	Spell_Dispel(100);		// Get rid of all spell effects.

	// Only players should loose stats upon death.
	if ( m_pPlayer )
	{
		m_pPlayer->m_wDeaths++;
		Noto_Fame( -Stat_GetAdjusted(STAT_FAME)/10 );

		//	experience could go down
		if ( g_Cfg.m_iExperienceMode && ( g_Cfg.m_iExperienceMode&EXP_MODE_ALLOW_DOWN ))
		{
			ChangeExperience(-((int)m_exp/10));
		}
	}

	// create the corpse item.
	StatFlag_Set(STATF_DEAD);
	StatFlag_Clear(STATF_Stone|STATF_Freeze|STATF_Hidden);
	Stat_SetVal(STAT_STR, 0);

	CItemCorpse * pCorpse = MakeCorpse(Calc_GetRandVal(2));
	if ( pCorpse )
	{
   		CScriptTriggerArgs Args(pCorpse);
   		OnTrigger(CTRIG_DeathCorpse, this, &Args);
	}

	//	clear list of attackers
	m_lastAttackers.clear();
	
	if ( m_pPlayer )
	{
		SetHue( HUE_DEFAULT );	// Get all pale.

		LPCTSTR pszGhostName = ( m_prev_id == CREID_ELFMAN ) ? "c_elf_ghost_man" : "c_ghost_man";
		CCharBase	*pCharDefPrev = CCharBase::FindCharBase( m_prev_id );

		if ( pCharDefPrev && pCharDefPrev->IsFemale() )
			pszGhostName = ( m_prev_id == CREID_ELFWOMAN ) ? "c_elf_ghost_woman" : "c_ghost_woman";

		SetID( (CREID_TYPE) g_Cfg.ResourceGetIndexType( RES_CHARDEF, pszGhostName ));
		LayerAdd( CItem::CreateScript( ITEMID_DEATHSHROUD, this ));
		Update();		// show everyone I am now a ghost.

		// Manifest the ghost War mode for ghosts.
		if ( ! IsStatFlag(STATF_War) )
			StatFlag_Set(STATF_Insubstantial);
	}

	Skill_Cleanup();

	if ( !IsClient() )
	{
		if ( m_pPlayer )
		{
			SetDisconnected();	// Respawn the NPC later
			return true;
		}

		// Makes no sense to link the corpse to something that is not going to be valid.
		if ( pCorpse && pCorpse->m_uidLink == GetUID())
			pCorpse->m_uidLink.InitUID();

		return false;	// delete this
	}
	return true;
}


bool CChar::OnFreezeCheck(bool bTagCheck)
{
	// Check why why are held in place.
	// Can we break free ?
	// RETURN: true = held in place.

	// Do not allow move if TAG.NoMoveTill > SERV.Time, 
	// needed for script purposes.
	if ( bTagCheck == true )
	{
		if ( GetKeyNum("NoMoveTill",true) > g_World.GetCurrentTime().GetTimeRaw() )
			return true;
		DeleteKey("NoMoveTill");
		return false;
	}

	CItem * pFlag = LayerFind( LAYER_FLAG_Stuck );
	if ( pFlag == NULL )	// stuck for some other reason i guess.
	{
		SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_FROZEN));
	}
	else
	{
		// IT_EQ_STUCK
		CItem * pWeb = pFlag->m_uidLink.ItemFind();
		if ( pWeb == NULL ||
			! pWeb->IsTopLevel() ||
			pWeb->GetTopPoint() != GetTopPoint())
		{
			// Maybe we teleported away ?
			pFlag->Delete();
			return false;
		}

		// Only allow me to try to damage it once per sec.
		if ( ! pFlag->IsTimerSet())
		{
			return( Use_Obj( pWeb, false ));
		}
	}
	return( ! IsPriv( PRIV_GM ));
}

void CChar::Flip()
{
	UpdateDir( GetDirTurn( m_dirFace, 1 ));
}

CRegionBase * CChar::CanMoveWalkTo( CPointBase & ptDst, bool fCheckChars, bool fCheckOnly, DIR_TYPE dir )
{
	// For both players and NPC's
	// Walk towards this point as best we can.
	// Affect stamina as if we WILL move !
	// RETURN:
	//  ptDst.m_z = the new z
	//  NULL = failed to walk here.
	if ( IsSetMagicFlags( MAGICF_PRECAST ) && m_Act_SkillCurrent == SKILL_MAGERY )
	{
		// Casting prevents movement with precasting enabled.
		return( NULL );
	}

	if (( IsStatFlag( STATF_Freeze | STATF_Stone ) && OnFreezeCheck()) || OnFreezeCheck(true) )
	{
		// NPC's would call here.
		return NULL;	// can't move.
	}

	int iWeightLoadPercent = 0;
	if ( !fCheckOnly )
	{
		if ( Stat_GetVal(STAT_DEX) <= 0 && !IsStatFlag(STATF_DEAD) )
		{
			SysMessageDefault(DEFMSG_FATIGUE);
			return NULL;
		}
		iWeightLoadPercent = GetWeightLoadPercent(GetTotalWeight());
		if ( iWeightLoadPercent > 200 )
		{
			SysMessageDefault(DEFMSG_OVERLOAD);
			return NULL;
		}
	}

	// ok to go here ? physical blocking objects ?
	WORD wBlockFlags = 0;
	signed char ClimbHeight = ptDst.m_z;
	CRegionBase *pArea = NULL;

	if ( IsSetEF( EF_WalkCheck ) )
		pArea = CheckValidMove_New(ptDst, &wBlockFlags, dir, &ClimbHeight);
	else
		pArea = CheckValidMove(ptDst, &wBlockFlags, dir);

	if ( !pArea )
		return NULL;

	if ( fCheckOnly )
	{
		if (( g_Cfg.m_iNpcAi&NPC_AI_PATH ) && fCheckChars )	// fast lookup of being able to go through char there
		{
			if ( !IsStatFlag(STATF_DEAD|STATF_Insubstantial) )
			{
				CWorldSearch AreaChars(ptDst, 0, this);
				AreaChars.SetAllShow(true);
				while ( CChar *pChar = AreaChars.GetChar() )
				{
					if ((pChar->GetTopZ() != ptDst.m_z) || !ptDst.IsSameMap(pChar->GetTopMap()) )
						continue;

					if ( m_pNPC && !pChar->m_pPlayer )
						return NULL;	// not through non-players
					if ( pChar->IsStatFlag(STATF_DEAD|STATF_Insubstantial) || pChar->IsDisconnected())
						continue;

					//	How much stamina to push past ?
					int iStamReq = g_Cfg.Calc_WalkThroughChar(this, pChar);
					//	cannot push the char
					if ( iStamReq < 0 || Stat_GetVal(STAT_DEX) <= iStamReq )
						return NULL;
				}
			}
		}
		return pArea;
	}

	if ( ! m_pPlayer )
	{
		// Does the NPC want to walk here ?
		if ( !NPC_CheckWalkHere( ptDst, pArea, wBlockFlags ) )
			return NULL;
	}

	// Bump into other creatures ?
	if ( !IsStatFlag(STATF_DEAD|STATF_Insubstantial) && fCheckChars )
	{
		CWorldSearch AreaChars(ptDst, 0, this);
		AreaChars.SetAllShow(true);
		while ( CChar *pChar = AreaChars.GetChar() )
		{
			if (( pChar->GetTopZ() != ptDst.m_z ) || !ptDst.IsSameMap(pChar->GetTopMap()) )
				continue;

			if ( pChar->IsStatFlag(STATF_DEAD|STATF_Insubstantial) || pChar->IsDisconnected() )
			{
				if ( CanDisturb(pChar) && pChar->IsStatFlag(STATF_SpiritSpeak) )
				{
					SysMessageDefault(DEFMSG_TINGLING);
				}
				continue;
			}

			//	NPC can't bump thru other characters
			if ( !m_pPlayer ) return NULL;

			// How much stamina to push past ?
			int iStamReq = g_Cfg.Calc_WalkThroughChar(this, pChar);
			TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;

			if ( IsTrigUsed(TRIGGER_PERSONALSPACE) )
			{
				CScriptTriggerArgs Args(iStamReq);
				iRet = pChar->OnTrigger(CTRIG_PersonalSpace, this, &Args);
				iStamReq = Args.m_iN1;

				if ( iRet == TRIGRET_RET_TRUE )
					return NULL;
			}

			TEMPSTRING(pszMsg);
			if ( pChar->IsStatFlag(STATF_Invisible) )
			{
				strcpy(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_INVISIBLE));
				continue;
			}
			else if ( pChar->IsStatFlag(STATF_Hidden) )
			{
				// reveal hidden people ?
				sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_HIDING_STUMBLE), pChar->GetName() );
				pChar->Reveal(STATF_Hidden);
			}
			else if (( iStamReq < 0 ) || ( Stat_GetVal(STAT_DEX) <= iStamReq ))
			{
				if ( !IsPriv(PRIV_GM) )
				{
					sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_CANTPUSH), pChar->GetName());
					SysMessage(pszMsg);
					return NULL;
				}
			}
			else
			{
				sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_PUSH), pChar->GetName());
			}

			if ( iRet != TRIGRET_RET_FALSE )
				SysMessage(pszMsg);

			iStamReq = ( IsPriv(PRIV_GM) ? -10 : (( iStamReq < 0 ) ? 0 : iStamReq) );
			UpdateStatVal(STAT_DEX, -iStamReq);
			break;
		}
	}

	// decrease stamina if running or overloaded.
	if ( !IsPriv(PRIV_GM) )
	{
		// We are overloaded. reduce our stamina faster.
		// Running acts like an increased load.
		int iStamReq = g_Cfg.Calc_DropStamWhileMoving( this, iWeightLoadPercent );
		if ( iStamReq )
		{
			// Lower avail stamina.
			UpdateStatVal( STAT_DEX, -iStamReq );
		}
	}

	StatFlag_Mod( STATF_InDoors, ( wBlockFlags & CAN_I_ROOF ) || pArea->IsFlag(REGION_FLAG_UNDERGROUND) );

	if ( wBlockFlags & CAN_I_CLIMB )
		m_zClimbHeight = ClimbHeight;
	else
		m_zClimbHeight = 0;

	return( pArea );
}

void CChar::CheckRevealOnMove()
{
	// Are we going to reveal ourselves by moving ?
	if ( IsStatFlag(STATF_Invisible|STATF_Hidden) )
	{
		bool bReveal(false);

		if ( IsStatFlag(STATF_Fly) || !IsStatFlag(STATF_Hidden) ||
			! Skill_UseQuick( SKILL_STEALTH, Calc_GetRandVal( 105 )))
			bReveal = true;

		CScriptTriggerArgs Args(bReveal);	// ARGN1 - reveal?
		OnTrigger(CTRIG_StepStealth, this, &Args);
		bReveal = Args.m_iN1 == 1;
		if ( bReveal ) Reveal();
	}
}

bool CChar::CheckLocation( bool fStanding )
{
	// We are at this location
	// what will happen ?
	// RETURN: true = we teleported.

	if ( ! fStanding )
	{
		SKILL_TYPE iSkillActive	= Skill_GetActive();

		if ( g_Cfg.IsSkillFlag( iSkillActive, SKF_IMMOBILE ) )
		{
			Skill_Fail(false);
			return true;
		}

		if ( g_Cfg.IsSkillFlag( iSkillActive, SKF_FIGHT ) )
		{
			// Are we using a skill that is effected by motion ?
			m_atFight.m_fMoved	= 1;
		}
		else switch ( iSkillActive )
		{
		case SKILL_MEDITATION:
		case SKILL_NECROMANCY:
		case SKILL_MAGERY:
			// Skill is broken if we move ?
			break;
		case SKILL_HIDING:	// this should become stealth ?
			break;
		case SKILL_FENCING:
		case SKILL_MACEFIGHTING:
		case SKILL_SWORDSMANSHIP:
		case SKILL_WRESTLING:
			m_atFight.m_fMoved	= 1;
			break;
		case SKILL_ARCHERY:
			m_atFight.m_fMoved	= 1;
			if ( !IsSetOF( OF_Archery_CanMove ) && ! IsStatFlag( STATF_ArcherCanMove ) )
			{
				// If we moved and are wielding are in combat and are using a
				// crossbow/bow kind of weapon, then reset the weaponswingtimer.
				Fight_ResetWeaponSwingTimer();
			}
			break;
		}

		// This could get REALLY EXPENSIVE !
		if ( m_pArea->OnRegionTrigger( this, RTRIG_STEP ) == TRIGRET_RET_TRUE )
			return false;
	}

	bool	fStepCancel	= false;
	CWorldSearch AreaItems( GetTopPoint());
	while ( CItem *pItem = AreaItems.GetItem() )
	{
		int zdiff = pItem->GetTopZ() - GetTopZ();

		int	height	= pItem->Item_GetDef()->GetHeight();
		if ( height < 3 )
			height	= 3;

		if ( zdiff > height || zdiff < -3 )

		// if ( abs(zdiff) > 3 )
			continue;

		CScriptTriggerArgs Args( (int) fStanding );
		if ( pItem->OnTrigger( ITRIG_STEP, this , &Args ) == TRIGRET_RET_TRUE )
		{
			fStepCancel	= true;
			continue;
		}

		switch ( pItem->GetType() )
		{
		case IT_SHRINE:
			// Resurrect the ghost
			if ( fStanding )
				continue;
			OnSpellEffect( SPELL_Resurrection, this, 1000, pItem );
			return false;
		case IT_WEB:
			if ( fStanding )
				continue;
			// Caught in a web.
			if ( Use_Item_Web( pItem ))
				return true;
			continue;
		// case IT_CAMPFIRE:	// does nothing. standing on burning kindling shouldn't hurt us
		case IT_FIRE:
			// fire object hurts us ?
			// pItem->m_itSpell.m_spelllevel = 0-1000 = heat level.
			{
				int iSkillLevel = pItem->m_itSpell.m_spelllevel/2;
				iSkillLevel = iSkillLevel + Calc_GetRandVal(iSkillLevel);
				if ( IsStatFlag( STATF_Fly ))	// run through fire.
				{
					iSkillLevel /= 2;
				}
				OnTakeDamage( g_Cfg.GetSpellEffect( SPELL_Fire_Field, iSkillLevel ), NULL, DAMAGE_FIRE | DAMAGE_GENERAL );
			}
			Sound( 0x15f ); // Fire noise.
			return false;
		case IT_SPELL:
			{
				SPELL_TYPE Spell = (SPELL_TYPE) RES_GET_INDEX(pItem->m_itSpell.m_spell);
				OnSpellEffect( Spell, pItem->m_uidLink.CharFind(), pItem->m_itSpell.m_spelllevel, pItem );
				const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(Spell);
				if ( pSpellDef )
				{
					Sound( pSpellDef->m_sound);
				}
			}
			return false;

		case IT_SWITCH:
			if ( pItem->m_itSwitch.m_fStep )
			{
				Use_Item( pItem );
			}
			return false;
		case IT_MOONGATE:
		case IT_TELEPAD:
			if ( fStanding )
				continue;
			Use_MoonGate( pItem );
			return true;
		case IT_SHIP_PLANK:
			// a plank is a teleporter off the ship.
			if ( ! fStanding && ! IsStatFlag( STATF_Fly ))
			{
				// Find some place to go. (in direction of plank)
				if ( MoveToValidSpot(m_dirFace, UO_MAP_VIEW_SIZE, 1, true) )
				{
					pItem->SetTimeout(5*TICK_PER_SEC);	// autoclose it behind us.
					return true;
				}
			}
			continue;

		case IT_CORPSE:
			{
				if ( m_pNPC && ( g_Cfg.m_iNpcAi&NPC_AI_EXTRA ))
				{
					//	NPC are likely to loot corpses
					if ( ( Calc_GetRandVal(150) < Stat_GetAdjusted(STAT_INT) ) || ( m_pNPC->m_Brain == NPCBRAIN_ANIMAL ) )
					{
						if ( m_pArea->IsFlag(REGION_FLAG_GUARDED|REGION_FLAG_SAFE) ) ;
						else if ( IsStatFlag(STATF_Pet) && !IsStatFlag(STATF_Conjured) ) ;
						else
						{
							CItemCorpse	*pCorpse = dynamic_cast <CItemCorpse*>(this);

							if ( pCorpse && pCorpse->GetCount() )
							{
								CItem *pItem = pCorpse->GetAt( Calc_GetRandVal(pCorpse->GetCount()) );
								bool bLoot = false;

								if ( !pItem ) ;
								//	animals are looting food only
								else if ( m_pNPC->m_Brain == NPCBRAIN_ANIMAL )
								{
									switch ( pItem->GetType() )
									{
									case IT_FOOD:
									case IT_FOOD_RAW:
									case IT_MEAT_RAW:
									case IT_FRUIT:
										bLoot = true;
									}
								}
								else
								{
									if ( pItem->IsAttr(ATTR_NEWBIE) ) ;
									else if (( pItem->GetDispID() == ITEMID_BANDAGES_BLOODY1 ) || ( pItem->GetDispID() == ITEMID_BANDAGES_BLOODY2 )) ;
									else if ( pItem->GetDispID() == ITEMID_EMPTY_BOTTLE ) ;
									else if ( !CanCarry(pItem) ) ;
									else if ( pItem->IsType(IT_CONTAINER) )
									{
										CItemContainer *pItemCont = dynamic_cast <CItemContainer*>(pItem);
										if ( pItemCont->GetCount() )
											bLoot = true;
									}
									else
										bLoot = true;
								}

								if ( bLoot )
								{
									TEMPSTRING(zMsg);
									sprintf(zMsg, "looting %s from %s", pItem->GetName(), pCorpse->GetName());
									Emote(zMsg);
									ItemBounce(pItem);
								}
							}
						}
					}
				}
			}
		}
	}

	if ( fStepCancel )
		return false;

	if ( fStanding )
		return false;

	// Check the map teleporters in this CSector. (if any)
	CPointMap pt = GetTopPoint();
	CSector *pSector = pt.GetSector();
	if ( !pSector )
		return false;

	const CTeleport * pTel = pSector->GetTeleport(pt);
	if ( pTel )
	{
		if ( !IsClient() && m_pNPC )
		{
			if ( !pTel->bNpc )
				return false;

			// NPCs and gates.
			if ( m_pNPC->m_Brain == NPCBRAIN_GUARD )
			{
				// Guards won't gate into unguarded areas.
				CRegionWorld * pArea = dynamic_cast <CRegionWorld *> ( pTel->m_ptDst.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA));
				if ( !pArea || ! pArea->IsGuarded())
					return false;
			}
			else if ( Noto_IsCriminal() )
			{
				// wont teleport to guarded areas.
				CRegionWorld *pArea = dynamic_cast <CRegionWorld *> ( pTel->m_ptDst.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA));
				if ( !pArea || pArea->IsGuarded())
					return false;
			}
		}
		
		Spell_Teleport(pTel->m_ptDst, true, false, ITEMID_NOTHING);
		return true;
	}
	return false;
}

bool CChar::MoveToRegion( CRegionWorld * pNewArea, bool fAllowReject )
{
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
			if ( m_pArea->OnRegionTrigger( this, RTRIG_EXIT ) == TRIGRET_RET_TRUE )
			{
				if ( pNewArea && fAllowReject )
					return false;
			}
		}

		if ( IsClient() && pNewArea )
		{
			if ( pNewArea->IsFlag(REGION_FLAG_ANNOUNCE) && !pNewArea->IsInside2d( GetTopPoint()) )	// new area.
			{
				VariableList::Variable *pVarStr = pNewArea->m_TagDefs.GetKey("ANNOUNCEMENT");
				SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_REGION_ENTER), pVarStr ? pVarStr->GetValStr() : pNewArea->GetName());
			}

			// Is it guarded / safe / non-pvp?
			else if ( m_pArea && !IsStatFlag(STATF_DEAD) )
			{
				bool redNew = pNewArea->m_TagDefs.GetKeyNum("RED", true);
				bool redOld = m_pArea->m_TagDefs.GetKeyNum("RED", true);

				if ( pNewArea->IsGuarded() != m_pArea->IsGuarded() )
				{
					if ( pNewArea->IsGuarded() )	// now under the protection
					{
						VariableList::Variable *pVarStr = pNewArea->m_TagDefs.GetKey("GUARDOWNER");
						SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_REGION_GUARDS_1 ),
							( pVarStr ) ? (LPCTSTR)pVarStr->GetValStr() : g_Cfg.GetDefaultMsg( DEFMSG_REGION_GUARD_ART ) );
					}
					else							// have left the protection
					{
						VariableList::Variable *pVarStr = m_pArea->m_TagDefs.GetKey("GUARDOWNER");
						SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_REGION_GUARDS_2 ),
							( pVarStr ) ? (LPCTSTR)pVarStr->GetValStr() : g_Cfg.GetDefaultMsg( DEFMSG_REGION_GUARD_ART ) );
					}
				}
				if ( redNew != redOld )
				{
					SysMessagef("You have %s the red region.", ( redNew ? "entered" : "left" ));
				}
				else if ( redNew && ( redNew == redOld ))
				{
					SysMessage("You are still in the red region.");
				}
				if ( pNewArea->IsFlag(REGION_FLAG_NO_PVP) != m_pArea->IsFlag(REGION_FLAG_NO_PVP))
				{
					SysMessageDefault(( pNewArea->IsFlag(REGION_FLAG_NO_PVP)) ? DEFMSG_REGION_PVPSAFE : DEFMSG_REGION_PVPNOT );
				}
				if ( pNewArea->IsFlag(REGION_FLAG_SAFE) != m_pArea->IsFlag(REGION_FLAG_SAFE) )
				{
					SysMessageDefault((pNewArea->IsFlag(REGION_FLAG_SAFE)) ? DEFMSG_REGION_SAFETYGET : DEFMSG_REGION_SAFETYLOSE);
				}
			}
		}

		// Entering region trigger.
		if ( pNewArea )
		{
			if ( pNewArea->OnRegionTrigger( this, RTRIG_ENTER ) == TRIGRET_RET_TRUE )
			{
				if ( m_pArea && fAllowReject )
					return false;
			}
		}
	}

	m_pArea = pNewArea;
	return true;
}

bool CChar::MoveToChar( CPointMap pt )
{
	// Same as MoveTo
	// This could be us just taking a step or being teleported.
	// Low level: DOES NOT UPDATE DISPLAYS or container flags. (may be offline)
	// This does not check for gravity.
	//

	if ( ! pt.IsCharValid() || !pt.IsValidXY() )
		return false;

	if ( m_pPlayer && ! IsClient())	// moving a logged out client !
	{
		CSector *pSector = pt.GetSector();
		if ( !pSector )
			return false;

		// We cannot put this char in non-disconnect state.
		SetDisconnected();
		pSector->m_Chars_Disconnect.InsertHead(this);
		SetUnkPoint( pt );
		return true;
	}

	// Did we step into a new region ?
	CRegionWorld * pAreaNew = dynamic_cast <CRegionWorld *>( pt.GetRegion( REGION_TYPE_MULTI|REGION_TYPE_AREA ));
	if ( ! MoveToRegion( pAreaNew, true ))
		return false;

	bool fSectorChange	= pt.GetSector()->MoveCharToSector(this);
	bool fMapChange = false;
	CPointMap	prevPt	= GetUnkPoint();

	// Move this item to it's point in the world. (ground or top level)
	SetTopPoint(pt);
	if ( IsClient() && ( prevPt.m_map != pt.m_map ))
		fSectorChange = fMapChange = true;

	if ( fSectorChange && ! g_Serv.IsLoading() )	// there was a change in environment.
	{
		if ( fMapChange )
			GetClient()->addReSync(true);			// a must-have for map change

		CScriptTriggerArgs	Args( prevPt.m_x, prevPt.m_y, prevPt.m_z << 16 | prevPt.m_map );
		OnTrigger(CTRIG_EnvironChange, this, &Args);
	}

	if ( !m_fClimbUpdated )
		FixClimbHeight();

	return true;
}

bool CChar::MoveToValidSpot(DIR_TYPE dir, int iDist, int iDistStart, bool bFromShip)
{
	// Move from here to a valid spot.
	// ASSUME "here" is not a valid spot. (even if it really is)

	CPointMap pt = GetTopPoint();
	pt.MoveN( dir, iDistStart );
	pt.m_z += PLAYER_HEIGHT;

	WORD wCan = GetMoveBlockFlags();	// CAN_C_SWIM
	for ( int i=0; i<iDist; i++ )
	{
		// Don't allow boarding of other ships (they may be locked)
		CRegionBase * pRegionBase = pt.GetRegion( REGION_TYPE_MULTI );
		if ( pRegionBase && pRegionBase->IsFlag( REGION_FLAG_SHIP ) )
		{
			pt.Move( dir );
			continue;
		}

		WORD wBlockFlags = wCan;
		signed char z = g_World.GetHeightPoint( pt, wBlockFlags, true );;
		if ( ! ( wBlockFlags &~ wCan ))
		{
			// we can go here. (maybe)
			if ( Spell_Teleport( pt, true, !bFromShip, ITEMID_NOTHING) )
				return true;
		}
		pt.Move( dir );
	}
	return false;
}

TRIGRET_TYPE CChar::OnTrigger( LPCTSTR pszTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs )
{
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

	EXC_TRY("Trigger");

	if ( !m_pPlayer )			//	CHARDEF triggers (based on body type)
	{
		EXC_SET("chardef triggers");
		if ( pCharDef->HasTrigger(iAction) )
		{
			CResourceLock s;
			if ( pCharDef->ResourceLock(s) )
			{
				TRIGRET_TYPE iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
				if (( iRet != TRIGRET_RET_FALSE ) && ( iRet != TRIGRET_RET_DEFAULT ))
					return iRet;
			}
		}
	}


	//
	// Go thru the event blocks for the NPC/PC to do events.
	//
	EXC_SET("events");
	int i;
	int origEvents = m_OEvents.GetCount();
	int curEvents = origEvents;
	for ( i = 0; i < curEvents; i++ )			//	EVENTS (could be modifyed ingame!)
	{
		CResourceLink	*pLink = m_OEvents[i];
		if ( !pLink || !pLink->HasTrigger(iAction) )
			continue;
		CResourceLock s;
		if ( !pLink->ResourceLock(s) )
			continue;

		TRIGRET_TYPE iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
		if ( iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT )
			return iRet;

		curEvents = m_OEvents.GetCount();
		if ( curEvents < origEvents ) // the event has been deleted, modify the counter for other trigs to work
		{
			i--;
			origEvents = curEvents;
		}
	}

	if ( m_pNPC )								//	TEVENTS (constant events of NPCs)
	{
		EXC_SET("NPC triggers");
		for ( i=0; i < pCharDef->m_TEvents.GetCount(); i++ )
		{
			CResourceLink	*pLink = pCharDef->m_TEvents[i];
			if ( !pLink || !pLink->HasTrigger(iAction) )
				continue;
			CResourceLock s;
			if ( !pLink->ResourceLock(s) )
				continue;
			TRIGRET_TYPE iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
			if ( iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT )
				return iRet;
		}
		if ( g_Cfg.m_pEventsPetLink && g_Cfg.m_pEventsPetLink->HasTrigger(iAction) )
		{
			CResourceLock s;
			if ( g_Cfg.m_pEventsPetLink->ResourceLock(s) )
			{
				TRIGRET_TYPE iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
				if ( iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT )
					return iRet;
			}
		}
	}
	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.Debug("trigger '%s' action '%d' [0%lx]\n", pszTrigName, iAction, (DWORD)GetUID());
	EXC_DEBUG_END;
	return TRIGRET_RET_DEFAULT;
}

void CChar::OnTickFood()
{
	if ( IsStatFlag(STATF_Conjured) || !Stat_GetMax(STAT_FOOD) )
		return;	// No need for food.

	// This may be money instead of food
	if ( IsStatFlag(STATF_Pet) && !NPC_CheckHirelingStatus() )
		return;

	long	lFood = Stat_GetVal(STAT_FOOD);
   	if ( Stat_GetVal(STAT_FOOD) > 0 ) lFood--;
	else lFood++;

	CScriptTriggerArgs Args(lFood);	// ARGN1 - new food level
	if ( OnTrigger(CTRIG_Hunger, this, &Args) == TRIGRET_RET_TRUE )
		return;
	lFood = Args.m_iN1;

	Stat_SetVal(STAT_FOOD, lFood);

	int  nFoodLevel = Food_GetLevelPercent();
	if ( nFoodLevel < 40 )	// start looking for food at 40%
 	{
		// Tell everyone we look hungry.
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_HUNGER), Food_GetLevelMessage(false, false));
		NPC_OnTickFood(nFoodLevel);
	}
}

bool CChar::OnTick()
{
#ifdef DEBUGTIMING
	TIME_PROFILE_INIT;
	TIME_PROFILE_START;
#endif
	// Assume this is only called 1 time per sec.
	// Get a timer tick when our timer expires.
	// RETURN: false = delete this.
	EXC_TRY("Tick");

	int iTimeDiff = - g_World.GetTimeDiff(m_timeLastRegen);
	if ( !iTimeDiff )
		return true;

	if ( iTimeDiff >= TICK_PER_SEC )	// don't bother with < 1 sec times.
	{
		// decay equipped items (spells)
		CItem* pItemNext = NULL;
		CItem* pItem = GetContentHead();

		for ( ; pItem != NULL; pItem = pItemNext )
		{
			EXC_TRYSUB("Ticking items");
			pItemNext = pItem->GetNext();

			// always check the validity of the memory objects
			if ( pItem->IsType(IT_EQ_MEMORY_OBJ) && !pItem->m_uidLink.ObjFind() )
			{
				pItem->Delete();
				continue;
			}

			if ( !pItem->IsTimerSet() || !pItem->IsTimerExpired() )
				continue;
			else if ( !OnTickEquip(pItem) )
				pItem->Delete();
			EXC_CATCHSUB("Char");
		}

		EXC_SET("timed functions");
		for ( int tf = m_timedFunctions.size()-1; tf >= 0; tf-- )
		{
			TimedFunction *t = &m_timedFunctions.at(tf);
			t->elapsed--;
			if ( t->elapsed < 0 )
			{
				CGString sVal;
				r_Call(t->funcname, this, NULL, &sVal);
				vector<TimedFunction>::iterator it = m_timedFunctions.begin();
				it += tf;
				m_timedFunctions.erase(it);
				break;
			}
		}

		EXC_SET("last attackers");
		for ( int la = m_lastAttackers.size()-1; la >= 0; la-- )
		{
			LastAttackers *l = &m_lastAttackers.at(la);
			l->elapsed++;
			if ( l->elapsed > 300 )	// 300 sec = 5 minutes to remember
			{
				vector<LastAttackers>::iterator it = m_lastAttackers.begin();
				it += la;
				m_lastAttackers.erase(it);
				break;
			}
		}

		if ( m_pPlayer )
		{
			EXC_SET("quests");
			vector<Quest *>::iterator it;
			for ( it = m_pPlayer->m_quests.begin(); it != m_pPlayer->m_quests.end(); it++ )
			{
				(*it)->OnTick();
			}

			//	clear "always run" flag
			if ( IsClient() )
			{
				if ( -g_World.GetTimeDiff(GetClient()->m_timeLastEvent) > TICK_PER_SEC )
					StatFlag_Clear(STATF_Fly);
			}
		}

		//	if i am death, i do not regen anything
		if (( Stat_GetVal(STAT_STR) <= 0 ) || IsStatFlag(STATF_DEAD) )
		{
			EXC_SET("death");
			m_timeLastRegen = CServTime::GetCurrentTime();
			return IsStatFlag(STATF_DEAD) ? true : Death();
		}

		for ( STAT_TYPE i = STAT_STR; i <= STAT_FOOD; i = (STAT_TYPE)(i+1) )
		{
			EXC_SET(g_Stat_Name[i]);
			m_Stat[i].m_regen += iTimeDiff;

			int iRate = g_Cfg.m_iRegenRate[i];		// in TICK_PER_SEC

			//	no food - slow regen
			if (( i != STAT_FOOD ) && Stat_GetMax(STAT_FOOD) && !Stat_GetVal(STAT_FOOD) )
				iRate *= 2;

			int mod=1;
			if (i < STAT_FOOD)
			{
				char sRegen[21];
				sprintf(sRegen, "OVERRIDE.REGEN_%d", (int)i);
				iRate -= ( GetKeyNum(sRegen, true) * 10 );
				sprintf(sRegen, "OVERRIDE.REGENVAL_%d", (int)i);
				mod += GetKeyNum(sRegen, true);
			}

			//	metabolism bonus
			if ( i == STAT_STR )
				iRate += iRate / (1 + (Stat_GetVal(STAT_DEX)/8));

			if ( m_Stat[i].m_regen < iRate )
				continue;

			m_Stat[i].m_regen = 0;
			if ( i == STAT_FOOD )
				OnTickFood();
			else if ( Stat_GetVal(i) != Stat_GetMax(i))
				UpdateStatVal(i, mod);
		}
	}
	else
	{
		// Check this all the time.
		if ( Stat_GetVal(STAT_STR) <= 0 )
		{
			EXC_SET("death");
			return Death();
		}
	}

	if ( IsStatFlag(STATF_DEAD) )
		return true;
	if ( IsDisconnected() )		// mounted horses can still get a tick.
	{
		m_timeLastRegen = CServTime::GetCurrentTime();
		return true;
	}

	if ( IsTimerExpired() && IsTimerSet())
	{
		EXC_SET("timer expired");
		// My turn to do some action.
		switch ( Skill_Done())
		{
			case -SKTRIG_ABORT:	EXC_SET("skill abort"); Skill_Fail(true); break;	// fail with no message or credit.
			case -SKTRIG_FAIL:	EXC_SET("skill fail"); Skill_Fail(false); break;
			case -SKTRIG_QTY:	EXC_SET("skill cleanup"); Skill_Cleanup(); break;
		}

		if ( m_pNPC )		// What to do next ?
		{
			EXC_SET("NPC action");
			NPC_OnTickAction();
		}
	}
	else
	{
		// Hit my current target. (if i'm ready)
		EXC_SET("combat hit try");
		if ( IsStatFlag(STATF_War) )
		{
			if ( Fight_IsActive() )
			{
				if ( m_atFight.m_War_Swing_State == WAR_SWING_READY )
					Fight_HitTry();
			}
			else if ( Skill_GetActive() == SKILL_NONE )
				Fight_AttackNext();
		}
	}

	if ( iTimeDiff >= TICK_PER_SEC )
	{
		// Check location periodically for standing in fire fields, traps, etc.
		CheckLocation(true);
		m_timeLastRegen = CServTime::GetCurrentTime();
	}

	if ( IsClient() )
	{
		EXC_SET("update stats");
		GetClient()->UpdateStats();
	}

	iTimeDiff = - g_World.GetTimeDiff(m_timeLastHitsUpdate);
	if ( g_Cfg.m_iHitsUpdateRate && ( iTimeDiff >= g_Cfg.m_iHitsUpdateRate ) ) // update hits for all
	{
		if ( m_fHitsUpdate )
		{
			EXC_SET("update hits to others");
			UpdateHitsForOthers();
			m_fHitsUpdate = false;
		}
		m_timeLastHitsUpdate = CServTime::GetCurrentTime();
	}
	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.Debug("'%s' npc '%d' player '%d' [0%lx]\n",
		GetName(), (int)(m_pNPC ? m_pNPC->m_Brain : 0), (int)(m_pPlayer != 0), (int)IsClient(), (DWORD)GetUID());
	EXC_DEBUG_END;
#ifdef DEBUGTIMING
	TIME_PROFILE_END;
	g_Log.Debug("CChar::OnTick(%x) took %d.%d to run\n", (DWORD)GetUID(), TIME_PROFILE_GET_HI, TIME_PROFILE_GET_LO);
#endif
	return true;
}
