#ifndef _CSV_FILE_H
#define _CSV_FILE_H

//
// CsvFile.h
//

#include "CFile.h"

#define MAX_COLUMNS	64	// maximum number of columns in a file

typedef std::map<std::string, std::string> CSVRowData;

class CSVFile : public CacheableScriptFile
{
private:
	TCHAR * m_pszColumnTypes[MAX_COLUMNS];
	TCHAR * m_pszColumnNames[MAX_COLUMNS];
	int m_iColumnCount;
	int m_iCurrentRow;

private:
	virtual bool OpenBase(void *pExtra);

public:
	CSVFile();
	~CSVFile();

private:
	CSVFile(const CSVFile& copy);
	CSVFile& operator=(const CSVFile& other);

public:
	int GetColumnCount();
	int ReadRowContent(char ** ppOutput, DWORD dwRowIndex = -1, int iMaxColumns = MAX_COLUMNS);
	CSVRowData ReadRowContent(DWORD dwRowIndex = -1);
};

#endif
