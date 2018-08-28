// Protocol formats.
// Define all the data passed from Client to Server.

#ifndef _INC_GRAYPROTO_H
#define _INC_GRAYPROTO_H
#pragma once

//---------------------------PROTOCOL DEFS---------------------------

// All these structures must be BYTE packed.
#if defined _WIN32 && (!__MINGW32__)
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
		UNREFERENCED_PARAMETER(iDefault);
		ASSERT(iDefault==0);
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
		TCHAR * pszTmp = Str_GetTemp();
		GetStr( pszTmp );
		return( pszTmp );
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
		return( false );
	}
};

enum XCMD_TYPE	// XCMD_* messages are unique in both directions.
{
	//	0x00
	XCMD_Create			= 0x00,
	XCMD_WalkRequest	= 0x02,
	XCMD_Talk			= 0x03,
	XCMD_Attack			= 0x05,
	XCMD_DClick			= 0x06,
	XCMD_ItemPickupReq	= 0x07,
	XCMD_ItemDropReq	= 0x08,
	XCMD_Click			= 0x09,
	XCMD_DamagePacket	= 0x0b,
	//	0x10
	XCMD_Status			= 0x11,
	XCMD_ExtCmd			= 0x12,
	XCMD_ItemEquipReq	= 0x13,
	XCMD_HealthBarColor	= 0x17,
	XCMD_Put			= 0x1a,
	XCMD_Start			= 0x1b,
	XCMD_Speak			= 0x1c,
	XCMD_Remove			= 0x1d,
	//	0x20
	XCMD_PlayerUpdate	= 0x20,
	XCMD_WalkReject		= 0x21,
	XCMD_WalkAck		= 0x22,
	XCMD_DragAnim		= 0x23,
	XCMD_ContOpen		= 0x24,
	XCMD_ContAdd		= 0x25,
	XCMD_Kick			= 0x26,
	XCMD_DragCancel		= 0x27,
	XCMD_DropRejected	= 0x28,
	XCMD_DropAccepted	= 0x29,
	XCMD_DeathMenu		= 0x2c,
	XCMD_ItemEquip		= 0x2e,
	XCMD_Fight			= 0x2f,
	//	0x30
	XCMD_CharStatReq	= 0x34,
	XCMD_Skill			= 0x3a,
	XCMD_VendorBuy		= 0x3b,
	XCMD_Content		= 0x3c,
	XCMD_StaticUpdate	= 0x3f,
	//	0x40
	XCMD_LightPoint		= 0x4e,
	XCMD_Light			= 0x4f,
	//	0x50
	XCMD_IdleWarning	= 0x53,
	XCMD_Sound			= 0x54,
	XCMD_LoginComplete	= 0x55,
	XCMD_MapEdit		= 0x56,
	XCMD_Time			= 0x5b,
	XCMD_CharPlay		= 0x5d,
	//	0x60
	XCMD_Weather		= 0x65,
	XCMD_BookPage		= 0x66,
	XCMD_Options		= 0x69,
	XCMD_Target			= 0x6c,
	XCMD_PlayMusic		= 0x6d,
	XCMD_CharAction		= 0x6e,
	XCMD_SecureTrade	= 0x6f,
	//	0x70
	XCMD_Effect			= 0x70,
	XCMD_BBoard			= 0x71,
	XCMD_War			= 0x72,
	XCMD_Ping			= 0x73,
	XCMD_VendOpenBuy	= 0x74,
	XCMD_CharName		= 0x75,
	XCMD_ZoneChange		= 0x76,
	XCMD_CharMove		= 0x77,
	XCMD_Char			= 0x78,
	XCMD_MenuItems		= 0x7c,
	XCMD_MenuChoice		= 0x7d,
	XCMD_UOGRequest		= 0x7F,
	//	0x80
	XCMD_ServersReq		= 0x80,
	XCMD_CharList3		= 0x81,
	XCMD_LogBad			= 0x82,
	XCMD_CharDelete		= 0x83,
	XCMD_DeleteBad		= 0x85,
	XCMD_CharList2		= 0x86,
	XCMD_PaperDoll		= 0x88,
	XCMD_CorpEquip		= 0x89,
	XCMD_GumpTextDisp	= 0x8b,
	XCMD_Relay			= 0x8c,
	XCMD_CreateNew		= 0x8d,
	//	0x90
	XCMD_MapDisplay		= 0x90,
	XCMD_CharListReq	= 0x91,
	XCMD_BookOpen		= 0x93,
	XCMD_DyeVat			= 0x95,
	XCMD_AllNames3D		= 0x98,
	XCMD_TargetMulti	= 0x99,
	XCMD_Prompt			= 0x9a,
	XCMD_HelpPage		= 0x9b,
	XCMD_VendOpenSell	= 0x9e,
	XCMD_VendorSell		= 0x9f,
	//	0xA0
	XCMD_ServerSelect	= 0xa0,
	XCMD_StatChngStr	= 0xa1,
	XCMD_StatChngInt	= 0xa2,
	XCMD_StatChngDex	= 0xa3,
	XCMD_Spy			= 0xa4,
	XCMD_Web			= 0xa5,
	XCMD_Scroll			= 0xa6,
	XCMD_TipReq			= 0xa7,
	XCMD_ServerList		= 0xa8,
	XCMD_CharList		= 0xa9,
	XCMD_AttackOK		= 0xaa,
	XCMD_GumpInpVal		= 0xab,
	XCMD_GumpInpValRet	= 0xac,
	XCMD_TalkUNICODE	= 0xad,
	XCMD_SpeakUNICODE	= 0xae,
	XCMD_CharDeath		= 0xaf,
	//	0xB0
	XCMD_GumpDialog		= 0xb0,
	XCMD_GumpDialogRet	= 0xb1,
	XCMD_ChatReq		= 0xb2,
	XCMD_ChatText		= 0xb3,
	XCMD_Chat			= 0xb5,
	XCMD_ToolTipReq		= 0xb6,
	XCMD_ToolTip		= 0xb7,
	XCMD_CharProfile	= 0xb8,
	XCMD_Features		= 0xb9,
	XCMD_Arrow			= 0xba,
	XCMD_MailMsg		= 0xbb,
	XCMD_Season			= 0xbc,
	XCMD_ClientVersion	= 0xbd,
	XCMD_ExtData		= 0xbf,
	//	0xC0
	XCMD_EffectEx		= 0xc0,
	XCMD_SpeakLocalized	= 0xc1,
	XCMD_PromptUNICODE	= 0xc2,
	XCMD_Semivisible	= 0xc4,
	XCMD_EffectParticle	= 0xc7,
	XCMD_ViewRange		= 0xc8,
	XCMD_GQCount		= 0xcb,
	XCMD_SpeakLocalizedEx	= 0xcc,
	//	0xD0
	XCMD_ConfigFile		= 0xd0,
	XCMD_LogoutStatus	= 0xd1,
	XCMD_CharMove2		= 0xd2,
	XCMD_DrawChar2		= 0xd3,
	XCMD_AOSBookPage	= 0xd4,
	XCMD_AOSTooltip		= 0xd6,
	XCMD_ExtAosData		= 0xd7,
	XCMD_AOSCustomHouse	= 0xd8,
	XCMD_Spy2			= 0xd9,
	XCMD_Majong			= 0xda,
	XCMD_CharTransferLog	= 0xdb,
	XCMD_AOSTooltipInfo	= 0xdc,
	XCMD_CompressedGumpDialog	= 0xdd,
	XCMD_BuffPacket		= 0xdf,
	//	0xE0
	XCMD_BugReport				= 0xe0,
	XCMD_KRClientType			= 0xe1,
	XCMD_NewAnimUpdate			= 0xe2,
	XCMD_EncryptionReq			= 0xe3,
	XCMD_EncryptionReply		= 0xe4,
	XCMD_WaypointShow			= 0xe5,
	XCMD_WaypointHide			= 0xe6,
	XCMD_HighlightUIContinue	= 0xe7,
	XCMD_HighlightUIRemove		= 0xe8,
	XCMD_HighlightUIToggle		= 0xe9,
	XCMD_ToggleHotbar			= 0xea,
	XCMD_UseHotbar				= 0xeb,
	XCMD_MacroEquipItem			= 0xec,
	XCMD_MacroUnEquipItem		= 0xed,
	XCMD_NewSeed				= 0xef,
	//	0xF0
	XCMD_WalkRequestNew		= 0xf0,
	XCMD_TimeSyncRequest	= 0xf1,
	XCMD_TimeSyncResponse	= 0xf2,
	XCMD_PutNew				= 0xf3,
	XCMD_CrashReport		= 0xf4,
	XCMD_MapDisplayNew		= 0xf5,
	XCMD_MoveShip			= 0xf6,
	XCMD_PacketCont			= 0xf7,
	XCMD_CreateHS			= 0xf8,
	XCMD_GlobalChat			= 0xf9,
	XCMD_UltimaStoreButton	= 0xfa
};

#define SEEDLENGTH_OLD (sizeof( DWORD ))
#define SEEDLENGTH_NEW (1 + sizeof( DWORD )*5)

enum PARTYMSG_TYPE
{
	PARTYMSG_Add = 1,	// (from client) Invite someone to join my party. or (from server) list who is in the party.
	PARTYMSG_Remove = 2,	// (to/from client) request to kick someone or notify that someone has been kicked.
	PARTYMSG_MsgMember = 3,	// (from client ) send a msg to a single member.
	PARTYMSG_Msg = 4,		// (to/from client) echos
	PARTYMSG_Disband,
	PARTYMSG_Option = 6,	// (from client) loot flag.
	PARTYMSG_NotoInvited = 7,	// (to client) I've been invited to join another party.
	PARTYMSG_Accept = 8,	// (from client) first
	PARTYMSG_Decline,
	PARTYMSG_QTY
};

