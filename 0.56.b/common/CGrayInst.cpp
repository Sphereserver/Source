//
// GRAYINST.CPP
// Copyright Menace Software (www.menasoft.com).
//

#include "graycom.h"
#include "CGrayInst.h"
#include "../graysvr/graysvr.h"

bool CGrayInstall::FindInstall()
{
#ifdef _WIN32
	// Get the install path from the registry.

	static LPCTSTR m_szKeys[] =
	{
		"Software\\Menasoft\\" GRAY_FILE,
		"Software\\Origin Worlds Online\\Ultima Online\\1.0", 
		"Software\\Origin Worlds Online\\Ultima Online\\KR Legacy Beta", 
		"Software\\Origin Worlds Online\\Ultima Online Third Dawn\\1.0", 
		"Electronic Arts\\EA Games\\Ultima Online Stygian Abyss Classic"
	};
	
	HKEY hKey = NULL;
	LSTATUS lRet = 0;
	for ( size_t i = 0; i < COUNTOF(m_szKeys); i++ )
	{
		lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
			m_szKeys[i], // address of name of subkey to query
			0, KEY_READ, &hKey );
		if ( lRet == ERROR_SUCCESS )
			break;
	}
	if ( lRet != ERROR_SUCCESS )
	{
		return( false );
	}

	TCHAR szValue[ _MAX_PATH ];
	DWORD lSize = sizeof( szValue );
	DWORD dwType = REG_SZ;
	lRet = RegQueryValueEx(hKey, "ExePath", NULL, &dwType, (BYTE*)szValue, &lSize);

	if ( lRet == ERROR_SUCCESS && dwType == REG_SZ )
	{
		TCHAR * pSlash = strrchr( szValue, '\\' );	// get rid of the client.exe part of the name
		if ( pSlash ) * pSlash = '\0';
		m_sExePath = szValue;
	}
	else
	{
		lRet = RegQueryValueEx(hKey, "InstallDir", NULL, &dwType, (BYTE*)szValue, &lSize);
		if ( lRet == ERROR_SUCCESS && dwType == REG_SZ )
			m_sExePath = szValue;
	}

	// ??? Find CDROM install base as well, just in case.
	// uo.cfg CdRomDataPath=e:\uo

	lRet = RegQueryValueEx(hKey, "InstCDPath", NULL, &dwType, (BYTE*)szValue, &lSize);

	if ( lRet == ERROR_SUCCESS && dwType == REG_SZ )
	{
		m_sCDPath = szValue;
	}

	RegCloseKey( hKey );

#else
	// LINUX has no registry so we must have the INI file show us where it is installed.
#endif
	return true;
}

void CGrayInstall::DetectMulVersions()
{
	ADDTOCALLSTACK("CGrayInstall::DetectMulVersions");

	// assume all formats are original to start with
	for (size_t i = 0; i < COUNTOF(m_FileFormat); i++)
		m_FileFormat[i] = VERFORMAT_ORIGINAL;

	// check for High Seas tiledata format
	// this can be tested for by checking the file size, which was 3188736 bytes at release
	if ( m_File[VERFILE_TILEDATA].IsFileOpen() && m_File[VERFILE_TILEDATA].GetLength() >= 3188736 )
		m_FileFormat[VERFILE_TILEDATA] = VERFORMAT_HIGHSEAS;
	
	// check for High Seas multi format
	// we can't use multi.mul length because it varies and multi.idx is always 98184 bytes, the best option
	// so far seems to be to check the size of the first entry to see if its length is divisible by the new
	// format length (risky if the first entry is custom and happens to be be divisible by both lengths)
	CUOIndexRec index;
	if (ReadMulIndex( VERFILE_MULTIIDX, VERFILE_MULTI, 0x00, index) && (index.GetBlockLength() % sizeof(CUOMultiItemRec2)) == 0)
		m_FileFormat[VERFILE_MULTIIDX] = VERFORMAT_HIGHSEAS;
}

