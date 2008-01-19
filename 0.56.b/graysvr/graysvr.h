//
// GraySvr.H
// Copyright Menace Software (www.menasoft.com).
// Precompiled header
//

#ifndef _INC_GRAYSVR_H_
#define _INC_GRAYSVR_H_
#pragma once

// NOTE: If we want a max number of sockets we must compile for it !
#ifdef _WIN32
	#undef FD_SETSIZE		// This shuts off a warning
	#define FD_SETSIZE 1024 // for max of n users ! default = 64
#endif

//	Enable advanced exceptions catching. Consumes some more resources, but is very useful
//	for debug on a running environment. Also it makes sphere more stable since exceptions
//	are local
#ifndef _DEBUG
	#ifndef EXCEPTIONS_DEBUG
	#define EXCEPTIONS_DEBUG
	#endif
#endif

//#define DEBUGWALKSTUFF 1
//#ifdef _DEBUG
#ifdef DEBUGWALKSTUFF
	#define WARNWALK(_x_)		g_pLog->EventWarn _x_;
#else
	#define WARNWALK(_x_)		if ( g_Cfg.m_wDebugFlags & DEBUGF_WALK ) { g_pLog->EventWarn _x_; }
#endif

class CServTime
{
#undef GetCurrentTime
#define TICK_PER_SEC 10
	// A time stamp in the server/game world.
public:
	static const char *m_sClassName;
	long m_lPrivateTime;
public:
	long GetTimeRaw() const
	{
		if ( m_lPrivateTime < 0 )
			return 0;

		return m_lPrivateTime;
	}
	int GetTimeDiff( const CServTime & time ) const
	{
		return( m_lPrivateTime - time.m_lPrivateTime );
	}
	void Init()
	{
		m_lPrivateTime = 0;
	}
	void InitTime( long lTimeBase )
	{
		if ( lTimeBase < 0 )
			lTimeBase = 0;

		m_lPrivateTime = lTimeBase;
	}
	bool IsTimeValid() const
	{
		return( m_lPrivateTime > 0 ? true : false );
	}
	CServTime operator+( int iTimeDiff ) const
	{
		CServTime time;
		time.m_lPrivateTime = m_lPrivateTime + iTimeDiff;
		if ( time.m_lPrivateTime < 0 )
			time.m_lPrivateTime = 0;

		return( time );
	}
	CServTime operator-( int iTimeDiff ) const
	{
		CServTime time;
		time.m_lPrivateTime = m_lPrivateTime - iTimeDiff;
		if ( time.m_lPrivateTime < 0 )
			time.m_lPrivateTime = 0;

		return( time );
	}
	int operator-( CServTime time ) const
	{
		return(m_lPrivateTime-time.m_lPrivateTime);
	}
	bool operator==(CServTime time) const
	{
		return(m_lPrivateTime==time.m_lPrivateTime);
	}
	bool operator!=(CServTime time) const
	{
		return(m_lPrivateTime!=time.m_lPrivateTime);
	}
	bool operator<(CServTime time) const
	{
		return(m_lPrivateTime<time.m_lPrivateTime);
	}
	bool operator>(CServTime time) const
	{
		return(m_lPrivateTime>time.m_lPrivateTime);
	}
	bool operator<=(CServTime time) const
	{
		return(m_lPrivateTime<=time.m_lPrivateTime);
	}
	bool operator>=(CServTime time) const
	{
		return(m_lPrivateTime>=time.m_lPrivateTime);
	}
	void SetCurrentTime()
	{
		m_lPrivateTime = GetCurrentTime().m_lPrivateTime;
	}
	static CServTime GetCurrentTime();
};

enum RESDISPLAY_VERSION
{
	RDS_T2A = 1,
	RDS_LBR,
	RDS_AOS,
	RDS_SE,
	RDS_ML,
	RDS_KR,
	RDS_QTY,
};

#include "../common/graycom.h"
#include "../common/graymul.h"
#include "../common/grayproto.h"
#include "../common/CGrayInst.h"
#include "../common/CResourceBase.h"
#include "../common/CRegion.h"
#include "../common/CGrayMap.h"
#include "../sphere/mutex.h"
#include "../sphere/threads.h"
#ifndef _WIN32
	#include "../sphere/linuxev.h"
#endif
#include "../common/CQueue.h"
#include "../common/CSectorTemplate.h"
#include "../common/CDataBase.h"

#include "CResource.h"
#include "CServRef.h"
#include "CAccount.h"

#include <vector>
#include <string>
using namespace std;

class CClient;
class CAccount;
class CWebPageDef;
class CServerDef;
class CCharBase;
class CItemBase;
class CItemContainer;
class CItemMessage;
class CItemMap;
class CItemMultiCustom;
class CDataBase;

///////////////////////////////////////////////

// Text mashers.

extern DIR_TYPE GetDirStr( LPCTSTR pszDir );
extern LPCTSTR GetTimeMinDesc( int dwMinutes );
extern int FindStrWord( LPCTSTR pTextSearch, LPCTSTR pszKeyWord );

extern struct CLog : public CFileText, public CEventLog
{
	// subject matter. (severity level is first 4 bits, LOGL_EVENT)
#define LOGM_ACCOUNTS		0x00080
//#define LOGM_INIT			0x00100	// start up messages.
#define LOGM_SAVE			0x00200	// world save status.
#define LOGM_CLIENTS_LOG	0x00400	// all clients as they log in and out.
#define LOGM_GM_PAGE		0x00800	// player gm pages.
#define LOGM_PLAYER_SPEAK	0x01000	// All that the players say.
#define LOGM_GM_CMDS		0x02000	// Log all GM commands.
#define LOGM_CHEAT			0x04000	// Probably an exploit !
#define LOGM_KILLS			0x08000	// Log player combat results.
#define LOGM_HTTP			0x10000
//#define	LOGM_NOCONTEXT		0x20000	// do not include context information
//#define LOGM_DEBUG			0x40000	// debug kind of message with DEBUG: prefix

private:
	DWORD m_dwMsgMask;			// Level of log detail messages. IsLogMsg()
	CGTime m_dateStamp;			// last real time stamp.
	CGString m_sBaseDir;

	const CScript * m_pScriptContext;	// The current context.
	const CScriptObj * m_pObjectContext;	// The current context.

	static CGTime sm_prevCatchTick;	// don't flood with these.
public:
	bool m_fLockOpen;
#ifndef _WIN32
	bool m_fColoredConsole;
#endif

public:
	const CScript * SetScriptContext( const CScript * pScriptContext )
	{
		const CScript * pOldScript = m_pScriptContext;
		m_pScriptContext = pScriptContext;
		return( pOldScript );
	}
	const CScriptObj * SetObjectContext( const CScriptObj * pObjectContext )
	{
		const CScriptObj * pOldObject = m_pObjectContext;
		m_pObjectContext = pObjectContext;
		return( pOldObject );
	}
	bool SetFilePath( LPCTSTR pszName )
	{
		ASSERT( ! IsFileOpen());
		return CFileText::SetFilePath( pszName );
	}

	LPCTSTR GetLogDir() const
	{
		return( m_sBaseDir );
	}
	bool OpenLog( LPCTSTR pszName = NULL );	// name set previously.
	DWORD GetLogMask() const
	{
		return( m_dwMsgMask &~ 0x0f ) ;
	}
	void SetLogMask( DWORD dwMask )
	{
		m_dwMsgMask = GetLogLevel() | dwMask;
	}
	bool IsLoggedMask( DWORD dwMask ) const
	{
		return(( GetLogMask() & ( dwMask &~ 0x0f )) ? true : false );
	}
	LOGL_TYPE GetLogLevel() const
	{
		return((LOGL_TYPE)( m_dwMsgMask & 0x0f ));
	}
	void SetLogLevel( LOGL_TYPE level )
	{
		m_dwMsgMask = GetLogMask() | level;
	}
	bool IsLogged( DWORD wMask ) const
	{
		return( IsLoggedMask(wMask) ||
			( (DWORD)GetLogLevel() >= ( wMask & 0x0f )));
	}

	virtual int EventStr( DWORD wMask, LPCTSTR pszMsg );
	void _cdecl CatchEvent( CGrayError * pErr, LPCTSTR pszCatchContext, ...  );

	CLog()
	{
#ifndef _WIN32
		m_fColoredConsole = false;
#endif
		m_fLockOpen = false;
		m_pScriptContext = NULL;
		m_dwMsgMask = LOGL_EVENT |
			LOGM_INIT | LOGM_CLIENTS_LOG | LOGM_GM_PAGE;
		SetFilePath( GRAY_FILE "log.log" );	// default name to go to.
	}

} g_Log;		// Log file

//////////////////

