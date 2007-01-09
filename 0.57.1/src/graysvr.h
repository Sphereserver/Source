#ifndef _INC_GRAYSVR_H_
#define _INC_GRAYSVR_H_
#pragma once

#ifdef _WIN32
// NOTE: If we want a max number of sockets we must compile for it !
#undef FD_SETSIZE		// This shuts off a warning
#define FD_SETSIZE 1024 // for max of n users ! default = 64
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
		m_lPrivateTime = lTimeBase;
	}
	bool IsTimeValid() const
	{
		return( m_lPrivateTime ? true : false );
	}
	CServTime operator+( int iTimeDiff ) const
	{
		CServTime time;
		time.m_lPrivateTime = m_lPrivateTime + iTimeDiff;
		return( time );
	}
	CServTime operator-( int iTimeDiff ) const
	{
		CServTime time;
		time.m_lPrivateTime = m_lPrivateTime - iTimeDiff;
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

	RDS_QTY,
};

#include "common/common.h"
#include "common/config.h"
#include "common/graycom.h"
#include "common/graymul.h"
#include "common/grayproto.h"
#include "common/CGrayInst.h"
#include "common/CResourceBase.h"
#include "common/CRegion.h"
#include "common/CGrayMap.h"
#include "common/CDataBase.h"
#include "common/VariableList.h"
#include "CResource.h"
#include "CServRef.h"
#include "CAccount.h"
#include "CSector.h"

class CClient;
class CAccount;
class CWebPageDef;
class CServerDef;
class CCharBase;
class CItemBase;
class CItemContainer;
class CItemMessage;
class CItemMap;
class CDataBase;
class CSector;
class VariableList;

///////////////////////////////////////////////

// Text mashers.

extern DIR_TYPE GetDirStr(LPCTSTR pszDir, CChar *pChar);
extern LPCTSTR GetTimeMinDesc(int dwMinutes, bool numeric = false);
extern int FindStrWord( LPCTSTR pTextSearch, LPCTSTR pszKeyWord);

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
		return( static_cast <CGMPage*>( CGObListRec::GetNext()));
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
		return static_cast <CChatChannel *>(m_Channels.GetHead());
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
	CGString m_sText;
public:
	bool ParseLine( TCHAR * pszArgs, CScriptObj * pObjBase, CTextConsole * pSrc );
};

enum CLIMODE_TYPE	// What mode is the client to server connection in ? (waiting for input ?)
{
	// setup events ------------------------------------------------

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

	// prompting for text input.------------------------------------------------------
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

	// Targeting mouse cursor. -------------------------------------------------------------
	CLIMODE_MOUSE_TYPE,	// Greater than this = mouse type targeting.

	// GM targeting command stuff.
	CLIMODE_TARG_OBJ_SET,		// Set some attribute of the item i will show.
	CLIMODE_TARG_OBJ_INFO,		// what item do i want props for ?
	CLIMODE_TARG_OBJ_FUNC,

	CLIMODE_TARG_ADDITEM,		// "ADDITEM" command.
	CLIMODE_TARG_LINK,			// "LINK" command
	CLIMODE_TARG_TILE,			// "TILE" command.

	// Normal user stuff. (mouse targeting)
	CLIMODE_TARG_SKILL,				// targeting a skill or spell.
	CLIMODE_TARG_SKILL_MAGERY,
	CLIMODE_TARG_SKILL_HERD_DEST,
	CLIMODE_TARG_SKILL_POISON,
	CLIMODE_TARG_SKILL_PROVOKE,

	CLIMODE_TARG_USE_ITEM,			// target for using the selected item
	CLIMODE_TARG_PET_CMD,			// targeted pet command
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

class NetState;
class Packet;
class PacketSend;

class CClient : public CGObListRec, public CScriptObj, public CChatChanMember, public CTextConsole
{
	// TCP/IP connection to the player or console.
public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szRefKeys[];
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];

	CChar		*m_pChar;			// What char are we playing ?
	NetState	*m_net;				// network state
	CSector::Environment	m_Env; // Last Environment Info Sent. so i don't have to keep resending if it's the same.

private:
	char m_fUpdateStats;	// update our own status (weight change) when done with the cycle.

