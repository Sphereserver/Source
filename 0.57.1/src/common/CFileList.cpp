//
// CFileList
//

#include "graycom.h"
#include "CFileList.h"

#if defined(_WIN32)
#include <io.h> 		// findfirst
static struct _finddata_t fileinfo;
#else	// LINUX
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
static struct dirent * fileinfo;
static DIR * dirp;
#endif

// Similar to the MFC CFileFind
bool CFileList::ReadFileInfo( LPCTSTR pszFilePath, time_t & dwDateChange, DWORD & dwSize ) // static
{
#if defined(_WIN32)
	fileinfo.attrib = _A_NORMAL;
	long lFind = _findfirst( pszFilePath, &fileinfo );
	if ( lFind == -1 )
#else
	// LINUX
	struct stat fileStat;
	if ( stat( pszFilePath, &fileStat) == -1 )
#endif
	{
		g_Log.Error("Can't open input dir [%s]\n", pszFilePath);
		return false;
	}
#if defined(_WIN32)
	dwDateChange = fileinfo.time_write;
	dwSize = fileinfo.size;
	_findclose( lFind );
#else
	// LINUX
	dwDateChange = fileStat.st_mtime;
	dwSize = fileStat.st_size;
#endif
	return true;
}

int CFileList::ReadDir( LPCTSTR pszFileDir )
{
	// NOTE: It seems NOT to like the trailing \ alone
	TCHAR szFileDir[_MAX_PATH];
	int len = strcpylen(szFileDir, pszFileDir);
#ifdef _WIN32
	if ( len )
	{
		len--;
		if ( szFileDir[len] == '\\' || szFileDir[len] == '/' )
			strcat( szFileDir, "*.*" );
	}
#endif

#if defined(_WIN32)
	long lFind = _findfirst(szFileDir, &fileinfo);
	if ( lFind == -1 )
#else
	char szFilename[_MAX_PATH];
	// Need to strip out the *.scp part
	for ( int i = len; i > 0; i-- )
	{
		if ( szFileDir[i] == '/' )
		{
			szFileDir[i+1] = 0x00;
			break;
		}
	}
	dirp = opendir(szFileDir);
	if ( !dirp )
#endif
	{
		g_Log.Error("Unable to open directory %s\n", szFileDir);
		return -1;
	}

	do
	{
#if defined(_WIN32)
		if ( fileinfo.name[0] == '.' )
			continue;
		AddHead(fileinfo.name);
#else
		fileinfo = readdir(dirp);
		if ( !fileinfo )
			break;
		if ( fileinfo->d_name[0] == '.' )
			continue;

		sprintf(szFilename, "%s%s", szFileDir, fileinfo->d_name);
		len = strlen(szFilename);
		if ( len > 4 && !strcmpi(&szFilename[len - 4], ".scp") )
		AddHead(fileinfo->d_name);
#endif
	}
#if defined(_WIN32)
	while ( !_findnext(lFind, &fileinfo));
	_findclose(lFind);
#else
	while ( fileinfo != NULL );
#endif
	return GetCount();
}

