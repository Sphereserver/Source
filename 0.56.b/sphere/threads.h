#ifndef THREADS_H
#define THREADS_H

/**
 * Sphere threading system
 * Threads should be inherited from AbstractThread with overridden tick() method
 * Also useful to override onStart() in order to initialise class data variables for ticking
 *   which is triggeted whanever the thread is starting/restarting
**/

// the maximal number of supported threads. not should be adjusted even as we think that we
// will have 3-4 threads.
// NOTE: Lies here since trying to avoid any problems with lists, etc with
// threads, using as much of static single non-heap data as possible
#define MAX_THREADS	30

// Types definition for different platforms
#ifdef _WIN32
	typedef HANDLE spherethread_t;
	#define SPHERE_THREADENTRY_RETNTYPE unsigned;
	#define SPHERE_THREADENTRY_CALLTYPE __stdcall
#else
	typedef pthread_t spherethread_t;
	#define SPHERE_THREADENTRY_RETNTYPE void *;
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
	virtual void checkStuck() = 0;

	virtual void start() = 0;
	virtual void terminate() = 0;
	virtual void waitForClose() = 0;
};

// Singleton utility class for working with threads. Holds all running threads inside
class ThreadHolder 
{
public:
	// returns current working thread or NULL if no IThread threads are running
	static IThread *current();
	// records a thread to the list. Sould NOT be called, internal usage
	static void push(IThread *thread);
	// removes a thread from the list. Sould NOT be called, internal usage
	static void pop(IThread *thread);
	// returns number of running threads. Sould NOT be called, unit tests usage
	static int getActiveThreads();

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
	IThread::Priority m_priority;

public:
	AbstractThread(const char *name, IThread::Priority priority = IThread::Priority::Normal);
	~AbstractThread();

	virtual unsigned getId();
	virtual const char *getName();

	virtual bool isActive();
	virtual void checkStuck();

	virtual void start();
	virtual void terminate();
	virtual void waitForClose();

protected:
	virtual void tick() = 0;
	// NOTE: this should not be too long-lasted function, so no world loading, etc here!!!
	virtual void onStart();

private:
	void run();
	static SPHERE_THREADENTRY_RETNTYPE SPHERE_THREADENTRY_CALLTYPE runner(void *callerThread);
};

#endif
