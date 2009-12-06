//
// CClientUse.cpp
// Copyright Menace Software (www.menasoft.com).
//
#include "graysvr.h"	// predef header.
#include "CClient.h"
#include "../network/send.h"

inline bool CClient::Cmd_Use_Item_MustEquip( CItem * pItem )
{
	ADDTOCALLSTACK("CClient::Cmd_Use_Item_MustEquip");
	if ( ! m_pChar->CanMove( pItem ) || ! m_pChar->ItemEquip( pItem ))
	{
		// SysMessage( "The item should be equipped to use." );
		return false;
	}
	return true;
}

bool CClient::Cmd_Use_Item( CItem * pItem, bool fTestTouch, bool fScript )
{
	ADDTOCALLSTACK("CClient::Cmd_Use_Item");
	// Assume we can see the target.
	// called from Event_DoubleClick

	if ( pItem == NULL )
		return false;

	if ( fTestTouch )
	{
		if( !fScript ) 
		{
			CItemContainer * container = (dynamic_cast <CItemContainer*>(pItem->GetParent()));
			if( container != NULL ) 
			{
				// protect from ,snoop - disallow picking from not opened containers
				bool isInOpenedContainer = false;
				CClient::OpenedContainerMap_t::iterator itContainerFound = m_openedContainers.find( container->GetUID().GetPrivateUID() );
				if ( itContainerFound != m_openedContainers.end() )
				{
					DWORD dwTopContainerUID = (((*itContainerFound).second).first).first;
					DWORD dwTopMostContainerUID = (((*itContainerFound).second).first).second;
					CPointMap ptOpenedContainerPosition = ((*itContainerFound).second).second;
					const CObjBaseTemplate * pObjTop = pItem->GetTopLevelObj();
					
					DWORD dwTopContainerUID_ToCheck = 0;
					if ( container->GetContainer() )
						dwTopContainerUID_ToCheck = container->GetContainer()->GetUID().GetPrivateUID();
					else
						dwTopContainerUID_ToCheck = pObjTop->GetUID().GetPrivateUID();

					if ( ( dwTopMostContainerUID == pObjTop->GetUID().GetPrivateUID() ) && ( dwTopContainerUID == dwTopContainerUID_ToCheck ) )
					{
						if ( pObjTop->IsChar() )
						{
							isInOpenedContainer = true;
							// probably a pickup check from pack if pCharTop != this?
						}
						else
						{
							CItem * pItemTop = dynamic_cast<CItem *>(const_cast<CObjBaseTemplate *>(pObjTop));
							if ( pItemTop && (pItemTop->IsType(IT_SHIP_HOLD) || pItemTop->IsType(IT_SHIP_HOLD_LOCK)) && (pItemTop->GetTopPoint().GetRegion(REGION_TYPE_MULTI) == m_pChar->GetTopPoint().GetRegion(REGION_TYPE_MULTI)) )
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
				{
					SysMessageDefault(DEFMSG_REACH_UNABLE);
					return false;
				}
			}
		}

		// CanTouch handles priv level compares for chars
		if ( ! m_pChar->CanUse( pItem, false ))
		{
			if ( ! m_pChar->CanTouch( pItem ))
			{
				SysMessage(( m_pChar->IsStatFlag( STATF_DEAD )) ?
					g_Cfg.GetDefaultMsg( DEFMSG_REACH_GHOST ) :
					g_Cfg.GetDefaultMsg( DEFMSG_REACH_FAIL ) );
			}
			else
			{
				SysMessageDefault( DEFMSG_REACH_UNABLE );
			}
			
			return false;
		}
	}

	CItemBase * pItemDef = pItem->Item_GetDef();

	if ( pItem->OnTrigger( ITRIG_DCLICK, m_pChar ) == TRIGRET_RET_TRUE )
	{
		return true;
	}

	if ( pItemDef->IsTypeEquippable() && ! pItem->IsItemEquipped() && pItemDef->GetEquipLayer() )
	{
		if ( pItem->IsType(IT_LIGHT_OUT) && pItem->IsItemInContainer())
		{
			if ( ! Cmd_Use_Item_MustEquip( pItem ) )
				return false;
		}
		else if ( ! pItem->IsType(IT_LIGHT_OUT) && ! pItem->IsType(IT_LIGHT_LIT) &&
					! pItem->IsTypeSpellbook() )
		{
			if ( ! Cmd_Use_Item_MustEquip( pItem ) )
				return false;
		}
	}

	SetTargMode();
	m_Targ_UID = pItem->GetUID();	// probably already set anyhow.
	m_tmUseItem.m_pParent = pItem->GetParent();	// Cheat Verify.
	// Use types of items. (specific to client)
	switch ( pItem->GetType() )
	{
		case IT_TRACKER:
			{
				DIR_TYPE dir = (DIR_TYPE) ( DIR_QTY + 1 );	// invalid value.
				if ( ! m_pChar->Skill_Tracking( pItem->m_uidLink, dir ))
				{
					if ( pItem->m_uidLink.IsValidUID())
					{
						SysMessageDefault( DEFMSG_TRACKING_UNABLE );
					}
					m_Targ_UID = pItem->GetUID();
					addTarget( CLIMODE_TARG_LINK, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_TRACKER_ATTUNE ) );
				}
			}
			return true;

		case IT_TRACK_ITEM:		// 109 - track a id or type of item.
		case IT_TRACK_CHAR:		// 110 = track a char or range of char id's
			// Track a type of item or creature.
			{
				// Look in the local area for this item or char.


			}
			break;

		case IT_SHAFT:
		case IT_FEATHER:
			{
				CScriptTriggerArgs args("sm_bolts");
				if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE ) return true;
				return Cmd_Skill_Menu( g_Cfg.ResourceGetIDType( RES_SKILLMENU, "sm_bolts" ) );
			}
		case IT_FISH_POLE:	// Just be near water ?
			m_pChar->m_atResource.m_ridType	= RESOURCE_ID( RES_TYPEDEF, IT_WATER );
			addTarget( CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg( DEFMSG_FISHING_PROMT ), true );
			return true;
		case IT_DEED:
			addTargetDeed( pItem );
			return true;

		case IT_EQ_BANK_BOX:
		case IT_EQ_VENDOR_BOX:
			if ( !fScript )
			g_Log.Event( LOGL_WARN|LOGM_CHEAT,
				"%x:Cheater '%s' is using 3rd party tools to open bank box\n",
				GetSocketID(), (LPCTSTR) GetAccount()->GetName());
			return false;

		case IT_CONTAINER_LOCKED:
		case IT_SHIP_HOLD_LOCK:
			if ( ! m_pChar->GetPackSafe()->ContentFindKeyFor( pItem ))
			{
				SysMessageDefault( DEFMSG_ITEMUSE_LOCKED );
				if ( ! IsPriv( PRIV_GM ))
					return false;
			}

		case IT_CORPSE:
		case IT_SHIP_HOLD:
		case IT_CONTAINER:
		case IT_TRASH_CAN:
			{
				CItemContainer * pPack = dynamic_cast <CItemContainer *>(pItem);
				if ( ! m_pChar->Skill_Snoop_Check( pPack ))
				{
					if( !addContainerSetup(pPack) ) {
						return false;
					}
				}
			}
			return true;

		case IT_GAME_BOARD:
			if ( ! pItem->IsTopLevel())
			{
				SysMessageDefault( DEFMSG_ITEMUSE_GAMEBOARD_FAIL );
				return false;
			}
			{
				CItemContainer* pBoard = dynamic_cast <CItemContainer *>(pItem);
				ASSERT(pBoard);
				pBoard->Game_Create();
				addContainerSetup( pBoard );
			}
			return true;

		case IT_BBOARD:
			addBulletinBoard( dynamic_cast<CItemContainer *>(pItem));
			return true;

		case IT_SIGN_GUMP:
			// Things like grave stones and sign plaques.
			// Need custom gumps.
			{
			GUMP_TYPE gumpid = pItemDef->m_ttContainer.m_gumpid;
			if ( ! gumpid )
			{
				return false;
			}
			addGumpTextDisp( pItem, gumpid,	pItem->GetName(),
				( pItem->IsIndividualName()) ? pItem->GetName() : NULL );
			}
			return true;

		case IT_BOOK:
		case IT_MESSAGE:
			if ( ! addBookOpen( pItem ))
			{
				SysMessageDefault( DEFMSG_ITEMUSE_BOOK_FAIL );
			}
			return true;

		case IT_STONE_GUILD:
		case IT_STONE_TOWN:
			// Guild and town stones.
			{
				CItemStone * pStone = dynamic_cast<CItemStone*>(pItem);
				if ( pStone == NULL )
					break;
				pStone->Use_Item( this );
			}
			return true;

		case IT_POTION:
			if ( ! m_pChar->CanMove( pItem ))	// locked down decoration.
			{
				SysMessageDefault( DEFMSG_ITEMUSE_POTION_FAIL );
				return false;
			}
			if ( RES_GET_INDEX(pItem->m_itPotion.m_Type) == SPELL_Poison )
			{
				// ask them what they want to use the poison on ?
				// Poisoning or poison self ?
				addTarget( CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg( DEFMSG_TARGET_PROMT ), false, true );
				return true;
			}
			else if ( RES_GET_INDEX(pItem->m_itPotion.m_Type) == SPELL_Explosion )
			{
				// Throw explode potion.
				if ( ! m_pChar->ItemPickup( pItem, 1 ))
					return true;
				m_tmUseItem.m_pParent = pItem->GetParent();	// Cheat Verify FIX.

				addTarget( CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg( DEFMSG_SELECT_POTION_TARGET ), true, true );
				// Put the potion in our hand as well. (and set it's timer)
				pItem->m_itPotion.m_tick = 4;	// countdown to explode purple.
				pItem->m_itPotion.m_ignited = 1; // ignite it
				pItem->SetTimeout( TICK_PER_SEC );
				pItem->m_uidLink = m_pChar->GetUID();
				return true;
			}
			m_pChar->Use_Drink(pItem);
			return true;

		case IT_ANIM_ACTIVE:
			SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_ITEM_IN_USE ) );
			return false;

		case IT_CLOCK:
			addObjMessage( m_pChar->GetTopSector()->GetLocalGameTime(), pItem );
			return true;

		case IT_SPAWN_CHAR:
			{
				bool	fReset	= false;
			if ( pItem->m_itSpawnChar.m_current )
				SysMessageDefault( DEFMSG_ITEMUSE_SPAWNCHAR_NEG );
			else
			{
				SysMessageDefault( DEFMSG_ITEMUSE_SPAWNCHAR_RSET );
				fReset	= true;
			}
			pItem->Spawn_KillChildren();
			if ( fReset )
				pItem->SetTimeout( TICK_PER_SEC );
			}
			return true;

		case IT_SPAWN_ITEM:
			SysMessageDefault( DEFMSG_ITEMUSE_SPAWNITEM_TRIG );
			pItem->Spawn_OnTick( true );
			return true;

		case IT_SHRINE:
			if ( m_pChar->OnSpellEffect( SPELL_Resurrection, m_pChar, 1000, pItem ))
				return true;
			SysMessageDefault( DEFMSG_ITEMUSE_SHRINE );
			return true;

		case IT_SHIP_TILLER:
			// dclick on tiller man.
			pItem->Speak( g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_TILLERMAN ), HUE_TEXT_ITEM, TALKMODE_SAY, FONT_NORMAL );
			return true;

			// A menu or such other action ( not instantanious)

		case IT_WAND:
		case IT_SCROLL:	// activate the scroll.
			{
				SPELL_TYPE spell = (SPELL_TYPE) RES_GET_INDEX(pItem->m_itWeapon.m_spell);
				const CSpellDef* pSpellDef = g_Cfg.GetSpellDef(spell);
				if (pSpellDef == NULL)
					return false;

				if ( IsSetMagicFlags( MAGICF_PRECAST ) && !pSpellDef->IsSpellType( SPELLFLAG_NOPRECAST ) )
				{
					int skill;
					if (!pSpellDef->GetPrimarySkill(&skill, NULL))
						return false;

					m_tmSkillMagery.m_Spell = spell;	// m_atMagery.m_Spell
					m_pChar->m_atMagery.m_Spell = spell;
					m_Targ_UID = pItem->GetUID();	// default target.
					m_Targ_PrvUID = pItem->GetUID();
					m_pChar->Skill_Start( (SKILL_TYPE)skill );
					return true;
				}
				return Cmd_Skill_Magery( spell, pItem );
			}
		case IT_RUNE:
			// name the rune.
			if ( ! m_pChar->CanMove( pItem, true ))
			{
				return false;
			}
			addPromptConsole( CLIMODE_PROMPT_NAME_RUNE, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_RUNE_NAME ), pItem->GetUID() );
			return true;

		case IT_CARPENTRY:
			// Carpentry type tool
			{
				CScriptTriggerArgs args("sm_carpentry");
				if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE ) return true;
				return Cmd_Skill_Menu( g_Cfg.ResourceGetIDType( RES_SKILLMENU, "sm_carpentry" ) );
			}
			// Solve for the combination of this item with another.
		case IT_FORGE:
			addTarget( CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg ( DEFMSG_ITEMUSE_FORGE ) );
			return true;
		case IT_ORE:
			// just use the nearest forge.
			return m_pChar->Skill_Mining_Smelt( pItem, NULL );
		case IT_INGOT:
			return Cmd_Skill_Smith( pItem );

		case IT_KEY:
		case IT_KEYRING:
			if ( pItem->GetTopLevelObj() != m_pChar && ! m_pChar->IsPriv(PRIV_GM))
			{
				SysMessageDefault( DEFMSG_ITEMUSE_KEY_FAIL );
				return false;
			}
			addTarget( CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_KEY_PROMT ), false, true );
			return true;

		case IT_BANDAGE:		// SKILL_HEALING, or SKILL_VETERINARY
			addTarget( CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_BANDAGE_PROMT ), false, false );
			return true;
		case IT_WEAPON_MACE_SHARP:	// war axe can be used to cut/chop trees.
		case IT_WEAPON_SWORD:		// 23 =
		case IT_WEAPON_FENCE:		// 24 = can't be used to chop trees.
		case IT_WEAPON_AXE:
			// set resource to trees
			m_pChar->m_atResource.m_ridType	= RESOURCE_ID( RES_TYPEDEF, IT_TREE );

		case IT_BANDAGE_BLOOD:	// Clean the bandages.
		case IT_COTTON:			// use on a spinning wheel.
		case IT_WOOL:			// use on a spinning wheel.
		case IT_YARN:			// Use this on a loom.
		case IT_THREAD: 		// Use this on a loom.
		case IT_COMM_CRYSTAL:
		case IT_SCISSORS:
		case IT_LOCKPICK:		// Use on a locked thing.
		case IT_CARPENTRY_CHOP:
		case IT_WEAPON_MACE_STAFF:
		case IT_WEAPON_MACE_SMITH:	// Can be used for smithing ?
			addTarget( CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_WEAPON_PROMT ), false, true );
			return true;

		case IT_MEAT_RAW:
		case IT_FOOD_RAW:
			addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_FOODRAW_PROMT));
			return true;
		case IT_FISH:
			SysMessageDefault( DEFMSG_ITEMUSE_FISH_FAIL );
			return true;
		case IT_TELESCOPE:
			// Big telescope.
			SysMessageDefault( DEFMSG_ITEMUSE_TELESCOPE );
			return true;
		case IT_MAP:
		case IT_MAP_BLANK:
			if ( ! pItem->m_itMap.m_right && ! pItem->m_itMap.m_bottom )
			{
				CScriptTriggerArgs args("sm_cartography");
				if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE ) return true;
				return Cmd_Skill_Menu( g_Cfg.ResourceGetIDType( RES_SKILLMENU, "sm_cartography" ) );
			}
			else if ( ! IsPriv(PRIV_GM) && pItem->GetTopLevelObj() != m_pChar )	// must be on your person.
			{
				SysMessageDefault( DEFMSG_ITEMUSE_MAP_FAIL );
			}
			else
			{
				addMap( dynamic_cast <CItemMap*>( pItem ));
			}
			return true;

		case IT_CANNON_BALL:
			{
				TCHAR *pszTemp = Str_GetTemp();
				sprintf(pszTemp, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_CBALL_PROMT ), (LPCTSTR) pItem->GetName());
				addTarget(CLIMODE_TARG_USE_ITEM, pszTemp);
			}
			return true;

		case IT_CANNON_MUZZLE:
			// Make sure the cannon is loaded.
			if ( ! m_pChar->CanUse( pItem, false ))
				return( false );
			if ( ! ( pItem->m_itCannon.m_Load & 1 ))
			{
				addTarget( CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_CANNON_POWDER ) );
				return true;
			}
			if ( ! ( pItem->m_itCannon.m_Load & 2 ))
			{
				addTarget( CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_CANNON_SHOT ) );
				return true;
			}
			addTarget( CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_CANNON_TARG ), false, true );
			return true;

		case IT_CRYSTAL_BALL:
			// Gaze into the crystal ball.

			return true;

		case IT_WEAPON_MACE_CROOK:
			addTarget( CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_CROOK_PROMT ), false, true );
			return true;

		case IT_SEED:
		case IT_PITCHER_EMPTY:
			{ // not a crime.
				TCHAR *pszTemp = Str_GetTemp();
				sprintf(pszTemp, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_PITCHER_TARG ), (LPCTSTR) pItem->GetName());
				addTarget(CLIMODE_TARG_USE_ITEM, pszTemp, true);
			}
			return true;

		case IT_WEAPON_MACE_PICK:
			{	// Mine at the location. (possible crime?)
				TCHAR *pszTemp = Str_GetTemp();
				sprintf(pszTemp, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_MACEPICK_TARG ), (LPCTSTR) pItem->GetName());
				m_pChar->m_atResource.m_ridType	= RESOURCE_ID(RES_TYPEDEF, IT_ROCK);
				addTarget(CLIMODE_TARG_USE_ITEM, pszTemp, true, true);
			}
			return true;

		case IT_SPELLBOOK:
			addSpellbookOpen( pItem );
			return true;

		case IT_SPELLBOOK_NECRO:
			addSpellbookOpen( pItem, 101 );
			return true;

		case IT_SPELLBOOK_PALA:
			addSpellbookOpen( pItem, 201 );
			return true;

		case IT_SPELLBOOK_BUSHIDO:
			addSpellbookOpen( pItem, 401 );
			return true;

		case IT_SPELLBOOK_NINJITSU:
			addSpellbookOpen( pItem, 501 );
			return true;

		case IT_SPELLBOOK_ARCANIST:
			addSpellbookOpen( pItem, 601 );
			return true;

		case IT_HAIR_DYE:
			if (!m_pChar->LayerFind( LAYER_BEARD ) && !m_pChar->LayerFind( LAYER_HAIR ))
			{
				SysMessageDefault( DEFMSG_ITEMUSE_DYE_NOHAIR );
				return true;
			}
			Dialog_Setup(CLIMODE_DIALOG, g_Cfg.ResourceGetIDType( RES_DIALOG, "d_HAIR_DYE" ), 0, pItem );
			return true;
		case IT_DYE:
			addTarget( CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_DYE_VAT ) );
			return true;
		case IT_DYE_VAT:
			addTarget( CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_DYE_TARG ), false, true );
			return true;

		case IT_MORTAR:
			// If we have a mortar then do alchemy.
			addTarget( CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_MORTAR_PROMT ) );
			return true;
		case IT_POTION_EMPTY:
			if ( ! m_pChar->ContentFind(RESOURCE_ID(RES_TYPEDEF,IT_MORTAR)))
			{
				SysMessageDefault( DEFMSG_ITEMUSE_NO_MORTAR );
				return( false );
			}
			addTarget( CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_MORTAR_PROMT ) );
			return true;
		case IT_REAGENT:
			// Make a potion with this. (The best type we can)
			if ( ! m_pChar->ContentFind( RESOURCE_ID(RES_TYPEDEF,IT_MORTAR)))
			{
				SysMessageDefault( DEFMSG_ITEMUSE_NO_MORTAR );
				return false;
			}
			Cmd_Skill_Alchemy( pItem );
			return true;

			// Put up menus to better decide what to do.

		case IT_TINKER_TOOLS:	// Tinker tools.
			{
				CScriptTriggerArgs args("sm_tinker");
				if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE ) return true;
				return Cmd_Skill_Menu( g_Cfg.ResourceGetIDType( RES_SKILLMENU, "sm_tinker" ) );
			}
		case IT_SEWING_KIT:	// IT_SEWING_KIT Sew with materials we have on hand.
			{
				TCHAR *pszTemp = Str_GetTemp();
				sprintf(pszTemp, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_SEWKIT_PROMT ), (LPCTSTR) pItem->GetName());
				addTarget(CLIMODE_TARG_USE_ITEM, pszTemp);
			}
			return true;

		case IT_SCROLL_BLANK:
			Cmd_Skill_Inscription();
			return true;

		default:
			// An NPC could use it this way.
			if ( m_pChar->Use_Item( pItem ))
				return( true );
			break;
	}

	SysMessageDefault( DEFMSG_ITEMUSE_CANTTHINK );
	return( false );
}

