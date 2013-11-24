#include "../common/CMapCache.h"
#include "../common/CFileList.h"

CMapCache::CMapCache()
{
	m_hFile = NULL;
	m_map = m_maxX = m_maxY = 0;
}

CMapCache::~CMapCache()
{
	if ( m_hFile )
	{
		//	we do not delete the cache file, to free up some time initialising the block
		fclose(m_hFile);
	}
}

bool CMapCache::Init(int maxX, int maxY, int map, signed char mapDefaultHeight)
{
	ADDTOCALLSTACK("CMapCache::Init");
	char	*z = Str_GetTemp();
	DWORD	sizeNeeded = sizeof(TMapCachePoint) * maxX * maxY;
	m_map = map;

	sprintf(z, "map%dcache.tmp", map);

	m_hFile = fopen(z, "r");
	if ( m_hFile )
	{
		//	file already exists. check if size of the file is EXACTLY the same we need
		//	probably do some more check if it is correct
		DWORD	d;
		time_t	t;
		if ( CFileList::ReadFileInfo(z, t, d) )
		{
			if ( d != sizeNeeded )
				fclose(m_hFile);
		}
		else
			fclose(m_hFile);
	}

	if ( !m_hFile )
	{
		m_hFile = fopen(z, "w");
		if ( !m_hFile )
		{
			g_Log.EventError("Cannot create cache file %s. Pathfinding and map cache will be not available.\n", z);
			return false;
		}

		//	create initial block basing on statics
		TMapCachePoint point;
		point.m_itemBlock = point.m_charBlock = 0;
		for ( int i = 0; i < maxX; i++ )
		{
			for ( int j = 0; j < maxY; j++ )
			{
				CPointMap pt(i, j, 0, map);
				point.m_baseZ = UO_SIZE_Z;
				point.m_tileBlock = 0;

				const CGrayMapBlock	*pMapBlock = g_World.GetMapBlock(pt);
				size_t	iQty = pMapBlock->m_Statics.GetStaticQty();

				if ( iQty > 0 )
				{
					int x2 = pMapBlock->GetOffsetX(pt.m_x);
					int y2 = pMapBlock->GetOffsetY(pt.m_y);

					for ( size_t k = 0; k < iQty; k++ )
					{
						if ( !pMapBlock->m_Statics.IsStaticPoint(k, x2, y2) )
							continue;

						const CUOStaticItemRec *pStatic = pMapBlock->m_Statics.GetStatic(k);
						DWORD wBlockThis = 0;
						CItemBase::GetItemHeight(pStatic->GetDispID(), wBlockThis);

						if (( pStatic->m_z < point.m_baseZ ) && ( wBlockThis & (CAN_I_CLIMB|CAN_I_PLATFORM|CAN_I_ROOF) ))
						{
							point.m_baseZ = pStatic->m_z;
						}

						if (( (wBlockThis & CAN_I_BLOCK|CAN_I_PLATFORM) == CAN_I_BLOCK ) ||
							( wBlockThis & CAN_I_FIRE ) ||
							( wBlockThis & CAN_I_WATER ) )
						{
							//	interested flags for us are set, thus record this statics
							point.m_tileBlock |= wBlockThis & (CAN_I_BLOCK|CAN_I_FIRE|CAN_I_WATER);
						}
					}
				}

				if ( point.m_baseZ == UO_SIZE_Z )
					point.m_baseZ = mapDefaultHeight;

				fwrite(&point, sizeof(point), 1, m_hFile);
			}
		}

		//	flush the bufers to be sure that the file will be correctly saved and accessable after restart
		fflush(m_hFile);
	}
	return true;
}

bool CMapCache::Get(int x, int y, TMapCachePoint &point)
{
	ADDTOCALLSTACK("CMapCache::Get");
	if (( x < 0 ) || ( y < 0 ) || ( x >= m_maxX ) || ( y >= m_maxY ))
		return false;

	//	TODO:
	//	here should lie critical section to be thread-safe

	long seekpos = (( y * m_maxX ) + x) * sizeof(TMapCachePoint);
	if ( !fseek(m_hFile, seekpos, SEEK_SET) )
		return false;

	fread(&point, sizeof(TMapCachePoint), 1, m_hFile);
	return true;
}

bool CMapCache::Set(int x, int y, TMapCachePoint &point)
{
	ADDTOCALLSTACK("CMapCache::Set");
	if (( x < 0 ) || ( y < 0 ) || ( x >= m_maxX ) || ( y >= m_maxY ))
		return false;

	//	TODO:
	//	here should lie critical section to be thread-safe

	long seekpos = (( y * m_maxX ) + x) * sizeof(TMapCachePoint);
	if ( !fseek(m_hFile, seekpos, SEEK_SET) )
		return false;

	fwrite(&point, sizeof(TMapCachePoint), 1, m_hFile);
	return true;
}
