#ifdef VJAKA_REDO

#include "../graysvr/graysvr.h"
#include "../sphere/threads.h"

/**
 * Sphere unit testing
 * threading unit test
 * give command line argument containing testThreads (ntservice.cpp) to test this
**/

#define USUAL_DELAY	500
#define TESTS_OK_NOTIFY	0
#define	IGNORE_THIS_WINDOW 0

class ThreadSimple : public AbstractThread {
public:
	ThreadSimple() : AbstractThread("simple") {
	}

	virtual void tick() {
	}
};

class ThreadExceptions : public AbstractThread {
public:
	int style;

	ThreadExceptions() : AbstractThread("exceptions") {
		style = 0;
	}

	void mykillingstackmachine(int a, int b, int c, int d, int e, int f) {
		//6*4*1000 = 240kb/function + arguments
		int arrA[10000];
		memset(arrA, a, sizeof(arrA));
		int arrB[10000];
		memset(arrB, b, sizeof(arrB));
		int arrC[10000];
		memset(arrC, c, sizeof(arrC));
		int arrD[10000];
		memset(arrD, d, sizeof(arrD));
		int arrE[10000];
		memset(arrE, e, sizeof(arrE));
		int arrF[10000];
		memset(arrF, f, sizeof(arrF));

		mykillingstackmachine(a+b, b+c, c+d, d+e, e+f, f+a);
	}

	virtual void tick() {
		switch( style ) {
			case 0: {	// divizion by zero
				int a = 10;
				int b = 3;
				b-=3;
				a /= b;
				break;
					}
			case 1: {	// throw (int) exception
				throw 0;
				break;
					}

			case 2: {	// NULL pointer with vtable
				IThread *thread = NULL;
				thread->isActive();
				break;
					}

			case 3: {	// invalid pointer with vtable
				IThread *thread = (IThread *)0x335337L;
				thread->isActive();
				break;
					}

			case 4: {
				mykillingstackmachine(1, 2, 3, 4, 5, 6);
				break;
					}
		}
	}
};

class ThreadHang : public AbstractThread {
public:
	ThreadHang() : AbstractThread("hanged") {
	}

	virtual void tick() {
		while( true ) {
			Sleep(0);
		}
	}
};

void testThreadsRunningAndTerminating() {
	// ONE stopped, TWO stopped
	IThread *one = new ThreadSimple();
	IThread *two = new ThreadSimple();

	assert(one->getId() == 0);
	assert(!one->isActive());
	assert(two->getId() == 0);
	assert(!two->isActive());
	assert(ThreadHolder::getActiveThreads() == 0);

	// ONE started, TWO started
	one->start();
	two->start();

	assert(one->getId() != 0);
	assert(one->isActive());
	assert(two->getId() != 0);
	assert(two->isActive());
	assert(ThreadHolder::getActiveThreads() == 2);

	Sleep(USUAL_DELAY);

	// ONE stopped, TWO started
	one->terminate();

	assert(one->getId() == 0);
	assert(!one->isActive());
	assert(two->getId() != 0);
	assert(two->isActive());
	assert(ThreadHolder::getActiveThreads() == 1);

	Sleep(USUAL_DELAY);

	// ONE started, TWO started
	one->start();

	assert(one->getId() != 0);
	assert(one->isActive());
	assert(two->getId() != 0);
	assert(two->isActive());
	assert(ThreadHolder::getActiveThreads() == 2);

	Sleep(USUAL_DELAY);

	// ONE stopped, TWO stopped
	one->terminate();
	two->terminate();

	assert(one->getId() == 0);
	assert(!one->isActive());
	assert(two->getId() == 0);
	assert(!two->isActive());
	assert(ThreadHolder::getActiveThreads() == 0);

	delete one;
	delete two;
}

void testThreadsExceptions() {
	ThreadExceptions *thread = new ThreadExceptions();

	thread->start();
	Sleep(USUAL_DELAY);

	thread->style = 1;
	Sleep(USUAL_DELAY);

	thread->style = 2;
	Sleep(USUAL_DELAY);

	thread->style = 3;
	Sleep(USUAL_DELAY);

	//	NOTE: very strange, but this does not drop stack even if making long sleep fe (60000)
	thread->style = 4;
	Sleep(USUAL_DELAY);

	delete thread;
}

void testThreadsHangAndRestart() {
	ThreadHang *thread = new ThreadHang();

	thread->start();
	int id = thread->getId();
	Sleep(USUAL_DELAY);

	//	first check - should just prepare
	thread->checkStuck();
	assert(thread->isActive());
	assert(id == thread->getId());
	assert(ThreadHolder::getActiveThreads() == 1);

	//	second check - should just notify
	Sleep(USUAL_DELAY);
	thread->checkStuck();
	assert(thread->isActive());
	assert(id == thread->getId());
	assert(ThreadHolder::getActiveThreads() == 1);

	//	third check - should kill and respawn (with new ID of course)
	Sleep(USUAL_DELAY);
	thread->checkStuck();
	assert(thread->isActive());
	assert(id != thread->getId());
	assert(ThreadHolder::getActiveThreads() == 1);

	//	deleting thread should trigger it removal and stop
	delete thread;
	assert(ThreadHolder::getActiveThreads() == 0);
}

void testThreads() {
	//	running 2 threads
	testThreadsRunningAndTerminating();

	//	generating exceptions should not spoil threads
	testThreadsExceptions();

	//	hanging and restarting threads
	testThreadsHangAndRestart();

	assert(TESTS_OK_NOTIFY && IGNORE_THIS_WINDOW);
}

#endif