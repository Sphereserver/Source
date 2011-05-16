#include "send.h"
#include "network.h"
#pragma warning(disable:4096)
#include "../common/zlib/zlib.h"
#pragma warning(default:4096)

/***************************************************************************
 *
 *
 *	Packet **** : PacketGeneric				Temporary packet till all will be redone! (NORMAL)
 *
 *
 ***************************************************************************/
PacketGeneric::PacketGeneric(const CClient* target, BYTE *data, size_t length) : PacketSend(0, length, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketGeneric::PacketGeneric");

	seek();
	writeData(data, length);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet **** : PacketTelnet				send message to telnet client (NORMAL)
 *
 *
 ***************************************************************************/
PacketTelnet::PacketTelnet(const CClient* target, LPCTSTR message) : PacketSend(0, 0, PRI_HIGHEST)
{
	ADDTOCALLSTACK("PacketTelnet::PacketTelnet");

	seek();

	for (size_t i = 0; message[i] != '\0'; i++)
	{
		if (message[i] == '\n')
			writeCharASCII('\r');

		writeCharASCII(message[i]);
	}

	trim();
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet **** : PacketWeb					send message to web client (NORMAL)
 *
 *
 ***************************************************************************/
PacketWeb::PacketWeb(const CClient* target, BYTE* data, size_t length) : PacketSend(0, 0, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketWeb::PacketWeb");

	if (data != NULL && length > 0)
		setData(data, length);

	if (target != NULL)
		push(target);
}

void PacketWeb::setData(BYTE* data, size_t length)
{
	seek();
	writeData(data, length);
	trim();
}


/***************************************************************************
 *
 *
 *	Packet 0x0B : PacketCombatDamage		sends notification of got damage (NORMAL)
 *
 *
 ***************************************************************************/
PacketCombatDamage::PacketCombatDamage(const CClient* target, DWORD damage, CGrayUID defender) : PacketSend(XCMD_DamagePacket, 7, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketCombatDamage::PacketCombatDamage");

	if ( damage >= 0x10000 )
		damage = 0xffff;

	writeInt32(defender);
	writeInt16(damage);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x11 : PacketCharacterStatus		sends status window data (LOW)
 *
 *
 ***************************************************************************/
PacketCharacterStatus::PacketCharacterStatus(const CClient* target, CChar* other) : PacketSend(XCMD_Status, 7, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketCharacterStatus::PacketCharacterStatus");

	const CChar* character = target->GetChar();
	bool canRename = (character != other && other->NPC_IsOwnedBy(character) && other->Char_GetDef()->GetHireDayWage() == 0 );

	initLength();

	writeInt32(other->GetUID());
	writeStringFixedASCII(other->GetName(), 30);

	if (character == other)
	{
		writeInt16(other->Stat_GetVal(STAT_STR));
		writeInt16(other->Stat_GetMax(STAT_STR));
		writeBool(canRename);

		int version(0);
		if (target->GetNetState()->isClientLessVersion(MINCLIVER_STATUS_V2))
			version = 1;
		else if (target->GetNetState()->isClientLessVersion(MINCLIVER_STATUS_V3))
			version = 2;
		else if (target->GetNetState()->isClientLessVersion(MINCLIVER_STATUS_V4))
			version = 3;
		else if (target->GetNetState()->isClientLessVersion(MINCLIVER_STATUS_V5))
			version = 4;
		else
			version = 5;

		int strength = other->Stat_GetAdjusted(STAT_STR);
		if (strength < 0) strength = 0;

		int dexterity = other->Stat_GetAdjusted(STAT_DEX);
		if (dexterity < 0) dexterity = 0;

		int intelligence = other->Stat_GetAdjusted(STAT_INT);
		if (intelligence < 0) intelligence = 0;

		writeByte(version);

		writeBool(other->Char_GetDef()->IsFemale());
		writeInt16(strength);
		writeInt16(dexterity);
		writeInt16(intelligence);
		writeInt16(other->Stat_GetVal(STAT_DEX));
		writeInt16(other->Stat_GetMax(STAT_DEX));
		writeInt16(other->Stat_GetVal(STAT_INT));
		writeInt16(other->Stat_GetMax(STAT_INT));

		if ( !g_Cfg.m_fPayFromPackOnly )
			writeInt32(other->ContentCount(RESOURCE_ID(RES_TYPEDEF,IT_GOLD)));
		else
			writeInt32(other->GetPackSafe()->ContentCount(RESOURCE_ID(RES_TYPEDEF,IT_GOLD)));
		
		writeInt16(other->m_defense + other->Char_GetDef()->m_defense);
		writeInt16(other->GetTotalWeight() / WEIGHT_UNITS);

		if (version >= 5) // ML attributes
		{
			writeInt16(g_Cfg.Calc_MaxCarryWeight(other) / WEIGHT_UNITS);

			switch (other->GetDispID())
			{
				case CREID_MAN:
				case CREID_WOMAN:
				case CREID_GHOSTMAN:
				case CREID_GHOSTWOMAN:
					writeByte(RACETYPE_HUMAN);
					break;
				case CREID_ELFMAN:
				case CREID_ELFWOMAN:
				case CREID_ELFGHOSTMAN:
				case CREID_ELFGHOSTWOMAN:
					writeByte(RACETYPE_ELF);
					break;
				case CREID_GARGMAN:
				case CREID_GARGWOMAN:
				case CREID_GARGGHOSTMAN:
				case CREID_GARGGHOSTWOMAN:
					writeByte(RACETYPE_GARGOYLE);
					break;
				default:
					writeByte(RACETYPE_UNDEFINED);
					break;
			}
		}

		if (version >= 2) // T2A attributes
		{
			int statcap = other->Stat_GetLimit(STAT_QTY);
			if (statcap < 0) statcap = 0;

			writeInt16(statcap);
		}

		if (version >= 3) // AOS attributes
		{
			if (other->m_pPlayer != NULL)
			{
				writeByte(other->m_pPlayer->m_curFollower);
				writeByte(other->m_pPlayer->m_maxFollower);
			}
			else
			{
				writeByte(0);
				writeByte(0);
			}
		}

		if (version >= 4) // SE attributes
		{
			writeInt16(other->m_ResFire);
			writeInt16(other->m_ResCold);
			writeInt16(other->m_ResPoison);
			writeInt16(other->m_ResEnergy);
			writeInt16(other->m_pPlayer? other->m_pPlayer->m_luck : 0);

			const CItem* weapon = other->m_uidWeapon.ItemFind();
			if (weapon != NULL)
			{
				writeInt16(weapon->Item_GetDef()->m_attackBase + weapon->m_ModAr);
				writeInt16(other->Fight_CalcDamage(weapon, weapon->Weapon_GetSkill(), true));
			}
			else
			{
				writeInt16(other->Char_GetDef()->m_attackBase);
				writeInt16(other->Fight_CalcDamage(NULL, SKILL_WRESTLING, true));
			}

			writeInt32(other->m_pPlayer? other->m_pPlayer->m_iTithingPoints : 0);
		}
	}
	else
	{
		writeInt16((other->Stat_GetVal(STAT_STR) * 100) / maximum(other->Stat_GetMax(STAT_STR), 1));
		writeInt16(100);
		writeBool(canRename);
		writeByte(0);
	}

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x17 : PacketHealthBarUpdate		update health bar colour (LOW)
 *
 *
 ***************************************************************************/
PacketHealthBarUpdate::PacketHealthBarUpdate(const CClient* target, const CChar* character) : PacketSend(XCMD_HealthBarColor, 15, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL), m_character(character->GetUID())
{
	ADDTOCALLSTACK("PacketHealthBarUpdate::PacketHealthBarUpdate");

	initLength();

	writeInt32(character->GetUID());

	writeInt16(2);
	writeInt16(GreenBar);
	writeByte(character->IsStatFlag(STATF_Poisoned));
	writeInt16(YellowBar);
	writeByte(character->IsStatFlag(STATF_Freeze|STATF_Sleeping|STATF_Hallucinating|STATF_Stone));

	push(target);
}

bool PacketHealthBarUpdate::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketHealthBarUpdate::onSend");
#ifndef _MTNETWORK
	if (g_NetworkOut.isActive())
#else
	if (g_NetworkManager.isOutputThreaded())
#endif
		return true;

	return client->CanSee(m_character.CharFind());
}


/***************************************************************************
 *
 *
 *	Packet 0x1A : PacketItemWorld			sends item on ground (NORMAL)
 *
 *
 ***************************************************************************/
PacketItemWorld::PacketItemWorld(BYTE id, size_t size, CGrayUID uid) : PacketSend(id, size, PRI_NORMAL), m_item(uid)
{
}

PacketItemWorld::PacketItemWorld(const CClient* target, CItem *item) : PacketSend(XCMD_Put, 20, PRI_NORMAL), m_item(item->GetUID())
{
	ADDTOCALLSTACK("PacketItemWorld::PacketItemWorld");

	DWORD uid = item->GetUID();
	long amount = ( item->GetAmount() > 1 ) ? item->GetAmount() : 0;
	ITEMID_TYPE id = item->GetDispID();
	CPointMap p = item->GetTopPoint();
	BYTE dir = DIR_N;
	HUE_TYPE hue = item->GetHue();
	BYTE flags = 0;

	adjustItemData(target, item, id, hue, amount, p, dir, flags, dir);

	// this packet only supports item ids up to 0x3fff, and multis start from 0x4000 (ITEMID_MULTI_LEGACY)
	// multis need to be adjusted to the lower range, and items between 03fff and 08000 need to be adjusted
	// to something safer
	if (id >= ITEMID_MULTI)
		id = (ITEMID_TYPE)(id - (ITEMID_MULTI - ITEMID_MULTI_LEGACY));
	else if (id >= ITEMID_MULTI_LEGACY)
		id = ITEMID_WorldGem;

	if (amount > 0)
		uid |= 0x80000000;
	else
		uid &= 0x7fffffff;

	p.m_x &= 0x7fff;
	if (dir > 0)
		p.m_x |= 0x8000;
	p.m_y &= 0x3fff;
	if (hue > 0)
		p.m_y |= 0x8000;
	if (flags > 0)
		p.m_y |= 0x4000;

	initLength();
	writeInt32(uid);
	writeInt16(id);
	if (amount > 0)
		writeInt16((WORD)amount);
	writeInt16(p.m_x);
	writeInt16(p.m_y);
	if (dir > 0)
		writeByte(dir);
	writeByte(p.m_z);
	if (hue > 0)
		writeInt16(hue);
	if (flags > 0)
		writeByte(flags);

	push(target);
}

void PacketItemWorld::adjustItemData(const CClient* target, CItem* item, ITEMID_TYPE &id, HUE_TYPE &hue, long &amount, CPointMap &p, BYTE &dir, BYTE &flags, BYTE& light)
{
	ADDTOCALLSTACK("PacketItemWorld::adjustItemData");

	const CChar* character = target->GetChar();
	ASSERT(character);

	// modify the values for the specific client/item.
	if (id != ITEMID_CORPSE)
	{
		const CItemBase* itemDefintion = item->Item_GetDef();
		if (itemDefintion && (target->GetResDisp() < itemDefintion->GetResLevel()))
		{
			id = (ITEMID_TYPE)itemDefintion->GetResDispDnId();
			if (itemDefintion->GetResDispDnHue() != HUE_DEFAULT)
				hue = itemDefintion->GetResDispDnHue();
		}

		// on monster this just colors the underwear. thats it.
		if (hue & HUE_UNDERWEAR)
			hue = 0;
		else if ((hue & HUE_MASK_HI) > HUE_QTY)
			hue &= HUE_MASK_LO | HUE_TRANSLUCENT;
		else
			hue &= HUE_MASK_HI | HUE_TRANSLUCENT;
	}
	else
	{
		// adjust amount and hue of corpse if necessary
		const CCharBase* charDefinition = CCharBase::FindCharBase(item->m_itCorpse.m_BaseID);
		if (charDefinition && (target->GetResDisp() < charDefinition->GetResLevel()))
		{
			amount = charDefinition->GetResDispDnId();
			if (charDefinition->GetResDispDnHue() != HUE_DEFAULT)
				hue = charDefinition->GetResDispDnHue();
		}
		
		// allow HUE_UNDERWEAR colours only on corpses
		if ((hue & HUE_MASK_HI) > HUE_QTY)
			hue &= HUE_MASK_LO | HUE_UNDERWEAR | HUE_TRANSLUCENT;
		else
			hue &= HUE_MASK_HI | HUE_UNDERWEAR | HUE_TRANSLUCENT;
	}

	if (character->IsStatFlag(STATF_Hallucinating))
		hue = Calc_GetRandVal(HUE_DYE_HIGH); // restrict colours

	if (character->CanMove(item, false))
		flags |= ITEMF_MOVABLE;

	if (target->IsPriv(PRIV_DEBUG))
	{
		id = ITEMID_WorldGem;
		p.m_z = character->GetTopZ();
		amount = 0;
		flags |= ITEMF_MOVABLE;
	}
	else
	{
		if (character->CanSee(item) == false)
			flags |= ITEMF_INVIS;

		if (item->Item_GetDef()->Can(CAN_I_LIGHT))
		{
			if (item->IsTypeLit())
				light = item->m_itLight.m_pattern;
			else
				light = LIGHT_LARGE;
		}
		else if (id == ITEMID_CORPSE)
		{
			dir = item->m_itCorpse.m_facing_dir;
		}
	}
}

bool PacketItemWorld::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketItemWorld::onSend");
#ifndef _MTNETWORK
	if (g_NetworkOut.isActive())
#else
	if (g_NetworkManager.isOutputThreaded())
#endif
		return true;

	return client->CanSee(m_item.ItemFind());
}


/***************************************************************************
 *
 *
 *	Packet 0x1B : PacketPlayerStart			allow client to start playing (HIGH)
 *
 *
 ***************************************************************************/
PacketPlayerStart::PacketPlayerStart(const CClient* target) : PacketSend(XCMD_Start, 37, g_Cfg.m_fUsePacketPriorities? PRI_HIGH : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPlayerStart::PacketPlayerStart");

	const CChar* character = target->GetChar();
	const CPointMap& pt = character->GetTopPoint();
	
	writeInt32(character->GetUID());
	writeInt32(0);
	writeInt16(character->GetDispID());
	writeInt16(pt.m_x);
	writeInt16(pt.m_y);
	writeInt16(pt.m_z);
	writeByte(character->GetDirFlag());
	writeByte(0);
	writeInt32(0xffffffff);
	writeInt16(0);
	writeInt16(0);
	writeInt16(pt.m_map > 0? g_MapList.GetX(pt.m_map) : 0x1800);
	writeInt16(pt.m_map > 0? g_MapList.GetY(pt.m_map) : 0x1000);
	writeInt16(0);
	writeInt32(0);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x1C: PacketMessageASCII			show message to client (NORMAL)
 *
 *
 ***************************************************************************/
PacketMessageASCII::PacketMessageASCII(const CClient* target, LPCTSTR pszText, const CObjBaseTemplate * source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font) : PacketSend(XCMD_Speak, 42, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketMessageASCII::PacketMessageASCII");

	initLength();

	if (source == NULL)
		writeInt32(0xFFFFFFFF);
	else
		writeInt32(source->GetUID());
	
	if (source == NULL || source->IsChar() == false)
	{
		writeInt16(0xFFFF);
	}
	else
	{
		const CChar* sourceCharacter = dynamic_cast<const CChar*>(source);
		ASSERT(sourceCharacter);
		writeInt16(sourceCharacter->GetDispID());
	}
	
	writeByte(mode);
	writeInt16(hue);
	writeInt16(font);

	// we need to ensure that the name is null terminated here when using TALKMODE_ITEM, otherwise
	// the journal can freeze and crash older client versions
	if (source == NULL)
		writeStringFixedASCII("System", 30);
	else
		writeStringFixedASCII(source->GetName(), 30, true);

	writeStringASCII(pszText);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x1D : PacketRemoveObject		removes object from view (NORMAL)
 *
 *
 ***************************************************************************/
PacketRemoveObject::PacketRemoveObject(const CClient* target, CGrayUID uid) : PacketSend(XCMD_Remove, 5, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketRemoveObject::PacketRemoveObject");

	writeInt32(uid);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x20 : PacketPlayerPosition		updates player position (NORMAL)
 *
 *
 ***************************************************************************/
PacketPlayerPosition::PacketPlayerPosition(const CClient* target) : PacketSend(XCMD_View, 19, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPlayerPosition::PacketPlayerPosition");

	const CChar* character = target->GetChar();
	ASSERT(character);

	CREID_TYPE id;
	HUE_TYPE hue;
	target->GetAdjustedCharID(character, id, hue);

	const CPointMap& pt = character->GetTopPoint();

	writeInt32(character->GetUID());
	writeInt16(id);
	writeByte(0);
	writeInt16(hue);
	writeByte(character->GetModeFlag(false, target));
	writeInt16(pt.m_x);
	writeInt16(pt.m_y);
	writeInt16(0);
	writeByte(character->GetDirFlag());
	writeByte(pt.m_z);

	push(target);
};


/***************************************************************************
 *
 *
 *	Packet 0x21 : PacketMovementRej			rejects movement (HIGHEST)
 *
 *
 ***************************************************************************/
PacketMovementRej::PacketMovementRej(const CClient* target, BYTE sequence) : PacketSend(XCMD_WalkCancel, 8, g_Cfg.m_fUsePacketPriorities? PRI_HIGHEST : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketMovementRej::PacketMovementRej");

	const CChar* character = target->GetChar();
	ASSERT(character);

	const CPointMap& pt = character->GetTopPoint();
	writeByte(sequence);
	writeInt16(pt.m_x);
	writeInt16(pt.m_y);
	writeByte(character->GetDirFlag());
	writeByte(pt.m_z);
	push(target);
};


/***************************************************************************
 *
 *
 *	Packet 0x22 : PacketMovementAck			accepts movement (HIGHEST)
 *
 *
 ***************************************************************************/
PacketMovementAck::PacketMovementAck(const CClient* target) : PacketSend(XCMD_WalkAck, 3, g_Cfg.m_fUsePacketPriorities? PRI_HIGHEST : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketMovementAck::PacketMovementAck");

	writeByte(target->GetNetState()->m_sequence);
	writeByte(target->GetChar()->Noto_GetFlag(target->GetChar(), false, target->GetNetState()->isClientVersion(MINCLIVER_NOTOINVUL)));
	push(target);
}

/***************************************************************************
 *
 *
 *	Packet 0x23 : PacketDragAnimation		drag animation (LOW)
 *
 *
 ***************************************************************************/
PacketDragAnimation::PacketDragAnimation(const CChar* source, const CItem* item, const CObjBase* container, const CPointMap* pt) : PacketSend(XCMD_DragAnim, 26, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDragAnimation::PacketDragAnimation");

	writeInt16(item->GetDispID());
	writeByte(0);
	writeInt16(item->GetHue());
	writeInt16(item->GetAmount());

	const CPointMap& sourcepos = source->GetTopPoint();

	if (container != NULL)
	{
		// item is being dragged into a container
		const CObjBaseTemplate* target = container->GetTopLevelObj();
		const CPointMap& targetpos = target->GetTopPoint();

		writeInt32(source->GetUID());
		writeInt16(sourcepos.m_x);
		writeInt16(sourcepos.m_y);
		writeByte(sourcepos.m_z);
		writeInt32(target->GetUID());
		writeInt16(targetpos.m_x);
		writeInt16(targetpos.m_y);
		writeByte(targetpos.m_z);
	}
	else if (pt != NULL)
	{
		// item is being dropped onto the floor
		writeInt32(source->GetUID());
		writeInt16(sourcepos.m_x);
		writeInt16(sourcepos.m_y);
		writeByte(sourcepos.m_z);
		writeInt32(0);
		writeInt16(pt->m_x);
		writeInt16(pt->m_y);
		writeByte(pt->m_z);
	}
	else
	{
		// item is being picked up from the ground
		const CObjBaseTemplate* target = item->GetTopLevelObj();
		const CPointMap& targetpos = target->GetTopPoint();

		writeInt32((target == item)? 0 : (DWORD)target->GetUID());
		writeInt16(targetpos.m_x);
		writeInt16(targetpos.m_y);
		writeByte(targetpos.m_z);
		writeInt32(0);
		writeInt16(sourcepos.m_x);
		writeInt16(sourcepos.m_y);
		writeByte(sourcepos.m_z);
	}
}

bool PacketDragAnimation::canSendTo(const NetState* state) const
{
	// don't send to SA clients
	if (state->isClientSA() || state->isClientVersion(MINCLIVER_SA))
		return false;

	return PacketSend::canSendTo(state);
}

/***************************************************************************
 *
 *
 *	Packet 0x24 : PacketContainerOpen		open container gump (LOW)
 *
 *
 ***************************************************************************/
PacketContainerOpen::PacketContainerOpen(const CClient* target, const CObjBase* container, GUMP_TYPE gump) : PacketSend(XCMD_ContOpen, 9, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL), m_container(container->GetUID())
{
	ADDTOCALLSTACK("PacketContainerOpen::PacketContainerOpen");

	writeInt32(container->GetUID());
	writeInt16(gump);
	if (target->GetNetState()->isClientVersion(MINCLIVER_HIGHSEAS))
		writeInt16(0);

	trim();
	push(target);
}

bool PacketContainerOpen::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketContainerOpen::onSend");
#ifndef _MTNETWORK
	if (g_NetworkOut.isActive())
#else
	if (g_NetworkManager.isOutputThreaded())
#endif
		return true;

	return client->CanSee(m_container.ObjFind());
}


/***************************************************************************
 *
 *
 *	Packet 0x25 : PacketItemContainer		sends item in a container (NORMAL)
 *
 *
 ***************************************************************************/
PacketItemContainer::PacketItemContainer(const CClient* target, const CItem* item) : PacketSend(XCMD_ContAdd, 21, PRI_NORMAL), m_item(item->GetUID())
{
	ADDTOCALLSTACK("PacketItemContainer::PacketItemContainer");

	const CItemContainer* container = dynamic_cast<CItemContainer*>( item->GetParent() );
	const CPointBase& pt = item->GetContainedPoint();

	const CItemBase* itemDefinition = item->Item_GetDef();
	ITEMID_TYPE id = item->GetDispID();
	HUE_TYPE hue = item->GetHue() & HUE_MASK_HI;

	if (itemDefinition && target->GetResDisp() < itemDefinition->GetResLevel())
	{
		id = (ITEMID_TYPE)itemDefinition->GetResDispDnId();
		if (itemDefinition->GetResDispDnHue() != HUE_DEFAULT)
			hue = itemDefinition->GetResDispDnHue() & HUE_MASK_HI;
	}

	if (target->GetChar()->IsStatFlag(STATF_Hallucinating))
		hue = Calc_GetRandVal(HUE_DYE_HIGH); // restrict colours

	if (hue > HUE_QTY)
		hue &= HUE_MASK_LO;

	writeInt32(item->GetUID());
	writeInt16(id);
	writeByte(0);
	writeInt16(item->GetAmount());
	writeInt16(pt.m_x);
	writeInt16(pt.m_y);

	if (target->GetNetState()->isClientVersion(MINCLIVER_ITEMGRID) || target->GetNetState()->isClientKR() || target->GetNetState()->isClientSA())
		writeByte(item->GetContainedGridIndex());

	writeInt32(container->GetUID());
	writeInt16(hue);

	trim();
	push(target);
}

PacketItemContainer::PacketItemContainer(const CItem* spellbook, const CSpellDef* spell) : PacketSend(XCMD_ContAdd, 21, PRI_NORMAL), m_item(spellbook->GetUID())
{
	ADDTOCALLSTACK("PacketItemContainer::PacketItemContainer(2)");

	writeInt32(UID_F_ITEM|UID_O_INDEX_FREE|spell->m_idSpell);
	writeInt16(spell->m_idScroll);
	writeByte(0);
	writeInt16(spell->m_idSpell);
	writeInt16(0x48);
	writeInt16(0x7D);
}

void PacketItemContainer::completeForTarget(const CClient* target, const CItem* spellbook)
{
	ADDTOCALLSTACK("PacketItemContainer::completeForTarget");
	
	bool shouldIncludeGrid = (target->GetNetState()->isClientVersion(MINCLIVER_ITEMGRID) || target->GetNetState()->isClientKR() || target->GetNetState()->isClientSA());

	if (getLength() >= 20)
	{
		// only append the additional information if it needs to be changed
		bool containsGrid = getLength() == 21;
		if (shouldIncludeGrid == containsGrid)
			return;
	}

	seek(14);

	if (shouldIncludeGrid)
		writeByte(0);

	writeInt32(spellbook->GetUID());
	writeInt16(HUE_DEFAULT);

	trim();
}

bool PacketItemContainer::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketItemContainer::onSend");
#ifndef _MTNETWORK
	if (g_NetworkOut.isActive())
#else
	if (g_NetworkManager.isOutputThreaded())
#endif
		return true;

	return client->CanSee(m_item.ItemFind());
}


/***************************************************************************
 *
 *
 *	Packet 0x26 : PacketKick				notifies client they have been kicked (HIGHEST)
 *
 *
 ***************************************************************************/
PacketKick::PacketKick(const CClient* target) : PacketSend(XCMD_Kick, 5, PRI_HIGHEST)
{
	ADDTOCALLSTACK("PacketKick::PacketKick");

	writeInt32(0);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x27 : PacketDragCancel			cancel item drag (HIGH)
 *
 *
 ***************************************************************************/
PacketDragCancel::PacketDragCancel(const CClient* target, Reason code) : PacketSend(XCMD_DragCancel, 2, g_Cfg.m_fUsePacketPriorities? PRI_HIGH : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDragCancel::PacketDragCancel");

	writeByte(code);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x29 : PacketDropAccepted		notify drop accepted (kr) (NORMAL)
 *
 *
 ***************************************************************************/
PacketDropAccepted::PacketDropAccepted(const CClient* target) : PacketSend(XCMD_DropAccepted, 1, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDropAccepted::PacketDropAccepted");

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x2E : PacketItemEquipped		sends equipped item  (NORMAL)
 *
 *
 ***************************************************************************/
PacketItemEquipped::PacketItemEquipped(const CClient* target, const CItem* item) : PacketSend(XCMD_ItemEquip, 15, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketItemEquipped::PacketItemEquipped");

	const CChar* parent = dynamic_cast<CChar*>(item->GetParent());
	ASSERT(parent);

	LAYER_TYPE layer = item->GetEquipLayer();
	ITEMID_TYPE id;
	HUE_TYPE hue;
	target->GetAdjustedItemID(parent, item, id, hue);

	writeInt32(item->GetUID());
	writeInt16(layer == LAYER_BANKBOX? ITEMID_CHEST_SILVER : id);
	writeByte(0);
	writeByte(layer);
	writeInt32(parent->GetUID());
	writeInt16(hue);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x2F : PacketSwing				fight swing (LOW)
 *
 *
 ***************************************************************************/
PacketSwing::PacketSwing(const CClient* target, const CChar* defender) : PacketSend(XCMD_Fight, 10, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketSwing::PacketSwing");

	writeByte(0);
	writeInt32(target->GetChar()->GetUID());
	writeInt32(defender->GetUID());
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x3A : PacketSkills				character skills (LOW)
 *
 *
 ***************************************************************************/
PacketSkills::PacketSkills(const CClient* target, const CChar* character, SKILL_TYPE skill) : PacketSend(XCMD_Skill, 15, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketSkills::PacketSkills");

	initLength();

	if (character == NULL)
		character = target->GetChar();

	bool includeCaps = target->GetNetState()->isClientVersion(MINCLIVER_SKILLCAPS);
	if (skill >= SKILL_MAX)
	{
		// all skills
		if (includeCaps)
			writeByte(0x02);
		else
			writeByte(0x00);

		for (size_t i = 0; i < g_Cfg.m_iMaxSkill; i++)
		{
			if (g_Cfg.m_SkillIndexDefs.IsValidIndex((SKILL_TYPE)i) == false)
				continue;

			writeInt16((SKILL_TYPE)(i + 1));
			writeInt16(character->Skill_GetAdjusted((SKILL_TYPE)i));
			writeInt16(character->Skill_GetBase((SKILL_TYPE)i));
			writeByte(character->Skill_GetLock((SKILL_TYPE)i));
			if (includeCaps)
				writeInt16(character->Skill_GetMax((SKILL_TYPE)i));
		}

		writeInt16(0);
	}
	else
	{
		// one skill
		if (includeCaps)
			writeByte(0xDF);
		else
			writeByte(0xFF);

		writeInt16(skill);
		writeInt16(character->Skill_GetAdjusted(skill));
		writeInt16(character->Skill_GetBase(skill));
		writeByte(character->Skill_GetLock(skill));
		if (includeCaps)
			writeInt16(character->Skill_GetMax(skill));
	}

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x3B : PacketCloseVendor			close vendor menu (NORMAL)
 *
 *
 ***************************************************************************/
PacketCloseVendor::PacketCloseVendor(const CClient* target, const CChar* vendor) : PacketSend(XCMD_VendorBuy, 8, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketCloseVendor::PacketCloseVendor");

	initLength();
	writeInt32(vendor->GetUID());
	writeByte(0); // no items

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x3C : PacketItemContents		contents of an item (NORMAL)
 *
 *
 ***************************************************************************/
PacketItemContents::PacketItemContents(CClient* target, const CItemContainer* container, bool isShop, bool filterLayers) : PacketSend(XCMD_Content, 5, PRI_NORMAL), m_container(container->GetUID())
{
	ADDTOCALLSTACK("PacketItemContents::PacketItemContents");

	const CChar* viewer = target->GetChar();
	const CItemBase* itemDefinition;
	bool includeGrid = (target->GetNetState()->isClientVersion(MINCLIVER_ITEMGRID) || target->GetNetState()->isClientKR() || target->GetNetState()->isClientSA());

	initLength();
	skip(2);

	bool isLayerSent[LAYER_HORSE];
	memset(isLayerSent, false, sizeof(isLayerSent));

	m_count = 0;
	ITEMID_TYPE id;
	HUE_TYPE hue;
	LAYER_TYPE layer;
	int amount;
	CPointMap pos;
	for (const CItem* item = container->GetContentHead(); item != NULL && m_count < MAX_ITEMS_CONT; item = item->GetNext())
	{
		if (isShop == false && item->IsAttr(ATTR_INVIS) && viewer->CanSee(item) == false)
			continue;

		if (filterLayers == true)
		{
			layer = (LAYER_TYPE)item->GetContainedLayer();
			ASSERT(layer < LAYER_HORSE);
			switch (layer) // don't put these on a corpse.
			{
				case LAYER_NONE:
				case LAYER_PACK: // these display strange.
					continue;

				case LAYER_NEWLIGHT:
					continue;

				default:
					// make certain that no more than one of each layer goes out to client....crashes otherwise!!
					if (isLayerSent[layer])
						continue;
					isLayerSent[layer] = true;
					break;
			}
		}

		itemDefinition = item->Item_GetDef();
		id = item->GetDispID();
		amount = item->GetAmount();

		if (itemDefinition != NULL && target->GetResDisp() < itemDefinition->GetResLevel())
			id = (ITEMID_TYPE)itemDefinition->GetResDispDnId();

		if (isShop == true)
		{
			const CItemVendable* vendorItem = dynamic_cast<const CItemVendable*>(item);
			if (vendorItem == NULL || vendorItem->GetAmount() == 0 || vendorItem->IsType(IT_GOLD))
				continue;

			amount = minimum(g_Cfg.m_iVendorMaxSell, amount);
			pos.m_x = m_count + 1;
			pos.m_y = 1;
		}
		else
		{
			pos = item->GetContainedPoint();
		}

		if (viewer->IsStatFlag(STATF_Hallucinating))
		{
			hue = Calc_GetRandVal(HUE_DYE_HIGH);
		}
		else
		{
			hue = item->GetHue() & HUE_MASK_HI;

			if (itemDefinition != NULL && target->GetResDisp() < itemDefinition->GetResLevel())
			{
				if (itemDefinition->GetResDispDnHue() != HUE_DEFAULT)
					hue = itemDefinition->GetResDispDnHue() & HUE_MASK_HI;
			}

			if (hue > HUE_QTY)
				hue &= HUE_MASK_LO; // restrict colors
		}

		// write item data
		writeInt32(item->GetUID());
		writeInt16(id);
		writeByte(0);
		writeInt16(amount);
		writeInt16(pos.m_x);
		writeInt16(pos.m_y);
		if (includeGrid)	writeByte(item->GetContainedGridIndex());
		writeInt32(container->GetUID());
		writeInt16(hue);
		m_count++;

		// include tooltip
		target->addAOSTooltip(item, false, isShop);
	}

	// write item count
	size_t l = getPosition();
	seek(3);
	writeInt16(m_count);
	seek(l);

	push(target);
}

PacketItemContents::PacketItemContents(const CClient* target, const CItem* spellbook) : PacketSend(XCMD_Content, 5, PRI_NORMAL), m_container(spellbook->GetUID())
{
	ADDTOCALLSTACK("PacketItemContents::PacketItemContents(2)");

	bool includeGrid = (target->GetNetState()->isClientVersion(MINCLIVER_ITEMGRID) || target->GetNetState()->isClientKR() || target->GetNetState()->isClientSA());

	initLength();
	skip(2);

	m_count = 0;
	for (int i = SPELL_Clumsy; i < SPELL_BOOK_QTY; i++)
	{
		if (spellbook->IsSpellInBook((SPELL_TYPE)i) == false)
			continue;

		writeInt32(UID_F_ITEM + UID_O_INDEX_FREE + i);
		writeInt16(0x1F2E);
		writeByte(0);
		writeInt16(i);
		writeInt16(0);
		writeInt16(0);
		if (includeGrid)	writeByte(m_count);
		writeInt32(spellbook->GetUID());
		writeInt16(HUE_DEFAULT);

		m_count++;
	}

	// write item count
	size_t l = getPosition();
	seek(3);
	writeInt16(m_count);
	seek(l);

	push(target);
}

PacketItemContents::PacketItemContents(const CClient* target, const CItemContainer* spellbook) : PacketSend(XCMD_Content, 5, PRI_NORMAL), m_container(spellbook->GetUID())
{
	ADDTOCALLSTACK("PacketItemContents::PacketItemContents(3)");

	bool includeGrid = (target->GetNetState()->isClientVersion(MINCLIVER_ITEMGRID) || target->GetNetState()->isClientKR() || target->GetNetState()->isClientSA());
	const CSpellDef* spellDefinition;

	initLength();
	skip(2);

	m_count = 0;
	for (CItem* item = spellbook->GetContentHead(); item != NULL; item = item->GetNext())
	{
		if (item->IsType(IT_SCROLL) == false)
			continue;

		spellDefinition = g_Cfg.GetSpellDef((SPELL_TYPE)item->m_itSpell.m_spell);
		if (spellDefinition == NULL)
			continue;

		writeInt32(item->GetUID());
		writeInt16(spellDefinition->m_idScroll);
		writeByte(0);
		writeInt16(item->m_itSpell.m_spell);
		writeInt16(0);
		writeInt16(0);
		if (includeGrid)	writeByte(m_count);
		writeInt32(spellbook->GetUID());
		writeInt16(HUE_DEFAULT);

		m_count++;
	}

	// write item count
	size_t l = getPosition();
	seek(3);
	writeInt16(m_count);
	seek(l);

	push(target);
}

bool PacketItemContents::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketItemContents::onSend");

	if (m_count <= 0)
		return false;
#ifndef _MTNETWORK
	if (g_NetworkOut.isActive())
#else
	if (g_NetworkManager.isOutputThreaded())
#endif
		return true;

	return client->CanSee(m_container.ItemFind());
}


/***************************************************************************
 *
 *
 *	Packet 0x4F : PacketGlobalLight			sets global light level (NORMAL)
 *
 *
 ***************************************************************************/
PacketGlobalLight::PacketGlobalLight(const CClient* target, int light) : PacketSend(XCMD_Light, 2, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketGlobalLight::PacketGlobalLight");

	writeByte(light);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x53 : PacketWarningMessage		show popup warning message (NORMAL)
 *
 *
 ***************************************************************************/
PacketWarningMessage::PacketWarningMessage(const CClient* target, PacketWarningMessage::Message code) : PacketSend(XCMD_IdleWarning, 2, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketWarningMessage::PacketWarningMessage");

	writeByte(code);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x54 : PacketPlaySound			play a sound (NORMAL)
 *
 *
 ***************************************************************************/
PacketPlaySound::PacketPlaySound(const CClient* target, SOUND_TYPE sound, int flags, int volume, const CPointMap& pos) : PacketSend(XCMD_Sound, 12, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPlaySound::PacketPlaySound");

	writeByte(flags);
	writeInt16(sound);
	writeInt16(volume);
	writeInt16(pos.m_x);
	writeInt16(pos.m_y);
	writeInt16(pos.m_z);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x55 : PacketRedrawAll			redraw all (NORMAL)
 *
 *
 ***************************************************************************/
PacketRedrawAll::PacketRedrawAll(const CClient* target) : PacketSend(XCMD_ReDrawAll, 1, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketRedrawAll::PacketRedrawAll");

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x56 : PacketMapPlot				show/edit map plots (LOW)
 *
 *
 ***************************************************************************/
PacketMapPlot::PacketMapPlot(const CClient* target, const CItem* map, MAPCMD_TYPE mode, bool edit) : PacketSend(XCMD_MapEdit, 11, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketMapPlot::PacketMapPlot");

	writeInt32(map->GetUID());
	writeByte(mode);
	writeBool(edit);
	writeInt16(0);
	writeInt16(0);

	push(target);
}

PacketMapPlot::PacketMapPlot(const CItem* map, MAPCMD_TYPE mode, bool edit) : PacketSend(XCMD_MapEdit, 11, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketMapPlot::PacketMapPlot");

	writeInt32(map->GetUID());
	writeByte(mode);
	writeBool(edit);
}

void PacketMapPlot::setPin(int x, int y)
{
	ADDTOCALLSTACK("PacketMapPlot::PacketMapPlot");

	seek(7);
	writeInt16(x);
	writeInt16(y);
}


/***************************************************************************
 *
 *
 *	Packet 0x5B : PacketGameTime			current game time (IDLE)
 *
 *
 ***************************************************************************/
PacketGameTime::PacketGameTime(const CClient* target, int hours, int minutes, int seconds) : PacketSend(XCMD_Time, 4, g_Cfg.m_fUsePacketPriorities? PRI_IDLE : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketGameTime::PacketGameTime");

	writeByte(hours);
	writeByte(minutes);
	writeByte(seconds);
	push(target);
};


/***************************************************************************
 *
 *
 *	Packet 0x65 : PacketWeather				set current weather (IDLE)
 *
 *
 ***************************************************************************/
PacketWeather::PacketWeather(const CClient* target, WEATHER_TYPE weather, int severity, int temperature) : PacketSend(XCMD_Weather, 4, g_Cfg.m_fUsePacketPriorities? PRI_IDLE : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketWeather::PacketWeather");

	switch (weather)
	{
		case WEATHER_RAIN:
		case WEATHER_STORM:
		case WEATHER_SNOW:
			// fix weird client transition problem.
			// May only transition from Dry.
			writeByte(WEATHER_DRY);
			writeByte(0x00);
			writeByte(0x10);
			send(target);

			seek(1);
			writeByte(weather);
			writeByte(severity);
			break;

		default:
			writeByte(WEATHER_DRY);
			writeByte(0x00);
			break;
	}
	
	writeByte(temperature);
	push(target);
};


/***************************************************************************
 *
 *
 *	Packet 0x66 : PacketBookPageContent		send book page content (LOW)
 *
 *
 ***************************************************************************/
PacketBookPageContent::PacketBookPageContent(const CClient* target, const CItem* book, size_t startpage, size_t pagecount) : PacketSend(XCMD_BookPage, 8, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketBookPageContent::PacketBookPageContent");

	m_pages = 0;

	initLength();
	writeInt32(book->GetUID());
	writeInt16(0);

	for (size_t i = 0; i < pagecount; i++)
		addPage(book, startpage + i);

	push(target);
};

void PacketBookPageContent::addPage(const CItem* book, size_t page)
{
	ADDTOCALLSTACK("PacketBookPageContent::addPage");

	writeInt16(page);

	// skip line count for now
	size_t linesPos = getPosition();
	size_t lines = 0;
	writeInt16(0);

	if (book->IsBookSystem())
	{
		CResourceLock s;
		if (g_Cfg.ResourceLock(s, RESOURCE_ID(RES_BOOK, book->m_itBook.m_ResID.GetResIndex(), page)) == true)
		{
			while (s.ReadKey(false))
			{
				lines++;
				writeStringASCII(s.GetKey());
			}
		}
	}
	else
	{
		// user written book pages
		const CItemMessage* message = dynamic_cast<const CItemMessage*>(book);
		if (message != NULL)
		{
			if (page > 0 && page <= message->GetPageCount())
			{
				// copy the pages from the book
				LPCTSTR text = message->GetPageText(page - 1);
				if (text != NULL)
				{
					for (TCHAR ch = *text; ch != '\0'; ch = *(++text))
					{
						if (ch == '\t')
						{
							ch = '\0';
							lines++;
						}

						writeCharASCII(ch);
					}

					writeCharASCII('\0');
					lines++;
				}
			}
		}
	}

	size_t endPos = getPosition();

	// seek back to write line count
	seek(linesPos);
	writeInt16(lines);

	// seek further back to increment page count
	seek(7);
	writeInt16(++m_pages);

	// return to end
	seek(endPos);
}


/***************************************************************************
 *
 *
 *	Packet 0x6C : PacketAddTarget				adds target cursor to client (LOW)
 *	Packet 0x99 : PacketAddTarget				adds target cursor to client with multi (LOW)
 *
 *
 ***************************************************************************/
PacketAddTarget::PacketAddTarget(const CClient* target, PacketAddTarget::TargetType type, DWORD context, PacketAddTarget::Flags flags) : PacketSend(XCMD_Target, 19, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketAddTarget::PacketAddTarget");

	writeByte(type);
	writeInt32(context);
	writeByte(flags);

	// unused data
	writeInt32(0);
	writeInt16(0);
	writeInt16(0);
	writeByte(0);
	writeByte(0);
	writeInt16(0);

	push(target);
};

PacketAddTarget::PacketAddTarget(const CClient* target, PacketAddTarget::TargetType type, DWORD context, PacketAddTarget::Flags flags, ITEMID_TYPE id) : PacketSend(XCMD_TargetMulti, 30, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketAddTarget::PacketAddTarget(2)");

	writeByte(type);
	writeInt32(context);
	writeByte(flags);
	
	writeInt32(0);
	writeInt32(0);
	writeInt16(0);
	writeByte(0);

	writeInt16(id - ITEMID_MULTI);

	writeInt16(0); // x
	writeInt16(0); // y
	writeInt16(0); // z

	if (target->GetNetState()->isClientVersion(MINCLIVER_HIGHSEAS))
	{
		writeInt16(0);
		writeInt16(0); // hue
	}

	trim();
	push(target);
};


/***************************************************************************
 *
 *
 *	Packet 0x6D : PacketPlayMusic			adds music to the client (IDLE)
 *
 *
 ***************************************************************************/
PacketPlayMusic::PacketPlayMusic(const CClient* target, WORD musicID) : PacketSend(XCMD_PlayMusic, 3, g_Cfg.m_fUsePacketPriorities? PRI_IDLE : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPlayMusic::PacketPlayMusic");

	writeInt16(musicID);
	push(target);
};


/***************************************************************************
 *
 *
 *	Packet 0x6E : PacketAction				plays an animation (LOW)
 *
 *
 ***************************************************************************/
PacketAction::PacketAction(const CChar* character, ANIM_TYPE action, WORD repeat, bool backward, BYTE delay) : PacketSend(XCMD_CharAction, 14, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketAction::PacketAction");

	writeInt32(character->GetUID());
	writeInt16(action);
	writeByte(0);
	writeByte(character->m_dirFace);
	writeInt16(repeat);
	writeBool(backward);
	writeBool(repeat != 1);
	writeByte(delay);
};


/***************************************************************************
 *
 *
 *	Packet 0x6F : PacketTradeAction			perform a trade action (NORMAL)
 *
 *
 ***************************************************************************/
PacketTradeAction::PacketTradeAction(SECURE_TRADE_TYPE action) : PacketSend(XCMD_SecureTrade, 17, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketTradeAction::PacketTradeAction");

	initLength();
	writeByte(action);
}

void PacketTradeAction::prepareContainerOpen(const CChar* character, const CItem* container1, const CItem* container2)
{
	ADDTOCALLSTACK("PacketTradeAction::prepareContainerOpen");

	seek(4);
	writeInt32(character->GetUID());
	writeInt32(container1->GetUID());
	writeInt32(container2->GetUID());
	writeBool(true);
	writeStringFixedASCII(character->GetName(), 30);
}
	
void PacketTradeAction::prepareReadyChange(const CItemContainer* container1, const CItemContainer* container2)
{
	ADDTOCALLSTACK("PacketTradeAction::prepareReadyChange");

	seek(4);
	writeInt32(container1->GetUID());
	writeInt32(container1->m_itEqTradeWindow.m_fCheck);
	writeInt32(container2->m_itEqTradeWindow.m_fCheck);
	writeBool(false);
}


void PacketTradeAction::prepareClose(const CItemContainer* container)
{
	ADDTOCALLSTACK("PacketTradeAction::prepareClose");

	seek(4);
	writeInt32(container->GetUID());
	writeInt32(0);
	writeInt32(0);
	writeBool(false);
}


/***************************************************************************
 *
 *
 *	Packet 0x70 : PacketEffect				displays a visual effect (NORMAL)
 *	Packet 0xC0 : PacketEffect				displays a hued visual effect (NORMAL)
 *
 *
 ***************************************************************************/
PacketEffect::PacketEffect(const CClient* target, EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate* dst, const CObjBaseTemplate* src, BYTE speed, BYTE loop, bool explode) : PacketSend(XCMD_Effect, 20, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketEffect::PacketEffect");

	writeBasicEffect(motion, id, dst, src, speed, loop, explode);

	push(target);
}

PacketEffect::PacketEffect(const CClient* target, EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate* dst, const CObjBaseTemplate* src, BYTE speed, BYTE loop, bool explode, DWORD hue, DWORD render) : PacketSend(XCMD_EffectEx, 28, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketEffect::PacketEffect(2)");

	writeBasicEffect(motion, id, dst, src, speed, loop, explode);
	writeHuedEffect(hue, render);

	push(target);
}

void PacketEffect::writeBasicEffect(EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate* dst, const CObjBaseTemplate* src, BYTE speed, BYTE loop, bool explode)
{
	ADDTOCALLSTACK("PacketEffect::writeBasicEffect");

	bool oneDirection = true;
	dst = dst->GetTopLevelObj();
	CPointMap dstpos = dst->GetTopPoint();

	CPointMap srcpos;
	if (src != NULL && motion == EFFECT_BOLT)
	{
		src = src->GetTopLevelObj();
		srcpos = src->GetTopPoint();
	}
	else
		srcpos = dstpos;


	writeByte(motion);

	switch (motion)
	{
		case EFFECT_BOLT: // a targeted bolt
			if (src == NULL)
				src = dst;

			oneDirection = false;
			loop = 0; // does not apply.

			writeInt32(src->GetUID()); // source
			writeInt32(dst->GetUID());
			break;

		case EFFECT_LIGHTNING: // lightning bolt.
		case EFFECT_XYZ: // stay at current xyz
		case EFFECT_OBJ: // effect at single object.
			writeInt32(dst->GetUID());
			writeInt32(0);
			break;

		default: // unknown (should never happen)
			writeInt32(0);
			writeInt32(0);
			break;
	}

	writeInt16(id);
	writeInt16(srcpos.m_x);
	writeInt16(srcpos.m_y);
	writeByte(srcpos.m_z);
	writeInt16(dstpos.m_x);
	writeInt16(dstpos.m_y);
	writeByte(dstpos.m_z);
	writeByte(speed); // 0=very fast, 7=slow
	writeByte(loop); // 0=really long, 1=shortest, 6=longer
	writeInt16(0);
	writeByte(oneDirection);
	writeByte(explode);

}

void PacketEffect::writeHuedEffect(DWORD hue, DWORD render)
{
	ADDTOCALLSTACK("PacketEffect::writeHuedEffect");

	writeInt32(hue);
	writeInt32(render);
}


/***************************************************************************
 *
 *
 *	Packet 0x71 : PacketBulletinBoard		display a bulletin board or message (NORMAL/LOW)
 *
 *
 ***************************************************************************/
PacketBulletinBoard::PacketBulletinBoard(const CClient* target, const CItemContainer* board) : PacketSend(XCMD_BBoard, 20, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketBulletinBoard::PacketBulletinBoard");

	initLength();
	writeByte(BBOARDF_NAME);
	writeInt32(board->GetUID());
	writeStringASCII(board->GetName());

	push(target);
}

PacketBulletinBoard::PacketBulletinBoard(const CClient* target, BBOARDF_TYPE action, const CItemContainer* board, const CItemMessage* message) : PacketSend(XCMD_BBoard, 20, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketBulletinBoard::PacketBulletinBoard(2)");

	initLength();
	writeByte(action == BBOARDF_REQ_FULL? BBOARDF_MSG_BODY : BBOARDF_MSG_HEAD);
	writeInt32(board->GetUID());

	writeInt32(message->GetUID());
	if (action == BBOARDF_REQ_HEAD)
		writeInt32(0);

	size_t lenstr = 0;
	TCHAR* tempstr = Str_GetTemp();

	// author name. if it has one.
	if (message->m_sAuthor.IsEmpty())
	{
		writeByte(0x01);
		writeCharASCII('\0');
	}
	else
	{
		LPCTSTR author = message->m_sAuthor;
		if (target->IsPriv(PRIV_GM))
			author = target->GetChar()->GetName();

		lenstr = strlen(author) + 1;
		if (lenstr > 255) lenstr = 255;

		writeByte(lenstr);
		writeStringFixedASCII(author, lenstr);
	}

	// message title
	lenstr = strlen(message->GetName()) + 1;
	if (lenstr > 255) lenstr = 255;

	writeByte(lenstr);
	writeStringFixedASCII(message->GetName(), lenstr);

	// message time
	CServTime time = message->m_itBook.m_Time;
	sprintf(tempstr, "Day %lu", (g_World.GetGameWorldTime(time) / (24 * 60)) % 365);
	lenstr = strlen(tempstr) + 1;

	writeByte(lenstr);
	writeStringFixedASCII(tempstr, lenstr);

	if (action == BBOARDF_REQ_FULL)
	{
		// requesst for full message body
		writeInt32(0);

		size_t lines = message->GetPageCount();
		writeInt16(lines);

		for (size_t i = 0; i < lines; i++)
		{
			LPCTSTR text = message->GetPageText(i);
			if (text == NULL)
				continue;

			lenstr = strlen(text) + 2;
			if (lenstr > 255) lenstr = 255;

			writeByte(lenstr);
			writeStringFixedASCII(text, lenstr);
		}
	}

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x72 : PacketWarMode				update war mode status (LOW)
 *
 *
 ***************************************************************************/
PacketWarMode::PacketWarMode(const CClient* target, const CChar* character) : PacketSend(XCMD_War, 5, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketWarMode::PacketWarMode");

	writeBool(character->IsStatFlag(STATF_War));
	writeByte(0x00);
	writeByte(0x32);
	writeByte(0x00);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x73 : PacketPingAck				ping reply (IDLE)
 *
 *
 ***************************************************************************/
PacketPingAck::PacketPingAck(const CClient* target, BYTE value) : PacketSend(XCMD_Ping, 2, g_Cfg.m_fUsePacketPriorities? PRI_IDLE : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPingAck::PacketPingAck");

	writeByte(value);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x74 : PacketVendorBuyList		show list of vendor items (LOW)
 *
 *
 ***************************************************************************/
PacketVendorBuyList::PacketVendorBuyList(void) : PacketSend(XCMD_VendOpenBuy, 8, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
}

int PacketVendorBuyList::fillContainer(const CItemContainer* container, int convertFactor, size_t maxItems)
{
	ADDTOCALLSTACK("PacketVendorBuyList::fillContainer");

	seek(1); // just to be sure
	initLength();

	writeInt32(container->GetUID());

	size_t countpos = getPosition();
	skip(1);
	size_t count(0);

	for (CItem* item = container->GetContentHead(); item != NULL && count < maxItems; item = item->GetNext())
	{
		if (item->GetAmount() == 0)
			continue;

		CItemVendable* venditem = dynamic_cast<CItemVendable*>( item );
		if (venditem == NULL)
			continue;

		long price = venditem->GetVendorPrice(convertFactor);
		if (price == 0)
		{
			venditem->Item_GetDef()->ResetMakeValue();
			price = venditem->GetVendorPrice(convertFactor);

			if (price == 0 && venditem->IsValidNPCSaleItem())
				price = venditem->GetBasePrice();

			if (price == 0)
				price = 100000;
		}

		writeInt32(price);

		LPCTSTR name = venditem->GetName();
		size_t len = strlen(name) + 1;
		if (len > 255)
			len = 255;

		writeByte(len);
		writeStringFixedASCII(name, len);

		count++;
	}

	// seek back to write count
	size_t endpos = getPosition();
	seek(countpos);
	writeByte(count);
	seek(endpos);

	return count;
}


/***************************************************************************
 *
 *
 *	Packet 0x76 : PacketZoneChange			change server zone (LOW)
 *
 *
 ***************************************************************************/
PacketZoneChange::PacketZoneChange(const CClient* target, const CPointMap& pos) : PacketSend(XCMD_ZoneChange, 16, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketZoneChange::PacketZoneChange");

	writeInt16(pos.m_x);
	writeInt16(pos.m_y);
	writeInt16(pos.m_z);
	writeByte(0);
	writeInt16(0);
	writeInt16(0);
	writeInt16(g_MapList.GetX(pos.m_map));
	writeInt16(g_MapList.GetY(pos.m_map));

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x77 : PacketCharacterMove		move a character (NORMAL)
 *
 *
 ***************************************************************************/
PacketCharacterMove::PacketCharacterMove(const CClient* target, const CChar* character, BYTE direction) : PacketSend(XCMD_CharMove, 17, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketCharacterMove::PacketCharacterMove");

	CREID_TYPE id;
	HUE_TYPE hue;
	target->GetAdjustedCharID(character, id, hue);
	const CPointMap& pos = character->GetTopPoint();

	writeInt32(character->GetUID());
	writeInt16(id);
	writeInt16(pos.m_x);
	writeInt16(pos.m_y);
	writeByte(pos.m_z);
	writeByte(direction);
	writeInt16(hue);
	writeByte(character->GetModeFlag(character->CanSeeTrue(target->GetChar()), target));
	writeByte(character->Noto_GetFlag(target->GetChar(), false, target->GetNetState()->isClientVersion(MINCLIVER_NOTOINVUL)));

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x78 : PacketCharacter			create a character (NORMAL)
 *
 *
 ***************************************************************************/
PacketCharacter::PacketCharacter(CClient* target, const CChar* character) : PacketSend(XCMD_Char, 23, PRI_NORMAL), m_character(character->GetUID())
{
	ADDTOCALLSTACK("PacketCharacter::PacketCharacter");

	const CChar* viewer = target->GetChar();
	ASSERT(viewer);

	CREID_TYPE id;
	HUE_TYPE hue;
	target->GetAdjustedCharID(character, id, hue);
	const CPointMap& pos = character->GetTopPoint();

	initLength();
	writeInt32(character->GetUID());
	writeInt16(id);
	writeInt16(pos.m_x);
	writeInt16(pos.m_y);
	writeByte(pos.m_z);
	writeByte(character->GetDirFlag());
	writeInt16(hue);
	writeByte(character->GetModeFlag(character->CanSeeTrue(target->GetChar()), target));
	writeByte(character->Noto_GetFlag(target->GetChar(), false, target->GetNetState()->isClientVersion(MINCLIVER_NOTOINVUL)));

	if (character->IsStatFlag(STATF_Sleeping) == false)
	{
		bool isLayerSent[LAYER_HORSE+1];
		memset(isLayerSent, false, sizeof(isLayerSent));

		for (const CItem* item = character->GetContentHead(); item != NULL; item = item->GetNext())
		{
			LAYER_TYPE layer = item->GetEquipLayer();
			if (CItemBase::IsVisibleLayer(layer) == false)
				continue;
			if (viewer->CanSeeItem(item) == false && viewer != character)
				continue;

			// prevent same layer being sent twice
			ASSERT(layer <= LAYER_HORSE);
			if (isLayerSent[layer])
				continue;

			isLayerSent[layer] = true;

			target->addAOSTooltip(item);

			ITEMID_TYPE itemid;
			target->GetAdjustedItemID(character, item, itemid, hue);

			writeInt32(item->GetUID());

			if (hue != 0)
			{
				writeInt16(itemid | 0x8000);
				writeByte(layer);
				writeInt16(hue);
			}
			else
			{
				writeInt16(itemid);
				writeByte(layer);
			}
		}
	}

	writeInt32(0);

	push(target);
}

bool PacketCharacter::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketCharacter::onSend");
#ifndef _MTNETWORK
	if (g_NetworkOut.isActive())
#else
	if (g_NetworkManager.isOutputThreaded())
#endif
		return true;

	return client->CanSee(m_character.CharFind());
}


/***************************************************************************
 *
 *
 *	Packet 0x7C : PacketDisplayMenu			show a menu selection (LOW)
 *
 *
 ***************************************************************************/
PacketDisplayMenu::PacketDisplayMenu(const CClient* target, CLIMODE_TYPE mode, const CMenuItem* items, size_t count, const CObjBase* object) : PacketSend(XCMD_MenuItems, 11, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDisplayMenu::PacketDisplayMenu");

	initLength();
	writeInt32(object->GetUID());
	writeInt16(mode);

	int len = items[0].m_sText.GetLength();
	if (len > 255) len = 255;
	writeByte(len);
	writeStringFixedASCII((LPCTSTR)items[0].m_sText, len);

	writeByte(count);
	for (size_t i = 1; i <= count; i++)
	{
		writeInt16(items[i].m_id);
		writeInt16(items[i].m_color);

		len = items[i].m_sText.GetLength();
		if (len > 255) len = 255;
		writeByte(len);
		writeStringFixedASCII((LPCTSTR)items[i].m_sText, len);
	}

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x81 : PacketChangeCharacter		allow client to change character (LOW)
 *
 *
 ***************************************************************************/
PacketChangeCharacter::PacketChangeCharacter(CClient* target) : PacketSend(XCMD_CharList3, 5, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketChangeCharacter::PacketChangeCharacter");

	initLength();

	size_t countPos = getPosition();
	skip(1);

	writeByte(0);
	size_t count = target->Setup_FillCharList(this, target->GetChar());

	seek(countPos);
	writeByte(count);
	skip((count * 60) + 1);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x82 : PacketLoginError			login error response (HIGHEST)
 *
 *
 ***************************************************************************/
PacketLoginError::PacketLoginError(const CClient* target, PacketLoginError::Reason reason) : PacketSend(XCMD_LogBad, 2, PRI_HIGHEST)
{
	ADDTOCALLSTACK("PacketLoginError::PacketLoginError");

	writeByte(reason);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x85 : PacketDeleteError			delete character error response (LOW)
 *
 *
 ***************************************************************************/
PacketDeleteError::PacketDeleteError(const CClient* target, PacketDeleteError::Reason reason) : PacketSend(XCMD_DeleteBad, 2, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDeleteError::PacketDeleteError");

	writeByte(reason);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x86 : PacketCharacterListUpdate	update character list (LOW)
 *
 *
 ***************************************************************************/
PacketCharacterListUpdate::PacketCharacterListUpdate(CClient* target, const CChar* lastCharacter) : PacketSend(XCMD_CharList2, 4, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketCharacterListUpdate::PacketCharacterListUpdate");

	initLength();

	size_t countPos = getPosition();
	skip(1);

	size_t count = target->Setup_FillCharList(this, lastCharacter);

	seek(countPos);
	writeByte(count);
	skip(count * 60);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x88 : PacketPaperdoll			show paperdoll (LOW)
 *
 *
 ***************************************************************************/
PacketPaperdoll::PacketPaperdoll(const CClient* target, const CChar* character) : PacketSend(XCMD_PaperDoll, 66, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPaperdoll::PacketPaperdoll");

	unsigned int mode = 0;
	if (character->IsStatFlag(STATF_War))
		mode |= 0x1;
	if (character == target->GetChar() ||
		(g_Cfg.m_fCanUndressPets? (character->NPC_IsOwnedBy(target->GetChar())) : (target->IsPriv(PRIV_GM) && target->GetPrivLevel() > character->GetPrivLevel())) )
		mode |= 0x2;

	writeInt32(character->GetUID());

	if (character->IsStatFlag(STATF_Incognito))
	{
		writeStringFixedASCII(character->GetName(), 60);
	}
	else
	{
		TCHAR* text = Str_GetTemp();
		int len = 0;

		const CStoneMember* guildMember = character->Guild_FindMember(MEMORY_GUILD);
		if (guildMember != NULL && guildMember->IsAbbrevOn() && guildMember->GetParentStone()->GetAbbrev()[0])
		{
			len = sprintf(text, "%s [%s], %s", character->Noto_GetTitle(), guildMember->GetParentStone()->GetAbbrev(),
							guildMember->GetTitle()[0]? guildMember->GetTitle() : character->GetTradeTitle());
		}
		

		if (len <= 0)
			sprintf(text, "%s, %s", character->Noto_GetTitle(), character->GetTradeTitle());

		writeStringFixedASCII(text, 60);
	}

	writeByte(mode);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x89 : PacketCorpseEquipment		send corpse equipment (NORMAL)
 *
 *
 ***************************************************************************/
PacketCorpseEquipment::PacketCorpseEquipment(CClient* target, const CItemContainer* corpse) : PacketSend(XCMD_CorpEquip, 7, PRI_NORMAL), m_corpse(corpse->GetUID())
{
	ADDTOCALLSTACK("PacketCorpseEquipment::PacketCorpseEquipment");

	const CChar* viewer = target->GetChar();

	bool isLayerSent[LAYER_HORSE];
	memset(isLayerSent, false, sizeof(isLayerSent));

	initLength();
	writeInt32(corpse->GetUID());

	LAYER_TYPE layer;
	m_count = 0;

	for (const CItem* item = corpse->GetContentHead(); item != NULL && m_count < MAX_ITEMS_CONT; item = item->GetNext())
	{
		if (item->IsAttr(ATTR_INVIS) && viewer->CanSee(item) == false)
			continue;
		
		layer = (LAYER_TYPE)item->GetContainedLayer();
		ASSERT(layer < LAYER_HORSE);
		switch (layer) // don't put these on a corpse.
		{
			case LAYER_NONE:
			case LAYER_PACK: // these display strange.
				continue;

			case LAYER_NEWLIGHT:
				continue;

			default:
				// make certain that no more than one of each layer goes out to client....crashes otherwise!!
				if (isLayerSent[layer])
					continue;
				isLayerSent[layer] = true;
				break;
		}


		writeByte(layer);
		writeInt32(item->GetUID());
		m_count++;

		// include tooltip
		target->addAOSTooltip(item);
	}

	writeByte(0); // terminator
	push(target);
}

bool PacketCorpseEquipment::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketCorpseEquipment::onSend");

	if (m_count <= 0)
		return false;
#ifndef _MTNETWORK
	if (g_NetworkOut.isActive())
#else
	if (g_NetworkManager.isOutputThreaded())
#endif
		return true;

	return client->CanSee(m_corpse.ItemFind());
}


/***************************************************************************
 *
 *
 *	Packet 0x8B : PacketSignGump			show a sign (LOW)
 *
 *
 ***************************************************************************/
PacketSignGump::PacketSignGump(const CClient* target, const CObjBase* object, GUMP_TYPE gump, LPCTSTR unknown, LPCTSTR text) : PacketSend(XCMD_GumpTextDisp, 13, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketSignGump::PacketSignGump");

	initLength();
	writeInt32(object->GetUID());
	writeInt16(gump);

	if (unknown != NULL)
	{
		size_t len = strlen(unknown) + 1;
		writeInt16(len);
		writeStringFixedASCII(unknown, len);
	}
	else
	{
		writeInt16(0);
	}

	if (text != NULL)
	{
		size_t len = strlen(text) + 1;
		writeInt16(len);
		writeStringFixedASCII(text, len);
	}
	else
	{
		writeInt16(0);
	}

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x8C : PacketServerRelay			relay client to server (IDLE)
 *
 *
 ***************************************************************************/
PacketServerRelay::PacketServerRelay(const CClient* target, DWORD ip, WORD port, DWORD customerId) : PacketSend(XCMD_Relay, 11, g_Cfg.m_fUsePacketPriorities? PRI_IDLE : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketServerRelay::PacketServerRelay");
	m_customerId = customerId;

	writeByte((ip      ) & 0xFF);
	writeByte((ip >> 8 ) & 0xFF);
	writeByte((ip >> 16) & 0xFF);
	writeByte((ip >> 24) & 0xFF);
	writeInt16(port);
	writeInt32(customerId);

	push(target);
}

void PacketServerRelay::onSent(CClient* client)
{
	ADDTOCALLSTACK("PacketServerRelay::onSent");

	// in case the client decides not to establish a new connection, change over to the game encryption
	client->m_Crypt.InitFast(m_customerId, CONNECT_GAME); // init decryption table
	client->SetConnectType(client->m_Crypt.GetConnectType());
}


/***************************************************************************
 *
 *
 *	Packet 0x90 : PacketDisplayMap			display map (LOW)
 *
 *
 ***************************************************************************/
PacketDisplayMap::PacketDisplayMap(const CClient* target, const CItemMap* map, const CRectMap& rect) : PacketSend(XCMD_MapDisplay, 19, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDisplayMap::PacketDisplayMap");

	const CItemBase* itemDef = map->Item_GetDef();
	ASSERT(itemDef != NULL);

	WORD width = itemDef->m_ttMap.m_iGumpWidth > 0 ? itemDef->m_ttMap.m_iGumpWidth : CItemMap::DEFAULT_SIZE;
	WORD height = itemDef->m_ttMap.m_iGumpHeight > 0 ? itemDef->m_ttMap.m_iGumpHeight : CItemMap::DEFAULT_SIZE;

	writeInt32(map->GetUID());
	writeInt16(GUMP_MAP_2_NORTH);
	writeInt16(rect.m_left);
	writeInt16(rect.m_top);
	writeInt16(rect.m_right);
	writeInt16(rect.m_bottom);
	writeInt16(width);
	writeInt16(height);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x93 : PacketDisplayBook			display book (LOW)
 *
 *
 ***************************************************************************/
PacketDisplayBook::PacketDisplayBook(const CClient* target, CItem* book) : PacketSend(XCMD_BookOpen, 99, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDisplayBook::PacketDisplayBook");

	bool isWritable = false;
	int pages = 0;
	CGString title;
	CGString author;

	if (book->IsBookSystem())
	{
		isWritable = false;

		CResourceLock s;
		if (g_Cfg.ResourceLock(s, book->m_itBook.m_ResID))
		{
			while (s.ReadKeyParse())
			{
				switch (FindTableSorted(s.GetKey(), CItemMessage::sm_szLoadKeys, COUNTOF(CItemMessage::sm_szLoadKeys )-1))
				{
					case CIC_AUTHOR:
						author = s.GetArgStr();
						break;
					case CIC_PAGES:
						pages = s.GetArgVal();
						break;
					case CIC_TITLE:
						title = s.GetArgStr();
						break;
				}
			}
		}

		// make sure book is named
		if (title.IsEmpty() == false)
			book->SetName((LPCTSTR)title);
	}
	else
	{
		// user written book
		const CItemMessage* message = dynamic_cast<const CItemMessage*>(book);
		if (message != NULL)
		{
			isWritable = message->IsBookWritable();
			pages = isWritable? MAX_BOOK_PAGES : message->GetPageCount();
			title = message->GetName();
			author = message->m_sAuthor.IsEmpty()? g_Cfg.GetDefaultMsg(DEFMSG_BOOK_AUTHOR_UNKNOWN) : (LPCTSTR)message->m_sAuthor;
		}
	}


	writeInt32(book->GetUID());
	writeBool(isWritable);
	writeBool(isWritable);
	writeInt16(pages);
	writeStringFixedASCII((LPCTSTR)title, 60);
	writeStringFixedASCII((LPCTSTR)author, 30);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x95 : PacketShowDyeWindow		show dye window (LOW)
 *
 *
 ***************************************************************************/
PacketShowDyeWindow::PacketShowDyeWindow(const CClient* target, const CObjBase* object) : PacketSend(XCMD_DyeVat, 9, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketShowDyeWindow::PacketShowDyeWindow");

	ITEMID_TYPE id;
	if (object->IsItem())
	{
		const CItem* item = dynamic_cast<const CItem*>(object);
		ASSERT(item);
		id = item->GetDispID();
	}
	else
	{
		// get the item equiv for the creature
		const CChar* character = dynamic_cast<const CChar*>(object);
		ASSERT(character);
		id = character->Char_GetDef()->m_trackID;
	}

	writeInt32(object->GetUID());
	writeInt16(object->GetHue());
	writeInt16(id);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x98 : PacketAllNamesResponse	all names macro response (PRI_IDLE)
 *
 *
 ***************************************************************************/
PacketAllNamesResponse::PacketAllNamesResponse(const CClient* target, const CObjBase* object) : PacketSend(XCMD_AllNames3D, 37, g_Cfg.m_fUsePacketPriorities? PRI_IDLE : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketAllNamesResponse::PacketAllNamesResponse");

	initLength();
	writeInt32(object->GetUID());
	writeStringFixedASCII(object->GetName(), 30);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x9A : PacketAddPrompt			prompt for ascii text response (LOW)
 *	Packet 0xC2 : PacketAddPrompt			prompt for unicode text response (LOW)
 *
 *
 ***************************************************************************/
PacketAddPrompt::PacketAddPrompt(const CClient* target, CGrayUID context1, CGrayUID context2, bool useUnicode) : PacketSend(useUnicode? XCMD_PromptUNICODE : XCMD_Prompt, 16, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketAddPrompt::PacketAddPrompt");

	initLength();

	writeInt32(context1);
	writeInt32(context2);
	writeInt32(0);

	if (useUnicode == true)
	{
		writeStringFixedASCII("", 4);
		writeCharUNICODE('\0');
	}
	else
	{
		writeCharASCII('\0');
	}

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x9E : PacketVendorSellList		show list of items to sell (LOW)
 *
 *
 ***************************************************************************/
PacketVendorSellList::PacketVendorSellList(const CChar* vendor) : PacketSend(XCMD_VendOpenSell, 9, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketVendorSellList::PacketVendorSellList");

	initLength();
	writeInt32(vendor->GetUID());
}

size_t PacketVendorSellList::searchContainer(CClient* target, const CItemContainer* container, CItemContainer* stock1, CItemContainer* stock2, int convertFactor, size_t maxItems)
{
	ADDTOCALLSTACK("PacketVendorSellList::searchContainer");

	seek(7); // just to be sure

	size_t countpos = getPosition();
	skip(2);

	CItem* item = container->GetContentHead();
	if (item == NULL)
		return 0;

	size_t count(0);
	std::deque<const CItemContainer*> otherBoxes;

	for (;;)
	{
		if (item != NULL)
		{
			container = dynamic_cast<CItemContainer*>(item);
			if (container != NULL && container->GetCount() > 0)
			{
				if (container->IsSearchable())
					otherBoxes.push_back(container);
			}
			else
			{
				CItemVendable* vendItem = dynamic_cast<CItemVendable*>(item);
				if (vendItem != NULL)
				{
					CItemVendable* vendSell = CChar::NPC_FindVendableItem(vendItem, stock1, stock2);
					if (vendSell != NULL)
					{
						HUE_TYPE hue = vendItem->GetHue() & HUE_MASK_HI;
						if (hue > HUE_QTY)
							hue &= HUE_MASK_LO;

						writeInt32(vendItem->GetUID());
						writeInt16(vendItem->GetDispID());
						writeInt16(hue);
						writeInt16(vendItem->GetAmount());
						writeInt16(vendSell->GetVendorPrice(convertFactor));

						LPCTSTR name = vendItem->GetName();
						int len = strlen(name) + 1;
						if (len > 255) len = 255;

						writeInt16(len);
						writeStringFixedASCII(name, len);
						
						target->addAOSTooltip(vendItem, false, true);
						if (++count >= maxItems)
							break;
					}
				}
			}

			item = item->GetNext();
		}

		if (item == NULL)
		{
			if (otherBoxes.empty())
				break;

			container = otherBoxes.front();
			otherBoxes.pop_front();
			item = container->GetContentHead();
		}
	}

	// seek back to write count
	size_t endpos = getPosition();
	seek(countpos);
	writeInt16(count);
	seek(endpos);

	return count;
}


/***************************************************************************
 *
 *
 *	Packet 0xA1 : PacketHealthUpdate		update character health (LOW)
 *
 *
 ***************************************************************************/
PacketHealthUpdate::PacketHealthUpdate(const CChar* character, bool full) : PacketSend(XCMD_StatChngStr, 9, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketHealthUpdate::PacketHealthUpdate");

	writeInt32(character->GetUID());

	if (full == true)
	{
		writeInt16(character->Stat_GetMax(STAT_STR));
		writeInt16(character->Stat_GetVal(STAT_STR));
	}
	else
	{
		int iMaxHits = maximum(character->Stat_GetMax(STAT_STR), 1);

		writeInt16(50);
		writeInt16((character->Stat_GetVal(STAT_STR) * 50) / iMaxHits);
	}
}


/***************************************************************************
 *
 *
 *	Packet 0xA2 : PacketManaUpdate			update character mana (LOW)
 *
 *
 ***************************************************************************/
PacketManaUpdate::PacketManaUpdate(const CChar* character) : PacketSend(XCMD_StatChngInt, 9, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketManaUpdate::PacketManaUpdate");

	writeInt32(character->GetUID());
	writeInt16(character->Stat_GetMax(STAT_INT));
	writeInt16(character->Stat_GetVal(STAT_INT));
}


/***************************************************************************
 *
 *
 *	Packet 0xA3 : PacketStaminaUpdate		update character stamina (LOW)
 *
 *
 ***************************************************************************/
PacketStaminaUpdate::PacketStaminaUpdate(const CChar* character) : PacketSend(XCMD_StatChngDex, 9, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketStaminaUpdate::PacketStaminaUpdate");

	writeInt32(character->GetUID());
	writeInt16(character->Stat_GetMax(STAT_DEX));
	writeInt16(character->Stat_GetVal(STAT_DEX));
}


/***************************************************************************
 *
 *
 *	Packet 0xA5 : PacketWebPage				send client to a webpage (LOW)
 *
 *
 ***************************************************************************/
PacketWebPage::PacketWebPage(const CClient* target, LPCTSTR url) : PacketSend(XCMD_Web, 3, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketWebPage::PacketWebPage");

	initLength();
	writeStringASCII(url);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xA6 : PacketOpenScroll			open scroll message (LOW)
 *
 *
 ***************************************************************************/
PacketOpenScroll::PacketOpenScroll(const CClient* target, CResourceLock &s, SCROLL_TYPE type, DWORD context, LPCTSTR header) : PacketSend(XCMD_Scroll, 10, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketOpenScroll::PacketOpenScroll");

	initLength();

	writeByte(type);
	writeInt32(context);

	size_t lengthPosition(getPosition());
	skip(2);

	if (header)
	{
		writeStringASCII(header, false);
		writeCharASCII(0x0D);
		writeStringASCII("  ", false);
		writeCharASCII(0x0D);
	}

	while (s.ReadKey(false))
	{
		writeStringASCII(s.GetKey(), false);
		writeCharASCII(0x0D);
	}

	size_t endPosition(getPosition());
	size_t length = getPosition() - lengthPosition;
	seek(lengthPosition);
	writeInt16(length);
	seek(endPosition);

	writeCharASCII('\0');

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xA8 : PacketServerList			send server list (LOW)
 *
 *
 ***************************************************************************/
PacketServerList::PacketServerList(const CClient* target) : PacketSend(XCMD_ServerList, 46, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketServerList::PacketServerList");

	// clients before 4.0.0 require serverlist ips to be in reverse
	bool reverseIp = target->GetNetState()->isClientLessVersion(MAXCLIVER_REVERSEIP);

	initLength();
	writeByte(0xFF);

	size_t countPosition(getPosition());
	skip(2);
	int count(0);

	writeServerEntry(&g_Serv, ++count, reverseIp);

	//	too many servers in list can crash the client
#define	MAX_SERVERS_LIST	32
	for (size_t i = 0; count < MAX_SERVERS_LIST; i++)
	{
		CServerRef server = g_Cfg.Server_GetDef(i);
		if (server == NULL)
			break;

		writeServerEntry(server, ++count, reverseIp);
	}
#undef MAX_SERVERS_LIST

	size_t endPosition(getPosition());
	seek(countPosition);
	writeInt16(count);
	seek(endPosition);

	push(target);
}

void PacketServerList::writeServerEntry(const CServerRef& server, int index, bool reverseIp)
{
	ADDTOCALLSTACK("PacketServerList::writeServerEntry");

	int percentFull;
	if (server == &g_Serv)
		percentFull = maximum(0, minimum((server->StatGet(SERV_STAT_CLIENTS) * 100) / maximum(1, g_Cfg.m_iClientsMax), 100));
	else
		percentFull = minimum(server->StatGet(SERV_STAT_CLIENTS), 100);

	DWORD ip = server->m_ip.GetAddrIP();


	writeInt16(index);
	writeStringFixedASCII(server->GetName(), 32);
	writeByte(percentFull);
	writeByte(server->m_TimeZone);

	if (reverseIp)
	{
		// Clients less than 4.0.0 require IP to be sent in reverse
		writeByte((ip      ) & 0xFF);
		writeByte((ip >> 8 ) & 0xFF);
		writeByte((ip >> 16) & 0xFF);
		writeByte((ip >> 24) & 0xFF);
	}
	else
	{
		// Clients since 4.0.0 require IP to be sent in order
		writeByte((ip >> 24) & 0xFF);
		writeByte((ip >> 16) & 0xFF);
		writeByte((ip >> 8 ) & 0xFF);
		writeByte((ip      ) & 0xFF);
	}
}


/***************************************************************************
 *
 *
 *	Packet 0xA9 : PacketCharacterList		send character list (LOW)
 *
 *
 ***************************************************************************/
PacketCharacterList::PacketCharacterList(CClient* target, const CChar* lastCharacter) : PacketSend(XCMD_CharList, 9, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketCharacterList::PacketCharacterList");

	const CAccountRef account = target->GetAccount();
	ASSERT(account != NULL);
	DWORD tmVer = account->m_TagDefs.GetKeyNum("clientversion");
	DWORD tmVerReported = account->m_TagDefs.GetKeyNum("reportedcliver");

	initLength();

	size_t countPos = getPosition();
	skip(1);

	size_t count = target->Setup_FillCharList(this, lastCharacter);

	seek(countPos);
	writeByte(count);
	skip(count * 60);

	
	int startCount = g_Cfg.m_StartDefs.GetCount();
	writeByte(startCount);
	
	// since 7.0.13.0, start locations have extra information
	bool includeExtraStartInfo = (tmVer >= MINCLIVER_EXTRASTARTINFO || tmVerReported >= MINCLIVER_EXTRASTARTINFO);

	if (includeExtraStartInfo)
	{
		// newer clients receive additional start info
		for (int i = 0; i < startCount; i++)
		{
			const CStartLoc* start = g_Cfg.m_StartDefs[i];

			writeByte(i + 1);
			writeStringFixedASCII((LPCTSTR)start->m_sArea, MAX_NAME_SIZE + 2);
			writeStringFixedASCII((LPCTSTR)start->m_sName, MAX_NAME_SIZE + 2);
			writeInt32(start->m_pt.m_x);
			writeInt32(start->m_pt.m_y);
			writeInt32(start->m_pt.m_z);
			writeInt32(start->m_pt.m_map);
			writeInt32(1149559); // todo: add support for 'description' cliloc
			writeInt32(0);
		}
	}
	else
	{
		for (int i = 0; i < startCount; i++)
		{
			writeByte(i + 1);
			writeStringFixedASCII((LPCTSTR)g_Cfg.m_StartDefs[i]->m_sArea, MAX_NAME_SIZE + 1);
			writeStringFixedASCII((LPCTSTR)g_Cfg.m_StartDefs[i]->m_sName, MAX_NAME_SIZE + 1);
		}
	}

	writeInt32(g_Cfg.GetPacketFlag(true, (RESDISPLAY_VERSION)account->GetResDisp(), maximum(account->GetMaxChars(), account->m_Chars.GetCharCount())));
	
	if (includeExtraStartInfo)
		writeInt16(0);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xAA : PacketAttack				set attack target (NORMAL)
 *
 *
 ***************************************************************************/
PacketAttack::PacketAttack(const CClient* target, CGrayUID serial) : PacketSend(XCMD_AttackOK, 5, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketAttack::PacketAttack");

	writeInt32(serial);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xAB : PacketGumpValueInput		show input dialog (LOW)
 *
 *
 ***************************************************************************/
PacketGumpValueInput::PacketGumpValueInput(const CClient* target, bool cancel, INPVAL_STYLE style, DWORD maxLength, LPCTSTR text, LPCTSTR caption, CObjBase* object) : PacketSend(XCMD_GumpInpVal, 21, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketGumpValueInput::PacketGumpValueInput");

	initLength();
	writeInt32(object->GetUID());
	writeInt16(CLIMODE_INPVAL);

	int len = strlen(text) + 1;
	if (len > 255) len = 255;

	writeInt16(len);
	writeStringFixedASCII(text, len);

	writeBool(cancel);
	writeByte(style);
	writeInt32(maxLength);

	TCHAR* z(NULL);
	switch (style)
	{
		case INPVAL_STYLE_NOEDIT: // None
		default:
			len = 1;
			break;

		case INPVAL_STYLE_TEXTEDIT: // Text
			z = Str_GetTemp();
			len = sprintf(z, "%s (%lu chars max)", caption, maxLength) + 1;
			break;

		case INPVAL_STYLE_NUMEDIT: // Numeric
			z = Str_GetTemp();
			len = sprintf(z, "%s (0 - %lu)", caption, maxLength) + 1;
			break;
	}

	if (len > 255) len = 255;
	writeInt16(len);
	writeStringFixedASCII(z, len);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xAE: PacketMessageUNICODE			show message to client (NORMAL)
 *
 *
 ***************************************************************************/
PacketMessageUNICODE::PacketMessageUNICODE(const CClient* target, const NWORD* pszText, const CObjBaseTemplate * source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID language) : PacketSend(XCMD_SpeakUNICODE, 48, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketMessageUNICODE::PacketMessageUNICODE");

	initLength();

	if (source == NULL)
		writeInt32(0xFFFFFFFF);
	else
		writeInt32(source->GetUID());
	
	if (source == NULL || source->IsChar() == false)
	{
		writeInt16(0xFFFF);
	}
	else
	{
		const CChar* sourceCharacter = dynamic_cast<const CChar*>(source);
		ASSERT(sourceCharacter);
		writeInt16(sourceCharacter->GetDispID());
	}
	
	writeByte(mode);
	writeInt16(hue);
	writeInt16(font);
	writeStringFixedASCII(language.GetStr(), 4);

	if (source == NULL)
		writeStringFixedASCII("System", 30);
	else
		writeStringFixedASCII(source->GetName(), 30);

	writeStringUNICODE((const WCHAR*)pszText);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xAF : PacketDeath				notifies about character death (NORMAL)
 *
 *
 ***************************************************************************/
PacketDeath::PacketDeath(CChar* dead, CItemCorpse* corpse) : PacketSend(XCMD_CharDeath, 13, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDeath::PacketDeath");

	writeInt32(dead->GetUID());
	writeInt32(corpse == NULL? 0 : (DWORD)corpse->GetUID());
	writeInt32(0);
}


/***************************************************************************
 *
 *
 *	Packet 0xB0 : PacketGumpDialog			displays a dialog gump (LOW)
 *	Packet 0xDD : PacketGumpDialog			displays a dialog gump using compression (LOW)
 *
 *
 ***************************************************************************/
PacketGumpDialog::PacketGumpDialog(int x, int y, CObjBase* object, DWORD context) : PacketSend(XCMD_GumpDialog, 24, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketGumpDialog::PacketGumpDialog");

	initLength();

	writeInt32(object->GetUID());
	writeInt32(context);
	writeInt32(x);
	writeInt32(y);
}

void PacketGumpDialog::writeControls(const CClient* target, const CGString* controls, size_t controlCount, const CGString* texts, size_t textCount)
{	
	ADDTOCALLSTACK("PacketGumpDialog::writeControls");

	const NetState* net = target->GetNetState();
	if (net->isClientVersion(MINCLIVER_COMPRESSDIALOG) || net->isClientKR() || net->isClientSA())
		writeCompressedControls(controls, controlCount, texts, textCount);
	else
		writeStandardControls(controls, controlCount, texts, textCount);
}

void PacketGumpDialog::writeCompressedControls(const CGString* controls, size_t controlCount, const CGString* texts, size_t textCount)
{
	ADDTOCALLSTACK("PacketGumpDialog::writeCompressedControls");

	seek(0);
	writeByte(XCMD_CompressedGumpDialog);

	seek(19);

	{
		// compress and write controls
		int controlLength = 1;
		for (size_t i = 0; i < controlCount; i++)
			controlLength += controls[i].GetLength() + 2;

		char* toCompress = new char[controlLength];

		int controlLengthActual = 0;
		for (size_t i = 0; i < controlCount; i++)
			controlLengthActual += sprintf(&toCompress[controlLengthActual], "{%s}", (LPCTSTR)controls[i]);
		controlLengthActual++;

		ASSERT(controlLengthActual == controlLength);

		z_uLong compressLength = z_compressBound(controlLengthActual);
		BYTE* compressBuffer = new BYTE[compressLength];

		int error = z_compress2(compressBuffer, &compressLength, (BYTE*)toCompress, controlLengthActual, Z_DEFAULT_COMPRESSION);
		delete[] toCompress;

		if (error != Z_OK || compressLength <= 0)
		{
			delete[] compressBuffer;
			g_Log.EventError("Compress failed with error %d when generating gump. Using old packet.\n", error);

			writeStandardControls(controls, controlCount, texts, textCount);
			return;
		}

		writeInt32(compressLength + 4);
		writeInt32(controlLengthActual);
		writeData(compressBuffer, compressLength);

		delete[] compressBuffer;
	}

	{
		// compress and write texts
		size_t textsPosition(getPosition());

		for (size_t i = 0; i < textCount; i++)
		{
			writeInt16(texts[i].GetLength());
			writeStringFixedNUNICODE((LPCTSTR)texts[i], texts[i].GetLength());
		}

		size_t textsLength = getPosition() - textsPosition;
		
		z_uLong compressLength = z_compressBound(textsLength);
		BYTE* compressBuffer = new BYTE[compressLength];

		int error = z_compress2(compressBuffer, &compressLength, &m_buffer[textsPosition], textsLength, Z_DEFAULT_COMPRESSION);
		if (error != Z_OK || compressLength <= 0)
		{
			delete[] compressBuffer;

			g_Log.EventError("Compress failed with error %d when generating gump. Using old packet.\n", error);

			writeStandardControls(controls, controlCount, texts, textCount);
			return;
		}

		seek(textsPosition);
		writeInt32(textCount);
		writeInt32(compressLength + 4);
		writeInt32(textsLength);
		writeData(compressBuffer, compressLength);

		delete[] compressBuffer;
	}
}

void PacketGumpDialog::writeStandardControls(const CGString* controls, size_t controlCount, const CGString* texts, size_t textCount)
{
	ADDTOCALLSTACK("PacketGumpDialog::writeStandardControls");

	seek(0);
	writeByte(XCMD_GumpDialog);

	seek(19);

	// skip controls length until they're written
	size_t controlLengthPosition(getPosition());
	skip(2);

	// write controls
	for (size_t i = 0; i < controlCount; i++)
	{
		writeCharASCII('{');
		writeStringASCII((LPCTSTR)controls[i], false);
		writeCharASCII('}');
	}

	// write controls length
	size_t endPosition(getPosition());
	seek(controlLengthPosition);
	writeInt16((WORD)(endPosition - controlLengthPosition - 2));
	seek(endPosition);

	// write texts
	writeInt16(textCount);
	for (size_t i = 0; i < textCount; i++)
	{
		writeInt16(texts[i].GetLength());
		writeStringFixedNUNICODE((LPCTSTR)texts[i], texts[i].GetLength());
	}
}


/***************************************************************************
 *
 *
 *	Packet 0xB2 : PacketChatMessage			send a chat system message (LOW)
 *
 *
 ***************************************************************************/
PacketChatMessage::PacketChatMessage(const CClient* target, CHATMSG_TYPE type, LPCTSTR param1, LPCTSTR param2, CLanguageID language) : PacketSend(XCMD_ChatReq, 11, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketChatMessage::PacketChatMessage");

	initLength();
	writeInt16(type);
	writeStringFixedASCII(language.GetStr(), 4);

	if (param1 != NULL)
		writeStringNUNICODE(param1);
	else
		writeCharNUNICODE('\0');

	if (param2 != NULL)
		writeStringNUNICODE(param2);
	else
		writeCharNUNICODE('\0');

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xB7 : PacketTooltip				send a tooltip (IDLE)
 *
 *
 ***************************************************************************/
PacketTooltip::PacketTooltip(const CClient* target, const CObjBase* object, LPCTSTR text) : PacketSend(XCMD_ToolTip, 8, g_Cfg.m_fUsePacketPriorities? PRI_IDLE : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketTooltip::PacketTooltip");

	initLength();
	writeInt32(object->GetUID());
	writeStringNUNICODE(text);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xB8 : PacketProfile				send a character profile (LOW)
 *
 *
 ***************************************************************************/
PacketProfile::PacketProfile(const CClient* target, const CChar* character) : PacketSend(XCMD_CharProfile, 12, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketProfile::PacketProfile");

	// alter profile when viewing an incognitoed player, unless being viewed by a GM or the profile is our own
	bool isIncognito = character->IsStatFlag(STATF_Incognito) && !target->IsPriv(PRIV_GM) && character != target->GetChar();

	initLength();

	writeInt32(character->GetUID());
	writeStringASCII(character->GetName());

	if (isIncognito == false)
	{
		CGString sConstText;
		sConstText.Format("%s, %s", character->Noto_GetTitle(), character->GetTradeTitle());

		writeStringNUNICODE((LPCTSTR)sConstText);

		if (character->m_pPlayer != NULL)
			writeStringNUNICODE((LPCTSTR)character->m_pPlayer->m_sProfile);
		else
			writeCharNUNICODE('\0');
	}
	else
	{
		writeStringNUNICODE(character->Noto_GetTitle());
		writeCharNUNICODE('\0');
	}

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xB9 : PacketEnableFeatures		enable client features (NORMAL)
 *
 *
 ***************************************************************************/
PacketEnableFeatures::PacketEnableFeatures(const CClient* target, DWORD flags) : PacketSend(XCMD_Features, 5, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketEnableFeatures::PacketEnableFeatures");

	const CAccountRef account = target->GetAccount();
	ASSERT(account != NULL);
	DWORD tmVer = account->m_TagDefs.GetKeyNum("clientversion");
	DWORD tmVerReported = account->m_TagDefs.GetKeyNum("reportedcliver");
	
	// since 6.0.14.2, feature flags are 4 bytes instead of 2.
	if (tmVer >= MINCLIVER_EXTRAFEATURES || tmVerReported >= MINCLIVER_EXTRAFEATURES)
		writeInt32(flags);
	else
		writeInt16((WORD)flags);

	trim();
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBA : PacketArrowQuest			display onscreen arrow for client to follow (NORMAL)
 *
 *
 ***************************************************************************/
PacketArrowQuest::PacketArrowQuest(const CClient* target, int x, int y, int id) : PacketSend(XCMD_Arrow, 10, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketArrowQuest::PacketArrowQuest");

	writeBool(x && y);
	writeInt16(x);
	writeInt16(y);

	if (target->GetNetState()->isClientVersion(MINCLIVER_HIGHSEAS) || target->GetNetState()->isClientSA())
		writeInt32(id);
	
	trim();
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBC : PacketSeason				change season (NORMAL)
 *
 *
 ***************************************************************************/
PacketSeason::PacketSeason(const CClient* target, SEASON_TYPE season, bool playMusic) : PacketSend(XCMD_Season, 3, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketSeason::PacketSeason");

	writeByte(season);
	writeBool(playMusic);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF : PacketExtended			extended command
 *
 *
 ***************************************************************************/
PacketExtended::PacketExtended(EXTDATA_TYPE type, size_t len, Priority priority) : PacketSend(XCMD_ExtData, len, priority)
{
	ADDTOCALLSTACK("PacketExtended::PacketExtended");

	initLength();

	writeInt16(type);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x01 : PacketFastWalk		send fask walk keys (NORMAL)
 *
 *
 ***************************************************************************/
PacketFastWalk::PacketFastWalk(const CClient* target, DWORD* codes, size_t count, size_t sendCount) : PacketExtended(EXTDATA_WalkCode_Prime, 29, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketFastWalk::PacketFastWalk");

	for (size_t i = count - sendCount; i < count; i++)
		writeInt32(codes[i]);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x04 : PacketGumpChange		change gump (LOW)
 *
 *
 ***************************************************************************/
PacketGumpChange::PacketGumpChange(const CClient* target, DWORD context, int buttonId) : PacketExtended(EXTDATA_GumpChange, 13, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketGumpChange::PacketGumpChange");

	writeInt32(context);
	writeInt32(buttonId);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x06 : PacketParty			party packet
 *
 *
 ***************************************************************************/
PacketParty::PacketParty(PARTYMSG_TYPE type, size_t len, Priority priority) : PacketExtended(EXTDATA_Party_Msg, len, priority)
{
	ADDTOCALLSTACK("PacketParty::PacketParty");

	writeByte(type);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x06.0x01 : PacketPartyList		send list of party members (NORMAL)
 *
 *
 ***************************************************************************/
PacketPartyList::PacketPartyList(const CCharRefArray* members) : PacketParty(PARTYMSG_Add, 11, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPartyList::PacketPartyList");

	size_t iQty = members->GetCharCount();

	writeByte(iQty);

	for (size_t i = 0; i < iQty; i++)
		writeInt32(members->GetChar(i));
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x06.0x02 : PacketPartyRemoveMember		remove member from party (NORMAL)
 *
 *
 ***************************************************************************/
PacketPartyRemoveMember::PacketPartyRemoveMember(const CChar* member, const CCharRefArray* members) : PacketParty(PARTYMSG_Remove, 11, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPartyRemoveMember::PacketPartyRemoveMember");

	ASSERT(member != NULL);

	size_t iQty = members == NULL? 0 : members->GetCharCount();

	writeByte(iQty);
	writeInt32(member->GetUID());

	for (size_t i = 0; i < iQty; i++)
		writeInt32(members->GetChar(i));
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x06.0x04 : PacketPartyChat		send party chat message (NORMAL)
 *
 *
 ***************************************************************************/
PacketPartyChat::PacketPartyChat(const CChar* source, const NCHAR* text) : PacketParty(PARTYMSG_Msg, 11, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPartyChat::PacketPartyChat");

	writeInt32(source->GetUID());
	writeStringUNICODE((const WCHAR*)text);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x06.0x07 : PacketPartyInvite	send party invitation (NORMAL)
 *
 *
 ***************************************************************************/
PacketPartyInvite::PacketPartyInvite(const CClient* target, const CChar* inviter) : PacketParty(PARTYMSG_NotoInvited, 10, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPartyInvite::PacketPartyInvite");

	writeInt32(inviter->GetUID());
	
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x08 : PacketMapChange			change map (NORMAL)
 *
 *
 ***************************************************************************/
PacketMapChange::PacketMapChange(const CClient* target, int map) : PacketExtended(EXTDATA_Map_Change, 6, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketMapChange::PacketMapChange");

	writeByte(map);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x10 : PacketPropertyListVersionOld		property (tool tip) version (LOW)
 *
 *
 ***************************************************************************/
PacketPropertyListVersionOld::PacketPropertyListVersionOld(const CClient* target, const CObjBase* object, DWORD version) : PacketExtended(EXTDATA_OldAOSTooltipInfo, 13, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPropertyListVersionOld::PacketPropertyListVersionOld");

	m_object = object->GetUID();

	writeInt32(object->GetUID());
	writeInt32(version);

	if (target != NULL)
		push(target, false);
}

bool PacketPropertyListVersionOld::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketPropertyListVersionOld::onSend");
#ifndef _MTNETWORK
	if (g_NetworkOut.isActive())
#else
	if (g_NetworkManager.isOutputThreaded())
#endif
		return true;

	const CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	const CObjBase* object = m_object.ObjFind();
	if (object == NULL || character->GetTopDistSight(object->GetTopLevelObj()) > UO_MAP_VIEW_SIZE)
		return false;

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x14 : PacketDisplayPopup		display popup menu (LOW)
 *
 *
 ***************************************************************************/
PacketDisplayPopup::PacketDisplayPopup(const CClient* target, CGrayUID uid) : PacketExtended(EXTDATA_Popup_Display, 12, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDisplayPopup::PacketDisplayPopup");

	m_popupCount = 0;
	m_isKr = target->GetNetState()->isClientKR();

	if (m_isKr)
		writeInt16(2);
	else
		writeInt16(1);

	writeInt32(uid);

	writeByte(0); // popup count
}

void PacketDisplayPopup::addOption(WORD entryTag, WORD textId, WORD flags, WORD color)
{
	ADDTOCALLSTACK("PacketDisplayPopup::addOption");

	if (m_popupCount >= MAX_POPUPS)
	{
		DEBUG_ERR(("Bad AddContextEntry usage: Too many entries!\n"));
		return;
	}

	if (m_isKr)
	{
		if (flags & POPUPFLAG_COLOR)
			flags &= ~POPUPFLAG_COLOR;

		writeInt32(3000000 + textId);
		writeInt16(entryTag);
		writeInt16(flags);
	}
	else
	{
		writeInt16(entryTag);
		writeInt16(textId);
		writeInt16(flags);

		if (flags & POPUPFLAG_COLOR)
			writeInt16(color);
	}

	m_popupCount++;
}

void PacketDisplayPopup::finalise(void)
{
	ADDTOCALLSTACK("PacketDisplayPopup::finalise");

	size_t endPosition(getPosition());

	seek(11);
	writeByte(m_popupCount);

	seek(endPosition);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x18 : PacketEnableMapDiffs		enable use of map diff files (NORMAL)
 *
 *
 ***************************************************************************/
PacketEnableMapDiffs::PacketEnableMapDiffs(const CClient* target) : PacketExtended(EXTDATA_Map_Diff, 13, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketEnableMapDiffs::PacketEnableMapDiffs");

	int mapCount = 1;
	int map;

	// find map count
	for (map = 255; map >= 0; map--)
	{
		if (g_MapList.m_maps[map] == false)
			continue;

		mapCount = map;
		break;
	}

	writeInt32(mapCount);

	for (map = 0; map < mapCount; map++)
	{
		if (g_Cfg.m_fUseMapDiffs && g_MapList.m_maps[map])
		{
			if (g_Install.m_Mapdifl[map].IsFileOpen())
				writeInt32(g_Install.m_Mapdifl[map].GetLength() / 4);
			else
				writeInt32(0);

			if (g_Install.m_Stadifl[map].IsFileOpen())
				writeInt32(g_Install.m_Stadifl[map].GetLength() / 4);
			else
				writeInt32(0);
		}
		else
		{
			// mapdiffs are disabled or map does not exist
			writeInt32(0);
			writeInt32(0);
		}
	}

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x19.0x02 : PacketStatLocks		update lock status of stats (NORMAL)
 *
 *
 ***************************************************************************/
PacketStatLocks::PacketStatLocks(const CClient* target, const CChar* character) : PacketExtended(EXTDATA_Stats_Enable, 12, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketStatLocks::PacketStatLocks");

	BYTE status(0);
	if (character->m_pPlayer != NULL)
	{
		status |= (BYTE)character->m_pPlayer->Stat_GetLock(STAT_INT);
		status |= (BYTE)character->m_pPlayer->Stat_GetLock(STAT_DEX) << 2;
		status |= (BYTE)character->m_pPlayer->Stat_GetLock(STAT_STR) << 4;
	}

	writeByte(0x02);
	writeInt32(character->GetUID());
	writeByte(0);
	writeByte(status);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x1B : PacketSpellbookContent	spellbook content (NORMAL)
 *
 *
 ***************************************************************************/
PacketSpellbookContent::PacketSpellbookContent(const CClient* target, const CItem* spellbook, WORD offset) : PacketExtended(EXTDATA_NewSpellbook, 23, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketSpellbookContent::PacketSpellbookContent");

	writeInt16(1);
	writeInt32(spellbook->GetUID());
	writeInt16(spellbook->GetDispID());
	writeInt16(offset);
	writeInt64(spellbook->m_itSpellbook.m_spells1, spellbook->m_itSpellbook.m_spells2);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x1D : PacketHouseDesignVersion	house design version (LOW)
 *
 *
 ***************************************************************************/
PacketHouseDesignVersion::PacketHouseDesignVersion(const CClient* target, const CItemMultiCustom* house) : PacketExtended(EXTDATA_HouseDesignVer, 13, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketHouseDesignVersion::PacketHouseDesignVersion");

	writeInt32(house->GetUID());
	writeInt32(house->GetRevision(target));

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x20.0x04 : PacketHouseBeginCustomise	begin house customisation (NORMAL)
 *
 *
 ***************************************************************************/
PacketHouseBeginCustomise::PacketHouseBeginCustomise(const CClient* target, const CItemMultiCustom* house) : PacketExtended(EXTDATA_HouseCustom, 17, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketHouseBeginCustomise::PacketHouseBeginCustomise");

	writeInt32(house->GetUID());
	writeByte(0x04);
	writeInt16(0x0000);
	writeInt16(0xFFFF);
	writeInt16(0xFFFF);
	writeByte(0xFF);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x20.0x05 : PacketHouseEndCustomise	end house customisation (NORMAL)
 *
 *
 ***************************************************************************/
PacketHouseEndCustomise::PacketHouseEndCustomise(const CClient* target, const CItemMultiCustom* house) : PacketExtended(EXTDATA_HouseCustom, 17, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketHouseEndCustomise::PacketHouseEndCustomise");

	writeInt32(house->GetUID());
	writeByte(0x05);
	writeInt16(0x0000);
	writeInt16(0xFFFF);
	writeInt16(0xFFFF);
	writeByte(0xFF);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x22 : PacketCombatDamageOld		[old] sends notification of got damage (NORMAL)
 *
 *
 ***************************************************************************/
PacketCombatDamageOld::PacketCombatDamageOld(const CClient* target, DWORD damage, CGrayUID defender) : PacketExtended(EXTDATA_DamagePacketOld, 11, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketCombatDamageOld::PacketCombatDamageOld");

	writeByte(0x01);
	writeInt32(defender);
	writeByte((BYTE)(minimum(damage, 255)));

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x26 : PacketSpeedMode		set movement speed (HIGH)
 *
 *
 ***************************************************************************/
PacketSpeedMode::PacketSpeedMode(const CClient* target, BYTE mode) : PacketExtended(EXTDATA_SpeedMode, 6, g_Cfg.m_fUsePacketPriorities? PRI_HIGH : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketSpeedMode::PacketSpeedMode");

	writeByte(mode);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xC1: PacketMessageLocalised		show localised message to client (NORMAL)
 *
 *
 ***************************************************************************/
PacketMessageLocalised::PacketMessageLocalised(const CClient* target, int cliloc, const CObjBaseTemplate* source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font, TCHAR* args) : PacketSend(XCMD_SpeakLocalized, 50, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketMessageLocalised::PacketMessageLocalised");

	initLength();

	if (source == NULL)
		writeInt32(0xFFFFFFFF);
	else
		writeInt32(source->GetUID());

	if (source == NULL || source->IsChar() == false)
	{
		writeInt16(0xFFFF);
	}
	else
	{
		const CChar* sourceCharacter = dynamic_cast<const CChar*>(source);
		ASSERT(sourceCharacter);
		writeInt16(sourceCharacter->GetDispID());
	}

	writeByte(mode);
	writeInt16(hue);
	writeInt16(font);
	writeInt32(cliloc);

	if (source == NULL)
		writeStringFixedASCII("System", 30);
	else
		writeStringFixedASCII(source->GetName(), 30);

	writeStringUNICODE(args);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xC8: PacketVisualRange			set the visual range of the client (NORMAL)
 *
 *
 ***************************************************************************/
PacketVisualRange::PacketVisualRange(const CClient* target, BYTE range) : PacketSend(XCMD_ViewRange, 2, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketVisualRange::PacketVisualRange");

	writeByte(range);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xCC: PacketMessageLocalisedEx	show extended localised message to client (NORMAL)
 *
 *
 ***************************************************************************/
PacketMessageLocalisedEx::PacketMessageLocalisedEx(const CClient* target, int cliloc, const CObjBaseTemplate* source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font, AFFIX_TYPE affixType, TCHAR* affix, TCHAR* args) : PacketSend(XCMD_SpeakLocalizedEx, 52, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketMessageLocalisedEx::PacketMessageLocalisedEx");

	initLength();

	if (source == NULL)
		writeInt32(0xFFFFFFFF);
	else
		writeInt32(source->GetUID());

	if (source == NULL || source->IsChar() == false)
	{
		writeInt16(0xFFFF);
	}
	else
	{
		const CChar* sourceCharacter = dynamic_cast<const CChar*>(source);
		ASSERT(sourceCharacter);
		writeInt16(sourceCharacter->GetDispID());
	}

	writeByte(mode);
	writeInt16(hue);
	writeInt16(font);
	writeInt32(cliloc);
	writeByte(affixType);

	if (source == NULL)
		writeStringFixedASCII("System", 30);
	else
		writeStringFixedASCII(source->GetName(), 30);

	writeStringASCII(affix);
	writeStringUNICODE(args);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xD1 : PacketLogoutAck			accept logout char (LOW)
 *
 *
 ***************************************************************************/
PacketLogoutAck::PacketLogoutAck(const CClient* target) : PacketSend(XCMD_LogoutStatus, 2, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketLogoutAck::PacketLogoutAck");

	writeByte(1);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xD4 : PacketDisplayBookNew		display book  (LOW)
 *
 *
 ***************************************************************************/
PacketDisplayBookNew::PacketDisplayBookNew(const CClient* target, CItem* book) : PacketSend(XCMD_AOSBookPage, 17, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDisplayBookNew::PacketDisplayBookNew");

	bool isWritable = false;
	int pages = 0;
	CGString title;
	CGString author;

	if (book->IsBookSystem())
	{
		isWritable = false;

		CResourceLock s;
		if (g_Cfg.ResourceLock(s, book->m_itBook.m_ResID))
		{
			while (s.ReadKeyParse())
			{
				switch (FindTableSorted(s.GetKey(), CItemMessage::sm_szLoadKeys, COUNTOF(CItemMessage::sm_szLoadKeys )-1))
				{
					case CIC_AUTHOR:
						author = s.GetArgStr();
						break;
					case CIC_PAGES:
						pages = s.GetArgVal();
						break;
					case CIC_TITLE:
						title = s.GetArgStr();
						break;
				}
			}
		}

		// make sure book is named
		if (title.IsEmpty() == false)
			book->SetName((LPCTSTR)title);
	}
	else
	{
		// user written book
		const CItemMessage* message = dynamic_cast<const CItemMessage*>(book);
		if (message != NULL)
		{
			isWritable = message->IsBookWritable();
			pages = isWritable? MAX_BOOK_PAGES : message->GetPageCount();
			title = message->GetName();
			author = message->m_sAuthor.IsEmpty()? g_Cfg.GetDefaultMsg(DEFMSG_BOOK_AUTHOR_UNKNOWN) : (LPCTSTR)message->m_sAuthor;
		}
	}


	initLength();
	writeInt32(book->GetUID());
	writeBool(isWritable);
	writeBool(isWritable);
	writeInt16(pages);
	writeInt16(title.GetLength() + 1);
	writeStringFixedASCII(title.GetPtr(), title.GetLength() + 1);
	writeInt16(author.GetLength() + 1);
	writeStringFixedASCII(author.GetPtr(), author.GetLength() + 1);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xD6 : PacketPropertyList		property (tool tip) for objects (IDLE)
 *
 *
 ***************************************************************************/
PacketPropertyList::PacketPropertyList(const CObjBase* object, DWORD version, const CGObArray<CClientTooltip*>* data) : PacketSend(XCMD_AOSTooltip, 48, PRI_IDLE)
{
	ADDTOCALLSTACK("PacketPropertyList::PacketPropertyList");

	m_time = g_World.GetCurrentTime().GetTimeRaw();
	m_object = object->GetUID();
	m_version = version;
	m_entryCount = data->GetCount();

	initLength();
	writeInt16(1);
	writeInt32(object->GetUID());
	writeInt16(0);
	writeInt32(version);
	
	for (size_t x = 0; x < data->GetCount(); x++)
	{
		const CClientTooltip* tipEntry = data->GetAt(x);
		size_t tipLength = strlen(tipEntry->m_args);
		
		writeInt32(tipEntry->m_clilocid);
		writeInt16(tipLength * sizeof(WCHAR));
		writeStringFixedUNICODE(tipEntry->m_args, tipLength);
	}

	writeInt32(0);
}

PacketPropertyList::PacketPropertyList(const CClient* target, const PacketPropertyList* other) : PacketSend(other)
{
	ADDTOCALLSTACK("PacketPropertyList::PacketPropertyList2");

	m_time = g_World.GetCurrentTime().GetTimeRaw();
	m_object = other->getObject();
	m_version = other->getVersion();
	m_entryCount = other->getEntryCount();

	push(target, false);
}

bool PacketPropertyList::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketPropertyList::onSend");
#ifndef _MTNETWORK
	if (g_NetworkOut.isActive())
#else
	if (g_NetworkManager.isOutputThreaded())
#endif
		return true;

	const CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	const CObjBase* object = m_object.ObjFind();
	if (object == NULL || character->GetTopDistSight(object->GetTopLevelObj()) > UO_MAP_VIEW_SIZE)
		return false;

	if (hasExpired(TICK_PER_SEC * 30))
		return false;

	return true;
}

bool PacketPropertyList::hasExpired(int timeout) const
{
	ADDTOCALLSTACK("PacketPropertyList::hasExpired");
	return (m_time + timeout) < g_World.GetCurrentTime().GetTimeRaw();
}


/***************************************************************************
 *
 *
 *	Packet 0xD8 : PacketHouseDesign			house design (IDLE)
 *
 *
 ***************************************************************************/
PacketHouseDesign::PacketHouseDesign(const CItemMultiCustom* house, int revision) : PacketSend(XCMD_AOSCustomHouse, 64, g_Cfg.m_fUsePacketPriorities? PRI_IDLE : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketHouseDesign::PacketHouseDesign");

	m_house = house;

	initLength();

	writeByte(0x03);
	writeByte(0x00);
	writeInt32(house->GetUID());
	writeInt32(revision);
	writeInt16(0); // item count
	writeInt16(0); // data size
	writeByte(0); // plane count

	m_itemCount = 0;
	m_dataSize = 1;
	m_planeCount = 0;
	m_stairPlaneCount = 0;

	m_stairBuffer = new StairData[STAIRSPERBLOCK];
	memset(m_stairBuffer, 0, STAIRDATA_BUFFER);
	m_stairCount = 0;
}

PacketHouseDesign::PacketHouseDesign(const PacketHouseDesign* other) : PacketSend(other)
{
	ADDTOCALLSTACK("PacketHouseDesign::PacketHouseDesign(2)");

	m_house = other->m_house;
	m_itemCount = other->m_itemCount;
	m_dataSize = other->m_dataSize;
	m_planeCount = other->m_planeCount;
	m_stairPlaneCount = other->m_stairPlaneCount;

	m_stairBuffer = new StairData[STAIRSPERBLOCK];
	memcpy(m_stairBuffer, other->m_stairBuffer, STAIRDATA_BUFFER);
	m_stairCount = other->m_stairCount;
}

PacketHouseDesign::~PacketHouseDesign(void)
{
	if (m_stairBuffer != NULL)
	{
		delete[] m_stairBuffer;
		m_stairBuffer = NULL;
	}
}

bool PacketHouseDesign::writePlaneData(int plane, int itemCount, BYTE* data, int dataSize)
{
	ADDTOCALLSTACK("PacketHouseDesign::writePlaneData");

	// compress data
	z_uLong compressLength = z_compressBound(dataSize);
	BYTE* compressBuffer = new BYTE[compressLength];

	int error = z_compress2(compressBuffer, &compressLength, data, dataSize, Z_DEFAULT_COMPRESSION);
	if ( error != Z_OK )
	{
		// an error occured with this floor, but we should be able to continue to the next without problems
		delete[] compressBuffer;
		g_Log.EventError("Compress failed with error %d when generating house design for floor %d on building 0%lx.\n", error, plane, (DWORD)m_house->GetUID());
		return false;
	}
	else if ( compressLength <= 0 || compressLength >= PLANEDATA_BUFFER )
	{
		// too much data, but we should be able to continue to the next floor without problems
		delete[] compressBuffer;
		g_Log.EventWarn("Floor %d on building 0%lx too large with compressed length of %lu.\n", plane, (DWORD)m_house->GetUID(), compressLength);
		return false;
	}

	writeByte(plane | 0x20);
	writeByte(dataSize);
	writeByte((BYTE)compressLength);
	writeByte(((dataSize >> 4) & 0xF0) | ((compressLength >> 8) & 0x0F));
	writeData(compressBuffer, compressLength);
	delete[] compressBuffer;

	m_planeCount++;
	m_itemCount += itemCount;
	m_dataSize += (4 + compressLength);
	return true;
}

bool PacketHouseDesign::writeStairData(ITEMID_TYPE id, int x, int y, int z)
{
	ADDTOCALLSTACK("PacketHouseDesign::writeStairData");

	m_stairBuffer[m_stairCount].m_id = id;
	m_stairBuffer[m_stairCount].m_x = x;
	m_stairBuffer[m_stairCount].m_y = y;
	m_stairBuffer[m_stairCount].m_z = z;
	m_stairCount++;

	if (m_stairCount >= STAIRSPERBLOCK)
		flushStairData();

	return true;
}

void PacketHouseDesign::flushStairData(void)
{
	ADDTOCALLSTACK("PacketHouseDesign::flushStairData");

	if (m_stairCount <= 0)
		return;

	int stairCount = m_stairCount;
	int stairSize = stairCount * sizeof(StairData);

	m_stairCount = 0;

	// compress data
	z_uLong compressLength = z_compressBound(stairSize);
	BYTE* compressBuffer = new BYTE[compressLength];

	int error = z_compress2(compressBuffer, &compressLength, (BYTE*)m_stairBuffer, stairSize, Z_DEFAULT_COMPRESSION);
	if ( error != Z_OK )
	{
		// an error occured with this block, but we should be able to continue to the next without problems
		delete[] compressBuffer;
		g_Log.EventError("Compress failed with error %d when generating house design on building 0%lx.\n", error, (DWORD)m_house->GetUID());
		return;
	}
	else if (compressLength <= 0 || compressLength >= STAIRDATA_BUFFER)
	{
		// too much data, but we should be able to continue to the next block without problems
		delete[] compressBuffer;
		g_Log.EventWarn("Building 0%lx too large with compressed length of %lu.\n", (DWORD)m_house->GetUID(), compressLength);
		return;
	}

	writeByte(9 + m_stairPlaneCount);
	writeByte(stairSize);
	writeByte((BYTE)compressLength);
	writeByte(((stairSize >> 4) & 0xF0) | ((compressLength >> 8) & 0x0F));
	writeData(compressBuffer, compressLength);
	delete[] compressBuffer;

	m_stairPlaneCount++;
	m_itemCount += stairCount;
	m_dataSize += (4 + compressLength);
	return;
}

void PacketHouseDesign::finalise(void)
{
	ADDTOCALLSTACK("PacketHouseDesign::finalise");

	flushStairData();

	size_t endPosition(getPosition());

	seek(13);
	writeInt16(m_itemCount);
	writeInt16(m_dataSize);
	writeByte(m_planeCount + m_stairPlaneCount);

	seek(endPosition);
}


/***************************************************************************
 *
 *
 *	Packet 0xDC : PacketPropertyListVersion		property (tool tip) version (LOW)
 *
 *
 ***************************************************************************/
PacketPropertyListVersion::PacketPropertyListVersion(const CClient* target, const CObjBase* object, DWORD version) : PacketSend(XCMD_AOSTooltipInfo, 9, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPropertyListVersion::PacketPropertyListVersion");

	m_object = object->GetUID();

	writeInt32(object->GetUID());
	writeInt32(version);

	if (target != NULL)
		push(target, false);
}

bool PacketPropertyListVersion::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketPropertyList::onSend");
#ifndef _MTNETWORK
	if (g_NetworkOut.isActive())
#else
	if (g_NetworkManager.isOutputThreaded())
#endif
		return true;

	const CChar* character = client->GetChar();
	if (character == NULL)
		return false;

	const CObjBase* object = m_object.ObjFind();
	if (object == NULL || character->GetTopDistSight(object->GetTopLevelObj()) > UO_MAP_VIEW_SIZE)
		return false;

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xDF : PacketBuff				add/remove buff icon (LOW)
 *
 *
 ***************************************************************************/
PacketBuff::PacketBuff(const CClient* target, const WORD iconId, const DWORD clilocOne, const DWORD clilocTwo, const short time, LPCTSTR* args, size_t argCount) : PacketSend(XCMD_BuffPacket, 72, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketBuff::PacketBuff");

	const CChar* character = target->GetChar();
	ASSERT(character != NULL);

	initLength();

	writeInt32(character->GetUID());
	writeInt16(iconId);
	writeInt16(0x01); // show (count?)

	writeInt32(0);
	writeInt16(iconId);
	writeInt16(0x01); // show
	writeInt32(0);
	writeInt16(time);
	writeInt16(0);
	writeByte(0);
	writeInt32(clilocOne);
	writeInt32(clilocTwo);
	writeInt32(0);
	writeInt16(0x01);
	writeInt16(0);

	writeInt16(9);

	for (size_t i = 0; i < argCount; i++)
	{
		if (i > 0)
			writeCharUNICODE('\t');

		writeStringUNICODE(args[i], false);
	}

	writeCharUNICODE('\t');
	writeCharUNICODE('\0');

	push(target);
}

PacketBuff::PacketBuff(const CClient* target, const WORD iconId) : PacketSend(XCMD_BuffPacket, 15, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketBuff::PacketBuff(2)");

	const CChar* character = target->GetChar();
	ASSERT(character != NULL);

	initLength();

	writeInt32(character->GetUID());
	writeInt16(iconId);
	writeInt16(0x00); // hide

	writeInt32(0);

	push(target);
}

/***************************************************************************
 *
 *
 *	Packet 0xE3 : PacketKREncryption		Sends encryption data to KR client
 *
 *
 ***************************************************************************/
PacketKREncryption::PacketKREncryption(const CClient* target) : PacketSend(XCMD_EncryptionReq, 77, g_Cfg.m_fUsePacketPriorities? PRI_HIGH : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketKREncryption::PacketKREncryption");

	static BYTE pDataKR_E3[76] = {
						0x00, 0x4d,
						0x00, 0x00, 0x00, 0x03, 0x02, 0x00, 0x03,
						0x00, 0x00, 0x00, 0x13, 0x02, 0x11, 0x00, 0x00, 0x2f, 0xe3, 0x81, 0x93, 0xcb, 0xaf, 0x98, 0xdd, 0x83, 0x13, 0xd2, 0x9e, 0xea, 0xe4, 0x13,
						0x00, 0x00, 0x00, 0x10, 0x00, 0x13, 0xb7, 0x00, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
						0x00, 0x00, 0x00, 0x20,
						0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	writeData(pDataKR_E3, sizeof(pDataKR_E3));
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xEA : PacketToggleHotbar		toggle kr hotbar (NORMAL)
 *
 *
 ***************************************************************************/
PacketToggleHotbar::PacketToggleHotbar(const CClient* target, bool enable) : PacketSend(XCMD_ToggleHotbar, 3, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketToggleHotbar::PacketToggleHotbar");

	writeInt16(enable? 0x01 : 0x00);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xF3 : PacketItemWorldNew		sends item on ground (NORMAL)
 *
 *
 ***************************************************************************/
PacketItemWorldNew::PacketItemWorldNew(const CClient* target, CItem *item) : PacketItemWorld(XCMD_PutNew, 26, item->GetUID())
{
	ADDTOCALLSTACK("PacketItemWorldNew::PacketItemWorldNew");

	DataSource source = TileData;
	DWORD uid = item->GetUID();
	long amount = item->GetAmount();
	ITEMID_TYPE id = item->GetDispID();
	CPointMap p = item->GetTopPoint();
	BYTE dir = DIR_N;
	HUE_TYPE hue = item->GetHue();
	BYTE flags = 0;
	BYTE light = 0;

	adjustItemData(target, item, id, hue, amount, p, dir, flags, light);

	if (id >= ITEMID_MULTI)
	{
		id = (ITEMID_TYPE)(id - ITEMID_MULTI);
		source = Multi;
	}
	else if (target->GetNetState()->isClientLessVersion(MINCLIVER_HIGHSEAS) && id >= ITEMID_MULTI_SA)
	{
		id = ITEMID_WorldGem;
	}

	writeInt16(1);
	writeByte(source);// 0=tiledata,1=character,2=multi
	writeInt32(uid);
	writeInt16(id);
	writeByte(dir);
	writeInt16((WORD)amount);
	writeInt16((WORD)amount);
	writeInt16(p.m_x);
	writeInt16(p.m_y);
	writeByte(p.m_z);
	writeByte(light);
	writeInt16(hue);
	writeByte(flags);

	if (target->GetNetState()->isClientVersion(MINCLIVER_HIGHSEAS))
		writeInt16(0);
	
	trim();
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xF5 : PacketDisplayMapNew		display map (LOW)
 *
 *
 ***************************************************************************/
PacketDisplayMapNew::PacketDisplayMapNew(const CClient* target, const CItemMap* map, const CRectMap& rect) : PacketSend(XCMD_MapDisplayNew, 21, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDisplayMapNew::PacketDisplayMapNew");

	const CItemBase* itemDef = map->Item_GetDef();
	ASSERT(itemDef != NULL);

	WORD width = itemDef->m_ttMap.m_iGumpWidth > 0 ? itemDef->m_ttMap.m_iGumpWidth : CItemMap::DEFAULT_SIZE;
	WORD height = itemDef->m_ttMap.m_iGumpHeight > 0 ? itemDef->m_ttMap.m_iGumpHeight : CItemMap::DEFAULT_SIZE;

	writeInt32(map->GetUID());
	writeInt16(GUMP_MAP_2_NORTH);
	writeInt16(rect.m_left);
	writeInt16(rect.m_top);
	writeInt16(rect.m_right);
	writeInt16(rect.m_bottom);
	writeInt16(width);
	writeInt16(height);
	writeInt16(rect.m_map);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xF6 : PacketMoveShip			move ship (NORMAL)
 *
 *
 ***************************************************************************/
PacketMoveShip::PacketMoveShip(const CItemShip* ship, CObjBase** objects, size_t objectCount, DIR_TYPE direction, BYTE speed) : PacketSend(XCMD_MoveShip, 18, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketMoveShip::PacketMoveShip");
	ASSERT(objectCount > 0);
	const CPointMap& shipLocation = ship->GetTopPoint();

	initLength();
	writeInt32(ship->GetUID());
	writeByte(speed);
	writeByte(direction);
	writeByte(direction); // unknown
	writeInt16(shipLocation.m_x);
	writeInt16(shipLocation.m_y);
	writeInt16(shipLocation.m_z);

	// assume that first object is the ship itself
	writeInt16(objectCount - 1);

	for (size_t i = 1; i < objectCount; i++)
	{
		const CObjBase* object = objects[i];
		const CPointMap& objectLocation = object->GetTopPoint();
		
		writeInt32(object->GetUID());
		writeInt16(objectLocation.m_x);
		writeInt16(objectLocation.m_y);
		writeInt16(objectLocation.m_z);
	}
}

