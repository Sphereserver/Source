//
// CScriptObj.cpp
// A scriptable object.
//

#ifdef _WIN32
	#include <process.h>
#else
	#include <errno.h>	// errno
#endif

#include "../graysvr/graysvr.h"

////////////////////////////////////////////////////////////////////////////////////////
// -CScriptTriggerArgs

void CScriptTriggerArgs::Init( LPCTSTR pszStr )
{
	ADDTOCALLSTACK("CScriptTriggerArgs::Init");
	m_pO1 = NULL;

	if ( pszStr == NULL )
		pszStr	= "";
	// raw is left untouched for now - it'll be split the 1st time argv is accessed
	m_s1_raw		= pszStr;
	bool fQuote = false;
	if ( *pszStr == '"' )
	{
		fQuote = true;
		++pszStr;
	}

	m_s1	= pszStr ;

	// take quote if present.
	if (fQuote)
	{
		TCHAR * str = const_cast<TCHAR*>( strchr(m_s1.GetPtr(), '"') );
		if ( str != NULL )
			*str	= '\0';
	}
	
	m_iN1	= 0;
	m_iN2	= 0;
	m_iN3	= 0;

	// attempt to parse this.
	if ( IsDigit(*pszStr) || ((*pszStr == '-') && IsDigit(*(pszStr+1))) )
	{
		m_iN1 = Exp_GetSingle(pszStr);
		SKIP_ARGSEP( pszStr );
		if ( IsDigit(*pszStr) || ((*pszStr == '-') && IsDigit(*(pszStr+1))) )
		{
			m_iN2 = Exp_GetSingle(pszStr);
			SKIP_ARGSEP( pszStr );
			if ( IsDigit(*pszStr) || ((*pszStr == '-') && IsDigit(*(pszStr+1))) )
			{
				m_iN3 = Exp_GetSingle(pszStr);
			}
		}
	}

	// ensure argv will be recalculated next time it is accessed
	m_v.SetCount(0);
}



CScriptTriggerArgs::CScriptTriggerArgs( LPCTSTR pszStr )
{
	Init( pszStr );
}



bool CScriptTriggerArgs::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CScriptTriggerArgs::r_GetRef");

	if ( !strnicmp(pszKey, "ARGO.", 5) )	// ARGO.NAME
	{
		pszKey += 5;
		if ( *pszKey == '1' )
			++pszKey;
		pRef = m_pO1;
		return( true );
	}
	else if ( !strnicmp(pszKey, "REF", 3) )	// REF[1-65535].NAME
	{
		LPCTSTR pszTemp = pszKey;
		pszTemp += 3;
		if (*pszTemp && IsDigit( *pszTemp ))
		{
			char * pEnd;
			unsigned short number = static_cast<unsigned short>(strtol( pszTemp, &pEnd, 10 ));
			if ( number > 0 ) // Can only use 1 to 65535 as REFs
			{
				pszTemp = pEnd;
				// Make sure REFx or REFx.KEY is being used
				if (( !*pszTemp ) || ( *pszTemp == '.' ))
				{
					if ( *pszTemp == '.' ) pszTemp++;

					pRef = m_VarObjs.Get( number );
					pszKey = pszTemp;
					return true;
				}
			}
		}
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
	AGC_TRY,
	AGC_TRYSRV,
	AGC_QTY
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
	"TRY",
	"TRYSRV",
	NULL
};


