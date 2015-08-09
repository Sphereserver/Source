/*
 *	Copyright (c) 2010 Thomas Lowe
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

#ifndef _ODBCCONNECTION_H
#define _ODBCCONNECTION_H

#include "../common/SphereDatabasePlugin.h"

#ifndef _WIN32
typedef SQLTCHAR SQLSTATE[6];
#endif

// The first 2 characters of SQLSTATE is the error class,
// these are the ones we might be interested in
#define ODBCERR_CLASS_CONNECTION	"08" // connection error
#define ODBCERR_CLASS_DRIVER		"HY" // driver error
#define ODBCERR_CLASS_ODBC			"IM" // odbc error
#define SQLSTATE_CLASS_LENGTH		2

class OdbcConnection
{
	// Static members
private:
	static OdbcConnection * _instance;

public:
	static OdbcConnection * GetCurrentInstance(void);
	static void DestroyCurrentInstance(void);

	// Dynamic members
private:
	bool m_connected;		//	are we online?
	SQLHANDLE m_odbc;		// ODBC link
	SQLHANDLE m_connection;	// database connection
	SQLHANDLE m_statement;	// current statement

	SQLINTEGER m_lastError;				// error code from latest operation
	char m_lastErrorText[MAX_DATA_LENGTH];	// error text from latest operation
	bool m_lastErrorWasConnection;	// indicates last operation produced a connection error
	bool m_lastErrorWasDriver;		// indicates last operation produced a driver error
	bool m_lastErrorWasOdbc;		// indicates last operation produced an odbc error

public:
	OdbcConnection();
	~OdbcConnection();

	bool IsConnected(void);
	void SetConnected(bool connected);

	bool ServerDisconnect(void);
	bool ServerConnect(const char * user, const char * password, const char * database, const char * host, int port);

	int GetLastError(void);
	const char * GetLastStringError(void);

	int Execute(const char * statement);
	int Query(const char * statement);

	int LastQueryNumFields(void);
	int LastQueryFetchFields(fieldarray_t * fields);
	int LastQueryFetchRow(resultarray_t * results);

	int EscapeString(const char * inString, int inLength, char * outString);

private:
	void ClearResult(void);
	void InitOdbcData(void);
	void ClearOdbcData(void);
	void SetLastError(SQLSMALLINT handleType, SQLHANDLE handle, SQLRETURN result);
};

#endif
