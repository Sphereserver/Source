#include "graysvr.h"	// predef header.
#include "../network/send.h"

CItemMultiCustom::CItemMultiCustom(ITEMID_TYPE id, CItemBase *pItemDef) : CItemMulti(id, pItemDef)
{
	m_designMain.m_dwRevision = 0;
	m_designMain.m_pPacket = NULL;
	m_designMain.m_dwPacketRevision = 0;
	m_designWorking.m_dwRevision = 0;
	m_designWorking.m_pPacket = NULL;
	m_designWorking.m_dwPacketRevision = 0;
	m_designBackup.m_dwRevision = 0;
	m_designBackup.m_pPacket = NULL;
	m_designBackup.m_dwPacketRevision = 0;
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
	for ( ComponentsContainer::iterator i = m_designMain.m_vectorComponents.begin(); i != m_designMain.m_vectorComponents.end(); i = m_designMain.m_vectorComponents.erase(i) )
		delete *i;
	for ( ComponentsContainer::iterator i = m_designWorking.m_vectorComponents.begin(); i != m_designWorking.m_vectorComponents.end(); i = m_designWorking.m_vectorComponents.erase(i) )
		delete *i;
	for ( ComponentsContainer::iterator i = m_designBackup.m_vectorComponents.begin(); i != m_designBackup.m_vectorComponents.end(); i = m_designBackup.m_vectorComponents.erase(i) )
		delete *i;

	m_designMain.m_vectorComponents.clear();
	m_designWorking.m_vectorComponents.clear();
	m_designBackup.m_vectorComponents.clear();

	if ( m_designMain.m_pPacket )
	{
		delete m_designMain.m_pPacket;
		m_designMain.m_pPacket = NULL;
		m_designMain.m_dwPacketRevision = 0;
	}
	if ( m_designWorking.m_pPacket )
	{
		delete m_designWorking.m_pPacket;
		m_designWorking.m_pPacket = NULL;
		m_designWorking.m_dwPacketRevision = 0;
	}
	if ( m_designBackup.m_pPacket )
	{
		delete m_designBackup.m_pPacket;
		m_designBackup.m_pPacket = NULL;
		m_designBackup.m_dwPacketRevision = 0;
	}

	if ( m_pArchitect )
	{
		m_pArchitect->m_pHouseDesign = NULL;
		m_pArchitect = NULL;
	}

	if ( m_pGrayMulti )
	{
		delete m_pGrayMulti;
		m_pGrayMulti = NULL;
	}
}

void CItemMultiCustom::BeginCustomize(CClient *pClient)
{
	ADDTOCALLSTACK("CItemMultiCustom::BeginCustomize");
	// Make client enter house customization mode
	if ( !pClient || !pClient->GetChar() || !PacketHouseBeginCustomize::CanSendTo(pClient->m_NetState) )
		return;

	if ( m_pArchitect )
		EndCustomize(true);

	// Client will close all opened dialogs without notify the server, so dialogs must be closed on server-side too
	CDialogDef *pDialog = NULL;
	for ( CClient::OpenedGumpsMap_t::iterator it = pClient->m_mapOpenedGumps.begin(); it != pClient->m_mapOpenedGumps.end(); ++it )
	{
		pDialog = dynamic_cast<CDialogDef *>(g_Cfg.ResourceGetDef(RESOURCE_ID(RES_DIALOG, it->first)));
		if ( pDialog && pDialog->m_bNoDispose )		// client will keep 'nodispose' dialogs open, so they doesn't need to be closed
			continue;

		it->second = 0;
	}

	// Move char inside building and hide it
	CChar *pChar = pClient->GetChar();
	pChar->StatFlag_Set(STATF_Hidden);

	CPointMap ptOld = pChar->GetTopPoint();
	CPointMap ptNew(GetTopPoint());
	ptNew.m_z += 7;

	pChar->MoveToChar(ptNew);
	pChar->UpdateMove(ptOld);

	// Hide all dynamic items inside the building
	const CGRect rect = GetDesignArea();
	CWorldSearch Area(GetTopPoint(), Multi_GetMaxDist());
	for (;;)
	{
		CItem *pItem = Area.GetItem();
		if ( !pItem )
			break;
		if ( (pItem == this) || !rect.IsInsideX(pItem->GetTopPoint().m_x) || !rect.IsInsideY(pItem->GetTopPoint().m_y - 1) )
			continue;

		pClient->addObjectRemove(pItem);
	}

	// Enter customization mode
	CopyDesign(&m_designMain, &m_designWorking);
	++m_designWorking.m_dwRevision;

	m_pArchitect = pClient;
	new PacketHouseBeginCustomize(pClient, this);

	pClient->m_pHouseDesign = this;
	pClient->addItem(this);
	SendStructureTo(pClient);
}

void CItemMultiCustom::EndCustomize(bool fForce)
{
	ADDTOCALLSTACK("CItemMultiCustom::EndCustomize");
	// Exit house customization mode
	CClient *pClient = m_pArchitect;
	if ( !pClient )
		return;

	// Exit customization mode
	new PacketHouseEndCustomize(pClient, this);
	pClient->m_pHouseDesign = NULL;
	m_pArchitect = NULL;

	CChar *pChar = pClient->GetChar();
	if ( !pChar )
		return;

	if ( IsTrigUsed(TRIGGER_HOUSEDESIGNEXIT) )
	{
		CScriptTriggerArgs Args(this);
		Args.m_iN1 = fForce;

		if ( (pChar->OnTrigger(CTRIG_HouseDesignExit, pChar, &Args) == TRIGRET_RET_TRUE) && !fForce )
		{
			BeginCustomize(pClient);
			return;
		}
	}

	// Move char outside building and reveal it
	pChar->StatFlag_Clear(STATF_Hidden);

	CItem *pSign = Multi_GetSign();
	CPointMap ptNew = pSign ? pSign->GetTopPoint() : GetTopPoint();

	// Find ground height, since the signpost is usually raised
	DWORD dwBlockFlags = 0;
	ptNew.m_z = g_World.GetHeightPoint2(ptNew, dwBlockFlags, true);

	pChar->MoveToChar(ptNew);
	pChar->UpdateMove(NULL);
}

