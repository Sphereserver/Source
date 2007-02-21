//
// CScriptObj.cpp
// A scriptable object.
//

#ifdef _WIN32
	#include <process.h>
#else
	#include <errno.h>	// errno
	extern int errno;
#endif

#include "../graysvr.h"
#include "../quest.h"

//	stories extra information for SHOW/XSHOW commands to add convenient information
struct ShowCmdExtras
{
	typedef const char * CCP;
	static CCP m_flags[32];
	static CCP m_attr[16];

	static CCP *match(char *text)		// text ends on command that suits?
	{
		int len = strlen(text);

		if (( len >= 5 ) && !strnicmp(text + len - 5, "flags", 5) )
			return &m_flags[0];
		else if (( len >= 4 ) && !strnicmp(text + len - 4, "attr", 4) )
			return &m_attr[0];

		return NULL;
	}

	static int maxIndex(CCP *forArray)	// how many bits are storied in here?
	{
		if ( forArray == &m_flags[0] )
			return 32;
		if ( forArray == &m_attr[0] )
			return 16;
		else
			return 0;
	}
};

const char *ShowCmdExtras::m_flags[32] =
{
	"invul",
	"dead",
	"freeze",
	"invis",
	"?",
	"war",
	"reactive",
	"poison",
	"nightsight",
	"reflect",
	"poly",
	"incognito",
	"spiritspeak",
	"insub",
	"emote",
	"crystal",
	"shield",
	"archerymove",
	"stone",
	"?",
	"flyrun",
	"?",
	"hallu",
	"hidden",
	"indoors",
	"crim",
	"conj",
	"pet",
	"spawn",
	"savebit",
	"ridden",
	"mounted",
};

const char *ShowCmdExtras::m_attr[16] =
{
	"identified",
	"decay",
	"newbie",
	"movealways",
	"movenever",
	"magic",
	"owned",
	"invis",
	"?",
	"?",
	"?",
	"?",
	"forsale",
	"stolen",
	"candecay",
	"static",
};

////////////////////////////////////////////////////////////////////////////////////////
// -CScriptTriggerArgs

void CScriptTriggerArgs::Init( LPCTSTR pszStr )
{
	m_pO1			= NULL;

	if ( !pszStr )
		pszStr	= "";
	// raw is left untouched for now - it'll be split the 1st time argv is accessed
	m_s1_raw		= pszStr;
	if ( *pszStr == '"' )
		pszStr++;
	m_s1	= pszStr ;

	// take quote if present.
	char	* str;
	if ( (str = strchr(m_s1.GetPtr(), '"')) )
		*str	= '\0';
	
	m_iN1	= 0;
	m_iN2	= 0;
	m_iN3	= 0;

	// attempt to parse this.
	if ( isdigit(*pszStr) || ((*pszStr == '-') && isdigit(*(pszStr+1))) )
	{
		m_iN1 = Exp_GetSingle(pszStr);
		SKIP_ARGSEP( pszStr );
		if ( isdigit(*pszStr) || ((*pszStr == '-') && isdigit(*(pszStr+1))) )
		{
			m_iN2 = Exp_GetSingle(pszStr);
			SKIP_ARGSEP( pszStr );
			if ( isdigit(*pszStr) || ((*pszStr == '-') && isdigit(*(pszStr+1))) )
			{
				m_iN3 = Exp_GetSingle(pszStr);
			}
		}
	}
}



CScriptTriggerArgs::CScriptTriggerArgs( LPCTSTR pszStr )
{
	Init( pszStr );
}



bool CScriptTriggerArgs::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	if ( !strnicmp(pszKey, "ARGO.", 5) )	// ARGO.NAME
	{
		pszKey += 5;
		if ( *pszKey == '1' )
			pszKey++;
		pRef = m_pO1;
		return true;
	}
	return false;
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
	AGC_QTY,
};

LPCTSTR const CScriptTriggerArgs::sm_szLoadKeys[AGC_QTY+1] =
{
	"ARGN",
	"ARGN1",
	"ARGN2",
	"ARGN3",
	"ARGO",
	"ARGS",
	"ARGV",
	"LOCAL",
	NULL,
};


bool CScriptTriggerArgs::r_Verb( CScript & s, CTextConsole * pSrc )
{
	EXC_TRY("Verb");
	int	index;
	LPCTSTR		pszKey = s.GetKey();
	LPCTSTR		pszArgs = s.GetArgStr();

	if ( !strnicmp( "LOCAL.", pszKey, 6 ) )
	{
		bool fQuoted = false;
		m_VarsLocal.SetStr( s.GetKey()+6, fQuoted, s.GetArgStr( &fQuoted ), false );
		return true;
	}

	index = FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 );
	switch (index)
	{
		case AGC_N:
		case AGC_N1:
			m_iN1	= s.GetArgVal();
			return true;
		case AGC_N2:
			m_iN2	= s.GetArgVal();
			return true;
		case AGC_N3:
			m_iN3	= s.GetArgVal();
			return true;
		case AGC_S:
			Init( s.GetArgStr() );
			return true;
		default:
			return false;
	}
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}



bool CScriptTriggerArgs::r_LoadVal( CScript & s )
{
	return false;
}

bool CScriptTriggerArgs::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	EXC_TRY("WriteVal");
	if ( IsSetEF( EF_Intrinsic_Locals ) )
	{
		EXC_SET("intrinsic");
		VariableList::Variable *pVar = m_VarsLocal.GetKey(pszKey);
		if ( pVar )
		{
			sVal	= pVar->GetValStr();
			return true;
		}
	}
	else if ( !strnicmp("LOCAL.", pszKey, 6) )
	{
		EXC_SET("local");
		pszKey	+= 6;
		sVal	= m_VarsLocal.GetKeyStr(pszKey, true);
		return true;
	}

	if ( !strnicmp(pszKey, "ARGV", 4) )
	{
		EXC_SET("argv");
		pszKey+=4;
		SKIP_SEPARATORS(pszKey);
		
		int iQty = m_v.GetCount();
		if ( iQty == 0 )
		{
			// PARSE IT HERE
			TCHAR *		pszArg		= (char*)m_s1_raw.GetPtr();
			TCHAR *		s			= pszArg;
			bool		fQuotes		= false;
			while ( *s )
			{
				// ignore leading spaces
				if ( isspace(*s ) )
				{
					s++;
					continue;
				}

				// add empty arguments if they are provided
				if ( (*s == ',') && (!fQuotes))
				{
					m_v.Add( '\0' );
					s++;
					continue;
				}
				
				// check to see if the argument is quoted (incase it contains commas)
				if ( *s == '"' )
				{
					s++;
					fQuotes = true;
				}

				pszArg	= s;	// arg starts here
				s++;

				while (*s)
				{
					if ( *s == '"' )
					{
						if ( fQuotes )	{	*s	= '\0';	fQuotes = false;	break;	}
						*s = '\0';
						s++;
						fQuotes	= true;	// maintain
						break;
					}
					if ( !fQuotes && (*s == ',') )
					{ *s = '\0'; s++; break; }
					s++;
				}
				m_v.Add( pszArg );
			}
			iQty = m_v.GetCount();
		}	
		
		if ( *pszKey == '\0' )
		{
			sVal.FormatVal(iQty);
			return true;
		}

		int iNum = Exp_GetSingle(pszKey);
		SKIP_SEPARATORS(pszKey);
		if ( !m_v.IsValidIndex(iNum) )
		{
			sVal = "";
			return true;
		}
		sVal.Format(m_v.GetAt(iNum));
		return true;
	}

	EXC_SET("generic");
	int index = FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );
	switch (index)
	{
		case AGC_N:
		case AGC_N1:
			sVal.FormatVal(m_iN1);
			break;
		case AGC_N2:
			sVal.FormatVal(m_iN2);
			break;
		case AGC_N3:
			sVal.FormatVal(m_iN3);
			break;
		case AGC_O:
			{
				CObjBase *pObj = dynamic_cast <CObjBase*> (m_pO1);
				if ( pObj )
					sVal.FormatHex(pObj->GetUID());
				else
					sVal.FormatVal(0);
			}
			break;
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

////////////////////////////////////////////////////////////////////////////////////////
// -CScriptObj

bool CScriptObj::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	// A key name that just links to another object.
	if ( !strnicmp(pszKey, "SERV.", 5) )
	{
		pszKey += 5;
		pRef = &g_Serv;
		return true;
	}
	else if ( !strnicmp(pszKey, "UID.", 4) )
	{
		pszKey += 4;
		UID uid = (DWORD)Exp_GetVal(pszKey);
		SKIP_SEPARATORS(pszKey);
		pRef = uid.ObjFind();
		return true;
	}
	else if ( ! strnicmp( pszKey, "OBJ.", 4 ))
	{
		pszKey += 4;
		pRef = g_World.m_uidObj.ObjFind();
		return true;
	}
	else if ( !strnicmp(pszKey, "NEW.", 4) )
	{
		pszKey += 4;
		pRef = g_World.m_uidNew.ObjFind();
		return true;
	}
	else if ( !strnicmp(pszKey, "I.", 2) )
	{
		pszKey += 2;
		pRef = this;
		return true;
	}
	else if ( IsSetOF( OF_FileCommands ) && !strnicmp(pszKey, "FILE.", 5) )
	{
		pszKey += 5;
		pRef = &(g_Serv.fhFile);
		return true;
	}
	else if ( !strnicmp(pszKey, "DB.", 3) )
	{
		pszKey += 3;
		pRef = &(g_Serv.m_hdb);
		return true;
	}
	else if ( !strnicmp(pszKey, "QUEST.", 6) )
	{
		pszKey += 6;
		int id = Exp_GetVal(pszKey);
		SKIP_SEPARATORS(pszKey);
		pRef = (( id >= 0 ) && ( id < g_Quests.size() )) ? g_Quests[id] : NULL;
		return true;
	}
	return false;
}

