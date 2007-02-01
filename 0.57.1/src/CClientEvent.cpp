#include "graysvr.h"
#include "CClient.h"
#include <wchar.h>
#include "abilities.h"
#include "network/network.h"
#include "network/send.h"

/////////////////////////////////
// Events from the Client.

void CClient::Event_ChatButton(const NCHAR * pszName) // Client's chat button was pressed
{
	// See if they've made a chat name yet
	// m_ChatPersona.SetClient(this);

	if (m_pChar->OnTrigger(CTRIG_UserChatButton, m_pChar) == TRIGRET_RET_TRUE)
		return;

	if ( GetAccount()->m_sChatName.IsEmpty())
	{
		// No chatname yet, see if the client sent one
		if (pszName[0] == 0) // No name was sent, so ask for a permanent chat system nickname (account based)
		{
			addChatSystemMessage(CHATMSG_GetChatName);
			return;
		}
		// OK, we got a nick, now store it with the account stuff.

		// Make it non Unicode
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
	// Just send it all to the chat system
	g_Serv.m_Chats.EventMsg( this, pszText, len, lang );
}

void CClient::Event_MapEdit( UID uid, const CEvent * pEvent )
{
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

void CClient::Event_Item_Dye( UID uid, HUE_TYPE wHue ) // Rehue an item
{
	// CLIMODE_DYE : Result from addDyeOption()
	CObjBase	*pObj = uid.ObjFind();
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
		if ( pObj->GetBaseID() != 0xFAB )
			return;
		if ( wHue < HUE_BLUE_LOW )
			wHue = HUE_BLUE_LOW;
		if ( wHue > HUE_DYE_HIGH )
			wHue = HUE_DYE_HIGH;
	}
	else if ( pObj->IsChar() )
	{
		pObj->RemoveFromView();
		wHue |= HUE_UNDERWEAR;
	}

	pObj->SetHue(wHue);
	pObj->Update();
}

void CClient::Event_Book_Title( UID uid, LPCTSTR pszTitle, LPCTSTR pszAuthor )
{
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

void CClient::Event_Book_Page( UID uid, const CEvent * pEvent ) // Book window
{
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
	int iLines = min(pEvent->BookPage.m_page[0].m_lines, config.get("client.book.maxlines"));

	if ( !pText || !pBook->IsBookWritable() )
		return;
	if (( iLines <= 0 ) || ( iPage <= 0 ) || iPage > config.get("client.book.maxpages") )
		return;
	iPage--;

	int		len = 0;
	int		maxlen = pEvent->BookPage.m_len - sizeof(NDWORD) - sizeof(NWORD)*4 - sizeof(BYTE);

	if (( maxlen <= 0 ) || ( maxlen >= SCRIPT_MAX_LINE_LEN ))
		return;

	TEMPSTRING(pszTemp);
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

void CClient::Event_Item_Pickup(UID uid, int amount) // Client grabs an item
{
	// Player/client is picking up an item.

	CItem	*pItem = uid.ItemFind();
	if ( !pItem || pItem->IsWeird() )
	{
		addObjectRemove(uid);
cancel:	addItemDragCancel(0);
		return;
	}

	//	fastloot (,emptycontainer) protection
	if ( m_tNextPickup > m_tNextPickup.GetCurrentTime() )
		goto cancel;
	m_tNextPickup = m_tNextPickup.GetCurrentTime() + 3;

	// Where is the item coming from ? (just in case we have to toss it back)
	CObjBase * pObjParent = dynamic_cast <CObjBase *>(pItem->GetParent());
	m_Targ_PrvUID = pObjParent ? pObjParent->uid() : UID_CLEAR;
	m_Targ_p = pItem->GetUnkPoint();

	amount = m_pChar->ItemPickup(pItem, amount);
	if ( amount < 0 )
		goto cancel;
	else if ( amount > 1 )
		m_tNextPickup = m_tNextPickup + 2;	// +100 msec if amount should slow down the client

	SetTargMode(CLIMODE_DRAG);
	m_Targ_UID = uid;
}



void CClient::Event_Item_Drop( const CEvent * pEvent ) // Item is dropped
{
	// This started from the Event_Item_Pickup()
	if ( !m_pChar )
		return;

	UID uidItem( pEvent->ItemDropReq.m_UID );
	CItem * pItem = uidItem.ItemFind();
	UID uidOn( pEvent->ItemDropReq.m_UIDCont );	// dropped on this item.
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
			goto cantdrop;
		}

		if ( pObjOn->IsChar())	// Drop on a chars head.
		{
			CChar * pChar = dynamic_cast <CChar*>( pObjOn );
			if ( pChar != m_pChar )
			{
				if ( ! Cmd_SecureTrade( pChar, pItem ))
					goto cantdrop;
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
			if ( ! pChar->NPC_IsOwnedBy( m_pChar ))
			{
				// Slyly dropping item in someone elses pack.
				// or just dropping on their trade window.
				if ( ! Cmd_SecureTrade( pChar, pItem ))
					goto cantdrop;
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
					goto cantdrop;
			}
			else if ( ! pChar->CanCarry( pItem ))
			{
				// SysMessage( "That is too heavy" );
				goto cantdrop;
			}
		}
		if ( pContItem != NULL )
		{
			//	bug with shifting selling list by gold coins
			if ( pContItem->IsType(IT_EQ_VENDOR_BOX) &&
				( pItem->IsType(IT_GOLD) || pItem->IsType(IT_COIN) ))
			{
				goto cantdrop;
			}
		}

		CObjBase *pOldCont = pItem->GetContainer();
		CScriptTriggerArgs Args( pObjOn );
		if ( pItem->OnTrigger( ITRIG_DROPON_ITEM, m_pChar, &Args ) == TRIGRET_RET_TRUE )
			goto cantdrop;

		if ( pOldCont != pItem->GetContainer() )
			return;

		CItem * pItemOn = dynamic_cast <CItem*> ( pObjOn );
		if ( pItemOn )
		{
			CScriptTriggerArgs Args( pItem );
			if ( pItemOn->OnTrigger( ITRIG_DROPON_SELF, m_pChar, &Args ) == TRIGRET_RET_TRUE )
				goto cantdrop;
		}

		if ( pContItem != NULL )
		{
			// pChar->GetBank()->IsItemInside( pContItem )
			bool isCheating = false;
			bool isBank = pContItem->IsType( IT_EQ_BANK_BOX );
			
			if ( isBank )
				isCheating = pContItem->m_itEqBankBox.m_pntOpen != m_pChar->GetTopPoint();
			else
				isCheating = m_pChar->GetBank()->IsItemInside( pContItem ) && 
						m_pChar->GetBank()->m_itEqBankBox.m_pntOpen != m_pChar->GetTopPoint();
						
			if ( isCheating )
				goto cantdrop;
			if ( !pContItem->CanContainerHold(pItem, m_pChar) )
				goto cantdrop;
			
			// only IT_GAME_PIECE can be dropped on IT_GAME_BOARD or clients will crash
			if (pContItem->IsType( IT_GAME_BOARD ) && !pItem->IsType( IT_GAME_PIECE ))
				goto cantdrop;
			
			// non-vendable items should never be dropped inside IT_EQ_VENDOR_BOX
			if ( pContItem->IsType( IT_EQ_VENDOR_BOX ) &&  !pItem->Item_GetDef()->GetMakeValue(0) )
			{
				SysMessageDefault( DEFMSG_ERR_NOT4SALE );
				goto cantdrop;
			}
		}
		else
		{
			// dropped on top of a non container item.
			// can i pile them ?
			// Still in same container.

			pObjOn = pItemOn->GetContainer();
			pt = pItemOn->GetUnkPoint();

			if ( ! pItem->Stack( pItemOn ))
			{
				if ( pItemOn->IsTypeSpellbook() )
				{
					if ( pItemOn->AddSpellbookScroll( pItem ))
					{
						SysMessage( "Can't add this to the spellbook" );
						goto cantdrop;
					}
					// We only need to add a sound here if there is no
					// scroll left to bounce back.
					if (pItem->IsDeleted())
						addSound( 0x057, pItemOn );	// add to inv sound.
					return;
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
	cantdrop:
			// The item was in the LAYER_DRAGGING.
			// Try to bounce it back to where it came from.
			if ( pItem == m_pChar->LayerFind( LAYER_DRAGGING ) )	// if still being dragged
				m_pChar->ItemBounce( pItem );
			return;
		}
	}

	// Game pieces can only be droped on their game boards.
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
		pContOn->ContentAdd( pItem, pt );
		addSound( pItem->GetDropSound( pObjOn ));
	}
	else
	{
		// on ground
		m_pChar->UpdateDrag( pItem, NULL, &pt );
		m_pChar->ItemDrop( pItem, pt );
	}
}


void CClient::Event_Skill_Locks( const CEvent * pEvent )
{
	// Skill lock buttons in the skills window.
	if ( !GetChar() || !GetChar()->m_pPlayer )
		return;

	int len = pEvent->Skill.m_len;
	len -= 3;
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
	// All the push button skills come through here.
	// Any "Last skill" macro comes here as well. (push button only)

	bool fContinue = false;

	if ( m_pChar->Skill_Wait(skill) )
		return;

	if ( m_pChar->Skill_OnTrigger( skill, SKTRIG_SELECT ) == TRIGRET_RET_TRUE )
	{
		m_pChar->Skill_Fail( true );	// clean up current skill.
		return;
	}

	SetTargMode();
	m_Targ_UID.InitUID();	// This is a start point for targ more.

	bool fCheckCrime	= false;

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

dotargetting:
		// Go into targtting mode.
		if ( g_Cfg.GetSkillDef(skill)->m_sTargetPrompt.IsEmpty() )
		{
			g_Log.Error("%x: Event_Skill_Use bad skill %d\n", socketId(), skill);
			return;
		}

		m_tmSkillTarg.m_Skill = skill;	// targetting what skill ?
		addTarget( CLIMODE_TARG_SKILL, g_Cfg.GetSkillDef(skill)->m_sTargetPrompt, false, fCheckCrime );
		return;

	case SKILL_STEALING:
	case SKILL_ENTICEMENT:
	case SKILL_PROVOCATION:
	case SKILL_POISONING:
		// Go into targtting mode.
		fCheckCrime = true;
		goto dotargetting;

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
}

bool CClient::Event_Walking(BYTE direction)
{
	if ( !m_pChar || ( direction != ( direction & 0x87) ))
		return false;

	// Movement whilst precasting is not allowed
	if ( IsSetMagicFlags( MAGICF_PRECAST ) && m_pChar->m_Act_SkillCurrent == SKILL_MAGERY )
	{
		SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_FROZEN ) );
		return false;
	}

	if (( m_pChar->IsStatFlag(STATF_Freeze|STATF_Stone) && m_pChar->OnFreezeCheck() ) || m_pChar->OnFreezeCheck(true) )
		return false;
	m_pChar->CheckRevealOnMove();

	DIR_TYPE dir = (DIR_TYPE)(direction & 0x0f);
	CPointMap pt = m_pChar->GetTopPoint();
	CPointMap ptold = pt;
	bool amTurning = ( dir != m_pChar->m_dirFace );
	bool amRunning = ( dir & 0x80 );

	if ( !amTurning )
	{
/*		LONGLONG	CurrTime	= GetTickCount();
		m_iWalkStepCount++;
		// Move in this dir.
		if ( ( m_iWalkStepCount % 7 ) == 0 )	// we have taken 8 steps ? direction changes don't count.
		{
			// Client only allows 4 steps of walk ahead.
			int		iTimeDiff	= ((CurrTime - m_timeWalkStep)/10);
			int		iTimeMin	= m_pChar->IsStatFlag( STATF_OnHorse ) ? 70 : 140;

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

			if ( m_iWalkTimeAvg < 0 && iTimeDiff >= 0 && ! IsPriv(PRIV_GM) )	// TICK_PER_SEC
			{
				// walking too fast.
				DEBUG_WARN(("%s (%x): Fast Walk ?\n", GetName(), socketId()));

				m_iWalkStepCount--; // eval again next time !
				return false;
			}
			m_timeWalkStep = CurrTime;
		}	// nth step
*/
		pt.Move(dir);

		if ( !m_pChar->CanMoveWalkTo(pt, true, false, dir) )
			return false;
		m_pChar->MoveToChar(pt);

		if ( m_pChar->CheckLocation() )		// stepped on telepad or so?
			return false;
	}
	else
		m_pChar->m_dirFace = dir;

	m_pChar->StatFlag_Mod(STATF_Fly, amRunning);

	PacketMovementAck *packet = new PacketMovementAck(this);
	if ( amTurning )
	{
		m_pChar->UpdateMode(this);			// show others I've turned
	}
	else
	{
		m_pChar->UpdateMove(ptold, this);	// who now sees me?
		addPlayerSee(ptold);				// what new stuff do I now see?
	}
	return true;
}

void CClient::Event_CombatMode( bool fWar ) // Only for switching to combat mode
{
	// If peacmaking then this doens't work ??
	// Say "you are feeling too peacefull"
	if ( IsTrigUsed(TRIGGER_USERWARMODE) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = m_pChar->IsStatFlag(STATF_War) ? 1 : 0;
		if (m_pChar->OnTrigger(CTRIG_UserWarmode, m_pChar, &Args) == TRIGRET_RET_TRUE)
			return;
	}

	m_pChar->StatFlag_Mod(STATF_War, fWar);

	if ( m_pChar->IsStatFlag(STATF_DEAD) )
	{
		// Manifest the ghost, war mode for ghosts.
		m_pChar->StatFlag_Mod(STATF_Insubstantial, !fWar);
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
		if ( m_pChar->r_Call(g_Cfg.m_sCommandTrigger, this, &Args, NULL, &tr) )
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
		static LPCTSTR const sm_szCmd_Redirect[] =		// default to redirect these.
		{
			"BANK",
			"DUPE",
			"FORGIVE",
			"JAIL",
			"KICK",
			"KILL",
			"NUDGEDOWN",
			"NUDGEUP",
			"REMOVE",
			"SHRINK",
		};
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

	g_Log.Event(LOGM_GM_CMDS, "%x:'%s' commands '%s'=%d\n", socketId(), GetName(), pszCommand, m_bAllowCommand);

	return !m_bAllowSay;
}

void CClient::Event_Attack( UID uid )
{
	// d-click in war mode
	// I am attacking someone.

	CChar *pChar = uid.CharFind();
	if ( !pChar )
		return;

	// Accept or decline the attack.
	CCommand cmd;
	cmd.AttackOK.m_Cmd = XCMD_AttackOK;
	cmd.AttackOK.m_UID = m_pChar->Fight_Attack(pChar) ? pChar->uid() : 0;
	xSend(&cmd, sizeof(cmd.AttackOK));
}

void CClient::Event_BBoardRequest( UID uid, const CEvent * pEvent )
{
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
	
	UID uidMsg( (DWORD)( pEvent->BBoard.m_UIDMsg ) );

	switch ( pEvent->BBoard.m_flag )
	{
		case BBOARDF_REQ_FULL:
		case BBOARDF_REQ_HEAD:
			// request for message header and/or body.
			if ( pEvent->BBoard.m_len != 0x0c )
				return;
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
					DEBUG_ERR(("%x:BBoard can't create message item\n", socketId()));
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
			DEBUG_ERR(("%x:BBoard unknown flag %d\n", socketId(), (int)pEvent->BBoard.m_flag));
			return;
	}
}



void CClient::Event_SecureTrade( UID uid, const CEvent * pEvent )
{
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
			VariableList::Variable *vardef = pCont->GetTagDefs()->GetKey("wait1sec");
			if ( vardef ) need2wait = vardef->GetValNum();
			if ( need2wait > 0 )
			{
				long timerow = g_World.GetCurrentTime().GetTimeRaw();
				if ( need2wait > timerow )
				{
					long		seconds = (need2wait-timerow)/TICK_PER_SEC;
					SysMessagef(g_Cfg.GetDefaultMsg( DEFMSG_TRADE_WAIT ), seconds);
					return;
				}
			}
			pCont->Trade_Status( pEvent->SecureTrade.m_UID1 );
			return;
	}
}



void CClient::Event_Profile( BYTE fWriteMode, UID uid, const CEvent * pEvent )
{
	// mode = 0 = Get profile, 1 = Set profile

	CChar	*pChar = uid.CharFind();
	if ( !pChar || !pChar->m_pPlayer )
		return;

	if ( IsTrigUsed(TRIGGER_PROFILE) )
	{
		if ( pChar->OnTrigger(CTRIG_Profile, m_pChar) == TRIGRET_RET_TRUE )
			return;
	}

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
		bool fIncognito = m_pChar->IsStatFlag( STATF_Incognito ) && ! IsPriv(PRIV_GM);

		CGString sConstText;
		CCommand cmd;

		cmd.CharProfile.m_Cmd = XCMD_CharProfile;
		cmd.CharProfile.m_UID = uid;

		int len = strcpylen(cmd.CharProfile.m_title, pChar->GetName()) + 1;

		sConstText.Format( "%s, %s", pChar->Noto_GetTitle(), pChar->GetTradeTitle());

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
		xSend(&cmd, len);
	}
}



