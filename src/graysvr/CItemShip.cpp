//
// CItemShip.cpp
//

#include "graysvr.h"	// predef header.
#include "../network/send.h"

/////////////////////////////////////////////////////////////////////////////

#define MAX_MULTI_LIST_OBJS 128

CItemShip::CItemShip( ITEMID_TYPE id, CItemBase * pItemDef ) : CItemMulti( id, pItemDef )
{
	m_NextMove.Init();
}

CItemShip::~CItemShip()
{
	g_Serv.ShipTimers_Delete(this);
}

bool CItem::Ship_Plank( bool fOpen )
{
	ADDTOCALLSTACK("CItem::Ship_Plank");
	// IT_SHIP_PLANK to IT_SHIP_SIDE and IT_SHIP_SIDE_LOCKED
	// This item is the ships plank.

	CItemBase * pItemDef = Item_GetDef();
	ITEMID_TYPE idState = static_cast<ITEMID_TYPE>(RES_GET_INDEX(pItemDef->m_ttShipPlank.m_idState));
	if ( !idState )
		return( false );

	if ( IsType(IT_SHIP_PLANK))
	{
		if ( fOpen )
			return( true );
	}
	else
	{
		if ( ! fOpen )
			return( true );
	}

	IT_TYPE oldType = GetType();
	SetID( idState );

	if ( IsType(IT_SHIP_PLANK) && (oldType == IT_SHIP_SIDE || oldType == IT_SHIP_SIDE_LOCKED) )
	{
		// Save the original Type of the plank if it used to be a ship side
		m_itShipPlank.m_itSideType = static_cast<WORD>(oldType);
	}
	else if ( oldType == IT_SHIP_PLANK )
	{
		// Restore the type of the ship side
		if ( m_itShipPlank.m_itSideType == IT_SHIP_SIDE || m_itShipPlank.m_itSideType == IT_SHIP_SIDE_LOCKED )
			SetType(static_cast<IT_TYPE>(m_itShipPlank.m_itSideType));

		m_itShipPlank.m_itSideType = IT_NORMAL;
	}

	Update();
	return( true );
}

void CItemShip::Ship_Stop()
{
	ADDTOCALLSTACK("CItemShip::Ship_Stop");
	// Make sure we have stopped.
	m_itShip.m_fSail = 0;
}

void CItemShip::Ship_SetPilot(CChar *pChar)
{
	ADDTOCALLSTACK("CItemShip::Ship_SetPilot");
	// Enable boat mouse movement on HS clients >= 7.0.9.0

	CChar *pCharPrev = m_itShip.m_Pilot.CharFind();
	if ( pChar && (!pChar->m_pPlayer || (pChar == pCharPrev)) )
		return;

	// Remove memory on previous pilot
	if ( pCharPrev )
	{
		CItem *pMemoryPrev = pCharPrev->ContentFind(RESOURCE_ID(RES_ITEMDEF, ITEMID_MEMORY_SHIP_PILOT));
		if ( pMemoryPrev )
		{
			pMemoryPrev->Delete();
			pCharPrev->SysMessageDefault(DEFMSG_SHIP_PILOT_OFF);
		}
		m_itShip.m_Pilot.InitUID();
	}

	// Create memory on new pilot
	if ( pChar )
	{
		if ( pChar->m_pArea != m_pRegion )
		{
			pChar->SysMessageDefault(DEFMSG_SHIP_PILOT_CANTABOARD);
			return;
		}
		else if ( pChar->IsStatFlag(STATF_OnHorse) )
		{
			pChar->SysMessageDefault(DEFMSG_ITEMUSE_CANTMOUNTED);
			return;
		}
		else if ( pChar->IsStatFlag(STATF_Hovering) )
		{
			pChar->SysMessageDefault(DEFMSG_SHIP_PILOT_CANTFLYING);
			return;
		}
		else if ( m_itShip.m_fAnchored != 0 )
		{
			pChar->SysMessageDefault(DEFMSG_SHIP_PILOT_CANTANCHOR);
			return;
		}

		CItem *pMemory = CItem::CreateScript(ITEMID_MEMORY_SHIP_PILOT);
		if ( !pMemory )
			return;

		Ship_Stop();
		m_itShip.m_Pilot = pChar->GetUID();

		pMemory->SetType(IT_EQ_HORSE);
		pMemory->SetName(GetName());
		pMemory->m_uidLink = GetUID();
		pChar->LayerAdd(pMemory, LAYER_HORSE);
		pChar->UpdateDir(static_cast<DIR_TYPE>(m_itShip.m_DirFace));
		pChar->SysMessageDefault(DEFMSG_SHIP_PILOT_ON);
	}
}

bool CItemShip::Ship_SetMoveDir(DIR_TYPE dir, BYTE bSpeed, bool fWheelMove)
{
	ADDTOCALLSTACK("CItemShip::Ship_SetMoveDir");
	// Set the direction we will move next time we get a tick.
	// Called from Packet 0xBF.0x32 : PacketWheelBoatMove to check if ship can move while setting dir and checking times in the proccess, otherwise for each click with mouse it will do 1 move.

	m_itShip.m_fSail = bSpeed;
	m_itShip.m_DirMove = static_cast<BYTE>(dir);	// we set new direction regardless of click limitations, so click in another direction means changing dir but makes not more moves until ship's timer moves it.

	if ( fWheelMove && (m_NextMove > CServTime::GetCurrentTime()) )
		return false;

	GetTopSector()->SetSectorWakeStatus();	// may get here b4 my client does.

	CItemMulti *pItemMulti = static_cast<CItemMulti *>(this);
	pItemMulti->m_SpeedMode = (bSpeed == 1) ? 3 : 4;

	g_Serv.ShipTimers_Delete(this);
	g_Serv.ShipTimers_Add(this);

	m_NextMove = CServTime::GetCurrentTime() + ((m_itShip.m_fSail == 1) ? pItemMulti->m_shipSpeed.period * 4 : pItemMulti->m_shipSpeed.period * 2);
	return true;
}


