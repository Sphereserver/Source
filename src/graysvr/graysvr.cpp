#include "graysvr.h"	// predef header.
#include "../common/grayver.h"	// sphere version
#include "PingServer.h"	// ping server
#include "../network/network.h" // network thread
#include "../sphere/asyncdb.h"
#if !defined(_WIN32) || defined(_LIBEV)
	#include "../sphere/linuxev.h"
	#include "UnixTerminal.h"
#endif

#if !defined(pid_t)
#define pid_t int
#endif

#ifdef _WIN32
	#include "ntservice.h"	// g_Service
	#include <process.h>	// getpid()
#endif

//Trigger function start

struct T_TRIGGERS
{
	char	m_name[48];
	long	m_used;
};

std::vector<T_TRIGGERS> g_triggers;

bool IsTrigUsed(E_TRIGGERS id)
{
	if ( g_Serv.IsLoading() == true)
		return false;
	return (( static_cast<unsigned>(id) < g_triggers.size() ) && g_triggers[id].m_used );
}

bool IsTrigUsed(const char *name)
{
	if ( g_Serv.IsLoading() == true)
		return false;
	std::vector<T_TRIGGERS>::iterator it;
	for ( it = g_triggers.begin(); it != g_triggers.end(); ++it )
	{
		if ( !strcmpi(it->m_name, name) )
			return (it->m_used != 0); // Returns true or false for known triggers
	}
	return true; //Must return true for custom triggers
}

void TriglistInit()
{
	T_TRIGGERS	trig;
	g_triggers.clear();

#define ADD(_a_)	strcpy(trig.m_name, "@"); strcat(trig.m_name, #_a_); trig.m_used = 0; g_triggers.push_back(trig);
#include "../tables/triggers.tbl"

}

void TriglistClear()
{
	std::vector<T_TRIGGERS>::iterator it;
	for ( it = g_triggers.begin(); it != g_triggers.end(); ++it )
	{
		(*it).m_used = 0;
	}
}

void TriglistAdd(E_TRIGGERS id)
{
	if ( g_triggers.size() )
		g_triggers[id].m_used++;
}

void TriglistAdd(const char *name)
{
	std::vector<T_TRIGGERS>::iterator it;
	for ( it = g_triggers.begin(); it != g_triggers.end(); ++it )
	{
		if ( !strcmpi(it->m_name, name) )
		{
			it->m_used++;
			break;
		}
	}
}

void Triglist(long &total, long &used)
{
	total = used = 0;
	std::vector<T_TRIGGERS>::iterator it;
	for ( it = g_triggers.begin(); it != g_triggers.end(); ++it )
	{
		total++;
		if ( it->m_used )
			used++;
	}
}

void TriglistPrint()
{
	std::vector<T_TRIGGERS>::iterator it;
	for ( it = g_triggers.begin(); it != g_triggers.end(); ++it )
	{
		if ( it->m_used )
			g_Serv.SysMessagef("Trigger %-25s Used %ld time%s\n", it->m_name, it->m_used, (it->m_used > 1) ? "s" : "");
		else
			g_Serv.SysMessagef("Trigger %-25s Unused\n", it->m_name);
	}
}

//Trigger function end

