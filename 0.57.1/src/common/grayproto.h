#ifndef _INC_GRAYPROTO_H
#define _INC_GRAYPROTO_H
#pragma once

//---------------------------PROTOCOL DEFS---------------------------

// All these structures must be BYTE packed.
#if defined _WIN32
// Microsoft dependant pragma
#pragma pack(1)
#define PACK_NEEDED
#else
// GCC based compiler you can add:
#define PACK_NEEDED __attribute__ ((packed))
#endif

// Pack/unpack in network order.

struct NWORD
{
	// Deal with the stupid Little Endian / Big Endian crap just once.
	// On little endian machines this code would do nothing.
	WORD m_val;
	operator WORD () const
	{
		return( ntohs( m_val ));
	}
	NWORD & operator = ( WORD val )
	{
		m_val = htons( val );
		return( * this );
	}

#define PACKWORD(p,w)	(p)[0]=HIBYTE(w);(p)[1]=LOBYTE(w)
#define UNPACKWORD(p)	MAKEWORD((p)[1],(p)[0])	// low,high

} PACK_NEEDED;
struct NDWORD
{
	DWORD m_val;
	operator DWORD () const
	{
		return( ntohl( m_val ));
	}
	NDWORD & operator = ( DWORD val )
	{
		m_val = htonl( val );
		return( * this );
	}

#define PACKDWORD(p,d)	(p)[0]=((d)>>24)&0xFF;(p)[1]=((d)>>16)&0xFF;(p)[2]=HIBYTE(d);(p)[3]=LOBYTE(d)
#define UNPACKDWORD(p)	MAKEDWORD( MAKEWORD((p)[3],(p)[2]), MAKEWORD((p)[1],(p)[0]))

} PACK_NEEDED;

#define	NCHAR	NWORD			// a UNICODE text char on the network.

extern int CvtSystemToNUNICODE( NCHAR * pOut, int iSizeOutChars, LPCTSTR pInp, int iSizeInBytes );
extern int CvtNUNICODEToSystem( TCHAR * pOut, int iSizeOutBytes, const NCHAR * pInp, int iSizeInChars );

class CLanguageID
{
private:
	// 3 letter code for language.
	// ENU,FRA,DEU,etc. (see langcode.iff)
	// terminate with a 0
	// 0 = english default.
	char m_codes[4]; // UNICODE language pref. ('ENU'=english)
public:
	CLanguageID()
	{
		m_codes[0] = 0;
	}
	CLanguageID( const char * pszInit )
	{
		Set( pszInit );
	}
	CLanguageID( int iDefault )
	{
		m_codes[0] = 0;
	}
	bool IsDef() const
	{
		return( m_codes[0] != 0 );
	}
	void GetStrDef( TCHAR * pszLang )
	{
		if ( ! IsDef())
		{
			strcpy( pszLang, "enu" );
		}
		else
		{
			memcpy( pszLang, m_codes, 3 );
			pszLang[3] = '\0';
		}
	}
	void GetStr( TCHAR * pszLang ) const
	{
		memcpy( pszLang, m_codes, 3 );
		pszLang[3] = '\0';
	}
	LPCTSTR GetStr() const
	{
		TEMPSTRING(pszTmp);
		GetStr(pszTmp);
		return pszTmp;
	}
	bool Set( LPCTSTR pszLang )
	{
		// needs not be terminated!
		if ( pszLang != NULL )
		{
			memcpy( m_codes, pszLang, 3 );
			m_codes[3] = 0;
			if ( isalnum(m_codes[0]))
				return true;
			// not valid !
		}
		m_codes[0] = 0;
		return false;
	}
};

enum XCMD_TYPE	// XCMD_* messages are unique in both directions.
{
	//	0x00
	XCMD_Create			= 0x00,	// in
	XCMD_Talk			= 0x03,	// in
	XCMD_Attack			= 0x05,	// in
	XCMD_DClick			= 0x06,	// in
	XCMD_ItemPickupReq	= 0x07,	// in
	XCMD_ItemDropReq	= 0x08,	// in
	XCMD_Click			= 0x09,	// in
	//	0x10
	XCMD_Status			= 0x11,	// out
	XCMD_ExtCmd			= 0x12,	// in
	XCMD_Start			= 0x1b,	// out
	XCMD_Speak			= 0x1c,	// out
	//	0x20
	XCMD_View			= 0x20,	// out
	XCMD_DragAnim		= 0x23,	// out
	XCMD_ContOpen		= 0x24,	// out
	XCMD_ContAdd		= 0x25,	// out
	XCMD_DragCancel		= 0x27,	// out
	XCMD_ItemEquip		= 0x2e,	// out
	//	0x30
	XCMD_Skill			= 0x3a,	// in/out
	XCMD_Content		= 0x3c,	// out
	//	0x50
	XCMD_IdleWarning	= 0x53,	// out
	XCMD_Sound			= 0x54,	// out
	XCMD_ReDrawAll		= 0x55,	// out
	XCMD_MapEdit		= 0x56,	// in/out
	XCMD_Time			= 0x5b,	// out
	XCMD_CharPlay		= 0x5d,	// out
	//	0x60
	XCMD_Weather		= 0x65,	// out
	XCMD_BookPage		= 0x66,	// in/out
	XCMD_Target			= 0x6c,	// in/out
	XCMD_CharAction		= 0x6e,	// out
	XCMD_SecureTrade	= 0x6f,	// in/out
	//	0x70
	XCMD_Effect			= 0x70,	// out
	XCMD_BBoard			= 0x71,	// in/out
	XCMD_War			= 0x72,	// in/out
	XCMD_CharName		= 0x75,	// in
	XCMD_CharMove		= 0x77,	// out
	XCMD_Char			= 0x78,	// out
	XCMD_MenuItems		= 0x7c,	// out
	XCMD_MenuChoice		= 0x7d,	// in
	//	0x80
	XCMD_ServersReq		= 0x80,	// in
	XCMD_CharList3		= 0x81,	// out
	XCMD_CharDelete		= 0x83,	// in
	XCMD_DeleteBad		= 0x85,	// out
	XCMD_CharList2		= 0x86,	// out
	XCMD_PaperDoll		= 0x88,	// out
	XCMD_CorpEquip		= 0x89,	// out
	XCMD_Relay			= 0x8c,	// out
	//	0x90
	XCMD_MapDisplay		= 0x90,	// out
	XCMD_CharListReq	= 0x91,	// in
	XCMD_BookOpen		= 0x93,	// out
	XCMD_DyeVat			= 0x95,	// in/out
	XCMD_AllNames3D		= 0x98,	// in/out
	XCMD_TargetMulti	= 0x99,	// out
	XCMD_Prompt			= 0x9a,	// in/out
	XCMD_HelpPage		= 0x9b,	// in
	//	0xA0
	XCMD_ServerSelect	= 0xa0,	// in
	XCMD_StatChngStr	= 0xa1,	// out
	XCMD_StatChngInt	= 0xa2,	// out
	XCMD_StatChngDex	= 0xa3,	// out
	XCMD_Web			= 0xa5,	// out
	XCMD_ServerList		= 0xa8,	// out
	XCMD_CharList		= 0xa9,	// out
	XCMD_AttackOK		= 0xaa,	// out
	XCMD_GumpInpVal		= 0xab,	// out
	XCMD_GumpInpValRet	= 0xac,	// in
	XCMD_TalkUNICODE	= 0xad,	// in
	XCMD_SpeakUNICODE	= 0xae,	// out
	//	0xB0
	XCMD_GumpDialog		= 0xb0,	// out
	XCMD_GumpDialogRet	= 0xb1,	// in
	XCMD_ChatReq		= 0xb2,	// in
	XCMD_ChatText		= 0xb3,	// ?/out
	XCMD_Chat			= 0xb5,	// in/out
	XCMD_CharProfile	= 0xb8,	// in/out
	XCMD_Features		= 0xb9,	// out
	XCMD_Arrow			= 0xba,	// out
	XCMD_MailMsg		= 0xbb,	// in/out
	XCMD_Season			= 0xbc,	// out
	XCMD_ClientVersion	= 0xbd,	// in/out
	XCMD_ExtData		= 0xbf,	// in/out
	//	0xC0
	XCMD_EffectEx		= 0xc0,	// out
	XCMD_SpeakLocalized	= 0xc1,
	XCMD_TextentryUnicode =0xc2,// in/out
	XCMD_Semivisible	= 0xc4,	// out
	XCMD_EffectParticle	= 0xc7,	// out
	XCMD_ViewRange		= 0xc8,	// in/out
	//	0xD0
	XCMD_ConfigFile		= 0xd0,	// in/out
	XCMD_AOSBookPage	= 0xd4,	// in/out
	XCMD_AOSTooltip		= 0xd6, // out
	XCMD_ExtAosData		= 0xd7, // in/out
	XCMD_AOSCustomHouse	= 0xd8, // out
	XCMD_Majong			= 0xda, // in/out
	XCMD_AOSTooltipInfo	= 0xdc,
	XCMD_CompressedGumpDialog = 0xdd,
	
	XCMD_QTY		= 0xde,
};

enum PARTYMSG_TYPE
{
	PARTYMSG_Add = 1,	// (from client) Invite someone to join my party. or (from server) list who is in the party.
	PARTYMSG_Remove = 2,	// (to/from client) request to kick someone or notify that someone has been kicked.
	PARTYMSG_MsgMember = 3,	// (from client ) send a msg to a single member.
	PARTYMSG_Msg = 4,		// (to/from client) echoes
	PARTYMSG_Disband,
	PARTYMSG_Option = 6,	// (from client) loot flag.
	PARTYMSG_NotoInvited = 7,	// (to client) I've been invited to join another party.
	PARTYMSG_Accept = 8,	// (from client) first
	PARTYMSG_Decline,
	PARTYMSG_QTY,
};

