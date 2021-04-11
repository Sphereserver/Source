#include "../graysvr/graysvr.h"

void CPointBase::Set(const CPointBase &pt)
{
	m_x = pt.m_x;
	m_y = pt.m_y;
	m_z = pt.m_z;
	m_map = pt.m_map;
}

void CPointBase::Set(signed short x, signed short y, signed char z, BYTE map)
{
	m_x = x;
	m_y = y;
	m_z = z;
	m_map = map;
}

void CPointBase::InitPoint()
{
	m_x = -1;
	m_y = -1;
	m_z = 0;
	m_map = 0;
}

void CPointBase::ZeroPoint()
{
	m_x = 0;
	m_y = 0;
	m_z = 0;
	m_map = 0;
}

void CPointBase::ValidatePoint()
{
	if ( m_x < 0 )
		m_x = 0;
	else if ( m_x >= static_cast<signed short>(g_MapList.GetX(m_map)) )
		m_x = static_cast<signed short>(g_MapList.GetX(m_map) - 1);

	if ( m_y < 0 )
		m_y = 0;
	else if ( m_y >= static_cast<signed short>(g_MapList.GetY(m_map)) )
		m_y = static_cast<signed short>(g_MapList.GetY(m_map) - 1);
}

int CPointBase::GetDistBase(const CPointBase &pt) const
{
	int dx = abs(m_x - pt.m_x);
	int dy = abs(m_y - pt.m_y);
	return maximum(dx, dy);
}

int CPointBase::GetDist(const CPointBase &pt) const
{
	ADDTOCALLSTACK("CPointBase::GetDist");
	if ( !pt.IsValidPoint() || (pt.m_map != m_map) )
		return SHRT_MAX;

	return GetDistBase(pt);
}

int CPointBase::GetDist3D(const CPointBase &pt) const
{
	ADDTOCALLSTACK("CPointBase::GetDist3D");
	int dist = GetDist(pt);
	int dz = abs(m_z - pt.m_z) / (PLAYER_HEIGHT / 2);
	return maximum(dist, dz);
}

bool CPointBase::IsSame2D(const CPointBase &pt) const
{
	return ((m_x == pt.m_x) && (m_y == pt.m_y));
}

bool CPointBase::IsValidXY() const
{
	return ((m_x >= 0) && (m_x < g_MapList.GetX(m_map)) && (m_y >= 0) && (m_y < g_MapList.GetY(m_map)));
}

bool CPointBase::IsValidZ() const
{
	return ((m_z > UO_SIZE_MIN_Z) && (m_z < UO_SIZE_Z));
}

bool CPointBase::IsValidPoint() const
{
	return (IsValidXY() && IsValidZ());
}

const LPCTSTR CPointBase::sm_szDirs[DIR_QTY + 1] =	// static
{
	g_Cfg.GetDefaultMsg(DEFMSG_MAP_DIR_0),
	g_Cfg.GetDefaultMsg(DEFMSG_MAP_DIR_1),
	g_Cfg.GetDefaultMsg(DEFMSG_MAP_DIR_2),
	g_Cfg.GetDefaultMsg(DEFMSG_MAP_DIR_3),
	g_Cfg.GetDefaultMsg(DEFMSG_MAP_DIR_4),
	g_Cfg.GetDefaultMsg(DEFMSG_MAP_DIR_5),
	g_Cfg.GetDefaultMsg(DEFMSG_MAP_DIR_6),
	g_Cfg.GetDefaultMsg(DEFMSG_MAP_DIR_7),
	g_Cfg.GetDefaultMsg(DEFMSG_MAP_DIR_8)
};

const int CPointBase::sm_Dirs[DIR_QTY + 1][2] =	// static
{
	{  0, -1 },	// DIR_N
	{  1, -1 },	// DIR_NE
	{  1,  0 },	// DIR_E
	{  1,  1 },	// DIR_SE
	{  0,  1 },	// DIR_S
	{ -1,  1 },	// DIR_SW
	{ -1,  0 },	// DIR_W
	{ -1, -1 },	// DIR_NW
	{  0,  0 }	// DIR_QTY
};

void CPointBase::Move(DIR_TYPE dir)
{
	ASSERT(dir <= DIR_QTY);
	m_x += static_cast<signed short>(sm_Dirs[dir][0]);
	m_y += static_cast<signed short>(sm_Dirs[dir][1]);
}

