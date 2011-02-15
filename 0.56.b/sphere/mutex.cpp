#define _WIN32_DCOM
#include "mutex.h"

// *************************
//		SimpleMutex
// *************************

SimpleMutex::SimpleMutex()
{
#ifdef _WIN32
	InitializeCriticalSectionAndSpinCount(&m_criticalSection, 0x80000020);
#else
	pthread_mutexattr_init(&m_criticalSectionAttr);
	pthread_mutexattr_settype(&m_criticalSectionAttr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&m_criticalSection, &m_criticalSectionAttr);
#endif
}

SimpleMutex::~SimpleMutex()
{
#ifdef _WIN32
	DeleteCriticalSection(&m_criticalSection);
#else
	pthread_mutexattr_destroy(&m_criticalSectionAttr);
	pthread_mutex_destroy(&m_criticalSection);
#endif
}

//lock
void SimpleMutex::lock()
{
#ifdef _WIN32
	EnterCriticalSection(&m_criticalSection);
#else
	pthread_mutex_lock(&m_criticalSection);
#endif
}

//try lock
bool SimpleMutex::tryLock()
{
#ifdef _WIN32
	return TryEnterCriticalSection(&m_criticalSection) == TRUE;
#else
	return pthread_mutex_trylock(&m_criticalSection) == 0;
#endif
}

//unlock
void SimpleMutex::unlock()
{
#ifdef _WIN32
	LeaveCriticalSection(&m_criticalSection);
#else
	pthread_mutex_unlock(&m_criticalSection);
#endif
}

// ****************************
//		SimpleThreadLock
// ****************************

SimpleThreadLock::SimpleThreadLock(SimpleMutex &mutex) : m_mutex(mutex), m_locked(true)
{
	mutex.lock();
}

//the destructor
SimpleThreadLock::~SimpleThreadLock()
{
	m_mutex.unlock();
}

//report the state of locking when used as a boolean
SimpleThreadLock::operator bool() const
{
	return m_locked;
}

// ****************************
//		ManualThreadLock
// ****************************

ManualThreadLock::ManualThreadLock() : m_mutex(NULL), m_locked(false)
{
}

ManualThreadLock::ManualThreadLock(SimpleMutex * mutex) : m_locked(false)
{
	setMutex(mutex);
}

//the destructor
ManualThreadLock::~ManualThreadLock()
{
	if (m_mutex != NULL)
		doUnlock();
}

void ManualThreadLock::setMutex(SimpleMutex * mutex)
{
	m_mutex = mutex;
}

//report the state of locking when used as a boolean
ManualThreadLock::operator bool() const
{
	return m_locked;
}

void ManualThreadLock::doLock()
{
	m_mutex->lock();
	m_locked = true;
}

bool ManualThreadLock::doTryLock()
{
	if (m_mutex->tryLock() == false)
		return false;

	m_locked = true;
	return true;
}

void ManualThreadLock::doUnlock()
{
	m_locked = false;
	m_mutex->unlock();
}

// ****************************
//		AutoResetEvent
// ****************************

AutoResetEvent::AutoResetEvent()
{
#ifdef _WIN32
	m_handle = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
	pthread_mutexattr_init(&m_criticalSectionAttr);
	pthread_mutexattr_settype(&m_criticalSectionAttr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&m_criticalSection, &m_criticalSectionAttr);

	pthread_condattr_init(&m_conditionAttr);
	pthread_cond_init(&m_condition, &m_conditionAttr);
#endif
}

AutoResetEvent::~AutoResetEvent()
{
#ifdef _WIN32
	CloseHandle(m_handle);
#else
	pthread_condattr_destroy(&m_conditionAttr);
	pthread_cond_destroy(&m_condition);
	pthread_mutexattr_destroy(&m_criticalSectionAttr);
	pthread_mutex_destroy(&m_criticalSection);
#endif
}

void AutoResetEvent::wait(unsigned long timeout)
{
	if (timeout == 0)
	{
		// if timeout is 0 then the thread's timeslice may not be given up as with normal
		// sleep methods - so we will check for this condition ourselves and use SleepEx
		// instead
		SleepEx(0, TRUE);
		return;
	}

#ifdef _WIN32
	// it's possible for WaitForSingleObjectEx to exit before the timeout and without
	// the signal. due to bAlertable=TRUE other events (e.g. async i/o) can cancel the
	// waiting period early.
	WaitForSingleObjectEx(m_handle, timeout, TRUE);
#else
	pthread_mutex_lock(&m_criticalSection);

	// it's possible for pthread_cond_wait/timedwait to exit before the timeout and
	// without a signal, but there's little we can actually do to check it since we
	// don't usually care about the condition - if the calling thread does care then
	// it needs to implement it's own checks
	if (timeout == _infinite)
	{
		pthread_cond_wait(&m_condition, &m_criticalSection);
	}
	else
	{
		// pthread_cond_timedwait expects the timeout to be the actual time
		timespec time;
		clock_gettime(CLOCK_REALTIME, &time);
		time.tv_sec += timeout / 1000;
		time.tv_nsec += (timeout % 1000) * 1000000L;

		pthread_cond_timedwait(&m_condition, &m_criticalSection, &time);
	}

	pthread_mutex_unlock(&m_criticalSection);
#endif
}

void AutoResetEvent::signal()
{
#ifdef _WIN32
	SetEvent(m_handle);
#else
	pthread_mutex_lock(&m_criticalSection);
	pthread_cond_signal(&m_condition);
	pthread_mutex_unlock(&m_criticalSection);
#endif
}