class CGMPage : public CGObListRec, public CScriptObj
{
	// RES_GMPAGE
	// ONly one page allowed per account at a time.
	static LPCTSTR const sm_szLoadKeys[];
private:
	CGString m_sAccount;	// The account that paged me.
	CClient * m_pGMClient;	// assigned to a GM
	CGString m_sReason;		// Players Description of reason for call.
public:
	static const char *m_sClassName;
	// Queue a GM page. (based on account)
	CServTime  m_timePage;	// Time of the last call.
	CPointMap  m_ptOrigin;		// Origin Point of call.
public:
	CGMPage( LPCTSTR pszAccount );
	~CGMPage();
	CAccountRef FindAccount() const;
	LPCTSTR GetAccountStatus() const;
	LPCTSTR GetName() const
	{
		return( m_sAccount );
	}
	LPCTSTR GetReason() const
	{
		return( m_sReason );
	}
	void SetReason( LPCTSTR pszReason )
	{
		m_sReason = pszReason;
	}
	CClient * FindGMHandler() const
	{
		return( m_pGMClient );
	}
	void ClearGMHandler()
	{
		m_pGMClient = NULL;
	}
	void SetGMHandler( CClient * pClient )
	{
		m_pGMClient = pClient;
	}
	int GetAge() const;

	bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );
	void r_Write( CScript & s ) const;
	bool r_LoadVal( CScript & s );

	CGMPage * GetNext() const
	{
		return( STATIC_CAST <CGMPage*>( CGObListRec::GetNext()));
	}
};

class CChat;
class CChatChannel;
class CChatChanMember
{
	// This is a member of the CClient.
private:
	bool m_fChatActive;
	bool m_fReceiving;
	bool m_fAllowWhoIs;
	CChatChannel * m_pChannel;	// I can only be a member of one chan at a time.
public:
	static const char *m_sClassName;
	CGObArray< CGString * > m_IgnoredMembers;	// Player's list of ignored members
private:
	friend class CChatChannel;
	friend class CChat;
	// friend CClient;
	bool GetWhoIs() const { return m_fAllowWhoIs; }
	void SetWhoIs(bool fAllowWhoIs) { m_fAllowWhoIs = fAllowWhoIs; }
	bool IsReceivingAllowed() const { return m_fReceiving; }
	LPCTSTR GetChatName() const;

	int FindIgnoringIndex( LPCTSTR pszName) const;

protected:
	void SetChatActive();
	void SetChatInactive();
public:
	CChatChanMember()
	{
		m_fChatActive = false;
		m_pChannel = NULL;
		m_fReceiving = true;
		m_fAllowWhoIs = true;
	}
	virtual ~CChatChanMember();

	CClient * GetClient();
	const CClient * GetClient() const;

	bool IsChatActive() const
	{
		return( m_fChatActive );
	}

	void SetReceiving(bool fOnOff)
	{
		if (m_fReceiving != fOnOff)
			ToggleReceiving();
	}
	void ToggleReceiving();

	void PermitWhoIs();
	void ForbidWhoIs();
	void ToggleWhoIs();

	CChatChannel * GetChannel() const { return m_pChannel; }
	void SetChannel(CChatChannel * pChannel) { m_pChannel = pChannel; }
	void SendChatMsg( CHATMSG_TYPE iType, LPCTSTR pszName1 = NULL, LPCTSTR pszName2 = NULL, CLanguageID lang = 0 );
	void RenameChannel(LPCTSTR pszName);

	void Ignore(LPCTSTR pszName);
	void DontIgnore(LPCTSTR pszName);
	void ToggleIgnore(LPCTSTR pszName);
	void ClearIgnoreList();
	bool IsIgnoring(LPCTSTR pszName) const
	{
		return( FindIgnoringIndex( pszName ) >= 0 );
	}
};

class CChatChannel : public CGObListRec
{
	// a number of clients can be attached to this chat channel.
private:
	friend class CChatChanMember;
	friend class CChat;
	CGString m_sName;
	CGString m_sPassword;
	bool m_fVoiceDefault;	// give others voice by default.
public:
	static const char *m_sClassName;
	CGObArray< CGString * > m_NoVoices;// Current list of channel members with no voice
	CGObArray< CGString * > m_Moderators;// Current list of channel's moderators (may or may not be currently in the channel)
	CGPtrTypeArray< CChatChanMember* > m_Members;	// Current list of members in this channel
private:
	void SetModerator(LPCTSTR pszName, bool fFlag = true);
	void SetVoice(LPCTSTR pszName, bool fFlag = true);
	void RenameChannel(CChatChanMember * pBy, LPCTSTR pszName);
	int FindMemberIndex( LPCTSTR pszName ) const;

public:
	CChatChannel(LPCTSTR pszName, LPCTSTR pszPassword = NULL)
	{
		m_sName = pszName;
		m_sPassword = pszPassword;
		m_fVoiceDefault = true;
	};
	CChatChannel* GetNext() const
	{
		return( static_cast <CChatChannel *>( CGObListRec::GetNext()));
	}
	LPCTSTR GetName() const
	{
		return( m_sName );
	}
	LPCTSTR GetModeString() const
	{
		// (client needs this) "0" = not passworded, "1" = passworded
		return(( IsPassworded()) ? "1" : "0" );
	}

	LPCTSTR GetPassword() const
	{
		return( m_sPassword );
	}
	void SetPassword( LPCTSTR pszPassword)
	{
		m_sPassword = pszPassword;
		return;
	}
	bool IsPassworded() const
	{
		return ( !m_sPassword.IsEmpty());
	}

	bool GetVoiceDefault()  const { return m_fVoiceDefault; }
	void SetVoiceDefault(bool fVoiceDefault) { m_fVoiceDefault = fVoiceDefault; }
	void ToggleVoiceDefault(LPCTSTR  pszBy);
	void DisableVoiceDefault(LPCTSTR  pszBy);
	void EnableVoiceDefault(LPCTSTR  pszBy);
	void Emote(LPCTSTR pszBy, LPCTSTR pszMsg, CLanguageID lang = 0 );
	void WhoIs(LPCTSTR pszBy, LPCTSTR pszMember);
	bool AddMember(CChatChanMember * pMember);
	void KickMember( CChatChanMember *pByMember, CChatChanMember * pMember );
	void Broadcast(CHATMSG_TYPE iType, LPCTSTR pszName, LPCTSTR pszText, CLanguageID lang = 0, bool fOverride = false);
	void SendThisMember(CChatChanMember * pMember, CChatChanMember * pToMember = NULL);
	void SendMembers(CChatChanMember * pMember);
	void RemoveMember(CChatChanMember * pMember);
	CChatChanMember * FindMember(LPCTSTR pszName) const
	{
		int i = FindMemberIndex( pszName );
		return(( i >= 0 ) ? m_Members[i] : NULL );
	}
	bool RemoveMember(LPCTSTR pszName)
	{
		int i = FindMemberIndex( pszName );
		if ( i >= 0 )
		{
			RemoveMember(m_Members[i]);
			return true;
		}
		return false;
	}
	void SetName(LPCTSTR pszName)
	{
		m_sName = pszName;
	}
	bool IsModerator(LPCTSTR pszName) const;
	bool HasVoice(LPCTSTR pszName) const;

	void MemberTalk(CChatChanMember * pByMember, LPCTSTR pszText, CLanguageID lang );
	void ChangePassword(CChatChanMember * pByMember, LPCTSTR pszPassword);
	void GrantVoice(CChatChanMember * pByMember, LPCTSTR pszName);
	void RevokeVoice(CChatChanMember * pByMember, LPCTSTR pszName);
	void ToggleVoice(CChatChanMember * pByMember, LPCTSTR pszName);
	void GrantModerator(CChatChanMember * pByMember, LPCTSTR pszName);
	void RevokeModerator(CChatChanMember * pByMember, LPCTSTR pszName);
	void ToggleModerator(CChatChanMember * pByMember, LPCTSTR pszName);
	void SendPrivateMessage(CChatChanMember * pFrom, LPCTSTR pszTo, LPCTSTR  pszMsg);
	void KickAll(CChatChanMember * pMember = NULL);
};

class CChat
{
	// All the chat channels.
private:
	bool m_fChatsOK;	// allowed to create new chats ?
	CGObList m_Channels;		// CChatChannel // List of chat channels.
private:
	void DoCommand(CChatChanMember * pBy, LPCTSTR szMsg);
	void DeleteChannel(CChatChannel * pChannel);
	void WhereIs(CChatChanMember * pBy, LPCTSTR pszName) const;
	void KillChannels();
	bool JoinChannel(CChatChanMember * pMember, LPCTSTR pszChannel, LPCTSTR pszPassword);
	bool CreateChannel(LPCTSTR pszName, LPCTSTR pszPassword, CChatChanMember * pMember);
	void CreateJoinChannel(CChatChanMember * pByMember, LPCTSTR pszName, LPCTSTR pszPassword);
	CChatChannel * FindChannel(LPCTSTR pszChannel) const
	{
		CChatChannel * pChannel = GetFirstChannel();
		for ( ; pChannel != NULL; pChannel = pChannel->GetNext())
		{
			if (strcmp(pChannel->GetName(), pszChannel) == 0)
				break;
		}
		return pChannel;
	};
public:
	static const char *m_sClassName;
	CChat()
	{
		m_fChatsOK = true;
	}

