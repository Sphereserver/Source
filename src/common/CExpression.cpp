#include <cmath>
#include "../graysvr/graysvr.h"

TCHAR CExpression::sm_szMessages[DEFMSG_QTY][128] =
{
	#define MSG(a,b) b,
	#include "../tables/defmessages.tbl"
};

LPCTSTR const CExpression::sm_szMsgNames[DEFMSG_QTY] =
{
	#define MSG(a,b) #a,
	#include "../tables/defmessages.tbl"
};

DWORD ahextoi(LPCTSTR pszArgs)	// convert hex string to int
{
	// Unfortunately the library func can't handle the number 0xFFFFFFFF
	// TCHAR *sstop; return strtol(s, &sstop, 16);

	if ( pszArgs == NULL )
		return 0;

	GETNONWHITESPACE(pszArgs);

	bool bHex = false;
	if ( *pszArgs == '0' )
	{
		if ( *++pszArgs != '.' )
			bHex = true;
		pszArgs--;
	}

	DWORD val = 0;
	for (;;)
	{
		TCHAR ch = static_cast<TCHAR>(toupper(*pszArgs));
		if ( IsDigit(ch) )
			ch -= '0';
		else if ( bHex && (ch >= 'A') && (ch <= 'F') )
			ch -= 'A' - 10;
		else if ( !bHex && (ch == '.') )
		{
			pszArgs++;
			continue;
		}
		else
			break;

		val *= (bHex ? 0x10 : 10);
		val += ch;
		pszArgs++;
	}
	return val;
}

INT64 ahextoi64(LPCTSTR pszArgs)		// convert hex string to int64
{
	if ( pszArgs == NULL )
		return 0;

	GETNONWHITESPACE(pszArgs);

	bool bHex = false;
	if ( *pszArgs == '0' )
	{
		if ( *++pszArgs != '.' )
			bHex = true;
		pszArgs--;
	}

	INT64 val = 0;
	for (;;)
	{
		TCHAR ch = static_cast<TCHAR>(toupper(*pszArgs));
		if ( IsDigit(ch) )
			ch -= '0';
		else if ( bHex && (ch >= 'A') && (ch <= 'F') )
			ch -= 'A' - 10;
		else if ( !bHex && (ch == '.') )
		{
			pszArgs++;
			continue;
		}
		else
			break;

		val *= (bHex ? 0x10 : 10);
		val += ch;
		pszArgs++;
	}
	return val;
}

inline bool IsCharNumeric(char &Test)
{
	if ( !Test )
		return false;
	if ( IsDigit(Test) )
		return true;
	if ( (tolower(Test) >= 'a') && (tolower(Test) <= 'f') )
		return true;

	return false;
}

bool IsStrEmpty(LPCTSTR pszTest)
{
	if ( !pszTest || !*pszTest )
		return true;

	do
	{
		if ( !IsSpace(*pszTest) )
			return false;
	} while ( *(++pszTest) );

	return true;
}

bool IsStrNumericDec(LPCTSTR pszTest)
{
	if ( !pszTest || !*pszTest )
		return false;

	do
	{
		if ( !IsDigit(*pszTest) )
			return false;
	} while ( *(++pszTest) );

	return true;
}


bool IsStrNumeric(LPCTSTR pszTest)
{
	if ( !pszTest || !*pszTest )
		return false;

	bool bHex = false;
	if ( pszTest[0] == '0' )
		bHex = true;

	do
	{
		if ( IsDigit(*pszTest) )
			continue;
		if ( bHex && (tolower(*pszTest) >= 'a') && (tolower(*pszTest) <= 'f') )
			continue;
		return false;
	} while ( *(++pszTest) );

	return true;
}

