#include "../graysvr/graysvr.h"
#include <random>

TCHAR CExpression::sm_szMessages[DEFMSG_QTY][EXPRESSION_MAX_KEY_LEN] =
{
	#define MSG(a,b) b,
	#include "../tables/defmessages.tbl"
	#undef MSG
};

const LPCTSTR CExpression::sm_szMsgNames[DEFMSG_QTY] =
{
	#define MSG(a,b) #a,
	#include "../tables/defmessages.tbl"
	#undef MSG
};

int ahextoi(LPCTSTR pszArgs)	// convert hex string to int
{
	// Unfortunately the library func can't handle UINT_MAX
	//TCHAR *pszEnd; return strtol(s, &pszEnd, 16);

	if ( !pszArgs || !*pszArgs )
		return 0;

	GETNONWHITESPACE(pszArgs);

	bool fHex = false;
	if ( *pszArgs == '0' )
	{
		if ( *++pszArgs != '.' )
			fHex = true;
		--pszArgs;
	}

	int iVal = 0;
	for (;;)
	{
		TCHAR ch = static_cast<TCHAR>(toupper(*pszArgs));
		if ( IsDigit(ch) )
			ch -= '0';
		else if ( fHex && (ch >= 'A') && (ch <= 'F') )
			ch -= 'A' - 10;
		else if ( !fHex && (ch == '.') )
		{
			++pszArgs;
			continue;
		}
		else
			break;

		iVal *= (fHex ? 0x10 : 10);
		iVal += ch;
		++pszArgs;
	}
	return iVal;
}

INT64 ahextoi64(LPCTSTR pszArgs)	// convert hex string to int64
{
	if ( !pszArgs || !*pszArgs )
		return 0;

	GETNONWHITESPACE(pszArgs);

	bool fHex = false;
	if ( *pszArgs == '0' )
	{
		if ( *++pszArgs != '.' )
			fHex = true;
		--pszArgs;
	}

	INT64 iVal = 0;
	for (;;)
	{
		TCHAR ch = static_cast<TCHAR>(toupper(*pszArgs));
		if ( IsDigit(ch) )
			ch -= '0';
		else if ( fHex && (ch >= 'A') && (ch <= 'F') )
			ch -= 'A' - 10;
		else if ( !fHex && (ch == '.') )
		{
			++pszArgs;
			continue;
		}
		else
			break;

		iVal *= (fHex ? 0x10 : 10);
		iVal += ch;
		++pszArgs;
	}
	return iVal;
}

bool IsStrEmpty(LPCTSTR pszArgs)
{
	if ( !pszArgs || !*pszArgs )
		return true;

	do
	{
		if ( !IsSpace(*pszArgs) )
			return false;
	} while ( *(++pszArgs) );

	return true;
}

bool IsStrNumericDec(LPCTSTR pszArgs)
{
	if ( !pszArgs || !*pszArgs )
		return false;

	do
	{
		if ( !IsDigit(*pszArgs) )
			return false;
	} while ( *(++pszArgs) );

	return true;
}


bool IsStrNumeric(LPCTSTR pszArgs)
{
	if ( !pszArgs || !*pszArgs )
		return false;

	bool fHex = false;
	if ( pszArgs[0] == '0' )
		fHex = true;

	do
	{
		if ( IsDigit(*pszArgs) )
			continue;
		if ( fHex && (tolower(*pszArgs) >= 'a') && (tolower(*pszArgs) <= 'f') )
			continue;
		return false;
	} while ( *(++pszArgs) );

	return true;
}

