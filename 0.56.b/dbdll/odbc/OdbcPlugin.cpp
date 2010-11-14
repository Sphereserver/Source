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

// DLLs specific things
#ifdef _WIN32
BOOL APIENTRY DllMain( HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved )
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
			OdbcConnection::GetCurrentInstance();
		} break;

		case DLL_PROCESS_DETACH:
		{
			OdbcConnection::DestroyCurrentInstance();
		} break;
	}

	return TRUE;
}
#else
void __attribute__((constructor)) dll_init()
{
	OdbcConnection::GetCurrentInstance();
}

void __attribute__((destructor)) dll_fini()
{
	OdbcConnection::DestroyCurrentInstance();
}
#endif

// Exported things
DBDLL_API bool __CALLTYPE db_isrightversion(void)
{
	// not applicable to odbc
	return true;
}

DBDLL_API int __CALLTYPE db_ping(void)
{
	// not applicable to odbc
	return 0;
}

DBDLL_API bool __CALLTYPE db_close(void)
{
	return OdbcConnection::GetCurrentInstance()->ServerDisconnect();
}

DBDLL_API bool __CALLTYPE db_isconnected(void)
{
	return OdbcConnection::GetCurrentInstance()->IsConnected();
}

DBDLL_API int __CALLTYPE db_getlasterror(void)
{
	return OdbcConnection::GetCurrentInstance()->GetLastError();
}

DBDLL_API const char * __CALLTYPE db_getlaststringerror(void)
{
	return OdbcConnection::GetCurrentInstance()->GetLastStringError();
}

DBDLL_API bool __CALLTYPE db_connect(const char ** args, int argc)
{
	if (argc < 5)
		return false;

	return OdbcConnection::GetCurrentInstance()->ServerConnect(args[0], args[1], args[2], args[3], atoi(args[4]));
}

DBDLL_API int __CALLTYPE db_execute(const char * query)
{
	return OdbcConnection::GetCurrentInstance()->Execute(query);
}

DBDLL_API int __CALLTYPE db_query(const char * query)
{
	return OdbcConnection::GetCurrentInstance()->Query(query);
}

DBDLL_API int __CALLTYPE db_fetchfields(fieldarray_t * fields)
{
	return OdbcConnection::GetCurrentInstance()->LastQueryFetchFields(fields);
}

DBDLL_API int __CALLTYPE db_numfields(void)
{
	return OdbcConnection::GetCurrentInstance()->LastQueryNumFields();
}

DBDLL_API int __CALLTYPE db_fetchrow(resultarray_t * results)
{
	return OdbcConnection::GetCurrentInstance()->LastQueryFetchRow(results);
}

DBDLL_API int __CALLTYPE db_escapestring(const char * inString, int inLength, char * outString)
{
	return OdbcConnection::GetCurrentInstance()->EscapeString(inString, inLength, outString);
}
