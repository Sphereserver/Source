#ifndef C_MAP_CACHE_H
#define C_MAP_CACHE_H

#include "../graysvr/graysvr.h"

struct TMapCachePoint
{
	signed	char m_baseZ;	// base (zero level of the map)
	BYTE	m_tileBlock;	// block flags (CAN_I_ [DOOR...ROOF])
	BYTE	m_itemBlock;	// block flags basing on items in the pos (could be incorrect)
	BYTE	m_charBlock;	// block flags basing on chars in the pos (could be incorrect)
};
// 4 bytes for each map.
// 0x1800 * 0x1000 * 4 = 100Mb for map #0

class CMapCache
{
public:
	static const char *m_sClassName;
	CMapCache();
	~CMapCache();

private:
	CMapCache(const CMapCache& copy);
	CMapCache& operator=(const CMapCache& other);

public:
	bool Init(int maxX, int maxY, int map = 0, int mapDefaultHeight = 0);

	bool Get(int x, int y, TMapCachePoint &point);
	bool Set(int x, int y, TMapCachePoint &point);

	void SnapItems();		// update items on the whole map
	void SnapChars();		// update chars on the whole map

protected:
	FILE	*m_hFile;
	int		m_map;
	int		m_maxX;
	int		m_maxY;
};

#endif