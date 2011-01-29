#include "../graysvr/graysvr.h"
#include "CsvFile.h"

CSVFile::CSVFile()
{
	m_iColumnCount = 0;
	m_iCurrentRow = 0;
	m_pszColumnTypes[0] = NULL;
	m_pszColumnNames[0] = NULL;
}

CSVFile::~CSVFile()
{
	for (size_t i = 0; m_pszColumnTypes[i] != NULL; i++)
		delete[] m_pszColumnTypes[i];

	for (size_t i = 0; m_pszColumnNames[i] != NULL; i++)
		delete[] m_pszColumnNames[i];
}

bool CSVFile::OpenBase(void * pExtra)
{
	ADDTOCALLSTACK("CSVFile::OpenBase");
	if ( !PhysicalScriptFile::OpenBase(pExtra) )
		return false;

	m_iCurrentRow = 0;

	// remove all empty lines so that we just have data rows stored
	for (std::vector<std::string>::iterator i = m_fileContent->begin(); i != m_fileContent->end(); )
	{
		LPCTSTR pszLine = i->c_str();
		GETNONWHITESPACE(pszLine);
		if ( *pszLine == '\0' )
			i = m_fileContent->erase(i);
		else
			++i;
	}

	// find the types and names of the columns
	TCHAR * ppColumnTypes[MAX_COLUMNS];
	TCHAR * ppColumnNames[MAX_COLUMNS];

	// first row tells us how many columns there are
	m_iColumnCount = ReadRowContent(ppColumnTypes, 0);
	if ( m_iColumnCount <= 0 )
	{
		m_iColumnCount = 0;
		Close();
		return false;
	}
	
	// second row lets us validate the column count
	if ( ReadRowContent(ppColumnNames, 1) != m_iColumnCount )
	{
		m_iColumnCount = 0;
		Close();
		return false;
	}

	// copy the names
	for (size_t i = 0; i < m_iColumnCount; i++)
	{
		m_pszColumnTypes[i] = new TCHAR[128];
		strcpy(m_pszColumnTypes[i], ppColumnTypes[i]);

		m_pszColumnNames[i] = new TCHAR[128];
		strcpy(m_pszColumnNames[i], ppColumnNames[i]);
	}

	m_pszColumnTypes[m_iColumnCount] = NULL;
	m_pszColumnNames[m_iColumnCount] = NULL;
	return true;
}

size_t CSVFile::ReadRowContent(TCHAR ** ppOutput, size_t rowIndex, int columns)
{
	ADDTOCALLSTACK("CSVFile::ReadRowContent");
	ASSERT(columns > 0 && columns <= MAX_COLUMNS);
	if ( GetPosition() != rowIndex )
		Seek(rowIndex, SEEK_SET);

	TCHAR * pszLine = Str_GetTemp();
	if ( ReadString(pszLine, THREAD_STRING_LENGTH) == NULL )
		return 0;

	return Str_ParseCmds(pszLine, ppOutput, columns, "\t");
}

size_t CSVFile::ReadNextRowContent(TCHAR ** ppOutput)
{
	ADDTOCALLSTACK("CSVFile::ReadNextRowContent");
	++m_iCurrentRow;
	return ReadRowContent(ppOutput, m_iCurrentRow);
}

bool CSVFile::ReadRowContent(size_t rowIndex, CSVRowData& target)
{
	ADDTOCALLSTACK("CSVFile::ReadRowContent");
	// get row data
	TCHAR * ppRowContent[MAX_COLUMNS];
	size_t columns = ReadRowContent(ppRowContent, rowIndex);
	if ( columns != m_iColumnCount )
		return false;

	// copy to target
	target.clear();
	for (size_t i = 0; i < columns; i++)
		target[m_pszColumnNames[i]] = ppRowContent[i];

	return ! target.empty();
}

bool CSVFile::ReadNextRowContent(CSVRowData& target)
{
	ADDTOCALLSTACK("CSVFile::ReadNextRowContent");
	++m_iCurrentRow;
	return ReadRowContent(m_iCurrentRow, target);
}
