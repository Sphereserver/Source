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

TCHAR *CScriptKey::GetArgStr(bool *fQuoted)
{
	ADDTOCALLSTACK("CScriptKey::GetArgStr");
	ASSERT(m_pszKey);

	TCHAR *pszStr = GetArgRaw();
	if ( *pszStr != '"' )
		return pszStr;

	pszStr++;

	// Search for last quote symbol starting from the end
	for ( TCHAR *pszEnd = pszStr + strlen(pszStr) - 1; pszEnd >= pszStr; pszEnd-- )
	{
		if ( *pszEnd == '"' )
		{
			*pszEnd = '\0';
			if ( fQuoted )
				*fQuoted = true;
			break;
		}
	}
	return pszStr;
}

UINT64 CScriptKey::GetArgFlag(UINT64 uiStart, UINT64 uiMask)
{
	ADDTOCALLSTACK("CScriptKey::GetArgFlag");
	// No args = toggle the flag
	// 1 = set the flag
	// 0 = clear the flag
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);

	if ( !HasArgs() )
		return (uiStart ^ uiMask);
	else if ( GetArgVal() )
		return (uiStart | uiMask);
	else
		return (uiStart & ~uiMask);
}

long long CScriptKey::GetArgLLVal()
{
	ADDTOCALLSTACK("CScriptKey::GetArgLLVal");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return Exp_GetLLVal(m_pszArg);
}

long CScriptKey::GetArgVal()
{
	ADDTOCALLSTACK("CScriptKey::GetArgVal");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return Exp_GetVal(m_pszArg);
}

long CScriptKey::GetArgRange()
{
	ADDTOCALLSTACK("CScriptKey::GetArgRange");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return Exp_GetRange(m_pszArg);
}

///////////////////////////////////////////////////////////////
// -CScriptKeyAlloc

TCHAR *CScriptKeyAlloc::GetKeyBufferRaw(size_t iLen)
{
	ADDTOCALLSTACK("CScriptKeyAlloc::GetKeyBufferRaw");
	// ARGS: iLen = length of the string to hold
	if ( iLen > SCRIPT_MAX_LINE_LEN )
		iLen = SCRIPT_MAX_LINE_LEN;
	iLen++;		// add null

	if ( m_Mem.GetDataLength() < iLen )
		m_Mem.Alloc(iLen);

	m_pszKey = m_pszArg = GetKeyBuffer();
	m_pszKey[0] = '\0';
	return m_pszKey;
}

bool CScriptKeyAlloc::ParseKey(LPCTSTR pszKey)
{
	ADDTOCALLSTACK("CScriptKeyAlloc::ParseKey");
	// Skip leading whitespace 
	if ( !pszKey )
	{
		GetKeyBufferRaw(0);
		return false;
	}

	GETNONWHITESPACE(pszKey);

	TCHAR *pszBuffer = GetKeyBufferRaw(strlen(pszKey));
	ASSERT(pszBuffer);

	size_t iLen = m_Mem.GetDataLength() - 1;
	strncpy(pszBuffer, pszKey, iLen);
	pszBuffer[iLen] = '\0';

	Str_Parse(pszBuffer, &m_pszArg);
	return true;
}

bool CScriptKeyAlloc::ParseKey(LPCTSTR pszKey, LPCTSTR pszVal)
{
	ADDTOCALLSTACK("CScriptKeyAlloc::ParseKey");
	ASSERT(pszKey);

	size_t iLenKey = strlen(pszKey);
	if ( !iLenKey )
		return ParseKey(pszVal);

	ASSERT(iLenKey < SCRIPT_MAX_LINE_LEN - 2);

	size_t iLenVal = 0;
	if ( pszVal )
		iLenVal = strlen(pszVal);

	m_pszKey = GetKeyBufferRaw(iLenKey + iLenVal + 1);

	strcpy(m_pszKey, pszKey);
	m_pszArg = m_pszKey + iLenKey;

	if ( pszVal )
	{
		m_pszArg++;
		iLenVal = m_Mem.GetDataLength() - 2;
		strcpylen(m_pszArg, pszVal, (iLenVal - iLenKey) + 1);
	}

	return true;
}

