#include <cmath>
#include "../graysvr/graysvr.h"

typedef double RealType;
#define VARDEF_FLOAT_MAXBUFFERSIZE 82

CVarFloat::CVarFloat()
{
}

CVarFloat::~CVarFloat()
{
	m_VarMap.clear();
}

inline bool CVarFloat::Set( const char* VarName, const char* VarValue )
{
	ADDTOCALLSTACK("CVarFloat::Set");
	return Insert(VarName, VarValue, true);
}

bool CVarFloat::Insert( const char* VarName, const char* VarValue, bool ForceSet) 
{
	ADDTOCALLSTACK("CVarFloat::Insert");
	if (!VarValue || !VarName)
		return false;
	MapType::iterator i = m_VarMap.find(VarName);
	if ( i != m_VarMap.end() && !ForceSet)
		return false;
	SKIP_ARGSEP(VarValue);
	SKIP_ARGSEP(VarName);
	char* pEnd;
	RealType Real = strtod(VarValue,&pEnd);
	m_VarMap[CGString(VarName)] = Real;
	return true;
}

RealType CVarFloat::GetVal( const char* VarName ) 
{
	ADDTOCALLSTACK("CVarFloat::GetVal");
	if ( !VarName )
		return 0.0;
	SKIP_ARGSEP(VarName);
	MapType::iterator i = m_VarMap.find(VarName);
	if ( i == m_VarMap.end())
	{
		return 0.0;
	}
	return i->second;
}

CGString CVarFloat::Get( const char* VarName ) 
{
	ADDTOCALLSTACK("CVarFloat::Get");
	if ( !VarName )
		return CGString();
	SKIP_ARGSEP(VarName);
	if ( strlen(VarName) > VARDEF_FLOAT_MAXBUFFERSIZE )
		return CGString();

	RealType Real = GetVal(VarName);
	char szReal[VARDEF_FLOAT_MAXBUFFERSIZE];
	sprintf(szReal, "%f", Real);

	return CGString(szReal);
}

short int Reentrant_Count = 0;

CGString CVarFloat::FloatMath( LPCTSTR & Expr )
{
	ADDTOCALLSTACK("CVarFloat::FloatMath");
	//DEBUG_ERR(("FloatMath\n"));
	//DEBUG_ERR(("Expr: '%s'\n",Expr));
	char szReal[VARDEF_FLOAT_MAXBUFFERSIZE];
	sprintf(szReal, "%f", MakeFloatMath(Expr));
	return CGString( szReal );
}

RealType CVarFloat::MakeFloatMath( LPCTSTR & Expr )
{
	ADDTOCALLSTACK("CVarFloat::MakeFloatMath");
	//DEBUG_ERR(("MakeFloatMath\n"));
	if ( ! Expr )
		return( 0 );

	GETNONWHITESPACE( Expr );

	++Reentrant_Count;
	if ( Reentrant_Count > 128 )
	{
		DEBUG_WARN(( "Deadlock detected while parsing '%s'. Fix the error in your scripts.\n", Expr ));
		--Reentrant_Count;
		return( 0 );
	}
	//DEBUG_ERR(("Expr: '%s' GetSingle(Expr) '%f' GetValMath(GetSingle(Expr), Expr) '%f'\n",Expr,GetSingle(Expr),GetValMath(GetSingle(Expr), Expr)));
	RealType dVal = GetValMath(GetSingle(Expr), Expr);
	--Reentrant_Count;
	return dVal;
}