	// Walk limiting code
	int	m_iWalkTimeAvg;
	int m_iWalkStepCount;		// Count the actual steps . Turning does not count.
	LONGLONG m_timeWalkStep;	// the last %8 walk step time.
	
	// Reported ClientVersion
	bool m_bClient3d;
	int m_reportedCliver;

	// Screen size
	DWORD m_wScreenx;
	DWORD m_wScreeny;

public:
	CONNECT_TYPE	m_iConnectType;	// what sort of a connection is this ?
	CAccount		*m_pAccount;		// The account name. we logged in on

	static BYTE sm_xCompress_Buffer[MAX_BUFFER];

	CServTime m_timeLogin;		// World clock of login time. "LASTCONNECTTIME"
	CServTime m_timeLastEvent;	// Last time we got event from client.
	CServTime m_timeLastSend;		// Last time i tried to send to the client

	// GM only stuff.
	CGMPage		*m_pGMPage;		// Current GM page we are connected to.
	UID			m_Prop_UID;		// The object of /props (used for skills list as well!)

	// Current operation context args for modal async operations..
private:
	CLIMODE_TYPE m_Targ_Mode;	// Type of async operation under way.
public:
	UID		m_Targ_UID;			// The object of interest to apply to the target.
	UID		m_Targ_PrvUID;		// The object of interest before this.
	CGString m_Targ_Text;		// Text transfered up from client.
	CPointMap  m_Targ_p;			// For script targeting,

	// Context of the targeting setup. depends on CLIMODE_TYPE m_Targ_Mode
	union
	{
		// CLIMODE_SETUP_CHARLIST
		UIDBase m_tmSetupCharList[MAX_CHARS_PER_ACCT];

		// CLIMODE_INPVAL
		struct
		{
			UIDBase m_UID;
			RESOURCE_ID_BASE m_PrvGumpID;	// the gump that was up before this
		} m_tmInpVal;

		// CLIMODE_MENU_*
		// CLIMODE_MENU_SKILL
		// CLIMODE_MENU_GM_PAGES
		struct
		{
			UIDBase m_UID;
			RESOURCE_ID_BASE m_ResourceID;		// What menu is this ?
			DWORD m_Item[MAX_MENU_ITEMS];	// This saves the in range tracking targets or other context
		} m_tmMenu;	// the menu that is up.

		// CLIMODE_TARG_PET_CMD
		struct
		{
			int	m_iCmd;
			bool m_fAllPets;
		} m_tmPetCmd;	// which pet command am i targeting ?

		// CLIMODE_TARG_CHAR_BANK
		struct
		{
			LAYER_TYPE m_Layer;	// gm command targeting what layer ?
		} m_tmCharBank;

		// CLIMODE_TARG_TILE
		struct
		{
			CPointBase m_ptFirst; // Multi stage targeting.
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
			SKILL_TYPE m_Skill;			// targeting what spell ?
		} m_tmSkillTarg;

		// CLIMODE_TARG_SKILL_MAGERY
		struct
		{
			SPELL_TYPE m_Spell;			// targeting what spell ?
			CREID_TYPE m_SummonID;
			bool m_fSummonPet;
		} m_tmSkillMagery;

