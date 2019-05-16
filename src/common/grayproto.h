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
	XCMD_HealthBarColorNew	= 0x16,
	XCMD_HealthBarColor	= 0x17,
	XCMD_WorldItem		= 0x1a,
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
	XCMD_WorldObj			= 0xf3,
	XCMD_CrashReport		= 0xf4,
	XCMD_MapDisplayNew		= 0xf5,
	XCMD_MoveShip			= 0xf6,
	XCMD_WorldObjCont		= 0xf7,
	XCMD_CreateHS			= 0xf8,
	XCMD_GlobalChat			= 0xf9,
	XCMD_UltimaStoreButton	= 0xfa
};

#define SEEDLENGTH_OLD (sizeof( DWORD ))
#define SEEDLENGTH_NEW (1 + sizeof( DWORD )*5)

enum PARTYMSG_TYPE
{
	PARTYMSG_Add				= 0x1,	// client <-> server
	PARTYMSG_Remove				= 0x2,	// client <-> server
	PARTYMSG_MsgMember			= 0x3,	// client  -> server
	PARTYMSG_MsgAll				= 0x4,	// client <-> server
	PARTYMSG_Disband			= 0x5,	// client  -> server
	PARTYMSG_ToggleLooting		= 0x6,	// client  -> server
	PARTYMSG_Invite				= 0x7,	// client <-  server
	PARTYMSG_Accept				= 0x8,	// client  -> server
	PARTYMSG_Decline			= 0x9	// client  -> server
};

enum EXTDATA_TYPE
{
	EXTDATA_Fastwalk_Init		= 0x01,	// client <-  server
	EXTDATA_Fastwalk_Add		= 0x02,	// client <-  server
	//
	EXTDATA_CloseGump			= 0x04,	// client <-  server
	EXTDATA_ScreenSize			= 0x05,	// client  -> server
	EXTDATA_Party_Msg			= 0x06,	// client <-> server
	EXTDATA_QuestArrow_Click	= 0x07,	// client  -> server
	EXTDATA_Map_Change			= 0x08,	// client <-  server
	EXTDATA_Wrestle_Disarm		= 0x09,	// client  -> server
	EXTDATA_Wrestle_Stun		= 0x0A,	// client  -> server
	EXTDATA_Language			= 0x0B,	// client  -> server
	EXTDATA_StatusClose			= 0x0C,	// client  -> server
	//
	EXTDATA_AnimationReq		= 0x0E,	// client  -> server
	EXTDATA_ClientInfo			= 0x0F,	// client  -> server
	EXTDATA_OldAOSTooltipInfo	= 0x10,	// client <-> server
	//
	//
	EXTDATA_Popup_Request		= 0x13,	// client  -> server
	EXTDATA_Popup_Display		= 0x14,	// client <-  server
	EXTDATA_Popup_Select		= 0x15,	// client  -> server
	EXTDATA_CloseUIWindow		= 0x16,	// client <-  server
	EXTDATA_CodexOfWisdom		= 0x17,	// client <-  server
	EXTDATA_EnableMapDiffs		= 0x18,	// client <-  server
	EXTDATA_ExtendedStats		= 0x19,	// client <-  server
	EXTDATA_StatLock			= 0x1A,	// client <-  server
	EXTDATA_SpellbookContent	= 0x1B,	// client <-  server
	EXTDATA_CastSpell			= 0x1C,	// client  -> server
	EXTDATA_HouseDesignVer		= 0x1D,	// client <-  server
	EXTDATA_HouseDesignReq		= 0x1E,	// client  -> server
	//
	EXTDATA_HouseCustomize		= 0x20,	// client <-  server
	//
	EXTDATA_CombatDamage		= 0x22,	// client <-  server
	//
	EXTDATA_AntiCheat			= 0x24,	// client  -> server
	//
	EXTDATA_SpeedMode			= 0x26,	// client <-  server
	//
	//
	//
	//
	//
	EXTDATA_BandageMacro		= 0x2C,	// client  -> server
	EXTDATA_TargetedSpell		= 0x2D,	// client  -> server
	EXTDATA_TargetedSkill		= 0x2E,	// client  -> server
	//
	EXTDATA_TargetedResource	= 0x30,	// client  -> server
	//
	EXTDATA_GargoyleFly			= 0x32,	// client  -> server
	EXTDATA_WheelBoatMove		= 0x33	// client  -> server
};

