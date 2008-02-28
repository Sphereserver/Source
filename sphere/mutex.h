#ifndef MUTEX_H
#define MUTEX_H

#include "../common/common.h"

#ifdef _BSD
	#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#endif

class SimpleMutex
{
public:
	SimpleMutex();
	~SimpleMutex();

	void lock();
	void unlock();

private:
#ifdef _WIN32
	CRITICAL_SECTION m_criticalSection;
#else
	pthread_mutex_t m_criticalSection;
	pthread_mutexattr_t m_criticalSectionAttr;
#endif
};

class SimpleThreadLock
{
public:
	SimpleThreadLock(SimpleMutex &mutex);
	~SimpleThreadLock();

	operator bool() const;

private:
	SimpleMutex &m_mutex;
	bool m_locked;
};

class ManualThreadLock
{
public:
	ManualThreadLock();
	ManualThreadLock(SimpleMutex * mutex);
	~ManualThreadLock();

	void setMutex(SimpleMutex * mutex);

	operator bool() const;
	void doLock();
	void doUnlock();

private:
	SimpleMutex * m_mutex;
	bool m_locked;
};

#endif
