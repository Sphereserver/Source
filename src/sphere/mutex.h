#ifndef _INC_MUTEX_H
#define _INC_MUTEX_H
#pragma once

#include "../common/os_common.h"

#ifdef _BSD
	#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#endif

class SimpleMutex
{
public:
	SimpleMutex();
	~SimpleMutex();

private:
	SimpleMutex(const SimpleMutex& copy);
	SimpleMutex& operator=(const SimpleMutex& other);

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

};

class SimpleThreadLock
{
public:
	explicit SimpleThreadLock(SimpleMutex &mutex);
	~SimpleThreadLock();

private:
	SimpleThreadLock(const SimpleThreadLock& copy);
	SimpleThreadLock& operator=(const SimpleThreadLock& other);

public:
	operator bool() const;

private:
	SimpleMutex &m_mutex;
	bool m_locked;
};

class ManualThreadLock
{
public:
	ManualThreadLock();
	explicit ManualThreadLock(SimpleMutex * mutex);
	~ManualThreadLock();

private:
	ManualThreadLock(const ManualThreadLock& copy);
	ManualThreadLock& operator=(const ManualThreadLock& other);

public:
	void setMutex(SimpleMutex * mutex);

	operator bool() const;
	void doLock();
	bool doTryLock();
	void doUnlock();

private:
	SimpleMutex * m_mutex;
	bool m_locked;
};

class AutoResetEvent
{
public:
#ifdef _WIN32
	static const unsigned long _infinite = INFINITE;
#else
	static const unsigned long _infinite = 0xffffffff;
#endif

public:
	AutoResetEvent();
	~AutoResetEvent();

private:
	AutoResetEvent(const AutoResetEvent& copy);
	AutoResetEvent& operator=(const AutoResetEvent& other);

public:
	void wait(unsigned long timeout = _infinite);
	void signal();

private:
#ifdef _WIN32
	HANDLE m_handle;
#else
	pthread_mutex_t m_criticalSection;
	pthread_mutexattr_t m_criticalSectionAttr;
	pthread_condattr_t m_conditionAttr;
	pthread_cond_t m_condition;
#endif
};

class ManualResetEvent
{
public:
#ifdef _WIN32
	static const unsigned long _infinite = INFINITE;
#else
	static const unsigned long _infinite = 0xffffffff;
#endif

public:
	ManualResetEvent();
	~ManualResetEvent();

private:
	ManualResetEvent(const ManualResetEvent& copy);
	ManualResetEvent& operator=(const ManualResetEvent& other);

public:
	void wait(unsigned long timeout = _infinite);
	void reset();
	void set();

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
};

#endif	// _INC_MUTEX_H