enum EXTDATA_TYPE
{
	EXTDATA_Fastwalk_Init	= 0x01,	// send to client
	EXTDATA_Fastwalk_Add	= 0x02,	// send to client
	EXTDATA_Unk3,
	EXTDATA_GumpChange		= 0x04,	// len=8 "00 00 00 67 00 00 00 00"
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
	EXTDATA_CloseUI_Window	= 0x16,	// client message
	EXTDATA_Codex_Wisdom	= 0x17,	// server message
	EXTDATA_Map_Diff		= 0x18,	// enable mapdiff files
	EXTDATA_Stats_Enable	= 0x19,	// extended stats
	EXTDATA_Stats_Change	= 0x1a,	// extended stats
	EXTDATA_NewSpellbook	= 0x1b,
	EXTDATA_NewSpellSelect	= 0x1c,
	EXTDATA_HouseDesignVer	= 0x1d,	// server
	EXTDATA_HouseDesignDet	= 0x1e, // client
	//
	EXTDATA_HouseCustom		= 0x20,
	EXTDATA_Ability_Confirm	= 0x21,	// server (empty packet only id is required)
	EXTDATA_DamagePacketOld	= 0x22,	//  server
	//
	EXTDATA_Unk23			= 0x23,
	EXTDATA_AntiCheat		= 0x24, // Sent by SE clients, every second or so.
	EXTDATA_SpellSE			= 0x25,
	EXTDATA_SpeedMode		= 0x26, // server message
	//
	//
	//
	//
	//
	EXTDATA_BandageMacro	= 0x2c, // client message
	//
	//
	//
	//
	//
	EXTDATA_GargoyleFly		= 0x32, // client message
	EXTDATA_WheelBoatMove = 0x33, // client message
	EXTDATA_QTY
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
	EXTAOS_HcPlaceRoof = 0x13, // House Customization :: Place Roof Tile
	EXTAOS_HcDestroyRoof = 0x14, // House Customization :: Destroy Roof Tile
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

	EXTAOS_QTY
};

#define MAX_TALK_BUFFER		256	// how many chars can anyone speak all at once? (client speech is limited to 128 chars and journal is limited to 256 chars)

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

	struct	// EXTDATA_GumpChange = 0x04
	{
		NDWORD	dialogID;
		NDWORD	buttonID;
	} GumpChange;	// from server

	struct	// EXTDATA_ScreenSize = 0x05
	{
		NDWORD m_x;
		NDWORD m_y;
	} ScreenSize;	// from client

	struct	// EXTDATA_Arrow_Click = 0x07
	{
		BYTE m_rightClick;	// arrow was right clicked
	} QuestArrow;

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
		NWORD m_unk1; // 2
		NDWORD m_UID;
		BYTE m_NumPopups;
		struct
		{
			NDWORD m_TextID;
			NWORD m_EntryTag;
			NWORD m_Flags;
			char memreserve[112];
		} m_List;
	} KRPopup_Display;

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
			char memreserve[112];
		} m_List;
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
#ifdef _WIN32
		DWORD m_Content0;
		DWORD m_Content1;
#else
		NDWORD m_Content0;
		NDWORD m_Content1;
#endif
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

	struct  // EXTDATA_DamagePacketOld = 0x22 (LBR/AOS damage counter)
	{
		BYTE m_unk1;
		NDWORD m_UID;
		BYTE m_dmg;
	} DamagePacketOld;  // from server

	struct	// EXTDATA_Unk24
	{
		BYTE m_unk; // variable
	} AntiCheat; // from client

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

	struct // EXTDATA_GargoyleFly = 0x32
	{
		NWORD m_one;	// always 1
		DWORD m_zero;	// always 0
	} GargoyleFly;

}PACK_NEEDED;

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
		BYTE m_Unk_00_1; // 0x00
		NDWORD m_Roof;
		BYTE m_Unk_00_2; // 0x00
		NDWORD m_PosX;
		BYTE m_Unk_00_3; // 0x00
		NDWORD m_PosY;
		BYTE m_Unk_00_4; // 0x00
		NDWORD m_PosZ;
		BYTE m_Unk;		 // 0x0A
	} HousePlaceRoof;

	struct
	{
		BYTE m_Unk_00_1; // 0x00
		NDWORD m_Roof;
		BYTE m_Unk_00_2; // 0x00
		NDWORD m_PosX;
		BYTE m_Unk_00_3; // 0x00
		NDWORD m_PosY;
		BYTE m_Unk_00_4; // 0x00
		NDWORD m_PosZ;
		BYTE m_Unk;		 // 0x0A
	} HouseDestroyRoof;

	struct
	{
		BYTE m_Unk_0;	// 0x00
		NDWORD m_Ability;
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
	SECURE_TRADE_UPDATEGOLD = 3,
	SECURE_TRADE_UPDATELEDGER = 4
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
	SEASON_QTY
};

enum BBOARDF_TYPE	// Bulletin Board Flags. m_flag
{
	BBOARDF_NAME = 0,	// board name
	BBOARDF_MSG_HEAD,	// 1=message header, 
	BBOARDF_MSG_BODY,	// 2=message body
	BBOARDF_REQ_FULL,	// 3=request for full msg.
	BBOARDF_REQ_HEAD,	// 4=request for just head.
	BBOARDF_NEW_MSG,	// 5=new message, 
	BBOARDF_DELETE		// 6=Delete
};

enum EXTCMD_TYPE
{
	EXTCMD_SKILL			= 0x24,	// skill start. "skill number"
	EXTCMD_CAST_BOOK		= 0x27,	// cast spell from book. "spell number"
	EXTCMD_AUTOTARG			= 0x2f,	// bizarre new autotarget mode. "target x y z"
	EXTCMD_OPEN_SPELLBOOK	= 0x43,	// open spell book if we have one. "book type"
	EXTCMD_CAST_MACRO		= 0x56,	// macro spell. "spell number"
	EXTCMD_DOOR_AUTO		= 0x58,	// open door macro
	EXTCMD_ANIMATE			= 0xc7,	// "bow" or "salute"
	EXTCMD_INVOKE_VIRTUE	= 0xf4	// invoke virtue
};

enum CHATMSG_TYPE	// Chat system messages.
{
	// Messages (client <- server)
	CHATMSG_AlreadyIgnoringMax		= 0x1,		// 0x01 - You are already ignoring the maximum number of people.
	CHATMSG_AlreadyIgnoringPlayer,				// 0x02 - You are already ignoring %1.
	CHATMSG_NowIgnoring,						// 0x03 - You are now ignoring %1.
	CHATMSG_NoLongerIgnoring,					// 0x04 - You are no longer ignoring %1.
	CHATMSG_NotIgnoring,						// 0x05 - You are not ignoring %1.
	CHATMSG_NoLongerIgnoringAnyone,				// 0x06 - You are no longer ignoring anyone.
	CHATMSG_InvalidConferenceName,				// 0x07 - That is not a valid conference name.
	CHATMSG_DuplicatedConferenceName,			// 0x08 - There is already a conference of that name.
	CHATMSG_MustHaveOps,						// 0x09 - You must have operator status to do this.
	CHATMSG_ConferenceRenamed,					// 0x0A - Conference %1 renamed to %2.
	CHATMSG_MustBeInAConference,				// 0x0B - You must be in a conference to do this. To join a conference, select one from the Conference menu.
	CHATMSG_NoPlayer,							// 0x0C - There is no player named '%1'.
	CHATMSG_NoConference,						// 0x0D - There is no conference named '%1'.
	CHATMSG_IncorrectPassword,					// 0x0E - That is not the correct password.
	CHATMSG_PlayerIsIgnoring,					// 0x0F - %1 has chosen to ignore you. None of your messages to them will get through.
	CHATMSG_RevokedSpeaking,					// 0x10 - The moderator of this conference has not given you speaking privileges.
	CHATMSG_ReceivingPrivateMessages,			// 0x11 - You can now receive private messages.
	CHATMSG_NoLongerReceivingPrivateMessages,	// 0x12 - You will no longer receive private messages. Those who send you a message will be notified that you are blocking incoming messages.
	CHATMSG_ShowingName,						// 0x13 - You are now showing your character name to any players who inquire with the whois command.
	CHATMSG_NoLongerShowingName,				// 0x14 - You are no longer showing your character name to any players who inquire with the whois command.
	CHATMSG_PlayerIsAnonymous,					// 0x15 - %1 is remaining anonymous.
	CHATMSG_PlayerNotReceivingPrivateMessages,	// 0x16 - %1 has chosen to not receive private messages at the moment.
	CHATMSG_PlayerKnownAs,						// 0x17 - %1 is known in the lands of Britannia as %2.
	CHATMSG_PlayerKicked,						// 0x18 - %1 has been kicked out of the conference.
	CHATMSG_ModeratorHasKicked,					// 0x19 - %1, a conference moderator, has kicked you out of the conference.
	CHATMSG_AlreadyInConference,				// 0x1A - You are already in the conference '%1'.
	CHATMSG_PlayerNoLongerModerator,			// 0x1B - %1 is no longer a conference moderator.
	CHATMSG_PlayerIsModerator,					// 0x1C - %1 is now a conference moderator.
	CHATMSG_RemovedListModerators,				// 0x1D - %1 has removed you from the list of conference moderators.
	CHATMSG_YouAreAModerator,					// 0x1E - %1 has made you a conference moderator.
	CHATMSG_PlayerNoSpeaking,					// 0x1F - %1 no longer has speaking privileges in this conference.
	CHATMSG_PlayerNowSpeaking,					// 0x20 - %1 now has speaking privileges in this conference.
	CHATMSG_ModeratorHasRemovedSpeakPriv,		// 0x21 - %1, a conference moderator, has removed your speaking privileges for this conference.
	CHATMSG_ModeratorHasGrantedSpeakPriv,		// 0x22 - %1, a conference moderator, has granted you speaking privileges in this conference.
	CHATMSG_EveryoneSpeakingPrivByDefault,		// 0x23 - From now on, everyone in the conference will have speaking privileges by default.
	CHATMSG_ModeratorSpeakingPrivByDefault,		// 0x24 - From now on, only moderators will have speaking privileges in this conference by default.
	CHATMSG_PlayerMessage,						// 0x25 - <name>: <msg>
	CHATMSG_PlayerEmote,						// 0x26 - *<name> <msg>*
	CHATMSG_PlayerPrivateMessage,				// 0x27 - [<name>]: <msg>
	CHATMSG_PasswordChanged,					// 0x28 - The password to the conference has been changed.
	CHATMSG_ConferenceIsFull,					// 0x29 - Sorry--the conference named '%1' is full and no more players are allowed in.
	CHATMSG_Banning,							// 0x2A - You are banning %1 from this conference.
	CHATMSG_ModeratorHasBanned,					// 0x2B - %1, a conference moderator, has banned you from the conference.
	CHATMSG_Banned,								// 0x2C - You have been banned from this conference.
	CHATMSG_TrialAccOnlyHelpChannel	= 0x3F5,	// 0x3F5 - Trial accounts may only join the Help channel.
	CHATMSG_TrialAccNoCustomChannel,			// 0x3F6 - Trial accounts may not participate in custom channels.

