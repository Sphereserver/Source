#ifndef _INC_CFILE_H
#define _INC_CFILE_H
#pragma once

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
	#define OF_READ					O_RDONLY
	#define OF_WRITE				O_WRONLY
	#define OF_READWRITE			O_RDWR
	#define OF_SHARE_DENY_NONE		0x0
	#define OF_SHARE_DENY_WRITE		0x0		// not defined in LINUX
	#define OF_CREATE				O_CREAT
#endif

#define OF_NONCRIT			0x40000000	// just a test
#define OF_TEXT				0x20000000
#define OF_BINARY			0x10000000
#define OF_DEFAULTMODE		0x80000000

#ifndef HFILE_ERROR
	#define HFILE_ERROR -1
	#define HFILE int
#endif

#ifdef _WIN32
	#define INVALID_HANDLE	((HANDLE) -1)
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
	OSFILE_TYPE m_hFile;

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
		Seek(0, SEEK_SET);
	}
	DWORD SeekToEnd()
	{
		return Seek(0, SEEK_END);
	}

	DWORD GetLength();
	virtual DWORD GetPosition() const;
	virtual DWORD Seek(LONG lOffset = 0, UINT uOrigin = SEEK_SET);
	virtual DWORD Read(void *pData, DWORD dwLength) const;
	virtual bool Write(const void *pData, DWORD dwLength) const;
#ifdef _WIN32
	void NotifyIOError(LPCTSTR pszMessage) const;
#endif

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
		fNoBuffer = false;
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
	bool fNoBuffer;
#endif

protected:
	LPCTSTR GetModeStr() const;
	virtual bool OpenBase(void *pExtra);
	virtual void CloseBase();

public:
	virtual DWORD Seek(LONG lOffset = 0, UINT uOrigin = SEEK_SET);
	void Flush() const;
	DWORD GetPosition() const;

	DWORD Read(void *pBuffer, size_t iSizeMax) const;
	TCHAR *ReadString(TCHAR *pszBuffer, size_t iSizeMax) const;
#ifndef _WIN32
	bool Write(const void *pData, DWORD dwLen) const;
#else
	bool Write(const void *pData, DWORD dwLen);
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
