#include "../graysvr/graysvr.h"
#include <limits>

/***************************************************************************
*
*
*	class CListDefContElem		Interface for list element
*
*
***************************************************************************/
CListDefContElem::CListDefContElem( LPCTSTR pszKey )
: m_Key(pszKey)
{
	m_Key.MakeLower();
}

CListDefContElem::~CListDefContElem()
{
}

LPCTSTR CListDefContElem::GetKey() const 
{ 
	return( m_Key.GetPtr() ); 
}

void CListDefContElem::SetKey( LPCTSTR pszKey )
{
	m_Key = pszKey;
	m_Key.MakeLower(); 
}

/***************************************************************************
*
*
*	class CVarDefContNum		List element implementation (Number)
*
*
***************************************************************************/
CListDefContNum::CListDefContNum( LPCTSTR pszKey, int iVal ) : CListDefContElem( pszKey ), m_iVal( iVal )
{
}

CListDefContNum::CListDefContNum( LPCTSTR pszKey ) : CListDefContElem( pszKey ) 
{
}

CListDefContNum::~CListDefContNum()
{
}

int CListDefContNum::GetValNum() const 
{ 
	return( m_iVal ); 
}

void CListDefContNum::SetValNum( int iVal ) 
{ 
	m_iVal = iVal;
}

inline LPCTSTR CListDefContNum::GetValStr() const
{
	TemporaryString pszTmp;
	sprintf(pszTmp, "0%x", m_iVal);
	return pszTmp;
}

bool CListDefContNum::r_LoadVal( CScript & s )
{
	SetValNum( s.GetArgVal() );
	return( true );
}

bool CListDefContNum::r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc = NULL )
{
	UNREFERENCED_PARAMETER(pKey);
	UNREFERENCED_PARAMETER(pSrc);
	sVal.FormatVal( GetValNum() );
	return( true );
}

CListDefContElem * CListDefContNum::CopySelf() const
{ 
	return new CListDefContNum( GetKey(), m_iVal );
}

/***************************************************************************
*
*
*	class CListDefContStr		List element implementation (String)
*
*
***************************************************************************/
CListDefContStr::CListDefContStr( LPCTSTR pszKey, LPCTSTR pszVal ) : CListDefContElem( pszKey ), m_sVal( pszVal ) 
{
}

CListDefContStr::CListDefContStr( LPCTSTR pszKey ) : CListDefContElem( pszKey )
{
}

CListDefContStr::~CListDefContStr()
{
}

LPCTSTR CListDefContStr::GetValStr() const 
{ 
	return( m_sVal ); 
}

inline int CListDefContStr::GetValNum() const
{
	LPCTSTR pszStr = m_sVal;
	return( Exp_GetVal(pszStr) );
}

void CListDefContStr::SetValStr( LPCTSTR pszVal ) 
{ 
	m_sVal.Copy( pszVal );
}


bool CListDefContStr::r_LoadVal( CScript & s )
{
	SetValStr( s.GetArgStr());
	return( true );
}

bool CListDefContStr::r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc = NULL )
{
	UNREFERENCED_PARAMETER(pKey);
	UNREFERENCED_PARAMETER(pSrc);
	sVal = GetValStr();
	return( true );
}

CListDefContElem * CListDefContStr::CopySelf() const 
{ 
	return new CListDefContStr( GetKey(), m_sVal ); 
}

/***************************************************************************
*
*
*	class CListDefCont		List of elements (numeric & string)
*
*
***************************************************************************/
CListDefCont::CListDefCont( LPCTSTR pszKey ) : m_Key( pszKey ) 
{ 
	m_Key.MakeLower(); 
}

CListDefCont::~CListDefCont()
{
}

LPCTSTR CListDefCont::GetKey() const 
{ 
	return( m_Key.GetPtr() ); 
}

void CListDefCont::SetKey( LPCTSTR pszKey )
{ 
	m_Key = pszKey;
	m_Key.MakeLower(); 
}

CListDefContElem* CListDefCont::GetAt(int nIndex) const
{
	return ElementAt(nIndex);
}