void CClient::Cmd_EditItem( CObjBase * pObj, int iSelect )
{
	ADDTOCALLSTACK("CClient::Cmd_EditItem");
	// ARGS:
	//   iSelect == -1 = setup.
	//   m_Targ_Text = what are we doing to it ?
	//
	if ( pObj == NULL )
		return;

	CContainer * pContainer = dynamic_cast <CContainer *> (pObj);
	if ( pContainer == NULL )
	{
		addGumpDialogProps( pObj->GetUID());
		return;
	}

	if ( iSelect == 0 )
		return;	// cancelled.

	if ( iSelect > 0 )
	{
		// We selected an item.
		if ( iSelect >= COUNTOF(m_tmMenu.m_Item))
			return;

		if ( m_Targ_Text.IsEmpty())
		{
			addGumpDialogProps( m_tmMenu.m_Item[iSelect] );
		}
		else
		{
			OnTarg_Obj_Set( CGrayUID( m_tmMenu.m_Item[iSelect] ).ObjFind() );
		}
		return;
	}

	CMenuItem item[MAX_MENU_ITEMS];	// Most as we want to display at one time.
	item[0].m_sText.Format( "Contents of %s", (LPCTSTR) pObj->GetName());

	int count = 0;
	CItem * pItemNext;
	for ( CItem * pItem = pContainer->GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		if ( ++count >= MAX_MENU_ITEMS )
			break;

		pItemNext = pItem->GetNext();
		m_tmMenu.m_Item[count] = pItem->GetUID();
		item[count].m_sText = pItem->GetName();
		ITEMID_TYPE idi = pItem->GetDispID();
		item[count].m_id = idi;
		item[count].m_color = 0;

		if ( !pItem->IsType( IT_EQ_MEMORY_OBJ ) )
		{
			HUE_TYPE wHue = pItem->GetHue();
			if ( wHue != 0 )
			{
				wHue = (wHue == 1? 0x7FF : wHue-1 );
				item[count].m_color = wHue;
			}
		}
	}

	addItemMenu( CLIMODE_MENU_EDIT, item, count, pObj );
}

