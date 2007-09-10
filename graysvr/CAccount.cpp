//
// CAccount.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "graysvr.h"	// predef header.
#include "../common/CMD5.h"

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
	// ARGS:
	//   fChanges = false = trap duplicates

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
		ASSERT(pAccount);
	}
	pAccount->r_Load(s);

	m_fLoading = false;

	return true;
}

bool CAccounts::Account_LoadAll( bool fChanges, bool fClearChanges )
{
	ADDTOCALLSTACK("CAccounts::Account_LoadAll");
	// Load the accounts file. (at start up)
	LPCTSTR pszBaseDir;
	LPCTSTR pszBaseName;
	char	*z = Str_GetTemp();

	pszBaseDir = g_Cfg.m_sAcctBaseDir.IsEmpty() ? g_Cfg.m_sWorldBaseDir : g_Cfg.m_sAcctBaseDir;
	pszBaseName = ( fChanges ) ? (GRAY_FILE "acct") : (GRAY_FILE "accu");

	strcpy(z, pszBaseDir);
	strcat(z, pszBaseName);

	CScript s;
	if ( ! s.Open(z, OF_READ|OF_TEXT|OF_DEFAULTMODE| ( fChanges ? OF_NONCRIT : 0) ))
	{
		if ( !fChanges )
		{
			if ( Account_LoadAll(true) )	// if we have changes then we are ok.
				return true;

											//	auto-creating account files
			if ( !Account_SaveAll() )
				g_Log.Event(LOGL_FATAL|LOGM_INIT, "Can't open account file '%s'\n", s.GetFilePath());
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
		s.WriteString( "// Accounts are periodically moved to the " GRAY_FILE "accu" GRAY_SCRIPT " file.\n"
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

	s.Printf("\\\\ " GRAY_TITLE " %s accounts file\n"
		"\\\\ NOTE: This file cannot be edited while the server is running.\n"
		"\\\\ Any file changes must be made to " GRAY_FILE "accu" GRAY_SCRIPT ". This is read in at save time.\n",
		g_Serv.GetName());

	for ( int i=0; i < m_Accounts.GetCount(); i++ )
	{
		CAccountRef pAccount = Account_Get(i);
		if ( pAccount )
			pAccount->r_Write(s);
	}

	Account_LoadAll(true, true);	// clear the change file now.
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_DEBUG_END;
	return false;
}

CAccountRef CAccounts::Account_FindChat( LPCTSTR pszChatName )
{
	ADDTOCALLSTACK("CAccounts::Account_FindChat");
	// IS this new chat name already used ?
	for ( int i=0; i < m_Accounts.GetCount(); i++ )
	{
		CAccountRef pAccount = Account_Get(i);
		if ( pAccount )
			if ( !pAccount->m_sChatName.CompareNoCase(pszChatName) )
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

	int i = m_Accounts.FindKey(szName);
	if ( i >= 0 )
		return Account_Get(i);

	return NULL;
}

CAccountRef CAccounts::Account_FindCreate( LPCTSTR pszName, bool fAutoCreate )
{
	ADDTOCALLSTACK("CAccounts::Account_FindCreate");
	// Find an account by this name.
	// Create one in some circumstances.

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
	m_Accounts.AddSortKey(pAccount,pAccount->GetName());
}

CAccountRef CAccounts::Account_Get( int index )
{
	ADDTOCALLSTACK("CAccounts::Account_Get");
	if ( index >= m_Accounts.GetCount())
		return( NULL );
	return( CAccountRef( STATIC_CAST <CAccount *>( m_Accounts[index])));
}

bool CAccounts::Cmd_AddNew( CTextConsole * pSrc, LPCTSTR pszName, LPCTSTR pszArg, bool md5 )
{
	ADDTOCALLSTACK("CAccounts::Cmd_AddNew");
	if ( (pszName == NULL) || !strlen(pszName) )
	{
		g_Log.Event(LOGL_ERROR|LOGM_INIT, "Username is required to add an account.\n" );
		return false;
	}

	CAccountRef pAccount = Account_Find( pszName );
	if ( pAccount != NULL )
	{
		pSrc->SysMessagef( "Account '%s' already exists\n", (LPCTSTR) pszName );
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

	pAccount->SetPassword(pszArg, !md5);
	return true;
}

enum VACS_TYPE
{
	VACS_ADD,
	VACS_ADDMD5,
	VACS_BLOCKED,
	VACS_HELP,
	VACS_JAILED,
	VACS_UNUSED,
	VACS_UPDATE,
	VACS_QTY,
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
	// do something to all the unused accounts.
	int iDaysTest = Exp_GetVal(pszDays);

	CGTime datetime = CGTime::GetCurrentTime();
	int iDaysCur = datetime.GetDaysTotal();

	int		iCountOrig	= Account_GetCount();
	int		iCountCheck	= iCountOrig;
	int		iCount		= 0;
	for ( int i=0; true; i++ )
	{
		if ( Account_GetCount() < iCountCheck )
		{
			iCountCheck--;
			i--;
		}
		CAccountRef pAccount = Account_Get(i);
		if ( !pAccount ) break;

		int iDaysAcc = pAccount->m_dateLastConnect.GetDaysTotal();
		if ( ! iDaysAcc )
		{
			// account has never been used ? (get create date instead)
			iDaysAcc = pAccount->m_dateFirstConnect.GetDaysTotal();
		}

		if ( iDaysCur - iDaysAcc < iDaysTest ) continue;
		if ( dwMask && !pAccount->IsPriv(dwMask) ) continue;

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
			pSrc->SysMessagef( "Can't Delete PrivLevel %d Account '%s' this way.\n",
				pAccount->GetPrivLevel(), (LPCTSTR) pAccount->GetName() );
		}
		else
		{
			CScript script( pszVerb, pszArgs );
			pAccount->r_Verb( script, pSrc );
		}
	}

	pSrc->SysMessagef( "Matched %d of %d accounts unused for %d days\n",
		iCount, iCountOrig, iDaysTest );

	if ( pszVerb && ! strcmpi(pszVerb, "DELETE") )
	{
		int	iDeleted = iCountOrig - Account_GetCount();

		if ( iDeleted < iCount )
			pSrc->SysMessagef("%d deleted, %d cleared of characters (must try to delete again)\n",
				iDeleted, iCount - iDeleted );
		else if ( iDeleted > 0 )
			pSrc->SysMessagef("All %d unused accounts deleted.\n", iDeleted);
		else
			pSrc->SysMessagef("No accounts deleted.\n", iDeleted);
	}

	return true;
}

bool CAccounts::Account_OnCmd( TCHAR * pszArgs, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CAccounts::Account_OnCmd");
	// Modify the accounts on line. "ACCOUNT"
	ASSERT( pSrc );

	if ( pSrc->GetPrivLevel() < PLEVEL_Admin )
		return( false );

	TCHAR * ppCmd[5];
	int iQty = Str_ParseCmds( pszArgs, ppCmd, COUNTOF( ppCmd ));

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
		"/ACCOUNT name EXPORT filename\n",
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
				for ( int i=0; i<COUNTOF(sm_pszCmds); i++ )
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

		char	*z = Str_GetTemp();
		sprintf(z, "Account '%s': PLEVEL:%d, BLOCK:%d, IP:%s, CONNECTED:%s, ONLINE:%s\n",
			pAccount->GetName(), pAccount->GetPrivLevel(), pAccount->IsPriv(PRIV_BLOCKED),
			pAccount->m_Last_IP.GetAddrStr(), pAccount->m_dateLastConnect.Format(NULL),
			( pClient ? ( pClient->GetChar() ? pClient->GetChar()->GetName() : "<not logged>" ) : "no" ));
		pSrc->SysMessage(z);
		return true;
	}
	else
	{
		CScript script( ppCmd[1], ppCmd[2] );
		return pAccount->r_Verb( script, pSrc );
	}
}

//**********************************************************************
// -CAccount

bool CAccount::NameStrip( TCHAR * pszNameOut, LPCTSTR pszNameInp )
{
	ADDTOCALLSTACK("CAccount::NameStrip");
	// allow just basic chars. No spaces, only numbers, letters and underbar. -+. and single quotes ?

	int iLen = Str_GetBare(pszNameOut, pszNameInp, MAX_ACCOUNT_NAME_SIZE, " !\"#$%&()*,/:;<=>?@[\\]^{|}~");

	if ( iLen <= 0 )
		return false;
	if ( strchr(pszNameOut,0x0A) || strchr(pszNameOut,0x0C) || strchr(pszNameOut,0x0D) )
		return false;
	if ( !strcmpi(pszNameOut, "EOF") || !strcmpi(pszNameOut, "ACCOUNT") )
		return false;
	if ( FindTableSorted(pszNameOut, g_Accounts.sm_szVerbKeys, COUNTOF(g_Accounts.sm_szVerbKeys)-1) >= 0 )
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
	NULL,
};

PLEVEL_TYPE CAccount::GetPrivLevelText( LPCTSTR pszFlags ) // static
{
	ADDTOCALLSTACK("CAccount::GetPrivLevelText");
	int level = FindTable( pszFlags, sm_szPrivLevels, COUNTOF(sm_szPrivLevels)-1 );
	if ( level >= 0 )
	{
		return( (PLEVEL_TYPE) level );
	}

	if ( ! strnicmp( pszFlags, "CO", 2 ))
		return PLEVEL_Counsel;

	level = Exp_GetVal( pszFlags );
	if ( level < 0 || level > PLEVEL_QTY )
		return( PLEVEL_Player );

	return( (PLEVEL_TYPE) level );
}

CAccount::CAccount( LPCTSTR pszName, bool fGuest )
{
	// Make sure the name is in valid format.
	// Assume the pszName has been stripped of all just !

	g_Serv.StatInc( SERV_STAT_ACCOUNTS );

	TCHAR szName[ MAX_ACCOUNT_NAME_SIZE ];
	if ( !CAccount::NameStrip( szName, pszName ) )
	{
		g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': BAD name\n", pszName);
	}
	m_sName = szName;

	if ( !strnicmp(m_sName, "GUEST", 5) || fGuest )
		SetPrivLevel(PLEVEL_Guest);
	else
		SetPrivLevel(PLEVEL_Player);
	
	m_ResDisp = g_Cfg.m_iAutoResDisp;
	m_PrivFlags = g_Cfg.m_iAutoPrivFlags;

	m_Total_Connect_Time = 0;
	m_Last_Connect_Time = 0;
	// Add myself to the list.
	g_Accounts.Account_Add( this );
}

void CAccount::DeleteChars()
{
	ADDTOCALLSTACK("CAccount::DeleteChars");
	CClient * pClient = FindClient();
	if ( pClient )
	{	// we have no choice but to kick them.
		pClient->m_fClosed	= true;
	}

	// Now track down all my disconnected chars !
	if ( ! g_Serv.IsLoading())
	{
		m_Chars.DeleteChars();
	}
}


CAccount::~CAccount()
{
	// We should go track down and delete all the chars and clients that use this account !

	g_Serv.StatDec( SERV_STAT_ACCOUNTS );

	DEBUG_WARN(( "Account delete '%s'\n", GetName() ));
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
	// Is the account logged in.
	if ( this == NULL )
		return( NULL );	// this might be possible.
	CClient * pClient = g_Serv.GetClientHead();
	for ( ; pClient!=NULL; pClient = pClient->GetNext())
	{
		if ( pClient == pExclude )
			continue;
		if ( this == pClient->GetAccount())
			break;
	}
	return( pClient );
}

bool CAccount::IsMyAccountChar( const CChar * pChar ) const
{
	ADDTOCALLSTACK("CAccount::IsMyAccountChar");
	// this char is mine ?
	if ( pChar == NULL )
		return( false );
	if ( pChar->m_pPlayer == NULL )
		return( false );
	return(	pChar->m_pPlayer->GetAccount() == this );
}

int CAccount::DetachChar( CChar * pChar )
{
	ADDTOCALLSTACK("CAccount::DetachChar");
	// unlink the CChar from this CAccount.
	ASSERT( pChar );
	ASSERT( IsMyAccountChar( pChar ));

	if ( m_uidLastChar == pChar->GetUID())
	{
		m_uidLastChar.InitUID();
	}

	return( m_Chars.DetachChar( pChar ));
}

int CAccount::AttachChar( CChar * pChar )
{
	ADDTOCALLSTACK("CAccount::AttachChar");
	// link the char to this account.
	ASSERT(pChar);
	ASSERT( IsMyAccountChar( pChar ));

	// is it already linked ?
	int i = m_Chars.AttachChar( pChar );
	if ( i >= 0 )
	{
		int iQty = m_Chars.GetCharCount();
		if ( iQty > MAX_CHARS_PER_ACCT )
		{
			g_Log.Event( LOGM_ACCOUNTS|LOGL_ERROR, "Account '%s' has %d characters\n", (LPCTSTR) GetName(), iQty );
		}
	}

	return( i );
}

void CAccount::TogPrivFlags( WORD wPrivFlags, LPCTSTR pszArgs )
{
	ADDTOCALLSTACK("CAccount::TogPrivFlags");
	// s.GetArgFlag
	// No args = toggle the flag.
	// 1 = set the flag.
	// 0 = clear the flag.

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
	// The account just logged in.

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
		m_First_IP = pClient->m_PeerName;
		m_dateFirstConnect = datetime;
	}

	m_Last_IP = pClient->m_PeerName;
	m_TagDefs.SetStr("LastLogged", false, m_dateLastConnect.Format(NULL));
	m_dateLastConnect = datetime;

	if ( pClient->GetConnectType() == CONNECT_TELNET )
	{
		// link the admin client.
		g_Serv.m_iAdminClients++;
	}
	g_Log.Event( LOGM_CLIENTS_LOG, "%x:Login '%s'\n", pClient->m_Socket.GetSocket(), (LPCTSTR) GetName());
}

void CAccount::OnLogout(CClient *pClient, bool bWasChar)
{
	ADDTOCALLSTACK("CAccount::OnLogout");
	// CClient is disconnecting from this CAccount.
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
	if ( GetPrivLevel() >= pSrc->GetPrivLevel())
	{
		pSrc->SysMessageDefault(DEFMSG_ACC_PRIV);
		return( false );
	}

	if ( fBlock )
	{
		SetPrivFlags( PRIV_BLOCKED );
		pSrc->SysMessagef( g_Cfg.GetDefaultMsg(DEFMSG_ACC_BLOCK), (LPCTSTR) GetName() );
	}

	LPCTSTR pszAction = fBlock ? "KICK" : "DISCONNECT";

	char *z = Str_GetTemp();
	sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_ACC_KICK), GetName(), pszAction, pSrc->GetName());
	g_Log.Event(LOGL_EVENT|LOGM_GM_CMDS, "%s\n", z);
	
	return true;
}

