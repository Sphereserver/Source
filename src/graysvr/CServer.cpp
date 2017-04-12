#include "graysvr.h"	// predef header.

#include "../common/grayver.h"	// sphere version
#include "../common/version/GitRevision.h"
#include "../common/CAssoc.h"
#include "../common/CFileList.h"
#include "../network/network.h"
#include "../graysvr/PingServer.h"

#ifdef _WIN32
	#include "ntservice.h"	// g_Service
#else
	#include "UnixTerminal.h"
#endif

#if defined(_WIN32) && !defined(_DEBUG)
	#include "../common/crashdump/crashdump.h"
#endif

#if !defined(_WIN32) || defined(_LIBEV)
	extern LinuxEv g_NetworkEvent;
#endif


////////////////////////////////////////////////////////
// -CTextConsole

CChar * CTextConsole::GetChar() const
{
	ADDTOCALLSTACK("CTextConsole::GetChar");
	return( const_cast <CChar *>( dynamic_cast <const CChar *>( this )));
}

int CTextConsole::OnConsoleKey( CGString & sText, TCHAR nChar, bool fEcho )
{
	ADDTOCALLSTACK("CTextConsole::OnConsoleKey");
	// eventaully we should call OnConsoleCmd
	// RETURN:
	//  0 = dump this connection.
	//  1 = keep processing.
	//  2 = process this.

	if ( sText.GetLength() >= SCRIPT_MAX_LINE_LEN )
	{
commandtoolong:
		SysMessage( "Command too long\n" );

		sText.Empty();
		return( 0 );
	}

	if ( nChar == '\r' || nChar == '\n' )
	{
		// Ignore the character if we have no text stored
		if (!sText.GetLength())
			return 1;

		if ( fEcho )
		{
			SysMessage("\n");
		}
		return 2;
	}
	else if ( nChar == 9 )			// TAB (auto-completion)
	{
		LPCTSTR p = NULL;
		LPCTSTR tmp = NULL;
		size_t inputLen = 0;
		bool matched(false);

		//	extract up to start of the word
		p = sText.GetPtr() + sText.GetLength();
		while (( p >= sText.GetPtr() ) && ( *p != '.' ) && ( *p != ' ' ) && ( *p != '/' ) && ( *p != '=' )) p--;
		p++;
		inputLen = strlen(p);

		// search in the auto-complete list for starting on P, and save coords of 1st and Last matched
		CGStringListRec	*firstmatch = NULL;
		CGStringListRec	*lastmatch = NULL;
		CGStringListRec	*curmatch = NULL;	// the one that should be set
		for ( curmatch = g_AutoComplete.GetHead(); curmatch != NULL; curmatch = curmatch->GetNext() )
		{
			if ( !strnicmp(curmatch->GetPtr(), p, inputLen) )	// matched
			{
				if ( firstmatch == NULL ) firstmatch = lastmatch = curmatch;
				else lastmatch = curmatch;
			}
			else if ( lastmatch ) break;					// if no longer matches - save time by instant quit
		}

		if (( firstmatch != NULL ) && ( firstmatch == lastmatch ))	// there IS a match and the ONLY
		{
			tmp = firstmatch->GetPtr() + inputLen;
			matched = true;
		}
		else if ( firstmatch != NULL )						// also make SE (if SERV/SERVER in dic) to become SERV
		{
			p = tmp = firstmatch->GetPtr();
			tmp += inputLen;
			inputLen = strlen(p);
			matched = true;
			for ( curmatch = firstmatch->GetNext(); curmatch != lastmatch->GetNext(); curmatch = curmatch->GetNext() )
			{
				if (strnicmp(curmatch->GetPtr(), p, inputLen) != 0)	// mismatched
				{
					matched = false;
					break;
				}
			}
		}

		if ( matched )
		{
			if ( fEcho )
				SysMessage(tmp);

			sText += tmp;
			if ( sText.GetLength() > SCRIPT_MAX_LINE_LEN )
				goto commandtoolong;
		}
		return 1;
	}

	if ( fEcho )
	{
		// Echo
		TCHAR szTmp[2];
		szTmp[0] = nChar;
		szTmp[1] = '\0';
		SysMessage( szTmp );
	}

	if ( nChar == 8 )
	{
		if ( sText.GetLength())	// back key
		{
			sText.SetLength( sText.GetLength() - 1 );
		}
		return 1;
	}

	sText += nChar;
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// -CServer

CServer::CServer() : CServerDef( GRAY_TITLE, CSocketAddressIP( SOCKET_LOCAL_ADDRESS ))
{
	m_iExitFlag = 0;
	m_fResyncPause = false;
	m_fResyncRequested = NULL;
	m_fResyncMultiRegions = false;

	m_iAdminClients = 0;

	m_timeShutdown.Init();

	m_fConsoleTextReadyFlag = false;

	// we are in start up mode. // IsLoading()
	SetServerMode( SERVMODE_Loading );

	memset(m_PacketFilter, 0, sizeof(m_PacketFilter));
	memset(m_OutPacketFilter, 0, sizeof(m_OutPacketFilter));
}

CServer::~CServer()
{
}

void CServer::SetSignals( bool fMsg )
{
	// We have just started or we changed Secure mode.

	// set_terminate(  &Exception_Terminate );
	// set_unexpected( &Exception_Unexpected );

	SetUnixSignals(g_Cfg.m_fSecure);
#ifndef _WIN32
	if ( g_Cfg.m_fSecure )
	{
		g_Log.Event( (IsLoading() ? 0 : LOGL_EVENT) | LOGM_INIT, "Signal handlers installed.\n" );
	}
	else
	{
		g_Log.Event( (IsLoading() ? 0 : LOGL_EVENT) | LOGM_INIT, "Signal handlers UNinstalled.\n" );
	}
#endif

	if ( fMsg && !IsLoading() )
	{
		g_World.Broadcast( g_Cfg.m_fSecure ?
			"The world is now running in SECURE MODE" :
			"WARNING: The world is NOT running in SECURE MODE" );
	}
}

void CServer::SetServerMode( SERVMODE_TYPE mode )
{
	ADDTOCALLSTACK("CServer::SetServerMode");
	m_iModeCode = mode;
#ifdef _WIN32
	NTWindow_SetWindowTitle();
#endif
}

bool CServer::IsValidBusy() const
{
	// We might appear to be stopped but it's really ok ?
	// ?
	switch ( m_iModeCode )
	{
		case SERVMODE_Saving:
			if ( g_World.IsSaving())
				return true;
			break;
		case SERVMODE_Loading:
		case SERVMODE_RestockAll:	// these may look stuck but are not.
			return( true );
		default:
			return( false );
	}
	return( false );
}

void CServer::SetExitFlag( int iFlag )
{
	ADDTOCALLSTACK("CServer::SetExitFlag");
	if ( m_iExitFlag )
		return;
	m_iExitFlag = iFlag;
}

void CServer::Shutdown( INT64 iMinutes ) // If shutdown is initialized
{
	ADDTOCALLSTACK("CServer::Shutdown");
	if ( iMinutes == 0 )
	{
		if ( ! m_timeShutdown.IsTimeValid() )
			return;
		m_timeShutdown.Init();
		g_World.Broadcast( g_Cfg.GetDefaultMsg( DEFMSG_MSG_SERV_SHUTDOWN_CANCEL ) );
		return;
	}

	if ( iMinutes < 0 )
	{
		iMinutes = g_World.GetTimeDiff( m_timeShutdown ) / ( 60 * TICK_PER_SEC );
	}
	else
	{
		m_timeShutdown = CServTime::GetCurrentTime() + ( iMinutes * 60 * TICK_PER_SEC );
	}

	g_World.Broadcastf(g_Cfg.GetDefaultMsg( DEFMSG_MSG_SERV_SHUTDOWN ), iMinutes);
}

void CServer::SysMessage( LPCTSTR pszMsg ) const
{
	// Print just to the main console.
	if ( !pszMsg || ISINTRESOURCE(pszMsg) )
		return;

#ifdef _WIN32
	NTWindow_PostMsg(pszMsg);
#else
	g_UnixTerminal.print(pszMsg);
#endif
}

void CServer::PrintTelnet( LPCTSTR pszMsg ) const
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

void CServer::PrintStr( LPCTSTR pszMsg ) const
{
	// print to all consoles.
	SysMessage( pszMsg );
	PrintTelnet( pszMsg );
}

int CServer::PrintPercent( long iCount, long iTotal )
{
	ADDTOCALLSTACK("CServer::PrintPercent");
	// These vals can get very large. so use MulDiv to prevent overflow. (not IMULDIV)
	if ( iTotal <= 0 )
		return 100;

    //int iPercent = MulDiv( iCount, 100, iTotal );
	int iPercent = IMULDIV( iCount, 100, iTotal );
	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "%d%%", iPercent);
	size_t len = strlen(pszTemp);

	PrintTelnet(pszTemp);

#ifndef _WIN32
	if ( g_UnixTerminal.isColorEnabled() )
	{
		SysMessage(pszTemp);
#endif
		while ( len > 0) // backspace it
		{
			PrintTelnet("\x08");
#ifndef _WIN32
 			SysMessage("\x08");
#endif
			len--;
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

INT64 CServer::GetAgeHours() const
{
	ADDTOCALLSTACK("CServer::GetAgeHours");
	return( CServTime::GetCurrentTime().GetTimeRaw() / (60*60*TICK_PER_SEC));
}

LPCTSTR CServer::GetStatusString( BYTE iIndex ) const
{
	ADDTOCALLSTACK("CServer::GetStatusString");
	// NOTE: The key names should match those in CServerDef::r_LoadVal
	// A ping will return this as well.
	// 0 or 0x21 = main status.

	TCHAR * pTemp = Str_GetTemp();
	DWORD iClients = StatGet(SERV_STAT_CLIENTS);
	INT64 iHours = GetAgeHours() / 24;

	switch ( iIndex )
	{
		case 0x21:	// '!'
			// typical (first time) poll response.
			{
				TCHAR szVersion[128];
				sprintf(pTemp, GRAY_TITLE ", Name=%s, Port=%d, Ver=" GRAY_VERSION ", TZ=%d, EMail=%s, URL=%s, Lang=%s, CliVer=%s\n",
					GetName(), m_ip.GetPort(), m_TimeZone, static_cast<LPCTSTR>(m_sEMail), static_cast<LPCTSTR>(m_sURL), static_cast<LPCTSTR>(m_sLang),
					m_ClientVersion.WriteClientVerString(m_ClientVersion.GetClientVer(), szVersion));
			}
			break;
		case 0x22: // '"'
			{
			// shown in the INFO page in game.
			sprintf(pTemp, GRAY_TITLE ", Name=%s, Age=%lld, Clients=%lu, Items=%lu, Chars=%lu, Mem=%luK\n",
				GetName(), iHours, iClients, StatGet(SERV_STAT_ITEMS), StatGet(SERV_STAT_CHARS), StatGet(SERV_STAT_MEM));
			}
			break;
		case 0x24: // '$'
			// show at startup.
			sprintf(pTemp, "Admin=%s, URL=%s, Lang=%s, TZ=%d\n", static_cast<LPCTSTR>(m_sEMail), static_cast<LPCTSTR>(m_sURL), static_cast<LPCTSTR>(m_sLang), m_TimeZone);
			break;
		case 0x25: // '%'
			// ConnectUO Status string
			sprintf(pTemp, GRAY_TITLE " Items=%lu, Mobiles=%lu, Clients=%lu, Mem=%lu", StatGet(SERV_STAT_ITEMS), StatGet(SERV_STAT_CHARS), iClients, StatGet(SERV_STAT_MEM));
			break;
	}

	return( pTemp );
}

//*********************************************************

void CServer::ListClients( CTextConsole *pConsole ) const
{
	ADDTOCALLSTACK("CServer::ListClients");
	// Mask which clients we want ?
	// Give a format of what info we want to SHOW ?
	if ( pConsole == NULL )
		return;

	const CChar *pCharCmd = pConsole->GetChar();
	const CChar *pChar = NULL;
	const CAccount *pAcc = NULL;
	TCHAR *pszMsg = Str_GetTemp();
	TCHAR *tmpMsg = Str_GetTemp();
	TCHAR chRank;
	LPCTSTR pszState = NULL;
	size_t numClients = 0;

	ClientIterator it;
	for ( const CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		numClients++;
		pChar = pClient->GetChar();
		pAcc = pClient->m_pAccount;
		chRank = (pAcc && pAcc->GetPrivLevel() > PLEVEL_Player) ? '+' : '=';

		if ( pChar )
		{
			if ( pCharCmd && !pCharCmd->CanDisturb(pChar) )
				continue;

			sprintf(tmpMsg, "%lx:Acc%c'%s', Char='%s' (IP: %s)\n", pClient->GetSocketID(), chRank, pAcc ? pAcc->GetName() : "<NA>", pChar->GetName(), pClient->GetPeerStr());
		}
		else
		{
			if ( pConsole->GetPrivLevel() < pClient->GetPrivLevel() )
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

			sprintf(tmpMsg, "%lx:Acc%c'%s' (IP: %s) %s\n", pClient->GetSocketID(), chRank, pAcc ? pAcc->GetName() : "<NA>", pClient->GetPeerStr(), pszState);
		}

		// If we have many clients, SCRIPT_MAX_LINE_LEN may be too short ;) (matex)
		if ( strlen(pszMsg) + strlen(tmpMsg) >= SCRIPT_MAX_LINE_LEN )
		{
			pConsole->SysMessage(pszMsg);
			pszMsg[0] = '\0';
		}
		ASSERT((strlen(pszMsg) + strlen(tmpMsg)) < SCRIPT_MAX_LINE_LEN);
		strcat(pszMsg, tmpMsg);
	}

	if ( numClients <= 0 )
		sprintf(tmpMsg, "%s\n", g_Cfg.GetDefaultMsg(DEFMSG_HL_NO_CLIENT));
	else if ( numClients == 1 )
		sprintf(tmpMsg, "%s\n", g_Cfg.GetDefaultMsg(DEFMSG_HL_ONE_CLIENT));
	else
		sprintf(tmpMsg, "%s %" FMTSIZE_T "\n", g_Cfg.GetDefaultMsg(DEFMSG_HL_MANY_CLIENTS), numClients);

	pConsole->SysMessage(pszMsg);
	pConsole->SysMessage(tmpMsg);
}

bool CServer::OnConsoleCmd( CGString & sText, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CServer::OnConsoleCmd");
	// RETURN: false = unsuccessful command.
	int	len = sText.GetLength();

	// We can't have a command with no length
	if ( len <= 0 )
		return true;

	// Convert first character to lowercase
	TCHAR low = static_cast<TCHAR>(tolower(sText[0]));
	bool fRet = true;

	if ((( len > 2 ) || (( len == 2 ) && ( sText[1] != '#' ))) && ( sText[0] != 'd' ))
		goto longcommand;

	switch ( low )
	{
		case '?':
			pSrc->SysMessagef(
				"Available commands:\n"
				"#         Immediate save world (## to save both world and statics)\n"
				"A         Update pending changes on Accounts file\n"
				"B [msg]   Broadcast message to all clients\n"
				"C         List of online Clients (%lu)\n"
				"DA        Dump Areas to external file\n"
				"DUI       Dump Unscripted Items to external file\n"
				"E         Clear internal variables (like script profile)\n"
				"G         Garbage collection\n"
				"H         Hear all that is said (%s)\n"
				"I         View server Information\n"
				"L         Toggle log file (%s)\n"
				"P         Profile Info (%s) (P# to dump to profiler_dump.txt)\n"
				"R         Resync Pause\n"
				"S         Toggle Secure mode (%s)\n"
				"STRIP     Dump all script templates to external file\n"
				"T         List of active Threads\n"
				"U         List used triggers\n"
				"X         Immediate exit the server (X# to save world and statics before exit)\n"
				,
				StatGet(SERV_STAT_CLIENTS),
				g_Log.IsLoggedMask( LOGM_PLAYER_SPEAK ) ? "ON" : "OFF",
				g_Log.IsFileOpen() ? "OPEN" : "CLOSED",
				CurrentProfileData.IsActive() ? "ON" : "OFF",
				g_Cfg.m_fSecure ? "ON" : "OFF"
				);
			break;
		case '#':	//	# - save world, ## - save both world and statics
			{		// Start a syncronous save or finish a background save synchronously
				if ( g_Serv.m_fResyncPause )
					goto do_resync;

				g_World.Save(true);
				if (( len > 1 ) && ( sText[1] == '#' ))
					g_World.SaveStatics();	// ## means
			} break;
		case 'a':
			{
				// Force periodic stuff
				g_Accounts.Account_SaveAll();
				g_Cfg.OnTick(true);
			} break;
		case 'c':	// List all clients on line.
			{
				ListClients( pSrc );
			} break;
		case 'd': // dump
			{
				LPCTSTR pszKey = sText + 1;
				GETNONWHITESPACE( pszKey );
				switch ( tolower(*pszKey) )
				{
					case 'a': // areas
						pszKey++;	GETNONWHITESPACE( pszKey );
						if ( !g_World.DumpAreas( pSrc, pszKey ) )
						{
							pSrc->SysMessage( "Area dump failed.\n" );
							fRet = false;
						}
						else
							pSrc->SysMessage( "Area dump successful.\n" );
						break;

					case 'u': // unscripted
						pszKey++;

						switch ( tolower(*pszKey) )
						{
							case 'i': // items
							{
								pszKey++;	GETNONWHITESPACE( pszKey );
								if ( !g_Cfg.DumpUnscriptedItems( pSrc, pszKey ) )
								{
									pSrc->SysMessage( "Unscripted item dump failed.\n" );
									fRet = false;
								}
								else
									pSrc->SysMessage( "Unscripted item dump successful.\n" );
								break;
							}

							default:
								goto longcommand;
						}
						break;

					default:
						goto longcommand;
				}
			} break;
		case 'e':
			{
				if ( IsSetEF(EF_Script_Profiler) && (g_profiler.initstate == 0xf1) )
				{
					TScriptProfiler::TScriptProfilerFunction *pFun;
					TScriptProfiler::TScriptProfilerTrigger *pTrig;

					for ( pFun = g_profiler.FunctionsHead; pFun != NULL; pFun = pFun->next )
						pFun->average = pFun->max = pFun->min = pFun->total = pFun->called = 0;

					for ( pTrig = g_profiler.TriggersHead; pTrig != NULL; pTrig = pTrig->next )
						pTrig->average = pTrig->max = pTrig->min = pTrig->total = pTrig->called = 0;

					g_profiler.total = g_profiler.called = 0;
					pSrc->SysMessage("Scripts profiler info cleared.\n");
				}
				else
				{
					pSrc->SysMessage("Script profiler feature is not enabled on " GRAY_FILE ".ini.\n");
				}
			} break;
		case 'g':
			{
				if ( g_Serv.m_fResyncPause )
				{
	do_resync:
					pSrc->SysMessage("Not allowed during resync pause. Use 'R' to restart.\n");
					fRet = false;
					break;
				}
				if ( g_World.IsSaving() )
				{
	do_saving:
					pSrc->SysMessage("Not allowed during background worldsave. Use '#' to finish.\n");
					fRet = false;
					break;
				}
				g_World.GarbageCollection();
			} break;
		case 'h':	// Hear all said.
			{
				CScript script( "HEARALL" );
				fRet = r_Verb( script, pSrc );
			} break;
		case 'i':
			{
				CScript script( "INFORMATION" );
				fRet = r_Verb( script, pSrc );
			} break;
		case 'l': // Turn the log file on or off.
			{
				CScript script( "LOG" );
				fRet = r_Verb( script, pSrc );
			} break;
		case 'p':	// Display profile information.
			{
				ProfileDump(pSrc, ( ( len > 1 ) && ( sText[1] == '#' ) ));
			} break;
		case 'r':	// resync Pause mode. Allows resync of things in files.
			{
				if ( pSrc != this ) // not from console
				{
					pSrc->SysMessage("Not allowed to use 'r' command via Telnet. Use 'resync' instead.\n");
				} 
				else 
				{
					if ( !m_fResyncPause && g_World.IsSaving() )
						goto do_saving;
					SetResyncPause(!m_fResyncPause, pSrc, true);
				}
			} break;
		case 's':
			{
				CScript script( "SECURE" );
				fRet = r_Verb( script, pSrc );
			} break;
		case 't':
			{
				pSrc->SysMessagef("Current active threads: %" FMTSIZE_T "\n", ThreadHolder::getActiveThreads());
				size_t iThreadCount = ThreadHolder::getActiveThreads();
				for ( size_t iThreads = 0; iThreads < iThreadCount; ++iThreads )
				{
					IThread *thrCurrent = ThreadHolder::getThreadAt(iThreads);
					if ( thrCurrent )
						pSrc->SysMessagef("Thread %" FMTSIZE_T ": %s (ID=%u, Priority=%d)\n", iThreads + 1, thrCurrent->getName(), thrCurrent->getId(), thrCurrent->getPriority());
				}
			} break;
		case 'u':
			TriglistPrint();
			break;
		case 'x':
			{
				bool bSave = ((len > 1) && (sText[1] == '#'));
				if ( g_Cfg.m_fSecure && !bSave )
				{
					pSrc->SysMessage("NOTE: Secure mode prevents keyboard exit!\n");
					fRet = false;
				}
				else
				{
					if ( bSave )
					{
						if ( g_Serv.m_fResyncPause )
							goto do_resync;

						g_World.Save(true);
						g_World.SaveStatics();
					}
					g_Log.Event(LOGL_FATAL, "Immediate Shutdown initialized!\n");
					SetExitFlag(1);
				}
			} break;
#ifdef _TESTEXCEPTION
		case '$':	// call stack integrity
			{
#ifdef EXCEPTIONS_DEBUG
				{ // test without PAUSECALLSTACK
					EXC_TRY("Test1");
					ADDTOCALLSTACK("CServer::TestException1");
					throw CGrayError( LOGM_DEBUG, 0, "Test Exception #1");
					}
					catch (const CGrayError& e)
					{
						// the following call will destroy the stack trace on linux due to
						// a call to CGFile::Close fromn CLog::EventStr.
						g_Log.Event( LOGM_DEBUG, "Caught exception\n" );
						EXC_CATCH_EXCEPTION(&e);
					}
				}

				{ // test with PAUSECALLSTACK
					EXC_TRY("Test2");
					ADDTOCALLSTACK("CServer::TestException2");
					throw CGrayError( LOGM_DEBUG, 0, "Test Exception #2");
					}
					catch (const CGrayError& e)
					{
						PAUSECALLSTACK;
						// with pausecallstack, the following call won't be recorded
						g_Log.Event( LOGM_DEBUG, "Caught exception\n" );
						UNPAUSECALLSTACK;
						EXC_CATCH_EXCEPTION(&e);
					}
				}
#else
				throw CGrayError(LOGL_CRIT, E_FAIL, "This test requires exception debugging enabled");
#endif
			} break;
		case '%':	// throw simple exception
			{
				throw 0;
			}
		case '^':	// throw language exception (div 0)
			{
				int i = 5;
				int j = 0;
				i /= j;
			}
		case '&':	// throw NULL pointer exception
			{
				CChar *character = NULL;
				character->SetName("yo!");
			}
		case '*':	// throw custom exception
			{
				throw CGrayError(LOGL_CRIT, E_FAIL, "Test Exception");
			}
#endif
		default:
			goto longcommand;
	}
	goto endconsole;

longcommand:
	if ( ((len > 1) && (sText[1] != ' ')) || (low == 'b') )
	{
		LPCTSTR	pszText = sText;

		if ( !strnicmp(pszText, "strip", 5) )
		{
			if ( g_Cfg.m_sStripPath.IsEmpty() )
			{
				pSrc->SysMessage("StripPath not defined on " GRAY_FILE ".ini, function aborted.\n");
				return false;
			}

			FILE *stripFile, *scriptFile;
			char *x;
			char *y = Str_GetTemp();
			char *z = Str_GetTemp();

			strcpy(z, g_Cfg.m_sStripPath);
			strcat(z, "sphere_strip" GRAY_SCRIPT);
			pSrc->SysMessagef("StripPath is %s\n", z);

			stripFile = fopen(z, "w");
			if ( !stripFile )
			{
				pSrc->SysMessagef("Cannot open file %s for writing.\n", z);
				return false;
			}

			size_t i = 0;
			CResourceScript	*script;
			while ( (script = g_Cfg.GetResourceFile(i++)) != NULL )
			{
				strcpy(z, script->GetFilePath());
				scriptFile = fopen(z, "r");
				if ( !scriptFile )
				{
					pSrc->SysMessagef("Cannot open file %s for reading.\n", z);
					continue;
				}

				while ( !feof(scriptFile) )
				{
					z[0] = 0;
					y[0] = 0;
					fgets(y, SCRIPT_MAX_LINE_LEN, scriptFile);

					x = y;
					GETNONWHITESPACE(x);
					strcpy(z, x);

					if ( ((z[0] == '[') && strnicmp(z, "[EOF]", 5) != 0) || !strnicmp(z, "DEFNAME", 7) || !strnicmp(z, "NAME", 4) ||
						!strnicmp(z, "ID", 2) || !strnicmp(z, "TYPE", 4) || !strnicmp(z, "WEIGHT", 6) || !strnicmp(z, "VALUE", 5) ||
						!strnicmp(z, "DAM", 3) || !strnicmp(z, "ARMOR", 5) || !strnicmp(z, "SKILLMAKE", 9) || !strnicmp(z, "RESOURCES", 9) ||
						!strnicmp(z, "DUPEITEM", 8) || !strnicmp(z, "DUPELIST", 8) || !strnicmp(z, "CAN", 3) || !strnicmp(z, "TEVENTS", 7) ||
						!strnicmp(z, "CATEGORY", 8) || !strnicmp(z, "SUBSECTION", 10) || !strnicmp(z, "DESCRIPTION", 11) || !strnicmp(z, "COLOR", 5) ||
						!strnicmp(z, "GROUP", 5) || !strnicmp(z, "P=", 2) || !strnicmp(z, "RECT=", 5) || !strnicmp(z, "ON=@", 4) )
					{
						fputs(y, stripFile);
					}
				}
				fclose(scriptFile);
			}
			fclose(stripFile);
			pSrc->SysMessagef("Scripts have just been stripped.\n");
			return true;
		}

		if ( g_Cfg.IsConsoleCmd(low) )
			pszText++;

		CScript	script(pszText);
		if ( !g_Cfg.CanUsePrivVerb(this, pszText, pSrc) )
		{
			pSrc->SysMessagef("not privileged for command '%s'\n", pszText);
			fRet = false;
		}
		else if ( !r_Verb(script, pSrc) )
		{
			pSrc->SysMessagef("unknown command '%s'\n", pszText);
			fRet = false;
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

//************************************************

PLEVEL_TYPE CServer::GetPrivLevel() const
{
	return PLEVEL_Owner;
}

void CServer::ProfileDump( CTextConsole *pSrc, bool bDump )
{
	ADDTOCALLSTACK("CServer::ProfileDump");
	if ( !pSrc )
		return;

	CFileText *ftDump = NULL;
	if ( bDump )
	{
		ftDump = new CFileText();
		if ( !ftDump->Open("profiler_dump.txt", OF_CREATE|OF_TEXT) )
		{
			delete ftDump;
			ftDump = NULL;
		}
	}

	if ( ftDump )
		ftDump->Printf("Profiles %s: (%d sec total)\n", CurrentProfileData.IsActive() ? "ON" : "OFF", CurrentProfileData.GetActiveWindow());
	else
		pSrc->SysMessagef("Profiles %s: (%d sec total)\n", CurrentProfileData.IsActive() ? "ON" : "OFF", CurrentProfileData.GetActiveWindow());

	if ( IsSetEF(EF_Script_Profiler) )
	{
		size_t iThreadCount = ThreadHolder::getActiveThreads();
		for ( size_t iThreads = 0; iThreads < iThreadCount; ++iThreads )
		{
			IThread *thrCurrent = ThreadHolder::getThreadAt(iThreads);
			if ( thrCurrent == NULL )
				continue;

			const ProfileData &profile = static_cast<AbstractSphereThread *>(thrCurrent)->m_profile;
			if ( profile.IsEnabled() == false )
				continue;

			if ( ftDump )
				ftDump->Printf("Thread %u, Name=%s\n", thrCurrent->getId(), thrCurrent->getName());
			else
				pSrc->SysMessagef("Thread %u, Name=%s\n", thrCurrent->getId(), thrCurrent->getName());

			for ( int i = 0; i < PROFILE_QTY; i++ )
			{
				if ( profile.IsEnabled(static_cast<PROFILE_TYPE>(i)) == false )
					continue;

				if ( ftDump )
					ftDump->Printf("%-10s = %s\n", profile.GetName(static_cast<PROFILE_TYPE>(i)), profile.GetDescription(static_cast<PROFILE_TYPE>(i)));
				else
					pSrc->SysMessagef("%-10s = %s\n", profile.GetName(static_cast<PROFILE_TYPE>(i)), profile.GetDescription(static_cast<PROFILE_TYPE>(i)));
			}
		}

		if ( g_profiler.initstate != 0xf1 )
			pSrc->SysMessagef("Scripts profiler is not initialized\n");
		else if ( !g_profiler.called )
			pSrc->SysMessagef("Script profiler is not yet informational\n");
		else
		{
			ULONGLONG average = g_profiler.total / g_profiler.called;
			TScriptProfiler::TScriptProfilerFunction *pFun;
			TScriptProfiler::TScriptProfilerTrigger *pTrig;
			ULONGLONG divby = llTimeProfileFrequency / 1000;

			if ( ftDump )
				ftDump->Printf("Scripts: called %lu times and took %i.%04i ms (%i.%04i ms average). Reporting with highest average.\n",
					g_profiler.called,
					static_cast<int>(g_profiler.total / divby),
					static_cast<int>(((g_profiler.total * 10000) / divby) % 10000),
					static_cast<int>(average / divby),
					static_cast<int>(((average * 10000) / divby) % 10000)
				);
			else
				pSrc->SysMessagef("Scripts: called %lu times and took %i.%04i ms (%i.%04i ms average). Reporting with highest average.\n",
					g_profiler.called,
					static_cast<int>(g_profiler.total / divby),
					static_cast<int>(((g_profiler.total * 10000) / divby) % 10000),
					static_cast<int>(average / divby),
					static_cast<int>(((average * 10000) / divby) % 10000)
				);

			for ( pFun = g_profiler.FunctionsHead; pFun != NULL; pFun = pFun->next )
			{
				if ( pFun->average > average )
				{
					if ( ftDump )
						ftDump->Printf("FUNCTION '%-30s' called %6lu times (%6i.%04i min, %6i.%04i avg, %6i.%04i max) [total: %6i.%04i ms]\n",
							pFun->name,
							pFun->called,
							static_cast<int>(pFun->min / divby),
							static_cast<int>(((pFun->min * 10000) / divby) % 10000),
							static_cast<int>(pFun->average / divby),
							static_cast<int>(((pFun->average * 10000) / divby) % 10000),
							static_cast<int>(pFun->max / divby),
							static_cast<int>(((pFun->max * 10000) / divby) % 10000),
							static_cast<int>(pFun->total / divby),
							static_cast<int>(((pFun->total * 10000) / divby) % 10000)
						);
					else
						pSrc->SysMessagef("FUNCTION '%-30s' called %6lu times (%6i.%04i min, %6i.%04i avg, %6i.%04i max) [total: %6i.%04i ms]\n",
							pFun->name,
							pFun->called,
							static_cast<int>(pFun->min / divby),
							static_cast<int>(((pFun->min * 10000) / divby) % 10000),
							static_cast<int>(pFun->average / divby),
							static_cast<int>(((pFun->average * 10000) / divby) % 10000),
							static_cast<int>(pFun->max / divby),
							static_cast<int>(((pFun->max * 10000) / divby) % 10000),
							static_cast<int>(pFun->total / divby),
							static_cast<int>(((pFun->total * 10000) / divby) % 10000)
						);
				}
			}
			for ( pTrig = g_profiler.TriggersHead; pTrig != NULL; pTrig = pTrig->next )
			{
				if ( pTrig->average > average )
				{
					if ( ftDump )
						ftDump->Printf("TRIGGER '%-25s' called %6lu times (%6i.%04i min, %6i.%04i avg, %6i.%04i max), total: %6i.%04i ms\n",
							pTrig->name,
							pTrig->called,
							static_cast<int>(pTrig->min / divby),
							static_cast<int>(((pTrig->min * 10000) / divby) % 10000),
							static_cast<int>(pTrig->average / divby),
							static_cast<int>(((pTrig->average * 10000) / divby) % 10000),
							static_cast<int>(pTrig->max / divby),
							static_cast<int>(((pTrig->max * 10000) / divby) % 10000),
							static_cast<int>(pTrig->total / divby),
							static_cast<int>(((pTrig->total * 10000) / divby) % 10000)
						);
					else
						pSrc->SysMessagef("TRIGGER '%-25s' called %6lu times (%6i.%04i min, %6i.%04i avg, %6i.%04i max), total: %6i.%04i ms\n",
							pTrig->name,
							pTrig->called,
							static_cast<int>(pTrig->min / divby),
							static_cast<int>(((pTrig->min * 10000) / divby) % 10000),
							static_cast<int>(pTrig->average / divby),
							static_cast<int>(((pTrig->average * 10000) / divby) % 10000),
							static_cast<int>(pTrig->max / divby),
							static_cast<int>(((pTrig->max * 10000) / divby) % 10000),
							static_cast<int>(pTrig->total / divby),
							static_cast<int>(((pTrig->total * 10000) / divby) % 10000)
						);
				}
			}

			pSrc->SysMessage("Report complete!\n");
		}
	}
	else
	{
		pSrc->SysMessage("Script profiler feature is not enabled on " GRAY_FILE ".ini.\n");
	}

	if ( ftDump )
	{
		ftDump->Close();
		delete ftDump;
	}
}

// ---------------------------------------------------------------------

bool CServer::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CServer::r_GetRef");
	if ( IsDigit( pszKey[0] ))
	{
		size_t i = 1;
		while ( IsDigit( pszKey[i] ))
			i++;

		if ( pszKey[i] == '.' )
		{
			size_t index = ATOI( pszKey );	// must use this to stop at .
			pRef = g_Cfg.Server_GetDef(index);
			pszKey += i + 1;
			return( true );
		}
	}
	if ( g_Cfg.r_GetRef( pszKey, pRef ))
	{
		return( true );
	}
	if ( g_World.r_GetRef( pszKey, pRef ))
	{
		return( true );
	}
	return( CScriptObj::r_GetRef( pszKey, pRef ));
}

bool CServer::r_LoadVal( CScript &s )
{
	ADDTOCALLSTACK("CServer::r_LoadVal");

	if ( g_Cfg.r_LoadVal(s) )
		return true;
	if ( g_World.r_LoadVal(s) )
		return true;
	return CServerDef::r_LoadVal(s);
}

bool CServer::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CServer::r_WriteVal");
	if ( !strnicmp(pszKey, "ACCOUNT.", 8) )
	{
		pszKey += 8;
		CAccountRef pAccount = NULL;
		
		// extract account name/index to a temporary buffer
		TCHAR * pszTemp = Str_GetTemp();
		TCHAR * pszTempStart = pszTemp;

		strcpy(pszTemp, pszKey);
		TCHAR * split = strchr(pszTemp, '.');
		if ( split != NULL )
			*split = '\0';

		// adjust pszKey to point to end of account name/index
		pszKey += strlen(pszTemp);

		//	try to fetch using indexes
		if (( *pszTemp >= '0' ) && ( *pszTemp <= '9' ))
		{
			size_t num = Exp_GetVal(pszTemp);
			if (*pszTemp == '\0' && num < g_Accounts.Account_GetCount())
				pAccount = g_Accounts.Account_Get(num);
		}

		//	indexes failed, try to fetch using names
		if ( pAccount == NULL )
		{
			pszTemp = pszTempStart;
			pAccount = g_Accounts.Account_Find(pszTemp);
		}

		if ( !*pszKey) // we're just checking if the account exists
		{
			sVal.FormatVal( (pAccount? 1 : 0) );
			return true;
		}
		else if ( pAccount ) // we're retrieving a property from the account
		{
			SKIP_SEPARATORS(pszKey);
			return pAccount->r_WriteVal(pszKey, sVal, pSrc);
		}
		// we're trying to retrieve a property from an invalid account
		return false;
	}

	// Just do stats values for now.
	if ( g_Cfg.r_WriteVal(pszKey, sVal, pSrc) )
		return true;
	if ( g_World.r_WriteVal(pszKey, sVal, pSrc) )
		return true;
	return CServerDef::r_WriteVal(pszKey, sVal, pSrc);
}

enum SV_TYPE
{
	SV_ACCOUNT,
	SV_ACCOUNTS, //read only
	SV_ALLCLIENTS,
	SV_B,
	SV_BLOCKIP,
	SV_CHARS, //read only
	SV_CLEARLISTS,
	SV_CONSOLE,
#ifdef _TEST_EXCEPTION
	SV_CRASH,
#endif
	SV_EXPORT,
	SV_GARBAGE,
	SV_HEARALL,
	SV_IMPORT,
	SV_INFORMATION,
	SV_ITEMS, //read only
	SV_LOAD,
	SV_LOG,
	SV_PRINTLISTS,
	SV_RESPAWN,
	SV_RESTOCK,
	SV_RESTORE,
	SV_RESYNC,
	SV_SAVE,
	SV_SAVECOUNT, //read only
	SV_SAVESTATICS,
	SV_SECURE,
	SV_SHRINKMEM,
	SV_SHUTDOWN,
	SV_TIME, // read only
	SV_UNBLOCKIP,
	SV_VARLIST,
	SV_QTY
};

LPCTSTR const CServer::sm_szVerbKeys[SV_QTY+1] =
{
	"ACCOUNT",
	"ACCOUNTS", // read only
	"ALLCLIENTS",
	"B",
	"BLOCKIP",
	"CHARS", // read only
	"CLEARLISTS",
	"CONSOLE",
#ifdef _TEST_EXCEPTION
	"CRASH",
#endif
	"EXPORT",
	"GARBAGE",
	"HEARALL",
	"IMPORT",
	"INFORMATION",
	"ITEMS", // read only
	"LOAD",
	"LOG",
	"PRINTLISTS",
	"RESPAWN",
	"RESTOCK",
	"RESTORE",
	"RESYNC",
	"SAVE",
	"SAVECOUNT", // read only
	"SAVESTATICS",
	"SECURE",
	"SHRINKMEM",
	"SHUTDOWN",
	"TIME", // read only
	"UNBLOCKIP",
	"VARLIST",
	NULL
};

bool CServer::r_Verb( CScript &s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CServer::r_Verb");
	if ( !pSrc )
		return false;

	EXC_TRY("Verb");
	LPCTSTR pszKey = s.GetKey();
	TCHAR *pszMsg = NULL;

	int index = FindTableSorted( s.GetKey(), sm_szVerbKeys, COUNTOF( sm_szVerbKeys )-1 );

	if ( index < 0 )
	{
		CGString sVal;
		CScriptTriggerArgs Args( s.GetArgRaw() );
		if ( r_Call( pszKey, pSrc, &Args, &sVal ) )
			return true;

		if ( !strnicmp(pszKey, "ACCOUNT.", 8) )
		{
			index = SV_ACCOUNT;
			pszKey += 8;

			CAccountRef pAccount = NULL;
			char *pszTemp = Str_GetTemp();

			strcpy(pszTemp, pszKey);
			char *split = strchr(pszTemp, '.');
			if ( split )
			{
				*split = 0;
				pAccount = g_Accounts.Account_Find(pszTemp);
				pszKey += strlen(pszTemp);
			}

			SKIP_SEPARATORS(pszKey);
			if ( pAccount && *pszKey )
			{
				CScript script(pszKey, s.GetArgStr());
				return pAccount->r_LoadVal(script);
			}
			return false;
		}

		if ( !strnicmp(pszKey, "CLEARVARS", 9) )
		{
			pszKey = s.GetArgStr();
			SKIP_SEPARATORS(pszKey);
			g_Exp.m_VarGlobals.ClearKeys(pszKey);
			return true;
		}
	}

	switch (index)
	{
		case SV_ACCOUNTS:
		case SV_CHARS:
		case SV_ITEMS:
		case SV_SAVECOUNT:
		case SV_TIME:
			return false;
		case SV_ACCOUNT: // "ACCOUNT"
			return g_Accounts.Account_OnCmd(s.GetArgRaw(), pSrc);

		case SV_ALLCLIENTS:	// "ALLCLIENTS"
			{
				// Send a verb to all clients
				ClientIterator it;
				for (CClient* pClient = it.next(); pClient != NULL; pClient = it.next())
				{
					if ( pClient->GetChar() == NULL )
						continue;
					CScript script( s.GetArgStr() );
					pClient->GetChar()->r_Verb( script, pSrc );
				}
			}
			break;

		case SV_B: // "B"
			g_World.Broadcast( s.GetArgStr());
			break;

		case SV_BLOCKIP:
			if ( pSrc->GetPrivLevel() >= PLEVEL_Admin )
			{
				int iTimeDecay(-1);

				TCHAR* ppArgs[2];
				if (Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), ", ") == false)
					return false;

				if (ppArgs[1])
					iTimeDecay = Exp_GetVal(ppArgs[1]);
				
#ifndef _MTNETWORK
				HistoryIP& history = g_NetworkIn.getIPHistoryManager().getHistoryForIP(ppArgs[0]);
#else
				HistoryIP& history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(ppArgs[0]);
#endif

				if (iTimeDecay >= 0)
					pSrc->SysMessagef("IP blocked for %d seconds\n", iTimeDecay);
				else
					pSrc->SysMessagef("IP%s blocked\n", history.m_blocked ? " already" : "");

				history.setBlocked(true, iTimeDecay);
			}
			break;

		case SV_CONSOLE:
			{
				CGString z = s.GetArgRaw();
				OnConsoleCmd(z, pSrc);
			}
			break;

#ifdef _TEST_EXCEPTION
		case SV_CRASH:
			{
				g_Log.Event(0, "Testing exceptions.\n");
				pSrc = NULL;
				pSrc->GetChar();
			} break;
#endif

		case SV_EXPORT: // "EXPORT" name [chars] [area distance]
			if ( pSrc->GetPrivLevel() < PLEVEL_Admin )
				return( false );
			if ( s.HasArgs())
			{
				TCHAR * Arg_ppCmd[5];
				size_t Arg_Qty = Str_ParseCmds( s.GetArgRaw(), Arg_ppCmd, COUNTOF( Arg_ppCmd ));
				if ( Arg_Qty <= 0 )
					break;
				// IMPFLAGS_ITEMS
				if ( ! g_World.Export( Arg_ppCmd[0], pSrc->GetChar(),
					(Arg_Qty >= 2) ? static_cast<WORD>(ATOI(Arg_ppCmd[1])) : IMPFLAGS_ITEMS,
					(Arg_Qty >= 3)? ATOI(Arg_ppCmd[2]) : SHRT_MAX ))
				{
					pSrc->SysMessage( "Export failed\n" );
				}
			}
			else
			{
				pSrc->SysMessage( "EXPORT name [flags] [area distance]" );
			}
			break;
		case SV_GARBAGE:
			if ( g_Serv.m_fResyncPause || g_World.IsSaving() )
			{
				pSrc->SysMessage( "Not allowed during world save and/or resync pause" );
				break;
			}
			else
			{
				g_World.GarbageCollection();
			}
			break;

		case SV_HEARALL:	// "HEARALL" = Hear all said.
			{
				pszMsg = Str_GetTemp();
				g_Log.SetLogMask(static_cast<DWORD>(s.GetArgFlag(g_Log.GetLogMask(), LOGM_PLAYER_SPEAK)));
				sprintf(pszMsg, "Hear All %s.\n", g_Log.IsLoggedMask(LOGM_PLAYER_SPEAK) ? "Enabled" : "Disabled" );
			}
			break;

		case SV_INFORMATION:
			pSrc->SysMessage( GetStatusString( 0x22 ));
			pSrc->SysMessage( GetStatusString( 0x24 ));
			break;

		case SV_IMPORT: // "IMPORT" name [flags] [area distance]
			if ( pSrc->GetPrivLevel() < PLEVEL_Admin )
				return( false );
			if (s.HasArgs())
			{
				TCHAR * Arg_ppCmd[5];
				size_t Arg_Qty = Str_ParseCmds( s.GetArgRaw(), Arg_ppCmd, COUNTOF( Arg_ppCmd ));
				if ( Arg_Qty <= 0 )
				{
					break;
				}
				// IMPFLAGS_ITEMS
				if ( ! g_World.Import( Arg_ppCmd[0], pSrc->GetChar(),
					(Arg_Qty >= 2) ? static_cast<WORD>(ATOI(Arg_ppCmd[1])) : IMPFLAGS_BOTH,
					(Arg_Qty>=3)?ATOI(Arg_ppCmd[2]) : SHRT_MAX ))
					pSrc->SysMessage( "Import failed\n" );
			}
			else
			{
				pSrc->SysMessage( "IMPORT name [flags] [area distance]" );
			}
			break;
		case SV_LOAD:
			// Load a resource file.
			if ( g_Cfg.LoadResourcesAdd( s.GetArgStr()) == NULL )
				return( false );
			return( true );

		case SV_LOG:
			{
				LPCTSTR	pszArgs = s.GetArgStr();
				int		mask = LOGL_EVENT;
				if ( pszArgs && ( *pszArgs == '@' ))
				{
					pszArgs++;
					if ( *pszArgs != '@' )
						mask |= LOGM_NOCONTEXT;
				}
				g_Log.Event(mask, "%s\n", pszArgs);
			}
			break;

		case SV_RESPAWN:
			g_World.RespawnDeadNPCs();
			break;

		case SV_RESTOCK:
			// set restock time of all vendors in World.
			// set the respawn time of all spawns in World.
			g_World.Restock();
			break;

		case SV_RESTORE:	// "RESTORE" backupfile.SCP Account CharName
			if ( pSrc->GetPrivLevel() < PLEVEL_Admin )
				return( false );
			if (s.HasArgs())
			{
				TCHAR * Arg_ppCmd[4];
				size_t Arg_Qty = Str_ParseCmds( s.GetArgRaw(), Arg_ppCmd, COUNTOF( Arg_ppCmd ));
				if ( Arg_Qty <= 0 )
				{
					break;
				}
				if ( ! g_World.Import( Arg_ppCmd[0], pSrc->GetChar(),
					IMPFLAGS_BOTH|IMPFLAGS_ACCOUNT, SHRT_MAX,
					Arg_ppCmd[1], Arg_ppCmd[2] ))
				{
					pSrc->SysMessage( "Restore failed\n" );
				}
				else
				{
					pSrc->SysMessage( "Restore success\n" );
				}
			}
			break;
		case SV_RESYNC:
			{
				// if a resync occurs whilst a script is executing then bad thing
				// will happen. Flag that a resync has been requested and it will
				// take place during a server tick
				m_fResyncRequested = pSrc;
				break;
			}
		case SV_SAVE: // "SAVE" x
			g_World.Save(s.GetArgVal() != 0);
			break;
		case SV_SAVESTATICS:
			g_World.SaveStatics();
			break;
		case SV_SECURE: // "SECURE"
			pszMsg = Str_GetTemp();
			g_Cfg.m_fSecure = (s.GetArgFlag(g_Cfg.m_fSecure, true) != 0);
			SetSignals();
			sprintf(pszMsg, "Secure mode %s.\n", g_Cfg.m_fSecure ? "enabled" : "disabled" );
			break;
		case SV_SHRINKMEM:
			{
#ifdef _WIN32
				if ( GRAY_GetOSInfo()->dwPlatformId != 2 )
				{
					g_Log.EventError( "Command not available on Windows 95/98/ME.\n" );
					return( false );
				}

				if ( !SetProcessWorkingSetSize(GetCurrentProcess(), static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1)) )
				{
					g_Log.EventError( "Error during process shrink.\n" );
					return false;
				}

				pSrc->SysMessage( "Memory shrinked succesfully.\n" );
#else
				g_Log.EventError( "Command not available on *NIX os.\n" );
				return false;
#endif
			} break;

		case SV_SHUTDOWN: // "SHUTDOWN"
			Shutdown(( s.HasArgs()) ? s.GetArgVal() : 15 );
			break;

		case SV_UNBLOCKIP:	// "UNBLOCKIP"
			if (pSrc->GetPrivLevel() >= PLEVEL_Admin)
			{
#ifndef _MTNETWORK
				HistoryIP& history = g_NetworkIn.getIPHistoryManager().getHistoryForIP(s.GetArgRaw());
#else
				HistoryIP& history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(s.GetArgRaw());
#endif
				pSrc->SysMessagef("IP%s unblocked\n", history.m_blocked ? "" : " already");
				history.setBlocked(false);
			}
			break;

		case SV_VARLIST:
			if ( ! strcmpi( s.GetArgStr(), "log" ))
				pSrc = &g_Serv;
			g_Exp.m_VarGlobals.DumpKeys(pSrc, "VAR.");
			break;
		case SV_PRINTLISTS:
			if ( ! strcmpi( s.GetArgStr(), "log" ))
				pSrc = &g_Serv;
			g_Exp.m_ListGlobals.DumpKeys(pSrc, "LIST.");
			break;
		case SV_CLEARLISTS:
			g_Exp.m_ListGlobals.ClearKeys(s.GetArgStr());
			break;
		default:
			return CScriptObj::r_Verb(s, pSrc);

	}

	if ( pszMsg && *pszMsg )
		pSrc->SysMessage(pszMsg);

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	g_Log.EventDebug("source '%s' char '%s' uid '0%lx'\n", (pSrc && pSrc->GetName()) ? pSrc->GetName() : "",
		(pSrc && pSrc->GetChar()) ? pSrc->GetChar()->GetName() : "",
		(pSrc && pSrc->GetChar()) ? (DWORD)pSrc->GetChar()->GetUID() : 0 );
	EXC_DEBUG_END;
	return false;
}

