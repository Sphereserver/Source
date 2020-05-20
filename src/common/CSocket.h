#ifndef _INC_CSOCKET_H
#define _INC_CSOCKET_H
#pragma once

#ifdef _WIN32
	#undef FD_SETSIZE
	#define FD_SETSIZE 1024		// for max of N users (default = 64)
	#include <WS2tcpip.h>
#else
	#include <arpa/inet.h>
	#include <fcntl.h>
	#include <netdb.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <signal.h>
	#include <sys/socket.h>
	#include <unistd.h>

	// Compatibility stuff
	typedef int				SOCKET;
	#define INVALID_SOCKET	(SOCKET)(~0)
	#define SOCKET_ERROR	(-1)
#endif

#define INADDR_LOOPBACK_REVERSE 0x100007f	// reversed INADDR_LOOPBACK (0x7f000001 = 127.0.0.1)

struct CSocketAddressIP : public in_addr
{
	// Store IP without port
public:
	CSocketAddressIP()
	{
		s_addr = INADDR_BROADCAST;
	}
	explicit CSocketAddressIP(DWORD dwIP)
	{
		s_addr = dwIP;
	}
	explicit CSocketAddressIP(const char *ip)
	{
		s_addr = inet_addr(ip);
	}

public:
	void SetHostStr(LPCTSTR pszHostName);

	LPCTSTR GetAddrStr() const
	{
		return inet_ntoa(*this);
	}
	void SetAddrStr(LPCTSTR pszIP)
	{
		// IP must be in IPv4 format (x.x.x.x)
		s_addr = inet_addr(pszIP);
	}

	DWORD GetAddrIP() const
	{
		return s_addr;
	}
	void SetAddrIP(DWORD dwIP)
	{
		s_addr = dwIP;
	}

	bool IsValidAddr() const
	{
		return ((s_addr != INADDR_ANY) && (s_addr != INADDR_BROADCAST));
	}
	bool IsLocalAddr() const
	{
		return ((s_addr == INADDR_ANY) || (s_addr == INADDR_LOOPBACK_REVERSE));
	}
	bool IsSameIP(const CSocketAddressIP &ip) const
	{
		return (ip.s_addr == s_addr);
	}

	bool operator==(const CSocketAddressIP &ip) const
	{
		return IsSameIP(ip);
	}
};

struct CSocketAddress : public CSocketAddressIP
{
	// Store IP + port
	// Similar to sockaddr_in, but without the waste, so use this instead
public:
	CSocketAddress()
	{
		//s_addr = INADDR_BROADCAST;
		m_port = 0;
	}
	CSocketAddress(in_addr dwIP, WORD wPort)
	{
		s_addr = dwIP.s_addr;
		m_port = wPort;
	}
	CSocketAddress(CSocketAddressIP ip, WORD wPort)
	{
		s_addr = ip.GetAddrIP();
		m_port = wPort;
	}
	CSocketAddress(DWORD dwIP, WORD wPort)
	{
		s_addr = dwIP;
		m_port = wPort;
	}
	explicit CSocketAddress(const sockaddr_in &sockAddrIn)
	{
		SetAddrPort(sockAddrIn);
	}

private:
	WORD m_port;

public:
	struct sockaddr_in GetAddrPort() const;
	void SetAddrPort(const struct sockaddr_in &sockAddrIn);

	// Just the port
	WORD GetPort() const
	{
		return m_port;
	}
	void SetPortNum(WORD wPort)
	{
		m_port = wPort;
	}
	void SetPortStr(LPCTSTR pszPort)
	{
		m_port = static_cast<WORD>(ATOI(pszPort));
	}
	void SetPortExtStr(TCHAR *pszIP);

	// Host + port
	void SetHostPortStr(LPCTSTR pszIP);

	bool operator==(const CSocketAddress &SockAddr) const
	{
		return ((GetAddrIP() == SockAddr.GetAddrIP()) && (GetPort() == SockAddr.GetPort()));
	}
	bool operator==(const struct sockaddr_in &sockAddrIn) const
	{
		return ((GetAddrIP() == sockAddrIn.sin_addr.s_addr) && (GetPort() == ntohs(sockAddrIn.sin_port)));
	}
	CSocketAddress &operator=(const struct sockaddr_in &sockAddrIn)
	{
		SetAddrPort(sockAddrIn);
		return *this;
	}
};

class CGSocket
{
public:
	static const char *m_sClassName;

	CGSocket()
	{
		Clear();
	}
	explicit CGSocket(SOCKET hSocket)
	{
		m_hSocket = hSocket;
	}
	~CGSocket()
	{
		Close();
	}

private:
	SOCKET m_hSocket;	// socket connect handle

	void Clear()
	{
		m_hSocket = INVALID_SOCKET;
	}

public:
	static int GetLastError(bool fUseErrno = false);
	bool IsOpen() const
	{
		return (m_hSocket != INVALID_SOCKET);
	}

	void SetSocket(SOCKET hSocket)
	{
		Close();
		m_hSocket = hSocket;
	}
	SOCKET GetSocket() const
	{
		return m_hSocket;
	}

	bool Create();
	bool Create(int iAf, int iType, int iProtocol);

	int Bind(struct sockaddr_in *pSockAddrIn);
	int Bind(const CSocketAddress &SockAddr);

	int Listen(int iMaxBacklogConnections = SOMAXCONN);

	int Connect(struct sockaddr_in *pSockAddrIn);
	int Connect(const CSocketAddress &SockAddr);

	SOCKET Accept(struct sockaddr_in *pSockAddrIn) const;
	SOCKET Accept(CSocketAddress &SockAddr) const;

	int Send(const void *pBuffer, int iLen) const;
	int Receive(void *pBuffer, int iLen, int iFlags = 0);

	int GetSockName(struct sockaddr_in *pSockAddrIn) const;
	CSocketAddress GetSockName() const;

	int SetSockOpt(int iOptName, const void *pOptVal, int iOptLen, int iLevel = SOL_SOCKET) const;
	int GetSockOpt(int iOptName, void *pOptVal, int *iOptLen, int iLevel = SOL_SOCKET) const;

	int SetNonBlocking(bool fEnable = true);
	void Close();

	static void CloseSocket(SOCKET hSocket);
	static short GetProtocolIdByName(LPCTSTR pszName);

#ifdef _WIN32
	int SendAsync(LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) const;
	void ClearAsync();
#endif

private:
	CGSocket(const CGSocket &copy);
	CGSocket &operator=(const CGSocket &other);
};

#endif	// _INC_CSOCKET_H
