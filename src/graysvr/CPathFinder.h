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
public:
	CPathFinderPoint();
	explicit CPathFinderPoint(const CPointMap pt);

public:
	CPathFinderPoint *m_pParent;
	bool m_fWalkable;
	int iValF;
	int iValG;
	int iValH;

private:
	CPathFinderPoint(const CPathFinderPoint &copy);
	CPathFinderPoint &operator=(const CPathFinderPoint &other);
};

class CPathFinderPointRef
{
public:
	CPathFinderPointRef()
	{
		m_pPoint = 0;
	}
	explicit CPathFinderPointRef(CPathFinderPoint &pt)
	{
		m_pPoint = &pt;
	}

public:
	CPathFinderPoint *m_pPoint;

public:
	CPathFinderPointRef &operator=(const CPathFinderPointRef &pt)
	{
		m_pPoint = pt.m_pPoint;
		return *this;
	}
	bool operator < (const CPathFinderPointRef &pt) const
	{
		return (m_pPoint->iValF < pt.m_pPoint->iValF);
	}
	bool operator == (const CPathFinderPointRef &pt) const
	{
		return (m_pPoint == pt.m_pPoint);
	}
};

class CPathFinder
{
public:
	static const char *m_sClassName;

	CPathFinder(CChar *pChar, CPointMap ptTarget);

protected:
	CPathFinderPoint m_Points[MAX_NPC_PATH_STORAGE_SIZE][MAX_NPC_PATH_STORAGE_SIZE];
	std::deque<CPathFinderPointRef> m_Opened;
	std::deque<CPathFinderPointRef> m_Closed;
	std::deque<CPointMap> m_LastPath;

	signed short m_RealX;
	signed short m_RealY;

	CChar *m_pChar;
	CPointMap m_ptTarget;

public:
	bool FindPath();

	CPointMap ReadStep(signed short i) const
	{
		return m_LastPath[i];
	}

	signed short LastPathSize() const
	{
		return static_cast<signed short>(m_LastPath.size());
	}

	void ClearLastPath()
	{
		m_LastPath.clear();
	}

protected:
	void GetChildren(CPathFinderPointRef pt, std::list<CPathFinderPointRef> ptChildList);
	void Clear();
	void FillMap();

	int Heuristic(CPathFinderPointRef ptStart, CPathFinderPointRef ptEnd) const
	{
		return 10 * (abs(ptStart.m_pPoint->m_x - ptEnd.m_pPoint->m_x) + abs(ptStart.m_pPoint->m_y - ptEnd.m_pPoint->m_y));
	}

private:
	CPathFinder(const CPathFinder &copy);
	CPathFinder &operator=(const CPathFinder &other);
};

#endif	// _INC_CPATHFINDER_H
