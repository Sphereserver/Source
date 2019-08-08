#include "graysvr.h"	// predef header.
#include "../network/send.h"
#include "../network/receive.h"

/////////////////////////////////
// Events from the Client.

LPCTSTR const CClient::sm_szCmd_Redirect[] =
{
	"BANK",
	"CONTROL",
	"DESTROY",
	"DUPE",
	"FORGIVE",
	"JAIL",
	"KICK",
	"KILL",
	"NUDGEDOWN",
	"NUDGEUP",
	"PRIVSET",
	"REMOVE",
	"SHRINK"
};

// Client pressed the chat button
void CClient::Event_ChatButton(const NCHAR *pszName)
{
	ADDTOCALLSTACK("CClient::Event_ChatButton");

	if ( IsTrigUsed(TRIGGER_USERCHATBUTTON) )
	{
		if ( m_pChar && m_pChar->OnTrigger(CTRIG_UserChatButton, m_pChar) == TRIGRET_RET_TRUE )
			return;
	}

	if ( !m_UseNewChatSystem && m_pAccount->m_sChatName.IsEmpty() && (g_Cfg.m_iChatFlags & CHATF_CUSTOMNAMES) )
	{
		// Client doesn't had sent any chat name to server yet, so ask for a permanent chat name (account based)
		if ( (pszName == NULL) || (pszName[0] == 0) )
		{
			addChatSystemMessage(CHATCMD_SetChatName);
			return;
		}

		// Make the chat name non-unicode and store it on account
		TCHAR szChatName[MAX_NAME_SIZE * 2 + 2];
		CvtNUNICODEToSystem(szChatName, sizeof(szChatName), pszName, 128);

		if ( !CChat::IsValidName(szChatName, true) || !g_Accounts.Account_ChatNameAvailable(szChatName) )
		{
			addChatSystemMessage(CHATCMD_SetChatName);
			return;
		}
		m_pAccount->m_sChatName = szChatName;
	}

	addChatWindow();
}

// Client sent a text on chat
void CClient::Event_ChatText(const NCHAR *pszText, int iTextLen, CLanguageID lang)
{
	ADDTOCALLSTACK("CClient::Event_ChatText");
	g_Serv.m_Chats.Action(this, pszText, iTextLen, lang);
}

// Client selected an item hue
void CClient::Event_Item_Dye(CGrayUID uid, HUE_TYPE wHue)
{
	ADDTOCALLSTACK("CClient::Event_Item_Dye");
	if ( !m_pChar || (GetTargMode() != CLIMODE_DYE) )
		return;

	ClearTargMode();

	CObjBase *pObj = uid.ObjFind();
	if ( !m_pChar->CanTouch(pObj) )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_DYE_REACH);
		return;
	}

	if ( !IsPriv(PRIV_GM) )
	{
		CItem *pItem = dynamic_cast<CItem *>(pObj);
		if ( !pItem || ((pObj->GetBaseID() != ITEMID_DYE_TUB) && (!pItem->IsType(IT_DYE_VAT) || !IsSetOF(OF_DyeType))) )
			return;
	}

	if ( wHue < HUE_BLUE_LOW )
		wHue = HUE_BLUE_LOW;
	if ( wHue > HUE_DYE_HIGH )
		wHue = HUE_DYE_HIGH;

	if ( pObj->IsChar() )
		wHue |= HUE_MASK_UNDERWEAR;

	pObj->SetHue(wHue, false, GetChar(), pObj);
	pObj->Update();
}

// Tip of the day window
void CClient::Event_Tips(DWORD dwIndex)
{
	ADDTOCALLSTACK("CClient::Event_Tips");
	if ( dwIndex == 0 )
		dwIndex = 1;

	CResourceLock s;
	if ( !g_Cfg.ResourceLock(s, RESOURCE_ID(RES_TIP, dwIndex)) )
	{
		// Requested tip was not found, default to tip 1 if possible
		if ( (dwIndex == 1) || !g_Cfg.ResourceLock(s, RESOURCE_ID(RES_TIP, 1)) )
			return;
		dwIndex = 1;
	}

	addScrollScript(s, SCROLL_Tips, dwIndex + 1);
}

// Client changed book title/author info
void CClient::Event_Book_Title(CItem *pItem, LPCTSTR pszTitle, LPCTSTR pszAuthor)
{
	ADDTOCALLSTACK("CClient::Event_Book_Title");
	if ( !m_pChar )
		return;

	CItemMessage *pBook = dynamic_cast<CItemMessage *>(pItem);
	if ( !m_pChar->CanTouch(pBook) )
	{
		SysMessageDefault(DEFMSG_REACH_FAIL);
		return;
	}
	if ( !pBook->IsBookWritable() )
		return;
	if ( Str_Check(pszTitle) || Str_Check(pszAuthor) )
		return;

	pBook->m_sAuthor = pszAuthor;
	pBook->SetName(pszTitle);
}

// Client picked up an item
void CClient::Event_Item_Pickup(CGrayUID uid, int iAmount)
{
	ADDTOCALLSTACK("CClient::Event_Item_Pickup");
	EXC_TRY("CClient::Event_Item_Pickup");
	if ( !m_pChar )
		return;

	EXC_SET("Item");
	CItem *pItem = uid.ItemFind();
	if ( !pItem || pItem->IsWeird() )
	{
		EXC_SET("Item - addObjectRemove(uid)");
		addObjectRemove(uid);
		EXC_SET("Item - addItemDragCancel(0)");
		new PacketDragCancel(this, PacketDragCancel::CannotLift);
		return;
	}

	// FastLoot prevention (,emptycontainer)
	EXC_SET("FastLoot");
	CServTime CurTime = g_World.GetCurrentTime();
	if ( CurTime < m_timeLastEventItemPickup + 5 )
	{
		EXC_SET("FastLoot - addItemDragCancel(0)");
		new PacketDragCancel(this, PacketDragCancel::CannotLift);
		return;
	}
	m_timeLastEventItemPickup = CurTime;

	// Where is the item coming from? (just in case we have to toss it back)
	EXC_SET("Origin");
	CObjBase *pObjParent = dynamic_cast<CObjBase *>(pItem->GetParent());
	m_Targ_PrvUID = pObjParent ? pObjParent->GetUID() : static_cast<CGrayUID>(UID_CLEAR);
	m_Targ_p = pItem->GetTopPoint();

	EXC_SET("ItemPickup");
	iAmount = m_pChar->ItemPickup(pItem, static_cast<WORD>(iAmount));
	if ( iAmount < 0 )
	{
		EXC_SET("ItemPickup - addItemDragCancel(0)");
		new PacketDragCancel(this, PacketDragCancel::CannotLift);
		return;
	}

	SOUND_TYPE iSnd = static_cast<SOUND_TYPE>(pItem->GetDefNum("PICKUPSOUND", true));
	addSound(iSnd ? iSnd : SOUND_USE_CLOTH);

	EXC_SET("TargMode");
	SetTargMode(CLIMODE_DRAG);
	m_Targ_UID = uid;
	EXC_CATCH;
}

// Try bounce the item on LAYER_DRAGGING back to where it came from
void CClient::Event_Item_Drop_Fail(CItem *pItem)
{
	ADDTOCALLSTACK("CClient::Event_Item_Drop_Fail");
	if ( !pItem || (pItem != m_pChar->LayerFind(LAYER_DRAGGING)) )
		return;

	CItemContainer *pPrevCont = static_cast<CItemContainer *>(m_Targ_PrvUID.ItemFind());
	if ( pPrevCont )
	{
		pPrevCont->ContentAdd(pItem, m_Targ_p);
		return;
	}

	CChar *pPrevChar = m_Targ_PrvUID.CharFind();
	if ( pPrevChar )
	{
		pPrevChar->ItemEquip(pItem);
		return;
	}

	if ( m_Targ_PrvUID )	// if there's a previous cont UID set but it's not a valid container/char, probably this container/char got removed. So drop the item at player foot
		m_Targ_p = m_pChar->GetTopPoint();

	pItem->MoveToCheck(m_Targ_p);
}

// Client dropped an item somewhere
void CClient::Event_Item_Drop(CItem *pItem, CPointMap pt, CGrayUID uidOn, BYTE gridIndex)
{
	ADDTOCALLSTACK("CClient::Event_Item_Drop");
	if ( !m_pChar || (GetTargMode() != CLIMODE_DRAG) )
		return;

	ClearTargMode();

	CObjBase *pObjOn = uidOn.ObjFind();
	CItemContainer *pContOn = NULL;

	// Are we out of sync?
	if ( !pItem || (pItem == pObjOn) || (pItem != m_pChar->LayerFind(LAYER_DRAGGING)) )
	{
		new PacketDragCancel(this, PacketDragCancel::Other);
		return;
	}

	if ( pItem->IsAttr(ATTR_NODROPTRADE) )
	{
		// These items can be dropped only on player backpack or trash can
		CItem *pPack = dynamic_cast<CItem *>(pObjOn);
		if ( pPack && pPack->IsType(IT_TRASH_CAN) )
		{
			addSound(pItem->GetDropSound(pObjOn));
			pItem->Delete();
			return;
		}
		else if ( (pPack != m_pChar->LayerFind(LAYER_PACK)) && !IsPriv(PRIV_GM) )
		{
			SysMessageDefault(DEFMSG_ITEM_CANTDROPTRADE);
			return Event_Item_Drop_Fail(pItem);
		}
	}

	if ( pObjOn )
	{
		// Dropped over an object
		if ( !m_pChar->CanTouch(pObjOn) )
			return Event_Item_Drop_Fail(pItem);

		if ( pObjOn->IsChar() )
		{
			// Dropped directly over an char
			CChar *pCharOn = static_cast<CChar *>(pObjOn);
			if ( pCharOn != m_pChar )
			{
				if ( Cmd_SecureTrade(pCharOn, pItem) )
					return;
				if ( !IsPriv(PRIV_GM) )
					return Event_Item_Drop_Fail(pItem);
			}
			pObjOn = pCharOn->GetContainerCreate(LAYER_PACK);
		}

		pContOn = dynamic_cast<CItemContainer *>(pObjOn);
		if ( !pContOn )
			pContOn = dynamic_cast<CItemContainer *>(pObjOn->GetParent());

		CChar *pCharTop = dynamic_cast<CChar *>(pObjOn->GetTopLevelObj());
		if ( pCharTop && pContOn )
		{
			// Dropped on container inside an char
			if ( !pCharTop->NPC_IsOwnedBy(m_pChar) )
				return Event_Item_Drop_Fail(pItem);

			if ( pCharTop->m_pNPC )
			{
				pItem->ClrAttr(ATTR_OWNED);
				if ( !g_Cfg.m_bAllowNewbTransfer )
					pItem->ClrAttr(ATTR_NEWBIE|ATTR_BLESSED|ATTR_INSURED);
			}

			CItemContainer *pBankBox = pCharTop->GetContainerCreate(LAYER_BANKBOX);
			if ( pBankBox->IsItemInside(pContOn) )
			{
				// Dropped on bank box
				// Only accept items if the client already had opened the bank box
				if ( pBankBox->m_itEqBankBox.m_ptOpen != pCharTop->GetTopPoint() )
					return Event_Item_Drop_Fail(pItem);

				// Convert physical gold into virtual gold
				if ( pItem->IsType(IT_GOLD) && (g_Cfg.m_iFeatureTOL & FEATURE_TOL_VIRTUALGOLD) )
				{
					pCharTop->m_virtualGold += pItem->GetAmount();
					SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_BVBOX_DEPOSITED), pItem->GetAmount());
					addSound(pItem->GetDropSound(pObjOn));
					pItem->Delete();
					return;
				}

				// Check if bank box have enough item/weight limit to hold this item
				if ( !pBankBox->CanContainerHold(pItem, m_pChar) )
					return Event_Item_Drop_Fail(pItem);
			}
			else if ( !pCharTop->CanCarry(pItem) )
			{
				SysMessageDefault(DEFMSG_MSG_HEAVY);
				return Event_Item_Drop_Fail(pItem);
			}
		}

		if ( pContOn )
		{
			// Dropped on container
			if ( !pContOn->CanContainerHold(pItem, m_pChar) )
				return Event_Item_Drop_Fail(pItem);

			// Dropped on vendor box
			if ( pContOn->IsType(IT_EQ_VENDOR_BOX) )
			{
				if ( !pItem->Item_GetDef()->GetMakeValue(0) )
				{
					SysMessageDefault(DEFMSG_MSG_ERR_NOT4SALE);
					return Event_Item_Drop_Fail(pItem);
				}

				// Bug with shifting selling list by gold coins
				if ( pItem->IsType(IT_GOLD) || pItem->IsType(IT_COIN) )
					return Event_Item_Drop_Fail(pItem);
			}

			// Dropped on game board (chess, checker, backgammon, etc)
			// Only allow IT_GAME_PIECE items or the client will crash
			if ( pContOn->IsType(IT_GAME_BOARD) && !pItem->IsType(IT_GAME_PIECE) )
				return Event_Item_Drop_Fail(pItem);
		}

		if ( IsTrigUsed(TRIGGER_DROPON_ITEM) || IsTrigUsed(TRIGGER_ITEMDROPON_ITEM) )
		{
			CObjBase *pOldObj = pItem->GetParentObj();

			CScriptTriggerArgs Args(pObjOn);
			if ( pItem->OnTrigger(ITRIG_DROPON_ITEM, m_pChar, &Args) == TRIGRET_RET_TRUE )
				return Event_Item_Drop_Fail(pItem);

			if ( pOldObj != pItem->GetParentObj() )		// the trigger moved the item to another location
				return;
		}

		CItem *pItemOn = dynamic_cast<CItem *>(pObjOn);
		if ( pItemOn )
		{
			if ( IsTrigUsed(TRIGGER_DROPON_SELF) || IsTrigUsed(TRIGGER_ITEMDROPON_SELF) )
			{
				CScriptTriggerArgs Args(pItem);
				if ( pItemOn->OnTrigger(ITRIG_DROPON_SELF, m_pChar, &Args) == TRIGRET_RET_TRUE )
					return Event_Item_Drop_Fail(pItem);
			}

			if ( pItemOn->IsTypeMulti() && (m_NetState->isClientVersion(MINCLIVER_HS) || m_NetState->isClientEnhanced()) )
			{
				pt.m_x += pItemOn->GetTopPoint().m_x;
				pt.m_y += pItemOn->GetTopPoint().m_y;
				pt.m_z = pItemOn->GetTopPoint().m_z + pItemOn->GetHeight();
			}
			else
			{
				// Dropped on top of an non-container item
				// Check if the item can be piled
				if ( !pItem->Stack(pItemOn) )
				{
					if ( pItemOn->IsTypeSpellbook() )
					{
						if ( pItemOn->AddSpellbookScroll(pItem) )
						{
							SysMessageDefault(DEFMSG_CANT_ADD_SPELLBOOK);
							return Event_Item_Drop_Fail(pItem);
						}

						addSound(SOUND_USE_CLOTH, pItemOn);
						if ( pItem )	// bounce remaining scrolls amount on backpack
							Event_Item_Drop_Fail(pItem);
						return;
					}
				}
			}
		}
	}
	else
	{
		// Dropped on ground
		if ( !m_pChar->CanTouch(pt) || g_World.IsItemTypeNear(pt, IT_WALL, 0, true, true) )
			return Event_Item_Drop_Fail(pItem);
	}

	// Game pieces can only be dropped on game boards
	if ( pItem->IsType(IT_GAME_PIECE) )
	{
		if ( !pObjOn || (m_Targ_PrvUID != pObjOn->GetUID()) )
		{
			CItemContainer *pGameBoard = static_cast<CItemContainer *>(m_Targ_PrvUID.ItemFind());
			if ( pGameBoard )
				pGameBoard->ContentAdd(pItem, m_Targ_p);
			else
				pItem->Delete();
			return;
		}
	}

	// Move the item and do the dragging anim for everyone else to see
	if ( pContOn )
	{
		// On container
		m_pChar->UpdateDrag(pItem, pObjOn);
		pContOn->ContentAdd(pItem, pt, gridIndex);
		addSound(pItem->GetDropSound(pObjOn));
	}
	else
	{
		// On ground
		m_pChar->UpdateDrag(pItem, NULL, &pt);
		if ( !m_pChar->ItemDrop(pItem, pt) )
			return Event_Item_Drop_Fail(pItem);
	}
}