enum EXTDATA_TYPE
{
	EXTDATA_WalkCode_Prime	= 0x01,	// send to client
	EXTDATA_WalkCode_Add	= 0x02,	// send to client
	EXTDATA_Unk3,
	EXTDATA_ScreenSize		= 0x05,	// len=8 "00 00 02 80 00 00 00 0a"
	EXTDATA_Party_Msg		= 0x06,	// len=5 data for total of 10. Client wants to add to the party.
	EXTDATA_Arrow_Click		= 0x07,	// Client clicked on the quest arrow.
	EXTDATA_Map_Change		= 0x08, // send to the client (? not sure)
	EXTDATA_Wrestle_DisArm  = 0x09,	// From Client: Wrestling disarm
	EXTDATA_Wrestle_Stun    = 0x0a,	// From Client: Wrestling stun
	EXTDATA_Lang			= 0x0b,	// len=3 = my language. ENU CLanguageID
	EXTDATA_StatusClose		= 0x0c, // 12= closing a status window.
	EXTDATA_Unk13,
	EXTDATA_Yawn			= 0x0e, // Yawn animation
	EXTDATA_Unk15			= 0x0f,	// Unknown, sent at login
	EXTDATA_OldAOSTooltipInfo	= 0x10, // Equip info
	//
	//
	EXTDATA_Popup_Request	= 0x13, // client message
	EXTDATA_Popup_Display	= 0x14,	// server message
	EXTDATA_Popup_Select	= 0x15,	// client message
	//
	EXTDATA_Codex_Wisdom	= 0x17,	// server message
	EXTDATA_Map_Diff		= 0x18,	// enable mapdiff files
	EXTDATA_Stats_Enable	= 0x19,	// extended stats
	EXTDATA_Stats_Change	= 0x1a,	// extended stats
	EXTDATA_NewSpellbook	= 0x1b,
	EXTDATA_NewSpellSelect	= 0x1c,
	EXTDATA_HouseDesignVer	= 0x1d,	// server
	EXTDATA_HouseDesignDet	= 0x1e, // client
	
	EXTDATA_HouseCustom		= 0x20,
	EXTDATA_Ability_Confirm	= 0x21,	// server (empty packet only id is required)
	
	EXTDATA_Unk23			= 0x23,
	EXTDATA_Unk24			= 0x24, // Sent by SE clients, every second or so.
	EXTDATA_SpellSE			= 0x25,
	EXTDATA_SpeedMode		= 0x26,





	EXTDATA_BandageMacro	= 0x2c,

	EXTDATA_QTY,
};

enum EXTAOS_TYPE
{
	//
	//
	EXTAOS_HcBackup = 0x02, // House Customization :: Backup
    EXTAOS_HcRestore = 0x03, // House Customization :: Restore
	EXTAOS_HcCommit = 0x04, // House Customization :: Commit
	EXTAOS_HcDestroyItem = 0x05, // House Customization :: Destroy Item
	EXTAOS_HcPlaceItem = 0x06, // House Customization :: Place Item
	//
	//
	//
	//
	//
	EXTAOS_HcExit = 0x0C, // House Customization :: Exit
	EXTAOS_HcPlaceStair = 0x0D, // House Customization :: Place Multi (Stairs)
	EXTAOS_HcSynch = 0x0E, // House Customization :: Synch
	//
	EXTAOS_HcClear = 0x10, // House Customization :: Clear
	//
	EXTAOS_HcSwitch = 0x12, // House Customization :: Switch Floors
	EXTAOS_HcFloorDesign = 0x13, // House Customization :: Floor Design
	EXTAOS_HcFloorDelete = 0x14, // House Customization :: Floor Delete
	//
	//
	//
	//
	EXTAOS_SpecialMove = 0x19, // Special Moves :: Activate/Deactivate
	EXTAOS_HcRevert = 0x1A, // House Customization :: Revert
	//
	//
	//
	EXTAOS_EquipLastWeapon = 0x1E, // Equip Last Weapon
	//
	// 0x20
	//
	//
	//
	//
	//
	//
	//
	EXTAOS_GuildButton = 0x28,	// Guild Button
	//
	//
	//
	//
	//
	//
	// 0x30
	//
	EXTAOS_QuestButton = 0x32,	// Quest Button

	EXTAOS_QTY,
};

#define MAX_TALK_BUFFER		1024	// how many chars can anyone speak all at once?

union CExtData
{
	BYTE Default[1];			// Unknown stuff.
	NDWORD WalkCode[16];	// EXTDATA_WalkCode_Prime or EXTDATA_WalkCode_Add

	struct
	{
		BYTE m_code;	// PARTYMSG_TYPE
		BYTE m_data[32];
	} Party_Msg_Opt;	// from client.

	struct
	{
		BYTE m_code;	// PARTYMSG_TYPE
		NDWORD m_UID;		// source/dest uid.
		NCHAR m_msg[MAX_TALK_BUFFER];	// text of the msg. var len
	} Party_Msg_Rsp;	// from server.

	struct
	{
		BYTE m_code;	// PARTYMSG_TYPE
		BYTE m_Qty;
		NDWORD m_uids[3];
	} Party_Msg_Data;	// from client.

	struct // EXTDATA_Map_Change = 0x08
	{
		BYTE m_state;	// 1 = on
	} MapChange;

	struct	// EXTDATA_ScreenSize = 0x05
	{
		NDWORD m_x;
		NDWORD m_y;
	} ScreenSize;	// from client

	struct	// EXTDATA_Lang = 0x0b
	{
		char m_code[3];	// CLanguageID
	} Lang;	// from client

	struct // EXTDATA_OldAOSTooltipInfo	= 0x10 (server version)
	{
		NDWORD m_uid;
		NDWORD m_ListID;
	} OldAOSTooltipInfo;

	struct // EXTDATA_OldAOSTooltipInfo = 0x10 (client version)
	{
		NDWORD m_uid;
	} OldAOSTooltipRequest;

	struct  // EXTDATA_Popup_Request = 0x13
	{
		NDWORD m_UID;
	} Popup_Request; // from client

	struct  // EXTDATA_Popup_Display = 0x14
	{
		NWORD m_unk1; // 1
		NDWORD m_UID;
		BYTE m_NumPopups;
		struct
		{
			NWORD m_EntryTag;
			NWORD m_TextID;
			NWORD m_Flags;
			NWORD m_Color;
		} m_List[1];
	} Popup_Display;

	struct  // EXTDATA_Popup_Select = 0x15
	{
		NDWORD m_UID;
		NWORD m_EntryTag;
	} Popup_Select;
	
	struct // EXTDATA_Codex_Wisdom = 0x17
	{
		BYTE m_unk_1; // 0x01 always
		NDWORD m_messageid;
		BYTE m_presentation; // 0 = Flash, 1 = Direct
	} Codex_Wisdom;
	
	struct // EXTDATA_Map_Diff = 0x18
	{
		NDWORD m_maps_number;
		struct
		{
			NDWORD m_map_patch;
			NDWORD m_static_patch;
		} m_patch[1];
	} Map_Diff;

	struct // EXTDATA_Stats_Enable = 0x19 type 0
	{
		BYTE m_type; // 0x00
		NDWORD m_UID; // (of Bonded Pet?)
		BYTE m_unk; // 0x01
	} Bonded_Status;

	struct // EXTDATA_Stats_Enable = 0x19 type 2
	{
		BYTE m_type; // 0x02
		NDWORD m_UID;
		BYTE m_unk; // 0x00
		BYTE m_lockbits; // Bits: XXSSDDII (s=strength, d=dex, i=int), 0 = up, 1 = down, 2 = locked
	} Stats_Enable;
	
	struct // EXTDATA_Stats_Change = 0x1A
	{
		BYTE m_stat;
		BYTE m_status;
	} Stats_Change;
	
	struct  // EXTDATA_NewSpellbook = 0x1b
	{
		NWORD m_Unk1;
		NDWORD m_UID;
		NWORD m_ItemId;
		NWORD m_Offset;
		DWORD m_Content[2];
	} NewSpellbook;

	struct  // EXTDATA_NewSpellSelect = 0x1C
	{
		NWORD m_Unk1;
		NWORD m_SpellId;
	} NewSpellSelect;

	struct  // EXTDATA_NewSpellSelectWyatt = 0x1C (version as in wyatt docs)
	{
		NWORD m_HasSpell; // 2=no spell, 1=has spellbook, 0=no spellbook but has spell
		NDWORD m_SpellBook; // if 1 spellbook uid
		NWORD m_Expansionflag; // 0x0 = LBR&AOS; 0x1 = SE; 0x2 = ML;
		NWORD m_SpellId; // 0x00 if last spell
	} NewSpellSelectWyatt;

	struct // EXTDATA_HouseDesignVer = 0x1d
	{
		NDWORD m_HouseUID;
		NDWORD m_Version;
	} HouseDesignVersion;

	struct // EXTDATA_HouseDesignDet = 0x1e
	{
		NDWORD m_HouseUID;
	} HouseDesignDetail;

	struct // EXTDATA_HouseCustom = 0x20
	{
		NDWORD m_HouseSerial;
		BYTE m_type; // 0x04 = start; 0x05 = end;
		NWORD m_unk1; // 0x0000
		NWORD m_unk2; // 0xFFFF
		NWORD m_unk3; // 0xFFFF
		BYTE  m_unk4; // 0xFF
	} HouseCustom;

	struct	// EXTDATA_Unk24
	{
		BYTE m_unk; // variable
	} Unk24; // from client

	struct // EXTDATA_SpellSE = 0x25
	{
		BYTE m_unk; // 0x01
		BYTE m_SpellID;
		BYTE m_Enable; // 0x00/0x01
	} SpellSE;

	struct // EXTDATA_SpeedMode	= 0x26
	{
		BYTE m_speed; // 0x0 = Normal movement, 0x1 = Fast movement, 0x2 = Slow movement, 0x3 and above = Hybrid movement
	} SpeedMode;

	struct // EXTDATA_BandageMacro = 0x2c
	{
		NDWORD m_bandageSerial;
		NDWORD m_targetSerial;
	} BandageMacro;

} PACK_NEEDED;

union CExtAosData
{
	struct
	{
		BYTE m_Unk; // 0x0A
		
	} HouseBackup;
	
	struct
	{
		BYTE m_Unk; // 0x0A
		
	} HouseRestore;

	struct
	{
		BYTE m_Unk; // 0x0A
		
	} HouseCommit;
	
	struct
	{
		BYTE m_Unk_00_1; // 0x00
		NDWORD m_Dispid;
		BYTE m_Unk_00_2; // 0x00
		NDWORD m_PosX;
		BYTE m_Unk_00_3; // 0x00
		NDWORD m_PosY;
		BYTE m_Unk_00_4; // 0x00
		NDWORD m_PosZ;
		BYTE m_Unk; 	 // 0x0A
		
	} HouseDestroyItem;
	
	struct
	{
		BYTE m_Unk_00_1; // 0x00
		NDWORD m_Dispid;
		BYTE m_Unk_00_2; // 0x00
		NDWORD m_PosX;
		BYTE m_Unk_00_3; // 0x00
		NDWORD m_PosY;
		BYTE m_Unk; 	 // 0x0A
		
	} HousePlaceItem;

	struct
	{
		BYTE m_Unk; // 0x0A
		
	} HouseExit;

