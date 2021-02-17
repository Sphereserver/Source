#include "graysvr.h"	// predef header.
#include "CLog.h"
#ifndef _WIN32
	#include "CUnixTerminal.h"
#endif

///////////////////////////////////////////////////////////////
// -CLog

bool CLog::OpenLog( LPCTSTR pszBaseDirName )	// name set previously.
{
	if ( m_fLockOpen )	// the log is already locked open
		return false;
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
	TCHAR szFileName[FILENAME_MAX];
	snprintf(szFileName, sizeof(szFileName), SPHERE_FILE "%d-%02d-%02d.log", m_dateStamp.GetYear(), m_dateStamp.GetMonth(), m_dateStamp.GetDay());

	// Use the OF_READWRITE to append to an existing file.
	if ( CFileText::Open(GetMergedFileName(m_sBaseDir, szFileName), OF_SHARE_DENY_NONE|OF_READWRITE|OF_TEXT) )
	{
		setvbuf(m_pStream, NULL, _IONBF, 0);
		return true;
	}
	return false;
}

void CLog::SetColor( Color color )
{
	// Change current console color to the specified one
	// NOTE: Value should be reseted after used
	switch ( color )
	{
#ifdef _WIN32
		case WHITE:
			NTWindow_PostMsgColor(RGB(255, 255, 255));
			break;
		case YELLOW:
			NTWindow_PostMsgColor(RGB(127, 127, 0));
			break;
		case RED:
			NTWindow_PostMsgColor(RGB(255, 0, 0));
			break;
		case CYAN:
			NTWindow_PostMsgColor(RGB(0, 127, 255));
			break;
		default:
			NTWindow_PostMsgColor(0);
#else
		case WHITE:
			g_UnixTerminal.setColor(CUnixTerminal::COL_WHITE);
			break;
		case YELLOW:
			g_UnixTerminal.setColor(CUnixTerminal::COL_YELLOW);
			break;
		case RED:
			g_UnixTerminal.setColor(CUnixTerminal::COL_RED);
			break;
		case CYAN:
			g_UnixTerminal.setColor(CUnixTerminal::COL_CYAN);
			break;
		default:
			g_UnixTerminal.setColor(CUnixTerminal::COL_DEFAULT);
#endif
	}
}

int CLog::EventStr( DWORD dwMask, LPCTSTR pszMsg )
{
	// NOTE: This could be called in odd interrupt context so don't use dynamic stuff
	if ( !IsLogged(dwMask) )	// I don't care about these.
		return 0;
	else if ( !pszMsg )
		return 0;

	int iRet = 0;
	m_mutex.lock();

	try
	{
		CGTime datetime = CGTime::GetCurrentTime();
		if ( datetime.GetDay() != m_dateStamp.GetDay() )	// it's a new day, open with new day name.
		{
			Close();	// LINUX should already be closed.
			OpenLog();
		}
#ifndef _WIN32
		else
		{
			// Linux needs to close and re-open for each log line
			static_cast<void>(Open(NULL, OF_READWRITE|OF_TEXT|OF_SHARE_DENY_WRITE));
		}
#endif

		TCHAR szTime[7];
		snprintf(szTime, sizeof(szTime), "%02d:%02d:", datetime.GetHour(), datetime.GetMinute());
		m_dateStamp = datetime;

		LPCTSTR pszLabel = NULL;
		switch ( dwMask & LOGL_QTY )
		{
			case LOGL_FATAL:
				pszLabel = "FATAL:";
				break;
			case LOGL_CRIT:
				pszLabel = "CRITICAL:";
				break;
			case LOGL_ERROR:
				pszLabel = "ERROR:";
				break;
			case LOGL_WARN:
				pszLabel = "WARNING:";
				break;
		}
		if ( !pszLabel && (dwMask & LOGM_DEBUG) )
			pszLabel = "DEBUG:";

		// Get the script context. (if there is one)
		TCHAR szScriptContext[_MAX_PATH + 16];
		if ( m_pScriptContext && !(dwMask & LOGM_NOCONTEXT) )
		{
			CScriptLineContext LineContext = m_pScriptContext->GetContext();
			snprintf(szScriptContext, sizeof(szScriptContext), "(%s,%d)", m_pScriptContext->GetFileTitle(), LineContext.m_iLineNum);
		}
		else
		{
			szScriptContext[0] = '\0';
		}

		// Print to screen.
		if ( !g_Serv.IsLoading() )
		{
			SetColor(YELLOW);
			g_Serv.PrintStr(szTime);
			SetColor(DEFAULT);
		}

		if ( pszLabel )	// some sort of error
		{
			SetColor(RED);
			g_Serv.PrintStr(pszLabel);
			SetColor(WHITE);
		}

		if ( szScriptContext[0] )
		{
			SetColor(CYAN);
			g_Serv.PrintStr(szScriptContext);
			SetColor(DEFAULT);
		}
		g_Serv.PrintStr(pszMsg);

		// Back to normal color.
		SetColor(DEFAULT);

		// Print to log file.
		WriteString(szTime);
		if ( pszLabel )
			WriteString(pszLabel);
		if ( szScriptContext[0] )
			WriteString(szScriptContext);
		WriteString(pszMsg);

		iRet = 1;

#ifndef _WIN32
		Close();
#endif
	}
	catch ( ... )
	{
		// Not much we can do about this
		iRet = 0;
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}

	m_mutex.unlock();
	return iRet;
}

CGTime CLog::sm_prevCatchTick;

void _cdecl CLog::CatchEvent( const CGrayError *pErr, LPCTSTR pszCatchContext, ... )
{
	CGTime timeCurrent = CGTime::GetCurrentTime();
	if ( sm_prevCatchTick.GetTime() == timeCurrent.GetTime() )	// prevent message floods.
		return;

	// Keep a record of what we catch.
	try
	{
		LOGL_TYPE eSeverity;
		TCHAR szMsg[512];

		if ( pErr )
		{
			eSeverity = pErr->m_eSeverity;
			const CGrayAssert *pAssertErr = dynamic_cast<const CGrayAssert *>(pErr);
			if ( pAssertErr )
				pAssertErr->GetErrorMessage(szMsg);
			else
				pErr->GetErrorMessage(szMsg);
		}
		else
		{
			eSeverity = LOGL_CRIT;
			strcpy(szMsg, "Exception");
		}

		strncat(szMsg, ", in ", sizeof(szMsg) - 1);

		va_list vargs;
		va_start(vargs, pszCatchContext);
		vsprintf(szMsg + strlen(szMsg), pszCatchContext, vargs);
		va_end(vargs);

		strncat(szMsg, "\n", sizeof(szMsg) - 1);
		EventStr(eSeverity, szMsg);
	}
	catch ( ... )
	{
		// Not much we can do about this.
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	sm_prevCatchTick = timeCurrent;
}