	CChatChannel * GetFirstChannel() const
	{
		return STATIC_CAST <CChatChannel *>(m_Channels.GetHead());
	}

	void EventMsg( CClient * pClient, const NCHAR * pszText, int len, CLanguageID lang ); // Text from a client

	static bool IsValidName(LPCTSTR pszName, bool fPlayer);

	void SendDeleteChannel(CChatChannel * pChannel);
	void SendNewChannel(CChatChannel * pNewChannel);
	bool IsDuplicateChannelName(const char * pszName) const
	{
		return FindChannel(pszName) != NULL;
	}

	void Broadcast(CChatChanMember * pFrom, LPCTSTR pszText, CLanguageID lang = 0, bool fOverride = false);
	void QuitChat(CChatChanMember * pClient);
	static void DecorateName(CGString & sName, const CChatChanMember * pMember = NULL, bool fSystem = false);
};

class CDialogResponseArgs : public CScriptTriggerArgs
{
	// The scriptable return from a gump dialog.
	// "ARG" = dialog args script block. ex. ARGTXT(id), ARGCHK(i)
public:
	static const char *m_sClassName;
	struct TResponseString
	{
		const WORD m_ID;
		CGString const m_sText;

		TResponseString(WORD id, LPCTSTR pszText):m_ID(id),m_sText(pszText)
		{
		}
	};

	CGTypedArray<DWORD,DWORD>		m_CheckArray;
	CGObArray<TResponseString *>	m_TextArray;
public:
	void AddText( WORD id, LPCTSTR pszText )
	{
		m_TextArray.Add(new TResponseString(id, pszText));
	}
	LPCTSTR GetName() const
	{
		return "ARGD";
	}
	bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );
};

struct CMenuItem 	// describe a menu item.
{
public:
	WORD m_id;			// ITEMID_TYPE in base set.
	WORD m_color;
	CGString m_sText;
public:
	bool ParseLine( TCHAR * pszArgs, CScriptObj * pObjBase, CTextConsole * pSrc );
};

struct CSectorEnviron	// When these change it is an CTRIG_EnvironChange,
{
#define LIGHT_OVERRIDE 0x80
public:
	BYTE m_Light;		// the calculated light level in this area. |0x80 = override.
	SEASON_TYPE m_Season;		// What is the season for this sector.
	WEATHER_TYPE m_Weather;		// the weather in this area now.
public:
	CSectorEnviron()
	{
		m_Light = LIGHT_BRIGHT;	// set based on time later.
		m_Season = SEASON_Summer;
		m_Weather = WEATHER_DRY;
	}
	void SetInvalid()
	{
		// Force a resync of all this. we changed location by teleport etc.
		m_Light = -1;	// set based on time later.
		m_Season = SEASON_QTY;
		m_Weather = WEATHER_DEFAULT;
	}
};

enum CLIMODE_TYPE	// What mode is the client to server connection in ? (waiting for input ?)
{
	// setup events ------------------------------------------------

	CLIMODE_SETUP_CONNECTING,
	CLIMODE_SETUP_SERVERS,		// client has received the servers list.
	CLIMODE_SETUP_RELAY,		// client has been relayed to the game server. wait for new login.
	CLIMODE_SETUP_CHARLIST,	// client has the char list and may (play a char, delete a char, create a new char)

	// Capture the user input for this mode. ------------------------------------------
	CLIMODE_NORMAL,		// No targetting going on. we are just walking around etc.

	// asyc events enum here. --------------------------------------------------------
	CLIMODE_DRAG,			// I'm dragging something. (not quite a targeting but similar)
	CLIMODE_DEATH,			// The death menu is up.
	CLIMODE_DYE,			// The dye dialog is up.
	CLIMODE_INPVAL,		// special text input dialog (for setting item attrib)

	// Some sort of general gump dialog ----------------------------------------------
	CLIMODE_DIALOG,		// from RES_DIALOG

	// Hard-coded (internal) gumps.
	CLIMODE_DIALOG_ADMIN,
	CLIMODE_DIALOG_GUILD,	// reserved.
	CLIMODE_DIALOG_HAIR_DYE, // Using hair dye // Not used anymore
	CLIMODE_DIALOG_TOME,
	CLIMODE_DIALOG_VIRTUE = 0x1CD,

	// Making a selection from a menu. ----------------------------------------------
	CLIMODE_MENU,		// RES_MENU

	// Hard-coded (internal) menus.
	CLIMODE_MENU_SKILL,		// result of some skill. tracking, tinkering, blacksmith, etc.
	CLIMODE_MENU_SKILL_TRACK_SETUP,
	CLIMODE_MENU_SKILL_TRACK,
	CLIMODE_MENU_GM_PAGES,		// show me the gm pages .
	CLIMODE_MENU_EDIT,		// edit the contents of a container.

	// promting for text input.------------------------------------------------------
	//CLIMODE_PROMPT,					// Some sort of text prompt input.
	CLIMODE_PROMPT_NAME_RUNE,
	CLIMODE_PROMPT_NAME_KEY,		// naming a key.
	CLIMODE_PROMPT_NAME_SIGN,		// name a house sign
	CLIMODE_PROMPT_NAME_SHIP,
	CLIMODE_PROMPT_GM_PAGE_TEXT,	// allowed to enter text for page.
	CLIMODE_PROMPT_VENDOR_PRICE,	// What would you like the price to be ?
	CLIMODE_PROMPT_TARG_VERB,		// Send a msg to another player.
	CLIMODE_PROMPT_SCRIPT_VERB,		// Script verb
	CLIMODE_PROMPT_STONE_NAME,		// prompt for text.
	CLIMODE_PROMPT_STONE_SET_ABBREV,
	CLIMODE_PROMPT_STONE_SET_TITLE,
	CLIMODE_PROMPT_STONE_GRANT_TITLE,

	// Targetting mouse cursor. -------------------------------------------------------------
	CLIMODE_MOUSE_TYPE,	// Greater than this = mouse type targetting.

	// GM targetting command stuff.
	CLIMODE_TARG_OBJ_SET,		// Set some attribute of the item i will show.
	CLIMODE_TARG_OBJ_INFO,		// what item do i want props for ?
	CLIMODE_TARG_OBJ_FUNC,

	CLIMODE_TARG_UNEXTRACT,		// Break out Multi items
	CLIMODE_TARG_ADDITEM,		// "ADDITEM" command.
	CLIMODE_TARG_LINK,			// "LINK" command
	CLIMODE_TARG_TILE,			// "TILE" command.

	// Normal user stuff. (mouse targetting)
	CLIMODE_TARG_SKILL,				// targeting a skill or spell.
	CLIMODE_TARG_SKILL_MAGERY,
	CLIMODE_TARG_SKILL_HERD_DEST,
	CLIMODE_TARG_SKILL_POISON,
	CLIMODE_TARG_SKILL_PROVOKE,

	CLIMODE_TARG_USE_ITEM,			// target for using the selected item
	CLIMODE_TARG_PET_CMD,			// targetted pet command
	CLIMODE_TARG_PET_STABLE,		// Pick a creature to stable.
	CLIMODE_TARG_REPAIR,		// attempt to repair an item.
	CLIMODE_TARG_STONE_RECRUIT,		// Recruit members for a stone	(mouse select)
	CLIMODE_TARG_STONE_RECRUITFULL,	// Recruit/make a member and set abbrev show
	CLIMODE_TARG_PARTY_ADD,

	CLIMODE_TARG_QTY,
};

// Storage for Tooltip data while in trigger on an item
class CClientTooltip
{
public:
	static const char *m_sClassName;
	DWORD m_clilocid;
	TCHAR m_args[SCRIPT_MAX_LINE_LEN];

	CClientTooltip(DWORD clilocid, LPCTSTR args = NULL)
	{
		m_clilocid = clilocid;
		if ( args )
			strcpylen(m_args, args, SCRIPT_MAX_LINE_LEN);
		else
			m_args[0] = '\0';
	}

	void __cdecl FormatArgs(LPCTSTR format, ...)
	{
		va_list vargs;
		va_start( vargs, format );

		if ( ! _vsnprintf( m_args, SCRIPT_MAX_LINE_LEN, format, vargs ) )
			strcpylen( m_args, format, SCRIPT_MAX_LINE_LEN );

		va_end( vargs );
	}
};

