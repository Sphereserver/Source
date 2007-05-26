#include "../graysvr/graysvr.h"

static int GetIdentifierString( TCHAR * szTag, LPCTSTR pszArgs )
{
	// Copy the identifier (valid char set) out to this buffer.
	int i=0;
	for ( ;pszArgs[i]; i++ )
	{
		if ( ! _ISCSYM(pszArgs[i]))
			break;
		if ( i>=EXPRESSION_MAX_KEY_LEN )
			return( NULL );
		szTag[i] = pszArgs[i];
	}

	szTag[i] = '\0';
	return i;
}

/***************************************************************************
*
*
*	class CVarDefCont		Interface for variables
*
*
***************************************************************************/
CVarDefCont::CVarDefCont( LPCTSTR pszKey ) : m_Key( pszKey ) 
{ 
	m_Key.MakeLower(); 
}

CVarDefCont::~CVarDefCont()
{
}

LPCTSTR CVarDefCont::GetKey() const 
{ 
	return( m_Key.GetPtr() ); 
}

/***************************************************************************
*
*
*	class CVarDefContNum		Variable implementation (Number)
*
*
***************************************************************************/

CVarDefContNum::CVarDefContNum( LPCTSTR pszKey, int iVal ) : CVarDefCont( pszKey ), m_iVal( iVal )
{
}

CVarDefContNum::CVarDefContNum( LPCTSTR pszKey ) : CVarDefCont( pszKey ) 
{
}

CVarDefContNum::~CVarDefContNum()
{
}

int CVarDefContNum::GetValNum() const 
{ 
	return( m_iVal ); 
}

void CVarDefContNum::SetValNum( int iVal ) 
{ 
	m_iVal = iVal;
}

inline LPCTSTR CVarDefContNum::GetValStr() const
{
	TemporaryString pszTmp;
	sprintf(pszTmp, "0%x", m_iVal);
	return pszTmp;
}

bool CVarDefContNum::r_LoadVal( CScript & s )
{
	SetValNum( s.GetArgVal());
	return( true );
}

bool CVarDefContNum::r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc = NULL )
{
	UNREFERENCED_PARAMETER(pKey);
	UNREFERENCED_PARAMETER(pSrc);
	sVal.FormatVal( GetValNum() );
	return( true );
}

CVarDefCont * CVarDefContNum::CopySelf() const
{ 
	return new CVarDefContNum( GetKey(), m_iVal );
}

/***************************************************************************
*
*
*	class CVarDefContStr		Variable implementation (String)
*
*
***************************************************************************/

CVarDefContStr::CVarDefContStr( LPCTSTR pszKey, LPCTSTR pszVal ) : CVarDefCont( pszKey ), m_sVal( pszVal ) 
{
}

CVarDefContStr::CVarDefContStr( LPCTSTR pszKey ) : CVarDefCont( pszKey )
{
}

CVarDefContStr::~CVarDefContStr()
{
}

LPCTSTR CVarDefContStr::GetValStr() const 
{ 
	return( m_sVal ); 
}

inline int CVarDefContStr::GetValNum() const
{
	LPCTSTR pszStr = m_sVal;
	return( Exp_GetVal(pszStr) );
}

void CVarDefContStr::SetValStr( LPCTSTR pszVal ) 
{ 
	m_sVal.Copy( pszVal );
}


bool CVarDefContStr::r_LoadVal( CScript & s )
{
	SetValStr( s.GetArgStr());
	return( true );
}

bool CVarDefContStr::r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc = NULL )
{
	UNREFERENCED_PARAMETER(pKey);
	UNREFERENCED_PARAMETER(pSrc);
	sVal = GetValStr();
	return( true );
}

CVarDefCont * CVarDefContStr::CopySelf() const 
{ 
	return new CVarDefContStr( GetKey(), m_sVal ); 
}

/***************************************************************************
*
*
*	class CVarDefMap::CVarDefContTest	Variable implementation (search-only internal useage)
*
*
***************************************************************************/

