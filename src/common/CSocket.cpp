#include "../graysvr/graysvr.h"

//		***		***			***
//
//		CSocketAddressIP
//
//		***		***			***

#ifdef _WIN32
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

CSocketAddressIP::CSocketAddressIP()
{
	s_addr = INADDR_BROADCAST;
}
CSocketAddressIP::CSocketAddressIP( DWORD dwIP )
{
	s_addr = dwIP;
}

CSocketAddressIP::CSocketAddressIP(const char *ip)
{
	s_addr = inet_addr(ip);
}

bool CSocketAddressIP::operator==( const CSocketAddressIP & ip ) const
{
	return( IsSameIP( ip ) );
}

DWORD CSocketAddressIP::GetAddrIP() const
{
	return( s_addr );
}

void CSocketAddressIP::SetAddrIP( DWORD dwIP )
{
	s_addr = dwIP;
}

LPCTSTR CSocketAddressIP::GetAddrStr() const
{
	return inet_ntoa( *this );
}

void CSocketAddressIP::SetAddrStr( LPCTSTR pszIP )
{
	// NOTE: This must be in 1.2.3.4 format.
	s_addr = inet_addr( pszIP );
}

bool CSocketAddressIP::IsValidAddr() const
{
	// 0 and 0xffffffff=INADDR_NONE
	return( s_addr != INADDR_ANY && s_addr != INADDR_BROADCAST );
}

bool CSocketAddressIP::IsLocalAddr() const
{
	return( s_addr == 0 || s_addr == SOCKET_LOCAL_ADDRESS );
}

bool CSocketAddressIP::IsMatchIP( const CSocketAddressIP & ip ) const
{
	BYTE		ip1	[4];
	BYTE		ip2	[4];

	memcpy( ip1, (void*) &ip.s_addr,	4 );
	memcpy( ip2, (void*) &s_addr,		4 );

	for ( int i = 0; i < 4; i++ )
	{
		if ( ip1[i] == 255 || ip2[i] == 255 || ip1[i] == ip2[i] )
			continue;
		return false;
	}
	return true;
}

bool CSocketAddressIP::IsSameIP( const CSocketAddressIP & ip ) const
{
	return (ip.s_addr == s_addr);
}

bool CSocketAddressIP::SetHostStruct( const struct hostent * pHost )
{
	// Set the ip from the address name we looked up.
	if ( pHost == NULL ||
		pHost->h_addr_list == NULL ||
		pHost->h_addr == NULL )	// can't resolve the address.
	{
		return( false );
	}
	SetAddrIP( *((DWORD*)( pHost->h_addr ))); // 0.1.2.3
	return true;
}

bool CSocketAddressIP::SetHostStr( LPCTSTR pszHostName )
{
	// try to resolve the host name with DNS for the true ip address.
	if ( pszHostName[0] == '\0' )
		return( false );
	if ( IsDigit( pszHostName[0] ))
	{
		SetAddrStr( pszHostName ); // 0.1.2.3
		return( true );
	}
	// NOTE: This is a blocking call !!!!
	return SetHostStruct( gethostbyname( pszHostName ));
}

//		***		***			***
//
//		CSocketAddress
//
//		***		***			***

CSocketAddress::CSocketAddress()
{
	// s_addr = INADDR_BROADCAST;
	m_port = 0;
}

CSocketAddress::CSocketAddress( in_addr dwIP, WORD uPort )
{
	s_addr = dwIP.s_addr;
	m_port = uPort;
}

CSocketAddress::CSocketAddress( CSocketAddressIP ip, WORD uPort )
{
	s_addr = ip.GetAddrIP();
	m_port = uPort;
}

CSocketAddress::CSocketAddress( DWORD dwIP, WORD uPort )
{
	s_addr = dwIP;
	m_port = uPort;
}

CSocketAddress::CSocketAddress( const sockaddr_in & SockAddrIn )
{
	SetAddrPort( SockAddrIn );
}

bool CSocketAddress::operator==( const CSocketAddress & SockAddr ) const
{
	return( GetAddrIP() == SockAddr.GetAddrIP() && GetPort() == SockAddr.GetPort() );
}

CSocketAddress & CSocketAddress::operator = ( const struct sockaddr_in & SockAddrIn )
{
	SetAddrPort(SockAddrIn);
	return( *this );
}

bool CSocketAddress::operator==( const struct sockaddr_in & SockAddrIn ) const
{
	return( GetAddrIP() == SockAddrIn.sin_addr.s_addr && GetPort() == ntohs( SockAddrIn.sin_port ) );
}

WORD CSocketAddress::GetPort() const
{
	return( m_port );
}

void CSocketAddress::SetPort( WORD wPort )
{
	m_port = wPort;
}

void CSocketAddress::SetPortStr( LPCTSTR pszPort )
{
	m_port = static_cast<WORD>(ATOI(pszPort));
}

