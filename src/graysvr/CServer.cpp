#include "graysvr.h"	// predef header
#include "../network/network.h"
#include "../graysvr/CPingServer.h"

#ifdef _WIN32
	#include "CNTService.h"
	#include "../common/crashdump/crashdump.h"
#else
	#include "CUnixTerminal.h"
	#ifdef _LIBEV
		extern LinuxEv g_NetworkEvent;
	#endif
#endif

CServer::CServer() : CServerDef(SPHERE_TITLE, CSocketAddressIP(SOCKET_LOCAL_ADDRESS))
{
	SetServerMode(SERVMODE_Loading);
	m_iExitFlag = 0;

	m_fResyncPause = false;
	m_fResyncMultiRegions = false;
	m_fResyncRequested = NULL;

	m_iAdminClients = 0;
	m_fConsoleTextReadyFlag = false;

	m_timeShutdown.Init();

	memset(m_PacketFilter, 0, sizeof(m_PacketFilter));
	memset(m_OutPacketFilter, 0, sizeof(m_OutPacketFilter));
}

void CServer::SetServerMode(SERVMODE_TYPE mode)
{
	ADDTOCALLSTACK("CServer::SetServerMode");
	m_iModeCode = mode;
#ifdef _WIN32
	NTWindow_SetWindowTitle();
#endif
}

bool CServer::IsValidBusy() const
{
	// We might appear to be stopped but it's really ok?
	switch ( m_iModeCode )
	{
		case SERVMODE_Saving:
			return g_World.IsSaving();
		case SERVMODE_Loading:
		case SERVMODE_RestockAll:	// these may look stuck but are not
			return true;
		default:
			return false;
	}
}

void CServer::SetExitFlag(int iFlag)
{
	ADDTOCALLSTACK("CServer::SetExitFlag");
	if ( m_iExitFlag )
		return;
	m_iExitFlag = iFlag;
}

void CServer::Shutdown(INT64 iMinutes)
{
	ADDTOCALLSTACK("CServer::Shutdown");
	if ( iMinutes == 0 )
	{
		if ( m_timeShutdown.IsTimeValid() )
		{
			m_timeShutdown.Init();
			g_World.Broadcast(g_Cfg.GetDefaultMsg(DEFMSG_MSG_SERV_SHUTDOWN_CANCEL));
		}
		return;
	}

	if ( iMinutes < 0 )
		iMinutes = g_World.GetTimeDiff(m_timeShutdown) / (60 * TICK_PER_SEC);
	else
		m_timeShutdown = CServTime::GetCurrentTime() + (iMinutes * 60 * TICK_PER_SEC);

	g_World.Broadcastf(g_Cfg.GetDefaultMsg(DEFMSG_MSG_SERV_SHUTDOWN), iMinutes);
}

bool CServer::SocketsInit()
{
	ADDTOCALLSTACK("CServer::SocketsInit");
	// Initialize socket
#ifdef _WIN32
	if ( !m_SocketMain.IsOpen() )
	{
		WSADATA wsaData;
		int iRetWSA = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if ( iRetWSA != 0 )
		{
			g_Log.Event(LOGL_FATAL|LOGM_INIT, "Unable to initialize WinSock (code %d)\n", iRetWSA);
			return false;
		}
		else if ( (LOBYTE(wsaData.wVersion) != 2) || (HIBYTE(wsaData.wVersion) != 2) )
		{
			g_Log.Event(LOGL_FATAL|LOGM_INIT, "WinSock 2.2 is not supported\n", iRetWSA);
			WSACleanup();
			return false;
		}
	}
#endif

	if ( !m_SocketMain.Create() )
	{
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "Unable to create listen socket\n");
		return false;
	}

	linger lVal;
	lVal.l_onoff = 0;
	lVal.l_linger = 10;
	if ( m_SocketMain.SetSockOpt(SO_LINGER, reinterpret_cast<const void *>(&lVal), sizeof(lVal)) == SOCKET_ERROR )
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "Unable to set listen socket option SO_LINGER\n");
	if ( m_SocketMain.SetNonBlocking() == SOCKET_ERROR )
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "Unable to set listen socket non-blocking mode\n");
#ifndef _WIN32
	int iEnable = 1;
	if ( m_SocketMain.SetSockOpt(SO_REUSEADDR, reinterpret_cast<const void *>(&iEnable), sizeof(iEnable)) == SOCKET_ERROR )
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "Unable to set listen socket option SO_REUSEADDR\n");
#endif

	// Bind socket to specific IP/port
	CSocketAddress SockAddr = m_ip;
	if ( m_SocketMain.Bind(SockAddr) == SOCKET_ERROR )
	{
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "Unable to bind listen socket %s port %hu\n", SockAddr.GetAddrStr(), SockAddr.GetPort());
		return false;
	}
	m_SocketMain.Listen();

#if !defined(_WIN32) && defined(LIBEV_REGISTERMAIN)
	if ( g_Cfg.m_fUseAsyncNetwork != 0 )
		g_NetworkEvent.registerMainsocket();
#endif

	TCHAR szName[_MAX_PATH];
	struct hostent *pHost = NULL;

	int iRet = gethostname(szName, sizeof(szName));
	if ( iRet )
		strncpy(szName, m_ip.GetAddrStr(), sizeof(szName) - 1);
	else
	{
		pHost = gethostbyname(szName);
		if ( pHost && pHost->h_addr && pHost->h_name )
			strncpy(szName, pHost->h_name, sizeof(szName) - 1);
	}

	g_Log.Event(LOGM_INIT, "\nServer started on hostname '%s'\n", szName);
	if ( !iRet && pHost && pHost->h_addr )
	{
		for ( size_t i = 0; pHost->h_addr_list[i] != NULL; ++i )
		{
			CSocketAddressIP ip;
			ip.SetAddrIP(*reinterpret_cast<DWORD *>(pHost->h_addr_list[i]));	// 0.1.2.3
			if ( !m_ip.IsLocalAddr() && !m_ip.IsSameIP(ip) )
				continue;
			g_Log.Event(LOGM_INIT, "Monitoring local IP %s:%d (TCP) - Main server\n", ip.GetAddrStr(), m_ip.GetPort());
			if ( IsSetEF(EF_UsePingServer) )
				g_Log.Event(LOGM_INIT, "Monitoring local IP %s:%d (UDP) - Ping server\n", ip.GetAddrStr(), PINGSERVER_PORT);
		}
	}
	if ( GetPublicIP() )
	{
		g_Log.Event(LOGM_INIT, "Monitoring public IP %s:%d (TCP) - Main server\n", m_ip.GetAddrStr(), m_ip.GetPort());
		if ( IsSetEF(EF_UsePingServer) )
			g_Log.Event(LOGM_INIT, "Monitoring public IP %s:%d (UDP) - Ping server\n", m_ip.GetAddrStr(), PINGSERVER_PORT);
	}
	return true;
}

void CServer::SocketsClose()
{
	ADDTOCALLSTACK("CServer::SocketsClose");
#if !defined(_WIN32) && defined(LIBEV_REGISTERMAIN)
	if ( g_Cfg.m_fUseAsyncNetwork != 0 )
		g_NetworkEvent.unregisterMainsocket();
#endif
	m_SocketMain.Close();
}