size_t CItemShip::Ship_ListObjs( CObjBase ** ppObjList )
{
	ADDTOCALLSTACK("CItemShip::Ship_ListObjs");
	// List all the objects in the structure.
	// Move the ship and everything on the deck
	// If too much stuff. then some will fall overboard. hehe.

	if ( ! IsTopLevel())
		return 0;

	int iMaxDist = Multi_GetMaxDist();
	int iShipHeight = GetTopZ() + maximum(3,Item_GetDef()->GetHeight());

	// always list myself first. All other items must see my new region !
	size_t iCount = 0;
	ppObjList[iCount++] = this;

	CWorldSearch AreaChar( GetTopPoint(), iMaxDist );
	AreaChar.SetAllShow( true );
	while ( iCount < MAX_MULTI_LIST_OBJS )
	{
		CChar * pChar = AreaChar.GetChar();
		if ( pChar == NULL )
			break;
		if ( ! m_pRegion->IsInside2d( pChar->GetTopPoint()))
			continue;
		if ( pChar->IsDisconnected() && pChar->m_pNPC )
			continue;

		int zdiff = pChar->GetTopZ() - iShipHeight;
		if ( zdiff < -2 || zdiff > PLAYER_HEIGHT )
			continue;

		ppObjList[iCount++] = pChar;
	}

	CWorldSearch AreaItem(GetTopPoint(), iMaxDist);
	while (iCount < MAX_MULTI_LIST_OBJS)
	{
		CItem * pItem = AreaItem.GetItem();
		if (pItem == NULL)
			break;
		if (pItem == this)	// already listed.
			continue;
		if (!Multi_IsPartOf(pItem))
		{
			if (!m_pRegion->IsInside2d(pItem->GetTopPoint()))
				continue;
	
			//I guess we can allow items to be locked on the ships and still move... but disallow attr_static from moving
			//if ( ! pItem->IsMovable() && !pItem->IsType(IT_CORPSE))
			if (IsAttr(ATTR_STATIC))
				continue;
	
			int zdiff = pItem->GetTopZ() - iShipHeight;
			if (zdiff < -2 || zdiff > PLAYER_HEIGHT)
				continue;
		}
		ppObjList[iCount++] = pItem;
	}
	return(iCount);
}

bool CItemShip::Ship_MoveDelta(CPointBase ptDelta, bool fMapBoundary)
{
	ADDTOCALLSTACK("CItemShip::Ship_MoveDelta");
	// Move the ship one space in some direction.
	ASSERT(m_pRegion->m_iLinkedSectors);

	signed char zNew = GetTopZ() + ptDelta.m_z;
	if ( (ptDelta.m_z > 0) && (zNew >= (UO_SIZE_Z - PLAYER_HEIGHT) - 1) )
		return false;
	if ( (ptDelta.m_z < 0) && (zNew <= (UO_SIZE_MIN_Z + 3)) )
		return false;

	// Move the ship and everything on the deck
	CObjBase *ppObjs[MAX_MULTI_LIST_OBJS + 1];
	size_t iCount = Ship_ListObjs(ppObjs);

	CObjBase *pObj = NULL;
	CPointMap pt, ptOld;
	for ( size_t i = 0; i < iCount; ++i )
	{
		pObj = ppObjs[i];
		if ( !pObj )
			continue;

		pt = pObj->GetTopPoint();
		pt += ptDelta;

		if ( !pt.IsValidPoint() )
		{
			DEBUG_ERR(("Ship UID=0%" FMTDWORDH " out of bounds\n", static_cast<DWORD>(GetUID())));
			continue;
		}
		pObj->MoveTo(pt);
	}

	CChar *pChar = NULL;
	CChar *pCharSrc = NULL;
	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		pCharSrc = pClient->GetChar();
		if ( !pCharSrc )
			continue;

		int iViewDist = pCharSrc->GetSight();
		for ( size_t i = 0; i < iCount; ++i )
		{
			pObj = ppObjs[i];
			if ( !pObj )
				continue;

			pt = pObj->GetTopPoint();
			ptOld = pt;
			ptOld -= ptDelta;

			// Remove objects that just moved out of sight
			if ( (pCharSrc->GetTopPoint().GetDist(pt) >= iViewDist) && (pCharSrc->GetTopPoint().GetDist(ptOld) < iViewDist) )
			{
				pClient->addObjectRemove(pObj);
				continue;
			}

			if ( pClient->CanSee(pObj) )
			{
				if ( pObj == this )		// this is the ship (usually the first item in the list)
				{
					if ( pClient->m_NetState->isClientVersion(MINCLIVER_HS) || pClient->m_NetState->isClientEnhanced() )
					{
						if ( !IsSetOF(OF_NoSmoothSailing) )
						{
							new PacketMoveShip(pClient, this, ppObjs, iCount, m_itShip.m_DirMove, m_itShip.m_DirFace, Multi_GetDef()->m_SpeedMode);

							if ( pCharSrc->m_pArea == m_pRegion )	// if client is on ship
							{
								if ( fMapBoundary )
									pClient->addPlayerView(NULL);
								else
									pClient->addPlayerSee(NULL, true);
								break;
							}
						}
						else if ( pClient->m_NetState->isClientEnhanced() )
							pClient->addObjectRemove(pObj);		// it will be added again in the if clause below
					}
				}

				if ( pObj->IsItem() )
				{
					if ( (pCharSrc->GetTopPoint().GetDist(pt) < iViewDist) && (IsSetOF(OF_NoSmoothSailing) || (pCharSrc->GetTopPoint().GetDist(ptOld) >= iViewDist) || !(pClient->m_NetState->isClientVersion(MINCLIVER_HS) || pClient->m_NetState->isClientEnhanced())) )
						pClient->addItem(static_cast<CItem *>(pObj));
				}
				else
				{
					pChar = static_cast<CChar *>(pObj);
					if ( pClient == pChar->m_pClient )
						pClient->addPlayerView(ptOld);
					else if ( (pCharSrc->GetTopPoint().GetDist(pt) <= iViewDist) && (IsSetOF(OF_NoSmoothSailing) || (pCharSrc->GetTopPoint().GetDist(ptOld) > iViewDist) || !(pClient->m_NetState->isClientVersion(MINCLIVER_HS) || pClient->m_NetState->isClientEnhanced())) )
					{
						if ( (pt.GetDist(ptOld) > 1) && (pClient->m_NetState->isClientLessVersion(MINCLIVER_HS)) && (pChar->GetTopPoint().GetDist(ptOld) < iViewDist) )
							pClient->addCharMove(pChar);
						else
						{
							pClient->addObjectRemove(pChar);
							pClient->addChar(pChar);
						}
					}
				}
			}
		}
	}
	return true;
}

