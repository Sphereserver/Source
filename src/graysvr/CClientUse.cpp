#include "graysvr.h"	// predef header.
#include "../network/send.h"

bool CClient::Cmd_Use_Item(CItem *pItem, bool fTestTouch, bool fScript)
{
	ADDTOCALLSTACK("CClient::Cmd_Use_Item");
	// Assume we can see the target.
	// called from Event_DoubleClick

	if ( !pItem )
		return false;

	const CObjBaseTemplate *pObjTop = pItem->GetTopLevelObj();
	if ( pItem->m_Can & CAN_I_FORCEDC )
		fTestTouch = false;

	if ( fTestTouch )
	{
		if ( !fScript )
		{
			CItemContainer *pContainer = dynamic_cast<CItemContainer *>(pItem->GetParent());
			if ( pContainer )
			{
				// Protect from ,snoop - disallow picking from not opened containers
				bool fInsideOpenedContainer = false;
				if ( pContainer->GetType() == IT_EQ_TRADE_WINDOW )
					fInsideOpenedContainer = true;
				else
				{
					CClient::OpenedContainerMap_t::iterator itContainerFound = m_openedContainers.find(pContainer->GetUID().GetPrivateUID());
					if ( itContainerFound != m_openedContainers.end() )
					{
						DWORD dwTopContainerUID = (((*itContainerFound).second).first).first;
						DWORD dwTopMostContainerUID = (((*itContainerFound).second).first).second;
						CPointMap ptOpenedContainerPosition = ((*itContainerFound).second).second;

						const CObjBase *pObjParent = pContainer->GetParentObj();
						DWORD dwTopContainerUID_ToCheck = pObjParent ? pObjParent->GetUID().GetPrivateUID() : pObjTop->GetUID().GetPrivateUID();

						if ( (dwTopMostContainerUID == pObjTop->GetUID().GetPrivateUID()) && (dwTopContainerUID == dwTopContainerUID_ToCheck) )
						{
							if ( pObjTop->IsChar() )
							{
								// probably a pickup check from pack if pCharTop != this?
								fInsideOpenedContainer = true;
							}
							else
							{
								const CItem *pItemTop = static_cast<const CItem *>(pObjTop);
								if ( pItemTop && (pItemTop->IsType(IT_SHIP_HOLD) || pItemTop->IsType(IT_SHIP_HOLD_LOCK)) && (pItemTop->GetTopPoint().GetRegion(REGION_TYPE_MULTI) == m_pChar->GetTopPoint().GetRegion(REGION_TYPE_MULTI)) )
									fInsideOpenedContainer = true;
								else if ( ptOpenedContainerPosition.GetDist(pObjTop->GetTopPoint()) <= 3 )
									fInsideOpenedContainer = true;
							}
						}
					}
				}

				if ( !fInsideOpenedContainer )
				{
					SysMessageDefault(DEFMSG_REACH_UNABLE);
					return false;
				}
			}
		}

		if ( !m_pChar->CanUse(pItem, false) )
		{
			if ( !m_pChar->CanTouch(pItem) )
				SysMessage(g_Cfg.GetDefaultMsg((m_pChar->IsStatFlag(STATF_DEAD)) ? DEFMSG_REACH_GHOST : DEFMSG_REACH_FAIL));
			else
				SysMessageDefault(DEFMSG_REACH_UNABLE);

			return false;
		}
	}

	if ( IsTrigUsed(TRIGGER_DCLICK) || IsTrigUsed(TRIGGER_ITEMDCLICK) )
	{
		if ( pItem->OnTrigger(ITRIG_DCLICK, m_pChar) == TRIGRET_RET_TRUE )
			return true;
	}

	CItemBase *pItemDef = pItem->Item_GetDef();
	bool fEquipped = pItem->IsItemEquipped();
	if ( pItemDef->IsTypeEquippable() && !fEquipped && pItemDef->GetEquipLayer() )
	{
		bool fMustEquip = true;
		if ( pItem->IsTypeSpellbook() )
			fMustEquip = false;
		else if ( (pItem->IsType(IT_LIGHT_OUT) || pItem->IsType(IT_LIGHT_LIT)) && !pItem->IsItemInContainer() )
			fMustEquip = false;

		if ( fMustEquip )
		{
			if ( !m_pChar->CanMove(pItem) )
				return false;

			if ( (pObjTop != m_pChar) && !m_pChar->CanCarry(pItem) )
			{
				SysMessageDefault(DEFMSG_MSG_HEAVY);
				return false;
			}

			if ( !m_pChar->ItemEquip(pItem, NULL, true) )
				return false;
		}
	}

	CItemSpawn *pSpawn = static_cast<CItemSpawn *>(pItem->m_uidSpawnItem.ItemFind());	// remove this item from its spawn when players DClick it from ground, no other way to take it out.
	if ( pSpawn )
		pSpawn->DelObj(pItem->GetUID());

	SetTargMode();
	m_Targ_PrvUID = m_Targ_UID;
	m_Targ_UID = pItem->GetUID();
	m_tmUseItem.m_pParent = pItem->GetParent();		// store item location to check later if it was not moved

	// Use types of items (specific to client)
	switch ( pItem->GetType() )
	{
		case IT_TRACKER:
		{
			if ( !m_pChar->Skill_Tracking(pItem->m_uidLink) )
			{
				if ( pItem->m_uidLink.IsValidUID() )
					SysMessageDefault(DEFMSG_TRACKING_UNABLE);
				addTarget(CLIMODE_TARG_LINK, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_TRACKER_ATTUNE));
			}
			return true;
		}

		case IT_SHAFT:
		case IT_FEATHER:
		{
			if ( IsTrigUsed(TRIGGER_SKILLMENU) )
			{
				CScriptTriggerArgs args("sm_bolts");
				if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
					return true;
			}
			return Cmd_Skill_Menu(g_Cfg.ResourceGetIDType(RES_SKILLMENU, "sm_bolts"));
		}

		case IT_FISH_POLE:
			if ( fEquipped || !IsSetOF(OF_NoDClickTarget) )
			{
				const CSkillDef *pSkillDef = g_Cfg.GetSkillDef(SKILL_FISHING);
				addTarget(CLIMODE_TARG_USE_ITEM, pSkillDef ? pSkillDef->m_sTargetPrompt.GetPtr() : NULL, true);
			}
			return true;

		case IT_DEED:
			addTargetDeed(pItem);
			return true;

		case IT_EQ_BANK_BOX:
		case IT_EQ_VENDOR_BOX:
			if ( !fScript )
				g_Log.Event(LOGL_WARN|LOGM_CHEAT, "%lx:Cheater '%s' is using 3rd party tools to open bank box\n", GetSocketID(), m_pAccount->GetName());
			return false;

		case IT_CONTAINER_LOCKED:
		case IT_SHIP_HOLD_LOCK:
			if ( !m_pChar->GetContainerCreate(LAYER_PACK)->ContentFindKeyFor(pItem) )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_LOCKED);
				if ( !IsPriv(PRIV_GM) )
					return false;
			}
			// fall through
		case IT_CORPSE:
		case IT_SHIP_HOLD:
		case IT_CONTAINER:
		case IT_TRASH_CAN:
		{
			const CItemContainer *pPack = dynamic_cast<const CItemContainer *>(pItem);
			if ( !pPack )
				return false;

			if ( !m_pChar->Skill_Snoop_Check(pPack) )
			{
				if ( !addContainerSetup(pPack) )
					return false;
			}

			const CItemCorpse *pCorpseItem = dynamic_cast<const CItemCorpse *>(pPack);
			if ( m_pChar->CheckCorpseCrime(pCorpseItem, true, true) )
				SysMessageDefault(DEFMSG_LOOT_CRIMINAL_ACT);
			return true;
		}

		case IT_GAME_BOARD:
		{
			if ( !pItem->IsTopLevel() )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_GAMEBOARD_FAIL);
				return false;
			}
			CItemContainer *pBoard = dynamic_cast<CItemContainer *>(pItem);
			if ( !pBoard )
				return false;
			pBoard->Game_Create();
			addContainerSetup(pBoard);
			return true;
		}

		case IT_BBOARD:
			addBulletinBoard(static_cast<CItemContainer *>(pItem));
			return true;

		case IT_SIGN_GUMP:
		{
			GUMP_TYPE gumpid = pItemDef->m_ttContainer.m_gumpid;
			if ( !gumpid )
				return false;
			addGumpTextDisp(pItem, gumpid, pItem->GetName(), pItem->IsIndividualName() ? pItem->GetName() : NULL);
			return true;
		}

		case IT_BOOK:
		case IT_MESSAGE:
		{
			if ( !addBookOpen(pItem) )
				SysMessageDefault(DEFMSG_ITEMUSE_BOOK_FAIL);
			return true;
		}

		case IT_STONE_GUILD:
		case IT_STONE_TOWN:
			return true;

		case IT_POTION:
		{
			if ( !m_pChar->CanMove(pItem) )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_POTION_FAIL);
				return false;
			}
			if ( RES_GET_INDEX(pItem->m_itPotion.m_Type) == SPELL_Poison )
			{
				// If we click directly on poison potion, we will drink poison and get ill.
				// To use it on Poisoning skill, the skill will request to target the potion.
				m_pChar->OnSpellEffect(SPELL_Poison, m_pChar, pItem->m_itSpell.m_spelllevel, NULL);
				return true;
			}
			if ( RES_GET_INDEX(pItem->m_itPotion.m_Type) == SPELL_Explosion )
			{
				// Throw explosion potion
				if ( m_pChar->ItemPickup(pItem, 1) == -1 )	// put the potion in our hand
					return false;

				pItem->m_itPotion.m_tick = 4;		// countdown to explode
				pItem->m_itPotion.m_ignited = 1;	// ignite it
				pItem->m_uidLink = m_pChar->GetUID();
				pItem->SetTimeout(TICK_PER_SEC);
				m_tmUseItem.m_pParent = pItem->GetParent();
				addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_SELECT_POTION_TARGET), true, true, pItem->m_itPotion.m_tick * TICK_PER_SEC);
				return true;
			}
			m_pChar->Use_Drink(pItem);
			return true;
		}

		case IT_ANIM_ACTIVE:
			SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_ITEM_IN_USE));
			return false;

		case IT_CLOCK:
			addObjMessage(m_pChar->GetTopSector()->GetLocalGameTime(), pItem);
			return true;

		case IT_SPAWN_ITEM:
		case IT_SPAWN_CHAR:
		{
			pSpawn = dynamic_cast<CItemSpawn *>(pItem);
			if ( !pSpawn )
				return false;

			if ( pSpawn->m_currentSpawned )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_SPAWN_NEG);
				pSpawn->KillChildren();		// Removing existing objects spawned from it ( RESET ).
			}
			else
			{
				SysMessageDefault(DEFMSG_ITEMUSE_SPAWN_RESET);
				pSpawn->OnTick(true);		// Forcing the spawn to work and create some objects ( START ).
			}
			return true;
		}

		case IT_SHRINE:
		{
			if ( m_pChar->OnSpellEffect(SPELL_Resurrection, m_pChar, 1000, pItem) )
				return true;
			SysMessageDefault(DEFMSG_ITEMUSE_SHRINE);
			return true;
		}

		case IT_SHIP_TILLER:
		{
			if ( m_NetState->isClientVersion(MINCLIVER_HS) )
			{
				CItemShip *pShip = dynamic_cast<CItemShip *>(pItem->m_uidLink.ItemFind());
				if ( pShip )
				{
					if ( m_pChar->ContentFindKeyFor(pItem) )
						pShip->Ship_SetPilot((pShip->m_itShip.m_Pilot != m_pChar->GetUID()) ? m_pChar : NULL);
					else
						pItem->Speak(g_Cfg.GetDefaultMsg(DEFMSG_TILLER_NOTYOURSHIP));
					return true;
				}
			}
			pItem->Speak(g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_TILLERMAN), HUE_TEXT_DEF, TALKMODE_SAY, FONT_NORMAL);
			return true;
		}

		case IT_WAND:
		case IT_SCROLL:
		{
			SPELL_TYPE spell = static_cast<SPELL_TYPE>(RES_GET_INDEX(pItem->m_itWeapon.m_spell));
			CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
			if ( !pSpellDef )
				return false;

			if ( IsSetMagicFlags(MAGICF_PRECAST) && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST) )
			{
				int skill;
				if ( !pSpellDef->GetPrimarySkill(&skill) )
					return false;

				m_tmSkillMagery.m_Spell = spell;	// m_atMagery.m_Spell
				m_pChar->m_atMagery.m_Spell = spell;
				m_pChar->Skill_Start(static_cast<SKILL_TYPE>(skill));
				return true;
			}
			return Cmd_Skill_Magery(spell, pItem);
		}

		case IT_RUNE:
		{
			if ( !m_pChar->CanMove(pItem) )
				return false;

			if ( !pItem->m_itTelepad.m_ptMark.IsValidXY() )
			{
				SysMessageDefault(DEFMSG_SPELL_RECALL_RUNENOTMARKED);
				return false;
			}
			addPromptConsole(CLIMODE_PROMPT_NAME_RUNE, g_Cfg.GetDefaultMsg(DEFMSG_RUNE_RENAME_PROMPT), pItem->GetUID());
			return true;
		}

		case IT_CARPENTRY:
		{
			if ( IsTrigUsed(TRIGGER_SKILLMENU) )
			{
				CScriptTriggerArgs args("sm_carpentry");
				if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
					return true;
			}
			return Cmd_Skill_Menu(g_Cfg.ResourceGetIDType(RES_SKILLMENU, "sm_carpentry"));
		}

		case IT_ORE:
			addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_ORE));
			return true;

		case IT_INGOT:
			return Cmd_Skill_Smith(pItem);

		case IT_KEY:
		case IT_KEYRING:
		{
			if ( (pItem->GetTopLevelObj() != m_pChar) && !m_pChar->IsPriv(PRIV_GM) )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_KEY_FAIL);
				return false;
			}
			addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_KEY_PROMT), false, true);
			return true;
		}

		case IT_BANDAGE:		// SKILL_HEALING, or SKILL_VETERINARY
			addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_BANDAGE_PROMT));
			return true;

		case IT_BANDAGE_BLOOD:	// Clean the bandages.
		case IT_COTTON:			// use on a spinning wheel.
		case IT_WOOL:			// use on a spinning wheel.
		case IT_YARN:			// Use this on a loom.
		case IT_THREAD: 		// Use this on a loom.
		case IT_COMM_CRYSTAL:
			addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_TARGET_PROMT));
			return true;

		case IT_CARPENTRY_CHOP:
		case IT_LOCKPICK:		// Use on a locked thing.
		case IT_SCISSORS:
			addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_TARGET_PROMT), false, true);
			return true;

		case IT_WEAPON_MACE_PICK:
			if ( fEquipped || !IsSetOF(OF_NoDClickTarget) )
			{
				// Mine at the location
				TCHAR *pszTemp = Str_GetTemp();
				sprintf(pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_MACEPICK_TARG), pItem->GetName());
				addTarget(CLIMODE_TARG_USE_ITEM, pszTemp, true, true);
			}
			return true;

		case IT_WEAPON_SWORD:
		case IT_WEAPON_FENCE:
		case IT_WEAPON_AXE:
		case IT_WEAPON_MACE_SHARP:
		case IT_WEAPON_MACE_STAFF:
		case IT_WEAPON_MACE_SMITH:
			if ( fEquipped || !IsSetOF(OF_NoDClickTarget) )
				addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_WEAPON_PROMT), false, true);
			return true;

		case IT_WEAPON_MACE_CROOK:
			if ( fEquipped || !IsSetOF(OF_NoDClickTarget) )
				addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_CROOK_PROMT), false, true);
			return true;

		case IT_FISH:
			SysMessageDefault(DEFMSG_ITEMUSE_FISH_FAIL);
			return true;

		case IT_MAP:
			addDrawMap(static_cast<CItemMap *>(pItem));
			return true;

		case IT_CANNON_BALL:
		{
			TCHAR *pszTemp = Str_GetTemp();
			sprintf(pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_CBALL_PROMT), pItem->GetName());
			addTarget(CLIMODE_TARG_USE_ITEM, pszTemp);
			return true;
		}

		case IT_CANNON_MUZZLE:
		{
			if ( !m_pChar->CanUse(pItem, false) )
				return false;

			// Make sure the cannon is loaded.
			if ( !(pItem->m_itCannon.m_Load & 1) )
			{
				addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_CANNON_POWDER));
				return true;
			}
			if ( !(pItem->m_itCannon.m_Load & 2) )
			{
				addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_CANNON_SHOT));
				return true;
			}
			addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_CANNON_TARG), false, true);
			return true;
		}

		case IT_CRYSTAL_BALL:
			// Gaze into the crystal ball.
			return true;

		case IT_SEED:
		case IT_PITCHER_EMPTY:
		{
			TCHAR *pszTemp = Str_GetTemp();
			sprintf(pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_PITCHER_TARG), pItem->GetName());
			addTarget(CLIMODE_TARG_USE_ITEM, pszTemp, true);
			return true;
		}

		case IT_SPELLBOOK:
		case IT_SPELLBOOK_NECRO:
		case IT_SPELLBOOK_PALA:
		case IT_SPELLBOOK_BUSHIDO:
		case IT_SPELLBOOK_NINJITSU:
		case IT_SPELLBOOK_ARCANIST:
		case IT_SPELLBOOK_MYSTIC:
		case IT_SPELLBOOK_MASTERY:
			addSpellbookOpen(pItem);
			return true;

		case IT_DYE:
			addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_DYE_VAT));
			return true;

		case IT_DYE_VAT:
			addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_DYE_TARG), false, true);
			return true;

		case IT_MORTAR:
		{
			if ( IsTrigUsed(TRIGGER_SKILLMENU) )
			{
				CScriptTriggerArgs args("sm_alchemy");
				if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
					return true;
			}
			return Cmd_Skill_Menu(g_Cfg.ResourceGetIDType(RES_SKILLMENU, "sm_alchemy"));
		}

		case IT_CARTOGRAPHY:
		{
			if ( IsTrigUsed(TRIGGER_SKILLMENU) )
			{
				CScriptTriggerArgs args("sm_cartography");
				if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
					return true;
			}
			return Cmd_Skill_Menu(g_Cfg.ResourceGetIDType(RES_SKILLMENU, "sm_cartography"));
		}

		case IT_COOKING:
		{
			if ( IsTrigUsed(TRIGGER_SKILLMENU) )
			{
				CScriptTriggerArgs args("sm_cooking");
				if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
					return true;
			}
			return Cmd_Skill_Menu(g_Cfg.ResourceGetIDType(RES_SKILLMENU, "sm_cooking"));
		}

		case IT_TINKER_TOOLS:
		{
			if ( IsTrigUsed(TRIGGER_SKILLMENU) )
			{
				CScriptTriggerArgs args("sm_tinker");
				if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
					return true;
			}
			return Cmd_Skill_Menu(g_Cfg.ResourceGetIDType(RES_SKILLMENU, "sm_tinker"));
		}

		case IT_SEWING_KIT:
		{
			TCHAR *pszTemp = Str_GetTemp();
			sprintf(pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SEWKIT_PROMT), pItem->GetName());
			addTarget(CLIMODE_TARG_USE_ITEM, pszTemp);
			return true;
		}

		case IT_SCROLL_BLANK:
			Cmd_Skill_Inscription();
			return true;

		default:
		{
			// An NPC could use it this way.
			if ( m_pChar->Use_Item(pItem) )
				return true;
			break;
		}
	}

	SysMessageDefault(DEFMSG_ITEMUSE_CANTTHINK);
	return false;
}