bool CServer::GetPublicIP()
{
	ADDTOCALLSTACK("CServer::GetPublicIP");
	// REST API used to get server public IP and set it as ServIP automatically at server startup
	// Should be used when local IP is behind NAT router/firewall and can't be reached directly
	if ( g_Serv.m_sRestAPIPublicIP.IsEmpty() )
		return false;

	// Parse URL into domain/path
	TCHAR szURL[_MAX_PATH];
	strncpy(szURL, g_Serv.m_sRestAPIPublicIP, sizeof(szURL) - 1);
	szURL[sizeof(szURL) - 1] = '\0';

	TCHAR *pszPath = strchr(szURL, '/');
	TCHAR *pszDomain = Str_GetTemp();
	strncpy(pszDomain, szURL, pszPath ? pszPath - szURL : sizeof(szURL) - 1);
	pszDomain[pszPath ? pszPath - szURL : sizeof(szURL) - 1] = '\0';

	// Create socket
	CSocketAddress sockAddr;
	sockAddr.SetHostStr(pszDomain);
	sockAddr.SetPort(80);

	CGSocket sock;
	sock.Create();

	if ( sock.Connect(sockAddr) == SOCKET_ERROR )
	{
		DEBUG_ERR(("Failed to get server public IP: can't connect on REST API 'http://%s:%hu' (TCP)\n", pszDomain, sockAddr.GetPort()));
		sock.Close();
		return false;
	}

	// Send HTTP request
	TCHAR *pszHeader = Str_GetTemp();
	sprintf(pszHeader, "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: " SPHERE_TITLE " V" SPHERE_VERSION "\r\nConnection: Close\r\n\r\n", pszPath ? pszPath : "/", pszDomain);
	if ( sock.Send(pszHeader, strlen(pszHeader)) == SOCKET_ERROR )
	{
		sock.Close();
		return false;
	}

	// Receive HTTP response
	TCHAR *pszBuffer = Str_GetTemp();
	if ( sock.Receive(pszBuffer, NETWORK_BUFFERSIZE) == SOCKET_ERROR )
	{
		sock.Close();
		return false;
	}

	sock.Close();

	// Skip HTTP header
	pszBuffer = strstr(pszBuffer, "\r\n\r\n");

	// Remove '\r' and '\n' chars
	if ( pszBuffer )
	{
		for ( size_t i = 0; i < strlen(pszBuffer); ++i )
		{
			if ( (pszBuffer[i] == '\r') || (pszBuffer[i] == '\n') )
			{
				for ( size_t j = i; j < strlen(pszBuffer); ++j )
					pszBuffer[j] = pszBuffer[j + 1];
				--i;
			}
		}
	}

	// Check if it's a valid IP address
	if ( Str_RegExMatch("^([0-9]{1,3}).([0-9]{1,3}).([0-9]{1,3}).([0-9]{1,3})$", pszBuffer) == MATCH_VALID )
	{
		m_ip.SetAddrStr(pszBuffer);
		return true;
	}
	DEBUG_ERR(("Failed to get server public IP: REST API 'http://%s' returned a non-IP value. Please check RestAPIPublicIP setting on " SPHERE_FILE ".ini\n", szURL));
	return false;
}

void CServer::OnTick()
{
	ADDTOCALLSTACK("CServer::OnTick");
	EXC_TRY("Tick");

#ifndef _WIN32
	if ( g_UnixTerminal.isReady() )
	{
		if ( OnConsoleKey(m_sConsoleText, g_UnixTerminal.read(), false) == 2 )
			m_fConsoleTextReadyFlag = true;
	}
#endif

	EXC_SET("ConsoleInput");
	if ( m_fConsoleTextReadyFlag )
	{
		EXC_SET("console input");
		CGString sText = m_sConsoleText;	// make a copy
		m_sConsoleText.Empty();				// done using this
		m_fConsoleTextReadyFlag = false;	// ready to use again
		OnConsoleCmd(sText, this);
	}

	EXC_SET("ResyncCommand");
	if ( m_fResyncRequested )
	{
		if ( m_fResyncPause )
			SetResyncPause(false, m_fResyncRequested, true);
		else
		{
			SetResyncPause(true, m_fResyncRequested);
			SetResyncPause(false, m_fResyncRequested);
		}
		m_fResyncRequested = NULL;
	}

	EXC_SET("SetTime");
	SetValidTime();

	ProfileTask overheadTask(PROFILE_OVERHEAD);

	if ( m_timeShutdown.IsTimeValid() )
	{
		EXC_SET("shutdown");
		if ( g_World.GetTimeDiff(m_timeShutdown) <= 0 )
			SetExitFlag(2);
	}

	EXC_SET("generic");
	g_Cfg.OnTick(false);
	m_hdb.OnTick();
	EXC_CATCH;
}

bool CServer::Load()
{
	EXC_TRY("Load");

	EXC_SET("loading ini");
	if ( !g_Cfg.LoadIni(false) )
		return false;

	EXC_SET("log write");
	g_Log.WriteString("\n");

#if defined(__GITREVISION__) && defined(__GITHASH__)
	g_Log.Event(LOGL_EVENT, "%s\nCompiled: %s (%s) [build %d / Git hash %s]\n\n", g_szServerDescription, g_szServerBuildDate, g_szServerBuildTime, __GITREVISION__, __GITHASH__);
#else
	g_Log.Event(LOGL_EVENT, "%s\nCompiled: %s (%s)\n\n", g_szServerDescription, g_szServerBuildDate, g_szServerBuildTime);
#endif

#ifdef _NIGHTLYBUILD
	g_Log.EventWarn("\n"
					"This is a NIGHTLY build of SphereServer. Nightly builds are compiled automatically\n"
					"from the source code with the latest updates, but might contain errors or might be\n"
					"unstable. Take caution when using it on live servers.\n\n");
#endif
#ifdef _DEBUG
	g_Log.EventWarn("\n"
					"This is a DEBUG build of SphereServer. Debug builds are compiled manually by developers\n"
					"for testing purposes. It have extra debug behaviors that are useful for development but\n"
					"will decrease server performance. Do not use it on live servers.\n\n");
#endif
#ifdef _PRIVATEBUILD
	g_Log.EventWarn("\n"
					"This is a CUSTOM build of SphereServer. Custom builds are non-official builds, which\n"
					"have custom changes on source code not verified by official Sphere development team.\n"
					"Use it at your own risk.\n\n");
#endif

#ifndef _WIN32
	EXC_SET("setting signals");
	SetSignals(g_Cfg.m_fSecure);
#endif

	EXC_SET("loading scripts");
	TriglistInit();
	if ( !g_Cfg.Load(false) )
		return false;

	EXC_SET("finalizing");
#ifdef _WIN32
	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, SPHERE_TITLE " V" SPHERE_VERSION " - %s", GetName());
	SetConsoleTitle(pszTemp);
#endif

	return true;
	EXC_CATCH;
	return false;
}

void CServer::SysMessage(LPCTSTR pszMsg) const
{
	// Print just to the main console
	if ( !pszMsg || ISINTRESOURCE(pszMsg) )
		return;

#ifdef _WIN32
	NTWindow_PostMsg(pszMsg);
#else
	g_UnixTerminal.print(pszMsg);
#endif
}

void CServer::PrintTelnet(LPCTSTR pszMsg) const
{
	if ( !m_iAdminClients )
		return;

	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( (pClient->GetConnectType() == CONNECT_TELNET) && pClient->m_pAccount )
		{
			if ( !pClient->m_pAccount->IsPriv(PRIV_TELNET_SHORT) )	// this client accepts broadcasts
				pClient->SysMessage(pszMsg);
		}
	}
}

void CServer::PrintStr(LPCTSTR pszMsg) const
{
	// Print to all consoles
	SysMessage(pszMsg);
	PrintTelnet(pszMsg);
}

