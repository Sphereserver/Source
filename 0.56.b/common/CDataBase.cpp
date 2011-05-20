#include "../common/CDataBase.h"
#include "../graysvr/graysvr.h"
#include "../sphere/asyncdb.h"

extern CDataBaseAsyncHelper g_asyncHdb;

CDataBase::CDataBase() : stlqueryLock(&m_resultMutex)
{
#ifndef _DBPLUGIN
	_bConnected = false;
	_myData = NULL;
#else
	ResizeFieldArraySize(0);
	ResizeResultArraySize(0);
#endif
}

CDataBase::~CDataBase()
{
#ifndef _DBPLUGIN
	if ( isConnected() )
		Close();
#else
	if ( isConnected() )
		cDatabaseLoader::GetCurrentInstance()->DbClose();

	ResizeFieldArraySize(0);
	ResizeResultArraySize(0);
#endif
}

#ifdef _DBPLUGIN
fieldarray_t * CDataBase::GetFieldArrayBuffer()
{
	ADDTOCALLSTACK("CDataBase::GetFieldArrayBuffer");
	return m_faContainer.faData;
}

int CDataBase::GetFieldArraySize()
{	
	ADDTOCALLSTACK("CDataBase::GetFieldArraySize");
	return maximum(m_faContainer.faDataSize, m_faContainer.faDataActualSize);
}

void CDataBase::ResizeFieldArraySize(int howmuch, bool bForceResize)
{
	ADDTOCALLSTACK("CDataBase::ResizeFieldArraySize");
	if ( !howmuch )
	{
		if ( m_faContainer.faData )
		{
			delete m_faContainer.faData;
		}
		
		m_faContainer.faDataSize = m_faContainer.faDataActualSize = 0;
		return;
	}

	if ( !bForceResize && m_faContainer.faDataSize >= howmuch )
	{
		m_faContainer.faDataActualSize = howmuch;
		return;
	}

	if ( bForceResize && m_faContainer.faDataSize == howmuch )
	{
		m_faContainer.faDataActualSize = howmuch;
		return;
	}

	m_faContainer.faData = (fieldarray_t *)realloc(m_faContainer.faData, howmuch * sizeof(fieldarray_t));
	m_faContainer.faDataSize = m_faContainer.faDataActualSize = howmuch;
}

resultarray_t * CDataBase::GetResultArrayBuffer()
{
	ADDTOCALLSTACK("CDataBase::GetResultArrayBuffer");
	return m_raContainer.raData;
}

int CDataBase::GetResultArraySize()
{
	ADDTOCALLSTACK("CDataBase::GetResultArraySize");
	return maximum(m_raContainer.raDataSize, m_raContainer.raDataActualSize);
}

void CDataBase::ResizeResultArraySize(int howmuch, bool bForceResize)
{
	ADDTOCALLSTACK("CDataBase::ResizeResultArraySize");
	if ( !howmuch )
	{
		if ( m_raContainer.raData )
		{
			delete m_raContainer.raData;
		}
		
		m_raContainer.raDataSize = m_raContainer.raDataActualSize = 0;
		return;
	}

	if ( !bForceResize && m_raContainer.raDataSize >= howmuch )
	{
		m_raContainer.raDataActualSize = howmuch;
		return;
	}

	if ( bForceResize && m_raContainer.raDataSize == howmuch )
	{
		m_raContainer.raDataActualSize = howmuch;
		return;
	}

	m_raContainer.raData = (resultarray_t *)realloc(m_raContainer.raData, howmuch * sizeof(resultarray_t));
	m_raContainer.raDataSize = m_raContainer.raDataActualSize = howmuch;
}
#endif

