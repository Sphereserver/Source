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
CListDefContNum::CListDefContNum( LPCTSTR pszKey, INT64 iVal ) : CListDefContElem( pszKey ), m_iVal( iVal )
{
}

CListDefContNum::CListDefContNum( LPCTSTR pszKey ) : CListDefContElem( pszKey ), m_iVal( 0 )
{
}

CListDefContNum::~CListDefContNum()
{
}

INT64 CListDefContNum::GetValNum() const 
{ 
	return( m_iVal ); 
}

void CListDefContNum::SetValNum( INT64 iVal ) 
{ 
	m_iVal = iVal;
}

inline LPCTSTR CListDefContNum::GetValStr() const
{
	TemporaryString pszTmp;
	sprintf(pszTmp, "0%llx", m_iVal);
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
	sVal.FormatLLVal( GetValNum() );
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

inline INT64 CListDefContStr::GetValNum() const
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

CListDefContElem* CListDefCont::GetAt(size_t nIndex) const
{
	return ElementAt(nIndex);
}

bool CListDefCont::SetNumAt(size_t nIndex, INT64 iVal)
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

bool CListDefCont::SetStrAt(size_t nIndex, LPCTSTR pszVal)
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

inline CListDefCont::DefList::iterator CListDefCont::_GetAt(size_t nIndex)
{
	ADDTOCALLSTACK("CListDefCont::_GetAt");
	DefList::iterator it = m_listElements.begin();

	while ( it != m_listElements.end() )
	{
		if ( nIndex-- == 0 )
			return it;

		++it;
	}

	return it;
}

inline CListDefContElem* CListDefCont::ElementAt(size_t nIndex) const
{
	ADDTOCALLSTACK("CListDefCont::ElementAt");
	if ( nIndex >= m_listElements.size() )
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

LPCTSTR CListDefCont::GetValStr(size_t nIndex) const
{
	ADDTOCALLSTACK("CListDefCont::GetValStr");
	CListDefContElem* pElem = ElementAt(nIndex);

	if ( !pElem )
		return NULL;

	return pElem->GetValStr();
}

INT64 CListDefCont::GetValNum(size_t nIndex) const
{
	ADDTOCALLSTACK("CListDefCont::GetValNum");
	CListDefContElem* pElem = ElementAt(nIndex);

	if ( !pElem )
		return 0;

	return pElem->GetValNum();
}

int CListDefCont::FindValStr( LPCTSTR pVal, size_t nStartIndex /* = 0 */ ) const
{
	ADDTOCALLSTACK("CListDefCont::FindValStr");

	if ( !pVal || !(*pVal) )
		return -1;

	size_t nIndex = 0;
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
			return static_cast<int>(nIndex);
	}

	return -1;
}

int CListDefCont::FindValNum( INT64 iVal, size_t nStartIndex /* = 0 */ ) const
{
	ADDTOCALLSTACK("CListDefCont::FindValNum");

	DefList::const_iterator i;
	size_t nIndex = 0;

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
			return static_cast<int>(nIndex);
	}

	return -1;
}

bool CListDefCont::AddElementNum(INT64 iVal)
{
#undef max
	ADDTOCALLSTACK("CListDefCont::AddElementNum");
	if ( (m_listElements.size() + 1) >= std::numeric_limits<size_t>::max() )
		return false;

	m_listElements.push_back( new CListDefContNum(m_Key.GetPtr(), iVal) );

	return true;
}

bool CListDefCont::AddElementStr(LPCTSTR pszKey)
{
	ADDTOCALLSTACK("CListDefCont::AddElementStr");
	if ( (m_listElements.size() + 1) >= std::numeric_limits<size_t>::max() )
		return false;
#define max maximum

	REMOVE_QUOTES( pszKey );

	m_listElements.push_back( new CListDefContStr(m_Key.GetPtr(), pszKey) );

	return true;
}

inline void CListDefCont::DeleteAtIterator(DefList::iterator it)
{
	ADDTOCALLSTACK("CListDefCont::DeleteAtIterator");

	if ( it != m_listElements.end() )
	{
		CListDefContElem *pListBase = (*it);
		m_listElements.erase(it);

		if ( pListBase )
		{
			CListDefContNum *pListNum = dynamic_cast<CListDefContNum *>(pListBase);
			if ( pListNum )
				delete pListNum;
			else
			{
				CListDefContStr *pListStr = dynamic_cast<CListDefContStr *>(pListBase);
				if ( pListStr )
					delete pListStr;
			}
		}
	}
}

