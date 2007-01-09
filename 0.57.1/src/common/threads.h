#ifndef CTHREAD_H
#define CTHREAD_H

#include "common.h"

#define THREAD_MAX_LINE_LEN	0x1000	// length of temp line
#define	THREAD_TEMP_LINES	0x200	// number of static temporary lines in thread storage
#define	THREAD_EXC_STACK	0x400	// number of recorded call stack

//
// represents an object with locking possibility
//
#define THREAD_LOCK_MASK	0xf0000000L		// mask for critical lock

class CThreadSafe
{
public:
	static const char *m_sClassName;

	CThreadSafe();
	~CThreadSafe();

	void Lock(bool critical = false);		// locks object (if critical than absolutely)
	void Release();							// releases object

public:
	DWORD m_lockThread;
};

//
// represents a basic class for the threads
//
class CScript;

enum E_DEADLOCK_TYPE
{
	E_DL_GETSINGLE,
	E_DL_QTY,
};

class CThread
{
protected:
	DWORD	m_id;
	const char *m_name;
#ifdef _WIN32
	DWORD m_hThread;
#else
	pthread_t		m_hThread;
	pthread_attr_t	m_hThread_attr;
#endif
	PTHREAD_ENTRY_PROC m_proc;

	bool IsActive() const;
public:
	static const char *m_sClassName;
	static DWORD Current();					// current working thread ID
	static CThread* Thread();				// current working thread pointer

	CThread(const char *name = NULL);
	~CThread();

	void Name(const char *name);
	const char *Name();

	//	general thread operations
	void OnClose();							// callback to be called on exit
	void OnCreate();						// callback to be called on create
	DWORD ID();								// returns ID of the current thread
	void Terminate(DWORD dwExitCode);		// closes the thread
	void Create(PTHREAD_ENTRY_PROC pEntryProc, void *pArgs = NULL);
	virtual void Start() = 0;				// starts the thread execution
	void WaitForClose();					// waits for some time for the thread to quit
	void CheckStuck();						// checks thread for stucking and restarts it if needed
	void OnTick(int sleepTime = 0);			// thread tick operations

	void CriticalStart();					// starts critical section
	bool CriticalStartTry(int tryes = 1);	// trys to enter critical
	void CriticalEnd();						// ends critical section
	bool Critical();						// am i in a critical mode?

	//	private thread data operations
	const CScript *ScriptContext(const CScript *context = (const CScript *)(-1));	// set/get script context
	char *TempString(int amount = 1);		// gets a string from a temp bufer
	long ExcCount();
	void ExcCount(long count);
	void ExcRegister(const char *classname, const char *funcname);
	void ExcStack();						// prints exception stack
	long DL(E_DEADLOCK_TYPE tp);
	void DL(E_DEADLOCK_TYPE tp, long value);
public:
	//	static global thread support features
	static DWORD			m_stThreads;
	static CRITICAL_SECTION	m_stSafeAccess;

	static void StaticInit();				// inits critical section
	static void StaticDeinit();				// deinits critical section

	const char *m_action;					// current action
private:
	//	internal structure specific to threads only (2Mbytes) per each thread
	//	temporary lines stack (4 + 256*4096)
	long	m_tmpStringIndex;
	char	m_tmpStrings[THREAD_TEMP_LINES][THREAD_MAX_LINE_LEN];

	//	exception catching stack (4 + 1024*4 + 1024*4)
	long		m_excPosAbsolute;			// virtual stack position (shoud be >= catch)
	const char *m_excClassNames[THREAD_EXC_STACK];
	const char *m_excFunctNames[THREAD_EXC_STACK];

	//	script context link
	const CScript *m_scriptContext;

	//	general variables
	bool m_inCritical;
	long m_hangCheck;
	long m_antiDeadLock[E_DL_QTY];
};

extern vector<CThread *>	g_Threads;

#endif