		// CLIMODE_TARG_USE_ITEM
		struct
		{
			CGObList * m_pParent;	// the parent of the item being targeted .
		} m_tmUseItem;
	};

public:
	// encrypt/decrypt stuff.
	CCrypt m_Crypt;			// Client source communications are always encrypted.
	static CCompressTree sm_xComp;

public:
	CSocketAddress &peer();								// get peer address
	LPCTSTR speer();									// get string representation of the peer address
	long socketId();									// get socket id
	bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );

	bool OnRxConsoleLoginComplete();
	bool OnRxConsole( const BYTE * pData, int len );
	bool OnRxWebPageRequest( CWebPageDef * pPage, LPCTSTR pszMatch );
	bool OnRxWebPageRequest( BYTE * pRequest, int len );

	BYTE LogIn( CAccountRef pAccount, CGString & sMsg );
	BYTE LogIn( LPCTSTR pszName, LPCTSTR pPassword, CGString & sMsg );

	int  Setup_FillCharList( CEventCharDef * pCharList, const CChar * pCharFirst );

	bool CanInstantLogOut() const;
	void Cmd_GM_PageClear();
	void GetAdjustedCharID( const CChar * pChar, CREID_TYPE & id, HUE_TYPE &wHue );
	void GetAdjustedItemID( const CChar * pChar, const CItem * pItem, ITEMID_TYPE & id, HUE_TYPE &wHue );
	void CharDisconnect();

	TRIGRET_TYPE Menu_OnSelect( RESOURCE_ID_BASE rid, int iSelect, CObjBase * pObj );
	TRIGRET_TYPE Dialog_OnButton( RESOURCE_ID_BASE rid, DWORD dwButtonID, CObjBase * pObj, CDialogResponseArgs * pArgs );

	BYTE Login_ServerList( const char * pszAccount, const char * pszPassword );		// Initial login (Login on "loginserver", new format)
	bool Login_Relay( int iServer );			// Relay player to a certain IP
	void Announce( bool fArrive ) const;

	BYTE Setup_ListReq( const char * pszAccount, const char * pszPassword, bool fTest );	// Game server login and character listing
	bool Setup_Start( CChar * pChar );	// Send character startup stuff to player
	void Setup_CreateDialog( const CEvent * pEvent );	// All the character creation stuff
	DELETE_ERR_TYPE Setup_Delete( int iSlot );			// Deletion of character
	bool Setup_Play( int iSlot );		// After hitting "Play Character" button

	// GM stuff.
	bool OnTarg_Obj_Set( CObjBase * pObj );
	bool OnTarg_Obj_Info( CObjBase * pObj, const CPointMap & pt, ITEMID_TYPE id );
	bool OnTarg_Obj_Function( CObjBase * pObj, const CPointMap & pt, ITEMID_TYPE id );

	bool OnTarg_Stone_Recruit(CChar* pChar, bool bFull = false);
	bool OnTarg_Item_Add( CObjBase * pObj, const CPointMap & pt ) ;
	bool OnTarg_Item_Link( CObjBase * pObj );
	bool OnTarg_Tile( CObjBase * pObj, const CPointMap & pt );

	// Normal user stuff.
	bool OnTarg_Use_Deed( CItem * pDeed, const CPointMap &pt );
	bool OnTarg_Use_Item( CObjBase * pObj, CPointMap & pt, ITEMID_TYPE id );
	bool OnTarg_Party_Add( CChar * pChar );
	CItem* OnTarg_Use_Multi( const CItemBase * pItemDef, const CPointMap & pt, WORD wAttr, HUE_TYPE wHue );

	int OnSkill_AnimalLore(UID uid, int iTestLevel, bool fTest );
	int OnSkill_Anatomy(UID uid, int iTestLevel, bool fTest );
	int OnSkill_Forensics(UID uid, int iTestLevel, bool fTest );
	int OnSkill_EvalInt(UID uid, int iTestLevel, bool fTest );
	int OnSkill_ArmsLore(UID uid, int iTestLevel, bool fTest );
	int OnSkill_ItemID(UID uid, int iTestLevel, bool fTest );
	int OnSkill_TasteID(UID uid, int iTestLevel, bool fTest );

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
	void Event_Attack(UID uid);
	void Event_Skill_Locks( const CEvent * pEvent );
	void Event_Skill_Use( SKILL_TYPE x ); // Skill is clicked on the skill list
	void Event_Book_Title( UID uid, LPCTSTR pszTitle, LPCTSTR pszAuthor );
	void Event_Book_Page( UID uid, const CEvent * pEvent ); // Book window
	void Event_Item_Dye( UID uid, HUE_TYPE wHue );	// Re hue an item
	void Event_Item_Pickup( UID uid, int amount ); // Client grabs an item
	void Event_Item_Drop( const CEvent * pEvent ); // Item is dropped on ground
	void Event_SecureTrade( UID uid, const CEvent * pEvent );
	bool Event_Walking(BYTE dir); // Player moves
	void Event_CombatMode( bool fWar ); // Only for switching to combat mode
	void Event_MenuChoice( const CEvent * pEvent ); // Choice from GMMenu or Item menu received
	void Event_PromptResp( LPCTSTR pszText, int len );
	void Event_Talk_Common(char *szText ); // PC speech
	void Event_Talk( LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode ); // PC speech
	void Event_TalkUNICODE( const CEvent * pEvent );
	void Event_SingleClick( UID uid );
	void Event_SetName( UID uid, const char * pszCharName );
	void Event_ExtCmd( EXTCMD_TYPE type, const char * pszName );
	bool Event_Command( LPCTSTR pszCommand, TALKMODE_TYPE mode = TALKMODE_SYSTEM ); // Client entered a '/' command like /ADD
	void Event_GumpInpValRet( const CEvent * pEvent );
	void Event_GumpDialogRet( const CEvent * pEvent );
	void Event_ExtData( EXTDATA_TYPE type, const CExtData * pData, int len );
	void Event_ExtAosData( EXTAOS_TYPE type, const CExtAosData * pData, DWORD m_uid, int len );
	void Event_MailMsg( UID uid1, UID uid2 );
	void Event_Profile( BYTE fWriteMode, UID uid, const CEvent * pEvent );
	void Event_MapEdit( UID uid, const CEvent * pEvent );
	void Event_BBoardRequest( UID uid, const CEvent * pEvent );
	void Event_ChatButton(const NCHAR * pszName); // Client's chat button was pressed
	void Event_ChatText( const NCHAR * pszText, int len, CLanguageID lang = 0 ); // Text from a client
	void Event_AOSItemInfo( int count , const NDWORD * uidList );