void CItemMultiCustom::AddItem(CClient *pClient, ITEMID_TYPE id, signed short x, signed short y, signed char z, int iStairID)
{
	ADDTOCALLSTACK("CItemMultiCustom::AddItem");
	// Add item to the building design at the given location

	const CChar *pChar = pClient ? pClient->GetChar() : NULL;
	CItemBase *pItemBase = CItemBase::FindItemBase(id);
	if ( !pItemBase )
	{
		g_Log.EventWarn("Char UID=0%" FMTDWORDH " is trying to add unscripted item 0%x on multi UID=0%" FMTDWORDH " using house customization menu\n", pChar ? static_cast<DWORD>(pChar->GetUID()) : 0, id, static_cast<DWORD>(GetUID()));
		SendStructureTo(pClient);
		return;
	}

	bool fFloor = (pItemBase->GetHeight() == 0);
	bool fFixture = (pItemBase->IsType(IT_DOOR) || pItemBase->IsType(IT_DOOR_LOCKED) || pItemBase->IsID_Door(id) || pItemBase->IsType(IT_TELEPAD));

	if ( !g_Serv.IsLoading() )
	{
		if ( !IsValidItem(id, pClient, false) )
		{
			g_Log.EventWarn("Char UID=0%" FMTDWORDH " is trying to add invalid item 0%x on multi UID=0%" FMTDWORDH " using house customization menu\n", pChar ? static_cast<DWORD>(pChar->GetUID()) : 0, id, static_cast<DWORD>(GetUID()));
			SendStructureTo(pClient);
			return;
		}

		if ( pClient )
		{
			CPointMap pt = GetTopPoint();
			pt.m_x += x;
			pt.m_y += y;

			const CGRect rect = GetDesignArea();
			if ( !rect.IsInside2d(pt) )
			{
				if ( (pItemBase->GetTFlags() & UFLAG2_CLIMBABLE) && rect.IsInsideY(pt.m_y - 1) )
				{
					// External stairs can be placed 1 tile south of the edit area, but only at ground level
					z = 0;
				}
				else
				{
					g_Log.EventWarn("Char UID=0%" FMTDWORDH " is trying to add item 0%x at invalid location on multi UID=0%" FMTDWORDH " using house customization menu\n", pChar ? static_cast<DWORD>(pChar->GetUID()) : 0, id, static_cast<DWORD>(GetUID()));
					SendStructureTo(pClient);
					return;
				}
			}
			else
			{
				if ( z == SCHAR_MIN )
					z = pChar ? GetLevelZ(GetLevel(pChar->GetTopZ() - GetTopZ())) : 0;
			}
		}

		// Delete existing item at given location
		Component *pPrevComponents[SCHAR_MAX];
		size_t iCount = GetComponentsAt(x, y, z, pPrevComponents, &m_designWorking);
		if ( iCount > 0 )
		{
			for ( size_t i = 0; i < iCount; ++i )
			{
				if ( (fFloor != pPrevComponents[i]->m_fFloor) || ((iStairID > 0) && (iStairID == pPrevComponents[i]->m_iStairID)) )
					continue;

				RemoveItem(NULL, pPrevComponents[i]->m_item.GetDispID(), pPrevComponents[i]->m_item.m_dx, pPrevComponents[i]->m_item.m_dy, static_cast<signed char>(pPrevComponents[i]->m_item.m_dz));
			}
		}
	}

	// Create new item at given location
	Component *pComponent = new Component;
	pComponent->m_item.m_wTileID = static_cast<WORD>(id);
	pComponent->m_item.m_dx = x;
	pComponent->m_item.m_dy = y;
	pComponent->m_item.m_dz = z;
	pComponent->m_item.m_visible = !fFixture;
	pComponent->m_iStairID = iStairID;
	pComponent->m_fFloor = fFloor;

	m_designWorking.m_vectorComponents.push_back(pComponent);
	++m_designWorking.m_dwRevision;
}

void CItemMultiCustom::RemoveItem(CClient *pClient, ITEMID_TYPE id, signed short x, signed short y, signed char z)
{
	ADDTOCALLSTACK("CItemMultiCustom::RemoveItem");
	// Remove item from the building design at the given location
	// NOTE: id = ITEMID_NOTHING means that any id should be removed

	const CGRect rect = GetDesignArea();
	CPointMap pt = GetTopPoint();
	pt.m_x += x;
	pt.m_y += y;

	if ( pClient )
	{
		switch ( GetLevel(z) )
		{
			case 0:
			{
				// At ground level, clients can only remove external stairs
				if ( pt.m_y != rect.m_bottom )
				{
					SendStructureTo(pClient);
					return;
				}
				break;
			}
			case 1:
			{
				// At first level, clients can't remove dirt floor tiles
				if ( id == ITEMID_DIRT_TILE )
				{
					SendStructureTo(pClient);
					return;
				}
				break;
			}
		}
	}

	Component *pComponentList[SCHAR_MAX];
	size_t iCount = GetComponentsAt(x, y, z, pComponentList, &m_designWorking);
	if ( iCount <= 0 )
		return;

	bool fRestoreDirt;
	Component *pComponent;
	for ( size_t i = 0; i < iCount; ++i )
	{
		for ( ComponentsContainer::iterator j = m_designWorking.m_vectorComponents.begin(); j != m_designWorking.m_vectorComponents.end(); ++j )
		{
			pComponent = *j;
			if ( pComponent != pComponentList[i] )
				continue;
			if ( (id != ITEMID_NOTHING) && (pComponent->m_item.GetDispID() != id) )
				break;

			if ( pComponent->m_iStairID > 0 )
			{
				for ( ComponentsContainer::iterator k = m_designWorking.m_vectorComponents.begin(); k != m_designWorking.m_vectorComponents.end(); )
				{
					if ( (*k)->m_iStairID != pComponent->m_iStairID )
					{
						++k;
						continue;
					}

					k = m_designWorking.m_vectorComponents.erase(k);
					++m_designWorking.m_dwRevision;
				}
			}
			else
			{
				fRestoreDirt = (pClient && pComponent->m_fFloor && (GetLevel(pComponent->m_item.m_dz) == 1) && (GetLevelZ(GetLevel(pComponent->m_item.m_dz)) == pComponent->m_item.m_dz) && (pComponent->m_item.m_wTileID != ITEMID_DIRT_TILE));

				m_designWorking.m_vectorComponents.erase(j);
				++m_designWorking.m_dwRevision;

				if ( fRestoreDirt )
					AddItem(NULL, ITEMID_DIRT_TILE, x, y, 7);
			}
			break;
		}
	}

	SendStructureTo(pClient);
}

