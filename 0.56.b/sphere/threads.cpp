// this thing is somehow required to be able to initialise OLE
#define _WIN32_DCOM

#include "../common/common.h"
#include "../common/CException.h"
#include "../graysvr/graysvr.h"
#include "threads.h"
#include "mutex.h"

// number of exceptions after which we restart thread and think that the thread have gone in exceptioning loops
#define EXCEPTIONS_ALLOWED	10

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
 * NOTE: due to it is difficult to create a good sync on this level, instead of storying a list of threads
 * we store an array and mark records being removed, which is absolutely thread-safe
**/
IThread *ThreadHolder::m_threads[MAX_THREADS];
int ThreadHolder::m_threadCount = 0;
bool ThreadHolder::m_inited = false;

extern CLog g_Log;

IThread *ThreadHolder::current()
{
	init();
#ifdef _WIN32
	unsigned id = ::GetCurrentThreadId();
#else
	unsigned id = (unsigned)pthread_self();
#endif


	for( int i = 0; i < MAX_THREADS; i++ )
	{
		if( m_threads[i] != NULL )
		{
			if( m_threads[i]->getId() == id )
			{
				return m_threads[i];
			}
		}
	}

	return DummySphereThread::getInstance();
}

void ThreadHolder::push(IThread *thread)
{
	init();
	if( m_threadCount >= MAX_THREADS-1 )
	{
		throw CException(LOGL_FATAL, 0, "Too many opened threads");
	}

	for( int i = 0; i < MAX_THREADS; i++ )
	{
		if( m_threads[i] == NULL )
		{
			m_threads[i] = thread;
			m_threadCount++;
			return;
		}
	}

	throw CException(LOGL_FATAL, 0, "Unable to find an empty slot for thread");
}

void ThreadHolder::pop(IThread *thread)
{
	init();
	if( m_threadCount <= 0 )
	{
		throw CException(LOGL_ERROR, 0, "Trying to dequeue thread while no threads are active");
	}

	for( int i = 0; i < MAX_THREADS; i++ )
	{
		if( m_threads[i] != NULL )
		{
			if( m_threads[i]->getId() == thread->getId() )
			{
				m_threads[i] = NULL;
				m_threadCount--;
				return;
			}
		}
	}

	throw CException(LOGL_ERROR, 0, "Unable to dequeue a thread (not registered)");
}

IThread * ThreadHolder::getThreadAt(int at)
{
	if (( at < 0 ) || ( at > getActiveThreads() ))
		return NULL;

	return m_threads[at];
}

void ThreadHolder::init()
{
	if( !m_inited )
	{
		for( int i = 0; i < MAX_THREADS; i++ )
		{
			m_threads[i] = NULL;
		}

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
	m_handle = NULL;
	m_hangCheck = 0;
	setPriority(priority);
}

AbstractThread::~AbstractThread()
{
	terminate();
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
	m_handle = (spherethread_t) _beginthreadex(NULL, 0, &runner, this, 0, &m_id);
#else
	if ( pthread_create( &m_handle, NULL, &runner, this ) )
	{
		m_handle = (spherethread_t) NULL;
		throw CException(LOGL_FATAL, 0, "Unable to spawn a new thread");
	}
	else
		m_id = (unsigned) m_handle; //pthread_self() and m_handle should be the same
#endif
	ThreadHolder::push(this);
}

void AbstractThread::terminate()
{
	if( isActive() )
	{
#ifdef _WIN32
		if ( isCurrentThread() )
		{
			_endthreadex(0);
		}
		else
		{
			TerminateThread(m_handle, 0);
		}
		CloseHandle(m_handle);
#else
		pthread_detach(m_handle); // required for thread memory to be freed after exit

		if ( isCurrentThread() )
		{
			pthread_exit(0);
		}
		else
		{
			pthread_cancel(m_handle); // IBM say it so
		}
#endif
		// Common things
		ThreadHolder::pop(this);
		m_id = 0;
		m_handle = NULL;
	}
}

void AbstractThread::run()
{
	// is the very first since there is a possibility of something being altered there
	onStart();

	int exceptions = 0;
	bool lastWasException = false;
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
		}
		catch( ... )
		{
			gotException = true;
			g_Log.CatchEvent(NULL, "%s::tick", getName());
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
		{
			terminate();
			return;
		}

		m_sleepEvent.wait(m_tickPeriod);
	}
}

SPHERE_THREADENTRY_RETNTYPE AbstractThread::runner(void *callerThread)
{
	AbstractThread *caller = (AbstractThread*)callerThread;

	caller->run();

#ifdef _WIN32
	_endthreadex(0);
#else
	pthread_exit(0);
#endif
	return 0;
}

bool AbstractThread::isActive() const
{
#ifdef _WIN32
	return m_handle != NULL;
#else
	return m_handle != 0;
#endif
}

void AbstractThread::waitForClose()
{
	terminate();
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
			terminate();
			start();
			return true;
		}
	}

	return false;
}

void AbstractThread::onStart()
{
	//	empty. override if need in subclass
}

void AbstractThread::setPriority(IThread::Priority pri)
{
	m_priority = pri;

	// detect a sleep period for thread depending on priority
	switch( m_priority )
	{
		case IThread::Idle:
			m_tickPeriod = 1000;
			break;
		case IThread::Low:
			m_tickPeriod = 200;
			break;
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
		default:
			throw CException(LOGL_FATAL, 0, "Unable to determine thread priority");
	}
}

bool AbstractThread::shouldExit()
{
	return false;
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
#endif
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
			DEBUG_WARN(( "Thread temporary string buffer is full.\n" ));
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
	if( g_Serv.m_iModeCode == SERVMODE_Exiting )
	{
		return true;
	}
	return false;
}

#ifdef THREAD_TRACK_CALLSTACK
void AbstractSphereThread::printStackTrace()
{
	LONGLONG startTime = m_stackInfo[0].startTime;
	long timedelta;
	unsigned int threadId = getId();

	g_Log.EventDebug("__ thread (%u) __ |  # | _____ function _____________ | ticks passed from previous function start ______\n", threadId);
	for( int i = 0; i < 0x1000; i++ )
	{
		if( m_stackInfo[i].startTime == 0 )
			break;

		timedelta = (long)(m_stackInfo[i].startTime - startTime);
		g_Log.EventDebug(">>         %u     | %2d | %28s | +%ld %s\n",
			threadId, i, m_stackInfo[i].functionName, timedelta,
				( i == m_stackPos-1 ) ?
				"<-- exception catch point (below is guessed and could be incorrect!)" :
				"");
		startTime = m_stackInfo[i].startTime;
	}
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

#ifdef THREAD_TRACK_CALLSTACK
/*
 * StackDebugInformation
*/
StackDebugInformation::StackDebugInformation(const char *name)
{
	m_context = (AbstractSphereThread *)ThreadHolder::current();
	if (m_context != NULL)
		m_context->pushStackCall(name);
}

StackDebugInformation::~StackDebugInformation()
{
	if (m_context != NULL)
		m_context->popStackCall();
}

void StackDebugInformation::printStackTrace()
{
	((AbstractSphereThread *)ThreadHolder::current())->printStackTrace();
}

#endif
