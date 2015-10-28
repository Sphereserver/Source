#if !defined(_CRASHDUMP_H) && defined(_WIN32) && !defined(_DEBUG) && !defined(_NO_CRASHDUMP)
#define _CRASHDUMP_H

#include <windows.h>
#pragma warning(disable:4091)
#include <Dbghelp.h>
#pragma warning(default:4091)
#include <stdio.h>

typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess,
										 DWORD dwPid,
										 HANDLE hFile,
										 MINIDUMP_TYPE DumpType,
										 CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
										 CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
										 CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
class CrashDump
{
	private:
		static bool m_bEnabled;
		static HMODULE m_hDll;
		static MINIDUMPWRITEDUMP m_tDumpFunction;

	public:
		static void StartCrashDump( DWORD , DWORD , struct _EXCEPTION_POINTERS* );
		static bool IsEnabled();
		static void Enable();
		static void Disable();

	private:
		CrashDump(void);
		CrashDump(const CrashDump& copy);
		CrashDump& operator=(const CrashDump& other);
};

#endif