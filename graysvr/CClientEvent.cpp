//
// CClientEvent.cpp
// Copyright Menace Software (www.menasoft.com).
//
#include "graysvr.h"	// predef header.
#include "CClient.h"
#include <wchar.h>

/////////////////////////////////
// Events from the Client.

LPCTSTR const CClient::sm_szCmd_Redirect[12] =
{
	"BANK",
	"CONTROL",
	"DUPE",
	"FORGIVE",
	"JAIL",
	"KICK",
	"KILL",
	"NUDGEDOWN",
	"NUDGEUP",
	"PRIVSET",
	"REMOVE",
	"SHRINK",
};

void CClient::Event_ChatButton(const NCHAR * pszName) // Client's chat button was pressed
{
	ADDTOCALLSTACK("CClient::Event_ChatButton");
	// See if they've made a chatname yet
	// m_ChatPersona.SetClient(this);

	if (m_pChar->OnTrigger(CTRIG_UserChatButton, m_pChar) == TRIGRET_RET_TRUE)
		return;

	ASSERT(GetAccount());

	if ( GetAccount()->m_sChatName.IsEmpty())
	{
		// No chatname yet, see if the client sent one
		if (pszName[0] == 0) // No name was sent, so ask for a permanent chat system nickname (account based)
		{
			addChatSystemMessage(CHATMSG_GetChatName);
			return;
		}
		// OK, we got a nick, now store it with the account stuff.

		// Make it non unicode
		TCHAR szChatName[ MAX_NAME_SIZE * 2 + 2 ];
		CvtNUNICODEToSystem( szChatName, sizeof(szChatName), pszName, 128 );

		if ( ! CChat::IsValidName(szChatName, true) ||
			g_Accounts.Account_FindChat(szChatName)) // Check for legal name, duplicates, etc
		{
			addChatSystemMessage(CHATMSG_Error);
			addChatSystemMessage(CHATMSG_GetChatName);
			return;
		}
		GetAccount()->m_sChatName = szChatName;
	}

	// Ok, below here we have a chat system nickname
	// Tell the chat system it has a new client using it
	SetChatActive();
}

void CClient::Event_ChatText( const NCHAR * pszText, int len, CLanguageID lang ) // Text from a client
{
	ADDTOCALLSTACK("CClient::Event_ChatText");
	// Just send it all to the chat system
	g_Serv.m_Chats.EventMsg( this, pszText, len, lang );
}

void CClient::Event_MapEdit( CGrayUID uid, const CEvent * pEvent )
{
	ADDTOCALLSTACK("CClient::Event_MapEdit");
	CItemMap * pMap = dynamic_cast <CItemMap*> ( uid.ItemFind());
	if ( ! m_pChar->CanTouch( pMap ))	// sanity check.
	{
		SysMessage( "You can't reach it" );
		return;
	}
	if ( pMap->m_itMap.m_fPinsGlued )
	{
		SysMessage( "The pins seem to be glued in place" );
		if ( ! IsPriv(PRIV_GM))
		{
			return;
		}
	}

	// NOTE: while in edit mode, right click canceling of the
	// dialog sends the same msg as
	// request to edit in either mode...strange huh?

	switch (pEvent->MapEdit.m_action)
	{
		case MAP_ADD: // add pin
			{
				if ( pMap->m_Pins.GetCount() > CItemMap::MAX_PINS )
					return;	// too many.
				CMapPinRec pin( pEvent->MapEdit.m_pin_x, pEvent->MapEdit.m_pin_y );
				pMap->m_Pins.Add( pin );
				break;
			}
		case MAP_INSERT: // insert between 2 pins
			{
			if ( pMap->m_Pins.GetCount() > CItemMap::MAX_PINS )
				return;	// too many.
			CMapPinRec pin( pEvent->MapEdit.m_pin_x, pEvent->MapEdit.m_pin_y );
			pMap->m_Pins.InsertAt( pEvent->MapEdit.m_pin, pin);
			break;
			}
		case MAP_MOVE: // move pin
			if ( pEvent->MapEdit.m_pin >= pMap->m_Pins.GetCount())
			{
				SysMessage( "That's strange... (bad pin)" );
				return;
			}
			pMap->m_Pins[pEvent->MapEdit.m_pin].m_x = pEvent->MapEdit.m_pin_x;
			pMap->m_Pins[pEvent->MapEdit.m_pin].m_y = pEvent->MapEdit.m_pin_y;
			break;
		case MAP_DELETE: // delete pin
			{
				if ( pEvent->MapEdit.m_pin >= pMap->m_Pins.GetCount())
				{
					SysMessage( "That's strange... (bad pin)" );
					return;
				}
				pMap->m_Pins.RemoveAt(pEvent->MapEdit.m_pin);
			}
			break;
		case MAP_CLEAR: // clear all pins
			pMap->m_Pins.RemoveAll();
			break;
		case MAP_TOGGLE: // edit req/cancel
			addMapMode( pMap, MAP_SENT, ! pMap->m_fPlotMode );
			break;
	}
}

void CClient::Event_Item_Dye( CGrayUID uid, HUE_TYPE wHue ) // Rehue an item
{
	ADDTOCALLSTACK("CClient::Event_Item_Dye");
	// CLIMODE_DYE : Result from addDyeOption()
	CObjBase	*pObj = uid.ObjFind();
	CItem	*pItem;

	if ( !m_pChar->CanTouch(pObj) )
	{
		SysMessage("You can't reach it");
		return;
	}
	if ( GetTargMode() != CLIMODE_DYE )
		return;

	ClearTargMode();

	if ( !IsPriv(PRIV_GM) )
	{
		if ( !pObj->IsChar() )
		{
			pItem = (CItem *) pObj;
			if ( ( pObj->GetBaseID() != 0xFAB ) && (!pItem->IsType(IT_DYE_VAT) || !IsSetOF(OF_DyeType)) )
				return;

			if ( wHue < HUE_BLUE_LOW )
				wHue = HUE_BLUE_LOW;
			if ( wHue > HUE_DYE_HIGH )
				wHue = HUE_DYE_HIGH;
		} else
			return;
	}
	else if ( pObj->IsChar() )
	{
		pObj->RemoveFromView();
		wHue |= HUE_UNDERWEAR;
	}

	pObj->SetHue(wHue);
	pObj->Update();
}


void CClient::Event_Tips( WORD i) // Tip of the day window
{
	ADDTOCALLSTACK("CClient::Event_Tips");
	if (i==0)
		i=1;
	CResourceLock s;
	if ( g_Cfg.ResourceLock( s, RESOURCE_ID( RES_TIP, i )))
	{
		addScrollScript( s, SCROLL_TYPE_TIPS, i );
	}
}



void CClient::Event_Book_Title( CGrayUID uid, LPCTSTR pszTitle, LPCTSTR pszAuthor )
{
	ADDTOCALLSTACK("CClient::Event_Book_Title");
	// XCMD_BookOpen : user is changing the books title/author info.

	CItemMessage * pBook = dynamic_cast <CItemMessage *> (uid.ItemFind());
	if ( !m_pChar->CanTouch(pBook) )
	{
		SysMessage("you can't reach it");
		return;
	}
	if ( !pBook->IsBookWritable() )
		return;

	if ( Str_Check(pszTitle) || Str_Check(pszAuthor) )
		return;

	pBook->SetName(pszTitle);
	pBook->m_sAuthor = pszAuthor;
}

void CClient::Event_Book_Page( CGrayUID uid, const CEvent * pEvent ) // Book window
{
	ADDTOCALLSTACK("CClient::Event_Book_Page");
	// XCMD_BookPage : read or write to a book page.

	CItem	*pBook = uid.ItemFind();
	if ( !m_pChar->CanSee(pBook) )
	{
		addObjectRemoveCantSee(uid, "the book");
		return;
	}

	int iPage = pEvent->BookPage.m_page[0].m_pagenum;

	if ( pEvent->BookPage.m_page[0].m_lines == 0xFFFF || pEvent->BookPage.m_len <= 0x0d )
	{
		addBookPage(pBook, iPage);	// just a request for pages.
		return;
	}

	// Trying to write to the book.
	CItemMessage * pText = dynamic_cast <CItemMessage *> (pBook);
	int iLines = minimum(pEvent->BookPage.m_page[0].m_lines, MAX_BOOK_LINES);

	if ( !pText || !pBook->IsBookWritable() )
		return;
	if (( iLines <= 0 ) || ( iPage <= 0 ) || iPage > MAX_BOOK_PAGES )
		return;
	iPage--;

	int		len = 0;
	TCHAR	*pszTemp = Str_GetTemp();
	int		maxlen = pEvent->BookPage.m_len - sizeof(NDWORD) - sizeof(NWORD)*4 - sizeof(BYTE);

	if (( maxlen <= 0 ) || ( maxlen >= SCRIPT_MAX_LINE_LEN ))
		return;

	for ( int i = 0; i < iLines; i++ )
	{
		len += strcpylen(pszTemp+len, pEvent->BookPage.m_page[0].m_text + len);
		pszTemp[len++] = '\t';
	}
	pszTemp[--len] = '\0';

    if ( Str_Check(pszTemp) )
		return;

	pText->SetPageText(iPage, pszTemp);
}

void CClient::Event_Item_Pickup(CGrayUID uid, int amount) // Client grabs an item
{
	ADDTOCALLSTACK("CClient::Event_Item_Pickup");
	EXC_TRY("CClient::Event_Item_Pickup");
	// Player/client is picking up an item.

	EXC_SET("Item");
	CItem	*pItem = uid.ItemFind();
	if ( !pItem || pItem->IsWeird() )
	{
		EXC_SET("Item - addObjectRemove(uid)");
		addObjectRemove(uid);
		EXC_SET("Item - addItemDragCancel(0)");
		addItemDragCancel(0);
		return;
	}

	EXC_SET("FastLoot");
	//	fastloot (,emptycontainer) protection
	if ( m_tNextPickup > m_tNextPickup.GetCurrentTime() )
	{
		EXC_SET("FastLoot - addItemDragCancel(0)");
		addItemDragCancel(0);
		return;
	}
	m_tNextPickup = m_tNextPickup.GetCurrentTime() + 3;

	EXC_SET("origin");
	// Where is the item coming from ? (just in case we have to toss it back)
	CObjBase * pObjParent = dynamic_cast <CObjBase *>(pItem->GetParent());
	m_Targ_PrvUID = ( pObjParent ) ? (DWORD) pObjParent->GetUID() : UID_CLEAR;
	m_Targ_p = pItem->GetUnkPoint();

	EXC_SET("ItemPickup");
	amount = m_pChar->ItemPickup(pItem, amount);
	if ( amount < 0 )
	{
		EXC_SET("ItemPickup - addItemDragCancel(0)");
		addItemDragCancel(0);
		return;
	}
	else if ( amount > 1 )
		m_tNextPickup = m_tNextPickup + 2;	// +100 msec if amount should slow down the client

	EXC_SET("TargMode");
	SetTargMode(CLIMODE_DRAG);
	m_Targ_UID = uid;
	EXC_CATCH;
}

void inline CClient::Event_Item_Drop_Fail( CItem * pItem )
{
	ADDTOCALLSTACK("CClient::Event_Item_Drop_Fail");
	// The item was in the LAYER_DRAGGING.
	// Try to bounce it back to where it came from.
	if ( pItem == NULL )
		return;

	// Game pieces should be returned to their game boards.
	if ( pItem->IsType(IT_GAME_PIECE) )
	{
		CItemContainer *pGame = dynamic_cast<CItemContainer *>( m_Targ_PrvUID.ItemFind() );
		if ( pGame != NULL )
			pGame->ContentAdd( pItem, m_Targ_p );
		else
			pItem->Delete();

		return;
	}

	if ( pItem == m_pChar->LayerFind( LAYER_DRAGGING ) )	// if still being dragged
		m_pChar->ItemBounce( pItem );
}

void CClient::Event_Item_Drop( const CEvent * pEvent ) // Item is dropped
{
	ADDTOCALLSTACK("CClient::Event_Item_Drop");
	// This started from the Event_Item_Pickup()
	if ( !m_pChar )
		return;

	CGrayUID uidItem( pEvent->ItemDropReq.m_UID );
	CItem * pItem = uidItem.ItemFind();
	CGrayUID uidOn;	// dropped on this item.
	unsigned char gridIndex;

	if ( IsClientVersion(0x0600018) || m_reportedCliver >= 0x0600018 || IsClientKR() )
	{
		uidOn = (DWORD)pEvent->ItemDropReqNew.m_UIDCont;
		gridIndex = pEvent->ItemDropReqNew.m_grid;
	}
	else
	{
		uidOn = (DWORD)pEvent->ItemDropReq.m_UIDCont;
		gridIndex = 0;
	}

	CObjBase * pObjOn = uidOn.ObjFind();
	CPointMap  pt( pEvent->ItemDropReq.m_x, pEvent->ItemDropReq.m_y, pEvent->ItemDropReq.m_z, m_pChar->GetTopMap() );

	// Are we out of sync ?
	if ( pItem == NULL ||
		pItem == pObjOn ||	// silliness.
		GetTargMode() != CLIMODE_DRAG ||
		pItem != m_pChar->LayerFind( LAYER_DRAGGING ))
	{
		addItemDragCancel(5);
		return;
	}

	ClearTargMode();	// done dragging

	if ( pObjOn != NULL )	// Put on or in another object
	{
		if ( ! m_pChar->CanTouch( pObjOn ))	// Must also be LOS !
		{
			Event_Item_Drop_Fail( pItem );
			return;
		}

		if ( pObjOn->IsChar())	// Drop on a chars head.
		{
			CChar * pChar = dynamic_cast <CChar*>( pObjOn );
			if ( pChar != m_pChar )
			{
				if ( ! Cmd_SecureTrade( pChar, pItem ))
					Event_Item_Drop_Fail( pItem );
				return;
			}

			// dropped on myself. Get my Pack.
			pObjOn = m_pChar->GetPackSafe();
		}

		// On a container item ?
		CItemContainer * pContItem = dynamic_cast <CItemContainer *>( pObjOn );

		// Is the object on a person ? check the weight.
		CObjBaseTemplate * pObjTop = pObjOn->GetTopLevelObj();
		if ( pObjTop->IsChar())
		{
			CChar * pChar = dynamic_cast <CChar*>( pObjTop );
			ASSERT(pChar);
			if ( ! pChar->NPC_IsOwnedBy( m_pChar ))
			{
				// Slyly dropping item in someone elses pack.
				// or just dropping on their trade window.
				if ( ! Cmd_SecureTrade( pChar, pItem ))
					Event_Item_Drop_Fail( pItem );
				return;
			}
			if ( ! pChar->m_pPlayer )
			{
				// newbie items lose newbie status when transfered to NPC
				pItem->ClrAttr(ATTR_NEWBIE|ATTR_OWNED);
			}
			if ( pChar->GetBank()->IsItemInside( pContItem ))
			{
				// Diff Weight restrict for bank box and items in the bank box.
				if ( ! pChar->GetBank()->CanContainerHold( pItem, m_pChar ))
				{
					Event_Item_Drop_Fail( pItem );
					return;
				}
			}
			else if ( ! pChar->CanCarry( pItem ))
			{
				// SysMessage( "That is too heavy" );
				Event_Item_Drop_Fail( pItem );
				return;
			}
		}
		if ( pContItem != NULL )
		{
			//	bug with shifting selling list by gold coins
			if ( pContItem->IsType(IT_EQ_VENDOR_BOX) &&
				( pItem->IsType(IT_GOLD) || pItem->IsType(IT_COIN) ))
			{
				Event_Item_Drop_Fail( pItem );
				return;
			}
		}

		CObjBase *pOldCont = pItem->GetContainer();
		CScriptTriggerArgs Args( pObjOn );
		if ( pItem->OnTrigger( ITRIG_DROPON_ITEM, m_pChar, &Args ) == TRIGRET_RET_TRUE )
		{
			Event_Item_Drop_Fail( pItem );
			return;
		}

		if ( pOldCont != pItem->GetContainer() )
			return;

		CItem * pItemOn = dynamic_cast <CItem*> ( pObjOn );
		if ( pItemOn )
		{
			CScriptTriggerArgs Args( pItem );
			if ( pItemOn->OnTrigger( ITRIG_DROPON_SELF, m_pChar, &Args ) == TRIGRET_RET_TRUE )
			{
				Event_Item_Drop_Fail( pItem );
				return;
			}
		}

		if ( pContItem != NULL )
		{
			// pChar->GetBank()->IsItemInside( pContItem )
			bool isCheating = false;
			bool isBank = pContItem->IsType( IT_EQ_BANK_BOX );

			if ( isBank )
				isCheating = isBank &&
						pContItem->m_itEqBankBox.m_pntOpen != m_pChar->GetTopPoint();
			else
				isCheating = m_pChar->GetBank()->IsItemInside( pContItem ) &&
						m_pChar->GetBank()->m_itEqBankBox.m_pntOpen != m_pChar->GetTopPoint();

//			g_Log.Event( LOGL_WARN, "%x:IsBank '%d', IsItemInside '%d'\n", m_Socket.GetSocket(), isBank, isBank ? -1 : m_pChar->GetBank()->IsItemInside( pContItem ) );

//			if ( pContItem->IsType( IT_EQ_BANK_BOX ) && pContItem->m_itEqBankBox.m_pntOpen != m_pChar->GetTopPoint() )

			if ( isCheating )
			{
				Event_Item_Drop_Fail( pItem );
				return;
			}
			if ( !pContItem->CanContainerHold(pItem, m_pChar) )
			{
				Event_Item_Drop_Fail( pItem );
				return;
			}

			// only IT_GAME_PIECE can be dropped on IT_GAME_BOARD or clients will crash
			if (pContItem->IsType( IT_GAME_BOARD ) && !pItem->IsType( IT_GAME_PIECE ))
			{
				Event_Item_Drop_Fail( pItem );
				return;
			}

			// non-vendable items should never be dropped inside IT_EQ_VENDOR_BOX
			if ( pContItem->IsType( IT_EQ_VENDOR_BOX ) &&  !pItem->Item_GetDef()->GetMakeValue(0) )
			{
				SysMessageDefault( DEFMSG_ERR_NOT4SALE );
				Event_Item_Drop_Fail( pItem );
				return;
			}
		}
		else
		{
			// dropped on top of a non container item.
			// can i pile them ?
			// Still in same container.

			ASSERT(pItemOn);
			pObjOn = pItemOn->GetContainer();
			pt = pItemOn->GetUnkPoint();

			if ( ! pItem->Stack( pItemOn ))
			{
				if ( pItemOn->IsTypeSpellbook() )
				{
					if ( pItemOn->AddSpellbookScroll( pItem ))
					{
						SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_CANT_ADD_SPELLBOOK ) );
						Event_Item_Drop_Fail( pItem );
						return;
					}
					// We only need to add a sound here if there is no
					// scroll left to bounce back.
					if (pItem->IsDeleted())
						addSound( 0x057, pItemOn );	// add to inv sound.
					Event_Item_Drop_Fail( pItem );
					return; // We can't drop any remaining scrolls
				}

				// Just drop on top of the current item.
				// Client probably doesn't allow this anyhow.
			}
		}
	}
	else
	{
		if ( ! m_pChar->CanTouch( pt ))	// Must also be LOS !
		{
			Event_Item_Drop_Fail( pItem );
			return;
		}
	}

	// Game pieces can only be dropped on their game boards.
	if ( pItem->IsType(IT_GAME_PIECE))
	{
		if ( pObjOn == NULL || m_Targ_PrvUID != pObjOn->GetUID())
		{
			CItemContainer * pGame = dynamic_cast <CItemContainer *>( m_Targ_PrvUID.ItemFind());
			if ( pGame != NULL )
			{
				pGame->ContentAdd( pItem, m_Targ_p );
			}
			else
				pItem->Delete();	// Not sure what else to do with it.
			return;
		}
	}

	// do the dragging anim for everyone else to see.

	if ( pObjOn != NULL )
	{
		// in pack or other CItemContainer.
		m_pChar->UpdateDrag( pItem, pObjOn );
		CItemContainer * pContOn = dynamic_cast <CItemContainer *>(pObjOn);
		ASSERT(pContOn);

		if ( !pContOn )
		{
			if ( pObjOn->IsChar() )
			{
				CChar* pChar = dynamic_cast <CChar*>(pObjOn);
				
				if ( pChar )
					pContOn = pChar->GetBank( LAYER_PACK );
			}

			if ( !pContOn )
			{
				// on ground
				m_pChar->UpdateDrag( pItem, NULL, &pt );
				m_pChar->ItemDrop( pItem, pt );

				return;
			}
		}

		pContOn->ContentAdd( pItem, pt, gridIndex );
		addSound( pItem->GetDropSound( pObjOn ));
	}
	else
	{
		// on ground
		m_pChar->UpdateDrag( pItem, NULL, &pt );
		m_pChar->ItemDrop( pItem, pt );
	}
}



void CClient::Event_Item_Equip( const CEvent * pEvent ) // Item is dropped on paperdoll
{
	ADDTOCALLSTACK("CClient::Event_Item_Equip");
	// This started from the Event_Item_Pickup()

	CGrayUID uidItem( pEvent->ItemEquipReq.m_UID );
	CItem * pItem = uidItem.ItemFind();
	CGrayUID uidChar( pEvent->ItemEquipReq.m_UIDChar );
	CChar * pChar = uidChar.CharFind();
	LAYER_TYPE layer = (LAYER_TYPE)( pEvent->ItemEquipReq.m_layer );

	if ( pItem == NULL ||
		GetTargMode() != CLIMODE_DRAG ||
		pItem != m_pChar->LayerFind( LAYER_DRAGGING ))
	{
		// I have no idea why i got here.
		addItemDragCancel(5);
		return;
	}

	ClearTargMode(); // done dragging.

	if ( pChar == NULL ||
		layer >= LAYER_HORSE )	// Can't equip this way.
	{
		// The item was in the LAYER_DRAGGING.
		m_pChar->ItemBounce( pItem );	// put in pack or drop at our feet.
		return;
	}

	if ( ! pChar->NPC_IsOwnedBy( m_pChar ))
	{
		// trying to equip another char ?
		// can if he works for you.
		// else just give it to him ?
		m_pChar->ItemBounce( pItem ); //cannot equip
		return;
	}

	if ( ! pChar->ItemEquip( pItem, m_pChar ))
	{
		m_pChar->ItemBounce( pItem ); //cannot equip
		return;
	}
}



