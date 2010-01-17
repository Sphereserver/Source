//
// GRAYSRV.CPP.
// Copyright Menace Software (www.menasoft.com).
//
#include "graysvr.h"	// predef header.
#include "../common/grayver.h"	// sphere version
#include "PingServer.h"	// ping server
#include "../network/network.h" // network thread
#include "../sphere/asyncdb.h"
#ifndef _WIN32
	#include "../sphere/linuxev.h"
#endif

#if !defined(pid_t)
#define pid_t int
#endif

#ifdef _WIN32
	#include "ntservice.h"	// g_Service
	#include <process.h>	// getpid()
#else
	#include <sys/time.h>
	#include <stdio.h>
	#include <unistd.h>
	#include <termios.h>
#endif

CMapList::CMapList()
{
	memset(m_mapsinitalized, false, sizeof(m_mapsinitalized));
	memset(m_sizex, 0, sizeof(m_sizex));
	memset(m_sizey, 0, sizeof(m_sizey));
	memset(m_maps, true, sizeof(m_maps));
	memset(m_mapnum, -1, sizeof(m_mapnum));
	memset(m_mapid, -1, sizeof(m_mapid));
	memset(m_sectorsize, 0, sizeof(m_sectorsize));

	Load(0, 0x1800, 0x1000, 64, 0, 0);	// #0 map0.mul (felucca)
	Load(1, 0x1800, 0x1000, 64, 0, 1);	// #1 map0.mul (trammel)
	Load(2, 0x900, 0x640, 64, 2, 2);	// #2 map2.mul (ilshenar)
	Load(3, 0xa00, 0x800, 64, 3, 3);	// #3 map3.mul (malas)
	Load(4, 0x5a8, 0x5a8, 8, 4, 4);		// #4 map4.mul (tokuno islands)
	Load(5, 0x500, 0x1000, 64, 5, 5);	// #5 map5.mul (tel mur)

	m_pMapDiffCollection = NULL;
}

bool CMapList::Load(int map, int maxx, int maxy, int sectorsize, int realmapnum, int mapid)
{
	m_sizex[map] = maxx;
	m_sizey[map] = maxy;
	m_sectorsize[map] = sectorsize;
	m_mapnum[map] = realmapnum;
	m_mapid[map] = mapid;
	return true;
}

bool CMapList::Load(int map, char *args)
{
	if (( map < 0 ) || ( map > 255 ))
	{
		g_Log.EventError("Invalid map #%d couldn't be initialized.\n", map);
		return false;
	}
	else if ( !m_mapsinitalized[map] )	// disable double intialization
	{
		TCHAR	*ppCmd[5];	// maxx,maxy,sectorsize,mapnum[like 0 for map0/statics0/staidx0],mapid
		int		iCount = Str_ParseCmds(args, ppCmd, COUNTOF(ppCmd), ",");

		if ( !iCount )	// simple MAPX= same as disabling the map
		{
			m_maps[map] = false;
		}
		else
		{
			int	maxx = 0, maxy = 0, sectorsize = 0, realmapnum = 0, mapid = -1;
			if ( ppCmd[0] ) maxx = ATOI(ppCmd[0]);
			if ( ppCmd[1] ) maxy = ATOI(ppCmd[1]);
			if ( ppCmd[2] ) sectorsize = ATOI(ppCmd[2]);
			if ( ppCmd[3] ) realmapnum = ATOI(ppCmd[3]);
			if ( ppCmd[4] ) mapid = ATOI(ppCmd[4]);

										// zero settings of anything except the real map num means
			if ( maxx )					// skipping the argument
			{
				if (( maxx < 8 ) || ( maxx % 8 ))
				{
					g_Log.EventError("MAP%d: X coord must be multiple of 8 (%d is invalid, %d is still effective).\n", map, maxx, m_sizex[map]);
				}
				else m_sizex[map] = maxx;
			}
			if ( maxy )
			{
				if (( maxy < 8 ) || ( maxy % 8 ))
				{
					g_Log.EventError("MAP%d: Y coord must be multiple of 8 (%d is invalid, %d is still effective).\n", map, maxy, m_sizey[map]);
				}
				else m_sizey[map] = maxy;
			}
			if ( sectorsize > 0 )
			{
				if (( sectorsize < 8 ) || ( sectorsize % 8 ))
				{
					g_Log.EventError("MAP%d: Sector size must be multiple of 8 (%d is invalid, %d is still effective).\n", map, sectorsize, m_sectorsize[map]);
				}
				else if (( m_sizex[map]%sectorsize ) || ( m_sizey[map]%sectorsize ))
				{
					g_Log.EventError("MAP%d: Map dimensions [%d,%d] must be multiple of sector size (%d is invalid, %d is still effective).\n", map, m_sizex[map], m_sizey[map], sectorsize, m_sectorsize[map]);
				}
				else m_sectorsize[map] = sectorsize;
			}
			if ( realmapnum >= 0 )
				m_mapnum[map] = realmapnum;
			if ( mapid >= 0 )
				m_mapid[map] = mapid;
			else
				m_mapid[map] = map;
		}
		m_mapsinitalized[map] = true;
	}
	return true;
}

