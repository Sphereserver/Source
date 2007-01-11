//
// CSectorTemplate.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "../graysvr/graysvr.h"

////////////////////////////////////////////////////////////////////////
// -CCharsActiveList

CCharsActiveList::CCharsActiveList()
{
	m_timeLastClient.Init();
	m_iClients = 0;
}

void CCharsActiveList::OnRemoveOb( CGObListRec * pObRec )
{
	ADDTOCALLSTACK("CCharsActiveList::OnRemoveOb");
	// Override this = called when removed from group.
	CChar * pChar = STATIC_CAST <CChar*>(pObRec);
	ASSERT( pChar );
	if ( pChar->IsClient())
	{
		ClientDetach();
		m_timeLastClient = CServTime::GetCurrentTime();	// mark time in case it's the last client
	}
	CGObList::OnRemoveOb(pObRec);
	pChar->SetContainerFlags(UID_O_DISCONNECT);
}

void CCharsActiveList::AddCharToSector( CChar * pChar )
{
	ADDTOCALLSTACK("CCharsActiveList::AddCharToSector");
	ASSERT( pChar );
	// ASSERT( pChar->m_pt.IsValid());
	if ( pChar->IsClient())
	{
		ClientAttach();
	}
	CGObList::InsertHead(pChar);
}

void CCharsActiveList::ClientAttach()
{
	ADDTOCALLSTACK("CCharsActiveList::ClientAttach");
	m_iClients++;
}

void CCharsActiveList::ClientDetach()
{
	ADDTOCALLSTACK("CCharsActiveList::ClientDetach");
	m_iClients--;
}

//////////////////////////////////////////////////////////////
// -CItemList

bool CItemsList::sm_fNotAMove = false;

void CItemsList::OnRemoveOb( CGObListRec * pObRec )
{
	ADDTOCALLSTACK("CItemsList::OnRemoveOb");
	// Item is picked up off the ground. (may be put right back down though)
	CItem * pItem = STATIC_CAST <CItem*>(pObRec);
	ASSERT( pItem );

	if ( ! sm_fNotAMove )
	{
		pItem->OnMoveFrom();	// IT_MULTI, IT_SHIP and IT_COMM_CRYSTAL
	}

	CGObList::OnRemoveOb(pObRec);
	pItem->SetContainerFlags(UID_O_DISCONNECT);	// It is no place for the moment.
}

void CItemsList::AddItemToSector( CItem * pItem )
{
	ADDTOCALLSTACK("CItemsList::AddItemToSector");
	// Add to top level.
	// Either MoveTo() or SetTimeout is being called.
	ASSERT( pItem );
	CGObList::InsertHead( pItem );
}

//////////////////////////////////////////////////////////////////
// -CSectorBase

CSectorBase::CSectorBase()
{
	m_map = 0;
	m_index = 0;
}

void CSectorBase::Init(int index, int newmap)
{
	ADDTOCALLSTACK("CSectorBase::Init");
	if (( newmap < 0 ) || ( newmap >= 256 ) || !g_MapList.m_maps[newmap] )
	{
		g_Log.EventError("Trying to initalize a sector %d in unsupported map #%d. Defaulting to 0,0.\n", index, newmap);
	}
	else if (( index < 0 ) || ( index >= g_MapList.GetSectorQty(newmap) ))
	{
		m_map = newmap;
		g_Log.EventError("Trying to initalize a sector by sector number %d out-of-range for map #%d. Defaulting to 0,%d.\n", index, newmap, newmap);
	}
	else
	{
		m_index = index;
		m_map = newmap;
	}
}

int CSectorBase::GetIndex() const
{
	ADDTOCALLSTACK("CSectorBase::GetIndex");
	return m_index;
}

int CSectorBase::GetMap() const
{
	ADDTOCALLSTACK("CSectorBase::GetMap");
	return m_map;
}