class CClient : public CGObListRec, public CScriptObj, public CChatChanMember, public CTextConsole
{
	// TCP/IP connection to the player or console.
private:
	static LPCTSTR const sm_szCmd_Redirect[];		// default to redirect these.

public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szRefKeys[];
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];
private:
	CChar * m_pChar;			// What char are we playing ?

	// Client last know state stuff.
	CSectorEnviron m_Env;		// Last Environment Info Sent. so i don't have to keep resending if it's the same.

	char m_fUpdateStats;	// update our own status (weight change) when done with the cycle.

	// Walk limiting code
	WORD m_wWalkCount;		// Make sure we are synced up with client walk count. may be be off by 4
	int	m_iWalkTimeAvg;
	int m_iWalkStepCount;		// Count the actual steps . Turning does not count.
	LONGLONG m_timeWalkStep;	// the last %8 walk step time.

	// Stupid Walk limiting code. (Not working really)
	DWORD	m_Walk_LIFO[16];	// Client > 1.26 must match these .
	int	m_Walk_InvalidEchos;
	int	m_Walk_CodeQty;

	// Reported ClientVersion
	bool m_bClient3d;
	bool m_bClientKR;
	int m_reportedCliver;

	// Screensize
	struct __screensize
	{
		DWORD x;
		DWORD y;
	} m_ScreenSize;

	// OxBF - 0x24 AntiCheat
	struct __bfanticheat
	{
		BYTE lastvalue;
		BYTE count;
	} m_BfAntiCheat;

	int m_packetExceptions;

public:
	CONNECT_TYPE	m_iConnectType;	// what sort of a connection is this ?
	CGSocket		m_Socket;
	bool			m_fClosed;	// flag it to get the socket closed
	CSocketAddress	m_PeerName;
	CAccount * m_pAccount;		// The account name. we logged in on

	static BYTE sm_xCompress_Buffer[MAX_BUFFER];

	CServTime m_timeLogin;		// World clock of login time. "LASTCONNECTTIME"
	CServTime m_timeLastEvent;	// Last time we got event from client.
	CServTime m_timeLastSend;		// Last time i tried to send to the client

	// GM only stuff.
	CGMPage * m_pGMPage;		// Current GM page we are connected to.
	CGrayUID m_Prop_UID;		// The object of /props (used for skills list as well!)

	// Gump stuff
	typedef std::map<int,int> OpenedGumpsMap_t;
	OpenedGumpsMap_t m_mapOpenedGumps;

	// Current operation context args for modal async operations..
private:
	CLIMODE_TYPE m_Targ_Mode;	// Type of async operation under way.
public:
	CGrayUID m_Targ_Last;	// The last object targeted by the client
	CGrayUID m_Targ_UID;			// The object of interest to apply to the target.
	CGrayUID m_Targ_PrvUID;		// The object of interest before this.
	CGString m_Targ_Text;		// Text transfered up from client.
	CPointMap  m_Targ_p;			// For script targeting,

	// Context of the targetting setup. depends on CLIMODE_TYPE m_Targ_Mode
	union
	{
		// CLIMODE_SETUP_CONNECTING
		struct
		{
			DWORD m_dwIP;
			int m_iConnect;	// used for debug only.
		} m_tmSetup;

		// CLIMODE_SETUP_CHARLIST
		CGrayUIDBase m_tmSetupCharList[MAX_CHARS_PER_ACCT];

		// CLIMODE_INPVAL
		struct
		{
			CGrayUIDBase m_UID;
			RESOURCE_ID_BASE m_PrvGumpID;	// the gump that was up before this
		} m_tmInpVal;

		// CLIMODE_MENU_*
		// CLIMODE_MENU_SKILL
		// CLIMODE_MENU_GM_PAGES
		struct
		{
			CGrayUIDBase m_UID;
			RESOURCE_ID_BASE m_ResourceID;		// What menu is this ?
			DWORD m_Item[MAX_MENU_ITEMS];	// This saves the inrange tracking targets or other context
		} m_tmMenu;	// the menu that is up.

		// CLIMODE_TARG_PET_CMD
		struct
		{
			int	m_iCmd;
			bool m_fAllPets;
		} m_tmPetCmd;	// which pet command am i targetting ?

		// CLIMODE_TARG_CHAR_BANK
		struct
		{
			LAYER_TYPE m_Layer;	// gm command targetting what layer ?
		} m_tmCharBank;

		// CLIMODE_TARG_TILE
		// CLIMODE_TARG_UNEXTRACT
		struct
		{
			CPointBase m_ptFirst; // Multi stage targetting.
			int m_Code;
			int m_id;
		} m_tmTile;

		// CLIMODE_TARG_ADDITEM
		struct
		{
			DWORD m_junk0;
			ITEMID_TYPE m_id;
			int		m_fStatic;
		} m_tmAdd;

		// CLIMODE_TARG_SKILL
		struct
		{
			SKILL_TYPE m_Skill;			// targetting what spell ?
		} m_tmSkillTarg;

		// CLIMODE_TARG_SKILL_MAGERY
		struct
		{
			SPELL_TYPE m_Spell;			// targetting what spell ?
			CREID_TYPE m_SummonID;
			bool m_fSummonPet;
		} m_tmSkillMagery;

		// CLIMODE_TARG_USE_ITEM
		struct
		{
			CGObList * m_pParent;	// the parent of the item being targetted .
		} m_tmUseItem;
	};

private:
	// Low level data transfer to client.
	XCMD_TYPE m_bin_PrvMsg;
	XCMD_TYPE m_bin_ErrMsg;
	int m_bin_msg_len;		// the current message packet to decode. (estimated length)

	// encrypt/decrypt stuff.
	CCrypt m_Crypt;			// Client source communications are always encrypted.
	static CHuffman m_Comp;

	class CTooltipData
	{
		private:
			long time;
			int len;
			DWORD objUid;
			BYTE * tooltipData;

		public:
			CTooltipData(const void * bData, int length, DWORD oUid, long lTime)
			{
				time = lTime;
				len = length;
				objUid = oUid;
				tooltipData = new BYTE[length];
				memcpy(tooltipData, bData, length);
			}

			~CTooltipData()
			{
				delete[] tooltipData;
			}

			long GetTime() { return time; }
			int GetLength() { return len; }
			BYTE * GetData() { return tooltipData; }
			DWORD GetObjUid() { return objUid; }
			CObjBase * GetObject() { return CGrayUID(objUid).ObjFind(); }
			bool IsObjectValid() { return (CGrayUID(objUid).ObjFind() != NULL); }
	};

	long m_LastTooltipSend;
	std::vector<CTooltipData *> m_TooltipQueue;
	void CleanTooltipQueue();