void CPointBase::MoveN(DIR_TYPE dir, int iSteps)
{
	ASSERT(dir <= DIR_QTY);
	m_x += static_cast<signed short>(sm_Dirs[dir][0] * iSteps);
	m_y += static_cast<signed short>(sm_Dirs[dir][1] * iSteps);
}

int CPointBase::StepLinePath(const CPointBase &pt, int iSteps)
{
	ADDTOCALLSTACK("CPointBase::StepLinePath");
	int iDist = GetDist(pt);
	if ( !iDist )
		return 0;

	int dx = m_x - pt.m_x;
	int dy = m_y - pt.m_y;
	m_x = pt.m_x + static_cast<signed short>(IMULDIV(dx, iSteps, iDist));
	m_y = pt.m_y + static_cast<signed short>(IMULDIV(dy, iSteps, iDist));
	return iDist;
}

CRegionBase *CPointBase::GetRegion(BYTE bType) const
{
	ADDTOCALLSTACK("CPointBase::GetRegion");
	if ( !IsValidPoint() )
		return NULL;

	CSector *pSector = GetSector();
	if ( pSector )
		return pSector->GetRegion(*this, bType);

	return NULL;
}

size_t CPointBase::GetRegions(BYTE bType, CRegionLinks &rlinks) const
{
	ADDTOCALLSTACK("CPointBase::GetRegions");
	if ( !IsValidPoint() )
		return 0;

	CSector *pSector = GetSector();
	if ( pSector )
		return pSector->GetRegions(*this, bType, rlinks);

	return 0;
}

CSector *CPointBase::GetSector() const
{
	ADDTOCALLSTACK("CPointBase::GetSector");
	if ( !g_MapList.IsMapSupported(m_map) )
	{
		DEBUG_ERR(("Trying to get sector %hd,%hd on unsupported map #%hhu\n", m_x, m_y, m_map));
		return NULL;
	}
	else if ( !IsValidXY() )
	{
		DEBUG_ERR(("Trying to get sector %hd,%hd out of map #%hhu bounds %d,%d\n", m_x, m_y, m_map, g_MapList.GetX(m_map), g_MapList.GetY(m_map)));
		return NULL;
	}

	int iSectorSize = g_MapList.GetSectorSize(m_map);
	if ( iSectorSize <= 0 )
		return NULL;

	return g_World.GetSector(m_map, (m_y / iSectorSize * g_MapList.GetSectorCols(m_map)) + (m_x / iSectorSize));
}

DIR_TYPE CPointBase::GetDir(const CPointBase &pt, DIR_TYPE dirDefault) const
{
	ADDTOCALLSTACK("CPointBase::GetDir");
	int dx = m_x - pt.m_x;
	int dy = m_y - pt.m_y;
	int ax = abs(dx);
	int ay = abs(dy);

	if ( ay > ax )
	{
		if ( !ax )
			return (dy > 0) ? DIR_N : DIR_S;

		int slope = ay / ax;
		if ( slope > 2 )
			return (dy > 0) ? DIR_N : DIR_S;

		if ( dx > 0 )
			return (dy > 0) ? DIR_NW : DIR_SW;

		return (dy > 0) ? DIR_NE : DIR_SE;
	}
	else
	{
		if ( !ay )
		{
			if ( !dx )
				return dirDefault;

			return (dx > 0) ? DIR_W : DIR_E;
		}

		int slope = ax / ay;
		if ( slope > 2 )
			return (dx > 0) ? DIR_W : DIR_E;

		if ( dy > 0 )
			return (dx > 0) ? DIR_NW : DIR_NE;

		return (dx > 0) ? DIR_SW : DIR_SE;
	}
}

size_t CPointBase::Read(TCHAR *pszVal)
{
	ADDTOCALLSTACK("CPointBase::Read");
	m_z = 0;
	m_map = 0;

	TCHAR *ppVal[4];
	size_t iArgQty = Str_ParseCmds(pszVal, ppVal, COUNTOF(ppVal), " ,\t");		// don't use '=' as separator here
	switch ( iArgQty )
	{
		default:
		case 4:
			if ( IsDigit(ppVal[3][0]) )
			{
				m_map = static_cast<BYTE>(ATOI(ppVal[3]));
				if ( !g_MapList.IsMapSupported(m_map) )
				{
					DEBUG_ERR(("Unsupported map #%hhu (defaulting to 0)\n", m_map));
					m_map = 0;
				}
			}
		case 3:
			if ( IsDigit(ppVal[2][0]) || (ppVal[2][0] == '-') )
				m_z = static_cast<signed char>(ATOI(ppVal[2]));
		case 2:
			m_y = static_cast<signed short>(ATOI(ppVal[1]));
		case 1:
			m_x = static_cast<signed short>(ATOI(ppVal[0]));
		case 0:
			break;
	}
	return iArgQty;
}

