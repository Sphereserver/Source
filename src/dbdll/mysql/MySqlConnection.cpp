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
#include "MySqlConnection.h"

MySqlConnection * MySqlConnection::_mysqlConnection = NULL;

MySqlConnection * MySqlConnection::GetCurrentInstance()
{
	if ( MySqlConnection::_mysqlConnection == NULL )
	{
		MySqlConnection::_mysqlConnection = new MySqlConnection();
	}

	return MySqlConnection::_mysqlConnection;
}

void MySqlConnection::DestroyCurrentInstance()
{
	if ( MySqlConnection::_mysqlConnection != NULL )
	{
		delete MySqlConnection::_mysqlConnection;
		MySqlConnection::_mysqlConnection = NULL;
	}
}

MySqlConnection::MySqlConnection()
{
	m_myData = NULL;
	m_curResult = NULL;
	SetConnected(false);
}

MySqlConnection::~MySqlConnection()
{
	ClearResult();
	ClearMySqlData();

	SetConnected(false);
}

void MySqlConnection::ClearResult()
{
	if ( m_curResult != NULL )
	{
		mysql_free_result(m_curResult);
		m_curResult = NULL;
	}
}

void MySqlConnection::InitMySqlData()
{
	if ( m_myData == NULL )
	{
		m_myData = mysql_init(NULL);
	}
}

void MySqlConnection::ClearMySqlData()
{
	if ( m_myData != NULL )
	{
		mysql_close(m_myData);
		m_myData = NULL;
	}
}


bool MySqlConnection::IsConnected()
{
	return m_connected;
}

void MySqlConnection::SetConnected( bool connected )
{
	m_connected = connected;
}

unsigned long MySqlConnection::GetClientVersion()
{
	return mysql_get_client_version();
}

int MySqlConnection::ServerPing()
{
	return mysql_ping(m_myData);
}

bool MySqlConnection::ServerDisconnect()
{
	ClearResult();
	ClearMySqlData();

	SetConnected(false);

	return IsConnected();
}

bool MySqlConnection::ServerConnect(const char * user, const char * password, const char * database, const char * hostip, int hostport)
{
	InitMySqlData();

	MYSQL * result = mysql_real_connect(m_myData, hostip, user, password, database, hostport, NULL, CLIENT_MULTI_STATEMENTS );

	if ( result == NULL )
	{
		ServerDisconnect();
	}
	else
	{
		SetConnected(true);
	}

	return IsConnected();
}

int MySqlConnection::GetLastError()
{
	if ( m_myData != NULL )
	{
		return mysql_errno(m_myData);
	}
	
	return DBPLUGIN_INTERNAL_ERROR;
}

const char * MySqlConnection::GetLastStringError()
{
	if ( m_myData != NULL )
	{
		return mysql_error(m_myData);
	}

	return NULL;
}

int MySqlConnection::Execute(const char * statement)
{
	if ( m_myData != NULL && statement != NULL && *statement != '\0' )
	{
		int queryResult = mysql_query(m_myData, statement);

		switch ( queryResult )
		{
			case CR_SERVER_GONE_ERROR:
			case CR_SERVER_LOST:
				return DBPLUGIN_SERVER_LOST_ERROR;

			case CR_UNKNOWN_ERROR:
				return DBPLUGIN_UNKNOWN_ERROR;

			case CR_COMMANDS_OUT_OF_SYNC:
				return DBPLUGIN_API_SPECIFIC_ERROR;
				
			case 0:
			{
				// even though we don't want (or expect) any result data, it must be
				// retrieved anyway otherwise we will lose server connection
				MYSQL_RES* res = mysql_store_result(m_myData);
				if (res != NULL)
					mysql_free_result(res);
			}
		}

		return 0;
	}
	
	return DBPLUGIN_INTERNAL_ERROR;
}

int MySqlConnection::Query(const char * statement)
{
	if ( m_myData != NULL && statement != NULL && *statement != '\0' )
	{
		int queryResult = mysql_query(m_myData, statement);

		switch ( queryResult )
		{
			case CR_SERVER_GONE_ERROR:
			case CR_SERVER_LOST:
				return DBPLUGIN_SERVER_LOST_ERROR;

			case CR_UNKNOWN_ERROR:
				return DBPLUGIN_UNKNOWN_ERROR;

			case CR_COMMANDS_OUT_OF_SYNC:
				return DBPLUGIN_API_SPECIFIC_ERROR;
		}

		ClearResult();
		m_curResult = mysql_store_result(m_myData);
		
		if ( m_curResult == NULL )
		{
			int resultError = mysql_errno(m_myData);

			switch ( resultError )
			{
				case 0:
					return DBPLUGIN_RESULT_NOQUERYRES;

				case CR_SERVER_GONE_ERROR:
				case CR_SERVER_LOST:
					return DBPLUGIN_SERVER_LOST_ERROR;

				case CR_OUT_OF_MEMORY:
					return DBPLUGIN_API_SPECIFIC_ERROR;
			}

			return DBPLUGIN_UNKNOWN_ERROR;
		}

		return 0;
	}

	return DBPLUGIN_INTERNAL_ERROR;
}

int MySqlConnection::LastQueryNumFields()
{
	if ( m_myData != NULL && m_curResult != NULL )
	{
		return mysql_num_fields(m_curResult);
	}

	return DBPLUGIN_INTERNAL_ERROR;
}

int MySqlConnection::LastQueryFetchFields(fieldarray_t * fields)
{
	if ( m_myData != NULL && m_curResult != NULL )
	{
		MYSQL_FIELD *msFields = mysql_fetch_fields(m_curResult);
		int numField = mysql_num_fields(m_curResult);

		for ( int i = 0; i < numField; i++ )
		{
			strncpy(fields[i].name, msFields[i].name, (MAX_FIELD_NAME - 1));
		}

		return numField;
	}

	return DBPLUGIN_INTERNAL_ERROR;
}

int MySqlConnection::LastQueryFetchRow(resultarray_t * results)
{
	if ( m_myData != NULL && m_curResult != NULL )
	{
		MYSQL_ROW row = mysql_fetch_row(m_curResult);
		if ( row == NULL )
		{
			int resultError = mysql_errno(m_myData);

			switch ( resultError )
			{
				case 0:
					return 0;

				case CR_SERVER_LOST:
					return DBPLUGIN_SERVER_LOST_ERROR;
			}

			return DBPLUGIN_UNKNOWN_ERROR;
		}

		int num_fields = mysql_num_fields(m_curResult);

		for ( int i = 0; i < num_fields; i++ )
		{
			strncpy(results[i].data, row[i], (MAX_DATA_LENGTH - 1));
		}

		return num_fields;
	}

	return DBPLUGIN_INTERNAL_ERROR;
}

int MySqlConnection::EscapeString(const char * inString, int inLength, char * outString)
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
