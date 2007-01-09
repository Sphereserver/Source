#include "graysvr.h"
#include "network/network.h"
#include "common/exceptions.h"
#include "common/version.h"
#include "common/CAssoc.h"
#include "common/CFileList.h"
#include "SimpleCommand.h"

#include <signal.h>

#ifndef _WIN32
	#include <exception>
	#include <setjmp.h>
#endif

#if defined(_WIN32) && !defined(_DEBUG)
	#include "common/crashdump/crashdump.h"
#endif

////////////////////////////////////////////////////////
// -CTextConsole

void __cdecl CTextConsole::SysMessagef(LPCTSTR pszFormat, ...) const
{
	va_list vargs;
	TEMPSTRING(z);
	va_start(vargs, pszFormat);

	vsprintf(z, pszFormat, vargs);
	SysMessage(z);
	va_end(vargs);
}

CChar * CTextConsole::GetChar() const
{
	return( const_cast <CChar *>( dynamic_cast <const CChar *>( this )));
}

int CTextConsole::OnConsoleKey( CGString & sText, TCHAR nChar, bool fEcho )
{
	// eventually we should call OnConsoleCmd
	// RETURN:
	//  0 = dump this connection.
	//  1 = keep processing.
	//  2 = process this.

	if ( sText.GetLength() >= SCRIPT_MAX_LINE_LEN )
	{
commandtoolong:
		SysMessage( "Command too long\n" );
		sText.Empty();
		return 0;
	}

	if ( nChar == '\r' || nChar == '\n' )
	{
		if ( !sText.GetLength() ) return 1;

		if ( fEcho )
		{
			SysMessage("\n");
		}
		return 2;
	}
	else if ( nChar == 9 )			// TAB (auto-completion)
	{
		LPCTSTR	p;
		LPCTSTR tmp;
		int		inputLen;
		bool	matched(false);

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
				if ( strnicmp(curmatch->GetPtr(), p, inputLen) )	// mismatched
				{
					matched = false;
					break;
				}
			}
		}

		if ( matched )
		{
			if ( fEcho ) SysMessage(tmp);
			sText += tmp;
			if ( sText.GetLength() > SCRIPT_MAX_LINE_LEN ) goto commandtoolong;
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

CServer::CServer() : CServerDef( SPHERE_TITLE, CSocketAddressIP( SOCKET_LOCAL_ADDRESS ))
{
	m_iExitFlag = 0;
	m_iAdminClients = 0;
	m_fConsoleTextReadyFlag = false;

	// we are in start up mode.
	SetServerMode(SERVMODE_Loading);

	memset(m_PacketFilter, 0, sizeof(m_PacketFilter));
}

CServer::~CServer()
{
}


void CServer::SetServerMode(SERVMODE_TYPE mode)
{
	m_iModeCode = mode;
#ifdef _WIN32
	NTWindow_SetWindowTitle();
#endif
}

void CServer::SetExitFlag(int iFlag)
{
	if ( !m_iExitFlag )
		m_iExitFlag = iFlag;
}

void CServer::SysMessage( LPCTSTR pszMsg ) const
{
	// Print just to the main console.
	if ( !pszMsg || ISINTRESOURCE(pszMsg) )
		return;

#ifdef _WIN32
	NTWindow_PostMsg(pszMsg);
#else
	fputs(pszMsg, stdout);
#endif
}

void CServer::PrintTelnet( LPCTSTR pszMsg ) const
{
	if ( ! m_iAdminClients )
		return;
	ClientIterator it;
	CClient *client;
	while ( client = it.next() )
	{
		if (( client->GetConnectType() == CONNECT_TELNET ) && client->GetAccount() )
		{
			if ( !client->GetAccount()->IsPriv(PRIV_TELNET_SHORT) )	// this client accepts broadcasts
				client->SysMessage(pszMsg);
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
	// These vals can get very large. so use MulDiv to prevent overflow. (not IMULDIV)
	if ( iTotal <= 0 )
		return 100;

    int iPercent = IMULDIV(iCount, 100, iTotal);
	TEMPSTRING(pszTemp);
	sprintf(pszTemp, "%d%%", iPercent);
	int len = strlen(pszTemp);

	PrintTelnet(pszTemp);
	
#ifndef _WIN32
	SysMessage(pszTemp);
#endif
	while ( len-- )	// backspace it
	{
		PrintTelnet("\x08");
#ifndef _WIN32
 		SysMessage("\x08");
#endif
	}

#ifdef _WIN32
	NTWindow_SetWindowTitle(pszTemp);
#endif
	return iPercent;
}

int CServer::GetAgeHours() const
{
	return( CServTime::GetCurrentTime().GetTimeRaw() / (60*60*TICK_PER_SEC));
}

LPCTSTR CServer::GetStatusString( BYTE iIndex ) const
{
	TEMPSTRING(z);
	if ( iIndex == 0x22 )		// .info
	{
		sprintf(z, SPHERE_TITLE ", Name=%s, Age=%d, Clients=%d, Items=%d, Chars=%d, Mem=%dK\n",
			GetName(), GetAgeHours()/24, StatGet(SERV_STAT_CLIENTS), StatGet(SERV_STAT_ITEMS), StatGet(SERV_STAT_CHARS), StatGet(SERV_STAT_MEM));
	}
	else if ( iIndex == 0x24 )	// startup
	{
		sprintf(z, "Admin=%s, URL=%s, Lang=%s, TZ=%d\n",
			m_sEMail.GetPtr(), m_sURL.GetPtr(), m_sLang.GetPtr(), m_TimeZone);
	}
	return z;
}

//*********************************************************

void CServer::ListClients( CTextConsole * pConsole ) const
{
	// Mask which clients we want ?
	// Give a format of what info we want to SHOW ?
	if ( !pConsole )
		return;

	CChar * pCharCmd = pConsole->GetChar();

	TEMPSTRING(pszMsg);
	ClientIterator it;
	CClient *client;
	pConsole->SysMessage("Clients online:\n");
	while ( client = it.next() )
	{
		CChar *pChar = client->GetChar();
		if ( pChar )
		{
			if ( pCharCmd && !pCharCmd->CanDisturb(pChar) )
				continue;

			TCHAR chRank = '=';
			if ( client->IsPriv(PRIV_GM) || client->GetPrivLevel() >= PLEVEL_Counsel )
				chRank = pChar->IsStatFlag(STATF_Insubstantial) ? '*' : '+';

			sprintf(pszMsg, "%x:Acc%c'%s', (%s) Char='%s',(%s)\n",
				client->socketId(),
				chRank,
				client->GetAccount()->GetName(),
				client->speer(),
				pChar->GetName(),
				pChar->GetTopPoint().WriteUsed());
		}
		else
		{
			if ( pConsole->GetPrivLevel() < client->GetPrivLevel())
				continue;

			LPCTSTR pszState;
			switch ( client->GetConnectType() )
			{
			case CONNECT_TELNET:	pszState = "TelNet"; break;
			case CONNECT_HTTP:		pszState = "Web"; break;
			default: pszState = "NOT LOGGED IN"; break;
			}

			sprintf(pszMsg, "%x:Acc='%s', (%s) %s\n",
				client->socketId(),
				client->GetAccount() ? client->GetAccount()->GetName() : "<NA>",
				client->speer(),
				pszState );
		}
		pConsole->SysMessage(pszMsg);
	}
}

bool CServer::OnConsoleCmd( CGString & sText, CTextConsole * pSrc )
{
	// RETURN: false = boot the client.
	int		len = sText.GetLength();
	if ( len <= 0 ) return true;

	static const char *shortCommandsWithArgs = "#&";		// short commands with 2 characters starts with this
	static const char *scriptShortCommand = "b";			// script commands with 1 character starts with this
	char	low = tolower(sText[0]);
	bool	scriptCommand = ( len > 2 );

	if ( !scriptCommand )
	{
		scriptCommand = ( strchr(scriptShortCommand, low) != NULL );
	}
	if ( !scriptCommand )
	{
		if (( len == 2 ) && ( strchr(shortCommandsWithArgs, low) == NULL ))
			scriptCommand = true;
	}

	if ( scriptCommand )
	{
		LPCTSTR	pszText = sText;

		if ( g_Cfg.IsConsoleCmd(low) ) pszText++;

		CScript	script(pszText);
		if ( !g_Cfg.CanUsePrivVerb(this, pszText, pSrc) )
		{
			pSrc->SysMessagef("not privileged for command '%s'\n", pszText);
		}
		else if ( !r_Verb(script, pSrc) )
		{
			pSrc->SysMessagef("unknown command '%s'\n", pszText);
		}
	}
	else
	{
		switch ( low )
		{
		case '#':	//	save world
			g_World.m_timeSave.Init();
			break;
		case 'c':	// clients
			ListClients(pSrc);
			break;
		case 'e':	// profiler info reset
			{
				if ( IsSetEF(EF_Script_Profiler) )
				{
					if ( g_profiler.isValid() )
					{
						TScriptProfiler::TScriptProfilerFunction	*pFun;
						TScriptProfiler::TScriptProfilerTrigger		*pTrig;

						for ( pFun = g_profiler.FunctionsHead; pFun != NULL; pFun = pFun->next )
						{
							pFun->average = pFun->called = pFun->max = pFun->min = pFun->total = 0;
						}
						for ( pTrig = g_profiler.TriggersHead; pTrig != NULL; pTrig = pTrig->next )
						{
							pTrig->average = pTrig->called = pTrig->max = pTrig->min = pTrig->total = 0;
						}

						g_profiler.called = g_profiler.total = 0;
						pSrc->SysMessage("Scripts profiler info cleared\n");
					}
				}
				pSrc->SysMessage("Complete!\n");
			}
			break;
		case 'h':	// hearall
			{
				CScript script("HEARALL");
				r_Verb(script, pSrc);
				break;
			}
		case 'i':	// information
			{
				//extern void dumpStrings();
				//dumpStrings();
				CScript script("INFORMATION");
				r_Verb(script, pSrc);
				break;
			}
		case 'p':	// profiler info show
			ProfileDump(pSrc, ( ( len > 1 ) && ( sText[1] == '#' ) ));
			break;
		case 'r':	// resync
			{
				CScript script("RESYNC");
				r_Verb(script, pSrc);
			}
			break;
		case 't':
			TriglistPrint();
			break;
		case 'x':
			if (( len > 1 ) && ( sText[1] == '#' ))	//	X# - exit with save. Such exit is not protected by secure mode
			{
				g_Log.Event(LOGL_FATAL, "Immediate Shutdown initialized!\n");
				g_World.m_timeSave.Init();	// <- trigger save start
				SetExitFlag(1);
				do
				{
					Sleep(1500);
				} while ( g_World.m_iSaveStage != INT_MAX );
			}
			else
				pSrc->SysMessage("NOTE: Exitting this way with not saving is denied!\n");
			break;
	#ifdef _DEBUG
		case '&':
			{
				// generate exception
				if ( len > 1 )
				{
					if ( sText[1] == '1'  )
					{
						int xxx = 5;
						int j = 0;
						xxx /= j;
					}
					else if ( sText[1] == '2' )
					{
						CAccount *pAccount = NULL;
						pAccount->DeleteChars();
					}
				}
			} break;
	#endif
		default:
			pSrc->SysMessagef(
				"Available Commands:\n"
				"# = Immediate Save world\n"
				"B message = Broadcast a message\n"
				"C = Clients List (%d)\n"
				"E = Clear internal variables (like script profile)\n"
				"H = Hear all that is said (%s)\n"
				"I = Information\n"
				"P = Profile Info (P# to dump to profiler_dump.txt)\n"
				"R = Resync Pause\n"
				"T = Show list of triggers (with use count)\n"
				"X = immediate exit of the server (X# to save world and statics before exit)\n"
	#ifdef _DEBUG
				"& = generate exception &1 - div0, &1 - null pointer\n"
	#endif
				,
				StatGet(SERV_STAT_CLIENTS),
				( g_Log.LogMask() & LOGM_PLAYER_SPEAK ) ? "ON" : "OFF"
			);
		}
	}

	sText.Empty();
	return true;
}

//************************************************

PLEVEL_TYPE CServer::GetPrivLevel() const
{
	return PLEVEL_Owner;
}

void CServer::ProfileDump(CTextConsole *pSrc, bool bDump)
{
	if ( !pSrc || !IsSetEF(EF_Script_Profiler) )
		return;

	FILE	*dumper = NULL;

	TEMPSTRING(z);
	if ( bDump )
		dumper = fopen("profiler_dump.txt", "wt");

	if ( !g_profiler.isValid() )
		pSrc->SysMessagef("Script profiler is not yet informational\n");
	else
	{
		LONGLONG	average = g_profiler.total / g_profiler.called;
		TScriptProfiler::TScriptProfilerFunction	*pFun;
		TScriptProfiler::TScriptProfilerTrigger		*pTrig;
		DWORD	divby(1);
		divby = llTimeProfileFrequency/1000;

		sprintf(z, "Scripts: called %d times and took %d.%04d msec (%d.%04d msec average). Reporting with highest average.\n",
				g_profiler.called, 
				(int)(g_profiler.total/divby),
				(int)(((g_profiler.total*10000)/(divby))%10000),
				(int)(average/divby),
				(int)(((average*10000)/(divby))%10000)
			);
		if (dumper)
			fwrite(z, strlen(z), 1, dumper);
		else
			pSrc->SysMessage(z);

		for ( pFun = g_profiler.FunctionsHead; pFun != NULL; pFun = pFun->next )
		{
			if ( pFun->average > average )
			{
				sprintf(z, "FUNCTION '%s' called %d times, took %d.%04d msec average (%d.%04d min, %d.%04d max), total: %d.%04d msec\n",
					pFun->name, pFun->called,
					(int)(pFun->average/divby),
					(int)(((pFun->average*10000)/(divby))%10000),
					(int)(pFun->min/divby),
					(int)(((pFun->min*10000)/(divby))%10000),
					(int)(pFun->max/divby),
					(int)(((pFun->max*10000)/(divby))%10000),
					(int)(pFun->total/divby),
					(int)(((pFun->total*10000)/(divby))%10000)
				);
				pSrc->SysMessage(z);
				if ( dumper )
					fwrite(z, strlen(z), 1, dumper);
			}
		}
		for ( pTrig = g_profiler.TriggersHead; pTrig != NULL; pTrig = pTrig->next )
		{
			if ( pTrig->average > average )
			{
				sprintf(z, "TRIGGER '%s' called %d times, took %d.%04d msec average (%d.%04d min, %d.%04d max), total: %d.%04d msec\n",
					pTrig->name, pTrig->called,
					(int)(pTrig->average/divby),
					(int)(((pTrig->average*10000)/(divby))%10000),
					(int)(pTrig->min/divby),
					(int)(((pTrig->min*10000)/(divby))%10000),
					(int)(pTrig->max/divby),
					(int)(((pTrig->max*10000)/(divby))%10000),
					(int)(pTrig->total/divby),
					(int)(((pTrig->total*10000)/(divby))%10000)
				);
				pSrc->SysMessage(z);
				if ( dumper )
					fwrite(z, strlen(z), 1, dumper);
			}
		}
		pSrc->SysMessage("Report complete!\n");
	}

	if ( dumper )
		fclose(dumper);
}

// ---------------------------------------------------------------------

bool CServer::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	if ( isdigit( pszKey[0] ))
	{
		int i=1;
		while ( isdigit( pszKey[i] ))
			i++;
		if ( pszKey[i] == '.' )
		{
			int index = ATOI( pszKey );	// must use this to stop at .
			pRef = g_Cfg.Server_GetDef(index);
			pszKey += i + 1;
			return true;
		}
	}
	if ( g_Cfg.r_GetRef( pszKey, pRef ))
	{
		return true;
	}
	if ( g_World.r_GetRef( pszKey, pRef ))
	{
		return true;
	}
	return( CScriptObj::r_GetRef( pszKey, pRef ));
}

bool CServer::r_LoadVal( CScript &s )
{
	if ( g_Cfg.r_LoadVal(s) )
		return true;
	if ( g_World.r_LoadVal(s) )
		return true;
	return CServerDef::r_LoadVal(s);
}

bool CServer::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	if ( !strnicmp(pszKey, "ACCOUNT.", 8) )
	{
		pszKey += 8;
		CAccountRef pAccount = NULL;

		//	try to fetch using indexes
		if (( *pszKey >= '0' ) && ( *pszKey <= '9' ))
		{
			int num = Exp_GetVal(pszKey);
			if (( num >= 0 ) && ( num < g_Accounts.Account_GetCount() ))
				pAccount = g_Accounts.Account_Get(num);
		}

		//	indexes failed, try to fetch using names
		if ( !pAccount )
		{
			TEMPSTRING(pszTemp);
			strcpy(pszTemp, pszKey);
			char *split = strchr(pszTemp, '.');
			if ( split )
				*split = 0;

			pAccount = g_Accounts.Account_Find(pszTemp);
			pszKey += strlen(pszTemp);
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
	else if ( !strnicmp(pszKey, "CONFIG.", 7) )
	{
		pszKey += 7;
		config.scriptGet(pszKey, sVal);
		return true;
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
	SV_ALLCLIENTS,
	SV_B,
	SV_BLOCKIP,
	SV_CONSOLE,
	SV_HEARALL,
	SV_INFORMATION,
	SV_LOAD,
	SV_LOG,
	SV_RESPAWN,
	SV_RESTOCK,
	SV_RESYNC,
	SV_UNBLOCKIP,
	SV_VARLIST,
	SV_QTY,
};

LPCTSTR const CServer::sm_szVerbKeys[SV_QTY+1] =
{
	"ACCOUNT",
	"ALLCLIENTS",
	"B",
	"BLOCKIP",
	"CONSOLE",
	"HEARALL",
	"INFORMATION",
	"LOAD",
	"LOG",
	"RESPAWN",
	"RESTOCK",
	"RESYNC",
	"UNBLOCKIP",
	"VARLIST",
	NULL,
};

bool CServer::r_Verb( CScript &s, CTextConsole * pSrc )
{
	if ( !pSrc )
		return false;

	EXC_TRY("Verb");
	LPCTSTR pszKey = s.GetKey();
	TCHAR *pszMsg = NULL;
	int index = FindTableSorted( s.GetKey(), sm_szVerbKeys, COUNTOF( sm_szVerbKeys )-1 );

	if ( index < 0 )
	{
		CScriptTriggerArgs Args(s.GetArgRaw());
		if ( r_Call(pszKey, pSrc, &Args) )
			return true;

		if ( !strnicmp(pszKey, "ACCOUNT.", 8) )
		{
			index = SV_ACCOUNT;
			pszKey += 8;

			CAccountRef pAccount = NULL;
			TEMPSTRING(pszTemp);
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
		else if ( !strnicmp(pszKey, "CONFIG.", 7) )
		{
			pszKey += 7;
			return config.scriptSet(pszKey, s.GetArgStr(), pSrc);
		}

		if ( !strnicmp(pszKey, "CLEARVARS", 9) )
		{
			pszKey = s.GetArgStr();
			SKIP_SEPARATORS(pszKey);
			g_Exp.m_VarGlobals.ClearKeys(pszKey);
			return true;
		}

		if ( !strnicmp(pszKey, "strip", 5) )
		{
			int				i = 0;
			CResourceScript	*script;
			FILE			*f, *f1;
			LPCTSTR			dirname;

			TEMPSTRING(z);
			pszKey += 5;
			while ( *pszKey == ' ' ) pszKey++;
			dirname = ( *pszKey ? pszKey : "" );

			while ( script = g_Cfg.GetResourceFile(i++) )
			{
				strcpy(z, script->GetFilePath());
				f = fopen(z, "r");
				if ( !f )
				{
					pSrc->SysMessagef("Cannot open file %s for reading.\n", z);
					continue;
				}
				strcpy(z, dirname);
				strcat(z, script->GetFilePath());
				if ( !*dirname || !strcmpi(z, script->GetFilePath()) )
					strcat(z, ".strip");	// rename to *.strip if operating the same dir

				f1 = fopen(z, "w");
				if ( !f1 )
				{
					pSrc->SysMessagef("Cannot open file %s for writing.\n", z);
					fclose(f);
					continue;
				}
				while ( !feof(f) )
				{
					z[0] = 0;
					fgets(z, SCRIPT_MAX_LINE_LEN, f);
					_strlwr(z);
					if (( z[0] == '[' ) || !strncmp(z, "defname", 7) || !strncmp(z, "resources", 9) ||
						!strncmp(z, "name=", 5) || !strncmp(z, "type=", 5) || !strncmp(z, "id=", 3) ||
						!strncmp(z, "weight=", 7) || !strncmp(z, "value=", 6) || !strncmp(z, "dam=", 4) ||
						!strncmp(z, "armor=", 6) || !strncmp(z, "skillmake", 9) || !strncmp(z, "on=@", 4) ||
						!strncmp(z, "dupeitem", 8) || !strncmp(z, "dupelist", 8) || !strncmp(z, "id=", 3) ||
						!strncmp(z, "can=", 4) || !strncmp(z, "tevents=", 8) || !strncmp(z, "subsection=", 11) ||
						!strncmp(z, "description=", 12) || !strncmp(z, "category=", 9) )
						fputs(z, f1);
				}
				fclose(f);
				fclose(f1);
			}
			pSrc->SysMessagef("Scripts have just been stripped.\n");
			return true;
		}
	}

	switch (index)
	{
		case SV_ACCOUNT: // "ACCOUNT"
			return g_Accounts.Account_OnCmd(s.GetArgRaw(), pSrc);
	
		case SV_ALLCLIENTS:	// "ALLCLIENTS"
			{
				// Send a verb to all clients
				ClientIterator it;
				CClient *client;
				while ( client = it.next() )
				{
					if ( client->GetChar() == NULL )
						continue;
					CScript script(s.GetArgStr());
					client->GetChar()->r_Verb(script, pSrc);
				}
			}
			break;
	
		case SV_B: // "B"
			g_World.Broadcast( s.GetArgStr());
			break;
	
		case SV_BLOCKIP:
			if ( pSrc->GetPrivLevel() >= PLEVEL_Admin )
			{
				NetworkIn::HistoryIP *hist = &g_Network->getHistoryForIP(s.GetArgRaw());
				pSrc->SysMessagef("IP%s blocked\n", hist->m_blocked ? " already" : "");
				hist->m_blocked = true;
			} break;

		case SV_CONSOLE:
			{
				CGString z = s.GetArgRaw();
				OnConsoleCmd(z, pSrc);
			}
			break;

		case SV_HEARALL:	// "HEARALL" = Hear all said.
			{
				TEMPSTRING(pszMsg);
				g_Log.LogMask(s.GetArgFlag(g_Log.LogMask(), LOGM_PLAYER_SPEAK));
				sprintf(pszMsg, "Hear All %s.\n", (g_Log.LogMask() & LOGM_PLAYER_SPEAK) ? "Enabled" : "Disabled" );
			}
			break;
	
		case SV_INFORMATION:
			pSrc->SysMessage(GetStatusString(0x22));
			pSrc->SysMessage(GetStatusString(0x24));
			break;
	
		case SV_LOAD:
			// Load a resource file.
			if ( g_Cfg.LoadResourcesAdd( s.GetArgStr()) == NULL )
				return false;
			return true;
	
		case SV_LOG:
			{
				LPCTSTR	pszArgs = s.GetArgStr();
				int		mask = LOGM_CLIENTS_LOG|LOGL_EVENT;
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
			g_World.m_timeRespawn.Init();
			break;
	
		case SV_RESTOCK:
			{
				pCurThread->CriticalStart();

				// recalc all the base items
				for ( int i = 0; i < COUNTOF(g_Cfg.m_ResHash.m_Array); i++ )
					for ( int j = 0; j < g_Cfg.m_ResHash.m_Array[i].GetCount(); j++ )
					{
						CResourceDef	*pResDef = g_Cfg.m_ResHash.m_Array[i][j];
						if ( !pResDef || ( pResDef->GetResType() != RES_ITEMDEF ))
							continue;

						CItemBase	*pBase = dynamic_cast <CItemBase *>(pResDef);
						if ( pBase )
							pBase->Restock();
					}

				// restock vendors
				for ( int m = 0; m < 256; m++ )
				{
					if ( !g_MapList.m_maps[m] ) continue;

					for ( int s = 0; s < g_MapList.GetSectorQty(m); s++ )
					{
						CSector	*pSector = g_World.GetSector(m, s);
						if ( pSector )
							pSector->Restock();
					}
				}

				pCurThread->CriticalEnd();

				break;
			}
		case SV_RESYNC:
			{
				pCurThread->CriticalStart();
				g_Install.CloseFiles();

				CResourceScript *pResFile;
				for ( int j = 0; ; j++ )
				{
					 if ( !(pResFile = g_Cfg.GetResourceFile(j)) )
						 break;
					pResFile->CloseForce();
				}
				g_Cfg.m_scpIni.CloseForce();
				g_Cfg.m_scpTables.CloseForce();
				pSrc->SysMessage( g_Cfg.Load(true) ? "Resync complete!\n" : "Resync failed!\n" );
				g_Serv.SetServerMode(SERVMODE_Run);
				pCurThread->CriticalEnd();
				break;
			}

		case SV_UNBLOCKIP:
			if ( pSrc->GetPrivLevel() >= PLEVEL_Admin )
			{
				NetworkIn::HistoryIP *hist = &g_Network->getHistoryForIP(s.GetArgRaw());
				pSrc->SysMessagef("IP%s unblocked\n", hist->m_blocked ? "" : " already");
				hist->m_blocked = false;
			} break;

		case SV_VARLIST:
			if ( ! strcmpi( s.GetArgStr(), "log" ))
				pSrc = (CTextConsole *)&g_Serv;
			g_Exp.m_VarGlobals.DumpKeys(pSrc, "VAR.");
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
	g_Log.Debug("source '%s' char '%s' uid '0%lx'\n", (pSrc && pSrc->GetName()) ? pSrc->GetName() : "", 
		(pSrc && pSrc->GetChar()) ? pSrc->GetChar()->GetName() : "",
		(pSrc && pSrc->GetChar()) ? pSrc->GetChar()->uid() : 0 );
	EXC_DEBUG_END;
	return false;
}

//*********************************************************

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
			PrintStr( SPHERE_TITLE " \n"
				"Command Line Switches:\n"
#ifdef _WIN32
				"-cClassName Setup custom window class name for sphere (default: " SPHERE_TITLE "Svr)\n"
#endif
#if defined(_WIN32) && !defined(_DEBUG)
				"-E Enable the CrashDumper\n"
#endif
				"-Ofilename Use file to override SERV.CONFIG settings at start-time.\n"
				"-Q Quit when finished.\n"
				);
			SimpleCommand::help();
			return false;
		case 'C':
			//	these are parsed in other places - nt window part, etc
			continue;
#if defined(_WIN32) && !defined(_DEBUG)
		case 'E':
			CrashDump::Enable();
			continue;
#endif
		case 'O':
			config.loadFrom(pArg + 1);
			continue;
		case 'Q':
			return false;

		default:
			g_Log.Event(LOGM_INIT|LOGL_CRIT, "Don't recognize command line data '%s'\n", argv[argn]);
			break;
		}
	}

	return true;
}

//*********************************************************

bool CServer::SocketsInit() // Initialize sockets
{
	if ( !m_SocketMain.Create() )
	{
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "Unable to create socket!\n");
		return false;
	}

	linger lval;
	lval.l_onoff = 0;
	lval.l_linger = 10;
	m_SocketMain.SetSockOpt(SO_LINGER, (const char*) &lval, sizeof(lval));

	m_SocketMain.SetNonBlocking();

	// Bind to just one specific port if they say so.
	CSocketAddress SockAddr = m_ip;

	int iRet = m_SocketMain.Bind(SockAddr);
	if ( iRet < 0 )			// Probably already a server running.
	{
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "Unable to bind listen socket %s port %d - Error code: %d\n",
			SockAddr.GetAddrStr(), SockAddr.GetPort(), iRet);
		return false;
	}
	m_SocketMain.Listen();

	// What are we listing our port as to the world.
	// Tell the administrator what we know.

	TCHAR szName[ _MAX_PATH ];
	struct hostent * pHost = NULL;

	iRet = gethostname(szName, sizeof(szName));
	if ( iRet )
		strcpy(szName, m_ip.GetAddrStr());
	else
	{
		pHost = gethostbyname(szName);
		if ( pHost && pHost->h_addr && pHost->h_name && pHost->h_name[0] )
			strcpy(szName, pHost->h_name);
	}

	g_Log.Init("Server started on '%s' port %d.\n", szName, m_ip.GetPort());
	if ( !iRet && pHost && pHost->h_addr )
	{
		TEMPSTRING(ipList);
		for ( int i=0; pHost->h_addr_list[i] != NULL; i++ )
		{
			CSocketAddressIP ip;
			ip.SetAddrIP(*((DWORD*)(pHost->h_addr_list[i])));
			if ( !m_ip.IsLocalAddr() && !m_ip.IsSameIP(ip) ) continue;
			strcat(ipList, " \"");
			strcat(ipList, ip.GetAddrStr());
			strcat(ipList, "\"");
		}
		g_Log.Init("Monitoring IPs:%s\n", ipList);
	}
	return true;
}

bool CServer::Load()
{
	EXC_TRY("Load");
	// Keep track of the thread that is the parent.
	m_dwParentThread = CThread::Current();

#ifdef _WIN32
	EXC_SET("init winsock");
	TEMPSTRING(wSockInfo);
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
nowinsock:		g_Log.Event(LOGL_FATAL|LOGM_INIT, "Winsock 1.1 not found!\n");
				return false;
			}
			sprintf(wSockInfo, "Using WinSock ver %d.%d (%s)\n", HIBYTE(wsaData.wVersion), LOBYTE(wsaData.wVersion), wsaData.szDescription);
		}
	}
