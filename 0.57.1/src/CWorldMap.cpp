#include "graysvr.h"

//************************
// Natural resources.

CItem * CWorld::CheckNaturalResource( const CPointMap & pt, IT_TYPE Type, bool fTest, CChar * pCharSrc )
{
	// RETURN: 
	//  The resource tracking item.
	//  NULL = nothing here.

	if ( !pt.IsValidPoint() )
		return NULL;

	EXC_TRY("CheckNaturalResource");

	// Check/Decrement natural resources at this location.
	// We create an invis object to time out and produce more.
	// RETURN: Quantity they wanted. 0 = none here.

	if ( fTest )	// Is the resource avail at all here ?
	{
		EXC_SET("is item near type");
		if ( !g_World.IsItemTypeNear(pt, Type) )
			return NULL;
	}

	// Find the resource object.
	EXC_SET("find existant bit");
	CItem * pResBit;
	CWorldSearch Area(pt);
	while ( pResBit = Area.GetItem() )
	{
		// NOTE: ??? Not all resource objects are world gems. should they be ?
		// I wanted to make tree stumps etc be the resource block some day.
		if ( pResBit->IsType(Type) && pResBit->GetID() == ITEMID_WorldGem )
			break;
	}
	if ( pResBit ) return pResBit;

	// What type of ore is here ?
	// NOTE: This is unrelated to the fact that we might not be skilled enough to find it.
	// Odds of a vein of ore being present are unrelated to my skill level.
	// Odds of my finding it are.
	// RES_REGIONRESOURCE from RES_REGIONTYPE linked to RES_AREA

	EXC_SET("get region");
	CRegionWorld* pRegion = dynamic_cast <CRegionWorld*>( pt.GetRegion( REGION_TYPE_AREA ));
	if ( !pRegion )
		return NULL;

	// just use the background (default) region for this
	if ( !pRegion->m_Events.GetCount() )
	{
		CPointMap ptZero(0,0,0,pt.m_map);
		pRegion = dynamic_cast <CRegionWorld*>(ptZero.GetRegion(REGION_TYPE_AREA));
	}

	// Find RES_REGIONTYPE
	EXC_SET("resource group");
	const CRandGroupDef * pResGroup = pRegion->FindNaturalResource(Type);
	if ( !pResGroup )
		return NULL;

	// Find RES_REGIONRESOURCE
	EXC_SET("get random group element");
	int	id	= pResGroup->GetRandMemberIndex(pCharSrc);
	CRegionResourceDef *	pOreDef;
	if ( id == -1 )
	{
		pOreDef	= dynamic_cast <CRegionResourceDef *> (g_Cfg.ResourceGetDefByName(RES_REGIONRESOURCE, "mr_nothing"));
	}
	else
	{
		RESOURCE_ID rid	= pResGroup->GetMemberID( id );
		pOreDef		= dynamic_cast <CRegionResourceDef *>( g_Cfg.ResourceGetDef( rid ));
	}

	if ( !pOreDef )
		return NULL;

	EXC_SET("create bit");
	pResBit = CItem::CreateScript(ITEMID_WorldGem, pCharSrc);
	if ( !pResBit )
		return NULL;
	
	pResBit->SetType(Type);
	pResBit->SetAttr(ATTR_INVIS|ATTR_MOVE_NEVER);
	pResBit->m_itResource.m_rid_res = pOreDef->GetResourceID();

	// Total amount of ore here.
	pResBit->SetAmount(pOreDef->m_Amount.GetRandom());
	pResBit->MoveToDecay(pt, pOreDef->m_iRegenerateTime * TICK_PER_SEC);	// Delete myself in this amount of time.

	EXC_SET("resourcefound");
	CScriptTriggerArgs	Args(0, 0, pResBit);
	if ( pOreDef->OnTrigger("@ResourceFound", pCharSrc, &Args) == TRIGRET_RET_TRUE )
	{
		if ( pResBit->IsDisconnected() )
			return NULL;
		pResBit->SetAmount(0);
	}
	return pResBit;

	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.Debug("point '%d,%d,%d,%d' type '%d' [0%lx]\n", pt.m_x, pt.m_y, pt.m_z, pt.m_map, (int)Type, 
		pCharSrc ? pCharSrc->uid() : 0);
	EXC_DEBUG_END;
	return NULL;
}