bool CScriptTriggerArgs::r_Verb( CScript & s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CScriptTriggerArgs::r_Verb");
	EXC_TRY("Verb");
	int	index = -1;
	LPCTSTR pszKey = s.GetKey();

	if ( !strnicmp( "FLOAT.", pszKey, 6 ) )
	{
		return( m_VarsFloat.Insert( (pszKey+6), s.GetArgStr(), true ) );
	}
	else if ( !strnicmp( "LOCAL.", pszKey, 6 ) )
	{
		bool fQuoted = false;
		m_VarsLocal.SetStr( s.GetKey()+6, fQuoted, s.GetArgStr( &fQuoted ), false );
		return( true );
	}
	else if ( !strnicmp( "REF", pszKey, 3 ) )
	{
		LPCTSTR pszTemp = pszKey;
		pszTemp += 3;
		if (*pszTemp && IsDigit( *pszTemp ))
		{
			char * pEnd;
			unsigned short number = static_cast<unsigned short>(strtol( pszTemp, &pEnd, 10 ));
			if ( number > 0 ) // Can only use 1 to 65535 as REFs
			{
				pszTemp = pEnd;
				if ( !*pszTemp ) // setting REFx to a new object
				{
					CGrayUID uid = s.GetArgVal();
					CObjBase * pObj = uid.ObjFind();
					m_VarObjs.Insert( number, pObj, true );
					pszKey = pszTemp;
					return( true );
				}
				else if ( *pszTemp == '.' ) // accessing REFx object
				{
					pszKey = ++pszTemp;
					CObjBase * pObj = m_VarObjs.Get( number );
					if ( !pObj )
						return false;

					CScript script( pszKey, s.GetArgStr());
					return pObj->r_Verb( script, pSrc );
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
			pszKey ++;
			CObjBase * pObj = static_cast<CObjBase*>(static_cast<CGrayUID>(Exp_GetSingle(pszKey)).ObjFind());
			if (!pObj)
				m_pO1 = NULL;	// no pObj = cleaning argo
			else
				m_pO1 = pObj;
			return( true );
		}
	}
	else
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
		case AGC_O:
			{
				LPCTSTR pszTemp = s.GetKey() + strlen(sm_szLoadKeys[AGC_O]);
				if ( *pszTemp == '.' )
				{
					++pszTemp;
					if ( !m_pO1 )
						return false;

					CScript script( pszTemp, s.GetArgStr() );
					return m_pO1->r_Verb( script, pSrc );
				}
			} return false;
		case AGC_TRY:
		case AGC_TRYSRV:
			{
				CScript try_script( s.GetArgStr() );
				if ( r_Verb( try_script, pSrc ) )
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

bool CScriptTriggerArgs::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CScriptTriggerArgs::r_LoadVal");
	UNREFERENCED_PARAMETER(s);
	return false;
}

bool CScriptTriggerArgs::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CScriptTriggerArgs::r_WriteVal");
	EXC_TRY("WriteVal");
	if ( IsSetEF( EF_Intrinsic_Locals ) )
	{
		EXC_SET("intrinsic");
		CVarDefCont *	pVar	= m_VarsLocal.GetKey( pszKey );
		
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
		return( true );
	}

	if ( !strnicmp( "FLOAT.", pszKey, 6 ) )
	{
		EXC_SET("float");
		pszKey += 6;
		sVal = m_VarsFloat.Get( pszKey );
		return( true );
	}
	else if ( !strnicmp(pszKey, "ARGV", 4) )
	{
		EXC_SET("argv");
		pszKey += 4;
		SKIP_SEPARATORS(pszKey);
		
		size_t iQty = m_v.GetCount();
		if ( iQty <= 0 )
		{
			// PARSE IT HERE
			TCHAR * pszArg = const_cast<TCHAR *>(m_s1_raw.GetPtr());
			TCHAR * s = pszArg;
			bool fQuotes = false;
			bool fInerQuotes = false;
			while ( *s )
			{
				// ignore leading spaces
				if ( IsSpace(*s ) )
				{
					++s;
					continue;
				}

				// add empty arguments if they are provided
				if ( (*s == ',') && (!fQuotes))
				{
					m_v.Add( '\0' );
					++s;
					continue;
				}
				
				// check to see if the argument is quoted (incase it contains commas)
				if ( *s == '"' )
				{
					++s;
					fQuotes = true;
					fInerQuotes = false;
				}

				pszArg	= s;	// arg starts here
				++s;

				while (*s)
				{
					if (( *s == '"' ) && ( fQuotes ))	{	*s	= '\0';	fQuotes = false;	}
					else if (( *s == '"' ))	{	fInerQuotes = !fInerQuotes;	}
					/*if ( *s == '"' )
					{
						if ( fQuotes )	{	*s	= '\0';	fQuotes = false;	break;	}
						*s = '\0';
						++s;
						fQuotes	= true;	// maintain
						break;
					}*/
					if ( !fQuotes && !fInerQuotes && (*s == ',') )
					{ *s = '\0'; s++; break; }
					++s;
				}
				m_v.Add( pszArg );
			}
			iQty = m_v.GetCount();
		}	
		
		if ( *pszKey == '\0' )
		{
			sVal.FormatVal(static_cast<long>(iQty));
			return( true );
		}

		size_t iNum = static_cast<size_t>(Exp_GetSingle(pszKey));
		SKIP_SEPARATORS(pszKey);
		if ( !m_v.IsValidIndex(iNum) )
		{
			sVal = "";
			return true;
		}
		sVal = m_v.GetAt(iNum);
		return true;
	}

	EXC_SET("generic");
	int index = FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );
	switch (index)
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
	ADDTOCALLSTACK("CScriptObj::r_GetRef");
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
		CGrayUID uid = static_cast<CGrayUID>(Exp_GetVal(pszKey));
		SKIP_SEPARATORS(pszKey);
		pRef = uid.ObjFind();
		return true;
	}
	else if ( ! strnicmp( pszKey, "OBJ.", 4 ))
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
	else if ( IsSetOF( OF_FileCommands ) && !strnicmp(pszKey, "FILE.", 5) )
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

enum SSC_TYPE
{
	#define ADD(a,b) SSC_##a,
	#include "../tables/CScriptObj_functions.tbl"
	#undef ADD
	SSC_QTY
};

LPCTSTR const CScriptObj::sm_szLoadKeys[SSC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CScriptObj_functions.tbl"
	#undef ADD
	NULL
};

bool CScriptObj::r_Call( LPCTSTR pszFunction, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * psVal, TRIGRET_TYPE * piRet )
{
	ADDTOCALLSTACK("CScriptObj::r_Call");
	GETNONWHITESPACE( pszFunction );
	size_t index;
	{
		int	iCompareRes	= -1;
		index = g_Cfg.m_Functions.FindKeyNear( pszFunction, iCompareRes, true );
		if ( iCompareRes != 0 )
			index = g_Cfg.m_Functions.BadIndex();
	}

	if ( index == g_Cfg.m_Functions.BadIndex() )
		return false;

	CResourceNamed *pFunction = static_cast<CResourceNamed *>(g_Cfg.m_Functions[index]);
	ASSERT(pFunction);
	CResourceLock sFunction;
	if ( pFunction->ResourceLock(sFunction) )
	{
		TScriptProfiler::TScriptProfilerFunction *pFun = NULL;
		ULONGLONG llTicksStart, llTicksEnd;

		//	If functions profiler is on, search this function record and get pointer to it
		//	if not, create the corresponding record
		if ( IsSetEF(EF_Script_Profiler) )
		{
			char	*pName = Str_GetTemp();
			char	*pSpace;

			//	lowercase for speed, and strip arguments
			strcpy(pName, pszFunction);
			if ( (pSpace = strchr(pName, ' ')) != NULL ) *pSpace = 0;
			_strlwr(pName);

			if ( g_profiler.initstate != 0xf1 )	// it is not initalised
			{
				memset(&g_profiler, 0, sizeof(g_profiler));
				g_profiler.initstate = static_cast<BYTE>(0xf1); // ''
			}
			for ( pFun = g_profiler.FunctionsHead; pFun != NULL; pFun = pFun->next )
			{
				if ( !strcmp(pFun->name, pName) ) break;
			}

			// first time function called. so create a record for it
			if ( pFun == NULL )
			{
				pFun = new TScriptProfiler::TScriptProfilerFunction;
				memset(pFun, 0, sizeof(TScriptProfiler::TScriptProfilerFunction));
				strcpy(pFun->name, pName);
				if ( g_profiler.FunctionsTail )
					g_profiler.FunctionsTail->next = pFun;
				else
					g_profiler.FunctionsHead = pFun;
				g_profiler.FunctionsTail = pFun;
			}

			//	prepare the informational block
			pFun->called++;
			g_profiler.called++;
			TIME_PROFILE_START;
		}

		TRIGRET_TYPE iRet = OnTriggerRun(sFunction, TRIGRUN_SECTION_TRUE, pSrc, pArgs, psVal);

		if ( IsSetEF(EF_Script_Profiler) )
		{
			//	update the time call information
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
			*piRet	= iRet;
	}
	return( true );
}


bool CScriptObj::r_LoadVal( CScript & s )
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

	// ignore these.
	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1);
	if ( index < 0 )
	{
		DEBUG_ERR(("Undefined keyword '%s'\n", s.GetKey()));
		return false;
	}

	switch ( index )
	{
		case SSC_VAR:
			{
				bool fQuoted = false;
				g_Exp.m_VarGlobals.SetStr( pszKey+4, fQuoted, s.GetArgStr( &fQuoted ), false );
				return( true );
			}
		case  SSC_VAR0:
			{
				bool fQuoted = false;
				g_Exp.m_VarGlobals.SetStr( pszKey+5, fQuoted, s.GetArgStr( &fQuoted ), true );
				return( true );
			}

		case  SSC_LIST:
			{
				if ( !g_Exp.m_ListGlobals.r_LoadVal(pszKey + 5, s) )
					DEBUG_ERR(("Unable to process command '%s %s'\n", pszKey, s.GetArgRaw()));

				return true;
			}

		case SSC_DEFMSG:
			{
				long	l;
				pszKey += 7;
				for ( l = 0; l < DEFMSG_QTY; ++l )
				{
					if ( !strcmpi(pszKey, g_Exp.sm_szMsgNames[l]) )
					{
						bool fQuoted = false;
						TCHAR * args = s.GetArgStr(&fQuoted);
						strcpy(g_Exp.sm_szMessages[l], args);
						return(true);
					}
				}
				g_Log.Event(LOGM_INIT|LOGL_ERROR, "Setting not used message override named '%s'\n", pszKey);
				return(false);
			}
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

static void StringFunction( int iFunc, LPCTSTR pszKey, CGString &sVal )
{
	GETNONWHITESPACE(pszKey);
	if ( *pszKey == '(' )
		++pszKey;

	TCHAR * ppCmd[4];
	size_t iCount = Str_ParseCmds( const_cast<TCHAR *>(pszKey), ppCmd, COUNTOF(ppCmd), ")" );
	if ( iCount <= 0 )
	{
		DEBUG_ERR(( "Bad string function usage. missing )\n" ));
		return;
	}

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
	ADDTOCALLSTACK("CScriptObj::r_WriteVal");
	EXC_TRY("WriteVal");
	CObjBase * pObj;
	CScriptObj * pRef = NULL;
	bool fGetRef = r_GetRef( pszKey, pRef );

	if ( !strnicmp(pszKey, "GetRefType", 10) )
	{
		CScriptObj * pTmpRef;
		if ( pRef )
			pTmpRef = pRef;
		else
			pTmpRef = pSrc->GetChar();

		if ( pTmpRef == &g_Serv )
			sVal.FormatHex( 0x01 );
		else if ( pTmpRef == &(g_Serv.fhFile) )
			sVal.FormatHex( 0x02 );
		else if (( pTmpRef == &(g_Serv.m_hdb) ) || dynamic_cast<CDataBase*>(pTmpRef) )
			sVal.FormatHex( 0x00008 );
		else if ( dynamic_cast<CResourceDef*>(pTmpRef) )
			sVal.FormatHex( 0x00010 );
		else if ( dynamic_cast<CResourceBase*>(pTmpRef) )
			sVal.FormatHex( 0x00020 );
		else if ( dynamic_cast<CScriptTriggerArgs*>(pTmpRef) )
			sVal.FormatHex( 0x00040 );
		else if ( dynamic_cast<CFileObj*>(pTmpRef) )
			sVal.FormatHex( 0x00080 );
		else if ( dynamic_cast<CFileObjContainer*>(pTmpRef) )
			sVal.FormatHex( 0x00100 );
		else if ( dynamic_cast<CAccount*>(pTmpRef) )
			sVal.FormatHex( 0x00200 );
		//else if ( dynamic_cast<CPartyDef*>(pTmpRef) )
		//	sVal.FormatHex( 0x00400 );
		else if (( pTmpRef == &(g_Serv.m_hldb) ) || dynamic_cast<CSQLite*>(pTmpRef) )
			sVal.FormatHex( 0x00400 );
		else if ( dynamic_cast<CStoneMember*>(pTmpRef) )
			sVal.FormatHex( 0x00800 );
		else if ( dynamic_cast<CServerDef*>(pTmpRef) )
			sVal.FormatHex( 0x01000 );
		else if ( dynamic_cast<CSector*>(pTmpRef) )
			sVal.FormatHex( 0x02000 );
		else if ( dynamic_cast<CWorld*>(pTmpRef) )
			sVal.FormatHex( 0x04000 );
		else if ( dynamic_cast<CGMPage*>(pTmpRef) )
			sVal.FormatHex( 0x08000 );
		else if ( dynamic_cast<CClient*>(pTmpRef) )
			sVal.FormatHex( 0x10000 );
		else if ( (pObj = dynamic_cast<CObjBase*>(pTmpRef)) != NULL )
		{
			if ( dynamic_cast<CChar*>(pObj) )
				sVal.FormatHex( 0x40000 );
			else if ( dynamic_cast<CItem*>(pObj) )
				sVal.FormatHex( 0x80000 );
			else
				sVal.FormatHex( 0x20000 );
		}
		else
			sVal.FormatHex( 0x01 );
		return true;
	}

	if ( fGetRef )
	{
		if ( pRef == NULL )	// good command but bad link.
		{
			sVal = "0";
			return true;
		}
		if ( pszKey[0] == '\0' )	// we where just testing the ref.
		{
			pObj = dynamic_cast<CObjBase*>(pRef);
			if ( pObj )
				sVal.FormatHex( static_cast<DWORD>(pObj->GetUID()) );
			else
				sVal.FormatVal( 1 );
			return( true );
		}
		return pRef->r_WriteVal( pszKey, sVal, pSrc );
	}

	int index = FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );
	if ( index < 0 )
	{
		// <dSOMEVAL> same as <eval <SOMEVAL>> to get dec from the val
		if (( *pszKey == 'd' ) || ( *pszKey == 'D' ))
		{
			LPCTSTR arg = pszKey + 1;
			if ( r_WriteVal(arg, sVal, pSrc) )
			{
				if ( *sVal != '-' )
					sVal.FormatLLVal(ahextoi64(sVal));

				return true;
			}
		}
		// <r>, <r15>, <r3,15> are shortcuts to rand(), rand(15) and rand(3,15)
		else if (( *pszKey == 'r' ) || ( *pszKey == 'R' ))
		{
			pszKey += 1;
			if ( *pszKey && (( *pszKey < '0' ) || ( *pszKey > '9' )) && *pszKey != '-' )
				goto badcmd;

			INT64	min = 1000, max = LLONG_MIN;

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

			if ( max == LLONG_MIN )
			{
				max = min - 1;
				min = 0;
			}

			if ( min >= max )
				sVal.FormatLLVal(min);
			else
				sVal.FormatLLVal(Calc_GetRandLLVal2(min, max));

			return true;
		}
badcmd:
		return false;	// Bad command.
	}

	pszKey += strlen( sm_szLoadKeys[index] );
	SKIP_SEPARATORS(pszKey);
	bool	fZero	= false;

	switch ( index )
	{
		case SSC_BETWEEN:
		case SSC_BETWEEN2:
			{
				INT64	iMin = Exp_GetLLVal(pszKey);
				SKIP_ARGSEP(pszKey);
				INT64	iMax = Exp_GetLLVal(pszKey);
				SKIP_ARGSEP(pszKey);
				INT64 iCurrent = Exp_GetLLVal(pszKey);
				SKIP_ARGSEP(pszKey);
				INT64 iAbsMax = Exp_GetLLVal(pszKey);
				SKIP_ARGSEP(pszKey);
				if ( index == SSC_BETWEEN2 )
				{
					iCurrent = iAbsMax - iCurrent;
				}
				
				if (( iMin >= iMax ) || ( iAbsMax <= 0 ) || ( iCurrent <= 0 ) )
					sVal.FormatLLVal(iMin);
				else if ( iCurrent >= iAbsMax )
					sVal.FormatLLVal(iMax);
				else
					sVal.FormatLLVal((iCurrent * (iMax - iMin))/iAbsMax + iMin);
			} break;

		case SSC_LISTCOL:
			// Set the alternating color.
			sVal = (CWebPageDef::sm_iListIndex&1) ? "bgcolor=\"#E8E8E8\"" : "";
			return( true );
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
				pObj = dynamic_cast <CObjBase*> (pRef);	// if it can be converted .
				sVal.FormatHex( pObj ? static_cast<DWORD>(pObj->GetUID()) : 0 );
				return true;
			}
			return pRef->r_WriteVal( pszKey, sVal, pSrc );
		case SSC_VAR0:
			fZero	= true;
		case SSC_VAR:
			// "VAR." = get/set a system wide variable.
			{
				CVarDefCont * pVar = g_Exp.m_VarGlobals.GetKey(pszKey);
				if ( pVar )
					sVal	= pVar->GetValStr();
				else if ( fZero )
					sVal	= "0";
			}
			return true;
		case SSC_DEFLIST:
			{
				g_Exp.m_ListInternals.r_Write(pSrc, pszKey, sVal);	
			}
			return true;
		case SSC_LIST:
			{
				g_Exp.m_ListGlobals.r_Write(pSrc, pszKey, sVal);
			}
			return true;
		case SSC_DEF0:
			fZero	= true;
		case SSC_DEF:
			{
				CVarDefCont * pVar = g_Exp.m_VarDefs.GetKey(pszKey);
				if ( pVar )
					sVal	= pVar->GetValStr();
				else if ( fZero )
					sVal	= "0";
			}
			return( true );
		case SSC_DEFMSG:
			sVal = g_Cfg.GetDefaultMsg(pszKey);
			return(true);
		case SSC_EVAL:
			sVal.FormatLLVal( Exp_GetLLVal( pszKey ));
			return( true );
		case SSC_UVAL:
			sVal.FormatULLVal(static_cast<unsigned long long>(Exp_GetLLVal(pszKey)));
			return( true );
		case SSC_FVAL:
			{
				INT64 iVal = Exp_GetLLVal(pszKey);
				sVal.Format( "%s%lld.%lld", (iVal >= 0) ? "" : "-", llabs(iVal/10), llabs(iVal%10) );
				return true;
			}
		case SSC_HVAL:
			sVal.FormatLLHex(Exp_GetLLVal(pszKey));
			return( true );
//FLOAT STUFF BEGINS HERE
		case SSC_FEVAL: //Float EVAL
			sVal.FormatVal( ATOI( pszKey ) );
			break;
		case SSC_FHVAL: //Float HVAL
			sVal.FormatHex( ATOI( pszKey ) );
			break;
		case SSC_FLOATVAL: //Float math
			{
				sVal = CVarFloat::FloatMath( pszKey );
				break;
			}
//FLOAT STUFF ENDS HERE
		case SSC_QVAL:
			{	// Do a switch ? type statement <QVAL conditional ? option1 : option2>
				TCHAR * ppCmds[3];
				ppCmds[0] = const_cast<TCHAR*>(pszKey);
				Str_Parse( ppCmds[0], &(ppCmds[1]), "?" );
				Str_Parse( ppCmds[1], &(ppCmds[2]), ":" );
				sVal = ppCmds[ Exp_GetVal( ppCmds[0] ) ? 1 : 2 ];
				if ( sVal.IsEmpty())
					sVal = "";
			}
			return( true );
		case SSC_ISBIT:
		case SSC_SETBIT:
		case SSC_CLRBIT:
			{
				INT64 val = Exp_GetLLVal(pszKey);
				SKIP_ARGSEP(pszKey);
				INT64 bit = Exp_GetLLVal(pszKey);

				if ( index == SSC_ISBIT )
					sVal.FormatLLVal(val & (static_cast<INT64>(1) << bit));
				else if ( index == SSC_SETBIT )
					sVal.FormatLLVal(val | (static_cast<INT64>(1) << bit));
				else
					sVal.FormatLLVal(val & ~(static_cast<INT64>(1) << bit));
				break;
			}
		case SSC_ISEMPTY:
			sVal.FormatVal( IsStrEmpty( pszKey ) );
			return true;
		case SSC_ISNUM:
			GETNONWHITESPACE( pszKey );
			if (*pszKey == '-')
				pszKey++;
			sVal.FormatVal( IsStrNumeric( pszKey ) );
			return true;
		case SSC_StrPos:
			{
				GETNONWHITESPACE( pszKey );
				INT64 iPos = Exp_GetVal( pszKey );
				TCHAR ch;
				if ( IsDigit( *pszKey) && IsDigit( *(pszKey+1) ) )
					ch = static_cast<TCHAR>(Exp_GetVal(pszKey));
				else
				{
					ch = *pszKey;
					++pszKey;
				}
				
				GETNONWHITESPACE( pszKey );
				INT64	iLen	= strlen( pszKey );
				if ( iPos < 0 )
					iPos	= iLen + iPos;
				if ( iPos < 0 )
					iPos	= 0;
				else if ( iPos > iLen )
					iPos	= iLen;

				TCHAR *	pszPos	= const_cast<TCHAR*>(strchr( pszKey + iPos, ch ));
				if ( !pszPos )
					sVal.FormatVal( -1 );
				else
					sVal.FormatVal(static_cast<long>( pszPos - pszKey ) );
			}
			return true;
		case SSC_StrSub:
			{
				TCHAR * ppArgs[3];
				size_t iQty = Str_ParseCmds(const_cast<TCHAR *>(pszKey), ppArgs, COUNTOF(ppArgs));
				if ( iQty < 3 )
					return false;

				INT64	iPos = Exp_GetVal( ppArgs[0] );
				INT64	iCnt = Exp_GetVal( ppArgs[1] );
				if ( iCnt < 0 )
					return false;

				INT64	iLen = strlen( ppArgs[2] );
				if ( iPos < 0 ) iPos += iLen;
				if ( iPos > iLen || iPos < 0 ) iPos = 0;

				if ( iPos + iCnt > iLen || iCnt == 0 )
					iCnt = iLen - iPos;

				TCHAR *buf = Str_GetTemp();
				strncpy( buf, ppArgs[2] + iPos, static_cast<size_t>(iCnt) );
				buf[iCnt] = '\0';

				if ( g_Cfg.m_wDebugFlags & DEBUGF_SCRIPTS )
					g_Log.EventDebug("SCRIPT: strsub(%lld,%lld,'%s') -> '%s'\n", iPos, iCnt, ppArgs[2], buf);

				sVal = buf;
			}
			return true;
		case SSC_StrArg:
			{
				TCHAR * buf = Str_GetTemp();
				GETNONWHITESPACE( pszKey );
				if ( *pszKey == '"' )
					++pszKey;

				size_t len = 0;
				while ( *pszKey && !IsSpace( *pszKey ) && *pszKey != ',' )
				{
					buf[len] = *pszKey;
					++pszKey;
					++len;
				}
				buf[len]= '\0';
				sVal = buf;
			}
			return true;
		case SSC_StrEat:
			{
				GETNONWHITESPACE( pszKey );
				while ( *pszKey && !IsSpace( *pszKey ) && *pszKey != ',' )
					++pszKey;
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
				TCHAR	*buf = Str_GetTemp();
				REMOVE_QUOTES( pszKey );
				sVal.FormatLLHex( *pszKey );
				strcpy( buf, sVal );
				while ( *(++pszKey) )
				{
					if ( *pszKey == '"' ) break;
					sVal.FormatLLHex(*pszKey);
					strcat( buf, " " );
					strcat( buf, sVal );
				}
				sVal	= buf;
			}
			return true;
		case SSC_ASCPAD:
			{
				TCHAR * ppArgs[2];
				size_t iQty = Str_ParseCmds(const_cast<TCHAR *>(pszKey), ppArgs, COUNTOF(ppArgs));
				if ( iQty < 2 )
					return false;

				INT64	iPad = Exp_GetVal( ppArgs[0] );
				if ( iPad < 0 )
					return false;
				TCHAR	*buf = Str_GetTemp();
				REMOVE_QUOTES( ppArgs[1] );
				sVal.FormatLLHex(*ppArgs[1]);
				strcpy( buf, sVal );
				while ( --iPad )
				{
					if ( *ppArgs[1] == '"' ) continue;
					if ( *(ppArgs[1]) )
					{
						++ppArgs[1];
						sVal.FormatLLHex(*ppArgs[1]);
					}
					else
						sVal.FormatLLHex('\0');

					strcat( buf, " " );
					strcat( buf, sVal );
				}
				sVal	= buf;
			}
			return true;
		case SSC_SYSCMD:
		case SSC_SYSSPAWN:
			{
				if ( !IsSetOF(OF_FileCommands) )
					return false;

				GETNONWHITESPACE(pszKey);
				TCHAR	*buf = Str_GetTemp();
				TCHAR	*Arg_ppCmd[10];		// limit to 9 arguments
				strcpy(buf, pszKey);
				size_t iQty = Str_ParseCmds(buf, Arg_ppCmd, COUNTOF(Arg_ppCmd));
				if ( iQty < 1 )
					return false;
				
				bool bWait = (index == SSC_SYSCMD);

#ifdef _WIN32
				_spawnl( bWait ? _P_WAIT : _P_NOWAIT,
					Arg_ppCmd[0], Arg_ppCmd[0], Arg_ppCmd[1],
					Arg_ppCmd[2], Arg_ppCmd[3], Arg_ppCmd[4],
					Arg_ppCmd[5], Arg_ppCmd[6], Arg_ppCmd[7],
					Arg_ppCmd[8], Arg_ppCmd[9], NULL );
#else
				// I think fork will cause problems.. we'll see.. if yes new thread + execlp is required.
				int child_pid = vfork();
				if ( child_pid < 0 )
				{
					g_Log.EventError("SYSSPAWN failed when executing %s.\n", pszKey);
					return( false );
				}
				else if ( child_pid == 0 )
				{
					//Don't touch this :P
					execlp( Arg_ppCmd[0], Arg_ppCmd[0], Arg_ppCmd[1], Arg_ppCmd[2],
										Arg_ppCmd[3], Arg_ppCmd[4], Arg_ppCmd[5], Arg_ppCmd[6],
										Arg_ppCmd[7], Arg_ppCmd[8], Arg_ppCmd[9], NULL );
					
					g_Log.EventError("SYSSPAWN failed with error %d (\"%s\") when executing %s.\n", errno, strerror(errno), pszKey);
					raise(SIGKILL);
					g_Log.EventError("Failed errorhandling in SYSSPAWN. Server is UNSTABLE.\n");
					while(true) {} // do NOT leave here until the process receives SIGKILL otherwise it will free up resources
								   // it inherited from the main process, which pretty will fuck everything up. Normally this point should never be reached.
				}
				else if(bWait) // parent process here (do we have to wait?)
				{
					int status;
					do
					{
						if(waitpid(child_pid, &status, 0))
							break;
					} while (!WIFSIGNALED(status) && !WIFEXITED(status));
					sVal.FormatLLHex(WEXITSTATUS(status));
				}
#endif
				g_Log.EventDebug("Process execution finished\n");
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
						++p;
					*p = 0;
				}

				const char *p = pszKey + strlen(separators) + 1;
				sVal = "";
				if (( p > pszKey ) && *p )		//	we have list of accessible separators 
				{
					TCHAR *ppCmd[255];
					TCHAR * z = Str_GetTemp();
					strcpy(z, p);
					size_t count = Str_ParseCmds(z, ppCmd, COUNTOF(ppCmd), separators); 
					if (count > 0)
					{
						sVal.Add(ppCmd[0]);
						for (size_t i = 1; i < count; ++i)
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
				char digest[33];
				GETNONWHITESPACE(pszKey);

				CMD5::fastDigest( digest, pszKey );
				sVal.Format("%s", digest);
			} return true;
		case SSC_MULDIV:
			{
				INT64	iNum	= Exp_GetLLVal( pszKey );
				SKIP_ARGSEP(pszKey);
				INT64	iMul	= Exp_GetLLVal( pszKey );
				SKIP_ARGSEP(pszKey);
				INT64	iDiv	= Exp_GetLLVal( pszKey );
				INT64 iRes = 0;

				if ( iDiv == 0 )
					g_Log.EventWarn("MULDIV(%lld,%lld,%lld) -> Dividing by '0'\n", iNum, iMul, iDiv);
				else
					iRes = IMULDIV(iNum,iMul,iDiv);

				if ( g_Cfg.m_wDebugFlags & DEBUGF_SCRIPTS )
					g_Log.EventDebug("SCRIPT: muldiv(%lld,%lld,%lld) -> %lld\n", iNum, iMul, iDiv, iRes);

				sVal.FormatLLVal(iRes);
			}
			return true;

		case SSC_StrRegexNew:
			{
				size_t iLenString = Exp_GetVal( pszKey );
				TCHAR * sToMatch = Str_GetTemp();
				if ( iLenString > 0 )
				{
					SKIP_ARGSEP(pszKey);
					strcpylen(sToMatch, pszKey, iLenString + 1);
					pszKey += iLenString;
				}

				SKIP_ARGSEP(pszKey);
				TCHAR * tLastError = Str_GetTemp();
				int iDataResult = Str_RegExMatch( pszKey, sToMatch, tLastError );
				sVal.FormatVal(iDataResult);
				
				if ( iDataResult == -1 )
				{
					DEBUG_ERR(( "STRREGEX bad function usage. Error: %s\n", tLastError ));
				}
			} return true;

		default:
			StringFunction( index, pszKey, sVal );
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
	SSV_QTY
};

LPCTSTR const CScriptObj::sm_szVerbKeys[SSV_QTY+1] =
{
	"NEW",
	"NEWDUPE",
	"NEWITEM",
	"NEWNPC",
	"OBJ",
	"SHOW",
	NULL
};


bool CScriptObj::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CScriptObj::r_Verb");
	EXC_TRY("Verb");
	int	index;
	LPCTSTR pszKey = s.GetKey();
	
	ASSERT( pSrc );
	CScriptObj * pRef = NULL;
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
				CGrayUID uid(s.GetArgVal());
				CObjBase	*pObj = uid.ObjFind();
				if (pObj == NULL)
				{
					g_World.m_uidNew = 0;
					return false;
				}

				g_World.m_uidNew = uid;
				CScript script("DUPE");
				bool bRc = pObj->r_Verb(script, pSrc);

				if (this != &g_Serv)
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
				TCHAR * ppCmd[4];
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
					pItem->SetAmount(static_cast<WORD>(Exp_GetVal(ppCmd[1])));
				
				if ( ppCmd[2] )
				{
					CGrayUID uidEquipper = static_cast<CGrayUID>(Exp_GetVal(ppCmd[2]));
					bool bTriggerEquip = ppCmd[3] != NULL ? (Exp_GetVal(ppCmd[3]) != 0) : false;

					if ( !bTriggerEquip || uidEquipper.IsItem() )
						pItem->LoadSetContainer(uidEquipper, LAYER_NONE);
					else
					{
						if ( bTriggerEquip )
						{
							CChar * pCharEquipper = uidEquipper.CharFind();
							if ( pCharEquipper != NULL )
								pCharEquipper->ItemEquip(pItem);
						}
					}
				}

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
				CREID_TYPE id = static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType(RES_CHARDEF, s.GetArgRaw()));
				CChar * pChar = CChar::CreateNPC(id);
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
			}
			break;

		case SSV_SHOW:
			{
				CGString sVal;
				if ( ! r_WriteVal( s.GetArgStr(), sVal, pSrc ))
					return( false );
				TCHAR * pszMsg = Str_GetTemp();
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

bool CScriptObj::r_Load( CScript & s )
{
	ADDTOCALLSTACK("CScriptObj::r_Load");
	while ( s.ReadKeyParse())
	{
		if ( s.IsKeyHead( "ON", 2 ))	// trigger scripting marks the end
			break;
		r_LoadVal(s);
	}
	return( true );
}

size_t CScriptObj::ParseText( TCHAR * pszResponse, CTextConsole * pSrc, int iFlags, CScriptTriggerArgs * pArgs )
{
	ADDTOCALLSTACK("CScriptObj::ParseText");
	// Take in a line of text that may have fields that can be replaced with operators here.
	// ARGS:
	// iFlags = 2=Allow recusive bracket count. 1=use HTML %% as the delimiters.
	// NOTE:
	//  html will have opening <script language="SPHERE_FILE"> and then closing </script>
	// RETURN:
	//  New length of the string.
	//
	// Parsing flags
	LPCTSTR pszKey; // temporary, set below
	bool fRes;

	static int sm_iReentrant = 0;
	static bool sm_fBrackets = false;	// allowed to span multi lines.

	//***Qval Fix***
	bool bQvalCondition = false;
	TCHAR chQval = '?';

	if ((iFlags & 2) == 0)
	{
		sm_fBrackets = false;
	}

	size_t iBegin = 0;
	TCHAR chBegin = '<';
	TCHAR chEnd = '>';

	bool fHTML = (iFlags & 1) != 0;
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

		if ( ! sm_fBrackets )	// not in brackets
		{
			if ( ch == chBegin )	// found the start !
			{
				 if ( !( isalnum( pszResponse[i + 1] ) || pszResponse[i + 1] == '<' ) ) // ignore this.
					continue;
				iBegin = i;
				sm_fBrackets = true;
			}
			continue;
		}

		if ( ch == '<' )	// recursive brackets
		{			
			if ( !( isalnum( pszResponse[i + 1] ) || pszResponse[i + 1] == '<' ) ) // ignore this.
				continue;
			
			if (sm_iReentrant > 32 )
			{
				EXC_SET("reentrant limit");
				ASSERT( sm_iReentrant < 32 );
			}
			++sm_iReentrant;
			sm_fBrackets = false;
			size_t ilen = ParseText( pszResponse + i, pSrc, 2, pArgs );
			sm_fBrackets = true;
			--sm_iReentrant;
			i += ilen;
			continue;
		}
		//***Qval Fix***
		if ( ch == chQval )
		{
			if ( !strnicmp( static_cast<LPCTSTR>(pszResponse) + iBegin + 1, "QVAL", 4 ) )
				bQvalCondition = true;
		}

		if ( ch == chEnd )
		{
			if ( !strnicmp( static_cast<LPCTSTR>(pszResponse) + iBegin + 1, "QVAL", 4 ) && !bQvalCondition)
				continue;
			//***Qval Fix End***
			sm_fBrackets = false;
			pszResponse[i] = '\0';

			CGString sVal;
			pszKey = static_cast<LPCTSTR>(pszResponse) + iBegin + 1;

			EXC_SET("writeval");
			fRes = r_WriteVal( pszKey, sVal, pSrc );
			if ( fRes == false )
			{
				EXC_SET("writeval");
				if ( pArgs != NULL && pArgs->r_WriteVal( pszKey, sVal, pSrc ) )
					fRes = true;
			}

			if ( fRes == false )
			{
				DEBUG_ERR(( "Can't resolve <%s>\n", pszKey ));
				// Just in case this really is a <= operator ?
				pszResponse[i] = chEnd;
			}

			if ( sVal.IsEmpty() && fHTML )
			{
				sVal = "&nbsp";
			}

			int len = sVal.GetLength();

			EXC_SET("mem shifting");

			memmove( pszResponse + iBegin + len, pszResponse + i + 1, strlen( pszResponse + i + 1 ) + 1 );
			memcpy( pszResponse + iBegin, static_cast<LPCTSTR>(sVal), len );
			i = iBegin + len - 1;

			if ((iFlags & 2) != 0) // just do this one then bail out.
				return i;
		}
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("response '%s' source addr '0%p' flags '%d' args '%p'\n", pszResponse, static_cast<void *>(pSrc), iFlags, static_cast<void *>(pArgs));
	EXC_DEBUG_END;
	return i;
}


TRIGRET_TYPE CScriptObj::OnTriggerForLoop( CScript &s, int iType, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * pResult )
{
	ADDTOCALLSTACK("CScriptObj::OnTriggerForLoop");
	// loop from start here to the ENDFOR
	// See WebPageScriptList for dealing with Arrays.

	CScriptLineContext StartContext = s.GetContext();
	CScriptLineContext EndContext = StartContext;
	int LoopsMade = 0;

	if ( iType & 8 )		// WHILE
	{
		TCHAR * pszCond;
		CGString pszOrig;
		TemporaryString pszTemp;
		int iWhile	= 0;

		pszOrig.Copy( s.GetArgStr() );
		for (;;)
		{
			++LoopsMade;
			if ( g_Cfg.m_iMaxLoopTimes && ( LoopsMade >= g_Cfg.m_iMaxLoopTimes ))
				goto toomanyloops;

			pArgs->m_VarsLocal.SetNum( "_WHILE", iWhile, false );
			++iWhile;
			strcpy( pszTemp, pszOrig.GetPtr() );
			pszCond	= pszTemp;
			ParseText( pszCond, pSrc, 0, pArgs );
			if ( !Exp_GetVal( pszCond ) )
				break;
			TRIGRET_TYPE iRet = OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult );
			if ( iRet == TRIGRET_BREAK )
			{
				EndContext = StartContext;
				break;
			}
			if (( iRet != TRIGRET_ENDIF ) && ( iRet != TRIGRET_CONTINUE ))
				return( iRet );
			if ( iRet == TRIGRET_CONTINUE )
				EndContext = StartContext;
			else
				EndContext = s.GetContext();
			s.SeekContext( StartContext );
		}
	}
	else
		ParseText( s.GetArgStr(), pSrc, 0, pArgs );


	
	if ( iType & 4 )		// FOR
	{ 
		int fCountDown = FALSE;
		int iMin = 0;
		int iMax = 0;
		int i;
		TCHAR * ppArgs[3];
		size_t iQty = Str_ParseCmds( s.GetArgStr(), ppArgs, COUNTOF(ppArgs), ", " );
		CGString sLoopVar = "_FOR";
		
		switch( iQty )
		{
		case 1:		// FOR x
			iMin = 1;
			iMax = Exp_GetSingle( ppArgs[0] );
			break;
		case 2:
			if ( IsDigit( *ppArgs[0] ) )
			{
				iMin = Exp_GetSingle( ppArgs[0] );
				iMax = Exp_GetSingle( ppArgs[1] );
			}
			else
			{
				iMin = 1;
				iMax = Exp_GetSingle( ppArgs[1] );
				sLoopVar = ppArgs[0];
			}
			break;
		case 3:
			sLoopVar = ppArgs[0];
			iMin = Exp_GetSingle( ppArgs[1] );
			iMax = Exp_GetSingle( ppArgs[2] );
			break;
		default:
			iMin = iMax = 1;
			break;
		}

		if ( iMin > iMax )
			fCountDown	= true;

		if ( fCountDown )
			for ( i = iMin; i >= iMax; --i )
			{
				++LoopsMade;
				if ( g_Cfg.m_iMaxLoopTimes && ( LoopsMade >= g_Cfg.m_iMaxLoopTimes ))
					goto toomanyloops;

				pArgs->m_VarsLocal.SetNum( sLoopVar, i, false );
				TRIGRET_TYPE iRet = OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult );
				if ( iRet == TRIGRET_BREAK )
				{
					EndContext = StartContext;
					break;
				}
				if (( iRet != TRIGRET_ENDIF ) && ( iRet != TRIGRET_CONTINUE ))
					return( iRet );

				if ( iRet == TRIGRET_CONTINUE )
					EndContext = StartContext;
				else
					EndContext = s.GetContext();
				s.SeekContext( StartContext );
			}
		else
			for ( i = iMin; i <= iMax; ++i )
			{
				++LoopsMade;
				if ( g_Cfg.m_iMaxLoopTimes && ( LoopsMade >= g_Cfg.m_iMaxLoopTimes ))
					goto toomanyloops;

				pArgs->m_VarsLocal.SetNum( sLoopVar, i, false );
				TRIGRET_TYPE iRet = OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult );
				if ( iRet == TRIGRET_BREAK )
				{
					EndContext = StartContext;
					break;
				} 
				if (( iRet != TRIGRET_ENDIF ) && ( iRet != TRIGRET_CONTINUE ))
					return( iRet );

				if ( iRet == TRIGRET_CONTINUE )
					EndContext = StartContext;
				else
					EndContext = s.GetContext();
				s.SeekContext( StartContext );
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
			DEBUG_ERR(( "FOR Loop trigger on non-world object '%s'\n", GetName()));
		}
		else
		{
			CObjBaseTemplate * pObjTop = pObj->GetTopLevelObj();
			CPointMap pt = pObjTop->GetTopPoint();
			if ( iType & 1 )		// FORITEM, FOROBJ
			{
				CWorldSearch AreaItems( pt, iDist );
				for (;;)
				{
					++LoopsMade;
					if ( g_Cfg.m_iMaxLoopTimes && ( LoopsMade >= g_Cfg.m_iMaxLoopTimes ))
						goto toomanyloops;

					CItem * pItem = AreaItems.GetItem();
					if ( pItem == NULL )
						break;
					TRIGRET_TYPE iRet = pItem->OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult );
					if ( iRet == TRIGRET_BREAK )
					{
						EndContext = StartContext;
						break;
					}
					if (( iRet != TRIGRET_ENDIF ) && ( iRet != TRIGRET_CONTINUE ))
						return( iRet );
					if ( iRet == TRIGRET_CONTINUE )
						EndContext = StartContext;
					else
						EndContext = s.GetContext();
					s.SeekContext( StartContext );
				}
			}
			if ( iType & 2 )		// FORCHAR, FOROBJ
			{
				CWorldSearch AreaChars( pt, iDist );
				AreaChars.SetAllShow((iType & 0x20) ? true : false);
				for (;;)
				{
					++LoopsMade;
					if ( g_Cfg.m_iMaxLoopTimes && ( LoopsMade >= g_Cfg.m_iMaxLoopTimes ))
						goto toomanyloops;

					CChar * pChar = AreaChars.GetChar();
					if ( !pChar )
						break;
					if ( (iType & 0x10) && !pChar->m_pClient )	// FORCLIENTS
						continue;
					if ( (iType & 0x20) && !pChar->m_pPlayer )	// FORPLAYERS
						continue;
					TRIGRET_TYPE iRet = pChar->OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult );
					if ( iRet == TRIGRET_BREAK )
					{
						EndContext = StartContext;
						break;
					}
					if (( iRet != TRIGRET_ENDIF ) && ( iRet != TRIGRET_CONTINUE ))
						return( iRet );
					if ( iRet == TRIGRET_CONTINUE )
						EndContext = StartContext;
					else
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
			rid = g_Cfg.ResourceGetID(RES_UNKNOWN, const_cast<LPCTSTR &>(static_cast<LPTSTR &>(ppArgs[0])));
		}
		else
		{
			const CObjBase * pObj = dynamic_cast <CObjBase*>(this);
			if ( pObj && pObj->Base_GetDef() )
			{
				rid = pObj->Base_GetDef()->GetResourceID();
			}
		}

		// No need to loop if there is no valid resource id
		if ( rid.IsValidUID() )
		{
			DWORD dwTotalInstances = 0; // Will acquire the correct value for this during the loop
			DWORD dwUID = 0;
			DWORD dwTotal = g_World.GetUIDCount();
			DWORD dwCount = dwTotal-1;
			DWORD dwFound = 0;

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
				++LoopsMade;
				if ( g_Cfg.m_iMaxLoopTimes && ( LoopsMade >= g_Cfg.m_iMaxLoopTimes ))
					goto toomanyloops;

				// Execute script on this object
				TRIGRET_TYPE iRet = pObj->OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult );
				if ( iRet == TRIGRET_BREAK )
				{
					EndContext = StartContext;
					break;
				}
 				if (( iRet != TRIGRET_ENDIF ) && ( iRet != TRIGRET_CONTINUE ))
					return( iRet );
				if ( iRet == TRIGRET_CONTINUE )
					EndContext = StartContext;
				else
					EndContext = s.GetContext();
				s.SeekContext( StartContext );

				// Acquire the total instances that exist for this item if we can
				if ( dwTotalInstances == 0 && pObj->Base_GetDef() != NULL )
					dwTotalInstances = pObj->Base_GetDef()->GetRefInstances();

				++dwFound;

				// If we know how many instances there are, abort the loop once we've found them all
				if ( (dwTotalInstances > 0) && (dwFound >= dwTotalInstances) )
					break;
			}
		}
	}

	if (iType & 0x100)	// FORTIMERF
	{
		TCHAR * ppArgs[1];

		if (Str_ParseCmds(s.GetArgStr(), ppArgs, COUNTOF(ppArgs), " \t,") >= 1)
		{			
			char funcname[1024];
			strcpy(funcname, ppArgs[0]);

			TRIGRET_TYPE iRet = g_World.m_TimedFunctions.Loop(funcname, LoopsMade, StartContext, EndContext, s, pSrc, pArgs, pResult);
			if ((iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE))
				return(iRet);
		}
	}

	if ( g_Cfg.m_iMaxLoopTimes )
	{
toomanyloops:
		if ( LoopsMade >= g_Cfg.m_iMaxLoopTimes )
			g_Log.EventError("Terminating loop cycle since it seems being dead-locked (%d iterations already passed)\n", LoopsMade);
	}

	if ( EndContext.m_lOffset <= StartContext.m_lOffset )
	{
		// just skip to the end.
		TRIGRET_TYPE iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
		if ( iRet != TRIGRET_ENDIF )
			return( iRet );
	}
	else
		s.SeekContext( EndContext );

	return( TRIGRET_ENDIF );
}