enum SSC_TYPE
{
	#define ADD(a,b) SSC_##a,
	#include "../tables/CScriptObj_functions.tbl"
	#undef ADD
	SSC_QTY,
};

LPCTSTR const CScriptObj::sm_szLoadKeys[SSC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CScriptObj_functions.tbl"
	#undef ADD
	NULL,
};

bool	CScriptObj::r_Call( LPCTSTR pszFunction, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * psVal, TRIGRET_TYPE * piRet )
{
	GETNONWHITESPACE( pszFunction );
	int	index;
	{
		int	iCompareRes	= -1;
		index = g_Cfg.m_Functions.FindKeyNear( pszFunction, iCompareRes, true );
		if ( iCompareRes )
			index	= -1;
	}

	if ( index < 0 )
		return false;

	CResourceNamed * pFunction = static_cast <CResourceNamed *>( g_Cfg.m_Functions[index] );
	CResourceLock sFunction;
	if ( pFunction->ResourceLock(sFunction) )
	{
		TScriptProfiler::TScriptProfilerFunction	*pFun;
		TIME_PROFILE_INIT;

		//	If functions profiler is on, search this function record and get pointer to it
		//	if not, create the corresponding record
		if ( IsSetEF(EF_Script_Profiler) )
		{
			char	*pSpace;
			TEMPSTRING(pName);
			//	lowercase for speed, and strip arguments
			strcpy(pName, pszFunction);
			if ( pSpace = strchr(pName, ' ') ) *pSpace = 0;
			_strlwr(pName);

			g_profiler.init();
			for ( pFun = g_profiler.FunctionsHead; pFun != NULL; pFun = pFun->next )
			{
				if ( !strcmp(pFun->name, pName) ) break;
			}

			// first time function called. so create a record for it
			if ( !pFun )
			{
				pFun = new TScriptProfiler::TScriptProfilerFunction;
				memset(pFun, 0, sizeof(TScriptProfiler::TScriptProfilerFunction));
				strcpy(pFun->name, pName);
				if ( g_profiler.FunctionsTail ) g_profiler.FunctionsTail->next = pFun;
				else g_profiler.FunctionsHead = pFun;
				g_profiler.FunctionsTail = pFun;
			}

			//	prepare the informational block
			pFun->called++;
			g_profiler.called++;
			TIME_PROFILE_START;
		}

		Lock(true);
		TRIGRET_TYPE iRet = OnTriggerRun(sFunction, TRIGRUN_SECTION_TRUE, pSrc, pArgs, psVal);
		Release();

		if ( IsSetEF(EF_Script_Profiler) )
		{
			//	update the time call information
			TIME_PROFILE_END;
			llTicks = llTicksEnd - llTicks;
			pFun->total += llTicks;
			pFun->average = (pFun->total/pFun->called);
			if ( pFun->max < llTicks ) pFun->max = llTicks;
			if (( pFun->min > llTicks ) || ( !pFun->min )) pFun->min = llTicks;
			g_profiler.total += llTicks;
		}

		if ( piRet )
			*piRet	= iRet;
	}
	return true;
}


bool CScriptObj::r_LoadVal( CScript & s )
{
	EXC_TRY("LoadVal");
	LPCTSTR pszKey = s.GetKey();

	if ( !strnicmp(pszKey, "CLEARVARS", 9) )
	{
		pszKey = s.GetArgStr();
		SKIP_SEPARATORS(pszKey);
		g_Exp.m_VarGlobals.ClearKeys(pszKey);
		return true;
	}

	// ignore these.
	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1);
	if ( index < 0 )
	{
		g_Log.Error("Undefined keyword '%s'\n", s.GetKey());
		return false;
	}
	
	if ( index == SSC_VAR )
	{
		bool fQuoted = false;
		int	i	= g_Exp.m_VarGlobals.SetStr( pszKey+4, fQuoted, s.GetArgStr( &fQuoted ), false );
		return true;
	}
	if ( index == SSC_VAR0 )
	{
		bool fQuoted = false;
		int	i	= g_Exp.m_VarGlobals.SetStr( pszKey+5, fQuoted, s.GetArgStr( &fQuoted ), true );
		return true;
	}

	if ( index == SSC_DEFMSG )
	{
		long	l;
		pszKey += 7;
		for ( l = 0; l < DEFMSG_QTY; l++ )
		{
			if ( !strcmpi(pszKey, (const char *)g_Exp.sm_szMsgNames[l]) )
			{
				bool	fQuoted = false;
				TCHAR	*args = s.GetArgStr(&fQuoted);
				strcpy(g_Exp.sm_szMessages[l], args);
				return(true);
			}
		}
		g_Log.Event(LOGM_INIT|LOGL_ERROR, "Setting not used message override named '%s'\n", pszKey);
		return(false);
	}
	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

#define REMOVE_QUOTES( x )			\
{									\
	GETNONWHITESPACE( x );			\
	if ( *x == '"' )	x++;				\
	TCHAR * psX	= strchr( x, '"' );	\
	if ( psX )						\
		*psX	= '\0';				\
}


