//
// CItemMultiCustom.cpp
//

#include "graysvr.h"	// predef header.
#include "../network/network.h"
#include "../network/send.h"

/////////////////////////////////////////////////////////////////////////////

CItemMultiCustom::CItemMultiCustom( ITEMID_TYPE id, CItemBase * pItemDef ) : CItemMulti( id, pItemDef )
{
	m_designMain.m_iRevision = 1;
	m_designMain.m_pData = NULL;
	m_designMain.m_iDataRevision = 0;
	m_designWorking.m_iRevision = 1;
	m_designWorking.m_pData = NULL;
	m_designWorking.m_iDataRevision = 0;
	m_designBackup.m_iRevision = 0;
	m_designBackup.m_pData = NULL;
	m_designBackup.m_iDataRevision = 0;
	m_designRevert.m_iRevision = 0;
	m_designRevert.m_pData = NULL;
	m_designRevert.m_iDataRevision = 0;
	m_pArchitect = NULL;
	m_pGrayMulti = NULL;
	m_rectDesignArea.SetRectEmpty();

	if ( !g_Serv.IsLoading() )
	{
		ResetStructure();
		CommitChanges();
	}
}

CItemMultiCustom::~CItemMultiCustom()
{
	if ( m_pArchitect != NULL)
	{
		m_pArchitect->m_pHouseDesign = NULL;
		m_pArchitect = NULL;
	}

	if ( m_pGrayMulti != NULL)
	{
		delete m_pGrayMulti;
		m_pGrayMulti = NULL;
	}

	ComponentsContainer::iterator it;
	for (it = m_designMain.m_vectorComponents.begin(); it != m_designMain.m_vectorComponents.end(); it = m_designMain.m_vectorComponents.erase(it))				delete *it;
	for (it = m_designWorking.m_vectorComponents.begin(); it != m_designWorking.m_vectorComponents.end(); it = m_designWorking.m_vectorComponents.erase(it))	delete *it;
	for (it = m_designBackup.m_vectorComponents.begin(); it != m_designBackup.m_vectorComponents.end(); it = m_designBackup.m_vectorComponents.erase(it))		delete *it;
	for (it = m_designRevert.m_vectorComponents.begin(); it != m_designRevert.m_vectorComponents.end(); it = m_designRevert.m_vectorComponents.erase(it))		delete *it;

	m_designMain.m_vectorComponents.clear();
	if ( m_designMain.m_pData != NULL )
	{
		delete[] m_designMain.m_pData;
		m_designMain.m_pData = NULL;
		m_designMain.m_iDataRevision = 0;
	}

	m_designWorking.m_vectorComponents.clear();
	if ( m_designWorking.m_pData != NULL )
	{
		delete[] m_designWorking.m_pData;
		m_designWorking.m_pData = NULL;
		m_designWorking.m_iDataRevision = 0;
	}

	m_designBackup.m_vectorComponents.clear();
	if ( m_designBackup.m_pData != NULL )
	{
		delete[] m_designBackup.m_pData;
		m_designBackup.m_pData = NULL;
		m_designBackup.m_iDataRevision = 0;
	}

	m_designRevert.m_vectorComponents.clear();
	if ( m_designRevert.m_pData != NULL )
	{
		delete[] m_designRevert.m_pData;
		m_designRevert.m_pData = NULL;
		m_designRevert.m_iDataRevision = 0;
	}
}

void CItemMultiCustom::BeginCustomize(CClient * pClientSrc)
{
	ADDTOCALLSTACK("CItemMultiCustom::BeginCustomize");
	// enter the given client into design mode for this building
	if ( pClientSrc == NULL || pClientSrc->GetChar() == NULL )
		return;

	if ( m_pArchitect != NULL )
		EndCustomize(true);

	if ( PacketHouseBeginCustomise::CanSendTo(pClientSrc->GetNetState()) == false )
		return;

	// copy the main design to working, ready for editing
	CopyDesign(&m_designMain, &m_designWorking);
	m_designWorking.m_iRevision++;

	// client will silently close all open dialogs and let the server think they're still open, so we need to update opened gump counts here
	CDialogDef* pDlg = NULL;
	for (CClient::OpenedGumpsMap_t::iterator it = pClientSrc->m_mapOpenedGumps.begin(); it != pClientSrc->m_mapOpenedGumps.end(); ++it)
	{
		// the client leaves 'nodispose' dialogs open
		pDlg = dynamic_cast<CDialogDef*>( g_Cfg.ResourceGetDef(RESOURCE_ID(RES_DIALOG, it->first)) );
		if (pDlg != NULL && pDlg->m_bNoDispose == true)
			continue;

		it->second = 0;
	}

	// hide dynamic item fixtures
	CWorldSearch Area(GetTopPoint(), GetDesignArea().GetWidth());
	Area.SetSearchSquare(true);
	for (;;)
	{
		CItem * pItem = Area.GetItem();
		if ( pItem == NULL )
			break;

		if ( (DWORD)pItem->GetTagDefs()->GetKeyNum("FIXTURE") != (DWORD)GetUID() )
			continue;

		pClientSrc->addObjectRemove(pItem);
	}

	// copy the working design to revert
	CopyDesign(&m_designWorking, &m_designRevert);

	// register client in design mode
	m_pArchitect = pClientSrc;
	m_pArchitect->m_pHouseDesign = this;

	new PacketHouseBeginCustomise(pClientSrc, this);

	// make sure they know the building exists
	pClientSrc->addItem(this);

	// send the latest building design
	SendStructureTo(pClientSrc);

	// move client to building and hide it
	CChar * pChar = pClientSrc->GetChar();
	pChar->StatFlag_Set( STATF_Hidden );

	CPointMap pt(GetTopPoint());
	pt.m_z += 7;

	CPointMap ptOld = pChar->GetTopPoint();
	pChar->MoveToChar(pt);
	pChar->UpdateMove(ptOld, NULL, true);
}

void CItemMultiCustom::EndCustomize(bool bForced)
{
	ADDTOCALLSTACK("CItemMultiCustom::EndCustomize");
	// end customization, exiting the client from design mode
	if ( m_pArchitect == NULL )
		return;

	// exit the 'architect' from customise mode
	new PacketHouseEndCustomise(m_pArchitect, this);
	m_pArchitect->m_pHouseDesign = NULL;

	CClient * pClient = m_pArchitect;
	m_pArchitect = NULL;

	CChar * pChar = pClient->GetChar();
	if ( pChar != NULL )
	{
		if ( IsTrigUsed(TRIGGER_HOUSEDESIGNEXIT) )
		{
			CScriptTriggerArgs Args(this);
			Args.m_iN1 = bForced;
			if ( pChar->OnTrigger(CTRIG_HouseDesignExit, pChar, &Args) == TRIGRET_RET_TRUE && !bForced)
			{
				BeginCustomize(pClient);
				return;
			}
		}
		
		// make sure scripts don't try and force the client back into design mode when
		// they're trying to log out
		if ( m_pArchitect && bForced )
		{
			m_pArchitect->m_pHouseDesign = NULL;
			m_pArchitect = NULL;
		}

		// reveal character
		pChar->StatFlag_Clear( STATF_Hidden );

		// move character to signpost (unless they're already outside of the building)
		if ( Multi_GetSign() && m_pRegion->IsInside2d(pChar->GetTopPoint()) )
		{
			CPointMap ptOld = pChar->GetTopPoint();
			CPointMap ptDest = Multi_GetSign()->GetTopPoint();

			// find ground height, since the signpost is usually raised
			DWORD wBlockFlags = 0;
			ptDest.m_z = g_World.GetHeightPoint(ptDest, wBlockFlags, true);

			pChar->MoveToChar(ptDest);
			pChar->UpdateMove(ptOld, NULL, true);
		}

		SendStructureTo(pClient);
		pClient->addReSync();
	}
}