void CClient::Event_MailMsg( UID uid1, UID uid2 )
{
	// NOTE: How do i protect this from spamming others !!!
	// Drag the mail bag to this clients char.

	CChar * pChar = uid1.CharFind();

	if ( pChar == NULL )
	{
		SysMessageDefault( DEFMSG_MAILBAG_DROP_1 );
		return;
	}

	if ( IsTrigUsed(TRIGGER_USERMAILBAG) )
	{
		if ( pChar->OnTrigger(CTRIG_UserMailBag, m_pChar, NULL) == TRIGRET_RET_TRUE )
			return;
	}

	if ( pChar == m_pChar ) // this is normal (for some reason) at startup.
	{
		return;
	}
	// Might be an NPC ?
	pChar->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MAILBAG_DROP_2), m_pChar->GetName());
}

void CClient::Event_PromptResp( LPCTSTR pszText, int len )
{
	// result of addPrompt
	TCHAR szText[MAX_TALK_BUFFER];

	if ( Str_Check( pszText ) )
		return;

	if ( len <= 0 )	// cancel
		szText[0] = 0;
	else
		len = Str_GetBare( szText, pszText, sizeof(szText), "|~,=[]{|}~" );

	LPCTSTR pszReName = NULL;
	LPCTSTR pszPrefix = NULL;

	CLIMODE_TYPE PrvTargMode = GetTargMode();
	ClearTargMode();

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
			pszReName = "Rune";
			pszPrefix = "Rune to:";
			break;
	
		case CLIMODE_PROMPT_NAME_KEY:
			pszReName = "Key";
			pszPrefix = "Key to:";
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
			pszReName = "Stone";
			pszPrefix = "Stone for the ";
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
				if ( szText[0] != '\0' )	// cancel
				{
				// CChar * pChar = m_Targ_PrvUID.CharFind();
					CScript script( m_Targ_Text, szText );
					if ( m_pChar )
						m_pChar->r_Verb( script, this );
				}
			}
			return;

		default:
			SysMessage("Unexpected prompt info");
			return;
	}

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
	case IT_STONE_GUILD:
	case IT_STONE_TOWN:
		{
			CItemStone * pStone = dynamic_cast <CItemStone*> ( pItem );
			if ( !pStone || !pStone->OnPromptResp(this, PrvTargMode, szText, sMsg) )
				return;
		}
		break;
	default:
		pItem->SetName(sMsg);
		sMsg.Format(g_Cfg.GetDefaultMsg(DEFMSG_RENAME_SUCCESS), pszReName, pItem->GetName());
		break;
	}

	SysMessage(sMsg);
}




