#include "graycom.h"
#include "CGrayInst.h"
#include "../graysvr/graysvr.h"

bool CGrayInstall::FindInstall()
{
#ifdef _WIN32
	// Get the install path from the registry
	static LPCTSTR m_szKeys[] =
	{
		"Software\\Electronic Arts\\EA Games\\Ultima Online Classic",
		"Software\\Electronic Arts\\EA Games\\Ultima Online Stygian Abyss Classic",
		"Software\\Origin Worlds Online\\Ultima Online\\KR Legacy Beta",
		"Software\\Origin Worlds Online\\Ultima Online Third Dawn\\1.0",
		"Software\\Origin Worlds Online\\Ultima Online\\1.0"
	};

	LSTATUS lRet = 0;
	HKEY hKey = NULL;
	for ( size_t i = 0; i < COUNTOF(m_szKeys); ++i )
	{
		lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, m_szKeys[i], 0, KEY_READ|KEY_WOW64_32KEY, &hKey);
		if ( lRet == ERROR_SUCCESS )
			break;
	}
	if ( lRet != ERROR_SUCCESS )
		return false;

	DWORD dwType = REG_SZ;
	TCHAR szValue[_MAX_PATH];
	DWORD dwSize = sizeof(szValue);
	lRet = RegQueryValueEx(hKey, "ExePath", NULL, &dwType, reinterpret_cast<BYTE *>(szValue), &dwSize);
	if ( (lRet == ERROR_SUCCESS) && (dwType == REG_SZ) )
	{
		TCHAR *pSlash = strrchr(szValue, '\\');		// get rid of the client.exe part of the name
		if ( pSlash )
			*pSlash = '\0';
		m_sExePath = szValue;
	}
	else
	{
		lRet = RegQueryValueEx(hKey, "InstallDir", NULL, &dwType, reinterpret_cast<BYTE *>(szValue), &dwSize);
		if ( (lRet == ERROR_SUCCESS) && (dwType == REG_SZ) )
			m_sExePath = szValue;
	}

	// Check CD-ROM install path as well, just in case
	lRet = RegQueryValueEx(hKey, "InstCDPath", NULL, &dwType, reinterpret_cast<BYTE *>(szValue), &dwSize);
	if ( (lRet == ERROR_SUCCESS) && (dwType == REG_SZ) )
		m_sCDPath = szValue;

	RegCloseKey(hKey);
#else
	// Linux has no registry, 'MulFiles' path must be set manually on sphere.ini
#endif
	return true;
}

void CGrayInstall::DetectMulVersions()
{
	ADDTOCALLSTACK("CGrayInstall::DetectMulVersions");

	// Assume all formats are original to start with
	for ( size_t i = 0; i < COUNTOF(m_FileFormat); ++i )
		m_FileFormat[i] = VERFORMAT_ORIGINAL;

	// Check for High Seas tiledata format
	// This can be tested checking the file size, which was 3188736 bytes at release
	if ( m_File[VERFILE_TILEDATA].IsFileOpen() && (m_File[VERFILE_TILEDATA].GetLength() >= 3188736) )
		m_FileFormat[VERFILE_TILEDATA] = VERFORMAT_HIGHSEAS;

	// Check for High Seas multi format
	// We can't use multi.mul length because it varies and multi.idx is always 98184 bytes, the best option
	// so far seems to be to check the size of the first entry to see if its length is divisible by the new
	// format length (risky if the first entry is custom and happens to be be divisible by both lengths)
	CUOIndexRec index;
	if ( ReadMulIndex(VERFILE_MULTIIDX, VERFILE_MULTI, 0x0, index) && (index.GetBlockLength() % sizeof(CUOMultiItemRec2) == 0) )
		m_FileFormat[VERFILE_MULTIIDX] = VERFORMAT_HIGHSEAS;
}

LPCTSTR CGrayInstall::GetBaseFileName(VERFILE_TYPE i)	// static
{
	ADDTOCALLSTACK("CGrayInstall::GetBaseFileName");
	static LPCTSTR const sm_szFileNames[VERFILE_QTY] =
	{
		"artidx.mul",
		"art.mul",
		"anim.idx",
		"anim.mul",
		"soundidx.mul",
		"sound.mul",
		"texidx.mul",
		"texmaps.mul",
		"gumpidx.mul",
		"gumpart.mul",
		"multi.idx",
		"multi.mul",
		"skills.idx",
		"skills.mul",
		"verdata.mul",
		"map0.mul",
		"staidx0.mul",
		"statics0.mul",
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		"tiledata.mul",
		"animdata.mul",
		"hues.mul"
	};

	if ( (i >= 0) && (i < VERFILE_QTY) )
		return sm_szFileNames[i];

	return NULL;
}

