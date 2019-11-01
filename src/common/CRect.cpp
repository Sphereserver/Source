#include "../graysvr/graysvr.h"

//*************************************************************************
// -CPointBase

LPCTSTR CPointBase::sm_szDirs[DIR_QTY+1] =
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

const int CPointBase::sm_Moves[DIR_QTY+1][2] =
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


LPCTSTR const CPointBase::sm_szLoadKeys[PT_QTY+1] =
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

void CPointBase::InitPoint()
{
	m_x = -1;	// invalid location.
	m_y = -1;
	m_z = 0;
	m_map = 0;
}
void CPointBase::ZeroPoint()
{
	m_x = 0;	// invalid location.
	m_y = 0;
	m_z = 0;
	m_map = 0;
}
int CPointBase::GetDistZ( const CPointBase & pt ) const
{
	return abs(m_z - pt.m_z);
}
int CPointBase::GetDistZAdj( const CPointBase & pt ) const
{
	return GetDistZ(pt) / (PLAYER_HEIGHT / 2);
}
int CPointBase::GetDistBase( const CPointBase & pt ) const // Distance between points
{
	// Do not consider z or m_map.
	int dx = abs(m_x - pt.m_x);
	int dy = abs(m_y - pt.m_y);

	return maximum(dx, dy);

	// What the heck?
	/*double dist = sqrt(static_cast<double>((dx * dx) + (dy * dy)));

	return static_cast<int>(( (dist - floor(dist)) > 0.5 ) ? (ceil(dist)) : (floor(dist)));*/
	// Return the real distance return((int) sqrt(dx*dx+dy*dy+dz*dz));
}

int CPointBase::GetDist( const CPointBase & pt ) const // Distance between points
{
	ADDTOCALLSTACK("CPointBase::GetDist");
	// Get the basic 2d distance.
	if ( !pt.IsValidPoint() )
		return SHRT_MAX;
	if ( pt.m_map != m_map )
		return SHRT_MAX;

	return GetDistBase(pt);
}

int CPointBase::GetDist3D( const CPointBase & pt ) const // Distance between points
{
	ADDTOCALLSTACK("CPointBase::GetDist3D");
	// OK, 1 unit of Z is not the same (real life) distance as 1
	// unit of X (or Y)
	int dist = GetDist(pt);

	// Get the deltas and correct the Z for height first
	int dz = GetDistZAdj(pt); // Take player height into consideration
			
	return maximum(dz, dist);
	// What the heck?
	/*double realdist = sqrt(static_cast<double>((dist * dist) + (dz * dz)));
	
	return static_cast<int>(( (realdist - floor(realdist)) > 0.5 ) ? (ceil(realdist)) : (floor(realdist)));*/
}

bool CPointBase::IsValidXY() const
{
	if ( m_x < 0 || m_x >= g_MapList.GetX(m_map) )
		return false;
	if ( m_y < 0 || m_y >= g_MapList.GetY(m_map) )
		return false;
	return true;
}

bool CPointBase::IsValidZ() const
{
	return ((m_z > UO_SIZE_MIN_Z) && (m_z < UO_SIZE_Z));
}

bool CPointBase::IsValidPoint() const
{
	return (IsValidXY() && IsValidZ());
}

void CPointBase::ValidatePoint()
{
	if ( m_x < 0 )
		m_x = 0;
	if ( m_x >= static_cast<signed short>(g_MapList.GetX(m_map)) )
		m_x = static_cast<signed short>(g_MapList.GetX(m_map) - 1);

	if ( m_y < 0 )
		m_y = 0;
	if ( m_y >= static_cast<signed short>(g_MapList.GetY(m_map)) )
		m_y = static_cast<signed short>(g_MapList.GetY(m_map) - 1);
}

bool CPointBase::IsSame2D( const CPointBase & pt ) const
{
	return( m_x == pt.m_x && m_y == pt.m_y );
}

void CPointBase::Set( const CPointBase & pt )
{
	m_x = pt.m_x;
	m_y = pt.m_y;
	m_z = pt.m_z;
	m_map = pt.m_map;
}

