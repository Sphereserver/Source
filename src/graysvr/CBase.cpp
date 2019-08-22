#include "graysvr.h"	// predef header.

///////////////////////////////////////////////////////////////
// -CBaseBaseDef

CBaseBaseDef::CBaseBaseDef(RESOURCE_ID id) : CResourceLink(id)
{
	m_dwDispIndex = 0;
	m_BaseResources.setNoMergeOnLoad();
	SetDefNum("RANGE", 1);	//m_range = 1;
	m_attackBase = 0;
	m_attackRange = 0;
	m_defenseBase = 0;
	m_defenseRange = 0;
	m_DamPhysical = 0;
	m_DamFire = 0;
	m_DamCold = 0;
	m_DamPoison = 0;
	m_DamEnergy = 0;
	m_ResPhysical = 0;
	m_ResPhysicalMax = 0;
	m_ResFire = 0;
	m_ResFireMax = 0;
	m_ResCold = 0;
	m_ResColdMax = 0;
	m_ResPoison = 0;
	m_ResPoisonMax = 0;
	m_ResEnergy = 0;
	m_ResEnergyMax = 0;
	m_Luck = 0;
	m_DamIncrease = 0;
	m_SpellDamIncrease = 0;
	m_HitLifeLeech = 0;
	m_HitManaDrain = 0;
	m_HitManaLeech = 0;
	m_HitStaminaLeech = 0;
	m_HitChanceIncrease = 0;
	m_DefChanceIncrease = 0;
	m_DefChanceIncreaseMax = 0;
	m_SwingSpeedIncrease = 0;
	m_FasterCasting = 0;
	m_FasterCastRecovery = 0;
	m_LowerManaCost = 0;
	m_LowerReagentCost = 0;
	m_EnhancePotions = 0;
	m_NightSight = 0;
	m_ReflectPhysicalDamage = 0;
	m_Can = CAN_C_INDOORS;
	m_Height = 0;
	m_ResLevel = RDS_NONE;
	m_ResDispDnId = 0;
	m_ResDispDnHue = HUE_DEFAULT;
}

void CBaseBaseDef::CopyBasic(const CBaseBaseDef *pBaseDef)
{
	ADDTOCALLSTACK("CBaseBaseDef::CopyBasic");
	// NOTE: Do not copy CResourceLink

	if ( m_sName.IsEmpty() )	// base type name, should set this itself most times (don't overwrite it!)
		m_sName = pBaseDef->m_sName;

	m_dwDispIndex = pBaseDef->m_dwDispIndex;
	//m_BaseResources = pBaseDef->m_BaseResources;		// items might not want this
	SetDefNum("RANGE", pBaseDef->GetDefNum("RANGE"));	//m_range = pBaseDef->m_range
	m_attackBase = pBaseDef->m_attackBase;
	m_attackRange = pBaseDef->m_attackRange;
	m_defenseBase = pBaseDef->m_defenseBase;
	m_defenseRange = pBaseDef->m_defenseRange;
	m_DamPhysical = pBaseDef->m_DamPhysical;
	m_DamFire = pBaseDef->m_DamFire;
	m_DamCold = pBaseDef->m_DamCold;
	m_DamPoison = pBaseDef->m_DamPoison;
	m_DamEnergy = pBaseDef->m_DamEnergy;
	m_ResPhysical = pBaseDef->m_ResPhysical;
	m_ResPhysicalMax = pBaseDef->m_ResPhysicalMax;
	m_ResFire = pBaseDef->m_ResFire;
	m_ResFireMax = pBaseDef->m_ResFireMax;
	m_ResCold = pBaseDef->m_ResCold;
	m_ResColdMax = pBaseDef->m_ResColdMax;
	m_ResPoison = pBaseDef->m_ResPoison;
	m_ResPoisonMax = pBaseDef->m_ResPoisonMax;
	m_ResEnergy = pBaseDef->m_ResEnergy;
	m_ResEnergyMax = pBaseDef->m_ResEnergyMax;
	m_Luck = pBaseDef->m_Luck;
	m_DamIncrease = pBaseDef->m_DamIncrease;
	m_SpellDamIncrease = pBaseDef->m_SpellDamIncrease;
	m_HitLifeLeech = pBaseDef->m_HitLifeLeech;
	m_HitManaDrain = pBaseDef->m_HitManaDrain;
	m_HitManaLeech = pBaseDef->m_HitManaLeech;
	m_HitStaminaLeech = pBaseDef->m_HitStaminaLeech;
	m_HitChanceIncrease = pBaseDef->m_HitChanceIncrease;
	m_DefChanceIncrease = pBaseDef->m_DefChanceIncrease;
	m_DefChanceIncreaseMax = pBaseDef->m_DefChanceIncreaseMax;
	m_SwingSpeedIncrease = pBaseDef->m_SwingSpeedIncrease;
	m_FasterCasting = pBaseDef->m_FasterCasting;
	m_FasterCastRecovery = pBaseDef->m_FasterCastRecovery;
	m_LowerManaCost = pBaseDef->m_LowerManaCost;
	m_LowerReagentCost = pBaseDef->m_LowerReagentCost;
	m_EnhancePotions = pBaseDef->m_EnhancePotions;
	m_NightSight = pBaseDef->m_NightSight;
	m_ReflectPhysicalDamage = pBaseDef->m_ReflectPhysicalDamage;
	m_Can = pBaseDef->m_Can;
	m_Height = pBaseDef->m_Height;
	m_ResLevel = pBaseDef->m_ResLevel;
	m_ResDispDnId = pBaseDef->m_ResDispDnId;
	m_ResDispDnHue = pBaseDef->m_ResDispDnHue;
}