RealType CVarFloat::GetValMath( RealType dVal, LPCTSTR & pExpr )
{
	ADDTOCALLSTACK("CVarFloat::GetValMath");
	//DEBUG_ERR(("GetValMath  dVal %f  pExpr %s\n",dVal,pExpr));
	GETNONWHITESPACE(pExpr);

	// Look for math type operator.
	switch ( pExpr[0] )
	{
		case '\0':
			break;
		case ')':  // expression end markers.
		case '}':
		case ']':
			++pExpr;	// consume this.
			break;
		case '+':
			++pExpr;
			dVal += MakeFloatMath( pExpr );
			break;
		case '-':
			++pExpr;
			dVal -= MakeFloatMath( pExpr );
			break;
		case '*':
			++pExpr;
			dVal *= MakeFloatMath( pExpr );
			break;
		case '/':
			++pExpr;
			{
				RealType dTempVal = MakeFloatMath( pExpr );
				if ( ! dTempVal )
				{
					DEBUG_ERR(( "Float_MakeFloatMath: Divide by 0\n" ));
					break;
				}
				dVal /= dTempVal;
			}
			break;
		case '!':
			++pExpr;
			if ( pExpr[0] != '=' )
				break; // boolean ! is handled as a single expresion.
			++pExpr;
			dVal = ( dVal != MakeFloatMath( pExpr ));
			break;
		case '=': // boolean
			while ( pExpr[0] == '=' )
				++pExpr;
			dVal = ( dVal == MakeFloatMath( pExpr ));
			break;
		case '@':
			++pExpr;
			{
				RealType dTempVal = MakeFloatMath( pExpr );
				if ( (dVal == 0) && (dTempVal < 0) )
				{
					DEBUG_ERR(( "Float_MakeFloatMath: Power of zero with negative exponent is undefined\n" ));
					break;
				}
				//DEBUG_ERR(("dVal %f  dTempVal %f  Result %f\n",dVal,dTempVal,pow(dVal, dTempVal)));
				dVal = pow(dVal, dTempVal);
			}
			break;
		//Following operations are not allowed with Double
		case '|':
			++pExpr;
			if ( pExpr[0] == '|' )	// boolean ?
			{
				++pExpr;
				dVal = ( MakeFloatMath( pExpr ) || dVal );
			}
			else	// bitwise
				DEBUG_ERR(("Operator '%s' is not allowed with floats.\n","|"));
			break;
		case '&':
			++pExpr;
			if ( pExpr[0] == '&' )	// boolean ?
			{
				++pExpr;
				dVal = ( MakeFloatMath( pExpr ) && dVal );	// tricky stuff here. logical ops must come first or possibly not get processed.
			}
			else	// bitwise
				DEBUG_ERR(("Operator '%s' is not allowed with floats.\n","&"));
			break;
		case '%':
			++pExpr;
			DEBUG_ERR(("Operator '%s' is not allowed with floats.\n","%"));
			break;
		case '^':
			++pExpr;
			DEBUG_ERR(("Operator '%s' is not allowed with floats.\n","^"));
			break;
		case '>': // boolean
			++pExpr;
			if ( pExpr[0] == '=' )	// boolean ?
			{
				++pExpr;
				dVal = ( dVal >= MakeFloatMath( pExpr ));
			}
			else if ( pExpr[0] == '>' )	// shift
			{
				++pExpr;
				DEBUG_ERR(("Operator '%s' is not allowed with floats.\n",">>"));
			}
			else
			{
				dVal = ( dVal > MakeFloatMath( pExpr ));
			}
			break;
		case '<': // boolean
			++pExpr;
			if ( pExpr[0] == '=' )	// boolean ?
			{
				++pExpr;
				dVal = ( dVal <= MakeFloatMath( pExpr ));
			}
			else if ( pExpr[0] == '<' )	// shift
			{
				++pExpr;
				DEBUG_ERR(("Operator '%s' is not allowed with floats.\n","<<"));
			}
			else
			{
				dVal = ( dVal < MakeFloatMath( pExpr ));
			}
			break;
	}
	return dVal;
}

