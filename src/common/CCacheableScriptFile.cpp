#include "../graysvr/graysvr.h"
#include "CCacheableScriptFile.h"

CCacheableScriptFile::CCacheableScriptFile()
{
	m_closed = true;
	m_realFile = false;
	m_currentLine = 0;
	m_fileContent = NULL;
}

CCacheableScriptFile::~CCacheableScriptFile()
{
	Close();
}

bool CCacheableScriptFile::OpenBase(void *pExtra)
{
	if ( UseDefaultFile() )
		return CFileText::OpenBase(pExtra);

	ADDTOCALLSTACK("CCacheableScriptFile::OpenBase");
	m_pStream = fopen(GetFilePath(), GetModeStr());
	if ( !m_pStream )
		return false;

	m_hFile = reinterpret_cast<HANDLE>(STDFUNC_FILENO(m_pStream));
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

void CCacheableScriptFile::CloseBase()
{
	if ( UseDefaultFile() )
		return CFileText::CloseBase();

	ADDTOCALLSTACK("CCacheableScriptFile::CloseBase");
	if ( m_realFile && m_fileContent )
	{
		m_fileContent->clear();
		delete m_fileContent;
	}

	m_fileContent = NULL;
	m_currentLine = 0;
	m_closed = true;
}

void CCacheableScriptFile::DupeFrom(CCacheableScriptFile *other)
{
	if ( UseDefaultFile() )
		return;

	m_closed = other->m_closed;
	m_realFile = false;
	m_fileContent = other->m_fileContent;
}

bool CCacheableScriptFile::IsFileOpen() const
{
	if ( UseDefaultFile() )
		return CFileText::IsFileOpen();

	ADDTOCALLSTACK("CCacheableScriptFile::IsFileOpen");
	return !m_closed;
}

bool CCacheableScriptFile::IsEOF() const
{
	if ( UseDefaultFile() )
		return CFileText::IsEOF();

	ADDTOCALLSTACK("CCacheableScriptFile::IsEOF");
	return (!m_fileContent || (m_currentLine == m_fileContent->size()));
}

TCHAR *CCacheableScriptFile::ReadString(TCHAR *pBuffer, size_t iSizeMax)
{
	if ( UseDefaultFile() )
		return CFileText::ReadString(pBuffer, iSizeMax);

	ADDTOCALLSTACK("CCacheableScriptFile::ReadString");
	if ( m_fileContent && (m_currentLine < m_fileContent->size()) )
	{
		*pBuffer = '\0';
		strcpy(pBuffer, m_fileContent->at(m_currentLine).c_str());
		++m_currentLine;
		return pBuffer;
	}
	return NULL;
}

DWORD CCacheableScriptFile::Seek(long lOffset, UINT uOrigin)
{
	if ( UseDefaultFile() )
		return CFileText::Seek(lOffset, uOrigin);

	ADDTOCALLSTACK("CCacheableScriptFile::Seek");
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

DWORD CCacheableScriptFile::GetPosition() const
{
	if ( UseDefaultFile() )
		return CFileText::GetPosition();

	ADDTOCALLSTACK("CCacheableScriptFile::GetPosition");
	return m_currentLine;
}

bool CCacheableScriptFile::UseDefaultFile() const
{
	return (IsWriteMode() || (GetFullMode() & OF_DEFAULTMODE));
}
