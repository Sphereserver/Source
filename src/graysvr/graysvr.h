#ifndef _INC_GRAYSVR_H_
#define _INC_GRAYSVR_H_
#pragma once

//	Enable advanced exceptions catching. Consumes some more resources, but is very useful
//	for debug on a running environment. Also it makes sphere more stable since exceptions
//	are local
#ifndef _DEBUG
	#ifndef EXCEPTIONS_DEBUG
	#define EXCEPTIONS_DEBUG
	#endif
#endif

#if defined(_WIN32) && !defined(_MTNETWORK)
	// _MTNETWORK enabled via makefile for other systems
	#define _MTNETWORK
#endif

//#define DEBUGWALKSTUFF 1
//#ifdef _DEBUG
#ifdef DEBUGWALKSTUFF
	#define WARNWALK(_x_)		g_pLog->EventWarn _x_;
#else
	#define WARNWALK(_x_)		if ( g_Cfg.m_wDebugFlags & DEBUGF_WALK ) { g_pLog->EventWarn _x_; }
#endif

#include "../common/graycom.h"


class CServTime
{
#undef GetCurrentTime
#define TICK_PER_SEC 10
#define TENTHS_PER_SEC 1
	// A time stamp in the server/game world.
public:
	static const char *m_sClassName;
	UINT64 m_lPrivateTime;
public:
	UINT64 GetTimeRaw() const
	{
		if ( m_lPrivateTime < 0 )
			return 0;

		return m_lPrivateTime;
	}
	INT64 GetTimeDiff( const CServTime & time ) const
	{
		return( m_lPrivateTime - time.m_lPrivateTime );
	}
	void Init()
	{
		m_lPrivateTime = 0;
	}
	void InitTime( UINT64 lTimeBase )
	{
		if ( lTimeBase < 0 )
			lTimeBase = 0;

		m_lPrivateTime = lTimeBase;
	}
	bool IsTimeValid() const
	{
		return( m_lPrivateTime > 0 ? true : false );
	}
	CServTime operator+( INT64 iTimeDiff ) const
	{
		CServTime time;
		time.m_lPrivateTime = m_lPrivateTime + iTimeDiff;
		if ( time.m_lPrivateTime < 0 )
			time.m_lPrivateTime = 0;

		return( time );
	}
	CServTime operator-( INT64 iTimeDiff ) const
	{
		CServTime time;
		time.m_lPrivateTime = m_lPrivateTime - iTimeDiff;
		if ( time.m_lPrivateTime < 0 )
			time.m_lPrivateTime = 0;

		return( time );
	}
	INT64 operator-( CServTime time ) const
	{
		return(m_lPrivateTime-time.m_lPrivateTime);
	}
	bool operator==(CServTime time) const
	{
		return(m_lPrivateTime==time.m_lPrivateTime);
	}
	bool operator!=(CServTime time) const
	{
		return(m_lPrivateTime!=time.m_lPrivateTime);
	}
	bool operator<(CServTime time) const
	{
		return(m_lPrivateTime<time.m_lPrivateTime);
	}
	bool operator>(CServTime time) const
	{
		return(m_lPrivateTime>time.m_lPrivateTime);
	}
	bool operator<=(CServTime time) const
	{
		return(m_lPrivateTime<=time.m_lPrivateTime);
	}
	bool operator>=(CServTime time) const
	{
		return(m_lPrivateTime>=time.m_lPrivateTime);
	}
	void SetCurrentTime()
	{
		m_lPrivateTime = GetCurrentTime().m_lPrivateTime;
	}
	static CServTime GetCurrentTime();
};

enum RESDISPLAY_VERSION
{
	RDS_NONE,
	RDS_T2A,
	RDS_LBR,
	RDS_AOS,
	RDS_SE,
	RDS_ML,
	RDS_KR,
	RDS_SA,
	RDS_HS,
	RDS_TOL,
	RDS_QTY
};

#include "../common/graymul.h"
#include "../common/grayproto.h"
#include "../common/CGrayInst.h"
#include "../common/CResourceBase.h"
#include "../common/CRegion.h"
#include "../common/CGrayMap.h"
#include "../sphere/mutex.h"
#include "../sphere/ProfileData.h"
#include "../sphere/threads.h"
#if !defined(_WIN32) || defined(_LIBEV)
	#include "../sphere/linuxev.h"
#endif
#include "../common/CQueue.h"
#include "../common/CSectorTemplate.h"
#include "../common/CDataBase.h"
#include "../common/sqlite/SQLite.h"

