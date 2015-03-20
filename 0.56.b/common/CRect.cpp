//
// Crect.cpp
// Copyright Menace Software (www.menasoft.com).
// Common for client and server.
//

#include <cmath>
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
	g_Cfg.GetDefaultMsg(DEFMSG_MAP_DIR_8),
};

const int CPointBase::sm_Moves[DIR_QTY+1][2] =
{
	{  0, -1 }, // DIR_N
	{  1, -1 }, // DIR_NE
	{  1,  0 }, // DIR_E
	{  1,  1 }, // DIR_SE
	{  0,  1 }, // DIR_S
	{ -1,  1 }, // DIR_SW
	{ -1,  0 }, // DIR_W
	{ -1, -1 }, // DIR_NW
	{  0,  0 },	// DIR_QTY = here.
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
	return( abs(m_z-pt.m_z));
}
int CPointBase::GetDistZAdj( const CPointBase & pt ) const
{
	return( GetDistZ(pt) / (PLAYER_HEIGHT/2) );
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
	if ( !pt.IsValidPoint())
		return( SHRT_MAX );
	if ( ! IsSameMap( pt.m_map ))
		return( SHRT_MAX );

	return( GetDistBase( pt ));
}

int CPointBase::GetDistSightBase( const CPointBase & pt ) const // Distance between points based on UO sight
{
	int dx = abs(m_x-pt.m_x);
	int dy = abs(m_y-pt.m_y);
	return( maximum( dx, dy ));
}

