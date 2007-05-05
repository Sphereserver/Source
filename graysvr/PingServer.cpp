#include "PingServer.h"

PingServer g_PingServer;

// run the thread in RealTime as we need pings to be responded to ASAP
PingServer::PingServer() : AbstractThread("PingServer", IThread::RealTime)
{
	m_socket = 0;
	m_active = false;
}

PingServer::~PingServer()
{
}

void PingServer::onStart()
{
	if ( m_active )
		return;

	if ( m_socket > 0 )
	{
		// socket already created? lets close it
#ifdef _WIN32
		closesocket(m_socket);
#else
		close(m_socket);
#endif
	}

	sockaddr_in sin;
	servent * pse;
	protoent * ppe;

	// prepare to create an open socket for "0.0.0.0" on the ping
	// server port
	sin.sin_addr.s_addr = inet_addr("0.0.0.0");
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PINGSERVER_PORT);

	ppe = getprotobyname("udp");
	if ( !ppe )
		return;
	
	// create the socket
	m_socket = socket(PF_INET, SOCK_DGRAM, ppe->p_proto);
	if ( m_socket < 0 )
		return;

	// bind the port
	if ( bind(m_socket, (sockaddr *)&sin, sizeof(sin)) < 0 )
		return;

	// mark the server as active
	m_active = true;
}

void PingServer::tick()
{
	if ( !m_active )
	{
		// if we're inactive we should try to start up again
		onStart();
		return;
	}

	// prepare to receive data from somewhere
	char buffer[PINGSERVER_BUFFER];
	sockaddr_in addr;
	int addr_len = sizeof(addr);
	
	// receive data from someone
	int length = recvfrom(m_socket, buffer, sizeof(buffer), 0, (sockaddr *)&addr, &addr_len);
	if ( length <= 0 )
		return;

	// return the data to them
	int sent = sendto(m_socket, buffer, length, 0, (sockaddr *)&addr, addr_len);
	if ( sent <= 0 )
		return;
}

void PingServer::waitForClose()
{
	terminate();
	return;
}
