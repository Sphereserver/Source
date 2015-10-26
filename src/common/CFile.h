/**
* @file CFile.h
*/

#pragma once
#ifndef _INC_CFILE_H
#define _INC_CFILE_H

#ifndef _WIN32
#ifdef LONG
#undef LONG
#endif
#define LONG int
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "CString.h"

#ifndef OF_WRITE
#define OF_READ             O_RDONLY
#define OF_WRITE            O_WRONLY
#define OF_READWRITE        O_RDWR
#define OF_SHARE_DENY_NONE	0x00
#define OF_SHARE_DENY_WRITE	0x00	// not defined in LINUX
#define OF_CREATE			O_CREAT
#endif

#define OF_NONCRIT			0x40000000	// just a test.
#define OF_TEXT				0x20000000
#define OF_BINARY			0x10000000
#define OF_DEFAULTMODE		0x80000000

#ifndef HFILE_ERROR
#define HFILE_ERROR	-1
#define HFILE int
#endif // HFILE_ERROR

#ifdef _WIN32
#define INVALID_HANDLE ((HANDLE) -1)
#endif

#ifdef _WIN32
#define OSFILE_TYPE		HANDLE
#define NOFILE_HANDLE	INVALID_HANDLE
#else
#define OSFILE_TYPE		HFILE
#define NOFILE_HANDLE	HFILE_ERROR
#endif

class CGrayError;
#define CFileException CGrayError

/**
* @brief Class that dupes the MFC functionality we need
*/
class CFile
{
public:
	static const char *m_sClassName;
	OSFILE_TYPE m_hFile;	///< File type.

protected:
	CGString m_strFileName;	///< File name (with path).

public:
	/**
	* @brief Get file name and path (for compatibility with MFC)
	* @return file name and path.
	*/
	const CGString & GetFilePath() const { return( m_strFileName); }
	/**
	* @brief Sets a new file path.
	*
	* If CFile has already have a file path, close it if is opened and open new 
	* file.
	* @param pszName new file path.
	* @return true if new file name is setted, false otherwise.
	*/
	virtual bool SetFilePath( LPCTSTR pszName );
	/**
	* @brief Gets the basename of the file.
	* @return the basename of the file (name withouth paths).
	*/
	LPCTSTR GetFileTitle() const;
	/**
	* @brief Open a file in a specified mode.
	* @param pszName file to open.
	* @param uMode open mode.
	* @param e TODOC.
	* @return true if file is open, false otherwise.
	*/
	virtual bool Open( LPCTSTR pszName = NULL, UINT uMode = OF_READ | OF_SHARE_DENY_NONE, CFileException * e = NULL );
	/**
	* @brief Closes the file if is open.
	*/
	virtual void Close();
	/**
	* @brief Sets the position indicator at the begin of the file.
	*/
	void SeekToBegin() { Seek( 0, SEEK_SET ); }
	/**
	* @brief Sets the position indicator at the end of the file.
	* @return The length of the file on success, -1 on error.
	*/
	DWORD SeekToEnd() { return( Seek( 0, SEEK_END )); }
	/**
	* @brief Get the length of the file.
	* @return the length of the file.
	*/
	DWORD GetLength();
	/**
	* @brief Gets the position indicator of the file.
	* @return The position indicator of the file.
	*/
	virtual DWORD GetPosition() const;
	/**
	* @brief Set the position indicator.
	* @param lOffset position to set.
	* @param iOrigin origin (current position or init of the file).
	* @return position where the position indicator is set on success, -1 on error.
	*/
	virtual DWORD Seek( LONG lOffset = 0, UINT iOrigin = SEEK_SET );
	/**
	* @brief Reads data from the file.
	* @param pData buffer where store the readed data.
	* @param dwLength count of bytes to read.
	* @return count of bytes readed.
	*/
	virtual DWORD Read( void * pData, DWORD dwLength ) const;
	/**
	* @brief writes supplied data into file.
	* @param pData data to write.
	* @param dwLength lenght of the data to write.
	* @return true is success, false otherwise.
	*/
	virtual bool Write( const void * pData, DWORD dwLength ) const;
#ifdef _WIN32
	/**
	* @brief Notify a file input / output error (win32 only).
	* @param szMessage error to notify.
	*/
	void NotifyIOError( LPCTSTR szMessage ) const;
#endif

public:
	CFile() { m_hFile = NOFILE_HANDLE; }
	virtual ~CFile() { Close(); }
private:
	/**
	* @brief No copy on construction allowed.
	*/
	CFile(const CFile& copy);
	/**
	* @brief No copy allowed.
	*/
	CFile& operator=(const CFile& other);
};


