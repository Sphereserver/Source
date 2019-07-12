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

	return doCreate(net, charname, isFemale, rtRace, strength, dexterity, intelligence, prof, skill1, skillval1, skill2, skillval2, skill3, skillval3, skill4, skillval4, hue, hairid, hairhue, beardid, beardhue, shirthue, pantshue, ITEMID_NOTHING, startloc, flags);
}

bool PacketCreate::doCreate(NetState *net, LPCTSTR pszName, bool fFemale, RACE_TYPE race, BYTE bStr, BYTE bDex, BYTE bInt, PROFESSION_TYPE profession, SKILL_TYPE skill1, BYTE bSkillVal1, SKILL_TYPE skill2, BYTE bSkillVal2, SKILL_TYPE skill3, BYTE bSkillVal3, SKILL_TYPE skill4, BYTE bSkillVal4, HUE_TYPE hueSkin, ITEMID_TYPE idHair, HUE_TYPE hueHair, ITEMID_TYPE idBeard, HUE_TYPE hueBeard, HUE_TYPE hueShirt, HUE_TYPE huePants, ITEMID_TYPE idFace, BYTE bStartLoc, DWORD dwFlags)
{
	ADDTOCALLSTACK("PacketCreate::doCreate");

	CClient *client = net->m_client;
	ASSERT(client);
	CAccount *account = client->m_pAccount;
	ASSERT(account);
	RESDISPLAY_VERSION resdisp = static_cast<RESDISPLAY_VERSION>(account->GetResDisp());

	// Check if the account is already connected using another character
	if ( client->GetChar() )
	{
		client->addSysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_ALREADYONLINE));
		g_Log.Event(LOGM_CLIENTS_LOG, "%lx:Account '%s' already in use\n", net->id(), account->GetName());
		return false;
	}

	// Check if the account have an idling character
	const CChar *pCharLast = account->m_uidLastChar.CharFind();
	if ( pCharLast && account->IsMyAccountChar(pCharLast) && !pCharLast->IsDisconnected() )
	{
		client->addIdleWarning(PacketWarningMessage::CharacterInWorld);
		client->addLoginErr(PacketLoginError::CharIdle);
		return false;
	}

	// Validate free character slots
	size_t iChars = account->m_Chars.GetCharCount();
	BYTE iMaxChars = account->GetMaxChars();
	if ( iChars >= iMaxChars )
	{
		client->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_MAXCHARS), static_cast<int>(iChars));
		client->addLoginErr(PacketLoginError::TooManyChars);
		return false;
	}

	// Validate all info sent by client to prevent exploits
	if ( !strlen(pszName) || g_Cfg.IsObscene(pszName) || Str_CheckName(pszName) || !strnicmp(pszName, "lord ", 5) || !strnicmp(pszName, "lady ", 5) || !strnicmp(pszName, "counselor ", 10) || !strnicmp(pszName, "seer ", 5) || !strnicmp(pszName, "gm ", 3) || !strnicmp(pszName, "admin ", 6) )
		goto InvalidInfo;

	if ( (bStr > 60) || (bDex > 60) || (bInt > 60) )
		goto InvalidInfo;
	if ( (bSkillVal1 > 50) || (bSkillVal2 > 50) || (bSkillVal3 > 50) || (bSkillVal4 > 50) )
		goto InvalidInfo;
	if ( skill4 != SKILL_NONE )
	{
		if ( bStr + bDex + bInt > 90 )
			goto InvalidInfo;
		if ( bSkillVal1 + bSkillVal2 + bSkillVal3 + bSkillVal4 > 120 )
			goto InvalidInfo;
	}
	else
	{
		if ( bStr + bDex + bInt > 80 )
			goto InvalidInfo;
		if ( bSkillVal1 + bSkillVal2 + bSkillVal3 > 100 )
			goto InvalidInfo;
	}

	if ( (resdisp < RDS_AOS) || (!(g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_A)) )
	{
		if ( (profession == PROFESSION_NECROMANCER) || (profession == PROFESSION_PALADIN) )
			goto InvalidInfo;
	}
	if ( (resdisp < RDS_SE) || (!(g_Cfg.m_iFeatureSE & FEATURE_SE_UPDATE)) )
	{
		if ( (profession == PROFESSION_SAMURAI) || (profession == PROFESSION_NINJA) )
			goto InvalidInfo;
	}
	if ( (resdisp < RDS_ML) || (!(g_Cfg.m_iFeatureML & FEATURE_ML_UPDATE)) )
	{
		if ( race == RACETYPE_ELF )
			goto InvalidInfo;
	}
	if ( (resdisp < RDS_SA) || (!(g_Cfg.m_iFeatureSA & FEATURE_SA_UPDATE)) )
	{
		if ( race == RACETYPE_GARGOYLE )
			goto InvalidInfo;
	}

	switch ( race )
	{
		default:
		case RACETYPE_HUMAN:
		{
			if ( (hueSkin < HUE_SKIN_LOW) || (hueSkin > HUE_SKIN_HIGH) )
				goto InvalidInfo;

			if ( idHair )
			{
				if ( !(((idHair >= ITEMID_HAIR_SHORT) && (idHair <= ITEMID_HAIR_PONYTAIL)) || ((idHair >= ITEMID_HAIR_MOHAWK) && (idHair <= ITEMID_HAIR_TOPKNOT))) )
					goto InvalidInfo;
				if ( fFemale && (idHair == ITEMID_HAIR_RECEDING) )
					goto InvalidInfo;
				if ( !fFemale && (idHair == ITEMID_HAIR_BUNS) )
					goto InvalidInfo;

				if ( (hueHair < HUE_HAIR_LOW) || (hueHair > HUE_HAIR_HIGH) )
					goto InvalidInfo;
			}

			if ( idBeard )
			{
				if ( fFemale )
					goto InvalidInfo;
				if ( !(((idBeard >= ITEMID_BEARD_LONG) && (idBeard <= ITEMID_BEARD_MUST)) || ((idBeard >= ITEMID_BEARD_SHORT_MUST) && (idBeard <= ITEMID_BEARD_VANDYKE))) )
					goto InvalidInfo;

				if ( (hueBeard < HUE_HAIR_LOW) || (hueBeard > HUE_HAIR_HIGH) )
					goto InvalidInfo;
			}

			if ( idFace )
			{
				if ( !(((idFace >= ITEMID_FACE_1) && (idFace <= ITEMID_FACE_10)) || ((idFace >= ITEMID_FACE_ANIME) && (idFace <= ITEMID_FACE_VAMPIRE))) )
					goto InvalidInfo;
			}
			break;
		}
		case RACETYPE_ELF:
		{
			const HUE_TYPE sm_ElfSkinHues[] = { 0xBF, 0x24D, 0x24E, 0x24F, 0x353, 0x361, 0x367, 0x374, 0x375, 0x376, 0x381, 0x382, 0x383, 0x384, 0x385, 0x389, 0x3DE, 0x3E5, 0x3E6, 0x3E8, 0x3E9, 0x430, 0x4A7, 0x4DE, 0x51D, 0x53F, 0x579, 0x76B, 0x76C, 0x76D, 0x835, 0x903 };
			if ( !isValidHue(hueSkin, sm_ElfSkinHues, COUNTOF(sm_ElfSkinHues)) )
				goto InvalidInfo;

			if ( idHair )
			{
				if ( !(((idHair >= ITEMID_HAIR_MID_LONG) && (idHair <= ITEMID_HAIR_MULLET)) || ((idHair >= ITEMID_HAIR_FLOWER) && (idHair <= ITEMID_HAIR_SPIKED))) )
					goto InvalidInfo;
				if ( fFemale && ((idHair == ITEMID_HAIR_MID_LONG) || (idHair == ITEMID_HAIR_LONG_ELF)) )
					goto InvalidInfo;
				if ( !fFemale && ((idHair == ITEMID_HAIR_FLOWER) || (idHair == ITEMID_HAIR_BUNS_ELF)) )
					goto InvalidInfo;

				const HUE_TYPE sm_ElfHairHues[] = { 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x58, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x101, 0x159, 0x15A, 0x15B, 0x15C, 0x15D, 0x15E, 0x128, 0x12F, 0x1BD, 0x1E4, 0x1F3, 0x207, 0x211, 0x239, 0x251, 0x26C, 0x2C3, 0x2C9, 0x31D, 0x31E, 0x31F, 0x320, 0x321, 0x322, 0x323, 0x324, 0x325, 0x326, 0x369, 0x386, 0x387, 0x388, 0x389, 0x38A, 0x59D, 0x6B8, 0x725, 0x853 };
				if ( !isValidHue(hueHair, sm_ElfHairHues, COUNTOF(sm_ElfHairHues)) )
					goto InvalidInfo;
			}

			if ( idBeard )
				goto InvalidInfo;

			if ( idFace )
			{
				if ( !(((idFace >= ITEMID_FACE_1) && (idFace <= ITEMID_FACE_10)) || ((idFace >= ITEMID_FACE_ANIME) && (idFace <= ITEMID_FACE_VAMPIRE))) )
					goto InvalidInfo;
			}
			break;
		}
		case RACETYPE_GARGOYLE:
		{
			if ( (hueSkin < HUE_GARGSKIN_LOW) || (hueSkin > HUE_GARGSKIN_HIGH) )
				goto InvalidInfo;

			if ( idHair )
			{
				if ( fFemale && !((idHair == ITEMID_GARG_HORN_FEMALE_1) || (idHair == ITEMID_GARG_HORN_FEMALE_2) || ((idHair >= ITEMID_GARG_HORN_FEMALE_3) && (idHair <= ITEMID_GARG_HORN_FEMALE_5)) || (idHair == ITEMID_GARG_HORN_FEMALE_6) || (idHair == ITEMID_GARG_HORN_FEMALE_7) || (idHair == ITEMID_GARG_HORN_FEMALE_8)) )
					goto InvalidInfo;
				if ( !fFemale && !((idHair >= ITEMID_GARG_HORN_1) && (idHair <= ITEMID_GARG_HORN_8)) )
					goto InvalidInfo;

				const HUE_TYPE sm_GargoyleHornHues[] = { 0x709, 0x70B, 0x70D, 0x70F, 0x711, 0x763, 0x765, 0x768, 0x76B, 0x6F3, 0x6F1, 0x6EF, 0x6E4, 0x6E2, 0x6E0, 0x709, 0x70B, 0x70D };
				if ( !isValidHue(hueHair, sm_GargoyleHornHues, COUNTOF(sm_GargoyleHornHues)) )
					goto InvalidInfo;
			}

			if ( idBeard )
			{
				if ( fFemale )
					goto InvalidInfo;
				if ( !((idBeard >= ITEMID_GARG_HORN_FACIAL_1) && (idBeard <= ITEMID_GARG_HORN_FACIAL_4)) )
					goto InvalidInfo;

				const HUE_TYPE sm_GargoyleFacialHornHues[] = { 0x709, 0x70B, 0x70D, 0x70F, 0x711, 0x763, 0x765, 0x768, 0x76B, 0x6F3, 0x6F1, 0x6EF, 0x6E4, 0x6E2, 0x6E0, 0x709, 0x70B, 0x70D };
				if ( !isValidHue(hueBeard, sm_GargoyleFacialHornHues, COUNTOF(sm_GargoyleFacialHornHues)) )
					goto InvalidInfo;
			}

			if ( idFace )
			{
				if ( !((idFace >= ITEMID_FACE_1_GARG) && (idFace <= ITEMID_FACE_6_GARG)) )
					goto InvalidInfo;
			}
			break;
		}
	}

	if ( !(net->isClientKR() || net->isClientEnhanced()) )
	{
		if ( (hueShirt < HUE_BLUE_LOW) || (hueShirt > HUE_DYE_HIGH) )
			goto InvalidInfo;
		if ( (huePants < HUE_BLUE_LOW) || (huePants > HUE_DYE_HIGH) )
			goto InvalidInfo;
	}

	if ( 0 )
	{
	InvalidInfo:
		client->addLoginErr(PacketLoginError::CreationBlocked);
		return false;
	}

	// Create the character
	CChar *pChar = CChar::CreateBasic(CREID_MAN);
	ASSERT(pChar);
	pChar->SetName(pszName);
	pChar->SetPlayerAccount(account);
	pChar->SetHue(hueSkin|HUE_MASK_UNDERWEAR);

	switch ( race )
	{
		default:
		case RACETYPE_HUMAN:
			pChar->SetID(fFemale ? CREID_WOMAN : CREID_MAN);
			break;
		case RACETYPE_ELF:
			pChar->SetID(fFemale ? CREID_ELFWOMAN : CREID_ELFMAN);
			break;
		case RACETYPE_GARGOYLE:
			pChar->SetID(fFemale ? CREID_GARGWOMAN : CREID_GARGMAN);
			break;
	}

	// Set starting position
	if ( g_Cfg.m_StartDefs.IsValidIndex(bStartLoc) )
		pChar->m_ptHome = g_Cfg.m_StartDefs[bStartLoc]->m_pt;
	else
		pChar->m_ptHome.InitPoint();

	if ( !pChar->m_ptHome.IsValidPoint() )
	{
		g_Log.Event(LOGM_CLIENTS_LOG, "%lx:Account '%s' can't create new character because server has invalid START location '%hhu'\n", net->id(), account->GetName(), bStartLoc);
		pChar->Delete();
		goto InvalidInfo;
	}

	pChar->SetUnkPoint(pChar->m_ptHome);	// don't put me in the world yet

	// Set stats
	pChar->Stat_SetBase(STAT_STR, bStr);
	pChar->Stat_SetBase(STAT_DEX, bDex);
	pChar->Stat_SetBase(STAT_INT, bInt);
	pChar->CreateNewCharCheck();

	// Set skills
	for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; ++i )
	{
		if ( g_Cfg.m_SkillIndexDefs.IsValidIndex(i) )
			pChar->Skill_SetBase(static_cast<SKILL_TYPE>(i), static_cast<WORD>(Calc_GetRandVal(g_Cfg.m_iMaxBaseSkill)));
	}
	if ( pChar->IsSkillBase(skill1) && g_Cfg.m_SkillIndexDefs.IsValidIndex(skill1) )
		pChar->Skill_SetBase(skill1, bSkillVal1 * 10);
	if ( pChar->IsSkillBase(skill2) && g_Cfg.m_SkillIndexDefs.IsValidIndex(skill2) )
		pChar->Skill_SetBase(skill2, bSkillVal2 * 10);
	if ( pChar->IsSkillBase(skill3) && g_Cfg.m_SkillIndexDefs.IsValidIndex(skill3) )
		pChar->Skill_SetBase(skill3, bSkillVal3 * 10);
	if ( pChar->IsSkillBase(skill4) && g_Cfg.m_SkillIndexDefs.IsValidIndex(skill4) )
		pChar->Skill_SetBase(skill4, bSkillVal4 * 10);

	// Create basic resources
	pChar->GetContainerCreate(LAYER_PACK);
	pChar->GetContainerCreate(LAYER_BANKBOX);

	if ( idHair )
	{
		CItem *pHair = CItem::CreateScript(idHair, pChar);
		ASSERT(pHair);
		if ( pHair->IsType(IT_HAIR) )
		{
			pHair->SetHue(hueHair);
			pHair->SetAttr(ATTR_NEWBIE|ATTR_MOVE_NEVER);
			pChar->LayerAdd(pHair);
		}
		else
			pHair->Delete();
	}
	if ( idBeard )
	{
		CItem *pBeard = CItem::CreateScript(idBeard, pChar);
		ASSERT(pBeard);
		if ( pBeard->IsType(IT_BEARD) )
		{
			pBeard->SetHue(hueBeard);
			pBeard->SetAttr(ATTR_NEWBIE|ATTR_MOVE_NEVER);
			pChar->LayerAdd(pBeard);
		}
		else
			pBeard->Delete();
	}
	if ( idFace )
	{
		CItem *pFace = CItem::CreateScript(idFace, pChar);
		ASSERT(pFace);
		pFace->SetHue(hueSkin);
		pFace->SetAttr(ATTR_NEWBIE|ATTR_MOVE_NEVER);
		pChar->LayerAdd(pFace);
	}

	// Get starting items for the profession / skills
	int iProfession = INT_MAX;
	bool fCreateSkillItems = true;
	switch ( profession )
	{
		case PROFESSION_ADVANCED:
			iProfession = RES_NEWBIE_PROF_ADVANCED;
			break;
		case PROFESSION_WARRIOR:
			iProfession = RES_NEWBIE_PROF_WARRIOR;
			break;
		case PROFESSION_MAGE:
			iProfession = RES_NEWBIE_PROF_MAGE;
			break;
		case PROFESSION_BLACKSMITH:
			iProfession = RES_NEWBIE_PROF_BLACKSMITH;
			break;
		case PROFESSION_NECROMANCER:
			iProfession = RES_NEWBIE_PROF_NECROMANCER;
			fCreateSkillItems = false;
			break;
		case PROFESSION_PALADIN:
			iProfession = RES_NEWBIE_PROF_PALADIN;
			fCreateSkillItems = false;
			break;
		case PROFESSION_SAMURAI:
			iProfession = RES_NEWBIE_PROF_SAMURAI;
			fCreateSkillItems = false;
			break;
		case PROFESSION_NINJA:
			iProfession = RES_NEWBIE_PROF_NINJA;
			fCreateSkillItems = false;
			break;
	}

	CResourceLock s;
	if ( g_Cfg.ResourceLock(s, RESOURCE_ID(RES_NEWBIE, fFemale ? RES_NEWBIE_FEMALE_DEFAULT : RES_NEWBIE_MALE_DEFAULT, race)) )
		pChar->ReadScript(s);
	else if ( g_Cfg.ResourceLock(s, RESOURCE_ID(RES_NEWBIE, fFemale ? RES_NEWBIE_FEMALE_DEFAULT : RES_NEWBIE_MALE_DEFAULT)) )
		pChar->ReadScript(s);

	CItem *pLayer = pChar->LayerFind(LAYER_SHIRT);
	if ( pLayer )
		pLayer->SetHue(hueShirt);

	pLayer = fFemale ? pChar->LayerFind(LAYER_SKIRT) : pChar->LayerFind(LAYER_PANTS);
	if ( pLayer )
		pLayer->SetHue(huePants);

	if ( g_Cfg.ResourceLock(s, RESOURCE_ID(RES_NEWBIE, iProfession, race)) )
		pChar->ReadScript(s);
	else if ( g_Cfg.ResourceLock(s, RESOURCE_ID(RES_NEWBIE, iProfession)) )
		pChar->ReadScript(s);

	if ( fCreateSkillItems )
	{
		for ( BYTE i = 1; i < 5; ++i )
		{
			int iSkill = INT_MAX;
			switch ( i )
			{
				case 1:
					iSkill = skill1;
					break;
				case 2:
					iSkill = skill2;
					break;
				case 3:
					iSkill = skill3;
					break;
				case 4:
					iSkill = skill4;
					break;
			}

			if ( g_Cfg.ResourceLock(s, RESOURCE_ID(RES_NEWBIE, iSkill, race)) )
				pChar->ReadScript(s);
			else if ( g_Cfg.ResourceLock(s, RESOURCE_ID(RES_NEWBIE, iSkill)) )
				pChar->ReadScript(s);
		}
	}
	s.Close();

	// Call triggers
	CScriptTriggerArgs createArgs;
	createArgs.m_iN1 = dwFlags;
	createArgs.m_iN2 = profession;
	createArgs.m_iN3 = race;
	createArgs.m_pO1 = client;
	createArgs.m_s1 = account->GetName();

	TRIGRET_TYPE tr;
	client->r_Call("f_onchar_create", pChar, &createArgs, NULL, &tr);
	if ( tr == TRIGRET_RET_TRUE )
	{
		pChar->Delete();
		goto InvalidInfo;
	}

	if ( IsTrigUsed(TRIGGER_RENAME) )
	{
		CScriptTriggerArgs args;
		args.m_s1 = pszName;
		args.m_pO1 = pChar;
		if ( pChar->OnTrigger(CTRIG_Rename, pChar, &args) == TRIGRET_RET_TRUE )
		{
			client->addIdleWarning(PacketWarningMessage::InvalidName);
			pChar->Delete();
			return false;
		}
	}

	g_Log.Event(LOGM_CLIENTS_LOG, "%lx:Account '%s' created new char '%s' [0%lx]\n", net->id(), account->GetName(), pChar->GetName(), static_cast<DWORD>(pChar->GetUID()));
	client->Setup_Start(pChar);
	return true;
}

