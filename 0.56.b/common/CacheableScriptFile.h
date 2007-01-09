#ifndef CACHEABLE_SCRIPT_FILE_H
#define CACHEABLE_SCRIPT_FILE_H

#include "CFile.h"

class CacheableScriptFile : public CFileText {
protected:
	virtual bool OpenBase(void *pExtra);
	virtual void CloseBase();
	void dupeFrom(CacheableScriptFile *other);

public:
	CacheableScriptFile();
	~CacheableScriptFile();

	virtual bool IsFileOpen() const;
	virtual bool IsEOF() const;
	virtual TCHAR *ReadString(TCHAR *pBuffer, size_t sizemax);
	virtual LONG Seek(LONG offset = 0, UINT origin = SEEK_SET);
	virtual DWORD GetPosition() const;

private:
	bool m_closed;
	bool m_realFile;
	struct TLine 
	{
		char *line;
		TLine *next;
	};
	TLine *lineHead;
	TLine *m_currentLine;

private:
	bool useDefaultFile() const;
	long getLineNumber(TLine *line) const;
};

#endif
