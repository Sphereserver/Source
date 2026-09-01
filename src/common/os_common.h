#ifndef _INC_OS_COMMON_H
#define _INC_OS_COMMON_H
#pragma once

#include <cinttypes>
#include <queue>
#ifdef _WIN32
	#include "os_windows.h"
#else
	#include "os_unix.h"
#endif

#define SPHERE_FILE				"sphere"
#define SPHERE_FILE_EXT_SCP		".scp"
#define SPHERE_FILE_EXT_INI		".ini"

#define SCRIPT_MAX_LINE_LEN		4096

#define minimum(a, b)			(((a) < (b)) ? (a) : (b))
#define maximum(a, b)			(((a) > (b)) ? (a) : (b))

#define IsNegative(a)			(((a) < 0) ? 1 : 0)
#define IMULDIV(a, b, c)		(((((LONGLONG)(a) * (LONGLONG)(b)) + ((c) / 2)) / (c)) - IsNegative((LONGLONG)(a) * (LONGLONG)(b)))

#ifndef MAKEDWORD
	#define MAKEDWORD(a, b)		((DWORD)(((WORD)(((DWORD_PTR)(a)) & 0xFFFF)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xFFFF))) << 16))
#endif

#define FMTDWORD		PRIu32
#define FMTDWORDH		PRIx32
#define FMTSIZE_T		"zu"

#ifdef _WIN32
	#define strcmpi		_strcmpi
	#define strnicmp	_strnicmp
#else
	#define strcmpi		strcasecmp
	#define strnicmp	strncasecmp
#endif

#ifndef COUNTOF
	#ifdef _WIN32
		#define COUNTOF(_Array)	_countof(_Array)
	#else
		// Ported from Windows _countof() macro defined in vcruntime.h
		#ifdef __cplusplus
			extern "C++"
			{
				template <typename _CountofType, size_t _SizeOfArray>
				char (*__countof_helper(_CountofType(&_Array)[_SizeOfArray]))[_SizeOfArray];

				#define COUNTOF(_Array)	(sizeof(*__countof_helper(_Array)) + 0)
			}
		#else
			#define COUNTOF(_Array)	(sizeof(_Array) / sizeof(_Array[0]))
		#endif
	#endif
#endif

#ifndef STDFUNC_FILENO
	#ifdef _WIN32
		#define STDFUNC_FILENO(a)	_get_osfhandle(_fileno(a))
	#else
		#define STDFUNC_FILENO		fileno
	#endif
#endif

#ifndef STDFUNC_GETPID
	#ifdef _WIN32
		#define STDFUNC_GETPID		_getpid
	#else
		#define STDFUNC_GETPID		getpid
	#endif
#endif

#ifndef STDFUNC_UNLINK
	#ifdef _WIN32
		#define STDFUNC_UNLINK		_unlink
	#else
		#define STDFUNC_UNLINK		unlink
	#endif
#endif

#ifdef _WIN32
	typedef void	THREAD_ENTRY_RET;
#else
	typedef void	*THREAD_ENTRY_RET;
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