//////////////////////////////////////////////////////////////////
// Map reading and blocking.

bool CWorld::IsItemTypeNear( const CPointMap & pt, IT_TYPE iType, int iDistance )
{
	if ( !pt.IsValidPoint() )
		return false;
	CPointMap ptn = FindItemTypeNearby( pt, iType, iDistance );
	return( ptn.IsValidPoint());
}

CPointMap CWorld::FindItemTypeNearby(const CPointMap & pt, IT_TYPE iType, int iDistance, bool bLimitZ)
{
	// Find the closest item of this type.
	// This does not mean that i can touch it.
	// ARGS:
	//   iDistance = 2d distance to search.

	CPointMap ptFound;
	int iTestDistance;

		
	// Check dynamics first since they are the easiest.
	CWorldSearch Area(pt, iDistance);
	while ( CItem *pItem = Area.GetItem() )
	{
		if ( !pItem->IsType(iType) && !pItem->Item_GetDef()->IsType(iType) )
			continue;
		if ( bLimitZ && ( pItem->GetTopPoint().m_z != pt.m_z ))
			continue;

		iTestDistance = pt.GetDist(pItem->GetTopPoint());
		if ( iTestDistance > iDistance )
			continue;

		ptFound = pItem->GetTopPoint();
		iDistance = iTestDistance;	// tighten up the search.
		if ( ! iDistance )
			return( ptFound );
	}

	// Check for appropriate terrain type
	CRectMap rect;
	CItemTypeDef	* pTypeDef	= NULL;
	rect.SetRect( pt.m_x - iDistance, pt.m_y - iDistance,
		pt.m_x + iDistance + 1, pt.m_y + iDistance + 1,
		pt.m_map);

	const CUOMapMeter * pMeter = NULL;
	for (int x = rect.m_left; x < rect.m_right; x++, pMeter = NULL )
	{
		for ( int y = rect.m_top; y < rect.m_bottom; y++, pMeter = NULL )
		{
			CPointMap ptTest( x, y, pt.m_z, pt.m_map);
			pMeter = GetMapMeter(ptTest);
			if ( !pMeter )
				continue;
			if ( bLimitZ && ( pMeter->m_z != pt.m_z ) )
				continue;

			ptTest.m_z = pMeter->m_z;

			if ( iType != g_World.GetTerrainItemType( pMeter->m_wTerrainIndex ) )
				continue;

			iTestDistance = pt.GetDist(ptTest);
			if ( iTestDistance > iDistance )
				break;

			ptFound = ptTest;
			iDistance = iTestDistance;	// tighten up the search.
			if ( ! iDistance )
				return( ptFound );
			rect.SetRect( pt.m_x - iDistance, pt.m_y - iDistance,
				pt.m_x + iDistance + 1, pt.m_y + iDistance + 1,
				pt.m_map);
		}
	}

	// Any statics here? (checks just 1 8x8 block !???)
	const CGrayMapBlock * pMapBlock = GetMapBlock( pt );
	int iQty = pMapBlock->m_Statics.GetStaticQty();
	if ( iQty )  // no static items here.
	{
		int x2=pMapBlock->GetOffsetX(pt.m_x);
		int y2=pMapBlock->GetOffsetY(pt.m_y);
		const CUOStaticItemRec * pStatic = NULL;
		const CItemBase * pItemDef = NULL;

		for ( int i=0; i < iQty; i++, pStatic = NULL, pItemDef = NULL )
		{
			pStatic = pMapBlock->m_Statics.GetStatic( i );
			if ( bLimitZ && ( pStatic->m_z != pt.m_z ) )
				continue;

			// inside the range we want ?
			CPointMap ptTest( pStatic->m_x+pMapBlock->m_x, pStatic->m_y+pMapBlock->m_y, pStatic->m_z, pt.m_map);
			iTestDistance = pt.GetDist(ptTest);
			if ( iTestDistance > iDistance )
				continue;

			ITEMID_TYPE idTile = pStatic->GetDispID();

			// Check the script def for the item.
			pItemDef = CItemBase::FindItemBase( idTile );
			if ( pItemDef == NULL )
				continue;
			if ( ! pItemDef->IsType( iType ))
				continue;
			ptFound = ptTest;
			iDistance = iTestDistance;
			if ( ! iDistance )
				return( ptFound );
		}
	}
	// Parts of multis ?

	return ptFound;
}

