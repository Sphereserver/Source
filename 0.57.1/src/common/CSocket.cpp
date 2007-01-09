#include "../graysvr.h"
#include "../network/network.h"

//		***		***			***
//
//		CSocketAddressIP
//
//		***		***			***

DWORD CSocketAddressIP::GetAddrIP() const
{
	return s_addr;
}

void CSocketAddressIP::SetAddrIP(DWORD dwIP)
{
	s_addr = dwIP;
}

LPCTSTR CSocketAddressIP::GetAddrStr() const
{
	return inet_ntoa(*this);
}

void CSocketAddressIP::SetAddrStr(LPCTSTR pszIP)
{
	// NOTE: This must be in 1.2.3.4 format.
	s_addr = inet_addr(pszIP);
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

bool CSocketAddressIP::IsSameIP(const CSocketAddressIP &ip) const
{
	return (ip.s_addr == s_addr);
}

bool CSocketAddressIP::SetHostStruct(const struct hostent *pHost)
{
	// Set the ip from the address name we looked up.
	if ( !pHost || !pHost->h_addr_list || !pHost->h_addr )	// can't resolve the address.
		return false;
	SetAddrIP(*((DWORD*)(pHost->h_addr))); // 0.1.2.3
	return true;
}

bool CSocketAddressIP::SetHostStr(LPCTSTR pszHostName)
{
	// try to resolve the host name with DNS for the true ip address.
	if ( pszHostName[0] == '\0' )
		return false;

	if ( isdigit(pszHostName[0]) )
	{
		SetAddrStr(pszHostName); // 0.1.2.3
		return true;
	}
	// NOTE: This is a blocking call !!!!
	return SetHostStruct(gethostbyname(pszHostName));
}

bool CSocketAddressIP::operator==(CSocketAddressIP ip) const
{
	return IsSameIP(ip);
}

CSocketAddressIP::CSocketAddressIP()
{
	s_addr = INADDR_BROADCAST;
}

CSocketAddressIP::CSocketAddressIP(DWORD ip)
{
	s_addr = ip;
}

CSocketAddressIP::CSocketAddressIP(char *ip)
{
	s_addr = inet_addr(ip);
}

//		***		***			***
//
//		CSocketAddress
//
//		***		***			***

WORD CSocketAddress::GetPort() const
{
	return m_port;
}

void CSocketAddress::SetPort(WORD wPort)
{
	m_port = wPort;
}

void CSocketAddress::SetPortStr(LPCTSTR pszPort)
{
	m_port = (WORD)ATOI(pszPort);
}

bool CSocketAddress::SetPortExtStr(TCHAR *pszIP)
{
	// assume the port is at the end of the line.
	TCHAR *pszPort = strchr(pszIP, ',');
	if ( !pszPort )
	{
		pszPort = strchr(pszIP, ':');
		if ( !pszPort )
			return false;
	}

	SetPortStr(pszPort + 1);
	*pszPort = '\0';
	return true;
}

bool CSocketAddress::SetHostPortStr(LPCTSTR pszIP)
{
	// NOTE: This is a blocking call !!!!
	TCHAR szIP[256];
	strncpy(szIP, pszIP, sizeof(szIP));
	SetPortExtStr(szIP);
	return SetHostStr(szIP);
}

bool CSocketAddress::operator==(const CSocketAddress &SockAddr) const
{
	return ( GetAddrIP() == SockAddr.GetAddrIP() && GetPort() == SockAddr.GetPort() );
}

CSocketAddress::CSocketAddress(in_addr dwIP, WORD uPort)
{
	s_addr = dwIP.s_addr;
	m_port = uPort;
}

CSocketAddress::CSocketAddress(CSocketAddressIP ip, WORD uPort)
{
	s_addr = ip.GetAddrIP();
	m_port = uPort;
}

CSocketAddress::CSocketAddress(DWORD dwIP, WORD uPort)
{
	s_addr = dwIP;
	m_port = uPort;
}

CSocketAddress::CSocketAddress()
{
	// s_addr = INADDR_BROADCAST;
	m_port = 0;
}

struct sockaddr_in CSocketAddress::GetAddrPort() const
{
	struct sockaddr_in SockAddrIn;
	SockAddrIn.sin_family = AF_INET;
	SockAddrIn.sin_addr.s_addr = s_addr;
	SockAddrIn.sin_port = htons(m_port);
	return SockAddrIn;
}

void CSocketAddress::SetAddrPort(const struct sockaddr_in &SockAddrIn)
{
	s_addr = SockAddrIn.sin_addr.s_addr;
	m_port = ntohs(SockAddrIn.sin_port);
}

CSocketAddress &CSocketAddress::operator = (const struct sockaddr_in &SockAddrIn)
{
	SetAddrPort(SockAddrIn);
	return *this;
}

bool CSocketAddress::operator==(const struct sockaddr_in& SockAddrIn) const
{
	return ( GetAddrIP() == SockAddrIn.sin_addr.s_addr && GetPort() == ntohs(SockAddrIn.sin_port) );
}

CSocketAddress::CSocketAddress(const sockaddr_in &SockAddrIn)
{
	SetAddrPort(SockAddrIn);
}

//		***		***			***
//
//		CGSocket
//
//		***		***			***

void CGSocket::Clear()
{
	// Transfer the socket someplace else.
	m_hSocket = INVALID_SOCKET;
}

CGSocket::CGSocket()
{
	Clear();
}

CGSocket::CGSocket(SOCKET socket)
{
	m_hSocket = socket;
}

CGSocket::~CGSocket()
{
	Close();
}

bool CGSocket::IsOpen() const
{
	return ( m_hSocket != INVALID_SOCKET );
}

SOCKET CGSocket::GetSocket() const
{
	return m_hSocket;
}

bool CGSocket::Create()
{
	m_hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	return IsOpen();
}

int CGSocket::Bind(struct sockaddr_in *pSockAddrIn)
{
	return bind(m_hSocket, (struct sockaddr *)pSockAddrIn, sizeof(*pSockAddrIn));
}

int CGSocket::Bind(const CSocketAddress &SockAddr)
{
	struct sockaddr_in SockAddrIn = SockAddr.GetAddrPort();
	if ( SockAddr.IsLocalAddr() )
		SockAddrIn.sin_addr.s_addr = INADDR_ANY;	// use all addresses.
	return Bind(&SockAddrIn);
}

int CGSocket::Listen(int iMaxBacklogConnections)
{
	return listen(m_hSocket, iMaxBacklogConnections);
}

int CGSocket::Connect(struct sockaddr_in *pSockAddrIn)
{
	// RETURN: 0 = success, else SOCKET_ERROR
	return connect(m_hSocket, (struct sockaddr*)pSockAddrIn, sizeof(*pSockAddrIn));
}

int CGSocket::Connect(const CSocketAddress &SockAddr)
{
	struct sockaddr_in SockAddrIn = SockAddr.GetAddrPort();
	return Connect(&SockAddrIn);
}

int CGSocket::Connect(const struct in_addr ip, WORD wPort)
{
	CSocketAddress SockAddr(ip.s_addr, wPort);
	return Connect(SockAddr);
}

int CGSocket::Connect(LPCTSTR pszHostName, WORD wPort)
{
	CSocketAddress SockAddr;
	SockAddr.SetHostStr(pszHostName);
	SockAddr.SetPort(wPort);
	return Connect(SockAddr);
}

SOCKET CGSocket::Accept(struct sockaddr_in *pSockAddrIn) const
{
	int len = sizeof(struct sockaddr_in);
	return accept(m_hSocket, (struct sockaddr*)pSockAddrIn, (socklen_t*)&len);
}

SOCKET CGSocket::Accept(CSocketAddress &SockAddr) const
{
	// RETURN: Error = hSocketClient < 0 || hSocketClient == INVALID_SOCKET 
	struct sockaddr_in SockAddrIn;
	SOCKET hSocket = Accept(&SockAddrIn);
	SockAddr.SetAddrPort(SockAddrIn);
	return hSocket;
}

int CGSocket::Send(const void *pData, int len) const
{
	// RETURN: length sent
	return send(m_hSocket, (char*)pData, len, 0);
}

int CGSocket::Receive(void *pData, int len, int flags)
{
	// RETURN: length, <= 0 is closed or error.
	// flags = MSG_PEEK or MSG_OOB
	return recv(m_hSocket, (char*)pData, len, flags);
}

int CGSocket::SetSockOpt(int nOptionName, const void* optval, int optlen, int nLevel) const
{
	// level = SOL_SOCKET and IPPROTO_TCP.
	return setsockopt(m_hSocket, nLevel, nOptionName, (const char FAR *)optval, optlen);
}

int CGSocket::GetSockOpt(int nOptionName, void* optval, int *poptlen, int nLevel) const
{
	return getsockopt(m_hSocket, nLevel, nOptionName, (char FAR *)optval, (socklen_t*)poptlen);
}

void CGSocket::SetNonBlocking()
{
#ifdef _WIN32
	DWORD lVal = 1;	// 0 =  block
	ioctlsocket(m_hSocket, FIONBIO, &lVal);
#else
	fcntl(m_hSocket, F_SETFL, fcntl(m_hSocket, F_GETFL)|O_NONBLOCK);
#endif
}

void CGSocket::Close()
{
	if ( !IsOpen() )
		return;
	shutdown(m_hSocket, 2);
#ifdef _WIN32
	closesocket(m_hSocket);
#else
	close(m_hSocket);		// LINUX i assume. SD_BOTH
#endif
	Clear();
}

int CGSocket::GetLastError()
{
#ifdef _WIN32
	return WSAGetLastError();
#else
	return h_errno;
#endif
}

CGSocket &CGSocket::operator=(SOCKET h)
{
	m_hSocket = h;
	return *this;
}