void CPointBase::Set(signed short x, signed short y, signed char z, BYTE map )
{
	m_x = x;
	m_y = y;
	m_z = z;
	m_map = map;
}

void CPointBase::Move( DIR_TYPE dir )
{
	// Move a point in a direction.
	ASSERT( dir <= DIR_QTY );
	m_x += static_cast<signed short>(sm_Moves[dir][0]);
	m_y += static_cast<signed short>(sm_Moves[dir][1]);
}

void CPointBase::MoveN( DIR_TYPE dir, int amount )
{
	// Move a point in a direction.
	ASSERT( dir <= DIR_QTY );
	m_x += static_cast<signed short>(sm_Moves[dir][0] * amount);
	m_y += static_cast<signed short>(sm_Moves[dir][1] * amount);
}

bool CPointBase::r_WriteVal( LPCTSTR pszKey, CGString & sVal ) const
{
	ADDTOCALLSTACK("CPointBase::r_WriteVal");
	if ( !strnicmp( pszKey, "STATICS", 7 ) )
	{
		pszKey += 7;
		const CGrayMapBlock * pBlock = g_World.GetMapBlock( *(this) );
		if ( !pBlock ) return false;

		if ( *pszKey == '\0' )
		{
			int iStaticQty = 0;
			for ( size_t i = 0; i < pBlock->m_Statics.GetStaticQty(); i++ )
			{
				const CUOStaticItemRec * pStatic = pBlock->m_Statics.GetStatic( i );
				CPointMap ptTest( pStatic->m_x+pBlock->m_x, pStatic->m_y+pBlock->m_y, pStatic->m_z, this->m_map );
				if ( this->GetDist( ptTest ) > 0 )
					continue;
				iStaticQty++;
			}

			sVal.FormatVal( iStaticQty );
			return true;
		}

		SKIP_SEPARATORS( pszKey );

		const CUOStaticItemRec * pStatic = NULL;
		int iStatic = 0;
		int type = 0;
		
		if ( !strnicmp( pszKey, "FINDID", 6 ) )
		{
			pszKey += 6;
			SKIP_SEPARATORS( pszKey );
			iStatic = Exp_GetVal( pszKey );
			type = RES_GET_TYPE( iStatic );
			if ( type == 0 )
				type = RES_ITEMDEF;
			SKIP_SEPARATORS( pszKey );
		}
		else
		{
			iStatic = Exp_GetVal( pszKey );
			type = RES_GET_TYPE( iStatic );
		}
		
		if ( type == RES_ITEMDEF )
		{
			const CItemBase * pItemDef = CItemBase::FindItemBase(static_cast<ITEMID_TYPE>(RES_GET_INDEX(iStatic)));
			if ( !pItemDef )
			{
				sVal.FormatVal( 0 );
				return false;
			}
			for ( size_t i = 0; i < pBlock->m_Statics.GetStaticQty(); pStatic = NULL, i++ )
			{
				pStatic = pBlock->m_Statics.GetStatic( i );
				CPointMap ptTest( pStatic->m_x+pBlock->m_x, pStatic->m_y+pBlock->m_y, pStatic->m_z, this->m_map);
				if ( this->GetDist( ptTest ) > 0 )
					continue;
				if ( pStatic->GetDispID() == pItemDef->GetDispID() )
					break;
			}
		}
		else
		{
			for ( size_t i = 0; i < pBlock->m_Statics.GetStaticQty(); pStatic = NULL, i++ )
			{
				pStatic = pBlock->m_Statics.GetStatic( i );
				CPointMap ptTest( pStatic->m_x+pBlock->m_x, pStatic->m_y+pBlock->m_y, pStatic->m_z, this->m_map);
				if ( this->GetDist( ptTest ) > 0 )
					continue;
				if ( iStatic == 0 )
					break;
				iStatic--;
			}
		}

		if ( !pStatic )
		{
			sVal.FormatHex(0);
			return true;
		}

		SKIP_SEPARATORS( pszKey );

		ITEMID_TYPE idTile = pStatic->GetDispID();

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

		// Check the script def for the item.
		CItemBase * pItemDef = CItemBase::FindItemBase( idTile );
		if ( pItemDef == NULL )
		{
			DEBUG_ERR(("Must have ITEMDEF section for item ID 0%x\n", idTile ));
			return false;
		}

		return pItemDef->r_WriteVal( pszKey, sVal, &g_Serv );
	}
	else if ( !strnicmp( pszKey, "COMPONENTS", 10) )
	{
		pszKey += 10;
		
		CRegionLinks rlinks;
		const CRegionBase* pRegion = NULL;
		CItem* pItem = NULL;
		const CGrayMulti* pMulti = NULL;
		const CUOMultiItemRecHS *pMultiItem = NULL;
		size_t iMultiQty = GetRegions(REGION_TYPE_MULTI, rlinks);

		if ( *pszKey == '\0' )
		{
			int iComponentQty = 0;
			for (size_t i = 0; i < iMultiQty; i++)
			{
				pRegion = rlinks.GetAt(i);
				if (pRegion == NULL)
					continue;

				pItem = pRegion->GetResourceID().ItemFind();
				if (pItem == NULL)
					continue;

				const CPointMap ptMulti = pItem->GetTopPoint();
				pMulti = g_Cfg.GetMultiItemDefs(pItem);
				if (pMulti == NULL)
					continue;

				size_t iQty = pMulti->GetItemCount();
				for (size_t ii = 0; ii < iQty; ii++)
				{
					pMultiItem = pMulti->GetItem(ii);
					if (pMultiItem == NULL)
						break;
					if (pMultiItem->m_visible == 0)
						continue;
					CPointMap ptTest(ptMulti.m_x + pMultiItem->m_dx, ptMulti.m_y + pMultiItem->m_dy, ptMulti.m_z + static_cast<signed char>(pMultiItem->m_dz), m_map);
					if (GetDist(ptTest) > 0)
						continue;

					iComponentQty++;
				}
			}

			sVal.FormatVal( iComponentQty );
			return true;
		}

		SKIP_SEPARATORS( pszKey );

		int iComponent = 0;
		int type = 0;
		
		if ( !strnicmp(pszKey, "FINDID", 6) )
		{
			pszKey += 6;
			SKIP_SEPARATORS( pszKey );
			iComponent = Exp_GetVal( pszKey );
			type = RES_GET_TYPE( iComponent );
			if ( type == 0 )
				type = RES_ITEMDEF;
			SKIP_SEPARATORS( pszKey );
		}
		else
		{
			iComponent = Exp_GetVal( pszKey );
			type = RES_GET_TYPE( iComponent );
		}
		
		if ( type == RES_ITEMDEF )
		{
			const CItemBase * pItemDef = CItemBase::FindItemBase(static_cast<ITEMID_TYPE>(RES_GET_INDEX(iComponent)));
			if ( pItemDef == NULL )
			{
				sVal.FormatVal( 0 );
				return false;
			}
			
			for (size_t i = 0; i < iMultiQty; i++)
			{
				pRegion = rlinks.GetAt(i);
				if (pRegion == NULL)
					continue;

				pItem = pRegion->GetResourceID().ItemFind();
				if (pItem == NULL)
					continue;

				const CPointMap ptMulti = pItem->GetTopPoint();
				pMulti = g_Cfg.GetMultiItemDefs(pItem);
				if (pMulti == NULL)
					continue;

				size_t iQty = pMulti->GetItemCount();
				for (size_t ii = 0; ii < iQty; pMultiItem = NULL, ii++)
				{
					pMultiItem = pMulti->GetItem(ii);
					if (pMultiItem == NULL)
						break;
					if (pMultiItem->m_visible == 0)
						continue;
					CPointMap ptTest(ptMulti.m_x + pMultiItem->m_dx, ptMulti.m_y + pMultiItem->m_dy, ptMulti.m_z + static_cast<signed char>(pMultiItem->m_dz), m_map);
					if (GetDist(ptTest) > 0)
						continue;

					const CItemBase* pMultiItemDef = CItemBase::FindItemBase(pMultiItem->GetDispID());
					if (pMultiItemDef != NULL && pMultiItemDef->GetDispID() == pItemDef->GetDispID())
						break;
				}

				if (pMultiItem != NULL)
					break;
			}
		}
		else
		{
			for (size_t i = 0; i < iMultiQty; i++)
			{
				pRegion = rlinks.GetAt(i);
				if (pRegion == NULL)
					continue;

				pItem = pRegion->GetResourceID().ItemFind();
				if (pItem == NULL)
					continue;

				const CPointMap ptMulti = pItem->GetTopPoint();
				pMulti = g_Cfg.GetMultiItemDefs(pItem);
				if (pMulti == NULL)
					continue;

				size_t iQty = pMulti->GetItemCount();
				for (size_t ii = 0; ii < iQty; pMultiItem = NULL, ii++)
				{
					pMultiItem = pMulti->GetItem(ii);
					if (pMultiItem == NULL)
						break;
					if (pMultiItem->m_visible == 0)
						continue;
					CPointMap ptTest(ptMulti.m_x + pMultiItem->m_dx, ptMulti.m_y + pMultiItem->m_dy, ptMulti.m_z + static_cast<signed char>(pMultiItem->m_dz), m_map);
					if (GetDist(ptTest) > 0)
						continue;

					if (iComponent == 0)
						break;

					iComponent--;
				}

				if (pMultiItem != NULL)
					break;
			}
		}

		if ( pMultiItem == NULL )
		{
			sVal.FormatHex(0);
			return true;
		}

		SKIP_SEPARATORS( pszKey );

		ITEMID_TYPE idTile = pMultiItem->GetDispID();

		if ( !strnicmp(pszKey, "ID", 2) || (*pszKey == '\0') )
		{
			sVal.FormatHex( idTile );
			return true;
		}
		else if ( !strnicmp(pszKey, "MULTI", 5) )
		{
			pszKey += 5;
			if (*pszKey != '\0')
			{
				SKIP_SEPARATORS(pszKey);
				return pItem->r_WriteVal( pszKey, sVal, &g_Serv );
			}

			sVal.FormatHex( pItem->GetUID() );
			return true;
		}
		else if ( !strnicmp(pszKey, "Z", 1) )
		{
			sVal.FormatVal( pItem->GetTopZ() + pMultiItem->m_dz );
			return true;
		}

		// Check the script def for the item.
		CItemBase * pItemDef = CItemBase::FindItemBase( idTile );
		if ( pItemDef == NULL )
		{
			DEBUG_ERR(("Must have ITEMDEF section for item ID 0%x\n", idTile ));
			return false;
		}

		return pItemDef->r_WriteVal( pszKey, sVal, &g_Serv );
	}
	
	int index = FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 );
	if ( index < 0 )
		return false;

	switch ( index )
	{
		case PT_M:
		case PT_MAP:
			sVal.FormatVal(m_map);
			break;
		case PT_X:
			sVal.FormatVal(m_x);
			break;
		case PT_Y:
			sVal.FormatVal(m_y);
			break;
		case PT_Z:
			sVal.FormatVal(m_z);
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

			int iDistance = *pszKey ? Exp_GetVal(pszKey) : 0;
			bool bCheckMulti = *pszKey ? (Exp_GetVal(pszKey) != 0) : false;
			bool bLimitZ = *pszKey ? (Exp_GetVal(pszKey) != 0) : false;
			sVal.FormatVal(g_World.IsItemTypeNear(*this, iType, iDistance, bCheckMulti, bLimitZ));
			break;
		}
		case PT_REGION:
		{
			// Check that the syntax is correct.
			if ( pszKey[6] && pszKey[6] != '.' )
				return false;

			CRegionWorld * pRegionTemp = dynamic_cast <CRegionWorld*>(this->GetRegion(REGION_TYPE_AREA | REGION_TYPE_MULTI));

			if ( !pszKey[6] )
			{
				// We're just checking if the reference is valid.
				sVal.FormatVal( pRegionTemp? 1:0 );
				return true;
			}
			
			// We're trying to retrieve a property from the region.
			pszKey += 7;
			if ( pRegionTemp )
				return pRegionTemp->r_WriteVal( pszKey, sVal, &g_Serv );

			return false;
		}
		case PT_ROOM:
		{
			if ( pszKey[4] && pszKey[4] != '.' )
				return false;

			CRegionBase * pRegionTemp = this->GetRegion( REGION_TYPE_ROOM );

			if ( !pszKey[4] )
			{
				sVal.FormatVal( pRegionTemp? 1:0 );
				return true;
			}

			pszKey += 5;
			if ( pRegionTemp )
				return pRegionTemp->r_WriteVal( pszKey, sVal, &g_Serv );

			return false;
		}
		case PT_SECTOR:
		{
			if ( pszKey[6] == '.' )
			{
				pszKey += 7;
				CSector * pSectorTemp = this->GetSector();
				if (pSectorTemp)
					return pSectorTemp->r_WriteVal(pszKey, sVal, &g_Serv);
			}
			return false;
		}
		default:
		{
			const CUOMapMeter * pMeter = g_World.GetMapMeter(*this);
			if ( pMeter )
			{
				switch( index )
				{
					case PT_TYPE:
						{
							CItemTypeDef * pTypeDef = g_World.GetTerrainItemTypeDef( pMeter->m_wTerrainIndex );
							if ( pTypeDef != NULL )
								sVal = pTypeDef->GetResourceName();
							else
								sVal = "";
						} return true;
					case PT_TERRAIN:
						{
							pszKey += 7;
							if ( *pszKey == '.' )	// do we have an argument?
							{
								SKIP_SEPARATORS( pszKey );
								if ( !strnicmp( pszKey, "Z", 1 ))
								{
									sVal.FormatVal( pMeter->m_z );
									return( true );
								}
								
								return( false );
							}
							else
							{
								sVal.FormatHex( pMeter->m_wTerrainIndex );
							}
						} return true;
				}
			}
			return false;
		}
	}

	return true;
}