	struct
	{
		BYTE m_Unk_00_1; // 0x00
		NDWORD m_Dispid;
		BYTE m_Unk_00_2; // 0x00
		NDWORD m_PosX;
		BYTE m_Unk_00_3; // 0x00
		NDWORD m_PosY;
		BYTE m_Unk; 	 // 0x0A
		
	} HousePlaceStair;

	struct
	{
		BYTE m_Unk; // 0x0A
		
	} HouseSynch;
	
	struct
	{
		BYTE m_Unk; // 0x0A
		
	} HouseClear;

	struct
	{
		BYTE m_Unk_00_1; // 0x00
		NDWORD m_Floor;
		BYTE m_Unk; 	 // 0x0A
		
	} HouseSwitchFloor;

	struct
	{
		NDWORD m_Unk_0;
		BYTE m_Ability;
		BYTE m_Unk; 	// 0x0A
		
	} SpecialMove;
	
	struct
	{
		BYTE m_Unk; 	 // 0x0A
		
	} HouseRevert;

	struct
	{
		BYTE m_Unk;		// 0x0A
	} EquipLastWeapon;
	
	struct
	{
		BYTE m_Unk; // 0x0A
	} GuildButton;
};

enum SECURE_TRADE_TYPE
{
	// SecureTrade Action types.
	SECURE_TRADE_OPEN = 0,
	SECURE_TRADE_CLOSE = 1,
	SECURE_TRADE_CHANGE = 2,
};

enum SEASON_TYPE
{
	// The seasons can be:
	SEASON_Spring = 0,
	SEASON_Summer,		// 1
	SEASON_Fall,		// 2
	SEASON_Winter,		// 3
	SEASON_Desolate,	// 4 = (Felucca) undead
	SEASON_Nice,		// 5 = (Trammal) summer ?
	SEASON_QTY,
};

enum BBOARDF_TYPE	// Bulletin Board Flags. m_flag
{
	BBOARDF_NAME = 0,	// board name
	BBOARDF_MSG_HEAD,	// 1=message header, 
	BBOARDF_MSG_BODY,	// 2=message body
	BBOARDF_REQ_FULL,	// 3=request for full msg.
	BBOARDF_REQ_HEAD,	// 4=request for just head.
	BBOARDF_NEW_MSG,	// 5=new message, 
	BBOARDF_DELETE,		// 6=Delete
};

enum EXTCMD_TYPE
{
	EXTCMD_OPEN_SPELLBOOK	= 0x43,	// 67 = open spell book if we have one.
	EXTCMD_ANIMATE			= 0xC7,	// "bow" or "salute"
	EXTCMD_SKILL			= 0x24,	// Skill start "number of the skill"
	EXTCMD_AUTOTARG			= 47,	// bizarre new autotarget mode. "target x y z"
	EXTCMD_CAST_MACRO		= 86,	// macro spell. "spell number"
	EXTCMD_CAST_BOOK		= 39,	// cast spell from book. "spell number"
	EXTCMD_DOOR_AUTO		= 88,	// open door macro = Attempt to open a door around us.
	EXTCMD_UNKGODCMD		= 107,  // Unknow god command
};

enum CHATMSG_TYPE	// Chat system messages.
{
	CHATMSG_NoError = -1,				// -1 = Our return code for no error, but no message either
	CHATMSG_Error = 0,					// 0 - Error
	CHATMSG_AlreadyIgnoringMax,			// 1 - You are already ignoring the maximum amount of people
	CHATMSG_AlreadyIgnoringPlayer,		// 2 - You are already ignoring <name>
	CHATMSG_NowIgnoring,				// 3 - You are now ignoring <name>
	CHATMSG_NoLongerIgnoring,			// 4 - You no longer ignoring <name>
	CHATMSG_NotIgnoring,				// 5 - You are not ignoring <name>
	CHATMSG_NoLongerIgnoringAnyone,		// 6 - You are no longer ignoring anyone
	CHATMSG_InvalidConferenceName,		// 7 - That is not a valid conference name
	CHATMSG_AlreadyAConference,			// 8 - There is already a conference of that name
	CHATMSG_MustHaveOps,				// 9 - You must have operator status to do this
	CHATMSG_ConferenceRenamed,			// a - Conference <name> renamed to .
	CHATMSG_MustBeInAConference,		// b - You must be in a conference to do this. To join a conference, select one from the Conference menu
	CHATMSG_NoPlayer,					// c - There is no player named <name>
	CHATMSG_NoConference,				// d - There is no conference named <name>
	CHATMSG_IncorrectPassword,			// e - That is not the correct password
	CHATMSG_PlayerIsIgnoring,			// f - <name> has chosen to ignore you. None of your messages to them will get through
	CHATMSG_RevokedSpeaking,			// 10 - The moderator of this conference has not given you speaking priveledges.
	CHATMSG_ReceivingPrivate,			// 11 - You can now receive private messages
	CHATMSG_NoLongerReceivingPrivate,	// 12 - You will no longer receive private messages. Those who send you a message will be notified that you are blocking incoming messages
	CHATMSG_ShowingName,				// 13 - You are now showing your character name to any players who inquire with the whois command
	CHATMSG_NotShowingName,				// 14 - You are no long showing your character name to any players who inquire with the whois command
	CHATMSG_PlayerIsAnonymous,			// 15 - <name> is remaining anonymous
	CHATMSG_PlayerNotReceivingPrivate,	// 16 - <name> has chosen to not receive private messages at the moment
	CHATMSG_PlayerKnownAs,				// 17 - <name> is known in the lands of britania as .
	CHATMSG_PlayerIsKicked,				// 18 - <name> has been kicked out of the conference
	CHATMSG_ModeratorHasKicked,			// 19 - <name>, a conference moderator, has kicked you out of the conference
	CHATMSG_AlreadyInConference,		// 1a - You are already in the conference <name>
	CHATMSG_PlayerNoLongerModerator,	// 1b - <name> is no longer a conference moderator
	CHATMSG_PlayerIsAModerator,			// 1c - <name> is now a conference moderator
	CHATMSG_RemovedListModerators,		// 1d - <name> has removed you from the list of conference moderators.
	CHATMSG_YouAreAModerator,			// 1e - <name> has made you a conference moderator
	CHATMSG_PlayerNoSpeaking,			// 1f - <name> no longer has speaking priveledges in this conference.
	CHATMSG_PlayerNowSpeaking,			// 20 - <name> now has speaking priveledges in this conference
	CHATMSG_ModeratorRemovedSpeaking,	// 21 - <name>, a channel moderator, has removed your speaking priveledges for this conference.
	CHATMSG_ModeratorGrantSpeaking,		// 22 - <name>, a channel moderator, has granted you speaking priveledges for this conference.
	CHATMSG_SpeakingByDefault,			// 23 - From now on, everyone in the conference will have speaking priviledges by default.
	CHATMSG_ModeratorsSpeakDefault,		// 24 - From now on, only moderators in this conference will have speaking priviledges by default.
	CHATMSG_PlayerTalk,					// 25 - <name>:
	CHATMSG_PlayerEmote,				// 26 - *<name>*
	CHATMSG_PlayerPrivate,				// 27 - [<name>]:
	CHATMSG_PasswordChanged,			// 28 - The password to the conference has been changed
	CHATMSG_NoMorePlayersAllowed,		// 29 - Sorry--the conference named <name> is full and no more players are allowed in.

	CHATMSG_SendChannelName =	0x03e8, // Message to send a channel name and mode to client's channel list
	CHATMSG_RemoveChannelName =	0x03e9, // Message to remove a channel name from client's channel list
	CHATMSG_GetChatName =		0x03eb,	// Ask the client for a permanent chat name
	CHATMSG_CloseChatWindow =	0x03ec,	// Close the chat system dialogs on the client???
	CHATMSG_OpenChatWindow =	0x03ed,	// Open the chat system dialogs on the client
	CHATMSG_SendPlayerName =	0x03ee,	// Message to add a player out to client (prefixed with a "1" if they are the moderator or a "0" if not
	CHATMSG_RemoveMember =		0x03ef,	// Message to remove a player from clients channel member list
	CHATMSG_ClearMemberList =	0x03f0,	// This message clears the list of channel participants (for when leaving a channel)
	CHATMSG_UpdateChannelBar =	0x03f1,	// This message changes the name in the channel name bar
	CHATMSG_QTY,						// Error (but does 0x03f1 anyways)
};

enum INPVAL_STYLE	// for the various styles for InpVal box.
{
	INPVAL_STYLE_NOEDIT	= 0,		// No textbox, just a message
	INPVAL_STYLE_TEXTEDIT	= 1,		// Alphanumeric
	INPVAL_STYLE_NUMEDIT	= 2,		// Numeric
};

enum MAPCMD_TYPE
{
	MAP_ADD = 1,
	MAP_PIN = 1,
	MAP_INSERT = 2,
	MAP_MOVE = 3,
	MAP_DELETE = 4,
	MAP_UNSENT = 5,
	MAP_CLEAR = 5,
	MAP_TOGGLE = 6,
	MAP_SENT = 7,
};

enum WEATHER_TYPE
{
	WEATHER_DEFAULT = 0xFE,
	WEATHER_DRY = 0xFF,
	WEATHER_RAIN = 0,
	WEATHER_STORM,
	WEATHER_SNOW,
	WEATHER_CLOUDY,	// not client supported ? (Storm brewing)
};

enum EFFECT_TYPE
{
	EFFECT_BOLT = 0,	// a targetted bolt
	EFFECT_LIGHTNING,	// lightning bolt.
	EFFECT_XYZ,	// Stay at current xyz ??? not sure about this.
	EFFECT_OBJ,	// effect at single Object.
};

enum NOTO_TYPE
{
	NOTO_INVALID = 0,	// 0= not a valid color!!
	NOTO_GOOD,		// 1= good(blue),
	NOTO_GUILD_SAME,// 2= same guild,
	NOTO_NEUTRAL,	// 3= Neutral,
	NOTO_CRIMINAL,	// 4= criminal
	NOTO_GUILD_WAR,	// 5= Waring guilds,
	NOTO_EVIL,		// 6= evil(red),
	NOTO_INVUL,		// 7= invulnerable,
};

