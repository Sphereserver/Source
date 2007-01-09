#include "../graysvr.h"
#include <math.h>

#ifndef M_PI 
	#define M_PI 3.14159265358979323846
#endif

TCHAR CExpression::sm_szMessages[DEFMSG_QTY+1][128] =
{
	#define MSG(a,b,c) c,
	#include "../tables/defmessages.tbl"
	#undef MSG
	NULL,
};

LPCTSTR const CExpression::sm_szMsgNames[DEFMSG_QTY+1] =
{
	#define MSG(a,b,c) b,
	#include "../tables/defmessages.tbl"
	#undef MSG
	NULL,
};

int power(int base, int level)
{
	double rc = pow((double)base,(double)level);
	return (int)rc;
}

bool IsStrEmpty( LPCTSTR pszTest )
{
	if ( !pszTest || !*pszTest ) return true;

	do
	{
		if ( !isspace(*pszTest) ) return false;
	}
	while( *(++pszTest) );
	return true;
}

bool IsStrNumericDec( LPCTSTR pszTest )
{
	if ( !pszTest || !*pszTest ) return false;

	do
	{
		if ( !isdigit(*pszTest) ) return false;
	}
	while ( *(++pszTest) );

	return true;
}


bool IsStrNumeric( LPCTSTR pszTest )
{
	if ( !pszTest || !*pszTest )
		return false;
	bool	fHex	= false;
	if ( pszTest[0] == '0' )
		fHex	= true;

	do
	{
		if ( isdigit( *pszTest ) )
			continue;
		if ( fHex && tolower(*pszTest) >= 'a' && tolower(*pszTest) <= 'f' )
			continue;
		return false;
	}
	while ( *(++pszTest) );
	return true;
}


bool IsSimpleNumberString( LPCTSTR pszTest )
{
	// is this a string or a simple numeric expression ?
	// string = 1 2 3, sdf, sdf sdf sdf, 123d, 123 d,
	// number = 1.0+-\*~|&!%^()2, 0aed, 123

	bool fMathSep = true;	// last non whitespace was a math sep.
	bool fHextDigitStart = false;
	bool fWhiteSpace = false;

	for ( ; true; pszTest++ )
	{
		TCHAR ch = *pszTest;
		if ( ! ch )
		{
			return true;
		}
		if (( ch >= 'A' && ch <= 'F') || ( ch >= 'a' && ch <= 'f' ))	// isxdigit
		{
			if ( ! fHextDigitStart )
				return false;
			fWhiteSpace = false;
			fMathSep = false;
			continue;
		}
		if ( isspace( ch ))
		{
			fHextDigitStart = false;
			fWhiteSpace = true;
			continue;
		}
		if ( isdigit( ch ))
		{
			if ( fWhiteSpace && ! fMathSep )
				return false;
			if ( ch == '0' )
			{
				fHextDigitStart = true;
			}
			fWhiteSpace = false;
			fMathSep = false;
			continue;
		}
		if ( ch == '/' && pszTest[1] != '/' )
			fMathSep	= true;
		else
			fMathSep = strchr("+-\\*~|&!%^()", ch ) ? true : false ;

		if ( ! fMathSep )
		{
			return false;
		}
		fHextDigitStart = false;
		fWhiteSpace = false;
	}
}

int GetIdentifierString( TCHAR * szTag, LPCTSTR pszArgs )
{
	// Copy the identifier (valid char set) out to this buffer.
	int i=0;
	int maxLen = config.get("script.expkey.max");
	for ( ;pszArgs[i]; i++ )
	{
		if ( !_ISCSYM(pszArgs[i]))
			break;
		if ( i >= maxLen )
			return NULL;

		szTag[i] = pszArgs[i];
	}

	szTag[i] = '\0';
	return i;
}

/////////////////////////////////////////////////////////////////////////
// -Calculus

int Calc_GetLog2( UINT iVal )
{
	// This is really log2 + 1
	int i=0;
	for ( ; iVal; i++ )
	{
		iVal >>= 1 ;
	}
	return( i );
}

int Calc_GetRandVal( int iqty )
{
	if ( iqty <= 0 )
		return 0;
	if ( iqty > RAND_MAX )
	{
		return( IMULDIV( g_World.m_Rand.randInt(), (DWORD) iqty, RAND_MAX + 1 )) ;
	}
	return( g_World.m_Rand.randInt() % iqty );
}