bool CClient::Cmd_CreateItem( ITEMID_TYPE id, bool fStatic )
{
	ADDTOCALLSTACK("CClient::Cmd_CreateItem");
	// make an item near by.
	m_tmAdd.m_id = id;
	m_tmAdd.m_fStatic = fStatic;
	return addTargetItems( CLIMODE_TARG_ADDITEM, m_tmAdd.m_id );
}

bool CClient::Cmd_CreateChar( CREID_TYPE id, SPELL_TYPE iSpell, bool fPet )
{
	ADDTOCALLSTACK("CClient::Cmd_CreateChar");
	// make a creature near by. (GM or script used only)
	// "ADDNPC"
	// spell = SPELL_Summon

	ASSERT(m_pChar);
	m_tmSkillMagery.m_Spell = iSpell;	// m_atMagery.m_Spell
	m_tmSkillMagery.m_SummonID = id;				// m_atMagery.m_SummonID
	m_tmSkillMagery.m_fSummonPet = fPet;
	if ( ! m_Targ_PrvUID.IsValidUID())
	{
		m_Targ_PrvUID = m_pChar->GetUID();	// what id this was already a scroll.
	}

	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef( iSpell );
	ASSERT( pSpellDef );

	return addTargetChars( CLIMODE_TARG_SKILL_MAGERY, id, pSpellDef->IsSpellType( SPELLFLAG_HARM ), g_Cfg.m_iSpellTimeout * TICK_PER_SEC);
}

