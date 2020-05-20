#include "CPingServer.h"

CPingServer g_PingServer;

CPingServer::CPingServer() : AbstractSphereThread("PingServer", IThread::RealTime)
{
	m_profile.EnableProfile(PROFILE_NETWORK_RX);
	m_profile.EnableProfile(PROFILE_NETWORK_TX);
	m_profile.EnableProfile(PROFILE_DATA_TX);
	m_profile.EnableProfile(PROFILE_DATA_RX);
	m_socket.Close();
}

void CPingServer::onStart()
{
	AbstractSphereThread::onStart();
	if ( m_socket.IsOpen() )
		m_socket.Close();

	if ( !m_socket.Create(PF_INET, SOCK_DGRAM, CGSocket::GetProtocolIdByName("udp")) )
	{
		m_socket.Close();
		return;
	}

	CSocketAddress SockAddr;
	SockAddr.SetAddrIP(INADDR_LOOPBACK_REVERSE);
	SockAddr.SetPortNum(PINGSERVER_PORT);

	if ( m_socket.Bind(SockAddr) == SOCKET_ERROR )
	{
		m_socket.Close();
		return;
	}
}

void CPingServer::tick()
{
	// Prepare to receive data from somewhere
	char buffer[PINGSERVER_BUFFER];
	sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);

	// Receive data
	ProfileTask receiveTask(PROFILE_NETWORK_RX);
	int received = recvfrom(m_socket.GetSocket(), buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr *>(&addr), &addr_len);
	if ( received <= 0 )
		return;
	CurrentProfileData.Count(PROFILE_DATA_RX, received);

	// Reply data
	ProfileTask sendTask(PROFILE_NETWORK_TX);
	int sent = sendto(m_socket.GetSocket(), buffer, received, 0, reinterpret_cast<const sockaddr *>(&addr), addr_len);
	if ( sent <= 0 )
		return;
	CurrentProfileData.Count(PROFILE_DATA_TX, sent);
}

bool CPingServer::shouldExit()
{
	if ( !m_socket.IsOpen() )
		return true;
	return AbstractSphereThread::shouldExit();
}

void CPingServer::waitForClose()
{
	m_socket.Close();
	AbstractSphereThread::waitForClose();
}
