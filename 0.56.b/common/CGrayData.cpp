//
// CGrayData.Cpp
//

#include "../graysvr/graysvr.h"

//////////////////////////////////////////////////////////////////////
// -CVerDataMul

int CVerDataMul::QCompare( size_t left, DWORD dwRefIndex ) const
{
	DWORD dwIndex2 = GetEntry(left)->GetIndex();
	return( dwIndex2 - dwRefIndex );
}

void CVerDataMul::QSort( size_t left, size_t right )
{
	ADDTOCALLSTACK("CVerDataMul::QSort");
	static int iReentrant = 0;
	ASSERT( left <= right );
	size_t j = left;
	size_t i = right;

	DWORD dwRefIndex = GetEntry( (left + right) / 2 )->GetIndex();

	do
	{
		while ( j < m_Data.GetCount() && QCompare( j, dwRefIndex ) < 0 )
			j++;
		while ( i > 0 && QCompare( i, dwRefIndex ) > 0 )
			i--;

		if ( i >= j )
		{
			if ( i != j )
			{
				CUOVersionBlock Tmp = m_Data.GetAt(i);
				CUOVersionBlock block = m_Data.GetAt(j);
				m_Data.SetAt( i, block );
				m_Data.SetAt( j, Tmp );
			}

			if ( i > 0 )
				i--;
			if ( j < m_Data.GetCount() )
				j++;
		}

	} while (j <= i);

	iReentrant++;
	if (left < i)
		QSort(left, i);
	if (j < right)
		QSort(j, right);
	iReentrant--;
}

void CVerDataMul::Load( CGFile & file )
{
	ADDTOCALLSTACK("CVerDataMul::Load");
	// assume it is in sorted order.
	if ( GetCount())	// already loaded.
	{
		return;
	}

// #define g_fVerData g_Install.m_File[VERFILE_VERDATA]

	if ( ! file.IsFileOpen())		// T2a might not have this.
		return;

	file.SeekToBegin();
	DWORD dwQty;
	if ( file.Read(static_cast<void *>(&dwQty), sizeof(dwQty)) <= 0 )
	{
		throw CGrayError( LOGL_CRIT, CGFile::GetLastError(), "VerData: Read Qty");
	}

	Unload();
	m_Data.SetCount( dwQty );

	if ( file.Read(static_cast<void *>(m_Data.GetBasePtr()), dwQty * sizeof( CUOVersionBlock )) <= 0 )
	{
		throw CGrayError( LOGL_CRIT, CGFile::GetLastError(), "VerData: Read");
	}

	if ( dwQty <= 0 )
		return;

	// Now sort it for fast searching.
	// Make sure it is sorted.
	QSort( 0, dwQty - 1 );

#ifdef _DEBUG
	for ( size_t i = 0; i < (dwQty - 1); i++ )
	{
		DWORD dwIndex1 = GetEntry(i)->GetIndex();
		DWORD dwIndex2 = GetEntry(i + 1)->GetIndex();
		if ( dwIndex1 > dwIndex2 )
		{
			DEBUG_ERR(( "VerData Array is NOT sorted !\n" ));
			throw CGrayError( LOGL_CRIT, -1, "VerData: NOT Sorted!");
			break;
		}
	}
#endif

}

bool CVerDataMul::FindVerDataBlock( VERFILE_TYPE type, DWORD id, CUOIndexRec & Index ) const
{
	ADDTOCALLSTACK("CVerDataMul::FindVerDataBlock");
	// Search the verdata.mul for changes to stuff.
	// search for a specific block.
	// assume it is in sorted order.

	int iHigh = GetCount()-1;
	if ( iHigh < 0 )
	{
		return( false );
	}

	DWORD dwIndex = VERDATA_MAKE_INDEX(type,id);
	const CUOVersionBlock * pArray = m_Data.GetBasePtr();
	int iLow = 0;
	while ( iLow <= iHigh )
	{
		int i = (iHigh+iLow)/2;
		DWORD dwIndex2 = pArray[i].GetIndex();
		int iCompare = dwIndex - dwIndex2;
		if ( iCompare == 0 )
		{
			Index.CopyIndex( &(pArray[i]));
			return( true );
		}
		if ( iCompare > 0 )
		{
			iLow = i+1;
		}
		else
		{
			iHigh = i-1;
		}
	}
	return( false );
}

//*********************************************8
// -CGrayItemInfo