bool IsSimpleNumberString(LPCTSTR pszTest)
{
	// is this a string or a simple numeric expression ?
	// string = 1 2 3, sdf, sdf sdf sdf, 123d, 123 d,
	// number = 1.0+-\*~|&!%^()2, 0aed, 123

	bool bMathSep = true;	// last non whitespace was a math sep.
	bool bHextDigitStart = false;
	bool bWhiteSpace = false;

	for ( ; ; pszTest++ )
	{
		TCHAR ch = *pszTest;
		if ( !ch )
			return true;

		if ( ((ch >= 'A') && (ch <= 'F')) || ((ch >= 'a') && (ch <= 'f')) )	// isxdigit
		{
			if ( !bHextDigitStart )
				return false;
			bWhiteSpace = false;
			bMathSep = false;
			continue;
		}
		if ( IsSpace(ch) )
		{
			bHextDigitStart = false;
			bWhiteSpace = true;
			continue;
		}
		if ( IsDigit(ch) )
		{
			if ( bWhiteSpace && !bMathSep )
				return false;
			if ( ch == '0' )
				bHextDigitStart = true;
			bWhiteSpace = false;
			bMathSep = false;
			continue;
		}
		if ( (ch == '/') && (pszTest[1] != '/') )
			bMathSep = true;
		else
			bMathSep = strchr("+-\\*~|&!%^()", ch) ? true : false;

		if ( !bMathSep )
			return false;

		bHextDigitStart = false;
		bWhiteSpace = false;
	}
}

static size_t GetIdentifierString(TCHAR *szTag, LPCTSTR pszArgs)
{
	// Copy the identifier (valid char set) out to this buffer.
	size_t i = 0;
	for ( ; pszArgs[i]; i++ )
	{
		if ( !_ISCSYM(pszArgs[i]) )
			break;
		if ( i >= EXPRESSION_MAX_KEY_LEN )
			return 0;
		szTag[i] = pszArgs[i];
	}
	szTag[i] = '\0';
	return i;
}

bool IsValidDef(LPCTSTR pszTest)
{
	CVarDefCont *pVarBase = g_Exp.m_VarDefs.CheckParseKey(pszTest);
	if ( !pVarBase )
	{
		//check VAR.X also
		pVarBase = g_Exp.m_VarGlobals.CheckParseKey(pszTest);
		if ( !pVarBase )
			return false;
	}
	return true;
}

bool IsValidGameObjDef(LPCTSTR pszTest)
{
	if ( !IsSimpleNumberString(pszTest) )
	{
		CVarDefCont *pVarBase = g_Exp.m_VarDefs.CheckParseKey(pszTest);
		if ( !pVarBase )
			return false;

		TCHAR ch = *pVarBase->GetValStr();
		if ( !ch || (ch == '<') )
			return false;

		RESOURCE_ID rid = g_Cfg.ResourceGetID(RES_QTY, pszTest);
		if ( (rid.GetResType() != RES_CHARDEF) && (rid.GetResType() != RES_ITEMDEF) && (rid.GetResType() != RES_SPAWN) && (rid.GetResType() != RES_TEMPLATE) )
			return false;
	}
	return true;
}

///////////////////////////////////////////////////////////
// Numeric formulas

int Calc_GetLog2(UINT iVal)
{
	// This is really log2 + 1
	int i = 0;
	for ( ; iVal; i++ )
	{
		ASSERT(i < 32);
		iVal >>= 1;
	}
	return i;
}

int Calc_GetRandVal(int iQty)
{
	if ( iQty <= 0 )
		return 0;
	if ( iQty >= INT_MAX )
		return IMULDIV(g_World.m_Rand.randInt(), static_cast<DWORD>(iQty), INT_MAX);

	return g_World.m_Rand.randInt() % iQty;
}

int Calc_GetRandVal2(int iMin, int iMax)
{
	if ( iMin > iMax )
	{
		int tmp = iMin;
		iMin = iMax;
		iMax = tmp;
	}
	return iMin + g_World.m_Rand.randInt() % ((iMax - iMin) + 1);
}

