// CSocket.h

#ifndef _INC_CSOCKET_H
#define _INC_CSOCKET_H
#pragma once

#include "common.h"
#include "exceptions.h"

#ifdef _WIN32
#include <winsock.h>
typedef int socklen_t;
#else

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>

// Compatibility stuff.
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR    (-1)
#define SOCKET			int
#define TCP_NODELAY		0x0001

#endif	// _WIN32

struct CSocketAddressIP : public in_addr
{
	// Just the ip address. Not the port.
#define SOCKET_LOCAL_ADDRESS 0x0100007f
	// INADDR_ANY              (u_long)0x00000000
	// INADDR_LOOPBACK         0x7f000001
	// INADDR_BROADCAST        (u_long)0xffffffff
	// INADDR_NONE             0xffffffff

	DWORD GetAddrIP() const;
	void SetAddrIP( DWORD dwIP );
	LPCTSTR GetAddrStr() const;
	void SetAddrStr( LPCTSTR pszIP );
	bool IsValidAddr() const;
	bool IsLocalAddr() const;
	bool IsSameIP( const CSocketAddressIP & ip ) const;

	bool SetHostStruct( const struct hostent * pHost );
	bool SetHostStr( LPCTSTR pszHostName );
	bool operator==( CSocketAddressIP ip ) const;
	CSocketAddressIP();
	CSocketAddressIP(DWORD ip);
	CSocketAddressIP(char *ip);
};

struct CSocketAddress : public CSocketAddressIP
{
	// IP plus port.
private:
	WORD m_port;
public:
	WORD GetPort() const;
	void SetPort( WORD wPort );
	void SetPortStr( LPCTSTR pszPort );
	bool SetPortExtStr( TCHAR * pszIP );
	bool SetHostPortStr( LPCTSTR pszIP );

	CSocketAddress( in_addr dwIP, WORD uPort );
	CSocketAddress( CSocketAddressIP ip, WORD uPort );
	CSocketAddress( DWORD dwIP, WORD uPort );
	CSocketAddress();
	CSocketAddress( const sockaddr_in & SockAddrIn );

	struct sockaddr_in GetAddrPort() const;
	void SetAddrPort( const struct sockaddr_in & SockAddrIn );
	CSocketAddress & operator = ( const struct sockaddr_in & SockAddrIn );
	bool operator==( const struct sockaddr_in & SockAddrIn ) const;
	bool operator==( const CSocketAddress & SockAddr ) const;
};

class CGSocket
{
private:
	SOCKET  m_hSocket;	// socket connect handle
private:
	void Clear();
public:
	static const char *m_sClassName;
	CGSocket();
	CGSocket(SOCKET socket);	// accept case.
	static int GetLastError();
	bool IsOpen() const;
	SOCKET GetSocket() const;

	bool Create();
	int Bind( struct sockaddr_in * pSockAddrIn );
	int Bind( const CSocketAddress & SockAddr );
	int Listen( int iMaxBacklogConnections = SOMAXCONN );
	int Connect( struct sockaddr_in * pSockAddrIn );
	int Connect( const CSocketAddress & SockAddr );
	int Connect( const struct in_addr ip, WORD wPort );
	int Connect( LPCTSTR pszHostName, WORD wPort );
	SOCKET Accept( struct sockaddr_in * pSockAddrIn ) const;
	SOCKET Accept( CSocketAddress & SockAddr ) const;
	int Send( const void * pData, int len ) const;
	int Receive( void * pData, int len, int flags = 0 );

	int SetSockOpt( int nOptionName, const void* optval, int optlen, int nLevel = SOL_SOCKET ) const;
	int GetSockOpt( int nOptionName, void* optval, int * poptlen, int nLevel = SOL_SOCKET ) const;

	void SetNonBlocking();

	CGSocket &operator=(SOCKET h);

	void Close();
	~CGSocket();
};

#endif // _INC_CSOCKET_H
