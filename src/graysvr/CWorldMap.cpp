#include "graysvr.h"	// predef header

CItem *CWorld::CheckNaturalResource(const CPointMap &pt, IT_TYPE type, bool fTest, CChar *pCharSrc)
{
	ADDTOCALLSTACK("CWorld::CheckNaturalResource");
	// Check natural resources at this location
	// RETURN: The resource tracking item (NULL = nothing here)
	if ( !pt.IsValidPoint() )
		return NULL;

	EXC_TRY("CheckNaturalResource");
	if ( fTest )
	{
		EXC_SET("check nearby");
		if ( (type != IT_TREE) && (type != IT_ROCK) )
		{
			if ( !g_World.IsTypeNear_Top(pt, type) )
				return NULL;
		}
		else
		{
			if ( !g_World.IsItemTypeNear(pt, type) )		// cannot be used, because it does no Z check... what if there is a static tile 70 tiles under me?
				return NULL;
		}
	}

	// Find the resource object
	EXC_SET("get worldgem bit");
	CItem *pResBit = NULL;
	CWorldSearch Area(pt);
	for (;;)
	{
		pResBit = Area.GetItem();
		if ( !pResBit )
			break;
		if ( pResBit->IsType(type) && (pResBit->GetID() == ITEMID_WorldGem) )
			break;
	}
	if ( pResBit )
		return pResBit;

	EXC_SET("get region");
	CRegionWorld *pRegion = dynamic_cast<CRegionWorld *>(pt.GetRegion(REGION_TYPE_AREA));
	if ( !pRegion )
		return NULL;

	CItem *pItem = NULL;
	CWorldSearch AreaItems(pt);
	AreaItems.SetAllShow(true);
	for (;;)
	{
		pItem = AreaItems.GetItem();
		if ( !pItem )
			break;
		if ( !pItem->IsType(type) )
			return NULL;
	}

	// Just use the background (default) region for this
	if ( pRegion->m_Events.GetCount() <= 0 )
	{
		CPointMap ptZero(0, 0, 0, pt.m_map);
		pRegion = dynamic_cast<CRegionWorld *>(ptZero.GetRegion(REGION_TYPE_AREA));
		if ( !pRegion )
			return NULL;
	}

	// Find RES_REGIONTYPE
	EXC_SET("get resource group");
	const CRandGroupDef *pResGroup = pRegion->FindNaturalResource(type);
	if ( !pResGroup )
		return NULL;

	// Find RES_REGIONRESOURCE
	EXC_SET("get random group element");
	size_t id = pResGroup->GetRandMemberIndex(pCharSrc);
	CRegionResourceDef *pOreDef;
	if ( id != pResGroup->BadMemberIndex() )
		pOreDef = dynamic_cast<CRegionResourceDef *>(g_Cfg.ResourceGetDef(pResGroup->GetMemberID(id)));
	else
		pOreDef = dynamic_cast<CRegionResourceDef *>(g_Cfg.ResourceGetDefByName(RES_REGIONRESOURCE, "mr_nothing"));

	if ( !pOreDef )
		return NULL;

	EXC_SET("create worldgem bit");
	pResBit = CItem::CreateScript(ITEMID_WorldGem, pCharSrc);
	if ( !pResBit )
		return NULL;

	WORD wAmount = static_cast<WORD>(pOreDef->m_Amount.GetRandom());
	if ( (g_Cfg.m_iRacialFlags & RACIALF_HUMAN_WORKHORSE) && pCharSrc->IsHuman() )
	{
		if ( (type == IT_ROCK) && (pCharSrc->GetTopMap() == 0) )
			wAmount += 1;	// Workhorse racial trait, give +1 ore to humans in Felucca
		else if ( (type == IT_TREE) && (pCharSrc->GetTopMap() == 1) )
			wAmount += 2;	// Workhorse racial trait, give +2 logs to humans in Trammel
	}

	pResBit->SetType(type);
	pResBit->SetAttr(ATTR_INVIS|ATTR_MOVE_NEVER);
	pResBit->SetAmount(wAmount);
	pResBit->m_itResource.m_rid_res = pOreDef->GetResourceID();
	pResBit->MoveToDecay(pt, static_cast<INT64>(pOreDef->m_iRegenerateTime.GetRandom()) * TICK_PER_SEC);

	EXC_SET("resource found");
	if ( pCharSrc )
	{
		CScriptTriggerArgs Args(0, 0, pResBit);
		TRIGRET_TYPE tr = TRIGRET_RET_DEFAULT;
		if ( IsTrigUsed(TRIGGER_REGIONRESOURCEFOUND) )
			tr = pCharSrc->OnTrigger(CTRIG_RegionResourceFound, pCharSrc, &Args);
		if ( IsTrigUsed(TRIGGER_RESOURCEFOUND) )
			tr = pOreDef->OnTrigger("@ResourceFound", pCharSrc, &Args);

		if ( tr == TRIGRET_RET_TRUE )
		{
			if ( pResBit->IsDisconnected() )
				return NULL;
			pResBit->SetAmount(0);
		}
	}
	return pResBit;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("point '%hd,%hd,%hhd,%hhu' type '%d' [0%" FMTDWORDH "]\n", pt.m_x, pt.m_y, pt.m_z, pt.m_map, static_cast<int>(type), pCharSrc ? static_cast<DWORD>(pCharSrc->GetUID()) : 0);
	EXC_DEBUG_END;
	return NULL;
}

bool CWorld::IsTypeNear_Top(const CPointMap &pt, IT_TYPE type, int iDist)
{
	ADDTOCALLSTACK("CWorld::IsTypeNear_Top");
	if ( !pt.IsValidPoint() )
		return false;

	CPointMap ptTest = FindTypeNear_Top(pt, type, iDist);
	return ptTest.IsValidPoint();
}

