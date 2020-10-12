#include "CDataBase.h"
#include "../sphere/asyncdb.h"

extern CDataBaseAsyncHelper g_asyncHdb;

CDataBase::CDataBase()
{
	m_wTickCount = 0;
	m_socket = NULL;
}

CDataBase::~CDataBase()
{
	if ( m_socket )
		Close();
}

void CDataBase::Connect()
{
	ADDTOCALLSTACK("CDataBase::Connect");
	SimpleThreadLock lock(m_connectionMutex);
	if ( m_socket )
		return;

#ifdef _WIN32
	if ( mysql_get_client_version() < LIBMYSQL_VERSION_ID )
		g_Log.EventWarn("Your MySQL client library %s is outdated. For better compatibility, update your 'libmysql.dll' file to version %s\n", mysql_get_client_info(), LIBMYSQL_VERSION);
#else
	if ( mysql_get_client_version() < MYSQL_VERSION_ID )
		g_Log.EventWarn("Your MySQL client library %s is outdated. For better compatibility, update your 'libmysqlclient' (Ubuntu) / 'mysql-community-libs' (CentOS / Red Hat) package to version %s\n", mysql_get_client_info(), MYSQL_SERVER_VERSION);
#endif

	m_socket = mysql_init(NULL);
	if ( !m_socket )
	{
		g_Log.EventError("Insufficient memory to initialize MySQL client socket\n");
		return;
	}

	const char *pszUser = g_Cfg.m_sMySqlUser;
	const char *pszPassword = g_Cfg.m_sMySqlPass;
	const char *pszDB = g_Cfg.m_sMySqlDB;
	const char *pszHost = g_Cfg.m_sMySqlHost;
	unsigned int uiPort = MYSQL_PORT;

	// If user define server port using hostname:port format, split values into different variables
	const char *pszArgs = strchr(pszHost, ':');
	if ( pszArgs )
	{
		char *pszTemp = Str_GetTemp();
		strncpy(pszTemp, pszHost, HOSTNAME_LENGTH - 1);
		pszTemp[HOSTNAME_LENGTH - 1] = '\0';
		*(strchr(pszTemp, ':')) = '\0';
		uiPort = ATOI(pszArgs + 1);
		pszHost = pszTemp;
	}

	if ( mysql_real_connect(m_socket, pszHost, pszUser, pszPassword, pszDB, uiPort, NULL, CLIENT_MULTI_STATEMENTS) )
	{
		if ( mysql_get_server_version(m_socket) < MYSQL_VERSION_ID )
			g_Log.EventWarn("Your MySQL server %s is outdated. For better compatibility, update your MySQL server to version %s\n", mysql_get_server_info(m_socket), MYSQL_SERVER_VERSION);
	}
	else
	{
		g_Log.EventError("MySQL error #%u: %s\n", mysql_errno(m_socket), mysql_error(m_socket));
		mysql_close(m_socket);
		m_socket = NULL;
	}
}

void CDataBase::Close()
{
	ADDTOCALLSTACK("CDataBase::Close");
	SimpleThreadLock lock(m_connectionMutex);
	mysql_close(m_socket);
	m_socket = NULL;
}

bool CDataBase::Query(LPCTSTR pszQuery, CVarDefMap &mapQueryResult)
{
	ADDTOCALLSTACK("CDataBase::Query");
	mapQueryResult.Empty();
	mapQueryResult.SetNumNew("NUMROWS", 0);

	if ( !m_socket )
		return false;

	// Connection can only handle one query at a time, so lock the thread until the query finishes
	SimpleThreadLock lock(m_connectionMutex);

	int iResultCode = mysql_query(m_socket, pszQuery);
	if ( iResultCode == 0 )
	{
		MYSQL_RES *pResult = mysql_store_result(m_socket);
		if ( !pResult )
			return false;

		unsigned int uiNumFields = mysql_num_fields(pResult);
		char key[64];
		if ( uiNumFields > COUNTOF(key) )
		{
			// This check is not really needed, MySQL client should be able to handle the same columns amount of MySQL server (4096).
			// But since this value is too big and create an 4096 char array at -every- query call is not performance-friendly, maybe
			// it's better just use an smaller array to prioritize performance.
			g_Log.EventError("MySQL query returned too many columns [Max: %" FMTSIZE_T " / Cmd: \"%s\"]\n", COUNTOF(key), pszQuery);
			mysql_free_result(pResult);
			return false;
		}

		MYSQL_FIELD *pFields = mysql_fetch_fields(pResult);
		mapQueryResult.SetNum("NUMROWS", mysql_num_rows(pResult));
		mapQueryResult.SetNum("NUMCOLS", uiNumFields);

		int iRowNum = 0;
		char **pRow = NULL;
		char *pszKey = Str_GetTemp();
		char *pszVal = NULL;
		while ( (pRow = mysql_fetch_row(pResult)) != NULL )
		{
			for ( unsigned int i = 0; i < uiNumFields; ++i )
			{
				pszVal = pRow[i];
				if ( !iRowNum )
				{
					mapQueryResult.SetStr(ITOA(i, key, 10), true, pszVal);
					mapQueryResult.SetStr(pFields[i].name, true, pszVal);
				}

				snprintf(pszKey, 24, "%d.%u", iRowNum, i);
				mapQueryResult.SetStr(pszKey, true, pszVal);
				snprintf(pszKey, 76, "%d.%s", iRowNum, pFields[i].name);
				mapQueryResult.SetStr(pszKey, true, pszVal);
			}
			++iRowNum;
		}

		mysql_free_result(pResult);
		return true;
	}
	else
	{
		g_Log.EventError("MySQL error #%u: %s [Cmd: \"%s\"]\n", mysql_errno(m_socket), mysql_error(m_socket), pszQuery);
		if ( (iResultCode == CR_SERVER_GONE_ERROR) || (iResultCode == CR_SERVER_LOST) )
			Close();

		return false;
	}
}

