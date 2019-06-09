// this thing is somehow required to be able to initialise OLE
#define _WIN32_DCOM

#include "../common/graycom.h"
#include "../graysvr/graysvr.h"
#include "threads.h"
#if !defined(_WIN32) && !defined(_BSD)
#include <sys/prctl.h>
#endif
#include <algorithm>

// number of exceptions after which we restart thread and think that the thread have gone in exceptioning loops
#define EXCEPTIONS_ALLOWED	10

// number of milliseconds to wait for a thread to close
#define THREADJOIN_TIMEOUT	60000

// Normal Buffer
SimpleMutex g_tmpStringMutex;
volatile long g_tmpStringIndex = 0;
char g_tmpStrings[THREAD_TSTRING_STORAGE][THREAD_STRING_LENGTH];

// TemporaryString Buffer
SimpleMutex g_tmpTemporaryStringMutex;
volatile long g_tmpTemporaryStringIndex = 0;

struct TemporaryStringStorage
{
	char m_buffer[THREAD_STRING_LENGTH];
	char m_state;
} g_tmpTemporaryStringStorage[THREAD_STRING_STORAGE];

/**
 * ThreadHolder
**/
spherethreadlist_t ThreadHolder::m_threads;
size_t ThreadHolder::m_threadCount = 0;
bool ThreadHolder::m_inited = false;
SimpleMutex ThreadHolder::m_mutex;
TlsValue<IThread *> ThreadHolder::m_currentThread;

extern CLog g_Log;

IThread *ThreadHolder::current()
{
	init();

	IThread * thread = m_currentThread;
	if (thread == NULL)
		return DummySphereThread::getInstance();

#ifdef _WIN32
	ASSERT(thread->getId() == ::GetCurrentThreadId());
#else
	ASSERT(thread->getId() == (unsigned)pthread_self());
#endif
	return thread;
}

void ThreadHolder::push(IThread *thread)
{
	init();

	SimpleThreadLock lock(m_mutex);
	m_threads.push_back(thread);
	m_threadCount++;
}

void ThreadHolder::pop(IThread *thread)
{
	init();
	if( m_threadCount <= 0 )
		throw CException(LOGL_ERROR, 0, "Trying to dequeue thread while no threads are active");

	SimpleThreadLock lock(m_mutex);
	spherethreadlist_t::iterator it = std::find(m_threads.begin(), m_threads.end(), thread);
	if (it != m_threads.end())
	{
		m_threadCount--;
		m_threads.erase(it);
		return;
	}

	throw CException(LOGL_ERROR, 0, "Unable to dequeue a thread (not registered)");
}

IThread * ThreadHolder::getThreadAt(size_t at)
{
	if ( at > getActiveThreads() )
		return NULL;
	
	SimpleThreadLock lock(m_mutex);
	for ( spherethreadlist_t::const_iterator it = m_threads.begin(); it != m_threads.end(); ++it )
	{
		if ( at == 0 )
			return *it;

		at--;
	}

	return NULL;
}

void ThreadHolder::init()
{
	if( !m_inited )
	{
		memset(g_tmpStrings, 0, sizeof(g_tmpStrings));
		memset(g_tmpTemporaryStringStorage, 0, sizeof(g_tmpTemporaryStringStorage));

		m_inited = true;
	}
}

/*
 * AbstractThread
*/
int AbstractThread::m_threadsAvailable = 0;

AbstractThread::AbstractThread(const char *name, IThread::Priority priority)
{
	if( AbstractThread::m_threadsAvailable == 0 )
	{
		// no threads were started before - initialise thread subsystem
#ifdef _WIN32
		if( CoInitializeEx(NULL, COINIT_MULTITHREADED) != S_OK )
		{
			throw CException(LOGL_FATAL, 0, "OLE is not available, threading model unimplementable");
		}
#endif
		AbstractThread::m_threadsAvailable++;
	}
	m_id = 0;
	m_name = name;
	m_handle = 0;
	m_hangCheck = 0;
	m_terminateRequested = true;
	setPriority(priority);
}

