#ifndef PACKET_H
#define PACKET_H

#include "../graysvr.h"

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
	BYTE	*m_buf;			// raw buffer
	long	m_len;			// current ensured buffer length
	long	m_maxlen;		// maximal allocated buffer length
	long	m_index;		// current buffer index
	long	m_baselen;		// maximal length of the packet (-1 means dynamic packet)

protected:
	void clear();								// internal init (DOEST NOT deallocate!)
	void copy(Packet &other);

public:
	Packet(long size = -1);
	Packet(Packet &other);						// copy constructor
	Packet(BYTE *buf, long size);				// TODO: should be removed when all packets will be done new way
	~Packet();

	long baseLength();							// length of the packet for checking the incoming packets being valid
	long length();								// packet length
	BYTE *data();								// packet data

	bool valid();								// is packet valid?
	char *dump();								// dumps the packet

	void seek(long pos = 0);					// jumps to position of the packet
	void resize(long newsize);					// changes the size of the packet

	BYTE &operator[](int index);				// set byte X (unsafe!)
	const BYTE &operator[](int index) const;	// get byte X (unsafe!)

	//	stream subsequent write
	void write(bool value);						// write 1-byte boolean
	void write(BYTE value);						// write 1-byte byte
	void write(WORD value);						// write 2-byte word
	void write(DWORD value);					// write 4-byte DWORD
	void write(char *buf, long size);			// write sequence of bytes
	void writeFixed(char *buf, long size);		// write fixed string
	void write(char *buf);						// write dynamic string, appending '\0'
	//TODO: add unicode writers
	void fill();								// fills up to end with '\0'
	long sync();								// sets length to index, useful in unknown write

	//	stream subsequent read
	DWORD readDWORD();							// read 4-byte DWORD
	WORD readWORD();							// read 2-byte word
	BYTE readBYTE();							// read 1-byte byte
	bool readBOOL();							// read 1-byte boolean
	void readString(char *, int = 0);			// read dynamic (static if INT) string
	void readStringSafe(char *, int = 0);		// read dynamic (static) safe string
	//TODO; add unicode readers

	long getValidLength();						// check if the incoming packet has valid length and returns it

	//	ready-to-override
	virtual bool onReceive(NetState *client);
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
	long	m_priority;							// packet priority
	NetState *m_target;							// selected network target for this packet
	long	m_lenPos;							// position of length-byte

public:
	PacketSend(BYTE id, long len = 0, Priority priority = PRI_NORMAL);
	PacketSend(PacketSend *other);

	void initLength();							// write empty length and ensure that it is remembered

	void target(CClient *client);				// sets person to send packet to

	void send(CClient *client = NULL);			// adds the packet to the send queue
	void push(CClient *client = NULL);			// moves the packet to the send queue (will not be used anywhere else)

	long getPriority();

	//	ready-to-override
	virtual bool onSend(CClient *pClient);

	friend class NetworkOut;

protected:
	void fixLength();							// write correct packet length to it's slot

};

#endif
