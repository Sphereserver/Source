//
// Crect.cpp
// Copyright Menace Software (www.menasoft.com).
// Common for client and server.
//

#include "../graysvr/graysvr.h"

//*************************************************************************
// -CPointBase


LPCTSTR const CPointBase::sm_szDirs[DIR_QTY+1] =
{
	"North",
	"North East",
	"East",
	"South East",
	"South",
	"South West",
	"West",
	"North West",
	"Here",
};
/*
LPCTSTR const CPointBase::sm_szDirs[DIR_QTY+1] =
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
*/
const int CPointBase::sm_Moves[DIR_QTY+1][2] =
{
	 0, -1, // DIR_N
	 1, -1, // DIR_NE
	 1,  0, // DIR_E
	 1,  1, // DIR_SE
	 0,  1, // DIR_S
	-1,  1, // DIR_SW
	-1,  0, // DIR_W
	-1, -1, // DIR_NW
 	 0,	 0,	// DIR_QTY = here.
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
	PT_QTY,
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
	NULL,
};

bool CPointBase::r_WriteVal( LPCTSTR pszKey, CGString & sVal ) const
{
	ADDTOCALLSTACK("CPointBase::r_WriteVal");
	if ( !strnicmp( pszKey, "STATICS", 7 ) )
	{
		pszKey	+= 7;
		const CGrayMapBlock * pBlock	= g_World.GetMapBlock( *(this) );
		if ( !pBlock ) return false;

		if ( *pszKey == '\0' )
		{
			int		iStaticQty	= 0;
			for ( int i = 0; i <  pBlock->m_Statics.GetStaticQty(); i++ )
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

		const CUOStaticItemRec * pStatic	= NULL;


		int		iStatic	= 0;
		int		type	= 0;
		
		if ( !strnicmp( pszKey, "FINDID", 6 ) )
		{
			pszKey	+= 6;
			SKIP_SEPARATORS( pszKey );
			iStatic	= Exp_GetVal( pszKey );
			type	= RES_GET_TYPE( iStatic );
			if ( type == 0 )
				type	= RES_ITEMDEF;
			SKIP_SEPARATORS( pszKey );
		}
		else
		{
			iStatic	= Exp_GetVal( pszKey );
			type	= RES_GET_TYPE( iStatic );
		}
		
		if ( type == RES_ITEMDEF )
		{
			CItemBase * pItemDef	= CItemBase::FindItemBase( (ITEMID_TYPE) RES_GET_INDEX(iStatic) );
			if ( !pItemDef )
			{
				sVal.FormatVal( 0 );
				return false;
			}
			for ( int i = 0; i < pBlock->m_Statics.GetStaticQty(); pStatic	= NULL, i++ )
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
			for ( int i = 0; i <  pBlock->m_Statics.GetStaticQty(); pStatic	= NULL, i++ )
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
			DEBUG_ERR(("Must have ITEMDEF section for item ID %x\n", idTile ));
			return false;
		}

		return pItemDef->r_WriteVal( pszKey, sVal, &g_Serv );
	}
	
	int		index = FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 );
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

			SKIP_IDENTIFIERSTRING( pszKey );
			SKIP_SEPARATORS( pszKey );
			SKIP_ARGSEP( pszKey );

			if ( *pszKey ) iDistance = Exp_GetVal(pszKey);
			sVal.FormatVal( g_World.IsItemTypeNear(*this, (IT_TYPE) iType, iDistance));
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
							CItemTypeDef *	pTypeDef	= g_World.GetTerrainItemTypeDef( pMeter->m_wTerrainIndex );
							sVal.Format( pTypeDef ? pTypeDef->GetResourceName() : "" );
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
		case 0: m_map = iVal; break;
		case 1: m_x = iVal; break;
		case 2: m_y = iVal; break;
		case 3: m_z = iVal; break;
	}
	return( true );
}

int CPointBase::GetDist( const CPointBase & pt ) const // Distance between points
{
	ADDTOCALLSTACK("CPointBase::GetDist");
	// Get the basic 2d distance.
	if ( !pt.IsValidPoint() )
	{
#ifndef _NIGHTLYBUILD
		DEBUG_ERR(("GetDist::InvalidPoint -> '%s'\n", pt.WriteUsed()));
#endif
		return( SHRT_MAX );
	}

	if ( ! IsSameMap( pt.m_map ))	// as far apart as possible
		return( SHRT_MAX );
	return( GetDistBase( pt ));
}