	// Actions (client -> server)				// OLD CHAT		NEW CHAT
	CHATACT_ChangeChannelPassword	= 0x41,		// x
	CHATACT_LeaveChannel			= 0x43,		//				x
	CHATACT_LeaveChat				= 0x58,		// x
	CHATACT_ChannelMessage			= 0x61,		// x			x
	CHATACT_JoinChannel				= 0x62,		// x			x
	CHATACT_CreateChannel			= 0x63,		// x			x
	CHATACT_RenameChannel			= 0x64,		// x
	CHATACT_PrivateMessage			= 0x65,		// x
	CHATACT_AddIgnore				= 0x66,		// x
	CHATACT_RemoveIgnore			= 0x67,		// x
	CHATACT_ToggleIgnore			= 0x68,		// x
	CHATACT_AddVoice				= 0x69,		// x
	CHATACT_RemoveVoice				= 0x6A,		// x
	CHATACT_ToggleVoice				= 0x6B,		// x
	CHATACT_AddModerator			= 0x6C,		// x
	CHATACT_RemoveModerator			= 0x6D,		// x
	CHATACT_ToggleModerator			= 0x6E,		// x
	CHATACT_EnablePrivateMessages	= 0x6F,		// x
	CHATACT_DisablePrivateMessages	= 0x70,		// x
	CHATACT_TogglePrivateMessages	= 0x71,		// x
	CHATACT_ShowCharacterName		= 0x72,		// x
	CHATACT_HideCharacterName		= 0x73,		// x
	CHATACT_ToggleCharacterName		= 0x74,		// x
	CHATACT_WhoIs					= 0x75,		// x
	CHATACT_Kick					= 0x76,		// x
	CHATACT_EnableDefaultVoice		= 0x77,		// x
	CHATACT_DisableDefaultVoice		= 0x78,		// x
	CHATACT_ToggleDefaultVoice		= 0x79,		// x
	CHATACT_EmoteMessage			= 0x7A,		// x

	// Commands (client <- server)				// OLD CHAT		NEW CHAT
	CHATCMD_AddChannel				= 0x3E8,	// x			x
	CHATCMD_RemoveChannel,						// x			x
	CHATCMD_SetChatName				= 0x3EB,	// x
	CHATCMD_CloseChatWindow,					// x
	CHATCMD_OpenChatWindow,						// x
	CHATCMD_AddMemberToChannel,					// x
	CHATCMD_RemoveMemberFromChannel,			// x
	CHATCMD_ClearMembers,						// x
	CHATCMD_JoinedChannel,						// x			x
	CHATCMD_LeftChannel				= 0x3F4		//				x
};

enum INPVAL_STYLE	// for the various styles for InpVal box.
{
	INPVAL_STYLE_NOEDIT		= 0,	// No textbox, just a message
	INPVAL_STYLE_TEXTEDIT	= 1,	// Alphanumeric
	INPVAL_STYLE_NUMEDIT	= 2		// Numeric
};

enum MAPCMD_TYPE
{
	MAPCMD_AddPin				= 0x1,
	MAPCMD_InsertPin			= 0x2,
	MAPCMD_MovePin				= 0x3,
	MAPCMD_RemovePin			= 0x4,
	MAPCMD_ClearPins			= 0x5,
	MAPCMD_ToggleEdit_Request	= 0x6,
	MAPCMD_ToggleEdit_Reply		= 0x7
};

enum MAPWAYPOINT_TYPE
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

enum WEATHER_TYPE
{
	WEATHER_DEFAULT = 0xFE,
	WEATHER_DRY = 0xFF,
	WEATHER_RAIN = 0,
	WEATHER_STORM,
	WEATHER_SNOW,
	WEATHER_CLOUDY	// not client supported ? (Storm brewing)
};

enum SCROLL_TYPE	// Client messages for scrolls types.
{
	SCROLL_TYPE_TIPS = 0,	// type = 0 = TIPS
	SCROLL_TYPE_NOTICE = 1,
	SCROLL_TYPE_UPDATES = 2	// type = 2 = UPDATES
};

enum EFFECT_TYPE
{
	EFFECT_BOLT,		// Flying bolt
	EFFECT_LIGHTNING,	// Lightning bolt
	EFFECT_XYZ,			// Ground-based effect
	EFFECT_OBJ,			// Object-based effect
	EFFECT_FADE_SCREEN	// Fade client screen (only available on clients >= 6.0.0.0)
};

enum NOTO_TYPE
{
	NOTO_INVALID = 0,	// 0= not a valid color!!
	NOTO_GOOD,			// 1= good(blue),
	NOTO_GUILD_SAME,	// 2= same guild,
	NOTO_NEUTRAL,		// 3= Neutral,
	NOTO_CRIMINAL,		// 4= criminal
	NOTO_GUILD_WAR,		// 5= Waring guilds,
	NOTO_EVIL,			// 6= evil(red),
	NOTO_INVUL			// 7= invulnerable
};

// Client versions (expansions)
#define MINCLIVER_T2A				1253502	// minimum client to activate T2A packets (1.25.35b)
//#define MINCLIVER_R				2000000	// minimum client to activate R packets (2.0.0a)
//#define MINCLIVER_TD				3000000	// minimum client to activate TD packets (3.0.0a)
#define MINCLIVER_LBR				3000702	// minimum client to activate LBR packets (3.0.7b)
#define MINCLIVER_AOS				4000000	// minimum client to activate AOS packets (4.0.0a)
#define MINCLIVER_SE				4000500	// minimum client to activate SE packets (4.0.5a)
#define MINCLIVER_ML				5000000	// minimum client to activate ML packets (5.0.0a)
#define MINCLIVER_SA				7000000	// minimum client to activate SA packets (7.0.0.0)
#define MINCLIVER_HS				7000900	// minimum client to activate HS packets (7.0.9.0)
#define MINCLIVER_TOL				7004565	// minimum client to activate TOL packets (7.0.45.65)

// Client versions (extended status gump info)
#define MINCLIVER_STATUS_V2			3000804	// minimum client to receive v2 of 0x11 packet (3.0.8d)
#define MINCLIVER_STATUS_V3			3000810	// minimum client to receive v3 of 0x11 packet (3.0.8j)
#define MINCLIVER_STATUS_V4			4000000	// minimum client to receive v4 of 0x11 packet (4.0.0a)
#define MINCLIVER_STATUS_V5			5000000	// minimum client to receive v5 of 0x11 packet (5.0.0a)
#define MINCLIVER_STATUS_V6			7003000	// minimum client to receive v6 of 0x11 packet (7.0.30.0)

// Client versions (behaviours)
//#define MINCLIVER_CHECKWALKCODE	1260000	// minimum client to use walk crypt codes for fastwalk prevention (1.26.0a)
//#define MINCLIVER_CLILOCS			1260202	// minimum client to use clilocs (1.26.2b)
#define MINCLIVER_PADCHARLIST		3000010	// minimum client to pad character list to at least 5 characters
#define MINCLIVER_CLOSEDIALOG		4000400	// minimum client where close dialog does not trigger a client response
#define MINCLIVER_NEWVERSIONING		5000605	// minimum client to use new versioning format (after 5.0.6e it change to 5.0.6.5)
#define MINCLIVER_CONTAINERGRID		6000107	// minimum client to use container grid index (6.0.1.7)
#define MINCLIVER_NEWCHATSYSTEM		7000400	// minimum client to use new chat system (7.0.4.0)
#define MINCLIVER_GLOBALCHAT		7006202	// minimum client to use global chat system (7.0.62.2)

// Client versions (packets)
#define MINCLIVER_STATLOCKS			4000100	// minimum client to receive 0xBF.0x19.0x02 packet
#define MINCLIVER_NEWDAMAGE			4000700	// minimum client to receive 0x0B packet (4.0.7a)
#define MINCLIVER_BUFFS				5000202	// minimum client to receive buff packets (5.0.2b)
#define MINCLIVER_NEWCONTEXTMENU	6000000	// minimun client to receive 0xBF.0x14.0x02 packet instead of 0xBF.0x14.0x01 (6.0.0.0)
#define MINCLIVER_EXTRAFEATURES		6001402	// minimum client to receive 4-byte feature mask (6.0.14.2)
#define MINCLIVER_NEWMAPDISPLAY		7000800	// minimum client to receive 0xF5 packet (7.0.8.0)
#define MINCLIVER_EXTRASTARTINFO 	7001300	// minimum client to receive extra start info (7.0.13.0)
#define MINCLIVER_NEWMOBINCOMING	7003301	// minimun client to receive 0x78 packet (7.0.33.1)

// Enhanced clients report version to server using masks, so the formula 'reported version - mask' must be used to get the real value.
// Eg: SA enhanced client 4.0.52.0 will report 67.0.52.0 to server, so the real value is 67.0.52.0 - mask = 4.0.52.0
#define MASK_CLIENTTYPE_EC			63000000	// SA+ enhanced clients starts on version 4.0.0.0