// Client pressed on an skill button on skill list gump
void CClient::Event_Skill_Use(SKILL_TYPE skill)
{
	ADDTOCALLSTACK("CClient::Event_Skill_Use");
	// All the push button skills come through here.
	// Any "Last skill" macro comes here as well. (push button only)
	if ( !m_pChar || (skill == SKILL_NONE) || !g_Cfg.m_SkillIndexDefs.IsValidIndex(skill) )
		return;
	if ( !m_pChar->Skill_CanUse(skill) || m_pChar->Skill_Wait(skill) )
		return;

	if ( IsTrigUsed(TRIGGER_SKILLSELECT) )
	{
		if ( m_pChar->Skill_OnCharTrigger(skill, CTRIG_SkillSelect) == TRIGRET_RET_TRUE )
		{
			m_pChar->Skill_Fail(true);	// clean up current skill
			return;
		}
	}

	if ( IsTrigUsed(TRIGGER_SELECT) )
	{
		if ( m_pChar->Skill_OnTrigger(skill, SKTRIG_SELECT) == TRIGRET_RET_TRUE )
		{
			m_pChar->Skill_Fail(true);	// clean up current skill
			return;
		}
	}

	SetTargMode();
	m_Targ_UID.InitUID();	// this is a start point for targ more

	bool fCheckCrime = false;
	bool fDoTargeting = false;

	if ( g_Cfg.IsSkillFlag(skill, SKF_SCRIPTED) )
	{
		const CSkillDef *pSkillDef = g_Cfg.GetSkillDef(skill);
		if ( pSkillDef && !pSkillDef->m_sTargetPrompt.IsEmpty() )
		{
			m_tmSkillTarg.m_Skill = skill;
			addTarget(CLIMODE_TARG_SKILL, pSkillDef->m_sTargetPrompt.GetPtr(), false, fCheckCrime);
			return;
		}
		else
		{
			m_pChar->Skill_Start(skill);
		}
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
			fCheckCrime = true;
			fDoTargeting = true;
			break;

		case SKILL_STEALTH:
		case SKILL_HIDING:
		case SKILL_SPIRITSPEAK:
		case SKILL_PEACEMAKING:
		case SKILL_DETECTINGHIDDEN:
		case SKILL_MEDITATION:
		case SKILL_IMBUING:
			m_pChar->Skill_Start(skill);
			return;

		case SKILL_TRACKING:
			Cmd_Skill_Tracking();
			break;

		case SKILL_CARTOGRAPHY:
			// EMPTY. On OSI cartography is not used clicking on the skill button anymore.
			// This code is empty just for compatibility purposes if someone want customize
			// the softcoded skill to enable the button again using @Select trigger.
			break;

		case SKILL_INSCRIPTION:
			Cmd_Skill_Inscription();
			break;
	}

	if ( fDoTargeting )
	{
		const CSkillDef *pSkillDef = g_Cfg.GetSkillDef(skill);
		if ( !pSkillDef || pSkillDef->m_sTargetPrompt.IsEmpty() )
			return;

		m_tmSkillTarg.m_Skill = skill;
		addTarget(CLIMODE_TARG_SKILL, pSkillDef->m_sTargetPrompt.GetPtr(), false, fCheckCrime);
		return;
	}
}

bool CClient::Event_CheckWalkBuffer()
{
	ADDTOCALLSTACK("CClient::Event_CheckWalkBuffer");
	// Check if the client is trying to walk too fast.
	// Direction changes don't count.

	if ( !g_Cfg.m_iWalkBuffer || !m_pChar->m_pPlayer )
		return true;
	if ( (m_iWalkStepCount % 7) != 0 )	// only check when we have taken 8 steps
		return true;

	// Client only allows 4 steps of walk ahead.
	ULONGLONG uCurTime = GetTickCount64();
	int iTimeDiff = static_cast<int>((uCurTime - m_timeWalkStep) / 10);
	int iTimeMin = m_pChar->IsStatFlag(STATF_OnHorse|STATF_Hovering) ? 70 : 140; // minimum time to move 8 steps

	switch ( m_pChar->m_pPlayer->m_speedMode )
	{
		case SPEEDMODE_DEFAULT:
			break;
		case SPEEDMODE_FAST:
			iTimeMin = 70;
			break;
		case SPEEDMODE_SLOW:
			iTimeMin *= 2;
			break;
		case SPEEDMODE_HYBRID:
			iTimeMin = 140;
			break;
	}

	if ( iTimeDiff > iTimeMin )
	{
		int	iRegen = ((iTimeDiff - iTimeMin) * g_Cfg.m_iWalkRegen) / 150;
		if ( iRegen > g_Cfg.m_iWalkBuffer )
			iRegen = g_Cfg.m_iWalkBuffer;
		else if ( iRegen < -((g_Cfg.m_iWalkBuffer * g_Cfg.m_iWalkRegen) / 100) )
			iRegen = -((g_Cfg.m_iWalkBuffer * g_Cfg.m_iWalkRegen) / 100);
		iTimeDiff = iTimeMin + iRegen;
	}

	m_iWalkTimeAvg = m_iWalkTimeAvg + iTimeDiff - iTimeMin;
	if ( m_iWalkTimeAvg > g_Cfg.m_iWalkBuffer )
		m_iWalkTimeAvg = g_Cfg.m_iWalkBuffer;
	else if ( m_iWalkTimeAvg < -g_Cfg.m_iWalkBuffer )
		m_iWalkTimeAvg = -g_Cfg.m_iWalkBuffer;

	if ( (m_iWalkTimeAvg < 0) && (iTimeDiff >= 0) )	// TICK_PER_SEC
	{
		// Walking too fast
		if ( IsTrigUsed(TRIGGER_USEREXWALKLIMIT) )
		{
			if ( m_pChar->OnTrigger(CTRIG_UserExWalkLimit, m_pChar) != TRIGRET_RET_TRUE )
				return false;
		}
	}

	m_timeWalkStep = uCurTime;
	return true;
}

// Client sent a walk request to server
bool CClient::Event_Walk(BYTE rawdir, BYTE sequence)
{
	ADDTOCALLSTACK("CClient::Event_Walk");
	// The client is sending a walk request to server, so the server must check
	// if the movement is possible and reply with another allow/reject packet
	// Return:
	//  true    = walking was allowed
	//  false   = walking was rejected

	// The theory....
	// The client sometimes echos 1 or 2 zeros or invalid echos when you first start
	//	walking (the invalid non zeros happen when you log off and don't exit the
	//	client.exe all the way and then log back in, XXX doesn't clear the stack)
	if ( !m_pChar )
		return false;

	DIR_TYPE dir = static_cast<DIR_TYPE>(rawdir & 0x0F);
	if ( dir >= DIR_QTY )
	{
		new PacketMovementRej(this, sequence);
		return false;
	}

	CPointMap pt = m_pChar->GetTopPoint();
	CPointMap ptOld = pt;

	if ( dir == m_pChar->m_dirFace )
	{
		if ( IsSetEF(EF_FastWalkPrevention) )
		{
			// The fastest way to get system clock is using g_World.GetCurrentTime().GetTimeRaw() to
			// read the value already stored by Sphere main timer. But this value is only updated at
			// tenths of second precision, which won't work here because we need millisecs precision.
			// So to reach this precision we must get the system clock manually at each walk request.
			UINT64 iCurTime = CWorldClock::GetSystemClock();
			if ( iCurTime < m_timeNextEventWalk )		// fastwalk detected
			{
				new PacketMovementRej(this, sequence);
				return false;
			}

			UINT64 iDelay = 0;
			if ( m_pChar->IsStatFlag(STATF_OnHorse|STATF_Hovering) )
				iDelay = (rawdir & 0x80) ? 100 : 200;
			else
				iDelay = (rawdir & 0x80) ? 200 : 400;

			// Decrease it a bit (2x16ms ticks) to avoid false-positive results if GetSystemClock()
			// lacks accuracy for some reason, which often happens on CPUs with dynamic clock speed
			iDelay -= 32;

			m_timeNextEventWalk = iCurTime + iDelay;
		}
		else if ( !Event_CheckWalkBuffer() )
		{
			new PacketMovementRej(this, sequence);
			return false;
		}

		pt.Move(dir);

		// Check Z height. The client already knows this but doesn't tell us.
		if ( m_pChar->CanMoveWalkTo(pt, true, false, dir) == NULL )
		{
			new PacketMovementRej(this, sequence);
			return false;
		}

		if ( !m_pChar->MoveToChar(pt) )
		{
			new PacketMovementRej(this, sequence);
			return false;
		}

		// Check if I stepped on any item/teleport
		TRIGRET_TYPE tr = m_pChar->CheckLocation();
		if ( tr == TRIGRET_RET_FALSE )
		{
			if ( m_pChar->GetTopPoint() == pt )	// check if position still the same, because the movement can't be rejected if the char already got moved/teleported
			{
				m_pChar->SetUnkPoint(ptOld);	// we already moved, so move back to previous location
				new PacketMovementRej(this, sequence);
				return false;
			}
		}

		// Check if invisible chars will be revealed
		m_pChar->CheckRevealOnMove();

		// Set running flag if I'm running
		m_pChar->StatFlag_Mod(STATF_Fly, (rawdir & 0x80) ? true : false);

		if ( tr == TRIGRET_RET_TRUE )
		{
			new PacketMovementAck(this, sequence);
			m_pChar->UpdateMove(ptOld, this);	// Who now sees me ?
			addPlayerSee(ptOld);				// What new stuff do I now see ?

			if ( m_pChar->m_pParty && ((m_iWalkStepCount % 10) == 0) )	// Send map waypoint location to party members at each 10 steps taken (enhanced clients only)
				m_pChar->m_pParty->UpdateWaypointAll(m_pChar, MAPWAYPOINT_PartyMember);
		}

		m_timeLastEventWalk = CServTime::GetCurrentTime();
		++m_iWalkStepCount;					// Increase step count to use on walk buffer checks
	}
	else
	{
		// Just a change in dir
		new PacketMovementAck(this, sequence);
		m_pChar->m_dirFace = dir;
		m_pChar->UpdateMove(ptOld, this);	// Who now sees me ?
	}
	return true;
}

