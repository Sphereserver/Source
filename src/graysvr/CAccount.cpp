#include "graysvr.h"	// predef header.
#include "../network/network.h"

extern "C"
{
	void globalstartsymbol() {}	// put this here as just the starting offset.
	const int globalstartdata = 0xffffffff;
}

//**********************************************************************
// -CAccounts


bool CAccounts::Account_Load( LPCTSTR pszNameRaw, CScript & s, bool fChanges )
{
	ADDTOCALLSTACK("CAccounts::Account_Load");

	// Only read as "[ACCOUNT name]" format if arguments exist.
	if ( s.HasArgs() && !strcmpi(pszNameRaw, "ACCOUNT") )
	{
		pszNameRaw = s.GetArgStr();
	}

	TCHAR szName[MAX_ACCOUNT_NAME_SIZE];
	if ( !CAccount::NameStrip(szName, pszNameRaw) )
	{
		if ( !fChanges )
		{
			g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': BAD name\n", pszNameRaw);
			return false;
		}
	}
	m_fLoading = true;

	CAccountRef pAccount = Account_Find(szName);
	if ( pAccount )
	{
		// Look for existing duplicate ?
		if ( !fChanges )
		{
			g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': duplicate name\n", pszNameRaw);
			return false;
		}
	}
	else
	{
		pAccount = new CAccount(szName);
		ASSERT(pAccount != NULL);
	}
	pAccount->r_Load(s);

	m_fLoading = false;

	return true;
}

bool CAccounts::Account_LoadAll( bool fChanges, bool fClearChanges )
{
	ADDTOCALLSTACK("CAccounts::Account_LoadAll");
	LPCTSTR pszBaseDir;
	LPCTSTR pszBaseName;
	char	*z = Str_GetTemp();

	pszBaseDir = g_Cfg.m_sAcctBaseDir.IsEmpty() ? g_Cfg.m_sWorldBaseDir : g_Cfg.m_sAcctBaseDir;
	pszBaseName = fChanges ? (SPHERE_FILE "acct") : (SPHERE_FILE "accu");

	strcpy(z, pszBaseDir);
	strcat(z, pszBaseName);

	if ( !fChanges )
		g_Log.Event(LOGM_INIT, "Loading %s%s\n", z, SPHERE_SCRIPT);

	CScript s;
	if ( ! s.Open(z, OF_READ|OF_TEXT|OF_DEFAULTMODE| ( fChanges ? OF_NONCRIT : 0) ))
	{
		if ( !fChanges )
		{
			if ( Account_LoadAll(true) )	// if we have changes then we are ok.
				return true;

											//	auto-creating account files
			if ( !Account_SaveAll() )
				g_Log.Event(LOGL_FATAL|LOGM_INIT, "Can't open account file '%s'\n", static_cast<LPCTSTR>(s.GetFilePath()));
			else
				return true;
		}
		return false;
	}

	if ( fClearChanges )
	{
		ASSERT( fChanges );
		// empty the changes file.
		s.Close();
		s.Open(NULL, OF_WRITE|OF_TEXT|OF_DEFAULTMODE);
		s.WriteString( "// Accounts are periodically moved to the " SPHERE_FILE "accu" SPHERE_SCRIPT " file.\n"
			"// All account changes should be made here.\n"
			"// Use the /ACCOUNT UPDATE command to force accounts to update.\n"
			);
		return true;
	}

	CScriptFileContext ScriptContext(&s);
	while (s.FindNextSection())
	{
		Account_Load(s.GetKey(), s, fChanges);
	}

	if ( !fChanges ) Account_LoadAll(true);

	return true;
}


bool CAccounts::Account_SaveAll()
{
	ADDTOCALLSTACK("CAccounts::Account_SaveAll");
	EXC_TRY("SaveAll");
	// Look for changes FIRST.
	Account_LoadAll(true);

	LPCTSTR pszBaseDir;

	if ( g_Cfg.m_sAcctBaseDir.IsEmpty() ) pszBaseDir = g_Cfg.m_sWorldBaseDir;
	else pszBaseDir = g_Cfg.m_sAcctBaseDir;

	CScript s;
	if ( !CWorld::OpenScriptBackup(s, pszBaseDir, "accu", g_World.m_iSaveCountID) )
		return false;

	s.Printf("// " SPHERE_TITLE " %s accounts file\n"
		"// NOTE: This file cannot be edited while the server is running.\n"
		"// Any file changes must be made to " SPHERE_FILE "acct" SPHERE_SCRIPT ". This is read in at save time.\n",
		g_Serv.GetName());

	for ( size_t i = 0; i < m_Accounts.GetCount(); i++ )
	{
		CAccountRef pAccount = Account_Get(i);
		if ( pAccount )
			pAccount->r_Write(s);
	}

	Account_LoadAll(true, true);	// clear the change file now.
	return true;
	EXC_CATCH;
	return false;
}

CAccountRef CAccounts::Account_FindChat( LPCTSTR pszChatName )
{
	ADDTOCALLSTACK("CAccounts::Account_FindChat");
	for ( size_t i = 0; i < m_Accounts.GetCount(); i++ )
	{
		CAccountRef pAccount = Account_Get(i);
		if ( pAccount != NULL && pAccount->m_sChatName.CompareNoCase(pszChatName) == 0 )
			return pAccount;
	}
	return NULL;
}

CAccountRef CAccounts::Account_Find( LPCTSTR pszName )
{
	ADDTOCALLSTACK("CAccounts::Account_Find");
	TCHAR szName[ MAX_ACCOUNT_NAME_SIZE ];

	if ( !CAccount::NameStrip(szName, pszName) )
		return( NULL );

	size_t i = m_Accounts.FindKey(szName);
	if ( i != m_Accounts.BadIndex() )
		return Account_Get(i);

	return NULL;
}

CAccountRef CAccounts::Account_FindCreate( LPCTSTR pszName, bool fAutoCreate )
{
	ADDTOCALLSTACK("CAccounts::Account_FindCreate");

	CAccountRef pAccount = Account_Find(pszName);
	if ( pAccount )
		return pAccount;

	if ( fAutoCreate )	// Create if not found.
	{
		bool fGuest = ( g_Serv.m_eAccApp == ACCAPP_GuestAuto || g_Serv.m_eAccApp == ACCAPP_GuestTrial || ! strnicmp( pszName, "GUEST", 5 ));
		TCHAR szName[ MAX_ACCOUNT_NAME_SIZE ];

		if (( pszName[0] >= '0' ) && ( pszName[0] <= '9' ))
			g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': BAD name. Due to account enumerations incompatibilities, account names could not start with digits.\n", pszName);
		else if ( CAccount::NameStrip(szName, pszName) )
			return new CAccount(szName, fGuest);
		else
			g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': BAD name\n", pszName);
	}
	return NULL;
}