static void StringFunction( int iFunc, LPCTSTR pszKey, CGString &sVal )
{
	GETNONWHITESPACE(pszKey);
	if ( *pszKey == '(' )
		pszKey++;

	TCHAR * ppCmd[4];
	int iCount = Str_ParseCmds( const_cast<TCHAR *>(pszKey), ppCmd, COUNTOF(ppCmd), ",)" );
	if ( !iCount )
	{
		g_Log.Error("Bad string function usage. missing )\n");
		return;
	}

	TCHAR * psArg1	= ppCmd[0];

	switch ( iFunc )
	{
		case SSC_CHR:
			sVal.Format( "%c", Exp_GetSingle( ppCmd[0] ) );
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

bool CScriptObj::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	EXC_TRY("WriteVal");
	CScriptObj * pRef = NULL;
	if ( r_GetRef( pszKey, pRef ))
	{
		if ( pRef == NULL )	// good command but bad link.
		{
			sVal = "0";
			return true;
		}
		if ( pszKey[0] == '\0' )	// we where just testing the ref.
		{
			CObjBase *	pObj	= dynamic_cast <CObjBase *> (pRef);
			if ( pObj )
				sVal.FormatHex(pObj->uid());
			else
				sVal.FormatVal(1);
			return true;
		}
		return pRef->r_WriteVal( pszKey, sVal, pSrc );
	}

	int i = FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );
	if ( i < 0 )
	{
		// <dSOMEVAL> same as <eval <SOMEVAL>> to get dec from the val
		if (( *pszKey == 'd' ) || ( *pszKey == 'D' ))
		{
			LPCTSTR arg = pszKey + 1;
			if ( r_WriteVal(arg, sVal, pSrc) )
			{
				sVal.FormatVal(ATOI(sVal));
				return true;
			}
		}
		// <r>, <r15>, <r3,15> are shortcuts to rand(), rand(15) and rand(3,15)
		else if (( *pszKey == 'r' ) || ( *pszKey == 'R' ))
		{
			pszKey += 1;
			if ( *pszKey && (( *pszKey < '0' ) || ( *pszKey > '9' )) && *pszKey != '-' )
				return false;

			int	min = 1000, max = INT_MIN;

			if ( *pszKey )
			{
				min = Exp_GetVal(pszKey);
				SKIP_ARGSEP(pszKey);
			}
			if ( *pszKey )
			{
				max = Exp_GetVal(pszKey);
				SKIP_ARGSEP(pszKey);
			}

			if ( max == INT_MIN )
			{
				max = min - 1;
				min = 0;
			}

			if ( min >= max )
				sVal.FormatVal(min);
			else
				sVal.FormatVal(Calc_GetRandVal2(min, max));

			return true;
		}
		return false;	// Bad command.
	}

	pszKey += strlen( sm_szLoadKeys[i] );
	SKIP_SEPARATORS(pszKey);
	bool	fZero	= false;

	switch ( i )
	{
	case SSC_BETWEEN:
	case SSC_BETWEEN2:
		{
			int	iMin = Exp_GetVal(pszKey);
			SKIP_ARGSEP(pszKey);
			int	iMax = Exp_GetVal(pszKey);
			SKIP_ARGSEP(pszKey);
			int iCurrent = Exp_GetVal(pszKey);
			SKIP_ARGSEP(pszKey);
			int iAbsMax = Exp_GetVal(pszKey);
			SKIP_ARGSEP(pszKey);
			if ( i == SSC_BETWEEN2 )
			{
				iCurrent = iAbsMax - iCurrent;
			}

			if (( iMin >= iMax ) || ( iAbsMax <= 0 ) || ( iCurrent <= 0 ) )
				sVal.FormatVal(iMin);
			else if ( iCurrent >= iAbsMax )
				sVal.FormatVal(iMax);
			else
				sVal.FormatVal((iCurrent * (iMax - iMin))/iAbsMax + iMin);
		} break;

	case SSC_LISTCOL:
		// Set the alternating color.
		sVal = (CWebPageDef::sm_iListIndex&1) ? "bgcolor=\"#E8E8E8\"" : "";
		return true;
	case SSC_OBJ:
		if ( !g_World.m_uidObj.ObjFind() ) g_World.m_uidObj = 0;
		sVal.FormatHex((DWORD)g_World.m_uidObj);
		return true;
	case SSC_NEW:
		if ( !g_World.m_uidNew.ObjFind() ) g_World.m_uidNew = 0;
		sVal.FormatHex((DWORD)g_World.m_uidNew);
		return true;
	case SSC_SRC:
		if ( pSrc == NULL )
			pRef	= NULL;
		else
		{
			pRef = pSrc->GetChar();	// if it can be converted .
			if ( ! pRef )
				pRef = dynamic_cast <CScriptObj*> (pSrc);	// if it can be converted .
		}
		if ( ! pRef )
		{
			sVal.FormatVal( 0 );
			return true;
		}
		if ( !*pszKey )
		{
			CObjBase * pObj = dynamic_cast <CObjBase*> (pRef);	// if it can be converted .
			sVal.FormatHex( pObj ? pObj->uid() : 0 );
			return true;
		}
		return pRef->r_WriteVal( pszKey, sVal, pSrc );
	case SSC_VAR0:
		fZero	= true;
	case SSC_VAR:
		// "VAR." = get/set a system wide variable.
		{
			VariableList::Variable *pVar = g_Exp.m_VarGlobals.GetKey(pszKey);
			if ( pVar )
				sVal	= pVar->GetValStr();
			else if ( fZero )
				sVal	= "0";
		}
		return true;
	case SSC_DEF0:
		fZero	= true;
	case SSC_DEF:
		{
			VariableList::Variable *pVar = g_Exp.m_VarDefs.GetKey(pszKey);
			if ( pVar )
				sVal	= pVar->GetValStr();
			else if ( fZero )
				sVal	= "0";
		}
		return true;
	case SSC_EVAL:
		sVal.FormatVal( Exp_GetVal( pszKey ));
		return true;
	case SSC_FVAL:
		{
			int	iVal		= Exp_GetVal( pszKey );
			sVal.Format("%s%d.%d", (iVal >= 0) ? "" : "-", abs(iVal/10), abs(iVal%10));
			return true;
		}
	case SSC_HVAL:
		sVal.FormatHex( Exp_GetVal( pszKey ));
		return true;
	case SSC_QVAL:
		{	// Do a switch ? type statement <QVAL conditional ? option1 : option2>
			TCHAR * ppCmds[3];
			ppCmds[0] = const_cast<TCHAR*>(pszKey);
			Str_Parse( ppCmds[0], &(ppCmds[1]), "?" );
			Str_Parse( ppCmds[1], &(ppCmds[2]), ":" );
			sVal = ppCmds[ Exp_GetVal( ppCmds[0] ) ? 1 : 2 ];
			if ( sVal.IsEmpty())
				sVal = " ";
		}
		return true;
	case SSC_ISBIT:
	case SSC_SETBIT:
	case SSC_CLRBIT:
		{
			int val = Exp_GetVal(pszKey);
			SKIP_ARGSEP(pszKey);
			int bit = Exp_GetVal(pszKey);

			if ( i == SSC_ISBIT )
				sVal.FormatVal(val & ( 1 << bit ));
			else if ( i == SSC_SETBIT )
				sVal.FormatVal(val | ( 1 << bit ));
			else
				sVal.FormatVal(val & (~ ( 1 << bit )));
			break;
		}
	case SSC_ISEMPTY:
		sVal.FormatVal( IsStrEmpty( pszKey ) );
		return true;
	case SSC_ISNUM:
		GETNONWHITESPACE( pszKey );
		sVal.FormatVal( IsStrNumeric( pszKey ) );
		return true;
	case SSC_StrPos:
		{
			GETNONWHITESPACE( pszKey );
			int	iPos	= Exp_GetVal( pszKey );
			TCHAR	ch;
			if ( isdigit( *pszKey) && isdigit( *(pszKey+1) ) )
				ch	= (TCHAR) Exp_GetVal( pszKey );
			else
			{
				ch	= *pszKey;
				pszKey++;
			}
			
			GETNONWHITESPACE( pszKey );
			int	iLen	= strlen( pszKey );
			if ( iPos < 0 )
				iPos	= iLen + iPos;
			if ( iPos < 0 )
				iPos	= 0;
			else if ( iPos > iLen )
				iPos	= iLen;

			const char *pszPos	= strchr( pszKey + iPos, ch );
			if ( !pszPos )
				sVal.FormatVal( -1 );
			else
				sVal.FormatVal( pszPos - pszKey );
		}
		return true;
	case SSC_StrSub:
		{
			int	iPos	= Exp_GetVal( pszKey );
			int	iCnt	= Exp_GetVal( pszKey );
			SKIP_ARGSEP( pszKey );
			GETNONWHITESPACE( pszKey );

			int	iLen	= strlen( pszKey );
			if ( iPos < 0 ) iPos += iLen;
			if ( iPos > iLen || iPos < 0 ) iPos = 0;

			if ( iPos + iCnt > iLen || iCnt == 0 )
				iCnt = iLen - iPos;

			TEMPSTRING(buf);
			strncpy( buf, pszKey + iPos, iCnt );
			buf[iCnt] = '\0';
			sVal = buf;
		}
		return true;
	case SSC_StrArg:
		{
			TEMPSTRING(buf);
			GETNONWHITESPACE( pszKey );
			if ( *pszKey == '"' )
				pszKey++;
			int	i	= 0;
			while ( *pszKey && !isspace( *pszKey ) && *pszKey != ',' )
			{
				buf[i]	= *pszKey;
				pszKey++;
				i++;
			}
			buf[i]	= '\0';
			sVal	= buf;
		}
		return true;
	case SSC_StrEat:
		{
			GETNONWHITESPACE( pszKey );
			while ( *pszKey && !isspace( *pszKey ) && *pszKey != ',' )
				pszKey++;
			SKIP_ARGSEP( pszKey );
			sVal	= pszKey;
		}
		return true;
	case SSC_StrTrim:
		{
			if ( *pszKey )
				sVal = Str_TrimWhitespace(const_cast<TCHAR*>(pszKey));
			else
				sVal = "";

			return true;
		}
	case SSC_ASC:
		{
			TEMPSTRING(buf);
			TEMPSTRING(key);
			strcpy(key, pszKey);
			REMOVE_QUOTES(key);
			sVal.FormatHex(*key);
			sprintf(buf, sVal);
			while ( *(++key) )
			{
				if ( *key == '"' ) break;
				sVal.FormatHex(*key);
				strcat(buf, " ");
				strcat(buf, sVal);
			}
			sVal = buf;
		}
		return true;

	case SSC_SYSCMD:
	case SSC_SYSSPAWN:
		{
			if ( !IsSetOF(OF_FileCommands) )
				return false;

			GETNONWHITESPACE(pszKey);
			TCHAR	*Arg_ppCmd[10];		// limit to 9 arguments
			TEMPSTRING(buf);
			strcpy(buf, pszKey);
			int iQty = Str_ParseCmds(buf, Arg_ppCmd, COUNTOF(Arg_ppCmd));
			if ( iQty < 1 )
				return false;

			bool bWait = (i == SSC_SYSCMD);

#ifdef _WIN32
			_spawnl( bWait ? _P_WAIT : _P_NOWAIT,
				Arg_ppCmd[0], Arg_ppCmd[0], Arg_ppCmd[1],
				Arg_ppCmd[2], Arg_ppCmd[3], Arg_ppCmd[4],
				Arg_ppCmd[5], Arg_ppCmd[6], Arg_ppCmd[7],
				Arg_ppCmd[8], Arg_ppCmd[9], NULL );
#else
			if ( bWait )
			{
				iQty = 0;
				iQty = execlp( Arg_ppCmd[0], Arg_ppCmd[0], Arg_ppCmd[1], Arg_ppCmd[2],
							   Arg_ppCmd[3], Arg_ppCmd[4], Arg_ppCmd[5], Arg_ppCmd[6],
							   Arg_ppCmd[7], Arg_ppCmd[8], Arg_ppCmd[9], NULL );

				if ( iQty == -1 )
				{
					g_Log.Error("SYSCMD failed with error %d (\"%s\") when executing %s.\n", errno, strerror(errno), pszKey);
					return false;
				}
			}
			else
			{
				// I think fork will cause problems.. we'll see.. if yes new thread + execlp is required.
				int child_pid = fork();
				if ( child_pid < 0 )
				{
					g_Log.Error("SYSSPAWN failed when executing %s.\n", pszKey);
					return false;
				}
				else if ( child_pid == 0 )
				{
					int iResult = execlp( Arg_ppCmd[0], Arg_ppCmd[0], Arg_ppCmd[1], Arg_ppCmd[2],
										Arg_ppCmd[3], Arg_ppCmd[4], Arg_ppCmd[5], Arg_ppCmd[6],
										 Arg_ppCmd[7], Arg_ppCmd[8], Arg_ppCmd[9], NULL );
				
					if ( iQty == -1 )
						g_Log.Error("SYSSPAWN failed with error %d (\"%s\") when executing %s.\n", errno, strerror(errno), pszKey);

					exit(iQty);
				}
			}
#endif
			return true;
		}

	case SSC_EXPLODE:
		{
			char separators[16];

			GETNONWHITESPACE(pszKey);
			strcpylen(separators, pszKey, 16);
			{
				char *p = separators;
				while ( *p && *p != ',' )
					p++;
				*p = 0;
			}

			const char *p = pszKey + strlen(separators);
			sVal = "";
			if (( p > pszKey ) && *p )		//	we have list of accessible separators 
			{
				TCHAR *ppCmd[16];
				TEMPSTRING(z);
				strcpy(z, p);
				int count = Str_ParseCmds(z, ppCmd, COUNTOF(ppCmd), separators); 
				for ( int i = 0; i < count; i++ )
				{
					sVal.Add(ppCmd[i]);
					if ( i < count-1 )
						sVal.Add(',');
				}
			}
			return true;
		}

	default:
		StringFunction( i, pszKey, sVal );
		return true;
	}
	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

enum SSV_TYPE
{
	SSV_NEW,
	SSV_NEWDUPE,
	SSV_NEWITEM,
	SSV_NEWNPC,
	SSV_OBJ,
	SSV_SHOW,
	SSV_QTY,
};

LPCTSTR const CScriptObj::sm_szVerbKeys[SSV_QTY+1] =
{
	"NEW",
	"NEWDUPE",
	"NEWITEM",
	"NEWNPC",
	"OBJ",
	"SHOW",
	NULL,
};


bool CScriptObj::r_Verb(CScript &s, CTextConsole *pSrc)		// Execute command from script
{
	EXC_TRY("Verb");
	int	index;
	LPCTSTR pszKey = s.GetKey();
	CScriptObj * pRef;

	if ( r_GetRef( pszKey, pRef ))
	{
		if ( pszKey[0] )
		{
			if ( !pRef ) return true;
			CScript script( pszKey, s.GetArgStr());
			return pRef->r_Verb( script, pSrc );
		}
		// else just fall through. as they seem to be setting the pointer !?
	}

	if ( s.IsKeyHead("SRC.", 4 ))
	{
		pszKey += 4;
		pRef = dynamic_cast <CScriptObj*> (pSrc->GetChar());	// if it can be converted .
		if ( ! pRef )
		{
			pRef = dynamic_cast <CScriptObj*> (pSrc);
			if ( !pRef )
				return false;
		}
		CScript script(pszKey, s.GetArgStr());
		return pRef->r_Verb(script, pSrc);
	}

	index = FindTableSorted( s.GetKey(), sm_szVerbKeys, COUNTOF( sm_szVerbKeys )-1 );
	switch (index)
	{
		case SSV_OBJ:
			g_World.m_uidObj = s.GetArgVal();
			if ( !g_World.m_uidObj.ObjFind() )
				g_World.m_uidObj = 0;
			break;

		case SSV_NEW:
			g_World.m_uidNew = s.GetArgVal();
			if ( !g_World.m_uidNew.ObjFind() )
				g_World.m_uidNew = 0;
			break;

		case SSV_NEWDUPE:
			{
				UID uid(s.GetArgVal());
				CObjBase	*pObj = uid.ObjFind();
				g_World.m_uidNew = uid;
				CScript script("DUPE");
				bool bRc = pObj->r_Verb(script, pSrc);

				if ( (long)this != (long)&g_Serv )
				{
					CChar *pChar = dynamic_cast <CChar *>(this);
					if ( pChar )
						pChar->m_Act_Targ = g_World.m_uidNew;
					else
					{
						CClient *pClient = dynamic_cast <CClient *> (this);
						if ( pClient && pClient->GetChar() )
							pClient->GetChar()->m_Act_Targ = g_World.m_uidNew;
					}
				}
				return bRc;
			}

		case SSV_NEWITEM:	// just add an item but don't put it anyplace yet..
			{
				TCHAR	*ppCmd[3];
				int iQty = Str_ParseCmds(s.GetArgRaw(), ppCmd, COUNTOF(ppCmd), ",");
				if ( !iQty )
					return false;

				CItem *pItem = CItem::CreateHeader(ppCmd[0], NULL, false, pSrc->GetChar());
				if ( !pItem )
				{
					g_World.m_uidNew = (DWORD)0;
					return false;
				}
				if ( ppCmd[1] )
					pItem->SetAmount(Exp_GetVal(ppCmd[1]));
				if ( ppCmd[2] )
					pItem->LoadSetContainer(Exp_GetVal(ppCmd[2]), LAYER_NONE);
				g_World.m_uidNew = pItem->GetUID();

				if ( this != &g_Serv )
				{
					CChar *pChar = dynamic_cast <CChar *> (this);
					if ( pChar )
						pChar->m_Act_Targ = g_World.m_uidNew;
					else
					{
						CClient *pClient = dynamic_cast <CClient *> (this);
						if ( pClient && pClient->GetChar() )
							pClient->GetChar()->m_Act_Targ = g_World.m_uidNew;
					}
				}
			}
			break;

		case SSV_NEWNPC:
			{
				CREID_TYPE id = (CREID_TYPE) g_Cfg.ResourceGetIndexType(RES_CHARDEF, s.GetArgRaw());
				if ( id <= CREID_INVALID )
					return false;
				CChar *pChar = CChar::CreateNPC(id);
				if ( !pChar )
				{
					g_World.m_uidNew = (DWORD)0;
					return false;
				}
				CChar *pCharSrc = pSrc->GetChar();
				if ( pCharSrc )
				{
					pChar->MoveToChar(pCharSrc->GetTopPoint());
					pChar->MoveNearObj(pChar, 1);
					pChar->Update();
				}
				g_World.m_uidNew = pChar->GetUID();

				if ( this != &g_Serv )
				{
					pChar = dynamic_cast <CChar *> (this);
					if ( pChar )
						pChar->m_Act_Targ = g_World.m_uidNew;
					else
					{
						CClient *pClient = dynamic_cast <CClient *> (this);
						if ( pClient && pClient->GetChar() )
							pClient->GetChar()->m_Act_Targ = g_World.m_uidNew;
					}
				}
			}
			break;

		case SSV_SHOW:
			{
				CGString sVal;
				bool rc;
				char *z = s.GetArgStr();

				rc = r_WriteVal(z, sVal, pSrc);
				if ( !rc && pSrc->GetChar() )
					rc = pSrc->GetChar()->r_WriteVal(z, sVal, pSrc);

				if ( !rc )
					pSrc->SysMessagef("'%s' for '%s' is unresolved\n", z, GetName());
				else
				{
					TEMPSTRING(extra);
					ShowCmdExtras::CCP *table;

					if ( table = ShowCmdExtras::match(z) )
					{
						long l = ATOI(sVal);
						for ( int i = 0; ( i < 32 ) && ( i < ShowCmdExtras::maxIndex(table) ); i++ )
						{
							long mask = ( i ) ? 2 << i : 1;

							if ( l & ( mask ))
							{
								if ( *extra )
									strcat(extra, "+");
								strcat(extra, table[i]);
							}
						}
					}

					pSrc->SysMessagef("'%s' for '%s' is '%s'\n", z, GetName(), sVal.GetPtr());
					if ( *extra )
						pSrc->SysMessagef("(%s)\n", extra);
				}
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

bool CScriptObj::r_Load( CScript & s )
{
	while ( s.ReadKeyParse())
	{
		if ( s.IsKeyHead( "ON", 2 ))	// trigger scripting marks the end
			break;
		r_LoadVal(s);
	}
	return true;
}

int CScriptObj::ParseText( TCHAR * pszResponse, CTextConsole * pSrc, int iFlags, CScriptTriggerArgs * pArgs )
{
	// Take in a line of text that may have fields that can be replaced with operators here.
	// ARGS:
	// iFlags = 2=Allow recusive bracket count. 1=use HTML %% as the delimiters.
	// NOTE:
	//  html will have opening <script language="SPHERE_FILE"> and then closing </script>
	// RETURN:
	//  New length of the string.
	//
	// Parsing flags
	LPCTSTR			pszKey; // temporary, set below
	bool			fRes;

	static int sm_iReentrant = 0;
	static bool sm_fBrackets = false;	// allowed to span multi lines.
	if ( ! (iFlags&2))
	{
		sm_fBrackets = false;
	}

	int iBegin = 0;
	TCHAR chBegin = '<';
	TCHAR chEnd = '>';

	bool fHTML = (iFlags&1);
	if ( fHTML )
	{
		chBegin = '%';
		chEnd = '%';
	}

	int i;
	EXC_TRY("ParseText");
	for ( i = 0; pszResponse[i]; i++ )
	{
		TCHAR ch = pszResponse[i];

		if ( ! sm_fBrackets )	// not in brackets
		{
			if ( ch == chBegin )	// found the start !
			{
				 if ( !( isalnum( pszResponse[i+1] ) || pszResponse[i+1] == '<' ) )		// ignore this.
					continue;
				iBegin = i;
				sm_fBrackets = true;
			}
			continue;
		}

		if ( ch == '<' )	// recursive brackets
		{			
			if ( !( isalnum( pszResponse[i+1] ) || pszResponse[i+1] == '<' ) )		// ignore this.
				continue;
			
			if (sm_iReentrant > 32 )
			{
				EXC_SET("reentrant limit");
				ASSERT( sm_iReentrant < 32 );
			}
			sm_iReentrant++;
			sm_fBrackets = false;
			int ilen = ParseText( pszResponse+i, pSrc, 2, pArgs );
			sm_fBrackets = true;
			sm_iReentrant--;
			i += ilen;
			continue;
		}

		if ( ch == chEnd )
		{
			sm_fBrackets = false;
			pszResponse[i] = '\0';

			CGString sVal;
			pszKey		= (LPCTSTR) pszResponse+iBegin+1;

			EXC_SET("writeval");
			if ( !( fRes = r_WriteVal( pszKey, sVal, pSrc ) ) )
			{
				EXC_SET("writeval");
				if ( pArgs && pArgs->r_WriteVal( pszKey, sVal, pSrc ) )
					fRes	= true;
			}

			if ( !fRes )
			{
				g_Log.Error("Can't resolve <%s>\n", pszKey);
				// Just in case this really is a <= operator ?
				pszResponse[i] = chEnd;
			}

resolved:
			if ( sVal.IsEmpty() && fHTML )
			{
				sVal = "&nbsp";
			}

			int len = sVal.GetLength();
			EXC_SET("mem shifting");
			memmove( pszResponse+iBegin+len, pszResponse+i+1, strlen( pszResponse+i+1 ) + 1 );
			memcpy( pszResponse+iBegin, (LPCTSTR) sVal, len );
			i = iBegin+len-1;

			if ( iFlags&2 )	// just do this one then bail out.
				return( i );
		}
	}
	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.Debug("responce '%s' source addr '0%lx' flags '%d' args '%lx'\n", pszResponse, pSrc, iFlags, pArgs);
	EXC_DEBUG_END;
	return i;
}


TRIGRET_TYPE CScriptObj::OnTriggerForLoop( CScript &s, int iType, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * pResult )
{
	// loop from start here to the ENDFOR
	// See WebPageScriptList for dealing with Arrays.

	CScriptLineContext StartContext = s.GetContext();
	CScriptLineContext EndContext = StartContext;
	int LoopsMade = 0;
	int maxLoops = config.get("core.loop.max");

	if ( iType & 8 )		// WHILE
	{
		TCHAR *		pszCond;
		CGString	pszOrig;
		int			iWhile	= 0;

		TEMPSTRING(pszTemp);
		pszOrig.Copy( s.GetArgStr() );
		while(true)
		{
			LoopsMade++;
			if ( maxLoops && ( LoopsMade >= maxLoops ))
				goto toomanyloops;

			pArgs->m_VarsLocal.SetNum( "_WHILE", iWhile, false );
			iWhile++;
			strcpy( pszTemp, pszOrig.GetPtr() );
			pszCond	= pszTemp;
			ParseText( pszCond, pSrc, 0, pArgs );
			if ( !Exp_GetVal( pszCond ) )
				break;
			TRIGRET_TYPE iRet = OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult );
			if ( iRet != TRIGRET_ENDIF )
			{
				return( iRet );
			}
			EndContext = s.GetContext();
			s.SeekContext( StartContext );
		}
	}
	else
		ParseText( s.GetArgStr(), pSrc, 0, pArgs );


	
	if ( iType & 4 )		// FOR
	{ 
		int			iMin			= 0;
		int			iMax			= 0;
		int			i;
		TCHAR *		ppArgs[3];
		int			iQty			= Str_ParseCmds( s.GetArgStr(), ppArgs, 3, ", " );
		CGString	sLoopVar	= "_FOR";
		
		switch( iQty )
		{
		case 1:		// FOR x
			iMin	= 1;
			iMax	= Exp_GetSingle( ppArgs[0] );
			break;
		case 2:
			if ( isdigit( *ppArgs[0] ) )
			{
				iMin	= Exp_GetSingle( ppArgs[0] );
				iMax	= Exp_GetSingle( ppArgs[1] );
			}
			else
			{
				iMin		= 1;
				iMax		= Exp_GetSingle( ppArgs[1] );
				sLoopVar	= ppArgs[0];
			}
			break;
		case 3:
			sLoopVar	= ppArgs[0];
			iMin		= Exp_GetSingle( ppArgs[1] );;
			iMax		= Exp_GetSingle( ppArgs[2] );
			break;
		default:
			iMin	= iMax		= 1;
			break;
		}

		bool fCountDown = ( iMin > iMax );

		if ( fCountDown )
		{
			for ( i = iMin; i >= iMax; --i )
			{
				LoopsMade++;
				if ( maxLoops && ( LoopsMade >= maxLoops ))
					goto toomanyloops;

				pArgs->m_VarsLocal.SetNum( sLoopVar, i, false );
				TRIGRET_TYPE iRet = OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult );
				if ( iRet != TRIGRET_ENDIF )
				{
					return( iRet );
				}
				EndContext = s.GetContext();
				s.SeekContext( StartContext );
			}
		}
		else
		{
			for ( i = iMin; i <= iMax; ++i )
			{
				LoopsMade++;
				if ( maxLoops && ( LoopsMade >= maxLoops ))
					goto toomanyloops;

				pArgs->m_VarsLocal.SetNum( sLoopVar, i, false );
				TRIGRET_TYPE iRet = OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult );
				if ( iRet != TRIGRET_ENDIF )
				{
					return( iRet );
				}
				EndContext = s.GetContext();
				s.SeekContext( StartContext );
			}
		}
	}

	if ( (iType & 1) || (iType & 2) )
	{
		int iDist;
		if ( s.HasArgs() )
			iDist = s.GetArgVal();
		else
			iDist = UO_MAP_VIEW_SIZE;

		CObjBaseTemplate * pObj = dynamic_cast <CObjBaseTemplate *>(this);
		if ( pObj == NULL )
		{
			iType = 0;
			g_Log.Error("FOR Loop trigger on non-world object '%s'\n", GetName());
		}
		else
		{
			CObjBaseTemplate * pObjTop = pObj->GetTopLevelObj();
			CPointMap pt = pObjTop->GetTopPoint();
			if ( iType & 1 )		// FORITEM, FOROBJ
			{
				CWorldSearch AreaItems(pt, iDist);
				while ( CItem *pItem = AreaItems.GetItem() )
				{
					LoopsMade++;
					if ( maxLoops && ( LoopsMade >= maxLoops )) goto toomanyloops;

					TRIGRET_TYPE iRet = pItem->OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult );
					if ( iRet != TRIGRET_ENDIF )
					{
						return( iRet );
					}
					EndContext = s.GetContext();
					s.SeekContext( StartContext );
				}
			}
			if ( iType & 2 )		// FORCHAR, FOROBJ
			{
				CWorldSearch AreaChars(pt, iDist);
				while ( CChar *pChar = AreaChars.GetChar() )
				{
					LoopsMade++;
					if ( maxLoops && ( LoopsMade >= maxLoops )) goto toomanyloops;

					if ( ( iType & 0x10 ) && ( ! pChar->IsClient() ) )	// FORCLIENTS
						continue;
					if ( ( iType & 0x20 ) && ( pChar->m_pPlayer == NULL ) )	// FORPLAYERS
						continue;
					TRIGRET_TYPE iRet = pChar->OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult );
					if ( iRet != TRIGRET_ENDIF )
					{
						return( iRet );
					}
					EndContext = s.GetContext();
					s.SeekContext( StartContext );
				}
			}
		}
	}

	if ( iType & 0x40 )	// FORINSTANCES
	{
		RESOURCE_ID rid;
		TCHAR * ppArgs[1];

		if (Str_ParseCmds( s.GetArgStr(), ppArgs, COUNTOF( ppArgs ), " \t," ) >= 1)
		{
			rid = g_Cfg.ResourceGetID( RES_UNKNOWN, ppArgs[0] );
		}
		else
		{
			CObjBase * pObj = dynamic_cast <CObjBase*>(this);
			if ( pObj && pObj->Base_GetDef() )
			{
				rid = pObj->Base_GetDef()->GetResourceID();
			}
		}

		// No need to loop if there is no valid resource id
		if ( rid.IsValidUID() )
		{
			DWORD dwUID = 0;
			DWORD dwTotal = g_World.GetUIDCount();
			DWORD dwCount = dwTotal-1;

			while ( dwCount-- )
			{
				// Check the current UID to test is within our range
				if ( ++dwUID >= dwTotal )
					break;

				// Acquire the object with this UID and check it exists
				CObjBase * pObj = g_World.FindUID( dwUID );
				if ( pObj == NULL )
					continue;

				// Check to see if the object resource id matches what we're looking for
				if (pObj->Base_GetDef()->GetResourceID() != rid)
					continue;

				// Check we do not loop too many times
				LoopsMade++;
				if ( maxLoops && ( LoopsMade >= maxLoops )) goto toomanyloops;

				// Execute script on this object
				TRIGRET_TYPE iRet = pObj->OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult );
 				if ( iRet != TRIGRET_ENDIF )
				{
					return( iRet );
				}
				EndContext = s.GetContext();
				s.SeekContext( StartContext );
			}
		}
	}

	if ( maxLoops )
	{
toomanyloops:
		if ( LoopsMade >= maxLoops )
		{
			g_Log.Error("Terminating loop cycle since it seems being dead-locked (%d iterations already passed)\n", LoopsMade);
		}
	}

	if ( EndContext.m_lOffset <= StartContext.m_lOffset )
	{
		// just skip to the end.
		TRIGRET_TYPE iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
		if ( iRet != TRIGRET_ENDIF )
		{
			return( iRet );
		}
	}
	else
	{
		s.SeekContext( EndContext );
	}
	return( TRIGRET_ENDIF );
}