// Client selected an combat ability on book
void CClient::Event_CombatAbilitySelect(DWORD dwAbility)
{
	ADDTOCALLSTACK("CClient::Event_CombatAbilitySelect");
	if ( !m_pChar )
		return;

	if ( IsTrigUsed(TRIGGER_USERSPECIALMOVE) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = dwAbility;
		m_pChar->OnTrigger(CTRIG_UserSpecialMove, m_pChar, &Args);
	}
}

// Client selected an virtue on gump
void CClient::Event_VirtueSelect(DWORD dwVirtue, CChar *pCharTarg)
{
	ADDTOCALLSTACK("CClient::Event_VirtueSelect");
	if ( !m_pChar )
		return;

	if ( IsTrigUsed(TRIGGER_USERVIRTUE) )
	{
		CScriptTriggerArgs Args(pCharTarg);
		Args.m_iN1 = dwVirtue;
		m_pChar->OnTrigger(CTRIG_UserVirtue, m_pChar, &Args);
	}
}

// Client changed peace/war mode
void CClient::Event_CombatMode(bool fWar)
{
	ADDTOCALLSTACK("CClient::Event_CombatMode");
	if ( !m_pChar )
		return;

	if ( fWar && (-g_World.GetTimeDiff(m_timeLastEventWarMode) < 1) )
	{
		SysMessageDefault(DEFMSG_SKILLWAIT_3);
		return;
	}

	bool fSkillFail = true;
	if ( IsTrigUsed(TRIGGER_USERWARMODE) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = m_pChar->IsStatFlag(STATF_War) ? 0 : 1;
		Args.m_iN2 = 1;
		if ( m_pChar->OnTrigger(CTRIG_UserWarmode, m_pChar, &Args) == TRIGRET_RET_TRUE )
			return;

		fWar = Args.m_iN1 ? true : false;
		if ( !Args.m_iN2 )
			fSkillFail = false;
	}

	m_pChar->StatFlag_Mod(STATF_War, fWar);

	if ( m_pChar->IsStatFlag(STATF_DEAD) )
		m_pChar->StatFlag_Mod(STATF_Insubstantial, !fWar);	// manifest the ghost

	if ( fSkillFail )
		m_pChar->Skill_Fail(true);

	addPlayerWarMode();
	m_pChar->Update(m_pChar->IsStatFlag(STATF_DEAD), this);
	m_timeLastEventWarMode = CServTime::GetCurrentTime();
}

bool CClient::Event_Command(LPCTSTR pszCommand, TALKMODE_TYPE mode)
{
	ADDTOCALLSTACK("CClient::Event_Command");
	if ( (mode == TALKMODE_GUILD) || (mode == TALKMODE_ALLIANCE) )
		return false;
	if ( (pszCommand[0] == 0) || Str_Check(pszCommand) )
		return true;		// should not be said

	if ( ((m_pChar->GetID() == CREID_EQUIP_GM_ROBE) && (pszCommand[0] == '=')) || (pszCommand[0] == g_Cfg.m_cCommandPrefix) )
	{
		// Lazy :P
	}
	else
		return false;

	pszCommand += 1;
	GETNONWHITESPACE(pszCommand);
	bool fAllowCommand = g_Cfg.CanUsePrivVerb(this, pszCommand, this);
	bool fAllowSay = true;

	if ( !fAllowCommand )
		fAllowSay = (GetPrivLevel() <= PLEVEL_Player);

	// Filter on commands is active - so trigger it
	if ( !g_Cfg.m_sCommandTrigger.IsEmpty() )
	{
		CScriptTriggerArgs Args(pszCommand);
		Args.m_iN1 = fAllowCommand;
		Args.m_iN2 = fAllowSay;
		TRIGRET_TYPE tr;

		//	Call the filtering function
		if ( m_pChar->r_Call(g_Cfg.m_sCommandTrigger, m_pChar, &Args, NULL, &tr) )
		{
			if ( tr == TRIGRET_RET_TRUE )
				return (Args.m_iN2 != 0);
		}

		fAllowCommand = (Args.m_iN1 != 0);
		fAllowSay = (Args.m_iN2 != 0);
	}

	if ( !fAllowCommand && !fAllowSay )
		SysMessageDefault(DEFMSG_MSG_ACC_PRIV);

	if ( fAllowCommand )
	{
		fAllowSay = false;

		// Assume you don't mean yourself
		if ( FindTableHeadSorted(pszCommand, sm_szCmd_Redirect, COUNTOF(sm_szCmd_Redirect)) >= 0 )
		{
			// Targetted verbs are logged once the target is selected
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
		g_Log.Event(LOGM_GM_CMDS, "%lx:'%s' commands '%s'=%d\n", GetSocketID(), GetName(), pszCommand, fAllowCommand);

	return !fAllowSay;
}

// Client is attacking someone
void CClient::Event_Attack(CGrayUID uid)
{
	ADDTOCALLSTACK("CClient::Event_Attack");
	if ( !m_pChar )
		return;

	CChar *pChar = uid.CharFind();
	if ( !pChar )
		return;

	new PacketAttack(this, (m_pChar->Fight_Attack(pChar) ? pChar->GetUID() : static_cast<CGrayUID>(UID_CLEAR)));
}

void CClient::Event_VendorBuy_Cheater(int iCode)
{
	ADDTOCALLSTACK("CClient::Event_VendorBuy_Cheater");
	static LPCTSTR const sm_BuyPacketCheats[] =
	{
		"Other",
		"Bad vendor UID",
		"Bad item UID",
		"Requested items out of stock",
		"Total cost is too high"
	};
	g_Log.Event(LOGL_WARN|LOGM_CHEAT, "%lx:Cheater '%s' is submitting illegal buy packet (%s)\n", GetSocketID(), m_pAccount->GetName(), sm_BuyPacketCheats[iCode]);
}

// Client is buying items from an vendor
void CClient::Event_VendorBuy(CChar *pVendor, const VendorItem *items, size_t iItemCount)
{
	ADDTOCALLSTACK("CClient::Event_VendorBuy");
	if ( !m_pChar || !pVendor )
		return;
	if ( !items || (iItemCount <= 0) )
	{
		addVendorClose(pVendor);
		return;
	}

	bool fPlayerVendor = pVendor->IsStatFlag(STATF_Pet);
	pVendor->GetContainerCreate(LAYER_VENDOR_STOCK);
	CItemContainer *pPack = m_pChar->GetContainerCreate(LAYER_PACK);

	CItemVendable *pItem = NULL;
	WORD wAmount = 0;
	DWORD dwCostTotal = 0;
	short iFollowerSlots = 0;

	// Check if there's something that can block the action
	for ( size_t i = 0; i < iItemCount; ++i )
	{
		if ( !items[i].m_serial.IsValidUID() )
			continue;

		pItem = static_cast<CItemVendable *>(items[i].m_serial.ItemFind());
		if ( !pItem )
			continue;

		if ( (items[i].m_amount <= 0) || (items[i].m_amount > pItem->GetAmount()) )
		{
			pVendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_ORDER_CANTFULFILL));
			Event_VendorBuy_Cheater(0x3);
			return;
		}

		switch ( pItem->GetType() )
		{
			case IT_FIGURINE:
			{
				if ( IsSetOF(OF_PetSlots) )
				{
					CCharBase *pPetDef = CCharBase::FindCharBase(pItem->m_itFigurine.m_ID);
					iFollowerSlots += items[i].m_amount * (pPetDef ? pPetDef->m_FollowerSlots : 1);
				}
				break;
			}
			case IT_HAIR:
			{
				if ( !m_pChar->IsPlayableCharacter() )
				{
					pVendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_ORDER_CANTFULFILL));
					continue;
				}
				break;
			}
			case IT_BEARD:
			{
				if ( (m_pChar->GetDispID() != CREID_MAN) && (m_pChar->GetDispID() != CREID_GARGMAN) )
				{
					pVendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_ORDER_CANTFULFILL));
					return;
				}
				break;
			}
		}

		dwCostTotal += items[i].m_amount * items[i].m_price;
	}

	if ( dwCostTotal > ULONG_MAX / 4 )
	{
		pVendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_ORDER_CANTFULFILL));
		Event_VendorBuy_Cheater(0x4);
		return;
	}
	else if ( iFollowerSlots && !m_pChar->FollowersUpdate(pVendor, iFollowerSlots, true) )
	{
		m_pChar->SysMessageDefault(DEFMSG_PETSLOTS_TRY_CONTROL);
		return;
	}

	// Check if buyer have enough gold
	bool fOwner = pVendor->NPC_IsOwnedBy(m_pChar);
	if ( !fOwner )
	{
		if ( g_Cfg.m_iFeatureTOL & FEATURE_TOL_VIRTUALGOLD )
		{
			if ( m_pChar->m_virtualGold < dwCostTotal )
			{
				pVendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_BUY_CANTAFFORD_BANK));
				return;
			}
		}
		else
		{
			DWORD dwGold = m_pChar->GetContainerCreate(LAYER_PACK)->ContentConsume(RESOURCE_ID(RES_TYPEDEF, IT_GOLD), dwCostTotal, true);
			if ( !g_Cfg.m_fPayFromPackOnly && dwGold )
				dwGold = m_pChar->ContentConsume(RESOURCE_ID(RES_TYPEDEF, IT_GOLD), dwCostTotal, true);

			if ( dwGold )
			{
				pVendor->Speak(g_Cfg.GetDefaultMsg(g_Cfg.m_fPayFromPackOnly ? DEFMSG_NPC_VENDOR_BUY_CANTAFFORD : DEFMSG_NPC_VENDOR_BUY_CANTAFFORD_BANK));
				return;
			}
		}
	}

	// Move bought items to backpack
	for ( size_t i = 0; i < iItemCount; ++i )
	{
		if ( !items[i].m_serial.IsValidUID() )
			continue;

		pItem = static_cast<CItemVendable *>(items[i].m_serial.ItemFind());
		if ( !pItem )
			continue;

		wAmount = items[i].m_amount;

		if ( IsTrigUsed(TRIGGER_BUY) || IsTrigUsed(TRIGGER_ITEMBUY) )
		{
			CScriptTriggerArgs Args(wAmount, static_cast<INT64>(wAmount) * items[i].m_price, pVendor);
			Args.m_VarsLocal.SetNum("TOTALCOST", dwCostTotal);
			if ( pItem->OnTrigger(ITRIG_Buy, GetChar(), &Args) == TRIGRET_RET_TRUE )
				continue;
		}

		if ( !fPlayerVendor )		// NPC vendors
		{
			pItem->SetAmount(pItem->GetAmount() - wAmount);

			switch ( pItem->GetType() )
			{
				case IT_FIGURINE:
				{
					// Pets must be created on ground instead place the figurine on backpack
					for ( int j = 0; j < wAmount; ++j )
						m_pChar->Use_Figurine(pItem);
					goto do_consume;
				}
				case IT_HAIR:
				case IT_BEARD:
				{
					// Hair and beard must be placed directly on char instead backpack
					CItem *pItemNew = CItem::CreateDupeItem(pItem);
					m_pChar->LayerAdd(pItemNew);
					pItemNew->m_TagDefs.SetNum("NOSAVE", 0, true);
					break;
				}
			}

			if ( (wAmount > 1) && !pItem->Item_GetDef()->IsStackableType() )
			{
				while ( wAmount-- )
				{
					CItem *pItemNew = CItem::CreateDupeItem(pItem);
					pItemNew->SetAmount(1);
					pItemNew->m_TagDefs.SetNum("NOSAVE", 0, true);
					if ( !pPack->CanContainerHold(pItemNew, m_pChar) || !m_pChar->CanCarry(pItemNew) )
						m_pChar->ItemDrop(pItemNew, m_pChar->GetTopPoint());
					else
						pPack->ContentAdd(pItemNew);
				}
			}
			else
			{
				CItem *pItemNew = CItem::CreateDupeItem(pItem);
				pItemNew->SetAmount(wAmount);
				pItemNew->m_TagDefs.SetNum("NOSAVE", 0, true);
				if ( !pPack->CanContainerHold(pItemNew, m_pChar) || !m_pChar->CanCarry(pItemNew) )
					m_pChar->ItemDrop(pItemNew, m_pChar->GetTopPoint());
				else
					pPack->ContentAdd(pItemNew);
			}
		}
		else						// Player vendors
		{
			if ( pItem->GetAmount() <= wAmount )		// buy the whole item
			{
				if ( !pPack->CanContainerHold(pItem, m_pChar) || !m_pChar->CanCarry(pItem) )
					m_pChar->ItemDrop(pItem, m_pChar->GetTopPoint());
				else
					pPack->ContentAdd(pItem);

				pItem->m_TagDefs.SetNum("NOSAVE", 0, true);
			}
			else
			{
				pItem->SetAmount(pItem->GetAmount() - wAmount);

				CItem *pItemNew = CItem::CreateDupeItem(pItem);
				pItemNew->m_TagDefs.SetNum("NOSAVE", 0, true);
				pItemNew->SetAmount(wAmount);
				if ( !pPack->CanContainerHold(pItemNew, m_pChar) || !m_pChar->CanCarry(pItemNew) )
					m_pChar->ItemDrop(pItemNew, m_pChar->GetTopPoint());
				else
					pPack->ContentAdd(pItemNew);
			}
		}

	do_consume:
		pItem->Update();
	}

	// Consume gold
	if ( fOwner )
		pVendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_BUY_CHARGE));
	else
	{
		TCHAR *pszText = Str_GetTemp();
		if ( g_Cfg.m_iFeatureTOL & FEATURE_TOL_VIRTUALGOLD )
		{
			m_pChar->m_virtualGold -= dwCostTotal;
			sprintf(pszText, g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_BUY_BANK), static_cast<int>(dwCostTotal));
		}
		else
		{
			DWORD dwGold = m_pChar->GetContainerCreate(LAYER_PACK)->ContentConsume(RESOURCE_ID(RES_TYPEDEF, IT_GOLD), dwCostTotal);
			if ( !g_Cfg.m_fPayFromPackOnly && dwGold )
				m_pChar->ContentConsume(RESOURCE_ID(RES_TYPEDEF, IT_GOLD), dwGold);
			sprintf(pszText, g_Cfg.GetDefaultMsg(g_Cfg.m_fPayFromPackOnly ? DEFMSG_NPC_VENDOR_BUY : DEFMSG_NPC_VENDOR_BUY_BANK), static_cast<int>(dwCostTotal));
		}
		pVendor->GetContainerCreate(LAYER_BANKBOX)->m_itEqBankBox.m_Check_Amount += dwCostTotal;
		pVendor->Speak(pszText);
	}

	// Close vendor gump
	addVendorClose(pVendor);
	addSound(SOUND_DROP_GOLD1);
}

