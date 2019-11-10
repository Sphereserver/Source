#ifndef _INC_CLOG_H
#define _INC_CLOG_H

#include "../common/CFile.h"
#include "../common/CScript.h"
#include "../common/CScriptObj.h"

enum LOGL_TYPE
{
	// Severity level (first 4 bits = binary 1111 = dec 15 = hex 0xF)
	LOGL_FATAL	= 1,				// Fatal error (can't continue)
	LOGL_CRIT,						// Critical error (might not continue)
	LOGL_ERROR,						// Non-critical error (can continue)
	LOGL_WARN,						// Warning message
	LOGL_EVENT,						// Misc message
	LOGL_QTY	= 0xF,

	// Message type
	LOGM_ACCOUNTS		= 0x00080,	// Account operations
	LOGM_INIT			= 0x00100,	// Server startup messages
	LOGM_SAVE			= 0x00200,	// Server worldsave messages
	LOGM_CLIENTS_LOG	= 0x00400,	// Client login/logout messages
	LOGM_GM_PAGE		= 0x00800,	// GM pages
	LOGM_PLAYER_SPEAK	= 0x01000,	// Player speak
	LOGM_GM_CMDS		= 0x02000,	// GM commands
	LOGM_CHEAT			= 0x04000,	// Possible cheat attempts
	LOGM_KILLS			= 0x08000,	// Player combat results
	LOGM_HTTP			= 0x10000,	// HTTP operations
	LOGM_NOCONTEXT		= 0x20000,	// Do not include context information
	LOGM_DEBUG			= 0x40000	// Debug message ('DEBUG:' prefix)
};

extern class CEventLog
{
	// Any text event stream (destination is independent)
	// May include __LINE__ or __FILE__ macro as well?
public:
	CEventLog() { };

protected:
	virtual int VEvent(DWORD dwMask, LPCTSTR pszFormat, va_list args);
	virtual int EventStr(DWORD dwMask, LPCTSTR pszMsg)
	{
		UNREFERENCED_PARAMETER(dwMask);
		UNREFERENCED_PARAMETER(pszMsg);
		return 0;
	}

public:
	int _cdecl Event(DWORD dwMask, LPCTSTR pszFormat, ...) __printfargs(3, 4)
	{
		va_list vargs;
		va_start(vargs, pszFormat);
		int iRet = VEvent(dwMask, pszFormat, vargs);
		va_end(vargs);
		return iRet;
	}

	int _cdecl EventDebug(LPCTSTR pszFormat, ...) __printfargs(2, 3)
	{
		va_list vargs;
		va_start(vargs, pszFormat);
		int iRet = VEvent(LOGM_NOCONTEXT | LOGM_DEBUG, pszFormat, vargs);
		va_end(vargs);
		return iRet;
	}

	int _cdecl EventError(LPCTSTR pszFormat, ...) __printfargs(2, 3)
	{
		va_list vargs;
		va_start(vargs, pszFormat);
		int iRet = VEvent(LOGL_ERROR, pszFormat, vargs);
		va_end(vargs);
		return iRet;
	}

	int _cdecl EventWarn(LPCTSTR pszFormat, ...) __printfargs(2, 3)
	{
		va_list vargs;
		va_start(vargs, pszFormat);
		int iRet = VEvent(LOGL_WARN, pszFormat, vargs);
		va_end(vargs);
		return iRet;
	}

#ifdef _DEBUG
	int _cdecl EventEvent(LPCTSTR pszFormat, ...) __printfargs(2, 3)
	{
		va_list vargs;
		va_start(vargs, pszFormat);
		int iRet = VEvent(LOGL_EVENT, pszFormat, vargs);
		va_end(vargs);
		return iRet;
	}
#endif

private:
	CEventLog(const CEventLog &copy);
	CEventLog &operator=(const CEventLog &other);
} *g_pLog;

