#ifndef _INC_CREGION_H
#define _INC_CREGION_H
#pragma once

#include "CRect.h"
#include "graymul.h"

class CObjBaseTemplate : public CGObListRec
{
	// A dynamic object of some sort.
private:
	CGrayUID	m_UID;		// How the server will refer to this. 0 = static item
	CGString	m_sName;	// unique name for the individual object.
	CPointMap	m_pt;		// List is sorted by m_z_sort.
protected:
	void DupeCopy( const CObjBaseTemplate * pObj )
	{
		// NOTE: Never copy m_UID
		ASSERT(pObj);
		m_sName = pObj->m_sName;
		m_pt = pObj->m_pt;
	}

	void SetUID( DWORD dwIndex )
	{
		// don't set container flags through here.
		m_UID.SetObjUID( dwIndex );	// Will have UID_F_ITEM as well.
	}

public:
	static const char *m_sClassName;
	CObjBaseTemplate()
	{
	}
	virtual ~CObjBaseTemplate()
	{
	}

private:
	CObjBaseTemplate(const CObjBaseTemplate& copy);
	CObjBaseTemplate& operator=(const CObjBaseTemplate& other);

public:
	CObjBaseTemplate * GetNext() const
	{
		return static_cast<CObjBaseTemplate *>(CGObListRec::GetNext());
	}
	CObjBaseTemplate * GetPrev() const
	{
		return static_cast<CObjBaseTemplate *>(CGObListRec::GetPrev());
	}

	CGrayUID GetUID() const			{ return m_UID; }
	bool IsItem() const				{ return m_UID.IsItem(); }
	bool IsChar() const				{ return m_UID.IsChar(); }
	bool IsItemInContainer() const	{ return m_UID.IsItemInContainer(); }
	bool IsItemEquipped() const		{ return m_UID.IsItemEquipped(); }
	bool IsDisconnected() const		{ return m_UID.IsObjDisconnected(); }
	bool IsTopLevel() const			{ return m_UID.IsObjTopLevel(); }
	bool IsValidUID() const			{ return m_UID.IsValidUID(); }
	bool IsDeleted() const;

	void SetContainerFlags( DWORD dwFlags = 0 )
	{
		m_UID.SetObjContainerFlags(dwFlags);
	}

	virtual int IsWeird() const;
	virtual CObjBaseTemplate * GetTopLevelObj() const = 0;

	CSector * GetTopSector() const
	{
		return GetTopLevelObj()->GetTopPoint().GetSector();
	}
	// Location

	LAYER_TYPE GetEquipLayer() const
	{
		return static_cast<LAYER_TYPE>(m_pt.m_z);
	}
	void SetEquipLayer( LAYER_TYPE layer )
	{
		SetContainerFlags( UID_O_EQUIPPED );
		m_pt.m_x = 0;	// these don't apply.
		m_pt.m_y = 0;
		// future: strongly typed enums will remove the need for this cast
		m_pt.m_z = static_cast<signed char>(layer); // layer equipped.
		m_pt.m_map = 0;
	}

	BYTE GetContainedLayer() const
	{
		// used for corpse or Restock count as well in Vendor container.
		return( m_pt.m_z );
	}
	void SetContainedLayer( BYTE layer )
	{
		// used for corpse or Restock count as well in Vendor container.
		m_pt.m_z = layer;
	}
	const CPointMap & GetContainedPoint() const
	{
		return( m_pt );
	}
	void SetContainedPoint( const CPointMap & pt )
	{
		SetContainerFlags( UID_O_CONTAINED );
		m_pt.m_x = pt.m_x;
		m_pt.m_y = pt.m_y;
		m_pt.m_z = LAYER_NONE;
		m_pt.m_map = 0;
	}

