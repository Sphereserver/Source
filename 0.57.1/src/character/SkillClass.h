#ifndef CHARACTER_SKILLCLASS_H
#define CHARACTER_SKILLCLASS_H

#include "../graysvr.h"

/***************************************************************************
 *
 *
 *	class SkillClass				Player skill and class specialty
 *
 *
 ***************************************************************************/

class SkillClass : public CResourceLink
{
public:
	static LPCTSTR const sm_szLoadKeys[];
	static const char *m_sClassName;
	CGString m_sName;

	WORD m_StatSumMax;
	DWORD m_SkillSumMax;

	WORD m_StatMax[STAT_BASE_QTY];
	WORD m_SkillLevelMax[SKILL_QTY];

public:
	SkillClass(RESOURCE_ID rid);
	virtual ~SkillClass();
	LPCTSTR GetName() const;

	bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	bool r_LoadVal(CScript &s);

private:
	void Init();
};

#endif