bool IsSimpleNumberString(LPCTSTR pszArgs)
{
	// Is this a string or a simple numeric expression?
	// String = '1 2 3', 'sdf', 'sdf sdf sdf', '123d', '123 d'
	// Number = '1.0+-\*~|&!%^()2', '0aed', '123'

	bool fMathSep = true;	// last non whitespace was a math sep
	bool fHextDigitStart = false;
	bool fWhiteSpace = false;

	for ( ; ; ++pszArgs )
	{
		TCHAR ch = *pszArgs;
		if ( !ch )
			return true;

		if ( ((ch >= 'A') && (ch <= 'F')) || ((ch >= 'a') && (ch <= 'f')) )	// isxdigit
		{
			if ( !fHextDigitStart )
				return false;
			fWhiteSpace = false;
			fMathSep = false;
			continue;
		}
		if ( IsSpace(ch) )
		{
			fHextDigitStart = false;
			fWhiteSpace = true;
			continue;
		}
		if ( IsDigit(ch) )
		{
			if ( fWhiteSpace && !fMathSep )
				return false;
			if ( ch == '0' )
				fHextDigitStart = true;
			fWhiteSpace = false;
			fMathSep = false;
			continue;
		}
		if ( (ch == '/') && (pszArgs[1] != '/') )
			fMathSep = true;
		else
			fMathSep = strchr("+-\\*~|&!%^()", ch) ? true : false;

		if ( !fMathSep )
			return false;

		fHextDigitStart = false;
		fWhiteSpace = false;
	}
}