bool CListDefCont::SetNumAt(int nIndex, int iVal)
{
	CListDefContElem* pListElem = ElementAt(nIndex);

	if ( !pListElem )
		return false;

	DefList::iterator it = _GetAt(nIndex);
	CListDefContElem* pListNewElem = new CListDefContNum(m_Key.GetPtr(), iVal);

	m_listElements.insert(it, pListNewElem);
	DeleteAtIterator(it);	

	return true;
}

bool CListDefCont::SetStrAt(int nIndex, LPCTSTR pszVal)
{
	CListDefContElem* pListElem = ElementAt(nIndex);

	if ( !pListElem )
		return false;

	DefList::iterator it = _GetAt(nIndex);
	CListDefContElem* pListNewElem = new CListDefContStr(m_Key.GetPtr(), pszVal);

	m_listElements.insert(it, pListNewElem);
	DeleteAtIterator(it);	

	return true;
}

inline CListDefCont::DefList::iterator CListDefCont::_GetAt(int nIndex)
{
	DefList::iterator it = m_listElements.begin();

	while ( it != m_listElements.end() )
	{
		if ( nIndex-- == 0 )
			return it;

		++it;
	}

	return it;
}

inline CListDefContElem* CListDefCont::ElementAt(int nIndex) const
{
	if ( nIndex < 0 || nIndex >= static_cast<int>(m_listElements.size()) )
		return NULL;

	DefList::const_iterator it = m_listElements.begin();

	while ( it != m_listElements.end() )
	{
		if ( nIndex-- == 0 )
			return (*it);

		++it;
	}

	return NULL;
}

LPCTSTR CListDefCont::GetValStr(int nIndex) const
{
	CListDefContElem* pElem = ElementAt(nIndex);

	if ( !pElem )
		return NULL;

	return pElem->GetValStr();
}

int CListDefCont::GetValNum(int nIndex) const
{
	CListDefContElem* pElem = ElementAt(nIndex);

	if ( !pElem )
		return 0;

	return pElem->GetValNum();
}

int CListDefCont::FindValStr( LPCTSTR pVal, int nStartIndex /* = 0 */ ) const
{
	ADDTOCALLSTACK("CListDefCont::FindValStr");

	if ( !pVal || !(*pVal) )
		return -1;

	int nIndex = 0;
	DefList::const_iterator i;

	for ( i = m_listElements.begin(), nIndex = 0; i != m_listElements.end(); ++i, ++nIndex )
	{
		if ( nIndex < nStartIndex )
			continue;

		const CListDefContElem * pListBase = (*i);
		ASSERT( pListBase );

		const CListDefContStr * pListStr = dynamic_cast <const CListDefContStr *>( pListBase );

		if ( pListStr == NULL )
			continue;

		if ( ! strcmpi( pVal, pListStr->GetValStr()))
			return nIndex;
	}

	return -1;
}

int CListDefCont::FindValNum( int iVal, int nStartIndex /* = 0 */ ) const
{
	ADDTOCALLSTACK("CListDefCont::FindValNum");

	DefList::const_iterator i;
	int nIndex = 0;

	for ( i = m_listElements.begin(), nIndex = 0; i != m_listElements.end(); ++i, ++nIndex )
	{
		if ( nIndex < nStartIndex )
			continue;

		const CListDefContElem * pListBase = (*i);
		ASSERT( pListBase );

		const CListDefContNum * pListNum = dynamic_cast <const CListDefContNum *>( pListBase );

		if ( pListNum == NULL )
			continue;

		if ( pListNum->GetValNum() == iVal )
			return nIndex;
	}

	return -1;
}

bool CListDefCont::AddElementNum(int iVal)
{
#undef max
	if ( m_listElements.size() + 1 >= std::numeric_limits<int>::max() )
		return false;

	m_listElements.push_back( new CListDefContNum(m_Key.GetPtr(), iVal) );

	return true;
}

bool CListDefCont::AddElementStr(LPCTSTR pszKey)
{
	if ( m_listElements.size() + 1 >= std::numeric_limits<int>::max() )
		return false;
#define max maximum

	if ( pszKey && *pszKey && pszKey[0] == '"' )
		++pszKey;

	m_listElements.push_back( new CListDefContStr(m_Key.GetPtr(), pszKey) );

	return true;
}

