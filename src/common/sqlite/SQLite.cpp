#include "SQLite.h"
#include "../../graysvr/graysvr.h"

CSQLite::CSQLite()
{
	m_socket = NULL;
	m_resultCode = SQLITE_OK;
}

CSQLite::~CSQLite()
{
	if ( m_socket )
		Close();
}

void CSQLite::Connect(LPCTSTR pszFileName)
{
	ADDTOCALLSTACK("CSQLite::Connect");
	if ( m_socket )
		return;

	m_resultCode = sqlite3_open(UTF8MBSTR(pszFileName), &m_socket);
}

void CSQLite::Close()
{
	ADDTOCALLSTACK("CSQLite::Close");
	sqlite3_close(m_socket);
	m_socket = NULL;
}

TablePtr CSQLite::QueryPtr(LPCTSTR pszQuery)
{
	ADDTOCALLSTACK("CSQLite::QueryPtr");
	char **retStrings = 0;
	char *errmsg = 0;
	int iRows = 0, iCols = 0;

	m_resultCode = sqlite3_get_table(m_socket, UTF8MBSTR(pszQuery), &retStrings, &iRows, &iCols, &errmsg);
	if ( m_resultCode != SQLITE_OK )
		g_Log.EventError("SQLite error #%d: %s [Cmd: \"%s\"]\n", m_resultCode, errmsg, pszQuery);

	sqlite3_free(errmsg);

	Table *pTable = new Table();
	pTable->m_iCols = iCols;
	pTable->m_iRows = iRows;
	if ( iRows > 0 )
		pTable->m_iPos = 0;
	pTable->m_strlstCols.reserve(iCols);

	int iPos = 0;
	for ( ; iPos < iCols; ++iPos )
	{
		pTable->m_strlstCols.push_back(stdvstring());

		if ( retStrings[iPos] )
			ConvertUTF8ToString(retStrings[iPos], pTable->m_strlstCols.back());
		else
			pTable->m_strlstCols.back().push_back('\0');
	}

	pTable->m_lstRows.resize(iRows);
	for ( int iRow = 0; iRow < iRows; ++iRow )
	{
		pTable->m_lstRows[iRow].reserve(iCols);
		for ( int iCol = 0; iCol < iCols; ++iCol, ++iPos )
		{
			pTable->m_lstRows[iRow].push_back(stdvstring());

			if ( retStrings[iPos] )
				ConvertUTF8ToString(retStrings[iPos], pTable->m_lstRows[iRow].back());
			else
				pTable->m_lstRows[iRow].back().push_back('\0');
		}
	}

	sqlite3_free_table(retStrings);
	return TablePtr(pTable);
}

void CSQLite::Query(LPCTSTR pszQuery, CVarDefMap &mapQueryResult)
{
	ADDTOCALLSTACK("CSQLite::Query");
	mapQueryResult.Empty();
	mapQueryResult.SetNumNew("NUMROWS", 0);

	if ( !m_socket )
		return;

	TablePtr pTable = QueryPtr(pszQuery);
	if ( pTable.m_pTable->m_iRows )
	{
		int iRows = pTable.m_pTable->m_iRows;
		int iCols = pTable.m_pTable->m_iCols;
		mapQueryResult.SetNum("NUMROWS", iRows);
		mapQueryResult.SetNum("NUMCOLS", iCols);

		char *pszKey = Str_GetTemp();
		for ( int iRow = 0; iRow < iRows; ++iRow )
		{
			pTable.m_pTable->GoRow(iRow);
			for ( int iCol = 0; iCol < iCols; ++iCol )
			{
				sprintf(pszKey, "%d.%d", iRow, iCol);
				mapQueryResult.SetStr(pszKey, true, pTable.m_pTable->GetColValue(iCol));
				sprintf(pszKey, "%d.%s", iRow, pTable.m_pTable->GetColName(iCol));
				mapQueryResult.SetStr(pszKey, true, pTable.m_pTable->GetColValue(iCol));
			}
		}
	}
}

void CSQLite::Exec(LPCTSTR pszQuery)
{
	ADDTOCALLSTACK("CSQLite::Exec");
	if ( !m_socket )
		return;

	char *errmsg = 0;

	m_resultCode = sqlite3_exec(m_socket, UTF8MBSTR(pszQuery), 0, 0, &errmsg);
	if ( m_resultCode != SQLITE_OK )
		g_Log.EventError("SQLite error #%d: %s [Cmd: \"%s\"]\n", m_resultCode, errmsg, pszQuery);

	sqlite3_free(errmsg);
}

void CSQLite::ConvertUTF8ToString(LPTSTR pszIn, stdvstring &pszOut)
{
	ADDTOCALLSTACK("CSQLite::ConvertUTF8ToString");
	size_t len = strlen(pszIn) + 1;
	pszOut.resize(len, 0);

	wchar_t *wChar = new wchar_t[len];
	wChar[0] = 0;

	mbstowcs(wChar, pszIn, len);
	wcstombs(&pszOut[0], wChar, len);

	delete[] wChar;
}

enum LDBO_TYPE
{
	LDBO_CONNECTED,
	LDBO_ROW,
	LDBO_QTY
};

