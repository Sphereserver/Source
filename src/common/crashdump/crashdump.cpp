#if defined(_WIN32) && !defined(_DEBUG) && !defined(_NO_CRASHDUMP)

#include "crashdump.h"

bool CrashDump::m_bEnabled = false;
HMODULE CrashDump::m_hDll = NULL;
MINIDUMPWRITEDUMP CrashDump::m_tDumpFunction = NULL;

bool CrashDump::IsEnabled()
{
	return m_bEnabled;
}

void CrashDump::Enable()
{
	TCHAR szLibPath[MAX_PATH];
	GetSystemDirectory(szLibPath, sizeof(szLibPath));
	strncat(szLibPath, "\\dbghelp.dll", sizeof(szLibPath) - strlen(szLibPath) - 1);

	m_hDll = LoadLibrary(szLibPath);
	if (!m_hDll)
	{
		m_bEnabled = false;
		return;
	}

	m_tDumpFunction = reinterpret_cast<MINIDUMPWRITEDUMP>(GetProcAddress(m_hDll, "MiniDumpWriteDump"));
	if (!m_tDumpFunction)
	{
		m_bEnabled = false;
		FreeLibrary(m_hDll);
		return;
	}

	m_bEnabled = true;
}

void CrashDump::Disable()
{
	m_bEnabled = false;

	if (m_tDumpFunction != NULL)
		m_tDumpFunction = NULL;

	if (m_hDll != NULL)
	{
		FreeLibrary(m_hDll);
		m_hDll = NULL;
	}
}

void CrashDump::StartCrashDump( DWORD processID, DWORD threadID, struct _EXCEPTION_POINTERS* pExceptionData ) 
{
	if (!processID || !threadID || !pExceptionData)
		return;

	HANDLE process = GetCurrentProcess();
	if (process == NULL)
		return;
	
	SYSTEMTIME st;
	GetSystemTime(&st);

	char szFileName[FILENAME_MAX];
	snprintf(szFileName, sizeof(szFileName), "sphereCrash_%02hu%02hu-%02hu%02hu%02hu%04hu.dmp", st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	HANDLE dumpFile = CreateFile(szFileName, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
	if (dumpFile == INVALID_HANDLE_VALUE)
		return;
	
	MINIDUMP_EXCEPTION_INFORMATION eInfo;
	eInfo.ThreadId = threadID;
	eInfo.ExceptionPointers = pExceptionData;
	eInfo.ClientPointers = true;
	
	m_tDumpFunction(process, processID, dumpFile, MiniDumpWithDataSegs, &eInfo, NULL, NULL);
	
	CloseHandle(dumpFile);
	return;
}

#endif