#ifndef _INC_GRAYSVR_H_
#define _INC_GRAYSVR_H_
#pragma once

//	Enable advanced exceptions catching. Consumes some more resources, but is very useful
//	for debug on a running environment. Also it makes sphere more stable since exceptions
//	are local
#ifndef _DEBUG
	#ifndef EXCEPTIONS_DEBUG
	#define EXCEPTIONS_DEBUG
	#endif
#endif

#if defined(_WIN32) && !defined(_MTNETWORK)
	// _MTNETWORK enabled via makefile for other systems
	#define _MTNETWORK
#endif

//#define DEBUGWALKSTUFF 1
//#ifdef _DEBUG
#ifdef DEBUGWALKSTUFF
	#define WARNWALK(_x_)		g_pLog->EventWarn _x_;
#else
	#define WARNWALK(_x_)		if ( g_Cfg.m_wDebugFlags & DEBUGF_WALK ) { g_pLog->EventWarn _x_; }
#endif

#include "../common/graycom.h"


class CServTime
{
#undef GetCurrentTime
#define TICK_PER_SEC 10
#define TENTHS_PER_SEC 1
	// A time stamp in the server/game world.
public:
	static const char *m_sClassName;
	INT64 m_lPrivateTime;
public:
	INT64 GetTimeRaw() const
	{
		if ( m_lPrivateTime < 0 )
			return 0;

		return m_lPrivateTime;
	}
	INT64 GetTimeDiff( const CServTime & time ) const
	{
		return( m_lPrivateTime - time.m_lPrivateTime );
	}
	void Init()
	{
		m_lPrivateTime = 0;
	}
	void InitTime( INT64 lTimeBase )
	{
		if ( lTimeBase < 0 )
			lTimeBase = 0;

		m_lPrivateTime = lTimeBase;
	}
	bool IsTimeValid() const
	{
		return( m_lPrivateTime > 0 ? true : false );
	}
	CServTime operator+( INT64 iTimeDiff ) const
	{
		CServTime time;
		time.m_lPrivateTime = m_lPrivateTime + iTimeDiff;
		if ( time.m_lPrivateTime < 0 )
			time.m_lPrivateTime = 0;

		return( time );
	}
	CServTime operator-( INT64 iTimeDiff ) const
	{
		CServTime time;
		time.m_lPrivateTime = m_lPrivateTime - iTimeDiff;
		if ( time.m_lPrivateTime < 0 )
			time.m_lPrivateTime = 0;

		return( time );
	}
	INT64 operator-( CServTime time ) const
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
	RDS_SA,
	RDS_HS,
	RDS_TOL,
	RDS_QTY
};

//#include "../common/graycom.h"
#include "../common/graymul.h"
#include "../common/grayproto.h"
#include "../common/CGrayInst.h"
#include "../common/CResourceBase.h"
#include "../common/CRegion.h"
#include "../common/CGrayMap.h"
#include "../sphere/mutex.h"
#include "../sphere/ProfileData.h"
#include "../sphere/threads.h"
#if !defined(_WIN32) || defined(_LIBEV)
	#include "../sphere/linuxev.h"
#endif
#include "../common/CQueue.h"
#include "../common/CSectorTemplate.h"
#include "../common/CDataBase.h"
#include "../common/sqlite/SQLite.h" //New Database

#include "CResource.h"
#include "CServRef.h"
#include "CAccount.h"

#include <vector>
#include <string>

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

//	Triggers list
enum E_TRIGGERS
{
	#define ADD(a) TRIGGER_##a,
	#include "../tables/triggers.tbl"
	TRIGGER_QTY,
};

extern bool IsTrigUsed(E_TRIGGERS id);
extern bool IsTrigUsed(const char *name);
extern void TriglistInit();
extern void TriglistClear();
extern void TriglistAdd(E_TRIGGERS id);
extern void TriglistAdd(const char *name);
extern void Triglist(long &total, long &used);
extern void TriglistPrint();


// Text mashers.

extern DIR_TYPE GetDirStr( LPCTSTR pszDir );
extern LPCTSTR GetTimeMinDesc( int dwMinutes );
extern size_t FindStrWord( LPCTSTR pTextSearch, LPCTSTR pszKeyWord );

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
	SimpleMutex m_mutex;

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
		m_dwMsgMask = GetLogLevel() | ( dwMask &~ 0x0f );
	}
	bool IsLoggedMask( DWORD dwMask ) const
	{
		return( ((dwMask &~ (0x0f | LOGM_NOCONTEXT | LOGM_DEBUG)) == 0) ||
				(( GetLogMask() & ( dwMask &~ 0x0f )) != 0) );
	}
	LOGL_TYPE GetLogLevel() const
	{
		return static_cast<LOGL_TYPE>(m_dwMsgMask & 0x0f);
	}
	void SetLogLevel( LOGL_TYPE level )
	{
		m_dwMsgMask = GetLogMask() | ( level & 0x0f );
	}
	bool IsLoggedLevel( LOGL_TYPE level ) const
	{
		return ( ((level & 0x0f) != 0) &&
				 (GetLogLevel() >= ( level & 0x0f ) ) );
	}
	bool IsLogged( DWORD wMask ) const
	{
		return IsLoggedMask(wMask) || IsLoggedLevel(static_cast<LOGL_TYPE>(wMask));
	}

	virtual int EventStr( DWORD wMask, LPCTSTR pszMsg );
	void _cdecl CatchEvent( const CGrayError * pErr, LPCTSTR pszCatchContext, ...  ) __printfargs(3,4);

public:
	CLog()
	{
		m_fLockOpen = false;
		m_pScriptContext = NULL;
		m_pObjectContext = NULL;
		m_dwMsgMask = LOGL_ERROR |
			LOGM_INIT | LOGM_CLIENTS_LOG | LOGM_GM_PAGE;
		SetFilePath( GRAY_FILE "log.log" );	// default name to go to.
	}

