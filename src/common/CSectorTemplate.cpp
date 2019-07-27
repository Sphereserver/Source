#include "../graysvr/graysvr.h"

///////////////////////////////////////////////////////////
// CCharsActiveList

CCharsActiveList::CCharsActiveList()
{
	m_iClients = 0;
	m_timeLastClient.Init();
}

void CCharsActiveList::OnRemoveOb(CGObListRec *pObRec)
{
	ADDTOCALLSTACK("CCharsActiveList::OnRemoveOb");
	// Override this = called when removed from group
	CChar *pChar = static_cast<CChar *>(pObRec);
	ASSERT(pChar);
	if ( pChar->m_pClient )
	{
		ClientDetach();
		m_timeLastClient = CServTime::GetCurrentTime();	// mark time in case it's the last client
	}
	CGObList::OnRemoveOb(pObRec);
	pChar->SetContainerFlags(UID_O_DISCONNECT);
}

void CCharsActiveList::AddCharToSector(CChar *pChar)
{
	ADDTOCALLSTACK("CCharsActiveList::AddCharToSector");
	ASSERT(pChar);
	if ( pChar->m_pClient )
		ClientAttach();
	CGObList::InsertHead(pChar);
}

void CCharsActiveList::ClientAttach()
{
	ADDTOCALLSTACK("CCharsActiveList::ClientAttach");
	++m_iClients;
}

void CCharsActiveList::ClientDetach()
{
	ADDTOCALLSTACK("CCharsActiveList::ClientDetach");
	--m_iClients;
}

///////////////////////////////////////////////////////////
// CItemsList

bool CItemsList::sm_fNotAMove = false;

void CItemsList::OnRemoveOb(CGObListRec *pObRec)
{
	ADDTOCALLSTACK("CItemsList::OnRemoveOb");
	// Item is picked up off the ground (may be put right back down though)
	CItem *pItem = static_cast<CItem *>(pObRec);
	ASSERT(pItem);

	if ( !sm_fNotAMove )
		pItem->OnMoveFrom();	// IT_MULTI, IT_SHIP and IT_COMM_CRYSTAL

	CGObList::OnRemoveOb(pObRec);
	pItem->SetContainerFlags(UID_O_DISCONNECT);		// it is no place for the moment
}

void CItemsList::AddItemToSector(CItem *pItem)
{
	ADDTOCALLSTACK("CItemsList::AddItemToSector");
	// Add to top level
	// Either MoveTo() or SetTimeout() is being called
	ASSERT(pItem);
	CGObList::InsertHead(pItem);
}

///////////////////////////////////////////////////////////
// CSectorBase

CSectorBase::CSectorBase()
{
	m_index = 0;
	m_map = 0;
	m_bFlags = 0;
}

CSectorBase::~CSectorBase()
{
	ClearMapBlockCache();
}

void CSectorBase::Init(int index, int newmap)
{
	ADDTOCALLSTACK("CSectorBase::Init");
	if ( (newmap < 0) || (newmap >= 256) || !g_MapList.m_maps[newmap] )
	{
		g_Log.EventError("Trying to initalize a sector %d in unsupported map #%d. Defaulting to 0,0\n", index, newmap);
	}
	else if ( (index < 0) || (index >= g_MapList.GetSectorQty(newmap)) )
	{
		m_map = newmap;
		g_Log.EventError("Trying to initalize a sector by sector number %d out-of-range for map #%d. Defaulting to 0,%d\n", index, newmap, newmap);
	}
	else
	{
		m_index = index;
		m_map = newmap;
	}
}

bool CSectorBase::CheckMapBlockTime(const MapBlockCache::value_type &elem)	// static
{
	ADDTOCALLSTACK("CSectorBase::CheckMapBlockTime");
	return (elem.second->m_CacheTime.GetCacheAge() > m_iMapBlockCacheTime);
}

void CSectorBase::ClearMapBlockCache()
{
	ADDTOCALLSTACK("CSectorBase::ClearMapBlockCache");

	for ( MapBlockCache::iterator it = m_MapBlockCache.begin(); it != m_MapBlockCache.end(); ++it )
		delete it->second;

	m_MapBlockCache.clear();
}

void CSectorBase::CheckMapBlockCache()
{
	ADDTOCALLSTACK("CSectorBase::CheckMapBlockCache");
	// Clean out the sectors map cache if it has not been used recently
	if ( m_MapBlockCache.empty() )
		return;

	MapBlockCache::iterator it;
	for (;;)
	{
		it = find_if(m_MapBlockCache.begin(), m_MapBlockCache.end(), CheckMapBlockTime);
		if ( it == m_MapBlockCache.end() )
			break;

		if ( (m_iMapBlockCacheTime <= 0) || (it->second->m_CacheTime.GetCacheAge() >= m_iMapBlockCacheTime) )
		{
			delete it->second;
			m_MapBlockCache.erase(it);
		}
	}
}

