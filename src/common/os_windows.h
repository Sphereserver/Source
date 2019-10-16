#ifndef _INC_OS_WINDOWS_H
#define _INC_OS_WINDOWS_H
#pragma once

#ifdef _WIN32
#undef FD_SETSIZE
#define FD_SETSIZE 1024	// for max of n users ! default = 64

#include <io.h>
#include <process.h>
#include <time.h>
#include <WinSock2.h>
#include <windows.h>

extern bool NTWindow_Init(HINSTANCE hInstance, LPTSTR lpCmdLinel, int nCmdShow);
extern void NTWindow_Exit();
extern void NTWindow_DeleteIcon();
extern bool NTWindow_OnTick(int iWaitmSec);
extern bool NTWindow_PostMsg(LPCTSTR pszMsg);
extern bool NTWindow_PostMsgColor(COLORREF color);
extern void NTWindow_SetWindowTitle(LPCTSTR pszText = NULL);

#ifdef __MINGW32__
	// On MinGW the 'operator=' is private on many classes and UNREFERENCED_PARAMETER macro on MinGW is (P)=(P), so we have a compilation error here
	#ifdef UNREFERENCED_PARAMETER
		#undef UNREFERENCED_PARAMETER
	#endif
	#define UNREFERENCED_PARAMETER(P)	(void)(P)

	// Not defined on MinGW
	typedef int		LSTATUS;
	typedef void(__cdecl *_invalid_parameter_handler)(const wchar_t *, const wchar_t *, const wchar_t *, unsigned int, uintptr_t);

	// Stuctured exception handling Windows API not implemented on MinGW
	#define __except(P)		catch(int)
#endif

#endif	// _WIN32
#endif	// _INC_OS_WINDOWS_H