bool CScriptObj::OnTriggerFind( CScript & s, LPCTSTR pszTrigName )
{
	ADDTOCALLSTACK("CScriptObj::OnTriggerFind");
	while ( s.ReadKey(false) )
	{
		// Is it a trigger ?
		if (strnicmp(s.GetKey(), "ON", 2) != 0)
			continue;

		// Is it the right trigger ?
		s.ParseKeyLate();
		if (strcmpi(s.GetArgRaw(), pszTrigName) == 0)
			return true;
	}
	return false;
}

TRIGRET_TYPE CScriptObj::OnTriggerScript( CScript & s, LPCTSTR pszTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs )
{
	ADDTOCALLSTACK("CScriptObj::OnTriggerScript");
	// look for exact trigger matches
	if ( !OnTriggerFind(s, pszTrigName) )
		return TRIGRET_RET_DEFAULT;

	ProfileTask scriptsTask(PROFILE_SCRIPTS);

	TScriptProfiler::TScriptProfilerTrigger	*pTrig = NULL;
	ULONGLONG llTicksStart, llTicksEnd;

	//	If script profiler is on, search this trigger record and get pointer to it
	//	if not, create the corresponding record
	if ( IsSetEF(EF_Script_Profiler) )
	{
		TCHAR * pName = Str_GetTemp();

		//	lowercase for speed
		strcpy(pName, pszTrigName);
		_strlwr(pName);

		if ( g_profiler.initstate != 0xf1 )	// it is not initalised
		{
			memset(&g_profiler, 0, sizeof(g_profiler));
			g_profiler.initstate = static_cast<BYTE>(0xf1); // ''
		}

		for ( pTrig = g_profiler.TriggersHead; pTrig != NULL; pTrig = pTrig->next )
		{
			if ( !strcmp(pTrig->name, pName) )
				break;
		}

		// first time function called. so create a record for it
		if ( pTrig == NULL )
		{
			pTrig = new TScriptProfiler::TScriptProfilerTrigger;
			memset(pTrig, 0, sizeof(TScriptProfiler::TScriptProfilerTrigger));
			strcpy(pTrig->name, pName);
			if ( g_profiler.TriggersTail )
				g_profiler.TriggersTail->next = pTrig;
			else
				g_profiler.TriggersHead = pTrig;
			g_profiler.TriggersTail = pTrig;
		}

		//	prepare the informational block
		pTrig->called++;
		g_profiler.called++;
		TIME_PROFILE_START;
	}

	TRIGRET_TYPE	iRet = OnTriggerRunVal(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs);

	if ( IsSetEF(EF_Script_Profiler) && pTrig != NULL )
	{
		//	update the time call information
		TIME_PROFILE_END;
		llTicksStart = llTicksEnd - llTicksStart;
		pTrig->total += llTicksStart;
		pTrig->average = pTrig->total / pTrig->called;
		if ( pTrig->max < llTicksStart )
			pTrig->max = llTicksStart;
		if ( (pTrig->min > llTicksStart) || !pTrig->min )
			pTrig->min = llTicksStart;
		g_profiler.total += llTicksStart;
	}

	return iRet;
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
	SK_FORCONTID,		// loop through all items with this ID in the cont
	SK_FORCONTTYPE,
	SK_FORINSTANCE,
	SK_FORITEM,
	SK_FOROBJ,
	SK_FORPLAYERS,		// not necessary to be online
	SK_FORTIMERF,
	SK_IF,
	SK_RETURN,
	SK_WHILE,
	SK_QTY
};



