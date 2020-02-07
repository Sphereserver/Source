#ifndef _CEXCEPTION_H
#define _CEXCEPTION_H

#include "../sphere/threads.h"
#include "../graysvr/CLog.h"

// Enable advanced exceptions catching. Consumes some more resources, but is very useful
// for debug on a running environment. Also it makes sphere more stable since exceptions
// are local
#ifndef _DEBUG
	#ifndef EXCEPTIONS_DEBUG
		#define EXCEPTIONS_DEBUG
	#endif
#endif

extern "C"
{
	extern void globalstartsymbol();
	extern void globalendsymbol();
	extern const int globalstartdata;
	extern const int globalenddata;
}

#ifdef _WIN32
	#if !defined(__MINGW32__) && !defined(_DEBUG)
		void SetExceptionTranslator();
	#endif
#else
	void SetSignals(bool bSet);
#endif

///////////////////////////////////////////////////////////

class CGrayError
{
	// Throw this structure to produce an error
public:
	CGrayError(LOGL_TYPE eSev, DWORD hErr, LPCTSTR pszDescription);
	CGrayError(const CGrayError &e);	// copy constructor needed
	virtual ~CGrayError() { };

public:
	LOGL_TYPE m_eSeverity;	// const
	DWORD m_hError;			// HRESULT S_OK, "winerror.h" code (0x20000000 = start of custom codes)
	LPCTSTR m_pszDescription;

public:
#ifdef _WIN32
	static int GetSystemErrorMessage(DWORD dwError, LPTSTR lpszError, DWORD dwMaxError);
#endif
	virtual bool GetErrorMessage(LPTSTR lpszError) const;

public:
	CGrayError &operator=(const CGrayError &other);
};

class CGrayAssert : public CGrayError
{
public:
	static const char *m_sClassName;

	CGrayAssert(LOGL_TYPE eSeverity, LPCTSTR pszExp, LPCTSTR pszFile, long lLine);
	virtual ~CGrayAssert() { }; 

protected:
	const LPCTSTR m_pszExp;
	const LPCTSTR m_pszFile;
	const long m_lLine;

public:
	virtual bool GetErrorMessage(LPTSTR lpszError) const;

private:
	CGrayAssert &operator=(const CGrayAssert &other);
};

#ifdef _WIN32

	// Catch and get details on the system exceptions.
class CGrayException : public CGrayError
{
public:
	static const char *m_sClassName;

	CGrayException(unsigned int uCode, DWORD dwAddress);
	virtual ~CGrayException() { };

public:
	const DWORD m_dwAddress;

public:
	virtual bool GetErrorMessage(LPTSTR lpszError) const;

private:
	CGrayException &operator=(const CGrayException &other);
};

#endif

///////////////////////////////////////////////////////////