#include "CResource.h"
#include "CAccount.h"
#include "CBase.h"
#include "CCharBase.h"
#include "CItemBase.h"
#include "CChat.h"
#include "CClient.h"
#include "CGMPage.h"
#include "CObjBase.h"
#include "CItem.h"
#include "CChar.h"
#include "CServRef.h"
#include "CServer.h"
#include "CWorld.h"

///////////////////////////////////////////////

//	Triggers list
enum E_TRIGGERS
{
	#define ADD(a) TRIGGER_##a,
	#include "../tables/triggers.tbl"
	TRIGGER_QTY,
};

extern bool IsTrigUsed(E_TRIGGERS id);
extern bool IsTrigUsed(const char *name);
extern void TriglistInit();
extern void TriglistClear();
extern void TriglistAdd(E_TRIGGERS id);
extern void TriglistAdd(const char *name);
extern void Triglist(long &total, long &used);
extern void TriglistPrint();


// Text mashers.

extern DIR_TYPE GetDirStr( LPCTSTR pszDir );
extern LPCTSTR GetTimeMinDesc( int dwMinutes );
extern size_t FindStrWord( LPCTSTR pTextSearch, LPCTSTR pszKeyWord );

extern struct CLog : public CFileText, public CEventLog
{
	// subject matter. (severity level is first 4 bits, LOGL_EVENT)
#define LOGM_ACCOUNTS		0x00080
//#define LOGM_INIT			0x00100	// start up messages.
#define LOGM_SAVE			0x00200	// world save status.
#define LOGM_CLIENTS_LOG	0x00400	// all clients as they log in and out.
#define LOGM_GM_PAGE		0x00800	// player gm pages.
#define LOGM_PLAYER_SPEAK	0x01000	// All that the players say.
#define LOGM_GM_CMDS		0x02000	// Log all GM commands.
#define LOGM_CHEAT			0x04000	// Probably an exploit !
#define LOGM_KILLS			0x08000	// Log player combat results.
#define LOGM_HTTP			0x10000
//#define	LOGM_NOCONTEXT		0x20000	// do not include context information
//#define LOGM_DEBUG			0x40000	// debug kind of message with DEBUG: prefix

private:
	DWORD m_dwMsgMask;			// Level of log detail messages. IsLogMsg()
	CGTime m_dateStamp;			// last real time stamp.
	CGString m_sBaseDir;

	const CScript * m_pScriptContext;	// The current context.
	const CScriptObj * m_pObjectContext;	// The current context.

	static CGTime sm_prevCatchTick;	// don't flood with these.
public:
	bool m_fLockOpen;
	SimpleMutex m_mutex;

public:
	const CScript * SetScriptContext( const CScript * pScriptContext )
	{
		const CScript * pOldScript = m_pScriptContext;
		m_pScriptContext = pScriptContext;
		return( pOldScript );
	}
	const CScriptObj * SetObjectContext( const CScriptObj * pObjectContext )
	{
		const CScriptObj * pOldObject = m_pObjectContext;
		m_pObjectContext = pObjectContext;
		return( pOldObject );
	}
	bool SetFilePath( LPCTSTR pszName )
	{
		ASSERT( ! IsFileOpen());
		return CFileText::SetFilePath( pszName );
	}

	LPCTSTR GetLogDir() const
	{
		return( m_sBaseDir );
	}
	bool OpenLog( LPCTSTR pszName = NULL );	// name set previously.
	DWORD GetLogMask() const
	{
		return (m_dwMsgMask & ~0xF);
	}
	void SetLogMask(DWORD dwMask)
	{
		m_dwMsgMask = GetLogLevel() | (dwMask & ~0xF);
	}
	bool IsLoggedMask(DWORD dwMask) const
	{
		return (((dwMask & ~(0xF|LOGM_NOCONTEXT|LOGM_DEBUG)) == 0) || ((GetLogMask() & (dwMask & ~0xF)) != 0));
	}
	LOGL_TYPE GetLogLevel() const
	{
		return static_cast<LOGL_TYPE>(m_dwMsgMask & 0xF);
	}
	void SetLogLevel(LOGL_TYPE level)
	{
		m_dwMsgMask = GetLogMask() | (level & 0xF);
	}
	bool IsLoggedLevel(LOGL_TYPE level) const
	{
		return (((level & 0xF) != 0) && (GetLogLevel() >= (level & 0xF)));
	}
	bool IsLogged(DWORD dwMask) const
	{
		return IsLoggedMask(dwMask) || IsLoggedLevel(static_cast<LOGL_TYPE>(dwMask));
	}

	virtual int EventStr(DWORD dwMask, LPCTSTR pszMsg);
	void _cdecl CatchEvent(const CGrayError * pErr, LPCTSTR pszCatchContext, ...) __printfargs(3,4);

public:
	CLog()
	{
		m_fLockOpen = false;
		m_pScriptContext = NULL;
		m_pObjectContext = NULL;
		m_dwMsgMask = LOGL_ERROR|LOGM_INIT|LOGM_CLIENTS_LOG|LOGM_GM_PAGE;
		SetFilePath(SPHERE_FILE "log.log");	// default name to go to.
	}

private:
	CLog(const CLog& copy);
	CLog& operator=(const CLog& other);

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

	/**
	 * Changes current console color to the specified one. Note, that the color should be reset after being set
	 */
	void SetColor(Color color);
} g_Log;		// Log file