void CClient::Event_VendorSell_Cheater(int iCode)
{
	ADDTOCALLSTACK("CClient::Event_VendorSell_Cheater");
	static LPCTSTR const sm_SellPacketCheats[] =
	{
		"Other",
		"Bad vendor UID",
		"Bad item UID"
	};
	g_Log.Event(LOGL_WARN|LOGM_CHEAT, "%lx:Cheater '%s' is submitting illegal sell packet (%s)\n", GetSocketID(), m_pAccount->GetName(), sm_SellPacketCheats[iCode]);
}

// Client is selling items to an vendor
void CClient::Event_VendorSell(CChar *pVendor, const VendorItem *items, size_t iItemCount)
{
	ADDTOCALLSTACK("CClient::Event_VendorSell");
	if ( !m_pChar || !pVendor || !items || (iItemCount <= 0) )
		return;

	CItemContainer *pBank = pVendor->GetContainerCreate(LAYER_BANKBOX);
	CItemContainer *pContBuy = pVendor->GetContainerCreate(LAYER_VENDOR_BUYS);
	CItemContainer *pContExtra = pVendor->GetContainerCreate(LAYER_VENDOR_EXTRA);
	int iConvertFactor = -pVendor->NPC_GetVendorMarkup();
	DWORD dwGold = 0;
	bool fShortfall = false;

	for ( size_t i = 0; i < iItemCount; ++i )
	{
		CItemVendable *pItem = dynamic_cast<CItemVendable *>(items[i].m_serial.ItemFind());
		if ( !pItem || !pItem->IsValidSaleItem(true) )
		{
			pVendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_ORDER_CANTFULFILL));
			Event_VendorSell_Cheater(0x2);
			return;
		}

		// Do we still have it ? (cheat check)
		if ( pItem->GetTopLevelObj() != m_pChar )
			continue;

		// Find the valid sell item from vendors stuff.
		CItemVendable *pItemSell = CChar::NPC_FindVendableItem(pItem, pContBuy);
		if ( !pItemSell )
			continue;

		// Now how much did i say i wanted to sell?
		WORD wAmount = items[i].m_amount;
		if ( pItem->GetAmount() < wAmount )	// selling more than I have?
			wAmount = pItem->GetAmount();

		DWORD dwPrice = pItemSell->GetVendorPrice(iConvertFactor) * wAmount;

		if ( IsTrigUsed(TRIGGER_SELL) || IsTrigUsed(TRIGGER_ITEMSELL) )
		{
			CScriptTriggerArgs Args(wAmount, dwPrice, pVendor);
			if ( pItem->OnTrigger(ITRIG_Sell, GetChar(), &Args) == TRIGRET_RET_TRUE )
				continue;
		}

		// Can vendor afford this ?
		if ( dwPrice > pBank->m_itEqBankBox.m_Check_Amount )
		{
			fShortfall = true;
			break;
		}
		pBank->m_itEqBankBox.m_Check_Amount -= dwPrice;

		// give them the appropriate amount of gold.
		dwGold += dwPrice;

		// Take the items from player.
		// Put items in vendor inventory.
		if ( wAmount >= pItem->GetAmount() )
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
				CItem *pItemNew = CItem::CreateDupeItem(pItem);
				pItemNew->SetAmount(wAmount);
				pContExtra->ContentAdd(pItemNew);
			}
			pItem->SetAmountUpdate(pItem->GetAmount() - wAmount);
		}
	}

	if ( dwGold )
	{
		TCHAR *pszText = Str_GetTemp();
		sprintf(pszText, g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_SELL), static_cast<int>(dwGold));
		pVendor->Speak(pszText);

		if ( fShortfall )
			pVendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_SELL_CANTAFFORD));

		if ( g_Cfg.m_iFeatureTOL & FEATURE_TOL_VIRTUALGOLD )
			m_pChar->m_virtualGold += dwGold;
		else
			m_pChar->AddGoldToPack(dwGold, NULL, false);

		addVendorClose(pVendor);
		addSound(SOUND_DROP_GOLD1);
		addSound(SOUND_DROP_GOLD2);
	}
	else
	{
		if ( fShortfall )
			pVendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_SELL_CANTAFFORD));
	}
}

void CClient::Event_Profile(bool fWrite, CChar *pChar, TCHAR *pszText)
{
	ADDTOCALLSTACK("CClient::Event_Profile");
	if ( !m_pChar || !pChar || !pChar->m_pPlayer )
		return;

	if ( fWrite )
	{
		if ( ((pChar != m_pChar) && (!IsPriv(PRIV_GM) || (m_pChar->GetPrivLevel() < pChar->GetPrivLevel()))) )
			return;
		pszText = Str_TrimWhitespace(pszText);
	}

	if ( IsTrigUsed(TRIGGER_PROFILE) )
	{
		if ( pChar->OnTrigger(CTRIG_Profile, m_pChar) == TRIGRET_RET_TRUE )
			return;
	}

	if ( fWrite )
		pChar->m_pPlayer->m_sProfile = pszText;
	else
		new PacketProfile(this, pChar);
}

void CClient::Event_MailMsg(CChar *pChar)
{
	ADDTOCALLSTACK("CClient::Event_MailMsg");
	// Ultima Messenger button pressed on paperdoll (obsolete, feature removed since client 3.0.6e)
	// NOTE: No hardcoded behavior, this should be softcoded using @UserMailBag trigger
	if ( !m_pChar )
		return;

	if ( !pChar )
	{
		SysMessageDefault(DEFMSG_MSG_MAILBAG_DROP_1);
		return;
	}
	if ( pChar == m_pChar )		// for some reason this is normal at startup
		return;

	if ( IsTrigUsed(TRIGGER_USERMAILBAG) )
	{
		if ( pChar->OnTrigger(CTRIG_UserMailBag, m_pChar) == TRIGRET_RET_TRUE )
			return;
		pChar->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_MAILBAG_DROP_2), m_pChar->GetName());
	}
}

void CClient::Event_ToolTip(CGrayUID uid)
{
	ADDTOCALLSTACK("CClient::Event_ToolTip");
	CObjBase *pObj = uid.ObjFind();
	if ( !pObj )
		return;

	if ( IsTrigUsed(TRIGGER_TOOLTIP) || (IsTrigUsed(TRIGGER_ITEMTOOLTIP) && pObj->IsItem()) )
	{
		if ( pObj->OnTrigger("@ToolTip", this) == TRIGRET_RET_TRUE )	// CTRIG_ToolTip, ITRIG_ToolTip
			return;
	}

	char *z = Str_GetTemp();
	sprintf(z, "'%s'", pObj->GetName());
	addToolTip(uid.ObjFind(), z);
}

// Client replied the addPrompt sent by server
void CClient::Event_PromptResp(LPCTSTR pszText, size_t iTextLen, CGrayUID uidChar, CGrayUID uidPrompt, DWORD dwType)
{
	ADDTOCALLSTACK("CClient::Event_PromptResp");
	if ( !m_pChar )
		return;
	if ( Str_Check(pszText) )
		return;

	CLIMODE_TYPE promptMode = m_Prompt_Mode;
	m_Prompt_Mode = CLIMODE_NORMAL;

	if ( m_Prompt_Uid != uidChar )
		return;

	TCHAR szText[MAX_TALK_BUFFER];
	if ( iTextLen <= 0 )		// cancel
		szText[0] = '\0';
	else if ( promptMode == CLIMODE_PROMPT_SCRIPT_VERB )
		iTextLen = Str_GetBare(szText, pszText, COUNTOF(szText), "|~=[]{|}~");
	else
		iTextLen = Str_GetBare(szText, pszText, COUNTOF(szText), "|~,=[]{|}~");

	LPCTSTR pszPrefix = "";

	switch ( promptMode )
	{
		case CLIMODE_PROMPT_NAME_RUNE:
		case CLIMODE_PROMPT_NAME_SIGN:
			break;

		case CLIMODE_PROMPT_NAME_KEY:
			pszPrefix = g_Cfg.GetDefaultMsg(DEFMSG_KEY_TO);
			break;

		case CLIMODE_PROMPT_NAME_SHIP:
			pszPrefix = "SS ";
			break;

		case CLIMODE_PROMPT_GM_PAGE_TEXT:
			// m_Targ_Text
			Event_PromptResp_GMPage(szText);
			return;

		case CLIMODE_PROMPT_VENDOR_PRICE:
		{
			// Setting the vendor price for an item.
			if ( (dwType == 0) || (szText[0] == '\0') )	// cancel
				return;
			CChar *pCharVendor = uidPrompt.CharFind();
			if ( pCharVendor )
				pCharVendor->NPC_SetVendorPrice(m_Prompt_Uid.ItemFind(), ATOI(szText));
			return;
		}

		case CLIMODE_PROMPT_TARG_VERB:
			// Send a msg to the pre-tergetted player. "ETARGVERB"
			// m_Prompt_Uid = the target.
			// m_Prompt_Text = the prefix.
			if ( szText[0] != '\0' )
			{
				CObjBase *pObj = m_Prompt_Uid.ObjFind();
				if ( pObj )
				{
					CScript script(m_Prompt_Text, szText);
					pObj->r_Verb(script, this);
				}
			}
			return;

		case CLIMODE_PROMPT_SCRIPT_VERB:
		{
			CScript script(m_Prompt_Text, szText);
			if ( m_pChar )
				m_pChar->r_Verb(script, this);
			return;
		}

		case CLIMODE_PROMPT_STONE_NAME:
			pszPrefix = g_Cfg.GetDefaultMsg(DEFMSG_STONE_FOR);
			break;

		default:
			SysMessageDefault(DEFMSG_MSG_PROMPT_UNEXPECTED);
			return;
	}

	CItem *pItem = m_Prompt_Uid.ItemFind();
	if ( !pItem || (dwType == 0) || (szText[0] == '\0') )
	{
		SysMessageDefault(DEFMSG_MSG_RENAME_CANCEL);
		return;
	}

	if ( g_Cfg.IsObscene(szText) )
	{
		SysMessageDefault(DEFMSG_MSG_RENAME_OBSCENE);
		return;
	}

	CGString sMsg;
	sMsg.Format("%s%s", pszPrefix, szText);
	pItem->SetName(sMsg);

	if ( promptMode == CLIMODE_PROMPT_NAME_RUNE )
		SysMessageDefault(DEFMSG_RUNE_RENAME_SUCCESS);
}