// Exceptions debugging routine
#ifdef EXCEPTIONS_DEBUG

	#define EXC_TRY(a)			LPCTSTR pszLocalBlock = ""; \
								LPCTSTR pszLocalArgs = a; \
								unsigned int uiLocalBlockCnt = 0; \
								bool fCatchExcept = false; \
								UNREFERENCED_PARAMETER(fCatchExcept); \
								try {

	#define EXC_SET(a)			pszLocalBlock = a; ++uiLocalBlockCnt

	#ifdef _THREAD_TRACK_CALLSTACK
		#define EXC_CATCH_EVENT(a)			fCatchExcept = true; \
											StackDebugInformation::printStackTrace(); \
											if ( (pszLocalBlock != NULL) && (uiLocalBlockCnt > 0) ) \
												g_Log.CatchEvent(a, "%s::%s() #%u \"%s\"", m_sClassName, pszLocalArgs, uiLocalBlockCnt, pszLocalBlock); \
											else \
												g_Log.CatchEvent(a, "%s::%s()", m_sClassName, pszLocalArgs)
	#else
		#define EXC_CATCH_EVENT(a)			fCatchExcept = true; \
											if ( (pszLocalBlock != NULL) && (uiLocalBlockCnt > 0) ) \
												g_Log.CatchEvent(a, "%s::%s() #%u \"%s\"", m_sClassName, pszLocalArgs, uiLocalBlockCnt, pszLocalBlock); \
											else \
												g_Log.CatchEvent(a, "%s::%s()", m_sClassName, pszLocalArgs)
	#endif

	#define EXC_CATCH			} \
								catch ( const CGrayError &e )	{ EXC_CATCH_EVENT(&e); CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); } \
								catch ( ... )					{ EXC_CATCH_EVENT(NULL); CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); }

	#define EXC_TRYSUB(a)		LPCTSTR pszLocalBlockSub = ""; \
								LPCTSTR pszLocalArgsSub = a; \
								unsigned int uiLocalBlockSubCnt = 0; \
								bool fCatchExceptSub = false; \
								UNREFERENCED_PARAMETER(fCatchExceptSub); \
								try {

	#define EXC_SETSUB(a)		pszLocalBlockSub = a; ++uiLocalBlockSubCnt

	#ifdef _THREAD_TRACK_CALLSTACK
		#define EXC_CATCHSUB_EVENT(a, b)	fCatchExceptSub = true; \
											StackDebugInformation::printStackTrace(); \
											if ( (pszLocalBlockSub != NULL) && (uiLocalBlockSubCnt > 0) ) \
												g_Log.CatchEvent(a, "SUB: %s::%s::%s() #%u \"%s\"", m_sClassName, b, pszLocalArgsSub, uiLocalBlockSubCnt, pszLocalBlockSub); \
											else \
												g_Log.CatchEvent(a, "SUB: %s::%s::%s()", m_sClassName, b, pszLocalArgsSub)
	#else
		#define EXC_CATCHSUB_EVENT(a, b)	fCatchExceptSub = true; \
											if ( (pszLocalBlockSub != NULL) && (uiLocalBlockSubCnt > 0) ) \
												g_Log.CatchEvent(a, "SUB: %s::%s::%s() #%u \"%s\"", m_sClassName, b, pszLocalArgsSub, uiLocalBlockSubCnt, pszLocalBlockSub); \
											else \
												g_Log.CatchEvent(a, "SUB: %s::%s::%s()", m_sClassName, b, pszLocalArgsSub)
	#endif

	#define EXC_CATCHSUB(a)		} \
								catch ( const CGrayError &e )	{ EXC_CATCHSUB_EVENT(&e, a); CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); } \
								catch ( ... )					{ EXC_CATCHSUB_EVENT(NULL, a); CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); }

	#define EXC_DEBUG_START		if ( fCatchExcept ) { try {

	#define EXC_DEBUG_END		/*StackDebugInformation::printStackTrace();*/ \
								} catch ( ... )	{ g_Log.EventError("Exception adding debug message on the exception\n"); CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); } }

	#define EXC_DEBUGSUB_START	if ( fCatchExceptSub ) { try {
	
	#define EXC_DEBUGSUB_END	/*StackDebugInformation::printStackTrace();*/ \
								} catch ( ... )	{ g_Log.EventError("Exception adding debug message on the exception\n"); CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); } }

	#define EXC_ADD_SCRIPT		g_Log.EventDebug("command '%s' args '%s'\n", s.GetKey(), s.GetArgRaw());
	#define EXC_ADD_SCRIPTSRC	g_Log.EventDebug("command '%s' args '%s' [%p]\n", s.GetKey(), s.GetArgRaw(), static_cast<void *>(pSrc));
	#define EXC_ADD_KEYRET(src)	g_Log.EventDebug("command '%s' ret '%s' [%p]\n", pszKey, static_cast<LPCTSTR>(sVal), static_cast<void *>(src));

#else

	#define EXC_TRY(a)	{
	#define EXC_SET(a)
	#define EXC_CATCH	}

	#define EXC_TRYSUB(a)	{
	#define EXC_SETSUB(a)
	#define EXC_CATCHSUB(a)	}
	
	#define EXC_DEBUG_START	{
	#define EXC_DEBUG_END	}

	#define EXC_DEBUGSUB_START	{
	#define EXC_DEBUGSUB_END	}

	#define EXC_ADD_SCRIPT
	#define EXC_ADD_SCRIPTSRC
	#define EXC_ADD_KEYRET(a)

#endif

#endif	// _CEXCEPTION_H
