//
// Crect.h
//

#ifndef _INC_CRECT_H
#define _INC_CRECT_H
#pragma once

///////////////////////////////////////////////////////////
// CPointBase

class CSector;
class CRegionBase;
class CRegionLinks;

struct CPointBase	// Non initialized 3d point.
{
public:
	static const LPCTSTR sm_szLoadKeys[];
	static const LPCTSTR sm_szDirs[DIR_QTY + 1];
	static const int sm_Moves[DIR_QTY + 1][2];
public:
	signed short m_x;	// equipped items dont need x,y
	signed short m_y;
	signed char m_z;	// This might be layer if equipped ? or equipped on corpse. Not used if in other container.
	BYTE m_map;			// another map? (only if top level.)

public:
	bool operator == ( const CPointBase & pt ) const
	{
		return( m_x == pt.m_x && m_y == pt.m_y && m_z == pt.m_z && m_map == pt.m_map );
	}

	bool operator != ( const CPointBase & pt ) const
	{
		return( ! ( *this == pt ));
	}

	const CPointBase operator += ( const CPointBase & pt )
	{
		m_x += pt.m_x;
		m_y += pt.m_y;
		m_z += pt.m_z;
		return( * this );
	}

	const CPointBase operator -= ( const CPointBase & pt )
	{
		m_x -= pt.m_x;
		m_y -= pt.m_y;
		m_z -= pt.m_z;
		return( * this );
	}

	void InitPoint();
	void ZeroPoint();
	int GetDistZ( const CPointBase & pt ) const;
	int GetDistZAdj( const CPointBase & pt ) const;
	int GetDistBase( const CPointBase & pt ) const; // Distance between points
	int GetDist( const CPointBase & pt ) const; // Distance between points
	int GetDist3D( const CPointBase & pt ) const; // 3D Distance between points

	bool IsValidXY() const;
	bool IsValidZ() const;
	bool IsValidPoint() const;

	void ValidatePoint();

	bool IsSame2D( const CPointBase & pt ) const;

	void Set( const CPointBase & pt );
	void Set(signed short x, signed short y, signed char z = 0, BYTE map = 0);
	size_t Read( TCHAR * pVal );

	TCHAR * WriteUsed( TCHAR * pszBuffer ) const;
	LPCTSTR WriteUsed() const;

	void Move( DIR_TYPE dir );
	void MoveN( DIR_TYPE dir, int amount );

	DIR_TYPE GetDir( const CPointBase & pt, DIR_TYPE DirDefault = DIR_QTY ) const; // Direction to point pt

	// Take a step directly toward the target.
	int StepLinePath( const CPointBase & ptSrc, int iSteps );

	CSector * GetSector() const;

#define REGION_TYPE_AREA  1
#define REGION_TYPE_ROOM  2
#define REGION_TYPE_HOUSE 4
#define REGION_TYPE_SHIP  8
#define REGION_TYPE_MULTI 12
	CRegionBase *GetRegion(BYTE bType) const;
	size_t GetRegions(BYTE bType, CRegionLinks &rlinks) const;

	long GetPointSortIndex() const
	{
		return( MAKELONG( m_x, m_y ));
	}

	bool r_WriteVal( LPCTSTR pszKey, CGString & sVal ) const;
	bool r_LoadVal( LPCTSTR pszKey, LPCTSTR pszArgs );
};

struct CPointMap : public CPointBase
{
	// A point in the world (or in a container) (initialized)
	CPointMap()
	{
		InitPoint();
	}
	CPointMap(signed short x, signed short y, signed char z = 0, BYTE map = 0)
	{
		m_x = x;
		m_y = y;
		m_z = z;
		m_map = map;
	}
	CPointMap & operator = ( const CPointBase & pt )
	{
		Set( pt );
		return( * this );
	}
	CPointMap( const CPointBase & pt )
	{
		Set( pt );
	}
	CPointMap( TCHAR * pVal )
	{
		Read( pVal );
	}
};

///////////////////////////////////////////////////////////
// CPointSort

struct CPointSort : public CPointMap
{
	CPointSort()
	{
		InitPoint();
	}
	CPointSort(signed short x, signed short y, signed char z = 0, BYTE map = 0)
	{
		m_x = x;
		m_y = y;
		m_z = z;
		m_map = map;
	}
	CPointSort( const CPointBase & pt )
	{
		Set( pt );
	}
	virtual ~CPointSort()	// just to make this dynamic
	{
	}
};

///////////////////////////////////////////////////////////
// CGRect

struct CGRect
{
	// Basic rectangle (may not be on the map)
public:
	int m_left;		// West (X = 0)
	int m_top;		// North (Y = 0)
	int m_right;	// East (non inclusive)
	int m_bottom;	// South (non inclusive)
	int m_map;

public:
	CPointBase GetRectCorner(DIR_TYPE dir) const;
	CSector *GetSector(int i) const;