bool CItemShip::Ship_CanMoveTo( const CPointMap & pt ) const
{
	ADDTOCALLSTACK("CItemShip::Ship_CanMoveTo");
	// Can we move to the new location ? all water type ?
	if ( IsAttr(ATTR_MAGIC) )
		return true;

	DWORD dwBlockFlags = CAN_I_WATER;
	g_World.GetHeightPoint2(pt, dwBlockFlags, true);
	return (dwBlockFlags & CAN_I_WATER);
}

static const DIR_TYPE sm_Ship_FaceDir[] =
{
	DIR_N,
	DIR_E,
	DIR_S,
	DIR_W
};

bool CItemShip::Ship_Face( DIR_TYPE dir )
{
	ADDTOCALLSTACK("CItemShip::Ship_Face");
	// Change the direction of the ship.

	if ( !IsTopLevel() || !m_pRegion || (dir % 2 == 1) )	// ship can't face diagonal directions
		return false;

	unsigned int iDirection = 0;
	for ( ; ; ++iDirection )
	{
		if ( iDirection >= COUNTOF(sm_Ship_FaceDir))
			return( false );
		if ( dir == sm_Ship_FaceDir[iDirection] )
			break;
	}

	int iFaceOffset = Ship_GetFaceOffset();
	ITEMID_TYPE idNew = static_cast<ITEMID_TYPE>(GetID() - iFaceOffset + iDirection);
	const CItemBaseMulti *pMultiDefNew = Multi_GetDef(idNew);
	if ( !pMultiDefNew )
		return false;

	int iTurn = dir - sm_Ship_FaceDir[ iFaceOffset ];

	// ?? Are there blocking items in the way of the turn ?

	// Acquire the CRectMap for the new direction of the ship
	CRectMap rect;
	reinterpret_cast<CGRect &>(rect) = pMultiDefNew->m_rect;
	rect.m_map = GetTopPoint().m_map;
	rect.OffsetRect( GetTopPoint().m_x, GetTopPoint().m_y );

	// Check that we can fit into this space.
	CPointMap ptTmp;
	ptTmp.m_z = GetTopPoint().m_z;
	ptTmp.m_map = static_cast<BYTE>(rect.m_map);
	for ( ptTmp.m_x = static_cast<signed short>(rect.m_left); ptTmp.m_x < static_cast<signed short>(rect.m_right); ++ptTmp.m_x )
	{
		for ( ptTmp.m_y = static_cast<signed short>(rect.m_top); ptTmp.m_y < static_cast<signed short>(rect.m_bottom); ++ptTmp.m_y )
		{
			if (m_pRegion->IsInside2d(ptTmp))
				continue;
			// If the ship already overlaps a point then we must
			// already be allowed there.
			if ( !ptTmp.IsValidPoint() || !Ship_CanMoveTo(ptTmp) )
			{
				CItem *pTiller = Multi_GetSign();
				ASSERT(pTiller);
				pTiller->Speak( g_Cfg.GetDefaultMsg( DEFMSG_TILLER_CANT_TURN ), HUE_TEXT_DEF, TALKMODE_SAY, FONT_NORMAL);
				return false;
			}
		}
	}

	const CItemBaseMulti *pMultiDefOld = Multi_GetDef(GetID());

	// Reorient everything on the deck
	CObjBase *ppObjs[MAX_MULTI_LIST_OBJS + 1];
	size_t iCount = Ship_ListObjs(ppObjs);

	CObjBase *pObj = NULL;
	CItem *pItem = NULL;
	CChar *pChar = NULL;

	CPointMap pt;
	int xd, xdiff, yd, ydiff;

	for ( size_t i = 0; i < iCount; ++i )
	{
		pObj = ppObjs[i];
		pt = pObj->GetTopPoint();
		xdiff = pt.m_x - GetTopPoint().m_x;
		ydiff = pt.m_y - GetTopPoint().m_y;

		switch ( iTurn )
		{
			case 2:		// right
			case 2 - DIR_QTY:
				xd = -ydiff;
				yd = xdiff;
				break;
			case -2:	// left
			case DIR_QTY - 2:
				xd = ydiff;
				yd = -xdiff;
				break;
			default:	// u turn
				xd = -xdiff;
				yd = -ydiff;
				break;
		}

		pt.m_x = static_cast<signed short>(GetTopPoint().m_x + xd);
		pt.m_y = static_cast<signed short>(GetTopPoint().m_y + yd);

		if ( pObj->IsItem() )
		{
			pItem = static_cast<CItem *>(pObj);
			if ( pItem == this )
			{
				m_pRegion->UnRealizeRegion();
				SetID(idNew);
				MultiRealizeRegion();
			}
			else if ( Multi_IsPartOf(pItem) )
			{
				for ( size_t j = 0; j < pMultiDefOld->m_Components.GetCount(); ++j )
				{
					const CItemBaseMulti::CMultiComponentItem &componentOld = pMultiDefOld->m_Components.ElementAt(j);
					if ( (xdiff == componentOld.m_dx) && (ydiff == componentOld.m_dy) && ((pItem->GetTopZ() - GetTopZ()) == componentOld.m_dz) )
					{
						const CItemBaseMulti::CMultiComponentItem &componentNew = pMultiDefNew->m_Components.ElementAt(j);
						IT_TYPE oldType = pItem->GetType();
						pItem->SetID(componentNew.m_id);
						pItem->SetType(oldType);
						pt.m_x = GetTopPoint().m_x + componentNew.m_dx;
						pt.m_y = GetTopPoint().m_y + componentNew.m_dy;
					}
				}
			}

			if ( IsTrigUsed(TRIGGER_SHIPTURN) )
			{
				CScriptTriggerArgs Args(dir, sm_Ship_FaceDir[iFaceOffset]);
				pItem->OnTrigger(ITRIG_Ship_Turn, &g_Serv, &Args);
			}

			pItem->MoveTo(pt);
			pItem->Update();
		}
		else
		{
			pChar = static_cast<CChar *>(pObj);
			CPointMap ptOld = pChar->GetTopPoint();
			pChar->MoveTo(pt);
			pChar->m_dirFace = GetDirTurn(pChar->m_dirFace, iTurn);

			CClient *pClient = pChar->m_pClient;
			if ( pClient )
				pClient->addPlayerView(ptOld);

			pChar->Update(false, pClient);
		}
	}

	m_itShip.m_DirFace = static_cast<BYTE>(dir);
	return true;
}

