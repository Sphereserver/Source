#ifndef _INC_CDATABASE_H
#define _INC_CDATABASE_H
#pragma once

#include "graycom.h"
#include "CScriptObj.h"
#include "../sphere/mutex.h"
#include "mysql/include/mysql.h"
#include "mysql/include/errmsg.h"

#ifdef _WIN32
	#pragma comment(lib, "libmysql")
#else
	#pragma comment(lib, "libmysqlclient")
#endif

class CDataBase : public CScriptObj
{
public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];

	CDataBase();
	~CDataBase();

private:
	typedef std::pair<CGString, CScriptTriggerArgs *> FunctionArgsPair_t;
	typedef std::queue<FunctionArgsPair_t> QueueFunction_t;

protected:
	MYSQL *m_socket;
	QueueFunction_t m_QueryArgs;

public:
	CVarDefMap m_QueryResult;

private:
	SimpleMutex m_connectionMutex;
	SimpleMutex m_resultMutex;

public:
	void Connect();
	void Close();

	bool Query(LPCTSTR pszQuery, CVarDefMap &mapQueryResult);		// SQL commands that need query result (SELECT)
	bool __cdecl Queryf(CVarDefMap &mapQueryResult, char *fmt, ...) __printfargs(3, 4);

	bool Exec(LPCTSTR pszQuery);		// SQL commands that doesn't need query result (SET, INSERT, DELETE, ...)
	bool __cdecl Execf(char *fmt, ...) __printfargs(2, 3);

	bool AsyncQueue(bool fQuery, LPCTSTR pszFunction, LPCTSTR pszQuery);
	void AsyncQueueCallback(CGString &sFunction, CScriptTriggerArgs *Args);

	bool OnTick();

	virtual bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);
	virtual bool r_LoadVal(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);

	LPCTSTR GetName() const
	{
		return "SQL_OBJ";
	}

private:
	CDataBase(const CDataBase &copy);
	CDataBase &operator=(const CDataBase &other);
};

#endif	// _INC_CDATABASE_H