INT64 Calc_GetRandLLVal(INT64 iQty)
{
	if ( iQty <= 0 )
		return 0;
	if ( iQty >= LLONG_MAX )
		return IMULDIV(g_World.m_Rand.genrand64_int64(), static_cast<DWORD>(iQty), LLONG_MAX);

	return g_World.m_Rand.genrand64_int64() % iQty;
}

INT64 Calc_GetRandLLVal2(INT64 iMin, INT64 iMax)
{
	if ( iMin > iMax )
	{
		INT64 tmp = iMin;
		iMin = iMax;
		iMax = tmp;
	}
	return iMin + g_World.m_Rand.genrand64_int64() % ((iMax - iMin) + 1);
}

int Calc_GetBellCurve(int iValDiff, int iVariance)
{
	// Produce a log curve.
	//
	// 50+
	//	 |
	//	 |
	//	 |
	// 25|  +
	//	 |
	//	 |     +
	//	 |        +
	//	0 --+--+--+--+------
	//    iVar				iValDiff
	//
	// ARGS:
	//   iValDiff = Given a value relative to 0
	//     0 = 50.0% chance.
	//   iVariance = the 25.0% point of the bell curve
	// RETURN:
	//   (0-100.0) % chance at this iValDiff.
	//   Chance gets smaller as Diff gets bigger.
	// EXAMPLE:
	//   if ( iValDiff == iVariance ) return 250
	//   if ( iValDiff == 0 ) return 500

	if ( iVariance <= 0 )	// this really should not happen but just in case.
		return 500;

	if ( iValDiff < 0 )
		iValDiff = -iValDiff;

	int iChance = 500;
	while ( (iValDiff > iVariance) && iChance )
	{
		iValDiff -= iVariance;
		iChance /= 2;	// chance is halved for each Variance period.
	}

	return iChance - IMULDIV(iChance / 2, iValDiff, iVariance);
}

int Calc_GetSCurve(int iValDiff, int iVariance)
{
	// ARGS:
	//   iValDiff = Difference between our skill level and difficulty.
	//     positive = high chance, negative = lower chance
	//     0 = 50.0% chance.
	//   iVariance = the 25.0% difference point of the bell curve
	// RETURN:
	//   What is the (0-100.0)% chance of success = 0-1000
	// NOTE:
	//   Chance of skill gain is inverse to chance of success.

	int iChance = Calc_GetBellCurve(iValDiff, iVariance);
	if ( iValDiff > 0 )
		return 1000 - iChance;
	return iChance;
}

///////////////////////////////////////////////////////////
// CExpression

CExpression::CExpression()
{
}

CExpression::~CExpression()
{
}

