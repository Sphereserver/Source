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
PacketUnknown::PacketUnknown(size_t size) : Packet(size)
{
}

bool PacketUnknown::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketUnknown::onReceive");
	UNREFERENCED_PARAMETER(net);

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x00 : PacketCreate		create new character request
 *
 *
 ***************************************************************************/
PacketCreate::PacketCreate(size_t size) : Packet(size)
{
}

bool PacketCreate::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketCreate::onReceive");
	return PacketCreate::onReceive(net, false);
}

bool PacketCreate::onReceive(NetState* net, bool hasExtraSkill)
{
	ADDTOCALLSTACK("PacketCreate::onReceive[1]");
	TCHAR charname[MAX_NAME_SIZE];
	SKILL_TYPE skill1 = SKILL_NONE, skill2 = SKILL_NONE, skill3 = SKILL_NONE, skill4 = SKILL_NONE;
	BYTE skillval1 = 0, skillval2 = 0, skillval3 = 0, skillval4 = 0;

	skip(9); // 4=pattern1, 4=pattern2, 1=kuoc
	readStringASCII(charname, MAX_NAME_SIZE);
	skip(2); // 0x00
	DWORD flags = readInt32();
	skip(8); // unk
	PROFESSION_TYPE prof = static_cast<PROFESSION_TYPE>(readByte());
	skip(15); // 0x00
	BYTE sex = readByte();
	BYTE strength = readByte();
	BYTE dexterity = readByte();
	BYTE intelligence = readByte();
	skill1 = static_cast<SKILL_TYPE>(readByte());
	skillval1 = readByte();
	skill2 = static_cast<SKILL_TYPE>(readByte());
	skillval2 = readByte();
	skill3 = static_cast<SKILL_TYPE>(readByte());
	skillval3 = readByte();
	if (hasExtraSkill)
	{
		skill4 = static_cast<SKILL_TYPE>(readByte());
		skillval4 = readByte();
	}
	HUE_TYPE hue = static_cast<HUE_TYPE>(readInt16());
	ITEMID_TYPE hairid = static_cast<ITEMID_TYPE>(readInt16());
	HUE_TYPE hairhue = static_cast<HUE_TYPE>(readInt16());
	ITEMID_TYPE beardid = static_cast<ITEMID_TYPE>(readInt16());
	HUE_TYPE beardhue = static_cast<HUE_TYPE>(readInt16());
	skip(1); // shard index
	BYTE startloc = readByte();
	skip(8); // 4=slot, 4=ip
	HUE_TYPE shirthue = static_cast<HUE_TYPE>(readInt16());
	HUE_TYPE pantshue = static_cast<HUE_TYPE>(readInt16());

	bool isFemale = (sex % 2) != 0; // Even=Male, Odd=Female (rule applies to all clients)
	RACE_TYPE rtRace = RACETYPE_HUMAN; // Human

	// determine which race the client has selected
	if (net->isClientVersion(MINCLIVER_SA) || net->isClientEnhanced())
	{
		/*
			m_sex values from clients 7.0.0.0+
			0x2 = Human (male)
			0x3 = Human (female)
			0x4 = Elf (male)
			0x5 = Elf (female)
			0x6 = Gargoyle (male)
			0x7 = Gargoyle (female)
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
		/*
			m_sex values from clients pre-7.0.0.0
			0x0 = Human (male)
			0x1 = Human (female)
			0x2 = Elf (male)
			0x3 = Elf (female)
		*/
		if ((sex - 2) >= 0)
			rtRace = RACETYPE_ELF;
	}

	// validate race against resdisp
	RESDISPLAY_VERSION resdisp = net->m_client->m_pAccount ? static_cast<RESDISPLAY_VERSION>(net->m_client->m_pAccount->GetResDisp()) : RDS_NONE;
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
	
	return doCreate(net, charname, isFemale, rtRace,
		strength, dexterity, intelligence, prof,
		skill1, skillval1, skill2, skillval2, skill3, skillval3, skill4, skillval4,
		hue, hairid, hairhue, beardid, beardhue, shirthue, pantshue,
		startloc, 0, flags);
}

bool PacketCreate::doCreate(NetState* net, LPCTSTR charname, bool bFemale, RACE_TYPE rtRace, short wStr, short wDex, short wInt, PROFESSION_TYPE prProf, SKILL_TYPE skSkill1, int iSkillVal1, SKILL_TYPE skSkill2, int iSkillVal2, SKILL_TYPE skSkill3, int iSkillVal3, SKILL_TYPE skSkill4, int iSkillVal4, HUE_TYPE wSkinHue, ITEMID_TYPE idHair, HUE_TYPE wHairHue, ITEMID_TYPE idBeard, HUE_TYPE wBeardHue, HUE_TYPE wShirtHue, HUE_TYPE wPantsHue, int iStartLoc, int iPortrait, int iFlags)
{
	ADDTOCALLSTACK("PacketCreate::doCreate");

	CClient* client = net->m_client;
	ASSERT(client);
	const CAccountRef account = client->m_pAccount;
	ASSERT(account);

	if (client->GetChar() != NULL)
	{
		// logging in as a new player whilst already online !
		client->addSysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_ALREADYONLINE));
		DEBUG_ERR(("%lx:Setup_CreateDialog acct='%s' already online!\n", net->id(), account->GetName()));
		return false;
	}

	// make sure they don't have an idling character
	const CChar* pCharLast = account->m_uidLastChar.CharFind();
	if (pCharLast != NULL && account->IsMyAccountChar(pCharLast) && account->GetPrivLevel() <= PLEVEL_GM && !pCharLast->IsDisconnected() )
	{
		client->addIdleWarning(PacketWarningMessage::CharacterInWorld);
		client->addLoginErr(PacketLoginError::CharIdle);
		return false;
	}

	// make sure they don't already have too many characters
	BYTE iMaxChars = account->GetMaxChars();
	size_t iQtyChars = account->m_Chars.GetCharCount();
	if (iQtyChars >= iMaxChars)
	{
		client->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_MAXCHARS), static_cast<int>(iQtyChars));
		if (client->GetPrivLevel() < PLEVEL_Seer)
		{
			client->addLoginErr(PacketLoginError::TooManyChars);
			return false;
		}
	}
	

	CChar* pChar = CChar::CreateBasic(CREID_MAN);
	ASSERT(pChar != NULL);

	TRIGRET_TYPE tr;
	CScriptTriggerArgs createArgs;
	createArgs.m_iN1 = iFlags;
	createArgs.m_iN2 = prProf;
	createArgs.m_iN3 = rtRace;
	createArgs.m_VarsLocal.SetNum("PORTRAIT", iPortrait);
	createArgs.m_s1 = account->GetName();
	createArgs.m_pO1 = client;
	
	//Creating the pChar
	pChar->InitPlayer(client, charname, bFemale, rtRace, wStr, wDex, wInt, prProf, skSkill1, iSkillVal1, skSkill2, iSkillVal2, skSkill3, iSkillVal3, skSkill4, iSkillVal4, wSkinHue, idHair, wHairHue, idBeard, wBeardHue, wShirtHue, wPantsHue, iStartLoc);

	//Calling the function after the char creation, it can't be done before or the function won't have SRC
	client->r_Call("f_onchar_create", pChar, &createArgs, NULL, &tr);
	
	if ( tr == 1 )
	{
		client->addLoginErr(PacketLoginError::CreationBlocked);
		pChar->Delete();	//Delete it if function is returning 1 or the char will remain created
		return false;
	}

	g_Log.Event(LOGM_CLIENTS_LOG, "%lx:Setup_CreateDialog acct='%s', char='%s'\n", net->id(), account->GetName(), pChar->GetName());
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
PacketMovementReq::PacketMovementReq(size_t size) : Packet(size)
{
}

bool PacketMovementReq::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketMovementReq::onReceive");

	CClient *client = net->m_client;
	ASSERT(client);

	BYTE direction = readByte();
	BYTE sequence = readByte();
	//DWORD crypt = readInt32();	// client fastwalk crypt (not used anymore)

	if ( net->m_sequence == 0 && sequence != 0 )
		direction = DIR_QTY;	// setting invalid direction to intentionally reject the walk request

	if ( client->Event_Walk(direction, sequence) )
	{
		if ( sequence == UCHAR_MAX )
			sequence = 0;
		net->m_sequence = ++sequence;
	}
	else
	{
		net->m_sequence = 0;
	}
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x03 : PacketSpeakReq			character talking
 *
 *
 ***************************************************************************/
PacketSpeakReq::PacketSpeakReq() : Packet(0)
{
}