bool CListDefCont::RemoveElement(size_t nIndex)
{
	ADDTOCALLSTACK("CListDefCont::RemoveElement");
	if ( nIndex >= m_listElements.size() )
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
	ADDTOCALLSTACK("CListDefCont::RemoveAll");
	if ( !m_listElements.size() )
		return;

	DefList::iterator it = m_listElements.begin();

	while ( it != m_listElements.end() )
	{
		DeleteAtIterator(it);
		it = m_listElements.begin();
	}
}

bool compare_insensitive (CListDefContElem * firstelem, CListDefContElem * secondelem)
{
	LPCTSTR first = firstelem->GetValStr();
	LPCTSTR second = secondelem->GetValStr();

	if((IsSimpleNumberString(first)) && (IsSimpleNumberString(second)))
	{
		INT64 ifirst = firstelem->GetValNum();
		INT64 isecond = secondelem->GetValNum();
		return ( ifirst < isecond );
	}
	else
	{
		unsigned int i = 0;
		while ( (i < strlen(first)) && (i < strlen(second)))
		{
			if (tolower(first[i]) < tolower(second[i])) return true;
			else if (tolower(first[i]) > tolower(second[i])) return false;
			++i;
		}
		return ( strlen(first) < strlen(second) );
	}
}

bool compare_sensitive (CListDefContElem * firstelem, CListDefContElem * secondelem)
{
	LPCTSTR first = firstelem->GetValStr();
	LPCTSTR second = secondelem->GetValStr();

	if((IsSimpleNumberString(first)) && (IsSimpleNumberString(second)))
	{
		INT64 ifirst = firstelem->GetValNum();
		INT64 isecond = secondelem->GetValNum();
		return ( ifirst < isecond );
	}
	else
	{
		unsigned int i = 0;
		while ( (i < strlen(first)) && (i < strlen(second)))
		{
			if (first[i] < second[i]) return true;
			else if (first[i] > second[i]) return false;
			++i;
		}
		return ( strlen(first) < strlen(second) );
	}
}

void CListDefCont::Sort(bool bDesc, bool bCase)
{
	ADDTOCALLSTACK("CListDefCont::Sort");
	if ( !m_listElements.size() )
		return;

	if (bCase)
		m_listElements.sort(compare_insensitive);
	else
		m_listElements.sort(compare_sensitive);

	if (bDesc)
		m_listElements.reverse();
}

bool CListDefCont::InsertElementNum(size_t nIndex, INT64 iVal)
{
	ADDTOCALLSTACK("CListDefCont::InsertElementNum");
	if ( nIndex >= m_listElements.size() )
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

bool CListDefCont::InsertElementStr(size_t nIndex, LPCTSTR pszKey)
{
	ADDTOCALLSTACK("CListDefCont::InsertElementStr");
	if ( nIndex >= m_listElements.size() )
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
	ADDTOCALLSTACK("CListDefCont::CopySelf");
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
	ADDTOCALLSTACK("CListDefCont::PrintElements");
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
		else if ( pListElem )
			strElements += pListElem->GetValStr();

		strElements += ",";
	}

	strElements.SetAt(strElements.GetLength() - 1, '}');
}

void CListDefCont::DumpElements( CTextConsole * pSrc, LPCTSTR pszPrefix /* = NULL */ ) const
{
	ADDTOCALLSTACK("CListDefCont::DumpElements");
	CGString strResult;

	PrintElements(strResult);
	pSrc->SysMessagef("%s%s=%s\n", pszPrefix, m_Key.GetPtr(), static_cast<LPCTSTR>(strResult));
}

size_t CListDefCont::GetCount() const
{
	return m_listElements.size();
}

void CListDefCont::r_WriteSave( CScript& s )
{
	ADDTOCALLSTACK("CListDefCont::r_WriteSave");
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
		else if ( pListElem )
			s.WriteKey("ELEM", pListElem->GetValStr());
	}
}

bool CListDefCont::r_LoadVal( CScript& s )
{
	ADDTOCALLSTACK("CListDefCont::r_LoadVal");
	bool fQuoted = false;
	LPCTSTR pszArg = s.GetArgStr(&fQuoted);

	if ( fQuoted || !IsSimpleNumberString(pszArg) )
		return AddElementStr(pszArg);

	return AddElementNum(Exp_GetVal(pszArg));
}

bool CListDefCont::r_LoadVal( LPCTSTR pszArg )
{
	ADDTOCALLSTACK("CListDefCont::r_LoadVal");

	if (!IsSimpleNumberString(pszArg) )
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

CListDefCont * CListDefMap::GetAt( size_t at )
{
	ADDTOCALLSTACK("CListDefMap::GetAt");

	if ( at > m_Container.size() )
		return( NULL );

	DefSet::iterator i = m_Container.begin();
	while ( at-- )
		++i;

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

inline void CListDefMap::DeleteAt( size_t at )
{
	ADDTOCALLSTACK("CListDefMap::DeleteAt");

	if ( at > m_Container.size() )
		return;

	DefSet::iterator i = m_Container.begin();
	while ( at-- )
		++i;

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

	if ( !pArray || pArray == this )
		return;

	Empty();

	if ( pArray->GetCount() <= 0 )
		return;

	for ( DefSet::const_iterator i = pArray->m_Container.begin(); i != pArray->m_Container.end(); ++i )
	{
		m_Container.insert( (*i)->CopySelf() );
	}
}

size_t CListDefMap::GetCount() const
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
		DefSet::const_iterator i = m_Container.find(pListBase);
		delete pListBase;

		if ( i != m_Container.end() )
			pReturn = (*i);
	}

	return pReturn;
}