void CClient::Event_Skill_Locks( const CEvent * pEvent )
{
	ADDTOCALLSTACK("CClient::Event_Skill_Locks");
	// Skill lock buttons in the skills window.
	if ( !GetChar() || !GetChar()->m_pPlayer )
		return;

	int len = pEvent->Skill.m_len;
	len -= 3;

	if ( !len || ((len % sizeof( pEvent->Skill.skills[0] )) != 0) )
		return;

	for ( int i=0; len; i++ )
	{
		SKILL_TYPE index = (SKILL_TYPE)(WORD) pEvent->Skill.skills[i].m_index;
		SKILLLOCK_TYPE state = (SKILLLOCK_TYPE) pEvent->Skill.skills[i].m_lock;

		GetChar()->m_pPlayer->Skill_SetLock( index, state );

		len -= sizeof( pEvent->Skill.skills[0] );
	}
}



void CClient::Event_Skill_Use( SKILL_TYPE skill ) // Skill is clicked on the skill list
{
	ADDTOCALLSTACK("CClient::Event_Skill_Use");
	// All the push button skills come through here.
	// Any "Last skill" macro comes here as well. (push button only)
	if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(skill) )
	{
		SysMessage( "There is no such skill. Please tell support you saw this message.");
		return;
	}

	bool fContinue = false;

	if ( m_pChar->Skill_Wait(skill) )
		return;

	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		if ( m_pChar->Skill_OnTrigger( skill, SKTRIG_SELECT ) == TRIGRET_RET_TRUE )
		{
			m_pChar->Skill_Fail( true );	// clean up current skill.
			return;
		}
	}

	SetTargMode();
	m_Targ_UID.InitUID();	// This is a start point for targ more.

	bool fCheckCrime	= false;
	bool fDoTargeting	= false;

	if ( g_Cfg.IsSkillFlag( skill, SKF_SCRIPTED ) )
	{
		if ( !g_Cfg.GetSkillDef(skill)->m_sTargetPrompt.IsEmpty() )
		{
			m_tmSkillTarg.m_Skill = skill;	// targetting what skill ?
			addTarget( CLIMODE_TARG_SKILL, g_Cfg.GetSkillDef(skill)->m_sTargetPrompt, false, fCheckCrime );
			return;
		}
		else
			m_pChar->Skill_Start( skill );
	}
	else switch ( skill )
	{
		case SKILL_ARMSLORE:
		case SKILL_ITEMID:
		case SKILL_ANATOMY:
		case SKILL_ANIMALLORE:
		case SKILL_EVALINT:
		case SKILL_FORENSICS:
		case SKILL_TASTEID:

		case SKILL_BEGGING:
		case SKILL_TAMING:
		case SKILL_REMOVETRAP:
			fCheckCrime = false;
			fDoTargeting = true;
			break;

		case SKILL_STEALING:
		case SKILL_ENTICEMENT:
		case SKILL_PROVOCATION:
		case SKILL_POISONING:
			// Go into targtting mode.
			fCheckCrime = true;
			fDoTargeting = true;
			break;

		case SKILL_STEALTH:	// How is this supposed to work.
		case SKILL_HIDING:
		case SKILL_SPIRITSPEAK:
		case SKILL_PEACEMAKING:
		case SKILL_DETECTINGHIDDEN:
		case SKILL_MEDITATION:
			// These start/stop automatically.
			m_pChar->Skill_Start(skill);
			return;

		case SKILL_TRACKING:
			Cmd_Skill_Tracking( -1, false );
			break;

		case SKILL_CARTOGRAPHY:
			// Menu select for map type.
			Cmd_Skill_Cartography( 0 );
			break;

		case SKILL_INSCRIPTION:
			// Menu select for spell type.
			Cmd_Skill_Inscription();
			break;

		default:
			SysMessage( "There is no such skill. Please tell support you saw this message.");
			break;
	}
	if ( fDoTargeting )
	{
		// Go into targtting mode.
		if ( g_Cfg.GetSkillDef(skill)->m_sTargetPrompt.IsEmpty() )
		{
			DEBUG_ERR(( "%x: Event_Skill_Use bad skill %d\n", m_Socket.GetSocket(), skill ));
			return;
		}

		m_tmSkillTarg.m_Skill = skill;	// targetting what skill ?
		addTarget( CLIMODE_TARG_SKILL, g_Cfg.GetSkillDef(skill)->m_sTargetPrompt, false, fCheckCrime );
		return;
	}
}



bool CClient::Event_WalkingCheck(DWORD dwEcho)
{
	ADDTOCALLSTACK("CClient::Event_WalkingCheck");
	// look for the walk code, and remove it
	// Client will send 0's if we have not given it any EXTDATA_WalkCode_Prime message.
	// The client will use codes in order.
	// But it will skip codes sometimes. so delete codes that get skipped.

	// RETURN:
	//  true = ok to walk.
	//  false = the client is cheating. I did not send that code.

	if ( IsNoCryptLessVer(0x126000) )
		return( true );

	if ( IsClientLessVer(0x126000) )
		return( true );

	if ( ! ( g_Cfg.m_wDebugFlags & DEBUGF_WALKCODES ))
		return( true );

	// If the LIFO stack has not been sent, send them out now
	if ( m_Walk_CodeQty < 0 )
	{
		addWalkCode(EXTDATA_WalkCode_Prime,COUNTOF(m_Walk_LIFO));
	}

	// Keep track of echo'd 0's and invalid non 0's
	// (you can get 1 to 4 of these legitimately when you first start walking)
	ASSERT( m_Walk_CodeQty >= 0 );

	for ( int i=0; i<m_Walk_CodeQty; i++ )
	{
		if ( m_Walk_LIFO[i] == dwEcho )	// found a good code.
		{
			// Move next to the head.
			i++;
			memmove( m_Walk_LIFO, m_Walk_LIFO+i, (m_Walk_CodeQty-i)*sizeof(DWORD));
			m_Walk_CodeQty -= i;
			// Set this to negative so we know later if we've gotten at least 1 valid echo
			m_Walk_InvalidEchos = -1;
			return( true );
		}
	}

	if ( m_Walk_InvalidEchos < 0 )
	{
		// If we're here, we got at least one valid echo....therefore
		// we should never get another one
		DEBUG_ERR(( "%x: Invalid walk echo (0%x). Invalid after valid.\n", m_Socket.GetSocket(), dwEcho ));
		addPlayerWalkCancel();
		return false;
	}

	// Increment # of invalids received. This is allowed at startup.
	// These "should" always be 0's
	if ( ++ m_Walk_InvalidEchos >= COUNTOF(m_Walk_LIFO))
	{
		// The most I ever got was 2 of these, but I've seen 4
		// a couple of times on a real server...we might have to
		// increase this # if it becomes a problem (but I doubt that)
		DEBUG_ERR(( "%x: Invalid walk echo. Too many initial invalid.\n", m_Socket.GetSocket()));
		addPlayerWalkCancel();
		return false;
	}

	// Allow them to walk a bit till we catch up.
	return true;
}



void CClient::Event_Walking( BYTE rawdir, BYTE count, DWORD dwEcho ) // Player moves
{
	ADDTOCALLSTACK("CClient::Event_Walking");
	// XCMD_Walk
	// The theory....
	// The client sometimes echos 1 or 2 zeros or invalid echos when you first start
	//	walking (the invalid non zeros happen when you log off and don't exit the
	//	client.exe all the way and then log back in, XXX doesn't clear the stack)

	if ( !m_pChar || !Event_WalkingCheck(dwEcho) )
		return;

	// Movement whilst precasting is not allowed
	if ( IsSetMagicFlags( MAGICF_PRECAST ) && CChar::IsSkillMagic(m_pChar->m_Act_SkillCurrent) )
	{
		const CSpellDef* pSpellDef = g_Cfg.GetSpellDef(m_pChar->m_atMagery.m_Spell);
		if (pSpellDef != NULL && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST))
		{
			SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_FROZEN ) );
			addPlayerWalkCancel();
			return;
		}
	}

	if (( m_pChar->IsStatFlag(STATF_Freeze|STATF_Stone) && m_pChar->OnFreezeCheck() ) || m_pChar->OnFreezeCheck(true) )
	{
		addPlayerWalkCancel();
		return;
	}

	if ( count != (BYTE)( m_wWalkCount+1 ))	// && count != 255
	{
		if ( (WORD)(m_wWalkCount) == 0xFFFF )
			return;	// just playing catch up with last reset. don't cancel again.
		addPlayerWalkCancel();
		return;
	}

	m_timeLastEventWalk = CServTime::GetCurrentTime();

	bool fRun = ( rawdir & 0x80 ); // or flying ?

	m_pChar->StatFlag_Mod( STATF_Fly, fRun );

	DIR_TYPE dir = (DIR_TYPE)( rawdir & 0x0F );
	if ( dir >= DIR_QTY )
	{
		addPlayerWalkCancel();
		return;
	}
	CPointMap pt = m_pChar->GetTopPoint();
	CPointMap ptold = pt;
	bool	fMove = true;
	bool	fUpdate	= false;

	if ( dir == m_pChar->m_dirFace )
	{
		LONGLONG	CurrTime	= GetTickCount();
		m_iWalkStepCount++;
		// Move in this dir.
		if ( ( m_iWalkStepCount % 7 ) == 0 )	// we have taken 8 steps ? direction changes don't count. (why we do this check also for gm?)
		{
			// Client only allows 4 steps of walk ahead.
			if ( g_Cfg.m_iWalkBuffer )
			{
				int		iTimeDiff	= ((CurrTime - m_timeWalkStep)/10);
				int		iTimeMin;
				if (m_pChar->m_pPlayer)
				{
					switch (m_pChar->m_pPlayer->m_speedMode)
					{
						case 0: // Normal Speed
							iTimeMin = m_pChar->IsStatFlag( STATF_OnHorse )? 70 : 140; // it should check of walking (80 - 160)
							break;
						case 1: // Foot=Double Speed, Mount=Normal
							iTimeMin = 70;
							break;
						case 2: // Foot=Always Walk, Mount=Always Walk (Half Speed)
							iTimeMin = m_pChar->IsStatFlag( STATF_OnHorse )? 140 : 280;
							break;
						case 3: // Foot=Always Run, Mount=Always Walk
						default:
							iTimeMin = 140;
							break;
					}
				}
				else
					iTimeMin = m_pChar->IsStatFlag( STATF_OnHorse ) ? 70 : 140;

				if ( iTimeDiff > iTimeMin )
				{
					int	iRegen	= ((iTimeDiff - iTimeMin) * g_Cfg.m_iWalkRegen) / 150;
					if ( iRegen > g_Cfg.m_iWalkBuffer )
						iRegen	= g_Cfg.m_iWalkBuffer;
					else if ( iRegen < -((g_Cfg.m_iWalkBuffer * g_Cfg.m_iWalkRegen) / 100) )
						iRegen	= -((g_Cfg.m_iWalkBuffer * g_Cfg.m_iWalkRegen) / 100);
					iTimeDiff	= iTimeMin + iRegen;
				}

				m_iWalkTimeAvg		+= iTimeDiff;

				int	oldAvg	= m_iWalkTimeAvg;
				m_iWalkTimeAvg	-= iTimeMin;

				if ( m_iWalkTimeAvg > g_Cfg.m_iWalkBuffer )
					m_iWalkTimeAvg	= g_Cfg.m_iWalkBuffer;
				else if ( m_iWalkTimeAvg < -g_Cfg.m_iWalkBuffer )
					m_iWalkTimeAvg	= -g_Cfg.m_iWalkBuffer;

				if ( IsPriv( PRIV_DETAIL ) && IsPriv( PRIV_DEBUG ) )
				{
					SysMessagef( "Walkcheck trace: %i / %i (%i) :: %i", iTimeDiff, iTimeMin, oldAvg, m_iWalkTimeAvg );
				}

				if ( m_iWalkTimeAvg < 0 && iTimeDiff >= 0 && ! IsPriv(PRIV_GM) )	// TICK_PER_SEC
				{
					// walking too fast.
					DEBUG_WARN(("%s (%x): Fast Walk ?\n", GetName(), m_Socket.GetSocket()));

					TRIGRET_TYPE iAction = TRIGRET_RET_DEFAULT;
					if ( !IsSetEF(EF_Minimize_Triggers) )
					{
						iAction = m_pChar->OnTrigger( CTRIG_UserExWalkLimit, m_pChar, NULL );
					}
					m_iWalkStepCount--; // eval again next time !

					if ( iAction != TRIGRET_RET_TRUE )
					{
						addPlayerWalkCancel();
						return;
					}
				}
			}
			m_timeWalkStep = CurrTime;
		}	// nth step

		pt.Move(dir);

		// Before moving, check if we were indoors
		bool fRoof = m_pChar->IsStatFlag( STATF_InDoors );

		// Check the z height here.
		// The client already knows this but doesn't tell us.
#ifdef _DIAGONALWALKCHECK_PLAYERWALKONLY
		if ( !m_pChar->CanMoveWalkTo(pt, true, false, dir, false, true) )
#else
		if ( !m_pChar->CanMoveWalkTo(pt, true, false, dir) )
#endif
		{
			addPlayerWalkCancel();
			return;
		}

		// Are we invis ?
		m_pChar->CheckRevealOnMove();

		if (!m_pChar->MoveToChar( pt ))
		{
			addPlayerWalkCancel();
			return;
		}
//		m_pChar->MoveToChar( pt );

		// Now we've moved, are we now or no longer indoors and need to update weather?
		if ( fRoof != m_pChar->IsStatFlag( STATF_InDoors ))
		{
			addWeather( WEATHER_DEFAULT );
		}

		// did i step on a telepad, trap, etc ?
		if ( m_pChar->CheckLocation())
		{
			// We stepped on teleporter
			return;
		}
	}

	if ( dir != m_pChar->m_dirFace )		// Just a change in dir.
	{
		m_pChar->m_dirFace = dir;
		fMove = false;
	}

	// Ack the move. ( if this does not go back we get rubber banding )
	m_wWalkCount = count;	// m_wWalkCount++
	CCommand cmd;
	cmd.WalkAck.m_Cmd = XCMD_WalkAck;
	cmd.WalkAck.m_count = (BYTE) m_wWalkCount;
	cmd.WalkAck.m_noto = m_pChar->Noto_GetFlag( m_pChar, false, IsClientVersion(MINCLIVER_NOTOINVUL) );
	xSendPkt( &cmd, sizeof( cmd.WalkAck ));

	if ( !fMove )
		m_pChar->UpdateMode(this);			// Show others I have turned !!
	else
	{
		m_pChar->UpdateMove( ptold, this );	// Who now sees me ?
		addPlayerSee( ptold );				// What new stuff do I now see ?
	}
}



void CClient::Event_CombatMode( bool fWar ) // Only for switching to combat mode
{
	ADDTOCALLSTACK("CClient::Event_CombatMode");
	// If peacmaking then this doens't work ??
	// Say "you are feeling too peacefull"
	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = m_pChar->IsStatFlag(STATF_War) ? 1 : 0;
		if (m_pChar->OnTrigger(CTRIG_UserWarmode, m_pChar, &Args) == TRIGRET_RET_TRUE)
			return;
	}

	m_pChar->StatFlag_Mod( STATF_War, fWar );

	if ( m_pChar->IsStatFlag( STATF_DEAD ))
	{
		// Manifest the ghost.
		// War mode for ghosts.
		m_pChar->StatFlag_Mod( STATF_Insubstantial, ! fWar );
	}

	m_pChar->Skill_Fail( true );	// clean up current skill.
	if ( ! fWar )
	{
		m_pChar->Fight_ClearAll();
	}

	addPlayerWarMode();
	m_pChar->UpdateMode( this, m_pChar->IsStatFlag( STATF_DEAD ));
}

bool CClient::Event_Command(LPCTSTR pszCommand, TALKMODE_TYPE mode)
{
	ADDTOCALLSTACK("CClient::Event_Command");
	if ( mode == 13 || mode == 14 ) // guild and alliance don't pass this.
		return false;
	if ( pszCommand[0] == 0 )
		return true;		// should not be said
	if ( Str_Check(pszCommand) )
		return true;		// should not be said
	if ( ( ( m_pChar->GetID() == 0x3db ) && ( pszCommand[0] == '=' ) ) || ( pszCommand[0] == g_Cfg.m_cCommandPrefix ) )
	{
		// Lazy :P
	}
	else
		return false;

	if ( !strcmpi(pszCommand, "q") && ( GetPrivLevel() > PLEVEL_Player ))
	{
		SysMessage("Probably you forgot about Ctrl?");
		return true;
	}

	bool m_bAllowCommand = true;
	bool m_bAllowSay = true;

	pszCommand += 1;
	GETNONWHITESPACE(pszCommand);
	m_bAllowCommand = g_Cfg.CanUsePrivVerb(this, pszCommand, this);

	if ( !m_bAllowCommand )
		m_bAllowSay = ( GetPrivLevel() <= PLEVEL_Player );

	//	filter on commands is active - so trigger it
	if ( !g_Cfg.m_sCommandTrigger.IsEmpty() )
	{
		CScriptTriggerArgs Args(pszCommand);
		Args.m_iN1 = m_bAllowCommand;
		Args.m_iN2 = m_bAllowSay;
		enum TRIGRET_TYPE tr;

		//	Call the filtering function
		if ( m_pChar->r_Call(g_Cfg.m_sCommandTrigger, m_pChar, &Args, NULL, &tr) )
			if ( tr == TRIGRET_RET_TRUE )
				return Args.m_iN2;

		m_bAllowCommand = Args.m_iN1;
		m_bAllowSay = Args.m_iN2;
	}

	if ( !m_bAllowCommand && !m_bAllowSay )
		SysMessage("You can't use this command.");

	if ( m_bAllowCommand )
	{
		m_bAllowSay = false;

		// Assume you don't mean yourself !
		if ( FindTableHeadSorted( pszCommand, sm_szCmd_Redirect, COUNTOF(sm_szCmd_Redirect)) >= 0 )
		{
			// targetted verbs are logged once the target is selected.
			addTargetVerb(pszCommand, "");
		}
		else
		{
			CScript s(pszCommand);
			if ( !m_pChar->r_Verb(s, m_pChar) )
				SysMessageDefault(DEFMSG_CMD_INVALID);
		}
	}

	if ( GetPrivLevel() >= g_Cfg.m_iCommandLog )
		g_Log.Event( LOGM_GM_CMDS, "%x:'%s' commands '%s'=%d\n", m_Socket.GetSocket(), (LPCTSTR) GetName(), (LPCTSTR) pszCommand, m_bAllowCommand);

	return !m_bAllowSay;
}

void CClient::Event_Attack( CGrayUID uid )
{
	ADDTOCALLSTACK("CClient::Event_Attack");
	// d-click in war mode
	// I am attacking someone.

	CChar * pChar = uid.CharFind();
	if ( pChar == NULL )
		return;

	// Accept or decline the attack.
	CCommand cmd;
	cmd.AttackOK.m_Cmd = XCMD_AttackOK;
	cmd.AttackOK.m_UID = (m_pChar->Fight_Attack( pChar )) ? (DWORD) pChar->GetUID() : 0;
	xSendPkt( &cmd, sizeof( cmd.AttackOK ));
}

// Client/Player buying items from the Vendor

inline void CClient::Event_VendorBuy_Cheater( int iCode )
{
	ADDTOCALLSTACK("CClient::Event_VendorBuy_Cheater");

	// iCode descriptions
	static LPCTSTR const sm_BuyPacketCheats[] =
	{
		"Other",
		"Bad vendor UID",
		"Vendor is off-duty",
		"Bad item UID",
		"Requested items out of stock",
		"Total cost is too great",
	};

	g_Log.Event(LOGL_WARN|LOGM_CHEAT, "%x:Cheater '%s' is submitting illegal buy packet (%s)\n", this->m_Socket.GetSocket(),
		this->GetAccount()->GetName(),
		sm_BuyPacketCheats[iCode]);
	SysMessage("You cannot buy that.");
}

void CClient::Event_VendorBuy(CGrayUID uidVendor, const CEvent *pEvent)
{
	ADDTOCALLSTACK("CClient::Event_VendorBuy");
	if ( !pEvent->VendorBuy.m_flag )	// just a close command.
		return;

	CChar	*pVendor = uidVendor.CharFind();
	bool	bPlayerVendor = pVendor->IsStatFlag(STATF_Pet);

	if ( !pVendor || ( !pVendor->NPC_IsVendor() && pVendor->m_pNPC->m_Brain != NPCBRAIN_VENDOR_OFFDUTY ) )
	{
		Event_VendorBuy_Cheater( 0x1 );
		return;
	}

	if ( !m_pChar->CanTouch(pVendor) )
	{
		SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_CANTREACH));
		return;
	}

	if ( pVendor->m_pNPC->m_Brain == NPCBRAIN_VENDOR_OFFDUTY ) //cheaters
	{
		pVendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_OFFDUTY));
		Event_VendorBuy_Cheater( 0x2 );
		return;
	}

	CItemContainer	*pContStock = pVendor->GetBank(LAYER_VENDOR_STOCK);
	CItemContainer	*pPack = m_pChar->GetPackSafe();

	int			iConvertFactor = pVendor->NPC_GetVendorMarkup(m_pChar);

	DWORD	BuyUids[MAX_ITEMS_CONT];
	DWORD	BuyAmounts[MAX_ITEMS_CONT];
	DWORD	BuyPrices[MAX_ITEMS_CONT];
	memset(BuyUids, 0, sizeof(BuyUids));
	memset(BuyAmounts, 0, sizeof(BuyAmounts));
	memset(BuyPrices, 0, sizeof(BuyPrices));