bool CAccount::CheckPasswordTries(CSocketAddress csaPeerName)
{
	if ( csaPeerName.IsLocalAddr() || (csaPeerName.GetAddrIP() == 0x7F000001) )
	{
		return true;
	}

	int iAccountMaxTries = g_Cfg.m_iMaxAccountLoginTries;
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
					ttsData.m_Delay = ttsData.m_Last + (2*60*TICK_PER_SEC);
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

		m_BlockIP[dwCurrentIP] = make_pair(ttsData, 0);
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
	}
	else
	{
		long timeCurrent = CServTime::CServTime().GetTimeRaw();

		for ( BlockLocalTime_t::iterator itData = m_BlockIP.begin(); itData != m_BlockIP.end(); ++itData )
		{
			BlockLocalTimePair_t itResult = (*itData).second;
			if (( timeCurrent - itResult.first.m_Last ) > 3*60*TICK_PER_SEC )
			{
				m_BlockIP.erase(itData);
			}

			if ( itData != m_BlockIP.begin() )
			{
				--itData;
			}
		}
	}
}

bool CAccount::CheckPassword( LPCTSTR pszPassword )
{
	ADDTOCALLSTACK("CAccount::CheckPassword");
	// RETURN:
	//  false = failure.
	ASSERT(pszPassword);

	if ( m_sCurPassword.IsEmpty())
	{
		// First guy in sets the password.
		// check the account in use first.
		if ( *pszPassword == '\0' )
			return( false );

		SetPassword( pszPassword );
		return( true );
	}

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

bool CAccount::SetPassword( LPCTSTR pszPassword, bool dontUseMD5Config)
{
	ADDTOCALLSTACK("CAccount::SetPassword");

	//dontUseMD5Config allows to disable the hashing of the password (false means "use sphere.ini")
	bool useMD5;
	if (dontUseMD5Config)  
		useMD5 = false;
	else	
		useMD5 = g_Cfg.m_fMd5Passwords;
	
	if ( Str_Check( pszPassword ) )
		return false;

	short int iSize = minimum(MAX_ACCOUNT_PASSWORD_ENTER,strlen(pszPassword))+1;
	char * pszPassw = new char[iSize];
	strcpylen(pszPassw,pszPassword,iSize);

	if( useMD5 )
	{
		char digest[33];

		// Auto-Hash if not loading or reloading
		if( !g_Accounts.m_fLoading )
		{
			CMD5::fastDigest( digest, pszPassw );
			m_sCurPassword = digest;
		}

		// Automatically convert the password into a hash if it's coming from the files
		// and shorter or longer than 32 characters
		else
		{
			if( strlen( pszPassword ) == 32 )
				m_sCurPassword = pszPassword;
			else
			{
				// Autoconverted a Password on Load. Print
				g_Log.Event( LOGM_INIT|LOGL_EVENT, "MD5: Converted password for '%s'.\n", GetName() );
				CMD5::fastDigest( digest, pszPassw );
				m_sCurPassword = digest;
			}
		}
	}
	else
		m_sCurPassword = pszPassw;
	delete[] pszPassw;
	return true;
}

void CAccount::SetNewPassword( LPCTSTR pszPassword )
{
	ADDTOCALLSTACK("CAccount::SetNewPassword");
	if ( !pszPassword || !pszPassword[0] )		// no password given, auto-generate password
	{
		static char const passwdChars[] = "ABCDEFGHJKLMNPQRTUVWXYZ2346789";
		int	len = strlen(passwdChars);
		int charsCnt = Calc_GetRandVal(4) + 6;	// 6 - 10 chars
		if ( charsCnt > MAX_ACCOUNT_PASSWORD_ENTER-1 )
			charsCnt = MAX_ACCOUNT_PASSWORD_ENTER-1;
		char	szTmp[MAX_ACCOUNT_PASSWORD_ENTER + 1];

		for ( int i = 0; i < charsCnt; i++ )
			szTmp[i] = passwdChars[Calc_GetRandVal(len)];

		szTmp[charsCnt] = 0;
		m_sNewPassword = szTmp;
		return;
	}

	m_sNewPassword = pszPassword;
	if ( m_sNewPassword.GetLength() > MAX_ACCOUNT_PASSWORD_ENTER )
		m_sNewPassword.SetLength(MAX_ACCOUNT_PASSWORD_ENTER);
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
	AC_QTY,
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
		int i=Exp_GetVal(pszKey);
		if ( i>=0 && i<m_Chars.GetCharCount())
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
			sVal.FormatVal( m_Last_Connect_Time );
			break;
		case AC_LASTIP:
			sVal = m_Last_IP.GetAddrStr();
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
			sVal.FormatVal( m_Total_Connect_Time );
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
					DEBUG_ERR(( "Invalid CHARUID 0%x for account '%s'\n", (DWORD) uid, (LPCTSTR) GetName()));
					return( false );
				}
				if ( ! IsMyAccountChar( pChar ))
				{
					DEBUG_ERR(( "CHARUID 0%x (%s) not attached to account '%s'\n", (DWORD) uid, (LPCTSTR) pChar->GetName(), (LPCTSTR) GetName()));
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
			m_uidLastChar = s.GetArgVal();
			break;
		case AC_LASTCONNECTDATE:
			m_dateLastConnect.Read( s.GetArgStr());
			break;
		case AC_LASTCONNECTTIME:
			// Previous total amount of time in game
			m_Last_Connect_Time = s.GetArgVal();
			break;
		case AC_LASTIP:
			m_Last_IP.SetAddrStr( s.GetArgStr());
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
			m_PrivFlags = s.GetArgVal();
			if ( m_PrivFlags & PRIV_UNUSED )
			{
				g_Log.EventError("Fixing PRIV field (0%x) for account %s have not supported flags set (caught by mask 0%x).\n", m_PrivFlags, GetName(), (WORD)PRIV_UNUSED);
				m_PrivFlags &= ~PRIV_UNUSED;
			}
			break;
		case AC_RESDISP:
			SetResDisp(s.GetArgVal());
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
			m_Total_Connect_Time = s.GetArgVal();
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
	ADDTOCALLSTACK("CAccount::r_Write");
	if ( GetPrivLevel() >= PLEVEL_QTY )
		return;

	s.WriteSection(m_sName);

	if ( GetPrivLevel() != PLEVEL_Player )
	{
		s.WriteKey( "PLEVEL", sm_szPrivLevels[ GetPrivLevel() ] );
	}
	if ( m_PrivFlags != PRIV_DETAIL )
	{
		s.WriteKeyHex( "PRIV", m_PrivFlags &~( PRIV_BLOCKED | PRIV_JAILED ));
	}
	if ( GetResDisp() != g_Cfg.m_iAutoResDisp )
	{
		s.WriteKeyVal( "RESDISP", GetResDisp() );
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
		s.WriteKey( "CHATNAME", (LPCTSTR) m_sChatName );
	}
	if ( m_lang.IsDef())
	{
		s.WriteKey( "LANG", m_lang.GetStr());
	}

	m_TagDefs.r_WritePrefix(s, "TAG");
}

enum AV_TYPE
{
	AV_BLOCK,
	AV_DELETE,
	AV_KICK,
	AV_TAGLIST,
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
		return( CScriptObj::r_Verb( s, pSrc ));
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
					pClient->m_fClosed = true;
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
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}