int CServer::PrintPercent(long iCount, long iTotal)
{
	ADDTOCALLSTACK("CServer::PrintPercent");
	if ( iTotal <= 0 )
		return 100;

	int iPercent = IMULDIV(iCount, 100, iTotal);
	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "%d%%", iPercent);

	PrintTelnet(pszTemp);

#ifndef _WIN32
	if ( g_UnixTerminal.isColorEnabled() )
	{
		SysMessage(pszTemp);
#endif
		size_t iLen = strlen(pszTemp);
		while ( iLen > 0 )	// backspace it
		{
			PrintTelnet("\x08");
#ifndef _WIN32
			SysMessage("\x08");
#endif
			--iLen;
		}
#ifndef _WIN32
	}
#endif

#ifdef _WIN32
	NTWindow_SetWindowTitle(pszTemp);
	g_Service.OnTick();
#endif
	return iPercent;
}

extern void defragSphere(char *);

bool CServer::CommandLine(int argc, TCHAR *argv[])
{
	// Console command line
	// This runs after script file enum but before loading the world file
	// RETURN:
	//  true = keep running after this

	for ( int argn = 1; argn < argc; ++argn )
	{
		TCHAR *pszArg = argv[argn];
		if ( !_IS_SWITCH(pszArg[0]) )
			continue;

		++pszArg;
		switch ( toupper(pszArg[0]) )
		{
			case '?':
			{
				PrintStr(SPHERE_TITLE " \n"
					"Command line switches:\n"
#ifdef _WIN32
					"-cClassName Setup custom window class name for " SPHERE_TITLE " (default: " SPHERE_TITLE "Svr)\n"
#else
					"-c Use colored console output (default: off)\n"
#endif
					"-D Dump global variable DEFNAMEs to defs.txt\n"
#if defined(_WIN32) && !defined(_DEBUG) && !defined(_NO_CRASHDUMP)
					"-E Enable crash dumper\n"
#endif
					"-Gpath/to/saves/ Defrag " SPHERE_TITLE " saves\n"
#ifdef _WIN32
					"-K install/remove Installs or removes NT Service\n"
#endif
					"-Nstring Set the " SPHERE_TITLE " name\n"
					"-Ofilename Output console to this file name\n"
					"-P# Set the port number\n"
					"-Q Quit when finished\n"
				);
				return false;
			}
#ifdef _WIN32
			case 'C':
			case 'K':
			{
				// These are parsed in other places (NT service, NT window part, etc)
				continue;
			}
#else
			case 'C':
			{
				g_UnixTerminal.setColorEnabled(true);
				continue;
			}
#endif
			case 'P':
			{
				m_ip.SetPort(static_cast<WORD>(ATOI(pszArg + 1)));
				continue;
			}
			case 'N':
			{
				SetName(pszArg + 1);
				continue;
			}
			case 'D':
			{
				CFileText ft;
				if ( !ft.Open("defs.txt", OF_WRITE|OF_TEXT) )
					return false;

				for ( size_t i = 0; i < g_Exp.m_VarDefs.GetCount(); ++i )
				{
					if ( (i % 0x1FF) == 0 )
						PrintPercent(i, g_Exp.m_VarDefs.GetCount());

					CVarDefCont *pCont = g_Exp.m_VarDefs.GetAt(i);
					if ( pCont )
						ft.Printf("%s=%s\n", pCont->GetKey(), pCont->GetValStr());
				}
				continue;
			}
#if defined(_WIN32) && !defined(_DEBUG) && !defined(_NO_CRASHDUMP)
			case 'E':
			{
				CrashDump::Enable();
				if ( CrashDump::IsEnabled() )
					PrintStr("Crash dump enabled\n");
				else
					PrintStr("Crash dump NOT enabled\n");
				continue;
			}
#endif
			case 'G':
			{
				defragSphere(pszArg + 1);
				continue;
			}
			case 'O':
			{
				if ( g_Log.Open(pszArg + 1, OF_SHARE_DENY_WRITE|OF_READWRITE|OF_TEXT) )
					g_Log.m_fLockOpen = true;
				continue;
			}
			case 'Q':
				return false;
			default:
				g_Log.Event(LOGM_INIT|LOGL_CRIT, "Don't recognize command line data '%s'\n", static_cast<LPCTSTR>(argv[argn]));
				break;
		}
	}
	return true;
}