void CItemMultiCustom::SwitchToLevel( CClient * pClientSrc, int iLevel )
{
	ADDTOCALLSTACK("CItemMultiCustom::SwitchToLevel");
	// switch the client to the given level of the building

	if ( iLevel < 0 )
		iLevel = 0;

	int iMaxLevel = GetLevelCount();
	if ( iLevel > iMaxLevel )
		iLevel = iMaxLevel;

	CChar * pChar = pClientSrc->GetChar();
	if ( pChar != NULL )
	{
		CPointMap pt(GetTopPoint());
		pt.m_z += GetPlaneZ(iLevel);

		CPointMap ptOld = pChar->GetTopPoint();
		pChar->MoveToChar(pt);
		pChar->UpdateMove(ptOld, NULL, true);
	}

	pClientSrc->addItem(this);
}

void CItemMultiCustom::CommitChanges(CClient * pClientSrc)
{
	ADDTOCALLSTACK("CItemMultiCustom::CommitChanges");
	// commit all the changes that have been made to the
	// working copy so that everyone can see them
	if ( m_designWorking.m_iRevision == m_designMain.m_iRevision )
		return;

	if (( pClientSrc != NULL && pClientSrc->GetChar() != NULL ) && ( IsTrigUsed(TRIGGER_HOUSEDESIGNCOMMIT) ))
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = m_designMain.m_vectorComponents.size();
		Args.m_iN2 = m_designWorking.m_vectorComponents.size();
		Args.m_iN3 = m_designWorking.m_iRevision;
		Args.m_pO1 = this;
		Args.m_VarsLocal.SetNum("FIXTURES.OLD", GetFixtureCount(&m_designMain));
		Args.m_VarsLocal.SetNum("FIXTURES.NEW", GetFixtureCount(&m_designWorking));

		if (pClientSrc->GetChar()->OnTrigger(CTRIG_HouseDesignCommit, pClientSrc->GetChar(), &Args) == TRIGRET_RET_TRUE)
			return;
	}

	// replace the main design with the working design
	CopyDesign(&m_designWorking, &m_designMain);

	if ( g_Serv.IsLoading() || !GetTopPoint().IsValidPoint() )
		return;

	// remove all existing dynamic item fixtures
	CWorldSearch Area(GetTopPoint(), GetDesignArea().GetWidth());
	Area.SetSearchSquare(true);
	CItem * pItem;
	for (;;)
	{
		pItem = Area.GetItem();
		if ( pItem == NULL )
			break;

		if ( (DWORD)pItem->GetTagDefs()->GetKeyNum("FIXTURE") != (DWORD)GetUID() )
			continue;

		pItem->Delete();
	}

	CRectMap rectNew;
	rectNew.SetRectEmpty();
	rectNew.m_map = GetTopMap();

	for ( ComponentsContainer::iterator i = m_designMain.m_vectorComponents.begin(); i != m_designMain.m_vectorComponents.end(); ++i)
	{
		rectNew.UnionPoint((*i)->m_item.m_dx, (*i)->m_item.m_dy);
		if ( (*i)->m_item.m_visible )
			continue;

		// replace the doors and teleporters with real items
		CItem * pItem = CItem::CreateScript((*i)->m_item.GetDispID());
		if ( pItem == NULL )
			continue;

		CPointMap pt(GetTopPoint());
		pt.m_x += (*i)->m_item.m_dx;
		pt.m_y += (*i)->m_item.m_dy;
		pt.m_z += (*i)->m_item.m_dz;

		pItem->m_uidLink = GetUID();
		pItem->ClrAttr(ATTR_DECAY|ATTR_CAN_DECAY);
		pItem->SetAttr(ATTR_MOVE_NEVER);
		pItem->m_Attr = ATTR_MOVE_NEVER;
		pItem->GetTagDefs()->SetNum("FIXTURE", (DWORD)GetUID());

		if ( pItem->IsType(IT_TELEPAD) )
		{
			// link telepads
			for ( ComponentsContainer::iterator j = i+1; j != i; ++j )
			{
				if ( j == m_designMain.m_vectorComponents.end() )
					j = m_designMain.m_vectorComponents.begin();

				if ( (*j)->m_item.GetDispID() != (*i)->m_item.GetDispID() )
					continue;

				pItem->m_itTelepad.m_pntMark = GetComponentPoint(*j);
				break;
			}
		}

		pItem->MoveTo(pt);
	}

	rectNew.OffsetRect(GetTopPoint().m_x, GetTopPoint().m_y);
	if ( m_pRegion != NULL && !m_pRegion->IsInside(rectNew) )
	{
		// items outside the region won't be noticed in los/movement checks,
		// so the region boundaries need to be stretched to fit all the components
		g_Log.EventWarn("Building design for 0%lx does not fit inside the MULTIREGION boundaries (design boundaries: %s). Attempting to resize region...\n", (DWORD)GetUID(), rectNew.Write());

		CGRect rect = m_pRegion->GetRegionRect(0);
		rectNew.UnionRect(rect);
		
		m_pRegion->SetRegionRect(rectNew);
		m_pRegion->UnRealizeRegion();
		m_pRegion->RealizeRegion();
	}

	m_designMain.m_iRevision++;
	m_designWorking.m_iRevision = m_designMain.m_iRevision;

	if ( m_pGrayMulti != NULL )
	{
		// multi object needs to be recreated
		delete m_pGrayMulti;
		m_pGrayMulti = NULL;
	}

	// update to all
	Update();
}

