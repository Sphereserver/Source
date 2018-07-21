#include "../graysvr/graysvr.h"
#ifdef _WIN32
	#include <process.h>
#else
	#include <errno.h>
#endif

///////////////////////////////////////////////////////////
// CTextConsole

CChar *CTextConsole::GetChar() const
{
	ADDTOCALLSTACK("CTextConsole::GetChar");
	return const_cast<CChar *>(dynamic_cast<const CChar *>(this));
}

int CTextConsole::OnConsoleKey(CGString &sText, TCHAR szChar, bool fEcho)
{
	ADDTOCALLSTACK("CTextConsole::OnConsoleKey");
	// Eventaully we should call OnConsoleCmd
	// RETURN:
	//  0 = dump this connection
	//  1 = keep processing
	//  2 = process this

	if ( sText.GetLength() >= SCRIPT_MAX_LINE_LEN )
	{
	commandtoolong:
		SysMessage("Command too long\n");
		sText.Empty();
		return 0;
	}

	if ( (szChar == '\r') || (szChar == '\n') )
	{
		// Ignore the character if we have no text stored
		if ( !sText.GetLength() )
			return 1;
		if ( fEcho )
			SysMessage("\n");
		return 2;
	}
	else if ( szChar == 9 )			// TAB (auto-completion)
	{
		// Extract up to start of the word
		LPCTSTR pszArgs = sText.GetPtr() + sText.GetLength();
		while ( (pszArgs >= sText.GetPtr()) && (*pszArgs != '.') && (*pszArgs != ' ') && (*pszArgs != '/') && (*pszArgs != '=') )
			--pszArgs;
		++pszArgs;
		size_t inputLen = strlen(pszArgs);

		// Search in the auto-complete list for starting on P, and save coords of first/last match
		CGStringListRec *psFirstMatch = NULL;
		CGStringListRec *psLastMatch = NULL;
		CGStringListRec *psCurMatch = NULL;		// the one that should be set
		for ( psCurMatch = g_AutoComplete.GetHead(); psCurMatch != NULL; psCurMatch = psCurMatch->GetNext() )
		{
			if ( !strnicmp(psCurMatch->GetPtr(), pszArgs, inputLen) )	// matched
			{
				if ( !psFirstMatch )
					psFirstMatch = psLastMatch = psCurMatch;
				else
					psLastMatch = psCurMatch;
			}
			else if ( psLastMatch )
				break;		// if no longer matches - save time by instant quit
		}

		LPCTSTR pszTemp = NULL;
		bool fMatch = false;
		if ( psFirstMatch && (psFirstMatch == psLastMatch) )	// there IS a match and the ONLY
		{
			pszTemp = psFirstMatch->GetPtr() + inputLen;
			fMatch = true;
		}
		else if ( psFirstMatch )		// also make SE (if SERV/SERVER in dic) to become SERV
		{
			pszArgs = pszTemp = psFirstMatch->GetPtr();
			pszTemp += inputLen;
			inputLen = strlen(pszArgs);
			fMatch = true;
			for ( psCurMatch = psFirstMatch->GetNext(); psCurMatch != psLastMatch->GetNext(); psCurMatch = psCurMatch->GetNext() )
			{
				if ( strnicmp(psCurMatch->GetPtr(), pszArgs, inputLen) != 0 )	// mismatched
				{
					fMatch = false;
					break;
				}
			}
		}

		if ( fMatch )
		{
			if ( fEcho )
				SysMessage(pszTemp);

			sText += pszTemp;
			if ( sText.GetLength() > SCRIPT_MAX_LINE_LEN )
				goto commandtoolong;
		}
		return 1;
	}

	if ( fEcho )
	{
		TCHAR szTmp[2];
		szTmp[0] = szChar;
		szTmp[1] = '\0';
		SysMessage(szTmp);
	}

	if ( szChar == 8 )
	{
		if ( sText.GetLength() )	// back key
			sText.SetLength(sText.GetLength() - 1);
		return 1;
	}

	sText += szChar;
	return 1;
}

///////////////////////////////////////////////////////////
// CScriptObj

TRIGRET_TYPE CScriptObj::OnTriggerForLoop(CScript &s, int iType, CTextConsole *pSrc, CScriptTriggerArgs *pArgs, CGString *psResult)
{
	ADDTOCALLSTACK("CScriptObj::OnTriggerForLoop");
	// Loop from start here to the ENDFOR
	// See WebPageScriptList for dealing with arrays

	CScriptLineContext StartContext = s.GetContext();
	CScriptLineContext EndContext = StartContext;
	int iLoopsMade = 0;

	if ( iType & 8 )		// WHILE
	{
		TCHAR *pszCond;
		CGString sOrig;
		TemporaryString pszTemp;
		int i = 0;

		sOrig.Copy(s.GetArgStr());
		for (;;)
		{
			++iLoopsMade;
			if ( g_Cfg.m_iMaxLoopTimes && (iLoopsMade >= g_Cfg.m_iMaxLoopTimes) )
				goto toomanyloops;

			strcpy(pszTemp, sOrig.GetPtr());
			pszCond = pszTemp;
			ParseText(pszCond, pSrc, 0, pArgs);
			if ( !Exp_GetLLVal(pszCond) )
				break;

			pArgs->m_VarsLocal.SetNum("_WHILE", i, false);
			TRIGRET_TYPE iRet = OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, psResult);
			if ( iRet == TRIGRET_BREAK )
			{
				EndContext = StartContext;
				break;
			}
			if ( (iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE) )
				return iRet;
			if ( iRet == TRIGRET_CONTINUE )
				EndContext = StartContext;
			else
				EndContext = s.GetContext();
			s.SeekContext(StartContext);
			++i;
		}
	}
	else
		ParseText(s.GetArgStr(), pSrc, 0, pArgs);

	if ( iType & 4 )	// FOR
	{
		int iMin = 0;
		int iMax = 0;
		TCHAR *ppArgs[3];
		size_t iQty = Str_ParseCmds(s.GetArgStr(), ppArgs, COUNTOF(ppArgs), ", ");
		CGString sLoopVar = "_FOR";

		switch ( iQty )
		{
			case 1:		// FOR max
			{
				iMin = 1;
				iMax = Exp_GetSingle(ppArgs[0]);
				break;
			}
			case 2:		// FOR min max, FOR name max
			{
				if ( IsDigit(*ppArgs[0]) || ((*ppArgs[0] == '-') && IsDigit(*(ppArgs[0] + 1))) )
				{
					iMin = Exp_GetSingle(ppArgs[0]);
					iMax = Exp_GetSingle(ppArgs[1]);
				}
				else
				{
					sLoopVar = ppArgs[0];
					iMin = 1;
					iMax = Exp_GetSingle(ppArgs[1]);
				}
				break;
			}
			case 3:		// FOR name min max
			{
				sLoopVar = ppArgs[0];
				iMin = Exp_GetSingle(ppArgs[1]);
				iMax = Exp_GetSingle(ppArgs[2]);
				break;
			}
			default:
			{
				iMin = iMax = 1;
				break;
			}
		}

		if ( iMin > iMax )
		{
			for ( int i = iMin; i >= iMax; --i )
			{
				++iLoopsMade;
				if ( g_Cfg.m_iMaxLoopTimes && (iLoopsMade >= g_Cfg.m_iMaxLoopTimes) )
					goto toomanyloops;

				pArgs->m_VarsLocal.SetNum(sLoopVar, i, false);
				TRIGRET_TYPE iRet = OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, psResult);
				if ( iRet == TRIGRET_BREAK )
				{
					EndContext = StartContext;
					break;
				}
				if ( (iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE) )
					return iRet;
				if ( iRet == TRIGRET_CONTINUE )
					EndContext = StartContext;
				else
					EndContext = s.GetContext();
				s.SeekContext(StartContext);
			}
		}
		else
		{
			for ( int i = iMin; i <= iMax; ++i )
			{
				++iLoopsMade;
				if ( g_Cfg.m_iMaxLoopTimes && (iLoopsMade >= g_Cfg.m_iMaxLoopTimes) )
					goto toomanyloops;

				pArgs->m_VarsLocal.SetNum(sLoopVar, i, false);
				TRIGRET_TYPE iRet = OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, psResult);
				if ( iRet == TRIGRET_BREAK )
				{
					EndContext = StartContext;
					break;
				}
				if ( (iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE) )
					return iRet;
				if ( iRet == TRIGRET_CONTINUE )
					EndContext = StartContext;
				else
					EndContext = s.GetContext();
				s.SeekContext(StartContext);
			}
		}
	}

	if ( (iType & 1) || (iType & 2) )
	{
		CObjBaseTemplate *pObj = dynamic_cast<CObjBaseTemplate *>(this);
		if ( !pObj )
		{
			iType = 0;
			DEBUG_ERR(("FOR Loop trigger on non-world object '%s'\n", GetName()));
		}
		else
		{
			CObjBaseTemplate *pObjTop = pObj->GetTopLevelObj();
			CPointMap pt = pObjTop->GetTopPoint();
			int iDist = s.HasArgs() ? s.GetArgVal() : UO_MAP_VIEW_SIZE;

			if ( iType & 1 )		// FORITEM, FOROBJ
			{
				CWorldSearch AreaItems(pt, iDist);
				for (;;)
				{
					++iLoopsMade;
					if ( g_Cfg.m_iMaxLoopTimes && (iLoopsMade >= g_Cfg.m_iMaxLoopTimes) )
						goto toomanyloops;

					CItem *pItem = AreaItems.GetItem();
					if ( !pItem )
						break;
					TRIGRET_TYPE iRet = pItem->OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, psResult);
					if ( iRet == TRIGRET_BREAK )
					{
						EndContext = StartContext;
						break;
					}
					if ( (iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE) )
						return iRet;
					if ( iRet == TRIGRET_CONTINUE )
						EndContext = StartContext;
					else
						EndContext = s.GetContext();
					s.SeekContext(StartContext);
				}
			}
			if ( iType & 2 )		// FORCHAR, FOROBJ
			{
				CWorldSearch AreaChars(pt, iDist);
				AreaChars.SetAllShow(iType & 0x20);
				for (;;)
				{
					++iLoopsMade;
					if ( g_Cfg.m_iMaxLoopTimes && (iLoopsMade >= g_Cfg.m_iMaxLoopTimes) )
						goto toomanyloops;

					CChar *pChar = AreaChars.GetChar();
					if ( !pChar )
						break;
					if ( (iType & 0x10) && !pChar->m_pClient )	// FORCLIENTS
						continue;
					if ( (iType & 0x20) && !pChar->m_pPlayer )	// FORPLAYERS
						continue;
					TRIGRET_TYPE iRet = pChar->OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, psResult);
					if ( iRet == TRIGRET_BREAK )
					{
						EndContext = StartContext;
						break;
					}
					if ( (iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE) )
						return iRet;
					if ( iRet == TRIGRET_CONTINUE )
						EndContext = StartContext;
					else
						EndContext = s.GetContext();
					s.SeekContext(StartContext);
				}
			}
		}
	}

	if ( iType & 0x40 )		// FORINSTANCES
	{
		RESOURCE_ID rid;
		TCHAR *ppArgs[1];
		if ( Str_ParseCmds(s.GetArgStr(), ppArgs, COUNTOF(ppArgs), " \t,") >= 1 )
			rid = g_Cfg.ResourceGetID(RES_UNKNOWN, const_cast<LPCTSTR &>(static_cast<LPTSTR &>(ppArgs[0])));
		else
		{
			const CObjBase *pObj = dynamic_cast<CObjBase *>(this);
			if ( pObj && pObj->Base_GetDef() )
				rid = pObj->Base_GetDef()->GetResourceID();
		}

		// No need to loop if there is no valid resource id
		if ( rid.IsValidUID() )
		{
			DWORD dwTotalInstances = 0;		// will acquire the correct value for this during the loop
			DWORD dwUID = 0;
			DWORD dwTotal = g_World.GetUIDCount();
			DWORD dwCount = dwTotal - 1;
			DWORD dwFound = 0;

			while ( dwCount-- )
			{
				if ( ++dwUID >= dwTotal )
					break;

				CObjBase *pObj = g_World.FindUID(dwUID);
				if ( !pObj || (pObj->Base_GetDef()->GetResourceID() != rid) )
					continue;

				++iLoopsMade;
				if ( g_Cfg.m_iMaxLoopTimes && (iLoopsMade >= g_Cfg.m_iMaxLoopTimes) )
					goto toomanyloops;

				TRIGRET_TYPE iRet = pObj->OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, psResult);
				if ( iRet == TRIGRET_BREAK )
				{
					EndContext = StartContext;
					break;
				}
				if ( (iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE) )
					return iRet;
				if ( iRet == TRIGRET_CONTINUE )
					EndContext = StartContext;
				else
					EndContext = s.GetContext();
				s.SeekContext(StartContext);

				if ( (dwTotalInstances == 0) && pObj->Base_GetDef() )
					dwTotalInstances = pObj->Base_GetDef()->GetRefInstances();

				++dwFound;
				if ( (dwTotalInstances > 0) && (dwFound >= dwTotalInstances) )
					break;
			}
		}
	}

	if ( iType & 0x100 )	// FORTIMERF
	{
		TCHAR *ppArgs[1];
		if ( Str_ParseCmds(s.GetArgStr(), ppArgs, COUNTOF(ppArgs), " \t,") >= 1 )
		{
			char chFunctionName[1024];
			strncpy(chFunctionName, ppArgs[0], sizeof(chFunctionName) - 1);

			TRIGRET_TYPE iRet = g_World.m_TimedFunctions.Loop(chFunctionName, iLoopsMade, StartContext, s, pSrc, pArgs, psResult);
			if ( (iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE) )
				return iRet;
		}
	}

	if ( g_Cfg.m_iMaxLoopTimes )
	{
	toomanyloops:
		if ( iLoopsMade >= g_Cfg.m_iMaxLoopTimes )
			g_Log.EventError("Terminating loop cycle since it seems being dead-locked (%d iterations already passed)\n", iLoopsMade);
	}

	if ( EndContext.m_lOffset <= StartContext.m_lOffset )
	{
		// Just skip to the end
		TRIGRET_TYPE iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, psResult);
		if ( iRet != TRIGRET_ENDIF )
			return iRet;
	}
	else
		s.SeekContext(EndContext);

	return TRIGRET_ENDIF;
}

