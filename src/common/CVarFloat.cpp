#include "../graysvr/graysvr.h"
#include <numbers>
#include <random>

typedef double RealType;

CVarFloat::CVarFloat()
{
}

CVarFloat::~CVarFloat()
{
	m_VarMap.clear();
}

bool CVarFloat::Insert(LPCTSTR pszName, LPCTSTR pszValue, bool fForceSet)
{
	ADDTOCALLSTACK("CVarFloat::Insert");
	if ( !pszName || !pszValue )
		return false;

	MapType::iterator i = m_VarMap.find(pszName);
	if ( (i != m_VarMap.end()) && !fForceSet )
		return false;

	SkipArgSeparator(pszName);
	SkipArgSeparator(pszValue);

	char *pchEnd;
	m_VarMap[CGString(pszName)] = static_cast<RealType>(strtod(pszValue, &pchEnd));
	return true;
}

RealType CVarFloat::GetVal(LPCTSTR pszName)
{
	ADDTOCALLSTACK("CVarFloat::GetVal");
	if ( !pszName )
		return 0.0;

	SkipArgSeparator(pszName);

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

	SkipArgSeparator(pszName);

	if ( strlen(pszName) > EXPRESSION_MAX_KEY_LEN )
		return CGString();

	TCHAR szReal[EXPRESSION_MAX_KEY_LEN];
	snprintf(szReal, sizeof(szReal), "%f", GetVal(pszName));
	return CGString(szReal);
}

thread_local int g_MakeFloatMath_LoopCount = 0;

RealType CVarFloat::MakeFloatMath(LPCTSTR &pszArgs)
{
	ADDTOCALLSTACK("CVarFloat::MakeFloatMath");
	if ( !pszArgs )
		return 0.0;

	SkipWhitespace(pszArgs);

	++g_MakeFloatMath_LoopCount;
	if ( g_MakeFloatMath_LoopCount > 128 )
	{
		DEBUG_WARN(("Deadlock detected while parsing '%s'. Fix the error in your scripts\n", pszArgs));
		--g_MakeFloatMath_LoopCount;
		return 0.0;
	}

	RealType dVal = GetValMath(GetSingle(pszArgs), pszArgs);
	--g_MakeFloatMath_LoopCount;
	return dVal;
}

RealType CVarFloat::GetValMath(RealType dVal, LPCTSTR &pszArgs)
{
	ADDTOCALLSTACK("CVarFloat::GetValMath");
	// Look for math operators

	SkipWhitespace(pszArgs);
	switch ( pszArgs[0] )
	{
		case '\0':
			break;
		case ')':	// expression end markers
		case '}':
		case ']':
		{
			++pszArgs;
			break;
		}
		case '+':
		{
			++pszArgs;
			dVal += MakeFloatMath(pszArgs);
			break;
		}
		case '-':
		{
			++pszArgs;
			dVal -= MakeFloatMath(pszArgs);
			break;
		}
		case '*':
		{
			++pszArgs;
			dVal *= MakeFloatMath(pszArgs);
			break;
		}
		case '|':
		{
			++pszArgs;
			DEBUG_ERR(("FloatVal: operator '|' is not allowed\n"));
			break;
		}
		case '&':
		{
			++pszArgs;
			DEBUG_ERR(("FloatVal: operator '&' is not allowed\n"));
			break;
		}
		case '/':
		{
			++pszArgs;
			RealType dArgs = MakeFloatMath(pszArgs);
			if ( dArgs == 0.0 )
			{
				DEBUG_ERR(("FloatVal: can't divide by 0\n"));
				break;
			}
			dVal /= dArgs;
			break;
		}
		case '%':
		{
			++pszArgs;
			DEBUG_ERR(("FloatVal: operator '%%' is not allowed\n"));
			break;
		}
		case '^':
		{
			++pszArgs;
			DEBUG_ERR(("FloatVal: operator '^' is not allowed\n"));
			break;
		}
		case '>':
		{
			++pszArgs;
			if ( pszArgs[0] == '>' )
			{
				++pszArgs;
				DEBUG_ERR(("FloatVal: operator '>>' is not allowed\n"));
			}
			break;
		}
		case '<':
		{
			++pszArgs;
			if ( pszArgs[0] == '<' )
			{
				++pszArgs;
				DEBUG_ERR(("FloatVal: operator '<<' is not allowed\n"));
			}
			break;
		}
		case '@':
		{
			++pszArgs;
			RealType dArgs = MakeFloatMath(pszArgs);
			if ( (dVal == 0.0) && (dArgs < 0.0) )
			{
				DEBUG_ERR(("FloatVal: can't raise zero to a negative power\n"));
				break;
			}
			dVal = static_cast<RealType>(pow(static_cast<double>(dVal), static_cast<double>(dArgs)));
			break;
		}
	}
	return dVal;
}