void CClient::Event_PromptResp_GMPage(LPCTSTR pszReason)
{
	ADDTOCALLSTACK("CClient::Event_PromptResp_GMPage");
	// Player sent an GM page
	// CLIMODE_PROMPT_GM_PAGE_TEXT

	if ( pszReason[0] == '\0' )
	{
		SysMessageDefault(DEFMSG_GMPAGE_PROMPT_CANCEL);
		return;
	}

	const CPointMap &pt = m_pChar->GetTopPoint();
	TCHAR *pszMsg = Str_GetTemp();
	sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_GMPAGE_RECEIVED), m_pChar->GetName(), static_cast<DWORD>(m_pChar->GetUID()), pt.WriteUsed(), pszReason);
	g_Log.Event(LOGM_NOCONTEXT|LOGM_GM_PAGE, "%s\n", pszMsg);

	CGMPage *pGMPage = static_cast<CGMPage *>(g_World.m_GMPages.GetHead());
	for ( ; pGMPage != NULL; pGMPage = pGMPage->GetNext() )
	{
		if ( strcmp(pGMPage->GetName(), m_pAccount->GetName()) == 0 )
			break;
	}

	if ( pGMPage )
		SysMessageDefault(DEFMSG_GMPAGE_UPDATED);
	else
	{
		pGMPage = new CGMPage(m_pAccount->GetName());
		SysMessageDefault(DEFMSG_GMPAGE_SENT);
	}
	pGMPage->m_uidChar = m_pChar->GetUID();
	pGMPage->m_pt = pt;
	pGMPage->m_sReason = pszReason;
	pGMPage->m_time = CServTime::GetCurrentTime();
	SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_GMPAGE_QUEUE), static_cast<int>(g_World.m_GMPages.GetCount()));

	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( pClient->IsPriv(PRIV_GM_PAGE) )
			pClient->SysMessage(pszMsg);
	}
}