private:
	CLog(const CLog& copy);
	CLog& operator=(const CLog& other);

	enum Color
	{
		DEFAULT,
		YELLOW,
		RED,
		CYAN
	};

	/**
	 * Changes current console color to the specified one. Note, that the color should be reset after being set
	 */
	void SetColor(Color color);
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

private:
	CGMPage(const CGMPage& copy);
	CGMPage& operator=(const CGMPage& other);

public:
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
	void ClearGMHandler();
	void SetGMHandler( CClient * pClient );
	INT64 GetAge() const;

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

	size_t FindIgnoringIndex( LPCTSTR pszName) const;

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

private:
	CChatChanMember(const CChatChanMember& copy);
	CChatChanMember& operator=(const CChatChanMember& other);

public:
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
		return( FindIgnoringIndex( pszName ) != m_IgnoredMembers.BadIndex() );
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
	size_t FindMemberIndex( LPCTSTR pszName ) const;

public:
	explicit CChatChannel(LPCTSTR pszName, LPCTSTR pszPassword = NULL)
	{
		m_sName = pszName;
		m_sPassword = pszPassword;
		m_fVoiceDefault = true;
	};

private:
	CChatChannel(const CChatChannel& copy);
	CChatChannel& operator=(const CChatChannel& other);

public:
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
		size_t i = FindMemberIndex( pszName );
		if ( i == m_Members.BadIndex() )
			return NULL;
		return m_Members[i];
	}
	bool RemoveMember(LPCTSTR pszName)
	{
		CChatChanMember * pMember = FindMember(pszName);
		if ( pMember == NULL )
			return false;
		RemoveMember(pMember);
		return true;
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

private:
	CChat(const CChat& copy);
	CChat& operator=(const CChat& other);

public:
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
	static void GenerateChatName(CGString & sName, const CClient * pClient);
};

class CDialogResponseArgs : public CScriptTriggerArgs
{
	// The scriptable return from a gump dialog.
	// "ARG" = dialog args script block. ex. ARGTXT(id), ARGCHK(i)
public:
	static const char *m_sClassName;
	struct TResponseString
	{
	public:
		const WORD m_ID;
		CGString const m_sText;

		TResponseString(WORD id, LPCTSTR pszText) : m_ID(id), m_sText(pszText)
		{
		}

	private:
		TResponseString(const TResponseString& copy);
		TResponseString& operator=(const TResponseString& other);
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

public:
	CDialogResponseArgs() { };

private:
	CDialogResponseArgs(const CDialogResponseArgs& copy);
	CDialogResponseArgs& operator=(const CDialogResponseArgs& other);
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
		m_Light = UCHAR_MAX;	// set based on time later.
		m_Season = SEASON_QTY;
		m_Weather = WEATHER_DEFAULT;
	}
};

enum CLIMODE_TYPE	// What mode is the client to server connection in ? (waiting for input ?)
{
	// Setup events ------------------------------------------------------------------
	CLIMODE_SETUP_CONNECTING,
	CLIMODE_SETUP_SERVERS,			// client has received the servers list
	CLIMODE_SETUP_RELAY,			// client has been relayed to the game server. wait for new login
	CLIMODE_SETUP_CHARLIST,			// client has the char list and may (select char, delete char, create new char)

	// Capture the user input for this mode  -----------------------------------------
	CLIMODE_NORMAL,					// No targeting going on, we are just walking around, etc

	// Asyc events enum here  --------------------------------------------------------
	CLIMODE_DRAG,					// I'm dragging something (not quite a targeting but similar)
	CLIMODE_DYE,					// the dye dialog is up and I'm targeting something to dye
	CLIMODE_INPVAL,					// special text input dialog (for setting item attrib)

	// Some sort of general gump dialog ----------------------------------------------
	CLIMODE_DIALOG,					// from RES_DIALOG

	// Hard-coded (internal) dialogs
	CLIMODE_DIALOG_VIRTUE = 0x1CD,

	// Making a selection from a menu  -----------------------------------------------
	CLIMODE_MENU,					// from RES_MENU

	// Hard-coded (internal) menus
	CLIMODE_MENU_SKILL,				// result of some skill (tracking, tinkering, blacksmith, etc)
	CLIMODE_MENU_SKILL_TRACK_SETUP,
	CLIMODE_MENU_SKILL_TRACK,
	CLIMODE_MENU_GM_PAGES,			// open gm pages list
	CLIMODE_MENU_EDIT,				// edit the contents of a container

	// Prompting for text input ------------------------------------------------------
	CLIMODE_PROMPT_NAME_RUNE,
	CLIMODE_PROMPT_NAME_KEY,		// naming a key
	CLIMODE_PROMPT_NAME_SIGN,		// naming a house sign
	CLIMODE_PROMPT_NAME_SHIP,
	CLIMODE_PROMPT_GM_PAGE_TEXT,	// allowed to enter text for GM page
	CLIMODE_PROMPT_VENDOR_PRICE,	// what would you like the price to be?
	CLIMODE_PROMPT_TARG_VERB,		// send message to another player
	CLIMODE_PROMPT_SCRIPT_VERB,		// script verb
	CLIMODE_PROMPT_STONE_NAME,		// prompt for text

	// Targeting mouse cursor  -------------------------------------------------------
	CLIMODE_MOUSE_TYPE,				// greater than this = mouse type targeting

	// GM targeting command stuff
	CLIMODE_TARG_OBJ_SET,			// set some attribute of the item I will show
	CLIMODE_TARG_OBJ_INFO,			// what item do I want props for?
	CLIMODE_TARG_OBJ_FUNC,

	CLIMODE_TARG_UNEXTRACT,			// break out multi items
	CLIMODE_TARG_ADDCHAR,			// "ADDNPC" command
	CLIMODE_TARG_ADDITEM,			// "ADDITEM" command
	CLIMODE_TARG_LINK,				// "LINK" command
	CLIMODE_TARG_TILE,				// "TILE" command