//****************************************************

void CWorld::GetHeightPoint( const CPointMap & pt, CGrayMapBlockState & block, bool fHouseCheck )
{
	// Height of statics at/above given coordinates
	// do gravity here for the z.

	WORD wBlockThis = 0;
	const CGrayMapBlock * pMapBlock = GetMapBlock( pt );

	if ( pMapBlock )
	{
		int x2=pMapBlock->GetOffsetX(pt.m_x);
		int y2=pMapBlock->GetOffsetY(pt.m_y);
		for ( int i=0; i < pMapBlock->m_Statics.GetStaticQty(); i++ )
		{
			if ( ! pMapBlock->m_Statics.IsStaticPoint( i, x2, y2 ))
				continue;
			const CUOStaticItemRec * pStatic = pMapBlock->m_Statics.GetStatic( i );
			signed char z = pStatic->m_z;
			if ( ! block.IsUsableZ(z,PLAYER_HEIGHT))
				continue;

			// This static is at the coordinates in question.
			// enough room for me to stand here ?
			wBlockThis = 0;
			signed char zHeight = CItemBase::GetItemHeight( pStatic->GetDispID(), wBlockThis );
			block.CheckTile( wBlockThis, z, zHeight, pStatic->GetDispID() + TERRAIN_QTY );
		}
	}

	// Any multi items here ?
	if ( fHouseCheck )
	{
		CRegionLinks rlinks;
		for ( int i = 0; i < pt.GetRegions(REGION_TYPE_MULTI, rlinks); i++)
		{
			CRegionBase * pRegion = rlinks.GetAt(i);
			CItem * pItem = pRegion->GetResourceID().ItemFind();
			if ( pItem != NULL )
			{
				const CGrayMulti * pMulti = g_Cfg.GetMultiItemDefs( pItem->GetDispID());
				if ( pMulti )
				{
					int x2 = pt.m_x - pItem->GetTopPoint().m_x;
					int y2 = pt.m_y - pItem->GetTopPoint().m_y;

					int iQty = pMulti->GetItemCount();
					for ( int i=0; iQty--; i++ )
					{
						const CUOMultiItemRec * pMultiItem = pMulti->GetItem(i);
						if ( ! pMultiItem->m_visible )
							continue;
						if ( pMultiItem->m_dx != x2 || pMultiItem->m_dy != y2 )
							continue;

						signed char zitem = pItem->GetTopZ() + pMultiItem->m_dz;
						if ( ! block.IsUsableZ(zitem,PLAYER_HEIGHT))
							continue;

						wBlockThis = 0;
						signed char zHeight = CItemBase::GetItemHeight( pMultiItem->GetDispID(), wBlockThis );
						block.CheckTile( wBlockThis, zitem, zHeight, pMultiItem->GetDispID() + TERRAIN_QTY );
					}
				}
			}
		}
	}

	// Any dynamic items here ?
	// NOTE: This could just be an item that an NPC could just move ?
	CWorldSearch Area(pt);
	while ( CItem *pItem = Area.GetItem() )
	{
		signed char zitem = pItem->GetTopZ();
		if ( !block.IsUsableZ(zitem,PLAYER_HEIGHT) )
			continue;

		// Invis items should not block ???
		CItemBase * pItemDef = pItem->Item_GetDef();
		WORD canFlags = pItemDef->m_Can & (CAN_I_DOOR|CAN_I_WATER|CAN_I_CLIMB|CAN_I_BLOCK|CAN_I_PLATFORM);
		block.CheckTile(canFlags, zitem, pItemDef->GetHeight(), pItemDef->GetDispID() + TERRAIN_QTY);
		wBlockThis |= canFlags;
		if ( wBlockThis & CAN_I_BLOCK ) return;
	}

	// Check Terrain here.
	// Terrain height is screwed. Since it is related to all the terrain around it.

	const CUOMapMeter * pMeter = pMapBlock->GetTerrain( UO_BLOCK_OFFSET(pt.m_x), UO_BLOCK_OFFSET(pt.m_y));

	if ( pMeter && block.IsUsableZ(pMeter->m_z,0) )
	{
		if ( pMeter->m_wTerrainIndex == TERRAIN_HOLE )
			wBlockThis = 0;
		else if ( pMeter->m_wTerrainIndex == TERRAIN_NULL )	// inter dungeon type.
			wBlockThis = CAN_I_BLOCK;
		else
		{
			CGrayTerrainInfo land( pMeter->m_wTerrainIndex );
			if ( land.m_flags & UFLAG2_PLATFORM ) // Platform items should take precendence over non-platforms.
				wBlockThis = CAN_I_PLATFORM;
			else if ( land.m_flags & UFLAG1_WATER )
				wBlockThis = CAN_I_WATER;
			else if ( land.m_flags & UFLAG1_DAMAGE )
				wBlockThis = CAN_I_FIRE;
			else if ( land.m_flags & UFLAG1_BLOCK )
				wBlockThis = CAN_I_BLOCK;
			else
				wBlockThis = CAN_I_PLATFORM;
		}
		block.CheckTile( wBlockThis, pMeter->m_z, 0, pMeter->m_wTerrainIndex );
	}

	if ( block.m_Bottom.m_z == UO_SIZE_MIN_Z )
	{
		block.m_Bottom = block.m_Lowest;
	}
}