void CClient::Event_Talk_Common(TCHAR *szText)
{
	ADDTOCALLSTACK("CClient::Event_Talk_Common");
	if ( !m_pChar || !m_pChar->m_pPlayer || !m_pChar->m_pArea )
		return;

	// Guards are special
	LPCTSTR pszMsgGuards = g_Exp.m_VarDefs.GetKeyStr("guardcall");
	if ( !strnicmp(pszMsgGuards, "", 0) )
		pszMsgGuards = "GUARD,GUARDS";
	if ( FindStrWord(szText, pszMsgGuards) > 0 )
		m_pChar->CallGuards();

	// Are we in a region that can hear ?
	CItemMulti *pItemMulti = dynamic_cast<CItemMulti *>(m_pChar->m_pArea->GetResourceID().ItemFind());
	if ( pItemMulti )
		pItemMulti->OnHearRegion(szText, m_pChar);

	// Are there items on the ground that might hear u ?
	CSector *pSector = m_pChar->GetTopSector();
	if ( pSector->HasListenItems() )
		pSector->OnHearItem(m_pChar, szText);

	// Find an NPC that may have heard us.
	CChar *pChar = NULL;
	CChar *pCharAlt = NULL;
	size_t i = 0;
	int iAltDist = UO_MAP_VIEW_SIGHT;
	bool fGhostSpeak = m_pChar->IsSpeakAsGhost();

	CWorldSearch AreaChars(m_pChar->GetTopPoint(), UO_MAP_VIEW_SIGHT);
	for (;;)
	{
		pChar = AreaChars.GetChar();
		if ( !pChar )
			break;

		if ( pChar->IsStatFlag(STATF_COMM_CRYSTAL) )
		{
			for ( CItem *pItem = pChar->GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
				pItem->OnHear(szText, m_pChar);
		}

		if ( pChar == m_pChar )
			continue;
		if ( fGhostSpeak && !pChar->CanUnderstandGhost() )
			continue;

		bool fNamed = false;
		if ( !strnicmp(szText, "ALL ", 4) )
			i = 4;
		else
		{
			// Named the char specifically ?
			i = pChar->NPC_OnHearName(szText);
			fNamed = true;
		}

		if ( i > 0 )
		{
			while ( ISWHITESPACE(szText[i]) )
				++i;

			if ( pChar->NPC_OnHearPetCmd(szText + i, m_pChar, !fNamed) )
			{
				if ( fNamed || (GetTargMode() == CLIMODE_TARG_PET_CMD) )
					return;
				continue;	// the command might apply to others pets
			}
			if ( fNamed )
				break;
		}

		// Are we close to the char ?
		int iDist = m_pChar->GetTopDist3D(pChar);
		if ( (pChar->Skill_GetActive() == NPCACT_TALK) && (pChar->m_Act_Targ == m_pChar->GetUID()) )	// already talking to him
		{
			pCharAlt = pChar;
			iAltDist = 1;
		}
		else if ( pChar->m_pClient && (iAltDist >= 2) )	// PC's have higher priority
		{
			pCharAlt = pChar;
			iAltDist = 2;	// high priority
		}
		else if ( iDist < iAltDist )	// closest NPC guy ?
		{
			pCharAlt = pChar;
			iAltDist = iDist;
		}
	}

	if ( !pChar )
	{
		i = 0;
		pChar = pCharAlt;
		if ( !pChar )
			return;		// no one heard it
	}

	// Change to lowercase for ease of search
	_strlwr(szText);

	// The char hears you say this.
	pChar->NPC_OnHear(&szText[i], m_pChar);
}

// Client sent ASCII speech text
void CClient::Event_Talk(LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, bool fNoStrip)
{
	ADDTOCALLSTACK("CClient::Event_Talk");
	if ( !m_pChar || !m_pAccount )
		return;
	if ( !((mode == TALKMODE_SYSTEM) || (mode == TALKMODE_EMOTE) || (mode == TALKMODE_WHISPER) || (mode == TALKMODE_YELL) || (mode == TALKMODE_GUILD) || (mode == TALKMODE_ALLIANCE)) )
		return;

	if ( (wHue < HUE_BLUE_LOW) || (wHue > HUE_DYE_HIGH) )
		wHue = HUE_TEXT_DEF;

	m_pAccount->m_lang.Set(NULL);		// default
	if ( mode == TALKMODE_SYSTEM )
		m_pChar->m_SpeechHue = wHue;

	// Rip out the unprintables first
	TCHAR szText[MAX_TALK_BUFFER];
	size_t iLen;

	if ( fNoStrip )
	{
		// The characters in Unicode speech don't need to be filtered
		strncpy(szText, pszText, MAX_TALK_BUFFER - 1);
		szText[MAX_TALK_BUFFER - 1] = '\0';
		iLen = strlen(szText);
	}
	else
	{
		TCHAR szTextG[MAX_TALK_BUFFER];
		strncpy(szTextG, pszText, MAX_TALK_BUFFER - 1);
		szTextG[MAX_TALK_BUFFER - 1] = '\0';
		iLen = Str_GetBare(szText, szTextG, sizeof(szText) - 1);
	}

	if ( iLen <= 0 )
		return;

	pszText = szText;
	GETNONWHITESPACE(pszText);

	if ( !Event_Command(pszText, mode) )
	{
		bool fCancelSpeech = false;
		TCHAR z[MAX_TALK_BUFFER];

		if ( m_pChar->OnTriggerSpeech(false, pszText, m_pChar, mode, wHue) )
			fCancelSpeech = true;

		if ( g_Log.IsLoggedMask(LOGM_PLAYER_SPEAK) )
			g_Log.Event(LOGM_PLAYER_SPEAK, "%lx:'%s' Says '%s' mode=%d%s\n", GetSocketID(), m_pChar->GetName(), pszText, mode, fCancelSpeech ? " (muted)" : "");

		if ( fCancelSpeech )
			return;
		if ( (mode == TALKMODE_GUILD) || (mode == TALKMODE_ALLIANCE) )		// guild/alliance mode will not pass this
			return;

		strncpy(z, pszText, MAX_TALK_BUFFER - 1);
		z[MAX_TALK_BUFFER - 1] = '\0';

		if ( g_Cfg.m_fSuppressCapitals )
		{
			int iChars = strlen(z);
			int iCapitals = 0;
			int i = 0;
			for ( i = 0; i < iChars; ++i )
			{
				if ( (z[i] >= 'A') && (z[i] <= 'Z') )
					++iCapitals;
			}

			if ( (iChars > 5) && (((iCapitals * 100) / iChars) > 75) )		// 75% of chars are in capital letters. lowercase it
			{
				for ( i = 1; i < iChars; ++i )				// instead of the 1st char
				{
					if ( (z[i] >= 'A') && (z[i] <= 'Z') )
						z[i] += 0x20;
				}
			}
		}

		if ( iLen <= MAX_TALK_BUFFER / 2 )	// from this point max 128 chars
		{
			m_pChar->SpeakUTF8(z, wHue, mode, m_pChar->m_fonttype, m_pAccount->m_lang);
			Event_Talk_Common(static_cast<TCHAR *>(z));
		}
	}
}

// Client sent UNICODE speech text
void CClient::Event_TalkUNICODE(NWORD *wszText, int iTextLen, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, LPCTSTR pszLang)
{
	ADDTOCALLSTACK("CClient::Event_TalkUNICODE");
	// Get the text in wide bytes.
	// ENU = English
	// FRC = French
	// mode == TALKMODE_SYSTEM if coming from player talking.
	if ( !m_pChar || !m_pAccount || (iTextLen <= 0) )
		return;
	if ( !((mode == TALKMODE_SYSTEM) || (mode == TALKMODE_EMOTE) || (mode == TALKMODE_WHISPER) || (mode == TALKMODE_YELL) || (mode == TALKMODE_GUILD) || (mode == TALKMODE_ALLIANCE)) )
		return;

	if ( (wHue < HUE_BLUE_LOW) || (wHue > HUE_DYE_HIGH) )
		wHue = HUE_TEXT_DEF;

	m_pAccount->m_lang.Set(pszLang);
	if ( mode == TALKMODE_SYSTEM )
		m_pChar->m_SpeechHue = wHue;

	TCHAR szText[MAX_TALK_BUFFER];
	const NWORD *puText = wszText;

	int iLen = CvtNUNICODEToSystem(szText, sizeof(szText), wszText, iTextLen);
	if ( iLen <= 0 )
		return;

	TCHAR *pszText = szText;
	GETNONWHITESPACE(pszText);

	if ( !Event_Command(pszText, mode) )
	{
		bool fCancelSpeech = false;
		if ( m_pChar->OnTriggerSpeech(false, pszText, m_pChar, mode, wHue) )
			fCancelSpeech = true;

		if ( g_Log.IsLoggedMask(LOGM_PLAYER_SPEAK) )
			g_Log.Event(LOGM_PLAYER_SPEAK, "%lx:'%s' Says UNICODE '%s' '%s' mode=%d%s\n", GetSocketID(), m_pChar->GetName(), m_pAccount->m_lang.GetStr(), pszText, mode, fCancelSpeech ? " (muted)" : "");

		if ( fCancelSpeech )
			return;
		if ( (mode == TALKMODE_GUILD) || (mode == TALKMODE_ALLIANCE) )		// guild/alliance mode will not pass this
			return;

		if ( g_Cfg.m_fSuppressCapitals )
		{
			size_t iChars = strlen(szText);
			size_t iCapitals = 0;
			size_t i = 0;
			for ( i = 0; i < iChars; ++i )
			{
				if ( (szText[i] >= 'A') && (szText[i] <= 'Z') )
					++iCapitals;
			}

			if ( (iChars > 5) && (((iCapitals * 100) / iChars) > 75) )		// 75% of chars are in capital letters. lowercase it
			{
				for ( i = 1; i < iChars; ++i )				// instead of the 1st char
				{
					if ( (szText[i] >= 'A') && (szText[i] <= 'Z') )
						szText[i] += 0x20;
				}
				iLen = CvtSystemToNUNICODE(wszText, iTextLen, szText, iChars);
			}
		}

		if ( iLen <= MAX_TALK_BUFFER / 2 )	// from this point max 128 chars
		{
			m_pChar->SpeakUTF8Ex(puText, wHue, mode, font, m_pAccount->m_lang);
			Event_Talk_Common(pszText);
		}
	}
}

void CClient::Event_CharRename(CChar *pChar, LPCTSTR pszName)
{
	ADDTOCALLSTACK("CClient::Event_CharRename");
	// Set the name in the character status window.
	if ( !pChar || !m_pChar )
		return;
	if ( Str_CheckName(pszName) || !strlen(pszName) )
		return;

	if ( (m_pChar == pChar) || !pChar->NPC_IsOwnedBy(m_pChar) )
		return;
	if ( FindTableSorted(pszName, sm_szCmd_Redirect, COUNTOF(sm_szCmd_Redirect)) >= 0 )
		return;
	if ( FindTableSorted(pszName, CCharNPC::sm_szVerbKeys, 14) >= 0 )
		return;
	if ( g_Cfg.IsObscene(pszName) )
		return;

	if ( IsTrigUsed(TRIGGER_RENAME) )
	{
		CScriptTriggerArgs Args;
		Args.m_pO1 = pChar;
		Args.m_s1 = pszName;
		if ( m_pChar->OnTrigger(CTRIG_Rename, this, &Args) == TRIGRET_RET_TRUE )
			return;
	}

	pChar->SetName(pszName);
}

bool CDialogResponseArgs::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CDialogResponseArgs::r_WriteVal");
	EXC_TRY("WriteVal");
	if ( !strnicmp(pszKey, "ARGCHK", 6) )
	{
		pszKey += 6;
		SKIP_SEPARATORS(pszKey);

		size_t iQty = m_CheckArray.GetCount();
		if ( pszKey[0] == '\0' )
		{
			sVal.FormatVal(iQty);
			return true;
		}
		else if ( !strnicmp(pszKey, "ID", 2) )
		{
			pszKey += 2;
			if ( (iQty > 0) && m_CheckArray[0] )
				sVal.FormatVal(m_CheckArray[0]);
			else
				sVal.FormatVal(-1);
			return true;
		}

		DWORD dwNum = static_cast<DWORD>(Exp_GetLLSingle(pszKey));
		SKIP_SEPARATORS(pszKey);
		for ( size_t i = 0; i < iQty; ++i )
		{
			if ( dwNum == m_CheckArray[i] )
			{
				sVal = "1";
				return true;
			}
		}
		sVal = "0";
		return true;
	}
	if ( !strnicmp(pszKey, "ARGTXT", 6) )
	{
		pszKey += 6;
		SKIP_SEPARATORS(pszKey);

		size_t iQty = m_TextArray.GetCount();
		if ( pszKey[0] == '\0' )
		{
			sVal.FormatVal(iQty);
			return true;
		}

		WORD wNum = static_cast<WORD>(Exp_GetSingle(pszKey));
		SKIP_SEPARATORS(pszKey);
		for ( size_t i = 0; i < iQty; ++i )
		{
			if ( wNum == m_TextArray[i]->m_ID )
			{
				sVal = m_TextArray[i]->m_sText;
				return true;
			}
		}
		sVal.Empty();
		return false;
	}
	return CScriptTriggerArgs::r_WriteVal(pszKey, sVal, pSrc);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CClient::Event_DoubleClick(CGrayUID uid, bool fMacro, bool fTestTouch, bool fScript)
{
	ADDTOCALLSTACK("CClient::Event_DoubleClick");
	// Try to use the object in some way.
	// will trigger a OnTarg_Use_Item() usually.
	// fMacro = ALTP vs dbl click. no unmount.

	// Allow some static in game objects to have function?
	// Not possible with dclick.
	if ( !m_pChar )
		return false;

	CObjBase *pObj = uid.ObjFind();
	if ( !pObj || (fTestTouch && !m_pChar->CanSee(pObj) && !(pObj->m_Can & CAN_I_FORCEDC)) )
	{
		addObjectRemoveCantSee(uid, "the target");
		return false;
	}

	// Face the object
	if ( !IsSetOF(OF_NoDClickTurn) && (pObj->GetTopLevelObj() != m_pChar) )
	{
		SetTargMode();
		m_Targ_UID = uid;
		m_pChar->UpdateDir(pObj);
	}

	CItem *pItem = dynamic_cast<CItem *>(pObj);
	if ( pItem )
		return Cmd_Use_Item(pItem, fTestTouch, fScript);

	CChar *pChar = static_cast<CChar *>(pObj);
	if ( IsTrigUsed(TRIGGER_DCLICK) || IsTrigUsed(TRIGGER_CHARDCLICK) )
	{
		if ( pChar->OnTrigger(CTRIG_DClick, m_pChar) == TRIGRET_RET_TRUE )
			return true;
	}

	if ( !fMacro )
	{
		if ( pChar == m_pChar )
		{
			if ( pChar->IsStatFlag(STATF_OnHorse) )
			{
				if ( !IsSetCombatFlags(COMBAT_DCLICKSELF_UNMOUNTS) && pChar->IsStatFlag(STATF_War) )
				{
					addCharPaperdoll(pChar);
					return true;
				}
				if ( m_pChar->Horse_UnMount() )
					return true;
			}
		}

		if ( pChar->m_pNPC && (pChar->GetNPCBrain() != NPCBRAIN_HUMAN) )
		{
			if ( m_pChar->Horse_Mount(pChar) )
				return true;

			switch ( pChar->GetID() )
			{
				case CREID_HORSE_PACK:
				case CREID_LLAMA_PACK:
					return Cmd_Use_Item(pChar->GetContainerCreate(LAYER_PACK), fTestTouch);	// pack animals open container

				default:
					if ( IsPriv(PRIV_GM) )
						return Cmd_Use_Item(pChar->GetContainerCreate(LAYER_PACK), false);	// snoop the creature
					return false;
			}
		}
	}

	addCharPaperdoll(pChar);
	return true;
}

void CClient::Event_SingleClick(CGrayUID uid)
{
	ADDTOCALLSTACK("CClient::Event_SingleClick");
	// The client is doing a single click on obj.
	// Also called to show incoming char names
	// on screen (including ALLNAMES macro).
	//
	// PS: Clients using tooltips don't send single click
	//	   requests when clicking on obj.
	if ( !m_pChar )
		return;

	CObjBase *pObj = uid.ObjFind();
	if ( !m_pChar->CanSee(pObj) )
	{
		// ALLNAMES makes this happen as we are running thru an area,
		// so display no msg. Do not use addObjectRemoveCantSee()
		addObjectRemove(uid);
		return;
	}

	if ( IsTrigUsed(TRIGGER_CLICK) || (IsTrigUsed(TRIGGER_ITEMCLICK) && pObj->IsItem()) || (IsTrigUsed(TRIGGER_CHARCLICK) && pObj->IsChar()) )
	{
		CScriptTriggerArgs Args(this);
		if ( pObj->OnTrigger("@Click", m_pChar, &Args) == TRIGRET_RET_TRUE )	// CTRIG_Click, ITRIG_Click
			return;
	}

	if ( pObj->IsItem() )
	{
		addItemName(static_cast<const CItem *>(pObj));
		return;
	}

	if ( pObj->IsChar() )
	{
		addCharName(static_cast<const CChar *>(pObj));
		return;
	}

	SysMessagef("Bogus item UID=0%lx?", static_cast<DWORD>(uid));
}

void CClient::Event_Target(CLIMODE_TYPE context, CGrayUID uid, CPointMap pt, BYTE flags, ITEMID_TYPE id)
{
	ADDTOCALLSTACK("CClient::Event_Target");
	// If player clicks on something with the targetting cursor
	// Assume addTarget was called before this.
	// NOTE: Make sure they can actually validly trarget this item !
	if ( !m_pChar )
		return;

	if ( context != GetTargMode() )		// unexpected context
	{
		if ( (context != 0) && ((pt.m_x != -1) || (uid.GetPrivateUID() != UID_CLEAR)) )
			SysMessageDefault(DEFMSG_MSG_TARG_UNEXPECTED);
		return;
	}

	if ( (pt.m_x == -1) && (uid.GetPrivateUID() == UID_CLEAR) )		// cancelled
	{
		SetTargMode();
		return;
	}

	CLIMODE_TYPE prevmode = GetTargMode();
	ClearTargMode();

	if ( m_NetState->isClientKR() && (flags & 0xA0) )
		uid = m_Targ_Last;

	CObjBase *pTarget = uid.ObjFind();
	if ( IsPriv(PRIV_GM) )
	{
		if ( uid.IsValidUID() && !pTarget )
		{
			addObjectRemoveCantSee(uid, "the target");
			return;
		}
	}
	else
	{
		if ( uid.IsValidUID() )
		{
			if ( !m_pChar->CanSee(pTarget) )
			{
				addObjectRemoveCantSee(uid, "the target");
				return;
			}
		}
		else
		{
			if ( m_pChar->GetTopPoint().GetDist(pt) > m_pChar->GetSight() )
				return;
		}
	}

	if ( pTarget )
	{
		m_Targ_Last = uid;		// remove the last existing target
		pt = pTarget->GetTopLevelObj()->GetTopPoint();		// point inside a container is not really meaningful here
	}

	switch ( prevmode )
	{
		// GM stuff
		case CLIMODE_TARG_OBJ_SET:			OnTarg_Obj_Set(pTarget);					break;
		case CLIMODE_TARG_OBJ_INFO:			OnTarg_Obj_Info(pTarget, pt, id);			break;
		case CLIMODE_TARG_OBJ_FUNC:			OnTarg_Obj_Function(pTarget, pt, id);		break;
		case CLIMODE_TARG_UNEXTRACT:		OnTarg_UnExtract(pTarget, pt);				break;
		case CLIMODE_TARG_ADDCHAR:			OnTarg_Char_Add(pTarget, pt);				break;
		case CLIMODE_TARG_ADDITEM:			OnTarg_Item_Add(pTarget, pt);				break;
		case CLIMODE_TARG_LINK:				OnTarg_Item_Link(pTarget);					break;
		case CLIMODE_TARG_TILE:				OnTarg_Tile(pTarget, pt);					break;

		// Player stuff
		case CLIMODE_TARG_SKILL:			OnTarg_Skill(pTarget);						break;
		case CLIMODE_TARG_SKILL_MAGERY:		OnTarg_Skill_Magery(pTarget, pt);			break;
		case CLIMODE_TARG_SKILL_HERD_DEST:	OnTarg_Skill_Herd_Dest(pTarget, pt);		break;
		case CLIMODE_TARG_SKILL_POISON:		OnTarg_Skill_Poison(pTarget);				break;
		case CLIMODE_TARG_SKILL_PROVOKE:	OnTarg_Skill_Provoke(pTarget);				break;
		case CLIMODE_TARG_REPAIR:			m_pChar->Use_Repair(uid.ItemFind());		break;
		case CLIMODE_TARG_PET_CMD:			OnTarg_Pet_Command(pTarget, pt);			break;
		case CLIMODE_TARG_PET_STABLE:		OnTarg_Pet_Stable(uid.CharFind());			break;
		case CLIMODE_TARG_USE_ITEM:			OnTarg_Use_Item(pTarget, pt, id);			break;
		case CLIMODE_TARG_STONE_RECRUIT:	OnTarg_Stone_Recruit(uid.CharFind());		break;
		case CLIMODE_TARG_STONE_RECRUITFULL:OnTarg_Stone_Recruit(uid.CharFind(), true);	break;
		case CLIMODE_TARG_PARTY_ADD:		OnTarg_Party_Add(uid.CharFind());			break;
		case CLIMODE_TARG_PARTY_REMOVE:		OnTarg_Party_Remove(uid.CharFind());		break;
		case CLIMODE_TARG_GLOBALCHAT_ADD:	OnTarg_GlobalChat_Add(uid.CharFind());		break;

		default:																		break;
	}
}

void CClient::Event_AOSPopupMenuRequest(CGrayUID uid) //construct packet after a client request
{
	ADDTOCALLSTACK("CClient::Event_AOSPopupMenuRequest");

	CObjBaseTemplate *pObj = uid.ObjFind();
	if ( !m_pChar || (!IsSetOF(OF_NoContextMenuLOS) && !m_pChar->CanSeeLOS(pObj)) )
		return;

	if ( m_pPopupPacket )
	{
		delete m_pPopupPacket;
		m_pPopupPacket = NULL;
	}
	m_pPopupPacket = new PacketDisplayPopup(this, uid);

	CScriptTriggerArgs Args;
	bool fPreparePacket = false;
	CItem *pItem = uid.ItemFind();
	CChar *pChar = uid.CharFind();

	if ( pItem )
	{
		if ( IsTrigUsed(TRIGGER_CONTEXTMENUREQUEST) || IsTrigUsed(TRIGGER_ITEMCONTEXTMENUREQUEST) )
		{
			Args.m_iN1 = 1;
			static_cast<void>(pItem->OnTrigger(ITRIG_ContextMenuRequest, GetChar(), &Args));
			fPreparePacket = true;		// there's no hardcoded stuff for items
		}
		else
		{
			delete m_pPopupPacket;
			m_pPopupPacket = NULL;
			return;
		}
	}
	else if ( pChar )
	{
		if ( IsTrigUsed(TRIGGER_CONTEXTMENUREQUEST) )
		{
			Args.m_iN1 = 1;
			if ( pChar->OnTrigger(CTRIG_ContextMenuRequest, GetChar(), &Args) == TRIGRET_RET_TRUE )
				fPreparePacket = true;
		}
	}
	else
	{
		delete m_pPopupPacket;
		m_pPopupPacket = NULL;
		return;
	}

	if ( pChar && !fPreparePacket )
	{

		if ( pChar->IsPlayableCharacter() )
			m_pPopupPacket->addOption(POPUP_OPEN_PAPERDOLL, 6123);

		if ( pChar->m_pNPC )
		{
			if ( pChar->NPC_IsVendor() )
			{
				if ( pChar->m_pNPC->m_Brain == NPCBRAIN_BANKER )
					m_pPopupPacket->addOption(POPUP_OPEN_BANKBOX, 6105);

				m_pPopupPacket->addOption(POPUP_BUY, 6103);
				m_pPopupPacket->addOption(POPUP_SELL, 6104);

				for ( unsigned int i = 0; i < g_Cfg.m_iMaxSkill; ++i )
				{
					if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(i) )
						continue;

					SKILL_TYPE skillCheck = static_cast<SKILL_TYPE>(i);
					if ( pChar->Skill_GetBase(skillCheck) <= 0 )
						continue;
					if ( skillCheck == SKILL_SPELLWEAVING )		// skip this skill, because on OSI it can be trained only on quests so they didn't included the cliloc to use on NPCs
						continue;

					WORD wFlag = (pChar->NPC_GetTrainValue(m_pChar, skillCheck) <= 0) ? POPUPFLAG_DISABLED : 0;
					m_pPopupPacket->addOption(POPUP_TRAIN_SKILL + i, 6000 + i, wFlag);
				}

				if ( pChar->m_pNPC->m_Brain == NPCBRAIN_ANIMAL_TRAINER )
				{
					m_pPopupPacket->addOption(POPUP_STABLE_PET, 6126);
					m_pPopupPacket->addOption(POPUP_CLAIM_PETS, 6127);
				}
			}
			else
			{
				WORD wFlag = pChar->IsStatFlag(STATF_DEAD) ? POPUPFLAG_DISABLED : 0;
				if ( pChar->NPC_IsOwnedBy(m_pChar, false) )
				{
					CREID_TYPE id = pChar->GetID();
					bool fBackpack = ((id == CREID_HORSE_PACK) || (id == CREID_LLAMA_PACK) || (id == CREID_GIANT_BEETLE));

					m_pPopupPacket->addOption(POPUP_PETCMD_GUARD, 6107, wFlag);
					m_pPopupPacket->addOption(POPUP_PETCMD_FOLLOW, 6108, POPUPFLAG_COLOR);
					if ( fBackpack )
						m_pPopupPacket->addOption(POPUP_PETCMD_DROP, 6109, wFlag);
					m_pPopupPacket->addOption(POPUP_PETCMD_KILL, 6111, wFlag);
					m_pPopupPacket->addOption(POPUP_PETCMD_STOP, 6112);
					m_pPopupPacket->addOption(POPUP_PETCMD_STAY, 6114);
					if ( !pChar->IsStatFlag(STATF_Conjured) )
					{
						m_pPopupPacket->addOption(POPUP_PETCMD_FRIEND_ADD, 6110, wFlag);
						m_pPopupPacket->addOption(POPUP_PETCMD_FRIEND_REMOVE, 6099, wFlag);
						m_pPopupPacket->addOption(POPUP_PETCMD_TRANSFER, 6113);
					}
					m_pPopupPacket->addOption(POPUP_PETCMD_RELEASE, 6118);
					if ( fBackpack )
						m_pPopupPacket->addOption(POPUP_OPEN_BACKPACK, 6145, wFlag);
				}
				else if ( pChar->Memory_FindObjTypes(m_pChar, MEMORY_FRIEND) )
				{
					m_pPopupPacket->addOption(POPUP_PETCMD_FOLLOW, 6108, wFlag);
					m_pPopupPacket->addOption(POPUP_PETCMD_STOP, 6112, wFlag);
					m_pPopupPacket->addOption(POPUP_PETCMD_STAY, 6114, wFlag);
				}
				else if ( !pChar->IsStatFlag(STATF_Pet) && (pChar->Skill_GetBase(SKILL_TAMING) > 0) )
					m_pPopupPacket->addOption(POPUP_TAME, 6130);
			}
		}
		else if ( pChar->m_pPlayer && (pChar == m_pChar) )
		{
			m_pPopupPacket->addOption(POPUP_OPEN_BACKPACK, 6145);
			if ( m_NetState->isClientVersion(MINCLIVER_NEWCONTEXTMENU) && pChar->m_pParty )
			{
				m_pPopupPacket->addOption(POPUP_PARTY_LEAVE, pChar->m_pParty->IsPartyMaster(pChar) ? 196 : 195);
				m_pPopupPacket->addOption(POPUP_PARTY_LOOT, m_pChar->m_pParty->GetLootFlag(m_pChar) ? 199 : 194);
			}
			if ( m_NetState->isClientVersion(MINCLIVER_STATUS_V6) )
			{
				if ( pChar->m_pPlayer->m_bRefuseTrades )
					m_pPopupPacket->addOption(POPUP_TRADE_ALLOW, 1154112);
				else
					m_pPopupPacket->addOption(POPUP_TRADE_REFUSE, 1154113);
			}
			if ( m_NetState->isClientVersion(MINCLIVER_GLOBALCHAT) && (g_Cfg.m_iChatFlags & CHATF_GLOBALCHAT) )
			{
				if ( pChar->m_pPlayer->m_bRefuseGlobalChatRequests )
					m_pPopupPacket->addOption(POPUP_GLOBALCHAT_ALLOW, 1158415);
				else
					m_pPopupPacket->addOption(POPUP_GLOBALCHAT_REFUSE, 1158416);
			}
		}
		else
		{
			if ( m_NetState->isClientVersion(MINCLIVER_NEWCONTEXTMENU) )
			{
				bool fPartyMaster = (m_pChar->m_pParty && m_pChar->m_pParty->IsPartyMaster(m_pChar));
				if ( !pChar->m_pParty && (!m_pChar->m_pParty || fPartyMaster) )
					m_pPopupPacket->addOption(POPUP_PARTY_ADD, 197);
				else if ( fPartyMaster && (pChar->m_pParty == m_pChar->m_pParty) )
					m_pPopupPacket->addOption(POPUP_PARTY_REMOVE, 198);
			}
			if ( m_NetState->isClientVersion(MINCLIVER_TOL) && (m_pChar->GetDist(pChar) <= 2) )
				m_pPopupPacket->addOption(POPUP_TRADE_OPEN, 1077728);
		}

		if ( IsTrigUsed(TRIGGER_CONTEXTMENUREQUEST) && (Args.m_iN1 != 1) )
		{
			Args.m_iN1 = 2;
			static_cast<void>(pChar->OnTrigger(CTRIG_ContextMenuRequest, GetChar(), &Args));
		}
	}

	if ( m_pPopupPacket->getOptionCount() <= 0 )
	{
		delete m_pPopupPacket;
		m_pPopupPacket = NULL;
		return;
	}

	m_pPopupPacket->finalise();
	m_pPopupPacket->push(this);
	m_pPopupPacket = NULL;
}