void CMapList::Init()
{
	for ( int i = 0; i < 256; i++ )
	{
		if ( m_maps[i] )	// map marked as available. check whatever it's possible
		{
			//	check coordinates first
			if ( !m_sizex[i] || !m_sizey[i] || !m_sectorsize[i] || ( m_mapnum[i] == -1 ) )
				m_maps[i] = false;
		}
	}

	if ( g_Cfg.m_fUseMapDiffs && !m_pMapDiffCollection )
		m_pMapDiffCollection = new CMapDiffCollection();
}

bool WritePidFile(int iMode = 0)
{
	LPCTSTR	file = GRAY_FILE ".pid";
	FILE	*pidFile;

	if ( iMode == 1 )		// delete
	{
		return ( STDFUNC_UNLINK(file) == 0 );
	}
	else if ( iMode == 2 )	// check for .pid file
	{
		pidFile = fopen(file, "r");
		if ( pidFile )
		{
			g_Log.Event(LOGM_INIT, GRAY_FILE ".pid already exists. Secondary launch or unclean shutdown?\n");
			fclose(pidFile);
		}
		return true;
	}
	else
	{
		pidFile = fopen(file, "w");
		if ( pidFile )
		{
			pid_t spherepid = STDFUNC_GETPID();

			fprintf(pidFile,"%d\n", spherepid);
			fclose(pidFile);
			return true;
		}
		g_Log.Event(LOGM_INIT, "Cannot create pid file!\n");
		return false;
	}
}

int CEventLog::VEvent( DWORD wMask, LPCTSTR pszFormat, va_list args )
{
	if ( !pszFormat || !*pszFormat )
		return false;

	TemporaryString pszTemp;
	size_t len = _vsnprintf(pszTemp, (SCRIPT_MAX_LINE_LEN - 1), pszFormat, args);
	if ( ! len ) strncpy(pszTemp, pszFormat, (SCRIPT_MAX_LINE_LEN - 1));

	// This get rids of exploits done sending 0x0C to the log subsytem.
	// TCHAR *	 pFix;
	// if ( ( pFix = strchr( pszText, 0x0C ) ) )
	//	*pFix = ' ';

	return EventStr(wMask, pszTemp);
}

LPCTSTR const g_Stat_Name[STAT_QTY] =	// not sorted obviously.
{
	"STR",
	"INT",
	"DEX",
	"FOOD",
	"KARMA",
	"FAME",
};

LPCTSTR g_szServerDescription =
	GRAY_TITLE " Version " GRAY_VERSION " "
#ifdef _WIN32
	"[WIN32]"
#else
#ifdef _BSD
	"[FreeBSD]"
#else
	"[Linux]"
#endif
#endif
#ifdef _DEBUG
	"[DEBUG]"
#endif
	" by www.sphereserver.com";

int CObjBase::sm_iCount = 0;	// UID table.
LONGLONG llTimeProfileFrequency = 1000;	// time profiler

// game servers stuff.
CWorld		g_World;	// the world. (we save this stuff)
CServer		g_Serv;	// current state stuff not saved.
CResource	g_Cfg;
CGrayInstall g_Install;
CVerDataMul	g_VerData;
CExpression g_Exp;	// Global script variables.
CLog		g_Log;
CEventLog * g_pLog = &g_Log;
CAccounts	g_Accounts;	// All the player accounts. name sorted CAccount
CGStringList	g_AutoComplete;	// auto-complete list
TScriptProfiler g_profiler;		// script profiler
CMapList	g_MapList;			// global maps information

DIR_TYPE GetDirStr( LPCTSTR pszDir )
{
	char iDir2, iDir = toupper(pszDir[0]);

	switch ( iDir )
	{
		case 'E': return DIR_E;
		case 'W': return DIR_W;
		case 'N':
			iDir2 = toupper(pszDir[1]);
			if ( iDir2 == 'E' ) return DIR_NE;
			if ( iDir2 == 'W' ) return DIR_NW;
			return DIR_N;
		case 'S':
			iDir2 = toupper(pszDir[1]);
			if ( iDir2 == 'E' ) return DIR_SE;
			if ( iDir2 == 'W' ) return DIR_SW;
			return DIR_S;
		default:
			if (( iDir >= '0' ) && ( iDir <= '7' ))
				return (DIR_TYPE)(iDir - '0');
	}
	return DIR_QTY;
}