public:
	bool Event_DoubleClick( UID uid, bool fMacro, bool fTestTouch, bool fScript = false );

	// translated commands.
private:
	void Cmd_GM_PageInfo();
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

public:
	CClient(NetState *state);
	~CClient();

	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute script type command on me
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & s, CTextConsole * pSrc );
	virtual bool r_LoadVal( CScript & s );

	// Low level message traffic.
	static int xCompress( BYTE * pOutput, const BYTE * pInput, int inplen );

	void xSend(const void *pData, int length); // Buffering send function
	void xProcessClientSetup(CEvent *pEvent, int iLen);
	long xDispatchMsg(Packet *packet);
	bool xCanEncLogin(bool bCheckCliver = false);	// Login crypt check
	// Low level push world data to the client.

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
	bool addDeleteErr( DELETE_ERR_TYPE code );
	void addExtData( EXTDATA_TYPE type, const CExtData * pData, int iSize );
	void addSeason(SEASON_TYPE season);
	void addTime();
	void addObjectRemoveCantSee( UID uid, LPCTSTR pszName = NULL );
	void addObjectRemove( UID uid );
	void addObjectRemove( const CObjBase * pObj );
	void addRemoveAll( bool fItems, bool fChars );

	void addItem_OnGround( CItem * pItem ); // Send items (on ground)
	void addItem_Equipped( const CItem * pItem );
	void addItem_InContainer( const CItem * pItem );
	void addItem( CItem * pItem );

	void addOpenGump( const CObjBase * pCont, GUMP_TYPE gump );
	int  addContents( const CItemContainer * pCont, bool fCorpseEquip = false, bool fCorpseFilter = false, bool fShop = false); // Send items
	bool addContainerSetup( const CItemContainer * pCont ); // Send Backpack (with items)

	void addPlayerStart( CChar * pChar );
	void addPlayerSee( const CPointMap & pt ); // Send objects the player can now see
	void addPlayerView( const CPointMap & pt, bool playerStart = false );
	void addPlayerWarMode();

	void addCharMove( const CChar * pChar );
	void addChar( const CChar * pChar );
	void addCharName( const CChar * pChar ); // Singleclick text for a character
	void addItemName( const CItem * pItem );

	bool addKick( CTextConsole * pSrc, bool fBlock = true );
	void addWeather( WEATHER_TYPE weather = WEATHER_DEFAULT ); // Send new weather to player
	void addLight( int iLight = -1 );
	void addArrowQuest( int x, int y );
	void addEffect( EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate * pDst, const CObjBaseTemplate * pSrc, BYTE speed = 5, BYTE loop = 1, bool explode = false, DWORD color = 0, DWORD render = 0 );
	void addSound( SOUND_TYPE id, const CObjBaseTemplate * pBase = NULL, int iRepeat = 1 );
	void addReSync(bool bForceMap = false);
	void addMap( const CPointMap * pOldP, bool playerStart = false );

	void addBark( LPCTSTR pText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue = HUE_DEFAULT, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD );
	void addBarkUNICODE( const NCHAR * pText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang = 0 );
	void addBarkLocalized( int iClilocId, const CObjBaseTemplate * pSrc, HUE_TYPE wHue = HUE_DEFAULT, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD, TCHAR * pArgs = NULL );
	void addBarkParse( LPCTSTR pszText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, LPCTSTR name="" );
	void addSysMessage( LPCTSTR pMsg ); // System message (In lower left corner)
	void addObjMessage( LPCTSTR pMsg, const CObjBaseTemplate * pSrc, HUE_TYPE wHue = HUE_TEXT_DEF ); // The message when an item is clicked

	void addDyeOption( const CObjBase * pBase );
	void addItemDragCancel( BYTE type );
	void addWebLaunch( LPCTSTR pMsg ); // Direct client to a web page

	void addPromptConsole( CLIMODE_TYPE mode, LPCTSTR pMsg );
	void addTarget( CLIMODE_TYPE targmode, LPCTSTR pMsg, bool fAllowGround = false, bool fCheckCrime = false ); // Send targetting cursor to client
	void addTargetDeed( const CItem * pDeed );
	bool addTargetItems( CLIMODE_TYPE targmode, ITEMID_TYPE id );
	bool addTargetChars( CLIMODE_TYPE mode, CREID_TYPE id, bool fNoto );
	void addTargetVerb( LPCTSTR pCmd, LPCTSTR pArg );
	void addTargetFunction( LPCTSTR pszFunction, bool fAllowGround, bool fCheckCrime );
	void addPromptConsoleFunction( LPCTSTR pszFunction, LPCTSTR pszSysmessage );

	void addShopMenuBuy( CChar * pVendor );
	void addShopMenuSell( CChar * pVendor );
	void addBankOpen( CChar * pChar, LAYER_TYPE layer = LAYER_BANKBOX );

	void addSpellbookOpen( CItem * pBook, WORD offset = 1 );
	void addCustomSpellbookOpen( CItem * pBook, DWORD gumpID );
	bool addBookOpen( CItem * pBook );
	void addBookPage( const CItem * pBook, int iPage );
	void addCharStatWindow( UID uid, bool fRequested = false ); // Opens the status window
	void addHitsUpdate( UID uid );
	void addManaUpdate( UID uid );
	void addStamUpdate( UID uid );
	void addSkillWindow(SKILL_TYPE skill, bool bFromInfo = false); // Opens the skills list
	void addBulletinBoard( const CItemContainer * pBoard );
	bool addBBoardMessage( const CItemContainer * pBoard, BBOARDF_TYPE flag, UID uidMsg );

	void addMap( CItemMap * pItem );
	void addMapMode( CItemMap * pItem, MAPCMD_TYPE iType, bool fEdit = false );

	void addGumpInpVal( bool fcancel, INPVAL_STYLE style,
		DWORD dwmask, LPCTSTR ptext1, LPCTSTR ptext2, CObjBase * pObj );

	void addItemMenu( CLIMODE_TYPE mode, const CMenuItem * item, int count, CObjBase * pObj = NULL );
	void addGumpDialog( CLIMODE_TYPE mode, vector<CDialogDef::Element> *sControls, int iControls, vector<CDialogDef::Element> *psText, int iTexts, int x, int y, CObjBase * pObj = NULL, DWORD rid = 0 );

	bool addGumpDialogProps( UID uid );

	void addRedrawAll();
	void addChatSystemMessage(CHATMSG_TYPE iType, LPCTSTR pszName1 = NULL, LPCTSTR pszName2 = NULL, CLanguageID lang = 0 );

	void addCharPaperdoll( CChar * pChar );

	void addAOSTooltip( const CObjBase * pObj, bool bRealTooltip = false );
	void Event_AOSPopupMenu( DWORD uid, WORD EntryTag = 0 );

	void addSpeedMode( int speedMode = 0 );
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
			return false;
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
	void Menu_Setup( RESOURCE_ID_BASE rid, CObjBase * pObj = NULL );

	int OnSkill_Info( SKILL_TYPE skill, UID uid, int iTestLevel, bool fTest );

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
	CLIMODE_TYPE GetTargMode() const;
	void SetTargMode( CLIMODE_TYPE targmode = CLIMODE_NORMAL, LPCTSTR pszPrompt = NULL );
	void ClearTargMode();

	bool IsConnecting();