#define	MAX_COST (INT_MAX/2)
	int		costtotal=0;
	unsigned int nItems = minimum((pEvent->VendorBuy.m_len - 8)/sizeof(pEvent->VendorBuy.m_item[0]), MAX_ITEMS_CONT);
	int		i;

	CVarDefCont *vardef = g_Cfg.m_bAllowBuySellAgent ? NULL : m_TagDefs.GetKey("BUYSELLTIME");

	if ( vardef )
	{
		CServTime	allowsell;
		allowsell.InitTime(vardef->GetValNum() + ( nItems*3 ));
		if ( g_World.GetCurrentTime() < allowsell )
		{
			SysMessage("You are buying too fast.");
			return;
		}
	}

	CItemVendable * pItem = NULL;

	//	Step #1
	//	Combine all goods into one list.
	for ( i = 0 ; i < nItems ; ++i )
	{
		DWORD			d = pEvent->VendorBuy.m_item[i].m_UID;
		int				index;
		CGrayUID		uid(d);
		pItem = dynamic_cast <CItemVendable *> (uid.ItemFind());

		if ( !pItem || !pItem->IsValidSaleItem(true) )
		{
			Event_VendorBuy_Cheater( 0x3 );
			return;
		}

		//	search if it is already in the list
		for ( index = 0; index < nItems; index++ )
		{
			if ( d == BuyUids[index] )
				break;
			else if ( !BuyUids[index] )	// zero-entry (empty)
			{
				BuyUids[index] = d;
				BuyPrices[index] = pItem->GetVendorPrice(iConvertFactor);
				break;
			}
		}
		BuyAmounts[index] += pEvent->VendorBuy.m_item[i].m_amount;

		if ( ( BuyPrices[index] <= 0 ) || !pContStock || !pContStock->IsItemInside(pItem) )
		{
			pVendor->Speak("Alas, I don't have these goods currently stocked. Let me know if there is something else thou wouldst buy.");
			Event_VendorBuy_Cheater( 0x4 );
			return;
		}
	}

	//	Step #2
	//	Check if the vendor really has so much items
	for ( i = 0; i < nItems; ++i )
	{
		if ( BuyUids[i] )
		{
			CGrayUID		uid(BuyUids[i]);
			pItem = dynamic_cast <CItemVendable *> (uid.ItemFind());

			if (( BuyAmounts[i] <= 0 ) || ( BuyAmounts[i] > pItem->GetAmount() ))
			{
				pVendor->Speak("Alas, I don't have all those goods in stock. Let me know if there is something else thou wouldst buy.");
				Event_VendorBuy_Cheater( 0x4 );
				return;
			}

			costtotal += BuyAmounts[i] * BuyPrices[i];
			if ( costtotal > MAX_COST )
			{
				pVendor->Speak("Alas, I am not allowed to operate such huge sums.");
				Event_VendorBuy_Cheater( 0x5 );
				return;
			}
		}
	}

	//	Step #3
	//	Check for gold being enough to buy this
	bool fBoss = pVendor->NPC_IsOwnedBy(m_pChar);
	if ( !fBoss )
	{
		if ( ( g_Cfg.m_fPayFromPackOnly ) ?
				m_pChar->GetPackSafe()->ContentConsume(RESOURCE_ID(RES_TYPEDEF,IT_GOLD), costtotal, true) :
				m_pChar->ContentConsume(RESOURCE_ID(RES_TYPEDEF,IT_GOLD), costtotal, true)
			)
		{
			pVendor->Speak("Alas, thou dost not possess sufficient gold for this purchase!");
			return;
		}
	}

	if ( costtotal <= 0 )
	{
		pVendor->Speak("You have bought nothing. But feel free to browse");
		return;
	}

	//	Step #4
	//	Move the items bought into your pack.
	for ( i = 0; i < nItems; ++i )
	{
		if ( !BuyUids[i] )
			break;

		CGrayUID		uid(BuyUids[i]);
		pItem = dynamic_cast <CItemVendable *> (uid.ItemFind());

		WORD amount = BuyAmounts[i];

		if ( IsSetEF(EF_New_Triggers) )
		{
			CScriptTriggerArgs Args( amount, BuyAmounts[i] * BuyPrices[i], pVendor );
			Args.m_VarsLocal.SetNum( "TOTALCOST", costtotal);
			if ( pItem->OnTrigger( ITRIG_Buy, this->GetChar(), &Args ) == TRIGRET_RET_TRUE )
				continue;
		}

		if ( !bPlayerVendor )									//	NPC vendors
		{
			pItem->SetAmount(pItem->GetAmount() - amount);

			switch ( pItem->GetType() )
			{
				case IT_FIGURINE:
					{
						for ( int f = 0; f < amount; f++ )
							m_pChar->Use_Figurine(pItem, 2);
					}
					goto do_consume;
				case IT_BEARD:
					if (( m_pChar->GetDispID() != CREID_MAN ) && ( m_pChar->GetDispID() != CREID_EQUIP_GM_ROBE ))
					{
						pVendor->Speak("Sorry, I cannot do anything for you.");
						continue;
					}
				case IT_HAIR:
					// Must be added directly. can't exist in pack!
					if ( ! m_pChar->IsHuman())
					{
						pVendor->Speak("Sorry, I cannot do anything for you.");
						continue;
					}
					{
						CItem * pItemNew = CItem::CreateDupeItem( pItem );
						m_pChar->LayerAdd(pItemNew);
						pItemNew->m_TagDefs.SetNum("NOSAVE", 0, true);
						pItemNew->SetTimeout( 55000*TICK_PER_SEC );	// set the grow timer.
						pVendor->UpdateAnimate(ANIM_ATTACK_1H_WIDE);
						m_pChar->Sound( SOUND_SNIP );	// snip noise.
					}
					continue;
			}

			if ( amount > 1 && !pItem->Item_GetDef()->IsStackableType() )
			{
				while ( amount -- )
				{
					CItem * pItemNew = CItem::CreateDupeItem(pItem);
					pItemNew->SetAmount(1);
					pItemNew->m_TagDefs.SetNum("NOSAVE", 0, true);
					if ( !pPack->CanContainerHold( pItemNew, m_pChar ) || !m_pChar->CanCarry( pItemNew ) )
						m_pChar->ItemDrop( pItemNew, m_pChar->GetTopPoint() );
					else
						pPack->ContentAdd( pItemNew );
				}
			}
			else
			{
				CItem * pItemNew = CItem::CreateDupeItem(pItem);
				pItemNew->SetAmount(amount);
				pItemNew->m_TagDefs.SetNum("NOSAVE", 0, true);
				if ( !pPack->CanContainerHold( pItemNew, m_pChar ) || !m_pChar->CanCarry( pItemNew ) )
					m_pChar->ItemDrop( pItemNew, m_pChar->GetTopPoint() );
				else
					pPack->ContentAdd( pItemNew );
			}
		}
		else													// Player vendors
		{
			if ( pItem->GetAmount() <= amount )		// buy the whole item
			{
				if ( !pPack->CanContainerHold( pItem, m_pChar ) || !m_pChar->CanCarry( pItem ) )
					m_pChar->ItemDrop( pItem, m_pChar->GetTopPoint() );
				else
					pPack->ContentAdd( pItem );

				pItem->m_TagDefs.SetNum("NOSAVE", 0, true);
			}
			else
			{
				pItem->SetAmount(pItem->GetAmount() - amount);

				CItem *pItemNew = CItem::CreateDupeItem(pItem);
				pItemNew->m_TagDefs.SetNum("NOSAVE", 0, true);
				pItemNew->SetAmount(amount);
				if ( !pPack->CanContainerHold( pItemNew, m_pChar ) || !m_pChar->CanCarry( pItemNew ) )
					m_pChar->ItemDrop( pItemNew, m_pChar->GetTopPoint() );
				else
					pPack->ContentAdd( pItemNew );
			}
		}

do_consume:
		pItem->Update();
	}

	//	Step #5
	//	Say the message about the bought goods
	TCHAR *sMsg = Str_GetTemp();
	TCHAR *pszTemp1 = Str_GetTemp();
	TCHAR *pszTemp2 = Str_GetTemp();
	sprintf(pszTemp1, g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_HYARE), m_pChar->GetName());
	sprintf(pszTemp2, fBoss ? g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_S1) : g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_B1),
		costtotal, (costtotal==1) ? "" : g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_CA));
	sprintf(sMsg, "%s %s %s", pszTemp1, pszTemp2, g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_TY));
	pVendor->Speak(sMsg);

	//	Step #6
	//	Take the gold and add it to the vendor
	if ( !fBoss )
	{
		int rc = ( g_Cfg.m_fPayFromPackOnly ) ?
			m_pChar->GetPackSafe()->ContentConsume( RESOURCE_ID(RES_TYPEDEF,IT_GOLD), costtotal) :
			m_pChar->ContentConsume( RESOURCE_ID(RES_TYPEDEF,IT_GOLD), costtotal);
		pVendor->GetBank()->m_itEqBankBox.m_Check_Amount += costtotal;
	}

	//	Clear the vendor display.
	addVendorClose(pVendor);

	if ( i )	// if anything was sold, sound this
		addSound( 0x057 );
}

inline void CClient::Event_VendorSell_Cheater( int iCode )
{
	ADDTOCALLSTACK("CClient::Event_VendorSell_Cheater");

	// iCode descriptions
	static LPCTSTR const sm_SellPacketCheats[] =
	{
		"Other",
		"Bad vendor UID",
		"Vendor is off-duty",
		"Bad item UID",
	};

	g_Log.Event(LOGL_WARN|LOGM_CHEAT, "%x:Cheater '%s' is submitting illegal sell packet (%s)\n", this->m_Socket.GetSocket(),
		this->GetAccount()->GetName(),
		sm_SellPacketCheats[iCode]);
	SysMessage("You cannot sell that.");
}

void CClient::Event_VendorSell( CGrayUID uidVendor, const CEvent * pEvent )
{
	ADDTOCALLSTACK("CClient::Event_VendorSell");
	// Player Selling items to the vendor.
	// Done with the selling action.

	CChar * pVendor = uidVendor.CharFind();
	if ( !pVendor || ( !pVendor->NPC_IsVendor() && pVendor->m_pNPC->m_Brain != NPCBRAIN_VENDOR_OFFDUTY ) )
	{
		Event_VendorSell_Cheater( 0x1 );
		return;
	}

	if ( !m_pChar->CanTouch(pVendor) )
	{
		SysMessage("You can't reach the vendor");
		return;
	}

	if ( pVendor->m_pNPC->m_Brain == NPCBRAIN_VENDOR_OFFDUTY ) //cheaters
	{
		pVendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_OFFDUTY));
		Event_VendorSell_Cheater( 0x2 );
		return;
	}

	if ( pEvent->VendorSell.m_count < 1 )
	{
		addVendorClose(pVendor);
		return;
	}

				//	selling list has same container limitations as all others
	if ( pEvent->VendorSell.m_count >= MAX_ITEMS_CONT )
	{
		SysMessage("You cannot sell so much.");
		return;
	}

	CVarDefCont *vardef = g_Cfg.m_bAllowBuySellAgent ? NULL : m_TagDefs.GetKey("BUYSELLTIME");

	if ( vardef )
	{
		CServTime	allowsell;
		allowsell.InitTime(vardef->GetValNum() + ( pEvent->VendorSell.m_count*3 ));
		if ( g_World.GetCurrentTime() < allowsell )
		{
			SysMessage("You are selling too fast.");
			return;
		}
	}

	CItemContainer	*pBank = pVendor->GetBank();
	CItemContainer	*pContStock = pVendor->GetBank( LAYER_VENDOR_STOCK );
	CItemContainer	*pContBuy = pVendor->GetBank( LAYER_VENDOR_BUYS );
	CItemContainer	*pContExtra = pVendor->GetBank( LAYER_VENDOR_EXTRA );
	if ( pBank == NULL || pContStock == NULL )
	{
		addVendorClose(pVendor);
		pVendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_GUARDS));
		return;
	}

	int iConvertFactor = -pVendor->NPC_GetVendorMarkup(m_pChar);

	int iGold = 0;
	bool fShortfall = false;

	for ( int i = 0; i < pEvent->VendorSell.m_count ; ++i )
	{
		CGrayUID uid( pEvent->VendorSell.m_item[i].m_UID );
		CItemVendable * pItem = dynamic_cast <CItemVendable *> (uid.ItemFind());
		if ( !pItem || !pItem->IsValidSaleItem(true) )
		{
			Event_VendorSell_Cheater( 0x3 );
			return;
		}

		// Do we still have it ? (cheat check)
		if ( pItem->GetTopLevelObj() != m_pChar )
			continue;

		if ( IsSetEF(EF_New_Triggers) )
		{
			CScriptTriggerArgs Args( pItem->GetAmount(), 0, pVendor );
			if ( pItem->OnTrigger( ITRIG_Sell, this->GetChar(), &Args ) == TRIGRET_RET_TRUE )
				continue;
		}

		// Find the valid sell item from vendors stuff.
		CItemVendable * pItemSell = CChar::NPC_FindVendableItem( pItem, pContBuy, pContStock );
		if ( pItemSell == NULL )
			continue;

		// Now how much did i say i wanted to sell ?
		int amount = pEvent->VendorSell.m_item[i].m_amount;
		if ( pItem->GetAmount() < amount )	// Selling more than i have ?
		{
			amount = pItem->GetAmount();
		}

		long iPrice = pItemSell->GetVendorPrice(iConvertFactor) * amount;

		// Can vendor afford this ?
		if ( iPrice > pBank->m_itEqBankBox.m_Check_Amount )
		{
			fShortfall = true;
			break;
		}
		pBank->m_itEqBankBox.m_Check_Amount -= iPrice;

		// give them the appropriate amount of gold.
		iGold += iPrice;

		// Take the items from player.
		// Put items in vendor inventory.
		if ( amount >= pItem->GetAmount())
		{
			pItem->RemoveFromView();
			if ( pVendor->IsStatFlag(STATF_Pet) && pContExtra )
				pContExtra->ContentAdd(pItem);
			else
				pItem->Delete();
		}
		else
		{
			if ( pVendor->IsStatFlag(STATF_Pet) && pContExtra )
			{
				CItem * pItemNew = CItem::CreateDupeItem(pItem);
				pItemNew->SetAmount(amount);
				pContExtra->ContentAdd(pItemNew);
			}
			pItem->SetAmountUpdate( pItem->GetAmount() - amount );
		}
	}

	if ( iGold )
	{
		char	*z = Str_GetTemp();
		sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_SELL_TY),
			iGold, (iGold==1) ? "" : g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_CA));
		pVendor->Speak(z);

		if ( fShortfall )
			pVendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_NOMONEY));

		m_pChar->AddGoldToPack(iGold);
		addVendorClose(pVendor);
	}
	else
	{
		if ( fShortfall )
			pVendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_CANTAFFORD));
	}
}

void CClient::Event_BBoardRequest( CGrayUID uid, const CEvent * pEvent )
{
	ADDTOCALLSTACK("CClient::Event_BBoardRequest");
	// Answer a request reguarding the BBoard.
	// addBulletinBoard

	CItemContainer * pBoard = dynamic_cast <CItemContainer *> ( uid.ItemFind());
	if ( ! m_pChar->CanSee( pBoard ))
	{
		addObjectRemoveCantSee( uid, "the board" );
		return;
	}

	if ( !pBoard->IsType(IT_BBOARD) )
		return;

	CGrayUID uidMsg( (DWORD)( pEvent->BBoard.m_UIDMsg ) );

	switch ( pEvent->BBoard.m_flag )
	{
		case BBOARDF_REQ_FULL:
		case BBOARDF_REQ_HEAD:
			// request for message header and/or body.
			if ( pEvent->BBoard.m_len != 0x0c )
			{
				DEBUG_ERR(( "%x:BBoard feed back message bad length %d\n", m_Socket.GetSocket(), (int) pEvent->BBoard.m_len ));
				return;
			}
			if ( ! addBBoardMessage( pBoard, (BBOARDF_TYPE) pEvent->BBoard.m_flag, uidMsg ))
			{
				// sanity check fails.
				addObjectRemoveCantSee( (DWORD)( pEvent->BBoard.m_UIDMsg ), "the message" );
				return;
			}
			break;
		case BBOARDF_NEW_MSG:
			{
				// Submit a message
				if ( pEvent->BBoard.m_len < 0x0c )
					return;
				if ( !m_pChar->CanTouch(pBoard) )
				{
					SysMessageDefault(DEFMSG_ITEMUSE_BBOARD_REACH);
					return;
				}
				if ( pBoard->GetCount() > 32 )
				{
					// Role a message off.
					delete pBoard->GetAt(pBoard->GetCount()-1);
				}

				int lenstr = pEvent->BBoard.m_data[0];
				if ( Str_Check((LPCTSTR)&pEvent->BBoard.m_data[1]) )
					return;

				// if pMsgItem then this is a reply to it !
				CItemMessage * pMsgNew = dynamic_cast <CItemMessage *>( CItem::CreateBase( ITEMID_BBOARD_MSG ));
				if ( !pMsgNew )
				{
					DEBUG_ERR(( "%x:BBoard can't create message item\n", m_Socket.GetSocket()));
					return;
				}

				pMsgNew->SetAttr( ATTR_MOVE_NEVER );
				pMsgNew->SetName((LPCTSTR)&pEvent->BBoard.m_data[1]);
				pMsgNew->m_itBook.m_Time = CServTime::GetCurrentTime();
				pMsgNew->m_sAuthor = m_pChar->GetName();
				pMsgNew->m_uidLink = m_pChar->GetUID();	// Link it to you forever.

				int len = 1 + lenstr;
				int lines = pEvent->BBoard.m_data[len++];
				if ( lines > 32 ) lines = 32;	// limit this.

				while ( lines-- )
				{
					lenstr = pEvent->BBoard.m_data[len++];
					if ( !Str_Check((LPCTSTR) &pEvent->BBoard.m_data[len]) )
						pMsgNew->AddPageText( (LPCTSTR) &pEvent->BBoard.m_data[len] );
					len += lenstr;
				}

				pBoard->ContentAdd( pMsgNew );
			}
			break;

		case BBOARDF_DELETE:
			// remove the msg. (if it is yours?)
			{
				CItemMessage * pMsgItem = dynamic_cast <CItemMessage *> ( uidMsg.ItemFind());
				if ( ! pBoard->IsItemInside( pMsgItem ))
				{
					SysMessageDefault( DEFMSG_ITEMUSE_BBOARD_COR );
					return;
				}
				if ( ! IsPriv(PRIV_GM) && pMsgItem->m_uidLink != m_pChar->GetUID())
				{
					SysMessageDefault( DEFMSG_ITEMUSE_BBOARD_DEL );
					return;
				}

				pMsgItem->Delete();
			}
			break;

		default:
			DEBUG_ERR(( "%x:BBoard unknown flag %d\n", m_Socket.GetSocket(), (int) pEvent->BBoard.m_flag ));
			return;
	}
}



void CClient::Event_SecureTrade( CGrayUID uid, const CEvent * pEvent )
{
	ADDTOCALLSTACK("CClient::Event_SecureTrade");
	// pressed a button on the secure trade window.

	CItemContainer * pCont = dynamic_cast <CItemContainer *> ( uid.ItemFind());
	if ( pCont == NULL )
		return;
	if ( m_pChar != pCont->GetParent())
		return;

	// perform the trade.
	switch ( pEvent->SecureTrade.m_action )
	{
		case SECURE_TRADE_CLOSE: // Cancel trade.  Send each person cancel messages, move items.
			pCont->Delete();
			return;
		case SECURE_TRADE_CHANGE: // Change check marks.  Possibly conclude trade
			if ( m_pChar->GetDist( pCont ) > UO_MAP_VIEW_SIZE )
			{
				// To far away.
				SysMessageDefault( DEFMSG_TRADE_TOOFAR );
				return;
			}
			long need2wait(0);
			CVarDefCont *vardef = pCont->GetTagDefs()->GetKey("wait1sec");
			if ( vardef ) need2wait = vardef->GetValNum();
			if ( need2wait > 0 )
			{
				long timerow = g_World.GetCurrentTime().GetTimeRaw();
				if ( need2wait > timerow )
				{
					TCHAR *pszMsg = Str_GetTemp();
					long		seconds = (need2wait-timerow)/TICK_PER_SEC;
					sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_TRADE_WAIT ), seconds);
					SysMessage(pszMsg);
					return;
				}
			}
			pCont->Trade_Status( pEvent->SecureTrade.m_UID1 );
			return;
	}
}



void CClient::Event_Profile( BYTE fWriteMode, CGrayUID uid, const CEvent * pEvent )
{
	ADDTOCALLSTACK("CClient::Event_Profile");
	// mode = 0 = Get profile, 1 = Set profile

	CChar	*pChar = uid.CharFind();
	if ( !pChar || !pChar->m_pPlayer )
		return;

	if ( pChar->OnTrigger(CTRIG_Profile, m_pChar) == TRIGRET_RET_TRUE )
		return;

	if ( fWriteMode )
	{
		// write stuff to the profile.
		if ( m_pChar != pChar )
		{
			if ( ! IsPriv(PRIV_GM))
				return;
			if ( m_pChar->GetPrivLevel() < pChar->GetPrivLevel())
				return;
		}

		const int iSizeAll = sizeof(pEvent->CharProfile);
		const int iSizeTxt = sizeof(pEvent->CharProfile.m_utext);

		int len = pEvent->CharProfile.m_len;
		if ( len <= (sizeof(pEvent->CharProfile)-sizeof(pEvent->CharProfile.m_utext)))
			return;

		int iTextLen = pEvent->CharProfile.m_textlen;
		if ( iTextLen*sizeof(NCHAR) != len - (sizeof(pEvent->CharProfile)-sizeof(pEvent->CharProfile.m_utext)) )
			return;

		BYTE retcode = pEvent->CharProfile.m_retcode;	// 0=canceled, 1=okayed or something similar???

		TCHAR szLine[SCRIPT_MAX_LINE_LEN-16];
		int iWLen = CvtNUNICODEToSystem( szLine, COUNTOF(szLine), pEvent->CharProfile.m_utext, iTextLen );
		if ( szLine && !strchr(szLine, 0x0A) )
			pChar->m_pPlayer->m_sProfile = szLine;
	}
	else
	{
		// alter profile when viewing an incognitoed player, unless being viewed by a GM or the profile is our own
		bool fIncognito = pChar->IsStatFlag( STATF_Incognito ) && ! IsPriv(PRIV_GM) && (pChar != m_pChar);

		CGString sConstText;
		CCommand cmd;

		cmd.CharProfile.m_Cmd = XCMD_CharProfile;
		cmd.CharProfile.m_UID = uid;

		int len = strcpylen(cmd.CharProfile.m_title, pChar->GetName()) + 1;

		if (fIncognito == false)
			sConstText.Format( "%s, %s", pChar->Noto_GetTitle(), pChar->GetTradeTitle());
		else
			sConstText.Format( "%s", pChar->Noto_GetTitle());

		int iWLen = CvtSystemToNUNICODE(
			(NCHAR *) ( cmd.CharProfile.m_title + len ), 1024,
			sConstText, sConstText.GetLength() );
		len += (iWLen+1)*sizeof(NCHAR);

		LPCTSTR pszProfile = fIncognito ? "" : ((LPCTSTR)pChar->m_pPlayer->m_sProfile);
		iWLen = CvtSystemToNUNICODE(
			(NCHAR *) ( cmd.CharProfile.m_title + len ), SCRIPT_MAX_LINE_LEN-16,
			pszProfile, -1 );
		len += (iWLen+1)*sizeof(NCHAR);
		len += 7;

		cmd.CharProfile.m_len = len;
		xSendPkt(&cmd, len);
	}
}



