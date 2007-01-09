//
// CExpression.h
// Copyright Menace Software (www.menasoft.com).
//

#ifndef _INC_CEXPRSSION_H
#define _INC_CEXPRSSION_H
#pragma once

#include "CAtom.h"

#define _ISCSYMF(ch) ( isalpha(ch) || (ch)=='_')	// __iscsym or __iscsymf
#define _ISCSYM(ch) ( isalnum(ch) || (ch)=='_')	// __iscsym or __iscsymf

#ifndef M_PI 
	#define M_PI 3.14159265358979323846
#endif

class CVarDefBase	// A variable from GRAYDEFS.SCP or other.
{
	// Similar to CScriptKey
private:
#define EXPRESSION_MAX_KEY_LEN SCRIPT_MAX_SECTION_LEN
	const CAtomRef m_aKey;	// the key for sorting/ etc.
public:
	static const char *m_sClassName;
	LPCTSTR GetKey() const
	{
		return( m_aKey.GetStr() );
	}
	CVarDefBase( LPCTSTR pszKey ) :
		m_aKey( pszKey )
	{
	}
	virtual LPCTSTR GetValStr() const = 0;
	virtual int GetValNum() const = 0;
	virtual CVarDefBase * CopySelf() const = 0;
};

class CVarDefNum : public CVarDefBase
{
	// Simple number equiv.
private:
	int m_iVal;	// the assigned value.
public:
	static const char *m_sClassName;
	int GetValNum() const
	{
		return( m_iVal );
	}
	void SetValNum( int iVal )
	{
		m_iVal = iVal;
	}
	LPCTSTR GetValStr() const;
	bool r_LoadVal( CScript & s )
	{
		SetValNum( s.GetArgVal());
		return( true );
	}
	bool r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc = NULL )
	{
		UNREFERENCED_PARAMETER(pKey);
		UNREFERENCED_PARAMETER(pSrc);
		sVal.FormatVal( GetValNum());
		return( true );
	}
	virtual CVarDefBase * CopySelf() const
	{
		return new CVarDefNum( GetKey(), m_iVal );
	}
	CVarDefNum( LPCTSTR pszKey, int iVal ) :
		CVarDefBase( pszKey ),
		m_iVal( iVal )
	{
	}
	CVarDefNum( LPCTSTR pszKey ) :
		CVarDefBase( pszKey )
	{
	}
};

class CVarDefStr : public CVarDefBase
{
private:
	CGString m_sVal;	// the assigned value. (What if numeric?)
public:
	static const char *m_sClassName;
	LPCTSTR GetValStr() const
	{
		return( m_sVal );
	}
	int GetValNum() const;
	void SetValStr( LPCTSTR pszVal )
	{
		m_sVal.Copy( pszVal );
	}
	bool r_LoadVal( CScript & s )
	{
		SetValStr( s.GetArgStr());
		return( true );
	}
	bool r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc = NULL )
	{
		UNREFERENCED_PARAMETER(pKey);
		UNREFERENCED_PARAMETER(pSrc);
		sVal = GetValStr();
		return( true );
	}
	virtual CVarDefBase * CopySelf() const
	{
		return new CVarDefStr( GetKey(), m_sVal );
	}
	CVarDefStr( LPCTSTR pszKey, LPCTSTR pszVal ) :
		CVarDefBase( pszKey ),
		m_sVal( pszVal )
	{
	}
	CVarDefStr( LPCTSTR pszKey ) :
		CVarDefBase( pszKey )
	{
	}
};

struct CVarDefArray : public CGObSortArray< CVarDefBase *, LPCTSTR>
{
	// Sorted array
protected:
	int CompareKey( LPCTSTR pKey, CVarDefBase * pVar, bool fNoSpaces ) const
	{
		ASSERT(pVar);
		return strcmpi(pKey, pVar->GetKey());
	}
	int Add(CVarDefBase *pVar)
	{
		return AddSortKey(pVar, pVar->GetKey());
	}

public:
	void Copy( const CVarDefArray * pArray );

	CVarDefArray & operator = ( const CVarDefArray & array )
	{
		Copy( &array );
		return( *this );
	}

	~CVarDefArray()
	{
		Clean(true);
		SetCount(0);
	}

	int FindValNum( int iVal ) const;
	int FindValStr( LPCTSTR pVal ) const;

	// Manipulate the list of Vars
	CVarDefBase * GetKey( LPCTSTR pszKey ) const;
	int GetKeyNum( LPCTSTR pszKey, bool fZero = false ) const;
	LPCTSTR GetKeyStr( LPCTSTR pszKey, bool fZero = false ) const;

	CVarDefBase * GetParseKey( LPCTSTR & pArgs ) const;
	bool GetParseVal( LPCTSTR & pArgs, long * plVal ) const;

	int SetNumNew( LPCTSTR pszKey, int iVal );

	int SetNum( LPCTSTR pszKey, int iVal, bool fZero = false );
	int SetStr( LPCTSTR pszKey, bool fQuoted, LPCTSTR pszVal, bool fZero = false );

	bool r_LoadVal( CScript & s )
	{
		bool fQuoted = false;
		return SetStr( s.GetKey(), fQuoted, s.GetArgStr( &fQuoted )) ? true : false;
	}
	void r_WritePrefix( CScript & s, LPCTSTR pszPrefix );
	void DumpKeys( CTextConsole * pSrc, LPCTSTR pszPrefix = NULL );
	void ClearKeys(LPCTSTR mask = NULL);
};

enum DEFMSG_TYPE
{
	#define MSG(a,b,c) DEFMSG_##a,
	#include "../tables/defmessages.tbl"
	#undef MSG
	DEFMSG_QTY,
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
	INTRINSIC_QTY,
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
	NULL,
};

extern class CExpression
{
public:
	static const char *m_sClassName;
	CVarDefArray	m_VarDefs;		// Defined variables in sorted order.
	CVarDefMap		m_VarGlobals;	// Global variables
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

inline int CVarDefStr::GetValNum() const
{
	LPCTSTR pszStr = m_sVal;
	return( Exp_GetVal(pszStr));
}

inline LPCTSTR CVarDefNum::GetValStr() const
{
	TCHAR * pszTmp = Str_GetTemp();
	sprintf(pszTmp, "0%x", m_iVal);
	return pszTmp;
}

#endif	// _INC_CEXPRSSION_H