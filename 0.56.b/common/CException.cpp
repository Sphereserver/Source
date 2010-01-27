
#include "../graysvr/graysvr.h"

#ifdef _WIN32
	int CGrayError::GetSystemErrorMessage( DWORD dwError, LPTSTR lpszError, UINT nMaxError ) // static
	{
		//	PURPOSE:  copies error message text to a string
		//
		//	PARAMETERS:
		//		lpszBuf - destination buffer
		//		dwSize - size of buffer
		//
		//	RETURN VALUE:
		//		destination buffer

		LPCVOID lpSource = NULL;
		const va_list* Arguments = NULL;
		DWORD nChars = ::FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			lpSource,
			dwError, LANG_NEUTRAL, 
			lpszError, nMaxError, NULL );

		if (nChars > 0)
		{     // successful translation -- trim any trailing junk
			int index = nChars - 1;      // index of last character
			while ((index >= 0) && ((lpszError[index] == '\n') || (lpszError[index] == '\r')))
				lpszError[index--] = '\0';
			nChars = index + 1;
		}

		return nChars;
	}
#endif

bool CGrayError::GetErrorMessage( LPSTR lpszError, UINT nMaxError,	UINT * pnHelpContext )
{
#ifdef _WIN32
	// Compatible with CException and CFileException
	if ( m_hError )
	{
		// return the message defined by the system for the error code
		TCHAR szCode[ 1024 ];
		int nChars = GetSystemErrorMessage( m_hError, szCode, sizeof(szCode));
		if ( nChars )
		{
			if ( m_hError & 0x80000000 )
				sprintf( lpszError, "Error Pri=%d, Code=0x%x(%s), Desc='%s'", m_eSeverity, m_hError, szCode, m_pszDescription );
			else
				sprintf( lpszError, "Error Pri=%d, Code=%d(%s), Desc='%s'", m_eSeverity, m_hError, szCode, m_pszDescription );
			return( true );
		}
	}
#endif

	if ( m_hError & 0x80000000 )
		sprintf( lpszError, "Error Pri=%d, Code=0x%x, Desc='%s'", m_eSeverity, m_hError, m_pszDescription );
	else
		sprintf( lpszError, "Error Pri=%d, Code=%d, Desc='%s'", m_eSeverity, m_hError, m_pszDescription );
	return( true );
}

CGrayError::CGrayError( const CGrayError &e ) :
m_eSeverity( e.m_eSeverity ),
m_hError( e.m_hError ),
m_pszDescription( e.m_pszDescription )
{
}

CGrayError::CGrayError( LOGL_TYPE eSev, DWORD hErr, LPCTSTR pszDescription ) :
m_eSeverity( eSev ),
m_hError( hErr ),
m_pszDescription( pszDescription )
{
}
CGrayError::~CGrayError() 
{
}

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------

CGrayAssert::CGrayAssert(LOGL_TYPE eSeverity, LPCTSTR pExp, LPCTSTR pFile, unsigned uLine) :
CGrayError(eSeverity, 0, "Assert"), m_pExp(pExp), m_pFile(pFile), m_uLine(uLine)
{
}

CGrayAssert::~CGrayAssert()
{
}

bool CGrayAssert::GetErrorMessage(LPSTR lpszError, UINT nMaxError, UINT * pnHelpContext)
{
	sprintf(lpszError, "Assert pri=%d:'%s' file '%s', line %d", m_eSeverity, m_pExp, m_pFile, m_uLine);
	return true;
}

/*LPCTSTR const CGrayAssert::GetAssertFile()
{
	LPSTR pTmp = Str_GetTemp();
	strcpylen( pTmp, m_pFile, strlen( m_pFile ) ); //make a copy, don't send the original
	return pTmp;
}

const unsigned CGrayAssert::GetAssertLine()
{
	return m_uLine;
}*/

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------

#ifdef _WIN32

CGrayException::CGrayException(unsigned int uCode, DWORD dwAddress) :
m_dwAddress(dwAddress), CGrayError(LOGL_CRIT, uCode, "Exception")
{
}

CGrayException::~CGrayException()
{
}

BOOL CGrayException::GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT * pnHelpContext)
{
	const char *zMsg;
	switch ( m_hError )
	{
		case STATUS_BREAKPOINT:				zMsg = "Breakpoint";				break;
		case STATUS_ACCESS_VIOLATION:		zMsg = "Access Violation";			break;
		case STATUS_FLOAT_DIVIDE_BY_ZERO:	zMsg = "Float: Divide by Zero";		break;
		case STATUS_INTEGER_DIVIDE_BY_ZERO:	zMsg = "Integer: Divide by Zero";	break;
		case STATUS_STACK_OVERFLOW:			zMsg = "Stack Overflow";			break;
		default:
			sprintf(lpszError, "code=0x%x, (0x%x)", m_hError, m_dwAddress);
			return true;
	}

	sprintf(lpszError, "\"%s\" (0x%x)", zMsg, m_dwAddress);
	return true;
}