#define DEBUG_ERR(_x_)			g_pLog->EventError _x_
#ifdef _DEBUG
	#define DEBUG_WARN(_x_)		g_pLog->EventWarn _x_
	#define DEBUG_MSG(_x_)		g_pLog->EventEvent _x_
	#define DEBUG_MYFLAG(_x_)	g_pLog->Event _x_
#else
	#define DEBUG_WARN(_x_)
	#define DEBUG_MSG(_x_)
	#define DEBUG_MYFLAG(_x_)
#endif

extern struct CLog : public CFileText, public CEventLog
{
public:
	CLog()
	{
		m_fLockOpen = false;
		m_dwMsgMask = (LOGM_ACCOUNTS|LOGM_INIT|LOGM_SAVE|LOGM_CLIENTS_LOG|LOGM_GM_PAGE|LOGM_PLAYER_SPEAK|LOGM_GM_CMDS|LOGM_CHEAT|LOGM_KILLS|LOGM_HTTP);
		m_pScriptContext = NULL;
		m_pObjectContext = NULL;
		SetFilePath(SPHERE_FILE "log.log");		// default name
	}

public:
	bool m_fLockOpen;
	SimpleMutex m_mutex;

private:
	DWORD m_dwMsgMask;					// Level of log detail messages. IsLogMsg()
	CGTime m_dateStamp;					// last real time stamp.
	CGString m_sBaseDir;

	const CScript *m_pScriptContext;	// The current context.
	const CScriptObj *m_pObjectContext;	// The current context.

	static CGTime sm_prevCatchTick;		// don't flood with these.

public:
	bool OpenLog(LPCTSTR pszName = NULL);
	virtual int EventStr(DWORD dwMask, LPCTSTR pszMsg);
	void _cdecl CatchEvent(const CGrayError *pErr, LPCTSTR pszCatchContext, ...) __printfargs(3, 4);

	const CScript *SetScriptContext(const CScript *pScriptContext)
	{
		const CScript *pOldScript = m_pScriptContext;
		m_pScriptContext = pScriptContext;
		return pOldScript;
	}
	const CScriptObj *SetObjectContext(const CScriptObj *pObjectContext)
	{
		const CScriptObj *pOldObject = m_pObjectContext;
		m_pObjectContext = pObjectContext;
		return pOldObject;
	}

	bool SetFilePath(LPCTSTR pszName)
	{
		ASSERT(!IsFileOpen());
		return CFileText::SetFilePath(pszName);
	}

	LPCTSTR GetLogDir() const
	{
		return m_sBaseDir;
	}

	DWORD GetLogMask() const
	{
		return (m_dwMsgMask & ~LOGL_QTY);
	}
	void SetLogMask(DWORD dwMask)
	{
		m_dwMsgMask = GetLogLevel() | (dwMask & ~LOGL_QTY);
	}
	bool IsLoggedMask(DWORD dwMask) const
	{
		return (((dwMask & ~(LOGL_QTY|LOGM_NOCONTEXT|LOGM_DEBUG)) == 0) || ((GetLogMask() & (dwMask & ~LOGL_QTY)) != 0));
	}

	LOGL_TYPE GetLogLevel() const
	{
		return static_cast<LOGL_TYPE>(m_dwMsgMask & LOGL_QTY);
	}
	void SetLogLevel(LOGL_TYPE level)
	{
		m_dwMsgMask = GetLogMask() | (level & LOGL_QTY);
	}
	bool IsLoggedLevel(LOGL_TYPE level) const
	{
		return (((level & LOGL_QTY) != 0) && (GetLogLevel() >= (level & LOGL_QTY)));
	}

	bool IsLogged(DWORD dwMask) const
	{
		return (IsLoggedMask(dwMask) || IsLoggedLevel(static_cast<LOGL_TYPE>(dwMask)));
	}

private:
	enum Color
	{
		DEFAULT,
		RED,
		GREEN,
		YELLOW,
		BLUE,
		MAGENTA,
		CYAN,
		WHITE
	};
	void SetColor(Color color);

private:
	CLog(const CLog &copy);
	CLog &operator=(const CLog &other);
} g_Log;

#endif // _INC_CLOG_H
