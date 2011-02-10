#ifndef __PACKET_H__
#define __PACKET_H__
#pragma once

#include "../common/common.h"
#include "../graysvr/graysvr.h"

class NetState;
class SimplePacketTransaction;

#define PACKET_BUFFERDEFAULT 4
#define PACKET_BUFFERGROWTH 4

/***************************************************************************
 *
 *
 *	class Packet				Base packet class for both sending/receiving
 *
 *
 ***************************************************************************/
class Packet
{
protected:
	BYTE* m_buffer;				// raw data
	size_t m_bufferSize;		// size of raw data

	size_t m_length;			// length of packet
	size_t m_position;			// current position in packet
	size_t m_expectedLength;	// expected length of this packet (0 = dynamic)

public:
	explicit Packet(size_t size = 0);
	Packet(const Packet& other);
	Packet(const BYTE* data, size_t size);
	virtual ~Packet(void);

private:
	Packet& operator=(const Packet& other);

public:
	bool isValid(void) const;
	size_t getLength(void) const;
	size_t getPosition(void) const;
	BYTE* getData(void) const;
	void dump(AbstractString& output) const;

	void expand(size_t size = 0); // expand packet (resize whilst maintaining position)
	void resize(size_t newsize); // resize packet
	void seek(size_t pos = 0); // seek to position
	void skip(long count = 1); // skip count bytes

	BYTE &operator[](size_t index);
	const BYTE &operator[](size_t index) const;

	// write
	void writeBool(const bool value); // write boolean (1 byte)
	void writeCharASCII(const char value); // write ASCII character (1 byte)
	void writeCharUNICODE(const WCHAR value); // write UNICODE character (2 bytes)
	void writeCharNUNICODE(const WCHAR value); // write UNICODE character, network order (2 bytes)
	void writeByte(const BYTE value); // write 8-bit integer (1 byte)
	void writeInt16(const WORD value); // write 16-bit integer (2 bytes)
	void writeInt32(const DWORD value); // write 32-bit integer (4 bytes)
	void writeInt64(const DWORD hi, const DWORD lo); // write 64-bit integer (8 bytes)
	void writeStringASCII(const char* value, bool terminate = true); // write ascii string until null terminator found
	void writeStringASCII(const WCHAR* value, bool terminate = true); // write ascii string until null terminator found
	void writeStringFixedASCII(const char* value, size_t size, bool terminate = false); // write fixed-length ascii string
	void writeStringFixedASCII(const WCHAR* value, size_t size, bool terminate = false); // write fixed-length ascii string
	void writeStringUNICODE(const char* value, bool terminate = true); // write unicode string until null terminator found
	void writeStringUNICODE(const WCHAR* value, bool terminate = true); // write unicode string until null terminator found
	void writeStringFixedUNICODE(const char* value, size_t size, bool terminate = false); // write fixed-length unicode string
	void writeStringFixedUNICODE(const WCHAR* value, size_t size, bool terminate = false); // write fixed-length unicode string
	void writeStringNUNICODE(const char* value, bool terminate = true); // write unicode string until null terminator found, network order
	void writeStringNUNICODE(const WCHAR* value, bool terminate = true); // write unicode string until null terminator found, network order
	void writeStringFixedNUNICODE(const char* value, size_t size, bool terminate = false); // write fixed-length unicode string, network order
	void writeStringFixedNUNICODE(const WCHAR* value, size_t size, bool terminate = false); // write fixed-length unicode string, network order
	void writeData(const BYTE* buffer, size_t size); // write block of data
	void fill(void); // zeroes remaining buffer
	size_t sync(void);
	void trim(void); // trim packet length down to current position

	// read
	bool readBool(void); // read boolean (1 byte)
	char readCharASCII(void); // read ASCII character (1 byte)
	WCHAR readCharUNICODE(void); // read UNICODE character (2 bytes)
	WCHAR readCharNUNICODE(void); // read UNICODE character, network order (2 bytes)
	BYTE readByte(void); // read 8-bit integer (1 byte)
	WORD readInt16(void); // read 16-bit integer (2 bytes)
	DWORD readInt32(void); // read 32-bit integer (4 bytes)
	void readStringASCII(char* buffer, size_t length, bool includeNull = true); // read fixed-length ascii string
	void readStringASCII(WCHAR* buffer, size_t length, bool includeNull = true); // read fixed-length ascii string
	void readStringUNICODE(char* buffer, size_t bufferSize, size_t length, bool includeNull = true); // read fixed length unicode string
	void readStringUNICODE(WCHAR* buffer, size_t length, bool includeNull = true); // read fixed length unicode string
	void readStringNUNICODE(char* buffer, size_t bufferSize, size_t length, bool includeNull = true); // read fixed length unicode string, network order
	void readStringNUNICODE(WCHAR* buffer, size_t length, bool includeNull = true); // read fixed length unicode string, network order
	size_t readStringNullASCII(char* buffer, size_t maxlength); // read ascii string until null terminator found
	size_t readStringNullASCII(WCHAR* buffer, size_t maxlength); // read ascii string until null terminator found
	size_t readStringNullUNICODE(char* buffer, size_t bufferSize, size_t maxlength); // read unicode-string until null terminator found
	size_t readStringNullUNICODE(WCHAR* buffer, size_t maxlength); // read unicode-string until null terminator found
	size_t readStringNullNUNICODE(char* buffer, size_t bufferSize, size_t maxlength); // read unicode-string until null terminator found, network order
	size_t readStringNullNUNICODE(WCHAR* buffer, size_t maxlength); // read unicode-string until null terminator found, network order

