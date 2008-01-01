#ifndef _INC_NETWORKOUT_H
#define _INC_NETWORKOUT_H
#pragma once

#include "threads.h"
#include "mutex.h"

class CGSocket;
class CClient;

class NetworkInterface
{
protected:
	bool m_active;
	bool m_loopActive;
	ManualThreadLock m_netwLock;

public:
	NetworkInterface(SimpleMutex *);
	~NetworkInterface(void);

	bool getActive();
	bool getLoopActive();
	void setActive(bool);
	void setLoopActive(bool);

	void SetLockFromClient();
	void UnsetLockFromClient();
};

//class NetworkIn : public AbstractSphereThread, public NetworkInterface
//{
//private:
//	int m_tickCount;
//
//public:
//	NetworkIn(SimpleMutex *);
//	~NetworkIn(void);
//
//	virtual void onStart();
//	virtual void tick();
//	virtual void waitForClose();
//	virtual bool shouldExit();
//
//private:
//	CClient * doNewClient( CGSocket & );
//	bool doReceiveData( CClient * );
//};

class NetworkOut : public AbstractSphereThread, public NetworkInterface
{
public:
	NetworkOut(SimpleMutex *);
	~NetworkOut(void);

	virtual void onStart();
	virtual void tick();
	virtual void waitForClose();
	virtual bool shouldExit();
};

class NetworkContainer
{
private:
	//NetworkIn m_NetworkIn;
	NetworkOut m_NetworkOut;
	SimpleMutex m_clientLock;

public:
	NetworkContainer(void);
	~NetworkContainer(void);

	void start();
	void stop();

	void setLockFromClient();
	void unsetLockFromClient();
};


#endif