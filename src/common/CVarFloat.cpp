#include "../graysvr/graysvr.h"
#include <cmath>

typedef double RealType;
#define VARDEF_FLOAT_MAXBUFFERSIZE 82

CVarFloat::CVarFloat()
{
}

CVarFloat::~CVarFloat()
{
	m_VarMap.clear();
}

inline bool CVarFloat::Set(LPCTSTR pszName, LPCTSTR pszValue)
{
	ADDTOCALLSTACK("CVarFloat::Set");
	return Insert(pszName, pszValue, true);
}

bool CVarFloat::Insert(LPCTSTR pszName, LPCTSTR pszValue, bool fForceSet)
{
	ADDTOCALLSTACK("CVarFloat::Insert");
	if ( !pszName || !pszValue )
		return false;

	MapType::iterator i = m_VarMap.find(pszName);
	if ( (i != m_VarMap.end()) && !fForceSet )
		return false;

	SKIP_ARGSEP(pszValue);
	SKIP_ARGSEP(pszName);
	char *pchEnd;
	m_VarMap[CGString(pszName)] = static_cast<RealType>(strtod(pszValue, &pchEnd));
	return true;
}

RealType CVarFloat::GetVal(LPCTSTR pszName)
{
	ADDTOCALLSTACK("CVarFloat::GetVal");
	if ( !pszName )
		return 0.0;

	SKIP_ARGSEP(pszName);
	MapType::iterator i = m_VarMap.find(pszName);
	if ( i == m_VarMap.end() )
		return 0.0;

	return i->second;
}

CGString CVarFloat::Get(LPCTSTR pszName)
{
	ADDTOCALLSTACK("CVarFloat::Get");
	if ( !pszName )
		return CGString();

	SKIP_ARGSEP(pszName);
	if ( strlen(pszName) > VARDEF_FLOAT_MAXBUFFERSIZE )
		return CGString();

	char chReal[VARDEF_FLOAT_MAXBUFFERSIZE];
	sprintf(chReal, "%f", static_cast<RealType>(GetVal(pszName)));
	return CGString(chReal);
}

short int Reentrant_Count = 0;

CGString CVarFloat::FloatMath(LPCTSTR &pszExpr)
{
	ADDTOCALLSTACK("CVarFloat::FloatMath");
	char chReal[VARDEF_FLOAT_MAXBUFFERSIZE];
	sprintf(chReal, "%f", MakeFloatMath(pszExpr));
	return CGString(chReal);
}

RealType CVarFloat::MakeFloatMath(LPCTSTR &pszExpr)
{
	ADDTOCALLSTACK("CVarFloat::MakeFloatMath");
	if ( !pszExpr )
		return 0;

	GETNONWHITESPACE(pszExpr);

	++Reentrant_Count;
	if ( Reentrant_Count > 128 )
	{
		DEBUG_WARN(("Deadlock detected while parsing '%s'. Fix the error in your scripts\n", pszExpr));
		--Reentrant_Count;
		return 0;
	}

	RealType dVal = GetValMath(GetSingle(pszExpr), pszExpr);
	--Reentrant_Count;
	return dVal;
}