INT64 CExpression::GetSingle(LPCTSTR &pszArgs)
{
	ADDTOCALLSTACK("CExpression::GetSingle");
	// Parse just a single expression without any operators or ranges.
	ASSERT(pszArgs);
	GETNONWHITESPACE(pszArgs);

	LPCTSTR orig = pszArgs;
	if ( pszArgs[0] == '.' )
		pszArgs++;

	if ( pszArgs[0] == '0' )	// leading '0' = hex value.
	{
		// A hex value.
		if ( pszArgs[1] == '.' )	// leading 0. means it really is decimal.
		{
			pszArgs += 2;
			goto try_dec;
		}

		LPCTSTR pStart = pszArgs;
		INT64 iVal = 0;
		for (;;)
		{
			TCHAR ch = *pszArgs;
			if ( IsDigit(ch) )
				ch -= '0';
			else
			{
				ch = static_cast<TCHAR>(tolower(ch));
				if ( (ch > 'f') || (ch < 'a') )
				{
					if ( (ch == '.') && (pStart[0] != '0') )	// ok i'm confused. it must be decimal.
					{
						pszArgs = pStart;
						goto try_dec;
					}
					break;
				}
				ch -= 'a' - 10;
			}
			iVal *= 0x10;
			iVal += ch;
			pszArgs++;
		}
		return iVal;
	}
	else if ( (pszArgs[0] == '.') || IsDigit(pszArgs[0]) )
	{
		// A decminal number
	try_dec:
		INT64 iVal = 0;
		for ( ; ; pszArgs++ )
		{
			if ( *pszArgs == '.' )
				continue;	// just skip this.
			if ( !IsDigit(*pszArgs) )
				break;
			iVal *= 10;
			iVal += *pszArgs - '0';
		}
		return iVal;
	}
	else if ( !_ISCSYMF(pszArgs[0]) )
	{
#pragma region maths
		// some sort of math op ?
		switch ( pszArgs[0] )
		{
			case '{':
				pszArgs++;
				return GetRange(pszArgs);
			case '[':
			case '(': // Parse out a sub expression.
				pszArgs++;
				return GetVal(pszArgs);
			case '+':
				pszArgs++;
				break;
			case '-':
				pszArgs++;
				return -GetSingle(pszArgs);
			case '~':	// Bitwise not.
				pszArgs++;
				return ~GetSingle(pszArgs);
			case '!':	// boolean not.
				pszArgs++;
				if ( pszArgs[0] == '=' )	// odd condition such as (!=x) which is always true of course.
				{
					pszArgs++;		// so just skip it. and compare it to 0
					return GetSingle(pszArgs);
				}
				return !GetSingle(pszArgs);
			case ';':	// seperate field.
			case ',':	// seperate field.
			case '\0':
				return 0;
		}
#pragma endregion maths
	}
	else
#pragma region intrinsics
	{
		// Symbol or intrinsinc function ?
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
				INT64 iResult;

				switch ( static_cast<INTRINSIC_TYPE>(index) )
				{
					case INTRINSIC_ABS:
					{
						iCount = 1;
						iResult = llabs(GetVal(pszArgs));
						break;
					}
					case INTRINSIC_ARCCOS:
					{
						if ( pszArgs )
						{
							iCount = 1;
							iResult = static_cast<INT64>(acos(static_cast<double>(GetVal(pszArgs))));
						}
						else
						{
							iCount = 0;
							iResult = 0;
						}
						break;
					}
					case INTRINSIC_ARCSIN:
					{
						if ( pszArgs )
						{
							iCount = 1;
							iResult = static_cast<INT64>(asin(static_cast<double>(GetVal(pszArgs))));
						}
						else
						{
							iCount = 0;
							iResult = 0;
						}
						break;
					}
					case INTRINSIC_ARCTAN:
					{
						if ( pszArgs )
						{
							iCount = 1;
							iResult = static_cast<INT64>(atan(static_cast<double>(GetVal(pszArgs))));
						}
						else
						{
							iCount = 0;
							iResult = 0;
						}
						break;
					}
					case INTRINSIC_COS:
					{
						if ( pszArgs )
						{
							iCount = 1;
							iResult = static_cast<INT64>(cos(static_cast<double>(GetVal(pszArgs))));
						}
						else
						{
							iCount = 0;
							iResult = 0;
						}
						break;
					}
					case INTRINSIC_ID:
					{
						if ( pszArgs )
						{
							iCount = 1;
							iResult = RES_GET_INDEX(GetVal(pszArgs));
						}
						else
						{
							iCount = 0;
							iResult = 0;
						}
						break;
					}
					case INTRINSIC_ISNUMBER:
					{
						char z[64];
						LTOA(atol(pszArgs), z, 10);

						iCount = 1;
						iResult = strcmp(pszArgs, z) ? 0 : 1;
						break;
					}
					case INTRINSIC_ISOBSCENE:
					{
						iCount = 1;
						iResult = g_Cfg.IsObscene(pszArgs);
						break;
					}
					case INTRINSIC_LOGARITHM:
					{
						iCount = 0;
						iResult = 0;

						if ( pszArgs )
						{
							INT64 iArgument = GetVal(pszArgs);
							if ( iArgument <= 0 )
								DEBUG_ERR(("Exp_GetVal: (x)Log(%lld) is %s\n", iArgument, !iArgument ? "infinite" : "undefined"));
							else
							{
								iCount = 1;
								if ( strchr(pszArgs, ',') )
								{
									iCount++;
									SKIP_ARGSEP(pszArgs);
									if ( !strcmpi(pszArgs, "e") )
										iResult = static_cast<INT64>(log(static_cast<double>(iArgument)));
									else if ( !strcmpi(pszArgs, "pi") )
										iResult = static_cast<INT64>(log(static_cast<double>(iArgument)) / log(M_PI));
									else
									{
										INT64 iBase = GetVal(pszArgs);
										if ( iBase <= 0 )
										{
											DEBUG_ERR(("Exp_GetVal: (%lld)Log(%lld) is %s\n", iBase, iArgument, !iBase ? "infinite" : "undefined"));
											iCount = 0;
										}
										else
											iResult = static_cast<INT64>(log(static_cast<double>(iArgument)) / log(static_cast<double>(iBase)));
									}
								}
								else
									iResult = static_cast<INT64>(log10(static_cast<double>(iArgument)));
							}
						}
						break;
					}
					case INTRINSIC_NAPIERPOW:
					{
						if ( pszArgs )
						{
							iCount = 1;
							iResult = static_cast<INT64>(exp(static_cast<double>(GetVal(pszArgs))));
						}
						else
						{
							iCount = 0;
							iResult = 0;
						}
						break;
					}
					case INTRINSIC_QVAL:
					{
						TCHAR *ppCmd[5];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppCmd, COUNTOF(ppCmd), ",");
						if ( iCount < 3 )
							iResult = 0;
						else
						{
							INT64 a1 = GetSingle(ppCmd[0]);
							INT64 a2 = GetSingle(ppCmd[1]);
							if ( a1 < a2 )
								iResult = GetSingle(ppCmd[2]);
							else if ( a1 == a2 )
								iResult = (iCount >= 4) ? GetSingle(ppCmd[3]) : 0;
							else
								iResult = (iCount >= 5) ? GetSingle(ppCmd[4]) : 0;
						}
						break;
					}
					case INTRINSIC_RAND:
					{
						TCHAR *ppCmd[2];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppCmd, COUNTOF(ppCmd), ",");
						if ( iCount <= 0 )
							iResult = 0;
						else if ( iCount < 2 )
							iResult = Calc_GetRandLLVal(GetVal(ppCmd[0]));
						else
							iResult = Calc_GetRandLLVal2(GetVal(ppCmd[0]), GetVal(ppCmd[1]));
						break;
					}
					case INTRINSIC_RANDBELL:
					{
						TCHAR *ppCmd[2];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppCmd, COUNTOF(ppCmd), ",");
						if ( iCount < 2 )
							iResult = 0;
						else
							iResult = Calc_GetBellCurve(static_cast<int>(GetVal(ppCmd[0])), static_cast<int>(GetVal(ppCmd[1])));
						break;
					}
					case INTRINSIC_SIN:
					{
						if ( pszArgs )
						{
							iCount = 1;
							iResult = static_cast<INT64>(sin(static_cast<double>(GetVal(pszArgs))));
						}
						else
						{
							iCount = 0;
							iResult = 0;
						}
						break;
					}
					case INTRINSIC_SQRT:
					{
						iCount = 0;
						iResult = 0;

						if ( pszArgs )
						{
							INT64 iArgument = GetVal(pszArgs);
							if ( iArgument >= 0 )
							{
								iCount++;
								iResult = static_cast<INT64>(sqrt(static_cast<double>(iArgument)));
							}
							else
								DEBUG_ERR(("Exp_GetVal: Sqrt of negative number (%lld) is impossible\n", iArgument));
						}
						break;
					}
					case INTRINSIC_STRASCII:
					{
						if ( pszArgs )
						{
							iCount = 1;
							iResult = pszArgs[0];
						}
						else
						{
							iCount = 0;
							iResult = 0;
						}
						break;
					}
					case INTRINSIC_STRCMP:
					{
						TCHAR *ppCmd[2];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppCmd, COUNTOF(ppCmd), ",");
						if ( iCount < 2 )
							iResult = 1;
						else
							iResult = strcmp(ppCmd[0], ppCmd[1]);
						break;
					}
					case INTRINSIC_STRCMPI:
					{
						TCHAR *ppCmd[2];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppCmd, COUNTOF(ppCmd), ",");
						if ( iCount < 2 )
							iResult = 1;
						else
							iResult = strcmpi(ppCmd[0], ppCmd[1]);
						break;
					}
					case INTRINSIC_STRINDEXOF:
					{
						TCHAR *ppCmd[3];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppCmd, COUNTOF(ppCmd), ",");
						if ( iCount < 2 )
							iResult = -1;
						else
							iResult = Str_IndexOf(ppCmd[0], ppCmd[1], (iCount >= 3) ? static_cast<int>(GetVal(ppCmd[2])) : 0);
						break;
					}
					case INTRINSIC_STRLEN:
					{
						iCount = 1;
						iResult = strlen(pszArgs);
						break;
					}
					case INTRINSIC_STRMATCH:
					{
						TCHAR *ppCmd[2];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppCmd, COUNTOF(ppCmd), ",");
						if ( iCount < 2 )
							iResult = 0;
						else
							iResult = (Str_Match(ppCmd[0], ppCmd[1]) == MATCH_VALID) ? 1 : 0;
						break;
					}
					case INTRINSIC_STRREGEX:
					{
						TCHAR *ppCmd[2];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppCmd, COUNTOF(ppCmd), ",");
						if ( iCount < 2 )
							iResult = 0;
						else
						{
							TCHAR *pszLastError = Str_GetTemp();
							iResult = Str_RegExMatch(ppCmd[0], ppCmd[1], pszLastError);
							if ( iResult == -1 )
								DEBUG_ERR(("STRREGEX: Bad function usage. Error: %s\n", pszLastError));
						}
						break;
					}
					case INTRINSIC_TAN:
					{
						if ( pszArgs )
						{
							iCount = 1;
							iResult = static_cast<INT64>(tan(static_cast<double>(GetVal(pszArgs))));
						}
						else
						{
							iCount = 0;
							iResult = 0;
						}
						break;
					}
					default:
					{
						iCount = 0;
						iResult = 0;
						break;
					}
				}

				pszArgs = pszArgsNext;

				if ( !iCount )
				{
					DEBUG_ERR(("Bad intrinsic function usage: Missing arguments\n"));
					return 0;
				}
				return iResult;
			}
		}

		// Must be a symbol of some sort ?
		long long lVal;
		if ( m_VarGlobals.GetParseVal(pszArgs, &lVal) )
			return lVal;
		if ( m_VarDefs.GetParseVal(pszArgs, &lVal) )
			return lVal;
	}
