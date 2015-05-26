#include "SQLite.h"
#include "../../graysvr/graysvr.h"


CSQLite::CSQLite()
{
	m_sqlite3=0;
	m_iLastError=SQLITE_OK;
}

CSQLite::~CSQLite()
{
	Close();
}

int CSQLite::Open( LPCTSTR strFileName )
{
	Close();

	int iErr=sqlite3_open(UTF8MBSTR(strFileName), &m_sqlite3);

	if (iErr!=SQLITE_OK) m_iLastError=iErr;

	return iErr;
}

void CSQLite::Close()
{
	if (m_sqlite3)
	{
		sqlite3_close(m_sqlite3);
		m_sqlite3=0;
	}
}

bool CSQLite::IsOpen()
{
	if (this==0) return false;
	return m_sqlite3!=0;
}

int CSQLite::QuerySQL( LPCTSTR strSQL,  CVarDefMap & mapQueryResult )
{

	mapQueryResult.Empty();
	mapQueryResult.SetNumNew("NUMROWS", 0);

	TablePtr retTable = QuerySQLPtr(strSQL);

	if (retTable.m_pTable->GetRowCount())
	{
		int iRows = retTable.m_pTable->GetRowCount();
		int iCols = retTable.m_pTable->GetColCount();
		mapQueryResult.SetNum("NUMROWS", iRows);
		mapQueryResult.SetNum("NUMCOLS", iCols);

		char	*zStore = Str_GetTemp();
		for (int iRow=0; iRow<iRows; iRow++)
		{
			retTable.m_pTable->GoRow(iRow);
			for (int iCol=0; iCol<iCols; iCol++)
			{
				sprintf(zStore, "%d.%d", iRow, iCol);
				mapQueryResult.SetStr(zStore, true, retTable.m_pTable->GetValue(iCol));
				sprintf(zStore, "%d.%s", iRow, retTable.m_pTable->GetColName(iCol));
				mapQueryResult.SetStr(zStore, true, retTable.m_pTable->GetValue(iCol));
			}
		}
	}

	return GetLastError();
}

Table CSQLite::QuerySQL( LPCTSTR strSQL)
{

	if (!IsOpen()) {
		m_iLastError=SQLITE_ERROR;
		return Table();
	}

	char ** retStrings = 0;
	char * errmsg = 0;
	int iRows=0, iCols=0;

	int iErr=sqlite3_get_table(m_sqlite3, UTF8MBSTR(strSQL), &retStrings,
		&iRows, &iCols, &errmsg);

	if (iErr!=SQLITE_OK)
	{
		m_iLastError=iErr;
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "SQLite query \"%s\" failed. Error:%d \n", strSQL, iErr);
	}

	sqlite3_free(errmsg);

	Table retTable;

	if (iRows>0) retTable.m_iPos=0;

	retTable.m_iCols=iCols;
	retTable.m_iRows=iRows;
	retTable.m_strlstCols.reserve(iCols);

	int iPos=0;

	for (; iPos<iCols; iPos++)
	{
		retTable.m_strlstCols.push_back(stdvstring());

		if (retStrings[iPos])
			ConvertUTF8ToString( retStrings[iPos], retTable.m_strlstCols.back() );
		else retTable.m_strlstCols.back().push_back('\0');
	}

	retTable.m_lstRows.resize(iRows);
	for (int iRow=0; iRow<iRows; iRow++)
	{
		retTable.m_lstRows[iRow].reserve(iCols);
		for (int iCol=0; iCol<iCols; iCol++)
		{
			retTable.m_lstRows[iRow].push_back(stdvstring());
					
			if (retStrings[iPos])
				ConvertUTF8ToString( retStrings[iPos], retTable.m_lstRows[iRow].back() );
			else retTable.m_lstRows[iRow].back().push_back('\0');

			iPos++;
		}
	}

	sqlite3_free_table(retStrings);

	return retTable;
}

