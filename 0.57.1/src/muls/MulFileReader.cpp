#include "MulFileReader.h"

const char *MulFileReader::m_sClassName = "MulFileReader";

MulFileReader::MulFileReader(const char *baseFileName)
{
	m_fileName = baseFileName;
	m_hFile = NULL;
	m_buf = (char *)malloc(0x1000 * sizeof(BYTE));
}

MulFileReader::~MulFileReader()
{
	close();
	free(m_buf);
}

void MulFileReader::close()
{
	if ( m_hFile != NULL )
	{
		fclose(m_hFile);
		m_hFile = NULL;
	}
}

const char *MulFileReader::name()
{
	return m_fileName;
}

bool MulFileReader::open()
{
	EXC_TRY("open");

	CGString fullFileName = g_Install.GetPreferPath(m_fileName);
	m_hFile = fopen(fullFileName, "rb");
	return ( m_hFile ? onOpen() : false );

	EXC_CATCH;
	return false;
}

bool MulFileReader::onOpen()
{
	return true;
}

DWORD MulFileReader::readDWORD()
{
	DWORD ret;
	fread(&ret, sizeof(DWORD), 1, m_hFile);
	return ret;
}

WORD MulFileReader::readWORD()
{
	WORD ret;
	fread(&ret, sizeof(WORD), 1, m_hFile);
	return ret;
}

BYTE MulFileReader::readBYTE()
{
	BYTE ret;
	fread(&ret, sizeof(BYTE), 1, m_hFile);
	return ret;
}

char *MulFileReader::readString(long max)
{
	*m_buf = 0;
	fread(m_buf, 1, max, m_hFile);
	return m_buf;
}