LPCTSTR CPointBase::WriteUsed() const
{
	ADDTOCALLSTACK_INTENSIVE("CPointBase::WriteUsed");

	TCHAR *pszTemp = Str_GetTemp();
	if ( m_map )
		snprintf(pszTemp, 20, "%hd,%hd,%hhd,%hhu", m_x, m_y, m_z, m_map);
	else if ( m_z )
		snprintf(pszTemp, 16, "%hd,%hd,%hhd", m_x, m_y, m_z);
	else
		snprintf(pszTemp, 12, "%hd,%hd", m_x, m_y);

	return pszTemp;
}

enum PT_TYPE
{
	PT_ISNEARTYPE,
	PT_M,
	PT_MAP,
	PT_REGION,
	PT_ROOM,
	PT_SECTOR,
	PT_TERRAIN,
	PT_TYPE,
	PT_X,
	PT_Y,
	PT_Z,
	PT_QTY
};

const LPCTSTR CPointBase::sm_szLoadKeys[PT_QTY + 1] =	// static
{
	"ISNEARTYPE",
	"M",
	"MAP",
	"REGION",
	"ROOM",
	"SECTOR",
	"TERRAIN",
	"TYPE",
	"X",
	"Y",
	"Z",
	NULL
};

bool CPointBase::r_WriteVal(LPCTSTR pszKey, CGString &sVal) const
{
	ADDTOCALLSTACK("CPointBase::r_WriteVal");
	if ( !strnicmp(pszKey, "STATICS", 7) )
	{
		pszKey += 7;
		const CGrayMapBlock *pBlock = g_World.GetMapBlock(*(this));
		if ( !pBlock )
			return false;

		if ( *pszKey == '\0' )
		{
			int iStaticQty = 0;
			for ( size_t i = 0; i < pBlock->m_Statics.GetStaticQty(); ++i )
			{
				const CUOStaticItemRec *pStatic = pBlock->m_Statics.GetStatic(i);

				CPointMap ptTest(pStatic->m_x + pBlock->m_x, pStatic->m_y + pBlock->m_y, pStatic->m_z, m_map);
				if ( GetDist(ptTest) > 0 )
					continue;

				++iStaticQty;
			}

			sVal.FormatVal(iStaticQty);
			return true;
		}

		const CUOStaticItemRec *pStatic = NULL;
		int iStatic = 0;
		int type = 0;

		SKIP_SEPARATORS(pszKey);
		if ( !strnicmp(pszKey, "FINDID", 6) )
		{
			pszKey += 6;
			SKIP_SEPARATORS(pszKey);

			iStatic = Exp_GetVal(pszKey);
			type = RES_GET_TYPE(iStatic);
			if ( type == 0 )
				type = RES_ITEMDEF;

			SKIP_SEPARATORS(pszKey);
		}
		else
		{
			iStatic = Exp_GetVal(pszKey);
			type = RES_GET_TYPE(iStatic);
		}

		if ( type == RES_ITEMDEF )
		{
			const CItemBase *pItemDef = CItemBase::FindItemBase(static_cast<ITEMID_TYPE>(RES_GET_INDEX(iStatic)));
			if ( !pItemDef )
			{
				sVal = "0";
				return false;
			}

			for ( size_t i = 0; i < pBlock->m_Statics.GetStaticQty(); pStatic = NULL, ++i )
			{
				pStatic = pBlock->m_Statics.GetStatic(i);

				CPointMap ptTest(pStatic->m_x + pBlock->m_x, pStatic->m_y + pBlock->m_y, pStatic->m_z, m_map);
				if ( GetDist(ptTest) > 0 )
					continue;
				if ( pStatic->GetDispID() == pItemDef->GetDispID() )
					break;
			}
		}
		else
		{
			for ( size_t i = 0; i < pBlock->m_Statics.GetStaticQty(); pStatic = NULL, ++i )
			{
				pStatic = pBlock->m_Statics.GetStatic(i);

				CPointMap ptTest(pStatic->m_x + pBlock->m_x, pStatic->m_y + pBlock->m_y, pStatic->m_z, m_map);
				if ( GetDist(ptTest) > 0 )
					continue;
				if ( iStatic == 0 )
					break;

				--iStatic;
			}
		}

		if ( !pStatic )
		{
			sVal = "0";
			return true;
		}

		ITEMID_TYPE idTile = pStatic->GetDispID();

		SKIP_SEPARATORS(pszKey);
		if ( !strnicmp(pszKey, "ID", 2) || (*pszKey == '\0') )
		{
			sVal.FormatHex(idTile);
			return true;
		}
		else if ( !strnicmp(pszKey, "COLOR", 5) )
		{
			sVal.FormatHex(pStatic->m_wHue);
			return true;
		}
		else if ( !strnicmp(pszKey, "Z", 1) )
		{
			sVal.FormatVal(pStatic->m_z);
			return true;
		}

		CItemBase *pItemDef = CItemBase::FindItemBase(idTile);
		if ( !pItemDef )
		{
			DEBUG_ERR(("ITEMDEF 0%x not found\n", idTile));
			return false;
		}
		return pItemDef->r_WriteVal(pszKey, sVal, &g_Serv);
	}
	else if ( !strnicmp(pszKey, "COMPONENTS", 10) )
	{
		pszKey += 10;
		CItem *pItem = NULL;
		const CRegionBase *pRegion = NULL;
		const CGrayMulti *pMulti = NULL;
		const CUOMultiItemRecHS *pMultiItem = NULL;

		CRegionLinks rlinks;
		size_t iRegionQty = GetRegions(REGION_TYPE_MULTI, rlinks);

		if ( *pszKey == '\0' )
		{
			int iComponentQty = 0;
			for ( size_t i = 0; i < iRegionQty; ++i )
			{
				pRegion = rlinks.GetAt(i);
				if ( !pRegion )
					continue;

				pItem = pRegion->GetResourceID().ItemFind();
				if ( !pItem )
					continue;

				pMulti = g_Cfg.GetMultiItemDefs(pItem);
				if ( !pMulti )
					continue;

				const CPointMap ptMulti = pItem->GetTopPoint();
				size_t iQty = pMulti->GetItemCount();
				for ( size_t j = 0; j < iQty; ++j )
				{
					pMultiItem = pMulti->GetItem(j);
					if ( !pMultiItem )
						break;
					if ( pMultiItem->m_visible == 0 )
						continue;

					CPointMap ptTest(ptMulti.m_x + pMultiItem->m_dx, ptMulti.m_y + pMultiItem->m_dy, ptMulti.m_z + static_cast<signed char>(pMultiItem->m_dz), m_map);
					if ( GetDist(ptTest) > 0 )
						continue;

					++iComponentQty;
				}
			}
			sVal.FormatVal(iComponentQty);
			return true;
		}

		int iComponent = 0;
		int iResType = 0;

		SKIP_SEPARATORS(pszKey);
		if ( !strnicmp(pszKey, "FINDID", 6) )
		{
			pszKey += 6;
			SKIP_SEPARATORS(pszKey);

			iComponent = Exp_GetVal(pszKey);
			iResType = RES_GET_TYPE(iComponent);
			if ( iResType == RES_UNKNOWN )
				iResType = RES_ITEMDEF;

			SKIP_SEPARATORS(pszKey);
		}
		else
		{
			iComponent = Exp_GetVal(pszKey);
			iResType = RES_GET_TYPE(iComponent);
		}

		if ( iResType == RES_ITEMDEF )
		{
			const CItemBase *pItemDef = CItemBase::FindItemBase(static_cast<ITEMID_TYPE>(RES_GET_INDEX(iComponent)));
			if ( !pItemDef )
			{
				sVal = "0";
				return false;
			}

			for ( size_t i = 0; i < iRegionQty; ++i )
			{
				pRegion = rlinks.GetAt(i);
				if ( !pRegion )
					continue;

				pItem = pRegion->GetResourceID().ItemFind();
				if ( !pItem )
					continue;

				pMulti = g_Cfg.GetMultiItemDefs(pItem);
				if ( !pMulti )
					continue;

				const CPointMap ptMulti = pItem->GetTopPoint();
				size_t iQty = pMulti->GetItemCount();
				for ( size_t j = 0; j < iQty; pMultiItem = NULL, ++j )
				{
					pMultiItem = pMulti->GetItem(j);
					if ( !pMultiItem )
						break;
					if ( pMultiItem->m_visible == 0 )
						continue;

					CPointMap ptTest(ptMulti.m_x + pMultiItem->m_dx, ptMulti.m_y + pMultiItem->m_dy, ptMulti.m_z + static_cast<signed char>(pMultiItem->m_dz), m_map);
					if ( GetDist(ptTest) > 0 )
						continue;

					const CItemBase *pMultiItemDef = CItemBase::FindItemBase(pMultiItem->GetDispID());
					if ( pMultiItemDef && (pMultiItemDef->GetDispID() == pItemDef->GetDispID()) )
						break;
				}

				if ( pMultiItem )
					break;
			}
		}
		else
		{
			for ( size_t i = 0; i < iRegionQty; ++i )
			{
				pRegion = rlinks.GetAt(i);
				if ( !pRegion )
					continue;

				pItem = pRegion->GetResourceID().ItemFind();
				if ( !pItem )
					continue;

				pMulti = g_Cfg.GetMultiItemDefs(pItem);
				if ( !pMulti )
					continue;

				const CPointMap ptMulti = pItem->GetTopPoint();
				size_t iQty = pMulti->GetItemCount();
				for ( size_t j = 0; j < iQty; pMultiItem = NULL, ++j )
				{
					pMultiItem = pMulti->GetItem(j);
					if ( !pMultiItem )
						break;
					if ( pMultiItem->m_visible == 0 )
						continue;

					CPointMap ptTest(ptMulti.m_x + pMultiItem->m_dx, ptMulti.m_y + pMultiItem->m_dy, ptMulti.m_z + static_cast<signed char>(pMultiItem->m_dz), m_map);
					if ( GetDist(ptTest) > 0 )
						continue;

					if ( iComponent == 0 )
						break;

					--iComponent;
				}

				if ( pMultiItem )
					break;
			}
		}

		if ( !pMultiItem )
		{
			sVal = "0";
			return true;
		}

		ITEMID_TYPE idTile = pMultiItem->GetDispID();

		SKIP_SEPARATORS(pszKey);
		if ( !strnicmp(pszKey, "ID", 2) || (*pszKey == '\0') )
		{
			sVal.FormatHex(idTile);
			return true;
		}
		else if ( !strnicmp(pszKey, "MULTI", 5) )
		{
			pszKey += 5;
			if ( *pszKey != '\0' )
			{
				SKIP_SEPARATORS(pszKey);
				return pItem->r_WriteVal(pszKey, sVal, &g_Serv);
			}
			sVal.FormatHex(pItem->GetUID());
			return true;
		}
		else if ( !strnicmp(pszKey, "Z", 1) )
		{
			sVal.FormatVal(pItem->GetTopZ() + pMultiItem->m_dz);
			return true;
		}

		CItemBase *pItemDef = CItemBase::FindItemBase(idTile);
		if ( !pItemDef )
		{
			DEBUG_ERR(("ITEMDEF 0%x not found\n", idTile));
			return false;
		}
		return pItemDef->r_WriteVal(pszKey, sVal, &g_Serv);
	}

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	if ( index < 0 )
		return false;

	switch ( index )
	{
		case PT_X:
			sVal.FormatVal(m_x);
			break;
		case PT_Y:
			sVal.FormatVal(m_y);
			break;
		case PT_Z:
			sVal.FormatVal(m_z);
			break;
		case PT_M:
		case PT_MAP:
			sVal.FormatVal(m_map);
			break;
		case PT_ISNEARTYPE:
		{
			pszKey += 10;
			SKIP_SEPARATORS(pszKey);
			SKIP_ARGSEP(pszKey);
			IT_TYPE iType = static_cast<IT_TYPE>(g_Cfg.ResourceGetIndexType(RES_TYPEDEF, pszKey));

			SKIP_IDENTIFIERSTRING(pszKey);
			SKIP_SEPARATORS(pszKey);
			SKIP_ARGSEP(pszKey);

			int iDist = *pszKey ? Exp_GetVal(pszKey) : 0;
			bool fCheckMulti = *pszKey ? (Exp_GetVal(pszKey) != 0) : false;
			bool fLimitZ = *pszKey ? (Exp_GetVal(pszKey) != 0) : false;
			sVal.FormatVal(g_World.IsItemTypeNear(*this, iType, iDist, fCheckMulti, fLimitZ));
			break;
		}
		case PT_REGION:
		{
			if ( pszKey[6] && (pszKey[6] != '.') )
				return false;

			CRegionWorld *pRegionTemp = dynamic_cast<CRegionWorld *>(GetRegion(REGION_TYPE_AREA|REGION_TYPE_MULTI));
			if ( !pszKey[6] )
			{
				sVal.FormatVal(pRegionTemp ? 1 : 0);
				return true;
			}

			pszKey += 7;
			if ( pRegionTemp )
				return pRegionTemp->r_WriteVal(pszKey, sVal, &g_Serv);

			return false;
		}
		case PT_ROOM:
		{
			if ( pszKey[4] && (pszKey[4] != '.') )
				return false;

			CRegionBase *pRegionTemp = GetRegion(REGION_TYPE_ROOM);
			if ( !pszKey[4] )
			{
				sVal.FormatVal(pRegionTemp ? 1 : 0);
				return true;
			}

			pszKey += 5;
			if ( pRegionTemp )
				return pRegionTemp->r_WriteVal(pszKey, sVal, &g_Serv);

			return false;
		}
		case PT_SECTOR:
		{
			if ( pszKey[6] == '.' )
			{
				pszKey += 7;
				CSector *pSector = GetSector();
				if ( pSector )
					return pSector->r_WriteVal(pszKey, sVal, &g_Serv);
			}
			return false;
		}
		default:
		{
			const CUOMapMeter *pMapMeter = g_World.GetMapMeter(*this);
			if ( pMapMeter )
			{
				switch ( index )
				{
					case PT_TERRAIN:
					{
						pszKey += 7;
						if ( *pszKey == '.' )
						{
							SKIP_SEPARATORS(pszKey);
							if ( !strnicmp(pszKey, "Z", 1) )
							{
								sVal.FormatVal(pMapMeter->m_z);
								return true;
							}
							return false;
						}
						sVal.FormatHex(pMapMeter->m_wTerrainIndex);
						return true;
					}
					case PT_TYPE:
					{
						CItemTypeDef *pTypeDef = g_World.GetTerrainItemTypeDef(pMapMeter->m_wTerrainIndex);
						sVal = pTypeDef ? pTypeDef->GetResourceName() : "";
						return true;
					}
				}
			}
			return false;
		}
	}
	return true;
}