bool CItemShip::Ship_Move(DIR_TYPE dir, int iDist)
{
	ADDTOCALLSTACK("CItemShip::Ship_Move");
	if ( dir >= DIR_QTY )
		return( false );

	if ( m_pRegion == NULL )
	{
		DEBUG_ERR(( "Ship bad region\n" ));
		return false;
	}

	CPointMap ptDelta;
	ptDelta.ZeroPoint();

	CPointMap ptFore = m_pRegion->GetRegionCorner(dir);
	CPointMap ptBack = m_pRegion->GetRegionCorner(GetDirTurn(dir, 4));
	CPointMap ptLeft = m_pRegion->GetRegionCorner(GetDirTurn(dir, -1 - (dir % 2)));		// acquiring the flat edges requires two 'turns' for diagonal movement
	CPointMap ptRight = m_pRegion->GetRegionCorner(GetDirTurn(dir, 1 + (dir % 2)));
	CPointMap ptTest(ptLeft.m_x, ptLeft.m_y, GetTopZ(), GetTopMap());

	signed short iMapBoundX = (ptBack.m_map <= 1) ? UO_SIZE_X_REAL : static_cast<signed short>(g_MapList.GetX(ptBack.m_map));
	signed short iMapBoundY = static_cast<signed short>(g_MapList.GetY(ptBack.m_map));

	bool fStopped = false, fTurbulent = false, fMapBoundary = false;

	for (int i = 0; i < iDist; ++i)
	{
		ptFore.Move(dir);
		ptFore.m_z = GetTopZ();

		if ( IsSetOF(OF_MapBoundarySailing) )
		{
			if ( ptFore.m_x < 0 )
			{
				signed short iDelta = iMapBoundX - ptBack.m_x;
				ptDelta.m_x += iDelta;
				ptFore.m_x += iDelta;
				ptLeft.m_x += iDelta;
				ptRight.m_x += iDelta;
				ptTest.m_x += iDelta;
				fMapBoundary = true;
			}
			else if ( ptFore.m_y < 0 )
			{
				signed short iDelta = iMapBoundY - ptBack.m_y;
				ptDelta.m_y += iDelta;
				ptFore.m_y += iDelta;
				ptLeft.m_y += iDelta;
				ptRight.m_y += iDelta;
				ptTest.m_y += iDelta;
				fMapBoundary = true;
			}
			else if ( ptFore.m_x >= iMapBoundX )
			{
				signed short iDelta = ptBack.m_x + 1;
				ptDelta.m_x -= iDelta;
				ptFore.m_x -= iDelta;
				ptLeft.m_x -= iDelta;
				ptRight.m_x -= iDelta;
				ptTest.m_x -= iDelta;
				fMapBoundary = true;
			}
			else if ( ptFore.m_y >= iMapBoundY )
			{
				signed short iDelta = ptBack.m_y + 1;
				ptDelta.m_y -= iDelta;
				ptFore.m_y -= iDelta;
				ptLeft.m_y -= iDelta;
				ptRight.m_y -= iDelta;
				ptTest.m_y -= iDelta;
				fMapBoundary = true;
			}
		}
		else
		{
			if ( !ptFore.IsValidPoint() )
			{
				fStopped = true;
				fTurbulent = true;
				break;
			}
		}

#ifdef _DEBUG
		// In debug builds, this flashes some spots over tiles as they are checked for valid movement
		CItem* pItemDebug = NULL;
#define SPAWNSHIPTRACK(a,b)		pItemDebug = CItem::CreateBase(ITEMID_WorldGem);	\
								pItemDebug->SetType(IT_NORMAL);						\
								pItemDebug->SetAttr(ATTR_MOVE_NEVER|ATTR_DECAY);	\
								pItemDebug->SetHue(b);								\
								pItemDebug->MoveToDecay(a, TICK_PER_SEC / 2);						\
								a.GetSector()->SetSectorWakeStatus();
#else
#define SPAWNSHIPTRACK(a,b)
#endif

		// Test along the ship's edge to make sure nothing is blocking movement, this is split into two switch
		// statements (n/s and e/w) since moving diagonally requires the testing of both edges (i.e. 'north+east')
		ptLeft.Move(dir);
		ptRight.Move(dir);

		// test north/south edge (as needed)
		switch (dir)
		{
			case DIR_N:
			case DIR_NE:
			case DIR_NW:
				ptTest.m_y = ptFore.m_y; // align y coordinate
				for ( signed short x = ptLeft.m_x; x <= ptRight.m_x; ++x )
				{
					ptTest.m_x = x;
					SPAWNSHIPTRACK(ptTest, 0x40)
					if (Ship_CanMoveTo(ptTest) == false)
					{
						fStopped = true;
						break;
					}
				}
				break;

			case DIR_S:
			case DIR_SE:
			case DIR_SW:
				ptTest.m_y = ptFore.m_y;
				for ( signed short x = ptRight.m_x; x <= ptLeft.m_x; ++x )
				{
					ptTest.m_x = x;
					SPAWNSHIPTRACK(ptTest, 0x40)
					if (Ship_CanMoveTo(ptTest) == false)
					{
						fStopped = true;
						break;
					}
				}
				break;

			default:
				break;
		}

		// test east/west edge (as needed)
		switch (dir)
		{
			case DIR_E:
			case DIR_NE:
			case DIR_SE:
				ptTest.m_x = ptFore.m_x; // align x coordinate
				for ( signed short y = ptLeft.m_y; y <= ptRight.m_y; ++y )
				{
					ptTest.m_y = y;
					SPAWNSHIPTRACK(ptTest, 0xE0)
					if (Ship_CanMoveTo(ptTest) == false)
					{
						fStopped = true;
						break;
					}
				}
				break;

			case DIR_W:
			case DIR_NW:
			case DIR_SW:
				ptTest.m_x = ptFore.m_x;
				for ( signed short y = ptRight.m_y; y <= ptLeft.m_y; ++y )
				{
					ptTest.m_y = y;
					SPAWNSHIPTRACK(ptTest, 0xE0)
					if (Ship_CanMoveTo(ptTest) == false)
					{
						fStopped = true;
						break;
					}
				}
				break;

			default:
				break;
		}

#undef SHIPSPAWNTRACK // macro no longer needed

		// If the ship has been flagged as stopped, then abort movement here
		if ( fStopped )
			break;

		// Move delta one space
		ptDelta.Move(dir);
	}

	if (ptDelta.m_x != 0 || ptDelta.m_y != 0 || ptDelta.m_z != 0)
	{
		Ship_MoveDelta(ptDelta, fMapBoundary);

		// Move again
		GetTopSector()->SetSectorWakeStatus();	// may get here b4 my client does.
	}

	if ( fStopped )
	{
		CItem *pTiller = Multi_GetSign();
		ASSERT(pTiller);
		pTiller->Speak(g_Cfg.GetDefaultMsg(fTurbulent ? DEFMSG_TILLER_TURB_WATER : DEFMSG_TILLER_STOPPED), HUE_TEXT_DEF, TALKMODE_SAY, FONT_NORMAL);
		return false;
	}

	return true;
}

