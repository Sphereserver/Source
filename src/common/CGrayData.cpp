#include "../graysvr/graysvr.h"

#define UOTILE_BLOCK_SIZE		32		// bytes per block
#define UOTILE_BLOCK_PADDING	4		// bytes between blocks
#define UOTILE_TERRAIN_SIZE		(((TERRAIN_QTY / UOTILE_BLOCK_SIZE) * UOTILE_BLOCK_PADDING) + (TERRAIN_QTY * sizeof(CUOTerrainTypeRec)))
#define UOTILE_TERRAIN_SIZE_HS	(((TERRAIN_QTY / UOTILE_BLOCK_SIZE) * UOTILE_BLOCK_PADDING) + (TERRAIN_QTY * sizeof(CUOTerrainTypeRecHS)))

///////////////////////////////////////////////////////////
// CVerDataMul

void CVerDataMul::QSort(size_t left, size_t right)
{
	ADDTOCALLSTACK("CVerDataMul::QSort");
	ASSERT(left <= right);

	static int sm_iReentrantCount = 0;
	DWORD dwIndex = GetEntry((left + right) / 2)->GetIndex();
	size_t j = left;
	size_t i = right;

	do
	{
		while ( (j < m_Data.GetCount()) && (static_cast<int>(GetEntry(j)->GetIndex() - dwIndex) < 0) )
			++j;
		while ( (i > 0) && (static_cast<int>(GetEntry(i)->GetIndex() - dwIndex) > 0) )
			--i;

		if ( i >= j )
		{
			if ( i != j )
			{
				CUOVersionBlock temp = m_Data.GetAt(i);
				CUOVersionBlock block = m_Data.GetAt(j);
				m_Data.SetAt(i, block);
				m_Data.SetAt(j, temp);
			}

			if ( i > 0 )
				--i;
			if ( j < m_Data.GetCount() )
				++j;
		}

	} while ( j <= i );

	++sm_iReentrantCount;
	if ( left < i )
		QSort(left, i);
	if ( j < right )
		QSort(j, right);
	--sm_iReentrantCount;
}

void CVerDataMul::Load(CGFile &file)
{
	ADDTOCALLSTACK("CVerDataMul::Load");
	if ( GetCount() || !file.IsFileOpen() )
		return;

	file.Seek(0, FILE_BEGIN);

	DWORD dwQty;
	if ( file.Read(static_cast<void *>(&dwQty), sizeof(dwQty)) == 0 )
		throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "VerData: Read Qty");

	Unload();
	m_Data.SetCount(dwQty);

	if ( file.Read(static_cast<void *>(m_Data.GetData()), dwQty * sizeof(CUOVersionBlock)) == 0 )
		throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "VerData: Read");

	if ( dwQty == 0 )
		return;

	// To faster searching, make sure it is sorted
	QSort(0, dwQty - 1);
}

bool CVerDataMul::FindVerDataBlock(VERFILE_TYPE type, DWORD id, CUOIndexRec &index) const
{
	ADDTOCALLSTACK("CVerDataMul::FindVerDataBlock");
	// Search specific block on verdata.mul file
	// Assume it is in sorted order

	int iHigh = static_cast<int>(GetCount()) - 1;
	if ( iHigh < 0 )
		return false;

	const DWORD dwIndex = VERDATA_MAKE_INDEX(type, id);
	const CUOVersionBlock *pArray = m_Data.GetData();

	int iLow = 0;
	while ( iLow <= iHigh )
	{
		int i = (iHigh + iLow) / 2;
		int iCompare = dwIndex - pArray[i].GetIndex();
		if ( iCompare == 0 )
		{
			index.CopyIndex(&pArray[i]);
			return true;
		}
		else if ( iCompare > 0 )
			iLow = i + 1;
		else
			iHigh = i - 1;
	}
	return false;
}

///////////////////////////////////////////////////////////
// CGrayItemInfo

