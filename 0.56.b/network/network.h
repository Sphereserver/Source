#ifndef __NETWORK_H__
#define __NETWORK_H__
#pragma once

#include "packet.h"
#include "../common/common.h"
#include "../sphere/containers.h"

#define NETWORK_PACKETCOUNT 0x100	// number of unique packets
#define NETWORK_BUFFERSIZE 0xF000	// size of receive buffer
#define NETWORK_SEEDLEN_OLD (sizeof( DWORD ))
#define NETWORK_SEEDLEN_NEW (1 + sizeof( DWORD )*5)

#define NETWORK_MAXPACKETS		g_Cfg.m_iNetMaxPacketsPerTick	// max packets to send per tick (per queue)
#define NETWORK_MAXPACKETLEN	g_Cfg.m_iNetMaxLengthPerTick	// max packet length to send per tick (per queue)
#define NETWORK_MAXQUEUESIZE	g_Cfg.m_iNetMaxQueueSize		// max packets to hold per queue (comment out for unlimited)
#define NETHISTORY_TTL			g_Cfg.m_iNetHistoryTTL			// time to remember an ip
#define NETHISTORY_MAXPINGS		g_Cfg.m_iNetMaxPings			// max 'pings' before blocking an ip

#define NETWORK_DISCONNECTPRI	PacketSend::PRI_HIGHEST			// packet priorty to continue sending before closing sockets

#ifdef DEBUGPACKETS
	#define DEBUGNETWORK(_x_)		if ( g_Cfg.m_wDebugFlags & DEBUGF_NETWORK ) { g_pLog->EventDebug _x_; }
#else
#ifdef NDEBUGPACKETS
	#define DEBUGNETWORK(_x_)
#else
	#define DEBUGNETWORK(_x_)		if ( g_Cfg.m_wDebugFlags & DEBUGF_NETWORK ) { g_pLog->EventDebug _x_; }
#endif
#endif

class CClient;
class NetworkIn;
struct CSocketAddress;

#if defined(_PACKETDUMP) || defined(_DUMPSUPPORT)
	void xRecordPacketData(const CClient* client, const BYTE* data, int length, LPCTSTR heading);
	void xRecordPacket(const CClient* client, Packet* packet, LPCTSTR heading);
#else
	#define xRecordPacketData(_client_, _data_, _length, _heading_)
	#define xRecordPacket(_client_, _packet_, _heading_)
#endif

/***************************************************************************
 *
 *
 *	class NetState				Network status (client info, etc)
 *
 *
 ***************************************************************************/
class NetState
{
protected:
	long m_id; // net id
	CGSocket m_socket; // socket
	CClient* m_client; // client
	CSocketAddress m_peerAddress; // client address

	volatile bool m_isReadClosed; // is closed by read thread
	volatile bool m_isWriteClosed; // is closed by write thread
	volatile bool m_needsFlush; // does data need to be flushed

	bool m_seeded; // is seed received
	DWORD m_seed; // client seed
	bool m_newseed; // is the client using new seed

	bool m_useAsync; // is this socket using asynchronous sends
	volatile bool m_isSendingAsync; // is a packet currently being sent asynchronously?
#ifdef _WIN32
	WSABUF m_bufferWSA; // Winsock Async Buffer
	WSAOVERLAPPED m_overlapped; // Winsock Overlapped structure
#else
	struct ev_io m_eventWatcher;
#endif

	typedef ThreadSafeQueue<PacketSend*> PacketQueue;
	typedef ThreadSafeQueue<PacketTransaction*> PacketTransactionQueue;

	PacketTransactionQueue m_queue[PacketSend::PRI_QTY]; // outgoing packets queue
	PacketQueue m_asyncQueue; // outgoing async packet queue

	PacketTransaction* m_currentTransaction; // transaction currently being processed
	ExtendedPacketTransaction* m_pendingTransaction; // transaction being built

	int m_packetExceptions;

public:
	GAMECLIENT_TYPE m_clientType; // type of client
	DWORD m_clientVersion; // client version (encryption)
	DWORD m_reportedVersion; // client version (reported)
	short m_sequence; // movement sequence

public:
	NetState(long id);
	~NetState(void);

