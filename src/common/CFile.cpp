#include "graycom.h"

#ifndef _WIN32
#include <errno.h>	// errno
#endif


bool CFile::SetFilePath( LPCTSTR pszName )
{
	ADDTOCALLSTACK("CFile::SetFilePath");
	if ( pszName == NULL )
		return false;
	if ( ! m_strFileName.CompareNoCase( pszName ))
		return( true );
	bool fIsOpen = ( m_hFile != NOFILE_HANDLE );
	if ( fIsOpen )
	{
		Close();
	}
	m_strFileName = pszName;
	if ( fIsOpen )
	{
		return( Open( NULL, OF_READ|OF_BINARY )); // GetMode()	// open it back up. (in same mode as before)
	}
	return( true );
}

LPCTSTR CFile::GetFileTitle() const
{
	ADDTOCALLSTACK("CFile::GetFileTitle");
	return( CGFile::GetFilesTitle( GetFilePath()));
}

bool CFile::Open( LPCTSTR pszName, UINT uMode, CFileException * e )
{
	UNREFERENCED_PARAMETER(e);
	ASSERT( m_hFile == NOFILE_HANDLE );
	SetFilePath( pszName );

#ifdef _WIN32
	DWORD dwShareMode, dwCreationDisposition, dwDesiredAccess;

	dwDesiredAccess = GENERIC_READ;
	if ( uMode & OF_WRITE )
		dwDesiredAccess |= GENERIC_WRITE;
	if ( uMode & OF_READWRITE )
		dwDesiredAccess |= (GENERIC_READ | GENERIC_WRITE);

	if ( uMode & OF_SHARE_COMPAT )
		dwShareMode = (FILE_SHARE_READ | FILE_SHARE_WRITE);
	else if ( uMode & OF_SHARE_EXCLUSIVE )
		dwShareMode = 0;
	else if ( uMode & OF_SHARE_DENY_WRITE )
		dwShareMode = FILE_SHARE_READ;
	else if ( uMode & OF_SHARE_DENY_READ )
		dwShareMode = FILE_SHARE_WRITE;
	else if ( uMode & OF_SHARE_DENY_NONE )
		dwShareMode = (FILE_SHARE_READ | FILE_SHARE_WRITE);
	else
		dwShareMode = 0;

	if ( uMode & OF_CREATE )
		dwCreationDisposition = (OPEN_ALWAYS|CREATE_NEW);
	else
		dwCreationDisposition = OPEN_EXISTING;

	m_hFile = CreateFile( GetFilePath(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL );
#else // _WIN32
	m_hFile = open( GetFilePath(), uMode );
#endif // _WIN32
	return( m_hFile != NOFILE_HANDLE );
}

void CFile::Close()
{
	if ( m_hFile != NOFILE_HANDLE )
	{
#ifdef _WIN32
		CloseHandle( m_hFile );
#else
		close( m_hFile );
#endif
		m_hFile = NOFILE_HANDLE;
	}
}

DWORD CFile::GetLength()
{
	// Get the size of the file.
	DWORD lPos = GetPosition();	// save current pos.
	DWORD lSize = SeekToEnd();
	Seek( lPos, SEEK_SET );	// restore previous pos.
	return( lSize );
}

DWORD CFile::GetPosition() const
{
#ifdef _WIN32
	return SetFilePointer( m_hFile, 0, NULL, FILE_CURRENT );
#else
	return( lseek( m_hFile, 0, SEEK_CUR ) );
#endif
}

DWORD CFile::Seek( LONG lOffset, UINT iOrigin )
{
#ifdef _WIN32
	return SetFilePointer( m_hFile, lOffset, NULL, iOrigin );
#else
	if ( m_hFile <= 0 )
		return( -1 );
	return( lseek( m_hFile, lOffset, iOrigin ));
#endif
}

DWORD CFile::Read( void * pData, DWORD dwLength ) const
{
#ifdef _WIN32
	DWORD dwRead;
	if ( ! ReadFile( m_hFile, pData, dwLength, &dwRead, NULL ))
	{
		NotifyIOError("read");
		return 0;
	}
	return dwRead;
#else
	return( read( m_hFile, pData, (long) dwLength ));
#endif
}

bool CFile::Write( const void * pData, DWORD dwLength ) const
{
#ifdef _WIN32
	DWORD dwWritten;
	BOOL ret = ::WriteFile( m_hFile, pData, dwLength, &dwWritten, NULL );
	if ( ret == FALSE )
	{
		NotifyIOError("write");
		return false;
	}
	return true;
#else
	return( write( m_hFile, (const char *) pData, (long) dwLength ) == (long) dwLength );
#endif
}

#ifdef _WIN32
void CFile::NotifyIOError( LPCTSTR szMessage )
{
	LPVOID lpMsgBuf;
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), 0, reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, NULL );
	DEBUG_ERR(( "File I/O \"%s\" failed on file \"%s\" (%ld): %s\n", szMessage, static_cast<LPCTSTR>(GetFilePath()), GetLastError(), static_cast<LPTSTR>(lpMsgBuf) ));

	LocalFree( lpMsgBuf );
}
#endif


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
	ADDTOCALLSTACK("CGFile::GetMergedFileName");
	// Merge path and file name.

	TCHAR szFilePath[ _MAX_PATH ];
	if ( pszBase && pszBase[0] )
	{
		strcpy( szFilePath, pszBase );
		int len = static_cast<int>(strlen( szFilePath ));
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
	return static_cast<CGString>(szFilePath);
}