enum TALKMODE_TYPE	// Modes we can talk/bark in.
{
	TALKMODE_SYSTEM = 0,	// normal system message
	TALKMODE_PROMPT,		// 1= Display as system prompt
	TALKMODE_EMOTE,			// 2= :	*smiles* at object
	TALKMODE_SAY,			// 3= A chacter speaking.
	TALKMODE_OBJ,			// 4= At Object
	TALKMODE_NOTHING,		// 5= Does not display
	TALKMODE_ITEM,			// 6= text labeling an item. Preceeded by "You see"
	TALKMODE_NOSCROLL,		// 7= As a status msg. Does not scroll
	TALKMODE_WHISPER,		// 8= ;	only those close can here.
	TALKMODE_YELL,			// 9= ! can be heard 2 screens away.
	TALKMODE_SPELL,			// 10 = used by spells
	TALKMODE_GUILD = 0xd,	// 13 = guild speech
	TALKMODE_ALLIANCE,		// 14 = alliance speech
	TALKMODE_BROADCAST = 0xFF,
};

enum SKILLLOCK_TYPE
{
	SKILLLOCK_UP = 0,
	SKILLLOCK_DOWN,
	SKILLLOCK_LOCK,
};

enum DELETE_ERR_TYPE
{
	DELETE_ERR_BAD_PASS = 0, // 0 That character password is invalid.
	DELETE_ERR_NOT_EXIST,	// 1 That character does not exist.
	DELETE_ERR_IN_USE,	// 2 That character is being played right now.
	DELETE_ERR_NOT_OLD_ENOUGH, // 3 That character is not old enough to delete. The character must be 7 days old before it can be deleted.
	DELETE_SUCCESS = 255,
};

struct CEventCharDef
{
#define MAX_ITEM_NAME_SIZE	256		// imposed by client for protocol
#define MAX_NAME_SIZE 30
#define MAX_ACCOUNT_NAME_SIZE MAX_NAME_SIZE
#define MAX_ACCOUNT_PASSWORD_ENTER 16	// client only allows n chars.
	char m_charname[MAX_ACCOUNT_NAME_SIZE];
	char m_charpass[MAX_NAME_SIZE];	// but the size of the structure is 30 (go figure)
};

struct CEvent	// event buffer from client to server..
{
#define MAX_BUFFER			15360	// Buffer Size (For socket operations)
#define MAX_ITEMS_CONT		256		// Max items in a container. (arbitrary)
#define MAX_MENU_ITEMS		64		// number of items in a menu. (arbitrary)
#define MAX_CHARS_PER_ACCT	5

	// Some messages are bidirectional.

	union
	{
		BYTE m_Raw[ MAX_BUFFER ];

		DWORD m_CryptHeader;	// This may just be a crypt header from the client.

		struct
		{
			BYTE m_Cmd;		// XCMD_TYPE, 0 = ?
			BYTE m_Arg[1];	// unknown size.
		} Default;	// default unknown type.

		struct	// XCMD_Create, size = 100	// create a new char
		{
			BYTE m_Cmd;		// 0=0
			NDWORD m_pattern1; // 0xedededed
			NDWORD m_pattern2; // 0xffffffff
			BYTE m_kuoc; // KUOC Signal (0x00 normally. 0xff for Krrios' client)

			char m_charname[MAX_NAME_SIZE];		// 10
			//char m_charpass[MAX_NAME_SIZE];		// Not used anymore
			
			BYTE m_unk8[2];
			NDWORD m_flags;
			BYTE m_unk10[8];
			BYTE m_prof;
			BYTE m_unk11[15];

			BYTE m_sex;		// 70, 0 = male
			BYTE m_str;		// 71
			BYTE m_dex;		// 72
			BYTE m_int;		// 73

			BYTE m_skill1;
			BYTE m_val1;
			BYTE m_skill2;
			BYTE m_val2;
			BYTE m_skill3;
			BYTE m_val3;

			NWORD m_wSkinHue;	// 0x50 // HUE_TYPE
			NWORD m_hairid;
			NWORD m_hairHue;
			NWORD m_beardid;
			NWORD m_beardHue;	// 0x58
			BYTE m_unk2;
			BYTE m_startloc;
			BYTE m_unk3;
			BYTE m_unk4;
			BYTE m_unk5;
			BYTE m_slot;
			BYTE m_clientip[4];
			NWORD m_shirtHue;
			NWORD m_pantsHue;
		} Create;

		struct	// size = >3	// user typed in text.
		{
			BYTE m_Cmd;		// 0 = 0x03
			NWORD m_len;	// 1,2=length of packet
			BYTE m_mode;	// 3=mode(9=yell) TALKMODE_TYPE
			NWORD m_wHue;
			NWORD m_font;	// 6,7 = FONT_TYPE
			char m_text[1];	// 8=var size MAX_TALK_BUFFER
		} Talk;

		struct	// size = 5
		{
			BYTE m_Cmd;	// 0 = 0x05, 0x06 or 0x09
			NDWORD m_UID;
		} Click;	// Attack, DClick, Click

		struct // size = 7 = pick up an item
		{
			BYTE m_Cmd;			// 0 = 0x07
			NDWORD m_UID;		// 1-4
			NWORD m_amount;
		} ItemPickupReq;

		struct	// size = 14 = drop item on ground or in container.
		{
			BYTE m_Cmd;			// 0 = 0x08
			NDWORD m_UID;		// 1-4 = object being dropped.
			NWORD m_x;		// 5,6 = 255 = invalid
			NWORD m_y;		// 7,8
			BYTE m_z;			// 9
			NDWORD m_UIDCont;	// 10 = dropped on this object. 0xFFFFFFFF = no object
		} ItemDropReq;

		struct	// size > 3
		{
			BYTE m_Cmd;		// 0 = 0x12
			NWORD m_len;	// 1-2 = len
			BYTE m_type;	// 3 = 0xc7=anim, 0x24=skill...
			char m_name[1];	// 4=var size string
		} ExtCmd;

		struct // size = ? reserved GM tool message.
		{
			BYTE m_Cmd;		// 0 = 0x11
			NWORD m_len;	// 1-2 = len
			BYTE m_type;	// 3 = sub type

		} GMToolMsg;

		struct // size = ??? = Get a skill lock (or multiple)
		{
			BYTE m_Cmd;		// 0= 0x3A
			NWORD m_len;	// 1= varies
			struct
			{
				NWORD m_index;	// skill index
				BYTE m_lock;	// SKILLLOCK_TYPE current lock mode (0 = none (up), 1 = down, 2 = locked)
			} skills[1];
		} Skill;

		struct	// size = 11	// plot course on map.
		{
			BYTE m_Cmd;		// 0 = 0x56
			NDWORD m_UID;  // uid of the map
			BYTE m_action;	// 1, MAPCMD_TYPE 1: add pin, 5: delete pin, 6: toggle edit/noedit or cancel while in edit
			BYTE m_pin;
			NWORD m_pin_x;
			NWORD m_pin_y;
		} MapEdit;

		struct	// size = 0x49 = 73	// play this slot char
		{
			BYTE m_Cmd;			// 0 = 0x5D
			NDWORD m_edededed;	// 1-4 = ed ed ed ed
			char m_charname[MAX_NAME_SIZE];
			char m_charpass[MAX_NAME_SIZE];
			NDWORD m_slot;		// 0x44 = slot
			BYTE m_clientip[4];	// = 18 80 ea 15
		} CharPlay;

		struct	// size > 3 // user is changing a book page.
		{
			BYTE m_Cmd;		// 0 = 0x66
			NWORD m_len;	// 1-2
			NDWORD m_UID;	// 3-6 = the book
			NWORD m_pages;	// 7-8 = 0x0001 = # of pages here
			// repeat these fields for multiple pages.
			struct
			{
				NWORD m_pagenum;	// 9-10=page number  (1 based)
				NWORD m_lines;	// 11
				char m_text[1];
			} m_page[1];
		} BookPage;

		struct	// size = 19
		{
			BYTE m_Cmd;		// 0 = 0x6C
			BYTE m_TargType;	// 1 = 0=select object, 1=x,y,z=allow ground
			NDWORD m_context;	// 2-5 = we sent this at target setup.
			BYTE m_fCheckCrime;		// 6= 0

			NDWORD m_UID;	// 7-10	= serial number of the target.
			NWORD m_x;		// 11,12
			NWORD m_y;		// 13,14
			BYTE m_unk2;	// 15 = fd ?
			BYTE m_z;		// 16
			NWORD m_id;		// 17-18 = static id of tile
		} Target;

		struct // size = var // Secure trading
		{
			BYTE m_Cmd;		// 0=0x6F
			NWORD m_len;	// 1-2 = len
			BYTE m_action;	// 3 = trade action
			NDWORD m_UID;	// 4-7 = uid
			NDWORD m_UID1;	// 8-11
		} SecureTrade;

		struct // size = var (0x0c)	// Bulletin request
		{
			BYTE m_Cmd;		// 0=0x71
			NWORD m_len;	// 1-2 = len = 0x0c
			BYTE m_flag;	// 3= BBOARDF_TYPE 0=board name, 5=new message, 4=associate message, 1=message header, 2=message body
			NDWORD m_UID;	// 4-7 = UID for the bboard.
			NDWORD m_UIDMsg; // 8- = name or links data.
			BYTE m_data[1];	// submit new message data here.
		} BBoard;

		struct // size = 5	// Client wants to go into war mode.
		{
			BYTE m_Cmd;		// 0 = 0x72
			BYTE m_warmode;	// 1 = 0 or 1
			BYTE m_unk2[3];	// 2 = 00 32 00
		} War;

		struct // size = 35	// set the name for this NPC (pet) char
		{
			BYTE m_Cmd;		// 0=0x75
			NDWORD m_UID;
			char m_charname[ MAX_NAME_SIZE ];
		} CharName;

		struct	// size = 13	// choice off a menu
		{
			BYTE m_Cmd;		// 0 = 0x7d
			NDWORD m_UID;	// 1 = char id from 0x7c message
			NWORD m_context;	// 5,6 = context info passed from server
			NWORD m_select;	// 7,8	= what item selected from menu. 1 based ?
			NDWORD m_unk9;
		} MenuChoice;

		struct // size = 62		// first login to req listing the servers.
		{
			BYTE m_Cmd;	// 0 = 0x80 = XCMD_ServersReq
			char m_acctname[ MAX_ACCOUNT_NAME_SIZE ];
			char m_acctpass[ MAX_NAME_SIZE ];
			BYTE m_unk;	// 61 = ff
		} ServersReq;

		struct	// size = 39  // delete the char in this slot.
		{
			BYTE m_Cmd;		// 0 = 0x83
			BYTE m_charpass[MAX_NAME_SIZE];
			NDWORD m_slot;	// 0x22
			BYTE m_clientip[4];
		} CharDelete;

