#pragma warning(disable:4786)
#include "../graysvr.h"
#include "VariableList.h"

/***************************************************************************
 *
 *
 *	class VariableList::Variable		Interface for variables
 *
 *
 ***************************************************************************/
VariableList::Variable::Variable(LPCTSTR pszKey)
		: m_Key(pszKey)
{
	m_Key.MakeLower();
}

VariableList::Variable::~Variable()
{
}

LPCTSTR VariableList::Variable::GetKey() const
{
	return m_Key.GetPtr();
}

/***************************************************************************
 *
 *
 *	class VariableList::lstr			KEY part sorting wrapper over std::set
 *
 *
 ***************************************************************************/
bool VariableList::ltstr::operator()(Variable *s1, Variable *s2) const
{
	return ( strcmpi(s1->GetKey(), s2->GetKey()) < 0 );
}

/***************************************************************************
 *
 *
 *	class VariableList			Holds list of pairs KEY = VALUE and operates it
 *
 *
 ***************************************************************************/
VariableList &VariableList::operator=(const VariableList &array)
{
	Copy(&array);
	return *this;
}

VariableList::~VariableList()
{
	Empty();
}

LPCTSTR VariableList::FindValStr( LPCTSTR pVal ) const
{
	for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); i++ )
	{
		const Variable *pVarBase = (*i);
		const VariableString *pVarStr = dynamic_cast <const VariableString *>(pVarBase);
		if ( !pVarStr )
			continue;
		
		if ( !strcmpi(pVal, pVarStr->GetValStr()) )
			return pVarBase->GetKey();
	}
	return NULL;
}

LPCTSTR VariableList::FindValNum(int iVal) const
{
	for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); i++ )
	{
		const Variable *pVarBase = (*i);
		const VariableNumber *pVarNum = dynamic_cast <const VariableNumber *>(pVarBase);
		if ( !pVarNum )
			continue;

		if ( pVarNum->GetValNum() == iVal )
			return pVarBase->GetKey();
	}
	return NULL;
}

VariableList::Variable *VariableList::GetAt(int at)
{
	if ( at > m_Container.size() )
		return NULL;

	DefSet::iterator i = m_Container.begin();
	while ( at-- ) i++;

	if ( i != m_Container.end() )
		return (*i);
	else
		return NULL;
}

VariableList::Variable * VariableList::GetAtKey(LPCTSTR at)
{
	VariableSearcher *pVarBase = new VariableSearcher(at);
	DefSet::iterator i = m_Container.find((Variable*)pVarBase);
	delete pVarBase;

	if ( i != m_Container.end() )
		return (*i);
	else
		return NULL;
}

void VariableList::DeleteAt(int at)
{
	if ( at > m_Container.size() )
		return;

	DefSet::iterator i = m_Container.begin();
	while ( at-- ) i++;

	DeleteAtIterator(i);
}

void VariableList::DeleteAtKey(LPCTSTR at)
{
	VariableString *pVarBased = new VariableString(at);
	DefSet::iterator i = m_Container.find((Variable*)pVarBased);
	delete pVarBased;

	DeleteAtIterator(i);
}

void VariableList::DeleteAtIterator(DefSet::iterator it)
{
	if ( it != m_Container.end() )
	{
		Variable * pVarBase = (*it);
		VariableNumber * pVarNum = NULL;
		VariableString * pVarStr = NULL;
		m_Container.erase(it);

		if ( pVarBase )
		{
			pVarNum = dynamic_cast<VariableNumber *>(pVarBase);
			if (pVarNum)
			{
				delete pVarNum;
			}
			else
			{
				pVarStr = dynamic_cast<VariableString *>(pVarBase);
				delete pVarStr;
			}
		}
	}
}

void VariableList::DeleteKey(LPCTSTR key)
{
	if ( key && *key)
	{
		DeleteAtKey(key);
	}
}

void VariableList::Empty()
{
	DefSet::iterator i = m_Container.begin();
	Variable *pVarBase = NULL;
	VariableNumber *pVarNum = NULL;
	VariableString *pVarStr = NULL;

	while ( i != m_Container.end() )
	{
		pVarBase = (*i);
		m_Container.erase(i); // This don't free all the resource
		i = m_Container.begin();

		if ( pVarBase )
		{
			pVarNum = dynamic_cast<VariableNumber *>(pVarBase);
			if (pVarNum)
			{
				delete pVarNum;
			}
			else
			{
				pVarStr = dynamic_cast<VariableString *>(pVarBase);
				delete pVarStr;
			}
		}
	}
	m_Container.clear();
}

