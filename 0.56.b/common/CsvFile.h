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
	size_t m_iColumnCount;
	size_t m_iCurrentRow;

private:
	virtual bool OpenBase(void *pExtra);

public:
	CSVFile();
	~CSVFile();

private:
	CSVFile(const CSVFile& copy);
	CSVFile& operator=(const CSVFile& other);

public:
	size_t GetColumnCount() const { return m_iColumnCount; }
	size_t GetCurrentRow() const { return m_iCurrentRow; }

public:
	size_t ReadRowContent(TCHAR ** ppOutput, size_t row, int columns = MAX_COLUMNS);
	bool ReadRowContent(size_t row, CSVRowData& target);

	size_t ReadNextRowContent(TCHAR ** ppOutput);
	bool ReadNextRowContent(CSVRowData& target);
};

#endif