inline void CListDefCont::DeleteAtIterator(DefList::iterator it)
{
	ADDTOCALLSTACK("CListDefCont::DeleteAtIterator");

	if ( it != m_listElements.end() )
	{
		CListDefContElem *pListBase = (*it);
		CListDefContNum * pListNum = NULL;
		CListDefContStr * pListStr = NULL;

		m_listElements.erase(it);

		if ( pListBase )
		{
			pListNum = dynamic_cast<CListDefContNum *>(pListBase);

			if ( pListNum )
				delete pListNum;
			else
			{
				pListStr = dynamic_cast<CListDefContStr *>(pListBase);

				if ( pListStr )
					delete pListStr;
			}
		}
	}
}

bool CListDefCont::RemoveElement(int nIndex)
{
	if ( nIndex < 0 || nIndex >= static_cast<int>(m_listElements.size()) )
		return false;

	DefList::iterator it = m_listElements.begin();

	while ( it != m_listElements.end() )
	{
		if ( nIndex-- == 0 )
		{
			DeleteAtIterator(it);

			return true;
		}

		++it;
	}

	return false;
}

void CListDefCont::RemoveAll()
{
	if ( !m_listElements.size() )
		return;

	DefList::iterator it = m_listElements.begin();

	while ( it != m_listElements.end() )
	{
		DeleteAtIterator(it);
		it = m_listElements.begin();
	}
}

bool CListDefCont::InsertElementNum(int nIndex, int iVal)
{
	if ( nIndex < 0 || nIndex >= static_cast<int>(m_listElements.size()) )
		return false;

	DefList::iterator it = m_listElements.begin();

	while ( it != m_listElements.end() )
	{
		if ( nIndex-- == 0 )
		{
			m_listElements.insert(it, new CListDefContNum(m_Key.GetPtr(), iVal));

			return true;
		}

		++it;
	}

	return false;
}

bool CListDefCont::InsertElementStr(int nIndex, LPCTSTR pszKey)
{
	if ( nIndex < 0 || nIndex >= static_cast<int>(m_listElements.size()) )
		return false;

	DefList::iterator it = m_listElements.begin();

	while ( it != m_listElements.end() )
	{
		if ( nIndex-- == 0 )
		{
			m_listElements.insert(it, new CListDefContStr(m_Key.GetPtr(), pszKey));

			return true;
		}

		++it;
	}

	return false;	
}

CListDefCont* CListDefCont::CopySelf()
{
	CListDefCont* pNewList = new CListDefCont(m_Key.GetPtr());

	if ( pNewList )
	{
		if ( m_listElements.size() )
		{
			for ( DefList::const_iterator i = m_listElements.begin(); i != m_listElements.end(); ++i )
			{
				pNewList->m_listElements.push_back( (*i)->CopySelf() );
			}		
		}
	}

	return pNewList;
}

void CListDefCont::PrintElements(CGString& strElements) const
{
	if ( !m_listElements.size() )
	{
		strElements = "";
		return;
	}

	CListDefContElem *pListElem;
	CListDefContStr *pListElemStr;

	strElements = "{";

	for ( DefList::const_iterator i = m_listElements.begin(); i != m_listElements.end(); ++i )
	{
		pListElem = (*i);
		pListElemStr = dynamic_cast<CListDefContStr*>(pListElem);

		if ( pListElemStr )
		{
			strElements += "\"";
			strElements += pListElemStr->GetValStr();
			strElements += "\"";
		}
		else
			strElements += pListElem->GetValStr();

		strElements += ",";
	}

	strElements.SetAt(strElements.GetLength() - 1, '}');
}

void CListDefCont::DumpElements( CTextConsole * pSrc, LPCTSTR pszPrefix /* = NULL */ ) const
{
	CGString strResult;

	PrintElements(strResult);
	pSrc->SysMessagef("%s%s=%s\n", (LPCTSTR) pszPrefix, (LPCTSTR) m_Key.GetPtr(), strResult);
}

int CListDefCont::GetCount() const
{
	return static_cast<int>(m_listElements.size());
}