RealType CVarFloat::GetValMath(RealType dVal, LPCTSTR &pszExpr)
{
	ADDTOCALLSTACK("CVarFloat::GetValMath");
	GETNONWHITESPACE(pszExpr);

	// Look for math type operator
	switch ( pszExpr[0] )
	{
		case '\0':
			break;
		case ')':	// expression end markers
		case '}':
		case ']':
			++pszExpr;
			break;
		case '+':
			++pszExpr;
			dVal += MakeFloatMath(pszExpr);
			break;
		case '-':
			++pszExpr;
			dVal -= MakeFloatMath(pszExpr);
			break;
		case '*':
			++pszExpr;
			dVal *= MakeFloatMath(pszExpr);
			break;
		case '/':
		{
			++pszExpr;
			RealType dTempVal = MakeFloatMath(pszExpr);
			if ( dTempVal == 0 )
			{
				DEBUG_ERR(("FloatVal: can't divide by 0\n"));
				break;
			}
			dVal /= dTempVal;
			break;
		}
		case '!':
			++pszExpr;
			if ( pszExpr[0] != '=' )
				break;	// boolean ! is handled as a single expresion
			++pszExpr;
			dVal = static_cast<RealType>(dVal != MakeFloatMath(pszExpr));
			break;
		case '=':	// boolean
			while ( pszExpr[0] == '=' )
				++pszExpr;
			dVal = static_cast<RealType>(dVal == MakeFloatMath(pszExpr));
			break;
		case '@':
		{
			++pszExpr;
			RealType dTempVal = MakeFloatMath(pszExpr);
			if ( (dVal == 0) && (dTempVal < 0) )
			{
				DEBUG_ERR(("FloatVal: power of zero with negative exponent is undefined\n"));
				break;
			}
			dVal = pow(dVal, dTempVal);
			break;
		}
		// Following operations are not allowed with double
		case '|':
			++pszExpr;
			if ( pszExpr[0] == '|' )	// boolean ?
			{
				++pszExpr;
				dVal = static_cast<RealType>(MakeFloatMath(pszExpr) || dVal);
			}
			else	// bitwise
				DEBUG_ERR(("FloatVal: operator '|' is not allowed\n"));
			break;
		case '&':
			++pszExpr;
			if ( pszExpr[0] == '&' )	// boolean ?
			{
				++pszExpr;
				dVal = static_cast<RealType>(MakeFloatMath(pszExpr) && dVal);	// tricky stuff here, logical ops must come first or possibly not get processed
			}
			else	// bitwise
				DEBUG_ERR(("FloatVal: operator '&' is not allowed\n"));
			break;
		case '%':
			++pszExpr;
			DEBUG_ERR(("FloatVal: operator '%' is not allowed\n"));
			break;
		case '^':
			++pszExpr;
			DEBUG_ERR(("FloatVal: operator '^' is not allowed\n"));
			break;
		case '>':	// boolean
			++pszExpr;
			if ( pszExpr[0] == '=' )	// boolean ?
			{
				++pszExpr;
				dVal = static_cast<RealType>(dVal >= MakeFloatMath(pszExpr));
			}
			else if ( pszExpr[0] == '>' )	// shift
			{
				++pszExpr;
				DEBUG_ERR(("FloatVal: operator '>>' is not allowed\n"));
			}
			else
				dVal = static_cast<RealType>(dVal > MakeFloatMath(pszExpr));
			break;
		case '<':	// boolean
			++pszExpr;
			if ( pszExpr[0] == '=' )	// boolean ?
			{
				++pszExpr;
				dVal = static_cast<RealType>(dVal <= MakeFloatMath(pszExpr));
			}
			else if ( pszExpr[0] == '<' )	// shift
			{
				++pszExpr;
				DEBUG_ERR(("FloatVal: operator '<<' is not allowed\n"));
			}
			else
				dVal = static_cast<RealType>(dVal < MakeFloatMath(pszExpr));
			break;
	}
	return dVal;
}

