#include "graysvr.h"
#include "network/network.h"

//**********************************************************************
// -CAccounts

bool CAccounts::Account_Load(LPCTSTR pszNameRaw, CScript & s)
{
	if ( s.HasArgs() && !strcmpi(pszNameRaw, "ACCOUNT") )
	{
		pszNameRaw = s.GetArgStr();
	}

	TCHAR szName[MAX_ACCOUNT_NAME_SIZE];
	if ( !CAccount::NameStrip(szName, pszNameRaw) )
	{
		g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': BAD name\n", pszNameRaw);
		return false;
	}
	m_fLoading = true;

	CAccountRef pAccount = Account_Find(szName);
	if ( pAccount )
	{
		g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': duplicate name\n", pszNameRaw);
		return false;
	}
	else
	{
		pAccount = new CAccount(szName);
	}
	pAccount->r_Load(s);

	m_fLoading = false;

	return true;
}

CAccountRef CAccounts::Account_FindChat( LPCTSTR pszChatName )
{
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
	TCHAR szName[ MAX_ACCOUNT_NAME_SIZE ];

	if ( !CAccount::NameStrip(szName, pszName) )
		return NULL;

	int i = m_Accounts.FindKey(szName);
	if ( i >= 0 )
		return Account_Get(i);

	return NULL;
}

CAccountRef CAccounts::Account_FindCreate( LPCTSTR pszName, bool fAutoCreate )
{
	// Find an account by this name.
	// Create one in some circumstances.

	CAccountRef pAccount = Account_Find(pszName);
	if ( pAccount )
		return pAccount;

	if ( fAutoCreate )	// Create if not found.
	{
		bool fGuest = !strnicmp(pszName, "GUEST", 5);
		TCHAR szName[MAX_ACCOUNT_NAME_SIZE];

		if ( CAccount::NameStrip(szName, pszName) )
			return new CAccount(szName, fGuest);
		else
			g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': BAD name\n", pszName);
	}
	return NULL;
}

bool CAccounts::Account_Delete( CAccount * pAccount )
{
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
	m_Accounts.AddSortKey(pAccount,pAccount->GetName());
}

CAccountRef CAccounts::Account_Get( int index )
{
	if ( index >= m_Accounts.GetCount() )
		return NULL;
	return CAccountRef(static_cast <CAccount *>(m_Accounts[index]));
}

bool CAccounts::Cmd_AddNew( CTextConsole * pSrc, LPCTSTR pszName, LPCTSTR pszArg )
{
	if ( (pszName == NULL) || !strlen(pszName) )
	{
		g_Log.Event(LOGL_ERROR|LOGM_INIT, "Username is required to add an account.\n" );
		return false;
	}

	CAccountRef pAccount = Account_Find( pszName );
	if ( pAccount != NULL )
	{
		pSrc->SysMessagef("Account '%s' already exists\n", pszName);
		return false;
	}
	
	TCHAR szName[ MAX_ACCOUNT_NAME_SIZE ];

	if ( !CAccount::NameStrip(szName, pszName) )
	{
		g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': BAD name\n", pszName);
		return false;
	}

	pAccount = new CAccount(szName);
	pAccount->m_dateFirstConnect = pAccount->m_dateLastConnect = CGTime::GetCurrentTime();

	pAccount->SetPassword(pszArg);
	return true;
}

enum VACS_TYPE
{
	VACS_ADD,
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
	"BLOCKED",
	"HELP",
	"JAILED",
	"UNUSED",
	NULL,
};