void CItemMultiCustom::AddItem(CClient * pClientSrc, ITEMID_TYPE id, short x, short y, signed char z, int iStairID)
{
	ADDTOCALLSTACK("CItemMultiCustom::AddItem");
	// add an item to the building design at the given location
	const CChar * pCharSrc = NULL;
	if (pClientSrc != NULL)
	{
		pCharSrc = pClientSrc->GetChar();
		if (pCharSrc == NULL)
			return;
	}

	CItemBase * pItemBase = CItemBase::FindItemBase(id);
	if ( pItemBase == NULL )
	{
		g_Log.EventWarn("Unscripted item 0%x being added to building 0%lx by 0%lx.\n", id, (DWORD)GetUID(), pCharSrc != NULL? (DWORD)pCharSrc->GetUID() : 0);
		SendStructureTo(pClientSrc);
		return;
	}

	// floor tiles have no height and can be placed under other objects (walls, doors, etc)
	bool bFloor = (pItemBase->GetHeight() == 0);
	// fixtures are items that should be replaced by dynamic items
	bool bFixture = (pItemBase->IsType(IT_DOOR) || pItemBase->IsType(IT_DOOR_LOCKED) || pItemBase->IsID_Door(id) || pItemBase->IsType(IT_TELEPAD));

	if ( !g_Serv.IsLoading() )
	{
		if ( !IsValidItem(id, pClientSrc, false) )
		{
			g_Log.EventWarn("Invalid item 0%x being added to building 0%lx by 0%lx.\n", id, (DWORD)GetUID(), pCharSrc != NULL? (DWORD)pCharSrc->GetUID() : 0);
			SendStructureTo(pClientSrc);
			return;
		}

		CPointMap pt(GetTopPoint());
		pt.m_x += x;
		pt.m_y += y;

		if ( pClientSrc != NULL )
		{
			// if a client is placing the item, make sure that
			// it is within the design area
			CGRect rectDesign = GetDesignArea();
			if ( !rectDesign.IsInside2d( pt ) )
			{
				if ( !rectDesign.IsInsideX( pt.m_x ) || !rectDesign.IsInsideY( pt.m_y - 1 ) )
				{
					g_Log.EventWarn("Item 0%x being added to building 0%lx outside of boundaries by 0%lx (%s).\n", id, (DWORD)GetUID(), pCharSrc != NULL? (DWORD)pCharSrc->GetUID() : 0, pt.WriteUsed());
					SendStructureTo(pClientSrc);
					return;
				}

				// items can be placed 1 tile south of the edit area, but
				// only at ground level
				z = 0;
			}
		}

		if ( z == -128 )
		{
			// determine z level based on player's position
			if ( pCharSrc != NULL )
				z = GetPlaneZ(GetPlane(pCharSrc->GetTopZ() - GetTopZ()));
			else
				z = 0;
		}

		Component * pPrevComponents[128];
		size_t iCount = GetComponentsAt(x, y, z, pPrevComponents, &m_designWorking);
		if ( iCount > 0 )
		{
			// remove previous item(s) in this location
			for (size_t i = 0; i < iCount; i++)
			{
				if ( bFloor != pPrevComponents[i]->m_isFloor )
					continue;

				RemoveItem( NULL, pPrevComponents[i]->m_item.GetDispID(), pPrevComponents[i]->m_item.m_dx, pPrevComponents[i]->m_item.m_dy, pPrevComponents[i]->m_item.m_dz);
			}
		}
	}

	Component * pComponent = new Component;
	pComponent->m_item.m_wTileID = id;
	pComponent->m_item.m_dx = x;
	pComponent->m_item.m_dy = y;
	pComponent->m_item.m_dz = z;
	pComponent->m_item.m_visible = !bFixture;
	pComponent->m_isStair = iStairID;
	pComponent->m_isFloor = bFloor;

	m_designWorking.m_vectorComponents.push_back(pComponent);
	m_designWorking.m_iRevision++;
}

void CItemMultiCustom::AddStairs(CClient * pClientSrc, ITEMID_TYPE id, short x, short y, signed char z, int iStairID)
{
	ADDTOCALLSTACK("CItemMultiCustom::AddStairs");
	// add a staircase to the building, the given ID must
	// be the ID of a multi to extract the items from
	const CChar * pCharSrc = NULL;
	if (pClientSrc != NULL)
	{
		pCharSrc = pClientSrc->GetChar();
		if (pCharSrc == NULL)
			return;
	}

    const CGrayMulti * pMulti = g_Cfg.GetMultiItemDefs(id);
	if ( pMulti == NULL )
	{
		g_Log.EventWarn("Unscripted multi 0%x being added to building 0%lx by 0%lx.\n", id, (DWORD)GetUID(), pCharSrc != NULL? (DWORD)pCharSrc->GetUID() : 0);
		SendStructureTo(pClientSrc);
		return;
	}
	
	if ( !IsValidItem(id, pClientSrc, true) )
	{
		g_Log.EventWarn("Invalid multi 0%x being added to building 0%lx by 0%lx.\n", id, (DWORD)GetUID(), pCharSrc != NULL? (DWORD)pCharSrc->GetUID() : 0);
		SendStructureTo(pClientSrc);
		return;
	}

	if ( z == -128 )
	{
		if ( pCharSrc != NULL )
			z = GetPlaneZ(GetPlane(pCharSrc->GetTopZ() - GetTopZ()));
		else
			z = 0;
	}

	if ( iStairID == -1 )
		iStairID = GetStairCount() + 1;

	size_t iQty = pMulti->GetItemCount();
	for ( size_t i = 0; i < iQty; i++ )
	{
		const CUOMultiItemRec2 * pMultiItem = pMulti->GetItem(i);
		if ( pMultiItem == NULL )
			continue;

		if ( !pMultiItem->m_visible )
			continue;

		AddItem(NULL, pMultiItem->GetDispID(), x + pMultiItem->m_dx, y + pMultiItem->m_dy, z + pMultiItem->m_dz, iStairID);
	}
	SendStructureTo(pClientSrc);
}

void CItemMultiCustom::AddRoof(CClient * pClientSrc, ITEMID_TYPE id, short x, short y, signed char z)
{
	ADDTOCALLSTACK("CItemMultiCustom::AddRoof");
	// add a roof piece to the building
	const CChar * pCharSrc = NULL;
	if (pClientSrc != NULL)
	{
		pCharSrc = pClientSrc->GetChar();
		if (pCharSrc == NULL)
			return;
	}

	CItemBase * pItemBase = CItemBase::FindItemBase(id);
	if ( pItemBase == NULL )
	{
		g_Log.EventWarn("Unscripted roof tile 0%x being added to building 0%lx by 0%lx.\n", id, (DWORD)GetUID(), pCharSrc != NULL? (DWORD)pCharSrc->GetUID() : 0);
		SendStructureTo(pClientSrc);
		return;
	}

	if ( (pItemBase->GetTFlags() & UFLAG4_ROOF) == 0 )
	{
		g_Log.EventWarn("Non-roof tile 0%x being added as a roof to building 0%lx by 0%lx.\n", id, (DWORD)GetUID(), pCharSrc != NULL? (DWORD)pCharSrc->GetUID() : 0);
		SendStructureTo(pClientSrc);
		return;
	}

	if ( z < -3 || z > 12 || (z % 3 != 0) )
	{
		g_Log.EventWarn("Roof tile 0%x being added at invalid height %d to building 0%lx by 0%lx.\n", id, z, (DWORD)GetUID(), pCharSrc != NULL? (DWORD)pCharSrc->GetUID() : 0);
		SendStructureTo(pClientSrc);
		return;
	}

	if ( pCharSrc != NULL )
		z += pCharSrc->GetTopZ();

	AddItem(pClientSrc, id, x, y, z);
}

