#ifndef __SEND_H__
#define __SEND_H__
#pragma once

#include "packet.h"
#include "network.h"

/***************************************************************************
 *
 *
 *	Packet **** : PacketGeneric				Temporary packet till all will be redone! (NORMAL)
 *
 *
 ***************************************************************************/
class PacketGeneric : public PacketSend
{
public:
	PacketGeneric(const CClient* target, BYTE *data, size_t length);
};

/***************************************************************************
 *
 *
 *	Packet **** : PacketTelnet				send message to telnet client (NORMAL)
 *
 *
 ***************************************************************************/
class PacketTelnet : public PacketSend
{
public:

	PacketTelnet(const CClient* target, LPCTSTR message, bool bNullTerminated = false);
};

/***************************************************************************
 *
 *
 *	Packet **** : PacketWeb					send message to web client (NORMAL)
 *
 *
 ***************************************************************************/
class PacketWeb : public PacketSend
{
public:
	PacketWeb(const CClient * target = NULL, const BYTE * data = NULL, size_t length = 0);
	void setData(const BYTE * data, size_t length);
};

/***************************************************************************
 *
 *
 *	Packet 0x0B : PacketCombatDamage		sends notification of got damage (NORMAL)
 *
 *
 ***************************************************************************/
class PacketCombatDamage : public PacketSend
{
public:
	PacketCombatDamage(const CClient* target, WORD damage, CGrayUID uid);

	virtual bool canSendTo(const NetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState* state)
	{
		return state->isClientVersion(MINCLIVER_NEWDAMAGE);
	}
};

/***************************************************************************
 *
 *
 *	Packet 0x11 : PacketHealthBarInfo		sends health bar info (LOW)
 *
 *
 ***************************************************************************/
class PacketHealthBarInfo : public PacketSend
{
public:
	PacketHealthBarInfo(const CClient* target, CObjBase* other);
};

/***************************************************************************
 *
 *
 *	Packet 0x17 : PacketHealthBarUpdate		update health bar colour (LOW)
 *
 *
 ***************************************************************************/
class PacketHealthBarUpdate : public PacketSend
{
private:
	CGrayUID m_character;

public:
	enum Color
	{
		GreenBar = 1,
		YellowBar = 2
	};

	PacketHealthBarUpdate(const CClient* target, const CChar* character);

	virtual bool onSend(const CClient* client);