CPointMap CWorld::FindTypeNear_Top(const CPointMap &pt, IT_TYPE type, int iDist)
{
	ADDTOCALLSTACK("CWorld::FindTypeNear_Top");
	#define RESOURCE_Z_CHECK 8

	BYTE z = 0;
	height_t iHeight = 0;
	CItem *pItem = NULL;
	CItemBase *pItemDef = NULL;
	CItemBaseDupe *pDupeDef = NULL;
	CPointMap ptTest;

	CPointMap ptElem[5];
	memset(ptElem, 0, sizeof(ptElem));
	ptElem[0].m_z = ptElem[1].m_z = ptElem[2].m_z = ptElem[3].m_z = UO_SIZE_MIN_Z;
	ptElem[4] = CPointMap(SHRT_MAX, SHRT_MAX, UO_SIZE_MIN_Z);
	bool fElem[4] = { false, false, false, false };
	size_t iRetElem = 4;

	// Check dynamic items
	CWorldSearch Area(pt, iDist);
	Area.SetAllShow(true);
	for (;;)
	{
		z = 0;
		iHeight = 0;
		pItem = Area.GetItem();
		if ( !pItem )
			break;

		if ( pt.GetDist(pItem->GetTopPoint()) > iDist )
			continue;

		pItemDef = CItemBase::FindItemBase(pItem->GetDispID());
		if ( !pItemDef )
			continue;

		iHeight = pItemDef->GetHeight();
		if ( pItemDef->GetID() != pItem->GetDispID() )		// not a parent item
		{
			pDupeDef = CItemBaseDupe::GetDupeRef(static_cast<ITEMID_TYPE>(pItem->GetDispID()));
			if ( pDupeDef )
				iHeight = pDupeDef->GetHeight();
			else
			{
				g_Log.EventDebug("Failed to get non-parent reference (dynamic) (DispID 0%x) (X:%hd Y:%hd Z:%hhd)\n", pItem->GetDispID(), ptTest.m_x, ptTest.m_y, ptTest.m_z);
				iHeight = pItemDef->GetHeight();
			}
		}

		int iTopZ = pItem->GetTopPoint().m_z + iHeight;
		if ( iTopZ > UO_SIZE_Z )
			iTopZ = UO_SIZE_Z;

		z = static_cast<BYTE>(iTopZ);
		if ( ptElem[0].m_z > z )
			continue;
		if ( (pt.m_z - z < -RESOURCE_Z_CHECK) || (z - pt.m_z > RESOURCE_Z_CHECK) )
			continue;
		if ( (z < ptElem[0].m_z) || ((z == ptElem[0].m_z) && fElem[0]) )
			continue;

		ptElem[0] = pItem->GetTopPoint();
		ptElem[0].m_z = z;
		fElem[0] = false;

		if ( pItem->IsType(type) )
		{
			fElem[0] = true;
			iRetElem = 0;
		}
	}

	// Check multi components
	CRegionLinks rlinks;
	size_t iRegionQty = pt.GetRegions(REGION_TYPE_MULTI, rlinks);
	if ( iRegionQty > 0 )
	{
		CRegionBase *pRegion = NULL;
		const CGrayMulti *pMulti = NULL;
		const CUOMultiItemRecHS *pMultiItem = NULL;
		for ( size_t iRegion = 0; iRegion < iRegionQty; ++iRegion )
		{
			pRegion = rlinks.GetAt(iRegion);
			pItem = pRegion->GetResourceID().ItemFind();
			if ( !pItem )
				continue;

			pMulti = g_Cfg.GetMultiItemDefs(pItem);
			if ( !pMulti )
				continue;

			size_t iMultiQty = pMulti->GetItemCount();
			for ( size_t iMulti = 0; iMulti < iMultiQty; ++iMulti )
			{
				pMultiItem = pMulti->GetItem(iMulti);
				if ( !pMultiItem )
					break;
				if ( !pMultiItem->m_visible )
					continue;

				pItemDef = CItemBase::FindItemBase(pMultiItem->GetDispID());
				if ( !pItemDef )
					continue;

				ptTest = CPointMap(pMultiItem->m_dx + pt.m_x, pMultiItem->m_dy + pt.m_y, static_cast<signed char>(pMultiItem->m_dz) + pt.m_z, pt.m_map);
				iHeight = pItemDef->GetHeight();
				if ( pItemDef->GetID() != pMultiItem->GetDispID() )		// not a parent item
				{
					pDupeDef = CItemBaseDupe::GetDupeRef(static_cast<ITEMID_TYPE>(pMultiItem->GetDispID()));
					if ( pDupeDef )
						iHeight = pDupeDef->GetHeight();
					else
					{
						g_Log.EventDebug("Failed to get non-parent reference (multi) (DispID 0%x) (X:%hd Y:%hd Z:%hhd)\n", pMultiItem->GetDispID(), ptTest.m_x, ptTest.m_y, ptTest.m_z);
						iHeight = pItemDef->GetHeight();
					}
				}
				ptTest.m_z = minimum(ptTest.m_z + iHeight, UO_SIZE_Z);	// Z + height = top point

				if ( pt.GetDist(ptTest) > iDist )
					continue;
				if ( ptElem[1].m_z > ptTest.m_z )
					continue;
				if ( (pt.m_z - ptTest.m_z < -RESOURCE_Z_CHECK) || (ptTest.m_z - pt.m_z > RESOURCE_Z_CHECK) )
					continue;
				if ( (ptTest.m_z < ptElem[1].m_z) || ((ptTest.m_z == ptElem[1].m_z) && fElem[1]) )
					continue;

				ptElem[1] = ptTest;
				fElem[1] = false;

				if ( pItemDef->IsType(type) )
				{
					fElem[1] = true;
					if ( ptElem[1].m_z > ptElem[iRetElem].m_z )
						iRetElem = 1;
				}
			}
		}
	}

	// Check map statics
	const CGrayMapBlock *pMapBlock = GetMapBlock(pt);
	size_t iStaticQty = pMapBlock ? pMapBlock->m_Statics.GetStaticQty() : 0;
	if ( iStaticQty > 0 )
	{
		const CUOStaticItemRec *pStatic = NULL;
		for ( size_t i = 0; i < iStaticQty; ++i )
		{
			pStatic = pMapBlock->m_Statics.GetStatic(i);
			pItemDef = CItemBase::FindItemBase(pStatic->GetDispID());
			if ( !pItemDef )
				continue;

			ptTest = CPointMap(pStatic->m_x + pMapBlock->m_x, pStatic->m_y + pMapBlock->m_y, pStatic->m_z, pt.m_map);
			iHeight = pItemDef->GetHeight();
			if ( pItemDef->GetID() != pStatic->GetDispID() )	// not a parent item
			{
				pDupeDef = CItemBaseDupe::GetDupeRef(static_cast<ITEMID_TYPE>(pStatic->GetDispID()));
				if ( pDupeDef )
					iHeight = pDupeDef->GetHeight();
				else
				{
					g_Log.EventDebug("Failed to get non-parent reference (static) (DispID 0%x) (X:%hd Y:%hd Z:%hhd)\n", pStatic->GetDispID(), ptTest.m_x, ptTest.m_y, ptTest.m_z);
					iHeight = pItemDef->GetHeight();
				}
			}
			ptTest.m_z = minimum(ptTest.m_z + iHeight, UO_SIZE_Z);	// Z + height = top point

			if ( pt.GetDist(ptTest) > iDist )
				continue;
			if ( ptElem[2].m_z > ptTest.m_z )
				continue;
			if ( (pt.m_z - pStatic->m_z < -RESOURCE_Z_CHECK) || (pStatic->m_z - pt.m_z > RESOURCE_Z_CHECK) )
				continue;
			if ( (ptTest.m_z < ptElem[2].m_z) || ((ptTest.m_z == ptElem[2].m_z) && fElem[2]) )
				continue;

			ptElem[2] = ptTest;
			fElem[2] = false;

			if ( pItemDef->IsType(type) )
			{
				fElem[2] = true;
				if ( ptElem[2].m_z > ptElem[iRetElem].m_z )
					iRetElem = 2;
			}
		}
	}

	// Check terrain type
	CRectMap rect;
	rect.SetRect(pt.m_x - iDist, pt.m_y - iDist, pt.m_x + iDist + 1, pt.m_y + iDist + 1, pt.m_map);
	const CUOMapMeter *pMapMeter = NULL;
	for ( int x = rect.m_left; x < rect.m_right; ++x )
	{
		for ( int y = rect.m_top; y < rect.m_bottom; ++y )
		{
			ptTest = CPointMap(static_cast<signed short>(x), static_cast<signed short>(y), pt.m_z, pt.m_map);
			pMapMeter = GetMapMeter(ptTest);
			if ( !pMapMeter )
				continue;
			if ( pt.GetDist(ptTest) > iDist )
				continue;
			if ( ptElem[3].m_z > pMapMeter->m_z )
				continue;
			if ( (pt.m_z - pMapMeter->m_z < -RESOURCE_Z_CHECK) || (pMapMeter->m_z - pt.m_z > RESOURCE_Z_CHECK) )
				continue;
			if ( (pMapMeter->m_z < ptElem[3].m_z) || ((pMapMeter->m_z == ptElem[3].m_z) && fElem[3]) )
				continue;

			ptElem[3] = ptTest;
			fElem[3] = false;

			if ( type == g_World.GetTerrainItemType(pMapMeter->m_wTerrainIndex) )
			{
				fElem[3] = true;
				if ( ptElem[3].m_z > ptElem[iRetElem].m_z )
					iRetElem = 3;
				continue;
			}

			rect.SetRect(pt.m_x - iDist, pt.m_y - iDist, pt.m_x + iDist + 1, pt.m_y + iDist + 1, pt.m_map);
		}
	}

	ASSERT(iRetElem < COUNTOF(ptElem));
	if ( (iRetElem != 0) && (ptElem[0].m_z > ptElem[iRetElem].m_z) )
		iRetElem = 4;
	else if ( (iRetElem != 1) && (ptElem[1].m_z > ptElem[iRetElem].m_z) )
		iRetElem = 4;
	else if ( (iRetElem != 2) && (ptElem[2].m_z > ptElem[iRetElem].m_z) )
		iRetElem = 4;
	else if ( (iRetElem != 3) && (ptElem[3].m_z > ptElem[iRetElem].m_z) )
		iRetElem = 4;

	return ptElem[iRetElem];
	#undef RESOURCE_Z_CHECK
}

