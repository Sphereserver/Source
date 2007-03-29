#include "graysvr.h"

/////////////////////////////////////////////////////////////////
// -CCharBase

CCharBase::CCharBase(CREID_TYPE id)
		: CBaseBaseDef(RESOURCE_ID(RES_CHARDEF, id))
{
	m_iHireDayWage = 0;
	m_trackID = ITEMID_TRACK_WISP;
	m_soundbase = 0;
	m_defense = 0;
	m_Anims = 0xFFFFFF;
  	m_MaxFood = 0;			// Default value
	m_wBloodHue = 0;
	m_Str = 0;
	m_Dex = 0;

	m_iMoveRate = g_Cfg.m_iMoveRate;
	m_wDispIndex = IsValidDispID(id) ? id : 0;
}

LPCTSTR CCharBase::GetTradeName() const
{
	// From "Bill the carpenter" or "#HUMANMALE the Carpenter",
	// Get "Carpenter"
	LPCTSTR pName = CBaseBaseDef::GetTypeName();
	if ( pName[0] != '#' )
		return( pName );

	LPCTSTR pSpace = strchr( pName, ' ' );
	if ( pSpace == NULL )
		return( pName );

	pSpace++;
	if ( ! strnicmp( pSpace, "the ", 4 ))
		pSpace += 4;
	return( pSpace );
}

void CCharBase::CopyBasic( const CCharBase * pCharDef )
{
	m_trackID = pCharDef->m_trackID;
	m_soundbase = pCharDef->m_soundbase;

	m_wBloodHue = pCharDef->m_wBloodHue;
	m_MaxFood = pCharDef->m_MaxFood;
	m_FoodType = pCharDef->m_FoodType;
	m_Desires = pCharDef->m_Desires;

	m_defense = pCharDef->m_defense;
	m_Anims = pCharDef->m_Anims;

	m_BaseResources = pCharDef->m_BaseResources;

	CBaseBaseDef::CopyBasic( pCharDef );	// This will overwrite the CResourceLink!!
}

bool CCharBase::SetDispID( CREID_TYPE id )
{
	// Setting the "ID" for this.
	if (( id == GetID() ) || ( id == GetDispID() ))
		return true;
	if ( !IsValidDispID(id) )
	{
		g_Log.Error("ID 0%x too high (limited by %d).\n", id, CREID_QTY);
		return false;
	}

	// Copy the rest of the stuff from the display base.
	CCharBase	*pCharDef = FindCharBase( id );
	if ( !pCharDef )
	{
		g_Log.Error("ID 0%x undefined.\n", id);
		return false;
	}

	CopyBasic(pCharDef);
	return true;
}

void CCharBase::SetFoodType( LPCTSTR pszFood )
{
  	m_FoodType.Load( pszFood );

  	// Try to determine the real value
	m_MaxFood = 0;
	for ( int i=0; i<m_FoodType.GetCount(); i++ )
	{
		if ( m_MaxFood < m_FoodType[i].GetResQty())
			m_MaxFood = m_FoodType[i].GetResQty();
	}
}

enum CBC_TYPE
{
	#define ADD(a,b) CBC_##a,
	#include "tables/CCharBase_props.tbl"
	#undef ADD
	CBC_QTY,
};

LPCTSTR const CCharBase::sm_szLoadKeys[CBC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "tables/CCharBase_props.tbl"
	#undef ADD
	NULL,
};

