#ifdef _EXTERNAL_DLL

#include "CDatabaseLoader.h"
#include "../graysvr/graysvr.h"

cDatabaseLoader * cDatabaseLoader::pCurrentIstance = NULL;

cDatabaseLoader * cDatabaseLoader::GetCurrentIstance()
{
	ADDTOCALLSTACK("cDatabaseLoader::GetCurrentIstance");
	if ( pCurrentIstance == NULL )
		pCurrentIstance = new cDatabaseLoader();

	return pCurrentIstance;
}

void cDatabaseLoader::ForceIstanceReload()
{
	ADDTOCALLSTACK("cDatabaseLoader::ForceIstanceReload");
	if ( pCurrentIstance != NULL )
	{
		delete pCurrentIstance;
	}

	pCurrentIstance = new cDatabaseLoader();
}

// ---------------------------------------------------
// ---------------------------------------------------
// ---------------------------------------------------

cDatabaseLoader::cDatabaseLoader()
{
	if ( !g_Cfg.m_sDbDll.IsEmpty() )
	{
#ifdef _WIN32
		char DbDll[4096];

		::GetCurrentDirectory(4096, DbDll);
		if(DbDll != NULL)
		{
			strcat(DbDll, "\\");
			strcat(DbDll, g_Cfg.m_sDbDll.GetPtr());
		}
		else
		{
			memset(DbDll, 0, sizeof(DbDll));
			strcat(DbDll,g_Cfg.m_sDbDll.GetPtr());
		}

		dhDatabase = SetDllHandle(DbDll);
#else
		CGString DbDll;
		DbDll.Format("./%s", g_Cfg.m_sDbDll.GetPtr());

		dhDatabase = SetDllHandle(DbDll.GetPtr());
#endif

		if ( dhDatabase )
		{
			FillFunctionsHandle();
		}
		else
		{
			DEBUG_ERR(("DBDLL: Cannot load the dll %s.\n", g_Cfg.m_sDbDll.GetPtr()));
		}
	}
}

cDatabaseLoader::~cDatabaseLoader()
{
	if ( dhDatabase )
		CloseDllHandle();
}

bool cDatabaseLoader::IsReady()
{
	ADDTOCALLSTACK("cDatabaseLoader::IsReady");
	return ( dhDatabase != NULL );
}

dllhandle_t cDatabaseLoader::SetDllHandle( const char * dllPath )
{
	ADDTOCALLSTACK("cDatabaseLoader::SetDllHandle");
#ifdef _WIN32
	return LoadLibrary(dllPath);
#else
	return dlopen(dllPath, RTLD_LAZY);
#endif
}

void cDatabaseLoader::CloseDllHandle()
{
	ADDTOCALLSTACK("cDatabaseLoader::CloseDllHandle");
#ifdef _WIN32
	FreeLibrary(dhDatabase);
#else
	dlclose(dhDatabase);
#endif
}

void * cDatabaseLoader::SetFunctionHandle( const char * functionName )
{
	ADDTOCALLSTACK("cDatabaseLoader::SetFunctionHandle");
#ifdef _WIN32
	return GetProcAddress(dhDatabase, functionName);
#else
	return dlsym(dhDatabase, functionName);
#endif
}

bool cDatabaseLoader::FillFunctionsHandle()
{
	ADDTOCALLSTACK("cDatabaseLoader::FillFunctionsHandle");
	pfRightversion = (dfRightver_t) SetFunctionHandle("db_isrightversion");
	pfPing = (dfPing_t) SetFunctionHandle("db_ping");
	pfConnect = (dfConnect_t) SetFunctionHandle("db_connect");
	pfClose = (dfClose_t) SetFunctionHandle("db_close");
	pfIsconnect = (dfConnected_t) SetFunctionHandle("db_isconnected");
	pfLasterror = (dfLasterror_t) SetFunctionHandle("db_getlasterror");
	pfLastcharerror = (dfLastcharerror_t) SetFunctionHandle("db_getlaststringerror");
	pfExecute = (dfExec_t) SetFunctionHandle("db_execute");
	pfQuery = (dfQuery_t) SetFunctionHandle("db_query");
	pfFieldnum = (dfFieldnum_t) SetFunctionHandle("db_numfields");
	pfFieldarray = (dfFieldarray_t) SetFunctionHandle("db_fetchfields");
	pfNumrows = (dfNumrows_t) SetFunctionHandle("db_numrows");
	pfFetchrow = (dfFetchrow_t) SetFunctionHandle("db_fetchrow");
	pfEscapestring = (dfEscapeString_t) SetFunctionHandle("db_escapestring");

	return true;
}

// ---------------------------------------------------

bool cDatabaseLoader::DbIsRightVersion()
{
	ADDTOCALLSTACK("cDatabaseLoader::DbIsRightVersion");
	return pfRightversion();
}

int cDatabaseLoader::DbPing()
{
	ADDTOCALLSTACK("cDatabaseLoader::DbPing");
	return pfPing();
}

bool cDatabaseLoader::DbConnect( const char * user, const char * password, const char * dbname, const char * hostip, int hostport )
{
	ADDTOCALLSTACK("cDatabaseLoader::DbConnect");
	TCHAR	*buf = Str_GetTemp();
	TCHAR	*Arg_ppCmd[5];

	sprintf(buf, "%s,%s,%s,%s,%d", user, password, dbname, hostip, hostport);
	int iQty = Str_ParseCmds(buf, Arg_ppCmd, COUNTOF(Arg_ppCmd), ",");

	return pfConnect(Arg_ppCmd);
}

bool cDatabaseLoader::DbClose()
{
	ADDTOCALLSTACK("cDatabaseLoader::DbClose");
	return pfClose();
}

bool cDatabaseLoader::DbIsConnected()
{
	ADDTOCALLSTACK("cDatabaseLoader::DbIsConnected");
	return pfIsconnect();
}

int	cDatabaseLoader::DbGetLastError()
{
	ADDTOCALLSTACK("cDatabaseLoader::DbGetLastError");
	return pfLasterror();
}

const char * cDatabaseLoader::DbGetLastErrorString()
{
	ADDTOCALLSTACK("cDatabaseLoader::DbGetLastErrorString");
	return pfLastcharerror();
}

int cDatabaseLoader::DbExecute( const char * query )
{
	ADDTOCALLSTACK("cDatabaseLoader::DbExecute");
	return pfExecute(query);
}

int cDatabaseLoader::DbQuery( const char * query )
{
	ADDTOCALLSTACK("cDatabaseLoader::DbQuery");
	return pfQuery(query);
}

int cDatabaseLoader::DbNumFields()
{
	ADDTOCALLSTACK("cDatabaseLoader::DbNumFields");
	return pfFieldnum();
}

int cDatabaseLoader::DbFetchFields( fieldarray_t * fields )
{
	ADDTOCALLSTACK("cDatabaseLoader::DbFetchFields");
	return pfFieldarray(fields);
}

int cDatabaseLoader::DbNumRows()
{
	ADDTOCALLSTACK("cDatabaseLoader::DbNumRows");
	return pfNumrows();
}

int cDatabaseLoader::DbFetchRow( resultarray_t * results )
{
	ADDTOCALLSTACK("cDatabaseLoader::DbFetchRow");
	return pfFetchrow(results);
}

int cDatabaseLoader::DbEscapeString( const char * inString, int inputLength, char * outString )
{
	ADDTOCALLSTACK("cDatabaseLoader::DbEscapeString");
	return pfEscapestring(inString, inputLength, outString);
}

#endif