enum TALKMODE_TYPE	// Modes we can talk/bark in.
{
	TALKMODE_SYSTEM = 0,		// 0 = Normal system message
	TALKMODE_PROMPT,			// 1 = Display as system prompt
	TALKMODE_EMOTE,				// 2 = *smiles* at object (client shortcut: :+space)
	TALKMODE_SAY,				// 3 = A chacter speaking.
	TALKMODE_OBJ,				// 4 = At object
	TALKMODE_NOTHING,			// 5 = Does not display
	TALKMODE_ITEM,				// 6 = text labeling an item. Preceeded by "You see"
	TALKMODE_NOSCROLL,			// 7 = As a status msg. Does not scroll
	TALKMODE_WHISPER,			// 8 = Only those close can here. (client shortcut: ;+space)
	TALKMODE_YELL,				// 9 = Can be heard 2 screens away. (client shortcut: !+space)
	TALKMODE_SPELL,				// 10 = Used by spells WOP
	TALKMODE_GUILD = 0xD,		// 13 = Used by guild chat (client shortcut: \)
	TALKMODE_ALLIANCE,			// 14 = Used by alliance chat (client shortcut: shift+\)
	TALKMODE_GM_COMMAND,		// 15 = GM command input (client shortcut: .). Only available when using GM body (CREID_EQUIP_GM_ROBE)
	TALKMODE_ENCODED = 0xC0,
	TALKMODE_BROADCAST = 0xFF
};

enum SKILLLOCK_TYPE
{
	SKILLLOCK_UP = 0,
	SKILLLOCK_DOWN,
	SKILLLOCK_LOCK
};

enum DEATH_MODE_TYPE	// DeathMenu
{
	DEATH_MODE_MANIFEST = 0,
	DEATH_MODE_RES_IMMEDIATE,
	DEATH_MODE_PLAY_GHOST
};

enum DELETE_ERR_TYPE
{
	DELETE_ERR_BAD_PASS = 0, // 0 That character password is invalid.
	DELETE_ERR_NOT_EXIST,	// 1 That character does not exist.
	DELETE_ERR_IN_USE,	// 2 That character is being played right now.
	DELETE_ERR_NOT_OLD_ENOUGH, // 3 That character is not old enough to delete. The character must be 7 days old before it can be deleted.
	DELETE_SUCCESS = 255
};

enum BUGREPORT_TYPE	// bug report codes
{
	BUGREPORT_ENVIRONMENT	= 0x01,
	BUGREPORT_WEARABLES		= 0x02,
	BUGREPORT_COMBAT		= 0x03,
	BUGREPORT_UI			= 0x04,
	BUGREPORT_CRASH			= 0x05,
	BUGREPORT_STUCK			= 0x06,
	BUGREPORT_ANIMATIONS	= 0x07,
	BUGREPORT_PERFORMANCE	= 0x08,
	BUGREPORT_NPCS			= 0x09,
	BUGREPORT_CREATURES		= 0x0A,
	BUGREPORT_PETS			= 0x0B,
	BUGREPORT_HOUSING		= 0x0C,
	BUGREPORT_LOST_ITEM		= 0x0D,
	BUGREPORT_EXPLOIT		= 0x0E,
	BUGREPORT_OTHER			= 0x0F
};

enum PROFESSION_TYPE	// profession ids
{
	PROFESSION_ADVANCED		= 0x00,
	PROFESSION_WARRIOR		= 0x01,
	PROFESSION_MAGE			= 0x02,
	PROFESSION_BLACKSMITH	= 0x03,
	PROFESSION_NECROMANCER	= 0x04,
	PROFESSION_PALADIN		= 0x05,
	PROFESSION_SAMURAI		= 0x06,
	PROFESSION_NINJA		= 0x07
};

enum GAMECLIENT_TYPE	// game client type, KR and SA are from the 0xE1 packet, other values are for convenience
{
	CLIENTTYPE_2D	= 0x0,	// 2D classic client
	CLIENTTYPE_3D	= 0x1,	// 3D classic client
	CLIENTTYPE_KR	= 0x2,	// KR client
	CLIENTTYPE_EC	= 0x3	// Enhanced client
};

enum RACE_TYPE		// character race, used in new character creation (0x8D) and status (0x11) packets
{
	RACETYPE_UNDEFINED	= 0x0,
	RACETYPE_HUMAN		= 0x1,
	RACETYPE_ELF		= 0x2,
	RACETYPE_GARGOYLE	= 0x3
};

struct CEventCharDef
{
#define MAX_ITEM_NAME_SIZE	256		// imposed by client for protocol
#define MAX_NAME_SIZE 30
#define ACCOUNT_NAME_VALID_CHAR " !\"#$%&()*,/:;<=>?@[\\]^{|}~"
#define MAX_ACCOUNT_NAME_SIZE MAX_NAME_SIZE
#define MAX_ACCOUNT_PASSWORD_ENTER 16	// client only allows n chars.
	char m_charname[MAX_ACCOUNT_NAME_SIZE];
	char m_charpass[MAX_NAME_SIZE];	// but the size of the structure is 30 (go figure)
};

struct CEvent	// event buffer from client to server..
{
#define MAX_BUFFER			15360	// Buffer Size (For socket operations)
#define MAX_ITEMS_CONT		125		// Max items in a container. (arbitrary)
#define MAX_MENU_ITEMS		64		// number of items in a menu. (arbitrary)
#define MAX_CHARS_PER_ACCT	7

	// Some messages are bydirectional.

	union
	{
		BYTE m_Raw[ MAX_BUFFER ];

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

		struct	// size = 7
		{
			BYTE m_Cmd;		// 0 = 0x02
			BYTE m_dir;		// 1 = DIR_TYPE (| 0x80 = running)
			BYTE m_count; 	// 2 = just a count that goes up as we walk. (handshake)
			NDWORD m_cryptcode;
		} Walk;

		struct // size = 38
		{
			BYTE m_Cmd;		// 0     = 0xF0  XCMD_WalkRequestNew
			NWORD m_len;	// 1-2   = length of packet
			BYTE m_unk1;	// 3     = 01
			NDWORD m_unk2;	// 4-7   = 00 00 01 22
			NDWORD m_unk3;	// 8-11  = AE . DF 64
			NDWORD m_unk4;	// 12-15 = 00 00 01 22
			NDWORD m_unk5;	// 16-19 = AE 8A BC 1A
			BYTE m_count;	// 20    = increments every packet
			BYTE m_dir;		// 21    = new direction
			NDWORD m_mode;	// 22-25 = movement mode (1 = walk, 2 = run)
			NDWORD m_x;		// 26-29 = new x position
			NDWORD m_y;		// 30-33 = new y position
			NDWORD m_z;		// 34-37 = new z position
		} WalkNew;

		struct // size = 9
		{
			BYTE m_Cmd;		// 0   = 0xF1  XCMD_TSyncRequest
			NDWORD m_unk1;	// 1-4 = 00 00 01 22
			NDWORD m_unk2;	// 5-8 = AE .. E1 0E
		} TSyncRequest;

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

		struct	// size = 15 = drop item on ground or in container.
		{
			BYTE m_Cmd;			// 0 = 0x08
			NDWORD m_UID;		// 1-4 = object being dropped.
			NWORD m_x;		// 5,6 = 255 = invalid
			NWORD m_y;		// 7,8
			BYTE m_z;			// 9
			BYTE m_grid;		// 10	(Client 6.0.1.7+)
			NDWORD m_UIDCont;	// 11 = dropped on this object. 0xFFFFFFFF = no object
		} ItemDropReqNew;

		struct	// size > 3
		{
			BYTE m_Cmd;		// 0 = 0x12
			NWORD m_len;	// 1-2 = len
			BYTE m_type;	// 3 = 0xc7=anim, 0x24=skill...
			char m_name[1];	// 4=var size string
		} ExtCmd;

		struct	// size = 10 = item dropped on paper doll.
		{
			BYTE m_Cmd;		// 0 = 0x13
			NDWORD m_UID;	// 1-4	item UID.
			BYTE m_layer;	// LAYER_TYPE
			NDWORD m_UIDChar;
		} ItemEquipReq;

		struct // size = ? reserved GM tool message.
		{
			BYTE m_Cmd;		// 0 = 0x11
			NWORD m_len;	// 1-2 = len
			BYTE m_type;	// 3 = sub type

		} GMToolMsg;

		struct // size = 3	// WalkAck gone bad = request a resync
		{
			BYTE m_Cmd;		// 0 = 0x22
			BYTE m_unk1[2];
		} WalkAck;

		struct // size = 7(m_mode==0) or 2(m_mode!=0)  // Manifest ghost (War Mode) or auto res ?
		{
			BYTE m_Cmd;		// 0 = 0x2c
			BYTE m_mode;	// 1 = DEATH_MODE_TYPE 0=manifest or unmanifest, 1=res w/penalties, 2=play as a ghost 
			BYTE m_unk2;	// 2 = 72 or 73
			BYTE m_manifest;// 3 = manifest or not. = war mode.
			BYTE m_unk4[3]; // 4 = unk = 00 32 00
		} DeathMenu;

		struct	// size = 10	// Client requests stats.
		{
			BYTE m_Cmd;		// 0 =0x34
			NDWORD m_edededed;	// 1-4 = 0xedededed
			BYTE m_type;	// 5 = 4 = Basic Stats (Packet 0x11 Response)  5 = Request Skills (Packet 0x3A Response)
			NDWORD m_UID;	// 6 = character UID for status
		} CharStatReq;

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

		struct // size = variable // Buy item from vendor.
		{
			BYTE m_Cmd;			// 0 =0x3b
			NWORD m_len;		// 1-2=
			NDWORD m_UIDVendor;	// 3-6=
			BYTE m_flag;	// 0x00 = no items following, 0x02 - items following
			struct
			{
				BYTE m_layer; // (0x1A)
				NDWORD m_UID; // itemID (from 3C packet)
				NWORD m_amount; //  # bought
			} m_item[1];
		} VendorBuy;

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

#define MAX_BOOK_PAGES 64	// arbitrary max number of pages.
#define MAX_BOOK_LINES 8	// max lines per page.

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

