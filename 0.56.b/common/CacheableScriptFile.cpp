#include "../graysvr/graysvr.h"
#include "CacheableScriptFile.h"

CacheableScriptFile::CacheableScriptFile() 
{
	m_closed = true;
	lineHead = NULL;
	m_currentLine = NULL;
	m_realFile = false;
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

	m_hFile = (OSFILE_TYPE)_fileno(m_pStream);

	m_closed = false;

	lineHead = new TLine;
	lineHead->line = NULL;
	lineHead->next = NULL;

	TLine *currentLine = lineHead;

	char *buf = Str_GetTemp();
	int index = 0;
	while( true ) 
	{
		fgets(buf, SCRIPT_MAX_LINE_LEN, m_pStream);
		if( feof(m_pStream) )  
		{
			break;
		}

		int len = strlen(buf);
		currentLine->line = new char[len+1];
		strcpy(currentLine->line, buf);
		currentLine->index = index;

		TLine *newLine = new TLine;
		newLine->line = NULL;
		newLine->next = NULL;

		currentLine->next = newLine;
		currentLine = newLine;

		index++;
	}

	fclose(m_pStream);
	m_pStream = NULL;
	m_hFile = 0;
	m_currentLine = lineHead;

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
			if( lineHead != NULL ) 
			{
				TLine *prevLine = NULL;
				for( TLine *line = lineHead; line != NULL; line = line->next ) 
				{
					if( prevLine != NULL ) 
					{
						delete prevLine;
					}

					prevLine = line;

					if( line->line != NULL ) 
					{
						delete []line->line;
					}
				}
			}
		}
		lineHead = NULL;
		m_currentLine = NULL;
		m_closed = true;
	}
}

bool CacheableScriptFile::IsFileOpen() const {
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
	return (( m_currentLine == NULL ) || ( m_currentLine->next == NULL ));
}

TCHAR *CacheableScriptFile::ReadString(TCHAR *pBuffer, size_t sizemax) 
{
	if( useDefaultFile() ) 
	{
		return CFileText::ReadString(pBuffer, sizemax);
	}

	ADDTOCALLSTACK("CacheableScriptFile::ReadString");
	*pBuffer = NULL;

	if(( m_currentLine != NULL ) && ( m_currentLine->line != NULL )) 
	{
		strcpy(pBuffer, m_currentLine->line);
		m_currentLine = m_currentLine->next;
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

	for( TLine *l = lineHead; l != NULL; l = l->next ) 
	{
		if( l->index == linenum ) 
		{
			m_currentLine = l;
			return l->index;
		}
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
	return m_currentLine->index;
}

void CacheableScriptFile::dupeFrom(CacheableScriptFile *other) 
{
	if( useDefaultFile() ) 
	{
		return;
	}

	ADDTOCALLSTACK("CacheableScriptFile::dupeFrom");
	m_closed = other->m_closed;

	m_realFile = false;
	lineHead = other->lineHead;
}

bool CacheableScriptFile::useDefaultFile() const 
{
	if( IsWriteMode() || ( GetFullMode() & OF_DEFAULTMODE )) 
	{
		return true;
	}

	return false;
}
