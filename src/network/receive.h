#ifndef __RECEIVE_H__
#define __RECEIVE_H__
#pragma once

#include "packet.h"

/***************************************************************************
 *
 *
 *	Packet ???? : PacketUnknown						unknown or unhandled packet
 *
 *
 ***************************************************************************/
class PacketUnknown : public Packet
{
public:
	PacketUnknown(size_t size = 0);
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x00 : PacketCreate						create new character request
 *
 *
 ***************************************************************************/
class PacketCreate : public Packet
{
public:
	PacketCreate(size_t size = 104);
	virtual bool onReceive(NetState* net);

protected:
	bool onReceive(NetState* net, bool hasExtraSkill);
	bool doCreate(NetState* net, LPCTSTR charname, bool bFemale, RACE_TYPE rtRace, short wStr, short wDex, short wInt, PROFESSION_TYPE prProf, SKILL_TYPE skSkill1, BYTE iSkillVal1, SKILL_TYPE skSkill2, BYTE iSkillVal2, SKILL_TYPE skSkill3, BYTE iSkillVal3, SKILL_TYPE skSkill4, BYTE iSkillVal4, HUE_TYPE wSkinHue, ITEMID_TYPE idHair, HUE_TYPE wHairHue, ITEMID_TYPE idBeard, HUE_TYPE wBeardHue, HUE_TYPE wShirtHue, HUE_TYPE wPantsHue, ITEMID_TYPE idFace, int iStartLoc, DWORD iFlags);
	bool isValidHue(HUE_TYPE hue, const HUE_TYPE sm_Array[], size_t iArraySize);
};

/***************************************************************************
 *
 *
 *	Packet 0x02 : PacketMovementReq					movement request
 *
 *
 ***************************************************************************/
class PacketMovementReq : public Packet
{
public:
	PacketMovementReq(size_t size = 7);
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x03 : PacketSpeakReq					character talking
 *
 *
 ***************************************************************************/
class PacketSpeakReq : public Packet
{
public:
	PacketSpeakReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x05 : PacketAttackReq					attack request
 *
 *
 ***************************************************************************/
class PacketAttackReq : public Packet
{
public:
	PacketAttackReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x06 : PacketDoubleClick					double click object
 *
 *
 ***************************************************************************/
class PacketDoubleClick : public Packet
{
public:
	PacketDoubleClick();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x07 : PacketItemPickupReq				pick up item request
 *
 *
 ***************************************************************************/
class PacketItemPickupReq : public Packet
{
public:
	PacketItemPickupReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x08 : PacketItemDropReq					drop item request
 *
 *
 ***************************************************************************/
class PacketItemDropReq : public Packet
{
public:
	PacketItemDropReq();
	virtual size_t getExpectedLength(NetState* client, Packet* packet);
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x09 : PacketSingleClick					single click object
 *
 *
 ***************************************************************************/
class PacketSingleClick : public Packet
{
public:
	PacketSingleClick();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x12 : PacketTextCommand					text command
 *
 *
 ***************************************************************************/
class PacketTextCommand : public Packet
{
public:
	PacketTextCommand();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x13 : PacketItemEquipReq				item equip request
 *
 *
 ***************************************************************************/
class PacketItemEquipReq : public Packet
{
public:
	PacketItemEquipReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x22 : PacketResynchronize				resend all request
 *
 *
 ***************************************************************************/
class PacketResynchronize : public Packet
{
public:
	PacketResynchronize();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x2c : PacketDeathStatus					death status
 *
 *
 ***************************************************************************/
class PacketDeathStatus : public Packet
{
public:
	PacketDeathStatus();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x34 : PacketCharStatusReq				request information on the mobile
 *
 *
 ***************************************************************************/
class PacketCharStatusReq : public Packet
{
public:
	PacketCharStatusReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x3A : PacketSkillLockChange				change skill locks
 *
 *
 ***************************************************************************/
class PacketSkillLockChange : public Packet
{
public:
	PacketSkillLockChange();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x3B : PacketVendorBuyReq				buy item from vendor
 *
 *
 ***************************************************************************/
struct VendorItem
{
	CGrayUID m_serial;
	WORD m_amount;
	DWORD m_price;
};

class PacketVendorBuyReq : public Packet
{
public:
	PacketVendorBuyReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x3F : PacketStaticUpdate			Ultima live and (God Client?)
 *
 *
 ***************************************************************************/

class PacketStaticUpdate : public Packet
{
public:
	PacketStaticUpdate();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x56 : PacketMapEdit						edit map pins
 *
 *
 ***************************************************************************/
class PacketMapEdit : public Packet
{
public:
	PacketMapEdit();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x5D : PacketCharPlay					character select
 *
 *
 ***************************************************************************/
class PacketCharPlay : public Packet
{
public:
	PacketCharPlay();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x66 : PacketBookPageEdit				edit book page
 *
 *
 ***************************************************************************/
class PacketBookPageEdit : public Packet
{
public:
	PacketBookPageEdit();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x6C : PacketTarget						target object
 *
 *
 ***************************************************************************/
class PacketTarget : public Packet
{
public:
	PacketTarget();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x6F : PacketSecureTradeReq				trade with another character
 *
 *
 ***************************************************************************/
class PacketSecureTradeReq : public Packet
{
public:
	PacketSecureTradeReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x71 : PacketBulletinBoardReq			request bulletin board
 *
 *
 ***************************************************************************/
class PacketBulletinBoardReq : public Packet
{
public:
	PacketBulletinBoardReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x72 : PacketWarModeReq					toggle war mode
 *
 *
 ***************************************************************************/
class PacketWarModeReq : public Packet
{
public:
	PacketWarModeReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x73 : PacketPingReq						ping requests
 *
 *
 ***************************************************************************/
class PacketPingReq : public Packet
{
public:
	PacketPingReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x75 : PacketCharRename					rename character/pet
 *
 *
 ***************************************************************************/
class PacketCharRename : public Packet
{
public:
	PacketCharRename();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x7D : PacketMenuChoice					select menu option
 *
 *
 ***************************************************************************/
class PacketMenuChoice : public Packet
{
public:
	PacketMenuChoice();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x80 : PacketServersReq					request server list
 *
 *
 ***************************************************************************/
class PacketServersReq : public Packet
{
public:
	PacketServersReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x83 : PacketCharDelete					delete character
 *
 *
 ***************************************************************************/
class PacketCharDelete : public Packet
{
public:
	PacketCharDelete();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x8D : PacketCreateNew					create new character request (KR/SA)
 *
 *
 ***************************************************************************/
class PacketCreateNew : public PacketCreate
{
public:
	PacketCreateNew();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x91 : PacketCharListReq					request character list
 *
 *
 ***************************************************************************/
class PacketCharListReq : public Packet
{
public:
	PacketCharListReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x93 : PacketBookHeaderEdit				edit book header (title/author)
 *
 *
 ***************************************************************************/
class PacketBookHeaderEdit : public Packet
{
public:
	PacketBookHeaderEdit();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x95 : PacketDyeObject					colour selection dialog
 *
 *
 ***************************************************************************/
class PacketDyeObject : public Packet
{
public:
	PacketDyeObject();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x98 : PacketAllNamesReq					all names command (ctrl+shift)
 *
 *
 ***************************************************************************/
class PacketAllNamesReq : public Packet
{
public:
	PacketAllNamesReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x9A : PacketPromptResponse				prompt response (ascii)
 *
 *
 ***************************************************************************/
class PacketPromptResponse : public Packet
{
public:
	PacketPromptResponse();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x9B : PacketHelpPageReq					GM help page request
 *
 *
 ***************************************************************************/
class PacketHelpPageReq : public Packet
{
public:
	PacketHelpPageReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0x9F : PacketVendorSellReq				sell item to vendor
 *
 *
 ***************************************************************************/
class PacketVendorSellReq : public Packet
{
public:
	PacketVendorSellReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xA0 : PacketServerSelect				select server
 *
 *
 ***************************************************************************/
class PacketServerSelect : public Packet
{
public:
	PacketServerSelect();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xA4 : PacketSystemInfo					system info from client
 *
 *
 ***************************************************************************/
class PacketSystemInfo : public Packet
{
public:
	PacketSystemInfo();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xA7 : PacketTipReq						tip request
 *
 *
 ***************************************************************************/
class PacketTipReq : public Packet
{
public:
	PacketTipReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xAC : PacketGumpValueInputResponse		gump text input
 *
 *
 ***************************************************************************/
class PacketGumpValueInputResponse : public Packet
{
public:
	PacketGumpValueInputResponse();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xAD : PacketSpeakReqUNICODE				character talking (unicode)
 *
 *
 ***************************************************************************/
class PacketSpeakReqUNICODE : public Packet
{
public:
	PacketSpeakReqUNICODE();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xB1 : PacketGumpDialogRet				dialog button pressed
 *
 *
 ***************************************************************************/
class PacketGumpDialogRet : public Packet
{
public:
	PacketGumpDialogRet();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xB3 : PacketChatCommand					chat command
 *
 *
 ***************************************************************************/
class PacketChatCommand : public Packet
{
public:
	PacketChatCommand();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xB5 : PacketChatButton					chat button pressed
 *
 *
 ***************************************************************************/
class PacketChatButton : public Packet
{
public:
	PacketChatButton();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xB6 : PacketToolTipReq					tooltip requested
 *
 *
 ***************************************************************************/
class PacketToolTipReq : public Packet
{
public:
	PacketToolTipReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xB8 : PacketProfileReq					character profile requested
 *
 *
 ***************************************************************************/
class PacketProfileReq : public Packet
{
public:
	PacketProfileReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBB : PacketMailMessage					send mail message
 *
 *
 ***************************************************************************/
class PacketMailMessage : public Packet
{
public:
	PacketMailMessage();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBD : PacketClientVersion				client version
 *
 *
 ***************************************************************************/
class PacketClientVersion : public Packet
{
public:
	PacketClientVersion();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF : PacketExtendedCommand				extended command
 *
 *
 ***************************************************************************/
class PacketExtendedCommand : public Packet
{
public:
	PacketExtendedCommand();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x05 : PacketScreenSize				screen size report
 *
 *
 ***************************************************************************/
class PacketScreenSize : public Packet
{
public:
	PacketScreenSize();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x06 : PacketPartyMessage			party message
 *
 *
 ***************************************************************************/
class PacketPartyMessage : public Packet
{
public:
	PacketPartyMessage();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x07 : PacketArrowClick				click quest arrow
 *
 *
 ***************************************************************************/
class PacketArrowClick : public Packet
{
public:
	PacketArrowClick();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x09 : PacketWrestleDisarm			wrestle disarm macro
 *
 *
 ***************************************************************************/
class PacketWrestleDisarm : public Packet
{
public:
	PacketWrestleDisarm();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x0A : PacketWrestleStun			wrestle stun macro
 *
 *
 ***************************************************************************/
class PacketWrestleStun : public Packet
{
public:
	PacketWrestleStun();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x0B : PacketLanguage				language report
 *
 *
 ***************************************************************************/
class PacketLanguage : public Packet
{
public:
	PacketLanguage();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x0C : PacketStatusClose			status window closed
 *
 *
 ***************************************************************************/
class PacketStatusClose : public Packet
{
public:
	PacketStatusClose();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x0E : PacketAnimationReq			play an animation
 *
 *
 ***************************************************************************/
class PacketAnimationReq : public Packet
{
public:
	PacketAnimationReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x0F : PacketClientInfo				client information
 *
 *
 ***************************************************************************/
class PacketClientInfo : public Packet
{
public:
	PacketClientInfo();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x10 : PacketAosTooltipInfo			tooltip request (old)
 *
 *
 ***************************************************************************/
class PacketAosTooltipInfo : public Packet
{
public:
	PacketAosTooltipInfo();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x13 : PacketPopupReq				request popup menu
 *
 *
 ***************************************************************************/
class PacketPopupReq : public Packet
{
public:
	PacketPopupReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x15 : PacketPopupSelect			popup menu option selected
 *
 *
 ***************************************************************************/
class PacketPopupSelect : public Packet
{
public:
	PacketPopupSelect();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x1A : PacketChangeStatLock			set stat locks
 *
 *
 ***************************************************************************/
class PacketChangeStatLock : public Packet
{
public:
	PacketChangeStatLock();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x1C : PacketSpellSelect			select/cast spell
 *
 *
 ***************************************************************************/
class PacketSpellSelect : public Packet
{
public:
	PacketSpellSelect();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x1E : PacketHouseDesignReq			house design request
 *
 *
 ***************************************************************************/
class PacketHouseDesignReq : public Packet
{
public:
	PacketHouseDesignReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x24 : PacketAntiCheat				anti-cheat (unknown)
 *
 *
 ***************************************************************************/
class PacketAntiCheat : public Packet
{
public:
	PacketAntiCheat();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x2C : PacketBandageMacro			bandage macro
 *
 *
 ***************************************************************************/
class PacketBandageMacro : public Packet
{
public:
	PacketBandageMacro();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x2D : PacketTargetedSpell			use targeted spell
 *
 *
 ***************************************************************************/
class PacketTargetedSpell : public Packet
{
public:
	PacketTargetedSpell();
	virtual bool onReceive(NetState *net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x2E : PacketTargetedSkill			use targeted skill
 *
 *
 ***************************************************************************/
class PacketTargetedSkill : public Packet
{
public:
	PacketTargetedSkill();
	virtual bool onReceive(NetState *net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x30 : PacketTargetedResource		use targeted resource
 *
 *
 ***************************************************************************/
class PacketTargetedResource : public Packet
{
public:
	PacketTargetedResource();
	virtual bool onReceive(NetState *net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x32 : PacketGargoyleFly			gargoyle toggle flying
 *
 *
 ***************************************************************************/
class PacketGargoyleFly : public Packet
{
public:
	PacketGargoyleFly();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x33 : PacketWheelBoatMove			wheel boat movement
 *
 *
 ***************************************************************************/
class PacketWheelBoatMove : public Packet
{
public:
	PacketWheelBoatMove();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xC2 : PacketPromptResponseUnicode		prompt response (unicode)
 *
 *
 ***************************************************************************/
class PacketPromptResponseUnicode : public Packet
{
public:
	PacketPromptResponseUnicode();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xC8 : PacketViewRange					change view range
 *
 *
 ***************************************************************************/
class PacketViewRange : public Packet
{
public:
	PacketViewRange();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD1 : PacketLogout						client logout notification
 *
 *
 ***************************************************************************/
class PacketLogout : public Packet
{
public:
	PacketLogout();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD4 : PacketBookHeaderEditNew		edit book header (title/author)
 *
 *
 ***************************************************************************/
class PacketBookHeaderEditNew : public Packet
{
public:
	PacketBookHeaderEditNew();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD6 : PacketAOSTooltipReq				aos tooltip request
 *
 *
 ***************************************************************************/
class PacketAOSTooltipReq : public Packet
{
public:
	PacketAOSTooltipReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD7 : PacketEncodedCommand				encoded command
 *
 *
 ***************************************************************************/
class PacketEncodedCommand : public Packet
{
public:
	PacketEncodedCommand();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD7.0x02 : PacketHouseDesignBackup		backup house design
 *
 *
 ***************************************************************************/
class PacketHouseDesignBackup : public Packet
{
public:
	PacketHouseDesignBackup();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD7.0x03 : PacketHouseDesignRestore		restore house design
 *
 *
 ***************************************************************************/
class PacketHouseDesignRestore : public Packet
{
public:
	PacketHouseDesignRestore();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD7.0x04 : PacketHouseDesignCommit		commit house design
 *
 *
 ***************************************************************************/
class PacketHouseDesignCommit : public Packet
{
public:
	PacketHouseDesignCommit();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD7.0x05 : PacketHouseDesignDestroyItem	destroy house design item
 *
 *
 ***************************************************************************/
class PacketHouseDesignDestroyItem : public Packet
{
public:
	PacketHouseDesignDestroyItem();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD7.0x06 : PacketHouseDesignPlaceItem	place house design item
 *
 *
 ***************************************************************************/
class PacketHouseDesignPlaceItem : public Packet
{
public:
	PacketHouseDesignPlaceItem();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD7.0x0C : PacketHouseDesignExit		exit house designer
 *
 *
 ***************************************************************************/
class PacketHouseDesignExit : public Packet
{
public:
	PacketHouseDesignExit();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD7.0x0D : PacketHouseDesignPlaceStair	place house design stairs
 *
 *
 ***************************************************************************/
class PacketHouseDesignPlaceStair : public Packet
{
public:
	PacketHouseDesignPlaceStair();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD7.0x0E : PacketHouseDesignSync		synchronise house design
 *
 *
 ***************************************************************************/
class PacketHouseDesignSync : public Packet
{
public:
	PacketHouseDesignSync();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD7.0x10 : PacketHouseDesignClear		clear house design
 *
 *
 ***************************************************************************/
class PacketHouseDesignClear : public Packet
{
public:
	PacketHouseDesignClear();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD7.0x12 : PacketHouseDesignSwitch		switch house design floor
 *
 *
 ***************************************************************************/
class PacketHouseDesignSwitch : public Packet
{
public:
	PacketHouseDesignSwitch();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD7.0x13 : PacketHouseDesignPlaceRoof	place house design roof
 *
 *
 ***************************************************************************/
class PacketHouseDesignPlaceRoof : public Packet
{
public:
	PacketHouseDesignPlaceRoof();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD7.0x14 : PacketHouseDesignDestroyRoof	destroy house design roof
 *
 *
 ***************************************************************************/
class PacketHouseDesignDestroyRoof : public Packet
{
public:
	PacketHouseDesignDestroyRoof();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD7.0x19 : PacketSpecialMove			perform special move
 *
 *
 ***************************************************************************/
class PacketSpecialMove : public Packet
{
public:
	PacketSpecialMove();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD7.0x1A : PacketHouseDesignRevert		revert house design
 *
 *
 ***************************************************************************/
class PacketHouseDesignRevert : public Packet
{
public:
	PacketHouseDesignRevert();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD7.0x1E : PacketEquipLastWeapon		equip last weapon macro
 *
 *
 ***************************************************************************/
class PacketEquipLastWeapon : public Packet
{
public:
	PacketEquipLastWeapon();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD7.0x28 : PacketGuildButton			guild button pressed
 *
 *
 ***************************************************************************/
class PacketGuildButton : public Packet
{
public:
	PacketGuildButton();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD7.0x32 : PacketQuestButton			quest button pressed
 *
 *
 ***************************************************************************/
class PacketQuestButton : public Packet
{
public:
	PacketQuestButton();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xD9 : PacketHardwareInfo				hardware info from client
 *
 *
 ***************************************************************************/
class PacketHardwareInfo : public Packet
{
public:
	PacketHardwareInfo();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xE0 : PacketBugReport					bug report
 *
 *
 ***************************************************************************/
class PacketBugReport : public Packet
{
public:
	PacketBugReport();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xE1 : PacketClientType					client type (KR/SA)
 *
 *
 ***************************************************************************/
class PacketClientType : public Packet
{
public:
	PacketClientType();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xE8 : PacketRemoveUIHighlight			remove ui highlight
 *
 *
 ***************************************************************************/
class PacketRemoveUIHighlight : public Packet
{
public:
	PacketRemoveUIHighlight();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xEB : PacketUseHotbar					use hotbar
 *
 *
 ***************************************************************************/
class PacketUseHotbar : public Packet
{
public:
	PacketUseHotbar();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xEC : PacketEquipItemMacro				equip item(s) macro (KR)
 *
 *
 ***************************************************************************/
class PacketEquipItemMacro : public Packet
{
public:
	PacketEquipItemMacro();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xED : PacketUnEquipItemMacro			unequip item(s) macro (KR)
 *
 *
 ***************************************************************************/
class PacketUnEquipItemMacro : public Packet
{
public:
	PacketUnEquipItemMacro();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xF0 : PacketMovementReqNew				movement request (KR/SA)
 *
 *
 ***************************************************************************/
class PacketMovementReqNew : public Packet
{
public:
	PacketMovementReqNew();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xF1 : PacketTimeSyncRequest				time sync request (KR/SA)
 *
 *
 ***************************************************************************/
class PacketTimeSyncRequest : public Packet
{
public:
	PacketTimeSyncRequest();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xF4 : PacketCrashReport					crash report
 *
 *
 ***************************************************************************/
class PacketCrashReport : public Packet
{
public:
	PacketCrashReport();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xF8 : PacketCreateHS					create new character request (HS)
 *
 *
 ***************************************************************************/
class PacketCreateHS : public PacketCreate
{
public:
	PacketCreateHS();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
 *
 *
 *	Packet 0xF9 : PacketGlobalChatReq				global chat (INCOMPLETE)
 *
 *
 ***************************************************************************/
class PacketGlobalChatReq : public Packet
{
public:
	PacketGlobalChatReq();
	virtual bool onReceive(NetState* net);
};

/***************************************************************************
*
*
*	Packet 0xFA : PacketUltimaStoreButton			ultima store button pressed
*
*
***************************************************************************/
class PacketUltimaStoreButton : public Packet
{
public:
	PacketUltimaStoreButton();
	virtual bool onReceive(NetState* net);
};

#endif
