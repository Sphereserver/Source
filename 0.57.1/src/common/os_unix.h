#ifndef OS_UNIX_H
#define OS_UNIX_H
#pragma once

#include <pthread.h>
#include <iconv.h>
#include <unistd.h>
#include <signal.h>

#ifndef _MAX_PATH			// stdlib.h ?
	#define _MAX_PATH   260 	// max. length of full pathname
#endif

#ifndef UNREFERENCED_PARAMETER	// comes from winnt.h
	#define UNREFERENCED_PARAMETER(P)          (P)
#endif

#ifndef BYTE	// might be a typedef ?
	#define BYTE 		unsigned char	// 8 bits
	#define WORD 		unsigned short	// 16 bits
	#define DWORD		unsigned long	// 32 bits
	#define UINT		unsigned int
	#define LONGLONG	unsigned long
	#define LONG		unsigned long
#endif	// BYTE

#ifndef min
	#define min(x,y)	((x) <? (y))
	#define max(x,y)	((x) >? (y))
#endif //min

#define _cdecl
#define true 1
#define false 0
#define TCHAR char
#define LPCSTR const char *
#define LPCTSTR const char *
#define LPTSTR const char *
#define LPSTR char *
#define MulDiv	IMULDIV

// unix flushing works perfectly, so we do not need disabling bufer
#define	FILE_SETNOCACHE(_x_)
#define FILE_FLUSH(_x_)			fflush(_x_)

//	thread-specific definitions
#include "CTime.h"
#define THREAD_ENTRY_RET void *
#define CRITICAL_SECTION pthread_mutex_t
#define Sleep sleep

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