LPCTSTR GetTimeMinDesc( int minutes )
{
	TCHAR	*pTime = Str_GetTemp();

	int minute = minutes % 60;
	int hour = ( minutes / 60 ) % 24;

	LPCTSTR pMinDif;
	if ( minute <= 14 )
//		pMinDif = "";
		pMinDif = g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_QUARTER_FIRST);
	else if ( ( minute >= 15 ) && ( minute <= 30 ) )
//		pMinDif = "a quarter past";
		pMinDif = g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_QUARTER_SECOND);
	else if ( ( minute >= 30 ) && ( minute <= 45 ) )
		//pMinDif = "half past";
		pMinDif = g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_QUARTER_THIRD);
	else
	{
//		pMinDif = "a quarter till";
		pMinDif = g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_QUARTER_FOURTH);
		hour = ( hour + 1 ) % 24;
	}
/*
	static LPCTSTR const sm_ClockHour[] =
	{
		"midnight",
		"one",
		"two",
		"three",
		"four",
		"five",
		"six",
		"seven",
		"eight",
		"nine",
		"ten",
		"eleven",
		"noon",
	};
*/
	LPCTSTR sm_ClockHour[] =
	{
 		g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_ZERO),
 		g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_ONE),
 		g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_TWO),
 		g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_THREE),
 		g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_FOUR),
 		g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_FIVE),
 		g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_SIX),
 		g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_SEVEN),
 		g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_EIGHT),
 		g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_NINE),
 		g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_TEN),
 		g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_ELEVEN),
 		g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_TWELVE),
	};

	LPCTSTR pTail;
	if ( hour == 0 || hour==12 )
		pTail = "";
	else if ( hour > 12 )
	{
		hour -= 12;
		if ((hour>=1)&&(hour<6))
			pTail = g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_13_TO_18);
//			pTail = " o'clock in the afternoon";
		else if ((hour>=6)&&(hour<9))
			pTail = g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_18_TO_21);
//			pTail = " o'clock in the evening.";
		else
			pTail = g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_21_TO_24);
//			pTail = " o'clock at night";
	}
	else
	{
		pTail = g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_24_TO_12);
//		pTail = " o'clock in the morning";
	}

	sprintf( pTime, "%s %s %s", pMinDif, sm_ClockHour[hour], pTail );
	return pTime;
}

int FindStrWord( LPCTSTR pTextSearch, LPCTSTR pszKeyWord )
{
	// Find the pszKeyWord in the pTextSearch string.
	// Make sure we look for starts of words.

	int j=0;
	for ( int i=0; 1; i++ )
	{
		if ( pszKeyWord[j] == '\0' )
		{
			if ( pTextSearch[i]=='\0' || ISWHITESPACE(pTextSearch[i]))
				return( i );
			return( 0 );
		}
		if ( pTextSearch[i] == '\0' )
			return( 0 );
		if ( !j && i )
		{
			if ( IsAlpha( pTextSearch[i-1] ))	// not start of word ?
				continue;
		}
		if ( toupper( pTextSearch[i] ) == toupper( pszKeyWord[j] ))
			j++;
		else
			j=0;
	}
}

//*******************************************************************
//	Main server loop

Main::Main()
	: AbstractThread("Main", IThread::RealTime)
{
}

void Main::onStart()
{
	SetExceptionTranslator();
}

void Main::tick()
{
	Sphere_OnTick();
	if( g_Serv.m_iExitFlag )
	{
		terminate();
	}
}

Main g_Main;
extern PingServer g_PingServer;
extern CDataBaseAsyncHelper g_asyncHdb;
#ifndef _WIN32
	extern LinuxEv g_NetworkEvent;
#endif

//*******************************************************************
// CProfileData

void CProfileData::SetActive(int iSampleSec)
{
	ADDTOCALLSTACK("CProfileData::SetActive");
	m_iActiveWindowSec = iSampleSec;
	memset(m_AvgTimes, 0, sizeof(m_AvgTimes));
	memset(m_CurTimes, 0, sizeof(m_CurTimes));
	memset(m_PrvTimes, 0, sizeof(m_PrvTimes));
	m_iAvgCount		= 1;

	if ( !m_iActiveWindowSec )
		return;

	LONGLONG llTicks;
	TIME_PROFILE_START;
	m_CurTime = llTicks;

	m_CurTask = PROFILE_OVERHEAD;
	m_TimeTotal = 0;
}