RealType CVarFloat::GetSingle(LPCTSTR &pszArgs)
{
	ADDTOCALLSTACK("CVarFloat::GetSingle");
	if ( !pszArgs || !*pszArgs )
		return 0;

	GETNONWHITESPACE(pszArgs);
	char *pchArgsCopy = new char[strlen(pszArgs) + 1];
	strcpy(pchArgsCopy, pszArgs);

	bool fIsNum = false;
	for ( TCHAR ch = static_cast<TCHAR>(tolower(*pszArgs)); ch; ch = static_cast<TCHAR>(tolower(*(++pszArgs))) )
	{
		if ( (IsDigit(ch)) || (ch == '.') || (ch == ',') )
		{
			if ( !fIsNum )
				fIsNum = (IsDigit(ch) != 0);
			continue;
		}
		if ( ((ch >= '*') && (ch <= '/')) || ((ch == ')') || (ch == ']')) || (ch == '@') )
			break;

		fIsNum = false;
		break;
	}

	if ( fIsNum )
	{
		char *pchEnd;
		RealType ret = strtod(pchArgsCopy, &pchEnd);
		delete[] pchArgsCopy;
		return ret;
	}
	delete[] pchArgsCopy;

	switch ( pszArgs[0] )
	{
		case '{':
		case '[':
		case '(':	// parse out a sub expression
			++pszArgs;
			return MakeFloatMath(pszArgs);
		case '+':
			++pszArgs;
			break;
		case '-':
			++pszArgs;
			return -GetSingle(pszArgs);
		case '~':	// bitwise not
			++pszArgs;
			DEBUG_ERR(("FloatVal: operator '~' is not allowed\n"));
			return 0;
		case '!':	// boolean not
			++pszArgs;
			if ( pszArgs[0] == '=' )	// odd condition such as (!=x) which is always true of course
			{
				++pszArgs;	// so just skip it, and compare it to 0
				return GetSingle(pszArgs);
			}
			return !GetSingle(pszArgs);
		case ';':	// seperate field
		case ',':	// seperate field
		case '\0':
			return 0;
	}

	int index = FindTableHeadSorted(pszArgs, sm_IntrinsicFunctions, COUNTOF(sm_IntrinsicFunctions) - 1);
	if ( index >= 0 )
	{
		size_t iLen = strlen(sm_IntrinsicFunctions[index]);
		if ( pszArgs[iLen] == '(' )
		{
			pszArgs += (iLen + 1);
			TCHAR *pszArgsNext;
			Str_Parse(const_cast<TCHAR *>(pszArgs), &pszArgsNext, ")");

			size_t iCount;
			RealType dResult;

			switch ( static_cast<INTRINSIC_TYPE>(index) )
			{
				case INTRINSIC_ARCCOS:
				{
					if ( *pszArgs != '\0' )
					{
						iCount = 1;
						dResult = acos(MakeFloatMath(pszArgs)) * 180 / M_PI;
					}
					else
					{
						iCount = 0;
						dResult = 0;
					}
					break;
				}
				case INTRINSIC_ARCSIN:
				{
					if ( *pszArgs != '\0' )
					{
						iCount = 1;
						dResult = asin(MakeFloatMath(pszArgs)) * 180 / M_PI;
					}
					else
					{
						iCount = 0;
						dResult = 0;
					}
					break;
				}
				case INTRINSIC_ARCTAN:
				{
					if ( *pszArgs != '\0' )
					{
						iCount = 1;
						dResult = atan(MakeFloatMath(pszArgs)) * 180 / M_PI;
					}
					else
					{
						iCount = 0;
						dResult = 0;
					}
					break;
				}
				case INTRINSIC_COS:
				{
					if ( *pszArgs != '\0' )
					{
						iCount = 1;
						dResult = cos(MakeFloatMath(pszArgs) * M_PI / 180);
					}
					else
					{
						iCount = 0;
						dResult = 0;
					}
					break;
				}
				case INTRINSIC_ID:
				{
					if ( *pszArgs != '\0' )
					{
						iCount = 1;
						dResult = RES_GET_INDEX(static_cast<int>(MakeFloatMath(pszArgs)));
					}
					else
					{
						iCount = 0;
						dResult = 0;
					}
					break;
				}
				case INTRINSIC_ISNUMBER:
				{
					iCount = 1;
					{
						char ch[64];
						LTOA(atol(pszArgs), ch, 10);
						dResult = strcmp(pszArgs, ch) ? 0 : 1;
					}
					break;
				}
				case INTRINSIC_ISOBSCENE:
				{
					iCount = 1;
					dResult = g_Cfg.IsObscene(pszArgs);
					break;
				}
				case INTRINSIC_LOGARITHM:
				{
					TCHAR *ppCmd[3];
					iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppCmd, COUNTOF(ppCmd), ",");
					if ( iCount < 1 )
					{
						dResult = 0;
						break;
					}

					LPCTSTR pszCmd = ppCmd[0];
					RealType dArgument = MakeFloatMath(pszCmd);

					if ( iCount < 2 )
						dResult = log10(dArgument);
					else
					{
						if ( !strcmpi(ppCmd[1], "e") )
							dResult = log(dArgument);
						else if ( !strcmpi(ppCmd[1], "pi") )
							dResult = log(dArgument) / log(M_PI);
						else
						{
							pszCmd = ppCmd[1];
							RealType dBase = MakeFloatMath(pszCmd);
							if ( dBase <= 0 )
							{
								DEBUG_ERR(("%s: (%f)Log(%f) is %s\n", sm_IntrinsicFunctions[index], dBase, dArgument, !dBase ? "infinite" : "undefined"));
								iCount = 0;
								dResult = 0;
							}
							else
								dResult = log(dArgument) / log(dBase);
						}
					}
					break;
				}
				case INTRINSIC_NAPIERPOW:
				{
					if ( *pszArgs != '\0' )
					{
						iCount = 1;
						dResult = exp(MakeFloatMath(pszArgs));
					}
					else
					{
						iCount = 0;
						dResult = 0;
					}
					break;
				}
				case INTRINSIC_QVAL:
				{
					TCHAR *ppCmd[5];
					iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppCmd, COUNTOF(ppCmd), ",");
					if ( iCount < 3 )
						dResult = 0;
					else
					{
						LPCTSTR pszArg1 = ppCmd[0];
						LPCTSTR pszArg2 = ppCmd[1];
						RealType dVal1 = GetSingle(pszArg1);
						RealType dVal2 = GetSingle(pszArg2);
						if ( dVal1 < dVal2 )
						{
							pszArg1 = ppCmd[2];
							dResult = GetSingle(pszArg1);
						}
						else if ( dVal1 == dVal2 )
						{
							pszArg1 = ppCmd[3];
							dResult = (iCount >= 4) ? GetSingle(pszArg1) : 0;
						}
						else
						{
							pszArg1 = ppCmd[4];
							dResult = (iCount >= 5) ? GetSingle(pszArg1) : 0;
						}
					}
					break;
				}
				case INTRINSIC_RAND:
				{
					TCHAR *ppCmd[2];
					iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppCmd, COUNTOF(ppCmd), ",");
					if ( iCount <= 0 )
						dResult = 0;
					else
					{
						LPCTSTR pszArg1 = ppCmd[0];
						RealType dVal1 = MakeFloatMath(pszArg1);
						if ( iCount >= 2 )
						{
							LPCTSTR pszArg2 = ppCmd[1];
							RealType dVal2 = MakeFloatMath(pszArg2);
							dResult = GetRandVal2(dVal1, dVal2);
						}
						else
							dResult = GetRandVal(dVal1);
					}
					break;
				}
				case INTRINSIC_RANDBELL:
				{
					TCHAR *ppCmd[2];
					iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppCmd, COUNTOF(ppCmd), ",");
					if ( iCount < 2 )
						dResult = 0;
					else
					{
						LPCTSTR pszArg1 = ppCmd[0];
						LPCTSTR pszArg2 = ppCmd[1];
						dResult = Calc_GetBellCurve(static_cast<int>(MakeFloatMath(pszArg1)), static_cast<int>(MakeFloatMath(pszArg2)));
					}
					break;
				}
				case INTRINSIC_SIN:
				{
					if ( *pszArgs != '\0' )
					{
						iCount = 1;
						dResult = sin(MakeFloatMath(pszArgs) * M_PI / 180);
					}
					else
					{
						iCount = 0;
						dResult = 0;
					}
					break;
				}
				case INTRINSIC_SQRT:
				{
					iCount = 0;
					if ( *pszArgs != '\0' )
					{
						RealType dTosquare = MakeFloatMath(pszArgs);
						if ( dTosquare >= 0 )
						{
							++iCount;
							dResult = sqrt(dTosquare);
						}
						else
						{
							DEBUG_ERR(("%s(%f): can't get square root of negative number\n", sm_IntrinsicFunctions[index], dTosquare));
							dResult = 0;
						}
					}
					else
						dResult = 0;
					break;
				}
				case INTRINSIC_STRASCII:
				{
					if ( *pszArgs != '\0' )
					{
						iCount = 1;
						dResult = pszArgs[0];
					}
					else
					{
						iCount = 0;
						dResult = 0;
					}
					break;
				}
				case INTRINSIC_STRCMP:
				{
					TCHAR *ppCmd[2];
					iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppCmd, COUNTOF(ppCmd), ",");
					if ( iCount < 2 )
						dResult = 1;
					else
						dResult = strcmp(ppCmd[0], ppCmd[1]);
					break;
				}
				case INTRINSIC_STRCMPI:
				{
					TCHAR *ppCmd[2];
					iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppCmd, COUNTOF(ppCmd), ",");
					if ( iCount < 2 )
						dResult = 1;
					else
						dResult = strcmpi(ppCmd[0], ppCmd[1]);
					break;
				}
				case INTRINSIC_STRINDEXOF:
				{
					TCHAR *ppCmd[3];
					iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppCmd, COUNTOF(ppCmd), ",");
					if ( iCount < 2 )
						dResult = -1;
					else
					{
						LPCTSTR pszArg = ppCmd[2];
						dResult = Str_IndexOf(ppCmd[0], ppCmd[1], (iCount == 3) ? static_cast<int>(MakeFloatMath(pszArg)) : 0);
					}
					break;
				}
				case INTRINSIC_STRLEN:
				{
					iCount = 1;
					dResult = static_cast<RealType>(strlen(pszArgs));
					break;
				}
				case INTRINSIC_STRMATCH:
				{
					TCHAR *ppCmd[2];
					iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppCmd, COUNTOF(ppCmd), ",");
					if ( iCount < 2 )
						dResult = 0;
					else
						dResult = (Str_Match(ppCmd[0], ppCmd[1]) == MATCH_VALID) ? 1 : 0;
					break;
				}
				case INTRINSIC_STRREGEX:
				{
					TCHAR *ppCmd[2];
					iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppCmd, COUNTOF(ppCmd), ",");
					if ( iCount < 2 )
						dResult = 0;
					else
					{
						TCHAR *pszLastError = Str_GetTemp();
						dResult = Str_RegExMatch(ppCmd[0], ppCmd[1], pszLastError);
						if ( dResult == -1 )
							DEBUG_ERR(("%s: %s\n", sm_IntrinsicFunctions[index], pszLastError));
					}
					break;
				}
				case INTRINSIC_TAN:
				{
					if ( *pszArgs != '\0' )
					{
						iCount = 1;
						dResult = tan(MakeFloatMath(pszArgs) * M_PI / 180);
					}
					else
					{
						iCount = 0;
						dResult = 0;
					}
					break;
				}
				default:
				{
					iCount = 0;
					dResult = 0;
					break;
				}
			}

			pszArgs = pszArgsNext;

			if ( iCount <= 0 )
			{
				DEBUG_ERR(("Bad intrinsic function usage: Missing ')'\n"));
				return 0;
			}
			return dResult;
		}
	}

	long long lVal;
	if ( g_Exp.m_VarGlobals.GetParseVal(pszArgs, &lVal) )
		return static_cast<RealType>(lVal);
	if ( g_Exp.m_VarDefs.GetParseVal(pszArgs, &lVal) )
		return static_cast<RealType>(lVal);
	return 0;
}