void CClient::Event_Talk_Common(char *szText) // PC speech
{
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

	CWorldSearch AreaChars(m_pChar->GetTopPoint(), UO_MAP_VIEW_SIGHT);
	while ( pChar = AreaChars.GetChar() )
	{
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
		else if ( ! strnicmp( szText, "ALL", 3 ))
			i = 4;
		else
		{
			// Named the char specifically ?
			i = pChar->NPC_OnHearName( szText );
			fNamed = true;
		}
		if ( i )
		{
			while ( isspace(szText[i]) ) i++;

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




void CClient::Event_Talk( LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, bool bNoStrip ) // PC speech
{
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

		if (    !g_Cfg.m_sSpeechSelf.IsEmpty()
			&& m_pChar->OnTriggerSpeech(g_Cfg.m_sSpeechSelf, (TCHAR *)pszText, m_pChar, mode, wHue) )
			fCancelSpeech	= true;

		g_Log.Event(LOGM_PLAYER_SPEAK, "%x:'%s' Says '%s' mode=%d%s\n",
			socketId(), m_pChar->GetName(), pszText, mode, fCancelSpeech ? " (muted)" : "");
		// TODO: Guild and Alliance mode will not pass this (for the moment)
		// TODO: Add some hardcoded things for guild.
		if ( mode == 13 || mode == 14 )
			return;

		strcpy(z, pszText);

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

		if ( !fCancelSpeech )
		{
			m_pChar->SpeakUTF8(z, wHue, (TALKMODE_TYPE)mode, m_pChar->m_fonttype, GetAccount()->m_lang);
			Event_Talk_Common((char *)z);
		}
	}
}

void CClient::Event_TalkUNICODE( const CEvent * pEvent )
{
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

	const NWORD *puText = &pEvent->TalkUNICODE.m_utext[0];
	if ( CvtNUNICODEToSystem(szText, sizeof(szText), pEvent->TalkUNICODE.m_utext, iLenChars) <= 0 )
		return;

	pszText = szText;
	GETNONWHITESPACE(pszText);

	if ( !Event_Command(pszText,Mode) )
	{
		bool	fCancelSpeech	= false;

		if (    !g_Cfg.m_sSpeechSelf.IsEmpty()
			&& m_pChar->OnTriggerSpeech( g_Cfg.m_sSpeechSelf, (TCHAR *) pszText, m_pChar, Mode, wHue) )
			fCancelSpeech	= true;

		g_Log.Event(LOGM_PLAYER_SPEAK, "%x:'%s' Says UNICODE '%s' '%s' mode=%d%s\n",
			socketId(), m_pChar->GetName(), pAccount->m_lang.GetStr(), pszText, Mode, fCancelSpeech ? " (muted)" : "" );
		// TODO: Guild and Alliance mode will not pass this (for the moment)
		// TODO: Add some hardcoded things for guild.
		if ( Mode == 13 || Mode == 14 )
			return;

		int chars = strlen(szText);
		int capitals = 0;
		int i = 0;
		for ( i = 0; i < chars; i++ )
			if (( szText[i] >= 'A' ) && ( szText[i] <= 'Z' ))
				capitals++;

		if (( chars > 5 ) && ((( capitals * 100 )/chars) > 75 ))
		{							// 80% of chars are in capital letters. lowercase it
			for ( i = 1; i < chars; i++ )				// instead of the 1st char
				if (( szText[i] >= 'A' ) && ( szText[i] <= 'Z' )) szText[i] += 0x20;
			CvtSystemToNUNICODE(wszText, iLenChars, szText, chars);
		}

		if ( !fCancelSpeech )
		{
			m_pChar->SpeakUTF8Ex(puText, wHue, Mode, m_pChar->m_fonttype, pAccount->m_lang);
			Event_Talk_Common((char *)pszText);
		}
	}
}

void CClient::Event_SetName( UID uid, const char * pszCharName )
{
	// Set the name in the character status window.
	CChar * pChar = uid.CharFind();
	if ( !pChar || !m_pChar )
		return;

   if ( Str_CheckName(pszCharName) || !strlen(pszCharName) )
		return;

	// Do we have the right to do this ?
	if ( m_pChar == pChar || ! pChar->NPC_IsOwnedBy( m_pChar, true ))
		return;
	if ( g_Cfg.IsObscene(pszCharName))
		return;

	CScriptTriggerArgs args;
	args.m_pO1 = pChar;
	args.m_s1 = pszCharName;
	if ( m_pChar->OnTrigger(CTRIG_Rename, this, &args) == TRIGRET_RET_TRUE ) 
		return;
	pChar->SetName(pszCharName);

	if ( pChar->m_pNPC )		// mark the name being changed
	{
		pChar->m_pNPC->m_renamed = true;
	}
}



void CClient::Event_MenuChoice( const CEvent * pEvent ) // Choice from GMMenu or Itemmenu received
{
	// Select from a menu. CMenuItem
	// result of addItemMenu call previous.
	// select = 0 = cancel.

	UID uidItem( pEvent->MenuChoice.m_UID );
	WORD context = pEvent->MenuChoice.m_context;

	if ( context != GetTargMode() || uidItem != m_tmMenu.m_UID )
	{
		SysMessage("Unexpected menu info");
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
		g_Log.Error("%x:Unknown Targetting mode for menu %d\n", socketId(), context);
		return;
	}
}



void CClient::Event_GumpInpValRet( const CEvent * pEvent )
{
	// Text was typed into the gump on the screen.
	// pEvent->GumpInpValRet
	// result of addGumpInputBox. GumpInputBox
	// ARGS:
	// 	m_Targ_UID = pObj->GetUID();
	//  m_Targ_Text = verb

	UID uidItem( pEvent->GumpInpValRet.m_UID );
	WORD context = pEvent->GumpInpValRet.m_context;	// word context is odd.

	BYTE retcode = pEvent->GumpInpValRet.m_retcode; // 0=canceled, 1=okayed
	WORD textlen = pEvent->GumpInpValRet.m_textlen; // length of text entered
	char pszText[30];
	strcpy(pszText, pEvent->GumpInpValRet.m_text);

	char *pFix;
	if ( ( pFix = strchr( pszText, '\n' ) ) )
		*pFix	= '\0';
	if ( ( pFix = strchr( pszText, '\r' ) ) )
		*pFix	= '\0';
	if ( ( pFix = strchr( pszText, '\t' ) ) )
		*pFix	= ' ';

	if ( GetTargMode() != CLIMODE_INPVAL || uidItem != m_Targ_UID )
	{
		SysMessage("Unexpected text input");
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
			SysMessagef("Invalid set: %s = %s", m_Targ_Text.GetPtr(), pszText);
		}
		else
		{
			if ( IsPriv( PRIV_DETAIL ))
			{
				SysMessagef("Set: %s = %s", m_Targ_Text.GetPtr(), pszText);
			}
			pObj->RemoveFromView(); // weird client thing
			pObj->Update();
		}

		g_Log.Event(LOGM_GM_CMDS, "%x:'%s' tweak uid=0%x (%s) to '%s %s'=%d\n",
			socketId(), GetName(), pObj->uid(), pObj->GetName(), m_Targ_Text.GetPtr(), pszText, fRet);
	}
}

bool CDialogResponseArgs::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
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
			return true;
		}
		else if ( ! strnicmp( pszKey, "ID", 2) )
		{
			pszKey += 2;

			if ( m_CheckArray[0] )
				sVal.FormatVal( m_CheckArray[0] );
			else
				sVal.FormatVal( -1 );

			return true;
		}

		int iNum = Exp_GetSingle( pszKey );
		SKIP_SEPARATORS(pszKey);
		for ( int i=0; i<iQty; i++ )
		{
			if ( iNum == m_CheckArray[i] )
			{
				sVal = "1";
				return true;
			}
		}
		sVal = "0";
		return true;
	}
	if ( ! strnicmp( pszKey, "ARGTXT", 6 ))
	{
		pszKey += 6;
		SKIP_SEPARATORS(pszKey);

		int iQty = m_TextArray.GetCount();
		if ( pszKey[0] == '\0' )
		{
			sVal.FormatVal(iQty);
			return true;
		}

		int iNum = Exp_GetSingle( pszKey );
		SKIP_SEPARATORS(pszKey);

		for ( int i=0; i<m_TextArray.GetCount(); i++ )
		{
			if ( iNum == m_TextArray[i]->m_ID )
			{
				sVal = m_TextArray[i]->m_sText;
				return true;
			}
		}
		sVal.Empty();
		return false;
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
	// CLIMODE_DIALOG
	// initiated by addGumpDialog()
	// A button was pressed in a gump on the screen.
	// possibly multiple check boxes.

	// First let's completely decode this packet
	UID	uid		( pEvent->GumpDialogRet.m_UID );
	DWORD	context		= pEvent->GumpDialogRet.m_context;
	DWORD dwButtonID	= pEvent->GumpDialogRet.m_buttonID;

	// relying on the context given by the gump might be a security problem, much like
	// relying on the uid returned.
	// maybe keep a memory for each gump?
	CObjBase * pObj = uid.ObjFind();

	// Virtue button -- Handleing this here because the packet is a little different and causes exceptions somewhere
	if ( ( context == CLIMODE_DIALOG_VIRTUE ) && ( (CObjBase *)m_pChar == pObj ) )
	{
		if ( IsTrigUsed(TRIGGER_USERVIRTUE) )
		{
			m_pChar->OnTrigger( CTRIG_UserVirtue, (CTextConsole *) m_pChar, NULL );
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
				g_Log.Event(LOGL_EVENT, "Gump: %d (%s), Uid: 0x%x, Button: %d.\n", context, pDlg->GetName(), (DWORD)uid, dwButtonID);
		}
	}
#endif

	// Sanity check
	CItemMemory * pCheckMemory = m_pChar->Memory_FindGump(context, (DWORD)uid);
	if ( pCheckMemory )
	{
		if ( context == CLIMODE_DIALOG_GUILD )
		{
			VariableList::Variable *sTempVal = pCheckMemory->GetTagDefs()->GetKey("targ_uid");
			if ( sTempVal )
			{
				m_Targ_UID = (DWORD)sTempVal->GetValNum();
			}
			else
			{
				m_Targ_UID = 0;
			}
		}

		pCheckMemory->Delete();
	}
	else
		return;

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
		TEMPSTRING(scratch);

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

	switch ( context ) // This is the page number
	{
		case CLIMODE_DIALOG_GUILD: // Guild/Leige/Townstones stuff comes here
			{
				CItemStone * pStone = dynamic_cast <CItemStone *> ( m_Targ_UID.ItemFind());
				if ( !pStone || pStone->OnDialogButton( this, (STONEDISP_TYPE) dwButtonID, resp ))
					return;
			}
			break;
	}

	RESOURCE_ID_BASE	rid	= RESOURCE_ID(RES_DIALOG,context);
	//
	// Call the scripted response. Lose all the checks and text.
	//
	Dialog_OnButton( rid, dwButtonID, pObj, &resp );
}