void CClient::Event_MailMsg( CGrayUID uid1, CGrayUID uid2 )
{
	ADDTOCALLSTACK("CClient::Event_MailMsg");
	// NOTE: How do i protect this from spamming others !!!
	// Drag the mail bag to this clients char.

	CChar * pChar = uid1.CharFind();
	if ( pChar == NULL )
	{
		SysMessageDefault( DEFMSG_MAILBAG_DROP_1 );
		return;
	}

	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		if (pChar->OnTrigger(CTRIG_UserMailBag, m_pChar, NULL) == TRIGRET_RET_TRUE)
			return;
	}

	if ( pChar == m_pChar ) // this is normal (for some reason) at startup.
	{
		return;
	}
	// Might be an NPC ?
	TCHAR *pszMsg = Str_GetTemp();
	sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_MAILBAG_DROP_2 ), (LPCTSTR) m_pChar->GetName());
	pChar->SysMessage(pszMsg);
}



void CClient::Event_ToolTip( CGrayUID uid )
{
	ADDTOCALLSTACK("CClient::Event_ToolTip");
	CObjBase * pObj = uid.ObjFind();
	if ( pObj == NULL )
		return;

	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		if ( pObj->OnTrigger("@ToolTip", this) == TRIGRET_RET_TRUE )	// CTRIG_ToolTip, ITRIG_ToolTip
			return;
	}

	char *z = Str_GetTemp();
	sprintf(z, "'%s'", pObj->GetName());
	addToolTip(uid.ObjFind(), z);
}

void CClient::Event_PromptResp( LPCTSTR pszText, int len )
{
	ADDTOCALLSTACK("CClient::Event_PromptResp");
	// result of addPrompt
	TCHAR szText[MAX_TALK_BUFFER];

	if ( Str_Check( pszText ) )
		return;

	CLIMODE_TYPE PrvTargMode = GetTargMode();
	ClearTargMode();

	if ( len <= 0 )	// cancel
		szText[0] = 0;
	else
	{
		if ( PrvTargMode == CLIMODE_PROMPT_SCRIPT_VERB )
			len = Str_GetBare( szText, pszText, sizeof(szText), "|~=[]{|}~" );
		else
			len = Str_GetBare( szText, pszText, sizeof(szText), "|~,=[]{|}~" );
	}

	LPCTSTR pszReName = NULL;
	LPCTSTR pszPrefix = NULL;

	switch ( PrvTargMode )
	{
		case CLIMODE_PROMPT_GM_PAGE_TEXT:
			// m_Targ_Text
			Cmd_GM_Page( szText );
			return;

		case CLIMODE_PROMPT_VENDOR_PRICE:
			// Setting the vendor price for an item.
			{
				if ( szText[0] == '\0' )	// cancel
					return;
				CChar * pCharVendor = m_Targ_PrvUID.CharFind();
				if ( pCharVendor )
				{
					pCharVendor->NPC_SetVendorPrice( m_Targ_UID.ItemFind(), ATOI(szText) );
				}
			}
			return;

		case CLIMODE_PROMPT_NAME_RUNE:
			pszReName = g_Cfg.GetDefaultMsg(DEFMSG_RUNE_NAME);
			//pszPrefix = "Rune to:";
			pszPrefix = g_Cfg.GetDefaultMsg(DEFMSG_RUNE_TO);
			break;

		case CLIMODE_PROMPT_NAME_KEY:
			pszReName = g_Cfg.GetDefaultMsg(DEFMSG_KEY_NAME);
			//pszPrefix = "Key to:";
			pszPrefix = g_Cfg.GetDefaultMsg(DEFMSG_KEY_TO);
			break;

		case CLIMODE_PROMPT_NAME_SHIP:
			pszReName = "Ship";
			pszPrefix = "SS ";
			break;

		case CLIMODE_PROMPT_NAME_SIGN:
			pszReName = "Sign";
			pszPrefix = "";
			break;

		case CLIMODE_PROMPT_STONE_NAME:
			//pszReName = "Stone";
			pszReName = g_Cfg.GetDefaultMsg(DEFMSG_STONE_NAME);
			pszPrefix = g_Cfg.GetDefaultMsg(DEFMSG_STONE_FOR);
			//pszPrefix = "Stone for the ";
			break;

		case CLIMODE_PROMPT_STONE_SET_ABBREV:
			pszReName = "Abbreviation";
			pszPrefix = "";
			break;

		case CLIMODE_PROMPT_STONE_GRANT_TITLE:
		case CLIMODE_PROMPT_STONE_SET_TITLE:
			pszReName = "Title";
			pszPrefix = "";
			break;

		case CLIMODE_PROMPT_TARG_VERB:
			// Send a msg to the pre-tergetted player. "ETARGVERB"
			// m_Targ_UID = the target.
			// m_Targ_Text = the prefix.
			if ( szText[0] != '\0' )
			{
				CObjBase * pObj = m_Targ_UID.ObjFind();
				if ( pObj )
				{
					CScript script( m_Targ_Text, szText );
					pObj->r_Verb( script, this );
				}
			}
			return;

		case CLIMODE_PROMPT_SCRIPT_VERB:
			{
				// CChar * pChar = m_Targ_PrvUID.CharFind();
				CScript script( m_Targ_Text, szText );
				if ( m_pChar )
					m_pChar->r_Verb( script, this );
			}
			return;

		default:
			// DEBUG_ERR(( "%x:Unrequested Prompt mode %d\n", m_Socket.GetSocket(), PrvTargMode ));
			SysMessage( "Unexpected prompt info" );
			return;
	}

	ASSERT(pszReName);

	CGString sMsg;
	CItem * pItem = m_Targ_UID.ItemFind();

	if ( pItem == NULL || szText[0] == '\0' )
	{
		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_RENAME_CANCEL ), pszReName );
		return;
	}

	if ( g_Cfg.IsObscene( szText ))
	{
		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_RENAME_WNAME ), pszReName, szText );
		return;
	}

	sMsg.Format("%s%s", pszPrefix, szText);
	switch (pItem->GetType())
	{
#ifndef _NEWGUILDSYSTEM
		case IT_STONE_GUILD:
		case IT_STONE_TOWN:
			{
				CItemStone * pStone = dynamic_cast <CItemStone*> ( pItem );
				if ( !pStone || !pStone->OnPromptResp(this, PrvTargMode, szText, sMsg) )
					return;
			}
			break;
#endif

		default:
			pItem->SetName(sMsg);
			sMsg.Format(g_Cfg.GetDefaultMsg( DEFMSG_RENAME_SUCCESS ), pszReName, (LPCTSTR) pItem->GetName());
			break;
	}

	SysMessage(sMsg);
}




void CClient::Event_Talk_Common(char *szText) // PC speech
{
	ADDTOCALLSTACK("CClient::Event_Talk_Common");
	// ??? Allow NPC's to talk to each other in the future.
	// Do hearing here so there is not feedback loop with NPC's talking to each other.
	if ( !m_pChar || !m_pChar->m_pPlayer || !m_pChar->m_pArea )
		return;

	if ( ! strnicmp( szText, "I resign from my guild", 22 ))
	{
		m_pChar->Guild_Resign(MEMORY_GUILD);
		return;
	}
	if ( ! strnicmp( szText, "I resign from my town", 21 ))
	{
		m_pChar->Guild_Resign(MEMORY_TOWN);
		return;
	}

	static LPCTSTR const sm_szTextMurderer[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_TEXT_MURD_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_TEXT_MURD_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_TEXT_MURD_3 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_TEXT_MURD_4 ),
	};

	if ( ! strnicmp( szText, "I must consider my sins", 23 ))
	{
		int i = m_pChar->m_pPlayer->m_wMurders;
		if ( i >= COUNTOF(sm_szTextMurderer))
			i = COUNTOF(sm_szTextMurderer)-1;
		SysMessage( sm_szTextMurderer[i] );
		return;
	}

	// Guards are special
	// They can't hear u if your dead.
	bool fGhostSpeak = m_pChar->IsSpeakAsGhost();
	if ( ! fGhostSpeak && ( FindStrWord( szText, "GUARD" ) || FindStrWord( szText, "GUARDS" )))
	{
		m_pChar->CallGuards(NULL);
	}

	// Are we in a region that can hear ?
	if ( m_pChar->m_pArea->GetResourceID().IsItem())
	{
		CItemMulti * pItemMulti = dynamic_cast <CItemMulti *>( m_pChar->m_pArea->GetResourceID().ItemFind());
		if ( pItemMulti )
			pItemMulti->OnHearRegion( szText, m_pChar );
	}

	// Are there items on the ground that might hear u ?
	CSector * pSector = m_pChar->GetTopSector();
	if ( pSector->HasListenItems())
	{
		pSector->OnHearItem( m_pChar, szText );
	}

	// Find an NPC that may have heard us.
	CChar * pCharAlt = NULL;
	int iAltDist = UO_MAP_VIEW_SIGHT;
	CChar * pChar;
	int i=0;

	CWorldSearch AreaChars( m_pChar->GetTopPoint(), UO_MAP_VIEW_SIGHT );
	while (true)
	{
		pChar = AreaChars.GetChar();
		if ( !pChar )
			break;

		if ( pChar->IsStatFlag(STATF_COMM_CRYSTAL))
		{
			CItem	*pItemNext, *pItem = pChar->GetContentHead();
			for ( ; pItem ; pItem = pItemNext )
			{
				pItemNext = pItem->GetNext();
				pItem->OnHear(szText, m_pChar);
			}
		}

		if ( pChar == m_pChar )
			continue;

		if ( fGhostSpeak && ! pChar->CanUnderstandGhost())
			continue;

		bool fNamed = false;
		i = 0;
		if ( ! strnicmp( szText, "PETS", 4 ))
			i = 5;
		else if ( ! strnicmp( szText, "ALL ", 4 ))
			i = 4;
		else
		{
			// Named the char specifically ?
			i = pChar->NPC_OnHearName( szText );
			fNamed = true;
		}
		if ( i )
		{
			while ( ISWHITESPACE( szText[i] ))
				i++;

			if ( pChar->NPC_OnHearPetCmd( szText+i, m_pChar, !fNamed ))
			{
				if ( fNamed )
					return;
				if ( GetTargMode() == CLIMODE_TARG_PET_CMD )
					return;
				// The command might apply to other pets.
				continue;
			}
			if ( fNamed )
				break;
		}

		// Are we close to the char ?
		int iDist = m_pChar->GetTopDist3D( pChar );

		if ( pChar->Skill_GetActive() == NPCACT_TALK &&
			pChar->m_Act_Targ == m_pChar->GetUID()) // already talking to him
		{
			pCharAlt = pChar;
			iAltDist = 1;
		}
		else if ( pChar->IsClient() && iAltDist >= 2 )	// PC's have higher priority
		{
			pCharAlt = pChar;
			iAltDist = 2;	// high priority
		}
		else if ( iDist < iAltDist )	// closest NPC guy ?
		{
			pCharAlt = pChar;
			iAltDist = iDist;
		}

		// NPC's with special key words ?
		if ( pChar->m_pNPC )
		{
			if ( pChar->m_pNPC->m_Brain == NPCBRAIN_BANKER )
			{
				if ( FindStrWord( szText, "BANK" ))
					break;
			}
		}
	}

	if ( pChar == NULL )
	{
		i = 0;
		pChar = pCharAlt;
		if ( pChar == NULL )
			return;	// no one heard it.
	}

	// Change to all upper case for ease of search. ???
	_strupr( szText );

	// The char hears you say this.
	pChar->NPC_OnHear( &szText[i], m_pChar );
}




void CClient::Event_Talk( LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, bool bNoStrip) // PC speech
{
	ADDTOCALLSTACK("CClient::Event_Talk");
	if ( !GetAccount() || !GetChar() )
		return;

	if ( mode < 0 || mode > 14 ) // Less or greater is an exploit
		return;

	// These modes are server->client only
	if ( mode == 1 || mode == 3 || mode == 4 || mode == 5 || mode == 6 || mode == 7 || mode == 10 || mode == 11 || mode == 12 )
		return;

	if (( wHue < 2 ) || ( wHue > 0x03e9 ))
		wHue = HUE_TEXT_DEF;

	// store the language of choice.
	GetAccount()->m_lang.Set( NULL );	// default.

	// Rip out the unprintables first.
	TCHAR szText[MAX_TALK_BUFFER];
	int len;

	if ( bNoStrip )
	{
		// The characters in Unicode speech don't need to be filtered
		strncpy( szText, pszText, MAX_TALK_BUFFER - 1 );
		len = strlen( szText );
	}
	else
	{
		TCHAR szTextG[MAX_TALK_BUFFER];
		strncpy( szTextG, pszText, MAX_TALK_BUFFER - 1 );
		len = Str_GetBare( szText, szTextG, sizeof(szText)-1 );
	}

	if ( len <= 0 )
		return;

	pszText = szText;
	GETNONWHITESPACE(pszText);

	if ( !Event_Command(pszText,mode) )
	{
		bool	fCancelSpeech	= false;
		char	z[MAX_TALK_BUFFER];

		if ( m_pChar->OnTriggerSpeech(false, (TCHAR *)pszText, m_pChar, mode, wHue) )
			fCancelSpeech	= true;

		if ( g_Log.IsLoggedMask(LOGM_PLAYER_SPEAK) )
		{
			g_Log.Event( LOGM_PLAYER_SPEAK, "%x:'%s' Says '%s' mode=%d%s\n",
				m_Socket.GetSocket(), m_pChar->GetName(), pszText, mode, fCancelSpeech ? " (muted)" : "");
		}

		// Guild and Alliance mode will not pass this.
		if ( mode == 13 || mode == 14 )
			return;

		strcpy(z, pszText);

		if ( g_Cfg.m_fSuppressCapitals )
		{
			int chars = strlen(z);
			int capitals = 0;
			int i = 0;
			for ( i = 0; i < chars; i++ )
				if (( z[i] >= 'A' ) && ( z[i] <= 'Z' ))
					capitals++;

			if (( chars > 5 ) && ((( capitals * 100 )/chars) > 75 ))
			{							// 80% of chars are in capital letters. lowercase it
				for ( i = 1; i < chars; i++ )				// instead of the 1st char
					if (( z[i] >= 'A' ) && ( z[i] <= 'Z' )) z[i] += 0x20;
			}
		}

		if ( !fCancelSpeech && ( len <= 128 ) ) // From this point max 128 chars
		{
			m_pChar->SpeakUTF8(z, wHue, (TALKMODE_TYPE)mode, m_pChar->m_fonttype, GetAccount()->m_lang);
			Event_Talk_Common((char *)z);
		}
	}
}

void CClient::Event_TalkUNICODE( const CEvent * pEvent )
{
	ADDTOCALLSTACK("CClient::Event_TalkUNICODE");
	// Get the text in wide bytes.
	// ENU = English
	// FRC = French
	// mode == TALKMODE_SYSTEM if coming from player talking.

	CAccount * pAccount = GetAccount();
	if ( !pAccount )	// this should not happen
		return;

	unsigned char	mMode	= pEvent->TalkUNICODE.m_mode;
	HUE_TYPE wHue = pEvent->TalkUNICODE.m_wHue;
	int iLen = pEvent->TalkUNICODE.m_len - sizeof(pEvent->TalkUNICODE);
	if ( iLen <= 0 )
		return;

	bool bHasKeywords = ( mMode & 0xc0 ) != 0;
	if ( bHasKeywords )
	{
		mMode &= ~0xc0;
		BYTE * pKeywords = (BYTE*)(&pEvent->TalkUNICODE.m_utext);

		int count = (UNPACKWORD(pKeywords) & 0xFFF0) >> 4;
		if ( count < 0 || count > 50 )	// malformed check
			return;

		count += 1;
		count *= 12;

		int toskip = count / 8;
		if ( count % 8 > 0 )
			toskip++;

		if ( toskip > (iLen * 2) )	// malformed check
			return;

		Event_Talk( (const char *)(pKeywords + toskip), wHue, (TALKMODE_TYPE)mMode, true );
		return;
	}

	TALKMODE_TYPE	Mode	= (TALKMODE_TYPE) mMode;

	if ( mMode < 0 || mMode > 14 ) // Less or greater is an exploit
		return;

	// These modes are server->client only
	if ( mMode == 1 || mMode == 3 || mMode == 4 || mMode == 5 || mMode == 6 || mMode == 7 || mMode == 10 || mMode == 11 || mMode == 12 )
		return;

	if (( wHue < 0 ) || ( wHue > 0x03e9 ))
		wHue = HUE_TEXT_DEF;

	// store the default language of choice. CLanguageID
	pAccount->m_lang.Set( pEvent->TalkUNICODE.m_lang );

	int iLenChars = iLen/sizeof(WCHAR);
	TCHAR szText[MAX_TALK_BUFFER];
   	NWORD wszText[MAX_TALK_BUFFER];
	LPCTSTR pszText;
	const NWORD * puText;

	if ( iLenChars <= 0 )
		return;

	if ( IsSetEF( EF_UNICODE ) )
	{
		wcsncpy( (wchar_t *)wszText, (wchar_t *)&pEvent->TalkUNICODE.m_utext[0], MAX_TALK_BUFFER - 2 );
   		iLen = CvtNUNICODEToSystem( szText, sizeof(szText), wszText, iLenChars );
		puText	= wszText;
	}
	else
	{
		puText = &pEvent->TalkUNICODE.m_utext[0];
		iLen = CvtNUNICODEToSystem( szText, sizeof(szText), pEvent->TalkUNICODE.m_utext, iLenChars );
	}

	if ( iLen <= 0 )
		return;

	pszText = szText;
	GETNONWHITESPACE(pszText);

	if ( !Event_Command(pszText,Mode) )
	{
		bool	fCancelSpeech	= false;

		if ( m_pChar->OnTriggerSpeech( false, (TCHAR *) pszText, m_pChar, Mode, wHue) )
			fCancelSpeech	= true;

		if ( g_Log.IsLoggedMask(LOGM_PLAYER_SPEAK) )
		{
			g_Log.Event(LOGM_PLAYER_SPEAK, "%x:'%s' Says UNICODE '%s' '%s' mode=%d%s\n", m_Socket.GetSocket(),
				m_pChar->GetName(), pAccount->m_lang.GetStr(), pszText, Mode, fCancelSpeech ? " (muted)" : "" );
		}

		// Guild and Alliance mode will not pass this.
		if ( Mode == 13 || Mode == 14 )
			return;

		if ( IsSetEF(EF_UNICODE) )
		{
			if ( g_Cfg.m_fSuppressCapitals )
			{
				int chars = strlen(szText);
				int capitals = 0;
				int i = 0;
				for ( i = 0; i < chars; i++ )
					if (( szText[i] >= 'A' ) && ( szText[i] <= 'Z' ))
						capitals++;

				if (( chars > 5 ) && ((( capitals * 100 )/chars) > 75 ))
				{							// 80% of chars are in capital letters. lowercase it
					for ( i = 1; i < chars; i++ )				// instead of the 1st char
						if (( szText[i] >= 'A' ) && ( szText[i] <= 'Z' ))
							szText[i] += 0x20;

					CvtSystemToNUNICODE(wszText, iLenChars, szText, chars);
				}
			}
		}

		if ( !fCancelSpeech && ( iLen <= 128 ) ) // From this point max 128 chars
		{
			m_pChar->SpeakUTF8Ex(puText, wHue, Mode, m_pChar->m_fonttype, pAccount->m_lang);
			Event_Talk_Common((char *)pszText);
		}
	}
}

bool CClient::Event_DeathOption( DEATH_MODE_TYPE mode, const CEvent * pEvent )
{
	ADDTOCALLSTACK("CClient::Event_DeathOption");
	if ( m_pChar == NULL )
		return false;
	if ( mode != DEATH_MODE_MANIFEST )
	{
		// Play as a ghost.
		SysMessage("You are a ghost");
		addSound(0x17f);
		addPlayerStart(m_pChar); // Do practically a full reset (to clear death menu)
		return true;
	}

	// Toggle manifest mode. (this has more data) (anomoly to size rules)
	if ( !xCheckMsgSize(sizeof(pEvent->DeathMenu)) )
		return false;

	Event_CombatMode(pEvent->DeathMenu.m_manifest);
	return true;
}


void CClient::Event_SetName( CGrayUID uid, const char * pszCharName )
{
	ADDTOCALLSTACK("CClient::Event_SetName");
	// Set the name in the character status window.
	CChar * pChar = uid.CharFind();
	if ( !pChar || !m_pChar )
		return;

   if ( Str_CheckName(pszCharName) || !strlen(pszCharName) )
		return;

	// Do we have the right to do this ?
	if ( m_pChar == pChar || ! pChar->NPC_IsOwnedBy( m_pChar, true ))
		return;
	if ( FindTableSorted( pszCharName, sm_szCmd_Redirect, COUNTOF(sm_szCmd_Redirect) ) >= 0 )
		return;
	if ( FindTableSorted( pszCharName, CCharNPC::sm_szVerbKeys, 14 ) >= 0 )
		return;
	if ( g_Cfg.IsObscene(pszCharName))
		return;

	CScriptTriggerArgs args;
	args.m_pO1 = pChar;
	args.m_s1 = pszCharName;
	if ( m_pChar->OnTrigger(CTRIG_Rename, this, &args) == TRIGRET_RET_TRUE )
		return;

	pChar->SetName(pszCharName);
}


void CClient::Event_MenuChoice( const CEvent * pEvent ) // Choice from GMMenu or Itemmenu received
{
	ADDTOCALLSTACK("CClient::Event_MenuChoice");
	// Select from a menu. CMenuItem
	// result of addItemMenu call previous.
	// select = 0 = cancel.

	CGrayUID uidItem( pEvent->MenuChoice.m_UID );
	WORD context = pEvent->MenuChoice.m_context;

	if ( context != GetTargMode() || uidItem != m_tmMenu.m_UID )
	{
		// DEBUG_ERR(( "%x: Menu choice unrequested %d!=%d\n", m_Socket.GetSocket(), context, m_Targ_Mode ));
		SysMessage( "Unexpected menu info" );
		return;
	}

	ClearTargMode();
	WORD select = pEvent->MenuChoice.m_select;

	// Item Script or GM menu script got us here.
	switch ( context )
	{
	case CLIMODE_MENU:
		// A generic menu from script.
		Menu_OnSelect( m_tmMenu.m_ResourceID, select, uidItem.ObjFind() );
		return;
	case CLIMODE_MENU_SKILL:
		// Some skill menu got us here.
		if ( select >= COUNTOF(m_tmMenu.m_Item))
			return;
		Cmd_Skill_Menu( m_tmMenu.m_ResourceID, (select) ? m_tmMenu.m_Item[select] : 0 );
		return;
	case CLIMODE_MENU_SKILL_TRACK_SETUP:
		// PreTracking menu got us here.
		Cmd_Skill_Tracking( select, false );
		return;
	case CLIMODE_MENU_SKILL_TRACK:
		// Tracking menu got us here. Start tracking the selected creature.
		Cmd_Skill_Tracking( select, true );
		return;

	case CLIMODE_MENU_GM_PAGES:
		// Select a GM page from the menu.
		Cmd_GM_PageSelect( select );
		return;
	case CLIMODE_MENU_EDIT:
		// m_Targ_Text = what are we doing to it ?
		Cmd_EditItem( uidItem.ObjFind(), select );
		return;
	default:
		DEBUG_ERR(( "%x:Unknown Targetting mode for menu %d\n", m_Socket.GetSocket(), context ));
		return;
	}
}