bool CAccounts::Account_Delete( CAccount * pAccount )
{
	ADDTOCALLSTACK("CAccounts::Account_Delete");
	ASSERT(pAccount != NULL);

	CScriptTriggerArgs Args;
	Args.Init(pAccount->GetName());
	enum TRIGRET_TYPE tr = TRIGRET_RET_FALSE;
	g_Serv.r_Call("f_onaccount_delete", &g_Serv, &Args, NULL, &tr);
	if ( tr == TRIGRET_RET_TRUE )
	{
		return false;
	}

	m_Accounts.DeleteOb( pAccount );
	return true;
}

void CAccounts::Account_Add( CAccount * pAccount )
{
	ADDTOCALLSTACK("CAccounts::Account_Add");
	ASSERT(pAccount != NULL);
	if ( !g_Serv.IsLoading() )
	{
		CScriptTriggerArgs Args;
		Args.Init(pAccount->GetName());
		//Accounts are 'created' in server startup so we don't fire the function.
		TRIGRET_TYPE tRet = TRIGRET_RET_FALSE;
		g_Serv.r_Call("f_onaccount_create", &g_Serv, &Args, NULL, &tRet);
		if ( tRet == TRIGRET_RET_TRUE )
		{
			g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': Creation blocked via script\n", pAccount->GetName());
			Account_Delete(pAccount);
			return;
		}
	}
	m_Accounts.AddSortKey(pAccount,pAccount->GetName());
}

CAccountRef CAccounts::Account_Get( size_t index )
{
	ADDTOCALLSTACK("CAccounts::Account_Get");
	if ( !m_Accounts.IsValidIndex(index) )
		return NULL;
	return CAccountRef(static_cast<CAccount *>(m_Accounts[index]));
}

bool CAccounts::Cmd_AddNew( CTextConsole * pSrc, LPCTSTR pszName, LPCTSTR pszArg, bool md5 )
{
	ADDTOCALLSTACK("CAccounts::Cmd_AddNew");
	if (pszName == NULL || pszName[0] == '\0')
	{
		g_Log.Event(LOGL_ERROR|LOGM_INIT, "Username is required to add an account.\n" );
		return false;
	}

	CAccountRef pAccount = Account_Find( pszName );
	if ( pAccount != NULL )
	{
		pSrc->SysMessagef( "Account '%s' already exists\n", pszName );
		return false;
	}
	
	TCHAR szName[ MAX_ACCOUNT_NAME_SIZE ];

	if ( !CAccount::NameStrip(szName, pszName) )
	{
		g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': BAD name\n", pszName);
		return false;
	}

	pAccount = new CAccount(szName);
	ASSERT(pAccount);
	pAccount->m_dateFirstConnect = pAccount->m_dateLastConnect = CGTime::GetCurrentTime();

	pAccount->SetPassword(pszArg, md5);
	return true;
}

/**
* CAccount actions.
* This enum describes the CACcount acctions performed by command shell.
*/
enum VACS_TYPE
{
	VACS_ADD, ///< Add a new CAccount.
	VACS_ADDMD5, ///< Add a new CAccount, storing the password with md5.
	VACS_BLOCKED, ///< Bloc a CAccount.
	VACS_HELP, ///< Show CAccount commands.
	VACS_JAILED, ///< "Jail" the CAccount.
	VACS_UNUSED, ///< Use a command on unused CAccount.
	VACS_UPDATE, ///< Process the acct file to add accounts.
	VACS_QTY ///< TODOC.
};

LPCTSTR const CAccounts::sm_szVerbKeys[] =	// CAccounts:: // account group verbs.
{
	"ADD",
	"ADDMD5",
	"BLOCKED",
	"HELP",
	"JAILED",
	"UNUSED",
	"UPDATE",
	NULL,
};

bool CAccounts::Cmd_ListUnused(CTextConsole * pSrc, LPCTSTR pszDays, LPCTSTR pszVerb, LPCTSTR pszArgs, DWORD dwMask)
{
	ADDTOCALLSTACK("CAccounts::Cmd_ListUnused");
	int iDaysTest = Exp_GetVal(pszDays);

	CGTime datetime = CGTime::GetCurrentTime();
	int iDaysCur = datetime.GetDaysTotal();

	size_t iCountOrig = Account_GetCount();
	size_t iCountCheck = iCountOrig;
	size_t iCount = 0;
	for ( size_t i = 0; ; i++ )
	{
		if ( Account_GetCount() < iCountCheck )
		{
			iCountCheck--;
			i--;
		}
		CAccountRef pAccount = Account_Get(i);
		if ( pAccount == NULL )
			break;

		int iDaysAcc = pAccount->m_dateLastConnect.GetDaysTotal();
		if ( ! iDaysAcc )
		{
			// account has never been used ? (get create date instead)
			iDaysAcc = pAccount->m_dateFirstConnect.GetDaysTotal();
		}

		if ( (iDaysCur - iDaysAcc) < iDaysTest ) continue;
		if ( dwMask && !pAccount->IsPriv(static_cast<WORD>(dwMask)) ) continue;

		iCount ++;
		if ( pszVerb == NULL || pszVerb[0] == '\0' )
		{
			// just list stuff about the account.
			CScript script("SHOW LASTCONNECTDATE");
			pAccount->r_Verb( script, pSrc );
			continue;
		}

		// can't delete unused priv accounts this way.
		if ( ! strcmpi( pszVerb, "DELETE" ) && pAccount->GetPrivLevel() > PLEVEL_Player )
		{
			iCount--;
			pSrc->SysMessagef("Can't delete PrivLevel %d account '%s' this way.\n", pAccount->GetPrivLevel(), pAccount->GetName());
		}
		else
		{
			CScript script( pszVerb, pszArgs );
			pAccount->r_Verb( script, pSrc );
		}
	}

	pSrc->SysMessagef("Matched %" FMTSIZE_T " of %" FMTSIZE_T " accounts unused for %d days\n", iCount, iCountOrig, iDaysTest);

	if ( pszVerb && ! strcmpi(pszVerb, "DELETE") )
	{
		size_t iDeleted = iCountOrig - Account_GetCount();

		if ( iDeleted < iCount )
			pSrc->SysMessagef("%" FMTSIZE_T " deleted, %" FMTSIZE_T " cleared of characters (must try to delete again)\n", iDeleted, iCount - iDeleted);
		else if ( iDeleted > 0 )
			pSrc->SysMessagef("All %" FMTSIZE_T " unused accounts deleted.\n", iDeleted);
		else
			pSrc->SysMessagef("No accounts deleted.\n");
	}

	return true;
}

