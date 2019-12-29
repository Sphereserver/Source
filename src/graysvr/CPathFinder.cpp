// Pathfinding algorithm based on A* Pathfinder (Version 1.71a) by Patrick Lester, pwlester@policyalmanac.org
#include "CPathFinder.h"

///////////////////////////////////////////////////////////
// CPathFinderPoint

CPathFinderPoint::CPathFinderPoint() : m_pParent(NULL), m_fWalkable(false), iValF(0), iValG(0), iValH(0)
{
	ADDTOCALLSTACK("CPathFinderPoint::CPathFinderPoint");
	m_x = 0;
	m_y = 0;
	m_z = 0;
	m_map = 0;
}

CPathFinderPoint::CPathFinderPoint(const CPointMap pt) : m_pParent(NULL), m_fWalkable(false), iValF(0), iValG(0), iValH(0)
{
	ADDTOCALLSTACK("CPathFinderPoint::CPathFinderPoint(pt)");
	m_x = pt.m_x;
	m_y = pt.m_y;
	m_z = pt.m_z;
	m_map = pt.m_map;
}

///////////////////////////////////////////////////////////
// CPathFinder

CPathFinder::CPathFinder(CChar *pChar, CPointMap ptTarget)
{
	ADDTOCALLSTACK("CPathFinder::CPathFinder");
	CPointMap pt = pChar->GetTopPoint();
	m_RealX = pt.m_x - (MAX_NPC_PATH_STORAGE_SIZE / 2);
	m_RealY = pt.m_y - (MAX_NPC_PATH_STORAGE_SIZE / 2);

	m_pChar = pChar;
	m_ptTarget = ptTarget;
	m_ptTarget.m_x -= m_RealX;
	m_ptTarget.m_y -= m_RealY;

	FillMap();
}

void CPathFinder::GetChildren(CPathFinderPointRef pt, std::list<CPathFinderPointRef> ptChildList)
{
	ADDTOCALLSTACK("CPathFinder::GetChildren");

	signed short RealX = 0, RealY = 0;
	for ( signed short x = -1; x != 2; ++x )
	{
		for ( signed short y = -1; y != 2; ++y )
		{
			if ( (x == 0) && (y == 0) )
				continue;

			RealX = pt.m_pPoint->m_x + x;
			RealY = pt.m_pPoint->m_y + y;
			if ( (RealX < 0) || (RealY < 0) || (RealX >= MAX_NPC_PATH_STORAGE_SIZE) || (RealY >= MAX_NPC_PATH_STORAGE_SIZE) )
				continue;
			if ( !m_Points[RealX][RealY].m_fWalkable )
				continue;

			if ( (x != 0) && (y != 0) )		// diagonal
			{
				if ( (RealX - x >= MAX_NPC_PATH_STORAGE_SIZE) || (RealY - y >= MAX_NPC_PATH_STORAGE_SIZE) )
					continue;
				if ( !m_Points[RealX - x][RealY].m_fWalkable || !m_Points[RealX][RealY - y].m_fWalkable )
					continue;
			}

			CPathFinderPointRef ptRef(m_Points[RealX][RealY]);
			ptChildList.push_back(ptRef);
		}
	}
}

