
/**
* @file CAcount.h
*/

#ifndef _INC_CACCOUNT_H
#define _INC_CACCOUNT_H
#pragma once

#define PRIV_UNUSED0		0x0001
#define PRIV_GM				0x0002	///< Acts as a GM (dif from having GM level)
#define PRIV_UNUSED1		0x0004
#define PRIV_GM_PAGE		0x0008	///< Listen to GM pages or not.
#define PRIV_HEARALL		0x0010	///< I can hear everything said by people of lower plevel
#define PRIV_ALLMOVE		0x0020	///< I can move all things. (GM only)
#define PRIV_DETAIL			0x0040	///< Show combat detail messages
#define PRIV_DEBUG			0x0080	///< Show all objects as boxes and chars as humans.
#define PRIV_UNUSED2		0x0100
#define PRIV_PRIV_NOSHOW	0x0200	///< Show the GM title and Invul flags.
#define PRIV_TELNET_SHORT	0x0400	///< Disable broadcasts to be accepted by client
#define PRIV_JAILED			0x0800	///< Must be /PARDONed from jail.
#define PRIV_UNUSED3		0x1000
#define PRIV_BLOCKED		0x2000	///< The account is blocked.
#define PRIV_ALLSHOW		0x4000	///< Show even the offline chars.
#define PRIV_UNUSED4		0x8000
#define PRIV_UNUSED (PRIV_UNUSED0|PRIV_UNUSED1|PRIV_UNUSED2|PRIV_UNUSED3|PRIV_UNUSED4)


class CClient;

/**
* @brief Single account information.
*/
class CAccount : public CScriptObj
{
	// RES_ACCOUNT
	static LPCTSTR const sm_szVerbKeys[]; ///< Action list.
	static LPCTSTR const sm_szLoadKeys[]; ///< Script fields.
private:
	PLEVEL_TYPE m_PrivLevel; ///< Privileges level of the CAccount.
	CGString m_sName; ///< Name = no spaces. case independant.
	CGString m_sCurPassword; ///< Accounts auto-generated but never used should not last long !
	CGString m_sNewPassword; ///< The new password will be transfered when they use it.


	WORD m_PrivFlags; ///< optional privileges for char (bit-mapped)

	BYTE m_ResDisp; ///< current CAccount resdisp.
	BYTE m_MaxChars; ///< Max chars allowed for this CAccount.
	
	typedef struct { long long m_First; long long m_Last; long long m_Delay; } TimeTriesStruct_t;
	typedef std::pair<TimeTriesStruct_t, int> BlockLocalTimePair_t;
	typedef std::map<DWORD,BlockLocalTimePair_t> BlockLocalTime_t;
	BlockLocalTime_t m_BlockIP; ///< Password tries.

public:
	static const char *m_sClassName;

	CLanguageID m_lang;			///< UNICODE language preference (ENU=english).
	CGString m_sChatName;		///< Chat System Name

	INT64 m_Total_Connect_Time;	///< Previous total amount of time in game (minutes). "TOTALCONNECTTIME"

	CSocketAddressIP m_Last_IP;	///< last ip logged in from.
	CGTime m_dateLastConnect;	///< Last logged in date (use localtime()).
	INT64  m_Last_Connect_Time;	///< Amount of time spent online last time (in minutes).

	CSocketAddressIP m_First_IP;	///< First ip logged in from.
	CGTime m_dateFirstConnect;	///< First date logged in (use localtime()).

	CGrayUID m_uidLastChar;		///< Last CChar logged with this CAccount.
	CCharRefArray m_Chars;		///< CChars attached to this CAccount.
	CVarDefMap m_TagDefs;		///< Tags storage system.
	CVarDefMap m_BaseDefs;		///< New Variable storage system.

public:
	/**
	* @brief Creates a new CAccount.
	* Also sanitizes name and register de CAccount.
	* @param pszName CAccount name.
	* @param fGuest flag for guest accounts.
	*/
	CAccount(LPCTSTR pszName, bool fGuest = false);

private:
	CAccount(const CAccount& copy);
	CAccount& operator=(const CAccount& other);
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

	
	/**
	* @brief Remove a CAccount.
	* We should go track down and delete all the chars and clients that use this account !
	*/
	// virtual not required at the moment but might be if subclassed
	virtual ~CAccount();

	/************************************************************************
	* SCP related section.
	************************************************************************/