static size_t GetIdentifierString(TCHAR *szTag, LPCTSTR pszArgs)
{
	// Copy the identifier (valid char set) out to this buffer
	size_t i = 0;
	for ( ; pszArgs[i]; ++i )
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

bool IsValidDef(LPCTSTR pszArgs)
{
	CVarDefCont *pVarBase = g_Exp.m_VarDefs.CheckParseKey(pszArgs);
	if ( !pVarBase )
	{
		// Check VAR.X also
		pVarBase = g_Exp.m_VarGlobals.CheckParseKey(pszArgs);
		if ( !pVarBase )
			return false;
	}
	return true;
}

bool IsValidGameObjDef(LPCTSTR pszArgs)
{
	if ( !IsSimpleNumberString(pszArgs) )
	{
		CVarDefCont *pVarBase = g_Exp.m_VarDefs.CheckParseKey(pszArgs);
		if ( !pVarBase )
			return false;

		TCHAR ch = *pVarBase->GetValStr();
		if ( !ch || (ch == '<') )
			return false;

		RESOURCE_ID rid = g_Cfg.ResourceGetID(RES_QTY, pszArgs);
		if ( (rid.GetResType() != RES_CHARDEF) && (rid.GetResType() != RES_ITEMDEF) && (rid.GetResType() != RES_SPAWN) && (rid.GetResType() != RES_TEMPLATE) )
			return false;
	}
	return true;
}

///////////////////////////////////////////////////////////
// Numeric formulas

int Calc_GetRandVal(int iMin, int iMax)
{
	if ( iMin > iMax )
	{
		int iTemp = iMin;
		iMin = iMax;
		iMax = iTemp;
	}

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(iMin, iMax);
	return dist(gen);
}

INT64 Calc_GetRandLLVal(INT64 iMin, INT64 iMax)
{
	if ( iMin > iMax )
	{
		INT64 iTemp = iMin;
		iMin = iMax;
		iMax = iTemp;
	}

	std::random_device rd;
	std::mt19937_64 gen(rd());
	std::uniform_int_distribution<INT64> dist(iMin, iMax);
	return dist(gen);
}

int Calc_GetBellCurve(int iMean, int iVariance)
{
	// Produce a log curve
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
	//    iVariance			iMean
	//
	// ARGS:
	//   iMean = Given a value relative to 0
	//       Negative = lower chance
	//       0 = 50% chance
	//       Positive = higher chance
	//   iVariance = the 25% point of the bell curve
	// RETURN:
	//   0-100 (%) = chance at this iMean
	//   Chance gets smaller as difference gets bigger
	// EXAMPLE:
	//   if ( iMean == iVariance ) return 250
	//   if ( iMean == 0 ) return 500

	if ( iVariance <= 0 )	// this really should not happen but just in case
		return 500;

	if ( iMean < 0 )
		iMean = -iMean;

	int iChance = 500;
	while ( (iMean > iVariance) && iChance )
	{
		iMean -= iVariance;
		iChance /= 2;	// chance is halved for each variance period
	}

	return iChance - IMULDIV(iChance / 2, iMean, iVariance);
}

int Calc_GetSCurve(int iMean, int iVariance)
{
	// ARGS:
	//   iMean = Difference between our skill level and difficulty
	//       Negative = lower chance
	//       0 = 50% chance
	//       Positive = higher chance
	//   iVariance = the 25% difference point of the bell curve
	// RETURN:
	//   0-1000 (%) = chance of success
	// NOTE:
	//   Chance of skill gain is inverse to chance of success

	int iChance = Calc_GetBellCurve(iMean, iVariance);
	if ( iMean > 0 )
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
	GETNONWHITESPACE(pszArgs);

	LPCTSTR orig = pszArgs;
	if ( pszArgs[0] == '.' )
		++pszArgs;

	if ( pszArgs[0] == '0' )	// leading '0' = hex value
	{
		// Hex value
		if ( pszArgs[1] == '.' )	// leading '0.' = decimal value
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
					if ( (ch == '.') && (pStart[0] != '0') )	// ok I'm confused, it must be decimal
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
			++pszArgs;
		}
		return iVal;
	}
	else if ( (pszArgs[0] == '.') || IsDigit(pszArgs[0]) )
	{
		// Decimal value
	try_dec:
		INT64 iVal = 0;
		for ( ; ; ++pszArgs )
		{
			if ( *pszArgs == '.' )
				continue;	// just skip this
			if ( !IsDigit(*pszArgs) )
				break;
			iVal *= 10;
			iVal += static_cast<INT64>(*pszArgs) - '0';
		}
		return iVal;
	}
	else if ( !_ISCSYMF(pszArgs[0]) )
	{
		// Some sort of math operator?
		switch ( pszArgs[0] )
		{
			case '{':
				++pszArgs;
				return GetRange(pszArgs);
			case '[':
			case '(':	// parse out a sub expression
				++pszArgs;
				return GetVal(pszArgs);
			case '+':
				++pszArgs;
				break;
			case '-':
				++pszArgs;
				return -GetSingle(pszArgs);
			case '~':	// bitwise 'not'
				++pszArgs;
				return ~GetSingle(pszArgs);
			case '!':	// boolean 'not'
				++pszArgs;
				if ( pszArgs[0] == '=' )	// odd condition such as (!=x) which is always true of course
				{
					++pszArgs;		// so just skip and compare it to 0
					return GetSingle(pszArgs);
				}
				return !GetSingle(pszArgs);
			case ';':	// seperator field
			case ',':	// seperator field
			case '\0':
				return 0;
		}
	}
	else
	{
		// Symbol or intrinsinc function?
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
						if ( *pszArgs != '\0' )
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
						if ( *pszArgs != '\0' )
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
						if ( *pszArgs != '\0' )
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
						if ( *pszArgs != '\0' )
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
						if ( *pszArgs != '\0' )
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
						char *pchEnd;
						static_cast<void>(strtol(pszArgs, &pchEnd, 10));

						iCount = 1;
						iResult = *pchEnd ? 0 : 1;
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

						if ( *pszArgs != '\0' )
						{
							INT64 iArgs = GetVal(pszArgs);
							if ( iArgs <= 0 )
								DEBUG_ERR(("%s: Log(%lld) is %s\n", sm_IntrinsicFunctions[index], iArgs, !iArgs ? "infinite" : "undefined"));
							else
							{
								iCount = 1;
								if ( strchr(pszArgs, ',') )
								{
									++iCount;
									SKIP_ARGSEP(pszArgs);
									if ( !strcmpi(pszArgs, "e") )
										iResult = static_cast<INT64>(log(static_cast<double>(iArgs)));
									else if ( !strcmpi(pszArgs, "pi") )
										iResult = static_cast<INT64>(log(static_cast<double>(iArgs)) / log(M_PI));
									else
									{
										INT64 iBase = GetVal(pszArgs);
										if ( iBase <= 0 )
										{
											DEBUG_ERR(("%s: (%lld)Log(%lld) is %s\n", sm_IntrinsicFunctions[index], iBase, iArgs, !iBase ? "infinite" : "undefined"));
											iCount = 0;
										}
										else
											iResult = static_cast<INT64>(log(static_cast<double>(iArgs)) / log(static_cast<double>(iBase)));
									}
								}
								else
									iResult = static_cast<INT64>(log10(static_cast<double>(iArgs)));
							}
						}
						break;
					}
					case INTRINSIC_NAPIERPOW:
					{
						if ( *pszArgs != '\0' )
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
						TCHAR *ppArgs[5];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
						if ( iCount < 3 )
							iResult = 0;
						else
						{
							INT64 iVal1 = GetSingle(ppArgs[0]);
							INT64 iVal2 = GetSingle(ppArgs[1]);
							if ( iVal1 < iVal2 )
								iResult = GetSingle(ppArgs[2]);
							else if ( iVal1 == iVal2 )
								iResult = (iCount >= 4) ? GetSingle(ppArgs[3]) : 0;
							else
								iResult = (iCount >= 5) ? GetSingle(ppArgs[4]) : 0;
						}
						break;
					}
					case INTRINSIC_RAND:
					{
						TCHAR *ppArgs[2];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
						if ( iCount <= 0 )
							iResult = 0;
						else if ( iCount < 2 )
							iResult = Calc_GetRandLLVal(GetVal(ppArgs[0]));
						else
							iResult = Calc_GetRandLLVal(GetVal(ppArgs[0]), GetVal(ppArgs[1]));
						break;
					}
					case INTRINSIC_RANDBELL:
					{
						TCHAR *ppArgs[2];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
						if ( iCount < 2 )
							iResult = 0;
						else
							iResult = Calc_GetBellCurve(static_cast<int>(GetVal(ppArgs[0])), static_cast<int>(GetVal(ppArgs[1])));
						break;
					}
					case INTRINSIC_SIN:
					{
						if ( *pszArgs != '\0' )
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

						if ( *pszArgs != '\0' )
						{
							INT64 iArgs = GetVal(pszArgs);
							if ( iArgs >= 0 )
							{
								++iCount;
								iResult = static_cast<INT64>(sqrt(static_cast<double>(iArgs)));
							}
							else
								DEBUG_ERR(("%s(%lld): can't get square root of negative number\n", sm_IntrinsicFunctions[index], iArgs));
						}
						break;
					}
					case INTRINSIC_STRASCII:
					{
						if ( *pszArgs != '\0' )
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
						TCHAR *ppArgs[2];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
						if ( iCount < 2 )
							iResult = 1;
						else
							iResult = strcmp(ppArgs[0], ppArgs[1]);
						break;
					}
					case INTRINSIC_STRCMPI:
					{
						TCHAR *ppArgs[2];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
						if ( iCount < 2 )
							iResult = 1;
						else
							iResult = strcmpi(ppArgs[0], ppArgs[1]);
						break;
					}
					case INTRINSIC_STRINDEXOF:
					{
						TCHAR *ppArgs[3];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
						if ( iCount < 2 )
							iResult = -1;
						else
							iResult = Str_IndexOf(ppArgs[0], ppArgs[1], (iCount >= 3) ? static_cast<int>(GetVal(ppArgs[2])) : 0);
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
						TCHAR *ppArgs[2];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
						if ( iCount < 2 )
							iResult = 0;
						else
							iResult = (Str_Match(ppArgs[0], ppArgs[1]) == MATCH_VALID) ? 1 : 0;
						break;
					}
					case INTRINSIC_STRREGEX:
					{
						TCHAR *ppArgs[2];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
						if ( iCount < 2 )
							iResult = 0;
						else
							iResult = (Str_RegExMatch(ppArgs[0], ppArgs[1]) == MATCH_VALID) ? 1 : 0;
						break;
					}
					case INTRINSIC_TAN:
					{
						if ( *pszArgs != '\0' )
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

		// Must be a symbol of some sort?
		long long llVal;
		if ( m_VarGlobals.GetParseVal(pszArgs, &llVal) )
			return llVal;
		if ( m_VarDefs.GetParseVal(pszArgs, &llVal) )
			return llVal;
	}

	// Error of some sort
	TCHAR szTag[EXPRESSION_MAX_KEY_LEN];
	pszArgs += GetIdentifierString(szTag, pszArgs);		// skip it
	if ( strlen(orig) > 1 )
		DEBUG_ERR(("Undefined symbol '%s' ['%s']\n", szTag, orig));
	else
		DEBUG_ERR(("Undefined symbol '%s'\n", szTag));
	return 0;
}