	void SetTopPoint( const CPointMap & pt )
	{
		SetContainerFlags(0);
		ASSERT( pt.IsValidPoint() );	// already checked b4.
		m_pt = pt;
	}
	const CPointMap & GetTopPoint() const
	{
		return( m_pt );
	}
	virtual void SetTopZ( signed char z )
	{
		m_pt.m_z = z;
	}
	signed char GetTopZ() const
	{
		return( m_pt.m_z );
	}
	unsigned char GetTopMap() const
	{
		return( m_pt.m_map );
	}

	void SetUnkPoint( const CPointMap & pt )
	{
		m_pt = pt;
	}

	// Distance and direction
	int GetTopDist( const CPointMap & pt ) const
	{
		return( GetTopPoint().GetDist( pt ));
	}

	int GetTopDist( const CObjBaseTemplate * pObj ) const
	{
		// don't check for logged out.
		// Assume both already at top level.
		ASSERT( pObj );
		if ( pObj->IsDisconnected())
			return( SHRT_MAX );
		return( GetTopPoint().GetDist( pObj->GetTopPoint()));
	}

	int GetTopDistSight( const CObjBaseTemplate * pObj ) const
	{
		// don't check for logged out.
		// Assume both already at top level.
		ASSERT( pObj );
		if ( pObj->IsDisconnected())
			return( SHRT_MAX );
		return( GetTopPoint().GetDistSight( pObj->GetTopPoint()));
	}

	int GetDist( const CObjBaseTemplate * pObj ) const
	{
		// logged out chars have infinite distance
		if ( pObj == NULL )
			return( SHRT_MAX );
		pObj = pObj->GetTopLevelObj();
		if ( pObj->IsDisconnected())
			return( SHRT_MAX );
		return( GetTopDist( pObj ));
	}

	int GetTopDist3D( const CObjBaseTemplate * pObj ) const // 3D Distance between points
	{
		// logged out chars have infinite distance
		// Assume both already at top level.
		ASSERT( pObj );
		if ( pObj->IsDisconnected())
			return( SHRT_MAX );
		return( GetTopPoint().GetDist3D( pObj->GetTopPoint()));
	}

	DIR_TYPE GetTopDir( const CObjBaseTemplate * pObj, DIR_TYPE DirDefault = DIR_QTY ) const
	{
		ASSERT( pObj );
		return( GetTopPoint().GetDir( pObj->GetTopPoint(), DirDefault ));
	}

	DIR_TYPE GetDir( const CObjBaseTemplate * pObj, DIR_TYPE DirDefault = DIR_QTY ) const
	{
		ASSERT( pObj );
		pObj = pObj->GetTopLevelObj();
		return( GetTopDir( pObj, DirDefault ));
	}

	// Names
	LPCTSTR GetIndividualName() const
	{
		return( m_sName );
	}
	bool IsIndividualName() const
	{
		return( ! m_sName.IsEmpty());
	}
	virtual LPCTSTR GetName() const
	{
		return( m_sName );
	}
	virtual bool SetName( LPCTSTR pszName )
	{
		// NOTE: Name length <= MAX_NAME_SIZE
		if ( !pszName )
			return false;
		m_sName = pszName;
		return true;
	}
};

inline DIR_TYPE GetDirTurn( DIR_TYPE dir, int offset )
{
	// Turn in a direction.
	// +1 = to the right.
	// -1 = to the left.
	ASSERT((offset + DIR_QTY + dir) >= 0);
	offset += DIR_QTY + dir;
	offset %= DIR_QTY;
	return static_cast<DIR_TYPE>(offset);
}

struct CGRect			// Basic rectangle. (May not be on the map)
{						// Similar to _WIN32 RECT
public:
	int m_left;		// West	 x=0
	int m_top;		// North y=0
	int m_right;	// East	( NON INCLUSIVE !)
	int m_bottom;	// South ( NON INCLUSIVE !)
	int m_map;
public:
	int GetWidth() const { return( m_right - m_left ); }
	int GetHeight() const { return( m_bottom - m_top ); }

