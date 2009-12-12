//
// CGrayMap.cpp
//

#include "../graysvr/graysvr.h"

//////////////////////////////////////////////////////////////////
// -CGrayMapBlockState

#ifndef CAN_I_BLOCK
#define CAN_I_BLOCK		UFLAG1_BLOCK
#define CAN_I_PLATFORM	UFLAG2_PLATFORM
#define CAN_I_CLIMB		UFLAG2_CLIMBABLE
#define CAN_I_DOOR		UFLAG4_DOOR
#endif

CGrayMapBlockState::CGrayMapBlockState( DWORD dwBlockFlags, signed char z, int iHeight, signed char zHeight ) :
	m_dwBlockFlags(dwBlockFlags),	m_z(z), m_iHeight(iHeight), m_zClimb(0), m_zHeight(zHeight)
{
	// m_z = PLAYER_HEIGHT
	m_Top.m_dwBlockFlags = 0;
	m_Top.m_wTile = 0;
	m_Top.m_z = UO_SIZE_Z;	// the z of the item that would be over our head.

	m_Bottom.m_dwBlockFlags = CAN_I_BLOCK; // The bottom item has these blocking flags.
	m_Bottom.m_wTile = 0;
	m_Bottom.m_z = UO_SIZE_MIN_Z;	// the z we would stand on,

	m_Lowest.m_dwBlockFlags = CAN_I_BLOCK; 
	m_Lowest.m_wTile = 0;
	m_Lowest.m_z = UO_SIZE_Z;

	m_zClimbHeight = 0;
}

CGrayMapBlockState::CGrayMapBlockState( DWORD dwBlockFlags, signed char z, int iHeight, signed char zClimb, signed char zHeight ) :
	m_dwBlockFlags(dwBlockFlags),	m_z(z), m_iHeight(iHeight), m_zClimb(zClimb), m_zHeight(zHeight)
{
	m_Top.m_dwBlockFlags = 0;
	m_Top.m_wTile = 0;
	m_Top.m_z = UO_SIZE_Z;	// the z of the item that would be over our head.

	m_Bottom.m_dwBlockFlags = CAN_I_BLOCK; // The bottom item has these blocking flags.
	m_Bottom.m_wTile = 0;
	m_Bottom.m_z = UO_SIZE_MIN_Z;	// the z we would stand on,

	m_Lowest.m_dwBlockFlags = CAN_I_BLOCK; 
	m_Lowest.m_wTile = 0;
	m_Lowest.m_z = UO_SIZE_Z;

	m_zClimbHeight = 0;
}

LPCTSTR CGrayMapBlockState::GetTileName( WORD wID )	// static
{
	ADDTOCALLSTACK("CGrayMapBlockState::GetTileName");
	if ( wID == 0 )
	{
		return( "<null>" );
	}
	TCHAR * pStr = Str_GetTemp();
	if ( wID < TERRAIN_QTY )
	{
		CGrayTerrainInfo land( wID );
		strcpy( pStr, land.m_name );
	}
	else
	{
		wID -= TERRAIN_QTY;
		CGrayItemInfo item( (ITEMID_TYPE) wID );
		strcpy( pStr, item.m_name );
	}
	return( pStr );
}