		struct	// size = 65	// request to list the chars I can play.
		{
			BYTE m_Cmd;		  // 0 = 0x91
			NDWORD m_Account; // 1-4 = account id from XCMD_Relay message to log server.
			char m_acctname[MAX_ACCOUNT_NAME_SIZE];	// This is corrupted or encrypted seperatly ?
			char m_acctpass[MAX_NAME_SIZE];
		} CharListReq;

		struct // size = 98 // Get a change to the book title.
		{
			BYTE m_Cmd;			// 0 = 0x93
			NDWORD m_UID;		// 1-4 = book
			BYTE m_writable;	// 5 = 0 = non writable, 1 = writable. (NOT USED HERE)
			BYTE m_NEWunk1;
			NWORD m_pages;		// 6-7 = number of pages. (NOT USED HERE)
			char m_title[ 2 * MAX_NAME_SIZE ];
			char m_author[ MAX_NAME_SIZE ];
		} BookOpen;

		struct	// size = 9		// select Hue from dye chart.
		{
			BYTE m_Cmd;		// 0x95
			NDWORD m_UID;	// 1-4
			NWORD m_unk5;	// = 0
			NWORD m_wHue;	// 7,8	HUE_TYPE
		} DyeVat;

		struct	// size = 9		// New AllNames3D.
		{
			BYTE m_Cmd;		// 0x98
			NWORD m_len;	// Always 7
			NDWORD m_UID;
		} AllNames3D;

		struct // size = 16+var = console Prompt response.
		{
			BYTE m_Cmd;		// 0x9a
			NWORD m_len;
			BYTE m_unk3[12];
			char m_text[1];	// null terminated text.
		} Prompt;

		struct // size = 0x102	// GM page = request for help.
		{
			BYTE m_Cmd;		// 0 = 0x9b
			BYTE m_unk1[0x101];
		} HelpPage;

		struct	// size = 3;	// relay me to the server i choose.
		{
			BYTE m_Cmd;		// 0 = 0xa0
			NWORD m_select;	// 2 = selection this server number
		} ServerSelect;

		struct // size = var // Get text from gump input
		{
			BYTE m_Cmd;		// 0=0xAC
			NWORD m_len;
			NDWORD m_UID;	// just another context really.
			NWORD m_context; // = context info passed from server
			BYTE m_retcode; // 0=canceled, 1=okayed
			NWORD m_textlen; // length of text entered
			char m_text[MAX_NAME_SIZE];
		} GumpInpValRet;

		struct // size = var // Unicode Speech Request
		{
			BYTE m_Cmd;		// 0=0xAD
			NWORD m_len;
			BYTE m_mode;	// (0=say, 2=emote, 8=whisper, 9=yell) TALKMODE_TYPE
			NWORD m_wHue;	// HUE_TYPE
			NWORD m_font;	// FONT_TYPE
			char m_lang[4];	// lang (null terminated, "ENU" for US english.) CLanguageID
			NCHAR m_utext[1];	// NCHAR[?] text (null terminated, ?=(msgsize-12)/2) MAX_TALK_BUFFER
		} TalkUNICODE;

		struct // size = var // Get gump button change
		{
			BYTE m_Cmd;				//  0 =  0xB1
			NWORD m_len;			//  1 -  2
			NDWORD m_UID;			//  1 -  4
			NDWORD m_context;	//  7 - 10 = context info passed from server
			NDWORD m_buttonID;		// 11 - 14 = what button was pressed ?
			NDWORD m_checkQty;	// 15 - 18
			NDWORD m_checkIds[1];	// 19 - ?? id of a checkbox or radio button that is checked
			NDWORD m_textQty;	// textentry boxes have returned data
			struct
			{
				NWORD m_id;		// id specified in the gump definition file
				NWORD m_len;		// length of the string (in unicode so multiply by 2!)
				NCHAR m_utext[1];	// character data (in unicode!):
			} m_texts[1];
		} GumpDialogRet;

		struct // size = 13?	// ?Request user name
		{
			BYTE m_Cmd;		// 0 = 0xb2
			NDWORD m_uid;	// 1 = targ uid ?
			NDWORD m_unk2;
			NDWORD m_unk3;
		} ChatReq;

		struct // size = various
		{
			BYTE m_Cmd;		//	0   = 0xb3
			NWORD m_len;	//  1-2 = packet length
			char m_lang[4];	//	3-6 = unicode language code CLanguageID
			NCHAR m_utext[1];	//	7-? = text from client
		} ChatText;

		struct // size = 64	// response to pressing the chat button
		{
			BYTE m_Cmd;		// 0 = 0xb5
			BYTE m_unk1;	// 1 = ???
			NCHAR m_uname[MAX_NAME_SIZE+1];	// 2-63 - unicode name
		} Chat;

		struct	// size = 8 // Get Character Profile.
		{
			BYTE m_Cmd;		// 0=0xB8
			NWORD m_len;	// 1 - 2
			BYTE m_WriteMode;	// 3; 0 = Get profile, 1 = Set profile
			NDWORD m_UID;	// 4=uid
			BYTE m_unk1;	// 8
			BYTE m_retcode; // ????? 0=canceled, 1=okayed or something similar???
			NWORD m_textlen; // length of text entered
			NCHAR m_utext[1]; // Text, unicode!
		} CharProfile;

		struct	// size = 9 // mail msg. get primed with itself at start.
		{
			BYTE m_Cmd;		// 0=0xBB
			NDWORD m_uid1;	// 1-4 = uid
			NDWORD m_uid2;	// 5-8 = uid
		} MailMsg;

		struct	// size = ??
		{
			BYTE m_Cmd;		// 0=0xBD
			NWORD m_len;	// 1-2 = len of packet
			char m_text[1024];
		} ClientVersion;

		struct	// size = 2 // mail msg. get primed with itself at start.
		{
			BYTE m_Cmd;	// 0=0xC8
			BYTE m_Range;	// 5-18 = range
		} ViewRange;
		
		struct	// we get walk codes from the > 1.26.0 server to be sent back in the Walk messages.
		{
			// walk codes from the > 1.26.0 server to be sent back in the Walk messages
			BYTE m_Cmd;	// 0=0xbf
			NWORD m_len;	// 1 - 2 (len = 5 + (# of codes * 4)
			NWORD m_type;	// 3 - 4 EXTDATA_TYPE 1=prime LIFO stack, 2=add to beginning of LIFO stack, 6=add to party
			CExtData m_u;
		} ExtData;
		
		struct	// size = ?? // Config File (IGR)
		{
			BYTE m_Cmd;		// 0=0xD0
			NWORD m_len;	// 1 - 2
			BYTE m_type;	// file type
			BYTE m_config_data[1]; // Data m_len-4
		} ConfigFile;
		
		struct
		{
			BYTE m_Cmd;	// 0 = 0xd6
			NWORD m_len;	// 1 - 2 (len = 5 + (# of codes * 4)
			NDWORD m_uid[1];
		} AosItemInfoRequest;

		struct
		{
			BYTE m_Cmd;	// 0=0xd7
			NWORD m_len;	// 1 - 2 (len = 5 + (# of codes * 4)
			NDWORD m_uid; 
			NWORD m_type;	// 3 - 4 EXTDATA_TYPE
			CExtAosData m_u;
		} ExtAosData;
	};
} PACK_NEEDED;

struct CCommand	// command buffer from server to client.
{
	union
	{
		BYTE m_Raw[ MAX_BUFFER ];

		struct
		{
			BYTE m_Cmd;		// 0 = ?
			BYTE m_Arg[1];	// unknown size.
		} Default;	// default unknown type.

		struct // size = 36 + 30 = 66	// Get full status on char.
		{
			BYTE m_Cmd;			// 0 = 0x11
			NWORD  m_len;		// 1-2 = size of packet (2 bytes)
			NDWORD m_UID;		// 3-6 = (first byte is suspected to be an id byte - 0x00 = player)
			char m_charname[ MAX_NAME_SIZE ];	// 7-36= character name (30 bytes) ( 00 padded at end)
			NWORD m_health;		// 37-38 = current health
			NWORD m_maxhealth;	// 39-40 = max health
			BYTE m_perm;		// 41 = permission to change name ? 0=stats invalid, 1=stats valid., 0xff = i can change the name
			BYTE m_ValidStats;	// 42 = 0 not valid, 1 = till weight, 2 = till statcap, 3 till follower, 4 = till the end.
			// only send the rest here if player.
			BYTE m_sex;			// 43 = sex (0 = male)
			NWORD m_str;		// 44-45 = strength
			NWORD m_dex;		// 46-47 = dexterity
			NWORD m_int;		// 48-49 = intelligence
			NWORD m_stam;		// 50-51 = current stamina
			NWORD m_maxstam;	// 52-53 = max stamina
			NWORD m_mana;		// 54-55 = current mana
			NWORD m_maxmana;	// max mana (2 bytes)
			NDWORD m_gold;		// 58-61 = gold
			NWORD m_armor;		// 62-63 = armor class
			NWORD m_weight;		// 64-65 = weight in stones
			// new {5}
			// NWORD m_maxweight;
			// BYTE m_race;
			// new (2)
			NWORD m_statcap;
			// new (3)
			BYTE m_curFollower;
			BYTE m_maxFollower;
			// new (4)
			NWORD m_resFire;
			NWORD m_resCold;
			NWORD m_resPoison;
			NWORD m_resEnergy;
			NWORD m_Luck;
			NWORD m_minDamage;
			NWORD m_maxDamage;
			NDWORD m_Tithing;
		} Status;

		struct // size = 36 + 30 = 66	// Get full status on char.
		{
			BYTE m_Cmd;			// 0 = 0x11
			NWORD  m_len;		// 1-2 = size of packet (2 bytes)
			NDWORD m_UID;		// 3-6 = (first byte is suspected to be an id byte - 0x00 = player)
			char m_charname[ MAX_NAME_SIZE ];	// 7-36= character name (30 bytes) ( 00 padded at end)
			NWORD m_health;		// 37-38 = current health
			NWORD m_maxhealth;	// 39-40 = max health
			BYTE m_perm;		// 41 = permission to change name ? 0=stats invalid, 1=stats valid., 0xff = i can change the name
			BYTE m_ValidStats;	// 42 = 0 not valid, 1 = till weight, 2 = till statcap, 3 till follower, 4 = till the end.
			// only send the rest here if player.
			BYTE m_sex;			// 43 = sex (0 = male)
			NWORD m_str;		// 44-45 = strength
			NWORD m_dex;		// 46-47 = dexterity
			NWORD m_int;		// 48-49 = intelligence
			NWORD m_stam;		// 50-51 = current stamina
			NWORD m_maxstam;	// 52-53 = max stamina
			NWORD m_mana;		// 54-55 = current mana
			NWORD m_maxmana;	// max mana (2 bytes)
			NDWORD m_gold;		// 58-61 = gold
			NWORD m_armor;		// 62-63 = armor class
			NWORD m_weight;		// 64-65 = weight in stones
			NWORD m_maxWeight;	// 66-68 = max weight in stones
			BYTE m_race;		// 69 = race (0=human, 1=elf)
			NWORD m_statcap;
			BYTE m_curFollower;
			BYTE m_maxFollower;
			NWORD m_resFire;
			NWORD m_resCold;
			NWORD m_resPoison;
			NWORD m_resEnergy;
			NWORD m_Luck;
			NWORD m_minDamage;
			NWORD m_maxDamage;
			NDWORD m_Tithing;
		} StatusNew;

