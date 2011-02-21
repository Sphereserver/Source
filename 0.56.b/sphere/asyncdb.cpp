
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
	stlqueryLock.setMutex(&m_queryMutex);
}

void CDataBaseAsyncHelper::tick()
{
	if ( !m_queriesTodo.empty() )
	{
		stlqueryLock.doLock();

		QueryBlob_t currentPair = m_queriesTodo.front();
		m_queriesTodo.pop_front();

		stlqueryLock.doUnlock();

		FunctionQueryPair_t currentFunctionPair = currentPair.second;

		CScriptTriggerArgs * theArgs = new CScriptTriggerArgs();
		theArgs->m_iN1 = currentPair.first;
		theArgs->m_s1 = currentFunctionPair.second;

		if ( currentPair.first )
			theArgs->m_iN2 = g_Serv.m_hdb.query(currentFunctionPair.second, theArgs->m_VarsLocal);
		else
			theArgs->m_iN2 = g_Serv.m_hdb.exec(currentFunctionPair.second);

		g_Serv.m_hdb.addQueryResult(currentFunctionPair.first, theArgs);
	}
}

void CDataBaseAsyncHelper::waitForClose()
{
	{
		SimpleThreadLock stlThelock(m_queryMutex);

		m_queriesTodo.clear();
	}

	AbstractSphereThread::waitForClose();
}

void CDataBaseAsyncHelper::addQuery(bool isQuery, LPCTSTR sFunction, LPCTSTR sQuery)
{
	SimpleThreadLock stlThelock(m_queryMutex);

	m_queriesTodo.push_back( QueryBlob_t(isQuery, FunctionQueryPair_t(CGString(sFunction), CGString(sQuery))) );
}