bool CScriptObj::OnTriggerFind( CScript & s, LPCTSTR pszTrigName )
{
	while ( s.ReadKey(false) )
	{
		// Is it a trigger ?
		if ( strnicmp(s.GetKey(), "ON", 2) )
			continue;

		// Is it the right trigger ?
		s.ParseKeyLate();
		if ( !strcmpi(s.GetArgRaw(), pszTrigName) )
			return true;
	}
	return false;
}

TRIGRET_TYPE CScriptObj::OnTriggerScript( CScript & s, LPCTSTR pszTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs )
{
	// look for exact trigger matches
	if ( !OnTriggerFind(s, pszTrigName) )
		return TRIGRET_RET_DEFAULT;

	CThread *pThread = CThread::Thread();

	TScriptProfiler::TScriptProfilerTrigger	*pTrig;
	TIME_PROFILE_INIT;

	//	If script profiler is on, search this trigger record and get pointer to it
	//	if not, create the corresponding record
	if ( IsSetEF(EF_Script_Profiler) )
	{
		TEMPSTRING(pName);
		//	lowercase for speed
		strcpy(pName, pszTrigName);
		_strlwr(pName);

		g_profiler.init();
		for ( pTrig = g_profiler.TriggersHead; pTrig != NULL; pTrig = pTrig->next )
		{
			if ( !strcmp(pTrig->name, pName) ) break;
		}

		// first time function called. so create a record for it
		if ( !pTrig )
		{
			pTrig = new TScriptProfiler::TScriptProfilerTrigger;
			memset(pTrig, 0, sizeof(TScriptProfiler::TScriptProfilerTrigger));
			strcpy(pTrig->name, pName);
			if ( g_profiler.TriggersTail ) g_profiler.TriggersTail->next = pTrig;
			else g_profiler.TriggersHead = pTrig;
			g_profiler.TriggersTail = pTrig;
		}

		//	prepare the informational block
		pTrig->called++;
		g_profiler.called++;
		TIME_PROFILE_START;
	}

	Lock(true);
	TRIGRET_TYPE	iRet = OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs);
	Release();

	if ( IsSetEF(EF_Script_Profiler) )
	{
		//	update the time call information
		TIME_PROFILE_END;
		llTicks = llTicksEnd - llTicks;
		pTrig->total += llTicks;
		pTrig->average = (pTrig->total/pTrig->called);
		if ( pTrig->max < llTicks ) pTrig->max = llTicks;
		if (( pTrig->min > llTicks ) || ( !pTrig->min )) pTrig->min = llTicks;
		g_profiler.total += llTicks;
	}
	return iRet;
}



