#include "asyncdb.h"

CDataBaseAsyncHelper g_asyncHdb;

CDataBaseAsyncHelper::CDataBaseAsyncHelper(void) : AbstractSphereThread("AsyncDatabaseHelper", IThread::Low)
{
}

CDataBaseAsyncHelper::~CDataBaseAsyncHelper(void)
{
}

void CDataBaseAsyncHelper::onStart()
{
	AbstractSphereThread::onStart();
}

void CDataBaseAsyncHelper::tick()
{
	if ( !m_queriesTodo.empty() )
	{
		SimpleThreadLock lock(m_queryMutex);
		QueryBlob_t currentPair = m_queriesTodo.front();
		m_queriesTodo.pop_front();
		FunctionQueryPair_t currentFunctionPair = currentPair.second;

		CScriptTriggerArgs *args = new CScriptTriggerArgs();
		args->m_iN1 = currentPair.first;
		args->m_s1 = currentFunctionPair.second;

		if ( currentPair.first )
			args->m_iN2 = g_Serv.m_hdb.Query(currentFunctionPair.second, args->m_VarsLocal);
		else
			args->m_iN2 = g_Serv.m_hdb.Exec(currentFunctionPair.second);

		g_Serv.m_hdb.AsyncQueueCallback(currentFunctionPair.first, args);
	}
}

void CDataBaseAsyncHelper::waitForClose()
{
	SimpleThreadLock lock(m_queryMutex);
	m_queriesTodo.clear();
	AbstractSphereThread::waitForClose();
}

void CDataBaseAsyncHelper::addQuery(bool isQuery, LPCTSTR function, LPCTSTR query)
{
	SimpleThreadLock lock(m_queryMutex);
	m_queriesTodo.push_back(QueryBlob_t(isQuery, FunctionQueryPair_t(CGString(function), CGString(query))));
}