RealType CVarFloat::GetSingle( LPCTSTR & pArgs )
{
	ADDTOCALLSTACK("CVarFloat::GetSingle");
	//DEBUG_ERR(("GetSingle  pArgs %s\n",pArgs));
	GETNONWHITESPACE( pArgs );
	char * pArgsCopy = new char[strlen(pArgs)+1];
	strcpy(pArgsCopy,pArgs);
	/*bool IsNum = true; // Old Ellessar's code without support for negative numbers
	for( char ch = tolower(*pArgs); ch; ch = tolower(*(++pArgs)) )
	{
		if (( IsDigit( ch ) ) || ( ch == '.' ) || ( ch == ',' ))
			continue;

		if ((( ch >= '*' ) && ( ch <= '/' )) || (( ch == ')' ) || ( ch == ']' )) || ( ch == '@' ))
			break;
		//DEBUG_ERR(("ch '0%x'\n",ch));
		IsNum = false;
		break;
	}*/
	bool IsNum = false;
	for (TCHAR ch = static_cast<TCHAR>(tolower(*pArgs)); ch; ch = static_cast<TCHAR>(tolower(*(++pArgs))))
    {
        if (( IsDigit( ch ) ) || ( ch == '.' ) || ( ch == ',' ))
        {
			if ( IsNum == false)
				IsNum = (IsDigit(ch) != 0);
            continue;
        }

        if ((( ch >= '*' ) && ( ch <= '/' )) || (( ch == ')' ) || ( ch == ']' )) || ( ch == '@' ))
            break;
        //DEBUG_ERR(("ch '0%x'\n",ch));
        // IsNum = false;
        break;
    }
	if ( IsNum )
	{
		char * pEnd;
		RealType ret = strtod(pArgsCopy,&pEnd);
		//DEBUG_ERR(("IsNum: '%d' pArgsCopy '%s' Ret: '%f'\n",IsNum,pArgsCopy,strtod(pArgsCopy,&pEnd)));
		delete[] pArgsCopy;
		return( ret );
	}
	delete[] pArgsCopy;
	switch ( pArgs[0] )
	{
		case '{':
		//	++pArgs;
		//	return( GetRange( pArgs ));
		case '[':
		case '(': // Parse out a sub expression.
			++pArgs;
			return( MakeFloatMath( pArgs ));
		case '+':
			++pArgs;
			break;
		case '-':
			++pArgs;
			return( -GetSingle( pArgs ));
		case '~':	// Bitwise not.
			++pArgs;
			DEBUG_ERR(("Operator '~' is not allowed with floats.\n"));
			return( 0 );
		case '!':	// boolean not.
			++pArgs;
			if ( pArgs[0] == '=' )  // odd condition such as (!=x) which is always true of course.
			{
				++pArgs;		// so just skip it. and compare it to 0
				return( GetSingle( pArgs ));
			}
			return( !GetSingle( pArgs ));
		case ';':	// seperate field.
		case ',':	// seperate field.
		case '\0':
			return( 0 );
	}
	INTRINSIC_TYPE iIntrinsic = (INTRINSIC_TYPE) FindTableHeadSorted( pArgs, sm_IntrinsicFunctions, COUNTOF(sm_IntrinsicFunctions)-1 );
	if ( iIntrinsic >= 0 )
	{
		size_t iLen = strlen(sm_IntrinsicFunctions[iIntrinsic]);
		if ( pArgs[iLen] == '(' )
		{
			pArgs += (iLen + 1);
			TCHAR * pArgsNext;
			Str_Parse( const_cast<TCHAR*>(pArgs), &(pArgsNext), ")" );

			TCHAR * ppCmd[5];
			RealType dResult;
			size_t iCount;
			const char * cparg1 = NULL; //some functions need a const char instead of a char and GCC cannot bear it :)
			const char * cparg2 = NULL; //some functions need a const char instead of a char and GCC cannot bear it :)

			switch ( iIntrinsic )
			{
				case INTRINSIC_ID:
				{
					if ( pArgs && *pArgs )
					{
						iCount = 1;
						dResult = RES_GET_INDEX(static_cast<int>(MakeFloatMath(pArgs))); // RES_GET_INDEX
					}
					else
					{
						iCount = 0;
						dResult = 0;
					}

				} break;

				case INTRINSIC_LOGARITHM:
				{
					iCount = Str_ParseCmds( const_cast<TCHAR*>(pArgs), ppCmd, 3, "," );
					if ( iCount < 1 )
					{
						dResult = 0;
						break;
					} 

					LPCTSTR tCmd = ppCmd[0];
					RealType dArgument = MakeFloatMath( tCmd );

					if ( iCount < 2 )
					{
						dResult = log10(dArgument);
					}
					else
					{
						if ( !strcmpi(ppCmd[1], "e") )
						{
							dResult = log(dArgument);
						}
						else if ( !strcmpi(ppCmd[1], "pi") )
						{
							dResult = log(dArgument) / log(M_PI);
						}
						else
						{
							tCmd = ppCmd[1];
							RealType dBase = MakeFloatMath( tCmd );
							if ( dBase <= 0 )
							{
								DEBUG_ERR(( "Float_MakeFloatMath: (%f)Log(%f) is %s\n", dBase, dArgument, (!dBase) ? "infinite" : "undefined" ));
								iCount = 0;
								dResult = 0;
							}
							else
							{
								dResult = (log(dArgument)/log(dBase));
							}
						}
					}
				} break;

				case INTRINSIC_NAPIERPOW:
				{
					if ( pArgs && *pArgs )
					{
						iCount = 1;
						dResult = exp(MakeFloatMath(pArgs));
					}
					else
					{
						iCount = 0;
						dResult = 0;
					}

				} break;

				case INTRINSIC_SQRT:
				{
					iCount = 0;

					if ( pArgs && *pArgs )
					{
						RealType dTosquare = MakeFloatMath(pArgs);

						if (dTosquare >= 0)
						{
							++iCount;
							dResult = sqrt(dTosquare);
						}
						else
						{
							DEBUG_ERR(( "Float_MakeFloatMath: Sqrt of negative number (%f) is impossible\n", dTosquare ));
							dResult = 0;
						}
					}
					else
					{
						dResult = 0;
					}

				} break;

				case INTRINSIC_SIN:
				{
					if ( pArgs && *pArgs )
					{
						iCount = 1;
						RealType dArgument = MakeFloatMath(pArgs);
						dResult = sin(dArgument * M_PI / 180);
					}
					else
					{
						iCount = 0;
						dResult = 0;
					}

				} break;

				case INTRINSIC_ARCSIN:
				{
					if ( pArgs && *pArgs )
					{
						iCount = 1;
						RealType dArgument = MakeFloatMath(pArgs);
						dResult = asin(dArgument) * 180 / M_PI;
					}
					else
					{
						iCount = 0;
						dResult = 0;
					}

				} break;

				case INTRINSIC_COS:
				{
					if ( pArgs && *pArgs )
					{
						iCount = 1;
						RealType dArgument = MakeFloatMath(pArgs);
						dResult = cos(dArgument * M_PI / 180);
					}
					else
					{
						iCount = 0;
						dResult = 0;
					}

				} break;

				case INTRINSIC_ARCCOS:
				{
					if ( pArgs && *pArgs )
					{
						iCount = 1;
						RealType dArgument = MakeFloatMath(pArgs);
						dResult = acos(dArgument) * 180 / M_PI;
					}
					else
					{
						iCount = 0;
						dResult = 0;
					}

				} break;

				case INTRINSIC_TAN:
				{
					if ( pArgs && *pArgs )
					{
						iCount = 1;
						RealType dArgument = MakeFloatMath(pArgs);
						dResult = tan(dArgument * M_PI / 180);
					}
					else
					{
						iCount = 0;
						dResult = 0;
					}

				} break;

				case INTRINSIC_ARCTAN:
				{
					if ( pArgs && *pArgs )
					{
						iCount = 1;
						RealType dArgument = MakeFloatMath(pArgs);
						dResult = atan(dArgument) * 180 / M_PI;
					}
					else
					{
						iCount = 0;
						dResult = 0;
					}

				} break;

				case INTRINSIC_StrIndexOf:
				{
					iCount = Str_ParseCmds( const_cast<TCHAR*>(pArgs), ppCmd, 3, "," );
					if ( iCount < 2 )
						dResult = -1;
					else
					{
						cparg1 = ppCmd[2];
						dResult = Str_IndexOf(ppCmd[0], ppCmd[1], (iCount == 3)? static_cast<int>(MakeFloatMath(cparg1)) : 0);
					}
				} break;

				case INTRINSIC_STRMATCH:
				{
					iCount = Str_ParseCmds( const_cast<TCHAR*>(pArgs), ppCmd, 2, "," );
					if ( iCount < 2 )
						dResult = 0;
					else
						dResult = (Str_Match( ppCmd[0], ppCmd[1] ) == MATCH_VALID ) ? 1 : 0;
				} break;

				case INTRINSIC_STRREGEX:
				{
					iCount = Str_ParseCmds( const_cast<TCHAR*>(pArgs), ppCmd, 2, "," );
					if ( iCount < 2 )
						dResult = 0;
					else
					{
						TCHAR * tLastError = Str_GetTemp();
						dResult = Str_RegExMatch( ppCmd[0], ppCmd[1], tLastError );
						if ( dResult < 0 )
						{
							DEBUG_ERR(( "STRREGEX bad function usage. Error: %s\n", tLastError ));
						}
					}
				} break;

				case INTRINSIC_RANDBELL:
				{
					iCount = Str_ParseCmds( const_cast<TCHAR*>(pArgs), ppCmd, 2, "," );
					if ( iCount < 2 )
						dResult = 0;
					else
					{
						cparg1 = ppCmd[0];
						cparg2 = ppCmd[1];
						dResult = Calc_GetBellCurve(static_cast<int>(MakeFloatMath(cparg1)), static_cast<int>(MakeFloatMath(cparg2)));
					}
				} break;

				case INTRINSIC_STRASCII:
				{
					if ( pArgs && *pArgs )
					{
						iCount = 1;
						dResult = pArgs[0];
					}
					else
					{
						iCount = 0;
						dResult = 0;
					}
				} break;

				case INTRINSIC_RAND:
				{
					iCount = Str_ParseCmds( const_cast<TCHAR*>(pArgs), ppCmd, 2, "," );
					if ( iCount <= 0 )
						dResult = 0;
					else
					{
						cparg1 = ppCmd[0];
						RealType val1 = MakeFloatMath( cparg1 );
						if ( iCount >= 2 )
						{
							cparg2 = ppCmd[1];
							RealType val2 = MakeFloatMath( cparg2 );
							dResult = GetRandVal2( val1, val2 );
						}
						else
							dResult = GetRandVal(val1);
					}
				} break;

				case INTRINSIC_STRCMP:
				{
					iCount = Str_ParseCmds( const_cast<TCHAR*>(pArgs), ppCmd, 2, "," );
					if ( iCount < 2 )
						dResult = 1;
					else
						dResult = strcmp(ppCmd[0], ppCmd[1]);
				} break;

				case INTRINSIC_STRCMPI:
				{
					iCount = Str_ParseCmds( const_cast<TCHAR*>(pArgs), ppCmd, 2, "," );
					if ( iCount < 2 )
						dResult = 1;
					else
						dResult = strcmpi(ppCmd[0], ppCmd[1]);
				} break;

				case INTRINSIC_STRLEN:
				{
					iCount = 1;
					dResult = strlen(pArgs);
				} break;

				case INTRINSIC_ISOBSCENE:
				{
					iCount = 1;
					dResult = g_Cfg.IsObscene( pArgs );
				} break;
				case INTRINSIC_ISNUMBER:
				{
					iCount = 1;
					{
						char z[64];
						LTOA(atol(pArgs), z, 10);
						dResult = strcmp(pArgs, z) ? 0 : 1;
					}
				} break;

				case INTRINSIC_QVAL:
				{
					iCount = Str_ParseCmds( const_cast<TCHAR*>(pArgs), ppCmd, 5, "," );
					if ( iCount < 3 )
						dResult = 0;
					else
					{
						cparg1 = ppCmd[0];
						cparg2 = ppCmd[1];
						RealType a1 = GetSingle(cparg1);
						RealType a2 = GetSingle(cparg2);
						if ( a1 < a2 )
						{
							cparg1 = ppCmd[2];
							dResult = GetSingle(cparg1);
						}
						else if ( a1 == a2 )
						{
							cparg1 = ppCmd[3];
							dResult = ( iCount < 4 ) ? 0 : GetSingle(cparg1);
						}
						else
						{
							cparg1 = ppCmd[4];
							dResult = ( iCount < 5 ) ? 0 : GetSingle(cparg1);
						}
					}
				} break;

				default:
					iCount = 0;
					dResult = 0;
					break;
			}

			pArgs = pArgsNext;

			if ( iCount <= 0 )
			{
				DEBUG_ERR(( "Bad intrinsic function usage. missing )\n" ));
				return 0;
			}
			else
			{
				return dResult;
			}
		}
	}
	long long lVal;
	if ( g_Exp.m_VarGlobals.GetParseVal( pArgs, &lVal ) )
		return( static_cast<long>(lVal) );
	if ( g_Exp.m_VarDefs.GetParseVal( pArgs, &lVal ) )
		return( static_cast<long>(lVal) );
	return 0;
}