const CGrayMapBlock *CSectorBase::GetMapBlock(const CPointMap &pt)
{
	ADDTOCALLSTACK("CSectorBase::GetMapBlock");
	// Get a map block from cache
	ASSERT(pt.IsValidXY());
	CPointMap ptBlock(UO_BLOCK_ALIGN(pt.m_x), UO_BLOCK_ALIGN(pt.m_y), 0, pt.m_map);
	ASSERT(m_MapBlockCache.size() <= (UO_BLOCK_SIZE * UO_BLOCK_SIZE));

	ProfileTask mapTask(PROFILE_MAP);

	if ( !pt.IsValidXY() )
	{
		g_Log.EventWarn("Attempting to access invalid memory block at %s\n", pt.WriteUsed());
		return NULL;
	}

	CGrayMapBlock *pMapBlock;

	// Try to find it in cache
	long lBlock = ptBlock.GetPointSortIndex();
	MapBlockCache::iterator it = m_MapBlockCache.find(lBlock);
	if ( it != m_MapBlockCache.end() )
	{
		it->second->m_CacheTime.HitCacheTime();
		return it->second;
	}

	// Else load it
	try
	{
		pMapBlock = new CGrayMapBlock(ptBlock);
		ASSERT(pMapBlock);
	}
	catch ( const CGrayError &e )
	{
		g_Log.EventError("Exception creating new memory block at %s (%s)\n", ptBlock.WriteUsed(), e.m_pszDescription);
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		return NULL;
	}
	catch ( ... )
	{
		g_Log.EventError("Exception creating new memory block at %s\n", ptBlock.WriteUsed());
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		return NULL;
	}

	// Add it on cache
	m_MapBlockCache[lBlock] = pMapBlock;
	return pMapBlock;
}

bool CSectorBase::IsInDungeon() const
{
	ADDTOCALLSTACK("CSectorBase::IsInDungeon");
	CPointMap pt = GetBasePoint();
	CRegionBase *pRegion = GetRegion(pt, REGION_TYPE_AREA);

	return (pRegion && pRegion->IsFlag(REGION_FLAG_UNDERGROUND));
}

CRegionBase *CSectorBase::GetRegion(const CPointBase &pt, BYTE bType) const
{
	ADDTOCALLSTACK("CSectorBase::GetRegion");
	// Does it match the mask of types we care about?
	// Assume sorted so that the smallest are first
	//
	// REGION_TYPE_AREA => RES_AREA = World region area only (CRegionWorld)
	// REGION_TYPE_ROOM => RES_ROOM = NPC House areas only (CRegionBase)
	// REGION_TYPE_MULTI => RES_WORLDITEM = UID linked types in general (CRegionWorld)

	size_t iQty = m_RegionLinks.GetCount();
	for ( size_t i = 0; i < iQty; ++i )
	{
		CRegionBase *pRegion = m_RegionLinks[i];
		ASSERT(pRegion);

		ASSERT(pRegion->GetResourceID().IsValidUID());
		if ( pRegion->GetResourceID().IsItem() )
		{
			CItemShip *pShipItem = dynamic_cast<CItemShip *>(pRegion->GetResourceID().ItemFind());
			if ( pShipItem )
			{
				if ( !(bType & REGION_TYPE_SHIP) )
					continue;
			}
			else if ( !(bType & REGION_TYPE_HOUSE) )
				continue;
		}
		else if ( pRegion->GetResourceID().GetResType() == RES_AREA )
		{
			if ( !(bType & REGION_TYPE_AREA) )
				continue;
		}
		else
		{
			if ( !(bType & REGION_TYPE_ROOM) )
				continue;
		}

		if ( pRegion->m_pt.m_map != pt.m_map )
			continue;
		if ( !pRegion->IsInside2d(pt) )
			continue;
		return pRegion;
	}
	return NULL;
}

size_t CSectorBase::GetRegions(const CPointBase &pt, BYTE bType, CRegionLinks &rlinks) const
{
	ADDTOCALLSTACK("CSectorBase::GetRegions");
	// Get regions list (to cicle through intercepted house regions)

	size_t iQty = m_RegionLinks.GetCount();
	for ( size_t i = 0; i < iQty; ++i )
	{
		CRegionBase *pRegion = m_RegionLinks[i];
		ASSERT(pRegion);

		ASSERT(pRegion->GetResourceID().IsValidUID());
		if ( pRegion->GetResourceID().IsItem() )
		{
			CItemShip *pShipItem = dynamic_cast<CItemShip *>(pRegion->GetResourceID().ItemFind());
			if ( pShipItem )
			{
				if ( !(bType & REGION_TYPE_SHIP) )
					continue;
			}
			else if ( !(bType & REGION_TYPE_HOUSE) )
				continue;
		}
		else if ( pRegion->GetResourceID().GetResType() == RES_AREA )
		{
			if ( !(bType & REGION_TYPE_AREA) )
				continue;
		}
		else
		{
			if ( !(bType & REGION_TYPE_ROOM) )
				continue;
		}

		if ( pRegion->m_pt.m_map != pt.m_map )
			continue;
		if ( !pRegion->IsInside2d(pt) )
			continue;
		rlinks.Add(pRegion);
	}
	return rlinks.GetCount();
}

