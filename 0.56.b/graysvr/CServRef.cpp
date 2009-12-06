//
// CServerDef.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "graysvr.h"	// predef header.
#include "../common/grayver.h"

//	Memory profiling
#ifdef _WIN32	// (Win32)
	#include <process.h>

	//	grabbed from platform SDK, psapi.h
	typedef struct _PROCESS_MEMORY_COUNTERS {
		DWORD cb;
		DWORD PageFaultCount;
		SIZE_T PeakWorkingSetSize;
		SIZE_T WorkingSetSize;
		SIZE_T QuotaPeakPagedPoolUsage;
		SIZE_T QuotaPagedPoolUsage;
		SIZE_T QuotaPeakNonPagedPoolUsage;
		SIZE_T QuotaNonPagedPoolUsage;
		SIZE_T PagefileUsage;
		SIZE_T PeakPagefileUsage;
	} PROCESS_MEMORY_COUNTERS, *PPROCESS_MEMORY_COUNTERS;

	//	PSAPI external definitions
	typedef	BOOL (WINAPI *pGetProcessMemoryInfo)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
	HMODULE	m_hmPsapiDll = NULL;
	pGetProcessMemoryInfo m_GetProcessMemoryInfo = NULL;
	PROCESS_MEMORY_COUNTERS	pcnt;
#else			// (Unix)
	#include <sys/resource.h>
#endif
	bool	m_bPmemory = true;		// process memory information is available?

//////////////////////////////////////////////////////////////////////
// -CServerDef

CServerDef::CServerDef( LPCTSTR pszName, CSocketAddressIP dwIP ) :
	m_ip( dwIP, GRAY_DEF_PORT )	// SOCKET_LOCAL_ADDRESS
{
	// Statistics.
	memset( m_dwStat, 0, sizeof( m_dwStat ));	// THIS MUST BE FIRST !

	SetName( pszName );
	m_timeLastValid.Init();
	m_timeCreate = CServTime::GetCurrentTime();

	// Set default time zone from UTC
	m_TimeZone = (int)_timezone / (60*60);	// Greenwich mean time.
	m_eAccApp = ACCAPP_Unspecified;
}

DWORD CServerDef::StatGet(SERV_STAT_TYPE i) const
{
	ADDTOCALLSTACK("CServerDef::StatGet");
	ASSERT( i>=0 && i<=SERV_STAT_QTY );
	DWORD	d = m_dwStat[i];
	EXC_TRY("StatGet");
	if ( i == SERV_STAT_MEM )	// memory information
	{
		d = 0;
		if ( m_bPmemory )
		{
#ifdef _WIN32
			if ( !m_hmPsapiDll )			// try to load psapi.dll if not loaded yet
			{
				EXC_SET("load process info");
				if ( !(m_hmPsapiDll = LoadLibrary("psapi.dll")) )
				{
					m_bPmemory = false;
					g_Log.EventError(("Unable to load process information PSAPI.DLL library. Memory information will be not available.\n"));
				}
				else
					m_GetProcessMemoryInfo = (pGetProcessMemoryInfo)::GetProcAddress(m_hmPsapiDll,"GetProcessMemoryInfo");
			}

			if ( m_GetProcessMemoryInfo ) {
				EXC_SET("open process");
				HANDLE hProcess = GetCurrentProcess();
				if ( hProcess ) {
					ASSERT( hProcess == (HANDLE)-1 );
					EXC_SET("get memory info");
					if ( m_GetProcessMemoryInfo(hProcess, &pcnt, sizeof(pcnt)) ) {
						EXC_SET("read memory info");
						d = pcnt.WorkingSetSize;
					}
					CloseHandle(hProcess);
				}
			}
#else
			struct rusage usage;
			int res = getrusage(RUSAGE_SELF, &usage);

			if ( usage.ru_idrss )
				d = usage.ru_idrss;
			else
			{
				CFileText	inf;
				TCHAR		*buf = Str_GetTemp(), *head;

				sprintf(buf, "/proc/%d/status", getpid());
				if ( inf.Open(buf, OF_READ|OF_TEXT) )
				{
					while ( true )
					{
						if ( !inf.ReadString(buf, SCRIPT_MAX_LINE_LEN) )
							break;

						if ( head = strstr(buf, "VmSize:") )
						{
							head += 7;
							GETNONWHITESPACE(head)
							d = ATOI(head) * 1000;
							break;
						}
					}
					inf.Close();
				}
			}

			if ( !d )
			{
				g_Log.EventError(("Unable to load process information from getrusage() and procfs. Memory information will be not available.\n"));
				m_bPmemory = false;
			}
#endif

			if ( d != 0 )
				d /= 1024;
		}
	}
	return d;

	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.EventDebug("stat '%d', val '%d'\n", i, d);
	EXC_DEBUG_END;
	return 0;
}

