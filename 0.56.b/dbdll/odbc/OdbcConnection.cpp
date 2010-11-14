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

#include "stdafx.h"
#include "OdbcConnection.h"

OdbcConnection * OdbcConnection::_instance = NULL;

OdbcConnection * OdbcConnection::GetCurrentInstance()
{
	if ( OdbcConnection::_instance == NULL )
	{
		OdbcConnection::_instance = new OdbcConnection();
	}

	return OdbcConnection::_instance;
}

void OdbcConnection::DestroyCurrentInstance()
{
	if ( OdbcConnection::_instance != NULL )
	{
		delete OdbcConnection::_instance;
		OdbcConnection::_instance = NULL;
	}
}

OdbcConnection::OdbcConnection()
{
	m_odbc = NULL;
	m_connection = NULL;
	m_statement = NULL;
	m_lastError = 0;
	m_lastErrorText[0] = '\0';
	m_lastErrorWasConnection = false;
	m_lastErrorWasDriver = false;
	m_lastErrorWasOdbc = false;

	SetConnected(false);
}

OdbcConnection::~OdbcConnection()
{
	ClearResult();
	ClearOdbcData();

	SetConnected(false);
}

void OdbcConnection::ClearResult()
{
	if ( m_statement != NULL )
	{
		SQLFreeHandle(SQL_HANDLE_STMT, m_statement);
		m_statement = NULL;
	}
}

void OdbcConnection::InitOdbcData()
{
	if ( m_odbc == NULL )
	{
		SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_odbc);
		SQLSetEnvAttr(m_odbc, SQL_ATTR_ODBC_VERSION, (void*) SQL_OV_ODBC3, 0);
	}

	if ( m_connection == NULL )
	{
		SQLAllocHandle(SQL_HANDLE_DBC, m_odbc, &m_connection);
	}
}

void OdbcConnection::ClearOdbcData()
{
	if ( m_connection != NULL )
	{
		SQLDisconnect(m_connection);
		SQLFreeHandle(SQL_HANDLE_DBC, m_connection);
		m_connection = NULL;
	}

	if ( m_odbc != NULL )
	{
		SQLFreeHandle(SQL_HANDLE_ENV, m_odbc);
		m_odbc = NULL;
	}
}

bool OdbcConnection::IsConnected()
{
	return m_connected;
}

void OdbcConnection::SetConnected( bool connected )
{
	m_connected = connected;
}

bool OdbcConnection::ServerDisconnect()
{
	ClearResult();
	ClearOdbcData();

	SetConnected(false);

	return IsConnected();
}

bool OdbcConnection::ServerConnect(const char * user, const char * password, const char * database, const char * hostip, int hostport)
{
	InitOdbcData();

	// SQLConnect expects non-const pointers which we can't/shouldn't cast away, so
	// we'll need to copy the strings to our own variables...
	char * temp_servername = new char[strlen(database) + 1];
	char * temp_username = new char[strlen(user) + 1];
	char * temp_authentication = new char[strlen(password) + 1];

	std::strcpy(temp_servername, database);
	std::strcpy(temp_username, user);
	std::strcpy(temp_authentication, password);

	SQLRETURN result = SQLConnect(m_connection, reinterpret_cast<SQLCHAR*>(temp_servername), SQL_NTS, reinterpret_cast<SQLCHAR*>(temp_username), SQL_NTS, reinterpret_cast<SQLCHAR*>(temp_authentication), SQL_NTS);
	SetLastError(SQL_HANDLE_DBC, m_connection, result);

	if (SQL_SUCCEEDED(result))
	{
		SetConnected(true);
	}
	else
	{
		ServerDisconnect();
	}

	delete[] temp_servername;
	delete[] temp_username;
	delete[] temp_authentication;

	return IsConnected();
}

void OdbcConnection::SetLastError(SQLSMALLINT handleType, SQLHANDLE handle, SQLRETURN result)
{
	// clear last error
	m_lastError = 0;
	m_lastErrorText[0] = '\0';
	m_lastErrorWasConnection = false;
	m_lastErrorWasDriver = false;
	m_lastErrorWasOdbc = false;

	// check if failure occurred
	if (SQL_SUCCEEDED(result) == false)
	{
		SQLSMALLINT i = 0;
		SQLINTEGER native;
		SQLSTATE state;
		SQLCHAR text[256];
		SQLSMALLINT textLength;

		std::string errorText;
		errorText.reserve(MAX_DATA_LENGTH);

		while (true)
		{
			SQLRETURN result = SQLGetDiagRec(handleType, handle, ++i, state, &native, text, sizeof(text) / sizeof(text[0]), &textLength);
			if (SQL_SUCCEEDED(result) == false)
				break;

			if (i == 1)
				m_lastError = native;
			else
				errorText.append(1, '\n');

			// since we can receive multiple error states from a single operation (and in
			// no particular order), the best approach seems to be to flag which error
			// types have shown up
			if ( strncmp(reinterpret_cast<const char*>((state)), ODBCERR_CLASS_CONNECTION, SQLSTATE_CLASS_LENGTH) == 0 )
				m_lastErrorWasConnection = true;
			if ( strncmp(reinterpret_cast<const char*>((state)), ODBCERR_CLASS_DRIVER, SQLSTATE_CLASS_LENGTH) == 0 )
				m_lastErrorWasDriver = true;
			if ( strncmp(reinterpret_cast<const char*>((state)), ODBCERR_CLASS_ODBC, SQLSTATE_CLASS_LENGTH) == 0 )
				m_lastErrorWasOdbc = true;

			// build error messages (prepend with odbc error state)
			errorText.append("[");
			errorText.append(reinterpret_cast<const char*>(state));
			errorText.append(1, ']');
			errorText.append(reinterpret_cast<const char*>(text));
		}

		// copy error
		std::string::size_type length = errorText.copy(m_lastErrorText, MAX_DATA_LENGTH - 1);
		m_lastErrorText[length] = '\0';
	}
}