#pragma endregion intrinsics

	// hard end ! Error of some sort.
	TCHAR szTag[EXPRESSION_MAX_KEY_LEN];
	size_t i = GetIdentifierString(szTag, pszArgs);
	pszArgs += i;	// skip it.
	if ( strlen(orig) > 1 )
		DEBUG_ERR(("Undefined symbol '%s' ['%s']\n", szTag, orig));
	else
		DEBUG_ERR(("Undefined symbol '%s'\n", szTag));
	return 0;
}

INT64 CExpression::GetValMath(INT64 lVal, LPCTSTR &pszArgs)
{
	ADDTOCALLSTACK("CExpression::GetValMath");
	GETNONWHITESPACE(pszArgs);

	// Look for math type operator.
	switch ( pszArgs[0] )
	{
		case '\0':
			break;
		case ')':	// expression end markers.
		case '}':
		case ']':
		{
			pszArgs++;	// consume this.
			break;
		}
		case '+':
		{
			pszArgs++;
			lVal += GetVal(pszArgs);
			break;
		}
		case '-':
		{
			pszArgs++;
			lVal -= GetVal(pszArgs);
			break;
		}
		case '*':
		{
			pszArgs++;
			lVal *= GetVal(pszArgs);
			break;
		}
		case '|':
		{
			pszArgs++;
			if ( pszArgs[0] == '|' )	// boolean ?
			{
				pszArgs++;
				lVal = (GetVal(pszArgs) || lVal);
			}
			else	// bitwise
				lVal |= GetVal(pszArgs);
			break;
		}
		case '&':
		{
			pszArgs++;
			if ( pszArgs[0] == '&' )	// boolean ?
			{
				pszArgs++;
				lVal = (GetVal(pszArgs) && lVal);	// tricky stuff here. logical ops must come first or possibly not get processed.
			}
			else	// bitwise
				lVal &= GetVal(pszArgs);
			break;
		}
		case '/':
		{
			pszArgs++;
			INT64 iVal = GetVal(pszArgs);
			if ( !iVal )
			{
				DEBUG_ERR(("Exp_GetVal: Divide by 0\n"));
				break;
			}
			lVal /= iVal;
			break;
		}
		case '%':
		{
			pszArgs++;
			INT64 iVal = GetVal(pszArgs);
			if ( !iVal )
			{
				DEBUG_ERR(("Exp_GetVal: Divide by 0\n"));
				break;
			}
			lVal %= iVal;
			break;
		}
		case '^':
		{
			pszArgs++;
			lVal ^= GetVal(pszArgs);
			break;
		}
		case '>': // boolean
		{
			pszArgs++;
			if ( pszArgs[0] == '=' )	// boolean ?
			{
				pszArgs++;
				lVal = (lVal >= GetVal(pszArgs));
			}
			else if ( pszArgs[0] == '>' )	// shift
			{
				pszArgs++;
				lVal >>= GetVal(pszArgs);
			}
			else
				lVal = (lVal > GetVal(pszArgs));
			break;
		}
		case '<': // boolean
		{
			pszArgs++;
			if ( pszArgs[0] == '=' )	// boolean ?
			{
				pszArgs++;
				lVal = (lVal <= GetVal(pszArgs));
			}
			else if ( pszArgs[0] == '<' )	// shift
			{
				pszArgs++;
				lVal <<= GetVal(pszArgs);
			}
			else
				lVal = (lVal < GetVal(pszArgs));
			break;
		}
		case '!':
		{
			pszArgs++;
			if ( pszArgs[0] != '=' )
				break; // boolean ! is handled as a single expresion.
			pszArgs++;
			lVal = (lVal != GetVal(pszArgs));
			break;
		}
		case '=': // boolean
		{
			while ( pszArgs[0] == '=' )
				pszArgs++;
			lVal = (lVal == GetVal(pszArgs));
			break;
		}
		case '@':
		{
			pszArgs++;
			if ( lVal <= 0 )
			{
				DEBUG_ERR(("Exp_GetVal: Power of zero with negative base is undefined\n"));
				break;
			}
			lVal = static_cast<INT64>(pow(static_cast<double>(lVal), static_cast<int>(GetVal(pszArgs))));
			break;
		}
	}

	return lVal;
}