bool __cdecl CDataBase::Queryf(CVarDefMap &mapQueryResult, char *pFormat, ...)
{
	ADDTOCALLSTACK("CDataBase::Queryf");
	TemporaryString pszBuffer;
	va_list marker;

	va_start(marker, pFormat);
	_vsnprintf(pszBuffer, pszBuffer.realLength(), pFormat, marker);
	va_end(marker);

	return Query(pszBuffer, mapQueryResult);
}

bool CDataBase::Exec(LPCTSTR pszQuery)
{
	ADDTOCALLSTACK("CDataBase::Exec");
	if ( !m_socket )
		return false;

	// Connection can only handle one query at a time, so lock the thread until the query finishes
	SimpleThreadLock lock(m_connectionMutex);

	int iResultCode = mysql_query(m_socket, pszQuery);
	if ( iResultCode == 0 )
	{
		// Result must be retrieved from server even when no data is expected, otherwise the server will think the client has lost connection
		MYSQL_RES *pResult = mysql_store_result(m_socket);
		if ( !pResult )
			return false;

		mysql_free_result(pResult);
		return true;
	}
	else
	{
		g_Log.EventError("MySQL error #%u: %s [Cmd: \"%s\"]\n", mysql_errno(m_socket), mysql_error(m_socket), pszQuery);
		if ( (iResultCode == CR_SERVER_GONE_ERROR) || (iResultCode == CR_SERVER_LOST) )
			Close();

		return false;
	}
}

bool __cdecl CDataBase::Execf(char *pFormat, ...)
{
	ADDTOCALLSTACK("CDataBase::Execf");
	TemporaryString pszBuffer;
	va_list marker;

	va_start(marker, pFormat);
	_vsnprintf(pszBuffer, pszBuffer.realLength(), pFormat, marker);
	va_end(marker);

	return Exec(pszBuffer);
}

bool CDataBase::AsyncQueue(bool fQuery, LPCTSTR pszFunction, LPCTSTR pszQuery)
{
	ADDTOCALLSTACK("CDataBase::AsyncQueue");
	if ( !g_Cfg.m_Functions.ContainsKey(pszFunction) )
	{
		g_Log.EventError("Invalid %s callback function '%s'\n", fQuery ? "AQUERY" : "AEXECUTE", pszFunction);
		return false;
	}

	if ( !g_asyncHdb.isActive() )
		g_asyncHdb.start();

	g_asyncHdb.addQuery(fQuery, pszFunction, pszQuery);
	return true;
}

void CDataBase::AsyncQueueCallback(CGString &sFunction, CScriptTriggerArgs *pArgs)
{
	ADDTOCALLSTACK("CDataBase::AsyncQueueCallback");
	SimpleThreadLock lock(m_resultMutex);

	m_QueryArgs.push(FunctionArgsPair_t(sFunction, pArgs));
}

