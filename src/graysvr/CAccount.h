#ifndef _INC_CACCOUNT_H
#define _INC_CACCOUNT_H
#pragma once

#define PRIV_GM				0x0002	// Acts as a GM (dif from having GM level)
#define PRIV_GM_PAGE		0x0008	// Listen to GM pages or not
#define PRIV_HEARALL		0x0010	// I can hear everything said by people of lower plevel
#define PRIV_ALLMOVE		0x0020	// I can move all things (GM only)
#define PRIV_DETAIL			0x0040	// Show combat detail messages
#define PRIV_DEBUG			0x0080	// Show all objects as boxes and chars as humans
#define PRIV_PRIV_NOSHOW	0x0200	// Show the GM title and Invul flags
#define PRIV_TELNET_SHORT	0x0400	// Disable broadcasts to be accepted by client
#define PRIV_JAILED			0x0800	// Must be /PARDONed from jail
#define PRIV_BLOCKED		0x2000	// The account is blocked
#define PRIV_ALLSHOW		0x4000	// Show even the offline chars

class CAccounts
{
	friend class CAccount;

protected:
	static const char *m_sClassName;
	static LPCTSTR const sm_szVerbKeys[];

	CObNameSortArray m_Accounts;			// sorted CAccount list

public:
	bool m_fLoading;

public:
	void Account_Add(CAccount *pAccount);
	bool Account_Delete(CAccount *pAccount, bool fTest = false);

	bool Account_SaveAll();
	bool Account_Load(LPCTSTR pszName, CScript &s, bool fChanges);
	bool Account_LoadAll(bool fChanges = true, bool fClearChanges = false);
	bool Account_OnCmd(TCHAR *pszArgs, CTextConsole *pSrc);
	bool Account_ChatNameAvailable(LPCTSTR pszName);

	CAccount *Account_Get(size_t index);
	CAccount *Account_Find(LPCTSTR pszName);
	CAccount *Account_FindCreate(LPCTSTR pszName, bool fCreate = false);

	size_t Account_GetCount() const
	{
		return m_Accounts.GetCount();
	}

private:
	bool Cmd_AddNew(CTextConsole *pSrc, LPCTSTR pszName, LPCTSTR pszPassword, bool fMD5 = false);
	bool Cmd_ListUnused(CTextConsole *pSrc, LPCTSTR pszDays, LPCTSTR pszVerb, LPCTSTR pszArgs, WORD wPrivFlags = 0);
};

extern CAccounts g_Accounts;

class CAccount : public CScriptObj
{
	// RES_ACCOUNT
public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szLoadKeys[];

	CAccount(LPCTSTR pszName);
	virtual ~CAccount();	// virtual not required at the moment but might be if subclassed

private:
	CGString m_sName;				// Name = no spaces (case independant)
	CGString m_sPassword;			// Accounts auto-generated but never used should not last long

	PLEVEL_TYPE m_PrivLevel;		// Privileges level of the account
	WORD m_PrivFlags;				// Optional privileges for char (bit-mapped)

	BYTE m_ResDisp;					// Account resdisp
	BYTE m_MaxChars;				// Max chars allowed for this account

public:
	INT64 m_Total_Connect_Time;		// Account total connected time (in minutes) "TOTALCONNECTTIME"
	INT64 m_Last_Connect_Time;		// Time (in minutes) spent online on last connection

	CCharRefArray m_Chars;			// CChar's attached to this account
	CGrayUID m_uidLastChar;			// Last CChar logged with this account

	CGTime m_dateFirstConnect;		// First logged in date
	CSocketAddressIP m_First_IP;	// First logged in IP

	CGTime m_dateLastConnect;		// Last logged in date
	CSocketAddressIP m_Last_IP;		// Last logged in IP

	CGString m_sChatName;			// Name used on old chat system
	CLanguageID m_lang;				// Language code in ISO 639x format (ENU=english)

	CVarDefMap m_BaseDefs;			// New variable storage system
	CVarDefMap m_TagDefs;			// Tags storage system