bool PacketSpeakReq::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketSpeakReq::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	if (client->GetChar() == NULL)
		return false;

	size_t packetLength = readInt16();
	TALKMODE_TYPE mode = static_cast<TALKMODE_TYPE>(readByte());
	HUE_TYPE hue = static_cast<HUE_TYPE>(readInt16());
	skip(2); // font

	if (packetLength < getPosition())
		return false;

	packetLength -= getPosition();
	if (packetLength > MAX_TALK_BUFFER)
		packetLength = MAX_TALK_BUFFER;

	TCHAR text[MAX_TALK_BUFFER];
	readStringASCII(text, minimum(COUNTOF(text), packetLength));

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
	ADDTOCALLSTACK("PacketAttackReq::onReceive");

	CGrayUID target(readInt32());

	CClient* client = net->m_client;
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
	ADDTOCALLSTACK("PacketDoubleClick::onReceive");

	DWORD serial = readInt32();

	CGrayUID target(serial &~ UID_F_RESOURCE);
	bool macro = (serial & UID_F_RESOURCE) == UID_F_RESOURCE;

	CClient* client = net->m_client;
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
	ADDTOCALLSTACK("PacketItemPickupReq::onReceive");

	CGrayUID serial(readInt32());
	int amount = readInt16();
	
	CClient* client = net->m_client;
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

size_t PacketItemDropReq::getExpectedLength(NetState* net, Packet* packet)
{
	ADDTOCALLSTACK("PacketItemDropReq::getExpectedLength");
	UNREFERENCED_PARAMETER(packet);

	return (net && net->m_client->m_ContainerGridEnabled) ? 15 : 14;
}

bool PacketItemDropReq::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketItemDropReq::onReceive");

	CClient *client = net->m_client;
	ASSERT(client);
	const CChar *character = client->GetChar();
	if ( !character )
		return false;

	CGrayUID serial = readInt32();
	WORD x = readInt16();
	WORD y = readInt16();
	BYTE z = readByte();

	BYTE grid = 0;
	if ( client->m_ContainerGridEnabled )
	{
		grid = readByte();

		// Enhanced clients using containers on 'list view' mode always send grid=255 to server,
		// this means that the item must placed on first grid slot free, and not on slot 255.
		if ( grid == UCHAR_MAX )
			grid = 0;
	}

	CGrayUID container = readInt32();
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
	ADDTOCALLSTACK("PacketSingleClick::onReceive");

	CGrayUID serial(readInt32());

	CClient* client = net->m_client;
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
PacketTextCommand::PacketTextCommand() : Packet(0)
{
}

bool PacketTextCommand::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketTextCommand::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	
	WORD packetLength = readInt16();
	if (packetLength < 5)
		return false;

	EXTCMD_TYPE type = static_cast<EXTCMD_TYPE>(readByte());
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
	ADDTOCALLSTACK("PacketItemEquipReq::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);

	CGrayUID itemSerial(readInt32());
	LAYER_TYPE itemLayer = static_cast<LAYER_TYPE>(readByte());
	CGrayUID targetSerial(readInt32());

	CChar* source = client->GetChar();
	if (source == NULL)
		return false;

	CItem* item = source->LayerFind(LAYER_DRAGGING);
	if ( item == NULL || client->GetTargMode() != CLIMODE_DRAG || item->GetUID() != itemSerial )
	{
		// I have no idea why i got here.
		new PacketDragCancel(client, PacketDragCancel::Other);
		return true;
	}

	client->ClearTargMode(); // done dragging.

	CChar* target = targetSerial.CharFind();
	bool bCanCarry = target->CanCarry(item);
	if ( target == NULL || (itemLayer >= LAYER_HORSE) || !target->NPC_IsOwnedBy(source) || !bCanCarry || !target->ItemEquip(item, source) )
	{
		client->Event_Item_Drop_Fail(item);		//cannot equip
		if ( !bCanCarry )
			client->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_HEAVY));
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
	ADDTOCALLSTACK("PacketResynchronize::onReceive");

	CClient * client = net->m_client;
	ASSERT(client);

	CChar * pChar = client->GetChar();
	if ( !pChar )
		return false;

	new PacketCharacter(client, pChar);
	client->addPlayerUpdate();
	client->addPlayerSee(NULL);
	net->m_sequence = 0;
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

size_t PacketDeathStatus::getExpectedLength(NetState* net, Packet* packet)
{
	ADDTOCALLSTACK("PacketDeathStatus::getExpectedLength");
	UNREFERENCED_PARAMETER(net);

	// different size depending on client
	size_t pos = packet->getPosition();
	packet->skip(1);
	DEATH_MODE_TYPE mode = static_cast<DEATH_MODE_TYPE>(readByte());
	packet->seek(pos);

	if (mode != DEATH_MODE_MANIFEST)
		return 2;

	return 7;
}

bool PacketDeathStatus::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketDeathStatus::onReceive");  

	CClient* client = net->m_client;
	ASSERT(client);
	CChar* ghost = client->GetChar();
	if (ghost == NULL)
		return false;

	DEATH_MODE_TYPE mode = static_cast<DEATH_MODE_TYPE>(readByte());
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
	ADDTOCALLSTACK("PacketCharStatusReq::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	if ( !client->GetChar() )
		return false;

	skip(4);	// 0xedededed
	BYTE requestType = readByte();
	CGrayUID targetSerial = static_cast<CGrayUID>(readInt32());

	if ( requestType == 4 )
		client->addCharStatWindow(targetSerial.CharFind(), true);
	else if ( requestType == 5 )
		client->addSkillWindow(SKILL_QTY);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x3A : PacketSkillLockChange	change skill locks
 *
 *
 ***************************************************************************/
PacketSkillLockChange::PacketSkillLockChange() : Packet(0)
{
}

bool PacketSkillLockChange::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketSkillLockChange::onReceive");

	CClient* client = net->m_client;
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
		SKILL_TYPE index = static_cast<SKILL_TYPE>(readInt16());
		SKILLLOCK_TYPE state = static_cast<SKILLLOCK_TYPE>(readByte());
		len -= 3;
		
		if (index <= SKILL_NONE || index >= SKILL_QTY ||
			state < SKILLLOCK_UP || state > SKILLLOCK_LOCK)
			continue;

		character->m_pPlayer->Skill_SetLock(index, state);
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
PacketVendorBuyReq::PacketVendorBuyReq() : Packet(0)
{
}

bool PacketVendorBuyReq::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketVendorBuyReq::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	const CChar* buyer = client->GetChar();
	if (buyer == NULL)
		return false;

	WORD packetLength = readInt16();
	CGrayUID vendorSerial(readInt32());
	BYTE flags = readByte();
	if (flags == 0)
		return true;

	CChar* vendor = vendorSerial.CharFind();
	if (vendor == NULL || vendor->m_pNPC == NULL || !vendor->NPC_IsVendor())
	{
		client->Event_VendorBuy_Cheater(0x1);
		return true;
	}

	if (buyer->CanTouch(vendor) == false)
	{
		client->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_CANTREACH));
		return true;
	}

	int iConvertFactor = vendor->NPC_GetVendorMarkup();

	VendorItem items[MAX_ITEMS_CONT];
	memset(items, 0, sizeof(items));
	size_t itemCount = minimum((packetLength - 8) / 7, MAX_ITEMS_CONT);

	// check buying speed
	const CVarDefCont* vardef = g_Cfg.m_bAllowBuySellAgent ? NULL : client->m_TagDefs.GetKey("BUYSELLTIME");
	if (vardef != NULL)
	{
		CServTime allowsell;
		allowsell.InitTime(vardef->GetValNum() + (itemCount * 3));
		if (g_World.GetCurrentTime() < allowsell)
		{
			client->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_BUYFAST));
			return true;
		}
	}

	// combine goods into one list
	CItemVendable *item = NULL;
	for (size_t i = 0; i < itemCount; i++)
	{
		skip(1); // layer
		CGrayUID serial(readInt32());
		WORD amount = readInt16();

		item = dynamic_cast<CItemVendable*>(serial.ItemFind());
		if (item == NULL || item->IsValidSaleItem(true) == false)
		{
			client->Event_VendorBuy_Cheater(0x2);
			return true;
		}

		// search for it in the list
		size_t index;
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
			client->Event_VendorBuy_Cheater(0x3);
			return true;
		}
	}

	client->Event_VendorBuy(vendor, items, itemCount);
	return true;
}

/***************************************************************************
 *
 *
 *	Packet 0x3F : PacketStaticUpdate		Ultima live and (God Client?)
 *
 *
 ***************************************************************************/
PacketStaticUpdate::PacketStaticUpdate() : Packet(0)
{
}