void CClient::Cmd_EditItem(CObjBase *pObj, int iSelect)
{
	ADDTOCALLSTACK("CClient::Cmd_EditItem");
	// ARGS:
	//   iSelect == -1 = setup.
	//   m_Targ_Text = what are we doing to it ?
	//
	if ( !pObj )
		return;

	CContainer *pContainer = dynamic_cast<CContainer *>(pObj);
	if ( !pContainer )
	{
		addGumpDialogProps(pObj);
		return;
	}

	if ( iSelect == 0 )		// cancelled
		return;

	if ( iSelect > 0 )		// we selected an item
	{
		if ( static_cast<size_t>(iSelect) >= COUNTOF(m_tmMenu.m_Item) )
			return;

		if ( m_Targ_Text.IsEmpty() )
			addGumpDialogProps(CGrayUID(m_tmMenu.m_Item[iSelect]).ObjFind());
		else
			OnTarg_Obj_Set(CGrayUID(m_tmMenu.m_Item[iSelect]).ObjFind());
		return;
	}

	CMenuItem item[minimum(COUNTOF(m_tmMenu.m_Item), MAX_MENU_ITEMS)];	// Most as we want to display at one time.
	item[0].m_sText.Format("Contents of %s", pObj->GetName());

	size_t iCount = 0;
	for ( CItem *pItem = pContainer->GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		++iCount;
		m_tmMenu.m_Item[iCount] = pItem->GetUID();
		item[iCount].m_sText = pItem->GetName();
		item[iCount].m_id = static_cast<WORD>(pItem->GetDispID());
		item[iCount].m_color = 0;

		if ( !pItem->IsType(IT_EQ_MEMORY_OBJ) )
		{
			HUE_TYPE wHue = pItem->GetHue();
			if ( wHue != 0 )
			{
				wHue = (wHue == 1 ? 0x7FF : wHue - 1);
				item[iCount].m_color = wHue;
			}
		}

		if ( iCount >= (COUNTOF(item) - 1) )
			break;
	}

	ASSERT(iCount < COUNTOF(item));
	addItemMenu(CLIMODE_MENU_EDIT, item, iCount, pObj);
}

