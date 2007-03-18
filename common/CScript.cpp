//
// CScript.cpp
// Copyright Menace Software (www.menasoft.com).
//
// NOTE: all scripts should be encoded in UTF-8.
// So they may have full unicode chars inside.

#include "../graysvr/graysvr.h"

///////////////////////////////////////////////////////////////
// -CScriptKey

void CScriptKey::InitKey()
{
	ADDTOCALLSTACK("CScriptKey::InitKey");
	m_pszArg = m_pszKey = NULL;
}

TCHAR * CScriptKey::GetArgStr( bool * fQuoted )	// this could be a quoted string ?
{
	ADDTOCALLSTACK("CScriptKey::GetArgStr");
	ASSERT(m_pszKey);

	TCHAR * pStr = GetArgRaw();
	if ( *pStr != '"' )
		return( pStr );

	pStr++;
	//TCHAR * pEnd = strchr( pStr, '"' );
	// search for last qoute sybol starting from the end
	for (TCHAR * pEnd = pStr + strlen( pStr ) - 1; pEnd >= pStr; pEnd-- )
		if ( *pEnd == '"' )
		{
			*pEnd = '\0';
			if ( fQuoted )
				*fQuoted = true;
			break;
		}

	return( pStr );
}

DWORD CScriptKey::GetArgFlag( DWORD dwStart, DWORD dwMask )
{
	ADDTOCALLSTACK("CScriptKey::GetArgFlag");
	// No args = toggle the flag.
	// 1 = set the flag.
	// 0 = clear the flag.

	ASSERT(m_pszKey);
	ASSERT(m_pszArg);

	if ( ! HasArgs())
		return( dwStart ^ dwMask );

	else if ( GetArgVal())
		return( dwStart | dwMask );

	else
		return( dwStart &~ dwMask );
}

long CScriptKey::GetArgVal()
{
	ADDTOCALLSTACK("CScriptKey::GetArgVal");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return( Exp_GetVal( m_pszArg ));
}

long CScriptKey::GetArgRange()
{
	ADDTOCALLSTACK("CScriptKey::GetArgRange");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return( Exp_GetRange( m_pszArg ));
}

///////////////////////////////////////////////////////////////
// -CScriptKeyAlloc

TCHAR * CScriptKeyAlloc::GetKeyBufferRaw( int iLen )
{
	ADDTOCALLSTACK("CScriptKeyAlloc::GetKeyBufferRaw");
	// iLen = length of the string we want to hold.

	ASSERT( iLen >= 0 );

	if ( iLen > SCRIPT_MAX_LINE_LEN )
		iLen = SCRIPT_MAX_LINE_LEN;
	iLen ++;	// add null.

	if ( m_Mem.GetDataLength() < iLen )
	{
		m_Mem.Alloc( iLen );
	}

	m_pszKey = m_pszArg = GetKeyBuffer();
	m_pszKey[0] = '\0';

	return m_pszKey;
}

bool CScriptKeyAlloc::ParseKey( LPCTSTR pszKey )
{
	ADDTOCALLSTACK("CScriptKeyAlloc::ParseKey");
	// Skip leading white space 
	if ( ! pszKey )
	{
		GetKeyBufferRaw(0);
		return false;
	}

	GETNONWHITESPACE( pszKey );

	TCHAR * pBuffer = GetKeyBufferRaw( strlen( pszKey ));
	ASSERT(pBuffer);

	int iLen = m_Mem.GetDataLength()-1;
	strncpy( pBuffer, pszKey, iLen );
	pBuffer[iLen] = '\0';

	Str_Parse( pBuffer, &m_pszArg, "=, \t()[]{}" );
	return( true );
}

bool CScriptKeyAlloc::ParseKey( LPCTSTR pszKey, LPCTSTR pszVal )
{
	ADDTOCALLSTACK("CScriptKeyAlloc::ParseKey");
	ASSERT(pszKey);

	int lenkey = strlen( pszKey );
	if ( ! lenkey )
	{
		return ParseKey(pszVal);
	}

	ASSERT( lenkey < SCRIPT_MAX_LINE_LEN-2 );

	int lenval = 0;
	if ( pszVal )
	{
		lenval = strlen( pszVal );
	}

	m_pszKey = GetKeyBufferRaw( lenkey + lenval + 1 );

	strcpy( m_pszKey, pszKey );
	m_pszArg = m_pszKey + lenkey;

	if ( pszVal )
	{
		m_pszArg ++;
		lenval = m_Mem.GetDataLength()-2;
		strcpylen( m_pszArg, pszVal, ( lenval - lenkey ) + 1 );	// strcpylen
	}

	return( true );
}

