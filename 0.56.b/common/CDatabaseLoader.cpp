#ifdef _DBPLUGIN
#include "CDatabaseLoader.h"
#include "../graysvr/graysvr.h"

cDatabaseLoader * cDatabaseLoader::pCurrentInstance = NULL;

cDatabaseLoader * cDatabaseLoader::GetCurrentInstance()
{
	ADDTOCALLSTACK("cDatabaseLoader::GetCurrentInstance");
	if ( pCurrentInstance == NULL )
		pCurrentInstance = new cDatabaseLoader();

	return pCurrentInstance;
}

void cDatabaseLoader::ForceInstanceReload()
{
	ADDTOCALLSTACK("cDatabaseLoader::ForceInstanceReload");
	if ( pCurrentInstance != NULL )
	{
		delete pCurrentInstance;
	}

	pCurrentInstance = new cDatabaseLoader();
}

// ---------------------------------------------------
// ---------------------------------------------------
// ---------------------------------------------------

cDatabaseLoader::cDatabaseLoader()
{
	dhDatabase = (dllhandle_t) NULL;
	CloseDllHandle();

	if ( !g_Cfg.m_sDbDll.IsEmpty() )
	{
#ifdef _WIN32
		TCHAR DbDll[4096];

		::GetCurrentDirectory(4096, DbDll);
		if(DbDll[0] != '\0')
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

		if ( dhDatabase != NULL )
		{
			if ( FillFunctionsHandle() == false )
			{
				CloseDllHandle();
				DEBUG_ERR(("DBDLL: Database plugin %s is incompatible.\n", g_Cfg.m_sDbDll.GetPtr()));
			}
		}
		else
		{
#ifdef _WIN32
			TCHAR lpszError[256];
			CGrayError::GetSystemErrorMessage( GetLastError(), lpszError, COUNTOF(lpszError) );

			DEBUG_ERR(("DBDLL: Cannot open the database plugin %s - %s\n", g_Cfg.m_sDbDll.GetPtr(), lpszError));
#else
			DEBUG_ERR(("DBDLL: Cannot load the database plugin %s - %s\n", g_Cfg.m_sDbDll.GetPtr(), dlerror()));
#endif
		}
	}
}

cDatabaseLoader::~cDatabaseLoader()
{
	CloseDllHandle();
}

bool cDatabaseLoader::IsReady()
{
	ADDTOCALLSTACK("cDatabaseLoader::IsReady");
	return ( dhDatabase != NULL );
}

#ifdef _DBPLUGIN

