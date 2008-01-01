#include "networkdata.h"
#include "../graysvr/graysvr.h"

#define MAX_PACKET_SEND_TICK	10
#define NETWORKIN_SLEEP			1
#define NETWORKOUT_SLEEP		1

NetworkContainer g_GlobalNetwork;

NetworkContainer::NetworkContainer() : /*m_NetworkIn(&m_clientLock),*/ m_NetworkOut(&m_clientLock)
{
}

NetworkContainer::~NetworkContainer()
{
}
	
void NetworkContainer::start()
{
	//m_NetworkIn.start();
	m_NetworkOut.start();
}

void NetworkContainer::stop()
{
	//m_NetworkIn.waitForClose();
	m_NetworkOut.waitForClose();
}

void NetworkContainer::setLockFromClient()
{
	//m_NetworkIn.SetLockFromClient();
	m_NetworkOut.SetLockFromClient();
}

void NetworkContainer::unsetLockFromClient()
{
	//m_NetworkIn.UnsetLockFromClient();
	m_NetworkOut.UnsetLockFromClient();
}

// ----------------------------------------
// Network Interface
// ----------------------------------------

NetworkInterface::NetworkInterface(SimpleMutex * mutexLock) : m_active(true), m_loopActive(true), m_netwLock(mutexLock)
{
}

NetworkInterface::~NetworkInterface()
{
	m_netwLock.doUnlock();
}

bool NetworkInterface::getActive()
{
	return m_active;
}

bool NetworkInterface::getLoopActive()
{
	return m_loopActive;
}

void NetworkInterface::setActive(bool value)
{
	m_active = value;
}

void NetworkInterface::setLoopActive(bool value)
{
	m_loopActive = value;
}

void NetworkInterface::SetLockFromClient()
{
	setLoopActive(false);
	m_netwLock.doLock();
}

void NetworkInterface::UnsetLockFromClient()
{
	m_netwLock.doUnlock();
	setLoopActive(true);
}

// ----------------------------------------
// Network In
// ----------------------------------------

