//
//	CDataBase
//		MySQL wrapper for easier data operations witheen in-game server
//
#ifndef CDATABASE_H
#define	CDATABASE_H

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
	CDataBase();
	~CDataBase();

private:
	CDataBase(const CDataBase &copy);
	CDataBase &operator=(const CDataBase &other);

public:
	void Connect();
	void Close();

	bool Query(const char *query, CVarDefMap &mapQueryResult);			// SQL commands that need query result (SELECT)
	bool __cdecl Queryf(CVarDefMap &mapQueryResult, char *fmt, ...) __printfargs(3, 4);

	bool Exec(const char *query);			// SQL commands that doesn't need query result (SET, INSERT, DELETE, ...)
	bool __cdecl Execf(char *fmt, ...) __printfargs(2, 3);

	bool AsyncQueue(bool isQuery, LPCTSTR function, LPCTSTR query);
	void AsyncQueueCallback(CGString &function, CScriptTriggerArgs *result);

	bool OnTick();

	virtual bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);
	virtual bool r_LoadVal(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);

	LPCTSTR GetName() const
	{
		return "SQL_OBJ";
	}

public:
	CVarDefMap m_QueryResult;
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];

private:
	typedef std::pair<CGString, CScriptTriggerArgs *> FunctionArgsPair_t;
	typedef std::queue<FunctionArgsPair_t> QueueFunction_t;

protected:
	bool m_connected;
	MYSQL *m_socket;
	QueueFunction_t m_QueryArgs;

private:
	SimpleMutex m_connectionMutex;
	SimpleMutex m_resultMutex;
};

#endif
