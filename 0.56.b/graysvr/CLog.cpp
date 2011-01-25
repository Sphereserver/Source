//
// CLog.cpp
// Copyright Menace Software (www.menasoft.com).
//
#include "graysvr.h"	// predef header.

///////////////////////////////////////////////////////////////
// -CLog

bool CLog::OpenLog( LPCTSTR pszBaseDirName )	// name set previously.
{
	if ( m_fLockOpen )	// the log is already locked open
		return( false );

	if ( m_sBaseDir == NULL )
		return false;

	if ( pszBaseDirName != NULL )
	{
		if ( pszBaseDirName[0] && pszBaseDirName[1] == '\0' )
		{
			if ( *pszBaseDirName == '0' )
			{
				Close();
				return false;
			}
		}
		else
		{
			m_sBaseDir = pszBaseDirName;
		}
	}

	// Get the new name based on date.
	m_dateStamp = CGTime::GetCurrentTime();
	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, GRAY_FILE "%d-%02d-%02d.log",
		m_dateStamp.GetYear(), m_dateStamp.GetMonth(), m_dateStamp.GetDay());
	CGString sFileName = GetMergedFileName(m_sBaseDir, pszTemp);

	// Use the OF_READWRITE to append to an existing file.
	if ( CFileText::Open( sFileName, OF_SHARE_DENY_NONE|OF_READWRITE|OF_TEXT ) )
	{
		setvbuf(m_pStream, NULL, _IONBF, 0);
		return true;
	}
	return false;
}

int CLog::EventStr( DWORD wMask, LPCTSTR pszMsg )
{
	// NOTE: This could be called in odd interrupt context so don't use dynamic stuff
	if ( !IsLogged(wMask) )	// I don't care about these.
		return 0;
	else if ( !pszMsg || !*pszMsg )
		return 0;

	int iRet = 0;
	m_mutex.lock();

	try
	{

		// Put up the date/time.
		CGTime datetime = CGTime::GetCurrentTime();	// last real time stamp.

		if ( datetime.GetDaysTotal() != m_dateStamp.GetDaysTotal())
		{
			// it's a new day, open with new day name.
			Close();	// LINUX should alrady be closed.

			OpenLog( NULL );
			Printf( datetime.Format(NULL));
		}
		else
		{
#ifndef _WIN32
			UINT	mode = OF_READWRITE|OF_TEXT;
			mode |= OF_SHARE_DENY_WRITE;
			Open(NULL, mode);	// LINUX needs to close and re-open for each log line !
#endif
		}

		TCHAR szTime[32];
		sprintf(szTime, "%02d:%02d:", datetime.GetHour(), datetime.GetMinute());
		m_dateStamp = datetime;

		LPCTSTR pszLabel = NULL;

		switch (wMask & 0x07)
		{
			case LOGL_FATAL:	// fatal error !
				pszLabel = "FATAL:";
				break;
			case LOGL_CRIT:	// critical.
				pszLabel = "CRITICAL:";
				break;
			case LOGL_ERROR:	// non-fatal errors.
				pszLabel = "ERROR:";
				break;
			case LOGL_WARN:
				pszLabel = "WARNING:";
				break;
		}
		if ( !pszLabel && ( wMask & LOGM_DEBUG ) && !( wMask & LOGM_INIT ))
			pszLabel = "DEBUG:";

		// Get the script context. (if there is one)
		TCHAR szScriptContext[ _MAX_PATH + 16 ];
		if ( !( wMask&LOGM_NOCONTEXT ) && m_pScriptContext )
		{
			CScriptLineContext LineContext = m_pScriptContext->GetContext();
			sprintf( szScriptContext, "(%s,%d)", m_pScriptContext->GetFileTitle(), LineContext.m_iLineNum );
		}
		else
		{
			szScriptContext[0] = '\0';
		}

		// Print to screen.
		if ( ! ( wMask & LOGM_INIT ) && ! g_Serv.IsLoading())
		{
#ifdef _WIN32
			NTWindow_PostMsgColor( RGB( 127,127,0 ));
#else
			if( m_fColoredConsole )
			{
				g_Serv.PrintStr("\e[0;33m");
			}
#endif

			g_Serv.PrintStr( szTime );

#ifdef _WIN32
			NTWindow_PostMsgColor(0);
#else
			if( m_fColoredConsole )
			{
				g_Serv.PrintStr("\e[0m");
			}
#endif
		}

		if ( pszLabel )	// some sort of error
		{
#ifdef _WIN32
			NTWindow_PostMsgColor( RGB( 255,0,0 ));
#else
			if( m_fColoredConsole )
			{
				g_Serv.PrintStr("\e[0;31m");
			}
#endif

			g_Serv.PrintStr( pszLabel );

#ifdef _WIN32
			NTWindow_PostMsgColor( RGB( 255,255,255 ));
#else
			if( m_fColoredConsole )
			{
				g_Serv.PrintStr("\e[0m");
			}
#endif
		}

		if ( szScriptContext[0] )
		{
#ifdef _WIN32
			NTWindow_PostMsgColor( RGB( 0,127,255 ));
#else
			if( m_fColoredConsole )
			{
				g_Serv.PrintStr("\e[1;36m");
			}
#endif

			g_Serv.PrintStr( szScriptContext );

#ifdef _WIN32
			NTWindow_PostMsgColor(0);
#else
			if( m_fColoredConsole )
			{
				g_Serv.PrintStr("\e[0m");
			}
#endif
		}
		g_Serv.PrintStr( pszMsg );

		// Back to normal color.
#ifdef _WIN32
		NTWindow_PostMsgColor(0);
#endif

		// Print to log file.
		WriteString( szTime );
		if ( pszLabel )	WriteString( pszLabel );
		if ( szScriptContext[0] )
		{
			WriteString( szScriptContext );
		}
		WriteString( pszMsg );

		iRet = 1;

#ifndef _WIN32
		Close();
#endif
	}
	catch (...)
	{
		// Not much we can do about this
		iRet = 0;
	}

	m_mutex.unlock();

	return( iRet );
}

CGTime CLog::sm_prevCatchTick;

void _cdecl CLog::CatchEvent( const CGrayError * pErr, LPCTSTR pszCatchContext, ... )
{
	CGTime timeCurrent = CGTime::GetCurrentTime();
	if ( sm_prevCatchTick.GetTime() == timeCurrent.GetTime() )	// prevent message floods.
		return;
	// Keep a record of what we catch.
	try
	{
		TCHAR szMsg[512];
		LOGL_TYPE eSeverity;
		int iLen = 0;
		if ( pErr != NULL )
		{
			eSeverity = pErr->m_eSeverity;
			const CGrayAssert * pAssertErr = dynamic_cast<const CGrayAssert*>(pErr);
			if ( pAssertErr )
				pAssertErr->GetErrorMessage(szMsg, sizeof(szMsg), 0);
			else
				pErr->GetErrorMessage(szMsg, sizeof(szMsg));
			iLen = strlen(szMsg);
		}
		else
		{
			eSeverity = LOGL_CRIT;
			strcpy(szMsg, "Exception");
			iLen = strlen(szMsg);
		}

		iLen += sprintf( szMsg+iLen, ", in " );

		va_list vargs;
		va_start(vargs, pszCatchContext);

		iLen += vsprintf(szMsg+iLen, pszCatchContext, vargs);
		iLen += sprintf(szMsg+iLen, "\n");

		EventStr(eSeverity, szMsg);
		va_end(vargs);
	}
	catch(...)
	{
		// Not much we can do about this.
		pErr = NULL;
	}
	sm_prevCatchTick = timeCurrent;
}