	virtual bool canSendTo(const NetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState* state)
	{
	    return state->isClientVersion(MINCLIVER_SA) || state->isClientEnhanced();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0x1A : PacketItemWorld			sends item on ground (NORMAL)
 *
 *
 ***************************************************************************/
class PacketItemWorld : public PacketSend
{
private:
	CGrayUID m_item;

protected:
	PacketItemWorld(BYTE id, size_t size, CGrayUID uid);

public:
	PacketItemWorld(const CClient* target, CItem* item);

	void adjustItemData(const CClient* target, CItem* item, ITEMID_TYPE &id, HUE_TYPE &hue, WORD &amount, BYTE &layer, BYTE &flags);

	virtual bool onSend(const CClient* client);
};

/***************************************************************************
 *
 *
 *	Packet 0x1B : PacketPlayerStart			allow client to start playing (HIGH)
 *
 *
 ***************************************************************************/
class PacketPlayerStart : public PacketSend
{
public:
	PacketPlayerStart(const CClient* target);
};

/***************************************************************************
 *
 *
 *	Packet 0x1C: PacketMessageASCII			show message to client (NORMAL)
 *
 *
 ***************************************************************************/
class PacketMessageASCII : public PacketSend
{
public:
	PacketMessageASCII(const CClient* target, LPCTSTR pszText, const CObjBaseTemplate* source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font);
};

/***************************************************************************
 *
 *
 *	Packet 0x1D : PacketRemoveObject		removes object from view (NORMAL)
 *
 *
 ***************************************************************************/
class PacketRemoveObject : public PacketSend
{
public:
	PacketRemoveObject(const CClient* target, CGrayUID uid);
};

/***************************************************************************
 *
 *
 *	Packet 0x20 : PacketPlayerUpdate		update player character on screen (NORMAL)
 *
 *
 ***************************************************************************/
class PacketPlayerUpdate : public PacketSend
{
public:
	PacketPlayerUpdate(const CClient* target);
};

/***************************************************************************
 *
 *
 *	Packet 0x21 : PacketMovementRej			rejects movement (HIGHEST)
 *
 *
 ***************************************************************************/
class PacketMovementRej : public PacketSend
{
public:
	PacketMovementRej(const CClient* target, BYTE sequence);
};

/***************************************************************************
 *
 *
 *	Packet 0x22 : PacketMovementAck			accepts movement (HIGHEST)
 *
 *
 ***************************************************************************/
class PacketMovementAck : public PacketSend
{
public:
	PacketMovementAck(const CClient* target, BYTE sequence);
};

/***************************************************************************
 *
 *
 *	Packet 0x23 : PacketDragAnimation		drag animation (LOW)
 *
 *
 ***************************************************************************/
class PacketDragAnimation : public PacketSend
{
public:
	PacketDragAnimation(const CChar* source, const CItem* item, const CObjBase* container, const CPointMap* pt);

	virtual bool canSendTo(const NetState* state) const;
};

/***************************************************************************
 *
 *
 *	Packet 0x24 : PacketContainerOpen		open container gump (LOW)
 *
 *
 ***************************************************************************/
class PacketContainerOpen : public PacketSend
{
private:
	CGrayUID m_container;

public:
	PacketContainerOpen(const CClient* target, const CObjBase* container, GUMP_TYPE gump);

	virtual bool onSend(const CClient* client);
};

/***************************************************************************
 *
 *
 *	Packet 0x25 : PacketItemContainer		sends item in a container (NORMAL)
 *
 *
 ***************************************************************************/
class PacketItemContainer : public PacketSend
{
private:
	CGrayUID m_item;

public:
	PacketItemContainer(const CClient* target, const CItem* item);
	PacketItemContainer(const CItem* spellbook, const CSpellDef* spell);

	void completeForTarget(const CClient* target, const CItem* spellbook);
	virtual bool onSend(const CClient* client);
};

/***************************************************************************
 *
 *
 *	Packet 0x26 : PacketKick				notifies client they have been kicked (HIGHEST)
 *
 *
 ***************************************************************************/
class PacketKick : public PacketSend
{
public:
	PacketKick(const CClient* target);
};

/***************************************************************************
 *
 *
 *	Packet 0x27 : PacketDragCancel			cancel item drag (HIGH)
 *
 *
 ***************************************************************************/
class PacketDragCancel : public PacketSend
{
public:
	enum Reason
	{
		CannotLift = 0x00,
		OutOfRange = 0x01,
		OutOfSight = 0x02,
		TryToSteal = 0x03,
		AreHolding = 0x04,
		Other = 0x05
	};

	PacketDragCancel(const CClient* target, Reason code);
};

/***************************************************************************
 *
 *
 *	Packet 0x29 : PacketDropAccepted		notify drop accepted (kr) (NORMAL)
 *
 *
 ***************************************************************************/
class PacketDropAccepted : public PacketSend
{
public:
	PacketDropAccepted(const CClient* target);

	virtual bool canSendTo(const NetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState* state)
	{
		return state->isClientKR();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0x2C : PacketDeathMenu			display death menu/effect (NORMAL)
 *
 *
 ***************************************************************************/
class PacketDeathMenu : public PacketSend
{
public:
	enum Reason
	{
		ServerSent = 0x00,
		Resurrect = 0x01,
		Ghost = 0x02
	};

	PacketDeathMenu(const CClient* target, Reason reason);
};

/***************************************************************************
 *
 *
 *	Packet 0x2E : PacketItemEquipped		sends equipped item (NORMAL)
 *
 *
 ***************************************************************************/
class PacketItemEquipped : public PacketSend
{
public:
	PacketItemEquipped(const CClient* target, const CItem* item);
};

/***************************************************************************
 *
 *
 *	Packet 0x2F : PacketSwing				fight swing (LOW)
 *
 *
 ***************************************************************************/
class PacketSwing : public PacketSend
{
public:
	PacketSwing(const CClient* target, const CChar* defender);
};

/***************************************************************************
 *
 *
 *	Packet 0x3A : PacketSkills				character skills (LOW)
 *
 *
 ***************************************************************************/
class PacketSkills : public PacketSend
{
public:
	PacketSkills(const CClient* target, const CChar* character, SKILL_TYPE skill);
};

/***************************************************************************
 *
 *
 *	Packet 0x3B : PacketCloseVendor			close vendor menu (NORMAL)
 *
 *
 ***************************************************************************/
class PacketCloseVendor : public PacketSend
{
public:
	PacketCloseVendor(const CClient* target, const CChar* vendor);
};

/***************************************************************************
 *
 *
 *	Packet 0x3C : PacketItemContents		contents of an item (NORMAL)
 *
 *
 ***************************************************************************/
class PacketItemContents : public PacketSend
{
private:
	CGrayUID m_container;

public:
	PacketItemContents(CClient* target, const CItemContainer* container, bool isShop, bool filterLayers); // standard content
	PacketItemContents(const CClient* target, const CItem* spellbook); // spellbook spells
	PacketItemContents(const CClient* target, const CItemContainer* spellbook); // custom spellbook spells
	virtual bool onSend(const CClient* client);
};

/***************************************************************************
 *
 *
 *	Packet 0x3F : PacketQueryClient			Query Client for block info (NORMAL)
 *
 *
 ***************************************************************************/
class PacketQueryClient : public PacketSend
{
public:
	PacketQueryClient(CClient* target, BYTE bCmd = 0xFF);

};

/***************************************************************************
 *
 *
 *	Packet 0x4F : PacketGlobalLight			sets global light level (NORMAL)
 *
 *
 ***************************************************************************/
class PacketGlobalLight : public PacketSend
{
public:
	PacketGlobalLight(const CClient* target, BYTE light);
};

/***************************************************************************
 *
 *
 *	Packet 0x53 : PacketWarningMessage		show popup warning message (NORMAL)
 *
 *
 ***************************************************************************/
class PacketWarningMessage : public PacketSend
{
public:
	enum Message
	{
		BadPassword =			0x00,
		NoCharacter =			0x01,
		CharacterExists =		0x02,
		NoFreeCharacterSlots =	0x03,	// enhanced client only
		AuthenticationError =	0x04,	// enhanced client only
		CharacterInWorld =		0x05,
		SyncError =				0x06,
		Idle =					0x07,
		CouldntAttachServer =	0x08,
		CharacterTransfer =		0x09,
		InvalidName =			0x0A
	};

	PacketWarningMessage(const CClient* target, Message code);
};

/***************************************************************************
 *
 *
 *	Packet 0x54 : PacketPlaySound			play a sound (NORMAL)
 *
 *
 ***************************************************************************/
class PacketPlaySound : public PacketSend
{
public:
	PacketPlaySound(const CClient* target, SOUND_TYPE sound, int flags, int volume, const CPointMap& pos);
};

/***************************************************************************
 *
 *
 *	Packet 0x55 : PacketLoginComplete		redraw all (NORMAL)
 *
 *
 ***************************************************************************/
class PacketLoginComplete : public PacketSend
{
public:
	PacketLoginComplete(const CClient* target);
};

/***************************************************************************
 *
 *
 *	Packet 0x56 : PacketMapPlot				show/edit map plots (LOW)
 *
 *
 ***************************************************************************/
class PacketMapPlot : public PacketSend
{
public:
	PacketMapPlot(const CClient* target, const CItem* map, MAPCMD_TYPE mode, bool edit);
	PacketMapPlot(const CItem* map, MAPCMD_TYPE mode, bool edit);

	void setPin(WORD x, WORD y);
};

/***************************************************************************
 *
 *
 *	Packet 0x5B : PacketGameTime			current game time (IDLE)
 *
 *
 ***************************************************************************/
class PacketGameTime : public PacketSend
{
public:
	PacketGameTime(const CClient* target, int hours = 0, int minutes = 0, int seconds = 0);
};

/***************************************************************************
 *
 *
 *	Packet 0x65 : PacketWeather				set current weather (IDLE)
 *
 *
 ***************************************************************************/
class PacketWeather : public PacketSend
{
public:
	PacketWeather(const CClient* target, WEATHER_TYPE weather, int severity, int temperature);
};

/***************************************************************************
 *
 *
 *	Packet 0x66 : PacketBookPageContent		send book page content (LOW)
 *
 *
 ***************************************************************************/
class PacketBookPageContent : public PacketSend
{
protected:
	size_t m_pages;

public:
	PacketBookPageContent(const CClient* target, const CItem* book, size_t startpage, size_t pagecount = 1);
	void addPage(const CItem* book, size_t page);
};

/***************************************************************************
 *
 *
 *	Packet 0x6C : PacketAddTarget				adds target cursor to client (LOW)
 *	Packet 0x99 : PacketAddTarget				adds target cursor to client with multi (LOW)
 *
 *
 ***************************************************************************/
class PacketAddTarget : public PacketSend
{
public:
	enum TargetType
	{
		Object = 0x00, // items/chars only
		Ground = 0x01  // also allow ground
	};

	enum Flags
	{
		None = 0x00,
		Harmful = 0x01,
		Beneficial = 0x02,
		Cancel = 0x03
	};

	PacketAddTarget(const CClient* target, TargetType type, DWORD context, Flags flags);
	PacketAddTarget(const CClient* target, TargetType type, DWORD context, Flags flags, ITEMID_TYPE id, HUE_TYPE color);
};

/***************************************************************************
 *
 *
 *	Packet 0x6D : PacketPlayMusic			adds music to the client (IDLE)
 *
 *
 ***************************************************************************/
class PacketPlayMusic : public PacketSend
{
public:
	PacketPlayMusic(const CClient* target, WORD musicID);
};

/***************************************************************************
 *
 *
 *	Packet 0x6E : PacketAction				plays an animation (LOW)
 *	Packet 0xE2 : PacketActionBasic			plays an animation (client > 7.0.0.0) (LOW)
 *
 *
 ***************************************************************************/
class PacketAction : public PacketSend
{
public:
	PacketAction(const CChar* character, ANIM_TYPE action, WORD repeat, bool backward, BYTE delay, BYTE len);
};

class PacketActionBasic : public PacketSend
{
public:
	PacketActionBasic(const CChar* character, ANIM_TYPE_NEW action, ANIM_TYPE_NEW subaction, BYTE variation);
};

/***************************************************************************
 *
 *
 *	Packet 0x6F : PacketTradeAction			perform a trade action (NORMAL)
 *
 *
 ***************************************************************************/
class PacketTradeAction : public PacketSend
{
public:
	PacketTradeAction(SECURE_TRADE_TYPE action);
	void prepareContainerOpen(const CChar *character, const CItem *container1, const CItem *container2);
	void prepareReadyChange(const CItemContainer *container1, const CItemContainer *container2);
	void prepareClose(const CItemContainer *container);
	void prepareUpdateGold(const CItemContainer *container, DWORD gold, DWORD platinum);
	void prepareUpdateLedger(const CItemContainer *container, DWORD gold, DWORD platinum);
};

/***************************************************************************
 *
 *
 *	Packet 0x70 : PacketEffect				displays a visual effect (NORMAL)
 *	Packet 0xC0 : PacketEffect				displays a hued visual effect (NORMAL)
 *
 *
 ***************************************************************************/
class PacketEffect : public PacketSend
{
public:
	PacketEffect(const CClient* target, EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate* dst, const CObjBaseTemplate* src, BYTE speed, BYTE loop, bool explode);
	PacketEffect(const CClient* target, EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate* dst, const CObjBaseTemplate* src, BYTE speed, BYTE loop, bool explode, DWORD hue, DWORD render);
	PacketEffect(const CClient* target, EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate* dst, const CObjBaseTemplate* src, BYTE speed, BYTE loop, bool explode, DWORD hue, DWORD render, WORD effectid, DWORD explodeid, WORD explodesound, DWORD effectuid, BYTE type);
	void writeBasicEffect(EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate* dst, const CObjBaseTemplate* src, BYTE speed, BYTE loop, bool explode);
	void writeHuedEffect(DWORD hue, DWORD render);
};

/***************************************************************************
 *
 *
 *	Packet 0x71 : PacketBulletinBoard		display a bulletin board or message (LOW)
 *
 *
 ***************************************************************************/
class PacketBulletinBoard : public PacketSend
{
public:
	PacketBulletinBoard(const CClient* target, const CItemContainer* board);
	PacketBulletinBoard(const CClient* target, BBOARDF_TYPE action, const CItemContainer* board, const CItemMessage* message);
};

/***************************************************************************
 *
 *
 *	Packet 0x72 : PacketWarMode				update war mode status (LOW)
 *
 *
 ***************************************************************************/
class PacketWarMode : public PacketSend
{
public:
	PacketWarMode(const CClient* target, const CChar* character);
};

/***************************************************************************
 *
 *
 *	Packet 0x73 : PacketPingAck				ping reply (IDLE)
 *
 *
 ***************************************************************************/
class PacketPingAck : public PacketSend
{
public:
	PacketPingAck(const CClient* target, BYTE value = 0);
};

/***************************************************************************
 *
 *
 *	Packet 0x74 : PacketVendorBuyList		show list of vendor items (LOW)
 *
 *
 ***************************************************************************/
class PacketVendorBuyList : public PacketSend
{
public:
	PacketVendorBuyList(const CClient* target, const CItemContainer* contParent, int convertFactor);
};

/***************************************************************************
 *
 *
 *	Packet 0x76 : PacketZoneChange			change server zone (LOW)
 *
 *
 ***************************************************************************/
class PacketZoneChange : public PacketSend
{
public:
	PacketZoneChange(const CClient* target, const CPointMap& pos);
};

/***************************************************************************
 *
 *
 *	Packet 0x77 : PacketCharacterMove		move a character (NORMAL)
 *
 *
 ***************************************************************************/
class PacketCharacterMove : public PacketSend
{
public:
	PacketCharacterMove(const CClient* target, const CChar* character, BYTE direction);
};

/***************************************************************************
 *
 *
 *	Packet 0x78 : PacketCharacter			create a character (NORMAL)
 *
 *
 ***************************************************************************/
class PacketCharacter : public PacketSend
{
private:
	CGrayUID m_character;

public:
	PacketCharacter(CClient* target, const CChar* character);

	virtual bool onSend(const CClient* client);
};

/***************************************************************************
 *
 *
 *	Packet 0x7C : PacketDisplayMenu			show a menu selection (LOW)
 *
 *
 ***************************************************************************/
class PacketDisplayMenu : public PacketSend
{
public:
	PacketDisplayMenu(const CClient* target, CLIMODE_TYPE mode, const CMenuItem* items, size_t count, const CObjBase* object);
};

/***************************************************************************
 *
 *
 *	Packet 0x81 : PacketChangeCharacter		allow client to change character (LOW)
 *
 *
 ***************************************************************************/
class PacketChangeCharacter : public PacketSend
{
public:
	PacketChangeCharacter(CClient* target);

	virtual bool canSendTo(const NetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState* state)
	{
		return !(state->isClientKR() || state->isClientEnhanced());
	}
};

/***************************************************************************
 *
 *
 *	Packet 0x82 : PacketLoginError			login error response (HIGHEST)
 *
 *
 ***************************************************************************/
class PacketLoginError : public PacketSend
{
public:
	enum Reason
	{
		Invalid = 0x00, // no account
		InUse	= 0x01, // already in use
		Blocked = 0x02, // client blocked
		BadPass = 0x03, // incorrect password
		Other	= 0x04, // other (e.g. timeout)

		// the error codes below are not sent to or understood by the client,
		// and should be translated into one of the codes above
		BadVersion,     // version not permitted
		BadCharacter,   // invalid character selected
		BadAuthID,      // incorrect auth id
		BadAccount,     // bad account name (length, characters)
		BadPassword,    // bad password (length, characters)
		BadEncLength,   // bad message length
		EncUnknown,     // unknown encryption
		EncCrypt,       // crypted client not allowed
		EncNoCrypt,     // non-crypted client not allowed
		CharIdle,       // character is already ingame
		TooManyChars,   // account has too many characters
		CreationBlocked,// character creation is blocked in this moments.
		BlockedIP,      // ip is blocked
		MaxClients,     // max clients reached
		MaxGuests,      // max guests reached
		MaxPassTries,   // max password tries reached


		Success = 0xFF  // no error
	};

	PacketLoginError(const CClient* target, Reason reason);
};

/***************************************************************************
 *
 *
 *	Packet 0x85 : PacketDeleteError			delete character error response (LOW)
 *
 *
 ***************************************************************************/
class PacketDeleteError : public PacketSend
{
public:
	enum Reason
	{
		BadPass        = 0x00, // incorrect password
		NotExist       = 0x01, // character does not exist
		InUse          = 0x02, // character is being played right now
		NotOldEnough   = 0x03, // character is not old enough to delete
		BackupQueue    = 0x04, // character is currently queued for backup
		InvalidRequest = 0x05, // couldn't carry out the request

		Success        = 0xFF  // no error
	};

	PacketDeleteError(const CClient* target, Reason reason);
};

/***************************************************************************
 *
 *
 *	Packet 0x86 : PacketCharacterListUpdate	update character list (LOW)
 *
 *
 ***************************************************************************/
class PacketCharacterListUpdate : public PacketSend
{
public:
	PacketCharacterListUpdate(CClient* target);
};

/***************************************************************************
 *
 *
 *	Packet 0x88 : PacketPaperdoll			show paperdoll (LOW)
 *
 *
 ***************************************************************************/
class PacketPaperdoll : public PacketSend
{
public:
	PacketPaperdoll(const CClient* target, const CChar* character);
};

/***************************************************************************
 *
 *
 *	Packet 0x89 : PacketCorpseEquipment		send corpse equipment (NORMAL)
 *
 *
 ***************************************************************************/
class PacketCorpseEquipment : public PacketSend
{
private:
	CGrayUID m_corpse;

public:
	PacketCorpseEquipment(CClient* target, const CItemContainer* corpse);
	virtual bool onSend(const CClient* client);
};

/***************************************************************************
 *
 *
 *	Packet 0x8B : PacketSignGump			show a sign (LOW)
 *
 *
 ***************************************************************************/
class PacketSignGump : public PacketSend
{
public:
	PacketSignGump(const CClient* target, const CObjBase* object, GUMP_TYPE gump, LPCTSTR unknown, LPCTSTR text);
};

/***************************************************************************
 *
 *
 *	Packet 0x8C : PacketServerRelay			relay client to server (IDLE)
 *
 *
 ***************************************************************************/
class PacketServerRelay : public PacketSend
{
private:
	DWORD m_customerId;

public:
	PacketServerRelay(const CClient* target, DWORD ip, WORD port, DWORD customerId);
	virtual void onSent(CClient* client);
};

/***************************************************************************
 *
 *
 *	Packet 0x90 : PacketDisplayMap			display map (LOW)
 *
 *
 ***************************************************************************/
class PacketDisplayMap : public PacketSend
{
public:
	PacketDisplayMap(const CClient* target, const CItemMap* map, const CRectMap& rect);
};

/***************************************************************************
 *
 *
 *	Packet 0x93 : PacketDisplayBook			display book (LOW)
 *
 *
 ***************************************************************************/
class PacketDisplayBook : public PacketSend
{
public:
	PacketDisplayBook(const CClient* target, CItem* book);
};

/***************************************************************************
 *
 *
 *	Packet 0x95 : PacketShowDyeWindow		show dye window (LOW)
 *
 *
 ***************************************************************************/
class PacketShowDyeWindow : public PacketSend
{
public:
	PacketShowDyeWindow(const CClient* target, const CObjBase* object);
};

/***************************************************************************
 *
 *
 *	Packet 0x98 : PacketAllNamesResponse	all names macro response (IDLE)
 *
 *
 ***************************************************************************/
class PacketAllNamesResponse : public PacketSend
{
public:
	PacketAllNamesResponse(const CClient* target, const CObjBase* object);
};

/***************************************************************************
 *
 *
 *	Packet 0x9A : PacketAddPrompt			prompt for ascii text response (LOW)
 *	Packet 0xC2 : PacketAddPrompt			prompt for unicode text response (LOW)
 *
 *
 ***************************************************************************/
class PacketAddPrompt : public PacketSend
{
public:
	PacketAddPrompt(const CClient* target, CGrayUID context1, CGrayUID context2, bool useUnicode);
};

/***************************************************************************
 *
 *
 *	Packet 0x9E : PacketVendorSellList		show list of items to sell (LOW)
 *
 *
 ***************************************************************************/
class PacketVendorSellList : public PacketSend
{
public:
	PacketVendorSellList(const CClient* target, const CChar* vendor, const CItemContainer* contParent, CItemContainer* contBuy, int convertFactor);
};

/***************************************************************************
 *
 *
 *	Packet 0xA1 : PacketHealthUpdate		update character health (LOW)
 *
 *
 ***************************************************************************/
class PacketHealthUpdate : public PacketSend
{
public:
	PacketHealthUpdate(const CChar* character, bool full);
};

/***************************************************************************
 *
 *
 *	Packet 0xA2 : PacketManaUpdate			update character mana (LOW)
 *
 *
 ***************************************************************************/
class PacketManaUpdate : public PacketSend
{
public:
	PacketManaUpdate(const CChar* character, bool full);
};

/***************************************************************************
 *
 *
 *	Packet 0xA3 : PacketStaminaUpdate		update character stamina (LOW)
 *
 *
 ***************************************************************************/
class PacketStaminaUpdate : public PacketSend
{
public:
	PacketStaminaUpdate(const CChar* character, bool full);
};

/***************************************************************************
 *
 *
 *	Packet 0xA5 : PacketWebPage				send client to a webpage (LOW)
 *
 *
 ***************************************************************************/
class PacketWebPage : public PacketSend
{
public:
	PacketWebPage(const CClient* target, LPCTSTR url);
};

/***************************************************************************
 *
 *
 *	Packet 0xA6 : PacketOpenScroll			open scroll message (LOW)
 *
 *
 ***************************************************************************/
class PacketOpenScroll : public PacketSend
{
public:
	PacketOpenScroll(const CClient* target, CResourceLock &s, SCROLL_TYPE type, DWORD context, LPCTSTR header);
};

/***************************************************************************
 *
 *
 *	Packet 0xA8 : PacketServerList			send server list (LOW)
 *
 *
 ***************************************************************************/
class PacketServerList : public PacketSend
{
public:
	PacketServerList(const CClient* target);
	void writeServerEntry(const CServerRef& server, int index, bool reverseIp);
};

/***************************************************************************
 *
 *
 *	Packet 0xA9 : PacketCharacterList		send character list (LOW)
 *
 *
 ***************************************************************************/
class PacketCharacterList : public PacketSend
{
public:
	PacketCharacterList(CClient* target);
};

/***************************************************************************
 *
 *
 *	Packet 0xAA : PacketAttack				set attack target (NORMAL)
 *
 *
 ***************************************************************************/
class PacketAttack : public PacketSend
{
public:
	PacketAttack(const CClient* target, CGrayUID serial);
};

/***************************************************************************
 *
 *
 *	Packet 0xAB : PacketGumpValueInput		show input dialog (LOW)
 *
 *
 ***************************************************************************/
class PacketGumpValueInput : public PacketSend
{
public:
	PacketGumpValueInput(const CClient* target, bool cancel, INPVAL_STYLE style, DWORD maxLength, LPCTSTR text, LPCTSTR caption, CObjBase* object);
};

/***************************************************************************
 *
 *
 *	Packet 0xAE: PacketMessageUNICODE			show message to client (NORMAL)
 *
 *
 ***************************************************************************/
class PacketMessageUNICODE : public PacketSend
{
public:
	PacketMessageUNICODE(const CClient* target, const NWORD* pszText, const CObjBaseTemplate* source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID language);
};

/***************************************************************************
 *
 *
 *	Packet 0xAF : PacketDeath				notifies about character death (NORMAL)
 *
 *
 ***************************************************************************/
class PacketDeath : public PacketSend
{
public:
	PacketDeath(CChar* dead, CItemCorpse* corpse, bool bFrontFall);
};

/***************************************************************************
 *
 *
 *	Packet 0xB0 : PacketGumpDialog			displays a dialog gump (LOW)
 *	Packet 0xDD : PacketGumpDialog			displays a dialog gump using compression (LOW)
 *
 *
 ***************************************************************************/
class PacketGumpDialog : public PacketSend
{
public:
	PacketGumpDialog(int x, int y, CObjBase* object, DWORD context);
	void writeControls(const CClient* target, const CGString* controls, size_t controlCount, const CGString* texts, size_t textCount);

protected:
	void writeCompressedControls(const CGString* controls, size_t controlCount, const CGString* texts, size_t textCount);
	void writeStandardControls(const CGString* controls, size_t controlCount, const CGString* texts, size_t textCount);
};

/***************************************************************************
 *
 *
 *	Packet 0xB2 : PacketChatMessage			send a chat system message (LOW)
 *
 *
 ***************************************************************************/
class PacketChatMessage : public PacketSend
{
public:
	PacketChatMessage(const CClient* target, CHATMSG_TYPE type, LPCTSTR param1, LPCTSTR param2, CLanguageID language);
};

/***************************************************************************
 *
 *
 *	Packet 0xB7 : PacketTooltip				send a tooltip (IDLE)
 *
 *
 ***************************************************************************/
class PacketTooltip : public PacketSend
{
public:
	PacketTooltip(const CClient* target, const CObjBase* object, LPCTSTR text);
};

/***************************************************************************
 *
 *
 *	Packet 0xB8 : PacketProfile				send a character profile (LOW)
 *
 *
 ***************************************************************************/
class PacketProfile : public PacketSend
{
public:
	PacketProfile(const CClient* target, const CChar* character);
};

/***************************************************************************
 *
 *
 *	Packet 0xB9 : PacketEnableFeatures		enable client features (NORMAL)
 *
 *
 ***************************************************************************/
class PacketEnableFeatures : public PacketSend
{
public:
	PacketEnableFeatures(const CClient* target, DWORD flags);
};

/***************************************************************************
 *
 *
 *	Packet 0xBA : PacketArrowQuest			display onscreen arrow for client to follow (NORMAL)
 *
 *
 ***************************************************************************/
class PacketArrowQuest : public PacketSend
{
public:
	PacketArrowQuest(const CClient* target, WORD x, WORD y, DWORD id);
};

/***************************************************************************
 *
 *
 *	Packet 0xBC : PacketSeason				change season (NORMAL)
 *
 *
 ***************************************************************************/
class PacketSeason : public PacketSend
{
public:
	PacketSeason(const CClient* target, SEASON_TYPE season, bool playMusic);
};

/***************************************************************************
*
*
*	Packet 0xBD : PacketClientVersionReq	request client version (HIGH)
*
*
***************************************************************************/
class PacketClientVersionReq : public PacketSend
{
public:
	PacketClientVersionReq(const CClient* target);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF : PacketExtended			extended command
 *
 *
 ***************************************************************************/
class PacketExtended : public PacketSend
{
public:
	PacketExtended(EXTDATA_TYPE type, size_t len = 0, Priority priority = PRI_NORMAL);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x04 : PacketGumpChange		change gump (LOW)
 *
 *
 ***************************************************************************/
class PacketGumpChange : public PacketExtended
{
public:
	PacketGumpChange(const CClient* target, DWORD context, int buttonId);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x06 : PacketParty			party packet
 *
 *
 ***************************************************************************/
class PacketParty : public PacketExtended
{
public:
	PacketParty(PARTYMSG_TYPE type, size_t len = 0, Priority priority = PRI_NORMAL);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x06.0x01 : PacketPartyList		send list of party members (NORMAL)
 *
 *
 ***************************************************************************/
class PacketPartyList : public PacketParty
{
public:
	PacketPartyList(const CCharRefArray* members);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x06.0x02 : PacketPartyRemoveMember		remove member from party (NORMAL)
 *
 *
 ***************************************************************************/
class PacketPartyRemoveMember : public PacketParty
{
public:
	PacketPartyRemoveMember(const CChar* member, const CCharRefArray* members);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x06.0x04 : PacketPartyChat		send party chat message (NORMAL)
 *
 *
 ***************************************************************************/
class PacketPartyChat : public PacketParty
{
public:
	PacketPartyChat(const CChar* source, const NCHAR* text);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x06.0x07 : PacketPartyInvite	send party invitation (NORMAL)
 *
 *
 ***************************************************************************/
class PacketPartyInvite : public PacketParty
{
public:
	PacketPartyInvite(const CClient* target, const CChar* inviter);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x08 : PacketMapChange			change map (NORMAL)
 *
 *
 ***************************************************************************/
class PacketMapChange : public PacketExtended
{
public:
	PacketMapChange(const CClient* target, int map);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x10 : PacketPropertyListVersionOld		property (tool tip) version (LOW)
 *
 *
 ***************************************************************************/
class PacketPropertyListVersionOld : public PacketExtended
{
protected:
	CGrayUID m_object;

public:
	PacketPropertyListVersionOld(const CClient* target, const CObjBase* object, DWORD version);
	virtual bool onSend(const CClient* client);

	virtual bool canSendTo(const NetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState* state)
	{
		return state->isClientVersion(MINCLIVER_AOS);
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x14 : PacketDisplayPopup		display popup menu (LOW)
 *
 *
 ***************************************************************************/
class PacketDisplayPopup : public PacketExtended
{
private:
	bool m_newPacketFormat;
	int m_popupCount;

public:
	PacketDisplayPopup(const CClient* target, CGrayUID uid);

	void addOption(WORD entryTag, DWORD textId, WORD flags, WORD color);
	void finalise(void);

	int getOptionCount(void) const
	{
		return m_popupCount;
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x16 : PacketCloseUIWindow		Close Container (NORMAL)
 *
 *
 ***************************************************************************/
class PacketCloseUIWindow : public PacketExtended
{
public:
	PacketCloseUIWindow(const CClient* target, const CChar* character, DWORD command);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x16.0x0C : PacketCloseContainer		Close Container (NORMAL)
 *
 *
 ***************************************************************************/
class PacketCloseContainer : public PacketExtended
{
public:
	PacketCloseContainer(const CClient* target, const CObjBase* object);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x18 : PacketEnableMapDiffs		enable use of map diff files (NORMAL)
 *
 *
 ***************************************************************************/
class PacketEnableMapDiffs : public PacketExtended
{
public:
	PacketEnableMapDiffs(const CClient* target);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x19.0x02 : PacketStatLocks		update lock status of stats (NORMAL)
 *
 *
 ***************************************************************************/
class PacketStatLocks : public PacketExtended
{
public:
	PacketStatLocks(const CClient* target, const CChar* character);

	virtual bool canSendTo(const NetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState* state)
	{
		return state->isClientVersion(MINCLIVER_STATLOCKS);
	}
};

/***************************************************************************
*
*
*	Packet 0xBF.0x19 : BondedStatus			set bonded status (NORMAL)
*
*
***************************************************************************/
class PacketBondedStatus : public PacketExtended
{
public:
	PacketBondedStatus(const CClient * target, const CChar * pChar, bool IsGhost);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x1B : PacketSpellbookContent	fill spellbook content (NORMAL)
 *
 *
 ***************************************************************************/
class PacketSpellbookContent : public PacketExtended
{
public:
	PacketSpellbookContent(const CClient* target, const CItem* spellbook, WORD offset);

	virtual bool canSendTo(const NetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState* state)
	{
	    return state->m_client->m_TooltipEnabled;
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x1D : PacketHouseDesignVersion			house design version (LOW)
 *
 *
 ***************************************************************************/
class PacketHouseDesignVersion : public PacketExtended
{
public:
	PacketHouseDesignVersion(const CClient* target, const CItemMultiCustom* house);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x20.0x04 : PacketHouseBeginCustomise	begin house customisation (NORMAL)
 *
 *
 ***************************************************************************/
class PacketHouseBeginCustomise : public PacketExtended
{
public:
	PacketHouseBeginCustomise(const CClient* target, const CItemMultiCustom* house);

	virtual bool canSendTo(const NetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState* state)
	{
		return state->isClientVersion(MINCLIVER_AOS) || state->isClientKR() || state->isClientEnhanced();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x20.0x05 : PacketHouseEndCustomise		end house customisation (NORMAL)
 *
 *
 ***************************************************************************/
class PacketHouseEndCustomise : public PacketExtended
{
public:
	PacketHouseEndCustomise(const CClient* target, const CItemMultiCustom* house);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x22 : PacketCombatDamageOld		[old] sends notification of got damage (NORMAL)
 *
 *
 ***************************************************************************/
class PacketCombatDamageOld : public PacketExtended
{
public:
	PacketCombatDamageOld(const CClient* target, BYTE damage, CGrayUID uid);

	virtual bool canSendTo(const NetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState* state)
	{
		return state->isClientVersion(MINCLIVER_AOS);
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x26 : PacketSpeedMode		set movement speed (HIGH)
 *
 *
 ***************************************************************************/
class PacketSpeedMode : public PacketExtended
{
public:
	PacketSpeedMode(const CClient* target, BYTE mode);
};

/***************************************************************************
 *
 *
 *	Packet 0xC1: PacketMessageLocalised		show localised message to client (NORMAL)
 *
 *
 ***************************************************************************/
class PacketMessageLocalised : public PacketSend
{
public:
	PacketMessageLocalised(const CClient* target, DWORD cliloc, const CObjBaseTemplate* source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font, LPCTSTR args);
};

/***************************************************************************
 *
 *
 *	Packet 0xC8: PacketVisualRange			set the visual range of the client (NORMAL)
 *
 *
 ***************************************************************************/
class PacketVisualRange : public PacketSend
{
public:
	PacketVisualRange(const CClient* target, BYTE range);
};

/***************************************************************************
 *
 *
 *	Packet 0xCC: PacketMessageLocalisedEx	show extended localised message to client (NORMAL)
 *
 *
 ***************************************************************************/
class PacketMessageLocalisedEx : public PacketSend
{
public:
	PacketMessageLocalisedEx(const CClient* target, DWORD cliloc, const CObjBaseTemplate* source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font, AFFIX_TYPE affixType, LPCTSTR affix, LPCTSTR args);
};

/***************************************************************************
 *
 *
 *	Packet 0xD1 : PacketLogoutAck			accept logout char (LOW)
 *
 *
 ***************************************************************************/
class PacketLogoutAck : public PacketSend
{
public:
	PacketLogoutAck(const CClient* target);
};

/***************************************************************************
 *
 *
 *	Packet 0xD4 : PacketDisplayBookNew		display book (LOW)
 *
 *
 ***************************************************************************/
class PacketDisplayBookNew : public PacketSend
{
public:
	PacketDisplayBookNew(const CClient* target, CItem* book);

	virtual bool canSendTo(const NetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState* state)
	{
		return state->isClientVersion(MINCLIVER_ML) || state->isClientKR() || state->isClientEnhanced();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xD6 : PacketPropertyList		property (tool tip) for objects (IDLE)
 *
 *
 ***************************************************************************/
class PacketPropertyList : public PacketSend
{
protected:
	CGrayUID m_object;
	long long m_time;
	DWORD m_version;
	int m_entryCount;

public:
	PacketPropertyList(const CObjBase* object, DWORD version, const CGObArray<CClientTooltip*>* data);
	PacketPropertyList(const CClient* target, const PacketPropertyList* other);
	virtual bool onSend(const CClient* client);

	CGrayUID getObject(void) const { return m_object; }
	DWORD getVersion(void) const { return m_version; }
	int getEntryCount(void) const { return m_entryCount; }
	bool isEmpty(void) const { return m_entryCount == 0; }

	bool hasExpired(int timeout) const;

	virtual bool canSendTo(const NetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState* state)
	{
		return state->isClientVersion(MINCLIVER_AOS) || state->isClientKR() || state->isClientEnhanced();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xD8 : PacketHouseDesign			house design (IDLE)
 *
 *
 ***************************************************************************/
class PacketHouseDesign : public PacketSend
{
#define PLANEDATA_BUFFER	1024	// bytes reserved for plane data
#define STAIRSPERBLOCK		750		// number of stair items per block
#define STAIRDATA_BUFFER    (sizeof(StairData) * STAIRSPERBLOCK) // bytes reserved for stair data

private:
	struct StairData
	{
		NWORD m_id;
		BYTE m_x;
		BYTE m_y;
		BYTE m_z;
	};

	StairData* m_stairBuffer;
	int m_stairCount;

protected:
	int m_itemCount;
	int m_dataSize;
	int m_planeCount;
	int m_stairPlaneCount;
	const CItemMultiCustom* m_house;

public:
	PacketHouseDesign(const CItemMultiCustom* house, int revision);
	PacketHouseDesign(const PacketHouseDesign* other);
	virtual ~PacketHouseDesign(void);

	bool writePlaneData(int plane, int itemCount, BYTE* data, int dataSize);
	bool writeStairData(ITEMID_TYPE id, int x, int y, int z);
	void flushStairData(void);
	void finalise(void);

	virtual bool canSendTo(const NetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState* state)
	{
		return state->isClientVersion(MINCLIVER_AOS) || state->isClientKR() || state->isClientEnhanced();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xDC : PacketPropertyListVersion		property (tool tip) version (LOW)
 *
 *
 ***************************************************************************/
class PacketPropertyListVersion : public PacketSend
{
protected:
	CGrayUID m_object;

public:
	PacketPropertyListVersion(const CClient* target, const CObjBase* object, DWORD version);
	virtual bool onSend(const CClient* client);

	virtual bool canSendTo(const NetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState* state)
	{
		return state->isClientVersion(MINCLIVER_SE) || state->isClientKR() || state->isClientEnhanced();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xDF : PacketBuff				add/remove buff icon (LOW)
 *
 *
 ***************************************************************************/
class PacketBuff : public PacketSend
{
public:
	PacketBuff(const CClient* target, const BUFF_ICONS iconId, const DWORD clilocOne, const DWORD clilocTwo, const WORD time, LPCTSTR* args, size_t argCount); // add buff
	PacketBuff(const CClient* target, const BUFF_ICONS iconId); // remove buff

	virtual bool canSendTo(const NetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState* state)
	{
		return state->isClientVersion(MINCLIVER_BUFFS);
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xE3 : PacketKREncryption		kr encryption data (HIGH)
 *
 *
 ***************************************************************************/
class PacketKREncryption : public PacketSend
{
public:
	PacketKREncryption(const CClient* target);
};

/***************************************************************************
*
*
*	Packet 0xE5 : PacketWaypointAdd			add waypoint on KR/SA radar map (LOW)
*
*
***************************************************************************/
class PacketWaypointAdd : public PacketSend
{
public:
	enum WaypointType
	{
		Remove				= 0x0,
		Corpse				= 0x1,
		PartyMember			= 0x2,
		Unk1				= 0x3,
		QuestGiver			= 0x4,
		NewPlayerQuest		= 0x5,
		Healer				= 0x6,
		Unk2				= 0x7,
		Unk3				= 0x8,
		Unk4				= 0x9,
		Unk5				= 0xA,
		Shrine				= 0xB,
		Moongate			= 0xC,
		Unk6				= 0xD,
		GreenDot			= 0xE,
		GreenDotFlashing	= 0xF
	};

	PacketWaypointAdd(const CClient *target, CObjBase *object, WaypointType type);

	virtual bool canSendTo(const NetState *state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState *state)
	{
		return state->isClientKR() || state->isClientEnhanced();
	}
};

/***************************************************************************
*
*
*	Packet 0xE6 : PacketWaypointRemove		remove waypoint on KR/SA radar map (LOW)
*
*
***************************************************************************/
class PacketWaypointRemove : public PacketSend
{
public:
	PacketWaypointRemove(const CClient *target, CObjBase *object);

	virtual bool canSendTo(const NetState *state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState *state)
	{
		return state->isClientKR() || state->isClientEnhanced();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xEA : PacketToggleHotbar		toggle kr hotbar (NORMAL)
 *
 *
 ***************************************************************************/
class PacketToggleHotbar : public PacketSend
{
public:
	PacketToggleHotbar(const CClient* target, bool enable);

	virtual bool canSendTo(const NetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState* state)
	{
		return state->isClientKR();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xF2 : PacketTimeSyncResponse	time sync request (NORMAL)
 *
 *
 ***************************************************************************/
class PacketTimeSyncResponse : public PacketSend
{
public:
	PacketTimeSyncResponse(const CClient* target);

	virtual bool canSendTo(const NetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState* state)
	{
		return state->isClientVersion(MINCLIVER_SA) || state->isClientKR() || state->isClientEnhanced();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xF3 : PacketItemWorldNew		sends item on ground (NORMAL)
 *
 *
 ***************************************************************************/
class PacketItemWorldNew : public PacketItemWorld
{
protected:
	PacketItemWorldNew(BYTE id, size_t size, CGrayUID uid);

public:
	enum DataSource
	{
		TileData	= 0x0,
		Character	= 0x1,
		Multi		= 0x2,
		Damageable	= 0x3
	};

	PacketItemWorldNew(const CClient* target, CItem* item);
	PacketItemWorldNew(const CClient* target, CChar* mobile);

	virtual bool canSendTo(const NetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState* state)
	{
		return state->isClientVersion(MINCLIVER_SA) || state->isClientEnhanced();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xF5 : PacketDisplayMapNew		display map (LOW)
 *
 *
 ***************************************************************************/
class PacketDisplayMapNew : public PacketSend
{
public:
	PacketDisplayMapNew(const CClient* target, const CItemMap* map, const CRectMap& rect);

	virtual bool canSendTo(const NetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState* state)
	{
	    return state->isClientVersion(MINCLIVER_NEWMAPDISPLAY) || state->isClientEnhanced();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xF6 : PacketMoveShip			move ship (NORMAL)
 *
 *
 ***************************************************************************/
class PacketMoveShip : public PacketSend
{
public:
	PacketMoveShip(const CClient* target, const CItemShip* ship, CObjBase** objects, size_t objectCount, BYTE movedirection, BYTE boatdirection, BYTE speed);
};

/***************************************************************************
 *
 *
 *	Packet 0xF7 : PacketContainer			multiple packets (NORMAL)
 *
 *
 ***************************************************************************/
class PacketContainer : public PacketItemWorldNew// public PacketSend
	//friend PacketItemWorldNew
{
public:
	PacketContainer(const CClient* target, CObjBase** objects, size_t objectCount);

	virtual bool canSendTo(const NetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const NetState* state)
	{
		return state->isClientVersion(MINCLIVER_HS);
	}
};

#endif
