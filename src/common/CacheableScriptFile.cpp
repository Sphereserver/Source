#include "CacheableScriptFile.h"
#include "../graysvr/graysvr.h"

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
	if ( UseDefaultFile() )
		return CFileText::OpenBase(pExtra);

	ADDTOCALLSTACK("CacheableScriptFile::OpenBase");
	m_pStream = fopen(GetFilePath(), GetModeStr());
	if ( !m_pStream )
		return false;

	m_hFile = reinterpret_cast<OSFILE_TYPE>(STDFUNC_FILENO(m_pStream));
	m_fileContent = new std::vector<std::string>();
	m_closed = false;

	TemporaryString pszBuffer;
	size_t iLen;
	bool fFirstLine = true, fUTF = false;

	while ( !feof(m_pStream) )
	{
		pszBuffer.setAt(0, '\0');
		fgets(pszBuffer, SCRIPT_MAX_LINE_LEN, m_pStream);
		iLen = strlen(pszBuffer);

		// First line may contain UTF marker
		if ( fFirstLine && (iLen >= 3) && (pszBuffer[0] == 0xEF) && (pszBuffer[1] == 0xBB) && (pszBuffer[2] == 0xBF) )
			fUTF = true;

		std::string strLine((fUTF ? &pszBuffer[3] : pszBuffer), iLen - (fUTF ? 3 : 0));
		m_fileContent->push_back(strLine);
		fFirstLine = false;
		fUTF = false;
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
	if ( UseDefaultFile() )
		return CFileText::CloseBase();

	ADDTOCALLSTACK("CacheableScriptFile::CloseBase");
	if ( m_realFile && m_fileContent )
	{
		m_fileContent->clear();
		delete m_fileContent;
	}

	m_fileContent = NULL;
	m_currentLine = 0;
	m_closed = true;
}

void CacheableScriptFile::DupeFrom(CacheableScriptFile *other)
{
	if ( UseDefaultFile() )
		return;

	m_closed = other->m_closed;
	m_realFile = false;
	m_fileContent = other->m_fileContent;
}

bool CacheableScriptFile::IsFileOpen() const
{
	if ( UseDefaultFile() )
		return CFileText::IsFileOpen();

	ADDTOCALLSTACK("CacheableScriptFile::IsFileOpen");
	return !m_closed;
}

bool CacheableScriptFile::IsEOF() const
{
	if ( UseDefaultFile() )
		return CFileText::IsEOF();

	ADDTOCALLSTACK("CacheableScriptFile::IsEOF");
	return (!m_fileContent || (m_currentLine == m_fileContent->size()));
}

TCHAR *CacheableScriptFile::ReadString(TCHAR *pBuffer, size_t iSizeMax)
{
	if ( UseDefaultFile() )
		return CFileText::ReadString(pBuffer, iSizeMax);

	ADDTOCALLSTACK("CacheableScriptFile::ReadString");
	if ( m_fileContent && (m_currentLine < m_fileContent->size()) )
	{
		*pBuffer = '\0';
		strcpy(pBuffer, m_fileContent->at(m_currentLine).c_str());
		m_currentLine++;
		return pBuffer;
	}
	return NULL;
}

DWORD CacheableScriptFile::Seek(long lOffset, UINT uOrigin)
{
	if ( UseDefaultFile() )
		return CFileText::Seek(lOffset, uOrigin);

	ADDTOCALLSTACK("CacheableScriptFile::Seek");
	size_t iLine = lOffset;

	if ( uOrigin != SEEK_SET )
		iLine = 0;	// do not support non-SEEK_SET rotation

	if ( iLine <= m_fileContent->size() )
	{
		m_currentLine = iLine;
		return iLine;
	}
	return 0;
}

DWORD CacheableScriptFile::GetPosition() const
{
	if ( UseDefaultFile() )
		return CFileText::GetPosition();

	ADDTOCALLSTACK("CacheableScriptFile::GetPosition");
	return m_currentLine;
}

bool CacheableScriptFile::UseDefaultFile() const
{
	return (IsWriteMode() || (GetFullMode() & OF_DEFAULTMODE));
}