signed char CWorld::GetHeightPoint( const CPointBase & pt, WORD & wBlockFlags, bool fHouseCheck ) // Height of player who walked to X/Y/OLDZ
{
	// Given our coords at pt including pt.m_z
	// What is the height that gravity would put me at should i step hear ?
	// Assume my head height is PLAYER_HEIGHT/2
	// ARGS:
	//  pt = the point of interest.
	//  pt.m_z = my starting altitude.
	//  wBlockFlags = what we can pass thru. doors, water, walls ?
	//		CAN_C_GHOST	= Moves thru doors etc.	- CAN_I_DOOR						
	//		CAN_C_SWIM = walk thru water - CAN_I_WATER
	//		CAN_C_WALK = it is possible that i can step up. - CAN_I_PLATFORM
	//		CAN_C_PASSWALLS	= walk through all blocking items - CAN_I_BLOCK
	//		CAN_C_FLY  = gravity does not effect me. - CAN_I_CLIMB
	//		CAN_C_FIRE_IMMUNE = i can walk into lava etc. - CAN_I_FIRE
	//  pRegion = possible regional effects. (multi's)
	// RETURN:
	//  pt.m_z = Our new height at pt.m_x,pt.m_y
	//  wBlockFlags = our blocking flags at the given location. CAN_I_WATER,CAN_C_WALK,CAN_FLY,CAN_SPIRIT,
	//    CAN_C_INDOORS = i am covered from the sky
	//

	// ??? NOTE: some creatures should be taller than others !!!

	WORD wCan = wBlockFlags;
	CGrayMapBlockState block( wBlockFlags, pt.m_z, PLAYER_HEIGHT );

	GetHeightPoint( pt, block, fHouseCheck );

	// Pass along my results.
	wBlockFlags = block.m_Bottom.m_dwBlockFlags;
	if ( block.m_Top.m_dwBlockFlags )
		wBlockFlags |= CAN_I_ROOF;	// we are covered by something.

	if (( wBlockFlags & ( CAN_I_CLIMB|CAN_I_PLATFORM) ) && ( wCan & CAN_C_WALK ))
	{
		wBlockFlags &= ~CAN_I_CLIMB;
		wBlockFlags |= CAN_I_PLATFORM;	// not really true but hack it anyhow.
		return( block.m_Bottom.m_z );
	}

	if ( wCan & CAN_C_FLY )
		return pt.m_z;

	return( block.m_Bottom.m_z );
}

