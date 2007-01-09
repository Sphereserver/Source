//
// CGrayMap.cpp
//

#include "../graysvr.h"

//////////////////////////////////////////////////////////////////
// -CGrayMapBlockState

CGrayMapBlockState::CGrayMapBlockState(DWORD dwBlockFlags, signed char z, int iHeight, signed char zClimb) :
	m_dwBlockFlags(dwBlockFlags), m_z(z), m_iHeight(iHeight), m_zClimb(zClimb)
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

void CGrayMapBlockState::CheckTile( DWORD wItemBlockFlags, signed char zBottom, signed char zHeight, WORD wID )
{
	// RETURN:
	//  true = continue processing

	signed char zTop = zBottom;
	if ( (wItemBlockFlags & CAN_I_CLIMB) && !(wItemBlockFlags & CAN_I_PLATFORM) )
		zTop += ( zHeight / 2 );	// standing position is half way up climbable items.
	else
		zTop += zHeight;

	if ( zTop < m_Bottom.m_z )	// below something i can already step on.
		return;

	if ( ! wItemBlockFlags )	// no effect.
		return;

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
	if ( zBottom < ( m_z + m_iHeight/2 ))
	{
		// This is the new item ( that would be ) under me.
		// NOTE: Platform items should take precendence over non-platforms.
		if ( zTop >= m_Bottom.m_z )
		{
			if ( zTop == m_Bottom.m_z )
			{
				if ( m_Bottom.m_dwBlockFlags & CAN_I_PLATFORM )
					return;
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
}

void CGrayMapBlockState::CheckTile_Item( DWORD wItemBlockFlags, signed char zBottom, signed char zHeight, WORD wID )
{
	if ( !wItemBlockFlags )	return;

	signed char zTop = zBottom;
	if ( (wItemBlockFlags & CAN_I_CLIMB) && !(wItemBlockFlags & CAN_I_PLATFORM) )
		zTop += ( zHeight / 2 );	// standing position is half way up climbable items (except platforms).
	else
		zTop += zHeight;
	if ( zTop < m_Bottom.m_z )	// below something i can already step on.
		return;

	if ( zTop < m_Lowest.m_z )
	{
		m_Lowest.m_dwBlockFlags = wItemBlockFlags;
		m_Lowest.m_wTile = wID;
		m_Lowest.m_z = zTop;
	}

	if ( ! ( wItemBlockFlags &~ m_dwBlockFlags ))
	{	// this does not block me.
		if ( ! ( wItemBlockFlags & ( CAN_I_CLIMB | CAN_I_PLATFORM ) ) )
		{
			return;
		}
	}

	if ( ( zBottom <= m_zClimb ) )
	{
		if ( zTop >= m_Bottom.m_z )
		{
			if ( zTop == m_Bottom.m_z )
			{
				if ( ! ( wItemBlockFlags & CAN_I_CLIMB ) ) // climbable items have the highest priority
					if ( m_Bottom.m_dwBlockFlags & CAN_I_PLATFORM ) //than items with CAN_I_PLATFORM
						return;
			}
			m_Bottom.m_dwBlockFlags = wItemBlockFlags;
			m_Bottom.m_wTile = wID;
			m_Bottom.m_z = zTop;

			if ( wItemBlockFlags & CAN_I_CLIMB ) // return climb height
				m_zClimbHeight = ( zHeight / 2 );
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
}

void CGrayMapBlockState::CheckTile_Terrain( DWORD wItemBlockFlags, signed char z, WORD wID )
{
	if ( !wItemBlockFlags )	return;

	if ( z < m_Bottom.m_z )	// below something i can already step on.
		return;

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
					return;
				if ( ( m_Bottom.m_dwBlockFlags & CAN_I_PLATFORM ) && (!( wItemBlockFlags & CAN_I_PLATFORM )) )
					return;
			} else if ( z > m_z + PLAYER_HEIGHT/2 ) 
				if ( (m_Bottom.m_dwBlockFlags & (CAN_I_PLATFORM|CAN_I_CLIMB)) && (z >= m_Bottom.m_z + PLAYER_HEIGHT/2) ) // we can walk under it
					goto settop;

			m_Bottom.m_dwBlockFlags = wItemBlockFlags;
			m_Bottom.m_wTile = wID;
			m_Bottom.m_z = z;
			m_zClimbHeight = 0;
		}
	}
	else
	{
		// I could potentially fit under this. ( it would be above me )
settop:
		if ( z < m_Top.m_z )
		{
			m_Top.m_dwBlockFlags = wItemBlockFlags;
			m_Top.m_wTile = wID;
			m_Top.m_z = z;
		}
	}
}

bool CGrayMapBlockState::IsUsableZ(signed char zBottom, signed char zHeightEstimate) const
{
	if ( zBottom > m_Top.m_z )	// above something that is already over my head.
		return false;
	// NOTE: Assume multi overlapping items are not normal. so estimates are safe
	if ( zBottom + zHeightEstimate < m_Bottom.m_z )	// way below my feet
		return false;
	return true;	
}

//////////////////////////////////////////////////////////////////
// -CGrayStaticsBlock

void CGrayStaticsBlock::LoadStatics( DWORD ulBlockIndex, int map )
{
	// long ulBlockIndex = (bx*(UO_SIZE_Y/UO_BLOCK_SIZE) + by);
	// NOTE: What is index.m_wVal3 and index.m_wVal4 in VERFILE_STAIDX ?
	CUOIndexRec index;
	if ( g_Install.ReadMulIndex(g_Install.m_Staidx[g_MapList.m_mapnum[map]], ulBlockIndex, index) )
	{
		// make sure that the statics block length is valid
		if ((index.GetBlockLength() % sizeof(CUOStaticItemRec)) != 0)
		{
			TEMPSTRING(pszTemp);
			sprintf(pszTemp, "CGrapMapBlock: Read Statics - Block Length of %d", index.GetBlockLength());
			throw CError(LOGL_CRIT, CGFile::GetLastError(), pszTemp);
		}
		m_iStatics = index.GetBlockLength()/sizeof(CUOStaticItemRec);
		m_pStatics = new CUOStaticItemRec[m_iStatics];
		if ( !g_Install.ReadMulData(g_Install.m_Statics[g_MapList.m_mapnum[map]], index, m_pStatics) )
			throw CError(LOGL_CRIT, CGFile::GetLastError(), "CGrayMapBlock: Read fStatics0");
	}
}

bool CGrayStaticsBlock::IsStaticPoint( int i, int xo, int yo ) const
{
	if ( i >= m_iStatics )
		return false;
	return (( m_pStatics[i].m_x == xo ) && ( m_pStatics[i].m_y == yo ));
}

//////////////////////////////////////////////////////////////////
// -CGrayMapBlock

void CGrayMapBlock::Load( int bx, int by )
{
	// Read in all the statics data for this block.
	m_CacheTime.InitCacheTime();		// This is invalid !

	if (( m_map < 0 ) || ( m_map >= 255 ) || !g_MapList.m_maps[m_map] )
	{
		g_Log.Error("Unsupported map #%d specifyed. Auto-fixing that to 0.\n", m_map);
		m_map = 0;
	}

	long ulBlockIndex = (bx*(g_MapList.GetY(m_map)/UO_BLOCK_SIZE) + by);

	CGFile * pFile;
	CUOIndexRec index;
	index.SetupIndex( ulBlockIndex * sizeof(CUOMapBlock), sizeof(CUOMapBlock));
	pFile = &(g_Install.m_Maps[g_MapList.m_mapnum[m_map]]);

	if ( pFile->Seek( index.GetFileOffset(), SEEK_SET ) != index.GetFileOffset() )
	{
		memset( &m_Terrain, 0, sizeof( m_Terrain ));
		throw CError(LOGL_CRIT, CGFile::GetLastError(), "CGrayMapBlock: Seek Ver");
	}
	if ( pFile->Read( &m_Terrain, sizeof(CUOMapBlock)) <= 0 )
	{
		memset( &m_Terrain, 0, sizeof( m_Terrain ));
		throw CError(LOGL_CRIT, CGFile::GetLastError(), "CGrayMapBlock: Read");
	}

	m_Statics.LoadStatics(ulBlockIndex, m_map);
	m_CacheTime.HitCacheTime();		// validate.
}