//*********************************************************

extern void defragSphere(char *);

bool CServer::CommandLine( int argc, TCHAR * argv[] )
{
	// Console Command line.
	// This runs after script file enum but before loading the world file.
	// RETURN:
	//  true = keep running after this.

	for ( int argn = 1; argn < argc; argn++ )
	{
		TCHAR * pArg = argv[argn];
		if ( ! _IS_SWITCH(pArg[0]))
			continue;

		pArg++;

		switch ( toupper(pArg[0]) )
		{
			case '?':
				PrintStr( GRAY_TITLE " \n"
					"Command Line Switches:\n"
#ifdef _WIN32
					"-cClassName Setup custom window class name for sphere (default: " GRAY_TITLE "Svr)\n"
#else
					"-c use colored console output (default: off)\n"
#endif
					"-D Dump global variable DEFNAMEs to defs.txt\n"
#if defined(_WIN32) && !defined(_DEBUG)
					"-E Enable the CrashDumper\n"
#endif
					"-Gpath/to/saves/ Defrags sphere saves\n"
#ifdef _WIN32
					"-k install/remove Installs or removes NT Service\n"
#endif
					"-Nstring Set the sphere name.\n"
					"-P# Set the port number.\n"
					"-Ofilename Output console to this file name\n"
					"-Q Quit when finished.\n"
					);
				return( false );
#ifdef _WIN32
			case 'C':
			case 'K':
				//	these are parsed in other places - nt service, nt window part, etc
				continue;
#else
			case 'C':
				g_UnixTerminal.setColorEnabled(true);
				continue;
#endif
			case 'P':
				m_ip.SetPort(static_cast<WORD>(ATOI(pArg + 1)));
				continue;
			case 'N':
				// Set the system name.
				SetName(pArg+1);
				continue;
			case 'D':
				// dump all the defines to a file.
				{
					CFileText File;
					if ( ! File.Open( "defs.txt", OF_WRITE|OF_TEXT ))
						return( false );

					for ( size_t i = 0; i < g_Exp.m_VarDefs.GetCount(); i++ )
					{
						if ( ( i % 0x1ff ) == 0 )
							PrintPercent( i, g_Exp.m_VarDefs.GetCount());

						CVarDefCont * pCont = g_Exp.m_VarDefs.GetAt(i);
						if ( pCont != NULL )
						{
							File.Printf( "%s=%s\n", pCont->GetKey(), pCont->GetValStr());
						}
					}
				}
				continue;
#if defined(_WIN32) && !defined(_DEBUG) && !defined(_NO_CRASHDUMP)
			case 'E':
				CrashDump::Enable();
				if (CrashDump::IsEnabled())
					PrintStr("Crash dump enabled\n");
				else
					PrintStr("Crash dump NOT enabled.\n");
				continue;
#endif
			case 'G':
				defragSphere(pArg + 1);
				continue;
			case 'O':
				if ( g_Log.Open(pArg+1, OF_SHARE_DENY_WRITE|OF_READWRITE|OF_TEXT) )
					g_Log.m_fLockOpen = true;
				continue;
			case 'Q':
				return false;
			default:
				g_Log.Event(LOGM_INIT|LOGL_CRIT, "Don't recognize command line data '%s'\n", static_cast<LPCTSTR>(argv[argn]));
				break;
		}
	}

	return( true );
}

