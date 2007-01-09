#include "quest.h"

vector<QuestDef *> g_Quests;

//		***		***			***
//
//		QuestDef		ResourceTyped
//
//		***		***			***

enum REST_TYPELIST
{
	REST_AMOUNT,
	REST_REF,
	REST_TYPE,
	REST_QTY,
};

LPCTSTR const QuestDef::ResourceTyped::sm_szLoadKeys[REST_QTY+1] =
{
	"AMOUNT",
	"REF",
	"TYPE",
	NULL,
};

QuestDef::ResourceTyped::ResourceTyped() : m_type(0), m_amount(0), m_ref(0)
{
}

const char *QuestDef::ResourceTyped::Load(const char *s)
{
	char z[256];
	char *p = z;

	strcpy(z, s);
	m_type = Exp_GetVal(p);
	p++;
	m_amount = Exp_GetVal(p);
	p++;
	m_ref = Exp_GetVal(p);

	return ( s + ( p - z ));
}

void QuestDef::ResourceTyped::r_Write(CScript &s)
{
	char	z[32];
	sprintf(z, "%d:%d:0%x", m_type, m_amount, m_ref);
	s.WriteString(z);
}

bool QuestDef::ResourceTyped::r_LoadVal(CScript &s)
{
	LPCTSTR pszKey = s.GetKey();
	EXC_TRY("LoadVal");

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1);
	switch ( index )
	{
		case REST_AMOUNT:
			m_amount = s.GetArgVal();
			break;
		case REST_REF:
			m_ref = s.GetArgVal();
			break;
		case REST_TYPE:
			m_type = s.GetArgVal();
			break;

		default:
			return false;
	}

	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool QuestDef::ResourceTyped::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	EXC_TRY("WriteVal");

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1);
	switch ( index )
	{
		case REST_AMOUNT:
			sVal.FormatVal(m_amount);
			break;
		case REST_REF:
			sVal.FormatHex(m_ref);
			break;
		case REST_TYPE:
			sVal.FormatVal(m_type);
			break;

		default:
			return false;
	}

	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.Debug("command '%s' [%x]\n", pszKey, pSrc);
	EXC_DEBUG_END;
	return false;
}

//		***		***			***
//
//		QuestDef
//
//		***		***			***

enum QUESTDEFV_TYPE
{
	QUESTDEFV_START,
	QUESTDEFV_QTY,
};

LPCTSTR const QuestDef::sm_szVerbKeys[QUESTDEFV_QTY+1] =
{
	"START",
	NULL,
};

enum QUESTDEF_TYPE
{
	QUESTDEF_COMPLETE,
	QUESTDEF_NAME,
	QUESTDEF_OBJECTIVE,
	QUESTDEF_PRIZE,
	QUESTDEF_RESIGN,
	QUESTDEF_SOURCE,
	QUESTDEF_STATUS,
	QUESTDEF_TEXT,
	QUESTDEF_TIMELIMIT,
	QUESTDEF_UNCOMPLETE,
	QUESTDEF_QTY,
};

LPCTSTR const QuestDef::sm_szLoadKeys[QUESTDEF_QTY+1] =
{
	"COMPLETE",
	"NAME",
	"OBJECTIVE",
	"PRIZE",
	"RESIGN",
	"SOURCE",
	"STATUS",
	"TEXT",
	"TIMELIMIT",
	"UNCOMPLETE",
	NULL,
};

QuestDef::QuestDef()
{
	m_timelimit = 0;
}