void CClient::Event_AOSPopupMenuSelect(CGrayUID uid, WORD wIndex)	//do something after a player selected something from a pop-up menu
{
	ADDTOCALLSTACK("CClient::Event_AOSPopupMenuSelect");
	if ( !m_pChar || !wIndex )
		return;

	CObjBase *pObj = uid.ObjFind();
	if ( !IsSetOF(OF_NoContextMenuLOS) && !m_pChar->CanSeeLOS(pObj) )
		return;

	CScriptTriggerArgs Args;
	CItem *pItem = uid.ItemFind();
	if ( pItem )
	{
		if ( IsTrigUsed(TRIGGER_CONTEXTMENUSELECT) || IsTrigUsed(TRIGGER_ITEMCONTEXTMENUSELECT) )
		{
			Args.m_iN1 = wIndex;
			static_cast<void>(pItem->OnTrigger(ITRIG_ContextMenuSelect, GetChar(), &Args));
		}
		return;		// there's no hardcoded stuff for items
	}

	CChar *pChar = uid.CharFind();
	if ( !pChar )
		return;

	if ( IsTrigUsed(TRIGGER_CONTEXTMENUSELECT) )
	{
		Args.m_iN1 = wIndex;
		if ( pChar->OnTrigger(CTRIG_ContextMenuSelect, GetChar(), &Args) == TRIGRET_RET_TRUE )
			return;
	}

	if ( pChar->m_pNPC )
	{
		switch ( wIndex )
		{
			case POPUP_OPEN_BANKBOX:
				pChar->NPC_OnHear("bank", m_pChar);
				return;
			case POPUP_BUY:
				pChar->NPC_OnHear("buy", m_pChar);
				return;
			case POPUP_SELL:
				pChar->NPC_OnHear("sell", m_pChar);
				return;
			case POPUP_TAME:
				if ( m_pChar->Skill_CanUse(SKILL_TAMING) && !m_pChar->Skill_Wait(SKILL_TAMING) )
				{
					m_pChar->m_Act_Targ = pChar->GetUID();
					m_pChar->Skill_Start(SKILL_TAMING);
				}
				return;
			case POPUP_PETCMD_GUARD:
				pChar->NPC_OnHearPetCmd("guard", m_pChar);
				return;
			case POPUP_PETCMD_FOLLOW:
				pChar->NPC_OnHearPetCmd("follow", m_pChar);
				return;
			case POPUP_PETCMD_DROP:
				pChar->NPC_OnHearPetCmd("drop", m_pChar);
				return;
			case POPUP_PETCMD_KILL:
				pChar->NPC_OnHearPetCmd("kill", m_pChar);
				return;
			case POPUP_PETCMD_STOP:
				pChar->NPC_OnHearPetCmd("stop", m_pChar);
				return;
			case POPUP_PETCMD_STAY:
				pChar->NPC_OnHearPetCmd("stay", m_pChar);
				return;
			case POPUP_PETCMD_FRIEND_ADD:
				pChar->NPC_OnHearPetCmd("friend", m_pChar);
				return;
			case POPUP_PETCMD_FRIEND_REMOVE:
				pChar->NPC_OnHearPetCmd("unfriend", m_pChar);
				return;
			case POPUP_PETCMD_TRANSFER:
				pChar->NPC_OnHearPetCmd("transfer", m_pChar);
				return;
			case POPUP_PETCMD_RELEASE:
				pChar->NPC_OnHearPetCmd("release", m_pChar);
				return;
			case POPUP_STABLE_PET:
				pChar->NPC_OnHear("stable", m_pChar);
				return;
			case POPUP_CLAIM_PETS:
				pChar->NPC_OnHear("retrieve", m_pChar);
				return;
		}

		if ( (wIndex >= POPUP_TRAIN_SKILL) && (wIndex < POPUP_TRAIN_SKILL + g_Cfg.m_iMaxSkill) )
		{
			if ( wIndex != POPUP_TRAIN_SKILL + SKILL_SPELLWEAVING )
			{
				TCHAR *pszMsg = Str_GetTemp();
				sprintf(pszMsg, "train %s", g_Cfg.GetSkillKey(static_cast<SKILL_TYPE>(wIndex - POPUP_TRAIN_SKILL)));
				pChar->NPC_OnHear(pszMsg, m_pChar);
				return;
			}
		}
	}

	switch ( wIndex )
	{
		case POPUP_OPEN_PAPERDOLL:
			addCharPaperdoll(pChar);
			return;
		case POPUP_OPEN_BACKPACK:
			m_pChar->Use_Obj(pChar->LayerFind(LAYER_PACK), true);
			return;
		case POPUP_PARTY_ADD:
			OnTarg_Party_Add(pChar);
			return;
		case POPUP_PARTY_REMOVE:
			OnTarg_Party_Remove(pChar);
			return;
		case POPUP_PARTY_LEAVE:
			if ( m_pChar->m_pParty )
				m_pChar->m_pParty->RemoveMember(m_pChar, m_pChar);
			return;
		case POPUP_PARTY_LOOT:
			if ( m_pChar->m_pParty )
				m_pChar->m_pParty->SetLootFlag(m_pChar, !m_pChar->m_pParty->GetLootFlag(m_pChar));
			return;
		case POPUP_TRADE_ALLOW:
			if ( m_pChar->m_pPlayer )
				m_pChar->m_pPlayer->m_bRefuseTrades = false;
			return;
		case POPUP_TRADE_REFUSE:
			if ( m_pChar->m_pPlayer )
				m_pChar->m_pPlayer->m_bRefuseTrades = true;
			return;
		case POPUP_TRADE_OPEN:
			Cmd_SecureTrade(pChar, NULL);
			return;
		case POPUP_GLOBALCHAT_ALLOW:
			if ( m_pChar->m_pPlayer )
				m_pChar->m_pPlayer->m_bRefuseGlobalChatRequests = false;
			return;
		case POPUP_GLOBALCHAT_REFUSE:
			if ( m_pChar->m_pPlayer )
				m_pChar->m_pPlayer->m_bRefuseGlobalChatRequests = true;
			return;
	}
}