CGrayItemInfo::CGrayItemInfo(ITEMID_TYPE id)
{
	if ( id >= ITEMID_MULTI )
	{
		m_flags = 0;
		m_weight = BYTE_MAX;
		m_layer = LAYER_NONE;
		m_wAnim = 0;
		m_wHue = 0;
		m_wLight = 0;
		m_height = 0;
		strncpy(m_name, CItemBase::IsID_Ship(id) ? "ship" : "structure", sizeof(m_name));
		m_name[sizeof(m_name) - 1] = '\0';
		return;
	}

	VERFILE_TYPE type;
	VERFILE_FORMAT format;
	DWORD dwOffset;
	CUOIndexRec index;

	if ( g_VerData.FindVerDataBlock(VERFILE_TILEDATA, (id + TERRAIN_QTY) / UOTILE_BLOCK_SIZE, index) )
	{
		type = VERFILE_VERDATA;
		format = VERFORMAT_ORIGINAL;
		dwOffset = index.GetFileOffset() + UOTILE_BLOCK_PADDING + (sizeof(CUOItemTypeRec) * (id % UOTILE_BLOCK_SIZE));
		ASSERT(index.GetBlockLength() >= sizeof(CUOItemTypeRec));
	}
	else
	{
		type = VERFILE_TILEDATA;
		format = g_Install.GetMulFormat(type);
		if ( format == VERFORMAT_HIGHSEAS )
			dwOffset = UOTILE_TERRAIN_SIZE_HS + UOTILE_BLOCK_PADDING + ((id / static_cast<size_t>(UOTILE_BLOCK_SIZE)) * UOTILE_BLOCK_PADDING) + (id * sizeof(CUOItemTypeRecHS));
		else
			dwOffset = UOTILE_TERRAIN_SIZE + UOTILE_BLOCK_PADDING + ((id / static_cast<size_t>(UOTILE_BLOCK_SIZE)) * UOTILE_BLOCK_PADDING) + (id * sizeof(CUOItemTypeRec));
	}

	if ( g_Install.m_File[type].Seek(dwOffset, SEEK_SET) != dwOffset )
		throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "CTileItemType.ReadInfo: TileData Seek");

	if ( format == VERFORMAT_HIGHSEAS )
	{
		if ( g_Install.m_File[type].Read(static_cast<CUOItemTypeRecHS *>(this), sizeof(CUOItemTypeRecHS)) == 0 )
			throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "CTileItemType.ReadInfo: TileData Read");
	}
	else
	{
		CUOItemTypeRec record;
		if ( g_Install.m_File[type].Read(static_cast<CUOItemTypeRec *>(&record), sizeof(CUOItemTypeRec)) == 0 )
			throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "CTileTerrainType.ReadInfo: TileData Read");

		m_flags = record.m_flags;
		m_weight = record.m_weight;
		m_layer = record.m_layer;
		m_dwUnk11 = record.m_dwUnk6;
		m_wAnim = record.m_wAnim;
		m_wHue = record.m_wHue;
		m_wLight = record.m_wLight;
		m_height = record.m_height;
		strncpy(m_name, record.m_name, sizeof(m_name));
		m_name[sizeof(m_name) - 1] = '\0';
	}
}

ITEMID_TYPE CGrayItemInfo::GetMaxTileDataItem()
{
	ADDTOCALLSTACK("CGrayItemInfo::GetMaxTileDataItem");
	// Tiledata is sorted in blocks of 32 bytes with 4 bytes padding between, so
	// find how many blocks will fit in the file to find how many items it have

	CGFile *pTileData = g_Install.GetMulFile(VERFILE_TILEDATA);
	ASSERT(pTileData);

	DWORD dwLength = pTileData->GetLength();	// file length
	DWORD dwEntrySize = 0;						// size of tiledata entry
	DWORD dwOffset = 0;							// offset to tiledata items

	if ( g_Install.GetMulFormat(VERFILE_TILEDATA) == VERFORMAT_HIGHSEAS )
	{
		dwEntrySize = sizeof(CUOItemTypeRecHS);
		dwOffset = UOTILE_TERRAIN_SIZE_HS + UOTILE_BLOCK_PADDING;
	}
	else
	{
		dwEntrySize = sizeof(CUOItemTypeRec);
		dwOffset = UOTILE_TERRAIN_SIZE + UOTILE_BLOCK_PADDING;
	}

	ASSERT(dwEntrySize > 0);
	ASSERT(dwOffset < dwLength);

	dwLength -= dwOffset;
	DWORD dwBlocks = (dwLength / ((UOTILE_BLOCK_SIZE * dwEntrySize) + UOTILE_BLOCK_PADDING)) + 1;
	return static_cast<ITEMID_TYPE>(dwBlocks * UOTILE_BLOCK_SIZE);
}

///////////////////////////////////////////////////////////
// CGrayTerrainInfo

