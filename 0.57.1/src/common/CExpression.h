#ifndef _INC_CEXPRSSION_H
#define _INC_CEXPRSSION_H
#pragma once

#include "VariableList.h"

#define _ISCSYMF(ch) ( isalpha(ch) || (ch)=='_')	// __iscsym or __iscsymf
#define _ISCSYM(ch) ( isalnum(ch) || (ch)=='_')	// __iscsym or __iscsymf

enum DEFMSG_TYPE
{
	#define MSG(a,b,c) DEFMSG_##a,
	#include "../tables/defmessages.tbl"
	#undef MSG
	DEFMSG_QTY,
};

extern class CExpression
{
public:
	static const char *m_sClassName;
	VariableList	m_VarDefs;		// Defined variables in sorted order.
	VariableList	m_VarGlobals;	// Global variables
	CGString		m_sTmp;

								//	defined default messages
	static TCHAR sm_szMessages[DEFMSG_QTY+1][128];			// like: "You put %s to %s"
	static LPCTSTR const sm_szMsgNames[DEFMSG_QTY+1];		// like: "put_it"

public:
	// Strict G++ Prototyping produces an error when not casting char*& to const char*&
	// So this is a rather lazy workaround
	inline int GetSingle( LPSTR &pArgs )
	{
		return GetSingle( (LPCTSTR&)pArgs );
	}

	inline int GetRange( LPSTR &pArgs )
	{
		return GetRange( (LPCTSTR&)pArgs );
	}

	inline int GetRangeVals( LPSTR &pExpr, int * piVals, int iMaxQty )
	{
		return GetRangeVals( (LPCTSTR&)pExpr, piVals, iMaxQty );
	}

	inline int GetVal( LPSTR &pArgs )
	{
		return GetVal( (LPCTSTR&)pArgs );
	}

	// Evaluate using the stuff we know.
	int GetSingle( LPCTSTR & pArgs );
	int GetVal( LPCTSTR & pArgs );
	int GetValMath( int lVal, LPCTSTR & pExpr );
	int GetRangeVals( LPCTSTR & pExpr, int * piVals, int iMaxQty );
	int GetRange( LPCTSTR & pArgs );

	CExpression();
	~CExpression();

} g_Exp;

extern bool IsSimpleNumberString( LPCTSTR pszTest );
extern bool IsStrNumericDec( LPCTSTR pszTest );
extern bool IsStrNumeric( LPCTSTR pszTest );
extern bool IsStrEmpty( LPCTSTR pszTest );

// Numeric formulas
extern int Calc_GetRandVal( int iqty );
extern int Calc_GetRandVal2( int iMin, int iMax );
extern int Calc_GetLog2( UINT iVal );
extern int Calc_GetSCurve( int iValDiff, int iVariance );
extern int Calc_GetBellCurve( int iValDiff, int iVariance );

#define Exp_GetSingle( pa ) g_Exp.GetSingle( pa )
#define Exp_GetVal( pa )	g_Exp.GetVal( pa )
#define Exp_GetRange( pa )	g_Exp.GetRange( pa )

#endif	// _INC_CEXPRSSION_H