void CSectorBase::CheckMapBlockCache( int iTime )
{
	ADDTOCALLSTACK("CSectorBase::CheckMapBlockCache");
	// Clean out the sectors map cache if it has not been used recently.
	// iTime == 0 = delete all.
	for ( int i = 0; i < m_MapBlockCache.GetCount(); i++ )
	{
		CGrayMapBlock * pMapBlock = NULL;
		EXC_TRY("CheckMapBlockCache");
		EXC_SET("Casting");
		//CGrayMapBlock	*pMapBlock = STATIC_CAST <CGrayMapBlock *>(m_MapBlockCache[i]);
		pMapBlock = dynamic_cast<CGrayMapBlock *>(m_MapBlockCache.GetAt(i));
		// ASSERT(pMapBlock);
		// NOTE: Experimental thing... if we can't cast it to CGrayMapBlock
		// let's remove it, it'll be reloaded if necessary.
		if ( pMapBlock == NULL )
		{
			EXC_SET("Casting failed - Deleting");
			m_MapBlockCache.DeleteAt(i);
			--i;
			continue;
		}
		EXC_SET("Getting block's cache time");
		int iCacheTime = pMapBlock->m_CacheTime.GetCacheAge();
		if (( iTime <= 0 ) || ( iCacheTime >= iTime ))
		{
			EXC_SET("CacheTime up - Deleting");
			m_MapBlockCache.DeleteAt(i);
			--i;
		}
		EXC_CATCH;

		EXC_DEBUG_START;
		CPointMap pt = GetBasePoint();
		g_Log.EventDebug("m_MapBlockCache.GetAt(%d) (0%x); pMapBlock - (0%x)\n", i,  m_MapBlockCache.GetAt(i), pMapBlock); 
		g_Log.EventDebug("check time %d, index %d/%d\n", iTime, i, m_MapBlockCache.GetCount());
		g_Log.EventDebug("sector #%d [%d,%d,%d,%d]\n", GetIndex(), pt.m_x, pt.m_y, pt.m_z, pt.m_map);
		EXC_DEBUG_END;
	}
}


const CGrayMapBlock * CSectorBase::GetMapBlock( const CPointMap & pt )
{
	ADDTOCALLSTACK("CSectorBase::GetMapBlock");
	// Get a map block from the cache. load it if not.
	ASSERT( pt.IsValidXY());
	CPointMap pntBlock( UO_BLOCK_ALIGN(pt.m_x), UO_BLOCK_ALIGN(pt.m_y), 0, pt.m_map);
	ASSERT( m_MapBlockCache.GetCount() <= (UO_BLOCK_SIZE * UO_BLOCK_SIZE));

	PROFILE_TYPE prvProfileTask = g_Serv.m_Profile.GetCurrentTask();
	g_Serv.m_Profile.Start( PROFILE_MAP );

	if ( !pt.IsValidXY() )
	{
		g_Log.EventWarn("Attempting to access invalid memory block at %s.\n", pt.WriteUsed());
		g_Serv.m_Profile.Start( prvProfileTask );
		return NULL;
	}

	CGrayMapBlock * pMapBlock;

	// Find it in cache.
	int i = m_MapBlockCache.FindKey(pntBlock.GetPointSortIndex());
	if ( i >= 0 )
	{
		pMapBlock = STATIC_CAST <CGrayMapBlock *>(m_MapBlockCache[i]);
		pMapBlock->m_CacheTime.HitCacheTime();
		g_Serv.m_Profile.Start( prvProfileTask );
		return pMapBlock;
	}

	// else load it.
	try
	{
		pMapBlock = new CGrayMapBlock(pntBlock);
		ASSERT(pMapBlock);
	}
	catch (CGrayError& e)
	{
		g_Log.EventError("Exception creating new memory block at %s. (%s)\n", pntBlock.WriteUsed(), e.m_pszDescription);
		g_Serv.m_Profile.Start( prvProfileTask );
		return NULL;
	}
	catch (...)
	{
		g_Log.EventError("Exception creating new memory block at %s.\n", pntBlock.WriteUsed());
		g_Serv.m_Profile.Start( prvProfileTask );
		return NULL;
	}

	// Add it to the cache.
	m_MapBlockCache.AddSortKey( pMapBlock, pntBlock.GetPointSortIndex() );
	g_Serv.m_Profile.Start( prvProfileTask );
	return( pMapBlock );
}