int Calc_GetRandVal2( int iMin, int iMax )
{
	if ( iMin > iMax )
	{
		int tmp = iMin;
		iMin = iMax;
		iMax = tmp;
	}
	return( iMin + g_World.m_Rand.randInt() % ((iMax - iMin) + 1) );
}

int Calc_GetBellCurve( int iValDiff, int iVariance )
{
	// Produce a log curve.
	//
	// 50+
	//	 |
	//	 |
	//	 |
	// 25|  +
	//	 |
	//	 |	   +
	//	 |		  +
	//	0 --+--+--+--+------
	//    iVar				iValDiff
	//
	// ARGS:
	//  iValDiff = Given a value relative to 0
	//		0 = 50.0% chance.
	//  iVariance = the 25.0% point of the bell curve
	// RETURN:
	//  (0-100.0) % chance at this iValDiff.
	//  Chance gets smaller as Diff gets bigger.
	// EXAMPLE:
	//  if ( iValDiff == iVariance ) return( 250 )
	//  if ( iValDiff == 0 ) return( 500 );
	//

	if ( iVariance <= 0 )	// this really should not happen but just in case.
		return( 500 );
	if ( iValDiff < 0 ) iValDiff = -iValDiff;

	int iChance = 500;
	while ( iValDiff > iVariance && iChance )
	{
		iValDiff -= iVariance;
		iChance /= 2;	// chance is halved for each Variance period.
	}

	return( iChance - IMULDIV( iChance/2, iValDiff, iVariance ));
}

int Calc_GetSCurve( int iValDiff, int iVariance )
{
	// ARGS:
	//   iValDiff = Difference between our skill level and difficulty.
	//		positive = high chance, negative = lower chance
	//		0 = 50.0% chance.
	//   iVariance = the 25.0% difference point of the bell curve
	// RETURN:
	//	 what is the (0-100.0)% chance of success = 0-1000
	// NOTE:
	//   Chance of skill gain is inverse to chance of success.
	//
	int iChance = Calc_GetBellCurve( iValDiff, iVariance );
	if ( iValDiff > 0 )
		return( 1000 - iChance );
	return( iChance );
}

///////////////////////////////////////////////////////////////
// -CExpression

CExpression::CExpression()
{
}

CExpression::~CExpression()
{
}

enum INTRINSIC_TYPE
{
	INTRINSIC_ID = 0,
	INTRINSIC_ISNUMBER,
	INTRINSIC_LOGARITHM,
	INTRINSIC_NAPIERPOW,
	INTRINSIC_QVAL,
	INTRINSIC_RAND,
	INTRINSIC_RANDBELL,
	INTRINSIC_SQRT,
	INTRINSIC_STRASCII,
	INTRINSIC_STRCMP,
	INTRINSIC_STRCMPI,
	INTRINSIC_StrIndexOf,
	INTRINSIC_STRLEN,
	INTRINSIC_STRMATCH,
	INTRINSIC_TAN,
	INTRINSIC_QTY,
};

static LPCTSTR const sm_IntrinsicFunctions[INTRINSIC_QTY+1] =
{
	"ID",		// ID(x) = truncate the type portion of an Id
	"ISNUMBER",		// ISNUMBER(var)
	"LOGARITHM",	// log()/log10()
	"NAPIERPOW",	// exp()
	"QVAL",		// QVAL(test1,test2,ret1,ret2,ret3) - test1 ? test2 (< ret1, = ret2, > ret3)
	"RAND",		// RAND(x) = flat random
	"RANDBELL",	// RANDBELL(center,variance25)
	"SQRT",		// sqrt()
	"StrAscii",
	"STRCMP",	// STRCMP(str1,str2)
	"STRCMPI",	// STRCMPI(str1,str2)
	"StrIndexOf", // StrIndexOf(string,searchVal,[index]) = find the index of this, -1 = not here.
	"STRLEN",	// STRLEN(str)
	"STRMATCH",	// STRMATCH(str,*?pattern)
	"TAN",		// tan()
	NULL,
};