bool CAccounts::Cmd_ListUnused(CTextConsole * pSrc, LPCTSTR pszDays, LPCTSTR pszVerb, LPCTSTR pszArgs, DWORD dwMask)
{
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
				pAccount->GetPrivLevel(), pAccount->GetName());
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
	// Modify the accounts on line. "ACCOUNT"
	if ( pSrc->GetPrivLevel() < PLEVEL_Admin )
		return false;

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
		"/ACCOUNT UNUSED days [command]\n",
		"/ACCOUNT ADD name password",
		"/ACCOUNT name BLOCK 0/1\n",
		"/ACCOUNT name JAIL 0/1\n",
		"/ACCOUNT name DELETE = delete all chars and the account\n",
		"/ACCOUNT name PLEVEL x = set account priv level\n",
	};

	switch (index)
	{
	case VACS_ADD:
		return Cmd_AddNew(pSrc, ppCmd[1], ppCmd[2]);

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
		pSrc->SysMessagef("Account '%s': PLEVEL:%d, BLOCK:%d, IP:%s, CONNECTED:%s, ONLINE:%s\n",
			pAccount->GetName(), pAccount->GetPrivLevel(), pAccount->IsPriv(PRIV_BLOCKED),
			pAccount->m_Last_IP.GetAddrStr(), pAccount->m_dateLastConnect.Format(),
			( pClient ? ( pClient->GetChar() ? pClient->GetChar()->GetName() : "<not logged>" ) : "no" ));
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
	// allow just basic chars. No spaces, only numbers, letters and underbar. -+. and single quotes ?
	int len = Str_GetBare(pszNameOut, pszNameInp, MAX_ACCOUNT_NAME_SIZE, " !\"#$%&()*,/:;<=>?@[\\]^{|}~");

	if ( len <= 0 )
		return false;
	if ( strchr(pszNameOut,0x0A) || strchr(pszNameOut,0x0C) || strchr(pszNameOut,0x0D) )
		return false;
	if (( pszNameOut[0] >= '0' ) && ( pszNameOut[0] <= '9' ))
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

CAccount::CAccount( LPCTSTR pszName, bool fGuest )
{
	// Make sure the name is in valid format.
	// Assume the pszName has been stripped of all just !

	g_Serv.StatInc( SERV_STAT_ACCOUNTS );

	TCHAR szName[ MAX_ACCOUNT_NAME_SIZE ];
	if ( !CAccount::NameStrip(szName, pszName) )
	{
		g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': BAD name\n", pszName);
	}
	m_sName = szName;

	if ( !strnicmp(m_sName, "GUEST", 5) || fGuest )
		SetPrivLevel(PLEVEL_Guest);
	else
		SetPrivLevel(PLEVEL_Player);
	
	m_PrivFlags = g_Cfg.m_iAutoPrivFlags;

	m_Total_Connect_Time = 0;
	m_Last_Connect_Time = 0;
	// Add myself to the list.
	g_Accounts.Account_Add( this );
}

void CAccount::DeleteChars()
{
	CClient * pClient = FindClient();
	if ( pClient )
	{	// we have no choice but to kick them.
		pClient->m_net->markClose();
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
}


void CAccount::SetPrivLevel( PLEVEL_TYPE plevel )
{
	m_PrivLevel = plevel;	// PLEVEL_Counsel
}

CClient * CAccount::FindClient( const CClient * pExclude ) const
{
	if ( !this )
		return NULL;

	ClientIterator it;
	CClient *client;
	while ( client = it.next() )
	{
		if (( client != pExclude ) && ( client->GetAccount() == this ))
			break;
	}
	return client;
}

bool CAccount::IsMyAccountChar( const CChar * pChar ) const
{
	// this char is mine ?
	if ( !pChar || !pChar->m_pPlayer )
		return false;
	return (	pChar->m_pPlayer->GetAccount() == this );
}

int CAccount::DetachChar( CChar * pChar )
{
	// unlink the CChar from this CAccount.
	if ( m_uidLastChar == pChar->GetUID() )
	{
		m_uidLastChar.InitUID();
	}
	return m_Chars.DetachChar(pChar);
}

int CAccount::AttachChar(CChar *pChar)
{
	// link the char to this account.
	// is it already linked ?
	int i = m_Chars.AttachChar( pChar );
	if ( i >= 0 )
	{
		int iQty = m_Chars.GetCharCount();
		if ( iQty > MAX_CHARS_PER_ACCT )
		{
			g_Log.Event(LOGM_ACCOUNTS|LOGL_ERROR, "Account '%s' has %d characters\n", GetName(), iQty);
		}
	}

	return i;
}

void CAccount::TogPrivFlags( WORD wPrivFlags, LPCTSTR pszArgs )
{
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
	// The account just logged in.
	pClient->m_timeLogin = CServTime::GetCurrentTime();	// g_World clock of login time. "LASTCONNECTTIME"

	if ( GetPrivLevel() >= PLEVEL_Counsel )	// ON by default.
	{
		m_PrivFlags |= PRIV_GM_PAGE;
	}

	// Get the real world time/date.
	CGTime datetime = CGTime::GetCurrentTime();

	if ( !m_Total_Connect_Time )	// first time - save first ip and timestamp
	{
		m_First_IP = pClient->peer();
		m_dateFirstConnect = datetime;
	}

	m_Last_IP = pClient->peer();
	m_TagDefs.SetStr("LastLogged", false, m_dateLastConnect.Format());
	m_dateLastConnect = datetime;

	if ( pClient->GetConnectType() == CONNECT_TELNET )
	{
		// link the admin client.
		g_Serv.m_iAdminClients++;
	}
	g_Log.Event(LOGM_CLIENTS_LOG, "%x:Login '%s'\n", pClient->socketId(), GetName());
}

void CAccount::OnLogout(CClient *pClient, bool bWasChar)
{
	// CClient is disconnecting from this CAccount.
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
	if ( GetPrivLevel() >= pSrc->GetPrivLevel())
	{
		pSrc->SysMessageDefault(DEFMSG_ACC_PRIV);
		return false;
	}

	if ( fBlock )
	{
		SetPrivFlags( PRIV_BLOCKED );
		pSrc->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_ACC_BLOCK), GetName());
	}

	LPCTSTR pszAction = fBlock ? "KICK" : "DISCONNECT";

	TEMPSTRING(z);
	sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_ACC_KICK), GetName(), pszAction, pSrc->GetName());
	g_Log.Event(LOGL_EVENT|LOGM_GM_CMDS, "%s\n", z);
	
	return true;
}