bool CItemShip::Ship_OnMoveTick()
{
	ADDTOCALLSTACK("CItemShip::Ship_OnMoveTick");
	// We just got a move tick.
	// RETURN: false = delete the boat.

	if ( m_itShip.m_fSail == 0 )	// decay the ship instead ???
		return( true );

	// Calculate the leading point.
	CItemMulti *pItemMulti = static_cast<CItemMulti *>(this);
	if ( !Ship_Move(static_cast<DIR_TYPE>(m_itShip.m_DirMove), pItemMulti->m_shipSpeed.tiles) )
	{
		Ship_Stop();
		return( true );
	}

	m_NextMove = CServTime::GetCurrentTime() + ((m_itShip.m_fSail == 1) ? pItemMulti->m_shipSpeed.period * 4 : pItemMulti->m_shipSpeed.period * 2);
	return(true);
}

enum
{
	SHV_SHIPANCHORDROP,
	SHV_SHIPANCHORRAISE,
	SHV_SHIPBACK,
	SHV_SHIPBACKLEFT,
	SHV_SHIPBACKRIGHT,
	SHV_SHIPDOWN,
	SHV_SHIPDRIFTLEFT,
	SHV_SHIPDRIFTRIGHT,
	SHV_SHIPFACE,
	SHV_SHIPFORE,
	SHV_SHIPFORELEFT,
	SHV_SHIPFORERIGHT,
	SHV_SHIPGATE,
	SHV_SHIPLAND,
	SHV_SHIPMOVE,
	SHV_SHIPSTOP,
	SHV_SHIPTURN,
	SHV_SHIPTURNLEFT,
	SHV_SHIPTURNRIGHT,
	SHV_SHIPUP,
	SHV_QTY
};

LPCTSTR const CItemShip::sm_szVerbKeys[SHV_QTY+1] =
{
	"SHIPANCHORDROP",
	"SHIPANCHORRAISE",
	"SHIPBACK",
	"SHIPBACKLEFT",
	"SHIPBACKRIGHT",
	"SHIPDOWN",		// down one space.
	"SHIPDRIFTLEFT",
	"SHIPDRIFTRIGHT",
	"SHIPFACE",		// set the ships facing direction.
	"SHIPFORE",
	"SHIPFORELEFT",
	"SHIPFORERIGHT",
	"SHIPGATE",		// Moves the whole ship to some new point location.
	"SHIPLAND",
	"SHIPMOVE",		// move in a specified direction.
	"SHIPSTOP",
	"SHIPTURN",
	"SHIPTURNLEFT",
	"SHIPTURNRIGHT",
	"SHIPUP",		// up one space.
	NULL
};

bool CItemShip::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CItemShip::r_GetRef");

	if ( !strnicmp( pszKey, "HATCH.", 6 ) )
	{
		pszKey += 6;
		pRef = GetShipHold();
		return true;
	}
	else if ( !strnicmp( pszKey, "TILLER.", 7 ) )
	{
		pszKey += 7;
		pRef = Multi_GetSign();
		return true;
	}
	else if ( !strnicmp( pszKey, "PLANK.", 6 ) )
	{
		pszKey += 6;
		int i = Exp_GetVal(pszKey);
		SKIP_SEPARATORS(pszKey);
		pRef = (i >= 0) ? GetShipPlank(i) : NULL;
		return true;
	}

	return( CItemMulti::r_GetRef( pszKey, pRef ));
}