bool CAccounts::Account_OnCmd( TCHAR * pszArgs, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CAccounts::Account_OnCmd");
	ASSERT( pSrc );

	if ( pSrc->GetPrivLevel() < PLEVEL_Admin )
		return( false );

	TCHAR * ppCmd[5];
	size_t iQty = Str_ParseCmds( pszArgs, ppCmd, COUNTOF( ppCmd ));

	VACS_TYPE index;
	if ( iQty <= 0 ||
		ppCmd[0] == NULL ||
		ppCmd[0][0] == '\0' )
	{
		index = VACS_HELP;
	}
	else
	{
		index = (VACS_TYPE) FindTableSorted( ppCmd[0], sm_szVerbKeys, COUNTOF( sm_szVerbKeys )-1 );
	}

	static LPCTSTR const sm_pszCmds[] =
	{
		"/ACCOUNT UPDATE\n",
		"/ACCOUNT UNUSED days [command]\n",
		"/ACCOUNT ADD name password",
		"/ACCOUNT ADDMD5 name hash",
		"/ACCOUNT name BLOCK 0/1\n",
		"/ACCOUNT name JAIL 0/1\n",
		"/ACCOUNT name DELETE = delete all chars and the account\n",
		"/ACCOUNT name PLEVEL x = set account priv level\n",
		"/ACCOUNT name EXPORT filename\n"
	};

	switch (index)
	{
		case VACS_ADD:
			return Cmd_AddNew(pSrc, ppCmd[1], ppCmd[2]);

		case VACS_ADDMD5:
			return Cmd_AddNew(pSrc, ppCmd[1], ppCmd[2], true);

		case VACS_BLOCKED:
			return Cmd_ListUnused(pSrc, ppCmd[1], ppCmd[2], ppCmd[3], PRIV_BLOCKED);

		case VACS_HELP:
			{
				for ( size_t i = 0; i < COUNTOF(sm_pszCmds); i++ )
					pSrc->SysMessage( sm_pszCmds[i] );
			}
			return true;

		case VACS_JAILED:
			return Cmd_ListUnused(pSrc, ppCmd[1], ppCmd[2], ppCmd[3], PRIV_JAILED);

		case VACS_UPDATE:
			Account_SaveAll();
			return true;

		case VACS_UNUSED:
			return Cmd_ListUnused( pSrc, ppCmd[1], ppCmd[2], ppCmd[3] );

		default:
			break;
	}

	// Must be a valid account ?

	CAccountRef pAccount = Account_Find( ppCmd[0] );
	if ( pAccount == NULL )
	{
		pSrc->SysMessagef( "Account '%s' does not exist\n", ppCmd[0] );
		return false;
	}

	if ( !ppCmd[1] || !ppCmd[1][0] )
	{
		CClient	*pClient = pAccount->FindClient();

		char *z = Str_GetTemp();
		sprintf(z, "Account '%s': PLEVEL:%d, BLOCK:%d, IP:%s, CONNECTED:%s, ONLINE:%s\n",
			pAccount->GetName(), pAccount->GetPrivLevel(), static_cast<int>(pAccount->IsPriv(PRIV_BLOCKED)),
			pAccount->m_Last_IP.GetAddrStr(), pAccount->m_dateLastConnect.Format(NULL),
			( pClient ? ( pClient->GetChar() ? pClient->GetChar()->GetName() : "<not logged>" ) : "no" ));
		pSrc->SysMessage(z);
		return true;
	}
	else
	{
		CGString cmdArgs;
		if (ppCmd[4] && ppCmd[4][0])
			cmdArgs.Format("%s %s %s", ppCmd[2], ppCmd[3], ppCmd[4]);
		else if (ppCmd[3] && ppCmd[3][0])
			cmdArgs.Format("%s %s", ppCmd[2], ppCmd[3]);
		else if (ppCmd[2] && ppCmd[2][0])
			cmdArgs.Format("%s", ppCmd[2]);
		
		CScript script( ppCmd[1], cmdArgs.GetPtr() );
		
		return pAccount->r_Verb( script, pSrc );
	}
}

//**********************************************************************
// -CAccount


bool CAccount::NameStrip( TCHAR * pszNameOut, LPCTSTR pszNameInp )
{
	ADDTOCALLSTACK("CAccount::NameStrip");

	size_t iLen = Str_GetBare(pszNameOut, pszNameInp, MAX_ACCOUNT_NAME_SIZE, ACCOUNT_NAME_VALID_CHAR);

	if ( iLen <= 0 )
		return false;
	// Check for newline characters.
	if ( strchr(pszNameOut, 0x0A) || strchr(pszNameOut, 0x0C) || strchr(pszNameOut, 0x0D) )
		return false;
	if ( !strcmpi(pszNameOut, "EOF") || !strcmpi(pszNameOut, "ACCOUNT") )
		return false;
	// Check for name already used.
	if ( FindTableSorted(pszNameOut, CAccounts::sm_szVerbKeys, COUNTOF(CAccounts::sm_szVerbKeys)-1) >= 0 )
		return false;
	if ( g_Cfg.IsObscene(pszNameOut) )
		return false;

	return true;
}

static LPCTSTR const sm_szPrivLevels[ PLEVEL_QTY+1 ] =
{
	"Guest",		// 0 = This is just a guest account. (cannot PK)
	"Player",		// 1 = Player or NPC.
	"Counsel",		// 2 = Can travel and give advice.
	"Seer",			// 3 = Can make things and NPC's but not directly bother players.
	"GM",			// 4 = GM command clearance
	"Dev",			// 5 = Not bothererd by GM's
	"Admin",		// 6 = Can switch in and out of gm mode. assign GM's
	"Owner",		// 7 = Highest allowed level.
	NULL
};