bool CPointBase::r_LoadVal(LPCTSTR pszKey, LPCTSTR pszArgs)
{
	ADDTOCALLSTACK("CPointBase::r_LoadVal");
	int index = FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	if ( index <= 0 )
		return false;

	switch ( index )
	{
		case 1:
			m_x = static_cast<signed short>(Exp_GetLLVal(pszArgs));
			break;
		case 2:
			m_y = static_cast<signed short>(Exp_GetLLVal(pszArgs));
			break;
		case 3:
			m_z = static_cast<signed char>(Exp_GetLLVal(pszArgs));
			break;
	}
	return true;
}

///////////////////////////////////////////////////////////
// CGRect

CSector *CGRect::GetSector(int i) const
{
	ADDTOCALLSTACK("CGRect::GetSector");
	if ( !g_MapList.IsMapSupported(m_map) )
		return NULL;

	int iSectorSize = g_MapList.GetSectorSize(m_map);
	if ( iSectorSize <= 0 )
		return NULL;

	CRectMap rect;
	rect.m_left = m_left & ~(iSectorSize - 1);
	rect.m_top = m_top & ~(iSectorSize - 1);
	rect.m_right = (m_right | (iSectorSize - 1)) + 1;
	rect.m_bottom = (m_bottom | (iSectorSize - 1)) + 1;
	rect.m_map = m_map;
	rect.NormalizeRectMax();

	int iWidth = rect.GetWidth() / iSectorSize;
	int iHeight = rect.GetHeight() / iSectorSize;
	int iSectorCols = g_MapList.GetSectorCols(m_map);
	int iSector = ((rect.m_top / iSectorSize) * iSectorCols) + (rect.m_left / iSectorSize);

	if ( i >= iWidth * iHeight )
		return !i ? g_World.GetSector(m_map, iSector) : NULL;

	return g_World.GetSector(m_map, iSector + ((i / iWidth) * iSectorCols) + (i % iWidth));
}