bool CClient::Cmd_Skill_Menu( RESOURCE_ID_BASE rid, int iSelect )
{
	ADDTOCALLSTACK("CClient::Cmd_Skill_Menu");
	// Build the skill menu for the curent active skill.
	// Only list the things we have skill and ingrediants to make.
	//
	// ARGS:
	//	m_Targ_UID = the object used to get us here.
	//  rid = which menu ?
	//	iSelect = -2 = Just a test of the whole menu.,
	//	iSelect = -1 = 1st setup.
	//	iSelect = 0 = cancel
	//	iSelect = x = execute the selection.
	//
	// RETURN: false = fail/cancel the skill.
	//   m_tmMenu.m_Item = the menu entries.

	ASSERT(m_pChar);
	if ( rid.GetResType() != RES_SKILLMENU )
		return( false );

	bool fShowMenu = false;
	bool fLimitReached = false;

	if ( iSelect == 0 )
	{
		// menu cancelled
		return( Cmd_Skill_Menu_Build( rid, iSelect, NULL, 0, fShowMenu, fLimitReached ));
	}

	CMenuItem item[ minimum( COUNTOF( m_tmMenu.m_Item ), MAX_MENU_ITEMS ) ];
	int iShowCount = Cmd_Skill_Menu_Build( rid, iSelect, item, COUNTOF(item), fShowMenu, fLimitReached);

	if ( iSelect < -1 )		// just a test.
	{
		return( iShowCount ? true : false );
	}

	if ( iSelect > 0 )	// seems our resources disappeared.
	{
		if ( ! iShowCount )
		{
			SysMessageDefault( DEFMSG_CANT_MAKE );
		}
		return( iShowCount ? true : false );
	}

	if ( ! iShowCount )
	{
		SysMessageDefault( DEFMSG_CANT_MAKE_RES );
		return( false );
	}

	if ( iShowCount == 1 && fShowMenu == true && fLimitReached == false )
	{
		static int sm_iReentrant = 0;
		if ( sm_iReentrant < 12 )
		{
			sm_iReentrant++;

			// If there is just one menu then select it.
			bool fSuccess = Cmd_Skill_Menu( rid, m_tmMenu.m_Item[1] );

			sm_iReentrant--;
			return( fSuccess );
		}

		if ( g_Cfg.m_wDebugFlags & DEBUGF_SCRIPTS )
			g_Log.EventDebug("SCRIPT: Too many empty skill menus to continue seeking through menu '%s'\n", g_Cfg.ResourceGetDef(rid)->GetResourceName());
	}

	addItemMenu( CLIMODE_MENU_SKILL, item, iShowCount );
	return( true );
}

