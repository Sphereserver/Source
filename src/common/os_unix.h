#ifndef _INC_OS_UNIX_H
#define _INC_OS_UNIX_H
#pragma once

#ifndef _WIN32
#include <cctype>
#include <climits>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>

// Port some Windows stuff to Linux
typedef uint8_t				BYTE;		// unsigned char
#define BYTE_MAX			UINT8_MAX

typedef uint16_t			WORD;		// unsigned short
#define WORD_MAX			UINT16_MAX

typedef uint32_t			DWORD;		// unsigned int
typedef uintptr_t			DWORD_PTR, ULONG_PTR;
#define DWORD_MAX			UINT32_MAX
#define DWORD_PTR_MAX		UINTPTR_MAX

typedef int32_t				INT32;
typedef uint32_t			UINT32, UINT;
typedef int32_t				LONG;
typedef long long			LONGLONG, INT64;
typedef unsigned long long	ULONGLONG, UINT64;

typedef uint16_t			WCHAR;
typedef char				TCHAR;
typedef TCHAR				*LPSTR, *LPTSTR;
typedef const TCHAR			*LPCSTR, *LPCTSTR;

typedef int					BOOL;

#define MAKEWORD(a, b)      ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xFF)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xFF))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xFFFF)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xFFFF))) << 16))
#define LOWORD(l)           ((WORD)(((DWORD_PTR)(l)) & 0xFFFF))
#define HIWORD(l)           ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xFFFF))
#define LOBYTE(w)           ((BYTE)(((DWORD_PTR)(w)) & 0xFF))
#define HIBYTE(w)           ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xFF))

#define FAR

#define _cdecl
#define __cdecl

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

inline TCHAR* _strupr(TCHAR *pszStr)
{
	if ( pszStr )
	{
		for ( ; *pszStr != '\0'; ++pszStr )
			*pszStr = static_cast<TCHAR>(toupper(static_cast<unsigned char>(*pszStr)));
	}
	return pszStr;
}

inline TCHAR* _strlwr(TCHAR *pszStr)
{
	if ( pszStr )
	{
		for ( ; *pszStr != '\0'; ++pszStr )
			*pszStr = static_cast<TCHAR>(tolower(static_cast<unsigned char>(*pszStr)));
	}
	return pszStr;
}

#endif	// _WIN32
#endif	// _INC_OS_UNIX_H