	bool IsRectEmpty() const
	{
		return( m_left >= m_right || m_top >= m_bottom );
	}
	void SetRectEmpty()
	{
		m_left = m_top = 0;	// 0x7ffe
		m_right = m_bottom = 0;
		m_map = 0;
	}

	void OffsetRect( int x, int y )
	{
		m_left += x;
		m_top += y;
		m_right += x;
		m_bottom += y;
	}
	void UnionPoint( int x, int y )
	{
		// Inflate this rect to include this point.
		// NON inclusive rect! 
		if ( x	< m_left	) m_left = x;
		if ( y	< m_top		) m_top = y;
		if ( x	>= m_right	) m_right = x+1;
		if ( y	>= m_bottom	) m_bottom = y+1;
	}

	bool IsInsideX( int x ) const
	{	// non-inclusive
		return( x >= m_left && x < m_right );
	}
	bool IsInsideY( int y ) const
	{	// non-inclusive
		return( y >= m_top && y < m_bottom );
	}
	bool IsInside( int x, int y, int map ) const
	{
		// NON inclusive rect! Is the point in the rectangle ?
		return( IsInsideX(x) &&	IsInsideY(y) && ( m_map == map ));
	}
	bool IsInside2d( const CPointBase & pt ) const
	{
		// NON inclusive rect! Is the point in the rectangle ?
		return( IsInside( pt.m_x, pt.m_y, pt.m_map ) );
	}

	void UnionRect( const CGRect & rect )
	{
		// Inflate this rect to include both rectangles.
		// ASSUME: Normalized rect
		if ( rect.IsRectEmpty())
			return;
		if ( IsRectEmpty())
		{
			*this = rect;
			return;
		}
		if ( rect.m_left	< m_left	) m_left = rect.m_left;
		if ( rect.m_top		< m_top		) m_top = rect.m_top;
		if ( rect.m_right	> m_right	) m_right = rect.m_right;
		if ( rect.m_bottom	> m_bottom	) m_bottom = rect.m_bottom;
		if ( m_map != rect.m_map )
		{
			DEBUG_ERR(("Uniting regions from different maps!\n"));
		}
	}
	bool IsInside( const CGRect & rect ) const
	{
		// Is &rect inside me ?
		// ASSUME: Normalized rect
		if ( rect.m_map != m_map ) return false;
		if ( rect.m_left	< m_left	) 
			return( false );
		if ( rect.m_top		< m_top		)
			return( false );
		if ( rect.m_right	> m_right	)
			return( false );
		if ( rect.m_bottom	> m_bottom	)
			return( false );
		return( true );
	}
	bool IsOverlapped( const CGRect & rect ) const
	{
		// are the 2 rects overlapped at all ?
		// NON inclusive rect.
		// ASSUME: Normalized rect
//		if ( rect.m_map != m_map ) return false;
		if ( rect.m_left	>= m_right	) 
			return( false );
		if ( rect.m_top		>= m_bottom	)
			return( false );
		if ( rect.m_right	<= m_left	)
			return( false );
		if ( rect.m_bottom	<= m_top	)
			return( false );
		return( true );
	}
	bool IsEqual( const CGRect & rect ) const
	{
		return m_left == rect.m_left &&
			   m_top == rect.m_top &&
			   m_right == rect.m_right &&
			   m_bottom == rect.m_bottom &&
			   m_map == rect.m_map;
	}
	virtual void NormalizeRect()
	{
		if ( m_bottom < m_top )
		{
			int wtmp = m_bottom;
			m_bottom = m_top;
			m_top = wtmp;
		}
		if ( m_right < m_left )
		{
			int wtmp = m_right;
			m_right = m_left;
			m_left = wtmp;
		}
		if (( m_map < 0 ) || ( m_map >= 256 )) m_map = 0;
		if ( !g_MapList.m_maps[m_map] ) m_map = 0;
	}