	virtual bool r_LoadVal( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );
	virtual bool r_Verb( CScript &s, CTextConsole * pSrc );
	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	void r_Write(CScript & s);

	/************************************************************************
	* Name and password related section.
	************************************************************************/

	/**
	* @brief Check and sanitizes name.
	* Basically only accepts [a-zA-Z0-9_]+ string. Also check if name is obscene.
	* @param pszNameOut output string.
	* @param pszNameInp input string.
	* @return true if name is a valid account name and not repeated, false otherwise.
	*/
	static bool NameStrip(TCHAR * pszNameOut, LPCTSTR pszNameInp);
	/**
	* @brief Get the CAccount name.
	* @return the CAccount name.
	*/
	LPCTSTR GetName() const { return( m_sName ); }
	/**
	* @brief Get the CAccount password.
	* @return the CAccount password.
	*/
	LPCTSTR GetPassword() const { return( m_sCurPassword ); }
	/**
	* @brief Set a new password.
	* fires the server trigger f_onserver_pwchange. If true, return false.
	* If password is valid, set it and return true.
	* @return true if the password is set, false otherwise.
	*/
	bool SetPassword( LPCTSTR pszPassword, bool isMD5Hash = false );
	/**
	* @brief Removes the current password.
	* The password can be set on next login.
	*/
	void ClearPassword() { m_sCurPassword.Empty(); }
	/**
	* @brief Check password agains CAccount password.
	* If CAccount has no password and password length is 0, check fails.
	* If CAccount has no password and password lenght is greater than 0, try to set password check success.
	* Then server f_onaccount_connect trigger is fired. If it returns true, check fails.
	* Last the password is checked.
	* @param pszPassword pass to check.
	* @return true if password is ok and f_onaccount_connect not returns true. False otherwise.
	*/
	bool CheckPassword( LPCTSTR pszPassword );
	/**
	* @brief Get new password.
	* @return new password.
	*/
	LPCTSTR GetNewPassword() const { return( m_sNewPassword ); }
	void SetNewPassword( LPCTSTR pszPassword );
	/**
	* @brief Wrong pw given, check the failed tries to take actions again'st this ip.
	* If the peer is localhost or 127.0.0.1, the password try do not count.
	* Register the password try and, if there is too many password tries (>100) clear the last tries.
	* @param csaPeerName Client net information.
	* @return TODOC.
	*/
	bool CheckPasswordTries(CSocketAddress csaPeerName);
	/**
	* @brief Removes saved data related to passwordtries for this CAccount.
	* We can remove the last 3 minutes or all the data.
	* @param bAll true if we want to remove all passwordtries data.
	*/
	void ClearPasswordTries(bool bAll = false);

	/************************************************************************
	* Resdisp related section.
	************************************************************************/
	/**
	* @brief Sets the resdisp value to the CAccount.
	* @param what resdisp to set.
	* @return true on success, false otherwise.
	*/
	bool SetResDisp(BYTE ResDisp)
	{
		if ( (ResDisp < RDS_NONE) || (ResDisp >= RDS_QTY) )
			return false;

		m_ResDisp = ResDisp;
		return true;
	}
	/**
	* @brief Gets the current resdisp on this CAccount.
	* @return The current resdisp.
	*/
	BYTE GetResDisp() const { return m_ResDisp; }
	/**
	* @brief Sets the resdisp on this CAccount based on pClient version.
	* @return true if success, false otherwise.
	*/
	bool SetAutoResDisp(CClient *pClient);

	/************************************************************************
	* Privileges related section.
	************************************************************************/
	/**
	* @brief Sets the PLEVEL of the CAccount.
	* @param plevel PLEVEL to set.
	*/
	void SetPrivLevel( PLEVEL_TYPE plevel );
	/**
	* @brief Gets the CAccount PLEVEL description.
	* @param pszFlags TODOC.
	* @return TODOC.
	*/
	static PLEVEL_TYPE GetPrivLevelText( LPCTSTR pszFlags );
	/**
	* @brief Gets the CAccount PLEVEL.
	* @param pszFlags TODOC.
	* @return TODOC.
	*/
	PLEVEL_TYPE GetPrivLevel() const { return( m_PrivLevel ); }
	/**
	* @brief Check CAccount for a specified privilege flags.
	* @param wPrivFlags Privilege flags to test.
	* @return true if all the flags are set, false otherwise. 
	*/
	bool IsPriv( WORD wPrivFlags ) const { return (m_PrivFlags & wPrivFlags) != 0; }
	/**
	* @brief Set the privileges flags specified.
	* @param wPrivFlags flags to set.
	*/
	void SetPrivFlags( WORD wPrivFlags ) { m_PrivFlags |= wPrivFlags; }
	/**
	* @brief Unset the privileges flags specified.
	* @param wPrivFlags flags to unset.
	*/
	void ClearPrivFlags( WORD wPrivFlags ) { m_PrivFlags &= ~wPrivFlags; }
	/**
	* @brief Operate with privilege flags.
	* If pszArgs is empty, only intersection privileges with wPrivFlags are set.
	* If pszArgs is true, set wPrivFlags.
	* If pszArgs is false, unset wPrivFlags.
	* @param wPrivFlags flags to intersect, set or unset.
	* @param pszArgs the operation to do.
	*/
	void TogPrivFlags( WORD wPrivFlags, LPCTSTR pszArgs );