void CWorld::GetHeightPoint_New( const CPointMap & pt, CGrayMapBlockState & block, bool fHouseCheck )
{
	// Height of statics at/above given coordinates
	// do gravity here for the z.

	WORD wBlockThis = 0;
	const CGrayMapBlock * pMapBlock = GetMapBlock( pt );

	if ( pMapBlock )
	{
		int iQty = pMapBlock->m_Statics.GetStaticQty();
		if ( iQty )  // no static items here.
		{
			int x2 = pMapBlock->GetOffsetX(pt.m_x);
			int y2 = pMapBlock->GetOffsetY(pt.m_y);
			const CUOStaticItemRec * pStatic = NULL;
			signed char z, zHeight = 0;
			for ( int i=0; i<iQty; i++, z = 0, zHeight = 0, pStatic = NULL )
			{
				if ( ! pMapBlock->m_Statics.IsStaticPoint( i, x2, y2 ))
					continue;
				pStatic = pMapBlock->m_Statics.GetStatic( i );
				z = pStatic->m_z;
				if ( ! block.IsUsableZ(z,PLAYER_HEIGHT))
					continue;
				// This static is at the coordinates in question.
				// enough room for me to stand here ?
				wBlockThis = 0; // item CAN flags returned here
				zHeight = CItemBase::GetItemHeight( pStatic->GetDispID(), wBlockThis );
				block.CheckTile_Item( wBlockThis, z, zHeight, pStatic->GetDispID() + TERRAIN_QTY );
			}
		}
	}

	// Any multi items here ?
	// Check all of them
	if ( fHouseCheck )
	{
		CRegionLinks rlinks;
		if ( int iQty = pt.GetRegions( REGION_TYPE_MULTI, rlinks ) )
		{
			//  ------------ For variables --------------------
			CRegionBase * pRegion = NULL;
			CItem * pItem = NULL;
			const CGrayMulti * pMulti = NULL;
			const CUOMultiItemRec * pMultiItem = NULL;
			signed char	zitem = 0;
			signed char zHeight = 0;
			int x2 = 0;
			int y2 = 0;
			//  ------------ For variables --------------------

			for ( int i = 0; i < iQty; i++, pRegion = NULL, pItem = NULL, pMulti = NULL, x2 = 0, y2 = 0 )
			{
				pRegion = rlinks.GetAt(i);
				if ( pRegion )
					pItem = pRegion->GetResourceID().ItemFind();

				if ( pItem != NULL )
				{
					pMulti = g_Cfg.GetMultiItemDefs( pItem->GetDispID());
					if ( pMulti )
					{
						x2 = pt.m_x - pItem->GetTopPoint().m_x;
						y2 = pt.m_y - pItem->GetTopPoint().m_y;

						int iQty = pMulti->GetItemCount();
						for ( int i=0; iQty--; i++, pMultiItem = NULL, zitem = 0, zHeight = 0 )
						{
							pMultiItem = pMulti->GetItem(i);
							if ( ! pMultiItem->m_visible )
								continue;
							if ( pMultiItem->m_dx != x2 || pMultiItem->m_dy != y2 )
								continue;

							zitem = pItem->GetTopZ() + pMultiItem->m_dz;
							if ( ! block.IsUsableZ(zitem,PLAYER_HEIGHT))
								continue;

							wBlockThis = 0;
							zHeight = CItemBase::GetItemHeight( pMultiItem->GetDispID(), wBlockThis );
							block.CheckTile_Item( wBlockThis, zitem, zHeight, pMultiItem->GetDispID() + TERRAIN_QTY );
						}
					}
				}
			}
		}
	}

	// Any dynamic items here ?
	// NOTE: This could just be an item that an NPC could just move ?
	CWorldSearch Area(pt);
	signed char zitem;
	CItemBase * pItemDef;
	while ( CItem *pItem = Area.GetItem() )
	{
		zitem = pItem->GetTopZ();
		if ( !block.IsUsableZ(zitem,PLAYER_HEIGHT) )
			continue;

		// Invis items should not block ???
		pItemDef = pItem->Item_GetDef();

		WORD canFlags = pItemDef->m_Can & (CAN_I_DOOR|CAN_I_WATER|CAN_I_CLIMB|CAN_I_BLOCK|CAN_I_PLATFORM);
		block.CheckTile_Item(canFlags, zitem, pItemDef->GetHeight(), pItemDef->GetDispID() + TERRAIN_QTY);
		wBlockThis |= canFlags;
		if ( wBlockThis & CAN_I_BLOCK ) return;
	}

	// Terrain height is screwed. Since it is related to all the terrain around it.
	const CUOMapMeter * pMeter = pMapBlock->GetTerrain( UO_BLOCK_OFFSET(pt.m_x), UO_BLOCK_OFFSET(pt.m_y));
	if ( pMeter && block.IsUsableZ( pMeter->m_z,0 ) )
	{
		if ( pMeter->m_wTerrainIndex == TERRAIN_HOLE )
			wBlockThis = 0;
		else if ( pMeter->m_wTerrainIndex == TERRAIN_NULL )	// inter dungeon type.
			wBlockThis = CAN_I_BLOCK;
		else
		{
			CGrayTerrainInfo land( pMeter->m_wTerrainIndex );
			if ( land.m_flags & UFLAG2_PLATFORM ) // Platform items should take precendence over non-platforms.
				wBlockThis = CAN_I_PLATFORM;
			else if ( land.m_flags & UFLAG1_WATER )
				wBlockThis = CAN_I_WATER;
			else if ( land.m_flags & UFLAG1_DAMAGE )
				wBlockThis = CAN_I_FIRE;
			else if ( land.m_flags & UFLAG1_BLOCK )
				wBlockThis = CAN_I_BLOCK;
			else
				wBlockThis = CAN_I_PLATFORM;
		}
		block.CheckTile_Terrain( wBlockThis, pMeter->m_z, pMeter->m_wTerrainIndex );
	}

	if ( block.m_Bottom.m_z == UO_SIZE_MIN_Z )
	{
		block.m_Bottom = block.m_Lowest;
	}
}

