#include "threads.h"

vector<CThread*>	g_Threads;

//	//	//	//	//	//	//	//
//
//		CThreadSafe
//
//	//	//	//	//	//	//	//

const char *CThreadSafe::m_sClassName = "CThreadSafe";

CThreadSafe::CThreadSafe()
{
	m_lockThread = 0;
}

CThreadSafe::~CThreadSafe()
{
	Release();
}

void CThreadSafe::Lock(bool critical)
{
	CThread	*thread = CThread::Thread();

	if ( !thread || thread->Critical() )
		return;

	DWORD	id = thread->ID();

	//	some locks really require critical mode to enable normal sync
	if ( critical )
	{
		thread->CriticalStart();
		id |= THREAD_LOCK_MASK;
	}
	else
	{
		//	we cannot enter critical here, since then the lock will never be released by another thread :)
		//	thus overlaps are still possible, however very very rarely and should not be fatal in most cases
		long waitTime = 0;
		while ( m_lockThread && ( m_lockThread != id ))
		{
			Sleep(0);
			waitTime++;

			//	just ensure that the thread will not hang forever unable to lock the object. just resume it's
			//	functionality after some time hoping that nothing bad will happen :) really-really bad things
			//	should occure in critical sections only btw
			if ( waitTime > 512 )
			{
				const char					*oldThread = "unknown";
				vector<CThread *>::iterator it;
				for ( it = g_Threads.begin(); it != g_Threads.end(); it++ )
				{
					if ( (*it)->ID() == (m_lockThread & ~ THREAD_LOCK_MASK) )
					{
						oldThread = (*it)->Name();
						break;
					}
				}
#ifdef _DEBUG
				g_Log.Warn("Unable to lock object by thread '%s', taken by '%s'. Could be critical.\n", thread->Name(), oldThread);
#endif
				return;
			}
		}
	}
	m_lockThread = id;
}

void CThreadSafe::Release()
{
	if ( !m_lockThread )
		return;

	DWORD	id = CThread::Current();

	if ( m_lockThread & THREAD_LOCK_MASK )
	{
		CThread *thread = CThread::Thread();
		m_lockThread = 0;
		if ( thread )
			thread->CriticalEnd();
	}
	else if ( m_lockThread == id )
		m_lockThread = 0;
}

//	//	//	//	//	//	//	//
//
//		CThread
//
//	//	//	//	//	//	//	//

const char *CThread::m_sClassName = "CThread";
DWORD CThread::m_stThreads = 0;
CRITICAL_SECTION CThread::m_stSafeAccess;

CThread::CThread(const char *name)
{
	m_id = 0;
	m_hThread = 0;
	m_name = name ? name : "base thread";
	m_scriptContext = NULL;
	StaticInit();
	m_excPosAbsolute = 0;
	m_inCritical = false;
	m_hangCheck = 0;
	m_tmpStringIndex = 0;
	memset(m_antiDeadLock, 0, sizeof(m_antiDeadLock));
	LONGLONG llTicks;
	TIME_PROFILE_START;
	m_action = "";
}

CThread::~CThread()
{
	m_scriptContext = NULL;
	Terminate((DWORD)-1);
	StaticDeinit();
}

void CThread::Name(const char *name)
{
	m_name = name;
}

const char *CThread::Name()
{
	return m_name;
}

void CThread::StaticInit()
{
	if ( !m_stThreads )
	{
#ifdef _WIN32
		if ( CoInitializeEx(NULL, COINIT_MULTITHREADED) != S_OK )	// we need working OLE
			throw;
		InitializeCriticalSection(&m_stSafeAccess);
#else
		pthread_mutex_init(&m_stSafeAccess, NULL);
#endif
	}
	m_stThreads++;
}

void CThread::StaticDeinit()
{
	if ( !--m_stThreads )
	{
#ifdef _WIN32
		DeleteCriticalSection(&m_stSafeAccess);
		CoUninitialize();
#else
		pthread_mutex_destroy(&m_stSafeAccess);
#endif
	}
}

DWORD CThread::Current()
{
#ifdef _WIN32
	return ::GetCurrentThreadId();
#else
	return pthread_self();
#endif
}

char *CThread::TempString(int amount)
{
	static long maxIndex = THREAD_TEMP_LINES - 1;
	long nextIndex = m_tmpStringIndex + amount;

	if ( nextIndex >= maxIndex )	//	scroll back to the beginning if buffer is overlapping
	{
		nextIndex = amount;
	}
	m_tmpStringIndex = nextIndex;

	char *ret = m_tmpStrings[m_tmpStringIndex - amount];
	*ret = 0;
	return ret;
}

void CThread::ExcRegister(const char *classname, const char *funcname)
{
	if ( m_excPosAbsolute >= ( THREAD_EXC_STACK - 1 ) )
		return;

	m_excClassNames[m_excPosAbsolute] = classname;
	m_excFunctNames[m_excPosAbsolute] = funcname;
	m_excPosAbsolute++;
}

long CThread::ExcCount()
{
	return m_excPosAbsolute;
}

void CThread::ExcCount(long count)
{
	m_excPosAbsolute = count;
}