	void SetRect( int left, int top, int right, int bottom, int map )
	{
		m_left = left;
		m_top = top;
		m_right = right;
		m_bottom = bottom;
		m_map = map;
		NormalizeRect();
	}

	void NormalizeRectMax( int cx, int cy )
	{
		if ( m_left < 0 )
			m_left = 0;
		if ( m_top < 0 )
			m_top = 0;
		if ( m_right > cx )
			m_right = cx;
		if ( m_bottom > cy )
			m_bottom = cy;
	}

	size_t Read( LPCTSTR pVal );
	TCHAR * Write( TCHAR * pBuffer ) const
	{
		sprintf(pBuffer, "%d,%d,%d,%d,%d", m_left, m_top, m_right, m_bottom, m_map);
		return( pBuffer );
	}
	LPCTSTR Write() const;

	CPointBase GetCenter() const
	{
		CPointBase pt;
		pt.m_x = static_cast<short>(( m_left + m_right ) / 2);
		pt.m_y = static_cast<short>((m_top + m_bottom) / 2);
		pt.m_z = 0;
		pt.m_map = static_cast<unsigned char>(m_map);
		return( pt );
	}

	CPointBase GetRectCorner( DIR_TYPE dir ) const;
	CSector * GetSector( int i ) const;	// ge all the sectors that make up this rect.
};

struct CRectMap : public CGRect
{
public:

	bool IsValid() const
	{
		int iSizeX = GetWidth();
		if ( iSizeX < 0 || iSizeX > g_MapList.GetX(m_map) )
			return( false );
		int iSizeY = GetHeight();
		if ( iSizeY < 0 || iSizeY > g_MapList.GetY(m_map) )
			return( false );
		return( true );
	}

	void NormalizeRect();
	void NormalizeRectMax();
};

class CGRegion
{
	// A bunch of rectangles forming an area.
public:
	static const char *m_sClassName;
	CGRect m_rectUnion;	// The union rectangle.
	CGTypedArray<CGRect, const CGRect&> m_Rects;
	bool IsRegionEmpty() const
	{
		return( m_rectUnion.IsRectEmpty());
	}
	void EmptyRegion()
	{
		m_rectUnion.SetRectEmpty();
		m_Rects.Empty();
	}
	size_t GetRegionRectCount() const;
	CGRect & GetRegionRect(size_t i);
	const CGRect & GetRegionRect(size_t i) const;
	virtual bool AddRegionRect( const CGRect & rect );

	CPointBase GetRegionCorner( DIR_TYPE dir = DIR_QTY ) const;
	bool IsInside2d( const CPointBase & pt ) const;

	bool IsOverlapped( const CGRect & rect ) const;
	bool IsInside( const CGRect & rect ) const;

	bool IsInside( const CGRegion * pRegionIsSmaller ) const;
	bool IsOverlapped( const CGRegion * pRegionTest ) const;
	bool IsEqualRegion( const CGRegion * pRegionTest ) const;

	CSector * GetSector( int i ) const	// get all the sectors that make up this rect.
	{
		return m_rectUnion.GetSector(i);
	}

public:
	CGRegion();
	virtual ~CGRegion() { };

private:
	CGRegion(const CGRegion& copy);
	CGRegion& operator=(const CGRegion& other);
};

class CRegionLinks : public CGPtrTypeArray<CRegionBase*>
{
	//just named class for this, maybe something here later
public:
	CRegionLinks() { };

private:
	CRegionLinks(const CRegionLinks& copy);
	CRegionLinks& operator=(const CRegionLinks& other);
};

enum RTRIG_TYPE
{
	// XTRIG_UNKNOWN	= some named trigger not on this list.
	RTRIG_CLIPERIODIC=1,		// happens to each client.
	RTRIG_ENTER,
	RTRIG_EXIT,
	RTRIG_REGPERIODIC,	// regional periodic. Happens if just 1 or many clients)
	RTRIG_STEP,
	RTRIG_QTY
};