void CServer::SetResyncPause(bool fPause, CTextConsole * pSrc, bool bMessage)
{
	ADDTOCALLSTACK("CServer::SetResyncPause");
	if ( fPause )
	{
		m_fResyncPause = true;
		g_Serv.SysMessagef("%s\n",g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_START));

		if ( bMessage )
			g_World.Broadcast(g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_START));
		else if ( pSrc && pSrc->GetChar() )
			pSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_START));

		g_Cfg.Unload(true);
		SetServerMode(SERVMODE_ResyncPause);
	}
	else
	{
		g_Serv.SysMessagef("%s\n",g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_RESTART));
		SetServerMode(SERVMODE_ResyncLoad);

		if ( !g_Cfg.Load(true) )
		{
			g_Serv.SysMessagef("%s\n",g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_FAILED));
			if ( bMessage )
				g_World.Broadcast(g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_FAILED));
			else if ( pSrc && pSrc->GetChar() )
				pSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_FAILED));
		}
		else
		{
			g_Serv.SysMessagef("%s\n",g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_SUCCESS));
			if ( bMessage )
				g_World.Broadcast(g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_SUCCESS));
			else if ( pSrc && pSrc->GetChar() )
				pSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_SUCCESS));
		}

		m_fResyncPause = false;
		SetServerMode(SERVMODE_Run);
	}
}