private:
	bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );

	bool OnRxConsoleLoginComplete();
	bool OnRxConsole( const BYTE * pData, int len );
	bool OnRxPing( const BYTE * pData, int len );
	bool OnRxWebPageRequest( CWebPageDef * pPage, LPCTSTR pszMatch );
	bool OnRxWebPageRequest( BYTE * pRequest, int len );

	LOGIN_ERR_TYPE LogIn( CAccountRef pAccount, CGString & sMsg );
	LOGIN_ERR_TYPE LogIn( LPCTSTR pszName, LPCTSTR pPassword, CGString & sMsg );

	bool IsBlockedIP() const;
	CLogIP * GetLogIP() const;
	void	UpdateLogIPConnecting( bool fIncrease );
	void	UpdateLogIPConnected( bool fIncrease );
	int	GetLogIPConnecting() const;
	int	GetLogIPConnected() const;

	// Low level message traffic.
	bool xCheckMsgSize0( int len );	// check packet only for sizeof
	bool xCheckMsgSize( int len );	// check packet.
	void xDumpPacket(int iDataLen, const BYTE * pData); // dump the packet received

	int  Setup_FillCharList( CEventCharDef * pCharList, const CChar * pCharFirst );

	bool CanInstantLogOut() const;
	void Cmd_GM_PageClear();
	void GetAdjustedCharID( const CChar * pChar, CREID_TYPE & id, HUE_TYPE &wHue );
	void GetAdjustedItemID( const CChar * pChar, const CItem * pItem, ITEMID_TYPE & id, HUE_TYPE &wHue );

	TRIGRET_TYPE Menu_OnSelect( RESOURCE_ID_BASE rid, int iSelect, CObjBase * pObj );
	TRIGRET_TYPE Dialog_OnButton( RESOURCE_ID_BASE rid, DWORD dwButtonID, CObjBase * pObj, CDialogResponseArgs * pArgs );

	LOGIN_ERR_TYPE Login_ServerList( const char * pszAccount, const char * pszPassword );		// Initial login (Login on "loginserver", new format)
	bool Login_Relay( int iServer );			// Relay player to a certain IP
	void Announce( bool fArrive ) const;

	LOGIN_ERR_TYPE Setup_ListReq( const char * pszAccount, const char * pszPassword, bool fTest );	// Gameserver login and character listing
	LOGIN_ERR_TYPE Setup_Start( CChar * pChar );	// Send character startup stuff to player
	void Setup_CreateDialog( const CEvent * pEvent );	// All the character creation stuff
	DELETE_ERR_TYPE Setup_Delete( int iSlot );			// Deletion of character
	LOGIN_ERR_TYPE Setup_Play( int iSlot );		// After hitting "Play Character" button

	// GM stuff.
	bool OnTarg_Obj_Set( CObjBase * pObj );
	bool OnTarg_Obj_Info( CObjBase * pObj, const CPointMap & pt, ITEMID_TYPE id );
	bool OnTarg_Obj_Function( CObjBase * pObj, const CPointMap & pt, ITEMID_TYPE id );

	bool OnTarg_UnExtract( CObjBase * pObj, const CPointMap & pt ) ;
	bool OnTarg_Stone_Recruit(CChar* pChar, bool bFull = false);
	bool OnTarg_Item_Add( CObjBase * pObj, const CPointMap & pt ) ;
	bool OnTarg_Item_Link( CObjBase * pObj );
	bool OnTarg_Tile( CObjBase * pObj, const CPointMap & pt );

	// Normal user stuff.
	bool OnTarg_Use_Deed( CItem * pDeed, const CPointMap &pt );
	bool OnTarg_Use_Item( CObjBase * pObj, CPointMap & pt, ITEMID_TYPE id );
	bool OnTarg_Party_Add( CChar * pChar );
	CItem* OnTarg_Use_Multi( const CItemBase * pItemDef, const CPointMap & pt, WORD wAttr, HUE_TYPE wHue );

	int OnSkill_AnimalLore( CGrayUID uid, int iTestLevel, bool fTest );
	int OnSkill_Anatomy( CGrayUID uid, int iTestLevel, bool fTest );
	int OnSkill_Forensics( CGrayUID uid, int iTestLevel, bool fTest );
	int OnSkill_EvalInt( CGrayUID uid, int iTestLevel, bool fTest );
	int OnSkill_ArmsLore( CGrayUID uid, int iTestLevel, bool fTest );
	int OnSkill_ItemID( CGrayUID uid, int iTestLevel, bool fTest );
	int OnSkill_TasteID( CGrayUID uid, int iTestLevel, bool fTest );

	bool OnTarg_Skill_Magery( CObjBase * pObj, const CPointMap & pt );
	bool OnTarg_Skill_Herd_Dest( CObjBase * pObj, const CPointMap & pt );
	bool OnTarg_Skill_Poison( CObjBase * pObj );
	bool OnTarg_Skill_Provoke( CObjBase * pObj );
	bool OnTarg_Skill( CObjBase * pObj );

	bool OnTarg_Pet_Command( CObjBase * pObj, const CPointMap & pt );
	bool OnTarg_Pet_Stable( CChar * pCharPet );

	// Commands from client
	void Event_ClientVersion( const char * pData, int Len );
	void Event_Target( const CEvent * pEvent );
	void Event_Attack( CGrayUID uid );
	void Event_Skill_Locks( const CEvent * pEvent );
	void Event_Skill_Use( SKILL_TYPE x ); // Skill is clicked on the skill list
	void Event_Tips( WORD i); // Tip of the day window
	void Event_Book_Title( CGrayUID uid, LPCTSTR pszTitle, LPCTSTR pszAuthor );
	void Event_Book_Page( CGrayUID uid, const CEvent * pEvent ); // Book window
	void Event_Item_Dye( CGrayUID uid, HUE_TYPE wHue );	// Rehue an item
	void Event_Item_Pickup( CGrayUID uid, int amount ); // Client grabs an item
	void Event_Item_Equip( const CEvent * pEvent ); // Item is dropped on paperdoll
	inline void Event_Item_Drop_Fail( CItem * pItem );
	void Event_Item_Drop( const CEvent * pEvent ); // Item is dropped on ground
	inline void Event_VendorBuy_Cheater( int iCode = 0 );
	void Event_VendorBuy( CGrayUID uidVendor, const CEvent * pEvent );
	void Event_SecureTrade( CGrayUID uid, const CEvent * pEvent );
	bool Event_DeathOption( DEATH_MODE_TYPE mode, const CEvent * pEvent );
	void Event_Walking( BYTE rawdir, BYTE count, DWORD dwCryptCode = 0 ); // Player moves
	void Event_CombatMode( bool fWar ); // Only for switching to combat mode
	void Event_MenuChoice( const CEvent * pEvent ); // Choice from GMMenu or Itemmenu received
	void Event_PromptResp( LPCTSTR pszText, int len );
	void Event_Talk_Common(char *szText ); // PC speech
	void Event_Talk( LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, bool bNoStrip = false ); // PC speech
	void Event_TalkUNICODE( const CEvent * pEvent );
	void Event_SingleClick( CGrayUID uid );
	void Event_SetName( CGrayUID uid, const char * pszCharName );
	void Event_ExtCmd( EXTCMD_TYPE type, const char * pszName );
	bool Event_Command( LPCTSTR pszCommand, TALKMODE_TYPE mode = TALKMODE_SYSTEM ); // Client entered a '/' command like /ADD
	void Event_GumpInpValRet( const CEvent * pEvent );
	void Event_GumpDialogRet( const CEvent * pEvent );
	void Event_ToolTip( CGrayUID uid );
	void Event_ExtData( EXTDATA_TYPE type, const CExtData * pData, int len );
	void Event_ExtAosData( EXTAOS_TYPE type, const CExtAosData * pData, DWORD m_uid, int len );
	void Event_HouseDesigner( EXTAOS_TYPE type, const CExtAosData * pData, DWORD m_uid, int len );
	void Event_MailMsg( CGrayUID uid1, CGrayUID uid2 );
	void Event_Profile( BYTE fWriteMode, CGrayUID uid, const CEvent * pEvent );
	void Event_MapEdit( CGrayUID uid, const CEvent * pEvent );
	void Event_BBoardRequest( CGrayUID uid, const CEvent * pEvent );
	void Event_ChatButton(const NCHAR * pszName); // Client's chat button was pressed
	void Event_ChatText( const NCHAR * pszText, int len, CLanguageID lang = 0 ); // Text from a client
	bool Event_WalkingCheck(DWORD dwEcho);
	void Event_AOSItemInfo( int count , const NDWORD * uidList );
	void Event_AllNames3D( CGrayUID uid );
	void Event_BugReport( const NCHAR * pszText, int len, BUGREPORT_TYPE type, CLanguageID lang = 0 );
	void Event_MacroEquipItems( const NDWORD * pItems, int count );
	void Event_MacroUnEquipItems( const NWORD * pLayers, int count );

public:
	inline void Event_VendorSell_Cheater( int iCode = 0 );
	void Event_VendorSell( CGrayUID uidVendor, const CEvent * pEvent );
	bool Event_DoubleClick( CGrayUID uid, bool fMacro, bool fTestTouch, bool fScript = false );

	// translated commands.
private:
	void Cmd_GM_PageInfo();
	int Cmd_Extract( CScript * pScript, CRectMap &rect, int & zlowest );
public:
	bool Cmd_CreateItem( ITEMID_TYPE id, bool fStatic = false );
	bool Cmd_CreateChar( CREID_TYPE id, SPELL_TYPE iSpell = SPELL_Summon, bool fPet = true );

	void Cmd_GM_PageMenu( int iEntryStart = 0 );
	void Cmd_GM_PageCmd( LPCTSTR pCmd );
	void Cmd_GM_PageSelect( int iSelect );
	void Cmd_GM_Page( LPCTSTR pszreason); // Help button (Calls GM Call Menus up)

	bool Cmd_Skill_Menu( RESOURCE_ID_BASE rid, int iSelect = -1 );
	bool Cmd_Skill_Smith( CItem * pIngots );
	bool Cmd_Skill_Magery( SPELL_TYPE iSpell, CObjBase * pSrc );
	bool Cmd_Skill_Tracking( int track_type = -1, bool fExec = false ); // Fill menu with specified creature types
	bool Cmd_Skill_Inscription();
	bool Cmd_Skill_Alchemy( CItem * pItem );
	bool Cmd_Skill_Cartography( int iLevel );
	bool Cmd_SecureTrade( CChar * pChar, CItem * pItem );
	bool Cmd_Control( CChar * pChar );

public:
	CQueueBytes m_bin;		// CEvent in buffer. (from client)

private:
	std::queue<int> m_vExtPacketLengths;
	bool m_sendingData;
	CQueueBytes m_bout;		// CCommand out buffer. (to client) (we can build output to multiple clients at the same time)
#ifdef _WIN32
	WSABUF m_WSABuf;
	WSAOVERLAPPED m_overlapped;
#else
	struct ev_io m_eventWatcher;
#endif

public:
	bool xSendError(int);
	void xFlushAsync();
	void xAsyncSendComplete();
#ifndef _WIN32
	struct ev_io * GetIOCB();
	bool xCanSend();
	void xSetCanSend(bool);
#endif

public:

	CClient( SOCKET client );
	~CClient();
	void CharDisconnect();

	CClient* GetNext() const
	{
		return( STATIC_CAST <CClient*>( CGObListRec::GetNext()));
	}

	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute script type command on me
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & s, CTextConsole * pSrc );
	virtual bool r_LoadVal( CScript & s );

	// Low level message traffic.
	static int xCompress( BYTE * pOutput, const BYTE * pInput, int inplen );

	void xSendReady( const void *pData, int length, bool bNextFlush = true ); // We could send the packet now if we wanted to but wait til we have more.
	void xSend( const void *pData, int length, bool bQueue = false ); // Buffering send function
	void xProcessTooltipQueue();
	void xFlush();				// Sends buffered data at once
	void xSendPkt( const CCommand * pCmd, int length );
	void xSendPktNow( const CCommand * pCmd, int length );
	bool xProcessClientSetup( CEvent * pEvent, int iLen );
	void xProcessMsg(int fGood = 0);		// Process a packet
	bool xPacketFilter(const CEvent * pEvent, int iLen = 0);
	int xDispatchMsg();
	bool xRecvData();			// High Level Receive message from client
	bool xCanEncLogin(bool bCheckCliver = false);	// Login crypt check
	// Low level push world data to the client.

	bool addRelay( const CServerDef * pServ );
	bool addLoginErr( LOGIN_ERR_TYPE code );