	// Normal user stuff  (mouse targeting)
	CLIMODE_TARG_SKILL,				// targeting a skill or spell
	CLIMODE_TARG_SKILL_MAGERY,
	CLIMODE_TARG_SKILL_HERD_DEST,
	CLIMODE_TARG_SKILL_POISON,
	CLIMODE_TARG_SKILL_PROVOKE,

	CLIMODE_TARG_USE_ITEM,			// target for using the selected item
	CLIMODE_TARG_PET_CMD,			// targeted pet command
	CLIMODE_TARG_PET_STABLE,		// pick a creature to stable
	CLIMODE_TARG_REPAIR,			// attempt to repair an item
	CLIMODE_TARG_STONE_RECRUIT,		// recruit members for a stone (mouse select)
	CLIMODE_TARG_STONE_RECRUITFULL,	// recruit/make a member and set abbrev show
	CLIMODE_TARG_PARTY_ADD,

	CLIMODE_TARG_QTY
};

//////////////////////////////////////////////////////////////////////////
// Buff Icons

enum BUFF_ICONS
{
	BI_START,
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
	BI_BLESS,
	BI_SLEEP,
	BI_STONEFORM,
	BI_SPELLPLAGUE,
	BI_GARGOYLEBERSERK,
	BI_GARGOYLEFLY = 0x41E,
	// All icons below were added only on client patch 7.0.29.0
	BI_INSPIRE,
	BI_INVIGORATE,
	BI_RESILIENCE,
	BI_PERSEVERANCE,
	BI_TRIBULATIONDEBUFF,
	BI_DESPAIR,
	// BI_ARCANEEMPOWERMENT //already added 1026
	BI_FISHBUFF = 1062,
	BI_HITLOWERATTACK,
	BI_HITLOWERDEFENSE,
	BI_HITDUALWIELD,
	BI_BLOCK,
	BI_DEFENSEMASTERY,
	BI_DESPAIRDEBUFF,
	BI_HEALINGEFFECT,
	BI_SPELLFOCUSING,
	BI_SPELLFOCUSINGDEBUFF,
	BI_RAGEFOCUSINGDEBUFF,
	BI_RAGEFOCUSING,
	BI_WARDING,
	BI_TRIBULATION,
	BI_FORCEARROW,
	BI_DISARM,
	BI_SURGE,
	BI_FEIT,
	BI_TALONSTRIKE,
	BI_OHYSICATTACK,	//STRACTICS SAY PHYCHICATTACK?
	BI_CONSECRATE,
	BI_GRAPESOFWRATH,
	BI_ENEMYOFONEDEBUFF,
	BI_HORRIFICBEAST,
	BI_LICHFORM,
	BI_VAMPIRICEMBRACE,
	BI_CURSEWEAPON,
	BI_REAPERFORM,
	BI_INMOLATINGWEAPON,
	BI_ENCHANT,
	BI_HONORABLEEXECUTION,
	BI_CONFIDENCE,
	BI_EVASION,
	BI_COUNTERATTACK,
	BI_LIGHTNINGSTRIKE,
	BI_MOMENTUMSTRIKE,
	BI_ORANGEPETALS,
	BI_ROTPETALS,
	BI_POISONIMMUNITY,
	BI_VETERINARY,
	BI_PERFECTION,
	BI_HONORED,
	BI_MANAPHASE,
	BI_FANDANCERFANFIRE,
	BI_RAGE,
	BI_WEBBING,
	BI_MEDUSASTONE,
	BI_DRAGONSLASHERFEAR,
	BI_AURAOFNAUSEA,
	BI_HOWLOFCACOPHONY,
	BI_GAZEDESPAIR,
	BI_HIRYUPHYSICALRESISTANCE,
	BI_RUNEBEETLECORRUPTION,
	BI_BLOODWORMANEMIA,
	BI_ROTWORMBLOODDISEASE,
	BI_SKILLUSEDELAY,
	BI_FACTIONSTATLOSS,
	BI_HEATOFBATTLE,
	BI_CRIMINALSTATUS,
	BI_ARMORPIERCE,
	BI_SPLINTERINGEFFECT,
	BI_SWINGSPEEDSWING,
	BI_WRAITHFORM,
	//BI_HONORABLEEXECUTION	// already added on 1092
	BI_CITYTRADEDEAL = 1126,
	BI_QTY = 1126
};
// ---------------------------------------------------------------------------------------------

// Storage for Tooltip data while in trigger on an item
class CClientTooltip
{
public:
	static const char *m_sClassName;
	DWORD m_clilocid;
	TCHAR m_args[SCRIPT_MAX_LINE_LEN];

public:
	explicit CClientTooltip(DWORD clilocid, LPCTSTR args = NULL)
	{
		m_clilocid = clilocid;
		if ( args )
			strcpylen(m_args, args, SCRIPT_MAX_LINE_LEN);
		else
			m_args[0] = '\0';
	}

private:
	CClientTooltip(const CClientTooltip& copy);
	CClientTooltip& operator=(const CClientTooltip& other);

public:
	void __cdecl FormatArgs(LPCTSTR format, ...) __printfargs(2,3)
	{
		va_list vargs;
		va_start( vargs, format );

		if ( ! _vsnprintf( m_args, SCRIPT_MAX_LINE_LEN, format, vargs ) )
			strcpylen( m_args, format, SCRIPT_MAX_LINE_LEN );

		va_end( vargs );
	}
};