bool CGrayInstall::OpenFile(CGFile &file, LPCTSTR pszFileName, WORD wFlags)
{
	ADDTOCALLSTACK("CGrayInstall::OpenFile");
	ASSERT(pszFileName);
	if ( !m_sPreferPath.IsEmpty() && file.Open(GetPreferPath(pszFileName), wFlags) )
		return true;
	if ( file.Open(GetFullExePath(pszFileName), wFlags) )
		return true;
	if ( file.Open(GetFullCDPath(pszFileName), wFlags) )
		return true;

	return false;
}

bool CGrayInstall::OpenFile(VERFILE_TYPE i)
{
	ADDTOCALLSTACK("CGrayInstall::OpenFile");
	CGFile *pFile = GetMulFile(i);
	if ( !pFile )
		return false;

	if ( pFile->IsFileOpen() )
		return true;
	if ( !pFile->GetFilePath().IsEmpty() && pFile->Open(pFile->GetFilePath(), OF_READ|OF_SHARE_DENY_WRITE) )
		return true;

	LPCTSTR pszFileName = GetBaseFileName(i);
	if ( !pszFileName )
		return false;

	return OpenFile(m_File[i], pszFileName, OF_READ|OF_SHARE_DENY_WRITE);
}

VERFILE_TYPE CGrayInstall::OpenFiles(DWORD dwMask)
{
	ADDTOCALLSTACK("CGrayInstall::OpenFiles");
	// Open all the required files
	// RETURN:
	//  VERFILE_QTY = success

	int i = 0;
	for ( ; i < VERFILE_HUES; ++i )
	{
		if ( !(dwMask & (1 << i)) )
			continue;
		if ( !GetBaseFileName(static_cast<VERFILE_TYPE>(i)) )
			continue;

		bool fFileLoaded = true;
		switch ( i )
		{
			case VERFILE_MAP:
			{
				// Map file is handled differently
				TCHAR szFileName[_MAX_PATH];
				for ( size_t m = 0; m < MAP_QTY; ++m )
				{
					if ( g_MapList.IsInitialized(m) || (m == 0) )	// need at least a minimum of map0... (Ben)
					{
						int index = g_MapList.m_mapnum[m];
						if ( index == -1 )
						{
							g_MapList.m_maps[m] = false;
							continue;
						}

						if ( !m_Maps[index].IsFileOpen() )
						{
							sprintf(szFileName, "map%d.mul", index);
							OpenFile(m_Maps[index], szFileName, OF_READ|OF_SHARE_DENY_WRITE);

							if ( m_Maps[index].IsFileOpen() )
								m_IsMapUopFormat[index] = false;
							else
							{
								sprintf(szFileName, "map%dLegacyMUL.uop", index);
								OpenFile(m_Maps[index], szFileName, OF_READ|OF_SHARE_DENY_WRITE);

								// Should parse uop file here for faster reference later
								if ( m_Maps[index].IsFileOpen() )
								{
									m_IsMapUopFormat[index] = true;

									DWORD dwHashLo, dwHashHi, dwCompressedSize, dwHeaderLength, dwFilesInBlock, dwTotalFiles, dwLoop;
									UINT64 qwUOPPtr;

									m_Maps[index].Seek(sizeof(DWORD) * 3, SEEK_SET);
									m_Maps[index].Read(&dwHashLo, sizeof(DWORD));
									m_Maps[index].Read(&dwHashHi, sizeof(DWORD));
									qwUOPPtr = (static_cast<INT64>(dwHashHi) << 32) + dwHashLo;
									m_Maps[index].Seek(sizeof(DWORD), SEEK_CUR);
									m_Maps[index].Read(&dwTotalFiles, sizeof(DWORD));
									m_Maps[index].Seek(static_cast<LONG>(qwUOPPtr), SEEK_SET);
									dwLoop = dwTotalFiles;

									while ( qwUOPPtr > 0 )
									{
										m_Maps[index].Read(&dwFilesInBlock, sizeof(DWORD));
										m_Maps[index].Read(&dwHashLo, sizeof(DWORD));
										m_Maps[index].Read(&dwHashHi, sizeof(DWORD));
										qwUOPPtr = (static_cast<INT64>(dwHashHi) << 32) + dwHashLo;

										while ( (dwFilesInBlock > 0) && (dwTotalFiles > 0) )
										{
											--dwTotalFiles;
											--dwFilesInBlock;

											m_Maps[index].Read(&dwHashLo, sizeof(DWORD));
											m_Maps[index].Read(&dwHashHi, sizeof(DWORD));
											m_Maps[index].Read(&dwHeaderLength, sizeof(DWORD));
											m_Maps[index].Read(&dwCompressedSize, sizeof(DWORD));

											MapAddress pMapAddress;
											pMapAddress.qwAddress = ((static_cast<INT64>(dwHashHi) << 32) + dwHashLo) + dwHeaderLength;

											m_Maps[index].Seek(sizeof(DWORD), SEEK_CUR);
											m_Maps[index].Read(&dwHashLo, sizeof(DWORD));
											m_Maps[index].Read(&dwHashHi, sizeof(DWORD));
											UINT64 qwHash = (static_cast<INT64>(dwHashHi) << 32) + dwHashLo;
											m_Maps[index].Seek(sizeof(DWORD) + sizeof(WORD), SEEK_CUR);

											for ( DWORD x = 0; x < dwLoop; ++x )
											{
												sprintf(szFileName, "build/map%dlegacymul/%.8lu.dat", index, x);
												if ( HashFileName(szFileName) == qwHash )
												{
													pMapAddress.dwFirstBlock = x * 4096;
													pMapAddress.dwLastBlock = (x * 4096) + (dwCompressedSize / 196) - 1;
													m_UopMapAddress[index][x] = pMapAddress;
													break;
												}
											}
										}

										m_Maps[index].Seek(static_cast<LONG>(qwUOPPtr), SEEK_SET);
									}
								}
								else if ( index == 0 )	// neither file exists, map0 is required
								{
									fFileLoaded = false;
									break;
								}
							}
						}
						if ( !m_Staidx[index].IsFileOpen() )
						{
							sprintf(szFileName, "staidx%d.mul", index);
							OpenFile(m_Staidx[index], szFileName, OF_READ|OF_SHARE_DENY_WRITE);
						}
						if ( !m_Statics[index].IsFileOpen() )
						{
							sprintf(szFileName, "statics%d.mul", index);
							OpenFile(m_Statics[index], szFileName, OF_READ|OF_SHARE_DENY_WRITE);
						}
						if ( g_Cfg.m_fUseMapDiffs )
						{
							if ( !m_Mapdif[index].IsFileOpen() )
							{
								sprintf(szFileName, "mapdif%d.mul", index);
								OpenFile(m_Mapdif[index], szFileName, OF_READ|OF_SHARE_DENY_WRITE);
							}
							if ( !m_Mapdifl[index].IsFileOpen() )
							{
								sprintf(szFileName, "mapdifl%d.mul", index);
								OpenFile(m_Mapdifl[index], szFileName, OF_READ|OF_SHARE_DENY_WRITE);
							}
							if ( !m_Stadif[index].IsFileOpen() )
							{
								sprintf(szFileName, "stadif%d.mul", index);
								OpenFile(m_Stadif[index], szFileName, OF_READ|OF_SHARE_DENY_WRITE);
							}
							if ( !m_Stadifi[index].IsFileOpen() )
							{
								sprintf(szFileName, "stadifi%d.mul", index);
								OpenFile(m_Stadifi[index], szFileName, OF_READ|OF_SHARE_DENY_WRITE);
							}
							if ( !m_Stadifl[index].IsFileOpen() )
							{
								sprintf(szFileName, "stadifl%d.mul", index);
								OpenFile(m_Stadifl[index], szFileName, OF_READ|OF_SHARE_DENY_WRITE);
							}
						}

						// If any of the map files are not available, mark map as unavailable (mapdif and stadif files are not required)
						if ( !m_Maps[index].IsFileOpen() || !m_Staidx[index].IsFileOpen() || !m_Statics[index].IsFileOpen() )
						{
							if ( m_Maps[index].IsFileOpen() )
								m_Maps[index].Close();
							if ( m_Staidx[index].IsFileOpen() )
								m_Staidx[index].Close();
							if ( m_Statics[index].IsFileOpen() )
								m_Statics[index].Close();

							if ( (index == 1) && m_Maps[0].IsFileOpen() )
								g_MapList.m_mapnum[m] = 0;
							else
								g_MapList.m_mapid[m] = 0;
						}

						// mapdif and mapdifl are not required, but if one exists so should the other
						if ( m_Mapdif[index].IsFileOpen() != m_Mapdifl[index].IsFileOpen() )
						{
							if ( m_Mapdif[index].IsFileOpen() )
								m_Mapdif[index].Close();
							if ( m_Mapdifl[index].IsFileOpen() )
								m_Mapdifl[index].Close();
						}

						// If one of the stadif files exissts, so should the others
						if ( (m_Stadif[index].IsFileOpen() != m_Stadifi[index].IsFileOpen()) || (m_Stadif[index].IsFileOpen() != m_Stadifl[index].IsFileOpen()) )
						{
							if ( m_Stadif[index].IsFileOpen() )
								m_Stadif[index].Close();
							if ( m_Stadifi[index].IsFileOpen() )
								m_Stadifi[index].Close();
							if ( m_Stadifl[index].IsFileOpen() )
								m_Stadifl[index].Close();
						}
					}
				}
				break;
			}
			/*case VERFILE_MULTIIDX:
			{
				// If MultiCollection.uop is found, use it instead multi.idx + multi.mul (STILL INCOMPLETE)
				OpenFile(m_File[VERFILE_MULTI], "MultiCollection.uop", OF_READ|OF_SHARE_DENY_WRITE);
				if ( m_File[VERFILE_MULTI].IsFileOpen() )
				{
					m_IsMultiUopFormat = true;
					++i;	// skip VERFILE_MULTI
					break;
				}
				// fall through
			}*/
			default:
			{
				if ( !OpenFile(static_cast<VERFILE_TYPE>(i)) )
				{
					// Make some MULs optional
					switch ( i )
					{
						case VERFILE_VERDATA:
							break;
						default:
							fFileLoaded = false;
							break;
					}
				}
				break;
			}
		}

		// Stop on first failure
		if ( !fFileLoaded )
			break;
	}

	DetectMulVersions();
	g_MapList.Init();

	if ( i == VERFILE_HUES )	// return VERFILE_QTY if the for loop was completed successfully
		i = VERFILE_QTY;

	return static_cast<VERFILE_TYPE>(i);
}