bool CPointBase::r_LoadVal( LPCTSTR pszKey, LPCTSTR pszArgs )
{
	ADDTOCALLSTACK("CPointBase::r_LoadVal");
	int index = FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 );
	if ( index <= 0 )
	{
		return( false );
	}
	int iVal = Exp_GetVal(pszArgs);
	switch (index)
	{
		case 1: m_x = static_cast<signed short>(iVal); break;
		case 2: m_y = static_cast<signed short>(iVal); break;
		case 3: m_z = static_cast<signed char>(iVal); break;
	}
	return( true );
}


DIR_TYPE CPointBase::GetDir( const CPointBase & pt, DIR_TYPE DirDefault ) const // Direction to point pt
{
	ADDTOCALLSTACK("CPointBase::GetDir");
	// Get the 2D direction between points.
	int dx = (m_x-pt.m_x);
	int dy = (m_y-pt.m_y);

	int ax = abs(dx);
	int ay = abs(dy);

	if ( ay > ax )
	{
		if ( ! ax )
		{
			return(( dy > 0 ) ? DIR_N : DIR_S );
		}
		int slope = ay / ax;
		if ( slope > 2 )
			return(( dy > 0 ) ? DIR_N : DIR_S );
		if ( dx > 0 )	// westish
		{
			return(( dy > 0 ) ? DIR_NW : DIR_SW );
		}
		return(( dy > 0 ) ? DIR_NE : DIR_SE );
	}
	else
	{
		if ( ! ay )
		{
			if ( ! dx )
				return( DirDefault );	// here ?
			return(( dx > 0 ) ? DIR_W : DIR_E );
		}
		int slope = ax / ay;
		if ( slope > 2 )
			return(( dx > 0 ) ? DIR_W : DIR_E );
		if ( dy > 0 )
		{
			return(( dx > 0 ) ? DIR_NW : DIR_NE );
		}
		return(( dx > 0 ) ? DIR_SW : DIR_SE );
	}
}

