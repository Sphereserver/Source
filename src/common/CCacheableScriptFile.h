#ifndef _INC_CCACHEABLESCRIPTFILE_H
#define _INC_CCACHEABLESCRIPTFILE_H
#pragma once

#include <string>
#include "CFile.h"

class CCacheableScriptFile : public CFileText
{
public:
	CCacheableScriptFile();
	~CCacheableScriptFile();

private:
	bool m_closed;
	bool m_realFile;
	size_t m_currentLine;

protected:
	std::vector<std::string> *m_fileContent;

protected:
	virtual bool OpenBase(void *pExtra);
	virtual void CloseBase();
	void DupeFrom(CCacheableScriptFile *other);

public:
	virtual bool IsFileOpen() const;
	virtual bool IsEOF() const;
	virtual TCHAR *ReadString(TCHAR *pBuffer, size_t iSizeMax);
	virtual DWORD Seek(long lOffset = 0, UINT uOrigin = SEEK_SET);
	virtual DWORD GetPosition() const;

private:
	bool UseDefaultFile() const;

private:
	CCacheableScriptFile(const CCacheableScriptFile &copy);
	CCacheableScriptFile &operator=(const CCacheableScriptFile &other);
};

#endif	// _INC_CCACHEABLESCRIPTFILE_H
