// this thing is somehow required to be able to initialise OLE
#define _WIN32_DCOM

#include "../common/common.h"
#include "../common/CException.h"
#include "threads.h"

// number of exceptions after which we restart thread and think that the thread have gone in exceptioning loops
#define EXCEPTIONS_ALLOWED	10

/**
 * ThreadHolder
 * NOTE: due to it is difficult to create a good sync on this level, instead of storying a list of threads
 * we store an array and mark records being removed, which is absolutely thread-safe
**/
IThread *ThreadHolder::m_threads[MAX_THREADS];
int ThreadHolder::m_threadCount = 0;
bool ThreadHolder::m_inited = false;

IThread *ThreadHolder::current() {
	init();
	unsigned id = ::GetCurrentThreadId();

	for( int i = 0; i < MAX_THREADS; i++ ) {
		if( m_threads[i] != NULL ) {
			if( m_threads[i]->getId() == id ) {
				return m_threads[i];
			}
		}
	}

	return NULL;
}

void ThreadHolder::push(IThread *thread) {
	init();
	if( m_threadCount >= MAX_THREADS-1 ) {
		throw new CException(LOGL_FATAL, 0, "Too many opened threads");
	}

	for( int i = 0; i < MAX_THREADS; i++ ) {
		if( m_threads[i] == NULL ) {
			m_threads[i] = thread;
			m_threadCount++;
			return;
		}
	}

	throw new CException(LOGL_FATAL, 0, "Unable to find an empty slot for thread");
}

void ThreadHolder::pop(IThread *thread) {
	init();
	if( m_threadCount <= 0 ) {
		throw new CException(LOGL_ERROR, 0, "Trying to dequeue thread while no threads are active");
	}

	for( int i = 0; i < MAX_THREADS; i++ ) {
		if( m_threads[i] != NULL ) {
			if( m_threads[i]->getId() == thread->getId() ) {
				m_threads[i] = NULL;
				m_threadCount--;
				return;
			}
		}
	}

	throw new CException(LOGL_ERROR, 0, "Unable to dequeue a thread (not registered)");
}

int ThreadHolder::getActiveThreads() {
	return m_threadCount;
}

void ThreadHolder::init() {
	if( !m_inited ) {
		for( int i = 0; i < MAX_THREADS; i++ ) {
			m_threads[i] = NULL;
		}
		m_inited = true;
	}
}

/*
 * AbstractThread
*/
int AbstractThread::m_threadsAvailable = 0;

AbstractThread::AbstractThread(const char *name, IThread::Priority priority) {
	if( AbstractThread::m_threadsAvailable == 0 ) {
		// no threads were started before - initialise thread subsystem
		if( CoInitializeEx(NULL, COINIT_MULTITHREADED) != S_OK ) {
			throw new CException(LOGL_FATAL, 0, "OLE is not available, threading model unimplementable");
		}
		AbstractThread::m_threadsAvailable++;
	}
	m_id = 0;
	m_name = name;
	m_handle = NULL;
	m_hangCheck = 0;
	m_priority = priority;
}

AbstractThread::~AbstractThread() {
	terminate();
	AbstractThread::m_threadsAvailable--;
	if( AbstractThread::m_threadsAvailable == 0 ) {
		// all running threads have gone, the thread subsystem is no longer needed
		CoUninitialize();
	}
}

unsigned AbstractThread::getId() {
	return m_id;
}

const char *AbstractThread::getName() {
	return m_name;
}

void AbstractThread::start() {
	m_handle = (HANDLE)_beginthreadex(NULL, 0, &runner, this, 0, &m_id);
	push(this);
}

void AbstractThread::terminate() {
	if( isActive() ) {
		if( m_id == ::GetCurrentThreadId() ) {
			_endthreadex(0);
		}
		else {
			TerminateThread(m_handle, 0);
		}
		pop(this);
		m_id = 0;
		CloseHandle(m_handle);
		m_handle = NULL;
	}
}

void AbstractThread::run() {
	// is the very first since there is a possibility of something being altered there
	onStart();

	// detect a sleep period for thread depending on priority
	int tickPeriod;
	switch( m_priority ) {
		case IThread::Priority::Idle:
			tickPeriod = 1000;
			break;
		case IThread::Priority::Low:
			tickPeriod = 200;
			break;
		case IThread::Priority::Normal:
			tickPeriod = 100;
			break;
		case IThread::Priority::High:
			tickPeriod = 50;
			break;
		case IThread::Priority::Highest:
			tickPeriod = 25;
			break;
		case IThread::Priority::RealTime:
			tickPeriod = 0;
			break;
		default:
			throw new CException(LOGL_FATAL, 0, "Unable to determine thread priority");
	}

	int exceptions = 0;
	bool lastWasException = false;
	while( true ) {
		bool gotException = false;

		//	report me being alive if I am being checked for status
		if( m_hangCheck != 0 ) {
			m_hangCheck = 0;
		}

		try {
			tick();
		}
		catch( CException e ) {
			gotException = true;
			//	TODO: notify of exceptions
		}
		catch( ... ) {
			gotException = true;
			//	TODO: notify of exceptions
		}

		if( gotException ) {
			if( lastWasException ) {
				exceptions++;
			}
			else {
				lastWasException = true;
				exceptions = 0;
			}
			if( exceptions >= EXCEPTIONS_ALLOWED ) {
				// a bad thing really happened. ALL previous EXCEPTIONS_ALLOWED ticks resulted in exception
				// almost for sure we have looped somewhere and have no way to get out from this situation
				// probably a thread restart can fix the problems
				// but there is no real need to restart a thread, we will just simulate a thread restart,
				// notifying a subclass like we have been just restarted, so it will restart it's operations
				//	TODO: notify the logger
				onStart();
				lastWasException = false;
			}
		}
		else {
			lastWasException = false;
		}

		Sleep(tickPeriod);
	}
}

unsigned AbstractThread::runner(void *callerThread) {
	AbstractThread *caller = (AbstractThread*)callerThread;

	caller->run();

	_endthreadex(0);
	return 0;
}

bool AbstractThread::isActive() {
	return m_handle != NULL;
}

void AbstractThread::waitForClose() {
	int count = 150; // 15 seconds = 15000ms
	while( isActive() && count-- ) {
		Sleep(100);
	}
	terminate();
}

void AbstractThread::checkStuck() {
	if( isActive() ) {
		if( m_hangCheck == 0 ) {
			//	initiate hang check
			m_hangCheck = 0xDEAD;
		}
		else if( m_hangCheck == 0xDEAD ) {
			//	one time period was not answered, wait a bit more
			m_hangCheck = 0xDEADDEADl;
			//	TODO:
			//g_Log.Event(LOGL_CRIT, "'%s' thread seems being hang (frozen) at '%s'?\n", m_name, m_action);
		}
		else if( m_hangCheck == 0xDEADDEADl ) {
			//	TODO:
			//g_Log.Event(LOGL_CRIT, "'%s' thread hang, restarting...\n", m_name);
			terminate();
			start();
		}
	}
}

void AbstractThread::onStart() {
	//	empty. override if need in subclass
}