void CListDefCont::r_WriteSave( CScript& s )
{
	if ( !m_listElements.size() )
		return;

	CListDefContElem *pListElem;
	CListDefContStr *pListElemStr;
	CGString strElement;

	s.WriteSection("LIST %s", m_Key.GetPtr());

	for ( DefList::const_iterator i = m_listElements.begin(); i != m_listElements.end(); ++i )
	{
		pListElem = (*i);
		pListElemStr = dynamic_cast<CListDefContStr*>(pListElem);

		if ( pListElemStr )
		{
			strElement.Format("\"%s\"", pListElemStr->GetValStr());
			s.WriteKey("ELEM", strElement.GetPtr());
		}
		else
			s.WriteKey("ELEM", pListElem->GetValStr());
	}
}

bool CListDefCont::r_LoadVal( CScript& s )
{
	bool fQuoted = false;
	LPCTSTR pszArg = s.GetArgStr(&fQuoted);

	if ( fQuoted || !IsSimpleNumberString(pszArg) )
		return AddElementStr(pszArg);

	return AddElementNum(Exp_GetVal(pszArg));
}

/***************************************************************************
*
*
*	class CListDefMap::ltstr			KEY part sorting wrapper over std::set
*
*
***************************************************************************/

bool CListDefMap::ltstr::operator()(CListDefCont * s1, CListDefCont * s2) const
{
	return( strcmpi(s1->GetKey(), s2->GetKey()) < 0 );
}

/***************************************************************************
*
*
*	class CListDefMap			Holds list of pairs KEY = VALUE1... and operates it
*
*
***************************************************************************/

CListDefMap & CListDefMap::operator = ( const CListDefMap & array )
{
	Copy( &array );
	return( *this );
}

CListDefMap::~CListDefMap()
{
	Empty();
}

CListDefCont * CListDefMap::GetAt( int at )
{
	ADDTOCALLSTACK("CListDefMap::GetAt");

	if ( at > m_Container.size() )
		return( NULL );

	DefSet::iterator i = m_Container.begin();
	while ( at-- ) { ++i; }

	if ( i != m_Container.end() )
		return( (*i) );
	else
		return( NULL );
}

CListDefCont * CListDefMap::GetAtKey( LPCTSTR at )
{
	ADDTOCALLSTACK("CListDefMap::GetAtKey");

	CListDefCont * pListBase = new CListDefCont(at);
	DefSet::iterator i = m_Container.find(pListBase);
	delete pListBase;

	if ( i != m_Container.end() )
		return( (*i) );
	else
		return( NULL );
}

inline void CListDefMap::DeleteAt( int at )
{
	ADDTOCALLSTACK("CListDefMap::DeleteAt");

	if ( at > m_Container.size() )
		return;

	DefSet::iterator i = m_Container.begin();
	while ( at-- ) { ++i; }

	DeleteAtIterator(i);
}

inline void CListDefMap::DeleteAtKey( LPCTSTR at )
{
	ADDTOCALLSTACK("CListDefMap::DeleteAtKey");

	CListDefCont * pListBase = new CListDefCont(at);
	DefSet::iterator i = m_Container.find(pListBase);
	delete pListBase;

	DeleteAtIterator(i);
}

inline void CListDefMap::DeleteAtIterator( DefSet::iterator it )
{
	ADDTOCALLSTACK("CListDefMap::DeleteAtIterator");

	if ( it != m_Container.end() )
	{
		CListDefCont * pListBase = (*it);
		m_Container.erase(it);
		pListBase->RemoveAll();
		delete pListBase;
	}
}

void CListDefMap::DeleteKey( LPCTSTR key )
{
	ADDTOCALLSTACK("CListDefMap::DeleteKey");

	if ( key && *key)
	{
		DeleteAtKey(key);
	}
}

void CListDefMap::Empty()
{
	ADDTOCALLSTACK("CListDefMap::Empty");

	DefSet::iterator i = m_Container.begin();
	CListDefCont * pListBase = NULL;

	while ( i != m_Container.end() )
	{
		pListBase = (*i);
		m_Container.erase(i); // This don't free all the resource
		pListBase->RemoveAll();
		delete pListBase;
		i = m_Container.begin();
	}

	m_Container.clear();
}

