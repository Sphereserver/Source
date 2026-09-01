#include "../graysvr/graysvr.h"
#include <numbers>
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

INT64 ahextoi(LPCTSTR pszArgs)	// convert hex string to INT64
{
	if ( !pszArgs || !*pszArgs )
		return 0;

	SkipWhitespace(pszArgs);

	bool fHex = ((pszArgs[0] == '0') && (pszArgs[1] != '.'));
	INT64 iVal = 0;

	for ( ; *pszArgs != '\0'; ++pszArgs )
	{
		TCHAR ch = static_cast<TCHAR>(toupper(static_cast<unsigned char>(*pszArgs)));
		if ( IsDigit(ch) )
			ch -= '0';
		else if ( fHex && (ch >= 'A') && (ch <= 'F') )
			ch -= 'A' - 10;
		else if ( !fHex && (ch == '.') )
			continue;
		else
			break;

		iVal *= (fHex ? 0x10 : 10);
		iVal += ch;
	}
	return iVal;
}

bool IsStrEmpty(LPCTSTR pszArgs)
{
	if ( !pszArgs || !*pszArgs )
		return true;

	for ( ; *pszArgs != '\0'; ++pszArgs )
	{
		if ( !IsSpace(*pszArgs) )
			return false;
	}
	return true;
}

bool IsStrNumericDec(LPCTSTR pszArgs)
{
	if ( !pszArgs || !*pszArgs )
		return false;

	for ( ; *pszArgs != '\0'; ++pszArgs )
	{
		if ( !IsDigit(*pszArgs) )
			return false;
	}
	return true;
}

bool IsStrNumeric(LPCTSTR pszArgs)
{
	if ( !pszArgs || !*pszArgs )
		return false;

	bool fHex = (pszArgs[0] == '0');

	for ( ; *pszArgs != '\0'; ++pszArgs )
	{
		TCHAR ch = *pszArgs;
		if ( !IsDigit(ch) )
		{
			if ( !fHex || !(((ch >= 'A') && (ch <= 'F')) || ((ch >= 'a') && (ch <= 'f'))) )
				return false;
		}
	}
	return true;
}

bool IsSimpleNumberString(LPCTSTR pszArgs)
{
	// Is this a string or a simple numeric expression?
	// String = '1 2 3', 'sdf', 'sdf sdf sdf', '123d', '123 d'
	// Number = '1.0+-\*~|&%^()2', '0aed', '123'

	bool fHex = false;
	bool fMathSep = true;	// last non whitespace was a math sep
	bool fWhitespace = false;

	for ( ; *pszArgs != '\0'; ++pszArgs )
	{
		TCHAR ch = *pszArgs;
		if ( ((ch >= 'A') && (ch <= 'F')) || ((ch >= 'a') && (ch <= 'f')) )		// isxdigit(ch)
		{
			if ( !fHex )
				return false;

			fMathSep = false;
			fWhitespace = false;
			continue;
		}
		if ( IsSpace(ch) )
		{
			fHex = false;
			fWhitespace = true;
			continue;
		}
		if ( IsDigit(ch) )
		{
			if ( fWhitespace && !fMathSep )
				return false;
			if ( ch == '0' )
				fHex = true;

			fMathSep = false;
			fWhitespace = false;
			continue;
		}
		if ( (ch == '/') && (pszArgs[1] != '/') )
			fMathSep = true;
		else
			fMathSep = (strchr("+-\\*~|&%^()", ch) != NULL);

		if ( !fMathSep )
			return false;

		fHex = false;
		fWhitespace = false;
	}
	return true;
}

static size_t GetIdentifierString(TCHAR *pszDest, LPCTSTR pszArgs)
{
	// Copy the identifier (valid char set) out to this buffer
	size_t i = 0;
	for ( ; pszArgs[i]; ++i )
	{
		if ( !IsCSym(pszArgs[i]) )
			break;
		if ( i >= EXPRESSION_MAX_KEY_LEN )
			return 0;
		pszDest[i] = pszArgs[i];
	}
	pszDest[i] = '\0';
	return i;
}

