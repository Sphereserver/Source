#ifndef _INC_PINGSERVER_H
#define _INC_PINGSERVER_H
#pragma once

#include "../sphere/threads.h"

#define PINGSERVER_PORT		12000	// listen on this port for client pings (clients normally uses 12000)
#define PINGSERVER_BUFFER	64		// number of bytes to receive from clients (client normally sends 40)

class PingServer : public AbstractSphereThread
{
public:
	PingServer();
	virtual ~PingServer();

private:
	CGSocket m_socket;

public:
	virtual void onStart();
	virtual void tick();
	virtual bool shouldExit();
	virtual void waitForClose();
};

#endif