bool CServer::OnConsoleCmd(CGString &sText, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CServer::OnConsoleCmd");
	// RETURN:
	//  false = unsuccessful command

	size_t iLen = sText.GetLength();
	if ( iLen <= 0 )
		return true;

	// Convert first character to lowercase
	TCHAR szLow = static_cast<TCHAR>(tolower(sText[0]));
	bool fRet = true;

	if ( ((iLen > 2) || ((iLen == 2) && (sText[1] != '#'))) && (szLow != 'd') )
		goto longcommand;

	switch ( szLow )
	{
		case '?':
		{
			pSrc->SysMessagef(
				"Available commands:\n"
				"#             Force world save (## to save world and statics)\n"
				"A             Update accounts with pending changes of file '" SPHERE_FILE "acct" SPHERE_SCRIPT "'\n"
				"ACCOUNT HELP  View list of account related commands\n"
				"B [msg]       Broadcast message to all clients\n"
				"C             View list of online clients\n"
				"DA            Dump world areas to file 'map_all" SPHERE_SCRIPT "'\n"
				"DUI           Dump unscripted items to file 'unscripted_items" SPHERE_SCRIPT "'\n"
				"E             Clear server script profiler info\n"
				"G             Force world garbage collection\n"
				"H             Hear all that is said (%s)\n"
				"I             View server information\n"
				"L             Toggle log file (%s)\n"
				"P             View server script profiler info (%s) (P# to dump info to file 'profiler_dump.txt')\n"
				"R             Resync pause\n"
				"S             Toggle server secure mode (%s)\n"
				"STRIP         Dump all script templates to file 'sphere_strip" SPHERE_SCRIPT "'\n"
				"T             View list of server active threads\n"
				"U             View list of script triggers used\n"
				"X             Force server exit (X# to save world and statics before exit)\n"
				,
				g_Log.IsLoggedMask(LOGM_PLAYER_SPEAK) ? "ON" : "OFF",
				g_Log.IsFileOpen() ? "ON" : "OFF",
				CurrentProfileData.IsActive() ? "ON" : "OFF",
				g_Cfg.m_fSecure ? "ON" : "OFF"
			);
			break;
		}
		case '#':
		{
			if ( g_Serv.m_fResyncPause )
				goto do_resync;

			g_World.Save(true);
			if ( (iLen > 1) && (sText[1] == '#') )
				g_World.SaveStatics();
			break;
		}
		case 'a':
		{
			g_Accounts.Account_SaveAll();
			g_Cfg.OnTick(true);
			break;
		}
		case 'c':
		{
			ListClients(pSrc);
			break;
		}
		case 'd':
		{
			LPCTSTR pszKey = sText + 1;
			GETNONWHITESPACE(pszKey);
			switch ( tolower(*pszKey) )
			{
				case 'a':	// areas
				{
					++pszKey;
					GETNONWHITESPACE(pszKey);
					if ( g_World.DumpAreas(pSrc, pszKey) )
						pSrc->SysMessage("Areas dump successful\n");
					else
					{
						pSrc->SysMessage("Areas dump failed\n");
						fRet = false;
					}
					break;
				}
				case 'u':	// unscripted
				{
					++pszKey;
					switch ( tolower(*pszKey) )
					{
						case 'i':	// items
						{
							++pszKey;
							GETNONWHITESPACE(pszKey);
							if ( g_Cfg.DumpUnscriptedItems(pSrc, pszKey) )
								pSrc->SysMessage("Unscripted items dump successful\n");
							else
							{
								pSrc->SysMessage("Unscripted items dump failed\n");
								fRet = false;
							}
							break;
						}
						default:
							goto longcommand;
					}
					break;
				}
				default:
					goto longcommand;
			}
			break;
		}
		case 'e':
		{
			if ( IsSetEF(EF_Script_Profiler) && (g_profiler.initstate == 0xF1) )
			{
				TScriptProfiler::TScriptProfilerFunction *pFunction;
				for ( pFunction = g_profiler.FunctionsHead; pFunction != NULL; pFunction = pFunction->next )
					pFunction->average = pFunction->max = pFunction->min = pFunction->total = pFunction->called = 0;

				TScriptProfiler::TScriptProfilerTrigger *pTrigger;
				for ( pTrigger = g_profiler.TriggersHead; pTrigger != NULL; pTrigger = pTrigger->next )
					pTrigger->average = pTrigger->max = pTrigger->min = pTrigger->total = pTrigger->called = 0;

				g_profiler.total = g_profiler.called = 0;
				pSrc->SysMessage("Scripts profiler info cleared\n");
			}
			else
				pSrc->SysMessage("Script profiler feature is not enabled on " SPHERE_FILE ".ini\n");
			break;
		}
		case 'g':
		{
			if ( g_Serv.m_fResyncPause )
			{
			do_resync:
				pSrc->SysMessage("Not allowed during resync pause. Use 'R' to restart resync\n");
				fRet = false;
				break;
			}
			if ( g_World.IsSaving() )
			{
			do_saving:
				pSrc->SysMessage("Not allowed during background worldsave. Use '#' to finish worldsave\n");
				fRet = false;
				break;
			}
			g_World.GarbageCollection();
			break;
		}
		case 'h':
		{
			CScript script("HEARALL");
			fRet = r_Verb(script, pSrc);
			break;
		}
		case 'i':
		{
			CScript script("INFORMATION");
			fRet = r_Verb(script, pSrc);
			break;
		}
		case 'l':
		{
			if ( g_Log.IsFileOpen() )
				g_Log.Close();
			else
				g_Log.OpenLog();
			pSrc->SysMessagef("Log file %s\n", g_Log.IsFileOpen() ? "enabled" : "disabled");
			break;
		}
		case 'p':
		{
			ProfileDump(pSrc, ((iLen > 1) && (sText[1] == '#')));
			break;
		}
		case 'r':
		{
			if ( pSrc != this )		// not from console
				pSrc->SysMessage("Not allowed to use 'r' command via Telnet. Use 'resync' instead\n");
			else
			{
				if ( !m_fResyncPause && g_World.IsSaving() )
					goto do_saving;
				SetResyncPause(!m_fResyncPause, pSrc, true);
			}
			break;
		}
		case 's':
		{
			CScript script("SECURE");
			fRet = r_Verb(script, pSrc);
			break;
		}
		case 't':
		{
			size_t iThreadCount = ThreadHolder::getActiveThreads();
			pSrc->SysMessagef("Current active threads: %" FMTSIZE_T "\n", iThreadCount);
			for ( size_t iThread = 0; iThread < iThreadCount; ++iThread )
			{
				IThread *pThread = ThreadHolder::getThreadAt(iThread);
				if ( pThread )
					pSrc->SysMessagef("Thread %" FMTSIZE_T ": %s (ID=%u, Priority=%d)\n", iThread + 1, pThread->getName(), pThread->getId(), pThread->getPriority());
			}
			break;
		}
		case 'u':
		{
			TriglistPrint();
			break;
		}
		case 'x':
		{
			bool fSave = ((iLen > 1) && (sText[1] == '#'));
			if ( g_Cfg.m_fSecure && !fSave )
			{
				pSrc->SysMessage("Secure mode prevents keyboard exit. Use 'S' to disable secure mode first\n");
				fRet = false;
			}
			else
			{
				if ( fSave )
				{
					if ( g_Serv.m_fResyncPause )
						goto do_resync;

					g_World.Save(true);
					g_World.SaveStatics();
				}
				g_Log.Event(LOGL_FATAL, "Shutdown initialized\n");
				SetExitFlag(1);
			}
			break;
		}
		default:
			goto longcommand;
	}
	goto endconsole;

longcommand:
	if ( ((iLen > 1) && (sText[1] != ' ')) || (szLow == 'b') )
	{
		LPCTSTR pszText = sText;
		if ( !strnicmp(pszText, "strip", 5) )
		{
			if ( g_Cfg.m_sStripPath.IsEmpty() )
			{
				pSrc->SysMessage("StripPath not defined on " SPHERE_FILE ".ini, function aborted\n");
				return false;
			}

			TCHAR szFileStrip[_MAX_PATH];
			strncpy(szFileStrip, g_Cfg.m_sStripPath, _MAX_PATH - 1);
			szFileStrip[_MAX_PATH - 1] = '\0';
			strcat(szFileStrip, "sphere_strip" SPHERE_SCRIPT);

			FILE *pFileStrip = fopen(szFileStrip, "w");
			if ( !pFileStrip )
			{
				pSrc->SysMessagef("Can't open file '%s' for writing\n", szFileStrip);
				return false;
			}

			size_t i = 0;
			CResourceScript *script;
			while ( (script = g_Cfg.GetResourceFile(i++)) != NULL )
			{
				TCHAR szFileScript[_MAX_PATH];
				strncpy(szFileScript, script->GetFilePath(), _MAX_PATH - 1);
				szFileScript[_MAX_PATH - 1] = '\0';

				FILE *pFileScript = fopen(szFileScript, "r");
				if ( !pFileScript )
				{
					pSrc->SysMessagef("Can't read file '%s'\n", szFileScript);
					continue;
				}

				char *x;
				char *y = Str_GetTemp();
				char *z = Str_GetTemp();

				while ( !feof(pFileScript) )
				{
					z[0] = '\0';
					y[0] = '\0';
					fgets(y, SCRIPT_MAX_LINE_LEN, pFileScript);

					x = y;
					GETNONWHITESPACE(x);
					strncpy(z, x, THREAD_STRING_LENGTH - 1);
					z[THREAD_STRING_LENGTH - 1] = '\0';

					if ( ((z[0] == '[') && (strnicmp(z, "[EOF]", 5) != 0)) || !strnicmp(z, "DEFNAME", 7) || !strnicmp(z, "NAME", 4) ||
						!strnicmp(z, "ID", 2) || !strnicmp(z, "TYPE", 4) || !strnicmp(z, "WEIGHT", 6) || !strnicmp(z, "VALUE", 5) ||
						!strnicmp(z, "DAM", 3) || !strnicmp(z, "ARMOR", 5) || !strnicmp(z, "SKILLMAKE", 9) || !strnicmp(z, "RESOURCES", 9) ||
						!strnicmp(z, "DUPEITEM", 8) || !strnicmp(z, "DUPELIST", 8) || !strnicmp(z, "CAN", 3) || !strnicmp(z, "TEVENTS", 7) ||
						!strnicmp(z, "CATEGORY", 8) || !strnicmp(z, "SUBSECTION", 10) || !strnicmp(z, "DESCRIPTION", 11) || !strnicmp(z, "COLOR", 5) ||
						!strnicmp(z, "GROUP", 5) || !strnicmp(z, "P=", 2) || !strnicmp(z, "RECT=", 5) || !strnicmp(z, "ON=@", 4) )
					{
						fputs(y, pFileStrip);
					}
				}
				fclose(pFileScript);
			}
			fclose(pFileStrip);
			pSrc->SysMessagef("Scripts have been stripped to '%s'\n", szFileStrip);
			return true;
		}

		if ( g_Cfg.IsConsoleCmd(szLow) )
			++pszText;

		if ( !g_Cfg.CanUsePrivVerb(this, pszText, pSrc) )
		{
			pSrc->SysMessagef("not privileged for command '%s'\n", pszText);
			fRet = false;
		}
		else
		{
			CScript script(pszText);
			if ( !r_Verb(script, pSrc) )
			{
				pSrc->SysMessagef("unknown command '%s'\n", pszText);
				fRet = false;
			}
		}
	}
	else
	{
		pSrc->SysMessagef("unknown command '%s'\n", static_cast<LPCTSTR>(sText));
		fRet = false;
	}

endconsole:
	sText.Empty();
	return fRet;
}