int CClient::Cmd_Skill_Menu_Build( RESOURCE_ID_BASE rid, int iSelect, CMenuItem* item, int iMaxSize, bool &fShowMenu, bool &fLimitReached )
{
	ADDTOCALLSTACK("CClient::Cmd_Skill_Menu_Build");
	// Build the skill menu for the curent active skill.
	// Only list the things we have skill and ingrediants to make.
	//
	// ARGS:
	//	m_Targ_UID = the object used to get us here.
	//  rid = which menu ?
	//	iSelect = -2 = Just a test of the whole menu.,
	//	iSelect = -1 = 1st setup.
	//	iSelect = 0 = cancel
	//	iSelect = x = execute the selection.
	//  fShowMenu = whether or not menus can be shown
	//  item = pointer to entries list
	//  iMaxSize = maximum number of entries
	//
	// RETURN: number of entries in menu
	//   m_tmMenu.m_Item = the menu entries.

	ASSERT(m_pChar);
	if ( rid.GetResType() != RES_SKILLMENU )
		return( 0 );

	// Find section.
	CResourceLock s;
	if ( ! g_Cfg.ResourceLock( s, rid ))
	{
		return 0;
	}

	// Get title line
	if ( ! s.ReadKey())
		return( 0 );

	if ( iSelect == 0 )	// cancelled
	{
		while ( s.ReadKeyParse())
		{
			if ( !s.IsKey( "ON" ) || ( *s.GetArgStr() != '@' ) )
				continue;

			if ( strcmpi( s.GetArgStr(), "@cancel" ) )
				continue;

			TRIGRET_TYPE tRet = m_pChar->OnTriggerRunVal( s, TRIGRUN_SECTION_TRUE, m_pChar, NULL );
			if ( tRet == TRIGRET_RET_TRUE )
				return 0;

			break;
		}
		return 1;
	}

	if ( iSelect < 0 )
	{
		item[0].m_sText = s.GetKey();
		if ( iSelect == -1 )
		{
			m_tmMenu.m_ResourceID = rid;
		}
	}

	bool fSkip = false;	// skip this if we lack resources or skill.
	int iOnCount = 0;
	int iShowCount = 0;
	CScriptTriggerArgs Args;

	while ( s.ReadKeyParse())
	{
		if ( s.IsKeyHead( "ON", 2 ))
		{
			if ( *s.GetArgStr() == '@' )
			{
				fSkip = true;
				continue;
			}

			// a new option to look at.
			fSkip = false;
			iOnCount ++;

			if ( iSelect < 0 )	// building up the list.
			{
				if ( iSelect < -1 && iShowCount >= 1 )		// just a test. so we are done.
				{
					return( 1 );
				}
				iShowCount ++;
				if ( ! item[iSelect == -2? 0 : iShowCount].ParseLine( s.GetArgRaw(), NULL, m_pChar ))
				{
					// remove if the item is invalid.
					iShowCount --;
					fSkip = true;
					continue;
				}
				if ( iSelect == -1 )
				{
					m_tmMenu.m_Item[iShowCount] = iOnCount;
				}
				if ( iShowCount >= iMaxSize-1 )
					break;
			}
			else
			{
				if ( iOnCount > iSelect )	// we are done.
					break;
			}
			continue;
		}

		if ( fSkip ) // we have decided we cant do this option.
			continue;
		if ( iSelect > 0 && iOnCount != iSelect ) // only interested in the selected option
			continue;

		// Check for a skill / non-consumables required.
		if ( s.IsKey("TEST"))
		{
			m_pChar->ParseText( s.GetArgRaw(), m_pChar );
			CResourceQtyArray skills( s.GetArgStr());
			if ( ! skills.IsResourceMatchAll(m_pChar))
			{
				iShowCount--;
				fSkip = true;
			}
			continue;
		}
		if ( s.IsKey("TESTIF"))
		{
			m_pChar->ParseText( s.GetArgRaw(), m_pChar );
			if ( ! s.GetArgVal())
			{
				iShowCount--;
				fSkip = true;
			}
			continue;
		}

		// select to execute any entries here ?
		if ( iOnCount == iSelect )
		{
			// Execute command from script
			TRIGRET_TYPE tRet = m_pChar->OnTriggerRunVal( s, TRIGRUN_SINGLE_EXEC, m_pChar, &Args );

			if (tRet != TRIGRET_RET_DEFAULT)
			{
				if (tRet == TRIGRET_RET_TRUE)
					return( 0 );

				return( 1 );
			}

			iShowCount++;	// we are good. but continue til the end
		}
		else
		{
			ASSERT( iSelect < 0 );

			if ( s.IsKey("SKILLMENU"))
			{
				static int sm_iReentrant = 0;
				if (sm_iReentrant > 1024)
				{
					if ( g_Cfg.m_wDebugFlags & DEBUGF_SCRIPTS )
						g_Log.EventDebug("SCRIPT: Too many skill menus (circular menus?) to continue searching in menu '%s'\n", g_Cfg.ResourceGetDef(rid)->GetResourceName());

					fLimitReached = true;
				}
				else
				{
					// Test if there is anything in this skillmenu we can do.
					++sm_iReentrant;

					CMenuItem item;
					if ( ! Cmd_Skill_Menu_Build( g_Cfg.ResourceGetIDType( RES_SKILLMENU, s.GetArgStr()), -2, &item, iMaxSize, fShowMenu, fLimitReached ))
					{
						iShowCount--;
						fSkip = true;
					}
					else
					{
						fShowMenu = true;
					}

					--sm_iReentrant;
				}
				continue;
			}
			if ( s.IsKey("MAKEITEM"))
			{
				// test if i can make this item using m_Targ_UID.
				// There should ALWAYS be a valid id here.
				if ( ! m_pChar->Skill_MakeItem( (ITEMID_TYPE) g_Cfg.ResourceGetIndexType( RES_ITEMDEF, s.GetArgStr()),
					m_Targ_UID, SKTRIG_SELECT ))
				{
					iShowCount--;
					fSkip = true;
				}
				continue;
			}
		}
	}

	return iShowCount;
}

