#include "graycom.h"
#include "CTime.h"

#ifndef _WIN32
#include <time.h>

ULONGLONG GetTickCount64()
{
	struct timespec ts;
#ifdef __FreeBSD__
	clock_gettime(CLOCK_MONOTONIC_FAST, &ts);
#else
	clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
	return (static_cast<ULONGLONG>(ts.tv_sec * 10000) + (ts.tv_nsec / 100000)) / 10;
}
#endif

CGTime::CGTime(int iYear, int iMonth, int iDay, int iHour, int iMin, int iSec, int iDST)
{
	struct tm atm;
	atm.tm_sec = iSec;
	atm.tm_min = iMin;
	atm.tm_hour = iHour;
	atm.tm_mday = iDay;
	atm.tm_mon = iMonth - 1;		// tm_mon is 0 based
	atm.tm_year = iYear - 1900;		// tm_year is 1900 based
	atm.tm_isdst = iDST;
	m_time = mktime(&atm);
}

CGTime CGTime::GetCurrentTime()	// static
{
	// Return current system time
	return CGTime(time(NULL));
}

struct tm *CGTime::GetLocalTm() const
{
	return localtime(&m_time);
}

#ifdef _WIN32
void __cdecl invalidParameterHandler(const wchar_t *expression, const wchar_t *function, const wchar_t *file, unsigned int line, uintptr_t pReserved)
{
	// Bad format has specified
	UNREFERENCED_PARAMETER(expression);
	UNREFERENCED_PARAMETER(function);
	UNREFERENCED_PARAMETER(file);
	UNREFERENCED_PARAMETER(line);
	UNREFERENCED_PARAMETER(pReserved);
	DEBUG_ERR(("Invalid time format specified\n"));
}
#endif

void FormatDateTime(TCHAR *pszTemp, LPCTSTR pszFormat, const struct tm *ptmTemp)
{
	ASSERT(pszTemp);
	ASSERT(pszFormat);
	ASSERT(ptmTemp);

#ifdef _WIN32
	// On Windows we need to set the invalid parameter handler, or the program will terminate when a bad format is encountered
#ifndef __MINGW32__
	_invalid_parameter_handler newHandler = static_cast<_invalid_parameter_handler>(invalidParameterHandler);
	_invalid_parameter_handler oldHandler = _set_invalid_parameter_handler(newHandler);
#endif
	try
	{
#endif
		if ( strftime(pszTemp, 128, pszFormat, ptmTemp) == 0 )
			pszTemp[0] = '\0';
#ifdef _WIN32
	}
	catch (...)
	{
		// Since VS2010, it seems an exception gets thrown for invalid format strings too
		pszTemp[0] = '\0';
	}

#ifndef __MINGW32__
	// Restore previous parameter handler
	_set_invalid_parameter_handler(oldHandler);
#endif
#endif
}

LPCTSTR CGTime::Format(LPCTSTR pszFormat) const
{
	ADDTOCALLSTACK("CGTime::Format");
	if ( !pszFormat )
		pszFormat = "%Y/%m/%d %H:%M:%S";

	TCHAR *pszTemp = Str_GetTemp();
	FormatDateTime(pszTemp, pszFormat, localtime(&m_time));
	return pszTemp;
}

LPCTSTR CGTime::FormatGmt(LPCTSTR pszFormat) const
{
	ADDTOCALLSTACK("CGTime::FormatGmt");
	if ( !pszFormat )
		pszFormat = "%a, %d %b %Y %H:%M:%S GMT";

	TCHAR *pszTemp = Str_GetTemp();
	FormatDateTime(pszTemp, pszFormat, gmtime(&m_time));
	return pszTemp;
}

bool CGTime::Read(TCHAR *pszVal)
{
	ADDTOCALLSTACK("CGTime::Read");
	// Read the full date format (args must be given in format YYYY/MM/DD HH:MM:SS)

	TCHAR *ppCmds[6];
	size_t iQty = Str_ParseCmds(pszVal, ppCmds, COUNTOF(ppCmds), "/,: \t");
	if ( iQty < 6 )
		return false;

	struct tm atm;
	atm.tm_wday = 0;	// days since Sunday - [0, 6]
	atm.tm_yday = 0;	// days since January 1 - [0, 365]
	atm.tm_isdst = 0;	// daylight savings time flag

	atm.tm_year = ATOI(ppCmds[0]) - 1900;
	atm.tm_mon = ATOI(ppCmds[1]) - 1;
	atm.tm_mday = ATOI(ppCmds[2]);
	atm.tm_hour = ATOI(ppCmds[3]);
	atm.tm_min = ATOI(ppCmds[4]);
	atm.tm_sec = ATOI(ppCmds[5]);
	m_time = mktime(&atm);
	return true;
}
