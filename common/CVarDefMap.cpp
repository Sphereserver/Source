#include "../graysvr/graysvr.h"

inline int CVarDefContStr::GetValNum() const
{
	LPCTSTR pszStr = m_sVal;
	return( Exp_GetVal(pszStr) );
}

inline LPCTSTR CVarDefContNum::GetValStr() const
{
	TCHAR * pszTmp = Str_GetTemp();
	sprintf(pszTmp, "0%x", m_iVal);
	return pszTmp;
}

// *************************************************************************************
// *************************************************************************************
// *************************************************************************************

LPCTSTR CVarDefMap::FindValStr( LPCTSTR pVal ) const
{
	ADDTOCALLSTACK("CVarDefMap::FindValStr");
	for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); i++ )
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
	for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); i++ )
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
	while ( at-- ) { i++; }

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
	while ( at-- ) { i++; }

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

	for ( DefSet::const_iterator i = pArray->m_Container.begin(); i != pArray->m_Container.end(); i++ )
	{
		m_Container.insert( (*i)->CopySelf() );
	}
}

int CVarDefMap::GetCount() const
{
	ADDTOCALLSTACK("CVarDefMap::GetCount");
	return m_Container.size();
}


bool CVarDefMap::SetNumNew( LPCTSTR pszName, int iVal )
{
	ADDTOCALLSTACK("CVarDefMap::SetNumNew");
	CVarDefCont * pVarNum = new CVarDefContNum( pszName, iVal );
	if ( !pVarNum )
		return( false );

	DefPairResult res = m_Container.insert(pVarNum);
	return( res.second );
}

bool CVarDefMap::SetNumOverride( LPCTSTR pszKey, int iVal )
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
		return( false );

	if ( fZero && (iVal == 0) )
	{
		DeleteAtKey(pszName);
		return( -1 );
	}

	CVarDefCont * pVarBase = GetAtKey(pszName);
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

	return( true );
}

bool CVarDefMap::SetStrNew( LPCTSTR pszName, LPCTSTR pszVal )
{
	ADDTOCALLSTACK("CVarDefMap::SetStrNew");
	CVarDefCont * pVarStr = new CVarDefContStr( pszName, pszVal );
	if ( !pVarStr )
		return( false );

	DefPairResult res = m_Container.insert(pVarStr);
	return( res.second );
}

bool CVarDefMap::SetStrOverride( LPCTSTR pszKey, LPCTSTR pszVal )
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

	CVarDefCont * pVarBase = GetAtKey(pszName);
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
	return( true );
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

//	if ( this == (&g_Exp.m_VarGlobals) )
//		return NULL;

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

	for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); i++ )
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

		for ( DefSet::iterator i = m_Container.begin(); i != m_Container.end(); i++ )
		{
			CVarDefCont * pVar = (*i);
			if ( pVar )
			{
				if ( !strstr(pVar->GetKey(), sMask.GetPtr()) ) 
					continue;

				DeleteAtIterator(i);
				if ( i != m_Container.begin() )
				{
					i--;
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
	return SetStr( s.GetKey(), fQuoted, s.GetArgStr( &fQuoted )) ? true : false;
}

void CVarDefMap::r_WritePrefix( CScript & s, LPCTSTR pszPrefix, LPCTSTR pszKeyExclude )
{
	ADDTOCALLSTACK("CVarDefMap::r_WritePrefix");
	LPCTSTR		pszVal;
	bool bHasPrefix = (pszPrefix && *pszPrefix);
	bool bHasExclude = (pszKeyExclude && *pszKeyExclude);

	// Write with any prefix.
	for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); i++ )
	{
		const CVarDefCont * pVar = (*i);
		if ( !pVar )
		{
			// This should not happen, a warning maybe?
			continue;
		}

		if ( bHasExclude && !strcmpi(pszKeyExclude, pVar->GetKey()))
			continue;

		char	*z = Str_GetTemp();

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