void CClient::Event_GumpInpValRet( const CEvent * pEvent )
{
	ADDTOCALLSTACK("CClient::Event_GumpInpValRet");
	// Text was typed into the gump on the screen.
	// pEvent->GumpInpValRet
	// result of addGumpInputBox. GumpInputBox
	// ARGS:
	// 	m_Targ_UID = pObj->GetUID();
	//  m_Targ_Text = verb

	CGrayUID uidItem( pEvent->GumpInpValRet.m_UID );
	WORD context = pEvent->GumpInpValRet.m_context;	// word context is odd.

	BYTE retcode = pEvent->GumpInpValRet.m_retcode; // 0=canceled, 1=okayed
	WORD textlen = pEvent->GumpInpValRet.m_textlen; // length of text entered
	LPCTSTR pszText = pEvent->GumpInpValRet.m_text;

	TCHAR *		pFix;
	if ( ( pFix = const_cast<TCHAR*>(strchr( pszText, '\n' )) ) )
		*pFix	= '\0';
	if ( ( pFix = const_cast<TCHAR*>(strchr( pszText, '\r' )) ) )
		*pFix	= '\0';
	if ( ( pFix = const_cast<TCHAR*>(strchr( pszText, '\t' )) ) )
		*pFix	= ' ';

	if ( GetTargMode() != CLIMODE_INPVAL || uidItem != m_Targ_UID )
	{
		// DEBUG_ERR(( "%x:Event_GumpInpValRetIn unexpected input %d!=%d\n", m_Socket.GetSocket(), context, GetTargMode()));
		SysMessage( "Unexpected text input" );
		return;
	}

	ClearTargMode();

	CObjBase * pObj = uidItem.ObjFind();
	if ( pObj == NULL )
		return;

	// take action based on the parent context.
	if (retcode == 1)	// ok
	{
		// Properties Dialog, page x
		// m_Targ_Text = the verb we are dealing with.
		// m_Prop_UID = object we are after.

		CScript script( m_Targ_Text, pszText );
		bool fRet = pObj->r_Verb( script, m_pChar );
		if ( ! fRet )
		{
			SysMessagef( "Invalid set: %s = %s", (LPCTSTR) m_Targ_Text, (LPCTSTR) pszText );
		}
		else
		{
			if ( IsPriv( PRIV_DETAIL ))
			{
				SysMessagef( "Set: %s = %s", (LPCTSTR) m_Targ_Text, (LPCTSTR) pszText );
			}
			pObj->RemoveFromView(); // weird client thing
			pObj->Update();
		}

		g_Log.Event( LOGM_GM_CMDS, "%x:'%s' tweak uid=0%x (%s) to '%s %s'=%d\n",
			m_Socket.GetSocket(), (LPCTSTR) GetName(),
			(DWORD) pObj->GetUID(), (LPCTSTR) pObj->GetName(),
			(LPCTSTR) m_Targ_Text, (LPCTSTR) pszText, fRet );
	}
}

bool CDialogResponseArgs::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CDialogResponseArgs::r_WriteVal");
	EXC_TRY("WriteVal");
	if ( ! strnicmp( pszKey, "ARGCHK", 6 ))
	{
		// CGTypedArray <DWORD,DWORD> m_CheckArray;
		pszKey += 6;
		SKIP_SEPARATORS(pszKey);

		int iQty = m_CheckArray.GetCount();
		if ( pszKey[0] == '\0' )
		{
			sVal.FormatVal(iQty);
			return( true );
		}
		else if ( ! strnicmp( pszKey, "ID", 2) )
		{
			pszKey += 2;

			if ( iQty && m_CheckArray[0] )
				sVal.FormatVal( m_CheckArray[0] );
			else
				sVal.FormatVal( -1 );

			return( true );
		}

		int iNum = Exp_GetSingle( pszKey );
		SKIP_SEPARATORS(pszKey);
		for ( int i=0; i<iQty; i++ )
		{
			if ( iNum == m_CheckArray[i] )
			{
				sVal = "1";
				return( true );
			}
		}
		sVal = "0";
		return( true );
	}
	if ( ! strnicmp( pszKey, "ARGTXT", 6 ))
	{
		pszKey += 6;
		SKIP_SEPARATORS(pszKey);

		int iQty = m_TextArray.GetCount();
		if ( pszKey[0] == '\0' )
		{
			sVal.FormatVal(iQty);
			return( true );
		}

		int iNum = Exp_GetSingle( pszKey );
		SKIP_SEPARATORS(pszKey);

		for ( int i=0; i<m_TextArray.GetCount(); i++ )
		{
			if ( iNum == m_TextArray[i]->m_ID )
			{
				sVal = m_TextArray[i]->m_sText;
				return( true );
			}
		}
		sVal.Empty();
		return( false );
	}
	return CScriptTriggerArgs::r_WriteVal( pszKey, sVal, pSrc);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

void CClient::Event_GumpDialogRet( const CEvent * pEvent )
{
	ADDTOCALLSTACK("CClient::Event_GumpDialogRet");
	// CLIMODE_DIALOG
	// initiated by addGumpDialog()
	// A button was pressed in a gump on the screen.
	// possibly multiple check boxes.

	// First let's completely decode this packet
	CGrayUID	uid		( pEvent->GumpDialogRet.m_UID );
	DWORD	context		= pEvent->GumpDialogRet.m_context;
	DWORD dwButtonID	= pEvent->GumpDialogRet.m_buttonID;

	// relying on the context given by the gump might be a security problem, much like
	// relying on the uid returned.
	// maybe keep a memory for each gump?
	CObjBase * pObj = uid.ObjFind();

	// Virtue button -- Handleing this here because the packet is a little different and causes exceptions somewhere
	if ( ( context == CLIMODE_DIALOG_VIRTUE ) && ( (CObjBase *)m_pChar == pObj ) )
	{
		if ( !IsSetEF(EF_Minimize_Triggers) )
		{
			CChar *pCharViewed = m_pChar;
			if ( dwButtonID == 1 && pEvent->GumpDialogRet.m_checkQty > 0 )
			{
				// when button id is 1, player is trying to view character uid sent as the first check id
				pCharViewed = CGrayUID(pEvent->GumpDialogRet.m_checkIds[0]).CharFind();
				if ( pCharViewed == NULL )
					pCharViewed = m_pChar;
			}

			CScriptTriggerArgs Args(pCharViewed);
			Args.m_iN1 = dwButtonID;

			m_pChar->OnTrigger( CTRIG_UserVirtue, (CTextConsole *) m_pChar, &Args );
		}
		return;
	}

#ifdef _DEBUG
	{
		CResourceDef *	pRes = g_Cfg.ResourceGetDef(RESOURCE_ID( RES_DIALOG, context ));
		if ( !pRes )
			g_Log.Event(LOGL_EVENT, "Gump: %d (%s), Uid: 0x%x, Button: %d.\n", context, "undef", (DWORD)uid, dwButtonID);
		else
		{
			CDialogDef * pDlg = dynamic_cast <CDialogDef*>(pRes);
			if ( !pDlg )
				g_Log.Event(LOGL_EVENT, "Gump: %d (%s), Uid: 0x%x, Button: %d.\n", context, "undef", (DWORD)uid, dwButtonID);
			else
				g_Log.Event(LOGL_EVENT, "Gump: %d (%s), Uid: 0x%x, Button: %d.\n", context, (LPCTSTR) pDlg->GetName(), (DWORD)uid, dwButtonID);
		}
	}
#endif

	// Sanity check
	OpenedGumpsMap_t::iterator itGumpFound = m_mapOpenedGumps.find( ((int)(context)) );
	if (( itGumpFound != m_mapOpenedGumps.end() ) && ( (*itGumpFound).second > 0 ))
	{
		(*itGumpFound).second--;
	}
	else
	{
		return;
	}

	// package up the gump response info.
	CDialogResponseArgs resp;

	DWORD iCheckQty = pEvent->GumpDialogRet.m_checkQty; // this has the total of all checked boxes and radios
	int i = 0;
	for ( ; i < iCheckQty; i++ ) // Store the returned checked boxes' ids for possible later use
	{
		resp.m_CheckArray.Add( pEvent->GumpDialogRet.m_checkIds[i] );
	}

	// Find out how many textentry boxes we have that returned data
	CEvent * pMsg = (CEvent *)(((BYTE*)(pEvent))+(iCheckQty-1)*sizeof(pEvent->GumpDialogRet.m_checkIds[0]));
	DWORD iTextQty = pMsg->GumpDialogRet.m_textQty;
	for ( i = 0; i < iTextQty; i++)
	{
		// Get the length....no need to store this permanently
		int lenstr = pMsg->GumpDialogRet.m_texts[0].m_len;

		TCHAR *scratch = Str_GetTemp(); // use this as scratch storage

		// Do a loop and "convert" from unicode to normal ascii
		CvtNUNICODEToSystem( scratch, SCRIPT_MAX_LINE_LEN, pMsg->GumpDialogRet.m_texts[0].m_utext, lenstr );

		TCHAR *		pFix;
		if ( ( pFix = strchr( scratch, '\n' ) ) )
			*pFix	= '\0';
		if ( ( pFix = strchr( scratch, '\r' ) ) )
			*pFix	= '\0';
		if ( ( pFix = strchr( scratch, '\t' ) ) )
			*pFix	= ' ';

		resp.AddText( pMsg->GumpDialogRet.m_texts[0].m_id, scratch );

		lenstr = sizeof(pMsg->GumpDialogRet.m_texts[0]) + ( lenstr - 1 ) * sizeof(NCHAR);
		pMsg = (CEvent *)(((BYTE*)pMsg)+lenstr);
	}

	// ClearTargMode();

	switch ( context ) // This is the page number
	{
#ifndef _NEWGUILDSYSTEM
		case CLIMODE_DIALOG_GUILD: // Guild/Leige/Townstones stuff comes here
			{
				CItemStone * pStone = dynamic_cast <CItemStone *> ( m_Targ_UID.ItemFind());
				if ( !pStone || pStone->OnDialogButton( this, (STONEDISP_TYPE) dwButtonID, resp ))
					return;
			}
			break;
#endif

		default:
			break;
	}

	if ( IsClientKR() )
	{
		context = g_Cfg.GetKRDialogMap(context);
	}

	RESOURCE_ID_BASE	rid	= RESOURCE_ID(RES_DIALOG,context);
	//
	// Call the scripted response. Lose all the checks and text.
	//
	Dialog_OnButton( rid, dwButtonID, pObj, &resp );
}




bool CClient::Event_DoubleClick( CGrayUID uid, bool fMacro, bool fTestTouch, bool fScript )
{
	ADDTOCALLSTACK("CClient::Event_DoubleClick");
	// Try to use the object in some way.
	// will trigger a OnTarg_Use_Item() usually.
	// fMacro = ALTP vs dbl click. no unmount.

	// Allow some static in game objects to have function?
	// Not possible with dclick.

	ASSERT(m_pChar);
	CObjBase * pObj = uid.ObjFind();
	if ( !pObj || ( fTestTouch && !m_pChar->CanSee( pObj )) )
	{
		addObjectRemoveCantSee( uid, "the target" );
		return false;
	}

	// Face the object we are using/activating.
	SetTargMode();
	m_Targ_UID = uid;
	m_pChar->UpdateDir( pObj );

	if ( pObj->IsItem())
	{
		return Cmd_Use_Item( dynamic_cast <CItem *>(pObj), fTestTouch, fScript );
	}

	CChar * pChar = dynamic_cast <CChar*>(pObj);

	if ( pChar->OnTrigger( CTRIG_DClick, m_pChar ) == TRIGRET_RET_TRUE )
		return true;

	if ( ! fMacro )
	{
		if ( pChar == m_pChar )
		{
			if ( pChar->IsStatFlag(STATF_OnHorse) )
			{
				// in war mode not to drop from horse accidentally we need this check
				// Should also check for STATF_War in case someone starts fight and runs away.
				if ( pChar->IsStatFlag(STATF_War) && pChar->Memory_FindTypes(MEMORY_FIGHT) && !IsSetCombatFlags(COMBAT_DCLICKSELF_UNMOUNTS))
				{
					addCharPaperdoll(pChar);
					return true;
				}
				else if ( pChar->Horse_UnMount() )
					return true;
			}
		}

		if ( pChar->m_pNPC && ( pChar->GetNPCBrain(true) != NPCBRAIN_HUMAN ))
		{
			if ( m_pChar->Horse_Mount( pChar ))
				return true;
			switch ( pChar->GetID())
			{
				case CREID_HORSE_PACK:
				case CREID_LLAMA_PACK:
					// pack animals open container.
					return Cmd_Use_Item( pChar->GetPackSafe(), fTestTouch );
				default:
					if ( IsPriv(PRIV_GM))
					{
						// snoop the creature.
						return Cmd_Use_Item( pChar->GetPackSafe(), false );
					}
					return false;
			}
		}
	}

	// open paper doll.
	addCharPaperdoll(pChar);

	return( true );
}

void CClient::Event_SingleClick( CGrayUID uid )
{
	ADDTOCALLSTACK("CClient::Event_SingleClick");
	// the ALLNAMES macro comes through here.
	ASSERT(m_pChar);

	CObjBase * pObj = uid.ObjFind();
	if ( !m_pChar->CanSee(pObj) )
	{
		// ALLNAMES makes this happen as we are running thru an area.
		// So display no msg. Do not use (addObjectRemoveCantSee)
		addObjectRemove( uid );
		return;
	}

	CScriptTriggerArgs Args( this );
	if ( pObj->OnTrigger( "@Click", m_pChar, &Args ) == TRIGRET_RET_TRUE )	// CTRIG_Click, ITRIG_Click
		return;

	if ( pObj->IsItem())
	{
		addItemName( dynamic_cast <CItem *>(pObj));
		return;
	}

	// TODO: if tooltip are enabled this is not used.
	// and move char name creation to tooltip for .debug
	if ( pObj->IsChar())
	{
		addCharName( dynamic_cast <CChar*>(pObj) );
		return;
	}

	SysMessagef( "Bogus item uid=0%x?", (DWORD) uid );
}

void CClient::Event_ClientVersion( const char * pData, int Len )
{
	ADDTOCALLSTACK("CClient::Event_ClientVersion");
	// XCMD_ClientVersion
	DEBUG_MSG(( "%x:XCMD_ClientVersion %s\n", m_Socket.GetSocket(), pData));

	if ( Str_Check(pData) )
		return;

	if ( m_reportedCliver )
		return;

	TCHAR * sTemp = Str_GetTemp();
	memcpy(sTemp, pData, minimum(Len,20));

	if (strstr(sTemp, "UO:3D"))
		this->m_bClient3d = true;

	// a new client version change may toggle async mode, it's important
	// to flush pending data to the client before this happens
	xFlush();

	int len = Str_GetBare( sTemp, sTemp, minimum(Len,10), " '`-+!\"#$%&()*,/:;<=>?@[\\]^{|}~" );
	if ( len )
	{
		m_reportedCliver = CCrypt::GetVerFromString(sTemp);
		DEBUG_MSG(( "Getting cliver 0x%x/0x%x\n",m_reportedCliver, (m_reportedCliver&0xFFFFF0) ));
		if (g_Serv.m_ClientVersion.GetClientVer() && ((m_reportedCliver&0xFFFFF0) != g_Serv.m_ClientVersion.GetClientVer()))
			this->addLoginErr(LOGIN_ERR_BAD_CLIVER);
		if (( m_Crypt.GetClientVer() < 400000 ) && ( m_reportedCliver >= 400000 ) && ( IsResClient(RDS_AOS) ) && ( IsAosFlagEnabled(FEATURE_AOS_UPDATE_B) )) //workaround for a "bug", which sends all items in LOS before processing this packet
		{
			DEBUG_MSG(("m_Crypt.GetClientVer()(%x) != m_reportedCliver(%x) == %x\n", m_Crypt.GetClientVer(), m_reportedCliver, (m_Crypt.GetClientVer() != m_reportedCliver)));
			addAOSPlayerSeeNoCrypt();
		}
	}
}

void CClient::Event_Target( const CEvent * pEvent )
{
	ADDTOCALLSTACK("CClient::Event_Target");
	// XCMD_Target
	// If player clicks on something with the targetting cursor
	// Assume addTarget was called before this.
	// NOTE: Make sure they can actually validly trarget this item !

	ASSERT(m_pChar);
	if ( pEvent->Target.m_context != GetTargMode())
	{
		if ( pEvent->Target.m_context != 0 || pEvent->Target.m_x != 0xFFFF || pEvent->Target.m_UID != 0 )
		{
			// DEBUG_ERR(( "%x: Unrequested target info ?\n", m_Socket.GetSocket()));
			SysMessage( "Unexpected target info" );
		}
		return;
	}
	if ( pEvent->Target.m_x == 0xFFFF && pEvent->Target.m_UID == 0 )
	{
		// canceled
		SetTargMode();
		return;
	}

	CGrayUID uid( pEvent->Target.m_UID );
	CPointMap pt( pEvent->Target.m_x, pEvent->Target.m_y, pEvent->Target.m_z, m_pChar->GetTopMap() );
	ITEMID_TYPE id = (ITEMID_TYPE)(WORD) pEvent->Target.m_id;	// if static tile.

	CLIMODE_TYPE prevmode = GetTargMode();
	ClearTargMode();

	if ( IsClientKR() && ( pEvent->Target.m_fCheckCrime & 0xA0 ) )
		uid = m_Targ_Last;

	CObjBase * pObj = uid.ObjFind();
	if ( IsPriv( PRIV_GM ))
	{
		if ( uid.IsValidUID() && pObj == NULL )
		{
			addObjectRemoveCantSee( uid, "the target" );
			return;
		}
	}
	else
	{
		if ( uid.IsValidUID())
		{
			if ( ! m_pChar->CanSee(pObj))
			{
				addObjectRemoveCantSee( uid, "the target" );
				return;
			}
		}
		else
		{
			// The point must be valid.
			if ( m_pChar->GetTopDist(pt) > UO_MAP_VIEW_SIZE )
			{
				return;
			}
		}
	}

	if ( pObj )
	{
		// Remember the last existing target
		m_Targ_Last = uid;

		// Point inside a container is not really meaningful here.
		pt = pObj->GetTopLevelObj()->GetTopPoint();
	}

	bool fSuccess = false;

	switch ( prevmode )
	{
		// GM stuff.
		case CLIMODE_TARG_OBJ_SET:			fSuccess = OnTarg_Obj_Set( pObj ); break;
		case CLIMODE_TARG_OBJ_INFO:			fSuccess = OnTarg_Obj_Info( pObj, pt, id );  break;
		case CLIMODE_TARG_OBJ_FUNC:			fSuccess = OnTarg_Obj_Function( pObj, pt, id );  break;

		case CLIMODE_TARG_UNEXTRACT:		fSuccess = OnTarg_UnExtract( pObj, pt ); break;
		case CLIMODE_TARG_ADDITEM:			fSuccess = OnTarg_Item_Add( pObj, pt ); break;
		case CLIMODE_TARG_LINK:				fSuccess = OnTarg_Item_Link( pObj ); break;
		case CLIMODE_TARG_TILE:				fSuccess = OnTarg_Tile( pObj, pt );  break;

		// Player stuff.
		case CLIMODE_TARG_SKILL:			fSuccess = OnTarg_Skill( pObj ); break;
		case CLIMODE_TARG_SKILL_MAGERY:     fSuccess = OnTarg_Skill_Magery( pObj, pt ); break;
		case CLIMODE_TARG_SKILL_HERD_DEST:  fSuccess = OnTarg_Skill_Herd_Dest( pObj, pt ); break;
		case CLIMODE_TARG_SKILL_POISON:		fSuccess = OnTarg_Skill_Poison( pObj ); break;
		case CLIMODE_TARG_SKILL_PROVOKE:	fSuccess = OnTarg_Skill_Provoke( pObj ); break;

		case CLIMODE_TARG_REPAIR:			fSuccess = m_pChar->Use_Repair( uid.ItemFind()); break;
		case CLIMODE_TARG_PET_CMD:			fSuccess = OnTarg_Pet_Command( pObj, pt ); break;
		case CLIMODE_TARG_PET_STABLE:		fSuccess = OnTarg_Pet_Stable( uid.CharFind()); break;

		case CLIMODE_TARG_USE_ITEM:			fSuccess = OnTarg_Use_Item( pObj, pt, id );  break;
		case CLIMODE_TARG_STONE_RECRUIT:	fSuccess = OnTarg_Stone_Recruit( uid.CharFind() );  break;
		case CLIMODE_TARG_STONE_RECRUITFULL:fSuccess = OnTarg_Stone_Recruit(uid.CharFind(), true); break;
		case CLIMODE_TARG_PARTY_ADD:		fSuccess = OnTarg_Party_Add( uid.CharFind() );  break;
	}
}

void CClient::AOSPopupMenuAdd( WORD entrytag, WORD textid, WORD flags = 0, WORD color = 0 )
{
	ADDTOCALLSTACK("CClient::AOSPopupMenuAdd");
	
	if ( m_context_popup <= -1 )
	{
		DEBUG_ERR(("Bad AddContextEntry usage: Not used under a @ContextMenuRequest/@itemContextMenuRequest trigger!\n"));
		return;
	}

	if ( m_context_popup < MAX_POPUPS )
	{
		if ( IsClientKR() )
		{
			if ( flags & POPUPFLAG_COLOR )
			{
				flags &= ~POPUPFLAG_COLOR;
			}

			m_pPopupCur->KRPopup_Display.m_List.m_TextID = (3000000 + textid);
			m_pPopupCur->KRPopup_Display.m_List.m_EntryTag = entrytag;
			m_pPopupCur->KRPopup_Display.m_List.m_Flags = flags;
			++m_context_popup;
			m_pPopupCur = (CExtData *)( ((BYTE*) m_pPopupCur ) + 8 );
			m_PopupLen += 8;
		}
		else
		{
			short int length = 0;
			m_pPopupCur->Popup_Display.m_List.m_EntryTag = entrytag;
			m_pPopupCur->Popup_Display.m_List.m_TextID = textid;
			m_pPopupCur->Popup_Display.m_List.m_Flags = flags;
			length += 6;
			
			if ( flags & POPUPFLAG_COLOR )
			{
				m_pPopupCur->Popup_Display.m_List.m_Color = color;
				length += 2;
			}

			++m_context_popup;
			m_pPopupCur = (CExtData *)( ((BYTE*) m_pPopupCur ) + length );
			m_PopupLen += length;
		}
	}
	else
	{
		DEBUG_ERR(("Bad AddContextEntry usage: Too many entries!\n"));
	}
}

