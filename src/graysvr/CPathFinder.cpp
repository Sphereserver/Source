#include "CPathFinder.h"

int CPathFinder::Heuristic(CPathFinderPointRef &pt1, CPathFinderPointRef &pt2)
{
	return 10 * (abs(pt1.m_Point->m_x - pt2.m_Point->m_x) + abs(pt1.m_Point->m_y - pt2.m_Point->m_y));
}

void CPathFinder::GetChildren(CPathFinderPointRef &Point, std::list<CPathFinderPointRef> &ChildrenRefList)
{
	ADDTOCALLSTACK("CPathFinder::GetChildren");

	signed short RealX = 0, RealY = 0;
	for ( signed short x = -1; x != 2; ++x )
	{
		for ( signed short y = -1; y != 2; ++y )
		{
			if ( (x == 0) && (y == 0) )
				continue;
			RealX = Point.m_Point->m_x + x;
			RealY = Point.m_Point->m_y + y;
			if ( (RealX < 0) || (RealY < 0) || (RealX >= MAX_NPC_PATH_STORAGE_SIZE) || (RealY >= MAX_NPC_PATH_STORAGE_SIZE) )
				continue;
			if ( !m_Points[RealX][RealY].m_Walkable )
				continue;
			if ( (x != 0) && (y != 0) )		// diagonal
			{
				if ( (RealX - x >= MAX_NPC_PATH_STORAGE_SIZE) || (RealY - y >= MAX_NPC_PATH_STORAGE_SIZE) )
					continue;
				if ( !m_Points[RealX - x][RealY].m_Walkable || !m_Points[RealX][RealY - y].m_Walkable )
					continue;
			}

			CPathFinderPointRef ptRef(m_Points[RealX][RealY]);
			ChildrenRefList.push_back(ptRef);
		}
	}
}

CPathFinderPoint::CPathFinderPoint() : m_Parent(0), m_Walkable(false), FValue(0), GValue(0), HValue(0)
{
	ADDTOCALLSTACK("CPathFinderPoint::CPathFinderPoint");
	m_x = 0;
	m_y = 0;
	m_z = 0;
	m_map = 0;
}

CPathFinderPoint::CPathFinderPoint(const CPointMap& pt) : m_Parent(0), m_Walkable(false), FValue(0), GValue(0), HValue(0)
{
	ADDTOCALLSTACK("CPathFinderPoint::CPathFinderPoint");
	m_x = pt.m_x;
	m_y = pt.m_y;
	m_z = pt.m_z;
	m_map = pt.m_map;
}


bool CPathFinderPoint::operator < (const CPathFinderPoint& pt) const
{
	//ADDTOCALLSTACK("CPathFinderPoint::operator <");
	return (FValue < pt.FValue);
}

const CPathFinderPoint* CPathFinderPoint::GetParent() const
{
	ADDTOCALLSTACK("CPathFinderPoint::GetParent");
	return m_Parent;
}

void CPathFinderPoint::SetParent(CPathFinderPointRef& pt)
{
	ADDTOCALLSTACK("CPathFinderPoint::SetParent");
	m_Parent = pt.m_Point;
}

CPathFinder::CPathFinder(CChar *pChar, CPointMap ptTarget)
{
	ADDTOCALLSTACK("CPathFinder::CPathFinder");
	CPointMap pt = pChar->GetTopPoint();
	m_RealX = pt.m_x - (MAX_NPC_PATH_STORAGE_SIZE / 2);
	m_RealY = pt.m_y - (MAX_NPC_PATH_STORAGE_SIZE / 2);

	m_pChar = pChar;
	m_Target = ptTarget;
	m_Target.m_x -= m_RealX;
	m_Target.m_y -= m_RealY;

	FillMap();
}

CPathFinder::~CPathFinder()
{
	ADDTOCALLSTACK("CPathFinderPoint::~CPathFinderPoint");
}

