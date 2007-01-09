#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "common.h"
#include "threads.h"

extern void ExceptionsThreadInit();

class CError
{
public:
	long		m_severity; // LOGL_TYPE
	DWORD		m_error;
	const char *m_descr;
	DWORD		m_address;

	//	assert information
	const char	*m_exp;
	const char	*m_file;
	long		m_line;

	CError(long eSev, DWORD hErr, LPCTSTR pszDescription);
	CError(const CError &e);

	void GetErrorMessage(LPSTR lpszError);
};

#ifndef ASSERT
extern void Assert_CheckFail(const char *pExp, const char *pFile, long uLine);
#define ASSERT(exp)	(void)( (exp) || (Assert_CheckFail(#exp, __FILE__, __LINE__), 0) )
#endif // ASSERT

// normal exception block
#define EXC_TRY(_a_) \
				CThread *pCurThread = CThread::Thread(); \
				long lCurExcStackPos = pCurThread ? pCurThread->ExcCount() : 0; \
				CCallStack classStackCallInfo(m_sClassName, _a_, pCurThread, lCurExcStackPos); \
				const char *inLocalBlock = ""; \
				char *inLocalArgs = _a_; \
				int inLocalBlockCnt(0); \
				bool bCATCHExcept = false; \
				try \
				{
#define EXC_CATCH \
				} catch ( CError &e ) { bCATCHExcept = true; \
					if ( inLocalBlock ) g_Log.Catch(&e, "%s::%s() #%d \"%s\"", m_sClassName, inLocalArgs, inLocalBlockCnt, inLocalBlock); \
					else g_Log.Catch(&e, "%s::%s() - %s", m_sClassName, inLocalArgs); \
				} catch (...) { bCATCHExcept = true; \
					if ( inLocalBlock ) g_Log.Catch(NULL, "%s::%s() #%d \"%s\"", m_sClassName, inLocalArgs, inLocalBlockCnt, inLocalBlock); \
					else g_Log.Catch(NULL, "%s::%s() - %s", m_sClassName, inLocalArgs); \
				} \
				if ( bCATCHExcept && pCurThread ) pCurThread->ExcStack(); \
				if ( pCurThread ) pCurThread->ExcCount(lCurExcStackPos)
#define EXC_SET(_a_)	\
				if ( pCurThread ) pCurThread->m_action = _a_; \
				inLocalBlock = _a_; \
				inLocalBlockCnt++

#define EXC_DEBUG_START if ( bCATCHExcept ) { try {
#define EXC_DEBUG_END } catch ( ... ) { g_Log.Error("Exception adding debug message on the exception.\n"); }}

// subexception block
#define EXC_TRYSUB(_a_) \
				const char *inLocalSubBlock = _a_; \
				bool bCATCHExceptSub = false; \
				try \
				{
#define EXC_CATCHSUB(_a_)	\
				} catch ( CError &e ) { \
					bCATCHExceptSub = true; \
					g_Log.Catch(&e, "%s::%s", _a_, inLocalSubBlock); \
				} catch (...) { \
					bCATCHExceptSub = true; \
					g_Log.Catch(NULL, "%s::%s", _a_, inLocalSubBlock); \
				}
#define EXC_DEBUGSUB_START if ( bCATCHExceptSub ) { \
				try {
#define EXC_DEBUGSUB_END  } catch ( ... ) { \
					g_Log.Error("Exception adding debug message on the subexception.\n"); \
				} }

// debugging macro
#define EXC_ADD_SCRIPT		g_Log.Debug("command '%s' args '%s'\n", s.GetKey(), s.GetArgRaw())
#define EXC_ADD_SCRIPTSRC	g_Log.Debug("command '%s' args '%s' [0%lx]\n", s.GetKey(), s.GetArgRaw(), pSrc)
#define EXC_ADD_KEYRET(src)	g_Log.Debug("command '%s' ret '%s' [0%lx]\n", pszKey, sVal, src);

#define ADD2STACK(_a_)	CCallStack classStackCallInfo(m_sClassName, _a_)

class CThread;

class CCallStack
{
public:
	long	m_stackpos;
	CThread	*m_thread;

	CCallStack(const char *className, const char *funcName, CThread *thread = NULL, long stackpos = NULL);
	~CCallStack();
};

#endif