class NetState;
class Packet;
class PacketServerRelay;
struct VendorItem;
class PacketDisplayPopup;
#ifndef _MTNETWORK
class NetworkIn;
#else
class NetworkManager;
class NetworkThread;
class NetworkInput;
class NetworkOutput;
#endif

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
	NetState* m_net; // network state

	// Client last know state stuff.
	CSectorEnviron m_Env;		// Last Environment Info Sent. so i don't have to keep resending if it's the same.

	unsigned char m_fUpdateStats;	// update our own status (weight change) when done with the cycle.

	// Walk limiting code
	int	m_iWalkTimeAvg;
	int m_iWalkStepCount;		// Count the actual steps . Turning does not count.
	LONGLONG m_timeWalkStep;	// the last %8 walk step time.

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

	// Promptconsole
	CLIMODE_TYPE m_Prompt_Mode;	// type of prompt
	CGrayUID m_Prompt_Uid;		// context uid
	CGString m_Prompt_Text;		// text (i.e. callback function)

public:
	CONNECT_TYPE	m_iConnectType;	// what sort of a connection is this ?
	CAccount * m_pAccount;		// The account name. we logged in on

	CServTime m_timeLogin;			// World clock of login time. "LASTCONNECTTIME"
	CServTime m_timeLastEvent;		// Last time we got event from client.
	CServTime m_timeLastEventWalk;	// Last time we got a walk event from client (only used to handle STATF_Fly char flag)
	INT64 m_timeNextEventWalk;		// Fastwalk prevention: only allow more walk requests after this timer

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
	CServTime m_Targ_Timeout;	// timeout time for targeting

	// Context of the targetting setup. depends on CLIMODE_TYPE m_Targ_Mode
	union
	{
		// CLIMODE_SETUP_CONNECTING
		struct
		{
			DWORD m_dwIP;
			int m_iConnect;	// used for debug only.
			bool m_bNewSeed;
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

		// CLIMODE_TARG_ADDCHAR
		// CLIMODE_TARG_ADDITEM
		struct
		{
			DWORD m_junk0;
			int m_id;
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
		} m_tmSkillMagery;

		// CLIMODE_TARG_USE_ITEM
		struct
		{
			CGObList * m_pParent;	// the parent of the item being targetted .
		} m_tmUseItem;
	};

private:
	// encrypt/decrypt stuff.
	CCrypt m_Crypt;			// Client source communications are always encrypted.
	static CHuffman m_Comp;

private:
	bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );

	bool OnRxConsoleLoginComplete();
	bool OnRxConsole( const BYTE * pData, size_t len );
	bool OnRxAxis( const BYTE * pData, size_t len );
	bool OnRxPing( const BYTE * pData, size_t len );
	bool OnRxWebPageRequest( BYTE * pRequest, size_t len );

	BYTE LogIn( CAccountRef pAccount, CGString & sMsg );
	BYTE LogIn( LPCTSTR pszName, LPCTSTR pPassword, CGString & sMsg );

	bool CanInstantLogOut() const;
	void Cmd_GM_PageClear();

	void Announce( bool fArrive ) const;

	// GM stuff.
	bool OnTarg_Obj_Set( CObjBase * pObj );
	bool OnTarg_Obj_Info( CObjBase * pObj, const CPointMap & pt, ITEMID_TYPE id );
	bool OnTarg_Obj_Function( CObjBase * pObj, const CPointMap & pt, ITEMID_TYPE id );

	bool OnTarg_UnExtract( CObjBase * pObj, const CPointMap & pt );
	bool OnTarg_Stone_Recruit( CChar * pChar, bool bFull = false );
	bool OnTarg_Char_Add( CObjBase * pObj, const CPointMap & pt );
	bool OnTarg_Item_Add( CObjBase * pObj, CPointMap & pt );
	bool OnTarg_Item_Link( CObjBase * pObj );
	bool OnTarg_Tile( CObjBase * pObj, const CPointMap & pt );

	// Normal user stuff.
	bool OnTarg_Use_Deed( CItem * pDeed, CPointMap & pt );
	bool OnTarg_Use_Item( CObjBase * pObj, CPointMap & pt, ITEMID_TYPE id );
	bool OnTarg_Party_Add( CChar * pChar );
	CItem *OnTarg_Use_Multi( const CItemBase * pItemDef, CPointMap & pt, DWORD dwAttr, HUE_TYPE wHue );

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
	void Event_Skill_Use( SKILL_TYPE x ); // Skill is clicked on the skill list
	void Event_Talk_Common(TCHAR * szText ); // PC speech
	bool Event_Command( LPCTSTR pszCommand, TALKMODE_TYPE mode = TALKMODE_SYSTEM ); // Client entered a '/' command like /ADD