int CPointBase::StepLinePath( const CPointBase & ptSrc, int iSteps )
{
	ADDTOCALLSTACK("CPointBase::StepLinePath");
	// Take x steps toward this point.
	int dx = m_x - ptSrc.m_x;
	int dy = m_y - ptSrc.m_y;
	int iDist2D = GetDist( ptSrc );
	if ( ! iDist2D )
		return 0;

	m_x = ptSrc.m_x + static_cast<signed short>(IMULDIV(iSteps, dx, iDist2D));
	m_y = ptSrc.m_y + static_cast<signed short>(IMULDIV(iSteps, dy, iDist2D));
	return( iDist2D );
}

TCHAR * CPointBase::WriteUsed( TCHAR * pszBuffer ) const
{
	ADDTOCALLSTACK_INTENSIVE("CPointBase::WriteUsed");
	if ( m_map )
		sprintf(pszBuffer, "%hd,%hd,%hhd,%hhu", m_x, m_y, m_z, m_map);
	else if ( m_z )
		sprintf(pszBuffer, "%hd,%hd,%hhd", m_x, m_y, m_z);
	else
		sprintf(pszBuffer, "%hd,%hd", m_x, m_y);

	return pszBuffer;
}

LPCTSTR CPointBase::WriteUsed() const
{
	return( WriteUsed( Str_GetTemp()));
}

