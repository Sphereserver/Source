#ifndef _INC_CEXPRESSION_H
#define _INC_CEXPRESSION_H
#pragma once

#define ISWHITESPACE(ch)				(IsSpace(ch) || ((ch) == 0xA0))
#define _IS_SWITCH(ch)					(((ch) == '-') || ((ch) == '/'))	// command line switch
#define _ISCSYMF(ch)					(IsAlpha(ch) || (ch == '_'))		// __iscsymf
#define _ISCSYM(ch)						(isalnum(ch) || (ch == '_'))		// __iscsym

#define SKIP_SEPARATORS(pszStr)			while (*(pszStr) == '.') { ++(pszStr); }	// || ISWHITESPACE(*(pszStr))
#define SKIP_ARGSEP(pszStr)				while ((*(pszStr) == ',') || IsSpace(*(pszStr))) { ++(pszStr); }
#define SKIP_IDENTIFIERSTRING(pszStr)	while (_ISCSYM(*(pszStr))) { ++(pszStr); }

#define GETNONWHITESPACE(pszStr)		while (ISWHITESPACE(*(pszStr))) { ++(pszStr); }

#define REMOVE_QUOTES(x)	\
{							\
	GETNONWHITESPACE(x);	\
	if (*x == '"')			\
		++x;				\
	TCHAR *pszX = const_cast<TCHAR *>(strchr(x, '"'));	\
	if (pszX)				\
		*pszX = '\0';		\
}

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
	INTRINSIC_STRINDEXOF,
	INTRINSIC_STRLEN,
	INTRINSIC_STRMATCH,
	INTRINSIC_STRREGEX,
	INTRINSIC_TAN,
	INTRINSIC_QTY
};

static const LPCTSTR sm_IntrinsicFunctions[INTRINSIC_QTY + 1] =
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
	static const TCHAR sm_szMessages[DEFMSG_QTY][EXPRESSION_MAX_KEY_LEN];		// like: "You put %s to %s"
	static const LPCTSTR sm_szMsgNames[DEFMSG_QTY];		// like: "put_it"

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

	// Evaluate using the stuff we know
	INT64 GetSingle(LPCTSTR &pszArgs);
	INT64 GetValMath(INT64 iVal, LPCTSTR &pszArgs);
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

extern bool IsStrEmpty(LPCTSTR pszArgs);
extern bool IsStrNumericDec(LPCTSTR pszArgs);
extern bool IsStrNumeric(LPCTSTR pszArgs);
extern bool IsSimpleNumberString(LPCTSTR pszArgs);
extern bool IsValidDef(LPCTSTR pszArgs);
extern bool IsValidGameObjDef(LPCTSTR pszArgs);

// Numeric formulas
extern int Calc_GetRandVal(int iMin, int iMax);
extern inline int Calc_GetRandVal(int iVal)
{
	return (iVal > 1) ? Calc_GetRandVal(0, iVal - 1) : 0;
}

extern INT64 Calc_GetRandLLVal(INT64 iMin, INT64 iMax);
extern inline INT64 Calc_GetRandLLVal(INT64 iVal)
{
	return (iVal > 1) ? Calc_GetRandLLVal(0, iVal - 1) : 0;
}

extern int Calc_GetBellCurve(int iMean, int iVariance);
extern int Calc_GetSCurve(int iMean, int iVariance);

extern int ahextoi(LPCTSTR pszArgs);		// convert hex string to int
extern INT64 ahextoi64(LPCTSTR pszArgs);	// convert hex string to int64

#define Exp_GetSingle(pa)	static_cast<int>(g_Exp.GetSingle(pa))
#define Exp_GetLLSingle(pa)	g_Exp.GetSingle(pa)
#define Exp_GetVal(pa)		static_cast<int>(g_Exp.GetVal(pa))
#define Exp_GetLLVal(pa)	g_Exp.GetVal(pa)
#define Exp_GetRange(pa)	g_Exp.GetRange(pa)

#endif	// _INC_CEXPRESSION_H