bool CSocketAddress::SetPortExtStr( TCHAR * pszIP )
{
	// assume the port is at the end of the line.
	TCHAR * pszPort = strchr( pszIP, ',' );
	if ( pszPort == NULL )
	{
		pszPort = strchr( pszIP, ':' );
		if ( pszPort == NULL )
			return( false );
	}

	SetPortStr( pszPort + 1 );
	*pszPort = '\0';
	return( true );
}

// Port and address together.
bool CSocketAddress::SetHostPortStr( LPCTSTR pszIP )
{
	// NOTE: This is a blocking call !!!!
	TCHAR szIP[256];
	strncpy(szIP, pszIP, sizeof(szIP) - 1);
	SetPortExtStr( szIP );
	return SetHostStr( szIP );
}

// compare to sockaddr_in

struct sockaddr_in CSocketAddress::GetAddrPort() const
{
	struct sockaddr_in SockAddrIn;
	SockAddrIn.sin_family = AF_INET;
	SockAddrIn.sin_port = htons(m_port);
	SockAddrIn.sin_addr.s_addr = s_addr;
	memset(&SockAddrIn.sin_zero, 0, sizeof(SockAddrIn.sin_zero));
	return SockAddrIn;
}
void CSocketAddress::SetAddrPort( const struct sockaddr_in & SockAddrIn )
{
	s_addr = SockAddrIn.sin_addr.s_addr;
	m_port = ntohs( SockAddrIn.sin_port );
}

//		***		***			***
//
//		CGSocket
//
//		***		***			***


CGSocket::CGSocket()
{
	Clear();
}

CGSocket::CGSocket( SOCKET socket )	// accept case.
{
	m_hSocket = socket;
}

CGSocket::~CGSocket()
{
	Close();
}

void CGSocket::SetSocket(SOCKET socket)
{
	Close();
	m_hSocket = socket;
}

void CGSocket::Clear()
{
	// Transfer the socket someplace else.
	m_hSocket = INVALID_SOCKET;
}

int CGSocket::GetLastError(bool bUseErrno)
{
#ifdef _WIN32
	UNREFERENCED_PARAMETER(bUseErrno);
	return( WSAGetLastError() );
#else
	return( !bUseErrno ? h_errno : errno );	// WSAGetLastError()
#endif
}

bool CGSocket::IsOpen() const
{
	return( m_hSocket != INVALID_SOCKET );
}

SOCKET CGSocket::GetSocket() const
{
	return( m_hSocket );
}

bool CGSocket::Create()
{
	return( Create( AF_INET, SOCK_STREAM, IPPROTO_TCP ) );
}

bool CGSocket::Create( int iAf, int iType, int iProtocol )
{
	ASSERT( ! IsOpen());
	m_hSocket = socket( iAf, iType, iProtocol );
	return( IsOpen());
}

int CGSocket::Bind( struct sockaddr_in * pSockAddrIn )
{
	return bind( m_hSocket, reinterpret_cast<struct sockaddr *>(pSockAddrIn), sizeof(*pSockAddrIn));
}

int CGSocket::Bind( const CSocketAddress & SockAddr )
{
	struct sockaddr_in SockAddrIn = SockAddr.GetAddrPort();
	if ( SockAddr.IsLocalAddr())
	{
		SockAddrIn.sin_addr.s_addr = INADDR_ANY;	// use all addresses.
	}
	return( Bind( &SockAddrIn ));
}

int CGSocket::Listen( int iMaxBacklogConnections )
{
	return( listen( m_hSocket, iMaxBacklogConnections ));
}

int CGSocket::Connect( struct sockaddr_in * pSockAddrIn )
{
	// RETURN: 0 = success, else SOCKET_ERROR
	return connect( m_hSocket, reinterpret_cast<struct sockaddr *>(pSockAddrIn), sizeof(*pSockAddrIn));
}

int CGSocket::Connect( const CSocketAddress & SockAddr )
{
	struct sockaddr_in SockAddrIn = SockAddr.GetAddrPort();
	return( Connect( &SockAddrIn ));
}

int CGSocket::Connect( const struct in_addr & ip, WORD wPort )
{
	CSocketAddress SockAddr( ip.s_addr, wPort );
	return( Connect( SockAddr ));
}

int CGSocket::Connect( LPCTSTR pszHostName, WORD wPort )
{
	CSocketAddress SockAddr;
	SockAddr.SetHostStr( pszHostName );
	SockAddr.SetPort( wPort );
	return( Connect( SockAddr ));
}

SOCKET CGSocket::Accept( struct sockaddr_in * pSockAddrIn ) const
{
	int len = sizeof(struct sockaddr_in);
	return accept( m_hSocket, reinterpret_cast<struct sockaddr *>(pSockAddrIn), reinterpret_cast<socklen_t *>(&len));
}