bool CWorld::IsItemTypeNear(const CPointMap &pt, IT_TYPE type, int iDist, bool fCheckMulti, bool fLimitZ)
{
	ADDTOCALLSTACK("CWorld::IsItemTypeNear");
	if ( !pt.IsValidPoint() )
		return false;

	CPointMap ptTest = FindItemTypeNearby(pt, type, iDist, fCheckMulti, fLimitZ);
	return ptTest.IsValidPoint();
}

CPointMap CWorld::FindItemTypeNearby(const CPointMap &pt, IT_TYPE type, int iDist, bool fCheckMulti, bool fLimitZ)
{
	ADDTOCALLSTACK("CWorld::FindItemTypeNearby");
	// Find the closest item of this type
	// NOTE: this doesn't mean that I can touch it

	int iDistTest;
	CPointMap ptFound;

	// Check dynamic items
	CItem *pItem = NULL;
	CWorldSearch Area(pt, iDist);
	for (;;)
	{
		pItem = Area.GetItem();
		if ( !pItem )
			break;
		if ( !pItem->IsType(type) && !pItem->Item_GetDef()->IsType(type) )
			continue;
		if ( fLimitZ && (abs(pItem->GetTopPoint().m_z - pt.m_z) > PLAYER_HEIGHT) )
			continue;

		iDistTest = pt.GetDist(pItem->GetTopPoint());
		if ( iDistTest > iDist )
			continue;

		ptFound = pItem->GetTopPoint();
		iDist = iDistTest;		// tighten up the search
		if ( !iDist )
			return ptFound;
	}

	// Check terrain type
	CRectMap rect;
	rect.SetRect(pt.m_x - iDist, pt.m_y - iDist, pt.m_x + iDist + 1, pt.m_y + iDist + 1, pt.m_map);
	const CUOMapMeter *pMapMeter = NULL;
	for ( int x = rect.m_left; x < rect.m_right; ++x )
	{
		for ( int y = rect.m_top; y < rect.m_bottom; ++y )
		{
			CPointMap ptTest(static_cast<signed short>(x), static_cast<signed short>(y), pt.m_z, pt.m_map);
			pMapMeter = GetMapMeter(ptTest);
			if ( !pMapMeter )
				continue;
			if ( fLimitZ && (abs(pMapMeter->m_z - pt.m_z) > PLAYER_HEIGHT) )
				continue;
			if ( type != g_World.GetTerrainItemType(pMapMeter->m_wTerrainIndex) )
				continue;

			ptTest.m_z = pMapMeter->m_z;
			iDistTest = pt.GetDist(ptTest);
			if ( iDistTest > iDist )
				break;

			ptFound = ptTest;
			iDist = iDistTest;		// tighten up the search
			if ( !iDist )
				return ptFound;

			rect.SetRect(pt.m_x - iDist, pt.m_y - iDist, pt.m_x + iDist + 1, pt.m_y + iDist + 1, pt.m_map);
		}
	}

	// Check map statics
	rect.SetRect(pt.m_x - iDist, pt.m_y - iDist, pt.m_x + iDist + 1, pt.m_y + iDist + 1, pt.m_map);
	const CGrayMapBlock *pMapBlock = NULL;
	const CUOStaticItemRec *pStatic = NULL;
	const CItemBase *pItemDef = NULL;
	for ( int x = rect.m_left; x < rect.m_right; x += UO_BLOCK_SIZE )
	{
		for ( int y = rect.m_top; y < rect.m_bottom; y += UO_BLOCK_SIZE )
		{
			CPointMap ptTest(static_cast<signed short>(x), static_cast<signed short>(y), pt.m_z, pt.m_map);
			pMapBlock = GetMapBlock(ptTest);
			if ( !pMapBlock )
				continue;

			size_t iStaticQty = pMapBlock->m_Statics.GetStaticQty();
			if ( iStaticQty <= 0 )
				continue;

			for ( size_t i = 0; i < iStaticQty; ++i )
			{
				pStatic = pMapBlock->m_Statics.GetStatic(i);
				if ( fLimitZ && (abs(pStatic->m_z - ptTest.m_z) > PLAYER_HEIGHT) )
					continue;

				CPointMap ptStatic(pStatic->m_x + pMapBlock->m_x, pStatic->m_y + pMapBlock->m_y, pStatic->m_z, ptTest.m_map);
				iDistTest = pt.GetDist(ptStatic);
				if ( iDistTest > iDist )
					continue;

				pItemDef = CItemBase::FindItemBase(pStatic->GetDispID());
				if ( !pItemDef )
					continue;
				if ( !pItemDef->IsType(type) )
					continue;

				ptFound = ptStatic;
				iDist = iDistTest;
				if ( !iDist )
					return ptFound;

				rect.SetRect(pt.m_x - iDist, pt.m_y - iDist, pt.m_x + iDist + 1, pt.m_y + iDist + 1, pt.m_map);
			}
		}
	}

	// Check multi components
	if ( fCheckMulti )
	{
		rect.SetRect(pt.m_x - iDist, pt.m_y - iDist, pt.m_x + iDist + 1, pt.m_y + iDist + 1, pt.m_map);
		for ( int x = rect.m_left; x < rect.m_right; ++x )
		{
			for ( int y = rect.m_top; y < rect.m_bottom; ++y )
			{
				CPointMap ptTest(static_cast<signed short>(x), static_cast<signed short>(y), pt.m_z, pt.m_map);
				CRegionLinks rlinks;
				size_t iRegionQty = ptTest.GetRegions(REGION_TYPE_MULTI, rlinks);
				if ( iRegionQty > 0 )
				{
					CRegionBase *pRegion = NULL;
					for ( size_t iRegion = 0; iRegion < iRegionQty; ++iRegion )
					{
						pRegion = rlinks.GetAt(iRegion);
						pItem = pRegion->GetResourceID().ItemFind();
						if ( !pItem )
							continue;

						const CGrayMulti *pMulti = g_Cfg.GetMultiItemDefs(pItem);
						if ( !pMulti )
							continue;

						int x2 = ptTest.m_x - pItem->GetTopPoint().m_x;
						int y2 = ptTest.m_y - pItem->GetTopPoint().m_y;

						size_t iItemQty = pMulti->GetItemCount();
						for ( size_t iItem = 0; iItem < iItemQty; ++iItem )
						{
							const CUOMultiItemRecHS *pMultiItem = pMulti->GetItem(iItem);
							if ( !pMultiItem )
								break;
							if ( !pMultiItem->m_visible )
								continue;
							if ( (pMultiItem->m_dx != x2) || (pMultiItem->m_dy != y2) )
								continue;
							if ( fLimitZ && (abs(pMultiItem->m_dz - ptTest.m_z) > PLAYER_HEIGHT) )
								continue;

							iDistTest = pt.GetDist(ptTest);
							if ( iDistTest > iDist )
								continue;

							pItemDef = CItemBase::FindItemBase(pMultiItem->GetDispID());
							if ( !pItemDef || !pItemDef->IsType(type) )
								continue;

							ptFound = ptTest;
							iDist = iDistTest;
							if ( !iDist )
								return ptFound;

							rect.SetRect(pt.m_x - iDist, pt.m_y - iDist, pt.m_x + iDist + 1, pt.m_y + iDist + 1, pt.m_map);
						}
					}
				}
			}
		}
	}

	return ptFound;
}

