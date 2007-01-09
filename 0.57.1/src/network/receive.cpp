#include "receive.h"
#include "network.h"
#include "send.h"

/***************************************************************************
 *
 *
 *	Packet 0x** : PacketDummy			empty packet (ignored)
 *
 *
 ***************************************************************************/
PacketDummy::PacketDummy(long size)
		: Packet(size)
{
	m_dynamic = (size <= 0);
}

bool PacketDummy::onReceive(NetState *net)
{
	UNREFERENCED_PARAMETER(net);
	return true;
}

/***************************************************************************
 *
 *
 *	Packet 0x02 : PacketMovementReq		movement request
 *
 *
 ***************************************************************************/
PacketMovementReq::PacketMovementReq()
		: Packet(7)
{
}

bool PacketMovementReq::onReceive(NetState *net)
{
	BYTE dir = readBYTE();
	int seq = readBYTE();
	readDWORD();
	CClient *client = net->m_client;

	bool canMoveThere = client->Event_Walking(dir);
	if ((( net->m_sequence == 0 ) && ( seq != 0 )) || !canMoveThere )
	{
		PacketMovementRej *packet = new PacketMovementRej(client, seq);
		net->m_sequence = 0;
	}
	else
	{
		if ( ++seq == 256 )
			seq = 1;
		net->m_sequence = seq;
	}
	return true;
}

/***************************************************************************
 *
 *
 *	Packet 0x13 : PacketEquipReq		item equip request
 *
 *
 ***************************************************************************/
PacketEquipReq::PacketEquipReq()
		: Packet(10)
{
}

bool PacketEquipReq::onReceive(NetState *net)
{
	CChar *source = net->m_client->m_pChar;
	CItem *item = source->LayerFind(LAYER_DRAGGING);

	if ( !item )
	{
		net->m_client->addItemDragCancel(5);
		return true;
	}

	net->m_client->ClearTargMode();

	seek(6);
	UID targUid = readDWORD();
	CChar *target = targUid.CharFind();
	if ( !target )
		target = source;

	bool success = false;

	if ( !target->NPC_IsOwnedBy(source) || !target->ItemEquip(item, source) )
		source->ItemBounce(item);

	return true;
}

/***************************************************************************
 *
 *
 *	Packet 0x22 : PacketResynchronize	resend all request
 *
 *
 ***************************************************************************/
PacketResynchronize::PacketResynchronize()
		: Packet(3)
{
}

bool PacketResynchronize::onReceive(NetState *net)
{
	net->m_sequence = 0;
	net->m_client->addReSync();
	return true;
}

/***************************************************************************
 *
 *
 *	Packet 0x34 : PacketCharStatusReq	request information on the mobile
 *
 *
 ***************************************************************************/
PacketCharStatusReq::PacketCharStatusReq()
		: Packet(10)
{
}

bool PacketCharStatusReq::onReceive(NetState *net)
{
	readDWORD();
	int requestType = readBYTE();
	DWORD targetUID = readDWORD();

	if ( requestType == 4 )
	{
		net->m_client->addCharStatWindow(targetUID, true);
	}
	else if ( requestType == 5 )
	{
		net->m_client->addSkillWindow(SKILL_QTY);
	}
	return true;
}

/***************************************************************************
 *
 *
 *	Packet 0x73 : PacketPingReq			ping requests
 *
 *
 ***************************************************************************/
PacketPingReq::PacketPingReq()
		: Packet(2)
{
}

bool PacketPingReq::onReceive(NetState *net)
{
	BYTE value = readBYTE();
	PacketPingAck *packet = new PacketPingAck(net->m_client, value);
	return true;
}

/***************************************************************************
 *
 *
 *	Packet 0xd1 : PacketLogoutReq		logout requests
 *
 *
 ***************************************************************************/
PacketLogoutReq::PacketLogoutReq()
		: Packet(2)
{
}

bool PacketLogoutReq::onReceive(NetState *net)
{
	PacketLogoutAck *packet = new PacketLogoutAck(net->m_client);
	return true;
}