void CServer::ListClients(CTextConsole *pConsole) const
{
	ADDTOCALLSTACK("CServer::ListClients");
	// Mask which clients we want ?
	// Give a format of what info we want to SHOW ?
	if ( !pConsole )
		return;

	const CChar *pCharCmd = pConsole->GetChar();
	const CChar *pChar = NULL;
	const CAccount *pAcc = NULL;
	TCHAR *pszMsg = Str_GetTemp();
	TCHAR *pszMsgTemp = Str_GetTemp();
	TCHAR szPriv;
	LPCTSTR pszState = NULL;
	size_t iClients = 0;

	ClientIterator it;
	for ( const CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		++iClients;
		pChar = pClient->GetChar();
		pAcc = pClient->m_pAccount;
		szPriv = (pAcc && (pAcc->GetPrivLevel() > PLEVEL_Player)) ? '+' : '=';

		if ( pChar )
		{
			if ( pCharCmd && !pCharCmd->CanDisturb(pChar) )
				continue;

			sprintf(pszMsgTemp, "%lx:Acc%c'%s', Char='%s' (IP: %s)\n", pClient->GetSocketID(), szPriv, pAcc ? pAcc->GetName() : "<NA>", pChar->GetName(), pClient->GetPeerStr());
		}
		else
		{
			if ( pClient->GetPrivLevel() > pConsole->GetPrivLevel() )
				continue;

			switch ( pClient->GetConnectType() )
			{
				case CONNECT_HTTP:
					pszState = "WEB";
					break;
				case CONNECT_TELNET:
					pszState = "TELNET";
					break;
				default:
					pszState = "NOT LOGGED IN";
					break;
			}

			sprintf(pszMsgTemp, "%lx:Acc%c'%s' (IP: %s) %s\n", pClient->GetSocketID(), szPriv, pAcc ? pAcc->GetName() : "<NA>", pClient->GetPeerStr(), pszState);
		}

		// If we have many clients, SCRIPT_MAX_LINE_LEN may be too short ;) (matex)
		if ( strlen(pszMsg) + strlen(pszMsgTemp) >= SCRIPT_MAX_LINE_LEN )
		{
			pConsole->SysMessage(pszMsg);
			pszMsg[0] = '\0';
		}
		strcat(pszMsg, pszMsgTemp);
	}

	if ( iClients <= 0 )
		sprintf(pszMsgTemp, "%s\n", g_Cfg.GetDefaultMsg(DEFMSG_HL_NO_CLIENT));
	else if ( iClients == 1 )
		sprintf(pszMsgTemp, "%s\n", g_Cfg.GetDefaultMsg(DEFMSG_HL_ONE_CLIENT));
	else
		sprintf(pszMsgTemp, "%s %" FMTSIZE_T "\n", g_Cfg.GetDefaultMsg(DEFMSG_HL_MANY_CLIENTS), iClients);

	pConsole->SysMessage(pszMsg);
	pConsole->SysMessage(pszMsgTemp);
}

LPCTSTR CServer::GetStatusString(BYTE bIndex) const
{
	ADDTOCALLSTACK("CServer::GetStatusString");
	// NOTE: The key names should match those in CServerDef::r_LoadVal
	// A ping will return this as well
	// 0 or 0x21 = main status

	TCHAR *pszTemp = Str_GetTemp();
	switch ( bIndex )
	{
		case 0x21:	// '!'
		{
			// Typical (first time) poll response
			TCHAR szVersion[128];
			sprintf(pszTemp, SPHERE_TITLE ", Name=%s, Port=%hu, Ver=" SPHERE_VERSION ", TZ=%hhd, Email=%s, URL=%s, Lang=%s, CliVer=%s\n", GetName(), m_ip.GetPort(), m_TimeZone, static_cast<LPCTSTR>(m_sEMail), static_cast<LPCTSTR>(m_sURL), static_cast<LPCTSTR>(m_sLang), m_ClientVersion.WriteClientVerString(m_ClientVersion.GetClientVer(), szVersion));
			break;
		}
		case 0x22:	// '"'
		{
			// Shown in the INFO page in game
			sprintf(pszTemp, SPHERE_TITLE ", Name=%s, Age=%lld, Clients=%lu, Items=%lu, Chars=%lu, Mem=%luK\n", GetName(), GetAgeHours() / 24, StatGet(SERV_STAT_CLIENTS), StatGet(SERV_STAT_ITEMS), StatGet(SERV_STAT_CHARS), StatGet(SERV_STAT_MEM));
			break;
		}
		case 0x24:	// '$'
		{
			// Show at startup
			sprintf(pszTemp, "Admin=%s, URL=%s, Lang=%s, TZ=%hhd\n", static_cast<LPCTSTR>(m_sEMail), static_cast<LPCTSTR>(m_sURL), static_cast<LPCTSTR>(m_sLang), m_TimeZone);
			break;
		}
		case 0x25:	// '%'
		{
			// ConnectUO status string
			sprintf(pszTemp, SPHERE_TITLE " Items=%lu, Mobiles=%lu, Clients=%lu, Mem=%luK", StatGet(SERV_STAT_ITEMS), StatGet(SERV_STAT_CHARS), StatGet(SERV_STAT_CLIENTS), StatGet(SERV_STAT_MEM));
			break;
		}
	}
	return pszTemp;
}

bool CServer::r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef)
{
	ADDTOCALLSTACK("CServer::r_GetRef");
	if ( IsDigit(pszKey[0]) )
	{
		size_t i = 1;
		while ( IsDigit(pszKey[i]) )
			++i;

		if ( pszKey[i] == '.' )
		{
			pRef = g_Cfg.Server_GetDef(static_cast<size_t>(ATOI(pszKey)));	// must use this to stop at .
			pszKey += i + 1;
			return true;
		}
	}

	if ( g_Cfg.r_GetRef(pszKey, pRef) )
		return true;
	if ( g_World.r_GetRef(pszKey, pRef) )
		return true;
	return CScriptObj::r_GetRef(pszKey, pRef);
}