bool CDataBase::Connect(const char *user, const char *password, const char *base, const char *host)
{
	ADDTOCALLSTACK("CDataBase::Connect");
#ifndef _DBPLUGIN
	_bConnected = false;

	long ver = mysql_get_client_version();
	if ( ver < MIN_MYSQL_VERSION_ALLOW )
	{
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "Your MySQL client library is too old (version %ld). Minimal allowed version is %d. MySQL support disabled.\n", ver, MIN_MYSQL_VERSION_ALLOW);
		g_Cfg.m_bMySql = false;
		return false;
	}

	_myData = mysql_init(NULL);
	if ( !_myData )
		return false;

	int portnum = 0;
	const char *port = NULL;
	if ( (port = strchr(host, ':')) != NULL )
	{
		char *pszTemp = Str_GetTemp();
		strcpy(pszTemp, host);
		*(strchr(pszTemp, ':')) = 0;
		portnum = ATOI(port+1);
		host = pszTemp;
	}

	if ( !mysql_real_connect(_myData, host, user, password, base, portnum, NULL, CLIENT_MULTI_STATEMENTS ) )
	{
		const char *error = mysql_error(_myData);
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "MySQL connect fail: %s\n", error);
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "Visit this link for more information: http://dev.mysql.com/doc/mysql/search.php?q=%s\n", error);
		mysql_close(_myData);
		return false;
	}

	return (_bConnected = true);
#else
	cDatabaseLoader * pCurrent = cDatabaseLoader::GetCurrentInstance();

	if ( !pCurrent->DbIsRightVersion() )
	{
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "Your database client library is too old. Database support disabled.\n");
		return false;
	}
	
	int portnum = 0;
	const char *port = NULL;
	if ( (port = strchr(host, ':')) != NULL )
	{
		char *pszTemp = Str_GetTemp();
		strcpy(pszTemp, host);
		*(strchr(pszTemp, ':')) = 0;
		portnum = ATOI(port+1);
		host = pszTemp;
	}

	if ( !pCurrent->DbConnect(user, password, base, host, portnum) )
	{
		int iLastError = pCurrent->DbGetLastError();
		const char *error = pCurrent->DbGetLastErrorString();
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "Database connect fail (%d): %s\n", iLastError, error ? error : "Unknown" );
		return false;
	}

	// Prepare the buffer.
	ResizeFieldArraySize(g_Cfg.m_iDbQueryBuffer);
	ResizeResultArraySize(g_Cfg.m_iDbQueryBuffer);

	return isConnected();
#endif
}

bool CDataBase::Connect()
{
	ADDTOCALLSTACK("CDataBase::Connect");
#ifndef _DBPLUGIN
	return Connect(g_Cfg.m_sMySqlUser, g_Cfg.m_sMySqlPass, g_Cfg.m_sMySqlDB, g_Cfg.m_sMySqlHost);
#else
	return Connect(g_Cfg.m_sDbUser, g_Cfg.m_sDbPass, g_Cfg.m_sDbDatabase, g_Cfg.m_sDbHost);
#endif
}

bool CDataBase::isConnected()
{
	ADDTOCALLSTACK("CDataBase::isConnected");
#ifndef _DBPLUGIN
	return _bConnected;
#else
	return (cDatabaseLoader::GetCurrentInstance() && cDatabaseLoader::GetCurrentInstance()->IsReady() && cDatabaseLoader::GetCurrentInstance()->DbIsConnected());
#endif
}

void CDataBase::Close()
{
	ADDTOCALLSTACK("CDataBase::Close");
#ifndef _DBPLUGIN
	mysql_close(_myData);
	_bConnected = false;
#else
	cDatabaseLoader::GetCurrentInstance()->DbClose();
#endif
}

bool CDataBase::query(const char *query, CVarDefMap & mapQueryResult)
{
	ADDTOCALLSTACK("CDataBase::query");
	mapQueryResult.Empty();
	mapQueryResult.SetNumNew("NUMROWS", 0);

#ifndef _DBPLUGIN
	if ( !isConnected() )
		return false;

	int			result;
	MYSQL_RES	*m_res;

	result = mysql_query(_myData, query);
	if ( !result )
	{
		m_res = mysql_store_result(_myData);
		if ( !m_res )
			return false;

		MYSQL_FIELD *fields = mysql_fetch_fields(m_res);
		int			num_fields = mysql_num_fields(m_res);

		mapQueryResult.SetNum("NUMROWS", mysql_num_rows(m_res));
		mapQueryResult.SetNum("NUMCOLS", num_fields);

		char	key[12];
		char	**trow = NULL;
		int		rownum = 0;
		char	*zStore = Str_GetTemp();
		while ( (trow = mysql_fetch_row(m_res)) != NULL )
		{
			for ( int i = 0; i < num_fields; i++ )
			{
				char	*z = trow[i];
				if ( !rownum )
				{
					mapQueryResult.SetStr(ITOA(i, key, 10), true, z);
					mapQueryResult.SetStr(fields[i].name, true, z);
				}

				sprintf(zStore, "%d.%d", rownum, i);
				mapQueryResult.SetStr(zStore, true, z);
				sprintf(zStore, "%d.%s", rownum, fields[i].name);
				mapQueryResult.SetStr(zStore, true, z);
			}
			rownum++;
		}

		mysql_free_result(m_res);
		return true;
	}
	else
	{
		const char *myErr = mysql_error(_myData);
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR,
			"MySQL query \"%s\" failed due to \"%s\"\n",
			query, ( *myErr ? myErr : "unknown reason"));
	}

	if (( result == CR_SERVER_GONE_ERROR ) || ( result == CR_SERVER_LOST ))
		Close();