CPointBase CGRect::GetRectCorner(DIR_TYPE dir) const
{
	ADDTOCALLSTACK("CGRect::GetRectCorner");
	CPointBase pt;
	switch ( dir )
	{
		case DIR_N:
			pt.m_x = static_cast<signed short>((m_left + m_right) / 2);
			pt.m_y = static_cast<signed short>(m_top);
			break;
		case DIR_NE:
			pt.m_x = static_cast<signed short>(m_right);
			pt.m_y = static_cast<signed short>(m_top);
			break;
		case DIR_E:
			pt.m_x = static_cast<signed short>(m_right);
			pt.m_y = static_cast<signed short>((m_top + m_bottom) / 2);
			break;
		case DIR_SE:
			pt.m_x = static_cast<signed short>(m_right);
			pt.m_y = static_cast<signed short>(m_bottom);
			break;
		case DIR_S:
			pt.m_x = static_cast<signed short>((m_left + m_right) / 2);
			pt.m_y = static_cast<signed short>(m_bottom);
			break;
		case DIR_SW:
			pt.m_x = static_cast<signed short>(m_left);
			pt.m_y = static_cast<signed short>(m_bottom);
			break;
		case DIR_W:
			pt.m_x = static_cast<signed short>(m_left);
			pt.m_y = static_cast<signed short>((m_top + m_bottom) / 2);
			break;
		case DIR_NW:
			pt.m_x = static_cast<signed short>(m_left);
			pt.m_y = static_cast<signed short>(m_top);
			break;
		default:
			pt = GetCenter();
			break;
	}
	pt.m_z = 0;
	pt.m_map = static_cast<BYTE>(m_map);
	return pt;
}

