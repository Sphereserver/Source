//
// CResourceDef.cpp
// Copyright Menace Software (www.menasoft.com).
// A variety of resource blocks.
//
#include "graysvr.h"	// predef header.
#include "../common/CAssoc.h"

//*******************************************
// -CValueRangeDef

int CValueRangeDef::GetRandomLinear( int iPercent ) const
{
	ADDTOCALLSTACK("CValueRangeDef::GetRandomLinear");
	return( ( GetRandom() + GetLinear(iPercent) ) / 2 ); 
}

bool CValueRangeDef::Load( TCHAR * pszDef )
{
	ADDTOCALLSTACK("CValueRangeDef::Load");
	// Typically in {lo# hi#} format. is hi#,lo# is ok too ???
	int piVal[2];
	int iQty = g_Exp.GetRangeVals( pszDef, piVal, COUNTOF(piVal));
	if ( iQty< 0 ) 
		return(false);

	m_iLo = piVal[0];
	if ( iQty > 1 )
	{
		m_iHi = piVal[1];
	}
	else
	{
		m_iHi = m_iLo;
	}
	return( true );
}

const TCHAR * CValueRangeDef::Write() const
{
	ADDTOCALLSTACK("CValueRangeDef::Write");
	return( NULL );
}

//*******************************************
// -CValueCurveDef

const TCHAR * CValueCurveDef::Write() const
{
	ADDTOCALLSTACK("CValueCurveDef::Write");
	TCHAR * pszOut = Str_GetTemp();
	size_t j = 0;
	size_t iQty = m_aiValues.GetCount();
	for ( size_t i = 0; i < iQty; i++ )
	{
		if ( i > 0 )
			pszOut[j++] = ',';

		j += sprintf( pszOut + j, "%d", m_aiValues[i] );
	}
	pszOut[j] = '\0';
	return pszOut;
}

bool CValueCurveDef::Load( TCHAR * pszDef )
{
	ADDTOCALLSTACK("CValueCurveDef::Load");
	// ADV_RATE = Chance at 0, to 100.0
	INT64 Arg_piCmd[101];
	size_t iQty = Str_ParseCmds( pszDef, Arg_piCmd, COUNTOF(Arg_piCmd));
	m_aiValues.SetCount( iQty );
	if ( iQty == 0 )
	{
		return( false );
	}
	for ( size_t i = 0; i < iQty; i++ )
	{
		m_aiValues[i] = static_cast<int>(Arg_piCmd[i]);
	}
	return( true );
}

int CValueCurveDef::GetLinear( int iSkillPercent ) const
{
	ADDTOCALLSTACK("CValueCurveDef::GetLinear");
	//
	// ARGS:
	//  iSkillPercent = 0 - 1000 = 0 to 100.0 percent
	//  m_Rate[3] = the 3 advance rate control numbers, 100,50,0 skill levels
	//		Acts as line segments.
	// RETURN:
	//  raw chance value.

	size_t iSegSize;
	size_t iLoIdx;

	size_t iQty = m_aiValues.GetCount();
	switch (iQty)
	{
	case 0:
		return( 0 );	// no values defined !
	case 1:
		return( m_aiValues[0] );
	case 2:
		iLoIdx = 0;
		iSegSize = 1000;
		break;
	case 3:
		// Do this the fastest.
		if ( iSkillPercent >= 500 )
		{
			iLoIdx = 1;
			iSkillPercent -= 500;
		}
		else
		{
			iLoIdx = 0;
		}
		iSegSize = 500;
		break;
	default:
		// More
		iLoIdx = IMULDIV( iSkillPercent, iQty, 1000 );
		iQty--;
		if ( iLoIdx >= iQty )
			iLoIdx = iQty - 1;
		iSegSize = 1000 / iQty;
		iSkillPercent -= ( iLoIdx * iSegSize );
		break;
	}

	int iLoVal = m_aiValues[iLoIdx];
	int iHiVal = m_aiValues[iLoIdx + 1];
	int iChance = iLoVal + IMULDIV( iHiVal - iLoVal, static_cast<int>(iSkillPercent), static_cast<int>(iSegSize) );

	if ( iChance <= 0 )
		return 0; // less than no chance ?

	return( iChance );
}