		struct	// size = var // Client text Hue changed but it does not tell us to what !!
		{
			BYTE m_Cmd;		// 0=0x69
			NWORD m_len;
			NWORD m_index;
		} Options;

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

		struct // size = 2	// ping goes both ways.
		{
			BYTE m_Cmd;		// 0 = 0x73
			BYTE m_bCode;	// 1 = seems to make no diff
		} Ping;

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
			BYTE m_loginKey;	// 61 = NextLoginKey from uo.cfg
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
			NWORD m_len;	// 1-2
			NDWORD m_serial;	// 3-6 = serial
			NDWORD m_prompt;	// 7-10 = prompt id
			NDWORD m_type;	// 11-14 = type (0=request/esc, 1=reply)
			char m_text[1];	// 15+ = null terminated text.
		} Prompt;

		struct // size = 0x102	// GM page = request for help.
		{
			BYTE m_Cmd;		// 0 = 0x9b
			BYTE m_unk1[0x101];
		} HelpPage;

		struct // size = var // Sell stuff to vendor
		{
			BYTE m_Cmd;		// 0=0x9F
			NWORD m_len;	// 1-2
			NDWORD m_UIDVendor;	// 3-6
			NWORD m_count;		// 7-8
			struct
			{
				NDWORD m_UID;
				NWORD m_amount;
			} m_item[1];
		} VendorSell;

		struct	// size = 3;	// relay me to the server i choose.
		{
			BYTE m_Cmd;		// 0 = 0xa0
			NWORD m_select;	// 2 = selection this server number
		} ServerSelect;

		struct	// sizeof = 149 // not sure what this is for.
		{
			BYTE m_Cmd;		// 0=0xA4
			BYTE  m_ProcessorType;		// @0 = 03
			WORD m_ProcessorClock;		// @1 = 01 8E
			BYTE  m_nProcessors;		// @3 = 01
			WCHAR m_wDirectory[0x10];	// @4 = 00 00
			BYTE m_szVideoCardDescrip2[0x20]; // @24 = AA 7E 3F 04 00
			BYTE m_szModemManufacturer[0x10];	// @44 = 40 F8
			BYTE m_Unk54[0x30];
			BYTE m_bUnk84;				// @84 = 0
			WORD m_totalRAMinMB;		// @85 = ff 1e
			WORD m_largestPartitionInMB; // @87 = 2e 00 
			WORD m_Unk8a;					// @89 = 00 01
			DWORD m_timeZoneBias;		// @8b = 2c 77 f8 37
			BYTE m_bUnk8f;				// @8f = 6a
			DWORD m_dwUnk90;			// @90 = 20 00 00 00

			// ? m_szModemDescrip
			// ? m_szVideoCardDescrip1

		} Spy;

		struct // size = 5	// scroll close
		{
			BYTE m_Cmd;			// 0=0xA6
			NDWORD m_context;	// 1= context info passed from server
		} Scroll;

		struct	// size = 4 // request for tip
		{
			BYTE m_Cmd;		// 0=0xA7
			NWORD m_index;
			BYTE m_type;	// 0=tips, 1=notice
		} TipReq;

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

		struct // size = 9	// request for tooltip
		{
			BYTE m_Cmd;			// 0 = 0xb6
			NDWORD m_UID;		// 1-4
			BYTE m_langID;	// 5 = 1 = english CLanguageID
			BYTE m_lang[3];	// 6-8 = ENU = english
		} ToolTipReq;

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

		struct // size = 21+var = console Prompt response.
		{
			BYTE m_Cmd;			// 0xc2
			NWORD m_len;		// 1-2
			NDWORD m_serial;	// 3-6 = serial
			NDWORD m_prompt;	// 7-10 = prompt id
			NDWORD m_type;		// 11-14 = type (0=request/esc, 1=reply)
			char m_lang[4];		// 15-18 = language (3 chars + NULL)
			WCHAR m_utext[1];	// 19+ = null terminated unicode text.
		} PromptUNICODE;
		
		struct	// size = ?? // Config File (IGR)
		{
			BYTE m_Cmd;		// 0=0xD0
			NWORD m_len;	// 1 - 2
			BYTE m_type;	// file type
			BYTE m_config_data[1]; // Data m_len-4
		} ConfigFile;
		
		struct	// size = 2 // Logout Confirm
		{
			BYTE m_Cmd;		// 0=0xD1
			BYTE m_unk;	// always 1
		} LogoutStatus;

		struct // size = 17+var // Get a change to the book title.
		{
			BYTE m_Cmd;			// 0 = 0xD4
			NWORD m_len;		// 1-2 = length
			NDWORD m_UID;		// 3-6 = book
			BYTE m_unk1;		// 7 = 0x00
			BYTE m_writable;	// 8 = 0 = non writable, 1 = writable
			NWORD m_pages;		// 9-10 = number of pages
			NWORD m_titlelen;	// 11-12 = title length
			char m_title[1];	// 13 - = title
			NWORD m_authorlen;	// = author length
			char m_author[1];	// = author
		} BookOpenNew;
		
		struct	// sizeof = 268 // Spy2.
		{
			BYTE m_Cmd;		// 0=0xD9
			NWORD m_len;
			BYTE m_client; // 1 = client version < 4.0.1a; 0 = client version >= 4.0.1a
			NDWORD m_IstanceId;
			NDWORD m_OSMajor;
			NDWORD m_OSMinor;
			NDWORD m_OSRevision;
			BYTE m_CPUManufacter;
			NDWORD m_CPUFamily;
			NDWORD m_CPUModel;
			NDWORD m_CPUClockSpeed;
			BYTE m_PhysicalMemory;
			NDWORD m_ScreenWidth;
			NDWORD m_ScreenHeight;
			NDWORD m_ScreenDepth;
			NWORD m_DXMajor;
			NWORD m_DXMinor;
			char m_VCDesc[64];
			NDWORD m_VCVendor;
			NDWORD m_VCDevice;
			NDWORD m_VCMemory;
			BYTE m_Distribution;
			BYTE m_ClientRunning;
			BYTE m_ClientInstalled;
			BYTE m_PartialInstalled;
			char m_Language[4];
			char m_Unknown[64];
		} Spy2;

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

		struct // XCMD_CreateNew, size = ? // create a new char (KR/SA)
		{
			BYTE m_Cmd;						// 0 = 0x8D
			NWORD m_len;					// 1 - 2 = length
			NDWORD m_pattern1;				// 3 - 6 = 0xedededed
			NDWORD m_pattern2;				// 7 - 10 = 0xffffffff
			char m_charname[MAX_NAME_SIZE];	// 11 - 40 = ascii name
			char m_unknown[30];				// 41 - 70 = "Unknown"
			BYTE m_profession;				// 71 = profession (0=custom)
			BYTE m_unk1;	// 72;
			BYTE m_sex;						// 73 = sex (0=male, 1=female)
			BYTE m_race;					// 74 = race (1=human, 2=elf, 3=gargoyle) (RACE_TYPE)
			BYTE m_str;						// 75 = str
			BYTE m_dex;						// 76 = dex
			BYTE m_int;						// 77 = int
			NWORD m_wSkinHue;				// 78 - 79 = skin hue
			BYTE m_unk2[8];	// 80 - 87

			// skill values only used if m_prof=0
			BYTE m_skill1;					// 88 = skill #1
			BYTE m_val1;					// 89 = skill #1 val
			BYTE m_skill2;					// 90 = skill #2
			BYTE m_val2;					// 91 = skill #2 val
			BYTE m_skill4;					// 92 = skill #4
			BYTE m_val4;					// 93 = skill #4 val
			BYTE m_skill3;					// 94 = skill #3
			BYTE m_val3;					// 94 = skill #3 val

			BYTE m_unk3[26];	// 95 - 121
			NWORD m_hairHue;				// 122 - 123 = hair hue
			NWORD m_hairId;					// 124 - 125 = hair id
			BYTE m_unk4[11];	// 126 - 136
			NWORD m_wSkinHue2;				// 137 - 138 = skin hue (included twice?)
			BYTE m_unk5;	// 139
			BYTE m_portrait;				// 140 = portrait id (could be NWORD)
			BYTE m_unk6;	// 141
			NWORD m_beardHue;				// 142 - 143 = beard hue
			NWORD m_beardId;				// 144 - 145 = beard id
		} CreateKR;

		struct // XCMD_BugReport
		{
			BYTE m_Cmd;			// 0=0xe0
			NWORD m_len;		// 1 - 2 (len = )
			char m_Language[4];	// 3 - 6 (lang, ENU)
			NWORD m_type;		// 7 - 8 (bug type)
			NCHAR m_utext[1];	// 9 - ? (NCHAR[?] text)
		} BugReport;

		struct // XCMD_KRClientType
		{
			BYTE m_Cmd;			// 0=0xe1
			NWORD m_len;		// 1 - 2 (len = )
			NWORD m_one;		// 3 - 4 (0x01)
			NDWORD m_clientType;		// 5 - 8 (0x02 = KR, 0x03 = SA)
		} KRClientType;

		struct // XCMD_EncryptionReply
		{
			BYTE m_Cmd;			// 0 = 0xE4
			NWORD m_len;		// 1 - 2 = length
			NDWORD m_lenUnk1;				// 3 - 6 = length of m_unk1
			BYTE m_unk1[1];					// 7 - ? = ?
		} EncryptionReply;

		struct // XCMD_HighlightUIRemove
		{
			BYTE m_Cmd;				// 0 = 0xE8
			NDWORD m_Uid;			// 1 - 4 = serial uid
			NWORD m_IdUi;			// 5 - 6 = id ui
			NDWORD m_destUid;		// 7 - 11 = destination serial uid
			BYTE m_One;				// 12 = 0x01
			BYTE m_One2;			// 13 = 0x01
		} HighlightUIRemove;