bool PacketStaticUpdate::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketStaticUpdate::onReceive");
	/*skip(12);
    BYTE UlCmd = readByte();*/
	TemporaryString dump;
	this->dump(dump);
	g_Log.EventDebug("%lx:Parsing %s", net->id(), static_cast<LPCTSTR>(dump));
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
	ADDTOCALLSTACK("PacketMapEdit::onReceive");

	CGrayUID mapSerial(readInt32());
	MAPCMD_TYPE action = static_cast<MAPCMD_TYPE>(readByte());
	BYTE pin = readByte();
	WORD x = readInt16();
	WORD y = readInt16();

	CClient* client = net->m_client;
	ASSERT(client);
	const CChar* character = client->GetChar();
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

		default:
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
	ADDTOCALLSTACK("PacketCharPlay::onReceive");

	skip(4); // 0xedededed
	skip(MAX_NAME_SIZE); // char name
	skip(MAX_NAME_SIZE); // char pass
	unsigned int slot = readInt32();
	skip(4); // ip

	CClient* client = net->m_client;
	if (!client)	//Sometimes seems to happen? returning here to avoid console errors because of assert
		return false;
	//ASSERT(client);

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
PacketBookPageEdit::PacketBookPageEdit() : Packet(0)
{
}

bool PacketBookPageEdit::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketBookPageEdit::onReceive");

	CClient* client = net->m_client;
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
		client->addBookPage(book, page, 1); // just a request for a page
		return true;
	}

	// trying to write to the book
	CItemMessage* text = dynamic_cast<CItemMessage*>( book );
	if (text == NULL || book->IsBookWritable() == false)
		return true;

	skip(-4);

	size_t len = 0;
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

		ASSERT(len > 0);
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
	ADDTOCALLSTACK("PacketTarget::onReceive");

	CClient* client = net->m_client;
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
	ITEMID_TYPE id = static_cast<ITEMID_TYPE>(readInt16());

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
PacketSecureTradeReq::PacketSecureTradeReq() : Packet(0)
{
}

bool PacketSecureTradeReq::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketSecureTradeReq::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	const CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	skip(2); // length
	SECURE_TRADE_TYPE action = static_cast<SECURE_TRADE_TYPE>(readByte());
	CGrayUID containerSerial(readInt32());

	CItemContainer* container = dynamic_cast<CItemContainer*>( containerSerial.ItemFind() );
	if (container == NULL)
		return true;
	else if (character != container->GetParent())
		return true;

	switch ( action )
	{
		case SECURE_TRADE_CLOSE:		// cancel trade. send each person cancel messages, move items
			container->Delete();
			return true;

		case SECURE_TRADE_CHANGE:		// change check marks. possible conclude trade
		{
			if ( character->GetDist(container) > UO_MAP_VIEW_SIZE )
			{
				client->SysMessageDefault(DEFMSG_MSG_TRADE_TOOFAR);
				return true;
			}

			INT64 iWaitTime = container->m_itEqTradeWindow.m_iWaitTime;
			INT64 iTimestamp = g_World.GetCurrentTime().GetTimeRaw();
			if ( iWaitTime > iTimestamp )
			{
				INT64 iSeconds = (iWaitTime - iTimestamp) / TICK_PER_SEC;
				client->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_WAIT), iSeconds);
				return true;
			}

			container->Trade_Status(readInt32() != 0);
			return true;
		}

		case SECURE_TRADE_UPDATEGOLD:	// update trade window virtual gold
			container->Trade_UpdateGold(readInt32(), readInt32());
			return true;
		default:
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
PacketBulletinBoardReq::PacketBulletinBoardReq() : Packet(0)
{
}

bool PacketBulletinBoardReq::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketBulletinBoardReq::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	const CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	skip(2);
	BBOARDF_TYPE action = static_cast<BBOARDF_TYPE>(readByte());
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
				DEBUG_ERR(( "%lx:BBoard feed back message bad length %" FMTSIZE_T "\n", net->id(), getLength()));
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

			size_t lenstr = readByte();
			TCHAR* str = Str_GetTemp();
			readStringASCII(str, lenstr, false);
			if (Str_Check(str))
				return true;

			// if 
			CItemMessage* newMessage = dynamic_cast<CItemMessage*>( CItem::CreateBase(ITEMID_BBOARD_MSG) );
			if (newMessage == NULL)
			{
				DEBUG_ERR(("%lx:BBoard can't create message item\n", net->id()));
				return true;
			}

			newMessage->SetAttr(ATTR_MOVE_NEVER);
			newMessage->SetName(str);
			newMessage->SetTimeStamp(CServTime::GetCurrentTime().GetTimeRaw());
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
			DEBUG_ERR(( "%lx:BBoard unknown flag %d\n", net->id(), action));
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
	ADDTOCALLSTACK("PacketWarModeReq::onReceive");

	bool war = readBool();
	skip(3); // unknown
	net->m_client->Event_CombatMode(war);
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
	ADDTOCALLSTACK("PacketPingReq::onReceive");

	BYTE value = readByte();
	new PacketPingAck(net->m_client, value);
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
	ADDTOCALLSTACK("PacketCharRename::onReceive");

	CGrayUID serial(readInt32());
	TCHAR* name = Str_GetTemp();
	readStringASCII(name, MAX_NAME_SIZE);

	net->m_client->Event_SetName(serial, name);
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
	ADDTOCALLSTACK("PacketMenuChoice::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	const CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	CGrayUID serial(readInt32());
	WORD context = readInt16();
	WORD select = readInt16();

	if (context != client->GetTargMode() || serial != client->m_tmMenu.m_UID)
	{
		client->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MENU_UNEXPECTED));
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
			DEBUG_ERR(("%lx:Unknown Targetting mode for menu %d\n", net->id(), context));
			return true;
	}
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
	ADDTOCALLSTACK("PacketServersReq::onReceive");

	TCHAR acctname[MAX_ACCOUNT_NAME_SIZE];
	readStringASCII(acctname, COUNTOF(acctname));
	TCHAR acctpass[MAX_NAME_SIZE];
	readStringASCII(acctpass, COUNTOF(acctpass));
	skip(1);

	CClient* client = net->m_client;
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
	ADDTOCALLSTACK("PacketCharDelete::onReceive");

	skip(MAX_NAME_SIZE); // charpass
	DWORD slot = readInt32();
	skip(4); // client ip

	CClient* client = net->m_client;
	ASSERT(client);

	BYTE err = client->Setup_Delete(slot);
	client->addDeleteErr(err,slot);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x8D : PacketCreateNew		create new character request (KR/SA)
 *
 *
 ***************************************************************************/
PacketCreateNew::PacketCreateNew() : PacketCreate(0)
{
}

