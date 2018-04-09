#ifndef _CSV_FILE_H
#define _CSV_FILE_H
#pragma once

#define FILE_MAX_COLUMNS	64	// maximum number of columns in a file

typedef std::map<std::string, std::string> CSVRowData;

class CSVFile : public CacheableScriptFile
{
public:
	CSVFile();
	~CSVFile();

private:
	CSVFile(const CSVFile &copy);
	CSVFile &operator=(const CSVFile &other);

private:
	TCHAR *m_pszColumnTypes[FILE_MAX_COLUMNS];
	TCHAR *m_pszColumnNames[FILE_MAX_COLUMNS];
	size_t m_iColumnCount;
	size_t m_iCurrentRow;

	virtual bool OpenBase(void *pExtra);

public:
	size_t GetColumnCount() const { return m_iColumnCount; }
	size_t GetCurrentRow() const { return m_iCurrentRow; }

	size_t ReadRowContent(TCHAR **ppOutput, size_t rowIndex, size_t columns = FILE_MAX_COLUMNS);
	bool ReadRowContent(size_t rowIndex, CSVRowData &target);

	size_t ReadNextRowContent(TCHAR **ppOutput);
	bool ReadNextRowContent(CSVRowData &target);
};

#endif