	int GetWidth() const
	{
		return m_right - m_left;
	}
	int GetHeight() const
	{
		return m_bottom - m_top;
	}

	void SetRect(int left, int top, int right, int bottom, int map)
	{
		m_left = left;
		m_top = top;
		m_right = right;
		m_bottom = bottom;
		m_map = map;
		NormalizeRect();
	}
	void OffsetRect(int x, int y)
	{
		m_left += x;
		m_top += y;
		m_right += x;
		m_bottom += y;
	}
	void SetRectEmpty()
	{
		m_left = 0;
		m_top = 0;
		m_right = 0;
		m_bottom = 0;
		m_map = 0;
	}
	bool IsRectEmpty() const
	{
		return ((m_left >= m_right) || (m_top >= m_bottom));
	}

	bool IsInsideX(int x) const
	{
		return ((x >= m_left) && (x < m_right));
	}
	bool IsInsideY(int y) const
	{
		return ((y >= m_top) && (y < m_bottom));
	}
	bool IsInside(int x, int y, int map) const
	{
		return (IsInsideX(x) && IsInsideY(y) && (m_map == map));
	}
	bool IsInside2d(const CPointBase &pt) const
	{
		return IsInside(pt.m_x, pt.m_y, pt.m_map);
	}
	void UnionPoint(int x, int y)
	{
		// Inflate rect to include this point
		if ( x < m_left )
			m_left = x;
		if ( y < m_top )
			m_top = y;
		if ( x >= m_right )
			m_right = x + 1;
		if ( y >= m_bottom )
			m_bottom = y + 1;
	}

	bool IsInside(const CGRect &rect) const
	{
		return ((rect.m_left >= m_left) && (rect.m_top >= m_top) && (rect.m_right <= m_right) && (rect.m_bottom <= m_bottom) && (rect.m_map == m_map));
	}
	bool IsEqual(const CGRect &rect) const
	{
		return ((rect.m_left == m_left) && (rect.m_top == m_top) && (rect.m_right == m_right) && (rect.m_bottom == m_bottom) && (rect.m_map == m_map));
	}
	bool IsOverlapped(const CGRect &rect) const
	{
		return ((rect.m_left < m_right) && (rect.m_top < m_bottom) && (rect.m_right > m_left) && (rect.m_bottom > m_top) /*&& (rect.m_map == m_map)*/);
	}
	void UnionRect(const CGRect &rect)
	{
		// Inflate rect to include both rectangles
		if ( rect.IsRectEmpty() )
			return;

		if ( IsRectEmpty() )
		{
			*this = rect;
			return;
		}

		if ( rect.m_left < m_left )
			m_left = rect.m_left;
		if ( rect.m_top < m_top )
			m_top = rect.m_top;
		if ( rect.m_right > m_right )
			m_right = rect.m_right;
		if ( rect.m_bottom > m_bottom )
			m_bottom = rect.m_bottom;
		if ( m_map != rect.m_map )
			DEBUG_ERR(("Uniting regions from different maps\n"));
	}

	virtual void NormalizeRect()
	{
		if ( m_bottom < m_top )
		{
			int iTemp = m_bottom;
			m_bottom = m_top;
			m_top = iTemp;
		}
		if ( m_right < m_left )
		{
			int iTemp = m_right;
			m_right = m_left;
			m_left = iTemp;
		}
		if ( !g_MapList.IsMapSupported(m_map) )
			m_map = 0;
	}

	void NormalizeRectMax(int x, int y)
	{
		if ( m_left < 0 )
			m_left = 0;
		if ( m_top < 0 )
			m_top = 0;
		if ( m_right > x )
			m_right = x;
		if ( m_bottom > y )
			m_bottom = y;
	}

	CPointBase GetCenter() const
	{
		CPointBase pt;
		pt.m_x = static_cast<signed short>((m_left + m_right) / 2);
		pt.m_y = static_cast<signed short>((m_top + m_bottom) / 2);
		pt.m_z = 0;
		pt.m_map = static_cast<BYTE>(m_map);
		return pt;
	}

	size_t Read(LPCTSTR pszVal);
	LPCTSTR Write() const;
	TCHAR *Write(TCHAR *pszBuffer) const
	{
		snprintf(pszBuffer, 32, "%d,%d,%d,%d,%d", m_left, m_top, m_right, m_bottom, m_map);
		return pszBuffer;
	}
};

///////////////////////////////////////////////////////////
// CRectMap

struct CRectMap : public CGRect
{
public:
	void NormalizeRect();
	void NormalizeRectMax();

	bool IsValid() const
	{
		int iSizeX = GetWidth();
		if ( (iSizeX < 0) || (iSizeX > g_MapList.GetX(m_map)) )
			return false;

		int iSizeY = GetHeight();
		if ( (iSizeY < 0) || (iSizeY > g_MapList.GetY(m_map)) )
			return false;

		return true;
	}
};

#endif	// _INC_CRECT_H