bool PacketCreateNew::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketCreateNew::onReceive");

	skip(10); // 2=length, 4=pattern1, 4=pattern2
	TCHAR charname[MAX_NAME_SIZE];
	readStringASCII(charname, MAX_NAME_SIZE);
	skip(30);
	PROFESSION_TYPE profession = static_cast<PROFESSION_TYPE>(readByte());
	skip(1);
	BYTE sex = readByte();
	RACE_TYPE race = static_cast<RACE_TYPE>(readByte());
	BYTE strength = readByte();
	BYTE dexterity = readByte();
	BYTE intelligence = readByte();
	HUE_TYPE hue = static_cast<HUE_TYPE>(readInt16());
	skip(8);
	SKILL_TYPE skill1 = static_cast<SKILL_TYPE>(readByte());
	BYTE skillval1 = readByte();
	SKILL_TYPE skill2 = static_cast<SKILL_TYPE>(readByte());
	BYTE skillval2 = readByte();
	SKILL_TYPE skill4 = static_cast<SKILL_TYPE>(readByte());
	BYTE skillval4 = readByte();
	SKILL_TYPE skill3 = static_cast<SKILL_TYPE>(readByte());
	BYTE skillval3 = readByte();
	skip(26);
	HUE_TYPE hairhue = static_cast<HUE_TYPE>(readInt16());
	ITEMID_TYPE hairid = static_cast<ITEMID_TYPE>(readInt16());
	skip(14); // unk
	BYTE portrait = readByte();
	skip(1);
	HUE_TYPE beardhue = static_cast<HUE_TYPE>(readInt16());
	ITEMID_TYPE beardid = static_cast<ITEMID_TYPE>(readInt16());
	
	// Since client 7.0.16.0 the new creation packet does not contain skills and values if
	// a profession is selected, so here we must translate the selected profession -> skills
	switch (profession)
	{
		case PROFESSION_WARRIOR:
			strength = 45;
			dexterity = 35;
			intelligence = 10;
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
			strength = 25;
			dexterity = 20;
			intelligence = 45;
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
			strength = 60;
			dexterity = 10;
			intelligence = 10;
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
			strength = 25;
			dexterity = 20;
			intelligence = 45;
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
			strength = 45;
			dexterity = 20;
			intelligence = 25;
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
			strength = 40;
			dexterity = 30;
			intelligence = 10;
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
			strength = 40;
			dexterity = 30;
			intelligence = 10;
			skill1 = SKILL_NINJITSU;
			skillval1 = 30;
			skill2 = SKILL_FENCING;
			skillval2 = 30;
			skill3 = SKILL_HIDING;
			skillval3 = 30;
			skill4 = SKILL_STEALTH;
			skillval4 = 30;
			break;
		case PROFESSION_ADVANCED:
			break;
		default:
			DEBUG_WARN(("Unknown profession '%d' selected.\n", profession));
			break;
	}

	return doCreate(net, charname, sex > 0, race,
		strength, dexterity, intelligence, profession,
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
	ADDTOCALLSTACK("PacketCharListReq::onReceive");

	skip(4);
	TCHAR acctname[MAX_ACCOUNT_NAME_SIZE];
	readStringASCII(acctname, COUNTOF(acctname));
	TCHAR acctpass[MAX_NAME_SIZE];
	readStringASCII(acctpass, COUNTOF(acctpass));

	net->m_client->Setup_ListReq(acctname, acctpass, false);
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
	ADDTOCALLSTACK("PacketBookHeaderEdit::onReceive");

	CGrayUID bookSerial(readInt32());
	skip(1); // writable
	skip(1); // unknown
	skip(2); // pages

	TCHAR title[2 * MAX_NAME_SIZE];
	readStringASCII(title, COUNTOF(title));

	TCHAR author[MAX_NAME_SIZE];
	readStringASCII(author, COUNTOF(author));

	net->m_client->Event_Book_Title(bookSerial, title, author);
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
	ADDTOCALLSTACK("PacketDyeObject::onReceive");

	CGrayUID serial(readInt32());
	skip(2); // item id
	HUE_TYPE hue = static_cast<HUE_TYPE>(readInt16());

	net->m_client->Event_Item_Dye(serial, hue);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x98 : PacketAllNamesReq					all names command (ctrl+shift)
 *
 *
 ***************************************************************************/
PacketAllNamesReq::PacketAllNamesReq() : Packet(0)
{
}

bool PacketAllNamesReq::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketAllNamesReq::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	const CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	const CObjBase* object;

	for (WORD length = readInt16(); length > sizeof(DWORD); length -= sizeof(DWORD))
	{
		object = CGrayUID(readInt32()).ObjFind();
		if (object == NULL)
			continue;
		else if (character->CanSee(object) == false)
			continue;

		new PacketAllNamesResponse(client, object);
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
PacketPromptResponse::PacketPromptResponse() : Packet(0)
{
}

bool PacketPromptResponse::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketPromptResponse::onReceive");

	size_t packetLength = readInt16();
	DWORD context1 = readInt32();
	DWORD context2 = readInt32();
	DWORD type = readInt32();

	if (packetLength < getPosition())
		return false;

	packetLength -= getPosition();
	if (packetLength > MAX_TALK_BUFFER)
		packetLength = MAX_TALK_BUFFER;

	TCHAR* text = Str_GetTemp();
	readStringASCII(text, packetLength, false);

	net->m_client->Event_PromptResp(text, packetLength, context1, context2, type);
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
	ADDTOCALLSTACK("PacketHelpPageReq::onReceive");

	CClient* client = net->m_client;
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
PacketVendorSellReq::PacketVendorSellReq() : Packet(0)
{
}

bool PacketVendorSellReq::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketVendorSellReq::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	const CChar* seller = client->GetChar();
	if (seller == NULL)
		return false;

	skip(2); // length
	CGrayUID vendorSerial(readInt32());
	size_t itemCount = readInt16();

	CChar* vendor = vendorSerial.CharFind();
	if (vendor == NULL || vendor->m_pNPC == NULL || !vendor->NPC_IsVendor())
	{
		client->Event_VendorBuy_Cheater(0x1);
		return true;
	}
	
	if (seller->CanTouch(vendor) == false)
	{
		client->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_CANTREACH));
		return true;
	}

	if (itemCount < 1)
	{
		client->addVendorClose(vendor);
		return true;
	}
	else if (itemCount >= MAX_ITEMS_CONT)
	{
		client->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_SELLMUCH));
		return true;
	}

	// check selling speed
	const CVarDefCont* vardef = g_Cfg.m_bAllowBuySellAgent ? NULL : client->m_TagDefs.GetKey("BUYSELLTIME");
	if (vardef != NULL)
	{
		CServTime allowsell;
		allowsell.InitTime(vardef->GetValNum() + (itemCount * 3));
		if (g_World.GetCurrentTime() < allowsell)
		{
			client->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_SELLFAST));
			return true;
		}
	}

	VendorItem items[MAX_ITEMS_CONT];
	memset(items, 0, sizeof(items));

	for (size_t i = 0; i < itemCount; i++)
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
	ADDTOCALLSTACK("PacketServerSelect::onReceive");

	WORD server = readInt16();

	net->m_client->Login_Relay(server);
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
	ADDTOCALLSTACK("PacketSystemInfo::onReceive");
	UNREFERENCED_PARAMETER(net);

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
	ADDTOCALLSTACK("PacketTipReq::onReceive");

	WORD index = readInt16();	// current tip shown to the client
	bool forward = readBool();	// 0=previous, 1=next

	if (forward)
		index++;
	else
		index--;

	CClient* client = net->m_client;
	ASSERT(client);
	client->Event_Tips(index - 1);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xAC : PacketGumpValueInputResponse	gump text input
 *
 *
 ***************************************************************************/
PacketGumpValueInputResponse::PacketGumpValueInputResponse() : Packet(0)
{
}

bool PacketGumpValueInputResponse::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketGumpValueInputResponse::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);

	skip(2); // length
	CGrayUID uid(readInt32());
	readInt16(); // context
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
			client->SysMessagef("Invalid set: %s = %s", static_cast<LPCTSTR>(client->m_Targ_Text), static_cast<LPCTSTR>(text));
		}
		else
		{
			if (client->IsPriv(PRIV_DETAIL))
			{
				client->SysMessagef("Set: %s = %s", static_cast<LPCTSTR>(client->m_Targ_Text), static_cast<LPCTSTR>(text));
			}

			object->RemoveFromView(); // weird client thing
			object->Update();
		}

		g_Log.Event(LOGM_GM_CMDS, "%lx:'%s' tweak uid=0%lx (%s) to '%s %s'=%d\n", net->id(), client->GetName(), static_cast<DWORD>(object->GetUID()), object->GetName(), static_cast<LPCTSTR>(client->m_Targ_Text), static_cast<LPCTSTR>(text), ret);
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
PacketSpeakReqUNICODE::PacketSpeakReqUNICODE() : Packet(0)
{
}

bool PacketSpeakReqUNICODE::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketSpeakReqUNICODE::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	if (client->GetChar() == NULL)
		return false;

	size_t packetLength = readInt16();
	TALKMODE_TYPE mode = static_cast<TALKMODE_TYPE>(readByte());
	HUE_TYPE hue = static_cast<HUE_TYPE>(readInt16());
	FONT_TYPE font = static_cast<FONT_TYPE>(readInt16());
	TCHAR language[4];
	readStringASCII(language, COUNTOF(language));

	if (packetLength < getPosition())
		return false;

	packetLength = (packetLength - getPosition()) / 2;
	if (packetLength >= MAX_TALK_BUFFER)
		packetLength = MAX_TALK_BUFFER - 1;

	if (mode & 0xc0) // text contains keywords
	{
		mode = static_cast<TALKMODE_TYPE>(mode & ~0xc0);

		size_t count = (readInt16() & 0xFFF0) >> 4;
		if (count > 50) // malformed check
			return true;

		skip(-2);
		count = (count + 1) * 12;
		size_t toskip = count / 8;
		if ((count % 8) > 0)
			toskip++;

		if (toskip > (packetLength * 2))
			return true;

		skip(static_cast<long>(toskip));
		TCHAR text[MAX_TALK_BUFFER];
		readStringNullASCII(text, COUNTOF(text));
		client->Event_Talk(text, hue, mode, true);
	}
	else
	{
		NCHAR text[MAX_TALK_BUFFER];
		readStringUNICODE(reinterpret_cast<WCHAR *>(text), packetLength, false);
		client->Event_TalkUNICODE(text, static_cast<int>(packetLength), hue, mode, font, language);
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
PacketGumpDialogRet::PacketGumpDialogRet() : Packet(0)
{
}

bool PacketGumpDialogRet::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketGumpDialogRet::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	skip(2); // length
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
		CChar* viewed = character;
		if (button == 1 && checkCount > 0)
		{
			viewed = CGrayUID(readInt32()).CharFind();
			if (viewed == NULL)
				viewed = character;
		}

		if ( IsTrigUsed(TRIGGER_USERVIRTUE) )
		{
			CScriptTriggerArgs Args(viewed);
			Args.m_iN1 = button;

			character->OnTrigger(CTRIG_UserVirtue, static_cast<CTextConsole *>(character), &Args);
		}

		return true;
	}