CVarDefMap::CVarDefContTest::CVarDefContTest( LPCTSTR pszKey ) : CVarDefCont( pszKey )
{
}

CVarDefMap::CVarDefContTest::~CVarDefContTest()
{
}

LPCTSTR CVarDefMap::CVarDefContTest::GetValStr() const 
{ 
	return NULL; 
}
	
int CVarDefMap::CVarDefContTest::GetValNum() const 
{ 
	return -1; 
}

CVarDefCont * CVarDefMap::CVarDefContTest::CopySelf() const 
{ 
	return new CVarDefContTest( GetKey() ); 
}

/***************************************************************************
*
*
*	class CVarDefMap::ltstr			KEY part sorting wrapper over std::set
*
*
***************************************************************************/

bool CVarDefMap::ltstr::operator()(CVarDefCont * s1, CVarDefCont * s2) const
{
	return( strcmpi(s1->GetKey(), s2->GetKey()) < 0 );
}

/***************************************************************************
*
*
*	class CVarDefMap			Holds list of pairs KEY = VALUE and operates it
*
*
***************************************************************************/

CVarDefMap & CVarDefMap::operator = ( const CVarDefMap & array )
{
	Copy( &array );
	return( *this );
}

CVarDefMap::~CVarDefMap()
{
	Empty();
}

LPCTSTR CVarDefMap::FindValStr( LPCTSTR pVal ) const
{
	ADDTOCALLSTACK("CVarDefMap::FindValStr");
	for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); ++i )
	{
		const CVarDefCont * pVarBase = (*i);
		ASSERT( pVarBase );
		
		const CVarDefContStr * pVarStr = dynamic_cast <const CVarDefContStr *>( pVarBase );
		if ( pVarStr == NULL )
			continue;
		
		if ( ! strcmpi( pVal, pVarStr->GetValStr()))
			return( pVarBase->GetKey() );
	}

	return( NULL );
}

LPCTSTR CVarDefMap::FindValNum( int iVal ) const
{
	ADDTOCALLSTACK("CVarDefMap::FindValNum");
	for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); ++i )
	{
		const CVarDefCont * pVarBase = (*i);
		ASSERT( pVarBase );

		const CVarDefContNum * pVarNum = dynamic_cast <const CVarDefContNum *>( pVarBase );
		if ( pVarNum == NULL )
			continue;

		if ( pVarNum->GetValNum() == iVal )
			return( pVarBase->GetKey() );
	}

	return( NULL );
}

CVarDefCont * CVarDefMap::GetAt( int at )
{
	ADDTOCALLSTACK("CVarDefMap::GetAt");
	if ( at > m_Container.size() )
		return( NULL );

	DefSet::iterator i = m_Container.begin();
	while ( at-- ) { ++i; }

	if ( i != m_Container.end() )
		return( (*i) );
	else
		return( NULL );
}

CVarDefCont * CVarDefMap::GetAtKey( LPCTSTR at )
{
	ADDTOCALLSTACK("CVarDefMap::GetAtKey");
	CVarDefContTest * pVarBase = new CVarDefContTest(at);
	DefSet::iterator i = m_Container.find((CVarDefCont*)pVarBase);
	delete pVarBase;

	if ( i != m_Container.end() )
		return( (*i) );
	else
		return( NULL );
}

void CVarDefMap::DeleteAt( int at )
{
	ADDTOCALLSTACK("CVarDefMap::DeleteAt");
	if ( at > m_Container.size() )
		return;

	DefSet::iterator i = m_Container.begin();
	while ( at-- ) { ++i; }

	DeleteAtIterator(i);
}

void CVarDefMap::DeleteAtKey( LPCTSTR at )
{
	ADDTOCALLSTACK("CVarDefMap::DeleteAtKey");
	CVarDefContStr * pVarBased = new CVarDefContStr(at);
	DefSet::iterator i = m_Container.find((CVarDefCont*)pVarBased);
	delete pVarBased;

	DeleteAtIterator(i);
}

