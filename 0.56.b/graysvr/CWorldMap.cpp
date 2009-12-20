//
// CWorldMap.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "graysvr.h"	// predef header.

//************************
// Natural resources.

CItem * CWorld::CheckNaturalResource( const CPointMap & pt, IT_TYPE Type, bool fTest, CChar * pCharSrc )
{
	ADDTOCALLSTACK("CWorld::CheckNaturalResource");
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
		if (IsSetEF( EF_NewPositionChecks ) && (Type != IT_TREE) && (Type != IT_ROCK) )
		//		if (IsSetEF( EF_NewPositionChecks ))
		{
			if ( !g_World.IsTypeNear_Top(pt, Type, 0) )
				return NULL;
		}
		else
		{
			if ( !g_World.IsItemTypeNear(pt, Type, 0, false) ) //cannot be used, because it does no Z check... what if there is a static tile 70 tiles under me?
				return NULL;
		}
	}

	// Find the resource object.
	EXC_SET("find existant bit");
	CItem * pResBit;
	CWorldSearch Area(pt);
	while ( true )
	{
		pResBit = Area.GetItem();
		if ( !pResBit )
			break;
		// NOTE: ??? Not all resource objects are world gems. should they be ?
		// I wanted to make tree stumps etc be the resource block some day.

		if ( pResBit->IsType(Type) && pResBit->GetID() == ITEMID_WorldGem )
			break;
	}

	// If none then create one.
	if ( pResBit )
		return pResBit;

	// What type of ore is here ?
	// NOTE: This is unrelated to the fact that we might not be skilled enough to find it.
	// Odds of a vein of ore being present are unrelated to my skill level.
	// Odds of my finding it are.
	// RES_REGIONRESOURCE from RES_REGIONTYPE linked to RES_AREA

	EXC_SET("get region");
	CRegionWorld* pRegion = dynamic_cast <CRegionWorld*>( pt.GetRegion( REGION_TYPE_AREA ));
	if ( !pRegion )
		return NULL;

	CWorldSearch AreaItems( pt );
	AreaItems.SetAllShow(1);
	while (true)
	{
		CItem *pItem = AreaItems.GetItem();
		if ( !pItem )
			break;
		if ( pItem->GetType() != Type )
			return NULL;
	}

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
	g_Log.EventDebug("point '%d,%d,%d,%d' type '%d' [0%lx]\n", pt.m_x, pt.m_y, pt.m_z, pt.m_map, (int)Type, 
		pCharSrc ? (DWORD)pCharSrc->GetUID() : 0);
	EXC_DEBUG_END;
	return NULL;
}

//////////////////////////////////////////////////////////////////
// Map reading and blocking.

bool CWorld::IsTypeNear_Top( const CPointMap & pt, IT_TYPE iType, int iDistance )
{
	ADDTOCALLSTACK("CWorld::IsTypeNear_Top");
	if ( !pt.IsValidPoint() )
		return false;
	CPointMap ptn = FindTypeNear_Top( pt, iType, iDistance );
	return( ptn.IsValidPoint());
}

