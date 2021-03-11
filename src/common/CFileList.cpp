#include "graycom.h"
#include "CFileList.h"
#ifndef _WIN32
	#include <dirent.h>
#endif

bool CFileList::ReadFileInfo(LPCTSTR pszPath, time_t &timeDateChange, DWORD &dwSize)	// static
{
	ADDTOCALLSTACK("CFileList::ReadFileInfo");

#ifdef _WIN32
	struct _finddata_t fileinfo;
	fileinfo.attrib = _A_NORMAL;

	intptr_t hFile = _findfirst(pszPath, &fileinfo);
	if ( hFile != -1 )
	{
		timeDateChange = fileinfo.time_write;
		dwSize = fileinfo.size;
		_findclose(hFile);
		return true;
	}
#else
	struct stat fileStat;
	if ( stat(pszPath, &fileStat) != -1 )
	{
		timeDateChange = fileStat.st_mtime;
		dwSize = fileStat.st_size;
		return true;
	}
#endif

	return false;
}

int CFileList::ReadDir(LPCTSTR pszPath)
{
	ADDTOCALLSTACK("CFileList::ReadDir");
	// NOTE: It seems NOT to like the trailing \ alone

	TCHAR szPath[_MAX_PATH];
	size_t len = strcpylen(szPath, pszPath, sizeof(szPath));

#ifdef _WIN32
	if ( len > 0 )
	{
		--len;
		if ( (szPath[len] == '\\') || (szPath[len] == '/') )
			strncat(szPath, "*.*", sizeof(szPath) - strlen(szPath) - 1);
	}

	struct _finddata_t fileinfo;
	intptr_t hFile = _findfirst(szPath, &fileinfo);
	if ( hFile != -1 )
	{
		do
		{
			if ( fileinfo.name[0] == '.' )
				continue;
			AddTail(fileinfo.name);
		}
		while ( !_findnext(hFile, &fileinfo) );

		_findclose(hFile);
		return static_cast<int>(GetCount());
	}
#else
	// Strip out the *.scp part
	for ( size_t i = len; i > 0; --i )
	{
		if ( szPath[i] == '/' )
		{
			szPath[i++] = '\0';
			break;
		}
	}

	DIR *dirp = opendir(szPath);
	if ( dirp )
	{
		struct dirent *fileinfo = NULL;
		char szFileName[_MAX_PATH];

		do
		{
			fileinfo = readdir(dirp);
			if ( !fileinfo )
				break;
			if ( fileinfo->d_name[0] == '.' )
				continue;

			len = snprintf(szFileName, sizeof(szFileName), "%s%s", szPath, fileinfo->d_name);
			if ( (len > 4) && !strcmpi(&szFileName[len - 4], SPHERE_FILE_EXT_SCP) )
				AddTail(fileinfo->d_name);
		}
		while ( fileinfo != NULL );

		closedir(dirp);
		return static_cast<int>(GetCount());
	}
#endif

	return -1;
}