void CGrayInstall::CloseFiles()
{
	ADDTOCALLSTACK("CGrayInstall::CloseFiles");

	for ( size_t i = 0; i < VERFILE_QTY; ++i )
	{
		if ( m_File[i].IsFileOpen() )
			m_File[i].Close();
	}

	for ( size_t i = 0; i < MAP_QTY; ++i )
	{
		if ( m_Maps[i].IsFileOpen() )
			m_Maps[i].Close();
		if ( m_Statics[i].IsFileOpen() )
			m_Statics[i].Close();
		if ( m_Staidx[i].IsFileOpen() )
			m_Staidx[i].Close();
		if ( m_Mapdif[i].IsFileOpen() )
			m_Mapdif[i].Close();
		if ( m_Mapdifl[i].IsFileOpen() )
			m_Mapdifl[i].Close();
		if ( m_Stadif[i].IsFileOpen() )
			m_Stadif[i].Close();
		if ( m_Stadifi[i].IsFileOpen() )
			m_Stadifi[i].Close();
		if ( m_Stadifl[i].IsFileOpen() )
			m_Stadifl[i].Close();
	}
}

bool CGrayInstall::ReadMulIndex(CGFile &file, DWORD id, CUOIndexRec &index)
{
	ADDTOCALLSTACK("CGrayInstall::ReadMulIndex");

	DWORD dwOffset = id * sizeof(CUOIndexRec);
	if ( file.Seek(dwOffset, SEEK_SET) != dwOffset )
		return false;

	if ( file.Read(static_cast<void *>(&index), sizeof(CUOIndexRec)) != sizeof(CUOIndexRec) )
		return false;

	return index.HasData();
}