size_t CPointBase::Read( TCHAR * pszVal )
{
	ADDTOCALLSTACK("CPointBase::Read");
	// parse reading the point
	// NOTE: do not use = as a separator here !
	m_z = 0;
	m_map = 0;
	TCHAR * ppVal[4];
	size_t iArgs = Str_ParseCmds( pszVal, ppVal, COUNTOF( ppVal ), " ,\t" );
	switch ( iArgs )
	{
		default:
		case 4:	// m_map
			if ( IsDigit(ppVal[3][0]))
			{
				m_map = static_cast<BYTE>(ATOI(ppVal[3]));
				if ( !g_MapList.IsMapSupported(m_map) )
				{
					g_Log.EventError("Unsupported map #%hhu specified (defaulting to 0)\n", m_map);
					m_map = 0;
				}
			}
		case 3: // m_z
			if ( IsDigit(ppVal[2][0]) || ppVal[2][0] == '-' )
			{
				m_z = static_cast<signed char>(ATOI(ppVal[2]));
			}
		case 2:
			m_y = static_cast<signed short>(ATOI(ppVal[1]));
		case 1:
			m_x = static_cast<signed short>(ATOI(ppVal[0]));
		case 0:
			break;
	}
	return( iArgs );
}

CSector * CPointBase::GetSector() const
{
	ADDTOCALLSTACK("CPointBase::GetSector");
	if ( !g_MapList.IsMapSupported(m_map) )
	{
		g_Log.Event(LOGL_ERROR, "Point(%hd,%hd): trying to get a sector on unsupported map #%hhu\n", m_x, m_y, m_map);
		return NULL;
	}
	else if ( !IsValidXY() )
	{
		g_Log.Event(LOGL_ERROR, "Point(%hd,%hd): trying to get a sector out of map #%hhu bounds %d,%d (defaulting to sector 0)\n", m_x, m_y, m_map, g_MapList.GetX(m_map), g_MapList.GetY(m_map));
		return g_World.GetSector(m_map, 0);
	}

	int iSectorSize = g_MapList.GetSectorSize(m_map);
	if ( iSectorSize <= 0 )
	{
		g_Log.Event(LOGL_ERROR, "Point(%hd,%hd): trying to get a sector with no size on map #%hhu\n", m_x, m_y, m_map);
		return NULL;
	}

	return g_World.GetSector(m_map, (m_y / iSectorSize * g_MapList.GetSectorCols(m_map)) + (m_x / iSectorSize));
}

