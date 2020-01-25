#include "../graysvr/graysvr.h"

static size_t GetIdentifierString( TCHAR * szTag, LPCTSTR pszArgs )
{
	// Copy the identifier (valid char set) out to this buffer.
	size_t i = 0;
	for ( ;pszArgs[i]; i++ )
	{
		if ( ! _ISCSYM(pszArgs[i]))
			break;
		if ( i >= EXPRESSION_MAX_KEY_LEN )
			return 0;
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

void CVarDefCont::SetKey( LPCTSTR pszKey )
{ 
	m_Key = pszKey;
	m_Key.MakeLower(); 
}

/***************************************************************************
*
*
*	class CVarDefContNum		Variable implementation (Number)
*
*
***************************************************************************/

CVarDefContNum::CVarDefContNum( LPCTSTR pszKey, INT64 iVal ) : CVarDefCont( pszKey ), m_iVal( iVal )
{
}

CVarDefContNum::CVarDefContNum( LPCTSTR pszKey ) : CVarDefCont( pszKey ), m_iVal( 0 )
{
}

CVarDefContNum::~CVarDefContNum()
{
}

INT64 CVarDefContNum::GetValNum() const 
{ 
	return( m_iVal ); 
}

void CVarDefContNum::SetValNum( INT64 iVal ) 
{ 
	m_iVal = iVal;
}

inline LPCTSTR CVarDefContNum::GetValStr() const
{
	TemporaryString pszTmp;
	sprintf(pszTmp, "0%llx", m_iVal);
	return pszTmp;
}

bool CVarDefContNum::r_LoadVal( CScript & s )
{
	SetValNum(s.GetArgLLVal());
	return( true );
}

bool CVarDefContNum::r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc = NULL )
{
	UNREFERENCED_PARAMETER(pKey);
	UNREFERENCED_PARAMETER(pSrc);
	sVal.FormatLLVal( GetValNum() );
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
	return m_sVal.GetPtr();
}

inline INT64 CVarDefContStr::GetValNum() const
{
	LPCTSTR pszStr = m_sVal;
	return Exp_GetLLVal(pszStr);
}

void CVarDefContStr::SetValStr( LPCTSTR pszVal ) 
{
	if (strlen(pszVal) <= SCRIPT_MAX_LINE_LEN/2)
		m_sVal.Copy( pszVal );
	else
		g_Log.EventWarn("Setting max length of %d was exceeded on (VAR,TAG,LOCAL).%s \r", SCRIPT_MAX_LINE_LEN/2, GetKey() );
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
	
INT64 CVarDefMap::CVarDefContTest::GetValNum() const 
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

LPCTSTR CVarDefMap::FindValNum( INT64 iVal ) const
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

CVarDefCont * CVarDefMap::GetAt( size_t at ) const
{
	ADDTOCALLSTACK("CVarDefMap::GetAt");
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

CVarDefCont * CVarDefMap::GetAtKey( LPCTSTR at ) const
{
	ADDTOCALLSTACK("CVarDefMap::GetAtKey");
	CVarDefContTest * pVarBase = new CVarDefContTest(at);
	DefSet::iterator i = m_Container.find(pVarBase);
	delete pVarBase;

	if ( i != m_Container.end() )
		return( (*i) );
	else
		return( NULL );
}

void CVarDefMap::DeleteAt( size_t at )
{
	ADDTOCALLSTACK("CVarDefMap::DeleteAt");
	if ( at > m_Container.size() )
		return;

	DefSet::iterator i = m_Container.begin();
	while ( at-- ) 
		++i;

	DeleteAtIterator(i);
}

void CVarDefMap::DeleteAtKey( LPCTSTR at )
{
	ADDTOCALLSTACK("CVarDefMap::DeleteAtKey");
	CVarDefContStr * pVarBased = new CVarDefContStr(at);
	DefSet::iterator i = m_Container.find(pVarBased);
	delete pVarBased;

	DeleteAtIterator(i);
}

void CVarDefMap::DeleteAtIterator( DefSet::iterator it )
{
	ADDTOCALLSTACK("CVarDefMap::DeleteAtIterator");
	if ( it != m_Container.end() )
	{
		CVarDefCont *pVarBase = (*it);
		m_Container.erase(it);

		if ( pVarBase )
		{
			CVarDefContNum *pVarNum = dynamic_cast<CVarDefContNum *>(pVarBase);
			if ( pVarNum )
			{
				delete pVarNum;
			}
			else
			{
				CVarDefContStr *pVarStr = dynamic_cast<CVarDefContStr *>(pVarBase);
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

bool CVarDefMap::Compare( const CVarDefMap * pArray )
{
	ADDTOCALLSTACK("CVarDefMap::Compare");
	if ( !pArray )
		return false;
	if ( pArray == this )
		return true;
	if ( pArray->GetCount() != GetCount() )
		return false;

	if (pArray->GetCount())
	{
		for ( DefSet::const_iterator i = pArray->m_Container.begin(); i != pArray->m_Container.end(); ++i )
		{
			const CVarDefCont * pVar = (*i);
			LPCTSTR sKey = pVar->GetKey();
			if (!GetKey(sKey))
				return false;

			if (strcmpi(GetKeyStr(sKey),pVar->GetValStr()))
				return false;
		}
	}
	return true;
}

bool CVarDefMap::CompareAll( const CVarDefMap * pArray )
{
	ADDTOCALLSTACK("CVarDefMap::Compare");
	if ( !pArray )
		return false;
	if ( pArray == this )
		return true;

	if (pArray->GetCount())
	{
		for ( DefSet::const_iterator i = pArray->m_Container.begin(); i != pArray->m_Container.end(); ++i )
		{
			const CVarDefCont * pVar = (*i);
			LPCTSTR sKey = pVar->GetKey();

			if (strcmpi(GetKeyStr(sKey, true),pVar->GetValStr()))
				return false;
		}
	}
	if (GetCount())
	{
		for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); ++i )
		{
			const CVarDefCont * pVar = (*i);
			LPCTSTR sKey = pVar->GetKey();

			if (strcmpi(pArray->GetKeyStr(sKey, true),pVar->GetValStr()))
				return false;
		}
	}
	return true;
}

size_t CVarDefMap::GetCount() const
{
	ADDTOCALLSTACK("CVarDefMap::GetCount");
	return m_Container.size();
}

CVarDefContNum *CVarDefMap::SetNumNew(LPCTSTR pszName, INT64 iVal)
{
	ADDTOCALLSTACK("CVarDefMap::SetNumNew");
	CVarDefContNum *pVarNum = new CVarDefContNum(pszName, iVal);
	if ( !pVarNum )
		return NULL;

	DefPairResult res = m_Container.insert(static_cast<CVarDefCont *>(pVarNum));
	if ( res.second )
		return pVarNum;

	delete pVarNum;
	return NULL;
}

CVarDefContNum *CVarDefMap::SetNumOverride(LPCTSTR pszKey, INT64 iVal)
{
	ADDTOCALLSTACK("CVarDefMap::SetNumOverride");
	DeleteAtKey(pszKey);
	return SetNumNew(pszKey,iVal);
}

CVarDefContNum *CVarDefMap::SetNum(LPCTSTR pszName, INT64 iVal, bool fZero)
{
	ADDTOCALLSTACK("CVarDefMap::SetNum");
	ASSERT(pszName);

	if ( pszName[0] == '\0' )
		return NULL;

	if ( fZero && (iVal == 0) )
	{
		DeleteAtKey(pszName);
		return NULL;
	}

	CVarDefContTest * pVarSearch = new CVarDefContTest(pszName);
	DefSet::iterator iResult = m_Container.find(pVarSearch);
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
		return pVarNum;
	}

	if ( g_Serv.IsLoading() )
		DEBUG_ERR(("Replace existing VarStr '%s'\n", pVarBase->GetKey()));

	return SetNumOverride(pszName, iVal);
}

CVarDefContStr *CVarDefMap::SetStrNew(LPCTSTR pszName, LPCTSTR pszVal)
{
	ADDTOCALLSTACK("CVarDefMap::SetStrNew");
	CVarDefContStr *pVarStr = new CVarDefContStr(pszName, pszVal);
	if ( !pVarStr )
		return NULL;

	DefPairResult res = m_Container.insert(static_cast<CVarDefCont *>(pVarStr));
	if ( res.second )
		return pVarStr;

	delete pVarStr;
	return NULL;
}

CVarDefContStr *CVarDefMap::SetStrOverride(LPCTSTR pszKey, LPCTSTR pszVal)
{
	ADDTOCALLSTACK("CVarDefMap::SetStrOverride");
	DeleteAtKey(pszKey);
	return SetStrNew(pszKey,pszVal);
}

CVarDefCont *CVarDefMap::SetStr(LPCTSTR pszName, bool fQuoted, LPCTSTR pszVal, bool fZero)
{
	ADDTOCALLSTACK("CVarDefMap::SetStr");
	// ASSUME: This has been clipped of unwanted beginning and trailing spaces.
	if ( !pszName || !pszName[0] )
		return NULL;

	if ( pszVal == NULL || pszVal[0] == '\0' )	// but not if empty
	{
		DeleteAtKey(pszName);
		return NULL;
	}

	if ( !fQuoted && IsSimpleNumberString(pszVal))
	{
		// Just store the number and not the string.
		return SetNum( pszName, Exp_GetLLVal( pszVal ), fZero);
	}

	CVarDefContTest * pVarSearch = new CVarDefContTest(pszName);
	DefSet::iterator iResult = m_Container.find(pVarSearch);
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
		pVarStr->SetValStr(pszVal);
		return pVarStr;
	}

	if ( g_Serv.IsLoading() )
		DEBUG_ERR(("Replace existing VarNum '%s' with '%s'\n", pVarBase->GetKey(), pszVal));

	return SetStrOverride(pszName, pszVal);
}

CVarDefCont * CVarDefMap::GetKey( LPCTSTR pszKey ) const
{
	ADDTOCALLSTACK("CVarDefMap::GetKey");
	CVarDefCont * pReturn = NULL;

	if ( pszKey )
	{
		CVarDefContTest * pVarBase = new CVarDefContTest(pszKey);
		DefSet::const_iterator i = m_Container.find(pVarBase);
		delete pVarBase;
		
		if ( i != m_Container.end() )
			pReturn = (*i);
	}

	return( pReturn );
}

INT64 CVarDefMap::GetKeyNum( LPCTSTR pszKey ) const
{
	ADDTOCALLSTACK("CVarDefMap::GetKeyNum");
	CVarDefCont *pVar = GetKey(pszKey);
	return pVar ? pVar->GetValNum() : 0;
}

LPCTSTR CVarDefMap::GetKeyStr( LPCTSTR pszKey, bool fZero ) const
{
	ADDTOCALLSTACK("CVarDefMap::GetKeyStr");
	CVarDefCont * pVar = GetKey(pszKey);
	if ( pVar == NULL )
		return (fZero ? "0" : "");
	return pVar->GetValStr();
}

CVarDefCont * CVarDefMap::CheckParseKey( LPCTSTR & pszArgs ) const
{
	ADDTOCALLSTACK("CVarDefMap::CheckParseKey");
	TCHAR szTag[ EXPRESSION_MAX_KEY_LEN ];
	GetIdentifierString( szTag, pszArgs );
	CVarDefCont * pVar = GetKey(szTag);
	if ( pVar )
		return( pVar );

	return NULL;
}

CVarDefCont * CVarDefMap::GetParseKey( LPCTSTR & pszArgs ) const
{
	ADDTOCALLSTACK("CVarDefMap::GetParseKey");
	// Skip to the end of the expression name.
	// The name can only be valid.

	TCHAR szTag[ EXPRESSION_MAX_KEY_LEN ];
	size_t i = GetIdentifierString( szTag, pszArgs );
	CVarDefCont * pVar = GetKey(szTag);
	if ( pVar )
	{
		pszArgs += i;
		return( pVar );
	}

	return NULL;
}

bool CVarDefMap::GetParseVal( LPCTSTR & pszArgs, long long * plVal ) const
{
	ADDTOCALLSTACK("CVarDefMap::GetParseVal");
	CVarDefCont * pVarBase = GetParseKey( pszArgs );
	if ( pVarBase == NULL )
		return( false );
	*plVal = pVarBase->GetValNum();
	return( true );
}

void CVarDefMap::DumpKeys( CTextConsole * pSrc, LPCTSTR pszPrefix ) const
{
	ADDTOCALLSTACK("CVarDefMap::DumpKeys");
	// List out all the keys.
	ASSERT(pSrc);
	if ( pszPrefix == NULL )
		pszPrefix = "";

	for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); ++i )
	{
		const CVarDefCont *pVar = (*i);
		pSrc->SysMessagef(pSrc->GetChar()? "%s%s=%s" : "%s%s=%s\n", pszPrefix, pVar->GetKey(), pVar->GetValStr());
	}
}

void CVarDefMap::ClearKeys(LPCTSTR mask)
{
	ADDTOCALLSTACK("CVarDefMap::ClearKeys");
	if ( mask && *mask )
	{
		if ( !m_Container.size() )
			return;

		CGString sMask(mask);
		sMask.MakeLower();

		DefSet::iterator i = m_Container.begin();
		CVarDefCont * pVarBase = NULL;

		while ( i != m_Container.end() )
		{
			pVarBase = (*i);

			if ( pVarBase && ( strstr(pVarBase->GetKey(), sMask.GetPtr()) ) )
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

bool CVarDefMap::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CVarDefMap::r_LoadVal");
	bool fQuoted = false;
	return( ( SetStr( s.GetKey(), fQuoted, s.GetArgStr( &fQuoted )) >= 0 ) ? true : false );
}

void CVarDefMap::r_WritePrefix( CScript & s, LPCTSTR pszPrefix, LPCTSTR pszKeyExclude )
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::r_WritePrefix");
	TemporaryString z;
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

		if ( bHasPrefix )
			sprintf(z, "%s.%s", pszPrefix, pVar->GetKey());
		else
			sprintf(z, "%s", pVar->GetKey());

		pszVal = pVar->GetValStr();
		const CVarDefContStr * pVarStr = dynamic_cast <const CVarDefContStr *>(pVar);
		if ( pVarStr ) // IsSpace(pszVal[0]) || IsSpace( pszVal[strlen(pszVal)-1] )
			s.WriteKeyFormat(z, "\"%s\"", pszVal);
		else
			s.WriteKey(z, pszVal);
	}
}