SOCKET CGSocket::Accept( CSocketAddress & SockAddr ) const
{
	// RETURN: Error = hSocketClient < 0 || hSocketClient == INVALID_SOCKET 
	struct sockaddr_in SockAddrIn;
	SOCKET hSocket = Accept( &SockAddrIn );
	SockAddr.SetAddrPort( SockAddrIn );
	return( hSocket );
}

int CGSocket::Send( const void * pData, int len ) const
{
	// RETURN: length sent
	return( send( m_hSocket, static_cast<const char *>(pData), len, 0 ));
}

int CGSocket::Receive( void * pData, int len, int flags )
{
	// RETURN: length, <= 0 is closed or error.
	// flags = MSG_PEEK or MSG_OOB
	return( recv( m_hSocket, static_cast<char *>(pData), len, flags ));
}

int CGSocket::GetSockName( struct sockaddr_in * pSockAddrIn ) const
{
	// Get the address of the near end. (us)
	// RETURN: 0 = success
	int len = sizeof( *pSockAddrIn );
	return( getsockname( m_hSocket, reinterpret_cast<struct sockaddr *>(pSockAddrIn), reinterpret_cast<socklen_t *>(&len) ));
}

CSocketAddress CGSocket::GetSockName() const
{
	struct sockaddr_in SockAddrIn;
	int iRet = GetSockName( &SockAddrIn );
	if ( iRet )
	{
		return( CSocketAddress( INADDR_BROADCAST, 0 ));	// invalid.
	}
	else
	{
		return( CSocketAddress( SockAddrIn ));
	}
}

int CGSocket::GetPeerName( struct sockaddr_in * pSockAddrIn ) const
{
	// Get the address of the far end.
	// RETURN: 0 = success
	int len = sizeof( *pSockAddrIn );
	return( getpeername( m_hSocket, reinterpret_cast<struct sockaddr *>(pSockAddrIn), reinterpret_cast<socklen_t *>(&len) ));
}

CSocketAddress CGSocket::GetPeerName( ) const
{
	struct sockaddr_in SockAddrIn;
	int iRet = GetPeerName( &SockAddrIn );
	if ( iRet )
	{
		return( CSocketAddress( INADDR_BROADCAST, 0 ));	// invalid.
	}
	else
	{
		return( CSocketAddress( SockAddrIn ));
	}
}

int CGSocket::SetSockOpt( int nOptionName, const void * optval, int optlen, int nLevel ) const
{
	// level = SOL_SOCKET and IPPROTO_TCP.
	return( setsockopt( m_hSocket, nLevel, nOptionName, reinterpret_cast<const char FAR *>(optval), optlen ));
}

int CGSocket::GetSockOpt( int nOptionName, void * optval, int * poptlen, int nLevel ) const
{
	return( getsockopt( m_hSocket, nLevel, nOptionName, reinterpret_cast<char FAR *>(optval), reinterpret_cast<socklen_t *>(poptlen)));
}

#ifdef _WIN32
	int CGSocket::IOCtlSocket( long icmd, DWORD * pdwArgs )
	{
		return ioctlsocket( m_hSocket, icmd, pdwArgs );
	}

	int CGSocket::SendAsync( LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine ) const
	{
		 // RETURN: length sent
		 return( WSASend( m_hSocket, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine ));
	}
	
	void CGSocket::ClearAsync()
	{
     	// TO BE CALLED IN CClient destructor !!!
		CancelIo(reinterpret_cast<HANDLE>(m_hSocket));
		SleepEx(1, TRUE);
	}

#else
	int CGSocket::IOCtlSocket( long icmd, int iVal )	// LINUX ?
	{
		return fcntl( m_hSocket, icmd, iVal );
	}

	int CGSocket::GetIOCtlSocketFlags( void )
	{
		return fcntl( m_hSocket, F_GETFL );
	}
#endif

int CGSocket::SetNonBlocking(bool bEnable)
{
#ifdef _WIN32
	u_long uFlags = bEnable ? 1 : 0;		// 0 =  block
	return (ioctlsocket(m_hSocket, FIONBIO, &uFlags) == WSAEINVAL) ? -1 : 0;	// convert Windows result to Linux format
#else
	int iFlags = GetIOCtlSocketFlags();
	if ( bEnable )
		iFlags |= O_NONBLOCK;
	else
		iFlags &= ~O_NONBLOCK;
	return fcntl(m_hSocket, F_SETFL, iFlags);
#endif
}

void CGSocket::Close()
{
	if ( ! IsOpen())
		return;

	CGSocket::CloseSocket( m_hSocket );
	Clear();
}

void CGSocket::CloseSocket( SOCKET hClose )
{
	shutdown( hClose, 2 );
#ifdef _WIN32
	closesocket( hClose );
#else
	close( hClose ); // SD_BOTH
#endif
}

short CGSocket::GetProtocolIdByName( LPCTSTR pszName )
{
	protoent * ppe;

	ppe = getprotobyname(pszName);
	if ( !ppe )
		return 0;

	return ppe->p_proto;
}
