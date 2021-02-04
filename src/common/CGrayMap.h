//
// CGrayMap.h
//

#ifndef _INC_CGRAYMAP_H
#define _INC_CGRAYMAP_H

class CGrayCachedMulItem
{
private:
	CServTime m_timeRef;		// when in world.GetTime() was this last referenced
public:
	static const char *m_sClassName;
	CGrayCachedMulItem()
	{
		InitCacheTime();
	}
	virtual ~CGrayCachedMulItem()
	{
	}

private:
	CGrayCachedMulItem(const CGrayCachedMulItem &copy);
	CGrayCachedMulItem &operator=(const CGrayCachedMulItem &other);

public:
	void InitCacheTime()
	{
		m_timeRef.Init();
	}
	bool IsTimeValid() const
	{
		return m_timeRef.IsTimeValid();
	}
	void HitCacheTime()
	{
		// When in g_World.GetTime() was this last referenced
		m_timeRef = CServTime::GetCurrentTime();
	}
	INT64 GetCacheAge() const
	{
		// In TICK_PER_SEC or milliseconds
		return CServTime::GetCurrentTime() - m_timeRef;
	}
};

class CGrayStaticsBlock
{
private:
	size_t m_iStatics;
	CUOStaticItemRec *m_pStatics;	// dyn alloc array block
public:
	void LoadStatics(DWORD dwBlockIndex, int map);
	void LoadStatics(size_t iCount, CUOStaticItemRec *pStatics);
public:
	static const char *m_sClassName;
	CGrayStaticsBlock()
	{
		m_iStatics = 0;
		m_pStatics = NULL;
	}
	~CGrayStaticsBlock()
	{
		if ( m_pStatics )
			delete[] m_pStatics;
	}

private:
	CGrayStaticsBlock(const CGrayStaticsBlock &copy);
	CGrayStaticsBlock &operator=(const CGrayStaticsBlock &other);

public:
	size_t GetStaticQty() const
	{
		return m_iStatics;
	}
	const CUOStaticItemRec *GetStatic(size_t i) const
	{
		ASSERT(i < m_iStatics);
		return &m_pStatics[i];
	}
	bool IsStaticPoint(size_t i, int xo, int yo) const;
};

struct CGrayMapBlocker
{
	DWORD m_dwBlockFlags;	// how does this item block? CAN_I_PLATFORM
	DWORD m_dwTile;			// TERRAIN_QTY + id
	signed char m_z;		// top of a solid object, or bottom of non blocking one
};

struct CGrayMapBlockState
{
	// Go through the list of stuff at this location to decide what is blocking us and what is not
	//  dwBlockFlags = what we can pass thru. doors, water, walls ?
	//		CAN_C_GHOST	= Moves thru doors etc.	- CAN_I_DOOR = UFLAG4_DOOR						
	//		CAN_C_SWIM = walk thru water - CAN_I_WATER = UFLAG1_WATER
	//		CAN_C_WALK = it is possible that i can step up. - CAN_I_PLATFORM = UFLAG2_PLATFORM
	//		CAN_C_PASSWALLS	= walk through all blcking items - CAN_I_BLOCK = UFLAG1_BLOCK
	//		CAN_C_FLY  = gravity does not effect me. -  CAN_I_CLIMB = UFLAG2_CLIMBABLE
	//		CAN_C_FIRE_IMMUNE = i can walk into lava etc. - CAN_I_FIRE = UFLAG1_DAMAGE
	//		CAN_C_HOVER = i can follow hover routes. - CAN_I_HOVER = UFLAG4_HOVEROVER

	const DWORD m_dwBlockFlags;	// the block flags we can overcome.
	const signed char m_z;		// the z we start at (stay at if we are flying)
	const int m_iHeight;		// the height we need to stand here
	const signed char m_zClimb;	// we can climb at this height
	const height_t m_zHeight;	// our height

	height_t m_zClimbHeight;	// return item climb height here

	CGrayMapBlocker m_Top;
	CGrayMapBlocker m_Bottom;	// what i would be standing on
	CGrayMapBlocker m_Lowest;	// the lowest item we have found

public:
	CGrayMapBlockState(DWORD dwBlockFlags, signed char m_z, int iHeight = PLAYER_HEIGHT, height_t zHeight = PLAYER_HEIGHT);
	CGrayMapBlockState(DWORD dwBlockFlags, signed char m_z, int iHeight, signed char zClimb, height_t zHeight = PLAYER_HEIGHT);

private:
	CGrayMapBlockState(const CGrayMapBlockState &copy);
	CGrayMapBlockState &operator=(const CGrayMapBlockState &other);

public:
	bool IsUsableZ(signed char zBottom, height_t zHeightEstimate) const
	{
		if ( zBottom > m_Top.m_z )	// above something that is already over my head
			return false;
		// NOTE: Assume multi overlapping items are not normal, so estimates are safe
		if ( zBottom + zHeightEstimate < m_Bottom.m_z )		// way below my feet
			return false;
		return true;
	}
	bool CheckTile(DWORD dwItemBlockFlags, signed char zBottom, height_t zheight, DWORD dwID);
	bool CheckTile_Item(DWORD dwItemBlockFlags, signed char zBottom, height_t zheight, DWORD dwID);
	inline void SetTop(DWORD &dwItemBlockFlags, signed char &z, DWORD &dwID);
	bool CheckTile_Terrain(DWORD dwItemBlockFlags, signed char z, DWORD dwID);
};

struct CMapDiffBlock
{
	// A patched map block
	CUOStaticItemRec *m_pStaticsBlock;		// patched statics
	int m_iStaticsCount;					// patched statics count
	CUOMapBlock *m_pTerrainBlock;			// patched terrain
	DWORD m_BlockId;						// block represented
	int m_map;								// map this block is from

