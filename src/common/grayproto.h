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

#define SEEDLENGTH_OLD		sizeof(DWORD)
#define SEEDLENGTH_NEW		(1 + sizeof(DWORD) * 5)
#define MAX_TALK_BUFFER		256		// client speech is limited to 128 chars and journal is limited to 256 chars

enum PACKET_TYPE
{
	PACKET_Create				= 0x00,
	PACKET_WalkRequest			= 0x02,
	PACKET_Talk					= 0x03,
	PACKET_Attack				= 0x05,
	PACKET_DClick				= 0x06,
	PACKET_ItemPickupReq		= 0x07,
	PACKET_ItemDropReq			= 0x08,
	PACKET_Click				= 0x09,
	PACKET_DamagePacket			= 0x0B,
	PACKET_Status				= 0x11,
	PACKET_ExtCmd				= 0x12,
	PACKET_ItemEquipReq			= 0x13,
	PACKET_HealthBarColorNew	= 0x16,
	PACKET_HealthBarColor		= 0x17,
	PACKET_WorldItem			= 0x1A,
	PACKET_Start				= 0x1B,
	PACKET_Speak				= 0x1C,
	PACKET_Remove				= 0x1D,
	PACKET_PlayerUpdate			= 0x20,
	PACKET_WalkReject			= 0x21,
	PACKET_WalkAck				= 0x22,
	PACKET_DragAnim				= 0x23,
	PACKET_ContOpen				= 0x24,
	PACKET_ContAdd				= 0x25,
	PACKET_Kick					= 0x26,
	PACKET_DragCancel			= 0x27,
	PACKET_DropRejected			= 0x28,
	PACKET_DropAccepted			= 0x29,
	PACKET_DeathMenu			= 0x2C,
	PACKET_ItemEquip			= 0x2E,
	PACKET_Fight				= 0x2F,
	PACKET_CharStatReq			= 0x34,
	PACKET_Skill				= 0x3A,
	PACKET_VendorBuy			= 0x3B,
	PACKET_Content				= 0x3C,
	PACKET_StaticUpdate			= 0x3F,
	PACKET_LightPoint			= 0x4E,
	PACKET_Light				= 0x4F,
	PACKET_IdleWarning			= 0x53,
	PACKET_Sound				= 0x54,
	PACKET_LoginComplete		= 0x55,
	PACKET_MapEdit				= 0x56,
	PACKET_Time					= 0x5B,
	PACKET_CharPlay				= 0x5D,
	PACKET_Weather				= 0x65,
	PACKET_BookPage				= 0x66,
	PACKET_Options				= 0x69,
	PACKET_Target				= 0x6C,
	PACKET_PlayMusic			= 0x6D,
	PACKET_CharAction			= 0x6E,
	PACKET_SecureTrade			= 0x6F,
	PACKET_Effect				= 0x70,
	PACKET_BBoard				= 0x71,
	PACKET_War					= 0x72,
	PACKET_Ping					= 0x73,
	PACKET_VendOpenBuy			= 0x74,
	PACKET_CharName				= 0x75,
	PACKET_ZoneChange			= 0x76,
	PACKET_CharMove				= 0x77,
	PACKET_Char					= 0x78,
	PACKET_MenuItems			= 0x7C,
	PACKET_MenuChoice			= 0x7D,
	PACKET_UOGRequest			= 0x7F,
	PACKET_ServersReq			= 0x80,
	PACKET_CharList3			= 0x81,
	PACKET_LogBad				= 0x82,
	PACKET_CharDelete			= 0x83,
	PACKET_DeleteBad			= 0x85,
	PACKET_CharList2			= 0x86,
	PACKET_PaperDoll			= 0x88,
	PACKET_CorpEquip			= 0x89,
	PACKET_GumpTextDisp			= 0x8B,
	PACKET_Relay				= 0x8C,
	PACKET_CreateNew			= 0x8D,
	PACKET_MapDisplay			= 0x90,
	PACKET_CharListReq			= 0x91,
	PACKET_BookOpen				= 0x93,
	PACKET_DyeVat				= 0x95,
	PACKET_AllNames3D			= 0x98,
	PACKET_TargetMulti			= 0x99,
	PACKET_Prompt				= 0x9A,
	PACKET_HelpPage				= 0x9B,
	PACKET_VendOpenSell			= 0x9E,
	PACKET_VendorSell			= 0x9F,
	PACKET_ServerSelect			= 0xA0,
	PACKET_StatChngStr			= 0xA1,
	PACKET_StatChngInt			= 0xA2,
	PACKET_StatChngDex			= 0xA3,
	PACKET_Spy					= 0xA4,
	PACKET_Web					= 0xA5,
	PACKET_Scroll				= 0xA6,
	PACKET_TipReq				= 0xA7,
	PACKET_ServerList			= 0xA8,
	PACKET_CharList				= 0xA9,
	PACKET_AttackOK				= 0xAA,
	PACKET_GumpInpVal			= 0xAB,
	PACKET_GumpInpValRet		= 0xAC,
	PACKET_TalkUNICODE			= 0xAD,
	PACKET_SpeakUNICODE			= 0xAE,
	PACKET_CharDeath			= 0xAF,
	PACKET_GumpDialog			= 0xB0,
	PACKET_GumpDialogRet		= 0xB1,
	PACKET_ChatReq				= 0xB2,
	PACKET_ChatText				= 0xB3,
	PACKET_Chat					= 0xB5,
	PACKET_ToolTipReq			= 0xB6,
	PACKET_ToolTip				= 0xB7,
	PACKET_CharProfile			= 0xB8,
	PACKET_Features				= 0xB9,
	PACKET_Arrow				= 0xBA,
	PACKET_MailMsg				= 0xBB,
	PACKET_Season				= 0xBC,
	PACKET_ClientVersion		= 0xBD,
	PACKET_ExtendedData			= 0xBF,
	PACKET_EffectEx				= 0xC0,
	PACKET_SpeakLocalized		= 0xC1,
	PACKET_PromptUNICODE		= 0xC2,
	PACKET_Semivisible			= 0xC4,
	PACKET_EffectParticle		= 0xC7,
	PACKET_ViewRange			= 0xC8,
	PACKET_GQCount				= 0xCB,
	PACKET_SpeakLocalizedEx		= 0xCC,
	PACKET_ConfigFile			= 0xD0,
	PACKET_LogoutStatus			= 0xD1,
	PACKET_CharMove2			= 0xD2,
	PACKET_DrawChar2			= 0xD3,
	PACKET_AOSBookPage			= 0xD4,
	PACKET_AOSTooltip			= 0xD6,
	PACKET_EncodedData			= 0xD7,
	PACKET_AOSCustomHouse		= 0xD8,
	PACKET_Spy2					= 0xD9,
	PACKET_Majong				= 0xDA,
	PACKET_CharTransferLog		= 0xDB,
	PACKET_AOSTooltipInfo		= 0xDC,
	PACKET_CompressedGumpDialog	= 0xDD,
	PACKET_BuffPacket			= 0xDF,
	PACKET_BugReport			= 0xE0,
	PACKET_KRClientType			= 0xE1,
	PACKET_NewAnimUpdate		= 0xE2,
	PACKET_EncryptionReq		= 0xE3,
	PACKET_EncryptionReply		= 0xE4,
	PACKET_WaypointShow			= 0xE5,
	PACKET_WaypointHide			= 0xE6,
	PACKET_HighlightUIContinue	= 0xE7,
	PACKET_HighlightUIRemove	= 0xE8,
	PACKET_HighlightUIToggle	= 0xE9,
	PACKET_ToggleHotbar			= 0xEA,
	PACKET_UseHotbar			= 0xEB,
	PACKET_MacroEquipItem		= 0xEC,
	PACKET_MacroUnEquipItem		= 0xED,
	PACKET_NewSeed				= 0xEF,
	PACKET_WalkRequestNew		= 0xF0,
	PACKET_TimeSyncRequest		= 0xF1,
	PACKET_TimeSyncResponse		= 0xF2,
	PACKET_WorldObj				= 0xF3,
	PACKET_CrashReport			= 0xF4,
	PACKET_MapDisplayNew		= 0xF5,
	PACKET_MoveShip				= 0xF6,
	PACKET_WorldObjCont			= 0xF7,
	PACKET_CreateHS				= 0xF8,
	PACKET_GlobalChat			= 0xF9,
	PACKET_UltimaStoreButton	= 0xFA,
	PACKET_QTY
};

