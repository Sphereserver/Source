#include "send.h"
#include "network.h"
#include "stdarg.h"

/***************************************************************************
 *
 *
 *	Packet **** : PacketGeneric				Temporary packet till all will be redone!
 *
 *
 ***************************************************************************/
PacketGeneric::PacketGeneric(BYTE *data, long length, CClient *target)
		: PacketSend(0, length)
{
	m_priority = PRI_NORMAL;
	//causes problems with flashing when remove is send after or long before the appear
	//Calc_GetRandVal2(PRI_LOW, PRI_HIGH);
	seek();
	write((char*)data, length);
	push(target);
}

/***************************************************************************
 *
 *
 *	Packet 0x0b : PacketCombatDamage		sends notification of got damage (LOW)
 *
 *
 ***************************************************************************/
PacketCombatDamage::PacketCombatDamage(DWORD damage, CChar *defender, CChar *attacker)
		: PacketSend(0x0b, 7, PRI_LOW)
{
	write(defender->uid());
	if ( damage >= 0x10000 )
		damage = 0xffff;
	write((WORD)damage);

	if ( defender->IsClient() )
		send(defender->GetClient());

	if (( defender->uid() != attacker->uid() ) && attacker->IsClient() )
		send(attacker->GetClient());
}

/***************************************************************************
 *
 *
 *	Packet 0x1a : PacketItemWorld			sends item on ground (NORMAL)
 *
 *
 ***************************************************************************/
PacketItemWorld::PacketItemWorld(CItem *item, CClient *target)
		: PacketSend(0x1a, 20, PRI_NORMAL)
{
	initLength();

	DWORD uid = item->uid();
	long amount = ( item->GetAmount() > 1 ) ? item->GetAmount() : 0;
	ITEMID_TYPE id = item->GetDispID();
	CPointMap p = item->GetTopPoint();
	BYTE dir = DIR_N;
	HUE_TYPE hue = item->GetHue();
	BYTE flags = 0;

	if ( target->m_pChar->CanMove(item, false) )
		flags |= 0x20;
	if ( !target->m_pChar->CanSee(item) )
		flags |= 0x80;

	if ( amount )
		uid |= 0x80000000;
	else
		uid &= 0x7fffffff;

	p.m_x &= 0x7fff;
	if ( dir )
		p.m_x |= 0x8000;
	p.m_y &= 0x3fff;
	if ( hue )
		p.m_y |= 0x8000;
	if ( flags )
		p.m_y |= 0x4000;

	if ( id == ITEMID_CORPSE )
		dir = item->m_itCorpse.m_facing_dir;

	write(uid);
	write((WORD)id);
	if ( amount )
		write((WORD)amount);
	write((WORD)p.m_x);
	write((WORD)p.m_y);
	if ( dir )
		write(dir);
	write((BYTE)p.m_z);
	if ( hue )
		write(hue);
	if ( flags )
		write(flags);

	push(target);
}

/***************************************************************************
 *
 *
 *	Packet 0x1d : PacketRemoveObject		removes object from view (HIGHER)
 *
 *
 ***************************************************************************/
PacketRemoveObject::PacketRemoveObject(UID uid, CClient *target)
		: PacketSend(0x1d, 5, PRI_HIGH)
{
	write((DWORD)uid);
	push(target);
}

/***************************************************************************
 *
 *
 *	Packet 0x21 : PacketMovementRej			rejects movement (HIGHEST)
 *
 *
 ***************************************************************************/
PacketMovementRej::PacketMovementRej(CClient *target, BYTE sequence)
		: PacketSend(0x21, 8, PRI_HIGHEST)
{
	CChar *mobile = target->GetChar();
	CPointMap p = mobile->GetTopPoint();
	write(sequence);
	write((WORD)p.m_x);
	write((WORD)p.m_y);
	write(mobile->GetDirFlag());
	write((BYTE)p.m_z);
	push(target);
};

/***************************************************************************
 *
 *
 *	Packet 0x22 : PacketMovementAck			accepts movement (HIGHEST)
 *
 *
 ***************************************************************************/
PacketMovementAck::PacketMovementAck(CClient *target)
		: PacketSend(0x22, 3, PRI_HIGHEST)
{
	write((BYTE)target->m_net->m_sequence);
	write((BYTE)target->GetChar()->Noto_GetFlag(target->GetChar(), false, true));
	push(target);
}

/***************************************************************************
 *
 *
 *	Packet 0x2f : PacketSwing				fight swing (LOW)
 *
 *
 ***************************************************************************/
PacketSwing::PacketSwing(CClient *target, CChar *defender)
		: PacketSend(0x2f, 10, PRI_LOW)
{
	write((BYTE)0);
	write(target->m_pChar->uid());
	write(defender->uid());
	push(target);
}

/***************************************************************************
 *
 *
 *	Packet 0x4f : PacketGlobalLight			sets global light level (NORMAL)
 *
 *
 ***************************************************************************/
PacketGlobalLight::PacketGlobalLight(int light, CClient *target)
		: PacketSend(0x4f, 2, PRI_NORMAL)
{
	write((BYTE)light);
	push(target);
}

