#ifndef _INC_CEXPRSSION_H
#define _INC_CEXPRSSION_H
#pragma once

#define _ISCSYMF(ch) (IsAlpha(ch) || (ch == '_'))	// __iscsym or __iscsymf
#define _ISCSYM(ch) (isalnum(ch) || (ch == '_'))	// __iscsym or __iscsymf

#ifndef M_PI 
	#define M_PI 3.14159265358979323846
#endif

#define EXPRESSION_MAX_KEY_LEN 128

enum DEFMSG_TYPE
{
	#define MSG(a,b) DEFMSG_##a,
	#include "../tables/defmessages.tbl"
	DEFMSG_QTY
};

enum INTRINSIC_TYPE
{
	INTRINSIC_ABS,
	INTRINSIC_ARCCOS,
	INTRINSIC_ARCSIN,
	INTRINSIC_ARCTAN,
	INTRINSIC_COS,
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

static LPCTSTR const sm_IntrinsicFunctions[INTRINSIC_QTY + 1] =
{
	"ABS",			// llabs()
	"ARCCOS",		// acos()
	"ARCSIN",		// asin()
	"ARCTAN",		// atan()
	"COS",			// cos()
	"ID",			// ID(x) - truncate the type portion of an id
	"ISNUMBER",		// ISNUMBER(str) - check if string is number
	"ISOBSCENE",	// ISOBSCENE(str) - check if string is obscene
	"LOGARITHM",	// log(), log10()
	"NAPIERPOW",	// exp()
	"QVAL",			// QVAL(test1,test2,ret1,ret2,ret3) - test1 ? test2 (< ret1, = ret2, > ret3)
	"RAND",			// RAND(x) - flat random
	"RANDBELL",		// RANDBELL(center,variance25) - bell curve random
	"SIN",			// sin()
	"SQRT",			// sqrt()
	"STRASCII",		// STRASCII(str) - check if string is ASCII
	"STRCMP",		// STRCMP(str1,str2) - compare strings (case sensitive)
	"STRCMPI",		// STRCMPI(str1,str2) - compare strings (case insensitive)
	"STRINDEXOF",	// STRINDEXOF(string,searchVal,[index]) - get index of string inside substring (-1 = not found)
	"STRLEN",		// STRLEN(str) - get string length
	"STRMATCH",		// STRMATCH(str,*?pattern) - compare string with pattern
	"STRREGEX",		// STRREGEX(pattern,str) - compare string with regular expression
	"TAN",			// tan()
	NULL
};

extern class CExpression
{
public:
	static const char *m_sClassName;
	CVarDefMap m_VarDefs;			// Defined variables in sorted order
	CVarDefMap m_VarGlobals;		// Global variables
	CListDefMap m_ListGlobals;		// Global lists
	CListDefMap m_ListInternals;	// Internal lists
	CGString m_sTmp;

	// Defined default messages
	static TCHAR sm_szMessages[DEFMSG_QTY][128];		// like: "You put %s to %s"
	static LPCTSTR const sm_szMsgNames[DEFMSG_QTY];		// like: "put_it"

public:
	// Strict G++ Prototyping produces an error when not casting char*& to const char*&, so this is a rather lazy workaround
	inline INT64 GetSingle(LPTSTR &pszArgs)
	{
		return GetSingle(const_cast<LPCTSTR &>(pszArgs));
	}

	inline INT64 GetVal(LPTSTR &pszArgs)
	{
		return GetVal(const_cast<LPCTSTR &>(pszArgs));
	}

	inline int GetRangeVals(LPTSTR &pszArgs, INT64 *piVals, int iMaxQty)
	{
		return GetRangeVals(const_cast<LPCTSTR &>(pszArgs), piVals, iMaxQty);
	}

	inline INT64 GetRange(LPTSTR &pszArgs)
	{
		return GetRange(const_cast<LPCTSTR &>(pszArgs));
	}

	// Evaluate using the stuff we know.
	INT64 GetSingle(LPCTSTR &pszArgs);
	INT64 GetValMath(INT64 lVal, LPCTSTR &pszArgs);
	INT64 GetVal(LPCTSTR &pszArgs);
	int GetRangeVals(LPCTSTR &pszArgs, INT64 *piVals, int iMaxQty);
	INT64 GetRange(LPCTSTR &pszArgs);

public:
	CExpression();
	~CExpression();

private:
	CExpression(const CExpression &copy);
	CExpression &operator=(const CExpression &other);
} g_Exp;

inline extern bool IsCharNumeric(char &Test);
extern bool IsStrEmpty(LPCTSTR pszTest);
extern bool IsStrNumericDec(LPCTSTR pszTest);
extern bool IsStrNumeric(LPCTSTR pszTest);
extern bool IsSimpleNumberString(LPCTSTR pszTest);
extern bool IsValidDef(LPCTSTR pszTest);
extern bool IsValidGameObjDef(LPCTSTR pszTest);

// Numeric formulas
extern int Calc_GetLog2(UINT iVal);
extern int Calc_GetRandVal(int iQty);
extern int Calc_GetRandVal2(int iMin, int iMax);
extern INT64 Calc_GetRandLLVal(INT64 iQty);
extern INT64 Calc_GetRandLLVal2(INT64 iMin, INT64 iMax);
extern int Calc_GetBellCurve(int iValDiff, int iVariance);
extern int Calc_GetSCurve(int iValDiff, int iVariance);

extern DWORD ahextoi(LPCTSTR pszArgs);		// convert hex string to int
extern INT64 ahextoi64(LPCTSTR pszArgs);	// convert hex string to int64

#define Exp_GetSingle(pa)	static_cast<int>(g_Exp.GetSingle(pa))
#define Exp_GetLLSingle(pa)	g_Exp.GetSingle(pa)
#define Exp_GetVal(pa)		static_cast<int>(g_Exp.GetVal(pa))
#define Exp_GetLLVal(pa)	g_Exp.GetVal(pa)
#define Exp_GetRange(pa)	g_Exp.GetRange(pa)

#endif	// _INC_CEXPRSSION_H