bool CClient::Event_DoubleClick( UID uid, bool fMacro, bool fTestTouch, bool fScript )
{
	// Try to use the object in some way.
	// will trigger a OnTarg_Use_Item() ussually.
	// fMacro = ALTP vs dbl click. no unmount.

	// Allow some static in game objects to have function?
	// Not possible with dclick.

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
				// in war mode not to drop from horse accidentaly we need this check
				// Should also check for STATF_War incase someone starts fight and runs away.
				if ( pChar->IsStatFlag(STATF_War) && pChar->Memory_FindTypes(MEMORY_FIGHT) )
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

	return true;
}




void CClient::Event_SingleClick( UID uid )
{
	// the ALLNAMES macro comes thru here.
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
	// XCMD_ClientVersion
	DEBUG_MSG(("%x:XCMD_ClientVersion %s\n", socketId(), pData));
	
	if ( Str_Check(pData) )
		return;

	if ( m_reportedCliver )
		return;
	
	TEMPSTRING(sTemp);
	memcpy(sTemp, pData, min(Len,20));
	
	if (strstr(sTemp, "UO:3D"))
		this->m_bClient3d = true;
	
	int len = Str_GetBare( sTemp, sTemp, min(Len,10), " '`-+!\"#$%&()*,/:;<=>?@[\\]^{|}~" );
	if ( len )
	{
		m_reportedCliver = CCrypt::GetVerFromString(sTemp);
		DEBUG_MSG(( "Getting cliver 0x%x/0x%x\n",m_reportedCliver, (m_reportedCliver&0xFFFFF0) ));
	}
}

