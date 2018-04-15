#include "graysvr.h"	// predef header.

///////////////////////////////////////////////////////////////
// -CBaseBaseDef

// 

enum OBC_TYPE
{
	#define ADD(a,b) OBC_##a,
	#include "../tables/CBaseBaseDef_props.tbl"
	#undef ADD
	OBC_QTY
};

LPCTSTR const CBaseBaseDef::sm_szLoadKeys[OBC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CBaseBaseDef_props.tbl"
	#undef ADD
	NULL
};

bool CBaseBaseDef::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CBaseBaseDef::r_WriteVal");
	EXC_TRY("WriteVal");
	bool	fZero	= false;
	int index = FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );

	switch ( index )
	{
		//return as string or hex number or NULL if not set
		case OBC_CATEGORY:
		case OBC_DESCRIPTION:
		case OBC_SUBSECTION:
			sVal = GetDefStr(pszKey, false);
			break;
		//return as decimal number or 0 if not set
		case OBC_DAMCHAOS:
		case OBC_DAMDIRECT:
		case OBC_DECREASEHITCHANCE:
		case OBC_ENHANCEPOTIONS:
		case OBC_EXPANSION:
		case OBC_FASTERCASTING:
		case OBC_FASTERCASTRECOVERY:
		case OBC_HITLEECHLIFE:
		case OBC_HITLEECHMANA:
		case OBC_HITLEECHSTAM:
		case OBC_HITMANADRAIN:
		case OBC_INCREASEDAM:
		case OBC_INCREASEDEFCHANCE:
		case OBC_INCREASEDEFCHANCEMAX:
		case OBC_INCREASEHITCHANCE:
		case OBC_INCREASESPELLDAM:
		case OBC_INCREASESWINGSPEED:
		case OBC_LOWERMANACOST:
		case OBC_LOWERREAGENTCOST:
		case OBC_LOWERREQ:
		case OBC_NIGHTSIGHT:
		case OBC_REFLECTPHYSICALDAM:
		case OBC_REGENFOOD:
		case OBC_REGENHITS:
		case OBC_REGENMANA:
		case OBC_REGENSTAM:
		case OBC_REGENVALFOOD:
		case OBC_REGENVALHITS:
		case OBC_REGENVALMANA:
		case OBC_REGENVALSTAM:
		case OBC_SPELLCHANNELING:
		case OBC_NAMELOC:
		case OBC_WEIGHTREDUCTION:
		case OBC_COMBATBONUSSTAT:
		case OBC_COMBATBONUSPERCENT:
			sVal.FormatLLVal(GetDefNum(pszKey));
			break;

		case OBC_DEFNAME:
			sVal = GetResourceName();
			break;

		case OBC_ARMOR:
			{
				pszKey += strlen(sm_szLoadKeys[index]); // 9;
				if ( *pszKey == '.' )
				{
					SKIP_SEPARATORS( pszKey );

					if ( !strnicmp( pszKey, "LO", 2 ) )
						sVal.Format( "%hu", m_defenseBase );
					else if ( !strnicmp( pszKey, "HI", 2 ) )
						sVal.Format( "%hu", m_defenseBase+m_defenseRange );
				}
				else
				{
					sVal.Format( "%hu,%hu", m_defenseBase, m_defenseBase+m_defenseRange );
				}
			} break;
		case OBC_DAM:
			{
				pszKey += strlen(sm_szLoadKeys[index]); // 9;
				if ( *pszKey == '.' )
				{
					SKIP_SEPARATORS( pszKey );

					if ( !strnicmp( pszKey, "LO", 2 ) )
						sVal.Format( "%hu", m_attackBase );
					else if ( !strnicmp( pszKey, "HI", 2 ) )
						sVal.Format( "%hu", m_attackBase+m_attackRange );
				}
				else
				{
					sVal.Format( "%hu,%hu", m_attackBase, m_attackBase+m_attackRange );
				}
			} break;
		case OBC_DAMCOLD:
			sVal.FormatVal(m_DamCold);
			break;
		case OBC_DAMENERGY:
			sVal.FormatVal(m_DamEnergy);
			break;
		case OBC_DAMFIRE:
			sVal.FormatVal(m_DamFire);
			break;
		case OBC_DAMPHYSICAL:
			sVal.FormatVal(m_DamPhysical);
			break;
		case OBC_DAMPOISON:
			sVal.FormatVal(m_DamPoison);
			break;
		case OBC_BASEID:
			sVal = g_Cfg.ResourceGetName( GetResourceID());
			break;
		case OBC_CAN:
			sVal.FormatHex( m_Can );
			break;
		case OBC_HEIGHT:
			{
				//CBaseBaseDef * pBaseBaseDef = dynamic_cast<CBaseBaseDef*>(this);
				//DEBUG_ERR(("OBC_HEIGHT  -  m_dwDispIndex %d  GetHeight() %d  pBaseBaseDef->GetHeight() %d  pBaseBaseDef 0x%x\n",m_wDispIndex,GetHeight(),pBaseBaseDef->GetHeight(),pBaseBaseDef));
				sVal.FormatVal( GetHeight() );
			}
			break;
		case OBC_INSTANCES:
			sVal.FormatUVal( GetRefInstances());
			break;
		case OBC_LUCK:
			sVal.FormatVal(m_Luck);
			break;
		case OBC_NAME:
			sVal = GetName();
			break;

		case OBC_RANGE:
			if ( RangeH() == 0 )
				sVal.Format("%hhu", RangeL());
			else
				sVal.Format("%hhu,%hhu", RangeH(), RangeL());
			break;
		case OBC_RANGEL: // internally: rangel seems to be Range Highest value
			sVal.FormatUVal( RangeH() );
			break;
		case OBC_RANGEH: // but rangeh seems to be the Range Lowest value.
			sVal.FormatUVal( RangeL() );
			break;

		case OBC_RESOURCES:		// Print the resources
			{
				pszKey += strlen(sm_szLoadKeys[index]); // 9;
				if ( *pszKey == '.' )
				{
					SKIP_SEPARATORS( pszKey );

					if ( !strnicmp( pszKey, "COUNT", 5 ))
					{
						sVal.FormatVal(m_BaseResources.GetCount());
					}
					else
					{
						bool fQtyOnly = false;
						bool fKeyOnly = false;
						index = Exp_GetVal( pszKey );
						SKIP_SEPARATORS( pszKey );

						if ( !strnicmp( pszKey, "KEY", 3 ))
							fKeyOnly	= true;
						else if ( !strnicmp( pszKey, "VAL", 3 ))
							fQtyOnly	= true;

						TCHAR *pszTmp = Str_GetTemp();
						m_BaseResources.WriteKeys( pszTmp, index, fQtyOnly, fKeyOnly );
						if ( fQtyOnly && pszTmp[0] == '\0' )
							strcpy( pszTmp, "0" );

						sVal = pszTmp;
					}
				}
				else
				{
					TCHAR *pszTmp = Str_GetTemp();
					m_BaseResources.WriteKeys( pszTmp );
					sVal = pszTmp;
				}
			}
			break;
		case OBC_RESLEVEL:
			sVal.FormatUVal( GetResLevel() );
			break;
		case OBC_RESDISPDNHUE:
			sVal.FormatHex( GetResDispDnHue() );
			break;
		case OBC_RESCOLD:
			sVal.FormatVal(m_ResCold);
			break;
		case OBC_RESCOLDMAX:
			sVal.FormatVal(m_ResColdMax);
			break;
		case OBC_RESENERGY:
			sVal.FormatVal(m_ResEnergy);
			break;
		case OBC_RESENERGYMAX:
			sVal.FormatVal(m_ResEnergyMax);
			break;
		case OBC_RESFIRE:
			sVal.FormatVal(m_ResFire);
			break;
		case OBC_RESFIREMAX:
			sVal.FormatVal(m_ResFireMax);
			break;
		case OBC_RESPHYSICAL:
			sVal.FormatVal(m_ResPhysical);
			break;
		case OBC_RESPHYSICALMAX:
			sVal.FormatVal(m_ResPhysicalMax);
			break;
		case OBC_RESPOISON:
			sVal.FormatVal(m_ResPoison);
			break;
		case OBC_RESPOISONMAX:
			sVal.FormatVal(m_ResPoisonMax);
			break;
		case OBC_TAG0:
			fZero	= true;
			pszKey++;
		case OBC_TAG:			// "TAG" = get/set a local tag.
			if ( pszKey[3] != '.' )
				return( false );
			pszKey += 4;
			sVal = m_TagDefs.GetKeyStr(pszKey, fZero );
			break;
		case OBC_TEVENTS:
			m_TEvents.WriteResourceRefList( sVal );
			break;
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CBaseBaseDef::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CBaseBaseDef::r_LoadVal");
	EXC_TRY("LoadVal");
	if ( s.IsKeyHead( "TAG.", 4 ))
	{
		bool fQuoted = false;
		m_TagDefs.SetStr( s.GetKey()+4, fQuoted, s.GetArgStr( &fQuoted ), false );
		return( true );
	}
	if ( s.IsKeyHead( "TAG0.", 5 ))
	{
		bool fQuoted = false;
		m_TagDefs.SetStr( s.GetKey()+5, fQuoted, s.GetArgStr( &fQuoted ), true );
		return( true );
	}

	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		//Set as number only
		case OBC_DAMCHAOS:
		case OBC_DAMDIRECT:
		case OBC_DECREASEHITCHANCE:
		case OBC_ENHANCEPOTIONS:
		case OBC_EXPANSION:
		case OBC_FASTERCASTING:
		case OBC_FASTERCASTRECOVERY:
		case OBC_HITLEECHLIFE:
		case OBC_HITLEECHMANA:
		case OBC_HITLEECHSTAM:
		case OBC_HITMANADRAIN:
		case OBC_INCREASEDAM:
		case OBC_INCREASEDEFCHANCE:
		case OBC_INCREASEDEFCHANCEMAX:
		case OBC_INCREASEHITCHANCE:
		case OBC_INCREASESPELLDAM:
		case OBC_INCREASESWINGSPEED:
		case OBC_LOWERMANACOST:
		case OBC_LOWERREAGENTCOST:
		case OBC_LOWERREQ:
		case OBC_NIGHTSIGHT:
		case OBC_REFLECTPHYSICALDAM:
		case OBC_REGENFOOD:
		case OBC_REGENHITS:
		case OBC_REGENMANA:
		case OBC_REGENSTAM:
		case OBC_REGENVALFOOD:
		case OBC_REGENVALHITS:
		case OBC_REGENVALMANA:
		case OBC_REGENVALSTAM:
		case OBC_SPELLCHANNELING:
		case OBC_NAMELOC:
		case OBC_WEIGHTREDUCTION:
		case OBC_COMBATBONUSSTAT:
		case OBC_COMBATBONUSPERCENT:
			{
				SetDefNum(s.GetKey(),s.GetArgVal());
				return true;
			}
		case OBC_CATEGORY:
		case OBC_SUBSECTION:
		case OBC_DESCRIPTION:
			{
				bool fQuoted = false;
				SetDefStr(s.GetKey(), s.GetArgStr( &fQuoted ), fQuoted);
				if ( !strcmpi(GetDefStr(sm_szLoadKeys[OBC_DESCRIPTION]), "@") )
					SetDefStr(sm_szLoadKeys[OBC_DESCRIPTION], GetDefStr(sm_szLoadKeys[OBC_SUBSECTION]));
			}
			return true;
		case OBC_ARMOR:
			{
				INT64 piVal[2];
				size_t iQty = Str_ParseCmds( s.GetArgStr(), piVal, COUNTOF(piVal));
				m_defenseBase = static_cast<WORD>(piVal[0]);
				if ( iQty > 1 )
					m_defenseRange = static_cast<WORD>(piVal[1]) - m_defenseBase;
				else
					m_defenseRange = 0;
			}
			return( true );
		case OBC_DAM:
			{
				INT64 piVal[2];
				size_t iQty = Str_ParseCmds( s.GetArgStr(), piVal, COUNTOF(piVal));
				m_attackBase = static_cast<WORD>(piVal[0]);
				if ( iQty > 1 )
					m_attackRange = static_cast<WORD>(piVal[1]) - m_attackBase;
				else
					m_attackRange = 0;
			}
			return( true );
		case OBC_DAMCOLD:
			m_DamCold = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_DAMENERGY:
			m_DamEnergy = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_DAMFIRE:
			m_DamFire = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_DAMPHYSICAL:
			m_DamPhysical = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_DAMPOISON:
			m_DamPoison = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_BASEID:
			return( false );
		case OBC_CAN:
			m_Can = s.GetArgVal();// | ( m_Can & ( CAN_C_INDOORS|CAN_C_EQUIP|CAN_C_USEHANDS|CAN_C_NONHUMANOID )); //Fixed #2326 ?
			return( true );

		case OBC_DEFNAME:
		case OBC_DEFNAME2:
			return SetResourceName( s.GetArgStr());
		case OBC_HEIGHT:
			m_Height = static_cast<height_t>(s.GetArgVal());
			return( true );
		case OBC_INSTANCES:
			return( false );
		case OBC_LUCK:
			m_Luck = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_NAME:
			SetTypeName( s.GetArgStr());
			return( true );
		case OBC_RANGE:
			{
				INT64 piVal[2];
				size_t iQty = Str_ParseCmds( s.GetArgStr(), piVal, COUNTOF(piVal));
				if ( iQty > 1 )
				{
					INT64 iRange = ((piVal[0] & 0xff) << 8) & 0xff00;
					iRange |= (piVal[1] & 0xff);
					SetDefNum(s.GetKey(),iRange, false);
					//m_range	 = ((piVal[0] & 0xff) << 8) & 0xff00;
					//m_range	|= (piVal[1] & 0xff);
				}
				else
				{
					SetDefNum(s.GetKey(),piVal[0], false);
					//m_range	= static_cast<WORD>(piVal[0]);
				}
			}
			return( true );
		case OBC_RESOURCES:
			m_BaseResources.Load( s.GetArgStr());
			return( true );
		case OBC_RESLEVEL:
			return( SetResLevel(static_cast<BYTE>(s.GetArgVal())) );
		case OBC_RESDISPDNHUE:
			SetResDispDnHue(static_cast<HUE_TYPE>(s.GetArgVal()));
			return( true );
		case OBC_RESCOLD:
			m_ResCold = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_RESCOLDMAX:
			m_ResColdMax = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_RESENERGY:
			m_ResEnergy = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_RESENERGYMAX:
			m_ResEnergyMax = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_RESFIRE:
			m_ResFire = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_RESFIREMAX:
			m_ResFireMax = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_RESPHYSICAL:
			m_ResPhysical = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_RESPHYSICALMAX:
			m_ResPhysicalMax = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_RESPOISON:
			m_ResPoison = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_RESPOISONMAX:
			m_ResPoisonMax = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_TEVENTS:
			return( m_TEvents.r_LoadVal( s, RES_EVENTS ));
	}
	return( CScriptObj::r_LoadVal(s));
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}
;
// do not copy CResourceLink
void CBaseBaseDef::CopyBasic( const CBaseBaseDef * pBase )
{
	ADDTOCALLSTACK("CBaseBaseDef::CopyBasic");

	if ( m_sName.IsEmpty())	// Base type name. should set this itself most times. (don't overwrite it!)
		m_sName = pBase->m_sName;

	m_dwDispIndex = pBase->m_dwDispIndex;

	m_Height = pBase->m_Height;
	// -------------- ResLevel -------------
	m_ResLevel = pBase->m_ResLevel;
	m_ResDispDnHue = pBase->m_ResDispDnHue;
	m_ResDispDnId = pBase->m_ResDispDnId;
	// -------------------------------------
	// m_BaseResources = pBase->m_BaseResources;	// items might not want this.
	m_attackBase = pBase->m_attackBase;
	m_attackRange = pBase->m_attackRange;
	m_defenseBase = pBase->m_defenseBase;
	m_defenseRange = pBase->m_defenseRange;
	m_DamPhysical = pBase->m_DamPhysical;
	m_DamFire = pBase->m_DamFire;
	m_DamCold = pBase->m_DamCold;
	m_DamPoison = pBase->m_DamPoison;
	m_DamEnergy = pBase->m_DamEnergy;
	m_ResPhysical = pBase->m_ResPhysical;
	m_ResPhysicalMax = pBase->m_ResPhysicalMax;
	m_ResFire = pBase->m_ResFire;
	m_ResFireMax = pBase->m_ResFireMax;
	m_ResCold = pBase->m_ResCold;
	m_ResColdMax = pBase->m_ResColdMax;
	m_ResPoison = pBase->m_ResPoison;
	m_ResPoisonMax = pBase->m_ResPoisonMax;
	m_ResEnergy = pBase->m_ResEnergy;
	m_ResEnergyMax = pBase->m_ResEnergyMax;
	m_Luck = pBase->m_Luck;
	m_Can = pBase->m_Can;
}

void CBaseBaseDef::CopyTransfer( CBaseBaseDef * pBase )
{
	ADDTOCALLSTACK("CBaseBaseDef::CopyTransfer");
	CResourceLink::CopyTransfer( pBase );

	m_sName = pBase->m_sName;
	m_BaseResources = pBase->m_BaseResources;

	CopyBasic( pBase );
}