enum SK_TYPE
{
	SK_BEGIN,
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
	SK_FORCONTID,		// loop through all items with this ID in the cont
	SK_FORCONTTYPE,
	SK_FORINSTANCE,
	SK_FORITEM,
	SK_FOROBJ,
	SK_FORPLAYERS,		// not necessary to be online
	SK_IF,
	SK_RETURN,
	SK_WHILE,
	SK_QTY,
};



LPCTSTR const CScriptObj::sm_szScriptKeys[SK_QTY+1] =
{
	"BEGIN",
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
	"IF",
	"RETURN",
	"WHILE",
	NULL,
};



TRIGRET_TYPE CScriptObj::OnTriggerRun( CScript &s, TRIGRUN_TYPE trigrun, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * pResult )
{
	// ARGS:
	//	TRIGRUN_SECTION_SINGLE = just this 1 line.
	// RETURN:
	//  TRIGRET_RET_FALSE = 0 = return and continue processing.
	//  TRIGRET_RET_TRUE = 1 = return and handled. (halt further processing)
	//  TRIGRET_RET_DEFAULT = 2 = if process returns nothing specifically.

	// CScriptFileContext set g_Log.m_pObjectContext is the current context (we assume)
	// DEBUGCHECK( this == g_Log.m_pObjectContext );

	//	Script execution is always not threaded action
	EXC_TRY("TriggerRun");

	//	all scripts should have args for locals to work.
	CScriptTriggerArgs argsEmpty;
	if ( !pArgs )
		pArgs = &argsEmpty;

	bool fSectionFalse = (trigrun == TRIGRUN_SECTION_FALSE || trigrun == TRIGRUN_SINGLE_FALSE);
	if ( trigrun == TRIGRUN_SECTION_EXEC || trigrun == TRIGRUN_SINGLE_EXEC )	// header was already read in.
		goto jump_in;

	EXC_SET("parsing");
	while ( s.ReadKeyParse())
	{
		// Hit the end of the next trigger.
		if ( s.IsKeyHead( "ON", 2 ))	// done with this section.
			break;

jump_in:
		SK_TYPE iCmd = (SK_TYPE) FindTableSorted( s.GetKey(), sm_szScriptKeys, COUNTOF( sm_szScriptKeys )-1 );
		TRIGRET_TYPE iRet;

		switch ( iCmd )
		{
		case SK_ENDIF:
		case SK_END:
		case SK_ENDDO:
		case SK_ENDFOR:
		case SK_ENDRAND:
		case SK_ENDSWITCH:
		case SK_ENDWHILE:
			return( TRIGRET_ENDIF );

		case SK_ELIF:
		case SK_ELSEIF:
			return( TRIGRET_ELSEIF );

		case SK_ELSE:
			return( TRIGRET_ELSE );
		}

		if ( fSectionFalse )
		{
			// Ignoring this whole section. don't bother parsing it.
			switch ( iCmd )
			{
			case SK_IF:
				EXC_SET("if statement");
				do
				{
					iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
				} while ( iRet == TRIGRET_ELSEIF || iRet == TRIGRET_ELSE );
				break;
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
			case SK_DORAND:
			case SK_DOSWITCH:
			case SK_BEGIN:
				EXC_SET("begin/loop cycle");
				iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
				break;
			}
			if ( trigrun >= TRIGRUN_SINGLE_EXEC )
				return( TRIGRET_RET_DEFAULT );
			continue;	// just ignore it.
		}

		switch ( iCmd )
		{
		case SK_FORITEM:		EXC_SET("foritem");		iRet = OnTriggerForLoop( s, 1, pSrc, pArgs, pResult );			break;
		case SK_FORCHAR:		EXC_SET("forchar");		iRet = OnTriggerForLoop( s, 2, pSrc, pArgs, pResult );			break;
		case SK_FORCLIENTS:		EXC_SET("forclients");	iRet = OnTriggerForLoop( s, 0x12, pSrc, pArgs, pResult );		break;
		case SK_FOROBJ:			EXC_SET("forobjs");		iRet = OnTriggerForLoop( s, 3, pSrc, pArgs, pResult );			break;
		case SK_FORPLAYERS:		EXC_SET("forplayers");	iRet = OnTriggerForLoop( s, 0x22, pSrc, pArgs, pResult );		break;
		case SK_FOR:			EXC_SET("for");			iRet = OnTriggerForLoop( s, 4, pSrc, pArgs, pResult );			break;
		case SK_WHILE:			EXC_SET("while");		iRet = OnTriggerForLoop( s, 8, pSrc, pArgs, pResult );			break;
		case SK_FORINSTANCE:	EXC_SET("forinstance");	iRet = OnTriggerForLoop( s, 0x40, pSrc, pArgs, pResult );		break;
		case SK_FORCHARLAYER:
		case SK_FORCHARMEMORYTYPE:
			{
				EXC_SET("forchar[layer/memorytype]");
				CChar * pCharThis = dynamic_cast <CChar *> (this);
				if ( pCharThis )
				{
					if ( s.HasArgs() )
					{
						if ( iCmd == SK_FORCHARLAYER )
							iRet = pCharThis->OnCharTrigForLayerLoop( s, pSrc, pArgs, pResult, (LAYER_TYPE) s.GetArgVal() );
						else
							iRet = pCharThis->OnCharTrigForMemTypeLoop( s, pSrc, pArgs, pResult, s.GetArgVal() );
 					}
				}
			} break;
		case SK_FORCONT:
			{
				EXC_SET("forcont");
				if ( s.HasArgs() )
				{
					TCHAR *ppArgs[2];
					TCHAR *tempPoint;
					TEMPSTRING(porigValue);
					int iArgQty = Str_ParseCmds( (TCHAR*) s.GetArgRaw(), ppArgs, COUNTOF(ppArgs), " \t," );
					
					if ( iArgQty >= 1 )
					{
						strcpy(porigValue, ppArgs[0]);
						tempPoint = porigValue;
						ParseText( tempPoint, pSrc, 0, pArgs );
						
						UID pCurUid = (DWORD) Exp_GetVal(tempPoint);
						if ( pCurUid.IsValidUID() )
						{
							CObjBase * pObj = pCurUid.ObjFind();
							if ( pObj && pObj->IsContainer() )
							{
								CContainer * pContThis = dynamic_cast <CContainer *> (pObj);
								
								CScriptLineContext StartContext = s.GetContext();
								CScriptLineContext EndContext = StartContext;
								iRet = pContThis->OnGenericContTriggerForLoop( s, pSrc, pArgs, pResult, StartContext, EndContext, ppArgs[1] != NULL ? Exp_GetVal(ppArgs[1]) : 255 );
							}
						}
					}
				}
			} break;

		case SK_FORCONTID:
		case SK_FORCONTTYPE:
			{
				EXC_SET("forcont[id/type]");
				CContainer * pCont = dynamic_cast <CContainer *> (this);
				if ( pCont )
				{
					if ( s.HasArgs() )
					{
						LPCTSTR pszKey = s.GetArgRaw();
						SKIP_SEPARATORS(pszKey);
					
						TCHAR * ppArgs[2];
						
						TEMPSTRING(ppParsed);
						if ( Str_ParseCmds((TCHAR *)pszKey, ppArgs, COUNTOF(ppArgs), " \t,") >= 1 )
						{
							TEMPSTRING(ppParsed);
							strcpy(ppParsed, ppArgs[0]);
							if ( ParseText(ppParsed, pSrc, 0, pArgs) )
							{
								CScriptLineContext StartContext = s.GetContext();
								CScriptLineContext EndContext = StartContext;
								long levels = (ppArgs[1] != NULL) ? Exp_GetVal(ppArgs[1]) : 255;
								RESOURCE_ID_BASE rid = g_Cfg.ResourceGetID( ( iCmd == SK_FORCONTID ) ? RES_ITEMDEF : RES_TYPEDEF, (const char*&)ppArgs[0]);
								iRet = pCont->OnContTriggerForLoop(s, pSrc, pArgs, pResult, StartContext, EndContext, rid, 0, levels);
							}
						}
					}
				}
			} break;
		default:
			// Parse out any variables in it. (may act like a verb sometimes?)
			EXC_SET("parsing");
			ParseText( s.GetArgRaw(), pSrc, 0, pArgs );
		}

		switch ( iCmd )
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
		case SK_FOR:
		case SK_WHILE:
			if ( iRet != TRIGRET_ENDIF )
			{
				return iRet;
			}
			break;
		case SK_DORAND:	// Do a random line in here.
		case SK_DOSWITCH:
			{
			EXC_SET("dorand/doswitch");
			int iVal = s.GetArgVal();
			if ( iCmd == SK_DORAND )
				iVal = Calc_GetRandVal(iVal);
			for ( ;true; iVal-- )
			{
				iRet = OnTriggerRun( s, (!iVal) ? TRIGRUN_SINGLE_TRUE : TRIGRUN_SINGLE_FALSE, pSrc, pArgs, pResult );
				if ( iRet == TRIGRET_RET_DEFAULT )
					continue;
				if ( iRet == TRIGRET_ENDIF )
					break;
				return iRet;
			}
			}
			break;
		case SK_RETURN:		// Process the trigger.
			EXC_SET("return");
			if ( pResult )
			{
				pResult->Copy( s.GetArgStr() );
				return (TRIGRET_TYPE) 1;
			}
			return ( (TRIGRET_TYPE) s.GetArgVal() );
		case SK_IF:
			{
				EXC_SET("if statement");
				bool fTrigger = s.GetArgVal() ? true : false;
				bool fBeenTrue = false;
				while (true)
				{
					iRet = OnTriggerRun( s, fTrigger ? TRIGRUN_SECTION_TRUE : TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
					if ( iRet < TRIGRET_ENDIF )
						return( iRet );
					if ( iRet == TRIGRET_ENDIF )
						break;
					fBeenTrue |= fTrigger;
					if ( fBeenTrue )
						fTrigger = false;
					else if ( iRet == TRIGRET_ELSE )
						fTrigger = true;
					else if ( iRet == TRIGRET_ELSEIF )
					{
						ParseText( s.GetArgStr(), pSrc, 0, pArgs );
						fTrigger = s.GetArgVal() ? true : false;
					}
				}
			}
			break;

		case SK_BEGIN:
			// Do this block here.
			{
				EXC_SET("begin/loop cycle");
				iRet = OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult );
				if ( iRet != TRIGRET_ENDIF )
					return iRet;
			}
			break;

		default:
			{
				if ( strchr(s.GetKey(), '<') )
				{
					EXC_SET("parsing <> in a key");
					TEMPSTRING(buf);
					strcpy(buf, s.GetKey());
					ParseText(buf, pSrc, 0, pArgs);
					strcat(buf, " ");
					strcat(buf, s.GetArgRaw());
					CScript script(buf);
					r_Verb(script, pSrc);
					break;
				}
				EXC_SET("parsing");
				if ( !pArgs->r_Verb(s, pSrc) )
				{
					bool	fRes;
					if ( !strcmpi(s.GetKey(), "call" ) )
					{
						EXC_SET("call");
						char *argRaw = s.GetArgRaw();
						char *z = strchr(argRaw, ' ');
						CGString sVal;

						if( z )
						{
							*z = 0;
							z++;
							GETNONWHITESPACE(z);
						}

						if ( z && *z )
						{
							int iN1 = pArgs->m_iN1;
							int iN2 = pArgs->m_iN2;
							int iN3 = pArgs->m_iN3;
							CScriptObj *pO1 = pArgs->m_pO1;
							CGString s1 = pArgs->m_s1;
							CGString s1_raw = pArgs->m_s1;
							pArgs->Init(z);

							fRes = this->r_Call(argRaw, pSrc, pArgs, &sVal);

							pArgs->m_iN1 = iN1;
							pArgs->m_iN2 = iN2;
							pArgs->m_iN3 = iN3;
							pArgs->m_pO1 = pO1;
							pArgs->m_s1 = s1;
							pArgs->m_s1_raw = s1_raw;
						}
						else
							fRes = this->r_Call(argRaw, pSrc, pArgs, &sVal);
					}
					else
					{
						EXC_SET("verb");
						fRes = r_Verb(s, pSrc);
					}

					if ( !fRes  )
					{
						g_Log.Warn("Trigger Bad Verb '%s','%s'\n", s.GetKey(), s.GetArgStr());
					}
				}
			} break;
		}

		if ( trigrun >= TRIGRUN_SINGLE_EXEC )
			return( TRIGRET_RET_DEFAULT );
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.Debug("key '%s' runtype '%d' pargs '%lx' ret '%s' [0%lx]",
		s.GetKey(), trigrun, pArgs, *pResult, pSrc);
	EXC_DEBUG_END;
	return TRIGRET_RET_DEFAULT;
}

