#include "../graysvr/graysvr.h"

void Assert_CheckFail(LPCTSTR pszExp, LPCTSTR pszFile, long lLine)
{
	throw CGrayAssert(LOGL_CRIT, pszExp, pszFile, lLine);
}

#ifdef _WIN32

	#ifndef _DEBUG
		#include "./crashdump/crashdump.h"

		extern "C"
		{
			int _cdecl _purecall()
			{
				// Catch this special type of C++ exception as well
				Assert_CheckFail("purecall", "unknown", 1);
				return 0;
			}

			void _cdecl Sphere_Exception_Win32(unsigned int id, struct _EXCEPTION_POINTERS *pData)
			{
		#ifndef _NO_CRASHDUMP
				if ( CrashDump::IsEnabled() )
					CrashDump::StartCrashDump(GetCurrentProcessId(), GetCurrentThreadId(), pData);
		#endif
				// WIN32 gets an exception.
				DWORD dwCodeStart = (DWORD)(BYTE *)&globalstartsymbol;	// sync up to my MAP file

				DWORD dwAddr = (DWORD)(pData->ExceptionRecord->ExceptionAddress);
				dwAddr -= dwCodeStart;

				throw CGrayException(id, dwAddr);
			}
		}

		#ifndef __MINGW32__
			void SetExceptionTranslator()
			{
				_set_se_translator(Sphere_Exception_Win32);
			}
		#endif

	#endif

#else

	void _cdecl Signal_Hangup(int sig = 0)	// if shutdown is initialized
	{
		UNREFERENCED_PARAMETER(sig);
		if ( !g_Serv.m_fResyncPause )
			g_World.Save(true);

		g_Serv.SetExitFlag(SIGHUP);
	}

	void _cdecl Signal_Terminate(int sig = 0)	// if shutdown is initialized
	{
		g_Log.Event(LOGL_FATAL, "Server Unstable: %s\n", strsignal(sig));

		if ( sig )
		{
			sigset_t set;
			signal(sig, &Signal_Terminate);
			sigemptyset(&set);
			sigaddset(&set, sig);
			sigprocmask(SIG_UNBLOCK, &set, NULL);
		}

		pthread_exit(0);
		g_Serv.SetExitFlag(SIGABRT);
	}

	void _cdecl Signal_Break(int sig = 0)
	{
		g_Log.Event(LOGL_FATAL, "Secure mode prevents keyboard exit. Use 'S' to disable secure mode first\n");

		if ( sig )
		{
			sigset_t set;
			signal(sig, &Signal_Break);
			sigemptyset(&set);
			sigaddset(&set, sig);
			sigprocmask(SIG_UNBLOCK, &set, NULL);
		}
	}

	void _cdecl Signal_Illegal_Instruction(int sig = 0)
	{
		PAUSECALLSTACK;

		g_Log.Event(LOGL_FATAL, "%s\n", strsignal(sig));
	#ifdef THREAD_TRACK_CALLSTACK
		StackDebugInformation::printStackTrace();
	#endif

		if ( sig )
		{
			sigset_t set;
			signal(sig, &Signal_Illegal_Instruction);
			sigemptyset(&set);
			sigaddset(&set, sig);
			sigprocmask(SIG_UNBLOCK, &set, NULL);
		}

		UNPAUSECALLSTACK;
		throw CGrayError(LOGL_FATAL, sig, strsignal(sig));
	}

	void _cdecl Signal_Children(int sig = 0)
	{
		while ( waitpid((pid_t)(-1), 0, WNOHANG) > 0 ) { }
	}

	void SetSignals(bool bSet)
	{
		signal(SIGHUP,	bSet ? &Signal_Hangup : SIG_DFL);
		signal(SIGTERM,	bSet ? &Signal_Terminate : SIG_DFL);
		signal(SIGQUIT,	bSet ? &Signal_Terminate : SIG_DFL);
		signal(SIGABRT,	bSet ? &Signal_Terminate : SIG_DFL);
		signal(SIGILL,	bSet ? &Signal_Terminate : SIG_DFL);
		signal(SIGINT,	bSet ? &Signal_Break : SIG_DFL);
		signal(SIGSEGV,	bSet ? &Signal_Illegal_Instruction : SIG_DFL);
		signal(SIGFPE,	bSet ? &Signal_Illegal_Instruction : SIG_DFL);
		signal(SIGPIPE,	bSet ? SIG_IGN : SIG_DFL);
		signal(SIGCHLD,	bSet ? &Signal_Children : SIG_DFL);
	}

#endif

///////////////////////////////////////////////////////////
// CGrayError