	CMapDiffBlock(DWORD dwBlockId, int map)
	{
		m_BlockId = dwBlockId;
		m_map = map;
		m_iStaticsCount = -1;
		m_pStaticsBlock = NULL;
		m_pTerrainBlock = NULL;
	};

	~CMapDiffBlock()
	{
		if ( m_pStaticsBlock )
			delete[] m_pStaticsBlock;
		if ( m_pTerrainBlock )
			delete m_pTerrainBlock;
		m_pStaticsBlock = NULL;
		m_pTerrainBlock = NULL;
	};

private:
	CMapDiffBlock(const CMapDiffBlock &copy);
	CMapDiffBlock &operator=(const CMapDiffBlock &other);
};

class CMapDiffBlockArray : public CGObSortArray<CMapDiffBlock *, DWORD>
{
public:
	int CompareKey(DWORD id, CMapDiffBlock *pBase, bool fNoSpaces) const
	{
		UNREFERENCED_PARAMETER(fNoSpaces);
		ASSERT(pBase);
		return id - pBase->m_BlockId;
	}

public:
	CMapDiffBlockArray() { };
private:
	CMapDiffBlockArray(const CMapDiffBlockArray &copy);
	CMapDiffBlockArray &operator=(const CMapDiffBlockArray &other);
};

class CMapDiffCollection
{
	// This class will be used to access mapdiff data
private:
	bool m_bLoaded;

	CMapDiffBlockArray m_pMapDiffBlocks[MAP_QTY];
	CMapDiffBlock *GetNewBlock(DWORD dwBlockId, int map);
	void LoadMapDiffs();

public:
	CMapDiffCollection();
	~CMapDiffCollection();

private:
	CMapDiffCollection(const CMapDiffCollection &copy);
	CMapDiffCollection &operator=(const CMapDiffCollection &other);

public:
	void Init();
	CMapDiffBlock *GetAtBlock(int bx, int by, int map);
	CMapDiffBlock *GetAtBlock(DWORD dwBlockId, int map);
};

class CGrayMapBlock :	// cache this from the MUL files. 8x8 block of the world
	public CPointSort	// the upper left corner (ignore z) sort by this
{
protected:
	int		m_map;

private:
	static size_t sm_iCount;	// count number of loaded blocks

	CUOMapBlock m_Terrain;

public:
	static const char *m_sClassName;
	CGrayStaticsBlock m_Statics;
	CGrayCachedMulItem m_CacheTime;		// keep track of the use time of this item (client does not care about this)
private:
	void Load(int bx, int by);	// NOTE: This will "throw" on failure!
	//void LoadDiffs(DWORD dwBlockIndex, int map);

public:
	explicit CGrayMapBlock(const CPointMap &pt) : CPointSort(pt)	// the upper left corner
	{
		sm_iCount++;
		m_map = pt.m_map;
		Load(pt.m_x / UO_BLOCK_SIZE, pt.m_y / UO_BLOCK_SIZE);
	}

	CGrayMapBlock(int bx, int by, int map) : CPointSort(static_cast<WORD>(bx) * UO_BLOCK_SIZE, static_cast<WORD>(by) * UO_BLOCK_SIZE)
	{
		sm_iCount++;
		m_map = map;
		Load(bx, by);
	}

	virtual ~CGrayMapBlock()
	{
		sm_iCount--;
	}

private:
	CGrayMapBlock(const CGrayMapBlock &copy);
	CGrayMapBlock &operator=(const CGrayMapBlock &other);

public:
	int GetOffsetX(int x) const
	{
		// Allow this to go out of bounds
		//ASSERT((x - m_pt.m_x) == UO_BLOCK_OFFSET(x));
		return x - m_x;
	}
	int GetOffsetY(int y) const
	{
		return y - m_y;
	}

	const CUOMapMeter *GetTerrain(int xo, int yo) const
	{
		ASSERT((xo >= 0) && (xo < UO_BLOCK_SIZE));
		ASSERT((yo >= 0) && (yo < UO_BLOCK_SIZE));
		return &m_Terrain.m_Meter[yo * UO_BLOCK_SIZE + xo];
	}
	const CUOMapBlock *GetTerrainBlock() const
	{
		return &m_Terrain;
	}
};

class CGrayMulti : public CGrayCachedMulItem
{
	// Load all the relivant info for the
private:
	MULTI_TYPE m_id;
protected:
	CUOMultiItemRecHS *m_pItems;
	size_t m_iItemQty;
private:
	void Init()
	{
		m_id = MULTI_QTY;
		m_pItems = NULL;
		m_iItemQty = 0;
	}
	void Release()
	{
		if ( m_pItems )
		{
			delete[] m_pItems;
			Init();
		}
	}
public:
	static const char *m_sClassName;
	CGrayMulti()
	{
		Init();
	}
	explicit CGrayMulti(MULTI_TYPE id)
	{
		Init();
		Load(id);
	}
	virtual ~CGrayMulti()
	{
		Release();
	}

private:
	CGrayMulti(const CGrayMulti &copy);
	CGrayMulti &operator=(const CGrayMulti &other);

public:
	size_t Load(MULTI_TYPE id);

	MULTI_TYPE GetMultiID() const
	{
		return m_id;
	}
	size_t GetItemCount() const
	{
		return m_iItemQty;
	}
	const CUOMultiItemRecHS *GetItem(size_t i) const
	{
		ASSERT(i < m_iItemQty);
		return &m_pItems[i];
	}
};

#endif // _INC_CGRAYMAP_H