	/************************************************************************
	* Log in / Log out related section.
	************************************************************************/

	/**
	* @brief Updates context information on login.
	* Updates last time connected, flags (if GM) and first time connected if needed.
	* @param pClient client logged in with this CAccount.
	*/
	void OnLogin( CClient * pClient );
	/**
	* @brief Updates context information on logout.
	* Updates total time connected.
	* @param pClient client logging out from this CAccount.
	* @param bWasChar true if is logged with a CChar.
	*/
	void OnLogout(CClient *pClient, bool bWasChar = false);
	/**
	* @brief Kick / Ban a player.
	* Only if plevel of CAccount is higher than SRC plevel, do not kick or ban.
	* @param pSrc SRC of the action.
	* @param fBlock if true, kick and ban.
	*/
	bool Kick(CTextConsole * pSrc, bool fBlock);

	/************************************************************************
	* CChars related section.
	************************************************************************/

	/**
	* @brief Get the max chars count for this CAccount.
	* @return the max chars for this CAccount.
	*/
	BYTE GetMaxChars() const
	{
		return minimum(m_MaxChars > 0? m_MaxChars : g_Cfg.m_iMaxCharsPerAccount, MAX_CHARS_PER_ACCT);
	}
	/**
	* @brief Set the max chars for this acc.
	* The max is set only if the current number of chars is lesser than the new value.
	* @param chars New value for max chars.
	*/
	void SetMaxChars(BYTE chars) { m_MaxChars = minimum(chars, MAX_CHARS_PER_ACCT); }
	/**
	* @brief Check if a CChar is owned by this CAccount.
	* @param pChar CChar to check.
	* @return true if this CAccount owns the CChar. False otherwise.
	*/
	bool IsMyAccountChar( const CChar * pChar ) const;
	/**
	* @brief Unlink the CChar from this CAccount.
	* @param CChar to detach.
	* @return TODOC.
	*/
	size_t DetachChar( CChar * pChar );
	/**
	* @brief Link the CChar to this CAccount.
	* @param pChar CChar to link.
	* @return TODOC.
	*/
	size_t AttachChar( CChar * pChar );
	/**
	* @brief Removes all chars from this CAccount.
	* If client is connected, kicks it.
	*/
	void DeleteChars();
	/**
	* @brief Get the client logged into a CAccount.
	* Can be used to check if the CAccount is logged in.
	* @param pExclude excluded client.
	* @return CClient logged into de CAccount, NULL otherwise.
	*/
	CClient * FindClient( const CClient * pExclude = NULL ) const;
};

/**
* CAccount reference.
*/
typedef CAccount * CAccountRef;

