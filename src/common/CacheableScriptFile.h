#ifndef _INC_CACHEABLESCRIPTFILE_H
#define _INC_CACHEABLESCRIPTFILE_H
#pragma once

#include <string>
#include "CFile.h"

class CacheableScriptFile : public CFileText
{
public:
	CacheableScriptFile();
	~CacheableScriptFile();

private:
	bool m_closed;
	bool m_realFile;
	size_t m_currentLine;

protected:
	std::vector<std::string> *m_fileContent;

protected:
	virtual bool OpenBase(void *pExtra);
	virtual void CloseBase();
	void DupeFrom(CacheableScriptFile *other);

public:
	virtual bool IsFileOpen() const;
	virtual bool IsEOF() const;
	virtual TCHAR *ReadString(TCHAR *pBuffer, size_t iSizeMax);
	virtual DWORD Seek(long lOffset = 0, UINT uOrigin = SEEK_SET);
	virtual DWORD GetPosition() const;

private:
	bool UseDefaultFile() const;

private:
	CacheableScriptFile(const CacheableScriptFile &copy);
	CacheableScriptFile &operator=(const CacheableScriptFile &other);
};

#endif	// _INC_CACHEABLESCRIPTFILE_H