void CItemMultiCustom::RemoveItem(CClient * pClientSrc, ITEMID_TYPE id, short x, short y, signed char z)
{
	ADDTOCALLSTACK("CItemMultiCustom::RemoveItem");
	// remove the item that's found at given location
	// ITEMID_NOTHING means we should remove any items found
	CGRect rectDesign = GetDesignArea();
	CPointMap pt(GetTopPoint());
	pt.m_x += x;
	pt.m_y += y;

	if ( pClientSrc != NULL )
	{
		bool allowRemove = true;
		switch ( GetPlane(z) )
		{
			case 1:
				// at first level, clients cannot remove dirt tiles
				if ( id == ITEMID_DIRT_TILE )
					allowRemove = false;
				break;

			case 0:
				// at ground level, clients can only remove components along the bottom edge (stairs)
				if ( pt.m_y != rectDesign.m_bottom )
					allowRemove = false;
		}

		if ( allowRemove == false )
		{
			SendStructureTo(pClientSrc);
			return;
		}
	}

	Component * pComponents[128];
	size_t iCount = GetComponentsAt(x, y, z, pComponents, &m_designWorking);
	if ( iCount <= 0 )
		return;

	bool bReplaceDirt = false;
	for ( size_t i = 0; i < iCount; i++ )
	{
		for ( ComponentsContainer::iterator j = m_designWorking.m_vectorComponents.begin(); j != m_designWorking.m_vectorComponents.end(); ++j )
		{
			if ( *j != pComponents[i] )
				continue;

			if ( id != ITEMID_NOTHING && ((*j)->m_item.GetDispID() != id) )
				break;

			if ( pClientSrc != NULL && RemoveStairs(*j) )
				break;

			// floor tiles the ground floor are replaced with dirt tiles
			if ( ((*j)->m_item.m_wTileID != ITEMID_DIRT_TILE) && (*j)->m_isFloor && (GetPlane(*j) == 1) && (GetPlaneZ(GetPlane(*j)) == (*j)->m_item.m_dz) )
				bReplaceDirt = true;

			m_designWorking.m_vectorComponents.erase(j);
			m_designWorking.m_iRevision++;
			break;
		}
	}
	
	if ( pClientSrc != NULL && bReplaceDirt )
	{
		// make sure that the location is within the proper boundaries
		if ( rectDesign.IsInsideX(pt.m_x) && rectDesign.IsInsideX(pt.m_x-1) && rectDesign.IsInsideY(pt.m_y) && rectDesign.IsInsideY(pt.m_y-1) )
			AddItem(NULL, ITEMID_DIRT_TILE, x, y, 7);
	}
	SendStructureTo(pClientSrc);
}

bool CItemMultiCustom::RemoveStairs(Component * pStairComponent)
{
	ADDTOCALLSTACK("CItemMultiCustom::RemoveStairs");
	// attempt to remove the given component as a stair piece,
	// removing all 'linked' stair pieces along with it
	// return false means that the passed component was not a
	// stair piece and should be removed normally
	if ( pStairComponent == NULL )
		return false;

	if ( pStairComponent->m_isStair == 0 )
		return false;

	int iStairID = pStairComponent->m_isStair;

	for ( ComponentsContainer::iterator i = m_designWorking.m_vectorComponents.begin(); i != m_designWorking.m_vectorComponents.end(); )
	{
		if ( (*i)->m_isStair == iStairID )
		{
			bool bReplaceDirt = false;
			if ( (*i)->m_isFloor && (GetPlane(*i) == 1) && (GetPlaneZ(GetPlane(*i)) == (*i)->m_item.m_dz) )
				bReplaceDirt = true;

			int x=(*i)->m_item.m_dx;
			int y=(*i)->m_item.m_dy;
			int z=(*i)->m_item.m_dz;

			i = m_designWorking.m_vectorComponents.erase(i);
			m_designWorking.m_iRevision++;

			if (bReplaceDirt)
				AddItem(NULL, ITEMID_DIRT_TILE, x, y, z);
		}
		else
		{
			++i;
		}
	}

	return true;
}

void CItemMultiCustom::RemoveRoof(CClient * pClientSrc, ITEMID_TYPE id, short x, short y, signed char z)
{
	ADDTOCALLSTACK("CItemMultiCustom::RemoveRoof");
	
	CItemBase * pItemBase = CItemBase::FindItemBase(id);
	if ( pItemBase == NULL )
		return;

	if ( (pItemBase->GetTFlags() & UFLAG4_ROOF) == 0 )
		return;

	RemoveItem(pClientSrc, id, x, y, z);
}

void CItemMultiCustom::SendVersionTo(CClient * pClientSrc)
{
	ADDTOCALLSTACK("CItemMultiCustom::SendVersionTo");
	// send the revision number of this building to the given
	// client
	if ( pClientSrc == NULL || pClientSrc->IsPriv(PRIV_DEBUG) )
		return;

	// send multi version
	new PacketHouseDesignVersion(pClientSrc, this);
}