		struct // XCMD_UseHotbar
		{
			BYTE m_Cmd;				// 0 = 0xEB
			NWORD m_One;			// 1 - 2 = 0x01
			NWORD m_Six;			// 3 - 4 = 0x06
			BYTE m_Type;			// 5 = 0x1 � spell, 0x2 � weapon ability, 0x3 � skill, 0x4 � item, 0x5 � scroll
			BYTE m_Zero;			// 6 = 0x00
			NDWORD m_ObjectUID;		// 7 - 11 = serial uid
		} UseHotbar;

		struct // XCMD_MacroEquipItem
		{
			BYTE m_Cmd;			// 0=0xec
			NWORD m_len;		// 1 - 2 (len)
			BYTE m_count;		// 3	 (number of layers)
			NDWORD m_items[1];	// 4 - ? (items to equip)
		} MacroEquipItems;

		struct // XCMD_MacroUnEquipItem
		{
			BYTE m_Cmd;			// 0=0xed
			NWORD m_len;		// 1 - 2 (len)
			BYTE m_count;		// 3	 (number of layers)
			NWORD m_layers[1];	// 4 - ? (layers to unequip)
		} MacroUnEquipItems;

		struct // XCMD_NewSeed		// 21 bytes
		{
			BYTE m_Cmd;				// 0 = 0xEF
			NDWORD m_Seed;			// 1 - 4   = seed
			NDWORD m_Version_Maj;	// 5 - 8   = Ver Major
			NDWORD m_Version_Min;	// 9 - 12  = Ver Minor
			NDWORD m_Version_Rev;	// 13 - 16 = Ver Revision
			NDWORD m_Version_Pat;	// 17 - 20 = Ver Patch
		} NewSeed;

		struct // XCMD_PutNew	// size = 24	// draw the item at a location
		{
			BYTE m_Cmd;		// 0 = 0xF3
			NWORD m_unk;	// 1-2 = 0x001
			BYTE m_type;	// 3 = type (0x0=TileData, 0x2=Multi)
			NDWORD m_UID;	// 4-7 = UID | UID_O_ITEM | UID_F_RESOURCE
			NWORD m_id;		// 8-9
			BYTE m_dir;		// 10 = direction
			NWORD m_amount;	// 11-12 = amount
			NWORD m_unkAmount; // 13-14 = amount, no noticable effect on client
			NWORD m_x;		// 15-18 = x
			NWORD m_y;		// 19-22 = y
			BYTE m_z;		// 23 = signed char
			BYTE m_layer;	// 24 = item layer
			NWORD m_wHue;	// 25-26 = HUE_TYPE
			BYTE m_flags;	// 27 = 0x20 = is it movable, 0x80 = hidden
		} PutNew;

		struct // XCMD_CrashReport	// 310+ bytes
		{
			BYTE m_Cmd;					// 0 = 0xF4
			NWORD m_len;				// 1-2 = length
			BYTE m_versionMaj;			// 3 = exe version (major)
			BYTE m_versionMin;			// 4 = exe version (minor)
			BYTE m_versionRev;			// 5 = exe version (revision)
			BYTE m_versionPat;			// 6 = exe version (patch)
			NWORD m_x;					// 7-8 = location x
			NWORD m_y;					// 9-10 = location y
			BYTE m_z;					// 11= location z
			BYTE m_map;					// 12 = location map
			char m_account[32];			// 13-44 = account name
			char m_charname[32];		// 45-76 = character name
			char m_ipAddress[15];		// 77-91
			NDWORD m_unk;				// 92-95
			NDWORD m_errorCode;			// 96-99 = error code
			char m_executable[100];		// 100-199 = executable name
			char m_description[100];	// 200-299 = error description
			BYTE m_zero;				// 300
			NDWORD m_offset;			// 301-304 = exception offset
			BYTE m_addressCount;		// 305 = address count
			NDWORD m_address[1];		// 306-.. = address
		} CrashReport;

		struct // XCMD_CreateHS, size = 106 // create a new char (uohs)
		{
			BYTE m_Cmd;						// 0 = 0xF8
			NDWORD m_pattern1;				// 1 - 4 = 0xedededed
			NDWORD m_pattern2;				// 5 - 8 = 0xffffffff
			BYTE m_kuoc;					// 9 = 0x0 (0xFF = Krrios)
			char m_charname[MAX_NAME_SIZE];	// 10 - 39 = ascii name
			BYTE m_unk1[2];					// 40 - 41 = 0x00
			NDWORD m_flags;					// 42 - 45 = flags
			BYTE m_unk2[8];					// 46 - 53 = unknown
			BYTE m_prof;					// 54 = profession
			BYTE m_unk3[15];				// 55 - 69 = unknown
			BYTE m_sex;						// 70 = race+sex
			BYTE m_str;						// 71 = str
			BYTE m_dex;						// 72 = dex
			BYTE m_int;						// 73 = int
			BYTE m_skill1;					// 74 = skill id 1
			BYTE m_val1;					// 75 = skill value 1
			BYTE m_skill2;					// 76 = skill id 2
			BYTE m_val2;					// 77 = skill value 2
			BYTE m_skill3;					// 78 = skill id 3
			BYTE m_val3;					// 79 = skill value 3
			BYTE m_skill4;					// 80 = skill id 4
			BYTE m_val4;					// 81 = skill value 4
			NWORD m_wSkinHue;				// 82 - 83 = skin hue
			NWORD m_hairid;					// 84 - 85 = hair id
			NWORD m_hairHue;				// 86 - 87 = hair hue
			NWORD m_beardid;				// 88 - 89 = beard id
			NWORD m_beardHue;				// 90 - 91 = beard hue
			BYTE m_server;					// 92 = server index
			BYTE m_startloc;				// 93 = start location
			NDWORD m_slot;					// 94 - 97 = character slot
			NDWORD m_clientip;				// 98 - 101 = client ip
			NWORD m_shirtHue;				// 102 - 103 = shirt hue
			NWORD m_pantsHue;				// 104 - 105 = pants hue
		} CreateHS;
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

		struct  // Damage (SE/AOS damage counter)
		{
			BYTE m_Cmd;
			NDWORD m_UID;
			NWORD m_dmg;
		} DamagePacket;  // from server

		struct // size = 36 + 30 = 66	// Get full status on char.
		{
			BYTE m_Cmd;			// 0 = 0x11
			NWORD  m_len;		// 1-2 = size of packet (2 bytes)
			NDWORD m_UID;		// 3-6 = (first byte is suspected to be an id byte - 0x00 = player)
			char m_charname[ MAX_NAME_SIZE ];	// 7-36= character name (30 bytes) ( 00 padded at end)
			NWORD m_health;		// 37-38 = current health
			NWORD m_maxhealth;	// 39-40 = max health
			BYTE m_perm;		// 41 = permission to change name ? 0=stats invalid, 1=stats valid., 0xff = i can change the name
			BYTE m_ValidStats;	// 42 = 0 not valid, 1 = till weight, 2 = till statcap, 3 till follower, 4 = till the end, 5 = include maxweight+race.
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
			NWORD m_ResFire;
			NWORD m_ResCold;
			NWORD m_ResPoison;
			NWORD m_ResEnergy;
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
			BYTE m_race;		// 69 = race (1=human, 2=elf, 3=gargoyle) (RACE_TYPE)
			NWORD m_statcap;
			BYTE m_curFollower;
			BYTE m_maxFollower;
			NWORD m_ResFire;
			NWORD m_ResCold;
			NWORD m_ResPoison;
			NWORD m_ResEnergy;
			NWORD m_Luck;
			NWORD m_minDamage;
			NWORD m_maxDamage;
			NDWORD m_Tithing;
			// new {6}
			NWORD m_ResPhysicalMax;
			NWORD m_ResFireMax;
			NWORD m_ResColdMax;
			NWORD m_ResPoisonMax;
			NWORD m_ResEnergyMax;
			NWORD m_IncreaseDefChance;
			NWORD m_IncreaceDefChanceMax;
			NWORD m_IncreaseHitChance;
			NWORD m_IncreaseSwingSpeed;
			NWORD m_IncreaseDam;
			NWORD m_LowerReagentCost;
			NWORD m_IncreaseSpellDam;
			NWORD m_FasterCastRecovery;
			NWORD m_FasterCasting;
			NWORD m_LowerManaCost;
		} StatusNew;
 
		struct // size = ? change colour of hp bar, SA
		{
			BYTE m_Cmd;		// 0 = 0x17 / XCMD_HealthBarColor
			NWORD m_len;	// 1-2 = len
			NDWORD m_UID;	// 3-6 = character serial
			NWORD m_count;	// 7-8 = number of states
			struct
			{
				NWORD m_color;	// 9-10 = status color (1=green, 2=yellow)
				BYTE m_state;	// 11 = color state
			} m_states[1];
		} HealthBarColor;

		struct // size = 19	or var len // draw the item at a location
		{
			BYTE m_Cmd;		// 0 = 0x1a = XCMD_Put
			NWORD m_len;	// 1-2 = var len = 0x0013 or 0x000e or 0x000f
			NDWORD m_UID;	// 3-6 = UID | UID_O_ITEM | UID_F_RESOURCE
			NWORD m_id;		// 7-8
			NWORD m_amount;	// 9-10 - only present if m_UID | UID_F_RESOURCE = pile (optional)
			NWORD m_x;		// 11-12 - | 0x8000 = m_dir arg.
			NWORD m_y;		// 13-14 = y | 0xC000 = m_wHue and m_flags fields.
			BYTE m_dir;		// (optional)
			BYTE m_z;		// 15 = signed char
			NWORD m_wHue;	// 16-17 = HUE_TYPE (optional)
			BYTE m_flags;	// 18 = 0x20 = is it movable ? (direction?) (optional)
		} Put;

		// Item flags
#define ITEMF_MOVABLE	0x20
#define ITEMF_INVIS		0x80

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
#define CHARMODE_FREEZE		0x01
#define CHARMODE_FEMALE		0x02
#define CHARMODE_POISON		0x04	// green status bar. (note: see XCMD_HealthBarColor for SA)
#define CHARMODE_FLYING		0x04	// flying (gargoyles, SA)
#define CHARMODE_YELLOW		0x08	// yellow status bar. (note: see XCMD_HealthBarColor for SA)
#define CHARMODE_IGNOREMOBS	0x10
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