void CItemMultiCustom::AddStairs(CClient *pClient, ITEMID_TYPE id, signed short x, signed short y, signed char z)
{
	ADDTOCALLSTACK("CItemMultiCustom::AddStairs");
	// Add stairs to the building design at the given location

	const CChar *pChar = pClient ? pClient->GetChar() : NULL;
	const CGrayMulti *pMulti = g_Cfg.GetMultiItemDefs(id);
	if ( !pMulti )
	{
		g_Log.EventWarn("Char UID=0%" FMTDWORDH " is trying to add unscripted stairs 0%x on multi UID=0%" FMTDWORDH " using house customization menu\n", pChar ? static_cast<DWORD>(pChar->GetUID()) : 0, id, static_cast<DWORD>(GetUID()));
		SendStructureTo(pClient);
		return;
	}

	if ( !IsValidItem(id, pClient, true) )
	{
		g_Log.EventWarn("Char UID=0%" FMTDWORDH " is trying to add invalid stairs 0%x on multi UID=0%" FMTDWORDH " using house customization menu\n", pChar ? static_cast<DWORD>(pChar->GetUID()) : 0, id, static_cast<DWORD>(GetUID()));
		SendStructureTo(pClient);
		return;
	}

	if ( z == SCHAR_MIN )
		z = pChar ? GetLevelZ(GetLevel(pChar->GetTopZ() - GetTopZ())) : 0;

	int iStairID = 0;
	for ( ComponentsContainer::iterator i = m_designWorking.m_vectorComponents.begin(); i != m_designWorking.m_vectorComponents.end(); ++i )
	{
		if ( (*i)->m_iStairID > iStairID )
			iStairID = (*i)->m_iStairID;
	}
	++iStairID;

	size_t iCount = pMulti->GetItemCount();
	for ( size_t i = 0; i < iCount; ++i )
	{
		const CUOMultiItemRecHS *pMultiItem = pMulti->GetItem(i);
		if ( !pMultiItem || !pMultiItem->m_visible )
			continue;

		AddItem(NULL, pMultiItem->GetDispID(), x + pMultiItem->m_dx, y + pMultiItem->m_dy, z + static_cast<signed char>(pMultiItem->m_dz), iStairID);
	}
	SendStructureTo(pClient);
}

void CItemMultiCustom::AddRoof(CClient *pClient, ITEMID_TYPE id, signed short x, signed short y, signed char z)
{
	ADDTOCALLSTACK("CItemMultiCustom::AddRoof");
	// add a roof piece to the building
	const CChar *pChar = pClient ? pClient->GetChar() : NULL;
	CItemBase *pItemBase = CItemBase::FindItemBase(id);
	if ( !pItemBase )
	{
		g_Log.EventWarn("Char UID=0%" FMTDWORDH " is trying to add unscripted roof 0%x on multi UID=0%" FMTDWORDH " using house customization menu\n", pChar ? static_cast<DWORD>(pChar->GetUID()) : 0, id, static_cast<DWORD>(GetUID()));
		SendStructureTo(pClient);
		return;
	}

	if ( (pItemBase->GetTFlags() & UFLAG4_ROOF) == 0 )
	{
		g_Log.EventWarn("Char UID=0%" FMTDWORDH " is trying to add invalid roof 0%x on multi UID=0%" FMTDWORDH " using house customization menu\n", pChar ? static_cast<DWORD>(pChar->GetUID()) : 0, id, static_cast<DWORD>(GetUID()));
		SendStructureTo(pClient);
		return;
	}

	if ( (z < -3) || (z > 12) || ((z % 3) != 0) )
	{
		g_Log.EventWarn("Char UID=0%" FMTDWORDH " is trying to add roof 0%x at invalid height on multi UID=0%" FMTDWORDH " using house customization menu\n", pChar ? static_cast<DWORD>(pChar->GetUID()) : 0, id, static_cast<DWORD>(GetUID()));
		SendStructureTo(pClient);
		return;
	}

	if ( pChar )
		z += GetLevelZ(GetLevel(pChar->GetTopZ() - GetTopZ()));

	AddItem(pClient, id, x, y, z);
}

void CItemMultiCustom::RemoveRoof(CClient *pClient, ITEMID_TYPE id, signed short x, signed short y, signed char z)
{
	ADDTOCALLSTACK("CItemMultiCustom::RemoveRoof");
	CItemBase *pItemBase = CItemBase::FindItemBase(id);
	if ( !pItemBase )
		return;
	if ( (pItemBase->GetTFlags() & UFLAG4_ROOF) == 0 )
		return;

	RemoveItem(pClient, id, x, y, z);
}

void CItemMultiCustom::SwitchToLevel(CClient *pClient, BYTE bLevel)
{
	ADDTOCALLSTACK("CItemMultiCustom::SwitchToLevel");
	// Switch client to the given building level
	CChar *pChar = pClient->GetChar();
	if ( !pChar )
		return;

	const CGRect rect = GetDesignArea();
	BYTE bMaxLevel = ((rect.GetWidth() >= 14) || (rect.GetHeight() >= 14)) ? 4 : 3;
	if ( bLevel < 1 )
		bLevel = 1;
	else if ( bLevel > bMaxLevel )
		bLevel = bMaxLevel;

	CPointMap pt = GetTopPoint();
	pt.m_z += GetLevelZ(bLevel);

	pChar->SetTopZ(pt.m_z);
	pChar->UpdateMove(GetTopPoint());
	pClient->addItem(this);
}

void CItemMultiCustom::BackupStructure()
{
	ADDTOCALLSTACK("CItemMultiCustom::BackupStructure");
	// Create backup of working design
	if ( m_designWorking.m_dwRevision == m_designBackup.m_dwRevision )
		return;

	CopyDesign(&m_designWorking, &m_designBackup);
}

void CItemMultiCustom::RestoreStructure(CClient *pClient)
{
	ADDTOCALLSTACK("CItemMultiCustom::RestoreStructure");
	// Restore backup design to working design
	if ( (m_designBackup.m_dwRevision == m_designWorking.m_dwRevision) || (m_designBackup.m_dwRevision == 0) )
		return;

	CopyDesign(&m_designBackup, &m_designWorking);
	++m_designWorking.m_dwRevision;

	if ( pClient )
		pClient->addItem(this);
}

