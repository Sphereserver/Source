#include "CDataBase.h"
#include "../graysvr.h"

CDataBase::CDataBase()
{
	_bConnected = false;
}

CDataBase::~CDataBase()
{
	if ( _bConnected )
		Close();
}

bool CDataBase::Connect(const char *user, const char *password, const char *base, const char *host)
{
	_bConnected = false;

	long ver = mysql_get_client_version();
	if ( ver < MIN_MYSQL_VERSION_ALLOW )
	{
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "MySQL client library too old (%d), support disabled. Minimal allowed %d, recommended %d.\n", ver, MIN_MYSQL_VERSION_ALLOW, RECOMMENDED_MYSQL_VERSION);
		g_Cfg.m_bMySql = false;
		return false;
	}
	else if ( ver != RECOMMENDED_MYSQL_VERSION )
	{
		g_Log.Event(LOGM_NOCONTEXT|LOGL_WARN, "MySQL client library is %d. If you experience problems, change replace by version %d.\n", ver, RECOMMENDED_MYSQL_VERSION);
	}

	_myData = mysql_init(NULL);
	if ( !_myData )
		return false;

	int portnum = 0;
	const char *port = NULL;
	if ( port = strchr(host, ':') )
	{
		TEMPSTRING(pszTemp);
		strcpy(pszTemp, host);
		*(strchr(pszTemp, ':')) = 0;
		port++;
		portnum = ATOI(port);
		host = pszTemp;
	}

	if ( !mysql_real_connect(_myData, host, user, password, base, portnum, NULL, 0 ) )
	{
		const char *error = mysql_error(_myData);
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "MySQL connect fail: %s\n", error);
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "More information: http://dev.mysql.com/doc/mysql/search.php?doc=1&q=%s\n", error);
		mysql_close(_myData);
		return false;
	}
	_bConnected = true;
	return true;
}

bool CDataBase::Connect()
{
	return Connect(g_Cfg.m_sMySqlUser, g_Cfg.m_sMySqlPass, g_Cfg.m_sMySqlDB, g_Cfg.m_sMySqlHost);
}

bool CDataBase::isConnected()
{
	return _bConnected;
}

void CDataBase::Close()
{
	mysql_close(_myData);
	_bConnected = false;
}

bool CDataBase::query(const char *query)
{
	if ( !_bConnected )
		return false;
	m_QueryResult.Empty();
	m_QueryResult.SetNumNew("NUMROWS", 0);

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
		TEMPSTRING(zStore);
		while ( trow = mysql_fetch_row(m_res) )
		{
			for ( int i = 0; i < num_fields; i++ )
			{
				char	*z = trow[i];
				if ( !rownum )
				{
					m_QueryResult.SetStr(ITOA(i, key), false, z);
					m_QueryResult.SetStr(fields[i].name, false, z);
				}

				sprintf(zStore, "%d.%d", rownum, i);
				m_QueryResult.SetStr(zStore, false, z);
				sprintf(zStore, "%d.%s", rownum, fields[i].name);
				m_QueryResult.SetStr(zStore, false, z);
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
}

bool __cdecl CDataBase::queryf(char *fmt, ...)
{
	va_list	marker;

	TEMPSTRING(buf);
	va_start(marker, fmt);
	vsprintf(buf, fmt, marker);
	va_end(marker);

	return this->query(buf);
}

void CDataBase::exec(const char *query)
{
	if ( _bConnected && mysql_query(_myData, query) )
	{
		const char *myErr = mysql_error(_myData);
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "MySQL query \"%s\" failed due to \"%s\"\n",
			query, ( *myErr ? myErr : "unknown reason"));
	}
}

void __cdecl CDataBase::execf(char *fmt, ...)
{
	va_list	marker;

	TEMPSTRING(buf);
	va_start(marker, fmt);
	vsprintf(buf, fmt, marker);
	va_end(marker);

	this->exec(buf);
}

UINT CDataBase::getLastId()
{
	return mysql_insert_id(_myData);
}

bool CDataBase::OnTick()
{
	static int tickcnt = 0;
	EXC_TRY("Tick");

	if ( !g_Cfg.m_bMySql )	//	mySQL is not supported
		return true;

	//	do not ping sql server too heavily
	if ( ++tickcnt < 1000 )
		return true;
	tickcnt = 0;

	if ( _bConnected )	//	currently connected - just check that the link is alive
	{
		if ( mysql_ping(_myData) )
		{
			g_Log.Error("MySQL server link has been lost.\n");
			Close();

			if ( !Connect(g_Cfg.m_sMySqlUser, g_Cfg.m_sMySqlPass, g_Cfg.m_sMySqlDB, g_Cfg.m_sMySqlHost) )
			{
				g_Log.Error("MySQL reattach failed/timed out. MySQL support cancelled.\n");
			}
		}
	}

	return true;
	EXC_CATCH;
	return false;
}

enum DBO_TYPE
{
	DBO_CONNECTED,
	DBO_ROW,
	DBO_QTY,
};

LPCTSTR const CDataBase::sm_szLoadKeys[DBO_QTY+1] =
{
	"CONNECTED",
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
	return false;
}

bool CDataBase::r_LoadVal(CScript & s)
{
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
	EXC_TRY("WriteVal");

	// Just return 0 if MySQL is disabled
	if (!g_Cfg.m_bMySql)
	{
		sVal.FormatVal( 0 );
		return true;
	}

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1);
	switch ( index )
	{
		case DBO_CONNECTED:
			sVal.FormatVal(_bConnected);
			break;

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
	g_Log.Debug("command '%s' [%x]\n", pszKey, pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CDataBase::r_Verb(CScript & s, CTextConsole * pSrc)
{
	EXC_TRY("Verb");

	// Just return true if MySQL is disabled
	if (!g_Cfg.m_bMySql)
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
			Connect(g_Cfg.m_sMySqlUser, g_Cfg.m_sMySqlPass, g_Cfg.m_sMySqlDB, g_Cfg.m_sMySqlHost);
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
	g_Log.Debug("command '%s' args '%s' [%x]\n", s.GetKey(), s.GetArgRaw(), pSrc);
	EXC_DEBUG_END;
	return false;
}