#endif

	EXC_SET("loading ini");
	if ( !g_Cfg.LoadIni(false) )
		return false;

	EXC_SET("log write");
	g_Log.WriteRaw("\n");
	g_Log.Init(SPHERE_FULL ", built at " __DATE__ " (" __TIME__ ")\n");
#ifdef _WIN32
	if ( wSockInfo[0] )
		g_Log.Event(LOGM_INIT, wSockInfo);
#endif


#ifdef _NIGHTLYBUILD
	g_Log.Warn(" --- WARNING ---\r\n\r\n"
		"This is a nightly build of SphereServer. This build is to be used\r\n"
		"for testing and/or bug reporting ONLY. DO NOT run this build on a\r\n"
		"live shard unless you know what you are doing!\r\n"
		"Nightly builds are automatically made every night from source and\r\n"
		"might contain errors, might be unstable or even destroy your\r\n"
		"shard as they are mostly untested!\r\n");

	if (!g_Cfg.m_bAgree)
	{
		g_Log.Error("Please set AGREE=1 in the INI file to acknowledge that\nyou understand the terms for nightly builds\n");
		return false;
	}
#endif

	EXC_SET("setting signals");
	ExceptionsThreadInit();

	EXC_SET("loading scripts");
	TriglistInit();
	if ( !g_Cfg.Load(false) )
		return false;

	EXC_SET("finilizing");
#ifdef _WIN32
	TEMPSTRING(pszTemp);
	sprintf(pszTemp, SPHERE_TITLE " V" SPHERE_VERSION " - %s", GetName());
	SetConsoleTitle(pszTemp);
#endif

	return true;
	EXC_CATCH;
	return false;
}
