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

#include "stdafx.h"
#include "MySqlDll.h"

#ifdef _WIN32
	BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
	{
		UNREFERENCED_PARAMETER(hModule);
		UNREFERENCED_PARAMETER(lpReserved);

		switch (ul_reason_for_call)
		{
			case DLL_THREAD_ATTACH:
			case DLL_THREAD_DETACH:
				break;

			case DLL_PROCESS_ATTACH:
			{
				MySqlWrapper::GetCurrentWrapper();
			} break;

		/*
			case DLL_PROCESS_DETACH:
			{
				MySqlWrapper::DestroyCurrentWrapper();
			} break;
		*/


		}

		return TRUE;
	}
#else
	void dll_init()
	{
		MySqlWrapper::GetCurrentWrapper();
	}

	void dll_fini()
	{
		MySqlWrapper::DestroyCurrentWrapper();
	}
#endif

// ------------------------------------------

MySqlWrapper * MySqlWrapper::_mysqlWrapper = NULL;

MySqlWrapper * MySqlWrapper::GetCurrentWrapper()
{
	if ( MySqlWrapper::_mysqlWrapper == NULL )
	{
		MySqlWrapper::_mysqlWrapper = new MySqlWrapper();
	}

	return MySqlWrapper::_mysqlWrapper;
}

void MySqlWrapper::DestroyCurrentWrapper()
{
	if ( MySqlWrapper::_mysqlWrapper != NULL )
	{
		delete MySqlWrapper::_mysqlWrapper;
	}
}

MySqlWrapper::MySqlWrapper()
{
	m_myData = NULL;
	m_curResult = NULL;
	SetConnected(false);
}

MySqlWrapper::~MySqlWrapper()
{
	ClearResult();
	ClearMySqlData();

	SetConnected(false);
}

void MySqlWrapper::ClearResult()
{
	if ( m_curResult != NULL )
	{
		mysql_free_result(m_curResult);
		m_curResult = NULL;
	}
}

void MySqlWrapper::InitMySqlData()
{
	if ( m_myData == NULL )
	{
		m_myData = mysql_init(NULL);
	}
}

void MySqlWrapper::ClearMySqlData()
{
	if ( m_myData != NULL )
	{
		mysql_close(m_myData);
		m_myData = NULL;
	}
}


bool MySqlWrapper::IsConnected()
{
	return m_bConnected;
}

void MySqlWrapper::SetConnected( bool bInput )
{
	m_bConnected = bInput;
}

unsigned long MySqlWrapper::GetClientVersion()
{
	return mysql_get_client_version();
}

int MySqlWrapper::ServerPing()
{
	return mysql_ping(m_myData);
}

bool MySqlWrapper::ServerDisconnect()
{
	ClearResult();
	ClearMySqlData();

	SetConnected(false);

	return IsConnected();
}

bool MySqlWrapper::ServerConnect(const char * user, const char * password, const char * database, const char * hostip, int hostport)
{
	InitMySqlData();

	MYSQL * iResult = mysql_real_connect(m_myData, hostip, user, password, database, hostport, NULL, CLIENT_MULTI_STATEMENTS );

	if ( !iResult )
	{
		ServerDisconnect();
	}
	else
	{
		SetConnected(true);
	}

	return IsConnected();
}

int MySqlWrapper::GetLastError()
{
	if ( m_myData != NULL )
	{
		return mysql_errno(m_myData);
	}
	
	return WRAPPER_INTERNAL_ERROR;
}

const char * MySqlWrapper::GetLastStringError()
{
	if ( m_myData != NULL )
	{
		return mysql_error(m_myData);
	}

	return NULL;
}

int MySqlWrapper::Execute(const char * statement)
{
	if ( m_myData && statement && *statement )
	{
			int queryResult = mysql_query(m_myData, statement);
			if (( queryResult == CR_SERVER_GONE_ERROR ) || ( queryResult == CR_SERVER_LOST ))
			{
				return WRAPPER_SERVER_LOST_ERROR;
			}
			else if ( queryResult == CR_UNKNOWN_ERROR )
			{
				return WRAPPER_UNKNOWN_ERROR;
			}
			else if ( queryResult == CR_COMMANDS_OUT_OF_SYNC )
			{
				return WRAPPER_API_SPECIFIC_ERROR;
			}

			return 0;
	}
	
	return WRAPPER_INTERNAL_ERROR;
}

int MySqlWrapper::Query(const char * statement)
{
	if ( m_myData && statement && *statement )
	{
		unsigned long queryResult = mysql_query(m_myData, statement);
		if (( queryResult == CR_SERVER_GONE_ERROR ) || ( queryResult == CR_SERVER_LOST ))
		{
			return WRAPPER_SERVER_LOST_ERROR;
		}
		else if ( queryResult == CR_UNKNOWN_ERROR )
		{
			return WRAPPER_UNKNOWN_ERROR;
		}
		else if ( queryResult == CR_COMMANDS_OUT_OF_SYNC )
		{
			return WRAPPER_API_SPECIFIC_ERROR;
		}

		ClearResult();
		m_curResult = mysql_store_result(m_myData);
		
		if ( m_curResult == NULL )
		{
			int resultError = mysql_errno(m_myData);

			if ( resultError == 0 )
			{
				return WRAPPER_RESULT_NOQUERYRES;
			}
			else if (( resultError == CR_SERVER_GONE_ERROR ) || ( resultError == CR_SERVER_LOST ))
			{
				return WRAPPER_SERVER_LOST_ERROR;
			}
			else if ( resultError == CR_OUT_OF_MEMORY )
			{
				return WRAPPER_API_SPECIFIC_ERROR;
			}

			return WRAPPER_UNKNOWN_ERROR;
		}

		return 0;
	}

	return WRAPPER_INTERNAL_ERROR;
}