TablePtr CSQLite::QuerySQLPtr( LPCTSTR strSQL )
{
	if (!IsOpen()) {
		m_iLastError=SQLITE_ERROR;
		return 0;
	}

	char ** retStrings = 0;
	char * errmsg = 0;
	int iRows=0, iCols=0;

	int iErr=sqlite3_get_table(m_sqlite3, UTF8MBSTR(strSQL), &retStrings,
		&iRows, &iCols, &errmsg);

	if (iErr!=SQLITE_OK)
	{
		m_iLastError=iErr;
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "SQLite query \"%s\" failed. Error:%d \n", strSQL, iErr);
	}

	sqlite3_free(errmsg);

	Table * retTable = new Table();

	if (iRows>0) retTable->m_iPos=0;

	retTable->m_iCols=iCols;
	retTable->m_iRows=iRows;
	retTable->m_strlstCols.reserve(iCols);

	int iPos=0;

	for (; iPos<iCols; iPos++)
	{
		retTable->m_strlstCols.push_back(stdvstring());

		if (retStrings[iPos])
			ConvertUTF8ToString( retStrings[iPos], retTable->m_strlstCols.back() );
		else retTable->m_strlstCols.back().push_back('\0');
	}

	retTable->m_lstRows.resize(iRows);
	for (int iRow=0; iRow<iRows; iRow++)
	{
		retTable->m_lstRows[iRow].reserve(iCols);
		for (int iCol=0; iCol<iCols; iCol++)
		{
			retTable->m_lstRows[iRow].push_back(stdvstring());

			if (retStrings[iPos])
				ConvertUTF8ToString( retStrings[iPos], retTable->m_lstRows[iRow].back() );
			else retTable->m_lstRows[iRow].back().push_back('\0');

			iPos++;
		}
	}
	sqlite3_free_table(retStrings);

	return TablePtr(retTable);
}

void CSQLite::ConvertUTF8ToString( char * strInUTF8MB, stdvstring & strOut )
{
	int len=(int)strlen(strInUTF8MB)+1;
	strOut.resize(len, 0);
	wchar_t * wChar=new wchar_t[len];
	wChar[0]=0;
	mbstowcs(wChar,strInUTF8MB,len);
	wcstombs(&strOut[0],wChar,len);
	delete [] wChar;
}

int CSQLite::ExecuteSQL( LPCTSTR strSQL )
{
	if (!IsOpen()) {
		m_iLastError=SQLITE_ERROR;
		return SQLITE_ERROR;
	}

	char * errmsg = 0;

	int iErr = sqlite3_exec( m_sqlite3, UTF8MBSTR(strSQL), 0, 0, &errmsg );

	if (iErr!=SQLITE_OK)
	{
		m_iLastError=iErr;
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "SQLite query \"%s\" failed. Error:%d \n", strSQL, iErr);
	}

	sqlite3_free(errmsg);

	return iErr;
}

int CSQLite::IsSQLComplete( LPCTSTR strSQL )
{
	return sqlite3_complete( UTF8MBSTR(strSQL) );
}

int CSQLite::GetLastChangesCount()
{
	return sqlite3_changes(m_sqlite3);
}

sqlite_int64 CSQLite::GetLastInsertRowID()
{
	if (m_sqlite3==0) return 0; // RowID's starts with 1...

	return sqlite3_last_insert_rowid(m_sqlite3);
}

bool CSQLite::BeginTransaction()
{
	if (!IsOpen())
	{
		m_iLastError=SQLITE_ERROR; 
		return false;
	}
	m_iLastError = ExecuteSQL("BEGIN TRANSACTION");
	return m_iLastError==SQLITE_OK;
}

bool CSQLite::CommitTransaction()
{
	if (!IsOpen()) 
	{
		m_iLastError=SQLITE_ERROR; 
		return false;
	}
	m_iLastError = ExecuteSQL("COMMIT TRANSACTION");
	return m_iLastError==SQLITE_OK;
}

bool CSQLite::RollbackTransaction()
{
	if (!IsOpen()) 
	{
		m_iLastError=SQLITE_ERROR;
		return false;
	}
	m_iLastError = ExecuteSQL("ROLLBACK TRANSACTION");
	return m_iLastError==SQLITE_OK;
}

// CScriptObj functions

enum LDBO_TYPE
{
	LDBO_CONNECTED,
	LDBO_ROW,
	LDBO_QTY
};

LPCTSTR const CSQLite::sm_szLoadKeys[LDBO_QTY+1] =
{
	"CONNECTED",
	"ROW",
	NULL
};

enum LDBOV_TYPE
{
	LDBOV_CLOSE,
	LDBOV_CONNECT,
	LDBOV_EXECUTE,
	LDBOV_QUERY,
	LDBOV_QTY
};

LPCTSTR const CSQLite::sm_szVerbKeys[LDBOV_QTY+1] =
{
	"CLOSE",
	"CONNECT",
	"EXECUTE",
	"QUERY",
	NULL
};

bool CSQLite::r_GetRef(LPCTSTR & pszKey, CScriptObj * & pRef)
{
	ADDTOCALLSTACK("CSQLite::r_GetRef");
	UNREFERENCED_PARAMETER(pszKey);
	UNREFERENCED_PARAMETER(pRef);
	return false;
}