TRIGRET_TYPE CScriptObj::OnTriggerScript(CScript &s, LPCTSTR pszTrigName, CTextConsole *pSrc, CScriptTriggerArgs *pArgs)
{
	ADDTOCALLSTACK("CScriptObj::OnTriggerScript");
	// Look for exact trigger matches
	if ( !OnTriggerFind(s, pszTrigName) )
		return TRIGRET_RET_DEFAULT;

	ProfileTask scriptsTask(PROFILE_SCRIPTS);

	TScriptProfiler::TScriptProfilerTrigger *pTrigger = NULL;
	ULONGLONG llTicksStart, llTicksEnd;

	if ( IsSetEF(EF_Script_Profiler) )
	{
		// Lowercase for speed
		TCHAR *pszName = Str_GetTemp();
		strncpy(pszName, pszTrigName, sizeof(pTrigger->name) - 1);
		_strlwr(pszName);

		if ( g_profiler.initstate != 0xF1 )		// profiler is not initialized
		{
			memset(&g_profiler, 0, sizeof(g_profiler));
			g_profiler.initstate = static_cast<BYTE>(0xF1);		// ''
		}

		for ( pTrigger = g_profiler.TriggersHead; pTrigger != NULL; pTrigger = pTrigger->next )
		{
			if ( !strcmp(pTrigger->name, pszName) )
				break;
		}
		if ( !pTrigger )
		{
			// First time that the trigger is called, so create its record
			pTrigger = new TScriptProfiler::TScriptProfilerTrigger;
			memset(pTrigger, 0, sizeof(TScriptProfiler::TScriptProfilerTrigger));
			strncpy(pTrigger->name, pszName, sizeof(pTrigger->name) - 1);
			if ( g_profiler.TriggersTail )
				g_profiler.TriggersTail->next = pTrigger;
			else
				g_profiler.TriggersHead = pTrigger;
			g_profiler.TriggersTail = pTrigger;
		}

		pTrigger->called++;
		g_profiler.called++;
		TIME_PROFILE_START;
	}

	TRIGRET_TYPE iRet = OnTriggerRunVal(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs);

	if ( IsSetEF(EF_Script_Profiler) && pTrigger )
	{
		TIME_PROFILE_END;
		llTicksStart = llTicksEnd - llTicksStart;
		pTrigger->total += llTicksStart;
		pTrigger->average = pTrigger->total / pTrigger->called;
		if ( pTrigger->max < llTicksStart )
			pTrigger->max = llTicksStart;
		if ( (pTrigger->min > llTicksStart) || !pTrigger->min )
			pTrigger->min = llTicksStart;
		g_profiler.total += llTicksStart;
	}

	return iRet;
}

bool CScriptObj::OnTriggerFind(CScript &s, LPCTSTR pszTrigName)
{
	ADDTOCALLSTACK("CScriptObj::OnTriggerFind");
	while ( s.ReadKey(false) )
	{
		if ( strnicmp(s.GetKey(), "ON", 2) != 0 )
			continue;

		s.ParseKeyLate();
		if ( strcmpi(s.GetArgRaw(), pszTrigName) == 0 )
			return true;
	}
	return false;
}

enum SK_TYPE
{
	SK_BEGIN,
	SK_BREAK,
	SK_CONTINUE,
	SK_DORAND,
	SK_DOSWITCH,
	SK_ELIF,
	SK_ELSE,
	SK_ELSEIF,
	SK_END,
	SK_ENDDO,
	SK_ENDFOR,
	SK_ENDIF,
	SK_ENDRAND,
	SK_ENDSWITCH,
	SK_ENDWHILE,
	SK_FOR,
	SK_FORCHARLAYER,
	SK_FORCHARMEMORYTYPE,
	SK_FORCHAR,
	SK_FORCLIENTS,
	SK_FORCONT,
	SK_FORCONTID,
	SK_FORCONTTYPE,
	SK_FORINSTANCE,
	SK_FORITEM,
	SK_FOROBJ,
	SK_FORPLAYERS,
	SK_FORTIMERF,
	SK_IF,
	SK_RETURN,
	SK_WHILE,
	SK_QTY
};

LPCTSTR const CScriptObj::sm_szScriptKeys[SK_QTY + 1] =
{
	"BEGIN",
	"BREAK",
	"CONTINUE",
	"DORAND",
	"DOSWITCH",
	"ELIF",
	"ELSE",
	"ELSEIF",
	"END",
	"ENDDO",
	"ENDFOR",
	"ENDIF",
	"ENDRAND",
	"ENDSWITCH",
	"ENDWHILE",
	"FOR",
	"FORCHARLAYER",
	"FORCHARMEMORYTYPE",
	"FORCHARS",
	"FORCLIENTS",
	"FORCONT",
	"FORCONTID",
	"FORCONTTYPE",
	"FORINSTANCES",
	"FORITEMS",
	"FOROBJS",
	"FORPLAYERS",
	"FORTIMERF",
	"IF",
	"RETURN",
	"WHILE",
	NULL
};

