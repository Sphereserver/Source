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
	BYTE* m_buffer;		// raw data
	long m_bufferSize;	// size of raw data

	long m_length;		// length of packet
	long m_position;	// current position in packet

	long m_expectedLength; // expected length of this packet

public:
	Packet(long size = -1);
	Packet(const Packet& other);
	Packet(const BYTE* data, long size);
	virtual ~Packet(void);

	bool isValid(void) const;
	long getLength(void) const;
	long getPosition(void) const;
	BYTE* getData(void) const;
	TCHAR* dump(void) const;

	void expand(long size = 0); // expand packet (resize whilst maintaining position)
	void resize(long newsize); // resize packet
	void seek(long pos = 0); // seek to position
	void skip(long count = 1); // skip count bytes

	BYTE &operator[](long index);
	const BYTE &operator[](long index) const;

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
	void writeStringFixedASCII(const char* value, long size, bool terminate = false); // write fixed-length ascii string
	void writeStringFixedASCII(const WCHAR* value, long size, bool terminate = false); // write fixed-length ascii string
	void writeStringUNICODE(const char* value, bool terminate = true); // write unicode string until null terminator found
	void writeStringUNICODE(const WCHAR* value, bool terminate = true); // write unicode string until null terminator found
	void writeStringFixedUNICODE(const char* value, long size, bool terminate = false); // write fixed-length unicode string
	void writeStringFixedUNICODE(const WCHAR* value, long size, bool terminate = false); // write fixed-length unicode string
	void writeStringNUNICODE(const char* value, bool terminate = true); // write unicode string until null terminator found, network order
	void writeStringNUNICODE(const WCHAR* value, bool terminate = true); // write unicode string until null terminator found, network order
	void writeStringFixedNUNICODE(const char* value, long size, bool terminate = false); // write fixed-length unicode string, network order
	void writeStringFixedNUNICODE(const WCHAR* value, long size, bool terminate = false); // write fixed-length unicode string, network order
	void writeData(const BYTE* buffer, long size); // write block of data
	void fill(void); // zeroes remaining buffer
	long sync(void);
	void trim(void); // trim packet length down to current position

	// read
	bool readBool(void); // read boolean (1 byte)
	char readCharASCII(void); // read ASCII character (1 byte)
	WCHAR readCharUNICODE(void); // read UNICODE character (2 bytes)
	WCHAR readCharNUNICODE(void); // read UNICODE character, network order (2 bytes)
	BYTE readByte(void); // read 8-bit integer (1 byte)
	WORD readInt16(void); // read 16-bit integer (2 bytes)
	DWORD readInt32(void); // read 32-bit integer (4 bytes)
	void readStringASCII(char* buffer, long length, bool includeNull = true); // read fixed-length ascii string
	void readStringASCII(WCHAR* buffer, long length, bool includeNull = true); // read fixed-length ascii string
	void readStringUNICODE(char* buffer, long bufferSize, long length, bool includeNull = true); // read fixed length unicode string
	void readStringUNICODE(WCHAR* buffer, long length, bool includeNull = true); // read fixed length unicode string
	void readStringNUNICODE(char* buffer, long bufferSize, long length, bool includeNull = true); // read fixed length unicode string, network order
	void readStringNUNICODE(WCHAR* buffer, long length, bool includeNull = true); // read fixed length unicode string, network order
	long readStringNullASCII(char* buffer, long maxlength); // read ascii string until null terminator found
	long readStringNullASCII(WCHAR* buffer, long maxlength); // read ascii string until null terminator found
	long readStringNullUNICODE(char* buffer, long bufferSize, long maxlength); // read unicode-string until null terminator found
	long readStringNullUNICODE(WCHAR* buffer, long maxlength); // read unicode-string until null terminator found
	long readStringNullNUNICODE(char* buffer, long bufferSize, long maxlength); // read unicode-string until null terminator found, network order
	long readStringNullNUNICODE(WCHAR* buffer, long maxlength); // read unicode-string until null terminator found, network order

	long checkLength(NetState* client, Packet* packet);
	virtual long getExpectedLength(NetState* client, Packet* packet);
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
	long m_lengthPosition; // position of length-byte

public:
	PacketSend(BYTE id, long len = 0, Priority priority = PRI_NORMAL);
	PacketSend(const PacketSend* other);

	void initLength(void); // write empty length and ensure that it is remembered

	void target(const CClient* client); // sets person to send packet to

	void send(const CClient* client = NULL); // adds the packet to the send queue
	void push(const CClient* client = NULL); // moves the packet to the send queue (will not be used anywhere else)

	long getPriority() const { return m_priority; }; // get packet priority
	NetState* getTarget() const { return m_target; }; // get target state

	virtual bool onSend(const CClient* client);
	virtual void onSent(CClient* client);

	friend class NetworkOut;
	friend class SimplePacketTransaction;

protected:
	void fixLength(); // write correct packet length to it's slot
	virtual PacketSend* clone(void);
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

public:
	virtual ~PacketTransaction(void) { };

	virtual bool send(CClient* client) = 0; // send the transaction
	virtual bool onSend(CClient* client) = 0; // transaction is about to be sent

	virtual NetState* getTarget(void) const = 0; // get target of the transaction
	virtual long getPriority(void) const = 0; // get priority of the transaction
	virtual void setPriority(long priority) = 0; // set priority of the transaction
	virtual long getLength(void) const = 0; // get length of the transaction

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
	SimplePacketTransaction(PacketSend* packet) : m_packet(packet) { };
	~SimplePacketTransaction(void);

	bool send(CClient* client);
	bool onSend(CClient* client) { return m_packet->onSend(client); }

	NetState* getTarget(void) const { return m_packet->getTarget(); }
	long getPriority(void) const { return m_packet->getPriority(); }
	void setPriority(long priority) { m_packet->m_priority = priority; }
	long getLength(void) const { return m_packet->getLength(); }

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

	bool send(CClient* client);
	bool onSend(CClient* client);

	NetState* getTarget(void) const	{ return m_target; }
	long getPriority(void) const { return m_priority; }
	void setPriority(long priority) { m_priority = priority; }
	long getLength(void) const;

	void push_back(PacketSend* packet) { m_packets.push_back(packet); }
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
};


#endif
