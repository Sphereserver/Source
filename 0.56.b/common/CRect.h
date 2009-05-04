//
// Crect.h
//

#ifndef _INC_CRECT_H
#define _INC_CRECT_H
#pragma once

#include "CString.h"
//#include "cscript.h"

class CRegionBase;
class CRegionLinks;
class CSector;

struct CPointBase	// Non initialized 3d point.
{
public:
	static LPCTSTR const sm_szLoadKeys[];
	static const int sm_Moves[DIR_QTY+1][2];
	static LPCTSTR sm_szDirs[DIR_QTY+1];
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
	int GetDistSightBase( const CPointBase & pt ) const; // Distance between points based on UO sight
	int GetDistSight( const CPointBase & pt ) const; // Distance between points based on UO sight
	int GetDist3D( const CPointBase & pt ) const; // 3D Distance between points

	bool IsValidZ() const;
	bool IsValidXY() const;
	bool IsValidPoint() const;
	bool IsCharValid() const;

	void ValidatePoint();

	bool IsSameMap( BYTE map ) const;

	bool IsSame2D( const CPointBase & pt ) const;

	void Set( const CPointBase & pt );
	void Set( WORD x, WORD y, signed char z = 0, unsigned char map = 0 );
	int Read( TCHAR * pVal );

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