TRIGRET_TYPE CScriptObj::OnTriggerRun(CScript &s, TRIGRUN_TYPE trigger, CTextConsole *pSrc, CScriptTriggerArgs *pArgs, CGString *psResult)
{
	ADDTOCALLSTACK("CScriptObj::OnTriggerRun");
	// ARGS:
	//  TRIGRUN_SECTION_SINGLE = just this 1 line
	// RETURN:
	//  TRIGRET_RET_FALSE = return and continue processing
	//  TRIGRET_RET_TRUE = return and handled (halt further processing)
	//  TRIGRET_RET_DEFAULT = if process returns nothing specifically

	//CScriptFileContext set g_Log.m_pObjectContext is the current context (we assume)
	//DEBUGCHECK(this == g_Log.m_pObjectContext);

	// All scripts should have args for locals to work
	CScriptTriggerArgs argsEmpty;
	if ( !pArgs )
		pArgs = &argsEmpty;

	// Script execution is always not threaded action
	EXC_TRY("TriggerRun");

	bool fSectionFalse = ((trigger == TRIGRUN_SECTION_FALSE) || (trigger == TRIGRUN_SINGLE_FALSE));
	if ( (trigger == TRIGRUN_SECTION_EXEC) || (trigger == TRIGRUN_SINGLE_EXEC) )	// header was already read in
		goto jump_in;

	EXC_SET("parsing");
	while ( s.ReadKeyParse() )
	{
		// Hit the end of the next trigger
		if ( s.IsKeyHead("ON", 2) )		// done with this section
			break;

	jump_in:
		TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;

		SK_TYPE index = static_cast<SK_TYPE>(FindTableSorted(s.GetKey(), sm_szScriptKeys, COUNTOF(sm_szScriptKeys) - 1));
		switch ( index )
		{
			case SK_ENDIF:
			case SK_END:
			case SK_ENDDO:
			case SK_ENDFOR:
			case SK_ENDRAND:
			case SK_ENDSWITCH:
			case SK_ENDWHILE:
				return TRIGRET_ENDIF;
			case SK_ELIF:
			case SK_ELSEIF:
				return TRIGRET_ELSEIF;
			case SK_ELSE:
				return TRIGRET_ELSE;
			default:
				break;
		}

		if ( fSectionFalse )
		{
			// Ignoring this whole section, don't bother parsing it
			switch ( index )
			{
				case SK_IF:
				{
					EXC_SET("if statement");
					do
					{
						iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, psResult);
					} while ( (iRet == TRIGRET_ELSEIF) || (iRet == TRIGRET_ELSE) );
					break;
				}
				case SK_WHILE:
				case SK_FOR:
				case SK_FORCHARLAYER:
				case SK_FORCHARMEMORYTYPE:
				case SK_FORCHAR:
				case SK_FORCLIENTS:
				case SK_FORCONT:
				case SK_FORCONTID:
				case SK_FORCONTTYPE:
				case SK_FORINSTANCE:
				case SK_FORITEM:
				case SK_FOROBJ:
				case SK_FORPLAYERS:
				case SK_FORTIMERF:
				case SK_DORAND:
				case SK_DOSWITCH:
				case SK_BEGIN:
					EXC_SET("begin/loop cycle");
					iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, psResult);
					break;
				default:
					break;
			}
			if ( trigger >= TRIGRUN_SINGLE_EXEC )
				return TRIGRET_RET_DEFAULT;
			continue;	// just ignore it
		}

		switch ( index )
		{
			case SK_BREAK:
				return TRIGRET_BREAK;
			case SK_CONTINUE:
				return TRIGRET_CONTINUE;
			case SK_FORITEM:
				EXC_SET("foritem");
				iRet = OnTriggerForLoop(s, 1, pSrc, pArgs, psResult);
				break;
			case SK_FORCHAR:
				EXC_SET("forchar");
				iRet = OnTriggerForLoop(s, 2, pSrc, pArgs, psResult);
				break;
			case SK_FORCLIENTS:
				EXC_SET("forclients");
				iRet = OnTriggerForLoop(s, 0x12, pSrc, pArgs, psResult);
				break;
			case SK_FOROBJ:
				EXC_SET("forobjs");
				iRet = OnTriggerForLoop(s, 3, pSrc, pArgs, psResult);
				break;
			case SK_FORPLAYERS:
				EXC_SET("forplayers");
				iRet = OnTriggerForLoop(s, 0x22, pSrc, pArgs, psResult);
				break;
			case SK_FOR:
				EXC_SET("for");
				iRet = OnTriggerForLoop(s, 4, pSrc, pArgs, psResult);
				break;
			case SK_WHILE:
				EXC_SET("while");
				iRet = OnTriggerForLoop(s, 8, pSrc, pArgs, psResult);
				break;
			case SK_FORINSTANCE:
				EXC_SET("forinstance");
				iRet = OnTriggerForLoop(s, 0x40, pSrc, pArgs, psResult);
				break;
			case SK_FORTIMERF:
				EXC_SET("fortimerf");
				iRet = OnTriggerForLoop(s, 0x100, pSrc, pArgs, psResult);
				break;
			case SK_FORCHARLAYER:
			case SK_FORCHARMEMORYTYPE:
			{
				EXC_SET("forchar[layer/memorytype]");
				if ( !s.HasArgs() )
				{
					DEBUG_ERR(("FORCHAR[layer/memorytype] called without arguments\n"));
					iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, psResult);
					break;
				}

				CChar *pChar = dynamic_cast<CChar *>(this);
				if ( !pChar )
				{
					DEBUG_ERR(("FORCHAR[layer/memorytype] called on non-char object '%s'\n", GetName()));
					iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, psResult);
					break;
				}

				ParseText(s.GetArgRaw(), pSrc, 0, pArgs);
				if ( index == SK_FORCHARLAYER )
					iRet = pChar->OnCharTrigForLayerLoop(s, pSrc, pArgs, psResult, static_cast<LAYER_TYPE>(s.GetArgVal()));
				else
					iRet = pChar->OnCharTrigForMemTypeLoop(s, pSrc, pArgs, psResult, static_cast<WORD>(s.GetArgVal()));
				break;
			}
			case SK_FORCONT:
			{
				EXC_SET("forcont");
				if ( !s.HasArgs() )
				{
					DEBUG_ERR(("FORCONT called without arguments\n"));
					iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, psResult);
					break;
				}

				TCHAR *ppArgs[2];
				size_t iArgsQty = Str_ParseCmds(const_cast<TCHAR *>(s.GetArgRaw()), ppArgs, COUNTOF(ppArgs), " \t,");

				TemporaryString pszTemp;
				strcpy(pszTemp, ppArgs[0]);
				TCHAR *pszTempPoint = pszTemp;
				ParseText(pszTempPoint, pSrc, 0, pArgs);

				CGrayUID uid = static_cast<CGrayUID>(Exp_GetLLVal(pszTempPoint));
				if ( !uid.IsValidUID() )
				{
					DEBUG_ERR(("FORCONT called on invalid uid '0%lx'\n", uid.GetObjUID()));
					iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, psResult);
					break;
				}

				CContainer *pCont = dynamic_cast<CContainer *>(uid.ObjFind());
				if ( !pCont )
				{
					DEBUG_ERR(("FORCONT called on non-container uid '0%lx'\n", uid.GetObjUID()));
					iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, psResult);
					break;
				}

				CScriptLineContext StartContext = s.GetContext();
				CScriptLineContext EndContext = StartContext;
				iRet = pCont->OnGenericContTriggerForLoop(s, pSrc, pArgs, psResult, StartContext, EndContext, (iArgsQty >= 2) ? Exp_GetVal(ppArgs[1]) : 255);
				break;
			}
			case SK_FORCONTID:
			case SK_FORCONTTYPE:
			{
				EXC_SET("forcont[id/type]");
				if ( !s.HasArgs() )
				{
					DEBUG_ERR(("FORCONT[id/type] called without arguments\n"));
					iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, psResult);
					break;
				}

				CObjBase *pObj = dynamic_cast<CObjBase *>(this);
				CContainer *pCont = dynamic_cast<CContainer *>(this);
				if ( !pObj || !pCont )
				{
					DEBUG_ERR(("FORCONT[id/type] called on non-container object '%s'\n", GetName()));
					iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, psResult);
					break;
				}

				LPCTSTR pszKey = s.GetArgRaw();
				SKIP_SEPARATORS(pszKey);

				TCHAR *ppArgs[2];
				size_t iArgsQty = Str_ParseCmds(const_cast<TCHAR *>(pszKey), ppArgs, COUNTOF(ppArgs), " \t,");

				TemporaryString pszTemp;
				strcpy(pszTemp, ppArgs[0]);
				ParseText(pszTemp, pSrc, 0, pArgs);

				CScriptLineContext StartContext = s.GetContext();
				CScriptLineContext EndContext = StartContext;
				iRet = pCont->OnContTriggerForLoop(s, pSrc, pArgs, psResult, StartContext, EndContext, g_Cfg.ResourceGetID((index == SK_FORCONTID) ? RES_ITEMDEF : RES_TYPEDEF, static_cast<LPCTSTR &>(pszTemp)), 0, (iArgsQty >= 2) ? Exp_GetVal(ppArgs[1]) : 255);
				break;
			}
			default:
			{
				// Parse out any variables in it (may act like a verb sometimes?)
				EXC_SET("parsing");
				if ( strchr(s.GetKey(), '<') )
				{
					EXC_SET("parsing <> in a key");
					TemporaryString pszBuffer;
					strcpy(pszBuffer, s.GetKey());
					strcat(pszBuffer, " ");
					strcat(pszBuffer, s.GetArgRaw());
					ParseText(pszBuffer, pSrc, 0, pArgs);
					s.ParseKey(pszBuffer);
				}
				else
					ParseText(s.GetArgRaw(), pSrc, 0, pArgs);
			}
		}

		switch ( index )
		{
			case SK_FORITEM:
			case SK_FORCHAR:
			case SK_FORCHARLAYER:
			case SK_FORCHARMEMORYTYPE:
			case SK_FORCLIENTS:
			case SK_FORCONT:
			case SK_FORCONTID:
			case SK_FORCONTTYPE:
			case SK_FOROBJ:
			case SK_FORPLAYERS:
			case SK_FORINSTANCE:
			case SK_FORTIMERF:
			case SK_FOR:
			case SK_WHILE:
			{
				if ( iRet != TRIGRET_ENDIF )
					return iRet;
				break;
			}
			case SK_DORAND:
			case SK_DOSWITCH:
			{
				EXC_SET("dorand/doswitch");
				INT64 iVal = s.GetArgLLVal();
				if ( index == SK_DORAND )
					iVal = Calc_GetRandLLVal(iVal);
				for ( ; ; --iVal )
				{
					iRet = OnTriggerRun(s, (iVal == 0) ? TRIGRUN_SINGLE_TRUE : TRIGRUN_SINGLE_FALSE, pSrc, pArgs, psResult);
					if ( iRet == TRIGRET_RET_DEFAULT )
						continue;
					if ( iRet == TRIGRET_ENDIF )
						break;
					return iRet;
				}
				break;
			}
			case SK_RETURN:
			{
				EXC_SET("return");
				if ( psResult )
				{
					psResult->Copy(s.GetArgStr());
					return TRIGRET_RET_TRUE;
				}
				return static_cast<TRIGRET_TYPE>(s.GetArgVal());
			}
			case SK_IF:
			{
				EXC_SET("if statement");
				bool fTrigger = (s.GetArgVal() != 0);
				bool fBeenTrue = false;
				for ( ;;)
				{
					iRet = OnTriggerRun(s, fTrigger ? TRIGRUN_SECTION_TRUE : TRIGRUN_SECTION_FALSE, pSrc, pArgs, psResult);
					if ( (iRet < TRIGRET_ENDIF) || (iRet >= TRIGRET_RET_HALFBAKED) )
						return iRet;
					if ( iRet == TRIGRET_ENDIF )
						break;
					fBeenTrue |= fTrigger;
					if ( fBeenTrue )
						fTrigger = false;
					else if ( iRet == TRIGRET_ELSE )
						fTrigger = true;
					else if ( iRet == TRIGRET_ELSEIF )
					{
						ParseText(s.GetArgStr(), pSrc, 0, pArgs);
						fTrigger = (s.GetArgVal() != 0);
					}
				}
				break;
			}
			case SK_BEGIN:
			{
				EXC_SET("begin/loop cycle");
				iRet = OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, psResult);
				if ( iRet != TRIGRET_ENDIF )
					return iRet;
				break;
			}
			default:
			{
				EXC_SET("parsing");
				if ( !pArgs->r_Verb(s, pSrc) )
				{
					bool fRes;
					if ( !strcmpi(s.GetKey(), "call") )
					{
						EXC_SET("call");
						TCHAR *pszArgRaw = s.GetArgRaw();
						CScriptObj *pRef = this;

						// Parse object references, SRC.* is not parsed by r_GetRef so do it manually
						r_GetRef(const_cast<LPCTSTR &>(static_cast<LPTSTR &>(pszArgRaw)), pRef);
						if ( !strnicmp("SRC.", pszArgRaw, 4) )
						{
							pszArgRaw += 4;
							pRef = pSrc->GetChar();
						}
						if ( pRef )
						{
							// Locate arguments for the called function
							TCHAR *z = strchr(pszArgRaw, ' ');
							if ( z )
							{
								*z = 0;
								++z;
								GETNONWHITESPACE(z);
							}

							CGString sVal;
							if ( z && *z )
							{
								INT64 iN1 = pArgs->m_iN1;
								INT64 iN2 = pArgs->m_iN2;
								INT64 iN3 = pArgs->m_iN3;
								CScriptObj *pO1 = pArgs->m_pO1;
								CGString s1 = pArgs->m_s1;
								CGString s1_raw = pArgs->m_s1_raw;
								pArgs->m_v.SetCount(0);

								pArgs->Init(z);
								fRes = pRef->r_Call(pszArgRaw, pSrc, pArgs, &sVal);

								pArgs->m_iN1 = iN1;
								pArgs->m_iN2 = iN2;
								pArgs->m_iN3 = iN3;
								pArgs->m_pO1 = pO1;
								pArgs->m_s1 = s1;
								pArgs->m_s1_raw = s1_raw;
								pArgs->m_v.SetCount(0);
							}
							else
								fRes = pRef->r_Call(pszArgRaw, pSrc, pArgs, &sVal);
						}
						else
							fRes = false;
					}
					else if ( !strcmpi(s.GetKey(), "FullTrigger") )
					{
						EXC_SET("FullTrigger");
						TCHAR *pszArgs = s.GetArgRaw();
						CScriptObj *pRef = this;

						TCHAR *piCmd[7];
						size_t iQty = Str_ParseCmds(pszArgs, piCmd, COUNTOF(piCmd), " ,\t");
						if ( iQty == 2 )
						{
							CGrayUID uid = static_cast<CGrayUID>(ATOI(piCmd[1]));
							if ( uid.ObjFind() )
								pRef = uid.ObjFind();
						}

						// Parse object references, SRC.* is not parsed by r_GetRef so do it manually
						if ( !strnicmp("SRC.", pszArgs, 4) )
						{
							pszArgs += 4;
							pRef = pSrc->GetChar();
						}
						if ( pRef )
						{
							// Locate arguments for the called function
							TRIGRET_TYPE iRet;
							TCHAR *z = strchr(pszArgs, ' ');

							if ( z )
							{
								*z = 0;
								++z;
								GETNONWHITESPACE(z);
							}

							if ( z && *z )
							{
								INT64 iN1 = pArgs->m_iN1;
								INT64 iN2 = pArgs->m_iN2;
								INT64 iN3 = pArgs->m_iN3;
								CScriptObj *pO1 = pArgs->m_pO1;
								CGString s1 = pArgs->m_s1;
								CGString s1_raw = pArgs->m_s1_raw;
								pArgs->m_v.SetCount(0);

								pArgs->Init(z);
								iRet = pRef->OnTrigger(pszArgs, pSrc, pArgs);

								pArgs->m_iN1 = iN1;
								pArgs->m_iN2 = iN2;
								pArgs->m_iN3 = iN3;
								pArgs->m_pO1 = pO1;
								pArgs->m_s1 = s1;
								pArgs->m_s1_raw = s1_raw;
								pArgs->m_v.SetCount(0);
							}
							else
								iRet = pRef->OnTrigger(pszArgs, pSrc, pArgs);

							pArgs->m_VarsLocal.SetNum("return", iRet);
							fRes = (iRet > TRIGRET_RET_FALSE);
						}
						else
							fRes = false;
					}
					else
					{
						EXC_SET("verb");
						fRes = r_Verb(s, pSrc);
					}

					if ( !fRes )
						DEBUG_MSG(("WARNING: Trigger Bad Verb '%s','%s'\n", s.GetKey(), s.GetArgStr()));
				}
				break;
			}
		}

		if ( trigger >= TRIGRUN_SINGLE_EXEC )
			return TRIGRET_RET_DEFAULT;
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("key '%s' runtype '%d' pargs '%p' ret '%s' [%p]\n", s.GetKey(), trigger, static_cast<void *>(pArgs), psResult ? psResult->GetPtr() : "", static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return TRIGRET_RET_DEFAULT;
}

TRIGRET_TYPE CScriptObj::OnTriggerRunVal(CScript &s, TRIGRUN_TYPE trigger, CTextConsole *pSrc, CScriptTriggerArgs *pArgs)
{
	// Get the TRIGRET_TYPE that is returned by the script
	// This should be used instead of OnTriggerRun() when pReturn is not used
	ADDTOCALLSTACK("CScriptObj::OnTriggerRunVal");

	CGString sVal;
	OnTriggerRun(s, trigger, pSrc, pArgs, &sVal);

	LPCTSTR pszVal = sVal.GetPtr();
	if ( pszVal && *pszVal )
		return static_cast<TRIGRET_TYPE>(Exp_GetLLVal(pszVal));
	return TRIGRET_RET_DEFAULT;
}

