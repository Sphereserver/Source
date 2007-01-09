#ifndef QUEST_H
#define QUEST_H

#include "graysvr.h"

//	quest definition, defined and configured in scripts
class QuestDef : public CScriptObj
{
public:
	enum ObjectiveType
	{
		OBJTYPE_GET,
		OBJTYPE_KILL,
		OBJTYPE_DELIVER,
	};
	enum PrizeType
	{
		PRIZETYPE_ITEM,
	};
	class ResourceTyped
	{
	public:
		int		m_type;
		int		m_amount;
		DWORD	m_ref;

		static LPCTSTR const sm_szLoadKeys[];

	public:
		ResourceTyped();

		const char *Load(const char *s);
		void r_Write(CScript &s);
		bool r_LoadVal(CScript &s);
		bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	};

public:
	vector<ResourceTyped *>	m_objectives;
	vector<ResourceTyped *>	m_prizes;
	CGString				m_textName;
	CGString				m_text;
	CGString				m_textComplete;
	CGString				m_textUncomplete;
	CGString				m_textResign;
	long					m_timelimit;
	UID						m_source;

	static const char *m_sClassName;
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];

public:
	QuestDef();
	LPCTSTR GetName() const { return "QuestDef"; }

	virtual bool r_LoadVal(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	virtual bool r_Verb(CScript & s, CTextConsole *pSrc);
};

//	quest realisation, saved in saves
class Quest : public CScriptObj
{
protected:
	CChar			*m_pChar;

public:
	QuestDef		*m_base;
	vector<QuestDef::ResourceTyped *>	m_objectives;
	bool			m_failed;
	long			m_timeleft;

	static const char *m_sClassName;
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];

public:
	Quest(QuestDef *base, CChar *pChar);
	LPCTSTR GetName() const { return "Quest"; }

	virtual bool r_LoadVal(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	virtual bool r_Verb(CScript & s, CTextConsole *pSrc);
	virtual void r_Write(CScript &s);
	bool OnTick();
};

//	list of quests
extern vector<QuestDef *> g_Quests;

#endif