enum EXTAOS_TYPE
{
	//
	EXTAOS_HouseDesign_Backup	= 0x02,	// client  -> server
    EXTAOS_HouseDesign_Restore	= 0x03,	// client  -> server
	EXTAOS_HouseDesign_Commit	= 0x04,	// client  -> server
	EXTAOS_HouseDesign_RemItem	= 0x05,	// client  -> server
	EXTAOS_HouseDesign_AddItem	= 0x06,	// client  -> server
	//
	//
	//
	//
	//
	EXTAOS_HouseDesign_Exit		= 0x0C,	// client  -> server
	EXTAOS_HouseDesign_AddStair	= 0x0D,	// client  -> server
	EXTAOS_HouseDesign_Sync		= 0x0E,	// client  -> server
	//
	EXTAOS_HouseDesign_Clear	= 0x10,	// client  -> server
	//
	EXTAOS_HouseDesign_Switch	= 0x12,	// client  -> server
	EXTAOS_HouseDesign_AddRoof	= 0x13,	// client  -> server
	EXTAOS_HouseDesign_RemRoof	= 0x14,	// client  -> server
	//
	//
	//
	//
	EXTAOS_CombatAbility		= 0x19,	// client  -> server
	EXTAOS_HouseDesign_Revert	= 0x1A,	// client  -> server
	//
	//
	//
	EXTAOS_EquipLastWeapon		= 0x1E,	// client  -> server
	//
	//
	//
	//
	//
	//
	//
	//
	//
	EXTAOS_GuildButton			= 0x28,	// client  -> server
	//
	//
	//
	//
	//
	//
	//
	//
	//
	EXTAOS_QuestButton			= 0x32	// client  -> server
};

#define MAX_TALK_BUFFER		256	// how many chars can anyone speak all at once? (client speech is limited to 128 chars and journal is limited to 256 chars)

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
#define MINCLIVER_T2A					1253502	// minimum client to activate T2A packets (1.25.35b)
//#define MINCLIVER_R					2000000	// minimum client to activate R packets (2.0.0a)
//#define MINCLIVER_TD					3000000	// minimum client to activate TD packets (3.0.0a)
#define MINCLIVER_LBR					3000702	// minimum client to activate LBR packets (3.0.7b)
#define MINCLIVER_AOS					4000000	// minimum client to activate AOS packets (4.0.0a)
#define MINCLIVER_SE					4000500	// minimum client to activate SE packets (4.0.5a)
#define MINCLIVER_ML					5000000	// minimum client to activate ML packets (5.0.0a)
#define MINCLIVER_SA					7000000	// minimum client to activate SA packets (7.0.0.0)
#define MINCLIVER_HS					7000900	// minimum client to activate HS packets (7.0.9.0)
#define MINCLIVER_TOL					7004565	// minimum client to activate TOL packets (7.0.45.65)

// Client versions (extended status gump info)
#define MINCLIVER_STATUS_V2				3000804	// minimum client to receive v2 of 0x11 packet (3.0.8d)
#define MINCLIVER_STATUS_V3				3000810	// minimum client to receive v3 of 0x11 packet (3.0.8j)
#define MINCLIVER_STATUS_V4				4000000	// minimum client to receive v4 of 0x11 packet (4.0.0a)
#define MINCLIVER_STATUS_V5				5000000	// minimum client to receive v5 of 0x11 packet (5.0.0a)
#define MINCLIVER_STATUS_V6				7003000	// minimum client to receive v6 of 0x11 packet (7.0.30.0)

// Client versions (behaviours)
//#define MINCLIVER_CHECKWALKCODE		1260000	// minimum client to use walk crypt codes for fastwalk prevention (1.26.0a)
//#define MINCLIVER_CLILOCS				1260202	// minimum client to use clilocs (1.26.2b)
#define MINCLIVER_PADCHARLIST			3000010	// minimum client to pad character list to at least 5 characters
#define MINCLIVER_CLOSEDIALOG			4000400	// minimum client where close dialog does not trigger a client response
#define MINCLIVER_NEWVERSIONING			5000605	// minimum client to use new versioning format (after 5.0.6e it change to 5.0.6.5)
#define MINCLIVER_CONTAINERGRID			6000107	// minimum client to use container grid index (6.0.1.7)
#define MINCLIVER_NEWCHATSYSTEM			7000400	// minimum client to use new chat system (7.0.4.0)
#define MINCLIVER_GLOBALCHAT			7006202	// minimum client to use global chat system (7.0.62.2)
#define MINCLIVER_PARRYCHANCETOOLTIP	7007472	// minimum client to use 'last parry chance' tooltip (7.0.74.72)