void CThread::ExcStack()
{
	g_Log.Debug("Exception in '%s' call stack:\n", m_name);
	for ( int i = m_excPosAbsolute-1; i >= 0; i-- )
	{
		g_Log.Debug(">>> [%3d] %15s::%s()\n",
			i,
			m_excClassNames[i] ? m_excClassNames[i] : "<NULL>",
			m_excFunctNames[i] ? m_excFunctNames[i] : "<NULL>"
			);
	}
}

__inline bool CThread::IsActive() const
{
	return ( m_hThread != 0 );
}

DWORD CThread::ID()
{
	return m_id;
}

void CThread::OnCreate()
{
	m_id = CThread::Current();
	ExceptionsThreadInit();
}

void CThread::Create(PTHREAD_ENTRY_PROC pEntryProc, void *pArgs)
{
	if ( IsActive() )
		return;

	m_proc = pEntryProc;
	if ( !pArgs )
		pArgs = this;

#ifdef _WIN32
	m_hThread = _beginthread(pEntryProc, 0, pArgs);
	if ( !m_hThread )
#else
	pthread_attr_init(&m_hThread_attr);
	if ( pthread_create(&m_hThread, &m_hThread_attr, pEntryProc, pArgs) )
#endif
		m_hThread = NULL;
}

void CThread::Terminate(DWORD dwExitCode)
{
	if ( !IsActive() )
		return;

	if ( m_id == Current() )
	{
#ifdef _WIN32
		_endthread();
#else
		pthread_attr_destroy(&m_hThread_attr);
		pthread_exit((void *)&dwExitCode);
#endif
	}
	else
	{
#ifdef _WIN32
		if ( TerminateThread((HANDLE)m_hThread, dwExitCode) )
			CloseHandle((HANDLE)m_hThread);
#else
		pthread_attr_destroy(&m_hThread_attr);
		pthread_kill(m_hThread, 1);
#endif
	}

	OnClose();
}

void CThread::WaitForClose()
{
	int count = 150; // 15 seconds

	while ( IsActive() && count-- )
		Sleep(100);

	Terminate(-1);
}

void CThread::OnClose()
{
	CriticalEnd();
	m_hThread = 0;
}

void CThread::CheckStuck()
{
	if ( !IsActive() )
		return;

	if ( !m_hangCheck )		//	initiate self-hang check
		m_hangCheck = 0xDEAD;
							//	thread did not reply from last test
							//	give one more chance to awake
	else if ( m_hangCheck == 0xDEAD )
	{
		m_hangCheck = 0xDEADDEADL;
		g_Log.Event(LOGL_CRIT, "'%s' thread seems being hang (frozen) at '%s'?\n", m_name, m_action);
	}
							//	seems the thread is really dead
	else if ( m_hangCheck == 0xDEADDEADL )
	{
#ifndef _DEBUG
		g_Log.Event(LOGL_CRIT, "'%s' thread hang, restarting...\n", m_name);

		Terminate(0xDEAD);
		Create(m_proc, this);
#else
		g_Log.Warn("Stop debugging and give some life to sphere, thread '%s' waits! :)\n", m_name);
#endif
	}
}

const CScript *CThread::ScriptContext(const CScript *context)
{
	if ( (long)context == -1 )
		return m_scriptContext;

	const CScript *prev = m_scriptContext;
	m_scriptContext = context;
	return prev;
}

CThread *CThread::Thread()
{
	DWORD dID = Current();

	vector<CThread *>::iterator	it;
	for ( it = g_Threads.begin(); it != g_Threads.end(); it++ )
	{
		CThread *thread = (*it);
		if ( thread->ID() == dID )
			return thread;
	}

	return NULL;
}

void CThread::CriticalStart()
{
	if ( m_inCritical )
		return;

#ifdef _WIN32
	EnterCriticalSection(&m_stSafeAccess);
#else
	pthread_mutex_lock(&m_stSafeAccess);
#endif
	m_inCritical = true;
}

void CThread::CriticalEnd()
{
	if ( !m_inCritical )
		return;

	m_inCritical = false;
#ifdef _WIN32
	LeaveCriticalSection(&m_stSafeAccess);
#else
	pthread_mutex_unlock(&m_stSafeAccess);
#endif
}

bool CThread::CriticalStartTry(int tryes)
{
	if ( m_inCritical )
		return true;

	while ( tryes-- )
	{
#ifdef _WIN32
		if ( TryEnterCriticalSection(&m_stSafeAccess) )
#else
		if ( pthread_mutex_trylock(&m_stSafeAccess) )
#endif
		{
			m_inCritical = true;
			return true;
		}

		Sleep(0);
	}
	return false;
}

void CThread::OnTick(int sleepTime)
{
	//	i am being checked for hanging status - report back i am ok
	if ( m_hangCheck )
		m_hangCheck = 0;

	//	it is not correct to live in critical section. in most cases it will mean that
	//	exitting from critical was not called when needed, so we quit this mode manualy
	//	all critical operations should be done in ONE TICK time
	if ( m_inCritical )
	{
		CriticalEnd();
	}

	//	always give some ticks back to system
	Sleep(max(sleepTime, 0));
}

__inline bool CThread::Critical()
{
	return m_inCritical;
}

long CThread::DL(E_DEADLOCK_TYPE tp)
{
	return m_antiDeadLock[tp];
}

void CThread::DL(E_DEADLOCK_TYPE tp, long value)
{
	m_antiDeadLock[tp] = value;
}