int CScriptKeyAlloc::ParseKeyEnd()
{
	ADDTOCALLSTACK("CScriptKeyAlloc::ParseKeyEnd");
	// Now parse the line for comments and trailing whitespace junk
	// NOTE: leave leading whitespace for now.

	ASSERT(m_pszKey);

	int len = 0;
	for ( ; len<SCRIPT_MAX_LINE_LEN; len++ )
	{
		TCHAR ch = m_pszKey[len];
		if ( ch == '\0' )
			break;
		if ( ch == '/' && m_pszKey[len+1] == '/' )
		{
			// Remove comment at end of line.
			break;
		}
	}

	// Remove CR and LF from the end of the line.
	len = Str_TrimEndWhitespace( m_pszKey, len );
	if ( len <= 0 )		// fRemoveBlanks &&
		return 0;

	m_pszKey[len] = '\0';
	return( len );
}

void CScriptKeyAlloc::ParseKeyLate()
{
	ADDTOCALLSTACK("CScriptKeyAlloc::ParseKeyLate");
	ASSERT(m_pszKey);
	ParseKeyEnd();
	GETNONWHITESPACE(m_pszKey);
	Str_Parse(m_pszKey, &m_pszArg);
}

///////////////////////////////////////////////////////////////
// -CScript

CScript::CScript()
{
	InitBase();
}

CScript::CScript( LPCTSTR pszKey )
{
	InitBase();
	ParseKey(pszKey);
}

CScript::CScript( LPCTSTR pszKey, LPCTSTR pszVal )
{
	InitBase();
	ParseKey( pszKey, pszVal );
}

void CScript::InitBase()
{
	ADDTOCALLSTACK("CScript::InitBase");
	m_iLineNum		= 0;
	m_fSectionHead	= false;
	m_lSectionData	= 0;
	InitKey();
}

bool CScript::Open( LPCTSTR pszFilename, UINT wFlags )
{
	ADDTOCALLSTACK("CScript::Open");
	// If we are in read mode and we have no script file.
	// ARGS: wFlags = OF_READ, OF_NONCRIT etc
	// RETURN: true = success.

	InitBase();

	if ( pszFilename == NULL )
	{
		pszFilename = GetFilePath();
	}
	else
	{
		SetFilePath( pszFilename );
	}

	LPCTSTR pszTitle = GetFileTitle();
	if ( pszTitle == NULL || pszTitle[0] == '\0' )
		return( false );

	LPCTSTR pszExt = GetFilesExt( GetFilePath() ); 
	if ( pszExt == NULL )
	{
		TCHAR szTemp[ _MAX_PATH ];
		strcpy( szTemp, GetFilePath() );
		strcat( szTemp, GRAY_SCRIPT );
		SetFilePath( szTemp );
		wFlags |= OF_TEXT;
	}

	if ( !PhysicalScriptFile::Open( GetFilePath(), wFlags ))
	{
		if ( ! ( wFlags & OF_NONCRIT ))
		{
			g_Log.Event( LOGL_WARN, "'%s' not found...\n", (LPCTSTR) GetFilePath() );
		}
		return( false );
	}

	return( true );
}

bool CScript::ReadTextLine( bool fRemoveBlanks ) // Read a line from the opened script file
{
	ADDTOCALLSTACK("CScript::ReadTextLine");
	// ARGS:
	// fRemoveBlanks = Don't report any blank lines, (just keep reading)
	//

	while ( PhysicalScriptFile::ReadString( GetKeyBufferRaw(SCRIPT_MAX_LINE_LEN), SCRIPT_MAX_LINE_LEN ))
	{
		m_iLineNum++;
		if ( fRemoveBlanks )
		{
			if ( ParseKeyEnd() <= 0 )
				continue;
		}
		return( true );
	}

	m_pszKey[0] = '\0';
	return( false );
}