AbstractThread::~AbstractThread()
{
	terminate(false);
	AbstractThread::m_threadsAvailable--;
	if( AbstractThread::m_threadsAvailable == 0 )
	{
		// all running threads have gone, the thread subsystem is no longer needed
#ifdef _WIN32
		CoUninitialize();
#else
		// No pthread equivalent
#endif
	}
}

void AbstractThread::start()
{
#ifdef _WIN32
	m_handle = reinterpret_cast<spherethread_t>(_beginthreadex(NULL, 0, &runner, this, 0, NULL));
#else
	pthread_attr_t threadAttr;
	pthread_attr_init(&threadAttr);
	pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
	int result = pthread_create( &m_handle, &threadAttr, &runner, this );
	pthread_attr_destroy(&threadAttr);

	if (result != 0)
	{
		m_handle = 0;
		throw CException(LOGL_FATAL, 0, "Unable to spawn a new thread");
	}
#endif
	
	m_terminateEvent.reset();
	ThreadHolder::push(this);
}

void AbstractThread::terminate(bool ended)
{
	if( isActive() )
	{
		bool wasCurrentThread = isCurrentThread();
		if (ended == false)
		{
			g_Log.Event(LOGL_WARN, "Forcing thread '%s' to terminate...\n", getName());

			// if the thread is current then terminating here will prevent cleanup from occurring
			if (wasCurrentThread == false)
			{
#ifdef _WIN32
				ExitThread(0);
				CloseHandle(m_handle);
#else
				pthread_cancel(m_handle); // IBM say it so
#endif
			}
		}

		// Common things
		ThreadHolder::pop(this);
		m_id = 0;
		m_handle = 0;

		// let everyone know we have been terminated
		m_terminateEvent.set();

		// current thread can be terminated now
		if (ended == false && wasCurrentThread)
		{
#ifdef _WIN32
			_endthreadex(0);
#else
			pthread_exit(0);
#endif
		}
	}
}

void AbstractThread::run()
{
	// is the very first since there is a possibility of something being altered there
	onStart();

	int exceptions = 0;
	bool lastWasException = false;
	m_terminateRequested = false;

	for (;;)
	{
		bool gotException = false;

		//	report me being alive if I am being checked for status
		if( m_hangCheck != 0 )
		{
			m_hangCheck = 0;
		}

		try
		{
			tick();

			// ensure this is recorded as 'idle' time (ideally this should
			// be in tick() but we cannot guarantee it to be called there
			CurrentProfileData.Start(PROFILE_IDLE);
		}
		catch( const CException& e )
		{
			gotException = true;
			g_Log.CatchEvent(&e, "%s::tick", getName());
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
		catch( ... )
		{
			gotException = true;
			g_Log.CatchEvent(NULL, "%s::tick", getName());
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}

		if( gotException )
		{
			if( lastWasException )
			{
				exceptions++;
			}
			else
			{
				lastWasException = true;
				exceptions = 0;
			}
			if( exceptions >= EXCEPTIONS_ALLOWED )
			{
				// a bad thing really happened. ALL previous EXCEPTIONS_ALLOWED ticks resulted in exception
				// almost for sure we have looped somewhere and have no way to get out from this situation
				// probably a thread restart can fix the problems
				// but there is no real need to restart a thread, we will just simulate a thread restart,
				// notifying a subclass like we have been just restarted, so it will restart it's operations
				g_Log.Event(LOGL_CRIT, "'%s' thread raised too many exceptions, restarting...\n", getName());
				onStart();
				lastWasException = false;
			}
		}
		else
		{
			lastWasException = false;
		}

		if( shouldExit() )
			break;

		m_sleepEvent.wait(m_tickPeriod);
	}
}

SPHERE_THREADENTRY_RETNTYPE AbstractThread::runner(void *callerThread)
{
	AbstractThread * caller = reinterpret_cast<AbstractThread*>(callerThread);
	if (caller != NULL)
	{
		caller->run();
		caller->terminate(true);
	}

	return 0;
}

bool AbstractThread::isActive() const
{
	return m_handle != 0;
}

void AbstractThread::waitForClose()
{
	if (isActive())
	{
		if (isCurrentThread() == false)
		{
			// flag that we want the thread to terminate
			m_terminateRequested = true;
			awaken();

			// give the thread a chance to close on its own, and then
			// terminate anyway
			m_terminateEvent.wait(THREADJOIN_TIMEOUT);
		}

		terminate(false);
	}
}

void AbstractThread::awaken()
{
	m_sleepEvent.signal();
}

bool AbstractThread::isCurrentThread() const
{
#ifdef _WIN32
	return (getId() == ::GetCurrentThreadId());
#else
	return pthread_equal(m_handle,pthread_self());
#endif
}

bool AbstractThread::checkStuck()
{
	if( isActive() )
	{
		if( m_hangCheck == 0 )
		{
			//	initiate hang check
			m_hangCheck = 0xDEAD;
		}
		else if( m_hangCheck == 0xDEAD )
		{
			//	one time period was not answered, wait a bit more
			m_hangCheck = 0xDEADDEADl;
			//	TODO:
			//g_Log.Event(LOGL_CRIT, "'%s' thread seems being hang (frozen) at '%s'?\n", m_name, m_action);
		}
		else if( m_hangCheck == 0xDEADDEADl )
		{
			//	TODO:
			//g_Log.Event(LOGL_CRIT, "'%s' thread hang, restarting...\n", m_name);
			#ifdef THREAD_TRACK_CALLSTACK
				static_cast<AbstractSphereThread*>(this)->printStackTrace();
			#endif
			terminate(false);
			run();
			start();
			return true;
		}
	}

	return false;
}

#ifdef _WIN32
#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType;
	LPCTSTR szName;
	DWORD dwThreadID;
	DWORD dwFlags;
} THREADNAME_INFO;
#pragma pack(pop)