void CItemMultiCustom::RevertChanges(CClient *pClient)
{
	ADDTOCALLSTACK("CItemMultiCustom::RevertChanges");
	// Revert working design to main design
	CopyDesign(&m_designMain, &m_designWorking);
	++m_designWorking.m_dwRevision;

	if ( pClient )
		pClient->addItem(this);
}

void CItemMultiCustom::ResetStructure(CClient *pClient)
{
	ADDTOCALLSTACK("CItemMultiCustom::ResetStructure");
	// Reset working design to empty state
	m_designWorking.m_vectorComponents.clear();
	++m_designWorking.m_dwRevision;

	const CGrayMulti *pMulti = g_Cfg.GetMultiItemDefs(GetID());
	if ( !pMulti )
		return;

	size_t iCount = pMulti->GetItemCount();
	for ( size_t i = 0; i < iCount; ++i )
	{
		const CUOMultiItemRecHS *pMultiItem = pMulti->GetItem(i);
		if ( !pMultiItem || !pMultiItem->m_visible )
			continue;

		AddItem(NULL, pMultiItem->GetDispID(), pMultiItem->m_dx, pMultiItem->m_dy, static_cast<signed char>(pMultiItem->m_dz));
	}

	if ( pClient )
		pClient->addItem(this);
}

void CItemMultiCustom::CommitChanges(CClient *pClient)
{
	ADDTOCALLSTACK("CItemMultiCustom::CommitChanges");
	// Commit changes that have been made on working design
	if ( m_designWorking.m_dwRevision == m_designMain.m_dwRevision )
		return;

	CChar *pChar = pClient ? pClient->GetChar() : NULL;
	if ( pChar && IsTrigUsed(TRIGGER_HOUSEDESIGNCOMMIT) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = m_designMain.m_vectorComponents.size();
		Args.m_iN2 = m_designWorking.m_vectorComponents.size();
		Args.m_iN3 = m_designWorking.m_dwRevision;
		Args.m_pO1 = this;
		Args.m_VarsLocal.SetNum("FIXTURES.OLD", GetFixtureCount(&m_designMain));
		Args.m_VarsLocal.SetNum("FIXTURES.NEW", GetFixtureCount(&m_designWorking));

		if ( pChar->OnTrigger(CTRIG_HouseDesignCommit, pChar, &Args) == TRIGRET_RET_TRUE )
			return;
	}

	// Replace main design with working design
	CopyDesign(&m_designWorking, &m_designMain);

	if ( g_Serv.IsLoading() || !GetTopPoint().IsValidPoint() )
		return;

	// Delete existing fixtures
	CWorldSearch Area(GetTopPoint(), Multi_GetMaxDist());
	CItem *pItem;
	for (;;)
	{
		pItem = Area.GetItem();
		if ( !pItem )
			break;
		if ( GetUID() != static_cast<CGrayUID>(pItem->GetTagDefs()->GetKeyNum("FIXTURE")) )
			continue;

		pItem->Delete();
	}

	// Create new fixtures
	CRectMap rect;
	rect.SetRectEmpty();
	rect.m_map = GetTopMap();

	Component *pComponent;
	for ( ComponentsContainer::iterator i = m_designMain.m_vectorComponents.begin(); i != m_designMain.m_vectorComponents.end(); ++i )
	{
		pComponent = *i;
		rect.UnionPoint(pComponent->m_item.m_dx, pComponent->m_item.m_dy);
		if ( pComponent->m_item.m_visible )
			continue;

		pItem = CItem::CreateScript(pComponent->m_item.GetDispID());
		if ( !pItem )
			continue;

		CPointMap pt = GetTopPoint();
		pt.m_x += pComponent->m_item.m_dx;
		pt.m_y += pComponent->m_item.m_dy;
		pt.m_z += static_cast<signed char>(pComponent->m_item.m_dz);

		pItem->m_uidLink = GetUID();
		pItem->ClrAttr(ATTR_DECAY);
		pItem->SetAttr(ATTR_MOVE_NEVER);
		pItem->GetTagDefs()->SetNum("FIXTURE", static_cast<INT64>(GetUID()));

		if ( pItem->IsType(IT_DOOR) )
		{
			// House doors are locked by default
			pItem->SetType(IT_DOOR_LOCKED);
		}
		else if ( pItem->IsType(IT_TELEPAD) )
		{
			// Link telepads to each other
			for ( ComponentsContainer::iterator j = i + 1; j != i; ++j )
			{
				if ( j == m_designMain.m_vectorComponents.end() )
					j = m_designMain.m_vectorComponents.begin();

				if ( (*j)->m_item.GetDispID() != pComponent->m_item.GetDispID() )
					continue;

				pItem->m_itTelepad.m_ptMark = GetComponentPoint(*j);
				break;
			}
		}

		pItem->MoveToUpdate(pt);
	}

	rect.OffsetRect(GetTopPoint().m_x, GetTopPoint().m_y);
	if ( m_pRegion && !m_pRegion->IsInside(rect) )
	{
		// Items outside the region won't be noticed in LOS/movement checks, so the region boundaries need to be stretched to fit all components
		g_Log.EventWarn("Char UID=0%" FMTDWORDH " is trying to commit an invalid design on multi UID=0%" FMTDWORDH " using house customization menu\n", pChar ? static_cast<DWORD>(pChar->GetUID()) : 0, static_cast<DWORD>(GetUID()));

		const CGRect rectBackground = m_pRegion->GetRegionRect(0);
		rect.UnionRect(rectBackground);

		m_pRegion->SetRegionRect(rect);
		m_pRegion->UnRealizeRegion();
		m_pRegion->RealizeRegion();
	}

	++m_designMain.m_dwRevision;
	m_designWorking.m_dwRevision = m_designMain.m_dwRevision;

	if ( m_pGrayMulti )
	{
		// Multi object needs to be recreated
		delete m_pGrayMulti;
		m_pGrayMulti = NULL;
	}

	EndCustomize();
	Update();
}

