#include "../graysvr/graysvr.h"

///////////////////////////////////////////////////////////
// CSocketAddressIP

void CSocketAddressIP::SetHostStr(LPCTSTR pszHostName)
{
	if ( !pszHostName )
		return;

	// Try to resolve IP address
	if ( IsDigit(pszHostName[0]) )
		return SetAddrStr(pszHostName);

	// Try to resolve hostname
	struct hostent *pHost = gethostbyname(pszHostName);
	if ( pHost && pHost->h_addr )
		SetAddrIP(*reinterpret_cast<DWORD *>(pHost->h_addr));
}

///////////////////////////////////////////////////////////
// CSocketAddress

struct sockaddr_in CSocketAddress::GetAddrPort() const
{
	struct sockaddr_in sockAddrIn;
	sockAddrIn.sin_family = AF_INET;
	sockAddrIn.sin_port = htons(m_port);
	sockAddrIn.sin_addr.s_addr = s_addr;
	memset(&sockAddrIn.sin_zero, 0, sizeof(sockAddrIn.sin_zero));
	return sockAddrIn;
}

void CSocketAddress::SetAddrPort(const struct sockaddr_in &sockAddrIn)
{
	s_addr = sockAddrIn.sin_addr.s_addr;
	m_port = ntohs(sockAddrIn.sin_port);
}

void CSocketAddress::SetPortExtStr(TCHAR *pszIP)
{
	// Assume the port is at the end of the line
	TCHAR *pszPort = strchr(pszIP, ',');
	if ( !pszPort )
	{
		pszPort = strchr(pszIP, ':');
		if ( !pszPort )
			return;
	}

	SetPortStr(pszPort + 1);
}

void CSocketAddress::SetHostPortStr(LPCTSTR pszIP)
{
	// NOTE: This is a blocking call
	TCHAR szTemp[256];
	strncpy(szTemp, pszIP, sizeof(szTemp) - 1);
	SetPortExtStr(szTemp);
	SetHostStr(szTemp);
}

///////////////////////////////////////////////////////////
// CGSocket

int CGSocket::GetLastError(bool fUseErrno)
{
#ifdef _WIN32
	UNREFERENCED_PARAMETER(fUseErrno);
	return WSAGetLastError();
#else
	return fUseErrno ? errno : h_errno;
#endif
}