//*********************************************************

bool CServer::SocketsInit( CGSocket & socket )
{
	ADDTOCALLSTACK("CServer::SocketsInit");
	// Initialize socket
	if ( !socket.Create() )
	{
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "Unable to create socket!\n");
		return false;
	}

	linger lval;
	lval.l_onoff = 0;
	lval.l_linger = 10;
	socket.SetSockOpt(SO_LINGER, reinterpret_cast<const char *>(&lval), sizeof(lval));
	socket.SetNonBlocking();

#ifndef _WIN32
	int onNotOff = 1;
	if(socket.SetSockOpt(SO_REUSEADDR, (char *)&onNotOff , sizeof(onNotOff )) == -1) {
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "Unable to set SO_REUSEADDR!\n");
	}
#endif
	// Bind to just one specific port if they say so.
	CSocketAddress SockAddr = m_ip;
	// if ( fGod )
	//	SockAddr.SetPort(( g_Cfg.m_iUseGodPort > 1 ) ? g_Cfg.m_iUseGodPort : m_ip.GetPort()+1000);

	int iRet = socket.Bind(SockAddr);
	if ( iRet < 0 )			// Probably already a server running.
	{
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "Unable to bind listen socket %s port %d (error code: %i)\n", SockAddr.GetAddrStr(), SockAddr.GetPort(), iRet);
		return false;
	}
	socket.Listen();
	