int CPointBase::GetDistSight( const CPointBase & pt ) const // Distance between points based on UO sight
{
	ADDTOCALLSTACK("CPointBase::GetDistSight");
	if ( !pt.IsValidPoint())
		return( SHRT_MAX );
	if ( ! IsSameMap( pt.m_map ))
		return( SHRT_MAX );

	return( GetDistSightBase( pt ));
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

bool CPointBase::IsValidZ() const
{
	return( m_z > -UO_SIZE_Z && m_z < UO_SIZE_Z );
}

bool CPointBase::IsValidXY() const
{
	if ( m_x < 0 || m_x >= g_MapList.GetX(m_map) )
		return( false );
	if ( m_y < 0 || m_y >= g_MapList.GetY(m_map) )
		return( false );
	return( true );
}

bool CPointBase::IsValidPoint() const
{
	return( IsValidXY() && IsValidZ());
}

bool CPointBase::IsCharValid() const
{
	if ( m_z <= -UO_SIZE_Z || m_z >= UO_SIZE_Z )
		return( false );
	if (m_x <= 0 || m_x >= static_cast<signed short>(g_MapList.GetX(m_map)))
		return( false );
	if (m_y <= 0 || m_y >= static_cast<signed short>(g_MapList.GetY(m_map)))
		return( false );
	return( true );
}

void CPointBase::ValidatePoint()
{
	if ( m_x < 0 ) m_x = 0;
	if (m_x >= static_cast<signed short>(g_MapList.GetX(m_map))) m_x = static_cast<signed short>(g_MapList.GetX(m_map) - 1);
	if ( m_y < 0 ) m_y = 0;
	if (m_y >= static_cast<signed short>(g_MapList.GetY(m_map))) m_y = static_cast<signed short>(g_MapList.GetY(m_map) - 1);
}

bool CPointBase::IsSameMap( BYTE map ) const
{
	return( map == m_map );
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

void CPointBase::Set( WORD x, WORD y, signed char z, unsigned char map )
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
		pszKey	+= 7;
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
		if ( !*pszKey )
			pszKey	= "ID";

		ITEMID_TYPE idTile = pStatic->GetDispID();

		if ( !strnicmp( pszKey, "COLOR", 5 ) )
		{
			sVal.FormatHex( pStatic->m_wHue );
			return true;
		}
		else if ( !strnicmp( pszKey, "ID", 2 ) )
		{
			sVal.FormatHex( idTile );
			return true;
		}
		else if ( !strnicmp( pszKey, "Z", 1 ) )
		{
			sVal.FormatVal( pStatic->m_z );
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
		const CUOMultiItemRec2* pMultiItem = NULL;
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

					CPointMap ptTest(static_cast<WORD>(ptMulti.m_x + pMultiItem->m_dx), static_cast<WORD>(ptMulti.m_y + pMultiItem->m_dy), static_cast<signed char>(ptMulti.m_z + pMultiItem->m_dz), this->m_map);
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
		
		if ( strnicmp( pszKey, "FINDID", 6 ) == 0 )
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
					CPointMap ptTest(static_cast<WORD>(ptMulti.m_x + pMultiItem->m_dx), static_cast<WORD>(ptMulti.m_y + pMultiItem->m_dy), static_cast<signed char>(ptMulti.m_z + pMultiItem->m_dz), this->m_map);
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
					CPointMap ptTest(static_cast<WORD>(ptMulti.m_x + pMultiItem->m_dx), static_cast<WORD>(ptMulti.m_y + pMultiItem->m_dy), static_cast<signed char>(ptMulti.m_z + pMultiItem->m_dz), this->m_map);
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
		if ( !*pszKey )
			pszKey	= "ID";

		ITEMID_TYPE idTile = pMultiItem->GetDispID();

		if ( strnicmp( pszKey, "ID", 2 ) == 0 )
		{
			sVal.FormatHex( idTile );
			return true;
		}
		else if ( strnicmp( pszKey, "MULTI", 5 ) == 0 )
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
		else if ( strnicmp( pszKey, "Z", 1 ) == 0 )
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
			SKIP_SEPARATORS( pszKey );
			SKIP_ARGSEP( pszKey );

			int iType = g_Cfg.ResourceGetIndexType( RES_TYPEDEF, pszKey );
			int iDistance = 0;
			bool bCheckMulti = false;

			SKIP_IDENTIFIERSTRING( pszKey );
			SKIP_SEPARATORS( pszKey );
			SKIP_ARGSEP( pszKey );

			if ( *pszKey ) iDistance = Exp_GetVal(pszKey);
			if ( *pszKey ) bCheckMulti = Exp_GetVal(pszKey) != 0;
			sVal.FormatVal( g_World.IsItemTypeNear(*this, static_cast<IT_TYPE>(iType), iDistance, bCheckMulti));
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
							pszKey += strlen(sm_szLoadKeys[index]);
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
		case 0: m_map = static_cast<unsigned char>(iVal); break;
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

	m_x = static_cast<short>(ptSrc.m_x + IMULDIV( iSteps, dx, iDist2D ));
	m_y = static_cast<short>(ptSrc.m_y + IMULDIV( iSteps, dy, iDist2D ));
	return( iDist2D );
}

TCHAR * CPointBase::WriteUsed( TCHAR * pszBuffer ) const
{
	ADDTOCALLSTACK_INTENSIVE("CPointBase::WriteUsed");
	if ( m_map )
	{
		sprintf(pszBuffer, "%d,%d,%d,%d", m_x, m_y, m_z, m_map);
	}
	else if ( m_z )
	{
		sprintf(pszBuffer, "%d,%d,%d", m_x, m_y, m_z);
	}
	else
	{
		sprintf(pszBuffer, "%d,%d", m_x, m_y);
	}
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
				m_map = static_cast<unsigned char>(ATOI(ppVal[3]));
				if ( !g_MapList.m_maps[m_map] )
				{
					g_Log.EventError("Unsupported map #%d specified. Auto-fixing that to 0.\n", m_map);
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
	if ( !IsValidXY() )
	{
		g_Log.Event(LOGL_ERROR, "Point(%d,%d): trying to get a sector for point on map #%d out of bounds for this map(%d,%d). Defaulting to sector 0 of the map.\n",
			m_x, m_y, m_map, g_MapList.GetX(m_map), g_MapList.GetY(m_map));
		return g_World.GetSector(m_map, 0);
	}
	// Get the world Sector we are in.
	return g_World.GetSector(m_map, ((m_y / g_MapList.GetSectorSize(m_map) * g_MapList.GetSectorCols(m_map)) + ( m_x / g_MapList.GetSectorSize(m_map) )));
}

CRegionBase * CPointBase::GetRegion( DWORD dwType ) const
{
	ADDTOCALLSTACK("CPointBase::GetRegion");
	// What region in the current CSector am i in ?
	// We only need to update this every 8 or so steps ?
	// REGION_TYPE_AREA
	if ( ! IsValidPoint())
		return NULL;

	CSector *pSector = GetSector();
	if ( pSector )
		return pSector->GetRegion(*this, dwType);

	return NULL;
}

size_t CPointBase::GetRegions( DWORD dwType, CRegionLinks & rlinks ) const
{
	ADDTOCALLSTACK("CPointBase::GetRegions");
	if ( !IsValidPoint() )
		return 0;

	CSector *pSector = GetSector();
	if ( pSector )
		return pSector->GetRegions(*this, dwType, rlinks);

	return 0;
}

//*************************************************************************
// -CGRect

size_t CGRect::Read( LPCTSTR pszVal )
{
	ADDTOCALLSTACK("CGRect::Read");
	// parse reading the rectangle
	TCHAR *pszTemp = Str_GetTemp();
	strcpy( pszTemp, pszVal );
	TCHAR * ppVal[5];
	size_t i = Str_ParseCmds( pszTemp, ppVal, COUNTOF( ppVal ), " ,\t");
	switch (i)
	{
		case 5:
			m_map = ATOI(ppVal[4]);
			if (( m_map < 0 ) || ( m_map >= 256 ) || !g_MapList.m_maps[m_map] )
			{
				g_Log.EventError("Unsupported map #%d specified. Auto-fixing that to 0.\n", m_map);
				m_map = 0;
			}
			m_bottom = ATOI(ppVal[3]);
			m_right = ATOI(ppVal[2]);
			m_top =	ATOI(ppVal[1]);
			m_left = ATOI(ppVal[0]);
			break;
		case 4:
			m_map = 0;
			m_bottom = ATOI(ppVal[3]);
			m_right = ATOI(ppVal[2]);
			m_top =	ATOI(ppVal[1]);
			m_left = ATOI(ppVal[0]);
			break;
		case 3:
			m_map = 0;
			m_bottom = 0;
			m_right = ATOI(ppVal[2]);
			m_top =	ATOI(ppVal[1]);
			m_left = ATOI(ppVal[0]);
			break;
		case 2:
			m_map = 0;
			m_bottom = 0;
			m_right = 0;
			m_top =	ATOI(ppVal[1]);
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
	return( i );
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
	pt.m_z = 0;	// NOTE: remember this is a nonsense value.
	pt.m_map = static_cast<unsigned char>(m_map);
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
		case DIR_QTY:
			pt = GetCenter();
			break;
		default:
			break;
	}
	return( pt );
}

CSector * CGRect::GetSector( int i ) const	// ge all the sectors that make up this rect.
{
	ADDTOCALLSTACK("CGRect::GetSector");
	// get all the CSector(s) that overlap this rect.
	// RETURN: NULL = no more

	// Align new rect.
	CRectMap rect;
	rect.m_left = m_left &~ (g_MapList.GetSectorSize(m_map)-1);
	rect.m_right = ( m_right | (g_MapList.GetSectorSize(m_map)-1)) + 1;
	rect.m_top = m_top &~ (g_MapList.GetSectorSize(m_map)-1);
	rect.m_bottom = ( m_bottom | (g_MapList.GetSectorSize(m_map)-1)) + 1;
	rect.m_map = m_map;
	rect.NormalizeRectMax();

	int width = (rect.GetWidth()) / g_MapList.GetSectorSize(m_map);
	ASSERT(width <= g_MapList.GetSectorCols(m_map));
	int height = (rect.GetHeight()) / g_MapList.GetSectorSize(m_map);
	ASSERT(height <= g_MapList.GetSectorRows(m_map));

	int iBase = (( rect.m_top / g_MapList.GetSectorSize(m_map)) * g_MapList.GetSectorCols(m_map)) + ( rect.m_left / g_MapList.GetSectorSize(m_map) );

	if ( i >= ( height * width ))
	{
		if ( ! i )
		{
			return( g_World.GetSector(m_map, iBase) );
		}
		return( NULL );
	}

	int indexoffset = (( i / width ) * g_MapList.GetSectorCols(m_map)) + ( i % width );

	return g_World.GetSector(m_map, iBase+indexoffset);
}

//*************************************************************************
// -CRectMap

void CRectMap::NormalizeRect()
{
	ADDTOCALLSTACK("CRectMap::NormalizeRect");
	CGRect::NormalizeRect();
	NormalizeRectMax();
}

void CRectMap::NormalizeRectMax()
{
	ADDTOCALLSTACK("CRectMap::NormalizeRectMax");
	CGRect::NormalizeRectMax( g_MapList.GetX(m_map), g_MapList.GetY(m_map));
}