bool CGrayInstall::OpenFile( CGFile & file, LPCTSTR pszName, WORD wFlags )
{
	ADDTOCALLSTACK("CGrayInstall::OpenFile");
	ASSERT(pszName);
	if ( !m_sPreferPath.IsEmpty() )
	{
		if ( file.Open(GetPreferPath(pszName), wFlags) )
			return true;
	}
	else
	{
		if ( file.Open(GetFullExePath(pszName), wFlags) )
			return true;
		if ( file.Open(GetFullCDPath(pszName), wFlags) )
			return true;
	}
	return false;
}

LPCTSTR CGrayInstall::GetBaseFileName( VERFILE_TYPE i ) // static
{
	ADDTOCALLSTACK("CGrayInstall::GetBaseFileName");
	static LPCTSTR const sm_szFileNames[VERFILE_QTY] =
	{
		"artidx.mul",	// Index to ART
		"art.mul",		// Artwork such as ground, objects, etc.
		"anim.idx",
		"anim.mul",		// Animations such as monsters, people, and armor.
		"soundidx.mul", // Index into SOUND
		"sound.mul",	// Sampled sounds
		"texidx.mul",	// Index into TEXMAPS
		"texmaps.mul",	// Texture map data (the ground).
		"gumpidx.mul",	// Index to GUMPART
		"gumpart.mul",	// Gumps. Stationary controller bitmaps such as windows, buttons, paperdoll pieces, etc.
		"multi.idx",
		"multi.mul",	// Groups of art (houses, castles, etc)
		"skills.idx",
		"skills.mul",
		"verdata.mul",
		"map0.mul",		// MAP(s)
		"staidx0.mul",	// STAIDX(s)
		"statics0.mul",	// Static objects on MAP(s)
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
		"tiledata.mul", // Data about tiles in ART. name and flags, etc
		"animdata.mul", //
		"hues.mul"		// the 16 bit color pallete we use for everything.
	};

	return ( i<0 || i>=VERFILE_QTY ) ? NULL : sm_szFileNames[i];
}

bool CGrayInstall::OpenFile( VERFILE_TYPE i )
{
	ADDTOCALLSTACK("CGrayInstall::OpenFile");
	CGFile	*pFile = GetMulFile(i);
	if ( !pFile )
		return false;
	if ( pFile->IsFileOpen())
		return true;

	if ( !pFile->GetFilePath().IsEmpty() )
	{
		if ( pFile->Open(pFile->GetFilePath(), OF_READ|OF_SHARE_DENY_WRITE) )
			return true;
	}

	LPCTSTR pszTitle = GetBaseFileName((VERFILE_TYPE)i);
	if ( !pszTitle ) return false;

	return OpenFile(m_File[i], pszTitle, OF_READ|OF_SHARE_DENY_WRITE);
}

