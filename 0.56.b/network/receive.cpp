#include "receive.h"
#include "send.h"
#include "network.h"
#include "../graysvr/CClient.h"

/***************************************************************************
 *
 *
 *	Packet ???? : PacketUnknown			unknown or unhandled packet
 *
 *
 ***************************************************************************/
PacketUnknown::PacketUnknown(long size) : Packet(size)
{
}

bool PacketUnknown::onReceive(NetState* net)
{
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x00 : PacketCreate		create new character request
 *
 *
 ***************************************************************************/
PacketCreate::PacketCreate(long size) : Packet(size)
{
}

bool PacketCreate::onReceive(NetState* net)
{
	DWORD pattern1 = readInt32();
	DWORD pattern2 = readInt32();
	BYTE kuoc = readByte();
	TCHAR charname[MAX_NAME_SIZE];
	readStringASCII(charname, MAX_NAME_SIZE);
	skip(2);
	DWORD flags = readInt32();
	skip(8);
	PROFESSION_TYPE prof = (PROFESSION_TYPE)readByte();
	skip(15);
	BYTE sex = readByte();
	BYTE strength = readByte();
	BYTE dexterity = readByte();
	BYTE intelligence = readByte();
	SKILL_TYPE skill1 = (SKILL_TYPE)readByte();
	BYTE skillval1 = readByte();
	SKILL_TYPE skill2 = (SKILL_TYPE)readByte();
	BYTE skillval2 = readByte();
	SKILL_TYPE skill3 = (SKILL_TYPE)readByte();
	BYTE skillval3 = readByte();
	HUE_TYPE hue = (HUE_TYPE)readInt16();
	ITEMID_TYPE hairid = (ITEMID_TYPE)readInt16();
	HUE_TYPE hairhue = (HUE_TYPE)readInt16();
	ITEMID_TYPE beardid = (ITEMID_TYPE)readInt16();
	HUE_TYPE beardhue = (HUE_TYPE)readInt16();
	skip(1);
	BYTE startloc = readByte();
	skip(3);
	BYTE slot = readByte();
	DWORD ip = readInt32();
	HUE_TYPE shirthue = (HUE_TYPE)readInt16();
	HUE_TYPE pantshue = (HUE_TYPE)readInt16();

	bool bFemale = (sex % 2); // Even=Male, Odd=Female (rule applies to all clients)
	RACE_TYPE rtRace = RACETYPE_HUMAN; // Human

	// determine which race the client has selected
	if (net->isClientVersion(MINCLIVER_SA) || net->isClientSA())
	{
		/*
			m_sex values from client 7.0.0.0+
			0x2 = Human, Male
			0x3 = Human, Female
			0x4 = Elf, Male
			0x5 = Elf, Female
			0x6 = Gargoyle, Male
			0x7 = Gargoyle, Female
		*/
		switch (sex)
		{
			case 0x0: case 0x1: case 0x2: case 0x3:
			default:
				rtRace = RACETYPE_HUMAN;
				break;
			case 0x4: case 0x5:
				rtRace = RACETYPE_ELF;
				break;
			case 0x6: case 0x7:
				rtRace = RACETYPE_GARGOYLE;
				break;
		}
	}
	else 
	{
		if ((sex - 2) >= 0)
			rtRace = RACETYPE_ELF;
	}

	// validate race against resdisp
	BYTE resdisp = net->getClient()->GetAccount() != NULL? net->getClient()->GetAccount()->GetResDisp() : RDS_T2A;
	if (resdisp < RDS_ML) // prior to ML, only human
	{
		if (rtRace >= RACETYPE_ELF)
			rtRace = RACETYPE_HUMAN;
	}
	else if (resdisp < RDS_SA) // prior to SA, only human and elf
	{
		if (rtRace >= RACETYPE_GARGOYLE)
			rtRace = RACETYPE_HUMAN;
	}
	
	return doCreate(net, charname, bFemale, rtRace,
		strength, dexterity, intelligence, prof,
		skill1, skillval1, skill2, skillval2, skill3, skillval3, (SKILL_TYPE)0, 0,
		hue, hairid, hairhue, beardid, beardhue, shirthue, pantshue,
		startloc, 0, flags);
}

bool PacketCreate::doCreate(NetState* net, LPCTSTR charname, bool bFemale, RACE_TYPE rtRace, short wStr, short wDex, short wInt, PROFESSION_TYPE prProf, SKILL_TYPE skSkill1, int iSkillVal1, SKILL_TYPE skSkill2, int iSkillVal2, SKILL_TYPE skSkill3, int iSkillVal3, SKILL_TYPE skSkill4, int iSkillVal4, HUE_TYPE wSkinHue, ITEMID_TYPE idHair, HUE_TYPE wHairHue, ITEMID_TYPE idBeard, HUE_TYPE wBeardHue, HUE_TYPE wShirtHue, HUE_TYPE wPantsHue, int iStartLoc, int iPortrait, int iFlags)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CAccountRef account = client->GetAccount();
	ASSERT(account);

	if (client->GetChar() != NULL)
	{
		// logging in as a new player whilst already online !
		client->addSysMessage(g_Cfg.GetDefaultMsg(DEFMSG_ALREADYONLINE));
		DEBUG_ERR(("%x:Setup_CreateDialog acct='%s' already online!\n", net->id(), account->GetName()));
		return false;
	}

	// make sure they don't have an idling character
	CChar* pCharLast = account->m_uidLastChar.CharFind();
	if (pCharLast != NULL && account->IsMyAccountChar(pCharLast) && account->GetPrivLevel() <= PLEVEL_GM && !pCharLast->IsDisconnected() )
	{
		client->addIdleWarning(PacketWarningMessage::CharacterInWorld);
		client->addLoginErr(PacketLoginError::CharIdle);
		return false;
	}

	// make sure they don't already have too many characters
	int iMaxChars = account->GetMaxChars();
	int iQtyChars = account->m_Chars.GetCharCount();
	if (iQtyChars >= iMaxChars)
	{
		client->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MAXCHARS), iQtyChars);
		if (client->GetPrivLevel() < PLEVEL_Seer)
		{
			client->addLoginErr(PacketLoginError::TooManyChars);
			return false;
		}
	}

	CChar* pChar = CChar::CreateBasic(CREID_MAN);
	ASSERT(pChar != NULL);

	pChar->InitPlayer(client, charname, bFemale, rtRace, wStr, wDex, wInt, prProf, skSkill1, iSkillVal1, skSkill2, iSkillVal2, skSkill3, iSkillVal3, wSkinHue, idHair, wHairHue, idBeard, wBeardHue, wShirtHue, wPantsHue, iStartLoc);

	g_Log.Event( LOGM_CLIENTS_LOG, "%x:Setup_CreateDialog acct='%s', char='%s'\n",
		net->id(), (LPCTSTR)account->GetName(), (LPCTSTR)pChar->GetName());

	TRIGRET_TYPE tr;
	CScriptTriggerArgs createArgs;
	createArgs.m_iN1 = iFlags;
	createArgs.m_iN2 = prProf;
	createArgs.m_iN3 = rtRace;
	createArgs.m_VarsLocal.SetNum("PORTRAIT", iPortrait);
	createArgs.m_VarsLocal.SetNum("EXTRASKILL.KEY", skSkill4);
	createArgs.m_VarsLocal.SetNum("EXTRASKILL.VAL", iSkillVal4 * 10);
	createArgs.m_s1 = account->GetName();
	createArgs.m_pO1 = client;

	client->r_Call("f_onchar_create", pChar, &createArgs, NULL, &tr);

	client->Setup_Start(pChar);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x02 : PacketMovementReq		movement request
 *
 *
 ***************************************************************************/
PacketMovementReq::PacketMovementReq(long size) : Packet(size)
{
}

bool PacketMovementReq::onReceive(NetState* net)
{
	BYTE direction = readByte();
	int sequence = readByte();
	DWORD crypt = readInt32(); // fast walk key

	doMovement(net, direction, sequence, crypt);
	return true;
}

void PacketMovementReq::doMovement(NetState* net, BYTE direction, int sequence, DWORD crypt)
{
	CClient* client = net->getClient();
	ASSERT(client);

	bool canMoveThere(true);

	// check crypt key
	if (!net->isClientLessVersion(0x126000))
		canMoveThere = client->Event_WalkingCheck(crypt);

	// check sequence
	if (net->m_sequence == 0 && sequence != 0)
		canMoveThere = false;

	// perform movement
	if (canMoveThere)
		canMoveThere = client->Event_Walking(direction);

	if (canMoveThere == false)
	{
		PacketMovementRej* packet = new PacketMovementRej(client, net->m_sequence);
		net->m_sequence = 0;
	}
	else
	{
		if (++sequence == 256)
			sequence = 1;

		// Ack the move. ( if this does not go back we get rubber banding )
		PacketMovementAck* packet = new PacketMovementAck(client);
		net->m_sequence = sequence;
	}
}


/***************************************************************************
 *
 *
 *	Packet 0x03 : PacketSpeakReq			character talking
 *
 *
 ***************************************************************************/
PacketSpeakReq::PacketSpeakReq() : Packet(-1)
{
}

bool PacketSpeakReq::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	if (client->GetChar() == NULL)
		return false;

	WORD packetLength = readInt16();
	TALKMODE_TYPE mode = (TALKMODE_TYPE)readByte();
	HUE_TYPE hue = (HUE_TYPE)readInt16();
	FONT_TYPE font = (FONT_TYPE)readInt16();
	TCHAR text[MAX_TALK_BUFFER];

	packetLength -= getPosition();
	readStringASCII(text, minimum(MAX_TALK_BUFFER, packetLength));

	client->Event_Talk(text, hue, mode, false);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x05 : PacketAttackReq		attack request
 *
 *
 ***************************************************************************/
PacketAttackReq::PacketAttackReq() : Packet(5)
{
}