void CProfileData::Start(PROFILE_TYPE id)
{
	ADDTOCALLSTACK("CProfileData::Start");
	if (( id < 0 ) || ( id >= PROFILE_TIME_QTY ) || !m_iActiveWindowSec )
		return;

	// Stop prev task.
	if ( m_TimeTotal >= llTimeProfileFrequency * m_iActiveWindowSec )
	{
		for ( int i = 0; i < PROFILE_QTY; i++ )
		{
			if ( m_iAvgCount < 4 )
				memcpy( m_AvgTimes, m_CurTimes, sizeof( m_AvgTimes ));
			else
			{
			if ( m_PrvTimes[i].m_Time > llTimeProfileFrequency )
				m_PrvTimes[i].m_Time	= llTimeProfileFrequency;
			m_AvgTimes[i].m_Time	= (((m_AvgTimes[i].m_Time * 90)
									+ (m_PrvTimes[i].m_Time*10))/100);
			m_AvgTimes[i].m_iCount	= (((m_AvgTimes[i].m_iCount * 95)
									+ (m_PrvTimes[i].m_iCount*10))/100);
			}
		}

		++m_iAvgCount;

		memcpy( m_PrvTimes, m_CurTimes, sizeof( m_PrvTimes ));
		memset( m_CurTimes, 0, sizeof( m_CurTimes ));
		m_TimeTotal = 0;
	}

	// Get the current precise time.
	LONGLONG llTicks;
	TIME_PROFILE_START;

	// accumulate the time for this task.
	LONGLONG Diff = ( llTicks - m_CurTime );
	m_TimeTotal += Diff;
	m_CurTimes[m_CurTask].m_Time += Diff;
	m_CurTimes[m_CurTask].m_iCount ++;

	// We are now on to the new task.
	m_CurTime = llTicks;
	m_CurTask = id;
}

PROFILE_TYPE CProfileData::GetCurrentTask()
{
	return m_CurTask;
}

LPCTSTR CProfileData::GetName(PROFILE_TYPE id) const
{
	static LPCTSTR const sm_pszProfileName[PROFILE_QTY] =
	{
		"IDLE",
		"OVERHEAD",
		"NETWORK_RX",
		"CLIENTS",
		"NETWORK_TX",
		"CHARS",
		"ITEMS",
		"MAP",
		"NPC_AI",
		"SCRIPTS",
		"DATA_TX",
		"DATA_RX",
	};
	return (( id >= 0 ) && ( id < PROFILE_QTY )) ? sm_pszProfileName[id] : "";
}

LPCTSTR CProfileData::GetDesc(PROFILE_TYPE id) const
{
	ADDTOCALLSTACK("CProfileData::GetDesc");
	TCHAR	*pszTmp = Str_GetTemp();
	int		iCount	= m_PrvTimes[id].m_iCount;

	if ( id >= PROFILE_TIME_QTY )
	{
		sprintf(pszTmp, "%i (avg: %i) bytes", (int) m_PrvTimes[id].m_Time, m_AvgTimes[id].m_Time);
	}
	else
	{
		sprintf( pszTmp, "%i.%04is   avg: %i.%04is     [samples:%5i  avg:%5i ]  runtime: %is",
			(int)( m_PrvTimes[id].m_Time / ( llTimeProfileFrequency )),
			(int)((( m_PrvTimes[id].m_Time * 10000 ) / ( llTimeProfileFrequency )) % 10000 ),
			(int) ( m_AvgTimes[id].m_Time / ( llTimeProfileFrequency )),
			(int) ((( m_AvgTimes[id].m_Time * 10000 ) / ( llTimeProfileFrequency )) % 10000 ),
			iCount,
			(int) m_AvgTimes[id].m_iCount,
			m_iAvgCount );
	}
	return pszTmp;
}

//*******************************************************************