	long id(void) const { return m_id; }; // returns ID of the client
	void setId(long id) { m_id = id; }; // changes ID of the client
	void clear(void); // clears state

	void init(SOCKET socket, CSocketAddress addr); // initialized socket
	bool isValid(const CClient* client = NULL) const; // does this socket still belong to this client?
	bool hasPendingData(void) const; // is there any data waiting to be sent?
	bool canReceive(PacketSend* packet) const; // can the state receive the given packet?

	void setAsyncMode(void); // set asynchronous mode
	bool isAsyncMode(void) const { return m_useAsync; }; // get asyncronous mode
#ifndef _WIN32
	struct ev_io* iocb(void) { return &m_eventWatcher; }; // get io callback
#endif
	bool isSendingAsync(void) const { return m_isSendingAsync; }; // get if async packeet is being sent
	void setSendingAsync(bool isSending) { m_isSendingAsync = isSending; }; // set if async packet is being sent

	GAMECLIENT_TYPE getClientType(void) const { return m_clientType; }; // determined client type
	DWORD getCryptVersion(void) const { return m_clientVersion; }; // version as determined by encryption
	DWORD getReportedVersion(void) const { return m_reportedVersion; }; // version as reported by client

	void markClosed(void); // mark socket as closed by read thread
	void markWriteClosed(void); // mark socket as closed by write thread
	bool isClosing(void) const { return m_isReadClosed || m_isWriteClosed; } // is the socket closing?
	bool isClosed(void) const { return m_isReadClosed && m_isWriteClosed; } // is the socked closed?

	void markFlush(bool needsFlush); // mark socket as needing a flush
	bool needsFlush(void) const { return m_needsFlush; } // does the socket need to be flushed?

	CClient* getClient(void) const { return m_client; } // get linked client

	bool isClient3D(void) const { return m_clientType == CLIENTTYPE_3D; }; // is this a 3D client?
	bool isClientKR(void) const { return m_clientType == CLIENTTYPE_KR; }; // is this a KR client?
	bool isClientSA(void) const { return m_clientType == CLIENTTYPE_SA; }; // is this an SA client?

	bool isCryptVersion(DWORD version) const { return m_clientVersion && m_clientVersion >= version; }; // check the minimum crypt version
	bool isReportedVersion(DWORD version) const { return m_reportedVersion && m_reportedVersion >= version; }; // check the minimum reported verson
	bool isClientVersion(DWORD version) const { return isCryptVersion(version) || isReportedVersion(version); } // check the minimum client version
	bool isCryptLessVersion(DWORD version) const { return m_clientVersion && m_clientVersion < version; }; // check the maximum crypt version
	bool isReportedLessVersion(DWORD version) const { return m_reportedVersion && m_reportedVersion < version; }; // check the maximum reported version
	bool isClientLessVersion(DWORD version) const { return isCryptLessVersion(version) || isReportedLessVersion(version); } // check the maximum client version

	void beginTransaction(long priority); // begin a transaction for grouping packets
	void endTransaction(void); // end transaction
	
	friend class NetworkIn;
	friend class NetworkOut;
	friend class CClient;
	friend class ClientIterator;
	friend class SafeClientIterator;
#ifndef _WIN32
	friend class LinuxEv;
#endif
};


/***************************************************************************
 *
 *
 *	class ClientIterator		Works as client iterator getting the clients
 *
 *
 ***************************************************************************/
class ClientIterator
{
protected:
	const NetworkIn* m_network;
	CClient* m_nextClient;

public:
	ClientIterator(const NetworkIn* network = NULL);
	~ClientIterator(void);

	CClient* next(bool includeClosing = false); // finds next client
};


/***************************************************************************
 *
 *
 *	class SafeClientIterator		Works as client iterator getting the clients in a thread-safe way
 *
 *
 ***************************************************************************/
class SafeClientIterator
{
protected:
	const NetworkIn* m_network;
	int m_id;
	int m_max;

public:
	SafeClientIterator(const NetworkIn* network = NULL);
	~SafeClientIterator(void);

