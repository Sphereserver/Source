#ifndef NETWORK_H
#define NETWORK_H

#include "packet.h"
#include "../common/common.h"

/***************************************************************************
 *
 *
 *	class NetState				Network status (client info, etc)
 *
 *
 ***************************************************************************/

class CClient;
struct CSocketAddress;

class NetState
{
public:
	DWORD		m_clientVersion;
	CClient		*m_client;
	int			m_sequence;						// movement sequence

protected:
	long		m_id;
	CGSocket	m_sock;
	bool		m_toClose;
	CSocketAddress	m_peerAddr;
	bool		m_seeded;						// client seeded?
	DWORD		m_seed;							// login game seed
	queue<PacketSend *>	m_queue[PacketSend::PRI_QTY];

public:
	NetState(long id);
	~NetState();

	long id();									// returns ID of the client
	void clear();								// initializes socket

	void init(SOCKET sock, CSocketAddress addr);// initialize socket
	bool isValid(CClient *client = NULL);		// does this socket still belong to this client?

	void markClose();							// mark socket to be closed
	bool toClose();								// the socket should be closed?

	friend class NetworkIn;
	friend class NetworkOut;
	friend class CClient;
	friend class ClientIterator;
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
	long	m_id;
	long	m_max;

public:
	ClientIterator();
	~ClientIterator();

	CClient *next();				// finds next client
};

/***************************************************************************
 *
 *
 *	class NetworkIn				Networking thread used for receiving client data
 *
 *
 ***************************************************************************/

class NetworkIn : public CThread
{
public:
	struct HistoryIP
	{
		CSocketAddressIP	m_ip;
		long				m_pings;
		long				m_connecting;
		long				m_connected;
		bool				m_blocked;
		long				m_ttl;

		void update();
		bool checkPing();			// IP is blocked -or- too many pings to it?
	};
	static const char *m_sClassName;

protected:
	Packet		*m_handlers[0x100];	// packet handlers
	Packet		*m_extended[0x100];	// extended packet handlers
	Packet		*m_encoded[0x100];	// encoded packet handlers
	vector<HistoryIP>	m_ips;
	NetState			**m_states;

	long	getStateSlot(long startFrom = -1);	// finds suitable random slot for client to take

private:
	long		m_lastGivenSlot;				// last slot taken by client

public:
	NetworkIn();

	//	thread specific actions
	static THREAD_ENTRY_RET _cdecl EntryProc(void *lpThreadParameter);
	virtual void Start();
	void pereodically();

	void registerPacket(int packetId, Packet *handler);		// register packet handler
	void registerExtended(int packetId, Packet *handler);	// register extended packet handler
	void registerEncoded(int packetId, Packet *handler);	// register encoded packet handler

	HistoryIP &getHistoryForIP(CSocketAddressIP ip);
	HistoryIP &getHistoryForIP(char *ip);

	Packet *getHandler(BYTE packetID);

protected:
	void checkForData(fd_set *storage);			//	executes network state request for new packets
	void acceptConnection();					//	accepts the connection

	friend class ClientIterator;
};

extern NetworkIn *g_Network;

/***************************************************************************
 *
 *
 *	class NetworkOut			Networking thread used for queued sending outgoing packets
 *
 *
 ***************************************************************************/

class NetworkOut : public CThread
{
public:
	static const char		*m_sClassName;

public:
	NetworkOut();

	//	thread specific actions
	static THREAD_ENTRY_RET _cdecl EntryProc(void *lpThreadParameter);
	virtual void Start();

	void schedule(PacketSend *packet);						// schedule this packet to be sent
	void scheduleOnce(PacketSend *packet);					// schedule this packet to be sent MOVING it to the list

protected:
	void proceedQueue(long priority);
};

extern NetworkOut *g_Sender;

#endif