//NetworkIn::NetworkIn(SimpleMutex * mutexLock) : AbstractThread("NetworkIn", IThread::RealTime), NetworkInterface(mutexLock), m_tickCount(0)
//{
//}
//
//NetworkIn::~NetworkIn()
//{
//}
//
//void NetworkIn::onStart()
//{
//}
//
//#ifdef _WIN32
//	#define ADDTOSELECT(_x_)	FD_SET(_x_, &readfds)
//#else
//	#define ADDTOSELECT(_x_)	{ FD_SET(_x_, &readfds); if ( _x_ > nfds ) nfds = _x_; }
//#endif
//
//void NetworkIn::tick()
//{
//	if ( g_Serv.IsLoading() )
//	{
//		Sleep(NETWORKIN_SLEEP * 10);
//		return;
//	}
//	
//	Sleep(NETWORKIN_SLEEP);
//
//	if ( getLoopActive() && !((bool)m_netwLock) )
//	{
//		m_netwLock.doLock();
//
//		// ---------------------------
//		CClient * pClient = NULL;
//		CClient * pClientNext = NULL;
//		fd_set readfds;
//		int connecting = 0, nfds = 0;
//
//		FD_ZERO(&readfds);
//		ADDTOSELECT(g_Serv.m_SocketMain.GetSocket());
//
//		try
//		{
//			for ( pClient = g_Serv.GetClientHead(); getLoopActive() && pClient; pClient = pClientNext )
//			{
//				pClientNext = pClient->GetNext();
//
//				if ( !pClient->m_Socket.IsOpen() || pClient->m_fClosed )
//				{
//					g_Serv.addBadclient(pClient);
//					continue;
//				}
//
//				if (( pClient->IsConnecting() ) && ( ++connecting > g_Cfg.m_iConnectingMax ))
//				{
//					g_Serv.addBadclient(pClient);
//					continue;
//				}
//
//				ADDTOSELECT(pClient->m_Socket.GetSocket());
//			}
//
//			if ( getLoopActive() )
//			{
//				if ( connecting > g_Cfg.m_iConnectingMax )
//				{
//					g_Log.Event(LOGL_WARN|LOGM_CHEAT, "%d clients in connect mode (max %d), closing %d\n",
//						connecting, g_Cfg.m_iConnectingMax, connecting - g_Cfg.m_iConnectingMax);
//				}
//
//				g_Serv.m_Profile.Start(PROFILE_IDLE);
//				
//				timeval Timeout;	// time to wait for data.
//				Timeout.tv_sec = 0;
//				Timeout.tv_usec = 100;	// micro seconds = 1/1000000
//				int ret = select( nfds+1, &readfds, NULL, NULL, &Timeout );
//
//				g_Serv.m_Profile.Start( PROFILE_NETWORK_RX );
//
//				if ( ret > 0 )
//				{
//					// Process new connections.
//					if ( FD_ISSET( g_Serv.m_SocketMain.GetSocket(), &readfds))
//					{
//						doNewClient( g_Serv.m_SocketMain );
//					}
//
//					for ( pClient = g_Serv.GetClientHead(); getLoopActive() && pClient; pClient = pClientNext )
//					{
//						pClientNext = pClient->GetNext();
//
//						if ( !pClient->m_Socket.IsOpen() || pClient->m_fClosed )
//						{
//							g_Serv.addBadclient(pClient);
//							continue;
//						}
//
//						if ( FD_ISSET( pClient->m_Socket.GetSocket(), &readfds ))
//						{
//							pClient->m_timeLastEvent = CServTime::GetCurrentTime();	// We should always get pinged every couple minutes or so
//							
//							if ( ! doReceiveData(pClient) )
//							{
//								g_Serv.addBadclient(pClient);
//								continue;
//							}
//
//							if ( pClient->m_fClosed )		// can happen due to data received
//							{
//								g_Serv.addBadclient(pClient);
//								continue;
//							}
//						}
//						else
//						{
//							// NOTE: Not all CClient are game clients.
//
//							int iLastEventDiff = -g_World.GetTimeDiff( pClient->m_timeLastEvent );
//
//							if ( g_Cfg.m_iDeadSocketTime && ( iLastEventDiff > g_Cfg.m_iDeadSocketTime )&&
//								( pClient->GetConnectType() != CONNECT_TELNET ))
//							{
//								// We have not talked in several minutes.
//								DEBUG_ERR(( "%x:Dead Socket Timeout\n", pClient->m_Socket.GetSocket()));
//								g_Serv.addBadclient(pClient);
//								continue;
//							}
//						}
//					}
//					
//				}
//
//				g_Serv.m_Profile.Start( PROFILE_OVERHEAD );
//			}
//
//		}
//		catch ( CGrayError &e )	
//		{ 
//			g_Log.CatchEvent(&e, "NetworkIn::tick()"); 
//		}
//		catch (...) 
//		{ 
//			g_Log.CatchEvent(NULL, "NetworkIn::tick()"); 
//		}
//
//		m_netwLock.doUnlock();
//	}
//
//	Sleep(NETWORKIN_SLEEP);
//}
//
//void NetworkIn::waitForClose()
//{
//	setLoopActive(false);
//	setActive(false);
//	return;
//}
//
//bool NetworkIn::shouldExit()
//{
//	return !getActive();
//}
//
//CClient * NetworkIn::doNewClient( CGSocket & socket )
//{
//	CSocketAddress client_addr;
//	SOCKET hSocketClient = socket.Accept( client_addr );
//
//	if (( hSocketClient < 0 ) || ( hSocketClient == INVALID_SOCKET ))
//	{
//		// NOTE: Client_addr might be invalid.
//		g_Log.Event( LOGL_FATAL|LOGM_CLIENTS_LOG, "Failed at client connection to '%s'(?)\n", (LPCTSTR) client_addr.GetAddrStr());
//		return NULL;
//	}
//
//	CLogIP * pLogIP = g_Cfg.FindLogIP( client_addr, true );
//	if (( pLogIP == NULL ) || pLogIP->CheckPingBlock( true ))
//	{
//		// kill it by allowing it to go out of scope.
//		CGSocket::CloseSocket( hSocketClient );
//		return NULL;
//	}
//
//	// too many connecting on this IP
//	if ( ( (g_Cfg.m_iConnectingMaxIP > 0) && (pLogIP->m_iConnecting > g_Cfg.m_iConnectingMaxIP) )
//			|| (  (g_Cfg.m_iClientsMaxIP > 0) && (pLogIP->m_iConnected > g_Cfg.m_iClientsMaxIP) ) )
//	{
//		// kill
//		CGSocket::CloseSocket( hSocketClient );
//		return NULL;
//	}
//
//	CClient *client = new CClient(hSocketClient);
//	return client;
//}
//
//bool NetworkIn::doReceiveData( CClient * pClient )
//{
//	CEvent Event;
//	int iCountNew = pClient->m_Socket.Receive( &Event, sizeof(Event), 0 );
//
//	if ( iCountNew <= 0 )	// I should always get data here.
//		return( false ); // this means that the client is gone.
//
//	g_Serv.m_Profile.Count( PROFILE_DATA_RX, iCountNew );
//
//	// Enqueue
//	pClient->xAddInputPacket( &Event, iCountNew );
//
//	return( true );
//}