class CRegionBase : public CResourceDef, public CGRegion
{
	// region of the world of arbitrary size and location.
	// made of (possibly multiple) rectangles.
	// RES_ROOM or base for RES_AREA
private:
	CGString	m_sName;	// Name of the region.
	CGString	m_sGroup;

#define REGION_ANTIMAGIC_ALL		0x000001	// All magic banned here.
#define REGION_ANTIMAGIC_RECALL_IN	0x000002	// Teleport,recall in to this, and mark
#define REGION_ANTIMAGIC_RECALL_OUT	0x000004	// can't recall out of here.
#define REGION_ANTIMAGIC_GATE		0x000008
#define REGION_ANTIMAGIC_TELEPORT	0x000010	// Can't teleport into here.
#define REGION_ANTIMAGIC_DAMAGE		0x000020	// just no bad magic here

#define REGION_FLAG_SHIP			0x000040	// This is a ship region. ship commands
#define REGION_FLAG_NOBUILDING		0x000080	// No building in this area

#define REGION_FLAG_ANNOUNCE		0x000200	// Announce to all who enter.
#define REGION_FLAG_INSTA_LOGOUT	0x000400	// Instant Log out is allowed here. (hotel)
#define REGION_FLAG_UNDERGROUND		0x000800	// dungeon type area. (no weather)
#define REGION_FLAG_NODECAY			0x001000	// Things on the ground don't decay here.

#define REGION_FLAG_SAFE			0x002000	// This region is safe from all harm.
#define REGION_FLAG_GUARDED			0x004000	// try TAG.GUARDOWNER
#define REGION_FLAG_NO_PVP			0x008000	// Players cannot directly harm each other here.
#define REGION_FLAG_ARENA			0x010000	// Anything goes. no murder counts or crimes.

	DWORD m_dwFlags;

public:
	static const char *m_sClassName;
	CPointMap m_pt;			// safe point in the region. (for teleporting to)
	int m_iLinkedSectors;	// just for statistics tracking. How many sectors are linked ?
	int m_iModified;

	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szTrigName[RTRIG_QTY+1];
	static LPCTSTR const sm_szVerbKeys[];

	CResourceRefArray		m_Events;	// trigger [REGION x] when entered or exited RES_REGIONTYPE
	CVarDefMap				m_TagDefs;		// attach extra tags here.
	CVarDefMap				m_BaseDefs;		// New Variable storage system

	TRIGRET_TYPE OnRegionTrigger( CTextConsole * pChar, RTRIG_TYPE trig );

public:
	LPCTSTR GetDefStr( LPCTSTR pszKey, bool fZero = false ) const
	{
		return m_BaseDefs.GetKeyStr( pszKey, fZero );
	}

	INT64 GetDefNum( LPCTSTR pszKey, bool fZero = false ) const
	{
		return m_BaseDefs.GetKeyNum( pszKey, fZero );
	}

	void SetDefNum(LPCTSTR pszKey, INT64 iVal, bool fZero = true)
	{
		m_BaseDefs.SetNum(pszKey, iVal, fZero);
	}

	void SetDefStr(LPCTSTR pszKey, LPCTSTR pszVal, bool fQuoted = false, bool fZero = true)
	{
		m_BaseDefs.SetStr(pszKey, fQuoted, pszVal, fZero);
	}

	void DeleteDef(LPCTSTR pszKey)
	{
		m_BaseDefs.DeleteKey(pszKey);
	}

private:
	bool SendSectorsVerb( LPCTSTR pszVerb, LPCTSTR pszArgs, CTextConsole * pSrc ); // distribute to the CSectors

public:
	virtual bool RealizeRegion();
	void UnRealizeRegion();
#define REGMOD_FLAGS	0x0001
#define REGMOD_EVENTS	0x0002
#define REGMOD_TAGS		0x0004
#define REGMOD_NAME		0x0008
#define REGMOD_GROUP	0x0010