int Sphere_InitServer( int argc, char *argv[] )
{
	const char *m_sClassName = "Sphere";
	EXC_TRY("Init");
	ASSERT(MAX_BUFFER >= sizeof(CCommand));
	ASSERT(MAX_BUFFER >= sizeof(CEvent));
	ASSERT(sizeof(int) == sizeof(DWORD));	// make this assumption often.
	ASSERT(sizeof(ITEMID_TYPE) == sizeof(DWORD));
	ASSERT(sizeof(WORD) == 2 );
	ASSERT(sizeof(DWORD) == 4 );
	ASSERT(sizeof(NWORD) == 2 );
	ASSERT(sizeof(NDWORD) == 4 );
	ASSERT(sizeof(CUOItemTypeRec) == 37 );	// byte pack working ?

#ifdef _WIN32
	if ( !QueryPerformanceFrequency((LARGE_INTEGER *)&llTimeProfileFrequency))
		llTimeProfileFrequency = 1000;

	EXC_SET("setting exception catcher");
	SetExceptionTranslator();
#endif // _WIN32

	EXC_SET("loading");
	if ( !g_Serv.Load() )
		return -3;

	if ( argc > 1 )
	{
		EXC_SET("cmdline");
		if ( !g_Serv.CommandLine(argc, argv) )
			return -1;
	}

	WritePidFile(2);

	EXC_SET("sockets init");
	if ( !g_Serv.SocketsInit() )
		return -9;
	EXC_SET("load world");
	if ( !g_World.LoadAll() )
		return -8;

	//	load auto-complete dictionary
	EXC_SET("auto-complete");
	{
		CFileText	dict;
		if ( dict.Open(GRAY_FILE ".dic", OF_READ|OF_TEXT|OF_DEFAULTMODE) )
		{
			TCHAR	*pszTemp = Str_GetTemp();
			int		i(0);
			while ( !dict.IsEOF() )
			{
				dict.ReadString(pszTemp, SCRIPT_MAX_LINE_LEN-1);
				if ( *pszTemp )
				{
					char *c = strchr(pszTemp, '\r');
					if ( c ) *c = 0;
					c = strchr(pszTemp, '\n');
					if ( c ) *c = 0;
					if ( *pszTemp )
					{
						i++;
						g_AutoComplete.AddTail(pszTemp);
					}
				}
			}
			g_Log.Event(LOGM_INIT, "Auto-complete dictionary loaded (contains %i words).\n", i);
			dict.Close();
		}
	}
	g_Serv.SetServerMode(SERVMODE_Run);	// ready to go.

	// Display EF/OF Flags
	g_Cfg.PrintEFOFFlags();

	EXC_SET("finilizing");
	g_Log.Event(LOGM_INIT, g_Serv.GetStatusString(0x24));
	g_Log.Event(LOGM_INIT, "Startup complete. items=%d, chars=%d\n", g_Serv.StatGet(SERV_STAT_ITEMS), g_Serv.StatGet(SERV_STAT_CHARS));

#ifdef _WIN32
	g_Log.Event(LOGM_INIT, "Press '?' for console commands\n");
#endif

	// Trigger server start
	g_Serv.r_Call("f_onserver_start", &g_Serv, NULL);
	return 0;

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("cmdline argc=%d starting with %x (argv1='%s')\n", argc, argv, ( argc > 2 ) ? argv[1] : "");
	EXC_DEBUG_END;
	return -10;
}

void Sphere_ExitServer()
{
	// Trigger server quit
	g_Serv.r_Call("f_onserver_exit", &g_Serv, NULL);

	g_Serv.SetServerMode(SERVMODE_Exiting);

	g_NetworkOut.waitForClose();
	g_Main.waitForClose();
	g_PingServer.waitForClose();
	g_asyncHdb.waitForClose();
#ifndef _WIN32
	if ( g_Cfg.m_fUseAsyncNetwork )
		g_NetworkEvent.waitForClose();
#endif
		
	g_Serv.SocketsClose();
	g_World.Close();

	g_Log.Event(LOGM_INIT|LOGL_FATAL, g_Serv.m_iExitFlag ?
		"Server terminated by error %d!\n" : "Server shutdown (code %d) complete!\n",
		g_Serv.m_iExitFlag);
	g_Log.Close();
}

int Sphere_OnTick()
{
	// Give the world (CMainTask) a single tick. RETURN: 0 = everything is fine.
	const char *m_sClassName = "Sphere";
	EXC_TRY("Tick");
#ifdef _WIN32
	EXC_SET("service");
	g_Service.OnTick();
#endif
	EXC_SET("world");
	g_World.OnTick();

	// process incoming data
	g_Serv.m_Profile.Start( PROFILE_NETWORK_RX );
	g_NetworkIn.tick();
	g_Serv.m_Profile.Start( PROFILE_OVERHEAD );

	EXC_SET("server");
	g_Serv.OnTick();

	// push outgoing data
	g_Serv.m_Profile.Start( PROFILE_NETWORK_TX );

	if (g_NetworkOut.isActive() == false)
		g_NetworkOut.tick();

	g_Serv.m_Profile.Start( PROFILE_OVERHEAD );

	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_DEBUG_END;
	return g_Serv.m_iExitFlag;
}

//*****************************************************