public:
	void GetAdjustedCharID( const CChar * pChar, CREID_TYPE & id, HUE_TYPE &wHue ) const;
	void GetAdjustedItemID( const CChar * pChar, const CItem * pItem, ITEMID_TYPE & id, HUE_TYPE &wHue ) const;

	void Event_Attack(CGrayUID uid);
	void Event_Book_Title( CGrayUID uid, LPCTSTR pszTitle, LPCTSTR pszAuthor );
	void Event_BugReport( const TCHAR * pszText, int len, BUGREPORT_TYPE type, CLanguageID lang = 0 );
	void Event_ChatButton(const NCHAR * pszName); // Client's chat button was pressed
	void Event_ChatText( const NCHAR * pszText, int len, CLanguageID lang = 0 ); // Text from a client
	void Event_CombatMode( bool fWar ); // Only for switching to combat mode
	bool Event_DoubleClick( CGrayUID uid, bool fMacro, bool fTestTouch, bool fScript = false );
	void Event_ExtCmd( EXTCMD_TYPE type, TCHAR * pszName );
	void Event_Item_Drop( CGrayUID uidItem, CPointMap pt, CGrayUID uidOn, unsigned char gridIndex = 0 ); // Item is dropped on ground
	void Event_Item_Drop_Fail( CItem *pItem );
	void Event_Item_Dye( CGrayUID uid, HUE_TYPE wHue );	// Rehue an item
	void Event_Item_Pickup( CGrayUID uid, int amount ); // Client grabs an item
	void Event_MailMsg( CGrayUID uid1, CGrayUID uid2 );
	void Event_Profile( BYTE fWriteMode, CGrayUID uid, LPCTSTR pszProfile, int iProfileLen );
	void Event_PromptResp( LPCTSTR pszText, size_t len, DWORD context1, DWORD context2, DWORD type, bool bNoStrip = false );
	void Event_SetName( CGrayUID uid, const char * pszCharName );
	void Event_SingleClick( CGrayUID uid );
	void Event_Talk( LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, bool bNoStrip = false ); // PC speech
	void Event_TalkUNICODE( NWORD* wszText, int iTextLen, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, LPCTSTR pszLang );
	void Event_Target( DWORD context, CGrayUID uid, CPointMap pt, BYTE flags = 0, ITEMID_TYPE id = ITEMID_NOTHING );
	void Event_Tips( WORD i ); // Tip of the day window
	void Event_ToolTip( CGrayUID uid );
	void Event_UseToolbar(BYTE bType, DWORD dwArg);
	void Event_VendorBuy(CChar* pVendor, const VendorItem* items, size_t itemCount);
	void Event_VendorBuy_Cheater( int iCode = 0 );
	void Event_VendorSell(CChar* pVendor, const VendorItem* items, size_t itemCount);
	void Event_VendorSell_Cheater( int iCode = 0 );
	bool Event_Walk( BYTE rawdir, BYTE sequence = 0 ); // Player moves
	bool Event_CheckWalkBuffer();
	
	TRIGRET_TYPE Menu_OnSelect( RESOURCE_ID_BASE rid, int iSelect, CObjBase * pObj );
	TRIGRET_TYPE Dialog_OnButton( RESOURCE_ID_BASE rid, DWORD dwButtonID, CObjBase * pObj, CDialogResponseArgs * pArgs );

	bool Login_Relay( unsigned int iServer ); // Relay player to a certain IP
	BYTE Login_ServerList( const char * pszAccount, const char * pszPassword ); // Initial login (Login on "loginserver", new format)

	BYTE Setup_Delete( DWORD iSlot ); // Deletion of character
	int Setup_FillCharList(Packet* pPacket, const CChar * pCharFirst); // Write character list to packet
	BYTE Setup_ListReq( const char * pszAccount, const char * pszPassword, bool fTest ); // Gameserver login and character listing
	BYTE Setup_Play( unsigned int iSlot ); // After hitting "Play Character" button
	BYTE Setup_Start( CChar * pChar ); // Send character startup stuff to player
	

	// translated commands.
private:
	void Cmd_GM_PageInfo();
	int Cmd_Extract( CScript * pScript, CRectMap &rect, int & zlowest );
	size_t Cmd_Skill_Menu_Build( RESOURCE_ID_BASE rid, int iSelect, CMenuItem* item, size_t iMaxSize, bool &fShowMenu, bool &fLimitReached );
public:
	bool Cmd_CreateItem( ITEMID_TYPE id );
	bool Cmd_CreateChar( CREID_TYPE id );

	void Cmd_GM_PageMenu( unsigned int iEntryStart = 0 );
	void Cmd_GM_PageCmd( LPCTSTR pCmd );
	void Cmd_GM_PageSelect( size_t iSelect );
	void Cmd_GM_Page( LPCTSTR pszreason); // Help button (Calls GM Call Menus up)

	bool Cmd_Skill_Menu( RESOURCE_ID_BASE rid, int iSelect = -1 );
	bool Cmd_Skill_Smith( CItem * pIngots );
	bool Cmd_Skill_Magery( SPELL_TYPE iSpell, CObjBase * pSrc );
	bool Cmd_Skill_Tracking( unsigned int track_type = UINT_MAX, bool fExec = false ); // Fill menu with specified creature types
	bool Cmd_Skill_Inscription();
	bool Cmd_SecureTrade( CChar * pChar, CItem * pItem );
	bool Cmd_Control( CChar * pChar );

public:
	CSocketAddress &GetPeer();								// get peer address
	LPCTSTR GetPeerStr() const;								// get string representation of the peer address
	long GetSocketID() const;								// get socket id

public:
	explicit CClient(NetState* state);
	~CClient();

private:
	CClient(const CClient& copy);
	CClient& operator=(const CClient& other);

