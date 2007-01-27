//#include "winsock2.h"
//#include "mswsock.h"
#include "network.h"
#include "../graysvr/graysvr.h"

#ifdef VJAKA_REDO

#define BUF_SIZE 8192

CPServer PServer;

DWORD CPServer::Start(PPSERVERPARAM pParam)
{
	return PServer_Start(pParam);
}

VOID CPServer::Stop()
{
	PServer_Stop();
}

DWORD CPServer::PostRecv(PCONNECTION pConnection, PBYTE pbBuffer, DWORD dwSize, PVOID pvPerIoUserData)
{
	return PServer_PostRecv(pConnection, pbBuffer, dwSize, pvPerIoUserData);
}

DWORD CPServer::PostSend(PCONNECTION pConnection, PBYTE pbBuffer, DWORD dwSize, PVOID pvPerIoUserData)
{
	return PServer_PostSend( pConnection, pbBuffer, dwSize, pvPerIoUserData );
}

VOID CPServer::CloseConnection(PCONNECTION pConnection)
{
	PServer_CloseConnection( pConnection );
}

VOID CPServer::QueryInformation(PINFORMATION pInformation)
{
	PServer_QueryInformation(pInformation);
}

VOID CPServer::EnableConnectionIdleCheck(PCONNECTION pConnection, BOOL fEnable)
{
	PServer_EnableConnectionIdleCheck( pConnection, fEnable );
}

VOID CPServer::EnumConnections(ENUMCONNECTIONSPROC pEnumConnectionsProc, PVOID pvParam)
{
	PServer_EnumConnections( pEnumConnectionsProc, pvParam );
}

Network::Network() : AbstractThread("Network", IThread::High)
{
//	m_iocp = NULL;
}

Network::~Network()
{
}

BOOL CALLBACK OnConnect( PCONNECTION pConnection )
{
//    InterlockedIncrement( &g_lOnConnectCount );

    // Allocate connection buffer.
    pConnection->pvPerConnectionUserData = malloc( BUF_SIZE );
    if( NULL == pConnection->pvPerConnectionUserData ){
        return FALSE;
        }

    /*
    // Get IP string.
    TCHAR szIP[20] = {0};
    wsprintf( szIP, T("%d.%d.%d.%d"), ( (PBYTE) &pConnection->dwIP )[0], 
                                      ( (PBYTE) &pConnection->dwIP )[1], 
                                      ( (PBYTE) &pConnection->dwIP )[2], 
                                      ( (PBYTE) &pConnection->dwIP )[3] );
    */

	CSocketAddressIP ip(pConnection->dwIP);
	CLogIP * pLogIP = g_Cfg.FindLogIP(ip, true );
	if ( pLogIP == NULL || pLogIP->CheckPingBlock( true ))
	{
		// kill it by allowing it to go out of scope.
		CGSocket::CloseSocket( pConnection->Socket );
		return NULL;
	}

	// too many connecting on this IP
	if ( ( (g_Cfg.m_iConnectingMaxIP > 0) && (pLogIP->m_iConnecting > g_Cfg.m_iConnectingMaxIP) )
			|| (  (g_Cfg.m_iClientsMaxIP > 0) && (pLogIP->m_iConnected > g_Cfg.m_iClientsMaxIP) ) )
	{
		// kill
		CGSocket::CloseSocket( pConnection->Socket );
		return NULL;
	}

	new CClient(pConnection->Socket);

    // Receive request.
	DWORD dwRet = PServer.PostRecv( pConnection, 
                                    (PBYTE) pConnection->pvPerConnectionUserData, // Buffer.
                                    BUF_SIZE,                                     // Buffer size.
                                    NULL );
    if( dwRet != ERROR_SUCCESS ){
        return FALSE;
        }

    return TRUE;
}

//##############################################################################################################//
VOID CALLBACK OnDisconnect( PCONNECTION pConnection )
{
//    InterlockedIncrement( &g_lOnDisconnectCount );

    // Free memory.
    if( pConnection->pvPerConnectionUserData != NULL ){
        free( pConnection->pvPerConnectionUserData );
        }
}

//##############################################################################################################//
BOOL CALLBACK OnRecv( PCONNECTION pConnection, PIO_DATA pIoData )
{
    // Send request.
    DWORD dwRet = PServer.PostSend( pConnection,
                                    (PBYTE) pConnection->pvPerConnectionUserData, // Data.
                                    pIoData->dwBytesTransferred,                  // Data size.
                                    NULL );
    if( dwRet != ERROR_SUCCESS ){
//        LogError( "PostSend()", dwRet );
        return FALSE;
        }

    return TRUE;
}

//##############################################################################################################//
BOOL CALLBACK OnSend( PCONNECTION pConnection, PIO_DATA pIoData )
{
    // Data transferred - close connection.
    return FALSE;
}

//##############################################################################################################//
VOID CALLBACK OnError( PCWSTR pszFunc, DWORD dwError )
{
    TCHAR szError[64] = {0};
    wsprintf( szError, "%s(): %d", pszFunc, dwError );
}

void Network::tick()
{
/*	DWORD numberBytes;
	DWORD key;
	OVERLAPPED completedOverlapped;
	LPOVERLAPPED over = &completedOverlapped;
	while( GetQueuedCompletionStatus(m_iocp, &numberBytes, &key, &over, 0) ) {
		//	TODO: here we have the KEY for client and DATA incoming
		//	since we yet operate on main socket only, read it always
		g_Serv.SocketsReceive(g_Serv.m_SocketMain);
	}
	//	TODO: when client is dropped, it should be removed from queue
	*/
}

void Network::onStart()
{
	PSERVERPARAM param = {0};

            param.dwWorkerThreadsIdleTimeout = 10000;
            param.dwCpuUsageMin              = 70;
            param.dwCpuUsageMax              = 90;
            param.dwMultipleIoReqsMax        = 1;
            param.dwPort                     = 2599;
            param.dwSocketRecvBufferSize     = -1;
            param.dwSocketSendBufferSize     = -1;
            param.dwConnectionsIdleTimeout   = 500;
            param.dwInitAccepts              = 500;
            param.CALLBACKS.OnConnect        = OnConnect;
            param.CALLBACKS.OnDisconnect     = OnDisconnect;
            param.CALLBACKS.OnRecv           = OnRecv;
            param.CALLBACKS.OnSend           = OnSend;
            param.CALLBACKS.OnError          = OnError;

            DWORD dwRet = PServer.Start(&param);
            if( dwRet != ERROR_SUCCESS ) {
                //	TODO: exception
            }

/*	m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, (ULONG_PTR)0, 0);
	if( m_iocp == NULL )
	{
//		throw new CException(LOGL_FATAL, 0, "Unable to start high-speed network completion IO");
	}

	if( CreateIoCompletionPort((HANDLE)g_Serv.m_SocketMain.GetSocket(), m_iocp, (ULONG_PTR)0, 0) == NULL )
	{
//		throw new CException(LOGL_FATAL, 0, "Unable to start high-speed network completion IO");
	}*/
}

#endif