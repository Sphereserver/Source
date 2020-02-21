#include "graycom.h"
#include "CString.h"
#ifndef _WIN32
	#include <errno.h>
#endif

bool CFile::SetFilePath(LPCTSTR pszPath)
{
	ADDTOCALLSTACK("CFile::SetFilePath");
	if ( !pszPath )
		return false;
	if ( !m_strFileName.CompareNoCase(pszPath) )
		return true;

	bool fIsOpen = (m_hFile != NOFILE_HANDLE);
	if ( fIsOpen )
		Close();

	m_strFileName = pszPath;
	if ( fIsOpen )
		return Open(NULL, OF_READ|OF_BINARY);	// GetMode()	// open it back up (in same mode as before)

	return true;
}

LPCTSTR CFile::GetFileTitle() const
{
	ADDTOCALLSTACK("CFile::GetFileTitle");
	return CGFile::GetFilesTitle(GetFilePath());
}

bool CFile::Open(LPCTSTR pszFileName, UINT uMode)
{
	ASSERT(m_hFile == NOFILE_HANDLE);
	SetFilePath(pszFileName);

#ifdef _WIN32
	DWORD dwDesiredAccess, dwShareMode, dwCreationDisposition;

	dwDesiredAccess = GENERIC_READ;
	if ( uMode & OF_WRITE )
		dwDesiredAccess |= GENERIC_WRITE;
	if ( uMode & OF_READWRITE )
		dwDesiredAccess |= (GENERIC_READ|GENERIC_WRITE);

	if ( uMode & OF_SHARE_COMPAT )
		dwShareMode = (FILE_SHARE_READ|FILE_SHARE_WRITE);
	else if ( uMode & OF_SHARE_EXCLUSIVE )
		dwShareMode = 0;
	else if ( uMode & OF_SHARE_DENY_WRITE )
		dwShareMode = FILE_SHARE_READ;
	else if ( uMode & OF_SHARE_DENY_READ )
		dwShareMode = FILE_SHARE_WRITE;
	else if ( uMode & OF_SHARE_DENY_NONE )
		dwShareMode = (FILE_SHARE_READ|FILE_SHARE_WRITE);
	else
		dwShareMode = 0;

	if ( uMode & OF_CREATE )
		dwCreationDisposition = (OPEN_ALWAYS|CREATE_NEW);
	else
		dwCreationDisposition = OPEN_EXISTING;

	m_hFile = CreateFile(GetFilePath(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
#else
	m_hFile = open(GetFilePath(), uMode);
#endif
	return (m_hFile != NOFILE_HANDLE);
}

void CFile::Close()
{
	if ( m_hFile != NOFILE_HANDLE )
	{
#ifdef _WIN32
		CloseHandle(m_hFile);
#else
		close(m_hFile);
#endif
		m_hFile = NOFILE_HANDLE;
	}
}

DWORD CFile::GetLength()
{
	// Get the size of the file
	DWORD dwPos = GetPosition();	// save current pos
	DWORD dwSize = Seek(0, FILE_END);
	Seek(dwPos, FILE_BEGIN);		// restore previous pos
	return dwSize;
}

DWORD CFile::GetPosition() const
{
#ifdef _WIN32
	return SetFilePointer(m_hFile, 0, NULL, FILE_CURRENT);
#else
	return lseek(m_hFile, 0, FILE_CURRENT);
#endif
}

DWORD CFile::Seek(LONG lOffset, DWORD dwMoveMethod)
{
#ifdef _WIN32
	return SetFilePointer(m_hFile, lOffset, NULL, dwMoveMethod);
#else
	if ( m_hFile <= 0 )
		return -1;

	return lseek(m_hFile, static_cast<off_t>(lOffset), dwMoveMethod);
#endif
}

DWORD CFile::Read(void *pBuffer, size_t iLength) const
{
#ifdef _WIN32
	DWORD dwRead;
	if ( !ReadFile(m_hFile, pBuffer, iLength, &dwRead, NULL) )
	{
		NotifyIOError("read");
		return 0;
	}
	return dwRead;
#else
	ssize_t iRead = read(m_hFile, pBuffer, iLength);
	if ( iRead == -1 )
	{
		NotifyIOError("read");
		return 0;
	}
	return static_cast<DWORD>(iRead);
#endif
}

bool CFile::Write(const void *pBuffer, size_t iLength) const
{
#ifdef _WIN32
	DWORD dwWritten;
	if ( !WriteFile(m_hFile, pBuffer, iLength, &dwWritten, NULL) )
	{
		NotifyIOError("write");
		return false;
	}
	return true;
#else
	ssize_t iWritten = write(m_hFile, pBuffer, iLength);
	if ( iWritten == -1 )
	{
		NotifyIOError("write");
		return false;
	}
	return (iWritten == static_cast<ssize_t>(iLength));
#endif
}

void CFile::NotifyIOError(LPCTSTR pszMessage) const
{
#ifdef _WIN32
	LPSTR pszBuffer;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM, NULL, CGFile::GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&pszBuffer, 0, NULL);
#else
	LPSTR pszBuffer = strerror(CGFile::GetLastError());
#endif
	DEBUG_ERR(("File I/O '%s' failed on file '%s' (error %" FMTDWORD "): %s\n", pszMessage, static_cast<LPCTSTR>(GetFilePath()), CGFile::GetLastError(), pszBuffer));
#ifdef _WIN32
	LocalFree(pszBuffer);
#endif
}

///////////////////////////////////////////////////////////
// CGFile

DWORD CGFile::GetLastError()	// static
{
#ifdef _WIN32
	return ::GetLastError();
#else
	return errno;
#endif
}

CGString CGFile::GetMergedFileName(LPCTSTR pszPath, LPCTSTR pszFileName) // static
{
	ADDTOCALLSTACK("CGFile::GetMergedFileName");
	// Merge path and file name

	TCHAR szFullPath[_MAX_PATH];
	if ( pszPath && pszPath[0] )
	{
		strncpy(szFullPath, pszPath, sizeof(szFullPath) - 1);
		szFullPath[sizeof(szFullPath) - 1] = '\0';

		size_t iLen = strlen(szFullPath);
		if ( iLen && (szFullPath[iLen - 1] != '\\') && (szFullPath[iLen - 1] != '/') )
		{
#ifdef _WIN32
			strcat(szFullPath, "\\");
#else
			strcat(szFullPath, "/");
#endif
		}
	}
	else
		szFullPath[0] = '\0';

	if ( pszFileName )
		strncat(szFullPath, pszFileName, sizeof(szFullPath) - 1);

	return CGString(szFullPath);
}

LPCTSTR CGFile::GetFilesTitle(LPCTSTR pszPath)	// static
{
	ADDTOCALLSTACK("CGFile::GetFilesTitle");
	// Just use COMMDLG.H GetFileTitle(LPCTSTR, LPTSTR, WORD) instead ?
	// strrchr
	size_t iLen = strlen(pszPath);
	while ( iLen > 0 )
	{
		iLen--;
		if ( (pszPath[iLen] == '\\') || (pszPath[iLen] == '/') )
		{
			iLen++;
			break;
		}
	}
	return pszPath + iLen;
}

LPCTSTR CGFile::GetFilesExt(LPCTSTR pszPath)	// static
{
	ADDTOCALLSTACK("CGFile::GetFilesExt");
	// get the EXTension including the .
	size_t iLen = strlen(pszPath);
	while ( iLen > 0 )
	{
		iLen--;
		if ( (pszPath[iLen] == '\\') || (pszPath[iLen] == '/') )
			break;
		if ( pszPath[iLen] == '.' )
			return pszPath + iLen;
	}
	return NULL;
}

LPCTSTR CGFile::GetFileExt() const
{
	ADDTOCALLSTACK("CGFile::GetFileExt");
	// get the EXTension including the .
	return GetFilesExt(GetFilePath());
}


bool CGFile::OpenBase(void *pExtra)
{
	ADDTOCALLSTACK("CGFile::OpenBase");
	UNREFERENCED_PARAMETER(pExtra);
	return CFile::Open(GetFilePath(), GetMode());
}

void CGFile::CloseBase()
{
	ADDTOCALLSTACK("CGFile::CloseBase");
	CFile::Close();
}

bool CGFile::Open(LPCTSTR pszFileName, UINT uMode, void *pExtra)
{
	ADDTOCALLSTACK("CGFile::Open");
	// RETURN:
	//  true = success
	// OF_BINARY|OF_WRITE
	if ( !pszFileName )
	{
		if ( IsFileOpen() )
			return true;
	}
	else
		Close();	// make sure it's closed first

	if ( pszFileName )
		m_strFileName = pszFileName;

	if ( m_strFileName.IsEmpty() )
		return false;

	m_uMode = uMode;
	return OpenBase(pExtra);
}

void CGFile::Close()
{
	ADDTOCALLSTACK("CGFile::Close");
	if ( !IsFileOpen() )
		return;

	CloseBase();
	m_hFile = NOFILE_HANDLE;
}

///////////////////////////////////////////////////////////
// CFileText

LPCTSTR CFileText::GetModeStr() const
{
	ADDTOCALLSTACK("CFileText::GetModeStr");
	// end of line translation is crap. ftell and fseek don't work correctly when you use it.
	// fopen() args
	if ( IsBinaryMode() )
		return IsWriteMode() ? "wb" : "rb";
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
	if ( IsWriteMode() )
		fflush(m_pStream);

	fclose(m_pStream);
	m_pStream = NULL;
}

bool CFileText::OpenBase(void *pExtra)
{
	ADDTOCALLSTACK("CFileText::OpenBase");
	UNREFERENCED_PARAMETER(pExtra);

	// Open a file
	m_pStream = fopen(GetFilePath(), GetModeStr());
	if ( !m_pStream )
		return false;

	// Get the low level handle for it
	m_hFile = reinterpret_cast<HANDLE>(STDFUNC_FILENO(m_pStream));
	return true;
}

DWORD CFileText::Seek(LONG lOffset, DWORD dwMoveMethod)
{
	// RETURN:
	//  true = success
	if ( !IsFileOpen() )
		return 0;
	if ( lOffset < 0 )
		return 0;
	if ( fseek(m_pStream, lOffset, dwMoveMethod) != 0 )
		return 0;

	long lPos = ftell(m_pStream);
	if ( lPos < 0 )
		return 0;
	return lPos;
}

void CFileText::Flush() const
{
	if ( !IsFileOpen() )
		return;

	ASSERT(m_pStream);
	fflush(m_pStream);
}

DWORD CFileText::GetPosition() const
{
	// RETURN:
	//  -1 = error
	if ( !IsFileOpen() )
		return -1;

	return ftell(m_pStream);
}

DWORD CFileText::Read(void *pBuffer, size_t iLength) const
{
	// This can return: EOF(-1) constant.
	// returns the number of full items actually read
	ASSERT(pBuffer);
	if ( IsEOF() )
		return 0;	// LINUX will ASSERT if we read past end

	return fread(pBuffer, 1, iLength, m_pStream);
}

TCHAR *CFileText::ReadString(TCHAR *pszBuffer, size_t iLength) const
{
	// Read a line of text. NULL = EOF
	ASSERT(pszBuffer);
	if ( IsEOF() )
		return NULL;	// LINUX will ASSERT if we read past end

	return fgets(pszBuffer, static_cast<int>(iLength), m_pStream);
}

#ifndef _WIN32
bool CFileText::Write(const void *pBuffer, size_t iLength) const
#else
bool CFileText::Write(const void *pBuffer, size_t iLength)
#endif
{
	// RETURN:
	//  true = success
	ASSERT(pBuffer);
	if ( !IsFileOpen() )
		return false;

#ifdef _WIN32	// Windows flushing, the only safe mode to cancel it ;)
	if ( !m_fNoBuffer )
	{
		setvbuf(m_pStream, NULL, _IONBF, 0);
		m_fNoBuffer = true;
	}
#endif
	size_t iStatus = fwrite(pBuffer, iLength, 1, m_pStream);
#ifndef _WIN32	// However, in unix, it works
	fflush(m_pStream);
#endif
	return (iStatus == 1);
}

bool CFileText::WriteString(LPCTSTR pszArgs)
{
	ASSERT(pszArgs);
	return Write(pszArgs, strlen(pszArgs));
}

bool CFileText::IsEOF() const
{
	if ( !IsFileOpen() )
		return true;

	return static_cast<bool>(feof(m_pStream));
}

void CFileText::VPrintf(LPCTSTR pszFormat, va_list args)
{
	ASSERT(pszFormat);
	if ( IsFileOpen() )
		vfprintf(m_pStream, pszFormat, args);
}

void _cdecl CFileText::Printf(LPCTSTR pszFormat, ...)
{
	ASSERT(pszFormat);
	va_list vargs;
	va_start(vargs, pszFormat);
	VPrintf(pszFormat, vargs);
	va_end(vargs);
}
