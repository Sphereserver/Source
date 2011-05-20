//
// CExpression.h
// Copyright Menace Software (www.menasoft.com).
//

#ifndef _INC_CEXPRSSION_H
#define _INC_CEXPRSSION_H
#pragma once

#include "CAtom.h"

#define _ISCSYMF(ch) ( IsAlpha(ch) || (ch)=='_')	// __iscsym or __iscsymf
#define _ISCSYM(ch) ( isalnum(ch) || (ch)=='_')	// __iscsym or __iscsymf

#ifndef M_PI 
	#define M_PI 3.14159265358979323846
#endif

#define EXPRESSION_MAX_KEY_LEN SCRIPT_MAX_SECTION_LEN

enum DEFMSG_TYPE
{
	#define MSG(a,b,c) DEFMSG_##a,
	#include "../tables/defmessages.tbl"
	#undef MSG
	DEFMSG_QTY
};

enum INTRINSIC_TYPE
{
	INTRINSIC_COS = 0,
	INTRINSIC_ID,
	INTRINSIC_ISNUMBER,
	INTRINSIC_ISOBSCENE,
	INTRINSIC_LOGARITHM,
	INTRINSIC_NAPIERPOW,
	INTRINSIC_QVAL,
	INTRINSIC_RAND,
	INTRINSIC_RANDBELL,
	INTRINSIC_SIN,
	INTRINSIC_SQRT,
	INTRINSIC_STRASCII,
	INTRINSIC_STRCMP,
	INTRINSIC_STRCMPI,
	INTRINSIC_StrIndexOf,
	INTRINSIC_STRLEN,
	INTRINSIC_STRMATCH,
	INTRINSIC_STRREGEX,
	INTRINSIC_TAN,
	INTRINSIC_QTY
};

static LPCTSTR const sm_IntrinsicFunctions[INTRINSIC_QTY+1] =
{
	"COS",		// cosinus
	"ID",		// ID(x) = truncate the type portion of an Id
	"ISNUMBER",		// ISNUMBER(var)
	"ISOBSCENE",	// test for non-allowed strings
	"LOGARITHM",	// log()/log10()
	"NAPIERPOW",	// exp()
	"QVAL",		// QVAL(test1,test2,ret1,ret2,ret3) - test1 ? test2 (< ret1, = ret2, > ret3)
	"RAND",		// RAND(x) = flat random
	"RANDBELL",	// RANDBELL(center,variance25)
	"SIN",
	"SQRT",		// sqrt()
	"StrAscii",
	"STRCMP",	// STRCMP(str1,str2)
	"STRCMPI",	// STRCMPI(str1,str2)
	"StrIndexOf", // StrIndexOf(string,searchVal,[index]) = find the index of this, -1 = not here.
	"STRLEN",	// STRLEN(str)
	"STRMATCH",	// STRMATCH(str,*?pattern)
	"STRREGEX",
	"TAN",		// tan()
	NULL
};

extern class CExpression
{
public:
	static const char *m_sClassName;
	CVarDefMap		m_VarDefs;		// Defined variables in sorted order.
	CVarDefMap		m_VarGlobals;	// Global variables
	CListDefMap		m_ListGlobals; // Global lists
	CGString		m_sTmp;

								//	defined default messages
	static TCHAR sm_szMessages[DEFMSG_QTY][128];			// like: "You put %s to %s"
	static LPCTSTR const sm_szMsgNames[DEFMSG_QTY];		// like: "put_it"

public:
	// Strict G++ Prototyping produces an error when not casting char*& to const char*&
	// So this is a rather lazy workaround
	inline int GetSingle( LPTSTR &pArgs )
	{
		return GetSingle( (LPCTSTR&)pArgs );
	}

	inline int GetRange( LPTSTR &pArgs )
	{
		return GetRange( (LPCTSTR&)pArgs );
	}

	inline int GetRangeVals( LPTSTR &pExpr, int * piVals, int iMaxQty )
	{
		return GetRangeVals( (LPCTSTR&)pExpr, piVals, iMaxQty );
	}

	inline int GetVal( LPTSTR &pArgs )
	{
		return GetVal( (LPCTSTR&)pArgs );
	}

	// Evaluate using the stuff we know.
	int GetSingle( LPCTSTR & pArgs );
	int GetVal( LPCTSTR & pArgs );
	int GetValMath( int lVal, LPCTSTR & pExpr );
	int GetRangeVals( LPCTSTR & pExpr, int * piVals, int iMaxQty );
	int GetRange( LPCTSTR & pArgs );

public:
	CExpression();
	~CExpression();

private:
	CExpression(const CExpression& copy);
	CExpression& operator=(const CExpression& other);
} g_Exp;

extern bool IsSimpleNumberString( LPCTSTR pszTest );
extern bool IsStrNumericDec( LPCTSTR pszTest );
extern bool IsStrNumeric( LPCTSTR pszTest );
extern bool IsStrEmpty( LPCTSTR pszTest );
inline extern bool IsCharNumeric( char & Test );

// Numeric formulas
extern int Calc_GetRandVal( int iqty );
extern int Calc_GetRandVal2( int iMin, int iMax );
extern int Calc_GetLog2( UINT iVal );
extern int Calc_GetSCurve( int iValDiff, int iVariance );
extern int Calc_GetBellCurve( int iValDiff, int iVariance );

extern DWORD ahextoi( LPCTSTR pArgs ); // Convert hex string to integer

#define Exp_GetSingle( pa ) g_Exp.GetSingle( pa )
#define Exp_GetVal( pa )	g_Exp.GetVal( pa )
#define Exp_GetRange( pa )	g_Exp.GetRange( pa )

//inline int CVarDefStr::GetValNum() const
//{
//	LPCTSTR pszStr = m_sVal;
//	return( Exp_GetVal(pszStr));
//}
//
//inline LPCTSTR CVarDefNum::GetValStr() const
//{
//	TCHAR * pszTmp = Str_GetTemp();
//	sprintf(pszTmp, "0%x", m_iVal);
//	return pszTmp;
//}

#endif	// _INC_CEXPRSSION_H
