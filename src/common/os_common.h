#ifndef _INC_OS_COMMON_H
#define _INC_OS_COMMON_H
#pragma once

#include <queue>
#ifdef _WIN32
	#include "os_windows.h"
#else
	#include "os_unix.h"
#endif

#define SPHERE_FILE			"sphere"	// file name prefix
#define SPHERE_SCRIPT		".scp"		// file name extension

#define SCRIPT_MAX_LINE_LEN	4096

#ifndef MAKEDWORD
	#define MAKEDWORD(l, h)		((DWORD)(((WORD)(l))|(((DWORD)((WORD)(h))) << 16)))
#endif

#ifndef COUNTOF
	#define COUNTOF(a)	(sizeof(a) / sizeof(a[0]))
#endif

#define IsDigit(c)		isdigit((unsigned char)(c))
#define IsSpace(c)		isspace((unsigned char)(c))
#define IsAlpha(c)		isalpha((unsigned char)(c))
#define IsNegative(c)	(((c) < 0) ? 1 : 0)

#define minimum(a, b)			(((a) < (b)) ? (a) : (b))
#define maximum(a, b)			(((a) > (b)) ? (a) : (b))

#define IMULDIV(a, b, c)		(((((LONGLONG)(a) * (LONGLONG)(b)) + ((c) / 2)) / (c)) - IsNegative((LONGLONG)(a) * (LONGLONG)(b)) )

#ifdef _WIN32
	typedef void		THREAD_ENTRY_RET;
	#define FMTDWORD	"lu"	// Windows uses '%lu' to format dec DWORD (unsigned long)
	#define FMTDWORDH	"lx"	// Windows uses '%lx' to format hex DWORD (unsigned long)
	#define FMTSIZE_T	"Iu"	// Windows uses '%Iu' to format 'size_t'

	#define strcmpi		_strcmpi
	#define strnicmp	_strnicmp

	#ifndef STDFUNC_FILENO
		#define STDFUNC_FILENO(a)	_get_osfhandle(_fileno(a))
	#endif

	#ifndef STDFUNC_GETPID
		#define STDFUNC_GETPID		_getpid
	#endif

	#ifndef STDFUNC_UNLINK
		#define STDFUNC_UNLINK		_unlink
	#endif
#else
	typedef void		*THREAD_ENTRY_RET;
	#define FMTDWORD	"u"		// Linux uses '%u' to format dec DWORD (unsigned long)
	#define FMTDWORDH	"x"		// Linux uses '%x' to format hex DWORD (unsigned int)
	#define FMTSIZE_T	"zu"	// Linux uses '%zu' to format 'size_t'

	#define strcmpi		strcasecmp
	#define strnicmp	strncasecmp

	#ifndef STDFUNC_FILENO
		#define STDFUNC_FILENO		fileno
	#endif

	#ifndef STDFUNC_GETPID
		#define STDFUNC_GETPID		getpid
	#endif

	#ifndef STDFUNC_UNLINK
		#define STDFUNC_UNLINK		unlink
	#endif
#endif

typedef THREAD_ENTRY_RET(_cdecl *PTHREAD_ENTRY_PROC)(void *);
typedef unsigned int	ERROR_CODE;

// Time measurement macros
#include "CTime.h"
extern ULONGLONG llTimeProfileFrequency;

#ifdef _WIN32
	#define	TIME_PROFILE_START		if (!QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&llTicksStart)))	llTicksStart = GetTickCount64()
	#define TIME_PROFILE_END		if (!QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&llTicksEnd)))	llTicksEnd = GetTickCount64()

	#define TIME_PROFILE_GET_HI		((llTicksEnd - llTicksStart) / (llTimeProfileFrequency / 1000))
	#define	TIME_PROFILE_GET_LO		((((llTicksEnd - llTicksStart) * 10000) / (llTimeProfileFrequency / 1000)) % 10000)
#else
	#define	TIME_PROFILE_START		llTicksStart = GetTickCount64()
	#define TIME_PROFILE_END		llTicksEnd = GetTickCount64();

	#define TIME_PROFILE_GET_HI		(llTicksEnd - llTicksStart)
	#define	TIME_PROFILE_GET_LO		(((llTicksEnd - llTicksStart) * 10) % 10000)
#endif

// Use to indicate that a function uses printf-style arguments, allowing GCC to validate the format string and arguments:
//  a = 1-based index of format string
//  b = 1-based index of arguments
// Note: add 1 to index for non-static class methods because 'this' argument is inserted in position 1
#ifdef __GNUC__
	#define __printfargs(a, b)	__attribute__ ((format(printf, a, b)))
#else
	#define __printfargs(a, b)
#endif

///////////////////////////////////////////////////////////
// CValStr

struct CValStr
{
	// Associate a val with a string
	// Assume sorted values from min to max
public:
	LPCTSTR m_pszName;
	int m_iVal;

	LPCTSTR FindName(int iVal) const;
	void SetValues(int iVal, LPCTSTR pszName)
	{
		m_iVal = iVal;
		m_pszName = pszName;
	}
	void SetValue(int iVal)
	{
		m_iVal = iVal;
	}
};

#endif	// _INC_OS_COMMON_H