void CItemMultiCustom::SendStructureTo(CClient * pClientSrc)
{
	ADDTOCALLSTACK("CItemMultiCustom::SendStructureTo");
	// send the design details of this building to the given
	// client
	if ( pClientSrc == NULL || pClientSrc->GetChar() == NULL || pClientSrc->IsPriv(PRIV_DEBUG) )
		return;

	if ( PacketHouseDesign::CanSendTo(pClientSrc->GetNetState()) == false )
		return;

	DesignDetails * pDesign = NULL;
	if ( m_pArchitect == pClientSrc )
		pDesign = &m_designWorking;	// send the working copy to the designer
	else
		pDesign = &m_designMain;	// other clients will only see final designs

	// check if a packet is already saved
	if ( pDesign->m_pData != NULL )
	{
		// check the saved packet matches the design revision
		if ( pDesign->m_iDataRevision == pDesign->m_iRevision )
		{
			pDesign->m_pData->send(pClientSrc);
			return;
		}

		// remove the saved packet and continue to build a new one
		delete pDesign->m_pData;
		pDesign->m_pData = NULL;
		pDesign->m_iDataRevision = 0;
	}

	PacketHouseDesign* cmd = new PacketHouseDesign(this, pDesign->m_iRevision);

	if ( pDesign->m_vectorComponents.size() )
	{
		// determine the dimensions of the building
		const CGRect rectDesign = GetDesignArea();
		int iMinX = rectDesign.m_left, iMinY = rectDesign.m_top;
		int iWidth = rectDesign.GetWidth();
		int iHeight = rectDesign.GetHeight();

		int iMaxPlane = 0;
		ComponentsContainer vectorStairs;
		Component * pComp;
		CItemBase * pItemBase;

		// find the highest plane/floor
		for ( ComponentsContainer::iterator i = pDesign->m_vectorComponents.begin(); i != pDesign->m_vectorComponents.end(); ++i )
		{
			if ( GetPlane(*i) <= iMaxPlane )
				continue;

			iMaxPlane = GetPlane(*i);
		}

		NWORD wPlaneBuffer[PLANEDATA_BUFFER];

		for (int iCurrentPlane = 0; iCurrentPlane <= iMaxPlane; iCurrentPlane++)
		{
			// for each plane, generate a list of items
			bool bFoundItems = false;
			int iItemCount = 0;
			int iMaxIndex = 0;

			memset(wPlaneBuffer, 0, sizeof(wPlaneBuffer));
			for ( ComponentsContainer::iterator i = pDesign->m_vectorComponents.begin(); i != pDesign->m_vectorComponents.end(); ++i )
			{
				if ( GetPlane(*i) != iCurrentPlane )
					continue;

				pComp = *i;
				if ( !pComp->m_item.m_visible && pDesign != &m_designWorking )
					continue;

				pItemBase = CItemBase::FindItemBase(pComp->m_item.GetDispID());
				if ( pItemBase == NULL )
					continue;

				// calculate the x,y position as an offset from the topleft corner
				CPointMap ptComp = GetComponentPoint(pComp);
				int x = (ptComp.m_x - 1) - iMinX;
				int y = (ptComp.m_y - 1) - iMinY;

				// index is (x*height)+y
				int index; // = (x * iHeight) + y;
				if ( iCurrentPlane == 0 )
					index = ((x + 1) * (iHeight + 1)) + (y + 1);
				else
					index = (x * (iHeight - 1)) + y;

				if (( GetPlaneZ(GetPlane(pComp)) != pComp->m_item.m_dz ) ||
					( pItemBase->GetHeight() == 0 || pComp->m_isFloor ) ||
					( x < 0 || y < 0 || x >= iWidth || y >= iHeight ) ||
					( index < 0 || index >= PLANEDATA_BUFFER ))
				{
					// items that are:
					//  - not level with plane height
					//  - heightless / is a floor
					//  - outside of building area
					//  - outside of buffer bounds
					// are placed in the stairs list
					vectorStairs.push_back(pComp);
					continue;
				}

				wPlaneBuffer[index] = pComp->m_item.GetDispID();
				bFoundItems = true;
				iItemCount++;
				iMaxIndex = maximum(iMaxIndex, index);
			}

			if (bFoundItems == false)
				continue;

			int iPlaneSize = (iMaxIndex + 1) * sizeof(NWORD);
			cmd->writePlaneData(iCurrentPlane, iItemCount, (BYTE*)wPlaneBuffer, iPlaneSize);
		}

		for ( ComponentsContainer::iterator i = vectorStairs.begin(); i != vectorStairs.end(); ++i )
		{
			pComp = *i;
			if ( !pComp->m_item.m_visible && pDesign != &m_designWorking )
				continue;

			// stair items can be sent in any order
			cmd->writeStairData(pComp->m_item.GetDispID(), pComp->m_item.m_dx, pComp->m_item.m_dy, pComp->m_item.m_dz);
		}
	}

	cmd->finalise();

	// save the packet in the design data
	pDesign->m_pData = cmd;
	pDesign->m_iDataRevision = pDesign->m_iRevision;

	// send the packet
	pDesign->m_pData->send(pClientSrc);
}

void CItemMultiCustom::BackupStructure(CClient * pClientSrc)
{
	ADDTOCALLSTACK("CItemMultiCustom::BackupStructure");
	UNREFERENCED_PARAMETER(pClientSrc);
	// create a backup of the working copy
	if ( m_designWorking.m_iRevision == m_designBackup.m_iRevision )
		return;

	CopyDesign(&m_designWorking, &m_designBackup);
}

void CItemMultiCustom::RestoreStructure(CClient * pClientSrc)
{
	ADDTOCALLSTACK("CItemMultiCustom::RestoreStructure");
	// restore the working copy using the details stored in the
	// backup design
	if ( m_designWorking.m_iRevision == m_designBackup.m_iRevision || m_designBackup.m_iRevision == 0)
		return;

	CopyDesign(&m_designBackup, &m_designWorking);
	
	if ( pClientSrc != NULL )
		pClientSrc->addItem(this);
}

void CItemMultiCustom::RevertChanges( CClient * pClientSrc )
{
	ADDTOCALLSTACK("CItemMultiCustom::RevertChanges");
	// restore the working copy using the details stored in the
	// revert design
	if ( m_designWorking.m_iRevision == m_designRevert.m_iRevision || m_designRevert.m_iRevision == 0 )
		return;

	CopyDesign(&m_designRevert, &m_designWorking);

	if ( pClientSrc != NULL )
		pClientSrc->addItem(this);
}

void CItemMultiCustom::ResetStructure( CClient * pClientSrc )
{
	ADDTOCALLSTACK("CItemMultiCustom::ResetStructure");
	// return the building design to it's original state, which
	// is simply the 'foundation' design from the multi.mul file

	m_designWorking.m_vectorComponents.clear();
	m_designWorking.m_iRevision = 1;
	const CGrayMulti * pMulti =  g_Cfg.GetMultiItemDefs(GetID());
	if ( pMulti != NULL )
	{
		size_t iQty = pMulti->GetItemCount();
		for (size_t i = 0; i < iQty; i++)
		{
			const CUOMultiItemRec2 * pMultiItem = pMulti->GetItem(i);
			if ( pMultiItem == NULL )
				continue;

			if ( !pMultiItem->m_visible )
				continue;

			AddItem(NULL, pMultiItem->GetDispID(), pMultiItem->m_dx, pMultiItem->m_dy, pMultiItem->m_dz);
		}
	}

	if ( pClientSrc != NULL )
		pClientSrc->addItem(this);
}

int CItemMultiCustom::GetRevision(const CClient * pClientSrc) const
{
	ADDTOCALLSTACK("CItemMultiCustom::GetRevision");
	// return the revision number, making sure to return
	// the working revision if the client is the designer
	if ( pClientSrc != NULL && pClientSrc == m_pArchitect )
		return m_designWorking.m_iRevision;	// working

	return m_designMain.m_iRevision;
}

int CItemMultiCustom::GetLevelCount()
{
	ADDTOCALLSTACK("CItemMultiCustom::GetLevelCount");
	// return how many levels (including the roof) there are
	// client decides this based on the size of the foundation
	const CGRect rectDesign = GetDesignArea();
	if (rectDesign.GetWidth() >= 14 || rectDesign.GetHeight() >= 14)
		return 4;

	return 3;
}

int CItemMultiCustom::GetStairCount()
{
	ADDTOCALLSTACK("CItemMultiCustom::GetStairCount");
	// find and return the highest stair id
	int iCount = 0;
	for (ComponentsContainer::iterator i = m_designWorking.m_vectorComponents.begin(); i != m_designWorking.m_vectorComponents.end(); ++i)
	{
		if ((*i)->m_isStair == 0)
			continue;

		iCount = maximum(iCount, (*i)->m_isStair);
	}

	return iCount;
}

size_t CItemMultiCustom::GetFixtureCount(DesignDetails * pDesign)
{
	ADDTOCALLSTACK("CItemMultiCustom::GetFixtureCount");
	if ( pDesign == NULL )
		pDesign = &m_designMain;

	size_t count = 0;
	for ( ComponentsContainer::iterator i = pDesign->m_vectorComponents.begin(); i != pDesign->m_vectorComponents.end(); ++i)
	{
		if ((*i)->m_item.m_visible)
			continue;

		count++;
	}

	return count;
}