int g_getval_reentrant_check = 0;

INT64 CExpression::GetVal(LPCTSTR &pszArgs)
{
	ADDTOCALLSTACK("CExpression::GetVal");
	// Get a value (default decimal) that could also be an expression.
	// This does not parse beyond a comma !
	//
	// These are all the type of expressions and defines we'll see:
	//
	//	all_skin_colors					// simple DEF value
	//	7933 						// simple decimal
	//	-100.0						// simple negative decimal
	//	.5						// simple decimal
	//	0.5						// simple decimal
	//	073a 						// hex value (leading zero and no .)
	//
	//	0 -1						// Subtraction. has a space separator. (Yes I know I hate this)
	//	{0-1}						// hyphenated simple range (GET RID OF THIS!)
	//		complex ranges must be in {}
	//	{ 3 6}						// simple range
	//	{ 400 1 401 1 } 				// weighted values (2nd val = 1)
	//	{ 1102 1148 1 }					// weighted range (3rd val < 10)
	//	{ animal_colors 1 no_colors 1 } 		// weighted range
	//	{ red_colors 1 {34 39} 1 }			// same (red_colors expands to a range)

	if ( pszArgs == NULL )
		return 0;

	GETNONWHITESPACE(pszArgs);

	g_getval_reentrant_check++;
	if ( g_getval_reentrant_check > 128 )
	{
		DEBUG_WARN(("Deadlock detected while parsing '%s'. Fix the error in your scripts.\n", pszArgs));
		g_getval_reentrant_check--;
		return 0;
	}
	INT64 lVal = GetValMath(GetSingle(pszArgs), pszArgs);
	g_getval_reentrant_check--;

	return lVal;
}

