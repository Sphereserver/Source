#include "PingServer.h"

PingServer g_PingServer;

// run the thread in RealTime as we need pings to be responded to ASAP
PingServer::PingServer() : AbstractSphereThread("PingServer", IThread::RealTime)
{
	m_profile.EnableProfile(PROFILE_IDLE);
	m_profile.EnableProfile(PROFILE_OVERHEAD);
	m_profile.EnableProfile(PROFILE_NETWORK_RX);
	m_profile.EnableProfile(PROFILE_NETWORK_TX);
	m_profile.EnableProfile(PROFILE_DATA_TX);
	m_profile.EnableProfile(PROFILE_DATA_RX);

	m_socket.Close();
	m_active = false;
}

PingServer::~PingServer()
{
}

void PingServer::onStart()
{
	if ( m_active )
		return;

	if ( m_socket.IsOpen() )
	{
		// socket already created? lets close it
		m_socket.Close();
	}

	if ( ! m_socket.Create(PF_INET, SOCK_DGRAM, CGSocket::GetProtocolIdByName("udp")) )
		return;

	CSocketAddress saToBind;
	saToBind.SetHostStr("0.0.0.0");
	saToBind.SetPort(PINGSERVER_PORT);

	if ( m_socket.Bind(saToBind) < 0 )
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
	socklen_t addr_len = sizeof(addr);

	ProfileTask receiveTask(PROFILE_NETWORK_RX);
	
	// receive data from someone
	int length = recvfrom(m_socket.GetSocket(), buffer, sizeof(buffer), 0, (sockaddr *)&addr, &addr_len);
	if ( length <= 0 )
		return;

	CurrentProfileData.Count(PROFILE_DATA_RX, length);
	ProfileTask sendTask(PROFILE_NETWORK_TX);

	// return the data to them
	int sent = sendto(m_socket.GetSocket(), buffer, length, 0, (sockaddr *)&addr, addr_len);
	if ( sent <= 0 )
		return;

	CurrentProfileData.Count(PROFILE_DATA_TX, sent);
}