size_t CItemMultiCustom::GetComponentsAt(short x, short y, signed char z, Component ** pComponents, DesignDetails * pDesign)
{
	ADDTOCALLSTACK("CItemMultiCustom::GetComponentsAt");
	// find a list of components that are located at the given
	// position, and store them in the given Component* array,
	// returning the number of components found

	if ( pDesign == NULL )
		pDesign = &m_designMain;

	size_t count = 0;
	Component * pComponent;
	for ( size_t i = 0; i < pDesign->m_vectorComponents.size(); i++ )
	{
		pComponent = pDesign->m_vectorComponents.at(i);

		if ( pComponent->m_item.m_dx != x || pComponent->m_item.m_dy != y )
			continue;

		if ( z != -128 && pComponent->m_item.m_dz != z )
			continue;

		pComponents[count++] = pComponent;
	}

	return count;
}

const CPointMap CItemMultiCustom::GetComponentPoint(Component * pComp) const
{
	ADDTOCALLSTACK("CItemMultiCustom::GetComponentPoint");
	return GetComponentPoint(pComp->m_item.m_dx, pComp->m_item.m_dy, pComp->m_item.m_dz);
}

const CPointMap CItemMultiCustom::GetComponentPoint(int dx, int dy, int dz) const
{
	ADDTOCALLSTACK("CItemMultiCustom::GetComponentPoint");
	// return the real world location from the given offset
	CPointMap ptBase(GetTopPoint());
	ptBase.m_x += dx;
	ptBase.m_y += dy;
	ptBase.m_z += dz;

	return ptBase;
}

const CItemMultiCustom::CGrayMultiCustom * CItemMultiCustom::GetMultiItemDefs()
{
	ADDTOCALLSTACK("CItemMultiCustom::GetMultiItemDefs");
	// return a CGrayMultiCustom object that represents the components
	// in the main design
	if ( m_pGrayMulti == NULL )
	{
		m_pGrayMulti = new CGrayMultiCustom;
		m_pGrayMulti->LoadFrom(&m_designMain);
	}

	return m_pGrayMulti;
}

const CGRect CItemMultiCustom::GetDesignArea()
{
	ADDTOCALLSTACK("CItemMultiCustom::GetDesignArea");
	// return the foundation dimensions, which is the design
	// area editable by clients

	if ( m_rectDesignArea.IsRectEmpty() )
	{
		m_rectDesignArea.SetRect(0, 0, 1, 1, GetTopMap());

		const CGrayMulti * pMulti = g_Cfg.GetMultiItemDefs(GetID());
		if ( pMulti != NULL )
		{
			// the client uses the multi items to determine the area
			// that's editable
			size_t iQty = pMulti->GetItemCount();
			for ( size_t i = 0; i < iQty; i++ )
			{
				const CUOMultiItemRec2 * pMultiItem = pMulti->GetItem(i);
				if ( pMultiItem == NULL )
					continue;

				if ( !pMultiItem->m_visible )
					continue;

				m_rectDesignArea.UnionPoint( pMultiItem->m_dx, pMultiItem->m_dy );
			}
		}
		else
		{
			// multi data is not available, so assume the region boundaries
			// are correct
			CGRect rectRegion = m_pRegion->GetRegionRect(0);
			m_rectDesignArea.SetRect(rectRegion.m_left, rectRegion.m_top, rectRegion.m_right, rectRegion.m_top, rectRegion.m_map);

			const CPointMap pt = GetTopPoint();
			m_rectDesignArea.OffsetRect(-pt.m_x, -pt.m_y);

			DEBUG_WARN(("Multi data is not available for customizable building 0%x.", GetID()));
		}
	}

	CGRect rect;
	const CPointMap pt = GetTopPoint();
	
	rect.SetRect(m_rectDesignArea.m_left, m_rectDesignArea.m_top, m_rectDesignArea.m_right, m_rectDesignArea.m_bottom, GetTopMap());
	rect.OffsetRect(pt.m_x, pt.m_y);

	return rect;
}

void CItemMultiCustom::CopyDesign(DesignDetails * designFrom, DesignDetails * designTo)
{
	ADDTOCALLSTACK("CItemMultiCustom::CopyComponents");
	// overwrite the details of one design with the details
	// of another
	Component * pComponent;

	// copy components
	designTo->m_vectorComponents.clear();
	for ( ComponentsContainer::iterator i = designFrom->m_vectorComponents.begin(); i != designFrom->m_vectorComponents.end(); ++i)
	{
		pComponent = new Component;
		*pComponent = **i;

		designTo->m_vectorComponents.push_back(pComponent);
	}

	// copy revision
	designTo->m_iRevision = designFrom->m_iRevision;

	// copy saved packet
	if ( designTo->m_pData != NULL )
		delete designTo->m_pData;

	if ( designFrom->m_pData != NULL )
	{
		designTo->m_pData = new PacketHouseDesign(designFrom->m_pData);
		designTo->m_iDataRevision = designFrom->m_iDataRevision;
	}
	else
	{
		designTo->m_pData = NULL;
		designTo->m_iDataRevision = 0;
	}
}

enum
{
	IMCV_ADDITEM,
	IMCV_ADDMULTI,
	IMCV_CLEAR,
	IMCV_COMMIT,
	IMCV_CUSTOMIZE,
	IMCV_ENDCUSTOMIZE,
	IMCV_REMOVEITEM,
	IMCV_RESET,
	IMCV_RESYNC,
	IMCV_REVERT,
	IMCV_QTY
};

LPCTSTR const CItemMultiCustom::sm_szVerbKeys[IMCV_QTY+1] =
{
	"ADDITEM",
	"ADDMULTI",
	"CLEAR",
	"COMMIT",
	"CUSTOMIZE",
	"ENDCUSTOMIZE",
	"REMOVEITEM",
	"RESET",
	"RESYNC",
	"REVERT",
	NULL
};

bool CItemMultiCustom::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CItemMultiCustom::r_GetRef");
	if (!strcmpi("DESIGNER.", pszKey))
	{
		pszKey += 9;
		pRef = (m_pArchitect? m_pArchitect->GetChar() : NULL);
		return true;
	}

	return( CItemMulti::r_GetRef( pszKey, pRef ));
}