void CWorld::GetFixPoint(const CPointMap &pt, CGrayMapBlockState &block)
{
	ADDTOCALLSTACK("CWorld::GetFixPoint");
	// Get the highest CAN_I_PLATFORM|CAN_I_CLIMB and set it into block.m_Bottom

	CItem *pItem = NULL;
	CItemBase *pItemDef = NULL;
	CItemBaseDupe *pDupeDef = NULL;
	DWORD dwBlockThis = 0;
	int x2 = 0, y2 = 0;
	signed char z = 0;

	const CGrayMapBlock *pMapBlock = GetMapBlock(pt);
	if ( !pMapBlock )
		return;

	// Check map statics
	// Height of statics at/above given coordinates (do gravity here for the z)
	size_t iQty = pMapBlock->m_Statics.GetStaticQty();
	if ( iQty > 0 )
	{
		x2 = pMapBlock->GetOffsetX(pt.m_x);
		y2 = pMapBlock->GetOffsetY(pt.m_y);
		const CUOStaticItemRec *pStatic = NULL;
		for ( size_t i = 0; i < iQty; ++i )
		{
			if ( !pMapBlock->m_Statics.IsStaticPoint(i, x2, y2) )
				continue;

			pStatic = pMapBlock->m_Statics.GetStatic(i);
			if ( !pStatic )
				continue;

			z = pStatic->m_z;
			pItemDef = CItemBase::FindItemBase(pStatic->GetDispID());
			if ( pItemDef )
			{
				if ( pItemDef->GetID() == pStatic->GetDispID() )	// parent item
				{
					dwBlockThis = (pItemDef->m_Can & CAN_I_MOVEMASK);
					z += (dwBlockThis & CAN_I_CLIMB) ? pItemDef->GetHeight() / 2 : pItemDef->GetHeight();
				}
				else
				{
					pDupeDef = CItemBaseDupe::GetDupeRef(static_cast<ITEMID_TYPE>(pStatic->GetDispID()));
					if ( pDupeDef )
					{
						dwBlockThis = (pDupeDef->m_Can & CAN_I_MOVEMASK);
						z += (dwBlockThis & CAN_I_CLIMB) ? pDupeDef->GetHeight() / 2 : pDupeDef->GetHeight();
					}
					else
					{
						g_Log.EventDebug("Failed to get non-parent reference (static) (DispID 0%x) (X:%hd Y:%hd Z:%hhd)\n", pStatic->GetDispID(), pStatic->m_x + pMapBlock->m_x, pStatic->m_y + pMapBlock->m_y, pStatic->m_z);
						dwBlockThis = (pItemDef->m_Can & CAN_I_MOVEMASK);
						z += (dwBlockThis & CAN_I_CLIMB) ? pItemDef->GetHeight() / 2 : pItemDef->GetHeight();
					}
				}
			}
			else if ( pStatic->GetDispID() )
				CItemBase::GetItemTiledataFlags(dwBlockThis, pStatic->GetDispID());

			if ( block.m_Bottom.m_z < z )
			{
				if ( (z < pt.m_z + PLAYER_HEIGHT) && (dwBlockThis & (CAN_I_PLATFORM|CAN_I_CLIMB|CAN_I_WATER)) )
				{
					block.m_Bottom.m_dwBlockFlags = dwBlockThis;
					block.m_Bottom.m_dwTile = pStatic->GetDispID() + TERRAIN_QTY;
					block.m_Bottom.m_z = z;
				}
				else if ( block.m_Top.m_z > z )
				{
					block.m_Top.m_dwBlockFlags = dwBlockThis;
					block.m_Top.m_dwTile = pStatic->GetDispID() + TERRAIN_QTY;
					block.m_Top.m_z = z;
				}
			}
		}
	}

	// Check multi components
	CRegionLinks rlinks;
	size_t iRegionQty = pt.GetRegions(REGION_TYPE_MULTI, rlinks);
	if ( iRegionQty > 0 )
	{
		CRegionBase *pRegion = NULL;
		const CGrayMulti *pMulti = NULL;
		const CUOMultiItemRecHS *pMultiItem = NULL;
		for ( size_t iRegion = 0; iRegion < iRegionQty; ++iRegion )
		{
			pRegion = rlinks.GetAt(iRegion);
			pItem = pRegion->GetResourceID().ItemFind();
			if ( !pItem )
				continue;

			pMulti = g_Cfg.GetMultiItemDefs(pItem);
			if ( !pMulti )
				continue;

			x2 = pt.m_x - pItem->GetTopPoint().m_x;
			y2 = pt.m_y - pItem->GetTopPoint().m_y;
			iQty = pMulti->GetItemCount();
			for ( size_t iItem = 0; iItem < iQty; ++iItem )
			{
				pMultiItem = pMulti->GetItem(iItem);
				if ( !pMultiItem )
					break;
				if ( !pMultiItem->m_visible )
					continue;
				if ( (pMultiItem->m_dx != x2) || (pMultiItem->m_dy != y2) )
					continue;

				z = pItem->GetTopZ() + static_cast<signed char>(pMultiItem->m_dz);
				pItemDef = CItemBase::FindItemBase(pMultiItem->GetDispID());
				if ( pItemDef )
				{
					if ( pItemDef->GetID() == pMultiItem->GetDispID() )		// parent item
					{
						dwBlockThis = (pItemDef->m_Can & CAN_I_MOVEMASK);
						z += (dwBlockThis & CAN_I_CLIMB) ? pItemDef->GetHeight() / 2 : pItemDef->GetHeight();
					}
					else
					{
						pDupeDef = CItemBaseDupe::GetDupeRef(static_cast<ITEMID_TYPE>(pMultiItem->GetDispID()));
						if ( pDupeDef )
						{
							dwBlockThis = (pDupeDef->m_Can & CAN_I_MOVEMASK);
							z += (dwBlockThis & CAN_I_CLIMB) ? pDupeDef->GetHeight() / 2 : pDupeDef->GetHeight();
						}
						else
						{
							g_Log.EventDebug("Failed to get non-parent reference (multi) (DispID 0%x) (X:%hd Y:%hd Z:%hd)\n", pMultiItem->GetDispID(), pMultiItem->m_dx + pItem->GetTopPoint().m_x, pMultiItem->m_dy + pItem->GetTopPoint().m_y, pMultiItem->m_dz + pItem->GetTopPoint().m_z);
							dwBlockThis = (pItemDef->m_Can & CAN_I_MOVEMASK);
							z += (dwBlockThis & CAN_I_CLIMB) ? pItemDef->GetHeight() / 2 : pItemDef->GetHeight();
						}
					}
				}
				else if ( pMultiItem->GetDispID() )
					CItemBase::GetItemTiledataFlags(dwBlockThis, pMultiItem->GetDispID());

				if ( block.m_Bottom.m_z < z )
				{
					if ( (z < pt.m_z + PLAYER_HEIGHT) && (dwBlockThis & (CAN_I_PLATFORM|CAN_I_CLIMB|CAN_I_WATER)) )
					{
						block.m_Bottom.m_dwBlockFlags = dwBlockThis;
						block.m_Bottom.m_dwTile = pMultiItem->GetDispID() + TERRAIN_QTY;
						block.m_Bottom.m_z = z;
					}
					else if ( block.m_Top.m_z > z )
					{
						block.m_Top.m_dwBlockFlags = dwBlockThis;
						block.m_Top.m_dwTile = pMultiItem->GetDispID() + TERRAIN_QTY;
						block.m_Top.m_z = z;
					}
				}
			}
		}
	}

	// Check dynamic items
	CWorldSearch Area(pt);
	for (;;)
	{
		pItem = Area.GetItem();
		if ( !pItem )
			break;

		z = pItem->GetTopZ();
		pItemDef = CItemBase::FindItemBase(pItem->GetDispID());
		if ( pItemDef )
		{
			if ( pItemDef->GetDispID() == pItem->GetDispID() )		// parent item
			{
				dwBlockThis = (pItemDef->m_Can & CAN_I_MOVEMASK);
				z += (dwBlockThis & CAN_I_CLIMB) ? pItemDef->GetHeight() / 2 : pItemDef->GetHeight();
			}
			else
			{
				pDupeDef = CItemBaseDupe::GetDupeRef(static_cast<ITEMID_TYPE>(pItem->GetDispID()));
				if ( pDupeDef )
				{
					dwBlockThis = (pDupeDef->m_Can & CAN_I_MOVEMASK);
					z += (dwBlockThis & CAN_I_CLIMB) ? pDupeDef->GetHeight() / 2 : pDupeDef->GetHeight();
				}
				else
				{
					g_Log.EventDebug("Failed to get non-parent reference (dynamic) (DispID 0%x) (X:%hd Y:%hd Z:%hhd)\n", pItem->GetDispID(), pItem->GetTopPoint().m_x, pItem->GetTopPoint().m_y, pItem->GetTopPoint().m_z);
					dwBlockThis = (pItemDef->m_Can & CAN_I_MOVEMASK);
					z += (dwBlockThis & CAN_I_CLIMB) ? pItemDef->GetHeight() / 2 : pItemDef->GetHeight();
				}
			}

			if ( block.m_Bottom.m_z < z )
			{
				if ( (z < pt.m_z + PLAYER_HEIGHT) && (dwBlockThis & (CAN_I_PLATFORM|CAN_I_CLIMB|CAN_I_WATER)) )
				{
					block.m_Bottom.m_dwBlockFlags = dwBlockThis;
					block.m_Bottom.m_dwTile = pItemDef->GetDispID() + TERRAIN_QTY;
					block.m_Bottom.m_z = z;
				}
				else if ( block.m_Top.m_z > z )
				{
					block.m_Top.m_dwBlockFlags = dwBlockThis;
					block.m_Top.m_dwTile = pItemDef->GetDispID() + TERRAIN_QTY;
					block.m_Top.m_z = z;
				}
			}
		}
		else if ( pItem->GetDispID() )
			CItemBase::GetItemTiledataFlags(dwBlockThis, pItem->GetDispID());
	}

	// Check terrain
	const CUOMapMeter *pMapMeter = pMapBlock->GetTerrain(UO_BLOCK_OFFSET(pt.m_x), UO_BLOCK_OFFSET(pt.m_y));
	if ( !pMapMeter )
		return;

	if ( pMapMeter->m_wTerrainIndex == TERRAIN_HOLE )
		dwBlockThis = 0;
	else if ( CUOMapMeter::IsTerrainNull(pMapMeter->m_wTerrainIndex) )		// inter dungeon type
		dwBlockThis = CAN_I_BLOCK;
	else
	{
		CGrayTerrainInfo land(pMapMeter->m_wTerrainIndex);
		dwBlockThis = 0;
		if ( land.m_flags & UFLAG1_WATER )
			dwBlockThis |= CAN_I_WATER;
		if ( land.m_flags & UFLAG1_DAMAGE )
			dwBlockThis |= CAN_I_FIRE;
		if ( land.m_flags & UFLAG1_BLOCK )
			dwBlockThis |= CAN_I_BLOCK;
		if ( !dwBlockThis || (land.m_flags & UFLAG2_PLATFORM) )		// platform items should take precedence over non-platforms
			dwBlockThis = CAN_I_PLATFORM;
	}

	if ( block.m_Bottom.m_z < pMapMeter->m_z )
	{
		if ( ((pMapMeter->m_z < pt.m_z + PLAYER_HEIGHT) && (dwBlockThis & (CAN_I_PLATFORM|CAN_I_CLIMB|CAN_I_WATER))) || (block.m_Bottom.m_z == UO_SIZE_MIN_Z) )
		{
			block.m_Bottom.m_dwBlockFlags = dwBlockThis;
			block.m_Bottom.m_dwTile = pMapMeter->m_wTerrainIndex;
			block.m_Bottom.m_z = pMapMeter->m_z;
		}
		else if ( block.m_Top.m_z > pMapMeter->m_z )
		{
			block.m_Top.m_dwBlockFlags = dwBlockThis;
			block.m_Top.m_dwTile = pMapMeter->m_wTerrainIndex;
			block.m_Top.m_z = pMapMeter->m_z;
		}
	}

	if ( block.m_Bottom.m_z == UO_SIZE_MIN_Z )
	{
		// Fail safe, reset to 0 with no top block
		block.m_Bottom.m_dwBlockFlags = 0;
		block.m_Bottom.m_dwTile = 0;
		block.m_Bottom.m_z = 0;

		block.m_Top.m_dwBlockFlags = 0;
		block.m_Top.m_dwTile = 0;
		block.m_Top.m_z = UO_SIZE_Z;
	}
}

