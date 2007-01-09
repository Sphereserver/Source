//
// cthread.h
//

#ifndef _INC_CTHREAD_H
#define _INC_CTHREAD_H
#pragma once

#include "common.h"

class CThread	// basic multi tasking functionality.
{
protected:
	DWORD m_dwThreadID;	// unique thread id. ie. stack base pointer.
#ifdef _WIN32
	DWORD m_hThread;	// In windows there may be many handles to the same thread.
#else
	pthread_t m_hThread;
	pthread_attr_t m_hThread_attr;
#endif
	const char	*m_sName;
	CGTime		sm_timeRealPrev;
	PTHREAD_ENTRY_PROC m_proc;

public:
	static const char *m_sClassName;
	const char *GetName()
	{
		return m_sName;
	}

	CThread();
	~CThread()
	{
		TerminateThread( (DWORD) -1 );
	}
	static DWORD GetCurrentThreadId();
	void OnClose();
	void OnCreate()
	{
		// The id for the thread is diff from the handle.
		// There may be many handles but only one id !
		m_dwThreadID = GetCurrentThreadId();	
	}
	DWORD GetThreadID() const
	{
		return m_dwThreadID;
	}
	bool IsActive() const
	{
		return ( m_hThread ? true : false );
	}
	bool TerminateThread(DWORD dwExitCode);
	void CreateThread(PTHREAD_ENTRY_PROC pEntryProc, void * pArgs);
	void CreateThread(PTHREAD_ENTRY_PROC pEntryProc);
	void WaitForClose(int iSec = 15);

	// Periodically called to check if this thread is stuck. if so, then do something about it !
	void CheckStuckThread();
};

#endif	// _INC_CTHREAD_H