bool CClient::Cmd_Skill_Menu(RESOURCE_ID_BASE rid, int iSelect)
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
		return false;

	bool fShowMenu = false;
	bool fLimitReached = false;

	if ( iSelect == 0 )		// menu cancelled
		return (Cmd_Skill_Menu_Build(rid, iSelect, NULL, 0, fShowMenu, fLimitReached) > 0);

	CMenuItem pMenuItem[minimum(COUNTOF(m_tmMenu.m_Item), MAX_MENU_ITEMS)];
	size_t iMenuItemCount = COUNTOF(pMenuItem);
	size_t iShowCount = Cmd_Skill_Menu_Build(rid, iSelect, pMenuItem, iMenuItemCount, fShowMenu, fLimitReached);

	if ( iSelect < -1 )		// just a test
		return (iShowCount > 0);

	if ( iSelect > 0 )		// seems our resources disappeared.
	{
		if ( iShowCount <= 0 )
		{
			SysMessageDefault(DEFMSG_CANT_MAKE);
			return false;
		}
		return true;
	}

	if ( iShowCount <= 0 )
	{
		SysMessageDefault(DEFMSG_CANT_MAKE_RES);
		return false;
	}

	if ( (iShowCount == 1) && fShowMenu && !fLimitReached )
	{
		static int sm_iReentrant = 0;
		if ( sm_iReentrant < 12 )
		{
			++sm_iReentrant;

			// If there is just one menu then select it.
			bool fSuccess = Cmd_Skill_Menu(rid, m_tmMenu.m_Item[1]);

			--sm_iReentrant;
			return fSuccess;
		}

#ifdef _DEBUG
		if ( g_Cfg.m_wDebugFlags & DEBUGF_SCRIPTS )
			g_Log.EventDebug("SCRIPT: Too many skill menus (circular menus?) to continue searching in menu '%s'\n", g_Cfg.ResourceGetDef(rid)->GetResourceName());
#endif
	}

	if ( iShowCount >= iMenuItemCount )		// this should never happen
		iShowCount = iMenuItemCount - 1;

	addItemMenu(CLIMODE_MENU_SKILL, pMenuItem, iShowCount);
	return true;
}