RealType CVarFloat::GetSingle(LPCTSTR &pszArgs)
{
	ADDTOCALLSTACK("CVarFloat::GetSingle");
	if ( !pszArgs || !*pszArgs )
		return 0.0;

	SkipWhitespace(pszArgs);

	LPCTSTR pszCheck = pszArgs;
	bool fIsNum = false;
	for ( TCHAR ch = *pszCheck; ch; ch = *++pszCheck )
	{
		bool fIsDigit = IsDigit(ch);
		if ( fIsDigit || (ch == '.') || (ch == ',') )
		{
			if ( !fIsNum )
				fIsNum = fIsDigit;
			continue;
		}
		if ( (ch == '&') || ((ch >= ')') && (ch <= '/')) || (ch == '<') || (ch == '>') || (ch == '@') || (ch == ']') || (ch == '|') )
			break;

		fIsNum = false;
		break;
	}

	if ( fIsNum )
	{
		TCHAR *pchEnd;
		RealType dRet = static_cast<RealType>(strtod(pszArgs, &pchEnd));

		if ( pchEnd && (pchEnd != pszArgs) )
			pszArgs = pchEnd;

		return dRet;
	}

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
			return 0.0;
		case ';':	// seperate field
		case ',':	// seperate field
		case '\0':
			return 0.0;
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

			size_t iCount = 0;
			RealType dResult = 0.0;

			switch ( static_cast<INTRINSIC_TYPE>(index) )
			{
				case INTRINSIC_ARCCOS:
				{
					if ( *pszArgs != '\0' )
					{
						iCount = 1;
						dResult = static_cast<RealType>((acos(MakeFloatMath(pszArgs)) * 180.0) / std::numbers::pi);
					}
					break;
				}
				case INTRINSIC_ARCSIN:
				{
					if ( *pszArgs != '\0' )
					{
						iCount = 1;
						dResult = static_cast<RealType>((asin(MakeFloatMath(pszArgs)) * 180.0) / std::numbers::pi);
					}
					break;
				}
				case INTRINSIC_ARCTAN:
				{
					if ( *pszArgs != '\0' )
					{
						iCount = 1;
						dResult = static_cast<RealType>((atan(MakeFloatMath(pszArgs)) * 180.0) / std::numbers::pi);
					}
					break;
				}
				case INTRINSIC_COS:
				{
					if ( *pszArgs != '\0' )
					{
						iCount = 1;
						dResult = static_cast<RealType>(cos((MakeFloatMath(pszArgs) * std::numbers::pi) / 180.0));
					}
					break;
				}
				case INTRINSIC_ID:
				{
					if ( *pszArgs != '\0' )
					{
						iCount = 1;
						dResult = static_cast<RealType>(RES_GET_INDEX(static_cast<int>(MakeFloatMath(pszArgs))));
					}
					break;
				}
				case INTRINSIC_ISNUMBER:
				{
					TCHAR *pchEnd;
					static_cast<void>(strtol(pszArgs, &pchEnd, 10));

					iCount = 1;
					dResult = (*pchEnd == '\0') ? 1.0 : 0.0;
					break;
				}
				case INTRINSIC_ISOBSCENE:
				{
					iCount = 1;
					dResult = g_Cfg.IsObscene(pszArgs) ? 1.0 : 0.0;
					break;
				}
				case INTRINSIC_LOGARITHM:
				{
					TCHAR *ppArgs[2];
					iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
					if ( iCount < 1 )
						break;

					LPCTSTR pszVal = ppArgs[0];
					RealType dVal = MakeFloatMath(pszVal);
					if ( dVal <= 0.0 )
					{
						DEBUG_ERR(("%s(%f): result is %s\n", sm_IntrinsicFunctions[index], dVal, (dVal == 0.0) ? "infinite" : "undefined"));
						break;
					}

					if ( iCount == 2 )
					{
						if ( !strcmpi(ppArgs[1], "e") )
							dResult = static_cast<RealType>(log(static_cast<double>(dVal)));
						else if ( !strcmpi(ppArgs[1], "pi") )
						{
							static const double dLogPi = log(std::numbers::pi);
							dResult = static_cast<RealType>(log(static_cast<double>(dVal)) / dLogPi);
						}
						else
						{
							pszVal = ppArgs[1];
							RealType dBase = MakeFloatMath(pszVal);
							if ( dBase <= 0.0 )
							{
								DEBUG_ERR(("%s(%f, %f): result is %s\n", sm_IntrinsicFunctions[index], dVal, dBase, (dBase == 0.0) ? "infinite" : "undefined"));
								break;
							}
							dResult = static_cast<RealType>(log(static_cast<double>(dVal)) / log(static_cast<double>(dBase)));
						}
					}
					else
						dResult = static_cast<RealType>(log10(static_cast<double>(dVal)));
					break;
				}
				case INTRINSIC_NAPIERPOW:
				{
					if ( *pszArgs != '\0' )
					{
						iCount = 1;
						dResult = static_cast<RealType>(exp(static_cast<double>(MakeFloatMath(pszArgs))));
					}
					break;
				}
				case INTRINSIC_QVAL:
				{
					TCHAR *ppArgs[5];
					iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
					if ( iCount < 3 )
						break;

					LPCTSTR pszVal = ppArgs[0];
					RealType dVal1 = GetSingle(pszVal);

					pszVal = ppArgs[1];
					RealType dVal2 = GetSingle(pszVal);

					if ( dVal1 < dVal2 )
					{
						pszVal = ppArgs[2];
						dResult = GetSingle(pszVal);
					}
					else if ( dVal1 == dVal2 )
					{
						if ( iCount >= 4 )
						{
							pszVal = ppArgs[3];
							dResult = GetSingle(pszVal);
						}
					}
					else
					{
						if ( iCount >= 5 )
						{
							pszVal = ppArgs[4];
							dResult = GetSingle(pszVal);
						}
					}
					break;
				}
				case INTRINSIC_RAND:
				{
					TCHAR *ppArgs[2];
					iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
					if ( iCount == 2 )
					{
						LPCTSTR pszMin = ppArgs[0];
						LPCTSTR pszMax = ppArgs[1];
						dResult = GetRandVal(MakeFloatMath(pszMin), MakeFloatMath(pszMax));
					}
					else if ( iCount == 1 )
					{
						LPCTSTR pszMax = ppArgs[0];
						dResult = GetRandVal(0, MakeFloatMath(pszMax));
					}
					break;
				}
				case INTRINSIC_RANDBELL:
				{
					TCHAR *ppArgs[2];
					iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
					if ( iCount == 2 )
					{
						LPCTSTR pszMean = ppArgs[0];
						LPCTSTR pszVariance = ppArgs[1];
						dResult = static_cast<RealType>(Calc_GetBellCurve(static_cast<int>(MakeFloatMath(pszMean)), static_cast<int>(MakeFloatMath(pszVariance))));
					}
					break;
				}
				case INTRINSIC_SIN:
				{
					if ( *pszArgs != '\0' )
					{
						iCount = 1;
						dResult = static_cast<RealType>(sin((MakeFloatMath(pszArgs) * std::numbers::pi) / 180.0));
					}
					break;
				}
				case INTRINSIC_SQRT:
				{
					if ( *pszArgs != '\0' )
					{
						iCount = 1;
						RealType dVal = MakeFloatMath(pszArgs);
						if ( dVal < 0.0 )
						{
							DEBUG_ERR(("%s(%f): can't get square root of negative number\n", sm_IntrinsicFunctions[index], dVal));
							break;
						}
						dResult = static_cast<RealType>(sqrt(static_cast<double>(dVal)));
					}
					break;
				}
				case INTRINSIC_STRASCII:
				{
					if ( *pszArgs != '\0' )
					{
						iCount = 1;
						dResult = pszArgs[0];
					}
					break;
				}
				case INTRINSIC_STRCMP:
				{
					TCHAR *ppArgs[2];
					iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
					if ( iCount == 2 )
						dResult = static_cast<RealType>(strcmp(ppArgs[0], ppArgs[1]));
					else
						dResult = 1.0;
					break;
				}
				case INTRINSIC_STRCMPI:
				{
					TCHAR *ppArgs[2];
					iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
					if ( iCount == 2 )
						dResult = static_cast<RealType>(strcmpi(ppArgs[0], ppArgs[1]));
					else
						dResult = 1.0;
					break;
				}
				case INTRINSIC_STRINDEXOF:
				{
					TCHAR *ppArgs[3];
					iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
					if ( iCount >= 2 )
					{
						size_t iOffset = 0;
						if ( iCount == 3 )
						{
							LPCTSTR pszOffset = ppArgs[2];
							iOffset = static_cast<size_t>(MakeFloatMath(pszOffset));
						}
						dResult = static_cast<RealType>(Str_IndexOf(ppArgs[0], ppArgs[1], iOffset));
					}
					else
						dResult = -1.0;
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
					TCHAR *ppArgs[2];
					iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
					if ( iCount == 2 )
						dResult = (Str_Match(ppArgs[0], ppArgs[1]) == MATCH_VALID) ? 1.0 : 0.0;
					break;
				}
				case INTRINSIC_STRREGEX:
				{
					TCHAR *ppArgs[2];
					iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
					if ( iCount == 2 )
						dResult = (Str_RegExMatch(ppArgs[0], ppArgs[1]) == MATCH_VALID) ? 1.0 : 0.0;
					break;
				}
				case INTRINSIC_TAN:
				{
					if ( *pszArgs != '\0' )
					{
						iCount = 1;
						dResult = static_cast<RealType>(tan((MakeFloatMath(pszArgs) * std::numbers::pi) / 180.0));
					}
					break;
				}
				default:
					break;
			}

			pszArgs = pszArgsNext;

			if ( iCount == 0 )
			{
				DEBUG_ERR(("Bad intrinsic function usage: missing arguments\n"));
				return 0.0;
			}
			return dResult;
		}
	}

	long long lVal;
	if ( g_Exp.m_VarGlobals.GetParseVal(pszArgs, &lVal) )
		return static_cast<RealType>(lVal);
	if ( g_Exp.m_VarDefs.GetParseVal(pszArgs, &lVal) )
		return static_cast<RealType>(lVal);
	return 0.0;
}

RealType CVarFloat::GetRandVal(RealType dMin, RealType dMax)
{
	if ( dMin > dMax )
		std::swap(dMin, dMax);

	thread_local std::random_device rd;
	thread_local std::mt19937 gen(rd());

	std::uniform_real_distribution<RealType> dist(dMin, dMax);
	return dist(gen);
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
	if ( wNumber == 0 )
		return NULL;

	ObjMap::iterator i = m_ObjMap.find(wNumber);
	if ( i == m_ObjMap.end() )
		return NULL;

	return i->second;
}

bool CLocalObjMap::Insert(WORD wNumber, CObjBase *pObj, bool fForceSet)
{
	ADDTOCALLSTACK("CLocalObjMap::Insert");
	if ( wNumber == 0 )
		return false;

	ObjMap::iterator i = m_ObjMap.find(wNumber);
	if ( (i != m_ObjMap.end()) && !fForceSet )
		return false;

	m_ObjMap[wNumber] = pObj;
	return true;
}