void CItemMultiCustom::SendVersionTo(CClient *pClient)
{
	ADDTOCALLSTACK("CItemMultiCustom::SendVersionTo");
	// Send revision number of this building
	if ( !pClient || pClient->IsPriv(PRIV_DEBUG) )
		return;

	new PacketHouseDesignVersion(pClient, this);
}

void CItemMultiCustom::SendStructureTo(CClient *pClient)
{
	ADDTOCALLSTACK("CItemMultiCustom::SendStructureTo");
	// Send design details of this building
	if ( !pClient || pClient->IsPriv(PRIV_DEBUG) )
		return;
	if ( !PacketHouseDesign::CanSendTo(pClient->m_NetState) )
		return;

	// Check if packet is already cached
	DesignDetails *pDesign = (pClient == m_pArchitect) ? &m_designWorking : &m_designMain;
	if ( pDesign->m_pPacket )
	{
		// Check if cached packet revision matches design revision
		if ( pDesign->m_dwPacketRevision == pDesign->m_dwRevision )
		{
			pDesign->m_pPacket->send(pClient);
			return;
		}

		// Delete cached packet and build a new one
		delete pDesign->m_pPacket;
		pDesign->m_pPacket = NULL;
		pDesign->m_dwPacketRevision = 0;
	}

	PacketHouseDesign *pPacket = new PacketHouseDesign(this, pDesign->m_dwRevision);
	if ( pDesign->m_vectorComponents.size() )
	{
		const CGRect rect = GetDesignArea();
		int iWidth = rect.GetWidth();
		int iHeight = rect.GetHeight();

		ComponentsContainer vectorStairs;
		Component *pComponent;
		CItemBase *pItemBase;

		signed short iMaxZ = 0;
		for ( ComponentsContainer::iterator i = pDesign->m_vectorComponents.begin(); i != pDesign->m_vectorComponents.end(); ++i )
		{
			if ( (*i)->m_item.m_dz > iMaxZ )
				iMaxZ = (*i)->m_item.m_dz;
		}
		BYTE bMaxLevel = GetLevel(iMaxZ);

		// Generate a list of items for each level
		NWORD nwLevelBuffer[HOUSEDESIGN_LEVELDATA_BUFFER];
		for ( BYTE bLevel = 0; bLevel <= bMaxLevel; ++bLevel )
		{
			WORD wItemCount = 0;
			int iMaxIndex = 0;

			memset(nwLevelBuffer, 0, sizeof(nwLevelBuffer));
			for ( ComponentsContainer::iterator i = pDesign->m_vectorComponents.begin(); i != pDesign->m_vectorComponents.end(); ++i )
			{
				pComponent = *i;
				if ( GetLevel(pComponent->m_item.m_dz) != bLevel )
					continue;
				if ( !pComponent->m_item.m_visible && (pDesign != &m_designWorking) )
					continue;

				pItemBase = CItemBase::FindItemBase(pComponent->m_item.GetDispID());
				if ( !pItemBase )
					continue;

				CPointMap pt = GetComponentPoint(pComponent);
				int x = (pt.m_x - 1) - rect.m_left;
				int y = (pt.m_y - 1) - rect.m_top;

				int index;
				if ( bLevel == 0 )
					index = ((x + 1) * (iHeight + 1)) + (y + 1);
				else
					index = (x * (iHeight - 1)) + y;

				// Place on stairs list when item:
				// - is not placed on level ground Z
				// - is heightless
				// - is floor
				// - is outside building area
				// - is outside buffer bounds
				if ( (GetLevelZ(GetLevel(pComponent->m_item.m_dz)) != pComponent->m_item.m_dz) || (pItemBase->GetHeight() == 0) || pComponent->m_fFloor || ((x < 0) || (y < 0) || (x >= iWidth) || (y >= iHeight)) || ((index < 0) || (index >= HOUSEDESIGN_LEVELDATA_BUFFER)) )
				{
					vectorStairs.push_back(pComponent);
					continue;
				}

				if ( index > iMaxIndex )
					iMaxIndex = index;

				++wItemCount;
				nwLevelBuffer[index] = static_cast<WORD>(pComponent->m_item.GetDispID());
			}

			if ( wItemCount == 0 )
				continue;

			pPacket->writeLevelData(bLevel, wItemCount, reinterpret_cast<BYTE *>(nwLevelBuffer), (iMaxIndex + 1) * sizeof(DWORD));
		}

		for ( ComponentsContainer::iterator i = vectorStairs.begin(); i != vectorStairs.end(); ++i )
		{
			// Stair items can be sent in any order
			pComponent = *i;
			pPacket->writeStairData(pComponent->m_item.GetDispID(), static_cast<BYTE>(pComponent->m_item.m_dx), static_cast<BYTE>(pComponent->m_item.m_dy), static_cast<BYTE>(pComponent->m_item.m_dz));
		}
	}
	pPacket->finalize();

	pDesign->m_pPacket = pPacket;
	pDesign->m_dwPacketRevision = pDesign->m_dwRevision;
	pDesign->m_pPacket->send(pClient);
}

DWORD CItemMultiCustom::GetRevision(const CClient *pClient) const
{
	ADDTOCALLSTACK("CItemMultiCustom::GetRevision");
	// Return design revision
	return (pClient == m_pArchitect) ? m_designWorking.m_dwRevision : m_designMain.m_dwRevision;
}

size_t CItemMultiCustom::GetFixtureCount(DesignDetails *pDesign)
{
	ADDTOCALLSTACK("CItemMultiCustom::GetFixtureCount");

	size_t iCount = 0;
	for ( ComponentsContainer::iterator i = pDesign->m_vectorComponents.begin(); i != pDesign->m_vectorComponents.end(); ++i )
	{
		if ( !(*i)->m_item.m_visible )
			++iCount;
	}
	return iCount;
}

size_t CItemMultiCustom::GetComponentsAt(signed short x, signed short y, signed char z, Component **ppComponentList, DesignDetails *pDesign)
{
	ADDTOCALLSTACK("CItemMultiCustom::GetComponentsAt");
	// Fill an array with components found at given location and return array length

	Component *pComponent;
	size_t iCount = 0;
	for ( size_t i = 0; i < pDesign->m_vectorComponents.size(); ++i )
	{
		pComponent = pDesign->m_vectorComponents.at(i);
		if ( (pComponent->m_item.m_dx != x) || (pComponent->m_item.m_dy != y) )
			continue;
		if ( (z != SCHAR_MIN) && (GetLevel(z) != GetLevel(pComponent->m_item.m_dz)) )
			continue;

		ppComponentList[iCount++] = pComponent;
	}
	return iCount;
}