		struct // size = 37	// start up player
		{
			BYTE	m_Cmd;			// 0 = 0x1B
			NDWORD	m_UID;			// 1-4
			NDWORD	m_unk_5_8;		// 5-8 = 00 00 00 00
			NWORD	m_id;			// 9-10
			NWORD	m_x;			// 11-12
			NWORD	m_y;			// 13-14
			NWORD	m_z;			// 15-16
			BYTE	m_dir;			// 17
			BYTE	m_unk_18;		// 18 = 00
			NDWORD	m_unk_19_22;	// 19-22 = FF FF FF FF
			NWORD	m_boundX;	// 23-14
			NWORD	m_boundY;	// 23-14
			NWORD	m_mode;			// 27-28 = CHARMODE_WAR
			NWORD	m_boundH;	// 29-30 = 0960
			NWORD	m_zero_31;
			NDWORD	m_zero_33;
		} Start;

		// Char mode flags
#define CHARMODE_POISON		0x04	// green status bar.
#define CHARMODE_YELLOW		0x08	// ? yellow status bar.
#define CHARMODE_WAR		0x40
#define CHARMODE_INVIS		0x80

		struct // size = 14 + 30 + var
		{
			BYTE m_Cmd;			// 0 = 0x1C
			NWORD m_len;		// 1-2 = var len size.
			NDWORD m_UID;		// 3-6 = UID num of speaker. 01010101 = system
			NWORD m_id;			// 7-8 = CREID_TYPE of speaker.
			BYTE m_mode;		// 9 = TALKMODE_TYPE
			NWORD m_wHue;		// 10-11 = HUE_TYPE.
			NWORD m_font;		// 12-13 = FONT_TYPE
			char m_charname[MAX_NAME_SIZE];	// 14
			char m_text[1];		// var size. MAX_TALK_BUFFER
		} Speak;

		struct // size = 19 // set client player view x,y,z.
		{
			BYTE m_Cmd;			// 0 = 0x20
			NDWORD m_UID;		// 1-4 = my UID.
			NWORD m_id;			// 5-6
			BYTE m_zero7;		// 7 = 0
			NWORD m_wHue;		// 8-9 = HUE_TYPE
			BYTE m_mode;		// 10 = 0=normal, 4=poison, 0x40=attack, 0x80=hidden CHARMODE_WAR
			NWORD m_x;			// 11-12
			NWORD m_y;			// 13-14
			NWORD m_zero15;		// 15-16 = noto ?
			BYTE m_dir;			// 17 = high bit set = run
			BYTE m_z;			// 18
		} View;

		struct // size = 26
		{
			BYTE m_Cmd;		// 0=0x23
			NWORD m_id;
			NWORD m_unk3;	// 3-4 = 0
			NWORD m_unk5;	// 5-9 = ?
			BYTE  m_unk7;	// 7 = 0, 72, 99, c1, 73
			NDWORD m_srcUID; // NULL if from ground.
			NWORD m_src_x;
			NWORD m_src_y;
			BYTE m_src_z;
			NDWORD m_dstUID;
			NWORD m_dst_x;
			NWORD m_dst_y;
			BYTE m_dst_z;
		} DragAnim;

		struct // size = 7	// Open a container gump
		{
			BYTE m_Cmd;		// 0 = 0x24
			NDWORD m_UID;	// 1-4
			NWORD m_gump;	// 5 = gump graphic id. GUMP_TYPE
		} ContOpen;

		struct // size = 20	// Add Single Item To Container.
		{
			BYTE m_Cmd;		// 0 = 0x25
			NDWORD m_UID;	// 1-4
			NWORD m_id;
			BYTE m_zero7;
			NWORD m_amount;
			NWORD m_x;
			NWORD m_y;
			NDWORD m_UIDCont;	// the container.
			NWORD m_wHue;		// HUE_TYPE
		} ContAdd;

		struct // size = 2	// kill a drag. (Bounce)
		{
			BYTE m_Cmd;		// 0 = 0x27
			BYTE m_type;	// 1 = bounce type ? = 5 = drag
		} DragCancel;

		struct // size = 15 = equip this item
		{
			BYTE m_Cmd;			// 0 = 0x2e
			NDWORD m_UID;		// 1-4 = object UID.
			NWORD m_id;			// 5-6
			BYTE m_zero7;
			BYTE m_layer;		// 8=LAYER_TYPE
			NDWORD m_UIDchar;	// 9-12=UID num of owner.
			NWORD m_wHue;		// HUE_TYPE
		} ItemEquip;

		struct // size = ?? = Fill in the skills list.
		{
			BYTE m_Cmd;		// 0= 0x3A
			NWORD m_len;	// 1=
			BYTE m_single;	// 3=0 = all w/ 0 term, ff=single and no terminator
			struct
			{
				NWORD m_index;	// 1 based, 0 = terminator. (no val)
				NWORD m_val;	// Skill * 10 (stat modified!)
				NWORD m_valraw; // Skill * 10 (stat unmodified!)
				BYTE m_lock;	// current lock mode (0 = none (up), 1 = down, 2 = locked)
			} skills[1];
		} Skill;

		struct // size = ??? = Get a skill lock (or multiple)
		{
			BYTE m_Cmd;		// 0= 0x3A
			NWORD m_len;	// 1= varies
			BYTE m_single;	// 3=0 = all w/ 0 term, ff=single and no terminator
			struct
			{
				NWORD m_index;	// skill index
				NWORD m_val;	// Skill * 10 (stat modified!)
				NWORD m_valraw; // Skill * 10 (stat unmodified!)
				BYTE m_lock;	// SKILLLOCK_TYPE current lock mode (0 = none (up), 1 = down, 2 = locked)
				NWORD m_cap;	// cap of the current skill
			} skills[1];
		} Skill_New;

		struct // size = 5 + ( x * 19 ) // set up the content of some container.
		{
#define MAX_ITEMS_CONTENT (MAX_BUFFER - 5) / 19
			BYTE m_Cmd;		// 0 = 0x3C
			NWORD m_len;
			NWORD m_count;

			struct // size = 19
			{
				NDWORD m_UID;	// 0-3
				NWORD m_id;	// 4-5
				BYTE m_zero6;
				NWORD m_amount;	// 7-8
				NWORD m_x;	// 9-10
				NWORD m_y;	// 11-12
				NDWORD m_UIDCont;	// 13-16 = What container is it in.
				NWORD m_wHue;	// 17-18 = HUE_TYPE
			} m_item[ MAX_ITEMS_CONTENT ];
		} Content;

		struct	// size = 2	// Sent for idle/wrong char
		{
			BYTE m_Cmd;		// 0 =0x53
			BYTE m_Value;	// 0x05 = Another char is online / 0x07 = Idle for too much
		} IdleWarning;

		struct // size = 12
		{
			BYTE m_Cmd;		// 0 = 0x54
			BYTE m_flags;	// 1 = quiet, infinite repeat or single shot (0/1)
			NWORD m_id;		// 2-3=sound id (SOUND_TYPE)
			NWORD m_volume;	// 4-5=0 = (speed/volume modifier?)
			NWORD m_x;		// 6-7
			NWORD m_y;		// 8-9
			NWORD m_z;		// 10-11
		} Sound;

		struct	// size = 1	// Sent at start up. Cause Client to do full reload and redraw of config.
		{
			BYTE m_Cmd;		// 0 =0x55
		} ReDrawAll;

		struct	// size = 11
		{
			BYTE m_Cmd; // 0 = 0x56
			NDWORD m_UID;
			BYTE m_Mode; // 1: Add a pin location, 5: Display map, pins to follow 7: Editable, no pins follow 8: read only
			BYTE m_Req;	// 0: req for read mode, 1: request for edit mode
			NWORD m_pin_x;
			NWORD m_pin_y;
		} MapEdit;

		struct	// size = 4	// Set Game Time. not sure why the client cares about this.
		{
			BYTE m_Cmd;		// 0 =0x5b
			BYTE m_hours;
			BYTE m_min;
			BYTE m_sec;
		} Time;

		struct // size = 4
		{
			BYTE m_Cmd;			// 0 = 0x65
			BYTE m_type;		// 1 = WEATHER_TYPE. 0xff=dry, 0=rain, 1=storm, 2=snow
			BYTE m_ext1;		// 2 = other weather info (severity?) 0x40 = active, 0=dry
			BYTE m_unk010;		// 3 = =0x10 always
		} Weather;

		struct	// size = 13 + var // send a book page.
		{
			BYTE m_Cmd;		// 0 = 0x66
			NWORD m_len;	// 1-2
			NDWORD m_UID;	// 3-6 = the book
			NWORD m_pages;	// 7-8 = 0x0001 = # of pages here
			// repeat these fields for multiple pages.
			struct
			{
				NWORD m_pagenum;	// 9-10=page number  (1 based)
				NWORD m_lines;	// 11
				char m_text[1];
			} m_page[1];
		} BookPage;

		struct	// size = var	// seems to do nothing here.
		{
			BYTE m_Cmd;		// 0=0x69
			NWORD m_len;
			NWORD m_index;
		} Options;

		struct	// size = 19
		{
			BYTE m_Cmd;		// 0 = 0x6C
			BYTE m_TargType;	// 1 = 0=select object, 1=x,y,z
			NDWORD m_context;	// 2-5 = we sent this at target setup.
			BYTE m_fCheckCrime;		// 6= 0

			// Unused junk.
			NDWORD m_UID;	// 7-10	= serial number of the target.
			NWORD m_x;		// 11,12
			NWORD m_y;		// 13,14
			BYTE m_unk2;	// 15 = fd ?
			BYTE m_z;		// 16
			NWORD m_id;		// 17-18 = static id of tile
		} Target;