void CDataBase::OnTick()
{
	ADDTOCALLSTACK("CDataBase::OnTick");
	if ( !g_Cfg.m_bMySql )
		return;

	// Periodically check if connection still active
	if ( ++m_wTickCount >= 1000 )
	{
		m_wTickCount = 0;
		if ( m_socket )
		{
			SimpleThreadLock lock(m_connectionMutex);
			if ( mysql_ping(m_socket) != 0 )
			{
				g_Log.EventError("MySQL server connection has been lost. Trying to reconnect...\n");
				Close();
				Connect();
			}
		}
	}

	if ( !m_QueryArgs.empty() && !(m_wTickCount % TICK_PER_SEC) )
	{
		SimpleThreadLock lock(m_resultMutex);
		FunctionArgsPair_t currentPair = m_QueryArgs.front();
		m_QueryArgs.pop();

		g_Serv.r_Call(currentPair.first, &g_Serv, currentPair.second);
		ASSERT(currentPair.second != NULL);
		delete currentPair.second;
	}
}

enum DBO_TYPE
{
	DBO_AEXECUTE,
	DBO_AQUERY,
	DBO_CONNECTED,
	DBO_ESCAPEDATA,
	DBO_ROW,
	DBO_QTY
};

const LPCTSTR CDataBase::sm_szLoadKeys[DBO_QTY + 1] =
{
	"AEXECUTE",
	"AQUERY",
	"CONNECTED",
	"ESCAPEDATA",
	"ROW",
	NULL
};

enum DBOV_TYPE
{
	DBOV_CLOSE,
	DBOV_CONNECT,
	DBOV_EXECUTE,
	DBOV_QUERY,
	DBOV_QTY
};

const LPCTSTR CDataBase::sm_szVerbKeys[DBOV_QTY + 1] =
{
	"CLOSE",
	"CONNECT",
	"EXECUTE",
	"QUERY",
	NULL
};

bool CDataBase::r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef)
{
	ADDTOCALLSTACK("CDataBase::r_GetRef");
	UNREFERENCED_PARAMETER(pszKey);
	UNREFERENCED_PARAMETER(pRef);
	return false;
}

bool CDataBase::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CDataBase::r_LoadVal");
	UNREFERENCED_PARAMETER(s);
	return false;
}

bool CDataBase::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CDataBase::r_WriteVal");
	UNREFERENCED_PARAMETER(pSrc);
	if ( !g_Cfg.m_bMySql )
	{
		sVal.FormatVal(0);
		return true;
	}

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	switch ( index )
	{
		case DBO_AEXECUTE:
		case DBO_AQUERY:
		{
			pszKey += strlen(sm_szLoadKeys[index]);
			GETNONWHITESPACE(pszKey);

			TCHAR *ppArgs[2];
			if ( (pszKey[0] != '\0') && (Str_ParseCmds(const_cast<TCHAR *>(pszKey), ppArgs, COUNTOF(ppArgs)) == 2) )
				sVal.FormatVal(AsyncQueue((index == DBO_AQUERY), ppArgs[0], ppArgs[1]));
			else
			{
				g_Log.EventError("Invalid %s arguments\n", CDataBase::sm_szLoadKeys[index]);
				sVal.FormatVal(0);
			}
			return true;
		}
		case DBO_CONNECTED:
		{
			sVal.FormatVal(m_socket != NULL);
			return true;
		}
		case DBO_ESCAPEDATA:
		{
			pszKey += strlen(sm_szLoadKeys[index]);
			GETNONWHITESPACE(pszKey);
			sVal = "";

			if ( pszKey[0] != '\0' )
			{
				TCHAR *pszEscapedString = Str_GetTemp();
				SimpleThreadLock lock(m_connectionMutex);
				if ( m_socket && mysql_real_escape_string(m_socket, pszEscapedString, pszKey, static_cast<unsigned long>(strlen(pszKey))) )
					sVal = pszEscapedString;
			}
			return true;
		}
		case DBO_ROW:
		{
			pszKey += strlen(sm_szLoadKeys[index]);
			SKIP_SEPARATORS(pszKey);
			sVal = m_QueryResult.GetKeyStr(pszKey);
			return true;
		}
		default:
			return false;
	}
}

bool CDataBase::r_Verb(CScript &s, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CDataBase::r_Verb");
	UNREFERENCED_PARAMETER(pSrc);
	if ( !g_Cfg.m_bMySql )
		return true;

	int index = FindTableSorted(s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1);
	switch ( index )
	{
		case DBOV_CLOSE:
		{
			if ( m_socket )
				Close();
			return true;
		}
		case DBOV_CONNECT:
		{
			if ( !m_socket )
				Connect();
			return true;
		}
		case DBOV_EXECUTE:
		{
			Exec(s.GetArgRaw());
			return true;
		}
		case DBOV_QUERY:
		{
			Query(s.GetArgRaw(), m_QueryResult);
			return true;
		}
		default:
			return false;
	}
}
