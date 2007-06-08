#include "../graysvr/graysvr.h"
#include "CacheableScriptFile.h"

CacheableScriptFile::CacheableScriptFile()
{
	m_closed = true;
	m_realFile = false;
	m_currentLine = -1;
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
	m_fileContent = new std::vector<std::string>();
	m_pStream = fopen(GetFilePath(), GetModeStr());
	
	if( m_pStream == NULL ) 
	{
		return false;
	}

	m_hFile = (OSFILE_TYPE)STDFUNC_FILENO(m_pStream);
	m_closed = false;
	TemporaryString buf;
	
	while( !feof(m_pStream) ) 
	{
		buf.setAt(0, '\0');
		fgets(buf, SCRIPT_MAX_LINE_LEN, m_pStream);

		std::string strLine(buf, strlen(buf));
		m_fileContent->push_back(strLine);
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
			m_fileContent->clear();
			if ( m_fileContent )
				delete m_fileContent;
		}

		m_fileContent = NULL;
		m_currentLine = -1;
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
	return (( m_currentLine == -1 ) || ( m_currentLine == m_fileContent->size() ));
}

TCHAR * CacheableScriptFile::ReadString(TCHAR *pBuffer, size_t sizemax) 
{
	if( useDefaultFile() ) 
	{
		return CFileText::ReadString(pBuffer, sizemax);
	}

	ADDTOCALLSTACK("CacheableScriptFile::ReadString");
	*pBuffer = NULL;

	if(( m_currentLine != -1 ) && ( m_currentLine != m_fileContent->size() ))
	{
		strcpy(pBuffer, (m_fileContent->at(m_currentLine)).c_str() );
		m_currentLine += 1;
	}
	else 
	{
		return NULL;
	}

	return pBuffer;
}

LONG CacheableScriptFile::Seek(LONG offset, UINT origin) 
{
	if( useDefaultFile() ) 
	{
		return CFileText::Seek(offset, origin);
	}

	ADDTOCALLSTACK("CacheableScriptFile::Seek");
	int linenum = offset;

	if( origin != SEEK_SET ) 
	{
		linenum = 0;	//	do not support not SEEK_SET rotation
	}
	
	if ( linenum < m_fileContent->size() )
	{
		m_currentLine = linenum;
		return linenum;
	}

	return -1;
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