LPCTSTR CGFile::GetFilesTitle( LPCTSTR pszPath )	// static
{
	ADDTOCALLSTACK("CGFile::GetFilesTitle");
	// Just use COMMDLG.H GetFileTitle(LPCTSTR, LPTSTR, WORD) instead ?
	// strrchr
	size_t len = strlen(pszPath);
	while ( len > 0 )
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
	ADDTOCALLSTACK("CGFile::GetFilesExt");
	// get the EXTension including the .
	size_t lenall = strlen( pszName );
	size_t len = lenall;
	while ( len > 0 )
	{
		len--;
		if ( pszName[len] == '\\' || pszName[len] == '/' )
			break;
		if ( pszName[len] == '.' )
		{
			return( pszName + len );
		}
	}
	return( NULL );	// has no ext.
}

LPCTSTR CGFile::GetFileExt() const
{
	ADDTOCALLSTACK("CGFile::GetFileExt");
	// get the EXTension including the .
	return( GetFilesExt( GetFilePath()));
}


bool CGFile::OpenBase( void * pExtra )
{
	ADDTOCALLSTACK("CGFile::OpenBase");
	UNREFERENCED_PARAMETER(pExtra);

	return static_cast<CFile *>(this)->Open(GetFilePath(), GetMode());
}

void CGFile::CloseBase()
{
	ADDTOCALLSTACK("CGFile::CloseBase");
	CFile::Close();
}

