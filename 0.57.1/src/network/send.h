#ifndef SEND_H
#define SEND_H

#include "packet.h"

class CItemCorpse;

/***************************************************************************
 *
 *
 *	Packet **** : PacketGeneric				Temporary packet till all will be redone!
 *
 *
 ***************************************************************************/
class PacketGeneric : public PacketSend
{
public:
	PacketGeneric(BYTE *data, long length, CClient *target);
};

/***************************************************************************
 *
 *
 *	Packet 0x0b : PacketCombatDamage		sends notification of got damage (LOW)
 *
 *
 ***************************************************************************/
class PacketCombatDamage : public PacketSend
{
public:
	PacketCombatDamage(DWORD damage, CChar *defender, CChar *attacker);
};

/***************************************************************************
 *
 *
*	Packet 0x1a : PacketItemWorld			sends item on ground (NORMAL)
 *
 *
 ***************************************************************************/
class PacketItemWorld : public PacketSend
{
public:
	PacketItemWorld(CItem *item, CClient *target);
};

/***************************************************************************
 *
 *
 *	Packet 0x1d : PacketRemoveObject		removes object from view (HIGHER)
 *
 *
 ***************************************************************************/
class PacketRemoveObject : public PacketSend
{
public:
	PacketRemoveObject(UID uid, CClient *target);
};

/***************************************************************************
 *
 *
 *	Packet 0x21 : PacketMovementRej			rejects movement (HIGHEST)
 *
 *
 ***************************************************************************/
class PacketMovementRej : public PacketSend
{
public:
	PacketMovementRej(CClient *target, BYTE sequence);
};

/***************************************************************************
 *
 *
 *	Packet 0x22 : PacketMovementAck			accepts movement (HIGHEST)
 *
 *
 ***************************************************************************/
class PacketMovementAck : public PacketSend
{
public:
	PacketMovementAck(CClient *target);
};

/***************************************************************************
 *
 *
 *	Packet 0x2f : PacketSwing				fight swing (LOW)
 *
 *
 ***************************************************************************/
class PacketSwing : public PacketSend
{
public:
	PacketSwing(CClient *target, CChar *defender);
};

/***************************************************************************
 *
 *
 *	Packet 0x4f : PacketGlobalLight			sets global light level (NORMAL)
 *
 *
 ***************************************************************************/
class PacketGlobalLight : public PacketSend
{
public:
	PacketGlobalLight(int light, CClient *target);
};

/***************************************************************************
 *
 *
 *	Packet 0x6d : PacketPlayMusic			adds music to the client (IDLE)
 *
 *
 ***************************************************************************/
class PacketPlayMusic : public PacketSend
{
public:
	PacketPlayMusic(WORD musicID, CClient *target);
};

/***************************************************************************
 *
 *
 *	Packet 0x73 : PacketPingAck				ping reply (IDLE)
 *
 *
 ***************************************************************************/
class PacketPingAck : public PacketSend
{
public:
	PacketPingAck(CClient *target, BYTE value = 0);
};

/***************************************************************************
 *
 *
 *	Packet 0x82 : PacketLoginError			login error response (LOW)
 *
 *
 ***************************************************************************/
class PacketLoginError : public PacketSend
{
public:
	enum Reason
	{
		Invalid = 0x00,
		InUse	= 0x01,
		Blocked = 0x02,
		BadPass = 0x03,
		Other	= 0x04,
		Success = 0xFF
	};
	PacketLoginError(Reason reason, CClient *target);
};

/***************************************************************************
 *
 *
 *	Packet 0xaf : PacketDeath				notifies about character death (NORMAL)
 *
 *
 ***************************************************************************/
class PacketDeath : public PacketSend
{
public:
	PacketDeath(CChar *dead, CItemCorpse *corpse);
};

/***************************************************************************
 *
 *
 *	Packet 0xbf.04 : GumpClose				closes the gump (HIGHEST)
 *
 *
 ***************************************************************************/
class GumpClose : public PacketSend
{
public:
	GumpClose(CClient *target, int typeID = 0, int buttonID = 0);
};

/***************************************************************************
 *
 *
 *	Packet 0xd1 : PacketLogoutAck			accept logout char (LOW)
 *
 *
 ***************************************************************************/
class PacketLogoutAck : public PacketSend
{
public:
	PacketLogoutAck(CClient *target);
};

/***************************************************************************
 *
 *
 *	Packet 0xd6 : PacketPropertyList		property (tool tip) for objects (LOW)
 *
 *
 ***************************************************************************/
class PacketPropertyList : public PacketSend
{
public:
	DWORD m_hash;								//	calculated hash of the packet

public:
	PacketPropertyList(CObjBase *object);

	void restart(CObjBase *object);				//	starts this packet from scratch
	void terminate();							//	finalizes the packet

	void add(char *args);						//	add text link
	void _cdecl addf(char *format, ...);		//	add format able text link
	void add(long id, char *args);				//	add single id/text link
	void _cdecl addf(long id, char *format, ...);//	add format able id/text link

protected:
	void addHash(long num);
	void addHash(char *args);
};

/***************************************************************************
 *
 *
 *	Packet 0xdc : PacketPropertyHash		property (tool tip) hash (IDLE)
 *
 *
 ***************************************************************************/
class PacketPropertyHash : public PacketSend
{
public:
	PacketPropertyHash(CObjBase *object);

	void setHash(long hash);
};

#endif
