//
// CGrayData.Cpp
//

#include "../graysvr.h"

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
	long offset;
	CUOIndexRec Index;
	filedata = VERFILE_TILEDATA;
	offset = UOTILE_TERRAIN_SIZE + 4 + (( id / UOTILE_BLOCK_QTY ) * 4 ) + ( id * sizeof( CUOItemTypeRec ));

	if ( g_Install.m_File[filedata].Seek( offset, SEEK_SET ) != offset )
	{
		throw CError(LOGL_CRIT, CGFile::GetLastError(), "CTileItemType.ReadInfo: TileData Seek");
	}

	if ( g_Install.m_File[filedata].Read( static_cast <CUOItemTypeRec *>(this), sizeof(CUOItemTypeRec)) <= 0 )
	{
		throw CError(LOGL_CRIT, CGFile::GetLastError(), "CTileItemType.ReadInfo: TileData Read");
	}
}

CGrayTerrainInfo::CGrayTerrainInfo( TERRAIN_TYPE id )
{
	VERFILE_TYPE filedata;
	long offset;
	CUOIndexRec Index;
	filedata = VERFILE_TILEDATA;
	offset = 4 + (( id / UOTILE_BLOCK_QTY ) * 4 ) + ( id * sizeof( CUOTerrainTypeRec ));

	if ( g_Install.m_File[filedata].Seek( offset, SEEK_SET ) != offset )
	{
		throw CError(LOGL_CRIT, CGFile::GetLastError(), "CTileTerrainType.ReadInfo: TileData Seek");
	}

	if ( g_Install.m_File[filedata].Read(static_cast <CUOTerrainTypeRec *>(this), sizeof(CUOTerrainTypeRec)) <= 0 )
	{
		throw CError(LOGL_CRIT, CGFile::GetLastError(), "CTileTerrainType.ReadInfo: TileData Read");
	}
}

int CGrayMulti::Load( MULTI_TYPE id )
{
	// Just load the whole thing.

	ReleaseMulti();
	InitCacheTime();		// This is invalid !

	if ( id < 0 || id > MULTI_QTY )
		return 0;
	m_id = id;

	CUOIndexRec Index;
	if ( ! g_Install.ReadMulIndex( VERFILE_MULTIIDX, VERFILE_MULTI, id, Index ))
		return 0;

	m_iItemQty = Index.GetBlockLength() / sizeof(CUOMultiItemRec);
	m_pItems = new CUOMultiItemRec [ m_iItemQty ];

	if ( ! g_Install.ReadMulData( VERFILE_MULTI, Index, (void*) m_pItems ))
	{
		return 0;
	}

	HitCacheTime();
	return( m_iItemQty );
}