dllhandle_t cDatabaseLoader::SetDllHandle( const char * dllPath )
{
	ADDTOCALLSTACK("cDatabaseLoader::SetDllHandle");
#ifdef _WIN32
	return LoadLibrary(dllPath);
#else
	return dlopen(dllPath, RTLD_LAZY);
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

#endif

void cDatabaseLoader::CloseDllHandle()
{
	ADDTOCALLSTACK("cDatabaseLoader::CloseDllHandle");
	if ( dhDatabase != NULL )
	{
#ifdef _WIN32
		FreeLibrary(dhDatabase);
#else
		dlclose(dhDatabase);
#endif
		dhDatabase = NULL;
	}

	pfRightVersion = NULL;
	pfPing = NULL;
	pfConnect = NULL;
	pfClose = NULL;
	pfIsConnected = NULL;
	pfLastError = NULL;
	pfLastCharError = NULL;
	pfExecute = NULL;
	pfQuery = NULL;
	pfFieldNum = NULL;
	pfFieldArray = NULL;
	pfFetchRow = NULL;
	pfEscapeString = NULL;
}

bool cDatabaseLoader::FillFunctionsHandle()
{
	ADDTOCALLSTACK("cDatabaseLoader::FillFunctionsHandle");
	pfRightVersion = reinterpret_cast<dfRightVer_t>(SetFunctionHandle("db_isrightversion"));
	pfPing = reinterpret_cast<dfPing_t>(SetFunctionHandle("db_ping"));
	pfConnect = reinterpret_cast<dfConnect_t>(SetFunctionHandle("db_connect"));
	pfClose = reinterpret_cast<dfClose_t>(SetFunctionHandle("db_close"));
	pfIsConnected = reinterpret_cast<dfConnected_t>(SetFunctionHandle("db_isconnected"));
	pfLastError = reinterpret_cast<dfLastError_t>(SetFunctionHandle("db_getlasterror"));
	pfLastCharError = reinterpret_cast<dfLastCharError_t>(SetFunctionHandle("db_getlaststringerror"));
	pfExecute = reinterpret_cast<dfExec_t>(SetFunctionHandle("db_execute"));
	pfQuery = reinterpret_cast<dfQuery_t>(SetFunctionHandle("db_query"));
	pfFieldNum = reinterpret_cast<dfFieldNum_t>(SetFunctionHandle("db_numfields"));
	pfFieldArray = reinterpret_cast<dfFieldArray_t>(SetFunctionHandle("db_fetchfields"));
	pfFetchRow = reinterpret_cast<dfFetchRow_t>(SetFunctionHandle("db_fetchrow"));
	pfEscapeString = reinterpret_cast<dfEscapeString_t>(SetFunctionHandle("db_escapestring"));

	// check all functions exist
	return( pfRightVersion != NULL && pfPing != NULL &&
			pfConnect != NULL && pfClose != NULL &&
			pfIsConnected != NULL && pfLastError != NULL &&
			pfLastCharError != NULL && pfExecute != NULL &&
			pfQuery != NULL && pfFieldNum != NULL &&
			pfFieldArray != NULL && pfFetchRow != NULL &&
			pfEscapeString != NULL );
}

// ---------------------------------------------------

bool cDatabaseLoader::DbIsRightVersion()
{
	ADDTOCALLSTACK("cDatabaseLoader::DbIsRightVersion");
	if ( IsReady() == false )
		return false;

	return pfRightVersion();
}

int cDatabaseLoader::DbPing()
{
	ADDTOCALLSTACK("cDatabaseLoader::DbPing");
	if ( IsReady() == false )
		return DBPLUGIN_INTERNAL_ERROR;

	return pfPing();
}

bool cDatabaseLoader::DbConnect( const char * user, const char * password, const char * dbname, const char * hostip, int hostport )
{
	ADDTOCALLSTACK("cDatabaseLoader::DbConnect");
	if ( IsReady() == false )
		return false;

	TCHAR	*buf = Str_GetTemp();
	TCHAR	*Arg_ppCmd[5];

	sprintf(buf, "%s,%s,%s,%s,%d", user, password, dbname, hostip, hostport);
	size_t iQty = Str_ParseCmds(buf, Arg_ppCmd, COUNTOF(Arg_ppCmd), ",");

	return pfConnect(Arg_ppCmd, iQty);
}

bool cDatabaseLoader::DbClose()
{
	ADDTOCALLSTACK("cDatabaseLoader::DbClose");
	if ( IsReady() == false )
		return false;

	return pfClose();
}

bool cDatabaseLoader::DbIsConnected()
{
	ADDTOCALLSTACK("cDatabaseLoader::DbIsConnected");
	if ( IsReady() == false )
		return false;

	return pfIsConnected();
}

int	cDatabaseLoader::DbGetLastError()
{
	ADDTOCALLSTACK("cDatabaseLoader::DbGetLastError");
	if ( IsReady() == false )
		return DBPLUGIN_INTERNAL_ERROR;

	return pfLastError();
}

const char * cDatabaseLoader::DbGetLastErrorString()
{
	ADDTOCALLSTACK("cDatabaseLoader::DbGetLastErrorString");
	if ( IsReady() == false )
		return "Database plugin not loaded";

	return pfLastCharError();
}

int cDatabaseLoader::DbExecute( const char * query )
{
	ADDTOCALLSTACK("cDatabaseLoader::DbExecute");
	if ( IsReady() == false )
		return DBPLUGIN_INTERNAL_ERROR;

	return pfExecute(query);
}

int cDatabaseLoader::DbQuery( const char * query )
{
	ADDTOCALLSTACK("cDatabaseLoader::DbQuery");
	if ( IsReady() == false )
		return DBPLUGIN_INTERNAL_ERROR;

	return pfQuery(query);
}

int cDatabaseLoader::DbNumFields()
{
	ADDTOCALLSTACK("cDatabaseLoader::DbNumFields");
	if ( IsReady() == false )
		return DBPLUGIN_INTERNAL_ERROR;

	return pfFieldNum();
}

int cDatabaseLoader::DbFetchFields( fieldarray_t * fields )
{
	ADDTOCALLSTACK("cDatabaseLoader::DbFetchFields");
	if ( IsReady() == false )
		return DBPLUGIN_INTERNAL_ERROR;

	return pfFieldArray(fields);
}

int cDatabaseLoader::DbFetchRow( resultarray_t * results )
{
	ADDTOCALLSTACK("cDatabaseLoader::DbFetchRow");
	if ( IsReady() == false )
		return DBPLUGIN_INTERNAL_ERROR;

	return pfFetchRow(results);
}

int cDatabaseLoader::DbEscapeString( const char * inString, int inputLength, char * outString )
{
	ADDTOCALLSTACK("cDatabaseLoader::DbEscapeString");
	if ( IsReady() == false )
		return DBPLUGIN_INTERNAL_ERROR;

	return pfEscapeString(inString, inputLength, outString);
}

#endif
