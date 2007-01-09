//
// cthread.cpp
//

#include "CThread.h"
#include "../graysvr/graysvr.h"

/////////////
// -CThread

CThread::CThread()
{
	m_dwThreadID = 0;
	m_hThread = 0;
	sm_timeRealPrev = 0;
	m_sName = "unknown";
}

void CThread::CreateThread(PTHREAD_ENTRY_PROC pEntryProc, void *pArgs)
{
	if ( IsActive() )
		return;	// Already active.

	m_proc = pEntryProc;

#ifdef _WIN32
	m_hThread = _beginthread( pEntryProc, 0, pArgs );
	if ( m_hThread == -1 )
	{
		m_hThread = NULL;
	}
#else
	fprintf(stderr, "Creating thread.\n");

	pthread_attr_init(&m_hThread_attr);
	int status = pthread_create( &m_hThread, &m_hThread_attr, pEntryProc, pArgs );
	if ( status != 0 )
	{
		DEBUG_ERR(("Unable to create new thread\n"));
		m_hThread = NULL;
	}
#endif
}

void CThread::CreateThread(PTHREAD_ENTRY_PROC pEntryProc)
{
	CreateThread(pEntryProc, this);
}

DWORD CThread::GetCurrentThreadId() // static
{
#ifdef _WIN32
	return ::GetCurrentThreadId();
#else
	return pthread_self();
#endif
}

bool CThread::TerminateThread(DWORD dwExitCode)
{
	if ( !IsActive() )
		return true;
#ifdef _WIN32
	if ( m_dwThreadID == GetCurrentThreadId() )
	{
		_endthread();	// , dwExitCode
	}
	else
	{
		// Terminate a thread that is not us.
		if ( ::TerminateThread((HANDLE)m_hThread, dwExitCode) )
			CloseHandle((HANDLE)m_hThread);
	}
#else
	pthread_attr_destroy(&m_hThread_attr);
	// LINUX code to force a thread to exit !
	// But it may not be THIS thread !
	if ( m_dwThreadID == (DWORD) pthread_self())
	{
		pthread_exit((void *) &dwExitCode);
	}
	else
	{
		// Terminate a thread that is not us.
		pthread_kill( m_hThread, 1 );	// int sig
	}
#endif
	OnClose();
	return true;
}

void CThread::WaitForClose(int iSec)
{
	// wait for this thread to close.
	int icount = iSec*10;
	while ( IsActive() && icount -- )
	{
		Sleep(100);	// milliseconds
	}
	// Try to terminate it hard.
	TerminateThread(-1);
}

void CThread::OnClose()
{
	m_hThread = NULL;
}

void CThread::CheckStuckThread()
{
	ADDTOCALLSTACK("CThread::CheckStuckThread");
	static int inited = 0;
	static CServTime m_timeRestart;
	static CServTime m_timeWarn;
	static CServTime m_timePrev;

	if ( !inited )
	{
		m_timeRestart.Init();
		m_timeWarn.Init();
		m_timePrev.Init();
		inited = 1;
	}

	// Periodically called to check if the tread is stuck.

	CGTime timeRealCur = CGTime::GetCurrentTime();
	int iTimeDiff = timeRealCur.GetTime() - sm_timeRealPrev.GetTime();
	iTimeDiff = abs(iTimeDiff);
	
	if ( iTimeDiff < g_Cfg.m_iFreezeRestartTime )
		return;
	sm_timeRealPrev = timeRealCur;

	// Has server time changed ?
	CServTime timeCur = CServTime::GetCurrentTime();
	if ( timeCur != m_timePrev )	// Seems ok.
	{
		m_timePrev = timeCur;
		return;
	}

	if ( g_Serv.IsValidBusy() )	// Server is just busy.
		return;

	if ( m_timeRestart == timeCur )
	{
		g_Log.Event(LOGL_FATAL, "%s loop freeze RESTART FAILED!\n", m_sName);
	}

	if ( m_timeWarn == timeCur )
	{
		// Kill and revive the main process
		g_Log.Event(LOGL_CRIT, "%s loop freeze RESTART!\n", m_sName);

#ifndef _DEBUG
		TerminateThread(0xDEAD);

				// try to restart it.
		g_Log.Event(LOGL_EVENT, "Trying to restart the %s loop thread\n", m_sName);
		CreateThread(m_proc, this);
#endif
		m_timeRestart = timeCur;
	}
	else
	{
		g_Log.Event(LOGL_WARN, "%s loop frozen ?\n", m_sName);
		m_timeWarn = timeCur;
	}
}