bool CGrayMapBlockState::CheckTile( DWORD wItemBlockFlags, signed char zBottom, signed char zHeight, WORD wID )
{
	ADDTOCALLSTACK("CGrayMapBlockState::CheckTile");
	// RETURN:
	//  true = continue processing

	signed char zTop = zBottom;
	if ( (wItemBlockFlags & CAN_I_CLIMB) && !(wItemBlockFlags & CAN_I_PLATFORM) )
		zTop += ( zHeight / 2 );	// standing position is half way up climbable items (except platforms).
	else
		zTop += zHeight;

	if ( zTop < m_Bottom.m_z )	// below something i can already step on.
		return true;
	
	// hover flag has no effect for non-hovering entities
	if ( (wItemBlockFlags & CAN_I_HOVER) && !(m_dwBlockFlags & CAN_C_HOVER) )
		wItemBlockFlags &= ~CAN_I_HOVER;

	if ( ! wItemBlockFlags )	// no effect.
		return( true );

	// If this item does not block me at all then i guess it just does not count.
	if ( ! ( wItemBlockFlags &~ m_dwBlockFlags ))
	{	// this does not block me.
		if ( wItemBlockFlags & CAN_I_PLATFORM ) // i can always walk on the platform.
		{
			zBottom = zTop;
		}
		else if ( ! ( wItemBlockFlags & CAN_I_CLIMB ))
		{
			zTop = zBottom; // or i could walk under it.
		}
	}

	if ( zTop < m_Lowest.m_z )
	{
		m_Lowest.m_dwBlockFlags = wItemBlockFlags;
		m_Lowest.m_wTile = wID;
		m_Lowest.m_z = zTop;
	}

	// if i can't fit under this anyhow. it is something below me. (potentially)
	// we can climb a bit higher to reach climbables and platforms
	if ( zBottom < (m_z + (( m_iHeight + ((wItemBlockFlags & (CAN_I_CLIMB|CAN_I_PLATFORM)) ? zHeight : 0) ) / 2)) )
	{
		// This is the new item ( that would be ) under me.
		// NOTE: Platform items should take precedence over non-platforms.
		//       (Water acts as a platform for ships, so this also takes precedence)
		if ( zTop >= m_Bottom.m_z )
		{
			if ( zTop == m_Bottom.m_z )
			{
				if ( m_Bottom.m_dwBlockFlags & CAN_I_PLATFORM )
					return( true );
				else if ( (m_Bottom.m_dwBlockFlags & CAN_I_WATER) && !(wItemBlockFlags & CAN_I_PLATFORM))
					return( true );
			}
			m_Bottom.m_dwBlockFlags = wItemBlockFlags;
			m_Bottom.m_wTile = wID;
			m_Bottom.m_z = zTop;
		}
	}
	else
	{
		// I could potentially fit under this. ( it would be above me )
		if ( zBottom <= m_Top.m_z )
		{
			m_Top.m_dwBlockFlags = wItemBlockFlags;
			m_Top.m_wTile = wID;
			m_Top.m_z = zBottom;
		}
	}

	return true;
}

bool CGrayMapBlockState::CheckTile_Item( DWORD wItemBlockFlags, signed char zBottom, signed char zHeight, WORD wID )
{
	ADDTOCALLSTACK("CGrayMapBlockState::CheckTile_Item");
	// RETURN:
	//  true = continue processing

	// hover flag has no effect for non-hovering entities
	if ( (wItemBlockFlags & CAN_I_HOVER) && !(m_dwBlockFlags & CAN_C_HOVER) )
		wItemBlockFlags &= ~CAN_I_HOVER;

	if ( ! wItemBlockFlags )	// no effect.
		return( true );

	signed char zTop = zBottom;
	
	if ( (wItemBlockFlags & CAN_I_CLIMB) && (wItemBlockFlags & CAN_I_PLATFORM) )
	{
		//DEBUG_WARN(("EF_NewPositionChecks+Stairs\n"));
		zTop += ( zHeight / 2 );	// standing position is half way up climbable items (except platforms).
	}
	else
		zTop += zHeight;

	if ( zTop < m_Bottom.m_z )	// below something i can already step on.
		return true;

	if (IsSetEF( EF_NewPositionChecks ))
	{
		if ( zBottom > m_Top.m_z )	// above my head.
			return true;
	}
	if ( zTop < m_Lowest.m_z )
	{
		m_Lowest.m_dwBlockFlags = wItemBlockFlags;
		m_Lowest.m_wTile = wID;
		m_Lowest.m_z = zTop;
	}

	if ( ! ( wItemBlockFlags &~ m_dwBlockFlags ))
	{	// this does not block me.
		if ( ! ( wItemBlockFlags & ( CAN_I_CLIMB | CAN_I_PLATFORM | CAN_I_HOVER ) ) )
		{
			return true;
		}
	}
	//DEBUG_ERR(("wID 0%x zBottom %d  zHeight %d  zTop %d  m_zClimb %d  m_Bottom.m_z %d  m_Top.m_z %d\n",wID-TERRAIN_QTY,zBottom,zHeight,zTop,m_zClimb,m_Bottom.m_z,m_Top.m_z));
	//DEBUG_ERR(("zBottom(%d) <= m_zClimb(%d) - %d\n",zBottom,m_zClimb,(zBottom <= m_zClimb)));
	if ( zBottom <= ( m_zClimb + ( wItemBlockFlags & ( CAN_I_CLIMB ) ? zHeight / 2 : 0 ) ) )
	{
		if ( zTop >= m_Bottom.m_z )
		{
			if ( zTop == m_Bottom.m_z )
			{
				if ( ! ( wItemBlockFlags & CAN_I_CLIMB ) ) // climbable items have the highest priority
					if ( m_Bottom.m_dwBlockFlags & CAN_I_PLATFORM ) //than items with CAN_I_PLATFORM
						return ( true );
			}
			//DEBUG_ERR(("1wID 0%x zBottom %d  zHeight %d  zTop %d  m_zClimb %d  m_Bottom.m_z %d  m_Top.m_z %d\n",wID-TERRAIN_QTY,zBottom,zHeight,zTop,m_zClimb,m_Bottom.m_z,m_Top.m_z));
			m_Bottom.m_dwBlockFlags = wItemBlockFlags;
			m_Bottom.m_wTile = wID;
			m_Bottom.m_z = zTop;
			//DEBUG_ERR(("2wID 0%x zBottom %d  zHeight %d  zTop %d  m_zClimb %d  m_Bottom.m_z %d  m_Top.m_z %d\n",wID-TERRAIN_QTY,zBottom,zHeight,zTop,m_zClimb,m_Bottom.m_z,m_Top.m_z));

			if ( wItemBlockFlags & CAN_I_CLIMB ) // return climb height
				m_zClimbHeight = (( zHeight + 1 )/2); //if height is an odd number, then we need to add 1; if it isn't, this does nothing
			else
				m_zClimbHeight = 0;
		}
	}
	else
	{
		// I could potentially fit under this. ( it would be above me )
		if ( zBottom < m_Top.m_z )
		{
			m_Top.m_dwBlockFlags = wItemBlockFlags;
			m_Top.m_wTile = wID;
			m_Top.m_z = zBottom;
		}
	}
	return true;

}