VERFILE_TYPE CGrayInstall::OpenFiles( DWORD dwMask )
{
	ADDTOCALLSTACK("CGrayInstall::OpenFiles");
	// Now open all the required files.
	// RETURN: VERFILE_QTY = all open success.
	int i(0);

	for ( i = 0; i < VERFILE_QTY; i++ )
	{
		if ( ! ( dwMask & ( 1 << i )) ) continue;
		if ( GetBaseFileName( (VERFILE_TYPE)i ) == NULL ) continue;

		bool bFileLoaded = true;
		switch (i)
		{
			default:
				if ( !OpenFile( (VERFILE_TYPE)i ))
				{
					//	make some MULs optional
					switch ( i )
					{
						case VERFILE_VERDATA:
							break;

						default:
							bFileLoaded = false;
							break;
					}
				}
				break;

			case VERFILE_MAP:
				// map file is handled differently
				TCHAR z[256];

				//	check for map files of custom maps
				for ( int m = 0; m < 256; m++ )
				{
					if ( g_MapList.m_maps[m] )
					{
						int	index = g_MapList.m_mapnum[m];
						if ( index == -1 )
						{
							g_MapList.m_maps[m] = false;
							continue;
						}

						if ( !m_Maps[index].IsFileOpen() )
						{
							sprintf(z, "map%d.mul", index);
							OpenFile(m_Maps[index], z, OF_READ|OF_SHARE_DENY_WRITE);

							if (m_Maps[index].IsFileOpen())
							{
								m_IsMapUopFormat[index] = false;
							}
							else
							{
								sprintf(z, "map%dLegacyMUL.uop", index);
								OpenFile(m_Maps[index], z, OF_READ|OF_SHARE_DENY_WRITE);

								// neither file exists, map0 is required
								if (m_Maps[index].IsFileOpen() == false && index == 0)
								{
									bFileLoaded = false;
									break;
								}

								m_IsMapUopFormat[index] = true;
							}
						}
						if ( !m_Staidx[index].IsFileOpen() )
						{
							sprintf(z, "staidx%d.mul", index);
							OpenFile(m_Staidx[index], z, OF_READ|OF_SHARE_DENY_WRITE);
						}
						if ( !m_Statics[index].IsFileOpen() )
						{
							sprintf(z, "statics%d.mul", index);
							OpenFile(m_Statics[index], z, OF_READ|OF_SHARE_DENY_WRITE);
						}
						if ( g_Cfg.m_fUseMapDiffs )
						{
							if ( !m_Mapdif[index].IsFileOpen() )
							{
								sprintf(z, "mapdif%d.mul", index);
								OpenFile(m_Mapdif[index], z, OF_READ|OF_SHARE_DENY_WRITE);
							}
							if ( !m_Mapdifl[index].IsFileOpen() )
							{
								sprintf(z, "mapdifl%d.mul", index);
								OpenFile(m_Mapdifl[index], z, OF_READ|OF_SHARE_DENY_WRITE);
							}
							if ( !m_Stadif[index].IsFileOpen() )
							{
								sprintf(z, "stadif%d.mul", index);
								OpenFile(m_Stadif[index], z, OF_READ|OF_SHARE_DENY_WRITE);
							}
							if ( !m_Stadifi[index].IsFileOpen() )
							{
								sprintf(z, "stadifi%d.mul", index);
								OpenFile(m_Stadifi[index], z, OF_READ|OF_SHARE_DENY_WRITE);
							}
							if ( !m_Stadifl[index].IsFileOpen() )
							{
								sprintf(z, "stadifl%d.mul", index);
								OpenFile(m_Stadifl[index], z, OF_READ|OF_SHARE_DENY_WRITE);
							}
						}

						//	if any of the map files are not available, mark map as unavailable
						//	mapdif and stadif files are not required.
						if ( !m_Maps[index].IsFileOpen() ||
							 !m_Staidx[index].IsFileOpen() ||
							 !m_Statics[index].IsFileOpen() )
						{
							if ( m_Maps[index].IsFileOpen() )		m_Maps[index].Close();
							if ( m_Staidx[index].IsFileOpen() )		m_Staidx[index].Close();
							if ( m_Statics[index].IsFileOpen() )	m_Statics[index].Close();
						
							if (index == 1 && m_Maps[0].IsFileOpen())
								g_MapList.m_mapnum[m] = 0;
							else
								g_MapList.m_mapid[m] = 0;
						}

						//	mapdif and mapdifl are not required, but if one exists so should
						//	the other
						if ( m_Mapdif[index].IsFileOpen() != m_Mapdifl[index].IsFileOpen() )
						{
							if ( m_Mapdif[index].IsFileOpen() )		m_Mapdif[index].Close();
							if ( m_Mapdifl[index].IsFileOpen() )	m_Mapdifl[index].Close();
						}

						//	if one of the stadif files exissts, so should the others
						if ( m_Stadif[index].IsFileOpen() != m_Stadifi[index].IsFileOpen() ||
							 m_Stadif[index].IsFileOpen() != m_Stadifl[index].IsFileOpen() )
						{
							if ( m_Stadif[index].IsFileOpen() )		m_Stadif[index].Close();
							if ( m_Stadifi[index].IsFileOpen() )	m_Stadifi[index].Close();
							if ( m_Stadifl[index].IsFileOpen() )	m_Stadifl[index].Close();
						}
					}
				}
				break;
		}

		// stop if we hit a failure
		if (bFileLoaded == false)
			break;
	}

	DetectMulVersions();
	g_MapList.Init();

	TCHAR * z = Str_GetTemp();
	TCHAR * z1 = Str_GetTemp();
	for ( unsigned char j = 0; j < 7; j++ )
	{
		bool bSup = false;

		if ( j == 1 )
			continue;

		if ( j == 5 ) // ML
			bSup = ( g_MapList.GetX(0) == 7168 );
		else if ( j > 5 ) // SA+
			bSup = ( g_MapList.m_maps[j - 1] );
		else
			bSup = ( g_MapList.m_maps[j] );

		if ( bSup )
		{
			switch ( j )
			{
				case 0: strcpy(z1, "T2A"); break;
				case 2: strcpy(z1, "LBR"); break;
				case 3: strcpy(z1, "AOS"); break;
				case 4: strcpy(z1, "SE"); break;
				case 5: strcpy(z1, "ML"); break;
				case 6: strcpy(z1, "SA"); break;
			}
			if ( *z )
				strcat(z, ", ");
			strcat(z, z1);
		}
	}
	g_Log.Event(LOGM_INIT, "Expansion maps supported: %s\n", z);

	return (VERFILE_TYPE)i;
}