CGrayItemInfo::CGrayItemInfo( ITEMID_TYPE id )
{
	if ( id >= ITEMID_MULTI )
	{
		// composite/multi type objects.
		m_flags = 0;
		m_weight = 0xFF;
		m_layer = LAYER_NONE;
		m_dwAnim = 0;
		m_height = 0;
		strcpy( m_name, ( id <= ITEMID_SHIP6_W ) ? "ship" : "structure" );
		return;
	}

	VERFILE_TYPE filedata;
	DWORD offset;
	CUOIndexRec Index;
	VERFILE_FORMAT format;
	if ( g_VerData.FindVerDataBlock( VERFILE_TILEDATA, (id + TERRAIN_QTY) / UOTILE_BLOCK_QTY, Index ))
	{
		filedata = VERFILE_VERDATA;
		format = VERFORMAT_ORIGINAL;
		offset = Index.GetFileOffset() + 4 + (sizeof(CUOItemTypeRec) * (id % UOTILE_BLOCK_QTY));
		ASSERT( Index.GetBlockLength() >= sizeof( CUOItemTypeRec ));
	}
	else
	{
		filedata = VERFILE_TILEDATA;
		format = g_Install.GetMulFormat(filedata);
		
		switch (format)
		{
			case VERFORMAT_HIGHSEAS: // high seas format (CUOItemTypeRec2)
				offset = UOTILE_TERRAIN_SIZE2 + 4 + (( id / UOTILE_BLOCK_QTY ) * 4 ) + ( id * sizeof( CUOItemTypeRec2 ));
				break;

			case VERFORMAT_ORIGINAL: // original format (CUOItemTypeRec)
			default:
				offset = UOTILE_TERRAIN_SIZE + 4 + (( id / UOTILE_BLOCK_QTY ) * 4 ) + ( id * sizeof( CUOItemTypeRec ));
				break;
		}
	}

	if ( g_Install.m_File[filedata].Seek( offset, SEEK_SET ) != offset )
	{
		throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "CTileItemType.ReadInfo: TileData Seek");
	}

	switch (format)
	{
		case VERFORMAT_HIGHSEAS: // high seas format (CUOItemTypeRec2)
			if ( g_Install.m_File[filedata].Read( static_cast <CUOItemTypeRec2 *>(this), sizeof(CUOItemTypeRec2)) <= 0 )
				throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "CTileItemType.ReadInfo: TileData Read");
			break;

		case VERFORMAT_ORIGINAL: // old format (CUOItemTypeRec)
		default:
		{
			CUOItemTypeRec record;
			if ( g_Install.m_File[filedata].Read( static_cast <CUOItemTypeRec *>(&record), sizeof(CUOItemTypeRec)) <= 0 )
				throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "CTileTerrainType.ReadInfo: TileData Read");
			
			m_flags = record.m_flags;
			m_weight = record.m_weight;
			m_layer = record.m_layer;
			m_dwAnim = record.m_dwAnim;
			m_height = record.m_height;
			m_wUnk19 = record.m_wUnk14;
			m_dwUnk11 = record.m_dwUnk6;
			m_dwUnk5 = 0;
			strcpylen(m_name, record.m_name, COUNTOF(m_name));
			break;
		}
	}
}

ITEMID_TYPE CGrayItemInfo::GetMaxTileDataItem()
{
	ADDTOCALLSTACK("CGrayItemInfo::GetMaxTileDataItem");

	CGFile* pTileData = g_Install.GetMulFile(VERFILE_TILEDATA);
	ASSERT(pTileData != NULL);

	DWORD dwLength = pTileData->GetLength();	// length of file
	DWORD dwEntrySize = 0;						// size of tiledata entry
	DWORD dwOffset = 0;							// offset to tiledata items

	VERFILE_FORMAT format = g_Install.GetMulFormat(VERFILE_TILEDATA);
	switch (format)
	{
		case VERFORMAT_HIGHSEAS: // high seas format (CUOItemTypeRec2)
			dwEntrySize = sizeof(CUOItemTypeRec2);
			dwOffset = UOTILE_TERRAIN_SIZE2 + 4;
			break;

		case VERFORMAT_ORIGINAL: // original format (CUOItemTypeRec)
		default:
			dwEntrySize = sizeof(CUOItemTypeRec);
			dwOffset = UOTILE_TERRAIN_SIZE + 4;
			break;
	}

	ASSERT(dwEntrySize > 0);
	ASSERT(dwOffset < dwLength);

	// items are sorted in blocks of 32 with 4 byte padding between, so determine how
	// many blocks will fit in the file to find how many items there could be
	dwLength -= dwOffset;
	DWORD dwBlocks = (dwLength / ((UOTILE_BLOCK_QTY * dwEntrySize) + 4)) + 1;
	return static_cast<ITEMID_TYPE>(dwBlocks * UOTILE_BLOCK_QTY);
}

//*********************************************8
// -CGrayTerrainInfo

