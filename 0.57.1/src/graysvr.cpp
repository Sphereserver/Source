#include "graysvr.h"
#include "common/version.h"
#include "network/network.h"
#ifdef _WIN32
#include <direct.h>		// _chdir
#endif
#include "muls/Tiledata.h"

#if !defined(pid_t)
#define pid_t int
#endif

#ifndef _WIN32
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#endif

#include "SimpleCommand.h"

CMapList::CMapList()
{
	memset(m_mapsinitalized, false, sizeof(m_mapsinitalized));
	memset(m_sizex, 0, sizeof(m_sizex));
	memset(m_sizey, 0, sizeof(m_sizey));
	memset(m_maps, true, sizeof(m_maps));
	memset(m_mapnum, -1, sizeof(m_mapnum));
	memset(m_mapid, -1, sizeof(m_mapid));
	memset(m_sectorsize, 0, sizeof(m_sectorsize));

	Load(0, 0x1800, 0x1000, 64, 0, 0);	// #0 map0.mul
	Load(1, 0x1800, 0x1000, 64, 0, 1);	// #1 map0.mul
	Load(2, 0x900, 0x640, 64, 2, 2);	// #2 map2.mul
	Load(3, 0xa00, 0x800, 64, 3, 3);	// #3 map3.mul
	Load(4, 0x5a8, 0x5a8, 8, 4, 4);		// #4 map4.mul
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
		g_Log.Error("Invalid map #%d could be initialized.\n", map);
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
					g_Log.Error("MAP%d: X coord must be multiple of 8 (%d is invalid, %d is still effective).\n", map, maxx, m_sizex[map]);
				}
				else m_sizex[map] = maxx;
			}
			if ( maxy )
			{
				if (( maxy < 8 ) || ( maxy % 8 ))
				{
					g_Log.Error("MAP%d: Y coord must be multiple of 8 (%d is invalid, %d is still effective).\n", map, maxy, m_sizey[map]);
				}
				else m_sizey[map] = maxy;
			}
			if ( sectorsize > 0 )
			{
				if (( sectorsize < 8 ) || ( sectorsize % 8 ))
				{
					g_Log.Error("MAP%d: Sector size must be multiple of 8 (%d is invalid, %d is still effective).\n", map, sectorsize, m_sectorsize[map]);
				}
				else if (( m_sizex[map]%sectorsize ) || ( m_sizey[map]%sectorsize ))
				{
					g_Log.Error("MAP%d: Map dimensions [%d,%d] must be multiple of sector size (%d is invalid, %d is still effective).\n", map, m_sizex[map], m_sizey[map], sectorsize, m_sectorsize[map]);
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
}

bool CMapList::IsMapSupported(int map)
{
	if (( map < 0 ) || ( map > 255 )) return false;
	return( m_maps[map] );
}

int CMapList::GetX(int map)
{
	if (( map < 0 ) || ( map > 255 )) return 0;
	return m_sizex[map];
}
int CMapList::GetY(int map)
{
	if (( map < 0 ) || ( map > 255 )) return 0;
	return m_sizey[map];
}
int CMapList::GetCenterX(int map)
{
	if (( map < 0 ) || ( map > 255 )) return 0;
	return (m_sizex[map]/2);
}
int CMapList::GetCenterY(int map)
{
	if (( map < 0 ) || ( map > 255 )) return 0;
	return (m_sizey[map]/2);
}
int CMapList::GetSectorSize(int map)
{
	if (( map < 0 ) || ( map > 255 )) return 0;
	return m_sectorsize[map];
}
int CMapList::GetSectorCols(int map)
{
	if (( map < 0 ) || ( map > 255 )) return 0;
	return (m_sizex[map] / GetSectorSize(map));
}
int CMapList::GetSectorRows(int map)
{
	if (( map < 0 ) || ( map > 255 )) return 0;
	return (m_sizey[map] / GetSectorSize(map));
}
int CMapList::GetSectorQty(int map)
{
	return ( GetSectorCols(map) * GetSectorRows(map) );
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

LONGLONG llTimeProfileFrequency = 1000;	// time profiler

// game servers stuff.
CWorld		g_World;	// the world. (we save this stuff)
CServer		g_Serv;	// current state stuff not saved.
CResource	g_Cfg;
CGrayInstall g_Install;
CExpression g_Exp;	// Global script variables.
CAccounts	g_Accounts;	// All the player accounts. name sorted CAccount
CGStringList	g_AutoComplete;	// auto-complete list
TScriptProfiler g_profiler;		// script profiler
CMapList	g_MapList;			// global maps information
Tiledata	*g_Tiledata;

//	Tasks

DIR_TYPE GetDirStr(LPCTSTR pszDir, CChar *pChar = NULL)
{
	char iDir2, iDir = toupper(pszDir[0]);

	switch ( iDir )
	{
	case 'B': // back
		if ( !strcmpi(pszDir, "back") && pChar )
		{
			switch ( pChar->m_dirFace )
			{
			case 0: return DIR_S;
			case 1: return DIR_SW;
			case 2: return DIR_W;
			case 3: return DIR_NW;
			case 4: return DIR_N;
			case 5: return DIR_NE;
			case 6: return DIR_E;
			default: return DIR_SE;
			}
		}
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

LPCTSTR GetTimeMinDesc(int minutes, bool numeric)
{
	TEMPSTRING(pTime);
	int minute = minutes % 60;
	int hour = ( minutes / 60 ) % 24;

	if ( numeric )
		sprintf(pTime, "%02d:%02d", hour, minute);
	else
	{
		LPCTSTR pMinDif;
		if ( minute <= 14 )
			pMinDif = "";
		else if ( ( minute >= 15 ) && ( minute <= 30 ) ) 
			pMinDif = "a quarter past";
		else if ( ( minute >= 30 ) && ( minute <= 45 ) ) 
			pMinDif = "half past";
		else
		{
			pMinDif = "a quarter till";
			hour = ( hour + 1 ) % 24;
		}

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

		LPCTSTR pTail;
		if ( hour == 0 || hour==12 )
			pTail = "";
		else if ( hour > 12 )
		{
			hour -= 12;
			if ((hour>=1)&&(hour<6))
				pTail = " o'clock in the afternoon";
			else if ((hour>=6)&&(hour<9))
				pTail = " o'clock in the evening.";
			else
				pTail = " o'clock at night";
		}
		else
		{
			pTail = " o'clock in the morning";
		}

		sprintf( pTime, "%s %s%s.", pMinDif, sm_ClockHour[hour], pTail );
	}
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
			if ( pTextSearch[i]=='\0' || isspace(pTextSearch[i]) )
				return i;
			return 0;
		}
		if ( pTextSearch[i] == '\0' )
			return 0;
		if ( !j && i )
		{
			if ( isalpha( pTextSearch[i-1] ))	// not start of word ?
				continue;
		}
		if ( toupper( pTextSearch[i] ) == toupper( pszKeyWord[j] ))
			j++;
		else
			j=0;
	}
}

//*******************************************************************
// -CMainTask

class CMainTask : public CThread
{
public:
	static const char *m_sClassName;
	static THREAD_ENTRY_RET _cdecl EntryProc( void * lpThreadParameter )
	{
		CThread *thread = (CThread *)lpThreadParameter;

		thread->OnCreate();
		long l = 0;
		while ( !g_Serv.m_iExitFlag )
		{
			thread->OnTick(100);

			l++;
			EXC_TRY("Tick");

	#ifndef _WIN32
			// Do a select operation on the stdin handle and check
			// if there is any input waiting.
			EXC_SET("unix");
			fd_set consoleFds;
			FD_ZERO(&consoleFds);
			FD_SET(STDIN_FILENO, &consoleFds);

			timeval tvTimeout;
			tvTimeout.tv_sec = 0;
			tvTimeout.tv_usec = 1;

			if( select(1, &consoleFds, 0, 0, &tvTimeout) )
			{
				int c = fgetc(stdin);
				if ( g_Serv.OnConsoleKey(g_Serv.m_sConsoleText, c, false) == 2 )
					g_Serv.m_fConsoleTextReadyFlag = true;
			}
	#endif
			if ( g_Serv.m_fConsoleTextReadyFlag )
			{
				EXC_SET("console input");
				CGString sText = g_Serv.m_sConsoleText;
				g_Serv.m_sConsoleText.Empty();
				g_Serv.m_fConsoleTextReadyFlag = false;
				g_Serv.OnConsoleCmd(sText, &g_Serv);
			}

			if ( g_Serv.m_iModeCode != SERVMODE_Run )
				continue;

			EXC_SET("world tick");
			g_World.OnTick();

			// these are enough to be once per second
			if (( l % 100 )== 0 )	
			{
				EXC_SET("general");
				g_Cfg.OnTick(false);
				g_Serv.m_hdb.OnTick();
			}

			EXC_CATCH;
		}
		thread->OnClose();
	}

	virtual void Start()
	{
		m_name = "Main Task";
		CThread::Create(EntryProc);	
	}
};
const char *CMainTask::m_sClassName = "CMainTask";

//*******************************************************************
// NPC AI thread

class CAITask : public CThread
{
public:
	static const char *m_sClassName;
	static THREAD_ENTRY_RET _cdecl EntryProc(void *lpThreadParameter)
	{
		CThread *thread = (CThread *)lpThreadParameter;

		thread->OnCreate();
		while ( !g_Serv.m_iExitFlag )
		{
			thread->OnTick(50);

			if ( g_Serv.m_iModeCode != SERVMODE_Run )
				continue;

			EXC_TRY("EntryProc");

			CSector	*pSector;
			for ( int m = 0; m < 256; m++ )
			{
				if ( !g_MapList.m_maps[m] )
					continue;

				for ( long l = 0; l < g_MapList.GetSectorQty(m); l++ )
				{
					pSector = g_World.GetSector(m, l);
					if ( !pSector || pSector->IsSleeping() )
						continue;

					CChar	*pChar, *pCharNext;

					pSector->m_Chars_Active.Lock();
					for ( pChar = static_cast <CChar*>(pSector->m_Chars_Active.GetHead()) ; pChar ; pChar = pCharNext )
					{
						pCharNext = pChar->GetNext();
						if ( pChar->m_pNPC )
						{
							pChar->Lock();
							if ( g_Cfg.m_iNpcAi&NPC_AI_PATH )
							{
								pChar->NPC_Pathfinding();
							}
							if ( g_Cfg.m_iNpcAi&NPC_AI_FOOD )
							{
								pChar->NPC_Food();
							}
							if ( g_Cfg.m_iNpcAi&NPC_AI_EXTRA )
							{
								pChar->NPC_AI();
							}
							pChar->Release();
						}
						pChar = pCharNext;
					}
					pSector->m_Chars_Active.Release();
					Sleep(0);
				}
			}
			EXC_CATCH;

		} // while
		thread->OnClose();
	}

	virtual void Start()
	{
		m_name = "NPC Task";
		CThread::Create(EntryProc);	
	}
};
const char *CAITask::m_sClassName = "CAITask";

//*******************************************************************
// World save thread

class CTaskBackground : public CThread
{
public:
	static const char *m_sClassName;

	static THREAD_ENTRY_RET _cdecl EntryProc(void *lpThreadParameter)
	{
		CThread *thread = (CThread *)lpThreadParameter;

		thread->OnCreate();
		while ( !g_Serv.m_iExitFlag )
		{
			thread->OnTick(1000);		// respond once per second

			if ( g_Serv.m_iModeCode != SERVMODE_Run )
				continue;

			EXC_TRY("EntryProc");

			//	start a world save:
			if ( g_World.m_timeSave <= g_World.GetCurrentTime() )
			{
				g_World.m_timeSave = g_World.GetCurrentTime() + g_Cfg.m_iSavePeriod;

				int bSavedOk = -1;
				EXC_TRYSUB("Save");
				enum TRIGRET_TYPE tr;

				//	save time: 0-9 clients - instant, 100 clients = 10ms*(6144+2) ~61.5s
				long SleepPeriod = g_Serv.StatGet(SERV_STAT_CLIENTS) / 10;

				if ( !g_Serv.r_Call("f_onserver_save", &g_Serv, NULL, NULL, &tr) || ( tr != TRIGRET_RET_TRUE ))
				{
					g_World.m_iSaveStage = -1;
					while ( true )
					{
						thread->CriticalStart();
						if ( !g_World.SaveStage() )
						{
							if ( g_World.m_iSaveStage != INT_MAX )
								bSavedOk = 0;
							break;
						}
						//	immidiate save on shutdown, no clients give no slices at all - instant da good :)
						if ( !SleepPeriod || g_Serv.m_iExitFlag )
							continue;
						thread->CriticalEnd();
						thread->OnTick(SleepPeriod);
					}
				}
				bSavedOk = 1;

				EXC_CATCHSUB(m_sClassName);
				g_Serv.r_Call(( bSavedOk == 1 ) ? "f_onserver_save_ok" : "f_onserver_save_fail", &g_Serv, NULL);
			}

			EXC_CATCH;
		} // while
		thread->OnClose();
	}

	virtual void Start()
	{
		m_name = "Background Task";
		CThread::Create(EntryProc);
	}
};
const char *CTaskBackground::m_sClassName = "CTaskBackground";

//*******************************************************************
// Debug thread

#ifdef DEBUGTASK
class CTaskDebug : public CThread
{
public:
	static const char *m_sClassName;

	static THREAD_ENTRY_RET _cdecl EntryProc(void *lpThreadParameter)
	{
		CThread *thread = (CThread *)lpThreadParameter;

		thread->OnCreate();
		while ( !g_Serv.m_iExitFlag )
		{
			thread->OnTick(1000);		// respond once per second

			if ( g_Serv.m_iModeCode != SERVMODE_Run )
				continue;

			EXC_TRY("EntryProc");

			int sectors = 0;
			int sleeping = 0;
			for ( int m = 0; m < 256; m++ )
			{
				if ( g_MapList.IsMapSupported(m) )
				{
					for ( int s = 0; s < g_MapList.GetSectorQty(m); s++ )
					{
						CSector *pSector = g_World.GetSector(m, s);
						sectors++;
						if ( pSector->IsSleeping() )
							sleeping += 1;
					}
				}
			}
			g_Log.Debug("Sleeping %d sectors of %d (%d%%)\n", sleeping, sectors, (sleeping * 100)/sectors);

			EXC_CATCH;
		} // while
		thread->OnClose();
	}

	virtual void Start()
	{
		m_name = "Debug Task";
		CThread::Create(EntryProc);
	}
};
const char *CTaskDebug::m_sClassName = "CTaskDebug";
#endif

//*******************************************************************

int Sphere_InitServer( int argc, char *argv[] )
{
	int		i;
	const char *m_sClassName = "Sphere";
	EXC_TRY("Init");
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
#endif // _WIN32

	if ( argc > 1 )
	{
		if ( !strcmp(argv[1], "@") )
		{
			SimpleCommand cmd(argc, argv);
			return cmd.run();
		}
	}

	EXC_SET("crypt keys");
	CCrypt::client_keys.push_back(new CCryptClientKey(0x500020, 0x2EE3ADDD, 0xA265227F, ENC_TFISH));	// 5.0.2
	CCrypt::client_keys.push_back(new CCryptClientKey(0x500010, 0x2EABA7ED, 0xA2417E7F, ENC_TFISH));	// 5.0.1
	CCrypt::client_keys.push_back(new CCryptClientKey(0x500000, 0x2E93A5FD, 0xA25D527F, ENC_TFISH));	// 5.0.0

	EXC_SET("loading");
	if ( !g_Serv.Load() )
		return -3;

	if ( argc > 1 )
	{
		EXC_SET("cmdline");
		if ( !g_Serv.CommandLine(argc, argv) )
			return -1;
	}

	EXC_SET("sockets init");
	if ( !g_Serv.SocketsInit() )
		return -9;
	EXC_SET("load world");
	if ( !g_World.LoadAll() )
		return -8;

	//	load auto-complete dictionary
	EXC_SET("auto-complete");
	FILE	*f = fopen(SPHERE_FILE ".dic", "rt");
	if ( f )
	{
		int		i = 0;
		TEMPSTRING(z);
		while ( !feof(f) )
		{
			z[0] = 0;
			fgets(z, SCRIPT_MAX_LINE_LEN-1, f);
			if ( *z )
			{
				char *c = strchr(z, '\r');
				if ( c ) *c = 0;
				c = strchr(z, '\n');
				if ( c ) *c = 0;
				if ( *z )
				{
					i++;
					g_AutoComplete.AddTail(z);
				}
			}
		}
		fclose(f);
		g_Log.Init("Auto-complete dictionary loaded (contains %d words).\n", i);
	}

	g_Serv.SetServerMode(SERVMODE_Run);	// ready to go.
	
	// Display EF/OF Flags
	g_Cfg.PrintEFOFFlags();

	EXC_SET("finilizing");
	g_Log.Init(g_Serv.GetStatusString(0x24));
	g_Log.Init("Startup complete. items=%d, chars=%d\n", g_Serv.StatGet(SERV_STAT_ITEMS), g_Serv.StatGet(SERV_STAT_CHARS));
	g_Log.Init("Press '?' for console commands\n");

	// restock vendors
	for ( int m = 0; m < 256; m++ )
	{
		if ( !g_MapList.m_maps[m] )
			continue;
		for ( int s = 0; s < g_MapList.GetSectorQty(m); s++ )
		{
			CSector	*pSector = g_World.GetSector(m, s);
			if ( pSector )
				pSector->Restock();
		}
	}

	// Trigger server start
	g_Serv.r_Call("f_onserver_start", &g_Serv, NULL);
	return 0;

	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.Debug("cmdline argc=%d starting with %x (argv1='%s')\n", argc, argv, ( argc > 2 ) ? argv[1] : "");
	EXC_DEBUG_END;
	return -10;
}

void Sphere_ExitServer()
{
	// Trigger server quit
	g_Serv.r_Call("f_onserver_exit", &g_Serv, NULL);

	g_Serv.SetServerMode(SERVMODE_Exiting);

	vector<CThread *>::iterator it;
	for ( it = g_Threads.begin(); it != g_Threads.end(); it++ )
	{
		(*it)->WaitForClose();
	}

	if ( !g_Serv.m_iExitFlag )
		g_Log.Event(LOGM_INIT|LOGL_FATAL, "Server shutdown (code %d) complete!\n\n", g_Serv.m_iExitFlag);
	else if ( g_Serv.m_iExitFlag < 100 )
		g_Log.Event(LOGM_INIT|LOGL_FATAL, "Server terminated by error %d!\n\n", g_Serv.m_iExitFlag);
	else
		g_Log.Event(LOGM_INIT|LOGL_FATAL, "Server exited by code %d!\n\n", g_Serv.m_iExitFlag);

	g_Log.Close();
}

//******************************************************

#ifndef _WIN32
extern void setNonBlockingIo();
#endif

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int _cdecl main( int argc, char * argv[] )
#endif
{

#ifdef _WIN32
	TCHAR	*argv[32];
	argv[0] = NULL;
	int argc = Str_ParseCmds(lpCmdLine, &argv[1], COUNTOF(argv)-1, " \t") + 1;

	// We need to find out what the server name is....look it up in the .ini file
	if ( !g_Cfg.LoadIni(true) )
	{
		// Try to determine the name and path of this application.
		char szPath[_MAX_PATH];
		char *z;

		GetModuleFileName(NULL, szPath, sizeof(szPath));
		
		if ( !szPath[0] )
			return -2;

		z = &szPath[strlen(szPath) - 1];
		while (( z > szPath ) && ( *z != '/' ) && ( *z != '\\' ))
			z--;

		*z = 0;
		if ( z <= szPath )
			return -2;
		_chdir(szPath);

		if ( !g_Cfg.LoadIni(false) )
			return -2;
	}

	NTWindow_Init(hInstance, lpCmdLine, nCmdShow);
#else

	// Initialize nonblocking IO and disable readline on linux
	setNonBlockingIo();

#endif

	g_Serv.m_iExitFlag = Sphere_InitServer(argc, argv);
	if ( !g_Serv.m_iExitFlag )
	{
		//	create working threads
		vector<CThread *>::iterator it;

		g_Network = new NetworkIn();
		g_Sender = new NetworkOut();

		g_Threads.push_back(new CAITask);
		g_Threads.push_back(g_Network);
		g_Threads.push_back(g_Sender);
		g_Threads.push_back(new CMainTask);
		g_Threads.push_back(new CTaskBackground);
#ifdef DEBUGTASK
		g_Threads.push_back(new CTaskDebug);
#endif

		for ( it = g_Threads.begin(); it != g_Threads.end(); it++ )
		{
			(*it)->Start();
		}

		//	execute the monitoring
		while ( !g_Serv.m_iExitFlag )
		{
			try
			{
#ifdef _WIN32
				NTWindow_OnTick(30 * 1000);
#else
				Sleep(30 * 1000);
#endif
				//	actualy this occures more often than once per 30 seconds if console input is coming
				//	thus we should ensure that threads are not thought of being frozen if they are not

				if ( g_Serv.IsLoading() || g_Serv.m_fConsoleTextReadyFlag )
					continue;

#ifndef _DEBUG
				vector<CThread *>::iterator it;

				for ( it = g_Threads.begin(); it != g_Threads.end(); it++ )
				{
					(*it)->CheckStuck();
				}
#endif
			}
			catch (...)
			{
				g_Log.Error("Critical exception in Main Monitor loop.\n");
			}
		}
	}

	Sphere_ExitServer();
#ifdef _WIN32
	NTWindow_Exit();
#endif
	return g_Serv.m_iExitFlag;
}

#include "tables/classnames.tbl"