PLEVEL_TYPE CAccount::GetPrivLevelText( LPCTSTR pszFlags ) // static
{
	ADDTOCALLSTACK("CAccount::GetPrivLevelText");
	int level = FindTable( pszFlags, sm_szPrivLevels, COUNTOF(sm_szPrivLevels)-1 );
	if ( level >= 0 )
		return static_cast<PLEVEL_TYPE>(level);

	level = Exp_GetVal( pszFlags );
	if ( level < PLEVEL_Guest )
		return PLEVEL_Guest;
	if ( level > PLEVEL_Owner )
		return PLEVEL_Owner;

	return static_cast<PLEVEL_TYPE>(level);
}

CAccount::CAccount( LPCTSTR pszName, bool fGuest )
{
	g_Serv.StatInc(SERV_STAT_ACCOUNTS);

	TCHAR szName[MAX_ACCOUNT_NAME_SIZE];
	if ( !CAccount::NameStrip(szName, pszName) )
		g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': BAD name\n", pszName);

	m_sName = szName;
	SetPrivLevel((!strnicmp(m_sName, "GUEST", 5) || fGuest) ? PLEVEL_Guest : PLEVEL_Player);
	m_PrivFlags = 0;
	m_MaxChars = 0;
	m_Total_Connect_Time = 0;
	m_Last_Connect_Time = 0;

	g_Accounts.Account_Add(this);
}

void CAccount::DeleteChars()
{
	ADDTOCALLSTACK("CAccount::DeleteChars");
	if ( g_Serv.IsLoading() )
		return;

	size_t i = m_Chars.GetCharCount();
	while ( i > 0 )
	{
		CChar *pChar = m_Chars.GetChar(--i).CharFind();
		if ( pChar )
		{
			pChar->Delete();
			pChar->ClearPlayer();
		}
		m_Chars.DetachChar(i);
	}
}


CAccount::~CAccount()
{
	g_Serv.StatDec( SERV_STAT_ACCOUNTS );

	DeleteChars();
	ClearPasswordTries(true);
}

void CAccount::SetPrivLevel( PLEVEL_TYPE plevel )
{
	ADDTOCALLSTACK("CAccount::SetPrivLevel");
	m_PrivLevel = plevel;	// PLEVEL_Counsel
}

CClient * CAccount::FindClient( const CClient * pExclude ) const
{
	ADDTOCALLSTACK("CAccount::FindClient");

	CClient* pClient = NULL;
	ClientIterator it;
	for (pClient = it.next(); pClient != NULL; pClient = it.next())
	{
		if ( pClient == pExclude )
			continue;
		if ( pClient->m_pAccount == this )
			break;
	}
	return( pClient );
}

bool CAccount::IsMyAccountChar( const CChar * pChar ) const
{
	ADDTOCALLSTACK("CAccount::IsMyAccountChar");
	if ( pChar == NULL )
		return( false );
	if ( pChar->m_pPlayer == NULL )
		return( false );
	return(	pChar->m_pPlayer->m_pAccount == this );
}

size_t CAccount::DetachChar( CChar * pChar )
{
	ADDTOCALLSTACK("CAccount::DetachChar");
	ASSERT( pChar );
	ASSERT( IsMyAccountChar( pChar ));

	if ( m_uidLastChar == pChar->GetUID())
	{
		m_uidLastChar.InitUID();
	}

	return( m_Chars.DetachChar( pChar ));
}

size_t CAccount::AttachChar( CChar * pChar )
{
	ADDTOCALLSTACK("CAccount::AttachChar");
	ASSERT(pChar);
	ASSERT( IsMyAccountChar( pChar ));

	// is it already linked ?
	size_t i = m_Chars.AttachChar( pChar );
	if ( i != m_Chars.BadIndex() )
	{
		size_t iQty = m_Chars.GetCharCount();
		if ( iQty > MAX_CHARS_PER_ACCT )
		{
			g_Log.Event( LOGM_ACCOUNTS|LOGL_ERROR, "Account '%s' has %" FMTSIZE_T " characters\n", GetName(), iQty );
		}
	}

	return( i );
}

void CAccount::TogPrivFlags( WORD wPrivFlags, LPCTSTR pszArgs )
{
	ADDTOCALLSTACK("CAccount::TogPrivFlags");

	if ( pszArgs == NULL || pszArgs[0] == '\0' )	// toggle.
	{
		m_PrivFlags ^= wPrivFlags;
	}
	else if ( Exp_GetVal( pszArgs ))
	{
		m_PrivFlags |= wPrivFlags;
	}
	else
	{
		m_PrivFlags &= ~wPrivFlags;
	}
}

void CAccount::OnLogin( CClient * pClient )
{
	ADDTOCALLSTACK("CAccount::OnLogin");

	ASSERT(pClient);
	pClient->m_timeLogin = CServTime::GetCurrentTime();	// g_World clock of login time. "LASTCONNECTTIME"

	if ( GetPrivLevel() >= PLEVEL_Counsel )	// ON by default.
	{
		m_PrivFlags |= PRIV_GM_PAGE;
	}

	// Get the real world time/date.
	CGTime datetime = CGTime::GetCurrentTime();

	if ( !m_Total_Connect_Time )	// first time - save first ip and timestamp
	{
		m_First_IP = pClient->GetPeer();
		m_dateFirstConnect = datetime;
	}

	m_Last_IP = pClient->GetPeer();
	//m_TagDefs.SetStr("LastLogged", false, m_dateLastConnect.Format(NULL));
	//m_dateLastConnect = datetime;

	if ( pClient->GetConnectType() == CONNECT_TELNET )
	{
		// link the admin client.
		g_Serv.m_iAdminClients++;
	}
	g_Log.Event( LOGM_CLIENTS_LOG, "%lx:Login '%s'\n", pClient->GetSocketID(), GetName());
}

void CAccount::OnLogout(CClient *pClient, bool bWasChar)
{
	ADDTOCALLSTACK("CAccount::OnLogout");
	ASSERT(pClient);

	if ( pClient->GetConnectType() == CONNECT_TELNET )// unlink the admin client.
		g_Serv.m_iAdminClients --;

	// calculate total game time. skip this calculation in
	// case if it was login type packet. it has the same type,
	// so we should check whatever player is attached to a char
	if ( pClient->IsConnectTypePacket() && bWasChar )
	{
		m_Last_Connect_Time = ( -g_World.GetTimeDiff(pClient->m_timeLogin) ) / ( TICK_PER_SEC * 60 );
		if ( m_Last_Connect_Time < 0 )
			m_Last_Connect_Time = 0;

		m_Total_Connect_Time += m_Last_Connect_Time;
	}
}

