//
// Ctime.cpp
//
// Replace the MFC CTime function. Must be usable with file system.
//

#include "graycom.h"
#include "CTime.h"

#ifndef _WIN32
#include <sys/time.h>

ULONGLONG GetTickCount64()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((ts.tv_sec * 10000) + (ts.tv_nsec / 100000)) / 10;
}
#endif


//**************************************************************
// -CGTime - absolute time

CGTime::CGTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec,
			   int nDST)
{
	struct tm atm;
	atm.tm_sec = nSec;
	atm.tm_min = nMin;
	atm.tm_hour = nHour;
	atm.tm_mday = nDay;
	atm.tm_mon = nMonth - 1;        // tm_mon is 0 based
	atm.tm_year = nYear - 1900;     // tm_year is 1900 based
	atm.tm_isdst = nDST;
	m_time = mktime(&atm);
}

CGTime::CGTime( struct tm atm )
{
	m_time = mktime(&atm);
}

CGTime CGTime::GetCurrentTime()	// static
{
	// return the current system time
	return CGTime(::time(NULL));
}

struct tm* CGTime::GetLocalTm(struct tm* ptm) const
{
	if (ptm != NULL)
	{
		struct tm* ptmTemp = localtime(&m_time);
		if (ptmTemp == NULL)
			return NULL;    // indicates the m_time was not initialized!

		*ptm = *ptmTemp;
		return ptm;
	}
	else
		return localtime(&m_time);
}

////////////////////////////////////////////////////////////////////////////
// String formatting

#ifndef maxTimeBufferSize
	#define maxTimeBufferSize 128
#endif

#ifdef _WIN32
void __cdecl invalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
{
	// bad format has been specified
	UNREFERENCED_PARAMETER(expression);
	UNREFERENCED_PARAMETER(function);
	UNREFERENCED_PARAMETER(file);
	UNREFERENCED_PARAMETER(line);
	UNREFERENCED_PARAMETER(pReserved);
	DEBUG_ERR(("Invalid time format specified.\n"));
}
#endif

void FormatDateTime(TCHAR * pszTemp, LPCTSTR pszFormat, const struct tm * ptmTemp)
{
	ASSERT(pszTemp != NULL);
	ASSERT(pszFormat != NULL);
	ASSERT(ptmTemp != NULL);

#ifdef _WIN32
	// on windows we need to set the invalid parameter handler, or else the program will terminate when a bad format is encountered
	_invalid_parameter_handler oldHandler, newHandler;
	newHandler = static_cast<_invalid_parameter_handler>(invalidParameterHandler);
#ifndef __MINGW32__
	oldHandler = _set_invalid_parameter_handler(newHandler);
#endif  // __MINGW32__
	try
	{
#endif

	if (strftime( pszTemp, maxTimeBufferSize, pszFormat, ptmTemp) == 0)
		pszTemp[0] = '\0';

#ifdef _WIN32
	}
	catch (...)
	{
		// since VS2010, it seems an exception gets thrown for invalid format strings too
		pszTemp[0] = '\0';
	}

	// restore previous parameter handler

#ifndef __MINGW32__
	_set_invalid_parameter_handler(oldHandler);
#endif  // __MINGW32__
#endif
}

LPCTSTR CGTime::Format(LPCTSTR pszFormat) const
{
	TCHAR * pszTemp = Str_GetTemp();

	if ( pszFormat == NULL )
	{
		pszFormat = "%Y/%m/%d %H:%M:%S";
	}

	struct tm* ptmTemp = localtime(&m_time);
	if (ptmTemp == NULL )
	{
		pszTemp[0] = '\0';
		return( pszTemp );
	}

	FormatDateTime(pszTemp, pszFormat, ptmTemp);
	return pszTemp;
}

LPCTSTR CGTime::FormatGmt(LPCTSTR pszFormat) const
{
	TCHAR * pszTemp = Str_GetTemp();
	if ( pszFormat == NULL )
	{
		pszFormat = "%a, %d %b %Y %H:%M:%S GMT";
	}

	struct tm* ptmTemp = gmtime(&m_time);
	if (ptmTemp == NULL )
	{
		pszTemp[0] = '\0';
		return( pszTemp );
	}

	FormatDateTime(pszTemp, pszFormat, ptmTemp);
	return pszTemp;
}

//**************************************************************

bool CGTime::Read(TCHAR *pszVal)
{
	ADDTOCALLSTACK("CGTime::Read");
	// Read the full date format.

	TCHAR *ppCmds[10];
	size_t iQty = Str_ParseCmds( pszVal, ppCmds, COUNTOF(ppCmds), "/,: \t");
	if ( iQty < 6 )
		return false;

	struct tm atm;
	atm.tm_wday = 0;    // days since Sunday - [0,6] 
	atm.tm_yday = 0;    // days since January 1 - [0,365] 
	atm.tm_isdst = 0;   // daylight savings time flag 

	// Saves: "1999/8/1 14:30:18"
	atm.tm_year = ATOI(ppCmds[0]) - 1900;
	atm.tm_mon = ATOI(ppCmds[1]) - 1;
	atm.tm_mday = ATOI(ppCmds[2]);
	atm.tm_hour = ATOI(ppCmds[3]);
	atm.tm_min = ATOI(ppCmds[4]);
	atm.tm_sec = ATOI(ppCmds[5]);
	m_time = mktime(&atm);

	return true;
}