const CPointMap CItemMultiCustom::GetComponentPoint(Component *pComponent) const
{
	ADDTOCALLSTACK("CItemMultiCustom::GetComponentPoint");
	CPointMap pt = GetTopPoint();
	pt.m_x += pComponent->m_item.m_dx;
	pt.m_y += pComponent->m_item.m_dy;
	pt.m_z += static_cast<signed char>(pComponent->m_item.m_dz);
	return pt;
}

const CItemMultiCustom::CGrayMultiCustom *CItemMultiCustom::GetMultiItemDefs()
{
	ADDTOCALLSTACK("CItemMultiCustom::GetMultiItemDefs");
	// Get CGrayMultiCustom object that represents the components in the main design
	if ( !m_pGrayMulti )
	{
		m_pGrayMulti = new CGrayMultiCustom;
		m_pGrayMulti->LoadFrom(&m_designMain);
	}
	return m_pGrayMulti;
}

const CGRect CItemMultiCustom::GetDesignArea()
{
	ADDTOCALLSTACK("CItemMultiCustom::GetDesignArea");
	// Get foundation dimensions (which is the client editable area)

	if ( m_rectDesignArea.IsRectEmpty() )
	{
		m_rectDesignArea.SetRect(0, 0, 1, 1, GetTopMap());

		const CGrayMulti *pMulti = g_Cfg.GetMultiItemDefs(GetID());
		if ( pMulti )
		{
			// Client uses the multi items to determine editable area
			size_t iCount = pMulti->GetItemCount();
			for ( size_t i = 0; i < iCount; ++i )
			{
				const CUOMultiItemRecHS *pMultiItem = pMulti->GetItem(i);
				if ( !pMultiItem || !pMultiItem->m_visible )
					continue;

				m_rectDesignArea.UnionPoint(pMultiItem->m_dx, pMultiItem->m_dy);
			}
		}
		else
		{
			// Multi data is not available, so assume the background region boundaries are correct
			const CGRect rectBackground = m_pRegion->GetRegionRect(0);
			m_rectDesignArea.SetRect(rectBackground.m_left, rectBackground.m_top, rectBackground.m_right, rectBackground.m_top, rectBackground.m_map);

			const CPointMap pt = GetTopPoint();
			m_rectDesignArea.OffsetRect(-pt.m_x, -pt.m_y);

			g_Log.EventWarn("Multi UID=0%" FMTDWORDH " doesn't have an proper MULTIDEF set\n", static_cast<DWORD>(GetUID()));
		}
	}

	CGRect rect;
	const CPointMap pt = GetTopPoint();
	rect.SetRect(m_rectDesignArea.m_left, m_rectDesignArea.m_top, m_rectDesignArea.m_right, m_rectDesignArea.m_bottom, GetTopMap());
	rect.OffsetRect(pt.m_x, pt.m_y);
	return rect;
}

void CItemMultiCustom::CopyDesign(DesignDetails *pDesignFrom, DesignDetails *pDesignTo)
{
	ADDTOCALLSTACK("CItemMultiCustom::CopyComponents");
	// Overwrite design details with the details of another one

	pDesignTo->m_dwRevision = pDesignFrom->m_dwRevision;

	pDesignTo->m_vectorComponents.clear();
	Component *pComponent;
	for ( ComponentsContainer::iterator i = pDesignFrom->m_vectorComponents.begin(); i != pDesignFrom->m_vectorComponents.end(); ++i )
	{
		pComponent = new Component;
		*pComponent = **i;
		pDesignTo->m_vectorComponents.push_back(pComponent);
	}

	delete pDesignTo->m_pPacket;
	if ( pDesignFrom->m_pPacket )
	{
		pDesignTo->m_pPacket = new PacketHouseDesign(pDesignFrom->m_pPacket);
		pDesignTo->m_dwPacketRevision = pDesignFrom->m_dwPacketRevision;
	}
	else
	{
		pDesignTo->m_pPacket = NULL;
		pDesignTo->m_dwPacketRevision = 0;
	}
}

enum IMCV_TYPE
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

LPCTSTR const CItemMultiCustom::sm_szVerbKeys[IMCV_QTY + 1] =
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

bool CItemMultiCustom::r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef)
{
	ADDTOCALLSTACK("CItemMultiCustom::r_GetRef");
	if ( !strcmpi("DESIGNER.", pszKey) )
	{
		pszKey += 9;
		pRef = m_pArchitect ? m_pArchitect->GetChar() : NULL;
		return true;
	}
	return CItemMulti::r_GetRef(pszKey, pRef);
}

