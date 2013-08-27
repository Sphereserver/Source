#include "common.h"
#include "../graysvr/graysvr.h"

struct T_TRIGGERS
{
	char	m_name[48];
	long	m_used;
};

std::vector<T_TRIGGERS> g_triggers;

bool IsTrigUsed(E_TRIGGERS id)
{
	return (( id < g_triggers.size() ) && g_triggers[id].m_used );
}

bool IsTrigUsed(const char *name)
{
	std::vector<T_TRIGGERS>::iterator it;
	for ( it = g_triggers.begin(); it != g_triggers.end(); it++ )
	{
		if ( !strcmpi(it->m_name, name) )
			return it->m_used;
	}
	return false;
}

void TriglistInit()
{
	T_TRIGGERS	trig;
	g_triggers.clear();

#define ADD(_a_,_b_)	strcpy(trig.m_name, _b_); trig.m_used = 0; g_triggers.push_back(trig);
#include "../tables/triggers.tbl"
#undef ADD
}

void TriglistClear()
{
	std::vector<T_TRIGGERS>::iterator it;
	for ( it = g_triggers.begin(); it != g_triggers.end(); it++ )
	{
		(*it).m_used = 0;
	}
}

void TriglistAdd(E_TRIGGERS id)
{
	if ( g_triggers.size() )
		g_triggers[id].m_used++;
}

void TriglistAdd(const char *name)
{
	std::vector<T_TRIGGERS>::iterator it;
	for ( it = g_triggers.begin(); it != g_triggers.end(); it++ )
	{
		if ( !strcmpi(it->m_name, name) )
		{
			it->m_used++;
			break;
		}
	}
}

void Triglist(long &total, long &used)
{
	total = used = 0;
	std::vector<T_TRIGGERS>::iterator it;
	for ( it = g_triggers.begin(); it != g_triggers.end(); it++ )
	{
		total++;
		if ( it->m_used )
			used++;
	}
}

void TriglistPrint()
{
	std::vector<T_TRIGGERS>::iterator it;
	for ( it = g_triggers.begin(); it != g_triggers.end(); it++ )
	{
		if ( it->m_used )
		{
			g_Serv.SysMessagef("Trigger %s : used %d time%s.\n", it->m_name, it->m_used, (it->m_used > 1) ? "s" : "");
		} 
		else
		{
			g_Serv.SysMessagef("Trigger %s : NOT used.\n", it->m_name);
		}
	}
}