size_t CClient::Cmd_Skill_Menu_Build(RESOURCE_ID_BASE rid, int iSelect, CMenuItem *item, size_t iMaxSize, bool &fShowMenu, bool &fLimitReached)
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
		return 0;

	// Find section.
	CResourceLock s;
	if ( !g_Cfg.ResourceLock(s, rid) )
		return 0;

	// Get title line
	if ( !s.ReadKey() )
		return 0;

	if ( iSelect == 0 )		// cancelled
	{
		while ( s.ReadKeyParse() )
		{
			if ( !s.IsKey("ON") || (*s.GetArgStr() != '@') )
				continue;
			if ( strcmpi(s.GetArgStr(), "@Cancel") )
				continue;
			if ( m_pChar->OnTriggerRunVal(s, TRIGRUN_SECTION_TRUE, m_pChar, NULL) == TRIGRET_RET_TRUE )
				return 0;
			break;
		}
		return 1;
	}

	if ( iSelect < 0 )
	{
		item[0].m_sText = s.GetKey();
		if ( iSelect == -1 )
			m_tmMenu.m_ResourceID = rid;
	}

	bool fSkip = false;		// skip this if we lack resources or skill.
	int iOnCount = 0;
	size_t iShowCount = 0;
	CScriptTriggerArgs Args;

	while ( s.ReadKeyParse() )
	{
		if ( s.IsKeyHead("ON", 2) )
		{
			if ( *s.GetArgStr() == '@' )
			{
				fSkip = true;
				continue;
			}

			// a new option to look at.
			fSkip = false;
			++iOnCount;

			if ( iSelect < 0 )	// building up the list.
			{
				if ( (iSelect < -1) && (iShowCount >= 1) )		// just a test. so we are done.
					return 1;

				++iShowCount;
				if ( !item[iSelect == -2 ? 0 : iShowCount].ParseLine(s.GetArgRaw(), NULL, m_pChar) )
				{
					// remove if the item is invalid.
					--iShowCount;
					fSkip = true;
					continue;
				}

				if ( iSelect == -1 )
					m_tmMenu.m_Item[iShowCount] = iOnCount;

				if ( iShowCount >= (iMaxSize - 1) )
					break;
			}
			else
			{
				if ( iOnCount > iSelect )	// we are done.
					break;
			}
			continue;
		}

		if ( fSkip )	// we have decided we cant do this option.
			continue;
		if ( (iSelect > 0) && (iOnCount != iSelect) )		// only interested in the selected option
			continue;

		// Check for a skill / non-consumables required.
		if ( s.IsKey("TEST") )
		{
			m_pChar->ParseText(s.GetArgRaw(), m_pChar);
			CResourceQtyArray skills(s.GetArgStr());
			if ( !skills.IsResourceMatchAll(m_pChar) )
			{
				--iShowCount;
				fSkip = true;
			}
			continue;
		}

		if ( s.IsKey("TESTIF") )
		{
			m_pChar->ParseText(s.GetArgRaw(), m_pChar);
			if ( !s.GetArgVal() )
			{
				--iShowCount;
				fSkip = true;
			}
			continue;
		}

		// select to execute any entries here ?
		if ( iOnCount == iSelect )
		{
			// Execute command from script
			TRIGRET_TYPE tRet = m_pChar->OnTriggerRunVal(s, TRIGRUN_SINGLE_EXEC, m_pChar, &Args);
			if ( tRet != TRIGRET_RET_DEFAULT )
				return tRet == TRIGRET_RET_TRUE ? 0 : 1;

			++iShowCount;	// we are good. but continue til the end
		}
		else
		{
			ASSERT(iSelect < 0);
			if ( s.IsKey("SKILLMENU") )
			{
				static int sm_iReentrant = 0;
				if ( sm_iReentrant > 1024 )
				{
#ifdef _DEBUG
					if ( g_Cfg.m_wDebugFlags & DEBUGF_SCRIPTS )
						g_Log.EventDebug("SCRIPT: Too many skill menus (circular menus?) to continue searching in menu '%s'\n", g_Cfg.ResourceGetDef(rid)->GetResourceName());
#endif
					fLimitReached = true;
				}
				else
				{
					// Test if there is anything in this skillmenu we can do.
					++sm_iReentrant;
					CMenuItem pReentrantMenuItem;
					if ( !Cmd_Skill_Menu_Build(g_Cfg.ResourceGetIDType(RES_SKILLMENU, s.GetArgStr()), -2, &pReentrantMenuItem, iMaxSize, fShowMenu, fLimitReached) )
					{
						--iShowCount;
						fSkip = true;
					}
					else
						fShowMenu = true;

					--sm_iReentrant;
				}
				continue;
			}

			if ( s.IsKey("MAKEITEM") )
			{
				// test if i can make this item using m_Targ_UID.
				// There should ALWAYS be a valid id here.
				if ( !m_pChar->Skill_MakeItem(static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType(RES_ITEMDEF, s.GetArgStr())), m_Targ_UID, SKTRIG_SELECT) )
				{
					--iShowCount;
					fSkip = true;
				}
				continue;
			}
		}
	}
	return iShowCount;
}

