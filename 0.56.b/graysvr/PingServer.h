#pragma once
#include "../sphere/threads.h"

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define SOCKET unsigned int
#endif

#define PINGSERVER_PORT		12000	// listen on this port for client pings (clients normally uses 12000)
#define PINGSERVER_BUFFER	64		// number of bytes to receive from clients (client normally sends 40)

class PingServer : public AbstractThread
{
private:
	SOCKET m_socket;
	bool m_active;

public:
	PingServer(void);
	~PingServer(void);

	virtual void onStart();
	virtual void tick();
	virtual void waitForClose();
};