bool CSectorBase::IsInDungeon() const
{
	ADDTOCALLSTACK("CSectorBase::IsInDungeon");
	// What part of the maps are filled with dungeons.
	// Used for light / weather calcs.
	CPointMap pt = GetBasePoint();
	CRegionBase *pRegion = GetRegion(pt, REGION_TYPE_AREA);

	return ( pRegion && pRegion->IsFlag(REGION_FLAG_UNDERGROUND) );
}

CRegionBase * CSectorBase::GetRegion( const CPointBase & pt, DWORD dwType ) const
{
	ADDTOCALLSTACK("CSectorBase::GetRegion");
	// Does it match the mask of types we care about ?
	// Assume sorted so that the smallest are first.
	//
	// REGION_TYPE_AREA => RES_AREA = World region area only = CRegionWorld
	// REGION_TYPE_ROOM => RES_ROOM = NPC House areas only = CRegionBase.
	// REGION_TYPE_MULTI => RES_WORLDITEM = UID linked types in general = CRegionWorld

	int iQty = m_RegionLinks.GetCount();
	for ( int i=0; i<iQty; i++ )
	{
		CRegionBase * pRegion = m_RegionLinks[i];
		ASSERT(pRegion);

		ASSERT( pRegion->GetResourceID().IsValidUID());
		if ( pRegion->GetResourceID().IsItem())
		{
			if ( ! ( dwType & REGION_TYPE_MULTI ))
				continue;
		}
		else if ( pRegion->GetResourceID().GetResType() == RES_AREA )
		{
			if ( ! ( dwType & REGION_TYPE_AREA ))
				continue;
		}
		else
		{
			if ( ! ( dwType & REGION_TYPE_ROOM ))
				continue;
		}

		if ( ! pRegion->m_pt.IsSameMap(pt.m_map))
			continue;
		if ( ! pRegion->IsInside2d( pt ))
			continue;
		return( pRegion );
	}
	return( NULL );
}

// Balkon: get regions list (to cicle through intercepted house regions)
int CSectorBase::GetRegions( const CPointBase & pt, DWORD dwType, CRegionLinks & rlist ) const
{
	ADDTOCALLSTACK("CSectorBase::GetRegions");
	int iQty = m_RegionLinks.GetCount();
	for ( int i=0; i<iQty; i++ )
	{
		CRegionBase * pRegion = m_RegionLinks[i];
		ASSERT(pRegion);

		ASSERT( pRegion->GetResourceID().IsValidUID());
		if ( pRegion->GetResourceID().IsItem())
		{
			if ( ! ( dwType & REGION_TYPE_MULTI ))
				continue;
		}
		else if ( pRegion->GetResourceID().GetResType() == RES_AREA )
		{
			if ( ! ( dwType & REGION_TYPE_AREA ))
				continue;
		}
		else
		{
			if ( ! ( dwType & REGION_TYPE_ROOM ))
				continue;
		}

		if ( ! pRegion->m_pt.IsSameMap(pt.m_map))
			continue;
		if ( ! pRegion->IsInside2d( pt ))
			continue;
		rlist.Add( pRegion );
	}
	return( rlist.GetCount() );
}

bool CSectorBase::UnLinkRegion( CRegionBase * pRegionOld )
{
	ADDTOCALLSTACK("CSectorBase::UnLinkRegion");
	if ( !pRegionOld )
		return false;
	return m_RegionLinks.RemovePtr(pRegionOld);
}