int CExpression::GetRangeVals(LPCTSTR &pszArgs, INT64 *piVals, int iMaxQty)
{
	ADDTOCALLSTACK("CExpression::GetRangeVals");
	// Get a list of values.
	if ( pszArgs == NULL )
		return 0;

	ASSERT(piVals);

	int iQty = 0;
	for (;;)
	{
		if ( !pszArgs[0] )
			break;
		if ( pszArgs[0] == ';' )
			break;	// seperate field.
		if ( pszArgs[0] == ',' )
			pszArgs++;

		piVals[iQty] = GetSingle(pszArgs);
		if ( ++iQty >= iMaxQty )
			break;
		if ( (pszArgs[0] == '-') && (iQty == 1) )	// range separator. (if directly after, I know this is sort of strange)
		{
			pszArgs++;	// ??? This is stupid. get rid of this and clean up it's use in the scripts.
			continue;
		}

		GETNONWHITESPACE(pszArgs);

		// Look for math type operator.
		switch ( pszArgs[0] )
		{
			case ')':	// expression end markers.
			case '}':
			case ']':
				pszArgs++;	// consume this and end.
				return iQty;
			case '+':
			case '*':
			case '/':
			case '%':
			case '<':
			case '>':
			case '|':
			case '&':
			//case '^':
				piVals[iQty - 1] = GetValMath(piVals[iQty - 1], pszArgs);
				break;
		}
	}

	return iQty;
}

