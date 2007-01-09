#include "Profession.h"

/***************************************************************************
 *
 *
 *	class Profession				Player profession specialty
 *
 *
 ***************************************************************************/
const char *Profession::m_sClassName = "Profession";

enum PRC_TYPE
{
	PRC_ADVANCE,
	PRC_DEFNAME,
	PRC_LOSS,
	PRC_NAME,
	PRC_QTY,
};

LPCTSTR const Profession::sm_szLoadKeys[PRC_QTY+1] =
{
	"ADVANCE",
	"DEFNAME",
	"LOSS",	
	"NAME",
	NULL,
};

Profession::Profession(RESOURCE_ID rid)
		: CResourceLink(rid)
{
	init();
}

Profession::~Profession()
{
}

LPCTSTR Profession::GetName() const
{
	return (LPCTSTR)m_sName;
}

bool Profession::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	EXC_TRY("WriteVal");
	switch ( FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys )-1) )
	{
	case PRC_NAME:
		sVal = m_sName;
		break;

	case PRC_ADVANCE:
		{
			TEMPSTRING(buf);
			for ( int i = 0; i < m_skillsToAdvance.size(); i++ )
			{
				const CSkillDef *skill = m_skillsToAdvance.at(i);
				if ( ! *buf ) strcat(buf, ",");
				strcat(buf, skill->GetName());
			}
			sVal.Copy(buf);
		} break;

	case PRC_LOSS:
		{
			TEMPSTRING(buf);
			for ( int i = 0; i < m_skillsToLoss.size(); i++ )
			{
				const CSkillDef *skill = m_skillsToLoss.at(i);
				if ( ! *buf ) strcat(buf, ",");
				strcat(buf, skill->GetName());
			}
			sVal.Copy(buf);
		} break;

	default:
		return CResourceDef::r_WriteVal(pszKey, sVal, pSrc);
	}
	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool Profession::r_LoadVal(CScript &s)
{
	EXC_TRY("LoadVal");
	int index = FindTableSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1);
	switch ( index )
	{
	case PRC_DEFNAME:
		SetResourceName(s.GetArgStr());
		break;

	case PRC_NAME:
		m_sName = s.GetArgStr();
		break;

	case PRC_ADVANCE:
	case PRC_LOSS:
		{
			TCHAR *ppArgs[10];
			int argn = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), ",");
			while ( argn > 0 )
			{
				--argn;
				char *skillName = ppArgs[argn];
				const CSkillDef *skill = g_Cfg.SkillLookup(skillName);
				if ( skill == NULL )
				{
					g_Log.Error("Bad skill name '%s' specified.\n", skillName);
				}
				else
				{
					if ( index == PRC_ADVANCE ) m_skillsToAdvance.push_back(skill);
					else						m_skillsToLoss.push_back(skill);
				}
			}
		} break;

	default:
		return CResourceDef::r_LoadVal(s);
	}
	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

void Profession::init()
{
	m_skillsToAdvance.clear();
	m_skillsToLoss.clear();
}
