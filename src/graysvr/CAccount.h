//
// Caccount.h
//

#ifndef _INC_CACCOUNT_H
#define _INC_CACCOUNT_H
#pragma once

class CClient;

class CAccount : public CScriptObj
{
	// RES_ACCOUNT
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szLoadKeys[];
private:
	PLEVEL_TYPE m_PrivLevel;
	CGString m_sName;			///< Name = no spaces. case independant.
	CGString m_sCurPassword;	///< Accounts auto-generated but never used should not last long !
	CGString m_sNewPassword;	///< The new password will be transfered when they use it.

#define PRIV_UNUSED0		0x0001
#define PRIV_GM				0x0002	// Acts as a GM (dif from having GM level)
#define PRIV_UNUSED1		0x0004
#define PRIV_GM_PAGE		0x0008	// Listen to GM pages or not.
#define PRIV_HEARALL		0x0010	// I can hear everything said by people of lower plevel
#define PRIV_ALLMOVE		0x0020	// I can move all things. (GM only)
#define PRIV_DETAIL			0x0040	// Show combat detail messages
#define PRIV_DEBUG			0x0080	// Show all objects as boxes and chars as humans.
#define PRIV_UNUSED2		0x0100
#define PRIV_PRIV_NOSHOW	0x0200	// Show the GM title and Invul flags.
#define PRIV_TELNET_SHORT	0x0400	// Disable broadcasts to be accepted by client
#define PRIV_JAILED			0x0800	// Must be /PARDONed from jail.
#define PRIV_UNUSED3		0x1000
#define PRIV_BLOCKED		0x2000	// The account is blocked.
#define PRIV_ALLSHOW		0x4000	// Show even the offline chars.
#define PRIV_UNUSED4		0x8000

#define PRIV_UNUSED (PRIV_UNUSED0|PRIV_UNUSED1|PRIV_UNUSED2|PRIV_UNUSED3|PRIV_UNUSED4)
	WORD m_PrivFlags;			///< optional privileges for char (bit-mapped)

	BYTE m_ResDisp;
	BYTE m_MaxChars;
	
	typedef struct { long long m_First; long long m_Last; long long m_Delay; } TimeTriesStruct_t;
	typedef std::pair<TimeTriesStruct_t, int> BlockLocalTimePair_t;
	typedef std::map<DWORD,BlockLocalTimePair_t> BlockLocalTime_t;
	BlockLocalTime_t m_BlockIP;

public:
	static const char *m_sClassName;

	CLanguageID m_lang;			// UNICODE language pref. (ENU=english)
	CGString m_sChatName;		// Chat System Name

	INT64 m_Total_Connect_Time;	// Previous total amount of time in game. (minutes) "TOTALCONNECTTIME"

	CSocketAddressIP m_Last_IP;	// last ip i logged in from.
	CGTime m_dateLastConnect;	// The last date i logged in. (use localtime())
	INT64  m_Last_Connect_Time;	// Amount of time spent online last time. (in minutes)

	CSocketAddressIP m_First_IP;	// first ip i logged in from.
	CGTime m_dateFirstConnect;	// The first date i logged in. (use localtime())

	CGrayUID m_uidLastChar;		// Last char i logged in with.
	CCharRefArray m_Chars;		// list of chars attached to this account.
	CVarDefMap m_TagDefs;		// attach extra tags here. GM comments etc.
	CVarDefMap m_BaseDefs;		// New Variable storage system

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

public:
	CAccount( LPCTSTR pszName, bool fGuest = false );
	// TODO: Why virtual? Need fix.
	virtual ~CAccount();
private:
	CAccount(const CAccount& copy);
	CAccount& operator=(const CAccount& other);

public:
	void	DeleteChars();

	static bool NameStrip( TCHAR * pszNameOut, LPCTSTR pszNameInp );
	static PLEVEL_TYPE GetPrivLevelText( LPCTSTR pszFlags );

	virtual bool r_LoadVal( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );
	virtual bool r_Verb( CScript &s, CTextConsole * pSrc );
	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	void r_Write(CScript & s);

	bool IsMyAccountChar( const CChar * pChar ) const;
	bool Kick( CTextConsole * pSrc, bool fBlock );