#define SF_UPDATE_HITS		0x01
#define SF_UPDATE_MANA		0x02
#define SF_UPDATE_STAM		0x04
#define SF_UPDATE_STATUS	0x08

	void addUpdateStatsFlag()
	{
		m_fUpdateStats |= SF_UPDATE_STATUS;
	}
	void addUpdateHitsFlag()
	{
		m_fUpdateStats |= SF_UPDATE_HITS;
	}
	void addUpdateManaFlag()
	{
		m_fUpdateStats |= SF_UPDATE_MANA;
	}
	void addUpdateStamFlag()
	{
		m_fUpdateStats |= SF_UPDATE_STAM;
	}
	void UpdateStats();
	bool addDeleteErr( DELETE_ERR_TYPE code );
	void addExtData( EXTDATA_TYPE type, const CExtData * pData, int iSize );
	void addSeason(SEASON_TYPE season);
	void addTime( bool bCurrent = false );
	void addObjectRemoveCantSee( CGrayUID uid, LPCTSTR pszName = NULL );
	void addObjectRemove( CGrayUID uid );
	void addObjectRemove( const CObjBase * pObj );
	void addRemoveAll( bool fItems, bool fChars );

	void addItem_OnGround( CItem * pItem ); // Send items (on ground)
	void addItem_Equipped( const CItem * pItem );
	void addItem_InContainer( const CItem * pItem );
	void addItem( CItem * pItem );

	void addBuff( const WORD IconId, const DWORD ClilocOne, const DWORD ClilocTwo, const short Time, BYTE * pText = 0);
	void removeBuff (const WORD IconId);
	void resendBuffs();

	void addOpenGump( const CObjBase * pCont, GUMP_TYPE gump );
	int  addContents( const CItemContainer * pCont, bool fCorpseEquip = false, bool fCorpseFilter = false, bool fShop = false); // Send items
	bool addContainerSetup( const CItemContainer * pCont ); // Send Backpack (with items)

	void addPlayerStart( CChar * pChar );
	void addAOSPlayerSeeNoCrypt(); //sends tooltips for items in LOS
	void addPlayerSee( const CPointMap & pt ); // Send objects the player can now see
	void addPlayerView( const CPointMap & pt, bool playerStart = false );
	void addPlayerWarMode();
	void addPlayerWalkCancel();
	void addPlayerWalkCancel(const CPointMap & pt, BYTE bDir);

	void addCharMove( const CChar * pChar );
	void addCharMove( const CChar * pChar, BYTE bCharDir );
	void addChar( const CChar * pChar );
	void addCharName( const CChar * pChar ); // Singleclick text for a character
	void addItemName( const CItem * pItem );

	bool addKick( CTextConsole * pSrc, bool fBlock = true );
	void addWeather( WEATHER_TYPE weather = WEATHER_DEFAULT ); // Send new weather to player
	void addLight( int iLight = -1 );
	void addMusic( MIDI_TYPE id );
	void addArrowQuest( int x, int y );
	void addEffect( EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate * pDst, const CObjBaseTemplate * pSrc, BYTE speed = 5, BYTE loop = 1, bool explode = false, DWORD color = 0, DWORD render = 0 );
	void addSound( SOUND_TYPE id, const CObjBaseTemplate * pBase = NULL, int iRepeat = 1 );
	void addReSync(bool bForceMap = false);
	void addMap( const CPointMap * pOldP, bool playerStart = false );
	void addMapDiff();
	void addPing( BYTE bCode = 0 );
	void addChangeServer();

	void addBark( LPCTSTR pText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue = HUE_DEFAULT, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD );
	void addBarkUNICODE( const NCHAR * pText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang = 0 );
	void addBarkLocalized( int iClilocId, const CObjBaseTemplate * pSrc, HUE_TYPE wHue = HUE_DEFAULT, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD, TCHAR * pArgs = NULL );
	void addBarkParse( LPCTSTR pszText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, LPCTSTR name = "" );
	void addSysMessage( LPCTSTR pMsg ); // System message (In lower left corner)
	void addObjMessage( LPCTSTR pMsg, const CObjBaseTemplate * pSrc, HUE_TYPE wHue = HUE_TEXT_DEF ); // The message when an item is clicked

	void addDyeOption( const CObjBase * pBase );
	void addItemDragCancel( BYTE type );
	void addWebLaunch( LPCTSTR pMsg ); // Direct client to a web page

	void addPromptConsole( CLIMODE_TYPE mode, LPCTSTR pMsg );
	void addTarget( CLIMODE_TYPE targmode, LPCTSTR pMsg, bool fAllowGround = false, bool fCheckCrime = false ); // Send targetting cursor to client
	void addTargetDeed( const CItem * pDeed );
	bool addTargetItems( CLIMODE_TYPE targmode, ITEMID_TYPE id, bool fGround = true );
	bool addTargetChars( CLIMODE_TYPE mode, CREID_TYPE id, bool fNoto );
	void addTargetVerb( LPCTSTR pCmd, LPCTSTR pArg );
	void addTargetFunctionMulti( LPCTSTR pszFunction, ITEMID_TYPE itemid, bool fGround );
	void addTargetFunction( LPCTSTR pszFunction, bool fAllowGround, bool fCheckCrime );
	void addTargetCancel();
	void addPromptConsoleFunction( LPCTSTR pszFunction, LPCTSTR pszSysmessage );

	void addScrollScript( CResourceLock &s, SCROLL_TYPE type, DWORD dwcontext = 0, LPCTSTR pszHeader = NULL );
	void addScrollResource( LPCTSTR szResourceName, SCROLL_TYPE type, DWORD dwcontext = 0 );

	void addVendorClose( const CChar * pVendor );
	int  addShopItems(CChar * pVendor, LAYER_TYPE layer, bool bReal = true);
	bool addShopMenuBuy( CChar * pVendor );
	int  addShopMenuSellFind( CItemContainer * pSearch, CItemContainer * pFrom1, CItemContainer * pFrom2, int iConvertFactor, CCommand * & pCur );
	bool addShopMenuSell( CChar * pVendor );
	void addBankOpen( CChar * pChar, LAYER_TYPE layer = LAYER_BANKBOX );

	void addSpellbookOpen( CItem * pBook, WORD offset = 1 );
	void addCustomSpellbookOpen( CItem * pBook, DWORD gumpID );
	bool addBookOpen( CItem * pBook );
	void addBookPage( const CItem * pBook, int iPage );
	void addCharStatWindow( CGrayUID uid, bool fRequested = false ); // Opens the status window
	void addHitsUpdate( CGrayUID uid );
	void addManaUpdate( CGrayUID uid );
	void addStamUpdate( CGrayUID uid );
	void addSkillWindow(SKILL_TYPE skill, bool bFromInfo = false); // Opens the skills list
	void addBulletinBoard( const CItemContainer * pBoard );
	bool addBBoardMessage( const CItemContainer * pBoard, BBOARDF_TYPE flag, CGrayUID uidMsg );

	void addToolTip( const CObjBase * pObj, LPCTSTR psztext );
	void addMap( CItemMap * pItem );
	void addMapMode( CItemMap * pItem, MAPCMD_TYPE iType, bool fEdit = false );

	void addGumpTextDisp( const CObjBase * pObj, GUMP_TYPE gump, LPCTSTR pszName, LPCTSTR pszText );
	void addGumpInpVal( bool fcancel, INPVAL_STYLE style,
		DWORD dwmask, LPCTSTR ptext1, LPCTSTR ptext2, CObjBase * pObj );

	void addItemMenu( CLIMODE_TYPE mode, const CMenuItem * item, int count, CObjBase * pObj = NULL );
	void addGumpDialog( CLIMODE_TYPE mode, const CGString * sControls, int iControls, const CGString * psText, int iTexts, int x, int y, CObjBase * pObj = NULL, DWORD rid = 0 );

	bool addGumpDialogProps( CGrayUID uid );

	void addRedrawAll();
	void addChatSystemMessage(CHATMSG_TYPE iType, LPCTSTR pszName1 = NULL, LPCTSTR pszName2 = NULL, CLanguageID lang = 0 );

	bool addWalkCode( EXTDATA_TYPE iType, int iQty );

	void addCharPaperdoll( CChar * pChar );

	void addAOSTooltip( const CObjBase * pObj, bool bShop = false );