void CWorld::GetHeightPoint(const CPointMap &pt, CGrayMapBlockState &block, bool fHouseCheck)
{
	ADDTOCALLSTACK("CWorld::GetHeightPoint");

	CItem *pItem = NULL;
	CItemBase *pItemDef = NULL;
	CItemBaseDupe *pDupeDef = NULL;
	DWORD dwBlockThis = 0;
	int x2 = 0, y2 = 0;
	signed char z = 0;
	height_t zHeight = 0;

	const CGrayMapBlock *pMapBlock = GetMapBlock(pt);
	if ( !pMapBlock )
		return;

	// Check map statics
	size_t iQty = pMapBlock->m_Statics.GetStaticQty();
	if ( iQty > 0 )
	{
		x2 = pMapBlock->GetOffsetX(pt.m_x);
		y2 = pMapBlock->GetOffsetY(pt.m_y);
		const CUOStaticItemRec *pStatic = NULL;
		for ( size_t iStatic = 0; iStatic < iQty; ++iStatic )
		{
			if ( !pMapBlock->m_Statics.IsStaticPoint(iStatic, x2, y2) )
				continue;

			pStatic = pMapBlock->m_Statics.GetStatic(iStatic);
			if ( !pStatic )
				continue;

			z = pStatic->m_z;
			if ( !block.IsUsableZ(z, block.m_zHeight) )
				continue;

			pItemDef = CItemBase::FindItemBase(pStatic->GetDispID());
			if ( pItemDef )
			{
				if ( pItemDef->GetID() == pStatic->GetDispID() )	// parent item
				{
					zHeight = pItemDef->GetHeight();
					dwBlockThis = (pItemDef->m_Can & CAN_I_MOVEMASK);	// use only block flags, other remove
				}
				else
				{
					pDupeDef = CItemBaseDupe::GetDupeRef(static_cast<ITEMID_TYPE>(pStatic->GetDispID()));
					if ( pDupeDef )
					{
						zHeight = pDupeDef->GetHeight();
						dwBlockThis = (pDupeDef->m_Can & CAN_I_MOVEMASK);	// use only block flags, other remove - CAN flags cannot be inherited from the parent item due to bad script pack...
					}
					else
					{
						g_Log.EventDebug("Failed to get non-parent reference (static) (DispID 0%x) (X:%hd Y:%hd Z:%hhd)\n", pStatic->GetDispID(), pStatic->m_x + pMapBlock->m_x, pStatic->m_y + pMapBlock->m_y, pStatic->m_z);
						zHeight = pItemDef->GetHeight();
						dwBlockThis = (pItemDef->m_Can & CAN_I_MOVEMASK);
					}
				}
			}
			else if ( pStatic->GetDispID() )
				CItemBase::GetItemTiledataFlags(dwBlockThis, pStatic->GetDispID());

			block.CheckTile_Item(dwBlockThis, z, zHeight, pStatic->GetDispID() + TERRAIN_QTY);
		}
	}

	// Check multi components
	if ( fHouseCheck )
	{
		CRegionLinks rlinks;
		size_t iRegionQty = pt.GetRegions(REGION_TYPE_MULTI, rlinks);
		if ( iRegionQty > 0 )
		{
			CRegionBase *pRegion = NULL;
			const CGrayMulti *pMulti = NULL;
			const CUOMultiItemRecHS *pMultiItem = NULL;
			for ( size_t iRegion = 0; iRegion < iRegionQty; ++iRegion )
			{
				pRegion = rlinks.GetAt(iRegion);
				pItem = pRegion->GetResourceID().ItemFind();
				if ( !pItem )
					continue;

				pMulti = g_Cfg.GetMultiItemDefs(pItem);
				if ( !pMulti )
					continue;

				x2 = pt.m_x - pItem->GetTopPoint().m_x;
				y2 = pt.m_y - pItem->GetTopPoint().m_y;
				iQty = pMulti->GetItemCount();
				for ( size_t iMulti = 0; iMulti < iQty; ++iMulti )
				{
					pMultiItem = pMulti->GetItem(iMulti);
					if ( !pMultiItem )
						break;
					if ( !pMultiItem->m_visible )
						continue;
					if ( (pMultiItem->m_dx != x2) || (pMultiItem->m_dy != y2) )
						continue;

					z = pItem->GetTopZ() + static_cast<signed char>(pMultiItem->m_dz);
					if ( !block.IsUsableZ(z, block.m_zHeight) )
						continue;

					pItemDef = CItemBase::FindItemBase(pMultiItem->GetDispID());
					if ( pItemDef )
					{
						if ( pItemDef->GetID() == pMultiItem->GetDispID() )		// parent item
						{
							zHeight = pItemDef->GetHeight();
							dwBlockThis = (pItemDef->m_Can & CAN_I_MOVEMASK);	// use only block flags, other remove
						}
						else
						{
							pDupeDef = CItemBaseDupe::GetDupeRef(static_cast<ITEMID_TYPE>(pMultiItem->GetDispID()));
							if ( pDupeDef )
							{
								zHeight = pDupeDef->GetHeight();
								dwBlockThis = (pDupeDef->m_Can & CAN_I_MOVEMASK);	// use only block flags, other remove - CAN flags cannot be inherited from the parent item due to bad script pack...
							}
							else
							{
								g_Log.EventDebug("Failed to get non-parent reference (multi) (DispID 0%x) (X:%hd Y:%hd Z:%hd)\n", pMultiItem->GetDispID(), pMultiItem->m_dx + pItem->GetTopPoint().m_x, pMultiItem->m_dy + pItem->GetTopPoint().m_y, pMultiItem->m_dz + pItem->GetTopPoint().m_z);
								zHeight = pItemDef->GetHeight();
								dwBlockThis = (pItemDef->m_Can & CAN_I_MOVEMASK);
							}
						}
					}
					else if ( pMultiItem->GetDispID() )
						CItemBase::GetItemTiledataFlags(dwBlockThis, pMultiItem->GetDispID());

					block.CheckTile_Item(dwBlockThis, z, zHeight, pMultiItem->GetDispID() + TERRAIN_QTY);
				}
			}
		}
	}

	// Check dynamic items
	CWorldSearch Area(pt);
	for (;;)
	{
		pItem = Area.GetItem();
		if ( !pItem )
			break;

		z = pItem->GetTopZ();
		if ( !block.IsUsableZ(z, block.m_zHeight) )
			continue;

		pItemDef = CItemBase::FindItemBase(pItem->GetDispID());
		if ( pItemDef )
		{
			if ( pItemDef->GetDispID() == pItem->GetDispID() )		// parent item
			{
				zHeight = pItemDef->GetHeight();
				dwBlockThis = (pItemDef->m_Can & CAN_I_MOVEMASK);	// use only block flags, other remove
			}
			else
			{
				pDupeDef = CItemBaseDupe::GetDupeRef(static_cast<ITEMID_TYPE>(pItem->GetDispID()));
				if ( pDupeDef )
				{
					zHeight = pDupeDef->GetHeight();
					dwBlockThis = (pDupeDef->m_Can & CAN_I_MOVEMASK);	// use only block flags, other remove - CAN flags cannot be inherited from the parent item due to bad script pack...
				}
				else
				{
					g_Log.EventDebug("Failed to get non-parent reference (dynamic) (DispID 0%x) (X:%hd Y:%hd Z:%hhd)\n", pItem->GetDispID(), pItem->GetTopPoint().m_x, pItem->GetTopPoint().m_y, pItem->GetTopPoint().m_z);
					zHeight = pItemDef->GetHeight();
					dwBlockThis = (pItemDef->m_Can & CAN_I_MOVEMASK);
				}
			}
		}
		else if ( pItem->GetDispID() )
			CItemBase::GetItemTiledataFlags(dwBlockThis, pItem->GetDispID());

		block.CheckTile_Item(dwBlockThis, z, zHeight, pItem->GetDispID() + TERRAIN_QTY);
	}

	// Check terrain
	const CUOMapMeter *pMapMeter = pMapBlock->GetTerrain(UO_BLOCK_OFFSET(pt.m_x), UO_BLOCK_OFFSET(pt.m_y));
	if ( !pMapMeter )
		return;

	if ( block.IsUsableZ(pMapMeter->m_z, block.m_zHeight) )
	{
		if ( pMapMeter->m_wTerrainIndex == TERRAIN_HOLE )
			dwBlockThis = 0;
		else if ( CUOMapMeter::IsTerrainNull(pMapMeter->m_wTerrainIndex) )		// inter dungeon type
			dwBlockThis = CAN_I_BLOCK;
		else
		{
			CGrayTerrainInfo land(pMapMeter->m_wTerrainIndex);
			dwBlockThis = 0;
			if ( land.m_flags & UFLAG1_WATER )
				dwBlockThis |= CAN_I_WATER;
			if ( land.m_flags & UFLAG1_DAMAGE )
				dwBlockThis |= CAN_I_FIRE;
			if ( land.m_flags & UFLAG1_BLOCK )
				dwBlockThis |= CAN_I_BLOCK;
			if ( !dwBlockThis || (land.m_flags & UFLAG2_PLATFORM) )		// platform items should take precedence over non-platforms
				dwBlockThis = CAN_I_PLATFORM;
		}
		block.CheckTile_Terrain(dwBlockThis, pMapMeter->m_z, pMapMeter->m_wTerrainIndex);
	}

	if ( block.m_Bottom.m_z == UO_SIZE_MIN_Z )
	{
		block.m_Bottom = block.m_Lowest;
		if ( block.m_Top.m_z == block.m_Bottom.m_z )
		{
			block.m_Top.m_dwBlockFlags = 0;
			block.m_Top.m_dwTile = 0;
			block.m_Top.m_z = UO_SIZE_Z;
		}
	}
}