bool PacketAttackReq::onReceive(NetState* net)
{
	CGrayUID target(readInt32());

	CClient* client = net->getClient();
	ASSERT(client);
	client->Event_Attack(target);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x06 : PacketDoubleClick		double click object
 *
 *
 ***************************************************************************/
PacketDoubleClick::PacketDoubleClick() : Packet(5)
{
}

bool PacketDoubleClick::onReceive(NetState* net)
{
	DWORD serial = readInt32();

	CGrayUID target(serial &~ UID_F_RESOURCE);
	bool macro = serial & UID_F_RESOURCE;

	CClient* client = net->getClient();
	ASSERT(client);
	client->Event_DoubleClick(target, macro, true);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x07 : PacketItemPickupReq	pick up item request
 *
 *
 ***************************************************************************/
PacketItemPickupReq::PacketItemPickupReq() : Packet(7)
{
}

bool PacketItemPickupReq::onReceive(NetState* net)
{
	CGrayUID serial(readInt32());
	int amount = readInt16();
	
	CClient* client = net->getClient();
	ASSERT(client);
	client->Event_Item_Pickup(serial, amount);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x08 : PacketItemDropReq		drop item request
 *
 *
 ***************************************************************************/
PacketItemDropReq::PacketItemDropReq() : Packet(14)
{
}

long PacketItemDropReq::getExpectedLength(NetState* net, Packet* packet)
{
	// different size depending on client
	if (net != NULL && (net->isClientVersion(0x0600018) || net->isClientKR()))
		return 15;

	return 14;
}

bool PacketItemDropReq::onReceive(NetState* net)
{
	CGrayUID serial(readInt32());
	WORD x = readInt16();
	WORD y = readInt16();
	BYTE z = readByte();

	BYTE grid(0);
	if (net->isClientVersion(0x0600018) || net->isClientKR())
		grid = readByte();

	CGrayUID container(readInt32());

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;
	
	CPointMap pt(x, y, z, character->GetTopMap());

	client->Event_Item_Drop(serial, pt, container, grid);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x09 : PacketSingleClick		single click object
 *
 *
 ***************************************************************************/
PacketSingleClick::PacketSingleClick() : Packet(5)
{
}

bool PacketSingleClick::onReceive(NetState* net)
{
	CGrayUID serial(readInt32());

	CClient* client = net->getClient();
	ASSERT(client);
	client->Event_SingleClick(serial);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x12 : PacketTextCommand					text command
 *
 *
 ***************************************************************************/
PacketTextCommand::PacketTextCommand() : Packet(-1)
{
}

bool PacketTextCommand::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	
	WORD packetLength = readInt16();
	if (packetLength < 5)
		return false;

	EXTCMD_TYPE type = (EXTCMD_TYPE)readByte();
	TCHAR name[MAX_TALK_BUFFER];
	readStringNullASCII(name, MAX_TALK_BUFFER-1);

	client->Event_ExtCmd(type, name);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x13 : PacketItemEquipReq	item equip request
 *
 *
 ***************************************************************************/
PacketItemEquipReq::PacketItemEquipReq() : Packet(10)
{
}

bool PacketItemEquipReq::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	CGrayUID itemSerial(readInt32());
	LAYER_TYPE itemLayer = (LAYER_TYPE)readByte();
	CGrayUID targetSerial(readInt32());

	CChar* source = client->GetChar();
	CItem* item = source->LayerFind(LAYER_DRAGGING);
	if (item == NULL ||
		client->GetTargMode() != CLIMODE_DRAG ||
		item->GetUID() != itemSerial)
	{
		// I have no idea why i got here.
		new PacketDragCancel(client, PacketDragCancel::Other);
		return true;
	}

	client->ClearTargMode(); // done dragging.

	CChar* target = targetSerial.CharFind();
	if ( target == NULL ||
		 itemLayer >= LAYER_HORSE ||
		!target->NPC_IsOwnedBy(source) || 
		!target->ItemEquip(item, source) )
	{
		source->ItemBounce(item); //cannot equip
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x22 : PacketResynchronize	resend all request
 *
 *
 ***************************************************************************/
PacketResynchronize::PacketResynchronize() : Packet(3)
{
}

bool PacketResynchronize::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	net->m_sequence = 0;
	client->addReSync();
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x2c : PacketDeathStatus		death status
 *
 *
 ***************************************************************************/
PacketDeathStatus::PacketDeathStatus() : Packet(2)
{
}

long PacketDeathStatus::getExpectedLength(NetState* net, Packet* packet)
{
	// different size depending on client
	int pos = packet->getPosition();
	packet->seek(1);
	DEATH_MODE_TYPE mode = (DEATH_MODE_TYPE)readByte();
	packet->seek(pos);

	if (mode != DEATH_MODE_MANIFEST)
		return 2;

	return 7;
}

bool PacketDeathStatus::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	CChar* ghost = client->GetChar();
	if (ghost == NULL)
		return false;

	DEATH_MODE_TYPE mode = (DEATH_MODE_TYPE)readByte();
	if (mode != DEATH_MODE_MANIFEST)
	{
		// Play as a ghost.
		client->SysMessage("You are a ghost");
		client->addSound(0x17f);
		client->addPlayerStart(ghost); // Do practically a full reset (to clear death menu)
		return true;
	}

	// toggle manifest mode (more data)
	skip(1); // unknown
	bool manifest = readBool();
	skip(3); // unknown

	client->Event_CombatMode(manifest);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x34 : PacketCharStatusReq	request information on the mobile
 *
 *
 ***************************************************************************/
PacketCharStatusReq::PacketCharStatusReq() : Packet(10)
{
}

bool PacketCharStatusReq::onReceive(NetState* net)
{
	skip(4); // 0xedededed
	BYTE requestType = readByte();
	CGrayUID targetSerial(readInt32());

	CClient* client = net->getClient();
	ASSERT(client);

	if ( requestType == 4 )
	{
		client->addCharStatWindow(targetSerial, true);
	}
	else if ( requestType == 5 )
	{
		client->addSkillWindow(SKILL_QTY);
	}
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x3A : PacketSkillLockChange	change skill locks
 *
 *
 ***************************************************************************/
PacketSkillLockChange::PacketSkillLockChange() : Packet(-1)
{
}

bool PacketSkillLockChange::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL || character->m_pPlayer == NULL)
		return false;

	int len = readInt16();
	len -= 3;
	if (len <= 0 || (len % 3) != 0)
		return false;

	while (len > 0)
	{
		// set next lock
		SKILL_TYPE index = (SKILL_TYPE)readInt16();
		SKILLLOCK_TYPE state = (SKILLLOCK_TYPE)readByte();

		character->m_pPlayer->Skill_SetLock(index, state);

		len -= 3;
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x3B : PacketVendorBuyReq	buy item from vendor
 *
 *
 ***************************************************************************/
PacketVendorBuyReq::PacketVendorBuyReq() : Packet(-1)
{
}

bool PacketVendorBuyReq::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* buyer = client->GetChar();
	if (buyer == NULL)
		return false;

	WORD packetLength = readInt16();
	CGrayUID vendorSerial(readInt32());
	BYTE flags = readByte();
	if (flags == 0)
		return true;

	CChar* vendor = vendorSerial.CharFind();
	if (vendor == NULL || vendor->m_pNPC == NULL || (!vendor->NPC_IsVendor() && vendor->m_pNPC->m_Brain != NPCBRAIN_VENDOR_OFFDUTY))
	{
		client->Event_VendorBuy_Cheater(0x1);
		return true;
	}

	if (buyer->CanTouch(vendor) == false)
	{
		client->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_CANTREACH));
		return true;
	}

	if (vendor->m_pNPC->m_Brain == NPCBRAIN_VENDOR_OFFDUTY) // cheaters
	{
		vendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_OFFDUTY));
		client->Event_VendorBuy_Cheater(0x2);
		return true;
	}

	int iConvertFactor = vendor->NPC_GetVendorMarkup(buyer);

	VendorItem items[MAX_ITEMS_CONT];
	memset(items, 0, sizeof(items));
	DWORD itemCount = minimum((packetLength - 8) / 7, MAX_ITEMS_CONT);

	// check buying speed
	CVarDefCont* vardef = g_Cfg.m_bAllowBuySellAgent ? NULL : client->m_TagDefs.GetKey("BUYSELLTIME");
	if (vardef != NULL)
	{
		CServTime allowsell;
		allowsell.InitTime(vardef->GetValNum() + (itemCount * 3));
		if (g_World.GetCurrentTime() < allowsell)
		{
			client->SysMessage("You are buying too fast.");
			return true;
		}
	}

	// combine goods into one list
	CItemVendable* item;
	for (int i = 0; i < itemCount; i++)
	{
		skip(1); // layer
		CGrayUID serial(readInt32());
		WORD amount = readInt16();

		item = dynamic_cast<CItemVendable*>(serial.ItemFind());
		if (item == NULL || item->IsValidSaleItem(true) == false)
		{
			client->Event_VendorBuy_Cheater(0x3);
			return true;
		}

		// search for it in the list
		int index;
		for (index = 0; index < itemCount; index++)
		{
			if (serial == items[index].m_serial)
				break;
			else if (items[index].m_serial.GetPrivateUID() == 0)
			{
				items[index].m_serial = serial;
				items[index].m_price = item->GetVendorPrice(iConvertFactor);
				break;
			}
		}

		items[index].m_amount += amount;
		if (items[index].m_price <= 0)
		{
			vendor->Speak("Alas, I don't have these goods currently stocked. Let me know if there is something else thou wouldst buy.");
			client->Event_VendorBuy_Cheater(0x4);
			return true;
		}
	}

	client->Event_VendorBuy(vendor, items, itemCount);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x56 : PacketMapEdit			edit map pins
 *
 *
 ***************************************************************************/
PacketMapEdit::PacketMapEdit() : Packet(11)
{
}

bool PacketMapEdit::onReceive(NetState* net)
{
	CGrayUID mapSerial(readInt32());
	MAPCMD_TYPE action = (MAPCMD_TYPE)readByte();
	BYTE pin = readByte();
	WORD x = readInt16();
	WORD y = readInt16();

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	CItemMap* map = dynamic_cast<CItemMap*>(mapSerial.ItemFind());
	if (map == NULL || character->CanTouch(map) == false) // sanity check
	{
		client->SysMessage("You can't reach it");
		return true;
	}

	if (map->m_itMap.m_fPinsGlued)
	{
		client->SysMessage("The pins seem to be glued in place");
		if (client->IsPriv(PRIV_GM) == false)
			return true;
	}

	// NOTE: while in edit mode, right click canceling of the
	// dialog sends the same msg as
	// request to edit in either mode...strange huh?
	switch (action)
	{
		case MAP_ADD: // add pin
		{
			if (map->m_Pins.GetCount() > CItemMap::MAX_PINS)
				return true;

			CMapPinRec mapPin(x, y);
			map->m_Pins.Add(mapPin);
		} break;

		case MAP_INSERT: // insert between 2 pins
		{
			if (map->m_Pins.GetCount() > CItemMap::MAX_PINS)
				return true;

			CMapPinRec mapPin(x, y);
			map->m_Pins.InsertAt(pin, mapPin);
		} break;

		case MAP_MOVE: // move pin
			if (pin >= map->m_Pins.GetCount())
			{
				client->SysMessage("That's strange... (bad pin)");
				return true;
			}
			map->m_Pins[pin].m_x = x;
			map->m_Pins[pin].m_y = y;
			break;

		case MAP_DELETE: // delete pin
			if (pin >= map->m_Pins.GetCount())
			{
				client->SysMessage("That's strange... (bad pin)");
				return true;
			}
			map->m_Pins.RemoveAt(pin);
			break;

		case MAP_CLEAR: // clear all pins
			map->m_Pins.RemoveAll();
			break;

		case MAP_TOGGLE: // edit req/cancel
			client->addMapMode(map, MAP_SENT, !map->m_fPlotMode);
			break;
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x5D : PacketCharPlay		character select
 *
 *
 ***************************************************************************/
PacketCharPlay::PacketCharPlay() : Packet(73)
{
}

bool PacketCharPlay::onReceive(NetState* net)
{
	skip(4); // 0xedededed
	skip(MAX_NAME_SIZE); // char name
	skip(MAX_NAME_SIZE); // char pass
	int slot = readInt32();
	skip(4); // ip

	CClient* client = net->getClient();
	ASSERT(client);

	BYTE err = client->Setup_Play(slot);

	client->addLoginErr(err);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x66 : PacketBookPageEdit	edit book page
 *
 *
 ***************************************************************************/
PacketBookPageEdit::PacketBookPageEdit() : Packet(-1)
{
}

bool PacketBookPageEdit::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	skip(2); // packet length
	CGrayUID bookSerial(readInt32());
	WORD pageCount = readInt16();

	CItem* book = bookSerial.ItemFind();
	if (character->CanSee(book) == false)
	{
		client->addObjectRemoveCantSee(bookSerial, "the book");
		return true;
	}

	WORD page = readInt16();
	WORD lineCount = readInt16();
	if (lineCount == 0xFFFF || getLength() <= 0x0D)
	{
		client->addBookPage(book, page); // just a request for a page
		return true;
	}

	// trying to write to the book
	CItemMessage* text = dynamic_cast<CItemMessage*>( book );
	if (text == NULL || book->IsBookWritable() == false)
		return true;

	skip(-4);

	long len = 0;
	TCHAR* content = Str_GetTemp();

	for (int i = 0; i < pageCount; i++)
	{
		// read next page to change with line count
		page = readInt16();
		lineCount = readInt16();
		if (page < 1 || page > MAX_BOOK_PAGES || lineCount <= 0)
			continue;

		page--;
		len = 0;

		// read each line of the page
		while (lineCount > 0)
		{
			len += readStringNullASCII(content + len, SCRIPT_MAX_LINE_LEN-1 - len);
			if (len >= SCRIPT_MAX_LINE_LEN)
			{
				len = SCRIPT_MAX_LINE_LEN - 1;
				break;
			}

			content[len++] = '\t';
			lineCount--;
		}

		content[--len] = '\0';
		if (Str_Check(content))
			break;

		// set page content
		text->SetPageText(page, content);
	}
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x6C : PacketTarget			target object
 *
 *
 ***************************************************************************/
PacketTarget::PacketTarget() : Packet(19)
{
}

bool PacketTarget::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	skip(1); // target type
	DWORD context = readInt32();
	BYTE flags = readByte();
	CGrayUID targetSerial(readInt32());
	WORD x = readInt16();
	WORD y = readInt16();
	skip(1);
	BYTE z = readByte();
	ITEMID_TYPE id = (ITEMID_TYPE)readInt16();

	client->Event_Target(context, targetSerial, CPointMap(x, y, z, character->GetTopMap()), flags, id);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x6F : PacketSecureTradeReq	trade with another character
 *
 *
 ***************************************************************************/
PacketSecureTradeReq::PacketSecureTradeReq() : Packet(-1)
{
}

bool PacketSecureTradeReq::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	skip(2); // length
	SECURE_TRADE_TYPE action = (SECURE_TRADE_TYPE)readByte();
	CGrayUID containerSerial(readInt32());
	DWORD arg = readInt32();

	CItemContainer* container = dynamic_cast<CItemContainer*>( containerSerial.ItemFind() );
	if (container == NULL)
		return true;
	else if (character != container->GetParent())
		return true;

	// perform the trade
	switch (action)
	{
		case SECURE_TRADE_CLOSE: // cancel trade. send each person cancel messages, move items
			container->Delete();
			return true;

		case SECURE_TRADE_CHANGE: // change check marks. possible conclude trade
			if (character->GetDist(container) > UO_MAP_VIEW_SIZE)
			{
				// to far away
				client->SysMessageDefault(DEFMSG_TRADE_TOOFAR);
				return true;
			}

			long need2wait(0);
			CVarDefCont* vardef = container->GetTagDefs()->GetKey("wait1sec");
			if (vardef) need2wait = vardef->GetValNum();
			if (need2wait > 0)
			{
				long timerow = g_World.GetCurrentTime().GetTimeRaw();
				if (need2wait > timerow)
				{
					TCHAR* msg = Str_GetTemp();
					long seconds = (need2wait-timerow) / TICK_PER_SEC;
					sprintf(msg, g_Cfg.GetDefaultMsg(DEFMSG_TRADE_WAIT), seconds);
					client->SysMessage(msg);
					return true;
				}
			}

			container->Trade_Status(arg);
			return true;
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x71 : PacketBulletinBoardReq	request bulletin board
 *
 *
 ***************************************************************************/
PacketBulletinBoardReq::PacketBulletinBoardReq() : Packet(-1)
{
}

bool PacketBulletinBoardReq::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	skip(2);
	BBOARDF_TYPE action = (BBOARDF_TYPE)readByte();
	CGrayUID boardSerial(readInt32());
	CGrayUID messageSerial(readInt32());

	CItemContainer* board = dynamic_cast<CItemContainer*>( boardSerial.ItemFind() );
	if (character->CanSee(board) == false)
	{
		client->addObjectRemoveCantSee(boardSerial, "the board");
		return true;
	}

	if (board->IsType(IT_BBOARD) == false)
		return true;

	switch (action)
	{
		case BBOARDF_REQ_FULL:
		case BBOARDF_REQ_HEAD:
			// request for message header and/or body
			if (getLength() != 0x0c)
			{
				DEBUG_ERR(( "%x:BBoard feed back message bad length %d\n", net->id(), getLength()));
				return true;
			}
			if (client->addBBoardMessage(board, action, messageSerial) == false)
			{
				// sanity check fails
				client->addObjectRemoveCantSee(messageSerial, "the message");
				return true;
			}
			break;

		case BBOARDF_NEW_MSG:
		{
			// submit a message
			if (getLength() < 0x0c)
				return true;

			if (character->CanTouch(board) == false)
			{
				character->SysMessageDefault(DEFMSG_ITEMUSE_BBOARD_REACH);
				return true;
			}

			if (board->GetCount() > 32)
			{
				// roll a message off
				delete board->GetAt(board->GetCount() - 1);
			}

			int lenstr = readByte();
			TCHAR* str = Str_GetTemp();
			readStringASCII(str, lenstr, false);
			if (Str_Check(str))
				return true;

			// if 
			CItemMessage* newMessage = dynamic_cast<CItemMessage*>( CItem::CreateBase(ITEMID_BBOARD_MSG) );
			if (newMessage == NULL)
			{
				DEBUG_ERR(("%x:BBoard can't create message item\n", net->id()));
				return true;
			}

			newMessage->SetAttr(ATTR_MOVE_NEVER);
			newMessage->SetName(str);
			newMessage->m_itBook.m_Time = CServTime::GetCurrentTime();
			newMessage->m_sAuthor = character->GetName();
			newMessage->m_uidLink = character->GetUID();

			int lines = readByte();
			if (lines > 32) lines = 32;

			while (lines--)
			{
				lenstr = readByte();
				readStringASCII(str, lenstr, false);
				if (Str_Check(str) == false)
					newMessage->AddPageText(str);
			}

			board->ContentAdd(newMessage);
			break;
		}

		case BBOARDF_DELETE:
		{
			// remove the message
			CItemMessage* message = dynamic_cast<CItemMessage*>( messageSerial.ItemFind() );
			if (board->IsItemInside(message) == false)
			{
				client->SysMessageDefault(DEFMSG_ITEMUSE_BBOARD_COR);
				return true;
			}

			if (client->IsPriv(PRIV_GM) == false && message->m_uidLink != character->GetUID())
			{
				client->SysMessageDefault(DEFMSG_ITEMUSE_BBOARD_DEL);
				return true;
			}

			message->Delete();
			break;
		}

		default:
			DEBUG_ERR(( "%x:BBoard unknown flag %d\n", net->id(), action));
			return true;
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x72 : PacketWarModeReq		toggle war mode
 *
 *
 ***************************************************************************/
PacketWarModeReq::PacketWarModeReq() : Packet(5)
{
}

bool PacketWarModeReq::onReceive(NetState* net)
{
	bool war = readBool();
	skip(3); // unknown
	net->getClient()->Event_CombatMode(war);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x73 : PacketPingReq			ping requests
 *
 *
 ***************************************************************************/
PacketPingReq::PacketPingReq() : Packet(2)
{
}

bool PacketPingReq::onReceive(NetState* net)
{
	BYTE value = readByte();
	PacketPingAck* packet = new PacketPingAck(net->getClient(), value);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x75 : PacketCharRename		rename character/pet
 *
 *
 ***************************************************************************/
PacketCharRename::PacketCharRename() : Packet(35)
{
}

bool PacketCharRename::onReceive(NetState* net)
{
	CGrayUID serial(readInt32());
	TCHAR* name = Str_GetTemp();
	readStringASCII(name, MAX_NAME_SIZE);

	net->getClient()->Event_SetName(serial, name);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x7D : PacketMenuChoice		select menu option
 *
 *
 ***************************************************************************/
PacketMenuChoice::PacketMenuChoice() : Packet(13)
{
}

bool PacketMenuChoice::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	CGrayUID serial(readInt32());
	WORD context = readInt16();
	WORD select = readInt16();

	if (context != client->GetTargMode() || serial != client->m_tmMenu.m_UID)
	{
		client->SysMessage("Unexpected menu info");
		return true;
	}

	client->ClearTargMode();

	switch (context)
	{
		case CLIMODE_MENU:
			// a generic menu from script
			client->Menu_OnSelect(client->m_tmMenu.m_ResourceID, select, serial.ObjFind());
			return true;

		case CLIMODE_MENU_SKILL:
			// some skill menu got us here
			if (select >= COUNTOF(client->m_tmMenu.m_Item))
				return true;

			client->Cmd_Skill_Menu(client->m_tmMenu.m_ResourceID, (select) ? client->m_tmMenu.m_Item[select] : 0 );
			return true;

		case CLIMODE_MENU_SKILL_TRACK_SETUP:
			// pretracking menu got us here
			client->Cmd_Skill_Tracking(select, false);
			return true;

		case CLIMODE_MENU_SKILL_TRACK:
			// tracking menu got us here. start tracking the selected creature
			client->Cmd_Skill_Tracking(select, true);
			return true;

		case CLIMODE_MENU_GM_PAGES:
			// select a gm page from the menu
			client->Cmd_GM_PageSelect(select);
			return true;

		case CLIMODE_MENU_EDIT:
			// m_Targ_Text = what are we doing to it
			client->Cmd_EditItem(serial.ObjFind(), select);
			return true;

		default:
			DEBUG_ERR(("%x:Unknown Targetting mode for menu %d\n", net->id(), context));
			return true;
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x80 : PacketServersReq		request server list
 *
 *
 ***************************************************************************/
PacketServersReq::PacketServersReq() : Packet(62)
{
}

bool PacketServersReq::onReceive(NetState* net)
{
	TCHAR acctname[MAX_ACCOUNT_NAME_SIZE];
	readStringASCII(acctname, COUNTOF(acctname));
	TCHAR acctpass[MAX_NAME_SIZE];
	readStringASCII(acctpass, COUNTOF(acctpass));
	skip(1);

	CClient* client = net->getClient();
	ASSERT(client);

	BYTE lErr = client->Login_ServerList(acctname, acctpass);
	client->addLoginErr(lErr);
	return true;
}

/***************************************************************************
 *
 *
 *	Packet 0x83 : PacketCharDelete		delete character
 *
 *
 ***************************************************************************/
PacketCharDelete::PacketCharDelete() : Packet(39)
{
}

bool PacketCharDelete::onReceive(NetState* net)
{
	skip(MAX_NAME_SIZE); // charpass
	int slot = readInt32();
	skip(4); // client ip

	CClient* client = net->getClient();
	ASSERT(client);

	BYTE err = client->Setup_Delete(slot);
	client->addDeleteErr(err);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x8D : PacketCreateNew		create new character request (KR)
 *
 *
 ***************************************************************************/
PacketCreateNew::PacketCreateNew(void) : PacketCreate(-1)
{
}

bool PacketCreateNew::onReceive(NetState* net)
{
	skip(2);
	DWORD pattern1 = readInt32();
	DWORD pattern2 = readInt32();
	TCHAR charname[MAX_NAME_SIZE];
	readStringASCII(charname, MAX_NAME_SIZE);
	skip(30);
	BYTE profession = readByte();
	skip(1);
	BYTE sex = readByte();
	BYTE race = readByte();
	BYTE strength = readByte();
	BYTE dexterity = readByte();
	BYTE intelligence = readByte();
	HUE_TYPE hue = (HUE_TYPE)readInt16();
	skip(8);
	SKILL_TYPE skill1 = (SKILL_TYPE)readByte();
	BYTE skillval1 = readByte();
	SKILL_TYPE skill2 = (SKILL_TYPE)readByte();
	BYTE skillval2 = readByte();
	SKILL_TYPE skill4 = (SKILL_TYPE)readByte();
	BYTE skillval4 = readByte();
	SKILL_TYPE skill3 = (SKILL_TYPE)readByte();
	BYTE skillval3 = readByte();
	skip(26);
	HUE_TYPE hairhue = (HUE_TYPE)readInt16();
	ITEMID_TYPE hairid = (ITEMID_TYPE)readInt16();
	skip(11);
	HUE_TYPE hue2 = (HUE_TYPE)readInt16();
	skip(1);
	BYTE portrait = readByte();
	skip(1);
	HUE_TYPE beardhue = (HUE_TYPE)readInt16();
	ITEMID_TYPE beardid = (ITEMID_TYPE)readInt16();
	
	// The new creation packet does not contain skills and values if
	// a profession is selected, so here we must translate the selected
	// profession -> skills
	switch (profession)
	{
		case PROFESSION_WARRIOR:
			skill1 = SKILL_SWORDSMANSHIP;
			skillval1 = 30;
			skill2 = SKILL_TACTICS;
			skillval2 = 30;
			skill3 = SKILL_HEALING;
			skillval3 = 30;
			skill4 = SKILL_ANATOMY;
			skillval4 = 30;
			break;
		case PROFESSION_MAGE:
			skill1 = SKILL_MAGERY;
			skillval1 = 30;
			skill2 = SKILL_EVALINT;
			skillval2 = 30;
			skill3 = SKILL_MEDITATION;
			skillval3 = 30;
			skill4 = SKILL_WRESTLING;
			skillval4 = 30;
			break;
		case PROFESSION_BLACKSMITH:
			skill1 = SKILL_BLACKSMITHING;
			skillval1 = 30;
			skill2 = SKILL_MINING;
			skillval2 = 30;
			skill3 = SKILL_TINKERING;
			skillval3 = 30;
			skill4 = SKILL_TAILORING;
			skillval4 = 30;
			break;
		case PROFESSION_NECROMANCER:
			skill1 = SKILL_NECROMANCY;
			skillval1 = 30;
			skill2 = SKILL_SPIRITSPEAK;
			skillval2 = 30;
			skill3 = SKILL_FENCING;
			skillval3 = 30;
			skill4 = SKILL_MEDITATION;
			skillval4 = 30;
			break;
		case PROFESSION_PALADIN:
			skill1 = SKILL_CHIVALRY;
			skillval1 = 30;
			skill2 = SKILL_SWORDSMANSHIP;
			skillval2 = 30;
			skill3 = SKILL_TACTICS;
			skillval3 = 30;
			skill4 = SKILL_FOCUS;
			skillval4 = 30;
			break;
		case PROFESSION_SAMURAI:
			skill1 = SKILL_BUSHIDO;
			skillval1 = 30;
			skill2 = SKILL_SWORDSMANSHIP;
			skillval2 = 30;
			skill3 = SKILL_FOCUS;
			skillval3 = 30;
			skill4 = SKILL_PARRYING;
			skillval4 = 30;
			break;
		case PROFESSION_NINJA:
			skill1 = SKILL_NINJITSU;
			skillval1 = 30;
			skill2 = SKILL_FENCING;
			skillval2 = 30;
			skill3 = SKILL_HIDING;
			skillval3 = 30;
			skill4 = SKILL_PARRYING;
			skillval4 = 30;
			break;
		case PROFESSION_ADVANCED:
			break;
		default:
			DEBUG_WARN(("Unknown profession '%d' selected.\n", profession));
			break;
	}

	return doCreate(net, charname, sex > 0, (RACE_TYPE)race,
		strength, dexterity, intelligence, (PROFESSION_TYPE)profession,
		skill1, skillval1, skill2, skillval2, skill3, skillval3, skill4, skillval4,
		hue, hairid, hairhue, beardid, beardhue, HUE_DEFAULT, HUE_DEFAULT,
		0, portrait, 0xFFFFFFFF);
}


/***************************************************************************
 *
 *
 *	Packet 0x91 : PacketCharListReq		request character list
 *
 *
 ***************************************************************************/
PacketCharListReq::PacketCharListReq() : Packet(65)
{
}

bool PacketCharListReq::onReceive(NetState* net)
{
	skip(4);
	TCHAR acctname[MAX_ACCOUNT_NAME_SIZE];
	readStringASCII(acctname, COUNTOF(acctname));
	TCHAR acctpass[MAX_NAME_SIZE];
	readStringASCII(acctpass, COUNTOF(acctpass));

	net->getClient()->Setup_ListReq(acctname, acctpass, false);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x93 : PacketBookHeaderEdit	edit book header (title/author)
 *
 *
 ***************************************************************************/
PacketBookHeaderEdit::PacketBookHeaderEdit() : Packet(99)
{
}

bool PacketBookHeaderEdit::onReceive(NetState* net)
{
	CGrayUID bookSerial(readInt32());
	skip(1); // writable
	skip(1); // unknown
	skip(2); // pages

	TCHAR title[2 * MAX_NAME_SIZE];
	readStringASCII(title, COUNTOF(title));

	TCHAR author[MAX_NAME_SIZE];
	readStringASCII(author, COUNTOF(author));

	net->getClient()->Event_Book_Title(bookSerial, title, author);
	return true;
}

/***************************************************************************
 *
 *
 *	Packet 0x95 : PacketDyeObject		colour selection dialog
 *
 *
 ***************************************************************************/
PacketDyeObject::PacketDyeObject() : Packet(9)
{
}

bool PacketDyeObject::onReceive(NetState* net)
{
	CGrayUID serial(readInt32());
	skip(2); // item id
	HUE_TYPE hue = (HUE_TYPE)readInt16();

	net->getClient()->Event_Item_Dye(serial, hue);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x98 : PacketAllNamesReq					all names command (ctrl+shift)
 *
 *
 ***************************************************************************/
PacketAllNamesReq::PacketAllNamesReq() : Packet(-1)
{
}

bool PacketAllNamesReq::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	CObjBase* object;
	PacketAllNamesResponse* cmd;

	for (WORD length = readInt16(); length > sizeof(DWORD); length -= sizeof(DWORD))
	{
		object = CGrayUID(readInt32()).ObjFind();
		if (object == NULL)
			continue;
		else if (character->CanSee(object) == false)
			continue;

		cmd = new PacketAllNamesResponse(client, object);
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x9A : PacketPromptResponse	prompt response (ascii)
 *
 *
 ***************************************************************************/
PacketPromptResponse::PacketPromptResponse() : Packet(-1)
{
}

bool PacketPromptResponse::onReceive(NetState* net)
{
	WORD length = readInt16();
	DWORD context1 = readInt32();
	DWORD context2 = readInt32();
	DWORD type = readInt32();
	TCHAR* text = Str_GetTemp();
	length -= getPosition();
	readStringASCII(text, length, false);

	net->getClient()->Event_PromptResp(text, length, context1, context2, type);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x9B : PacketHelpPageReq		GM help page request
 *
 *
 ***************************************************************************/
PacketHelpPageReq::PacketHelpPageReq() : Packet(258)
{
}

bool PacketHelpPageReq::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	CScript script("HelpPage");
	character->r_Verb(script, client);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x9F : PacketVendorSellReq	sell item to vendor
 *
 *
 ***************************************************************************/
PacketVendorSellReq::PacketVendorSellReq() : Packet(-1)
{
}

bool PacketVendorSellReq::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* seller = client->GetChar();
	if (seller == NULL)
		return false;

	WORD packetLength = readInt16();
	CGrayUID vendorSerial(readInt32());
	WORD itemCount = readInt16();

	CChar* vendor = vendorSerial.CharFind();
	if (vendor == NULL || vendor->m_pNPC == NULL || (!vendor->NPC_IsVendor() && vendor->m_pNPC->m_Brain != NPCBRAIN_VENDOR_OFFDUTY))
	{
		client->Event_VendorBuy_Cheater(0x1);
		return true;
	}
	
	if (seller->CanTouch(vendor) == false)
	{
		client->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_CANTREACH));
		return true;
	}

	if (vendor->m_pNPC->m_Brain == NPCBRAIN_VENDOR_OFFDUTY) // cheaters
	{
		vendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_OFFDUTY));
		client->Event_VendorBuy_Cheater(0x2);
		return true;
	}

	if (itemCount < 1)
	{
		client->addVendorClose(vendor);
		return true;
	}
	else if (itemCount >= MAX_ITEMS_CONT)
	{
		client->SysMessage("You cannot sell so much.");
		return true;
	}

	// check selling speed
	CVarDefCont* vardef = g_Cfg.m_bAllowBuySellAgent ? NULL : client->m_TagDefs.GetKey("BUYSELLTIME");
	if (vardef != NULL)
	{
		CServTime allowsell;
		allowsell.InitTime(vardef->GetValNum() + (itemCount * 3));
		if (g_World.GetCurrentTime() < allowsell)
		{
			client->SysMessage("You are selling too fast.");
			return true;
		}
	}

	VendorItem items[MAX_ITEMS_CONT];
	memset(items, 0, sizeof(items));

	for (int i = 0; i < itemCount; i++)
	{
		items[i].m_serial = CGrayUID(readInt32());
		items[i].m_amount = readInt16();
	}

	client->Event_VendorSell(vendor, items, itemCount);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xA0 : PacketServerSelect	play server
 *
 *
 ***************************************************************************/
PacketServerSelect::PacketServerSelect() : Packet(3)
{
}

bool PacketServerSelect::onReceive(NetState* net)
{
	int server = readInt16();

	net->getClient()->Login_Relay(server);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xA4 : PacketSystemInfo		system info from client
 *
 *
 ***************************************************************************/
PacketSystemInfo::PacketSystemInfo() : Packet(149)
{
}

bool PacketSystemInfo::onReceive(NetState* net)
{
	skip(148);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xA7 : PacketTipReq			tip request
 *
 *
 ***************************************************************************/
PacketTipReq::PacketTipReq() : Packet(4)
{
}

bool PacketTipReq::onReceive(NetState* net)
{
	WORD index = readInt16() + 1;
	skip(1); // font type

	CClient* client = net->getClient();
	ASSERT(client);
	client->Event_Tips(index);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xAC : PacketGumpValueInputResponse	gump text input
 *
 *
 ***************************************************************************/
PacketGumpValueInputResponse::PacketGumpValueInputResponse() : Packet(-1)
{
}

bool PacketGumpValueInputResponse::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	WORD packetLength = readInt16();
	CGrayUID uid(readInt32());
	WORD context = readInt16();
	BYTE action = readByte();
	WORD textLength = readInt16();
	TCHAR text[MAX_NAME_SIZE];
	readStringASCII(text, minimum(MAX_NAME_SIZE, textLength));

	TCHAR* fix;
	if ((fix = strchr(text, '\n')) != NULL)
		*fix = '\0';
	if ((fix = strchr(text, '\r')) != NULL)
		*fix = '\0';
	if ((fix = strchr(text, '\t')) != NULL)
		*fix = ' ';

	if (client->GetTargMode() != CLIMODE_INPVAL || uid != client->m_Targ_UID)
	{
		client->SysMessage("Unexpected text input");
		return true;
	}

	client->ClearTargMode();

	CObjBase* object = uid.ObjFind();
	if (object == NULL)
		return true;

	// take action based on the parent context
	if (action == 1) // ok
	{
		// Properties Dialog, page x
		// m_Targ_Text = the verb we are dealing with
		// m_Prop_UID = object we are after

		CScript script(client->m_Targ_Text, text);
		bool ret = object->r_Verb(script, client->GetChar());
		if (ret == false)
		{
			client->SysMessagef("Invalid set: %s = %s", (LPCTSTR)client->m_Targ_Text, (LPCTSTR)text);
		}
		else
		{
			if (client->IsPriv(PRIV_DETAIL))
			{
				client->SysMessagef("Set: %s = %s", (LPCTSTR)client->m_Targ_Text, (LPCTSTR)text);
			}

			object->RemoveFromView(); // weird client thing
			object->Update();
		}

		g_Log.Event( LOGM_GM_CMDS, "%x:'%s' tweak uid=0%x (%s) to '%s %s'=%d\n",
			net->id(), (LPCTSTR)client->GetName(),
			(DWORD)object->GetUID(), (LPCTSTR)object->GetName(),
			(LPCTSTR)client->m_Targ_Text, (LPCTSTR)text, ret);
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xAD : PacketSpeakReqUNICODE				character talking (unicode)
 *
 *
 ***************************************************************************/
PacketSpeakReqUNICODE::PacketSpeakReqUNICODE() : Packet(-1)
{
}

bool PacketSpeakReqUNICODE::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	if (client->GetChar() == NULL)
		return false;

	WORD packetLength = readInt16();
	TALKMODE_TYPE mode = (TALKMODE_TYPE)readByte();
	HUE_TYPE hue = (HUE_TYPE)readInt16();
	FONT_TYPE font = (FONT_TYPE)readInt16();
	TCHAR language[4];
	readStringASCII(language, COUNTOF(language));

	packetLength = (packetLength - getPosition()) / 2;
	if (packetLength >= MAX_TALK_BUFFER)
		packetLength = MAX_TALK_BUFFER - 1;

	if (mode & 0xc0) // text contains keywords
	{
		mode = (TALKMODE_TYPE)(mode & ~0xc0);

		int count = (readInt16() & 0xFFF0) >> 4;
		if (count < 0 || count > 50) // malformed check
			return true;

		skip(-2);
		count = (count + 1) * 12;
		int toskip = count / 8;
		if ((count % 8) > 0)
			toskip++;

		if (toskip > (packetLength * 2))
			return true;

		skip(toskip);
		TCHAR text[MAX_TALK_BUFFER];
		readStringNullASCII(text, COUNTOF(text));
		client->Event_Talk(text, hue, mode, true);
	}
	else
	{
		NCHAR text[MAX_TALK_BUFFER];
		readStringUNICODE((WCHAR*)text, packetLength, false);
		client->Event_TalkUNICODE(text, packetLength, hue, mode, font, language);
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xB1 : PacketGumpDialogRet				dialog button pressed
 *
 *
 ***************************************************************************/
PacketGumpDialogRet::PacketGumpDialogRet() : Packet(-1)
{
}

bool PacketGumpDialogRet::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	WORD packetLength = readInt16();
	CGrayUID serial(readInt32());
	DWORD context = readInt32();
	DWORD button = readInt32();
	DWORD checkCount = readInt32();

	// relying on the context given by the gump might be a security problem, much like
	// relying on the uid returned.
	// maybe keep a memory for each gump?
	CObjBase* object = serial.ObjFind();

	// virtue button -- Handling this here because the packet is a little different
	if ((context == CLIMODE_DIALOG_VIRTUE) && (character == object))
	{
		if ( !IsSetEF(EF_Minimize_Triggers))
		{
			CChar* viewed = character;
			if (button == 1 && checkCount > 0)
			{
				viewed = CGrayUID(readInt32()).CharFind();
				if (viewed == NULL)
					viewed = character;
			}

			CScriptTriggerArgs Args(viewed);
			Args.m_iN1 = button;

			character->OnTrigger(CTRIG_UserVirtue, (CTextConsole*)character, &Args);
		}

		return true;
	}

#ifdef _DEBUG
	{
		CResourceDef* resource = g_Cfg.ResourceGetDef(RESOURCE_ID(RES_DIALOG, context));
		if (resource == NULL)
			g_Log.Event(LOGL_EVENT, "Gump: %d (%s), Uid: 0x%x, Button: %d.\n", context, "undef", (DWORD)serial, button);
		else
		{
			CDialogDef* dialog = dynamic_cast<CDialogDef*>(resource);
			if (dialog == NULL)
				g_Log.Event(LOGL_EVENT, "Gump: %d (%s), Uid: 0x%x, Button: %d.\n", context, "undef", (DWORD)serial, button);
			else
				g_Log.Event(LOGL_EVENT, "Gump: %d (%s), Uid: 0x%x, Button: %d.\n", context, (LPCTSTR)dialog->GetName(), (DWORD)serial, button);
		}
	}
#endif

	// sanity check
	CClient::OpenedGumpsMap_t::iterator itGumpFound = client->m_mapOpenedGumps.find(((int)(context)));
	if (itGumpFound == client->m_mapOpenedGumps.end() || (*itGumpFound).second <= 0)
		return true;
	(*itGumpFound).second--;

	// package up the gump response info.
	CDialogResponseArgs resp;
	int i;

	// store the returned checked boxes' ids for possible later use
	for (i = 0; i < checkCount; i++)
		resp.m_CheckArray.Add(readInt32());


	DWORD textCount = readInt32();
	TCHAR* text = Str_GetTemp();
	for (i = 0; i < textCount; i++)
	{
		WORD id = readInt16();
		WORD length = readInt16();
		readStringNUNICODE(text, length, false);

		TCHAR* fix;
		if ((fix = strchr(text, '\n')) != NULL)
			*fix = '\0';
		if ((fix = strchr(text, '\r')) != NULL)
			*fix = '\0';
		if ((fix = strchr(text, '\t')) != NULL)
			*fix = ' ';

		resp.AddText(id, text);
	}

#ifndef _NEWGUILDSYSTEM
	switch ( context ) // This is the page number
	{
		case CLIMODE_DIALOG_GUILD: // Guild/Leige/Townstones stuff comes here
		{
			CItemStone* stone = dynamic_cast<CItemStone*>( client->m_Targ_UID.ItemFind() );
			if (stone == NULL || stone->OnDialogButton(client, (STONEDISP_TYPE)button, resp))
				return true;
		} break;

		default:
			break;
	}
#endif

	if (net->isClientKR())
		context = g_Cfg.GetKRDialogMap(context);

	RESOURCE_ID_BASE	rid	= RESOURCE_ID(RES_DIALOG, context);
	//
	// Call the scripted response. Lose all the checks and text.
	//
	client->Dialog_OnButton( rid, button, object, &resp );
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xB3 : PacketChatCommand					chat command
 *
 *
 ***************************************************************************/
PacketChatCommand::PacketChatCommand() : Packet(-1)
{
}

bool PacketChatCommand::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	WORD packetLength = readInt16();
	TCHAR language[4];
	readStringASCII(language, COUNTOF(language));
	NCHAR text[MAX_TALK_BUFFER];
	int textLength = packetLength - getPosition();
	if (textLength >= MAX_TALK_BUFFER)
		textLength = MAX_TALK_BUFFER-1;

	readStringUNICODE((WCHAR*)text, textLength, false);

	client->Event_ChatText(text, textLength, CLanguageID(language));
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xB5 : PacketChatButton					chat button pressed
 *
 *
 ***************************************************************************/
PacketChatButton::PacketChatButton() : Packet(64)
{
}

bool PacketChatButton::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	skip(1); // 0x00
	NCHAR name[MAX_NAME_SIZE+1];
	readStringUNICODE((WCHAR*)name, COUNTOF(name));

	client->Event_ChatButton(name);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xB6 : PacketToolTipReq					tooltip requested
 *
 *
 ***************************************************************************/
PacketToolTipReq::PacketToolTipReq() : Packet(9)
{
}

bool PacketToolTipReq::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	CGrayUID serial(readInt32());
	client->Event_ToolTip(serial);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xB8 : PacketProfileReq					character profile requested
 *
 *
 ***************************************************************************/
PacketProfileReq::PacketProfileReq() : Packet(-1)
{
}

bool PacketProfileReq::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	WORD packetLength = readInt16();
	bool write = readBool();
	CGrayUID serial(readInt32());
	WORD textLength(0);
	TCHAR* text(NULL);

	if (write == true && packetLength > 12)
	{
		skip(1); // unknown
		skip(1); // unknown-return code?

		textLength = readInt16();
		text = Str_GetTemp();
		readStringNUNICODE(text, textLength+1, false);
	}

	client->Event_Profile(write, serial, text, textLength);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBB : PacketMailMessage					send mail message
 *
 *
 ***************************************************************************/
PacketMailMessage::PacketMailMessage() : Packet(9)
{
}

bool PacketMailMessage::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	CGrayUID serial1(readInt32());
	CGrayUID serial2(readInt32());

	client->Event_MailMsg(serial1, serial2);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBD : PacketClientVersion				client version
 *
 *
 ***************************************************************************/
PacketClientVersion::PacketClientVersion() : Packet(-1)
{
}

bool PacketClientVersion::onReceive(NetState* net)
{
	if (net->getReportedVersion() > 0)
		return true;

	WORD length = readInt16();
	length -= getPosition();
	if (length > 20)
		length = 20;

	TCHAR* versionStr = Str_GetTemp();
	readStringASCII(versionStr, length, false);

	if (Str_Check(versionStr))
		return true;

	if (strstr(versionStr, "UO:3D") != NULL)
		net->m_clientType = CLIENTTYPE_3D;

	length = Str_GetBare(versionStr, versionStr, length, " '`-+!\"#$%&()*,/:;<=>?@[\\]^{|}~");
	if (length > 0)
	{
		CClient* client = net->getClient();
		ASSERT(client);

		DWORD version = CCrypt::GetVerFromString(versionStr);
		net->m_reportedVersion = version;
		net->setAsyncMode();

		DEBUG_MSG(("Getting cliver 0x%x/0x%x\n", version, (version&0xFFFFF0)));
		
		if (g_Serv.m_ClientVersion.GetClientVer() && ((version&0xFFFFF0) != g_Serv.m_ClientVersion.GetClientVer()))
		{
			client->addLoginErr(PacketLoginError::BadVersion);
		}
		else if ((net->getCryptVersion() < 400000) && (version >= 0x400000) && (client->GetResDisp() >= RDS_AOS) && (IsAosFlagEnabled(FEATURE_AOS_UPDATE_B))) //workaround for a "bug", which sends all items in LOS before processing this packet
		{
			DEBUG_MSG(("m_Crypt.GetClientVer()(%x) != m_reportedCliver(%x) == %x\n", net->getCryptVersion(), version, (net->getCryptVersion() != version)));
			client->addAOSPlayerSeeNoCrypt();
		}
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF : PacketExtendedCommand				extended command
 *
 *
 ***************************************************************************/
PacketExtendedCommand::PacketExtendedCommand() : Packet(-1)
{
}

bool PacketExtendedCommand::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	if (client->GetChar() == NULL)
		return false;

	WORD packetLength = readInt16();
	EXTDATA_TYPE type = (EXTDATA_TYPE)readInt16();
	seek();

	Packet* handler = g_NetworkIn.getExtendedHandler(type);
	if (handler == NULL)
		return false;

	handler->seek();
	for (int i = 0; i < packetLength; i++)
	{
		BYTE next = readByte();
		handler->writeByte(next);
	}

	handler->resize(packetLength);
	handler->seek(5);
	return handler->onReceive(net);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x05 : PacketScreenSize				screen size report
 *
 *
 ***************************************************************************/
PacketScreenSize::PacketScreenSize() : Packet(-1)
{
}

bool PacketScreenSize::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	DWORD x = readInt32();
	DWORD y = readInt32();
	
	DEBUG_MSG(("0x%x - 0x%x (%d-%d)\n", x, y, x, y));

	client->SetScreenSize(x, y);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x06 : PacketPartyMessage			party message
 *
 *
 ***************************************************************************/
PacketPartyMessage::PacketPartyMessage() : Packet(-1)
{
}

bool PacketPartyMessage::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	PARTYMSG_TYPE code = (PARTYMSG_TYPE)readByte();
	switch (code)
	{
		case PARTYMSG_Add:
			// request to add a new member
			client->addTarget( CLIMODE_TARG_PARTY_ADD, g_Cfg.GetDefaultMsg(DEFMSG_PARTY_TARG_WHO), false, false);
			break;

		case PARTYMSG_Disband:
			if (character->m_pParty == NULL)
				return false;

			character->m_pParty->Disband(character->GetUID());
			break;

		case PARTYMSG_Remove:
		{
			// request to remove this member of the party
			if (character->m_pParty == NULL)
				return false;

			CGrayUID serial(readInt32());
			character->m_pParty->RemoveMember(serial, character->GetUID());
		} break;

		case PARTYMSG_MsgMember:
		{
			// message a specific member of my party
			if (character->m_pParty == NULL)
				return false;

			CGrayUID serial(readInt32());
			NWORD* text = (NWORD*)Str_GetTemp();
			int length = readStringNullUNICODE((WCHAR*)text, MAX_TALK_BUFFER);
			character->m_pParty->MessageEvent(serial, character->GetUID(), text, length);
		} break;

		case PARTYMSG_Msg:
		{
			// send message to the whole party
			if (character->m_pParty == NULL)
				return false;

			NWORD* text = (NWORD*)Str_GetTemp();
			int length = readStringNullUNICODE((WCHAR*)text, MAX_TALK_BUFFER);
			character->m_pParty->MessageEvent(CGrayUID(0), character->GetUID(), text, length);
		} break;

		case PARTYMSG_Option:
			// set the loot flag
			if (character->m_pParty == NULL)
				return false;

			character->m_pParty->SetLootFlag(character, readBool());
			break;

		case PARTYMSG_Accept:
		{
			// we accept or decline the offer of an invite
			CGrayUID serial(readInt32());
			CPartyDef::AcceptEvent(character, serial);
		} break;

		case PARTYMSG_Decline:
		{
			CGrayUID serial(readInt32());
			CPartyDef::DeclineEvent(character, serial);
		} break;

		default:
			client->SysMessagef("Unknown party system msg %d", code);
			break;
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x07 : PacketArrowClick				click quest arrow
 *
 *
 ***************************************************************************/
PacketArrowClick::PacketArrowClick() : Packet(-1)
{
}

bool PacketArrowClick::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	bool rightClick = readBool();

	client->SysMessageDefault(DEFMSG_FOLLOW_ARROW);

	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = (rightClick == true? 1 : 0);
#ifdef _ALPHASPHERE
		Args.m_iN2 = character->GetKeyNum("ARROWQUEST_X", true);
		Args.m_iN3 = character->GetKeyNum("ARROWQUEST_Y", true);zr
#endif

		character->OnTrigger(CTRIG_UserQuestArrowClick, character, &Args);
	}
	return true;
}

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x09 : PacketWrestleDisarm			wrestle disarm macro
 *
 *
 ***************************************************************************/
PacketWrestleDisarm::PacketWrestleDisarm() : Packet(-1)
{
}

bool PacketWrestleDisarm::onReceive(NetState* net)
{
	net->getClient()->SysMessageDefault(DEFMSG_WRESTLING_NOGO);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x0A : PacketWrestleStun			wrestle stun macro
 *
 *
 ***************************************************************************/
PacketWrestleStun::PacketWrestleStun() : Packet(-1)
{
}

bool PacketWrestleStun::onReceive(NetState* net)
{
	net->getClient()->SysMessageDefault(DEFMSG_WRESTLING_NOGO);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x0B : PacketLanguage				language report
 *
 *
 ***************************************************************************/
PacketLanguage::PacketLanguage() : Packet(-1)
{
}

bool PacketLanguage::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	TCHAR language[4];
	readStringNullASCII(language, COUNTOF(language));

	client->GetAccount()->m_lang.Set(language);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x0C : PacketStatusClose			status window closed
 *
 *
 ***************************************************************************/
PacketStatusClose::PacketStatusClose() : Packet(-1)
{
}

bool PacketStatusClose::onReceive(NetState* net)
{
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x0E : PacketAnimationReq			play an animation
 *
 *
 ***************************************************************************/
PacketAnimationReq::PacketAnimationReq() : Packet(-1)
{
}

bool PacketAnimationReq::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

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

	ANIM_TYPE anim = (ANIM_TYPE)readInt32();
	bool ok = false;
	for (int i = 0; ok == false && i < COUNTOF(validAnimations); i++)
		ok = (anim == validAnimations[i]);

	if (ok == false)
		return false;

	character->UpdateAnimate(anim);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x0F : PacketClientInfo				client information
 *
 *
 ***************************************************************************/
PacketClientInfo::PacketClientInfo() : Packet(-1)
{
}

bool PacketClientInfo::onReceive(NetState* net)
{
	BYTE a = readByte();
	DWORD flags = readInt32();
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x13 : PacketPopupReq				request popup menu
 *
 *
 ***************************************************************************/
PacketPopupReq::PacketPopupReq() : Packet(-1)
{
}

bool PacketPopupReq::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	if (IsAosFlagEnabled( FEATURE_AOS_POPUP ) && client->GetResDisp() >= RDS_AOS)
	{
		DWORD serial(readInt32());
		client->Event_AOSPopupMenuRequest(serial);
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x15 : PacketPopupSelect			popup menu option selected
 *
 *
 ***************************************************************************/
PacketPopupSelect::PacketPopupSelect() : Packet(-1)
{
}

bool PacketPopupSelect::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	if (IsAosFlagEnabled( FEATURE_AOS_POPUP ) && client->GetResDisp() >= RDS_AOS)
	{
		DWORD serial = readInt32();
		WORD tag = readInt16();

		client->Event_AOSPopupMenuSelect(serial, tag);
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x1A : PacketChangeStatLock			set stat locks
 *
 *
 ***************************************************************************/
PacketChangeStatLock::PacketChangeStatLock() : Packet(-1)
{
}

bool PacketChangeStatLock::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL || character->m_pPlayer == NULL)
		return false;

	BYTE code = readByte();
	SKILLLOCK_TYPE state = (SKILLLOCK_TYPE)readByte();

	if (code <= STAT_NONE || code >= STAT_BASE_QTY)
		return false;
	else if (state < SKILLLOCK_UP || state > SKILLLOCK_LOCK)
		return false;

	// translate UO stat to Sphere stat
	STAT_TYPE stat(STAT_NONE);
	switch (code)
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

	if (stat != STAT_NONE)
		character->m_pPlayer->Stat_SetLock(stat, state);

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x1C : PacketSpellSelect			select/cast spell
 *
 *
 ***************************************************************************/
PacketSpellSelect::PacketSpellSelect() : Packet(-1)
{
}

bool PacketSpellSelect::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	skip(2); // unknown
	SPELL_TYPE spell = (SPELL_TYPE)readInt16();

	if (IsSetMagicFlags(MAGICF_PRECAST))
	{
		const CSpellDef* spellDef = g_Cfg.GetSpellDef(spell);
		if (spellDef == NULL)
			return true;

		if (spellDef->IsSpellType(SPELLFLAG_NOPRECAST) == false)
		{
			int skill;
			if (spellDef->GetPrimarySkill(&skill, NULL) == false)
				return true;

			client->m_tmSkillMagery.m_Spell = spell;
			character->m_atMagery.m_Spell = spell;
			client->m_Targ_UID = character->GetUID();
			client->m_Targ_PrvUID = character->GetUID();
			character->Skill_Start((SKILL_TYPE)skill);
			return true;
		}
	}

	client->Cmd_Skill_Magery(spell, character);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x1E : PacketHouseDesignReq			house design request
 *
 *
 ***************************************************************************/
PacketHouseDesignReq::PacketHouseDesignReq() : Packet(-1)
{
}

bool PacketHouseDesignReq::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	CItem* house = CGrayUID(readInt32()).ItemFind();
	if (house == NULL)
		return true;

	CItemMultiCustom* multi = dynamic_cast<CItemMultiCustom*>(house);
	if (multi == NULL)
		return true;

	multi->SendStructureTo(client);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x24 : PacketAntiCheat				anti-cheat (unknown)
 *
 *
 ***************************************************************************/
PacketAntiCheat::PacketAntiCheat() : Packet(-1)
{
}

bool PacketAntiCheat::onReceive(NetState* net)
{
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x2C : PacketBandageMacro			bandage macro
 *
 *
 ***************************************************************************/
PacketBandageMacro::PacketBandageMacro() : Packet(-1)
{
}

bool PacketBandageMacro::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	CItem* bandage = CGrayUID(readInt32()).ItemFind();
	CObjBase* target = CGrayUID(readInt32()).ObjFind();
	if (bandage == NULL || target == NULL)
		return true;

	// check the client can see the bandage they're trying to use
	if (character->CanSee(bandage) == false)
	{
		client->addObjectRemoveCantSee(bandage->GetUID(), "the target");
		return true;
	}

	// check the client is capable of using the bandage
	if (character->CanUse(bandage, false) == false)
		return true;

	// check the bandage is in the possession of the client
	if (bandage->GetTopLevelObj() != character)
		return true;

	// make sure the macro isn't used for other types of items
	if (bandage->IsType(IT_BANDAGE) == false)
		return true;

	// clear previous target
	client->SetTargMode();

	// Should we simulate the dclick?
	// client->m_Targ_UID = bandage->GetUID();
	// CScriptTriggerArgs extArgs(1); // Signal we're from the macro
	// if (bandage->OnTrigger( ITRIG_DCLICK, m_pChar, &extArgs ) == TRIGRET_RET_TRUE)
	// 		return true;
	//
	// client->SetTargMode();

	// prepare targeting information
	client->m_Targ_UID = bandage->GetUID();
	client->m_tmUseItem.m_pParent = bandage->GetParent();
	client->SetTargMode(CLIMODE_TARG_USE_ITEM);

	client->Event_Target(CLIMODE_TARG_USE_ITEM, target->GetUID(), target->GetUnkPoint());
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x32 : PacketGargoyleFly			gargoyle toggle flying
 *
 *
 ***************************************************************************/
PacketGargoyleFly::PacketGargoyleFly() : Packet(-1)
{
}

bool PacketGargoyleFly::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

#ifdef _DEBUG
	int one = readInt16();
	int zero = readInt32();

	if (one != 1 || zero != 0)
		g_Log.EventDebug("Unexpected flying parameters: %d, %d.\n", one, zero);

#endif

	switch (character->GetDispID())
	{
		case CREID_GARGMAN:
		case CREID_GARGWOMAN:
		case CREID_GARGGHOSTMAN:
		case CREID_GARGGHOSTWOMAN:
			if (character->IsStatFlag(STATF_DEAD))
				break;

			if (character->IsStatFlag(STATF_Hovering))
			{
				// stop hovering
				character->StatFlag_Clear(STATF_Hovering);
			}
			else
			{
				// begin hovering
				character->StatFlag_Set(STATF_Hovering);

				// float player up to the hover Z
				CPointMap ptHover = g_World.FindItemTypeNearby(character->GetTopPoint(), IT_HOVEROVER, 0, false, false);
				if (ptHover.IsValidPoint())
					character->MoveTo(ptHover);
			}

			character->UpdateModeFlag();
			break;
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xC2 : PacketPromptResponseUnicode		prompt response (unicode)
 *
 *
 ***************************************************************************/
PacketPromptResponseUnicode::PacketPromptResponseUnicode() : Packet(-1)
{
}

bool PacketPromptResponseUnicode::onReceive(NetState* net)
{
	WORD length = readInt16();
	DWORD context1 = readInt32();
	DWORD context2 = readInt32();
	DWORD type = readInt32();
	TCHAR language[4];
	readStringASCII(language, COUNTOF(language));
	TCHAR* text = Str_GetTemp();
	length = (length - getPosition()) / 2;
	readStringUNICODE(text, length+1);

	net->getClient()->Event_PromptResp(text, length, context1, context2, type);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xC8 : PacketViewRange					change view range
 *
 *
 ***************************************************************************/
PacketViewRange::PacketViewRange() : Packet(2)
{
}

bool PacketViewRange::onReceive(NetState* net)
{
	BYTE range = readByte();
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD1 : PacketLogout			client logout notification
 *
 *
 ***************************************************************************/
PacketLogout::PacketLogout() : Packet(1)
{
}

bool PacketLogout::onReceive(NetState* net)
{
	PacketLogoutAck *packet = new PacketLogoutAck(net->getClient());
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD6 : PacketAOSTooltipReq				aos tooltip request
 *
 *
 ***************************************************************************/
PacketAOSTooltipReq::PacketAOSTooltipReq() : Packet(-1)
{
}

bool PacketAOSTooltipReq::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	if (net->isClientVersion(0x400000) == false)
		return true;
	else if (client->GetResDisp() < RDS_AOS || !IsAosFlagEnabled(FEATURE_AOS_UPDATE_B))
		return true;

	CObjBase* object;
	for (WORD length = readInt16(); length > sizeof(DWORD); length -= sizeof(DWORD))
	{
		object = CGrayUID(readInt32()).ObjFind();
		if (object == NULL)
			continue;
		else if (character->CanSee(object) == false)
			continue;

		client->addAOSTooltip(object);
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7 : PacketEncodedCommand				encoded command
 *
 *
 ***************************************************************************/
PacketEncodedCommand::PacketEncodedCommand() : Packet(-1)
{
}

bool PacketEncodedCommand::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	if (client->GetChar() == NULL)
		return false;

	WORD packetLength = readInt16();
	CGrayUID serial(readInt32());
	if (client->GetChar()->GetUID() != serial)
		return false;

	EXTAOS_TYPE type = (EXTAOS_TYPE)readInt16();
	seek();

	Packet* handler = g_NetworkIn.getEncodedHandler(type);
	if (handler == NULL)
		return false;

	handler->seek();
	for (int i = 0; i < packetLength; i++)
	{
		BYTE next = readByte();
		handler->writeByte(next);
	}

	handler->resize(packetLength);
	handler->seek(9);
	return handler->onReceive(net);
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x02 : PacketHouseDesignBackup		backup house design
 *
 *
 ***************************************************************************/
PacketHouseDesignBackup::PacketHouseDesignBackup() : Packet(-1)
{
}

bool PacketHouseDesignBackup::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	house->BackupStructure(client);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x03 : PacketHouseDesignRestore		restore house design
 *
 *
 ***************************************************************************/
PacketHouseDesignRestore::PacketHouseDesignRestore() : Packet(-1)
{
}

bool PacketHouseDesignRestore::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	house->RestoreStructure(client);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x04 : PacketHouseDesignCommit		commit house design
 *
 *
 ***************************************************************************/
PacketHouseDesignCommit::PacketHouseDesignCommit() : Packet(-1)
{
}

bool PacketHouseDesignCommit::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	house->CommitChanges(client);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x05 : PacketHouseDesignDestroyItem	destroy house design item
 *
 *
 ***************************************************************************/
PacketHouseDesignDestroyItem::PacketHouseDesignDestroyItem() : Packet(-1)
{
}

bool PacketHouseDesignDestroyItem::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	skip(1); // 0x00
	ITEMID_TYPE id = (ITEMID_TYPE)readInt32();
	skip(1); // 0x00
	WORD x = readInt32();
	skip(1); // 0x00
	WORD y = readInt32();
	skip(1); // 0x00
	WORD z = readInt32();

	house->RemoveItem(client, id, x, y, z);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x06 : PacketHouseDesignPlaceItem	place house design item
 *
 *
 ***************************************************************************/
PacketHouseDesignPlaceItem::PacketHouseDesignPlaceItem() : Packet(-1)
{
}

bool PacketHouseDesignPlaceItem::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	skip(1); // 0x00
	ITEMID_TYPE id = (ITEMID_TYPE)readInt32();
	skip(1); // 0x00
	WORD x = readInt32();
	skip(1); // 0x00
	WORD y = readInt32();

	house->AddItem(client, id, x, y);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x0C : PacketHouseDesignExit		exit house designer
 *
 *
 ***************************************************************************/
PacketHouseDesignExit::PacketHouseDesignExit() : Packet(-1)
{
}

bool PacketHouseDesignExit::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	house->EndCustomize();
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x0D : PacketHouseDesignPlaceStair	place house design stairs
 *
 *
 ***************************************************************************/
PacketHouseDesignPlaceStair::PacketHouseDesignPlaceStair() : Packet(-1)
{
}

bool PacketHouseDesignPlaceStair::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	skip(1); // 0x00
	ITEMID_TYPE id = (ITEMID_TYPE)(readInt32() + ITEMID_MULTI);
	skip(1); // 0x00
	WORD x = readInt32();
	skip(1); // 0x00
	WORD y = readInt32();

	house->AddStairs(client, id, x, y);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x0E : PacketHouseDesignSync		synchronise house design
 *
 *
 ***************************************************************************/
PacketHouseDesignSync::PacketHouseDesignSync() : Packet(-1)
{
}

bool PacketHouseDesignSync::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	house->SendStructureTo(client);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x10 : PacketHouseDesignClear		clear house design
 *
 *
 ***************************************************************************/
PacketHouseDesignClear::PacketHouseDesignClear() : Packet(-1)
{
}

bool PacketHouseDesignClear::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	house->ResetStructure(client);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x12 : PacketHouseDesignSwitch		switch house design floor
 *
 *
 ***************************************************************************/
PacketHouseDesignSwitch::PacketHouseDesignSwitch() : Packet(-1)
{
}

bool PacketHouseDesignSwitch::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	skip(1); // 0x00
	int level = readInt32();

	house->SwitchToLevel(client, level);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x13 : PacketHouseDesignPlaceRoof	place house design roof
 *
 *
 ***************************************************************************/
PacketHouseDesignPlaceRoof::PacketHouseDesignPlaceRoof() : Packet(-1)
{
}

bool PacketHouseDesignPlaceRoof::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	skip(1); // 0x00
	ITEMID_TYPE id = (ITEMID_TYPE)readInt32();
	skip(1); // 0x00
	WORD x = readInt32();
	skip(1); // 0x00
	WORD y = readInt32();
	skip(1); // 0x00
	WORD z = readInt32();

	house->AddRoof(client, id, x, y, z);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x14 : PacketHouseDesignDestroyRoof	destroy house design roof
 *
 *
 ***************************************************************************/
PacketHouseDesignDestroyRoof::PacketHouseDesignDestroyRoof() : Packet(-1)
{
}

bool PacketHouseDesignDestroyRoof::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	skip(1); // 0x00
	ITEMID_TYPE id = (ITEMID_TYPE)readInt32();
	skip(1); // 0x00
	WORD x = readInt32();
	skip(1); // 0x00
	WORD y = readInt32();
	skip(1); // 0x00
	WORD z = readInt32();

	house->RemoveRoof(client, id, x, y, z);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x19 : PacketSpecialMove			perform special move
 *
 *
 ***************************************************************************/
PacketSpecialMove::PacketSpecialMove() : Packet(-1)
{
}

bool PacketSpecialMove::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	skip(1);
	DWORD ability = readInt32();

	CScriptTriggerArgs args;
	args.m_iN1 = ability;
	character->OnTrigger(CTRIG_UserSpecialMove, character, &args);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x1A : PacketHouseDesignRevert		revert house design
 *
 *
 ***************************************************************************/
PacketHouseDesignRevert::PacketHouseDesignRevert() : Packet(-1)
{
}

bool PacketHouseDesignRevert::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	house->RevertChanges(client);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x1E : PacketEquipLastWeapon		equip last weapon macro
 *
 *
 ***************************************************************************/
PacketEquipLastWeapon::PacketEquipLastWeapon() : Packet(-1)
{
}

bool PacketEquipLastWeapon::onReceive(NetState* net)
{
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x28 : PacketGuildButton			guild button pressed
 *
 *
 ***************************************************************************/
PacketGuildButton::PacketGuildButton() : Packet(-1)
{
}

bool PacketGuildButton::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	character->OnTrigger(CTRIG_UserGuildButton, character, NULL);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x32 : PacketQuestButton			quest button pressed
 *
 *
 ***************************************************************************/
PacketQuestButton::PacketQuestButton() : Packet(-1)
{
}

bool PacketQuestButton::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	character->OnTrigger(CTRIG_UserQuestButton, character, NULL);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD9 : PacketHardwareInfo	hardware info from client
 *
 *
 ***************************************************************************/
PacketHardwareInfo::PacketHardwareInfo() : Packet(268)
{
}

bool PacketHardwareInfo::onReceive(NetState* net)
{
	skip(1); // client type
	skip(4); // instance id
	skip(4); // os major
	skip(4); // os minor
	skip(4); // os revision
	skip(1); // cpu manufacturer
	skip(4); // cpu family
	skip(4); // cpu model
	skip(4); // cpu clock speed
	skip(1); // cpu quantity
	skip(4); // physical memory
	skip(4); // screen width
	skip(4); // screen height
	skip(4); // screen depth
	skip(2); // directx major
	skip(2); // directx minor
	skip(64 * sizeof(WCHAR)); // video card description
	skip(4); // video card vendor id
	skip(4); // video card device id
	skip(4); // video card memory
	skip(1); // distribution
	skip(1); // clients running
	skip(1); // clients installed
	skip(1); // clients partial installed
	skip(4 * sizeof(WCHAR)); // language
	skip(32 * sizeof(WCHAR)); // unknown
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xE0 : PacketBugReport					bug report
 *
 *
 ***************************************************************************/
PacketBugReport::PacketBugReport() : Packet(-1)
{
}

bool PacketBugReport::onReceive(NetState* net)
{
	WORD packetLength = readInt16(); // packet length
	if (packetLength < 10)
		return false;

	TCHAR language[4];
	readStringASCII(language, COUNTOF(language));

	BUGREPORT_TYPE type = (BUGREPORT_TYPE)readInt16();

	TCHAR text[MAX_TALK_BUFFER];
	int textLength = readStringNullNUNICODE(text, MAX_TALK_BUFFER-1);

	net->getClient()->Event_BugReport(text, textLength, type, CLanguageID(language));
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xE1 : PacketClientType		client type (KR/SA)
 *
 *
 ***************************************************************************/
PacketClientType::PacketClientType() : Packet(-1)
{
}

bool PacketClientType::onReceive(NetState* net)
{
	WORD packetLength = readInt16(); // packet length
	if (packetLength < 9)
		return false;

	skip(2); // ..count?
	GAMECLIENT_TYPE type = (GAMECLIENT_TYPE)readInt32();

	net->m_clientType = type;
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xE8 : PacketRemoveUIHighlight			remove ui highlight
 *
 *
 ***************************************************************************/
PacketRemoveUIHighlight::PacketRemoveUIHighlight() : Packet(13)
{
}

bool PacketRemoveUIHighlight::onReceive(NetState* net)
{
	skip(4); // serial
	skip(2); // ui id
	skip(4); // destination serial
	skip(1); // 1
	skip(1); // 1
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xEB : PacketUseHotbar					use hotbar
 *
 *
 ***************************************************************************/
PacketUseHotbar::PacketUseHotbar() : Packet(11)
{
}

bool PacketUseHotbar::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	skip(2); // 1
	skip(2); // 6
	BYTE type = readByte();
	skip(1); // zero
	DWORD parameter = readInt32();

	client->Event_UseToolbar(type, parameter);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xEC : PacketEquipItemMacro				equip item(s) macro
 *
 *
 ***************************************************************************/
PacketEquipItemMacro::PacketEquipItemMacro() : Packet(-1)
{
}

bool PacketEquipItemMacro::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	skip(2); // packet length
	int itemCount = readByte();

	CItem* item;
	for (int i = 0; i < itemCount; i++)
	{
		item = CGrayUID(readInt32()).ItemFind();
		if (item == NULL)
			continue;

		if (item->GetTopLevelObj() != character || item->IsItemEquipped())
			continue;

		if (character->ItemPickup(item, item->GetAmount()) < 1)
			continue;

		character->ItemEquip(item);
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xED : PacketUnEquipItemMacro			unequip item(s) macro
 *
 *
 ***************************************************************************/
PacketUnEquipItemMacro::PacketUnEquipItemMacro() : Packet(-1)
{
}

bool PacketUnEquipItemMacro::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	skip(2); // packet length
	int layerCount = readByte();

	LAYER_TYPE layer;
	CItem* item;
	for (int i = 0; i < layerCount; i++)
	{
		layer = (LAYER_TYPE)readInt16();

		item = character->LayerFind(layer);
		if (item == NULL)
			continue;

		if (character->ItemPickup(item, item->GetAmount()) < 1)
			continue;

		character->ItemBounce(item);
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xF0 : PacketMovementReqNew	movement request (KR/SA)
 *
 *
 ***************************************************************************/
PacketMovementReqNew::PacketMovementReqNew() : PacketMovementReq(-1)
{
}

bool PacketMovementReqNew::onReceive(NetState* net)
{
	skip(2);
	skip(17);
	int sequence = readByte();
	BYTE direction = readByte();
	DWORD mode = readInt32();
	if (mode == 2)
		direction |= 0x80;

	doMovement(net, direction, sequence, 0);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xF4 : PacketCrashReport					crash report
 *
 *
 ***************************************************************************/
PacketCrashReport::PacketCrashReport() : Packet(-1)
{
}

bool PacketCrashReport::onReceive(NetState* net)
{
	CClient* client = net->getClient();
	ASSERT(client);

	WORD packetLength = readInt16(); // packet length
	BYTE versionMaj = readByte();
	BYTE versionMin = readByte();
	BYTE versionRev = readByte();
	BYTE versionPat = readByte();
	WORD x = readInt16();
	WORD y = readInt16();
	BYTE z = readByte();
	BYTE map = readByte();
	skip(32); // account name
	skip(32); // character name
	DWORD errorCode = readInt32();
	TCHAR executable[100];
	readStringASCII(executable, COUNTOF(executable));
	TCHAR description[100];
	readStringASCII(description, COUNTOF(description));

	g_Log.EventWarn("%x:Client crashed at %d,%d,%d,%d: 0x%08X %s (%s, %d.%d.%d.%d)\n", net->id(),
					x, y, z, map,
					errorCode, description, executable,
					versionMaj, versionMin, versionRev, versionPat);
	return true;
}