size_t CScriptObj::ParseText(TCHAR *pszResponse, CTextConsole *pSrc, int iFlags, CScriptTriggerArgs *pArgs)
{
	ADDTOCALLSTACK("CScriptObj::ParseText");
	// Take in a line of text that may have fields that can be replaced with operators here
	// ARGS:
	//  iFlags = 1=Use HTML %% as delimiters / 2=Allow recusive bracket count
	// NOTE:
	//  HTML will have opening <script language="SPHERE_FILE"> and then closing </script>
	// RETURN:
	//  New length of the string

	LPCTSTR pszKey;	// temporary, set below
	bool fRes;

	static int sm_iReentrant = 0;
	static bool sm_fBrackets = false;	// allowed to span multi lines

	bool fQvalCondition = false;
	TCHAR chQval = '?';

	if ( (iFlags & 2) == 0 )
		sm_fBrackets = false;

	size_t iBegin = 0;
	TCHAR chBegin = '<';
	TCHAR chEnd = '>';

	bool fHTML = (iFlags & 1);
	if ( fHTML )
	{
		chBegin = '%';
		chEnd = '%';
	}

	size_t i = 0;
	EXC_TRY("ParseText");
	for ( i = 0; pszResponse[i]; ++i )
	{
		TCHAR ch = pszResponse[i];

		if ( !sm_fBrackets )	// not in brackets
		{
			if ( ch == chBegin )	// found the start
			{
				if ( !(isalnum(pszResponse[i + 1]) || (pszResponse[i + 1] == '<')) )
					continue;
				iBegin = i;
				sm_fBrackets = true;
			}
			continue;
		}

		if ( ch == '<' )	// recursive brackets
		{
			if ( !(isalnum(pszResponse[i + 1]) || (pszResponse[i + 1] == '<')) )
				continue;

			if ( sm_iReentrant > 32 )
			{
				EXC_SET("reentrant limit");
				ASSERT(sm_iReentrant < 32);
			}
			++sm_iReentrant;
			sm_fBrackets = false;
			size_t iLen = ParseText(pszResponse + i, pSrc, 2, pArgs);
			sm_fBrackets = true;
			--sm_iReentrant;
			i += iLen;
			continue;
		}
		// QVAL fix
		if ( ch == chQval )
		{
			if ( !strnicmp(static_cast<LPCTSTR>(pszResponse) + iBegin + 1, "QVAL", 4) )
				fQvalCondition = true;
		}

		if ( ch == chEnd )
		{
			if ( !strnicmp(static_cast<LPCTSTR>(pszResponse) + iBegin + 1, "QVAL", 4) && !fQvalCondition )
				continue;
			// QVAL fix end
			sm_fBrackets = false;
			pszResponse[i] = '\0';

			EXC_SET("writeval");
			pszKey = static_cast<LPCTSTR>(pszResponse) + iBegin + 1;
			CGString sVal;
			fRes = r_WriteVal(pszKey, sVal, pSrc);
			if ( !fRes )
			{
				EXC_SET("writeval");
				if ( pArgs && pArgs->r_WriteVal(pszKey, sVal, pSrc) )
					fRes = true;
			}

			if ( !fRes )
			{
				DEBUG_ERR(("Can't resolve <%s>\n", pszKey));
				// Just in case this really is a <= operator?
				pszResponse[i] = chEnd;
			}

			if ( sVal.IsEmpty() && fHTML )
				sVal = "&nbsp";

			EXC_SET("mem shifting");
			size_t iLen = sVal.GetLength();
			memmove(pszResponse + iBegin + iLen, pszResponse + i + 1, strlen(pszResponse + i + 1) + 1);
			memcpy(pszResponse + iBegin, static_cast<LPCTSTR>(sVal), iLen);
			i = iBegin + iLen - 1;

			if ( (iFlags & 2) != 0 )	// just do this one then bail out
				return i;
		}
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("response '%s' source addr '0%p' flags '%d' args '%p'\n", pszResponse, static_cast<void *>(pSrc), iFlags, static_cast<void *>(pArgs));
	EXC_DEBUG_END;
	return i;
}

enum SSC_TYPE
{
	#define ADD(a,b) SSC_##a,
	#include "../tables/CScriptObj_functions.tbl"
	#undef ADD
	SSC_QTY
};

LPCTSTR const CScriptObj::sm_szLoadKeys[SSC_QTY + 1] =
{
	#define ADD(a,b) b,
	#include "../tables/CScriptObj_functions.tbl"
	#undef ADD
	NULL
};

enum SSV_TYPE
{
	SSV_NEW,
	SSV_NEWDUPE,
	SSV_NEWITEM,
	SSV_NEWNPC,
	SSV_OBJ,
	SSV_SHOW,
	SSV_QTY
};

LPCTSTR const CScriptObj::sm_szVerbKeys[SSV_QTY + 1] =
{
	"NEW",
	"NEWDUPE",
	"NEWITEM",
	"NEWNPC",
	"OBJ",
	"SHOW",
	NULL
};

static void StringFunction(int iFunc, LPCTSTR pszKey, CGString &sVal)
{
	GETNONWHITESPACE(pszKey);
	if ( *pszKey == '(' )
		++pszKey;

	TCHAR *ppCmd[4];
	size_t iCount = Str_ParseCmds(const_cast<TCHAR *>(pszKey), ppCmd, COUNTOF(ppCmd), ")");
	if ( iCount <= 0 )
	{
		DEBUG_ERR(("Bad string function usage. Missing ')'\n"));
		return;
	}

	switch ( iFunc )
	{
		case SSC_CHR:
			sVal.Format("%c", Exp_GetSingle(ppCmd[0]));
			return;
		case SSC_StrReverse:
			sVal = ppCmd[0];	// strreverse(str) = reverse the string
			sVal.Reverse();
			return;
		case SSC_StrToLower:	// strlower(str) = lower case the string
			sVal = ppCmd[0];
			sVal.MakeLower();
			return;
		case SSC_StrToUpper:	// strupper(str) = upper case the string
			sVal = ppCmd[0];
			sVal.MakeUpper();
			return;
	}
}

bool CScriptObj::r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef)
{
	ADDTOCALLSTACK("CScriptObj::r_GetRef");
	// A key name that just links to another object

	if ( !strnicmp(pszKey, "SERV.", 5) )
	{
		pszKey += 5;
		pRef = &g_Serv;
		return true;
	}
	else if ( !strnicmp(pszKey, "UID.", 4) )
	{
		pszKey += 4;
		CGrayUID uid = static_cast<CGrayUID>(Exp_GetLLVal(pszKey));
		pRef = uid.ObjFind();
		SKIP_SEPARATORS(pszKey);
		return true;
	}
	else if ( !strnicmp(pszKey, "OBJ.", 4) )
	{
		pszKey += 4;
		pRef = g_World.m_uidObj ? g_World.m_uidObj.ObjFind() : NULL;
		return true;
	}
	else if ( !strnicmp(pszKey, "NEW.", 4) )
	{
		pszKey += 4;
		pRef = g_World.m_uidNew ? g_World.m_uidNew.ObjFind() : NULL;
		return true;
	}
	else if ( !strnicmp(pszKey, "I.", 2) )
	{
		pszKey += 2;
		pRef = this;
		return true;
	}
	else if ( IsSetOF(OF_FileCommands) && !strnicmp(pszKey, "FILE.", 5) )
	{
		pszKey += 5;
		pRef = &g_Serv.fhFile;
		return true;
	}
	else if ( !strnicmp(pszKey, "DB.", 3) )
	{
		pszKey += 3;
		pRef = &g_Serv.m_hdb;
		return true;
	}
	else if ( !strnicmp(pszKey, "LDB.", 4) )
	{
		pszKey += 4;
		pRef = &g_Serv.m_hldb;
		return true;
	}
	return false;
}

