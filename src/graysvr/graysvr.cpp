#include "graysvr.h"	// predef header
#include "CPingServer.h"
#include "../network/network.h"
#include "../sphere/asyncdb.h"
#ifndef _WIN32
	#include "CUnixTerminal.h"
#endif

#if !defined(pid_t)
	#define pid_t int
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
	if ( g_Serv.IsLoading() )
		return false;
	return ((static_cast<size_t>(id) < g_triggers.size()) && g_triggers[id].m_used);
}

bool IsTrigUsed(const char *name)
{
	if ( g_Serv.IsLoading() )
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
	T_TRIGGERS trig;
	g_triggers.clear();
	#define ADD(_a_)	snprintf(trig.m_name, sizeof(trig.m_name), "@%s", #_a_); trig.m_used = 0; g_triggers.push_back(trig);
	#include "../tables/triggers.tbl"
	#undef ADD
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
		++g_triggers[id].m_used;
}

void TriglistAdd(const char *name)
{
	std::vector<T_TRIGGERS>::iterator it;
	for ( it = g_triggers.begin(); it != g_triggers.end(); ++it )
	{
		if ( !strcmpi(it->m_name, name) )
		{
			++it->m_used;
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
		++total;
		if ( it->m_used )
			++used;
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
	memset(m_sizex, 0, sizeof(m_sizex));
	memset(m_sizey, 0, sizeof(m_sizey));
	memset(m_sectorsize, 0, sizeof(m_sectorsize));
	memset(m_maps, true, sizeof(m_maps));
	memset(m_mapnum, -1, sizeof(m_mapnum));
	memset(m_mapid, -1, sizeof(m_mapid));
	memset(m_mapsinitalized, 0, sizeof(m_mapsinitalized));
	m_pMapDiffCollection = NULL;

	for ( int m = 0; m < MAP_QTY; ++m )
		Load(m, 0, 0, 0, m, m);
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
	if ( (map < 0) || (map >= MAP_QTY) )
	{
		g_Log.EventError("Can't initialize invalid map #%d\n", map);
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
					g_Log.EventError("MAP%d: X coord must be multiple of 8 (%d is invalid, %d is still effective)\n", map, maxx, m_sizex[map]);
				}
				else m_sizex[map] = maxx;
			}
			if ( maxy )
			{
				if (( maxy < 8 ) || ( maxy % 8 ))
				{
					g_Log.EventError("MAP%d: Y coord must be multiple of 8 (%d is invalid, %d is still effective)\n", map, maxy, m_sizey[map]);
				}
				else m_sizey[map] = maxy;
			}
			if ( sectorsize > 0 )
			{
				if ( (sectorsize < 8) || (sectorsize % 8) )
					g_Log.EventError("MAP%d: Sector size must be multiple of 8 (%d is invalid, %d is still effective)\n", map, sectorsize, m_sectorsize[map]);
				else if ( (m_sizex[map] % sectorsize) || (m_sizey[map] % sectorsize) )
					g_Log.EventError("MAP%d: Map dimensions [%d,%d] must be multiple of sector size (%d is invalid, %d is still effective)\n", map, m_sizex[map], m_sizey[map], sectorsize, m_sectorsize[map]);
				else
					m_sectorsize[map] = sectorsize;
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
			DEBUG_ERR(("Unknown map index %d with file size of %" FMTDWORD " bytes. Please specify the correct size manually\n", index, g_Install.m_Maps[index].GetLength()));
			break;
	}

	return (m_sizex[map] > 0 && m_sizey[map] > 0 && m_sectorsize[map] > 0);
}

void CMapList::Init()
{
	for ( int i = 0; i < MAP_QTY; ++i )
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

bool WritePidFile(int iMode)
{
	LPCTSTR	file = SPHERE_FILE ".pid";
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
			g_Log.Event(LOGL_WARN, "File " SPHERE_FILE ".pid already exists. Secondary launch or unclean shutdown?\n");
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
		g_Log.Event(LOGL_WARN, "Unable to create file " SPHERE_FILE ".pid\n");
		return false;
	}
}

int CEventLog::VEvent(DWORD dwMask, LPCTSTR pszFormat, va_list args)
{
	if ( !pszFormat || (pszFormat[0] == '\0') )
		return 0;

	TemporaryString pszTemp;
	if ( _vsnprintf(pszTemp, SCRIPT_MAX_LINE_LEN, pszFormat, args) == 0 )
	{
		strncpy(pszTemp, pszFormat, SCRIPT_MAX_LINE_LEN);
		pszTemp[SCRIPT_MAX_LINE_LEN - 1] = '\0';
	}

	return EventStr(dwMask, pszTemp);
}

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
	static const LPCTSTR sm_ClockHour[] =
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
	static const LPCTSTR sm_ClockHour[] =
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
 		g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_TWELVE)
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

	snprintf(pTime, EXPRESSION_MAX_KEY_LEN, "%s %s %s", pMinDif, sm_ClockHour[hour], pTail);
	return pTime;
}

