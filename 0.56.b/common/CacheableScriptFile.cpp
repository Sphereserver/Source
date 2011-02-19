#include "../graysvr/graysvr.h"
#include "CacheableScriptFile.h"

CacheableScriptFile::CacheableScriptFile()
{
	m_closed = true;
	m_realFile = false;
	m_currentLine = 0;
	m_fileContent = NULL;
}

CacheableScriptFile::~CacheableScriptFile() 
{
	Close();
}

bool CacheableScriptFile::OpenBase(void *pExtra) 
{
	if( useDefaultFile() ) 
	{
		return CFileText::OpenBase(pExtra);
	}

	ADDTOCALLSTACK("CacheableScriptFile::OpenBase");

	m_pStream = fopen(GetFilePath(), GetModeStr());
	if( m_pStream == NULL ) 
	{
		return false;
	}

	m_fileContent = new std::vector<std::string>();
	m_hFile = (OSFILE_TYPE)STDFUNC_FILENO(m_pStream);
	m_closed = false;
	TemporaryString buf;
	size_t nStrLen;
	bool bUTF = false, bFirstLine = true;
	
	while( !feof(m_pStream) ) 
	{
		buf.setAt(0, '\0');
		fgets(buf, SCRIPT_MAX_LINE_LEN, m_pStream);
		nStrLen = strlen(buf);

		// first line may contain utf marker
		if ( bFirstLine && nStrLen >= 3 &&
			static_cast<unsigned char>(buf[0]) == 0xEF &&
			static_cast<unsigned char>(buf[1]) == 0xBB &&
			static_cast<unsigned char>(buf[2]) == 0xBF )
			bUTF = true;

		std::string strLine((bUTF ? &buf[3]:buf), nStrLen - (bUTF ? 3:0));
		m_fileContent->push_back(strLine);
		bFirstLine = false;
		bUTF = false;
	}

	fclose(m_pStream);
	m_pStream = NULL;
	m_hFile = 0;
	m_currentLine = 0;
	m_realFile = true;

	return true;
}

void CacheableScriptFile::CloseBase() 
{
	if( useDefaultFile() ) 
	{
		CFileText::CloseBase();
	}
	else 
	{
		ADDTOCALLSTACK("CacheableScriptFile::CloseBase");

		//	clear all data
		if( m_realFile ) 
		{
			if ( m_fileContent != NULL )
			{
				m_fileContent->clear();
				delete m_fileContent;
			}
		}

		m_fileContent = NULL;
		m_currentLine = 0;
		m_closed = true;
	}
}

bool CacheableScriptFile::IsFileOpen() const 
{
	if( useDefaultFile() ) 
	{
		return CFileText::IsFileOpen();
	}

	ADDTOCALLSTACK("CacheableScriptFile::IsFileOpen");
	return !m_closed;
}

bool CacheableScriptFile::IsEOF() const 
{
	if( useDefaultFile() ) 
	{
		return CFileText::IsEOF();
	}

	ADDTOCALLSTACK("CacheableScriptFile::IsEOF");
	return ( m_fileContent == NULL || m_currentLine == m_fileContent->size() );
}

TCHAR * CacheableScriptFile::ReadString(TCHAR *pBuffer, size_t sizemax) 
{
	if( useDefaultFile() ) 
	{
		return CFileText::ReadString(pBuffer, sizemax);
	}

	ADDTOCALLSTACK("CacheableScriptFile::ReadString");
	*pBuffer = NULL;

	if ( m_fileContent != NULL && m_currentLine < m_fileContent->size() )
	{
		strcpy(pBuffer, (m_fileContent->at(m_currentLine)).c_str() );
		m_currentLine++;
	}
	else 
	{
		return NULL;
	}

	return pBuffer;
}

DWORD CacheableScriptFile::Seek(LONG offset, UINT origin) 
{
	if( useDefaultFile() ) 
	{
		return CFileText::Seek(offset, origin);
	}

	ADDTOCALLSTACK("CacheableScriptFile::Seek");
	size_t linenum = offset;

	if( origin != SEEK_SET ) 
	{
		linenum = 0;	//	do not support not SEEK_SET rotation
	}
	
	if ( linenum < m_fileContent->size() )
	{
		m_currentLine = linenum;
		return linenum;
	}

	return 0;
}

DWORD CacheableScriptFile::GetPosition() const 
{
	if( useDefaultFile() ) 
	{
		return CFileText::GetPosition();
	}

	ADDTOCALLSTACK("CacheableScriptFile::GetPosition");
	return m_currentLine;
}

void CacheableScriptFile::dupeFrom(CacheableScriptFile *other) 
{
	if( useDefaultFile() ) 
	{
		return;
	}

	m_closed = other->m_closed;
	m_realFile = false;
	m_fileContent = other->m_fileContent;
}

bool CacheableScriptFile::useDefaultFile() const 
{
	if( IsWriteMode() || ( GetFullMode() & OF_DEFAULTMODE )) 
	{
		return true;
	}

	return false;
}