void CVarDefMap::DeleteAtIterator( DefSet::iterator it )
{
	ADDTOCALLSTACK("CVarDefMap::DeleteAtIterator");
	if ( it != m_Container.end() )
	{
		CVarDefCont * pVarBase = (*it);
		CVarDefContNum * pVarNum = NULL;
		CVarDefContStr * pVarStr = NULL;
		m_Container.erase(it);

		if ( pVarBase )
		{
			pVarNum = dynamic_cast<CVarDefContNum *>(pVarBase);
			if (pVarNum)
			{
				delete pVarNum;
			}
			else
			{
				pVarStr = dynamic_cast<CVarDefContStr *>(pVarBase);
				if ( pVarStr )
					delete pVarStr;
			}
		}
	}
}

void CVarDefMap::DeleteKey( LPCTSTR key )
{
	ADDTOCALLSTACK("CVarDefMap::DeleteKey");
	if ( key && *key)
	{
		DeleteAtKey(key);
	}
}

void CVarDefMap::Empty()
{
	ADDTOCALLSTACK("CVarDefMap::Empty");
	DefSet::iterator i = m_Container.begin();
	CVarDefCont * pVarBase = NULL;
	CVarDefContNum * pVarNum = NULL;
	CVarDefContStr * pVarStr = NULL;

	while ( i != m_Container.end() )
	{
		pVarBase = (*i);
		m_Container.erase(i); // This don't free all the resource
		i = m_Container.begin();

		if ( pVarBase )
		{
			pVarNum = dynamic_cast<CVarDefContNum *>(pVarBase);
			if (pVarNum)
			{
				delete pVarNum;
			}
			else
			{
				pVarStr = dynamic_cast<CVarDefContStr *>(pVarBase);
				if ( pVarStr )
					delete pVarStr;
			}
		}
	}

	m_Container.clear();
}