INT64 CExpression::GetValMath(INT64 iVal, LPCTSTR &pszArgs)
{
	ADDTOCALLSTACK("CExpression::GetValMath");
	GETNONWHITESPACE(pszArgs);

	// Look for math type operator
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
			iVal += GetVal(pszArgs);
			break;
		}
		case '-':
		{
			++pszArgs;
			iVal -= GetVal(pszArgs);
			break;
		}
		case '*':
		{
			++pszArgs;
			iVal *= GetVal(pszArgs);
			break;
		}
		case '|':
		{
			++pszArgs;
			if ( pszArgs[0] == '|' )	// boolean
			{
				++pszArgs;
				iVal = (GetVal(pszArgs) || iVal);
			}
			else	// bitwise
				iVal |= GetVal(pszArgs);
			break;
		}
		case '&':
		{
			++pszArgs;
			if ( pszArgs[0] == '&' )	// boolean
			{
				++pszArgs;
				iVal = (GetVal(pszArgs) && iVal);	// tricky stuff here, logical operators must come first or possibly not get processed
			}
			else	// bitwise
				iVal &= GetVal(pszArgs);
			break;
		}
		case '/':
		{
			++pszArgs;
			INT64 iArgs = GetVal(pszArgs);
			if ( iArgs == 0 )
			{
				DEBUG_ERR(("Can't divide by 0\n"));
				break;
			}
			iVal /= iArgs;
			break;
		}
		case '%':
		{
			++pszArgs;
			INT64 iArgs = GetVal(pszArgs);
			if ( iArgs == 0 )
			{
				DEBUG_ERR(("Can't divide by 0\n"));
				break;
			}
			iVal %= iArgs;
			break;
		}
		case '^':
		{
			++pszArgs;
			iVal ^= GetVal(pszArgs);
			break;
		}
		case '>':
		{
			++pszArgs;
			if ( pszArgs[0] == '=' )	// boolean
			{
				++pszArgs;
				iVal = (iVal >= GetVal(pszArgs));
			}
			else if ( pszArgs[0] == '>' )	// shift
			{
				++pszArgs;
				iVal >>= GetVal(pszArgs);
			}
			else
				iVal = (iVal > GetVal(pszArgs));
			break;
		}
		case '<':
		{
			++pszArgs;
			if ( pszArgs[0] == '=' )	// boolean
			{
				++pszArgs;
				iVal = (iVal <= GetVal(pszArgs));
			}
			else if ( pszArgs[0] == '<' )	// shift
			{
				++pszArgs;
				iVal <<= GetVal(pszArgs);
			}
			else
				iVal = (iVal < GetVal(pszArgs));
			break;
		}
		case '!':
		{
			++pszArgs;
			if ( pszArgs[0] != '=' )	// boolean (handled as single expression)
				break;
			++pszArgs;
			iVal = (iVal != GetVal(pszArgs));
			break;
		}
		case '=':
		{
			while ( pszArgs[0] == '=' )	// boolean
				++pszArgs;
			iVal = (iVal == GetVal(pszArgs));
			break;
		}
		case '@':
		{
			++pszArgs;
			if ( iVal <= 0 )
			{
				DEBUG_ERR(("Power of zero with negative base is undefined\n"));
				break;
			}
			iVal = static_cast<INT64>(pow(static_cast<double>(iVal), static_cast<int>(GetVal(pszArgs))));
			break;
		}
	}

	return iVal;
}