bool CItemShip::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CItemShip::r_Verb");
	EXC_TRY("Verb");
	// Speaking in this ships region.
	// return: true = command for the ship.

	//"One (direction*)", " (Direction*), one" Moves ship one tile in desired direction and stops.
	//"Slow (direction*)" Moves ship slowly in desired direction (see below for possible directions).

	int iCmd = FindTableSorted(s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1);
	if ( iCmd < 0 )
		return CItemMulti::r_Verb(s, pSrc);

	if ( !pSrc || !IsTopLevel() )
		return false;

	// Find the tiller man object.
	CItem * pTiller = Multi_GetSign();
	ASSERT( pTiller );

	// Only key holders can command the ship ???
	CChar *pChar = pSrc->GetChar();
	if ( !pChar || !pChar->ContentFindKeyFor(pTiller) )
		return false;

	// Get current facing dir.
	int iDirOffset;
	LPCTSTR pszSpeak = NULL;

	switch ( iCmd )
	{
		case SHV_SHIPSTOP:
		{
			// "Furl sail"
			// "Stop"
			if ( m_itShip.m_fSail == 0 )
				return( false );
			Ship_Stop();
			break;
		}

		case SHV_SHIPFACE:
		{
			// Face this direction. do not change the direction of movement.
			if ( ! s.HasArgs())
				return( false );
			return Ship_Face( GetDirStr( s.GetArgStr()));
		}

		case SHV_SHIPMOVE:
		{
			// Move one space in this direction.
			// Does NOT protect against exploits !
			if ( ! s.HasArgs())
				return( false );
			m_itShip.m_DirMove = static_cast<BYTE>(GetDirStr(s.GetArgStr()));
			CItemMulti *pItemMulti = static_cast<CItemMulti *>(this);
			return Ship_Move(static_cast<DIR_TYPE>(m_itShip.m_DirMove), pItemMulti->m_shipSpeed.tiles);
		}

		case SHV_SHIPGATE:
		{
			// Move the whole ship and contents to another place.
			if ( ! s.HasArgs())
				return( false );

			CPointMap ptdelta = g_Cfg.GetRegionPoint( s.GetArgStr());
			if ( ! ptdelta.IsValidPoint())
				return( false );
			ptdelta -= GetTopPoint();
			return Ship_MoveDelta(ptdelta, true);
		}

		case SHV_SHIPTURNLEFT:
		{
			// "Turn left"
			// "Port"
			iDirOffset = -2;
doturn:
			if ( m_itShip.m_fAnchored != 0 )
			{
anchored:
				pszSpeak = g_Cfg.GetDefaultMsg(DEFMSG_TILLER_ANCHOR_IS_DOWN);
				break;
			}
			BYTE bDirMovePrev = static_cast<DIR_TYPE>(m_itShip.m_DirMove);
			m_itShip.m_DirMove = static_cast<BYTE>(GetDirTurn(sm_Ship_FaceDir[Ship_GetFaceOffset()], iDirOffset));
			if ( !Ship_Face(static_cast<DIR_TYPE>(m_itShip.m_DirMove)) )
			{
				m_itShip.m_DirMove = bDirMovePrev;
				return false;
			}
			break;
		}

		case SHV_SHIPTURNRIGHT:
		{
			// "Turn right"
			// "Starboard"
			iDirOffset = 2;
			goto doturn;
		}

		case SHV_SHIPDRIFTLEFT:
		{
			// "Left"
			// "Drift left"
			iDirOffset = -2;
dodirmovechange:
			if ( m_itShip.m_fAnchored != 0 )
				goto anchored;
			if ( !Ship_SetMoveDir(GetDirTurn(sm_Ship_FaceDir[Ship_GetFaceOffset()], iDirOffset)) )
				return false;
			break;
		}

		case SHV_SHIPDRIFTRIGHT:
		{
			// "Right"
			// "Drift right"
			iDirOffset = 2;
			goto dodirmovechange;
		}

		case SHV_SHIPBACK: 
		{
			// "Back"
			// "Backward"
			// "Backwards"
			iDirOffset = 4;
			goto dodirmovechange;
		}

		case SHV_SHIPFORE:
		{
			// "Forward"
			// "Foreward"
			// "Unfurl sail"
			iDirOffset = 0;
			goto dodirmovechange;
		}

		case SHV_SHIPFORELEFT:
		{
			// "Forward left"
			iDirOffset = -1;
			goto dodirmovechange;
		}

		case SHV_SHIPFORERIGHT:
		{
			// "Forward right"
			iDirOffset = 1;
			goto dodirmovechange;
		}

		case SHV_SHIPBACKLEFT:
		{
			// "Backward left"
			// "Back left"
			iDirOffset = -3;
			goto dodirmovechange;
		}

		case SHV_SHIPBACKRIGHT:
		{
			// "Backward right"
			// "Back right"
			iDirOffset = 3;
			goto dodirmovechange;
		}

		case SHV_SHIPANCHORRAISE:
		{
			// "Raise anchor"
			if ( m_itShip.m_fAnchored == 0 )
			{
				pszSpeak = g_Cfg.GetDefaultMsg( DEFMSG_TILLER_ANCHOR_IS_ALL_UP );
				break;
			}
			m_itShip.m_fAnchored = 0;
			break;
		}

		case SHV_SHIPANCHORDROP:
		{
			// "Drop anchor"
			if ( m_itShip.m_fAnchored != 0 )
			{
				pszSpeak = g_Cfg.GetDefaultMsg( DEFMSG_TILLER_ANCHOR_IS_ALL_DOWN );
				break;
			}
			m_itShip.m_fAnchored = 1;
			Ship_Stop();
			break;
		}

		case SHV_SHIPTURN:
		{
			// "Turn around"
			// "Come about"
			iDirOffset = 4;
			goto doturn;
		}

		case SHV_SHIPUP:
		{
			// "Up"
			if ( ! IsAttr(ATTR_MAGIC ))
				return( false );

			CPointMap pt;
			pt.m_z = PLAYER_HEIGHT;
			if ( Ship_MoveDelta( pt ))
			{
				pszSpeak = g_Cfg.GetDefaultMsg( DEFMSG_TILLER_CONFIRM );
			}
			else
			{
				pszSpeak = g_Cfg.GetDefaultMsg( DEFMSG_TILLER_DENY );
			}
			break;
		}

		case SHV_SHIPDOWN:
		{
			// "Down"
			if ( ! IsAttr(ATTR_MAGIC ))
				return( false );
			CPointMap pt;
			pt.m_z = -PLAYER_HEIGHT;
			if ( Ship_MoveDelta( pt ))
			{
				pszSpeak = g_Cfg.GetDefaultMsg( DEFMSG_TILLER_CONFIRM );
			}
			else
			{
				pszSpeak = g_Cfg.GetDefaultMsg( DEFMSG_TILLER_DENY );
			}
			break;
		}

		case SHV_SHIPLAND:
		{
			// "Land"
			if ( ! IsAttr(ATTR_MAGIC ))
				return( false );
			signed char zold = GetTopZ();
			CPointMap pt = GetTopPoint();
			pt.m_z = zold;
			SetTopZ( -UO_SIZE_Z );	// bottom of the world where i won't get in the way.
			DWORD dwBlockFlags = CAN_I_WATER;
			signed char z = g_World.GetHeightPoint2(pt, dwBlockFlags);
			SetTopZ( zold );	// restore z for now.
			pt.InitPoint();
			pt.m_z = z - zold;
			if ( pt.m_z )
			{
				Ship_MoveDelta( pt );
				pszSpeak = g_Cfg.GetDefaultMsg( DEFMSG_TILLER_CONFIRM );
			}
			else
			{
				pszSpeak = g_Cfg.GetDefaultMsg( DEFMSG_TILLER_DENY );
			}
			break;
		}

		default:
		{
			return false ;
		}
	}

	if ( pChar )
	{
		if ( !pszSpeak )
		{
			static LPCTSTR const sm_szTillerReplyMsg[] =
			{
				g_Cfg.GetDefaultMsg(DEFMSG_TILLER_REPLY_1),
				g_Cfg.GetDefaultMsg(DEFMSG_TILLER_REPLY_2),
				g_Cfg.GetDefaultMsg(DEFMSG_TILLER_REPLY_3)
			};
			pszSpeak = sm_szTillerReplyMsg[Calc_GetRandVal(COUNTOF(sm_szTillerReplyMsg))];
		}

		TCHAR szText[ MAX_TALK_BUFFER ];
		strncpy(szText, pszSpeak, MAX_TALK_BUFFER - 1);
		pChar->ParseText( szText, &g_Serv );
		pTiller->Speak( szText, HUE_TEXT_DEF, TALKMODE_SAY, FONT_NORMAL );
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return true;
}