bool CAccount::Kick( CTextConsole * pSrc, bool fBlock )
{
	ADDTOCALLSTACK("CAccount::Kick");
	if (( GetPrivLevel() >= pSrc->GetPrivLevel()) &&  ( pSrc->GetChar() ) )
	{
		pSrc->SysMessageDefault(DEFMSG_MSG_ACC_PRIV);
		return( false );
	}

	if ( fBlock )
	{
		SetPrivFlags( PRIV_BLOCKED );
		pSrc->SysMessagef( g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_BLOCK), GetName() );
	}

	LPCTSTR pszAction = fBlock ? "KICK" : "DISCONNECT";

	TCHAR * z = Str_GetTemp();
	sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_KICK), GetName(), pszAction, pSrc->GetName());
	g_Log.Event(LOGL_EVENT|LOGM_GM_CMDS, "%s\n", z);
	
	return true;
}


bool CAccount::CheckPasswordTries(CSocketAddress csaPeerName)
{
	int iAccountMaxTries = g_Cfg.m_iClientLoginMaxTries;
	bool bReturn = true;
	DWORD dwCurrentIP = csaPeerName.GetAddrIP();
	CServTime timeCurrent = CServTime::GetCurrentTime();

	BlockLocalTime_t::iterator itData = m_BlockIP.find(dwCurrentIP);
	if ( itData != m_BlockIP.end() )
	{
		BlockLocalTimePair_t itResult = (*itData).second;
		TimeTriesStruct_t & ttsData = itResult.first;
		ttsData.m_Last = timeCurrent.GetTimeRaw();

		if ( ttsData.m_Delay > timeCurrent.GetTimeRaw() )
		{
			bReturn = false;
		}
		else
		{
			if ((( ttsData.m_Last - ttsData.m_First ) > 15*TICK_PER_SEC ) && (itResult.second < iAccountMaxTries))
			{
				ttsData.m_First = timeCurrent.GetTimeRaw();
				ttsData.m_Delay = 0;
				itResult.second = 0;
			}
			else
			{
				itResult.second++;

				if ( itResult.second > iAccountMaxTries )
				{
					ttsData.m_First = ttsData.m_Delay;
					ttsData.m_Delay = 0;
					itResult.second = 0;
				}
				else if ( itResult.second == iAccountMaxTries )
				{
					ttsData.m_Delay = ttsData.m_Last + static_cast<UINT64>(g_Cfg.m_iClientLoginTempBan);
					bReturn = false;
				}
			}
		}

		m_BlockIP[dwCurrentIP] = itResult;
	}
	else
	{
		TimeTriesStruct_t ttsData;
		ttsData.m_First = timeCurrent.GetTimeRaw();
		ttsData.m_Last = timeCurrent.GetTimeRaw();
		ttsData.m_Delay = 0;

		m_BlockIP[dwCurrentIP] = std::make_pair(ttsData, 0);
	}

	if ( m_BlockIP.size() > 100 )
	{
		ClearPasswordTries();
	}

	return bReturn;
}


void CAccount::ClearPasswordTries(bool bAll)
{
	if ( bAll )
	{
		m_BlockIP.clear();
		return;
	}

	UINT64 timeCurrent = CServTime::GetCurrentTime().GetTimeRaw();
	for ( BlockLocalTime_t::iterator itData = m_BlockIP.begin(); itData != m_BlockIP.end(); ++itData )
	{
		BlockLocalTimePair_t itResult = (*itData).second;
		if ( timeCurrent - itResult.first.m_Last > static_cast<UINT64>(g_Cfg.m_iClientLoginTempBan) )
			m_BlockIP.erase(itData);

		if ( itData != m_BlockIP.begin() )
			--itData;
	}
}

bool CAccount::CheckPassword( LPCTSTR pszPassword )
{
	ADDTOCALLSTACK("CAccount::CheckPassword");
	ASSERT(pszPassword);

	if ( m_sCurPassword.IsEmpty() )
	{
		// If account password is empty, set the password given by the client trying to connect
		if ( !SetPassword(pszPassword) )
			return false;
	}
	
	CScriptTriggerArgs Args;
	Args.m_VarsLocal.SetStrNew("Account",GetName());
	Args.m_VarsLocal.SetStrNew("Password",pszPassword);
	TRIGRET_TYPE tr = TRIGRET_RET_FALSE;
	g_Serv.r_Call("f_onaccount_connect", &g_Serv, &Args, NULL, &tr);
	if ( tr == TRIGRET_RET_TRUE )
		return false;
	if ( tr == TRIGRET_RET_HALFBAKED)
		return true;

	// Get the password.
	if( g_Cfg.m_fMd5Passwords )
	{
		// Hash the Password provided by the user and compare the hashes
		char digest[33];
		CMD5::fastDigest( digest, pszPassword );

		if( !strcmpi( digest, GetPassword() ) )
			return( true );
	}
	else
	{
		if ( ! strcmpi( GetPassword(), pszPassword ) )
			return( true );
	}

	if ( ! m_sNewPassword.IsEmpty() && ! strcmpi( GetNewPassword(), pszPassword ))
	{
		// using the new password.
		// kill the old password.
		SetPassword( m_sNewPassword );
		m_sNewPassword.Empty();
		return( true );
	}

	return( false );	// failure.
}