inline void CGrayMapBlockState::SetTop( DWORD &wItemBlockFlags, signed char &z, WORD &wID )
{
	ADDTOCALLSTACK("CGrayMapBlockState::SetTop");
	if ( z < m_Top.m_z )
	{
		m_Top.m_dwBlockFlags = wItemBlockFlags;
		m_Top.m_wTile = wID;
		m_Top.m_z = z;
	}
}

bool CGrayMapBlockState::CheckTile_Terrain( DWORD wItemBlockFlags, signed char z, WORD wID )
{
	ADDTOCALLSTACK("CGrayMapBlockState::CheckTile_Terrain");
	// RETURN:
	//  true = continue processing

	if ( ! wItemBlockFlags )	// no effect.
		return( true );

	if ( z < m_Bottom.m_z )	// below something i can already step on.
	{
		return true;
	}

	if ( z < m_Lowest.m_z )
	{
		m_Lowest.m_dwBlockFlags = wItemBlockFlags;
		m_Lowest.m_wTile = wID;
		m_Lowest.m_z = z;
	}

	if	(  z <= m_iHeight )
	{
		if ( z >= m_Bottom.m_z )
		{
			if ( z == m_Bottom.m_z )
			{
				if ( m_Bottom.m_dwBlockFlags & CAN_I_CLIMB ) // climbable items have the highest priority
				{
					return ( true );
				}
				if ( ( m_Bottom.m_dwBlockFlags & CAN_I_PLATFORM ) && (!( wItemBlockFlags & CAN_I_PLATFORM )) )
				{
					return ( true );
				}
			}
			else if ( z > m_z + PLAYER_HEIGHT/2 ) 
			{
				if ( (m_Bottom.m_dwBlockFlags & (CAN_I_PLATFORM|CAN_I_CLIMB)) && (z >= m_Bottom.m_z + PLAYER_HEIGHT/2) ) // we can walk under it
				{
					SetTop( wItemBlockFlags, z, wID );
					return true;
				}
			}
			//DEBUG_ERR(("wItemBlockFlags 0x%x\n",wItemBlockFlags));
			m_Bottom.m_dwBlockFlags = wItemBlockFlags;
			m_Bottom.m_wTile = wID;
			m_Bottom.m_z = z;
			m_zClimbHeight = 0;
		}
	}
	else
	{
		SetTop( wItemBlockFlags, z, wID ); // I could potentially fit under this. ( it would be above me )
	}
	return true;
}


//////////////////////////////////////////////////////////////////
// -CGrayStaticsBlock