void CClient::Event_Target( const CEvent * pEvent )
{
	// XCMD_Target
	// If player clicks on something with the targetting cursor
	// Assume addTarget was called before this.
	// NOTE: Make sure they can actually validly trarget this item !

	if ( pEvent->Target.m_context != GetTargMode())
	{
		SysMessage("Unexpected target info");
		return;
	}
	if ( pEvent->Target.m_x == 0xFFFF && pEvent->Target.m_UID == 0 )
	{
		// canceled
		SetTargMode();
		return;
	}

	UID uid( pEvent->Target.m_UID );
	CPointMap pt( pEvent->Target.m_x, pEvent->Target.m_y, pEvent->Target.m_z, m_pChar->GetTopMap() );
	ITEMID_TYPE id = (ITEMID_TYPE)(WORD) pEvent->Target.m_id;	// if static tile.

	CLIMODE_TYPE prevmode = GetTargMode();
	ClearTargMode();

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

void CClient::Event_AOSPopupMenu( DWORD uid, WORD EntryTag )
{
	UID uObj = uid;
	CExtData cmd;

#define MAX_POPUPS 15

#define POPUPFLAG_LOCKED 0x01
#define POPUPFLAG_ARROW 0x02
#define POPUPFLAG_COLOR 0x20

#define POPUP_REQUEST 100
#define POPUP_PAPERDOLL 101
#define POPUP_BACKPACK 102
#define POPUP_BANKBOX 201
#define POPUP_BANKBALANCE 202
#define POPUP_VENDORBUY 301
#define POPUP_VENDORSELL 302
#define POPUP_PETGUARD 401
#define POPUP_PETFOLLOW 402
#define POPUP_PETDROP 403
#define POPUP_PETKILL 404
#define POPUP_PETSTOP 405
#define POPUP_PETSTAY 406
#define POPUP_PETFRIEND 407
#define POPUP_PETTRANSFER 408
#define POPUP_STABLESTABLE 501
#define POPUP_STABLERETRIEVE 502

	struct
	{
		WORD m_EntryTag;
		WORD m_TextID; // This comes from cliloc, the 3006xxx range &~ 3000000
		WORD m_Flags;
		WORD m_Color;
	} Popups[MAX_POPUPS];

	unsigned int x = 0;
	unsigned int y = 0;
	unsigned int len = 7; // Size of the header

	if ( uObj.IsItem() )
		CItem *pItem = uObj.ItemFind();
	else if ( uObj.IsChar() )
		CChar *pChar = uObj.CharFind();
	else
		return;

	if ( !CanSee( uObj.ObjFind() ) )
		return;

	if ( m_pChar && !(m_pChar->CanSeeLOS( uObj.ObjFind() )) )
		return;

	if ( ! EntryTag )
	{
		cmd.Popup_Display.m_unk1 = 1;
		cmd.Popup_Display.m_UID = uid;
	}

	if ( uObj.IsChar() )
	{
		CChar * pChar = uObj.CharFind();

		if ( pChar->IsHuman() )
		{
			Popups[x].m_EntryTag = POPUP_PAPERDOLL;
			Popups[x].m_TextID = 6123; // Open Paperdoll 3006123
			Popups[x].m_Color = 0xFFFF;
			Popups[x++].m_Flags = POPUPFLAG_COLOR;
		}

		if ( pChar == m_pChar )
		{
			Popups[x].m_EntryTag = POPUP_BACKPACK;
			Popups[x].m_TextID = 6145; // Open Backpack 3006145
			Popups[x].m_Color = 0xFFFF;
			Popups[x++].m_Flags = POPUPFLAG_COLOR;
		}

		if ( pChar->m_pNPC )
		{
			switch ( pChar->m_pNPC->m_Brain )
			{
				case NPCBRAIN_BANKER:
					Popups[x].m_EntryTag = POPUP_BANKBOX;
					Popups[x].m_TextID = 6105; // Open Bankbox 3006105
					Popups[x].m_Color = 0xFFFF;
					Popups[x++].m_Flags = POPUPFLAG_COLOR;

					Popups[x].m_EntryTag = POPUP_BANKBALANCE;
					Popups[x].m_TextID = 6124; // Check Balance	3006124
					Popups[x].m_Color = 0xFFFF;
					Popups[x++].m_Flags = POPUPFLAG_COLOR;
					break;

				case NPCBRAIN_STABLE:
					Popups[x].m_EntryTag = POPUP_STABLESTABLE;
					Popups[x].m_TextID = 6126; // Stable Pet 3006126
					Popups[x].m_Color = 0xFFFF;
					Popups[x++].m_Flags = POPUPFLAG_COLOR;

					Popups[x].m_EntryTag = POPUP_STABLERETRIEVE;
					Popups[x].m_TextID = 6127; // Claim All Pets 3006127
					Popups[x].m_Color = 0xFFFF;
					Popups[x++].m_Flags = POPUPFLAG_COLOR;

				case NPCBRAIN_VENDOR:
				case NPCBRAIN_HEALER:
					Popups[x].m_EntryTag = POPUP_VENDORBUY;
					Popups[x].m_TextID = 6103; // Buy 3006103
					Popups[x].m_Color = 0xFFFF;
					Popups[x++].m_Flags = POPUPFLAG_COLOR;

					Popups[x].m_EntryTag = POPUP_VENDORSELL;
					Popups[x].m_TextID = 6104; // Sell 3006104
					Popups[x].m_Color = 0xFFFF;
					Popups[x++].m_Flags = POPUPFLAG_COLOR;
					break;
			}

			if ( pChar->NPC_IsOwnedBy( m_pChar, false ) )
			{
				Popups[x].m_EntryTag = POPUP_PETGUARD;
				Popups[x].m_TextID = 6107; // Command: Guard 3006107
				Popups[x].m_Color = 0xFFFF;
				Popups[x++].m_Flags = POPUPFLAG_COLOR;

				Popups[x].m_EntryTag = POPUP_PETFOLLOW;
				Popups[x].m_TextID = 6108; // Command: Follow 3006108
				Popups[x].m_Color = 0xFFFF;
				Popups[x++].m_Flags = POPUPFLAG_COLOR;

				Popups[x].m_EntryTag = POPUP_PETDROP;
				Popups[x].m_TextID = 6109; // Command: Drop 3006109
				Popups[x].m_Color = 0xFFFF;
				Popups[x++].m_Flags = POPUPFLAG_COLOR;

				Popups[x].m_EntryTag = POPUP_PETKILL;
				Popups[x].m_TextID = 6111; // Command: Kill 3006111
				Popups[x].m_Color = 0xFFFF;
				Popups[x++].m_Flags = POPUPFLAG_COLOR;

				Popups[x].m_EntryTag = POPUP_PETSTOP;
				Popups[x].m_TextID = 6112; // Command: Stop 3006112
				Popups[x].m_Color = 0xFFFF;
				Popups[x++].m_Flags = POPUPFLAG_COLOR;

				Popups[x].m_EntryTag = POPUP_PETSTAY;
				Popups[x].m_TextID = 6114; // Command: Stay 3006114
				Popups[x].m_Color = 0xFFFF;
				Popups[x++].m_Flags = POPUPFLAG_COLOR;

				Popups[x].m_EntryTag = POPUP_PETFRIEND;
				Popups[x].m_TextID = 6110; // Add Friend 3006110
				Popups[x].m_Color = 0xFFFF;
				Popups[x++].m_Flags = POPUPFLAG_COLOR;

				Popups[x].m_EntryTag = POPUP_PETTRANSFER;
				Popups[x].m_TextID = 6113; // Transfer 3006113
				Popups[x].m_Color = 0xFFFF;
				Popups[x++].m_Flags = POPUPFLAG_COLOR;

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

	if ( ( ! EntryTag ) && ( x ) )
	{
		CExtData * pCur = &cmd;

		cmd.Popup_Display.m_NumPopups = x;
		for ( y = 0; Popups[y].m_EntryTag; y++ )
		{
			// Construct the packet

			pCur->Popup_Display.m_List[0].m_EntryTag = Popups[y].m_EntryTag;
			pCur->Popup_Display.m_List[0].m_TextID = Popups[y].m_TextID;
			pCur->Popup_Display.m_List[0].m_Flags = Popups[y].m_Flags;
			pCur->Popup_Display.m_List[0].m_Color = Popups[y].m_Color;

			len += sizeof( pCur->Popup_Display.m_List[0] );
			pCur = (CExtData *)( ((BYTE*) pCur ) + sizeof( pCur->Popup_Display.m_List[0] ));
		}

		addExtData(EXTDATA_Popup_Display, &cmd, len);
	}
}


void CClient::Event_AOSItemInfo( int count , const NDWORD * uidList )
{
	if ( !m_pChar )
		return;

	if ( !IsAosFlagEnabled( FEATURE_AOS_UPDATE_B ) ) return;

	CObjBase * obj;
	for ( int i = 0; i < count; i++, obj = NULL )
	{
		UID uid = (DWORD) uidList[i];
		obj = uid.ObjFind();

		if ( !obj )
			return;

		if ( m_pChar->CanSee(obj) )
		{
			this->addAOSTooltip(obj, true);
		}
	}
}

//----------------------------------------------------------------------

void CClient::Event_ExtAosData( EXTAOS_TYPE type, const CExtAosData * pData, DWORD m_uid, int len )
{
	// XCMD_ExtData = 5 bytes of overhead before this.
	switch ( type )
	{
		case EXTAOS_HcBackup:
			break;

		case EXTAOS_HcRestore:
			break;
			
		case EXTAOS_HcCommit:
			break;
			
		case EXTAOS_HcDestroyItem:
			break;
			
		case EXTAOS_HcPlaceItem:
			break;
			
		case EXTAOS_HcExit:
			break;
			
		case EXTAOS_HcPlaceStair:
			break;
			
		case EXTAOS_HcSynch:
			break;
		
		case EXTAOS_HcFloorDesign:
			break;

		case EXTAOS_HcFloorDelete:
			break;

		case EXTAOS_HcClear:
			break;
		
		case EXTAOS_HcSwitch:
			break;
			
		case EXTAOS_HcRevert:
			break;
			
		case EXTAOS_SpecialMove:
		{
			CScriptTriggerArgs args;
			args.m_iN1 = pData->SpecialMove.m_Ability;
			if ( IsTrigUsed(TRIGGER_USERSPECIALMOVE) )
			{
				if ( m_pChar->OnTrigger(CTRIG_UserSpecialMove, m_pChar, &args) == TRIGRET_RET_TRUE )
					break;
			}

			if ( !args.m_iN1 )
				CombatAbility::abils[0]->ClearCurrentAbility(m_pChar);
			else if ( args.m_iN1 < ABIL_QTY )
			{
				m_TagDefs.SetNum("SpecialCombatMove", args.m_iN1);
				m_TagDefs.SetNum("LastCombatAbility", g_World.GetCurrentTime().GetTimeRaw());
			}
		} break;

		case EXTAOS_EquipLastWeapon:
			break;
			
		case EXTAOS_GuildButton:
		{
			if ( IsTrigUsed(TRIGGER_USERGUILDBUTTON) )
			{
				m_pChar->OnTrigger(CTRIG_UserGuildButton, m_pChar, NULL);
			}
		} break;

		case EXTAOS_QuestButton:
		{
			if ( IsTrigUsed(TRIGGER_USERQUESTBUTTON) )
			{
				m_pChar->OnTrigger(CTRIG_UserQuestButton, m_pChar, NULL);
			}
		} break;
	
		default:
			SysMessagef( "Unknown AOS extended msg %d.", type );
			break;
	}
}

void CClient::Event_ExtData( EXTDATA_TYPE type, const CExtData * pData, int len )
{
	// XCMD_ExtData = 5 bytes of overhead before this.
	switch ( type )
	{
		case EXTDATA_ScreenSize:
			// Sent at start up for the party system ?
			{
				if ( !m_wScreenx )
					m_wScreenx = (DWORD)pData->ScreenSize.m_x;

				if ( !m_wScreeny )
					m_wScreeny = (DWORD)pData->ScreenSize.m_y;
			}
			break;
		case EXTDATA_Lang:
			// Tell what lang i use.
			GetAccount()->m_lang.Set( pData->Lang.m_code );
			break;
		case EXTDATA_Party_Msg: // = 6
			// Messages about the party we are in.
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
					m_pChar->UpdateAnimate(anim, false, 0, 7);
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
			if ( IsAosFlagEnabled( FEATURE_AOS_POPUP ) )
				Event_AOSPopupMenu( (DWORD) pData->Popup_Request.m_UID );
			break;
	
		case EXTDATA_Popup_Select:
			if ( IsAosFlagEnabled( FEATURE_AOS_POPUP ) )
				Event_AOSPopupMenu( (DWORD) pData->Popup_Select.m_UID, (WORD) pData->Popup_Select.m_EntryTag );
			break;
	
		case EXTDATA_NewSpellSelect:
			{
				WORD iSpell = pData->NewSpellSelect.m_SpellId;

				if ( IsSetMagicFlags( MAGICF_PRECAST ) )
				{
					m_tmSkillMagery.m_Spell = (SPELL_TYPE) iSpell;
					m_pChar->m_atMagery.m_Spell = (SPELL_TYPE) iSpell;
					m_Targ_UID = m_pChar->GetUID();
					m_Targ_PrvUID = m_pChar->GetUID();
					m_pChar->Skill_Start( SKILL_MAGERY );
				}
				else
					Cmd_Skill_Magery( (SPELL_TYPE)iSpell, m_pChar );
			}
			break;
			
		case EXTDATA_Unk24:
			break;

		case EXTDATA_BandageMacro:
			{
				UID bandageSerial = (DWORD)pData->BandageMacro.m_bandageSerial;
				UID targetSerial = (DWORD)pData->BandageMacro.m_targetSerial;

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
				if ( !m_pChar->CanUse( pBandage, false ) )
					return;

				// Check the bandage is in the possession of the client
				if ( pBandage->GetTopLevelObj() != m_pChar )
					return;

				// Make sure the macro isn't used for other types of items.
				if ( !pBandage->IsType( IT_BANDAGE ) )
					return;
				
				// Clear previous Target
				SetTargMode();

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
	// parse the args.
	TCHAR szTmp[ MAX_TALK_BUFFER ];
	strcpylen( szTmp, pszName, sizeof(szTmp));

	if ( IsTrigUsed(TRIGGER_USEREXTCMD) && m_pChar )
	{
		CScriptTriggerArgs	Args(szTmp);
		Args.m_iN1	= type;
		if ( m_pChar->OnTrigger(CTRIG_UserExtCmd, m_pChar, &Args) == TRIGRET_RET_TRUE )
			return;
		strcpy(szTmp, Args.m_s1);
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
				m_pChar->UpdateAnimate(32);
			else if ( ! strcmpi( ppArgs[0],"salute"))
				m_pChar->UpdateAnimate(33);
			else
			{
				DEBUG_ERR(("%x:Event Animate '%s'\n", socketId(), ppArgs[0]));
			}
			break;
	
		case EXTCMD_SKILL:			// Skill
			Event_Skill_Use( (SKILL_TYPE) ATOI( ppArgs[0] ));
			break;
	
		case EXTCMD_AUTOTARG:	// bizarre new autotarget mode.
			// "target x y z"
			{
				UID uid( ATOI( ppArgs[0] ));
				CObjBase * pObj = uid.ObjFind();
				if ( pObj )
				{
					DEBUG_ERR(("%x:Event Autotarget '%s' '%s'\n", socketId(), pObj->GetName(), ppArgs[1] ));
				}
				else
				{
					DEBUG_ERR(("%x:Event Autotarget UNK '%s' '%s'\n", socketId(), ppArgs[0], ppArgs[1] ));
				}
			}
			break;
	
		case EXTCMD_CAST_MACRO:	// macro spell.
		case EXTCMD_CAST_BOOK:	// cast spell from book.
			if ( IsSetMagicFlags( MAGICF_PRECAST ) )
			{
				m_tmSkillMagery.m_Spell = (SPELL_TYPE) ATOI( ppArgs[0] );
				m_pChar->m_atMagery.m_Spell = (SPELL_TYPE) ATOI( ppArgs[0] );	// m_atMagery.m_Spell
				m_Targ_UID = m_pChar->GetUID();	// default target.
				m_Targ_PrvUID = m_pChar->GetUID();
				m_pChar->Skill_Start( SKILL_MAGERY );
			}
			else
				Cmd_Skill_Magery( (SPELL_TYPE) ATOI( ppArgs[0] ), m_pChar );
			break;
	
		case EXTCMD_DOOR_AUTO: // open door macro = Attempt to open a door around us.
			if ( m_pChar && !m_pChar->IsStatFlag( STATF_DEAD ) )
			{
				CWorldSearch Area(m_pChar->GetTopPoint(), 4);
				while ( CItem *pItem = Area.GetItem() )
				{
					switch ( pItem->GetType() )
					{
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
	
		default:
			DEBUG_ERR(("%x:Event_ExtCmd unk %d, '%s'\n", socketId(), type, pszName));
	}
}

// ---------------------------------------------------------------------

#define xCheckMsgSize(_len_)	( _len_ && (_len_ <= sizeof(CEvent)) && ( packet->length() >= _len_ ) && ( packetLen = packetLen + _len_ ))
#define xRET					return packetLen

long CClient::xDispatchMsg(Packet *packet)
{
	long packetLen = 0;

	EXC_TRY("DispatchMsg");
	if ( !xCheckMsgSize(1) )	// just get the command
		xRET;

	const CEvent *pEvent = (const CEvent *)packet->data();

	if ( pEvent->Default.m_Cmd >= XCMD_QTY ) // bad packet type ?
	{
		DEBUG_ERR(( "Unimplemented command %d\n", pEvent->Default.m_Cmd ) );
#ifdef _DEBUG
		for ( int ilist = 0; ilist < (sizeof(pEvent->m_Raw)/sizeof(BYTE)); ilist++ )
		{
			if ( ilist % 30 )
				fprintf( stderr, "\n" );
			else
				fprintf( stderr, "0x%x ", pEvent->m_Raw[ilist] );
		}
#endif
		xRET;
	}

	// login server or a game server that has not yet logged in.
	if ( GetConnectType() != CONNECT_GAME || ! GetAccount() )
	{
		switch ( pEvent->Default.m_Cmd )
		{
			case XCMD_ServersReq: // First Login
				EXC_SET("not logged - server list");
				if ( !xCheckMsgSize(sizeof(pEvent->ServersReq)) ) xRET;
				Login_ServerList(pEvent->ServersReq.m_acctname, pEvent->ServersReq.m_acctpass);
				break;

			case XCMD_ServerSelect:// Server Select - relay me to the server.
				EXC_SET("not logged - login relay");
				if ( !xCheckMsgSize( sizeof( pEvent->ServerSelect )) ) xRET;
				Login_Relay(pEvent->ServerSelect.m_select);
				break;

			case XCMD_CharListReq: // Second Login to select char
				EXC_SET("not logged - char list req");
				if ( !xCheckMsgSize( sizeof(pEvent->CharListReq)) ) xRET;
				Setup_ListReq(pEvent->CharListReq.m_acctname, pEvent->CharListReq.m_acctpass, false);
				break;

			case XCMD_War:
				xCheckMsgSize(sizeof(pEvent->War));
				break;

			default:
				DEBUG_WARN(("%x:Packet (0x%x) received, but it should not be here.\n", socketId(), pEvent->Default.m_Cmd));
				xCheckMsgSize(sizeof(pEvent->Default));
		}
		xRET;
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
			if ( !xCheckMsgSize( sizeof( pEvent->Create ))) xRET;
			Setup_CreateDialog( pEvent );
			xRET;
		case XCMD_CharDelete: // Character Delete
			EXC_SET("delete char");
			if ( ! xCheckMsgSize( sizeof( pEvent->CharDelete )))  xRET;
			addDeleteErr( Setup_Delete( pEvent->CharDelete.m_slot ));
			xRET;
		case XCMD_CharPlay: // Character Select
			EXC_SET("select char");
			if ( !xCheckMsgSize(sizeof(pEvent->CharPlay)) )  xRET;
			if ( !Setup_Play(pEvent->CharPlay.m_slot) )
				addLoginErr(PacketLoginError::Invalid);
			xRET;
		case XCMD_ConfigFile: // ConfigFile NOT SUPPORTED
			EXC_SET("config file");
			if ( !xCheckMsgSize(3) || !xCheckMsgSize(pEvent->ConfigFile.m_len) ) xRET;
			xRET;
	}

	// must have a logged in char to use any other messages.
	if ( !m_pChar ) xRET;

	//////////////////////////////////////////////////////
	// We are now playing.

	//	we must ensure that the current sector is awake if person is there even not moving
	CSector *currentSector = m_pChar->GetTopPoint().GetSector();
	if ( !currentSector ) xRET;
	currentSector->wakeUp();

	switch ( pEvent->Default.m_Cmd )
	{
		case XCMD_Talk: // Speech or at least text was typed.
			EXC_SET("talk");
			if ( !xCheckMsgSize(3) || !xCheckMsgSize(pEvent->Talk.m_len) )
				xRET;
			Event_Talk( pEvent->Talk.m_text, pEvent->Talk.m_wHue, (TALKMODE_TYPE)( pEvent->Talk.m_mode ));
			break;
		case XCMD_Attack: // Attack
			EXC_SET("attack");
			if ( ! xCheckMsgSize( sizeof( pEvent->Click )))
				xRET;
			Event_Attack( (DWORD) pEvent->Click.m_UID );
			break;
		case XCMD_DClick:// Doubleclick
			EXC_SET("dclick");
			if ( ! xCheckMsgSize( sizeof( pEvent->Click )))
				xRET;
			Event_DoubleClick( ((DWORD)(pEvent->Click.m_UID)) &~ UID_F_RESOURCE, ((DWORD)(pEvent->Click.m_UID)) & UID_F_RESOURCE, true );
			break;
		case XCMD_ItemPickupReq: // Pick up Item
			EXC_SET("pickup item");
			if ( ! xCheckMsgSize( sizeof( pEvent->ItemPickupReq )))
				xRET;
			Event_Item_Pickup( (DWORD) pEvent->ItemPickupReq.m_UID, pEvent->ItemPickupReq.m_amount );
			break;
		case XCMD_ItemDropReq: // Drop Item
			EXC_SET("drop item");
			if ( ! xCheckMsgSize( sizeof( pEvent->ItemDropReq )))
				xRET;
			Event_Item_Drop(pEvent);
			break;
		case XCMD_Click: // Singleclick
			EXC_SET("singleclick");
			if ( ! xCheckMsgSize( sizeof( pEvent->Click )))
				xRET;
			Event_SingleClick( (DWORD) pEvent->Click.m_UID );
			break;
		case XCMD_ExtCmd: // Ext. Command
			EXC_SET("ext cmd");
			if ( ! xCheckMsgSize(3))
				xRET;
			if ( ! xCheckMsgSize( pEvent->ExtCmd.m_len ))
				xRET;
			Event_ExtCmd( (EXTCMD_TYPE) pEvent->ExtCmd.m_type, pEvent->ExtCmd.m_name );
			break;
		case XCMD_Skill:	// Skill locking.
			EXC_SET("skill lock");
			if ( ! xCheckMsgSize(3))
				xRET;
			if ( ! xCheckMsgSize( pEvent->Skill.m_len ))
				xRET;
			Event_Skill_Locks(pEvent);
			break;
		case XCMD_MapEdit:	// plot course on map.
			EXC_SET("plot map course");
			if ( ! xCheckMsgSize( sizeof( pEvent->MapEdit )))
				xRET;
			Event_MapEdit( (DWORD) pEvent->MapEdit.m_UID, pEvent );
			break;
		case XCMD_BookPage: // Read/Change Book
			EXC_SET("read/edit book");
			if ( ! xCheckMsgSize(3))
				xRET;
			if ( ! xCheckMsgSize( pEvent->BookPage.m_len ))
				xRET;
			Event_Book_Page( (DWORD) pEvent->BookPage.m_UID, pEvent );
			break;
		case XCMD_Target: // Targeting
			EXC_SET("target");
			if ( ! xCheckMsgSize( sizeof( pEvent->Target )))
				xRET;
			Event_Target( pEvent );
			break;
		case XCMD_SecureTrade: // Secure trading
			EXC_SET("secure trade");
			if ( ! xCheckMsgSize(3))
				xRET;
			if ( ! xCheckMsgSize( pEvent->SecureTrade.m_len ))
				xRET;
			Event_SecureTrade( (DWORD) pEvent->SecureTrade.m_UID, pEvent );
			break;
		case XCMD_BBoard: // BBoard Request.
			EXC_SET("bboard");
			if ( ! xCheckMsgSize(3))
				xRET;
			if ( ! xCheckMsgSize( pEvent->BBoard.m_len ))
				xRET;
			Event_BBoardRequest( (DWORD) pEvent->BBoard.m_UID, pEvent );
			break;
		case XCMD_War: // Combat Mode
			EXC_SET("combat mode");
			if ( ! xCheckMsgSize( sizeof( pEvent->War )))
				xRET;
			Event_CombatMode( pEvent->War.m_warmode );
			break;
		case XCMD_CharName: // Rename Character(pet)
			{
				EXC_SET("rename pet");
				if ( ! xCheckMsgSize( sizeof( pEvent->CharName )))
					xRET;
				TEMPSTRING(zCharName);
				strcpylen(zCharName, pEvent->CharName.m_charname, MAX_NAME_SIZE);
				Event_SetName((DWORD)pEvent->CharName.m_UID, zCharName);
			}
			break;
		case XCMD_MenuChoice: // Menu Choice
				EXC_SET("menu choice");
			if ( ! xCheckMsgSize( sizeof( pEvent->MenuChoice )))
				xRET;
			Event_MenuChoice(pEvent);
			break;
		case XCMD_BookOpen:	// Change a books title/author.
			EXC_SET("open book");
			if ( ! xCheckMsgSize( sizeof(pEvent->BookOpen)))
				xRET;
			Event_Book_Title((DWORD)pEvent->BookOpen.m_UID, pEvent->BookOpen.m_title, pEvent->BookOpen.m_author);
			break;
		case XCMD_DyeVat: // Color Select Dialog
			EXC_SET("color dialog");
			if ( ! xCheckMsgSize( sizeof( pEvent->DyeVat )))
				xRET;
			Event_Item_Dye( (DWORD) pEvent->DyeVat.m_UID, pEvent->DyeVat.m_wHue );
			break;
		case XCMD_Prompt: // Response to console prompt.
			EXC_SET("console resp");
			if ( ! xCheckMsgSize(3))
				xRET;
			if ( ! xCheckMsgSize( pEvent->Prompt.m_len ))
				xRET;
			Event_PromptResp( pEvent->Prompt.m_text, pEvent->Prompt.m_len-sizeof(pEvent->Prompt));
			break;
		case XCMD_HelpPage: // GM Page (i want to page a gm!)
			{
				EXC_SET("help page");
				if ( xCheckMsgSize(sizeof(pEvent->HelpPage)) && m_pChar )
				{
					CScript script("HelpPage");
					m_pChar->r_Verb(script, this);
				}
				break;
			}
		case XCMD_GumpInpValRet:	// Gump text input
			EXC_SET("gumptext input");
			if ( ! xCheckMsgSize(3))
				xRET;
			if ( ! xCheckMsgSize( pEvent->GumpInpValRet.m_len ))
				xRET;
			Event_GumpInpValRet(pEvent);
			break;
		case XCMD_TalkUNICODE:	// Talk unicode.
			EXC_SET("unicode talk");
			if ( ! xCheckMsgSize(3))
				xRET;
			if ( ! xCheckMsgSize(pEvent->TalkUNICODE.m_len ))
				xRET;
			Event_TalkUNICODE(pEvent);
			break;
		case XCMD_GumpDialogRet:	// Gump menu.
			EXC_SET("dialog ret");
			if ( ! xCheckMsgSize(3))
				xRET;
			if ( ! xCheckMsgSize( pEvent->GumpDialogRet.m_len ))
				xRET;
			Event_GumpDialogRet(pEvent);
			break;
		case XCMD_ChatText:	// ChatText
			EXC_SET("chat text");
			if ( ! xCheckMsgSize(3))
				xRET;
			if ( ! xCheckMsgSize( pEvent->ChatText.m_len ))
				xRET;
			Event_ChatText( pEvent->ChatText.m_utext, pEvent->ChatText.m_len, CLanguageID( pEvent->ChatText.m_lang ));
			break;
		case XCMD_Chat: // Chat
			EXC_SET("chat");
			if ( ! xCheckMsgSize( sizeof( pEvent->Chat)))
				xRET;
			Event_ChatButton(pEvent->Chat.m_uname);
			break;
		case XCMD_CharProfile:	// Get Character Profile.
			EXC_SET("profile");
			if ( ! xCheckMsgSize(3))
				xRET;
			if ( ! xCheckMsgSize( pEvent->CharProfile.m_len ))
				xRET;
			Event_Profile( pEvent->CharProfile.m_WriteMode, (DWORD) pEvent->CharProfile.m_UID, pEvent );
			break;
		case XCMD_MailMsg:
			EXC_SET("mailmsg");
			if ( ! xCheckMsgSize( sizeof(pEvent->MailMsg)))
				xRET;
			Event_MailMsg( (DWORD) pEvent->MailMsg.m_uid1, (DWORD) pEvent->MailMsg.m_uid2 );
			break;
		case XCMD_ClientVersion:	// Client Version string packet
			EXC_SET("client version");
			if ( ! xCheckMsgSize(3))
				xRET;
			if ( ! xCheckMsgSize(pEvent->ClientVersion.m_len))
				xRET;
			Event_ClientVersion( pEvent->ClientVersion.m_text, pEvent->ClientVersion.m_len );
			break;
		case XCMD_ViewRange:
			EXC_SET("view range");
			if ( !xCheckMsgSize(2) )
				xRET;
			if ( !xCheckMsgSize(sizeof(pEvent->ViewRange)) )
				xRET;
			// void
			xRET;
		case XCMD_ExtData:	// Add someone to the party system.
			EXC_SET("ext data");
			if ( ! xCheckMsgSize(3))
				xRET;
			if ( ! xCheckMsgSize( pEvent->ExtData.m_len ))
				xRET;
			Event_ExtData( (EXTDATA_TYPE)(WORD) pEvent->ExtData.m_type, &(pEvent->ExtData.m_u), pEvent->ExtData.m_len-5 );
			break;
		case XCMD_ExtAosData:	// Add someone to the party system.
			EXC_SET("ext aos");
			if ( ! xCheckMsgSize(3))
				xRET;
			if ( ! xCheckMsgSize( pEvent->ExtAosData.m_len ))
				xRET;
			Event_ExtAosData( (EXTAOS_TYPE)(WORD) pEvent->ExtAosData.m_type, &(pEvent->ExtAosData.m_u), (DWORD) pEvent->ExtAosData.m_uid, pEvent->ExtAosData.m_len-9 );
			break;

		case XCMD_AOSTooltip:
			{
				EXC_SET("tooltip request");
				if ( ! xCheckMsgSize(3))
					xRET;
				if ( ! xCheckMsgSize( pEvent->AosItemInfoRequest.m_len ))
					xRET;
				WORD wLen = ((WORD) pEvent->AosItemInfoRequest.m_len) - 3;
				if ( wLen < 0 || (wLen%4) != 0 )
					xRET;
				Event_AOSItemInfo( (int)(wLen/4), (NDWORD *) &pEvent->AosItemInfoRequest.m_uid );
			} break;			

		case XCMD_AllNames3D:
			{
				if ( xCheckMsgSize(3) && xCheckMsgSize(pEvent->AllNames3D.m_len) )
				{
					long len = pEvent->AllNames3D.m_len / sizeof(DWORD);
					for ( long l = 0; l < len; l++ )
					{
						UID uid = *(&pEvent->AllNames3D.m_UID + (l * sizeof(DWORD)));
						if ( m_pChar )
						{
							CObjBase *pObj = uid.ObjFind();
							if ( pObj && m_pChar->CanSee(pObj) )
							{
								CCommand cmd;
								cmd.AllNames3D.m_Cmd = XCMD_AllNames3D;
								cmd.AllNames3D.m_len = 37;
								cmd.AllNames3D.m_UID = pObj->uid();
								strcpylen(cmd.AllNames3D.m_name, pObj->GetName(), sizeof(cmd.AllNames3D.m_name));
								xSend(&cmd, cmd.AllNames3D.m_len);
							}
						}
					}
				}
			} break;

		default:
			EXC_SET("unknown");
			DEBUG_WARN(("%x:Unknown game packet (0x%x) received.\n", socketId(), pEvent->Default.m_Cmd));
			xRET;
	}

	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.Debug("account '%s'\n", GetAccount() ? GetAccount()->GetName() : "");
	EXC_DEBUG_END;
	xRET;
}
