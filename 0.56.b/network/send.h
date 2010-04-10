#ifndef __SEND_H__
#define __SEND_H__
#pragma once

#include "packet.h"

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
	PacketGeneric(CClient* target, BYTE *data, long length);
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

	PacketTelnet(const CClient* target, LPCTSTR message);
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
	PacketWeb(CClient* target = NULL, BYTE* data = NULL, int length = 0);
	void setData(BYTE* data, int length);
};

/***************************************************************************
 *
 *
 *	Packet 0x0B : PacketCombatDamage		sends notification of got damage (LOW)
 *
 *
 ***************************************************************************/
class PacketCombatDamage : public PacketSend
{
public:
	PacketCombatDamage(CClient* target, DWORD damage, CGrayUID defender);
};

/***************************************************************************
 *
 *
 *	Packet 0x11 : PacketCharacterStatus		sends status window data (LOW)
 *
 *
 ***************************************************************************/
class PacketCharacterStatus : public PacketSend
{
public:
	PacketCharacterStatus(CClient* target, CChar* other);
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

	PacketHealthBarUpdate(CClient* target, const CChar* character);

	virtual bool onSend(const CClient* client);
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
	PacketItemWorld(BYTE id, long size, CGrayUID uid);

public:
	PacketItemWorld(CClient* target, CItem* item);

	void adjustItemData(const CClient* target, CItem* item, ITEMID_TYPE &id, HUE_TYPE &hue, long &amount, CPointMap &p, BYTE &dir, BYTE &flags, BYTE &light);

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
	PacketPlayerStart(CClient* target);
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
	PacketMessageASCII(CClient* target, LPCTSTR pszText, const CObjBaseTemplate* source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font);
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
	PacketRemoveObject(CClient* target, CGrayUID uid);
};

/***************************************************************************
 *
 *
 *	Packet 0x20 : PacketPlayerPosition		updates player position (NORMAL)
 *
 *
 ***************************************************************************/
class PacketPlayerPosition : public PacketSend
{
public:
	PacketPlayerPosition(CClient* target);
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
	PacketMovementRej(CClient* target, BYTE sequence);
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
	PacketMovementAck(CClient* target);
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
	PacketDragAnimation(CChar* source, CItem* item, CObjBase* container, CPointMap* pt);
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
	PacketContainerOpen(CClient* target, const CObjBase* container, GUMP_TYPE gump);

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
	PacketItemContainer(CClient* target, const CItem* item);
	PacketItemContainer(const CItem* spellbook, const CSpellDef* spell);

	void completeForTarget(CClient* target, const CItem* spellbook);
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
	PacketKick(CClient* target);
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

	PacketDragCancel(CClient* target, Reason code);
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
	PacketDropAccepted(CClient* target);
};

/***************************************************************************
 *
 *
 *	Packet 0x2E : PacketItemEquipped		sends equipped item  (NORMAL)
 *
 *
 ***************************************************************************/
class PacketItemEquipped : public PacketSend
{
public:
	PacketItemEquipped(CClient* target, const CItem* item);
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
	PacketSwing(CClient* target, const CChar* defender);
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
	PacketSkills(CClient* target, CChar* character, SKILL_TYPE skill);
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
	PacketCloseVendor(CClient* target, const CChar* vendor);
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
	int m_count;

public:
	PacketItemContents(CClient* target, const CItemContainer* container, bool isShop, bool filterLayers); // standard content
	PacketItemContents(CClient* target, const CItem* spellbook); // spellbook spells
	PacketItemContents(CClient* target, const CItemContainer* spellbook); // custom spellbook spells
	virtual bool onSend(const CClient* client);
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
	PacketGlobalLight(CClient* target, int light);
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
		BadPassword = 0x00,
		NoCharacter = 0x01,
		CharacterExists = 0x02,
		Other = 0x03,
		CharacterInWorld = 0x05,
		LoginProblem = 0x06,
		Idle = 0x07,
		CharacterTransfer = 0x09,
		InvalidName = 0x0A
	};

	PacketWarningMessage(CClient* target, Message code);
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
	PacketPlaySound(CClient* target, SOUND_TYPE sound, int flags, int volume, CPointMap pos);
};

/***************************************************************************
 *
 *
 *	Packet 0x55 : PacketRedrawAll			redraw all (NORMAL)
 *
 *
 ***************************************************************************/
class PacketRedrawAll : public PacketSend
{
public:
	PacketRedrawAll(CClient* target);
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
	PacketMapPlot(CClient* target, CItem* map, MAPCMD_TYPE mode, bool edit);
	PacketMapPlot(CItem* map, MAPCMD_TYPE mode, bool edit);

