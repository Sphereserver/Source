//
// CChat.h
//

#ifndef _INC_CCHAT_H
#define _INC_CCHAT_H
#pragma once

class CChatChannel;
class CChatMember;

class CChat
{
private:
	CChat(const CChat &copy);
	CChat &operator=(const CChat &other);

public:
	static const char *m_sClassName;
	CGObList m_Channels;		// list of chat channels (CChatChannel)

	CChat() { };

	bool CreateChannel(LPCTSTR pszName, LPCTSTR pszPassword = NULL, CChatMember *pMember = NULL);
	void DeleteChannel(CChatChannel *pChannel);
	void JoinChannel(LPCTSTR pszName, LPCTSTR pszPassword, CChatMember *pMember);
	CChatChannel *FindChannel(LPCTSTR pszChannel) const;

	void Action(CClient *pClient, const NCHAR *pszText, int len, CLanguageID lang);
	void QuitChat(CChatMember *pClient);

	static void FormatName(CGString &sName, const CChatMember *pMember = NULL, bool bSystem = false);
	static bool IsValidName(LPCTSTR pszName, bool bPlayer);

	void Broadcast(CChatMember *pFrom, LPCTSTR pszText, CLanguageID lang = 0, bool bOverride = false);
	void BroadcastAddChannel(CChatChannel *pChannel);
	void BroadcastRemoveChannel(CChatChannel *pChannel);
};

class CChatChannel : public CGObListRec
{
private:
	friend class CChatMember;
	friend class CChat;
	CGString m_sName;
	CGString m_sPassword;
	bool m_bStatic;			// static channel created on server startup
	bool m_bDefaultVoice;	// give others voice by default

public:
	static const char *m_sClassName;
	CGPtrTypeArray<CChatMember *> m_Members;	// list of channel members
	CGObArray<CGString *> m_Moderators;			// list of channel moderators (may or may not be currently in the channel)
	CGObArray<CGString *> m_Muted;				// list of channel muted members

private:
	void AddMember(CChatMember *pMember);
	void RemoveMember(CChatMember *pMember);
	bool RemoveMember(LPCTSTR pszName);
	void KickMember(CChatMember *pByMember, CChatMember *pMember);
	CChatMember *FindMember(LPCTSTR pszName) const;

	bool IsModerator(LPCTSTR pszName);
	void SetModerator(LPCTSTR pszName, bool bRemoveAccess = false);
	void AddModerator(CChatMember *pByMember, LPCTSTR pszName);
	void RemoveModerator(CChatMember *pByMember, LPCTSTR pszName);
	void ToggleModerator(CChatMember *pByMember, LPCTSTR pszName);

	bool HasVoice(LPCTSTR pszName);
	void SetVoice(LPCTSTR pszName, bool bRemoveAccess = false);
	void AddVoice(CChatMember *pByMember, LPCTSTR pszName);
	void RemoveVoice(CChatMember *pByMember, LPCTSTR pszName);
	void ToggleVoice(CChatMember *pByMember, LPCTSTR pszName);

	void EnableDefaultVoice(LPCTSTR pszName);
	void DisableDefaultVoice(LPCTSTR pszName);
	void ToggleDefaultVoice(LPCTSTR pszName);

	void MemberTalk(CChatMember *pByMember, LPCTSTR pszText, CLanguageID lang);
	void Emote(LPCTSTR pszBy, LPCTSTR pszMsg, CLanguageID lang);
	void PrivateMessage(CChatMember *pFrom, LPCTSTR pszTo, LPCTSTR pszMsg, CLanguageID lang);

	void WhoIs(LPCTSTR pszBy, LPCTSTR pszMember);
	void RenameChannel(CChatMember *pByMember, LPCTSTR pszName);