void CClient::Event_AOSPopupMenuRequest( DWORD uid ) //construct packet after a client request
{
	ADDTOCALLSTACK("CClient::Event_AOSPopupMenuRequest");
	CGrayUID uObj = uid;
	CChar *pChar = uObj.CharFind();

	if ( !CanSee( uObj.ObjFind() ) )
		return;

	if ( m_pChar && !(m_pChar->CanSeeLOS( uObj.ObjFind(), 0x0 )) )
		return;

	CExtData cmd;
	m_pPopupCur = &cmd;
	m_context_popup = 0;
	m_PopupLen = 7; //size of header of Popup_Display struct
	CScriptTriggerArgs Args;

	bool fPreparePacket = false;

	if ( IsClientKR() )
	{
		cmd.KRPopup_Display.m_unk1 = 2;
		cmd.KRPopup_Display.m_UID = uid;
	}
	else
	{
		cmd.Popup_Display.m_unk1 = 1;
		cmd.Popup_Display.m_UID = uid;
	}

	if ( uObj.IsItem() )
	{
		if ( !IsSetEF(EF_Minimize_Triggers))
		{
			Args = 1;
			uObj.ItemFind()->OnTrigger(ITRIG_ContextMenuRequest, this->GetChar(), &Args);
			fPreparePacket = true; // There is no hardcoded stuff for items
		}
		else
			return;
	}
	else if ( uObj.IsChar() )
	{
		if ( !IsSetEF(EF_Minimize_Triggers))
		{
			Args = 1;
			int iRet = pChar->OnTrigger(CTRIG_ContextMenuRequest, this->GetChar(), &Args);
			if ( iRet  == TRIGRET_RET_TRUE )
				fPreparePacket = true;
		}
	}
	else
	{
		m_context_popup = -1;
		return;
	}

	if ( ! fPreparePacket )
	{

		if ( pChar->IsHuman() )
			AOSPopupMenuAdd(POPUP_PAPERDOLL, 6123, POPUPFLAG_COLOR, 0xFFFF);

		if ( pChar == m_pChar )
			AOSPopupMenuAdd(POPUP_BACKPACK, 6145, POPUPFLAG_COLOR, 0xFFFF);

		if ( pChar->m_pNPC )
		{
			switch ( pChar->m_pNPC->m_Brain )
			{
				case NPCBRAIN_BANKER:
					{
						AOSPopupMenuAdd(POPUP_BANKBOX, 6105, POPUPFLAG_COLOR, 0xFFFF);
						AOSPopupMenuAdd(POPUP_BANKBALANCE, 6124, POPUPFLAG_COLOR, 0xFFFF);
						break;
					}

				case NPCBRAIN_STABLE:
					AOSPopupMenuAdd(POPUP_STABLESTABLE, 6126, POPUPFLAG_COLOR, 0xFFFF);
					AOSPopupMenuAdd(POPUP_STABLERETRIEVE, 6127, POPUPFLAG_COLOR, 0xFFFF);

				case NPCBRAIN_VENDOR:
				case NPCBRAIN_HEALER:
					AOSPopupMenuAdd(POPUP_VENDORBUY, 6103, POPUPFLAG_COLOR, 0xFFFF);
					AOSPopupMenuAdd(POPUP_VENDORSELL, 6104, POPUPFLAG_COLOR, 0xFFFF);
					break;
			}

			if ( pChar->NPC_IsOwnedBy( m_pChar, false ) )
			{
				AOSPopupMenuAdd(POPUP_PETGUARD, 6107, POPUPFLAG_COLOR, 0xFFFF);
				AOSPopupMenuAdd(POPUP_PETFOLLOW, 6108, POPUPFLAG_COLOR, 0xFFFF);
				AOSPopupMenuAdd(POPUP_PETDROP, 6109, POPUPFLAG_COLOR, 0xFFFF);
				AOSPopupMenuAdd(POPUP_PETKILL, 6111, POPUPFLAG_COLOR, 0xFFFF);
				AOSPopupMenuAdd(POPUP_PETSTOP, 6112, POPUPFLAG_COLOR, 0xFFFF);
				AOSPopupMenuAdd(POPUP_PETSTAY, 6114, POPUPFLAG_COLOR, 0xFFFF);
				AOSPopupMenuAdd(POPUP_PETFRIEND, 6110, POPUPFLAG_COLOR, 0xFFFF);
				AOSPopupMenuAdd(POPUP_PETTRANSFER, 6113, POPUPFLAG_COLOR, 0xFFFF);
			}
		}

		if (( Args.m_iN1 != 1 ) && ( !IsSetEF(EF_Minimize_Triggers)))
		{
			Args = 2;
			pChar->OnTrigger(CTRIG_ContextMenuRequest, this->GetChar(), &Args);
		}
	}
	
	if ( m_context_popup > 0 )
	{
		if ( IsClientKR() )
			cmd.KRPopup_Display.m_NumPopups = m_context_popup;
		else
			cmd.Popup_Display.m_NumPopups = m_context_popup;

		addExtData(EXTDATA_Popup_Display, &cmd, m_PopupLen);
	}

	m_context_popup = -1;
}

void CClient::Event_AOSPopupMenuSelect( DWORD uid, WORD EntryTag ) //do something after a player selected something from a pop-up menu
{
	ADDTOCALLSTACK("CClient::Event_AOSPopupMenuSelect");
	if ( !EntryTag )
		return;

	CGrayUID uObj = uid;
	CScriptTriggerArgs Args;

	if ( !CanSee( uObj.ObjFind() ) )
		return;

	if ( m_pChar && !(m_pChar->CanSeeLOS( uObj.ObjFind(), 0x0 )) )
		return;

	if ( uObj.IsItem() )
	{
		if ( !IsSetEF(EF_Minimize_Triggers) )
		{
			Args = EntryTag;
			uObj.ItemFind()->OnTrigger(ITRIG_ContextMenuSelect, this->GetChar(), &Args);
		}
		return; //There is no hardcoded stuff for items
	}
	else if ( uObj.IsChar() )
	{
		if ( !IsSetEF(EF_Minimize_Triggers))
		{
			Args = EntryTag;
			int iRet = uObj.CharFind()->OnTrigger(CTRIG_ContextMenuSelect, this->GetChar(), &Args);
			if ( iRet == TRIGRET_RET_TRUE )
				return;
		}
	}
	else
		return;

	CChar *pChar = uObj.CharFind();

	if (( pChar->m_pNPC ) && pChar->NPC_IsOwnedBy( m_pChar, false ))
	{
		switch ( EntryTag )
		{
			case POPUP_PETGUARD:
				pChar->NPC_OnHearPetCmd( "guard", m_pChar, false );
				break;

			case POPUP_PETFOLLOW:
				pChar->NPC_OnHearPetCmd( "follow", m_pChar, false );
				break;

			case POPUP_PETDROP:
				pChar->NPC_OnHearPetCmd( "drop", m_pChar, false );
				break;

			case POPUP_PETKILL:
				pChar->NPC_OnHearPetCmd( "kill", m_pChar, false );
				break;

			case POPUP_PETSTOP:
				pChar->NPC_OnHearPetCmd( "stop", m_pChar, false );
				break;

			case POPUP_PETSTAY:
				pChar->NPC_OnHearPetCmd( "stay", m_pChar, false );
				break;

			case POPUP_PETFRIEND:
				pChar->NPC_OnHearPetCmd( "friend", m_pChar, false );
				break;

			case POPUP_PETTRANSFER:
				pChar->NPC_OnHearPetCmd( "transfer", m_pChar, false );
				break;
		}
	}

	switch ( EntryTag )
	{
		case POPUP_PAPERDOLL:
			if ( m_pChar == pChar )
				Event_DoubleClick(m_pChar->GetUID(), true, false);
			else
				m_pChar->Use_Obj((CObjBase *)pChar, false, false);
			break;

		case POPUP_BACKPACK:
			m_pChar->Use_Obj( (CObjBase *)m_pChar->LayerFind( LAYER_PACK ), false, false );
			break;

		case POPUP_BANKBOX:
			if ( pChar->m_pNPC->m_Brain == NPCBRAIN_BANKER )
				addBankOpen( m_pChar );
			break;

		case POPUP_BANKBALANCE:
			if ( pChar->m_pNPC->m_Brain == NPCBRAIN_BANKER )
				SysMessagef( "You have %d gold piece(s) in your bankbox", m_pChar->GetBank()->ContentCount( RESOURCE_ID(RES_TYPEDEF,IT_GOLD) ) );
			break;

		case POPUP_VENDORBUY:
			if ( pChar->NPC_IsVendor() )
				pChar->NPC_OnHear("buy", m_pChar);
			break;

		case POPUP_VENDORSELL:
			if ( pChar->NPC_IsVendor() )
				pChar->NPC_OnHear("sell", m_pChar);
			break;

		case POPUP_STABLESTABLE:
			if ( pChar->m_pNPC->m_Brain == NPCBRAIN_STABLE )
				pChar->NPC_OnHear("stable", m_pChar);
			break;

		case POPUP_STABLERETRIEVE:
			if ( pChar->m_pNPC->m_Brain == NPCBRAIN_STABLE )
				pChar->NPC_OnHear("retrieve", m_pChar);
			break;
	}
}

void CClient::Event_AOSItemInfo( int count , const NDWORD * uidList )
{
	ADDTOCALLSTACK("CClient::Event_AOSItemInfo");
	if ( !m_pChar )
		return;

	if ( IsNoCryptVer(0x400000) )
	{
		if ( !IsResClient(RDS_AOS) || !IsAosFlagEnabled( FEATURE_AOS_UPDATE_B ) )
			return;
	}
	else if ( IsClientVer(0x400000) )
	{
		if ( !IsResClient(RDS_AOS) || !IsAosFlagEnabled( FEATURE_AOS_UPDATE_B ) )
			return;
	}
	else
	{
		return;
	}

	CObjBase * obj;
	for ( int i = 0; i < count; i++, obj = NULL )
	{
		CGrayUID uid = (DWORD) uidList[i];
		obj = uid.ObjFind();

		if ( !obj )
			return;

		if ( m_pChar->CanSee(obj) )
		{
			this->addAOSTooltip(obj);
		}
	}
}

void CClient::Event_AllNames3D( CGrayUID uid )
{
	ADDTOCALLSTACK("CClient::Event_AllNames3D");
	if ( !m_pChar || !uid )
		return;

	CObjBase * pObj = uid.ObjFind();
	if ( !pObj )
		return;

	if ( ! m_pChar->CanSee(pObj) )
		return;

	CCommand cmd;
	cmd.AllNames3D.m_Cmd = XCMD_AllNames3D;
	cmd.AllNames3D.m_len = 37;
	cmd.AllNames3D.m_UID = (DWORD) pObj->GetUID();
	strcpylen(cmd.AllNames3D.m_name, pObj->GetName(), sizeof(cmd.AllNames3D.m_name));

	xSendPkt(&cmd, cmd.AllNames3D.m_len);
}

void CClient::Event_BugReport( const NCHAR * pszText, int len, BUGREPORT_TYPE type, CLanguageID lang )
{
	ADDTOCALLSTACK("CClient::Event_BugReport");
	if ( !m_pChar )
		return;

	TCHAR szText[MAX_TALK_BUFFER * 2];
	CvtNUNICODEToSystem( szText, sizeof(szText), pszText, len );

	CScriptTriggerArgs Args(type);
	Args.m_s1 = szText;
	Args.m_VarsLocal.SetStr("LANG", false, lang.GetStr());

	m_pChar->OnTrigger(CTRIG_UserBugReport, m_pChar, &Args);
}

void CClient::Event_MacroEquipItems( const NDWORD * pItems, int count )
{
	ADDTOCALLSTACK("CClient::Event_MacroEquipItems");
	if ( !m_pChar )
		return;

	CItem * pItem;

	for (int i = 0; i < count; i++ )
	{
		pItem = CGrayUID(pItems[i]).ItemFind();
		if ( !pItem )
			continue;
		
		if ( pItem->GetTopLevelObj() != m_pChar || pItem->IsItemEquipped() )
			continue;

		if ( m_pChar->ItemPickup(pItem, pItem->GetAmount()) < 1 )
			continue;

		m_pChar->ItemEquip(pItem);
	}
}

void CClient::Event_MacroUnEquipItems( const NWORD * pLayers, int count )
{
	ADDTOCALLSTACK("CClient::Event_MacroUnEquipItems");
	if ( !m_pChar )
		return;

	LAYER_TYPE layer;
	CItem * pItem;

	for (int i = 0; i < count; i++ )
	{
		layer = (LAYER_TYPE)(WORD)pLayers[i];

		pItem = m_pChar->LayerFind(layer);
		if ( !pItem )
			continue;

		if ( m_pChar->ItemPickup(pItem, pItem->GetAmount()) < 1 )
			continue;

		m_pChar->ItemBounce(pItem);
	}
}

void CClient::Event_UseToolbar(BYTE bType, DWORD dwArg)
{
	ADDTOCALLSTACK("CClient::Event_UseToolbar");
	if ( !m_pChar )
		return;

	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		CScriptTriggerArgs Args( bType, dwArg );
		if ( m_pChar->OnTrigger( CTRIG_UserKRToolbar, m_pChar, &Args ) == TRIGRET_RET_TRUE )
			return;
	}

	switch(bType)
	{
		case 0x01: // Spell call
		{
			Cmd_Skill_Magery((SPELL_TYPE)dwArg, m_pChar);
		} break;

		case 0x02: // Weapon ability
		{

		} break;

		case 0x03: // Skill
		{
			Event_Skill_Use((SKILL_TYPE)dwArg);
		} break;

		case 0x04: // Item
		{
			Event_DoubleClick(CGrayUID(dwArg), true, true);
		} break;

		case 0x05: // Scroll
		{

		} break;
	}

}

void CClient::Event_HouseDesigner( EXTAOS_TYPE type, const CExtAosData * pData, DWORD m_uid, int len )
{
	ADDTOCALLSTACK("CClient::Event_HouseDesigner");
	if ( m_pChar == NULL || m_uid != m_pChar->GetUID() )
		return;

	CItemMultiCustom * pItemMulti = m_pHouseDesign;
	if ( pItemMulti == NULL )
	{
		m_pHouseDesign = NULL;
		return;
	}

	switch ( type )
	{
		case EXTAOS_HcBackup:
			pItemMulti->BackupStructure(this);
			break;

		case EXTAOS_HcRestore:
			pItemMulti->RestoreStructure(this);
			break;

		case EXTAOS_HcCommit:
			pItemMulti->CommitChanges(this);
			break;

		case EXTAOS_HcDestroyItem:
			pItemMulti->RemoveItem(this, (ITEMID_TYPE)(WORD)pData->HouseDestroyItem.m_Dispid, pData->HouseDestroyItem.m_PosX, pData->HouseDestroyItem.m_PosY, pData->HouseDestroyItem.m_PosZ);
			break;

		case EXTAOS_HcPlaceItem:
			pItemMulti->AddItem(this, (ITEMID_TYPE)(WORD)pData->HousePlaceItem.m_Dispid, pData->HousePlaceItem.m_PosX, pData->HousePlaceItem.m_PosY);
			break;

		case EXTAOS_HcExit:
			pItemMulti->EndCustomize();
			break;

		case EXTAOS_HcPlaceStair:
			pItemMulti->AddStairs(this, (ITEMID_TYPE)((WORD)pData->HousePlaceStair.m_Dispid + ITEMID_MULTI), pData->HousePlaceStair.m_PosX, pData->HousePlaceStair.m_PosY);
			break;

		case EXTAOS_HcSynch:
			pItemMulti->SendStructureTo(this);
			break;

		case EXTAOS_HcClear:
			pItemMulti->ResetStructure(this);
			break;

		case EXTAOS_HcSwitch:
			pItemMulti->SwitchToLevel(this, pData->HouseSwitchFloor.m_Floor);
			break;
			
		case EXTAOS_HcPlaceRoof:
			pItemMulti->AddRoof(this, (ITEMID_TYPE)(WORD)pData->HousePlaceRoof.m_Roof, pData->HousePlaceRoof.m_PosX, pData->HousePlaceRoof.m_PosY, pData->HousePlaceRoof.m_PosZ);
			break;

		case EXTAOS_HcDestroyRoof:
			pItemMulti->RemoveRoof(this, (ITEMID_TYPE)(WORD)pData->HouseDestroyRoof.m_Roof, pData->HouseDestroyRoof.m_PosX, pData->HouseDestroyRoof.m_PosY, pData->HouseDestroyRoof.m_PosZ);
			break;

		case EXTAOS_HcRevert:
			pItemMulti->RevertChanges(this);
			break;

		default:
			SysMessagef("Unhandled AOS house designer msg 0x%2X.", type);
			break;
	}
}

//----------------------------------------------------------------------

void CClient::Event_ExtAosData( EXTAOS_TYPE type, const CExtAosData * pData, DWORD m_uid, int len )
{
	ADDTOCALLSTACK("CClient::Event_ExtAosData");
	// XCMD_ExtData = 5 bytes of overhead before this.
	switch ( type )
	{
		case EXTAOS_HcBackup:
		case EXTAOS_HcRestore:
		case EXTAOS_HcCommit:
		case EXTAOS_HcDestroyItem:
		case EXTAOS_HcPlaceItem:
		case EXTAOS_HcExit:
		case EXTAOS_HcPlaceStair:
		case EXTAOS_HcSynch:
		case EXTAOS_HcPlaceRoof:
		case EXTAOS_HcDestroyRoof:
		case EXTAOS_HcClear:
		case EXTAOS_HcSwitch:
		case EXTAOS_HcRevert:
			Event_HouseDesigner( type, pData, m_uid, len );
			break;

		case EXTAOS_SpecialMove:
		{
			CScriptTriggerArgs args;
			args.m_iN1 = pData->SpecialMove.m_Ability;
			m_pChar->OnTrigger(CTRIG_UserSpecialMove, m_pChar, &args);
		} break;

		case EXTAOS_EquipLastWeapon:
			break;

		case EXTAOS_GuildButton:
		{
			m_pChar->OnTrigger(CTRIG_UserGuildButton, m_pChar, NULL);
		} break;

		case EXTAOS_QuestButton:
		{
			m_pChar->OnTrigger(CTRIG_UserQuestButton, m_pChar, NULL);
		} break;

		default:
			SysMessagef( "Unknown AOS extended msg 0x%2X.", type );
			break;
	}
}

