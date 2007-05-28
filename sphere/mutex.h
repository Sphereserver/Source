#ifndef MUTEX_H
#define MUTEX_H

#include "../common/common.h"

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
	void setUnlock();

private:
	SimpleMutex &m_mutex;
	bool m_locked;
};

#endif