int CValueCurveDef::GetRandom( ) const
{
	ADDTOCALLSTACK("CValueCurveDef::GetRandom");
	return GetLinear( Calc_GetRandVal( 1000 ) );
}


int CValueCurveDef::GetRandomLinear( int iSkillPercent  ) const
{
	ADDTOCALLSTACK("CValueCurveDef::GetRandomLinear");
	return ( GetLinear( iSkillPercent ) + GetRandom() ) / 2;
}

int CValueCurveDef::GetChancePercent( int iSkillPercent ) const
{
	ADDTOCALLSTACK("CValueCurveDef::GetChancePercent");
	// ARGS:
	//  iSkillPercent = 0 - 1000 = 0 to 100.0 percent
	//
	//  m_Rate[3] = the 3 advance rate control numbers, 0,50,100 skill levels
	//   (How many uses for a gain of .1 (div by 100))
	// RETURN:
	//  percent chance of success * 10 = 0 - 1000.

	// How many uses for a gain of .1 (div by 100)
	int iChance = GetLinear( iSkillPercent );
	if ( iChance <= 0 )
		return 0; // less than no chance ?
	// Express uses as a percentage * 10.
	return( 100000 / iChance );
}

//*******************************************
// -CSkillDef

LPCTSTR const CSkillDef::sm_szTrigName[SKTRIG_QTY+1] = 
{
	"@AAAUNUSED",
	"@ABORT",
	"@FAIL",	// we failed the skill check.
	"@GAIN",
	"@PRESTART",
	"@SELECT",	// just selecting params for the skill
	"@START",	// params for skill are done.
	"@STROKE",
	"@SUCCESS",
	"@TargetCancel",
	"@UseQuick",
	"@Wait",
	NULL,
};

enum SKC_TYPE
{
	SKC_ADV_RATE,
	SKC_BONUS_DEX,
	SKC_BONUS_INT,
	SKC_BONUS_STATS,
	SKC_BONUS_STR,
	SKC_DEFNAME,
	SKC_DELAY,
	SKC_EFFECT,
	SKC_FLAGS,
	SKC_GAINRADIUS,
    SKC_GROUP,
	SKC_KEY,
	SKC_NAME,
	SKC_PROMPT_MSG,
	SKC_STAT_DEX,
	SKC_STAT_INT,
	SKC_STAT_STR,
	SKC_TITLE,
	SKC_VALUES,
	SKC_QTY
};

LPCTSTR const CSkillDef::sm_szLoadKeys[SKC_QTY+1] =
{
	"ADV_RATE",
	"BONUS_DEX",
	"BONUS_INT",
	"BONUS_STATS",
	"BONUS_STR",
	"DEFNAME",
	"DELAY",
	"EFFECT",
	"FLAGS",
	"GAINRADIUS",
	"GROUP",
	"KEY",
	"NAME",
	"PROMPT_MSG",
	"STAT_DEX",
	"STAT_INT",
	"STAT_STR",
	"TITLE",
	"VALUES",
	NULL
};

CSkillDef::CSkillDef( SKILL_TYPE skill ) :
	CResourceLink( RESOURCE_ID( RES_SKILL, skill ))
{
	m_StatPercent	= 0;
	m_GainRadius	= 0;
	m_dwFlags		= 0;
	m_dwGroup		= 0;
	memset(m_Stat, 0, sizeof(m_Stat));
	memset(m_StatBonus, 0, sizeof(m_StatBonus));
	m_AdvRate.Init();
}