/**
* @brief The full accounts database.
* This class has methods to manage accounts by scripts and by command interface.
* Stuff saved in *ACCT.SCP file.
*/
class CAccounts
{
protected:
	static const char *m_sClassName; ///< TODOC.
	static LPCTSTR const sm_szVerbKeys[]; ///< ACCOUNT action list.
	CObNameSortArray	m_Accounts; ///< Sorted CAccount list.
public:
	/**
	* CAccount needs CAccounts methods.
	* @see CAccount
	*/
	friend class CAccount;
	/**
	* Used to control if we are loading account files.
	*/
	bool m_fLoading;
private:
	/**
	* @brief Add a new CAccount, command interface.
	* Check if extist a CAccount with the same name, and validate the name. If not exists and name is valid, create the CAccount.
	* @param pSrc command shell interface.
	* @param pszName new CAccount name.
	* @param pszArg new CAccount password.
	* @param md5 true if we need md5 to store the password.
	* @return true if CAccount creation is success, false otherwise.
	*/
	bool Cmd_AddNew( CTextConsole * pSrc, LPCTSTR pszName, LPCTSTR pszArg, bool md5=false );
	/**
	* @brief Do something to all the unused accounts.
	* First check for accounts with an inactivity of greater or equal to pszDays. Then perform the action to that accounts.
	* If action is DELETE, the accounts with privileges will not be removed.
	* @param pSrc command shell interface.
	* @param pszDays number of days of inactivity to consider a CAccount unused.
	* @param pszVerb action to perform.
	* @param pszArgs args to action.
	* @param dwMask discard any CAccount with this mask.
	* @return Always true.
	*/
	bool Cmd_ListUnused( CTextConsole * pSrc, LPCTSTR pszDays, LPCTSTR pszVerb, LPCTSTR pszArgs, DWORD dwMask = 0);
public:
	/**
	* @brief Save the accounts file.
	* @return true if successfully saved, false otherwise.
	*/
	bool Account_SaveAll();
	/**
	* @brief Load a single account. 
	* @see Account_LoadAll()
	* @param pszNameRaw header of ACCOUNT section.
	* @param s Arguments for account.
	* @param fChanges false = trap duplicates.
	* @return true if account is successfully loaded, false otherwise.
	*/
	bool Account_Load( LPCTSTR pszNameRaw, CScript & s, bool fChanges );
	/**
	* @brief Load account file.
	* If fChanges is true, will read acct file.
	* @param fChanges true if is an update.
	* @param fClearChanges true to clear the acct file.
	* @return true if succesfully load new accounts, false otherwise.
	*/
	bool Account_LoadAll( bool fChanges = true, bool fClearChanges = false );
	/**
	* @brief Perform actions to accounts via command shell interface (Command ACCOUNT).
	* @param pszArgs args provided to command ACCOUNT.
	* @param pSrc command shell interface.
	* @return the status of the command executed.
	*/
	bool Account_OnCmd( TCHAR * pszArgs, CTextConsole * pSrc );
	/**
	* @brief Get the CAccount count.
	* @return The count of CAccounts.
	*/
	size_t Account_GetCount() const	{ return( m_Accounts.GetCount() ); }
	/**
	* @brief Get a CAccountRef of an CAccount by his index.
	* @param index array index of the CAccount.
	* @return CAccountRef of the CAccount if index is valid, NULL otherwise.
	*/
	CAccountRef Account_Get( size_t index );
	/**
	* @brief Get a CAccountRef from a valid name.
	* If the name is not valid NULL is returned.
	* @param pszName the name of the CAccount we are looking for.
	* @return CAccountRef if pszName si a valid account name and exists an CAccount with that name, Null otherwise.
	*/
	CAccountRef Account_Find( LPCTSTR pszName );
	/**
	* @brief Get or create an CAccount in some circumstances.
	* If there is an CAccount with the provided name, a CAccountRef of the account is returned.
	* If there is not an CAccount with the providded name, AutoAccount is enabled in sphere.ini and the name is a valid account name, a CAcount is created and a CAccountRef of the returned.
	* Otherwise, NULL is returned.
	* @param pszName name of the account.
	* @param fAutoCreate try to create the account if not exists.
	* @return CAccountRef if account exists or created, NULL otherwise.
	*/
	CAccountRef Account_FindCreate( LPCTSTR pszName, bool fCreate = false );
	/**
	* @brief Check if a chat name is already used.
	* @param pszChatName string containing the name.
	* @return CAccountRef if the name is already used, NULL otherwise.
	*/
	CAccountRef Account_FindChat( LPCTSTR pszName );
	/**
	* @brief Remove an CAccount.
	* First try to call the f_onaccount_delete server trigger. If trigger returns true, do not remove the account and return false. If trigger returns false, remove the account and return true.
	* @param pAccount CAccount to remove.
	* @return true if CAccount was successfully removed, true otherwise.
	*/
	bool Account_Delete( CAccount * pAccount );
	/**
	* @brief Add a new CAccount.
	* First call f_onaccount_create trigger. If trigger returns true, cancel creation. If triggers return false, create the CAccount.
	* @param pAccount Account to create.
	*/
	void Account_Add( CAccount * pAccount );
};

/**
* All the player accounts. Name sorted CAccount.
*/
extern CAccounts g_Accounts;

#endif

