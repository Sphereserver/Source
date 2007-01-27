#ifndef NETWORK_H
#define NETWORK_H

#ifdef VJAKA_REDO
#include "../common/common.h"
#include "threads.h"
#include "PServer.h"

class CPServer
{
public:
	DWORD Start(PPSERVERPARAM pParam);
	VOID  Stop();
	DWORD PostRecv(PCONNECTION pConnection, PBYTE pbBuffer, DWORD dwSize, PVOID pvPerIoUserData);
	DWORD PostSend(PCONNECTION pConnection, PBYTE pbBuffer, DWORD dwSize, PVOID pvPerIoUserData);
	VOID  CloseConnection(PCONNECTION pConnection);
	VOID  QueryInformation(PINFORMATION pInformation);
	VOID  EnableConnectionIdleCheck(PCONNECTION pConnection, BOOL fEnable);
	VOID  EnumConnections(ENUMCONNECTIONSPROC pEnumConnectionsProc, PVOID pvParam);
};

class Network : public AbstractThread
{
public:
	Network();
	~Network();

	virtual void tick();
	virtual void onStart();

//private:
//	HANDLE		m_iocp;
};
#endif

#endif