CPointMap CWorld::FindTypeNear_Top( const CPointMap & pt, IT_TYPE iType, int iDistance )
{
	ADDTOCALLSTACK("CWorld::FindTypeNear_Top");
#define RESOURCE_Z_CHECK 8
	CPointMap ptFound;
	CItemBase * pItemDef = NULL;
	CItem * pItem = NULL;
	CItemBaseDupe * pDupeDef = NULL;
	BYTE Height = 0;
	BYTE z = 0;
	CPointMap ptTest;
	int iQty;

	int iRetElem = 4;

	CPointMap ptElem[5];
	memset(ptElem, 0, sizeof(ptElem));
	//for ( iQty = 0; iQty < 4; ++iQty )
	//	ptElem[iQty].m_z = -127;
	ptElem[0].m_z = ptElem[1].m_z  = ptElem[2].m_z  = ptElem[3].m_z = -127;
	ptElem[4] = CPointMap(-1,-1,-127);

	bool fElem[4] = { false, false, false, false };

	// Check dynamics
	CWorldSearch Area( pt, iDistance );
	Area.SetAllShow( true );
	while (true)
	{
		z = 0;
		Height = 0;
		pItem = Area.GetItem();
		if ( pItem == NULL )
			break;

		if ( pt.GetDist( pItem->GetTopPoint() ) > iDistance )
			continue;

		pItemDef = CItemBase::FindItemBase( pItem->GetDispID() );
		if ( pItemDef == NULL )
			continue;

		Height = pItemDef->GetHeight();
		if ( pItemDef->GetID() != pItem->GetDispID() ) //not a parent item
		{
			pDupeDef = CItemBaseDupe::GetDupeRef((ITEMID_TYPE) pItem->GetDispID() );
			if ( ! pDupeDef )
			{
				g_Log.EventDebug("Failed to get non-parent reference (dynamic) (DispID 0%x) (X: %d Y: %d Z: %d)\n",pItem->GetDispID(),ptTest.m_x,ptTest.m_y,ptTest.m_z);
				Height = pItemDef->GetHeight();
			}
			else
				Height = pDupeDef->GetHeight();
		}
		z = ( Height + pItem->GetTopPoint().m_z ); //height + current position = the top point

		if ( ptElem[0].m_z > z ) //if ( ptElem[0].m_z > pItem->GetTopPoint().m_z )
			continue;

		if ( ((( z - pt.m_z ) > 0) && ( z - pt.m_z ) > RESOURCE_Z_CHECK ) || ((( pt.m_z - z ) < 0) && (( pt.m_z - z ) < - RESOURCE_Z_CHECK ))) //if ( ((( pItem->GetTopPoint().m_z - pt.m_z ) > 0) && ( pItem->GetTopPoint().m_z - pt.m_z ) > RESOURCE_Z_CHECK ) || ((( pt.m_z - pItem->GetTopPoint().m_z ) < 0) && (( pt.m_z - pItem->GetTopPoint().m_z ) < - RESOURCE_Z_CHECK )))
			continue;

		if (( z < ptElem[0].m_z ) || (( z == ptElem[0].m_z ) && ( fElem[0] )))
			continue;
        
		ptElem[0] = pItem->GetTopPoint();
		ptElem[0].m_z = z;
		fElem[0] = false;

		//DEBUG_ERR(("dynamic pItem->IsType( iType %d) %d\n",iType,pItem->IsType( iType )));
		if ( pItem->IsType( iType ) )  //( pItem->Item_GetDef()->IsType(iType) ) )
		{
			fElem[0] = true;
			iRetElem = 0;
		}
	}

	// Parts of multis ?
	const CGrayMulti * pMulti 			= NULL;		// Multi Def (multi check)
	const CUOMultiItemRec * pMultiItem 	= NULL;		// Multi item iterator
	CRegionBase * pRegion				= NULL;
	CRegionLinks rlinks;
	int iQtyr = pt.GetRegions( REGION_TYPE_MULTI, rlinks );
	if ( iQtyr )
	{
		for ( int aa = 0; aa < iQtyr; pMulti = NULL, ++aa)
		{
			pRegion = rlinks.GetAt(aa);
			pItem = pRegion->GetResourceID().ItemFind();
			if ( !pItem )
				continue;
			CItemMultiCustom * pItemMulti = dynamic_cast<CItemMultiCustom*>( pItem );
			if ( pItemMulti == NULL )
				pMulti = g_Cfg.GetMultiItemDefs( pItem->GetDispID() );
			else
				pMulti = pItemMulti->GetMultiItemDefs();
			if ( !pMulti )
				continue;
			int iQty = pMulti->GetItemCount();
			for ( int ab = 0; ab < iQty; pItemDef = NULL, pMultiItem = NULL, Height = 0, ++ab )
			{
				pMultiItem = pMulti->GetItem(ab);
				
				if ( !pMultiItem )
					break;

				//DEBUG_ERR(("abs( pMultiItem->m_dx ) %x, abs( pMultiItem->m_dy ) %x, abs( pMultiItem->m_dz ) %x,\n             iDistance %x IF STATEMENT %x %x\n", abs( pMultiItem->m_dx ), abs( pMultiItem->m_dy ), abs( pMultiItem->m_dz ), iDistance, ( abs( pMultiItem->m_dx ) <= iDistance ), ( abs( pMultiItem->m_dy ) <= iDistance ) ));

				if ( !pMultiItem->m_visible )
					continue;

				ptTest = CPointMap( pMultiItem->m_dx + pt.m_x, pMultiItem->m_dy + pt.m_y, pMultiItem->m_dz + pt.m_z, pt.m_map );

				pItemDef = CItemBase::FindItemBase( pMultiItem->GetDispID() );
				if ( pItemDef == NULL )
					continue;

				Height = pItemDef->GetHeight();
				if ( pItemDef->GetID() != pMultiItem->GetDispID() ) //not a parent item
				{
					pDupeDef = CItemBaseDupe::GetDupeRef((ITEMID_TYPE) pMultiItem->GetDispID() );
					if ( ! pDupeDef )
					{
						g_Log.EventDebug("Failed to get non-parent reference (multi) (DispID 0%x) (X: %d Y: %d Z: %d)\n",pMultiItem->GetDispID(),ptTest.m_x,ptTest.m_y,ptTest.m_z);
						Height = pItemDef->GetHeight();
					}
					else
						Height = pDupeDef->GetHeight();
				}
				ptTest.m_z += Height; //height + current position = the top point

				if ( pt.GetDist( ptTest ) > iDistance )
					continue;

				if ( ptElem[1].m_z > ptTest.m_z )
					continue;
		
				if ( ((( ptTest.m_z - pt.m_z ) > 0) && ( ptTest.m_z - pt.m_z ) > RESOURCE_Z_CHECK ) || ((( pt.m_z - ptTest.m_z ) < 0) && (( pt.m_z - ptTest.m_z ) < - RESOURCE_Z_CHECK )))
					continue;
	
				if (( ptTest.m_z < ptElem[1].m_z ) || (( ptTest.m_z == ptElem[1].m_z ) && ( fElem[1] )))
					continue;
		        //DEBUG_ERR(("pMultiItem->GetDispID()%x\n",pMultiItem->GetDispID()));
				ptElem[1] = ptTest;
				fElem[1] = false;

				//DEBUG_ERR(("multi pItemDef->IsType( iType %d) %d\n",iType,pItemDef->IsType( iType )));
				if ( pItemDef->IsType( iType ) )
				{
					fElem[1] = true;
					//if ( ptElem[iRetElem].m_z > ptElem[1].m_z )
					if ( ptElem[1].m_z > ptElem[iRetElem].m_z )
						iRetElem = 1;
				}

				//DEBUG_ERR(( "DISPID: %x X %d Y %d Z %d\n", pMultiItem->GetDispID(), (pMultiItem->m_dx), (pMultiItem->m_dy), (pMultiItem->m_dz) ));
			}
		}
	}

	// STATIC - checks one 8x8 block
	const CGrayMapBlock * pMapBlock = GetMapBlock( pt );
	ASSERT( pMapBlock );

	iQty = pMapBlock->m_Statics.GetStaticQty();
	if ( iQty )  // no static items here.
	{
		int x2 = pMapBlock->GetOffsetX(pt.m_x);
		int y2 = pMapBlock->GetOffsetY(pt.m_y);
		const CUOStaticItemRec * pStatic = NULL;

		for ( int i=0; i < iQty; ++i, pStatic = NULL, Height = 0, pItemDef = NULL )
		{
			pStatic = pMapBlock->m_Statics.GetStatic( i );

			ptTest = CPointMap( pStatic->m_x + pMapBlock->m_x, pStatic->m_y + pMapBlock->m_y, pStatic->m_z, pt.m_map );

			pItemDef = CItemBase::FindItemBase( pStatic->GetDispID() );
			if ( pItemDef == NULL )
				continue;

			//DEBUG_ERR(("pStatic->GetDispID() %d; name %s; pStatic->m_z %d\n",pStatic->GetDispID(),pItemDef->GetName(),pStatic->m_z));
			Height = pItemDef->GetHeight();
			if ( pItemDef->GetID() != pStatic->GetDispID() ) //not a parent item
			{
				pDupeDef = CItemBaseDupe::GetDupeRef((ITEMID_TYPE) pStatic->GetDispID() );
				if ( ! pDupeDef )
				{
					g_Log.EventDebug("Failed to get non-parent reference (static) (DispID 0%x) (X: %d Y: %d Z: %d)\n",pStatic->GetDispID(),ptTest.m_x,ptTest.m_y,ptTest.m_z);
					Height = pItemDef->GetHeight();
				}
				else
					Height = pDupeDef->GetHeight();
			}
			ptTest.m_z += Height; //height + current position = the top point

			if ( pt.GetDist( ptTest ) > iDistance )
				continue;

			if ( pt.GetDist( ptTest ) > iDistance )
				continue;

			//if ( ptElem[2].m_z > pStatic->m_z )
			if ( ptElem[2].m_z > ptTest.m_z )
				continue;

			if ( ((( pStatic->m_z - pt.m_z ) > 0) && ( pStatic->m_z - pt.m_z ) > RESOURCE_Z_CHECK ) || ((( pt.m_z - pStatic->m_z ) < 0) && (( pt.m_z - pStatic->m_z ) < - RESOURCE_Z_CHECK )))
				continue;

			if (( ptTest.m_z < ptElem[2].m_z ) || (( ptTest.m_z == ptElem[2].m_z ) && ( fElem[2] )))
				continue;

			ptElem[2] = ptTest;
			fElem[2] = false;

			//DEBUG_ERR(("static pItemDef->IsType( iType %d) %d;pItemDef->GetType() %d;pItemDef->GetID() %d;pItemDef->GetDispID() %d\n",iType,pItemDef->IsType( iType ),pItemDef->GetType(),pItemDef->GetID(),pItemDef->GetDispID()));
			if ( pItemDef->IsType( iType ) )
			{
				//DEBUG_ERR(("found %d; ptTest: %d,%d,%d\n",__LINE__,ptTest.m_x,ptTest.m_y,ptTest.m_z));
				fElem[2] = true;
				//DEBUG_ERR(("ptElem[iRetElem].m_z %d, ptElem[2].m_z %d\n",ptElem[iRetElem].m_z,ptElem[2].m_z));
				//if ( ptElem[iRetElem].m_z > ptElem[2].m_z )
				if ( ptElem[2].m_z > ptElem[iRetElem].m_z )
					iRetElem = 2;
			}
		}
	}

	// Check for appropriate terrain type
	CRectMap rect;
	rect.SetRect( pt.m_x - iDistance, pt.m_y - iDistance,
		pt.m_x + iDistance + 1, pt.m_y + iDistance + 1,
		pt.m_map);

	const CUOMapMeter * pMeter = NULL;
	for ( int x = rect.m_left; x < rect.m_right; ++x, pMeter = NULL )
	{
		for ( int y = rect.m_top; y < rect.m_bottom; ++y, pMeter = NULL )
		{
			ptTest = CPointMap( x, y, pt.m_z, pt.m_map);
			pMeter = GetMapMeter(ptTest);
			if ( !pMeter )
				continue;
			if ( pt.GetDist( ptTest ) > iDistance )
				continue;
			if ( ptElem[3].m_z > pMeter->m_z )
				continue;

			//DEBUG_ERR(("(( pMeter->m_z (%d) - pt.m_z (%d) ) > 0) && ( pMeter->m_z (%d) - pt.m_z (%d) ) > RESOURCE_Z_CHECK (%d) >> %d\n",pMeter->m_z,pt.m_z,pMeter->m_z,pt.m_z,RESOURCE_Z_CHECK,(( pMeter->m_z - pt.m_z ) > 0) && ( pMeter->m_z - pt.m_z ) > RESOURCE_Z_CHECK));
			//DEBUG_ERR(("(( pt.m_z (%d) - pMeter->m_z (%d) ) < 0) && (( pt.m_z (%d) - pMeter->m_z (%d) ) < - RESOURCE_Z_CHECK (%d) )) >> %d\n",pt.m_z,pMeter->m_z,pt.m_z,pMeter->m_z,- RESOURCE_Z_CHECK,((( pt.m_z - pMeter->m_z ) < 0) && (( pt.m_z - pMeter->m_z ) < - RESOURCE_Z_CHECK ))));
			if ( ((( pMeter->m_z - pt.m_z ) > 0) && ( pMeter->m_z - pt.m_z ) > RESOURCE_Z_CHECK ) || ((( pt.m_z - pMeter->m_z ) < 0) && (( pt.m_z - pMeter->m_z ) < - RESOURCE_Z_CHECK )))
				continue;

			//DEBUG_ERR(("pMeter->m_z (%d) < ptElem[3].m_z (%d) >> %d\n",pMeter->m_z,ptElem[3].m_z,pMeter->m_z < ptElem[3].m_z));
			if (( pMeter->m_z < ptElem[3].m_z ) || (( pMeter->m_z == ptElem[3].m_z ) && ( fElem[3] )))
				continue;

			ptElem[3] = ptTest;
			fElem[3] = false;

			//DEBUG_ERR(("iType %x, TerrainType %x\n",iType,g_World.GetTerrainItemType( pMeter->m_wTerrainIndex )));
			if ( iType == g_World.GetTerrainItemType( pMeter->m_wTerrainIndex ) )
			{
				fElem[3] = true;
				//if ( ptElem[iRetElem].m_z > ptElem[3].m_z )
				//DEBUG_ERR(("ptElem[3].m_z %d; ptElem[iRetElem].m_z %d\n",ptElem[3].m_z, ptElem[iRetElem].m_z));
				if ( ptElem[3].m_z > ptElem[iRetElem].m_z )
						iRetElem = 3;
				//DEBUG_ERR(("fElem3 %d %d\n",ptElem[3].m_z, fElem[3]));
				continue;
			}

			rect.SetRect( pt.m_x - iDistance, pt.m_y - iDistance, pt.m_x + iDistance + 1, pt.m_y + iDistance + 1, pt.m_map);
		}
	}

	/*CPointMap a;
	a.m_z = max(
	// priority dynamic->multi->static->terrain
	int iRetElem;
	if (( ptElem[0].m_z >= ptElem[1].m_z ) && ( fElem[0] ))
		iRetElem = 0;
	else if (( ptElem[1].m_z >= ptElem[2].m_z ) && ( fElem[1] ))
		iRetElem = 1;
	else if (( ptElem[2].m_z >= ptElem[3].m_z ) && ( fElem[2] ))
		iRetElem = 2;
	else if ( fElem[3] )
		iRetElem = 3;
	else
		iRetElem = 4;*/

	     if ( 0 != iRetElem && ptElem[0].m_z > ptElem[iRetElem].m_z )
			 iRetElem = 4;
	else if ( 1 != iRetElem && ptElem[1].m_z > ptElem[iRetElem].m_z )
		     iRetElem = 4;
	else if ( 2 != iRetElem && ptElem[2].m_z > ptElem[iRetElem].m_z )
		     iRetElem = 4;
	else if ( 3 != iRetElem && ptElem[3].m_z > ptElem[iRetElem].m_z )
		     iRetElem = 4;

	//DEBUG_ERR(("iRetElem %d; %d %d %d %d; %d %d %d ISVALID: %d\n",iRetElem,ptElem[1].m_z,ptElem[2].m_z,ptElem[3].m_z,ptElem[4].m_z,pt.m_x,pt.m_y,pt.m_z,ptElem[iRetElem].IsValidPoint()));
	//DEBUG_ERR(("X: %d  Y: %d  Z: %d\n",ptElem[iRetElem].m_x,ptElem[iRetElem].m_y,ptElem[iRetElem].m_z));
	return ( ptElem[iRetElem] );
#undef RESOURCE_Z_CHECK
}

