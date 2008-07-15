#ifndef THREADS_H
#define THREADS_H

// temporary string storage
#define THREAD_STRING_STORAGE	4096
#define THREAD_TSTRING_STORAGE	2048
#define THREAD_STRING_LENGTH	4096

#include "../common/common.h"
#include "../sphere/strings.h"

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
		Idle,		// tick 1000ms
		Low,		// tick 200ms
		Normal,		// tick 100ms
		High,		// tick 50ms
		Highest,	// tick 25ms
		RealTime	// tick almost instantly
	};

	virtual unsigned getId() = 0;
	virtual const char *getName() = 0;

	virtual bool isActive() = 0;
	virtual bool checkStuck() = 0;

	virtual void start() = 0;
	virtual void terminate() = 0;
	virtual void waitForClose() = 0;

	virtual void setPriority(Priority) = 0;
	virtual Priority getPriority() = 0;

protected:
	virtual bool shouldExit() = 0;
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
	static int getActiveThreads();
	// returns thread at i pos
	static IThread * getThreadAt(int at);

private:
	static void init();

private:
	static IThread *	m_threads[MAX_THREADS];
	static int			m_threadCount;
	static bool			m_inited;
};

// Thread implementation. See IThread for list of available methods.
class AbstractThread : public IThread, ThreadHolder
{
private:
	unsigned	m_id;
	const char *m_name;
	static int	m_threadsAvailable;
	spherethread_t	m_handle;
	unsigned	m_hangCheck;
	Priority m_priority;

public:
	AbstractThread(const char *name, Priority priority = IThread::Normal);
	~AbstractThread();

	virtual unsigned getId();
	virtual const char *getName();

	virtual bool isActive();
	virtual bool checkStuck();

	virtual void start();
	virtual void terminate();
	virtual void waitForClose();

	virtual void setPriority(Priority pri = IThread::Normal);
	virtual Priority getPriority();

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
public:
	AbstractSphereThread(const char *name, Priority priority = IThread::Normal);

	// allocates a char* with size of THREAD_MAX_LINE_LENGTH characters from the thread local storage
	char *allocateBuffer();
	TemporaryStringStorage *allocateStringBuffer();

	// allocates a manageable String from the thread local storage
	String allocateString();
	void allocateString(TemporaryString &string);


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


#endif
