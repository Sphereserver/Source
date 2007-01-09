#ifndef PACKET_RECEIVE_H
#define PACKET_RECEIVE_H

#include "packet.h"

/***************************************************************************
 *
 *
 *	Packet 0x** : PacketDummy			empty packet (ignored)
 *
 *
 ***************************************************************************/
class PacketDummy : public Packet
{
public:
	PacketDummy(long size);
	virtual bool onReceive(NetState *net);
private:
	bool m_dynamic;
};

/***************************************************************************
 *
 *
 *	Packet 0x02 : PacketMovementReq		movement request
 *
 *
 ***************************************************************************/
class PacketMovementReq : public Packet
{
public:
	PacketMovementReq();
	virtual bool onReceive(NetState *net);
};

/***************************************************************************
 *
 *
 *	Packet 0x13 : PacketEquipReq		item equip request
 *
 *
 ***************************************************************************/
class PacketEquipReq : public Packet
{
public:
	PacketEquipReq();
	virtual bool onReceive(NetState *net);
};

/***************************************************************************
 *
 *
 *	Packet 0x22 : PacketResynchronize	resend all request
 *
 *
 ***************************************************************************/
class PacketResynchronize : public Packet
{
public:
	PacketResynchronize();
	virtual bool onReceive(NetState *net);
};

/***************************************************************************
 *
 *
 *	Packet 0x34 : PacketCharStatusReq	request information on the mobile
 *
 *
 ***************************************************************************/
class PacketCharStatusReq : public Packet
{
public:
	PacketCharStatusReq();
	virtual bool onReceive(NetState *net);
};

/***************************************************************************
 *
 *
 *	Packet 0x73 : PacketPingReq			ping requests
 *
 *
 ***************************************************************************/
class PacketPingReq : public Packet
{
public:
	PacketPingReq();
	virtual bool onReceive(NetState *net);
};

/***************************************************************************
 *
 *
 *	Packet 0xd1 : PacketLogoutReq		logout requests
 *
 *
 ***************************************************************************/
class PacketLogoutReq : public Packet
{
public:
	PacketLogoutReq();
	virtual bool onReceive(NetState *net);
};

#endif