#else
	cDatabaseLoader * pCurrent = cDatabaseLoader::GetCurrentInstance();

	if ( !isConnected() )
		return false;

	int iResult = pCurrent->DbQuery(query);
	if ( !iResult )
	{
		int	num_fields = pCurrent->DbNumFields();
		if ( num_fields > 0 )
		{
			if ( num_fields > GetFieldArraySize() )
				ResizeFieldArraySize(num_fields);

			fieldarray_t * tfields = GetFieldArrayBuffer();
			pCurrent->DbFetchFields(tfields);

			if ( num_fields > GetResultArraySize() )
				ResizeResultArraySize(num_fields);

			char	key[12];
			resultarray_t * rfields = GetResultArrayBuffer();
			int		rownum = 0;
			TemporaryString zStore;

			while ( pCurrent->DbFetchRow(rfields) > 0 )
			{
				for ( int i = 0; i < num_fields; ++i )
				{
					char *z = rfields[i].data;

					if ( !rownum )
					{
						mapQueryResult.SetStr(ITOA(i, key, 10), true, z);
						mapQueryResult.SetStr(tfields[i].name, true, z);
					}

					sprintf(zStore, "%d.%d", rownum, i);
					mapQueryResult.SetStr(zStore, true, z);
					sprintf(zStore, "%d.%s", rownum, tfields[i].name);
					mapQueryResult.SetStr(zStore, true, z);
				}
				rownum++;
			}

			mapQueryResult.SetNum("NUMROWS", rownum);
			mapQueryResult.SetNum("NUMCOLS", num_fields);
			return true;
		}
	}

	if ( iResult < 0 ) // It's an error
	{
		const char *myErr = pCurrent->DbGetLastErrorString();
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "Database query \"%s\" failed due to \"%s\"\n", query, ( myErr && *myErr ? myErr : "unknown reason"));

		if ( iResult == DBPLUGIN_SERVER_LOST_ERROR )
			pCurrent->DbClose();
	}
#endif

	return false;
}

bool __cdecl CDataBase::queryf(CVarDefMap & mapQueryResult, char *fmt, ...)
{
	ADDTOCALLSTACK("CDataBase::queryf");
	TemporaryString buf;
	va_list	marker;

	va_start(marker, fmt);
	_vsnprintf(buf, buf.realLength(), fmt, marker);
	va_end(marker);

	return this->query(buf, mapQueryResult);
}

bool CDataBase::exec(const char *query)
{
	ADDTOCALLSTACK("CDataBase::exec");
#ifndef _DBPLUGIN
	if ( !isConnected() )
		return false;

	int result = mysql_query(_myData, query);
	if (result == 0)
	{
		// even though we don't want (or expect) any result data, we must retrieve
		// is anyway otherwise we will lose our connection to the server
		MYSQL_RES* res = mysql_store_result(_myData);
		if (res != NULL)
			mysql_free_result(res);

		return true;
	}
	else
	{
		const char *myErr = mysql_error(_myData);
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "MySQL query \"%s\" failed due to \"%s\"\n",
			query, ( *myErr ? myErr : "unknown reason"));
	}
	
	if (( result == CR_SERVER_GONE_ERROR ) || ( result == CR_SERVER_LOST ))
		Close();