bool CAccount::CheckPassword( LPCTSTR pszPassword )
{
	// RETURN:
	//  false = failure.
	if ( m_sCurPassword.IsEmpty())
	{
		// First guy in sets the password.
		// check the account in use first.
		if ( *pszPassword == '\0' )
			return false;

		SetPassword( pszPassword );
		return true;
	}

	if ( !strcmpi(GetPassword(), pszPassword) )
	{
		return true;
	}

	if ( ! m_sNewPassword.IsEmpty() && ! strcmpi( GetNewPassword(), pszPassword ))
	{
		// using the new password.
		// kill the old password.
		SetPassword( m_sNewPassword );
		m_sNewPassword.Empty();
		return true;
	}

	return false;	// failure.
}

void CAccount::SetPassword( LPCTSTR pszPassword )
{
    if ( Str_Check(pszPassword) )
        return;

	m_sCurPassword = pszPassword;

	// limit to 16 chars.
	if ( m_sCurPassword.GetLength() > MAX_ACCOUNT_PASSWORD_ENTER )
	{
		m_sCurPassword.SetLength( MAX_ACCOUNT_PASSWORD_ENTER );
	}
}

void CAccount::SetNewPassword( LPCTSTR pszPassword )
{
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
	"TAG",
	"TAG0",
	"TAGCOUNT",
	"TOTALCONNECTTIME",
	NULL,
};

bool CAccount::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
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
		return true;
	}
	return( CScriptObj::r_GetRef( pszKey, pRef ));
}

