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
	for (int i = 0; m_pszColumnTypes[i] != NULL; i++)
		delete[] m_pszColumnTypes[i];

	for (int i = 0; m_pszColumnNames[i] != NULL; i++)
		delete[] m_pszColumnNames[i];
}

bool CSVFile::OpenBase(void * pExtra)
{
	ADDTOCALLSTACK("CSVFile::OpenBase");
	if ( !PhysicalScriptFile::OpenBase(pExtra) )
		return false;

	m_iCurrentRow = 0;

	// remove all empty lines so that we just have data rows stored
	for (std::vector<std::string>::iterator i = m_fileContent->begin(); i != m_fileContent->end(); i++ )
	{
		LPCTSTR pszLine = i->c_str();
		GETNONWHITESPACE(pszLine);
		if ( *pszLine != '\0' )
			continue;

		i = m_fileContent->erase(i);
		i--;
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
	for (int i = 0; i < m_iColumnCount; i++)
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

int CSVFile::GetColumnCount()
{
	return m_iColumnCount;
}

int CSVFile::ReadRowContent(char ** ppOutput, DWORD dwRowIndex, int iMaxColumns)
{
	ADDTOCALLSTACK("CSVFile::ReadRowContent");
	if ( dwRowIndex == -1 )
		dwRowIndex = m_iCurrentRow++;

	if ( GetPosition() != dwRowIndex )
		Seek(dwRowIndex, SEEK_SET);

	TCHAR * pszLine = Str_GetTemp();
	if ( ReadString(pszLine, THREAD_STRING_LENGTH) == NULL )
		return -1;

	int iCount = Str_ParseCmds(pszLine, ppOutput, iMaxColumns, "\t");

	if ( m_iColumnCount > 0 && m_iColumnCount != iCount )
		return -1;

	return iCount;
}

CSVRowData CSVFile::ReadRowContent(DWORD dwRowIndex)
{
	ADDTOCALLSTACK("CSVFile::ReadRowContent");
	TCHAR * ppRowContent[MAX_COLUMNS];
	int iQty = ReadRowContent(ppRowContent, dwRowIndex);
	
	CSVRowData csvRowData;

	if ( iQty != m_iColumnCount )
		return csvRowData;

	for (int i = 0; i < iQty; i++)
		csvRowData[m_pszColumnNames[i]] = ppRowContent[i];

	return csvRowData;
}