INT64 CExpression::GetRange(LPCTSTR &pszArgs)
{
	ADDTOCALLSTACK("CExpression::GetRange");
	INT64 lVals[256];		// Maximum elements in a list

	int iQty = GetRangeVals(pszArgs, lVals, COUNTOF(lVals));
	if ( iQty == 0 )
		return 0;
	else if ( iQty == 1 )	// simple value
		return lVals[0];
	else if ( iQty == 2 )	// simple range
		return Calc_GetRandLLVal2(minimum(lVals[0], lVals[1]), maximum(lVals[0], lVals[1]));
	else	// weighted value
	{
		// Get total weight
		INT64 iTotalWeight = 0;
		int i = 1;
		for ( ; i < iQty; i += 2 )
		{
			if ( !lVals[i] )	// having a weight of 0 is very strange !
				DEBUG_ERR(("Weight of 0 in random set?\n"));	// the whole table should really just be invalid here !
			iTotalWeight += lVals[i];
		}

		// Roll the dice to see what value to pick
		iTotalWeight = Calc_GetRandLLVal(iTotalWeight) + 1;

		// Loop to that value
		i = 1;
		for ( ; i < iQty; i += 2 )
		{
			iTotalWeight -= lVals[i];
			if ( iTotalWeight <= 0 )
				break;
		}
		return lVals[i - 1];
	}
}