bool CScript::FindTextHeader( LPCTSTR pszName ) // Find a section in the current script
{
	ADDTOCALLSTACK("CScript::FindTextHeader");
	// RETURN: false = EOF reached.
	ASSERT(pszName);
	ASSERT( ! IsBinaryMode());

	SeekToBegin();

	int len = strlen( pszName );
	ASSERT(len);
	do
	{
		if ( ! ReadTextLine(false))
		{
			return( false );
		}
		if ( IsKeyHead( "[EOF]", 5 ))
		{
			return( false );
		}
	}
	while ( ! IsKeyHead( pszName, len ));
	return( true );
}

LONG CScript::Seek( long offset, UINT origin )
{
	ADDTOCALLSTACK("CScript::Seek");
	// Go to the start of a new section.
	// RETURN: the new offset in bytes from start of file.
	if ( offset == 0 && origin == SEEK_SET )
	{
		m_iLineNum = 0;	// so we don't have to override SeekToBegin
	}
	m_fSectionHead = false;		// unknown , so start at the beginning.
	m_lSectionData = offset;
	return( PhysicalScriptFile::Seek(offset,origin));
}

bool CScript::FindNextSection()
{
	ADDTOCALLSTACK("CScript::FindNextSection");
	EXC_TRY("FindNextSection");
	// RETURN: false = EOF.

	if ( m_fSectionHead )	// we have read a section already., (not at the start)
	{
		// Start from the previous line. It was the line that ended the last read.
		m_pszKey = GetKeyBuffer();
		ASSERT(m_pszKey);
		m_fSectionHead = false;
		if ( m_pszKey[0] == '[' )
			goto foundit;
	}

	while (true)
	{
		if ( !ReadTextLine(true) )
		{
			m_lSectionData = GetPosition();
			return( false );
		}
		if ( m_pszKey[0] == '[' )
			break;
	}

foundit:
	// Parse up the section name.
	m_pszKey++;
	int len = strlen( m_pszKey );
	for ( int i=0; i<len; i++ )
	{
		if ( m_pszKey[i] == ']' )
		{
			m_pszKey[i] = '\0';
			break;
		}
	}

	m_lSectionData = GetPosition();
	if ( IsSectionType( "EOF" ))
		return( false );

	Str_Parse( m_pszKey, &m_pszArg );
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_DEBUG_END;
	return false;
}

bool CScript::FindSection( LPCTSTR pszName, UINT uModeFlags )
{
	ADDTOCALLSTACK("CScript::FindSection");
	// Find a section in the current script
	// RETURN: true = success

	ASSERT(pszName);
	if ( strlen( pszName ) > 32 )
	{
		DEBUG_ERR(( "Bad script section name\n" ));
		return( false );
	}

	TCHAR	*pszSec = Str_GetTemp();
	sprintf(pszSec, "[%s]", pszName);
	if ( FindTextHeader(pszSec))
	{
		// Success
		m_lSectionData = GetPosition();
		return( true );
	}

	// Failure Error display. (default)

	if ( ! ( uModeFlags & OF_NONCRIT ))
	{
		g_Log.Event( LOGL_WARN, "Did not find '%s' section '%s'\n", (LPCTSTR) GetFileTitle(), (LPCTSTR) pszName );
	}
	return( false );
}

bool CScript::ReadKey( bool fRemoveBlanks )
{
	ADDTOCALLSTACK("CScript::ReadKey");
	if ( ! ReadTextLine(fRemoveBlanks))
		return( false );
	if ( m_pszKey[0] == '[' )	// hit the end of our section.
	{
		m_fSectionHead = true;
		return( false );
	}
	return( true );
}