/**
* Try to be compatible with MFC CFile class.
*/
class CGFile : public CFile
{
private:
	UINT m_uMode;	///< MMSYSTEM may use 32 bit flags.
public:
	static const char *m_sClassName;
private:
	/**
	* @brief Open file with CFile method.
	* @param pExtra unused.
	* @return true if file is open, false otherwise.
	*/
	virtual bool OpenBase( void * pExtra );
	/**	
	* @brief Closes the file if is open.
	*/
	virtual void CloseBase();
public:
	/**
	* @brief Gets the basename of the file.
	* @param pszPath file path.
	* @return the basename of the file (name withouth paths).
	*/
	static LPCTSTR GetFilesTitle( LPCTSTR pszPath );
	/**
	* @brief Gets the file extension, if any.
	* @param pszName file path where get the extension.
	* @return The extension of the file or NULL if the file has no extension.
	*/
	static LPCTSTR GetFilesExt( LPCTSTR pszName );
	/**
	* @brief Merges path and filename, adding slashes if needed.
	* @param pszBase path.
	* @param pszName filename.
	* @return merged path.
	*/
	static CGString GetMergedFileName( LPCTSTR pszBase, LPCTSTR pszName );
	/**
	* @brief Return the last IO error code.
	* @return IO error code.
	*/
	static int GetLastError();
	/**
	* @brief Gets the file extension, if any.
	* @return The extension of the file or NULL if the file has no extension.
	*/
	LPCTSTR GetFileExt() const;
	/**
	* @brief Check if file es in binary mode.
	* @return Always true (this method is virtual).
	*/
	virtual bool IsBinaryMode() const { return true; }
	/**
	 * @brief Get open mode (OF_NONCRIT, OF_TEXT, OF_BINARY, OF_DEFAULTMODE)
	 *
	 * Get rid of OF_NONCRIT type flags
	 * @return mode flags.
	*/
	UINT GetMode() const { return( m_uMode & 0x0FFFFFFF ); }
	/**
	* @brief Get open mode (full).
	* @return full mode flags.
	*/
	UINT GetFullMode() const { return m_uMode; }
	/**
	* @brief Check if file is open for write.
	* @return true if file is open for write, false otherwise.
	*/
	bool IsWriteMode() const { return ( m_uMode & OF_WRITE ); }

	/**
	* @brief Check if file is open.
	* @return true if file is open, false otherwise.
	*/
	virtual bool IsFileOpen() const { return ( m_hFile != NOFILE_HANDLE ); }
	/**
	* @brief Open a file in a specified mode.
	* @param pszName file to open.
	* @param uMode open mode.
	* @param pExtra TODOC.
	* @return true if file is open, false otherwise.
	*/
	virtual bool Open( LPCTSTR pszName = NULL, UINT uMode = OF_READ | OF_SHARE_DENY_NONE, void * pExtra = NULL );
	/**
	* @brief Closes the file if is open.
	*/
	virtual void Close();
	
	// File Access
public:
	CGFile() { m_uMode = 0; }
	virtual ~CGFile() { Close(); }

private:
	/**
	* @brief No copy on construction allowed.
	*/
	CGFile(const CGFile& copy);
	/**
	* @brief No copy allowed.
	*/
	CGFile& operator=(const CGFile& other);
};


/**
* @brief Text files. Try to be compatible with MFC CFile class.
*/
class CFileText : public CGFile
{
public:
	static const char *m_sClassName;
	FILE * m_pStream;		///< The current open script type file.
protected:
	/**
	* @brief Get open mode in string format.
	*
	* Formats:
	* - "rb"
	* - "w"
	* - "wb"
	* - "a+b"
	* @return string that describes the open mode.
	*/
	LPCTSTR GetModeStr() const;
#ifdef _WIN32
	bool	bNoBuffer;	///< TODOC.
#endif
protected:
	virtual bool OpenBase( void * pExtra );
	virtual void CloseBase();
public:
	CFileText()
	{
		m_pStream = NULL;
#ifdef _WIN32
		bNoBuffer = false;
#endif
	}
	virtual ~CFileText() { Close(); }
private:
	/**
	* @brief No copy on construction allowed.
	*/
	CFileText(const CFileText& copy);
	/**
	* @brief No copy allowed.
	*/
	CFileText& operator=(const CFileText& other);
public:
	/**
	* @brief Set the position indicator.
	* @param offset position to set.
	* @param origin origin (current position or init of the file).
	* @return position where the position indicator is set on success, 0 on error.
	*/
	virtual DWORD Seek( LONG offset = 0, UINT origin = SEEK_SET );
	/**
	* @brief Write changes to disk.
	*/
	void Flush() const;
	/**
	* @brief Get position indicator position.
	* @return The position indicator if file is opened, -1 otherwise.
	*/
	DWORD GetPosition() const;
	/**
	* @brief Reads data from the file.
	* @param pBuffer buffer where store the readed data.
	* @param sizemax count of bytes to read.
	* @return count of bytes readed.
	*/
	DWORD Read( void * pBuffer, size_t sizemax ) const;
	/**
	* @brief Reads from a file a line (up to sizemax - 1 characters).
	* @param pBuffer buffer where store the readed data.
	* @param sizemax count of characters to read.
	* @return the str readed if success, NULL on errors.
	*/
	TCHAR * ReadString( TCHAR * pBuffer, size_t sizemax ) const;
	/**
	* @brief writes supplied data into file.
	* @param pData data to write.
	* @param iLen lenght of the data to write.
	* @return true is success, false otherwise.
	*/
#ifndef _WIN32
	bool Write( const void * pData, DWORD iLen ) const;
#else
	bool Write( const void * pData, DWORD iLen );
#endif
	/**
	* @brief write string into file.
	* @return true is success, false otherwise.
	*/
	bool WriteString( LPCTSTR pStr );
	/**
	* @brief Check if file is open.
	* @return true if is open, false otherwise.
	*/
	bool IsFileOpen() const { return( m_pStream != NULL ); }
	/**
	* @brief Check if file is open in binary mode.
	* @return false always.
	*/
	bool IsBinaryMode() const { return false; }
	/**
	* @brief Check if EOF is reached.
	* @return true if EOF is reached, false otherwise.
	*/
	bool IsEOF() const;
	/**
	* @brief print in file a string with arguments (printf like).
	* @param pFormat string in "printf like" format.
	* @param args argument list.
	* @return total chars of the output.
	*/
	size_t VPrintf( LPCTSTR pFormat, va_list args );
	/**
	* @brief print in file a string with arguments (printf like).
	* @param pFormat string in "printf like" format.
	* @param ... argument list.
	* @return total chars of the output.
	*/
	size_t _cdecl Printf( LPCTSTR pFormat, ... ) __printfargs(2,3);
};

#endif // _INC_CFILE_H
