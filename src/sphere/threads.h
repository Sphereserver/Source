#ifndef THREADS_H
#define THREADS_H

// temporary string storage
#define THREAD_STRING_STORAGE	4096
#define THREAD_TSTRING_STORAGE	2048
#define THREAD_STRING_LENGTH	4096

#include "../sphere/mutex.h"
#include "../sphere/strings.h"
#include "../sphere/ProfileData.h"
#include "../common/graycom.h"
#include <list>
#ifndef _WIN32
	#include <pthread.h>
#endif


/**
 * Sphere threading system
 * Threads should be inherited from AbstractThread with overridden tick() method
 * Also useful to override onStart() in order to initialise class data variables for ticking
 *   which is triggered whenever the thread is starting/restarting
**/

// Types definition for different platforms
#ifdef _WIN32
	typedef HANDLE spherethread_t;
	#define SPHERE_THREADENTRY_RETNTYPE unsigned
	#define SPHERE_THREADENTRY_CALLTYPE __stdcall
#else
	typedef pthread_t spherethread_t;
	#define SPHERE_THREADENTRY_RETNTYPE void *
	#define SPHERE_THREADENTRY_CALLTYPE
#endif

// Interface for threads. Almost always should be used instead of any implementing classes
class IThread
{
public:
	enum Priority
	{
		Idle,			// tick 1000ms
		Low,			// tick 200ms
		Normal,			// tick 100ms
		High,			// tick 50ms
		Highest,		// tick 5ms
		RealTime,		// tick almost instantly
		Disabled = 0xFF	// tick never
	};

#ifdef _WIN32
	virtual DWORD getId() const = 0;
#else
	virtual pthread_t getId() const = 0;
#endif
	virtual const char *getName() const = 0;

	virtual bool isActive() const = 0;
	virtual bool checkStuck() = 0;

	virtual void start() = 0;
	virtual void terminate(bool ended) = 0;
	virtual void waitForClose() = 0;

	virtual void setPriority(Priority) = 0;
	virtual Priority getPriority() const = 0;

protected:
	virtual bool shouldExit() = 0;

public:
	virtual ~IThread() { };
};

typedef std::list<IThread *> spherethreadlist_t;
template<class T>
class TlsValue;

// Singleton utility class for working with threads. Holds all running threads inside
class ThreadHolder
{
public:
	// returns current working thread or DummySphereThread * if no IThread threads are running
	static IThread *current();
	// records a thread to the list. Sould NOT be called, internal usage
	static void push(IThread *thread);
	// removes a thread from the list. Sould NOT be called, internal usage
	static void pop(IThread *thread);
	// returns number of running threads. Sould NOT be called, unit tests usage
	static size_t getActiveThreads() { return m_threadCount; }
	// returns thread at i pos
	static IThread * getThreadAt(size_t at);

private:
	static void init();

private:
	static spherethreadlist_t m_threads;
	static size_t m_threadCount;
	static bool m_inited;
	static SimpleMutex m_mutex;

public:
	static TlsValue<IThread *> m_currentThread;

private:
	ThreadHolder() { };
};

// Thread implementation. See IThread for list of available methods.
class AbstractThread : public IThread
{
public:
	static const char *m_sClassName;

	AbstractThread(LPCTSTR name, Priority priority = IThread::Normal);
	virtual ~AbstractThread();

private:
#ifdef _WIN32
	DWORD m_id;
#else
	pthread_t m_id;
#endif
	LPCTSTR m_name;
	static int m_threadsAvailable;
	spherethread_t m_handle;
	unsigned int m_hangCheck;
	Priority m_priority;
	unsigned long m_tickPeriod;
	AutoResetEvent m_sleepEvent;

	bool m_terminateRequested;
	ManualResetEvent m_terminateEvent;

private:
	AbstractThread(const AbstractThread& copy);
	AbstractThread& operator=(const AbstractThread& other);

public:
#ifdef _WIN32
	DWORD getId() const { return m_id; }
#else
	pthread_t getId() const { return m_id; }
#endif
	LPCTSTR getName() const { return m_name; }

	bool isActive() const;
	bool isCurrentThread() const;
	bool checkStuck();

	virtual void start();
	virtual void terminate(bool ended);
	virtual void waitForClose();
	virtual void awaken();

	void setPriority(Priority pri);
	Priority getPriority() const { return m_priority; }


protected:
	virtual void tick() = 0;
	// NOTE: this should not be too long-lasted function, so no world loading, etc here!!!
	virtual void onStart();
	virtual bool shouldExit();

private:
	void run();
	static SPHERE_THREADENTRY_RETNTYPE SPHERE_THREADENTRY_CALLTYPE runner(void *callerThread);
};

struct TemporaryStringStorage;

// Sphere thread. Have some sphere-specific
class AbstractSphereThread : public AbstractThread
{
private:
#ifdef _THREAD_TRACK_CALLSTACK
	struct STACK_INFO_REC
	{
		const char *functionName;
		ULONGLONG startTime;
	};

