#include "../common/CDataBase.h"
#include "../graysvr/graysvr.h"

CDataBase::CDataBase()
{
#ifndef _EXTERNAL_DLL
	_bConnected = false;
#else
	ResizeFieldArraySize(0);
	ResizeResultArraySize(0);
#endif
}

CDataBase::~CDataBase()
{
#ifndef _EXTERNAL_DLL
	if ( isConnected() )
		Close();
#else
	if ( isConnected() )
		cDatabaseLoader::GetCurrentIstance()->DbClose();

	ResizeFieldArraySize(0);
	ResizeResultArraySize(0);
#endif
}

#ifdef _EXTERNAL_DLL
fieldarray_t * CDataBase::GetFieldArrayBuffer()
{
	ADDTOCALLSTACK("CDataBase::GetFieldArrayBuffer");
	return faContainer.faData;
}

int CDataBase::GetFieldArraySize()
{	
	ADDTOCALLSTACK("CDataBase::GetFieldArraySize");
	return maximum(faContainer.faDataSize,faContainer.faDataActualSize);
}

void CDataBase::ResizeFieldArraySize(int howmuch, bool bForceResize)
{
	ADDTOCALLSTACK("CDataBase::ResizeFieldArraySize");
	if ( !howmuch )
	{
		if ( faContainer.faData )
		{
			delete faContainer.faData;
		}
		
		faContainer.faDataSize = faContainer.faDataActualSize = 0;
		return;
	}

	if ( !bForceResize && faContainer.faDataSize >= howmuch )
	{
		faContainer.faDataActualSize = howmuch;
		return;
	}

	if ( bForceResize && faContainer.faDataSize == howmuch )
	{
		faContainer.faDataActualSize = howmuch;
		return;
	}

	faContainer.faData = (fieldarray_t *)realloc(faContainer.faData, howmuch*sizeof(fieldarray_t));
	faContainer.faDataSize = faContainer.faDataActualSize = howmuch;
}

resultarray_t * CDataBase::GetResultArrayBuffer()
{
	ADDTOCALLSTACK("CDataBase::GetResultArrayBuffer");
	return raContainer.raData;
}

int CDataBase::GetResultArraySize()
{
	ADDTOCALLSTACK("CDataBase::GetResultArraySize");
	return maximum(raContainer.raDataSize,raContainer.raDataActualSize);
}

void CDataBase::ResizeResultArraySize(int howmuch, bool bForceResize)
{
	ADDTOCALLSTACK("CDataBase::ResizeResultArraySize");
	if ( !howmuch )
	{
		if ( raContainer.raData )
		{
			delete raContainer.raData;
		}
		
		raContainer.raDataSize = raContainer.raDataActualSize = 0;
		return;
	}

	if ( !bForceResize && raContainer.raDataSize >= howmuch )
	{
		raContainer.raDataActualSize = howmuch;
		return;
	}

	if ( bForceResize && raContainer.raDataSize == howmuch )
	{
		raContainer.raDataActualSize = howmuch;
		return;
	}

	raContainer.raData = (resultarray_t *)realloc(raContainer.raData, howmuch*sizeof(resultarray_t));
	raContainer.raDataSize = raContainer.raDataActualSize = howmuch;
}
#endif