bool CItemMultiCustom::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CItemMultiCustom::r_Verb");
	EXC_TRY("Verb");
	// Speaking in this multis region.
	// return: true = command for the multi.

	int iCmd = FindTableSorted( s.GetKey(), sm_szVerbKeys, COUNTOF( sm_szVerbKeys )-1 );
	if ( iCmd < 0 )
	{
		return( CItemMulti::r_Verb( s, pSrc ));
	}

	CChar * pChar = (pSrc != NULL? pSrc->GetChar() : NULL);

	switch ( iCmd )
	{
		case IMCV_ADDITEM:
		{
			TCHAR * ppArgs[4];
			size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), ",");
			if ( iQty != 4 )
				return false;

			AddItem(NULL,
					static_cast<ITEMID_TYPE>(Exp_GetVal(ppArgs[0])),
					Exp_GetVal(ppArgs[1]),
					Exp_GetVal(ppArgs[2]),
					Exp_GetVal(ppArgs[3]));
		} break;

		case IMCV_ADDMULTI:
		{
			TCHAR * ppArgs[4];
			size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), ",");
			if ( iQty != 4 )
				return false;

			ITEMID_TYPE id = static_cast<ITEMID_TYPE>(Exp_GetVal(ppArgs[0]));
			if ( id <= 0 )
				return false;

			// if a multi is added that is not normally allowed through
			// design mode, the pieces need to be added individually
			AddStairs(NULL,
					id,
					Exp_GetVal(ppArgs[1]),
					Exp_GetVal(ppArgs[2]),
					Exp_GetVal(ppArgs[3]),
					(sm_mapValidItems.find(id) == sm_mapValidItems.end()? 0 : -1));
		} break;

		case IMCV_CLEAR:
		{
			m_designWorking.m_vectorComponents.clear();
			m_designWorking.m_iRevision = 1;
		} break;

		case IMCV_COMMIT:
		{
			CommitChanges();
		} break;

		case IMCV_CUSTOMIZE:
		{
			if ( s.HasArgs() )
				pChar = CGrayUID(s.GetArgVal()).CharFind();

			if ( pChar == NULL || !pChar->IsClient() )
				return false;

			BeginCustomize(pChar->GetClient());
		} break;

		case IMCV_ENDCUSTOMIZE:
		{
			EndCustomize(true);
		} break;

		case IMCV_REMOVEITEM:
		{
			TCHAR * ppArgs[4];
			size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), ",");
			if ( iQty != 4 )
				return false;

			RemoveItem(NULL,
					static_cast<ITEMID_TYPE>(Exp_GetVal(ppArgs[0])),
					Exp_GetVal(ppArgs[1]),
					Exp_GetVal(ppArgs[2]),
					Exp_GetVal(ppArgs[3]));
		} break;

		case IMCV_RESET:
		{
			ResetStructure();
		} break;

		case IMCV_RESYNC:
		{
			if ( s.HasArgs() )
				pChar = CGrayUID(s.GetArgVal()).CharFind();

			if ( pChar == NULL || !pChar->IsClient() )
				return false;

			SendStructureTo(pChar->GetClient());
		} break;

		case IMCV_REVERT:
		{
			CopyDesign(&m_designMain, &m_designWorking);
			m_designWorking.m_iRevision++;
		} break;

		default:
		{
			return CItemMulti::r_Verb( s, pSrc );
		}
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return true;
}

void CItemMultiCustom::r_Write( CScript & s )
{
	ADDTOCALLSTACK_INTENSIVE("CItemMultiCustom::r_Write");
	CItemMulti::r_Write(s);

	Component * comp;
	for ( ComponentsContainer::iterator i = m_designMain.m_vectorComponents.begin(); i != m_designMain.m_vectorComponents.end(); ++i )
	{
		comp = *i;
		s.WriteKeyFormat("COMP", "%d,%d,%d,%d,%d", comp->m_item.GetDispID(), comp->m_item.m_dx, comp->m_item.m_dy, comp->m_item.m_dz, comp->m_isStair);
	}

	if ( m_designMain.m_iRevision )
		s.WriteKeyVal("REVISION", m_designMain.m_iRevision);
}

enum IMCC_TYPE
{
	IMCC_COMPONENTS,
	IMCC_DESIGN,
	IMCC_DESIGNER,
	IMCC_EDITAREA,
	IMCC_FIXTURES,
	IMCC_REVISION,
	IMCC_QTY
};

LPCTSTR const CItemMultiCustom::sm_szLoadKeys[IMCC_QTY+1] = // static
{
	"COMPONENTS",
	"DESIGN",
	"DESIGNER",
	"EDITAREA",
	"FIXTURES",
	"REVISION",
	NULL
};

bool CItemMultiCustom::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CItemMultiCustom::r_WriteVal");
	EXC_TRY("WriteVal");

	int index = FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 );
	if ( index == -1 )
	{
		if ( !strnicmp(pszKey, "DESIGN.", 5) )
			index = IMCC_DESIGN;
	}

	switch ( index )
	{
		case IMCC_COMPONENTS:
			pszKey += 10;
			sVal.FormatVal(m_designMain.m_vectorComponents.size());
			break;

		case IMCC_DESIGN:
		{
			pszKey += 6;
			if ( !*pszKey )
				sVal.FormatVal(m_designMain.m_vectorComponents.size());
			else if ( *pszKey == '.' )
			{
				SKIP_SEPARATORS(pszKey);
				size_t iQty = static_cast<size_t>( Exp_GetVal(pszKey) );
				if ( iQty >= m_designMain.m_vectorComponents.size() )
					return false;

				SKIP_SEPARATORS(pszKey);
				CUOMultiItemRec2 item = m_designMain.m_vectorComponents.at(iQty)->m_item;

				if ( !strcmpi(pszKey, "ID") ) sVal.FormatVal(item.GetDispID());
				else if ( !strcmpi(pszKey, "DX") ) sVal.FormatVal(item.m_dx);
				else if ( !strcmpi(pszKey, "DY") ) sVal.FormatVal(item.m_dy);
				else if ( !strcmpi(pszKey, "DZ") ) sVal.FormatVal(item.m_dz);
				else if ( !strcmpi(pszKey, "D") ) sVal.Format("%i,%i,%i", item.m_dx, item.m_dy, item.m_dz);
				else if ( !strcmpi(pszKey, "FIXTURE") ) sVal.FormatVal(item.m_visible? 0:1);
				else if ( !*pszKey ) sVal.Format("%u,%i,%i,%i", item.GetDispID(), item.m_dx, item.m_dy, item.m_dz);
				else return false;
			}
			else
				return false;

		} break;

		case IMCC_DESIGNER:
		{
			pszKey += 8;
			CChar * pDesigner = m_pArchitect? m_pArchitect->GetChar() : NULL;
			if ( pDesigner != NULL )
				sVal.FormatHex(pDesigner->GetUID());
			else
				sVal.FormatHex(0);

		} break;

		case IMCC_EDITAREA:
		{
			pszKey += 8;
			CGRect rectDesign = GetDesignArea();
			sVal.Format("%d,%d,%d,%d", rectDesign.m_left, rectDesign.m_top, rectDesign.m_right, rectDesign.m_bottom);
		} break;

		case IMCC_FIXTURES:
			pszKey += 8;
			sVal.FormatVal(GetFixtureCount());
			break;

		case IMCC_REVISION:
			pszKey += 8;
			sVal.FormatVal(m_designMain.m_iRevision);
			break;

		default:
			return CItemMulti::r_WriteVal(pszKey, sVal, pSrc);
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CItemMultiCustom::r_LoadVal( CScript & s  )
{
	ADDTOCALLSTACK("CItemMultiCustom::r_LoadVal");
	EXC_TRY("LoadVal");

	if ( g_Serv.IsLoading() )
	{
		if ( s.IsKey("COMP") )
		{
			TCHAR * ppArgs[5];
			size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), ",");
			if ( iQty != 5 )
				return false;

			AddItem(NULL,
					static_cast<ITEMID_TYPE>(ATOI(ppArgs[0])),
					ATOI(ppArgs[1]),
					ATOI(ppArgs[2]),
					ATOI(ppArgs[3]),
					ATOI(ppArgs[4]));
			return true;
		}
		else if ( s.IsKey("REVISION") )
		{
			m_designWorking.m_iRevision = s.GetArgVal();
			CommitChanges();
			return true;
		}
	}
	return CItemMulti::r_LoadVal(s);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