bool CSQLite::r_LoadVal(CScript & s)
{
	ADDTOCALLSTACK("CSQLite::r_LoadVal");
	UNREFERENCED_PARAMETER(s);
	return false;
}

bool CSQLite::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CSQLite::r_WriteVal");
	EXC_TRY("WriteVal");

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1);
	switch ( index )
	{
		case LDBO_CONNECTED:
			sVal.FormatVal(IsOpen());
			break;

		case LDBO_ROW:
		{
			pszKey += strlen(sm_szLoadKeys[index]);
			SKIP_SEPARATORS(pszKey);
			sVal = m_QueryResult.GetKeyStr(pszKey);
		} break;

		default:
			return false;
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("command '%s' [%p]\n", pszKey, static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return false;
}

bool CSQLite::r_Verb(CScript & s, CTextConsole * pSrc)
{
	ADDTOCALLSTACK("CSQLite::r_Verb");
	EXC_TRY("Verb");

	int index = FindTableSorted(s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys)-1);
	switch ( index )
	{
		case LDBOV_CLOSE:
			if ( IsOpen() )
				Close();
			break;

		case LDBOV_CONNECT:
			if ( IsOpen() )
				Close();
			Open(s.GetArgRaw());
			break;

		case LDBOV_EXECUTE:
			ExecuteSQL(s.GetArgRaw());
			break;

		case LDBOV_QUERY:
			QuerySQL(s.GetArgRaw(), m_QueryResult);
			break;

		default:
			return false;
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("command '%s' args '%s' [%p]\n", s.GetKey(), s.GetArgRaw(), static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return false;
}


// Table Class...


LPCTSTR Table::GetColName( int iCol )
{
	if (iCol>=0 && iCol<m_iCols)
	{
		return &m_strlstCols[iCol][0];
	}
	return 0;
}

bool Table::GoFirst()
{ 
	if (this==0) return false;
	if (m_lstRows.size()) 
	{
		m_iPos=0; 
		return true;
	}
	return false;
}

bool Table::GoLast()
{ 
	if (m_lstRows.size()) 
	{
		m_iPos=(int)m_lstRows.size()-1; 
		return true;
	}
	return false;
}

bool Table::GoNext()
{ 
	if (m_iPos+1<(int)m_lstRows.size()) 
	{
		m_iPos++; 
		return true;
	}
	return false;
}

bool Table::GoPrev()
{
	if (m_iPos>0)
	{
		m_iPos--;
		return true;
	}
	return false;
}

bool Table::GoRow(unsigned int iRow)
{
	if (this==0) return false;
	if (iRow<m_lstRows.size())
	{
		m_iPos=iRow;
		return true;
	}
	return false;
}

LPCTSTR Table::GetValue(LPCTSTR lpColName)
{
	if (!lpColName) return 0;
	if (m_iPos<0) return 0;
	for (int i=0; i<m_iCols; i++)
	{
		if (!strcmpi(&m_strlstCols[i][0],lpColName))
		{
			return &m_lstRows[m_iPos][i][0];
		}
	}
	return 0;
}

LPCTSTR Table::GetValue(int iColIndex)
{
	if (iColIndex<0 || iColIndex>=m_iCols) return 0;
	if (m_iPos<0) return 0;
	return &m_lstRows[m_iPos][iColIndex][0];
}

LPCTSTR Table::operator [] (LPCTSTR lpColName)
{
	if (!lpColName) return 0;
	if (m_iPos<0) return 0;
	for (int i=0; i<m_iCols; i++)
	{
		if (!strcmpi(&m_strlstCols[i][0],lpColName))
		{
			return &m_lstRows[m_iPos][i][0];
		}
	}
	return 0;
}

LPCTSTR Table::operator [] (int iColIndex)
{
	if (iColIndex<0 || iColIndex>=m_iCols) return 0;
	if (m_iPos<0) return 0;
	return &m_lstRows[m_iPos][iColIndex][0];
}

void Table::JoinTable(Table & tblJoin)
{
	if (m_iCols==0) {
		*this=tblJoin;
		return;
	}
	if (m_iCols!=tblJoin.m_iCols) return;

	if (tblJoin.m_iRows>0)
	{
		m_iRows+=tblJoin.m_iRows;

		for (std::vector<row>::iterator it=tblJoin.m_lstRows.begin();
			it!=tblJoin.m_lstRows.end(); it++)
		{
			m_lstRows.push_back(*it);
		}
	}
}

TablePtr::TablePtr( )
{ 
	m_pTable=0; 
}

TablePtr::TablePtr( Table * pTable )
{
	m_pTable = pTable; 
}

TablePtr::TablePtr( const TablePtr& cTablePtr )
{
	m_pTable=cTablePtr.m_pTable;
	((TablePtr *)&cTablePtr)->m_pTable=0;
}

TablePtr::~TablePtr()
{ 
	if (m_pTable) delete m_pTable; 
}

void TablePtr::operator =( const TablePtr& cTablePtr )
{
	if (m_pTable) delete m_pTable;
	m_pTable=cTablePtr.m_pTable;
	((TablePtr *)&cTablePtr)->m_pTable=0;
}

Table * TablePtr::Detach()
{
	Table * pTable=m_pTable;
	m_pTable=0;
	return pTable;
}

void TablePtr::Attach( Table * pTable )
{
	if (m_pTable) delete m_pTable;
		m_pTable=pTable;
}

void TablePtr::Destroy()
{
	if (m_pTable) delete m_pTable;
		m_pTable=0;
}

UTF8MBSTR::UTF8MBSTR()
{
	m_strUTF8_MultiByte=new char[1];
	m_strUTF8_MultiByte[0]=0;
	m_iLen=0;
}

UTF8MBSTR::UTF8MBSTR( LPCTSTR lpStr )
{
	if (lpStr)
		m_iLen=ConvertStringToUTF8(lpStr, m_strUTF8_MultiByte);
	else 
	{
		m_strUTF8_MultiByte=new char[1];
		m_strUTF8_MultiByte[0]=0;
		m_iLen=0;
	}
}

UTF8MBSTR::UTF8MBSTR( UTF8MBSTR& lpStr )
{
	m_iLen=lpStr.m_iLen;
	m_strUTF8_MultiByte=new char[m_iLen+1];
	strncpy(m_strUTF8_MultiByte, lpStr.m_strUTF8_MultiByte, m_iLen+1);
}

UTF8MBSTR::~UTF8MBSTR()
{
	if (m_strUTF8_MultiByte)
		delete [] m_strUTF8_MultiByte;
}

void UTF8MBSTR::operator =( LPCTSTR lpStr )
{
	if (m_strUTF8_MultiByte)
		delete [] m_strUTF8_MultiByte;

	if (lpStr)
		m_iLen=ConvertStringToUTF8(lpStr, m_strUTF8_MultiByte);
	else 
	{
		m_strUTF8_MultiByte=new char[1];
		m_strUTF8_MultiByte[0]=0;
		m_iLen=0;
	}
}

void UTF8MBSTR::operator =( UTF8MBSTR& lpStr )
{
	if (m_strUTF8_MultiByte)
		delete [] m_strUTF8_MultiByte;

	m_iLen=lpStr.m_iLen;
	m_strUTF8_MultiByte=new char[m_iLen+1];
	strncpy(m_strUTF8_MultiByte, lpStr.m_strUTF8_MultiByte, m_iLen+1);
}

UTF8MBSTR::operator char* ()
{
	return m_strUTF8_MultiByte;
}

UTF8MBSTR::operator stdstring ()
{
	TCHAR * strRet;
	ConvertUTF8ToString(m_strUTF8_MultiByte, m_iLen+1, strRet);
	stdstring cstrRet(strRet);
	delete [] strRet;

	return cstrRet;
}

size_t UTF8MBSTR::ConvertStringToUTF8( LPCTSTR strIn, char *& strOutUTF8MB )
{
	size_t len=strlen(strIn);
	wchar_t * wChar=new wchar_t[len+1];
	wChar[0]=0;
	mbstowcs(wChar,strIn,len+1);
	int iRequiredSize = static_cast<int>(wcstombs(NULL,wChar,len+1));
	strOutUTF8MB=new char[iRequiredSize+1];
	strOutUTF8MB[0]=0;
	wcstombs(strOutUTF8MB,wChar,iRequiredSize+1);
	delete [] wChar;
	return len;
}

void UTF8MBSTR::ConvertUTF8ToString( char * strInUTF8MB, size_t len, LPTSTR & strOut )
{
	strOut=new TCHAR[len];
	strOut[0]=0;
	wchar_t * wChar=new wchar_t[len];
	wChar[0]=0;
	mbstowcs(wChar,strInUTF8MB,len);
	wcstombs(strOut,wChar,len);
	delete [] wChar;
}