bool CClient::Cmd_Skill_Magery( SPELL_TYPE iSpell, CObjBase * pSrc )
{
	ADDTOCALLSTACK("CClient::Cmd_Skill_Magery");
	// start casting a spell. prompt for target.
	// pSrc = you the char.
	// pSrc = magic object is source ?
	// static const TCHAR sm_Txt_Summon[] = "Where would you like to summon the creature ?";
	ASSERT(m_pChar);

	const CSpellDef * pSpellDef;

	if ( IsSetMagicFlags( MAGICF_PRECAST ) && iSpell == m_tmSkillMagery.m_Spell )
	{
		pSpellDef = g_Cfg.GetSpellDef(m_tmSkillMagery.m_Spell);
		if (pSpellDef != NULL && !pSpellDef->IsSpellType( SPELLFLAG_NOPRECAST ))
			iSpell = m_tmSkillMagery.m_Spell;
	}
	else
		pSpellDef = g_Cfg.GetSpellDef( iSpell );

	// Do we have the regs ? etc.
	if ( ! m_pChar->Spell_CanCast( iSpell, true, pSrc, true ))
		return false;

	ASSERT(pSpellDef);

	SetTargMode();
	m_tmSkillMagery.m_Spell		= iSpell;				// m_atMagery.m_Spell
	m_Targ_UID					= m_pChar->GetUID();	// default target.
	m_Targ_PrvUID				= pSrc->GetUID();		// source of the spell.

	// cast self
	if ( !pSpellDef->IsSpellType( SPELLFLAG_TARG_OBJ | SPELLFLAG_TARG_XYZ ) )
	{
		m_pChar->m_Act_p			= m_pChar->GetTopPoint();
		m_pChar->m_atMagery.m_Spell = iSpell;
		m_pChar->m_Act_Targ			= m_Targ_UID;
		m_pChar->m_Act_TargPrv		= m_Targ_PrvUID;
		m_Targ_p					= m_pChar->GetTopPoint();

		if ( !IsSetMagicFlags( MAGICF_PRECAST ) || pSpellDef->IsSpellType( SPELLFLAG_NOPRECAST ))
		{
			int skill;
			if (!pSpellDef->GetPrimarySkill(&skill, NULL))
				return false;

			return( m_pChar->Skill_Start( (SKILL_TYPE)skill ) );
		}
		else
		{
			m_pChar->Spell_CastDone();
			return true;
		}
	}

	LPCTSTR pPrompt = g_Cfg.GetDefaultMsg( DEFMSG_SELECT_MAGIC_TARGET );
	switch ( iSpell )
	{
	case SPELL_Recall:
		// pPrompt = g_Cfg.GetDefaultMsg( "Select rune to recall from." );
		break;
	case SPELL_Blade_Spirit:
		// pPrompt = sm_Txt_Summon;
		break;
	case SPELL_Summon:
		{
			CScriptTriggerArgs args("sm_summon");
			if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE ) return true;
			return Cmd_Skill_Menu( g_Cfg.ResourceGetIDType( RES_SKILLMENU, "sm_summon" ) );
		}
	case SPELL_Mark:
		// pPrompt = "Select rune to mark.";
		break;
	case SPELL_Gate_Travel:	// gate travel
		// pPrompt = "Select rune to gate from.";
		break;
	case SPELL_Polymorph:
		// polymorph creature menu.
		{
			if ( IsPriv(PRIV_GM))
			{
				// pPrompt = "Select creature to polymorph.";
				break;
			}
			CScriptTriggerArgs args("sm_polymorph");
			if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE ) return true;
			return Cmd_Skill_Menu( g_Cfg.ResourceGetIDType( RES_SKILLMENU, "sm_polymorph" ) );
		}
	case SPELL_Resurrection:
		// pPrompt = "Select ghost to resurrect.";
		break;
	case SPELL_Vortex:
	case SPELL_Air_Elem:
	case SPELL_Daemon:
	case SPELL_Earth_Elem:
	case SPELL_Fire_Elem:
	case SPELL_Water_Elem:
		// pPrompt = sm_Txt_Summon;
		break;

		// Necro spells
	case SPELL_Summon_Undead: // Summon an undead
		// pPrompt = sm_Txt_Summon;
		break;
	case SPELL_Animate_Dead: // Corpse to zombie
		// pPrompt = "Choose a corpse";
		break;
	case SPELL_Bone_Armor: // Skeleton corpse to bone armor
		// pPrompt = "Chose a skeleton";
		break;
	}

	if ( !pSpellDef->m_sTargetPrompt.IsEmpty() )
		pPrompt	= pSpellDef->m_sTargetPrompt;
	addTarget( CLIMODE_TARG_SKILL_MAGERY, pPrompt,
		pSpellDef->IsSpellType( SPELLFLAG_TARG_XYZ ),
		pSpellDef->IsSpellType( SPELLFLAG_HARM ),
		g_Cfg.m_iSpellTimeout * TICK_PER_SEC);
	return( true );
}