bool CClient::Cmd_Skill_Magery(SPELL_TYPE iSpell, CObjBase *pSrc)
{
	ADDTOCALLSTACK("CClient::Cmd_Skill_Magery");
	// Start casting a spell. Prompt for target.
	// ARGS:
	//	pSrc = you the char.
	//	pSrc = magic object is source ?

	ASSERT(m_pChar);

	const CSpellDef *pSpellDef;
	if ( IsSetMagicFlags(MAGICF_PRECAST) && (iSpell == m_tmSkillMagery.m_Spell) )
	{
		pSpellDef = g_Cfg.GetSpellDef(m_tmSkillMagery.m_Spell);
		if ( pSpellDef && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST) )
			iSpell = m_tmSkillMagery.m_Spell;
	}
	else
		pSpellDef = g_Cfg.GetSpellDef(iSpell);

	if ( !pSpellDef )
		return false;

	// Do we have the regs? Etc.
	if ( !m_pChar->Spell_CanCast(iSpell, true, pSrc, true) )
		return false;

	SetTargMode();
	m_tmSkillMagery.m_Spell = iSpell;	// m_atMagery.m_Spell
	m_Targ_UID = m_pChar->GetUID();		// Default target.
	m_Targ_PrvUID = pSrc->GetUID();		// Source of the spell.

	switch ( iSpell )
	{
		case SPELL_Polymorph:
		{
			if ( IsTrigUsed(TRIGGER_SKILLMENU) )
			{
				CScriptTriggerArgs args("sm_polymorph");
				if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
					return true;
			}
			return Cmd_Skill_Menu(g_Cfg.ResourceGetIDType(RES_SKILLMENU, "sm_polymorph"));
		}

		case SPELL_Summon:
		{
			if ( IsTrigUsed(TRIGGER_SKILLMENU) )
			{
				CScriptTriggerArgs args("sm_summon");
				if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
					return true;
			}
			return Cmd_Skill_Menu(g_Cfg.ResourceGetIDType(RES_SKILLMENU, "sm_summon"));
		}

		case SPELL_Summon_Familiar:
		{
			if ( IsTrigUsed(TRIGGER_SKILLMENU) )
			{
				CScriptTriggerArgs args("sm_summon_familiar");
				if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
					return true;
			}
			return Cmd_Skill_Menu(g_Cfg.ResourceGetIDType(RES_SKILLMENU, "sm_summon_familiar"));
		}

		default:
			break;
	}

	// Targeted spells
	if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_OBJ|SPELLFLAG_TARG_XYZ) )
	{
		LPCTSTR pszPrompt = !pSpellDef->m_sTargetPrompt.IsEmpty() ? static_cast<LPCTSTR>(pSpellDef->m_sTargetPrompt) : g_Cfg.GetDefaultMsg(DEFMSG_SELECT_MAGIC_TARGET);

		int iTimeout = static_cast<int>(m_pChar->GetDefNum("SPELLTIMEOUT"));
		if ( !iTimeout )
			iTimeout = g_Cfg.m_iSpellTimeout * TICK_PER_SEC;

		addTarget(CLIMODE_TARG_SKILL_MAGERY, pszPrompt, pSpellDef->IsSpellType(SPELLFLAG_TARG_XYZ), pSpellDef->IsSpellType(SPELLFLAG_HARM), iTimeout);
		return true;
	}

	// Non-targeted spells
	m_pChar->m_Act_p = m_pChar->GetTopPoint();
	m_pChar->m_Act_Targ = m_Targ_UID;
	m_pChar->m_Act_TargPrv = m_Targ_PrvUID;
	m_pChar->m_atMagery.m_Spell = iSpell;
	m_Targ_p = m_pChar->GetTopPoint();

	if ( IsSetMagicFlags(MAGICF_PRECAST) && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST) )
	{
		m_pChar->Spell_CastDone();
		return true;
	}
	else
	{
		int skill;
		if ( !pSpellDef->GetPrimarySkill(&skill) )
			return false;

		return m_pChar->Skill_Start(static_cast<SKILL_TYPE>(skill));
	}
}