void CVarDefMap::Copy( const CVarDefMap * pArray )
{
	ADDTOCALLSTACK("CVarDefMap::Copy");
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

int CVarDefMap::GetCount() const
{
	ADDTOCALLSTACK("CVarDefMap::GetCount");
	return m_Container.size();
}

int CVarDefMap::SetNumNew( LPCTSTR pszName, int iVal )
{
	ADDTOCALLSTACK("CVarDefMap::SetNumNew");
	CVarDefCont * pVarNum = new CVarDefContNum( pszName, iVal );
	if ( !pVarNum )
		return( -1 );

	DefPairResult res = m_Container.insert(pVarNum);
	if ( res.second )
		return std::distance(m_Container.begin(), res.first);
	else
		return -1;
}

int CVarDefMap::SetNumOverride( LPCTSTR pszKey, int iVal )
{
	ADDTOCALLSTACK("CVarDefMap::SetNumOverride");
	DeleteAtKey(pszKey);
	return SetNumNew(pszKey,iVal);
}

int CVarDefMap::SetNum( LPCTSTR pszName, int iVal, bool fZero )
{
	ADDTOCALLSTACK("CVarDefMap::SetNum");
	ASSERT(pszName);

	if ( pszName[0] == '\0' )
		return( -1 );

	if ( fZero && (iVal == 0) )
	{
		DeleteAtKey(pszName);
		return( -1 );
	}

	CVarDefContTest * pVarSearch = new CVarDefContTest(pszName);
	DefSet::iterator iResult = m_Container.find((CVarDefCont*)pVarSearch);
	delete pVarSearch;

	CVarDefCont * pVarBase = NULL;
	if ( iResult != m_Container.end() )
		pVarBase = (*iResult);

	if ( !pVarBase )
	{
		return SetNumNew( pszName, iVal );
	}

	CVarDefContNum * pVarNum = dynamic_cast <CVarDefContNum *>( pVarBase );
	if ( pVarNum )
	{
		pVarNum->SetValNum( iVal );
	}
	else
	{
		if ( g_Serv.IsLoading() )
		{
			DEBUG_ERR(( "Replace existing VarStr '%s'\n", pVarBase->GetKey()));
		}
		return SetNumOverride( pszName, iVal );
	}

	return std::distance(m_Container.begin(), iResult);
}

int CVarDefMap::SetStrNew( LPCTSTR pszName, LPCTSTR pszVal )
{
	ADDTOCALLSTACK("CVarDefMap::SetStrNew");
	CVarDefCont * pVarStr = new CVarDefContStr( pszName, pszVal );
	if ( !pVarStr )
		return( -1 );

	DefPairResult res = m_Container.insert(pVarStr);
	if ( res.second )
		return std::distance(m_Container.begin(), res.first);
	else
		return -1;
}

int CVarDefMap::SetStrOverride( LPCTSTR pszKey, LPCTSTR pszVal )
{
	ADDTOCALLSTACK("CVarDefMap::SetStrOverride");
	DeleteAtKey(pszKey);
	return SetStrNew(pszKey,pszVal);
}

int CVarDefMap::SetStr( LPCTSTR pszName, bool fQuoted, LPCTSTR pszVal, bool fZero )
{
	ADDTOCALLSTACK("CVarDefMap::SetStr");
	// ASSUME: This has been clipped of unwanted beginning and trailing spaces.
	if ( !pszName || !pszName[0] )
		return -1;

	if ( pszVal == NULL || pszVal[0] == '\0' )	// but not if empty
	{
		DeleteAtKey(pszName);
		return( -1 );
	}

	if ( !fQuoted && IsSimpleNumberString(pszVal))
	{
		// Just store the number and not the string.
		return SetNum( pszName, Exp_GetVal( pszVal ), fZero);
	}

	CVarDefContTest * pVarSearch = new CVarDefContTest(pszName);
	DefSet::iterator iResult = m_Container.find((CVarDefCont*)pVarSearch);
	delete pVarSearch;

	CVarDefCont * pVarBase = NULL;
	if ( iResult != m_Container.end() )
		pVarBase = (*iResult);

	if ( !pVarBase )
	{
		return SetStrNew( pszName, pszVal );
	}

	CVarDefContStr * pVarStr = dynamic_cast <CVarDefContStr *>( pVarBase );
	if ( pVarStr )
	{
		pVarStr->SetValStr( pszVal );
	}
	else
	{
		if ( g_Serv.IsLoading())
		{
			DEBUG_ERR(( "Replace existing VarNum '%s' with %s\n", pVarBase->GetKey(), pszVal ));
		}
		return SetStrOverride( pszName, pszVal );
	}
	return std::distance(m_Container.begin(), iResult);
}

CVarDefCont * CVarDefMap::GetKey( LPCTSTR pszKey ) const
{
	ADDTOCALLSTACK("CVarDefMap::GetKey");
	CVarDefCont * pReturn = NULL;

	if ( pszKey )
	{
		CVarDefContTest * pVarBase = new CVarDefContTest(pszKey);
		DefSet::const_iterator i = m_Container.find((CVarDefCont*)pVarBase);
		delete pVarBase;
		
		if ( i != m_Container.end() )
			pReturn = (*i);
	}

	return( pReturn );
}

int CVarDefMap::GetKeyNum( LPCTSTR pszKey, bool fZero  ) const
{
	ADDTOCALLSTACK("CVarDefMap::GetKeyNum");
	CVarDefCont * pVar = GetKey(pszKey);
	if ( pVar == NULL )
		return (fZero ? 0 : NULL);
	return pVar->GetValNum();
}

LPCTSTR CVarDefMap::GetKeyStr( LPCTSTR pszKey, bool fZero  ) const
{
	ADDTOCALLSTACK("CVarDefMap::GetKeyStr");
	CVarDefCont * pVar = GetKey(pszKey);
	if ( pVar == NULL )
		return (fZero ? "0" : "");
	return pVar->GetValStr();
}

CVarDefCont * CVarDefMap::GetParseKey( LPCTSTR & pszArgs ) const
{
	ADDTOCALLSTACK("CVarDefMap::GetParseKey");
	// Skip to the end of the expression name.
	// The name can only be valid.

	TCHAR szTag[ EXPRESSION_MAX_KEY_LEN ];
	int i = GetIdentifierString( szTag, pszArgs );
	CVarDefCont * pVar = GetKey(szTag);
	if ( pVar )
	{
		pszArgs += i;
		return( pVar );
	}

	return NULL;
}

bool CVarDefMap::GetParseVal( LPCTSTR & pszArgs, long * plVal ) const
{
	ADDTOCALLSTACK("CVarDefMap::GetParseVal");
	CVarDefCont * pVarBase = GetParseKey( pszArgs );
	if ( pVarBase == NULL )
		return( false );
	*plVal = pVarBase->GetValNum();
	return( true );
}

void CVarDefMap::DumpKeys( CTextConsole * pSrc, LPCTSTR pszPrefix )
{
	ADDTOCALLSTACK("CVarDefMap::DumpKeys");
	// List out all the keys.
	ASSERT(pSrc);
	if ( pszPrefix == NULL )
		pszPrefix = "";

	for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); ++i )
	{
		const CVarDefCont * pVar = (*i);
		pSrc->SysMessagef( "%s%s=%s\n", (LPCTSTR) pszPrefix, (LPCTSTR) pVar->GetKey(), (LPCTSTR) pVar->GetValStr());
	}
}