const DWORD MS_VC_EXCEPTION = 0x406D1388;
#endif

void AbstractThread::onStart()
{
	// start-up actions for each thread
	// when implemented in derived classes this method must always be called too, preferably before
	// the custom implementation

	// we set the id here to ensure it is available before the first tick, otherwise there's
	// a small delay when setting it from AbstractThread::start and it's possible for the id
	// to not be set fast enough (particular when using pthreads)
#ifdef _WIN32
	m_id = ::GetCurrentThreadId();
#else
	m_id = pthread_self();
#endif
	ThreadHolder::m_currentThread = this;

	// register the thread name
#ifdef _WIN32
	// Windows uses THREADNAME_INFO structure to set thread name
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = getName();
	info.dwThreadID = static_cast<DWORD>(-1);
	info.dwFlags = 0;

	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), reinterpret_cast<ULONG_PTR*>(&info));
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
#elif !defined(_BSD)
	// Unix uses prctl to set thread name
	// thread name must be 16 bytes, zero-padded if shorter
	char name[16] = { '\0' };
	strcpylen(name, m_name, COUNTOF(name));
	prctl(PR_SET_NAME, name, 0, 0, 0);
#endif
}

void AbstractThread::setPriority(IThread::Priority pri)
{
	// detect a sleep period for thread depending on priority
	switch ( pri )
	{
		case IThread::Idle:
			m_tickPeriod = 1000;
			break;
		case IThread::Low:
			m_tickPeriod = 200;
			break;
		default:
			pri = IThread::Normal;
			// fall through
		case IThread::Normal:
			m_tickPeriod = 100;
			break;
		case IThread::High:
			m_tickPeriod = 50;
			break;
		case IThread::Highest:
			m_tickPeriod = 5;
			break;
		case IThread::RealTime:
			m_tickPeriod = 0;
			break;
		case IThread::Disabled:
			m_tickPeriod = AutoResetEvent::_infinite;
			break;
	}
	m_priority = pri;
}

bool AbstractThread::shouldExit()
{
	return m_terminateRequested;
}

/*
 * AbstractSphereThread
*/
AbstractSphereThread::AbstractSphereThread(const char *name, Priority priority)
	: AbstractThread(name, priority)
{
#ifdef THREAD_TRACK_CALLSTACK
	m_stackPos = 0;
	memset(m_stackInfo, 0, sizeof(m_stackInfo));
	m_freezeCallStack = false;
#endif

	// profiles that apply to every thread
	m_profile.EnableProfile(PROFILE_IDLE);
	m_profile.EnableProfile(PROFILE_OVERHEAD);
	m_profile.EnableProfile(PROFILE_STAT_FAULTS);
}