public:
	char		m_zLastMessage[SCRIPT_MAX_LINE_LEN];	// last sys message
	char		m_zLastObjMessage[SCRIPT_MAX_LINE_LEN];	// last message
	char		m_zLogin[64];
	CServTime	m_tNextPickup;
	VariableList	m_TagDefs;		// client TAGs (auto-deleted upon logout, logically)

	CGObArray<CClientTooltip *> m_TooltipData; // Storage for tool tip data while in trigger

	friend class NetworkIn;
	friend class NetworkOut;
	friend class PacketSend;
};

////////////////////////////////////////////////////////////////////////////////////

#include "CBase.h"
#include "CObjBase.h"
#include "CWorld.h"

////////////////////////////////////////////////////////////////////////////////////

enum SERVMODE_TYPE
{
	SERVMODE_Saving,		// Forced save freezes the system.
	SERVMODE_Run,			// Game is up and running
	SERVMODE_Operate = SERVMODE_Run,

	SERVMODE_Loading,		// Initial load.
	SERVMODE_Exiting,		// Closing down
};

extern class CServer : public CServerDef, public CTextConsole
{
	static LPCTSTR const sm_szVerbKeys[];

public:
	static const char *m_sClassName;
	SERVMODE_TYPE m_iModeCode;  // Just some error code to return to system.
	int  m_iExitFlag;	// identifies who caused the exit. <0 = error
	DWORD m_dwParentThread;	// The thread we got Init in.

	CGSocket m_SocketMain;	// This is the incoming monitor socket.(might be multiple ports?)
	CGSocket m_SocketGod;	// This is for god clients.

	// admin console.
	int m_iAdminClients;		// how many of my clients are admin consoles ?
	CGString m_sConsoleText;
	bool m_fConsoleTextReadyFlag;	// interlocking flag for moving between tasks.

	CChat m_Chats;	// keep all the active chats

	char	m_PacketFilter[256][32];	// list of packet filtering functions

	CFileObj	fhFile;			//	file script object
	CDataBase	m_hdb;			//	SQL data base

private:
	void ProfileDump( CTextConsole * pSrc, bool bDump = false );

public:
	CServer();
	~CServer();

	void SetServerMode( SERVMODE_TYPE mode );

	void SetExitFlag( int iFlag );
	bool IsLoading() const
	{
		return ( m_iModeCode == SERVMODE_Loading );
	}

	bool SocketsInit(); // Initialize sockets

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

public:
	void ListClients( CTextConsole * pClient ) const;
	bool CommandLine( int argc, TCHAR * argv[] );

	LPCTSTR GetName() const { return( CServerDef::GetName()); }
	PLEVEL_TYPE GetPrivLevel() const;
} g_Serv;	// current state stuff not saved.

