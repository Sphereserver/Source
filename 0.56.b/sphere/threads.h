#ifndef THREADS_H
#define THREADS_H

// temporary string storage
#define THREAD_STRING_STORAGE	4096
#define THREAD_TSTRING_STORAGE	2048
#define THREAD_STRING_LENGTH	4096

#include "../common/common.h"
#include "../sphere/mutex.h"
#include "../sphere/strings.h"
#include "../sphere/ProfileData.h"


// keep track of callstack on windows release builds
#ifdef _WIN32
#	ifndef _DEBUG
#		define THREAD_TRACK_CALLSTACK
#	endif
#endif

/**
 * Sphere threading system
 * Threads should be inherited from AbstractThread with overridden tick() method
 * Also useful to override onStart() in order to initialise class data variables for ticking
 *   which is triggered whenever the thread is starting/restarting
**/

// the maximal number of supported threads. not should be adjusted even as we think that we
// will have 3-4 threads.
// NOTE: Lies here since trying to avoid any problems with lists, etc with
// threads, using as much of static single non-heap data as possible
#define MAX_THREADS	30

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

	virtual unsigned int getId() const = 0;
	virtual const char *getName() const = 0;

	virtual bool isActive() const = 0;
	virtual bool checkStuck() = 0;

	virtual void start() = 0;
	virtual void terminate() = 0;
	virtual void waitForClose() = 0;

	virtual void setPriority(Priority) = 0;
	virtual Priority getPriority() const = 0;

protected:
	virtual bool shouldExit() = 0;

public:
	virtual ~IThread() { };
};

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
	static int getActiveThreads() { return m_threadCount; }
	// returns thread at i pos
	static IThread * getThreadAt(int at);

private:
	static void init();

private:
	static IThread *	m_threads[MAX_THREADS];
	static int			m_threadCount;
	static bool			m_inited;

private:
	ThreadHolder() { };
};

// Thread implementation. See IThread for list of available methods.
class AbstractThread : public IThread
{
private:
	unsigned int m_id;
	const char *m_name;
	static int m_threadsAvailable;
	spherethread_t m_handle;
	unsigned int m_hangCheck;
	Priority m_priority;
	unsigned long m_tickPeriod;
	AutoResetEvent m_sleepEvent;

public:
	AbstractThread(const char *name, Priority priority = IThread::Normal);
	virtual ~AbstractThread();

private:
	AbstractThread(const AbstractThread& copy);
	AbstractThread& operator=(const AbstractThread& other);

public:
	unsigned int getId() const { return m_id; }
	const char *getName() const { return m_name; }

	bool isActive() const;
	bool isCurrentThread() const;
	bool checkStuck();

	virtual void start();
	virtual void terminate();
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
#ifdef THREAD_TRACK_CALLSTACK
	struct STACK_INFO_REC
	{
		const char *functionName;
		LONGLONG	startTime;
	};

	STACK_INFO_REC m_stackInfo[0x1000];
	long m_stackPos;
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

#ifdef THREAD_TRACK_CALLSTACK
	inline void pushStackCall(const char *name)
	{
		m_stackInfo[m_stackPos].functionName = name;
		m_stackInfo[m_stackPos].startTime = ::GetTickCount();
		m_stackPos++;
		m_stackInfo[m_stackPos].startTime = 0;
	}

	inline void popStackCall(void)
	{
		m_stackPos--;
	}

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

// used to hold debug information for stack
#ifdef THREAD_TRACK_CALLSTACK
class StackDebugInformation
{
private:
	AbstractSphereThread* m_context;

public:
	StackDebugInformation(const char *name);
	~StackDebugInformation();

private:
	StackDebugInformation(const StackDebugInformation& copy);
	StackDebugInformation& operator=(const StackDebugInformation& other);

public:
	static void printStackTrace();
};

#define ADDTOCALLSTACK(_function_)	StackDebugInformation debugStack(_function_);
#else
#define ADDTOCALLSTACK(_function_)
#endif // THREAD_TRACK_CALLSTACK

#endif