		struct // sizeo = 5	// remove an object (item or char)
		{
			BYTE m_Cmd;			// 0 = 0x1D
			NDWORD m_UID;		// 1-4 = object UID.
		} Remove;

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

		struct // size = 8
		{
			BYTE m_Cmd;			// 0 = 0x21
			BYTE m_count;		// sequence #
			NWORD m_x;
			NWORD m_y;
			BYTE m_dir;			// DIR_TYPE
			BYTE m_z;
		} WalkCancel;

		struct	// size=3
		{
			BYTE m_Cmd;		// 0 = 0x22
			BYTE m_count; 	// 1 = goes up as we walk. (handshake)
			BYTE m_noto;	// 2 = notoriety flag
		} WalkAck;

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

		struct // size = 21	// Add Single Item To Container.
		{
			BYTE m_Cmd;		// 0 = 0x25
			NDWORD m_UID;	// 1-4
			NWORD m_id;
			BYTE m_zero7;
			NWORD m_amount;
			NWORD m_x;
			NWORD m_y;
			BYTE m_grid;
			NDWORD m_UIDCont;	// the container.
			NWORD m_wHue;		// HUE_TYPE
		} ContAddNew;

		struct // size = 5	// Kick the player off.
		{
			BYTE m_Cmd;		// 0 = 0x26
			NDWORD m_unk1;	// 1-4 = 0 = GM who issued kick ?
		} Kick;

		struct // size = 2	// kill a drag. (Bounce)
		{
			BYTE m_Cmd;		// 0 = 0x27
			BYTE m_type;	// 1 = bounce type ? = 5 = drag
		} DragCancel;

		struct // size = 2 Death Menu
		{
			BYTE m_Cmd;		// 0 = 0x2c
			BYTE m_shift;	// 1 = 0
		} DeathMenu;

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

		struct // size = 10	// There is a fight some place.
		{
			BYTE m_Cmd;		// 0 = 0x2f
			BYTE m_dir;		// 1 = 0 = DIR_TYPE
			NDWORD m_AttackerUID;
			NDWORD m_AttackedUID;
		} Fight;

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

		struct // size = variable	// close the vendor window.
		{
			BYTE m_Cmd;		// 0 =0x3b
			NWORD m_len;
			NDWORD m_UIDVendor;
			BYTE m_flag;	// 0x00 = no items following, 0x02 - items following
		} VendorBuy;

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

