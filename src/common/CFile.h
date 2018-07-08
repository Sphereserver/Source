#ifndef _INC_CFILE_H
#define _INC_CFILE_H
#pragma once

#ifndef _WIN32
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <unistd.h>
#endif

#include "CString.h"

#ifndef _WIN32
	// Port some Windows stuff to Linux
	typedef int						HANDLE;

	#define FILE_BEGIN				SEEK_SET
	#define FILE_CURRENT			SEEK_CUR
	#define FILE_END				SEEK_END

	#define OF_READ					O_RDONLY
	#define OF_WRITE				O_WRONLY
	#define OF_READWRITE			O_RDWR
	#define OF_CREATE				O_CREAT
	#define OF_SHARE_DENY_WRITE		0x0		// not available on Linux
	#define OF_SHARE_DENY_NONE		0x0		// not available on Linux
#endif

#define OF_BINARY			0x10000000
#define OF_TEXT				0x20000000
#define OF_NONCRIT			0x40000000	// just a test
#define OF_DEFAULTMODE		0x80000000

#define NOFILE_HANDLE		((HANDLE)-1)

class CGrayError;
#define CFileException CGrayError

class CFile
{
public:
	static const char *m_sClassName;

	CFile()
	{
		m_hFile = NOFILE_HANDLE;
	}
	virtual ~CFile()
	{
		Close();
	}

public:
	HANDLE m_hFile;

protected:
	CGString m_strFileName;

public:
	const CGString &GetFilePath() const
	{
		return m_strFileName;
	}
	virtual bool SetFilePath(LPCTSTR pszPath);
	LPCTSTR GetFileTitle() const;

	virtual bool Open(LPCTSTR pszFileName = NULL, UINT uMode = OF_READ|OF_SHARE_DENY_NONE, CFileException *e = NULL);
	virtual void Close();

	void SeekToBegin()
	{
		Seek(0, FILE_BEGIN);
	}
	DWORD SeekToEnd()
	{
		return Seek(0, FILE_END);
	}

	DWORD GetLength();
	virtual DWORD GetPosition() const;
	virtual DWORD Seek(LONG lOffset = 0, DWORD dwMoveMethod = FILE_BEGIN);
	virtual DWORD Read(void *pBuffer, size_t iLength) const;
	virtual bool Write(const void *pBuffer, size_t iLength) const;
	void NotifyIOError(LPCTSTR pszMessage) const;

private:
	CFile(const CFile &copy);
	CFile &operator=(const CFile &other);
};

class CGFile : public CFile
{
public:
	static const char *m_sClassName;

	CGFile()
	{
		m_uMode = 0;
	}
	virtual ~CGFile()
	{
		Close();
	}

private:
	UINT m_uMode;	// MMSYSTEM may use 32 bit flags

private:
	virtual bool OpenBase(void *pExtra);
	virtual void CloseBase();

public:
	static LPCTSTR GetFilesTitle(LPCTSTR pszPath);
	static LPCTSTR GetFilesExt(LPCTSTR pszPath);
	static CGString GetMergedFileName(LPCTSTR pszPath, LPCTSTR pszFileName);
	static DWORD GetLastError();
	LPCTSTR GetFileExt() const;

	UINT GetMode() const
	{
		return (m_uMode & 0xFFFFFFF);
	}
	UINT GetFullMode() const
	{
		return m_uMode;
	}
	virtual bool IsBinaryMode() const
	{
		return true;
	}
	bool IsWriteMode() const
	{
		return (m_uMode & OF_WRITE);
	}

	virtual bool IsFileOpen() const
	{
		return (m_hFile != NOFILE_HANDLE);
	}
	virtual bool Open(LPCTSTR pszFileName = NULL, UINT uMode = OF_READ|OF_SHARE_DENY_NONE, void *pExtra = NULL);
	virtual void Close();

private:
	CGFile(const CGFile &copy);
	CGFile &operator=(const CGFile &other);
};

class CFileText : public CGFile
{
public:
	static const char *m_sClassName;

	CFileText()
	{
		m_pStream = NULL;
#ifdef _WIN32
		m_fNoBuffer = false;
#endif
	}
	virtual ~CFileText()
	{
		Close();
	}

public:
	FILE *m_pStream;

#ifdef _WIN32
protected:
	bool m_fNoBuffer;
#endif

protected:
	LPCTSTR GetModeStr() const;
	virtual bool OpenBase(void *pExtra);
	virtual void CloseBase();

public:
	virtual DWORD Seek(LONG lOffset = 0, DWORD dwMoveMethod = FILE_BEGIN);
	void Flush() const;
	DWORD GetPosition() const;

	DWORD Read(void *pBuffer, size_t iLength) const;
	TCHAR *ReadString(TCHAR *pszBuffer, size_t iLength) const;
#ifndef _WIN32
	bool Write(const void *pBuffer, size_t iLength) const;
#else
	bool Write(const void *pBuffer, size_t iLength);
#endif
	bool WriteString(LPCTSTR pszArgs);

	bool IsFileOpen() const
	{
		return (m_pStream != NULL);
	}
	bool IsBinaryMode() const
	{
		return false;
	}
	bool IsEOF() const;

	size_t VPrintf(LPCTSTR pszFormat, va_list args);
	size_t _cdecl Printf(LPCTSTR pszFormat, ...) __printfargs(2, 3);

private:
	CFileText(const CFileText &copy);
	CFileText &operator=(const CFileText &other);
};

#endif	// _INC_CFILE_H
