#ifndef TILEDATA_H
#define TILEDATA_H

#include "MulFileReader.h"
#include "../graysvr.h"

class Tiledata : public MulFileReader
{
public:
	enum TileFlag
	{
		None			= 0x00000000,	//
		Background		= 0x00000001,	// UFLAG1_FLOOR
		Weapon			= 0x00000002,	// UFLAG1_EQUIP
		Transparent		= 0x00000004,	// UFLAG1_NONBLOCKING
		Translucent		= 0x00000008,	// UFLAG1_LIQUID
		Wall			= 0x00000010,	// UFLAG1_WALL
		Damaging		= 0x00000020,	// UFLAG1_DAMAGE
		Impassable		= 0x00000040,	// UFLAG1_BLOCK
		Wet				= 0x00000080,	// UFLAG1_WATER
		Unknown1		= 0x00000100,	//
		Surface			= 0x00000200,	// UFLAG2_PLATFORM
		Bridge			= 0x00000400,	// UFLAG2_CLIMBABLE
		Generic			= 0x00000800,	// UFLAG2_STACKABLE
		Window			= 0x00001000,	// UFLAG2_WINDOW
		NoShoot			= 0x00002000,	// UFLAG2_WALL2
		ArticleA		= 0x00004000,	// UFLAG2_A
		ArticleAn		= 0x00008000,	// UFLAG2_AN
		Internal		= 0x00010000,	// UFLAG3_DESCRIPTION
		Foliage			= 0x00020000,	// UFLAG3_TRANSPARENT
		PartialHue		= 0x00040000,	// UFLAG3_CLOTH
		Unknown2		= 0x00080000,	//
		Map				= 0x00100000,	// UFLAG3_MAP
		Container		= 0x00200000,	// UFLAG3_CONTAINER
		Wearable		= 0x00400000,	// UFLAG3_EQUIP2
		LightSource		= 0x00800000,	// UFLAG3_LIGHT
		Animation		= 0x01000000,	// UFLAG4_ANIM
		NoDiagonal		= 0x02000000,	// UFLAG4_UNK1
		Unknown3		= 0x04000000,	//
		Armor			= 0x08000000,	// UFLAG4_BODYITEM
		Roof			= 0x10000000,	// UFLAG4_ROOF
		Door			= 0x20000000,	// UFLAG4_DOOR
		StairBack		= 0x40000000,	// UFLAG4_STAIRS
		StairRight		= 0x80000000	// UFLAG4_WALKABLE
	};
	struct LandData
	{
		char	m_Name[20];
		DWORD	m_Flags;
	};
	struct ItemData
	{
		char	m_Name[20];
		DWORD	m_Flags;
		BYTE	m_Weight;
		BYTE	m_Quality;
		BYTE	m_Quantity;
		BYTE	m_Value;
		BYTE	m_Height;

		void setBridge(bool on);
		void setImpassable(bool on);
		void setSurface(bool on);
		int calcHeight();
	};

	Tiledata();
	~Tiledata();

	ItemData *getItemData(ITEMID_TYPE id);
	LandData *getLandData(ITEMID_TYPE id);

	virtual bool onOpen();

private:
	enum SpecialItems
	{
		SpecialShip,
		SpecialStructure,
		SpecialQty
	};

	LandData	**m_LandData;
	ItemData	**m_ItemData;
	ItemData	m_specialItems[SpecialQty];
};

#endif