void CClient::Event_ExtData( EXTDATA_TYPE type, const CExtData * pData, int len )
{
	ADDTOCALLSTACK("CClient::Event_ExtData");
	// XCMD_ExtData = 5 bytes of overhead before this.
	switch ( type )
	{
		case EXTDATA_ScreenSize:
			// Sent at start up for the party system ?
			{
				DEBUG_MSG(("0x%x - 0x%x (%d-%d)\n", pData->ScreenSize.m_x, pData->ScreenSize.m_y,
					pData->ScreenSize.m_x, pData->ScreenSize.m_y));

				if ( m_ScreenSize.x != 0 )
					m_ScreenSize.x = (DWORD)pData->ScreenSize.m_x;

				if ( m_ScreenSize.y != 0 )
					m_ScreenSize.y = (DWORD)pData->ScreenSize.m_y;
			}
			break;
		case EXTDATA_Lang:
			// Tell what lang i use.
			GetAccount()->m_lang.Set( pData->Lang.m_code );
			break;
		case EXTDATA_Party_Msg: // = 6
			// Messages about the party we are in.
			ASSERT(m_pChar);
			switch ( pData->Party_Msg_Opt.m_code )
			{
				case PARTYMSG_Add:
					// request to add a new member. len=5. m_msg = 4x0's
					// CScriptTriggerArgs Args( m_pChar );
					// OnTrigger( CTRIG_PartyInvite, this, &Args );
					addTarget( CLIMODE_TARG_PARTY_ADD, g_Cfg.GetDefaultMsg( DEFMSG_PARTY_TARG_WHO ), false, false );
					break;
				case PARTYMSG_Disband:
					if ( m_pChar->m_pParty == NULL )
						return;
					m_pChar->m_pParty->Disband( m_pChar->GetUID());
					break;
				case PARTYMSG_Remove:
					// Request to remove this member of the party.
					if ( m_pChar->m_pParty == NULL )
						return;
					if ( len != 5 )
						return;
					m_pChar->m_pParty->RemoveMember( (DWORD) pData->Party_Msg_Rsp.m_UID, m_pChar->GetUID());
					break;
				case PARTYMSG_MsgMember:
					// Message a specific member of my party.
					if ( m_pChar->m_pParty == NULL )
						return;
					if ( len < 6 )
						return;
					m_pChar->m_pParty->MessageEvent( (DWORD) pData->Party_Msg_Rsp.m_UID, m_pChar->GetUID(), pData->Party_Msg_Rsp.m_msg, len-1 );
					break;
				case PARTYMSG_Msg:
					// Send this message to the whole party.
					if ( len < 6 )
						return;
					if ( m_pChar->m_pParty == NULL )
					{
						// No Party !
						// We must send a response back to the client for this or it will hang !?
						// CPartyDef::MessageClient( this, m_pChar->GetUID(), (const NCHAR*)( pData->Party_Msg_Opt.m_data ), len-1 );
						return;
					}
					// else
					// {
						m_pChar->m_pParty->MessageEvent( (DWORD) 0, m_pChar->GetUID(), (const NCHAR*)( pData->Party_Msg_Opt.m_data ), len-1 );
					// }
					break;
				case PARTYMSG_Option:
					// set the loot flag.
					if ( m_pChar->m_pParty == NULL )
						return;
					if ( len != 2 )
						return;
					m_pChar->m_pParty->SetLootFlag( m_pChar, pData->Party_Msg_Opt.m_data[0] );
					break;
				case PARTYMSG_Accept:
					// We accept or decline the offer of an invite.
					if ( len != 5 )
						return;
					CPartyDef::AcceptEvent( m_pChar, (DWORD) pData->Party_Msg_Rsp.m_UID );
					break;

				case PARTYMSG_Decline:
					// decline party invite.
					// " You notify %s that you do not wish to join the party"
					CPartyDef::DeclineEvent( m_pChar, (DWORD) pData->Party_Msg_Rsp.m_UID );
					break;

				default:
					SysMessagef( "Unknown party system msg %d", pData->Party_Msg_Rsp.m_code );
					break;
			}
			break;
		case EXTDATA_Arrow_Click:
			SysMessageDefault( DEFMSG_FOLLOW_ARROW );
			if ( !IsSetEF(EF_Minimize_Triggers) )
			{
				CScriptTriggerArgs Args;
				Args.m_iN1 = (pData->QuestArrow.m_rightClick > 0? 1:0);
#ifdef _ALPHA
				Args.m_iN2 = m_pChar->GetKeyNum("ARROWQUEST_X", true);
				Args.m_iN3 = m_pChar->GetKeyNum("ARROWQUEST_Y", true);
#endif

				m_pChar->OnTrigger(CTRIG_UserQuestArrowClick, m_pChar, &Args);
			}
			break;

		case EXTDATA_StatusClose:
			// The status window has been closed. (need send no more updates)
			// 4 bytes = uid of the char status closed.
			break;

		case EXTDATA_Stats_Change:
			{
				if ( !this->m_pChar )
					return;

				if ( !this->m_pChar->m_pPlayer )
					return;

				if (( pData->Stats_Change.m_stat <= STAT_NONE ) || ( pData->Stats_Change.m_stat >= STAT_BASE_QTY ))
					return;

				if (( pData->Stats_Change.m_status < SKILLLOCK_UP ) || ( pData->Stats_Change.m_status > SKILLLOCK_LOCK ))
					return;

				// OSI/EA use different IDs for Stats so we cannot just
				// simply pass m_stat to Stat_SetLock()
				STAT_TYPE stat = STAT_NONE;
				switch (pData->Stats_Change.m_stat)
				{
					case 0:
						stat = STAT_STR;
						break;
					case 1:
						stat = STAT_DEX;
						break;
					case 2:
						stat = STAT_INT;
						break;
				}
				if ( stat != STAT_NONE )
					this->m_pChar->m_pPlayer->Stat_SetLock(stat,(SKILLLOCK_TYPE)pData->Stats_Change.m_status);

			} break;

		case EXTDATA_Wrestle_DisArm:	// From Client: Wrestling disarm
		case EXTDATA_Wrestle_Stun:		// From Client: Wrestling stun
			SysMessageDefault( DEFMSG_WRESTLING_NOGO );
			break;

		case EXTDATA_Yawn:
			{
				static int validAnimations[] =
				{
					6, 21, 32, 33,
					100, 101, 102,
					103, 104, 105,
					106, 107, 108,
					109, 110, 111,
					112, 113, 114,
					115, 116, 117,
					118, 119, 120,
					121, 123, 124,
					125, 126, 127,
					128
				};

				DWORD anim = (DWORD)pData->ScreenSize.m_x;
				bool ok = false;
				for ( int i = 0; !ok && ( i < COUNTOF(validAnimations) ); i++ )
					ok = ( anim == validAnimations[i] );

				if ( ok )
					m_pChar->UpdateAnimate((ANIM_TYPE)anim);
			}
			break;

		case EXTDATA_Unk15:
			break;

		case EXTDATA_OldAOSTooltipInfo:
			{
				if ( len != 4 )
					return;

				Event_AOSItemInfo(1, &(pData->OldAOSTooltipRequest.m_uid));
			} break;

		case EXTDATA_Popup_Request:
			if ( IsAosFlagEnabled( FEATURE_AOS_POPUP ) && IsResClient( RDS_AOS ) )
				Event_AOSPopupMenuRequest( (DWORD) pData->Popup_Request.m_UID );
			break;

		case EXTDATA_Popup_Select:
			if ( IsAosFlagEnabled( FEATURE_AOS_POPUP ) && IsResClient( RDS_AOS ) )
				Event_AOSPopupMenuSelect( (DWORD) pData->Popup_Select.m_UID, (WORD) pData->Popup_Select.m_EntryTag );
			break;

		case EXTDATA_HouseDesignDet:
		{
			CGrayUID uid( (DWORD) pData->HouseDesignDetail.m_HouseUID );

			CItem * pItem = uid.ItemFind();
			if ( pItem == NULL )
				break;

			CItemMultiCustom * pMulti = dynamic_cast<CItemMultiCustom *>( pItem );
			if ( pMulti == NULL )
				break;

			pMulti->SendStructureTo(this);
			break;
		}

		case EXTDATA_NewSpellSelect:
			{
				WORD iSpell = pData->NewSpellSelect.m_SpellId;
				// DEBUG_ERR(("Spell selected: %d (%x)\n", iSpell, iSpell));

				if ( IsSetMagicFlags( MAGICF_PRECAST ) )
				{
					const CSpellDef *pSpellDef = g_Cfg.GetSpellDef((SPELL_TYPE) iSpell);
					if (pSpellDef == NULL)
						return;
					if ( pSpellDef->IsSpellType( SPELLFLAG_NOPRECAST ) )
					{
						Cmd_Skill_Magery( (SPELL_TYPE) iSpell, m_pChar );
					} else {
						int skill;
						if (!pSpellDef->GetPrimarySkill(&skill, NULL))
							return;

						m_tmSkillMagery.m_Spell = (SPELL_TYPE) iSpell;
						m_pChar->m_atMagery.m_Spell = (SPELL_TYPE) iSpell;	// m_atMagery.m_Spell
						m_Targ_UID = m_pChar->GetUID();	// default target.
						m_Targ_PrvUID = m_pChar->GetUID();
						m_pChar->Skill_Start( (SKILL_TYPE)skill );
					}
				}
				else
					Cmd_Skill_Magery( (SPELL_TYPE) iSpell, m_pChar );
			}
			break;

		case EXTDATA_AntiCheat:
			{
				/*
				if ( IsPriv(PRIV_GM) )
				{
					return;
				}

				if ( (pData->AntiCheat.m_unk >= 0x25) && (pData->AntiCheat.m_unk <= 0x83) )
				{
					if ( m_BfAntiCheat.lastvalue != pData->AntiCheat.m_unk )
					{
						m_BfAntiCheat.count = 0;
					}
					else // something wrong it always change
					{
						if ( m_BfAntiCheat.count >= 20 )
						{
							g_Log.Event( LOGL_WARN|LOGM_CHEAT, "%x:Cheater '%s' is using 3rd party tools to alter client functionality\n",
								m_Socket.GetSocket(), (LPCTSTR) GetAccount()->GetName());

							m_BfAntiCheat.count = 0;
						}
						else
						{
							m_BfAntiCheat.count++;
						}
					}

					m_BfAntiCheat.lastvalue = pData->AntiCheat.m_unk;
				}
				else
				{
					g_Log.Event( LOGL_WARN|LOGM_CHEAT, "%x:Cheater '%s' is using 3rd party tools to alter client functionality\n",
						m_Socket.GetSocket(), (LPCTSTR) GetAccount()->GetName());
				}
				*/
			}
			break;

		case EXTDATA_BandageMacro:
			{
				CGrayUID bandageSerial = (DWORD)pData->BandageMacro.m_bandageSerial;
				CGrayUID targetSerial = (DWORD)pData->BandageMacro.m_targetSerial;

				CItem * pBandage = bandageSerial.ItemFind();
				CObjBase * pTarget = targetSerial.ObjFind();

				if ( !pBandage || !pTarget )
					return;

				// Check the client can see the bandage that they're
				// trying to use.
				if ( !m_pChar->CanSee( pBandage ) )
				{
					addObjectRemoveCantSee( bandageSerial, "the target" );
					return;
				}

				// Check the client is capable of using the bandage
				if ( ! m_pChar->CanUse( pBandage, false ))
					return;

				// Check the bandage is in the possession of the client
				if ( pBandage->GetTopLevelObj() != m_pChar )
					return;

				// Make sure the macro isn't used for other types of items.
				if ( !pBandage->IsType( IT_BANDAGE ) )
					return;

				// Clear previous Target
				SetTargMode();

				// Should we simulate the dclick?
				// m_Targ_UID = pBandage->GetUID();
				// CScriptTriggerArgs extArgs(1); // Signal we're from the macro
				// if ( pBandage->OnTrigger( ITRIG_DCLICK, m_pChar, &extArgs ) == TRIGRET_RET_TRUE )
				// {
				// 		return true;
				// }
				// SetTargMode();

				// Prepare targeting information
				m_Targ_UID = pBandage->GetUID();
				m_tmUseItem.m_pParent = pBandage->GetParent();
				SetTargMode( CLIMODE_TARG_USE_ITEM );

				CEvent Event;
				CPointMap pt = pTarget->GetUnkPoint();
				Event.Target.m_context = GetTargMode();
				Event.Target.m_x = pt.m_x;
				Event.Target.m_y = pt.m_y;
				Event.Target.m_z = pt.m_z;
				Event.Target.m_UID = pTarget->GetUID();
				Event.Target.m_id = 0;

				Event_Target( &Event );
			}
			break;

		default:
			SysMessagef( "Unknown extended msg %d.", type );
			break;
	}
}

void CClient::Event_ExtCmd( EXTCMD_TYPE type, const char * pszName )
{
	ADDTOCALLSTACK("CClient::Event_ExtCmd");
	// parse the args.
	TCHAR szTmp[ MAX_TALK_BUFFER ];
	strcpylen( szTmp, pszName, sizeof(szTmp));


	if ( m_pChar )
	{
		if ( !IsSetEF(EF_Minimize_Triggers) )
		{
			CScriptTriggerArgs	Args( szTmp );
			Args.m_iN1	= type;
			if ( m_pChar->OnTrigger( CTRIG_UserExtCmd, m_pChar, &Args ) == TRIGRET_RET_TRUE )
				return;
			strcpy( szTmp, Args.m_s1 );
		}
	}

	TCHAR * ppArgs[2];
	Str_ParseCmds( szTmp, ppArgs, COUNTOF(ppArgs), " " );
	switch ( type )
	{
		case EXTCMD_OPEN_SPELLBOOK: // 67 = open spell book if we have one.
			{
				CItem * pBook = m_pChar->GetSpellbook();
				if ( pBook == NULL )
				{
					SysMessageDefault( DEFMSG_NOSPELLBOOK );
					break;
				}
				// Must send proper context info or it will crash tha client.
				// if ( pBook->GetParent() != m_pChar )
				{
					addItem( m_pChar->GetPackSafe());
				}
				addItem( pBook );
				addSpellbookOpen( pBook );
			}
			break;

		case EXTCMD_ANIMATE: // Cmd_Animate
			if ( !strcmpi( ppArgs[0],"bow"))
				m_pChar->UpdateAnimate( ANIM_BOW );
			else if ( ! strcmpi( ppArgs[0],"salute"))
				m_pChar->UpdateAnimate( ANIM_SALUTE );
			else
			{
				DEBUG_ERR(( "%x:Event Animate '%s'\n", m_Socket.GetSocket(), ppArgs[0] ));
			}
			break;

		case EXTCMD_SKILL:			// Skill
			Event_Skill_Use( (SKILL_TYPE) ATOI( ppArgs[0] ));
			break;

		case EXTCMD_AUTOTARG:	// bizarre new autotarget mode.
			// "target x y z"
			{
				CGrayUID uid( ATOI( ppArgs[0] ));
				CObjBase * pObj = uid.ObjFind();
				if ( pObj )
				{
					DEBUG_ERR(( "%x:Event Autotarget '%s' '%s'\n", m_Socket.GetSocket(), pObj->GetName(), ppArgs[1] ));
				}
				else
				{
					DEBUG_ERR(( "%x:Event Autotarget UNK '%s' '%s'\n", m_Socket.GetSocket(), ppArgs[0], ppArgs[1] ));
				}
			}
			break;

		case EXTCMD_CAST_MACRO:	// macro spell.
		case EXTCMD_CAST_BOOK:	// cast spell from book.
			{
				SPELL_TYPE spell = (SPELL_TYPE) ATOI(ppArgs[0]);
				const CSpellDef* pSpellDef = g_Cfg.GetSpellDef(spell);
				if (pSpellDef == NULL)
					return;

				if ( IsSetMagicFlags( MAGICF_PRECAST ) && !pSpellDef->IsSpellType( SPELLFLAG_NOPRECAST ) )
				{
					int skill;
					if (!pSpellDef->GetPrimarySkill(&skill, NULL))
						return;

					m_tmSkillMagery.m_Spell = spell;
					m_pChar->m_atMagery.m_Spell = spell;	// m_atMagery.m_Spell
					m_Targ_UID = m_pChar->GetUID();	// default target.
					m_Targ_PrvUID = m_pChar->GetUID();
					m_pChar->Skill_Start( (SKILL_TYPE)skill );
				}
				else
					Cmd_Skill_Magery(spell, m_pChar );
			}
			break;

		case EXTCMD_DOOR_AUTO: // open door macro = Attempt to open a door around us.
			if ( m_pChar && !m_pChar->IsStatFlag( STATF_DEAD ) )
			{
				CWorldSearch Area( m_pChar->GetTopPoint(), 4 );
				while(true)
				{
					CItem * pItem = Area.GetItem();
					if ( pItem == NULL )
						break;
					switch ( pItem->GetType() )
					{
						case IT_PORT_LOCKED:	// Can only be trigered.
						case IT_PORTCULIS:
						case IT_DOOR_LOCKED:
						case IT_DOOR:
							m_pChar->Use_Obj( pItem, false );
							return;
					}
				}
			}
			break;

		case EXTCMD_UNKGODCMD: // 107, seen this but no idea what it does.
			break;

		case EXTCMD_INVOKE_VIRTUE:
			{
				int iVirtueID = ppArgs[0][0] - '0';	// 0x1=Honor, 0x2=Sacrifice, 0x3=Valor
				CScriptTriggerArgs Args(m_pChar);
				Args.m_iN1 = iVirtueID;

				m_pChar->OnTrigger(CTRIG_UserVirtueInvoke, m_pChar, &Args);
			}
			break;

		default:
			DEBUG_ERR(( "%x:Event_ExtCmd unk %d, '%s'\n", m_Socket.GetSocket(), type, pszName ));
	}
}

// ---------------------------------------------------------------------

bool CClient::xCheckMsgSize0( int len )
{
	ADDTOCALLSTACK("CClient::xCheckMsgSize0");
	if ( ! len || len > sizeof( CEvent ))
		return( false );	// junk
	return true;
}


bool CClient::xCheckMsgSize( int len )
{
	ADDTOCALLSTACK("CClient::xCheckMsgSize");
	if ( !xCheckMsgSize0( len ) )
		return false;
	// Is there enough data from client to process this packet ?
	m_bin_msg_len = len;
	return( m_bin.GetDataQty() >= len );
}

#define RETURN_FALSE() 		\
{				\
	m_bin_ErrMsg = (XCMD_TYPE) pEvent->Default.m_Cmd;	\
	return 0;		\
}

bool CClient::xPacketFilter( const CEvent * pEvent, int iLen )
{
	ADDTOCALLSTACK("CClient::xPacketFilter");
	
	EXC_TRY("packet filter");
	if ( g_Serv.m_PacketFilter[pEvent->Default.m_Cmd][0] )
	{
		CScriptTriggerArgs Args(pEvent->Default.m_Cmd);
		enum TRIGRET_TYPE trigReturn;
		char idx[5];

		Args.m_s1 = m_PeerName.GetAddrStr();
		Args.m_pO1 = this; // Yay for ARGO.SENDPACKET
		Args.m_VarsLocal.SetNum("CONNECTIONTYPE", GetConnectType());

		int bytes;
		if ( iLen )
			bytes = iLen;
		else
		{
			bytes = minimum(m_bin.GetDataQty(), MAX_BUFFER);
		}
		int bytestr = minimum(bytes, SCRIPT_MAX_LINE_LEN);
		char *zBuf = Str_GetTemp();

		Args.m_VarsLocal.SetNum("NUM", bytes);
		memcpy(zBuf, &(pEvent->m_Raw[0]), bytestr);
		zBuf[bytestr] = 0;
		Args.m_VarsLocal.SetStr("STR", true, zBuf, true);
		if ( m_pAccount )
		{
			Args.m_VarsLocal.SetStr("ACCOUNT", false, m_pAccount->GetName());
			if ( m_pChar )
			{
				Args.m_VarsLocal.SetNum("CHAR", m_pChar->GetUID());
			}
		}

		//	Fill locals [0..X] to the first X bytes of the packet
		for ( int i = 0; i < bytes; ++i )
		{
			sprintf(idx, "%d", i);
			Args.m_VarsLocal.SetNum(idx, (int)pEvent->m_Raw[i]);
		}

		//	Call the filtering function
		if ( g_Serv.r_Call(g_Serv.m_PacketFilter[pEvent->Default.m_Cmd], &g_Serv, &Args, NULL, &trigReturn) )
			if ( trigReturn == TRIGRET_RET_TRUE )
				return true;	// do not cry about errors
	}

	EXC_CATCH;
	return false;
}

