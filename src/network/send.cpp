#include "send.h"
#include "network.h"
#include "../common/zlib/zlib.h"

#ifndef _WIN32
#include <sys/time.h>
#endif

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
 *	Packet **** : PacketTelnet				send message to telnet client (HIGHEST)
 *
 *
 ***************************************************************************/
PacketTelnet::PacketTelnet(const CClient* target, LPCTSTR message, bool bNullTerminated) : PacketSend(0, 0, PRI_HIGHEST)
{
	ADDTOCALLSTACK("PacketTelnet::PacketTelnet");

	seek();

	for (size_t i = 0; message[i] != '\0'; i++)
	{
		if (message[i] == '\n')
			writeCharASCII('\r');

		writeCharASCII(message[i]);
	}
	if (bNullTerminated)
		writeCharASCII('\0');

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
PacketWeb::PacketWeb(const CClient * target, const BYTE * data, size_t length) : PacketSend(0, 0, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketWeb::PacketWeb");

	if (data && (length > 0))
		setData(data, length);

	if (target)
		push(target);
}

void PacketWeb::setData(const BYTE * data, size_t length)
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
PacketCombatDamage::PacketCombatDamage(const CClient* target, WORD damage, CGrayUID uid) : PacketSend(PACKET_DamagePacket, 7, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketCombatDamage::PacketCombatDamage");

	writeInt32(static_cast<DWORD>(uid));
	writeInt16(damage);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x11 : PacketHealthBarInfo		sends health bar info (LOW)
 *
 *
 ***************************************************************************/
PacketHealthBarInfo::PacketHealthBarInfo(const CClient *target, CObjBase *object) : PacketSend(PACKET_Status, 7, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketHealthBarInfo::PacketHealthBarInfo");
	if ( !object )
		return;

	const CChar *character = target->GetChar();
	const CChar *objectChar = dynamic_cast<const CChar *>(object);
	bool canRename = false;
	BYTE version = 0;

	initLength();

	writeInt32(object->GetUID());
	writeStringFixedASCII(object->GetName(), MAX_NAME_SIZE);

	if (objectChar && (objectChar == character))
	{
		writeInt16(static_cast<WORD>(objectChar->Stat_GetVal(STAT_STR)));
		writeInt16(static_cast<WORD>(objectChar->Stat_GetMax(STAT_STR)));
		writeBool(canRename);

		const NetState *state = target->m_NetState;
		if (state->isClientVersion(MINCLIVER_STATUS_V6))
			version = 6;
		else if (state->isClientVersion(MINCLIVER_STATUS_V5))
			version = 5;
		else if (state->isClientVersion(MINCLIVER_STATUS_V4))
			version = 4;
		else if (state->isClientVersion(MINCLIVER_STATUS_V3))
			version = 3;
		else if (state->isClientVersion(MINCLIVER_STATUS_V2))
			version = 2;
		else
			version = 1;
		writeByte(version);

		int strength = objectChar->Stat_GetAdjusted(STAT_STR);
		if (strength < 0)
			strength = 0;

		int dexterity = objectChar->Stat_GetAdjusted(STAT_DEX);
		if (dexterity < 0)
			dexterity = 0;

		int intelligence = objectChar->Stat_GetAdjusted(STAT_INT);
		if (intelligence < 0)
			intelligence = 0;

		const CCharBase *objectCharDef = objectChar->Char_GetDef();
		writeBool(objectCharDef->IsFemale());
		writeInt16(static_cast<WORD>(strength));
		writeInt16(static_cast<WORD>(dexterity));
		writeInt16(static_cast<WORD>(intelligence));
		writeInt16(static_cast<WORD>(objectChar->Stat_GetVal(STAT_DEX)));
		writeInt16(static_cast<WORD>(objectChar->Stat_GetMax(STAT_DEX)));
		writeInt16(static_cast<WORD>(objectChar->Stat_GetVal(STAT_INT)));
		writeInt16(static_cast<WORD>(objectChar->Stat_GetMax(STAT_INT)));

		if ( g_Cfg.m_fPayFromPackOnly )
			writeInt32(const_cast<CChar *>(objectChar)->GetContainerCreate(LAYER_PACK)->ContentCount(RESOURCE_ID(RES_TYPEDEF,IT_GOLD)));
		else
			writeInt32(const_cast<CChar *>(objectChar)->ContentCount(RESOURCE_ID(RES_TYPEDEF,IT_GOLD)));
		
		if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
			writeInt16(static_cast<WORD>(object->m_ResPhysicalMax ? minimum(object->m_ResPhysical, object->m_ResPhysicalMax) : object->m_ResPhysical));
		else
			writeInt16(objectChar->m_defense + objectCharDef->m_defense);

		writeInt16(static_cast<WORD>(objectChar->GetTotalWeight() / WEIGHT_UNITS));

		if (version >= 5) // ML attributes
		{
			writeInt16(static_cast<WORD>(g_Cfg.Calc_MaxCarryWeight(objectChar) / WEIGHT_UNITS));

			switch (objectChar->GetDispID())
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
			writeInt16(static_cast<WORD>(objectChar->Stat_GetLimit(STAT_QTY)));

		if (version >= 3) // Renaissance attributes
		{
			writeByte(static_cast<BYTE>(objectChar->m_FollowerCur));
			writeByte(static_cast<BYTE>(objectChar->m_FollowerMax));
		}

		if (version >= 4) // AOS attributes
		{
			if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
			{
				writeInt16(static_cast<WORD>(object->m_ResFireMax ? minimum(object->m_ResFire, object->m_ResFireMax) : object->m_ResFire));
				writeInt16(static_cast<WORD>(object->m_ResColdMax ? minimum(object->m_ResCold, object->m_ResColdMax) : object->m_ResCold));
				writeInt16(static_cast<WORD>(object->m_ResPoisonMax ? minimum(object->m_ResPoison, object->m_ResPoisonMax) : object->m_ResPoison));
				writeInt16(static_cast<WORD>(object->m_ResEnergyMax ? minimum(object->m_ResEnergy, object->m_ResEnergyMax) : object->m_ResEnergy));
			}
			else
			{
				writeInt16(0);
				writeInt16(0);
				writeInt16(0);
				writeInt16(0);
			}

			writeInt16(static_cast<WORD>(object->m_Luck));

			const CItem *weapon = objectChar->m_uidWeapon.ItemFind();
			writeInt16(static_cast<WORD>(objectChar->Fight_CalcDamage(weapon, true, false)));
			writeInt16(static_cast<WORD>(objectChar->Fight_CalcDamage(weapon, true, true)));

			writeInt32(static_cast<DWORD>(objectChar->m_Tithing));
		}

		if (version >= 6)	// SA attributes
		{
			writeInt16(static_cast<WORD>(object->m_ResPhysicalMax));
			writeInt16(static_cast<WORD>(object->m_ResFireMax));
			writeInt16(static_cast<WORD>(object->m_ResColdMax));
			writeInt16(static_cast<WORD>(object->m_ResPoisonMax));
			writeInt16(static_cast<WORD>(object->m_ResEnergyMax));
			writeInt16(static_cast<WORD>(object->m_DefChanceIncrease));
			writeInt16(static_cast<WORD>(object->m_DefChanceIncreaseMax));
			writeInt16(static_cast<WORD>(object->m_HitChanceIncrease));
			writeInt16(static_cast<WORD>(object->m_SwingSpeedIncrease));
			writeInt16(static_cast<WORD>(object->m_DamIncrease));
			writeInt16(static_cast<WORD>(object->m_LowerReagentCost));
			writeInt16(static_cast<WORD>(object->m_SpellDamIncrease));
			writeInt16(static_cast<WORD>(object->m_FasterCastRecovery));
			writeInt16(static_cast<WORD>(object->m_FasterCasting));
			writeInt16(static_cast<WORD>(object->m_LowerManaCost));
		}

		// We really don't know what is going on here. RUOSI Packet Guide was way off... -Khaos
		// Possible KR client status info... -Ben
		if (target->m_NetState->isClientKR())
		{
			writeInt16(static_cast<WORD>(object->m_HitChanceIncrease));
			writeInt16(static_cast<WORD>(object->m_SwingSpeedIncrease));
			writeInt16(static_cast<WORD>(object->m_DamIncrease));
			writeInt16(static_cast<WORD>(object->m_LowerReagentCost));
			writeInt16(static_cast<WORD>(object->GetDefNum("REGENHITS", true)));
			writeInt16(static_cast<WORD>(object->GetDefNum("REGENSTAM", true)));
			writeInt16(static_cast<WORD>(object->GetDefNum("REGENMANA", true)));
			writeInt16(static_cast<WORD>(object->GetDefNum("REFLECTPHYSICALDAM", true)));
			writeInt16(static_cast<WORD>(object->m_EnhancePotions));
			writeInt16(static_cast<WORD>(object->m_DefChanceIncrease));
			writeInt16(static_cast<WORD>(object->m_SpellDamIncrease));
			writeInt16(static_cast<WORD>(object->m_FasterCastRecovery));
			writeInt16(static_cast<WORD>(object->m_FasterCasting));
			writeInt16(static_cast<WORD>(object->m_LowerManaCost));
			writeInt16(static_cast<WORD>(objectChar->Stat_GetMod(STAT_STR)));
			writeInt16(static_cast<WORD>(objectChar->Stat_GetMod(STAT_DEX)));
			writeInt16(static_cast<WORD>(objectChar->Stat_GetMod(STAT_INT)));
			writeInt16(static_cast<WORD>(objectChar->Stat_GetMax(STAT_STR)));
			writeInt16(static_cast<WORD>(objectChar->Stat_GetMax(STAT_DEX)));
			writeInt16(static_cast<WORD>(objectChar->Stat_GetMax(STAT_INT)));
			writeInt16(static_cast<WORD>(object->GetDefNum("BONUSHITSMAX", true)));
			writeInt16(static_cast<WORD>(object->GetDefNum("BONUSSTAMMAX", true)));
			writeInt16(static_cast<WORD>(object->GetDefNum("BONUSMANAMAX", true)));
		}
	}
	else
	{
		if ( objectChar )
		{
			canRename = objectChar->NPC_IsOwnedBy(character);
			writeInt16(static_cast<WORD>((objectChar->Stat_GetVal(STAT_STR) * 100) / maximum(objectChar->Stat_GetMax(STAT_STR), 1)));
		}
		else
		{
			const CItem *objectItem = static_cast<const CItem *>(object);
			writeInt16(static_cast<WORD>((objectItem->m_itArmor.m_Hits_Cur * 100) / maximum(objectItem->m_itArmor.m_Hits_Max, 1)));
		}

		writeInt16(100);
		writeBool(canRename);
		writeByte(version);
	}

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x16 : PacketHealthBarUpdateNew	update health bar color (LOW)
 *	Packet 0x17 : PacketHealthBarUpdate		update health bar color (LOW)
 *
 *
 ***************************************************************************/
PacketHealthBarUpdateNew::PacketHealthBarUpdateNew(const CClient* target, const CChar* character) : PacketSend(PACKET_HealthBarColorNew, 12, g_Cfg.m_fUsePacketPriorities ? PRI_LOW : PRI_NORMAL), m_character(character->GetUID())
{
	ADDTOCALLSTACK("PacketHealthBarUpdateNew::PacketHealthBarUpdateNew");

	WORD wColor = 0;
	if ( character->IsStatFlag(STATF_Poisoned) )
		wColor = GreenBar;
	else if ( character->IsStatFlag(STATF_Freeze|STATF_Stone) )
		wColor = YellowBar;

	initLength();

	writeInt32(character->GetUID());
	writeInt16(1);
	writeInt16(wColor);
	writeBool(wColor != 0);

	push(target);
}

bool PacketHealthBarUpdateNew::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketHealthBarUpdateNew::onSend");
	if (g_NetworkManager.isOutputThreaded())
		return true;

	return client->CanSee(m_character.CharFind());
}


PacketHealthBarUpdate::PacketHealthBarUpdate(const CClient* target, const CChar* character) : PacketSend(PACKET_HealthBarColor, 15, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL), m_character(character->GetUID())
{
	ADDTOCALLSTACK("PacketHealthBarUpdate::PacketHealthBarUpdate");

	initLength();

	writeInt32(character->GetUID());
	writeInt16(2);
	writeInt16(GreenBar);
	writeByte(character->IsStatFlag(STATF_Poisoned));
	writeInt16(YellowBar);
	writeByte(character->IsStatFlag(STATF_Freeze|STATF_Stone));

	push(target);
}

bool PacketHealthBarUpdate::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketHealthBarUpdate::onSend");
	if (g_NetworkManager.isOutputThreaded())
		return true;

	return client->CanSee(m_character.CharFind());
}


/***************************************************************************
 *
 *
 *	Packet 0x1A : PacketWorldItem			sends item on ground (NORMAL)
 *
 *
 ***************************************************************************/
PacketWorldItem::PacketWorldItem(BYTE id, size_t size, CGrayUID uid) : PacketSend(id, size, PRI_NORMAL), m_item(uid)
{
}

PacketWorldItem::PacketWorldItem(const CClient* target, CItem *item) : PacketSend(PACKET_WorldItem, 20, PRI_NORMAL), m_item(item->GetUID())
{
	ADDTOCALLSTACK("PacketWorldItem::PacketWorldItem");

	DWORD uid = static_cast<DWORD>(item->GetUID());
	WORD amount = item->GetAmount();
	ITEMID_TYPE id = item->GetDispID();
	CPointMap pt = item->GetTopPoint();
	HUE_TYPE hue = item->GetHue();
	BYTE layer = 0;
	BYTE flags = 0;

	adjustItemData(target, item, id, hue, amount, layer, flags);

	// This packet only supports item IDs up to 0x3FFF, and multis start from 0x4000 (ITEMID_MULTI_LEGACY) multis need
	// to be adjusted to the lower range, and items between 03FFF and 08000 need to be adjusted to something safer
	if (id >= ITEMID_MULTI)
		id = static_cast<ITEMID_TYPE>(id - (ITEMID_MULTI - ITEMID_MULTI_LEGACY));
	else if (id >= ITEMID_MULTI_LEGACY)
		id = ITEMID_WorldGem;

	if (amount > 0)
		uid |= 0x80000000;
	else
		uid &= 0x7FFFFFFF;

	pt.m_x &= 0x7FFF;
	if (layer > 0)
		pt.m_x |= 0x8000;

	pt.m_y &= 0x3FFF;
	if (hue > 0)
		pt.m_y |= 0x8000;
	if (flags > 0)
		pt.m_y |= 0x4000;

	initLength();
	writeInt32(uid);
	writeInt16(static_cast<WORD>(id));
	if (amount > 0)
		writeInt16(amount);
	writeInt16(static_cast<WORD>(pt.m_x));
	writeInt16(static_cast<WORD>(pt.m_y));
	if (layer > 0)
		writeByte(layer);
	writeByte(static_cast<BYTE>(pt.m_z));
	if (hue > 0)
		writeInt16(static_cast<WORD>(hue));
	if (flags > 0)
		writeByte(flags);

	push(target);
}

void PacketWorldItem::adjustItemData(const CClient* target, CItem* item, ITEMID_TYPE &id, HUE_TYPE &hue, WORD &amount, BYTE &layer, BYTE &flags)
{
	ADDTOCALLSTACK("PacketWorldItem::adjustItemData");
	// Modify item values for compatibility with this specific client
	// NOTE: layer value can return both 'light' (on light sources) or 'direction' (on corpses)
	const CItemBase *itemDefintion = item->Item_GetDef();
	const CChar *character = target->GetChar();
	if (!itemDefintion || !character)
		return;

	if (id != ITEMID_CORPSE)
	{
		if (itemDefintion && (target->GetResDisp() < itemDefintion->GetResLevel()))
		{
			id = static_cast<ITEMID_TYPE>(itemDefintion->GetResDispDnId());
			if (itemDefintion->GetResDispDnHue() != HUE_DEFAULT)
				hue = itemDefintion->GetResDispDnHue();
		}

		// on monster this just colors the underwear. thats it.
		if (hue & HUE_MASK_UNDERWEAR)
			hue = 0;
		else if ((hue & HUE_MASK_HI) > HUE_QTY)
			hue &= HUE_MASK_LO | HUE_MASK_TRANSLUCENT;
		else
			hue &= HUE_MASK_HI | HUE_MASK_TRANSLUCENT;
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
		
		// allow HUE_MASK_UNDERWEAR colours only on corpses
		if ((hue & HUE_MASK_HI) > HUE_QTY)
			hue &= HUE_MASK_LO | HUE_MASK_UNDERWEAR | HUE_MASK_TRANSLUCENT;
		else
			hue &= HUE_MASK_HI | HUE_MASK_UNDERWEAR | HUE_MASK_TRANSLUCENT;

		layer = static_cast<BYTE>(item->m_itCorpse.m_facing_dir);
	}

	if (character->CanMove(item, false))
		flags |= ITEMF_MOVABLE;

	if (target->IsPriv(PRIV_DEBUG))
	{
		id = ITEMID_WorldGem;
		amount = 0;
		flags |= ITEMF_MOVABLE;
	}
	else
	{
		if (!character->CanSee(item))
			flags |= ITEMF_INVIS;

		if (itemDefintion->Can(CAN_I_LIGHT))
		{
			if (item->IsTypeLit())
				layer = item->m_itLight.m_pattern;
			else
				layer = static_cast<BYTE>(LIGHT_LARGE);
		}
	}
}

bool PacketWorldItem::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketWorldItem::onSend");
	if (g_NetworkManager.isOutputThreaded())
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
PacketPlayerStart::PacketPlayerStart(const CClient* target) : PacketSend(PACKET_Start, 37, g_Cfg.m_fUsePacketPriorities? PRI_HIGH : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPlayerStart::PacketPlayerStart");

	const CChar* character = target->GetChar();
	const CPointMap& pt = character->GetTopPoint();
	
	writeInt32(character->GetUID());
	writeInt32(0);
	writeInt16(static_cast<WORD>(character->GetDispID()));
	writeInt16(pt.m_x);
	writeInt16(pt.m_y);
	writeInt16(pt.m_z);
	writeByte(character->GetDirFlag());
	writeByte(0);
	writeInt32(0xFFFFFFFF);
	writeInt16(0);
	writeInt16(0);
	writeInt16(pt.m_map > 0 ? static_cast<WORD>(g_MapList.GetX(pt.m_map)) : 0x1800);
	writeInt16(pt.m_map > 0 ? static_cast<WORD>(g_MapList.GetY(pt.m_map)) : 0x1000);
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
PacketMessageASCII::PacketMessageASCII(const CClient* target, LPCTSTR pszText, const CObjBaseTemplate * source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font) : PacketSend(PACKET_Speak, 42, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketMessageASCII::PacketMessageASCII");

	initLength();

	if (!source)
		writeInt32(0xFFFFFFFF);
	else
		writeInt32(source->GetUID());
	
	if (!source || !source->IsChar())
		writeInt16(0xFFFF);
	else
	{
		const CChar* sourceCharacter = dynamic_cast<const CChar *>(source);
		if ( !sourceCharacter )
			return;
		writeInt16(static_cast<WORD>(sourceCharacter->GetDispID()));
	}
	
	writeByte(static_cast<BYTE>(mode));
	writeInt16(static_cast<WORD>(hue));
	writeInt16(static_cast<WORD>(font));

	// we need to ensure that the name is null terminated here when using TALKMODE_ITEM, otherwise
	// the journal can freeze and crash older client versions
	if (source)
		writeStringFixedASCII(source->GetName(), MAX_NAME_SIZE, true);
	else
		writeStringFixedASCII("System", MAX_NAME_SIZE);

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
PacketRemoveObject::PacketRemoveObject(const CClient* target, CGrayUID uid) : PacketSend(PACKET_Remove, 5, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketRemoveObject::PacketRemoveObject");

	writeInt32(uid);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x20 : PacketPlayerUpdate		update player character on screen (NORMAL)
 *
 *
 ***************************************************************************/
PacketPlayerUpdate::PacketPlayerUpdate(const CClient* target) : PacketSend(PACKET_PlayerUpdate, 19, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPlayerUpdate::PacketPlayerUpdate");
	// NOTE: This packet remove weather effects on client screen.

	const CChar* character = target->GetChar();
	ASSERT(character);

	CREID_TYPE id;
	HUE_TYPE hue;
	target->GetAdjustedCharID(character, id, hue);

	const CPointMap& pt = character->GetTopPoint();

	writeInt32(character->GetUID());
	writeInt16(static_cast<WORD>(id));
	writeByte(0);
	writeInt16(hue);
	writeByte(character->GetModeFlag(target));
	writeInt16(pt.m_x);
	writeInt16(pt.m_y);
	writeInt16(0);
	writeByte(character->GetDirFlag());
	writeByte(pt.m_z);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x21 : PacketMovementRej			rejects movement (HIGHEST)
 *
 *
 ***************************************************************************/
PacketMovementRej::PacketMovementRej(const CClient* target, BYTE sequence) : PacketSend(PACKET_WalkReject, 8, g_Cfg.m_fUsePacketPriorities? PRI_HIGHEST : PRI_NORMAL)
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
}


/***************************************************************************
 *
 *
 *	Packet 0x22 : PacketMovementAck			accepts movement (HIGHEST)
 *
 *
 ***************************************************************************/
PacketMovementAck::PacketMovementAck(const CClient* target, BYTE sequence) : PacketSend(PACKET_WalkAck, 3, g_Cfg.m_fUsePacketPriorities? PRI_HIGHEST : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketMovementAck::PacketMovementAck");

	writeByte(sequence);
	writeByte(static_cast<BYTE>(target->GetChar()->Noto_GetFlag(target->GetChar(), target->m_NetState->isClientVersion(MINCLIVER_AOS), true)));
	push(target);
}

/***************************************************************************
 *
 *
 *	Packet 0x23 : PacketDragAnimation		drag animation (LOW)
 *
 *
 ***************************************************************************/
PacketDragAnimation::PacketDragAnimation(const CChar* source, const CItem* item, const CObjBase* container, const CPointMap* pt) : PacketSend(PACKET_DragAnim, 26, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDragAnimation::PacketDragAnimation");

	writeInt16(static_cast<WORD>(item->GetDispID()));
	writeByte(0);
	writeInt16(static_cast<WORD>(item->GetHue()));
	writeInt16(item->GetAmount());

	const CPointMap& sourcepos = source->GetTopPoint();

	if (container)
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

		writeInt32((target == item)? 0 : static_cast<DWORD>(target->GetUID()));
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
	if (state->isClientEnhanced() || state->isClientVersion(MINCLIVER_SA))
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
PacketContainerOpen::PacketContainerOpen(const CClient* target, const CObjBase* container, GUMP_TYPE gump) : PacketSend(PACKET_ContOpen, 9, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL), m_container(container->GetUID())
{
	ADDTOCALLSTACK("PacketContainerOpen::PacketContainerOpen");

	writeInt32(container->GetUID());
	writeInt16(static_cast<WORD>(gump));

	// HS clients needs an extra 'container type' byte (0x00 for vendors, 0x7D for spellbooks/containers)
	if (target->m_NetState->isClientVersion(MINCLIVER_HS) || target->m_NetState->isClientKR() || target->m_NetState->isClientEnhanced())
	{
		WORD ContType = (gump == GUMP_VENDOR_RECT) ? 0x00 : 0x7D;
		writeInt16(ContType);
	}

	trim();
	push(target);
}

bool PacketContainerOpen::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketContainerOpen::onSend");
	if (g_NetworkManager.isOutputThreaded())
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
PacketItemContainer::PacketItemContainer(const CClient* target, const CItem* item) : PacketSend(PACKET_ContAdd, 21, PRI_NORMAL), m_item(item->GetUID())
{
	ADDTOCALLSTACK("PacketItemContainer::PacketItemContainer");

	const CItemContainer* container = dynamic_cast<CItemContainer *>(item->GetParent());
	if ( !container )
		return;

	const CPointBase &pt = item->GetContainedPoint();
	const CItemBase* itemDefinition = item->Item_GetDef();
	ITEMID_TYPE id = item->GetDispID();
	HUE_TYPE hue = item->GetHue() & HUE_MASK_HI;

	if (itemDefinition && (target->GetResDisp() < itemDefinition->GetResLevel()))
	{
		id = static_cast<ITEMID_TYPE>(itemDefinition->GetResDispDnId());
		if (itemDefinition->GetResDispDnHue() != HUE_DEFAULT)
			hue = itemDefinition->GetResDispDnHue() & HUE_MASK_HI;
	}

	if (hue > HUE_QTY)
		hue &= HUE_MASK_LO;

	writeInt32(item->GetUID());
	writeInt16(static_cast<WORD>(id));
	writeByte(0);
	writeInt16(item->GetAmount());
	writeInt16(pt.m_x);
	writeInt16(pt.m_y);

	if (target->m_ContainerGridEnabled)
		writeByte(item->GetContainedGridIndex());

	writeInt32(container->GetUID());
	writeInt16(hue);

	trim();
	push(target);
}

PacketItemContainer::PacketItemContainer(const CItem* spellbook, const CSpellDef* spell) : PacketSend(PACKET_ContAdd, 21, PRI_NORMAL), m_item(spellbook->GetUID())
{
	ADDTOCALLSTACK("PacketItemContainer::PacketItemContainer(2)");

	writeInt32(UID_F_ITEM|UID_O_INDEX_FREE|spell->m_idSpell);
	writeInt16(static_cast<WORD>(spell->m_idScroll));
	writeByte(0);
	writeInt16(static_cast<WORD>(spell->m_idSpell));
	writeInt16(0x48);
	writeInt16(0x7D);
}

void PacketItemContainer::completeForTarget(const CClient* target, const CItem* spellbook)
{
	ADDTOCALLSTACK("PacketItemContainer::completeForTarget");
	
	if (getLength() >= 20)
	{
		// only append the additional information if it needs to be changed
		bool containsGrid = (getLength() == 21) ? true : false;
		if (target->m_ContainerGridEnabled == containsGrid)
			return;
	}

	seek(14);

	if (target->m_ContainerGridEnabled)
		writeByte(0);

	writeInt32(spellbook->GetUID());
	writeInt16(HUE_DEFAULT);

	trim();
}

bool PacketItemContainer::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketItemContainer::onSend");
	if (g_NetworkManager.isOutputThreaded())
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
PacketKick::PacketKick(const CClient* target) : PacketSend(PACKET_Kick, 5, PRI_HIGHEST)
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
PacketDragCancel::PacketDragCancel(const CClient* target, Reason code) : PacketSend(PACKET_DragCancel, 2, g_Cfg.m_fUsePacketPriorities? PRI_HIGH : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDragCancel::PacketDragCancel");

	writeByte(static_cast<BYTE>(code));
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x29 : PacketDropAccepted		notify drop accepted (NORMAL) (KR)
 *
 *
 ***************************************************************************/
PacketDropAccepted::PacketDropAccepted(const CClient* target) : PacketSend(PACKET_DropAccepted, 1, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDropAccepted::PacketDropAccepted");

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x2C : PacketDeathMenu			display death menu/effect (NORMAL)
 *
 *
 ***************************************************************************/
PacketDeathMenu::PacketDeathMenu(const CClient *target, BYTE mode) : PacketSend(PACKET_DeathMenu, 2, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDeathMenu::PacketDeathMenu");

	writeByte(mode);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x2E : PacketItemEquipped		sends equipped item (NORMAL)
 *
 *
 ***************************************************************************/
PacketItemEquipped::PacketItemEquipped(const CClient* target, const CItem* item) : PacketSend(PACKET_ItemEquip, 15, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketItemEquipped::PacketItemEquipped");

	const CChar* parent = dynamic_cast<CChar *>(item->GetParent());
	if ( !parent )
		return;

	LAYER_TYPE layer = item->GetEquipLayer();
	ITEMID_TYPE id;
	HUE_TYPE hue;
	target->GetAdjustedItemID(parent, item, id, hue);

	writeInt32(item->GetUID());
	writeInt16(static_cast<WORD>(layer == LAYER_BANKBOX ? ITEMID_CHEST_SILVER : id));
	writeByte(0);
	writeByte(static_cast<BYTE>(layer));
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
PacketSwing::PacketSwing(const CClient* target, const CChar* defender) : PacketSend(PACKET_Fight, 10, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
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
PacketSkills::PacketSkills(const CClient* target, const CChar* character, SKILL_TYPE skill) : PacketSend(PACKET_Skill, 15, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketSkills::PacketSkills");

	initLength();

	if (!character)
		character = target->GetChar();

	bool includeCaps = target->m_NetState->isClientVersion(MINCLIVER_AOS);
	if (skill >= SKILL_QTY)
	{
		// all skills
		if (includeCaps)
			writeByte(0x02);
		else
			writeByte(0x00);

		for (size_t i = 0; i < g_Cfg.m_iMaxSkill; i++)
		{
			if (!g_Cfg.m_SkillIndexDefs.IsValidIndex(i))
				continue;

			writeInt16(static_cast<WORD>(i + 1));
			writeInt16(character->Skill_GetAdjusted(static_cast<SKILL_TYPE>(i)));
			writeInt16(character->Skill_GetBase(static_cast<SKILL_TYPE>(i)));
			writeByte(static_cast<BYTE>(character->Skill_GetLock(static_cast<SKILL_TYPE>(i))));
			if (includeCaps)
				writeInt16(character->Skill_GetMax(static_cast<SKILL_TYPE>(i)));
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

		writeInt16(static_cast<WORD>(skill));
		writeInt16(character->Skill_GetAdjusted(skill));
		writeInt16(character->Skill_GetBase(skill));
		writeByte(static_cast<BYTE>(character->Skill_GetLock(skill)));
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
PacketCloseVendor::PacketCloseVendor(const CClient* target, const CChar* vendor) : PacketSend(PACKET_VendorBuy, 8, PRI_NORMAL)
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
PacketItemContents::PacketItemContents(CClient* target, const CItemContainer* container, bool isShop, bool filterLayers) : PacketSend(PACKET_Content, 5, PRI_NORMAL), m_container(container->GetUID())
{
	ADDTOCALLSTACK("PacketItemContents::PacketItemContents");

	const CChar* viewer = target->GetChar();

	bool isLayerSent[LAYER_HORSE];
	memset(isLayerSent, 0, sizeof(isLayerSent));

	// Skip these layers
	isLayerSent[LAYER_NONE] = true;
	isLayerSent[LAYER_PACK] = true;

	initLength();
	skip(2);

	const CItemBase* itemDefinition;
	ITEMID_TYPE id;
	WORD amount;
	HUE_TYPE hue;
	CPointMap pos;
	BYTE layer;
	WORD count = 0;

	for ( CItem* item = container->GetContentTail(); item != NULL; item = item->GetPrev() )
	{
		itemDefinition = item->Item_GetDef();
		id = item->GetDispID();
		amount = item->GetAmount();
		hue = item->GetHue() & HUE_MASK_HI;
		pos = item->GetContainedPoint();

		if ( isShop )
		{
			const CItemVendable* vendorItem = static_cast<const CItemVendable *>(item);
			if ( !vendorItem || (vendorItem->GetAmount() == 0) || vendorItem->IsType(IT_GOLD) )
				continue;

			amount = minimum(static_cast<WORD>(g_Cfg.m_iVendorMaxSell), amount);
			pos.m_x = count + 1;
			pos.m_y = 1;
		}
		else
		{
			if ( item->IsAttr(ATTR_INVIS) && !viewer->CanSee(item) )
				continue;
		}

		if ( filterLayers )
		{
			layer = item->GetContainedLayer();
			if ( isLayerSent[layer] )
				continue;

			isLayerSent[layer] = true;
		}

		if ( itemDefinition && (target->GetResDisp() < itemDefinition->GetResLevel()) )
		{
			id = static_cast<ITEMID_TYPE>(itemDefinition->GetResDispDnId());

			if ( itemDefinition->GetResDispDnHue() != HUE_DEFAULT )
				hue = itemDefinition->GetResDispDnHue() & HUE_MASK_HI;
		}

		if ( hue > HUE_QTY )
			hue &= HUE_MASK_LO;		// restrict colors

		// write item data
		writeInt32(item->GetUID());
		writeInt16(static_cast<WORD>(id));
		writeByte(0);
		writeInt16(amount);
		writeInt16(pos.m_x);
		writeInt16(pos.m_y);
		if ( target->m_ContainerGridEnabled )
			writeByte(item->GetContainedGridIndex());
		writeInt32(container->GetUID());
		writeInt16(static_cast<WORD>(hue));

		// include tooltip
		target->addAOSTooltip(item, false, isShop);

		if ( ++count >= MAX_ITEMS_CONT )
			break;
	}

	// write item count
	size_t l = getPosition();
	seek(3);
	writeInt16(count);
	seek(l);

	push(target);
}

PacketItemContents::PacketItemContents(const CClient* target, const CItem* spellbook) : PacketSend(PACKET_Content, 5, PRI_NORMAL), m_container(spellbook->GetUID())
{
	ADDTOCALLSTACK("PacketItemContents::PacketItemContents(2)");

	initLength();
	skip(2);

	WORD count = 0;
	for (WORD i = SPELL_Clumsy; i <= SPELL_MAGERY_QTY; i++)
	{
		if (!spellbook->IsSpellInBook(static_cast<SPELL_TYPE>(i)))
			continue;

		writeInt32(UID_F_ITEM + UID_O_INDEX_FREE + i);
		writeInt16(0x1F2E);
		writeByte(0);
		writeInt16(i);
		writeInt16(0);
		writeInt16(0);
		if (target->m_ContainerGridEnabled)
			writeByte(static_cast<BYTE>(count));
		writeInt32(spellbook->GetUID());
		writeInt16(HUE_DEFAULT);

		count++;
	}

	// write item count
	size_t l = getPosition();
	seek(3);
	writeInt16(count);
	seek(l);

	push(target);
}

PacketItemContents::PacketItemContents(const CClient* target, const CItemContainer* spellbook) : PacketSend(PACKET_Content, 5, PRI_NORMAL), m_container(spellbook->GetUID())
{
	ADDTOCALLSTACK("PacketItemContents::PacketItemContents(3)");

	const CSpellDef* spellDefinition;

	initLength();
	skip(2);

	WORD count = 0;
	for (CItem* item = spellbook->GetContentHead(); item != NULL; item = item->GetNext())
	{
		if (!item->IsType(IT_SCROLL))
			continue;

		spellDefinition = g_Cfg.GetSpellDef(static_cast<SPELL_TYPE>(item->m_itSpell.m_spell));
		if (!spellDefinition)
			continue;

		writeInt32(item->GetUID());
		writeInt16(static_cast<WORD>(spellDefinition->m_idScroll));
		writeByte(0);
		writeInt16(item->m_itSpell.m_spell);
		writeInt16(0);
		writeInt16(0);
		if (target->m_ContainerGridEnabled)
			writeByte(static_cast<BYTE>(count));
		writeInt32(spellbook->GetUID());
		writeInt16(static_cast<WORD>(HUE_DEFAULT));

		count++;
	}

	// write item count
	size_t l = getPosition();
	seek(3);
	writeInt16(count);
	seek(l);

	push(target);
}

bool PacketItemContents::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketItemContents::onSend");
	if (g_NetworkManager.isOutputThreaded())
		return true;

	return client->CanSee(m_container.ItemFind());
}

/***************************************************************************
 *
 *
 *	Packet 0x3F : PacketQueryClient			Query Client for block info (NORMAL)
 *
 *
 ***************************************************************************/
PacketQueryClient::PacketQueryClient(CClient* target, BYTE bCmd) : PacketSend(PACKET_StaticUpdate, 15, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketQueryClient::PacketQueryClient");
	initLength();
	switch (bCmd)
	{
		case 0x1:
		{
			// Update map definitions
			int length = 2 * 9;		// map count * 9
			int count = length / 7;
			int padding = 0;
			if (length - (count * 7) > 0)
			{
				count++;
				padding = (count * 7) - length; 
			}

			writeInt32(0);
			writeInt32(4);
			writeInt16(0);
			writeByte(0x1);
			writeByte(0);

			for (int i = 0; i < 2; i++)
			{
				writeByte(static_cast<BYTE>(i));
				writeInt16(static_cast<WORD>(g_MapList.GetX(i)));
				writeInt16(static_cast<WORD>(g_MapList.GetY(i)));
				writeInt16(static_cast<WORD>(g_MapList.GetX(i)));
				writeInt16(static_cast<WORD>(g_MapList.GetY(i)));
			}

			for (int i = 0; i < padding; i++)
				writeByte(0);
			break;
		}
		case 0x2:
		{
			// Login complete
			writeInt32(1);
			writeInt32(4);
			writeInt16(0);
			writeByte(0x2);
			writeByte(0);
			writeStringFixedASCII(g_Serv.GetName(),28);
			break;
		}
		case 0x3:
		{
			// Refresh client view
			writeInt32(0);
			writeInt32(0);
			writeInt16(0);
			writeByte(0x3);
			writeByte(0);
			break;
		}
		case 0xFF:
		{
			// Query client
			CPointMap pt = target->GetChar()->GetTopPoint();
			DWORD dwBlockId = (pt.m_x * (g_MapList.GetY(target->GetChar()->GetTopMap()) / UO_BLOCK_SIZE)) + pt.m_y;
			writeInt32(dwBlockId);
			writeInt32(0);
			writeInt16(0);
			writeByte(0xFF);
			writeByte(target->GetChar()->GetTopMap());
			break;
		}
	}

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x4F : PacketGlobalLight			sets global light level (NORMAL)
 *
 *
 ***************************************************************************/
PacketGlobalLight::PacketGlobalLight(const CClient* target, BYTE light) : PacketSend(PACKET_Light, 2, PRI_NORMAL)
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
PacketWarningMessage::PacketWarningMessage(const CClient* target, PacketWarningMessage::Message code) : PacketSend(PACKET_IdleWarning, 2, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketWarningMessage::PacketWarningMessage");

	writeByte(static_cast<BYTE>(code));
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x54 : PacketPlaySound			play a sound (NORMAL)
 *
 *
 ***************************************************************************/
PacketPlaySound::PacketPlaySound(const CClient *target, SOUND_TYPE sound, BYTE flags, WORD volume, const CPointMap &pos) : PacketSend(PACKET_Sound, 12, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPlaySound::PacketPlaySound");

	writeByte(flags);
	writeInt16(static_cast<WORD>(sound));
	writeInt16(volume);
	writeInt16(pos.m_x);
	writeInt16(pos.m_y);
	writeInt16(pos.m_z);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x55 : PacketLoginComplete		redraw all (NORMAL)
 *
 *
 ***************************************************************************/
PacketLoginComplete::PacketLoginComplete(const CClient* target) : PacketSend(PACKET_LoginComplete, 1, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketLoginComplete::PacketLoginComplete");

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x56 : PacketMapPlot				show/edit map plots (LOW)
 *
 *
 ***************************************************************************/
PacketMapPlot::PacketMapPlot(const CClient* target, const CItem* map, MAPCMD_TYPE mode, bool edit) : PacketSend(PACKET_MapEdit, 11, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketMapPlot::PacketMapPlot");

	writeInt32(map->GetUID());
	writeByte(static_cast<BYTE>(mode));
	writeBool(edit);
	writeInt16(0);
	writeInt16(0);

	push(target);
}

PacketMapPlot::PacketMapPlot(const CItem* map, MAPCMD_TYPE mode, bool edit) : PacketSend(PACKET_MapEdit, 11, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketMapPlot::PacketMapPlot");

	writeInt32(map->GetUID());
	writeByte(static_cast<BYTE>(mode));
	writeBool(edit);
}

void PacketMapPlot::setPin(WORD x, WORD y)
{
	ADDTOCALLSTACK("PacketMapPlot::setPin");

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
PacketGameTime::PacketGameTime(const CClient *target, BYTE hours, BYTE minutes, BYTE seconds) : PacketSend(PACKET_Time, 4, g_Cfg.m_fUsePacketPriorities? PRI_IDLE : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketGameTime::PacketGameTime");

	writeByte(hours);
	writeByte(minutes);
	writeByte(seconds);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x65 : PacketWeather				set current weather (IDLE)
 *
 *
 ***************************************************************************/
PacketWeather::PacketWeather(const CClient *target, WEATHER_TYPE weather, BYTE severity, BYTE temperature) : PacketSend(PACKET_Weather, 4, g_Cfg.m_fUsePacketPriorities? PRI_IDLE : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketWeather::PacketWeather");

	writeByte(static_cast<BYTE>(weather));
	writeByte(severity);
	writeByte(temperature);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x66 : PacketBookPageContent		send book page content (LOW)
 *
 *
 ***************************************************************************/
PacketBookPageContent::PacketBookPageContent(const CClient *target, const CItem *book, WORD startpage, WORD pagecount) : PacketSend(PACKET_BookPage, 8, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketBookPageContent::PacketBookPageContent");

	m_pages = 0;

	initLength();
	writeInt32(book->GetUID());
	writeInt16(0);

	for (WORD i = 0; i < pagecount; i++)
		addPage(book, startpage + i);

	push(target);
}

void PacketBookPageContent::addPage(const CItem *book, WORD page)
{
	ADDTOCALLSTACK("PacketBookPageContent::addPage");

	writeInt16(page);

	// skip line count for now
	size_t linesPos = getPosition();
	WORD lines = 0;
	writeInt16(0);

	if (book->IsBookSystem())
	{
		CResourceLock s;
		if (g_Cfg.ResourceLock(s, RESOURCE_ID(RES_BOOK, book->m_itBook.m_ResID.GetResIndex(), static_cast<int>(page))))
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
		const CItemMessage* message = dynamic_cast<const CItemMessage *>(book);
		if (message)
		{
			if ((page > 0) && (page <= message->GetPageCount()))
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
PacketAddTarget::PacketAddTarget(const CClient* target, PacketAddTarget::TargetType type, DWORD context, PacketAddTarget::Flags flags) : PacketSend(PACKET_Target, 19, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketAddTarget::PacketAddTarget");

	writeByte(static_cast<BYTE>(type));
	writeInt32(context);
	writeByte(static_cast<BYTE>(flags));

	// unused data
	writeInt32(0);
	writeInt16(0);
	writeInt16(0);
	writeByte(0);
	writeByte(0);
	writeInt16(0);

	push(target);
}

PacketAddTarget::PacketAddTarget(const CClient* target, PacketAddTarget::TargetType type, DWORD context, PacketAddTarget::Flags flags, ITEMID_TYPE id, HUE_TYPE color) : PacketSend(PACKET_TargetMulti, 30, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketAddTarget::PacketAddTarget(2)");

	CItemBase *pItemDef = CItemBase::FindItemBase(static_cast<ITEMID_TYPE>(RES_GET_INDEX(id)));
	if ( !pItemDef )
		return;

	WORD y = 0;
	CItemBaseMulti *pMultiDef = dynamic_cast<CItemBaseMulti *>(pItemDef);
	if ( pMultiDef && CItemBase::IsID_House(id) )
		y = static_cast<WORD>(pMultiDef->m_rect.m_bottom) - 1;

	writeByte(static_cast<BYTE>(type));
	writeInt32(context);
	writeByte(static_cast<BYTE>(flags));
	
	writeInt32(0);
	writeInt32(0);
	writeInt16(0);
	writeByte(0);

	writeInt16(static_cast<WORD>(id - ITEMID_MULTI));
	writeInt16(0);	// x
	writeInt16(y);	// y
	writeInt16(0);	// z

	if ( target->m_NetState->isClientVersion(MINCLIVER_HS) )
		writeInt32(static_cast<DWORD>(color));

	trim();
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x6D : PacketPlayMusic			adds music to the client (IDLE)
 *
 *
 ***************************************************************************/
PacketPlayMusic::PacketPlayMusic(const CClient* target, WORD musicID) : PacketSend(PACKET_PlayMusic, 3, g_Cfg.m_fUsePacketPriorities? PRI_IDLE : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPlayMusic::PacketPlayMusic");

	writeInt16(musicID);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x6E : PacketAction				plays an animation (LOW)
 *	Packet 0xE2 : PacketActionNew			plays an animation (LOW)
 *
 ***************************************************************************/
PacketAction::PacketAction(const CChar* character, ANIM_TYPE action, WORD repeat, bool backward, BYTE delay, BYTE len) : PacketSend(PACKET_CharAction, 14, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketAction::PacketAction");

	writeInt32(character->GetUID());
	writeInt16(static_cast<WORD>(action));
	writeInt16(len);
	writeInt16(repeat);
	writeBool(backward);
	writeBool(repeat != 1);
	writeByte(delay);
}

PacketActionNew::PacketActionNew(const CChar* character, ANIM_TYPE_NEW action, ANIM_TYPE_NEW subaction, BYTE variation) : PacketSend(PACKET_NewAnimUpdate, 10, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketActionNew::PacketActionNew");

	writeInt32(character->GetUID());
	writeInt16(static_cast<WORD>(action));
	writeInt16(static_cast<WORD>(subaction));
	writeByte(variation);
}

/***************************************************************************
 *
 *
 *	Packet 0x6F : PacketTradeAction			perform a trade action (NORMAL)
 *
 *
 ***************************************************************************/
PacketTradeAction::PacketTradeAction(SECURETRADE_TYPE action) : PacketSend(PACKET_SecureTrade, 17, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketTradeAction::PacketTradeAction");

	initLength();
	writeByte(static_cast<BYTE>(action));
}

void PacketTradeAction::prepareContainerOpen(const CChar *character, const CItem *container1, const CItem *container2)
{
	ADDTOCALLSTACK("PacketTradeAction::prepareContainerOpen");

	seek(4);
	writeInt32(character->GetUID());
	writeInt32(container1->GetUID());
	writeInt32(container2->GetUID());
	writeBool(true);
	writeStringFixedASCII(character->GetName(), MAX_NAME_SIZE);
}
	
void PacketTradeAction::prepareReadyChange(const CItemContainer *container1, const CItemContainer *container2)
{
	ADDTOCALLSTACK("PacketTradeAction::prepareReadyChange");

	seek(4);
	writeInt32(container1->GetUID());
	writeInt32(container1->m_itEqTradeWindow.m_bCheck);
	writeInt32(container2->m_itEqTradeWindow.m_bCheck);
	writeBool(false);
}

void PacketTradeAction::prepareClose(const CItemContainer *container)
{
	ADDTOCALLSTACK("PacketTradeAction::prepareClose");

	seek(4);
	writeInt32(container->GetUID());
	writeInt32(0);
	writeInt32(0);
	writeBool(false);
}

void PacketTradeAction::prepareUpdateGold(const CItemContainer *container, DWORD gold, DWORD platinum)
{
	ADDTOCALLSTACK("PacketTradeAction::prepareUpdateGold");

	seek(4);
	writeInt32(container->GetUID());
	writeInt32(gold);
	writeInt32(platinum);
	writeBool(false);
}

void PacketTradeAction::prepareUpdateLedger(const CItemContainer *container, DWORD gold, DWORD platinum)
{
	ADDTOCALLSTACK("PacketTradeAction::prepareUpdateLedger");

	seek(4);
	writeInt32(container->GetUID());
	writeInt32(gold);
	writeInt32(platinum);
	writeBool(false);
}


/***************************************************************************
 *
 *
 *	Packet 0x70 : PacketEffect				displays a visual effect (NORMAL)
 *	Packet 0xC0 : PacketEffect				displays a hued visual effect (NORMAL)
 *	Packet 0xC7 : PacketEffect				displays a particle visual effect (NORMAL)
 *
 *
 ***************************************************************************/
PacketEffect::PacketEffect(const CClient *target, EFFECT_TYPE motion, const CObjBaseTemplate *pSrc, CPointMap ptSrc, const CObjBaseTemplate *pDest, CPointMap ptDest, ITEMID_TYPE id, BYTE bSpeed, BYTE bFrames, bool fExplode) : PacketSend(PACKET_Effect, 20, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketEffect::PacketEffect");

	writeBasicEffect(motion, pSrc, ptSrc, pDest, ptDest, id, bSpeed, bFrames, fExplode);

	push(target);
}

PacketEffect::PacketEffect(const CClient *target, EFFECT_TYPE motion, const CObjBaseTemplate *pSrc, CPointMap ptSrc, const CObjBaseTemplate *pDest, CPointMap ptDest, ITEMID_TYPE id, BYTE bSpeed, BYTE bFrames, bool fExplode, DWORD dwColor, DWORD dwRender) : PacketSend(PACKET_EffectEx, 28, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketEffect::PacketEffect(2)");

	writeBasicEffect(motion, pSrc, ptSrc, pDest, ptDest, id, bSpeed, bFrames, fExplode);
	writeInt32(dwColor);
	writeInt32(dwRender);

	push(target);
}

PacketEffect::PacketEffect(const CClient *target, EFFECT_TYPE motion, const CObjBaseTemplate *pSrc, CPointMap ptSrc, const CObjBaseTemplate *pDest, CPointMap ptDest, ITEMID_TYPE id, BYTE bSpeed, BYTE bFrames, bool fExplode, DWORD dwColor, DWORD dwRender, WORD wEffectID, WORD wExplodeID, WORD wExplodeSound, DWORD dwItemUID, BYTE bLayer) : PacketSend(PACKET_EffectParticle, 49, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketEffect::PacketEffect(3)");

	writeBasicEffect(motion, pSrc, ptSrc, pDest, ptDest, id, bSpeed, bFrames, fExplode);
	writeInt32(dwColor);
	writeInt32(dwRender);
	writeInt16(wEffectID);
	writeInt16(wExplodeID);
	writeInt16(wExplodeSound);
	writeInt32(dwItemUID);
	writeByte((bLayer == 0) ? 0xFF : 0x3);
	writeInt16(0);

	push(target);
}

void PacketEffect::writeBasicEffect(EFFECT_TYPE motion, const CObjBaseTemplate *pSrc, CPointMap ptSrc, const CObjBaseTemplate *pDest, CPointMap ptDest, ITEMID_TYPE id, BYTE bSpeed, BYTE bFrames, bool fExplode)
{
	ADDTOCALLSTACK("PacketEffect::writeBasicEffect");

	writeByte(static_cast<BYTE>(motion));
	writeInt32(pSrc ? static_cast<DWORD>(pSrc->GetUID()) : 0);
	writeInt32(pDest ? static_cast<DWORD>(pDest->GetUID()) : 0);
	writeInt16(static_cast<WORD>(id));
	writeInt16(ptSrc.m_x);
	writeInt16(ptSrc.m_y);
	writeByte(ptSrc.m_z);
	writeInt16(ptDest.m_x);
	writeInt16(ptDest.m_y);
	writeByte(ptDest.m_z);
	writeByte(bSpeed);
	writeByte(bFrames);
	writeInt16(0);
	writeBool(motion != EFFECT_BOLT);
	writeBool(fExplode);
}


/***************************************************************************
 *
 *
 *	Packet 0x71 : PacketBulletinBoard		display a bulletin board or message (NORMAL/LOW)
 *
 *
 ***************************************************************************/
PacketBulletinBoard::PacketBulletinBoard(const CClient* target, const CItemContainer* board) : PacketSend(PACKET_BBoard, 20, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketBulletinBoard::PacketBulletinBoard");

	initLength();
	writeByte(BULLETINBOARD_Name);
	writeInt32(board->GetUID());
	writeStringASCII(board->GetName());

	push(target);
}

PacketBulletinBoard::PacketBulletinBoard(const CClient* target, BULLETINBOARD_TYPE action, const CItemContainer* board, const CItemMessage* message) : PacketSend(PACKET_BBoard, 20, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketBulletinBoard::PacketBulletinBoard(2)");

	initLength();
	writeByte(static_cast<BYTE>(action == BULLETINBOARD_ReqFull ? BULLETINBOARD_MsgFull : BULLETINBOARD_MsgTitle));
	writeInt32(board->GetUID());

	writeInt32(message->GetUID());
	if (action == BULLETINBOARD_ReqTitle)
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

		lenstr = strlen(author) + 1;
		if (lenstr > BYTE_MAX)
			lenstr = BYTE_MAX;

		writeByte(static_cast<BYTE>(lenstr));
		writeStringFixedASCII(author, lenstr);
	}

	// message title
	lenstr = strlen(message->GetName()) + 1;
	if (lenstr > BYTE_MAX)
		lenstr = BYTE_MAX;

	writeByte(static_cast<BYTE>(lenstr));
	writeStringFixedASCII(message->GetName(), lenstr);

	// message time
	sprintf(tempstr, "Day %" FMTDWORD, (g_World.GetGameWorldTime(message->GetTimeStamp()) / (24 * 60)) % 365);
	lenstr = strlen(tempstr) + 1;

	writeByte(static_cast<BYTE>(lenstr));
	writeStringFixedASCII(tempstr, lenstr);

	if (action == BULLETINBOARD_ReqFull)
	{
		// requesst for full message body
		writeInt32(0);

		WORD lines = message->GetPageCount();
		writeInt16(lines);

		for (WORD i = 0; i < lines; i++)
		{
			LPCTSTR text = message->GetPageText(i);
			if (text == NULL)
				continue;

			lenstr = strlen(text) + 2;
			if (lenstr > BYTE_MAX)
				lenstr = BYTE_MAX;

			writeByte(static_cast<BYTE>(lenstr));
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
PacketWarMode::PacketWarMode(const CClient* target, const CChar* character) : PacketSend(PACKET_War, 5, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
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
PacketPingAck::PacketPingAck(const CClient* target, BYTE value) : PacketSend(PACKET_Ping, 2, g_Cfg.m_fUsePacketPriorities? PRI_IDLE : PRI_NORMAL)
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
PacketVendorBuyList::PacketVendorBuyList(const CClient* target, const CChar *vendor, const CItemContainer* contParent) : PacketSend(PACKET_VendOpenBuy, 8, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketVendorBuyList::PacketVendorBuyList");

	initLength();
	writeInt32(contParent->GetUID());

	size_t count = 0;
	size_t countpos = getPosition();
	skip(1);

	int convertFactor = vendor->NPC_GetVendorMarkup();

	// Enhanced Client wants the prices to be sent in reverse order
	bool bIsClientEnhanced = target->m_NetState->isClientEnhanced();

	for ( CItem *item = (bIsClientEnhanced ? contParent->GetContentHead() : contParent->GetContentTail()); item != NULL; item = (bIsClientEnhanced ? item->GetNext() : item->GetPrev()) )
	{
		CItemVendable* vendorItem = static_cast<CItemVendable *>(item);
		if (!vendorItem || (vendorItem->GetAmount() == 0))
			continue;

		DWORD price = vendorItem->GetVendorPrice(convertFactor);
		if (price == 0)
		{
			vendorItem->Item_GetDef()->ResetMakeValue();
			price = vendorItem->GetVendorPrice(convertFactor);

			if ((price == 0) && vendorItem->IsValidNPCSaleItem())
				price = vendorItem->GetBasePrice();
			if (price == 0)
				price = 100000;
		}

		LPCTSTR name = vendorItem->GetName();
		size_t len = strlen(name) + 1;
		if (len > BYTE_MAX)
			len = BYTE_MAX;

		writeInt32(price);
		writeByte(static_cast<BYTE>(len));
		writeStringFixedASCII(name, len);

		if ( ++count > MAX_ITEMS_CONT )
			break;
	}

	// seek back to write count
	size_t endpos = getPosition();
	seek(countpos);
	writeByte(static_cast<BYTE>(count));
	seek(endpos);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x76 : PacketZoneChange			change server zone (LOW)
 *
 *
 ***************************************************************************/
PacketZoneChange::PacketZoneChange(const CClient* target, const CPointMap& pos) : PacketSend(PACKET_ZoneChange, 16, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketZoneChange::PacketZoneChange");

	writeInt16(pos.m_x);
	writeInt16(pos.m_y);
	writeInt16(pos.m_z);
	writeByte(0);
	writeInt16(0);
	writeInt16(0);
	writeInt16(static_cast<WORD>(g_MapList.GetX(pos.m_map)));
	writeInt16(static_cast<WORD>(g_MapList.GetY(pos.m_map)));

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x77 : PacketCharacterMove		move a character (NORMAL)
 *
 *
 ***************************************************************************/
PacketCharacterMove::PacketCharacterMove(const CClient* target, const CChar* character, BYTE direction) : PacketSend(PACKET_CharMove, 17, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketCharacterMove::PacketCharacterMove");
	// NOTE: This packet move characters on screen, but can't move the
	// client char receiving the packet (use packet 0x20 instead).

	CREID_TYPE id;
	HUE_TYPE hue;
	target->GetAdjustedCharID(character, id, hue);
	const CPointMap& pos = character->GetTopPoint();

	writeInt32(character->GetUID());
	writeInt16(static_cast<WORD>(id));
	writeInt16(pos.m_x);
	writeInt16(pos.m_y);
	writeByte(pos.m_z);
	writeByte(direction);
	writeInt16(hue);
	writeByte(character->GetModeFlag(target));
	writeByte(static_cast<BYTE>(character->Noto_GetFlag(target->GetChar(), target->m_NetState->isClientVersion(MINCLIVER_AOS), true)));

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x78 : PacketCharacter			create a character (NORMAL)
 *
 *
 ***************************************************************************/
PacketCharacter::PacketCharacter(CClient* target, const CChar* character) : PacketSend(PACKET_Char, 23, PRI_NORMAL), m_character(character->GetUID())
{
	ADDTOCALLSTACK("PacketCharacter::PacketCharacter");

	const CChar* viewer = target->GetChar();
	ASSERT(viewer);

	CREID_TYPE id;
	HUE_TYPE hue;
	target->GetAdjustedCharID(character, id, hue);
	const CPointMap &pos = character->GetTopPoint();

	initLength();
	writeInt32(character->GetUID());
	writeInt16(static_cast<WORD>(id));
	writeInt16(pos.m_x);
	writeInt16(pos.m_y);
	writeByte(pos.m_z);
	writeByte(character->GetDirFlag());
	writeInt16(hue);
	writeByte(character->GetModeFlag(target));
	writeByte(static_cast<BYTE>(character->Noto_GetFlag(target->GetChar(), target->m_NetState->isClientVersion(MINCLIVER_AOS), true)));

	bool isNewMobilePacket = target->m_NetState->isClientVersion(MINCLIVER_NEWMOBINCOMING);
	bool isLayerSent[LAYER_HORSE + 1];
	memset(isLayerSent, 0, sizeof(isLayerSent));

	for ( CItem *pItem = character->GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		LAYER_TYPE layer = pItem->GetEquipLayer();
		if ( !CItemBase::IsVisibleLayer(layer) )
			continue;
		if ( (viewer != character) && !viewer->CanSeeItem(pItem) )
			continue;

		// Prevent same layer to being sent twice
		if ( isLayerSent[layer] )
			continue;

		isLayerSent[layer] = true;

		ITEMID_TYPE itemid;
		target->GetAdjustedItemID(character, pItem, itemid, hue);
		target->addAOSTooltip(pItem);

		writeInt32(pItem->GetUID());

		if ( isNewMobilePacket )
		{
			writeInt16(static_cast<WORD>(itemid));
			writeByte(static_cast<BYTE>(layer));
			writeInt16(static_cast<WORD>(hue));
		}
		else if ( hue != 0 )
		{
			writeInt16(static_cast<WORD>(itemid|0x8000));
			writeByte(static_cast<BYTE>(layer));
			writeInt16(static_cast<WORD>(hue));
		}
		else
		{
			writeInt16(static_cast<WORD>(itemid));
			writeByte(static_cast<BYTE>(layer));
		}
	}

	writeInt32(0);

	push(target);
}

bool PacketCharacter::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketCharacter::onSend");
	if (g_NetworkManager.isOutputThreaded())
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
PacketDisplayMenu::PacketDisplayMenu(const CClient* target, CLIMODE_TYPE mode, const CMenuItem* items, size_t count, const CObjBase* object) : PacketSend(PACKET_MenuItems, 11, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDisplayMenu::PacketDisplayMenu");

	initLength();
	writeInt32(object->GetUID());
	writeInt16(static_cast<WORD>(mode));

	size_t len = items[0].m_sText.GetLength();
	if (len > BYTE_MAX)
		len = BYTE_MAX;
	writeByte(static_cast<BYTE>(len));
	writeStringFixedASCII(static_cast<LPCTSTR>(items[0].m_sText), len);

	writeByte(static_cast<BYTE>(count));
	for (size_t i = 1; i <= count; i++)
	{
		writeInt16(items[i].m_id);
		writeInt16(items[i].m_color);

		len = items[i].m_sText.GetLength();
		if (len > BYTE_MAX )
			len = BYTE_MAX;
		writeByte(static_cast<BYTE>(len));
		writeStringFixedASCII(static_cast<LPCTSTR>(items[i].m_sText), len);
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
PacketChangeCharacter::PacketChangeCharacter(CClient* target) : PacketSend(PACKET_CharList3, 5, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketChangeCharacter::PacketChangeCharacter");

	initLength();

	size_t countPos = getPosition();
	skip(1);

	writeByte(0);
	BYTE count = target->Setup_FillCharList(this);

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
PacketLoginError::PacketLoginError(const CClient* target, PacketLoginError::Reason reason) : PacketSend(PACKET_LogBad, 2, PRI_HIGHEST)
{
	ADDTOCALLSTACK("PacketLoginError::PacketLoginError");

	writeByte(static_cast<BYTE>(reason));
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x85 : PacketDeleteError			delete character error response (LOW)
 *
 *
 ***************************************************************************/
PacketDeleteError::PacketDeleteError(const CClient* target, PacketDeleteError::Reason reason) : PacketSend(PACKET_DeleteBad, 2, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDeleteError::PacketDeleteError");

	writeByte(static_cast<BYTE>(reason));
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x86 : PacketCharacterListUpdate	update character list (LOW)
 *
 *
 ***************************************************************************/
PacketCharacterListUpdate::PacketCharacterListUpdate(CClient* target) : PacketSend(PACKET_CharList2, 4, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketCharacterListUpdate::PacketCharacterListUpdate");

	initLength();

	size_t countPos = getPosition();
	skip(1);

	BYTE count = target->Setup_FillCharList(this);

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
PacketPaperdoll::PacketPaperdoll(const CClient* target, const CChar* character) : PacketSend(PACKET_PaperDoll, 66, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPaperdoll::PacketPaperdoll");

	TCHAR *name = Str_GetTemp();
	LPCTSTR title = character->GetTradeTitle();
	if ( title[0] )
		sprintf(name, "%s, %s", character->Noto_GetTitle(), title);
	else
		sprintf(name, "%s", character->Noto_GetTitle());

	BYTE flags = 0;
	if (character->IsStatFlag(STATF_War))
		flags |= target->m_NetState->isClientVersion(MINCLIVER_AOS) ? 0x1 : 0x40;
	if (target->m_NetState->isClientVersion(MINCLIVER_AOS))
	{
		if (character == target->GetChar() || (g_Cfg.m_fCanUndressPets ? character->NPC_IsOwnedBy(target->GetChar()) : (target->IsPriv(PRIV_GM) && (target->GetPrivLevel() > character->GetPrivLevel()))))
			flags |= 0x2;
	}

	writeInt32(character->GetUID());
	writeStringFixedASCII(name, MAX_NAME_SIZE * 2);
	writeByte(flags);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x89 : PacketCorpseEquipment		send corpse equipment (NORMAL)
 *
 *
 ***************************************************************************/
PacketCorpseEquipment::PacketCorpseEquipment(CClient* target, const CItemContainer* corpse) : PacketSend(PACKET_CorpEquip, 7, PRI_NORMAL), m_corpse(corpse->GetUID())
{
	ADDTOCALLSTACK("PacketCorpseEquipment::PacketCorpseEquipment");

	const CChar* viewer = target->GetChar();

	bool isLayerSent[LAYER_HORSE];
	memset(isLayerSent, 0, sizeof(isLayerSent));

	// Skip these layers
	isLayerSent[LAYER_NONE] = true;
	isLayerSent[LAYER_PACK] = true;

	initLength();
	writeInt32(corpse->GetUID());

	BYTE layer;
	WORD count = 0;

	for (CItem* item = corpse->GetContentHead(); item != NULL; item = item->GetNext())
	{
		if (item->IsAttr(ATTR_INVIS) && !viewer->CanSee(item))
			continue;
		
		layer = item->GetContainedLayer();
		if ( isLayerSent[layer] )
			continue;

		writeByte(layer + 1);
		writeInt32(item->GetUID());

		target->addAOSTooltip(item);

		if (++count > MAX_ITEMS_CONT)
			break;
	}

	writeByte(0);
	push(target);
}

bool PacketCorpseEquipment::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketCorpseEquipment::onSend");
	if (g_NetworkManager.isOutputThreaded())
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
PacketSignGump::PacketSignGump(const CClient* target, const CObjBase* object, GUMP_TYPE gump, LPCTSTR name, LPCTSTR text) : PacketSend(PACKET_GumpTextDisp, 13, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketSignGump::PacketSignGump");

	initLength();
	writeInt32(object->GetUID());
	writeInt16(static_cast<WORD>(gump));

	if (name != NULL)
	{
		size_t len = strlen(name) + 1;
		writeInt16(static_cast<WORD>(len));
		writeStringFixedASCII(name, len);
	}
	else
	{
		writeInt16(0);
	}

	if (text != NULL)
	{
		size_t len = strlen(text) + 1;
		writeInt16(static_cast<WORD>(len));
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
PacketServerRelay::PacketServerRelay(const CClient* target, DWORD ip, WORD port, DWORD customerId) : PacketSend(PACKET_Relay, 11, g_Cfg.m_fUsePacketPriorities? PRI_IDLE : PRI_NORMAL)
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
PacketDisplayMap::PacketDisplayMap(const CClient* target, const CItemMap* map, const CRectMap& rect) : PacketSend(PACKET_MapDisplay, 19, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDisplayMap::PacketDisplayMap");

	const CItemBase* itemDef = map->Item_GetDef();
	ASSERT(itemDef);

	writeInt32(map->GetUID());
	writeInt16(GUMP_MAP_2_NORTH);
	writeInt16(static_cast<WORD>(rect.m_left));
	writeInt16(static_cast<WORD>(rect.m_top));
	writeInt16(static_cast<WORD>(rect.m_right));
	writeInt16(static_cast<WORD>(rect.m_bottom));
	writeInt16(static_cast<WORD>(itemDef->m_ttMap.m_iGumpWidth > 0 ? itemDef->m_ttMap.m_iGumpWidth : CItemMap::DEFAULT_SIZE));
	writeInt16(static_cast<WORD>(itemDef->m_ttMap.m_iGumpHeight > 0 ? itemDef->m_ttMap.m_iGumpHeight : CItemMap::DEFAULT_SIZE));

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x93 : PacketDisplayBook			display book (LOW)
 *
 *
 ***************************************************************************/
PacketDisplayBook::PacketDisplayBook(const CClient* target, CItem* book) : PacketSend(PACKET_BookOpen, 99, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDisplayBook::PacketDisplayBook");

	if ( !book )
		return;

	bool isWritable = false;
	WORD pages = 0;
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
						pages = static_cast<WORD>(s.GetArgVal());
						break;
					case CIC_TITLE:
						title = s.GetArgStr();
						break;
				}
			}
		}

		// make sure book is named
		if (!title.IsEmpty())
			book->SetName(static_cast<LPCTSTR>(title));
	}
	else
	{
		// user written book
		const CItemMessage* message = dynamic_cast<const CItemMessage *>(book);
		if (message)
		{
			isWritable = message->IsBookWritable();
			pages = isWritable? MAX_BOOK_PAGES : message->GetPageCount();
			title = message->GetName();
			author = message->m_sAuthor.IsEmpty()? g_Cfg.GetDefaultMsg(DEFMSG_BOOK_AUTHOR_UNKNOWN) : static_cast<LPCTSTR>(message->m_sAuthor);
		}
	}


	writeInt32(book->GetUID());
	writeBool(isWritable);
	writeBool(isWritable);
	writeInt16(pages);
	writeStringFixedASCII(static_cast<LPCTSTR>(title), MAX_NAME_SIZE * 2);
	writeStringFixedASCII(static_cast<LPCTSTR>(author), MAX_NAME_SIZE);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x95 : PacketShowDyeWindow		show dye window (LOW)
 *
 *
 ***************************************************************************/
PacketShowDyeWindow::PacketShowDyeWindow(const CClient* target, const CObjBase* object) : PacketSend(PACKET_DyeVat, 9, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketShowDyeWindow::PacketShowDyeWindow");

	ITEMID_TYPE id;
	if (object->IsItem())
	{
		const CItem *item = static_cast<const CItem *>(object);
		ASSERT(item);
		id = item->GetDispID();
	}
	else
	{
		// get the item equiv for the creature
		const CChar *character = static_cast<const CChar *>(object);
		ASSERT(character);
		id = character->Char_GetDef()->m_trackID;
	}

	writeInt32(object->GetUID());
	writeInt16(object->GetHue());
	writeInt16(static_cast<WORD>(id));

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0x98 : PacketAllNamesResponse	all names macro response (IDLE)
 *
 *
 ***************************************************************************/
PacketAllNamesResponse::PacketAllNamesResponse(const CClient* target, const CObjBase* object) : PacketSend(PACKET_AllNames3D, 37, g_Cfg.m_fUsePacketPriorities? PRI_IDLE : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketAllNamesResponse::PacketAllNamesResponse");

	initLength();
	writeInt32(object->GetUID());
	writeStringFixedASCII(object->GetName(), MAX_NAME_SIZE);

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
PacketAddPrompt::PacketAddPrompt(const CClient* target, CGrayUID context1, CGrayUID context2, bool useUnicode) : PacketSend(static_cast<BYTE>(useUnicode ? PACKET_PromptUNICODE : PACKET_Prompt), 16, g_Cfg.m_fUsePacketPriorities ? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketAddPrompt::PacketAddPrompt");

	initLength();

	writeInt32(context1);
	writeInt32(context2);
	writeInt32(0);

	if (useUnicode)
	{
		writeStringFixedASCII("", sizeof(DWORD));
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
PacketVendorSellList::PacketVendorSellList(const CClient* target, const CChar* vendor, const CItemContainer* contParent, CItemContainer* contBuy) : PacketSend(PACKET_VendOpenSell, 9, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketVendorSellList::PacketVendorSellList");

	initLength();
	writeInt32(vendor->GetUID());

	size_t count = 0;
	size_t countpos = getPosition();
	skip(2);

	int convertFactor = -vendor->NPC_GetVendorMarkup();

	CItem* item = contParent->GetContentHead();
	if (!item)
		return;

	std::deque<const CItemContainer *> otherBoxes;
	for (;;)
	{
		if (item)
		{
			contParent = dynamic_cast<CItemContainer *>(item);
			if (contParent && (contParent->GetCount() > 0))
			{
				if (contParent->IsSearchable())
					otherBoxes.push_back(contParent);
			}
			else
			{
				CItemVendable* vendItem = dynamic_cast<CItemVendable *>(item);
				if (vendItem)
				{
					CItemVendable* vendSell = CChar::NPC_FindVendableItem(vendItem, contBuy);
					if (vendSell)
					{
						HUE_TYPE hue = vendItem->GetHue() & HUE_MASK_HI;
						if (hue > HUE_QTY)
							hue &= HUE_MASK_LO;

						LPCTSTR name = vendItem->GetName();
						size_t len = strlen(name) + 1;
						if (len > BYTE_MAX)
							len = BYTE_MAX;

						writeInt32(vendItem->GetUID());
						writeInt16(static_cast<WORD>(vendItem->GetDispID()));
						writeInt16(static_cast<WORD>(hue));
						writeInt16(vendItem->GetAmount());
						writeInt16(static_cast<WORD>(vendSell->GetVendorPrice(convertFactor)));
						writeInt16(static_cast<WORD>(len));
						writeStringFixedASCII(name, len);
						
						if (++count >= MAX_ITEMS_CONT)
							break;
					}
				}
			}

			item = item->GetNext();
		}
		else
		{
			if (otherBoxes.empty())
				break;

			contParent = otherBoxes.front();
			otherBoxes.pop_front();
			item = contParent->GetContentHead();
		}
	}

	if ( count == 0 )
	{
		const_cast<CChar *>(vendor)->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_SELL_NOTHING));
		return;
	}

	// seek back to write count
	size_t endpos = getPosition();
	seek(countpos);
	writeInt16(static_cast<WORD>(count));
	seek(endpos);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xA1 : PacketHealthUpdate		update character health (LOW)
 *
 *
 ***************************************************************************/
PacketHealthUpdate::PacketHealthUpdate(const CChar* character, bool full) : PacketSend(PACKET_StatChngStr, 9, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketHealthUpdate::PacketHealthUpdate");

	writeInt32(character->GetUID());

	if ( full )
	{
		writeInt16(static_cast<WORD>(character->Stat_GetMax(STAT_STR)));
		writeInt16(static_cast<WORD>(character->Stat_GetVal(STAT_STR)));
	}
	else
	{
		writeInt16(100);
		writeInt16(static_cast<WORD>((character->Stat_GetVal(STAT_STR) * 100) / maximum(character->Stat_GetMax(STAT_STR), 1)));
	}
}


/***************************************************************************
 *
 *
 *	Packet 0xA2 : PacketManaUpdate			update character mana (LOW)
 *
 *
 ***************************************************************************/
PacketManaUpdate::PacketManaUpdate(const CChar* character, bool full) : PacketSend(PACKET_StatChngInt, 9, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketManaUpdate::PacketManaUpdate");

	writeInt32(character->GetUID());

	if ( full )
	{
		writeInt16(static_cast<WORD>(character->Stat_GetMax(STAT_INT)));
		writeInt16(static_cast<WORD>(character->Stat_GetVal(STAT_INT)));
	}
	else
	{
		writeInt16(100);
		writeInt16(static_cast<WORD>((character->Stat_GetVal(STAT_INT) * 100) / maximum(character->Stat_GetMax(STAT_INT), 1)));
	}
}


/***************************************************************************
 *
 *
 *	Packet 0xA3 : PacketStaminaUpdate		update character stamina (LOW)
 *
 *
 ***************************************************************************/
PacketStaminaUpdate::PacketStaminaUpdate(const CChar* character, bool full) : PacketSend(PACKET_StatChngDex, 9, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketStaminaUpdate::PacketStaminaUpdate");

	writeInt32(character->GetUID());

	if ( full )
	{
		writeInt16(static_cast<WORD>(character->Stat_GetMax(STAT_DEX)));
		writeInt16(static_cast<WORD>(character->Stat_GetVal(STAT_DEX)));
	}
	else
	{
		writeInt16(100);
		writeInt16(static_cast<WORD>((character->Stat_GetVal(STAT_DEX) * 100) / maximum(character->Stat_GetMax(STAT_DEX), 1)));
	}
}


/***************************************************************************
 *
 *
 *	Packet 0xA5 : PacketWebPage				send client to a webpage (LOW)
 *
 *
 ***************************************************************************/
PacketWebPage::PacketWebPage(const CClient* target, LPCTSTR url) : PacketSend(PACKET_Web, 3, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
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
PacketOpenScroll::PacketOpenScroll(const CClient* target, CResourceLock &s, SCROLL_TYPE type, DWORD context, LPCTSTR header) : PacketSend(PACKET_Scroll, 10, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketOpenScroll::PacketOpenScroll");

	initLength();

	writeByte(static_cast<BYTE>(type));
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
	writeInt16(static_cast<WORD>(length));
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
PacketServerList::PacketServerList(const CClient* target) : PacketSend(PACKET_ServerList, 46, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketServerList::PacketServerList");

	// clients before 4.0.0 require serverlist ips to be in reverse
	bool reverseIp = target->m_NetState->isClientLessVersion(MINCLIVER_AOS);

	initLength();
	writeByte(0xFF);

	WORD count = 0;
	size_t countPosition = getPosition();
	skip(2);

	writeServerEntry(&g_Serv, ++count, reverseIp);

	for (size_t i = 0; count < MAX_SERVERS; i++)		// too many servers in list can crash the client
	{
		CServerRef server = g_Cfg.Server_GetDef(i);
		if (server == NULL)
			break;

		writeServerEntry(server, ++count, reverseIp);
	}

	size_t endPosition(getPosition());
	seek(countPosition);
	writeInt16(count);
	seek(endPosition);

	push(target);
}

void PacketServerList::writeServerEntry(const CServerRef &server, WORD index, bool reverseIp)
{
	ADDTOCALLSTACK("PacketServerList::writeServerEntry");

	BYTE percentFull = 0;
	DWORD ip = server->m_ip.GetAddrIP();

	if (server == &g_Serv)
		percentFull = static_cast<BYTE>(minimum((server->StatGet(SERV_STAT_CLIENTS) * 100) / maximum(1, g_Cfg.m_iClientsMax), 100));
	else
		percentFull = static_cast<BYTE>(minimum(server->StatGet(SERV_STAT_CLIENTS), 100));

	writeInt16(index);
	writeStringFixedASCII(server->GetName(), MAX_SERVER_NAME_SIZE);
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
PacketCharacterList::PacketCharacterList(CClient* target) : PacketSend(PACKET_CharList, 9, g_Cfg.m_fUsePacketPriorities ? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketCharacterList::PacketCharacterList");

	const CAccount *account = target->m_pAccount;
	ASSERT(account);

	initLength();

	size_t countPos = getPosition();
	skip(1);

	BYTE count = target->Setup_FillCharList(this);

	seek(countPos);
	writeByte(count);
	skip(count * 60);

	size_t startCount = g_Cfg.m_StartDefs.GetCount();
	writeByte(static_cast<BYTE>(startCount));

	// since 7.0.13.0, start locations have extra information
	if ( target->m_NetState->isClientVersion(MINCLIVER_EXTRASTARTINFO) )
	{
		// newer clients receive additional start info
		for ( size_t i = 0; i < startCount; i++ )
		{
			const CStartLoc *start = g_Cfg.m_StartDefs[i];
			writeByte(static_cast<BYTE>(i));
			writeStringFixedASCII(static_cast<LPCTSTR>(start->m_sArea), MAX_NAME_SIZE + 2);
			writeStringFixedASCII(static_cast<LPCTSTR>(start->m_sName), MAX_NAME_SIZE + 2);
			writeInt32(start->m_pt.m_x);
			writeInt32(start->m_pt.m_y);
			writeInt32(start->m_pt.m_z);
			writeInt32(start->m_pt.m_map);
			writeInt32(start->iClilocDescription);
			writeInt32(0);
		}
	}
	else
	{
		for ( size_t i = 0; i < startCount; i++ )
		{
			const CStartLoc *start = g_Cfg.m_StartDefs[i];
			writeByte(static_cast<BYTE>(i));
			writeStringFixedASCII(static_cast<LPCTSTR>(start->m_sArea), MAX_NAME_SIZE + 1);
			writeStringFixedASCII(static_cast<LPCTSTR>(start->m_sName), MAX_NAME_SIZE + 1);
		}
	}

	writeInt32(target->m_CharacterListFlags);

	WORD iLastCharSlot = 0;
	for ( size_t i = 0; i < count; i++ )
	{
		if ( !account->m_Chars.IsValidIndex(i) )
			continue;
		if ( account->m_Chars.GetChar(i) != account->m_uidLastChar )
			continue;

		iLastCharSlot = static_cast<WORD>(i);
		break;
	}
	writeInt16(iLastCharSlot);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xAA : PacketAttack				set attack target (NORMAL)
 *
 *
 ***************************************************************************/
PacketAttack::PacketAttack(const CClient* target, CGrayUID uid) : PacketSend(PACKET_AttackOK, 5, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketAttack::PacketAttack");

	writeInt32(uid);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xAB : PacketGumpValueInput		show input dialog (LOW)
 *
 *
 ***************************************************************************/
PacketGumpValueInput::PacketGumpValueInput(const CClient* target, bool cancel, INPVAL_TYPE type, DWORD maxLength, LPCTSTR text, LPCTSTR caption, CObjBase* object) : PacketSend(PACKET_GumpInpVal, 21, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketGumpValueInput::PacketGumpValueInput");

	initLength();
	writeInt32(object->GetUID());
	writeInt16(CLIMODE_INPVAL);

	size_t len = strlen(text) + 1;
	if (len > WORD_MAX)
		len = WORD_MAX;

	writeInt16(static_cast<WORD>(len));
	writeStringFixedASCII(text, len);

	writeBool(cancel);
	writeByte(static_cast<BYTE>(type));
	writeInt32(maxLength);

	TCHAR* z = NULL;
	switch (type)
	{
		case INPVAL_Disabled:
		default:
			len = 1;
			break;
		case INPVAL_Text:
			z = Str_GetTemp();
			len = sprintf(z, "%s (%" FMTDWORD " chars max)", caption, maxLength) + 1;
			break;
		case INPVAL_Numeric:
			z = Str_GetTemp();
			len = sprintf(z, "%s (0 - %" FMTDWORD ")", caption, maxLength) + 1;
			break;
	}
	if (len > WORD_MAX)
		len = WORD_MAX;

	writeInt16(static_cast<WORD>(len));
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
PacketMessageUNICODE::PacketMessageUNICODE(const CClient* target, const NWORD* pszText, const CObjBaseTemplate * source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID language) : PacketSend(PACKET_SpeakUNICODE, 48, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketMessageUNICODE::PacketMessageUNICODE");

	initLength();

	if (!source)
		writeInt32(0xFFFFFFFF);
	else
		writeInt32(source->GetUID());
	
	if (!source || !source->IsChar())
		writeInt16(0xFFFF);
	else
	{
		const CChar* sourceCharacter = dynamic_cast<const CChar *>(source);
		if ( !sourceCharacter )
			return;
		writeInt16(static_cast<WORD>(sourceCharacter->GetDispID()));
	}
	
	writeByte(static_cast<BYTE>(mode));
	writeInt16(static_cast<WORD>(hue));
	writeInt16(static_cast<WORD>(font));
	writeStringFixedASCII(language.GetStr(), sizeof(DWORD));

	if (source)
		writeStringFixedASCII(source->GetName(), MAX_NAME_SIZE);
	else
		writeStringFixedASCII("System", MAX_NAME_SIZE);

	writeStringUNICODE(reinterpret_cast<const WCHAR *>(pszText));

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xAF : PacketDeath				notifies about character death (NORMAL)
 *
 *
 ***************************************************************************/
PacketDeath::PacketDeath(CChar *dead, CItemCorpse *corpse, bool bFrontFall) : PacketSend(PACKET_CharDeath, 13, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDeath::PacketDeath");

	writeInt32(static_cast<DWORD>(dead->GetUID()));
	writeInt32(corpse ? static_cast<DWORD>(corpse->GetUID()) : 0);
	writeInt32(static_cast<DWORD>(bFrontFall));
}


/***************************************************************************
 *
 *
 *	Packet 0xB0 : PacketGumpDialog			displays a dialog gump (LOW)
 *	Packet 0xDD : PacketGumpDialog			displays a dialog gump using compression (LOW)
 *
 *
 ***************************************************************************/
PacketGumpDialog::PacketGumpDialog(DWORD x, DWORD y, CObjBase *object, DWORD context) : PacketSend(PACKET_GumpDialog, 24, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
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

	const NetState* net = target->m_NetState;
	if (net->isClientVersion(MINCLIVER_ML) || net->isClientKR() || net->isClientEnhanced())
		writeCompressedControls(controls, controlCount, texts, textCount);
	else
		writeStandardControls(controls, controlCount, texts, textCount);
}

void PacketGumpDialog::writeCompressedControls(const CGString* controls, size_t controlCount, const CGString* texts, size_t textCount)
{
	ADDTOCALLSTACK("PacketGumpDialog::writeCompressedControls");

	seek(0);
	writeByte(PACKET_CompressedGumpDialog);

	seek(19);

	{
		// compress and write controls
		z_uLong controlLength = 1;
		for (size_t i = 0; i < controlCount; i++)
			controlLength += controls[i].GetLength() + 2;

		char* toCompress = new char[controlLength];

		z_uLong controlLengthActual = 0;
		for (size_t i = 0; i < controlCount; i++)
			controlLengthActual += sprintf(&toCompress[controlLengthActual], "{%s}", static_cast<LPCTSTR>(controls[i]));
		controlLengthActual++;

		ASSERT(controlLengthActual == controlLength);

		z_uLongf compressLength = z_compressBound(controlLengthActual);
		z_Bytef *compressBuffer = new z_Bytef[compressLength];

		int error = z_compress2(compressBuffer, &compressLength, reinterpret_cast<const z_Bytef *>(toCompress), controlLengthActual, Z_DEFAULT_COMPRESSION);
		delete[] toCompress;

		if ((error != Z_OK) || (compressLength <= 0))
		{
			delete[] compressBuffer;
			g_Log.EventError("Compress failed with error %d when generating gump. Using old packet\n", error);
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
			writeInt16(static_cast<WORD>(texts[i].GetLength()));
			writeStringFixedNUNICODE(static_cast<LPCTSTR>(texts[i]), texts[i].GetLength());
		}

		z_uLong textsLength = getPosition() - textsPosition;
		
		z_uLongf compressLength = z_compressBound(textsLength);
		z_Bytef *compressBuffer = new z_Bytef[compressLength];

		int error = z_compress2(compressBuffer, &compressLength, &m_buffer[textsPosition], textsLength, Z_DEFAULT_COMPRESSION);
		if ((error != Z_OK) || (compressLength <= 0))
		{
			delete[] compressBuffer;
			g_Log.EventError("Compress failed with error %d when generating gump. Using old packet\n", error);
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
	writeByte(PACKET_GumpDialog);

	seek(19);

	// skip controls length until they're written
	size_t controlLengthPosition(getPosition());
	skip(2);

	// write controls
	for (size_t i = 0; i < controlCount; i++)
	{
		writeCharASCII('{');
		writeStringASCII(static_cast<LPCTSTR>(controls[i]), false);
		writeCharASCII('}');
	}

	// write controls length
	size_t endPosition(getPosition());
	seek(controlLengthPosition);
	writeInt16(static_cast<WORD>(endPosition - controlLengthPosition - 2));
	seek(endPosition);

	// write texts
	writeInt16(static_cast<WORD>(textCount));
	for (size_t i = 0; i < textCount; i++)
	{
		writeInt16(static_cast<WORD>(texts[i].GetLength()));
		writeStringFixedNUNICODE(static_cast<LPCTSTR>(texts[i]), texts[i].GetLength());
	}
}


/***************************************************************************
 *
 *
 *	Packet 0xB2 : PacketChatMessage			send a chat system message (LOW)
 *
 *
 ***************************************************************************/
PacketChatMessage::PacketChatMessage(const CClient* target, CHATMSG_TYPE type, LPCTSTR param1, LPCTSTR param2, CLanguageID language) : PacketSend(PACKET_ChatReq, 11, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketChatMessage::PacketChatMessage");

	initLength();
	writeInt16(static_cast<WORD>(type));
	writeStringFixedASCII(language.GetStr(), sizeof(DWORD));

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
PacketTooltip::PacketTooltip(const CClient* target, const CObjBase* object, LPCTSTR text) : PacketSend(PACKET_ToolTip, 8, g_Cfg.m_fUsePacketPriorities? PRI_IDLE : PRI_NORMAL)
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
PacketProfile::PacketProfile(const CClient* target, const CChar* character) : PacketSend(PACKET_CharProfile, 12, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketProfile::PacketProfile");

	initLength();
	writeInt32(character->GetUID());

	TCHAR *name = Str_GetTemp();
	LPCTSTR title = character->GetTradeTitle();
	if ( title[0] )
		sprintf(name, "%s, %s", character->Noto_GetTitle(), title);
	else
		sprintf(name, "%s", character->Noto_GetTitle());
	writeStringASCII(name);

	if ( character == target->GetChar() )
	{
		const CAccount *account = target->m_pAccount;
		ASSERT(account);
		TCHAR *age = Str_GetTemp();
		sprintf(age, g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_AGE), (CGTime::GetCurrentTime().GetDaysTotal() - account->m_dateFirstConnect.GetDaysTotal()) / 30);
		writeStringNUNICODE(age);
	}
	else
		writeCharNUNICODE('\0');

	if ( character->m_pPlayer )
		writeStringNUNICODE(static_cast<LPCTSTR>(character->m_pPlayer->m_sProfile));
	else
		writeCharNUNICODE('\0');

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xB9 : PacketEnableFeatures		enable client features (NORMAL)
 *
 *
 ***************************************************************************/
PacketEnableFeatures::PacketEnableFeatures(const CClient* target, DWORD flags) : PacketSend(PACKET_Features, 5, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketEnableFeatures::PacketEnableFeatures");

	// Since client 6.0.14.2, feature flags are 4 bytes instead of 2
	if ( target->m_NetState->isClientVersion(MINCLIVER_EXTRAFEATURES) )
		writeInt32(flags);
	else
		writeInt16(static_cast<WORD>(flags));

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
PacketArrowQuest::PacketArrowQuest(const CClient* target, WORD x, WORD y, DWORD id) : PacketSend(PACKET_Arrow, 10, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketArrowQuest::PacketArrowQuest");

	writeBool(x && y);
	writeInt16(x);
	writeInt16(y);

	if (target->m_NetState->isClientVersion(MINCLIVER_HS) || target->m_NetState->isClientEnhanced())
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
PacketSeason::PacketSeason(const CClient* target, SEASON_TYPE season, bool playMusic) : PacketSend(PACKET_Season, 3, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketSeason::PacketSeason");

	writeByte(static_cast<BYTE>(season));
	writeBool(playMusic);

	push(target);
}


/***************************************************************************
*
*
*	Packet 0xBD : PacketClientVersionReq	request client version (HIGH)
*
*
***************************************************************************/
PacketClientVersionReq::PacketClientVersionReq(const CClient* target) : PacketSend(PACKET_ClientVersion, 3, PRI_HIGH)
{
	ADDTOCALLSTACK("PacketClientVersionReq::PacketClientVersionReq");

	initLength();
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF : PacketExtended			extended command
 *
 *
 ***************************************************************************/
PacketExtended::PacketExtended(PACKETEXT_TYPE type, size_t len, Priority priority) : PacketSend(PACKET_ExtendedData, len, priority)
{
	ADDTOCALLSTACK("PacketExtended::PacketExtended");

	initLength();

	writeInt16(static_cast<WORD>(type));
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x04 : PacketGumpChange		change gump (LOW)
 *
 *
 ***************************************************************************/
PacketGumpChange::PacketGumpChange(const CClient *target, DWORD context, DWORD buttonId) : PacketExtended(PACKETEXT_CloseGump, 13, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
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
PacketParty::PacketParty(PARTYMSG_TYPE action, size_t len, Priority priority) : PacketExtended(PACKETEXT_Party_Msg, len, priority)
{
	ADDTOCALLSTACK("PacketParty::PacketParty");

	writeByte(static_cast<BYTE>(action));
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

	writeByte(static_cast<BYTE>(iQty));

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

	ASSERT(member);

	size_t iQty = !members ? 0 : members->GetCharCount();

	writeByte(static_cast<BYTE>(iQty));
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
PacketPartyChat::PacketPartyChat(const CChar* source, const NCHAR* text) : PacketParty(PARTYMSG_MsgAll, 11, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPartyChat::PacketPartyChat");

	writeInt32(source->GetUID());
	writeStringUNICODE(reinterpret_cast<const WCHAR *>(text));
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x06.0x07 : PacketPartyInvite	send party invitation (NORMAL)
 *
 *
 ***************************************************************************/
PacketPartyInvite::PacketPartyInvite(const CClient* target, const CChar* inviter) : PacketParty(PARTYMSG_Invite, 10, PRI_NORMAL)
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
PacketMapChange::PacketMapChange(const CClient *target, BYTE map) : PacketExtended(PACKETEXT_Map_Change, 6, PRI_NORMAL)
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
PacketPropertyListVersionOld::PacketPropertyListVersionOld(const CClient* target, const CObjBase* object, DWORD version) : PacketExtended(PACKETEXT_OldAOSTooltipInfo, 13, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPropertyListVersionOld::PacketPropertyListVersionOld");

	m_object = object->GetUID();

	writeInt32(object->GetUID());
	writeInt32(version);

	push(target, false);
}

bool PacketPropertyListVersionOld::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketPropertyListVersionOld::onSend");
	if (g_NetworkManager.isOutputThreaded())
		return true;

	const CChar* character = client->GetChar();
	if (!character)
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
PacketDisplayPopup::PacketDisplayPopup(const CClient* target, CGrayUID uid) : PacketExtended(PACKETEXT_Popup_Display, 12, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDisplayPopup::PacketDisplayPopup");

	m_popupCount = 0;
	m_newPacketFormat = target->m_NetState->isClientKR() || target->m_NetState->isClientEnhanced() || target->m_NetState->isClientVersion(MINCLIVER_NEWCONTEXTMENU);

	if (m_newPacketFormat)
		writeInt16(2);
	else
		writeInt16(1);

	writeInt32(uid);

	writeByte(0); // popup count
}

void PacketDisplayPopup::addOption(WORD entryTag, DWORD textId, WORD flags, WORD color)
{
	ADDTOCALLSTACK("PacketDisplayPopup::addOption");

	if (m_popupCount >= BYTE_MAX)
	{
		DEBUG_ERR(("Bad AddContextEntry usage: Too many entries, max = %d\n", BYTE_MAX));
		return;
	}

	if (m_newPacketFormat)
	{
		if (textId <= WORD_MAX)
			textId += 3000000;
		if (flags & POPUPFLAG_COLOR)
			flags &= ~POPUPFLAG_COLOR;

		writeInt32(textId);
		writeInt16(entryTag);
		writeInt16(flags);
	}
	else
	{
		if (flags & POPUPFLAG_HIGHLIGHT)
			flags &= ~POPUPFLAG_HIGHLIGHT;

		writeInt16(entryTag);
		writeInt16(static_cast<WORD>(textId));
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
	writeByte(static_cast<BYTE>(m_popupCount));

	seek(endPosition);
}

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x16 : PacketCloseUIWindow		Close User Interface Windows (NORMAL)
 *
 *
 ***************************************************************************/
PacketCloseUIWindow::PacketCloseUIWindow(const CClient *target, WindowType windowtype, const CObjBase *object) : PacketExtended(PACKETEXT_CloseUIWindow, 13, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketCloseUIWindow::PacketCloseUIWindow");

	writeInt32(static_cast<DWORD>(windowtype));
	writeInt32(object->GetUID());

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x17 : PacketCodexOfWisdom		open Codex of Wisdom (LOW)
 *
 *
 ***************************************************************************/
PacketCodexOfWisdom::PacketCodexOfWisdom(const CClient *target, DWORD dwTopicID, bool fForceOpen) : PacketExtended(PACKETEXT_CodexOfWisdom, 11, g_Cfg.m_fUsePacketPriorities ? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketCodexOfWisdom::PacketCodexOfWisdom");

	writeByte(1);
	writeInt32(dwTopicID);
	writeBool(fForceOpen);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x18 : PacketEnableMapDiffs		enable use of map diff files (NORMAL)
 *
 *
 ***************************************************************************/
PacketEnableMapDiffs::PacketEnableMapDiffs(const CClient* target) : PacketExtended(PACKETEXT_EnableMapDiffs, 13, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketEnableMapDiffs::PacketEnableMapDiffs");

	int map = 0;
	int mapCount = 1;

	// Find map count
	for (map = MAP_QTY - 1; map >= 0; --map)
	{
		if (!g_MapList.m_maps[map])
			continue;

		mapCount = map;
		break;
	}

	writeInt32(static_cast<DWORD>(mapCount));

	for (map = 0; map < mapCount; ++map)
	{
		if (g_MapList.m_maps[map] && g_Cfg.m_fUseMapDiffs)
		{
			writeInt32(g_Install.m_Mapdifl[map].IsFileOpen() ? g_Install.m_Mapdifl[map].GetLength() / 4 : 0);
			writeInt32(g_Install.m_Stadifl[map].IsFileOpen() ? g_Install.m_Stadifl[map].GetLength() / 4 : 0);
		}
		else
		{
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
*	Packet 0xBF.0x19.0x05 : PacketStatLocks		update lock status of stats (NORMAL) (KR/SA)
*
*
***************************************************************************/
PacketStatLocks::PacketStatLocks(const CClient* target, const CChar* character) : PacketExtended(PACKETEXT_ExtendedStats, 12, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketStatLocks::PacketStatLocks");

	BYTE type = (target->m_NetState->isClientKR() || target->m_NetState->isClientEnhanced()) ? 5 : 2;
	BYTE flags = 0;
	if (character->m_pPlayer)
	{
		flags |= static_cast<BYTE>(character->m_pPlayer->Stat_GetLock(STAT_INT));
		flags |= static_cast<BYTE>(character->m_pPlayer->Stat_GetLock(STAT_DEX)) << 2;
		flags |= static_cast<BYTE>(character->m_pPlayer->Stat_GetLock(STAT_STR)) << 4;
	}

	writeByte(type);
	writeInt32(character->GetUID());
	writeByte(0);
	writeByte(flags);

	if ( type == 5 )
	{
		writeByte(0);
		writeInt32(0);
	}

	push(target);
}


/***************************************************************************
*
*
*	Packet 0xBF.0x19 : BondedStatus				set bonded status (NORMAL)
*
*
***************************************************************************/

PacketBondedStatus::PacketBondedStatus(const CClient * target, const CChar * pChar, bool IsGhost) : PacketExtended(PACKETEXT_ExtendedStats, 11, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketBondedStatus::PacketBondedStatus");

	writeByte(0);
	writeInt32(pChar->GetUID());
	writeBool(IsGhost);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x1B : PacketSpellbookContent	spellbook content (NORMAL)
 *
 *
 ***************************************************************************/
PacketSpellbookContent::PacketSpellbookContent(const CClient* target, const CItem* spellbook, WORD offset) : PacketExtended(PACKETEXT_SpellbookContent, 23, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketSpellbookContent::PacketSpellbookContent");

	writeInt16(1);
	writeInt32(spellbook->GetUID());
	writeInt16(static_cast<WORD>(spellbook->GetDispID()));
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
PacketHouseDesignVersion::PacketHouseDesignVersion(const CClient *target, const CItemMultiCustom *pHouse) : PacketExtended(PACKETEXT_HouseDesignVer, 13, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketHouseDesignVersion::PacketHouseDesignVersion");

	writeInt32(pHouse->GetUID());
	writeInt32(pHouse->GetRevision(target));

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x20.0x04 : PacketHouseBeginCustomize	begin house customization (NORMAL)
 *
 *
 ***************************************************************************/
PacketHouseBeginCustomize::PacketHouseBeginCustomize(const CClient *target, const CItemMultiCustom *pHouse) : PacketExtended(PACKETEXT_HouseCustomize, 17, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketHouseBeginCustomize::PacketHouseBeginCustomize");

	writeInt32(pHouse->GetUID());
	writeByte(0x4);
	writeInt16(0);
	writeInt16(0xFFFF);
	writeInt16(0xFFFF);
	writeByte(0xFF);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x20.0x05 : PacketHouseEndCustomize	end house customization (NORMAL)
 *
 *
 ***************************************************************************/
PacketHouseEndCustomize::PacketHouseEndCustomize(const CClient *target, const CItemMultiCustom *pHouse) : PacketExtended(PACKETEXT_HouseCustomize, 17, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketHouseEndCustomize::PacketHouseEndCustomize");

	writeInt32(pHouse->GetUID());
	writeByte(0x5);
	writeInt16(0);
	writeInt16(0xFFFF);
	writeInt16(0xFFFF);
	writeByte(0xFF);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x21 : PacketSpecialMoveClear		clear special move selected (IDLE)
 *
 *
 ***************************************************************************/

PacketSpecialMoveClear::PacketSpecialMoveClear(const CClient *target) : PacketExtended(PACKETEXT_SpecialMoveClear, 0, PRI_IDLE)
{
	ADDTOCALLSTACK("PacketSpecialMoveClear::PacketSpecialMoveClear");

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x22 : PacketCombatDamageOld		[old] sends notification of got damage (NORMAL)
 *
 *
 ***************************************************************************/
PacketCombatDamageOld::PacketCombatDamageOld(const CClient* target, BYTE damage, CGrayUID uid) : PacketExtended(PACKETEXT_CombatDamage, 11, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketCombatDamageOld::PacketCombatDamageOld");

	writeByte(0x1);
	writeInt32(static_cast<DWORD>(uid));
	writeByte(damage);

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x26 : PacketSpeedMode		set movement speed (HIGH)
 *
 *
 ***************************************************************************/
PacketSpeedMode::PacketSpeedMode(const CClient* target, BYTE mode) : PacketExtended(PACKETEXT_SpeedMode, 6, g_Cfg.m_fUsePacketPriorities? PRI_HIGH : PRI_NORMAL)
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
PacketMessageLocalised::PacketMessageLocalised(const CClient* target, DWORD cliloc, const CObjBaseTemplate* source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font, LPCTSTR args) : PacketSend(PACKET_SpeakLocalized, 50, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketMessageLocalised::PacketMessageLocalised");

	initLength();

	if (!source)
		writeInt32(0xFFFFFFFF);
	else
		writeInt32(source->GetUID());

	if (!source || !source->IsChar())
		writeInt16(0xFFFF);
	else
	{
		const CChar* sourceCharacter = dynamic_cast<const CChar *>(source);
		if ( !sourceCharacter )
			return;
		writeInt16(static_cast<WORD>(sourceCharacter->GetDispID()));
	}

	writeByte(static_cast<BYTE>(mode));
	writeInt16(static_cast<WORD>(hue));
	writeInt16(static_cast<WORD>(font));
	writeInt32(cliloc);

	if (source)
		writeStringFixedASCII(source->GetName(), MAX_NAME_SIZE);
	else
		writeStringFixedASCII("System", MAX_NAME_SIZE);

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
PacketVisualRange::PacketVisualRange(const CClient* target, BYTE range) : PacketSend(PACKET_ViewRange, 2, PRI_NORMAL)
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
PacketMessageLocalisedEx::PacketMessageLocalisedEx(const CClient* target, DWORD cliloc, const CObjBaseTemplate* source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font, AFFIX_TYPE affixType, LPCTSTR affix, LPCTSTR args) : PacketSend(PACKET_SpeakLocalizedEx, 52, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketMessageLocalisedEx::PacketMessageLocalisedEx");

	initLength();

	if (!source)
		writeInt32(0xFFFFFFFF);
	else
		writeInt32(source->GetUID());

	if (!source || !source->IsChar())
		writeInt16(0xFFFF);
	else
	{
		const CChar* sourceCharacter = dynamic_cast<const CChar *>(source);
		if ( !sourceCharacter )
			return;
		writeInt16(static_cast<WORD>(sourceCharacter->GetDispID()));
	}

	writeByte(static_cast<BYTE>(mode));
	writeInt16(static_cast<WORD>(hue));
	writeInt16(static_cast<WORD>(font));
	writeInt32(cliloc);
	writeByte(static_cast<BYTE>(affixType));

	if (source)
		writeStringFixedASCII(source->GetName(), MAX_NAME_SIZE);
	else
		writeStringFixedASCII("System", MAX_NAME_SIZE);

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
PacketLogoutAck::PacketLogoutAck(const CClient* target) : PacketSend(PACKET_LogoutStatus, 2, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketLogoutAck::PacketLogoutAck");

	writeByte(1);
	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xD4 : PacketDisplayBookNew		display book (LOW)
 *
 *
 ***************************************************************************/
PacketDisplayBookNew::PacketDisplayBookNew(const CClient* target, CItem* book) : PacketSend(PACKET_AOSBookPage, 17, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDisplayBookNew::PacketDisplayBookNew");

	if ( !book )
		return;

	bool isWritable = false;
	WORD pages = 0;
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
						pages = static_cast<WORD>(s.GetArgVal());
						break;
					case CIC_TITLE:
						title = s.GetArgStr();
						break;
				}
			}
		}

		// make sure book is named
		if (!title.IsEmpty())
			book->SetName(static_cast<LPCTSTR>(title));
	}
	else
	{
		// user written book
		const CItemMessage* message = dynamic_cast<const CItemMessage *>(book);
		if (message)
		{
			isWritable = message->IsBookWritable();
			pages = isWritable? MAX_BOOK_PAGES : message->GetPageCount();
			title = message->GetName();
			author = message->m_sAuthor.IsEmpty()? g_Cfg.GetDefaultMsg(DEFMSG_BOOK_AUTHOR_UNKNOWN) : static_cast<LPCTSTR>(message->m_sAuthor);
		}
	}


	initLength();
	writeInt32(book->GetUID());
	writeBool(isWritable);
	writeBool(isWritable);
	writeInt16(pages);
	writeInt16(static_cast<WORD>(title.GetLength() + 1));
	writeStringFixedASCII(title.GetPtr(), title.GetLength() + 1);
	writeInt16(static_cast<WORD>(author.GetLength() + 1));
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
PacketPropertyList::PacketPropertyList(const CObjBase* object, DWORD version, const CGObArray<CClientTooltip *> *data) : PacketSend(PACKET_AOSTooltip, 48, PRI_IDLE)
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
		writeInt16(static_cast<WORD>(tipLength * sizeof(WCHAR)));
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
	if (g_NetworkManager.isOutputThreaded())
		return true;

	const CChar* character = client->GetChar();
	if (!character)
		return false;

	if (hasExpired(g_Cfg.m_iTooltipCache))
		return false;

	return true;
}

bool PacketPropertyList::hasExpired(int timeout) const
{
	ADDTOCALLSTACK("PacketPropertyList::hasExpired");
	return (g_World.GetCurrentTime().GetTimeRaw() > m_time + static_cast<UINT64>(timeout));
}


/***************************************************************************
 *
 *
 *	Packet 0xD8 : PacketHouseDesign			house design (IDLE)
 *
 *
 ***************************************************************************/
PacketHouseDesign::PacketHouseDesign(const CItemMultiCustom *pHouse, DWORD dwRevision) : PacketSend(PACKET_AOSCustomHouse, 64, g_Cfg.m_fUsePacketPriorities ? PRI_IDLE : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketHouseDesign::PacketHouseDesign");

	m_pStairBuffer = new StairData[HOUSEDESIGN_STAIRS_PER_BLOCK];
	memset(m_pStairBuffer, 0, HOUSEDESIGN_STAIRDATA_BUFFER);
	m_wStairCount = 0;
	m_wItemCount = 0;
	m_wDataSize = 1;
	m_bLevelCount = 0;
	m_bStairLevelCount = 0;
	m_pHouse = pHouse;

	initLength();

	writeByte(0x3);
	writeByte(0);
	writeInt32(pHouse->GetUID());
	writeInt32(dwRevision);
	writeInt16(0);	// item count
	writeInt16(0);	// data size
	writeByte(0);	// level count
}

PacketHouseDesign::PacketHouseDesign(const PacketHouseDesign *pPacket) : PacketSend(pPacket)
{
	ADDTOCALLSTACK("PacketHouseDesign::PacketHouseDesign(2)");

	m_pStairBuffer = new StairData[HOUSEDESIGN_STAIRS_PER_BLOCK];
	memcpy(m_pStairBuffer, pPacket->m_pStairBuffer, HOUSEDESIGN_STAIRDATA_BUFFER);
	m_wStairCount = pPacket->m_wStairCount;
	m_wItemCount = pPacket->m_wItemCount;
	m_wDataSize = pPacket->m_wDataSize;
	m_bLevelCount = pPacket->m_bLevelCount;
	m_bStairLevelCount = pPacket->m_bStairLevelCount;
	m_pHouse = pPacket->m_pHouse;
}

PacketHouseDesign::~PacketHouseDesign(void)
{
	if ( m_pStairBuffer )
	{
		delete[] m_pStairBuffer;
		m_pStairBuffer = NULL;
	}
}

bool PacketHouseDesign::writeLevelData(BYTE bLevel, WORD wItemCount, BYTE *pbData, DWORD dwDataSize)
{
	ADDTOCALLSTACK("PacketHouseDesign::writeLevelData");

	// Compress data
	z_uLongf compressLength = z_compressBound(dwDataSize);
	z_Bytef *compressBuffer = new z_Bytef[compressLength];

	int error = z_compress2(compressBuffer, &compressLength, pbData, dwDataSize, Z_DEFAULT_COMPRESSION);
	if ( error != Z_OK )
	{
		// An error occured with this floor, but we should be able to continue to the next without problems
		delete[] compressBuffer;
		g_Log.EventError("Compress failed with error %d when generating house design for floor %hhu on building UID=0%" FMTDWORDH "\n", error, bLevel, static_cast<DWORD>(m_pHouse->GetUID()));
		return false;
	}
	else if ( (compressLength <= 0) || (compressLength >= HOUSEDESIGN_LEVELDATA_BUFFER) )
	{
		// Too much data, but we should be able to continue to the next floor without problems
		delete[] compressBuffer;
		g_Log.EventWarn("Floor %hhu on building UID=0%" FMTDWORDH " too large with compressed length of %lu\n", bLevel, static_cast<DWORD>(m_pHouse->GetUID()), compressLength);
		return false;
	}

	writeByte(bLevel|0x20);
	writeByte(static_cast<BYTE>(dwDataSize));
	writeByte(static_cast<BYTE>(compressLength));
	writeByte(static_cast<BYTE>(((dwDataSize >> 4) & 0xF0) | ((compressLength >> 8) & 0xF)));
	writeData(compressBuffer, compressLength);
	delete[] compressBuffer;

	m_wItemCount += wItemCount;
	m_wDataSize += static_cast<WORD>(4 + compressLength);
	++m_bLevelCount;
	return true;
}

bool PacketHouseDesign::writeStairData(ITEMID_TYPE id, BYTE x, BYTE y, BYTE z)
{
	ADDTOCALLSTACK("PacketHouseDesign::writeStairData");

	m_pStairBuffer[m_wStairCount].m_id = static_cast<WORD>(id);
	m_pStairBuffer[m_wStairCount].m_x = x;
	m_pStairBuffer[m_wStairCount].m_y = y;
	m_pStairBuffer[m_wStairCount].m_z = z;

	if ( m_wStairCount++ >= HOUSEDESIGN_STAIRS_PER_BLOCK )
		flushStairData();

	return true;
}

void PacketHouseDesign::flushStairData(void)
{
	ADDTOCALLSTACK("PacketHouseDesign::flushStairData");

	if ( m_wStairCount <= 0 )
		return;

	WORD wStairCount = m_wStairCount;
	DWORD dwStairSize = wStairCount * sizeof(StairData);
	m_wStairCount = 0;

	// Compress data
	z_uLongf compressLength = z_compressBound(dwStairSize);
	z_Bytef *compressBuffer = new z_Bytef[compressLength];

	int error = z_compress2(compressBuffer, &compressLength, reinterpret_cast<const z_Bytef *>(m_pStairBuffer), dwStairSize, Z_DEFAULT_COMPRESSION);
	if ( error != Z_OK )
	{
		// An error occured with this block, but we should be able to continue to the next without problems
		delete[] compressBuffer;
		g_Log.EventError("Compress failed with error %d when generating house design on building UID=0%" FMTDWORDH "\n", error, static_cast<DWORD>(m_pHouse->GetUID()));
		return;
	}
	else if ( (compressLength <= 0) || (compressLength >= HOUSEDESIGN_STAIRDATA_BUFFER) )
	{
		// Too much data, but we should be able to continue to the next block without problems
		delete[] compressBuffer;
		g_Log.EventWarn("Building UID=0%" FMTDWORDH " too large with compressed length of %lu\n", static_cast<DWORD>(m_pHouse->GetUID()), compressLength);
		return;
	}

	writeByte(m_bStairLevelCount + 9);
	writeByte(static_cast<BYTE>(dwStairSize));
	writeByte(static_cast<BYTE>(compressLength));
	writeByte(static_cast<BYTE>(((dwStairSize >> 4) & 0xF0) | ((compressLength >> 8) & 0xF)));
	writeData(compressBuffer, compressLength);
	delete[] compressBuffer;

	m_wItemCount += wStairCount;
	m_wDataSize += static_cast<WORD>(4 + compressLength);
	++m_bStairLevelCount;
	return;
}

void PacketHouseDesign::finalize(void)
{
	ADDTOCALLSTACK("PacketHouseDesign::finalize");

	flushStairData();

	size_t endPosition(getPosition());

	seek(13);
	writeInt16(m_wItemCount);
	writeInt16(m_wDataSize);
	writeByte(m_bLevelCount + m_bStairLevelCount);

	seek(endPosition);
}


/***************************************************************************
 *
 *
 *	Packet 0xDC : PacketPropertyListVersion		property (tool tip) version (LOW)
 *
 *
 ***************************************************************************/
PacketPropertyListVersion::PacketPropertyListVersion(const CClient* target, const CObjBase* object, DWORD version) : PacketSend(PACKET_AOSTooltipInfo, 9, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketPropertyListVersion::PacketPropertyListVersion");

	m_object = object->GetUID();

	writeInt32(object->GetUID());
	writeInt32(version);

	push(target);
}

bool PacketPropertyListVersion::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketPropertyList::onSend");
	if (g_NetworkManager.isOutputThreaded())
		return true;

	const CChar* character = client->GetChar();
	if (!character)
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
PacketBuff::PacketBuff(const CClient* target, const BUFF_ICONS iconId, const DWORD clilocOne, const DWORD clilocTwo, const WORD time, LPCTSTR* args, size_t argCount) : PacketSend(PACKET_BuffPacket, 72, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketBuff::PacketBuff");
	// At date of 04/2015 RUOSI seems to have a different structure than the one we have with one more argument and different order... however this one seems to keep working: http://ruosi.org/packetguide/index.xml#serverDF

	const CChar* character = target->GetChar();
	ASSERT(character);

	initLength();

	writeInt32(character->GetUID());

	writeInt16(static_cast<WORD>(iconId));
	writeInt16(0x1);	// show
	writeInt32(0);

	writeInt16(static_cast<WORD>(iconId));
	writeInt16(0x1);	// show
	writeInt32(0);

	writeInt16(time);	// simple countdown without automatic remove
	writeInt16(0);
	writeByte(0);

	writeInt32(clilocOne);
	writeInt32(clilocTwo);

	if ( argCount )
	{
		writeInt32(0);
		writeInt16(0x1);		// show icon
		writeInt16(0);

		for (size_t i = 0; i < argCount; i++)
		{
			writeCharUNICODE('\t');
			writeStringUNICODE(args[i], false);
		}
		writeCharUNICODE('\t');
		writeCharUNICODE('\0');

		writeInt16(0x1);
		writeInt16(0);
	}
	else
	{
		// Original code - it leaves empty clilocTwo exactly as it is
		//writeInt32(0);
		//writeInt32(0);
		//writeInt16(0);

		// Workaround - it fills empty clilocTwo with an whitespace just to make it show ' ' instead '~1_SOMETHING~'
		// This is a Sphere custom behavior, since it uses ~1_NOTHING~ clilocs which are not really used on OSI
		writeInt32(0);
		writeInt16(0x1);
		writeInt16(0);

		writeStringUNICODE("\t ", true);
	}
	push(target);
}

PacketBuff::PacketBuff(const CClient* target, const BUFF_ICONS iconId) : PacketSend(PACKET_BuffPacket, 15, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketBuff::PacketBuff(2)");

	const CChar* character = target->GetChar();
	ASSERT(character);

	initLength();

	writeInt32(character->GetUID());
	writeInt16(static_cast<WORD>(iconId));
	writeInt16(0);		// hide icon

	push(target);
}

/***************************************************************************
 *
 *
 *	Packet 0xE3 : PacketKREncryption		Sends encryption data to KR client (HIGH)
 *
 *
 ***************************************************************************/
PacketKREncryption::PacketKREncryption(const CClient* target) : PacketSend(PACKET_EncryptionReq, 77, g_Cfg.m_fUsePacketPriorities? PRI_HIGH : PRI_NORMAL)
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
*	Packet 0xE5 : PacketWaypointAdd			Add waypoint on KR/SA radar map
*
*
***************************************************************************/
PacketWaypointAdd::PacketWaypointAdd(const CClient *target, CObjBase *object, MAPWAYPOINT_TYPE type) : PacketSend(PACKET_WaypointShow, 25, g_Cfg.m_fUsePacketPriorities ? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketWaypointAdd::PacketWaypointAdd");

	if ( !object )
		return;

	CPointMap pt = object->GetTopPoint();
	DWORD cliloc = (type == MAPWAYPOINT_Corpse) ? 1028198 : 1062613;	// corpse : "~1_NAME~"

	initLength();
	writeInt32(object->GetUID());

	writeInt16(pt.m_x);
	writeInt16(pt.m_y);
	writeByte(pt.m_z);
	writeByte(pt.m_map);

	writeInt16(static_cast<WORD>(type));
	writeInt16(0);

	writeInt32(cliloc);
	writeStringUNICODE(object->GetName());

	push(target);
}

/***************************************************************************
*
*
*	Packet 0xE6 : PacketWaypointRemove		Remove waypoint on KR/SA radar map
*
*
***************************************************************************/
PacketWaypointRemove::PacketWaypointRemove(const CClient *target, CObjBase *object) : PacketSend(PACKET_WaypointHide, 5, g_Cfg.m_fUsePacketPriorities ? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketWaypointRemove::PacketWaypointRemove");

	if ( !object )
		return;

	writeInt32(object->GetUID());

	push(target);
}

/***************************************************************************
 *
 *
 *	Packet 0xEA : PacketToggleHotbar		toggle kr hotbar (NORMAL)
 *
 *
 ***************************************************************************/
PacketToggleHotbar::PacketToggleHotbar(const CClient* target, bool enable) : PacketSend(PACKET_ToggleHotbar, 3, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketToggleHotbar::PacketToggleHotbar");

	writeInt16(enable? 0x01 : 0x00);

	push(target);
}

/***************************************************************************
 *
 *
 *	Packet 0xF2 : PacketTimeSyncResponse	time sync response (HIGH)
 *
 *
 ***************************************************************************/
PacketTimeSyncResponse::PacketTimeSyncResponse(const CClient* target) : PacketSend(PACKET_TimeSyncResponse, 25, PRI_HIGH)
{
	ADDTOCALLSTACK("PacketTimeSyncResponse::PacketTimeSyncResponse");

	INT64 llTime = g_World.GetCurrentTime().GetTimeRaw();
	writeInt64(llTime);
	writeInt64(llTime+100);
	writeInt64(llTime+100);	//No idea if different values make a difference. I didn't notice anything different when all values were the same.

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xF3 : PacketWorldObj			sends object on ground (NORMAL)
 *
 *
 ***************************************************************************/
PacketWorldObj::PacketWorldObj(BYTE id, size_t size, CGrayUID uid) : PacketWorldItem(id, size, uid)
{
}

PacketWorldObj::PacketWorldObj(const CClient* target, CItem *item) : PacketWorldItem(PACKET_WorldObj, 26, item->GetUID())
{
	ADDTOCALLSTACK("PacketWorldObj::PacketWorldObj");

	DataSource source;
	ITEMID_TYPE id = item->GetDispID();
	HUE_TYPE hue = item->GetHue();
	CPointMap pt = item->GetTopPoint();
	WORD amount = item->GetAmount();
	BYTE layer = 0;
	BYTE flags = 0;

	adjustItemData(target, item, id, hue, amount, layer, flags);

	if ( id >= ITEMID_MULTI )
	{
		source = Multi;
		id = static_cast<ITEMID_TYPE>(id & 0x3FFF);
	}
	else
	{
		source = (item->Can(CAN_I_DAMAGEABLE) && (target->m_NetState->isClientVersion(MINCLIVER_STATUS_V6))) ? Damageable : TileData;
		id = static_cast<ITEMID_TYPE>(id & 0xFFFF);		// SA: 0x7FFF, HS: 0xFFFF
	}

	writeInt16(1);
	writeByte(static_cast<BYTE>(source));
	writeInt32(static_cast<DWORD>(item->GetUID()));
	writeInt16(static_cast<WORD>(id));
	writeByte(0);
	writeInt16(amount);
	writeInt16(amount);
	writeInt16(static_cast<WORD>(pt.m_x & 0x7FFF));
	writeInt16(static_cast<WORD>(pt.m_y & 0x3FFF));
	writeByte(static_cast<BYTE>(pt.m_z));
	writeByte(layer);
	writeInt16(static_cast<WORD>(hue));
	writeByte(flags);

	if ( target->m_NetState->isClientVersion(MINCLIVER_HS) )
		writeInt16(0);		// 0 = World Item, 1 = Player Item (why should a item on the ground be defined as player item? and what is the difference?)

	trim();
	push(target);
}

PacketWorldObj::PacketWorldObj(const CClient* target, CChar* mobile) : PacketWorldItem(PACKET_WorldObj, 26, mobile->GetUID())
{
	DataSource source = Character;
	CREID_TYPE id = mobile->GetDispID();
	DIR_TYPE dir = mobile->m_dirFace;
	HUE_TYPE hue = mobile->GetHue();
	CPointMap pt = mobile->GetTopPoint();

	writeInt16(1);
	writeByte(static_cast<BYTE>(source));
	writeInt32(static_cast<DWORD>(mobile->GetUID()));
	writeInt16(static_cast<WORD>(id));
	writeByte(static_cast<BYTE>(dir));
	writeInt16(1);
	writeInt16(1);
	writeInt16(static_cast<WORD>(pt.m_x));
	writeInt16(static_cast<WORD>(pt.m_y));
	writeByte(static_cast<BYTE>(pt.m_z));
	writeByte(0);
	writeInt16(hue);
	writeByte(0);

	if ( target->m_NetState->isClientVersion(MINCLIVER_HS) )
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
PacketDisplayMapNew::PacketDisplayMapNew(const CClient* target, const CItemMap* map, const CRectMap& rect) : PacketSend(PACKET_MapDisplayNew, 21, g_Cfg.m_fUsePacketPriorities? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketDisplayMapNew::PacketDisplayMapNew");

	const CItemBase* itemDef = map->Item_GetDef();
	ASSERT(itemDef);

	writeInt32(map->GetUID());
	writeInt16(GUMP_MAP_2_NORTH);
	writeInt16(static_cast<WORD>(rect.m_left));
	writeInt16(static_cast<WORD>(rect.m_top));
	writeInt16(static_cast<WORD>(rect.m_right));
	writeInt16(static_cast<WORD>(rect.m_bottom));
	writeInt16(static_cast<WORD>(itemDef->m_ttMap.m_iGumpWidth > 0 ? itemDef->m_ttMap.m_iGumpWidth : CItemMap::DEFAULT_SIZE));
	writeInt16(static_cast<WORD>(itemDef->m_ttMap.m_iGumpHeight > 0 ? itemDef->m_ttMap.m_iGumpHeight : CItemMap::DEFAULT_SIZE));
	writeInt16(static_cast<WORD>(rect.m_map));

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xF6 : PacketMoveShip			move ship (NORMAL)
 *
 *
 ***************************************************************************/
PacketMoveShip::PacketMoveShip(const CClient *target, const CItemShip *pShip, CObjBase **ppComponentList, size_t iComponentCount, BYTE bDirMove, BYTE bDirFace, BYTE bSpeed) : PacketSend(PACKET_MoveShip, 18, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketMoveShip::PacketMoveShip");
	ASSERT(iComponentCount > 0);
	const CPointMap ptShip = pShip->GetTopPoint();

	initLength();

	writeInt32(pShip->GetUID());
	writeByte(bSpeed);
	writeByte(bDirMove);
	writeByte(bDirFace);
	writeInt16(ptShip.m_x);
	writeInt16(ptShip.m_y);
	writeInt16(ptShip.m_z);
	writeInt16(static_cast<WORD>(iComponentCount - 1));		// assume that first component is the ship itself

	for ( size_t i = 1; i < iComponentCount; ++i )
	{
		const CObjBase *pObj = ppComponentList[i];
		const CPointMap ptObj = pObj->GetTopPoint();

		writeInt32(pObj->GetUID());
		writeInt16(ptObj.m_x);
		writeInt16(ptObj.m_y);
		writeInt16(ptObj.m_z);
	}

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xF7 : PacketWorldObjCont		sends multiple objects on ground (NORMAL)
 *
 *
 ***************************************************************************/
PacketWorldObjCont::PacketWorldObjCont(const CClient* target, CObjBase** objects, size_t objectCount) : PacketWorldObj(PACKET_WorldObjCont, 5, PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketWorldObjCont::PacketWorldObjCont");
	ASSERT(objectCount > 0);

	initLength();
	writeInt16(static_cast<WORD>(objectCount));

	for (size_t i = 0; i < objectCount; i++)
	{
		CObjBase* object = objects[i];
		if (object->IsItem())
		{
			CItem *item = static_cast<CItem *>(object);
			DataSource source;
			ITEMID_TYPE id = item->GetDispID();
			HUE_TYPE hue = item->GetHue();
			CPointMap pt = item->GetTopPoint();
			WORD amount = item->GetAmount();
			BYTE layer = 0;
			BYTE flags = 0;

			adjustItemData(target, item, id, hue, amount, layer, flags);

			if ( id >= ITEMID_MULTI )
			{
				source = Multi;
				id = static_cast<ITEMID_TYPE>(id & 0x3FFF);
			}
			else
			{
				source = (item->Can(CAN_I_DAMAGEABLE) && (target->m_NetState->isClientVersion(MINCLIVER_STATUS_V6))) ? Damageable : TileData;
				id = static_cast<ITEMID_TYPE>(id & 0xFFFF);		// SA: 0x7FFF, HS: 0xFFFF
			}

			writeByte(0xF3);
			writeInt16(1);
			writeByte(static_cast<BYTE>(source));
			writeInt32(static_cast<DWORD>(item->GetUID()));
			writeInt16(static_cast<WORD>(id));
			writeByte(0);
			writeInt16(amount);
			writeInt16(amount);
			writeInt16(static_cast<WORD>(pt.m_x & 0x7FFF));
			writeInt16(static_cast<WORD>(pt.m_y & 0x3FFF));
			writeByte(static_cast<BYTE>(pt.m_z));
			writeByte(layer);
			writeInt16(static_cast<WORD>(hue));
			writeByte(flags);
			writeInt16(0);
		}
		else
		{
			CChar *mobile = static_cast<CChar *>(object);
			DataSource source = Character;
			CREID_TYPE id = mobile->GetDispID();
			HUE_TYPE hue = mobile->GetHue();
			CPointMap pt = mobile->GetTopPoint();
			DIR_TYPE dir = mobile->m_dirFace;

			writeByte(0xF3);
			writeInt16(1);
			writeByte(static_cast<BYTE>(source));
			writeInt32(static_cast<DWORD>(mobile->GetUID()));
			writeInt16(static_cast<WORD>(id));
			writeByte(static_cast<BYTE>(dir));
			writeInt16(1);
			writeInt16(1);
			writeInt16(static_cast<WORD>(pt.m_x));
			writeInt16(static_cast<WORD>(pt.m_y));
			writeByte(static_cast<BYTE>(pt.m_z));
			writeByte(0);
			writeInt16(static_cast<WORD>(hue));
			writeByte(0);
			writeInt16(0);
		}
	}

	push(target);
}


/***************************************************************************
 *
 *
 *	Packet 0xF9 : PacketGlobalChat			global chat (LOW) (INCOMPLETE)
 *
 *
 ***************************************************************************/
PacketGlobalChat::PacketGlobalChat(const CClient *target, BYTE unknown, BYTE action, BYTE stanza, LPCTSTR xml) : PacketSend(PACKET_GlobalChat, 4, g_Cfg.m_fUsePacketPriorities ? PRI_LOW : PRI_NORMAL)
{
	ADDTOCALLSTACK("PacketGlobalChat::PacketGlobalChat");

	TCHAR *xmlFull = Str_GetTemp();
	sprintf(xmlFull, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?><ultima_stanza>%s</ultima_stanza>", xml);
	//DEBUG_ERR(("GlobalChat XML sent: %s\n", xmlFull));

	writeByte(unknown);
	writeByte(action);
	writeByte(stanza);
	writeStringASCII(xmlFull);

	trim();
	push(target);
}