void CWorld::GetHeightPoint2(const CPointMap &pt, CGrayMapBlockState &block, bool fHouseCheck)
{
	ADDTOCALLSTACK_INTENSIVE("CWorld::GetHeightPoint2");
	// Height of statics at/above given coordinates (do gravity here for the z)

	const CGrayMapBlock *pMapBlock = GetMapBlock(pt);
	if ( !pMapBlock )
	{
		g_Log.EventWarn("GetMapBlock failed at %s\n", pt.WriteUsed());
		return;
	}

	DWORD dwBlockThis = 0;

	// Check map statics
	size_t iStaticQty = pMapBlock->m_Statics.GetStaticQty();
	if ( iStaticQty > 0 )
	{
		int x2 = pMapBlock->GetOffsetX(pt.m_x);
		int y2 = pMapBlock->GetOffsetY(pt.m_y);
		for ( size_t iStatic = 0; iStatic < iStaticQty; ++iStatic )
		{
			if ( !pMapBlock->m_Statics.IsStaticPoint(iStatic, x2, y2) )
				continue;

			const CUOStaticItemRec *pStatic = pMapBlock->m_Statics.GetStatic(iStatic);
			signed char z = pStatic->m_z;
			if ( !block.IsUsableZ(z, PLAYER_HEIGHT) )
				continue;

			dwBlockThis = 0;
			height_t zHeight = CItemBase::GetItemHeight(pStatic->GetDispID(), dwBlockThis);
			block.CheckTile(dwBlockThis, z, zHeight, pStatic->GetDispID() + TERRAIN_QTY);
		}
	}

	// Check multi components
	if ( fHouseCheck )
	{
		CRegionLinks rlinks;
		size_t iRegionQty = pt.GetRegions(REGION_TYPE_MULTI, rlinks);
		if ( iRegionQty > 0 )
		{
			for ( size_t iRegion = 0; iRegion < iRegionQty; ++iRegion )
			{
				CRegionBase *pRegion = rlinks.GetAt(iRegion);
				CItem *pItem = pRegion->GetResourceID().ItemFind();
				if ( !pItem )
					continue;

				const CGrayMulti *pMulti = g_Cfg.GetMultiItemDefs(pItem);
				if ( !pMulti )
					continue;

				int x2 = pt.m_x - pItem->GetTopPoint().m_x;
				int y2 = pt.m_y - pItem->GetTopPoint().m_y;
				size_t iMultiQty = pMulti->GetItemCount();
				for ( size_t iMulti = 0; iMulti < iMultiQty; ++iMulti )
				{
					const CUOMultiItemRecHS *pMultiItem = pMulti->GetItem(iMulti);
					if ( !pMultiItem )
						break;
					if ( !pMultiItem->m_visible )
						continue;
					if ( (pMultiItem->m_dx != x2) || (pMultiItem->m_dy != y2) )
						continue;

					signed char z = pItem->GetTopZ() + static_cast<signed char>(pMultiItem->m_dz);
					if ( !block.IsUsableZ(z, PLAYER_HEIGHT) )
						continue;

					dwBlockThis = 0;
					height_t zHeight = CItemBase::GetItemHeight(pMultiItem->GetDispID(), dwBlockThis);
					block.CheckTile(dwBlockThis, z, zHeight, pMultiItem->GetDispID() + TERRAIN_QTY);
				}
			}
		}
	}

	// Check dynamic items
	CWorldSearch Area(pt);
	for (;;)
	{
		CItem *pItem = Area.GetItem();
		if ( !pItem )
			break;

		signed char z = pItem->GetTopZ();
		if ( !block.IsUsableZ(z, PLAYER_HEIGHT) )
			continue;

		// Get Attributes from ItemDef. If not set, get them from the static object (DISPID)
		CItemBase *pItemDef = pItem->Item_GetDef();
		ASSERT(pItemDef);
		dwBlockThis = pItemDef->m_Can & (CAN_I_DOOR|CAN_I_WATER|CAN_I_CLIMB|CAN_I_BLOCK|CAN_I_PLATFORM);
		height_t zHeight = pItemDef->GetHeight();

		DWORD dwStaticBlockThis = 0;
		height_t zStaticHeight = CItemBase::GetItemHeight(pItem->GetDispID(), dwStaticBlockThis);

		if ( dwBlockThis == 0 )
			dwBlockThis = dwStaticBlockThis;
		if ( zHeight == 0 )
			zHeight = zStaticHeight;

		block.CheckTile(dwBlockThis, z, zHeight, pItemDef->GetDispID() + TERRAIN_QTY);
	}

	// Check terrain
	const CUOMapMeter *pMapMeter = pMapBlock->GetTerrain(UO_BLOCK_OFFSET(pt.m_x), UO_BLOCK_OFFSET(pt.m_y));
	if ( !pMapMeter )
		return;

	if ( block.IsUsableZ(pMapMeter->m_z, 0) )
	{
		if ( pMapMeter->m_wTerrainIndex == TERRAIN_HOLE )
			dwBlockThis = 0;
		else if ( pMapMeter->m_wTerrainIndex == TERRAIN_NULL )		// inter dungeon type
			dwBlockThis = CAN_I_BLOCK;
		else
		{
			CGrayTerrainInfo land(pMapMeter->m_wTerrainIndex);
			if ( land.m_flags & UFLAG2_PLATFORM )	// platform items should take precedence over non-platforms
				dwBlockThis = CAN_I_PLATFORM;
			else if ( land.m_flags & UFLAG1_WATER )
				dwBlockThis = CAN_I_WATER;
			else if ( land.m_flags & UFLAG1_DAMAGE )
				dwBlockThis = CAN_I_FIRE;
			else if ( land.m_flags & UFLAG1_BLOCK )
				dwBlockThis = CAN_I_BLOCK;
			else
				dwBlockThis = CAN_I_PLATFORM;
		}
		block.CheckTile(dwBlockThis, pMapMeter->m_z, 0, pMapMeter->m_wTerrainIndex);
	}

	if ( block.m_Bottom.m_z == UO_SIZE_MIN_Z )
		block.m_Bottom = block.m_Lowest;
}