	STACK_INFO_REC m_stackInfo[0x1000];
	size_t m_stackPos;
	bool m_freezeCallStack;
#endif

public:
	AbstractSphereThread(const char *name, Priority priority = IThread::Normal);
	virtual ~AbstractSphereThread() { };

private:
	AbstractSphereThread(const AbstractSphereThread& copy);
	AbstractSphereThread& operator=(const AbstractSphereThread& other);

public:
	// allocates a char* with size of THREAD_MAX_LINE_LENGTH characters from the thread local storage
	char *allocateBuffer();
	TemporaryStringStorage *allocateStringBuffer();

	// allocates a manageable String from the thread local storage
	void allocateString(TemporaryString &string);

#ifdef _THREAD_TRACK_CALLSTACK
	inline void freezeCallStack(bool freeze)
	{
		m_freezeCallStack = freeze;
	}

	inline void popStackCall(void)
	{
		if (!m_freezeCallStack)
			m_stackPos--;
	}

	void pushStackCall(const char *name);
	void printStackTrace(void);
#endif

	ProfileData m_profile;	// the current active statistical profile.

protected:
	virtual bool shouldExit();
};

// Dummy thread for context when no thread really exists
class DummySphereThread : public AbstractSphereThread
{
private:
	static DummySphereThread *instance;

public:
	static DummySphereThread *getInstance();

protected:
	DummySphereThread();
	virtual void tick();
};

// stores a value unique to each thread, intended to hold
// a pointer (e.g. the current IThread instance)
template<class T>
class TlsValue
{
private:
#ifdef _WIN32
	DWORD _key;
#else
	pthread_key_t _key;
#endif
	bool _ready;

public:
	TlsValue();
	~TlsValue();

private:
	TlsValue(const TlsValue& copy);
	TlsValue& operator=(const TlsValue& other);

public:
	// allows assignment to set the current value
	TlsValue& operator=(const T& value)
	{
		set(value);
		return *this;
	}
	
	// allows a cast to get current value
	operator T() const { return get(); }

public:
	void set(const T value); // set the value for the current thread
	T get() const; // get the value for the current thread
};

template<class T>
TlsValue<T>::TlsValue()
{
	// allocate thread storage
#ifdef _WIN32
	_key = TlsAlloc();
	_ready = (_key != TLS_OUT_OF_INDEXES);
#else
	_key = 0;
	_ready = (pthread_key_create(&_key, NULL) == 0);
#endif
}
	
template<class T>
TlsValue<T>::~TlsValue()
{
	// free the thread storage
	if (_ready)
#ifdef _WIN32
		TlsFree(_key);
#else
		pthread_key_delete(_key);
#endif
	_ready = false;
}

template<class T>
void TlsValue<T>::set(const T value)
{
	ASSERT(_ready);
#ifdef _WIN32
	TlsSetValue(_key, value);
#else
	pthread_setspecific(_key, value);
#endif
}

template<class T>
T TlsValue<T>::get() const
{
	if (_ready == false)
		return NULL;
#ifdef _WIN32
	return reinterpret_cast<T>(TlsGetValue(_key));
#else
	return reinterpret_cast<T>(pthread_getspecific(_key));
#endif
}


// used to hold debug information for stack
#ifdef _THREAD_TRACK_CALLSTACK
class StackDebugInformation
{
private:
	AbstractSphereThread* m_context;

public:
	inline StackDebugInformation(const char *name)
	{
		m_context = static_cast<AbstractSphereThread *>(ThreadHolder::current());
		if (m_context != NULL)
			m_context->pushStackCall(name);
	}

	inline ~StackDebugInformation()
	{
		if (m_context != NULL)
			m_context->popStackCall();
	}

private:
	StackDebugInformation(const StackDebugInformation& copy);
	StackDebugInformation& operator=(const StackDebugInformation& other);

public:
	inline static void printStackTrace()
	{
		static_cast<AbstractSphereThread *>(ThreadHolder::current())->printStackTrace();
	}
};

#define ADDTOCALLSTACK(_function_)	StackDebugInformation debugStack(_function_)
#define PAUSECALLSTACK static_cast<AbstractSphereThread *>(ThreadHolder::current())->freezeCallStack(true)
#define UNPAUSECALLSTACK static_cast<AbstractSphereThread *>(ThreadHolder::current())->freezeCallStack(false)

// Disable call stack on safe functions that are called intensively to avoid waste performance
#ifdef _DEBUG
#define ADDTOCALLSTACK_INTENSIVE(_function_)	ADDTOCALLSTACK(_function_)
#else
#define ADDTOCALLSTACK_INTENSIVE(_function_)
#endif
#else
#define ADDTOCALLSTACK(_function_)
#define ADDTOCALLSTACK_INTENSIVE(_function_)
#define PAUSECALLSTACK
#define UNPAUSECALLSTACK
#endif	// _THREAD_TRACK_CALLSTACK

#endif
