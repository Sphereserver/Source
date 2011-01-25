// CSocket.h

#ifndef _INC_CSOCKET_H
#define _INC_CSOCKET_H
#pragma once

#include "common.h"

#ifdef _WIN32
	#undef FD_SETSIZE
	#define FD_SETSIZE 1024 // for max of n users ! default = 64

	#include <winsock2.h>
	typedef int socklen_t;
#else	
	// else assume LINUX
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

public:
	CSocketAddressIP();
	explicit CSocketAddressIP( DWORD dwIP );
	explicit CSocketAddressIP( const char *ip );

public:
	DWORD GetAddrIP() const;
	void SetAddrIP( DWORD dwIP );
	LPCTSTR GetAddrStr() const;
	void SetAddrStr( LPCTSTR pszIP );
	bool IsValidAddr() const;
	bool IsLocalAddr() const;

	bool IsSameIP( const CSocketAddressIP & ip ) const;
	bool IsMatchIP( const CSocketAddressIP & ip ) const;

	bool SetHostStruct( const struct hostent * pHost );

	bool SetHostStr( LPCTSTR pszHostName );
	bool operator==( const CSocketAddressIP & ip ) const;
};

struct CSocketAddress : public CSocketAddressIP
{
	// IP plus port.
	// similar to sockaddr_in but without the waste.
	// use this instead.
private:
	WORD m_port;
public:
	CSocketAddress();
	CSocketAddress( in_addr dwIP, WORD uPort );
	CSocketAddress( CSocketAddressIP ip, WORD uPort );
	CSocketAddress( DWORD dwIP, WORD uPort );
	explicit CSocketAddress( const sockaddr_in & SockAddrIn );
	
	bool operator==( const CSocketAddress & SockAddr ) const;
	CSocketAddress & operator = ( const struct sockaddr_in & SockAddrIn );
	bool operator==( const struct sockaddr_in & SockAddrIn ) const;

	// compare to sockaddr_in
	struct sockaddr_in GetAddrPort() const;
	void SetAddrPort( const struct sockaddr_in & SockAddrIn );
	// Just the port.
	WORD GetPort() const;
	void SetPort( WORD wPort );
	void SetPortStr( LPCTSTR pszPort );
	bool SetPortExtStr( TCHAR * pszIP );
	// Port and address together.
	bool SetHostPortStr( LPCTSTR pszIP );
};

class CGSocket
{
private:
	SOCKET  m_hSocket;	// socket connect handle
	
	void Clear();
	
public:
	static const char *m_sClassName;

	CGSocket();
	explicit CGSocket( SOCKET socket );
	~CGSocket();

private:
	CGSocket(const CGSocket& copy);
	CGSocket& operator=(const CGSocket& other);

public:
	static int GetLastError(bool bUseErrno = false);
	bool IsOpen() const;

	void SetSocket(SOCKET socket);
	SOCKET GetSocket() const;

	bool Create();
	bool Create( int iAf, int iType, int iProtocol );
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

	int GetSockName( struct sockaddr_in * pSockAddrIn ) const;
	CSocketAddress GetSockName() const;

	int GetPeerName( struct sockaddr_in * pSockAddrIn ) const;
	CSocketAddress GetPeerName( ) const;

	int SetSockOpt( int nOptionName, const void* optval, int optlen, int nLevel = SOL_SOCKET ) const;
	int GetSockOpt( int nOptionName, void* optval, int * poptlen, int nLevel = SOL_SOCKET ) const;
#ifdef _WIN32
	int IOCtlSocket( long icmd, DWORD * pdwArgs );
	int SendAsync( LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine ) const;
	void ClearAsync();
#else
	int IOCtlSocket( long icmd, int iVal );
	int GetIOCtlSocketFlags( void );
#endif

	void SetNonBlocking(bool bEnable = true);
	void Close();

	static void CloseSocket( SOCKET hClose );
	static short GetProtocolIdByName( LPCTSTR pszName );
};

#endif // _INC_CSOCKET_H