void CClient::Event_BugReport(const TCHAR *pszText, int iTextLen, BUGREPORT_TYPE type, CLanguageID lang)
{
	ADDTOCALLSTACK("CClient::Event_BugReport");
	UNREFERENCED_PARAMETER(iTextLen);
	if ( !m_pChar )
		return;

	if ( IsTrigUsed(TRIGGER_USERBUGREPORT) )
	{
		CScriptTriggerArgs Args(type);
		Args.m_s1 = pszText;
		Args.m_VarsLocal.SetStr("LANG", false, lang.GetStr());

		m_pChar->OnTrigger(CTRIG_UserBugReport, m_pChar, &Args);
	}
}

void CClient::Event_UseToolbar(BYTE bType, DWORD dwArg)
{
	ADDTOCALLSTACK("CClient::Event_UseToolbar");
	if ( !m_pChar )
		return;

	if ( IsTrigUsed(TRIGGER_USERKRTOOLBAR) )
	{
		CScriptTriggerArgs Args(bType, dwArg);
		if ( m_pChar->OnTrigger(CTRIG_UserKRToolbar, m_pChar, &Args) == TRIGRET_RET_TRUE )
			return;
	}

	switch ( bType )
	{
		case 0x1:	// spell
			if ( static_cast<SPELL_TYPE>(dwArg) <= SPELL_SPELLWEAVING_QTY )		// KR clients only have support up to spellweaving spells
				Cmd_Skill_Magery(static_cast<SPELL_TYPE>(dwArg), m_pChar);
			return;

		case 0x2:	// combat ability
			Event_CombatAbilitySelect(dwArg);
			return;

		case 0x3:	// skill
			if ( static_cast<SKILL_TYPE>(dwArg) <= SKILL_SPELLWEAVING )		// KR clients only have support up to spellweaving skill
				Event_Skill_Use(static_cast<SKILL_TYPE>(dwArg));
			return;

		case 0x4:	// item
			Event_DoubleClick(static_cast<CGrayUID>(dwArg), true, true);
			return;

		case 0x5:	// virtue
			Event_VirtueSelect(dwArg, m_pChar);
			return;
	}
}

//----------------------------------------------------------------------

void CClient::Event_ExtCmd(EXTCMD_TYPE type, TCHAR *pszArgs)
{
	ADDTOCALLSTACK("CClient::Event_ExtCmd");
	if ( !m_pChar )
		return;

	if ( IsTrigUsed(TRIGGER_USEREXTCMD) )
	{
		CScriptTriggerArgs Args(pszArgs);
		Args.m_iN1 = type;
		if ( m_pChar->OnTrigger(CTRIG_UserExtCmd, m_pChar, &Args) == TRIGRET_RET_TRUE )
			return;
		strcpy(pszArgs, Args.m_s1);
	}

	TCHAR *ppArgs[2];
	Str_ParseCmds(pszArgs, ppArgs, COUNTOF(ppArgs), " ");

	switch ( type )
	{
		case EXTCMD_OpenSpellbook:
		{
			CItem *pBook = NULL;
			switch ( ATOI(ppArgs[0]) )
			{
				default:
				case 1:	pBook = m_pChar->GetSpellbook(SPELL_Clumsy);				break;	// magery
				case 2:	pBook = m_pChar->GetSpellbook(SPELL_Animate_Dead_AOS);		break;	// necromancy
				case 3:	pBook = m_pChar->GetSpellbook(SPELL_Cleanse_by_Fire);		break;	// paladin
				case 4:	pBook = m_pChar->GetSpellbook(SPELL_Honorable_Execution);	break;	// bushido
				case 5:	pBook = m_pChar->GetSpellbook(SPELL_Focus_Attack);			break;	// ninjitsu
				case 6:	pBook = m_pChar->GetSpellbook(SPELL_Arcane_Circle);			break;	// spellweaving
				case 7:	pBook = m_pChar->GetSpellbook(SPELL_Nether_Bolt);			break;	// mysticism
				case 8:	pBook = m_pChar->GetSpellbook(SPELL_Inspire);				break;	// bard
			}
			if ( pBook )
				m_pChar->Use_Obj(pBook, true);
			return;
		}

		case EXTCMD_Gesture:
		{
			if ( !strcmpi(ppArgs[0], "bow") )
				m_pChar->UpdateAnimate(ANIM_BOW);
			else if ( !strcmpi(ppArgs[0], "salute") )
				m_pChar->UpdateAnimate(ANIM_SALUTE);
			return;
		}

		case EXTCMD_UseSkill:
		{
			Event_Skill_Use(static_cast<SKILL_TYPE>(ATOI(ppArgs[0])));
			return;
		}

		case EXTCMD_UseScroll:		// obsolete
		{
			//CObjBase *pObj = static_cast<CGrayUID>(ATOI(ppArgs[0])).ObjFind();
			return;
		}

		case EXTCMD_CastSpellBook:
		case EXTCMD_CastSpell:
		{
			SPELL_TYPE spell = static_cast<SPELL_TYPE>(ATOI(ppArgs[0]));
			CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
			if ( !pSpellDef )
				return;

			if ( IsSetMagicFlags(MAGICF_PRECAST) && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST) )
			{
				int skill;
				if ( !pSpellDef->GetPrimarySkill(&skill) )
					return;

				m_tmSkillMagery.m_Spell = spell;
				m_pChar->m_atMagery.m_Spell = spell;
				m_pChar->m_Act_p = m_pChar->GetTopPoint();
				m_pChar->m_Act_Targ = m_Targ_UID;
				m_pChar->m_Act_TargPrv = m_Targ_PrvUID;
				m_pChar->Skill_Start(static_cast<SKILL_TYPE>(skill));
			}
			else
				Cmd_Skill_Magery(spell, m_pChar);
			return;
		}

		case EXTCMD_OpenDoor:
		{
			CPointMap pt = m_pChar->GetTopPoint();
			signed char iCharZ = pt.m_z;

			pt.Move(m_pChar->m_dirFace);
			CWorldSearch Area(pt, 1);
			for (;;)
			{
				CItem *pItem = Area.GetItem();
				if ( !pItem )
					return;

				switch ( pItem->GetType() )
				{
					case IT_DOOR:
					case IT_DOOR_LOCKED:
					case IT_PORTCULIS:
					case IT_PORT_LOCKED:
						if ( abs(iCharZ - pItem->GetTopPoint().m_z) < 20 )
						{
							m_pChar->SysMessageDefault(DEFMSG_MACRO_OPENDOOR);
							m_pChar->Use_Obj(pItem, true);
							return;
						}
				}
			}
			return;
		}

		case EXTCMD_InvokeVirtue:
		{
			if ( !IsTrigUsed(TRIGGER_USERVIRTUEINVOKE) )
				return;

			int iVirtueID = ppArgs[0][0] - '0';		// 0x1=Honor, 0x2=Sacrifice, 0x3=Valor
			CScriptTriggerArgs Args(m_pChar);
			Args.m_iN1 = iVirtueID;
			m_pChar->OnTrigger(CTRIG_UserVirtueInvoke, m_pChar, &Args);
			return;
		}
	}
}

// ---------------------------------------------------------------------

bool CClient::xPacketFilter(const BYTE *pData, size_t iLen)
{
	ADDTOCALLSTACK("CClient::xPacketFilter");

	EXC_TRY("packet filter");
	if ( (iLen > 0) && g_Serv.m_PacketFilter[pData[0]][0] )
	{
		CScriptTriggerArgs Args(pData[0]);
		TCHAR idx[5];

		Args.m_s1 = GetPeerStr();
		Args.m_pO1 = this;		// yay for ARGO.SENDPACKET
		Args.m_VarsLocal.SetNum("CONNECTIONTYPE", GetConnectType());

		size_t iBytes = iLen;
		size_t iByteStr = minimum(iBytes, SCRIPT_MAX_LINE_LEN);
		TCHAR *zBuf = Str_GetTemp();

		Args.m_VarsLocal.SetNum("NUM", iBytes);
		memcpy(zBuf, &(pData[0]), iByteStr);
		zBuf[iByteStr] = 0;
		Args.m_VarsLocal.SetStr("STR", true, zBuf, true);
		if ( m_pAccount )
		{
			Args.m_VarsLocal.SetStr("ACCOUNT", false, m_pAccount->GetName());
			if ( m_pChar )
				Args.m_VarsLocal.SetNum("CHAR", m_pChar->GetUID());
		}

		// Fill locals [0..X] to the first X bytes of the packet
		for ( size_t i = 0; i < iBytes; ++i )
		{
			sprintf(idx, "%" FMTSIZE_T, i);
			Args.m_VarsLocal.SetNum(idx, static_cast<int>(pData[i]));
		}

		// Call the filtering function
		TRIGRET_TYPE tr;
		if ( g_Serv.r_Call(g_Serv.m_PacketFilter[pData[0]], &g_Serv, &Args, NULL, &tr) )
		{
			if ( tr == TRIGRET_RET_TRUE )
				return true;
		}
	}

	EXC_CATCH;
	return false;
}

bool CClient::xOutPacketFilter(const BYTE *pData, size_t iLen)
{
	ADDTOCALLSTACK("CClient::xOutPacketFilter");

	EXC_TRY("Outgoing packet filter");
	if ( (iLen > 0) && g_Serv.m_OutPacketFilter[pData[0]][0] )
	{
		CScriptTriggerArgs Args(pData[0]);
		TCHAR idx[5];

		Args.m_s1 = GetPeerStr();
		Args.m_pO1 = this;
		Args.m_VarsLocal.SetNum("CONNECTIONTYPE", GetConnectType());

		size_t iBytes = iLen;
		size_t iByteStr = minimum(iBytes, SCRIPT_MAX_LINE_LEN);
		TCHAR *zBuf = Str_GetTemp();

		Args.m_VarsLocal.SetNum("NUM", iBytes);
		memcpy(zBuf, &(pData[0]), iByteStr);
		zBuf[iByteStr] = 0;
		Args.m_VarsLocal.SetStr("STR", true, zBuf, true);
		if ( m_pAccount )
		{
			Args.m_VarsLocal.SetStr("ACCOUNT", false, m_pAccount->GetName());
			if ( m_pChar )
				Args.m_VarsLocal.SetNum("CHAR", m_pChar->GetUID());
		}

		// Fill locals [0..X] to the first X bytes of the packet
		for ( size_t i = 0; i < iBytes; ++i )
		{
			sprintf(idx, "%" FMTSIZE_T, i);
			Args.m_VarsLocal.SetNum(idx, static_cast<int>(pData[i]));
		}

		// Call the filtering function
		TRIGRET_TYPE tr;
		if ( g_Serv.r_Call(g_Serv.m_OutPacketFilter[pData[0]], &g_Serv, &Args, NULL, &tr) )
		{
			if ( tr == TRIGRET_RET_TRUE )
				return true;
		}
	}

	EXC_CATCH;
	return false;
}