bool CGSocket::Create()
{
	return Create(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

bool CGSocket::Create(int iAf, int iType, int iProtocol)
{
	ASSERT(!IsOpen());
	m_hSocket = socket(iAf, iType, iProtocol);
	return IsOpen();
}

int CGSocket::Bind(struct sockaddr_in *pSockAddrIn)
{
	return bind(m_hSocket, reinterpret_cast<struct sockaddr *>(pSockAddrIn), sizeof(*pSockAddrIn));
}

int CGSocket::Bind(const CSocketAddress &sockAddr)
{
	struct sockaddr_in sockAddrIn = sockAddr.GetAddrPort();
	if ( sockAddr.IsLocalAddr() )
		sockAddrIn.sin_addr.s_addr = INADDR_ANY;	// use all addresses

	return Bind(&sockAddrIn);
}

int CGSocket::Listen(int iMaxBacklogConnections)
{
	return listen(m_hSocket, iMaxBacklogConnections);
}

int CGSocket::Connect(struct sockaddr_in *pSockAddrIn)
{
	// RETURN: 0 = success, else SOCKET_ERROR
	return connect(m_hSocket, reinterpret_cast<struct sockaddr *>(pSockAddrIn), sizeof(*pSockAddrIn));
}

int CGSocket::Connect(const CSocketAddress &sockAddr)
{
	struct sockaddr_in sockAddrIn = sockAddr.GetAddrPort();
	return Connect(&sockAddrIn);
}

SOCKET CGSocket::Accept(struct sockaddr_in *pSockAddrIn) const
{
	int len = sizeof(struct sockaddr_in);
	return accept(m_hSocket, reinterpret_cast<struct sockaddr *>(pSockAddrIn), reinterpret_cast<socklen_t *>(&len));
}

SOCKET CGSocket::Accept(CSocketAddress &sockAddr) const
{
	// RETURN: Error = sock < 0 or INVALID_SOCKET 
	struct sockaddr_in sockAddrIn;
	SOCKET sock = Accept(&sockAddrIn);
	sockAddr.SetAddrPort(sockAddrIn);
	return sock;
}

int CGSocket::Send(const void *pBuffer, int iLen) const
{
	// RETURN: length sent
	return send(m_hSocket, static_cast<const char *>(pBuffer), iLen, 0);
}

int CGSocket::Receive(void *pBuffer, int iLen, int iFlags)
{
	// RETURN: length received (<= 0 is closed or error)
	// ARGS: iFlags = MSG_PEEK or MSG_OOB
	return recv(m_hSocket, static_cast<char *>(pBuffer), iLen, iFlags);
}

int CGSocket::GetSockName(struct sockaddr_in *pSockAddrIn) const
{
	// Get the address of the near end (us)
	// RETURN: 0 = success
	int len = sizeof(*pSockAddrIn);
	return getsockname(m_hSocket, reinterpret_cast<struct sockaddr *>(pSockAddrIn), reinterpret_cast<socklen_t *>(&len));
}

CSocketAddress CGSocket::GetSockName() const
{
	struct sockaddr_in sockAddrIn;
	if ( GetSockName(&sockAddrIn) == 0 )
		return CSocketAddress(sockAddrIn);
	else
		return CSocketAddress(INADDR_BROADCAST, 0);		// invalid		
}

int CGSocket::SetSockOpt(int iOptName, const void *pOptVal, int iOptLen, int iLevel) const
{
	// ARGS: iLevel = SOL_SOCKET or IPPROTO_TCP
	return setsockopt(m_hSocket, iLevel, iOptName, reinterpret_cast<const char FAR *>(pOptVal), iOptLen);
}

int CGSocket::GetSockOpt(int iOptName, void *pOptVal, int *iOptLen, int iLevel) const
{
	return getsockopt(m_hSocket, iLevel, iOptName, reinterpret_cast<char FAR *>(pOptVal), reinterpret_cast<socklen_t *>(iOptLen));
}

int CGSocket::SetNonBlocking(bool fEnable)
{
#ifdef _WIN32
	u_long ulFlags = static_cast<u_long>(fEnable);
	return ioctlsocket(m_hSocket, FIONBIO, &ulFlags);
#else
	int iFlags = fcntl(m_hSocket, F_GETFL);
	if ( fEnable )
		iFlags |= O_NONBLOCK;
	else
		iFlags &= ~O_NONBLOCK;
	return fcntl(m_hSocket, F_SETFL, iFlags);
#endif
}

void CGSocket::Close()
{
	if ( !IsOpen() )
		return;

	CGSocket::CloseSocket(m_hSocket);
	Clear();
}

void CGSocket::CloseSocket(SOCKET sock)
{
#ifdef _WIN32
	shutdown(sock, SD_BOTH);
	closesocket(sock);
#else
	shutdown(sock, SHUT_RDWR);
	close(sock);
#endif
}

short CGSocket::GetProtocolIdByName(LPCTSTR pszName)
{
	protoent *ppe = getprotobyname(pszName);
	return ppe ? ppe->p_proto : 0;
}

#ifdef _WIN32
int CGSocket::SendAsync(LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) const
{
	// RETURN: length sent
	return WSASend(m_hSocket, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);
}

void CGSocket::ClearAsync()
{
	// TO BE CALLED IN CClient destructor !!!
	CancelIo(reinterpret_cast<HANDLE>(m_hSocket));
	SleepEx(1, TRUE);
}
#endif