bool CClient::Cmd_Skill_Tracking(WORD wTrackType, bool fExec)
{
	ADDTOCALLSTACK("CClient::Cmd_Skill_Tracking");
	// look around for stuff.

	ASSERT(m_pChar);
	if ( wTrackType == WORD_MAX )
	{
		// Unlike others skills, Tracking is used during menu setup
		m_pChar->Skill_Cleanup();	// clean up current skill

		CMenuItem item[6];
		item[0].m_sText = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_SKILLMENU_TITLE);

		item[1].m_id = ITEMID_TRACK_HORSE;
		item[1].m_color = 0;
		item[1].m_sText = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_SKILLMENU_ANIMALS);
		item[2].m_id = ITEMID_TRACK_OGRE;
		item[2].m_color = 0;
		item[2].m_sText = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_SKILLMENU_MONSTERS);
		item[3].m_id = ITEMID_TRACK_MAN;
		item[3].m_color = 0;
		item[3].m_sText = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_SKILLMENU_NPCS);
		item[4].m_id = ITEMID_TRACK_WOMAN;
		item[4].m_color = 0;
		item[4].m_sText = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_SKILLMENU_PLAYERS);

		m_tmMenu.m_Item[0] = 0;
		addItemMenu(CLIMODE_MENU_SKILL_TRACK_SETUP, item, 4);
		return true;
	}

	if ( wTrackType > 0 ) // Not Cancelled
	{
		if ( wTrackType >= COUNTOF(m_tmMenu.m_Item) )
			return false;

		if ( fExec )
		{
			// Tracking menu got us here. Start tracking the selected creature.
			m_pChar->SetTimeout(1 * TICK_PER_SEC);
			m_pChar->m_Act_Targ = static_cast<CGrayUID>(m_tmMenu.m_Item[wTrackType]);
			m_pChar->Skill_Start(SKILL_TRACKING);
			return true;
		}

		static const NPCBRAIN_TYPE sm_Track_Brain[] =
		{
			NPCBRAIN_QTY,	// not used here
			NPCBRAIN_ANIMAL,
			NPCBRAIN_MONSTER,
			NPCBRAIN_HUMAN,
			NPCBRAIN_NONE	// players
		};

		if ( wTrackType >= COUNTOF(sm_Track_Brain) )
			return false;

		NPCBRAIN_TYPE track_type = sm_Track_Brain[wTrackType];
		CMenuItem item[minimum(MAX_MENU_ITEMS, COUNTOF(m_tmMenu.m_Item))];
		size_t iCount = 0;

		item[0].m_sText = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_SKILLMENU_TITLE);
		m_tmMenu.m_Item[0] = wTrackType;

		CWorldSearch AreaChars(m_pChar->GetTopPoint(), m_pChar->Skill_GetBase(SKILL_TRACKING) / 10 + 10);
		for (;;)
		{
			CChar *pChar = AreaChars.GetChar();
			if ( !pChar )
				break;
			if ( pChar == m_pChar )
				continue;

			if ( pChar->GetNPCBrain() != track_type )
				continue;
			if ( pChar->IsStatFlag(STATF_DEAD) )	// can't track ghosts
				continue;

			if ( pChar->m_pPlayer )
			{
				// Prevent track hidden GMs
				if ( pChar->IsStatFlag(STATF_Insubstantial) && (pChar->GetPrivLevel() > GetPrivLevel()) )
					continue;

				// Check action difficulty when trying to track players
				WORD wTrackingSkill = m_pChar->Skill_GetBase(SKILL_TRACKING);
				WORD wDetectHiddenSkill = m_pChar->Skill_GetBase(SKILL_DETECTINGHIDDEN);
				if ( (g_Cfg.m_iRacialFlags & RACIALF_ELF_DIFFTRACK) && pChar->IsElf() )
					wTrackingSkill /= 2;			// elves are more difficult to track (Difficult to Track racial trait)

				WORD wDivisor = maximum(pChar->Skill_GetBase(SKILL_HIDING) + pChar->Skill_GetBase(SKILL_STEALTH), 1);

				int iChance;
				if ( g_Cfg.m_iFeatureSE & FEATURE_SE_UPDATE )
					iChance = 50 * (wTrackingSkill * 2 + wDetectHiddenSkill) / wDivisor;
				else
					iChance = 50 * (wTrackingSkill + wDetectHiddenSkill + 10 * Calc_GetRandVal(20)) / wDivisor;

				if ( Calc_GetRandVal(100) > iChance )
					continue;
			}

			CCharBase *pCharDef = pChar->Char_GetDef();
			if ( !pCharDef )
				continue;

			++iCount;
			item[iCount].m_id = static_cast<WORD>(pCharDef->m_trackID);
			item[iCount].m_color = 0;
			item[iCount].m_sText = pChar->GetName();
			m_tmMenu.m_Item[iCount] = pChar->GetUID();

			if ( iCount >= COUNTOF(item) - 1 )
				break;
		}

		// Some credit for trying
		if ( iCount > 0 )
		{
			m_pChar->Skill_UseQuick(SKILL_TRACKING, 20 + Calc_GetRandVal(30));
			ASSERT(iCount < COUNTOF(item));
			addItemMenu(CLIMODE_MENU_SKILL_TRACK, item, iCount);
			return true;
		}
		else
		{
			// Tracking failed or cancelled
			m_pChar->Skill_UseQuick(SKILL_TRACKING, 10 + Calc_GetRandVal(30));

			static LPCTSTR const sm_Track_FailMsg[] =
			{
				g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_FAIL_ANIMAL),
				g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_FAIL_MONSTER),
				g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_FAIL_PEOPLE),
				g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_FAIL_PEOPLE)
			};

			if ( sm_Track_FailMsg[wTrackType - 1] )
				SysMessage(sm_Track_FailMsg[wTrackType - 1]);
		}
	}
	return false;
}