		struct // size = 5 + ( x * 20 ) // set up the content of some container.
		{
#define MAX_ITEMS_CONTENTNEW (MAX_BUFFER - 5) / 20
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
				BYTE m_grid;	// 13
				NDWORD m_UIDCont;	// 14-17 = What container is it in.
				NWORD m_wHue;	// 18-19 = HUE_TYPE
			} m_item[ MAX_ITEMS_CONTENTNEW ];
		} ContentNew;

		struct // size = 6	// personal light level.
		{
			BYTE m_Cmd;		// 0 = 0x4e
			NDWORD m_UID;	// 1 = creature uid
			BYTE m_level;	// 5 = 0 = shape of the light. (LIGHT_PATTERN?)
		} LightPoint;

		struct // size = 2
		{
			BYTE m_Cmd;			// 0 = 0x4f
			BYTE m_level;		// 1=0-19, 19=dark, On t2a 30=dark, LIGHT_BRIGHT
		} Light;

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

		struct // size = 3 // play music
		{
			BYTE m_Cmd;		// 0 = 0x6d
			NWORD m_musicid;// 1= music id number
		} PlayMusic;

		struct // size = 14 // Combat type animation.
		{
			BYTE m_Cmd;			// 0 = 0x6e
			NDWORD m_UID;		// 1-4=uid
			NWORD m_action;		// 5-6 = ANIM_TYPE
			BYTE m_zero7;		// 7 = 0 or 5 ?
			BYTE m_dir;			// DIR_TYPE
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

		struct // size = 2	// ping goes both ways.
		{
			BYTE m_Cmd;		// 0 = 0x73
			BYTE m_bCode;	// 1 = ?
		} Ping;

		struct // size = var // Open buy window
		{
			BYTE m_Cmd;		// 0 = 0x74
			NWORD m_len;
			NDWORD m_VendorUID;
			BYTE m_count;
			struct
			{
				NDWORD m_price;
				BYTE m_len;
				char m_text[1];
			} m_item[1];
		} VendOpenBuy;

		struct // size = 16	// Move to a new server. Not sure why the client cares about this.
		{
			BYTE m_Cmd;		// 0 = 0x76
			NWORD m_x;
			NWORD m_y;
			NWORD m_z;
			BYTE m_unk7_zero;	// is a toggle of some sort
			WORD m_serv_boundX;	// distances ?
			WORD m_serv_boundY;
			NWORD m_serv_boundW;	// WORD ??
			NWORD m_serv_boundH;	// describes the new server some how.
		} ZoneChange;

		// 00 14 00 00 00 04 00 09 00

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
			CEventCharDef m_char[1];
		} CharList3;

		struct // size = 2
		{
			BYTE m_Cmd;		// 0 = 0x82
			BYTE m_code;	// 1 = PacketLoginError::Reason
		} LogBad;

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
			CEventCharDef m_char[1];
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

		struct	// display a gump with some text on it.
		{
			BYTE m_Cmd; // 0x8b
			NWORD m_len; // 13 + len(m_unktext) + len(m_text)
			NDWORD m_UID; // Some uid which doesn't clash with any others that the client might have
			NWORD m_gump; // Signs and scrolls work well with this. GUMP_TYPE
			NWORD m_len_unktext; // Evidently m_unktext is either an array of bytes, or a non-nulled char array
			NWORD m_unk11; // who knows??
			char m_unktext[1]; // same
			char m_text[1]; // text to display
		} GumpTextDisp;

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
			BYTE m_Cmd;				// 0 = 0x99
			BYTE m_fAllowGround;	// 1 = 1=ground only, 0=dynamic object
			NDWORD m_context;		// 2-5 = we sent this at target setup.
			BYTE m_zero6[12];		// 6-17
			NWORD m_id;				// 18-19 = The multi id to preview. (id-0x4000)
			NWORD m_x;				// 20-21 = x offset
			NWORD m_y;				// 22-23 = y offset
			NWORD m_z;				// 24-25 = z offset
		} TargetMulti;

		struct // size = 16 // console prompt request.
		{
			BYTE m_Cmd;		// 0 = 0x9a
			NWORD m_len;	// 1-2 = length = 16
			NDWORD m_serial;	// 3-6 = serial
			NDWORD m_prompt;	// 7-10 = prompt id
			NDWORD m_type;	// 11-14 = type (0=request/esc, 1=reply)
			char m_text[1];	// 15+ = null terminated text? doesn't seem to work.
		} Prompt;

		struct // size = var	// vendor sell dialog
		{
			BYTE m_Cmd;		// 0 = 0x9e
			NWORD m_len;	// 1-2
			NDWORD m_UIDVendor;	// 3-6
			NWORD m_count;	// 7-8
			struct
			{
				NDWORD m_UID;
				NWORD m_id;
				NWORD m_wHue;	// HUE_TYPE
				NWORD m_amount;
				NWORD m_price;	// Hmm what if price is > 65K?
				NWORD m_len;
				char m_text[1];
			} m_item[1];
		} VendOpenSell;

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

		struct // size = 10 + var	// Read scroll
		{
			BYTE m_Cmd;			// 0=0xA6
			NWORD m_len;		// 1-2
			BYTE  m_type;		// 3 = form or gump = 0=tips,1=notice or 2 (SCROLL_TYPE)
			NDWORD m_context;	// 4-7 = context info passed from server
			NWORD m_lentext;	// 8
			char m_text[1];		// 10
		} Scroll;

		struct // size = 6+servers*40
		{
			BYTE m_Cmd;				// 0 = 0xA8
			NWORD m_len;			// 1-2
			BYTE m_nextLoginKey;	// 3=next login key the client should use in ServersReq (0x80) packet
			NWORD m_count;			// 4-5=num servers.

#define MAX_SERVERS 32
#define MAX_SERVER_NAME_SIZE 32
			struct	// size=40
			{
				NWORD m_count;							// 0=0 based enum
				char m_servname[32];	// 2
				BYTE m_percentfull;						// 34 = 25 or 2e
				signed char m_timezone;					// 35 = 0x05=east coast or 0x08=west coast
				BYTE m_ip[4];							// 36-39 = ip to ping
			} m_serv[ MAX_SERVERS ];
		} ServerList;

		struct // size = 4+(5*2*MAX_NAME_SIZE)+1+(starts*63) // list available chars for your account.
		{
			BYTE m_Cmd;		// 0 = 0xa9
			NWORD m_len;	// 1-2 = var len
			BYTE m_count;	// 3=5 needs to be 5 for some reason.
			CEventCharDef m_char[1];

			BYTE m_startcount;
			struct	// size = 63
			{
				BYTE m_id;
				char m_area[MAX_NAME_SIZE+1];
				char m_name[MAX_NAME_SIZE+1];
			} m_start[1];
			NDWORD	m_flags;
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

		struct // size = 13	// Death anim of a creature
		{
			BYTE m_Cmd;		// 0 = 0xaf
			NDWORD m_UID;	// 1-4
			NDWORD m_UIDCorpse; // 5-8
			NWORD m_DeathFlight;		// 9= in flight ?
			NWORD m_Death2Anim;	// 11= forward/backward,dir ?
		} CharDeath;

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

		struct // size = variable	// Put up a tooltip
		{
			BYTE m_Cmd;				// 0 = 0xb7
			NWORD m_len;			// 1-2
			NDWORD m_UID;			// 3-6
			NCHAR m_utext[1];		// zero terminated UNICODE string
		} ToolTip;

		struct	// size = 8 // Show Character Profile.
		{
			BYTE m_Cmd;		// 0=0xB8
			NWORD m_len;	// 1 - 2
			NDWORD m_UID;	// 3-6=uid
			char m_title[1];	// Description 1 (not unicode!, name, zero terminated)
			NCHAR m_utext1[1];	// Static Description 2 (unicode!, zero terminated)
			NCHAR m_utext2[1];	// Player Description 3 (unicode!, zero terminated)
		} CharProfile;

		struct // size = 3	// enable features on client
		{
			BYTE m_Cmd;		// 0 = 0xb9
			NWORD m_enable;
		} FeaturesEnable;

		struct // size = 5	// enable features on client (new)
		{
			BYTE m_Cmd;		// 0 = 0xb9
			NDWORD m_enable;
		} FeaturesEnableNew;

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
			char m_charname[MAX_NAME_SIZE];	// 18-47
			TCHAR m_args[1];		// 48+ = arguments
		} SpeakLocalized;

		struct // size = 21 // console prompt request. (unicode version)
		{
			BYTE m_Cmd;			// 0 = 0xc2
			NWORD m_len;		// 1-2 = length = 16
			NDWORD m_serial;	// 3-6 = serial
			NDWORD m_prompt;	// 7-10 = prompt id
			NDWORD m_type;		// 11-14 = type (0=request/esc, 1=reply)
			char m_lang[4];		// 15-18 = lang (3 chars + NULL)
			WCHAR m_utext[1];	// 19+ = response
		} PromptUNICODE;
		
		struct
		{
			BYTE m_Cmd; // 0=0xc4
			NDWORD m_UID;
			BYTE m_Intensity;
		} Semivisible;
		
		struct	// size = 2	// Sent for altering visual range
		{
			BYTE m_Cmd;		// 0 =0xC8
			BYTE m_Value;	// 0x01 to UO_MAP_VIEW_SIZE
		} VisualRange;

		struct
		{
			BYTE m_Cmd;			// 0 = 0xCC
			NWORD m_len;		// 1-2 = var len size.
			NDWORD m_UID;		// 3-6 = UID num of speaker. 01010101 = system
			NWORD m_id;			// 7-8 = CREID_TYPE of speaker.
			BYTE m_mode;		// 9 = TALKMODE_TYPE
			NWORD m_wHue;		// 10-11 = HUE_TYPE.
			NWORD m_font;		// 12-13 = FONT_TYPE
			NDWORD m_clilocId;	// 14-17 = Cliloc ID to display
			BYTE m_affixType;	// 18 = Affix type (0=append, 1=prepend, 2=system)
			char m_charname[MAX_NAME_SIZE];	// 19-48
			char m_affix[1];	// 49+ = affix (ASCII only)
			TCHAR m_args[1];	// 50+len(affix)+ = arguments (UNICODE only)
		} SpeakLocalizedEx;

		struct	// size = 2 // Logout Confirm Answer
		{
			BYTE m_Cmd;		// 0=0xD1
			BYTE m_unk;	// always 1
		} LogoutStatus;

		struct // size = 17+var // Get a change to the book title.
		{
			BYTE m_Cmd;			// 0 = 0xD4
			NWORD m_len;		// 1-2 = length
			NDWORD m_UID;		// 3-6 = book
			BYTE m_unk1;		// 7 = 0x01
			BYTE m_writable;	// 8 = 0 = non writable, 1 = writable
			NWORD m_pages;		// 9-10 = number of pages
			NWORD m_titlelen;	// 11-12 = title length
			char m_title[1];	// 13 - = title
			NWORD m_authorlen;	// = author length
			char m_author[1];	// = author
		} BookOpenNew;
		
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
			BYTE m_compression; // 0x00 not compressed
			BYTE m_unk1;	// 0x00
			NDWORD m_UID;
			NDWORD m_revision;
			NWORD m_itemcount;
			NWORD m_datasize; // itemcount * 5
			BYTE m_planeCount;
			struct
			{
				BYTE m_index;	// index | 0x20
				BYTE m_size;
				BYTE m_length;
				BYTE m_flags;
				NWORD m_data[1];
			} m_planeList[1];
			struct
			{
				BYTE m_index;	// index + 9
				BYTE m_size;
				BYTE m_length;
				BYTE m_flags;
				struct
				{
					NWORD m_id;
					BYTE m_x;
					BYTE m_y;
					BYTE m_z;
				} m_data[1];
			} m_stairsList[1];
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

		struct
		{
			BYTE m_Cmd;
			NWORD m_Length;
			NDWORD m_Uid;
			NWORD m_IconId_1;
			NWORD m_Show_1;
			NDWORD m_unk_1;
			NWORD m_IconId_2;
			NWORD m_Show_2;
			NDWORD m_unk_2;
			NWORD m_Time;
			NWORD m_unk_3;
			BYTE m_unk_4;
			NDWORD m_ClilocOne;
			NDWORD m_ClilocTwo;
			NDWORD m_unk_5;
			NWORD m_unk_6;
			NWORD m_unk_7;
			BYTE m_MBUTab_1[2];
			BYTE m_MBUText_1[6];
			BYTE m_MBUTab_2[2];
			BYTE m_MBUText_2[6];
			BYTE m_MBUTab_3[2];
			BYTE m_MBUText_3[6];
			NWORD m_unk_8;
		} AddBuff;

		struct
		{
			BYTE m_Cmd;
			NWORD m_Length;
			NDWORD m_Uid;
			NWORD m_IconId;
			NWORD m_Show;
			NDWORD m_unk_1;
		} RemoveBuff;

		struct // XCMD_NewAnimUpdate
		{
			BYTE m_Cmd;			// 0=0xe2
			NDWORD m_uid;		// 1 - 4 (len)
			NWORD m_action;		// 5 - 6
			BYTE m_zero;		// 7	 (0x00)
			NWORD m_count;		// 8 - 10 (layers to unequip)
		} NewAnimUpdate;

		struct // XCMD_WaypointShow
		{
			BYTE m_Cmd;				// 0 = 0xE5
			NWORD m_len;			// 1 - 2 = length
			NDWORD m_Wpnt_uid;		// 3 - 7 = serial uid
			NWORD m_Wpnt_px;		// 8 - 9 = X
			NWORD m_Wpnt_py;		// 10 - 11 = Y
			BYTE m_Wpnt_pz;			// 12 = Y
			BYTE m_Wpnt_pmap;		// 13 = MAP
			NWORD m_Wpnt_type;		// 14 - 15 = object type (??)
			NWORD m_ignoreUid;		// 16 - 17 = don't use the uid (0/1)
			NDWORD m_ClilocId;		// 18 - 22 = clilocId
			NCHAR m_ClilocArgs[1];	// 23 - ? = clilocArgs
			NWORD m_zero;			// ? - ?+1 = zero
		} WaypointShow;

		struct // XCMD_WaypointHide
		{
			BYTE m_Cmd;				// 0 = 0xE6
			NDWORD m_Wpnt_uid;		// 1 - 4 = serial uid
		} WaypointHide;

		struct // XCMD_HighlightUIContinue
		{
			BYTE m_Cmd;				// 0 = 0xE7
			NDWORD m_Uid;			// 1 - 4 = serial uid
			NWORD m_IdUi;			// 5 - 6 = id ui
			NDWORD m_destUid;		// 7 - 11 = destination serial uid
			BYTE m_One;				// 12 = 0x01
		} HighlightUIContinue;

		struct // XCMD_HighlightUIToggle
		{
			BYTE m_Cmd;				// 0 = 0xE9
			NDWORD m_Uid;			// 1 - 4 = serial uid
			NWORD m_IdUi;			// 5 - 6 = id ui
			char m_Desc[64];		// 7 - 71 = dexcription: �ToggleInventory�, �TogglePaperdoll�, �ToggleMap�, ��
			NDWORD m_commandId;		// 72 - 76 = command Id
		} HighlightUIToggle;

		struct // XCMD_ToggleHotbar
		{
			BYTE m_Cmd;				// 0 = 0xEA
			NWORD m_Enable;			// 1 - 2 = enable
		} ToggleHotbar;

		struct // XCMD_TSyncReply
		{
			BYTE m_Cmd;				// 0 = 0xF2
			NDWORD m_unk1;			// 1 - 4
			NDWORD m_unk2;			// 5 - 8
			NDWORD m_unk3;			// 9 - 12
			NDWORD m_unk4;			// 13 - 16
			NDWORD m_unk5;			// 17 - 20
			NDWORD m_unk6;			// 21 - 24
		} TSyncReply;

		struct	// XCMD_MapDisplayNew, size = 21
		{
			BYTE m_Cmd; // 0 = 0xF5
			NDWORD m_UID; // uid of the map item
			NWORD m_Gump_Corner; // GUMP_TYPE always 0x139d....compass tile id in the corner.,
			NWORD m_x_ul; // upper left x coord.
			NWORD m_y_ul; // upper left y coord.
			NWORD m_x_lr; // lower right x coord.
			NWORD m_y_lr; // lower right y coord.
			NWORD m_xsize; // client width
			NWORD m_ysize; // client height
			NWORD m_map; // map id
		} MapDisplayNew;	// MapX

		struct	// XCMD_MoveShip, size = 18+var
		{
			BYTE m_Cmd; // 0 = 0xF6
			NWORD m_len;
			NDWORD m_UID; // uid of the ship
			BYTE m_unk1; // 0x04
			BYTE m_unk2; // 0x04
			BYTE m_unk3; // 0x04
			NWORD m_shipX; // ship x
			NWORD m_shipY; // ship y
			NWORD m_shipZ; // ship z
			NWORD m_objectCount; // number of objects on deck
			struct
			{
				NDWORD m_UID; // object uid
				NWORD m_x; // object x
				NWORD m_y; // object y
				NWORD m_z; // object z
			} m_objects[1];
		} MoveShip;
	};
} PACK_NEEDED;

// Turn off structure packing.
#if defined _WIN32 && (!__MINGW32__)
#pragma pack()
#endif

#endif // _INC_GRAYPROTO_H