bool CServer::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CServer::r_WriteVal");
	if ( !strnicmp(pszKey, "ACCOUNT.", 8) )
	{
		pszKey += 8;
		CAccount *pAccount = NULL;

		// Extract account name/index to a temporary buffer
		TCHAR *pszTemp = Str_GetTemp();
		strncpy(pszTemp, pszKey, MAX_ACCOUNT_NAME_SIZE - 1);
		pszTemp[MAX_ACCOUNT_NAME_SIZE - 1] = '\0';

		TCHAR *pszSplit = strchr(pszTemp, '.');
		if ( pszSplit )
			*pszSplit = '\0';

		// Adjust pszKey to point to end of account name/index
		pszKey += strlen(pszTemp);

		// Try to fetch using indexes
		if ( (*pszTemp >= '0') && (*pszTemp <= '9') )
		{
			size_t iNum = Exp_GetVal(pszTemp);
			if ( (*pszTemp == '\0') && (iNum < g_Accounts.Account_GetCount()) )
				pAccount = g_Accounts.Account_Get(iNum);
		}

		// Indexes failed, try to fetch using names
		if ( !pAccount )
			pAccount = g_Accounts.Account_Find(pszTemp);

		if ( !*pszKey )		// check if the account exists
		{
			sVal.FormatVal(pAccount != NULL);
			return true;
		}
		else if ( pAccount )	// get an account property
		{
			SKIP_SEPARATORS(pszKey);
			return pAccount->r_WriteVal(pszKey, sVal, pSrc);
		}
		return false;
	}
	else if ( !strnicmp(pszKey, "GMPAGE.", 7) )
	{
		pszKey += 7;
		size_t iNum = Exp_GetVal(pszKey);
		if ( iNum >= g_World.m_GMPages.GetCount() )
			return false;

		CGMPage *pGMPage = static_cast<CGMPage *>(g_World.m_GMPages.GetAt(iNum));
		if ( !pGMPage )
			return false;

		SKIP_SEPARATORS(pszKey);
		return pGMPage->r_WriteVal(pszKey, sVal, pSrc);
	}

	if ( g_Cfg.r_WriteVal(pszKey, sVal, pSrc) )
		return true;
	if ( g_World.r_WriteVal(pszKey, sVal, pSrc) )
		return true;
	return CServerDef::r_WriteVal(pszKey, sVal, pSrc);
}

bool CServer::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CServer::r_LoadVal");
	if ( g_Cfg.r_LoadVal(s) )
		return true;
	if ( g_World.r_LoadVal(s) )
		return true;
	return CServerDef::r_LoadVal(s);
}

enum SV_TYPE
{
	SV_ACCOUNT,
	SV_ACCOUNTS,	//read only
	SV_ALLCLIENTS,
	SV_B,
	SV_BLOCKIP,
	SV_CHARS,		//read only
	SV_CLEARLISTS,
	SV_CONSOLE,
	SV_EXPORT,
	SV_GARBAGE,
	SV_GMPAGES,		//read only
	SV_HEARALL,
	SV_IMPORT,
	SV_INFORMATION,
	SV_ITEMS,		//read only
	SV_LOAD,
	SV_LOG,
	SV_PRINTLISTS,
	SV_RESPAWN,
	SV_RESTOCK,
	SV_RESTORE,
	SV_RESYNC,
	SV_SAVE,
	SV_SAVECOUNT,	//read only
	SV_SAVESTATICS,
	SV_SECURE,
	SV_SHRINKMEM,
	SV_SHUTDOWN,
	SV_TIME,		// read only
	SV_UNBLOCKIP,
	SV_VARLIST,
	SV_QTY
};

LPCTSTR const CServer::sm_szVerbKeys[SV_QTY + 1] =
{
	"ACCOUNT",
	"ACCOUNTS",		// read only
	"ALLCLIENTS",
	"B",
	"BLOCKIP",
	"CHARS",		// read only
	"CLEARLISTS",
	"CONSOLE",
	"EXPORT",
	"GARBAGE",
	"GMPAGES",		// read only
	"HEARALL",
	"IMPORT",
	"INFORMATION",
	"ITEMS",		// read only
	"LOAD",
	"LOG",
	"PRINTLISTS",
	"RESPAWN",
	"RESTOCK",
	"RESTORE",
	"RESYNC",
	"SAVE",
	"SAVECOUNT",	// read only
	"SAVESTATICS",
	"SECURE",
	"SHRINKMEM",
	"SHUTDOWN",
	"TIME",			// read only
	"UNBLOCKIP",
	"VARLIST",
	NULL
};