////////////////////////////////////////////////////////////////////////////////////////
// -CFileObj

enum FO_TYPE
{
	#define ADD(a,b) FO_##a,
	#include "../tables/CFile_props.tbl"
	#undef ADD
	FO_QTY,
};

LPCTSTR const CFileObj::sm_szLoadKeys[FO_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CFile_props.tbl"
	#undef ADD
	NULL,
};

enum FOV_TYPE
{
	#define ADD(a,b) FOV_##a,
	#include "../tables/CFile_functions.tbl"
	#undef ADD
	FOV_QTY,
};

LPCTSTR const CFileObj::sm_szVerbKeys[FOV_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CFile_functions.tbl"
	#undef ADD
	NULL,
};

CFileObj::CFileObj()
{
	sWrite = new CFileText();
	tBuffer = new TCHAR [SCRIPT_MAX_LINE_LEN];
	cgWriteBuffer = new CGString();
	SetDefaultMode();
}

CFileObj::~CFileObj()
{
	if (sWrite->IsFileOpen())
		sWrite->Close();

	delete cgWriteBuffer;
	delete[] tBuffer;
	delete sWrite;
}

void CFileObj::SetDefaultMode(void)
{ 
	bAppend = true; bCreate = false; 
	bRead = true; bWrite = true; 
}