int OdbcConnection::GetLastError()
{
	return m_lastError;
}

const char * OdbcConnection::GetLastStringError()
{
	return m_lastErrorText;
}

int OdbcConnection::Execute(const char * statement)
{
	if ( m_odbc != NULL && m_connection != NULL && statement != NULL && *statement != '\0' )
	{
		ClearResult();

		// SQLExecDirect expects a non-const string, so we'll need to copy it
		char * temp_statement = new char[strlen(statement) + 1];
		std::strcpy(temp_statement, statement);

		// execute the statement
		SQLAllocHandle(SQL_HANDLE_STMT, m_connection, &m_statement);
		SQLRETURN result = SQLExecDirect(m_statement, reinterpret_cast<SQLCHAR*>(temp_statement), SQL_NTS);
		SetLastError(SQL_HANDLE_STMT, m_statement, result);

		delete[] temp_statement;

		// check success
		if (SQL_SUCCEEDED(result) == false)
		{
			if (m_lastErrorWasConnection)
				return DBPLUGIN_SERVER_LOST_ERROR;

			if (m_lastErrorWasDriver || m_lastErrorWasOdbc)
				return DBPLUGIN_INTERNAL_ERROR;

			return DBPLUGIN_API_SPECIFIC_ERROR;
		}

		return 0;
	}
	
	return DBPLUGIN_INTERNAL_ERROR;
}

int OdbcConnection::Query(const char * statement)
{
	if ( m_odbc != NULL && m_connection != NULL && statement != NULL && *statement != '\0' )
	{
		ClearResult();

		// SQLExecDirect expects a non-const string, so we'll need to copy it
		char * temp_statement = new char[strlen(statement) + 1];
		std::strcpy(temp_statement, statement);

		// execute statement
		SQLAllocHandle(SQL_HANDLE_STMT, m_connection, &m_statement);
		SQLRETURN result = SQLExecDirect(m_statement, reinterpret_cast<SQLCHAR*>(temp_statement), SQL_NTS);
		SetLastError(SQL_HANDLE_STMT, m_statement, result);

		delete[] temp_statement;

		// check success
		if (SQL_SUCCEEDED(result) == false)
		{
			if (m_lastErrorWasConnection)
				return DBPLUGIN_SERVER_LOST_ERROR;

			if (m_lastErrorWasDriver || m_lastErrorWasOdbc)
				return DBPLUGIN_INTERNAL_ERROR;

			return DBPLUGIN_API_SPECIFIC_ERROR;
		}

		return 0;
	}

	return DBPLUGIN_INTERNAL_ERROR;
}

int OdbcConnection::LastQueryNumFields()
{
	if ( m_odbc != NULL && m_connection != NULL && m_statement != NULL )
	{
		SQLSMALLINT fields = 0;
		SQLNumResultCols(m_statement, &fields);
		return fields;
	}

	return DBPLUGIN_INTERNAL_ERROR;
}

int OdbcConnection::LastQueryFetchFields(fieldarray_t * fields)
{
	if ( m_odbc != NULL && m_connection != NULL && m_statement != NULL )
	{
		SQLSMALLINT numField = 0;
		SQLNumResultCols(m_statement, &numField);

		for ( int i = 0; i < numField; i++ )
		{
			SQLDescribeCol(m_statement, i + 1, reinterpret_cast<SQLCHAR*>(fields[i].name), (MAX_FIELD_NAME - 1), NULL, NULL, NULL, NULL, NULL);
		}

		return numField;
	}

	return DBPLUGIN_INTERNAL_ERROR;
}

int OdbcConnection::LastQueryFetchRow(resultarray_t * results)
{
	if ( m_odbc != NULL && m_connection != NULL && m_statement != NULL )
	{
		SQLRETURN result = SQLFetch(m_statement);
		SetLastError(SQL_HANDLE_STMT, m_statement, result);
		if (SQL_SUCCEEDED(result) == false)
		{
			if (m_lastErrorWasConnection)
				return DBPLUGIN_SERVER_LOST_ERROR;

			if (m_lastErrorWasDriver || m_lastErrorWasOdbc)
				return DBPLUGIN_INTERNAL_ERROR;

			return DBPLUGIN_UNKNOWN_ERROR;;
		}

		SQLSMALLINT num_fields = 0;
		SQLNumResultCols(m_statement, &num_fields);

		for ( int i = 0; i < num_fields; i++ )
		{
			SQLINTEGER indicator;

			result = SQLGetData(m_statement, i + 1, SQL_C_CHAR, results[i].data, (MAX_DATA_LENGTH - 1), &indicator);
			SetLastError(SQL_HANDLE_STMT, m_statement, result);
			if (SQL_SUCCEEDED(result) == false || indicator == SQL_NULL_DATA)
				std::strcpy(results[i].data, "");
		}

		return num_fields;
	}

	return DBPLUGIN_INTERNAL_ERROR;
}

int OdbcConnection::EscapeString(const char * inString, int inLength, char * outString)
{
	// note: ODBC may not actually have a built-in way to escape a string, it seems the recommended
	//       approach is to bind parameters to a query instead.

	// for now, use the ms-sql style of double-quoting apostrophes (this might not
	// be correctly translated for different providers (e.g. mysql))
	std::string s(inString, inLength);
	for (std::string::size_type pos = 0; (pos = s.find('\'', pos)) != s.npos; pos += 2)
		s.replace(pos, 1, 2, '\'');

	s.copy(outString, s.length());
	outString[s.length()] = '\0';
	return s.length();
}