bool CWorld::IsItemTypeNear( const CPointMap & pt, IT_TYPE iType, int iDistance, bool bCheckMulti )
{
	ADDTOCALLSTACK("CWorld::IsItemTypeNear");
	if ( !pt.IsValidPoint() )
		return false;
	CPointMap ptn = FindItemTypeNearby( pt, iType, iDistance, bCheckMulti );
	return( ptn.IsValidPoint());
}

CPointMap CWorld::FindItemTypeNearby(const CPointMap & pt, IT_TYPE iType, int iDistance, bool bCheckMulti, bool bLimitZ)
{
	ADDTOCALLSTACK("CWorld::FindItemTypeNearby");
	// Find the closest item of this type.
	// This does not mean that i can touch it.
	// ARGS:
	//   iDistance = 2d distance to search.

	CPointMap ptFound;
	int iTestDistance;

		
	// Check dynamics first since they are the easiest.
	CWorldSearch Area( pt, iDistance );
	while (true)
	{
		CItem * pItem = Area.GetItem();
		if ( pItem == NULL )
			break;

		if ( ! pItem->IsType( iType ) && ! pItem->Item_GetDef()->IsType(iType) )
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


	// Check for statics
	rect.SetRect( pt.m_x - iDistance, pt.m_y - iDistance,
		pt.m_x + iDistance + 1, pt.m_y + iDistance + 1,
		pt.m_map);

	const CGrayMapBlock * pMapBlock = NULL;
	const CUOStaticItemRec * pStatic = NULL;
	const CItemBase * pItemDef = NULL;
	int iQty = 0;

	for (int x = rect.m_left; x < rect.m_right; x += UO_BLOCK_SIZE, pMapBlock = NULL )
	{
		for ( int y = rect.m_top; y < rect.m_bottom; y += UO_BLOCK_SIZE, pMapBlock = NULL )
		{
			CPointMap ptTest( x, y, pt.m_z, pt.m_map);
			pMapBlock = GetMapBlock( ptTest );

			if ( !pMapBlock )
				continue;
			
			iQty = 0;
			iQty = pMapBlock->m_Statics.GetStaticQty();

			if ( !iQty )
				continue;

			pStatic = NULL; 
			pItemDef = NULL;

			for ( int i=0; i < iQty; i++, pStatic = NULL, pItemDef = NULL )
			{
				pStatic = pMapBlock->m_Statics.GetStatic( i );
				if ( bLimitZ && ( pStatic->m_z != ptTest.m_z ) )
					continue;

				// inside the range we want ?
				CPointMap ptStatic( pStatic->m_x+pMapBlock->m_x, pStatic->m_y+pMapBlock->m_y, pStatic->m_z, ptTest.m_map);
				iTestDistance = pt.GetDist(ptStatic);
				if ( iTestDistance > iDistance )
					continue;

				ITEMID_TYPE idTile = pStatic->GetDispID();

				// Check the script def for the item.
				pItemDef = CItemBase::FindItemBase( idTile );
				if ( pItemDef == NULL )
					continue;
				if ( ! pItemDef->IsType( iType ))
					continue;

				ptFound = ptStatic;
				iDistance = iTestDistance;
				if ( ! iDistance )
					return( ptFound );

				rect.SetRect( pt.m_x - iDistance, pt.m_y - iDistance,
					pt.m_x + iDistance + 1, pt.m_y + iDistance + 1,
					pt.m_map);
			}
		}
	}

	// Check for multi components
	if (bCheckMulti == true)
	{
		rect.SetRect( pt.m_x - iDistance, pt.m_y - iDistance,
			pt.m_x + iDistance + 1, pt.m_y + iDistance + 1,
			pt.m_map);

		for (int x = rect.m_left; x < rect.m_right; x++)
		{
			for (int y = rect.m_top; y < rect.m_bottom; y++)
			{
				CPointMap ptTest(x, y, pt.m_z, pt.m_map);

				CRegionLinks rlinks;
				if ( int iQty = ptTest.GetRegions(REGION_TYPE_MULTI, rlinks) )
				{
					for (int i = 0; i < iQty; i++)
					{
						CRegionBase* pRegion = rlinks.GetAt(i);
						CItem* pItem = pRegion->GetResourceID().ItemFind();
						if (pItem == NULL)
							continue;

						const CGrayMulti* pMulti;
						CItemMultiCustom * pItemMulti = dynamic_cast<CItemMultiCustom*>( pItem );
						if ( pItemMulti == NULL )
							pMulti = g_Cfg.GetMultiItemDefs( pItem->GetDispID());
						else
							pMulti = pItemMulti->GetMultiItemDefs();

						if (pMulti == NULL)
							continue;

						int x2 = ptTest.m_x - pItem->GetTopPoint().m_x;
						int y2 = ptTest.m_y - pItem->GetTopPoint().m_y;
						int z2 = ptTest.m_z - pItem->GetTopPoint().m_z;

						int iItemQty = pMulti->GetItemCount();
						for (int i = 0; i < iItemQty; i++)
						{
							const CUOMultiItemRec* pMultiItem = pMulti->GetItem(i);
							ASSERT(pMultiItem);

							if ( !pMultiItem->m_visible )
								continue;
							if ( pMultiItem->m_dx != x2 || pMultiItem->m_dy != y2 )
								continue;
							if ( bLimitZ && (pMultiItem->m_dz != ptTest.m_z))
								continue;

							iTestDistance = pt.GetDist(ptTest);
							if (iTestDistance > iDistance)
								continue;

							ITEMID_TYPE idTile = pMultiItem->GetDispID();

							// Check the script def for the item.
							pItemDef = CItemBase::FindItemBase(idTile);
							if (pItemDef == NULL || !pItemDef->IsType(iType))
								continue;

							ptFound = ptTest;
							iDistance = iTestDistance;
							if ( !iDistance )
								return( ptFound );

							rect.SetRect( pt.m_x - iDistance, pt.m_y - iDistance,
										  pt.m_x + iDistance + 1, pt.m_y + iDistance + 1,
										  pt.m_map);
						}
					}
				}
			}
		}
	}

	return ptFound;
}

//****************************************************

void CWorld::GetHeightPoint( const CPointMap & pt, CGrayMapBlockState & block, bool fHouseCheck )
{
	ADDTOCALLSTACK("CWorld::GetHeightPoint");
	// Height of statics at/above given coordinates
	// do gravity here for the z.

	WORD wBlockThis = 0;
	const CGrayMapBlock * pMapBlock = GetMapBlock( pt );
	if ( !pMapBlock )
	{
		g_Log.EventWarn("GetMapBlock failed at %s.\n", pt.WriteUsed());
		return;
	}

	{
		int iQty = pMapBlock->m_Statics.GetStaticQty();
		if ( iQty )  // no static items here.
		{
			int x2=pMapBlock->GetOffsetX(pt.m_x);
			int y2=pMapBlock->GetOffsetY(pt.m_y);
			for ( int i=0; i<iQty; i++ )
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
	}

	// Any multi items here ?
	if ( fHouseCheck )
	{
		CRegionLinks rlinks;
		if ( int iQty = pt.GetRegions( REGION_TYPE_MULTI, rlinks ) )
		{
			for ( int i = 0; i < iQty; i++)
			{
				CRegionBase * pRegion = rlinks.GetAt(i);
				CItem * pItem = pRegion->GetResourceID().ItemFind();
				if ( pItem != NULL )
				{
					const CGrayMulti * pMulti;
					CItemMultiCustom * pItemMulti = dynamic_cast<CItemMultiCustom*>( pItem );
					if ( pItemMulti == NULL )
						pMulti = g_Cfg.GetMultiItemDefs( pItem->GetDispID());
					else
						pMulti = pItemMulti->GetMultiItemDefs();

					if ( pMulti )
					{
						int x2 = pt.m_x - pItem->GetTopPoint().m_x;
						int y2 = pt.m_y - pItem->GetTopPoint().m_y;

						int iQty = pMulti->GetItemCount();
						for ( int i=0; i < iQty; i++ )
						{
							const CUOMultiItemRec * pMultiItem = pMulti->GetItem(i);
							ASSERT(pMultiItem);

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
	}

	{
	// Any dynamic items here ?
	// NOTE: This could just be an item that an NPC could just move ?
	CWorldSearch Area( pt );
	while (true)
	{
		CItem * pItem = Area.GetItem();
		if ( pItem == NULL )
			break;

		signed char zitem = pItem->GetTopZ();
		if ( ! block.IsUsableZ(zitem,PLAYER_HEIGHT))
			continue;

		// Invis items should not block ???
		CItemBase * pItemDef = pItem->Item_GetDef();
		ASSERT(pItemDef);
		if ( !block.CheckTile( 
			pItemDef->m_Can & ( CAN_I_DOOR | CAN_I_WATER | CAN_I_CLIMB | CAN_I_BLOCK | CAN_I_PLATFORM ),
			zitem, pItemDef->GetHeight(), pItemDef->GetDispID() + TERRAIN_QTY ) )
		{
		}
	}
	}

	// Check Terrain here.
	// Terrain height is screwed. Since it is related to all the terrain around it.

	{
	const CUOMapMeter * pMeter = pMapBlock->GetTerrain( UO_BLOCK_OFFSET(pt.m_x), UO_BLOCK_OFFSET(pt.m_y));
	ASSERT(pMeter);

	if ( block.IsUsableZ(pMeter->m_z,0))
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
	}

	if ( block.m_Bottom.m_z == UO_SIZE_MIN_Z )
	{
		block.m_Bottom = block.m_Lowest;
	}
}

signed char CWorld::GetHeightPoint( const CPointBase & pt, WORD & wBlockFlags, bool fHouseCheck ) // Height of player who walked to X/Y/OLDZ
{
	ADDTOCALLSTACK("CWorld::GetHeightPoint");
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

	if (( block.m_Lowest.m_dwBlockFlags & CAN_I_HOVER ) || ( block.m_Bottom.m_dwBlockFlags & CAN_I_HOVER ) || ( block.m_Top.m_dwBlockFlags & CAN_I_HOVER ))
	{
		if ( wCan & CAN_C_HOVER )
			wBlockFlags = 0; // we can hover over this
		else
			wBlockFlags &= ~CAN_I_HOVER; // we don't have the ability to fly
	}

	if (( wBlockFlags & ( CAN_I_CLIMB|CAN_I_PLATFORM) ) && ( wCan & CAN_C_WALK ))
	{
		wBlockFlags &= ~CAN_I_CLIMB;
		wBlockFlags |= CAN_I_PLATFORM;	// not really true but hack it anyhow.
		return( block.m_Bottom.m_z );
	}

	if ( wCan & CAN_C_FLY )
		return( pt.m_z );

	return( block.m_Bottom.m_z );
}

void CWorld::GetHeightPoint_New( const CPointMap & pt, CGrayMapBlockState & block, bool fHouseCheck )
{
	ADDTOCALLSTACK("CWorld::GetHeightPoint_New");
	CItemBase * pItemDef = NULL;
	CItemBaseDupe * pDupeDef = NULL;
	CItem * pItem = NULL;
	WORD wBlockThis = 0;
	signed char z = 0;
	signed char zHeight = 0;
	int x2, y2, i, ii, iQty = 0;

	// Height of statics at/above given coordinates
	// do gravity here for the z.
	const CGrayMapBlock * pMapBlock = GetMapBlock( pt );
	if ( !pMapBlock)
		return;

	iQty = pMapBlock->m_Statics.GetStaticQty();
	if ( iQty )  // no static items here.
	{
		x2 = pMapBlock->GetOffsetX(pt.m_x);
		y2 = pMapBlock->GetOffsetY(pt.m_y);
		const CUOStaticItemRec * pStatic = NULL;
		for ( i = 0; i < iQty; ++i, z = 0, zHeight = 0, pStatic = NULL, pDupeDef = NULL )
		{
			if ( !pMapBlock )
				break;

			if ( ! pMapBlock->m_Statics.IsStaticPoint( i, x2, y2 ))
				continue;

			pStatic = pMapBlock->m_Statics.GetStatic( i );
			if ( ! pStatic )
				continue;

			z = pStatic->m_z;

			//DEBUG_ERR(("z (%d)  block.m_zHeight (%d) block.m_Bottom.m_z (%d)\n",z,block.m_zHeight,block.m_Bottom.m_z));
			if ( ! block.IsUsableZ( z, block.m_zHeight ))
				continue;

			// This static is at the coordinates in question.
			// enough room for me to stand here ?

			if (IsSetEF( EF_NewPositionChecks ))
			{
				pItemDef = CItemBase::FindItemBase( pStatic->GetDispID() );
				if ( pItemDef )
				{
					//DEBUG_ERR(("pItemDef->GetID(0%x) pItemDef->GetDispID(0%x) pStatic->GetDispID(0%x)\n",pItemDef->GetID(),pItemDef->GetDispID(),pStatic->GetDispID()));
					if ( pItemDef->GetID() == pStatic->GetDispID() ) //parent item
					{
						zHeight = pItemDef->GetHeight();
						wBlockThis = ( pItemDef->m_Can & (CAN_C_GHOST|CAN_C_SWIM|CAN_C_WALK|CAN_C_PASSWALLS|CAN_C_FLY|CAN_C_FIRE_IMMUNE|CAN_C_INDOORS|CAN_C_HOVER) ); //Use only Block flags, other remove
					}
					else //non-parent item
					{
						pDupeDef = CItemBaseDupe::GetDupeRef((ITEMID_TYPE) pStatic->GetDispID());
						if ( ! pDupeDef )
						{
							g_Log.EventDebug("Failed to get non-parent reference (static) (DispID 0%x) (X: %d Y: %d Z: %d)\n",pStatic->GetDispID(),pStatic->m_x+pMapBlock->m_x,pStatic->m_y+pMapBlock->m_y,pStatic->m_z);
							zHeight = pItemDef->GetHeight();
							wBlockThis = ( pItemDef->m_Can & (CAN_C_GHOST|CAN_C_SWIM|CAN_C_WALK|CAN_C_PASSWALLS|CAN_C_FLY|CAN_C_FIRE_IMMUNE|CAN_C_INDOORS|CAN_C_HOVER) );
						}
						else
						{
							zHeight = pDupeDef->GetHeight();
							wBlockThis = ( pDupeDef->m_Can & (CAN_C_GHOST|CAN_C_SWIM|CAN_C_WALK|CAN_C_PASSWALLS|CAN_C_FLY|CAN_C_FIRE_IMMUNE|CAN_C_INDOORS|CAN_C_HOVER) ); //Use only Block flags, other remove - CAN flags cannot be inherited from the parent item due to bad script pack...
						}
					}
				}
				else if ( pStatic->GetDispID() )
				{
					//DEBUG_ERR(("Item (0%x) has no definition in scripts.\n",pStatic->GetDispID()));
					wBlockThis = 0;
				}
			}
			else
			{
				wBlockThis = 0;
				zHeight = CItemBase::GetItemHeight( pStatic->GetDispID(), wBlockThis );
			}
			//DEBUG_ERR(("STATIC DispID (0%x) z (%d) zHeight (%d) wBlockThis (0%x)\n",pStatic->GetDispID(),z,zHeight,wBlockThis));
			block.CheckTile_Item( wBlockThis, z, zHeight, pStatic->GetDispID() + TERRAIN_QTY );
		}
	}

	pItemDef = NULL;
	pDupeDef = NULL;
	pItem = NULL;
	wBlockThis = z = zHeight = x2 = y2 = iQty = i = ii = 0;

	// Any multi items here ?
	// Check all of them
	if ( fHouseCheck )
	{
		CRegionLinks rlinks;
		int iQtyr = pt.GetRegions( REGION_TYPE_MULTI, rlinks );
		if ( iQtyr )
		{
			//  ------------ For variables --------------------
			CRegionBase * pRegion = NULL;
			const CGrayMulti * pMulti = NULL;
			const CUOMultiItemRec * pMultiItem = NULL;
			x2 = 0;
			y2 = 0;
			//  ------------ For variables --------------------

			for ( i = 0; i < iQtyr; ++i, pRegion = NULL, pItem = NULL, pMulti = NULL, x2 = 0, y2 = 0 )
			{
				pRegion = rlinks.GetAt(i);
				if ( pRegion )
					pItem = pRegion->GetResourceID().ItemFind();

				if ( pItem != NULL )
				{
					CItemMultiCustom * pItemMulti = dynamic_cast<CItemMultiCustom*>( pItem );
					if ( pItemMulti == NULL )
						pMulti = g_Cfg.GetMultiItemDefs( pItem->GetDispID());
					else
						pMulti = pItemMulti->GetMultiItemDefs();

					if ( pMulti )
					{
						x2 = pt.m_x - pItem->GetTopPoint().m_x;
						y2 = pt.m_y - pItem->GetTopPoint().m_y;

						iQty = pMulti->GetItemCount();
						for ( ii = 0; ii < iQty; ++ii, pMultiItem = NULL, z = 0, zHeight = 0 )
						{
							pMultiItem = pMulti->GetItem(ii);

							if ( !pMultiItem )
								break;

							if ( ! pMultiItem->m_visible )
								continue;

							if ( pMultiItem->m_dx != x2 || pMultiItem->m_dy != y2 )
								continue;

							z = pItem->GetTopZ() + pMultiItem->m_dz;
							if ( ! block.IsUsableZ(z,PLAYER_HEIGHT))
								continue;

							if (IsSetEF( EF_NewPositionChecks ))
							{
								pItemDef = CItemBase::FindItemBase( pMultiItem->GetDispID() );
								if ( pItemDef )
								{
									if ( pItemDef->GetID() == pMultiItem->GetDispID() ) //parent item
									{
										zHeight = pItemDef->GetHeight();
										wBlockThis = ( pItemDef->m_Can & (CAN_C_GHOST|CAN_C_SWIM|CAN_C_WALK|CAN_C_PASSWALLS|CAN_C_FLY|CAN_C_FIRE_IMMUNE|CAN_C_INDOORS|CAN_C_HOVER) ); //Use only Block flags, other remove
									}
									else //non-parent item
									{
										pDupeDef = CItemBaseDupe::GetDupeRef((ITEMID_TYPE) pMultiItem->GetDispID());
										if ( ! pDupeDef )
										{
											g_Log.EventDebug("Failed to get non-parent reference (multi) (DispID 0%x) (X: %d Y: %d Z: %d)\n",pMultiItem->GetDispID(),pMultiItem->m_dx+pItem->GetTopPoint().m_x,pMultiItem->m_dy+pItem->GetTopPoint().m_y,pMultiItem->m_dz+pItem->GetTopPoint().m_z);
											zHeight = pDupeDef->GetHeight();
											wBlockThis = ( pDupeDef->m_Can & (CAN_C_GHOST|CAN_C_SWIM|CAN_C_WALK|CAN_C_PASSWALLS|CAN_C_FLY|CAN_C_FIRE_IMMUNE|CAN_C_INDOORS|CAN_C_HOVER) ); 
										}
										else
										{
											zHeight = pDupeDef->GetHeight();
											wBlockThis = ( pDupeDef->m_Can & (CAN_C_GHOST|CAN_C_SWIM|CAN_C_WALK|CAN_C_PASSWALLS|CAN_C_FLY|CAN_C_FIRE_IMMUNE|CAN_C_INDOORS|CAN_C_HOVER) ); //Use only Block flags, other remove - CAN flags cannot be inherited from the parent item due to bad script pack...
										}
									}
								}
								else if ( pMultiItem->GetDispID() )
								{
									DEBUG_ERR(("Item (0%x) has no definition in scripts.\n",pMultiItem->GetDispID()));
									wBlockThis = 0;
								}
								//DEBUG_WARN(("wBlockThis (0%x) zHeight(%d) zitem(%d) pItem->GetTopZ()(%d) pMultiItem->m_dz(%d),pMultiItem->GetDispID()(0%x)\n",wBlockThis,zHeight,zitem,pItem->GetTopZ(),pMultiItem->m_dz,pMultiItem->GetDispID()));
							}
							else
							{
								wBlockThis = 0;
								zHeight = CItemBase::GetItemHeight( pMultiItem->GetDispID(), wBlockThis );
							}
							//DEBUG_WARN(("MULTI wBlockThis (0%x)\n",wBlockThis));
							block.CheckTile_Item( wBlockThis, z, zHeight, pMultiItem->GetDispID() + TERRAIN_QTY );
						}
					}
				}
			}
		}
	}

	pItemDef = NULL;
	pDupeDef = NULL;
	pItem = NULL;
	wBlockThis = z = zHeight = x2 = y2 = iQty = i = ii = 0;

	// Any dynamic items here ?
	// NOTE: This could just be an item that an NPC could just move ?
	CWorldSearch Area( pt );

	while ( true )
	{
		pItem = Area.GetItem();
		if ( !pItem )
			break;

		z = pItem->GetTopZ();
		if ( !block.IsUsableZ( z, block.m_zHeight ) )
			continue;

		// Invis items should not block ???
		pItemDef = CItemBase::FindItemBase( pItem->GetDispID() );

		if (IsSetEF( EF_NewPositionChecks ))
		{
			if ( pItemDef )
			{
				if ( pItemDef->GetDispID() == pItem->GetDispID() )//parent item
				{
					zHeight = pItemDef->GetHeight();
					wBlockThis = ( pItemDef->m_Can & (CAN_C_GHOST|CAN_C_SWIM|CAN_C_WALK|CAN_C_PASSWALLS|CAN_C_FLY|CAN_C_FIRE_IMMUNE|CAN_C_INDOORS|CAN_C_HOVER) ); //Use only Block flags, other remove
				}
				else //non-parent item
				{
					pDupeDef = CItemBaseDupe::GetDupeRef((ITEMID_TYPE) pItem->GetDispID());
					if ( ! pDupeDef )
					{
						g_Log.EventDebug("Failed to get non-parent reference (dynamic) (DispID 0%x) (X: %d Y: %d Z: %d)\n",pItem->GetDispID(),pItem->GetTopPoint().m_x,pItem->GetTopPoint().m_y,pItem->GetTopPoint().m_z);
						zHeight = pItemDef->GetHeight();
						wBlockThis = ( pItemDef->m_Can & (CAN_C_GHOST|CAN_C_SWIM|CAN_C_WALK|CAN_C_PASSWALLS|CAN_C_FLY|CAN_C_FIRE_IMMUNE|CAN_C_INDOORS|CAN_C_HOVER) );
					}
					else
					{
						zHeight = pDupeDef->GetHeight();
						wBlockThis = ( pDupeDef->m_Can & (CAN_C_GHOST|CAN_C_SWIM|CAN_C_WALK|CAN_C_PASSWALLS|CAN_C_FLY|CAN_C_FIRE_IMMUNE|CAN_C_INDOORS|CAN_C_HOVER) ); //Use only Block flags, other remove - CAN flags cannot be inherited from the parent item due to bad script pack...
					}
				}
			}
			else
			{
				DEBUG_ERR(("Item (0%x) has no definition in scripts.\n",pItem->GetDispID()));
				wBlockThis = 0;
			}
		}
		else
		{
			wBlockThis = 0;
			zHeight = CItemBase::GetItemHeight( pItem->GetDispID(), wBlockThis );
		}

		//DEBUG_WARN(("DYNAMIC wBlockThis (0%x)\n",pItemDef->m_Can & (CAN_I_DOOR|CAN_I_WATER|CAN_I_CLIMB|CAN_I_BLOCK|CAN_I_PLATFORM)));
		if ( !block.CheckTile_Item( wBlockThis,	z, zHeight, pItemDef->GetID() + TERRAIN_QTY) )
		{
		}
	}

	wBlockThis = 0;
	// Terrain height is screwed. Since it is related to all the terrain around it.
	const CUOMapMeter * pMeter = pMapBlock->GetTerrain( UO_BLOCK_OFFSET(pt.m_x), UO_BLOCK_OFFSET(pt.m_y));
	if ( ! pMeter )
		return;

	if ( block.IsUsableZ( pMeter->m_z,0 ) )
	{
		//DEBUG_ERR(("pMeter->m_wTerrainIndex 0%x wBlockThis (0%x)\n",pMeter->m_wTerrainIndex,wBlockThis));
		if ( pMeter->m_wTerrainIndex == TERRAIN_HOLE )
		{
			wBlockThis = 0;
		}
		else if ( CUOMapMeter::IsTerrainNull( pMeter->m_wTerrainIndex ) )	// inter dungeon type.
		{
			wBlockThis = CAN_I_BLOCK;
		}
		else
		{
			CGrayTerrainInfo land( pMeter->m_wTerrainIndex );
			//DEBUG_ERR(("Terrain flags - land.m_flags 0%x wBlockThis (0%x)\n",land.m_flags,wBlockThis));
			if ( land.m_flags & UFLAG1_WATER )
				wBlockThis |= CAN_I_WATER;
			if ( land.m_flags & UFLAG1_DAMAGE )
				wBlockThis |= CAN_I_FIRE;
			if ( land.m_flags & UFLAG1_BLOCK )
				wBlockThis |= CAN_I_BLOCK;
			if (( ! wBlockThis ) || ( land.m_flags & UFLAG2_PLATFORM )) // Platform items should take precendence over non-platforms.
				wBlockThis = CAN_I_PLATFORM;
		}
		//DEBUG_ERR(("TERRAIN wBlockThis (0%x)\n",wBlockThis));
		block.CheckTile_Terrain( wBlockThis, pMeter->m_z, pMeter->m_wTerrainIndex );
	}

	if ( block.m_Bottom.m_z == UO_SIZE_MIN_Z )
		block.m_Bottom = block.m_Lowest;
}

signed char CWorld::GetHeightPoint_New( const CPointBase & pt, WORD & wBlockFlags, bool fHouseCheck )
{
	ADDTOCALLSTACK("CWorld::GetHeightPoint_New");
	WORD wCan = wBlockFlags;
	CGrayMapBlockState block( wBlockFlags, pt.m_z + (PLAYER_HEIGHT / 2), pt.m_z + PLAYER_HEIGHT );

	GetHeightPoint_New( pt, block, fHouseCheck );

	// Pass along my results.
	wBlockFlags = block.m_Bottom.m_dwBlockFlags;
	if ( block.m_Top.m_dwBlockFlags )
		wBlockFlags |= CAN_I_ROOF;	// we are covered by something.

	if (( block.m_Lowest.m_dwBlockFlags & CAN_I_HOVER ) || ( block.m_Bottom.m_dwBlockFlags & CAN_I_HOVER ) || ( block.m_Top.m_dwBlockFlags & CAN_I_HOVER ))
	{
		if ( wCan & CAN_C_HOVER )
			wBlockFlags = 0; // we can hover over this
		else
			wBlockFlags &= ~CAN_I_HOVER; // we don't have the ability to fly
	}

	if (( wBlockFlags & ( CAN_I_CLIMB|CAN_I_PLATFORM) ) && ( wCan & CAN_C_WALK ))
	{
		wBlockFlags &= ~CAN_I_CLIMB;
		wBlockFlags |= CAN_I_PLATFORM;	// not really true but hack it anyhow.
		//DEBUG_MSG(("block.m_Bottom.m_z (%d)\n",block.m_Bottom.m_z));
		return( block.m_Bottom.m_z );
	}
	if ( wCan & CAN_C_FLY )
		return( pt.m_z );

	return( block.m_Bottom.m_z );
}


CItemTypeDef *	CWorld::GetTerrainItemTypeDef( DWORD dwTerrainIndex )
{
	ADDTOCALLSTACK("CWorld::GetTerrainItemTypeDef");
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
	ASSERT( pRes );

	CItemTypeDef *	pItemTypeDef	= dynamic_cast <CItemTypeDef*> (pRes);
	ASSERT( pItemTypeDef );

	return( pItemTypeDef );
}


IT_TYPE		CWorld::GetTerrainItemType( DWORD dwTerrainIndex )
{
	ADDTOCALLSTACK("CWorld::GetTerrainItemType");
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
