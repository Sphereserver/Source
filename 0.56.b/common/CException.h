#ifndef _CEXCEPTION_H
#define _CEXCEPTION_H

// #include <stack>
#include "../sphere/threads.h"

// -------------------------------------------------------------------
// -------------------------------------------------------------------

extern "C"
{
	extern void globalstartsymbol();
	extern void globalendsymbol();
	extern const int globalstartdata;
	extern const int globalenddata;
}

void SetExceptionTranslator();
void SetUnixSignals( bool );

// -------------------------------------------------------------------
// -------------------------------------------------------------------

#define CException CGrayError
class CGrayError
{
	// we can throw this structure to produce an error.
	// similar to CFileException and CException
public:
	LOGL_TYPE m_eSeverity;	// const
	DWORD m_hError;	// HRESULT S_OK, "winerror.h" code. 0x20000000 = start of custom codes.
	LPCTSTR m_pszDescription;
public:
	CGrayError( LOGL_TYPE eSev, DWORD hErr, LPCTSTR pszDescription );
	CGrayError( const CGrayError& e );	// copy contstructor needed.
	virtual ~CGrayError();
public:
	CGrayError& operator=(const CGrayError& other);
public:
#ifdef _WIN32
	static int GetSystemErrorMessage( DWORD dwError, LPTSTR lpszError, UINT nMaxError );
#endif
	virtual bool GetErrorMessage( LPSTR lpszError, UINT nMaxError,	UINT * pnHelpContext = NULL ) const;
};

class CGrayAssert : public CGrayError
{
protected:
	LPCTSTR const m_pExp;
	LPCTSTR const m_pFile;
	const long m_lLine;
public:
	/*
	LPCTSTR const GetAssertFile();
	const unsigned GetAssertLine();
	*/
	static const char *m_sClassName;
	CGrayAssert(LOGL_TYPE eSeverity, LPCTSTR pExp, LPCTSTR pFile, long lLine);
	virtual ~CGrayAssert();
private:
	CGrayAssert& operator=(const CGrayAssert& other);

public:
	virtual bool GetErrorMessage(LPSTR lpszError, UINT nMaxError, UINT * pnHelpContext = NULL ) const;
};

#ifdef _WIN32
	// Catch and get details on the system exceptions.
	class CGrayException : public CGrayError
	{
	public:
		static const char *m_sClassName;
		const DWORD m_dwAddress;

		CGrayException(unsigned int uCode, DWORD dwAddress);
		virtual ~CGrayException();
	private:
		CGrayException& operator=(const CGrayException& other);

	public:
		virtual bool GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT * pnHelpContext = NULL ) const;
	};
#endif


// -------------------------------------------------------------------
// -------------------------------------------------------------------