int CExpression::GetSingle( LPCTSTR & pszArgs )
{
	// Parse just a single expression without any operators or ranges.
	GETNONWHITESPACE( pszArgs );
	
	if (pszArgs[0]=='.') pszArgs++;

	if ( pszArgs[0] == '0' )	// leading '0' = hex value.
	{
		// A hex value.
		if ( pszArgs[1] == '.' )	// leading 0. means it really is decimal.
		{
			pszArgs += 2;
			goto try_dec;
		}

		LPCTSTR pStart = pszArgs;
		DWORD val = 0;
		while ( true )
		{
			TCHAR ch = *pszArgs;
			if ( isdigit( ch ))
				ch -= '0';
			else
			{
				ch = tolower(ch);
				if ( ch > 'f' || ch <'a' )
				{
					if ( ch == '.' && pStart[0] != '0' )	// ok i'm confused. it must be decimal.
					{
						pszArgs = pStart;
						goto try_dec;
					}
					break;
				}
				ch -= 'a' - 10;
			}
			val *= 0x10;
			val += ch;
			pszArgs ++;
		}
		return( val );
	}
	else if ( pszArgs[0] == '.' || isdigit(pszArgs[0]))
	{
		// A decminal number
try_dec:
		long iVal = 0;
		for ( ;true; pszArgs++ )
		{
			if ( *pszArgs == '.' )
				continue;	// just skip this.
			if ( ! isdigit( *pszArgs ))
				break;
			iVal *= 10;
			iVal += *pszArgs - '0';
		}
		return( iVal );
	}
	else if ( ! _ISCSYMF(pszArgs[0]))
	{
		// some sort of math op ?

		switch ( pszArgs[0] )
		{
		case '{':
			pszArgs ++;
			return( GetRange( pszArgs ));
		case '[':
		case '(': // Parse out a sub expression.
			pszArgs ++;
			return( GetVal( pszArgs ));
		case '+':
			pszArgs++;
			break;
		case '-':
			pszArgs++;
			return( -GetSingle( pszArgs ));
		case '~':	// Bitwise not.
			pszArgs++;
			return( ~GetSingle( pszArgs ));
		case '!':	// boolean not.
			pszArgs++;
			if ( pszArgs[0] == '=' )  // odd condition such as (!=x) which is always true of course.
			{
				pszArgs++;		// so just skip it. and compare it to 0
				return( GetSingle( pszArgs ));
			}
			return( !GetSingle( pszArgs ));
		case ';':	// seperate field.
		case ',':	// seperate field.
		case '\0':
			return 0;
		}
	}
	else
	{
		// Symbol or intrinsinc function ?

		INTRINSIC_TYPE iIntrinsic = (INTRINSIC_TYPE) FindTableHeadSorted( pszArgs, sm_IntrinsicFunctions, COUNTOF(sm_IntrinsicFunctions)-1 );
		if ( iIntrinsic >= 0 )
		{
			int iLen = strlen(sm_IntrinsicFunctions[iIntrinsic]);
			if ( pszArgs[iLen] == '(' )
			{
				pszArgs += iLen+1;
				TCHAR * pszArgsNext;
				Str_Parse( const_cast<TCHAR*>(pszArgs), &(pszArgsNext), ")" );
	
				TCHAR * ppCmd[5];
				int iResult;
				int iCount;
	
				switch ( iIntrinsic )
				{
					case INTRINSIC_ID:
					{
						if ( pszArgs && *pszArgs )
						{
							iCount = 1;
							iResult = RES_GET_INDEX( GetVal( pszArgs )); // RES_GET_INDEX
						}
						else
							iCount = 0;

					} break;

					case INTRINSIC_LOGARITHM:
					{
						iCount = 0;

						if ( pszArgs && *pszArgs )
						{
							int iArgument = GetVal(pszArgs);
							if ( iArgument <= 0 )
							{
								g_Log.Error("Exp_GetVal: (x)Log(%d) is %s\n", iArgument, (!iArgument) ? "infinite" : "undefined");
							}
							else
							{
								iCount = 1;

								if ( strchr(pszArgs, ',') )
								{
									iCount++; SKIP_ARGSEP(pszArgs);
									if ( !strcmpi(pszArgs, "e") )
									{
										iResult = (int) log((double)iArgument);
									}
									else if ( !strcmpi(pszArgs, "pi") )
									{
										iResult = (int) (log((double)iArgument)/log(M_PI));
									}
									else
									{
										int iBase = GetVal(pszArgs);
										if ( iBase <= 0 )
										{
											g_Log.Error("Exp_GetVal: (%d)Log(%d) is %s\n", iBase, iArgument, (!iBase) ? "infinite" : "undefined");
											iCount = 0;
										}
										else
										{
											iResult = (int) (log((double)iArgument)/log((double)iBase));
										}
									}
								}
								else
								{
									iResult = (int) log10((double)iArgument);
								}							
							}
						}

					} break;

					case INTRINSIC_NAPIERPOW:
					{
						if ( pszArgs && *pszArgs )
						{
							iCount = 1;
							iResult = (int) exp((double)GetVal(pszArgs));
						}
						else
							iCount = 0;

					} break;

					case INTRINSIC_SQRT:
					{
						iCount = 0;

						if ( pszArgs && *pszArgs )
						{
							int iTosquare = GetVal(pszArgs);

							if (iTosquare >= 0)
							{
								iCount++;
								iResult = (int) sqrt((double)iTosquare);
							}
							else
							{
								g_Log.Error("Exp_GetVal: Sqrt of negative number (%d) is impossible\n", iTosquare);
							}
						}

					} break;

					case INTRINSIC_TAN:
					{
						if ( pszArgs && *pszArgs )
						{
							iCount = 1;
							iResult = (int) tan((double)GetVal(pszArgs));
						}
						else
							iCount = 0;

					} break;
					
					case INTRINSIC_StrIndexOf:
					{
						iCount = Str_ParseCmds( const_cast<TCHAR*>(pszArgs), ppCmd, 3, "," );
						if ( iCount < 2 )
							iResult = -1;
						else
						{
							int offset = (iCount==3) ? GetVal(ppCmd[2]) : 0;
							int hijack = strlen(ppCmd[1]);
							int baseLength = strlen(ppCmd[1]);
							if (( offset < 0 ) || ( offset >= baseLength ) || ( hijack > baseLength ))
								iResult = -1;
							for ( iResult = offset; iResult < baseLength; iResult++ )
							{
								if ( !strnicmp(ppCmd[0], ppCmd[1], hijack) )
								{
									break;
								}
							}
							if ( iResult == baseLength )
								iResult = -1;
						}
					} break;

					case INTRINSIC_STRMATCH:
					{
						iCount = Str_ParseCmds( const_cast<TCHAR*>(pszArgs), ppCmd, 2, "," );
						if ( iCount < 2 )
							iResult = 0;
						else
							iResult = (Str_Match( ppCmd[0], ppCmd[1] ) == MATCH_VALID ) ? 1 : 0;
					} break;

					case INTRINSIC_RANDBELL:
					{
						iCount = Str_ParseCmds( const_cast<TCHAR*>(pszArgs), ppCmd, 2, "," );
						if ( iCount < 2 )
							iResult = 0;
						else
							iResult = Calc_GetBellCurve( GetVal( ppCmd[0] ), GetVal( ppCmd[1] ));
					} break;

					case INTRINSIC_STRASCII:
					{
						if ( pszArgs && *pszArgs )
						{
							iCount = 1;
							iResult = pszArgs[0];
						}
						else
							iCount = 0;
					} break;

					case INTRINSIC_RAND:
					{
						iCount = Str_ParseCmds( const_cast<TCHAR*>(pszArgs), ppCmd, 2, "," );
						if ( !iCount )
							iResult = 0;
						else
						{
							int val1 = GetVal( ppCmd[0] );
							if ( iCount == 2 )
							{
								int val2 = GetVal( ppCmd[1] );
								iResult = Calc_GetRandVal2( val1, val2 );
							}
							else
								iResult = Calc_GetRandVal(val1);
						}
					} break;

					case INTRINSIC_STRCMP:
					{
						iCount = Str_ParseCmds( const_cast<TCHAR*>(pszArgs), ppCmd, 2, "," );
						if ( iCount < 2 )
							iResult = 1;
						else
							iResult = strcmp(ppCmd[0], ppCmd[1]);
					} break;

					case INTRINSIC_STRCMPI:
					{
						iCount = Str_ParseCmds( const_cast<TCHAR*>(pszArgs), ppCmd, 2, "," );
						if ( iCount < 2 )
							iResult = 1;
						else
							iResult = strcmpi(ppCmd[0], ppCmd[1]);
					} break;

					case INTRINSIC_STRLEN:
					{
						iCount = 1;
						iResult = strlen(pszArgs);
					} break;

					case INTRINSIC_ISNUMBER:
					{
						iCount = 1;
						{
							char z[64];
							LTOA(atol(pszArgs), z, 10);
							iResult = strcmp(pszArgs, z) ? 0 : 1;
						}
					} break;

					case INTRINSIC_QVAL:
					{
						iCount = Str_ParseCmds( const_cast<TCHAR*>(pszArgs), ppCmd, 5, "," );
						if ( iCount < 3 )
							iResult = 0;
						else
						{
							int a1 = GetSingle(ppCmd[0]);
							int a2 = GetSingle(ppCmd[1]);
							if ( a1 < a2 ) iResult = GetSingle(ppCmd[2]);
							else if ( a1 == a2 ) iResult = ( iCount < 4 ) ? 0 : GetSingle(ppCmd[3]);
							else iResult = ( iCount < 5 ) ? 0 : GetSingle(ppCmd[4]);
						}
					} break;
				}

				pszArgs = pszArgsNext;

				if ( ! iCount )
				{
					g_Log.Error("Bad intrinsic function usage. missing )\n");
					return 0;
				}
				else
				{
					return iResult;
				}
			}
		}

		// Must be a symbol of some sort ?
		long lVal;
		if ( m_VarGlobals.GetParseVal( pszArgs, &lVal ) )
			return( lVal );
		if ( m_VarDefs.GetParseVal( pszArgs, &lVal ) )
			return( lVal );
	}

	// hard end ! Error of some sort.
	TCHAR szTag[256];
	int i = GetIdentifierString( szTag, pszArgs );
	pszArgs += i;	// skip it.

	g_Log.Error("Undefined symbol '%s'\n", szTag);
	return 0;
}