#if !defined(_WIN32) || defined(_LIBEV)
#ifdef LIBEV_REGISTERMAIN
	if ( g_Cfg.m_fUseAsyncNetwork != 0 )
		g_NetworkEvent.registerMainsocket();
#endif
#endif
		
	return true;
}

bool CServer::SocketsInit() // Initialize sockets
{
	ADDTOCALLSTACK("CServer::SocketsInit");
	if ( !SocketsInit(m_SocketMain) )
		return false;

	// What are we listing our port as to the world.
	// Tell the admin what we know.

	TCHAR szName[ _MAX_PATH ];
	struct hostent * pHost = NULL;

	int iRet = gethostname(szName, sizeof(szName));
	if ( iRet )
		strcpy(szName, m_ip.GetAddrStr());
	else
	{
		pHost = gethostbyname(szName);
		if ( pHost && pHost->h_addr && pHost->h_name && pHost->h_name[0] )
			strcpy(szName, pHost->h_name);
	}

	g_Log.Event(LOGM_INIT, "\nServer started on hostname '%s'\n", szName);
	if ( !iRet && pHost && pHost->h_addr )
	{
		for ( size_t i = 0; pHost->h_addr_list[i] != NULL; i++ )
		{
			CSocketAddressIP ip;
			ip.SetAddrIP(*((DWORD*)(pHost->h_addr_list[i]))); // 0.1.2.3
			if ( !m_ip.IsLocalAddr() && !m_ip.IsSameIP(ip) )
				continue;
			g_Log.Event(LOGM_INIT, "Monitoring IP %s:%d (TCP) - Main server\n", ip.GetAddrStr(), m_ip.GetPort());
			if ( IsSetEF(EF_UsePingServer) )
				g_Log.Event(LOGM_INIT, "Monitoring IP %s:%d (UDP) - Ping server\n", ip.GetAddrStr(), PINGSERVER_PORT);
		}
	}
	return true;
}

