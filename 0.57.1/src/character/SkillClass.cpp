#include "SkillClass.h"

/***************************************************************************
 *
 *
 *	class SkillClass				Player skill and class specialty
 *
 *
 ***************************************************************************/
const char *SkillClass::m_sClassName = "SkillClass";

enum SCC_TYPE
{
	SCC_DEFNAME,
	SCC_NAME,
	SCC_SKILLSUM,
	SCC_STATSUM,
	SCC_QTY,
};

LPCTSTR const SkillClass::sm_szLoadKeys[SCC_QTY+1] =
{
	"DEFNAME",
	"NAME",
	"SKILLSUM",
	"STATSUM",
	NULL,
};

void SkillClass::Init()
{
	m_SkillSumMax = 10*1000;
	m_StatSumMax = 300;
	int i;
	for ( i=0; i<COUNTOF(m_SkillLevelMax); i++ )
	{
		m_SkillLevelMax[i] = 1000;
	}
	for ( i=0; i<COUNTOF(m_StatMax); i++ )
	{
		m_StatMax[i] = 100;
	}
}

bool SkillClass::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	EXC_TRY("WriteVal");
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	case SCC_NAME: // "NAME"
		sVal = m_sName;
		break;
	case SCC_SKILLSUM:
		sVal.FormatVal( m_SkillSumMax );
		break;
	case SCC_STATSUM:
		sVal.FormatVal( m_StatSumMax );
		break;
	default:
		{
			int i = g_Cfg.FindSkillKey( pszKey);
			if ( i != SKILL_NONE )
			{
				sVal.FormatVal( m_SkillLevelMax[i] );
				break;
			}
			i = g_Cfg.FindStatKey( pszKey);
			if ( i >= 0 )
			{
				sVal.FormatVal( m_StatMax[i] );
				break;
			}
		}
		return CResourceDef::r_WriteVal(pszKey, sVal, pSrc);
	}
	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}


bool SkillClass::r_LoadVal( CScript &s )
{
	EXC_TRY("LoadVal");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	case SCC_DEFNAME:
		return SetResourceName( s.GetArgStr());
	case SCC_NAME:
		m_sName = s.GetArgStr();
		break;
	case SCC_SKILLSUM:
		m_SkillSumMax = s.GetArgVal();
		break;
	case SCC_STATSUM:
		m_StatSumMax = s.GetArgVal();
		break;
	default:
		{
			int i = g_Cfg.FindSkillKey( s.GetKey());
			if ( i != SKILL_NONE )
			{
				m_SkillLevelMax[i] = s.GetArgVal();
				break;
			}
			i = g_Cfg.FindStatKey( s.GetKey());
			if ( i >= 0 )
			{
				m_StatMax[i] = s.GetArgVal();
				break;
			}
		}
		return CResourceDef::r_LoadVal(s);
	}
	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

SkillClass::SkillClass(RESOURCE_ID rid)
		: CResourceLink(rid)
{
	Init();
}

SkillClass::~SkillClass()
{
}

LPCTSTR SkillClass::GetName() const
{
	return m_sName;
}
