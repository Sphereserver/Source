#include "CPathFinder.h"

CPathFinder::CPathFinder(CChar *pChar, CPointMap ptTarget, long searchDistance)
{
	CPointMap	pt;
	int			i;

	//	normalize
	if ( searchDistance % 2 ) searchDistance += 1;
	if ( searchDistance < 8 ) searchDistance = 8;
	if ( searchDistance > PATH_SIZE ) searchDistance = PATH_SIZE;

	//	initialise
	m_pChar = pChar;
	m_ptTarget = ptTarget;
	m_onClosedList = 10;
	m_searchDistance = searchDistance;

	// since we are in the center of our screen, remember the left top corner and simulate being zero
	pt = m_pChar->GetTopPoint();
	m_realXoffset = pt.m_x - m_searchDistance/2;
	m_realYoffset = pt.m_y - m_searchDistance/2;
	m_ptTarget.m_x -= m_realXoffset;
	m_ptTarget.m_y -= m_realYoffset;

	m_pathBank = (int*)malloc(sizeof(int));

	FillMap();
}

CPathFinder::~CPathFinder()
{
	free(m_pathBank);
}

int CPathFinder::FindPath()
{
	int onOpenList=0, parentXval=0, parentYval=0,
		a=0, b=0, m=0, u=0, v=0, temp=0, corner=0, numberOfOpenListItems=0,
		addedGCost=0, tempGcost = 0, path = 0, x=0, y=0,
		tempx, pathX, pathY, cellPosition,
		newOpenListItemID=0;

	//1. Convert location data (in pixels) to coordinates in the walkability array.
	int startX = m_pChar->GetTopPoint().m_x - m_realXoffset;
	int startY = m_pChar->GetTopPoint().m_y - m_realYoffset;

	//2.Quick Path Checks: Under the some circumstances no path needs to be generated ...

	if (startX == m_ptTarget.m_x && startY == m_ptTarget.m_y && m_pathLocation > 0) return PATH_FOUND;
	if (startX == m_ptTarget.m_x && startY == m_ptTarget.m_y && m_pathLocation == 0) return PATH_NONEXISTENT;
	if (( abs(startX - m_ptTarget.m_x) > m_searchDistance ) || ( abs(startY - m_ptTarget.m_y) > m_searchDistance )) return PATH_NONEXISTENT;
	if ( m_walkability[m_ptTarget.m_x][m_ptTarget.m_y] == PATH_UNWALKABLE ) goto noPath;

	//3.Reset some variables that need to be cleared
	for (x = 0; x < m_searchDistance; x++)
		for (y = 0; y < m_searchDistance;y++)
			m_whichList[x][y] = 0;

	m_onClosedList = 2; //changing the values of onOpenList and onClosed list is faster than redimming whichList() array
	onOpenList = 1;
	m_pathLength = PATH_NOTSTARTED;
	m_pathLocation = PATH_NOTSTARTED;
	m_Gcost[startX][startY] = 0;

	//4.Add the starting location to the open list of squares to be checked.
	numberOfOpenListItems = 1;
	m_openList[1] = 1;//assign it as the top (and currently only) item in the open list, which is maintained as a binary heap
	m_openX[1] = startX ;
	m_openY[1] = startY;

	//5.Do the following until a path is found or deemed nonexistent.
	do
	{
		//6.If the open list is not empty, take the first cell off of the list.
		//	This is the lowest F cost cell on the open list.
		if (numberOfOpenListItems != 0)
		{
			//7. Pop the first item off the open list.
			parentXval = m_openX[m_openList[1]];
			parentYval = m_openY[m_openList[1]]; //record cell coordinates of the item
			m_whichList[parentXval][parentYval] = m_onClosedList;//add the item to the closed list

			//	Open List = Binary Heap: Delete this item from the open list, which
			//  is maintained as a binary heap. For more information on binary heaps, see:
			//	http://www.policyalmanac.org/games/binaryHeaps.htm
			numberOfOpenListItems = numberOfOpenListItems - 1;//reduce number of open list items by 1	
		
			//	Delete the top item in binary heap and reorder the heap, with the lowest F cost item rising to the top.
			m_openList[1] = m_openList[numberOfOpenListItems+1];//move the last item in the heap up to slot #1
			v = 1;

			//	Repeat the following until the new item in slot #1 sinks to its proper spot in the heap.
			do
			{
				u = v;		
				if (2*u+1 <= numberOfOpenListItems) //if both children exist
				{
	 				//Check if the F cost of the parent is greater than each child.
					//Select the lowest of the two children.
					if (m_Fcost[m_openList[u]] >= m_Fcost[m_openList[2*u]]) v = 2*u;
					if (m_Fcost[m_openList[v]] >= m_Fcost[m_openList[2*u+1]]) v = 2*u+1;		
				}
				else
				{
					if (2*u <= numberOfOpenListItems) //if only child #1 exists
					{
	 					//Check if the F cost of the parent is greater than child #1	
						if (m_Fcost[m_openList[u]] >= m_Fcost[m_openList[2*u]]) v = 2*u;
					}
				}

				if (u != v) //if parent's F is > one of its children, swap them
				{
					temp = m_openList[u];
					m_openList[u] = m_openList[v];
					m_openList[v] = temp;			
				}
				else break; //otherwise, exit loop
			}
			while ( true );

			//7.Check the adjacent squares. (Its "children" -- these path children are similar, conceptually,
			//	to the binary heap children mentioned above, but don't confuse them. They are different. Add
			//	these adjacent child squares to the open list for later consideration if appropriate.
			for (b = parentYval-1; b <= parentYval+1; b++)
			{
				for (a = parentXval-1; a <= parentXval+1; a++)
				{
					//	If not off the map (do this first to avoid array out-of-bounds errors)
					if (a != -1 && b != -1 && a != m_searchDistance && b != m_searchDistance)
					{
						//	If not already on the closed list (items on the closed list have
						//	already been considered and can now be ignored).			
						if (m_whichList[a][b] != m_onClosedList)
						{
							//	If not a wall/obstacle square.
							if (m_walkability [a][b] != PATH_UNWALKABLE)
							{
								//	Don't cut across corners
								corner = PATH_WALKABLE;	
								if (a == parentXval-1) 
								{
									if (b == parentYval-1)
									{
										if (m_walkability[parentXval-1][parentYval] == PATH_UNWALKABLE
											|| m_walkability[parentXval][parentYval-1] == PATH_UNWALKABLE) \
											corner = PATH_UNWALKABLE;
									}
									else if (b == parentYval+1)
									{
										if (m_walkability[parentXval][parentYval+1] == PATH_UNWALKABLE
											|| m_walkability[parentXval-1][parentYval] == PATH_UNWALKABLE) 
											corner = PATH_UNWALKABLE; 
									}
								}
								else if (a == parentXval+1)
								{
									if (b == parentYval-1)
									{
										if (m_walkability[parentXval][parentYval-1] == PATH_UNWALKABLE 
											|| m_walkability[parentXval+1][parentYval] == PATH_UNWALKABLE) 
											corner = PATH_UNWALKABLE;
									}
									else if (b == parentYval+1)
									{
										if (m_walkability[parentXval+1][parentYval] == PATH_UNWALKABLE 
											|| m_walkability[parentXval][parentYval+1] == PATH_UNWALKABLE)
											corner = PATH_UNWALKABLE; 
									}
								}	
								if (corner == PATH_WALKABLE)
								{
									//	If not already on the open list, add it to the open list.			
									if (m_whichList[a][b] != onOpenList) 
									{	
										//Create a new open list item in the binary heap.
										newOpenListItemID = newOpenListItemID + 1; //each new item has a unique ID #
										m = numberOfOpenListItems+1;
										m_openList[m] = newOpenListItemID;//place the new open list item (actually, its ID#) at the bottom of the heap
										m_openX[newOpenListItemID] = a;
										m_openY[newOpenListItemID] = b;//record the x and y coordinates of the new item

										//Figure out its G cost
										if (abs(a-parentXval) == 1 && abs(b-parentYval) == 1) addedGCost = 14;//cost of going to diagonal squares	
										else addedGCost = 10;//cost of going to non-diagonal squares				
										m_Gcost[a][b] = m_Gcost[parentXval][parentYval] + addedGCost;

										//Figure out its H and F costs and parent
										m_Hcost[m_openList[m]] = 10*(abs(a - m_ptTarget.m_x) + abs(b - m_ptTarget.m_y));
										m_Fcost[m_openList[m]] = m_Gcost[a][b] + m_Hcost[m_openList[m]];
										m_parentX[a][b] = parentXval;
										m_parentY[a][b] = parentYval;	

										//Move the new open list item to the proper place in the binary heap.
										//Starting at the bottom, successively compare to parent items,
										//swapping as needed until the item finds its place in the heap
										//or bubbles all the way to the top (if it has the lowest F cost).
										while (m != 1) //While item hasn't bubbled to the top (m=1)	
										{
											//Check if child's F cost is < parent's F cost. If so, swap them.	
											if (m_Fcost[m_openList[m]] <= m_Fcost[m_openList[m/2]])
											{
												temp = m_openList[m/2];
												m_openList[m/2] = m_openList[m];
												m_openList[m] = temp;
												m = m/2;
											}
											else break;
										}
										numberOfOpenListItems = numberOfOpenListItems+1;//add one to the number of items in the heap

										//Change whichList to show that the new item is on the open list.
										m_whichList[a][b] = onOpenList;
									}
									//8.If adjacent cell is already on the open list, check to see if this 
									//	path to that cell from the starting location is a better one. 
									//	If so, change the parent of the cell and its G and F costs.	
									else //If whichList(a,b) = onOpenList
									{
										//Figure out the G cost of this possible new path
										if (abs(a-parentXval) == 1 && abs(b-parentYval) == 1) addedGCost = 14;//cost of going to diagonal tiles
										else addedGCost = 10;//cost of going to non-diagonal tiles
										tempGcost = m_Gcost[parentXval][parentYval] + addedGCost;
			
										//If this path is shorter (G cost is lower) then change
										//the parent cell, G cost and F cost. 		
										if (tempGcost < m_Gcost[a][b]) //if G cost is less,
										{
											m_parentX[a][b] = parentXval; //change the square's parent
											m_parentY[a][b] = parentYval;
											m_Gcost[a][b] = tempGcost;//change the G cost			

											//Because changing the G cost also changes the F cost, if
											//the item is on the open list we need to change the item's
											//recorded F cost and its position on the open list to make
											//sure that we maintain a properly ordered open list.
											for (int x = 1; x <= numberOfOpenListItems; x++) //look for the item in the heap
											{
												if (m_openX[m_openList[x]] == a && m_openY[m_openList[x]] == b) //item found
												{
													m_Fcost[m_openList[x]] = m_Gcost[a][b] + m_Hcost[m_openList[x]];//change the F cost
									
													//See if changing the F score bubbles the item up from it's current location in the heap
													m = x;
													while (m != 1) //While item hasn't bubbled to the top (m=1)	
													{
														//Check if child is < parent. If so, swap them.	
														if (m_Fcost[m_openList[m]] < m_Fcost[m_openList[m/2]])
														{
															temp = m_openList[m/2];
															m_openList[m/2] = m_openList[m];
															m_openList[m] = temp;
															m = m/2;
														}
														else break;
													} 
													break; //exit for x = loop
												} //If openX(openList(x)) = a
											} //For x = 1 To numberOfOpenListItems
										}//If tempGcost < Gcost(a,b)
									}//else If whichList(a,b) = onOpenList	
								}//If not cutting a corner
							}//If not a wall/obstacle square.
						}//If not already on the closed list 
					}//If not off the map
				}//for (a = parentXval-1; a <= parentXval+1; a++)
			}//for (b = parentYval-1; b <= parentYval+1; b++)
		}//if (numberOfOpenListItems != 0)
		//9.If open list is empty then there is no path.	
		else
		{
			path = PATH_NONEXISTENT;
			break;
		}  

		//If target is added to open list then path has been found.
		if (m_whichList[m_ptTarget.m_x][m_ptTarget.m_y] == onOpenList)
		{
			path = PATH_FOUND;
			break;
		}
	} while ( true );//Do until path is found or deemed nonexistent

	//10.Save the path if it exists.
	if (path == PATH_FOUND)
	{
		//a.Working backwards from the target to the starting location by checking
		//	each cell's parent, figure out the length of the path.
		pathX = m_ptTarget.m_x; pathY = m_ptTarget.m_y;
		do
		{
			//Look up the parent of the current cell.	
			tempx = m_parentX[pathX][pathY];		
			pathY = m_parentY[pathX][pathY];
			pathX = tempx;

			//Figure out the path length
			m_pathLength++;
		}
		while (pathX != startX || pathY != startY);

		//b.Resize the data bank to the right size in bytes
		m_pathBank = (int*)realloc(m_pathBank, m_pathLength*8);

		//c. Now copy the path information over to the databank. Since we are
		//	working backwards from the target to the start location, we copy
		//	the information to the data bank in reverse order. The result is
		//	a properly ordered set of path data, from the first step to the
		//	last.
		pathX = m_ptTarget.m_x; pathY = m_ptTarget.m_y;
		cellPosition = m_pathLength*2;//start at the end	
		do
		{
			cellPosition = cellPosition - 2;//work backwards 2 integers
			m_pathBank[cellPosition] = pathX;
			m_pathBank[cellPosition+1] = pathY;

			//d.Look up the parent of the current cell.	
			tempx = m_parentX[pathX][pathY];		
			pathY = m_parentY[pathX][pathY];
			pathX = tempx;
			//e.If we have reached the starting square, exit the loop.	
		}
		while (pathX != startX || pathY != startY);	
	}
	return path;

	//13.If there is no path to the selected target, set the pathfinder's xPath and yPath equal to its
	//	current location and return that the path is nonexistent.
noPath:
	m_xPath = m_pChar->GetTopPoint().m_x - m_realXoffset;
	m_yPath = m_pChar->GetTopPoint().m_y - m_realYoffset;
	return PATH_NONEXISTENT;
}