public:
	void CharDisconnect();

	CClient* GetNext() const
	{
		return( STATIC_CAST <CClient*>( CGObListRec::GetNext()));
	}

	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute script type command on me
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & s, CTextConsole * pSrc );
	virtual bool r_LoadVal( CScript & s );

	// Low level message traffic.
	static size_t xCompress( BYTE * pOutput, const BYTE * pInput, size_t inplen );

	bool xProcessClientSetup( CEvent * pEvent, size_t iLen );
	bool xPacketFilter(const BYTE * pEvent, size_t iLen = 0);
	bool xOutPacketFilter(const BYTE * pEvent, size_t iLen = 0);
	bool xCanEncLogin(bool bCheckCliver = false);	// Login crypt check
	// Low level push world data to the client.

	bool addRelay( const CServerDef * pServ );
	bool addLoginErr(BYTE code);
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
	bool addDeleteErr(BYTE code, DWORD iSlot);
	void addSeason(SEASON_TYPE season);
	void addTime( bool bCurrent = false );
	void addObjectRemoveCantSee( CGrayUID uid, LPCTSTR pszName = NULL );
	void closeContainer( const CObjBase * pObj );
	void closeUIWindow( const CChar* character, DWORD command );
	void addObjectRemove( CGrayUID uid );
	void addObjectRemove( const CObjBase * pObj );
	void addRemoveAll( bool fItems, bool fChars );

	void addItem_OnGround( CItem * pItem ); // Send items (on ground)
	void addItem_Equipped( const CItem * pItem );
	void addItem_InContainer( const CItem * pItem );
	void addItem( CItem * pItem );

	void addBuff( const BUFF_ICONS IconId, const DWORD ClilocOne, const DWORD ClilocTwo, const WORD Time = 0, LPCTSTR* pArgs = 0, size_t iArgCount = 0);
	void removeBuff(const BUFF_ICONS IconId);
	void resendBuffs();

	void addOpenGump( const CObjBase * pCont, GUMP_TYPE gump );
	void addContents( const CItemContainer * pCont, bool fCorpseEquip = false, bool fCorpseFilter = false, bool fShop = false ); // Send items
	bool addContainerSetup( const CItemContainer * pCont ); // Send Backpack (with items)

	void addPlayerStart( CChar * pChar );
	void addAOSPlayerSeeNoCrypt(); //sends tooltips for items in LOS
	void addPlayerSee( const CPointMap & pt ); // Send objects the player can now see
	void addPlayerSeeShip( const CPointMap & pt ); // Send objects the player can now see from a ship
	void addPlayerView( const CPointMap & pt, bool bFull = true );
	void addPlayerWarMode();

	void addCharMove( const CChar * pChar );
	void addCharMove( const CChar * pChar, BYTE bCharDir );
	void addChar( const CChar * pChar );
	void addCharName( const CChar * pChar ); // Singleclick text for a character
	void addItemName( const CItem * pItem );

	bool addKick( CTextConsole * pSrc, bool fBlock = true );
	void addWeather( WEATHER_TYPE weather = WEATHER_DEFAULT ); // Send new weather to player
	void addLight();
	void addMusic( MIDI_TYPE id );
	void addArrowQuest( int x, int y, int id );
	void addEffect( EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate * pDst, const CObjBaseTemplate * pSrc, BYTE speed = 5, BYTE loop = 1, bool explode = false, DWORD color = 0, DWORD render = 0, WORD effectid = 0, DWORD explodeid = 0, WORD explodesound = 0, DWORD effectuid = 0, BYTE type = 0 );
	void addSound( SOUND_TYPE id, const CObjBaseTemplate * pBase = NULL, int iRepeat = 1 );
	void addReSync();
	void addMap();
	void addMapDiff();
	void addChangeServer();
	void addPlayerUpdate();

	void addBark( LPCTSTR pText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue = HUE_DEFAULT, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD );
	void addBarkUNICODE( const NCHAR * pText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang = 0 );
	void addBarkLocalized( int iClilocId, const CObjBaseTemplate * pSrc, HUE_TYPE wHue = HUE_DEFAULT, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD, LPCTSTR pArgs = NULL );
	void addBarkLocalizedEx( int iClilocId, const CObjBaseTemplate * pSrc, HUE_TYPE wHue = HUE_DEFAULT, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD, AFFIX_TYPE affix = AFFIX_APPEND, LPCTSTR pAffix = NULL, LPCTSTR pArgs = NULL );
	void addBarkParse( LPCTSTR pszText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font = FONT_NORMAL, bool bUnicode = false, LPCTSTR name = "" );
	void addSysMessage( LPCTSTR pMsg ); // System message (In lower left corner)
	void addObjMessage( LPCTSTR pMsg, const CObjBaseTemplate * pSrc, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_OBJ ); // The message when an item is clicked

	void addDyeOption( const CObjBase * pBase );
	void addWebLaunch( LPCTSTR pMsg ); // Direct client to a web page

	void addPromptConsole( CLIMODE_TYPE mode, LPCTSTR pMsg, CGrayUID context1 = 0, CGrayUID context2 = 0, bool bUnicode = false );
	void addTarget( CLIMODE_TYPE targmode, LPCTSTR pMsg, bool fAllowGround = false, bool fCheckCrime = false, int iTimeout = 0 ); // Send targetting cursor to client
	void addTargetDeed( const CItem * pDeed );
	bool addTargetItems( CLIMODE_TYPE targmode, ITEMID_TYPE id, bool fGround = true );
	bool addTargetChars( CLIMODE_TYPE mode, CREID_TYPE id, bool fNoto, int iTimeout = 0 );
	void addTargetVerb( LPCTSTR pCmd, LPCTSTR pArg );
	void addTargetFunctionMulti( LPCTSTR pszFunction, ITEMID_TYPE itemid, bool fGround );
	void addTargetFunction( LPCTSTR pszFunction, bool fAllowGround, bool fCheckCrime );
	void addTargetCancel();
	void addPromptConsoleFunction( LPCTSTR pszFunction, LPCTSTR pszSysmessage, bool bUnicode = false );

	void addScrollScript( CResourceLock &s, SCROLL_TYPE type, DWORD dwcontext = 0, LPCTSTR pszHeader = NULL );
	void addScrollResource( LPCTSTR szResourceName, SCROLL_TYPE type, DWORD dwcontext = 0 );

	void addVendorClose( const CChar * pVendor );
	bool addShopMenuBuy( CChar * pVendor );
	bool addShopMenuSell( CChar * pVendor );
	void addBankOpen( CChar * pChar, LAYER_TYPE layer = LAYER_BANKBOX );

	void addSpellbookOpen( CItem * pBook, WORD offset = 1 );
	void addCustomSpellbookOpen( CItem * pBook, DWORD gumpID );
	bool addBookOpen( CItem * pBook );
	void addBookPage( const CItem * pBook, size_t iPage, size_t iCount );
	void addCharStatWindow( CChar * pChar, bool fRequested = false ); // Opens the status window
	void addHitsUpdate( CChar * pChar );
	void addManaUpdate( CChar * pChar );
	void addStamUpdate( CChar * pChar );
	void addHealthBarUpdate( const CChar * pChar );
	void addBondedStatus( const CChar * pChar, bool bIsDead );
	void addSkillWindow(SKILL_TYPE skill, bool bFromInfo = false); // Opens the skills list
	void addBulletinBoard( const CItemContainer * pBoard );
	bool addBBoardMessage( const CItemContainer * pBoard, BBOARDF_TYPE flag, CGrayUID uidMsg );

	void addToolTip( const CObjBase * pObj, LPCTSTR psztext );
	void addDrawMap( CItemMap * pItem );
	void addMapMode( CItemMap * pItem, MAPCMD_TYPE iType, bool fEdit = false );

	void addGumpTextDisp( const CObjBase * pObj, GUMP_TYPE gump, LPCTSTR pszName, LPCTSTR pszText );
	void addGumpInpVal( bool fcancel, INPVAL_STYLE style, DWORD dwmask, LPCTSTR ptext1, LPCTSTR ptext2, CObjBase * pObj );

	void addItemMenu( CLIMODE_TYPE mode, const CMenuItem * item, size_t count, CObjBase * pObj = NULL );
	void addGumpDialog( CLIMODE_TYPE mode, const CGString * sControls, size_t iControls, const CGString * psText, size_t iTexts, int x, int y, CObjBase * pObj = NULL, DWORD rid = 0 );

	bool addGumpDialogProps( CGrayUID uid );

	void addLoginComplete();
	void addChatSystemMessage(CHATMSG_TYPE iType, LPCTSTR pszName1 = NULL, LPCTSTR pszName2 = NULL, CLanguageID lang = 0 );

	void addCharPaperdoll( CChar * pChar );

	void addAOSTooltip( const CObjBase * pObj, bool bRequested = false, bool bShop = false );

