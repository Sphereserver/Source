#ifndef OS_UNIX_H
#define OS_UNIX_H
#pragma once

#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <exception>
#include <setjmp.h>
#include <dlfcn.h>
#include <errno.h>
#include <aio.h>
#include <cctype>

#ifndef _MAX_PATH			// stdlib.h ?
	#define _MAX_PATH   260 	// max. length of full pathname
#endif

#ifndef STDFUNC_FILENO
	#define STDFUNC_FILENO fileno
#endif

#ifndef STDFUNC_GETPID
	#define STDFUNC_GETPID getpid
#endif

#ifndef STDFUNC_UNLINK
	#define STDFUNC_UNLINK unlink
#endif

#ifndef ERROR_SUCCESS
	#define ERROR_SUCCESS	0
#endif

#ifndef UNREFERENCED_PARAMETER
	#define UNREFERENCED_PARAMETER(P)	(void)(P)
#endif

#ifndef HKEY_LOCAL_MACHINE
	#define HKEY_LOCAL_MACHINE	(( HKEY ) 0x80000002 )
#endif

#ifndef BYTE	// might be a typedef ?
	#ifdef LONGLONG
		#undef LONGLONG
	#endif
	#ifdef LONG
		#undef LONG
	#endif

	#define BYTE 		unsigned char	// 8 bits
	#define WORD 		unsigned short	// 16 bits
	#define DWORD		unsigned long	// 32 bits
	#define UINT		unsigned int
	#define LONGLONG	long long 	// this must be 64bit
	#define ULONGLONG	unsigned long long
	#define LONG		long		// this stays 32bit!
#endif	// BYTE

#define MAKEWORD(low,high) ((WORD)(((BYTE)(low))|(((WORD)((BYTE)(high)))<<8)))
#define MAKELONG(low,high) ((LONG)(((WORD)(low))|(((DWORD)((WORD)(high)))<<16)))
#define LOWORD(l)	((WORD)((DWORD)(l) & 0xffff))
#define HIWORD(l)	((WORD)((DWORD)(l) >> 16))
#define LOBYTE(w)	((BYTE)((DWORD)(w) &  0xff))
#define HIBYTE(w)	((BYTE)((DWORD)(w) >> 8))

#ifndef minimum					// limits.h ?
	#define minimum(x,y)	((x)<(y)?(x):(y))
	#define maximum(x,y)	((x)>(y)?(x):(y))
#endif	// minimum

#ifndef sign
	#define sign(n) (((n) < 0) ? -1 : (((n) > 0) ? 1 : 0))
//	#define abs(n) (((n) < 0) ? (-(n)) : (n))
#endif

#define _cdecl
#define true 1
#define false 0
#define TCHAR char
#define LPCSTR const char *
#define LPCTSTR const char *
#define LPSTR char *
#define LPTSTR char *
#define MulDiv	IMULDIV

// unix flushing works perfectly, so we do not need disabling bufer
#define	FILE_SETNOCACHE(_x_)
#define FILE_FLUSH(_x_)			fflush(_x_)

//	thread-specific definitions
#include "CTime.h"
#define THREAD_ENTRY_RET void *
#define CRITICAL_SECTION pthread_mutex_t
#define Sleep(mSec)	usleep(mSec*1000)	// arg is microseconds = 1/1000000
#define SleepEx(mSec, unused)		usleep(mSec*1000)	// arg is microseconds = 1/1000000

// printf format identifiers
#define FMTSIZE_T "zu" // linux uses %zu to format size_t

inline void _strupr( TCHAR * pszStr )
{
	// No portable UNIX/LINUX equiv to this.
	for ( ;pszStr[0] != '\0'; pszStr++ )
	{
		*pszStr = toupper( *pszStr );
	}
}

inline void _strlwr( TCHAR * pszStr )
{
	// No portable UNIX/LINUX equiv to this.
	for ( ;pszStr[0] != '\0'; pszStr++ )
	{
		*pszStr = tolower( *pszStr );
	}
}

#endif