#else
	cDatabaseLoader * pCurrent = cDatabaseLoader::GetCurrentInstance();

	if ( !isConnected() )
		return false;

	int iResult = pCurrent->DbExecute(query);
	if ( iResult == 0 )
		return true;
	
	const char *myErr = pCurrent->DbGetLastErrorString();
	g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "Database query \"%s\" failed due to \"%s\"\n", query, ( myErr && *myErr ? myErr : "unknown reason"));

	if ( iResult == DBPLUGIN_SERVER_LOST_ERROR )
		pCurrent->DbClose();
#endif

	return false;
}

bool __cdecl CDataBase::execf(char *fmt, ...)
{
	ADDTOCALLSTACK("CDataBase::execf");
	TemporaryString buf;
	va_list	marker;

	va_start(marker, fmt);
	_vsnprintf(buf, buf.realLength(), fmt, marker);
	va_end(marker);

	return this->exec(buf);
}

#ifndef _DBPLUGIN
UINT CDataBase::getLastId()
{
	ADDTOCALLSTACK("CDataBase::getLastId");
	return mysql_insert_id(_myData);
}
#endif

bool CDataBase::addQuery(bool isQuery, LPCTSTR theFunction, LPCTSTR theQuery)
{
	if ( g_Cfg.m_Functions.ContainsKey( theFunction ) == false )
	{
		DEBUG_ERR(("Invalid callback function (%s) for AEXECUTE/AQUERY.\n", theFunction));
		return false;
	}
	else
	{
		if ( !g_asyncHdb.isActive() )
			g_asyncHdb.start();

		g_asyncHdb.addQuery(isQuery,theFunction,theQuery);
		return true;
	}
}

void CDataBase::addQueryResult(CGString theFunction, CScriptTriggerArgs * theResult)
{
	SimpleThreadLock stlThelock(m_resultMutex);

	m_QueryArgs.push(FunctionArgsPair_t(theFunction,theResult));
}