void CGrayStaticsBlock::LoadStatics( DWORD ulBlockIndex, int map )
{
	ADDTOCALLSTACK("CGrayStaticsBlock::LoadStatics");
	// long ulBlockIndex = (bx*(UO_SIZE_Y/UO_BLOCK_SIZE) + by);
	// NOTE: What is index.m_wVal3 and index.m_wVal4 in VERFILE_STAIDX ?
	ASSERT( m_iStatics <= 0 );

	CUOIndexRec index;
	if ( g_Install.ReadMulIndex(g_Install.m_Staidx[g_MapList.m_mapnum[map]], ulBlockIndex, index) )
	{
		// make sure that the statics block length is valid
		if ((index.GetBlockLength() % sizeof(CUOStaticItemRec)) != 0)
		{
			TCHAR *pszTemp = Str_GetTemp();
			sprintf(pszTemp, "CGrapMapBlock: Read Statics - Block Length of %d", index.GetBlockLength());
			throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), pszTemp);
		}
		m_iStatics = index.GetBlockLength()/sizeof(CUOStaticItemRec);
		ASSERT(m_iStatics);
		m_pStatics = new CUOStaticItemRec[m_iStatics];
		ASSERT(m_pStatics);
		if ( ! g_Install.ReadMulData(g_Install.m_Statics[g_MapList.m_mapnum[map]], index, m_pStatics) )
		{
			throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "CGrayMapBlock: Read Statics");
		}
	}
}

void CGrayStaticsBlock::LoadStatics( int iCount, CUOStaticItemRec * pStatics )
{
	ADDTOCALLSTACK("CGrayStaticsBlock::LoadStatics2");
	// Load statics information directly (normally from difs)
	m_iStatics = iCount;
	if ( m_iStatics )
	{
		m_pStatics = new CUOStaticItemRec[m_iStatics];
		memcpy(m_pStatics, pStatics, sizeof(CUOStaticItemRec) * m_iStatics);
	}
	else
	{
		if ( m_pStatics )	delete[] m_pStatics;
		m_pStatics = NULL;
	}
}

bool CGrayStaticsBlock::IsStaticPoint( int i, int xo, int yo ) const
{
	ADDTOCALLSTACK("CGrayStaticsBlock::IsStaticPoint");
	ASSERT( xo >= 0 && xo < UO_BLOCK_SIZE );
	ASSERT( yo >= 0 && yo < UO_BLOCK_SIZE );
	ASSERT( i < m_iStatics );
	return( m_pStatics[i].m_x == xo && m_pStatics[i].m_y == yo );
}

//////////////////////////////////////////////////////////////////
// -CGrayMapBlock

int CGrayMapBlock::sm_iCount = 0;

void CGrayMapBlock::Load( int bx, int by )
{
	ADDTOCALLSTACK("CGrayMapBlock::Load");
	// Read in all the statics data for this block.
	m_CacheTime.InitCacheTime();		// This is invalid !

	ASSERT( bx < (g_MapList.GetX(m_map)/UO_BLOCK_SIZE) );
	ASSERT( by < (g_MapList.GetY(m_map)/UO_BLOCK_SIZE) );

	if (( m_map < 0 ) || ( m_map >= 255 ))
	{
		g_Log.EventError("Unsupported map #%d specified. Auto-fixing that to 0.\n", m_map);
		m_map = 0;
	}

	unsigned long ulBlockIndex = (bx*(g_MapList.GetY(m_map)/UO_BLOCK_SIZE) + by);

	if ( !g_MapList.m_maps[m_map] )
	{
		memset( &m_Terrain, 0, sizeof( m_Terrain ));
		throw CGrayError(LOGL_CRIT, 0, "CGrayMapBlock: Map is not supported since MUL files for it not available.");
	}

	bool bPatchedTerrain = false, bPatchedStatics = false;

	if ( g_Cfg.m_fUseMapDiffs && g_MapList.m_pMapDiffCollection )
	{
		// Check to see if the terrain or statics in this block is patched
		CMapDiffBlock * pDiffBlock = g_MapList.m_pMapDiffCollection->GetAtBlock( ulBlockIndex, g_MapList.m_mapid[m_map] );
		if ( pDiffBlock )
		{
			if ( pDiffBlock->m_pTerrainBlock )
			{
				memcpy( &m_Terrain, pDiffBlock->m_pTerrainBlock, sizeof(CUOMapBlock) );
				bPatchedTerrain = true;
			}

			if ( pDiffBlock->m_iStaticsCount >= 0 )
			{
				m_Statics.LoadStatics( pDiffBlock->m_iStaticsCount, pDiffBlock->m_pStaticsBlock );
				bPatchedStatics = true;
			}
		}
	}

	// Only load terrain if it wasn't patched
	if ( ! bPatchedTerrain )
	{
		CGFile * pFile;
		CUOIndexRec index;
		index.SetupIndex( ulBlockIndex * sizeof(CUOMapBlock), sizeof(CUOMapBlock));
		pFile = &(g_Install.m_Maps[g_MapList.m_mapnum[m_map]]);

		if ( pFile->Seek( index.GetFileOffset(), SEEK_SET ) != index.GetFileOffset() )
		{
			memset( &m_Terrain, 0, sizeof( m_Terrain ));
			throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "CGrayMapBlock: Seek Ver");
		}
		if ( pFile->Read( &m_Terrain, sizeof(CUOMapBlock)) <= 0 )
		{
			memset( &m_Terrain, 0, sizeof( m_Terrain ));
			throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "CGrayMapBlock: Read");
		}
	}

	// Only load statics if they weren't patched
	if ( ! bPatchedStatics )
	{
		m_Statics.LoadStatics( ulBlockIndex, m_map );
	}

	m_CacheTime.HitCacheTime();		// validate.
}