bool CAccount::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
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
			sVal = m_dateFirstConnect.Format();
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
			sVal = m_dateLastConnect.Format();
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
				return false;
			}
			sVal = GetNewPassword();
			break;
		case AC_PASSWORD:
			if ( pSrc->GetPrivLevel() < PLEVEL_Admin ||
				pSrc->GetPrivLevel() < GetPrivLevel())	// can't see accounts higher than you
			{
				return false;
			}
			sVal = GetPassword();
			break;
		case AC_PRIV:
			sVal.FormatHex( m_PrivFlags );
			break;
		case AC_TAG0:
			fZero	= true;
			pszKey++;
		case AC_TAG:			// "TAG" = get/set a local tag.
			{
				if ( pszKey[3] != '.' )
					return false;
				pszKey += 4;
				sVal = m_TagDefs.GetKeyStr(pszKey, fZero );
				return true;
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
	EXC_TRY("LoadVal");

	int i = FindTableHeadSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );
	if ( i < 0 )
	{
		return false;
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
				UID uid(s.GetArgVal());
				CChar * pChar = uid.CharFind();
				if (pChar == NULL)
				{
					g_Log.Error("Invalid CHARUID 0%x for account '%s'\n", (DWORD)uid, GetName());
					return false;
				}
				if ( !IsMyAccountChar(pChar) )
				{
					g_Log.Error("CHARUID 0%x (%s) not attached to account '%s'\n", (DWORD) uid, pChar->GetName(), GetName());
					return false;
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
			{
				char *args = s.GetArgStr();

				int level;
				if ( isalpha(*args) )
				{
					for ( level = 0; level < COUNTOF(sm_szPrivLevels); level++ )
						if ( !strcmpi(sm_szPrivLevels[level], args) )
							break;
				}
				else if ( isdigit(*args) )
					level = Exp_GetVal(args);
				
				if (( level < 0 ) || ( level >= PLEVEL_QTY ))
					level = PLEVEL_Player;
				SetPrivLevel((PLEVEL_TYPE)level);
			}
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
				g_Log.Error("Fixing PRIV field (0%x) for account %s have not supported flags set (caught by mask 0%x).\n", m_PrivFlags, GetName(), (WORD)PRIV_UNUSED);
				m_PrivFlags &= ~PRIV_UNUSED;
			}
			break;
		case AC_TAG0:
			{
				bool fQuoted = false;
				m_TagDefs.SetStr( s.GetKey()+ 5, fQuoted, s.GetArgStr( &fQuoted ), true );
			}
			return true;
		case AC_TAG:
			{
				bool fQuoted = false;
				m_TagDefs.SetStr( s.GetKey()+ 4, fQuoted, s.GetArgStr( &fQuoted ));
			}
			return true;

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
		s.WriteKey( "FIRSTCONNECTDATE", m_dateFirstConnect.Format());
	}
	if ( m_First_IP.IsValidAddr() )
	{
		s.WriteKey( "FIRSTIP", m_First_IP.GetAddrStr());
	}

	if ( m_dateLastConnect.IsTimeValid())
	{
		s.WriteKey( "LASTCONNECTDATE", m_dateLastConnect.Format());
	}
	if ( m_Last_IP.IsValidAddr() )
	{
		s.WriteKey( "LASTIP", m_Last_IP.GetAddrStr());
	}
	if ( ! m_sChatName.IsEmpty())
	{
		s.WriteKey("CHATNAME", m_sChatName);
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
	EXC_TRY("Verb");
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
					pClient->m_net->markClose();
				}

				TEMPSTRING(z);
				sprintf(z, "Account %s deleted.\n", sCurrentName);

				if ( !g_Accounts.Account_Delete(this) )
				{
					sprintf(z, "Cannot delete account %s.\n", sCurrentName);
				}

				g_Log.Event(LOGM_ACCOUNTS, z);
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
			return true;

		case AV_TAGLIST:
			m_TagDefs.DumpKeys( pSrc, "TAG." );
			return true;
	}
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}