void CBaseBaseDef::CopyTransfer(CBaseBaseDef *pBaseDef)
{
	ADDTOCALLSTACK("CBaseBaseDef::CopyTransfer");
	CResourceLink::CopyTransfer(pBaseDef);

	m_sName = pBaseDef->m_sName;
	m_BaseResources = pBaseDef->m_BaseResources;

	CopyBasic(pBaseDef);
}

enum OBC_TYPE
{
	#define ADD(a,b) OBC_##a,
	#include "../tables/CBaseBaseDef_props.tbl"
	#undef ADD
	OBC_QTY
};

LPCTSTR const CBaseBaseDef::sm_szLoadKeys[OBC_QTY + 1] =
{
	#define ADD(a,b) b,
	#include "../tables/CBaseBaseDef_props.tbl"
	#undef ADD
	NULL
};

bool CBaseBaseDef::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CBaseBaseDef::r_WriteVal");
	EXC_TRY("WriteVal");
	bool fZero = false;

	switch ( FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		// Return as string or hex number (NULL if not set)
		case OBC_CATEGORY:
		case OBC_DESCRIPTION:
		case OBC_SUBSECTION:
			sVal = GetDefStr(pszKey, false);
			break;
		// Return as decimal number (0 if not set)
		case OBC_COMBATBONUSPERCENT:
		case OBC_COMBATBONUSSTAT:
		case OBC_DAMCHAOS:
		case OBC_DAMDIRECT:
		case OBC_EXPANSION:
		case OBC_NAMELOC:
		case OBC_REGENFOOD:
		case OBC_REGENHITS:
		case OBC_REGENMANA:
		case OBC_REGENSTAM:
		case OBC_REGENVALFOOD:
		case OBC_REGENVALHITS:
		case OBC_REGENVALMANA:
		case OBC_REGENVALSTAM:
			sVal.FormatLLVal(GetDefNum(pszKey));
			break;
		case OBC_ARMOR:
		{
			pszKey += 5;
			if ( *pszKey == '.' )
			{
				SKIP_SEPARATORS(pszKey);
				if ( !strnicmp(pszKey, "LO", 2) )
					sVal.Format("%hu", m_defenseBase);
				else if ( !strnicmp(pszKey, "HI", 2) )
					sVal.Format("%hu", m_defenseBase + m_defenseRange);
			}
			else
				sVal.Format("%hu,%hu", m_defenseBase, m_defenseBase + m_defenseRange);
			break;
		}
		case OBC_BASEID:
			sVal = g_Cfg.ResourceGetName(GetResourceID());
			break;
		case OBC_CAN:
			sVal.FormatHex(m_Can);
			break;
		case OBC_DAM:
		{
			pszKey += 3;
			if ( *pszKey == '.' )
			{
				SKIP_SEPARATORS(pszKey);
				if ( !strnicmp(pszKey, "LO", 2) )
					sVal.Format("%hu", m_attackBase);
				else if ( !strnicmp(pszKey, "HI", 2) )
					sVal.Format("%hu", m_attackBase + m_attackRange);
			}
			else
				sVal.Format("%hu,%hu", m_attackBase, m_attackBase + m_attackRange);
			break;
		}
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
		case OBC_DEFNAME:
			sVal = GetResourceName();
			break;
		case OBC_ENHANCEPOTIONS:
			sVal.FormatVal(m_EnhancePotions);
			break;
		case OBC_FASTERCASTING:
			sVal.FormatVal(m_FasterCasting);
			break;
		case OBC_FASTERCASTRECOVERY:
			sVal.FormatVal(m_FasterCastRecovery);
			break;
		case OBC_HEIGHT:
			sVal.FormatVal(GetHeight());
			break;
		case OBC_HITLEECHLIFE:
			sVal.FormatVal(m_HitLifeLeech);
			break;
		case OBC_HITLEECHMANA:
			sVal.FormatVal(m_HitManaLeech);
			break;
		case OBC_HITLEECHSTAM:
			sVal.FormatVal(m_HitStaminaLeech);
			break;
		case OBC_HITMANADRAIN:
			sVal.FormatVal(m_HitManaDrain);
			break;
		case OBC_INCREASEDAM:
			sVal.FormatVal(m_DamIncrease);
			break;
		case OBC_INCREASEDEFCHANCE:
			sVal.FormatVal(m_DefChanceIncrease);
			break;
		case OBC_INCREASEDEFCHANCEMAX:
			sVal.FormatVal(m_DefChanceIncreaseMax);
			break;
		case OBC_INCREASEHITCHANCE:
			sVal.FormatVal(m_HitChanceIncrease);
			break;
		case OBC_INCREASESPELLDAM:
			sVal.FormatVal(m_SpellDamIncrease);
			break;
		case OBC_INCREASESWINGSPEED:
			sVal.FormatVal(m_SwingSpeedIncrease);
			break;
		case OBC_INSTANCES:
			sVal.FormatUVal(GetRefInstances());
			break;
		case OBC_LOWERMANACOST:
			sVal.FormatVal(m_LowerManaCost);
			break;
		case OBC_LOWERREAGENTCOST:
			sVal.FormatVal(m_LowerReagentCost);
			break;
		case OBC_LUCK:
			sVal.FormatVal(m_Luck);
			break;
		case OBC_NAME:
			sVal = GetName();
			break;
		case OBC_NIGHTSIGHT:
			sVal.FormatVal(m_NightSight);
			break;
		case OBC_REFLECTPHYSICALDAM:
			sVal.FormatVal(m_ReflectPhysicalDamage);
			break;
		case OBC_RANGE:
			if ( GetRangeH() == 0 )
				sVal.Format("%hhu", GetRangeL());
			else
				sVal.Format("%hhu,%hhu", GetRangeH(), GetRangeL());
			break;
		case OBC_RANGEH:
			sVal.FormatUVal(GetRangeL());
			break;
		case OBC_RANGEL:
			sVal.FormatUVal(GetRangeH());
			break;
		case OBC_RESCOLD:
			sVal.FormatVal(m_ResCold);
			break;
		case OBC_RESCOLDMAX:
			sVal.FormatVal(m_ResColdMax);
			break;
		case OBC_RESDISPDNHUE:
			sVal.FormatHex(GetResDispDnHue());
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
		case OBC_RESLEVEL:
			sVal.FormatUVal(GetResLevel());
			break;
		case OBC_RESOURCES:
		{
			pszKey += 9;
			if ( *pszKey == '.' )
			{
				SKIP_SEPARATORS(pszKey);
				if ( !strnicmp(pszKey, "COUNT", 5) )
					sVal.FormatVal(m_BaseResources.GetCount());
				else
				{
					size_t index = static_cast<size_t>(Exp_GetLLVal(pszKey));
					SKIP_SEPARATORS(pszKey);

					bool fQtyOnly = false;
					bool fKeyOnly = false;
					if ( !strnicmp(pszKey, "KEY", 3) )
						fKeyOnly = true;
					else if ( !strnicmp(pszKey, "VAL", 3) )
						fQtyOnly = true;

					TCHAR *pszTmp = Str_GetTemp();
					m_BaseResources.WriteKeys(pszTmp, index, fQtyOnly, fKeyOnly);
					if ( fQtyOnly && (pszTmp[0] == '\0') )
						strcpy(pszTmp, "0");
					sVal = pszTmp;
				}
			}
			else
			{
				TCHAR *pszTmp = Str_GetTemp();
				m_BaseResources.WriteKeys(pszTmp);
				sVal = pszTmp;
			}
			break;
		}
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
			fZero = true;
			++pszKey;
			// fall through
		case OBC_TAG:
			if ( pszKey[3] != '.' )
				return false;
			pszKey += 4;
			sVal = m_TagDefs.GetKeyStr(pszKey, fZero);
			break;
		case OBC_TEVENTS:
			m_TEvents.WriteResourceRefList(sVal);
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

bool CBaseBaseDef::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CBaseBaseDef::r_LoadVal");
	EXC_TRY("LoadVal");
	if ( s.IsKeyHead("TAG.", 4) )
	{
		bool fQuoted = false;
		m_TagDefs.SetStr(s.GetKey() + 4, fQuoted, s.GetArgStr(&fQuoted), false);
		return true;
	}
	if ( s.IsKeyHead("TAG0.", 5) )
	{
		bool fQuoted = false;
		m_TagDefs.SetStr(s.GetKey() + 5, fQuoted, s.GetArgStr(&fQuoted), true);
		return true;
	}

	switch ( FindTableSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		// Set as numeric
		case OBC_COMBATBONUSPERCENT:
		case OBC_COMBATBONUSSTAT:
		case OBC_DAMCHAOS:
		case OBC_DAMDIRECT:
		case OBC_EXPANSION:
		case OBC_NAMELOC:
		case OBC_REGENFOOD:
		case OBC_REGENHITS:
		case OBC_REGENMANA:
		case OBC_REGENSTAM:
		case OBC_REGENVALFOOD:
		case OBC_REGENVALHITS:
		case OBC_REGENVALMANA:
		case OBC_REGENVALSTAM:
		{
			SetDefNum(s.GetKey(), s.GetArgVal());
			return true;
		}
		case OBC_CATEGORY:
		case OBC_SUBSECTION:
		case OBC_DESCRIPTION:
		{
			bool fQuoted = false;
			SetDefStr(s.GetKey(), s.GetArgStr(&fQuoted), fQuoted);
			if ( !strcmpi(GetDefStr(sm_szLoadKeys[OBC_DESCRIPTION]), "@") )
				SetDefStr(sm_szLoadKeys[OBC_DESCRIPTION], GetDefStr(sm_szLoadKeys[OBC_SUBSECTION]));
			return true;
		}
		case OBC_ARMOR:
		{
			INT64 piVal[2];
			size_t iQty = Str_ParseCmds(s.GetArgStr(), piVal, COUNTOF(piVal));
			m_defenseBase = static_cast<WORD>(piVal[0]);
			if ( iQty > 1 )
				m_defenseRange = static_cast<WORD>(piVal[1]) - m_defenseBase;
			else
				m_defenseRange = 0;
			return true;
		}
		case OBC_BASEID:
			return false;
		case OBC_CAN:
			m_Can = s.GetArgVal();	//| (m_Can & (CAN_C_INDOORS|CAN_C_EQUIP|CAN_C_USEHANDS|CAN_C_NONHUMANOID));	//fixed #2326?
			return true;
		case OBC_DAM:
		{
			INT64 piVal[2];
			size_t iQty = Str_ParseCmds(s.GetArgStr(), piVal, COUNTOF(piVal));
			m_attackBase = static_cast<WORD>(piVal[0]);
			if ( iQty > 1 )
				m_attackRange = static_cast<WORD>(piVal[1]) - m_attackBase;
			else
				m_attackRange = 0;
			return true;
		}
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
		case OBC_DEFNAME:
		case OBC_DEFNAME2:
			return SetResourceName(s.GetArgStr());
		case OBC_ENHANCEPOTIONS:
			m_EnhancePotions = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_FASTERCASTING:
			m_FasterCasting = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_FASTERCASTRECOVERY:
			m_FasterCastRecovery = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_HEIGHT:
			m_Height = static_cast<height_t>(s.GetArgVal());
			return true;
		case OBC_HITLEECHLIFE:
			m_HitLifeLeech = static_cast<int>(s.GetArgVal());
			break;
		case OBC_HITLEECHMANA:
			m_HitManaLeech = static_cast<int>(s.GetArgVal());
			break;
		case OBC_HITLEECHSTAM:
			m_HitStaminaLeech = static_cast<int>(s.GetArgVal());
			break;
		case OBC_HITMANADRAIN:
			m_HitManaDrain = static_cast<int>(s.GetArgVal());
			break;
		case OBC_INCREASEDAM:
			m_DamIncrease = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_INCREASEDEFCHANCE:
			m_DefChanceIncrease = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_INCREASEDEFCHANCEMAX:
			m_DefChanceIncreaseMax = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_INCREASEHITCHANCE:
			m_HitChanceIncrease = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_INCREASESPELLDAM:
			m_SpellDamIncrease = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_INCREASESWINGSPEED:
			m_SwingSpeedIncrease = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_INSTANCES:
			return false;
		case OBC_LOWERMANACOST:
			m_LowerManaCost = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_LOWERREAGENTCOST:
			m_LowerReagentCost = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_LUCK:
			m_Luck = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_NAME:
			SetTypeName(s.GetArgStr());
			return true;
		case OBC_NIGHTSIGHT:
			m_NightSight = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_REFLECTPHYSICALDAM:
			m_ReflectPhysicalDamage = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_RANGE:
		{
			INT64 piVal[2];
			size_t iQty = Str_ParseCmds(s.GetArgStr(), piVal, COUNTOF(piVal));
			if ( iQty > 1 )
			{
				INT64 iRange = ((piVal[0] & 0xFF) << 8) & 0xFF00;
				iRange |= (piVal[1] & 0xFF);
				SetDefNum(s.GetKey(), iRange, false);
				//m_range = ((piVal[0] & 0xFF) << 8) & 0xFF00;
				//m_range |= (piVal[1] & 0xFF);
			}
			else
			{
				SetDefNum(s.GetKey(), piVal[0], false);
				//m_range = static_cast<WORD>(piVal[0]);
			}
			return true;
		}
		case OBC_RESCOLD:
			m_ResCold = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_RESCOLDMAX:
			m_ResColdMax = static_cast<int>(s.GetArgVal());
			return true;
		case OBC_RESDISPDNHUE:
			SetResDispDnHue(static_cast<HUE_TYPE>(s.GetArgVal()));
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
		case OBC_RESLEVEL:
			return SetResLevel(minimum(maximum(static_cast<BYTE>(s.GetArgVal()), RDS_NONE), RDS_QTY - 1));
		case OBC_RESOURCES:
			m_BaseResources.Load(s.GetArgStr());
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
			return m_TEvents.r_LoadVal(s, RES_EVENTS);
	}
	return CScriptObj::r_LoadVal(s);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}
