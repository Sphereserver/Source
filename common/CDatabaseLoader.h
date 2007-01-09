#ifndef CDATABASELOADER_H
#define	CDATABASELOADER_H
#ifdef _EXTERNAL_DLL

#include "common.h"

#ifdef _WIN32
	#define __CALLTYPE __stdcall
	typedef HMODULE dllhandle_t;
#else
	#define __CALLTYPE
	typedef void * dllhandle_t;
	#ifndef IN
		#define IN
		#define OUT
	#endif
#endif

// Interface things
#define WRAPPER_RESULT_NOQUERYRES 1
#define WRAPPER_INTERNAL_ERROR -1
#define WRAPPER_API_SPECIFIC_ERROR -(0xBEEF)
#define WRAPPER_UNKNOWN_ERROR -(0xDEDE)
#define WRAPPER_SERVER_LOST_ERROR -(0xDEAD)

#define MAX_FIELD_NAME 512
#define MAX_DATA_LENGTH 4096

struct stFieldContainer
{
	char name[MAX_FIELD_NAME];
};

typedef struct stFieldContainer fieldarray_t;

struct stResultContainer
{
	char data[MAX_DATA_LENGTH];
};

typedef struct stResultContainer resultarray_t;
// ---------------------------------------------

class cDatabaseLoader
{
// function types
private:
	typedef bool (__CALLTYPE *dfRightver_t)();
	typedef int (__CALLTYPE *dfPing_t)();
	typedef bool (__CALLTYPE *dfClose_t)();
	typedef bool (__CALLTYPE *dfConnected_t)();
	typedef int	(__CALLTYPE *dfLasterror_t)();
	typedef const char * (*dfLastcharerror_t)();
	typedef bool (__CALLTYPE *dfConnect_t)(IN char **);
	typedef int (__CALLTYPE *dfExec_t)(IN const char *);
	typedef int (__CALLTYPE *dfQuery_t)(IN const char *);
	typedef int (__CALLTYPE *dfFieldarray_t)(OUT fieldarray_t *);
	typedef int (__CALLTYPE *dfFieldnum_t)();
	typedef int (__CALLTYPE *dfNumrows_t)();
	typedef int (__CALLTYPE *dfFetchrow_t)(OUT resultarray_t *);
	typedef int (__CALLTYPE *dfEscapeString_t)(IN const char *, IN int, OUT char *);

// static
private:
	static cDatabaseLoader * pCurrentIstance;
public:
	static const char *m_sClassName;

public:
	static cDatabaseLoader * GetCurrentIstance( void );
	static void ForceIstanceReload( void );


// dynamic
private:
	// Dll handle
	dllhandle_t dhDatabase;
	// Function pointer
	dfRightver_t pfRightversion;
	dfPing_t pfPing;
	dfConnect_t pfConnect;
	dfClose_t pfClose;
	dfConnected_t pfIsconnect;
	dfLasterror_t pfLasterror;
	dfLastcharerror_t pfLastcharerror;
	dfExec_t pfExecute;
	dfQuery_t pfQuery;
	dfFieldarray_t pfFieldarray;
	dfFieldnum_t pfFieldnum;
	dfNumrows_t pfNumrows;
	dfFetchrow_t pfFetchrow;
	dfEscapeString_t pfEscapestring;


public:
	cDatabaseLoader();
	~cDatabaseLoader();
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
	int DbNumRows( void );
	int DbFetchRow( resultarray_t * );
	int DbEscapeString( const char *, int, char * );

private:
	dllhandle_t SetDllHandle( const char * );
	void CloseDllHandle( void );
	void * SetFunctionHandle( const char * );

	bool FillFunctionsHandle( void );
};

#undef __CALLTYPE

#endif
#endif