CGrayTerrainInfo::CGrayTerrainInfo(TERRAIN_TYPE id)
{
	ASSERT(id < TERRAIN_QTY);

	VERFILE_TYPE type;
	VERFILE_FORMAT format;
	DWORD dwOffset;
	CUOIndexRec index;

	if ( g_VerData.FindVerDataBlock(VERFILE_TILEDATA, id / UOTILE_BLOCK_SIZE, index) )
	{
		type = VERFILE_VERDATA;
		format = VERFORMAT_ORIGINAL;
		dwOffset = index.GetFileOffset() + UOTILE_BLOCK_PADDING + (sizeof(CUOTerrainTypeRec) * (id % UOTILE_BLOCK_SIZE));
		ASSERT(index.GetBlockLength() >= sizeof(CUOTerrainTypeRec));
	}
	else
	{
		type = VERFILE_TILEDATA;
		format = g_Install.GetMulFormat(type);
		if ( format == VERFORMAT_HIGHSEAS )
			dwOffset = UOTILE_BLOCK_PADDING + ((id / UOTILE_BLOCK_SIZE) * UOTILE_BLOCK_PADDING) + (id * sizeof(CUOTerrainTypeRecHS));
		else
			dwOffset = UOTILE_BLOCK_PADDING + ((id / UOTILE_BLOCK_SIZE) * UOTILE_BLOCK_PADDING) + (id * sizeof(CUOTerrainTypeRec));
	}

	if ( g_Install.m_File[type].Seek(dwOffset, SEEK_SET) != dwOffset )
		throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "CTileTerrainType.ReadInfo: TileData Seek");

	if ( format == VERFORMAT_HIGHSEAS )
	{
		if ( g_Install.m_File[type].Read(static_cast<CUOTerrainTypeRecHS *>(this), sizeof(CUOTerrainTypeRecHS)) == 0 )
			throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "CTileTerrainType.ReadInfo: TileData Read");
	}
	else
	{
		CUOTerrainTypeRec record;
		if ( g_Install.m_File[type].Read(static_cast<CUOTerrainTypeRec *>(&record), sizeof(CUOTerrainTypeRec)) == 0 )
			throw CGrayError(LOGL_CRIT, CGFile::GetLastError(), "CTileTerrainType.ReadInfo: TileData Read");

		m_flags = record.m_flags;
		m_unknown = 0;
		m_index = record.m_index;
		strncpy(m_name, record.m_name, sizeof(m_name));
		m_name[sizeof(m_name) - 1] = '\0';
	}
}

///////////////////////////////////////////////////////////
// CGrayMulti

size_t CGrayMulti::Load(MULTI_TYPE id)
{
	ADDTOCALLSTACK("CGrayMulti::Load");
	// Just load the whole thing

	if ( id >= MULTI_QTY )
		return 0;

	Release();
	InitCacheTime();	// this is invalid
	m_id = id;

	//if ( g_Install.m_IsMultiUopFormat )
	//	TO-DO: add support to read new multi UOP format here (MultiCollection.uop)
	//else
	//{
		CUOIndexRec index;
		if ( !g_Install.ReadMulIndex(VERFILE_MULTIIDX, VERFILE_MULTI, id, index) )
			return 0;

		if ( g_Install.GetMulFormat(VERFILE_MULTIIDX) == VERFORMAT_HIGHSEAS )
		{
			m_iItemQty = index.GetBlockLength() / sizeof(CUOMultiItemRecHS);
			m_pItems = new CUOMultiItemRecHS[m_iItemQty];
			ASSERT(m_pItems);

			ASSERT(m_iItemQty * sizeof(m_pItems[0]) >= index.GetBlockLength());
			if ( !g_Install.ReadMulData(VERFILE_MULTI, index, static_cast<void *>(m_pItems)) )
				return 0;
		}
		else
		{
			m_iItemQty = index.GetBlockLength() / sizeof(CUOMultiItemRec);
			m_pItems = new CUOMultiItemRecHS[m_iItemQty];
			ASSERT(m_pItems);

			CUOMultiItemRec *pItems = new CUOMultiItemRec[m_iItemQty];
			ASSERT(m_iItemQty * sizeof(pItems[0]) >= index.GetBlockLength());
			if ( !g_Install.ReadMulData(VERFILE_MULTI, index, static_cast<void *>(pItems)) )
			{
				delete[] pItems;
				return 0;
			}

			// Copy to new format
			for ( size_t i = 0; i < m_iItemQty; ++i )
			{
				m_pItems[i].m_wTileID = pItems[i].m_wTileID;
				m_pItems[i].m_dx = pItems[i].m_dx;
				m_pItems[i].m_dy = pItems[i].m_dy;
				m_pItems[i].m_dz = pItems[i].m_dz;
				m_pItems[i].m_visible = pItems[i].m_visible;
				m_pItems[i].m_shipAccess = 0;
			}
			delete[] pItems;
		}
	//}

	HitCacheTime();
	return m_iItemQty;
}