void CListDefMap::Copy( const CListDefMap * pArray )
{
	ADDTOCALLSTACK("CListDefMap::Copy");

	if ( this == pArray )
		return;

	Empty();

	if ( !pArray->GetCount() )
		return;

	for ( DefSet::const_iterator i = pArray->m_Container.begin(); i != pArray->m_Container.end(); ++i )
	{
		m_Container.insert( (*i)->CopySelf() );
	}
}

int CListDefMap::GetCount() const
{
	ADDTOCALLSTACK("CListDefMap::GetCount");

	return m_Container.size();
}

CListDefCont* CListDefMap::GetKey( LPCTSTR pszKey ) const
{
	ADDTOCALLSTACK("CListDefMap::GetKey");

	CListDefCont * pReturn = NULL;

	if ( pszKey && *pszKey )
	{
		CListDefCont *pListBase = new CListDefCont(pszKey);
		DefSet::const_iterator i = m_Container.find((CListDefCont*)pListBase);
		delete pListBase;

		if ( i != m_Container.end() )
			pReturn = (*i);
	}

	return pReturn;
}

CListDefCont* CListDefMap::AddList(LPCTSTR pszKey)
{
	CListDefCont* pListBase = GetKey(pszKey);
	
	if ( !pListBase && pszKey && *pszKey )
	{
		pListBase = new CListDefCont(pszKey);
		m_Container.insert(pListBase);
	}

	return pListBase;
}

void CListDefMap::DumpKeys( CTextConsole * pSrc, LPCTSTR pszPrefix )
{
	ADDTOCALLSTACK("CListDefMap::DumpKeys");
	// List out all the keys.
	ASSERT(pSrc);

	if ( pszPrefix == NULL )
		pszPrefix = "";

	for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); ++i )
	{
		(*i)->DumpElements(pSrc, pszPrefix);
	}
}

void CListDefMap::ClearKeys(LPCTSTR mask)
{
	ADDTOCALLSTACK("CListDefMap::ClearKeys");

	if ( mask && *mask )
	{
		if ( !m_Container.size() )
			return;

		CGString sMask(mask);
		sMask.MakeLower();

		DefSet::iterator i = m_Container.begin();
		CListDefCont * pListBase = NULL;

		while ( i != m_Container.end() )
		{
			pListBase = NULL;
			pListBase = (*i);

			if ( pListBase && ( strstr(pListBase->GetKey(), sMask.GetPtr()) ) )
			{
				DeleteAtIterator(i);
				i = m_Container.begin();
			}
			else
			{
				++i;
			}
		}
	}
	else
	{
		Empty();
	}
}