void CServerDef::SetName( LPCTSTR pszName )
{
	ADDTOCALLSTACK("CServerDef::SetName");
	if ( ! pszName )
		return;

	// No HTML tags using <> either.
	TCHAR szName[ 2*MAX_SERVER_NAME_SIZE ];
	int len = Str_GetBare( szName, pszName, sizeof(szName), "<>/\"\\" );
	if ( ! len )
		return;

	// allow just basic chars. No spaces, only numbers, letters and underbar.
	if ( g_Cfg.IsObscene( szName ))
	{
		DEBUG_ERR(( "Obscene server '%s' ignored.\n", szName ));
		return;
	}

	m_sName = szName;
}

void CServerDef::SetValidTime()
{
	ADDTOCALLSTACK("CServerDef::SetValidTime");
	m_timeLastValid = CServTime::GetCurrentTime();
}

int CServerDef::GetTimeSinceLastValid() const
{
	ADDTOCALLSTACK("CServerDef::GetTimeSinceLastValid");
	return( - g_World.GetTimeDiff( m_timeLastValid ));
}

enum SC_TYPE
{
	SC_ACCAPP,
	SC_ACCAPPS,
	SC_ACCOUNTS,
	SC_ADMINEMAIL,
	SC_AGE,
	SC_CHARS,
	SC_CLIENTS,
	SC_CLIENTVERSION,
	SC_CREATE,
	SC_ITEMS,
	SC_LANG,
	SC_LASTVALIDDATE,
	SC_LASTVALIDTIME,
	SC_MEM,
	SC_NAME,
	SC_SERVIP,
	SC_SERVNAME,
	SC_SERVPORT,
	SC_TIMEZONE,
	SC_URL,			// m_sURL
	SC_URLLINK,
	SC_VERSION,
	SC_QTY,
};

LPCTSTR const CServerDef::sm_szLoadKeys[SC_QTY+1] =	// static
{
	"ACCAPP",
	"ACCAPPS",
	"ACCOUNTS",
	"ADMINEMAIL",
	"AGE",
	"CHARS",
	"CLIENTS",
	"CLIENTVERSION",
	"CREATE",
	"ITEMS",
	"LANG",
	"LASTVALIDDATE",
	"LASTVALIDTIME",
	"MEM",
	"NAME",
	"SERVIP",
	"SERVNAME",
	"SERVPORT",
	"TIMEZONE",
	"URL",			// m_sURL
	"URLLINK",
	"VERSION",
	NULL,
};

static LPCTSTR const sm_AccAppTable[ ACCAPP_QTY ] =
{
	"Closed",		// Closed. Not accepting more.
	"Unused",
	"Free",			// Anyone can just log in and create a full account.
	"GuestAuto",	// You get to be a guest and are automatically sent email with u're new password.
	"GuestTrial",	// You get to be a guest til u're accepted for full by an Admin.
	"Unused",
	"Unspecified",	// Not specified.
	"Unused",
	"Unused",
};