unsigned char CItemMultiCustom::GetPlane( signed char z )
{
	if ( z >= 67 )
		return 4;
	else if ( z >= 47 )
		return 3;
	else if ( z >= 27 )
		return 2;
	else if ( z >= 7 )
		return 1;
	else
		return 0;
}

unsigned char CItemMultiCustom::GetPlane( Component * pComponent )
{
	return GetPlane(pComponent->m_item.m_dz);
}

signed char CItemMultiCustom::GetPlaneZ( unsigned char plane )
{
	switch ( plane )
	{
		case 7:
			return 127;
		case 6:
			return 107;
		case 5:
			return 87;
		case 4:
			return 67;
		case 3:
			return 47;
		case 2:
			return 27;
		case 1:
			return 7;
		case 0:
		default:
			return 0;
	}
}

bool CItemMultiCustom::IsValidItem( ITEMID_TYPE id, CClient * pClientSrc, bool bMulti )
{
	ADDTOCALLSTACK("CItemMultiCustom::IsValidItem");
	if ( !bMulti && (id <= 0 || id >= ITEMID_MULTI) )
		return false;
	else if ( bMulti && (id < ITEMID_MULTI || id > ITEMID_MULTI_MAX) )
		return false;

	// scripts can place any item
	if ( pClientSrc == NULL )
		return true;

	// gms can place any item
	if ( pClientSrc->IsPriv( PRIV_GM ) )
		return true;

	// load item lists
	if ( !LoadValidItems() )
		return false;

	// check the item exists in the database
	ValidItemsContainer::iterator it = sm_mapValidItems.find(id);
	if ( it == sm_mapValidItems.end() )
		return false;

	// check that the client's enabled features contains the item's featuremask
	int iFeatureFlag = g_Cfg.GetPacketFlag(false, static_cast<RESDISPLAY_VERSION>(pClientSrc->GetResDisp()));
	if ((iFeatureFlag & it->second) != it->second)
		return false;

	return true;
}

CItemMultiCustom::ValidItemsContainer CItemMultiCustom::sm_mapValidItems;

bool CItemMultiCustom::LoadValidItems()
{
	ADDTOCALLSTACK("CItemMultiCustom::LoadValidItems");
	if ( !sm_mapValidItems.empty() )	// already loaded?
		return true;

	static const char * sm_szItemFiles[][32] = {
		// list of files containing valid items
		{ "doors.txt", "Piece1", "Piece2", "Piece3", "Piece4", "Piece5", "Piece6", "Piece7", "Piece8", NULL },
		{ "misc.txt", "Piece1", "Piece2", "Piece3", "Piece4", "Piece5", "Piece6", "Piece7", "Piece8", NULL },
		{ "floors.txt", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16", NULL },
		{ "teleprts.txt", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16", NULL },
		{ "roof.txt", "North", "East", "South", "West", "NSCrosspiece", "EWCrosspiece", "NDent", "EDent", "SDent", "WDent", "NTPiece", "ETPiece", "STPiece", "WTPiece", "XPiece", "Extra Piece", NULL },
		{ "walls.txt", "South1", "South2", "South3", "Corner", "East1", "East2", "East3", "Post", "WindowS", "AltWindowS", "WindowE", "AltWindowE", "SecondAltWindowS", "SecondAltWindowE", NULL },
		{ "stairs.txt", "Block", "North", "East", "South", "West", "Squared1", "Squared2", "Rounded1", "Rounded2", NULL },
		{ NULL },
		// list of files containing valid multis
		{ "stairs.txt", "MultiNorth", "MultiEast", "MultiSouth", "MultiWest", NULL },
		{ NULL }
	};

	CSVRowData csvDataRow;
	bool bMultiFile = false;
	int iFileIndex = 0;
	int i = 0;

	EXC_TRY("LoadCSVFiles");

	for (i = 0; sm_szItemFiles[i][0] != NULL || !bMultiFile; i++, iFileIndex++)
	{
		if ( sm_szItemFiles[i][0] == NULL )
		{
			bMultiFile = true;
			iFileIndex--;
			continue;
		}

		if ( !g_Install.m_CsvFiles[iFileIndex].IsFileOpen() && !g_Install.OpenFile(g_Install.m_CsvFiles[iFileIndex], sm_szItemFiles[i][0], OF_READ|OF_SHARE_DENY_WRITE ) )
			continue;

		while (g_Install.m_CsvFiles[iFileIndex].ReadNextRowContent(csvDataRow))
		{
			for (int ii = 1; sm_szItemFiles[i][ii] != NULL; ii++)
			{
				ITEMID_TYPE itemid = static_cast<ITEMID_TYPE>(ATOI(csvDataRow[sm_szItemFiles[i][ii]].c_str()));
				if ( itemid <= 0 || itemid >= ITEMID_MULTI )
					continue;

				if ( bMultiFile )
				{
					itemid = static_cast<ITEMID_TYPE>(itemid + ITEMID_MULTI);
					if ( itemid <= ITEMID_MULTI || itemid > ITEMID_MULTI_MAX )
						continue;
				}

				sm_mapValidItems[itemid] = ATOI(csvDataRow["FeatureMask"].c_str());
			}
		}

		g_Install.m_CsvFiles[iFileIndex].Close();
	}

	// make sure we have at least 1 item
	sm_mapValidItems[ITEMID_NOTHING] = 0xFFFFFFFF;
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("file index '%d\n", iFileIndex);
	g_Log.EventDebug("file name '%s'\n", sm_szItemFiles[i][0]);

	TCHAR* pszRowFull = Str_GetTemp();
	TCHAR* pszHeaderFull = Str_GetTemp();
	for ( CSVRowData::iterator itCsv = csvDataRow.begin(); itCsv != csvDataRow.end(); ++itCsv )
	{
		strcat(pszHeaderFull, "\t");
		strcat(pszHeaderFull, itCsv->first.c_str());

		strcat(pszRowFull, "\t");
		strcat(pszRowFull, itCsv->second.c_str());
	}

	g_Log.EventDebug("header count '%" FMTSIZE_T "', header text '%s'\n", csvDataRow.size(), pszRowFull);
	g_Log.EventDebug("column count '%" FMTSIZE_T "', row text '%s'\n", csvDataRow.size(), pszHeaderFull);
	EXC_DEBUG_END;
	return false;
}

void CItemMultiCustom::CGrayMultiCustom::LoadFrom( CItemMultiCustom::DesignDetails * pDesign )
{
	m_iItemQty = pDesign->m_vectorComponents.size();

	m_pItems = new CUOMultiItemRec2[m_iItemQty];
	for ( size_t i = 0; i < m_iItemQty; i++ )
		memcpy(&m_pItems[i], &pDesign->m_vectorComponents.at(i)->m_item, sizeof(CUOMultiItemRec2));
}