size_t CScriptKeyAlloc::ParseKeyEnd()
{
	ADDTOCALLSTACK("CScriptKeyAlloc::ParseKeyEnd");
	// Now parse the line for comments and trailing whitespace junk
	// NOTE: leave leading whitespace for now

	ASSERT(m_pszKey);

	size_t iLen = 0;
	for ( ; iLen < SCRIPT_MAX_LINE_LEN; iLen++ )
	{
		TCHAR ch = m_pszKey[iLen];
		if ( ch == '\0' )
			break;
		if ( (ch == '/') && (m_pszKey[iLen + 1] == '/') )	// remove comment at end of line
			break;
	}

	// Remove CR and LF from the end of the line
	iLen = Str_TrimEndWhitespace(m_pszKey, iLen);
	if ( iLen <= 0 )
		return 0;

	m_pszKey[iLen] = '\0';
	return iLen;
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

CScript::CScript(LPCTSTR pszKey)
{
	InitBase();
	ParseKey(pszKey);
}

CScript::CScript(LPCTSTR pszKey, LPCTSTR pszVal)
{
	InitBase();
	ParseKey(pszKey, pszVal);
}

void CScript::InitBase()
{
	ADDTOCALLSTACK("CScript::InitBase");
	m_iLineNum = 0;
	m_fSectionHead = false;
	m_lSectionData = 0;
	InitKey();
}

bool CScript::Open(LPCTSTR pszFilename, UINT uiFlags)
{
	ADDTOCALLSTACK("CScript::Open");
	// If we are in read mode and we have no script file
	// RETURN: true = success

	InitBase();

	if ( pszFilename == NULL )
		pszFilename = GetFilePath();
	else
		SetFilePath(pszFilename);

	LPCTSTR pszTitle = GetFileTitle();
	if ( (pszTitle == NULL) || (pszTitle[0] == '\0') )
		return false;

	LPCTSTR pszExt = GetFilesExt(GetFilePath());
	if ( pszExt == NULL )
	{
		TCHAR szTemp[_MAX_PATH];
		strcpy(szTemp, GetFilePath());
		strcat(szTemp, SPHERE_SCRIPT);
		SetFilePath(szTemp);
		uiFlags |= OF_TEXT;
	}

	if ( !CCacheableScriptFile::Open(GetFilePath(), uiFlags) )
	{
		if ( !(uiFlags & OF_NONCRIT) )
			g_Log.Event(LOGL_WARN, "'%s' not found\n", static_cast<LPCTSTR>(GetFilePath()));
		return false;
	}

	return true;
}

bool CScript::ReadTextLine(bool fRemoveBlanks)
{
	ADDTOCALLSTACK("CScript::ReadTextLine");
	// ARGS: fRemoveBlanks = Don't report any blank lines (just keep reading)

	while ( CCacheableScriptFile::ReadString(GetKeyBufferRaw(SCRIPT_MAX_LINE_LEN), SCRIPT_MAX_LINE_LEN) )
	{
		m_iLineNum++;
		if ( fRemoveBlanks )
		{
			if ( ParseKeyEnd() <= 0 )
				continue;
		}
		return true;
	}

	m_pszKey[0] = '\0';
	return false;
}

bool CScript::FindTextHeader(LPCTSTR pszName)
{
	ADDTOCALLSTACK("CScript::FindTextHeader");
	// RETURN: false = EOF reached
	ASSERT(pszName);
	ASSERT(!IsBinaryMode());

	SeekToBegin();

	size_t iLen = strlen(pszName);
	ASSERT(iLen);
	do
	{
		if ( !ReadTextLine(false) )
			return false;
		if ( IsKeyHead("[EOF]", 5) )
			return false;
	} while (!IsKeyHead(pszName, iLen));
	return true;
}

DWORD CScript::Seek(long lOffset, UINT uiOrigin)
{
	ADDTOCALLSTACK("CScript::Seek");
	// Go to the start of a new section
	// RETURN: the new offset in bytes from start of file

	if ( (lOffset == 0) && (uiOrigin == SEEK_SET) )
		m_iLineNum = 0;		// so we don't have to override SeekToBegin

	m_fSectionHead = false;		// unknown, so start at the beginning
	m_lSectionData = lOffset;
	return CCacheableScriptFile::Seek(lOffset, uiOrigin);
}

bool CScript::FindNextSection()
{
	ADDTOCALLSTACK("CScript::FindNextSection");
	EXC_TRY("FindNextSection");
	// RETURN: false = EOF reached

	if ( m_fSectionHead )	// we have read a section already (not at the start)
	{
		// Start from the previous line, it was the line that ended the last read
		m_pszKey = GetKeyBuffer();
		ASSERT(m_pszKey);
		m_fSectionHead = false;
		if ( m_pszKey[0] == '[' )
			goto foundit;
	}

	for (;;)
	{
		if ( !ReadTextLine(true) )
		{
			m_lSectionData = GetPosition();
			return false;
		}
		if ( m_pszKey[0] == '[' )
			break;
	}

foundit:
	// Parse section name
	m_pszKey++;
	size_t iLen = strlen(m_pszKey);
	for ( size_t i = 0; i < iLen; i++ )
	{
		if ( m_pszKey[i] == ']' )
		{
			m_pszKey[i] = '\0';
			break;
		}
	}

	m_lSectionData = GetPosition();
	if ( IsSectionType("EOF") )
		return false;

	Str_Parse(m_pszKey, &m_pszArg);
	return true;
	EXC_CATCH;
	return false;
}

bool CScript::ReadKey(bool fRemoveBlanks)
{
	ADDTOCALLSTACK("CScript::ReadKey");
	if ( !ReadTextLine(fRemoveBlanks) )
		return false;
	if ( m_pszKey[0] == '[' )	// hit the end of section
	{
		m_fSectionHead = true;
		return false;
	}
	return true;
}

bool CScript::ReadKeyParse()
{
	ADDTOCALLSTACK("CScript::ReadKeyParse");
	EXC_TRY("ReadKeyParse");
	EXC_SET("read");
	if ( !ReadKey() )
	{
		EXC_SET("init");
		InitKey();
		return false;	// end of section
	}

	ASSERT(m_pszKey);
	GETNONWHITESPACE(m_pszKey);
	EXC_SET("parse");
	Str_Parse(m_pszKey, &m_pszArg);

	if ( !m_pszArg[0] || ((m_pszArg[1] != '=') && (m_pszArg[1] != '+') && (m_pszArg[1] != '-')) || !strchr(".*+-/%|&!^", m_pszArg[0]) )
		return true;

	EXC_SET("parse");
	LPCTSTR	pszArgs = m_pszArg;
	pszArgs += 2;
	GETNONWHITESPACE(pszArgs);
	TemporaryString buf;

	if ( m_pszArg[0] == '.' )
	{
		if ( *pszArgs == '"' )
		{
			TCHAR *pszQuote = const_cast<TCHAR *>(strchr(pszArgs + 1, '"'));
			if ( pszQuote )
			{
				pszArgs++;
				*pszQuote = '\0';
			}
		}
		sprintf(buf, "<%s>%s", m_pszKey, pszArgs);
	}
	else if ( (m_pszArg[0] == m_pszArg[1]) && (m_pszArg[1] == '+') )
	{
		if ( m_pszArg[2] != '\0' )
			return true;
		sprintf(buf, "<eval (<%s> + 1)>", m_pszKey);
	}
	else if ( (m_pszArg[0] == m_pszArg[1]) && (m_pszArg[1] == '-') )
	{
		if ( m_pszArg[2] != '\0' )
			return true;
		sprintf(buf, "<eval (<%s> - 1)>", m_pszKey);
	}
	else
	{
		sprintf(buf, "<%s (<%s> %c (%s))>", (strnicmp(m_pszKey, "float.", 6) == 0) ? "floatval" : "eval", m_pszKey, *m_pszArg, pszArgs);
	}
	strcpy(m_pszArg, buf);

	return true;
	EXC_CATCH;
	return false;
}

void CScript::Close()
{
	ADDTOCALLSTACK("CScript::Close");
	CCacheableScriptFile::Close();
}

bool _cdecl CScript::WriteSection(LPCTSTR pszSection, ...)
{
	ADDTOCALLSTACK_INTENSIVE("CScript::WriteSection");
	// Write section header
	va_list vargs;
	va_start(vargs, pszSection);

	Printf("\n[");
	VPrintf(pszSection, vargs);
	Printf("]\n");
	va_end(vargs);
	return true;
}

bool CScript::WriteKey(LPCTSTR pszKey, LPCTSTR pszVal)
{
	ADDTOCALLSTACK_INTENSIVE("CScript::WriteKey");
	if ( (pszKey == NULL) || (pszKey[0] == '\0') )
		return false;

	TCHAR ch = '\0';
	TCHAR *pszSep;
	if ( (pszVal == NULL) || (pszVal[0] == '\0') )
	{
		pszSep = const_cast<TCHAR *>(strchr(pszKey, '\n'));
		if ( pszSep == NULL )
			pszSep = const_cast<TCHAR *>(strchr(pszKey, '\r'));

		if ( pszSep != NULL )
		{
			g_Log.Event(LOGL_WARN|LOGM_CHEAT, "carriage return in key (book?) - truncating\n");
			ch = *pszSep;
			*pszSep = '\0';
		}

		// Books are like this, no real keys
		Printf("%s\n", pszKey);

		if ( pszSep != NULL )
			*pszSep = ch;
	}
	else
	{
		pszSep = const_cast<TCHAR *>(strchr(pszVal, '\n'));
		if ( pszSep == NULL )
			pszSep = const_cast<TCHAR *>(strchr(pszVal, '\r'));

		if ( pszSep != NULL )
		{
			g_Log.Event(LOGL_WARN|LOGM_CHEAT, "carriage return in key value - truncating\n");
			ch = *pszSep;
			*pszSep = '\0';
		}

		Printf("%s=%s\n", pszKey, pszVal);

		if ( pszSep != NULL )
			*pszSep = ch;
	}

	return true;
}

void _cdecl CScript::WriteKeyFormat(LPCTSTR pszKey, LPCTSTR pszVal, ...)
{
	ADDTOCALLSTACK_INTENSIVE("CScript::WriteKeyFormat");
	TemporaryString pszTemp;
	va_list vargs;
	va_start(vargs, pszVal);
	_vsnprintf(pszTemp, pszTemp.realLength(), pszVal, vargs);
	WriteKey(pszKey, pszTemp);
	va_end(vargs);
}