bool CDataBase::OnTick()
{
	ADDTOCALLSTACK("CDataBase::OnTick");
	static int tickcnt = 0;
	EXC_TRY("Tick");

#ifndef _DBPLUGIN
	if ( !g_Cfg.m_bMySql )	//	mySQL is not supported
		return true;
#else
	if ( g_Cfg.m_sDbDll.IsEmpty() )	//	DB is not supported
		return true;
#endif

	//	do not ping sql server too heavily
	if ( ++tickcnt >= 1000 )
	{
		tickcnt = 0;
#ifdef _DBPLUGIN
		cDatabaseLoader * pCurrent = cDatabaseLoader::GetCurrentInstance();
#endif

		if ( isConnected() )	//	currently connected - just check that the link is alive
		{

#ifndef _DBPLUGIN
			if ( mysql_ping(_myData) )
			{
				g_Log.EventError("MySQL server link has been lost. Trying to reattach to it.\n");
				Close();

				if ( !Connect() )
				{
					g_Log.EventError("MySQL reattach failed/timed out. SQL operations disabled.\n");
				}
			}
#else
			// We shrink mem if connected.
			ResizeFieldArraySize(g_Cfg.m_iDbQueryBuffer, true);
			ResizeResultArraySize(g_Cfg.m_iDbQueryBuffer, true);

			if ( pCurrent->DbPing() )
			{
				g_Log.EventError("Database server link has been lost. Trying to reattach to it.\n");
				pCurrent->DbClose();

				if ( !Connect() )
				{
					g_Log.EventError("Database reattach failed/timed out. SQL operations disabled.\n");
				}
			}
#endif
		}
	}

	if ( !m_QueryArgs.empty() && !(tickcnt % TICK_PER_SEC) )
	{
		stlqueryLock.doLock();

		FunctionArgsPair_t currentPair = m_QueryArgs.front();
		m_QueryArgs.pop();

		stlqueryLock.doUnlock();

		if ( !g_Serv.r_Call(currentPair.first, &g_Serv, currentPair.second) )
		{
			// error
		}

		delete currentPair.second;
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_DEBUG_END;
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

LPCTSTR const CDataBase::sm_szLoadKeys[DBO_QTY+1] =
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

LPCTSTR const CDataBase::sm_szVerbKeys[DBOV_QTY+1] =
{
	"CLOSE",
	"CONNECT",
	"EXECUTE",
	"QUERY",
	NULL
};

bool CDataBase::r_GetRef(LPCTSTR & pszKey, CScriptObj * & pRef)
{
	ADDTOCALLSTACK("CDataBase::r_GetRef");
	UNREFERENCED_PARAMETER(pszKey);
	UNREFERENCED_PARAMETER(pRef);
	return false;
}

bool CDataBase::r_LoadVal(CScript & s)
{
	ADDTOCALLSTACK("CDataBase::r_LoadVal");
	UNREFERENCED_PARAMETER(s);
	return false;
/*
	LPCTSTR pszKey = s.GetKey();
	EXC_TRY("LoadVal");

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1);

	switch ( index )
	{
		default:
			return false;
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
*/
}

bool CDataBase::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CDataBase::r_WriteVal");
	EXC_TRY("WriteVal");

#ifndef _DBPLUGIN
	// Just return 0 if MySQL is disabled
	if (!g_Cfg.m_bMySql)
#else
	if (g_Cfg.m_sDbDll.IsEmpty())
#endif
	{
		sVal.FormatVal( 0 );
		return true;
	}

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1);
	switch ( index )
	{
		case DBO_AEXECUTE:
		case DBO_AQUERY:
			{
				pszKey += strlen(sm_szLoadKeys[index]);
				GETNONWHITESPACE(pszKey);
				sVal.FormatVal(0);

				if ( pszKey && *pszKey )
				{
					TCHAR * ppArgs[2];
					if ( Str_ParseCmds( (TCHAR *)pszKey, ppArgs, COUNTOF( ppArgs )) != 2) 
					{
						DEBUG_ERR(("Not enough arguments for %s\n", CDataBase::sm_szLoadKeys[index]));
					}
					else
					{
						sVal.FormatVal( addQuery((index == DBO_AQUERY), ppArgs[0], ppArgs[1]) );
					}
				}
				else
				{
					DEBUG_ERR(("Not enough arguments for %s\n", CDataBase::sm_szLoadKeys[index]));
				}
			} break;

		case DBO_CONNECTED:
			sVal.FormatVal(isConnected());
			break;

		case DBO_ESCAPEDATA:
		{
			pszKey += strlen(sm_szLoadKeys[index]);
			GETNONWHITESPACE(pszKey);
			sVal = "";

			if ( pszKey && *pszKey )
			{
				TCHAR * escapedString = Str_GetTemp();

#ifndef _DBPLUGIN
				if ( isConnected() && mysql_real_escape_string(_myData, escapedString, pszKey, strlen(pszKey)) )
				{
					sVal = escapedString;
				}
#else
				int iOutLength = cDatabaseLoader::GetCurrentInstance()->DbEscapeString(pszKey, strlen(pszKey), escapedString);
				if ( iOutLength > 0 )
				{
					sVal = escapedString;
				}
#endif
			}
		} break;

		case DBO_ROW:
		{
			pszKey += strlen(sm_szLoadKeys[index]);
			SKIP_SEPARATORS(pszKey);
			sVal = m_QueryResult.GetKeyStr(pszKey);
		} break;

		default:
			return false;
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("command '%s' [%p]\n", pszKey, static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return false;
}

bool CDataBase::r_Verb(CScript & s, CTextConsole * pSrc)
{
	ADDTOCALLSTACK("CDataBase::r_Verb");
	EXC_TRY("Verb");

#ifndef _DBPLUGIN
	// Just return true if MySQL is disabled
	if (!g_Cfg.m_bMySql)
#else
	if (g_Cfg.m_sDbDll.IsEmpty())
#endif
	{
		return true;
	}

	int index = FindTableSorted(s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys)-1);
	switch ( index )
	{
		case DBOV_CLOSE:
			if ( isConnected() )
				Close();
			break;

		case DBOV_CONNECT:
			if ( isConnected() )
				Close();

			Connect();
			break;

		case DBOV_EXECUTE:
			exec(s.GetArgRaw());
			break;

		case DBOV_QUERY:
			query(s.GetArgRaw(), m_QueryResult);
			break;

		default:
			return false;
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("command '%s' args '%s' [%p]\n", s.GetKey(), s.GetArgRaw(), static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return false;
}