#ifdef _DEBUG
	{
		const CResourceDef* resource = g_Cfg.ResourceGetDef(RESOURCE_ID(RES_DIALOG, context));
		if (resource == NULL)
			g_Log.Event(LOGL_EVENT, "Gump: %lu (%s), Uid: 0x%lx, Button: %lu.\n", context, "undef", (DWORD)serial, button);
		else
		{
			const CDialogDef* dialog = dynamic_cast<const CDialogDef*>(resource);
			if (dialog == NULL)
				g_Log.Event(LOGL_EVENT, "Gump: %lu (%s), Uid: 0x%lx, Button: %lu.\n", context, "undef", (DWORD)serial, button);
			else
				g_Log.Event(LOGL_EVENT, "Gump: %lu (%s), Uid: 0x%lx, Button: %lu.\n", context, (LPCTSTR)dialog->GetName(), (DWORD)serial, button);
		}
	}
#endif

	// sanity check
	CClient::OpenedGumpsMap_t::iterator itGumpFound = client->m_mapOpenedGumps.find(static_cast<int>(context));
	if ((itGumpFound == client->m_mapOpenedGumps.end()) || ((*itGumpFound).second <= 0))
		return true;
	
	// Decrement, if <= 0, delete entry.
	(*itGumpFound).second--;
	if ((*itGumpFound).second <= 0)
		client->m_mapOpenedGumps.erase(itGumpFound);

	// package up the gump response info.
	CDialogResponseArgs resp;

	// store the returned checked boxes' ids for possible later use
	for (size_t i = 0; i < checkCount; i++)
		resp.m_CheckArray.Add(readInt32());


	DWORD textCount = readInt32();
	TCHAR* text = Str_GetTemp();
	for (size_t i = 0; i < textCount; i++)
	{
		WORD id = readInt16();
		WORD length = readInt16();
		readStringNUNICODE(text, THREAD_STRING_LENGTH, length, false);

		TCHAR* fix;
		if ((fix = strchr(text, '\n')) != NULL)
			*fix = '\0';
		if ((fix = strchr(text, '\r')) != NULL)
			*fix = '\0';
		if ((fix = strchr(text, '\t')) != NULL)
			*fix = ' ';

		resp.AddText(id, text);
	}

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
PacketChatCommand::PacketChatCommand() : Packet(0)
{
}

bool PacketChatCommand::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketChatCommand::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);

	size_t packetLength = readInt16();
	TCHAR language[4];
	readStringASCII(language, COUNTOF(language));

	if (packetLength < getPosition())
		return false;

	size_t textLength = (packetLength - getPosition()) / 2;
	if (textLength >= MAX_TALK_BUFFER)
		textLength = MAX_TALK_BUFFER - 1;

	NCHAR text[MAX_TALK_BUFFER];
	readStringUNICODE(reinterpret_cast<WCHAR *>(text), textLength, false);

	client->Event_ChatText(text, static_cast<int>(textLength), CLanguageID(language));
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
	ADDTOCALLSTACK("PacketChatButton::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);

	skip(1); // 0x00
	NCHAR name[MAX_NAME_SIZE+1];
	readStringUNICODE(reinterpret_cast<WCHAR *>(name), COUNTOF(name));

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
	ADDTOCALLSTACK("PacketToolTipReq::onReceive");

	CClient* client = net->m_client;
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
PacketProfileReq::PacketProfileReq() : Packet(0)
{
}

bool PacketProfileReq::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketProfileReq::onReceive");

	CClient* client = net->m_client;
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
		readStringNUNICODE(text, THREAD_STRING_LENGTH, textLength+1, false);
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
	ADDTOCALLSTACK("PacketMailMessage::onReceive");

	CClient* client = net->m_client;
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
PacketClientVersion::PacketClientVersion() : Packet(0)
{
}

bool PacketClientVersion::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketClientVersion::onReceive");

	if (net->getReportedVersion())
		return true;

	size_t length = readInt16();
	if (length < getPosition())
		return false;

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
		CClient* client = net->m_client;
		ASSERT(client);

		DWORD version = CCrypt::GetVerFromString(versionStr);
		net->m_reportedVersion = version;
		net->detectAsyncMode();

		DEBUG_MSG(("Getting CliVersionReported %lu\n", version));
		
		if ((g_Serv.m_ClientVersion.GetClientVer() != 0) && (g_Serv.m_ClientVersion.GetClientVer() != version))
			client->addLoginErr(PacketLoginError::BadVersion);

		// Store the value on a temporary tag, it will be needed later
		if ( client->m_pAccount )
			client->m_pAccount->m_TagDefs.SetNum("ReportedCliVer", version);
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
PacketExtendedCommand::PacketExtendedCommand() : Packet(0)
{
}

bool PacketExtendedCommand::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketExtendedCommand::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	if (client->GetChar() == NULL)
		return false;

	WORD packetLength = readInt16();
	EXTDATA_TYPE type = static_cast<EXTDATA_TYPE>(readInt16());
	seek();

#ifndef _MTNETWORK
	Packet* handler = g_NetworkIn.getPacketManager().getExtendedHandler(type);
#else
	Packet* handler = g_NetworkManager.getPacketManager().getExtendedHandler(type);
#endif
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
PacketScreenSize::PacketScreenSize() : Packet(0)
{
}

bool PacketScreenSize::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketScreenSize::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);

	DWORD x = readInt32();
	DWORD y = readInt32();
	
	DEBUG_MSG(("0x%lx - 0x%lx (%ld-%ld)\n", x, y, x, y));

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
PacketPartyMessage::PacketPartyMessage() : Packet(0)
{
}

bool PacketPartyMessage::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketPartyMessage::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	PARTYMSG_TYPE code = static_cast<PARTYMSG_TYPE>(readByte());
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
			NWORD * text = reinterpret_cast<NWORD *>(Str_GetTemp());
			int length = readStringNullUNICODE(reinterpret_cast<WCHAR *>(text), MAX_TALK_BUFFER);
			character->m_pParty->MessageEvent(serial, character->GetUID(), text, length);
		} break;

		case PARTYMSG_Msg:
		{
			// send message to the whole party
			if (character->m_pParty == NULL)
				return false;

			NWORD * text = reinterpret_cast<NWORD *>(Str_GetTemp());
			int length = readStringNullUNICODE(reinterpret_cast<WCHAR *>(text), MAX_TALK_BUFFER);
			character->m_pParty->MessageEvent(CGrayUID(0), character->GetUID(), text, length);
		} break;

		case PARTYMSG_Option:
		{
			// set the loot flag
			if (character->m_pParty == NULL)
				return false;

			character->m_pParty->SetLootFlag(character, readBool());
			character->NotoSave_Update();
		} break;

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
PacketArrowClick::PacketArrowClick() : Packet(0)
{
}

bool PacketArrowClick::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketArrowClick::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	bool rightClick = readBool();

	client->SysMessageDefault(DEFMSG_MSG_FOLLOW_ARROW);

	if ( IsTrigUsed(TRIGGER_USERQUESTARROWCLICK) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = (rightClick == true? 1 : 0);
#ifdef _ALPHASPHERE
		Args.m_iN2 = character->GetKeyNum("ARROWQUEST_X");
		Args.m_iN3 = character->GetKeyNum("ARROWQUEST_Y");
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
PacketWrestleDisarm::PacketWrestleDisarm() : Packet(0)
{
}

bool PacketWrestleDisarm::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketWrestleDisarm::onReceive");

	net->m_client->SysMessageDefault(DEFMSG_MSG_WRESTLING_NOGO);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x0A : PacketWrestleStun			wrestle stun macro
 *
 *
 ***************************************************************************/
PacketWrestleStun::PacketWrestleStun() : Packet(0)
{
}

bool PacketWrestleStun::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketWrestleStun::onReceive");

	net->m_client->SysMessageDefault(DEFMSG_MSG_WRESTLING_NOGO);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x0B : PacketLanguage				language report
 *
 *
 ***************************************************************************/
PacketLanguage::PacketLanguage() : Packet(0)
{
}

bool PacketLanguage::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketLanguage::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);

	TCHAR language[4];
	readStringNullASCII(language, COUNTOF(language));

	client->m_pAccount->m_lang.Set(language);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x0C : PacketStatusClose			status window closed
 *
 *
 ***************************************************************************/