size_t FindStrWord( LPCTSTR pTextSearch, LPCTSTR pszKeyWord )
{
	// Find any of the pszKeyWord in the pTextSearch string.
	// Make sure we look for starts of words.

	size_t j = 0;
	for ( size_t i = 0; ; ++i )
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
			++j;
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
}

void Main::onStart()
{
	AbstractSphereThread::onStart();
#if defined(_WIN32) && !defined(_DEBUG) && !defined(__MINGW32__)
	SetExceptionTranslator();
#endif
}

void Main::tick()
{
	// Give the world (CMainTask) a single tick
	EXC_TRY("Tick");

	EXC_SET("shipstimers");
	g_Serv.ShipTimers_Tick();

	EXC_SET("world");
	g_World.OnTick();

	// Process incoming data
	EXC_SET("network-in");
	g_NetworkManager.processAllInput();

	EXC_SET("server");
	g_Serv.OnTick();

	// Push outgoing data
	EXC_SET("network-tick");
	g_NetworkManager.tick();

	EXC_SET("network-out");
	g_NetworkManager.processAllOutput();

	EXC_CATCH;
}

bool Main::shouldExit()
{
	if (g_Serv.m_iExitFlag != 0)
		return true;
	return AbstractSphereThread::shouldExit();
}

Main g_Main;
extern CPingServer g_PingServer;
extern CDataBaseAsyncHelper g_asyncHdb;
#if !defined(_WIN32) || defined(_LIBEV)
	extern LinuxEv g_NetworkEvent;
#endif

//*******************************************************************

int Sphere_InitServer()
{
#ifdef EXCEPTIONS_DEBUG
	const char *m_sClassName = SPHERE_TITLE;
#endif
	EXC_TRY("Init");
	ASSERT(MAX_BUFFER >= sizeof(CEvent));
	ASSERT(sizeof(int) == sizeof(DWORD));	// make this assumption often.
	ASSERT(sizeof(ITEMID_TYPE) == sizeof(DWORD));
	ASSERT(sizeof(WORD) == 2 );
	ASSERT(sizeof(DWORD) == 4 );
	ASSERT(sizeof(NWORD) == 2 );
	ASSERT(sizeof(NDWORD) == 4 );
	ASSERT(sizeof(CUOItemTypeRec) == 37 ); // byte pack working ?

#ifdef _WIN32
	if ( !QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER *>(&llTimeProfileFrequency)) )
		llTimeProfileFrequency = 1000;

	#if !defined(_DEBUG) && !defined(__MINGW32__)
		EXC_SET("setting exception catcher");
		SetExceptionTranslator();
	#endif
#endif

	EXC_SET("loading");
	if ( !g_Serv.Load() )
		return -3;

	WritePidFile(2);

	EXC_SET("load world");
	if ( !g_World.LoadAll() )
		return -8;

	EXC_SET("sockets init");
	if ( !g_Serv.SocketsInit() )
		return -9;

	g_Log.Event(LOGL_EVENT, "\nStartup complete (Items=%" FMTDWORD ", Chars=%" FMTDWORD ", Accounts=%" FMTDWORD ")\n", g_Serv.StatGet(SERV_STAT_ITEMS), g_Serv.StatGet(SERV_STAT_CHARS), g_Serv.StatGet(SERV_STAT_ACCOUNTS));