static void Sphere_MainMonitorLoop()
{
	const char *m_sClassName = "Sphere";
	// Just make sure the main loop is alive every so often.
	// This should be the parent thread. try to restart it if it is not.
	while ( ! g_Serv.m_iExitFlag )
	{
		EXC_TRY("MainMonitorLoop");

		if ( g_Cfg.m_iFreezeRestartTime <= 0 )
		{
			DEBUG_ERR(("Freeze Restart Time cannot be cleared at run time\n"));
			g_Cfg.m_iFreezeRestartTime = 10;
		}

		EXC_SET("Sleep");
#ifdef _WIN32
		NTWindow_OnTick(g_Cfg.m_iFreezeRestartTime * 1000);
#else
		Sleep(g_Cfg.m_iFreezeRestartTime * 1000);
#endif

		EXC_SET("Checks");
		// Don't look for freezing when doing certain things.
		if ( g_Serv.IsLoading() || ! g_Cfg.m_fSecure || g_Serv.IsValidBusy() )
			continue;

		EXC_SET("Check Stuck");
#ifndef _DEBUG
		if (g_Main.checkStuck() == true)
			g_Log.Event(LOGL_CRIT, "'%s' thread hang, restarting...\n", g_Main.getName());
#endif
		EXC_CATCH;

		EXC_DEBUG_START;
		EXC_DEBUG_END;
	}

}

//******************************************************

#if !defined( _WIN32 )

// This is used to restore the original flags on exit
void resetNonBlockingIo()
{
	termios term_caps;

	if ( tcgetattr(STDIN_FILENO, &term_caps) < 0 ) return;

	term_caps.c_lflag |= ICANON;

	if( tcsetattr(STDIN_FILENO, TCSANOW, &term_caps) < 0 ) return;
}

void setNonBlockingIo()
{
	termios term_caps;

	if( tcgetattr( STDIN_FILENO, &term_caps ) < 0 )
	{
		printf( "ERROR: Could not get the termcap settings for this terminal.\n" );
		return;
	}

	term_caps.c_lflag &= ~ICANON;

	if( tcsetattr( STDIN_FILENO, TCSANOW, &term_caps ) < 0 )
	{
		printf( "ERROR: Could not set the termcap settings for this terminal.\n" );
		return;
	}
	setbuf(stdin, NULL);
	atexit(resetNonBlockingIo);
}
#endif

void dword_q_sort(DWORD numbers[], DWORD left, DWORD right)
{
	DWORD	pivot, l_hold, r_hold;

	l_hold = left;
	r_hold = right;
	pivot = numbers[left];
	while (left < right)
	{
		while ((numbers[right] >= pivot) && (left < right)) right--;
		if (left != right)
		{
			numbers[left] = numbers[right];
			left++;
		}
		while ((numbers[left] <= pivot) && (left < right)) left++;
		if (left != right)
		{
			numbers[right] = numbers[left];
			right--;
		}
	}
	numbers[left] = pivot;
	pivot = left;
	left = l_hold;
	right = r_hold;
	if (left < pivot) dword_q_sort(numbers, left, pivot-1);
	if (right > pivot) dword_q_sort(numbers, pivot+1, right);
}