bool CCharBase::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	EXC_TRY("WriteVal");
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case CBC_ANIM:
			sVal.FormatHex( m_Anims );
			break;
		case CBC_BLOODCOLOR:
			sVal.FormatHex( m_wBloodHue );
			break;
		case CBC_ARMOR:
			sVal.FormatVal( m_defense );
			break;
		case CBC_DESIRES:
			{
				TEMPSTRING(pszTmp);
				m_Desires.WriteKeys(pszTmp);
				sVal = pszTmp;
			}
			break;
		case CBC_DEX:
			sVal.FormatVal( m_Dex );
			break;
		case CBC_DISPID:
			sVal = g_Cfg.ResourceGetName( RESOURCE_ID( RES_CHARDEF, GetDispID()));
			break;
		case CBC_FOODTYPE:
			{
				TEMPSTRING(pszTmp);
				m_FoodType.WriteKeys(pszTmp);
				sVal = pszTmp;
			}
			break;
		case CBC_HIREDAYWAGE:
			sVal.FormatVal( m_iHireDayWage );
			break;
		case CBC_ICON:
			sVal.FormatHex( m_trackID );
			break;
		case CBC_JOB:
			sVal = GetTradeName();
			break;
		case CBC_MAXFOOD:
			sVal.FormatVal( m_MaxFood );
			break;
		case CBC_MOVERATE:
			sVal.FormatVal(m_iMoveRate);
			break;
		case CBC_SOUND:
			sVal.FormatHex( m_soundbase );
			break;
		case CBC_STR:
			sVal.FormatVal( m_Str );
			break;
		case CBC_TSPEECH:
			m_Speech.WriteResourceRefList( sVal );
			break;
		case CBC_TEVENTS:
			m_TEvents.WriteResourceRefList(sVal);
			break;
		default:
			return( CBaseBaseDef::r_WriteVal( pszKey, sVal ));
	}
	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CCharBase::r_LoadVal( CScript & s )
{
	EXC_TRY("LoadVal");
	if ( ! s.HasArgs())
		return false;
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		// compatibility for now
		case CBC_ANIM:
			m_Anims = s.GetArgVal();
			break;
		case CBC_BLOODCOLOR:
			m_wBloodHue = s.GetArgVal();
			break;
		case CBC_ARMOR:
			m_defense = s.GetArgVal();
			break;
		case CBC_DESIRES:
			m_Desires.Load( s.GetArgStr() );
			break;
		case CBC_DEX:
			m_Dex = s.GetArgVal();
			break;
		case CBC_DISPID:
			return false;
		case CBC_FOODTYPE:
			SetFoodType( s.GetArgStr());
			break;
		case CBC_HIREDAYWAGE:
			m_iHireDayWage = s.GetArgVal();
			break;
		case CBC_ICON:
			{
				ITEMID_TYPE id = (ITEMID_TYPE) g_Cfg.ResourceGetIndexType( RES_ITEMDEF, s.GetArgStr());
				if ( id < 0 || id >= ITEMID_MULTI )
				{
					return false;
				}
				m_trackID = id;
			}
			break;
		case CBC_ID:
			{
				return SetDispID( (CREID_TYPE) g_Cfg.ResourceGetIndexType( RES_CHARDEF, s.GetArgStr()));
			}
		case CBC_MAXFOOD:
			m_MaxFood = s.GetArgVal();
			break;
		case CBC_MOVERATE:
			m_iMoveRate = s.GetArgVal();
			break;
		case CBC_SOUND:
			m_soundbase = s.GetArgVal();
			break;
		case CBC_STR:
			m_Str = s.GetArgVal();
			break;
		case CBC_TSPEECH:
			return( m_Speech.r_LoadVal( s, RES_SPEECH ));
		case CBC_TEVENTS:
			return ( m_TEvents.r_LoadVal(s, RES_EVENTS));
		default:
			return( CBaseBaseDef::r_LoadVal( s ));
	}
	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CCharBase::r_Load( CScript & s )
{
	// Do a prelim read from the script file.
	CScriptObj::r_Load(s);

	if ( m_sName.IsEmpty() )
	{
		g_Log.Error("Char script '%s' has no name!\n", GetResourceName());
		return false;
	}

	if ( !IsValidDispID(GetDispID()) )
	{
 		g_Log.Event(LOGL_WARN, "Char script '%s' has bad DISPID 0%x. Defaulting to 0%d.\n", GetResourceName(), GetDispID(), (int)CREID_MAN);
		m_wDispIndex = CREID_MAN;
	}
	if ( m_Can == CAN_C_INDOORS )
		g_Log.Event(LOGL_WARN, "Char script '%s' has no CAN flags specified!\n", GetResourceName());

	return true;
}

////////////////////////////////////////////

CCharBase * CCharBase::FindCharBase( CREID_TYPE baseID ) // static
{
	// find it (or near it) if already loaded.
	RESOURCE_ID rid = RESOURCE_ID( RES_CHARDEF, baseID );
	int index = g_Cfg.m_ResHash.FindKey(rid);
	if ( index < 0 )
		return NULL;

	CResourceLink * pBaseLink = static_cast <CResourceLink *> ( g_Cfg.m_ResHash.GetAt(rid,index));
	CCharBase * pBase = dynamic_cast <CCharBase *> (pBaseLink);
	if ( pBase )
		return( pBase );	// already loaded.

	// create a new base.
	pBase = new CCharBase(baseID);
	pBase->CResourceLink::CopyTransfer(pBaseLink);

	// replace existing one
	g_Cfg.m_ResHash.SetAt(rid, index, pBase);

	// load it's data on demand.
	CResourceLock s;
	if ( !pBase->ResourceLock(s))
		return NULL;
	if ( !pBase->r_Load(s))
		return NULL;

	return pBase;
}