bool CServer::r_Verb(CScript &s, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CServer::r_Verb");
	if ( !pSrc )
		return false;

	EXC_TRY("Verb");
	int index = FindTableSorted(s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1);
	if ( index < 0 )
	{
		LPCTSTR pszKey = s.GetKey();
		CScriptTriggerArgs Args(s.GetArgRaw());
		CGString sVal;
		if ( r_Call(pszKey, pSrc, &Args, &sVal) )
			return true;

		if ( !strnicmp(pszKey, "ACCOUNT.", 8) )
		{
			pszKey += 8;
			CAccount *pAccount = NULL;

			// Extract account name/index to a temporary buffer
			TCHAR *pszTemp = Str_GetTemp();
			strncpy(pszTemp, pszKey, MAX_ACCOUNT_NAME_SIZE - 1);
			pszTemp[MAX_ACCOUNT_NAME_SIZE - 1] = '\0';

			TCHAR *pszSplit = strchr(pszTemp, '.');
			if ( pszSplit )
				*pszSplit = '\0';

			// Adjust pszKey to point to end of account name/index
			pszKey += strlen(pszTemp);

			// Try to fetch using indexes
			if ( (*pszTemp >= '0') && (*pszTemp <= '9') )
			{
				size_t iNum = Exp_GetVal(pszTemp);
				if ( (*pszTemp == '\0') && (iNum < g_Accounts.Account_GetCount()) )
					pAccount = g_Accounts.Account_Get(iNum);
			}

			// Indexes failed, try to fetch using names
			if ( !pAccount )
				pAccount = g_Accounts.Account_Find(pszTemp);

			if ( pAccount && *pszKey )	// execute function
			{
				SKIP_SEPARATORS(pszKey);
				CScript script(pszKey, s.GetArgStr());
				return pAccount->r_LoadVal(script);
			}
			return false;
		}
		else if ( !strnicmp(pszKey, "GMPAGE.", 7) )
		{
			pszKey += 7;
			size_t iNum = Exp_GetVal(pszKey);
			if ( iNum >= g_World.m_GMPages.GetCount() )
				return false;

			CGMPage *pGMPage = static_cast<CGMPage *>(g_World.m_GMPages.GetAt(iNum));
			if ( !pGMPage )
				return false;

			SKIP_SEPARATORS(pszKey);
			CScript script(pszKey, s.GetArgStr());
			return pGMPage->r_LoadVal(script);
		}
		else if ( !strnicmp(pszKey, "CLEARVARS", 9) )
		{
			pszKey = s.GetArgStr();
			SKIP_SEPARATORS(pszKey);
			g_Exp.m_VarGlobals.ClearKeys(pszKey);
			return true;
		}
	}

	switch ( static_cast<SV_TYPE>(index) )
	{
		case SV_ACCOUNTS:
		case SV_CHARS:
		case SV_GMPAGES:
		case SV_ITEMS:
		case SV_SAVECOUNT:
		case SV_TIME:
			return false;
		case SV_ACCOUNT:
			return g_Accounts.Account_OnCmd(s.GetArgRaw(), pSrc);
		case SV_ALLCLIENTS:
		{
			ClientIterator it;
			for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
			{
				if ( !pClient->GetChar() )
					continue;
				CScript script(s.GetArgStr());
				pClient->GetChar()->r_Verb(script, pSrc);
			}
			break;
		}
		case SV_B:
		{
			g_World.Broadcast(s.GetArgStr());
			break;
		}
		case SV_BLOCKIP:
		{
			TCHAR *ppArgs[2];
			size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), ", ");
			if ( iQty <= 0 )
			{
				pSrc->SysMessage("Usage: BLOCKIP [IP] [time]\n");
				return true;
			}

#ifdef _MTNETWORK
			HistoryIP &history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(ppArgs[0]);
#else
			HistoryIP &history = g_NetworkIn.getIPHistoryManager().getHistoryForIP(ppArgs[0]);
#endif

			INT64 iTimeout = (iQty >= 2) ? Exp_GetLLVal(ppArgs[1]) : -1;
			if ( iTimeout >= 0 )
				pSrc->SysMessagef("IP blocked for %lld seconds\n", iTimeout);
			else
				pSrc->SysMessagef("IP%s blocked\n", history.m_blocked ? " already" : "");

			history.setBlocked(true, iTimeout);
			break;
		}
		case SV_CONSOLE:
		{
			CGString sVal = s.GetArgRaw();
			OnConsoleCmd(sVal, pSrc);
			break;
		}
		case SV_EXPORT:
		{
			TCHAR *ppArgs[3];
			size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs));
			if ( iQty <= 0 )
			{
				pSrc->SysMessage("Usage: EXPORT [filename] [flags] [area distance]\n");
				break;
			}

			if ( g_World.Export(ppArgs[0], pSrc->GetChar(), (iQty >= 2) ? static_cast<WORD>(ATOI(ppArgs[1])) : IMPFLAGS_ITEMS, (iQty >= 3) ? ATOI(ppArgs[2]) : SHRT_MAX) )
				pSrc->SysMessagef("Export complete\n");
			else
				pSrc->SysMessage("Export failed\n");
			break;
		}
		case SV_GARBAGE:
		{
			if ( g_Serv.m_fResyncPause || g_World.IsSaving() )
				pSrc->SysMessage("Not allowed during world save or resync pause\n");
			else
				g_World.GarbageCollection();
			break;
		}
		case SV_HEARALL:
		{
			g_Log.SetLogMask(static_cast<DWORD>(s.GetArgFlag(g_Log.GetLogMask(), LOGM_PLAYER_SPEAK)));
			pSrc->SysMessagef("HearAll %s\n", g_Log.IsLoggedMask(LOGM_PLAYER_SPEAK) ? "enabled" : "disabled");
			break;
		}
		case SV_INFORMATION:
		{
			pSrc->SysMessage(GetStatusString(0x22));
			pSrc->SysMessage(GetStatusString(0x24));
			break;
		}
		case SV_IMPORT:
		{
			TCHAR *ppArgs[3];
			size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs));
			if ( iQty <= 0 )
			{
				pSrc->SysMessage("Usage: IMPORT [filename] [flags] [area distance]\n");
				break;
			}

			if ( g_World.Import(ppArgs[0], pSrc->GetChar(), (iQty >= 2) ? static_cast<WORD>(ATOI(ppArgs[1])) : IMPFLAGS_ITEMS|IMPFLAGS_CHARS, (iQty >= 3) ? ATOI(ppArgs[2]) : SHRT_MAX) )
				pSrc->SysMessagef("Import complete\n");
			else
				pSrc->SysMessage("Import failed\n");
			break;
		}
		case SV_LOAD:
		{
			return (g_Cfg.LoadResourcesAdd(s.GetArgStr()) != NULL);
		}
		case SV_LOG:
		{
			DWORD dwMask = LOGL_EVENT;
			LPCTSTR pszArgs = s.GetArgStr();
			if ( pszArgs && (*pszArgs == '@') )
			{
				++pszArgs;
				if ( *pszArgs != '@' )
					dwMask |= LOGM_NOCONTEXT;
			}
			g_Log.Event(dwMask, "%s\n", pszArgs);
			break;
		}
		case SV_RESPAWN:
		{
			g_World.RespawnDeadNPCs();
			break;
		}
		case SV_RESTOCK:
		{
			g_World.Restock();
			break;
		}
		case SV_RESTORE:
		{
			TCHAR *ppArgs[3];
			size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs));
			if ( iQty < 3 )
			{
				pSrc->SysMessage("Usage: RESTORE [filename] [account] [char]\n");
				break;
			}

			if ( g_World.Import(ppArgs[0], pSrc->GetChar(), IMPFLAGS_ITEMS|IMPFLAGS_CHARS|IMPFLAGS_ACCOUNT, SHRT_MAX, ppArgs[1], ppArgs[2]) )
				pSrc->SysMessagef("Restore complete\n");
			else
				pSrc->SysMessage("Restore failed\n");
			break;
		}
		case SV_RESYNC:
		{
			// Bad things will happen if a resync occur while a script is executing. Flag
			// that a resync has been requested and it will take place during server tick
			m_fResyncRequested = pSrc;
			break;
		}
		case SV_SAVE:
		{
			g_World.Save(s.GetArgVal() != 0);
			break;
		}
		case SV_SAVESTATICS:
		{
			g_World.SaveStatics();
			break;
		}
		case SV_SECURE:
		{
			g_Cfg.m_fSecure = (s.GetArgFlag(g_Cfg.m_fSecure, true) != 0);
#ifndef _WIN32
			SetSignals(g_Cfg.m_fSecure);
#endif
			pSrc->SysMessagef("Secure mode %s\n", g_Cfg.m_fSecure ? "enabled" : "disabled");
			break;
		}
		case SV_SHRINKMEM:
		{
#ifdef _WIN32
			if ( GRAY_GetOSInfo()->dwPlatformId != VER_PLATFORM_WIN32_NT )
			{
				g_Log.EventError("Command not available on Windows 95/98/ME\n");
				return false;
			}
			if ( !SetProcessWorkingSetSize(GetCurrentProcess(), static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1)) )
			{
				g_Log.EventError("Error during memory shrink\n");
				return false;
			}
			pSrc->SysMessage("Memory shrinked succesfully\n");
			break;
#else
			g_Log.EventError("Command only available on Windows builds\n");
			return false;
#endif
		}
		case SV_SHUTDOWN:
		{
			Shutdown(s.HasArgs() ? s.GetArgVal() : 15);
			break;
		}
		case SV_UNBLOCKIP:
		{
#ifdef _MTNETWORK
			HistoryIP &history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(s.GetArgRaw());
#else
			HistoryIP &history = g_NetworkIn.getIPHistoryManager().getHistoryForIP(s.GetArgRaw());
#endif
			pSrc->SysMessagef("IP%s unblocked\n", history.m_blocked ? "" : " already");
			history.setBlocked(false);
			break;
		}
		case SV_VARLIST:
		{
			if ( !strcmpi(s.GetArgStr(), "log") )
				pSrc = &g_Serv;
			g_Exp.m_VarGlobals.DumpKeys(pSrc, "VAR.");
			break;
		}
		case SV_PRINTLISTS:
		{
			if ( !strcmpi(s.GetArgStr(), "log") )
				pSrc = &g_Serv;
			g_Exp.m_ListGlobals.DumpKeys(pSrc, "LIST.");
			break;
		}
		case SV_CLEARLISTS:
		{
			g_Exp.m_ListGlobals.ClearKeys(s.GetArgStr());
			break;
		}
		default:
			return CScriptObj::r_Verb(s, pSrc);
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	g_Log.EventDebug("source '%s' char '%s' uid '0%lx'\n", (pSrc && pSrc->GetName()) ? pSrc->GetName() : "", (pSrc && pSrc->GetChar()) ? pSrc->GetChar()->GetName() : "", (pSrc && pSrc->GetChar()) ? static_cast<DWORD>(pSrc->GetChar()->GetUID()) : 0);
	EXC_DEBUG_END;
	return false;
}