	CClient *m_pClient;				// Client currently using this account

	typedef struct
	{
		UINT64 m_LastTry;
		UINT64 m_BlockDelay;
	} PasswordTriesTimeStruct_t;

	typedef std::pair<PasswordTriesTimeStruct_t, int> PasswordTriesTimePair_t;
	typedef std::map<DWORD, PasswordTriesTimePair_t> PasswordTriesTime_t;
	PasswordTriesTime_t m_PasswordTries;		// Password tries counter

public:
	LPCTSTR GetDefStr(LPCTSTR pszKey, bool fZero = false) const
	{
		return m_BaseDefs.GetKeyStr(pszKey, fZero);
	}
	INT64 GetDefNum(LPCTSTR pszKey) const
	{
		return m_BaseDefs.GetKeyNum(pszKey);
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

public:
	LPCTSTR GetName() const
	{
		return m_sName;
	}

	static bool NameStrip(TCHAR *pszNameOut, LPCTSTR pszNameIn);

	bool SetPassword(LPCTSTR pszPassword, bool fMD5 = false);
	bool CheckPassword(LPCTSTR pszPassword);

	bool CheckPasswordTries(CSocketAddress SockAddr);
	void ClearPasswordTries();

	static PLEVEL_TYPE GetPrivLevelText(LPCTSTR pszFlags);

	PLEVEL_TYPE GetPrivLevel() const
	{
		return m_PrivLevel;
	}
	void SetPrivLevel(PLEVEL_TYPE plevel)
	{
		m_PrivLevel = plevel;
		if ( plevel >= PLEVEL_GM )
			SetPrivFlags(PRIV_GM|PRIV_GM_PAGE);
		else
		{
			ClearPrivFlags(PRIV_GM|PRIV_GM_PAGE|PRIV_HEARALL|PRIV_ALLMOVE|PRIV_DEBUG|PRIV_PRIV_NOSHOW);
			if ( plevel >= PLEVEL_Counsel )
				SetPrivFlags(PRIV_GM_PAGE);
		}
	}

	bool IsPriv(WORD wPrivFlags) const
	{
		return (m_PrivFlags & wPrivFlags);
	}

	void SetPrivFlags(WORD wPrivFlags)
	{
		m_PrivFlags |= wPrivFlags;
	}
	void ClearPrivFlags(WORD wPrivFlags)
	{
		m_PrivFlags &= ~wPrivFlags;
	}
	void TogPrivFlags(WORD wPrivFlags, LPCTSTR pszArgs)
	{
		if ( !pszArgs || (pszArgs[0] == '\0') )
			m_PrivFlags ^= wPrivFlags;
		else if ( Exp_GetVal(pszArgs) )
			m_PrivFlags |= wPrivFlags;
		else
			m_PrivFlags &= ~wPrivFlags;
	}

	BYTE GetResDisp() const
	{
		return m_ResDisp;
	}
	bool SetResDisp(BYTE bResDisp)
	{
		m_ResDisp = bResDisp;
		return true;
	}
	bool SetAutoResDisp(CClient *pClient);

	void OnLogin(CClient *pClient);
	void OnLogout(CClient *pClient, bool fWasChar = false);
	bool Kick(CTextConsole *pSrc, bool fBlock);

	size_t AttachChar(CChar *pChar);
	size_t DetachChar(CChar *pChar);

	void DeleteChars();
	bool IsMyAccountChar(const CChar *pChar) const;

	BYTE GetMaxChars() const
	{
		return minimum(m_MaxChars ? m_MaxChars : g_Cfg.m_iMaxCharsPerAccount, MAX_CHARS_PER_ACCT);
	}
	void SetMaxChars(BYTE chars)
	{
		m_MaxChars = minimum(chars, MAX_CHARS_PER_ACCT);
	}

	virtual bool r_LoadVal(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);
	virtual bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);
	void r_Write(CScript &s);

private:
	CAccount(const CAccount &copy);
	CAccount &operator=(const CAccount &other);
};

#endif	// _INC_CACCOUNT_H