bool CItemMultiCustom::r_Verb(CScript &s, CTextConsole *pSrc)	// execute command from script
{
	ADDTOCALLSTACK("CItemMultiCustom::r_Verb");
	EXC_TRY("Verb");

	int index = FindTableSorted(s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1);
	if ( index < 0 )
		return CItemMulti::r_Verb(s, pSrc);

	switch ( index )
	{
		case IMCV_ADDITEM:
		{
			TCHAR *ppArgs[4];
			size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), ",");
			if ( iQty != 4 )
				return false;

			AddItem(NULL, static_cast<ITEMID_TYPE>(Exp_GetVal(ppArgs[0])), static_cast<signed short>(Exp_GetVal(ppArgs[1])), static_cast<signed short>(Exp_GetVal(ppArgs[2])), static_cast<signed char>(Exp_GetVal(ppArgs[3])));
			break;
		}
		case IMCV_ADDMULTI:
		{
			TCHAR *ppArgs[4];
			size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), ",");
			if ( iQty != 4 )
				return false;

			ITEMID_TYPE id = static_cast<ITEMID_TYPE>(Exp_GetVal(ppArgs[0]));
			AddStairs(NULL, id, static_cast<signed short>(Exp_GetVal(ppArgs[1])), static_cast<signed short>(Exp_GetVal(ppArgs[2])), static_cast<signed char>(Exp_GetVal(ppArgs[3])));
			break;
		}
		case IMCV_CLEAR:
		{
			ResetStructure();
			break;
		}
		case IMCV_COMMIT:
		{
			CommitChanges();
			break;
		}
		case IMCV_CUSTOMIZE:
		{
			const CChar *pChar = s.HasArgs() ? static_cast<CGrayUID>(s.GetArgVal()).CharFind() : (pSrc ? pSrc->GetChar() : NULL);
			if ( !pChar || !pChar->m_pClient )
				return false;

			BeginCustomize(pChar->m_pClient);
			break;
		}
		case IMCV_ENDCUSTOMIZE:
		{
			EndCustomize(true);
			break;
		}
		case IMCV_REMOVEITEM:
		{
			TCHAR *ppArgs[4];
			size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), ",");
			if ( iQty != 4 )
				return false;

			RemoveItem(NULL, static_cast<ITEMID_TYPE>(Exp_GetVal(ppArgs[0])), static_cast<signed short>(Exp_GetVal(ppArgs[1])), static_cast<signed short>(Exp_GetVal(ppArgs[2])), static_cast<signed char>(Exp_GetVal(ppArgs[3])));
			break;
		}
		case IMCV_RESET:
		{
			ResetStructure();
			break;
		}
		case IMCV_RESYNC:
		{
			const CChar *pChar = s.HasArgs() ? static_cast<CGrayUID>(s.GetArgVal()).CharFind() : (pSrc ? pSrc->GetChar() : NULL);
			if ( !pChar || !pChar->m_pClient )
				return false;

			SendStructureTo(pChar->m_pClient);
			break;
		}
		case IMCV_REVERT:
		{
			RevertChanges();
			break;
		}
		default:
		{
			return CItemMulti::r_Verb(s, pSrc);
		}
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return true;
}

