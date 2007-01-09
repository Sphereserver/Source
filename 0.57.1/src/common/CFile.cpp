#include "graycom.h"

#ifndef _WIN32
	#include <errno.h>	// errno
	extern int errno;
#endif

bool CFile::SetFilePath( LPCTSTR pszName )
{
	if ( pszName == NULL )
		return false;
	if ( ! m_strFileName.CompareNoCase( pszName ))
		return true;
	bool fIsOpen = ( m_hFile != HFILE_ERROR );
	if ( fIsOpen )
	{
		Close();
	}
	m_strFileName = pszName;
	if ( fIsOpen )
	{
		return( Open( NULL, OF_READ|OF_BINARY ));
	}
	return true;
}

LPCTSTR CFile::GetFileTitle() const
{
	return( CGFile::GetFilesTitle( GetFilePath()));
}

//***************************************************************************
// -CGFile

int CGFile::GetLastError()	// static
{
#ifdef _WIN32
	return ::GetLastError();
#else
	return errno;
#endif
}

CGString CGFile::GetMergedFileName( LPCTSTR pszBase, LPCTSTR pszName ) // static
{
	// Merge path and file name.

	TCHAR szFilePath[ _MAX_PATH ];
	if ( pszBase && pszBase[0] )
	{
		strcpy( szFilePath, pszBase );
		int len = strlen( szFilePath );
		if ( len && szFilePath[ len-1 ] != '\\' &&
			szFilePath[ len-1 ] != '/' )	// Might be LINUX
		{
			strcat( szFilePath, "\\" );
		}
	}
	else
	{
		szFilePath[0] = '\0';
	}
	if ( pszName )
	{
		strcat( szFilePath, pszName );
	}
	return( (CGString) szFilePath );
}

LPCTSTR CGFile::GetFilesTitle( LPCTSTR pszPath )	// static
{
	int len = strlen(pszPath);
	while ( len>0 )
	{
		len--;
		if ( pszPath[len] == '\\' || pszPath[len] == '/' )
		{
			len++;
			break;
		}
	}
	return( pszPath + len );
}

LPCTSTR CGFile::GetFilesExt( LPCTSTR pszName )	// static
{
	// get the EXTension including the .
	int lenall = strlen( pszName );
	int len = lenall;
	while ( len>0 )
	{
		len--;
		if ( pszName[len] == '\\' || pszName[len] == '/' )
			break;
		if ( pszName[len] == '.' )
		{
			return( pszName + len );
		}
	}
	return NULL;	// has no ext.
}

LPCTSTR CGFile::GetFileExt() const
{
	// get the EXTension including the .
	return( GetFilesExt( GetFilePath()));
}

bool CGFile::OpenBase( void * pExtra )
{
	UNREFERENCED_PARAMETER(pExtra);
#ifdef _WIN32
	OFSTRUCT ofs;
	m_hFile = ::OpenFile( GetFilePath(), &ofs, GetMode());
#else
	m_hFile = open( GetFilePath(), GetMode());
#endif
	return( IsFileOpen());
}

void CGFile::CloseBase()
{
	CFile::Close();
}

bool CGFile::Open( LPCTSTR pszFilename, UINT uModeFlags, void * pExtra )
{
	// RETURN: true = success.
	// OF_BINARY | OF_WRITE
	if ( pszFilename == NULL )
	{
		if ( IsFileOpen())
			return true;
	}
	else
	{
		Close();	// Make sure it's closed first.
	}

	if ( pszFilename == NULL )
		pszFilename = GetFilePath();
	else
		m_strFileName = pszFilename;

	if ( m_strFileName.IsEmpty())
		return false;

	m_uMode = uModeFlags;
	return OpenBase(pExtra);
}

void CGFile::Close()
{
	if ( !IsFileOpen() ) return;

	CloseBase();
	m_hFile = HFILE_ERROR;
}

//***************************************************************************
// -CFileText

LPCTSTR CFileText::GetModeStr() const
{
	// end of line translation is crap. ftell and fseek don't work correctly when you use it.
	// fopen() args
	if ( IsBinaryMode())
		return ( IsWriteMode()) ? "wb" : "rb";
	if ( GetMode() & OF_READWRITE )
		return "a+b";
	if ( GetMode() & OF_CREATE )
		return "w";
	if ( IsWriteMode() )
		return "w";
	else
		return "rb";	// don't parse out the \n\r
}

void CFileText::CloseBase()
{
	if ( IsWriteMode())
	{
		fflush(m_pStream);
	}
	bool fSuccess = ( fclose( m_pStream ) == 0 );
	m_pStream = NULL;
}

bool CFileText::OpenBase( void * pszExtra )
{
	// Open a file.
	m_pStream = fopen( GetFilePath(), GetModeStr());
	if ( m_pStream == NULL )
	{
		return false;
	}
	// Get the low level handle for it.
	m_hFile = (HFILE)_fileno(m_pStream);
	return true;
}