bool CGrayInstall::ReadMulData(CGFile &file, const CUOIndexRec &index, void *pData)
{
	ADDTOCALLSTACK("CGrayInstall::ReadMulData");

	DWORD dwOffset = index.GetFileOffset();
	if ( file.Seek(dwOffset, SEEK_SET) != dwOffset )
		return false;

	DWORD dwLength = index.GetBlockLength();
	if ( file.Read(pData, dwLength) != dwLength )
		return false;

	return true;
}

bool CGrayInstall::ReadMulIndex(VERFILE_TYPE fileindex, VERFILE_TYPE filedata, DWORD id, CUOIndexRec &index)
{
	ADDTOCALLSTACK("CGrayInstall::ReadMulIndex");
	// Read about this data type in one of the index files
	// RETURN:
	//  true = success
	ASSERT(fileindex < VERFILE_QTY);

	// Is there an index for it in the verdata?
	if ( g_VerData.FindVerDataBlock(filedata, id, index) )
		return true;

	return ReadMulIndex(m_File[fileindex], id, index);
}

bool CGrayInstall::ReadMulData(VERFILE_TYPE filedata, const CUOIndexRec &index, void *pData)
{
	ADDTOCALLSTACK("CGrayInstall::ReadMulData");
	// Use CGFile::GetLastError() for error
	if ( index.IsVerData() )
		filedata = VERFILE_VERDATA;

	return ReadMulData(m_File[filedata], index, pData);
}