bool PacketCreate::isValidHue(HUE_TYPE hue, const HUE_TYPE sm_Array[], size_t iArraySize)
{
	for ( size_t i = 0; i < iArraySize; ++i )
	{
		if ( sm_Array[i] == hue )
			return true;
	}
	return false;
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

	if ( (net->m_sequence == 0) && (sequence != 0) )
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
	if (!client->GetChar())
		return false;

	size_t packetLength = readInt16();
	TALKMODE_TYPE mode = static_cast<TALKMODE_TYPE>(readByte());
	HUE_TYPE hue = static_cast<HUE_TYPE>(readInt16());
	skip(2); // font

	if (packetLength <= getPosition())
		return false;

	packetLength -= getPosition();
	if (packetLength > MAX_TALK_BUFFER / 2)
		return false;

	TCHAR text[MAX_TALK_BUFFER / 2];
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

	CGrayUID uid = static_cast<CGrayUID>(readInt32());

	CClient* client = net->m_client;
	ASSERT(client);
	client->Event_Attack(uid);
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

	CClient *client = net->m_client;
	ASSERT(client);
	DWORD dwUID = readInt32();

	CGrayUID uid = static_cast<CGrayUID>(dwUID &~ UID_F_RESOURCE);
	bool fMacro = ((dwUID & UID_F_RESOURCE) == UID_F_RESOURCE);

	client->Event_DoubleClick(uid, fMacro, true);
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

	CGrayUID uid = static_cast<CGrayUID>(readInt32());
	WORD amount = readInt16();
	
	CClient* client = net->m_client;
	ASSERT(client);
	client->Event_Item_Pickup(uid, amount);
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

	CItem *pItem = static_cast<CGrayUID>(readInt32()).ItemFind();
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

	CGrayUID uidContainer = static_cast<CGrayUID>(readInt32());
	CPointMap pt(x, y, z, character->GetTopMap());

	client->Event_Item_Drop(pItem, pt, uidContainer, grid);
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

	CGrayUID uid = static_cast<CGrayUID>(readInt32());

	CClient* client = net->m_client;
	ASSERT(client);
	client->Event_SingleClick(uid);
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
	TCHAR name[MAX_NAME_SIZE];
	readStringNullASCII(name, MAX_NAME_SIZE - 1);

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

	CGrayUID uidItem = static_cast<CGrayUID>(readInt32());
	LAYER_TYPE layer = static_cast<LAYER_TYPE>(readByte());
	CGrayUID uidChar = static_cast<CGrayUID>(readInt32());

	CChar* source = client->GetChar();
	if (!source)
		return false;

	CItem* item = source->LayerFind(LAYER_DRAGGING);
	if ( !item || (client->GetTargMode() != CLIMODE_DRAG) || (item->GetUID() != uidItem) )
	{
		// I have no idea why i got here.
		new PacketDragCancel(client, PacketDragCancel::Other);
		return true;
	}

	client->ClearTargMode(); // done dragging.

	CChar *pChar = uidChar.CharFind();
	bool bCanCarry = (pChar && pChar->CanCarry(item));
	if ( !pChar || (layer >= LAYER_HORSE) || !pChar->NPC_IsOwnedBy(source) || !bCanCarry || !pChar->ItemEquip(item, source) )
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

bool PacketDeathStatus::onReceive(NetState* net)
{
	ADDTOCALLSTACK("PacketDeathStatus::onReceive");

	CClient* client = net->m_client;
	ASSERT(client);
	CChar* ghost = client->GetChar();
	if (!ghost)
		return false;

	BYTE mode = readByte();
	if (mode != DEATH_MODE_MANIFEST)
	{
		// Play as a ghost.
		client->SysMessage("You are a ghost");
		client->addSound(0x17f);
		client->addPlayerStart(ghost); // Do practically a full reset (to clear death menu)
	}
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

	skip(4);	// 0xEDEDEDED
	BYTE bType = readByte();

	if ( bType == 4 )
	{
		CObjBase *pObj = static_cast<CGrayUID>(readInt32()).ObjFind();
		client->addHealthBarInfo(pObj, true);
	}
	else if ( bType == 5 )
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
	if (!character || !character->m_pPlayer)
		return false;

	skip(2); // length

	SKILL_TYPE skill = static_cast<SKILL_TYPE>(readInt16());
	if ( !character->IsSkillBase(skill) )
		return false;

	SKILLLOCK_TYPE state = static_cast<SKILLLOCK_TYPE>(readByte());
	if ( (state < SKILLLOCK_UP) || (state > SKILLLOCK_LOCK) )
		return false;

	character->m_pPlayer->Skill_SetLock(skill, state);
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
	if (!buyer)
		return false;

	WORD packetLength = readInt16();
	CChar *pVendor = static_cast<CGrayUID>(readInt32()).CharFind();
	BYTE bFlags = readByte();
	if (bFlags == 0)
		return true;

	if (!pVendor || !pVendor->NPC_IsVendor())
	{
		client->Event_VendorBuy_Cheater(0x1);
		return true;
	}

	if (!buyer->CanTouch(pVendor))
	{
		client->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_TOOFAR));
		return true;
	}

	size_t itemCount = minimum((packetLength - 8) / 7, MAX_ITEMS_CONT);

	// combine goods into one list
	VendorItem items[MAX_ITEMS_CONT];
	memset(items, 0, sizeof(items));
	int iConvertFactor = pVendor->NPC_GetVendorMarkup();

	CItemVendable *item = NULL;
	for ( size_t i = 0; i < itemCount; ++i )
	{
		skip(1); // layer
		CGrayUID uidItem = static_cast<CGrayUID>(readInt32());
		WORD amount = readInt16();

		item = dynamic_cast<CItemVendable *>(uidItem.ItemFind());
		if (!item || !item->IsValidSaleItem(true))
		{
			pVendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_ORDER_CANTFULFILL));
			client->Event_VendorBuy_Cheater(0x2);
			return true;
		}

		// search for it in the list
		size_t index;
		for ( index = 0; index < itemCount; ++index )
		{
			if ( items[index].m_serial == uidItem )
				break;

			if ( items[index].m_serial.GetPrivateUID() == UID_CLEAR )
			{
				items[index].m_serial = uidItem;
				items[index].m_price = item->GetVendorPrice(iConvertFactor);
				break;
			}
		}

		items[index].m_amount += amount;
		if (items[index].m_price <= 0)
		{
			pVendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_ORDER_CANTFULFILL));
			client->Event_VendorBuy_Cheater(0x3);
			return true;
		}
	}

	client->Event_VendorBuy(pVendor, items, itemCount);
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

	CItemMap *pMap = dynamic_cast<CItemMap *>(static_cast<CGrayUID>(readInt32()).ItemFind());
	MAPCMD_TYPE action = static_cast<MAPCMD_TYPE>(readByte());
	BYTE pin = readByte();
	WORD x = readInt16();
	WORD y = readInt16();

	CClient *client = net->m_client;
	ASSERT(client);
	const CChar *character = client->GetChar();
	if ( !character )
		return false;

	if ( !pMap || !character->CanTouch(pMap) )
	{
		client->SysMessageDefault(DEFMSG_REACH_FAIL);
		return false;
	}

	if ( pMap->m_itMap.m_fPinsGlued && !client->IsPriv(PRIV_GM) )
	{
		client->SysMessageDefault(DEFMSG_CANT_MAKE);
		return false;
	}

	switch ( action )
	{
		case MAPCMD_AddPin:
		{
			if ( pMap->m_Pins.GetCount() < CItemMap::MAX_PINS )
			{
				CMapPinRec mapPin(x, y);
				pMap->m_Pins.Add(mapPin);
			}
			return true;
		}
		case MAPCMD_InsertPin:
		{
			if ( pMap->m_Pins.GetCount() < CItemMap::MAX_PINS )
			{
				CMapPinRec mapPin(x, y);
				pMap->m_Pins.InsertAt(pin, mapPin);
			}
			return true;
		}
		case MAPCMD_MovePin:
		{
			if ( pin < pMap->m_Pins.GetCount() )
			{
				pMap->m_Pins[pin].m_x = x;
				pMap->m_Pins[pin].m_y = y;
			}
			return true;
		}
		case MAPCMD_RemovePin:
		{
			if ( pin < pMap->m_Pins.GetCount() )
				pMap->m_Pins.RemoveAt(pin);
			return true;
		}
		case MAPCMD_ClearPins:
		{
			pMap->m_Pins.RemoveAll();
			return true;
		}
		case MAPCMD_ToggleEdit_Request:
		{
			client->addMapMode(pMap, MAPCMD_ToggleEdit_Reply, !pMap->m_fPlotMode);
			return true;
		}
		default:
		{
			return false;
		}
	}
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

	CClient* client = net->m_client;
	ASSERT(client);

	skip(4);	// 0xEDEDEDED
	skip(MAX_NAME_SIZE);	// char name
	skip(2);
	skip(4);	//LOGINFLAGS_TYPE flags = static_cast<LOGINFLAGS_TYPE>(readInt32());	// not really used, Sphere already have AutoResDisp feature to set ResDisp based on client version
	skip(24);
	DWORD slot = readInt32();
	skip(4);	// IP

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
	if (!character)
		return false;

	skip(2); // packet length
	CItem *pBook = static_cast<CGrayUID>(readInt32()).ItemFind();
	WORD pageCount = readInt16();

	if (!character->CanSee(pBook))
	{
		client->addObjectRemoveCantSee(pBook->GetUID(), "the book");
		return true;
	}

	WORD page = readInt16();
	WORD lineCount = readInt16();
	if ((lineCount == 0xFFFF) || (getLength() <= 13))
	{
		client->addBookPage(pBook, page, 1); // just a request for a page
		return true;
	}

	// trying to write to the book
	CItemMessage *pMessage = dynamic_cast<CItemMessage *>(pBook);
	if (!pMessage || (pageCount > pMessage->GetPageCount()) || !pMessage->IsBookWritable())
		return true;

	skip(-4);

	size_t len = 0;
	TCHAR* content = Str_GetTemp();

	for (WORD i = 0; i < pageCount; ++i)
	{
		// read next page to change with line count
		page = readInt16();
		if ((page <= 0) || (page > MAX_BOOK_PAGES))
			return true;

		lineCount = readInt16();
		if ((lineCount <= 0) || (lineCount > MAX_BOOK_LINES))
			return true;

		--page;
		len = 0;

		// read each line of the page
		while (lineCount > 0)
		{
			len += readStringNullASCII(content + len, (SCRIPT_MAX_LINE_LEN - 1) - len);
			if (len >= SCRIPT_MAX_LINE_LEN)
			{
				len = SCRIPT_MAX_LINE_LEN - 1;
				break;
			}

			content[len++] = '\t';
			--lineCount;
		}

		ASSERT(len > 0);
		content[--len] = '\0';
		if (Str_Check(content))
			break;

		// set page content
		pMessage->SetPageText(page, content);
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
	if (!character)
		return false;

	skip(1); // target type
	CLIMODE_TYPE context = static_cast<CLIMODE_TYPE>(readInt32());
	BYTE flags = readByte();
	CGrayUID uid = static_cast<CGrayUID>(readInt32());
	WORD x = readInt16();
	WORD y = readInt16();
	skip(1);
	BYTE z = readByte();
	ITEMID_TYPE id = static_cast<ITEMID_TYPE>(readInt16());

	client->Event_Target(context, uid, CPointMap(x, y, z, character->GetTopMap()), flags, id);
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
	if (!character)
		return false;

	skip(2);	// length
	SECURETRADE_TYPE action = static_cast<SECURETRADE_TYPE>(readByte());
	CItemContainer *pContainer = dynamic_cast<CItemContainer *>(static_cast<CGrayUID>(readInt32()).ItemFind());

	if ( !pContainer || (character != pContainer->GetParent()) )
		return true;

	switch ( action )
	{
		case SECURETRADE_Close:
		{
			pContainer->Delete();
			return true;
		}
		case SECURETRADE_Accept:
		{
			if ( character->GetDist(pContainer) > character->GetSight() )
			{
				client->SysMessageDefault(DEFMSG_MSG_TRADE_TOOFAR);
				return false;
			}

			UINT64 iWaitTime = pContainer->m_itEqTradeWindow.m_iWaitTime;
			UINT64 iTimestamp = g_World.GetCurrentTime().GetTimeRaw();
			if ( iWaitTime > iTimestamp )
			{
				client->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_WAIT), (iWaitTime - iTimestamp) / TICK_PER_SEC);
				return false;
			}

			pContainer->Trade_Status(readInt32() != 0);
			return true;
		}
		case SECURETRADE_UpdateGold:
		{
			pContainer->Trade_UpdateGold(readInt32(), readInt32());
			return true;
		}
		default:
		{
			return false;
		}
	}
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
	if (!character)
		return false;

	skip(2);
	BULLETINBOARD_TYPE action = static_cast<BULLETINBOARD_TYPE>(readByte());
	CGrayUID uidBulletinBoard = static_cast<CGrayUID>(readInt32());
	CGrayUID uidMessage = static_cast<CGrayUID>(readInt32());

	CItemContainer *board = dynamic_cast<CItemContainer *>(uidBulletinBoard.ItemFind());
	if (!character->CanSee(board))
	{
		client->addObjectRemoveCantSee(uidBulletinBoard, "the board");
		return true;
	}

	if (!board->IsType(IT_BBOARD))
		return true;

	switch (action)
	{
		case BULLETINBOARD_ReqFull:
		case BULLETINBOARD_ReqTitle:
		{
			if (getLength() != 12)
			{
				DEBUG_ERR(("%lx:BBoard feed back message bad length %" FMTSIZE_T "\n", net->id(), getLength()));
				return true;
			}
			if (!client->addBBoardMessage(board, action, uidMessage))
			{
				// sanity check fails
				client->addObjectRemoveCantSee(uidMessage, "the message");
				return true;
			}
			return true;
		}
		case BULLETINBOARD_PostMsg:
		{
			if (getLength() < 12)
				return true;

			if (!character->CanTouch(board))
			{
				character->SysMessageDefault(DEFMSG_ITEMUSE_BBOARD_REACH);
				return true;
			}

			if (board->GetCount() > 32)
				delete board->GetAt(board->GetCount() - 1);

			size_t lenstr = readByte();
			TCHAR* str = Str_GetTemp();
			readStringASCII(str, lenstr, false);
			if (Str_Check(str))
				return true;

			CItemMessage* newMessage = dynamic_cast<CItemMessage*>( CItem::CreateBase(ITEMID_BBOARD_MSG) );
			if (!newMessage)
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
				if (!Str_Check(str))
					newMessage->AddPageText(str);
			}

			board->ContentAdd(newMessage);
			return true;
		}
		case BULLETINBOARD_DeleteMsg:
		{
			CItemMessage *message = dynamic_cast<CItemMessage *>(uidMessage.ItemFind());
			if (!board->IsItemInside(message))
			{
				client->SysMessageDefault(DEFMSG_ITEMUSE_BBOARD_COR);
				return true;
			}

			if (!client->IsPriv(PRIV_GM) && (message->m_uidLink != character->GetUID()))
			{
				client->SysMessageDefault(DEFMSG_ITEMUSE_BBOARD_DEL);
				return true;
			}

			message->Delete();
			return true;
		}
		default:
		{
			return false;
		}
	}
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

	CChar *pChar = static_cast<CGrayUID>(readInt32()).CharFind();
	TCHAR *pszName = Str_GetTemp();
	readStringASCII(pszName, MAX_NAME_SIZE);

	net->m_client->Event_CharRename(pChar, pszName);
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

	CGrayUID uid = static_cast<CGrayUID>(readInt32());
	WORD context = readInt16();
	WORD select = readInt16();

	if ((context != client->GetTargMode()) || (uid != client->m_tmMenu.m_UID))
	{
		client->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MENU_UNEXPECTED));
		return true;
	}

	client->ClearTargMode();

	switch (context)
	{
		case CLIMODE_MENU:
			// a generic menu from script
			client->Menu_OnSelect(client->m_tmMenu.m_ResourceID, select, uid.ObjFind());
			return true;
		case CLIMODE_MENU_SKILL:
			// some skill menu got us here
			if ( select < COUNTOF(client->m_tmMenu.m_Item) )
				client->Cmd_Skill_Menu(client->m_tmMenu.m_ResourceID, (select) ? client->m_tmMenu.m_Item[select] : 0);
			return true;
		case CLIMODE_MENU_SKILL_TRACK_SETUP:
			// pretracking menu got us here
			client->Cmd_Skill_Tracking(select, false);
			return true;
		case CLIMODE_MENU_SKILL_TRACK:
			// tracking menu got us here. start tracking the selected creature
			client->Cmd_Skill_Tracking(select, true);
			return true;
		case CLIMODE_MENU_EDIT:
			// m_Targ_Text = what are we doing to it
			client->Cmd_EditItem(uid.ObjFind(), select);
			return true;
		default:
			return false;
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
	client->addDeleteErr(err, slot);
	new PacketCharacterListUpdate(client);
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
	BYTE flags = readByte();
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
	SKILL_TYPE skill3 = static_cast<SKILL_TYPE>(readByte());
	BYTE skillval3 = readByte();
	SKILL_TYPE skill4 = static_cast<SKILL_TYPE>(readByte());
	BYTE skillval4 = readByte();
	skip(26);
	HUE_TYPE hairhue = static_cast<HUE_TYPE>(readInt16());
	ITEMID_TYPE hairid = static_cast<ITEMID_TYPE>(readInt16());
	skip(13);
	ITEMID_TYPE faceid = static_cast<ITEMID_TYPE>(readInt16());
	skip(1);
	HUE_TYPE beardhue = static_cast<HUE_TYPE>(readInt16());
	ITEMID_TYPE beardid = static_cast<ITEMID_TYPE>(readInt16());

	// Since client 7.0.16.0 the new creation packet does not contain skills and values if
	// a profession is selected, so here we must translate the selected profession -> skills
	switch ( profession )
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
			dexterity = 15;
			intelligence = 15;
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
			skill3 = SKILL_SWORDSMANSHIP;
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
			intelligence = 20;
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
			intelligence = 20;
			skill1 = SKILL_NINJITSU;
			skillval1 = 30;
			skill2 = SKILL_FENCING;
			skillval2 = 30;
			skill3 = SKILL_HIDING;
			skillval3 = 30;
			skill4 = SKILL_STEALTH;
			skillval4 = 30;
			break;
	}

	return doCreate(net, charname, sex > 0, race, strength, dexterity, intelligence, profession, skill1, skillval1, skill2, skillval2, skill3, skillval3, skill4, skillval4, hue, hairid, hairhue, beardid, beardhue, HUE_DEFAULT, HUE_DEFAULT, faceid, 0, flags);
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

	CItem *pItem = static_cast<CGrayUID>(readInt32()).ItemFind();
	skip(1); // writable
	skip(1); // unknown
	skip(2); // pages

	TCHAR title[MAX_NAME_SIZE * 2];
	readStringASCII(title, COUNTOF(title));

	TCHAR author[MAX_NAME_SIZE];
	readStringASCII(author, COUNTOF(author));

	net->m_client->Event_Book_Title(pItem, title, author);
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

	CGrayUID uid = static_cast<CGrayUID>(readInt32());
	skip(2); // item id
	HUE_TYPE hue = static_cast<HUE_TYPE>(readInt16());

	net->m_client->Event_Item_Dye(uid, hue);
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
	if (!character)
		return false;

	const CObjBase* object;
	for (WORD length = readInt16(); length > sizeof(DWORD); length -= sizeof(DWORD))
	{
		object = static_cast<CGrayUID>(readInt32()).ObjFind();
		if (!object || !character->CanSee(object))
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
	CGrayUID uidChar = static_cast<CGrayUID>(readInt32());
	CGrayUID uidPrompt = static_cast<CGrayUID>(readInt32());
	DWORD type = readInt32();

	if (packetLength <= getPosition())
		return false;

	packetLength -= getPosition();
	if (packetLength > (MAX_TALK_BUFFER + 2) / 2)
		return false;

	TCHAR* text = Str_GetTemp();
	readStringASCII(text, packetLength, false);

	net->m_client->Event_PromptResp(text, packetLength, uidChar, uidPrompt, type);
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
	if (!character)
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
	if (!seller)
		return false;

	skip(2); // length
	CChar *pVendor = static_cast<CGrayUID>(readInt32()).CharFind();
	size_t itemCount = readInt16();

	if (!pVendor || !pVendor->NPC_IsVendor())
	{
		client->Event_VendorSell_Cheater(0x1);
		return true;
	}
	
	if (!seller->CanTouch(pVendor))
	{
		client->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_TOOFAR));
		return true;
	}

	if (itemCount < 1)
	{
		client->addVendorClose(pVendor);
		return true;
	}
	else if (itemCount >= MAX_ITEMS_CONT)
	{
		TCHAR *pszText = Str_GetTemp();
		sprintf(pszText, g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_SELL_LIMIT), MAX_ITEMS_CONT);
		pVendor->Speak(pszText);
		return true;
	}

	VendorItem items[MAX_ITEMS_CONT];
	memset(items, 0, sizeof(items));

	for ( size_t i = 0; i < itemCount; ++i )
	{
		items[i].m_serial = static_cast<CGrayUID>(readInt32());
		items[i].m_amount = readInt16();
	}

	client->Event_VendorSell(pVendor, items, itemCount);
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

	WORD wIndex = readInt16();
	if ( wIndex >= MAX_SERVERS )
		return false;

	net->m_client->Login_Relay(wIndex);
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

	// Ignore this packet
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
		++index;
	else
		--index;

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
	CGrayUID uid = static_cast<CGrayUID>(readInt32());
	readInt16(); // context
	BYTE action = readByte();
	WORD textLength = readInt16();
	TCHAR text[MAX_NAME_SIZE];
	readStringASCII(text, minimum(MAX_NAME_SIZE, textLength));

	if ((client->GetTargMode() != CLIMODE_INPVAL) || (uid != client->m_Targ_UID))
	{
		client->SysMessageDefault(DEFMSG_MENU_UNEXPECTED);
		return true;
	}

	client->ClearTargMode();

	CObjBase* object = uid.ObjFind();
	if (!object)
		return true;

	TCHAR *pszFix;
	if ( (pszFix = strchr(text, '\n')) != NULL )
		*pszFix = '\0';
	if ( (pszFix = strchr(text, '\r')) != NULL )
		*pszFix = '\0';
	if ( (pszFix = strchr(text, '\t')) != NULL )
		*pszFix = ' ';
	if ( (pszFix = strchr(text, '#')) != NULL )
		*pszFix = ' ';

	// take action based on the parent context
	if (action == 1) // ok
	{
		// Properties Dialog, page x
		// m_Targ_Text = the verb we are dealing with
		// m_Prop_UID = object we are after

		TCHAR *pszLogMsg = Str_GetTemp();
		sprintf(pszLogMsg, "%lx:'%s' tweak uid=0%lx (%s) to '%s %s'", net->id(), client->GetName(), static_cast<DWORD>(object->GetUID()), object->GetName(), static_cast<LPCTSTR>(client->m_Targ_Text), static_cast<LPCTSTR>(text));

		CScript script(client->m_Targ_Text, text);
		bool fRet = object->r_Verb(script, client->GetChar());
		if ( !fRet )
			client->SysMessageDefault(DEFMSG_MSG_ERR_INVSET);
		if ( client->GetPrivLevel() >= g_Cfg.m_iCommandLog )
			g_Log.Event(LOGM_GM_CMDS, "%s=%d\n", pszLogMsg, fRet);
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
	if (!client->GetChar())
		return false;

	size_t packetLength = readInt16();
	TALKMODE_TYPE mode = static_cast<TALKMODE_TYPE>(readByte());
	HUE_TYPE hue = static_cast<HUE_TYPE>(readInt16());
	FONT_TYPE font = static_cast<FONT_TYPE>(readInt16());
	TCHAR language[4];
	readStringASCII(language, COUNTOF(language));

	if (packetLength <= getPosition())
		return false;

	packetLength -= getPosition();
	if (packetLength > MAX_TALK_BUFFER + 2)
		return false;

	if (mode & TALKMODE_ENCODED) // text contains keywords
	{
		mode = static_cast<TALKMODE_TYPE>(mode & ~TALKMODE_ENCODED);

		size_t count = (readInt16() & 0xFFF0) >> 4;
		if (count > 50) // malformed check
			return true;

		skip(-2);
		count = (count + 1) * 12;
		size_t toskip = count / 8;
		if ((count % 8) > 0)
			++toskip;

		if (toskip > packetLength)
			return true;

		skip(static_cast<long>(toskip));
		TCHAR text[MAX_TALK_BUFFER + 2];
		readStringNullASCII(text, COUNTOF(text));
		client->Event_Talk(text, hue, mode, true);
	}
	else
	{
		NCHAR text[MAX_TALK_BUFFER + 2];
		readStringUNICODE(reinterpret_cast<WCHAR *>(text), (packetLength / sizeof(WCHAR)) - 1, false);

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
	if (!character)
		return false;

	skip(2); // length
	CObjBase *pObj = static_cast<CGrayUID>(readInt32()).ObjFind();
	DWORD context = readInt32();
	DWORD button = readInt32();
	DWORD checkCount = readInt32();
	if ( checkCount > MAX_TALK_BUFFER )
		return false;

	// Check client internal dialogs first (they must be handled separately because packets can be a bit different on these dialogs)
	if (pObj == character)
	{
		if (context == CLIMODE_DIALOG_VIRTUE)
		{
			CChar *pCharTarg = ((button == 1) && (checkCount > 0)) ? static_cast<CGrayUID>(readInt32()).CharFind() : character;
			client->Event_VirtueSelect(button, pCharTarg);
			return true;
		}
		else if (context == CLIMODE_DIALOG_FACESELECTION)
		{
			DWORD maxID = (g_Cfg.m_iFeatureExtra & FEATURE_EXTRA_ROLEPLAYFACES) ? ITEMID_FACE_VAMPIRE : ITEMID_FACE_10;
			if ((button >= ITEMID_FACE_1) && (button <= maxID))
			{
				CItem *pFace = character->LayerFind(LAYER_FACE);
				if (pFace)
					pFace->Delete();

				pFace = CItem::CreateBase(static_cast<ITEMID_TYPE>(button));
				if (pFace)
				{
					pFace->SetHue(character->GetHue());
					character->LayerAdd(pFace, LAYER_FACE);
				}
			}
			return true;
		}
	}

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
	for (DWORD i = 0; i < checkCount; ++i)
		resp.m_CheckArray.Add(readInt32());


	DWORD textCount = readInt32();
	if ( textCount > MAX_TALK_BUFFER )
		return false;

	TCHAR* text = Str_GetTemp();
	for (DWORD i = 0; i < textCount; ++i)
	{
		WORD id = readInt16();
		WORD length = readInt16();
		if ( length > MAX_TALK_BUFFER )
			return false;

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
	client->Dialog_OnButton(rid, button, pObj, &resp);
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

	if (packetLength <= getPosition())
		return false;

	packetLength -= getPosition();
	if (packetLength >= MAX_TALK_BUFFER)
		return false;

	NCHAR text[MAX_TALK_BUFFER];
	readStringUNICODE(reinterpret_cast<WCHAR *>(text), (packetLength / sizeof(WCHAR)) - 1, false);

	client->Event_ChatText(text, static_cast<int>(packetLength), CLanguageID(language));
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

	if ( !(g_Cfg.m_iFeatureT2A & FEATURE_T2A_CHAT) )
		return true;

	if ( client->m_UseNewChatSystem )
	{
		// On new chat system, the chat button is hardcoded on client-side and client will send
		// this packet only a single time after login complete to get initial chat channel list
		client->Event_ChatButton();
	}
	else
	{
		// On old chat system, client will always send this packet when click on chat button
		skip(1);	// 0x0
		NCHAR chatname[(MAX_NAME_SIZE + 1) * sizeof(WCHAR)];
		readStringUNICODE(reinterpret_cast<WCHAR *>(chatname), (COUNTOF(chatname) / sizeof(WCHAR)) - 1);

		client->Event_ChatButton(chatname);
	}
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

	CGrayUID uid = static_cast<CGrayUID>(readInt32());
	client->Event_ToolTip(uid);
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

	CClient *pClient = net->m_client;
	ASSERT(pClient);

	skip(2);	// length
	bool fWrite = readBool();
	CChar *pChar = static_cast<CGrayUID>(readInt32()).CharFind();
	TCHAR *pszText = NULL;

	if ( fWrite )
	{
		skip(2);
		WORD wTextLen = readInt16();

		size_t iMaxLen = SCRIPT_MAX_LINE_LEN - 16;
		if ( wTextLen >= iMaxLen )
			wTextLen = iMaxLen - 1;

		pszText = Str_GetTemp();
		readStringNUNICODE(pszText, iMaxLen, wTextLen + 1, false);
	}

	pClient->Event_Profile(fWrite, pChar, pszText);
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

	CChar *pChar = static_cast<CGrayUID>(readInt32()).CharFind();
	skip(4);	// char UID (sender)

	client->Event_MailMsg(pChar);
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
	if (!client->GetChar())
		return false;

	WORD packetLength = readInt16();
	if (packetLength > MAX_TALK_BUFFER)
		return false;

	PACKETEXT_TYPE type = static_cast<PACKETEXT_TYPE>(readInt16());
	seek();

#ifndef _MTNETWORK
	Packet* handler = g_NetworkIn.getPacketManager().getExtendedHandler(type);
#else
	Packet* handler = g_NetworkManager.getPacketManager().getExtendedHandler(type);
#endif
	if (!handler)
		return false;

	handler->seek();
	for ( WORD i = 0; i < packetLength; ++i )
		handler->writeByte(readByte());

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

	skip(2);
	WORD x = readInt16();
	WORD y = readInt16();
	skip(2);
	
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
	if (!character)
		return false;

	PARTYMSG_TYPE action = static_cast<PARTYMSG_TYPE>(readByte());
	switch ( action )
	{
		case PARTYMSG_Add:
		{
			if ( character->m_pParty && !character->m_pParty->IsPartyMaster(character) )
			{
				client->SysMessageDefault(DEFMSG_PARTY_TARG_ADD_PERMISSION);
				return false;
			}

			client->addTarget(CLIMODE_TARG_PARTY_ADD, g_Cfg.GetDefaultMsg(DEFMSG_PARTY_TARG_ADD), false, false);
			return true;
		}
		case PARTYMSG_Remove:
		{
			if ( !character->m_pParty )
			{
				client->SysMessageDefault(DEFMSG_PARTY_NOTINPARTY);
				return false;
			}

			CChar *pTarg = static_cast<CGrayUID>(readInt32()).CharFind();
			if ( pTarg )
				return character->m_pParty->RemoveMember(pTarg, character);

			client->addTarget(CLIMODE_TARG_PARTY_REMOVE, g_Cfg.GetDefaultMsg(DEFMSG_PARTY_TARG_REMOVE), false, false);
			return true;
		}
		case PARTYMSG_MsgMember:
		{
			if ( !character->m_pParty )
			{
				client->SysMessageDefault(DEFMSG_PARTY_NOTINPARTY);
				return false;
			}

			CChar *pTarg = static_cast<CGrayUID>(readInt32()).CharFind();
			NCHAR text[MAX_TALK_BUFFER];
			int length = readStringNullUNICODE(reinterpret_cast<WCHAR *>(text), MAX_TALK_BUFFER - 1);
			return character->m_pParty->MessageEvent(pTarg, character, text, length);
		}
		case PARTYMSG_MsgAll:
		{
			if ( !character->m_pParty )
			{
				client->SysMessageDefault(DEFMSG_PARTY_NOTINPARTY);
				return false;
			}

			NCHAR text[MAX_TALK_BUFFER];
			int length = readStringNullUNICODE(reinterpret_cast<WCHAR *>(text), MAX_TALK_BUFFER - 1);
			return character->m_pParty->MessageEvent(NULL, character, text, length);
		}
		case PARTYMSG_Disband:
		{
			if ( !character->m_pParty )
			{
				client->SysMessageDefault(DEFMSG_PARTY_NOTINPARTY);
				return false;
			}
			else if ( !character->m_pParty->IsPartyMaster(character) )
				return false;

			return character->m_pParty->Disband();
		}
		case PARTYMSG_ToggleLooting:
		{
			if ( !character->m_pParty )
				return false;

			character->m_pParty->SetLootFlag(character, readBool());
			character->NotoSave_Update();
			return true;
		}
		case PARTYMSG_Accept:
		{
			CChar *pCharInviter = static_cast<CGrayUID>(readInt32()).CharFind();
			return CPartyDef::AcceptEvent(character, pCharInviter);
		}
		case PARTYMSG_Decline:
		{
			CChar *pCharInviter = static_cast<CGrayUID>(readInt32()).CharFind();
			return CPartyDef::DeclineEvent(character, pCharInviter);
		}
		default:
		{
			return false;
		}
	}
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
	if (!character)
		return false;

	bool rightClick = readBool();

	client->SysMessageDefault(DEFMSG_MSG_FOLLOW_ARROW);

	if ( IsTrigUsed(TRIGGER_USERQUESTARROWCLICK) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = static_cast<INT64>(rightClick);
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

	CClient *pClient = net->m_client;
	ASSERT(pClient);

	pClient->Event_CombatAbilitySelect(0x5);	// Disarm
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

	CClient *pClient = net->m_client;
	ASSERT(pClient);

	pClient->Event_CombatAbilitySelect(0xB);	// Paralyzing Blow
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

	// Ignore this packet
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
	if (!character)
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
	for (size_t i = 0; !ok && (i < COUNTOF(validAnimations)); ++i)
		ok = (anim == validAnimations[i]);

	if (!ok)
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

	// Ignore this packet
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
	if (!character)
		return false;

	const CObjBase* object = static_cast<CGrayUID>(readInt32()).ObjFind();
	if (object && character->CanSee(object))
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

	CGrayUID uid = static_cast<CGrayUID>(readInt32());

	CClient *client = net->m_client;
	ASSERT(client);
	client->Event_AOSPopupMenuRequest(uid);
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

	CGrayUID uid = static_cast<CGrayUID>(readInt32());
	WORD tag = readInt16();

	CClient *client = net->m_client;
	ASSERT(client);
	client->Event_AOSPopupMenuSelect(uid, tag);
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
	if (!character || !character->m_pPlayer)
		return false;

	BYTE code = readByte();
	SKILLLOCK_TYPE state = static_cast<SKILLLOCK_TYPE>(readByte());

	if (code >= STAT_BASE_QTY)
		return false;
	else if ((state < SKILLLOCK_UP) || (state > SKILLLOCK_LOCK))
		return false;

	// translate UO stat to Sphere stat
	STAT_TYPE stat = STAT_NONE;
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

	CClient *pClient = net->m_client;
	ASSERT(pClient);
	CChar *pChar = pClient->GetChar();
	if ( !pChar )
		return false;

	skip(2);	// unknown
	SPELL_TYPE spell = static_cast<SPELL_TYPE>(readInt16());
	if ( spell > SPELL_SKILLMASTERIES_QTY )
		return false;

	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
	if ( !pSpellDef )
		return true;

	if ( IsSetMagicFlags(MAGICF_PRECAST) && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST) )
	{
		int skill;
		if ( !pSpellDef->GetPrimarySkill(&skill) )
			return true;

		pClient->m_tmSkillMagery.m_Spell = spell;
		pChar->m_atMagery.m_Spell = spell;
		pClient->m_Targ_UID = pChar->GetUID();
		pClient->m_Targ_PrvUID = pChar->GetUID();
		pChar->Skill_Start(static_cast<SKILL_TYPE>(skill));
	}
	else
		pClient->Cmd_Skill_Magery(spell, pChar);
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

	CClient *pClient = net->m_client;
	ASSERT(pClient);

	CItem *pItem = static_cast<CGrayUID>(readInt32()).ItemFind();
	if ( !pItem )
		return true;

	CItemMultiCustom *pHouse = dynamic_cast<CItemMultiCustom *>(pItem);
	if ( !pHouse )
		return true;

	pHouse->SendStructureTo(pClient);
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

	// Ignore this packet
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

	CClient *pClient = net->m_client;
	ASSERT(pClient);
	CChar *pChar = pClient->GetChar();
	if ( !pChar )
		return false;

	CItem *pBandage = static_cast<CGrayUID>(readInt32()).ItemFind();
	if ( !pBandage || !pBandage->IsType(IT_BANDAGE) || !pChar->CanUse(pBandage, false) )
		return false;

	CObjBase *pTarget = static_cast<CGrayUID>(readInt32()).ObjFind();
	if ( !pTarget )
		return false;

	pClient->m_Targ_UID = pBandage->GetUID();
	pClient->m_tmUseItem.m_pParent = pBandage->GetParent();
	pClient->SetTargMode(CLIMODE_TARG_USE_ITEM);
	pClient->Event_Target(CLIMODE_TARG_USE_ITEM, pTarget->GetUID(), pTarget->GetTopPoint());
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x2D : PacketTargetedSpell			use targeted spell
 *
 *
 ***************************************************************************/
PacketTargetedSpell::PacketTargetedSpell() : Packet(0)
{
}

bool PacketTargetedSpell::onReceive(NetState *net)
{
	ADDTOCALLSTACK("PacketTargetedSpell::onReceive");

	CClient *pClient = net->m_client;
	ASSERT(pClient);
	CChar *pChar = pClient->GetChar();
	if ( !pChar )
		return false;

	SPELL_TYPE spell = static_cast<SPELL_TYPE>(readInt16());
	if ( (spell > SPELL_SKILLMASTERIES_QTY) || !pChar->Spell_CanCast(spell, true, pChar, true) )
		return false;

	CObjBase *pTarget = static_cast<CGrayUID>(readInt32()).ObjFind();
	if ( !pTarget )
		return false;

	pClient->m_tmSkillMagery.m_Spell = spell;
	pClient->m_Targ_PrvUID = pChar->GetUID();
	pClient->OnTarg_Skill_Magery(pTarget, pTarget->GetTopPoint());
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x2E : PacketTargetedSkill			use targeted skill
 *
 *
 ***************************************************************************/
PacketTargetedSkill::PacketTargetedSkill() : Packet(0)
{
}

bool PacketTargetedSkill::onReceive(NetState *net)
{
	ADDTOCALLSTACK("PacketTargetedSkill::onReceive");

	CClient *pClient = net->m_client;
	ASSERT(pClient);
	CChar *pChar = pClient->GetChar();
	if ( !pChar )
		return false;

	SKILL_TYPE skill = static_cast<SKILL_TYPE>(readInt16());
	if ( (skill >= static_cast<SKILL_TYPE>(g_Cfg.m_iMaxSkill)) || !CChar::IsSkillBase(skill) || !pChar->Skill_CanUse(skill) || pChar->Skill_Wait(skill) )
		return false;

	CObjBase *pTarget = static_cast<CGrayUID>(readInt32()).ObjFind();
	if ( !pTarget )
		return false;

	pClient->m_tmSkillTarg.m_Skill = skill;
	pClient->OnTarg_Skill(pTarget);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x30 : PacketTargetedResource		use targeted resource
 *
 *
 ***************************************************************************/
PacketTargetedResource::PacketTargetedResource() : Packet(0)
{
}

bool PacketTargetedResource::onReceive(NetState *net)
{
	ADDTOCALLSTACK("PacketTargetedResource::onReceive");

	CClient *pClient = net->m_client;
	ASSERT(pClient);
	CChar *pChar = pClient->GetChar();
	if ( !pChar )
		return false;

	CItem *pTool = static_cast<CGrayUID>(readInt32()).ItemFind();
	if ( !pTool || !pChar->CanUse(pTool, false) )
		return false;

	IT_TYPE type = IT_NORMAL;
	int iDist = 0;

	WORD wResourceType = readInt16();
	switch ( wResourceType )
	{
		case 0:		type = IT_NORMAL;	iDist = g_Cfg.GetSkillDef(SKILL_MINING)->m_Range;			break;	// Ore
		case 1:		type = IT_ROCK;		iDist = g_Cfg.GetSkillDef(SKILL_MINING)->m_Range;			break;	// Sand
		case 2:		type = IT_TREE;		iDist = g_Cfg.GetSkillDef(SKILL_LUMBERJACKING)->m_Range;	break;	// Wood
		case 3:		type = IT_DIRT;		iDist = g_Cfg.GetSkillDef(SKILL_MINING)->m_Range;			break;	// Digging Graves
		//case 4:	type = IT_MUSHROOM;	iDist = g_Cfg.GetSkillDef(SKILL_LUMBERJACKING)->m_Range;	break;	// Red Mushroom (TO-DO)
		case 5:		type = IT_WATER;	iDist = g_Cfg.GetSkillDef(SKILL_FISHING)->m_Range;			break;	// Water
		case 6:		type = IT_LAVA;		iDist = g_Cfg.GetSkillDef(SKILL_FISHING)->m_Range;			break;	// Lava
	}

	CPointMap pt = g_World.FindItemTypeNearby(pChar->GetTopPoint(), type, iDist, false, true);
	if ( !pt.IsValidPoint() )
		pt = pChar->GetTopPoint();

	if ( pTool->Item_GetDef()->IsTypeEquippable() )
		pChar->ItemEquip(pTool);

	pClient->m_Targ_UID = pTool->GetUID();
	pClient->m_tmUseItem.m_pParent = pTool->GetParent();
	pClient->SetTargMode(CLIMODE_TARG_USE_ITEM);
	pClient->Event_Target(CLIMODE_TARG_USE_ITEM, UID_CLEAR, pt);
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

	// The client always send these 2 values to server, but they're not really used
	//WORD one = readInt16();
	//DWORD zero = readInt32();

	CClient* client = net->m_client;
	ASSERT(client);
	CChar* character = client->GetChar();
	if ( !character )
		return false;

	if ( !character->IsGargoyle() )
	{
		client->SysMessageDefault(DEFMSG_GARGOYLE_FLY_CANTCURRENTFORM);
		return false;
	}
	if ( character->IsStatFlag(STATF_DEAD) )
	{
		client->SysMessageDefault(DEFMSG_GARGOYLE_FLY_CANTDEAD);
		return false;
	}
	if ( character->IsStatFlag(STATF_Freeze|STATF_Stone) )
	{
		client->SysMessageDefault(DEFMSG_MSG_FROZEN);
		return false;
	}

	character->ToggleFlying();
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

bool PacketWheelBoatMove::onReceive(NetState *net)
{
	ADDTOCALLSTACK("PacketWheelBoatMove::onReceive");

	CClient *client = net->m_client;
	ASSERT(client);
	CChar *character = client->GetChar();
	if ( !character )
		return false;

	skip(4);	// char UID
	DIR_TYPE facing = static_cast<DIR_TYPE>(readByte());	// boat facing direction
	DIR_TYPE moving = static_cast<DIR_TYPE>(readByte());	// boat moving direction
	BYTE speed = readByte();								// boat moving speed (0 = stop, 1 = slow, 2 = fast) - values are NOT the same as packet 0xF6

	CRegionWorld *pRegion = character->m_pArea;
	if ( !pRegion || !pRegion->IsFlag(REGION_FLAG_SHIP) )
		return false;

	CItemShip *pShip = dynamic_cast<CItemShip *>(pRegion->GetResourceID().ItemFind());
	if ( !pShip || (pShip->m_itShip.m_Pilot != character->GetUID()) )
	{
		CItem *pMemory = character->ContentFind(RESOURCE_ID(RES_ITEMDEF, ITEMID_MEMORY_SHIP_PILOT));
		if ( pMemory )
			pMemory->Delete();
		return false;
	}

	if ( speed > 0 )
	{
		if ( speed > 2 )
			speed = 2;

		if ( (speed == 2) && (facing != pShip->m_itShip.m_DirFace) )
			pShip->Ship_Face(facing);

		if ( pShip->Ship_SetMoveDir(facing, speed, true) )
			pShip->Ship_Move(moving, speed);
	}
	else
		pShip->Ship_Stop();

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
	CGrayUID uidChar = static_cast<CGrayUID>(readInt32());
	CGrayUID uidPrompt = static_cast<CGrayUID>(readInt32());
	DWORD type = readInt32();

	TCHAR language[4];
	readStringASCII(language, COUNTOF(language));
	
	if (length < getPosition())
		return false;

	length -= getPosition();
	if ( length > MAX_TALK_BUFFER + 2 )
		return false;

	TCHAR *text = Str_GetTemp();
	readStringUNICODE(text, THREAD_STRING_LENGTH, (length / sizeof(WCHAR)) - 1);

	net->m_client->Event_PromptResp(text, length, uidChar, uidPrompt, type);
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
	CItem *pItem = static_cast<CGrayUID>(readInt32()).ItemFind();
	skip(1); // unknown
	skip(1); // writable
	skip(2); // pages

	TCHAR title[MAX_NAME_SIZE * 2];
	TCHAR author[MAX_NAME_SIZE];

	size_t titleLength = readInt16();
	readStringASCII(title, minimum(titleLength, COUNTOF(title)));

	size_t authorLength = readInt16();
	readStringASCII(author, minimum(authorLength, COUNTOF(author)));

	net->m_client->Event_Book_Title(pItem, title, author);
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
	if (!character)
		return false;

	const CObjBase* object;
	for (WORD length = readInt16(); length > sizeof(DWORD); length -= sizeof(DWORD))
	{
		object = static_cast<CGrayUID>(readInt32()).ObjFind();
		if (!object || !character->CanSee(object))
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
	if (!character)
		return false;

	WORD packetLength = readInt16();
	if (packetLength > 33)
		return false;

	skip(4);	// char UID
	PACKETENC_TYPE type = static_cast<PACKETENC_TYPE>(readInt16());
	seek();

#ifndef _MTNETWORK
	Packet* handler = g_NetworkIn.getPacketManager().getEncodedHandler(type);
#else
	Packet* handler = g_NetworkManager.getPacketManager().getEncodedHandler(type);
#endif
	if (!handler)
		return false;

	handler->seek();
	for ( WORD i = 0; i < packetLength; ++i )
		handler->writeByte(readByte());

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

	CClient *pClient = net->m_client;
	ASSERT(pClient);

	CItemMultiCustom *pHouse = pClient->m_pHouseDesign;
	if ( !pHouse )
		return true;

	pHouse->BackupStructure();
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

	CClient *pClient = net->m_client;
	ASSERT(pClient);

	CItemMultiCustom *pHouse = pClient->m_pHouseDesign;
	if ( !pHouse )
		return true;

	pHouse->RestoreStructure(pClient);
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

	CClient *pClient = net->m_client;
	ASSERT(pClient);

	CItemMultiCustom *pHouse = pClient->m_pHouseDesign;
	if ( !pHouse )
		return true;

	pHouse->CommitChanges(pClient);
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

	CClient *pClient = net->m_client;
	ASSERT(pClient);

	CItemMultiCustom *pHouse = pClient->m_pHouseDesign;
	if ( !pHouse )
		return true;

	skip(1);
	ITEMID_TYPE id = static_cast<ITEMID_TYPE>(readInt32());
	skip(1);
	signed short x = static_cast<signed short>(readInt32());
	skip(1);
	signed short y = static_cast<signed short>(readInt32());
	skip(1);
	signed char z = static_cast<signed char>(readInt32());

	pHouse->RemoveItem(pClient, id, x, y, z);
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

	CClient *pClient = net->m_client;
	ASSERT(pClient);

	CItemMultiCustom *pHouse = pClient->m_pHouseDesign;
	if ( !pHouse )
		return true;

	skip(1);
	ITEMID_TYPE id = static_cast<ITEMID_TYPE>(readInt32());
	skip(1);
	signed short x = static_cast<signed short>(readInt32());
	skip(1);
	signed short y = static_cast<signed short>(readInt32());

	pHouse->AddItem(pClient, id, x, y);
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

	CClient *pClient = net->m_client;
	ASSERT(pClient);

	CItemMultiCustom *pHouse = pClient->m_pHouseDesign;
	if ( !pHouse )
		return true;

	pHouse->EndCustomize();
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

	CClient *pClient = net->m_client;
	ASSERT(pClient);

	CItemMultiCustom *pHouse = pClient->m_pHouseDesign;
	if ( !pHouse )
		return true;

	skip(1);
	ITEMID_TYPE id = static_cast<ITEMID_TYPE>(readInt32() + ITEMID_MULTI);
	skip(1);
	signed short x = static_cast<signed short>(readInt32());
	skip(1);
	signed short y = static_cast<signed short>(readInt32());

	pHouse->AddStairs(pClient, id, x, y);
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

	CClient *pClient = net->m_client;
	ASSERT(pClient);

	CItemMultiCustom *pHouse = pClient->m_pHouseDesign;
	if ( !pHouse )
		return true;

	pHouse->SendStructureTo(pClient);
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

	CClient *pClient = net->m_client;
	ASSERT(pClient);

	CItemMultiCustom *pHouse = pClient->m_pHouseDesign;
	if ( !pHouse )
		return true;

	pHouse->ResetStructure(pClient);
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

	CClient *pClient = net->m_client;
	ASSERT(pClient);

	CItemMultiCustom *pHouse = pClient->m_pHouseDesign;
	if ( !pHouse )
		return true;

	skip(1);
	BYTE bLevel = static_cast<BYTE>(readInt32());

	pHouse->SwitchToLevel(pClient, bLevel);
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

	CClient *pClient = net->m_client;
	ASSERT(pClient);

	CItemMultiCustom *pHouse = pClient->m_pHouseDesign;
	if ( !pHouse )
		return true;

	skip(1);
	ITEMID_TYPE id = static_cast<ITEMID_TYPE>(readInt32());
	skip(1);
	signed short x = static_cast<signed short>(readInt32());
	skip(1);
	signed short y = static_cast<signed short>(readInt32());
	skip(1);
	signed char z = static_cast<signed char>(readInt32());

	pHouse->AddRoof(pClient, id, x, y, z);
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

	CClient *pClient = net->m_client;
	ASSERT(pClient);

	CItemMultiCustom *pHouse = pClient->m_pHouseDesign;
	if ( !pHouse )
		return true;

	skip(1);
	ITEMID_TYPE id = static_cast<ITEMID_TYPE>(readInt32());
	skip(1);
	signed short x = static_cast<signed short>(readInt32());
	skip(1);
	signed short y = static_cast<signed short>(readInt32());
	skip(1);
	signed char z = static_cast<signed char>(readInt32());

	pHouse->RemoveRoof(pClient, id, x, y, z);
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

	skip(1);
	DWORD ability = readInt32();

	client->Event_CombatAbilitySelect(ability);
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

	CClient *pClient = net->m_client;
	ASSERT(pClient);

	CItemMultiCustom *pHouse = pClient->m_pHouseDesign;
	if ( !pHouse )
		return true;

	pHouse->RevertChanges(pClient);
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

	CClient *client = net->m_client;
	ASSERT(client);
	CChar *character = client->GetChar();
	if ( !character )
		return false;

	if ( character->m_uidWeaponLast == character->m_uidWeapon )
		return true;

	CItem *pWeapon = character->m_uidWeaponLast.ItemFind();
	if ( character->ItemPickup(pWeapon, 1) == -1 )
		return true;

	character->ItemEquip(pWeapon);
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
	if (!character)
		return false;

	if ( IsTrigUsed(TRIGGER_USERGUILDBUTTON) )
		character->OnTrigger(CTRIG_UserGuildButton, character);
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
	if (!character)
		return false;

	if ( IsTrigUsed(TRIGGER_USERQUESTBUTTON) )
		character->OnTrigger(CTRIG_UserQuestButton, character);
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

	// Ignore this packet
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

	skip(2);	// length

	TCHAR language[4];
	readStringASCII(language, COUNTOF(language));

	BUGREPORT_TYPE type = static_cast<BUGREPORT_TYPE>(readInt16());

	TCHAR text[1001];
	int textLength = readStringNullNUNICODE(text, 1001, 1000);

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

	// Ignore this packet
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
	if (!character)
		return false;

	skip(2); // packet length
	BYTE itemCount = readByte();
	if ( itemCount > 3 )	// prevent packet exploit sending fake values just to create heavy loops and overload server CPU
		itemCount = 3;

	CItem* item = NULL;
	for (BYTE i = 0; i < itemCount; ++i)
	{
		item = static_cast<CGrayUID>(readInt32()).ItemFind();
		if (!item)
			continue;
		if ((item->GetTopLevelObj() != character) || item->IsItemEquipped())
			continue;
		if (character->ItemPickup(item, item->GetAmount()) == -1)
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
	if (!character)
		return false;

	skip(2); // packet length
	BYTE itemCount = readByte();
	if ( itemCount > 3 )	// prevent packet exploit sending fake values just to create heavy loops and overload server CPU
		itemCount = 3;

	LAYER_TYPE layer;
	CItem* item = NULL;
	for (BYTE i = 0; i < itemCount; ++i)
	{
		layer = static_cast<LAYER_TYPE>(readInt16());

		item = character->LayerFind(layer);
		if (!item)
			continue;
		if (character->ItemPickup(item, item->GetAmount()) == -1)
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

		--steps;
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

	g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "%lx:Client crashed at %hu,%hu,%hhu,%hhu: 0x%08lX %s @ 0x%08lX (%s, %hhu.%hhu.%hhu.%hhu)\n", net->id(), x, y, z, map, errorCode, description, errorOffset, executable, versionMaj, versionMin, versionRev, versionPat);
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


/***************************************************************************
 *
 *
 *	Packet 0xF9 : PacketGlobalChatReq				global chat (INCOMPLETE)
 *
 *
 ***************************************************************************/
PacketGlobalChatReq::PacketGlobalChatReq() : Packet(0)
{
}

bool PacketGlobalChatReq::onReceive(NetState *net)
{
	ADDTOCALLSTACK("PacketGlobalChatReq::onReceive");

	CClient *client = net->m_client;
	ASSERT(client);

	if ( !(g_Cfg.m_iChatFlags & CHATF_GLOBALCHAT) )
	{
		client->SysMessage("Global Chat is currently unavailable.");
		return true;
	}

	skip(1);
	BYTE action = readByte();
	skip(1);

	TCHAR xml[MAX_TALK_BUFFER * 2];
	readStringASCII(xml, COUNTOF(xml));
	//DEBUG_ERR(("GlobalChat XML received: %s\n", xml));
	
	switch ( action )
	{
		case PacketGlobalChat::MessageSend:
			// TO-DO
			return true;
		case PacketGlobalChat::FriendRemove:
			// TO-DO
			return true;
		case PacketGlobalChat::FriendAddTarg:
			client->addTarget(CLIMODE_TARG_GLOBALCHAT_ADD, "Target player to request as Global Chat friend.");
			return true;
		case PacketGlobalChat::StatusToggle:
			client->addGlobalChatStatusToggle();
			return true;
		default:
			return false;
	}
}


/***************************************************************************
*
*
*	Packet 0xFA : PacketUltimaStoreButton			ultima store button pressed
*
*
***************************************************************************/
PacketUltimaStoreButton::PacketUltimaStoreButton() : Packet(1)
{
}

bool PacketUltimaStoreButton::onReceive(NetState *net)
{
	ADDTOCALLSTACK("PacketUltimaStoreButton::onReceive");

	CClient *client = net->m_client;
	ASSERT(client);
	CChar *character = client->GetChar();
	if ( !character )
		return false;

	if ( IsTrigUsed(TRIGGER_USERULTIMASTOREBUTTON) )
		character->OnTrigger(CTRIG_UserUltimaStoreButton, character);
	return true;
}