int CExpression::GetValMath( int lVal, LPCTSTR & pExpr )
{
	GETNONWHITESPACE(pExpr);

	// Look for math type operator.
	switch ( pExpr[0] )
	{
		case '\0':
			break;
		case ')':  // expression end markers.
		case '}':
		case ']':
			pExpr++;	// consume this.
			break;
		case '+':
			pExpr++;
			lVal += GetVal( pExpr );
			break;
		case '-':
			pExpr++;
			lVal -= GetVal( pExpr );
			break;
		case '*':
			pExpr++;
			lVal *= GetVal( pExpr );
			break;
		case '|':
			pExpr++;
			if ( pExpr[0] == '|' )	// boolean ?
			{
				pExpr++;
				lVal = ( GetVal( pExpr ) || lVal );
			}
			else	// bitwise
			{
				lVal |= GetVal( pExpr );
			}
			break;
		case '&':
			pExpr++;
			if ( pExpr[0] == '&' )	// boolean ?
			{
				pExpr++;
				lVal = ( GetVal( pExpr ) && lVal );	// tricky stuff here. logical ops must come first or possibly not get processed.
			}
			else	// bitwise
			{
				lVal &= GetVal( pExpr );
			}
			break;
		case '/':
			pExpr++;
			{
				long iVal = GetVal( pExpr );
				if ( ! iVal )
				{
					g_Log.Error( "Exp_GetVal: Divide by 0\n" );
					break;
				}
				lVal /= iVal;
			}
			break;
		case '%':
			pExpr++;
			{
				long iVal = GetVal( pExpr );
				if ( ! iVal )
				{
					g_Log.Error( "Exp_GetVal: Divide by 0\n" );
					break;
				}
				lVal %= iVal;
			}
			break;
		case '^':
			pExpr ++;
			lVal ^= GetVal(pExpr);
			break;
		case '>': // boolean
			pExpr++;
			if ( pExpr[0] == '=' )	// boolean ?
			{
				pExpr++;
				lVal = ( lVal >= GetVal( pExpr ));
			}
			else if ( pExpr[0] == '>' )	// shift
			{
				pExpr++;
				lVal >>= GetVal( pExpr );
			}
			else
			{
				lVal = ( lVal > GetVal( pExpr ));
			}
			break;
		case '<': // boolean
			pExpr++;
			if ( pExpr[0] == '=' )	// boolean ?
			{
				pExpr++;
				lVal = ( lVal <= GetVal( pExpr ));
			}
			else if ( pExpr[0] == '<' )	// shift
			{
				pExpr++;
				lVal <<= GetVal( pExpr );
			}
			else
			{
				lVal = ( lVal < GetVal( pExpr ));
			}
			break;
		case '!':
			pExpr ++;
			if ( pExpr[0] != '=' )
				break; // boolean ! is handled as a single expresion.
			pExpr ++;
			lVal = ( lVal != GetVal( pExpr ));
			break;
		case '=': // boolean
			while ( pExpr[0] == '=' )
				pExpr ++;
			lVal = ( lVal == GetVal( pExpr ));
			break;
		case '@':
			pExpr++;
			{
				long powerLevel = GetVal(pExpr);
				if ( (lVal == 0) && (powerLevel < 0) )
				{
					g_Log.Error("Exp_GetVal: Power of zero with negative exponent is undefined\n");
					break;
				}
				lVal = (long)pow((double)lVal, powerLevel);
			}
			break;
	}

	return lVal;
}