void CPathFinder::FillMap()
{
	int x, y;
	CRegionBase	*pArea;
	CPointMap	pt;

	EXC_TRY("FillMap");
	pt = m_pChar->GetTopPoint();

	memset(m_walkability, PATH_UNWALKABLE, sizeof(m_walkability));
	for ( x = 0 ; x < m_searchDistance; x++ )
	{
		for ( y = 0; y < m_searchDistance; y++ )
		{
			pt.m_x = x + m_realXoffset;
			pt.m_y = y + m_realYoffset;
			//	we should rely on this check to get rid of problems near the edges of the map
			if ( pt.IsValidXY() )
			{
				if ( m_pChar->CanMoveWalkTo(pt, true, true, DIR_QTY) )
				{
					m_walkability[x][y] = PATH_WALKABLE;
				}
			}
		}
	}
	EXC_CATCH;
}

int CPathFinder::ReadPathX(int pathLocation)
{
	int x(0);
	if ( m_pathLocation <= m_pathLength )
	{
		x = m_pathBank[pathLocation*2-2] + m_realXoffset;
	}
	return x;
}	

int CPathFinder::ReadPathY(int pathLocation)
{
	int y(0);
	if ( m_pathLocation <= m_pathLength )
	{
		y = m_pathBank[pathLocation*2-1] + m_realYoffset;
	}
	return y;
}	

void CPathFinder::ReadStep(int step)
{
	m_xPath = ReadPathX(step);
	m_yPath = ReadPathY(step);
}
