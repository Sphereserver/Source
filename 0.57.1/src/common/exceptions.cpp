#include "exceptions.h"
#include "../graysvr.h"

#if defined(_WIN32) && !defined(_DEBUG)
#include "crashdump/crashdump.h"

// C-language specific exceptions like arifmetic, hardware, API, etc
void _cdecl Sphere_Exception_Win32(unsigned int id, struct _EXCEPTION_POINTERS* pData)
{
	if ( CrashDump::IsEnabled() )
	{
		CrashDump::StartCrashDump(GetCurrentProcessId(), GetCurrentThreadId(), pData);
	}

	CError	exc(LOGL_CRIT, id, "Exception");
	exc.m_address = (DWORD)pData->ExceptionRecord->ExceptionAddress;
	throw exc;
}

// catch this special type of C++ exception as well.
extern "C" {
int _cdecl _purecall()
{
	Assert_CheckFail("purecall", "unknown", 1);
	return 0;
}
}

#endif

#ifndef _WIN32

void _cdecl Signal_Hangup( int sig = 0 ) // If shutdown is initialized
{
	g_Serv.SetExitFlag(SIGHUP);
}

void _cdecl Signal_Terminate( int sig = 0 ) // If shutdown is initialized
{
	sigset_t set;
	
	g_Log.Event( LOGL_FATAL, "Server Unstable: %s\n", strsignal(sig) );
	
	if ( sig )
	{
		signal( sig, &Signal_Terminate);
		sigemptyset(&set);
		sigaddset(&set, sig);
		sigprocmask(SIG_UNBLOCK, &set, NULL);
	}

	pthread_exit(0);
}

void _cdecl Signal_Break( int sig = 0 )
{
	sigset_t set;

	g_Log.Event(LOGL_FATAL, "Secure Mode prevents CTRL+C\n" );
	
	if ( sig )
	{
		signal( sig, &Signal_Break );
		sigemptyset(&set);
		sigaddset(&set, sig);
		sigprocmask(SIG_UNBLOCK, &set, NULL);
	}
}

void _cdecl Signal_Illegal_Instruction( int sig = 0 )
{
	sigset_t set;

	g_Log.Event(LOGL_FATAL, "%s\n", strsignal(sig) );
	
	if ( sig )
	{
		signal( sig, &Signal_Illegal_Instruction );
		sigemptyset(&set);
		sigaddset(&set, sig);
		sigprocmask(SIG_UNBLOCK, &set, NULL);
	}

	throw CError(LOGL_FATAL, sig, strsignal(sig) );
}

#endif // _WIN32

void ExceptionsThreadInit()
{
#if defined(_WIN32) && !defined(_DEBUG)
	_set_se_translator(Sphere_Exception_Win32);
#endif

#ifndef _WIN32
	signal(SIGHUP,	&Signal_Hangup);
	signal(SIGTERM,	&Signal_Terminate);
	signal(SIGQUIT,	&Signal_Terminate);
	signal(SIGABRT,	&Signal_Terminate);
	signal(SIGILL,	&Signal_Terminate);
	signal(SIGINT,	&Signal_Break);
	signal(SIGSEGV,	&Signal_Illegal_Instruction);
	signal(SIGFPE,	&Signal_Illegal_Instruction);
	signal(SIGPIPE,	SIG_IGN);
#endif
}

void Assert_CheckFail(const char *pExp, const char *pFile, long uLine)
{
	CError	err(LOGL_CRIT, 0, "Assert");
	err.m_exp = pExp;
	err.m_file = pFile;
	err.m_line = uLine;
	throw err;
}

//	//	//	//	//	//	//	//
//
//		CError
//
//	//	//	//	//	//	//	//

void CError::GetErrorMessage(LPSTR lpszError)
{
#ifdef _WIN32
	if ( m_address )			// normal exception
	{
		const char *zMsg;
		switch ( m_error )
		{
			case STATUS_BREAKPOINT:				zMsg = "Breakpoint"; break;
			case STATUS_ACCESS_VIOLATION:		zMsg = "Access Violation"; break;
			case STATUS_FLOAT_DIVIDE_BY_ZERO:	zMsg = "Float: Divide by Zero"; break;
			case STATUS_INTEGER_DIVIDE_BY_ZERO:	zMsg = "Integer: Divide by Zero"; break;
			case STATUS_STACK_OVERFLOW:			zMsg = "Stack Overflow"; break;
			default:
				sprintf(lpszError, "code=0x%x, (0x%x)", m_error, m_address);
				return;
		}
		sprintf(lpszError, "\"%s\" (0x%x)", zMsg, m_address);
		return;
	}
	else if ( m_error )			// system-wide
	{
		char	z[512];
		int n = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, NULL, m_error, LANG_NEUTRAL, z, sizeof(z), NULL);
		if ( n )
		{
			int index = n - 1;
			while ( (index >= 0) && ((z[index] == '\n') || (z[index] == '\r')) )
				z[index--] = 0;
			n = index + 1;

			if ( n )
			{
				sprintf(lpszError, "Error Pri=%d, Code=%d(%s), Desc='%s'",
					m_severity, m_error, z, m_descr);
				return;
			}
		}
	}
#endif
	if ( m_file && *m_file )	// assert
	{
		sprintf(lpszError,
			"Assert pri=%d:'%s' file '%s', line %d",
			m_severity, m_exp, m_file, m_line);
	}
	else						// general exception
	{
		sprintf(lpszError,
			"Error Pri=%d, Code=%d, Desc='%s'",
			m_severity, m_error, m_descr);
	}
}


CError::CError(const CError &e) :
	m_severity(e.m_severity), m_error(e.m_error), m_descr(e.m_descr)
{
	m_exp = e.m_exp;
	m_file = e.m_file;
	m_line = e.m_line;
	m_address = e.m_address;
}

CError::CError(long eSev, DWORD hErr, LPCTSTR pszDescription) :
	m_severity(eSev), m_error(hErr), m_descr(pszDescription)
{
	m_exp = "";
	m_file = "";
	m_line = 0;
	m_address = 0;
}

//	//	//	//	//	//	//	//
//
//		CCallStack
//
//	//	//	//	//	//	//	//

CCallStack::CCallStack(const char *className, const char *funcName, CThread *thread, long stackpos)
{
	if ( !thread )
		thread = CThread::Thread();
	if ( thread && !stackpos )
		stackpos = thread->ExcCount();

	m_stackpos = stackpos;
	m_thread = thread;

	if ( m_thread )
		m_thread->ExcRegister(className, funcName);
}

CCallStack::~CCallStack()
{
	if ( m_thread )
		m_thread->ExcCount(m_stackpos);
}
