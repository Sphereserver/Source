#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <assert.h>
#include <deque>
#include <queue>
#include <vector>
using namespace std;

#ifdef _WIN32
	#include "os_windows.h"
#else
	#include "os_unix.h"
#endif

typedef THREAD_ENTRY_RET ( _cdecl * PTHREAD_ENTRY_PROC )(void *);

#include "threads.h"
#include "exceptions.h"

#define SPHERE_PORT		2593
#define SPHERE_FILE		"sphere"	// file name prefix
#define	SPHERE_SCRIPT	".scp"		// script extension

#define SCRIPT_MAX_LINE_LEN	0x1000

#define IMULDIV(a,b,c) (((a)*(b))/(c))

#ifndef COUNTOF
#define COUNTOF(a) 		(sizeof(a)/sizeof((a)[0]))	// dimensionof() ?
#endif

#ifndef MAKELONG
#define MAKELONG(low, high) ((long)(((WORD)(low)) | (((DWORD)((WORD)(high))) << 16)))
#define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define LOWORD(l)           ((WORD)(l))
#define HIWORD(l)           ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)			((BYTE)(((WORD)(w))&0xFF))
#define HIBYTE(w)			((BYTE)(((WORD)(w))>>8))
#endif	// MAKELONG
#ifndef MAKEDWORD
#define MAKEDWORD(low, high) ((DWORD)(((WORD)(low)) | (((DWORD)((WORD)(high))) << 16)))
#endif	// MAKEDWORD

#define GETNONWHITESPACE( pStr ) while ( isspace((pStr)[0]) ) { (pStr)++; }
#define _IS_SWITCH(c)    ((c) == '-' || (c) == '/' )	// command line switch.

extern char g_tmpBuf[10][1024];
extern long g_tmpBufIdx;
#define	TEMPSTRING(_x_) char *_x_; { \
							CThread *threadTS = CThread::Thread(); \
							if ( threadTS ) _x_ = threadTS->TempString(); \
							else { \
								if ( ++g_tmpBufIdx >= 10 ) g_tmpBufIdx = 0; \
								_x_ = g_tmpBuf[g_tmpBufIdx]; \
								_x_[0] = 0; \
						}}

/***************************************************************************
 *
 *
 *	class CLog					logging possibitilites support
 *
 *
 ***************************************************************************/

enum LOGL_TYPE
{
// lower 4 bits - critical level		0x00007
	LOGL_FATAL	= 1, 	// fatal error ! cannot continue
	LOGL_CRIT	= 2, 	// critical. might not continue
	LOGL_ERROR	= 3, 	// non-fatal errors. can continue
	LOGL_WARN	= 4,	// just a warning
	LOGL_EVENT	= 5,	// misc event
// middle 12 bits - specialization		0x0fff0
	LOGM_ACCOUNTS	=	0x00010,	// account login/operations
	LOGM_SAVE		=	0x00020,	// world save
	LOGM_CLIENTS_LOG =	0x00040,	// all clients as they log in and out.
	LOGM_GM_PAGE	=	0x00080,	// player gm pages.
	LOGM_PLAYER_SPEAK =	0x00100,	// All that the players say.
	LOGM_GM_CMDS	=	0x00200,	// Log all GM commands.
	LOGM_KILLS		=	0x00400,	// Log player combat results.
	LOGM_HTTP		=	0x00800,	// HTTP protocol operations
// upper 4 bits - modificator flags		0xf0000
	LOGM_INIT   	=	0x10000,	// start up messages.
	LOGM_NOCONTEXT	=	0x20000,	// do not include context information
	LOGM_DEBUG		=	0x40000,	// debug kind of message with DEBUG: prefix
};

class CChar;
class CError;

class CLog
{
public:
	static const char *m_sClassName;
	CLog();
	~CLog();

	DWORD	LogMask();
	void	LogMask(DWORD mask);
	char	*LogDir();
	void	LogDir(char *dir);

	void	Close();

public:
	void _cdecl Event(DWORD mask, LPCTSTR format, ...);
	void _cdecl Init(LPCTSTR format, ...);
	void _cdecl Debug(LPCTSTR format, ...);
	void _cdecl Error(LPCTSTR format, ...);
	void _cdecl Warn(LPCTSTR format, ...);
	void _cdecl Catch(CError *pErr, LPCTSTR CatchContext, ...);
	void WriteRaw(LPCTSTR line);
	void _cdecl WriteRawF(LPCTSTR format, ...);
	void gmLog(char *text, CChar *source = NULL);

#ifdef _DEBUG
	void _cdecl Event(LPCTSTR format, ...);
	#define DEBUG_MSG(_x_)	g_Log.Event _x_
	#define DEBUG_WARN(_x_)	g_Log.Warn _x_
	#define DEBUG_ERR(_x_)	g_Log.Error _x_
#else
	#define DEBUG_MSG(_x_)
	#define DEBUG_WARN(_x_)
	#define DEBUG_ERR(_x_)
#endif

protected:
	DWORD	m_logmask;
	time_t	m_days;
	char	m_dir[_MAX_PATH];
	FILE	*m_file;
	char	m_lastMessage[1024];
	long	m_lastMsgCount;
	char	m_buf[1024];
	FILE	*m_gmlog;

protected:
	void Open();
	void EventStr(DWORD mask, LPCTSTR msg);
	void VEvent(DWORD mask, LPCTSTR msg, va_list args);
};

extern CLog g_Log;

//
//	Triggers list
//
enum E_TRIGGERS
{
	#define ADD(a,b) TRIGGER_##a,
	#include "../tables/triggers.tbl"
	#undef ADD
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

//	Time measurement macroses
extern LONGLONG llTimeProfileFrequency;

#ifdef _WIN32

#define	TIME_PROFILE_INIT	\
	LONGLONG llTicks, llTicksEnd
#define	TIME_PROFILE_START	\
	if ( !QueryPerformanceCounter((LARGE_INTEGER *)&llTicks)) llTicks = GetTickCount()
#define TIME_PROFILE_END	if ( !QueryPerformanceCounter((LARGE_INTEGER *)&llTicksEnd)) llTicksEnd = GetTickCount()

#else

#define	TIME_PROFILE_INIT	\
	LONGLONG llTicks, llTicksEnd
#define	TIME_PROFILE_START	\
	llTicks = GetTickCount()
#define TIME_PROFILE_END	llTicksEnd = GetTickCount();

#endif

#define TIME_PROFILE_GET_HI	((llTicksEnd - llTicks)/(llTimeProfileFrequency/1000))
#define	TIME_PROFILE_GET_LO	((((llTicksEnd - llTicks)*10000)/(llTimeProfileFrequency/1000))%10000)

#define TIME_DAY	0x01
#define	TIME_TIME	0x02
#define	TIME_SHORT	0x03
#define TIME_LONG	0x04
#define	TIME_GMT	0x08
#define TIME_EXACT	0x10
char *formattime(char *storage, struct tm *time, int mask = TIME_SHORT);

extern int ATOI(const char *str);
extern char *ITOA(int i, char *str, int radix = 10);
extern char *LTOA(long l, char *str, long radix = 10);
extern char *STRREV(char *str);

//
//	???
//

struct CValStr
{
public:
	LPCTSTR	m_pszName;
	int		m_iVal;

public:
	void SetValues(int val, LPCTSTR name)
	{
		m_iVal = val;
		m_pszName = name;
	}
	LPCTSTR FindName(int iVal) const;
	void SetValue(int iVal)
	{
		m_iVal = iVal;
	}
};

#endif
