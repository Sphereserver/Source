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

#ifndef _SPHEREDATABASEPLUGIN_H
#define _SPHEREDATABASEPLUGIN_H

#ifdef _WIN32
	#define DBDLL_API
	#define __CALLTYPE __stdcall
#else
	#define DBDLL_API extern "C"
	#define __CALLTYPE
#endif

// Defines
#define DBPLUGIN_RESULT_NOQUERYRES    1
#define DBPLUGIN_INTERNAL_ERROR      -1
#define DBPLUGIN_API_SPECIFIC_ERROR  -(0xBEEF)
#define DBPLUGIN_UNKNOWN_ERROR       -(0xDEDE)
#define DBPLUGIN_SERVER_LOST_ERROR   -(0xDEAD)

#define MAX_FIELD_NAME               512
#define MAX_DATA_LENGTH              4096

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

// Exported things

/**
 * db_isrightversion
 * Used to check that the installed client library is usable.
 *
 * RETURNS:
 * true if the library is usable; otherwise, false.
 */
DBDLL_API bool __CALLTYPE db_isrightversion(void);

/**
 * db_ping
 * When connected, Sphere periodically calls this method to check that the connection is
 * still alive.
 *
 * RETURNS:
 * 0 to indicate success; otherwise, non-zero indicates that the connection has been lost.
 */
DBDLL_API int __CALLTYPE db_ping(void);

/**
 * db_close
 * Used to close the current database connection.
 *
 * RETURNS:
 * true if the connection has been closed; otherwise, false.
 */
DBDLL_API bool __CALLTYPE db_close(void);

/**
 * db_isconnected
 * Used to check if there is a current connection to the database.
 *
 * RETURNS:
 * true if there is a connection; otherwise, false.
 */
DBDLL_API bool __CALLTYPE db_isconnected(void);

/**
 * db_getlasterror
 * If an operation has failed, this method will be called to retrieve the native error
 * code.
 *
 * RETURNS:
 * The error code from a previous operation.
 */
DBDLL_API int __CALLTYPE db_getlasterror(void);

/**
 * db_getlaststringerror
 * If an operation has failed, this method will be called to retrieve a descriptive error
 * message that can be displayed to the user.
 *
 * RETURNS:
 * A null-terminated string containing the error message from a previous operation.
 */
DBDLL_API const char * __CALLTYPE db_getlaststringerror(void);

/**
 * db_connect
 * Used to connect to a database server.
 *
 * ARGUMENTS:
 * args: A list of null-terminated strings to use as the connection parameters, normally
 *       the parameters will be:
 *        args[0]: username
 *        args[1]: password
 *        args[2]: database
 *        args[3]: host
 *        args[4]: port
 * argc: The number of connection paramteers.
 *
 * RETURNS:
 * true if the connection was successful; otherwise, false.
 */
DBDLL_API bool __CALLTYPE db_connect(const char ** args, int argc);

/**
 * db_execute
 * Used to execute a database query where results are not wanted/needed.
 *
 * ARGUMENTS:
 * query: A null-terminated string containing the SQL query.
 *
 * RETURNS:
 * 0 to indicate success; otherwise, a non-zero value indicates failure.
 *
 * NOTES:
 * Use the DBPLUGIN_* defines to relay the type of error that occurred. Sphere will
 * invoke db_getlasterror and db_getlasterrortext to retrieve error details.
 */
DBDLL_API int __CALLTYPE db_execute(const char * query);

/**
 * db_query
 * Used to execute a database query where the results should be retrieved.
 *
 * ARGUMENTS:
 * query: A null-terminated string containing the SQL query.
 *
 * RETURNS:
 * 0 to indicate success; otherwise, a non-zero value indicates failure.
 *
 * NOTES:
 * Use the DBPLUGIN_* defines to relay the type of error that occurred. Sphere will
 * invoke db_getlasterror and db_getlasterrortext to retrieve error details.
 */
DBDLL_API int __CALLTYPE db_query(const char * query);

/**
 * db_fetchfields
 * Used to retrieve column names for query results.
 *
 * ARGUMENTS:
 * fields: A pointer to the list of field structures that need to be populated.
 *
 * RETURNS:
 * The number of fields populated. A negative value indicates an error occurred.
 */
DBDLL_API int __CALLTYPE db_fetchfields(fieldarray_t * fields);

/**
 * db_numfields
 * Used to retrieve the number of columns returned in the last query.
 *
 * RETURNS:
 * The number of columns returned in the last query. A negative value indicates an
 * error occurred.
 */
DBDLL_API int __CALLTYPE db_numfields(void);

/**
 * db_fetchrow
 * Used to retrieve the next row of data from the last query.
 *
 * ARGUMENTS:
 * results: A pointer to the list of result structures that need to be populated.
 *
 * RETURNS:
 * The number of results populated (expected to equal the number of columns), or zero
 * if there is no more data left to retrieve. A negative value indicates that an error
 * occurred.
 */
DBDLL_API int __CALLTYPE db_fetchrow(resultarray_t * results);

/**
 * db_escapestring
 * Used to escape special characters from a given character sequence.
 *
 * ARGUMENTS:
 * inString: A sequence of characters to escape.
 * inLength: The number of characters contained in inString.
 * outString: An output buffer to write the escaped string sequence to.
 *
 * RETURNS:
 * The number of characters written to outString. A negative value indicates that an
 * error occurred.
 */
DBDLL_API int __CALLTYPE db_escapestring(const char * inString, int inLength, char * outString);

#endif
