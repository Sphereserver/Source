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

#ifndef _MYSQLCONNECTION_H
#define _MYSQLCONNECTION_H

#define	MIN_MYSQL_VERSION_ALLOW    40115
#include "../common/SphereDatabasePlugin.h"

class MySqlConnection
{
	// Static members
private:
	static MySqlConnection * _mysqlConnection;

public:
	static MySqlConnection * GetCurrentInstance(void);
	static void DestroyCurrentInstance(void);

	// Dynamic members
private:
	bool m_connected;			//	are we online?
	MYSQL * m_myData;			//	mySQL link
	MYSQL_RES * m_curResult;	//	last result

public:
	MySqlConnection();
	~MySqlConnection();

	bool IsConnected(void);
	void SetConnected(bool connected);
	
	unsigned long GetClientVersion(void);
	int ServerPing(void);

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
	void InitMySqlData(void);
	void ClearMySqlData(void);
};

#endif