signed char CWorld::GetHeightPoint_New( const CPointBase & pt, WORD & wBlockFlags, bool fHouseCheck )
{
	WORD wCan = wBlockFlags;
	CGrayMapBlockState block( wBlockFlags, pt.m_z + (PLAYER_HEIGHT / 2), pt.m_z + PLAYER_HEIGHT );

	GetHeightPoint_New( pt, block, fHouseCheck );

	// Pass along my results.
	wBlockFlags = block.m_Bottom.m_dwBlockFlags;
	if ( block.m_Top.m_dwBlockFlags )
		wBlockFlags |= CAN_I_ROOF;	// we are covered by something.
	if (( wBlockFlags & ( CAN_I_CLIMB|CAN_I_PLATFORM) ) && ( wCan & CAN_C_WALK ))
	{
		wBlockFlags &= ~CAN_I_CLIMB;
		wBlockFlags |= CAN_I_PLATFORM;	// not really true but hack it anyhow.
		return( block.m_Bottom.m_z );
	}
	if ( wCan & CAN_C_FLY )
		return pt.m_z;

	return( block.m_Bottom.m_z );
}


CItemTypeDef *	CWorld::GetTerrainItemTypeDef( DWORD dwTerrainIndex )
{
	CResourceDef *	pRes	= NULL;

	if ( g_World.m_TileTypes.IsValidIndex( dwTerrainIndex ) )
	{
		pRes	= g_World.m_TileTypes[dwTerrainIndex];
	}
		
	if ( !pRes )
	{
		RESOURCE_ID	rid( RES_TYPEDEF, 0 );
		pRes	= g_Cfg.ResourceGetDef( rid );
	}
	CItemTypeDef *	pItemTypeDef	= dynamic_cast <CItemTypeDef*> (pRes);
	return pItemTypeDef;
}


IT_TYPE		CWorld::GetTerrainItemType( DWORD dwTerrainIndex )
{
	CResourceDef *	pRes	= NULL;

	if ( g_World.m_TileTypes.IsValidIndex( dwTerrainIndex ) )
	{
		pRes	= g_World.m_TileTypes[dwTerrainIndex];
	}
		
	if ( !pRes )
		return IT_NORMAL;

	CItemTypeDef	*pItemTypeDef	= dynamic_cast <CItemTypeDef*> (pRes);
	if ( !pItemTypeDef )
		return IT_NORMAL;

	return (IT_TYPE) pItemTypeDef->GetItemType();
}