/***************************************************************************
 *
 *
 *	Packet 0x6d : PacketPlayMusic			adds music to the client (IDLE)
 *
 *
 ***************************************************************************/
PacketPlayMusic::PacketPlayMusic(WORD musicID, CClient *target)
		: PacketSend(0x6d, 3, PRI_IDLE)
{
	write(musicID);
	push(target);
};

/***************************************************************************
 *
 *
 *	Packet 0x73 : PacketPingAck				ping reply (IDLE)
 *
 *
 ***************************************************************************/
PacketPingAck::PacketPingAck(CClient *target, BYTE value)
		: PacketSend(0x73, 2, PRI_IDLE)
{
	write(value);
	push(target);
}

/***************************************************************************
 *
 *
 *	Packet 0x82 : PacketLoginError			login error response (LOW)
 *
 *
 ***************************************************************************/
PacketLoginError::PacketLoginError(Reason reason, CClient *target)
		: PacketSend(0x82, 2, PRI_LOW)
{
	write((BYTE)reason);
	push(target);
}

/***************************************************************************
 *
 *
 *	Packet 0xaf : PacketDeath				notifies about character death (NORMAL)
 *
 *
 ***************************************************************************/
PacketDeath::PacketDeath(CChar *dead, CItemCorpse *corpse)
	: PacketSend(0xaf, 13, PRI_NORMAL)
{
	write(dead->uid());
	write(corpse ? (DWORD)0 : corpse->uid());
	write((DWORD)0);
}

/***************************************************************************
 *
 *
 *	Packet 0xbf.04 : GumpClose				closes the gump (HIGHEST)
 *
 *
 ***************************************************************************/
GumpClose::GumpClose(CClient *target, int typeID, int buttonID)
		: PacketSend(0xbf, 0, PRI_HIGHEST)
{
	initLength();
	write((WORD)0x04);
	write((DWORD)typeID);
	write((DWORD)buttonID);
	push(target);
}

/***************************************************************************
 *
 *
 *	Packet 0xd1 : PacketLogoutAck			accept logout char (LOW)
 *
 *
 ***************************************************************************/
PacketLogoutAck::PacketLogoutAck(CClient *target)
		: PacketSend(0xd1, 2, PRI_LOW)
{
	write((BYTE)1);
	push(target);
}

/***************************************************************************
 *
 *
 *	Packet 0xd6 : PacketPropertyList		property (tool tip) for objects (LOW)
 *
 *
 ***************************************************************************/
PacketPropertyList::PacketPropertyList(CObjBase *object)
		: PacketSend(0xd6, 48, PRI_LOW)
{
	restart(object);
/*
 0 byte	ID (D6)
 1 word	packet size
 3 word	1
 5 dword	serial
 9 word	0
11 dword	hash
loop---
+4 dword	number
+6 word		length(args)
+x byte[*]	args
loop---
+y dword	0
*/
}

void PacketPropertyList::restart(CObjBase *object)
{
	seek(1);
	m_hash = 0xebebebeb;
	initLength();

	write((WORD)1);
	write(object->uid());
	write((WORD)0);
	write(object->uid());
}

void PacketPropertyList::terminate()
{
	write((DWORD)0);
	fixLength();
	seek(11);
	write(m_hash);
}

void PacketPropertyList::add(long id, char *args)
{
	if ( !id )
		return;

	if ( !args )
		args = "";
	short len = strlen(args);

	addHash(id);
	addHash(args);

	write((DWORD)id);
	write((WORD)len);
	write(args, len);
}

void _cdecl PacketPropertyList::addf(long id, char *format, ...)
{
	char	buf[256];
	va_list	vargs;

	va_start(vargs, format);
	vsprintf(buf, format, vargs);
	add(id, buf);
	va_end(vargs);
}

void PacketPropertyList::add(char *args)
{
	add(1042971, args);
}

void _cdecl PacketPropertyList::addf(char *format, ...)
{
	char	buf[256];
	va_list	vargs;

	va_start(vargs, format);
	vsprintf(buf, format, vargs);
	add(1042971, buf);
	va_end(vargs);
}

void PacketPropertyList::addHash(long num)
{
	m_hash ^= ( num & 0x3fffffff );
	m_hash ^= ( num >> 26 ) & 0x3f;
}

void PacketPropertyList::addHash(char *args)
{
	long hash = 0xbebebebe;
	for ( int i = 0; i < strlen(args); i++ )
		hash -= args[i];
	addHash(hash);
}

/***************************************************************************
 *
 *
 *	Packet 0xdc : PacketPropertyHash		property (tool tip) hash (IDLE)
 *
 *
 ***************************************************************************/
PacketPropertyHash::PacketPropertyHash(CObjBase *object)
		: PacketSend(0xdc, 9, PRI_IDLE)
{
	write(object->uid());
	//	hash should be added externally
/*
byte	ID (DC)
dword	serial
dword	hash
*/
}

void PacketPropertyHash::setHash(long hash)
{
	seek(5);
	write((DWORD)hash);
}
