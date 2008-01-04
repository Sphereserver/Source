#ifndef OS_WINDOWS_H
#define OS_WINDOWS_H

#define _WIN32_DCOM

#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0501
#endif

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

extern bool NTWindow_Init( HINSTANCE hInstance, LPSTR lpCmdLinel, int nCmdShow );
extern void NTWindow_Exit();
extern bool NTWindow_OnTick( int iWaitmSec );
extern bool NTWindow_PostMsg( LPCTSTR pszMsg );
extern bool NTWindow_PostMsgColor( COLORREF color );
extern void NTWindow_SetWindowTitle( LPCTSTR pText = NULL );

// since the only way to make windows not to buffer file is to remove buffer, we
// use this instead flushing
#define	FILE_SETNOCACHE(_x_)	setvbuf(_x_, NULL, _IONBF, 0)
#define FILE_FLUSH(_x_)

#include "CTime.h"
#define THREAD_ENTRY_RET void

#endif