bool CGFile::Open( LPCTSTR pszFilename, UINT uModeFlags, void FAR * pExtra )
{
	ADDTOCALLSTACK("CGFile::Open");
	// RETURN: true = success.
	// OF_BINARY | OF_WRITE
	if ( pszFilename == NULL )
	{
		if ( IsFileOpen())
			return( true );
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
		return( false );

	m_uMode = uModeFlags;
	if ( ! OpenBase( pExtra ))
		return( false );

	return( true );
}

void CGFile::Close()
{
	ADDTOCALLSTACK("CGFile::Close");
	if ( ! IsFileOpen())
		return;

	CloseBase();
	m_hFile = NOFILE_HANDLE;
}

//***************************************************************************
// -CFileText

LPCTSTR CFileText::GetModeStr() const
{
	ADDTOCALLSTACK("CFileText::GetModeStr");
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
	ADDTOCALLSTACK("CFileText::CloseBase");
	if ( IsWriteMode())
	{
		fflush(m_pStream);
	}
	
	fclose(m_pStream);
	m_pStream = NULL;
}

bool CFileText::OpenBase( void FAR * pszExtra )
{
	ADDTOCALLSTACK("CFileText::OpenBase");
	UNREFERENCED_PARAMETER(pszExtra);

	// Open a file.
	m_pStream = fopen( GetFilePath(), GetModeStr());
	if ( m_pStream == NULL )
	{
		return( false );
	}
	// Get the low level handle for it.
	m_hFile = (OSFILE_TYPE)STDFUNC_FILENO(m_pStream);

	return( true );
}

DWORD CFileText::Seek( LONG offset, UINT origin )
{
	// true = success
	if ( ! IsFileOpen())
		return 0;
	if ( offset < 0 )
		return 0;
	if ( fseek( m_pStream, offset, origin ) != 0 )
		return 0;
	long position = ftell(m_pStream);
	if (position < 0)
		return 0;
	return position;
}

void CFileText::Flush() const
{
	if ( ! IsFileOpen())
		return;
	ASSERT(m_pStream);
	fflush(m_pStream);
}

DWORD CFileText::GetPosition() const
{
	// RETURN: -1 = error.
	if ( ! IsFileOpen())
		return static_cast<DWORD>(-1);
	return( ftell(m_pStream));
}

DWORD CFileText::Read( void * pBuffer, size_t sizemax ) const
{
	// This can return: EOF(-1) constant.
	// returns the number of full items actually read
	ASSERT(pBuffer);
	if ( IsEOF())
		return( 0 );	// LINUX will ASSERT if we read past end.
	return( static_cast<DWORD>(fread( pBuffer, 1, sizemax, m_pStream )));
}

TCHAR * CFileText::ReadString( TCHAR * pBuffer, size_t sizemax ) const
{
	// Read a line of text. NULL = EOF
	ASSERT(pBuffer);
	if ( IsEOF())
		return( NULL );	// LINUX will ASSERT if we read past end.
	return( fgets( pBuffer, static_cast<int>(sizemax), m_pStream ));
}

#ifndef _WIN32
	bool CFileText::Write( const void * pData, DWORD iLen ) const
#else
	bool CFileText::Write( const void * pData, DWORD iLen )
#endif
{
	// RETURN: 1 = success else fail.
	ASSERT(pData);
	if ( ! IsFileOpen())
		return( false );
#ifdef _WIN32 //	Windows flushing, the only safe mode to cancel it ;)
	if ( !bNoBuffer )
	{
		setvbuf(m_pStream, NULL, _IONBF, 0);
		bNoBuffer = true;
	}
#endif
	size_t iStatus = fwrite( pData, iLen, 1, m_pStream );
#ifndef _WIN32	// However, in unix, it works
	fflush( m_pStream );
#endif
	return ( iStatus == 1 );
}

bool CFileText::WriteString( LPCTSTR pStr )
{
	// RETURN: < 0 = failed.
	ASSERT(pStr);
	return( Write( pStr, static_cast<DWORD>(strlen( pStr ))));
}

bool CFileText::IsEOF() const
{
	if ( ! IsFileOpen())
		return( true );
	return(( feof( m_pStream )) ? true : false );
}

size_t CFileText::VPrintf( LPCTSTR pFormat, va_list args )
{
	ASSERT(pFormat);
	if ( ! IsFileOpen())
		return static_cast<size_t>(-1);

	size_t lenret = vfprintf( m_pStream, pFormat, args );
	return( lenret );
}

size_t _cdecl CFileText::Printf( LPCTSTR pFormat, ... )
{
	ASSERT(pFormat);
	va_list vargs;
	va_start( vargs, pFormat );
	size_t iRet = VPrintf( pFormat, vargs );
	va_end( vargs );
	return( iRet );
}