//	Exceptions debugging routine.
#ifdef EXCEPTIONS_DEBUG

	#define EXC_TRY(a) \
		LPCTSTR inLocalBlock = ""; \
		LPCTSTR inLocalArgs = a; \
		unsigned int inLocalBlockCnt(0); \
		bool bCATCHExcept = false; \
		try \
		{

	#define EXC_SET(a) inLocalBlock = a; inLocalBlockCnt++

	#ifdef THREAD_TRACK_CALLSTACK
		#define EXC_CATCH_EXCEPTION(a) \
			bCATCHExcept = true; \
			StackDebugInformation::printStackTrace(); \
			if ( inLocalBlock != NULL && inLocalBlockCnt > 0 ) \
				g_Log.CatchEvent(a, "%s::%s() #%u \"%s\"", m_sClassName, inLocalArgs, \
															inLocalBlockCnt, inLocalBlock); \
			else \
				g_Log.CatchEvent(a, "%s::%s()", m_sClassName, inLocalArgs)
	#else
		#define EXC_CATCH_EXCEPTION(a) \
			bCATCHExcept = true; \
			if ( inLocalBlock != NULL && inLocalBlockCnt > 0 ) \
				g_Log.CatchEvent(a, "%s::%s() #%u \"%s\"", m_sClassName, inLocalArgs, \
															inLocalBlockCnt, inLocalBlock); \
			else \
				g_Log.CatchEvent(a, "%s::%s()", m_sClassName, inLocalArgs)
	#endif

	#define EXC_CATCH	}	\
		catch ( const CGrayError& e )	{ EXC_CATCH_EXCEPTION(&e); } \
		catch (...) { EXC_CATCH_EXCEPTION(NULL); }

	#define EXC_DEBUG_START if ( bCATCHExcept ) { try {

	#define EXC_DEBUG_END \
		/*StackDebugInformation::printStackTrace();*/ \
	} catch ( ... ) { g_Log.EventError("Exception adding debug message on the exception.\n"); }}

	#define EXC_TRYSUB(a) \
		LPCTSTR inLocalSubBlock = ""; \
		LPCTSTR inLocalSubArgs = a; \
		unsigned int inLocalSubBlockCnt(0); \
		bool bCATCHExceptSub = false; \
		try \
		{

	#define EXC_SETSUB(a) inLocalSubBlock = a; inLocalSubBlockCnt++

	#ifdef THREAD_TRACK_CALLSTACK
		#define EXC_CATCH_SUB(a,b) \
			bCATCHExceptSub = true; \
			StackDebugInformation::printStackTrace(); \
			if ( inLocalSubBlock != NULL && inLocalSubBlockCnt > 0 ) \
				g_Log.CatchEvent(a, "SUB: %s::%s::%s() #%u \"%s\"", m_sClassName, b, inLocalSubArgs, \
															inLocalSubBlockCnt, inLocalSubBlock); \
			else \
				g_Log.CatchEvent(a, "SUB: %s::%s::%s()", m_sClassName, b, inLocalSubArgs)
			//g_Log.CatchEvent(a, "%s::%s", b, inLocalSubBlock)
	#else
		#define EXC_CATCH_SUB(a,b) \
			bCATCHExceptSub = true; \
			if ( inLocalSubBlock != NULL && inLocalSubBlockCnt > 0 ) \
				g_Log.CatchEvent(a, "SUB: %s::%s::%s() #%u \"%s\"", m_sClassName, b, inLocalSubArgs, \
															inLocalSubBlockCnt, inLocalSubBlock); \
			else \
				g_Log.CatchEvent(a, "SUB: %s::%s::%s()", m_sClassName, b, inLocalSubArgs)
			//g_Log.CatchEvent(a, "%s::%s", b, inLocalSubBlock)
	#endif

	#define EXC_CATCHSUB(a)	}	\
		catch ( const CGrayError& e )	\
						{ \
						EXC_CATCH_SUB(&e, a); \
						} \
						catch (...) \
						{ \
						EXC_CATCH_SUB(NULL, a); \
						}

	#define EXC_DEBUGSUB_START if ( bCATCHExceptSub ) { try {
	
	#define EXC_DEBUGSUB_END \
		/*StackDebugInformation::printStackTrace();*/ \
	} catch ( ... ) { g_Log.EventError("Exception adding debug message on the exception.\n"); }}

	#define EXC_ADD_SCRIPT		g_Log.EventDebug("command '%s' args '%s'\n", s.GetKey(), s.GetArgRaw());
	#define EXC_ADD_SCRIPTSRC	g_Log.EventDebug("command '%s' args '%s' [%p]\n", s.GetKey(), s.GetArgRaw(), pSrc);
	#define EXC_ADD_KEYRET(src)	g_Log.EventDebug("command '%s' ret '%s' [%p]\n", pszKey, (LPCTSTR)sVal, src);

#else

	#define EXC_TRY(a) {
	#define EXC_SET(a)
	#define EXC_SETSUB(a)
	#define EXC_CATCH }
	#define EXC_TRYSUB(a) {
	#define EXC_CATCHSUB(a) }
	#define EXC_DEBUG_START if ( 0 ) {
	#define EXC_DEBUG_END }
	#define EXC_DEBUGSUB_START	if ( 0 ) {
	#define EXC_DEBUGSUB_END }
	#define EXC_ADD_SCRIPT
	#define EXC_ADD_SCRIPTSRC
	#define EXC_ADD_KEYRET(a)

#endif

#endif