bool CDataBase::Connect(const char *user, const char *password, const char *base, const char *host)
{
	ADDTOCALLSTACK("CDataBase::Connect");
#ifndef _EXTERNAL_DLL
	_bConnected = false;

	long ver = mysql_get_client_version();
	if ( ver < MIN_MYSQL_VERSION_ALLOW )
	{
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "Your MySQL client library is too old (version %d). Minimal allowed version is %d. MySQL support disabled.\n", ver, MIN_MYSQL_VERSION_ALLOW);
		g_Cfg.m_bMySql = false;
		return false;
	}

	_myData = mysql_init(NULL);
	if ( !_myData )
		return false;

	int portnum = 0;
	char *port = NULL;
	if ( port = const_cast<char*>(strchr(host, ':')) )
	{
		char *pszTemp = Str_GetTemp();
		strcpy(pszTemp, host);
		*(strchr(pszTemp, ':')) = 0;
		port++;
		portnum = ATOI(port);
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
	cDatabaseLoader * pCurrent = cDatabaseLoader::GetCurrentIstance();

	if ( !pCurrent->DbIsRightVersion() )
	{
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "Your DataBase client library is too old. Database support disabled.\n");
		return false;
	}

	int portnum = 0;
	char *port = NULL;
	if ( port = strchr(host, ':') )
	{
		char *pszTemp = Str_GetTemp();
		strcpy(pszTemp, host);
		*(strchr(pszTemp, ':')) = 0;
		port++;
		portnum = ATOI(port);
		host = pszTemp;
	}

	if ( !pCurrent->DbConnect(user, password, base, host, portnum) )
	{
		int iLastError = pCurrent->DbGetLastError();
		const char *error = pCurrent->DbGetLastErrorString();
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "DataBase connect fail (%d): %s\n", iLastError, error ? error : "Unknown" );
		return false;
	}

	// Prepare the buffer.
	ResizeFieldArraySize(g_Cfg.m_iDbDllQueryBuffer);
	ResizeResultArraySize(g_Cfg.m_iDbDllQueryBuffer);

	return isConnected();
#endif
}

bool CDataBase::Connect()
{
	ADDTOCALLSTACK("CDataBase::Connect");
#ifndef _EXTERNAL_DLL
	return Connect(g_Cfg.m_sMySqlUser, g_Cfg.m_sMySqlPass, g_Cfg.m_sMySqlDB, g_Cfg.m_sMySqlHost);
#else
	return Connect(g_Cfg.m_sDbDllUser, g_Cfg.m_sDbDllPass, g_Cfg.m_sDbDllDB, g_Cfg.m_sDbDllHost);
#endif
}

bool CDataBase::isConnected()
{
	ADDTOCALLSTACK("CDataBase::isConnected");
#ifndef _EXTERNAL_DLL
	return _bConnected;
#else
	return (cDatabaseLoader::GetCurrentIstance() && cDatabaseLoader::GetCurrentIstance()->IsReady() && cDatabaseLoader::GetCurrentIstance()->DbIsConnected());
#endif
}

void CDataBase::Close()
{
	ADDTOCALLSTACK("CDataBase::Close");
#ifndef _EXTERNAL_DLL
	mysql_close(_myData);
	_bConnected = false;
#else
	cDatabaseLoader::GetCurrentIstance()->DbClose();
#endif
}

bool CDataBase::query(const char *query)
{
	ADDTOCALLSTACK("CDataBase::query");
	m_QueryResult.Empty();
	m_QueryResult.SetNumNew("NUMROWS", 0);

#ifndef _EXTERNAL_DLL
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

		m_QueryResult.SetNum("NUMROWS", mysql_num_rows(m_res));
		m_QueryResult.SetNum("NUMCOLS", num_fields);

		char	key[12];
		char	**trow = NULL;
		int		rownum = 0;
		char	*zStore = Str_GetTemp();
		while ( trow = mysql_fetch_row(m_res) )
		{
			for ( int i = 0; i < num_fields; i++ )
			{
				char	*z = trow[i];
				if ( !rownum )
				{
					m_QueryResult.SetStr(ITOA(i, key, 10), true, z);
					m_QueryResult.SetStr(fields[i].name, true, z);
				}

				sprintf(zStore, "%d.%d", rownum, i);
				m_QueryResult.SetStr(zStore, true, z);
				sprintf(zStore, "%d.%s", rownum, fields[i].name);
				m_QueryResult.SetStr(zStore, true, z);
			}
			rownum++;
		}
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
	return false;
#else
	cDatabaseLoader * pCurrent = cDatabaseLoader::GetCurrentIstance();

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

			m_QueryResult.SetNum("NUMROWS", pCurrent->DbNumRows());
			m_QueryResult.SetNum("NUMCOLS", num_fields);

			if ( num_fields > GetResultArraySize() )
				ResizeResultArraySize(num_fields);

			char	key[12];
			resultarray_t * rfields = GetResultArrayBuffer();
			int		rownum = 0;
			TemporaryString zStore;

			while ( pCurrent->DbFetchRow(rfields) > 0 )
			{
				for ( int i = 0; i < num_fields; ++i, zStore.setAt(0,'\0') )
				{
					char *z = rfields[i].data;

					if ( !rownum )
					{
						m_QueryResult.SetStr(ITOA(i, key, 10), true, z);
						m_QueryResult.SetStr(tfields[i].name, true, z);
					}

					sprintf(zStore, "%d.%d", rownum, i);
					m_QueryResult.SetStr(zStore, true, z);
					sprintf(zStore, "%d.%s", rownum, tfields[i].name);
					m_QueryResult.SetStr(zStore, true, z);
				}
				rownum++;
			}

			return true;
		}
	}

	if ( iResult < 0 ) // It's an error
	{
		if ( iResult == WRAPPER_SERVER_LOST_ERROR )
		{
			pCurrent->DbClose();
		} 
		else
		{
			const char *myErr = pCurrent->DbGetLastErrorString();
			g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "DataBase query \"%s\" failed due to \"%s\"\n", query, ( myErr && *myErr ? myErr : "unknown reason"));
		}
	}

	return false;