private:
	#define MAX_POPUPS 15
	#define POPUPFLAG_LOCKED 0x01
	#define POPUPFLAG_ARROW 0x02
	#define POPUPFLAG_COLOR 0x20
	#define POPUP_REQUEST 0
	#define POPUP_PAPERDOLL 11
	#define POPUP_BACKPACK 12
	#define POPUP_PARTY_ADD 13
	#define POPUP_PARTY_REMOVE 14
	#define POPUP_TRADE_ALLOW 15
	#define POPUP_TRADE_REFUSE 16
	#define POPUP_TRADE_OPEN 17
	#define POPUP_BANKBOX 21
	#define POPUP_VENDORBUY 31
	#define POPUP_VENDORSELL 32
	#define POPUP_PETGUARD 41
	#define POPUP_PETFOLLOW 42
	#define POPUP_PETDROP 43
	#define POPUP_PETKILL 44
	#define POPUP_PETSTOP 45
	#define POPUP_PETSTAY 46
	#define POPUP_PETFRIEND_ADD 47
	#define POPUP_PETFRIEND_REMOVE 48
	#define POPUP_PETTRANSFER 49
	#define POPUP_PETRELEASE 50
	#define POPUP_STABLESTABLE 51
	#define POPUP_STABLERETRIEVE 52
	#define POPUP_TRAINSKILL 100

	PacketDisplayPopup* m_pPopupPacket;

public:
	void Event_AOSPopupMenuSelect( DWORD uid, WORD EntryTag );
	void Event_AOSPopupMenuRequest( DWORD uid );


	void addShowDamage( int damage, DWORD uid_damage );
	void addSpeedMode( BYTE speedMode = 0 );
	void addVisualRange( BYTE visualRange = UO_MAP_VIEW_SIZE );
	void addIdleWarning( BYTE message );
	void addKRToolbar( bool bEnable );

	void SendPacket( TCHAR * pszPacket );
	void LogOpenedContainer(const CItemContainer* pContainer);

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
	BYTE GetResDisp() const
	{
		if ( GetAccount() == NULL )
			return( UCHAR_MAX );
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
	void SetScreenSize(DWORD x, DWORD y)
	{
		m_ScreenSize.x = x;
		m_ScreenSize.y = y;
	}

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

	bool Dialog_Setup( CLIMODE_TYPE mode, RESOURCE_ID_BASE rid, int iPage, CObjBase * pObj, LPCTSTR Arguments = "" );
	bool Dialog_Close( CObjBase * pObj, DWORD rid, int buttonID );
	void Menu_Setup( RESOURCE_ID_BASE rid, CObjBase * pObj = NULL );

	int OnSkill_Info( SKILL_TYPE skill, CGrayUID uid, int iTestLevel, bool fTest );

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
	void SetTargMode( CLIMODE_TYPE targmode = CLIMODE_NORMAL, LPCTSTR pszPrompt = NULL, int iTimeout = 0 );
	void ClearTargMode()
	{
		// done with the last mode.
		m_Targ_Mode = CLIMODE_NORMAL;
		m_Targ_Timeout.Init();
	}

	bool IsConnecting() const;

	NetState* GetNetState(void) const { return m_net; };

private:
	CGString	m_BarkBuffer;

public:
	char		m_zLastMessage[SCRIPT_MAX_LINE_LEN];	// last sysmessage
	char		m_zLastObjMessage[SCRIPT_MAX_LINE_LEN];	// last message
	char		m_zLogin[64];
	CServTime	m_tNextPickup;
	CVarDefMap	m_TagDefs;
	CVarDefMap	m_BaseDefs;		// New Variable storage system
	typedef std::map<DWORD, std::pair<std::pair<DWORD,DWORD>, CPointMap> > OpenedContainerMap_t;
	OpenedContainerMap_t m_openedContainers;	// list of UIDs of all opened containers by the client

	CGObArray<CClientTooltip *> m_TooltipData; // Storage for tooltip data while in trigger

	CItemMultiCustom * m_pHouseDesign; // The building this client is designing

public:
	LPCTSTR GetDefStr( LPCTSTR pszKey, bool fZero = false ) const
	{
		return m_BaseDefs.GetKeyStr( pszKey, fZero );
	}

	INT64 GetDefNum( LPCTSTR pszKey, bool fZero = false ) const
	{
		return m_BaseDefs.GetKeyNum( pszKey, fZero );
	}

	void SetDefNum(LPCTSTR pszKey, INT64 iVal, bool fZero = true)
	{
		m_BaseDefs.SetNum(pszKey, iVal, fZero);
	}

	void SetDefStr(LPCTSTR pszKey, LPCTSTR pszVal, bool fQuoted = false, bool fZero = true)
	{
		m_BaseDefs.SetStr(pszKey, fQuoted, pszVal, fZero);
	}

	void DeleteDef(LPCTSTR pszKey)
	{
		m_BaseDefs.DeleteKey(pszKey);
	}

#ifndef _MTNETWORK
	friend class NetworkIn;
	friend class NetworkOut;
#else
	friend class NetworkInput;
	friend class NetworkOutput;
#endif
	friend class PacketCreate;
	friend class PacketServerRelay;
};