CMapList::CMapList()
{
	memset(m_mapsinitalized, 0, sizeof(m_mapsinitalized));
	memset(m_sizex, 0, sizeof(m_sizex));
	memset(m_sizey, 0, sizeof(m_sizey));
	memset(m_maps, true, sizeof(m_maps));
	memset(m_mapnum, -1, sizeof(m_mapnum));
	memset(m_mapid, -1, sizeof(m_mapid));
	memset(m_sectorsize, 0, sizeof(m_sectorsize));

	for (int i = 0; i < 6; i++)
		Load(i, 0, 0, 0, i, i);

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
		TCHAR * ppCmd[5];	// maxx,maxy,sectorsize,mapnum[like 0 for map0/statics0/staidx0],mapid
		size_t iCount = Str_ParseCmds(args, ppCmd, COUNTOF(ppCmd), ",");

		if ( iCount <= 0 )	// simple MAPX= same as disabling the map
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

bool CMapList::DetectMapSize(int map)
{
	if ( m_maps[map] == false )
		return false;

	int	index = m_mapnum[map];
	if ( index < 0 )
		return false;

	if (g_Install.m_Maps[index].IsFileOpen() == false)
		return false;

	//
	//	#0 - map0.mul			(felucca, 6144x4096 or 7168x4096, 77070336 or 89915392 bytes)
	//	#1 - map0 or map1.mul	(trammel, 6144x4096 or 7168x4096, 77070336 or 89915392 bytes)
	//	#2 - map2.mul			(ilshenar, 2304x1600, 11289600 bytes)
	//	#3 - map3.mul			(malas, 2560x2048, 16056320 bytes)
	//	#4 - map4.mul			(tokuno islands, 1448x1448, 6421156 bytes)
	//	#5 - map5.mul			(ter mur, 1280x4096, 16056320 bytes)
	//

	switch (index)
	{
		case 0: // map0.mul
		case 1: // map1.mul
			if (g_Install.m_Maps[index].GetLength() == 89915392 || // ML-sized
				!strcmpi(g_Install.m_Maps[index].GetFileExt(), ".uop")) // UOP are all ML-sized
				//g_Install.m_Maps[index].GetLength() == 89923808)   // (UOP packed)
			{
				if (m_sizex[map] <= 0)		m_sizex[map] = 7168;
				if (m_sizey[map] <= 0)		m_sizey[map] = 4096;
			}
			else
			{
				if (m_sizex[map] <= 0)		m_sizex[map] = 6144;
				if (m_sizey[map] <= 0)		m_sizey[map] = 4096;
			}

			if (m_sectorsize[map] <= 0)	m_sectorsize[map] = 64;
			break;

		case 2: // map2.mul
			if (m_sizex[map] <= 0)		m_sizex[map] = 2304;
			if (m_sizey[map] <= 0)		m_sizey[map] = 1600;
			if (m_sectorsize[map] <= 0)	m_sectorsize[map] = 64;
			break;

		case 3: // map3.mul
			if (m_sizex[map] <= 0)		m_sizex[map] = 2560;
			if (m_sizey[map] <= 0)		m_sizey[map] = 2048;
			if (m_sectorsize[map] <= 0)	m_sectorsize[map] = 64;
			break;

		case 4: // map4.mul
			if (m_sizex[map] <= 0)		m_sizex[map] = 1448;
			if (m_sizey[map] <= 0)		m_sizey[map] = 1448;
			if (m_sectorsize[map] <= 0)	m_sectorsize[map] = 64;
			break;

		case 5: // map5.mul
			if (m_sizex[map] <= 0)		m_sizex[map] = 1280;
			if (m_sizey[map] <= 0)		m_sizey[map] = 4096;
			if (m_sectorsize[map] <= 0)	m_sectorsize[map] = 64;
			break;

		default:
			DEBUG_ERR(("Unknown map index %d with file size of %lu bytes. Please specify the correct size manually.\n", index, g_Install.m_Maps[index].GetLength()));
			break;
	}

	return (m_sizex[map] > 0 && m_sizey[map] > 0 && m_sectorsize[map] > 0);
}

void CMapList::Init()
{
	for ( int i = 0; i < 256; i++ )
	{
		if ( m_maps[i] )	// map marked as available. check whatever it's possible
		{
			//	check coordinates first
			if ( m_mapnum[i] == -1 )
				m_maps[i] = false;
			else if ( m_sizex[i] == 0 || m_sizey[i] == 0 || m_sectorsize[i] == 0 )
				m_maps[i] = DetectMapSize(i);
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
	if ( pszFormat == NULL || pszFormat[0] == '\0' )
		return 0;

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

LPCTSTR g_szServerDescription =	GRAY_TITLE " Version " GRAY_VERSION " " GRAY_VER_FILEOS_STR	" by www.spherecommunity.net";

size_t CObjBase::sm_iCount = 0;	// UID table.
ULONGLONG llTimeProfileFrequency = 1000;	// time profiler

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
	char iDir2, iDir = static_cast<char>(toupper(pszDir[0]));

	switch ( iDir )
	{
		case 'E': return DIR_E;
		case 'W': return DIR_W;
		case 'N':
			iDir2 = static_cast<char>(toupper(pszDir[1]));
			if ( iDir2 == 'E' ) return DIR_NE;
			if ( iDir2 == 'W' ) return DIR_NW;
			return DIR_N;
		case 'S':
			iDir2 = static_cast<char>(toupper(pszDir[1]));
			if ( iDir2 == 'E' ) return DIR_SE;
			if ( iDir2 == 'W' ) return DIR_SW;
			return DIR_S;
		default:
			if (( iDir >= '0' ) && ( iDir <= '7' ))
				return static_cast<DIR_TYPE>(iDir - '0');
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
		"noon"
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

size_t FindStrWord( LPCTSTR pTextSearch, LPCTSTR pszKeyWord )
{
	// Find any of the pszKeyWord in the pTextSearch string.
	// Make sure we look for starts of words.

	size_t j = 0;
	for ( size_t i = 0; ; i++ )
	{
		if ( pszKeyWord[j] == '\0' || pszKeyWord[j] == ',')
		{
			if ( pTextSearch[i]== '\0' || ISWHITESPACE(pTextSearch[i]))
				return( i );
			j = 0;
		}
		if ( pTextSearch[i] == '\0' )
		{
			pszKeyWord = strchr(pszKeyWord, ',');
			if (pszKeyWord)
			{
				++pszKeyWord;
				i = 0;
				j = 0;
			}
			else
			return( 0 );
		}
		if ( j == 0 && i > 0 )
		{
			if ( IsAlpha( pTextSearch[i-1] ))	// not start of word ?
				continue;
		}
		if ( toupper( pTextSearch[i] ) == toupper( pszKeyWord[j] ))
			j++;
		else
			j = 0;
	}
}

//*******************************************************************
//	Main server loop

Main::Main()
	: AbstractSphereThread("Main", IThread::RealTime)
{
	m_profile.EnableProfile(PROFILE_NETWORK_RX);
	m_profile.EnableProfile(PROFILE_CLIENTS);
	//m_profile.EnableProfile(PROFILE_NETWORK_TX);
	m_profile.EnableProfile(PROFILE_CHARS);
	m_profile.EnableProfile(PROFILE_ITEMS);
	m_profile.EnableProfile(PROFILE_MAP);
	m_profile.EnableProfile(PROFILE_NPC_AI);
	m_profile.EnableProfile(PROFILE_SCRIPTS);
#ifndef _MTNETWORK
	//m_profile.EnableProfile(PROFILE_DATA_TX);
	m_profile.EnableProfile(PROFILE_DATA_RX);
#else
#ifndef MTNETWORK_INPUT
	m_profile.EnableProfile(PROFILE_DATA_RX);
#endif
#ifndef MTNETWORK_OUTPUT
	m_profile.EnableProfile(PROFILE_DATA_TX);
#endif
#endif
}

void Main::onStart()
{
	AbstractSphereThread::onStart();
	SetExceptionTranslator();
}

void Main::tick()
{
	Sphere_OnTick();
}

bool Main::shouldExit()
{
	if (g_Serv.m_iExitFlag != 0)
		return true;
	return AbstractSphereThread::shouldExit();
}

Main g_Main;
extern PingServer g_PingServer;
extern CDataBaseAsyncHelper g_asyncHdb;
#if !defined(_WIN32) || defined(_LIBEV)
	extern LinuxEv g_NetworkEvent;
#endif

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
	ASSERT(sizeof(CUOItemTypeRec) == 37 ); // byte pack working ?
	ASSERT((std::numeric_limits<size_t>::min)() == 0); // ensure unsigned

#ifdef _WIN32
	if ( !QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER *>(&llTimeProfileFrequency)) )
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

	EXC_SET("load world");
	g_World.LoadAll();

	EXC_SET("sockets init");
	if ( !g_Serv.SocketsInit() )
		return -9;

	//	load auto-complete dictionary
	EXC_SET("auto-complete");
	{
		CFileText	dict;
		if ( dict.Open(GRAY_FILE ".dic", OF_READ|OF_TEXT|OF_DEFAULTMODE) )
		{
			TCHAR * pszTemp = Str_GetTemp();
			size_t count = 0;
			while ( !dict.IsEOF() )
			{
				dict.ReadString(pszTemp, SCRIPT_MAX_LINE_LEN-1);
				if ( *pszTemp )
				{
					TCHAR *c = strchr(pszTemp, '\r');
					if ( c != NULL )
						*c = '\0';

					c = strchr(pszTemp, '\n');
					if ( c != NULL )
						*c = '\0';

					if ( *pszTemp != '\0' )
					{
						count++;
						g_AutoComplete.AddTail(pszTemp);
					}
				}
			}
			g_Log.Event(LOGM_INIT, "Auto-complete dictionary loaded (contains %" FMTSIZE_T " words)\n", count);
			dict.Close();
		}
	}
	g_Serv.SetServerMode(SERVMODE_Run);	// ready to go.

	EXC_SET("finilizing");
	g_Log.Event(LOGM_INIT, "Startup complete (Items=%lu, Chars=%lu, Accounts=%lu)\nPress '?' for console commands\n\n", g_Serv.StatGet(SERV_STAT_ITEMS), g_Serv.StatGet(SERV_STAT_CHARS), g_Serv.StatGet(SERV_STAT_ACCOUNTS));

	if ( !g_Accounts.Account_GetCount() )
		g_Log.Event(LOGL_WARN|LOGM_INIT, "The server has no accounts. To create admin account you must type:\n  ACCOUNT ADD [login] [password]\n  ACCOUNT [login] PLEVEL 7\n");

	// Trigger server start
	g_Serv.r_Call("f_onserver_start", &g_Serv, NULL);
	return 0;

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("cmdline argc=%d starting with %p (argv1='%s')\n", argc, static_cast<void *>(argv), ( argc > 2 ) ? argv[1] : "");
	EXC_DEBUG_END;
	return -10;
}

void Sphere_ExitServer()
{
	// Trigger server quit
	g_Serv.r_Call("f_onserver_exit", &g_Serv, NULL);

	g_Serv.SetServerMode(SERVMODE_Exiting);

#ifndef _MTNETWORK
	g_NetworkOut.waitForClose();
#else
	g_NetworkManager.stop();
#endif
	g_Main.waitForClose();
	g_PingServer.waitForClose();
	g_asyncHdb.waitForClose();
#if !defined(_WIN32) || defined(_LIBEV)
	if ( g_Cfg.m_fUseAsyncNetwork != 0 )
		g_NetworkEvent.waitForClose();
#endif
		
	g_Serv.SocketsClose();
	g_World.Close();

	LPCTSTR Reason;
	switch ( g_Serv.m_iExitFlag )
	{
		case -10:	Reason = "Unexpected error occurred";			break;
		case -9:	Reason = "Failed to bind server IP/port";		break;
		case -8:	Reason = "Failed to load worldsave files";		break;
		case -3:	Reason = "Failed to load server settings";		break;
		case -1:	Reason = "Shutdown via commandline";			break;
#ifdef _WIN32
		case 1:		Reason = "X command on console";				break;
#else
		case 1:		Reason = "Terminal closed by SIGHUP signal";	break;
#endif
		case 2:		Reason = "SHUTDOWN command executed";			break;
		case 4:		Reason = "Service shutdown";					break;
		case 5:		Reason = "Console window closed";				break;
		case 6:		Reason = "Proccess aborted by SIGABRT signal";	break;
		default:	Reason = "Server shutdown complete";			break;
	}

	g_Log.Event(LOGM_INIT|LOGL_FATAL, "Server terminated: %s (code %d)\n", Reason, g_Serv.m_iExitFlag);
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
	EXC_SET("ships_tick");
	g_Serv.ShipTimers_Tick();

	EXC_SET("world");
	g_World.OnTick();

	// process incoming data
	EXC_SET("network-in");
#ifndef _MTNETWORK
	g_NetworkIn.tick();
#else
	g_NetworkManager.processAllInput();
#endif

	EXC_SET("server");
	g_Serv.OnTick();

	// push outgoing data
#ifndef _MTNETWORK
	if (g_NetworkOut.isActive() == false)
	{
		EXC_SET("network-out");
		g_NetworkOut.tick();
	}
#else
	EXC_SET("network-tick");
	g_NetworkManager.tick();

	EXC_SET("network-out");
	g_NetworkManager.processAllOutput();
#endif

	EXC_CATCH;
	return g_Serv.m_iExitFlag;
}
void CServer::ShipTimers_Tick()
{
	ADDTOCALLSTACK("CServer::ShipTimers_Tick");
	std::vector<CItemShip *>::iterator it;
	for ( it = m_ShipTimers.begin(); it != m_ShipTimers.end(); ) 
	{
		CItemShip * pShip = *it;
		if (pShip && pShip->m_itShip.m_fSail != 0)
		{
			pShip->OnTick();
			++it;
		}
		else if ( m_ShipTimers.size() == 1 )
		{
			m_ShipTimers.pop_back();
			break;
		}
		else
			it = m_ShipTimers.erase(it);
	}
}

void CServer::ShipTimers_Add(CItemShip * ship)
{
	ADDTOCALLSTACK("CServer::ShipTimers_Add");
	if (!ship)
		return;
	m_ShipTimers.push_back(ship);
}

void CServer::ShipTimers_Delete(CItemShip * ship)
{
	ADDTOCALLSTACK("CServer::ShipTimers_Delete");
	std::vector<CItemShip *>::iterator it;
	for ( it = m_ShipTimers.begin(); it != m_ShipTimers.end(); ) 
	{
		CItemShip * pShip = *it;
		if (pShip == ship)
		{	
			if ( m_ShipTimers.size() == 1 )
			{
				m_ShipTimers.pop_back();
				break;
			}
			else
				it = m_ShipTimers.erase(it);
		}
		else
			++it;
	}
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
		// only sleep 1 second at a time, to avoid getting stuck here when closing
		// down with large m_iFreezeRestartTime values set
		for (int i = 0; i < g_Cfg.m_iFreezeRestartTime; ++i)
		{
			if ( g_Serv.m_iExitFlag )
				break;

#ifdef _WIN32
			NTWindow_OnTick(1000);
#else
			Sleep(1000);
#endif
		}

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
	}

}

//******************************************************
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
	ASSERT(path != NULL);

	CFileText inf;
	CGFile ouf;
	char z[256], z1[256], buf[1024];
	size_t i;
	DWORD uid(0);
	char *p(NULL), *p1(NULL);
	DWORD dBytesRead;
	DWORD dTotalMb;
	DWORD mb10(10*1024*1024);
	DWORD mb5(5*1024*1024);
	bool bSpecial;
	DWORD dTotalUIDs;

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
		if ( i == 0 ) strcat(z, GRAY_FILE "statics" GRAY_SCRIPT);
		else if ( i == 1 ) strcat(z, GRAY_FILE "world" GRAY_SCRIPT);
		else strcat(z, GRAY_FILE "chars" GRAY_SCRIPT);

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
				g_Log.Event(LOGM_INIT, "Total read %lu Mb\n", dTotalMb);
			}
			if (( buf[0] == 'S' ) && ( strstr(buf, "SERIAL=") == buf ))
			{
				p = buf + 7;
				p1 = p;
				while ( *p1 && ( *p1 != '\r' ) && ( *p1 != '\n' ))
					p1++;
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
	g_Log.Event(LOGM_INIT, "Totally having %lu unique objects (UIDs), latest: 0%lx\n", uid, uids[uid-1]);

	g_Log.Event(LOGM_INIT, "Quick-Sorting the UIDs array...\n");
	dword_q_sort(uids, 0, dTotalUIDs-1);

	for ( i = 0; i < 5; i++ )
	{
		strcpy(z, path);
		if ( !i ) strcat(z, GRAY_FILE "accu.scp");
		else if ( i == 1 ) strcat(z, GRAY_FILE "chars" GRAY_SCRIPT);
		else if ( i == 2 ) strcat(z, GRAY_FILE "data" GRAY_SCRIPT);
		else if ( i == 3 ) strcat(z, GRAY_FILE "world" GRAY_SCRIPT);
		else if ( i == 4 ) strcat(z, GRAY_FILE "statics" GRAY_SCRIPT);
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
			if (uid > (COUNTOF(buf) - 3))
				uid = COUNTOF(buf) - 3;

			buf[uid] = buf[uid+1] = buf[uid+2] = 0;	// just to be sure to be in line always
							// NOTE: it is much faster than to use memcpy to clear before reading
			bSpecial = false;
			dBytesRead += uid;
			if ( dBytesRead > mb5 )
			{
				dBytesRead -= mb5;
				dTotalMb += 5;
				g_Log.Event(LOGM_INIT, "Total processed %lu Mb\n", dTotalMb);
			}
			p = buf;

			//	Note 28-Jun-2004
			//	mounts seems having ACTARG1 > 0x30000000. The actual UID is ACTARG1-0x30000000. The
			//	new also should be new+0x30000000. need investigation if this can help making mounts
			//	not to disappear after the defrag
			if (( buf[0] == 'A' ) && ( strstr(buf, "ACTARG1=0") == buf ))		// ACTARG1=
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
					for (;;)
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
					sprintf(z1, "0%lx", uid);
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
	g_UnixTerminal.prepare();
#endif

	g_Serv.m_iExitFlag = Sphere_InitServer( argc, argv );
	if ( ! g_Serv.m_iExitFlag )
	{
		WritePidFile();

		// Start the ping server, this can only be ran in a separate thread
		if ( IsSetEF( EF_UsePingServer ) )
			g_PingServer.start();
		
#if !defined(_WIN32) || defined(_LIBEV)
		if ( g_Cfg.m_fUseAsyncNetwork != 0 )
			g_NetworkEvent.start();
#endif

#ifndef _MTNETWORK
		g_NetworkIn.onStart();
		if (IsSetEF( EF_NetworkOutThread ))
			g_NetworkOut.start();
#else
		g_NetworkManager.start();
#endif
			
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

#ifdef _WIN32
	NTWindow_DeleteIcon();
#endif

	Sphere_ExitServer();
	WritePidFile(true);
	return( g_Serv.m_iExitFlag );
}

#include "../tables/classnames.tbl"
