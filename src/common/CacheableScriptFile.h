#ifndef _INC_CACHEABLE_SCRIPT_FILE_H
#define _INC_CACHEABLE_SCRIPT_FILE_H
#pragma once

#include <string>
#include "CFile.h"

class CacheableScriptFile : public CFileText
{
protected:
	virtual bool OpenBase(void *pExtra);
	virtual void CloseBase();
	void dupeFrom(CacheableScriptFile *other);

public:
	CacheableScriptFile();
	~CacheableScriptFile();
private:
	CacheableScriptFile(const CacheableScriptFile& copy);
	CacheableScriptFile& operator=(const CacheableScriptFile& other);

public:
	virtual bool IsFileOpen() const;
	virtual bool IsEOF() const;
	virtual TCHAR *ReadString(TCHAR *pBuffer, size_t sizemax);
	virtual DWORD Seek(LONG offset = 0, UINT origin = SEEK_SET);
	virtual DWORD GetPosition() const;

private:
	bool m_closed;
	bool m_realFile;
	size_t m_currentLine;

protected:
	std::vector<std::string> * m_fileContent;

private:
	bool useDefaultFile() const;
};

#endif	// _INC_CACHEABLE_SCRIPT_FILE_H