CGrayError::CGrayError(LOGL_TYPE eSev, DWORD hErr, LPCTSTR pszDescription) : m_eSeverity(eSev), m_hError(hErr), m_pszDescription(pszDescription)
{
}

CGrayError::CGrayError(const CGrayError &e) : m_eSeverity(e.m_eSeverity), m_hError(e.m_hError), m_pszDescription(e.m_pszDescription)
{
}

#ifdef _WIN32

int CGrayError::GetSystemErrorMessage(DWORD dwError, LPTSTR lpszError, DWORD dwMaxError) // static
{
	LPCVOID lpSource = NULL;
	va_list *Arguments = NULL;

	DWORD dwChars = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, lpSource, dwError, LANG_NEUTRAL, lpszError, dwMaxError, Arguments);
	if ( dwChars > 0 )
	{
		// Successful translation, trim any trailing junk
		DWORD index = dwChars - 1;	// index of last character
		while ( (index >= 0) && ((lpszError[index] == '\n') || (lpszError[index] == '\r')) )
			lpszError[index--] = '\0';
		dwChars = index + 1;
	}
	return dwChars;
}

#endif

bool CGrayError::GetErrorMessage(LPTSTR lpszError) const
{
#ifdef _WIN32
	// Compatible with CException and CFileException
	if ( m_hError )
	{
		// Return the message defined by the system for the error code
		TCHAR szCode[1024];
		int nChars = GetSystemErrorMessage(m_hError, szCode, sizeof(szCode));
		if ( nChars )
		{
			if ( m_hError & 0x80000000 )
				sprintf(lpszError, "Error Pri=%d, Code=0x%lx(%s), Desc='%s'", m_eSeverity, m_hError, szCode, m_pszDescription);
			else
				sprintf(lpszError, "Error Pri=%d, Code=%lu(%s), Desc='%s'", m_eSeverity, m_hError, szCode, m_pszDescription);
			return true;
		}
	}
#endif

	if ( m_hError & 0x80000000 )
		sprintf(lpszError, "Error Pri=%d, Code=0x%lx, Desc='%s'", m_eSeverity, m_hError, m_pszDescription);
	else
		sprintf(lpszError, "Error Pri=%d, Code=%lu, Desc='%s'", m_eSeverity, m_hError, m_pszDescription);
	return true;
}

///////////////////////////////////////////////////////////
// CGrayAssert

CGrayAssert::CGrayAssert(LOGL_TYPE eSeverity, LPCTSTR pszExp, LPCTSTR pszFile, long lLine) : CGrayError(eSeverity, 0, "Assert"), m_pszExp(pszExp), m_pszFile(pszFile), m_lLine(lLine)
{
}

bool CGrayAssert::GetErrorMessage(LPTSTR lpszError) const
{
	sprintf(lpszError, "Assert pri=%d:'%s' file '%s', line %ld", m_eSeverity, m_pszExp, m_pszFile, m_lLine);
	return true;
}

/*
LPCTSTR const CGrayAssert::GetAssertFile()
{
	LPTSTR pszTmp = Str_GetTemp();
	strcpylen(pszTmp, m_pszFile, strlen(m_pszFile));	//make a copy, don't send the original
	return pszTmp;
}

const unsigned CGrayAssert::GetAssertLine()
{
	return m_lLine;
}
*/

///////////////////////////////////////////////////////////
// CGrayException

#ifdef _WIN32

CGrayException::CGrayException(unsigned int uCode, DWORD dwAddress) : m_dwAddress(dwAddress), CGrayError(LOGL_CRIT, uCode, "Exception")
{
}

bool CGrayException::GetErrorMessage(LPTSTR lpszError) const
{
	LPCTSTR pszMsg;
	switch ( m_hError )
	{
		case STATUS_BREAKPOINT:				pszMsg = "Breakpoint";				break;
		case STATUS_ACCESS_VIOLATION:		pszMsg = "Access Violation";		break;
		case STATUS_FLOAT_DIVIDE_BY_ZERO:	pszMsg = "Float: Divide by Zero";	break;
		case STATUS_INTEGER_DIVIDE_BY_ZERO:	pszMsg = "Integer: Divide by Zero";	break;
		case STATUS_STACK_OVERFLOW:			pszMsg = "Stack Overflow";			break;
		default:
			sprintf(lpszError, "code=0x%lx, (0x%lx)", m_hError, m_dwAddress);
			return true;
	}

	sprintf(lpszError, "\"%s\" (0x%lx)", pszMsg, m_dwAddress);
	return true;
}

#endif