int CExpression::GetVal( LPCTSTR & pExpr )
{
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
	//		complex ranges must be in {}
	//	{ 3 6}						// simple range
	//	{ 400 1 401 1 } 				// weighted values (2nd val = 1)
	//	{ 1102 1148 1 }					// weighted range (3rd val < 10)
	//	{ animal_colors 1 no_colors 1 } 		// weighted range
	//	{ red_colors 1 {34 39} 1 }			// same (red_colors expands to a range)

	if ( pExpr == NULL )
		return 0;

	GETNONWHITESPACE(pExpr);

	CThread *thread = CThread::Thread();
	if ( thread )
	{
		if ( thread->DL(E_DL_GETSINGLE) >= 128 )
		{
			g_Log.Warn("Deadlock detected while parsing '%s'. Fix the error in your definitions.\n", pExpr);
			return 0;
		}
		thread->DL(E_DL_GETSINGLE, thread->DL(E_DL_GETSINGLE) + 1);
	}
	int lVal = GetValMath(GetSingle(pExpr), pExpr);
	if ( thread )
	{
		thread->DL(E_DL_GETSINGLE, thread->DL(E_DL_GETSINGLE) - 1);
	}
	return lVal;
}

int CExpression::GetRangeVals( LPCTSTR & pExpr, int * piVals, int iMaxQty )
{
	// Get a list of values.
	if ( pExpr == NULL )
		return 0;

	int iQty = 0;
	while (true)
	{
		if ( !pExpr[0] ) break;
		if ( pExpr[0] == ';' )
			break;	// seperate field.
		if ( pExpr[0] == ',' )
			pExpr++;

		piVals[iQty] = GetSingle( pExpr );
		if ( ++iQty >= iMaxQty )
			break;

		GETNONWHITESPACE(pExpr);

		// Look for math type operator.
		switch ( pExpr[0] )
		{
			case ')':  // expression end markers.
			case '}':
			case ']':
				pExpr++;	// consume this and end.
				return( iQty );
	
			case '+':
			case '*':
			case '/':
			case '%':
			// case '^':
			case '<':
			case '>':
			case '|':
			case '&':
				piVals[iQty-1] = GetValMath( piVals[iQty-1], pExpr );
				break;
		}
	}

	return( iQty );
}

