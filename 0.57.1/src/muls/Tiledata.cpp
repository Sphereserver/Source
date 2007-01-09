#include "Tiledata.h"
#include "../network/packet.h"

/***************************************************************************
 *
 *
 *	class Tiledata::LandData		Land Tile Info
 *
 *
 ***************************************************************************/

/***************************************************************************
 *
 *
 *	class Tiledata::ItemData		Item Tile Info
 *
 *
 ***************************************************************************/

void Tiledata::ItemData::setBridge(bool on)
{
	if ( on )
		m_Flags |= Tiledata::Bridge;
	else
		m_Flags &= ~Tiledata::Bridge;
}

void Tiledata::ItemData::setImpassable(bool on)
{
	if ( on )
		m_Flags |= Tiledata::Impassable;
	else
		m_Flags &= ~Tiledata::Impassable;
}

void Tiledata::ItemData::setSurface(bool on)
{
	if ( on )
		m_Flags |= Tiledata::Surface;
	else
		m_Flags &= ~Tiledata::Surface;
}

int Tiledata::ItemData::calcHeight()
{
	if ( m_Flags & Tiledata::Bridge )
		return m_Height / 2;
	return m_Height;
}

/***************************************************************************
 *
 *
 *	class Tiledata				Tiledata.mul cache storage class
 *
 *
 ***************************************************************************/

Tiledata::Tiledata()
		: MulFileReader("tiledata.mul")
{
	m_LandData = NULL;
	m_ItemData = NULL;
}

Tiledata::~Tiledata()
{
	if ( m_LandData )
	{
		for ( int i = 0; i < 0x4000; ++i )
		{
			delete m_LandData[i];
		}
		delete []m_LandData;
	}
	if ( m_ItemData )
	{
		for ( int i = 0; i < 0x4000; ++i )
		{
			delete m_ItemData[i];
		}
		delete []m_ItemData;
	}
}

bool Tiledata::onOpen()
{
	int i;

	//	Land definitions
	m_LandData = new LandData*[0x4000];

	for ( i = 0; i < 0x4000; ++i )
	{
		if ( (i & 0x1f) == 0)
		{
			readDWORD();
		}

		TileFlag flags = (TileFlag)readDWORD();
		readWORD();

		m_LandData[i] = new LandData();

		strcpy(m_LandData[i]->m_Name, readString(20));
		m_LandData[i]->m_Flags = flags;
	}

	//	Item definitions
	m_ItemData = new ItemData*[0x4000];

	for ( i = 0; i < 0x4000; ++i )
	{
		if (( i & 0x1f ) == 0 )
		{
			readDWORD();
		}
		TileFlag flags = (TileFlag)readDWORD();
		int weight = readBYTE();
		int quality = readBYTE();
		readWORD();
		readBYTE();
		int quantity = readBYTE();
		readDWORD();
		readBYTE();
		int value = readBYTE();
		int height = readBYTE();

		m_ItemData[i] = new ItemData();

		strcpy(m_ItemData[i]->m_Name, readString(20));
		m_ItemData[i]->m_Flags = flags;
		m_ItemData[i]->m_Weight = weight;
		m_ItemData[i]->m_Quality = quality;
		m_ItemData[i]->m_Quantity = quantity;
		m_ItemData[i]->m_Value = value;
		m_ItemData[i]->m_Height = height;
	}

	//	Add some special information (virtual)
	memset(m_specialItems, 0, sizeof(m_specialItems));

	//	Ship
	m_specialItems[Tiledata::SpecialShip].m_Weight = 0xff;
	strcpy(m_specialItems[Tiledata::SpecialShip].m_Name, "ship");

	//	Structure
	m_specialItems[Tiledata::SpecialStructure].m_Weight = 0xff;
	strcpy(m_specialItems[Tiledata::SpecialStructure].m_Name, "structure");

	return true;
}

Tiledata::ItemData *Tiledata::getItemData(ITEMID_TYPE id)
{
	if ( id >= 0x4000 )
	{
		if ( id <= ITEMID_SHIP6_W )
			return &m_specialItems[Tiledata::SpecialShip];
		else
			return &m_specialItems[Tiledata::SpecialStructure];
	}
	return m_ItemData[id];
}

Tiledata::LandData *Tiledata::getLandData(ITEMID_TYPE id)
{
	return m_LandData[id];
}