bool CAccount::SetPassword( LPCTSTR pszPassword, bool isMD5Hash )
{
	ADDTOCALLSTACK("CAccount::SetPassword");
	bool useMD5 = g_Cfg.m_fMd5Passwords;
	
	if ( Str_Check( pszPassword ) )	// Prevents exploits
		return false;
	
	//Accounts are 'created' in server startup so we don't fire the function.
	if ( !g_Serv.IsLoading() )
	{
		CScriptTriggerArgs Args;
		Args.Init(GetName());
		Args.m_VarsLocal.SetStrNew("password",pszPassword);
		TRIGRET_TYPE tRet = TRIGRET_RET_FALSE;
		g_Serv.r_Call("f_onaccount_pwchange", &g_Serv, &Args, NULL, &tRet);
		if ( tRet == TRIGRET_RET_TRUE )
		{
			return false;
		}
	}
	size_t enteredPasswordLength = strlen(pszPassword);
	if ( isMD5Hash && useMD5 ) // If it is a hash, check length and set it directly
	{
		if ( enteredPasswordLength == 32 )
			m_sCurPassword = pszPassword;

		return true;
	}
	
	size_t actualPasswordBufferSize = minimum(MAX_ACCOUNT_PASSWORD_ENTER, enteredPasswordLength) + 1;
	char * actualPassword = new char[actualPasswordBufferSize];
	strcpylen(actualPassword, pszPassword, actualPasswordBufferSize);

	if ( useMD5 )
	{
		char digest[33];

		// Auto-Hash if not loading or reloading
		if( !g_Accounts.m_fLoading )
		{
			CMD5::fastDigest( digest, actualPassword );
			m_sCurPassword = digest;
		}

		// Automatically convert the password into a hash if it's coming from the files
		// and shorter or longer than 32 characters
		else
		{
			if( enteredPasswordLength == 32 )
			{
				// password is already in hashed form
				m_sCurPassword = pszPassword;
			}
			else
			{
				// Autoconverted a Password on Load. Print
				g_Log.Event( LOGM_INIT|LOGL_EVENT, "MD5: Converted password for '%s'.\n", GetName() );
				CMD5::fastDigest( digest, actualPassword );
				m_sCurPassword = digest;
			}
		}
	}
	else
	{
		m_sCurPassword = actualPassword;
	}

	delete[] actualPassword;
	return true;
}

// Generate a new password
void CAccount::SetNewPassword( LPCTSTR pszPassword )
{
	ADDTOCALLSTACK("CAccount::SetNewPassword");
	if ( !pszPassword || !pszPassword[0] )		// no password given, auto-generate password
	{
		static TCHAR const passwdChars[] = "ABCDEFGHJKLMNPQRTUVWXYZ2346789";
		size_t len = strlen(passwdChars);
		size_t charsCnt = Calc_GetRandVal(4) + 6;	// 6 - 10 chars
		if ( charsCnt > (MAX_ACCOUNT_PASSWORD_ENTER - 1) )
			charsCnt = MAX_ACCOUNT_PASSWORD_ENTER - 1;

		TCHAR szTmp[MAX_ACCOUNT_PASSWORD_ENTER + 1];
		for ( size_t i = 0; i < charsCnt; ++i )
			szTmp[i] = passwdChars[Calc_GetRandVal(len)];

		szTmp[charsCnt] = '\0';
		m_sNewPassword = szTmp;
		return;
	}

	m_sNewPassword = pszPassword;
	if ( m_sNewPassword.GetLength() > MAX_ACCOUNT_PASSWORD_ENTER )
		m_sNewPassword.SetLength(MAX_ACCOUNT_PASSWORD_ENTER);
}

// Set account RESDISP automatically based on player client version
bool CAccount::SetAutoResDisp(CClient *pClient)
{
	ADDTOCALLSTACK("CAccount::SetAutoResDisp");
	if ( !pClient )
		return false;

	if ( pClient->m_NetState->isClientVersion(MINCLIVER_TOL) )
		return SetResDisp(RDS_TOL);
	else if ( pClient->m_NetState->isClientVersion(MINCLIVER_HS) )
		return SetResDisp(RDS_HS);
	else if ( pClient->m_NetState->isClientVersion(MINCLIVER_SA) )
		return SetResDisp(RDS_SA);
	else if ( pClient->m_NetState->isClientVersion(MINCLIVER_ML) )
		return SetResDisp(RDS_ML);
	else if ( pClient->m_NetState->isClientVersion(MINCLIVER_SE) )
		return SetResDisp(RDS_SE);
	else if ( pClient->m_NetState->isClientVersion(MINCLIVER_AOS) )
		return SetResDisp(RDS_AOS);
	else if ( pClient->m_NetState->isClientVersion(MINCLIVER_LBR) )
		return SetResDisp(RDS_LBR);
	else if ( pClient->m_NetState->isClientVersion(MINCLIVER_T2A) )
		return SetResDisp(RDS_T2A);
	else
		return SetResDisp(RDS_NONE);
}

enum AC_TYPE
{
	AC_ACCOUNT,
	AC_BLOCK,
	AC_CHARS,
	AC_CHARUID,
	AC_CHATNAME,
	AC_FIRSTCONNECTDATE,
	AC_FIRSTIP,
	AC_GUEST,
	AC_JAIL,
	AC_LANG,
	AC_LASTCHARUID,
	AC_LASTCONNECTDATE,
	AC_LASTCONNECTTIME,
	AC_LASTIP,
	AC_MAXCHARS,
	AC_MD5PASSWORD,
	AC_NAME,
	AC_NEWPASSWORD,
	AC_PASSWORD,
	AC_PLEVEL,
	AC_PRIV,
	AC_RESDISP,
	AC_TAG,
	AC_TAG0,
	AC_TAGCOUNT,
	AC_TOTALCONNECTTIME,
	AC_QTY
};

LPCTSTR const CAccount::sm_szLoadKeys[AC_QTY+1] = // static
{
	"ACCOUNT",
	"BLOCK",
	"CHARS",
	"CHARUID",
	"CHATNAME",
	"FIRSTCONNECTDATE",
	"FIRSTIP",
	"GUEST",
	"JAIL",
	"LANG",
	"LASTCHARUID",
	"LASTCONNECTDATE",
	"LASTCONNECTTIME",
	"LASTIP",
	"MAXCHARS",
	"MD5PASSWORD",
	"NAME",
	"NEWPASSWORD",
	"PASSWORD",
	"PLEVEL",
	"PRIV",
	"RESDISP",
	"TAG",
	"TAG0",
	"TAGCOUNT",
	"TOTALCONNECTTIME",
	NULL,
};

bool CAccount::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CAccount::r_GetRef");
	if ( ! strnicmp( pszKey, "CHAR.", 5 ))
	{
		// How many chars.
		pszKey += 5;
		size_t i = Exp_GetVal(pszKey);
		if ( m_Chars.IsValidIndex(i) )
		{
			pRef = m_Chars.GetChar(i).CharFind();
		}
		SKIP_SEPARATORS(pszKey);
		return( true );
	}
	return( CScriptObj::r_GetRef( pszKey, pRef ));
}

