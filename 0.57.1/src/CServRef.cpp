#include "graysvr.h"
#include "../src/common/version.h"
#include "quest.h"

//	Memory profiling
#ifdef _WIN32	// (Win32)
	#include <process.h>

	//	grabbed from platform SDK, psapi.h
	typedef struct _PROCESS_MEMORY_COUNTERS {
		DWORD	doesnotmatter[3];
		DWORD WorkingSetSize;
		DWORD	doesnotmatteralso[6];
	} PROCESS_MEMORY_COUNTERS, *PPROCESS_MEMORY_COUNTERS;
	BOOL WINAPI GetProcessMemoryInfo(HANDLE Process, PPROCESS_MEMORY_COUNTERS ppsmemCounters, DWORD cb);

	//	PSAPI external definitions
	typedef	BOOL (WINAPI *pGetProcessMemoryInfo)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
	HMODULE	m_hmPsapiDll = NULL;
	pGetProcessMemoryInfo m_GetProcessMemoryInfo = NULL;
#else			// (Unix)
	#include <sys/resource.h>
#endif
	bool	m_bPmemory = true;		// process memory information is available?

//////////////////////////////////////////////////////////////////////
// -CServerDef

CServerDef::CServerDef( LPCTSTR pszName, CSocketAddressIP dwIP ) :
	m_ip( dwIP, SPHERE_PORT )
{
	// Statistics.
	memset(m_dwStat, 0, sizeof(m_dwStat));

	SetName(pszName);
	m_timeCreate = CServTime::GetCurrentTime();

	// Set default time zone from UTC
	m_TimeZone = (int)_timezone/(60*60);	// GMT
}

DWORD CServerDef::StatGet(SERV_STAT_TYPE i) const
{
	DWORD	d = m_dwStat[i];
	if ( i == SERV_STAT_MEM )	// memory information
	{
		d = 0;
		if ( m_bPmemory )
		{
#ifdef _WIN32
			if ( !m_hmPsapiDll )			// try to load psapi.dll if not loaded yet
			{
				if ( !(m_hmPsapiDll = LoadLibrary("psapi.dll")) )
				{
					m_bPmemory = false;
					g_Log.Error(("Unable to load process information PSAPI.DLL library. Memory information will be not available.\n"));
				}
				else
					m_GetProcessMemoryInfo = (pGetProcessMemoryInfo)::GetProcAddress(m_hmPsapiDll,"GetProcessMemoryInfo");
			}

			if ( m_GetProcessMemoryInfo )
			{
				HANDLE	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, _getpid());
				if ( hProcess )
				{
					PROCESS_MEMORY_COUNTERS	pcnt;
					if ( m_GetProcessMemoryInfo(hProcess, &pcnt, sizeof(pcnt)) == TRUE )
						d = pcnt.WorkingSetSize;

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
				TCHAR		*head;

				TEMPSTRING(buf);
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
				g_Log.Error("Unable to load process information from getrusage() and procfs. Memory information will be not available.\n");
				m_bPmemory = false;
			}
#endif

			if ( d != 0 )
				d /= 1024;
		}
	}
	return d;
}

void CServerDef::SetName(LPCTSTR pszName)
{
	if ( pszName )
	{
		char z[2*MAX_SERVER_NAME_SIZE];
		if ( !Str_GetBare(z, pszName, sizeof(z), "<>/\"\\") )
			return;
		m_sName = z;
	}
}

void CServerDef::addToServersList( CCommand & cmd, int index, int j, bool bReverse ) const
{
	// Add myself to the server list.
	cmd.ServerList.m_serv[j].m_count = index;

	//pad zeros to length.
	strcpylen( cmd.ServerList.m_serv[j].m_servname, GetName(), sizeof(cmd.ServerList.m_serv[j].m_servname));

	cmd.ServerList.m_serv[j].m_percentfull = min(StatGet(SERV_STAT_CLIENTS), 100);
	cmd.ServerList.m_serv[j].m_timezone = m_TimeZone;

	DWORD dwAddr = m_ip.GetAddrIP();
	if ( bReverse )
	{
		// Clients less than 4.0.0 require IP to be sent in reverse
		cmd.ServerList.m_serv[j].m_ip[3] = ( dwAddr >> 24 ) & 0xFF;
		cmd.ServerList.m_serv[j].m_ip[2] = ( dwAddr >> 16 ) & 0xFF;
		cmd.ServerList.m_serv[j].m_ip[1] = ( dwAddr >> 8  ) & 0xFF;
		cmd.ServerList.m_serv[j].m_ip[0] = ( dwAddr       ) & 0xFF;
	}
	else
	{
		// Clients since 4.0.0 require IP to be sent in order
		cmd.ServerList.m_serv[j].m_ip[0] = ( dwAddr >> 24 ) & 0xFF;
		cmd.ServerList.m_serv[j].m_ip[1] = ( dwAddr >> 16 ) & 0xFF;
		cmd.ServerList.m_serv[j].m_ip[2] = ( dwAddr >> 8  ) & 0xFF;
		cmd.ServerList.m_serv[j].m_ip[3] = ( dwAddr       ) & 0xFF;
	}
}