void VariableList::Copy(const VariableList *pArray, bool deep)
{
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

int VariableList::GetCount() const
{
	return m_Container.size();
}

int VariableList::SetNumNew(LPCTSTR pszName, int iVal)
{
	Variable *pVarNum = new VariableNumber(pszName, iVal);
	if ( !pVarNum )
		return false;

	DefPairResult res = m_Container.insert(pVarNum);
	int i = 0;

	i = std::distance(m_Container.begin(), res.first);

	return i;
}

int VariableList::SetNumOverride(LPCTSTR pszKey, int iVal)
{
	DeleteAtKey(pszKey);
	return SetNumNew(pszKey,iVal);
}

int VariableList::SetNum(LPCTSTR pszName, int iVal, bool fZero)
{
	if ( pszName[0] == '\0' )
		return false;

	if ( fZero && (iVal == 0) )
	{
		DeleteAtKey(pszName);
		return -1;
	}

	Variable *pVarBase = GetAtKey(pszName);
	if ( !pVarBase )
	{
		return SetNumNew(pszName, iVal);
	}

	VariableNumber *pVarNum = dynamic_cast <VariableNumber *>(pVarBase);
	if ( pVarNum )
	{
		pVarNum->SetValNum(iVal);
	}
	else
	{
		if ( g_Serv.IsLoading() )
		{
			DEBUG_ERR(("Replace existing VarStr '%s'\n", pVarBase->GetKey()));
		}
		return SetNumOverride(pszName, iVal);
	}
	return true;
}

int VariableList::SetStrNew(LPCTSTR pszName, LPCTSTR pszVal)
{
	Variable * pVarStr = new VariableString(pszName, pszVal);
	if ( !pVarStr )
		return false;

	DefPairResult res = m_Container.insert(pVarStr);
	int i = 0;

	i = std::distance(m_Container.begin(), res.first);

	return i;
}

int VariableList::SetStrOverride(LPCTSTR pszKey, LPCTSTR pszVal)
{
	DeleteAtKey(pszKey);
	return SetStrNew(pszKey,pszVal);
}

int VariableList::SetStr(LPCTSTR pszName, bool fQuoted, LPCTSTR pszVal, bool fZero)
{
	// ASSUME: This has been clipped of unwanted beginning and trailing spaces.
	if ( !pszName || !pszName[0] )
		return -1;

	if ( !pszVal || ( pszVal[0] == '\0' ))	// but not if empty
	{
		DeleteAtKey(pszName);
		return -1;
	}

	if ( !fQuoted && IsSimpleNumberString(pszVal) )
	{
		// Just store the number and not the string.
		return SetNum(pszName, Exp_GetVal(pszVal), fZero);
	}

	Variable * pVarBase = GetAtKey(pszName);
	if ( !pVarBase )
	{
		return SetStrNew(pszName, pszVal);
	}

	VariableString * pVarStr = dynamic_cast <VariableString *>(pVarBase);
	if ( pVarStr )
	{
		pVarStr->SetValStr(pszVal);
	}
	else
	{
		if ( g_Serv.IsLoading() )
		{
			DEBUG_ERR(("Replace existing VarNum '%s' with %s\n", pVarBase->GetKey(), pszVal));
		}
		return SetStrOverride(pszName, pszVal);
	}
	return true;
}

VariableList::Variable * VariableList::GetKey(LPCTSTR pszKey) const
{
	Variable * pReturn = NULL;

	if ( pszKey )
	{
		VariableSearcher *pVarBase = new VariableSearcher(pszKey);
		DefSet::const_iterator i = m_Container.find((Variable*)pVarBase);
		delete pVarBase;
		
		if ( i != m_Container.end() )
			pReturn = (*i);
	}
	return pReturn;
}

int VariableList::GetKeyNum(LPCTSTR pszKey, bool fZero) const
{
	Variable * pVar = GetKey(pszKey);
	if ( !pVar )
		return (fZero ? 0 : NULL);
	return pVar->GetValNum();
}

LPCTSTR VariableList::GetKeyStr(LPCTSTR pszKey, bool fZero) const
{
	Variable * pVar = GetKey(pszKey);
	if ( !pVar )
		return (fZero ? "0" : "");
	return pVar->GetValStr();
}

extern int GetIdentifierString(TCHAR *, LPCTSTR );
VariableList::Variable * VariableList::GetParseKey(LPCTSTR &pszArgs) const
{
	// Skip to the end of the expression name. the name can only be valid.
	TCHAR szTag[256];
	int i = GetIdentifierString(szTag, pszArgs);
	Variable *pVar = GetKey(szTag);
	if ( pVar )
	{
		pszArgs += i;
		return pVar;
	}
	return NULL;
}

bool VariableList::GetParseVal( LPCTSTR & pszArgs, long * plVal ) const
{
	Variable *pVarBase = GetParseKey(pszArgs);
	if ( !pVarBase )
		return false;
	*plVal = pVarBase->GetValNum();
	return true;
}

void VariableList::DumpKeys(CTextConsole * pSrc, LPCTSTR pszPrefix)
{
	if ( pszPrefix == NULL )
		pszPrefix = "";

	for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); i++ )
	{
		const Variable * pVar = (*i);
		pSrc->SysMessagef("%s%s=%s\n", pszPrefix, pVar->GetKey(), pVar->GetValStr());
	}
}