signed char CWorld::GetHeightPoint(const CPointBase &pt, DWORD &dwBlockFlags, bool fHouseCheck)
{
	ADDTOCALLSTACK_INTENSIVE("CWorld::GetHeightPoint");

	DWORD dwCan = dwBlockFlags;
	CGrayMapBlockState block(dwBlockFlags, pt.m_z + (PLAYER_HEIGHT / 2), pt.m_z + PLAYER_HEIGHT);
	GetHeightPoint(pt, block, fHouseCheck);

	dwBlockFlags = block.m_Bottom.m_dwBlockFlags;
	if ( block.m_Top.m_dwBlockFlags )
		dwBlockFlags |= CAN_I_ROOF;		// we are covered by something

	if ( (block.m_Lowest.m_dwBlockFlags & CAN_I_HOVER) || (block.m_Bottom.m_dwBlockFlags & CAN_I_HOVER) || (block.m_Top.m_dwBlockFlags & CAN_I_HOVER) )
	{
		if ( dwCan & CAN_C_HOVER )
			dwBlockFlags = 0;		// we can hover over this
		else
			dwBlockFlags &= ~CAN_I_HOVER;	// we don't have the ability to fly
	}

	if ( (dwCan & CAN_C_WALK) && (dwBlockFlags & (CAN_I_CLIMB|CAN_I_PLATFORM)) )
	{
		dwBlockFlags &= ~CAN_I_CLIMB;
		dwBlockFlags |= CAN_I_PLATFORM;		// not really true but hack it anyhow
		return block.m_Bottom.m_z;
	}
	if ( dwCan & CAN_C_FLY )
		return pt.m_z;

	return block.m_Bottom.m_z;
}