int g_GetVal_LoopCount = 0;

INT64 CExpression::GetVal(LPCTSTR &pszArgs)
{
	ADDTOCALLSTACK("CExpression::GetVal");
	// Get a value (default decimal) that could also be an expression
	// This does not parse beyond a comma!
	//
	// All expression types:
	//  all_skin_colors               = simple DEF value
	//  7933                          = simple decimal
	//  -100.0                        = simple negative decimal
	//  .5                            = simple decimal
	//  0.5                           = simple decimal
	//  073a                          = simple hex value (leading zero and no .)
	//  0 -1                          = subtraction, has a space separator (yes, I know I hate this)
	//  {0-1}                         = hyphenated simple range (GET RID OF THIS!)
	//  {3 6}                         = simple range
	//  {400 1 401 1}                 = weighted values (2nd val = 1)
	//  {1102 1148 1}                 = weighted range (3rd val < 10)
	//  {animal_colors 1 no_colors 1} = weighted range
	//  {red_colors 1 {34 39} 1}      = weighted range (red_colors expands to a range)

	if ( !pszArgs || !*pszArgs )
		return 0;

	GETNONWHITESPACE(pszArgs);

	++g_GetVal_LoopCount;
	if ( g_GetVal_LoopCount > 128 )
	{
		DEBUG_WARN(("Deadlock detected while parsing '%s'. Fix the error in your scripts\n", pszArgs));
		--g_GetVal_LoopCount;
		return 0;
	}
	INT64 iVal = GetValMath(GetSingle(pszArgs), pszArgs);
	--g_GetVal_LoopCount;

	return iVal;
}