#endif
}

bool __cdecl CDataBase::queryf(char *fmt, ...)
{
	ADDTOCALLSTACK("CDataBase::queryf");
	TemporaryString buf;
	va_list	marker;

	va_start(marker, fmt);
	_vsnprintf(buf, buf.realLength(), fmt, marker);
	va_end(marker);

	return this->query(buf);
}

void CDataBase::exec(const char *query)
{
	ADDTOCALLSTACK("CDataBase::exec");
#ifndef _EXTERNAL_DLL
	if ( !isConnected() )
		return;

	if ( mysql_query(_myData, query) )
	{
		const char *myErr = mysql_error(_myData);
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "MySQL query \"%s\" failed due to \"%s\"\n",
			query, ( *myErr ? myErr : "unknown reason"));
	}
#else
	cDatabaseLoader * pCurrent = cDatabaseLoader::GetCurrentIstance();

	if ( isConnected() )
	{
		int iResult = pCurrent->DbExecute(query);
		if ( iResult )
		{
			if ( iResult == WRAPPER_SERVER_LOST_ERROR )
			{
				pCurrent->DbClose();
			} 
			else
			{
				const char *myErr = pCurrent->DbGetLastErrorString();
				g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR,
					"DataBase query \"%s\" failed due to \"%s\"\n",
					query, ( myErr && *myErr ? myErr : "unknown reason"));
			}
		}
	}
#endif
}

void __cdecl CDataBase::execf(char *fmt, ...)
{
	ADDTOCALLSTACK("CDataBase::execf");
	TemporaryString buf;
	va_list	marker;

	va_start(marker, fmt);
	_vsnprintf(buf, buf.realLength(), fmt, marker);
	va_end(marker);

	this->exec(buf);
}

#ifndef _EXTERNAL_DLL
UINT CDataBase::getLastId()
{
	ADDTOCALLSTACK("CDataBase::getLastId");
	return mysql_insert_id(_myData);
}
#endif