enum PACKETEXT_TYPE		// extended packets used by PACKET_ExtendedData
{
	PACKETEXT_Fastwalk_Init		= 0x01,	// client <-  server
	PACKETEXT_Fastwalk_Add		= 0x02,	// client <-  server
	PACKETEXT_CloseGump			= 0x04,	// client <-  server
	PACKETEXT_ScreenSize		= 0x05,	// client  -> server
	PACKETEXT_Party_Msg			= 0x06,	// client <-> server
	PACKETEXT_QuestArrow_Click	= 0x07,	// client  -> server
	PACKETEXT_Map_Change		= 0x08,	// client <-  server
	PACKETEXT_Wrestle_Disarm	= 0x09,	// client  -> server
	PACKETEXT_Wrestle_Stun		= 0x0A,	// client  -> server
	PACKETEXT_Language			= 0x0B,	// client  -> server
	PACKETEXT_StatusClose		= 0x0C,	// client  -> server
	PACKETEXT_AnimationReq		= 0x0E,	// client  -> server
	PACKETEXT_ClientInfo		= 0x0F,	// client  -> server
	PACKETEXT_OldAOSTooltipInfo	= 0x10,	// client <-> server
	PACKETEXT_Popup_Request		= 0x13,	// client  -> server
	PACKETEXT_Popup_Display		= 0x14,	// client <-  server
	PACKETEXT_Popup_Select		= 0x15,	// client  -> server
	PACKETEXT_CloseUIWindow		= 0x16,	// client <-  server
	PACKETEXT_CodexOfWisdom		= 0x17,	// client <-  server
	PACKETEXT_EnableMapDiffs	= 0x18,	// client <-  server
	PACKETEXT_ExtendedStats		= 0x19,	// client <-  server
	PACKETEXT_StatLock			= 0x1A,	// client <-  server
	PACKETEXT_SpellbookContent	= 0x1B,	// client <-  server
	PACKETEXT_CastSpell			= 0x1C,	// client  -> server
	PACKETEXT_HouseDesignVer	= 0x1D,	// client <-  server
	PACKETEXT_HouseDesignReq	= 0x1E,	// client  -> server
	PACKETEXT_HouseCustomize	= 0x20,	// client <-  server
	PACKETEXT_CombatDamage		= 0x22,	// client <-  server
	PACKETEXT_AntiCheat			= 0x24,	// client  -> server
	PACKETEXT_SpeedMode			= 0x26,	// client <-  server
	PACKETEXT_BandageMacro		= 0x2C,	// client  -> server
	PACKETEXT_TargetedSpell		= 0x2D,	// client  -> server
	PACKETEXT_TargetedSkill		= 0x2E,	// client  -> server
	PACKETEXT_TargetedResource	= 0x30,	// client  -> server
	PACKETEXT_GargoyleFly		= 0x32,	// client  -> server
	PACKETEXT_WheelBoatMove		= 0x33,	// client  -> server
	PACKETEXT_QTY
};