////////////////////////////////////////////////////////////////////////////////////

#include "CBase.h"
#include "CObjBase.h"
#include "CWorld.h"

////////////////////////////////////////////////////////////////////////////////////

enum SERVMODE_TYPE
{
	SERVMODE_RestockAll,	// Major event.
	SERVMODE_Saving,		// Forced save freezes the system.
	SERVMODE_Run,			// Game is up and running
	SERVMODE_ResyncPause,	// paused during resync

	SERVMODE_Loading,		// Initial load.
	SERVMODE_ResyncLoad,	// Loading after resync
	SERVMODE_Exiting		// Closing down
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
	CChat m_Chats;	// keep all the active chats

	std::vector<CItemShip *> m_ShipTimers;
	void ShipTimers_Tick();
	void ShipTimers_Add(CItemShip * ship);
	void ShipTimers_Delete(CItemShip * ship);

	char	m_PacketFilter[255][32];	// list of packet filtering functions
	char	m_OutPacketFilter[255][32];	// list of outgoing packet filtering functions

	CFileObj	fhFile;			//	file script object
	CDataBase	m_hdb;			//	SQL data base
	CSQLite		m_hldb;			//	Local database

private:
	void ProfileDump( CTextConsole * pSrc, bool bDump = false );

public:
	CServer();
	~CServer();

private:
	CServer(const CServer& copy);
	CServer& operator=(const CServer& other);

public:
	bool IsValidBusy() const;
	void SetServerMode( SERVMODE_TYPE mode );

	void SetExitFlag( int iFlag );
	void Shutdown( INT64 iMinutes );
	bool IsLoading() const
	{
		return( m_iModeCode > SERVMODE_Run || m_fResyncPause );
	}
	void SetSignals( bool fMsg = true );

	bool SocketsInit(); // Initialize sockets
	bool SocketsInit( CGSocket & socket );
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
	INT64 GetAgeHours() const;

	bool OnConsoleCmd( CGString & sText, CTextConsole * pSrc );

	void OnTick();

public:
	void ListClients( CTextConsole * pClient ) const;
	void SetResyncPause(bool fPause, CTextConsole * pSrc, bool bMessage = false);
	bool CommandLine( int argc, TCHAR * argv[] );

	LPCTSTR GetName() const { return( CServerDef::GetName()); }
	PLEVEL_TYPE GetPrivLevel() const;
} g_Serv;	// current state stuff not saved.

class Main : public AbstractSphereThread
{
public:
	Main();
	virtual ~Main() { };

private:
	Main(const Main& copy);
	Main& operator=(const Main& other);

public:
	// we increase the access level from protected to public in order to allow manual execution when
	// configuration disables using threads
	// TODO: in the future, such simulated functionality should lie in AbstractThread inself instead of hacks
	virtual void tick();

protected:
	virtual void onStart();
	virtual bool shouldExit();
};

//////////////////////////////////////////////////////////////

extern LPCTSTR g_szServerDescription;
extern int g_szServerBuild;
extern LPCTSTR const g_Stat_Name[STAT_QTY];
extern CGStringList g_AutoComplete;

extern int Sphere_InitServer( int argc, char *argv[] );
extern int Sphere_OnTick();
extern void Sphere_ExitServer();
extern int Sphere_MainEntryPoint( int argc, char *argv[] );

///////////////////////////////////////////////////////////////
// -CGrayUID

inline INT64 CObjBase::GetTimerDiff() const
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


struct TScriptProfiler
{
	unsigned char	initstate;
	DWORD		called;
	LONGLONG	total;
	struct TScriptProfilerFunction
	{
		TCHAR		name[128];	// name of the function
		DWORD		called;		// how many times called
		LONGLONG	total;		// total executions time
		LONGLONG	min;		// minimal executions time
		LONGLONG	max;		// maximal executions time
		LONGLONG	average;	// average executions time
		TScriptProfilerFunction *next;
	}		*FunctionsHead, *FunctionsTail;
	struct TScriptProfilerTrigger
	{
		TCHAR		name[128];	// name of the trigger
		DWORD		called;		// how many times called
		LONGLONG	total;		// total executions time
		LONGLONG	min;		// minimal executions time
		LONGLONG	max;		// maximal executions time
		LONGLONG	average;	// average executions time
		TScriptProfilerTrigger *next;
	}		*TriggersHead, *TriggersTail;
};
extern TScriptProfiler g_profiler;

//	Time measurement macros
extern LONGLONG llTimeProfileFrequency;

#ifdef _WIN32

#define	TIME_PROFILE_INIT	\
	LONGLONG llTicks(0), llTicksEnd
#define	TIME_PROFILE_START	\
	if ( !QueryPerformanceCounter((LARGE_INTEGER *)&llTicks)) llTicks = GetTickCount()
#define TIME_PROFILE_END	if ( !QueryPerformanceCounter((LARGE_INTEGER *)&llTicksEnd)) llTicksEnd = GetTickCount()

#else

#define	TIME_PROFILE_INIT	\
	LONGLONG llTicks(0), llTicksEnd
#define	TIME_PROFILE_START	\
	llTicks = GetTickCount()
#define TIME_PROFILE_END	llTicksEnd = GetTickCount();

#endif

#define TIME_PROFILE_GET_HI	((llTicksEnd - llTicks)/(llTimeProfileFrequency/1000))
#define	TIME_PROFILE_GET_LO	((((llTicksEnd - llTicks)*10000)/(llTimeProfileFrequency/1000))%10000)

#endif