signed char CWorld::GetHeightPoint2(const CPointBase &pt, DWORD &dwBlockFlags, bool fHouseCheck)
{
	ADDTOCALLSTACK_INTENSIVE("CWorld::GetHeightPoint2");
	// Given our coords at pt including pt.m_z
	// What is the height that gravity would put me at should i step hear ?
	// Assume my head height is PLAYER_HEIGHT/2
	// ARGS:
	//  pt = the point of interest.
	//  pt.m_z = my starting altitude.
	//  dwBlockFlags = what we can pass thru. doors, water, walls ?
	//    CAN_C_GHOST = Moves thru doors etc. - CAN_I_DOOR
	//    CAN_C_SWIM = walk thru water - CAN_I_WATER
	//    CAN_C_WALK = it is possible that i can step up. - CAN_I_PLATFORM
	//    CAN_C_PASSWALLS = walk through all blocking items - CAN_I_BLOCK
	//    CAN_C_FLY = gravity does not effect me. - CAN_I_CLIMB
	//    CAN_C_FIRE_IMMUNE = i can walk into lava etc. - CAN_I_FIRE
	//  pRegion = possible regional effects. (multi's)
	// RETURN:
	//  pt.m_z = Our new height at pt.m_x,pt.m_y
	//  dwBlockFlags = our blocking flags at the given location. CAN_I_WATER,CAN_C_WALK,CAN_FLY,CAN_SPIRIT,
	//    CAN_C_INDOORS = i am covered from the sky

	DWORD dwCan = dwBlockFlags;
	CGrayMapBlockState block(dwBlockFlags, pt.m_z, PLAYER_HEIGHT);
	GetHeightPoint2(pt, block, fHouseCheck);

	dwBlockFlags = block.m_Bottom.m_dwBlockFlags;
	if ( block.m_Top.m_dwBlockFlags )
	{
		dwBlockFlags |= CAN_I_ROOF;		// we are covered by something

		// Do not check for landtiles to block me. We pass through if statics are under them
		if ( block.m_Top.m_dwTile > TERRAIN_QTY )
		{
			// If this tile possibly blocks me, roof cannot block me
			if ( block.m_Top.m_dwBlockFlags & ~CAN_I_ROOF )
			{
				if ( block.m_Top.m_z < block.m_Bottom.m_z + PLAYER_HEIGHT )
					dwBlockFlags |= CAN_I_BLOCK;
			}
		}
	}

	if ( (block.m_Lowest.m_dwBlockFlags & CAN_I_HOVER) || (block.m_Bottom.m_dwBlockFlags & CAN_I_HOVER) || (block.m_Top.m_dwBlockFlags & CAN_I_HOVER) )
	{
		if ( dwCan & CAN_C_HOVER )
			dwBlockFlags = 0;		// we can hover over this
		else
			dwBlockFlags &= ~CAN_I_HOVER;	// we don't have the ability to fly
	}

	if ( (dwCan & CAN_C_WALK) && (dwBlockFlags & (CAN_I_CLIMB|CAN_I_PLATFORM)) )
	{
		dwBlockFlags &= ~CAN_I_CLIMB;
		dwBlockFlags |= CAN_I_PLATFORM;		// not really true but hack it anyhow
		return block.m_Bottom.m_z;
	}
	if ( dwCan & CAN_C_FLY )
		return pt.m_z;

	return block.m_Bottom.m_z;
}

CItemTypeDef *CWorld::GetTerrainItemTypeDef(DWORD dwIndex)
{
	ADDTOCALLSTACK("CWorld::GetTerrainItemTypeDef");
	CResourceDef *pRes = (g_World.m_TileTypes.IsValidIndex(dwIndex)) ? g_World.m_TileTypes[dwIndex] : NULL;
	if ( !pRes )
	{
		RESOURCE_ID rid(RES_TYPEDEF, IT_NORMAL);
		pRes = g_Cfg.ResourceGetDef(rid);
	}
	ASSERT(pRes);
	return dynamic_cast<CItemTypeDef *>(pRes);
}

IT_TYPE CWorld::GetTerrainItemType(DWORD dwIndex)
{
	ADDTOCALLSTACK("CWorld::GetTerrainItemType");
	CResourceDef *pRes = (g_World.m_TileTypes.IsValidIndex(dwIndex)) ? g_World.m_TileTypes[dwIndex] : NULL;
	if ( pRes )
	{
		const CItemTypeDef *pItemTypeDef = dynamic_cast<CItemTypeDef *>(pRes);
		if ( pItemTypeDef )
			return static_cast<IT_TYPE>(pItemTypeDef->GetItemType());
	}
	return IT_NORMAL;
}