enum PACKETENC_TYPE		// encoded packets used by PACKET_EncodedData
{
	PACKETENC_HouseDesign_Backup	= 0x02,	// client  -> server
	PACKETENC_HouseDesign_Restore	= 0x03,	// client  -> server
	PACKETENC_HouseDesign_Commit	= 0x04,	// client  -> server
	PACKETENC_HouseDesign_RemItem	= 0x05,	// client  -> server
	PACKETENC_HouseDesign_AddItem	= 0x06,	// client  -> server
	PACKETENC_HouseDesign_Exit		= 0x0C,	// client  -> server
	PACKETENC_HouseDesign_AddStair	= 0x0D,	// client  -> server
	PACKETENC_HouseDesign_Sync		= 0x0E,	// client  -> server
	PACKETENC_HouseDesign_Clear		= 0x10,	// client  -> server
	PACKETENC_HouseDesign_Switch	= 0x12,	// client  -> server
	PACKETENC_HouseDesign_AddRoof	= 0x13,	// client  -> server
	PACKETENC_HouseDesign_RemRoof	= 0x14,	// client  -> server
	PACKETENC_CombatAbility			= 0x19,	// client  -> server
	PACKETENC_HouseDesign_Revert	= 0x1A,	// client  -> server
	PACKETENC_EquipLastWeapon		= 0x1E,	// client  -> server
	PACKETENC_GuildButton			= 0x28,	// client  -> server
	PACKETENC_QuestButton			= 0x32,	// client  -> server
	PACKETENC_QTY
};

enum PARTYMSG_TYPE
{
	PARTYMSG_Add = 1,			// client <-> server
	PARTYMSG_Remove,			// client <-> server
	PARTYMSG_MsgMember,			// client  -> server
	PARTYMSG_MsgAll,			// client <-> server
	PARTYMSG_Disband,			// client  -> server
	PARTYMSG_ToggleLooting,		// client  -> server
	PARTYMSG_Invite,			// client <-  server
	PARTYMSG_Accept,			// client  -> server
	PARTYMSG_Decline			// client  -> server
};

enum SECURETRADE_TYPE
{
	SECURETRADE_Open,
	SECURETRADE_Close,
	SECURETRADE_Accept,
	SECURETRADE_UpdateGold,		// TOL clients only
	SECURETRADE_UpdateLedger	// TOL clients only
};