	size_t checkLength(NetState* client, Packet* packet);
	virtual size_t getExpectedLength(NetState* client, Packet* packet);
	virtual bool onReceive(NetState* client);

protected:
	void clear(void);
	void copy(const Packet& other);
};


/***************************************************************************
 *
 *
 *	class SendPacket			Send-type packet with priority and target client
 *
 *
 ***************************************************************************/
class PacketSend : public Packet
{
public:
	enum Priority
	{
		PRI_IDLE,
		PRI_LOW,
		PRI_NORMAL,
		PRI_HIGH,
		PRI_HIGHEST,
		PRI_QTY,
	};

protected:
	long m_priority; // packet priority
	NetState* m_target; // selected network target for this packet
	size_t m_lengthPosition; // position of length-byte

public:
	explicit PacketSend(BYTE id, size_t len = 0, Priority priority = PRI_NORMAL);
	PacketSend(const PacketSend* other);
	virtual ~PacketSend() { };

private:
	PacketSend& operator=(const PacketSend& other);

public:
	void initLength(void); // write empty length and ensure that it is remembered

	void target(const CClient* client); // sets person to send packet to

	void send(const CClient* client = NULL, bool appendTransaction = true); // adds the packet to the send queue
	void push(const CClient* client = NULL, bool appendTransaction = true); // moves the packet to the send queue (will not be used anywhere else)

	long getPriority() const { return m_priority; }; // get packet priority
	NetState* getTarget() const { return m_target; }; // get target state

	virtual bool onSend(const CClient* client);
	virtual void onSent(CClient* client);
	virtual bool canSendTo(const NetState* client) const;

#ifndef _MTNETWORK
	friend class NetworkOut;
#else
	friend class NetworkOutput;
#endif
	friend class SimplePacketTransaction;

protected:
	void fixLength(); // write correct packet length to it's slot
	virtual PacketSend* clone(void) const;
};


/***************************************************************************
 *
 *
 *	class PacketTransaction		Class for defining data to be sent
 *
 *
 ***************************************************************************/
class PacketTransaction
{
protected:
	PacketTransaction(void) { };

private:
	PacketTransaction(const PacketTransaction& copy);
	PacketTransaction& operator=(const PacketTransaction& other);

public:
	virtual ~PacketTransaction(void) { };

	virtual PacketSend* front(void) = 0; // get first packet in the transaction
	virtual void pop(void) = 0; // remove first packet from the transaction
	virtual bool empty(void) = 0; // check if any packets are available

	virtual NetState* getTarget(void) const = 0; // get target of the transaction
	virtual long getPriority(void) const = 0; // get priority of the transaction
	virtual void setPriority(long priority) = 0; // set priority of the transaction
};


/***************************************************************************
 *
 *
 *	class SimplePacketTransaction		Class for defining a single packet to be sent
 *
 *
 ***************************************************************************/
class SimplePacketTransaction : public PacketTransaction
{
private:
	PacketSend* m_packet;

public:
	explicit SimplePacketTransaction(PacketSend* packet) : m_packet(packet) { };
	~SimplePacketTransaction(void);

private:
	SimplePacketTransaction(const SimplePacketTransaction& copy);
	SimplePacketTransaction& operator=(const SimplePacketTransaction& other);

public:
	NetState* getTarget(void) const { return m_packet->getTarget(); }
	long getPriority(void) const { return m_packet->getPriority(); }
	void setPriority(long priority) { m_packet->m_priority = priority; }

	PacketSend* front(void) { return m_packet; };
	void pop(void) { m_packet = NULL; }
	bool empty(void) { return m_packet == NULL; }
};


/***************************************************************************
 *
 *
 *	class ExtendedPacketTransaction		Class for defining a set of packets to be sent together
 *
 *
 ***************************************************************************/
class ExtendedPacketTransaction : public PacketTransaction
{
private:
	std::list<PacketSend*> m_packets;
	NetState* m_target;
	long m_priority;

public:
	ExtendedPacketTransaction(NetState* target, long priority) : m_target(target), m_priority(priority) { };
	~ExtendedPacketTransaction(void);

private:
	ExtendedPacketTransaction(const ExtendedPacketTransaction& copy);
	ExtendedPacketTransaction& operator=(const ExtendedPacketTransaction& other);

public:
	NetState* getTarget(void) const	{ return m_target; }
	long getPriority(void) const { return m_priority; }
	void setPriority(long priority) { m_priority = priority; }

	void push_back(PacketSend* packet) { m_packets.push_back(packet); }
	PacketSend* front(void) { return m_packets.front(); };
	void pop(void) { m_packets.pop_front(); }
	bool empty(void) { return m_packets.empty(); }
};



/***************************************************************************
 *
 *
 *	class OpenPacketTransaction		Class to automatically begin and end a transaction
 *
 *
 ***************************************************************************/
class OpenPacketTransaction
{
private:
	NetState* m_client;

public:
	OpenPacketTransaction(const CClient* client, long priority);
	~OpenPacketTransaction(void);

private:
	OpenPacketTransaction(const OpenPacketTransaction& copy);
	OpenPacketTransaction& operator=(const OpenPacketTransaction& other);
};


#endif