CListDefCont* CListDefMap::AddList(LPCTSTR pszKey)
{
	ADDTOCALLSTACK("CListDefMap::AddList");
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
	ADDTOCALLSTACK("CListDefMap::r_LoadVal");
	TCHAR* ppCmds[3];
	ppCmds[0] = const_cast<TCHAR*>(pszKey);
	Str_Parse(ppCmds[0], &(ppCmds[1]), "." );

	CListDefCont* pListBase = GetKey(ppCmds[0]);
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
			else if (( strcmpi(ppCmds[1], "set") == 0 ) || ( strcmpi(ppCmds[1], "append") == 0 ))
			{
				if ( !pszArg || !(*pszArg) )
					return false;

				if (( pListBase ) && ( strcmpi(ppCmds[1], "set") == 0 ))
				{
					DeleteKey(ppCmds[0]);
					pListBase = NULL;
				}
				
				if ( !pListBase )
				{
					pListBase = new CListDefCont(ppCmds[0]);
					m_Container.insert(pListBase);
				}

				TCHAR* ppCmd[2];
				ppCmd[0] = const_cast<TCHAR*>(pszArg);
				while ( Str_Parse( ppCmd[0], &(ppCmd[1]), "," ))
				{
					if ( IsSimpleNumberString(ppCmd[0]) )
						pListBase->AddElementNum(Exp_GetVal(ppCmd[0]));
					else
						pListBase->AddElementStr(ppCmd[0]);
					ppCmd[0] = ppCmd[1];
				}
				//insert last element
				if ( IsSimpleNumberString(ppCmd[0]) )
					return pListBase->AddElementNum(Exp_GetVal(ppCmd[0]));
				else
					return pListBase->AddElementStr(ppCmd[0]);
			}
			else if ( strcmpi(ppCmds[1], "sort") == 0 )
			{
				if ( !pListBase )
					return false;

				if ( pszArg && *pszArg )
				{
					if ( strcmpi(pszArg, "asc") == 0 )
						pListBase->Sort();
					else if (( strcmpi(pszArg, "i") == 0 ) || ( strcmpi(pszArg, "iasc") == 0 ))
						pListBase->Sort(false, true);
					else if ( strcmpi(pszArg, "desc") == 0 )
						pListBase->Sort(true);
					else if ( strcmpi(pszArg, "idesc") == 0 )
						pListBase->Sort(true, true);
					else
						return false;
					return true;
				}
				//default to asc if not specified.
				pListBase->Sort();
				return true;
			}
		}
		else if ( pListBase )
		{
			size_t nIndex = Exp_GetVal(ppCmds[1]);

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
		else
		{
			if ( ppCmds[2] && *(ppCmds[2]) )
			{
				if ( strcmpi(ppCmds[2], "insert") == 0 && pszArg && *pszArg )
				{
					pListBase = new CListDefCont(ppCmds[0]);
					m_Container.insert(pListBase);

					if ( IsSimpleNumberString(pszArg) )
						return pListBase->AddElementNum(Exp_GetVal(pszArg));
					else
						return pListBase->AddElementStr(pszArg);
				}
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
	ADDTOCALLSTACK("CListDefMap::r_Write");
	UNREFERENCED_PARAMETER(pSrc);
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
		nStartIndex = Exp_GetVal(ppCmds[1]);
		CListDefContElem *pListElem = pListBase->GetAt(nStartIndex);
		if ( pListElem )
		{
			if ( !(*(ppCmds[2])) )
			{
				CListDefContStr *pListElemStr = dynamic_cast<CListDefContStr*>(pListElem);
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
		strVal.Format("%" FMTSIZE_T, pListBase->GetCount());

		return true;
	}

	CScript s(nStartIndex == -1 ? ppCmds[1]:ppCmds[2]);
	nStartIndex = maximum(0, nStartIndex);

	if ( strcmpi(s.GetKey(), "findelem") == 0 )
	{
		bool fQuoted = false;
		LPCTSTR pszArg = s.GetArgStr(&fQuoted);

		if (( fQuoted ) || (! IsSimpleNumberString(pszArg) ))
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