bool CScriptObj::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CScriptObj::r_LoadVal");
	EXC_TRY("LoadVal");

	LPCTSTR pszKey = s.GetKey();
	if ( !strnicmp(pszKey, "CLEARVARS", 9) )
	{
		pszKey = s.GetArgStr();
		SKIP_SEPARATORS(pszKey);
		g_Exp.m_VarGlobals.ClearKeys(pszKey);
		return true;
	}

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	if ( index < 0 )
	{
		DEBUG_ERR(("Undefined keyword '%s'\n", s.GetKey()));
		return false;
	}

	switch ( static_cast<SSC_TYPE>(index) )
	{
		case SSC_VAR:
		{
			bool fQuoted = false;
			g_Exp.m_VarGlobals.SetStr(pszKey + 4, fQuoted, s.GetArgStr(&fQuoted), false);
			return true;
		}
		case SSC_VAR0:
		{
			bool fQuoted = false;
			g_Exp.m_VarGlobals.SetStr(pszKey + 5, fQuoted, s.GetArgStr(&fQuoted), true);
			return true;
		}
		case SSC_LIST:
		{
			if ( !g_Exp.m_ListGlobals.r_LoadVal(pszKey + 5, s) )
				DEBUG_ERR(("Unable to process command '%s %s'\n", pszKey, s.GetArgRaw()));
			return true;
		}
		case SSC_DEFMSG:
		{
			pszKey += 7;
			for ( size_t i = 0; i < DEFMSG_QTY; ++i )
			{
				if ( !strcmpi(pszKey, g_Exp.sm_szMsgNames[i]) )
				{
					bool fQuoted = false;
					TCHAR *pszArgs = s.GetArgStr(&fQuoted);
					strcpy(g_Exp.sm_szMessages[i], pszArgs);
					return true;
				}
			}
			g_Log.Event(LOGM_INIT|LOGL_ERROR, "Setting not used message override named '%s'\n", pszKey);
			return false;
		}
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CScriptObj::r_Load(CScript &s)
{
	ADDTOCALLSTACK("CScriptObj::r_Load");
	while ( s.ReadKeyParse() )
	{
		if ( s.IsKeyHead("ON", 2) )		// trigger scripting marks the end
			break;
		r_LoadVal(s);
	}
	return true;
}

bool CScriptObj::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CScriptObj::r_WriteVal");
	EXC_TRY("WriteVal");
	CObjBase *pObj;
	CScriptObj *pRef = NULL;
	bool fGetRef = r_GetRef(pszKey, pRef);

	if ( !strnicmp(pszKey, "GetRefType", 10) )
	{
		CScriptObj *pRefTemp = pRef ? pRef : pSrc->GetChar();
		if ( pRefTemp == &g_Serv )
			sVal.FormatHex(0x1);
		else if ( pRefTemp == &(g_Serv.fhFile) )
			sVal.FormatHex(0x2);
		else if ( (pRefTemp == &g_Serv.m_hdb) || dynamic_cast<CDataBase *>(pRefTemp) )
			sVal.FormatHex(0x8);
		else if ( dynamic_cast<CResourceDef *>(pRefTemp) )
			sVal.FormatHex(0x10);
		else if ( dynamic_cast<CResourceBase *>(pRefTemp) )
			sVal.FormatHex(0x20);
		else if ( dynamic_cast<CScriptTriggerArgs *>(pRefTemp) )
			sVal.FormatHex(0x40);
		else if ( dynamic_cast<CFileObj *>(pRefTemp) )
			sVal.FormatHex(0x80);
		else if ( dynamic_cast<CFileObjContainer *>(pRefTemp) )
			sVal.FormatHex(0x100);
		else if ( dynamic_cast<CAccount *>(pRefTemp) )
			sVal.FormatHex(0x200);
		else if ( (pRefTemp == &g_Serv.m_hldb) || dynamic_cast<CSQLite *>(pRefTemp) )	//else if ( dynamic_cast<CPartyDef *>(pRefTemp) )
			sVal.FormatHex(0x400);
		else if ( dynamic_cast<CStoneMember *>(pRefTemp) )
			sVal.FormatHex(0x800);
		else if ( dynamic_cast<CServerDef *>(pRefTemp) )
			sVal.FormatHex(0x1000);
		else if ( dynamic_cast<CSector *>(pRefTemp) )
			sVal.FormatHex(0x2000);
		else if ( dynamic_cast<CWorld *>(pRefTemp) )
			sVal.FormatHex(0x4000);
		else if ( dynamic_cast<CGMPage *>(pRefTemp) )
			sVal.FormatHex(0x8000);
		else if ( dynamic_cast<CClient *>(pRefTemp) )
			sVal.FormatHex(0x10000);
		else if ( (pObj = dynamic_cast<CObjBase *>(pRefTemp)) != NULL )
		{
			if ( dynamic_cast<CChar *>(pObj) )
				sVal.FormatHex(0x40000);
			else if ( dynamic_cast<CItem *>(pObj) )
				sVal.FormatHex(0x80000);
			else
				sVal.FormatHex(0x20000);
		}
		else
			sVal.FormatHex(0x1);
		return true;
	}

	if ( fGetRef )
	{
		if ( !pRef )	// good command but bad link
		{
			sVal.FormatVal(0);
			return true;
		}
		if ( pszKey[0] == '\0' )	// just testing the ref
		{
			pObj = dynamic_cast<CObjBase *>(pRef);
			if ( pObj )
				sVal.FormatHex(static_cast<DWORD>(pObj->GetUID()));
			else
				sVal.FormatVal(1);
			return true;
		}
		return pRef->r_WriteVal(pszKey, sVal, pSrc);
	}

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	if ( index < 0 )
	{
		if ( (*pszKey == 'd') || (*pszKey == 'D') )
		{
			// <dSOMEVAL> same as <eval <SOMEVAL>> to get dec from the val
			LPCTSTR pszArg = pszKey + 1;
			if ( r_WriteVal(pszArg, sVal, pSrc) )
			{
				if ( *sVal != '-' )
					sVal.FormatLLVal(ahextoi64(sVal));
				return true;
			}
		}
		else if ( (*pszKey == 'r') || (*pszKey == 'R') )
		{
			// <R>, <R15>, <R3,15> are shortcuts to rand(), rand(15) and rand(3,15)
			pszKey += 1;
			if ( *pszKey && ((*pszKey < '0') || (*pszKey > '9')) && (*pszKey != '-') )
				goto badcmd;

			INT64 iMin = 1000, iMax = LLONG_MIN;

			if ( *pszKey )
			{
				iMin = Exp_GetLLVal(pszKey);
				SKIP_ARGSEP(pszKey);
			}
			if ( *pszKey )
			{
				iMax = Exp_GetLLVal(pszKey);
				SKIP_ARGSEP(pszKey);
			}

			if ( iMax == LLONG_MIN )
			{
				iMax = iMin - 1;
				iMin = 0;
			}

			if ( iMin >= iMax )
				sVal.FormatLLVal(iMin);
			else
				sVal.FormatLLVal(Calc_GetRandLLVal2(iMin, iMax));
			return true;
		}
	badcmd:
		return false;
	}

	pszKey += strlen(sm_szLoadKeys[index]);
	SKIP_SEPARATORS(pszKey);
	bool fZero = false;

	switch ( static_cast<SSC_TYPE>(index) )
	{
		case SSC_BETWEEN:
		case SSC_BETWEEN2:
		{
			INT64 iMin = Exp_GetLLVal(pszKey);
			SKIP_ARGSEP(pszKey);
			INT64 iMax = Exp_GetLLVal(pszKey);
			SKIP_ARGSEP(pszKey);
			INT64 iCurrent = Exp_GetLLVal(pszKey);
			SKIP_ARGSEP(pszKey);
			INT64 iAbsMax = Exp_GetLLVal(pszKey);
			SKIP_ARGSEP(pszKey);
			if ( index == SSC_BETWEEN2 )
				iCurrent = iAbsMax - iCurrent;

			if ( (iMin >= iMax) || (iAbsMax <= 0) || (iCurrent <= 0) )
				sVal.FormatLLVal(iMin);
			else if ( iCurrent >= iAbsMax )
				sVal.FormatLLVal(iMax);
			else
				sVal.FormatLLVal((iCurrent * (iMax - iMin)) / iAbsMax + iMin);
			break;
		}
		case SSC_LISTCOL:
			sVal = (CWebPageDef::sm_iListIndex & 1) ? "bgcolor=\"#E8E8E8\"" : "";	// alternating color
			return true;
		case SSC_OBJ:
			if ( !g_World.m_uidObj.ObjFind() )
				g_World.m_uidObj = 0;
			sVal.FormatHex(static_cast<DWORD>(g_World.m_uidObj));
			return true;
		case SSC_NEW:
			if ( !g_World.m_uidNew.ObjFind() )
				g_World.m_uidNew = 0;
			sVal.FormatHex(static_cast<DWORD>(g_World.m_uidNew));
			return true;
		case SSC_SRC:
		{
			if ( !pSrc )
				pRef = NULL;
			else
			{
				pRef = pSrc->GetChar();
				if ( !pRef )
					pRef = dynamic_cast<CScriptObj *>(pSrc);
			}
			if ( !pRef )
			{
				sVal.FormatVal(0);
				return true;
			}
			if ( !*pszKey )
			{
				pObj = dynamic_cast<CObjBase *>(pRef);
				sVal.FormatHex(pObj ? static_cast<DWORD>(pObj->GetUID()) : 0);
				return true;
			}
			return pRef->r_WriteVal(pszKey, sVal, pSrc);
		}
		case SSC_VAR0:
			fZero = true;
		case SSC_VAR:
		{
			CVarDefCont *pVar = g_Exp.m_VarGlobals.GetKey(pszKey);
			if ( pVar )
				sVal = pVar->GetValStr();
			else if ( fZero )
				sVal.FormatVal(0);
			return true;
		}
		case SSC_DEFLIST:
			g_Exp.m_ListInternals.r_Write(pSrc, pszKey, sVal);
			return true;
		case SSC_LIST:
			g_Exp.m_ListGlobals.r_Write(pSrc, pszKey, sVal);
			return true;
		case SSC_DEF0:
			fZero = true;
		case SSC_DEF:
		{
			CVarDefCont *pVar = g_Exp.m_VarDefs.GetKey(pszKey);
			if ( pVar )
				sVal = pVar->GetValStr();
			else if ( fZero )
				sVal.FormatVal(0);
			return true;
		}
		case SSC_DEFMSG:
			sVal = g_Cfg.GetDefaultMsg(pszKey);
			return true;
		case SSC_EVAL:
			sVal.FormatLLVal(Exp_GetLLVal(pszKey));
			return true;
		case SSC_UVAL:
			sVal.FormatULLVal(static_cast<unsigned long long>(Exp_GetLLVal(pszKey)));
			return true;
		case SSC_FVAL:
		{
			INT64 iVal = Exp_GetLLVal(pszKey);
			sVal.Format("%s%lld.%lld", (iVal >= 0) ? "" : "-", llabs(iVal / 10), llabs(iVal % 10));
			return true;
		}
		case SSC_HVAL:
			sVal.FormatLLHex(Exp_GetLLVal(pszKey));
			return true;
		case SSC_FEVAL:		// Float EVAL
			sVal.FormatVal(ATOI(pszKey));
			break;
		case SSC_FHVAL:		// Float HVAL
			sVal.FormatHex(ATOI(pszKey));
			break;
		case SSC_FLOATVAL:	// Float math
			sVal = CVarFloat::FloatMath(pszKey);
			break;
		case SSC_QVAL:
		{
			// Statement <QVAL (condition) ? option1 : option2>
			TCHAR *ppCmd[3];
			ppCmd[0] = const_cast<TCHAR *>(pszKey);
			Str_Parse(ppCmd[0], &ppCmd[1], "?");
			Str_Parse(ppCmd[1], &ppCmd[2], ":");
			sVal = ppCmd[Exp_GetVal(ppCmd[0]) ? 1 : 2];
			if ( sVal.IsEmpty() )
				sVal = "";
			return true;
		}
		case SSC_ISBIT:
		case SSC_SETBIT:
		case SSC_CLRBIT:
		{
			GETNONWHITESPACE(pszKey);
			if ( !IsDigit(pszKey[0]) )
				return false;

			INT64 iVal = Exp_GetLLVal(pszKey);
			INT64 iBit = 0;
			if ( index != SSC_ISBIT )
			{
				SKIP_ARGSEP(pszKey);
				iBit = Exp_GetLLVal(pszKey);
				if ( iBit < 0 )
				{
					g_Log.EventWarn("%s(%lld,%lld): Can't shift bit by negative amount\n", sm_szLoadKeys[index], iVal, iBit);
					iBit = 0;
				}
			}

			if ( index == SSC_ISBIT )
				sVal.FormatLLVal(iVal & (static_cast<INT64>(1) << iBit));
			else if ( index == SSC_SETBIT )
				sVal.FormatLLVal(iVal | (static_cast<INT64>(1) << iBit));
			else
				sVal.FormatLLVal(iVal & ~(static_cast<INT64>(1) << iBit));
			break;
		}
		case SSC_ISEMPTY:
			sVal.FormatVal(IsStrEmpty(pszKey));
			return true;
		case SSC_ISNUM:
		{
			GETNONWHITESPACE(pszKey);
			if ( *pszKey == '-' )
				++pszKey;
			sVal.FormatVal(IsStrNumeric(pszKey));
			return true;
		}
		case SSC_StrPos:
		{
			GETNONWHITESPACE(pszKey);
			INT64 iPos = Exp_GetLLVal(pszKey);
			TCHAR szVal;
			if ( IsDigit(*pszKey) && IsDigit(*(pszKey + 1)) )
				szVal = static_cast<TCHAR>(Exp_GetLLVal(pszKey));
			else
			{
				szVal = *pszKey;
				++pszKey;
			}

			GETNONWHITESPACE(pszKey);
			INT64 iLen = strlen(pszKey);
			if ( iPos < 0 )
				iPos = iLen + iPos;
			if ( iPos < 0 )
				iPos = 0;
			else if ( iPos > iLen )
				iPos = iLen;

			TCHAR *pszPos = const_cast<TCHAR *>(strchr(pszKey + iPos, szVal));
			if ( pszPos )
				sVal.FormatVal(static_cast<long>(pszPos - pszKey));
			else
				sVal.FormatVal(-1);
			return true;
		}
		case SSC_StrSub:
		{
			TCHAR *ppArgs[3];
			size_t iQty = Str_ParseCmds(const_cast<TCHAR *>(pszKey), ppArgs, COUNTOF(ppArgs));
			if ( iQty < 3 )
				return false;

			INT64 iPos = Exp_GetLLVal(ppArgs[0]);
			INT64 iCnt = Exp_GetLLVal(ppArgs[1]);
			if ( iCnt < 0 )
				return false;

			INT64 iLen = strlen(ppArgs[2]);
			if ( iPos < 0 )
				iPos += iLen;
			if ( (iPos > iLen) || (iPos < 0) )
				iPos = 0;
			if ( (iPos + iCnt > iLen) || (iCnt == 0) )
				iCnt = iLen - iPos;

			TCHAR *pszBuffer = Str_GetTemp();
			strncpy(pszBuffer, ppArgs[2] + iPos, static_cast<size_t>(iCnt));
			pszBuffer[iCnt] = '\0';

			if ( g_Cfg.m_wDebugFlags & DEBUGF_SCRIPTS )
				g_Log.EventDebug("SCRIPT: strsub(%lld,%lld,'%s') -> '%s'\n", iPos, iCnt, ppArgs[2], pszBuffer);

			sVal = pszBuffer;
			return true;
		}
		case SSC_StrArg:
		{
			TCHAR *pszBuffer = Str_GetTemp();
			GETNONWHITESPACE(pszKey);
			if ( *pszKey == '"' )
				++pszKey;

			size_t iLen = 0;
			while ( *pszKey && !IsSpace(*pszKey) && (*pszKey != ',') )
			{
				pszBuffer[iLen] = *pszKey;
				++pszKey;
				++iLen;
			}
			pszBuffer[iLen] = '\0';
			sVal = pszBuffer;
			return true;
		}
		case SSC_StrEat:
		{
			GETNONWHITESPACE(pszKey);
			while ( *pszKey && !IsSpace(*pszKey) && (*pszKey != ',') )
				++pszKey;

			SKIP_ARGSEP(pszKey);
			sVal = pszKey;
			return true;
		}
		case SSC_StrTrim:
			sVal = *pszKey ? Str_TrimWhitespace(const_cast<TCHAR *>(pszKey)) : "";
			return true;
		case SSC_ASC:
		{
			REMOVE_QUOTES(pszKey);
			sVal.FormatLLHex(*pszKey);

			TCHAR *pszBuffer = Str_GetTemp();
			strncpy(pszBuffer, sVal, SCRIPT_MAX_LINE_LEN - 1);

			while ( *(++pszKey) )
			{
				if ( *pszKey == '"' )
					break;
				sVal.FormatLLHex(*pszKey);
				strcat(pszBuffer, " ");
				strncat(pszBuffer, sVal, SCRIPT_MAX_LINE_LEN - 1);
			}
			sVal = pszBuffer;
			return true;
		}
		case SSC_ASCPAD:
		{
			TCHAR *ppArgs[2];
			size_t iQty = Str_ParseCmds(const_cast<TCHAR *>(pszKey), ppArgs, COUNTOF(ppArgs));
			if ( iQty < 2 )
				return false;

			INT64 iPad = Exp_GetLLVal(ppArgs[0]);
			if ( iPad < 0 )
				return false;

			REMOVE_QUOTES(ppArgs[1]);
			sVal.FormatLLHex(*ppArgs[1]);

			TCHAR *pszBuffer = Str_GetTemp();
			strncpy(pszBuffer, sVal, SCRIPT_MAX_LINE_LEN - 1);

			while ( --iPad )
			{
				if ( *ppArgs[1] == '"' )
					continue;
				if ( *ppArgs[1] )
				{
					++ppArgs[1];
					sVal.FormatLLHex(*ppArgs[1]);
				}
				else
					sVal.FormatLLHex('\0');

				strcat(pszBuffer, " ");
				strncat(pszBuffer, sVal, SCRIPT_MAX_LINE_LEN - 1);
			}
			sVal = pszBuffer;
			return true;
		}
		case SSC_SYSCMD:
		case SSC_SYSSPAWN:
		{
			if ( !IsSetOF(OF_FileCommands) )
				return false;

			GETNONWHITESPACE(pszKey);
			TCHAR *ppCmd[10];	// limit to 10 arguments
			TCHAR *pszBuffer = Str_GetTemp();
			strcpy(pszBuffer, pszKey);
			size_t iQty = Str_ParseCmds(pszBuffer, ppCmd, COUNTOF(ppCmd));
			if ( iQty < 1 )
				return false;

			bool fWait = (index == SSC_SYSCMD);
#ifdef _WIN32
			_spawnl(fWait ? _P_WAIT : _P_NOWAIT, ppCmd[0], ppCmd[0], ppCmd[1], ppCmd[2], ppCmd[3], ppCmd[4], ppCmd[5], ppCmd[6], ppCmd[7], ppCmd[8], ppCmd[9], NULL);
#else

			// I think fork will cause problems.. we'll see.. if yes new thread + execlp is required.
			pid_t child_pid = vfork();
			if ( child_pid < 0 )
			{
				g_Log.EventError("%s failed when executing '%s'\n", sm_szLoadKeys[index], pszKey);
				return false;
			}
			else if ( child_pid == 0 )
			{
				// Don't touch this :P
				execlp(ppCmd[0], ppCmd[0], ppCmd[1], ppCmd[2], ppCmd[3], ppCmd[4], ppCmd[5], ppCmd[6], ppCmd[7], ppCmd[8], ppCmd[9], NULL);

				g_Log.EventError("%s failed with error %d (\"%s\") when executing '%s'\n", sm_szLoadKeys[index], errno, strerror(errno), pszKey);
				raise(SIGKILL);
				g_Log.EventError("%s failed to handle error. Server is UNSTABLE\n", sm_szLoadKeys[index]);

				while ( true )
				{
					// Do NOT leave until the process receives SIGKILL, otherwise it will free up resources it inherited
					// from the main process, which will mess everything up. Normally this point should never be reached
				}
			}
			else if ( fWait )	// parent process here (do we have to wait?)
			{
				int status;
				do
				{
					if ( waitpid(child_pid, &status, 0) )
						break;
				} while ( !WIFSIGNALED(status) && !WIFEXITED(status) );
				sVal.FormatLLHex(WEXITSTATUS(status));
			}
#endif
			g_Log.EventDebug("Process execution finished\n");
			return true;
		}
		case SSC_EXPLODE:
		{
			GETNONWHITESPACE(pszKey);
			char chSeparators[16];
			strcpylen(chSeparators, pszKey, 16);
			{
				char *p = chSeparators;
				while ( *p && (*p != ',') )
					++p;
				*p = 0;
			}

			const char *p = pszKey + strlen(chSeparators) + 1;
			sVal = "";
			if ( (p > pszKey) && *p )		// list of accessible separators 
			{
				TCHAR *ppCmd[255];
				TCHAR *z = Str_GetTemp();
				strncpy(z, p, SCRIPT_MAX_LINE_LEN - 1);
				size_t iCount = Str_ParseCmds(z, ppCmd, COUNTOF(ppCmd), chSeparators);
				if ( iCount > 0 )
				{
					sVal.Add(ppCmd[0]);
					for ( size_t i = 1; i < iCount; ++i )
					{
						sVal.Add(',');
						sVal.Add(ppCmd[i]);
					}
				}
			}
			return true;
		}
		case SSC_MD5HASH:
		{
			GETNONWHITESPACE(pszKey);
			char digest[33];
			CMD5::fastDigest(digest, pszKey);
			sVal.Format("%s", digest);
			return true;
		}
		case SSC_MULDIV:
		{
			INT64 iNum = Exp_GetLLVal(pszKey);
			SKIP_ARGSEP(pszKey);
			INT64 iMul = Exp_GetLLVal(pszKey);
			SKIP_ARGSEP(pszKey);
			INT64 iDiv = Exp_GetLLVal(pszKey);
			INT64 iRes = 0;

			if ( iDiv == 0 )
				g_Log.EventWarn("MULDIV(%lld,%lld,%lld): Can't divide by '0'\n", iNum, iMul, iDiv);
			else
				iRes = IMULDIV(iNum, iMul, iDiv);

			if ( g_Cfg.m_wDebugFlags & DEBUGF_SCRIPTS )
				g_Log.EventDebug("SCRIPT: muldiv(%lld,%lld,%lld) -> %lld\n", iNum, iMul, iDiv, iRes);

			sVal.FormatLLVal(iRes);
			return true;
		}
		case SSC_StrRegexNew:
		{
			TCHAR *pszToMatch = Str_GetTemp();
			size_t iLen = Exp_GetVal(pszKey);
			if ( iLen > 0 )
			{
				SKIP_ARGSEP(pszKey);
				strcpylen(pszToMatch, pszKey, iLen + 1);
				pszKey += iLen;
			}

			SKIP_ARGSEP(pszKey);
			TCHAR *pszLastError = Str_GetTemp();
			int iResult = Str_RegExMatch(pszKey, pszToMatch, pszLastError);
			sVal.FormatVal(iResult);

			if ( iResult == -1 )
				DEBUG_ERR(("STRREGEX: Bad function usage. Error: %s\n", pszLastError));
			return true;
		}
		default:
			StringFunction(index, pszKey, sVal);
			return true;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CScriptObj::r_Verb(CScript &s, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CScriptObj::r_Verb");
	// Execute command from script

	EXC_TRY("Verb");
	ASSERT(pSrc);
	LPCTSTR pszKey = s.GetKey();
	CScriptObj *pRef = NULL;
	if ( r_GetRef(pszKey, pRef) )
	{
		if ( pszKey[0] )
		{
			if ( !pRef )
				return true;
			CScript script(pszKey, s.GetArgStr());
			return pRef->r_Verb(script, pSrc);
		}
		// else just fall through. as they seem to be setting the pointer !?
	}

	if ( s.IsKeyHead("SRC.", 4) )
	{
		pszKey += 4;
		pRef = dynamic_cast<CScriptObj *>(pSrc->GetChar());
		if ( !pRef )
		{
			pRef = dynamic_cast<CScriptObj *>(pSrc);
			if ( !pRef )
				return false;
		}
		CScript script(pszKey, s.GetArgStr());
		return pRef->r_Verb(script, pSrc);
	}

	SSV_TYPE index = static_cast<SSV_TYPE>(FindTableSorted(s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1));
	switch ( index )
	{
		case SSV_OBJ:
		{
			g_World.m_uidObj = s.GetArgVal();
			if ( !g_World.m_uidObj.ObjFind() )
				g_World.m_uidObj = 0;
			break;
		}
		case SSV_NEW:
		{
			g_World.m_uidNew = s.GetArgVal();
			if ( !g_World.m_uidNew.ObjFind() )
				g_World.m_uidNew = 0;
			break;
		}
		case SSV_NEWDUPE:
		{
			CGrayUID uid(s.GetArgVal());
			CObjBase *pObj = uid.ObjFind();
			if ( !pObj )
			{
				g_World.m_uidNew = 0;
				return false;
			}

			g_World.m_uidNew = uid;
			CScript script("DUPE");
			bool fRes = pObj->r_Verb(script, pSrc);

			if ( this != &g_Serv )
			{
				CChar *pChar = dynamic_cast<CChar *>(this);
				if ( pChar )
					pChar->m_Act_Targ = g_World.m_uidNew;
				else
				{
					CClient *pClient = dynamic_cast<CClient *>(this);
					if ( pClient && pClient->GetChar() )
						pClient->GetChar()->m_Act_Targ = g_World.m_uidNew;
				}
			}
			return fRes;
		}
		case SSV_NEWITEM:
		{
			// Just create the item but don't put it anyplace yet
			TCHAR *ppCmd[4];
			size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppCmd, COUNTOF(ppCmd), ",");
			if ( iQty <= 0 )
				return false;

			CItem *pItem = CItem::CreateHeader(ppCmd[0], NULL, false, pSrc->GetChar());
			if ( !pItem )
			{
				g_World.m_uidNew = static_cast<CGrayUID>(UID_CLEAR);
				return false;
			}

			if ( ppCmd[1] )
				pItem->SetAmount(static_cast<WORD>(Exp_GetLLVal(ppCmd[1])));

			if ( ppCmd[2] )
			{
				CGrayUID uidEquipper = static_cast<CGrayUID>(Exp_GetLLVal(ppCmd[2]));
				bool fTriggerEquip = ppCmd[3] ? (Exp_GetLLVal(ppCmd[3]) != 0) : false;

				if ( !fTriggerEquip || uidEquipper.IsItem() )
					pItem->LoadSetContainer(uidEquipper, LAYER_NONE);
				else
				{
					if ( fTriggerEquip )
					{
						CChar *pCharEquipper = uidEquipper.CharFind();
						if ( pCharEquipper )
							pCharEquipper->ItemEquip(pItem);
					}
				}
			}

			g_World.m_uidNew = pItem->GetUID();

			if ( this != &g_Serv )
			{
				CChar *pChar = dynamic_cast<CChar *>(this);
				if ( pChar )
					pChar->m_Act_Targ = g_World.m_uidNew;
				else
				{
					CClient *pClient = dynamic_cast<CClient *>(this);
					if ( pClient && pClient->GetChar() )
						pClient->GetChar()->m_Act_Targ = g_World.m_uidNew;
				}
			}
			break;
		}
		case SSV_NEWNPC:
		{
			CREID_TYPE id = static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType(RES_CHARDEF, s.GetArgRaw()));
			CChar *pChar = CChar::CreateNPC(id);
			if ( !pChar )
			{
				g_World.m_uidNew = static_cast<CGrayUID>(UID_CLEAR);
				return false;
			}

			g_World.m_uidNew = pChar->GetUID();

			if ( this != &g_Serv )
			{
				pChar = dynamic_cast<CChar *>(this);
				if ( pChar )
					pChar->m_Act_Targ = g_World.m_uidNew;
				else
				{
					const CClient *pClient = dynamic_cast<CClient *>(this);
					if ( pClient && pClient->GetChar() )
						pClient->GetChar()->m_Act_Targ = g_World.m_uidNew;
				}
			}
			break;
		}
		case SSV_SHOW:
		{
			CGString sVal;
			if ( !r_WriteVal(s.GetArgStr(), sVal, pSrc) )
				return false;
			TCHAR *pszMsg = Str_GetTemp();
			sprintf(pszMsg, "'%s' for '%s' is '%s'\n", static_cast<LPCTSTR>(s.GetArgStr()), GetName(), static_cast<LPCTSTR>(sVal));
			pSrc->SysMessage(pszMsg);
			break;
		}
		default:
			return r_LoadVal(s);		// default to loading values
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

bool CScriptObj::r_Call(LPCTSTR pszFunction, CTextConsole *pSrc, CScriptTriggerArgs *pArgs, CGString *psVal, TRIGRET_TYPE *piRet)
{
	ADDTOCALLSTACK("CScriptObj::r_Call");
	GETNONWHITESPACE(pszFunction);

	int iCompareRes = -1;
	size_t index = g_Cfg.m_Functions.FindKeyNear(pszFunction, iCompareRes, true);
	if ( (iCompareRes != 0) || (index == g_Cfg.m_Functions.BadIndex()) )
		return false;

	CResourceNamed *pFunction = static_cast<CResourceNamed *>(g_Cfg.m_Functions[index]);
	ASSERT(pFunction);
	CResourceLock sFunction;
	if ( pFunction->ResourceLock(sFunction) )
	{
		TScriptProfiler::TScriptProfilerFunction *pFun = NULL;
		ULONGLONG llTicksStart, llTicksEnd;

		if ( IsSetEF(EF_Script_Profiler) )
		{
			// Lowercase for speed, and strip arguments
			char *pchName = Str_GetTemp();
			char *pchSpace;
			strncpy(pchName, pszFunction, sizeof(pFun->name) - 1);
			if ( (pchSpace = strchr(pchName, ' ')) != NULL )
				*pchSpace = 0;
			_strlwr(pchName);

			if ( g_profiler.initstate != 0xF1 )		// profiler is not initialized
			{
				memset(&g_profiler, 0, sizeof(g_profiler));
				g_profiler.initstate = static_cast<BYTE>(0xF1);		// ''
			}

			for ( pFun = g_profiler.FunctionsHead; pFun != NULL; pFun = pFun->next )
			{
				if ( !strcmp(pFun->name, pchName) )
					break;
			}
			if ( !pFun )
			{
				// First time that the function is called, so create its record
				pFun = new TScriptProfiler::TScriptProfilerFunction;
				memset(pFun, 0, sizeof(TScriptProfiler::TScriptProfilerFunction));
				strncpy(pFun->name, pchName, sizeof(pFun->name) - 1);
				if ( g_profiler.FunctionsTail )
					g_profiler.FunctionsTail->next = pFun;
				else
					g_profiler.FunctionsHead = pFun;
				g_profiler.FunctionsTail = pFun;
			}

			pFun->called++;
			g_profiler.called++;
			TIME_PROFILE_START;
		}

		TRIGRET_TYPE iRet = OnTriggerRun(sFunction, TRIGRUN_SECTION_TRUE, pSrc, pArgs, psVal);

		if ( IsSetEF(EF_Script_Profiler) )
		{
			TIME_PROFILE_END;
			llTicksStart = llTicksEnd - llTicksStart;
			pFun->total += llTicksStart;
			pFun->average = pFun->total / pFun->called;
			if ( pFun->max < llTicksStart )
				pFun->max = llTicksStart;
			if ( (pFun->min > llTicksStart) || !pFun->min )
				pFun->min = llTicksStart;
			g_profiler.total += llTicksStart;
		}

		if ( piRet )
			*piRet = iRet;
	}
	return true;
}

///////////////////////////////////////////////////////////
// CScriptTriggerArgs

CScriptTriggerArgs::CScriptTriggerArgs(LPCTSTR pszStr)
{
	Init(pszStr);
}

void CScriptTriggerArgs::Init(LPCTSTR pszStr)
{
	ADDTOCALLSTACK("CScriptTriggerArgs::Init");
	m_iN1 = 0;
	m_iN2 = 0;
	m_iN3 = 0;
	m_pO1 = NULL;

	if ( !pszStr )
		pszStr = "";

	// Raw is left untouched for now - it'll be split the 1st time argv is accessed
	m_s1_raw = pszStr;
	bool fQuote = false;
	if ( *pszStr == '"' )
	{
		fQuote = true;
		++pszStr;
	}

	m_s1 = pszStr;

	// Take quote if present
	if ( fQuote )
	{
		TCHAR *str = const_cast<TCHAR *>(strchr(m_s1.GetPtr(), '"'));
		if ( str )
			*str = '\0';
	}

	// Attempt to parse this
	if ( IsDigit(*pszStr) || ((*pszStr == '-') && IsDigit(*(pszStr + 1))) )
	{
		m_iN1 = Exp_GetSingle(pszStr);
		SKIP_ARGSEP(pszStr);
		if ( IsDigit(*pszStr) || ((*pszStr == '-') && IsDigit(*(pszStr + 1))) )
		{
			m_iN2 = Exp_GetSingle(pszStr);
			SKIP_ARGSEP(pszStr);
			if ( IsDigit(*pszStr) || ((*pszStr == '-') && IsDigit(*(pszStr + 1))) )
				m_iN3 = Exp_GetSingle(pszStr);
		}
	}

	// ensure argv will be recalculated next time it is accessed
	m_v.SetCount(0);
}

enum AGC_TYPE
{
	AGC_N,
	AGC_N1,
	AGC_N2,
	AGC_N3,
	AGC_O,
	AGC_S,
	AGC_V,
	AGC_LVAR,
	AGC_TRY,
	AGC_TRYSRV,
	AGC_QTY
};

LPCTSTR const CScriptTriggerArgs::sm_szLoadKeys[AGC_QTY + 1] =
{
	"ARGN",
	"ARGN1",
	"ARGN2",
	"ARGN3",
	"ARGO",
	"ARGS",
	"ARGV",
	"LOCAL",
	"TRY",
	"TRYSRV",
	NULL
};

bool CScriptTriggerArgs::r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef)
{
	ADDTOCALLSTACK("CScriptTriggerArgs::r_GetRef");

	if ( !strnicmp(pszKey, "ARGO.", 5) )
	{
		pszKey += 5;
		if ( *pszKey == '1' )
			++pszKey;
		pRef = m_pO1;
		return true;
	}
	else if ( !strnicmp(pszKey, "REF", 3) )
	{
		LPCTSTR pszTemp = pszKey;
		pszTemp += 3;
		if ( *pszTemp && IsDigit(*pszTemp) )
		{
			char *pchEnd;
			WORD wNumber = static_cast<WORD>(strtol(pszTemp, &pchEnd, 10));
			if ( wNumber > 0 )	// can only use 1 to 65535 as REFx
			{
				// Make sure REFx or REFx.KEY is being used
				pszTemp = pchEnd;
				if ( !*pszTemp || (*pszTemp == '.') )
				{
					if ( *pszTemp == '.' )
						++pszTemp;

					pRef = m_VarObjs.Get(wNumber);
					pszKey = pszTemp;
					return true;
				}
			}
		}
	}
	return false;
}

bool CScriptTriggerArgs::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CScriptTriggerArgs::r_LoadVal");
	UNREFERENCED_PARAMETER(s);
	return false;
}