bool CListDefMap::r_LoadVal( LPCTSTR pszKey, CScript & s )
{
	TCHAR* ppCmds[3];
	ppCmds[0] = const_cast<TCHAR*>(pszKey);
	Str_Parse(ppCmds[0], &(ppCmds[1]), "." );

	CListDefCont* pListBase = GetKey(ppCmds[0]);
	CListDefContElem* pListElem;
	LPCTSTR pszArg = s.GetArgRaw();

	if ( ppCmds[1] && (*(ppCmds[1])) ) // LIST.<list_name>.<something...>
	{
		Str_Parse(ppCmds[1], &(ppCmds[2]), "." );

		if ( !IsSimpleNumberString(ppCmds[1]) )
		{
			if ( strcmpi(ppCmds[1], "clear") == 0 )
			{
				if ( pListBase )
					DeleteKey(ppCmds[0]);

				return true;
			}
			else if ( strcmpi(ppCmds[1], "add") == 0 )
			{
				if ( !pszArg || !(*pszArg) )
					return false;

				if ( !pListBase )
				{
					pListBase = new CListDefCont(ppCmds[0]);
					m_Container.insert(pListBase);
				}

				if ( IsSimpleNumberString(pszArg) )
					return pListBase->AddElementNum(Exp_GetVal(pszArg));
				else
					return pListBase->AddElementStr(pszArg);
			}
		}
		else if ( pListBase )
		{
			int nIndex = Exp_GetVal(ppCmds[1]);

			if ( ppCmds[2] && *(ppCmds[2]) )
			{
				if ( strcmpi(ppCmds[2], "remove") == 0 )
					return pListBase->RemoveElement(nIndex);
				else if ( strcmpi(ppCmds[2], "insert") == 0 && pszArg && *pszArg )
				{
					bool bIsNum = ( IsSimpleNumberString(pszArg) );

					if ( nIndex >= pListBase->GetCount() )
					{
						if ( bIsNum )
							return pListBase->AddElementNum(Exp_GetVal(pszArg));
						else
							return pListBase->AddElementStr(pszArg);
					}

					CListDefContElem* pListElem = pListBase->GetAt(nIndex);

					if ( !pListElem )
						return false;

					if ( bIsNum )
						return pListBase->InsertElementNum(nIndex, Exp_GetVal(pszArg));
					else
						return pListBase->InsertElementStr(nIndex, pszArg);
				}
			}
			else if ( pszArg && *pszArg )
			{
				CListDefContElem* pListElem = pListBase->GetAt(nIndex);

				if ( !pListElem )
					return false;

				if ( IsSimpleNumberString(pszArg) )
					return pListBase->SetNumAt(nIndex, Exp_GetVal(pszArg));
				else
					return pListBase->SetStrAt(nIndex, pszArg);
			}
		}
	}
	else if ( pszArg && *pszArg )
	{
		if ( pListBase )
			pListBase->RemoveAll();
		else
		{
			pListBase = new CListDefCont(ppCmds[0]);
			m_Container.insert(pListBase);
		}

		if ( IsSimpleNumberString(pszArg) )
			return pListBase->AddElementNum(Exp_GetVal(pszArg));
		else
			return pListBase->AddElementStr(pszArg);
	}
	else if ( pListBase )
	{
		DeleteKey(ppCmds[0]);

		return true;
	}

	return false;
}

bool CListDefMap::r_Write( CTextConsole *pSrc, LPCTSTR pszString, CGString& strVal )
{
	TCHAR * ppCmds[3];
	ppCmds[0] = const_cast<TCHAR*>(pszString);
	Str_Parse(ppCmds[0], &(ppCmds[1]), "." );

	CListDefCont* pListBase = GetKey(ppCmds[0]);

	if ( !pListBase )
		return false;

	if ( !ppCmds[1] || !(*(ppCmds[1])) ) // LIST.<list_name>
	{
		pListBase->PrintElements(strVal);

		return true;
	}

	// LIST.<list_name>.<list_elem_index>

	int nStartIndex = -1;
	Str_Parse(ppCmds[1], &(ppCmds[2]), "." );

	if ( IsSimpleNumberString(ppCmds[1]) )
	{
		CListDefContElem* pListElem = NULL;
		CListDefContStr* pListElemStr = NULL;

		nStartIndex = Exp_GetVal(ppCmds[1]);
		pListElem = pListBase->GetAt(nStartIndex);

		if ( pListElem )
		{
			if ( !(*(ppCmds[2])) )
			{
				pListElemStr = dynamic_cast<CListDefContStr*>(pListElem);

				if ( pListElemStr )
					strVal.Format("\"%s\"", pListElemStr->GetValStr());
				else
					strVal = pListElem->GetValStr();

				return true;
			}
		}
		else
			return false;
	}
	else if ( strcmpi(ppCmds[1], "count") == 0 )
	{
		strVal.Format("%d", pListBase->GetCount());

		return true;
	}

	CScript s(nStartIndex == -1 ? ppCmds[1]:ppCmds[2]);
	nStartIndex = max(0, nStartIndex);

	if ( strcmpi(s.GetKey(), "findelem") == 0 )
	{
		bool fQuoted = false;
		LPCTSTR pszArg = s.GetArgStr(&fQuoted);

		if ( fQuoted )
			strVal.Format("%d", pListBase->FindValStr(pszArg, nStartIndex));
		else
			strVal.Format("%d", pListBase->FindValNum(Exp_GetVal(pszArg), nStartIndex));

		return true;
	}

	return false;
}


void CListDefMap::r_WriteSave( CScript& s )
{
	ADDTOCALLSTACK("CListDefMap::r_WriteSave");

	for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); ++i )
	{
		(*i)->r_WriteSave(s);
	}
}