	bool CheckPasswordTries(CSocketAddress csaPeerName);
	void ClearPasswordTries(bool bAll = false);

	LPCTSTR GetName() const
	{
		return( m_sName );
	}
	LPCTSTR GetPassword() const
	{
		return( m_sCurPassword );
	}
	bool SetPassword( LPCTSTR pszPassword, bool isMD5Hash = false );

	void ClearPassword()
	{
		m_sCurPassword.Empty();	// can be set on next login.
	}
	bool CheckPassword( LPCTSTR pszPassword );

	LPCTSTR GetNewPassword() const
	{
		return( m_sNewPassword );
	}
	void SetNewPassword( LPCTSTR pszPassword );

	bool IsPriv( WORD wPrivFlags ) const
	{	// PRIV_GM
		return(( m_PrivFlags & wPrivFlags ) ? true : false);
	}
	void SetPrivFlags( WORD wPrivFlags )
	{
		m_PrivFlags |= wPrivFlags;
	}
	void ClearPrivFlags( WORD wPrivFlags )
	{
		m_PrivFlags &= ~wPrivFlags;
	}
	void TogPrivFlags( WORD wPrivFlags, LPCTSTR pszArgs );
	
	BYTE GetResDisp() const
	{
		return m_ResDisp;
	}
	bool SetResDisp(BYTE what)
	{
		if (what >= RDS_T2A && what < RDS_QTY)
		{
			m_ResDisp = what;
			return true;
		}
		return false;
	}
	bool SetGreaterResDisp(BYTE what)
	{
		if ( what > m_ResDisp )
			return SetResDisp( what );
		
		return false;
	}
	bool IsResDisp(BYTE what) const
	{
		return ( (m_ResDisp == what) ? true : false );
	}

	BYTE GetMaxChars() const
	{
		return minimum(m_MaxChars > 0? m_MaxChars : g_Cfg.m_iMaxCharsPerAccount, MAX_CHARS_PER_ACCT);
	}
	void SetMaxChars(BYTE chars)
	{
		m_MaxChars = minimum(chars, MAX_CHARS_PER_ACCT);
	}
	
	
	PLEVEL_TYPE GetPrivLevel() const
	{
		return( m_PrivLevel );	// PLEVEL_Counsel
	}
	void SetPrivLevel( PLEVEL_TYPE plevel );
	void OnLogin( CClient * pClient );
	void OnLogout(CClient *pClient, bool bWasChar = false);

	size_t DetachChar( CChar * pChar );
	size_t AttachChar( CChar * pChar );

	CClient * FindClient( const CClient * pExclude = NULL ) const;
};

typedef CAccount * CAccountRef;

extern class CAccounts
{
	// The full accounts database.
	// Stuff saved in *ACCT.SCP file.
protected:
	static const char *m_sClassName;
	static LPCTSTR const sm_szVerbKeys[];
	CObNameSortArray	m_Accounts;
private:

	bool Cmd_AddNew( CTextConsole * pSrc, LPCTSTR pszName, LPCTSTR pszArg, bool md5=false );
	bool Cmd_ListUnused( CTextConsole * pSrc, LPCTSTR pszDays, LPCTSTR pszVerb, LPCTSTR pszArgs, DWORD dwMask = 0);

public:
	friend class CAccount;

	bool m_fLoading;

	// Accounts.
	bool Account_SaveAll();
	bool Account_Load( LPCTSTR pszNameRaw, CScript & s, bool fChanges );
	bool Account_LoadAll( bool fChanges = true, bool fClearChanges = false );
	bool Account_OnCmd( TCHAR * pszArgs, CTextConsole * pSrc );
	size_t Account_GetCount() const
	{
		return( m_Accounts.GetCount());
	}
	CAccountRef Account_Get( size_t index );
	CAccountRef Account_Find( LPCTSTR pszName );
	CAccountRef Account_FindCreate( LPCTSTR pszName, bool fCreate = false );
	CAccountRef Account_FindChat( LPCTSTR pszName );
	bool Account_Delete( CAccount * pAccount );
	void Account_Add( CAccount * pAccount );
} g_Accounts;	// All the player accounts. name sorted CAccount

#endif

