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
	Close();
}

void CDataBase::Connect()
{
	ADDTOCALLSTACK("CDataBase::Connect");
	SimpleThreadLock lock(m_connectionMutex);
	if ( m_socket )
		return;

	m_socket = mysql_init(NULL);
	if ( !m_socket )
	{
		g_Log.EventError("Failed to initialize MySQL client (out of memory)\n");
		return;
	}

	if ( mysql_get_client_version() < LIBMYSQL_VERSION_ID )
	{
#ifdef _WIN32
		const char *pszLibrary = "libmysql.dll file";
#else
		const char *pszLibrary = "Debian-based: libmysqlclient21 package / RHEL-based: mysql-libs package";
#endif
		g_Log.EventWarn("MySQL client v%s is outdated. For better compatibility, update MySQL client library to v%s (%s)\n", mysql_get_client_info(), LIBMYSQL_VERSION, pszLibrary);
	}

	bool fGetServerPublicKey = true;
	mysql_options(m_socket, MYSQL_OPT_GET_SERVER_PUBLIC_KEY, &fGetServerPublicKey);

	char szHost[CONNECT_STRING_MAXLEN];
	strncpy(szHost, g_Cfg.m_sMySqlHost, COUNTOF(szHost));
	szHost[COUNTOF(szHost) - 1] = '\0';

	unsigned int uPort = MYSQL_PORT;
	char *pszPort = strchr(szHost, ':');
	if ( pszPort )
	{
		*pszPort = '\0';
		uPort = static_cast<unsigned int>(ATOI(pszPort + 1));
	}

	const char *pszUser = g_Cfg.m_sMySqlUser;
	const char *pszPass = g_Cfg.m_sMySqlPass;
	const char *pszDB = g_Cfg.m_sMySqlDB;

	if ( mysql_real_connect(m_socket, szHost, pszUser, pszPass, pszDB, uPort, NULL, CLIENT_MULTI_STATEMENTS) )
	{
		if ( mysql_get_server_version(m_socket) < MYSQL_VERSION_ID )
			g_Log.EventWarn("MySQL server v%s is outdated. For better compatibility, update MySQL server to v%s\n", mysql_get_server_info(m_socket), MYSQL_SERVER_VERSION);
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
	if ( !m_socket )
		return;

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
	if ( iResultCode != 0 )
	{
		unsigned int uErrNum = mysql_errno(m_socket);
		g_Log.EventError("MySQL error #%u: %s [Cmd: \"%s\"]\n", uErrNum, mysql_error(m_socket), pszQuery);

		if ( (uErrNum == CR_SERVER_GONE_ERROR) || (uErrNum == CR_SERVER_LOST) )
			Close();

		return false;
	}

	MYSQL_RES *pResult = mysql_store_result(m_socket);
	if ( !pResult )
	{
		bool fNoFields = (mysql_field_count(m_socket) == 0);
		while ( mysql_next_result(m_socket) == 0 )
		{
			pResult = mysql_store_result(m_socket);
			if ( pResult )
				mysql_free_result(pResult);
		}
		return fNoFields;
	}

	unsigned int uNumFields = mysql_num_fields(pResult);
	unsigned int uMaxFields = 100;
	if ( uNumFields > uMaxFields )
	{
		// MySQL client can handle the same column amount as MySQL server (4096), but it's
		// better to limit this to prevent key truncation/collision on query result
		g_Log.EventError("MySQL query exceeded max columns allowed (%u/%u) [Cmd: \"%s\"]\n", uNumFields, uMaxFields, pszQuery);

		mysql_free_result(pResult);
		while ( mysql_next_result(m_socket) == 0 )
		{
			pResult = mysql_store_result(m_socket);
			if ( pResult )
				mysql_free_result(pResult);
		}
		return false;
	}

	MYSQL_FIELD *pFields = mysql_fetch_fields(pResult);
	mapQueryResult.SetNum("NUMROWS", mysql_num_rows(pResult));
	mapQueryResult.SetNum("NUMCOLS", uNumFields);

	int iRowNum = 0;
	MYSQL_ROW pRow;
	char szKey[128];
	char *pszVal = NULL;
	while ( (pRow = mysql_fetch_row(pResult)) != NULL )
	{
		for ( unsigned int i = 0; i < uNumFields; ++i )
		{
			pszVal = pRow[i];
			if ( iRowNum == 0 )
			{
				snprintf(szKey, sizeof(szKey), "%u", i);
				mapQueryResult.SetStr(szKey, true, pszVal);

				snprintf(szKey, sizeof(szKey), "%s", pFields[i].name);
				mapQueryResult.SetStr(szKey, true, pszVal);
			}

			snprintf(szKey, sizeof(szKey), "%d.%u", iRowNum, i);
			mapQueryResult.SetStr(szKey, true, pszVal);

			snprintf(szKey, sizeof(szKey), "%d.%s", iRowNum, pFields[i].name);
			mapQueryResult.SetStr(szKey, true, pszVal);
		}
		++iRowNum;
	}

	mysql_free_result(pResult);
	while ( mysql_next_result(m_socket) == 0 )
	{
		pResult = mysql_store_result(m_socket);
		if ( pResult )
			mysql_free_result(pResult);
	}
	return true;
}

bool __cdecl CDataBase::Queryf(CVarDefMap &mapQueryResult, char *pFormat, ...)
{
	ADDTOCALLSTACK("CDataBase::Queryf");
	TemporaryString pszBuffer;
	va_list marker;

	va_start(marker, pFormat);
	vsnprintf(pszBuffer, pszBuffer.realLength(), pFormat, marker);
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
	if ( iResultCode != 0 )
	{
		unsigned int uErrNum = mysql_errno(m_socket);
		g_Log.EventError("MySQL error #%u: %s [Cmd: \"%s\"]\n", uErrNum, mysql_error(m_socket), pszQuery);

		if ( (uErrNum == CR_SERVER_GONE_ERROR) || (uErrNum == CR_SERVER_LOST) )
			Close();

		return false;
	}

	MYSQL_RES *pResult = mysql_store_result(m_socket);
	if ( pResult )
		mysql_free_result(pResult);

	while ( mysql_next_result(m_socket) == 0 )
	{
		pResult = mysql_store_result(m_socket);
		if ( pResult )
			mysql_free_result(pResult);
	}

	return true;
}

bool __cdecl CDataBase::Execf(char *pFormat, ...)
{
	ADDTOCALLSTACK("CDataBase::Execf");
	TemporaryString pszBuffer;
	va_list marker;

	va_start(marker, pFormat);
	vsnprintf(pszBuffer, pszBuffer.realLength(), pFormat, marker);
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

bool CDataBase::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CDataBase::r_WriteVal");
	UNREFERENCED_PARAMETER(pSrc);
	if ( !g_Cfg.m_bMySql )
	{
		sVal = "0";
		return true;
	}

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	switch ( index )
	{
		case DBO_AEXECUTE:
		case DBO_AQUERY:
		{
			pszKey += (index == DBO_AEXECUTE) ? 8 : 6;
			SkipWhitespace(pszKey);

			TCHAR *ppArgs[2];
			if ( (*pszKey != '\0') && (Str_ParseCmds(const_cast<TCHAR *>(pszKey), ppArgs, COUNTOF(ppArgs)) == 2) )
				sVal.FormatVal(AsyncQueue((index == DBO_AQUERY), ppArgs[0], ppArgs[1]));
			else
			{
				g_Log.EventError("%s: invalid arguments\n", sm_szLoadKeys[index]);
				sVal = "0";
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
			pszKey += 10;
			SkipWhitespace(pszKey);
			sVal = "";

			if ( m_socket && (*pszKey != '\0') )
			{
				char szEscapedString[THREAD_STRING_LENGTH];
				size_t iLen = strlen(pszKey);
				if ( iLen > (sizeof(szEscapedString) / 2) - 1 )
					g_Log.EventError("%s: args exceeded max length allowed (%zu/%zu)\n", sm_szLoadKeys[index], iLen, (sizeof(szEscapedString) / 2) - 1);
				else
				{
					SimpleThreadLock lock(m_connectionMutex);
					if ( mysql_real_escape_string(m_socket, szEscapedString, pszKey, static_cast<unsigned long>(iLen)) )
						sVal = szEscapedString;
				}
			}
			return true;
		}
		case DBO_ROW:
		{
			pszKey += 3;
			SkipDotSeparator(pszKey);
			sVal = m_QueryResult.GetKeyStr(pszKey);
			return true;
		}
		default:
			return false;
	}
}

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
			Close();
			return true;

		case DBOV_CONNECT:
			Connect();
			return true;

		case DBOV_EXECUTE:
			Exec(s.GetArgRaw());
			return true;

		case DBOV_QUERY:
			Query(s.GetArgRaw(), m_QueryResult);
			return true;

		default:
			return false;
	}
}