bool CPathFinder::FindPath()
{
	ADDTOCALLSTACK("CPathFinder::FindPath");
	// Based on A* algorithm
	ASSERT(m_pChar);

	signed short X = m_pChar->GetTopPoint().m_x - m_RealX;
	signed short Y = m_pChar->GetTopPoint().m_y - m_RealY;

	if ( (X < 0) || (Y < 0) || (X >= MAX_NPC_PATH_STORAGE_SIZE) || (Y >= MAX_NPC_PATH_STORAGE_SIZE) )
	{
		// Too far away
		Clear();
		return false;
	}

	CPathFinderPointRef ptStart(m_Points[X][Y]);
	CPathFinderPointRef ptEnd(m_Points[m_ptTarget.m_x][m_ptTarget.m_y]);
	ASSERT(ptStart.m_pPoint);
	ASSERT(ptEnd.m_pPoint);

	ptStart.m_pPoint->iValG = 0;
	ptStart.m_pPoint->iValH = Heuristic(ptStart, ptEnd);
	ptStart.m_pPoint->iValF = ptStart.m_pPoint->iValH;

	m_Opened.push_back(ptStart);

	CPathFinderPointRef ptCurrent, ptChild;
	std::list<CPathFinderPointRef> ptChildList;

	while ( !m_Opened.empty() )
	{
		std::sort(m_Opened.begin(), m_Opened.end());
		ptCurrent = *m_Opened.begin();

		m_Opened.pop_front();
		m_Closed.push_back(ptCurrent);

		if ( ptCurrent == ptEnd )
		{
			CPathFinderPointRef ptRef = ptCurrent;
			while ( ptRef.m_pPoint->m_pParent )		// rebuild path + save
			{
				ptRef.m_pPoint = ptRef.m_pPoint->m_pParent;
				m_LastPath.push_front(CPointMap(ptRef.m_pPoint->m_x + m_RealX, ptRef.m_pPoint->m_y + m_RealY, 0, ptRef.m_pPoint->m_map));
			}
			Clear();
			return true;
		}

		ptChildList.clear();
		GetChildren(ptCurrent, ptChildList);

		while ( !ptChildList.empty() )
		{
			ptChild = ptChildList.front();
			ptChildList.pop_front();

			if ( std::find(m_Closed.begin(), m_Closed.end(), ptChild) != m_Closed.end() )
				continue;

			if ( std::find(m_Opened.begin(), m_Opened.end(), ptChild) != m_Opened.end() )
			{
				if ( ptChild.m_pPoint->iValG < ptCurrent.m_pPoint->iValG )
				{
					ptChild.m_pPoint->m_pParent = ptCurrent.m_pPoint;
					if ( (ptChild.m_pPoint->m_x == ptCurrent.m_pPoint->m_x) || (ptChild.m_pPoint->m_y == ptCurrent.m_pPoint->m_y) )
						ptChild.m_pPoint->iValG += 10;	// not diagonal
					else
						ptChild.m_pPoint->iValG += 14;	// diagonal

					ptChild.m_pPoint->iValF = ptChild.m_pPoint->iValG + ptChild.m_pPoint->iValH;
				}
				continue;
			}

			ptChild.m_pPoint->m_pParent = ptCurrent.m_pPoint;
			ptChild.m_pPoint->iValG = ptCurrent.m_pPoint->iValG;

			if ( (ptChild.m_pPoint->m_x == ptCurrent.m_pPoint->m_x) || (ptChild.m_pPoint->m_y == ptCurrent.m_pPoint->m_y) )
				ptChild.m_pPoint->iValG += 10;	// not diagonal
			else
				ptChild.m_pPoint->iValG += 14;	// diagonal

			ptChild.m_pPoint->iValH = Heuristic(ptChild, ptEnd);
			ptChild.m_pPoint->iValF = ptChild.m_pPoint->iValG + ptChild.m_pPoint->iValH;
			m_Opened.push_back(ptChild);
		}
	}

	Clear();
	return false;
}

void CPathFinder::Clear()
{
	ADDTOCALLSTACK("CPathFinder::Clear");
	m_Opened.clear();
	m_Closed.clear();
	m_RealX = 0;
	m_RealY = 0;
	m_pChar = NULL;
	m_ptTarget = CPointMap(0, 0);
}

void CPathFinder::FillMap()
{
	ADDTOCALLSTACK("CPathFinder::FillMap");

	CPointMap pt = m_pChar->GetTopPoint();
	for ( signed short x = 0; x != MAX_NPC_PATH_STORAGE_SIZE; ++x )
	{
		for ( signed short y = 0; y != MAX_NPC_PATH_STORAGE_SIZE; ++y )
		{
			if ( (m_ptTarget.m_x == x) && (m_ptTarget.m_y == y) )
			{
				// Assume that target position is walkable
				m_Points[x][y].m_fWalkable = true;
			}
			else
			{
				pt.m_x = m_RealX + x;
				pt.m_y = m_RealY + y;
				m_Points[x][y].m_fWalkable = (m_pChar->CanMoveWalkTo(pt, true, true, DIR_QTY, true) != NULL);
			}
			m_Points[x][y].Set(static_cast<WORD>(x), static_cast<WORD>(y), pt.m_z, pt.m_map);
		}
	}
}