void VariableList::ClearKeys(LPCTSTR mask)
{
	if ( mask && *mask )
	{
		CGString sMask(mask);
		sMask.MakeLower();

		for ( DefSet::iterator i = m_Container.begin(); i != m_Container.end(); i++ )
		{
			Variable * pVar = (*i);
			if ( pVar )
			{
				if ( !strstr(pVar->GetKey(), sMask.GetPtr()) ) 
					continue;

				DeleteAtIterator(i);
				if ( i != m_Container.begin() )
					i--;
			}
		}	
	}
	else
		Empty();
}

bool VariableList::r_LoadVal( CScript & s )
{
	bool fQuoted = false;
	return SetStr(s.GetKey(), fQuoted, s.GetArgStr(&fQuoted)) ? true : false;
}

void VariableList::r_WritePrefix(CScript &s, LPCTSTR pszPrefix)
{
	LPCTSTR		pszVal;
	bool bHasPrefix = (pszPrefix && *pszPrefix);

	// Write with any prefix.
	for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); i++ )
	{
		const Variable *pVar = (*i);
		TEMPSTRING(z);

		if ( bHasPrefix )
			sprintf(z, "%s.%s", pszPrefix, pVar->GetKey());
		else
			sprintf(z, "%s", pVar->GetKey());

		pszVal = pVar->GetValStr();
		const VariableString *pVarStr = dynamic_cast <const VariableString *>(pVar);
		if ( pVarStr )
			s.WriteKeyFormat(z, "\"%s\"", pszVal);
		else
			s.WriteKey(z, pszVal);
	}
}

/***************************************************************************
 *
 *
 *	class VariableList::VariableSearcher	Variable implementation (search-only internal useage)
 *
 *
 ***************************************************************************/
VariableList::VariableSearcher::VariableSearcher(LPCTSTR pszKey)
		: Variable(pszKey)
{
}

VariableList::VariableSearcher::~VariableSearcher()
{
}

LPCTSTR VariableList::VariableSearcher::GetValStr() const
{
	return NULL;
}

int VariableList::VariableSearcher::GetValNum() const
{
	return -1;
}

VariableList::Variable *VariableList::VariableSearcher::CopySelf() const
{
	return new VariableSearcher(GetKey());
}

/***************************************************************************
 *
 *
 *	class VariableList::VariableNumber		Variable implementation (Number)
 *
 *
 ***************************************************************************/
inline LPCTSTR VariableList::VariableNumber::GetValStr() const
{
	TEMPSTRING(pszTmp);
	sprintf(pszTmp, "0%x", m_iVal);
	return pszTmp;
}

VariableList::VariableNumber::VariableNumber(LPCTSTR pszKey, int iVal)
		: Variable(pszKey), m_iVal(iVal)
{
}

VariableList::VariableNumber::VariableNumber(LPCTSTR pszKey)
		: Variable(pszKey)
{
}

VariableList::VariableNumber::~VariableNumber()
{
}

int VariableList::VariableNumber::GetValNum() const
{
	return m_iVal;
}

void VariableList::VariableNumber::SetValNum(int iVal)
{
	m_iVal = iVal;
}

bool VariableList::VariableNumber::r_LoadVal(CScript &s)
{
	SetValNum(s.GetArgVal());
	return true;
}

bool VariableList::VariableNumber::r_WriteVal(LPCTSTR pKey, CGString &sVal, CTextConsole *pSrc)
{
	UNREFERENCED_PARAMETER(pKey);
	UNREFERENCED_PARAMETER(pSrc);
	sVal.FormatVal(GetValNum());
	return true;
}

VariableList::Variable *VariableList::VariableNumber::CopySelf() const
{
	return new VariableNumber(GetKey(), m_iVal);
}

/***************************************************************************
 *
 *
 *	class VariableList::VariableString		Variable implementation (String)
 *
 *
 ***************************************************************************/
inline int VariableList::VariableString::GetValNum() const
{
	LPCTSTR pszStr = m_sVal;
	return Exp_GetVal(pszStr);
}

VariableList::VariableString::VariableString(LPCTSTR pszKey, LPCTSTR pszVal)
		: Variable(pszKey), m_sVal(pszVal)
{
}

VariableList::VariableString::VariableString(LPCTSTR pszKey)
		: Variable(pszKey)
{
}

VariableList::VariableString::~VariableString()
{
}

LPCTSTR VariableList::VariableString::GetValStr() const
{
	return m_sVal;
}

void VariableList::VariableString::SetValStr(LPCTSTR pszVal)
{
	m_sVal.Copy(pszVal);
}

bool VariableList::VariableString::r_LoadVal(CScript &s)
{
	SetValStr(s.GetArgStr());
	return true;
}

bool VariableList::VariableString::r_WriteVal(LPCTSTR pKey, CGString &sVal, CTextConsole *pSrc)
{
	UNREFERENCED_PARAMETER(pKey);
	UNREFERENCED_PARAMETER(pSrc);
	sVal = GetValStr();
	return true;
}

VariableList::Variable *VariableList::VariableString::CopySelf() const
{
	return new VariableString(GetKey(), m_sVal);
}