bool CAccount::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CAccount::r_WriteVal");
	EXC_TRY("WriteVal");
	if ( !pSrc )
		return false;

	bool	fZero	= false;

	switch ( FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case AC_NAME:
		case AC_ACCOUNT:
			sVal = m_sName;
			break;
		case AC_CHARS:
			sVal.FormatVal( m_Chars.GetCharCount());
			break;
		case AC_BLOCK:
			sVal.FormatVal( IsPriv( PRIV_BLOCKED ));
			break;
		case AC_CHATNAME:
			sVal = m_sChatName;
			break;
		case AC_TAGCOUNT:
			sVal.FormatVal( m_TagDefs.GetCount() );
			break;
		case AC_FIRSTCONNECTDATE:
			sVal = m_dateFirstConnect.Format(NULL);
			break;
		case AC_FIRSTIP:
			sVal = m_First_IP.GetAddrStr();
			break;
		case AC_GUEST:
			sVal.FormatVal( GetPrivLevel() == PLEVEL_Guest );
			break;
		case AC_JAIL:
			sVal.FormatVal( IsPriv( PRIV_JAILED ));
			break;
		case AC_LANG:
			sVal = m_lang.GetStr();
			break;
		case AC_LASTCHARUID:
			sVal.FormatHex( m_uidLastChar );
			break;
		case AC_LASTCONNECTDATE:
			sVal = m_dateLastConnect.Format(NULL);
			break;
		case AC_LASTCONNECTTIME:
			sVal.FormatLLVal( m_Last_Connect_Time );
			break;
		case AC_LASTIP:
			sVal = m_Last_IP.GetAddrStr();
			break;
		case AC_MAXCHARS:
			sVal.FormatVal( GetMaxChars() );
			break;
		case AC_PLEVEL:
			sVal.FormatVal( m_PrivLevel );
			break;
		case AC_NEWPASSWORD:
			if ( pSrc->GetPrivLevel() < PLEVEL_Admin ||
				pSrc->GetPrivLevel() < GetPrivLevel())	// can't see accounts higher than you
			{
				return( false );
			}
			sVal = GetNewPassword();
			break;
		case AC_MD5PASSWORD:
		case AC_PASSWORD:
			if ( pSrc->GetPrivLevel() < PLEVEL_Admin ||
				pSrc->GetPrivLevel() < GetPrivLevel())	// can't see accounts higher than you
			{
				return( false );
			}
			sVal = GetPassword();
			break;
		case AC_PRIV:
			sVal.FormatHex( m_PrivFlags );
			break;
		case AC_RESDISP:
			sVal.FormatVal( m_ResDisp );
			break;
		case AC_TAG0:
			fZero	= true;
			pszKey++;
		case AC_TAG:			// "TAG" = get/set a local tag.
			{
				if ( pszKey[3] != '.' )
					return( false );
				pszKey += 4;
				sVal = m_TagDefs.GetKeyStr(pszKey, fZero );
				return( true );
			}
		case AC_TOTALCONNECTTIME:
			sVal.FormatLLVal( m_Total_Connect_Time );
			break;

		default:
			return( CScriptObj::r_WriteVal( pszKey, sVal, pSrc ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CAccount::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CAccount::r_LoadVal");
	EXC_TRY("LoadVal");

	int i = FindTableHeadSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );
	if ( i < 0 )
	{
		return( false );
	}

	switch ( i )
	{
		case AC_BLOCK:
			if ( ! s.HasArgs() || s.GetArgVal())
			{
				SetPrivFlags( PRIV_BLOCKED );
			}
			else
			{
				ClearPrivFlags( PRIV_BLOCKED );
			}
			break;
		case AC_CHARUID:
			// just ignore this ? chars are loaded later !
			if ( ! g_Serv.IsLoading())
			{
				CGrayUID uid( s.GetArgVal());
				CChar * pChar = uid.CharFind();
				if (pChar == NULL)
				{
					DEBUG_ERR(( "Invalid CHARUID 0%lx for account '%s'\n", static_cast<DWORD>(uid), GetName()));
					return( false );
				}
				if ( ! IsMyAccountChar( pChar ))
				{
					DEBUG_ERR(( "CHARUID 0%lx (%s) not attached to account '%s'\n", static_cast<DWORD>(uid), pChar->GetName(), GetName()));
					return( false );
				}
				AttachChar(pChar);
			}
			break;
		case AC_CHATNAME:
			m_sChatName = s.GetArgStr();
			break;
		case AC_FIRSTCONNECTDATE:
			m_dateFirstConnect.Read( s.GetArgStr());
			break;
		case AC_FIRSTIP:
			m_First_IP.SetAddrStr( s.GetArgStr());
			break;
		case AC_GUEST:
			if ( ! s.HasArgs() || s.GetArgVal())
			{
				SetPrivLevel( PLEVEL_Guest );
			}
			else
			{
				if ( GetPrivLevel() == PLEVEL_Guest )
					SetPrivLevel( PLEVEL_Player );
			}
			break;
		case AC_JAIL:
			if ( ! s.HasArgs() || s.GetArgVal())
			{
				SetPrivFlags( PRIV_JAILED );
			}
			else
			{
				ClearPrivFlags( PRIV_JAILED );
			}
			break;
		case AC_LANG:
			m_lang.Set( s.GetArgStr());
			break;
		case AC_LASTCHARUID:
			m_uidLastChar = static_cast<CGrayUID>(s.GetArgVal());
			break;
		case AC_LASTCONNECTDATE:
			m_dateLastConnect.Read( s.GetArgStr());
			break;
		case AC_LASTCONNECTTIME:
			// Previous total amount of time in game
			m_Last_Connect_Time = s.GetArgLLVal();
			break;
		case AC_LASTIP:
			m_Last_IP.SetAddrStr( s.GetArgStr());
			break;
		case AC_MAXCHARS:
			SetMaxChars( static_cast<BYTE>(s.GetArgVal()) );
			break;
		case AC_MD5PASSWORD:
			SetPassword( s.GetArgStr(), true);
			break;
		case AC_PLEVEL:
			SetPrivLevel( GetPrivLevelText( s.GetArgRaw()));
			break;
		case AC_NEWPASSWORD:
			SetNewPassword( s.GetArgStr());
			break;
		case AC_PASSWORD:
			SetPassword( s.GetArgStr() );
			break;
		case AC_PRIV:
			m_PrivFlags = static_cast<WORD>(s.GetArgVal());
			if ( m_PrivFlags & PRIV_UNUSED )
			{
				g_Log.EventError("Fixing PRIV field (0%hx) for account %s have not supported flags set (caught by mask 0%hx).\n", m_PrivFlags, GetName(), static_cast<WORD>(PRIV_UNUSED));
				m_PrivFlags &= ~PRIV_UNUSED;
			}
			break;
		case AC_RESDISP:
			SetResDisp(static_cast<BYTE>(s.GetArgVal()));
			break;
		case AC_TAG0:
			{
				bool fQuoted = false;
				m_TagDefs.SetStr( s.GetKey()+ 5, fQuoted, s.GetArgStr( &fQuoted ), true );
			}
			return( true );
		case AC_TAG:
			{
				bool fQuoted = false;
				m_TagDefs.SetStr( s.GetKey()+ 4, fQuoted, s.GetArgStr( &fQuoted ));
			}
			return( true );

		case AC_TOTALCONNECTTIME:
			// Previous total amount of time in game
			m_Total_Connect_Time = s.GetArgLLVal();
			break;

		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

void CAccount::r_Write(CScript &s)
{
	ADDTOCALLSTACK_INTENSIVE("CAccount::r_Write");
	if ( GetPrivLevel() >= PLEVEL_QTY )
		return;

	s.WriteSection("%s", static_cast<LPCTSTR>(m_sName));

	if ( GetPrivLevel() != PLEVEL_Player )
	{
		s.WriteKey( "PLEVEL", sm_szPrivLevels[ GetPrivLevel() ] );
	}
	if ( m_PrivFlags )
	{
		s.WriteKeyHex( "PRIV", m_PrivFlags & ~(PRIV_BLOCKED|PRIV_JAILED) );
	}
	if ( GetResDisp() )
	{
		s.WriteKeyVal( "RESDISP", GetResDisp() );
	}
	if ( m_MaxChars > 0 )
	{
		s.WriteKeyVal( "MAXCHARS", m_MaxChars );
	}
	if ( IsPriv( PRIV_JAILED ))
	{
		s.WriteKeyVal( "JAIL", 1 );
	}
	if ( IsPriv( PRIV_BLOCKED ))
	{
		s.WriteKeyVal( "BLOCK", 1 );
	}
	if ( ! m_sCurPassword.IsEmpty())
	{
		s.WriteKey( "PASSWORD", GetPassword() );
	}
	if ( ! m_sNewPassword.IsEmpty())
	{
		s.WriteKey( "NEWPASSWORD", GetNewPassword() );
	}
	if ( m_Total_Connect_Time )
	{
		s.WriteKeyVal( "TOTALCONNECTTIME", m_Total_Connect_Time );
	}
	if ( m_Last_Connect_Time )
	{
		s.WriteKeyVal( "LASTCONNECTTIME", m_Last_Connect_Time );
	}
	if ( m_uidLastChar.IsValidUID())
	{
		s.WriteKeyHex( "LASTCHARUID", m_uidLastChar );
	}

	m_Chars.WritePartyChars(s);

	if ( m_dateFirstConnect.IsTimeValid())
	{
		s.WriteKey( "FIRSTCONNECTDATE", m_dateFirstConnect.Format(NULL));
	}
	if ( m_First_IP.IsValidAddr() )
	{
		s.WriteKey( "FIRSTIP", m_First_IP.GetAddrStr());
	}

	if ( m_dateLastConnect.IsTimeValid())
	{
		s.WriteKey( "LASTCONNECTDATE", m_dateLastConnect.Format(NULL));
	}
	if ( m_Last_IP.IsValidAddr() )
	{
		s.WriteKey( "LASTIP", m_Last_IP.GetAddrStr());
	}
	if ( ! m_sChatName.IsEmpty())
	{
		s.WriteKey( "CHATNAME", static_cast<LPCTSTR>(m_sChatName));
	}
	if ( m_lang.IsDef())
	{
		s.WriteKey( "LANG", m_lang.GetStr());
	}

	// Write New variables
	m_BaseDefs.r_WritePrefix(s);

	m_TagDefs.r_WritePrefix(s, "TAG");
}

enum AV_TYPE
{
	AV_BLOCK,
	AV_DELETE,
	AV_KICK,
	AV_TAGLIST
};

LPCTSTR const CAccount::sm_szVerbKeys[] =
{
	"BLOCK",
	"DELETE",
	"KICK",
	"TAGLIST",
	NULL,
};

bool CAccount::r_Verb( CScript &s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CAccount::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);

	LPCTSTR pszKey = s.GetKey();

	// can't change accounts higher than you in any way
	if (( pSrc->GetPrivLevel() < GetPrivLevel() ) &&  ( pSrc->GetPrivLevel() < PLEVEL_Admin ))
		return false;

	if ( !strnicmp(pszKey, "CLEARTAGS", 9) )
	{
		pszKey = s.GetArgStr();
		SKIP_SEPARATORS(pszKey);
		m_TagDefs.ClearKeys(pszKey);
		return true;
	}

	int i = FindTableSorted( s.GetKey(), sm_szVerbKeys, COUNTOF( sm_szVerbKeys )-1 );
	if ( i < 0 )
	{
		bool bLoad = CScriptObj::r_Verb( s, pSrc );
		if ( !bLoad ) //try calling custom functions
		{
			CGString sVal;
			CScriptTriggerArgs Args( s.GetArgRaw() );
			bLoad = r_Call( pszKey, pSrc, &Args, &sVal );
		}
		return bLoad;
	}

	switch ( i )
	{
		case AV_DELETE: // "DELETE"
			{
				CClient * pClient = FindClient();
				LPCTSTR sCurrentName = GetName();

				if ( pClient )
				{
					pClient->CharDisconnect();
					pClient->m_NetState->markReadClosed();
				}

				char *z = Str_GetTemp();
				sprintf(z, "Account %s deleted.\n", sCurrentName);

				if ( !g_Accounts.Account_Delete(this) )
				{
					sprintf(z, "Cannot delete account %s.\n", sCurrentName);
				}

				g_Log.EventStr(0, z);
				if ( pSrc != &g_Serv )
					pSrc->SysMessage(z);

				return true;
			}
		case AV_BLOCK:
			if ( ! s.HasArgs() || s.GetArgVal())
				SetPrivFlags(PRIV_BLOCKED);
			else
				ClearPrivFlags(PRIV_BLOCKED);
			return true;

		case AV_KICK:
			// if they are online right now then kick them!
			{
				CClient * pClient = FindClient();
				if ( pClient )
				{
					pClient->addKick( pSrc, i == AV_BLOCK );
				}
				else
				{
					Kick( pSrc, i == AV_BLOCK );
				}
			}
			return( true );

		case AV_TAGLIST:
			m_TagDefs.DumpKeys( pSrc, "TAG." );
			return( true );

		default:
			break;
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}