RealType CVarFloat::GetRandVal(RealType dQty)
{
	ADDTOCALLSTACK("CVarFloat::GetRandVal");
	if ( dQty <= 0 )
		return 0;
	if ( dQty >= INT_MAX )
		return static_cast<RealType>(IMULDIV(g_World.m_Rand.randDblExc(), dQty, INT_MAX));
	return g_World.m_Rand.randDblExc(dQty);
}

RealType CVarFloat::GetRandVal2(RealType dMin, RealType dMax)
{
	ADDTOCALLSTACK("CVarFloat::GetRandVal2");
	if ( dMin > dMax )
	{
		RealType tmp = dMin;
		dMin = dMax;
		dMax = tmp;
	}
	return (dMin + g_World.m_Rand.randDblExc(dMax));	// these weird numbers are taken from mtrand.h (cause calling that function from here spits out some weird external errors)
}

///////////////////////////////////////////////////////////
// CLocalObjMap

CLocalObjMap::CLocalObjMap()
{
}

CLocalObjMap::~CLocalObjMap()
{
	m_ObjMap.clear();
}

CObjBase *CLocalObjMap::Get(WORD wNumber)
{
	ADDTOCALLSTACK("CLocalObjMap::Get");
	if ( !wNumber )
		return NULL;

	ObjMap::iterator i = m_ObjMap.find(wNumber);
	if ( i == m_ObjMap.end() )
		return NULL;

	return i->second;
}

bool CLocalObjMap::Insert(WORD wNumber, CObjBase *pObj, bool fForceSet)
{
	ADDTOCALLSTACK("CLocalObjMap::Insert");
	if ( !wNumber )
		return false;

	ObjMap::iterator i = m_ObjMap.find(wNumber);
	if ( (i != m_ObjMap.end()) && !fForceSet )
		return false;

	m_ObjMap[wNumber] = pObj;
	return true;
}