int MySqlWrapper::LastQueryNumFields()
{
	if ( m_myData && m_curResult )
	{
		return mysql_num_fields(m_curResult);
	}

	return WRAPPER_INTERNAL_ERROR;
}

int MySqlWrapper::LastQueryNumRows()
{
	if ( m_myData && m_curResult )
	{
		return mysql_num_rows(m_curResult);
	}

	return WRAPPER_INTERNAL_ERROR;
}

int MySqlWrapper::LastQueryFetchFields(fieldarray_t * fields)
{
	if ( m_myData && m_curResult )
	{
		MYSQL_FIELD *msFields = mysql_fetch_fields(m_curResult);
		int numField = mysql_num_fields(m_curResult);

		for ( int i = 0; i < numField; i++ )
		{
			strncpy(fields[i].name, msFields[i].name, (MAX_FIELD_NAME - 1));
		}

		return numField;
	}

	return WRAPPER_INTERNAL_ERROR;
}

int MySqlWrapper::LastQueryFetchRow(resultarray_t * results)
{
	if ( m_myData && m_curResult )
	{
		char ** row = NULL;
		row = mysql_fetch_row(m_curResult);

		if ( row == NULL )
		{
			int resultError = mysql_errno(m_myData);

			if ( resultError == 0 )
			{
				return 0;
			}
			else if ( resultError == CR_SERVER_LOST )
			{
				return WRAPPER_SERVER_LOST_ERROR;
			}

			return WRAPPER_UNKNOWN_ERROR;
		}

		int num_fields = mysql_num_fields(m_curResult);

		for ( int i = 0; i < num_fields; i++ )
		{
			strncpy(results[i].data, row[i], (MAX_DATA_LENGTH - 1));
		}

		return num_fields;
	}

	return WRAPPER_INTERNAL_ERROR;
}

int MySqlWrapper::EscapeString(const char * inString, int inLength, char * outString)
{
	if ( IsConnected() )
	{
		return mysql_real_escape_string(m_myData, outString, inString, inLength);
	}
	else
	{
		return mysql_escape_string(outString, inString, inLength);
	}
}

// ------------------------------------------

MYSQLDLL_API bool __CALLTYPE db_isrightversion()
{
	return (MySqlWrapper::GetCurrentWrapper()->GetClientVersion() >= MIN_MYSQL_VERSION_ALLOW);
}

MYSQLDLL_API int __CALLTYPE db_ping()
{
	return MySqlWrapper::GetCurrentWrapper()->ServerPing();
}

MYSQLDLL_API bool __CALLTYPE db_isconnected()
{
	return MySqlWrapper::GetCurrentWrapper()->IsConnected();
}

MYSQLDLL_API bool __CALLTYPE db_connect(char ** args)
{
	return MySqlWrapper::GetCurrentWrapper()->ServerConnect(args[0],args[1],args[2],args[3],atoi(args[4]));
}

MYSQLDLL_API bool __CALLTYPE db_close()
{
	return MySqlWrapper::GetCurrentWrapper()->ServerDisconnect();
}

MYSQLDLL_API int __CALLTYPE db_getlasterror()
{
	return MySqlWrapper::GetCurrentWrapper()->GetLastError();
}

MYSQLDLL_API const char * __CALLTYPE db_getlaststringerror()
{
	return MySqlWrapper::GetCurrentWrapper()->GetLastStringError();
}
	
MYSQLDLL_API int __CALLTYPE db_execute(const char * query)
{
	return MySqlWrapper::GetCurrentWrapper()->Execute(query);
}

MYSQLDLL_API int __CALLTYPE db_query(const char * query)
{
	return MySqlWrapper::GetCurrentWrapper()->Query(query);
}

MYSQLDLL_API int __CALLTYPE db_numfields()
{
	return MySqlWrapper::GetCurrentWrapper()->LastQueryNumFields();
}

MYSQLDLL_API int __CALLTYPE db_numrows()
{
	return MySqlWrapper::GetCurrentWrapper()->LastQueryNumRows();
}

MYSQLDLL_API int __CALLTYPE db_fetchfields(fieldarray_t * fields)
{
	return MySqlWrapper::GetCurrentWrapper()->LastQueryFetchFields(fields);
}

MYSQLDLL_API int __CALLTYPE db_fetchrow(resultarray_t * results)
{
	return MySqlWrapper::GetCurrentWrapper()->LastQueryFetchRow(results);
}

MYSQLDLL_API int __CALLTYPE db_escapestring(const char * inString, int inLength, char * outString)
{
	return MySqlWrapper::GetCurrentWrapper()->EscapeString(inString, inLength, outString);
}