bool CScriptTriggerArgs::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CScriptTriggerArgs::r_WriteVal");
	EXC_TRY("WriteVal");
	if ( IsSetEF(EF_Intrinsic_Locals) )
	{
		EXC_SET("intrinsic");
		CVarDefCont *pVar = m_VarsLocal.GetKey(pszKey);
		if ( pVar )
		{
			sVal = pVar->GetValStr();
			return true;
		}
	}
	else if ( !strnicmp("LOCAL.", pszKey, 6) )
	{
		EXC_SET("local");
		pszKey += 6;
		sVal = m_VarsLocal.GetKeyStr(pszKey, true);
		return true;
	}

	if ( !strnicmp("FLOAT.", pszKey, 6) )
	{
		EXC_SET("float");
		pszKey += 6;
		sVal = m_VarsFloat.Get(pszKey);
		return true;
	}
	else if ( !strnicmp(pszKey, "ARGV", 4) )
	{
		EXC_SET("argv");
		pszKey += 4;
		SKIP_SEPARATORS(pszKey);

		size_t iQty = m_v.GetCount();
		if ( iQty <= 0 )
		{
			// Parse it here
			TCHAR *pszArgs = const_cast<TCHAR *>(m_s1_raw.GetPtr());
			TCHAR *s = pszArgs;
			bool fQuotes = false;
			bool fInerQuotes = false;
			while ( *s )
			{
				if ( IsSpace(*s) )	// ignore leading spaces
				{
					++s;
					continue;
				}
				if ( (*s == ',') && !fQuotes )	// add empty arguments if they are provided
				{
					m_v.Add(NULL);
					++s;
					continue;
				}
				if ( *s == '"' )	// check to see if the argument is quoted (in case it contains commas)
				{
					++s;
					fQuotes = true;
					fInerQuotes = false;
				}

				pszArgs = s;	// arg starts here
				++s;

				while ( *s )
				{
					if ( (*s == '"') && fQuotes )
					{
						*s = '\0';
						fQuotes = false;
					}
					else if ( (*s == '"') )
						fInerQuotes = !fInerQuotes;

					if ( !fQuotes && !fInerQuotes && (*s == ',') )
					{
						*s = '\0';
						++s;
						break;
					}
					++s;
				}
				m_v.Add(pszArgs);
			}
			iQty = m_v.GetCount();
		}

		if ( *pszKey == '\0' )
		{
			sVal.FormatVal(static_cast<long>(iQty));
			return true;
		}

		INT64 iKey = Exp_GetLLSingle(pszKey);
		if ( (iKey < 0) || !m_v.IsValidIndex(static_cast<size_t>(iKey)) )
		{
			sVal = "";
			return true;
		}

		sVal = m_v.GetAt(static_cast<size_t>(iKey));
		return true;
	}

	EXC_SET("generic");
	AGC_TYPE index = static_cast<AGC_TYPE>(FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1));
	switch ( index )
	{
		case AGC_N:
		case AGC_N1:
			sVal.FormatLLVal(m_iN1);
			break;
		case AGC_N2:
			sVal.FormatLLVal(m_iN2);
			break;
		case AGC_N3:
			sVal.FormatLLVal(m_iN3);
			break;
		case AGC_O:
		{
			CObjBase *pObj = dynamic_cast<CObjBase *>(m_pO1);
			if ( pObj )
				sVal.FormatHex(pObj->GetUID());
			else
				sVal.FormatVal(0);
			break;
		}
		case AGC_S:
			sVal = m_s1;
			break;
		default:
			return CScriptObj::r_WriteVal(pszKey, sVal, pSrc);
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CScriptTriggerArgs::r_Verb(CScript &s, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CScriptTriggerArgs::r_Verb");
	EXC_TRY("Verb");
	LPCTSTR pszKey = s.GetKey();
	int index = -1;

	if ( !strnicmp("FLOAT.", pszKey, 6) )
		return m_VarsFloat.Insert(pszKey + 6, s.GetArgStr(), true);
	else if ( !strnicmp("LOCAL.", pszKey, 6) )
	{
		bool fQuoted = false;
		m_VarsLocal.SetStr(s.GetKey() + 6, fQuoted, s.GetArgStr(&fQuoted), false);
		return true;
	}
	else if ( !strnicmp("REF", pszKey, 3) )
	{
		LPCTSTR pszTemp = pszKey;
		pszTemp += 3;
		if ( *pszTemp && IsDigit(*pszTemp) )
		{
			char *pchEnd;
			WORD wNumber = static_cast<WORD>(strtol(pszTemp, &pchEnd, 10));
			if ( wNumber > 0 )	// can only use 1 to 65535 as REFs
			{
				pszTemp = pchEnd;
				if ( !*pszTemp )	// setting REFx to a new object
				{
					CGrayUID uid = s.GetArgVal();
					CObjBase *pObj = uid.ObjFind();
					m_VarObjs.Insert(wNumber, pObj, true);
					pszKey = pszTemp;
					return true;
				}
				else if ( *pszTemp == '.' )	// accessing REFx object
				{
					pszKey = ++pszTemp;
					CObjBase *pObj = m_VarObjs.Get(wNumber);
					if ( !pObj )
						return false;

					CScript script(pszKey, s.GetArgStr());
					return pObj->r_Verb(script, pSrc);
				}
			}
		}
	}
	else if ( !strnicmp(pszKey, "ARGO", 4) )
	{
		pszKey += 4;
		if ( !strnicmp(pszKey, ".", 1) )
			index = AGC_O;
		else
		{
			++pszKey;
			CObjBase *pObj = static_cast<CObjBase *>(static_cast<CGrayUID>(Exp_GetSingle(pszKey)).ObjFind());
			m_pO1 = pObj ? pObj : NULL;
			return true;
		}
	}
	else
		index = FindTableSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);

	switch ( static_cast<AGC_TYPE>(index) )
	{
		case AGC_N:
		case AGC_N1:
			m_iN1 = s.GetArgVal();
			return true;
		case AGC_N2:
			m_iN2 = s.GetArgVal();
			return true;
		case AGC_N3:
			m_iN3 = s.GetArgVal();
			return true;
		case AGC_S:
			Init(s.GetArgStr());
			return true;
		case AGC_O:
		{
			LPCTSTR pszTemp = s.GetKey() + strlen(sm_szLoadKeys[AGC_O]);
			if ( *pszTemp == '.' )
			{
				++pszTemp;
				if ( !m_pO1 )
					return false;

				CScript script(pszTemp, s.GetArgStr());
				return m_pO1->r_Verb(script, pSrc);
			}
			return false;
		}
		case AGC_TRY:
		case AGC_TRYSRV:
		{
			CScript try_script(s.GetArgStr());
			if ( r_Verb(try_script, pSrc) )
				return true;
		}
		default:
			return false;
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

///////////////////////////////////////////////////////////
// CFileObj

CFileObj::CFileObj()
{
	m_pFile = new CFileText();
	m_pszBuffer = new TCHAR[SCRIPT_MAX_LINE_LEN];
	m_psWriteBuffer = new CGString();
	SetDefaultMode();
}

CFileObj::~CFileObj()
{
	if ( m_pFile->IsFileOpen() )
		m_pFile->Close();

	delete m_psWriteBuffer;
	delete[] m_pszBuffer;
	delete m_pFile;
}

void CFileObj::SetDefaultMode()
{
	ADDTOCALLSTACK("CFileObj::SetDefaultMode");
	m_fAppend = true;
	m_fCreate = false;
	m_fRead = true;
	m_fWrite = true;
}

bool CFileObj::FileOpen(LPCTSTR pszPath)
{
	ADDTOCALLSTACK("CFileObj::FileOpen");
	if ( m_pFile->IsFileOpen() )
		return false;

	UINT uMode = OF_SHARE_DENY_NONE|OF_TEXT;
	if ( m_fCreate )	// if we create, we can't append or read
		uMode |= OF_CREATE;
	else if ( (m_fRead && m_fWrite) || m_fAppend )
		uMode |= OF_READWRITE;
	else if ( m_fRead )
		uMode |= OF_READ;
	else if ( m_fWrite )
		uMode |= OF_WRITE;

	return m_pFile->Open(pszPath, uMode);
}

TCHAR *CFileObj::GetReadBuffer(bool fDelete)
{
	ADDTOCALLSTACK("CFileObj::GetReadBuffer");
	if ( fDelete )
		memset(m_pszBuffer, 0, SCRIPT_MAX_LINE_LEN);
	else
		*m_pszBuffer = 0;
	return m_pszBuffer;
}

CGString *CFileObj::GetWriteBuffer()
{
	ADDTOCALLSTACK("CFileObj::GetWriteBuffer");
	if ( !m_psWriteBuffer )
		m_psWriteBuffer = new CGString();

	m_psWriteBuffer->Empty(m_psWriteBuffer->GetLength() > SCRIPT_MAX_LINE_LEN / 4);
	return m_psWriteBuffer;
}

bool CFileObj::OnTick()
{
	ADDTOCALLSTACK("CFileObj::OnTick");
	return true;
}

int CFileObj::FixWeirdness()
{
	ADDTOCALLSTACK("CFileObj::FixWeirdness");
	return 0;
}

bool CFileObj::IsInUse()
{
	ADDTOCALLSTACK("CFileObj::IsInUse");
	return m_pFile->IsFileOpen();
}

void CFileObj::FlushAndClose()
{
	ADDTOCALLSTACK("CFileObj::FlushAndClose");
	if ( m_pFile->IsFileOpen() )
	{
		m_pFile->Flush();
		m_pFile->Close();
	}
}

enum FO_TYPE
{
	#define ADD(a,b) FO_##a,
	#include "../tables/CFile_props.tbl"
	#undef ADD
	FO_QTY
};

LPCTSTR const CFileObj::sm_szLoadKeys[FO_QTY + 1] =
{
	#define ADD(a,b) b,
	#include "../tables/CFile_props.tbl"
	#undef ADD
	NULL
};

enum FOV_TYPE
{
	#define ADD(a,b) FOV_##a,
	#include "../tables/CFile_functions.tbl"
	#undef ADD
	FOV_QTY
};

LPCTSTR const CFileObj::sm_szVerbKeys[FOV_QTY + 1] =
{
	#define ADD(a,b) b,
	#include "../tables/CFile_functions.tbl"
	#undef ADD
	NULL
};

bool CFileObj::r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef)
{
	UNREFERENCED_PARAMETER(pszKey);
	UNREFERENCED_PARAMETER(pRef);
	return false;
}

bool CFileObj::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CFileObj::r_LoadVal");
	EXC_TRY("LoadVal");
	LPCTSTR pszKey = s.GetKey();

	if ( !strnicmp("MODE.", pszKey, 5) )
	{
		pszKey += 5;
		if ( !m_pFile->IsFileOpen() )
		{
			if ( !strnicmp("APPEND", pszKey, 6) )
			{
				m_fAppend = (s.GetArgVal() != 0);
				m_fCreate = false;
			}
			else if ( !strnicmp("CREATE", pszKey, 6) )
			{
				m_fCreate = (s.GetArgVal() != 0);
				m_fAppend = false;
			}
			else if ( !strnicmp("READFLAG", pszKey, 8) )
				m_fRead = (s.GetArgVal() != 0);
			else if ( !strnicmp("WRITEFLAG", pszKey, 9) )
				m_fWrite = (s.GetArgVal() != 0);
			else if ( !strnicmp("SETDEFAULT", pszKey, 7) )
				SetDefaultMode();
			else
				return false;

			return true;
		}
		else
			g_Log.Event(LOGL_ERROR, "FILE (%s): Cannot set mode after file opening\n", static_cast<LPCTSTR>(m_pFile->GetFilePath()));
		return false;
	}

	FO_TYPE index = static_cast<FO_TYPE>(FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1));
	switch ( index )
	{
		case FO_WRITE:
		case FO_WRITECHR:
		case FO_WRITELINE:
		{
			if ( !m_pFile->IsFileOpen() )
			{
				g_Log.Event(LOGL_ERROR, "FILE: Cannot write content. Open the file first\n");
				return false;
			}

			if ( !s.HasArgs() )
				return false;

			CGString *psArgs = GetWriteBuffer();
			if ( index == FO_WRITELINE )
			{
				psArgs->Copy(s.GetArgStr());
#ifdef _WIN32
				psArgs->Add("\r\n");
#else
				psArgs->Add("\n");
#endif
			}
			else if ( index == FO_WRITECHR )
				psArgs->Format("%c", static_cast<TCHAR>(s.GetArgVal()));
			else
				psArgs->Copy(s.GetArgStr());

			bool fSuccess;
			if ( index == FO_WRITECHR )
				fSuccess = m_pFile->Write(psArgs->GetPtr(), 1);
			else
				fSuccess = m_pFile->WriteString(psArgs->GetPtr());

			if ( !fSuccess )
			{
				g_Log.Event(LOGL_ERROR, "FILE: Failed writing to \"%s\"\n", static_cast<LPCTSTR>(m_pFile->GetFilePath()));
				return false;
			}
			break;
		}
		default:
			return false;
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CFileObj::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CFileObj::r_WriteVal");
	EXC_TRY("WriteVal");
	ASSERT(pszKey);

	if ( !strnicmp("MODE.", pszKey, 5) )
	{
		pszKey += 5;
		if ( !strnicmp("APPEND", pszKey, 6) )
			sVal.FormatVal(m_fAppend);
		else if ( !strnicmp("CREATE", pszKey, 6) )
			sVal.FormatVal(m_fCreate);
		else if ( !strnicmp("READFLAG", pszKey, 8) )
			sVal.FormatVal(m_fRead);
		else if ( !strnicmp("WRITEFLAG", pszKey, 9) )
			sVal.FormatVal(m_fWrite);
		else
			return false;

		return true;
	}

	FO_TYPE index = static_cast<FO_TYPE>(FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1));
	switch ( index )
	{
		case FO_FILEEXIST:
		{
			pszKey += strlen(sm_szLoadKeys[index]);
			GETNONWHITESPACE(pszKey);

			TCHAR *ppCmd = Str_TrimWhitespace(const_cast<TCHAR *>(pszKey));
			if ( !(ppCmd && strlen(ppCmd)) )
				return false;

			CFile *pFile = new CFile();
			sVal.FormatVal(pFile->Open(ppCmd));

			delete pFile;
			break;
		}
		case FO_FILELINES:
		{
			pszKey += strlen(sm_szLoadKeys[index]);
			GETNONWHITESPACE(pszKey);

			TCHAR *ppCmd = Str_TrimWhitespace(const_cast<TCHAR *>(pszKey));
			if ( !(ppCmd && strlen(ppCmd)) )
				return false;

			CFileText *pFile = new CFileText();
			if ( !pFile->Open(ppCmd, OF_READ|OF_TEXT) )
			{
				delete pFile;
				return false;
			}

			long lLines = 0;
			while ( !pFile->IsEOF() )
			{
				pFile->ReadString(GetReadBuffer(), SCRIPT_MAX_LINE_LEN);
				++lLines;
			}
			pFile->Close();
			sVal.FormatVal(lLines);

			delete pFile;
			break;
		}
		case FO_FILEPATH:
			sVal.Format("%s", m_pFile->IsFileOpen() ? static_cast<LPCTSTR>(m_pFile->GetFilePath()) : "");
			break;
		case FO_INUSE:
			sVal.FormatVal(m_pFile->IsFileOpen());
			break;
		case FO_ISEOF:
			sVal.FormatVal(m_pFile->IsEOF());
			break;
		case FO_LENGTH:
			sVal.FormatVal(m_pFile->IsFileOpen() ? m_pFile->GetLength() : -1);
			break;
		case FO_OPEN:
		{
			pszKey += strlen(sm_szLoadKeys[index]);
			GETNONWHITESPACE(pszKey);

			TCHAR *pszFilename = Str_TrimWhitespace(const_cast<TCHAR *>(pszKey));
			if ( !(pszFilename && strlen(pszFilename)) )
				return false;

			if ( m_pFile->IsFileOpen() )
			{
				g_Log.Event(LOGL_ERROR, "FILE: Cannot open file (%s). First close \"%s\"\n", pszFilename, static_cast<LPCTSTR>(m_pFile->GetFilePath()));
				return false;
			}

			sVal.FormatVal(FileOpen(pszFilename));
			break;
		}
		case FO_POSITION:
			sVal.FormatVal(m_pFile->GetPosition());
			break;
		case FO_READBYTE:
		case FO_READCHAR:
		{
			size_t iRead = 1;
			if ( index != FO_READCHAR )
			{
				pszKey += strlen(sm_szLoadKeys[index]);
				GETNONWHITESPACE(pszKey);

				iRead = Exp_GetVal(pszKey);
				if ( (iRead <= 0) || (iRead >= SCRIPT_MAX_LINE_LEN) )
					return false;
			}

			if ( (m_pFile->GetPosition() + iRead > m_pFile->GetLength()) || m_pFile->IsEOF() )
			{
				g_Log.Event(LOGL_ERROR, "FILE: Failed reading %" FMTSIZE_T " byte from \"%s\". Too near to EOF\n", iRead, static_cast<LPCTSTR>(m_pFile->GetFilePath()));
				return false;
			}

			TCHAR *pszBuffer = GetReadBuffer(true);
			if ( iRead != m_pFile->Read(pszBuffer, iRead) )
			{
				g_Log.Event(LOGL_ERROR, "FILE: Failed reading %" FMTSIZE_T " byte from \"%s\"\n", iRead, static_cast<LPCTSTR>(m_pFile->GetFilePath()));
				return false;
			}

			if ( index == FO_READCHAR )
				sVal.FormatVal(*pszBuffer);
			else
				sVal.Format("%s", pszBuffer);
			break;
		}
		case FO_READLINE:
		{
			pszKey += strlen(sm_szLoadKeys[index]);
			GETNONWHITESPACE(pszKey);

			TCHAR *pszBuffer = GetReadBuffer();
			ASSERT(pszBuffer);

			INT64 iLines = Exp_GetLLVal(pszKey);
			if ( iLines < 0 )
				return false;

			DWORD dwSeek = m_pFile->GetPosition();
			m_pFile->SeekToBegin();

			if ( iLines == 0 )
			{
				while ( !m_pFile->IsEOF() )
					m_pFile->ReadString(pszBuffer, SCRIPT_MAX_LINE_LEN);
			}
			else
			{
				for ( INT64 i = 1; i <= iLines; ++i )
				{
					if ( m_pFile->IsEOF() )
						break;

					pszBuffer = GetReadBuffer();
					m_pFile->ReadString(pszBuffer, SCRIPT_MAX_LINE_LEN);
				}
			}

			m_pFile->Seek(dwSeek);

			size_t iLineLen = strlen(pszBuffer);
			while ( iLineLen > 0 )
			{
				--iLineLen;
				if ( isgraph(pszBuffer[iLineLen]) || (pszBuffer[iLineLen] == 0x20) || (pszBuffer[iLineLen] == '\t') )
				{
					++iLineLen;
					pszBuffer[iLineLen] = '\0';
					break;
				}
			}

			sVal.Format("%s", pszBuffer);
			break;
		}
		case FO_SEEK:
		{
			pszKey += strlen(sm_szLoadKeys[index]);
			GETNONWHITESPACE(pszKey);

			if ( pszKey[0] == '\0' )
				return false;

			if ( strcmpi("BEGIN", pszKey) == 0 )
				sVal.FormatVal(m_pFile->Seek(0, SEEK_SET));
			else if ( strcmpi("END", pszKey) == 0 )
				sVal.FormatVal(m_pFile->Seek(0, SEEK_END));
			else
				sVal.FormatVal(m_pFile->Seek(Exp_GetVal(pszKey), SEEK_SET));
			break;
		}
		default:
			return false;
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CFileObj::r_Verb(CScript &s, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CFileObj::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);

	LPCTSTR pszKey = s.GetKey();
	int index = FindTableSorted(pszKey, sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1);
	if ( index < 0 )
		return r_LoadVal(s);

	switch ( static_cast<FOV_TYPE>(index) )
	{
		case FOV_CLOSE:
		{
			if ( m_pFile->IsFileOpen() )
				m_pFile->Close();
			break;
		}
		case FOV_DELETEFILE:
		{
			if ( !s.GetArgStr() )
				return false;
			if ( m_pFile->IsFileOpen() && !strcmp(s.GetArgStr(), m_pFile->GetFileTitle()) )
				return false;
			STDFUNC_UNLINK(s.GetArgRaw());
			break;
		}
		case FOV_FLUSH:
		{
			if ( m_pFile->IsFileOpen() )
				m_pFile->Flush();
			break;
		}
		default:
			return false;
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

///////////////////////////////////////////////////////////
// CFileObjContainer

CFileObjContainer::CFileObjContainer()
{
	m_iGlobalTimeout = m_iCurrentTick = 0;
	SetFileNumber(0);
}

CFileObjContainer::~CFileObjContainer()
{
	ResizeContainer(0);
	m_FileList.clear();
}

void CFileObjContainer::ResizeContainer(size_t iNewRange)
{
	ADDTOCALLSTACK("CFileObjContainer::ResizeContainer");
	if ( iNewRange == m_FileList.size() )
		return;

	if ( iNewRange < m_FileList.size() )
	{
		if ( m_FileList.empty() )
			return;

		CFileObj *pObj = NULL;
		size_t iDiff = m_FileList.size() - iNewRange;
		for ( size_t i = m_FileList.size() - 1; iDiff > 0; --iDiff, --i )
		{
			pObj = m_FileList.at(i);
			m_FileList.pop_back();

			if ( pObj )
				delete pObj;
		}
	}
	else
	{
		size_t iDiff = iNewRange - m_FileList.size();
		for ( size_t i = 0; i < iDiff; ++i )
			m_FileList.push_back(new CFileObj());
	}
}

int CFileObjContainer::GetFileNumber()
{
	ADDTOCALLSTACK("CFileObjContainer::GetFilenumber");
	return m_iFileNumber;
}

void CFileObjContainer::SetFileNumber(int iNewRange)
{
	ADDTOCALLSTACK("CFileObjContainer::SetFilenumber");
	ResizeContainer(iNewRange);
	m_iFileNumber = iNewRange;
}

bool CFileObjContainer::OnTick()
{
	ADDTOCALLSTACK("CFileObjContainer::OnTick");
	EXC_TRY("Tick");

	if ( !m_iGlobalTimeout )
		return true;

	if ( ++m_iCurrentTick >= m_iGlobalTimeout )
	{
		m_iCurrentTick = 0;
		for ( std::vector<CFileObj *>::iterator i = m_FileList.begin(); i != m_FileList.end(); ++i )
		{
			if ( !(*i)->OnTick() )
			{
				// Error and fixWeirdness
			}
		}
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_DEBUG_END;
	return false;
}

int CFileObjContainer::FixWeirdness()
{
	ADDTOCALLSTACK("CFileObjContainer::FixWeirdness");
	return 0;
}

enum CFO_TYPE
{
	#define ADD(a,b) CFO_##a,
	#include "../tables/CFileObjContainer_props.tbl"
	#undef ADD
	CFO_QTY
};

LPCTSTR const CFileObjContainer::sm_szLoadKeys[CFO_QTY + 1] =
{
	#define ADD(a,b) b,
	#include "../tables/CFileObjContainer_props.tbl"
	#undef ADD
	NULL
};

enum CFOV_TYPE
{
	#define ADD(a,b) CFOV_##a,
	#include "../tables/CFileObjContainer_functions.tbl"
	#undef ADD
	CFOV_QTY
};

LPCTSTR const CFileObjContainer::sm_szVerbKeys[CFOV_QTY + 1] =
{
	#define ADD(a,b) b,
	#include "../tables/CFileObjContainer_functions.tbl"
	#undef ADD
	NULL
};

bool CFileObjContainer::r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef)
{
	ADDTOCALLSTACK("CFileObjContainer::r_GetRef");
	if ( !strnicmp("FIRSTUSED.", pszKey, 10) )
	{
		pszKey += 10;
		for ( std::vector<CFileObj *>::iterator i = m_FileList.begin(); i != m_FileList.end(); ++i )
		{
			if ( (*i)->IsInUse() )
			{
				pRef = (*i);
				return true;
			}
		}
		return false;
	}

	size_t iNumber = static_cast<size_t>(Exp_GetLLVal(pszKey));
	SKIP_SEPARATORS(pszKey);

	if ( iNumber >= m_FileList.size() )
		return false;

	CFileObj *pFile = m_FileList.at(iNumber);
	if ( pFile )
	{
		pRef = pFile;
		return true;
	}
	return false;
}

bool CFileObjContainer::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CFileObjContainer::r_LoadVal");
	EXC_TRY("LoadVal");
	LPCTSTR pszKey = s.GetKey();

	CFO_TYPE index = static_cast<CFO_TYPE>(FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1));
	switch ( index )
	{
		case CFO_OBJECTPOOL:
			SetFileNumber(s.GetArgVal());
			break;
		case CFO_GLOBALTIMEOUT:
			m_iGlobalTimeout = labs(s.GetArgVal() * TICK_PER_SEC);
			break;
		default:
			return false;
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CFileObjContainer::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CFileObjContainer::r_WriteVal");
	EXC_TRY("WriteVal");

	if ( !strnicmp("FIRSTUSED.", pszKey, 10) )
	{
		pszKey += 10;
		for ( std::vector<CFileObj *>::iterator i = m_FileList.begin(); i != m_FileList.end(); ++i )
		{
			if ( (*i)->IsInUse() )
				return static_cast<CScriptObj *>(*i)->r_WriteVal(pszKey, sVal, pSrc);
		}
		return false;
	}

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	if ( index < 0 )
	{
		size_t iNumber = static_cast<size_t>(Exp_GetLLVal(pszKey));
		SKIP_SEPARATORS(pszKey);

		if ( iNumber >= m_FileList.size() )
			return false;

		CFileObj *pFile = m_FileList.at(iNumber);
		if ( pFile )
		{
			CScriptObj *pObj = dynamic_cast<CScriptObj *>(pFile);
			if ( pObj )
				return pObj->r_WriteVal(pszKey, sVal, pSrc);
		}
		return false;
	}

	switch ( static_cast<CFO_TYPE>(index) )
	{
		case CFO_OBJECTPOOL:
			sVal.FormatVal(GetFileNumber());
			break;
		case CFO_GLOBALTIMEOUT:
			sVal.FormatVal(m_iGlobalTimeout / TICK_PER_SEC);
			break;
		default:
			return false;
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CFileObjContainer::r_Verb(CScript &s, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CFileObjContainer::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);
	LPCTSTR pszKey = s.GetKey();

	if ( !strnicmp("FIRSTUSED.", pszKey, 10) )
	{
		pszKey += 10;
		for ( std::vector<CFileObj *>::iterator i = m_FileList.begin(); i != m_FileList.end(); ++i )
		{
			if ( (*i)->IsInUse() )
				return static_cast<CScriptObj *>(*i)->r_Verb(s, pSrc);
		}
		return false;
	}

	int index = FindTableSorted(pszKey, sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1);
	if ( index < 0 )
	{
		if ( strchr(pszKey, '.') )	// 0.blah format
		{
			size_t iNumber = static_cast<size_t>(Exp_GetLLVal(pszKey));
			if ( iNumber < m_FileList.size() )
			{
				SKIP_SEPARATORS(pszKey);
				CFileObj *pFile = m_FileList.at(iNumber);
				if ( pFile )
				{
					CScriptObj *pObj = dynamic_cast<CScriptObj *>(pFile);
					if ( pObj )
					{
						CScript psContinue(pszKey, s.GetArgStr());
						return pObj->r_Verb(psContinue, pSrc);
					}
				}
				return false;
			}
		}
		return r_LoadVal(s);
	}

	switch ( static_cast<CFOV_TYPE>(index) )
	{
		case CFOV_CLOSEOBJECT:
		case CFOV_RESETOBJECT:
		{
			if ( s.HasArgs() )
			{
				size_t iNumber = static_cast<size_t>(s.GetArgVal());
				if ( iNumber >= m_FileList.size() )
					return false;

				CFileObj *pObj = m_FileList.at(iNumber);
				if ( index == CFOV_RESETOBJECT )
				{
					delete pObj;
					m_FileList.at(iNumber) = new CFileObj();
				}
				else
					pObj->FlushAndClose();
			}
			break;
		}
		default:
			return false;
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}