		struct // size = 14 // Combat type animation.
		{
			BYTE m_Cmd;			// 0 = 0x6e
			NDWORD m_UID;		// 1-4=uid
			NWORD m_action;		// 5-6 = animation
			NWORD m_framecount;	// 7-8 = frame count
			NWORD m_repeat;		// 9-10 = repeat count. 0=forever.
			BYTE m_backward;	// 11 = backwards (0/1)
			BYTE m_repflag;		// 12 = 0=dont repeat. 1=repeat
			BYTE m_framedelay;	// 13 = 0=fastest. (number of seconds)
		} CharAction;

		struct // size = var < 47 // Secure trading
		{
			BYTE m_Cmd;		// 0=0x6F
			NWORD m_len;	// 1-2 = len
			BYTE m_action;	// 3 = trade action. SECURE_TRADE_TYPE
			NDWORD m_UID;	// 4-7 = uid = other character
			NDWORD m_UID1;	// 8-11 = container 1
			NDWORD m_UID2;	// 12-15 = container 2
			BYTE m_fname;	// 16 = 0=none, 1=name
			char m_charname[ MAX_NAME_SIZE ];
		} SecureTrade;

		struct // size = 28 // Graphical effect.
		{
			BYTE m_Cmd;			// 0 = 0x70
			BYTE m_motion;		// 1= the motion type. (0=point to point,3=static) EFFECT_TYPE
			NDWORD m_UID;		// 2-5 = The target item. or source item if point to point.
			NDWORD m_targUID;	// 6-9 = 0
			NWORD m_id;			// 10-11 = base display item 0x36d4 = fireball
			NWORD m_srcx;		// 12
			NWORD m_srcy;		// 14
			BYTE  m_srcz;		// 16
			NWORD m_dstx;		// 17
			NWORD m_dsty;		// 19
			BYTE  m_dstz;		// 21
			BYTE  m_speed;		// 22= 0=very fast, 7=slow.
			BYTE  m_loop;		// 23= 0 is really long.  1 is the shortest.
			WORD 	m_unk;		// 24=0 HUE_TYPE ?
			BYTE 	m_OneDir;		// 26=1=point in single dir else turn.
			BYTE 	m_explode;	// 27=effects that explode on impact.
			NDWORD	m_hue;
			NDWORD	m_render;
		} Effect;

		struct // size = var (38)	// Bulletin Board stuff
		{
			BYTE m_Cmd;		// 0=0x71
			NWORD m_len;	// 1-2 = len
			BYTE m_flag;	// 3= 0=board name, 4=associate message, 1=message header, 2=message body
			NDWORD m_UID;	// 4-7 = UID for the bboard.
			BYTE m_data[1];	// 8- = name or links data.
		} BBoard;

		struct // size = 5	// put client to war mode.
		{
			BYTE m_Cmd;		// 0 = 0x72
			BYTE m_warmode;	// 1 = 0 or 1
			BYTE m_unk2[3];	// 2 = 00 32 00
		} War;

		struct // size = 17	// simple move of a char already on screen.
		{
			BYTE m_Cmd;		// 0 = 0x77
			NDWORD m_UID;	// 1-4
			NWORD m_id;		// 5-6 = id
			NWORD m_x;		// 7-8 = x
			NWORD m_y;		// 9-10
			BYTE m_z;		// 11
			BYTE m_dir;		// 12 = DIR_TYPE (| 0x80 = running ?)
			NWORD m_wHue;	// 13-14 = HUE_TYPE
			BYTE m_mode;	// 15 = CHARMODE_WAR
			BYTE m_noto;	// 16 = NOTO_TYPE
		} CharMove;

		struct // size = 23 or var len // draw char
		{
			BYTE m_Cmd;		// 0 = 0x78
			NWORD m_len;	// 1-2 = 0x0017 or var len?
			NDWORD m_UID;	// 3-6=
			NWORD m_id;	// 7-8
			NWORD m_x;	// 9-10
			NWORD m_y;	// 11-12
			BYTE m_z;		// 13
			BYTE m_dir;		// 14 = DIR_TYPE
			NWORD m_wHue;	// 15-16 = HUE_TYPE
			BYTE m_mode;	// 17 = CHARMODE_WAR
			BYTE m_noto;	// 18 = NOTO_TYPE

			struct	// This packet is extendable to show equip.
			{
				NDWORD m_UID;	// 0-3 = 0 = end of the list.
				NWORD m_id;		// 4-5 = | 0x8000 = include m_wHue.
				BYTE m_layer;	// LAYER_TYPE
				NWORD m_wHue;	// only if m_id | 0x8000
			} equip[1];

		} Char;

		struct // size = var // put up a menu of items.
		{
			BYTE m_Cmd;			// 0=0x7C
			NWORD m_len;
			NDWORD m_UID;		// if player then gray menu choice bar.
			NWORD m_context;		// = context info passed from server
			BYTE m_lenname;
			char m_name[1];		// var len. (not null term)
			BYTE m_count;
			struct		// size = var
			{
				NWORD m_id;		// image next to menu.
				NWORD m_color;	// color of the image
				BYTE m_lentext;
				char m_name[1];	// var len. (not null term)
			} m_item[1];
		} MenuItems;

		struct // size = var // Switch to this player? (in game)
		{
			// have no clue what it does to the crypt keys (probably nothing)
			// all I know is it puts up the list and sends back a 0x5d (Play) message...
			// if it goes further than that, it'll be awesome...what else can we do with this?

			BYTE m_Cmd; // 0 = 0x81
			NWORD m_len;
			BYTE m_count;
			BYTE m_unk;	// ? not sure.
			CEventCharDef m_char[MAX_CHARS_PER_ACCT];
		} CharList3;

		struct // size = 2
		{
			BYTE m_Cmd;		// 0 = 0x85
			BYTE m_code;	// 1 = DELETE_ERR_TYPE
		} DeleteBad;

		struct // size = var	// refill char list after delete.
		{
			BYTE m_Cmd;		// 0 = 0x86
			NWORD m_len;
			BYTE m_count;
			CEventCharDef m_char[MAX_CHARS_PER_ACCT];
		} CharList2;

		struct // size = 66
		{
			BYTE m_Cmd;			// 0 = 0x88
			NDWORD m_UID;		// 1-4 =
			char m_text[60];	// 5-
			BYTE m_mode;		// CHARMODE_WAR 0=normal, 4=poison, 0x40=attack, 0x80=hidden
		} PaperDoll;

		struct // size = 7 + count * 5 // Equip stuff on corpse.
		{
			BYTE m_Cmd;		// 0 = 0x89
			NWORD m_len;
			NDWORD m_UID;

			struct // size = 5
			{
				BYTE m_layer;	// 0 = LAYER_TYPE, list must be null terminated ! LAYER_NONE.
				NDWORD m_UID;	// 1-4
			} m_item[ MAX_ITEMS_CONT ];
		} CorpEquip;

		struct	// size = 11
		{
			BYTE m_Cmd;			// 0 = 0x8C
			BYTE m_ip[4];		// 1 = struct in_addr
			NWORD m_port;		// 5 = Port server is on
			NDWORD m_Account;	// 7-10 = customer id (sent to game server)
		} Relay;

		struct	// size = 19
		{
			BYTE m_Cmd; // 0 = 0x90
			NDWORD m_UID; // uid of the map item
			NWORD m_Gump_Corner; // GUMP_TYPE always 0x139d....compass tile id in the corner.,
			NWORD m_x_ul; // upper left x coord.
			NWORD m_y_ul; // upper left y coord.
			NWORD m_x_lr; // lower right x coord.
			NWORD m_y_lr; // lower right y coord.
			NWORD m_xsize; // client width
			NWORD m_ysize; // client height
		} MapDisplay;	// Map1

		struct // size = 98 // Get a change to the book title.
		{
			BYTE m_Cmd;			// 0 = 0x93
			NDWORD m_UID;		// 1-4 = book
			BYTE m_writable;	// 5 = 0 = non writable, 1 = writable. (NOT USED HERE)
			BYTE m_NEWunk1;
			NWORD m_pages;		// 6-7 = number of pages. (NOT USED HERE)
			char m_title[ 2 * MAX_NAME_SIZE ];
			char m_author[ MAX_NAME_SIZE ];
		} BookOpen;

		struct // size = 9
		{
			BYTE m_Cmd;			// 0 = 0x95
			NDWORD m_UID;		// 1-4
			NWORD m_zero5;		// 5-6
			NWORD m_id;		// 7-8
		} DyeVat;

		struct	// size = 9		// New AllNames3D.
		{
			BYTE m_Cmd;		// 0x98
			NWORD m_len;	// Always 37
			NDWORD m_UID;
			char m_name[ MAX_NAME_SIZE ];
		} AllNames3D;

		struct // size = 26	// preview a house/multi
		{
			BYTE m_Cmd;		// 0 = 0x99
			BYTE m_fAllowGround;	 // 1 = 1=ground only, 0=dynamic object
			NDWORD m_context;	// 2-5 = we sent this at target setup.
			BYTE m_zero6[12];	// 6-17
			NWORD m_id;		// 18-19 = The multi id to preview. (id-0x4000)
			BYTE m_zero20[6];
		} TargetMulti;

		struct // size = 16 // console prompt request.
		{
			BYTE m_Cmd;		// 0 = 0x9a
			NWORD m_len;	// 1-2 = length = 16
			BYTE m_unk3[12];
			char m_text[1];	// 3 = null terminated text? doesn't seem to work.
		} Prompt;

		struct	// size = 9	// update some change in stats.
		{
			BYTE m_Cmd;	// 0=0xa1 (str), 0xa2 (int), or 0xa3 (dex)
			NDWORD m_UID;	// 1-4
			NWORD m_max;	// 5-6
			NWORD m_val;	// 7-8
		} StatChng;

		struct // size = 3+var
		{
			BYTE m_Cmd;		// 0 = 0xA5
			NWORD m_len;
			char m_page[ 1 ];	// var size
		} Web;

		struct // size = 6+servers*40
		{
			BYTE m_Cmd;			// 0 = 0xA8
			NWORD m_len;		// 1-2
			BYTE m_unk3;		// 3=0x14 ?
			NWORD m_count;	// 4-5=num servers.

#define MAX_SERVER_NAME_SIZE 32

			struct	// size=40
			{
				NWORD m_count;			// 0=0 based enum
				char m_servname[32];	// 2
				BYTE m_percentfull;		// 34 = 25 or 2e
				signed char m_timezone;	// 35 = 0x05=east coast or 0x08=west coast
				NDWORD m_ip;			// 36-39 = ip to ping
			} m_serv[64];
		} ServerList;