//////////////////////////////////////////////////////////////////
// -CMapDiffCollection

CMapDiffCollection::CMapDiffCollection()
{
	m_bLoaded = false;
}

CMapDiffCollection::~CMapDiffCollection()
{
	// Remove all of the loaded dif data
	for ( int m = 0; m < 256; m++ )
	{
		while ( m_pMapDiffBlocks[m].GetCount() )
		{
			m_pMapDiffBlocks[m].RemoveAt(0);
		}
	}
}

void CMapDiffCollection::LoadMapDiffs()
{
	// Load mapdif* and stadif* Files
	ADDTOCALLSTACK("CMapDiffCollection::LoadMapDiffs");
	if ( m_bLoaded ) // already loaded
		return;

	DWORD dwLength = 0, dwBlockId = 0;
	int iOffset = 0, iRead = 0;
	CMapDiffBlock * pMapDiffBlock = NULL;

	for ( int m = 0; m < 256; ++m )
	{
		if ( !g_MapList.IsMapSupported( m ) )
			continue;

		int map = g_MapList.m_mapid[m];

		// Load Mapdif Files
		{
			CGFile * pFileMapdif	= &(g_Install.m_Mapdif[map]);
			CGFile * pFileMapdifl	= &(g_Install.m_Mapdifl[map]);

			// Check that the relevant dif files are available
			if ( pFileMapdif->IsFileOpen() && pFileMapdifl->IsFileOpen() )
			{
				// Make sure that we're at the beginning of the files
				pFileMapdif->SeekToBegin();
				pFileMapdifl->SeekToBegin();

				dwLength = pFileMapdifl->GetLength();
				iRead = iOffset = 0;

				for ( ; iRead < dwLength; iOffset += sizeof(CUOMapBlock) )
				{
					iRead += pFileMapdifl->Read( &dwBlockId, sizeof(dwBlockId) );
					pMapDiffBlock = GetNewBlock( dwBlockId, map );

					if ( pMapDiffBlock->m_pTerrainBlock )
						delete pMapDiffBlock->m_pTerrainBlock;

					CUOMapBlock * pTerrain = new CUOMapBlock();
					if ( pFileMapdif->Seek( iOffset ) != iOffset )
					{
						g_Log.EventError("Reading mapdif%d.mul FAILED.\n", map);
						delete pTerrain;
						break;
					}
					else if ( pFileMapdif->Read( pTerrain, sizeof(CUOMapBlock) ) != sizeof(CUOMapBlock) )
					{
						g_Log.EventError("Reading mapdif%d.mul FAILED. [index=%d offset=%d]\n", map, dwBlockId, iOffset);
						delete pTerrain;
						break;
					}

					pMapDiffBlock->m_pTerrainBlock = pTerrain;
				}
			}
		} // Mapdif

		// Load Stadif Files
		{
			CGFile * pFileStadif	= &(g_Install.m_Stadif[map]);
			CGFile * pFileStadifl	= &(g_Install.m_Stadifl[map]);
			CGFile * pFileStadifi	= &(g_Install.m_Stadifi[map]);

			// Check that the relevant dif files are available
			if ( !pFileStadif->IsFileOpen() || !pFileStadifl->IsFileOpen() || !pFileStadifi->IsFileOpen() )
				continue;

			// Make sure that we're at the beginning of the files
			pFileStadif->SeekToBegin();
			pFileStadifl->SeekToBegin();
			pFileStadifi->SeekToBegin();

			dwLength = pFileStadifl->GetLength();
			iRead = iOffset = 0;
	
			for ( ; iRead < dwLength; iOffset += sizeof(CUOIndexRec) )
			{
				iRead += pFileStadifl->Read( &dwBlockId, sizeof(dwBlockId) );
				
				pMapDiffBlock = GetNewBlock( dwBlockId, map );
				if ( pMapDiffBlock->m_pStaticsBlock )
					delete[] pMapDiffBlock->m_pStaticsBlock;

				pMapDiffBlock->m_iStaticsCount = 0;
				pMapDiffBlock->m_pStaticsBlock = NULL;

				if ( pFileStadifi->Seek( iOffset ) != iOffset )
				{
					g_Log.EventError("Reading stadifi%d.mul FAILED.\n", map);
					break;
				}

				CUOIndexRec index;
				if ( pFileStadifi->Read( &index, sizeof(CUOIndexRec)) != sizeof(CUOIndexRec) )
				{
					g_Log.EventError("Reading stadifi%d.mul FAILED. [index=%d offset=%d]\n", map, dwBlockId, iOffset);
					break;
				}
				else if ( !index.HasData() ) // This happens if the block has been intentionally patched to remove statics
				{
					continue;
				}
				else if ((index.GetBlockLength() % sizeof(CUOStaticItemRec)) != 0) // Make sure that the statics block length is valid
				{
					g_Log.EventError("Reading stadifi%d.mul FAILED. [index=%d offset=%d length=%d]\n", map, dwBlockId, iOffset, index.GetBlockLength());
					break;
				}

				pMapDiffBlock->m_iStaticsCount = index.GetBlockLength()/sizeof(CUOStaticItemRec);
				pMapDiffBlock->m_pStaticsBlock = new CUOStaticItemRec[pMapDiffBlock->m_iStaticsCount];
				if ( !g_Install.ReadMulData(*pFileStadif, index, pMapDiffBlock->m_pStaticsBlock) )
				{
					// This shouldn't happen, if this fails then the block will
					// be left with no statics
					pMapDiffBlock->m_iStaticsCount = 0;
					delete[] pMapDiffBlock->m_pStaticsBlock;
					pMapDiffBlock->m_pStaticsBlock = NULL;
					g_Log.EventError("Reading stadif%d.mul FAILED. [index=%d offset=%d]\n", map, dwBlockId, iOffset);
					break;
				}
			}
		} // Stadif
	}

	m_bLoaded = true;
}

