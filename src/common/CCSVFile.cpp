#include "../graysvr/graysvr.h"

CCSVFile::CCSVFile()
{
	m_iColumnCount = 0;
	m_iCurrentRow = 0;
	m_pszColumnTypes[0] = NULL;
	m_pszColumnNames[0] = NULL;
}

CCSVFile::~CCSVFile()
{
	for ( size_t i = 0; m_pszColumnTypes[i] != NULL; ++i )
		delete[] m_pszColumnTypes[i];

	for ( size_t i = 0; m_pszColumnNames[i] != NULL; ++i )
		delete[] m_pszColumnNames[i];
}

bool CCSVFile::OpenBase(void *pExtra)
{
	ADDTOCALLSTACK("CCSVFile::OpenBase");
	if ( !CCacheableScriptFile::OpenBase(pExtra) )
		return false;

	m_iCurrentRow = 0;

	// Remove all empty lines so that we just have data rows stored
	for ( std::vector<std::string>::iterator i = m_fileContent->begin(); i != m_fileContent->end(); )
	{
		LPCTSTR pszLine = i->c_str();
		GETNONWHITESPACE(pszLine);
		if ( *pszLine == '\0' )
			i = m_fileContent->erase(i);
		else
			++i;
	}

	// Find type and name of the columns
	TCHAR *ppColumnTypes[FILE_MAX_COLUMNS];
	TCHAR *ppColumnNames[FILE_MAX_COLUMNS];

	// First row tells us how many columns there are
	m_iColumnCount = ReadRowContent(ppColumnTypes, 0);
	if ( m_iColumnCount <= 0 )
	{
		m_iColumnCount = 0;
		Close();
		return false;
	}

	// Second row lets us validate the column count
	if ( ReadRowContent(ppColumnNames, 1) != m_iColumnCount )
	{
		m_iColumnCount = 0;
		Close();
		return false;
	}

	// Copy the names
	for ( size_t i = 0; i < m_iColumnCount; ++i )
	{
		m_pszColumnTypes[i] = new TCHAR[128];
		strncpy(m_pszColumnTypes[i], ppColumnTypes[i], COUNTOF(m_pszColumnTypes) - 1);

		m_pszColumnNames[i] = new TCHAR[128];
		strncpy(m_pszColumnNames[i], ppColumnNames[i], COUNTOF(m_pszColumnNames) - 1);
	}

	m_pszColumnTypes[m_iColumnCount] = NULL;
	m_pszColumnNames[m_iColumnCount] = NULL;
	return true;
}

size_t CCSVFile::ReadRowContent(TCHAR **ppOutput, size_t rowIndex, size_t columns)
{
	ADDTOCALLSTACK("CCSVFile::ReadRowContent");
	ASSERT((columns > 0) && (columns <= FILE_MAX_COLUMNS));
	if ( GetPosition() != rowIndex )
		Seek(static_cast<long>(rowIndex), SEEK_SET);

	TCHAR *pszLine = Str_GetTemp();
	if ( ReadString(pszLine, THREAD_STRING_LENGTH) == NULL )
		return 0;

	return Str_ParseCmds(pszLine, ppOutput, columns, "\t");
}

bool CCSVFile::ReadRowContent(size_t rowIndex, CSVRowData &target)
{
	ADDTOCALLSTACK("CCSVFile::ReadRowContent");
	// Get row data
	TCHAR *ppRowContent[FILE_MAX_COLUMNS];
	size_t columns = ReadRowContent(ppRowContent, rowIndex);
	if ( columns != m_iColumnCount )
		return false;

	// Copy to target
	target.clear();
	for ( size_t i = 0; i < columns; ++i )
		target[m_pszColumnNames[i]] = ppRowContent[i];

	return !target.empty();
}

size_t CCSVFile::ReadNextRowContent(TCHAR **ppOutput)
{
	ADDTOCALLSTACK("CCSVFile::ReadNextRowContent");
	++m_iCurrentRow;
	return ReadRowContent(ppOutput, m_iCurrentRow);
}

bool CCSVFile::ReadNextRowContent(CSVRowData &target)
{
	ADDTOCALLSTACK("CCSVFile::ReadNextRowContent");
	++m_iCurrentRow;
	return ReadRowContent(m_iCurrentRow, target);
}