PacketStatusClose::PacketStatusClose() : Packet(0)
{
}

bool PacketStatusClose::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketStatusClose::onReceive");
	UNREFERENCED_PARAMETER(net);

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x0E : PacketAnimationReq			play an animation
 *
 *
 ***************************************************************************/
PacketAnimationReq::PacketAnimationReq() : Packet(0)
{
}

bool PacketAnimationReq::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketAnimationReq::onReceive");

	CClient* client = net->m_client;
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

	ANIM_TYPE anim = static_cast<ANIM_TYPE>(readInt32());
	bool ok = false;
	for (size_t i = 0; ok == false && i < COUNTOF(validAnimations); i++)
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
PacketClientInfo::PacketClientInfo() : Packet(0)
{
}

bool PacketClientInfo::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketClientInfo::onReceive");
	UNREFERENCED_PARAMETER(net);

	skip(1); // 0x0A
	skip(4); // flags
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x10 : PacketAosTooltipInfo			tooltip request (old)
 *
 *
 ***************************************************************************/
PacketAosTooltipInfo::PacketAosTooltipInfo() : Packet(0)
{
}

bool PacketAosTooltipInfo::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketAosTooltipInfo::onReceive");
	
	CClient* client = net->m_client;
	ASSERT(client);
	const CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	const CObjBase* object = CGrayUID(readInt32()).ObjFind();
	if (object != NULL && character->CanSee(object))
		client->addAOSTooltip(object, true);

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x13 : PacketPopupReq				request popup menu
 *
 *
 ***************************************************************************/
PacketPopupReq::PacketPopupReq() : Packet(0)
{
}

bool PacketPopupReq::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketPopupReq::onReceive");

	DWORD serial = readInt32();

	CClient *client = net->m_client;
	ASSERT(client);
	client->Event_AOSPopupMenuRequest(serial);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x15 : PacketPopupSelect			popup menu option selected
 *
 *
 ***************************************************************************/
PacketPopupSelect::PacketPopupSelect() : Packet(0)
{
}

bool PacketPopupSelect::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketPopupSelect::onReceive");

	DWORD serial = readInt32();
	WORD tag = readInt16();

	CClient *client = net->m_client;
	ASSERT(client);
	client->Event_AOSPopupMenuSelect(serial, tag);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x1A : PacketChangeStatLock			set stat locks
 *
 *
 ***************************************************************************/
PacketChangeStatLock::PacketChangeStatLock() : Packet(0)
{
}

bool PacketChangeStatLock::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketChangeStatLock::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	const CChar* character = client->GetChar();
	if (character == NULL || character->m_pPlayer == NULL)
		return false;

	BYTE code = readByte();
	SKILLLOCK_TYPE state = static_cast<SKILLLOCK_TYPE>(readByte());

	if (code >= STAT_BASE_QTY)
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
		default:
			stat = STAT_NONE;
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
PacketSpellSelect::PacketSpellSelect() : Packet(0)
{
}

