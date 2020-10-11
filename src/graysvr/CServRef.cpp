#include "graysvr.h"	// predef header

// Memory profiling
#ifdef _WIN32
	#include <psapi.h>

	typedef	BOOL(WINAPI *pGetProcessMemoryInfo)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
	pGetProcessMemoryInfo m_GetProcessMemoryInfo = NULL;
	HMODULE m_hModule = NULL;
#else
	#include <sys/resource.h>
#endif
bool m_fProcessInfoAvailable = true;

#define SPHERE_DEF_PORT		2593

CServerDef::CServerDef(LPCTSTR pszName, CSocketAddressIP dwIP) : m_ip(dwIP, SPHERE_DEF_PORT)	// SOCKET_LOCAL_ADDRESS
{
	memset(m_dwStat, 0, sizeof(m_dwStat));	// THIS MUST BE FIRST
	SetName(pszName);
	m_timeCreate = CServTime::GetCurrentTime();
	m_TimeZone = static_cast<signed char>(_timezone / (60 * 60));
}

DWORD CServerDef::StatGet(SERV_STAT_TYPE i) const
{
	ADDTOCALLSTACK("CServerDef::StatGet");
	ASSERT((i >= SERV_STAT_CLIENTS) && (i <= SERV_STAT_QTY));

	DWORD d = m_dwStat[i];
	EXC_TRY("StatGet");
	if ( i == SERV_STAT_MEM )	// memory usage information
	{
		d = 0;
		if ( m_fProcessInfoAvailable )
		{
#ifdef _WIN32
			if ( !m_hModule )	// load psapi.dll if not loaded yet
			{
				EXC_SET("load process info");

				TCHAR szLibPath[MAX_PATH];
				GetSystemDirectory(szLibPath, sizeof(szLibPath));
				strcat(szLibPath, "\\psapi.dll");

				m_hModule = LoadLibrary(szLibPath);
				if ( !m_hModule )
				{
					m_fProcessInfoAvailable = false;
					g_Log.EventError(("Unable to load process information API (psapi.dll), memory information will be not available\n"));
				}
				else
					m_GetProcessMemoryInfo = reinterpret_cast<pGetProcessMemoryInfo>(GetProcAddress(m_hModule, "GetProcessMemoryInfo"));
			}

			if ( m_GetProcessMemoryInfo )
			{
				EXC_SET("open process");
				HANDLE hProcess = GetCurrentProcess();
				PROCESS_MEMORY_COUNTERS pmc;
				if ( hProcess )
				{
					ASSERT(hProcess == NOFILE_HANDLE);
					EXC_SET("get memory info");
					if ( m_GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)) )
					{
						EXC_SET("read memory info");
						d = pmc.WorkingSetSize;
					}
					CloseHandle(hProcess);
				}
			}
#else
			struct rusage usage;
			int res = getrusage(RUSAGE_SELF, &usage);

			if ( (res == 0) && usage.ru_idrss )
				d = usage.ru_idrss;
			else
			{
				CFileText inf;
				TCHAR *buf = Str_GetTemp(), *head;
				sprintf(buf, "/proc/%d/status", getpid());
				if ( inf.Open(buf, OF_READ|OF_TEXT) )
				{
					for (;;)
					{
						if ( !inf.ReadString(buf, SCRIPT_MAX_LINE_LEN) )
							break;

						if ( (head = strstr(buf, "VmSize:")) != NULL )
						{
							head += 7;
							GETNONWHITESPACE(head);
							d = ATOI(head) * 1000;
							break;
						}
					}
					inf.Close();
				}
			}

			if ( !d )
			{
				g_Log.EventError(("Unable to load process information from getrusage() and procfs, memory information will be not available\n"));
				m_fProcessInfoAvailable = false;
			}
#endif

			if ( d != 0 )
				d /= 1024;
		}
	}
	return d;

	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.EventDebug("stat '%d', val '%" FMTDWORD "'\n", i, d);
	EXC_DEBUG_END;
	return 0;
}

void CServerDef::SetName(LPCTSTR pszName)
{
	ADDTOCALLSTACK("CServerDef::SetName");
	if ( !pszName )
		return;

	TCHAR szName[MAX_SERVER_NAME_SIZE];
	size_t iLen = Str_GetBare(szName, pszName, sizeof(szName), "<>/\"\\");
	if ( iLen <= 0 )
		return;

	m_sName = szName;
#ifdef _WIN32
	// Update console window title
	NTWindow_SetWindowTitle();
#endif
}

enum SC_TYPE
{
	SC_ACCOUNTS,
	SC_ADMINEMAIL,
	SC_AGE,
	SC_CHARS,
	SC_CLIENTS,
	SC_CLIENTVERSION,
	SC_CREATE,
	SC_GMPAGES,
	SC_ITEMS,
	SC_LANG,
	SC_MEM,
	SC_NAME,
	SC_RESTAPIPUBLICIP,
	SC_SERVIP,
	SC_SERVNAME,
	SC_SERVPORT,
	SC_TIMEZONE,
	SC_URL,
	SC_VERSION,
	SC_QTY
};