bool CServerDef::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CServerDef::r_LoadVal");
	EXC_TRY("LoadVal");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case SC_ACCAPP:
		case SC_ACCAPPS:
			// Treat it as a value or a string.
			if ( IsDigit( s.GetArgStr()[0] ))
			{
				m_eAccApp = (ACCAPP_TYPE) s.GetArgVal();
			}
			else
			{
				// Treat it as a string. "Manual","Automatic","Guest"
				m_eAccApp = (ACCAPP_TYPE)FindTable(s.GetArgStr(), sm_AccAppTable, COUNTOF(sm_AccAppTable));
			}
			if ( m_eAccApp < 0 || m_eAccApp >= ACCAPP_QTY )
				m_eAccApp = ACCAPP_Unspecified;
			break;
		case SC_AGE:
			break;
		case SC_CLIENTVERSION:
			m_sClientVersion = s.GetArgRaw();
			// m_ClientVersion.SetClientVer( s.GetArgRaw());
			break;
		case SC_CREATE:
			m_timeCreate = CServTime::GetCurrentTime() + ( s.GetArgVal() * TICK_PER_SEC );
			break;
		case SC_ADMINEMAIL:
			if ( this != &g_Serv && !g_Serv.m_sEMail.IsEmpty() && strstr(s.GetArgStr(), g_Serv.m_sEMail) )
				return false;
			if ( !g_Cfg.IsValidEmailAddressFormat(s.GetArgStr()) )
				return false;
			if ( g_Cfg.IsObscene(s.GetArgStr()) )
				return false;
			m_sEMail = s.GetArgStr();
			break;
		case SC_LANG:
			{
				TCHAR szLang[ 32 ];
				int len = Str_GetBare( szLang, s.GetArgStr(), sizeof(szLang), "<>/\"\\" );
				if ( g_Cfg.IsObscene(szLang))	// Is the name unacceptable?
					return( false );
				m_sLang = szLang;
			}
			break;
		case SC_LASTVALIDDATE:
			m_dateLastValid.Read( s.GetArgStr());
			break;
		case SC_LASTVALIDTIME:
			{
				int iVal = s.GetArgVal() * TICK_PER_SEC;
				if ( iVal < 0 )
					m_timeLastValid = CServTime::GetCurrentTime() + iVal;
				else
					m_timeLastValid = CServTime::GetCurrentTime() - iVal;
			}
			break;
		case SC_SERVIP:
			m_ip.SetHostPortStr( s.GetArgStr());
			break;

		case SC_NAME:
		case SC_SERVNAME:
			SetName( s.GetArgStr());
			break;
		case SC_SERVPORT:
			m_ip.SetPort( s.GetArgVal());
			break;

		case SC_ACCOUNTS:
			SetStat( SERV_STAT_ACCOUNTS, s.GetArgVal());
			break;

		case SC_CLIENTS:
			{
				int iClients = s.GetArgVal();
				if ( iClients < 0 )
					return( false );	// invalid
				if ( iClients > FD_SETSIZE )	// Number is bugged !
					return( false );
				SetStat( SERV_STAT_CLIENTS, iClients );
			}
			break;
		case SC_ITEMS:
			SetStat( SERV_STAT_ITEMS, s.GetArgVal());
			break;
		case SC_CHARS:
			SetStat( SERV_STAT_CHARS, s.GetArgVal());
			break;
		case SC_TIMEZONE:
			m_TimeZone = s.GetArgVal();
			break;
		case SC_URL:
		case SC_URLLINK:
			// It is a basically valid URL ?
			if ( this != &g_Serv )
			{
				if ( !g_Serv.m_sURL.IsEmpty() && strstr(s.GetArgStr(), g_Serv.m_sURL) )
					return false;
			}
			if ( !strchr(s.GetArgStr(), '.' ) )
				return false;
			if ( g_Cfg.IsObscene(s.GetArgStr()) )	// Is the name unacceptable?
				return false;
			m_sURL = s.GetArgStr();
			break;
		default:
			return ( CScriptObj::r_LoadVal(s));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CServerDef::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CServerDef::r_WriteVal");
	EXC_TRY("WriteVal");
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	case SC_ACCAPP:
		sVal.FormatVal( m_eAccApp );
		break;
	case SC_ACCAPPS:
		// enum string
		ASSERT( m_eAccApp >= 0 && m_eAccApp < ACCAPP_QTY );
		sVal = sm_AccAppTable[ m_eAccApp ];
		break;
	case SC_ADMINEMAIL:
		sVal = m_sEMail;
		break;
	case SC_AGE:
		// display the age in days.
		sVal.FormatVal( GetAgeHours()/24 );
		break;
	case SC_CLIENTVERSION:
		{
			TCHAR szVersion[ 128 ];
			sVal = m_ClientVersion.WriteClientVer( szVersion );
		}
		break;
	case SC_CREATE:
		sVal.FormatVal( g_World.GetTimeDiff(m_timeCreate) / TICK_PER_SEC );
		break;
	case SC_LANG:
		sVal = m_sLang;
		break;

	case SC_LASTVALIDDATE:
		if ( m_timeLastValid.IsTimeValid() )
			sVal.FormatVal( GetTimeSinceLastValid() / ( TICK_PER_SEC * 60 ));
		else
			sVal = "NA";
		break;
	case SC_LASTVALIDTIME:
		// How many seconds ago.
		sVal.FormatVal( m_timeLastValid.IsTimeValid() ? ( GetTimeSinceLastValid() / TICK_PER_SEC ) : -1 );
		break;
	case SC_SERVIP:
		sVal = m_ip.GetAddrStr();
		break;
	case SC_NAME:
	case SC_SERVNAME:
		sVal = GetName();	// What the name should be. Fill in from ping.
		break;
	case SC_SERVPORT:
		sVal.FormatVal( m_ip.GetPort());
		break;
	case SC_ACCOUNTS:
		sVal.FormatVal( StatGet( SERV_STAT_ACCOUNTS ));
		break;
	case SC_CLIENTS:
		sVal.FormatVal( StatGet( SERV_STAT_CLIENTS ));
		break;
	case SC_ITEMS:
		sVal.FormatVal( StatGet( SERV_STAT_ITEMS ));
		break;
	case SC_MEM:
		sVal.FormatVal( StatGet( SERV_STAT_MEM ) );
		break;
	case SC_CHARS:
		sVal.FormatVal( StatGet( SERV_STAT_CHARS ));
		break;
	case SC_TIMEZONE:
		sVal.FormatVal( m_TimeZone );
		break;
	case SC_URL:
		sVal = m_sURL;
		break;
	case SC_URLLINK:
		// try to make a link of it.
		if ( m_sURL.IsEmpty())
		{
			sVal = GetName();
			break;
		}
		sVal.Format( "<a href=\"http://%s\">%s</a>", (LPCTSTR) m_sURL, (LPCTSTR) GetName() );
		break;
	case SC_VERSION:
		sVal = GRAY_VERSION;
		break;
	default:
		{
			LPCTSTR pszArgs = strchr(pszKey, ' ');
			if (pszArgs != NULL)
				GETNONWHITESPACE(pszArgs);

			CScriptTriggerArgs Args( pszArgs ? pszArgs : "" );
			if ( r_Call( pszKey, pSrc, &Args, &sVal ) )
				return true;

			return( CScriptObj::r_WriteVal( pszKey, sVal, pSrc ));
		}
	}
	return true;
	EXC_CATCH("ServerDef");

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

int CServerDef::GetAgeHours() const
{
	ADDTOCALLSTACK("CServerDef::GetAgeHours");
	// This is just the amount of time it has been listed.
	return(( - g_World.GetTimeDiff( m_timeCreate )) / ( TICK_PER_SEC * 60 * 60 ));
}