bool CPathFinder::FindPath()	// A* algorithm
{
	ADDTOCALLSTACK("CPathFinder::FindPath");
	ASSERT(m_pChar != NULL);

	signed short X = m_pChar->GetTopPoint().m_x - m_RealX;
	signed short Y = m_pChar->GetTopPoint().m_y - m_RealY;

	if ( (X < 0) || (Y < 0) || (X >= MAX_NPC_PATH_STORAGE_SIZE) || (Y >= MAX_NPC_PATH_STORAGE_SIZE) )
	{
		//Too far away
		Clear();
		return false;
	}

	CPathFinderPointRef Start(m_Points[X][Y]); //Start point
	CPathFinderPointRef End(m_Points[m_Target.m_x][m_Target.m_y]); //End Point

	ASSERT(Start.m_Point);
	ASSERT(End.m_Point);

	Start.m_Point->GValue = 0;
	Start.m_Point->HValue = Heuristic(Start, End);
	Start.m_Point->FValue = Start.m_Point->HValue;

	m_Opened.push_back( Start );

	std::list<CPathFinderPointRef> Children;
	CPathFinderPointRef Child, Current;
	std::deque<CPathFinderPointRef>::iterator InOpened, InClosed;

	while ( !m_Opened.empty() )
	{
		std::sort(m_Opened.begin(), m_Opened.end());
		Current = *m_Opened.begin();

		m_Opened.pop_front();
		m_Closed.push_back( Current );

		if ( Current == End )
		{
			CPathFinderPointRef PathRef = Current;
			while ( PathRef.m_Point->GetParent() ) //Rebuild path + save
			{
				PathRef.m_Point = const_cast<CPathFinderPoint*>(PathRef.m_Point->GetParent());
				m_LastPath.push_front(CPointMap(PathRef.m_Point->m_x + m_RealX, PathRef.m_Point->m_y + m_RealY, 0, PathRef.m_Point->m_map));
			}
			Clear();
			return true;
		}
		else
		{
			Children.clear();
			GetChildren( Current, Children );

			while ( !Children.empty() )
			{
				Child = Children.front();
				Children.pop_front();

				InClosed = std::find( m_Closed.begin(), m_Closed.end(), Child );
				InOpened = std::find( m_Opened.begin(), m_Opened.end(), Child );

				if ( InClosed != m_Closed.end() )
					continue;

				if ( InOpened == m_Opened.end() )
				{
					Child.m_Point->SetParent( Current );
					Child.m_Point->GValue = Current.m_Point->GValue;

					if ( Child.m_Point->m_x == Current.m_Point->m_x || Child.m_Point->m_y == Current.m_Point->m_y )
						Child.m_Point->GValue += 10; //Not diagonal
					else
						Child.m_Point->GValue += 14; //Diagonal

					Child.m_Point->HValue = Heuristic( Child, End );
					Child.m_Point->FValue = Child.m_Point->GValue + Child.m_Point->HValue;
					m_Opened.push_back( Child );
					//sort ( m_Opened.begin(), m_Opened.end() );
				}
				else
				{
					if ( Child.m_Point->GValue < Current.m_Point->GValue )
					{
						Child.m_Point->SetParent( Current );
						if ( Child.m_Point->m_x == Current.m_Point->m_x || Child.m_Point->m_y == Current.m_Point->m_y )
							Child.m_Point->GValue += 10;
						else
							Child.m_Point->GValue += 14;
						Child.m_Point->FValue = Child.m_Point->GValue + Child.m_Point->HValue;
						//sort ( m_Opened.begin(), m_Opened.end() );
					}
				}
			}
		}
	}


	Clear();
	return false;
}

void CPathFinder::Clear()
{
	ADDTOCALLSTACK("CPathFinder::Clear");
	m_Target = CPointMap(0,0);
	m_pChar = 0;
	m_Opened.clear();
	m_Closed.clear();
	m_RealX = 0;
	m_RealY = 0;
}

void CPathFinder::FillMap()
{
	ADDTOCALLSTACK("CPathFinder::FillMap");

	CPointMap pt = m_pChar->GetTopPoint();
	CRegionBase	*pArea;
	for ( signed short x = 0; x != MAX_NPC_PATH_STORAGE_SIZE; ++x )
	{
		for ( signed short y = 0; y != MAX_NPC_PATH_STORAGE_SIZE; ++y )
		{
			if ( (m_Target.m_x == x) && (m_Target.m_y == y) )
			{
				// Always assume that our target position is walkable
				m_Points[x][y].m_Walkable = true;
			}
			else
			{
				pt.m_x = m_RealX + x;
				pt.m_y = m_RealY + y;
				pArea = m_pChar->CanMoveWalkTo(pt, true, true, DIR_QTY, true);

				m_Points[x][y].m_Walkable = (pArea != NULL);
			}

			m_Points[x][y].Set(static_cast<WORD>(x), static_cast<WORD>(y), pt.m_z, pt.m_map);
		}
	}
}

CPointMap CPathFinder::ReadStep(signed short i)
{
	ADDTOCALLSTACK("CPathFinder::ReadStep");
	return m_LastPath[i];
}

signed short CPathFinder::LastPathSize()
{
	ADDTOCALLSTACK("CPathFinder::LastPathSize");
	return static_cast<signed short>(m_LastPath.size());
}

void CPathFinder::ClearLastPath()
{
	ADDTOCALLSTACK("CPathFinder::ClearLastPath");
	m_LastPath.clear();
}