LPCTSTR const CScriptObj::sm_szScriptKeys[SK_QTY+1] =
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



TRIGRET_TYPE CScriptObj::OnTriggerRun( CScript &s, TRIGRUN_TYPE trigrun, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * pResult )
{
	ADDTOCALLSTACK("CScriptObj::OnTriggerRun");
	// ARGS:
	//	TRIGRUN_SECTION_SINGLE = just this 1 line.
	// RETURN:
	//  TRIGRET_RET_FALSE = 0 = return and continue processing.
	//  TRIGRET_RET_TRUE = 1 = return and handled. (halt further processing)
	//  TRIGRET_RET_DEFAULT = 2 = if process returns nothing specifically.

	// CScriptFileContext set g_Log.m_pObjectContext is the current context (we assume)
	// DEBUGCHECK( this == g_Log.m_pObjectContext );

	//	all scripts should have args for locals to work.
	CScriptTriggerArgs argsEmpty;
	if ( !pArgs )
		pArgs = &argsEmpty;

	//	Script execution is always not threaded action
	EXC_TRY("TriggerRun");

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
		TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;

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

			default:
				break;
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
				case SK_FORTIMERF:
				case SK_DORAND:
				case SK_DOSWITCH:
				case SK_BEGIN:
					EXC_SET("begin/loop cycle");
					iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
					break;
				default:
					break;
			}
			if ( trigrun >= TRIGRUN_SINGLE_EXEC )
				return( TRIGRET_RET_DEFAULT );
			continue;	// just ignore it.
		}

		switch ( iCmd )
		{
			case SK_BREAK:
				return TRIGRET_BREAK;

			case SK_CONTINUE:
				return TRIGRET_CONTINUE;

			case SK_FORITEM:		EXC_SET("foritem");		iRet = OnTriggerForLoop( s, 1, pSrc, pArgs, pResult );			break;
			case SK_FORCHAR:		EXC_SET("forchar");		iRet = OnTriggerForLoop( s, 2, pSrc, pArgs, pResult );			break;
			case SK_FORCLIENTS:		EXC_SET("forclients");	iRet = OnTriggerForLoop( s, 0x12, pSrc, pArgs, pResult );		break;
			case SK_FOROBJ:			EXC_SET("forobjs");		iRet = OnTriggerForLoop( s, 3, pSrc, pArgs, pResult );			break;
			case SK_FORPLAYERS:		EXC_SET("forplayers");	iRet = OnTriggerForLoop( s, 0x22, pSrc, pArgs, pResult );		break;
			case SK_FOR:			EXC_SET("for");			iRet = OnTriggerForLoop( s, 4, pSrc, pArgs, pResult );			break;
			case SK_WHILE:			EXC_SET("while");		iRet = OnTriggerForLoop( s, 8, pSrc, pArgs, pResult );			break;
			case SK_FORINSTANCE:	EXC_SET("forinstance");	iRet = OnTriggerForLoop( s, 0x40, pSrc, pArgs, pResult );		break;
			case SK_FORTIMERF:		EXC_SET("fortimerf");	iRet = OnTriggerForLoop( s, 0x100, pSrc, pArgs, pResult );		break;
			case SK_FORCHARLAYER:
			case SK_FORCHARMEMORYTYPE:
				{
					EXC_SET("forchar[layer/memorytype]");
					CChar * pCharThis = dynamic_cast <CChar *> (this);
					if ( pCharThis )
					{
						if ( s.HasArgs() )
						{
							ParseText(s.GetArgRaw(), pSrc, 0, pArgs);
							if ( iCmd == SK_FORCHARLAYER )
								iRet = pCharThis->OnCharTrigForLayerLoop(s, pSrc, pArgs, pResult, static_cast<LAYER_TYPE>(s.GetArgVal()));
							else
								iRet = pCharThis->OnCharTrigForMemTypeLoop(s, pSrc, pArgs, pResult, static_cast<WORD>(s.GetArgVal()));
						}
						else
						{
							DEBUG_ERR(( "FORCHAR[layer/memorytype] called on char 0%lx (%s) without arguments.\n", static_cast<DWORD>(pCharThis->GetUID()), pCharThis->GetName() ));
							iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
						}
					}
					else
					{
						DEBUG_ERR(( "FORCHAR[layer/memorytype] called on non-char object '%s'.\n", GetName() ));
						iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
					}
				} break;
			case SK_FORCONT:
				{
					EXC_SET("forcont");
					if ( s.HasArgs() )
					{
						TCHAR * ppArgs[2];
						size_t iArgQty = Str_ParseCmds(const_cast<TCHAR *>(s.GetArgRaw()), ppArgs, COUNTOF(ppArgs), " \t,");
						
						if ( iArgQty >= 1 )
						{
							TemporaryString porigValue;
							strcpy(porigValue, ppArgs[0]);
							TCHAR *tempPoint = porigValue;
							ParseText( tempPoint, pSrc, 0, pArgs );
							
							CGrayUID pCurUid = static_cast<CGrayUID>(Exp_GetVal(tempPoint));
							if ( pCurUid.IsValidUID() )
							{
								CObjBase * pObj = pCurUid.ObjFind();
								if ( pObj && pObj->IsContainer() )
								{
									CContainer * pContThis = dynamic_cast<CContainer *>(pObj);
									
									CScriptLineContext StartContext = s.GetContext();
									CScriptLineContext EndContext = StartContext;
									iRet = pContThis->OnGenericContTriggerForLoop(s, pSrc, pArgs, pResult, StartContext, EndContext, ppArgs[1] != NULL ? Exp_GetVal(ppArgs[1]) : 255);
								}
								else
								{
									DEBUG_ERR(( "FORCONT called on invalid uid/invalid container (UID: 0%lx).\n", pCurUid.GetObjUID() ));
									iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
								}
							}
							else
							{
								DEBUG_ERR(( "FORCONT called with invalid arguments (UID: 0%lx, LEVEL: %s).\n", pCurUid.GetObjUID(), (ppArgs[1] && *ppArgs[1]) ? ppArgs[1] : "255" ));
								iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
							}
						}
						else
						{
							DEBUG_ERR(( "FORCONT called without arguments.\n" ));
							iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
						}
					}
					else
					{
						DEBUG_ERR(( "FORCONT called without arguments.\n" ));
						iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
					}
				} break;
			case SK_FORCONTID:
			case SK_FORCONTTYPE:
				{
					EXC_SET("forcont[id/type]");
					CObjBase * pObjCont = dynamic_cast <CObjBase *> (this);
					CContainer * pCont = dynamic_cast <CContainer *> (this);
					if ( pObjCont && pCont )
					{
						if ( s.HasArgs() )
						{
							LPCTSTR pszKey = s.GetArgRaw();
							SKIP_SEPARATORS(pszKey);
						
							TCHAR * ppArgs[2];
							TemporaryString ppParsed;

							if ( Str_ParseCmds(const_cast<TCHAR *>(pszKey), ppArgs, COUNTOF(ppArgs), " \t," ) >= 1 )
							{
								strcpy(ppParsed, ppArgs[0]);
								if ( ParseText( ppParsed, pSrc, 0, pArgs ) > 0 )
								{
									CScriptLineContext StartContext = s.GetContext();
									CScriptLineContext EndContext = StartContext;
									iRet = pCont->OnContTriggerForLoop( s, pSrc, pArgs, pResult, StartContext, EndContext, g_Cfg.ResourceGetID( ( iCmd == SK_FORCONTID ) ? RES_ITEMDEF : RES_TYPEDEF, static_cast<LPCTSTR &>(ppParsed)), 0, ppArgs[1] != NULL ? Exp_GetVal( ppArgs[1] ) : 255 );
								}
								else
								{
									DEBUG_ERR(( "FORCONT[id/type] called on container 0%lx with incorrect arguments.\n", static_cast<DWORD>(pObjCont->GetUID()) ));
									iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
								}
							}
							else
							{
								DEBUG_ERR(( "FORCONT[id/type] called on container 0%lx with incorrect arguments.\n", static_cast<DWORD>(pObjCont->GetUID()) ));
								iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
							}
						}
						else
						{
							DEBUG_ERR(( "FORCONT[id/type] called on container 0%lx without arguments.\n", static_cast<DWORD>(pObjCont->GetUID()) ));
							iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
						}
					}
					else
					{
						DEBUG_ERR(( "FORCONT[id/type] called on non-container object '%s'.\n", GetName() ));
						iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
					}
				} break;
			default:
				{
					// Parse out any variables in it. (may act like a verb sometimes?)
					EXC_SET("parsing");
					if( strchr(s.GetKey(), '<') ) 
					{
						EXC_SET("parsing <> in a key");
						TemporaryString buf;
						strcpy(buf, s.GetKey());
						strcat(buf, " ");
						strcat(buf, s.GetArgRaw());
						ParseText(buf, pSrc, 0, pArgs);

						s.ParseKey(buf);
					}
					else
					{
						ParseText( s.GetArgRaw(), pSrc, 0, pArgs );
					}
				}
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
			case SK_FORTIMERF:
			case SK_FOR:
			case SK_WHILE:
				if ( iRet != TRIGRET_ENDIF )
					return iRet;
				break;
			case SK_DORAND:	// Do a random line in here.
			case SK_DOSWITCH:
				{
					EXC_SET("dorand/doswitch");
					INT64 iVal = s.GetArgLLVal();
					if ( iCmd == SK_DORAND )
						iVal = Calc_GetRandLLVal(iVal);
					for ( ; ; --iVal )
					{
						iRet = OnTriggerRun( s, (iVal == 0) ? TRIGRUN_SINGLE_TRUE : TRIGRUN_SINGLE_FALSE, pSrc, pArgs, pResult );
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
					return TRIGRET_RET_TRUE;
				}
				return static_cast<TRIGRET_TYPE>(s.GetArgVal());
			case SK_IF:
				{
					EXC_SET("if statement");
					bool fTrigger = s.GetArgVal() ? true : false;
					bool fBeenTrue = false;
					for (;;)
					{
						iRet = OnTriggerRun( s, fTrigger ? TRIGRUN_SECTION_TRUE : TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
						if (( iRet < TRIGRET_ENDIF ) || ( iRet >= TRIGRET_RET_HALFBAKED ))
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
				EXC_SET("parsing");
				if ( !pArgs->r_Verb(s, pSrc) )
				{
					bool	fRes;
					if ( !strcmpi(s.GetKey(), "call" ) )
					{
						EXC_SET("call");
						CGString sVal;
						TCHAR * argRaw = s.GetArgRaw();
						CScriptObj *pRef = this;

						// Parse object references, src.* is not parsed
						// by r_GetRef so do it manually
						r_GetRef(const_cast<LPCTSTR &>(static_cast<LPTSTR &>(argRaw)), pRef);
						if ( !strnicmp("SRC.", argRaw, 4) )
						{
							argRaw += 4;
							pRef = pSrc->GetChar();
						}

						// Check that an object is referenced
						if (pRef != NULL)
						{
							// Locate arguments for the called function
							TCHAR *z = strchr(argRaw, ' ');

							if( z )
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
								CGString s1_raw = pArgs->m_s1;
								pArgs->m_v.SetCount(0);
								pArgs->Init(z);

								fRes = pRef->r_Call(argRaw, pSrc, pArgs, &sVal);

								pArgs->m_iN1 = iN1;
								pArgs->m_iN2 = iN2;
								pArgs->m_iN3 = iN3;
								pArgs->m_pO1 = pO1;
								pArgs->m_s1 = s1;
								pArgs->m_s1_raw = s1_raw;
								pArgs->m_v.SetCount(0);
							}
							else
							{
								fRes = pRef->r_Call(argRaw, pSrc, pArgs, &sVal);
							}
						}
						else
						{
							fRes = false;
						}
					} else if ( !strcmpi(s.GetKey(), "FullTrigger" ) )
					{
						EXC_SET("FullTrigger");
						CGString sVal;
						TCHAR *piCmd[7];
						TCHAR *psTmp = Str_GetTemp();
						strcpy(psTmp, s.GetArgRaw());
						size_t iArgQty = Str_ParseCmds(psTmp, piCmd, COUNTOF(piCmd), " ,\t");
						CScriptObj *pRef = this;
						if ( iArgQty == 2 )
						{
							CGrayUID uid = static_cast<CGrayUID>(ATOI(piCmd[1]));
							if ( uid.ObjFind() )
								pRef = uid.ObjFind();
						}

						// Parse object references, src.* is not parsed
						// by r_GetRef so do it manually
						//r_GetRef(const_cast<LPCTSTR &>(static_cast<LPTSTR &>(argRaw)), pRef);
						if ( !strnicmp("SRC.", psTmp, 4) )
						{
							psTmp += 4;
							pRef = pSrc->GetChar();
						}

						// Check that an object is referenced
						if (pRef != NULL)
						{
							// Locate arguments for the called function
							TRIGRET_TYPE tRet;
							TCHAR *z = strchr(psTmp, ' ');

							if( z )
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
								CGString s1_raw = pArgs->m_s1;
								pArgs->m_v.SetCount(0);
								pArgs->Init(z);

								tRet = pRef->OnTrigger( psTmp, pSrc, pArgs);

								pArgs->m_iN1 = iN1;
								pArgs->m_iN2 = iN2;
								pArgs->m_iN3 = iN3;
								pArgs->m_pO1 = pO1;
								pArgs->m_s1 = s1;
								pArgs->m_s1_raw = s1_raw;
								pArgs->m_v.SetCount(0);
							}
							else
							{
								tRet = pRef->OnTrigger( psTmp, pSrc, pArgs);
							}
							pArgs->m_VarsLocal.SetNum("return",tRet,false);
							fRes = tRet > 0 ? 1 : 0;
						}
						else
						{
							fRes = false;
						}
					}
					else
					{
						EXC_SET("verb");
						fRes = r_Verb(s, pSrc);
					}

					if ( !fRes  )
					{
						DEBUG_MSG(( "WARNING: Trigger Bad Verb '%s','%s'\n", s.GetKey(), s.GetArgStr()));
					}
				}
				break;
		}

		if ( trigrun >= TRIGRUN_SINGLE_EXEC )
			return( TRIGRET_RET_DEFAULT );
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("key '%s' runtype '%d' pargs '%p' ret '%s' [%p]\n",
		s.GetKey(), trigrun, static_cast<void *>(pArgs), pResult == NULL? "" : pResult->GetPtr(), static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return TRIGRET_RET_DEFAULT;
}

TRIGRET_TYPE CScriptObj::OnTriggerRunVal( CScript &s, TRIGRUN_TYPE trigrun, CTextConsole * pSrc, CScriptTriggerArgs * pArgs )
{
	// Get the TRIGRET_TYPE that is returned by the script
	// This should be used instead of OnTriggerRun() when pReturn is not used
	ADDTOCALLSTACK("CScriptObj::OnTriggerRunVal");

	CGString sVal;
	TRIGRET_TYPE tr = TRIGRET_RET_DEFAULT;

	OnTriggerRun( s, trigrun, pSrc, pArgs, &sVal );

	LPCTSTR pszVal = sVal.GetPtr();
	if ( pszVal && *pszVal )
	{
		tr = static_cast<TRIGRET_TYPE>(Exp_GetVal(pszVal));
	}

	return tr;
}

////////////////////////////////////////////////////////////////////////////////////////
// -CFileObj

enum FO_TYPE
{
	#define ADD(a,b) FO_##a,
	#include "../tables/CFile_props.tbl"
	#undef ADD
	FO_QTY
};

LPCTSTR const CFileObj::sm_szLoadKeys[FO_QTY+1] =
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

LPCTSTR const CFileObj::sm_szVerbKeys[FOV_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CFile_functions.tbl"
	#undef ADD
	NULL
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
	ADDTOCALLSTACK("CFileObj::SetDefaultMode");
	bAppend = true; bCreate = false; 
	bRead = true; bWrite = true; 
}

TCHAR * CFileObj::GetReadBuffer(bool bDelete = false)
{
	ADDTOCALLSTACK("CFileObj::GetReadBuffer");
	if ( bDelete )
		memset(this->tBuffer, 0, SCRIPT_MAX_LINE_LEN);
	else
		*tBuffer = 0;

	return tBuffer;
}

CGString * CFileObj::GetWriteBuffer(void)
{
	ADDTOCALLSTACK("CFileObj::GetWriteBuffer");
	if ( !cgWriteBuffer )
		cgWriteBuffer = new CGString();

	cgWriteBuffer->Empty( ( cgWriteBuffer->GetLength() > (SCRIPT_MAX_LINE_LEN/4) ) );

	return cgWriteBuffer;
}

bool CFileObj::IsInUse()
{
	ADDTOCALLSTACK("CFileObj::IsInUse");
	return sWrite->IsFileOpen();
}

void CFileObj::FlushAndClose()
{
	ADDTOCALLSTACK("CFileObj::FlushAndClose");
	if ( sWrite->IsFileOpen() )
	{
		sWrite->Flush();
		sWrite->Close();
	}
}

bool CFileObj::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	UNREFERENCED_PARAMETER(pszKey);
	UNREFERENCED_PARAMETER(pRef);
	return( false );
}

bool CFileObj::OnTick(){ return( true ); }
int CFileObj::FixWeirdness(){ return( 0 ); }

bool CFileObj::r_LoadVal( CScript & s )
{	
	ADDTOCALLSTACK("CFileObj::r_LoadVal");
	EXC_TRY("LoadVal");
	LPCTSTR pszKey = s.GetKey();

	if ( !strnicmp("MODE.",pszKey,5) )
	{
		pszKey += 5;
		if ( ! sWrite->IsFileOpen() )
		{
			if ( !strnicmp("APPEND",pszKey,6) )
			{
				bAppend = (s.GetArgVal() != 0);
				bCreate = false;
			}
			else if ( !strnicmp("CREATE",pszKey,6) )
			{
				bCreate = (s.GetArgVal() != 0);
				bAppend = false;
			}
			else if ( !strnicmp("READFLAG",pszKey,8) )
				bRead = (s.GetArgVal() != 0);
			else if ( !strnicmp("WRITEFLAG",pszKey,9) )
				bWrite = (s.GetArgVal() != 0);
			else if ( !strnicmp("SETDEFAULT",pszKey,7) )
				SetDefaultMode();
			else
				return( false );

			return( true );
		}
		else
		{
			g_Log.Event(LOGL_ERROR, "FILE (%s): Cannot set mode after file opening\n", static_cast<LPCTSTR>(sWrite->GetFilePath()));
		}	
		return( false );
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
				
				if ( bLine )
				{
					ppArgs->Copy( s.GetArgStr() );
#ifdef _WIN32
					ppArgs->Add( "\r\n" );
#else
					ppArgs->Add( "\n" );
#endif
				}
				else if ( bChr )
				{
					ppArgs->Format( "%c", static_cast<TCHAR>(s.GetArgVal()) );
				}
				else
					ppArgs->Copy( s.GetArgStr() );

				bool bSuccess = false;

				if ( bChr )
					bSuccess = sWrite->Write(ppArgs->GetPtr(), 1);
				else
					bSuccess = sWrite->WriteString( ppArgs->GetPtr() );
		
				if ( !bSuccess )
				{
					g_Log.Event(LOGL_ERROR, "FILE: Failed writing to \"%s\".\n", static_cast<LPCTSTR>(sWrite->GetFilePath()));
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
	ADDTOCALLSTACK("CFileObj::r_WriteVal");
	EXC_TRY("WriteVal");
	ASSERT(pszKey != NULL);

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
			return( false );

		return( true );
	}

	int index = FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );

	switch ( index )
	{
		case FO_FILEEXIST:
			{
				pszKey += strlen(sm_szLoadKeys[index]);
				GETNONWHITESPACE( pszKey );

				TCHAR * ppCmd = Str_TrimWhitespace(const_cast<TCHAR *>(pszKey));
				if ( !( ppCmd && strlen(ppCmd) ))
					return( false );

				CFile * pFileTest = new CFile();
				sVal.FormatVal(pFileTest->Open(ppCmd));

				delete pFileTest;
			} break;

		case FO_FILELINES:
			{
				pszKey += strlen(sm_szLoadKeys[index]);
				GETNONWHITESPACE( pszKey );
				
				TCHAR * ppCmd = Str_TrimWhitespace(const_cast<TCHAR *>(pszKey));
				if ( !( ppCmd && strlen(ppCmd) ))
					return( false );

				CFileText * sFileLine = new CFileText();
				if ( !sFileLine->Open(ppCmd, OF_READ|OF_TEXT) )
				{
					delete sFileLine;
					return( false );
				}

				TCHAR * ppArg = this->GetReadBuffer();
				int iLines = 0;

				while ( ! sFileLine->IsEOF() )
				{
					sFileLine->ReadString( ppArg, SCRIPT_MAX_LINE_LEN );
					++iLines;
				}
				sFileLine->Close();

				sVal.FormatVal( iLines );
				
				delete sFileLine;
			} break;

		case FO_FILEPATH:
			sVal.Format("%s", sWrite->IsFileOpen() ? static_cast<LPCTSTR>(sWrite->GetFilePath()) : "" );
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

				TCHAR * ppCmd = Str_TrimWhitespace(const_cast<TCHAR *>(pszKey));
				if ( !( ppCmd && strlen(ppCmd) ))
					return( false );

				if ( sWrite->IsFileOpen() )
				{
					g_Log.Event(LOGL_ERROR, "FILE: Cannot open file (%s). First close \"%s\".\n", ppCmd, static_cast<LPCTSTR>(sWrite->GetFilePath()));
					return( false );
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
				size_t iRead = 1;

				if ( !bChr )
				{
					pszKey += strlen(sm_szLoadKeys[index]);
					GETNONWHITESPACE( pszKey );

					iRead = Exp_GetVal(pszKey);
					if ( iRead <= 0 || iRead >= SCRIPT_MAX_LINE_LEN)
						return( false );
				}

				if ( ( ( sWrite->GetPosition() + iRead ) > sWrite->GetLength() ) || ( sWrite->IsEOF() ) )
				{
					g_Log.Event(LOGL_ERROR, "FILE: Failed reading %" FMTSIZE_T " byte from \"%s\". Too near to EOF.\n", iRead, static_cast<LPCTSTR>(sWrite->GetFilePath()));
					return( false );
				}

				TCHAR * ppArg = this->GetReadBuffer(true);

				if ( iRead != sWrite->Read(ppArg, iRead) )
				{
					g_Log.Event(LOGL_ERROR, "FILE: Failed reading %" FMTSIZE_T " byte from \"%s\".\n", iRead, static_cast<LPCTSTR>(sWrite->GetFilePath()));
					return( false );
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
				ASSERT(ppArg != NULL);

				INT64 iLines = Exp_GetVal(pszKey);
				if ( iLines < 0 )
					return( false );

				unsigned long ulSeek = sWrite->GetPosition();
				sWrite->SeekToBegin();

				if ( iLines == 0 )
				{
					while ( ! sWrite->IsEOF() )
						sWrite->ReadString( ppArg, SCRIPT_MAX_LINE_LEN );	
				}
				else
				{
					for ( INT64 x = 1; x <= iLines; ++x )
					{
						if ( sWrite->IsEOF() )
							break;

						ppArg = this->GetReadBuffer();
						sWrite->ReadString( ppArg, SCRIPT_MAX_LINE_LEN );
					}
				}

				sWrite->Seek(ulSeek);

				if ( size_t iLinelen = strlen(ppArg) )
				{
					while ( iLinelen > 0 )
					{
						--iLinelen;
						if ( isgraph(ppArg[iLinelen]) || (ppArg[iLinelen] == 0x20) || (ppArg[iLinelen] == '\t') )
						{
							++iLinelen;
							ppArg[iLinelen] = '\0';
							break;
						}
					}
				}

				sVal.Format( "%s", ppArg );
			} break;

		case FO_SEEK:
			{
				pszKey += strlen(sm_szLoadKeys[index]);
				GETNONWHITESPACE(pszKey);

				if (pszKey[0] == '\0')
					return( false );

				if (strcmpi("BEGIN", pszKey) == 0)
				{
					sVal.FormatVal( sWrite->Seek(0, SEEK_SET) );
				}
				else if (strcmpi("END", pszKey) == 0)
				{
					sVal.FormatVal( sWrite->Seek(0, SEEK_END) );
				}
				else
				{
					sVal.FormatVal( sWrite->Seek(Exp_GetVal(pszKey), SEEK_SET) );
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
	ADDTOCALLSTACK("CFileObj::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);

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
					return( false );

				if ( sWrite->IsFileOpen() && !strcmp(s.GetArgStr(),sWrite->GetFileTitle()) )
					return( false );

				STDFUNC_UNLINK(s.GetArgRaw());
			} break;

		case FOV_FLUSH:
			if ( sWrite->IsFileOpen() )
				sWrite->Flush();
			break;

		default:
			return( false );
	}

	return( true );
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

bool CFileObj::FileOpen( LPCTSTR sPath )
{
	ADDTOCALLSTACK("CFileObj::FileOpen");
	if ( sWrite->IsFileOpen() )
		return( false );

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

enum CFO_TYPE
{
#define ADD(a,b) CFO_##a,
#include "../tables/CFileObjContainer_props.tbl"
#undef ADD
	CFO_QTY
};

LPCTSTR const CFileObjContainer::sm_szLoadKeys[CFO_QTY+1] =
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

LPCTSTR const CFileObjContainer::sm_szVerbKeys[CFOV_QTY+1] =
{
#define ADD(a,b) b,
#include "../tables/CFileObjContainer_functions.tbl"
#undef ADD
	NULL
};

void CFileObjContainer::ResizeContainer( size_t iNewRange )
{
	ADDTOCALLSTACK("CFileObjContainer::ResizeContainer");
	if ( iNewRange == sFileList.size() )
	{
		return;
	}

	bool bDeleting = ( iNewRange < sFileList.size() );
	int howMuch = static_cast<int>(iNewRange) - static_cast<int>(sFileList.size());
	if ( howMuch < 0 )
	{
		howMuch = (-howMuch);
	}

	if ( bDeleting )
	{
		if ( sFileList.empty() )
		{
			return;
		}

		CFileObj * pObjHolder = NULL;

		for ( size_t i = (sFileList.size() - 1); howMuch > 0; --howMuch, --i )
		{
			pObjHolder = sFileList.at(i);
			sFileList.pop_back();

			if ( pObjHolder )
			{
				delete pObjHolder;
			}
		}
	} 
	else
	{
		for ( int i = 0; i < howMuch; ++i )
		{
			sFileList.push_back(new CFileObj());
		}
	}
}

CFileObjContainer::CFileObjContainer()
{
	iGlobalTimeout = iCurrentTick = 0;
	SetFilenumber(0);
}

CFileObjContainer::~CFileObjContainer()
{
	ResizeContainer(0);
	sFileList.clear();
}

int CFileObjContainer::GetFilenumber()
{
	ADDTOCALLSTACK("CFileObjContainer::GetFilenumber");
	return iFilenumber;
}

void CFileObjContainer::SetFilenumber( int iHowMuch )
{
	ADDTOCALLSTACK("CFileObjContainer::SetFilenumber");
	ResizeContainer(iHowMuch);
	iFilenumber = iHowMuch;
}

bool CFileObjContainer::OnTick()
{
	ADDTOCALLSTACK("CFileObjContainer::OnTick");
	EXC_TRY("Tick");

	if ( !iGlobalTimeout )
	{
		return true;
	}

	if ( ++iCurrentTick >= iGlobalTimeout )
	{
		iCurrentTick = 0;

		for ( std::vector<CFileObj *>::iterator i = sFileList.begin(); i != sFileList.end(); ++i )
		{
			if ( !(*i)->OnTick() )
			{
				// Error and fixweirdness
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

bool CFileObjContainer::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CFileObjContainer::r_GetRef");
	if ( !strnicmp("FIRSTUSED.",pszKey,10) )
	{
		pszKey += 10; 

		CFileObj * pFirstUsed = NULL;
		for ( std::vector<CFileObj *>::iterator i = sFileList.begin(); i != sFileList.end(); ++i )
		{
			if ( (*i)->IsInUse() )
			{
				pFirstUsed = (*i);
				break;
			}
		}

		if ( pFirstUsed != NULL ) 
		{ 
			pRef = pFirstUsed; 
			return( true ); 
		}
	}
	else
	{
		size_t nNumber = static_cast<size_t>( Exp_GetVal(pszKey) );
		SKIP_SEPARATORS(pszKey);

		if ( nNumber >= sFileList.size() )
			return( false );

		CFileObj * pFile = sFileList.at(nNumber);

		if ( pFile != NULL ) 
		{ 
			pRef = pFile; 
			return( true ); 
		}
	}

	return( false );
}

bool CFileObjContainer::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CFileObjContainer::r_LoadVal");
	EXC_TRY("LoadVal");
	LPCTSTR pszKey = s.GetKey();

	int index = FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );

	switch ( index )
	{
		case CFO_OBJECTPOOL:
			SetFilenumber(s.GetArgVal());
			break;

		case CFO_GLOBALTIMEOUT:
			iGlobalTimeout = labs(s.GetArgVal()*TICK_PER_SEC);
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

bool CFileObjContainer::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CFileObjContainer::r_WriteVal");
	EXC_TRY("WriteVal");

	if ( !strnicmp("FIRSTUSED.", pszKey, 10) )
	{
		pszKey += 10;

		CFileObj *pFirstUsed = NULL;
		for ( std::vector<CFileObj *>::iterator i = sFileList.begin(); i != sFileList.end(); ++i )
		{
			if ( (*i)->IsInUse() )
			{
				pFirstUsed = (*i);
				break;
			}
		}

		if ( pFirstUsed )
			return static_cast<CScriptObj *>(pFirstUsed)->r_WriteVal(pszKey, sVal, pSrc);

		return false;
	}

	int iIndex = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);

	if ( iIndex < 0 )
	{
		size_t nNumber = static_cast<size_t>(Exp_GetVal(pszKey));
		SKIP_SEPARATORS(pszKey);

		if ( nNumber >= sFileList.size() )
			return false;

		CFileObj *pFile = sFileList.at(nNumber);
		if ( pFile )
		{
			CScriptObj *pObj = dynamic_cast<CScriptObj*>(pFile);
			if ( pObj )
				return pObj->r_WriteVal(pszKey, sVal, pSrc);
		}

		return false;
	}

	switch ( iIndex )
	{
		case CFO_OBJECTPOOL:
			sVal.FormatVal(GetFilenumber());
			break;
		case CFO_GLOBALTIMEOUT:
			sVal.FormatVal(iGlobalTimeout / TICK_PER_SEC);
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

bool CFileObjContainer::r_Verb( CScript & s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CFileObjContainer::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);

	LPCTSTR pszKey = s.GetKey();

	if ( !strnicmp("FIRSTUSED.", pszKey, 10) )
	{
		pszKey += 10;

		CFileObj *pFirstUsed = NULL;
		for ( std::vector<CFileObj *>::iterator i = sFileList.begin(); i != sFileList.end(); ++i )
		{
			if ( (*i)->IsInUse() )
			{
				pFirstUsed = (*i);
				break;
			}
		}

		if ( pFirstUsed )
		{
			return static_cast<CScriptObj *>(pFirstUsed)->r_Verb(s, pSrc);
		}

		return false;
	}

	int index = FindTableSorted(pszKey, sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1);

	if ( index < 0 )
	{
		if ( strchr(pszKey, '.') ) // 0.blah format
		{
			size_t nNumber = static_cast<size_t>(Exp_GetVal(pszKey));
			if ( nNumber < sFileList.size() )
			{
				SKIP_SEPARATORS(pszKey);

				CFileObj *pFile = sFileList.at(nNumber);

				if ( pFile )
				{
					CScriptObj *pObj = dynamic_cast<CScriptObj*>(pFile);
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

	switch ( index )
	{
		case CFOV_CLOSEOBJECT:
		case CFOV_RESETOBJECT:
		{
			bool bResetObject = (index == CFOV_RESETOBJECT);
			if ( s.HasArgs() )
			{
				size_t nNumber = static_cast<size_t>(s.GetArgVal());
				if ( nNumber >= sFileList.size() )
					return false;

				CFileObj *pObjVerb = sFileList.at(nNumber);
				if ( bResetObject )
				{
					delete pObjVerb;
					sFileList.at(nNumber) = new CFileObj();
				}
				else
				{
					pObjVerb->FlushAndClose();
				}
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