bool CSkillDef::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CSkillDef::r_WriteVal");
	EXC_TRY("WriteVal");
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	case SKC_ADV_RATE:	// ADV_RATE=Chance at 100, Chance at 50, chance at 0
		sVal = m_AdvRate.Write();
		break;
	case SKC_BONUS_DEX: // "BONUS_DEX"
		sVal.FormatVal( m_StatBonus[STAT_DEX] );
		break;
	case SKC_BONUS_INT: // "BONUS_INT"
		sVal.FormatVal( m_StatBonus[STAT_INT] );
		break;
	case SKC_BONUS_STR: // "BONUS_STR"
		sVal.FormatVal( m_StatBonus[STAT_STR] );
		break;
	// case SKC_DEFNAME: // "DEFNAME"
	case SKC_DELAY:
		sVal = m_Delay.Write();
		break;
	case SKC_EFFECT:
		sVal = m_Effect.Write();
		break;
	case SKC_FLAGS:
		sVal.FormatHex( m_dwFlags );
		break;
	case SKC_GROUP:
		sVal.FormatHex( m_dwGroup );
		break;
	case SKC_KEY: // "KEY"
		sVal = m_sKey;
		break;
	case SKC_NAME: // "NAME"
		sVal = m_sName.IsEmpty() ? m_sKey : m_sName;
		break;
	case SKC_PROMPT_MSG: // "PROMPT_MSG"
		sVal = m_sTargetPrompt;
		break;
	case SKC_BONUS_STATS: // "BONUS_STATS"
		sVal.FormatVal( m_StatPercent );
		break;
	case SKC_STAT_DEX: // "STAT_DEX"
		sVal.FormatVal( m_Stat[STAT_DEX] );
		break;
	case SKC_STAT_INT: // "STAT_INT"
		sVal.FormatVal( m_Stat[STAT_INT] );
		break;
	case SKC_STAT_STR: // "STAT_STR"
		sVal.FormatVal( m_Stat[STAT_STR] );
		break;
	case SKC_TITLE: // "TITLE"
		sVal = m_sTitle;
		break;
	case SKC_VALUES: // VALUES = 100,50,0 price levels.
		sVal = m_Values.Write();
		break;
	case SKC_GAINRADIUS: // "GAINRADIUS"
		sVal.FormatVal( m_GainRadius );
		break;
   default:
		return( CResourceDef::r_WriteVal( pszKey, sVal, pSrc ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CSkillDef::r_LoadVal( CScript &s )
{
	ADDTOCALLSTACK("CSkillDef::r_LoadVal");
	EXC_TRY("LoadVal");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	case SKC_ADV_RATE:	// ADV_RATE=Chance at 100, Chance at 50, chance at 0
		m_AdvRate.Load( s.GetArgStr());
		break;
	case SKC_BONUS_DEX: // "BONUS_DEX"
		m_StatBonus[STAT_DEX] = static_cast<unsigned char>(s.GetArgVal());
		break;
	case SKC_BONUS_INT: // "BONUS_INT"
		m_StatBonus[STAT_INT] = static_cast<unsigned char>(s.GetArgVal());
		break;
	case SKC_BONUS_STR: // "BONUS_STR"
		m_StatBonus[STAT_STR] = static_cast<unsigned char>(s.GetArgVal());
		break;
	case SKC_DEFNAME: // "DEFNAME"
		return SetResourceName( s.GetArgStr());
	case SKC_DELAY:
		m_Delay.Load( s.GetArgStr());
		break;
	case SKC_FLAGS:
		m_dwFlags = s.GetArgVal();
		break;
	case SKC_GROUP:
		m_dwGroup = s.GetArgVal();
		break;
	case SKC_EFFECT:
		m_Effect.Load( s.GetArgStr());
		break;
	case SKC_KEY: // "KEY"
		m_sKey = s.GetArgStr();
		return SetResourceName( m_sKey );
	case SKC_NAME: // "KEY"
		m_sName = s.GetArgStr();
		break;
	case SKC_PROMPT_MSG: // "PROMPT_MSG"
		m_sTargetPrompt = s.GetArgStr();
		break;
	case SKC_BONUS_STATS: // "BONUS_STATS"
		m_StatPercent = static_cast<unsigned char>(s.GetArgVal());
		break;
	case SKC_STAT_DEX: // "STAT_DEX"
		m_Stat[STAT_DEX] = static_cast<unsigned char>(s.GetArgVal());
		break;
	case SKC_STAT_INT: // "STAT_INT"
		m_Stat[STAT_INT] = static_cast<unsigned char>(s.GetArgVal());
		break;
	case SKC_STAT_STR: // "STAT_STR"
		m_Stat[STAT_STR] = static_cast<unsigned char>(s.GetArgVal());
		break;
	case SKC_TITLE: // "TITLE"
		m_sTitle = s.GetArgStr();
		break;
	case SKC_VALUES: // VALUES = 100,50,0 price levels.
		m_Values.Load( s.GetArgStr() );
		break;
	case SKC_GAINRADIUS: // "GAINRADIUS"
		m_GainRadius = s.GetArgVal();
		break;
	
	default:
		return( CResourceDef::r_LoadVal( s ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}



//*******************************************
// -CSkillClassDef

enum SCC_TYPE
{
	SCC_DEFNAME,
	SCC_NAME,
	SCC_SKILLSUM,
	SCC_STATSUM,
	SCC_QTY
};

LPCTSTR const CSkillClassDef::sm_szLoadKeys[SCC_QTY+1] =
{
	"DEFNAME",
	"NAME",
	"SKILLSUM",
	"STATSUM",
	NULL
};

void CSkillClassDef::Init()
{
	ADDTOCALLSTACK("CSkillClassDef::Init");
	m_SkillSumMax = 10*1000;
	m_StatSumMax = 300;
	size_t i;
	for ( i = 0; i < COUNTOF(m_SkillLevelMax); i++ )
	{
		m_SkillLevelMax[i] = 1000;
	}
	for ( i = 0; i < COUNTOF(m_StatMax); i++ )
	{
		m_StatMax[i] = 100;
	}
}

bool CSkillClassDef::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CSkillClassDef::r_WriteVal");
	EXC_TRY("WriteVal");
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	case SCC_NAME: // "NAME"
		sVal = m_sName;
		break;
	case SCC_SKILLSUM:
		sVal.FormatVal( m_SkillSumMax );
		break;
	case SCC_STATSUM:
		sVal.FormatVal( m_StatSumMax );
		break;
	default:
		{
			int i = g_Cfg.FindSkillKey( pszKey);
			if ( i != SKILL_NONE )
			{
				ASSERT( i >= 0 && static_cast<size_t>(i) < COUNTOF(m_SkillLevelMax));
				sVal.FormatVal( m_SkillLevelMax[i] );
				break;
			}
			i = g_Cfg.FindStatKey( pszKey);
			if ( i >= 0 )
			{
				ASSERT( static_cast<size_t>(i) < COUNTOF(m_StatMax));
				sVal.FormatVal( m_StatMax[i] );
				break;
			}
		}
		return( CResourceDef::r_WriteVal( pszKey, sVal, pSrc ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}


bool CSkillClassDef::r_LoadVal( CScript &s )
{
	ADDTOCALLSTACK("CSkillClassDef::r_LoadVal");
	EXC_TRY("LoadVal");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	case SCC_DEFNAME:
		return SetResourceName( s.GetArgStr());
	case SCC_NAME:
		m_sName = s.GetArgStr();
		break;
	case SCC_SKILLSUM:
		m_SkillSumMax = s.GetArgVal();
		break;
	case SCC_STATSUM:
		m_StatSumMax = static_cast<WORD>(s.GetArgVal());
		break;
	default:
		{
			int i = g_Cfg.FindSkillKey( s.GetKey());
			if ( i != SKILL_NONE )
			{
				ASSERT( i >= 0 && static_cast<size_t>(i) < COUNTOF(m_SkillLevelMax));
				m_SkillLevelMax[i] = static_cast<WORD>(s.GetArgVal());
				break;
			}
			i = g_Cfg.FindStatKey( s.GetKey());
			if ( i >= 0 )
			{
				ASSERT( static_cast<size_t>(i) < COUNTOF(m_StatMax));
				m_StatMax[i] = static_cast<WORD>(s.GetArgVal());
				break;
			}
		}
		return( CResourceDef::r_LoadVal( s ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}


//*******************************************
// -CSpellDef

LPCTSTR const CSpellDef::sm_szTrigName[SPTRIG_QTY+1] = 
{
	"@AAAUNUSED",
	"@EFFECT",
	"@FAIL",
	"@SELECT",
	"@START",
	"@SUCCESS",
	"@TARGETCANCEL",
	NULL,
};

enum SPC_TYPE
{
	SPC_CAST_TIME,
	SPC_DEFNAME,
	SPC_DURATION,
	SPC_EFFECT,
	SPC_EFFECT_ID,
	SPC_FLAGS,
	SPC_GROUP,
	SPC_INTERRUPT,
	SPC_MANAUSE,
	SPC_NAME,
	SPC_PROMPT_MSG,
	SPC_RESOURCES,
	SPC_RUNE_ITEM,
	SPC_RUNES,
	SPC_SCROLL_ITEM,
	SPC_SKILLREQ,
	SPC_SOUND,
	SPC_QTY
};

LPCTSTR const CSpellDef::sm_szLoadKeys[SPC_QTY+1] =
{
	"CAST_TIME",
	"DEFNAME",
	"DURATION",
	"EFFECT",
	"EFFECT_ID",
	"FLAGS",
	"GROUP",
	"INTERRUPT",
	"MANAUSE",
	"NAME",
	"PROMPT_MSG",
	"RESOURCES",
	"RUNE_ITEM",
	"RUNES",
	"SCROLL_ITEM",
	"SKILLREQ",
	"SOUND",
	NULL
};

CSpellDef::CSpellDef( SPELL_TYPE id ) :
	CResourceLink( RESOURCE_ID( RES_SPELL, id ))
{
	m_dwFlags = SPELLFLAG_DISABLED;
	m_dwGroup = 0;
	m_sound = 0;
	m_idSpell = ITEMID_NOTHING;
	m_idScroll = ITEMID_NOTHING;
	m_idEffect = ITEMID_NOTHING;
	m_wManaUse = 0;
	m_CastTime.Init();
	m_Interrupt.Init();
	m_Interrupt.m_aiValues.SetCount( 1 );
	m_Interrupt.m_aiValues[0] = 1000;
}



bool CSpellDef::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CSpellDef::r_WriteVal");
	EXC_TRY("WriteVal");
	int index = FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );
	if (index < 0)
	{
		if (!strnicmp( "RESOURCES.", pszKey, 10 ))
			index = SPC_RESOURCES;
	}
	switch (index)
	{
		case SPC_CAST_TIME:
			sVal = m_CastTime.Write();
			break;
		case SPC_DEFNAME:
			sVal = GetResourceName();
			break;
		case SPC_DURATION:
			sVal = m_Duration.Write();
			break;
		case SPC_EFFECT:
			sVal = m_Effect.Write();
			break;
		case SPC_EFFECT_ID:
			sVal.FormatVal( m_idEffect );
			break;
		case SPC_FLAGS:
			sVal.FormatVal( m_dwFlags );
			break;
		case SPC_GROUP:
			sVal.FormatVal( m_dwGroup );
			break;
		case SPC_INTERRUPT:
			sVal = m_Interrupt.Write();
			break;
		case SPC_MANAUSE:
			sVal.FormatVal( m_wManaUse );
			break;
		case SPC_NAME:
			sVal = m_sName;
			break;
		case SPC_PROMPT_MSG:
			sVal	= m_sTargetPrompt;
			break;
		case SPC_RESOURCES:
			{
				pszKey	+= 9;
				// Check for "RESOURCES.*"
				if ( *pszKey == '.' )
				{
					bool fQtyOnly = false;
					bool fKeyOnly = false;
					SKIP_SEPARATORS( pszKey );
					// Determine the index of the resource
					// we wish to find
					int	index = Exp_GetVal( pszKey );
					SKIP_SEPARATORS( pszKey );

					// Check for "RESOURCES.x.KEY"
					if ( !strnicmp( pszKey, "KEY", 3 ))
						fKeyOnly = true;
					// Check for "RESORUCES.x.VAL"
					else if ( !strnicmp( pszKey, "VAL", 3 ))
						fQtyOnly = true;

					TCHAR *pszTmp = Str_GetTemp();
					m_Reags.WriteKeys( pszTmp, index, fQtyOnly, fKeyOnly );
					if ( fQtyOnly && pszTmp[0] == '\0' )
						strcpy( pszTmp, "0" ); // Return 0 for empty quantity
					sVal = pszTmp;
				}
				else
				{
					TCHAR *pszTmp = Str_GetTemp();
					m_Reags.WriteKeys( pszTmp );
					sVal = pszTmp;
				}
				break;
			}
		case SPC_RUNE_ITEM:
			sVal.FormatVal( m_idSpell );
			break;
		case SPC_RUNES:
			// This may only be basic chars !
			sVal = m_sRunes;
			break;
		case SPC_SCROLL_ITEM:
			sVal.FormatVal( m_idScroll );
			break;
		case SPC_SKILLREQ:
			{
				TCHAR *pszTmp = Str_GetTemp();
				m_SkillReq.WriteKeys( pszTmp );
				sVal = pszTmp;
			}
			break;
		case SPC_SOUND:
			sVal.FormatVal(m_sound);
			break;
		default:
			return( CResourceDef::r_WriteVal( pszKey, sVal, pSrc ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}



bool CSpellDef::r_LoadVal( CScript &s )
{
	ADDTOCALLSTACK("CSpellDef::r_LoadVal");
	EXC_TRY("LoadVal");
	// RES_SPELL
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case SPC_CAST_TIME:
			m_CastTime.Load( s.GetArgRaw());
			break;
		case SPC_DEFNAME:
			return SetResourceName( s.GetArgStr());
		case SPC_DURATION:
			m_Duration.Load( s.GetArgRaw());
			break;
		case SPC_EFFECT:
			m_Effect.Load( s.GetArgRaw());
			break;
		case SPC_EFFECT_ID:
			m_idEffect = static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, s.GetArgStr()));
			break;
		case SPC_FLAGS:
			m_dwFlags = s.GetArgVal();
			break;
		case SPC_GROUP:
			m_dwGroup = s.GetArgVal();
			break;
		case SPC_INTERRUPT:
			m_Interrupt.Load( s.GetArgRaw());
			break;
		case SPC_MANAUSE:
			m_wManaUse = static_cast<WORD>(s.GetArgVal());
			break;
		case SPC_NAME:
			m_sName = s.GetArgStr();
			break;
		case SPC_PROMPT_MSG:
			m_sTargetPrompt	= s.GetArgStr();
			break;
		case SPC_RESOURCES:
			m_Reags.Load( s.GetArgStr());
			break;
		case SPC_RUNE_ITEM:
			m_idSpell = static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, s.GetArgStr()));
			break;
		case SPC_RUNES:
			// This may only be basic chars !
			m_sRunes = s.GetArgStr();
			break;
		case SPC_SCROLL_ITEM:
			m_idScroll = static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, s.GetArgStr()));
			break;
		case SPC_SKILLREQ:
			m_SkillReq.Load( s.GetArgStr());
			break;
		case SPC_SOUND:
			m_sound = static_cast<SOUND_TYPE>(s.GetArgVal());
			break;
		default:
			return( CResourceDef::r_LoadVal( s ) );
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}



bool CSpellDef::GetPrimarySkill( int * piSkill, int * piQty ) const
{
	ADDTOCALLSTACK("CSpellDef::GetPrimarySkill");
	size_t i = m_SkillReq.FindResourceType( RES_SKILL );
	if ( i == m_SkillReq.BadIndex() )
		return NULL;

	if ( piQty != NULL )
		*piQty = m_SkillReq[i].GetResQty();
	if ( piSkill != NULL )
		*piSkill = m_SkillReq[i].GetResIndex();
	return (g_Cfg.GetSkillDef(static_cast<SKILL_TYPE>(m_SkillReq[i].GetResIndex())) != NULL);
}

//*******************************************
// -CRandGroupDef

enum RGC_TYPE
{
	RGC_CALCMEMBERINDEX,
	RGC_DEFNAME,
	RGC_ID,
	RGC_RESOURCES,
	RGC_WEIGHT,
	RGC_QTY
};

LPCTSTR const CRandGroupDef::sm_szLoadKeys[RGC_QTY+1] =
{
	"CALCMEMBERINDEX",
	"DEFNAME",
	"ID",
	"RESOURCES",
	"WEIGHT",
	NULL
};

int CRandGroupDef::CalcTotalWeight()
{
	ADDTOCALLSTACK("CRandGroupDef::CalcTotalWeight");
	int iTotal = 0;
	size_t iQty = m_Members.GetCount();
	for ( size_t i = 0; i < iQty; i++ )
	{
		iTotal += m_Members[i].GetResQty();
	}
	return( m_iTotalWeight = iTotal );
}

bool CRandGroupDef::r_LoadVal( CScript &s )
{
	ADDTOCALLSTACK("CRandGroupDef::r_LoadVal");
	EXC_TRY("LoadVal");
	// RES_SPAWN or RES_REGIONTYPE
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case RGC_DEFNAME: // "DEFNAME"
			return SetResourceName( s.GetArgStr());
		case RGC_ID:	// "ID"
			{
				TCHAR	*ppCmd[2];
				size_t iArgs = Str_ParseCmds(s.GetArgStr(), ppCmd, COUNTOF(ppCmd));
				CResourceQty rec;

				rec.SetResourceID(
					g_Cfg.ResourceGetID(RES_CHARDEF, const_cast<LPCTSTR &>(reinterpret_cast<LPTSTR &>(ppCmd[0]))),
					( iArgs > 1 && ppCmd[1][0] ) ? Exp_GetVal(ppCmd[1]) : 1 );
				m_iTotalWeight += rec.GetResQty();
				m_Members.Add(rec);
			}
			break;

		case RGC_RESOURCES:
			m_Members.Load(s.GetArgStr());
			CalcTotalWeight();
			break;

		case RGC_WEIGHT: // Modify the weight of the last item.
			if ( m_Members.GetCount() > 0 )
			{
				int iWeight = s.GetArgVal();
				m_Members[m_Members.GetCount() - 1].SetResQty(iWeight);
				CalcTotalWeight();
			}
			break;

		default:
			return( CResourceDef::r_LoadVal( s ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CRandGroupDef::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CRandGroupDef::r_WriteVal");
	EXC_TRY("WriteVal");

	switch ( FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case RGC_CALCMEMBERINDEX:
		{
			pszKey += 15;
			GETNONWHITESPACE( pszKey );

			if ( pszKey[0] == '\0' )
			{
				sVal.FormatVal( GetRandMemberIndex(NULL, false) );
			}
			else
			{
				CGrayUID uidTofind = static_cast<DWORD>(Exp_GetVal(pszKey));
				CChar * pSend = uidTofind.CharFind();

				if ( pSend )
				{
					sVal.FormatVal( GetRandMemberIndex(pSend, false) );
				}
				else
				{
					return false;
				}
			}

		} break;

		case RGC_DEFNAME: // "DEFNAME"
			sVal = GetResourceName();
			break;

		case RGC_RESOURCES:
		{
			pszKey	+= 9;
			if ( *pszKey == '.' )
			{
				SKIP_SEPARATORS( pszKey );

				if ( !strnicmp( pszKey, "COUNT", 5 ))
				{
					sVal.FormatVal(m_Members.GetCount());
				}
				else
				{
					bool fQtyOnly = false;
					bool fKeyOnly = false;
					int index = Exp_GetVal( pszKey );
					SKIP_SEPARATORS( pszKey );

					if ( !strnicmp( pszKey, "KEY", 3 ))
						fKeyOnly = true;
					else if ( !strnicmp( pszKey, "VAL", 3 ))
						fQtyOnly = true;

					TCHAR *pszTmp = Str_GetTemp();
					m_Members.WriteKeys( pszTmp, index, fQtyOnly, fKeyOnly );
					if ( fQtyOnly && pszTmp[0] == '\0' )
						strcpy( pszTmp, "0" );

					sVal = pszTmp;
				}
			}
			else
			{
				TCHAR *pszTmp = Str_GetTemp();
				m_Members.WriteKeys( pszTmp );
				sVal = pszTmp;
			}
		} break;

		default:
			return( CResourceDef::r_WriteVal( pszKey, sVal, pSrc ));
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

size_t CRandGroupDef::GetRandMemberIndex( CChar * pCharSrc, bool bTrigger ) const
{
	ADDTOCALLSTACK("CRandGroupDef::GetRandMemberIndex");
	int rid;
	size_t iCount = m_Members.GetCount();
	if ( iCount <= 0 )
		return m_Members.BadIndex();

	int iWeight = 0;
	size_t i;
	if ( pCharSrc == NULL )
	{
		iWeight	= Calc_GetRandVal( m_iTotalWeight ) + 1;

		for ( i = 0; iWeight > 0 && i < iCount; i++ )
		{
			iWeight -= m_Members[i].GetResQty();
		}
		if ( i >= iCount && iWeight > 0 )
			return m_Members.BadIndex();

		ASSERT(i > 0);
		return( i - 1 );
	}

	CGPtrTypeArray<size_t> members;

	// calculate weight only of items pCharSrc can get
	int iTotalWeight = 0;
	for ( i = 0; i < iCount; i++ )
	{
		CRegionResourceDef * pOreDef = dynamic_cast <CRegionResourceDef *>( g_Cfg.ResourceGetDef( m_Members[i].GetResourceID() ) );
		rid = pOreDef->m_ReapItem;
		if ( rid != 0 )
		{
			if ( !pCharSrc->Skill_MakeItem(static_cast<ITEMID_TYPE>(rid), UID_CLEAR, SKTRIG_SELECT ) )
				continue;
			if ( IsTrigUsed(TRIGGER_RESOURCETEST) )
			{
				if ( bTrigger && pOreDef->OnTrigger( "@ResourceTest", pCharSrc, NULL ) == TRIGRET_RET_TRUE )
					continue;
			}
		}
		members.Add(i);
		iTotalWeight += m_Members[i].GetResQty();
	}
	iWeight = Calc_GetRandVal( iTotalWeight ) + 1;
	iCount = members.GetCount();

	for ( i = 0; iWeight > 0 && i < iCount; i++ )
	{
		iWeight -= m_Members[members[i]].GetResQty();
	}
	if ( i >= iCount && iWeight > 0 )
		return m_Members.BadIndex();
	ASSERT(i > 0);
	return members[i - 1];
}

//*******************************************
// -CRegionResourceDef

enum RMC_TYPE
{
	RMC_AMOUNT,
	RMC_DEFNAME,
	RMC_REAP,
	RMC_REAPAMOUNT,
	RMC_REGEN,
	RMC_SKILL,
	RMC_QTY
};

LPCTSTR const CRegionResourceDef::sm_szLoadKeys[RMC_QTY+1] =
{
	"AMOUNT",
	"DEFNAME",
	"REAP",
	"REAPAMOUNT",
	"REGEN",
	"SKILL",
	NULL
};

LPCTSTR const CRegionResourceDef::sm_szTrigName[RRTRIG_QTY+1] =	// static
{
	"@AAAUNUSED",
	"@ResourceFound",
	"@ResourceGather",
	"@ResourceTest",
	NULL,
};



TRIGRET_TYPE CRegionResourceDef::OnTrigger( LPCTSTR pszTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs )
{
	ADDTOCALLSTACK("CRegionResourceDef::OnTrigger");
	// Attach some trigger to the cchar. (PC or NPC)
	// RETURN: true = block further action.

	CResourceLock s;
	if ( ResourceLock( s ))
	{
		TRIGRET_TYPE iRet = CScriptObj::OnTriggerScript( s, pszTrigName, pSrc, pArgs );
		return iRet;
	}
	return TRIGRET_RET_DEFAULT;
}


bool CRegionResourceDef::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CRegionResourceDef::r_LoadVal");
	EXC_TRY("LoadVal");
	// RES_REGIONRESOURCE
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case RMC_AMOUNT: // AMOUNT
			m_Amount.Load( s.GetArgRaw() );
			break;
		case RMC_DEFNAME: // "DEFNAME",
			return SetResourceName( s.GetArgStr());
		case RMC_REAP: // "REAP",
			m_ReapItem = static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, s.GetArgStr()));
			break;
		case RMC_REAPAMOUNT:
			m_ReapAmount.Load( s.GetArgRaw() );
			break;
		case RMC_REGEN:
			m_iRegenerateTime = s.GetArgVal();	// TICK_PER_SEC once found how long to regen this type.
			break;
		case RMC_SKILL:
			m_Skill.Load( s.GetArgRaw() );
			break;
		default:
			return( CResourceDef::r_LoadVal( s ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CRegionResourceDef::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CRegionResourceDef::r_WriteVal");
	EXC_TRY("r_WriteVal");
	// RES_REGIONRESOURCE
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case RMC_AMOUNT:
			sVal = m_Amount.Write();
			break;
		case RMC_REAP: // "REAP",
			{
				CItemBase * pItemDef = CItemBase::FindItemBase(m_ReapItem);
				if ( !pItemDef )
				{
					return false;
				}

				sVal = pItemDef->GetResourceName();
			} break;
		case RMC_REAPAMOUNT:
			sVal = m_ReapAmount.Write();
			break;
		case RMC_REGEN:
			sVal.FormatVal( m_iRegenerateTime );
			break;
		case RMC_SKILL:
			sVal = m_Skill.Write();
			break;
		default:
			return( CResourceDef::r_WriteVal( pszKey, sVal, pSrc ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

CRegionResourceDef::CRegionResourceDef( RESOURCE_ID rid ) :
	CResourceLink( rid )
{
	// set defaults first.
	m_ReapItem = ITEMID_NOTHING;
	m_iRegenerateTime = 0;	// TICK_PER_SEC once found how long to regen this type.
}

CRegionResourceDef::~CRegionResourceDef()
{
}
