#ifndef _INC_SQLITE_H
#define _INC_SQLITE_H
#pragma once

#include "../graycom.h"
#include "sqlite3.h"

typedef std::vector<TCHAR> stdvstring;
typedef std::vector<stdvstring> vstrlist;
typedef vstrlist row;

class TablePtr;

class CSQLite : public CScriptObj
{
public:
	static const char *m_sClassName;
	static const LPCTSTR sm_szLoadKeys[];
	static const LPCTSTR sm_szVerbKeys[];

	CSQLite();
	~CSQLite();

private:
	sqlite3 *m_socket;
	int m_resultCode;

public:
	CVarDefMap m_QueryResult;

public:
	void Connect(LPCTSTR pszFileName);
	void Close();

	TablePtr QueryPtr(LPCTSTR pszQuery);
	void Query(LPCTSTR pszQuery, CVarDefMap &mapQueryResult);
	void Exec(LPCTSTR pszQuery);

	virtual bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);
	virtual bool r_LoadVal(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);

	LPCTSTR GetName() const
	{
		return "SQLite_OBJ";
	}

private:
	void ConvertUTF8ToString(LPTSTR pszIn, stdvstring &pszOut);
};

class Table
{
	friend class CSQLite;

public:
	Table(void)
	{
		m_iRows = m_iCols = 0;
		m_iPos = -1;
	};
	virtual ~Table() { };

private:
	int m_iRows;
	int m_iCols;
	int m_iPos;

	row m_strlstCols;
	std::vector<row> m_lstRows;

public:
	bool GoRow(int iRow);
	LPCTSTR GetColName(int iCol);
	LPCTSTR GetColValue(int iCol);
};

// Class to contain a pointer to a Table class, which will spare the user from freeing a pointer
class TablePtr
{
public:
	TablePtr();
	TablePtr(Table *pTable);
	virtual ~TablePtr();

public:
	Table *m_pTable;
};

// Class for converting TCHAR to multi-byte UTF-8 and vice versa
class UTF8MBSTR
{
public:
	UTF8MBSTR(LPCTSTR pszArgs);
	UTF8MBSTR(UTF8MBSTR &pszArgs);
	virtual ~UTF8MBSTR();

private:
	size_t m_iLen;
	char *m_pszUTF8_MultiByte;

private:
	size_t ConvertStringToUTF8(LPCTSTR pszIn, char *&pszOut);

public:
	operator char *();
};

#endif	// _INC_SQLITE_H