//////////////////////////////////////////////////////////////

extern LPCTSTR const g_Stat_Name[STAT_QTY];
extern	CGStringList	g_AutoComplete;

extern int Sphere_InitServer( int argc, char *argv[] );
extern void Sphere_ExitServer();

///////////////////////////////////////////////////////////////
// -UIDBase

inline int CObjBase::GetTimerDiff() const
{
	// How long till this will expire ?
	return( g_World.GetTimeDiff( m_timeout ));
}
inline CObjBase * UIDBase::ObjFind() const
{
	if ( IsResource())
		return NULL;
	return( g_World.FindUID( m_dwInternalVal & UID_O_INDEX_MASK ));
}
inline CItem * UIDBase::ItemFind() const
{
	// IsItem() may be faster ?
	return( dynamic_cast <CItem *>( ObjFind()));
}
inline CChar * UIDBase::CharFind() const
{
	return( dynamic_cast <CChar *>( ObjFind()));
}

class TScriptProfiler
{
public:
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

public:
	void init()
	{
		if ( initstate == 0xf1 ) return;

		initstate = (unsigned char)0xf1;
		called = 0;
		total = 0;
		FunctionsHead = NULL;
		FunctionsTail = NULL;
		TriggersHead = NULL;
		TriggersTail = NULL;
	}

	bool isValid()
	{
		return (( initstate == 0xf1 ) && ( called != 0 ));
	}

private:
	unsigned char	initstate;
};
extern TScriptProfiler g_profiler;

#endif