void CMapDiffCollection::Init()
{
	// Initialise class (load diffs)
	ADDTOCALLSTACK("CMapDiffCollection::Init");
	LoadMapDiffs();
}

CMapDiffBlock * CMapDiffCollection::GetNewBlock(DWORD dwBlockId, int map)
{
	// Retrieve a MapDiff block for the specified block id, or
	// allocate a new MapDiff block if one doesn't exist already.
	ADDTOCALLSTACK("CMapDiffCollection::GetNewBlock");

	CMapDiffBlock * pMapDiffBlock = GetAtBlock( dwBlockId, map );
	if ( pMapDiffBlock )
		return pMapDiffBlock;

	pMapDiffBlock = new CMapDiffBlock(dwBlockId, map);
	m_pMapDiffBlocks[map].AddSortKey( pMapDiffBlock, dwBlockId );
	return pMapDiffBlock;
}

CMapDiffBlock * CMapDiffCollection::GetAtBlock(int bx, int by, int map)
{
	// See GetAtBlock(DWORD,int)
	DWORD dwBlockId = (bx * (g_MapList.GetY( map ) / UO_BLOCK_SIZE)) + by;
	return GetAtBlock( dwBlockId, map );
}

CMapDiffBlock * CMapDiffCollection::GetAtBlock(DWORD dwBlockId, int map)
{
	// Retrieve a MapDiff block for the specified block id
	ADDTOCALLSTACK("CMapDiffCollection::GetAtBlock");
	if ( !g_MapList.IsMapSupported( map ) )
		return NULL;

	// Locate the requested block
	int index = m_pMapDiffBlocks[map].FindKey( dwBlockId );
	if ( index < 0 )
		return NULL;

	return m_pMapDiffBlocks[map].GetAt( index );
}