bool CClient::Cmd_Skill_Smith(CItem *pIngots)
{
	ADDTOCALLSTACK("CClient::Cmd_Skill_Smith");
	ASSERT(m_pChar);
	if ( !pIngots || !pIngots->IsType(IT_INGOT) )
	{
		SysMessageDefault(DEFMSG_SMITHING_FAIL);
		return false;
	}

	ASSERT(m_Targ_UID == pIngots->GetUID());
	if ( pIngots->GetTopLevelObj() != m_pChar )
	{
		SysMessageDefault(DEFMSG_SMITHING_REACH);
		return false;
	}

	// Must have smith hammer equipped
	CItem *pSmithHammer = m_pChar->LayerFind(LAYER_HAND1);
	if ( !pSmithHammer || !pSmithHammer->IsType(IT_WEAPON_MACE_SMITH) )
	{
		SysMessageDefault(DEFMSG_SMITHING_HAMMER);
		return false;
	}

	// Select the blacksmith item type.
	// repair items or make type of items.
	if ( !g_World.IsItemTypeNear(m_pChar->GetTopPoint(), IT_FORGE, 3, false, true) )
	{
		SysMessageDefault(DEFMSG_SMITHING_FORGE);
		return false;
	}

	// Select the blacksmith item type.
	// repair items or make type of items.
	if ( IsTrigUsed(TRIGGER_SKILLMENU) )
	{
		CScriptTriggerArgs args("sm_blacksmith");
		if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
			return true;
	}
	return Cmd_Skill_Menu(g_Cfg.ResourceGetIDType(RES_SKILLMENU, "sm_blacksmith"));
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

	CItem *pBlankScroll = m_pChar->ContentFind(RESOURCE_ID(RES_TYPEDEF, IT_SCROLL_BLANK));
	if ( !pBlankScroll )
	{
		SysMessageDefault(DEFMSG_INSCRIPTION_FAIL);
		return false;
	}

	if ( IsTrigUsed(TRIGGER_SKILLMENU) )
	{
		CScriptTriggerArgs args("sm_inscription");
		if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
			return true;
	}
	return Cmd_Skill_Menu(g_Cfg.ResourceGetIDType(RES_SKILLMENU, "sm_inscription"));
}