LPCTSTR const CSQLite::sm_szLoadKeys[LDBO_QTY + 1] =
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

LPCTSTR const CSQLite::sm_szVerbKeys[LDBOV_QTY + 1] =
{
	"CLOSE",
	"CONNECT",
	"EXECUTE",
	"QUERY",
	NULL
};

bool CSQLite::r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef)
{
	ADDTOCALLSTACK("CSQLite::r_GetRef");
	UNREFERENCED_PARAMETER(pszKey);
	UNREFERENCED_PARAMETER(pRef);
	return false;
}

bool CSQLite::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CSQLite::r_LoadVal");
	UNREFERENCED_PARAMETER(s);
	return false;
}

bool CSQLite::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CSQLite::r_WriteVal");
	EXC_TRY("WriteVal");

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	switch ( index )
	{
		case LDBO_CONNECTED:
			sVal.FormatVal(m_socket != NULL);
			break;

		case LDBO_ROW:
			pszKey += strlen(sm_szLoadKeys[index]);
			SKIP_SEPARATORS(pszKey);
			sVal = m_QueryResult.GetKeyStr(pszKey);
			break;

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

bool CSQLite::r_Verb(CScript &s, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CSQLite::r_Verb");
	EXC_TRY("Verb");

	int index = FindTableSorted(s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1);
	switch ( index )
	{
		case LDBOV_CLOSE:
			if ( m_socket )
				Close();
			break;

		case LDBOV_CONNECT:
			if ( !m_socket )
				Connect(s.GetArgRaw());
			break;

		case LDBOV_EXECUTE:
			Exec(s.GetArgRaw());
			break;

		case LDBOV_QUERY:
			Query(s.GetArgRaw(), m_QueryResult);
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

///////////////////////////////////////////////////////////
// Table

bool Table::GoRow(int iRow)
{
	ADDTOCALLSTACK("Table::GoRow");
	if ( iRow < static_cast<int>(m_lstRows.size()) )
	{
		m_iPos = iRow;
		return true;
	}
	return false;
}

LPCTSTR Table::GetColName(int iCol)
{
	ADDTOCALLSTACK("Table::GetColName");
	if ( (iCol >= 0) && (iCol < m_iCols) )
		return &m_strlstCols[iCol][0];
	return 0;
}

LPCTSTR Table::GetColValue(int iCol)
{
	ADDTOCALLSTACK("Table::GetColValue");
	if ( (iCol >= 0) && (iCol < m_iCols) && (m_iPos >= 0) )
		return &m_lstRows[m_iPos][iCol][0];
	return 0;
}

///////////////////////////////////////////////////////////
// TablePtr

TablePtr::TablePtr()
{
	m_pTable = NULL;
}

TablePtr::TablePtr(Table *pTable)
{
	m_pTable = pTable;
}

TablePtr::~TablePtr()
{
	if ( m_pTable )
		delete m_pTable;
}

///////////////////////////////////////////////////////////
// UTF8MBSTR

UTF8MBSTR::UTF8MBSTR(LPCTSTR pszArgs)
{
	if ( pszArgs )
		m_iLen = ConvertStringToUTF8(pszArgs, m_pszUTF8_MultiByte);
	else
	{
		m_pszUTF8_MultiByte = new char[1];
		m_pszUTF8_MultiByte[0] = 0;
		m_iLen = 0;
	}
}

UTF8MBSTR::UTF8MBSTR(UTF8MBSTR &pszArgs)
{
	m_iLen = pszArgs.m_iLen;
	m_pszUTF8_MultiByte = new char[m_iLen + 1];
	strncpy(m_pszUTF8_MultiByte, pszArgs.m_pszUTF8_MultiByte, m_iLen + 1);
}

UTF8MBSTR::~UTF8MBSTR()
{
	if ( m_pszUTF8_MultiByte )
		delete[] m_pszUTF8_MultiByte;
}

size_t UTF8MBSTR::ConvertStringToUTF8(LPCTSTR pszIn, char *&pszOut)
{
	ADDTOCALLSTACK("UTF8MBSTR::ConvertStringToUTF8");
	size_t len = strlen(pszIn);
	wchar_t *wChar = new wchar_t[len + 1];
	wChar[0] = 0;
	mbstowcs(wChar, pszIn, len + 1);

	size_t lenRequired = wcstombs(NULL, wChar, len + 1);
	pszOut = new char[lenRequired + 1];
	pszOut[0] = 0;
	wcstombs(pszOut, wChar, lenRequired + 1);

	delete[] wChar;
	return len;
}

void UTF8MBSTR::ConvertUTF8ToString(LPCTSTR pszIn, char *&pszOut, size_t len)
{
	ADDTOCALLSTACK("UTF8MBSTR::ConvertUTF8ToString");
	wchar_t *wChar = new wchar_t[len];
	wChar[0] = 0;
	mbstowcs(wChar, pszIn, len);

	pszOut = new char[len];
	pszOut[0] = 0;
	wcstombs(pszOut, wChar, len);

	delete[] wChar;
}

UTF8MBSTR::operator char *()
{
	return m_pszUTF8_MultiByte;
}