enum SEASON_TYPE
{
	SEASON_Spring,
	SEASON_Summer,
	SEASON_Fall,
	SEASON_Winter,
	SEASON_Desolate
};

enum BULLETINBOARD_TYPE
{
	BULLETINBOARD_Name,			// client <-  server
	BULLETINBOARD_MsgTitle,		// client <-  server
	BULLETINBOARD_MsgFull,		// client <-  server
	BULLETINBOARD_ReqFull,		// client  -> server
	BULLETINBOARD_ReqTitle,		// client  -> server
	BULLETINBOARD_PostMsg,		// client  -> server
	BULLETINBOARD_DeleteMsg		// client  -> server
};

enum EXTCMD_TYPE
{
	EXTCMD_UseSkill			= 0x24,
	EXTCMD_CastSpellBook	= 0x27,
	EXTCMD_UseScroll		= 0x2F,
	EXTCMD_OpenSpellbook	= 0x43,
	EXTCMD_CastSpell		= 0x56,
	EXTCMD_OpenDoor			= 0x58,
	EXTCMD_Gesture			= 0xC7,
	EXTCMD_InvokeVirtue		= 0xF4
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

enum INPVAL_TYPE
{
	INPVAL_Disabled,
	INPVAL_Text,
	INPVAL_Numeric
};

enum MAPCMD_TYPE
{
	MAPCMD_AddPin,
	MAPCMD_InsertPin,
	MAPCMD_MovePin,
	MAPCMD_RemovePin,
	MAPCMD_ClearPins,
	MAPCMD_ToggleEdit_Request,
	MAPCMD_ToggleEdit_Reply
};

enum MAPWAYPOINT_TYPE
{
	MAPWAYPOINT_Remove,
	MAPWAYPOINT_Corpse,
	MAPWAYPOINT_PartyMember,
	MAPWAYPOINT_Unk1,
	MAPWAYPOINT_QuestGiver,
	MAPWAYPOINT_NewPlayerQuest,
	MAPWAYPOINT_Healer,
	MAPWAYPOINT_Unk2,
	MAPWAYPOINT_Unk3,
	MAPWAYPOINT_Unk4,
	MAPWAYPOINT_Unk5,
	MAPWAYPOINT_Shrine,
	MAPWAYPOINT_Moongate,
	MAPWAYPOINT_Unk6,
	MAPWAYPOINT_GreenDot,
	MAPWAYPOINT_GreenDotFlashing
};

enum WEATHER_TYPE
{
	WEATHER_Rain,
	WEATHER_Storm,
	WEATHER_Snow,
	WEATHER_Cloudy,
	WEATHER_Default	= 0xFE,
	WEATHER_Clear	= 0xFF
};

enum SCROLL_TYPE
{
	SCROLL_Tips,
	SCROLL_Notices,
	SCROLL_Updates
};

enum EFFECT_TYPE
{
	EFFECT_BOLT,
	EFFECT_LIGHTNING,
	EFFECT_XYZ,
	EFFECT_OBJ,
	EFFECT_FADE_SCREEN		// clients >= 6.0.0.0 only
};

enum NOTO_TYPE
{
	NOTO_INVALID,
	NOTO_GOOD,
	NOTO_GUILD_SAME,
	NOTO_NEUTRAL,
	NOTO_CRIMINAL,
	NOTO_GUILD_WAR,
	NOTO_EVIL,
	NOTO_INVUL
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
			BYTE m_Cmd;			// PACKET_* (size = unknown)
			BYTE m_Arg[1];
		} Default;

		struct
		{
			BYTE m_Cmd;			// PACKET_ServersReq (size = 62)
			char m_acctname[MAX_ACCOUNT_NAME_SIZE];
			char m_acctpass[MAX_NAME_SIZE];
			BYTE m_loginKey;	// 61 = NextLoginKey from uo.cfg
		} ServersReq;

		struct
		{
			BYTE m_Cmd;			// PACKET_CharListReq (size = 65)
			NDWORD m_Account;	// 1-4 = account id from PACKET_Relay message to log server.
			char m_acctname[MAX_ACCOUNT_NAME_SIZE];	// This is corrupted or encrypted seperatly ?
			char m_acctpass[MAX_NAME_SIZE];
		} CharListReq;

		struct 
		{
			BYTE m_Cmd;			// PACKET_EncryptionReply
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
#define CHARMODE_POISON			0x04	// green status bar. (note: see PACKET_HealthBarColor for SA)
#define CHARMODE_FLYING			0x04	// flying (gargoyles, SA)
#define CHARMODE_YELLOW			0x08	// yellow status bar. (note: see PACKET_HealthBarColor for SA)
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