#endif

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------

void Assert_CheckFail( LPCTSTR pExp, LPCTSTR pFile, unsigned uLine )
{
	throw CGrayAssert(LOGL_CRIT, pExp, pFile, uLine);
}

#if defined(_WIN32) && !defined(_DEBUG)
	#include "./crashdump/crashdump.h"
	
	extern "C"
	{
		int _cdecl _purecall()
		{
			// catch this special type of C++ exception as well.
			Assert_CheckFail( "purecall", "unknown", 1 );
			return 0;
		}

		void _cdecl Sphere_Exception_Win32( unsigned int id, struct _EXCEPTION_POINTERS* pData )
		{
#ifndef _NO_CRASHDUMP
			if ( CrashDump::IsEnabled() )
			{
				CrashDump::StartCrashDump(GetCurrentProcessId(), GetCurrentThreadId(), pData);
			}
#endif
			// WIN32 gets an exception.
			DWORD dwCodeStart = (DWORD)(BYTE *) &globalstartsymbol;	// sync up to my MAP file.

			DWORD dwAddr = (DWORD)(pData->ExceptionRecord->ExceptionAddress);
			dwAddr -= dwCodeStart;

			throw CGrayException(id, dwAddr);
		}
	}
#endif

void SetExceptionTranslator()
{
#if defined(_WIN32) && !defined(_DEBUG)
	_set_se_translator( Sphere_Exception_Win32 );
#endif
}

#ifndef _WIN32
	void _cdecl Signal_Hangup( int sig = 0 ) // If shutdown is initialized
	{
		if ( !g_Serv.m_fResyncPause )
			g_World.Save(true);

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
		g_Serv.SetExitFlag(SIGABRT);
	}

	void _cdecl Signal_Break( int sig = 0 )
	{
		sigset_t set;

		g_Log.Event( LOGL_FATAL, "Secure Mode prevents CTRL+C\n" );

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

		g_Log.Event( LOGL_FATAL, "%s\n", strsignal(sig) );

		if ( sig )
		{
			signal( sig, &Signal_Illegal_Instruction );
			sigemptyset(&set);
			sigaddset(&set, sig);
			sigprocmask(SIG_UNBLOCK, &set, NULL);
		}

		throw CGrayError( LOGL_FATAL, sig, strsignal(sig) );
	}
#endif

void SetUnixSignals( bool bSet )
{
#ifndef _WIN32
	signal( SIGHUP,		bSet ? &Signal_Hangup : SIG_DFL );
	signal( SIGTERM,	bSet ? &Signal_Terminate : SIG_DFL );
	signal( SIGQUIT,	bSet ? &Signal_Terminate : SIG_DFL );
	signal( SIGABRT,	bSet ? &Signal_Terminate : SIG_DFL );
	signal( SIGILL,		bSet ? &Signal_Terminate : SIG_DFL );
	signal( SIGINT,		bSet ? &Signal_Break : SIG_DFL );
	signal( SIGSEGV,	bSet ? &Signal_Illegal_Instruction : SIG_DFL );
	signal( SIGFPE,		bSet ? &Signal_Illegal_Instruction : SIG_DFL );
	signal( SIGPIPE,	bSet ? SIG_IGN : SIG_DFL );
#endif
}

#ifdef _WIN32

STACK_INFO_REC g_stackInfo[0x1000];
long g_stackPos = 0;

StackDebugInformation::StackDebugInformation(const char *name) {
	g_stackInfo[g_stackPos].functionName = name;
	g_stackInfo[g_stackPos].startTime = ::GetTickCount();
	g_stackPos++;
	g_stackInfo[g_stackPos].startTime = 0;
}

StackDebugInformation::~StackDebugInformation() {
	g_stackPos--;
}

void StackDebugInformation::printStackTrace() {
	LONGLONG startTime = g_stackInfo[0].startTime;
	long timedelta;
	g_Log.EventDebug("__ # | _____ function _____________ | ticks passed from previous function start ______\n");
	for( int i = 0; i < 0x1000; i++ ) {
		if( g_stackInfo[i].startTime == 0 ) {
			break;
		}
		timedelta = (long)(g_stackInfo[i].startTime - startTime);
		g_Log.EventDebug(">>%2d | %28s | +%d %s\n",
			i, g_stackInfo[i].functionName, timedelta, ( i == g_stackPos-1 ) ?
				"<-- exception catch point (below is guessed and could be incorrect!)" :
				"");
		startTime = g_stackInfo[i].startTime;
	}
}

#endif