const LPCTSTR CServerDef::sm_szLoadKeys[SC_QTY + 1] =	// static
{
	"ACCOUNTS",
	"ADMINEMAIL",
	"AGE",
	"CHARS",
	"CLIENTS",
	"CLIENTVERSION",
	"CREATE",
	"GMPAGES",
	"ITEMS",
	"LANG",
	"MEM",
	"NAME",
	"RESTAPIPUBLICIP",
	"SERVIP",
	"SERVNAME",
	"SERVPORT",
	"TIMEZONE",
	"URL",
	"VERSION",
	NULL
};

bool CServerDef::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CServerDef::r_LoadVal");
	EXC_TRY("LoadVal");
	switch ( FindTableSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case SC_ADMINEMAIL:
			m_sEMail = s.GetArgStr();
			break;
		case SC_CLIENTVERSION:
			m_ClientVersion.SetClientVer(s.GetArgRaw());
			break;
		case SC_LANG:
			m_sLang = s.GetArgStr();
			break;
		case SC_SERVIP:
			m_ip.SetHostPortStr(s.GetArgStr());
			break;
		case SC_NAME:
		case SC_SERVNAME:
			SetName(s.GetArgStr());
			break;
		case SC_RESTAPIPUBLICIP:
			m_sRestAPIPublicIP = s.GetArgStr();
			break;
		case SC_SERVPORT:
			m_ip.SetPortStr(s.GetArgStr());
			break;
		case SC_TIMEZONE:
			m_TimeZone = static_cast<signed char>(s.GetArgVal());
			break;
		case SC_URL:
			m_sURL = s.GetArgStr();
			break;
		case SC_ACCOUNTS:
		case SC_AGE:
		case SC_CHARS:
		case SC_CLIENTS:
		case SC_CREATE:
		case SC_ITEMS:
			return false;	// read-only
		default:
			return CScriptObj::r_LoadVal(s);
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CServerDef::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CServerDef::r_WriteVal");
	EXC_TRY("WriteVal");
	switch ( FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case SC_ACCOUNTS:
			sVal.FormatUVal(StatGet(SERV_STAT_ACCOUNTS));
			break;
		case SC_ADMINEMAIL:
			sVal = m_sEMail;
			break;
		case SC_AGE:
			sVal.FormatLLVal(GetAge());
			break;
		case SC_CHARS:
			sVal.FormatUVal(StatGet(SERV_STAT_CHARS));
			break;
		case SC_CLIENTS:
			sVal.FormatUVal(StatGet(SERV_STAT_CLIENTS));
			break;
		case SC_CLIENTVERSION:
		{
			TCHAR szVersion[128];
			sVal = m_ClientVersion.WriteClientVerString(m_ClientVersion.GetClientVer(), szVersion);
			break;
		}
		case SC_CREATE:
			sVal.FormatLLVal(-g_World.GetTimeDiff(m_timeCreate) / TICK_PER_SEC);
			break;
		case SC_GMPAGES:
			sVal.FormatUVal(g_World.m_GMPages.GetCount());
			break;
		case SC_ITEMS:
			sVal.FormatUVal(StatGet(SERV_STAT_ITEMS));
			break;
		case SC_LANG:
			sVal = m_sLang;
			break;
		case SC_MEM:
			sVal.FormatUVal(StatGet(SERV_STAT_MEM));
			break;
		case SC_NAME:
		case SC_SERVNAME:
			sVal = GetName();
			break;
		case SC_RESTAPIPUBLICIP:
			sVal = m_sRestAPIPublicIP;
			break;
		case SC_SERVIP:
			sVal = m_ip.GetAddrStr();
			break;
		case SC_SERVPORT:
			sVal.FormatUVal(m_ip.GetPort());
			break;
		case SC_TIMEZONE:
			sVal.FormatVal(m_TimeZone);
			break;
		case SC_URL:
			sVal = m_sURL;
			break;
		case SC_VERSION:
			sVal = SPHERE_VER_STR_FULL;
			break;
		default:
		{
			LPCTSTR pszArgs = strchr(pszKey, ' ');
			if ( pszArgs )
				GETNONWHITESPACE(pszArgs);

			CScriptTriggerArgs Args(pszArgs ? pszArgs : "");
			if ( r_Call(pszKey, pSrc, &Args, &sVal) )
				return true;

			return CScriptObj::r_WriteVal(pszKey, sVal, pSrc);
		}
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

INT64 CServerDef::GetAge() const
{
	ADDTOCALLSTACK("CServerDef::GetAge");
	// Get server age (days since m_timeCreate)
	return -g_World.GetTimeDiff(m_timeCreate) / (TICK_PER_SEC * 60 * 60 * 24);
}
