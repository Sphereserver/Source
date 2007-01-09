#ifndef MULFILEREADER_H
#define MULFILEREADER_H

#include "../graysvr.h"

class MulFileReader
{
public:
	static const char *m_sClassName;

	MulFileReader(const char *baseFileName);
	~MulFileReader();

	const char *name();			// returns MUL name

	bool open();				// open MUL file
	void close();				// close MUL file

	virtual bool onOpen();		// called if file opened OK

	DWORD readDWORD();			// read 32bit
	WORD  readWORD();			// read 16bit
	BYTE  readBYTE();			// read 8bit
	char *readString(long max);	// returns static buffer, copy early!

protected:
	const char	*m_fileName;
	FILE		*m_hFile;
	char		*m_buf;
};

#endif