bool PacketSpellSelect::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketSpellSelect::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	skip(2); // unknown
	SPELL_TYPE spell = static_cast<SPELL_TYPE>(readInt16());
	if (!spell)
		 return false;

	const CSpellDef* spellDef = g_Cfg.GetSpellDef(spell);
	if (spellDef == NULL)
		return true;

	int skill;
	if (spellDef->GetPrimarySkill(&skill, NULL) == false)
		return true;
	if ( !character->Skill_CanUse(static_cast<SKILL_TYPE>(skill)) )
		return true;

	if (IsSetMagicFlags(MAGICF_PRECAST))
	{
		if (spellDef->IsSpellType(SPELLFLAG_NOPRECAST) == false)
		{
			client->m_tmSkillMagery.m_Spell = spell;
			character->m_atMagery.m_Spell = spell;
			client->m_Targ_UID = character->GetUID();
			client->m_Targ_PrvUID = character->GetUID();
			character->Skill_Start(static_cast<SKILL_TYPE>(skill));
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
PacketHouseDesignReq::PacketHouseDesignReq() : Packet(0)
{
}

bool PacketHouseDesignReq::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignReq::onReceive");

	CClient* client = net->m_client;
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
PacketAntiCheat::PacketAntiCheat() : Packet(0)
{
}

bool PacketAntiCheat::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketAntiCheat::onReceive");
	UNREFERENCED_PARAMETER(net);

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x2C : PacketBandageMacro			bandage macro
 *
 *
 ***************************************************************************/
PacketBandageMacro::PacketBandageMacro() : Packet(0)
{
}

bool PacketBandageMacro::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketBandageMacro::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	const CChar* character = client->GetChar();
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

	client->Event_Target(CLIMODE_TARG_USE_ITEM, target->GetUID(), target->GetTopPoint());
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x32 : PacketGargoyleFly			gargoyle toggle flying
 *
 *
 ***************************************************************************/
PacketGargoyleFly::PacketGargoyleFly() : Packet(0)
{
}

bool PacketGargoyleFly::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketGargoyleFly::onReceive");

	if ( !(g_Cfg.m_iRacialFlags & RACIALF_GARG_FLY) )
		return false;

	CClient* client = net->m_client;
	ASSERT(client);
	CChar* character = client->GetChar();
	if ( character == NULL || !character->IsGargoyle() || character->IsStatFlag(STATF_DEAD) )
		return false;

	// The client always send these 2 values to server, but they're not really used
	//WORD one = readInt16();
	//DWORD zero = readInt32();
	
	if ( IsTrigUsed(TRIGGER_TOGGLEFLYING) )
	{
		if ( character->OnTrigger(CTRIG_ToggleFlying,character,0) == TRIGRET_RET_TRUE )
			return false;
	}

	if ( character->IsStatFlag(STATF_Hovering) )
	{
		// stop hovering
		character->StatFlag_Clear(STATF_Hovering);
		client->removeBuff(BI_GARGOYLEFLY);
	}
	else
	{
		// begin hovering
		character->StatFlag_Set(STATF_Hovering);
		client->addBuff(BI_GARGOYLEFLY, 1112193, 1112567);

		// float player up to the hover Z
		CPointMap ptHover = g_World.FindItemTypeNearby(character->GetTopPoint(), IT_HOVEROVER, 0);
		if ( ptHover.IsValidPoint() )
			character->MoveTo(ptHover);
	}

	// Sending this packet here instead of calling UpdateAnimate because of conversions, NANIM_TAKEOFF = 9 and the function
	// is reading 9 from old ANIM_TYPE to know when the character is attacking and modifying its animation accordingly
	PacketActionBasic *cmd = new PacketActionBasic(character, character->IsStatFlag(STATF_Hovering) ? NANIM_TAKEOFF : NANIM_LANDING, static_cast<ANIM_TYPE_NEW>(0), static_cast<BYTE>(0));
	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( !pClient->m_NetState->isClientVersion(MINCLIVER_SA) )
			continue;
		if ( !pClient->CanSee(character) )
			continue;
		pClient->addCharMove(character);
		cmd->send(pClient);
	}
	delete cmd;
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x33 : PacketWheelBoatMove			use mouse as boat wheel
 *
 *
 ***************************************************************************/
PacketWheelBoatMove::PacketWheelBoatMove() : Packet(0)
{
}

bool PacketWheelBoatMove::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketWheelBoatMove::onReceive");

	//UO:HS clients >= 7.0.8f
	//base code below, cleaning needed

	CClient* client = net->m_client;
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	skip(4);
	//DWORD serial = readInt32(); //player serial
	//CGrayUID from(serial &~ UID_F_RESOURCE); //do we need this? NetState provides the player character

	DIR_TYPE facing = static_cast<DIR_TYPE>(readByte()); //new boat facing, yes client send it
	DIR_TYPE moving = static_cast<DIR_TYPE>(readByte()); //the boat movement
	//skip(1);
	BYTE speed = readByte(); //(0 = Stop Movement, 1 = One Tile Movement, 2 = Normal Movement) ***These speeds are NOT the same as 0xF6 packet

	CRegionWorld *area = character->m_pArea;
	if (area && area->IsFlag(REGION_FLAG_SHIP))
	{
		CItemShip *pShipItem = dynamic_cast<CItemShip *>(area->GetResourceID().ItemFind());
		if (pShipItem && pShipItem->m_itShip.m_Pilot == character->GetUID())
		{
			//direction of movement = moving - ship_face
			//	moving = read from packet
			//	ship_face = pShipItem->Ship_Face()

			//Ship_* need to be private? there is another way to ask the ship to move?
			//pShipItem->Ship_Move(static_cast<DIR_TYPE>((moving - pShipItem->m_itShip.m_DirFace)), pShipItem->m_shipSpeed.tiles);

			if ((facing == DIR_N || facing == DIR_E || facing == DIR_S || facing == DIR_W) && pShipItem->m_itShip.m_DirFace != facing) //boat cannot face intermediate directions
				pShipItem->Ship_Face(moving);

			if (pShipItem->Ship_SetMoveDir(facing, speed, true))//pShipItem->m_itShip.m_DirMove = static_cast<BYTE>(facing);
				pShipItem->Ship_Move(moving, speed);
		}
		else
			return false;
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
PacketPromptResponseUnicode::PacketPromptResponseUnicode() : Packet(0)
{
}

bool PacketPromptResponseUnicode::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketPromptResponseUnicode::onReceive");

	size_t length = readInt16();
	DWORD context1 = readInt32();
	DWORD context2 = readInt32();
	DWORD type = readInt32();
	TCHAR language[4];
	readStringASCII(language, COUNTOF(language));
	
	if (length < getPosition())
		return false;

	length = (length - getPosition()) / 2;
	TCHAR* text = Str_GetTemp();
	readStringUNICODE(text, THREAD_STRING_LENGTH, length+1);

	net->m_client->Event_PromptResp(text, length, context1, context2, type);
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
	ADDTOCALLSTACK("PacketViewRange::onReceive");

	CChar *character = net->m_client->GetChar();
	if ( !character )
		return false;

	BYTE iVal = readByte();
	character->SetSight(iVal);
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
	ADDTOCALLSTACK("PacketLogout::onReceive");

	new PacketLogoutAck(net->m_client);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD4 : PacketBookHeaderEditNew	edit book header (title/author)
 *
 *
 ***************************************************************************/
PacketBookHeaderEditNew::PacketBookHeaderEditNew() : Packet(0)
{
}

bool PacketBookHeaderEditNew::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketBookHeaderEditNew::onReceive");

	skip(2); // length
	CGrayUID bookSerial(readInt32());
	skip(1); // unknown
	skip(1); // writable
	skip(2); // pages

	TCHAR title[2 * MAX_NAME_SIZE];
	TCHAR author[MAX_NAME_SIZE];

	size_t titleLength = readInt16();
	readStringASCII(title, minimum(titleLength, COUNTOF(title)));

	size_t authorLength = readInt16();
	readStringASCII(author, minimum(authorLength, COUNTOF(author)));

	net->m_client->Event_Book_Title(bookSerial, title, author);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD6 : PacketAOSTooltipReq				aos tooltip request
 *
 *
 ***************************************************************************/
PacketAOSTooltipReq::PacketAOSTooltipReq() : Packet(0)
{
}

bool PacketAOSTooltipReq::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketAOSTooltipReq::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	const CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	const CObjBase* object;
	for (WORD length = readInt16(); length > sizeof(DWORD); length -= sizeof(DWORD))
	{
		object = CGrayUID(readInt32()).ObjFind();
		if (object == NULL)
			continue;
		else if (character->CanSee(object) == false)
			continue;

		client->addAOSTooltip(object, true);
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
PacketEncodedCommand::PacketEncodedCommand() : Packet(0)
{
}

bool PacketEncodedCommand::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketEncodedCommand::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);

	const CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	WORD packetLength = readInt16();
	CGrayUID serial(readInt32());
	if (character->GetUID() != serial)
		return false;

	EXTAOS_TYPE type = static_cast<EXTAOS_TYPE>(readInt16());
	seek();
	

#ifndef _MTNETWORK
	Packet* handler = g_NetworkIn.getPacketManager().getEncodedHandler(type);
#else
	Packet* handler = g_NetworkManager.getPacketManager().getEncodedHandler(type);
#endif
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
PacketHouseDesignBackup::PacketHouseDesignBackup() : Packet(0)
{
}

bool PacketHouseDesignBackup::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignBackup::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	house->BackupStructure();
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x03 : PacketHouseDesignRestore		restore house design
 *
 *
 ***************************************************************************/
PacketHouseDesignRestore::PacketHouseDesignRestore() : Packet(0)
{
}

bool PacketHouseDesignRestore::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignRestore::onReceive");

	CClient* client = net->m_client;
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
PacketHouseDesignCommit::PacketHouseDesignCommit() : Packet(0)
{
}

bool PacketHouseDesignCommit::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignCommit::onReceive");

	CClient* client = net->m_client;
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
PacketHouseDesignDestroyItem::PacketHouseDesignDestroyItem() : Packet(0)
{
}

bool PacketHouseDesignDestroyItem::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignDestroyItem::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	skip(1); // 0x00
	ITEMID_TYPE id = static_cast<ITEMID_TYPE>(readInt32());
	skip(1); // 0x00
	WORD x = static_cast<WORD>(readInt32());
	skip(1); // 0x00
	WORD y = static_cast<WORD>(readInt32());
	skip(1); // 0x00
	WORD z = static_cast<WORD>(readInt32());

	house->RemoveItem(client, id, x, y, static_cast<signed char>(z));
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x06 : PacketHouseDesignPlaceItem	place house design item
 *
 *
 ***************************************************************************/
PacketHouseDesignPlaceItem::PacketHouseDesignPlaceItem() : Packet(0)
{
}

bool PacketHouseDesignPlaceItem::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignPlaceItem::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	skip(1); // 0x00
	ITEMID_TYPE id = static_cast<ITEMID_TYPE>(readInt32());
	skip(1); // 0x00
	WORD x = static_cast<WORD>(readInt32());
	skip(1); // 0x00
	WORD y = static_cast<WORD>(readInt32());

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
PacketHouseDesignExit::PacketHouseDesignExit() : Packet(0)
{
}

bool PacketHouseDesignExit::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignExit::onReceive");

	CClient* client = net->m_client;
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
PacketHouseDesignPlaceStair::PacketHouseDesignPlaceStair() : Packet(0)
{
}

bool PacketHouseDesignPlaceStair::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignPlaceStair::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	skip(1); // 0x00
	ITEMID_TYPE id = static_cast<ITEMID_TYPE>(readInt32() + ITEMID_MULTI);
	skip(1); // 0x00
	WORD x = static_cast<WORD>(readInt32());
	skip(1); // 0x00
	WORD y = static_cast<WORD>(readInt32());

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
PacketHouseDesignSync::PacketHouseDesignSync() : Packet(0)
{
}

bool PacketHouseDesignSync::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignSync::onReceive");

	CClient* client = net->m_client;
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
PacketHouseDesignClear::PacketHouseDesignClear() : Packet(0)
{
}

bool PacketHouseDesignClear::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignClear::onReceive");

	CClient* client = net->m_client;
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
PacketHouseDesignSwitch::PacketHouseDesignSwitch() : Packet(0)
{
}

bool PacketHouseDesignSwitch::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignSwitch::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	skip(1); // 0x00
	DWORD level = readInt32();

	house->SwitchToLevel(client, static_cast<unsigned char>(level));
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x13 : PacketHouseDesignPlaceRoof	place house design roof
 *
 *
 ***************************************************************************/
PacketHouseDesignPlaceRoof::PacketHouseDesignPlaceRoof() : Packet(0)
{
}

bool PacketHouseDesignPlaceRoof::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignPlaceRoof::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	skip(1); // 0x00
	ITEMID_TYPE id = static_cast<ITEMID_TYPE>(readInt32());
	skip(1); // 0x00
	WORD x = static_cast<WORD>(readInt32());
	skip(1); // 0x00
	WORD y = static_cast<WORD>(readInt32());
	skip(1); // 0x00
	WORD z = static_cast<WORD>(readInt32());

	house->AddRoof(client, id, x, y, static_cast<signed char>(z));
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x14 : PacketHouseDesignDestroyRoof	destroy house design roof
 *
 *
 ***************************************************************************/
PacketHouseDesignDestroyRoof::PacketHouseDesignDestroyRoof() : Packet(0)
{
}

bool PacketHouseDesignDestroyRoof::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignDestroyRoof::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == NULL)
		return true;

	skip(1); // 0x00
	ITEMID_TYPE id = static_cast<ITEMID_TYPE>(readInt32());
	skip(1); // 0x00
	WORD x = static_cast<WORD>(readInt32());
	skip(1); // 0x00
	WORD y = static_cast<WORD>(readInt32());
	skip(1); // 0x00
	WORD z = static_cast<WORD>(readInt32());

	house->RemoveRoof(client, id, x, y, static_cast<signed char>(z));
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x19 : PacketSpecialMove			perform special move
 *
 *
 ***************************************************************************/
PacketSpecialMove::PacketSpecialMove() : Packet(0)
{
}

bool PacketSpecialMove::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketSpecialMove::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	skip(1);
	DWORD ability = readInt32();

	if ( IsTrigUsed(TRIGGER_USERSPECIALMOVE) )
	{
		CScriptTriggerArgs args;
		args.m_iN1 = ability;
		character->OnTrigger(CTRIG_UserSpecialMove, character, &args);
	}
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x1A : PacketHouseDesignRevert		revert house design
 *
 *
 ***************************************************************************/
PacketHouseDesignRevert::PacketHouseDesignRevert() : Packet(0)
{
}

bool PacketHouseDesignRevert::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignRevert::onReceive");

	CClient* client = net->m_client;
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
PacketEquipLastWeapon::PacketEquipLastWeapon() : Packet(0)
{
}

bool PacketEquipLastWeapon::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketEquipLastWeapon::onReceive");
	UNREFERENCED_PARAMETER(net);

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x28 : PacketGuildButton			guild button pressed
 *
 *
 ***************************************************************************/
PacketGuildButton::PacketGuildButton() : Packet(0)
{
}

bool PacketGuildButton::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketGuildButton::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	if ( IsTrigUsed(TRIGGER_USERGUILDBUTTON) )
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
PacketQuestButton::PacketQuestButton() : Packet(0)
{
}

bool PacketQuestButton::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketQuestButton::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	if ( IsTrigUsed(TRIGGER_USERQUESTBUTTON) )
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
	ADDTOCALLSTACK("PacketHardwareInfo::onReceive");
	UNREFERENCED_PARAMETER(net);

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
PacketBugReport::PacketBugReport() : Packet(0)
{
}

bool PacketBugReport::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketBugReport::onReceive");

	WORD packetLength = readInt16(); // packet length
	if (packetLength < 10)
		return false;

	TCHAR language[4];
	readStringASCII(language, COUNTOF(language));

	BUGREPORT_TYPE type = static_cast<BUGREPORT_TYPE>(readInt16());

	TCHAR text[MAX_TALK_BUFFER];
	int textLength = readStringNullNUNICODE(text, MAX_TALK_BUFFER, MAX_TALK_BUFFER-1);

	net->m_client->Event_BugReport(text, textLength, type, CLanguageID(language));
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xE1 : PacketClientType		client type (KR/SA)
 *
 *
 ***************************************************************************/
PacketClientType::PacketClientType() : Packet(0)
{
}

bool PacketClientType::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketClientType::onReceive");

	WORD packetLength = readInt16(); // packet length
	if (packetLength < 9)
		return false;

	skip(2); // ..count?
	GAMECLIENT_TYPE type = static_cast<GAMECLIENT_TYPE>(readInt32());

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
	ADDTOCALLSTACK("PacketRemoveUIHighlight::onReceive");
	UNREFERENCED_PARAMETER(net);

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
	ADDTOCALLSTACK("PacketUseHotbar::onReceive");

	CClient* client = net->m_client;
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
 *	Packet 0xEC : PacketEquipItemMacro				equip item(s) macro (KR)
 *
 *
 ***************************************************************************/
PacketEquipItemMacro::PacketEquipItemMacro() : Packet(0)
{
}

bool PacketEquipItemMacro::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketEquipItemMacro::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	skip(2); // packet length
	BYTE itemCount = readByte();
	if ( itemCount > 3 )	// prevent packet exploit sending fake values just to create heavy loops and overload server CPU
		itemCount = 3;

	CItem* item;
	for (BYTE i = 0; i < itemCount; i++)
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
 *	Packet 0xED : PacketUnEquipItemMacro			unequip item(s) macro (KR)
 *
 *
 ***************************************************************************/
PacketUnEquipItemMacro::PacketUnEquipItemMacro() : Packet(0)
{
}

bool PacketUnEquipItemMacro::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketUnEquipItemMacro::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	skip(2); // packet length
	BYTE itemCount = readByte();
	if ( itemCount > 3 )	// prevent packet exploit sending fake values just to create heavy loops and overload server CPU
		itemCount = 3;

	LAYER_TYPE layer;
	CItem* item;
	for (BYTE i = 0; i < itemCount; i++)
	{
		layer = static_cast<LAYER_TYPE>(readInt16());

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
 *	Packet 0xF0 : PacketMovementReqNew			new movement request (KR/SA)
 *
 *
 ***************************************************************************/
PacketMovementReqNew::PacketMovementReqNew() : Packet(0)
{
}

bool PacketMovementReqNew::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketMovementReqNew::onReceive");
	// New walk packet used on KR/SA clients (still incomplete)
	// It must be enabled using login flags on packet 0xA9, otherwise the client will
	// stay using the old walk packet 0x02.
	// PS: Strangely some encrypted clients always use this packet even without receive
	// login flags to enable it. This doesn't happen on no-crypt clients.

	// The 'time' values here are used by fastwalk prevention, and linked somehow to
	// time sync packets 0xF1 (client request) / 0xF2 (server response) but I have no
	// idea how it works. The client request an time resync at every 60 seconds.
	// Anyway, these values are not in use because Sphere already have another fastwalk
	// detection engine from the old packet 0x02 (PacketMovementReq).

	// PS: On classic clients this packet is used as 'Krrios special client' (?) which
	// does some useless weird stuff. Also classic clients using Injection 2014 will
	// strangely send this packet to server when the player press the 'Chat' button,
	// so it's better leave this packet disabled on classic clients to prevent exploits.

	if ( !(g_Cfg.m_iFeatureSA & FEATURE_SA_MOVEMENT) )
		return false;

	CClient *client = net->m_client;
	ASSERT(client);

	skip(2);
	BYTE steps = readByte();
	while ( steps )
	{
		skip(8);	//INT64 iTime1 = readInt64();
		skip(8);	//INT64 iTime2 = readInt64();
		BYTE sequence = readByte();
		BYTE direction = readByte();
		DWORD mode = readInt32();	// 1 = walk, 2 = run
		if ( mode == 2 )
			direction |= 0x80;

		// The client send these values, but they're not really needed
		//DWORD x = readInt32();
		//DWORD y = readInt32();
		//DWORD z = readInt32();
		skip(12);

		if ( !client->Event_Walk(direction, sequence) )
		{
			net->m_sequence = 0;
			break;
		}

		steps--;
	}
	return true;
}

/***************************************************************************
 *
 *
 *	Packet 0xF1 : PacketTimeSyncRequest				time sync request (KR/SA)
 *
 *
 ***************************************************************************/
PacketTimeSyncRequest::PacketTimeSyncRequest() : Packet(9)
{
}

bool PacketTimeSyncRequest::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketTimeSyncRequest::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);

	//INT64 iTime = readInt64();	// what we must do with this value?
	new PacketTimeSyncResponse(client);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xF4 : PacketCrashReport					crash report
 *
 *
 ***************************************************************************/
PacketCrashReport::PacketCrashReport() : Packet(0)
{
}

bool PacketCrashReport::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketCrashReport::onReceive");

	skip(2); // packet length
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
	skip(15); // ip address
	skip(4); // unknown
	DWORD errorCode = readInt32();
	TCHAR executable[100];
	readStringASCII(executable, COUNTOF(executable));
	TCHAR description[100];
	readStringASCII(description, COUNTOF(description));
	skip(1); // zero
	DWORD errorOffset = readInt32();

	g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "%lx:Client crashed at %d,%d,%d,%d: 0x%08lX %s @ 0x%08lX (%s, %d.%d.%d.%d)\n", net->id(),
					x, y, z, map,
					errorCode, description, errorOffset, executable,
					versionMaj, versionMin, versionRev, versionPat);
	return true;
}

/***************************************************************************
 *
 *
 *	Packet 0xF8 : PacketCreateHS					create new character request (HS)
 *
 *
 ***************************************************************************/
PacketCreateHS::PacketCreateHS() : PacketCreate(106)
{
}

bool PacketCreateHS::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketCreateHS::onReceive");

	// standard character creation packet.. with 4 skills
	return PacketCreate::onReceive(net, true);
}