TCHAR * CFileObj::GetReadBuffer(bool bDelete = false)
{
	if ( bDelete )
		memset(this->tBuffer, 0, SCRIPT_MAX_LINE_LEN);
	else
		*tBuffer = 0;

	return tBuffer;
}

CGString * CFileObj::GetWriteBuffer(void)
{
	if ( !cgWriteBuffer )
		cgWriteBuffer = new CGString();

	cgWriteBuffer->Empty( ( cgWriteBuffer->GetLength() > (SCRIPT_MAX_LINE_LEN/4) ) );

	return cgWriteBuffer;
}

bool CFileObj::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef ){ return false; }


bool CFileObj::r_LoadVal( CScript & s )
{	
	EXC_TRY("LoadVal");
	LPCTSTR pszKey = s.GetKey();

	if ( !strnicmp("MODE.",pszKey,5) )
	{
		pszKey += 5;
		if ( ! sWrite->IsFileOpen() )
		{
			if ( !strnicmp("APPEND",pszKey,6) )
			{
				bAppend = s.GetArgVal();
				bCreate = false;
			}
			else if ( !strnicmp("CREATE",pszKey,6) )
			{
				bCreate = s.GetArgVal();
				bAppend = false;
			}
			else if ( !strnicmp("READFLAG",pszKey,8) )
				bRead = s.GetArgVal();
			else if ( !strnicmp("WRITEFLAG",pszKey,9) )
				bWrite = s.GetArgVal();
			else if ( !strnicmp("SETDEFAULT",pszKey,7) )
				SetDefaultMode();
			else
				return false;

			return true;
		}
		else
		{
			g_Log.Event(LOGL_ERROR, "FILE (%s): Cannot set mode after file opening\n", sWrite->GetFilePath());
		}	
		return false;
	}

	int index = FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );

	switch ( index )
	{
		case FO_WRITE:
		case FO_WRITECHR:
		case FO_WRITELINE:
			{
				bool bLine = (index == FO_WRITELINE);
				bool bChr = (index == FO_WRITECHR);

				if ( !sWrite->IsFileOpen() )
				{
					g_Log.Event(LOGL_ERROR, "FILE: Cannot write content. Open the file first.\n");
					return false;
				}

				if ( !s.HasArgs() )
					return false;
					
				CGString * ppArgs = this->GetWriteBuffer();
				ppArgs->Copy( s.GetArgStr() );

				if ( bLine )
					ppArgs->Add( "\n" );
				else if ( bChr )
					ppArgs->FormatVal( ATOI(ppArgs->GetPtr()) );

				bool bSuccess = false;

				if ( bChr )
					bSuccess = sWrite->Write(ppArgs->GetPtr(), 1);
				else
					bSuccess = sWrite->WriteString( ppArgs->GetPtr() );
		
				if ( !bSuccess )
				{
					g_Log.Event(LOGL_ERROR, "FILE: Failed writing to \"%s\".\n", sWrite->GetFilePath());
					return false;
				}
			} break;

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

bool CFileObj::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	EXC_TRY("WriteVal");

	if ( !strnicmp("MODE.",pszKey,5) )
	{
		pszKey += 5;
		if ( !strnicmp("APPEND",pszKey,6) )
			sVal.FormatVal( bAppend );
		else if ( !strnicmp("CREATE",pszKey,6) )
			sVal.FormatVal( bCreate );
		else if ( !strnicmp("READFLAG",pszKey,8) )
			sVal.FormatVal( bRead );
		else if ( !strnicmp("WRITEFLAG",pszKey,9) )
			sVal.FormatVal( bWrite );
		else
			return false;

		return true;
	}

	int index = FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );

	switch ( index )
	{
		case FO_FILEEXIST:
			{
				pszKey += strlen(sm_szLoadKeys[index]);
				GETNONWHITESPACE( pszKey );

				TCHAR * ppCmd = Str_TrimWhitespace((TCHAR *)pszKey);
				if ( !( ppCmd && strlen(ppCmd) ))
					return false;

				CFile * pFileTest = new CFile();
				sVal.FormatVal(pFileTest->Open(ppCmd));

				delete pFileTest;
			} break;

		case FO_FILELINES:
			{
				pszKey += strlen(sm_szLoadKeys[index]);
				GETNONWHITESPACE( pszKey );
				
				TCHAR * ppCmd = Str_TrimWhitespace((TCHAR *)pszKey);
				if ( !( ppCmd && strlen(ppCmd) ))
					return false;

				CFileText * sFileLine = new CFileText();
				if ( !sFileLine->Open(ppCmd, OF_READ|OF_TEXT) )
				{
					delete sFileLine;
					return false;
				}

				TCHAR * ppArg = this->GetReadBuffer();
				int iLines = 0;

				while ( ! sFileLine->IsEOF() )
				{
					sFileLine->ReadString( ppArg, SCRIPT_MAX_LINE_LEN );
					iLines++;
				}
				sFileLine->Close();

				sVal.FormatVal( iLines );
				
				delete sFileLine;
			} break;

		case FO_FILEPATH:
			sVal.Format("%s", sWrite->IsFileOpen() ? sWrite->GetFilePath() : "" );
			break;

		case FO_INUSE:
			sVal.FormatVal( sWrite->IsFileOpen() );
			break;

		case FO_ISEOF:
			sVal.FormatVal( sWrite->IsEOF() );
			break;

		case FO_LENGTH:
			sVal.FormatVal( sWrite->IsFileOpen() ? sWrite->GetLength() : -1 );
			break;

		case FO_OPEN:
			{
				pszKey += strlen(sm_szLoadKeys[index]);
				GETNONWHITESPACE( pszKey );

				TCHAR * ppCmd = Str_TrimWhitespace((TCHAR *)pszKey);
				if ( !( ppCmd && strlen(ppCmd) ))
					return false;

				if ( sWrite->IsFileOpen() )
				{
					g_Log.Event(LOGL_ERROR, "FILE: Cannot open file (%s). First close \"%s\".\n", ppCmd, sWrite->GetFilePath());
					return false;
				}

				sVal.FormatVal( FileOpen(ppCmd) );
			} break;

		case FO_POSITION:
			sVal.FormatVal( sWrite->GetPosition() );
			break;

		case FO_READBYTE:
		case FO_READCHAR:
			{
				bool bChr = ( index == FO_READCHAR );
				int iRead = 1;

				if ( !bChr )
				{
					pszKey += strlen(sm_szLoadKeys[index]);
					GETNONWHITESPACE( pszKey );

					iRead = Exp_GetVal(pszKey);
					if ( iRead <= 0 || iRead >= SCRIPT_MAX_LINE_LEN)
						return false;
				}

				if ( ( ( sWrite->GetPosition() + iRead ) > sWrite->GetLength() ) || ( sWrite->IsEOF() ) )
					return false;

				TCHAR * ppArg = this->GetReadBuffer(true);

				if ( iRead != sWrite->Read(ppArg,iRead) )
				{
					g_Log.Event(LOGL_ERROR, "FILE: Failed reading %d byte from \"%s\".\n", iRead, sWrite->GetFilePath());
					return false;
				}

				if ( bChr )
					sVal.FormatVal( *ppArg );
				else
					sVal.Format( "%s", ppArg );
			} break;

		case FO_READLINE:
			{
				pszKey += strlen(sm_szLoadKeys[index]);
				GETNONWHITESPACE( pszKey );

				TCHAR * ppArg = this->GetReadBuffer();
				int iLines = 0;

				iLines = Exp_GetVal(pszKey);
				if ( iLines < 0 )
					return false;

				unsigned long ulSeek = sWrite->GetPosition();
				sWrite->SeekToBegin();

				if ( !iLines )
				{
					while ( ! sWrite->IsEOF() )
						sWrite->ReadString( ppArg, SCRIPT_MAX_LINE_LEN );	
				}
				else
				{
					for ( int x = 1; x <= iLines; x++ )
					{
						if ( sWrite->IsEOF() )
							break;

						ppArg = this->GetReadBuffer();
						sWrite->ReadString( ppArg, SCRIPT_MAX_LINE_LEN );
					}
				}

				sWrite->Seek(ulSeek);

				if ( int iLinelen = strlen(ppArg) )
				{
					while ( iLinelen > 0 )
					{
						iLinelen--;
						if ( isgraph(ppArg[iLinelen]) || (ppArg[iLinelen] == 0x20) || (ppArg[iLinelen] == '\t') )
						{
							++iLinelen;
							ppArg[iLinelen] = '\0';
							break;
						}
					}
				}

				sVal.Format( "%s", (ppArg && strlen(ppArg)) ? ppArg : "" );
			} break;

		case FO_SEEK:
			{
				pszKey += strlen(sm_szLoadKeys[index]);
				GETNONWHITESPACE( pszKey );

				if ( pszKey && !strlen(pszKey) )
					return false;

				if (!strcmpi("BEGIN",pszKey))
				{
					sVal.FormatVal( sWrite->Seek(0,SEEK_SET) );
				}
				else if (!strcmpi("END",pszKey))
				{
					sVal.FormatVal( sWrite->SeekToEnd() );
				}
				else
				{
					sVal.FormatVal( sWrite->Seek(Exp_GetVal(pszKey)) );
				}
			} break;

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


bool CFileObj::r_Verb( CScript & s, CTextConsole * pSrc )
{
	EXC_TRY("Verb");
	LPCTSTR pszKey = s.GetKey();

	int index = FindTableSorted( pszKey, sm_szVerbKeys, COUNTOF( sm_szVerbKeys )-1 );

	if ( index < 0 )
		return( this->r_LoadVal( s ) );

	switch ( index )
	{
		case FOV_CLOSE:
			if ( sWrite->IsFileOpen() )
					sWrite->Close();
			break;

		case FOV_DELETEFILE:
			{
				if ( !s.GetArgStr() )
					return false;

				if ( sWrite->IsFileOpen() && !strcmp(s.GetArgStr(),sWrite->GetFileTitle()) )
					return false;

				_unlink(s.GetArgRaw());
			} break;

		case FOV_FLUSH:
			if ( sWrite->IsFileOpen() )
				sWrite->Flush();
			break;

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

bool CFileObj::FileOpen( LPCTSTR sPath )
{
	if ( sWrite->IsFileOpen() )
		return false;

	UINT uMode = OF_SHARE_DENY_NONE | OF_TEXT;

	if ( bCreate )	// if we create, we can't append or read
		uMode |= OF_CREATE;
	else
	{
		if (( bRead && bWrite ) || bAppend )
			uMode |= OF_READWRITE;
		else
		{
			if ( bRead )
				uMode |= OF_READ;
			else if ( bWrite )
				uMode |= OF_WRITE;
		}
	}

	return( sWrite->Open(sPath, uMode) );
}

////////////////////////////////////////////////////////////////////////////////////////
// -CFileObjCollection