void defragSphere(char *path)
{
	CFileText	inf;
	CGFile		ouf;
	char	z[256], z1[256], buf[1024];
	int		i;
	DWORD	uid(0);
	char	*p, *p1;
	DWORD	dBytesRead;
	DWORD	dTotalMb;
	DWORD	mb10(10*1024*1024);
	DWORD	mb5(5*1024*1024);
	bool	bSpecial;
	DWORD	dTotalUIDs;

	char	c,c1,c2;
	DWORD	d;

	//	NOTE: Sure I could use CVarDefArray, but it is extremely slow with memory allocation, takes hours
	//		to read and save the data. Moreover, it takes less memory in this case and does less convertations.
#define	MAX_UID	5000000L	// limit to 5mln of objects, takes 5mln*4 = 20mb
	DWORD	*uids;

	g_Log.Event(LOGM_INIT,	"Defragmentation (UID alteration) of " GRAY_TITLE " saves.\n"
		"Use it on your risk and if you know what you are doing since it can possibly harm your server.\n"
		"The process can take up to several hours depending on the CPU you have.\n"
		"After finished, you will have your '" GRAY_FILE "*.scp' files converted and saved as '" GRAY_FILE "*.scp.new'.\n");

	uids = (DWORD*)calloc(MAX_UID, sizeof(DWORD));
	for ( i = 0; i < 3; i++ )
	{
		strcpy(z, path);
		if ( i == 0 ) strcat(z, GRAY_FILE "statics.scp");
		else if ( i == 1 ) strcat(z, GRAY_FILE "world.scp");
		else strcat(z, GRAY_FILE "chars.scp");

		g_Log.Event(LOGM_INIT, "Reading current UIDs: %s\n", z);
		if ( !inf.Open(z, OF_READ|OF_TEXT|OF_DEFAULTMODE) )
		{
			g_Log.Event(LOGM_INIT, "Cannot open file for reading. Skipped!\n");
			continue;
		}
		dBytesRead = dTotalMb = 0;
		while ( !feof(inf.m_pStream) )
		{
			fgets(buf, sizeof(buf), inf.m_pStream);
			dBytesRead += strlen(buf);
			if ( dBytesRead > mb10 )
			{
				dBytesRead -= mb10;
				dTotalMb += 10;
				g_Log.Event(LOGM_INIT, "Total read %u Mb\n", dTotalMb);
			}
			if (( buf[0] == 'S' ) && ( strstr(buf, "SERIAL=") == buf ))
			{
				p = buf + 7;
				p1 = p;
				while ( *p1 && ( *p1 != '\r' ) && ( *p1 != '\n' )) p1++;
				*p1 = 0;

				//	prepare new uid
				*(p-1) = '0';
				*p = 'x';
				p--;
				uids[uid++] = strtoul(p, &p1, 16);
			}
		}
		inf.Close();
	}
	dTotalUIDs = uid;
	g_Log.Event(LOGM_INIT, "Totally having %u unique objects (UIDs), latest: 0%x\n", uid, uids[uid-1]);

	g_Log.Event(LOGM_INIT, "Quick-Sorting the UIDs array...\n");
	dword_q_sort(uids, 0, dTotalUIDs-1);

	for ( i = 0; i < 5; i++ )
	{
		strcpy(z, path);
		if ( !i ) strcat(z, GRAY_FILE "accu.scp");
		else if ( i == 1 ) strcat(z, GRAY_FILE "chars.scp");
		else if ( i == 2 ) strcat(z, GRAY_FILE "data.scp");
		else if ( i == 3 ) strcat(z, GRAY_FILE "world.scp");
		else if ( i == 4 ) strcat(z, GRAY_FILE "statics.scp");
		g_Log.Event(LOGM_INIT, "Updating UID-s in %s to %s.new\n", z, z);
		if ( !inf.Open(z, OF_READ|OF_TEXT|OF_DEFAULTMODE) )
		{
			g_Log.Event(LOGM_INIT, "Cannot open file for reading. Skipped!\n");
			continue;
		}
		strcat(z, ".new");
		if ( !ouf.Open(z, OF_WRITE|OF_CREATE|OF_DEFAULTMODE) )
		{
			g_Log.Event(LOGM_INIT, "Cannot open file for writing. Skipped!\n");
			continue;
		}
		dBytesRead = dTotalMb = 0;
		while ( inf.ReadString(buf, sizeof(buf)) )
		{
			uid = strlen(buf);
			buf[uid] = buf[uid+1] = buf[uid+2] = 0;	// just to be sure to be in line always
							// NOTE: it is much faster than to use memcpy to clear before reading
			bSpecial = false;
			dBytesRead += uid;
			if ( dBytesRead > mb5 )
			{
				dBytesRead -= mb5;
				dTotalMb += 5;
				g_Log.Event(LOGM_INIT, "Total processed %u Mb\n", dTotalMb);
			}
			p = buf;
			if ( 0 ) ;
			//	Note 28-Jun-2004
			//	mounts seems having ACTARG1 > 0x30000000. The actual UID is ACTARG1-0x30000000. The
			//	new also should be new+0x30000000. need investigation if this can help making mounts
			//	not to disappear after the defrag
			else if (( buf[0] == 'A' ) && ( strstr(buf, "ACTARG1=0") == buf ))		// ACTARG1=
				p += 8;
			else if (( buf[0] == 'C' ) && ( strstr(buf, "CONT=0") == buf ))			// CONT=
				p += 5;
			else if (( buf[0] == 'C' ) && ( strstr(buf, "CHARUID=0") == buf ))		// CHARUID=
				p += 8;
			else if (( buf[0] == 'L' ) && ( strstr(buf, "LASTCHARUID=0") == buf ))	// LASTCHARUID=
				p += 12;
			else if (( buf[0] == 'L' ) && ( strstr(buf, "LINK=0") == buf ))			// LINK=
				p += 5;
			else if (( buf[0] == 'M' ) && ( strstr(buf, "MEMBER=0") == buf ))		// MEMBER=
			{
				p += 7;
				bSpecial = true;
			}
			else if (( buf[0] == 'M' ) && ( strstr(buf, "MORE1=0") == buf ))		// MORE1=
				p += 6;
			else if (( buf[0] == 'M' ) && ( strstr(buf, "MORE2=0") == buf ))		// MORE2=
				p += 6;
			else if (( buf[0] == 'S' ) && ( strstr(buf, "SERIAL=0") == buf ))		// SERIAL=
				p += 7;
			else if ((( buf[0] == 'T' ) && ( strstr(buf, "TAG.") == buf )) ||		// TAG.=
					 (( buf[0] == 'R' ) && ( strstr(buf, "REGION.TAG") == buf )))
			{
				while ( *p && ( *p != '=' )) p++;
				p++;
			}
			else if (( i == 2 ) && strchr(buf, '='))	// spheredata.scp - plain VARs
			{
				while ( *p && ( *p != '=' )) p++;
				p++;
			}
			else p = NULL;

			//	UIDs are always hex, so prefixed by 0
			if ( p && ( *p != '0' )) p = NULL;

			//	here we got potentialy UID-contained variable
			//	check if it really is only UID-like var containing
			if ( p )
			{
				p1 = p;
				while ( *p1 &&
					((( *p1 >= '0' ) && ( *p1 <= '9' )) ||
					 (( *p1 >= 'a' ) && ( *p1 <= 'f' )))) p1++;
				if ( !bSpecial )
				{
					if ( *p1 && ( *p1 != '\r' ) && ( *p1 != '\n' )) // some more text in line
						p = NULL;
				}
			}

			//	here we definitely know that this is very uid-like
			if ( p )
			{
				c = *p1;

				*p1 = 0;
				//	here in p we have the current value of the line.
				//	check if it is a valid UID

				//	prepare converting 0.. to 0x..
				c1 = *(p-1);
				c2 = *p;
				*(p-1) = '0';
				*p = 'x';
				p--;
				uid = strtoul(p, &p1, 16);
				p++;
				*(p-1) = c1;
				*p = c2;
				//	Note 28-Jun-2004
				//	The search algourytm is very simple and fast. But maybe integrate some other, at least /2 algorythm
				//	since has amount/2 tryes at worst chance to get the item and never scans the whole array
				//	It should improve speed since defragmenting 150Mb saves takes ~2:30 on 2.0Mhz CPU
				{
					DWORD	dStep = dTotalUIDs/2;
					d = dStep;
					while ( true )
					{
						dStep /= 2;

						if ( uids[d] == uid )
						{
							uid = d | (uids[d]&0xF0000000);	// do not forget attach item and special flags like 04..
							break;
						}
						else if ( uids[d] < uid ) d += dStep;
						else d -= dStep;

						if ( dStep == 1 )
						{
							uid = 0xFFFFFFFFL;
							break; // did not find the UID
						}
					}
				}

				//	Search for this uid in the table
/*				for ( d = 0; d < dTotalUIDs; d++ )
				{
					if ( !uids[d] )	// end of array
					{
						uid = 0xFFFFFFFFL;
						break;
					}
					else if ( uids[d] == uid )
					{
						uid = d | (uids[d]&0xF0000000);	// do not forget attach item and special flags like 04..
						break;
					}
				}*/

				//	replace UID by the new one since it has been found
				*p1 = c;
				if ( uid != 0xFFFFFFFFL )
				{
					*p = 0;
					strcpy(z, p1);
					sprintf(z1, "0%x", uid);
					strcat(buf, z1);
					strcat(buf, z);
				}
			}
			//	output the resulting line
			ouf.Write(buf, strlen(buf));
		}
		inf.Close();
		ouf.Close();
	}
	free(uids);
	g_Log.Event(LOGM_INIT,	"Defragmentation complete.\n");
}

