#pragma once

#include "graycom.h"
#include "sqlite3.h"
#include <string>
#include <vector>
#include <tchar.h>
#include "CScriptObj.h"


//////////////////////////////////////////////////////////////////////////
// Typedefs
//////////////////////////////////////////////////////////////////////////

typedef std::basic_string<TCHAR> stdstring;
typedef std::vector<TCHAR> stdvstring; 
typedef std::vector<stdvstring> vstrlist;
typedef vstrlist row;

//////////////////////////////////////////////////////////////////////////
// Classes
//////////////////////////////////////////////////////////////////////////

class Table; // Forward declaration
class TablePtr; // Forward declaration

// Main wrapper
class CSQLite : public CScriptObj
{
public:
	static const char *m_sClassName;
	//	construction
	CSQLite();
	~CSQLite();

	int Open( LPCTSTR strFileName );
	void Close();
	bool IsOpen();


	sqlite3 * GetPtr(){ return m_sqlite3; };
	int GetLastError(){ return m_iLastError; };
	void ClearError() { m_iLastError=SQLITE_OK; };

	TablePtr QuerySQLPtr( LPCTSTR strSQL );
	Table QuerySQL( LPCTSTR strSQL );
	int QuerySQL( LPCTSTR strSQL,  CVarDefMap & mapQueryResult );
	int ExecuteSQL( LPCTSTR strSQL );
	int IsSQLComplete( LPCTSTR strSQL );

	int GetLastChangesCount();
	sqlite_int64 GetLastInsertRowID();


	bool BeginTransaction();
	bool CommitTransaction();
	bool RollbackTransaction();


	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual bool r_LoadVal( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc );

	LPCTSTR GetName() const
	{
		return "SQLite_OBJ";
	}

	CVarDefMap	m_QueryResult;
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];

private:
	sqlite3 * m_sqlite3;
	int m_iLastError;

	void ConvertUTF8ToString( char * strInUTF8MB, stdvstring & strOut );
};

// Table class...
class Table
{
	friend class CSQLite;
public:
	Table(void){ m_iRows=m_iCols=0;	m_iPos=-1; };
	virtual ~Table() {};
		
	// Gets the number of columns
	int GetColCount(){ if (this==0) return 0; return m_iCols; };

	// Gets the number of rows
	int GetRowCount(){ if (this==0) return 0; return m_iRows; };

	// Gets the current selected row. -1 when no rows exists.
	int GetCurRow(){ if (this==0) return -1; return m_iPos; };

	// Gets the column name at m_iCol index.
	// Returns null if the index is out of bounds.
	LPCTSTR GetColName( int iCol );

	void ResetRow(){ m_iPos = -1; };

	// Sets the 'iterator' to the first row
	// returns false if fails (no records)
	bool GoFirst();

	// Sets the 'iterator' to the last row
	// returns false if fails (no records)
	bool GoLast();

	// Sets the 'iterator' to the next row
	// returns false if fails (reached end)
	bool GoNext();

	// Sets the 'iterator' to the previous row
	// returns false if fails (reached beginning)
	bool GoPrev();

	// Sets the 'iterator' to the [iRow] row
	// returns false if fails (out of bounds)
	bool GoRow(unsigned int iRow);

	// Gets the value of lpColName column, in the current row
	// returns null if fails (no records)
	LPCTSTR GetValue(LPCTSTR lpColName);

	// Gets the value of iColIndex column, in the current row
	// returns null if fails (no records)
	LPCTSTR GetValue(int iColIndex);

	// Gets the value of lpColName column, in the current row
	// returns null if fails (no records)
	LPCTSTR operator [] (LPCTSTR lpColName);

	// Gets the value of iColIndex column, in the current row
	// returns null if fails (no records)
	LPCTSTR operator [] (int iColIndex);

	// Adds the rows from another table to this table
	// It is the caller's reponsibility to make sure
	// The two tables are matching
	void JoinTable(Table & tblJoin);

private:
	int m_iRows, m_iCols;

	row m_strlstCols;
	std::vector<row> m_lstRows;

	int m_iPos;
};

// A class to contain a pointer to a Table class,
// Which will spare the user from freeing a pointer.
class TablePtr
{
public:
	// Default constructor
	TablePtr( );

	// Construct from a Table*
	TablePtr( Table * pTable );

	// Copy constructor.
	// Will prevent the original TablePtr from deleting the table.
	// If you have a previous table connected to this class,
	//   you do not have to worry, 
	//   it will commit suicide before eating the new table.
	TablePtr( const TablePtr& cTablePtr );

	// Destructor...
	virtual ~TablePtr();

	// Copy operator.
	// Will prevent the original TablePtr from deleting the table.
	// If you have a previous table connected to this class,
	//   you do not have to worry, 
	//   it will commit suicide before eating the new table.
	void operator =( const TablePtr& cTablePtr );

	// Functor operator, will de-reference the m_pTable member.
	// WARNING: Use with care! Check for non-null m_pTable first!
	Table& operator()(){ return *m_pTable; };

	// bool operator, to check if m_pTable is valid.
	operator bool(){ return m_pTable!=0; };

	// Detaches the class from the Table,
	// and returns the Table that were just detached...
	Table * Detach();

	// Frees the current Table, and attaches the pTable.
	void Attach( Table * pTable );

	// Frees the current Table.
	void Destroy();

	// Pointer to the table.
	// I do not see any reason for encapsulating in Get/Set functions.
	Table * m_pTable;
};

// Class for converting TCHAR to Multi-Byte UTF-8
//   and vice versa
class UTF8MBSTR
{
public:
	UTF8MBSTR(void);
	UTF8MBSTR( LPCTSTR lpStr );
	UTF8MBSTR( UTF8MBSTR& lpStr );
	virtual ~UTF8MBSTR();

	void operator =( LPCTSTR lpStr );
	void operator =( UTF8MBSTR& lpStr );
	operator char* ();
	operator stdstring ();

private:
	char * m_strUTF8_MultiByte;
	size_t ConvertStringToUTF8( LPCTSTR strIn, char *& strOutUTF8MB );
	static void ConvertUTF8ToString( char * strInUTF8MB, size_t len, LPTSTR & strOut );

	size_t m_iLen;
};