void CItemMultiCustom::r_Write(CScript &s)
{
	ADDTOCALLSTACK_INTENSIVE("CItemMultiCustom::r_Write");
	CItemMulti::r_Write(s);

	Component *pComponent;
	for ( ComponentsContainer::iterator i = m_designMain.m_vectorComponents.begin(); i != m_designMain.m_vectorComponents.end(); ++i )
	{
		pComponent = *i;
		s.WriteKeyFormat("COMP", "%d,%hd,%hd,%hd,%d", pComponent->m_item.GetDispID(), pComponent->m_item.m_dx, pComponent->m_item.m_dy, pComponent->m_item.m_dz, pComponent->m_iStairID);
	}

	if ( m_designMain.m_dwRevision )
		s.WriteKeyVal("REVISION", m_designMain.m_dwRevision);
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

LPCTSTR const CItemMultiCustom::sm_szLoadKeys[IMCC_QTY + 1] =	// static
{
	"COMPONENTS",
	"DESIGN",
	"DESIGNER",
	"EDITAREA",
	"FIXTURES",
	"REVISION",
	NULL
};

bool CItemMultiCustom::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CItemMultiCustom::r_WriteVal");
	EXC_TRY("WriteVal");

	int index = FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	if ( (index < 0) && !strnicmp(pszKey, "DESIGN.", 7) )
		index = IMCC_DESIGN;

	switch ( index )
	{
		case IMCC_COMPONENTS:
		{
			pszKey += 10;
			sVal.FormatUVal(m_designMain.m_vectorComponents.size());
			break;
		}
		case IMCC_DESIGN:
		{
			pszKey += 6;
			if ( !*pszKey )
				sVal.FormatUVal(m_designMain.m_vectorComponents.size());
			else if ( *pszKey == '.' )
			{
				SKIP_SEPARATORS(pszKey);
				size_t iQty = static_cast<size_t>(Exp_GetVal(pszKey));
				if ( iQty >= m_designMain.m_vectorComponents.size() )
					return false;

				SKIP_SEPARATORS(pszKey);
				CUOMultiItemRecHS item = m_designMain.m_vectorComponents.at(iQty)->m_item;

				if ( !strcmpi(pszKey, "ID") )
					sVal.FormatVal(item.GetDispID());
				else if ( !strcmpi(pszKey, "DX") )
					sVal.FormatVal(item.m_dx);
				else if ( !strcmpi(pszKey, "DY") )
					sVal.FormatVal(item.m_dy);
				else if ( !strcmpi(pszKey, "DZ") )
					sVal.FormatVal(item.m_dz);
				else if ( !strcmpi(pszKey, "D") )
					sVal.Format("%hd,%hd,%hd", item.m_dx, item.m_dy, item.m_dz);
				else if ( !strcmpi(pszKey, "FIXTURE") )
					sVal.FormatVal(item.m_visible ? 0 : 1);
				else if ( !*pszKey )
					sVal.Format("%d,%hd,%hd,%hd", item.GetDispID(), item.m_dx, item.m_dy, item.m_dz);
				else
					return false;
			}
			else
				return false;
			break;
		}
		case IMCC_DESIGNER:
		{
			pszKey += 8;
			const CChar *pChar = m_pArchitect ? m_pArchitect->GetChar() : NULL;
			sVal.FormatHex(pChar ? static_cast<DWORD>(pChar->GetUID()) : 0);
			break;
		}
		case IMCC_EDITAREA:
		{
			pszKey += 8;
			const CGRect rect = GetDesignArea();
			sVal.Format("%d,%d,%d,%d", rect.m_left, rect.m_top, rect.m_right, rect.m_bottom);
			break;
		}
		case IMCC_FIXTURES:
		{
			pszKey += 8;
			sVal.FormatUVal(GetFixtureCount(&m_designMain));
			break;
		}
		case IMCC_REVISION:
		{
			pszKey += 8;
			sVal.FormatUVal(m_designMain.m_dwRevision);
			break;
		}
		default:
		{
			return CItemMulti::r_WriteVal(pszKey, sVal, pSrc);
		}
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CItemMultiCustom::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CItemMultiCustom::r_LoadVal");
	EXC_TRY("LoadVal");

	if ( g_Serv.IsLoading() )
	{
		if ( s.IsKey("COMP") )
		{
			TCHAR *ppArgs[5];
			size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), ",");
			if ( iQty != 5 )
				return false;

			AddItem(NULL, static_cast<ITEMID_TYPE>(ATOI(ppArgs[0])), static_cast<signed short>(ATOI(ppArgs[1])), static_cast<signed short>(ATOI(ppArgs[2])), static_cast<signed char>(ATOI(ppArgs[3])), static_cast<short>(ATOI(ppArgs[4])));
			return true;
		}
		else if ( s.IsKey("REVISION") )
		{
			m_designWorking.m_dwRevision = s.GetArgVal();
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

BYTE CItemMultiCustom::GetLevel(signed short z)
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

signed char CItemMultiCustom::GetLevelZ(BYTE bLevel)
{
	return 7 + ((bLevel - 1) * 20);
}

bool CItemMultiCustom::IsValidItem(ITEMID_TYPE id, CClient *pClient, bool fMulti)
{
	ADDTOCALLSTACK("CItemMultiCustom::IsValidItem");
	if ( !fMulti && ((id <= ITEMID_NOTHING) || (id >= ITEMID_QTY)) )
		return false;
	if ( fMulti && ((id < ITEMID_MULTI) || (id > ITEMID_MULTI_MAX)) )
		return false;

	// Scripts and GM clients can add any item
	if ( !pClient || pClient->IsPriv(PRIV_GM) )
		return true;

	// Load items database
	if ( !LoadValidItems() )
		return false;

	// Check if item exists in the database
	ValidItemsContainer::iterator it = sm_mapValidItems.find(id);
	if ( it == sm_mapValidItems.end() )
		return false;

	// Check if item is enabled on client feature flags
	if ( (static_cast<int>(pClient->m_FeatureFlags) & it->second) != it->second )
		return false;

	return true;
}

CItemMultiCustom::ValidItemsContainer CItemMultiCustom::sm_mapValidItems;

bool CItemMultiCustom::LoadValidItems()
{
	ADDTOCALLSTACK("CItemMultiCustom::LoadValidItems");
	if ( !sm_mapValidItems.empty() )	// already loaded
		return true;

	static const char *sm_szItemFiles[][32] = {
		// List of files containing valid items
		{ "doors.txt", "Piece1", "Piece2", "Piece3", "Piece4", "Piece5", "Piece6", "Piece7", "Piece8", NULL },
		{ "misc.txt", "Piece1", "Piece2", "Piece3", "Piece4", "Piece5", "Piece6", "Piece7", "Piece8", NULL },
		{ "floors.txt", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16", NULL },
		{ "teleprts.txt", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16", NULL },
		{ "roof.txt", "North", "East", "South", "West", "NSCrosspiece", "EWCrosspiece", "NDent", "EDent", "SDent", "WDent", "NTPiece", "ETPiece", "STPiece", "WTPiece", "XPiece", "Extra Piece", NULL },
		{ "walls.txt", "South1", "South2", "South3", "Corner", "East1", "East2", "East3", "Post", "WindowS", "AltWindowS", "WindowE", "AltWindowE", "SecondAltWindowS", "SecondAltWindowE", NULL },
		{ "stairs.txt", "Block", "North", "East", "South", "West", "Squared1", "Squared2", "Rounded1", "Rounded2", NULL },
		{ NULL },
		// List of files containing valid multis
		{ "stairs.txt", "MultiNorth", "MultiEast", "MultiSouth", "MultiWest", NULL },
		{ NULL }
	};

	CSVRowData csvDataRow;
	bool fMultiFile = false;
	int iFileIndex = 0;
	int i = 0;

	EXC_TRY("LoadCSVFiles");

	for ( i = 0; sm_szItemFiles[i][0] || !fMultiFile; ++i, ++iFileIndex )
	{
		if ( !sm_szItemFiles[i][0] )
		{
			fMultiFile = true;
			--iFileIndex;
			continue;
		}

		if ( !g_Install.m_CsvFiles[iFileIndex].IsFileOpen() && !g_Install.OpenFile(g_Install.m_CsvFiles[iFileIndex], sm_szItemFiles[i][0], OF_READ|OF_SHARE_DENY_WRITE) )
			continue;

		while ( g_Install.m_CsvFiles[iFileIndex].ReadNextRowContent(csvDataRow) )
		{
			for ( int j = 1; sm_szItemFiles[i][j] != NULL; ++j )
			{
				ITEMID_TYPE id = static_cast<ITEMID_TYPE>(ATOI(csvDataRow[sm_szItemFiles[i][j]].c_str()));
				if ( (id <= ITEMID_NOTHING) || (id >= ITEMID_QTY) )
					continue;

				if ( fMultiFile )
				{
					id = static_cast<ITEMID_TYPE>(ITEMID_MULTI + id);
					if ( (id < ITEMID_MULTI) || (id > ITEMID_MULTI_MAX) )
						continue;
				}

				sm_mapValidItems[id] = ATOI(csvDataRow["FeatureMask"].c_str());
			}
		}

		g_Install.m_CsvFiles[iFileIndex].Close();
	}

	// Make sure the list have at least 1 item
	sm_mapValidItems[ITEMID_NOTHING] = UINT_MAX;
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("file index '%d\n", iFileIndex);
	g_Log.EventDebug("file name '%s'\n", sm_szItemFiles[i][0]);

	TCHAR *pszHeaderFull = Str_GetTemp();
	TCHAR *pszRowFull = Str_GetTemp();
	for ( CSVRowData::iterator itCsv = csvDataRow.begin(); itCsv != csvDataRow.end(); ++itCsv )
	{
		strcat(pszHeaderFull, "\t");
		strcat(pszHeaderFull, itCsv->first.c_str());

		strcat(pszRowFull, "\t");
		strcat(pszRowFull, itCsv->second.c_str());
	}

	g_Log.EventDebug("header count '%" FMTSIZE_T "', header text '%s'\n", csvDataRow.size(), pszHeaderFull);
	g_Log.EventDebug("column count '%" FMTSIZE_T "', row text '%s'\n", csvDataRow.size(), pszRowFull);
	EXC_DEBUG_END;
	return false;
}

void CItemMultiCustom::CGrayMultiCustom::LoadFrom(CItemMultiCustom::DesignDetails *pDesign)
{
	ADDTOCALLSTACK("CGrayMultiCustom::LoadFrom");
	m_iItemQty = pDesign->m_vectorComponents.size();
	m_pItems = new CUOMultiItemRecHS[m_iItemQty];
	for ( size_t i = 0; i < m_iItemQty; ++i )
		memcpy(&m_pItems[i], &pDesign->m_vectorComponents.at(i)->m_item, sizeof(CUOMultiItemRecHS));
}