#ifdef _WIN32
int Sphere_MainEntryPoint( int argc, char *argv[] )
#else
int _cdecl main( int argc, char * argv[] )
#endif
{
#ifndef _WIN32
	// Initialize nonblocking IO and disable readline on linux
	setNonBlockingIo();
#endif

	g_Serv.m_iExitFlag = Sphere_InitServer( argc, argv );
	if ( ! g_Serv.m_iExitFlag )
	{
		WritePidFile();

		// Start the ping server, this can only be ran in a separate thread
		if ( IsSetEF( EF_UsePingServer ) )
			g_PingServer.start();
		
#ifndef _WIN32
		if ( g_Cfg.m_fUseAsyncNetwork )
			g_NetworkEvent.start();
#endif

		g_NetworkIn.onStart();
		if (IsSetEF( EF_NetworkOutThread ))
			g_NetworkOut.start();
			
		bool shouldRunInThread = ( g_Cfg.m_iFreezeRestartTime > 0 );

		if( shouldRunInThread )
		{
			g_Main.start();
			Sphere_MainMonitorLoop();
		}
		else
		{
			while( !g_Serv.m_iExitFlag )
			{
				g_Main.tick();
			}
		}
	}

	Sphere_ExitServer();
	WritePidFile(true);
	return( g_Serv.m_iExitFlag );
}

#include "../tables/classnames.tbl"