	void	SetModified( int iModFlag );
	void SetName( LPCTSTR pszName );
	LPCTSTR GetName() const
	{
		return( m_sName );
	}
	const CGString & GetNameStr() const
	{
		return( m_sName );
	}

	void r_WriteBase( CScript & s );

	virtual bool r_LoadVal( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc );
	virtual void r_WriteBody( CScript & s, LPCTSTR pszPrefix );
	virtual void r_WriteModified( CScript & s );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script
	virtual void r_Write( CScript & s );

	bool AddRegionRect( const CRectMap & rect );
	bool SetRegionRect( const CRectMap & rect )
	{
		EmptyRegion();
		return AddRegionRect( rect );
	}
	DWORD GetRegionFlags() const
	{
		return( m_dwFlags );
	}
	bool IsFlag( DWORD dwFlags ) const
	{	// REGION_FLAG_GUARDED
		return(( m_dwFlags & dwFlags ) ? true : false );
	}
	bool IsGuarded() const;
	void SetRegionFlags( DWORD dwFlags )
	{
		m_dwFlags |= dwFlags;
	}
	void TogRegionFlags( DWORD dwFlags, bool fSet )
	{
		if ( fSet )
			m_dwFlags |= dwFlags;
		else
			m_dwFlags &= ~dwFlags;
		SetModified( REGMOD_FLAGS );
	}

	bool CheckAntiMagic( SPELL_TYPE spell ) const;
	virtual bool IsValid() const
	{
		return( m_sName.IsValid());
	}

	bool	MakeRegionName();

public:
	explicit CRegionBase( RESOURCE_ID rid, LPCTSTR pszName = NULL );
	virtual ~CRegionBase();

private:
	CRegionBase(const CRegionBase& copy);
	CRegionBase& operator=(const CRegionBase& other);
};

class CRandGroupDef;

class CRegionWorld : public CRegionBase
{
	// A region with extra properties.
	// [AREA] = RES_AREA
public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];

public:
	const CRandGroupDef * FindNaturalResource( int /* IT_TYPE */ type ) const;

public:
	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual bool r_LoadVal( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc );
	virtual void r_WriteBody( CScript &s, LPCTSTR pszPrefix );
	virtual void r_WriteModified( CScript &s );
	virtual void r_Write( CScript & s );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script

public:
	explicit CRegionWorld( RESOURCE_ID rid, LPCTSTR pszName = NULL );
	virtual ~CRegionWorld();

private:
	CRegionWorld(const CRegionWorld& copy);
	CRegionWorld& operator=(const CRegionWorld& other);
};

class CTeleport : public CPointSort	// The static world teleporters. GRAYMAP.SCP
{
	// Put a built in trigger here ? can be Array sorted by CPointMap.
public:
	static const char *m_sClassName;
	bool bNpc;
	CPointMap m_ptDst;

public:
	explicit CTeleport( const CPointMap & pt ) : CPointSort(pt)
	{
		ASSERT( pt.IsValidPoint());
		m_ptDst = pt;
		bNpc = false;
	}

	explicit CTeleport( TCHAR * pszArgs );

	virtual ~CTeleport()
	{
	}

private:
	CTeleport(const CTeleport& copy);
	CTeleport& operator=(const CTeleport& other);

public:
	bool RealizeTeleport();
};

class CStartLoc		// The start locations for creating a new char. GRAY.INI
{
public:
	static const char *m_sClassName;
	CGString m_sArea;	// Area/City Name = Britain or Occlo
	CGString m_sName;	// Place name = Castle Britannia or Docks
	CPointMap m_pt;
	int iClilocDescription; //Only for clients 7.00.13 +

public:
	explicit CStartLoc( LPCTSTR pszArea )
	{
		m_sArea = pszArea;
		iClilocDescription = 1149559;
	}

private:
	CStartLoc(const CStartLoc& copy);
	CStartLoc& operator=(const CStartLoc& other);
};

#endif // _INC_CREGION_H