RealType CVarFloat::GetRandVal( RealType dQty )
{
	ADDTOCALLSTACK("CVarFloat::GetRandVal");
	if ( dQty <= 0 )
		return( 0 );
	if ( dQty >= INT_MAX )
	{
		return( static_cast<RealType>(IMULDIV( g_World.m_Rand.randDblExc(), dQty, INT_MAX)) );
	}
	return g_World.m_Rand.randDblExc(dQty);
}

RealType CVarFloat::GetRandVal2( RealType dMin, RealType dMax )
{
	ADDTOCALLSTACK("CVarFloat::GetRandVal2");
	if ( dMin > dMax )
	{
		RealType tmp = dMin;
		dMin = dMax;
		dMax = tmp;
	}
	//DEBUG_ERR(("GetRandVal2\n"));
	return ( dMin + g_World.m_Rand.randDblExc(dMax) ); //These weird numbers are taken from mtrand.cpp (cause calling that function from here spits out some weird external errors)
}

//Does not work as it should, would be too slow, and nobody needs that
/*RealType CVarFloat::GetRange( LPCTSTR & pExpr )
{
	RealType dVals[256];		// Maximum elements in a list

	short int iQty = GetRangeVals( pExpr, dVals, COUNTOF(dVals));

	if (iQty == 0)
	{
		DEBUG_ERR(("1"));
		return( 0 );
	}
	if (iQty == 1) // It's just a simple value
	{
		DEBUG_ERR(("2"));
		return( dVals[0] );
	}
	if (iQty == 2) // It's just a simple range....pick one in range at random
	{
		DEBUG_ERR(("3"));
		return( GetRandVal2( minimum(dVals[0],dVals[1]), maximum(dVals[0],dVals[1]) ) );
	}

	// I guess it's weighted values
	// First get the total of the weights

	RealType dTotalWeight = 0;
	int i = 1;
	for ( ; i < iQty; i+=2 )
	{
		if ( ! dVals[i] )	// having a weight of 0 is very strange !
		{
			DEBUG_ERR(( "Weight of 0 in random set?\n" ));	// the whole table should really just be invalid here !
		}
		dTotalWeight += dVals[i];
	}

	// Now roll the dice to see what value to pick
	dTotalWeight = GetRandVal(dTotalWeight) + 1;
	// Now loop to that value
	i = 1;
	for ( ; i<iQty; i+=2 )
	{
		dTotalWeight -= dVals[i];
		if ( dTotalWeight <= 0 )
			break;
	}

	return( dVals[i-1] );
}

int CVarFloat::GetRangeVals( LPCTSTR & pExpr, RealType * piVals, short int iMaxQty )
{
	ADDTOCALLSTACK("CVarFloat::GetRangeVals");
	// Get a list of values.
	if ( pExpr == NULL )
		return( 0 );

	ASSERT(piVals);

	int iQty = 0;
	for (;;)
	{
		if ( !pExpr[0] ) break;
		if ( pExpr[0] == ';' )
			break;	// seperate field.
		if ( pExpr[0] == ',' )
			++pExpr;

		piVals[iQty] = GetSingle( pExpr );
		if ( ++iQty >= iMaxQty )
			break;
		if ( pExpr[0] == '-' && iQty == 1 )	// range separator. (if directly after, I know this is sort of strange)
		{
			++pExpr;	// ??? This is stupid. get rid of this and clean up it's use in the scripts.
			continue;
		}

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
}*/

CLocalObjMap::CLocalObjMap()
{
}

CLocalObjMap::~CLocalObjMap()
{
	m_ObjMap.clear();
}

CObjBase * CLocalObjMap::Get( unsigned short Number )
{
	ADDTOCALLSTACK("CLocalObjMap::Get");
	if ( !Number )
		return NULL;
	ObjMap::iterator i = m_ObjMap.find(Number);
	if ( i == m_ObjMap.end() )
		return NULL;
	return i->second;
}

bool CLocalObjMap::Insert( unsigned short Number, CObjBase * pObj, bool ForceSet )
{
	ADDTOCALLSTACK("CLocalObjMap::Insert");
	if ( !Number )
		return false;
	ObjMap::iterator i = m_ObjMap.find(Number);
	if ( i != m_ObjMap.end() && !ForceSet )
		return false;
	m_ObjMap[Number] = pObj;
	return true;
}