/*

  Vjaka -	here lies mapdiff support sample code from wolfpack
			for investigation purposes

QMap<unsigned int, unsigned int> mappatches;
QMap<unsigned int, stIndexRecord> staticpatches;

struct mapblock
{
	unsigned int header;
	map_st cells[64];
};

void MapsPrivate::loadDiffs( const QString& basePath, unsigned int id )
{
	ADDTOCALLSTACK("MapsPrivate::loadDiffs");
	QDir baseFolder( basePath );
	QStringList files = baseFolder.entryList();
	QString mapDiffListName = QString( "mapdifl%1.mul" ).arg( id );
	QString mapDiffFileName = QString( "mapdif%1.mul" ).arg( id );
	QString statDiffFileName = QString( "stadif%1.mul" ).arg( id );
	QString statDiffListName = QString( "stadifl%1.mul" ).arg( id );
	QString statDiffIndexName = QString( "stadifi%1.mul" ).arg( id );
	for ( QStringList::const_iterator it = files.begin(); it != files.end(); ++it )
	{
		if ( ( *it ).lower() == mapDiffListName )
			mapDiffListName = *it;
		else if ( ( *it ).lower() == mapDiffFileName )
			mapDiffFileName = *it;
		else if ( ( *it ).lower() == statDiffFileName )
			statDiffFileName = *it;
		else if ( ( *it ).lower() == statDiffListName )
			statDiffListName = *it;
		else if ( ( *it ).lower() == statDiffIndexName )
			statDiffIndexName = *it;
	}

	QFile mapdiflist( basePath + mapDiffListName );
	mapdifdata.setName( basePath + mapDiffFileName );
	
	// Try to read a list of ids
	if ( mapdifdata.open( IO_ReadOnly ) && mapdiflist.open( IO_ReadOnly ) )
	{
		QDataStream listinput( &mapdiflist );
		listinput.setByteOrder( QDataStream::LittleEndian );
		unsigned int offset = 0;
		while ( !listinput.atEnd() )
		{
			unsigned int id;
			listinput >> id;
			mappatches.insert( id, offset );
			offset += sizeof( mapblock );
		}
		mapdiflist.close();
	}

	stadifdata.setName( basePath + statDiffFileName );
	stadifdata.open( IO_ReadOnly );

	QFile stadiflist( basePath + statDiffListName );
	QFile stadifindex( basePath + statDiffIndexName );

	if ( stadifindex.open( IO_ReadOnly ) && stadiflist.open( IO_ReadOnly ) )
	{
		QDataStream listinput( &stadiflist );
		QDataStream indexinput( &stadifindex );
		listinput.setByteOrder( QDataStream::LittleEndian );
		indexinput.setByteOrder( QDataStream::LittleEndian );

		stIndexRecord record;
		while ( !listinput.atEnd() )
		{
			unsigned int id;
			listinput >> id;

			indexinput >> record.offset;
			indexinput >> record.blocklength;
			indexinput >> record.extra;

			if ( !staticpatches.contains( id ) )
			{
				staticpatches.insert( id, record );
			}
		}
	}

	if ( stadiflist.isOpen() )
	{
		stadiflist.close();
	}

	if ( stadifindex.isOpen() )
	{
		stadifindex.close();
	}
}

map_st MapsPrivate::seekMap( ushort x, ushort y )
{
	ADDTOCALLSTACK("MapsPrivate::seekMap");
	// The blockid our cell is in
	unsigned int blockid = x / 8 * height + y / 8;

	// See if the block has been cached
	mapblock* result = mapCache.find( blockid );
	bool borrowed = true;

	if ( !result )
	{
		result = new mapblock;

		// See if the block has been patched
		if ( mappatches.contains( blockid ) )
		{
			unsigned int offset = mappatches[blockid];
			mapdifdata.at( offset );
			mapdifdata.readBlock( ( char * ) result, sizeof( mapblock ) );
		}
		else
		{
			mapfile.at( blockid * sizeof( mapblock ) );
			mapfile.readBlock( ( char * ) result, sizeof( mapblock ) );
		}

		borrowed = mapCache.insert( blockid, result );
	}

	// Convert to in-block values.
	y %= 8;
	x %= 8;
	map_st cell = result->cells[y * 8 + x];

	if ( !borrowed )
		delete result;

	return cell;
}
*/