bool CDataBase::OnTick()
{
	ADDTOCALLSTACK("CDataBase::OnTick");
	static int tickcnt = 0;
	EXC_TRY("Tick");

#ifndef _EXTERNAL_DLL
	if ( !g_Cfg.m_bMySql )	//	mySQL is not supported
		return true;

	//	do not ping sql server too heavily
	if ( ++tickcnt < 1000 )
		return true;
	tickcnt = 0;

	if ( isConnected() )	//	currently connected - just check that the link is alive
	{
		if ( mysql_ping(_myData) )
		{
			g_Log.EventError("MySQL server link has been lost. Trying to reattach to it\n");
			Close();

			if ( !Connect(g_Cfg.m_sMySqlUser, g_Cfg.m_sMySqlPass, g_Cfg.m_sMySqlDB, g_Cfg.m_sMySqlHost) )
			{
				g_Log.EventError("MySQL reattach failed/timed out. SQL operations disabled.\n");
			}
		}
	}
#else
	if ( g_Cfg.m_sDbDll.IsEmpty() )	//	mySQL is not supported
		return true;

	//	do not ping sql server too heavily
	if ( ++tickcnt < 1000 )
		return true;

	tickcnt = 0;
	cDatabaseLoader * pCurrent = cDatabaseLoader::GetCurrentIstance();

	if ( isConnected() )	//	currently connected - just check that the link is alive
	{
		// We shrink mem if connected.
		ResizeFieldArraySize(g_Cfg.m_iDbDllQueryBuffer, true);
		ResizeResultArraySize(g_Cfg.m_iDbDllQueryBuffer, true);

		if ( pCurrent->DbPing() )
		{
			g_Log.EventError("DataBase server link has been lost. Trying to reattach to it\n");
			pCurrent->DbClose();

			if ( !Connect(g_Cfg.m_sDbDllUser, g_Cfg.m_sDbDllPass, g_Cfg.m_sDbDllDB, g_Cfg.m_sDbDllHost) )
			{
				g_Log.EventError("DataBase reattach failed/timed out. SQL operations disabled.\n");
			}
		}
	}
#endif

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_DEBUG_END;
	return false;
}

enum DBO_TYPE
{
	DBO_CONNECTED,
	DBO_ESCAPEDATA,
	DBO_ROW,
	DBO_QTY,
};

LPCTSTR const CDataBase::sm_szLoadKeys[DBO_QTY+1] =
{
	"CONNECTED",
	"ESCAPEDATA",
	"ROW",
	NULL,
};

enum DBOV_TYPE
{
	DBOV_CLOSE,
	DBOV_CONNECT,
	DBOV_EXECUTE,
	DBOV_QUERY,
	DBOV_QTY,
};

LPCTSTR const CDataBase::sm_szVerbKeys[DBOV_QTY+1] =
{
	"CLOSE",
	"CONNECT",
	"EXECUTE",
	"QUERY",
	NULL,
};

bool CDataBase::r_GetRef(LPCTSTR & pszKey, CScriptObj * & pRef)
{
	ADDTOCALLSTACK("CDataBase::r_GetRef");
	return false;
}

bool CDataBase::r_LoadVal(CScript & s)
{
	ADDTOCALLSTACK("CDataBase::r_LoadVal");
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
}

bool CDataBase::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CDataBase::r_WriteVal");
	EXC_TRY("WriteVal");

#ifndef _EXTERNAL_DLL
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

#ifndef _EXTERNAL_DLL
				if ( isConnected() && mysql_real_escape_string(_myData, escapedString, pszKey, strlen(pszKey)) )
				{
					sVal = escapedString;
				}
#else
				int iOutLength = cDatabaseLoader::GetCurrentIstance()->DbEscapeString(pszKey, strlen(pszKey), escapedString);
				if ( iOutLength )
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
	g_Log.EventDebug("command '%s' [%x]\n", pszKey, pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CDataBase::r_Verb(CScript & s, CTextConsole * pSrc)
{
	ADDTOCALLSTACK("CDataBase::r_Verb");
	EXC_TRY("Verb");

#ifndef _EXTERNAL_DLL
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
#ifndef _EXTERNAL_DLL
			Connect(g_Cfg.m_sMySqlUser, g_Cfg.m_sMySqlPass, g_Cfg.m_sMySqlDB, g_Cfg.m_sMySqlHost);
#else
			Connect(g_Cfg.m_sDbDllUser, g_Cfg.m_sDbDllPass, g_Cfg.m_sDbDllDB, g_Cfg.m_sDbDllHost);
#endif
			break;

		case DBOV_EXECUTE:
			exec(s.GetArgRaw());
			break;

		case DBOV_QUERY:
			query(s.GetArgRaw());
			break;

		default:
			return false;
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("command '%s' args '%s' [%x]\n", s.GetKey(), s.GetArgRaw(), pSrc);
	EXC_DEBUG_END;
	return false;
}
