#ifdef _DBPLUGIN
#ifndef CDATABASELOADER_H
#define	CDATABASELOADER_H

#include "common.h"
#include "../dbdll/common/SphereDatabasePlugin.h"

#ifdef _WIN32
	typedef HMODULE dllhandle_t;
#else
	typedef void * dllhandle_t;
	#ifndef IN
		#define IN
		#define OUT
	#endif
#endif

class cDatabaseLoader
{
// function types
private:
	typedef bool (__CALLTYPE *dfRightVer_t)();
	typedef int (__CALLTYPE *dfPing_t)();
	typedef bool (__CALLTYPE *dfClose_t)();
	typedef bool (__CALLTYPE *dfConnected_t)();
	typedef int	(__CALLTYPE *dfLastError_t)();
	typedef const char * (*dfLastCharError_t)();
	typedef bool (__CALLTYPE *dfConnect_t)(IN char **, IN int);
	typedef int (__CALLTYPE *dfExec_t)(IN const char *);
	typedef int (__CALLTYPE *dfQuery_t)(IN const char *);
	typedef int (__CALLTYPE *dfFieldArray_t)(OUT fieldarray_t *);
	typedef int (__CALLTYPE *dfFieldNum_t)();
	typedef int (__CALLTYPE *dfFetchRow_t)(OUT resultarray_t *);
	typedef int (__CALLTYPE *dfEscapeString_t)(IN const char *, IN int, OUT char *);

// static
private:
	static cDatabaseLoader * pCurrentInstance;
public:
	static const char *m_sClassName;

public:
	static cDatabaseLoader * GetCurrentInstance( void );
	static void ForceInstanceReload( void );


// dynamic
private:
	// Dll handle
	dllhandle_t dhDatabase;

	// Function pointer
	dfRightVer_t pfRightVersion;
	dfPing_t pfPing;
	dfConnect_t pfConnect;
	dfClose_t pfClose;
	dfConnected_t pfIsConnected;
	dfLastError_t pfLastError;
	dfLastCharError_t pfLastCharError;
	dfExec_t pfExecute;
	dfQuery_t pfQuery;
	dfFieldArray_t pfFieldArray;
	dfFieldNum_t pfFieldNum;
	dfFetchRow_t pfFetchRow;
	dfEscapeString_t pfEscapeString;


public:
	cDatabaseLoader();
	~cDatabaseLoader();
private:
	cDatabaseLoader(const cDatabaseLoader& copy);
	cDatabaseLoader& operator=(const cDatabaseLoader& other);

public:
	bool IsReady();

	bool DbIsRightVersion( void );
	int DbPing( void );
	bool DbConnect( const char *, const char *, const char *, const char *, int );
	bool DbClose( void );
	bool DbIsConnected( void );
	int	DbGetLastError( void );
	const char * DbGetLastErrorString( void );
	int DbQuery( const char * );
	int DbExecute( const char * );
	int DbNumFields( void );
	int DbFetchFields( fieldarray_t * );
	int DbFetchRow( resultarray_t * );
	int DbEscapeString( const char *, int, char * );

private:
	dllhandle_t SetDllHandle( const char * );
	void * SetFunctionHandle( const char * );
	void CloseDllHandle( void );
	bool FillFunctionsHandle( void );
};

#undef __CALLTYPE

#endif
#endif