bool IsValidDef(LPCTSTR pszArgs)
{
	// Check DEF.X
	CVarDefCont *pVar = g_Exp.m_VarDefs.CheckParseKey(pszArgs);
	if ( !pVar )
	{
		// Check VAR.X
		pVar = g_Exp.m_VarGlobals.CheckParseKey(pszArgs);
		if ( !pVar )
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
		std::swap(iMin, iMax);

	thread_local std::random_device rd;
	thread_local std::mt19937 gen(rd());

	std::uniform_int_distribution<int> dist(iMin, iMax);
	return dist(gen);
}

INT64 Calc_GetRandLLVal(INT64 iMin, INT64 iMax)
{
	if ( iMin > iMax )
		std::swap(iMin, iMax);

	thread_local std::random_device rd;
	thread_local std::mt19937_64 gen(rd());

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

	SkipWhitespace(pszArgs);

	LPCTSTR pszOrig = pszArgs;
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

		LPCTSTR pszStart = pszArgs;
		INT64 iVal = 0;
		for ( ; *pszArgs != '\0'; ++pszArgs )
		{
			TCHAR ch = *pszArgs;
			if ( IsDigit(ch) )
				ch -= '0';
			else
			{
				ch = static_cast<TCHAR>(tolower(ch));
				if ( (ch < 'a') || (ch > 'f') )
				{
					if ( (ch == '.') && (pszStart[0] != '0') )	// ok I'm confused, it must be decimal
					{
						pszArgs = pszStart;
						goto try_dec;
					}
					break;
				}
				ch -= 'a' - 10;
			}
			iVal *= 0x10;
			iVal += ch;
		}
		return iVal;
	}
	else if ( (pszArgs[0] == '.') || IsDigit(pszArgs[0]) )
	{
		// Decimal value
	try_dec:
		INT64 iVal = 0;
		for ( ; *pszArgs != '\0'; ++pszArgs )
		{
			TCHAR ch = *pszArgs;
			if ( ch == '.' )
				continue;	// just skip this
			if ( !IsDigit(ch) )
				break;

			iVal *= 10;
			iVal += static_cast<INT64>(ch - '0');
		}
		return iVal;
	}
	else if ( !IsCSymF(pszArgs[0]) )
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

				size_t iCount = 0;
				INT64 iResult = 0;

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
							iResult = static_cast<INT64>((acos(static_cast<double>(GetVal(pszArgs))) * 180.0) / std::numbers::pi);
						}
						break;
					}
					case INTRINSIC_ARCSIN:
					{
						if ( *pszArgs != '\0' )
						{
							iCount = 1;
							iResult = static_cast<INT64>((asin(static_cast<double>(GetVal(pszArgs))) * 180.0) / std::numbers::pi);
						}
						break;
					}
					case INTRINSIC_ARCTAN:
					{
						if ( *pszArgs != '\0' )
						{
							iCount = 1;
							iResult = static_cast<INT64>((atan(static_cast<double>(GetVal(pszArgs))) * 180.0) / std::numbers::pi);
						}
						break;
					}
					case INTRINSIC_COS:
					{
						if ( *pszArgs != '\0' )
						{
							iCount = 1;
							iResult = static_cast<INT64>(cos((static_cast<double>(GetVal(pszArgs)) * std::numbers::pi) / 180.0));
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
						break;
					}
					case INTRINSIC_ISNUMBER:
					{
						TCHAR *pchEnd;
						static_cast<void>(strtol(pszArgs, &pchEnd, 10));

						iCount = 1;
						iResult = (*pchEnd == '\0') ? 1 : 0;
						break;
					}
					case INTRINSIC_ISOBSCENE:
					{
						iCount = 1;
						iResult = g_Cfg.IsObscene(pszArgs) ? 1 : 0;
						break;
					}
					case INTRINSIC_LOGARITHM:
					{
						TCHAR *ppArgs[2];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
						if ( iCount < 1 )
							break;

						LPCTSTR pszVal = ppArgs[0];
						INT64 iVal = GetVal(pszVal);
						if ( iVal <= 0 )
						{
							DEBUG_ERR(("%s(%lld): result is %s\n", sm_IntrinsicFunctions[index], iVal, (iVal == 0) ? "infinite" : "undefined"));
							break;
						}

						if ( iCount == 2 )
						{
							if ( !strcmpi(ppArgs[1], "e") )
								iResult = static_cast<INT64>(log(static_cast<double>(iVal)));
							else if ( !strcmpi(ppArgs[1], "pi") )
							{
								static const double dLogPi = log(std::numbers::pi);
								iResult = static_cast<INT64>(log(static_cast<double>(iVal)) / dLogPi);
							}
							else
							{
								pszVal = ppArgs[1];
								INT64 iBase = GetVal(pszVal);
								if ( iBase <= 0 )
								{
									DEBUG_ERR(("%s(%lld, %lld): result is %s\n", sm_IntrinsicFunctions[index], iVal, iBase, (iBase == 0) ? "infinite" : "undefined"));
									break;
								}
								iResult = static_cast<INT64>(log(static_cast<double>(iVal)) / log(static_cast<double>(iBase)));
							}
						}
						else
							iResult = static_cast<INT64>(log10(static_cast<double>(iVal)));
						break;
					}
					case INTRINSIC_NAPIERPOW:
					{
						if ( *pszArgs != '\0' )
						{
							iCount = 1;
							iResult = static_cast<INT64>(exp(static_cast<double>(GetVal(pszArgs))));
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
						INT64 iVal1 = GetSingle(pszVal);

						pszVal = ppArgs[1];
						INT64 iVal2 = GetSingle(pszVal);

						if ( iVal1 < iVal2 )
						{
							pszVal = ppArgs[2];
							iResult = GetSingle(pszVal);
						}
						else if ( iVal1 == iVal2 )
						{
							if ( iCount >= 4 )
							{
								pszVal = ppArgs[3];
								iResult = GetSingle(pszVal);
							}
						}
						else
						{
							if ( iCount >= 5 )
							{
								pszVal = ppArgs[4];
								iResult = GetSingle(pszVal);
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
							iResult = Calc_GetRandLLVal(GetVal(pszMin), GetVal(pszMax));
						}
						else if ( iCount == 1 )
						{
							LPCTSTR pszMax = ppArgs[0];
							iResult = Calc_GetRandLLVal(GetVal(pszMax));
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
							iResult = Calc_GetBellCurve(static_cast<int>(GetVal(pszMean)), static_cast<int>(GetVal(pszVariance)));
						}
						break;
					}
					case INTRINSIC_SIN:
					{
						if ( *pszArgs != '\0' )
						{
							iCount = 1;
							iResult = static_cast<INT64>(sin((static_cast<double>(GetVal(pszArgs)) * std::numbers::pi) / 180.0));
						}
						break;
					}
					case INTRINSIC_SQRT:
					{
						if ( *pszArgs != '\0' )
						{
							iCount = 1;
							INT64 iVal = GetVal(pszArgs);
							if ( iVal < 0 )
							{
								DEBUG_ERR(("%s(%lld): can't get square root of negative number\n", sm_IntrinsicFunctions[index], iVal));
								break;
							}
							iResult = static_cast<INT64>(sqrt(static_cast<double>(iVal)));
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
						break;
					}
					case INTRINSIC_STRCMP:
					{
						TCHAR *ppArgs[2];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
						if ( iCount == 2 )
							iResult = strcmp(ppArgs[0], ppArgs[1]);
						else
							iResult = 1;
						break;
					}
					case INTRINSIC_STRCMPI:
					{
						TCHAR *ppArgs[2];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
						if ( iCount == 2 )
							iResult = strcmpi(ppArgs[0], ppArgs[1]);
						else
							iResult = 1;
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
								iOffset = static_cast<size_t>(GetVal(pszOffset));
							}
							iResult = Str_IndexOf(ppArgs[0], ppArgs[1], iOffset);
						}
						else
							iResult = -1;
						break;
					}
					case INTRINSIC_STRLEN:
					{
						iCount = 1;
						iResult = static_cast<INT64>(strlen(pszArgs));
						break;
					}
					case INTRINSIC_STRMATCH:
					{
						TCHAR *ppArgs[2];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
						if ( iCount == 2 )
							iResult = (Str_Match(ppArgs[0], ppArgs[1]) == MATCH_VALID) ? 1 : 0;
						break;
					}
					case INTRINSIC_STRREGEX:
					{
						TCHAR *ppArgs[2];
						iCount = Str_ParseCmds(const_cast<TCHAR *>(pszArgs), ppArgs, COUNTOF(ppArgs), ",");
						if ( iCount == 2 )
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
						break;
					}
					default:
						break;
				}

				pszArgs = pszArgsNext;

				if ( iCount == 0 )
				{
					DEBUG_ERR(("Bad intrinsic function usage: missing arguments\n"));
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
	if ( strlen(pszOrig) > 1 )
		DEBUG_ERR(("Undefined symbol '%s' ['%s']\n", szTag, pszOrig));
	else
		DEBUG_ERR(("Undefined symbol '%s'\n", szTag));
	return 0;
}

INT64 CExpression::GetValMath(INT64 iVal, LPCTSTR &pszArgs)
{
	ADDTOCALLSTACK("CExpression::GetValMath");
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
			iVal |= GetVal(pszArgs);
			break;
		}
		case '&':
		{
			++pszArgs;
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
				DEBUG_ERR(("Can't modulo by 0\n"));
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
			if ( pszArgs[0] == '>' )
			{
				++pszArgs;
				iVal >>= GetVal(pszArgs);
			}
			break;
		}
		case '<':
		{
			++pszArgs;
			if ( pszArgs[0] == '<' )
			{
				++pszArgs;
				iVal <<= GetVal(pszArgs);
			}
			break;
		}
		case '@':
		{
			++pszArgs;
			INT64 iArgs = GetVal(pszArgs);
			if ( (iVal == 0) && (iArgs < 0) )
			{
				DEBUG_ERR(("Can't raise zero to a negative power\n"));
				break;
			}
			iVal = static_cast<INT64>(pow(static_cast<double>(iVal), static_cast<int>(iArgs)));
			break;
		}
	}
	return iVal;
}

thread_local int g_GetVal_LoopCount = 0;

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

	SkipWhitespace(pszArgs);

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

		SkipWhitespace(pszArgs);

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

	if ( iQty == 1 )
	{
		// Simple value
		return iVals[0];
	}
	else if ( iQty == 2 )
	{
		// Simple range
		return Calc_GetRandLLVal(iVals[0], iVals[1]);
	}
	else
	{
		// Weighted range
		INT64 iTotalWeight = 0;
		int i = 1;
		for ( ; i < iQty; i += 2 )
		{
			if ( iVals[i] <= 0 )
				DEBUG_ERR(("Invalid weight %lld in weighted range\n", iVals[i]));

			iTotalWeight += iVals[i];
		}

		iTotalWeight = Calc_GetRandLLVal(iTotalWeight) + 1;

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
