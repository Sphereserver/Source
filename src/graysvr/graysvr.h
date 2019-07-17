#ifndef _INC_GRAYSVR_H
#define _INC_GRAYSVR_H
#pragma once

//	Enable advanced exceptions catching. Consumes some more resources, but is very useful
//	for debug on a running environment. Also it makes sphere more stable since exceptions
//	are local
#ifndef _DEBUG
	#ifndef EXCEPTIONS_DEBUG
		#define EXCEPTIONS_DEBUG
	#endif
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
		m_lPrivateTime = lTimeBase;
	}
	bool IsTimeValid() const
	{
		return (m_lPrivateTime > 0);
	}
	CServTime operator+( INT64 iTimeDiff ) const
	{
		CServTime time;
		time.m_lPrivateTime = m_lPrivateTime + iTimeDiff;
		return time;
	}
	CServTime operator-( INT64 iTimeDiff ) const
	{
		CServTime time;
		time.m_lPrivateTime = m_lPrivateTime - iTimeDiff;
		return time;
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
#include "../common/grayver.h"
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
#include "CItemMulti.h"
#include "CItemMultiCustom.h"
#include "CItemShip.h"
#include "CItemStone.h"
#include "CChar.h"
#include "CServRef.h"
#include "CServer.h"
#include "CSector.h"
#include "CWorld.h"

///////////////////////////////////////////////

//	Triggers list
enum E_TRIGGERS
{
	#define ADD(a) TRIGGER_##a,
	#include "../tables/triggers.tbl"
	TRIGGER_QTY
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

#endif	// _INC_GRAYSVR_H