size_t CGRect::Read(LPCTSTR pszVal)
{
	ADDTOCALLSTACK("CGRect::Read");
	TCHAR *pszTemp = Str_GetTemp();
	strncpy(pszTemp, pszVal, 30);
	pszTemp[29] = '\0';

	TCHAR *ppVal[5];
	size_t iArgQty = Str_ParseCmds(pszTemp, ppVal, COUNTOF(ppVal), " ,\t");

	switch ( iArgQty )
	{
		case 1:
			m_left = ATOI(ppVal[0]);
			m_top = 0;
			m_right = 0;
			m_bottom = 0;
			m_map = 0;
			break;
		case 2:
			m_left = ATOI(ppVal[0]);
			m_top = ATOI(ppVal[1]);
			m_right = 0;
			m_bottom = 0;
			m_map = 0;
			break;
		case 3:
			m_left = ATOI(ppVal[0]);
			m_top = ATOI(ppVal[1]);
			m_right = ATOI(ppVal[2]);
			m_bottom = 0;
			m_map = 0;
			break;
		case 4:
			m_left = ATOI(ppVal[0]);
			m_top = ATOI(ppVal[1]);
			m_right = ATOI(ppVal[2]);
			m_bottom = ATOI(ppVal[3]);
			m_map = 0;
			break;
		case 5:
			m_left = ATOI(ppVal[0]);
			m_top = ATOI(ppVal[1]);
			m_right = ATOI(ppVal[2]);
			m_bottom = ATOI(ppVal[3]);
			m_map = ATOI(ppVal[4]);
			if ( !g_MapList.IsMapSupported(m_map) )
			{
				DEBUG_ERR(("Unsupported map #%d (defaulting to 0)\n", m_map));
				m_map = 0;
			}
			break;
	}
	NormalizeRect();
	return iArgQty;
}

LPCTSTR CGRect::Write() const
{
	ADDTOCALLSTACK("CGRect::Write");
	TCHAR *pszTemp = Str_GetTemp();
	snprintf(pszTemp, 32, "%d,%d,%d,%d,%d", m_left, m_top, m_right, m_bottom, m_map);
	return pszTemp;
}

///////////////////////////////////////////////////////////
// CRectMap

void CRectMap::NormalizeRect()
{
	CGRect::NormalizeRect();
	NormalizeRectMax();
}

void CRectMap::NormalizeRectMax()
{
	CGRect::NormalizeRectMax(g_MapList.GetX(m_map), g_MapList.GetY(m_map));
}