void CGrayInstall::CloseFiles()
{
	ADDTOCALLSTACK("CGrayInstall::CloseFiles");
	int i;

	for ( i = 0; i < VERFILE_QTY; i++ )
	{
		if ( m_File[i].IsFileOpen() ) m_File[i].Close();
	}

	for ( i = 0; i < 256; i++ )
	{
		if ( m_Maps[i].IsFileOpen() )		m_Maps[i].Close();
		if ( m_Statics[i].IsFileOpen() )	m_Statics[i].Close();
		if ( m_Staidx[i].IsFileOpen() )		m_Staidx[i].Close();
		if ( m_Mapdif[i].IsFileOpen() )		m_Mapdif[i].Close();
		if ( m_Mapdifl[i].IsFileOpen() )	m_Mapdifl[i].Close();
		if ( m_Stadif[i].IsFileOpen() )		m_Stadif[i].Close();
		if ( m_Stadifi[i].IsFileOpen() )		m_Stadifi[i].Close();
		if ( m_Stadifl[i].IsFileOpen() )		m_Stadifl[i].Close();
	}
}

bool CGrayInstall::ReadMulIndex(CGFile &file, DWORD id, CUOIndexRec &Index)
{
	ADDTOCALLSTACK("CGrayInstall::ReadMulIndex");
	DWORD lOffset = id * sizeof(CUOIndexRec);

	if ( file.Seek(lOffset, SEEK_SET) != lOffset )
		return false;

	if ( file.Read((void *)&Index, sizeof(CUOIndexRec)) != sizeof(CUOIndexRec) )
		return false;

	return Index.HasData();
}

bool CGrayInstall::ReadMulData(CGFile &file, const CUOIndexRec &Index, void * pData)
{
	ADDTOCALLSTACK("CGrayInstall::ReadMulData");
	if ( file.Seek(Index.GetFileOffset(), SEEK_SET) != Index.GetFileOffset() )
		return false;

	DWORD dwLength = Index.GetBlockLength();
	if ( file.Read(pData, dwLength) != dwLength )
		return false;

	return true;
}

bool CGrayInstall::ReadMulIndex(VERFILE_TYPE fileindex, VERFILE_TYPE filedata, DWORD id, CUOIndexRec & Index)
{
	ADDTOCALLSTACK("CGrayInstall::ReadMulIndex");
	// Read about this data type in one of the index files.
	// RETURN: true = we are ok.
	ASSERT(fileindex<VERFILE_QTY);

	// Is there an index for it in the VerData ?
	if ( g_VerData.FindVerDataBlock(filedata, id, Index) )
		return true;

	return ReadMulIndex(m_File[fileindex], id, Index);
}

bool CGrayInstall::ReadMulData(VERFILE_TYPE filedata, const CUOIndexRec & Index, void * pData)
{
	ADDTOCALLSTACK("CGrayInstall::ReadMulData");
	// Use CGFile::GetLastError() for error.
	if ( Index.IsVerData() ) filedata = VERFILE_VERDATA;

	return ReadMulData(m_File[filedata], Index, pData);
}