int CExpression::GetRange( LPCTSTR & pExpr )
{
	int lVals[256];		// Maximum elements in a list

	int iQty = GetRangeVals( pExpr, lVals, COUNTOF(lVals));

	if (iQty == 0)
	{
		return 0;
	}
	if (iQty == 1) // It's just a simple value
	{
		return( lVals[0] );
	}
	if (iQty == 2) // It's just a simple range....pick one in range at random
	{
		return( Calc_GetRandVal2( min(lVals[0],lVals[1]), max(lVals[0],lVals[1]) ) );
	}

	// I guess it's weighted values
	// First get the total of the weights

	int iTotalWeight = 0;
	int i = 1;
	for ( ; i < iQty; i+=2 )
	{
		if ( !lVals[i] )	// having a weight of 0 is very strange !
			g_Log.Error("Weight of 0 in random set?\n");	// the whole table should really just be invalid here !

		iTotalWeight += lVals[i];
	}

	// Now roll the dice to see what value to pick
	iTotalWeight = Calc_GetRandVal(iTotalWeight) + 1;
	// Now loop to that value
	i = 1;
	for ( ; i<iQty; i+=2 )
	{
		iTotalWeight -= lVals[i];
		if ( iTotalWeight <= 0 )
			break;
	}

	return( lVals[i-1] );
}