#ifdef _WIN32
	g_Log.Event(LOGL_EVENT, "Use '?' to view available console commands\n\n");
#else
	g_Log.Event(LOGL_EVENT, "Use '?' to view available console commands or Ctrl+C to exit\n\n");
#endif
	if ( !g_Accounts.Account_GetCount() )
		g_Log.Event(LOGL_WARN, "The server has no accounts. To create admin account use:\n  ACCOUNT ADD [login] [password]\n  ACCOUNT [login] PLEVEL 7\n\n");

	// Trigger server start
	EXC_SET("finalizing");
	g_Serv.SetServerMode(SERVMODE_Run);

	g_Serv.r_Call("f_onserver_start", &g_Serv);
	return g_Serv.m_iExitFlag;
	EXC_CATCH;

	return -10;
}

void Sphere_ExitServer()
{
	// Trigger server quit
	g_Serv.r_Call("f_onserver_exit", &g_Serv);

	g_Serv.SetServerMode(SERVMODE_Exiting);

	g_NetworkManager.stop();
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

	g_Log.Event(LOGL_FATAL, "Server terminated: %s (code %d)\n", Reason, g_Serv.m_iExitFlag);
	g_Log.Close();
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

void CServer::ShipTimers_Add(CItemShip *pShip)
{
	ADDTOCALLSTACK("CServer::ShipTimers_Add");
	if ( !pShip )
		return;
	m_ShipTimers.push_back(pShip);
}

void CServer::ShipTimers_Delete(CItemShip *pShip)
{
	ADDTOCALLSTACK("CServer::ShipTimers_Delete");
	std::vector<CItemShip *>::iterator it;
	for ( it = m_ShipTimers.begin(); it != m_ShipTimers.end(); ) 
	{
		CItemShip *pShipSearch = *it;
		if ( pShipSearch == pShip )
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
#ifdef EXCEPTIONS_DEBUG
	const char *m_sClassName = SPHERE_TITLE;
#endif
	// Just make sure the main loop is alive every so often.
	// This should be the parent thread. try to restart it if it is not.
	while ( ! g_Serv.m_iExitFlag )
	{
		EXC_TRY("MainMonitorLoop");
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

#ifndef _DEBUG
		EXC_SET("Check Stuck");
		if (g_Main.checkStuck() == true)
			g_Log.Event(LOGL_CRIT, "'%s' thread hang, restarting...\n", g_Main.getName());
#endif
		EXC_CATCH;
	}

}

#ifdef _WIN32
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	NTWindow_Init(hInstance, nShowCmd);
	int iRet = Sphere_MainEntryPoint();
	NTWindow_Exit();
	TerminateProcess(GetCurrentProcess(), iRet);
	return iRet;
}

int Sphere_MainEntryPoint()
#else
int _cdecl main( int argc, char * argv[] )
#endif
{
#ifdef EXCEPTIONS_DEBUG
	const char *m_sClassName = SPHERE_TITLE;
#endif
	EXC_TRY("Main");

#ifndef _WIN32
	// Initialize nonblocking IO and disable readline on linux
	g_UnixTerminal.prepare();
#endif

	g_Serv.m_iExitFlag = Sphere_InitServer();
	if ( ! g_Serv.m_iExitFlag )
	{
		WritePidFile(0);

		// Start the ping server, this can only be ran in a separate thread
		if ( IsSetEF( EF_UsePingServer ) )
			g_PingServer.start();
		
#if !defined(_WIN32) || defined(_LIBEV)
		if ( g_Cfg.m_fUseAsyncNetwork != 0 )
			g_NetworkEvent.start();
#endif

		g_NetworkManager.start();
		if ( g_Cfg.m_iFreezeRestartTime > 0 )
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
	WritePidFile(1);

	EXC_CATCH;
	return g_Serv.m_iExitFlag;
}

#define ADD(a,b) const char * a::m_sClassName = b
#include "../tables/classnames.tbl"
#undef ADD