bool CClient::Cmd_SecureTrade(CChar *pChar, CItem *pItem)
{
	ADDTOCALLSTACK("CClient::Cmd_SecureTrade");
	// Begin secure trading with a char. (Make the initial offer)
	if ( !pChar || (pChar == m_pChar) )
		return false;

	// Make sure both clients can see each other, because trade window is an container
	// and containers can be opened only after the object is already loaded on screen
	if ( !m_pChar->CanSee(pChar) || !pChar->CanSee(m_pChar) )
		return false;

	if ( pItem && (IsTrigUsed(TRIGGER_DROPON_CHAR) || IsTrigUsed(TRIGGER_ITEMDROPON_CHAR)) )
	{
		CScriptTriggerArgs Args(pChar);
		if ( pItem->OnTrigger(ITRIG_DROPON_CHAR, m_pChar, &Args) == TRIGRET_RET_TRUE )
			return false;
	}

	if ( pChar->m_pNPC )		// NPCs can't use trade window
		return pItem ? pChar->NPC_OnReceiveItem(m_pChar, pItem) : false;
	if ( !pChar->m_pClient )	// and also offline players
		return false;

	if ( pChar->m_pPlayer && pChar->m_pPlayer->m_bRefuseTrades )
	{
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_REFUSE), pChar->GetName());
		return false;
	}

	// Check if the trade window is already open
	for ( CItem *pItemCont = m_pChar->GetContentHead(); pItemCont != NULL; pItemCont = pItemCont->GetNext() )
	{
		if ( !pItemCont->IsType(IT_EQ_TRADE_WINDOW) )
			continue;

		CItem *pItemPartner = pItemCont->m_uidLink.ItemFind();
		if ( !pItemPartner )
			continue;

		CChar *pCharPartner = dynamic_cast<CChar *>(pItemPartner->GetParent());
		if ( pCharPartner != pChar )
			continue;

		if ( pItem )
		{
			if ( IsTrigUsed(TRIGGER_DROPON_TRADE) )
			{
				CScriptTriggerArgs Args1(pChar);
				if ( pItem->OnTrigger(ITRIG_DROPON_TRADE, this, &Args1) == TRIGRET_RET_TRUE )
					return false;
			}
			CItemContainer *pCont = static_cast<CItemContainer *>(pItemCont);
			if ( pCont )
				pCont->ContentAdd(pItem);
		}
		return true;
	}

	// Open new trade window
	if ( IsTrigUsed(TRIGGER_TRADECREATE) )
	{
		CScriptTriggerArgs Args(pItem);
		if ( (m_pChar->OnTrigger(CTRIG_TradeCreate, pChar, &Args) == TRIGRET_RET_TRUE) || (pChar->OnTrigger(CTRIG_TradeCreate, m_pChar, &Args) == TRIGRET_RET_TRUE) )
			return false;
	}

	if ( IsTrigUsed(TRIGGER_DROPON_TRADE) && pItem )
	{
		CScriptTriggerArgs Args1(pChar);
		if ( pItem->OnTrigger(ITRIG_DROPON_TRADE, this, &Args1) == TRIGRET_RET_TRUE )
			return false;
	}

	CItem *pItem1 = CItem::CreateBase(ITEMID_Bulletin1);
	CItem *pItem2 = CItem::CreateBase(ITEMID_Bulletin1);
	if ( !pItem1 || !pItem2 )
		return false;

	CItemContainer *pCont1 = dynamic_cast<CItemContainer *>(pItem1);
	CItemContainer *pCont2 = dynamic_cast<CItemContainer *>(pItem2);
	if ( !pCont1 || !pCont2 )
	{
		DEBUG_ERR(("Can't create trade window on char '%s' [0%lx]: Itemdef 0%x is not a container type\n", m_pChar->GetName(), static_cast<DWORD>(m_pChar->GetUID()), ITEMID_Bulletin1));
		pItem1->Delete();
		pItem2->Delete();
		return false;
	}

	pCont1->SetName("Trade Window");
	pCont1->SetType(IT_EQ_TRADE_WINDOW);
	pCont1->m_itEqTradeWindow.m_iWaitTime = 0;
	pCont1->m_itEqTradeWindow.m_bCheck = 0;
	pCont1->m_uidLink = pCont2->GetUID();
	m_pChar->LayerAdd(pCont1, LAYER_SPECIAL);

	pCont2->SetName("Trade Window");
	pCont2->SetType(IT_EQ_TRADE_WINDOW);
	pCont2->m_itEqTradeWindow.m_iWaitTime = 0;
	pCont2->m_itEqTradeWindow.m_bCheck = 0;
	pCont2->m_uidLink = pCont1->GetUID();
	pChar->LayerAdd(pCont2, LAYER_SPECIAL);

	PacketTradeAction cmd(SECURETRADE_Open);
	cmd.prepareContainerOpen(pChar, pCont1, pCont2);
	cmd.send(this);
	cmd.prepareContainerOpen(m_pChar, pCont2, pCont1);
	cmd.send(pChar->m_pClient);

	if ( g_Cfg.m_iFeatureTOL & FEATURE_TOL_VIRTUALGOLD )
	{
		PacketTradeAction cmd2(SECURETRADE_UpdateLedger);
		if ( m_NetState->isClientVersion(MINCLIVER_TOL) )
		{
			cmd2.prepareUpdateLedger(pCont1, static_cast<DWORD>(m_pChar->m_virtualGold % 1000000000), static_cast<DWORD>(m_pChar->m_virtualGold / 1000000000));
			cmd2.send(this);
		}
		if ( pChar->m_pClient->m_NetState->isClientVersion(MINCLIVER_TOL) )
		{
			cmd2.prepareUpdateLedger(pCont2, static_cast<DWORD>(pChar->m_virtualGold % 1000000000), static_cast<DWORD>(pChar->m_virtualGold / 1000000000));
			cmd2.send(pChar->m_pClient);
		}
	}

	LogOpenedContainer(pCont2);
	pChar->m_pClient->LogOpenedContainer(pCont1);

	if ( pItem )
	{
		if ( IsTrigUsed(TRIGGER_DROPON_TRADE) )
		{
			CScriptTriggerArgs Args1(pChar);
			if ( pItem->OnTrigger(ITRIG_DROPON_TRADE, this, &Args1) == TRIGRET_RET_TRUE )
			{
				pCont1->Delete();
				pCont2->Delete();
				return false;
			}
		}
		pCont1->ContentAdd(pItem, pCont1->GetTopPoint());
	}
	return true;
}