int CExpression::GetRangeVals(LPCTSTR &pszArgs, INT64 *piVals, int iMaxQty)
{
	ADDTOCALLSTACK("CExpression::GetRangeVals");
	// Get a list of values
	if ( !pszArgs || !*pszArgs )
		return 0;

	ASSERT(piVals);

	int iQty = 0;
	for (;;)
	{
		if ( !pszArgs[0] )
			break;
		if ( pszArgs[0] == ';' )	// separator field
			break;
		if ( pszArgs[0] == ',' )
			++pszArgs;

		piVals[iQty] = GetSingle(pszArgs);
		if ( ++iQty >= iMaxQty )
			break;
		if ( (pszArgs[0] == '-') && (iQty == 1) )	// range separator (if directly after, I know this is sort of strange)
		{
			++pszArgs;	// ??? This is stupid, get rid of this and clean up it's use in the scripts
			continue;
		}

		GETNONWHITESPACE(pszArgs);

		// Look for math type operator
		switch ( pszArgs[0] )
		{
			case ')':	// expression end markers
			case '}':
			case ']':
				++pszArgs;
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
	INT64 iVals[256];		// maximum elements in a list

	int iQty = GetRangeVals(pszArgs, iVals, COUNTOF(iVals));
	if ( iQty == 0 )
		return 0;
	else if ( iQty == 1 )	// simple value
		return iVals[0];
	else if ( iQty == 2 )	// simple range
		return Calc_GetRandLLVal(minimum(iVals[0], iVals[1]), maximum(iVals[0], iVals[1]));
	else	// weighted value
	{
		// Get total weight
		INT64 iTotalWeight = 0;
		int i = 1;
		for ( ; i < iQty; i += 2 )
		{
			if ( !iVals[i] )	// having a weight of 0 is very strange
				DEBUG_ERR(("Weight of 0 in random set?\n"));	// the whole table should really just be invalid here
			iTotalWeight += iVals[i];
		}

		// Roll the dice to see what value to pick
		iTotalWeight = Calc_GetRandLLVal(iTotalWeight) + 1;

		// Loop to that value
		i = 1;
		for ( ; i < iQty; i += 2 )
		{
			iTotalWeight -= iVals[i];
			if ( iTotalWeight <= 0 )
				break;
		}
		return iVals[i - 1];
	}
}