	CClient* next(bool includeClosing = false); // finds next client
};


/***************************************************************************
 *
 *
 *	class NetworkIn				Networking thread used for receiving client data
 *
 *
 ***************************************************************************/
class NetworkIn : public AbstractSphereThread
{
public:
	struct HistoryIP
	{
		CSocketAddressIP m_ip;
		long m_pings;
		long m_connecting;
		long m_connected;
		bool m_blocked;
		long m_ttl;
		CServTime m_blockExpire;

		void update(void);
		bool checkPing(void); // IP is blocked -or- too many pings to it?
		void setBlocked(bool isBlocked, int timeout = -1);
	};

private:
	long m_lastGivenSlot; // last slot taken by client
	Packet* m_handlers[NETWORK_PACKETCOUNT]; // standard packet handlers
	Packet* m_extended[NETWORK_PACKETCOUNT]; // extended packeet handlers (0xbf)
	Packet* m_encoded[NETWORK_PACKETCOUNT]; // encoded packet handlers (0xd7)
	vector<HistoryIP> m_ips;

	BYTE* m_buffer; // receive buffer
	BYTE* m_decryptBuffer; // receive buffer for decryption

protected:
	NetState** m_states; // client state pool
	long m_stateCount; // client state count
	CGObList m_clients; // current list of clients (CClient)

public:
	static const char* m_sClassName;

	NetworkIn(void);
	~NetworkIn(void);

	virtual void onStart(void);
	virtual void tick(void);
	virtual void waitForClose(void);

	HistoryIP &getHistoryForIP(CSocketAddressIP ip);
	HistoryIP &getHistoryForIP(const char* ip);

	void registerPacket(int packetId, Packet* handler); // register packet handler
	void registerExtended(int packetId, Packet* handler); // register extended packet handler
	void registerEncoded(int packetId, Packet* handler); // register encoded packet handler

	Packet* getHandler(int packetId) const; // get handler for standard packet
	Packet* getExtendedHandler(int packetId) const; // get handler for extended packet
	Packet* getEncodedHandler(int packetId) const; // get handler for encoded packet

	void acceptConnection(void); // accepts a new connection

protected:
	int checkForData(fd_set* storage); // executes network state request for new packets
	long getStateSlot(long startFrom = -1); // finds suitable random slot for client to take
	void periodic(void); // performs periodic actions
	void defragSlots(long fromSlot = 0); // moves used network slots to front

	friend class ClientIterator;
	friend class SafeClientIterator;
};


/***************************************************************************
 *
 *
 *	class NetworkOut			Networking thread used for queued sending outgoing packets
 *
 *
 ***************************************************************************/
class NetworkOut : public AbstractSphereThread
{
private:
	BYTE* m_encryptBuffer; // buffer for encryption

public:
	static const char* m_sClassName;

	NetworkOut(void);
	~NetworkOut(void);

	virtual void tick(void);
	virtual void waitForClose(void);

	void schedule(const PacketSend* packet, bool appendTransaction); // schedule this packet to be sent
	void scheduleOnce(PacketSend* packet, bool appendTransaction); // schedule this packet to be sent MOVING it to the list
	void scheduleOnce(PacketTransaction* transaction); // schedule a transaction to be sent

	void flush(CClient* client); // forces immediate send of all packets

protected:
	int proceedQueue(CClient* client, long priority); // send next set of packets with the specified priority (returns number of packets sent)
	int proceedQueueAsync(CClient* client); // send next set of asynchronous packets (returns number of packets sent, 1 max)
	void proceedFlush(void); // flush data to pending sockets
	bool sendPacket(CClient* client, PacketSend* packet); // send packet to a client
	bool sendPacketNow(CClient* client, PacketSend* packet); // send packet to a client now

public:
	void onAsyncSendComplete(CClient* client); // handle completion of async send

	friend class SimplePacketTransaction;
	friend class ExtendedPacketTransaction;
};

extern NetworkIn g_NetworkIn;
extern NetworkOut g_NetworkOut;

#endif