	void setPin(int x, int y);
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
	PacketGameTime(CClient* target, int hours = 0, int minutes = 0, int seconds = 0);
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
	PacketWeather(CClient* target, WEATHER_TYPE weather, int severity, int temperature);
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
	int m_pages;

public:
	PacketBookPageContent(CClient* target, const CItem* book, int startpage, int pagecount = 1);
	void addPage(const CItem* book, int page);
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

	PacketAddTarget(CClient* target, TargetType type, DWORD context, Flags flags);
	PacketAddTarget(CClient* target, TargetType type, DWORD context, Flags flags, ITEMID_TYPE id);
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
	PacketPlayMusic(CClient* target, WORD musicID);
};

/***************************************************************************
 *
 *
 *	Packet 0x6E : PacketAction				plays an animation (LOW)
 *
 *
 ***************************************************************************/
class PacketAction : public PacketSend
{
public:
	PacketAction(CChar* character, ANIM_TYPE action, WORD repeat, bool backward, BYTE delay);
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
	void prepareContainerOpen(const CChar* character, const CItem* container1, const CItem* container2);
	void prepareReadyChange(const CItemContainer* container1, const CItemContainer* container2);
	void prepareClose(const CItemContainer* container);
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
	PacketEffect(CClient* target, EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate* dst, const CObjBaseTemplate* src, BYTE speed, BYTE loop, bool explode);
	PacketEffect(CClient* target, EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate* dst, const CObjBaseTemplate* src, BYTE speed, BYTE loop, bool explode, DWORD hue, DWORD render);

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
	PacketBulletinBoard(CClient* target, const CItemContainer* board);
	PacketBulletinBoard(CClient* target, BBOARDF_TYPE action, const CItemContainer* board, const CItemMessage* message);
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
	PacketWarMode(CClient* target, const CChar* character);
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
	PacketPingAck(CClient* target, BYTE value = 0);
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
	PacketVendorBuyList(void);
	int fillContainer(const CItemContainer* container, int convertFactor, int maxItems = 115);
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
	PacketZoneChange(CClient* target, const CPointMap& pos);
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
	PacketCharacterMove(CClient* target, const CChar* character, BYTE direction);
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
	PacketDisplayMenu(CClient* target, CLIMODE_TYPE mode, const CMenuItem* items, int count, CObjBase* object);
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
		BlockedIP,      // ip is blocked
		MaxClients,     // max clients reached
		MaxGuests,      // max guests reached
		MaxPassTries,   // max password tries reached


		Success = 0xFF  // no error
	};

	PacketLoginError(CClient* target, Reason reason);
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
		BadPass      = 0x00, // incorrect password
		NotExist     = 0x01, // character does not exist
		InUse        = 0x02, // character is being played right now
		NotOldEnough = 0x03, // character is not old enough to delete

		Success      = 0xFF  // no error
	};

	PacketDeleteError(CClient* target, Reason reason);
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
	PacketCharacterListUpdate(CClient* target, const CChar* lastCharacter);
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
	PacketPaperdoll(CClient* target, const CChar* character);
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
	int m_count;

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
	PacketSignGump(CClient* target, const CObjBase* object, GUMP_TYPE gump, LPCTSTR unknown, LPCTSTR text);
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
public:
	PacketServerRelay(CClient* target, DWORD ip, WORD port, DWORD customerId);
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
	PacketDisplayMap(CClient* target, const CItemMap* map, const CRectMap& rect);
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
	PacketDisplayBook(CClient* target, CItem* book);
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
	PacketShowDyeWindow(CClient* target, const CObjBase* object);
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
	PacketAllNamesResponse(CClient* target, const CObjBase* object);
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
	PacketAddPrompt(CClient* target, CGrayUID context1, CGrayUID context2, bool useUnicode);
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
	PacketVendorSellList(const CChar* vendor);
	int searchContainer(CClient* target, const CItemContainer* container, CItemContainer* stock1, CItemContainer* stock2, int convertFactor, int maxItems = 115);
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
	PacketManaUpdate(const CChar* character);
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
	PacketStaminaUpdate(const CChar* character);
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
	PacketWebPage(CClient* target, LPCTSTR url);
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
	PacketOpenScroll(CClient* target, CResourceLock &s, SCROLL_TYPE type, DWORD context, LPCTSTR header);
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
	PacketServerList(CClient* target);
	void writeServerEntry(CServerRef server, int index, bool reverseIp);
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
	PacketCharacterList(CClient* target, const CChar* lastCharacter);
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
	PacketAttack(CClient* target, CGrayUID serial);
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
	PacketGumpValueInput(CClient* target, bool cancel, INPVAL_STYLE style, DWORD maxLength, LPCTSTR text, LPCTSTR caption, CObjBase* object);
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
	PacketMessageUNICODE(CClient* target, const NWORD* pszText, const CObjBaseTemplate* source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID language);
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
	PacketDeath(CChar* dead, CItemCorpse* corpse);
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
	void writeControls(CClient* target, const CGString* controls, int controlCount, const CGString* texts, int textCount);