bool QuestDef::r_LoadVal(CScript &s)
{
	LPCTSTR pszKey = s.GetKey();
	EXC_TRY("LoadVal");

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1);
	switch ( index )
	{
		case QUESTDEF_COMPLETE:
			m_textComplete = s.GetArgStr();
			break;
		case QUESTDEF_NAME:
			m_textName = s.GetArgStr();
			break;
		case QUESTDEF_RESIGN:
			m_textResign = s.GetArgStr();
			break;
		case QUESTDEF_TEXT:
			m_text = s.GetArgStr();
			break;
		case QUESTDEF_UNCOMPLETE:
			m_textUncomplete = s.GetArgStr();
			break;

		case QUESTDEF_OBJECTIVE:
			if ( g_Serv.m_iModeCode == SERVMODE_Loading )
			{
				QuestDef::ResourceTyped *res = new QuestDef::ResourceTyped();
				res->Load(s.GetArgRaw());
				m_objectives.push_back(res);
			}
			else
			{
				pszKey += strlen(sm_szLoadKeys[index]);
				if ( *pszKey == '.' )
				{
					pszKey++;
					int id = Exp_GetVal(pszKey);
					if (( id < 0 ) || ( id >= m_objectives.size() ))
						return false;
					SKIP_SEPARATORS(pszKey);

					CScript script(pszKey, s.GetArgRaw());
					return m_objectives[id]->r_LoadVal(script);
				}
			}
			break;
		case QUESTDEF_PRIZE:
			if ( g_Serv.m_iModeCode == SERVMODE_Loading )
			{
				QuestDef::ResourceTyped *res = new QuestDef::ResourceTyped();
				res->Load(s.GetArgRaw());
				m_prizes.push_back(res);
			}
			else
			{
				pszKey += strlen(sm_szLoadKeys[index]);
				if ( *pszKey == '.' )
				{
					pszKey++;
					int id = Exp_GetVal(pszKey);
					if (( id < 0 ) || ( id >= m_prizes.size() ))
						return false;
					SKIP_SEPARATORS(pszKey);

					CScript script(pszKey, s.GetArgRaw());
					return m_prizes[id]->r_LoadVal(script);
				}
			}
			break;

		case QUESTDEF_SOURCE:
			m_source.SetPrivateUID(s.GetArgVal());
			break;

		case QUESTDEF_TIMELIMIT:
			m_timelimit = s.GetArgVal();
			break;

		default:
			return false;
	}

	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool QuestDef::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	EXC_TRY("WriteVal");

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1);
	switch ( index )
	{
		case QUESTDEF_COMPLETE:
			sVal = m_textComplete;
			break;
		case QUESTDEF_NAME:
			sVal = m_textName;
			break;
		case QUESTDEF_RESIGN:
			sVal = m_textResign;
			break;
		case QUESTDEF_TEXT:
			sVal = m_text;
			break;
		case QUESTDEF_UNCOMPLETE:
			sVal = m_textUncomplete;
			break;

		case QUESTDEF_OBJECTIVE:
			pszKey += strlen(sm_szLoadKeys[index]);
			if ( *pszKey == '.' )
			{
				pszKey++;
				int id = Exp_GetVal(pszKey);
				if (( id < 0 ) || ( id >= m_objectives.size() ))
					return false;
				SKIP_SEPARATORS(pszKey);

				return m_objectives[id]->r_WriteVal(pszKey, sVal, pSrc);
			}
			else
				sVal.FormatVal(m_objectives.size());
			break;
		case QUESTDEF_PRIZE:
			pszKey += strlen(sm_szLoadKeys[index]);
			if ( *pszKey == '.' )
			{
				pszKey++;
				int id = Exp_GetVal(pszKey);
				if (( id < 0 ) || ( id >= m_prizes.size() ))
					return false;
				SKIP_SEPARATORS(pszKey);

				return m_prizes[id]->r_WriteVal(pszKey, sVal, pSrc);
			}
			else
				sVal.FormatVal(m_prizes.size());
			break;

		case QUESTDEF_SOURCE:
			if ( !m_source.CharFind() )
				sVal.FormatVal(0);
			else
			{
				pszKey += strlen(sm_szLoadKeys[index]);
				if ( *pszKey == '.' )
				{
					pszKey++;
					return m_source.CharFind()->r_WriteVal(pszKey, sVal, pSrc);
				}
				else
					sVal.FormatVal(m_source);
			}
			break;

		case QUESTDEF_STATUS:
			{
				CCharPlayer *player = pSrc->GetChar()->m_pPlayer;
				if ( !player )
					return false;

				vector<Quest *>::iterator	it;
				int index = 0;
				for ( it = player->m_quests.begin(); it != player->m_quests.end(); it++ )
				{
					if ( (*it)->m_base == this )
					{
						sVal.FormatVal(index);
						return true;
					}
					index++;
				}

				index = 1000;
				for ( it = player->m_questsHistory.begin(); it != player->m_questsHistory.end(); it++ )
				{
					if ( (*it)->m_base == this )
					{
						sVal.FormatVal(index);
						return true;
					}
					index++;
				}

				sVal.FormatVal(-1);
			} break;

		case QUESTDEF_TIMELIMIT:
			sVal.FormatVal(m_timelimit);
			break;

		default:
			return false;
	}

	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.Debug("command '%s' [%x]\n", pszKey, pSrc);
	EXC_DEBUG_END;
	return false;
}

bool QuestDef::r_Verb(CScript &s, CTextConsole *pSrc)
{
	EXC_TRY("Verb");

	CCharPlayer *pPlayer = pSrc->GetChar()->m_pPlayer;
	if ( !pPlayer )
		return false;

	int index = FindTableSorted(s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys)-1);
	switch ( index )
	{
		case QUESTDEFV_START:
			if ( pPlayer->m_quests.size() >= 10 )
				pSrc->SysMessage("You are already doing too many tasks, come back when you have done some of them.");
			else
			{
				pSrc->SysMessage("You accept the quest.");

				Quest *quest = new Quest(this, pSrc->GetChar());
				vector<QuestDef::ResourceTyped *>::iterator res;
				for ( res = m_objectives.begin(); res != m_objectives.end(); res++ )
				{
					QuestDef::ResourceTyped *copyed = new QuestDef::ResourceTyped();
					copyed->m_amount = (*res)->m_amount;
					copyed->m_type = (*res)->m_type;
					copyed->m_ref = (*res)->m_ref;
				}

				pPlayer->m_quests.push_back(quest);
			}
			break;

		default:
			return false;
	}

	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.Debug("command '%s' args '%s' [%x]\n", s.GetKey(), s.GetArgRaw(), pSrc);
	EXC_DEBUG_END;
	return false;
}