void CItemShip::r_Write( CScript & s )
{
	ADDTOCALLSTACK_INTENSIVE("CItemShip::r_Write");
	CItemMulti::r_Write(s);
	if ( m_uidHold )
		s.WriteKeyHex("HATCH", m_uidHold );
	if ( GetShipPlankCount() > 0 )
	{
		for ( size_t i = 0; i < m_uidPlanks.size(); ++i )
			s.WriteKeyHex("PLANK", m_uidPlanks.at(i));
	}
	if (m_itShip.m_Pilot)
		s.WriteKeyHex("PILOT", m_itShip.m_Pilot);
}

enum IMCS_TYPE
{
	IMCS_HATCH,
	IMCS_PILOT,
	IMCS_PLANKS,
	IMCS_SHIPSPEED,
	IMCS_SPEEDMODE,
	IMCS_TILLER,
	IMCS_QTY
};

LPCTSTR const CItemShip::sm_szLoadKeys[IMCS_QTY + 1] = // static
{
	"HATCH",
	"PILOT",
	"PLANKS",
	"SHIPSPEED",
	"SPEEDMODE",
	"TILLER",
	NULL
};

bool CItemShip::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CItemShip::r_WriteVal");
	EXC_TRY("WriteVal");

	int index = FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 );
	if ( index == -1 )
	{
		if ( !strnicmp(pszKey, "SHIPSPEED.", 10) )
			index = IMCS_SHIPSPEED;
	}

	switch (index)
	{
		case IMCS_PILOT:
		{
			if (m_itShip.m_Pilot)
				sVal.FormatHex(m_itShip.m_Pilot);
			else
				sVal.FormatVal(0);
		} break;

		case IMCS_HATCH:
		{
			pszKey += 5;
			CItem * pItemHold = GetShipHold();
			if ( pItemHold )
				sVal.FormatHex( pItemHold->GetUID() );
			else
				sVal.FormatVal( 0 );
		} break;

		case IMCS_PLANKS:
		{
			pszKey += 6;
			sVal.FormatVal( GetShipPlankCount() );
		} break;

		case IMCS_SHIPSPEED:
		{	/*
			* Intervals:
			*       drift forward
			* fast | 0.25|   0.25
			* slow | 0.50|   0.50
			*
			* Speed:
			*       drift forward
			* fast |  0x4|    0x4
			* slow |  0x3|    0x3
			*
			* Tiles (per interval):
			*       drift forward
			* fast |    1|      1
			* slow |    1|      1
			*
			* 'walking' in piloting mode has a 1s interval, speed 0x2
			*/
			pszKey += 9;
			CItemMulti *pItemMulti = static_cast<CItemMulti *>(this);

			if (*pszKey == '.')
			{
				++pszKey;
				if (!strcmpi(pszKey, "TILES"))
				{
					sVal.FormatVal(pItemMulti->m_shipSpeed.tiles);
					break;
				}
				else if (!strcmpi(pszKey, "PERIOD"))
				{
					sVal.FormatVal(pItemMulti->m_shipSpeed.period);
					break;
				}
				return false;
			}

			sVal.Format("%d,%d", pItemMulti->m_shipSpeed.period, pItemMulti->m_shipSpeed.tiles);
		} break;

		case IMCS_SPEEDMODE:
		{
			CItemMulti *pItemMulti = static_cast<CItemMulti *>(this);
			sVal.FormatVal(pItemMulti->m_SpeedMode);
		}	break;

		case IMCS_TILLER:
		{
			pszKey += 6;
			CItem * pTiller = Multi_GetSign();
			if ( pTiller )
				sVal.FormatHex( pTiller->GetUID() );
			else
				sVal.FormatVal( 0 );
		} break;

		default:
			return( CItemMulti::r_WriteVal(pszKey, sVal, pSrc) );
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CItemShip::r_LoadVal( CScript & s  )
{
	ADDTOCALLSTACK("CItemShip::r_LoadVal");
	EXC_TRY("LoadVal");
	LPCTSTR	pszKey = s.GetKey();
	IMCS_TYPE index = (IMCS_TYPE)FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	if (g_Serv.IsLoading())
	{
		if ( s.IsKey("HATCH") )
		{
			m_uidHold = s.GetArgVal();
			return true;
		}
		else if ( s.IsKey("TILLER") )
		{
			m_uidLink = s.GetArgVal();
			return true;
		}
		else if ( s.IsKey("PLANK") )
		{
			CGrayUID uid = s.GetArgVal();
			m_uidPlanks.push_back( uid );
			return true;
		}
		else if (s.IsKey("PILOT"))
		{
			m_itShip.m_Pilot.SetObjUID(s.GetArgVal());
		}
	}

	switch (index)
	{
		case IMCS_SPEEDMODE:
		{
			CItemMulti *pItemMulti = static_cast<CItemMulti *>(this);
			BYTE speed = static_cast<BYTE>(s.GetArgVal());
			if ( speed < 1 )
				speed = 1;
			else if ( speed > 4 )
				speed = 4;
			pItemMulti->m_SpeedMode = speed;
			return true;
		}
		case IMCS_SHIPSPEED:
		{
			pszKey += 9;
			if ( *pszKey == '.' )
			{
				++pszKey;
				CItemMulti *pItemMulti = static_cast<CItemMulti *>(this);
				if ( !strcmpi(pszKey, "TILES") )
				{
					pItemMulti->m_shipSpeed.tiles = static_cast<BYTE>(s.GetArgVal());
					return true;
				}
				else if ( !strcmpi(pszKey, "PERIOD") )
				{
					pItemMulti->m_shipSpeed.period = static_cast<BYTE>(s.GetArgVal());
					return true;
				}
				INT64 piVal[2];
				size_t iQty = Str_ParseCmds(s.GetArgStr(), piVal, COUNTOF(piVal));
				if ( iQty == 2 )
				{
					pItemMulti->m_shipSpeed.period = static_cast<BYTE>(piVal[0]);
					pItemMulti->m_shipSpeed.tiles = static_cast<BYTE>(piVal[1]);
					return true;
				}
				return false;
			}
			break;
		}
		case IMCS_PILOT:
		{
			Ship_SetPilot(static_cast<CGrayUID>(s.GetArgVal()).CharFind());
			return true;
		}
		default:
			return CItemMulti::r_LoadVal(s);
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

int CItemShip::FixWeirdness()
{
	ADDTOCALLSTACK("CItemShip::FixWeirdness");
	int iResultCode = CItemMulti::FixWeirdness();
	if ( iResultCode )
		return iResultCode;

	GetShipHold();		// update/correct the hold uid
	GetShipPlank(0);	// update/correct the list of planks
	return iResultCode;
}

CItemContainer *CItemShip::GetShipHold()
{
	ADDTOCALLSTACK("CItemShip::GetShipHold");
	CItem *pItem = m_uidHold.ItemFind();
	if ( !pItem || pItem->IsDeleted() )
	{
		pItem = Multi_FindItemType(IT_SHIP_HOLD);
		if ( !pItem || pItem->IsDeleted() )
		{
			pItem = Multi_FindItemType(IT_SHIP_HOLD_LOCK);
			if ( !pItem || pItem->IsDeleted() )
				return NULL;
		}
		m_uidHold = pItem->GetUID();
	}

	CItemContainer *pItemHold = dynamic_cast<CItemContainer *>(pItem);
	return pItemHold ? pItemHold : NULL;
}

size_t CItemShip::GetShipPlankCount()
{
	ADDTOCALLSTACK("CItemShip::GetShipPlankCount");
	// CItemShip::GetShipPlank() updates the list of planks, so
	// calling this first will get an accurate result
	GetShipPlank(0);
	return m_uidPlanks.size();
}

CItem * CItemShip::GetShipPlank(size_t index)
{
	ADDTOCALLSTACK("CItemShip::GetShipPlank");
	// Check the current list of planks is valid
	for ( std::vector<CGrayUID>::iterator i = m_uidPlanks.begin(); i != m_uidPlanks.end(); ++i )
	{
		CItem * pItem = (*i).ItemFind();
		if ( pItem && Multi_IsPartOf( pItem ) )
			continue;

		// If an invalid plank uid was found, then wipe the whole list
		// and rebuild it
		m_uidPlanks.clear();
		break;
	}

	// Find plank(s) if the list is empty
	if ( m_uidPlanks.empty() )
	{
		CWorldSearch Area( GetTopPoint(), Multi_GetMaxDist() );
		for (;;)
		{
			CItem * pItem = Area.GetItem();
			if ( pItem == NULL )
				break;

			if ( pItem->IsDeleted() )
				continue;

			if ( !Multi_IsPartOf( pItem ) )
				continue;

			if ( pItem->IsType( IT_SHIP_PLANK ) || pItem->IsType( IT_SHIP_SIDE ) || pItem->IsType( IT_SHIP_SIDE_LOCKED ) )
				m_uidPlanks.push_back( pItem->GetUID() );
		}
	}

	if (index >= m_uidPlanks.size())
		return NULL;

	CGrayUID uid = m_uidPlanks.at(index);
	return uid.ItemFind();
}

bool CItemShip::OnTick()
{
	ADDTOCALLSTACK("CItemShip::OnTick");
	// Ships move on their tick.
	if ( m_NextMove <= CServTime::GetCurrentTime() )
		Ship_OnMoveTick();
	return true;
}

void CItemShip::OnComponentCreate( const CItem * pComponent )
{
	ADDTOCALLSTACK("CItemShip::OnComponentCreate");
	switch ( pComponent->GetType() )
	{
		case IT_SHIP_TILLER:
			// Tillerman is already stored as m_uidLink (Multi_GetSign())
			break;
		case IT_SHIP_HOLD:
		case IT_SHIP_HOLD_LOCK:
			m_uidHold = pComponent->GetUID();
			break;
		case IT_SHIP_PLANK:
		case IT_SHIP_SIDE:
		case IT_SHIP_SIDE_LOCKED:
			m_uidPlanks.push_back( pComponent->GetUID() );
			break;

		default:
			break;
	}

	CItemMulti::OnComponentCreate( pComponent );
	return;
}