void CServer::SocketsClose()
{
	ADDTOCALLSTACK("CServer::SocketsClose");
#if !defined(_WIN32) || defined(_LIBEV)
#ifdef LIBEV_REGISTERMAIN
	if ( g_Cfg.m_fUseAsyncNetwork != 0 )
		g_NetworkEvent.unregisterMainsocket();
#endif
#endif
	m_SocketMain.Close();
}

void CServer::OnTick()
{
	ADDTOCALLSTACK("CServer::OnTick");
	EXC_TRY("Tick");

#ifndef _WIN32
	if (g_UnixTerminal.isReady())
	{
		TCHAR c = g_UnixTerminal.read();
		if ( OnConsoleKey(m_sConsoleText, c, false) == 2 )
			m_fConsoleTextReadyFlag = true;
	}
#endif

	EXC_SET("ConsoleInput");
	if ( m_fConsoleTextReadyFlag )
	{
		EXC_SET("console input");
		CGString sText = m_sConsoleText;	// make a copy.
		m_sConsoleText.Empty();	// done using this.
		m_fConsoleTextReadyFlag = false; // rady to use again
		OnConsoleCmd( sText, this );
	}

	EXC_SET("ResyncCommand");
	if ( m_fResyncRequested != NULL )
	{
		if ( !m_fResyncPause )
		{
			SetResyncPause(true, m_fResyncRequested);
			SetResyncPause(false, m_fResyncRequested);
		}
		else SetResyncPause(false, m_fResyncRequested, true);
		m_fResyncRequested = NULL;
	}

	EXC_SET("SetTime");
	SetValidTime();	// we are a valid game server.

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

#ifdef _WIN32
	EXC_SET("init winsock");
	TCHAR * wSockInfo = Str_GetTemp();
	if ( !m_SocketMain.IsOpen() )
	{
		WSADATA wsaData;
		int err = WSAStartup(MAKEWORD(2,2), &wsaData);
		if ( err )
		{
			if ( err == WSAVERNOTSUPPORTED )
			{
				err = WSAStartup(MAKEWORD(1,1), &wsaData);
				if ( err ) goto nowinsock;
			}
			else
			{
nowinsock:		g_Log.Event(LOGL_FATAL|LOGM_INIT, "WinSock 1.1 not found!\n");
				return( false );
			}
			sprintf(wSockInfo, "Using WinSock ver %d.%d (%s)\n", HIBYTE(wsaData.wVersion), LOBYTE(wsaData.wVersion), wsaData.szDescription);
		}
	}
#endif

	EXC_SET("loading ini");
	if ( !g_Cfg.LoadIni(false) )
		return false;

	EXC_SET("log write");
	g_Log.WriteString("\n");

#if defined(__GITREVISION__) && defined(__GITHASH__)
	g_Log.Event(LOGM_INIT, "%s\nCompiled: %s (%s) [build %d / GIT hash %s]\n\n", g_szServerDescription, __DATE__, __TIME__, __GITREVISION__, __GITHASH__);
#else
	g_Log.Event(LOGM_INIT, "%s\nCompiled: %s (%s)\n\n", g_szServerDescription, __DATE__, __TIME__);
#endif

#ifdef _WIN32
	if ( wSockInfo[0] )
		g_Log.Event(LOGM_INIT, wSockInfo);
#endif

#ifdef _NIGHTLYBUILD
	g_Log.EventWarn("\n"
					"This is a NIGHTLY build of SphereServer. Nightly builds are compiled automatically\n"
					"from the source code with the latest updates, but might contain errors or might be\n"
					"unstable. Take caution when using it on live servers.\n"
					"----------------------------------------------------------------------------------\n\n");
#endif
#ifdef _DEBUG
	g_Log.EventWarn("\n"
					"This is a DEBUG build of SphereServer. Debug builds are compiled manually by developers\n"
					"for testing purposes. It have extra debug behaviors that are useful for development but\n"
					"will decrease server performance. Do not use it on live servers.\n"
					"---------------------------------------------------------------------------------------\n\n");
#endif
#ifdef _PRIVATEBUILD
	g_Log.EventWarn("\n"
					"This is a CUSTOM build of SphereServer. Custom builds are non-official builds, which\n"
					"have custom changes on source code not verified by official Sphere development team.\n"
					"Use it as your own risk.\n"
					"------------------------------------------------------------------------------------\n\n");
#endif

	EXC_SET("setting signals");
	SetSignals();

	EXC_SET("loading scripts");
	TriglistInit();
	if ( !g_Cfg.Load(false) )
		return false;

	EXC_SET("finalizing");
#ifdef _WIN32
	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, GRAY_TITLE " V" GRAY_VERSION " - %s", GetName());
	SetConsoleTitle(pszTemp);
#endif

	return true;
	EXC_CATCH;
	return false;
}