//////////////////

class CDialogResponseArgs : public CScriptTriggerArgs
{
	// The scriptable return from a gump dialog.
	// "ARG" = dialog args script block. ex. ARGTXT(id), ARGCHK(i)
public:
	static const char *m_sClassName;
	struct TResponseString
	{
	public:
		const WORD m_ID;
		CGString const m_sText;

		TResponseString(WORD id, LPCTSTR pszText) : m_ID(id), m_sText(pszText)
		{
		}

	private:
		TResponseString(const TResponseString& copy);
		TResponseString& operator=(const TResponseString& other);
	};

	CGTypedArray<DWORD,DWORD>		m_CheckArray;
	CGObArray<TResponseString *>	m_TextArray;
public:
	void AddText( WORD id, LPCTSTR pszText )
	{
		m_TextArray.Add(new TResponseString(id, pszText));
	}
	LPCTSTR GetName() const
	{
		return "ARGD";
	}
	bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );

public:
	CDialogResponseArgs() { };

private:
	CDialogResponseArgs(const CDialogResponseArgs& copy);
	CDialogResponseArgs& operator=(const CDialogResponseArgs& other);
};

////////////////////////////////////////////////////////////////////////////////////

class Main : public AbstractSphereThread
{
public:
	Main();
	virtual ~Main() { };

private:
	Main(const Main& copy);
	Main& operator=(const Main& other);

public:
	// we increase the access level from protected to public in order to allow manual execution when
	// configuration disables using threads
	// TODO: in the future, such simulated functionality should lie in AbstractThread inself instead of hacks
	virtual void tick();

protected:
	virtual void onStart();
	virtual bool shouldExit();
};

//////////////////////////////////////////////////////////////

extern LPCTSTR g_szServerDescription;
extern LPCTSTR g_szServerBuildDate;
extern LPCTSTR g_szServerBuildTime;
extern LPCTSTR const g_Stat_Name[STAT_QTY];
extern CGStringList g_AutoComplete;

extern int Sphere_InitServer( int argc, char *argv[] );
extern int Sphere_OnTick();
extern void Sphere_ExitServer();
extern int Sphere_MainEntryPoint( int argc, char *argv[] );

///////////////////////////////////////////////////////////////
// -CGrayUID

inline INT64 CObjBase::GetTimerDiff() const
{
	// How long till this will expire ?
	return( g_World.GetTimeDiff( m_timeout ));
}
inline CObjBase * CGrayUIDBase::ObjFind() const
{
	if ( IsResource())
		return( NULL );
	return( g_World.FindUID( m_dwInternalVal & UID_O_INDEX_MASK ));
}
inline CItem * CGrayUIDBase::ItemFind() const
{
	// IsItem() may be faster ?
	return( dynamic_cast <CItem *>( ObjFind()));
}
inline CChar * CGrayUIDBase::CharFind() const
{
	return( dynamic_cast <CChar *>( ObjFind()));
}

// ---------------------------------------------------------------------------------------------

struct TScriptProfiler
{
	BYTE		initstate;
	DWORD		called;
	ULONGLONG	total;
	struct TScriptProfilerFunction
	{
		TCHAR		name[128];	// name of the function
		DWORD		called;		// how many times called
		ULONGLONG	total;		// total executions time
		ULONGLONG	min;		// minimal executions time
		ULONGLONG	max;		// maximal executions time
		ULONGLONG	average;	// average executions time
		TScriptProfilerFunction *next;
	}		*FunctionsHead, *FunctionsTail;
	struct TScriptProfilerTrigger
	{
		TCHAR		name[128];	// name of the trigger
		DWORD		called;		// how many times called
		ULONGLONG	total;		// total executions time
		ULONGLONG	min;		// minimal executions time
		ULONGLONG	max;		// maximal executions time
		ULONGLONG	average;	// average executions time
		TScriptProfilerTrigger *next;
	}		*TriggersHead, *TriggersTail;
};
extern TScriptProfiler g_profiler;

#endif