bool CSectorBase::LinkRegion( CRegionBase * pRegionNew )
{
	ADDTOCALLSTACK("CSectorBase::LinkRegion");
	// link in a region. may have just moved !
	// Make sure the smaller regions are first in the array !
	// Later added regions from the MAP file should be the smaller ones, 
	//  according to the old rules.
	ASSERT(pRegionNew);
	ASSERT( pRegionNew->IsOverlapped( GetRect()));
	int iQty = m_RegionLinks.GetCount();

	for ( int i=0; i<iQty; i++ )
	{
		CRegionBase * pRegion = m_RegionLinks[i];
		ASSERT(pRegion);
		if ( pRegionNew == pRegion )
		{
			DEBUG_ERR(( "region already linked!\n" ));
			return false;
		}

		if ( pRegion->IsOverlapped(pRegionNew))
		{
			// NOTE : We should use IsInside() but my version isn't completely accurate for it's FALSE return
			if ( pRegion->IsEqualRegion( pRegionNew ))
			{
				DEBUG_ERR(( "Conflicting region!\n" ));
				return( false );
			}
			if ( pRegionNew->IsInside(pRegion))	// it is accurate in the TRUE case.
				continue;

			// must insert before this.
			m_RegionLinks.InsertAt( i, pRegionNew );
			return( true );
		}
	}

	m_RegionLinks.Add( pRegionNew );
	return( true );
}

CTeleport * CSectorBase::GetTeleport2d( const CPointMap & pt ) const
{
	ADDTOCALLSTACK("CSectorBase::GetTeleport2d");
	// Any teleports here at this point ?

	int i = m_Teleports.FindKey( pt.GetPointSortIndex());
	if ( i < 0 )
		return( NULL );
	return STATIC_CAST <CTeleport *>( m_Teleports[i]);
}

CTeleport * CSectorBase::GetTeleport( const CPointMap & pt ) const
{
	ADDTOCALLSTACK("CSectorBase::GetTeleport");
	// Any teleports here at this point ?

	CTeleport * pTeleport = GetTeleport2d( pt );
	if ( pTeleport == NULL )
		return( NULL );

	int zdiff = pt.m_z - pTeleport->m_z;	
	if ( abs(zdiff) > 5 )
		return( NULL );

	// Check m_map ?
	if ( ! pTeleport->IsSameMap( pt.m_map ))
		return( NULL );

	return( pTeleport );
}

bool CSectorBase::AddTeleport( CTeleport * pTeleport )
{
	ADDTOCALLSTACK("CSectorBase::AddTeleport");
	// NOTE: can't be 2 teleports from the same place !
	// ASSERT( Teleport is actually in this sector !

	int i = m_Teleports.FindKey( pTeleport->GetPointSortIndex());
	if ( i >= 0 )
	{
		DEBUG_ERR(( "Conflicting teleport %s!\n", pTeleport->WriteUsed() ));
		return( false );
	}
	m_Teleports.AddSortKey( pTeleport, pTeleport->GetPointSortIndex());
	return( true );
}

CPointMap CSectorBase::GetBasePoint() const
{
	ADDTOCALLSTACK("CSectorBase::GetBasePoint");
	// What is the coord base of this sector. upper left point.
	ASSERT( m_index >= 0 && m_index < g_MapList.GetSectorQty(m_map) );
	CPointMap pt(( m_index % g_MapList.GetSectorCols(m_map)) * g_MapList.GetSectorSize(m_map),
		( m_index / g_MapList.GetSectorCols(m_map) ) * g_MapList.GetSectorSize(m_map),
		0,
		m_map);
	return( pt );
}

CRectMap CSectorBase::GetRect() const
{
	ADDTOCALLSTACK("CSectorBase::GetRect");
	// Get a rectangle for the sector.
	CPointMap pt = GetBasePoint();
	CRectMap rect;
	rect.m_left = pt.m_x;
	rect.m_top = pt.m_y;
	rect.m_right = pt.m_x + g_MapList.GetSectorSize(pt.m_map);	// East
	rect.m_bottom = pt.m_y + g_MapList.GetSectorSize(pt.m_map);	// South
	rect.m_map = pt.m_map;
	return( rect );
}