protected:
	void writeCompressedControls(const CGString* controls, int controlCount, const CGString* texts, int textCount);
	void writeStandardControls(const CGString* controls, int controlCount, const CGString* texts, int textCount);
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
	PacketChatMessage(CClient* target, CHATMSG_TYPE type, LPCTSTR param1, LPCTSTR param2, CLanguageID language);
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
	PacketTooltip(CClient* target, const CObjBase* object, LPCTSTR text);
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
	PacketProfile(CClient* target, const CChar* character);
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
	PacketEnableFeatures(CClient* target, DWORD flags);
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
	PacketArrowQuest(CClient* target, int x, int y);
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
	PacketSeason(CClient* target, SEASON_TYPE season, bool playMusic);
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
	PacketExtended(EXTDATA_TYPE type, long len = 0, Priority priority = PRI_NORMAL);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x01 : PacketFastWalk		send fask walk keys (NORMAL)
 *
 *
 ***************************************************************************/
class PacketFastWalk : public PacketExtended
{
public:
	PacketFastWalk(CClient* target, DWORD* codes, int count, int sendCount);
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
	PacketGumpChange(CClient* target, DWORD context, int buttonId);
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
	PacketParty(PARTYMSG_TYPE type, long len = 0, Priority priority = PRI_NORMAL);
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
	PacketPartyInvite(CClient* target, const CChar* inviter);
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
	PacketMapChange(CClient* target, int map);
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
	bool m_isKr;
	int m_popupCount;

public:
	PacketDisplayPopup(CClient* target, CGrayUID uid);

	void addOption(WORD entryTag, WORD textId, WORD flags, WORD color);
	void finalise(void);

	int getOptionCount(void) const
	{
		return m_popupCount;
	}
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
	PacketEnableMapDiffs(CClient* target);
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
	PacketStatLocks(CClient* target, const CChar* character);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x1B : PacketSpellbookContent	spellbook content (NORMAL)
 *
 *
 ***************************************************************************/
class PacketSpellbookContent : public PacketExtended
{
public:
	PacketSpellbookContent(CClient* target, const CItem* spellbook, WORD offset);
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
	PacketHouseDesignVersion(CClient* target, const CItemMultiCustom* house);
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
	PacketHouseBeginCustomise(CClient* target, const CItemMultiCustom* house);
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
	PacketHouseEndCustomise(CClient* target, const CItemMultiCustom* house);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x22 : PacketCombatDamageOld		[old] sends notification of got damage (LOW)
 *
 *
 ***************************************************************************/
class PacketCombatDamageOld : public PacketExtended
{
public:
	PacketCombatDamageOld(CClient* target, DWORD damage, CGrayUID defender);
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
	PacketSpeedMode(CClient* target, BYTE mode);
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
	PacketMessageLocalised(CClient* target, int cliloc, const CObjBaseTemplate* source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font, TCHAR* args);
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
	PacketVisualRange(CClient* target, BYTE range);
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
	PacketMessageLocalisedEx(CClient* target, int cliloc, const CObjBaseTemplate* source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font, AFFIX_TYPE affixType, TCHAR* affix, TCHAR* args);
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
	PacketLogoutAck(CClient* target);
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
	long m_time;

public:
	PacketPropertyList(CClient* target, const CObjBase* object, DWORD hash, CGObArray<CClientTooltip*>* data);
	virtual bool onSend(const CClient* client);
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
	PacketBuff(CClient* target, const WORD iconId, const DWORD clilocOne, const DWORD clilocTwo, const short time, LPCTSTR* args, int argCount); // add buff
	PacketBuff(CClient* target, const WORD iconId); // remove buff
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
	PacketKREncryption(CClient* target);
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
	PacketToggleHotbar(CClient* target, bool enable);
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
public:
	enum DataSource
	{
		TileData = 0x0,
		Multi = 0x2
	};

	PacketItemWorldNew(CClient* target, CItem* item);
};

#endif