// ----------------------------------------
// Network Out
// ----------------------------------------

NetworkOut::NetworkOut(SimpleMutex * mutexLock) : AbstractSphereThread("NetworkOut", IThread::RealTime), NetworkInterface(mutexLock)
{
}

NetworkOut::~NetworkOut()
{
}

void NetworkOut::onStart()
{
}

void NetworkOut::tick()
{
	if ( g_Serv.IsLoading() )
	{
		Sleep(NETWORKOUT_SLEEP * 10);
		return;
	}
			
	if ( getLoopActive() && !((bool)m_netwLock) )
	{
		m_netwLock.doLock();

		// ---------------------------
		CClient * pClient = NULL;
		CClient * pClientNext = NULL;

		try
		{
			for ( pClient = g_Serv.GetClientHead(); getLoopActive() && pClient; pClient = pClientNext )
			{
				pClientNext = pClient->GetNext();
				
				if ( pClient->m_Socket.IsOpen() && !pClient->m_fClosed )
				{
					for(int i = 0; m_loopActive && (i < MAX_PACKET_SEND_TICK); ++i)
					{
						pClient->m_timeLastSend = CServTime::GetCurrentTime();
						volatile int iLenSend = pClient->xGetFrontPacketSize();

						if ( iLenSend == -1 )
						{
							break;
						}
						else
						{
							int iLenRetn = pClient->m_Socket.Send(pClient->xGetFrontPacketData(), iLenSend);

							if ( iLenRetn != SOCKET_ERROR )
							{
								g_Serv.m_Profile.Count( PROFILE_DATA_TX, iLenSend );
								pClient->xRemoveFrontPacket();
							}
							
							if ( iLenRetn != iLenSend )
							{
								pClient->xSendError(CGSocket::GetLastError());
								break;
							}
						}
					}
				}
			}
		}
		catch ( CGrayError &e )	
		{ 
			g_Log.CatchEvent(&e, "NetworkOut::tick()"); 
		}
		catch (...) 
		{ 
			g_Log.CatchEvent(NULL, "NetworkOut::tick()"); 
		}

		m_netwLock.doUnlock();
	}

	Sleep(NETWORKOUT_SLEEP);
}

void NetworkOut::waitForClose()
{
	setLoopActive(false);
	setActive(false);
	return;
}

bool NetworkOut::shouldExit()
{
	return !getActive();
}