		struct // size = 4+(5*2*MAX_NAME_SIZE)+1+(starts*63) // list available chars for your account.
		{
			BYTE m_Cmd;		// 0 = 0xa9
			NWORD m_len;	// 1-2 = var len
			BYTE m_count;	// 3=5 needs to be 5 for some reason.
			CEventCharDef m_char[MAX_CHARS_PER_ACCT];

			BYTE m_startcount;
			struct	// size = 63
			{
				BYTE m_id;
				char m_area[MAX_NAME_SIZE+1];
				char m_name[MAX_NAME_SIZE+1];
			} m_start[1];
			NDWORD	flags;
		} CharList;

		struct // size = 5 = response to attack.
		{
			BYTE m_Cmd;		// 0= 0xaa
			NDWORD m_UID;	// 1= char attacked. 0=attack refused.
		} AttackOK;

		struct // size = 13 + var // Send a gump text entry dialog.
		{
			BYTE m_Cmd;			// 0 = 0xab
			NWORD m_len;		// 1-2
			NDWORD m_UID;		// 3-6
			NWORD m_context;	// 7-8= context info passed from server
			NWORD m_textlen1;	// length of string #1
			char m_text1[256];	// line 1 variable text length
			BYTE m_cancel;		// 0 = disable, 1=enable
			BYTE m_style;		// 0 = disable, 1=normal, 2=numerical
			NDWORD m_mask;		// format[1] = max length of textbox input, format[2] = max value of textbox input
			NWORD m_textlen2;	// length of string #2
			char m_text2[256];	// line 2 text
		} GumpInpVal;

		struct // size = 14 + 30 + var // Unicode speech
		{
			BYTE m_Cmd;			// 0 = 0xAE
			NWORD m_len;		// 1-2 = var len size.
			NDWORD m_UID;		// 3-6 = UID num of speaker., 0xFFFFFFFF = noone.
			NWORD m_id;			// 7-8 = CREID_TYPE of speaker. 0xFFFF = none
			BYTE m_mode;		// 9 = TALKMODE_TYPE
			NWORD m_wHue;		// 10-11 = HUE_TYPE.
			NWORD m_font;		// 12-13 = FONT_TYPE
			char m_lang[4];		// language (same as before) CLanguageID
			char m_charname[MAX_NAME_SIZE];	//
			NCHAR m_utext[1];		// text (null terminated, ?=(msgsize-48)/2 MAX_TALK_BUFFER
		} SpeakUNICODE;

		struct // size = 21 + var // Send a gump menu dialog.
		{
			BYTE m_Cmd;		// 0 = 0xb0
			NWORD m_len;	// 1
			NDWORD m_UID;	// 3
			NDWORD m_context;	// 7-10= context info passed from server
			NDWORD m_x;
			NDWORD m_y;
			NWORD m_lenCmds; // 19-20 = command section length.
			char m_commands[1];	// Format commands (var len) { %s }
			BYTE m_zeroterm;
			NWORD m_textlines;	// How many lines of text to follow.
			struct
			{
				NWORD m_len;	// len of this line.
				NCHAR m_utext[1];	// UNICODE text. (not null term)
			} m_texts[1];
		} GumpDialog;

		struct
		{
			BYTE m_Cmd;			// 0 = 0xb2
			NWORD m_len;		// 1-2 = length of packet
			NWORD m_subcmd;		// 3-4 = 0x001a - already in this channel
			char m_lang[4];// 5-8 = unicode language code (null term....default = 'enu' (english?) CLanguageID
			NCHAR m_uname[1];		// 9-? = name in unicode (zero terminated) (may have  prefix, moderator, etc)
			NCHAR m_uname2[1];	// ?-? = name in unicode (also used for other things (passworded, etc)
		} ChatReq;

		struct
		{
#define MAX_ITEMS_PREVIEW (MAX_BUFFER - 16) / 10
			// MAX_ITEMS_PREVIEW = 1534
			BYTE m_Cmd; // 0 = 0xb4
			NWORD m_len; // 1-2 = 16 + (10 * m_count)
			BYTE m_fAllowGround; // 3 =
			NDWORD m_code; // 4-7 = target id
			NWORD m_xOffset; // 8-9 = x targ offset
			NWORD m_yOffset; // 10-11 = y targ offset
			NWORD m_zOffset; // 12-13 = z targ offset
			NWORD m_count;	// 14-15 = how many items in this preview image
			struct
			{
				NWORD m_id;	// 0-1 = ITEMID_TYPE
				NWORD m_dx;	// 2-3 = delta x
				NWORD m_dy;	// 4-5 = delta y
				NWORD m_dz;	// 6-7 = delta z
				NWORD m_unk;	// 6-7 = ???
			} m_item[MAX_ITEMS_PREVIEW];
		} TargetItems;

		struct // size = 64	// response to pressing the chat button
		{
			BYTE m_Cmd;		// 0 = 0xb5
			BYTE unk1;		// 1 = 0 (???)
			NCHAR m_uname[MAX_NAME_SIZE+1];// 2-63 - unicode name
		} Chat;

		struct	// size = 8 // Show Character Profile.
		{
			BYTE m_Cmd;		// 0=0xB8
			NWORD m_len;	// 1 - 2
			NDWORD m_UID;	// 3-6=uid
			char m_title[1];	// Description 1 (not unicode!, name, zero terminated)
			NCHAR m_utext1[1];	// Static Description 2 (unicode!, zero terminated)
			NCHAR m_utext2[1];	// Player Description 3 (unicode!, zero terminated)
		} CharProfile;

		struct // size = 3	// response to pressing the chat button
		{
#define CLI_FEAT_T2A_CHAT	0x0001
#define CLI_FEAT_LBR_SOUND	0x0002
#define CLI_FEAT_T2A_FULL	0x0004
#define CLI_FEAT_LBR_FULL	0x0008
#define CLI_FEAT_AOS		0x0010
#define CLI_FEAT_T2A		(CLI_FEAT_T2A_CHAT|CLI_FEAT_T2A_FULL)
#define CLI_FEAT_LBR		(CLI_FEAT_LBR_SOUND|CLI_FEAT_LBR_FULL)
#define CLI_FEAT_T2A_LBR	(CLI_FEAT_T2A|CLI_FEAT_LBR)
			BYTE m_Cmd;		// 0 = 0xb9
			NWORD m_enable;
		} FeaturesEnable;

		struct
		{
			BYTE m_Cmd;	// 0xba
			BYTE m_Active;	// 1/0
			NWORD m_x;
			NWORD m_y;
		} Arrow;

		struct	// Season, len = 3
		{
			BYTE m_Cmd; // 0=0xbc
			BYTE m_season;	// 0-5 = SEASON_TYPE, 5 = undead.
			BYTE m_cursor;  // 0/1 (0=gold / 1=normal cursor? Haven't checked really)
		} Season;

		struct	// we get walk codes from the > 1.26.0 server to be sent back in the Walk messages.
		{
			// walk codes from the > 1.26.0 server to be sent back in the Walk messages
			BYTE m_Cmd;	// 0=0xbf
			NWORD m_len;	// 1 - 2 (len = 5 + (# of codes * 4)
			NWORD m_type;	// 3 - 4 EXTDATA_TYPE = 1=prime LIFO stack, 2=add to beginning of LIFO stack
			CExtData m_u;
		} ExtData;

		struct
		{
			BYTE m_Cmd;			// 0 = 0xc1
			NWORD m_len;		// 1-2 = var len size.
			NDWORD m_UID;		// 3-6 = UID num of speaker. 01010101 = system
			NWORD m_id;			// 7-8 = CREID_TYPE of speaker.
			BYTE m_mode;		// 9 = TALKMODE_TYPE
			NWORD m_wHue;		// 10-11 = HUE_TYPE.
			NWORD m_font;		// 12-13 = FONT_TYPE
			NDWORD m_clilocId;	// 14-17 = Cliloc ID to display
			char m_charname[MAX_NAME_SIZE];	// 18-48
			TCHAR m_args[1];		// 49+ = arguments
		} SpeakLocalized;
		
		struct
		{
			BYTE m_Cmd; // 0=0xc4
			NDWORD m_UID;
			BYTE m_Intensity;
		} Semivisible;
		
		struct
		{
			BYTE m_Cmd; // 0xD6
			NWORD m_len;
			NWORD m_Unk1; // 0x0000
			NDWORD m_UID;
			NWORD m_Unk2; // 0x0003
			NDWORD m_ListID;
			struct
			{
				NDWORD m_LocID;
				NWORD m_textlen;
				TCHAR m_utext[1]; // this is NCHAR !!!
			} m_list[1];
		} AOSTooltip;
		
		struct
		{
			BYTE m_Cmd;	// 0=0xd7
			NWORD m_len;	// 1 - 2 (len = 5 + (# of codes * 4)
			NDWORD m_uid; 
			NWORD m_type;	// 3 - 4 EXTDATA_TYPE
			CExtAosData m_u;
		} ExtAosData;
		
		struct 
		{
			BYTE m_Cmd; // 0xd8
			NWORD m_len;
			NWORD m_compression; // 0x0000 not compressed
			NDWORD m_UID;
			NWORD m_Unk_1; // 0x0000
			NWORD m_Unk_2; // 0x0000
			NWORD n_itemcount;
			NWORD n_datasize; // itemcount * 5
			struct
			{
				NWORD m_dispid;
				BYTE m_x;
				BYTE m_y;
				BYTE m_z;
			} m_list[1];
		} AOSCustomHouse;


		struct
		{
			BYTE m_Cmd;	// 0 = 0xdc
			NDWORD m_uid;
			NDWORD m_ListID;
		} AOSTooltipInfo;

		struct // Send a gump menu dialog.
		{
			BYTE m_Cmd;		// 0 = 0xDD
			NWORD m_len;	// 1
			NDWORD m_UID;	// 3
			NDWORD m_context;	// 7-10= context info passed from server
			NDWORD m_x;
			NDWORD m_y;
			NDWORD m_compressed_lenCmds;
			NDWORD m_uncompressed_lenCmds;
			BYTE m_commands[1];	// Format commands (Lenght: m_compressed_lenCmds)
			NDWORD m_lineTxts;
			NDWORD m_compressed_lenTxts;
			NDWORD m_uncompressed_lenTxs;
			BYTE m_textlines[1]; // Text lines (Lenght: m_compressed_lenTxts)
		} CompressedGumpDialog;
	};
} PACK_NEEDED;

// Turn off structure packing.
#if defined _WIN32
#pragma pack()
#endif

#endif