int CPointBase::GetDist3D( const CPointBase & pt ) const // Distance between points
{
	ADDTOCALLSTACK("CPointBase::GetDist3D");
	// OK, 1 unit of Z is not the same (real life) distance as 1
	// unit of X (or Y)
	int dist = GetDist(pt);

	// Get the deltas and correct the Z for height first
	int dz = GetDistZAdj(pt); // Take player height into consideration
	return( maximum( dist, dz ));
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

	m_x = ptSrc.m_x + IMULDIV( iSteps, dx, iDist2D );
	m_y = ptSrc.m_y + IMULDIV( iSteps, dy, iDist2D );
	return( iDist2D );
}

LPCTSTR CPointBase::WriteUsed() const
{
	ADDTOCALLSTACK("CPointBase::WriteUsed");
	return( WriteUsed( Str_GetTemp()));
}

int CPointBase::Read( TCHAR * pszVal )
{
	ADDTOCALLSTACK("CPointBase::Read");
	// parse reading the point
	// NOTE: do not use = as a separator here !
	m_z = 0;
	m_map = 0;
	TCHAR * ppVal[4];
	int iArgs = Str_ParseCmds( pszVal, ppVal, COUNTOF( ppVal ), " ,\t" );
	switch ( iArgs )
	{
	default:
	case 4:	// m_map
		if ( isdigit(ppVal[3][0]))
		{
			m_map = ATOI(ppVal[3]);
			if (( m_map < 0 ) || ( m_map >= 256 ) || !g_MapList.m_maps[m_map] )
			{
				g_Log.EventError("Unsupported map #%d specifyed. Auto-fixing that to 0.\n", m_map);
				m_map = 0;
			}
		}
	case 3: // m_z
		if ( isdigit(ppVal[2][0]) || ppVal[2][0] == '-' )
		{
			m_z = ATOI(ppVal[2]);
		}
	case 2:
		m_y = ATOI(ppVal[1]);
	case 1:
		m_x = ATOI(ppVal[0]);
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

int CPointBase::GetRegions( DWORD dwType, CRegionLinks & rlinks ) const
{
	ADDTOCALLSTACK("CPointBase::GetRegions");
	if ( !IsValidPoint() )
		return NULL;

	CSector *pSector = GetSector();
	if ( pSector )
		return pSector->GetRegions(*this, dwType, rlinks);

	return NULL;
}

//*************************************************************************
// -CGRect

int CGRect::Read( LPCTSTR pszVal )
{
	ADDTOCALLSTACK("CGRect::Read");
	// parse reading the rectangle
	TCHAR *pszTemp = Str_GetTemp();
	strcpy( pszTemp, pszVal );
	TCHAR * ppVal[5];
	int i = Str_ParseCmds( pszTemp, ppVal, COUNTOF( ppVal ), " ,\t");
	switch (i)
	{
	case 5:
		m_map = ATOI(ppVal[4]);
		if (( m_map < 0 ) || ( m_map >= 256 ) || !g_MapList.m_maps[m_map] )
		{
			g_Log.EventError("Unsupported map #%d specifyed. Auto-fixing that to 0.\n", m_map);
			m_map = 0;
		}
	case 4: m_bottom = ATOI(ppVal[3]);
	case 3: m_right = ATOI(ppVal[2]);
	case 2: m_top =	ATOI(ppVal[1]);
	case 1: m_left = ATOI(ppVal[0]);
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
	pt.m_map = m_map;
	switch ( dir )
	{
	case DIR_N:
		pt.m_x = ( m_left + m_right ) / 2;
		pt.m_y = m_top;
		break;
	case DIR_NE:
		pt.m_x = m_right;
		pt.m_y = m_top;
		break;
	case DIR_E:
		pt.m_x = m_right;
		pt.m_y = ( m_top + m_bottom ) / 2;
		break;
	case DIR_SE:
		pt.m_x = m_right;
		pt.m_y = m_bottom;
		break;
	case DIR_S:
		pt.m_x = ( m_left + m_right ) / 2;
		pt.m_y = m_bottom;
		break;
	case DIR_SW:
		pt.m_x = m_left;
		pt.m_y = m_bottom;
		break;
	case DIR_W:
		pt.m_x = m_left;
		pt.m_y = ( m_top + m_bottom ) / 2;
		break;
	case DIR_NW:
		pt.m_x = m_left;
		pt.m_y = m_top;
		break;
	case DIR_QTY:
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