void CServer::SetResyncPause(bool fPause, CTextConsole *pSrc, bool fMessage)
{
	ADDTOCALLSTACK("CServer::SetResyncPause");
	if ( fPause )
	{
		g_Serv.SysMessagef("%s\n", g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_START));
		if ( fMessage )
			g_World.Broadcast(g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_START));
		else if ( pSrc && pSrc->GetChar() )
			pSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_START));

		SetServerMode(SERVMODE_ResyncPause);
		m_fResyncPause = true;
		g_Cfg.Unload(true);
	}
	else
	{
		g_Serv.SysMessagef("%s\n", g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_RESTART));
		SetServerMode(SERVMODE_ResyncLoad);

		long lMsgNum = g_Cfg.Load(true) ? DEFMSG_SERVER_RESYNC_SUCCESS : DEFMSG_SERVER_RESYNC_FAILED;
		g_Serv.SysMessagef("%s\n", g_Cfg.GetDefaultMsg(lMsgNum));
		if ( fMessage )
			g_World.Broadcast(g_Cfg.GetDefaultMsg(lMsgNum));
		else if ( pSrc && pSrc->GetChar() )
			pSrc->SysMessage(g_Cfg.GetDefaultMsg(lMsgNum));

		m_fResyncPause = false;
		SetServerMode(SERVMODE_Run);
	}
}

void CServer::ProfileDump(CTextConsole *pSrc, bool fDump)
{
	ADDTOCALLSTACK("CServer::ProfileDump");
	if ( !pSrc )
		return;

	CFileText *ft = NULL;
	if ( fDump )
	{
		ft = new CFileText();
		if ( !ft->Open("profiler_dump.txt", OF_CREATE|OF_TEXT) )
		{
			delete ft;
			ft = NULL;
		}
	}

	if ( ft )
		ft->Printf("Profiles %s: (%d sec total)\n", CurrentProfileData.IsActive() ? "ON" : "OFF", CurrentProfileData.GetActiveWindow());
	else
		pSrc->SysMessagef("Profiles %s: (%d sec total)\n", CurrentProfileData.IsActive() ? "ON" : "OFF", CurrentProfileData.GetActiveWindow());

	if ( IsSetEF(EF_Script_Profiler) )
	{
		size_t iThreadCount = ThreadHolder::getActiveThreads();
		for ( size_t iThread = 0; iThread < iThreadCount; ++iThread )
		{
			IThread *pThread = ThreadHolder::getThreadAt(iThread);
			if ( !pThread )
				continue;

			const ProfileData &profile = static_cast<AbstractSphereThread *>(pThread)->m_profile;
			if ( !profile.IsEnabled() )
				continue;

			if ( ft )
				ft->Printf("Thread %u, Name=%s\n", pThread->getId(), pThread->getName());
			else
				pSrc->SysMessagef("Thread %u, Name=%s\n", pThread->getId(), pThread->getName());

			for ( PROFILE_TYPE i = PROFILE_IDLE; i < PROFILE_QTY; i = static_cast<PROFILE_TYPE>(i + 1) )
			{
				if ( !profile.IsEnabled(i) )
					continue;

				if ( ft )
					ft->Printf("%-10s = %s\n", profile.GetName(i), profile.GetDescription(i));
				else
					pSrc->SysMessagef("%-10s = %s\n", profile.GetName(i), profile.GetDescription(i));
			}
		}

		if ( g_profiler.initstate != 0xF1 )
			pSrc->SysMessagef("Scripts profiler is not initialized\n");
		else if ( !g_profiler.called )
			pSrc->SysMessagef("Script profiler is not yet informational\n");
		else
		{
			ULONGLONG average = g_profiler.total / g_profiler.called;
			ULONGLONG divby = llTimeProfileFrequency / 1000;

			if ( ft )
				ft->Printf("Scripts: called %lu times and took %llu.%04llu ms (%llu.%04llu ms average). Reporting with highest average\n",
					g_profiler.called,
					g_profiler.total / divby,
					((g_profiler.total * 10000) / divby) % 10000,
					average / divby,
					((average * 10000) / divby) % 10000
				);
			else
				pSrc->SysMessagef("Scripts: called %lu times and took %llu.%04llu ms (%llu.%04llu ms average). Reporting with highest average\n",
					g_profiler.called,
					g_profiler.total / divby,
					((g_profiler.total * 10000) / divby) % 10000,
					average / divby,
					((average * 10000) / divby) % 10000
				);

			TScriptProfiler::TScriptProfilerFunction *pFunction;
			for ( pFunction = g_profiler.FunctionsHead; pFunction != NULL; pFunction = pFunction->next )
			{
				if ( pFunction->average > average )
				{
					if ( ft )
						ft->Printf("FUNCTION '%-30s' called %6lu times (%6llu.%04llu min, %6llu.%04llu avg, %6llu.%04llu max) [total: %6llu.%04llu ms]\n",
							pFunction->name,
							pFunction->called,
							pFunction->min / divby,
							((pFunction->min * 10000) / divby) % 10000,
							pFunction->average / divby,
							((pFunction->average * 10000) / divby) % 10000,
							pFunction->max / divby,
							((pFunction->max * 10000) / divby) % 10000,
							pFunction->total / divby,
							((pFunction->total * 10000) / divby) % 10000
						);
					else
						pSrc->SysMessagef("FUNCTION '%-30s' called %6lu times (%6llu.%04llu min, %6llu.%04llu avg, %6llu.%04llu max) [total: %6llu.%04llu ms]\n",
							pFunction->name,
							pFunction->called,
							pFunction->min / divby,
							((pFunction->min * 10000) / divby) % 10000,
							pFunction->average / divby,
							((pFunction->average * 10000) / divby) % 10000,
							pFunction->max / divby,
							((pFunction->max * 10000) / divby) % 10000,
							pFunction->total / divby,
							((pFunction->total * 10000) / divby) % 10000
						);
				}
			}

			TScriptProfiler::TScriptProfilerTrigger *pTrigger;
			for ( pTrigger = g_profiler.TriggersHead; pTrigger != NULL; pTrigger = pTrigger->next )
			{
				if ( pTrigger->average > average )
				{
					if ( ft )
						ft->Printf("TRIGGER '%-25s' called %6lu times (%6llu.%04llu min, %6llu.%04llu avg, %6llu.%04llu max), total: %6llu.%04llu ms\n",
							pTrigger->name,
							pTrigger->called,
							pTrigger->min / divby,
							((pTrigger->min * 10000) / divby) % 10000,
							pTrigger->average / divby,
							((pTrigger->average * 10000) / divby) % 10000,
							pTrigger->max / divby,
							((pTrigger->max * 10000) / divby) % 10000,
							pTrigger->total / divby,
							((pTrigger->total * 10000) / divby) % 10000
						);
					else
						pSrc->SysMessagef("TRIGGER '%-25s' called %6lu times (%6llu.%04llu min, %6llu.%04llu avg, %6llu.%04llu max), total: %6llu.%04llu ms\n",
							pTrigger->name,
							pTrigger->called,
							pTrigger->min / divby,
							((pTrigger->min * 10000) / divby) % 10000,
							pTrigger->average / divby,
							((pTrigger->average * 10000) / divby) % 10000,
							pTrigger->max / divby,
							((pTrigger->max * 10000) / divby) % 10000,
							pTrigger->total / divby,
							((pTrigger->total * 10000) / divby) % 10000
						);
				}
			}

			pSrc->SysMessage("Report complete!\n");
		}
	}
	else
		pSrc->SysMessage("Script profiler feature is not enabled on " SPHERE_FILE ".ini\n");

	if ( ft )
	{
		ft->Close();
		delete ft;
	}
}
