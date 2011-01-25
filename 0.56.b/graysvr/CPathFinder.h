//
//	CPathFinder
//		pathfinding algorytm based on AStar (A*) algorithm
//		based on A* Pathfinder (Version 1.71a) by Patrick Lester, pwlester@policyalmanac.org
//
#ifndef PATH_FINDER_H
#define PATH_FINDER_H

#include "../graysvr/graysvr.h"
#include <deque>
#include <list>
#include <algorithm>

using std::deque;
using std::list;

#define PATH_SIZE (UO_MAP_VIEW_SIGHT*2)	// limit NPC view by one screen (both sides)

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
	CPathFinderPoint(const CPointMap& pt, CPointMap& parent);

private:
	CPathFinderPoint(const CPathFinderPoint& copy);
	CPathFinderPoint& operator=(const CPathFinderPoint& other);

public:
	const CPathFinderPoint* GetParent() const;
	CPathFinderPoint* GetPoint();
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
	CPathFinderPoint m_Points[PATH_SIZE][PATH_SIZE];
	std::deque<CPathFinderPointRef> m_Opened;
	std::deque<CPathFinderPointRef> m_Closed;

	std::deque<CPointMap> m_LastPath;

	int m_RealX;
	int m_RealY;

	CChar *m_pChar;
	CPointMap m_Target;

protected:
	void Clear();
	unsigned long Heuristic(CPathFinderPointRef& Pt1,CPathFinderPointRef& Pt2);
	void GetChildren(CPathFinderPointRef& Point, std::list<CPathFinderPointRef>& ChildrenRefList );
	void FillMap();	// prepares map with walkable statuses
};



#endif