//		***		***			***
//
//		Quest
//
//		***		***			***

enum QUESTV_TYPE
{
	QUESTV_CANCEL,
	QUESTV_FINISH,
	QUESTV_QTY,
};

LPCTSTR const Quest::sm_szVerbKeys[QUESTV_QTY+1] =
{
	"CANCEL",
	"FINISH",
	NULL,
};

enum QUEST_TYPE
{
	QUEST_FAILED,
	QUEST_OBJECTIVE,
	QUEST_TIMELEFT,
	QUEST_QTY,
};

LPCTSTR const Quest::sm_szLoadKeys[QUEST_QTY+1] =
{
	"FAILED",
	"OBJECTIVE",
	"TIMELEFT",
	NULL,
};

Quest::Quest(QuestDef *base, CChar *pChar) : m_base(base), m_pChar(pChar)
{
	m_failed = false;
	m_timeleft = -1;
}

bool Quest::r_LoadVal(CScript &s)
{
	LPCTSTR pszKey = s.GetKey();
	EXC_TRY("LoadVal");

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1);
	if ( index == -1 )
	{
		return m_base->r_LoadVal(s);
	}

	switch ( index )
	{
		case QUEST_FAILED:
			m_failed = s.GetArgVal();
			break;

		case QUEST_OBJECTIVE:
			pszKey += strlen(sm_szLoadKeys[index]);
			if ( *pszKey == '.' )
			{
				pszKey++;
				int id = Exp_GetVal(pszKey);
				if (( id < 0 ) || ( id >= m_objectives.size() ))
					return false;
				SKIP_SEPARATORS(pszKey);

				CScript script(pszKey, s.GetArgRaw());
				return m_objectives[id]->r_LoadVal(script);
			}
			break;

		case QUEST_TIMELEFT:
			m_timeleft = s.GetArgVal();
			break;

		default:
			return false;
	}

	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool Quest::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	EXC_TRY("WriteVal");

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1);
	if ( index == -1 )
	{
		return m_base->r_WriteVal(pszKey, sVal, pSrc);
	}

	switch ( index )
	{
		case QUEST_FAILED:
			sVal.FormatVal(m_failed);
			break;

		case QUEST_OBJECTIVE:
			pszKey += strlen(sm_szLoadKeys[index]);
			if ( *pszKey == '.' )
			{
				pszKey++;
				int id = Exp_GetVal(pszKey);
				if (( id < 0 ) || ( id >= m_objectives.size() ))
					return false;
				SKIP_SEPARATORS(pszKey);

				return m_objectives[id]->r_WriteVal(pszKey, sVal, pSrc);
			}
			else
				sVal.FormatVal(m_objectives.size());
			break;

		case QUEST_TIMELEFT:
			sVal.FormatVal(m_timeleft);
			break;

		default:
			return false;
	}

	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.Debug("command '%s' [%x]\n", pszKey, pSrc);
	EXC_DEBUG_END;
	return false;
}

bool Quest::r_Verb(CScript &s, CTextConsole *pSrc)
{
	EXC_TRY("Verb");

	CCharPlayer *pPlayer = pSrc->GetChar()->m_pPlayer;
	if ( !pPlayer )
		return false;

	int index = FindTableSorted(s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys)-1);
	switch ( index )
	{
		case QUESTV_FINISH:
			{
				Quest *quest = new Quest(m_base, m_pChar);
				pPlayer->m_questsHistory.push_back(quest);
			}

		case QUESTV_CANCEL:
			{
				vector<Quest *>::iterator it;
				for ( it = pPlayer->m_quests.begin(); it != pPlayer->m_quests.end(); it++ )
				{
					if ( *it == this )
					{
						pPlayer->m_quests.erase(it);
						delete this;
						return true;
					}
				}
			}
			break;

		default:
			return false;
	}

	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.Debug("command '%s' args '%s' [%x]\n", s.GetKey(), s.GetArgRaw(), pSrc);
	EXC_DEBUG_END;
	return false;
}

void Quest::r_Write(CScript &s)
{
	char z[128];
	vector<QuestDef *>::iterator qd;
	int index = 0;
	for ( qd = g_Quests.begin(); qd != g_Quests.end(); qd++ )
	{
		if ( *qd == m_base )
			break;
		index++;
	}
	sprintf(z, "QUEST%d=%d,%d,", index, m_failed, m_timeleft);
	s.WriteString(z);

	vector<QuestDef::ResourceTyped *>::iterator it;
	for ( it = m_objectives.begin(); it != m_objectives.end(); it++ )
	{
		if ( it != m_objectives.begin() )
			s.WriteString(",");

		(*it)->r_Write(s);
	}

	s.WriteString("\n");
}

bool Quest::OnTick()
{
	EXC_TRY("Tick");

	//	quest has time limitation
	if ( m_timeleft != -1 )
	{
		m_timeleft--;
		if ( m_timeleft == -1 )
		{
			m_failed = true;
			m_pChar->SysMessage("A quest objective has failed.");
		}
	}

	return true;
	EXC_CATCH;
	return false;
}