bool CSectorBase::UnLinkRegion(CRegionBase *pRegionOld)
{
	ADDTOCALLSTACK("CSectorBase::UnLinkRegion");
	if ( !pRegionOld )
		return false;
	return m_RegionLinks.RemovePtr(pRegionOld);
}

bool CSectorBase::LinkRegion(CRegionBase *pRegionNew)
{
	ADDTOCALLSTACK("CSectorBase::LinkRegion");
	// Link in a region. May have just moved
	// Make sure the smaller regions are first in the array
	// Later added regions from the MAP file should be the smaller ones, according to the old rules
	ASSERT(pRegionNew);
	ASSERT(pRegionNew->IsOverlapped(GetRect()));

	size_t iQty = m_RegionLinks.GetCount();
	for ( size_t i = 0; i < iQty; ++i )
	{
		CRegionBase *pRegion = m_RegionLinks[i];
		ASSERT(pRegion);
		if ( pRegionNew == pRegion )
		{
			DEBUG_ERR(("Region already linked\n"));
			return false;
		}

		if ( pRegion->IsOverlapped(pRegionNew) )
		{
			// NOTE: We should use IsInside() but my version isn't completely accurate for it's FALSE return
			if ( pRegion->IsEqualRegion(pRegionNew) )
			{
				DEBUG_ERR(("Conflicting region\n"));
				return false;
			}

			// It is accurate in the TRUE case
			if ( pRegionNew->IsInside(pRegion) )
				continue;

			// Keep item (multi) regions on top
			if ( pRegion->GetResourceID().IsItem() && !pRegionNew->GetResourceID().IsItem() )
				continue;

			// Must insert before this
			m_RegionLinks.InsertAt(i, pRegionNew);
			return true;
		}
	}

	m_RegionLinks.Add(pRegionNew);
	return true;
}

CTeleport *CSectorBase::GetTeleport(const CPointMap &pt) const
{
	ADDTOCALLSTACK("CSectorBase::GetTeleport");
	// Check if there's any teleport on this point

	size_t i = m_Teleports.FindKey(pt.GetPointSortIndex());
	if ( i != m_Teleports.BadIndex() )
	{
		CTeleport *pTeleport = static_cast<CTeleport *>(m_Teleports[i]);
		if ( (pTeleport->m_map == pt.m_map) && (abs(pTeleport->m_z - pt.m_z) <= 5) )
			return pTeleport;
	}
	return NULL;
}

bool CSectorBase::AddTeleport(CTeleport *pTeleport)
{
	ADDTOCALLSTACK("CSectorBase::AddTeleport");

	size_t i = m_Teleports.FindKey(pTeleport->GetPointSortIndex());
	if ( i != m_Teleports.BadIndex() )
	{
		DEBUG_ERR(("Conflicting teleport %s\n", pTeleport->WriteUsed()));
		return false;
	}
	m_Teleports.AddSortKey(pTeleport, pTeleport->GetPointSortIndex());
	return true;
}

CPointMap CSectorBase::GetBasePoint() const
{
	ADDTOCALLSTACK("CSectorBase::GetBasePoint");
	// Get base point of the sector (upper left point)
	ASSERT((m_index >= 0) && (m_index < g_MapList.GetSectorQty(m_map)));

	int iSectorCols = maximum(1, g_MapList.GetSectorCols(m_map));
	int iSectorSize = g_MapList.GetSectorSize(m_map);
	CPointMap pt(static_cast<signed short>((m_index % iSectorCols) * iSectorSize), static_cast<signed short>((m_index / iSectorCols) * iSectorSize), 0, static_cast<BYTE>(m_map));
	return pt;
}

CRectMap CSectorBase::GetRect() const
{
	ADDTOCALLSTACK("CSectorBase::GetRect");
	// Get rectangle area of the sector
	CPointMap pt = GetBasePoint();
	CRectMap rect;
	rect.m_left = pt.m_x;
	rect.m_top = pt.m_y;
	rect.m_right = pt.m_x + g_MapList.GetSectorSize(pt.m_map);
	rect.m_bottom = pt.m_y + g_MapList.GetSectorSize(pt.m_map);
	rect.m_map = pt.m_map;
	return rect;
}
