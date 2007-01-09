/*
*	Copyright (c) 2006 Francesco Furiani
*
*	Permission is hereby granted, free of charge, to any person
*	obtaining a copy of this software and associated documentation
*	files (the "Software"), to deal in the Software without
*	restriction, including without limitation the rights to use,
*	copy, modify, merge, publish, distribute, sublicense, and/or sell
*	copies of the Software, and to permit persons to whom the
*	Software is furnished to do so, subject to the following
*	conditions:
*
*	The above copyright notice and this permission notice shall be
*	included in all copies or substantial portions of the Software.
*
*	Except as contained in this notice, the name(s) of the above 
*	copyright holders shall not be used in advertising or otherwise 
*	to promote the sale, use or other dealings in this Software 
*	without prior written authorization.
*
*	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
*	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
*	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
*	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
*	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
*	OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef _MYSQLDLL_H
#define _MYSQLDLL_H

#ifdef _WIN32
	#define MYSQLDLL_API
	#define __CALLTYPE __stdcall
#else
	#define MYSQLDLL_API extern "C"
	#define __CALLTYPE
#endif

// DLLs specific things
#ifdef _WIN32
	BOOL APIENTRY DllMain( HANDLE, DWORD, LPVOID );
#else
	void __attribute__((constructor)) dll_init(void);
	void __attribute__((destructor)) dll_fini(void);
#endif

// Defines
#define WRAPPER_RESULT_NOQUERYRES 1
#define WRAPPER_INTERNAL_ERROR -1
#define WRAPPER_API_SPECIFIC_ERROR -(0xBEEF)
#define WRAPPER_UNKNOWN_ERROR -(0xDEDE)
#define WRAPPER_SERVER_LOST_ERROR -(0xDEAD)

#define MAX_FIELD_NAME 512
#define MAX_DATA_LENGTH 4096
#define	MIN_MYSQL_VERSION_ALLOW	40115

// Interface objects
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

// Class Wrapper
class MySqlWrapper
{
	// Static members
private:
	static MySqlWrapper * _mysqlWrapper;

public:
	static MySqlWrapper * GetCurrentWrapper(void);
	static void DestroyCurrentWrapper(void);

	// Dynamic members
private:
	bool m_bConnected;			//	are we online?
	MYSQL * m_myData;			//	mySQL link
	MYSQL_RES * m_curResult;	//	last result

public:
	MySqlWrapper();
	~MySqlWrapper();

	bool IsConnected(void);
	void SetConnected(bool);
	
	unsigned long GetClientVersion(void);
	int ServerPing(void);

	bool ServerDisconnect(void);
	bool ServerConnect(const char *, const char *, const char *, const char *, int);

	int GetLastError(void);
	const char * GetLastStringError(void);

	int Execute(const char *);
	int Query(const char *);

	int LastQueryNumFields(void);
	int LastQueryNumRows(void);
	int LastQueryFetchFields(fieldarray_t *);
	int LastQueryFetchRow(resultarray_t *);

	int EscapeString(const char *, int, char *);

private:
	void ClearResult(void);
	void InitMySqlData(void);
	void ClearMySqlData(void);

};

// Exported things
MYSQLDLL_API bool __CALLTYPE db_isrightversion(void);
MYSQLDLL_API int __CALLTYPE db_ping(void);
MYSQLDLL_API bool __CALLTYPE db_close(void);
MYSQLDLL_API bool __CALLTYPE db_isconnected(void);
MYSQLDLL_API int __CALLTYPE db_getlasterror(void);
MYSQLDLL_API const char * __CALLTYPE db_getlaststringerror();
MYSQLDLL_API bool __CALLTYPE db_connect(char **);
MYSQLDLL_API int __CALLTYPE db_execute(const char *);
MYSQLDLL_API int __CALLTYPE db_query(const char *);
MYSQLDLL_API int __CALLTYPE db_fetchfields(fieldarray_t *);
MYSQLDLL_API int __CALLTYPE db_numfields(void);
MYSQLDLL_API int __CALLTYPE db_numrows(void);
MYSQLDLL_API int __CALLTYPE db_fetchrow(resultarray_t *);
MYSQLDLL_API int __CALLTYPE db_escapestring(const char *, int, char *);

#endif