int CClient::xDispatchMsg()
{
	ADDTOCALLSTACK("CClient::xDispatchMsg");
	EXC_TRY("DispatchMsg");

#ifdef _DUMPSUPPORT
	if ( ! g_Cfg.m_sDumpAccPackets.IsEmpty() )
	{
		EXC_SET("DumpedClient Packets");
		if ( ! strnicmp( GetAccount()->GetName(), (LPCTSTR) g_Cfg.m_sDumpAccPackets, strlen( GetAccount()->GetName() ) ) )
			xDumpPacket(m_bin.GetDataQty(), m_bin.RemoveDataLock());
	}
#else
	#if _PACKETDUMP
		DEBUG_ERR(("xDispatchMsg\n"));
		xDumpPacket(m_bin.GetDataQty(), m_bin.RemoveDataLock());
	#endif
#endif

	EXC_SET("check message size");
	if ( !xCheckMsgSize(1) )	// just get the command
		return 0;

	EXC_SET("remove data");
	const CEvent * pEvent = (const CEvent *) m_bin.RemoveDataLock();

	//DEBUG_ERR(("Packet: 0x%x\n", pEvent->Default.m_Cmd));

	//	Packet filtering - check if any function triggeting is installed
	//		allow skipping the packet which we do not wish to get
	EXC_SET("packet filter");
	if ( xPacketFilter(pEvent) )
		return -1;

	EXC_SET("packet parsing");
	if ( pEvent->Default.m_Cmd >= XCMD_QTY ) // bad packet type ?
	{
		DEBUG_ERR(( "Unimplemented command %d\n", pEvent->Default.m_Cmd ) );
#ifdef _PACKETDUMP
		xDumpPacket( minimum(m_bin.GetDataQty(), MAX_BUFFER), m_bin.RemoveDataLock() );
#endif
		RETURN_FALSE();
	}

#ifdef _PACKET_LEN_CHECK
	// check the packet size first.
	if ( pEvent->Default.m_Cmd < XCMD_QTY )
	{
		WORD pLen = CCrypt::GetPacketSize(pEvent->Default.m_Cmd);
		if ( pLen >= 0x8000 )
		{
			if ( !xCheckMsgSize(3) ) // id + len
				return 0;
		}
		else
		{
			if ( !xCheckMsgSize(pLen) )
				return 0;
		}
	}
#endif

	if ( pEvent->Default.m_Cmd == XCMD_Ping )
	{
		EXC_SET("ping");
		// Ping from client. Respond with the same packet id & code
		if ( ! xCheckMsgSize( sizeof( pEvent->Ping )))
			RETURN_FALSE();
		addPing( pEvent->Ping.m_bCode );
		return 1;
	}

	// login server or a game server that has not yet logged in.
	if ( GetConnectType() != CONNECT_GAME || ! GetAccount() )
	{
		switch ( pEvent->Default.m_Cmd )
		{
			case XCMD_ServersReq: // First Login
			{
				EXC_SET("not logged - server list");
				if ( ! xCheckMsgSize( sizeof( pEvent->ServersReq )))
					RETURN_FALSE();
				LOGIN_ERR_TYPE lErr = Login_ServerList( pEvent->ServersReq.m_acctname, pEvent->ServersReq.m_acctpass );
				addLoginErr(lErr);
				return( lErr == LOGIN_SUCCESS );
			}
			case XCMD_ServerSelect:// Server Select - relay me to the server.
				EXC_SET("not logged - login relay");
				if ( ! xCheckMsgSize( sizeof( pEvent->ServerSelect )))
					RETURN_FALSE();
				return( Login_Relay( pEvent->ServerSelect.m_select ));
			case XCMD_CharListReq: // Second Login to select char
			{
				EXC_SET("not logged - char list req");
				if ( ! xCheckMsgSize( sizeof( pEvent->CharListReq )))
					RETURN_FALSE();
				return( Setup_ListReq( pEvent->CharListReq.m_acctname, pEvent->CharListReq.m_acctpass, false ) == LOGIN_SUCCESS);
			}
			case XCMD_Spy:
			case XCMD_Spy2:
			{
				EXC_SET("not logged - spy");
				int wSpyLen = ( pEvent->Default.m_Cmd == XCMD_Spy ) ? CCrypt::GetPacketSize(XCMD_Spy) : minimum(MAX_BUFFER,maximum(CCrypt::GetPacketSize(XCMD_Spy2), pEvent->Spy2.m_len));
				if ( !xCheckMsgSize( wSpyLen ) )
				{
					if ( xCheckMsgSize0( wSpyLen ) )
					{
						m_bin_msg_len = m_bin.GetDataQty();
					}
					else
						RETURN_FALSE();
				}
				return 1;
			}
			case XCMD_War:
			{
				EXC_SET("not logged - war");
				if ( !xCheckMsgSize( sizeof( pEvent->War ) ) )
					RETURN_FALSE();
				return 1;
			}
			default:
			{
				EXC_SET("not logged - anything");

				DEBUG_WARN(( "%x:Packet (0x%x) received, but it should not be here.\n", m_Socket.GetSocket(), pEvent->Default.m_Cmd ));
				if ( ! xCheckMsgSize( sizeof( pEvent->Default ) ) )
					RETURN_FALSE();
				return 1;	// Should be -1 since any weird packet in login sequence can screw the normal login process
			}
		}

		RETURN_FALSE();
	}

	/////////////////////////////////////////////////////
	// We should be encrypted below here. CONNECT_GAME
	// Get messages from the client.
	EXC_SET("game packet");
	switch ( pEvent->Default.m_Cmd )
	{
		case XCMD_Create: // Character Create
			EXC_SET("create char");
			// Removed support for 1.25 client
			if ( ! xCheckMsgSize( sizeof( pEvent->Create )))
				RETURN_FALSE();
			Setup_CreateDialog( pEvent );
			return 1;
		case XCMD_CreateNew: // Character Create (UOKR)
			EXC_SET("create char new");
			if ( !xCheckMsgSize( 3 ))
				RETURN_FALSE();
			if ( !xCheckMsgSize( pEvent->CreateNew.m_len ))
				RETURN_FALSE();
			if ( !IsClientKR() )
				RETURN_FALSE();
			Setup_CreateDialog(pEvent);
			return 1;
		case XCMD_CharDelete: // Character Delete
			EXC_SET("delete char");
			if ( ! xCheckMsgSize( sizeof( pEvent->CharDelete )))
				RETURN_FALSE();
			addDeleteErr( Setup_Delete( pEvent->CharDelete.m_slot ));
			return 1;
		case XCMD_CharPlay: // Character Select
			EXC_SET("select char");
			if ( ! xCheckMsgSize( sizeof( pEvent->CharPlay )))
				RETURN_FALSE();
			addLoginErr(Setup_Play( pEvent->CharPlay.m_slot ));
			return 1;
		case XCMD_TipReq: // Get Tip
			EXC_SET("get tip");
			if ( ! xCheckMsgSize( sizeof( pEvent->TipReq )))
				RETURN_FALSE();
			Event_Tips( pEvent->TipReq.m_index + 1 );
			return 1;
		case XCMD_LogoutStatus: // Logout Confirm
			EXC_SET("logout confirm");
			if ( ! xCheckMsgSize( sizeof( pEvent->LogoutStatus )))
				RETURN_FALSE();
			return 1;
		case XCMD_ConfigFile: // ConfigFile NOT SUPPORTED
			EXC_SET("config file");
			if ( ! xCheckMsgSize( 3 ))
				RETURN_FALSE();
			if ( ! xCheckMsgSize(pEvent->ConfigFile.m_len ))
				RETURN_FALSE();
			return 1;
		case XCMD_KRCharListUpdate:
			EXC_SET("charlist update");
			if ( ! xCheckMsgSize( sizeof( pEvent->KRCharListUpdate )))
				RETURN_FALSE();
			// What response is expected, if any? Re-sending char list
			// triggers another E1
			return 1;
	}

	// must have a logged in char to use any other messages.
	if ( m_pChar == NULL )
	{
		EXC_SET("no char");

		switch ( pEvent->Default.m_Cmd )
		{
			case XCMD_CharListReq: // Second Login to select char, only valid if there is no char selected yet
			{
				EXC_SET("not logged - char list req");
				if ( ! xCheckMsgSize( sizeof( pEvent->CharListReq )))
					RETURN_FALSE();
				return( Setup_ListReq( pEvent->CharListReq.m_acctname, pEvent->CharListReq.m_acctpass, false ) == LOGIN_SUCCESS);
			}
			case XCMD_ExtData:
				EXC_SET("no char - ext data");
				if ( ! xCheckMsgSize(3))
					RETURN_FALSE();
				if ( ! xCheckMsgSize( pEvent->ExtData.m_len ))
					RETURN_FALSE();
				return 1;
			case XCMD_ExtAosData:
				EXC_SET("no char - ext aos");
				if ( ! xCheckMsgSize(3))
					RETURN_FALSE();
				if ( ! xCheckMsgSize( pEvent->ExtAosData.m_len ))
					RETURN_FALSE();
				return 1;
		}
		RETURN_FALSE();
	}

	//////////////////////////////////////////////////////
	// We are now playing.

	switch ( pEvent->Default.m_Cmd )
	{
		case XCMD_Walk: // Walk
			EXC_SET("walk");
			if ( ! xCheckMsgSize(sizeof(pEvent->Walk)))
				RETURN_FALSE();
			Event_Walking( pEvent->Walk.m_dir, pEvent->Walk.m_count, pEvent->Walk.m_cryptcode );
			break;
		case XCMD_Talk: // Speech or at least text was typed.
			EXC_SET("talk");
			if ( ! xCheckMsgSize(3))
				RETURN_FALSE();
			if ( ! xCheckMsgSize(pEvent->Talk.m_len ))
				RETURN_FALSE();
			Event_Talk( pEvent->Talk.m_text, pEvent->Talk.m_wHue, (TALKMODE_TYPE)( pEvent->Talk.m_mode ));
			break;
		case XCMD_Attack: // Attack
			EXC_SET("attack");
			if ( ! xCheckMsgSize( sizeof( pEvent->Click )))
				RETURN_FALSE();
			Event_Attack( (DWORD) pEvent->Click.m_UID );
			break;
		case XCMD_DClick:// Doubleclick
			EXC_SET("dclick");
			if ( ! xCheckMsgSize( sizeof( pEvent->Click )))
				RETURN_FALSE();
			Event_DoubleClick( ((DWORD)(pEvent->Click.m_UID)) &~ UID_F_RESOURCE, ((DWORD)(pEvent->Click.m_UID)) & UID_F_RESOURCE, true );
			break;
		case XCMD_ItemPickupReq: // Pick up Item
			EXC_SET("pickup item");
			if ( ! xCheckMsgSize( sizeof( pEvent->ItemPickupReq )))
				RETURN_FALSE();
			Event_Item_Pickup( (DWORD) pEvent->ItemPickupReq.m_UID, pEvent->ItemPickupReq.m_amount );
			break;
		case XCMD_ItemDropReq: // Drop Item
			EXC_SET("drop item");
			if ( IsClientVersion(0x0600018) || m_reportedCliver >= 0x0600018 || IsClientKR() )
			{
				if ( ! xCheckMsgSize( sizeof( pEvent->ItemDropReqNew ) ) )
					RETURN_FALSE();
			}
			else
			{
				if ( ! xCheckMsgSize( sizeof( pEvent->ItemDropReq )))
					RETURN_FALSE();
			}
			Event_Item_Drop(pEvent);
			break;
		case XCMD_Click: // Singleclick
			EXC_SET("singleclick");
			if ( ! xCheckMsgSize( sizeof( pEvent->Click )))
				RETURN_FALSE();
			Event_SingleClick( (DWORD) pEvent->Click.m_UID );
			break;
		case XCMD_ExtCmd: // Ext. Command
			EXC_SET("ext cmd");
			if ( ! xCheckMsgSize(3))
				RETURN_FALSE();
			if ( ! xCheckMsgSize( pEvent->ExtCmd.m_len ))
				RETURN_FALSE();
			Event_ExtCmd( (EXTCMD_TYPE) pEvent->ExtCmd.m_type, pEvent->ExtCmd.m_name );
			break;
		case XCMD_ItemEquipReq: // Equip Item
			EXC_SET("equip item");
			if ( ! xCheckMsgSize( sizeof( pEvent->ItemEquipReq )))
				RETURN_FALSE();
			Event_Item_Equip(pEvent);
			break;
		case XCMD_WalkAck: // Resync Request
			EXC_SET("resync req");
			if ( ! xCheckMsgSize( sizeof( pEvent->WalkAck )))
				RETURN_FALSE();
			addReSync();
			break;
		case XCMD_DeathMenu:	// DeathOpt (un)Manifest ghost (size anomoly)
			EXC_SET("death menu");
			if ( ! xCheckMsgSize(2))
				RETURN_FALSE();
			if ( ! Event_DeathOption( (DEATH_MODE_TYPE) pEvent->DeathMenu.m_mode, pEvent ))
				RETURN_FALSE();
			break;
		case XCMD_CharStatReq: // Status Request
			EXC_SET("status req");
			if ( ! xCheckMsgSize( sizeof( pEvent->CharStatReq )))
				RETURN_FALSE();
			if ( pEvent->CharStatReq.m_type == 4 )
			{
				addCharStatWindow( (DWORD) pEvent->CharStatReq.m_UID, true );
			}
			if ( pEvent->CharStatReq.m_type == 5 )
				addSkillWindow(SKILL_QTY);
			break;
		case XCMD_Skill:	// Skill locking.
			EXC_SET("skill lock");
			if ( ! xCheckMsgSize(3))
				RETURN_FALSE();
			if ( ! xCheckMsgSize( pEvent->Skill.m_len ))
				RETURN_FALSE();
			Event_Skill_Locks(pEvent);
			break;
		case XCMD_VendorBuy:	// Buy item from vendor.
			EXC_SET("vendor buy");
			if ( ! xCheckMsgSize(3))
				RETURN_FALSE();
			if ( ! xCheckMsgSize( pEvent->VendorBuy.m_len ))
				RETURN_FALSE();
			Event_VendorBuy( (DWORD) pEvent->VendorBuy.m_UIDVendor, pEvent );
			break;
		case XCMD_MapEdit:	// plot course on map.
			EXC_SET("plot map course");
			if ( ! xCheckMsgSize( sizeof( pEvent->MapEdit )))
				RETURN_FALSE();
			Event_MapEdit( (DWORD) pEvent->MapEdit.m_UID, pEvent );
			break;
		case XCMD_BookPage: // Read/Change Book
			EXC_SET("read/edit book");
			if ( ! xCheckMsgSize(3))
				RETURN_FALSE();
			if ( ! xCheckMsgSize( pEvent->BookPage.m_len ))
				RETURN_FALSE();
			Event_Book_Page( (DWORD) pEvent->BookPage.m_UID, pEvent );
			break;
		case XCMD_Options: // Options set UNSUPPORTED
			EXC_SET("set opt");
			if ( ! xCheckMsgSize(3))
				RETURN_FALSE();
			if ( ! xCheckMsgSize( pEvent->Options.m_len ))
				RETURN_FALSE();
			DEBUG_MSG(( "%x:XCMD_Options len=%d\n", m_Socket.GetSocket(), pEvent->Options.m_len ));
			break;
		case XCMD_Target: // Targeting
			EXC_SET("target");
			if ( ! xCheckMsgSize( sizeof( pEvent->Target )))
				RETURN_FALSE();
			Event_Target( pEvent );
			break;
		case XCMD_SecureTrade: // Secure trading
			EXC_SET("secure trade");
			if ( ! xCheckMsgSize(3))
				RETURN_FALSE();
			if ( ! xCheckMsgSize( pEvent->SecureTrade.m_len ))
				RETURN_FALSE();
			Event_SecureTrade( (DWORD) pEvent->SecureTrade.m_UID, pEvent );
			break;
		case XCMD_BBoard: // BBoard Request.
			EXC_SET("bboard");
			if ( ! xCheckMsgSize(3))
				RETURN_FALSE();
			if ( ! xCheckMsgSize( pEvent->BBoard.m_len ))
				RETURN_FALSE();
			Event_BBoardRequest( (DWORD) pEvent->BBoard.m_UID, pEvent );
			break;
		case XCMD_War: // Combat Mode
			EXC_SET("combat mode");
			if ( ! xCheckMsgSize( sizeof( pEvent->War )))
				RETURN_FALSE();
			Event_CombatMode( pEvent->War.m_warmode );
			break;
		case XCMD_CharName: // Rename Character(pet)
			{
				EXC_SET("rename pet");
				if ( ! xCheckMsgSize( sizeof( pEvent->CharName )))
					RETURN_FALSE();
				char *zCharName = Str_GetTemp();
				strcpylen(zCharName, pEvent->CharName.m_charname, MAX_NAME_SIZE);
				Event_SetName((DWORD)pEvent->CharName.m_UID, zCharName);
			}
			break;
		case XCMD_MenuChoice: // Menu Choice
				EXC_SET("menu choice");
			if ( ! xCheckMsgSize( sizeof( pEvent->MenuChoice )))
				RETURN_FALSE();
			Event_MenuChoice(pEvent);
			break;
		case XCMD_BookOpen:	// Change a books title/author.
			EXC_SET("open book");
			if ( ! xCheckMsgSize( sizeof(pEvent->BookOpen)))
				RETURN_FALSE();
			Event_Book_Title((DWORD)pEvent->BookOpen.m_UID, pEvent->BookOpen.m_title, pEvent->BookOpen.m_author);
			break;
		case XCMD_DyeVat: // Color Select Dialog
			EXC_SET("color dialog");
			if ( ! xCheckMsgSize( sizeof( pEvent->DyeVat )))
				RETURN_FALSE();
			Event_Item_Dye( (DWORD) pEvent->DyeVat.m_UID, pEvent->DyeVat.m_wHue );
			break;
		case XCMD_Prompt: // Response to console prompt.
			EXC_SET("console resp");
			if ( ! xCheckMsgSize(3))
				RETURN_FALSE();
			if ( ! xCheckMsgSize( pEvent->Prompt.m_len ))
				RETURN_FALSE();
			Event_PromptResp( pEvent->Prompt.m_text, pEvent->Prompt.m_len-sizeof(pEvent->Prompt));
			break;
		case XCMD_HelpPage: // GM Page (i want to page a gm!)
			{
				EXC_SET("help page");
				if ( ! xCheckMsgSize( sizeof( pEvent->HelpPage )))
					RETURN_FALSE();
				if ( !m_pChar )
					return 0;
				CScript script("HelpPage");
				m_pChar->r_Verb( script, this );
				break;
			}
		case XCMD_VendorSell: // Vendor Sell
			EXC_SET("vendor sell");
			if ( ! xCheckMsgSize(3))
				RETURN_FALSE();
			if ( ! xCheckMsgSize( pEvent->VendorSell.m_len ))
				RETURN_FALSE();
			Event_VendorSell( (DWORD) pEvent->VendorSell.m_UIDVendor, pEvent );
			break;
		case XCMD_Scroll:	// Scroll Closed
			EXC_SET("scroll");
			if ( !xCheckMsgSize( sizeof(pEvent->Scroll)) )
				RETURN_FALSE();
			// void
			break;
		case XCMD_GumpInpValRet:	// Gump text input
			EXC_SET("gumptext input");
			if ( ! xCheckMsgSize(3))
				RETURN_FALSE();
			if ( ! xCheckMsgSize( pEvent->GumpInpValRet.m_len ))
				RETURN_FALSE();
			Event_GumpInpValRet(pEvent);
			break;
		case XCMD_TalkUNICODE:	// Talk unicode.
			EXC_SET("unicode talk");
			if ( ! xCheckMsgSize(3))
				RETURN_FALSE();
			if ( ! xCheckMsgSize(pEvent->TalkUNICODE.m_len ))
				RETURN_FALSE();
			Event_TalkUNICODE(pEvent);
			break;
		case XCMD_GumpDialogRet:	// Gump menu.
			EXC_SET("dialog ret");
			if ( ! xCheckMsgSize(3))
				RETURN_FALSE();
			if ( ! xCheckMsgSize( pEvent->GumpDialogRet.m_len ))
				RETURN_FALSE();
			Event_GumpDialogRet(pEvent);
			break;
		case XCMD_ChatText:	// ChatText
			EXC_SET("chat text");
			if ( ! xCheckMsgSize(3))
				RETURN_FALSE();
			if ( ! xCheckMsgSize( pEvent->ChatText.m_len ))
				RETURN_FALSE();
			Event_ChatText( pEvent->ChatText.m_utext, pEvent->ChatText.m_len, CLanguageID( pEvent->ChatText.m_lang ));
			break;
		case XCMD_Chat: // Chat
			EXC_SET("chat");
			if ( ! xCheckMsgSize( sizeof( pEvent->Chat)))
				RETURN_FALSE();
			Event_ChatButton(pEvent->Chat.m_uname);
			break;
		case XCMD_ToolTipReq:	// Tool Tip
			EXC_SET("tooltip");
			if ( ! xCheckMsgSize( sizeof( pEvent->ToolTipReq )))
				RETURN_FALSE();
			Event_ToolTip( (DWORD) pEvent->ToolTipReq.m_UID );
			break;
		case XCMD_CharProfile:	// Get Character Profile.
			EXC_SET("profile");
			if ( ! xCheckMsgSize(3))
				RETURN_FALSE();
			if ( ! xCheckMsgSize( pEvent->CharProfile.m_len ))
				RETURN_FALSE();
			Event_Profile( pEvent->CharProfile.m_WriteMode, (DWORD) pEvent->CharProfile.m_UID, pEvent );
			break;
		case XCMD_MailMsg:
			EXC_SET("mailmsg");
			if ( ! xCheckMsgSize( sizeof(pEvent->MailMsg)))
				RETURN_FALSE();
			Event_MailMsg( (DWORD) pEvent->MailMsg.m_uid1, (DWORD) pEvent->MailMsg.m_uid2 );
			break;
		case XCMD_ClientVersion:	// Client Version string packet
			EXC_SET("client version");
			if ( ! xCheckMsgSize(3))
				RETURN_FALSE();
			if ( ! xCheckMsgSize(pEvent->ClientVersion.m_len))
				RETURN_FALSE();
			Event_ClientVersion( pEvent->ClientVersion.m_text, pEvent->ClientVersion.m_len );
			break;
		case XCMD_ViewRange:
			EXC_SET("view range");
			if ( !xCheckMsgSize(2) )
				RETURN_FALSE();
			if ( !xCheckMsgSize(sizeof(pEvent->ViewRange)) )
				RETURN_FALSE();
			// void
			return 1;
		case XCMD_ExtData:	// Add someone to the party system.
			EXC_SET("ext data");
			if ( ! xCheckMsgSize(3))
				RETURN_FALSE();
			if ( ! xCheckMsgSize( pEvent->ExtData.m_len ))
				RETURN_FALSE();
			Event_ExtData( (EXTDATA_TYPE)(WORD) pEvent->ExtData.m_type, &(pEvent->ExtData.m_u), pEvent->ExtData.m_len-5 );
			break;
		case XCMD_ExtAosData:	// Add someone to the party system.
			EXC_SET("ext aos");
			if ( ! xCheckMsgSize(3))
				RETURN_FALSE();
			if ( ! xCheckMsgSize( pEvent->ExtAosData.m_len ))
				RETURN_FALSE();
			Event_ExtAosData( (EXTAOS_TYPE)(WORD) pEvent->ExtAosData.m_type, &(pEvent->ExtAosData.m_u), (DWORD) pEvent->ExtAosData.m_uid, pEvent->ExtAosData.m_len-9 );
			break;

		case XCMD_AOSTooltip:
			{
				EXC_SET("tooltip request");
				if ( ! xCheckMsgSize(3))
					RETURN_FALSE();
				if ( ! xCheckMsgSize( pEvent->AosItemInfoRequest.m_len ))
					RETURN_FALSE();
				WORD wLen = ((WORD) pEvent->AosItemInfoRequest.m_len) - 3;
				if ( wLen < 0 || (wLen%4) != 0 )
					RETURN_FALSE();
				Event_AOSItemInfo( (int)(wLen/4), (NDWORD *) &pEvent->AosItemInfoRequest.m_uid );
			} break;

		case XCMD_AllNames3D:
			{
				EXC_SET("allnames 3D");
				if ( ! xCheckMsgSize(3))
					RETURN_FALSE();
				if ( ! xCheckMsgSize( pEvent->AllNames3D.m_len ))
					RETURN_FALSE();
				Event_AllNames3D( (DWORD) pEvent->AllNames3D.m_UID );
			} break;

		case XCMD_BugReport:
			{
				EXC_SET("bugreport");
				if ( ! xCheckMsgSize(3))
					RETURN_FALSE();
				if ( ! xCheckMsgSize( pEvent->BugReport.m_len ))
					RETURN_FALSE();
				if ( ! IsClientKR() )
					RETURN_FALSE();

				Event_BugReport( pEvent->BugReport.m_utext, pEvent->BugReport.m_len, (BUGREPORT_TYPE)(WORD)pEvent->BugReport.m_type, CLanguageID( pEvent->BugReport.m_Language ) );
			} break;

		case XCMD_MacroEquipItem:
			{
				EXC_SET("macro - equip items");
				if ( !xCheckMsgSize(3) )
					RETURN_FALSE();
				if ( !xCheckMsgSize(pEvent->MacroEquipItems.m_len) )
					RETURN_FALSE();
				if ( ! IsClientKR() )
					RETURN_FALSE();

				Event_MacroEquipItems(pEvent->MacroEquipItems.m_items, pEvent->MacroEquipItems.m_count);
			} break;

		case XCMD_MacroUnEquipItem:
			{
				EXC_SET("macro - unequip items");
				if ( !xCheckMsgSize(3) )
					RETURN_FALSE();
				if ( !xCheckMsgSize(pEvent->MacroEquipItems.m_len) )
					RETURN_FALSE();
				if ( ! IsClientKR() )
					RETURN_FALSE();

				Event_MacroUnEquipItems(pEvent->MacroUnEquipItems.m_layers, pEvent->MacroUnEquipItems.m_count);
			} break;

		case XCMD_KRCharListUpdate:
			{
				if ( !xCheckMsgSize(3) )
					RETURN_FALSE();
				if ( !xCheckMsgSize(pEvent->KRCharListUpdate.m_len) )
					RETURN_FALSE();
				if ( ! IsClientKR() )
					RETURN_FALSE();

				DEBUG_WARN(("%x:KRCharListUpdate packet (0x%x) received.\n", m_Socket.GetSocket(), pEvent->Default.m_Cmd ));
				xDumpPacket( m_bin.GetDataQty(), pEvent->m_Raw );
			} break;

		case XCMD_UseHotbar:
			{
				if ( !xCheckMsgSize(11) )
					RETURN_FALSE();
				if (( pEvent->UseHotbar.m_One != 0x01 ) || ( pEvent->UseHotbar.m_One != 0x06 ))
					RETURN_FALSE();
				if ( ! IsClientKR() )
					RETURN_FALSE();

				Event_UseToolbar(pEvent->UseHotbar.m_Type, pEvent->UseHotbar.m_ObjectUID);
			} break;

		case XCMD_HighlightUIRemove:
			{
				if ( !xCheckMsgSize(3) )
					RETURN_FALSE();
				if ( ! IsClientKR() )
					RETURN_FALSE();

				DEBUG_WARN(("%x:Unimplemented KR packet (0x%x) received.\n", m_Socket.GetSocket(), pEvent->Default.m_Cmd ));
			} break;

		default:
			EXC_SET("unknown");
			DEBUG_WARN(( "%x:Unknown game packet (0x%x) received.\n", m_Socket.GetSocket(), pEvent->Default.m_Cmd ));
			RETURN_FALSE();
	}

	// This is the last message we are pretty sure we got correctly.
	m_bin_PrvMsg = (XCMD_TYPE) pEvent->Default.m_Cmd;
	return 1;
	EXC_CATCH;

	EXC_DEBUG_START;
	m_packetExceptions++;
	g_Log.EventDebug("account '%s'\n", GetAccount() ? GetAccount()->GetName() : "");
	if( m_packetExceptions > 10 )
	{
		g_Log.EventWarn("Disconnecting client from account '%s' since it is causing exceptions problems\n",
			GetAccount() ? GetAccount()->GetName() : "");
		addKick(&g_Serv, false);
	}
	EXC_DEBUG_END;
	return 0;
}


