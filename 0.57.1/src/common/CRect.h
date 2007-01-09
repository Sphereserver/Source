//
// Crect.h
//

#ifndef _INC_CRECT_H
#define _INC_CRECT_H
#pragma once

#include "CString.h"

class CRegionBase;
class CRegionLinks;
class CSector;

struct CPointBase	// Non initialized 3d point.
{
public:
	static LPCTSTR const sm_szLoadKeys[];
	static const int sm_Moves[DIR_QTY+1][2];
	static LPCTSTR const sm_szDirs[DIR_QTY+1];
public:
	signed short m_x;	// equipped items dont need x,y
	signed short m_y;
	signed char m_z;	// This might be layer if equipped ? or equipped on corpse. Not used if in other container.
	BYTE m_map;			// another map? (only if top level.)

public:
	void InitPoint()
	{
		m_x = -1;	// invalid location.
		m_y = -1;
		m_z = 0;
		m_map = 0;
	}
	void ZeroPoint()
	{
		m_x = 0;	// invalid location.
		m_y = 0;
		m_z = 0;
		m_map = 0;
	}
	int GetDistZ( const CPointBase & pt ) const
	{
		return( abs(m_z-pt.m_z));
	}
	int GetDistZAdj( const CPointBase & pt ) const
	{
		return( GetDistZ(pt) / (PLAYER_HEIGHT/2) );
	}
	int GetDistBase( const CPointBase & pt ) const // Distance between points
	{
		// Do not consider z or m_map.
		int dx = abs(m_x-pt.m_x);
		int dy = abs(m_y-pt.m_y);
		return( max( dx, dy ));
		// Return the real distance return((int) sqrt(dx*dx+dy*dy+dz*dz));
	}
	int GetDist( const CPointBase & pt ) const; // Distance between points
	int GetDist3D( const CPointBase & pt ) const; // 3D Distance between points

	bool IsValidZ() const
	{
		return( m_z > -UO_SIZE_Z && m_z < UO_SIZE_Z );
	}
	bool IsValidXY() const
	{
		if ( m_x < 0 || m_x >= g_MapList.GetX(m_map) )
			return false;
		if ( m_y < 0 || m_y >= g_MapList.GetY(m_map) )
			return false;
		return true;
	}
	bool IsValidPoint() const
	{
		return( IsValidXY() && IsValidZ());
	}
	bool IsCharValid() const
	{
		if ( m_z <= -UO_SIZE_Z || m_z >= UO_SIZE_Z )
			return false;
		if ( m_x <= 0 || m_x >= g_MapList.GetX(m_map) )
			return false;
		if ( m_y <= 0 || m_y >= g_MapList.GetY(m_map) )
			return false;
		return true;
	}

	void ValidatePoint()
	{
		if ( m_x < 0 ) m_x = 0;
		if ( m_x >= g_MapList.GetX(m_map) ) m_x = g_MapList.GetX(m_map)-1;
		if ( m_y < 0 ) m_y = 0;
		if ( m_y >= g_MapList.GetY(m_map) ) m_y = g_MapList.GetY(m_map)-1;
	}

	bool IsSameMap( BYTE map ) const
	{
		return( map == m_map );
	}

	bool IsSame2D( const CPointBase & pt ) const
	{
		return( m_x == pt.m_x && m_y == pt.m_y );
	}
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
	void Set( const CPointBase & pt )
	{
		m_x = pt.m_x;
		m_y = pt.m_y;
		m_z = pt.m_z;
		m_map = pt.m_map;
	}
	void Set( WORD x, WORD y, signed char z = 0, unsigned char map = 0 )
	{
		m_x = x;
		m_y = y;
		m_z = z;
		m_map = map;
	}
	int Read( TCHAR * pVal );

	TCHAR * WriteUsed( TCHAR * pszBuffer ) const
	{
		if ( m_map )
		{
			sprintf(pszBuffer, "%d,%d,%d,%d", m_x, m_y, m_z, m_map);
		}
		else if ( m_z )
		{
			sprintf(pszBuffer, "%d,%d,%d", m_x, m_y, m_z);
		}
		else
		{
			sprintf(pszBuffer, "%d,%d", m_x, m_y);
		}
		return pszBuffer;
	}
	LPCTSTR WriteUsed() const;

	void Move( DIR_TYPE dir )
	{
		// Move a point in a direction.
		m_x += sm_Moves[dir][0];
		m_y += sm_Moves[dir][1];
	}
	void MoveN( DIR_TYPE dir, int amount )
	{
		// Move a point in a direction.
		m_x += sm_Moves[dir][0] * amount;
		m_y += sm_Moves[dir][1] * amount;
	}

	DIR_TYPE GetDir( const CPointBase & pt, DIR_TYPE DirDefault = DIR_QTY ) const; // Direction to point pt

	// Take a step directly toward the target.
	int StepLinePath( const CPointBase & ptSrc, int iSteps );

	CSector * GetSector() const;

#define REGION_TYPE_AREA  1
#define REGION_TYPE_ROOM  2
#define REGION_TYPE_MULTI 4
	CRegionBase * GetRegion( DWORD dwType ) const;
	int GetRegions( DWORD dwType, CRegionLinks & rlinks ) const;

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
	CPointMap() { InitPoint(); }
	CPointMap( WORD x, WORD y, signed char z = 0, unsigned char map = 0 )
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

struct CPointSort : public CPointMap
{
	CPointSort() { InitPoint(); }
	CPointSort( WORD x, WORD y, signed char z = 0, unsigned char map = 0 )
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

#endif	// _INC_CRECT_H