bool CClient::Cmd_Skill_Tracking( int track_sel, bool fExec )
{
	ADDTOCALLSTACK("CClient::Cmd_Skill_Tracking");
	// look around for stuff.

	ASSERT(m_pChar);
	if ( track_sel < 0 )
	{
		// Initial pre-track setup.
		if ( m_pChar->m_pArea->IsFlag( REGION_FLAG_SHIP ) &&
			! m_pChar->ContentFind( RESOURCE_ID(RES_TYPEDEF,IT_SPY_GLASS)) )
		{
			SysMessageDefault( DEFMSG_TRACKING_SPYGLASS );
			return( false );
		}

		// Tacking (unlike other skills) is used during menu setup.
		m_pChar->Skill_Cleanup();	// clean up current skill.

		CMenuItem item[6];
		item[0].m_sText = g_Cfg.GetDefaultMsg( DEFMSG_SKILLMENU_TEXT_TRACKING );

		item[1].m_id = ITEMID_TRACK_HORSE;
		item[1].m_color = 0;
		item[1].m_sText = g_Cfg.GetDefaultMsg( DEFMSG_SKILLMENU_TEXT_ANIMALS );
		item[2].m_id = ITEMID_TRACK_OGRE;
		item[2].m_color = 0;
		item[2].m_sText = g_Cfg.GetDefaultMsg( DEFMSG_SKILLMENU_TEXT_MONSTERS );
		item[3].m_id = ITEMID_TRACK_MAN;
		item[3].m_color = 0;
		item[3].m_sText = g_Cfg.GetDefaultMsg( DEFMSG_SKILLMENU_TEXT_HUMANS );
		item[4].m_id = ITEMID_TRACK_MAN_NAKED;
		item[4].m_color = 0;
		item[4].m_sText = g_Cfg.GetDefaultMsg( DEFMSG_SKILLMENU_TEXT_PLAYERS );
		item[5].m_id = ITEMID_TRACK_WISP;
		item[5].m_color = 0;
		item[5].m_sText = g_Cfg.GetDefaultMsg( DEFMSG_SKILLMENU_TEXT_ANYTHING );

		m_tmMenu.m_Item[0] = 0;
		addItemMenu( CLIMODE_MENU_SKILL_TRACK_SETUP, item, 5 );
		return( true );
	}

	if ( track_sel ) // Not Cancelled
	{
		ASSERT( ((WORD)track_sel) < COUNTOF( m_tmMenu.m_Item ));
		if ( fExec )
		{
			// Tracking menu got us here. Start tracking the selected creature.
			m_pChar->SetTimeout( 1*TICK_PER_SEC );
			m_pChar->m_Act_Targ = m_tmMenu.m_Item[track_sel]; // selected UID
			m_pChar->Skill_Start( SKILL_TRACKING );
			return true;
		}

		bool fGM = IsPriv(PRIV_GM);

		static const NPCBRAIN_TYPE sm_Track_Brain[] =
		{
			NPCBRAIN_QTY,	// not used here.
			NPCBRAIN_ANIMAL,
			NPCBRAIN_MONSTER,
			NPCBRAIN_HUMAN,
			NPCBRAIN_NONE,	// players
			NPCBRAIN_QTY,	// anything.
		};

		if ( track_sel >= COUNTOF(sm_Track_Brain))
			track_sel = COUNTOF(sm_Track_Brain)-1;
		NPCBRAIN_TYPE track_type = sm_Track_Brain[ track_sel ];

		CMenuItem item[ minimum( MAX_MENU_ITEMS, COUNTOF( m_tmMenu.m_Item )) ];
		int count = 0;

		item[0].m_sText = g_Cfg.GetDefaultMsg( DEFMSG_SKILLMENU_TEXT_TRACKING );
		m_tmMenu.m_Item[0] = track_sel;

		CWorldSearch AreaChars( m_pChar->GetTopPoint(), m_pChar->Skill_GetBase(SKILL_TRACKING)/20 + 10 );
		while (true)
		{
			CChar * pChar = AreaChars.GetChar();
			if ( pChar == NULL )
				break;
			if ( m_pChar == pChar )
				continue;

			if ( GetPrivLevel() < pChar->GetPrivLevel() && pChar->IsStatFlag(STATF_Insubstantial) )
				continue;

			CCharBase * pCharDef = pChar->Char_GetDef();
			NPCBRAIN_TYPE basic_type = pChar->GetNPCBrain();
			if ( track_type != basic_type && track_type != NPCBRAIN_QTY )
			{
				if ( track_type != NPCBRAIN_NONE )		// no match.
				{
					continue;
				}

				// players
				if ( ! pChar->m_pPlayer )
					continue;
				if ( ! fGM && basic_type != NPCBRAIN_HUMAN )	// can't track polymorphed person.
					continue;
			}

			if ( ! fGM && ! pCharDef->Can( CAN_C_WALK ))	// never moves or just swims.
				continue;

			count ++;
			item[count].m_id = pCharDef->m_trackID;
			item[count].m_color = 0;
			item[count].m_sText = pChar->GetName();
			m_tmMenu.m_Item[count] = pChar->GetUID();
			if ( count >= COUNTOF( item )-1 )
				break;
		}

		if ( count )
		{
			// Some credit for trying.
			m_pChar->Skill_UseQuick( SKILL_TRACKING, 20 + Calc_GetRandVal( 30 ));
			addItemMenu( CLIMODE_MENU_SKILL_TRACK, item, count );
			return( true );
		}
		else
		{
			// Some credit for trying.
			m_pChar->Skill_UseQuick( SKILL_TRACKING, 10 + Calc_GetRandVal( 30 ));
		}
	}

	// Tracking failed or was cancelled .

	static LPCTSTR const sm_Track_FailMsg[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_TRACKING_FAIL_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_TRACKING_FAIL_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_TRACKING_FAIL_3 ),
		g_Cfg.GetDefaultMsg( DEFMSG_TRACKING_FAIL_4 ),
		g_Cfg.GetDefaultMsg( DEFMSG_TRACKING_FAIL_5 ),
		g_Cfg.GetDefaultMsg( DEFMSG_TRACKING_FAIL_6 )
	};

	SysMessage( sm_Track_FailMsg[track_sel] );
	return( false );
}

bool CClient::Cmd_Skill_Smith( CItem * pIngots )
{
	ADDTOCALLSTACK("CClient::Cmd_Skill_Smith");
	ASSERT(m_pChar);
	if ( pIngots == NULL || ! pIngots->IsType(IT_INGOT))
	{
		SysMessageDefault( DEFMSG_SMITHING_FAIL );
		return( false );
	}
	ASSERT( m_Targ_UID == pIngots->GetUID());
	if ( pIngots->GetTopLevelObj() != m_pChar )
	{
		SysMessageDefault( DEFMSG_SMITHING_REACH );
		return( false );
	}

	// must have hammer equipped.
	CItem * pHammer = m_pChar->LayerFind( LAYER_HAND1 );
	if ( pHammer == NULL || ! pHammer->IsType(IT_WEAPON_MACE_SMITH))
	{
		SysMessageDefault( DEFMSG_SMITHING_HAMMER );
		return( false );
	}

	// Select the blacksmith item type.
	// repair items or make type of items.
	if ( ! g_World.IsItemTypeNear( m_pChar->GetTopPoint(), IT_FORGE, 3, false ))
	{
		SysMessageDefault( DEFMSG_SMITHING_FORGE );
		return( false );
	}

	// Select the blacksmith item type.
	// repair items or make type of items.
	CScriptTriggerArgs args("sm_blacksmith");
	if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE ) return true;
	return Cmd_Skill_Menu( g_Cfg.ResourceGetIDType( RES_SKILLMENU, "sm_blacksmith" ) );
}