	void ChangePassword(CChatMember *pByMember, LPCTSTR pszPassword);
	LPCTSTR GetPasswordString() const
	{
		return m_sPassword.IsEmpty() ? "0" : "1";
	}

public:
	explicit CChatChannel(LPCTSTR pszName, LPCTSTR pszPassword = NULL, bool bStatic = false)
	{
		m_sName = pszName;
		m_sPassword = pszPassword;
		m_bStatic = bStatic;
		m_bDefaultVoice = true;
	};

private:
	CChatChannel(const CChatChannel &copy);
	CChatChannel &operator=(const CChatChannel &other);

	size_t FindMemberIndex(LPCTSTR pszName) const;
	CChatChannel *GetNext() const
	{
		return static_cast<CChatChannel *>(CGObListRec::GetNext());
	}

	void Broadcast(CHATMSG_TYPE iType, LPCTSTR pszName = NULL, LPCTSTR pszText = NULL, CLanguageID lang = 0, bool bOverride = false);
	void SendMember(CChatMember *pMember, CChatMember *pToMember = NULL);
	void FillMembersList(CChatMember *pMember);
};

class CChatMember		// This is member of CClient
{
private:
	friend class CChatChannel;
	friend class CChat;
	bool m_bReceiving;
	bool m_bAllowWhoIs;
	CChatChannel *m_pChannel;

	CChatMember(const CChatMember &copy);
	CChatMember &operator=(const CChatMember &other);

public:
	static const char *m_sClassName;
	CGObArray<CGString *> m_IgnoredMembers;
	bool m_bChatActive;

	CChatMember()
	{
		m_bChatActive = false;
		m_bReceiving = true;
		m_bAllowWhoIs = true;
		m_pChannel = NULL;
	}
	virtual ~CChatMember();

protected:
	void addChatWindow();

private:
	void SendChatMsg(CHATMSG_TYPE iType, LPCTSTR pszName1 = NULL, LPCTSTR pszName2 = NULL, CLanguageID lang = 0);

	void SetReceiving(bool bToggle)
	{
		if ( m_bReceiving != bToggle )
			ToggleReceiving();
	}
	void ToggleReceiving();

	void ShowCharacterName();
	void HideCharacterName();
	void ToggleCharacterName();

	CChatChannel *GetChannel() const { return m_pChannel; }
	void SetChannel(CChatChannel *pChannel)
	{
		m_pChannel = pChannel;
		m_bChatActive = (m_pChannel != NULL);
	}
	void RenameChannel(LPCTSTR pszName);

	bool IsIgnoring(LPCTSTR pszName) const
	{
		return (FindIgnoringIndex(pszName) != m_IgnoredMembers.BadIndex());
	}
	void AddIgnore(LPCTSTR pszName);
	void RemoveIgnore(LPCTSTR pszName);
	void ToggleIgnore(LPCTSTR pszName);
	void ClearIgnoreList();
	size_t FindIgnoringIndex(LPCTSTR pszName) const;

	CClient *GetClient();
	LPCTSTR GetChatName();
};

class CGlobalChat		// This is member of CClient
{
public:
	static const char *m_sClassName;

	CGlobalChat()
	{
		m_dwID = 0;
		m_pszJID = '\0';
		m_fVisible = false;
	}
	~CGlobalChat() { };

private:
	DWORD m_dwID;		// client connection ID
	LPCTSTR m_pszJID;	// client Jabber ID
	bool m_fVisible;	// client visibility status (online/offline)

public:
	void SetID(DWORD dwID)
	{
		m_dwID = dwID;
	}
	DWORD GetID() const
	{
		return m_dwID;
	}

	void SetJID(LPCTSTR pszJID)
	{
		m_pszJID = pszJID;
	}
	LPCTSTR GetJID() const
	{
		return m_pszJID;
	}

	void SetVisible(bool fSet)
	{
		m_fVisible = fSet;
	}
	bool IsVisible() const
	{
		return m_fVisible;
	}

private:
	CGlobalChat(const CGlobalChat &copy);
	CGlobalChat &operator=(const CGlobalChat &other);
};

#endif	// _INC_CCHAT_H
