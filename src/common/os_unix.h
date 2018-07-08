#ifndef _INC_OS_UNIX_H
#define _INC_OS_UNIX_H
#pragma once

#ifndef _WIN32
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <limits>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <cctype>

// Port some Windows stuff to Linux
typedef unsigned char		BYTE;				// 8 bits
typedef unsigned short		WORD;				// 16 bits
typedef unsigned long		DWORD;				// 32 bits

typedef int					INT32;				// 32 bits
typedef unsigned int		UINT32, UINT;		// 32 bits
typedef long				LONG;				// 32 bits
typedef long long			LONGLONG, INT64;	// 64 bits
typedef unsigned long long	ULONGLONG, UINT64;	// 64 bits

typedef wchar_t				WCHAR;
typedef char				TCHAR;
typedef char				*LPSTR, *LPTSTR;
typedef const char			*LPCSTR, *LPCTSTR;

typedef int					BOOL;

#define MAKEWORD(l, h)		((WORD)(((BYTE)(l))|(((WORD)((BYTE)(h))) << 8)))
#define MAKELONG(l, h)		((LONG)(((WORD)(l))|(((DWORD)((WORD)(h))) << 16)))
#define LOWORD(l)			((WORD)((DWORD)(l) & 0xFFFF))
#define HIWORD(l)			((WORD)((DWORD)(l) >> 16))
#define LOBYTE(w)			((BYTE)((DWORD)(w) & 0xFF))
#define HIBYTE(w)			((BYTE)((DWORD)(w) >> 8))

#define FAR

#define _cdecl
#define __cdecl
#define _vsnprintf				vsnprintf
#define Sleep(mSec)				usleep(mSec * 1000)		// arg is microseconds = 1/1000000
#define SleepEx(mSec, unused)	usleep(mSec * 1000)		// arg is microseconds = 1/1000000

#ifndef _MAX_PATH
	#define _MAX_PATH	260
#endif

#ifndef ERROR_SUCCESS
	#define ERROR_SUCCESS	0
#endif

#define E_FAIL	0x80004005

#ifndef HKEY_LOCAL_MACHINE
	#define HKEY_LOCAL_MACHINE	((HKEY)0x80000002)
#endif

#ifndef UNREFERENCED_PARAMETER
	#define UNREFERENCED_PARAMETER(P)	(void)(P)
#endif

inline void _strupr(TCHAR *pszStr)
{
	for ( ; pszStr[0] != '\0'; pszStr++ )
		*pszStr = toupper(*pszStr);
}

inline void _strlwr(TCHAR *pszStr)
{
	for ( ; pszStr[0] != '\0'; pszStr++ )
		*pszStr = tolower(*pszStr);
}

#endif	// _WIN32
#endif	// _INC_OS_UNIX_H