bool CScript::ReadKeyParse() // Read line from script
{
	ADDTOCALLSTACK("CScript::ReadKeyParse");
	EXC_TRY("ReadKeyParse");
	EXC_SET("read");
	if ( !ReadKey(true) )
	{
		EXC_SET("init");
		InitKey();
		return false;	// end of section.
	}

	ASSERT(m_pszKey);
	GETNONWHITESPACE( m_pszKey );
	EXC_SET("parse");
	Str_Parse( m_pszKey, &m_pszArg );

	//if ( !m_pszArg[0] || m_pszArg[1] != '=' || !strchr( ".*+-/%|&!^", m_pszArg[0] ) )
	if ( !m_pszArg[0] || ( m_pszArg[1] != '=' && m_pszArg[1] != '+' && m_pszArg[1] != '-' ) || !strchr( ".*+-/%|&!^", m_pszArg[0] ) )
		return true;

	EXC_SET("parse");
	LPCTSTR	pszArgs	= m_pszArg;
	pszArgs+=2;
	GETNONWHITESPACE( pszArgs );
	TCHAR	*buf = Str_GetTemp();
	if ( m_pszArg[0] == '.' )
	{
		if ( *pszArgs == '"' )
		{
			TCHAR *	pQuote	= const_cast<TCHAR*>(strchr( pszArgs+1, '"' ));
			if ( pQuote )
			{
				pszArgs++;
				*pQuote	= '\0';
			}
		}
		sprintf( buf, "<%s>%s", m_pszKey, pszArgs );
	}
	else if ( m_pszArg[0] == m_pszArg[1] && m_pszArg[1] == '+' )
		sprintf( buf, "<eval (<%s> +1)>", m_pszKey );
	else if ( m_pszArg[0] == m_pszArg[1] && m_pszArg[1] == '-' )
		sprintf( buf, "<eval (<%s> -1)>", m_pszKey );
	else
		sprintf( buf, "<eval (<%s> %c (%s))>", m_pszKey, *m_pszArg, pszArgs );
	strcpy( m_pszArg, buf );

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_DEBUG_END;
	return false;
}

bool CScript::FindKey( LPCTSTR pszName ) // Find a key in the current section
{
	ADDTOCALLSTACK("CScript::FindKey");
	if ( strlen( pszName ) > SCRIPT_MAX_SECTION_LEN )
	{
		DEBUG_ERR(( "Bad script key name\n" ));
		return( false );
	}
	Seek( m_lSectionData );
	while ( ReadKeyParse())
	{
		if ( IsKey( pszName ))
		{
			m_pszArg = Str_TrimWhitespace( m_pszArg );
			return true;
		}
	}
	return( false );
}

void CScript::Close()
{
	ADDTOCALLSTACK("CScript::Close");
	// EndSection();
	PhysicalScriptFile::Close();
}

bool _cdecl CScript::WriteSection( LPCTSTR pszSection, ... )
{
	ADDTOCALLSTACK("CScript::WriteSection");
	// Write out the section header.
	va_list vargs;
	va_start( vargs, pszSection );

	// EndSection();	// End any previous section.
	Printf( "\n[");
	VPrintf( pszSection, vargs );
	Printf( "]\n" );
	va_end( vargs );

	return( true );
}

bool CScript::WriteKey( LPCTSTR pszKey, LPCTSTR pszVal )
{
	ADDTOCALLSTACK("CScript::WriteKey");
	if ( pszKey == NULL || pszKey[0] == '\0' )
	{
		return false;
	}

	TCHAR		ch;
	TCHAR *		pszSep;
	if ( pszVal == NULL || pszVal[0] == '\0' )
	{
		if ( !( pszSep = const_cast<TCHAR*>(strchr( pszKey, '\n' ))) )
			pszSep = const_cast<TCHAR*>(strchr( pszKey, '\r' ));	// acts like const_cast

		if ( pszSep )
		{
			g_Log.Event( LOGL_WARN|LOGM_CHEAT, "carriage return in key (book?) - truncating" );
			ch		= *pszSep;
			*pszSep	= '\0';
		}

		// Books are like this. No real keys.
		Printf( "%s\n", pszKey );

		if ( pszSep )
			*pszSep	= ch;
	}
	else
	{
		if ( !( pszSep = const_cast<TCHAR*>(strchr( pszVal, '\n' ))) )
			pszSep = const_cast<TCHAR*>(strchr( pszVal, '\r' ));	// acts like const_cast

		if ( pszSep )
		{
			g_Log.Event( LOGL_WARN|LOGM_CHEAT, "carriage return in key value - truncating" );
			ch		= *pszSep;
			*pszSep	= '\0';
		}
		Printf( "%s=%s\n", pszKey, pszVal );
		if ( pszSep )
			*pszSep	= ch;
	}

	return( true );
}

void _cdecl CScript::WriteKeyFormat( LPCTSTR pszKey, LPCTSTR pszVal, ... )
{
	ADDTOCALLSTACK("CScript::WriteKeyFormat");
	TCHAR	*pszTemp = Str_GetTemp();
	va_list vargs;
	va_start( vargs, pszVal );
	vsprintf(pszTemp, pszVal, vargs);
	WriteKey(pszKey, pszTemp);
	va_end( vargs );
}


