//
//	CPathFinder
//		pathfinding algorytm based on AStar (A*) algorithm
//		based on A* Pathfinder (Version 1.71a) by Patrick Lester, pwlester@policyalmanac.org
//
#ifndef _INC_CPATHFINDER_H
#define _INC_CPATHFINDER_H
#pragma once

#include "../graysvr/graysvr.h"
#ifndef _WIN32
	#include <deque>
	#include <list>
	#include <algorithm>
#endif

using std::deque;
using std::list;

class CPathFinderPointRef;

class CPathFinderPoint : public CPointMap
{
protected:
	CPathFinderPoint* m_Parent;
public:
	bool m_Walkable;
	int FValue;
	int GValue;
	int HValue;

public:
	CPathFinderPoint();
	explicit CPathFinderPoint(const CPointMap& pt);

private:
	CPathFinderPoint(const CPathFinderPoint& copy);
	CPathFinderPoint& operator=(const CPathFinderPoint& other);

public:
	const CPathFinderPoint* GetParent() const;
	void SetParent(CPathFinderPointRef& pt);

	bool operator < (const CPathFinderPoint& pt) const;
};

class CPathFinderPointRef
{
public:
	CPathFinderPoint* m_Point;

public:
	CPathFinderPointRef() : m_Point(0)
	{
	}

	explicit CPathFinderPointRef(CPathFinderPoint& Pt)
	{
		m_Point = &Pt;
	}

public:
	CPathFinderPointRef& operator = ( const CPathFinderPointRef& Pt )
	{
		m_Point = Pt.m_Point;
		return *this;
	}

	bool operator < (const CPathFinderPointRef& Pt) const
	{
		return m_Point->FValue < Pt.m_Point->FValue;
	}

	bool operator == ( const CPathFinderPointRef& Pt )
	{
		return Pt.m_Point == m_Point;
	}
};

class CPathFinder
{
public:
	static const char *m_sClassName;

	#define PATH_NONEXISTENT 0
	#define PATH_FOUND 1


	#define PATH_WALKABLE 1
	#define PATH_UNWALKABLE 0

	CPathFinder(CChar *pChar, CPointMap ptTarget);
	~CPathFinder();

private:
	CPathFinder(const CPathFinder& copy);
	CPathFinder& operator=(const CPathFinder& other);

public:
	int FindPath();
	CPointMap ReadStep(size_t Step = 0);
	size_t LastPathSize();
	void ClearLastPath();

protected:
	CPathFinderPoint m_Points[MAX_NPC_PATH_STORAGE_SIZE][MAX_NPC_PATH_STORAGE_SIZE];
	std::deque<CPathFinderPointRef> m_Opened;
	std::deque<CPathFinderPointRef> m_Closed;

	std::deque<CPointMap> m_LastPath;

	signed short m_RealX;
	signed short m_RealY;

	CChar *m_pChar;
	CPointMap m_Target;

protected:
	void Clear();
	int Heuristic(CPathFinderPointRef &pt1, CPathFinderPointRef &pt2);
	void GetChildren(CPathFinderPointRef& Point, std::list<CPathFinderPointRef>& ChildrenRefList );
	void FillMap();	// prepares map with walkable statuses
};

#endif	// _INC_CPATHFINDER_H