CRegionBase *CPointBase::GetRegion(BYTE bType) const
{
	ADDTOCALLSTACK("CPointBase::GetRegion");
	// What region in the current CSector am i in ?
	// We only need to update this every 8 or so steps ?
	// REGION_TYPE_AREA
	if ( ! IsValidPoint())
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

//*************************************************************************
// -CGRect

size_t CGRect::Read( LPCTSTR pszVal )
{
	ADDTOCALLSTACK("CGRect::Read");
	// parse reading the rectangle
	TCHAR *pszTemp = Str_GetTemp();
	strncpy(pszTemp, pszVal, 30);

	TCHAR *ppVal[5];
	size_t iArgQty = Str_ParseCmds(pszTemp, ppVal, COUNTOF(ppVal), " ,\t");

	switch ( iArgQty )
	{
		case 5:
			m_map = ATOI(ppVal[4]);
			if ( !g_MapList.IsMapSupported(m_map) )
			{
				g_Log.EventError("Unsupported map #%d specified (defaulting to 0)\n", m_map);
				m_map = 0;
			}
			m_bottom = ATOI(ppVal[3]);
			m_right = ATOI(ppVal[2]);
			m_top = ATOI(ppVal[1]);
			m_left = ATOI(ppVal[0]);
			break;
		case 4:
			m_map = 0;
			m_bottom = ATOI(ppVal[3]);
			m_right = ATOI(ppVal[2]);
			m_top = ATOI(ppVal[1]);
			m_left = ATOI(ppVal[0]);
			break;
		case 3:
			m_map = 0;
			m_bottom = 0;
			m_right = ATOI(ppVal[2]);
			m_top = ATOI(ppVal[1]);
			m_left = ATOI(ppVal[0]);
			break;
		case 2:
			m_map = 0;
			m_bottom = 0;
			m_right = 0;
			m_top = ATOI(ppVal[1]);
			m_left = ATOI(ppVal[0]);
			break;
		case 1:
			m_map = 0;
			m_bottom = 0;
			m_right = 0;
			m_top = 0;
			m_left = ATOI(ppVal[0]);
			break;
	}
	NormalizeRect();
	return iArgQty;
}

LPCTSTR CGRect::Write() const
{
	ADDTOCALLSTACK("CGRect::Write");
	return( Write( Str_GetTemp()));
}

CPointBase CGRect::GetRectCorner( DIR_TYPE dir ) const
{
	ADDTOCALLSTACK("CGRect::GetRectCorner");
	// Get the point if a directional corner of the CRectMap.
	CPointBase pt;
	pt.m_z = 0;
	pt.m_map = static_cast<BYTE>(m_map);
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
	return( pt );
}

CSector * CGRect::GetSector( int i ) const	// ge all the sectors that make up this rect.
{
	ADDTOCALLSTACK("CGRect::GetSector");
	// get all the CSector(s) that overlap this rect.
	// RETURN: NULL = no more

	if ( !g_MapList.IsMapSupported(m_map) )
		return NULL;

	int iSectorSize = g_MapList.GetSectorSize(m_map);
	if ( iSectorSize <= 0 )
		return NULL;

	// Align new rect
	CRectMap rect;
	rect.m_left = m_left & ~(iSectorSize - 1);
	rect.m_right = (m_right | (iSectorSize - 1)) + 1;
	rect.m_top = m_top & ~(iSectorSize - 1);
	rect.m_bottom = (m_bottom | (iSectorSize - 1)) + 1;
	rect.m_map = m_map;
	rect.NormalizeRectMax();

	int iWidth = rect.GetWidth() / iSectorSize;
	int iHeight = rect.GetHeight() / iSectorSize;
	int iSectorCols = g_MapList.GetSectorCols(m_map);

	int iBase = ((rect.m_top / iSectorSize) * iSectorCols) + (rect.m_left / iSectorSize);

	if ( i >= iWidth * iHeight )
		return !i ? g_World.GetSector(m_map, iBase) : NULL;

	int iOffset = ((i / iWidth) * iSectorCols) + (i % iWidth);
	return g_World.GetSector(m_map, iBase + iOffset);
}

//*************************************************************************
// -CRectMap

void CRectMap::NormalizeRect()
{
	//ADDTOCALLSTACK("CRectMap::NormalizeRect");
	CGRect::NormalizeRect();
	NormalizeRectMax();
}

void CRectMap::NormalizeRectMax()
{
	//ADDTOCALLSTACK("CRectMap::NormalizeRectMax");
	CGRect::NormalizeRectMax(g_MapList.GetX(m_map), g_MapList.GetY(m_map));
}