private:
	#define MAX_POPUPS 15
	#define POPUPFLAG_LOCKED 0x01
	#define POPUPFLAG_ARROW 0x02
	#define POPUPFLAG_COLOR 0x20
	#define POPUP_REQUEST 0
	#define POPUP_PAPERDOLL 11
	#define POPUP_BACKPACK 12
	#define POPUP_BANKBOX 21
	#define POPUP_BANKBALANCE 22
	#define POPUP_VENDORBUY 31
	#define POPUP_VENDORSELL 32
	#define POPUP_PETGUARD 41
	#define POPUP_PETFOLLOW 42
	#define POPUP_PETDROP 43
	#define POPUP_PETKILL 44
	#define POPUP_PETSTOP 45
	#define POPUP_PETSTAY 46
	#define POPUP_PETFRIEND 47
	#define POPUP_PETTRANSFER 48
	#define POPUP_STABLESTABLE 51
	#define POPUP_STABLERETRIEVE 52
	CExtData * m_pPopupCur;
	short int m_context_popup;
	short int m_PopupLen;

public:
	void AOSPopupMenuAdd( WORD entrytag, WORD textid, WORD flags, WORD color );
	void Event_AOSPopupMenuSelect( DWORD uid, WORD EntryTag );
	void Event_AOSPopupMenuRequest( DWORD uid );


	void addShowDamage( int damage, DWORD uid_damage );
	void addSpeedMode( int speedMode = 0 );
	void addVisualRange( BYTE visualRange = UO_MAP_VIEW_SIZE );
	void addIdleWarning( bool bSameChar = false );

	void SendPacket( TCHAR * pszPacket );

	// Test what I can do
	CAccountRef GetAccount() const
	{
		return( m_pAccount );
	}
	bool IsPriv( WORD flag ) const
	{	// PRIV_GM
		if ( GetAccount() == NULL )
			return( false );
		return( GetAccount()->IsPriv( flag ));
	}
	void SetPrivFlags( WORD wPrivFlags )
	{
		if ( GetAccount() == NULL )
			return;
		GetAccount()->SetPrivFlags( wPrivFlags );
	}
	void ClearPrivFlags( WORD wPrivFlags )
	{
		if ( GetAccount() == NULL )
			return;
		GetAccount()->ClearPrivFlags( wPrivFlags );
	}

	// ------------------------------------------------
	bool IsResDisp( BYTE flag ) const
	{
		if ( GetAccount() == NULL )
			return( false );
		return( GetAccount()->IsResDisp( flag ) );
	}
	BYTE GetResDisp()
	{
		if ( GetAccount() == NULL )
			return( -1 );
		return( GetAccount()->GetResDisp() );
	}
	bool SetResDisp( BYTE res )
	{
		if ( GetAccount() == NULL )
			return( false );
		return ( GetAccount()->SetResDisp( res ) );
	}
	bool SetGreaterResDisp( BYTE res )
	{
		if ( GetAccount() == NULL )
			return( false );
		return( GetAccount()->SetGreaterResDisp( res ) );
	}
	// ------------------------------------------------

	PLEVEL_TYPE GetPrivLevel() const
	{
		// PLEVEL_Counsel
		if ( GetAccount() == NULL )
			return( PLEVEL_Guest );
		return( GetAccount()->GetPrivLevel());
	}
	LPCTSTR GetName() const
	{
		if ( GetAccount() == NULL )
			return( "NA" );
		return( GetAccount()->GetName());
	}
	CChar * GetChar() const
	{
		return( m_pChar );
	}

	void SysMessage( LPCTSTR pMsg ) const; // System message (In lower left corner)
	bool CanSee( const CObjBaseTemplate * pObj ) const;
	bool CanHear( const CObjBaseTemplate * pSrc, TALKMODE_TYPE mode ) const;
	bool IsSkillVisible( SKILL_TYPE skill );

	bool Dialog_Setup( CLIMODE_TYPE mode, RESOURCE_ID_BASE rid, int iPage, CObjBase * pObj, LPCTSTR Arguments = "" );
	bool Dialog_Close( CObjBase * pObj, RESOURCE_ID_BASE rid, int buttonID );
	void Menu_Setup( RESOURCE_ID_BASE rid, CObjBase * pObj = NULL );

	int OnSkill_Info( SKILL_TYPE skill, CGrayUID uid, int iTestLevel, bool fTest );

	inline bool Cmd_Use_Item_MustEquip( CItem * pItem );

	bool Cmd_Use_Item( CItem * pItem, bool fTestTouch, bool fScript = false );
	void Cmd_EditItem( CObjBase * pObj, int iSelect );

	bool IsConnectTypePacket() const
	{
		// This is a game or login server.
		// m_Crypt.IsInit()
		return( m_iConnectType == CONNECT_CRYPT || m_iConnectType == CONNECT_LOGIN || m_iConnectType == CONNECT_GAME );
	}

	CONNECT_TYPE	GetConnectType() const
	{
		return m_iConnectType;
	}

	void SetConnectType( CONNECT_TYPE iType );

	// Stuff I am doing. Started by a command
	CLIMODE_TYPE GetTargMode() const
	{
		return( m_Targ_Mode );
	}
	void SetTargMode( CLIMODE_TYPE targmode = CLIMODE_NORMAL, LPCTSTR pszPrompt = NULL );
	void ClearTargMode()
	{
		// done with the last mode.
		m_Targ_Mode = CLIMODE_NORMAL;
	}

	bool IsConnecting();

	int GetClientVersion()
	{
		return m_Crypt.GetClientVer();
	}

	int GetClientVersionReported()
	{
		return m_reportedCliver;
	}

	bool IsClientKR()
	{
		return m_bClientKR;
	}

private:
	CGString	m_BarkBuffer;

public:
	char		m_zLastMessage[SCRIPT_MAX_LINE_LEN];	// last sysmessage
	char		m_zLastObjMessage[SCRIPT_MAX_LINE_LEN];	// last message
	char		m_zLogin[64];
	CServTime	m_tNextPickup;
	CVarDefMap	m_TagDefs;
	typedef std::map<DWORD, std::pair<std::pair<DWORD,DWORD>, CPointMap> > OpenedContainerMap_t;
	OpenedContainerMap_t m_openedContainers;	// list of UIDs of all opened containers by the client

	CGObArray<CClientTooltip *> m_TooltipData; // Storage for tooltip data while in trigger

	CItemMultiCustom * m_pHouseDesign; // The building this client is designing
};

////////////////////////////////////////////////////////////////////////////////////

#include "CBase.h"
#include "CObjBase.h"
#include "CWorld.h"

////////////////////////////////////////////////////////////////////////////////////

enum PROFILE_TYPE
{
	PROFILE_IDLE,		// Wait for stuff.
	PROFILE_OVERHEAD,	// In between stuff.
	PROFILE_NETWORK_RX,	// Just get client info and monitor new client requests. No processing.
	PROFILE_CLIENTS,	// Client processing.
	PROFILE_NETWORK_TX,
	PROFILE_CHARS,
	PROFILE_ITEMS,
	PROFILE_MAP,
	PROFILE_NPC_AI,
	PROFILE_SCRIPTS,
	PROFILE_TIME_QTY,

	// Qty of bytes. Not Time.
	PROFILE_DATA_TX = PROFILE_TIME_QTY,
	PROFILE_DATA_RX,

	PROFILE_QTY,
};


class CProfileData
{
protected:
	struct CProfileDataRec
	{
		LONGLONG	m_Time;		// accumulated time in msec.
		int			m_iCount;	// how many passes made into this.
	};

	CProfileDataRec	m_AvgTimes[PROFILE_QTY];
	CProfileDataRec m_PrvTimes[PROFILE_QTY];
	CProfileDataRec m_CurTimes[PROFILE_QTY];

	int m_iActiveWindowSec;		// The sample window size in seconds. 0=off
	int	m_iAvgCount;

	LONGLONG m_TimeTotal;	// Average this over a total time period.

	// Store the last time start time.
	PROFILE_TYPE  m_CurTask;	// What task are we currently processing ?
	LONGLONG m_CurTime;		// QueryPerformanceCount()

public:
	static const char *m_sClassName;
	bool IsActive() const { return( m_iActiveWindowSec ? true : false ); }
	void SetActive( int iSampleSec );
	int GetActiveWindow() const { return m_iActiveWindowSec; }

	PROFILE_TYPE	GetCurrentTask();
	void Count( PROFILE_TYPE id, DWORD dwVal )
	{
		ASSERT( id >= PROFILE_TIME_QTY && id < PROFILE_QTY );
		m_CurTimes[id].m_Time += dwVal;
		m_CurTimes[id].m_iCount ++;
	}
	CProfileData()
	{
		SetActive(10);	// default to 10 sec window.
	}

	void Start( PROFILE_TYPE id );

	LPCTSTR GetName( PROFILE_TYPE id ) const;
	LPCTSTR GetDesc( PROFILE_TYPE id ) const;
};

enum SERVMODE_TYPE
{
	SERVMODE_RestockAll,	// Major event.
	SERVMODE_Saving,		// Forced save freezes the system.
	SERVMODE_Run,			// Game is up and running
	SERVMODE_ResyncPause,	// paused during resync

