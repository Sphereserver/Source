#ifndef _INC_OS_WINDOWS_H
#define _INC_OS_WINDOWS_H
#pragma once

#define _WIN32_DCOM

#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0501
#endif

#undef FD_SETSIZE
#define FD_SETSIZE 1024 // for max of n users ! default = 64

#include <winsock2.h>
#include <windows.h>
#include <process.h>

#pragma warning(disable:4786)

#ifndef STDFUNC_FILENO
	#define STDFUNC_FILENO _fileno
#endif

#ifndef STDFUNC_GETPID
	#define STDFUNC_GETPID _getpid
#endif

#ifndef STDFUNC_UNLINK
	#define STDFUNC_UNLINK _unlink
#endif

extern bool NTWindow_Init( HINSTANCE hInstance, LPTSTR lpCmdLinel, int nCmdShow );
extern void NTWindow_Exit();
extern void NTWindow_DeleteIcon();
extern bool NTWindow_OnTick( int iWaitmSec );
extern bool NTWindow_PostMsg( LPCTSTR pszMsg );
extern bool NTWindow_PostMsgColor( COLORREF color );
extern void NTWindow_SetWindowTitle( LPCTSTR pText = NULL );

// since the only way to make windows not to buffer file is to remove buffer, we
// use this instead flushing
#define	FILE_SETNOCACHE(_x_)	setvbuf(_x_, NULL, _IONBF, 0)
#define FILE_FLUSH(_x_)

//	thread-specific definitions
#include "CTime.h"
#define THREAD_ENTRY_RET void

// printf format identifiers
#define FMTSIZE_T "Iu" // windows uses %Iu to format size_t


#ifdef __MINGW32__
	/*
	There is a problem with the UNREFERENCED_PARAMETER macro from mingw and sphereserver.
	operator= is on many clases private and the UNREFERENCED_PARAMETER macro from mingw is (P)=(P),
	so we have a compilation error here.
	*/
	#ifdef UNREFERENCED_PARAMETER
		#undef UNREFERENCED_PARAMETER
	#endif
	#define UNREFERENCED_PARAMETER(P)	(void)(P)
	// Not defined for mingw.
	#define LSTATUS int
	typedef void (__cdecl *_invalid_parameter_handler)(const wchar_t *,const wchar_t *,const wchar_t *,unsigned int,uintptr_t);
	// Stuctured exception handling windows api not implemented on mingw.
	#define __except(P)		catch(int)
#endif	// __MINGW32__

#endif	// _INC_OS_WINDOWS_H