bool CClient::Cmd_Skill_Inscription()
{
	ADDTOCALLSTACK("CClient::Cmd_Skill_Inscription");
	// Select the scroll type to make.
	// iSelect = -1 = 1st setup.
	// iSelect = 0 = cancel
	// iSelect = x = execute the selection.
	// we should already be in inscription skill mode.

	ASSERT(m_pChar);

	CItem * pBlank = m_pChar->ContentFind( RESOURCE_ID(RES_TYPEDEF,IT_SCROLL_BLANK));
	if ( pBlank == NULL )
	{
		SysMessageDefault( DEFMSG_INSCRIPTION_FAIL );
		return( false );
	}

	CScriptTriggerArgs args("sm_inscription");
	if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE ) return true;
	return Cmd_Skill_Menu( g_Cfg.ResourceGetIDType( RES_SKILLMENU, "sm_inscription" ) );
}

bool CClient::Cmd_Skill_Alchemy( CItem * pReag )
{
	ADDTOCALLSTACK("CClient::Cmd_Skill_Alchemy");
	// SKILL_ALCHEMY

	if ( pReag == NULL )
		return( false );

	ASSERT(m_pChar);
	if ( ! m_pChar->CanUse( pReag, true ))
		return( false );

	if ( ! pReag->IsType(IT_REAGENT))
	{
		// That is not a reagent.
		SysMessageDefault( DEFMSG_ALCHEMY_NOT_REG );
		return( false );
	}

	// Find bottles to put potions in.
	if ( ! m_pChar->ContentFind(RESOURCE_ID(RES_TYPEDEF,IT_POTION_EMPTY)))
	{
		SysMessageDefault( DEFMSG_ALCHEMY_NOBOTTLES );
		return( false );
	}

	m_Targ_UID = pReag->GetUID();

	// Put up a menu to decide formula ?
	CScriptTriggerArgs args("sm_alchemy");
	if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE ) return true;
	return Cmd_Skill_Menu( g_Cfg.ResourceGetIDType( RES_SKILLMENU, "sm_alchemy" ) );
}

bool CClient::Cmd_Skill_Cartography( int iLevel )
{
	ADDTOCALLSTACK("CClient::Cmd_Skill_Cartography");
	// select the map type.

	ASSERT(m_pChar);
	if ( m_pChar->Skill_Wait(SKILL_CARTOGRAPHY))
		return( false );

	if ( ! m_pChar->ContentFind( RESOURCE_ID(RES_TYPEDEF,IT_MAP_BLANK)))
	{
		SysMessageDefault( DEFMSG_CARTOGRAPHY_FAIL );
		return( false );
	}

	CScriptTriggerArgs args("sm_cartography");
	if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE ) return true;
	return Cmd_Skill_Menu( g_Cfg.ResourceGetIDType( RES_SKILLMENU, "sm_cartography" ) );
}

bool CClient::Cmd_SecureTrade( CChar * pChar, CItem * pItem )
{
	ADDTOCALLSTACK("CClient::Cmd_SecureTrade");
	// Begin secure trading with a char. (Make the initial offer)
	ASSERT(m_pChar);

	if ( pChar )
	{
		CScriptTriggerArgs Args( pChar );
		if ( pItem->OnTrigger( ITRIG_DROPON_CHAR, m_pChar, &Args ) == TRIGRET_RET_TRUE )
				return ( false );
	}

	// Trade window only if another PC.
	if ( ! pChar->IsClient())
	{
		return( pChar->NPC_OnItemGive( m_pChar, pItem ));
	}

	// Is there already a trade window open for this client ?
	CItem * pItemCont = m_pChar->GetContentHead();
	for ( ; pItemCont != NULL; pItemCont = pItemCont->GetNext())
	{
		if ( ! pItemCont->IsType(IT_EQ_TRADE_WINDOW))
			continue; // found it
		CItem * pItemPartner = pItemCont->m_uidLink.ItemFind(); // counterpart trade window.
		if ( pItemPartner == NULL )
			continue;
		CChar * pCharPartner = dynamic_cast <CChar*>( pItemPartner->GetParent());
		if ( pCharPartner != pChar )
			continue;
		CItemContainer * pCont = dynamic_cast <CItemContainer *>( pItemCont );
		ASSERT(pCont);
		pCont->ContentAdd( pItem );
		return( true );
	}

	// Open a new one.
	CItem* pItem1 = CItem::CreateBase( ITEMID_Bulletin1 );
	if ( !pItem1 )
		return( false );

	CItemContainer* pCont1 = dynamic_cast <CItemContainer*> ( pItem1 );
	if ( !pCont1 )
	{
		DEBUG_ERR(("Item 0%x must be a container type to enable player trading.\n", ITEMID_Bulletin1));
		pItem1->Delete();
		return( false );
	}

	pCont1->SetType( IT_EQ_TRADE_WINDOW );

	// no need to perform the same tests on the second, since it is identical to the first
	CItemContainer* pCont2 = dynamic_cast <CItemContainer*> (CItem::CreateBase( ITEMID_Bulletin1 ));
	ASSERT(pCont2);
	pCont2->SetType( IT_EQ_TRADE_WINDOW );

	pCont1->m_itEqTradeWindow.m_fCheck = false;
	pCont1->m_uidLink = pCont2->GetUID();

	pCont2->m_itEqTradeWindow.m_fCheck = false;
	pCont2->m_uidLink = pCont1->GetUID();

	m_pChar->LayerAdd( pCont1, LAYER_SPECIAL );
	pChar->LayerAdd( pCont2, LAYER_SPECIAL );

	PacketTradeAction cmd(SECURE_TRADE_OPEN);
	cmd.prepareContainerOpen(pChar, pCont1, pCont2);
	cmd.send(this);
	cmd.prepareContainerOpen(m_pChar, pCont2, pCont1);
	cmd.send(pChar->GetClient());

	CPointMap pt( 30, 30, 9 );
	pCont1->ContentAdd( pItem, pt );
	return( true );
}


