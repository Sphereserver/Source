#ifndef CHARACTER_PROFESSION_H
#define CHARACTER_PROFESSION_H

#include "../graysvr.h"

/***************************************************************************
 *
 *
 *	class Profession				Player profession specialty
 *
 *
 ***************************************************************************/

class Profession : public CResourceLink
{
public:
	static LPCTSTR const sm_szLoadKeys[];
	static const char *m_sClassName;
	CGString m_sName;

public:
	Profession(RESOURCE_ID rid);
	~Profession();

	LPCTSTR GetName() const;

	bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	bool r_LoadVal(CScript &s);

protected:
	vector<const CSkillDef *>	m_skillsToAdvance;
	vector<const CSkillDef *>	m_skillsToLoss;

	void init();
};

#endif
