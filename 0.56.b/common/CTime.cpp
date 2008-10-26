//
// Ctime.cpp
//
// Replace the MFC CTime function. Must be usable with file system.
//

#include "graycom.h"
#include "CTime.h"

#ifndef _WIN32
#include <sys/time.h>

LONGLONG GetTickCount()
{
	struct timeval tv;
	gettimeofday( &tv, NULL );
	return (LONGLONG) (((LONGLONG) tv.tv_sec * 1000) + ((LONGLONG) tv.tv_usec/1000));
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
void invalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
{
	// bad format has been specified
	DEBUG_ERR(("Invalid time format specified.\n"));
}
#endif

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

#ifdef _WIN32
	// set invalid parameter handler, or else the program will terminate when a bad format is encountered
	_invalid_parameter_handler oldHandler, newHandler;
	newHandler = (_invalid_parameter_handler)invalidParameterHandler;
	oldHandler = _set_invalid_parameter_handler(newHandler);
#endif

	if (!strftime( pszTemp, maxTimeBufferSize, pszFormat, ptmTemp))
	{
		pszTemp[0] = '\0';
	}

#ifdef _WIN32
	// restore previous parameter handler
	_set_invalid_parameter_handler(oldHandler);
#endif
	return( pszTemp );
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

#ifdef _WIN32
	// set invalid parameter handler, or else the program will terminate when a bad format is encountered
	_invalid_parameter_handler oldHandler, newHandler;
	newHandler = (_invalid_parameter_handler)invalidParameterHandler;
	oldHandler = _set_invalid_parameter_handler(newHandler);
#endif

	if (!strftime( pszTemp, maxTimeBufferSize, pszFormat, ptmTemp))
	{
		pszTemp[0] = '\0';
	}

#ifdef _WIN32
	// restore previous parameter handler
	_set_invalid_parameter_handler(oldHandler);
#endif
	return pszTemp;
}

//**************************************************************

bool CGTime::Read(TCHAR *pszVal)
{
	ADDTOCALLSTACK("CGTime::Read");
	// Read the full date format.

	TCHAR *ppCmds[10];
	int iQty = Str_ParseCmds( pszVal, ppCmds, COUNTOF(ppCmds), "/,: \t");
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