void CVarDefMap::ClearKeys(LPCTSTR mask)
{
	ADDTOCALLSTACK("CVarDefMap::ClearKeys");
	if ( mask && *mask )
	{
		CGString sMask(mask);
		sMask.MakeLower();

		for ( DefSet::iterator i = m_Container.begin(); i != m_Container.end(); ++i )
		{
			CVarDefCont * pVar = (*i);
			if ( pVar )
			{
				if ( !strstr(pVar->GetKey(), sMask.GetPtr()) ) 
					continue;

				DeleteAtIterator(i);
				if ( i != m_Container.begin() )
				{
					--i;
				}
			}
		}	
	}
	else
	{
		Empty();
	}
}

bool CVarDefMap::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CVarDefMap::r_LoadVal");
	bool fQuoted = false;
	return( ( SetStr( s.GetKey(), fQuoted, s.GetArgStr( &fQuoted )) >= 0 ) ? true : false );
}

void CVarDefMap::r_WritePrefix( CScript & s, LPCTSTR pszPrefix, LPCTSTR pszKeyExclude )
{
	ADDTOCALLSTACK("CVarDefMap::r_WritePrefix");
	LPCTSTR		pszVal;
	bool bHasPrefix = (pszPrefix && *pszPrefix);
	bool bHasExclude = (pszKeyExclude && *pszKeyExclude);

	// Write with any prefix.
	for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); ++i )
	{
		const CVarDefCont * pVar = (*i);
		if ( !pVar )
		{
			// This should not happen, a warning maybe?
			continue;
		}

		if ( bHasExclude && !strcmpi(pszKeyExclude, pVar->GetKey()))
			continue;

		TemporaryString z;

		if ( bHasPrefix )
			sprintf(z, "%s.%s", pszPrefix, pVar->GetKey());
		else
			sprintf(z, "%s", pVar->GetKey());

		pszVal = pVar->GetValStr();
		const CVarDefContStr * pVarStr = dynamic_cast <const CVarDefContStr *>(pVar);
		if ( pVarStr ) // isspace(pszVal[0]) || isspace( pszVal[strlen(pszVal)-1] )
			s.WriteKeyFormat(z, "\"%s\"", pszVal);
		else
			s.WriteKey(z, pszVal);
	}
}