// IMHO we need a lock on allocateBuffer and allocateStringBuffer

char *AbstractSphereThread::allocateBuffer()
{
	SimpleThreadLock stlBuffer(g_tmpStringMutex);

	char * buffer = NULL; 
	g_tmpStringIndex++;

	if( g_tmpStringIndex >= THREAD_TSTRING_STORAGE )
	{
		g_tmpStringIndex %= THREAD_TSTRING_STORAGE;
	}

	buffer = g_tmpStrings[g_tmpStringIndex];
	*buffer = '\0';

	return buffer;
}

TemporaryStringStorage *AbstractSphereThread::allocateStringBuffer()
{
	long initialPosition = g_tmpTemporaryStringIndex;
	long index;
	for (;;)
	{
		index = ++g_tmpTemporaryStringIndex;
		if( g_tmpTemporaryStringIndex >= THREAD_STRING_STORAGE )
		{
			index = g_tmpTemporaryStringIndex %= THREAD_STRING_STORAGE;
		}

		if( g_tmpTemporaryStringStorage[index].m_state == 0 )
		{
			TemporaryStringStorage * store = &g_tmpTemporaryStringStorage[index];
			*store->m_buffer = '\0';
			return store;
		}

		// a protection against deadlock. All string buffers are marked as being used somewhere, so we
		// have few possibilities (the case shows that we have a bug and temporary strings used not such):
		// a) return NULL and wait for exceptions in the program
		// b) allocate a string from a heap
		if( initialPosition == index )
		{
			// but the best is to throw an exception to give better formed information for end users
			// rather than access violations
			throw CException(LOGL_FATAL, 0, "Thread temporary string buffer is full");
		}
	}
}

void AbstractSphereThread::allocateString(TemporaryString &string)
{
	SimpleThreadLock stlBuffer(g_tmpTemporaryStringMutex);

	TemporaryStringStorage * store = allocateStringBuffer();
	string.init(store->m_buffer, &store->m_state);
}

bool AbstractSphereThread::shouldExit()
{
	if ( g_Serv.m_iModeCode == SERVMODE_Exiting )
		return true;

	return AbstractThread::shouldExit();
}

#ifdef THREAD_TRACK_CALLSTACK
void AbstractSphereThread::pushStackCall(const char *name)
{
	if ( !m_freezeCallStack )
	{
		m_stackInfo[m_stackPos].functionName = name;
		m_stackInfo[m_stackPos].startTime = GetTickCount64();
		m_stackPos++;
		m_stackInfo[m_stackPos].startTime = 0;
	}
}

void AbstractSphereThread::printStackTrace()
{
	// don't allow call stack to be modified whilst we're printing it
	freezeCallStack(true);

	ULONGLONG startTime = m_stackInfo[0].startTime;
	ULONGLONG timedelta;
	unsigned int threadId = getId();

	g_Log.EventDebug("__ thread (%u) __ |  # | _____________ function _____________ | __ ticks passed from previous function start __\n", threadId);
	for ( size_t i = 0; i < 0x1000; i++ )
	{
		if ( m_stackInfo[i].startTime == 0 )
			break;

		timedelta = m_stackInfo[i].startTime - startTime;
		g_Log.EventDebug(">>         %u     | %2d | %36s | +%llu %s\n", threadId, i, m_stackInfo[i].functionName, timedelta, (i == (m_stackPos - 1)) ? "<-- exception catch point (below is guessed and could be incorrect!)" : "");
		startTime = m_stackInfo[i].startTime;
	}

	freezeCallStack(false);
}
#endif

/*
 * DummySphereThread
*/
DummySphereThread *DummySphereThread::instance = NULL;

DummySphereThread::DummySphereThread()
	: AbstractSphereThread("dummy", IThread::Normal)
{
}

DummySphereThread *DummySphereThread::getInstance()
{
	if( instance == NULL )
	{
		instance = new DummySphereThread();
	}
	return instance;
}

void DummySphereThread::tick()
{
}
