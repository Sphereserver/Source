#ifndef _INC_MUTEX_H
#define _INC_MUTEX_H
#pragma once

#include "../common/os_common.h"

class SimpleMutex
{
public:
	SimpleMutex();
	~SimpleMutex();

public:
	void lock();
	bool tryLock();
	void unlock();

private:
#ifdef _WIN32
	CRITICAL_SECTION m_criticalSection;
#else
	pthread_mutex_t m_criticalSection;
	pthread_mutexattr_t m_criticalSectionAttr;
#endif

private:
	SimpleMutex(const SimpleMutex &copy);
	SimpleMutex &operator=(const SimpleMutex &other);
};

class SimpleThreadLock
{
public:
	explicit SimpleThreadLock(SimpleMutex &mutex);
	~SimpleThreadLock();

private:
	SimpleMutex &m_mutex;
	bool m_locked;

public:
	operator bool() const
	{
		// Report the state of locking when used as a boolean
		return m_locked;
	}

private:
	SimpleThreadLock(const SimpleThreadLock &copy);
	SimpleThreadLock &operator=(const SimpleThreadLock &other);
};

class ManualThreadLock
{
public:
	ManualThreadLock();
	explicit ManualThreadLock(SimpleMutex *mutex);
	~ManualThreadLock();

private:
	SimpleMutex *m_mutex;
	bool m_locked;

public:
	void setMutex(SimpleMutex *mutex);
	void doLock();
	bool doTryLock();
	void doUnlock();

	operator bool() const
	{
		// Report the state of locking when used as a boolean
		return m_locked;
	}

private:
	ManualThreadLock(const ManualThreadLock &copy);
	ManualThreadLock &operator=(const ManualThreadLock &other);
};

class AutoResetEvent
{
public:
	AutoResetEvent();
	~AutoResetEvent();

private:
#ifdef _WIN32
	HANDLE m_handle;
#else
	pthread_mutex_t m_criticalSection;
	pthread_mutexattr_t m_criticalSectionAttr;
	pthread_condattr_t m_conditionAttr;
	pthread_cond_t m_condition;
#endif

public:
#ifdef _WIN32
	static const unsigned long _infinite = INFINITE;
#else
	static const unsigned long _infinite = 0xFFFFFFFF;
#endif


public:
	void wait(unsigned long timeout = _infinite);
	void signal();

private:
	AutoResetEvent(const AutoResetEvent &copy);
	AutoResetEvent &operator=(const AutoResetEvent &other);
};

class ManualResetEvent
{
public:
	ManualResetEvent();
	~ManualResetEvent();

private:
#ifdef _WIN32
	HANDLE m_handle;
#else
	bool m_value;
	pthread_mutex_t m_criticalSection;
	pthread_mutexattr_t m_criticalSectionAttr;
	pthread_condattr_t m_conditionAttr;
	pthread_cond_t m_condition;
#endif

public:
#ifdef _WIN32
	static const unsigned long _infinite = INFINITE;
#else
	static const unsigned long _infinite = 0xFFFFFFFF;
#endif

public:
	void wait(unsigned long timeout = _infinite);
	void set();
	void reset();

private:
	ManualResetEvent(const ManualResetEvent &copy);
	ManualResetEvent &operator=(const ManualResetEvent &other);
};

#endif	// _INC_MUTEX_H