UINT64 CGrayInstall::HashFileName(CGString sFile)
{
	ADDTOCALLSTACK("CGrayInstall::HashFileName");
	// Get UOP filename hash

	UINT eax, ecx, edx, ebx, edi, esi;
	eax = ecx = edx = 0;
	ebx = edi = esi = static_cast<INT32>(sFile.GetLength()) + 0xDEADBEEF;

	size_t i = 0;
	for ( ; i + 12 < sFile.GetLength(); i += 12 )
	{
		edi = static_cast<INT32>((sFile[i +  7] << 24) | (sFile[i +  6] << 16) | (sFile[i + 5] << 8) | sFile[i + 4]) + edi;
		esi = static_cast<INT32>((sFile[i + 11] << 24) | (sFile[i + 10] << 16) | (sFile[i + 9] << 8) | sFile[i + 8]) + esi;
		edx = static_cast<INT32>((sFile[i +  3] << 24) | (sFile[i +  2] << 16) | (sFile[i + 1] << 8) | sFile[i]) - esi;

		edx = (edx + ebx) ^ (esi >> 28) ^ (esi << 4);
		esi += edi;
		edi = (edi - edx) ^ (edx >> 26) ^ (edx << 6);
		edx += esi;
		esi = (esi - edi) ^ (edi >> 24) ^ (edi << 8);
		edi += edx;
		ebx = (edx - esi) ^ (esi >> 16) ^ (esi << 16);
		esi += edi;
		edi = (edi - ebx) ^ (ebx >> 13) ^ (ebx << 19);
		ebx += esi;
		esi = (esi - edi) ^ (edi >> 28) ^ (edi << 4);
		edi += ebx;
	}

	if ( sFile.GetLength() - i > 0 )
	{
		switch ( sFile.GetLength() - i )
		{
			case 12:
				esi += static_cast<INT32>(sFile[i + 11]) << 24;
			case 11:
				esi += static_cast<INT32>(sFile[i + 10]) << 16;
			case 10:
				esi += static_cast<INT32>(sFile[i + 9]) << 8;
			case 9:
				esi += static_cast<INT32>(sFile[i + 8]);
			case 8:
				edi += static_cast<INT32>(sFile[i + 7]) << 24;
			case 7:
				edi += static_cast<INT32>(sFile[i + 6]) << 16;
			case 6:
				edi += static_cast<INT32>(sFile[i + 5]) << 8;
			case 5:
				edi += static_cast<INT32>(sFile[i + 4]);
			case 4:
				ebx += static_cast<INT32>(sFile[i + 3]) << 24;
			case 3:
				ebx += static_cast<INT32>(sFile[i + 2]) << 16;
			case 2:
				ebx += static_cast<INT32>(sFile[i + 1]) << 8;
			case 1:
				ebx += static_cast<INT32>(sFile[i]);
				break;
		}

		esi = (esi ^ edi) - ((edi >> 18) ^ (edi << 14));
		ecx = (esi ^ ebx) - ((esi >> 21) ^ (esi << 11));
		edi = (edi ^ ecx) - ((ecx >>  7) ^ (ecx << 25));
		esi = (esi ^ edi) - ((edi >> 16) ^ (edi << 16));
		edx = (esi ^ ecx) - ((esi >> 28) ^ (esi <<  4));
		edi = (edi ^ edx) - ((edx >> 18) ^ (edx << 14));
		eax = (esi ^ edi) - ((edi >>  8) ^ (edi << 24));

		return (static_cast<INT64>(edi) << 32) | eax;
	}

	return (static_cast<INT64>(esi) << 32) | eax;
}