// Client versions (packets)
#define MINCLIVER_STATLOCKS				4000100	// minimum client to receive 0xBF.0x19.0x02 packet
#define MINCLIVER_NEWDAMAGE				4000700	// minimum client to receive 0x0B packet (4.0.7a)
#define MINCLIVER_BUFFS					5000202	// minimum client to receive buff packets (5.0.2b)
#define MINCLIVER_NEWCONTEXTMENU		6000000	// minimun client to receive 0xBF.0x14.0x02 packet instead of 0xBF.0x14.0x01 (6.0.0.0)
#define MINCLIVER_EXTRAFEATURES			6001402	// minimum client to receive 4-byte feature mask (6.0.14.2)
#define MINCLIVER_NEWMAPDISPLAY			7000800	// minimum client to receive 0xF5 packet (7.0.8.0)
#define MINCLIVER_EXTRASTARTINFO		7001300	// minimum client to receive extra start info (7.0.13.0)
#define MINCLIVER_NEWMOBINCOMING		7003301	// minimun client to receive 0x78 packet (7.0.33.1)

// Enhanced clients report version to server using masks, so the formula 'reported version - mask' must be used to get the real value.
// Eg: SA enhanced client 4.0.52.0 will report 67.0.52.0 to server, so the real value is 67.0.52.0 - mask = 4.0.52.0
#define MASK_CLIENTTYPE_EC				63000000	// SA+ enhanced clients starts on version 4.0.0.0

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
	#define MAX_BOOK_PAGES		64		// arbitrary max number of pages.
	#define MAX_BOOK_LINES		8		// max lines per page.

	// Some messages are bi-directional
	union
	{
		BYTE m_Raw[MAX_BUFFER];

		struct
		{
			BYTE m_Cmd;			// XCMD_TYPE, 0 = ?
			BYTE m_Arg[1];		// unknown size.
		} Default;

		struct // size = 62		// first login to req listing the servers.
		{
			BYTE m_Cmd;	// 0 = 0x80 = XCMD_ServersReq
			char m_acctname[MAX_ACCOUNT_NAME_SIZE];
			char m_acctpass[MAX_NAME_SIZE];
			BYTE m_loginKey;	// 61 = NextLoginKey from uo.cfg
		} ServersReq;

		struct	// size = 65	// request to list the chars I can play.
		{
			BYTE m_Cmd;			// 0 = 0x91
			NDWORD m_Account;	// 1-4 = account id from XCMD_Relay message to log server.
			char m_acctname[MAX_ACCOUNT_NAME_SIZE];	// This is corrupted or encrypted seperatly ?
			char m_acctpass[MAX_NAME_SIZE];
		} CharListReq;

		struct // XCMD_EncryptionReply
		{
			BYTE m_Cmd;			// 0 = 0xE4
			NWORD m_len;		// 1 - 2 = length
			NDWORD m_lenUnk1;	// 3 - 6 = length of m_unk1
			BYTE m_unk1[1];		// 7 - ? = ?
		} EncryptionReply;
	};
} PACK_NEEDED;

// Item flags
#define ITEMF_MOVABLE			0x20
#define ITEMF_INVIS				0x80

// Char mode flags
#define CHARMODE_FREEZE			0x01
#define CHARMODE_FEMALE			0x02
#define CHARMODE_POISON			0x04	// green status bar. (note: see XCMD_HealthBarColor for SA)
#define CHARMODE_FLYING			0x04	// flying (gargoyles, SA)
#define CHARMODE_YELLOW			0x08	// yellow status bar. (note: see XCMD_HealthBarColor for SA)
#define CHARMODE_IGNOREMOBS		0x10
#define CHARMODE_WAR			0x40
#define CHARMODE_INVIS			0x80

#define MAX_SERVERS				32
#define MAX_SERVER_NAME_SIZE	32

// Turn off structure packing.
#if defined _WIN32 && (!__MINGW32__)
#pragma pack()
#endif

#endif // _INC_GRAYPROTO_H