enum SC_TYPE
{
	SC_ACCOUNTS,
	SC_ADMINEMAIL,
	SC_AGE,
	SC_CHARS,
	SC_CLIENTS,
	SC_CREATE,
	SC_ITEMS,
	SC_LANG,
	SC_MEM,
	SC_NAME,
	SC_QUESTS,
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
	"ACCOUNTS",
	"ADMINEMAIL",
	"AGE",
	"CHARS",
	"CLIENTS",
	"CREATE",
	"ITEMS",
	"LANG",
	"MEM",
	"NAME",
	"QUESTS",
	"SERVIP",
	"SERVNAME",
	"SERVPORT",
	"TIMEZONE",
	"URL",			// m_sURL
	"URLLINK",
	"VERSION",
	NULL,
};

bool CServerDef::r_LoadVal( CScript & s )
{
	EXC_TRY("LoadVal");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case SC_AGE:
			break;
		case SC_CREATE:
			m_timeCreate = CServTime::GetCurrentTime() + ( s.GetArgVal() * TICK_PER_SEC );
			break;
		case SC_ADMINEMAIL:
			if ( this != &g_Serv && !g_Serv.m_sEMail.IsEmpty() && strstr(s.GetArgStr(), g_Serv.m_sEMail) )
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
					return false;
				m_sLang = szLang;
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
	EXC_TRY("WriteVal");
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	case SC_ADMINEMAIL:
		sVal = m_sEMail;
		break;
	case SC_AGE:
		// display the age in days.
		sVal.FormatVal( GetAgeHours()/24 );
		break;
	case SC_CREATE:
		sVal.FormatVal( g_World.GetTimeDiff(m_timeCreate) / TICK_PER_SEC );
		break;
	case SC_LANG:
		sVal = m_sLang;
		break;
	case SC_QUESTS:
		sVal.FormatVal(g_Quests.size());
		break;
	case SC_SERVIP:
		sVal = m_ip.GetAddrStr();
		break;
	case SC_NAME:
	case SC_SERVNAME:
		sVal = GetName();	// What the name should be. Fill in from ping.
		break;
	case SC_SERVPORT:
		sVal.FormatVal(m_ip.GetPort());
		break;
	case SC_ACCOUNTS:
		sVal.FormatVal(StatGet(SERV_STAT_ACCOUNTS));
		break;
	case SC_CLIENTS:
		sVal.FormatVal(StatGet(SERV_STAT_CLIENTS));
		break;
	case SC_ITEMS:
		sVal.FormatVal(StatGet(SERV_STAT_ITEMS));
		break;
	case SC_MEM:
		sVal.FormatVal( StatGet( SERV_STAT_MEM ) );
		break;
	case SC_CHARS:
		sVal.FormatVal(StatGet(SERV_STAT_CHARS));
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
		sVal.Format("<a href=\"http://%s\">%s</a>", m_sURL, GetName());
		break;
	case SC_VERSION:
		sVal = SPHERE_VERSION;
		break;
	default:
		CScriptTriggerArgs Args( pszKey );
		if ( r_Call( pszKey, pSrc, &Args, &sVal ) )
			return true;
		return( CScriptObj::r_WriteVal( pszKey, sVal, pSrc ));
	}
	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

int CServerDef::GetAgeHours() const
{
	// This is just the amount of time it has been listed.
	return(( - g_World.GetTimeDiff( m_timeCreate )) / ( TICK_PER_SEC * 60 * 60 ));
}