	SERVMODE_Loading,		// Initial load.
	SERVMODE_ResyncLoad,	// Loading after resync
	SERVMODE_Exiting,		// Closing down
};

extern class CServer : public CServerDef, public CTextConsole
{
	static LPCTSTR const sm_szVerbKeys[];

public:
	static const char *m_sClassName;
	SERVMODE_TYPE m_iModeCode;  // Just some error code to return to system.
	int  m_iExitFlag;	// identifies who caused the exit. <0 = error
	bool m_fResyncPause;		// Server is temporarily halted so files can be updated.
	CTextConsole * m_fResyncRequested;		// A resync pause has been requested by this source.

	CGSocket m_SocketMain;	// This is the incoming monitor socket.(might be multiple ports?)
	CGSocket m_SocketGod;	// This is for god clients.

	// admin console.
	int m_iAdminClients;		// how many of my clients are admin consoles ?
	CGString m_sConsoleText;
	bool m_fConsoleTextReadyFlag;	// interlocking flag for moving between tasks.

	CServTime m_timeShutdown;	// When to perform the shutdowm (g_World.clock)

	CGObList m_Clients;		// Current list of clients (CClient)

	CProfileData m_Profile;	// the current active statistical profile.
	CChat m_Chats;	// keep all the active chats

	char	m_PacketFilter[255][32];	// list of packet filtering functions

	CFileObj	fhFile;			//	file script object
#ifdef _NEW_FILE_COLLECTION
	CFileObjContainer fcFileContainer;
#endif
	CDataBase	m_hdb;			//	SQL data base

private:
	void ProfileDump( CTextConsole * pSrc, bool bDump = false );

public:
	CServer();
	~CServer();

	bool IsValidBusy() const;
	void SetServerMode( SERVMODE_TYPE mode );

	void SetExitFlag( int iFlag );
	void Shutdown( int iMinutes );
	bool IsLoading() const
	{
		return( m_iModeCode > SERVMODE_Run || m_fResyncPause );
	}
	void SetSignals( bool fMsg = true );

	bool SocketsInit(); // Initialize sockets
	bool SocketsInit( CGSocket & socket );
	void SocketsReceive();
	CClient * SocketsReceive( CGSocket & socket );
	void SocketsFlush();
	void SocketsClose();

	bool Load();

	void SysMessage( LPCTSTR pMsg ) const;
	void PrintTelnet( LPCTSTR pszMsg ) const;
	void PrintStr( LPCTSTR pMsg ) const;
	int  PrintPercent( long iCount, long iTotal );

	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc = NULL );
	virtual bool r_LoadVal( CScript & s );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc );

	LPCTSTR GetStatusString( BYTE iIndex = 0 ) const;
	int GetAgeHours() const;

	bool OnConsoleCmd( CGString & sText, CTextConsole * pSrc );

	void OnTick();

public:
	void ListClients( CTextConsole * pClient ) const;

	CClient * GetClientHead() const
	{
		return( STATIC_CAST <CClient*>( m_Clients.GetHead()));
	}

	void SetResyncPause(bool fPause, CTextConsole * pSrc, bool bMessage = false);
	bool CommandLine( int argc, TCHAR * argv[] );

	LPCTSTR GetName() const { return( CServerDef::GetName()); }
	PLEVEL_TYPE GetPrivLevel() const;
} g_Serv;	// current state stuff not saved.

class Main : public AbstractThread
{
public:
	Main();

	// we increase the access level from protected to public in order to allow manual execution when
	// configuration disables using threads
	// TODO: in the future, such simulated functionality should lie in AbstractThread inself instead of hacks
	virtual void tick();

protected:
	virtual void onStart();
};

//////////////////////////////////////////////////////////////

extern LPCTSTR g_szServerDescription;
extern LPCTSTR const g_Stat_Name[STAT_QTY];
extern	CGStringList	g_AutoComplete;

extern int Sphere_InitServer( int argc, char *argv[] );
extern int Sphere_OnTick();
extern void Sphere_ExitServer();
extern int Sphere_MainEntryPoint( int argc, char *argv[] );

///////////////////////////////////////////////////////////////
// -CGrayUID

inline int CObjBase::GetTimerDiff() const
{
	// How long till this will expire ?
	return( g_World.GetTimeDiff( m_timeout ));
}
inline CObjBase * CGrayUIDBase::ObjFind() const
{
	if ( IsResource())
		return( NULL );
	return( g_World.FindUID( m_dwInternalVal & UID_O_INDEX_MASK ));
}
inline CItem * CGrayUIDBase::ItemFind() const
{
	// IsItem() may be faster ?
	return( dynamic_cast <CItem *>( ObjFind()));
}
inline CChar * CGrayUIDBase::CharFind() const
{
	return( dynamic_cast <CChar *>( ObjFind()));
}

// ---------------------------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// Buff Icons

inline int GetStatPercentage( const CChar* Char, STAT_TYPE Stat, const int Difference )
{
	short Percentage = ((Difference*100) / (Char->Stat_GetBase(Stat) == 0 ? 1 : Char->Stat_GetBase(Stat)));
	if (Percentage < 0)
		Percentage = -(Percentage);
	if (Percentage > 0x3E7)
		Percentage = 0x3E7;
	if ( Percentage == 0 )
		Percentage = 1;
	return Percentage;
}
enum BUFF_ICONS {
	BI_NODISMOUNT = 0x3E9,
	BI_NOREARM = 0x3EA,
	BI_NIGHTSIGHT = 0x3ED,
	BI_DEATHSTRIKE,
	BI_EVILOMEN,
	BI_HEALINGTHROTTLE,
	BI_STAMINATHROTTLE,
	BI_DIVINEFURY,
	BI_ENEMYOFONE,
	BI_HIDDEN,
	BI_ACTIVEMEDITATION,
	BI_BLOODOATHCASTER,
	BI_BLOODOATHCURSE,
	BI_CORPSESKIN,
	BI_MINDROT,
	BI_PAINSPIKE,
	BI_STRANGLE,
	BI_GIFTOFRENEWAL,
	BI_ATTUNEWEAPON,
	BI_THUNDERSTORM,
	BI_ESSENCEOFWIND,
	BI_ETHEREALVOYAGE,
	BI_GIFTOFLIFE,
	BI_ARCANEEMPOWERMENT,
	BI_MORTALSTRIKE,
	BI_REACTIVEARMOR,
	BI_PROTECTION,
	BI_ARCHPROTECTION,
	BI_MAGICREFLECTION,
	BI_INCOGNITO,
	BI_DISGUISED,
	BI_ANIMALFORM,
	BI_POLYMORPH,
	BI_INVISIBILITY,
	BI_PARALYZE,
	BI_POISON,
	BI_BLEED,
	BI_CLUMSY,
	BI_FEEBLEMIND,
	BI_WEAKEN,
	BI_CURSE,
	BI_MASSCURSE,
	BI_AGILITY,
	BI_CUNNING,
	BI_STRENGTH,
	BI_BLESS
};
// ---------------------------------------------------------------------------------------------

struct TScriptProfiler
{
	unsigned char	initstate;
	DWORD		called;
	LONGLONG	total;
	struct TScriptProfilerFunction
	{
		char		name[128];	// name of the function
		DWORD		called;		// how many times called
		LONGLONG	total;		// total executions time
		DWORD		min;		// minimal executions time
		DWORD		max;		// maximal executions time
		DWORD		average;	// average executions time
		TScriptProfilerFunction *next;
	}		*FunctionsHead, *FunctionsTail;
	struct TScriptProfilerTrigger
	{
		char		name[128];	// name of the trigger
		DWORD		called;		// how many times called
		LONGLONG	total;		// total executions time
		DWORD		min;		// minimal executions time
		DWORD		max;		// maximal executions time
		DWORD		average;	// average executions time
		TScriptProfilerTrigger *next;
	}		*TriggersHead, *TriggersTail;
};
extern TScriptProfiler g_profiler;

//	Time measurement macros
extern LONGLONG llTimeProfileFrequency;

#ifdef _WIN32

#define	TIME_PROFILE_INIT	\
	LONGLONG llTicks, llTicksEnd
#define	TIME_PROFILE_START	\
	if ( !QueryPerformanceCounter((LARGE_INTEGER *)&llTicks)) llTicks = GetTickCount()
#define TIME_PROFILE_END	if ( !QueryPerformanceCounter((LARGE_INTEGER *)&llTicksEnd)) llTicksEnd = GetTickCount()

#else

#define	TIME_PROFILE_INIT	\
	LONGLONG llTicks, llTicksEnd
#define	TIME_PROFILE_START	\
	llTicks = GetTickCount()
#define TIME_PROFILE_END	llTicksEnd = GetTickCount();

#endif

#define TIME_PROFILE_GET_HI	((llTicksEnd - llTicks)/(llTimeProfileFrequency/1000))
#define	TIME_PROFILE_GET_LO	((((llTicksEnd - llTicks)*10000)/(llTimeProfileFrequency/1000))%10000)

#endif
