#ifndef _CEXCEPTION_H
#define _CEXCEPTION_H

// #include <stack>

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
#ifdef _WIN32
	static int GetSystemErrorMessage( DWORD dwError, LPTSTR lpszError, UINT nMaxError );
#endif
	bool GetErrorMessage( LPSTR lpszError, UINT nMaxError,	UINT * pnHelpContext = NULL );
};

class CGrayAssert : public CGrayError
{
protected:
	LPCTSTR const m_pExp;
	LPCTSTR const m_pFile;
	const unsigned m_uLine;
public:
	/*
	LPCTSTR const GetAssertFile();
	const unsigned GetAssertLine();
	*/
	static const char *m_sClassName;
	CGrayAssert(LOGL_TYPE eSeverity, LPCTSTR pExp, LPCTSTR pFile, unsigned uLine);
	virtual ~CGrayAssert();

	virtual bool GetErrorMessage(LPSTR lpszError, UINT nMaxError, UINT * pnHelpContext);
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

		virtual BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError,	UINT * pnHelpContext);
	};
#endif


// -------------------------------------------------------------------
// -------------------------------------------------------------------

//	Exceptions debugging routine.
#ifdef EXCEPTIONS_DEBUG

	#define EXC_TRY(a) \
		const char *inLocalBlock = ""; \
		char *inLocalArgs = a; \
		int inLocalBlockCnt(0); \
		bool bCATCHExcept = false; \
		try \
		{

	#define EXC_SET(a) inLocalBlock = a; inLocalBlockCnt++

#ifdef _WIN32
	#define EXC_CATCH_EXCEPTION(a) \
		bCATCHExcept = true; \
		StackDebugInformation::printStackTrace(); \
		if ( inLocalBlock ) \
			g_Log.CatchEvent(a, "%s::%s() #%d \"%s\"", m_sClassName, inLocalArgs, \
														inLocalBlockCnt, inLocalBlock); \
		else \
			g_Log.CatchEvent(a, "%s::%s() - %s", m_sClassName, inLocalArgs)
#else
		#define EXC_CATCH_EXCEPTION(a) \
		bCATCHExcept = true; \
		if ( inLocalBlock ) \
			g_Log.CatchEvent(a, "%s::%s() #%d \"%s\"", m_sClassName, inLocalArgs, \
														inLocalBlockCnt, inLocalBlock); \
		else \
			g_Log.CatchEvent(a, "%s::%s() - %s", m_sClassName, inLocalArgs)
#endif

	#define EXC_CATCH	}	\
		catch ( CGrayError &e )	{ EXC_CATCH_EXCEPTION(&e); } \
		catch (...) { EXC_CATCH_EXCEPTION(NULL); }

	#define EXC_DEBUG_START if ( bCATCHExcept ) { try {

	#define EXC_DEBUG_END \
		/*StackDebugInformation::printStackTrace();*/ \
	} catch ( ... ) { g_Log.EventError("Exception adding debug message on the exception.\n"); }}

	#define EXC_TRYSUB(a) \
		const char *inLocalSubBlock = ""; \
		char *inLocalSubArgs = a; \
		int inLocalSubBlockCnt(0); \
		bool bCATCHExceptSub = false; \
		try \
		{

	#define EXC_SETSUB(a) inLocalSubBlock = a; inLocalSubBlockCnt++

#ifdef _WIN32
	#define EXC_CATCH_SUB(a,b) \
		bCATCHExceptSub = true; \
		StackDebugInformation::printStackTrace(); \
		if ( inLocalSubBlock ) \
			g_Log.CatchEvent(a, "SUB: %s::%s::%s() #%d \"%s\"", m_sClassName, b, inLocalSubArgs, \
														inLocalSubBlockCnt, inLocalSubBlock); \
		else \
			g_Log.CatchEvent(a, "SUB: %s::%s::%s() - %s", m_sClassName, b, inLocalSubArgs)
		//g_Log.CatchEvent(a, "%s::%s", b, inLocalSubBlock)
#else
		#define EXC_CATCH_SUB(a,b) \
		bCATCHExceptSub = true; \
		if ( inLocalSubBlock ) \
			g_Log.CatchEvent(a, "SUB: %s::%s::%s() #%d \"%s\"", m_sClassName, b, inLocalSubArgs, \
														inLocalSubBlockCnt, inLocalSubBlock); \
		else \
			g_Log.CatchEvent(a, "SUB: %s::%s::%s() - %s", m_sClassName, b, inLocalSubArgs)
		//g_Log.CatchEvent(a, "%s::%s", b, inLocalSubBlock)
#endif

	#define EXC_CATCHSUB(a)	}	\
		catch ( CGrayError &e )	\
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
	#define EXC_ADD_SCRIPTSRC	g_Log.EventDebug("command '%s' args '%s' [0%lx]\n", s.GetKey(), s.GetArgRaw(), pSrc);
	#define EXC_ADD_KEYRET(src)	g_Log.EventDebug("command '%s' ret '%s' [0%lx]\n", pszKey, sVal, src);

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

#ifdef _WIN32

struct STACK_INFO_REC {
	const char *functionName;
	LONGLONG	startTime;
};
extern STACK_INFO_REC g_stackInfo[0x1000];
extern long g_stackPos;

class StackDebugInformation {
public:
	StackDebugInformation(const char *name);
	~StackDebugInformation();
	
	static void printStackTrace();
};

#define ADDTOCALLSTACK(_function_)	StackDebugInformation debugStack(_function_);

#else
#define ADDTOCALLSTACK(_function_)
#endif

#endif
