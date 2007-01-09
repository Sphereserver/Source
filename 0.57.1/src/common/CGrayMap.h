//
// CgrayMap.H
//

#ifndef _INC_CGRAYMAP_H
#define _INC_CGRAYMAP_H

class CGrayCachedMulItem
{
private:
	CServTime m_timeRef;		// When in world.GetTime() was this last referenced.
public:
	static const char *m_sClassName;
	void InitCacheTime()
	{
		m_timeRef.Init();
	}
	CGrayCachedMulItem()
	{
		InitCacheTime();
	}
	bool IsTimeValid() const
	{
		return( m_timeRef.IsTimeValid());
	}
	void HitCacheTime()
	{
		// When in g_World.GetTime() was this last referenced.
		m_timeRef = CServTime::GetCurrentTime();
	}
	int GetCacheAge() const
	{
		// In TICK_PER_SEC or milliseconds
		return( CServTime::GetCurrentTime() - m_timeRef );
	}
};

class CGrayStaticsBlock
{
private:
	int m_iStatics;
	CUOStaticItemRec * m_pStatics;	// dyn alloc array block.
public:
	void LoadStatics(DWORD dwBlockIndex, int map);
public:
	static const char *m_sClassName;
	CGrayStaticsBlock()
	{
		m_iStatics = 0;
		m_pStatics = NULL;
	}
	~CGrayStaticsBlock()
	{
		if ( m_pStatics ) delete [] m_pStatics;
	}
	int GetStaticQty() const
	{
		return( m_iStatics );
	}
	const CUOStaticItemRec * GetStatic( int i ) const
	{
		return &m_pStatics[i];
	}
	bool IsStaticPoint( int i, int xo, int yo ) const;
};

struct CGrayMapBlocker
{
	DWORD m_dwBlockFlags;	// How does this item block ? CAN_I_PLATFORM
	WORD m_wTile;			// TERRAIN_QTY + id.
	signed char m_z;		// Top of a solid object. or bottom of non blocking one.
};

struct CGrayMapBlockState
{
	// Go through the list of stuff at this location to decide what is  blocking us and what is not.
	//  dwBlockFlags = what we can pass thru. doors, water, walls ?
	//		CAN_C_GHOST	= Moves thru doors etc.	- CAN_I_DOOR = UFLAG4_DOOR						
	//		CAN_C_SWIM = walk thru water - CAN_I_WATER = UFLAG1_WATER
	//		CAN_C_WALK = it is possible that i can step up. - CAN_I_PLATFORM = UFLAG2_PLATFORM
	//		CAN_C_PASSWALLS	= walk through all blcking items - CAN_I_BLOCK = UFLAG1_BLOCK
	//		CAN_C_FLY  = gravity does not effect me. -  CAN_I_CLIMB = UFLAG2_CLIMBABLE
	//		CAN_C_FIRE_IMMUNE = i can walk into lava etc. - CAN_I_FIRE = UFLAG1_DAMAGE

	const DWORD m_dwBlockFlags;	// The block flags we can overcome.	
	const signed char m_z;	// the z we start at. (stay at if we are flying)
	const int m_iHeight;		// The height we need to stand here.
	const signed char m_zClimb; // We can climb at this height
	
	signed char m_zClimbHeight;	// return item climb height here

	CGrayMapBlocker m_Top;		// What would be over my head.
	CGrayMapBlocker m_Bottom;	// What i would be standing on.
	CGrayMapBlocker m_Lowest;	// the lowest item we have found.	

public:
	CGrayMapBlockState(DWORD dwBlockFlags, signed char m_z, int iHeight = PLAYER_HEIGHT, signed char zClimb = 0);
	bool IsUsableZ( signed char zBottom, signed char zHeightEstimate ) const;
	void CheckTile( DWORD dwItemBlockFlags, signed char zBottom, signed char zheight, WORD wID );
	void CheckTile_Item( DWORD dwItemBlockFlags, signed char zBottom, signed char zheight, WORD wID );
	void CheckTile_Terrain( DWORD dwItemBlockFlags, signed char z, WORD wID );
};

class CGrayMapBlock :	// Cache this from the MUL files. 8x8 block of the world.
	public CPointSort	// The upper left corner. (ignore z) sort by this
{
protected:
	int		m_map;

private:
	CUOMapBlock m_Terrain;

public:
	static const char *m_sClassName;
	CGrayStaticsBlock	m_Statics;
	CGrayCachedMulItem	m_CacheTime;	// keep track of the use time of this. (client does not care about this)
private:
	// NOTE: This will "throw" on failure !
	void Load(int bx, int by);

public:
	CGrayMapBlock(const CPointMap &pt) :
		CPointSort(pt)	// The upper left corner.
	{
		m_map = pt.m_map;
		Load(pt.m_x/UO_BLOCK_SIZE, pt.m_y/UO_BLOCK_SIZE);
	}

	virtual ~CGrayMapBlock()
	{
	}

	int GetOffsetX( int x ) const
	{
		return( x - m_x );
	}
	int GetOffsetY( int y ) const
	{
		return( y - m_y );
	}

	const CUOMapMeter * GetTerrain( int xo, int yo ) const
	{
		if (( xo < 0 ) || ( xo >= UO_BLOCK_SIZE ) || ( yo < 0 ) || ( yo >= UO_BLOCK_SIZE ))
			return NULL;
		return &m_Terrain.m_Meter[yo*UO_BLOCK_SIZE + xo];
	}
};

class CGrayMulti : public CGrayCachedMulItem
{
	// Load all the relivant info for the
private:
	MULTI_TYPE m_id;
	CUOMultiItemRec * m_pItems;
	int m_iItemQty;
private:
	void Init()
	{
		m_id = MULTI_QTY;
		m_pItems = NULL;
		m_iItemQty = 0;
	}
	void ReleaseMulti()
	{
		if ( m_pItems )
		{
			delete [] m_pItems;
			Init();
		}
	}
public:
	static const char *m_sClassName;
	int Load( MULTI_TYPE id );
	CGrayMulti()
	{
		Init();
	}
	CGrayMulti( MULTI_TYPE id )
	{
		Init();
		Load( id );
	}
	MULTI_TYPE GetMultiID() const
	{
		return( m_id );
	}
	int GetItemCount() const
	{
		return( m_iItemQty );
	}
	const CUOMultiItemRec * GetItem( int i ) const
	{
		return m_pItems+i;
	}
	virtual ~CGrayMulti()
	{
		ReleaseMulti();
	}
};

#endif // _INC_CGRAYMAP_H