CGrayTerrainInfo::CGrayTerrainInfo( TERRAIN_TYPE id )
{
	ASSERT( id < TERRAIN_QTY );

	VERFILE_TYPE filedata;
	DWORD offset;
	CUOIndexRec Index;
	VERFILE_FORMAT format;
	if ( g_VerData.FindVerDataBlock( VERFILE_TILEDATA, id/UOTILE_BLOCK_QTY, Index ))
	{
		filedata = VERFILE_VERDATA;
		format = VERFORMAT_ORIGINAL;
		offset = Index.GetFileOffset() + 4 + (sizeof(CUOTerrainTypeRec)*(id%UOTILE_BLOCK_QTY));
		ASSERT( Index.GetBlockLength() >= sizeof( CUOTerrainTypeRec ));
	}
	else
	{
		filedata = VERFILE_TILEDATA;
		format = g_Install.GetMulFormat(filedata);

		switch (format)
		{
			case VERFORMAT_HIGHSEAS: // high seas format (CUOTerrainTypeRec2)
				offset = (id == 0? 0 : 4) + (( id / UOTILE_BLOCK_QTY ) * 4 ) + ( id * sizeof( CUOTerrainTypeRec2 ));
				break;

			case VERFORMAT_ORIGINAL: // original format (CUOTerrainTypeRec)
			default:
				offset = 4 + (( id / UOTILE_BLOCK_QTY ) * 4 ) + ( id * sizeof( CUOTerrainTypeRec ));
				break;
		}
	}

	if ( g_Install.m_File[filedata].Seek( offset, SEEK_SET ) != offset )
	{
		throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "CTileTerrainType.ReadInfo: TileData Seek");
	}

	switch (format)
	{
		case VERFORMAT_HIGHSEAS: // high seas format (CUOTerrainTypeRec2)
			if ( g_Install.m_File[filedata].Read(static_cast <CUOTerrainTypeRec2 *>(this), sizeof(CUOTerrainTypeRec2)) <= 0 )
				throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "CTileTerrainType.ReadInfo: TileData Read");
			break;

		case VERFORMAT_ORIGINAL: // old format (CUOTerrainTypeRec)
		default:
		{
			CUOTerrainTypeRec record;
			if ( g_Install.m_File[filedata].Read(static_cast <CUOTerrainTypeRec *>(&record), sizeof(CUOTerrainTypeRec)) <= 0 )
				throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "CTileTerrainType.ReadInfo: TileData Read");

			m_flags = record.m_flags;
			m_unknown = 0;
			m_index = record.m_index;
			strcpylen(m_name, record.m_name, COUNTOF(m_name));
			break;
		}
	}
}

//*********************************************8
// -CGrayMulti

size_t CGrayMulti::Load( MULTI_TYPE id )
{
	ADDTOCALLSTACK("CGrayMulti::Load");
	// Just load the whole thing.

	Release();
	InitCacheTime();		// This is invalid !

	if ( id < 0 || id >= MULTI_QTY )
		return( 0 );
	m_id = id;

	CUOIndexRec Index;
	if ( ! g_Install.ReadMulIndex( VERFILE_MULTIIDX, VERFILE_MULTI, id, Index ))
		return 0;

	switch ( g_Install.GetMulFormat( VERFILE_MULTIIDX ) )
	{
		case VERFORMAT_HIGHSEAS: // high seas multi format (CUOMultiItemRec2)
			m_iItemQty = Index.GetBlockLength() / sizeof(CUOMultiItemRec2);
			m_pItems = new CUOMultiItemRec2 [ m_iItemQty ];
			ASSERT( m_pItems );

			ASSERT( (sizeof(m_pItems[0]) * m_iItemQty) >= Index.GetBlockLength() );
			if ( ! g_Install.ReadMulData( VERFILE_MULTI, Index, static_cast <CUOMultiItemRec2 *>(m_pItems) ))
				return 0;
			break;

		case VERFORMAT_ORIGINAL: // old format (CUOMultiItemRec)
		default:
			m_iItemQty = Index.GetBlockLength() / sizeof(CUOMultiItemRec);
			m_pItems = new CUOMultiItemRec2 [ m_iItemQty ];
			ASSERT( m_pItems );

			CUOMultiItemRec* pItems = new CUOMultiItemRec[m_iItemQty];
			ASSERT( (sizeof(pItems[0]) * m_iItemQty) >= Index.GetBlockLength() );
			if ( ! g_Install.ReadMulData( VERFILE_MULTI, Index, static_cast <CUOMultiItemRec *>(pItems) ))
			{
				delete[] pItems;
				return 0;
			}

			// copy to new format
			for (size_t i = 0; i < m_iItemQty; i++)
			{
				m_pItems[i].m_wTileID = pItems[i].m_wTileID;
				m_pItems[i].m_dx = pItems[i].m_dx;
				m_pItems[i].m_dy = pItems[i].m_dy;
				m_pItems[i].m_dz = pItems[i].m_dz;
				m_pItems[i].m_visible = pItems[i].m_visible;
				m_pItems[i].m_unknown = 0;
			}

			delete[] pItems;
			break;
	}

	HitCacheTime();
	return( m_iItemQty );
}
