//
// CItemBase.CPP
// Copyright Menace Software (www.menasoft.com).
// define the base types of an item (rather than the instance)
//
#include "graysvr.h"	// predef header.

/////////////////////////////////////////////////////////////////
// -CItemBase

CItemBase::CItemBase( ITEMID_TYPE id ) :
	CBaseBaseDef( RESOURCE_ID( RES_ITEMDEF, id ))
{
	m_weight = 0;
	m_speed = 0;
	m_iSkill = SKILL_NONE;
	m_range = 1;
	m_type = IT_NORMAL;
	m_layer = LAYER_NONE;

	// Just applies to equippable weapons/armor.
	m_ttNormal.m_tData1 = 0;
	m_ttNormal.m_tData2 = 0;
	m_ttNormal.m_tData3 = 0;
	m_ttNormal.m_tData4 = 0;

	if ( ! IsValidDispID( id ))
	{
		// There should be an ID= in the scripts later.
		m_dwDispIndex = ITEMID_GOLD_C1; // until i hear otherwise from the script file.
		return;
	}

	// Set the artwork/display id.
	m_dwDispIndex = id;

	// I have it indexed but it needs to be loaded.
	// read it in from the script and *.mul files.

	CUOItemTypeRec2 tiledata;
	memset( &tiledata, 0, sizeof(tiledata));
	if ( id < ITEMID_MULTI )
	{
		if ( ! CItemBase::GetItemData( id, &tiledata ))	// some valid items don't show up here !
		{
			// return( NULL );
		}
	}
	else
	{
		tiledata.m_weight = 0xFF;
	}

	m_dwFlags = tiledata.m_flags;
	m_type = GetTypeBase( id, tiledata );

	// Stuff read from .mul file.
	// Some items (like hair) have no names !
	// Get rid of the strange leading spaces in some of the names.
	TCHAR szName[ sizeof(tiledata.m_name)+1 ];
	size_t j = 0;
	for ( size_t i = 0; i < sizeof(tiledata.m_name) && tiledata.m_name[i]; i++ )
	{
		if ( j == 0 && ISWHITESPACE(tiledata.m_name[i]))
			continue;
		szName[j++] = tiledata.m_name[i];
	}

	szName[j] = '\0';
	m_sName = szName;	// default type name.

	// Do some special processing for certain items.

	if ( IsType(IT_CHAIR))
	{
		SetHeight( 0 ); // have no effective height if they don't block.
	}
	else
	{
		//CBaseBaseDef * pBaseBaseDef = dynamic_cast<CBaseBaseDef*>(this);
		//pBaseBaseDef->SetHeight( GetItemHeightFlags( tiledata, m_Can ));
		SetHeight( GetItemHeightFlags( tiledata, m_Can ));
		//DEBUG_WARN(("GetItemHeightFlags( tiledata, m_Can )(%d),tiledata.m_height(%d),m_dwDispIndex(0%x),GetHeight()(%d),pBaseBaseDef->GetHeight()(%d)\n",GetItemHeightFlags( tiledata, m_Can ),tiledata.m_height,m_wDispIndex,GetHeight(),pBaseBaseDef->GetHeight()));
	}
	//DEBUG_ERR(("ID: 0%x; this (0x%x), m_dwFlags (0%x), tiledata.m_flags (0%x), tiledata.m_height (%d) GetHeight() (%d)\n",id,this,m_dwFlags,tiledata.m_flags,tiledata.m_height,GetHeight()));

	GetItemSpecificFlags( tiledata, m_Can, m_type, id );

	if ( tiledata.m_weight == 0xFF ||	// not movable.
		( tiledata.m_flags & UFLAG1_WATER ))
	{
		// water can't be picked up.
		m_weight = USHRT_MAX;
	}
	else
	{
		m_weight = tiledata.m_weight * WEIGHT_UNITS;
	}

	if ( tiledata.m_flags & ( UFLAG1_EQUIP | UFLAG3_EQUIP2 ))
	{
		m_layer = tiledata.m_layer;
		if ( m_layer && ! IsMovableType())
		{
			// How am i supposed to equip something i can't pick up ?
			m_weight = WEIGHT_UNITS;
		}
	}

	// ResDisp
	SetResDispDnId(ITEMID_GOLD_C1);
}

CItemBase::~CItemBase()
{
	// These don't really get destroyed til the server is shut down but keep this around anyhow.
}


void CItemBase::SetTypeName( LPCTSTR pszName )
{
	ADDTOCALLSTACK("CItemBase::SetTypeName");
	ASSERT(pszName);
	if ( ! strcmp( pszName, GetTypeName()))
		return;
	m_dwFlags |= UFLAG2_ZERO1;	// we override the name
	CBaseBaseDef::SetTypeName( pszName );
}

LPCTSTR CItemBase::GetArticleAndSpace() const
{
	ADDTOCALLSTACK("CItemBase::GetArticleAndSpace");
	if ( IsSetOF(OF_NoPrefix) )
	{
		return( "" );
	}
	if ( m_dwFlags & UFLAG2_ZERO1 )	// Name has been changed from TILEDATA.MUL
	{
		return( Str_GetArticleAndSpace( GetTypeName() ));
	}
	if ( m_dwFlags & UFLAG2_AN )
	{
		return( "an " );
	}
	if ( m_dwFlags & UFLAG2_A )
	{
		return( "a " );
	}
	return( "" );
}

void CItemBase::CopyBasic( const CItemBase * pBase )
{
	ADDTOCALLSTACK("CItemBase::CopyBasic");
	m_speed		= pBase->m_speed;
	m_weight	= pBase->m_weight;
	m_flip_id	= pBase->m_flip_id;
	m_type		= pBase->m_type;
	m_layer		= pBase->m_layer;
	m_range		= pBase->m_range;

	// Just applies to weapons/armor.
	m_ttNormal.m_tData1 = pBase->m_ttNormal.m_tData1;
	m_ttNormal.m_tData2 = pBase->m_ttNormal.m_tData2;
	m_ttNormal.m_tData3 = pBase->m_ttNormal.m_tData3;
	m_ttNormal.m_tData4 = pBase->m_ttNormal.m_tData4;

	CBaseBaseDef::CopyBasic( pBase );	// This will overwrite the CResourceLink!!
}

void CItemBase::CopyTransfer( CItemBase * pBase )
{
	ADDTOCALLSTACK("CItemBase::CopyTransfer");
	CopyBasic( pBase );

	m_values = pBase->m_values;
	m_SkillMake = pBase->m_SkillMake;

	CBaseBaseDef::CopyTransfer( pBase );	// This will overwrite the CResourceLink!!
}

LPCTSTR CItemBase::GetName() const
{
	ADDTOCALLSTACK("CItemBase::GetName");
	// Get rid of the strange %s type stuff for pluralize rules of names.
	return( GetNamePluralize( GetTypeName(), false ));
}

TCHAR * CItemBase::GetNamePluralize( LPCTSTR pszNameBase, bool fPluralize )	// static
{
	ADDTOCALLSTACK("CItemBase::GetNamePluralize");
	TCHAR * pszName = Str_GetTemp();
	size_t j = 0;
	bool fInside = false;
	bool fPlural = false;
	for ( size_t i = 0; pszNameBase[i]; i++ )
	{
		if ( pszNameBase[i] == '%' )
		{
			fInside = ! fInside;
			fPlural = true;
			continue;
		}
		if ( fInside )
		{
			if ( pszNameBase[i] == '/' )
			{
				fPlural = false;
				continue;
			}
			if ( fPluralize )
			{
				if ( ! fPlural )
					continue;
			}
			else
			{
				if ( fPlural )
					continue;
			}
		}
		pszName[j++] = pszNameBase[i];
	}
	pszName[j] = '\0';
	return( pszName );
}

CREID_TYPE CItemBase::FindCharTrack( ITEMID_TYPE trackID )	// static
{
	ADDTOCALLSTACK("CItemBase::FindCharTrack");
	// For figurines. convert to a creature.
	// IT_EQ_HORSE
	// IT_FIGURINE

	CItemBase * pItemDef = CItemBase::FindItemBase( trackID );
	if ( pItemDef == NULL )
	{
		return( CREID_INVALID );
	}
	if ( ! pItemDef->IsType(IT_EQ_HORSE) && ! pItemDef->IsType(IT_FIGURINE) )
	{
		return( CREID_INVALID );
	}

	return static_cast<CREID_TYPE>(pItemDef->m_ttFigurine.m_charid.GetResIndex());
}

bool CItemBase::IsTypeArmor( IT_TYPE type )  // static
{
	switch( type )
	{
		case IT_CLOTHING:
		case IT_ARMOR:
		case IT_ARMOR_LEATHER:
		case IT_SHIELD:
			return( true );

		default:
			return( false );
	}
}
bool CItemBase::IsTypeWeapon( IT_TYPE type )  // static
{
	// NOTE: a wand can be a weapon.
	switch( type )
	{
		case IT_WEAPON_MACE_STAFF:
		case IT_WEAPON_MACE_CROOK:
		case IT_WEAPON_MACE_PICK:
		case IT_WEAPON_AXE:
		case IT_WEAPON_XBOW:
		case IT_WEAPON_THROWING:
			return( true );

		default:
			return( type >= IT_WEAPON_MACE_SMITH && type <= IT_WAND );
	}
}

GUMP_TYPE CItemBase::IsTypeContainer() const
{
	ADDTOCALLSTACK("CItemBase::IsTypeContainer");
	// IT_CONTAINER
	// return the container gump id.

	switch ( m_type )
	{
		case IT_CONTAINER:
		case IT_SIGN_GUMP:
		case IT_SHIP_HOLD:
		case IT_BBOARD:
		case IT_CORPSE:
		case IT_TRASH_CAN:
		case IT_GAME_BOARD:
		case IT_EQ_BANK_BOX:
		case IT_EQ_VENDOR_BOX:
		case IT_KEYRING:
			return(	m_ttContainer.m_gumpid );
		default:
			return( GUMP_NONE );
	}
}

bool CItemBase::IsTypeSpellbook( IT_TYPE type )  // static
{
	switch( type )
	{
		case IT_SPELLBOOK:
		case IT_SPELLBOOK_NECRO:
		case IT_SPELLBOOK_PALA:
		case IT_SPELLBOOK_EXTRA:
		case IT_SPELLBOOK_BUSHIDO:
		case IT_SPELLBOOK_NINJITSU:
		case IT_SPELLBOOK_ARCANIST:
		case IT_SPELLBOOK_MYSTIC:
		case IT_SPELLBOOK_BARD:
			return( true );
		default:
			return( false );
	}
}

bool CItemBase::IsTypeMulti( IT_TYPE type )	// static
{
	switch( type )
	{
		case IT_MULTI:
		case IT_MULTI_CUSTOM:
		case IT_SHIP:
			return( true );

		default:
			return( false );
	}
}

bool CItemBase::IsTypeEquippable() const
{
	ADDTOCALLSTACK("CItemBase::IsTypeEquippable");
	// Equippable on (possibly) visible layers.

	switch ( m_type )
	{
		case IT_LIGHT_LIT:
		case IT_LIGHT_OUT:	// Torches and lanterns.
		case IT_FISH_POLE:
		case IT_HAIR:
		case IT_BEARD:
		case IT_JEWELRY:
		case IT_EQ_HORSE:
			return( true );
		default:
			break;
	}
	if ( IsTypeSpellbook( m_type ))
		return( true );
	if ( IsTypeArmor( m_type ))
		return( true );
	if ( IsTypeWeapon( m_type ))
		return( true );

	// Even not visible things.
	switch ( m_type )
	{
		case IT_EQ_BANK_BOX:
		case IT_EQ_VENDOR_BOX:
		case IT_EQ_CLIENT_LINGER:
		case IT_EQ_MURDER_COUNT:
		case IT_EQ_STUCK:
		case IT_EQ_TRADE_WINDOW:
		case IT_EQ_MEMORY_OBJ:
		case IT_EQ_SCRIPT:
			if ( IsVisibleLayer( static_cast<LAYER_TYPE>(m_layer)))
				return( false );
			return( true );
		default:
			break;
	}

	return( false );
}

bool CItemBase::IsID_Multi( ITEMID_TYPE id ) // static
{
	// NOTE: Ships are also multi's
	return( id >= ITEMID_MULTI && id < ITEMID_MULTI_MAX );
}

int CItemBase::IsID_Door( ITEMID_TYPE id ) // static
{
	ADDTOCALLSTACK("CItemBase::IsID_Door");
	// IT_DOOR
	static const ITEMID_TYPE sm_Item_DoorBase[] =
	{
		ITEMID_DOOR_SECRET_1,
		ITEMID_DOOR_SECRET_2,
		ITEMID_DOOR_SECRET_3,
		ITEMID_DOOR_SECRET_4,
		ITEMID_DOOR_SECRET_5,
		ITEMID_DOOR_SECRET_6,
		ITEMID_DOOR_METAL_S,
		ITEMID_DOOR_BARRED,
		ITEMID_DOOR_RATTAN,
		ITEMID_DOOR_WOODEN_1,
		ITEMID_DOOR_WOODEN_2,
		ITEMID_DOOR_METAL_L,
		ITEMID_DOOR_WOODEN_3,
		ITEMID_DOOR_WOODEN_4,
		ITEMID_DOOR_IRONGATE_1,
		ITEMID_DOOR_WOODENGATE_1,
		ITEMID_DOOR_IRONGATE_2,
		ITEMID_DOOR_WOODENGATE_2,
		ITEMID_DOOR_BAR_METAL,
		//ITEMID_DOOR_WOOD_BLACK,
		//ITEMID_DOOR_ELVEN_BARK,
		//ITEMID_DOOR_ELVEN_SIMPLE,
		//ITEMID_DOOR_ELVEN_ORNATE,
		//ITEMID_DOOR_ELVEN_PLAIN,
		//ITEMID_DOOR_MOON,
		ITEMID_DOOR_CRYSTAL,
		ITEMID_DOOR_SHADOW,
		//ITEMID_DOOR_WALLSET3,
		//ITEMID_DOOR_GARGOYLE,
		//ITEMID_DOOR_QUEEN
	};

	if ( id == 0x190e )
	{
		// anomoly door. bar door just has 2 pieces.
		return( 1 );
	}
	if ( id == 0x190f )
	{
		// anomoly door. bar door just has 2 pieces.
		return( 2 );
	}

	for ( size_t i = 0; i < COUNTOF(sm_Item_DoorBase); i++)
	{
		int did = id - sm_Item_DoorBase[i];
		if ( did >= 0 && did <= 15 )
			return( did+1 );
	}
	return( 0 );
}

bool CItemBase::IsID_DoorOpen( ITEMID_TYPE id ) // static
{
	ADDTOCALLSTACK("CItemBase::IsID_DoorOpen");
  	int doordir = IsID_Door(id)-1;
    if ( doordir < 0 )
		return false;
    if ( doordir & DOOR_OPENED )
		return true;
	return( false );
}

bool IsID_Ship( ITEMID_TYPE id )
{
	// IT_SHIP
	return( id >= ITEMID_MULTI && id <= ITEMID_SHIP6_W );
}

bool IsID_GamePiece( ITEMID_TYPE id ) // static
{
	return( id >= ITEMID_GAME1_CHECKER && id <= ITEMID_GAME_HI );
}

bool IsID_Track( ITEMID_TYPE id ) // static
{
	return( id >= ITEMID_TRACK_BEGIN && id <= ITEMID_TRACK_END );
}

bool IsID_WaterFish( ITEMID_TYPE id ) // static
{
	// IT_WATER
	// Assume this means water we can fish in.
	// Not water we can wash in.
	if ( id >= 0x1796 && id <= 0x17b2 )
		return( true );
	if ( id == 0x1559 )
		return( true );
	return( false );
}

bool IsID_WaterWash( ITEMID_TYPE id ) // static
{
	// IT_WATER_WASH
	if ( id >= ITEMID_WATER_TROUGH_1 && id <= ITEMID_WATER_TROUGH_2	)
		return( true );
	return( IsID_WaterFish( id ));
}

//static bool IsID_Chair( ITEMID_TYPE id ) // static
bool IsID_Chair( ITEMID_TYPE id ) // static
{
	// Strangely there is not chair flag in the statics.mul file ??? !!!
	// IT_CHAIR

	// todo: consider enum values for these chairs
	switch (static_cast<WORD>(id))
	{
		case 0x0459: // 'marble bench'
		case 0x045a: // 'marble bench'
		case 0x045b: // 'stone bench'
		case 0x045c: // 'stone bench'
		case 0x0b2c: // 'wooden bench'
		case 0x0b2d: // 'wooden bench'
		case 0x0b2e: // 'wooden chair'
		case 0x0b2f: // 'wooden chair'
		case 0x0b30: // 'wooden chair'
		case 0x0b31: // 'wooden chair'
		case 0x0b32: // 'throne'
		case 0x0b33: // 'throne'
		case 0x0b4e: // 'chair'
		case 0x0b4f: // 'chair'
		case 0x0b50: // 'chair'
		case 0x0b51: // 'chair'
		case 0x0b52: // 'chair'
		case 0x0b53: // 'chair'
		case 0x0b54: // 'chair'
		case 0x0b55: // 'chair'
		case 0x0b56: // 'chair'
		case 0x0b57: // 'chair'
		case 0x0b58: // 'chair'
		case 0x0b59: // 'chair'
		case 0x0b5a: // 'chair'
		case 0x0b5b: // 'chair'
		case 0x0b5c: // 'chair'
		case 0x0b5d: // 'chair'
		case 0x0b5e: // 'foot stool'
		case 0x0b5f: // 'bench'
		case 0x0b60: // 'bench'
		case 0x0b61: // 'bench'
		case 0x0b62: // 'bench'
		case 0x0b63: // 'bench'
		case 0x0b64: // 'bench'
		case 0x0b65: // 'bench'
		case 0x0b66: // 'bench'
		case 0x0b67: // 'bench'
		case 0x0b68: // 'bench'
		case 0x0b69: // 'bench'
		case 0x0b6a: // 'bench'
		case 0x0b91: // 'bench'
		case 0x0b92: // 'bench'
		case 0x0b93: // 'bench'
		case 0x0b94: // 'bench'
		case 0x0c17: // 'covered chair'
		case 0x0c18: // 'covered chair'
		case 0x1049: // 'loom bench'
		case 0x104a: // 'loom bench'
		case 0x1207: // 'stone bench'
		case 0x1208: // 'stone bench'
		case 0x1209: // 'stone bench'
		case 0x120a: // 'stone bench'
		case 0x120b: // 'stone bench'
		case 0x120c: // 'stone bench'
		case 0x1218: // 'stone chair'
		case 0x1219: // 'stone chair'
		case 0x121a: // 'stone chair'
		case 0x121b: // 'stone chair'
		case 0x1526: // 'throne'
		case 0x1527: // 'throne'
		case 0x19f1: // 'woodworker's bench'
		case 0x19f2: // 'woodworker's bench'
		case 0x19f3: // 'woodworker's bench'
		case 0x19f5: // 'woodworker's bench'
		case 0x19f6: // 'woodworker's bench'
		case 0x19f7: // 'woodworker's bench'
		case 0x19f9: // 'cooper's bench'
		case 0x19fa: // 'cooper's bench'
		case 0x19fb: // 'cooper's bench'
		case 0x19fc: // 'cooper's bench'
		case 0x1dc7: // 'sandstone bench'
		case 0x1dc8: // 'sandstone bench'
		case 0x1dc9: // 'sandstone bench'
		case 0x1dca: // 'sandstone bench'
		case 0x1dcb: // 'sandstone bench'
		case 0x1dcc: // 'sandstone bench'
		case 0x1dcd: // 'marble bench'
		case 0x1dce: // 'marble bench'
		case 0x1dcf: // 'marble bench'
		case 0x1dd0: // 'marble bench'
		case 0x1dd1: // 'marble bench'
		case 0x1dd2: // 'marble bench'
		case 0x1e6f: // 'chair'
		case 0x1e78: // 'chair'
		case 0x3dff: // 'bench'
		case 0x3e00: // 'bench'
			return( true );

		default:
			return( false );
	}
}

bool CItemBase::GetItemData( ITEMID_TYPE id, CUOItemTypeRec2 * pData ) // static
{
	ADDTOCALLSTACK("CItemBase::GetItemData");
	// Read from g_Install.m_fTileData
	// Get an Item tiledata def data.
	// Invalid object id ?
	// NOTE: This data should already be read into the m_ItemBase table ???

	if ( ! IsValidDispID(id))
		return( false );

	try
	{
		CGrayItemInfo info( id );
		*pData = *( static_cast <CUOItemTypeRec2 *>( & info ));
	}
	catch ( const CGrayError& e )
	{
		g_Log.CatchEvent( &e, "GetItemData" );
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		return( false );
	}
	catch (...)
	{
		g_Log.CatchEvent(NULL, "GetItemData" );
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		return( false );
	}

	// Unused tiledata I guess. Don't create it.
	if ( ! pData->m_flags &&
		! pData->m_weight &&
		! pData->m_layer &&
		! pData->m_dwUnk11 &&
		! pData->m_dwAnim &&
		! pData->m_wUnk19 &&
		! pData->m_height &&
		! pData->m_name[0]
		)
	{
		// What are the exceptions to the rule ?
		if ( id == ITEMID_BBOARD_MSG ) // special
			return( true );
		if ( IsID_GamePiece( id ))
			return( true );
		if ( IsID_Track(id))	// preserve these
			return( true );
		return( false );
	}
	return( true );
}

inline void CItemBase::GetItemSpecificFlags( const CUOItemTypeRec2 & tiledata, DWORD & wBlockThis, IT_TYPE type, ITEMID_TYPE id ) // static
{
	ADDTOCALLSTACK("CItemBase::GetItemSpecificFlags");
	if ( type == IT_DOOR )
	{
		wBlockThis &= ~CAN_I_BLOCK;
		if ( IsID_DoorOpen(id))
		{
			wBlockThis &= ~CAN_I_DOOR;
		}
		else
		{
			wBlockThis |= CAN_I_DOOR;
		}
	}

	if ( tiledata.m_flags & UFLAG3_LIGHT )	// this may actually be a moon gate or fire ?
	{
		wBlockThis |= CAN_I_LIGHT;	// normally of type IT_LIGHT_LIT;
	}

	if (( tiledata.m_flags & UFLAG2_STACKABLE ) || type == IT_REAGENT ||
		id == ITEMID_EMPTY_BOTTLE )
	{
		wBlockThis |= CAN_I_PILE;
	}
}

inline height_t CItemBase::GetItemHeightFlags( const CUOItemTypeRec2 & tiledata, DWORD & wBlockThis ) // static
{
	ADDTOCALLSTACK("CItemBase::GetItemHeightFlags");
	// Chairs are marked as blocking for some reason ?
	if ( tiledata.m_flags & UFLAG4_DOOR ) // door
	{
		wBlockThis = CAN_I_DOOR;
		return( tiledata.m_height );
	}
	//if (( tiledata.m_flags & UFLAG1_BLOCK ) || (( IsSetEF(EF_NewPositionChecks) ) && ( tiledata.m_flags & UFLAG1_WALL )))
	if ( tiledata.m_flags & UFLAG1_BLOCK )
	{
		if ( tiledata.m_flags & UFLAG1_WATER )	// water
		{
			wBlockThis = CAN_I_WATER;
			return( tiledata.m_height );
		}
		wBlockThis = CAN_I_BLOCK;
	}
	else
	{
		//DEBUG_ERR(("tiledata.m_flags 0x%x\n",tiledata.m_flags));
		wBlockThis = 0;
		if ( ! ( tiledata.m_flags & (UFLAG2_PLATFORM|UFLAG4_ROOF|UFLAG4_HOVEROVER) ))
			return 0;	// have no effective height if it doesn't block.
	}
	if ( IsSetEF( EF_WalkCheck ) )
	{
		if ( tiledata.m_flags & UFLAG4_ROOF)
		{
			wBlockThis |= CAN_I_BLOCK;
		}
		else if ( tiledata.m_flags & UFLAG2_PLATFORM )
		{
			wBlockThis |= CAN_I_PLATFORM;
		}
	}
	else
	{
		if ( tiledata.m_flags & (UFLAG2_PLATFORM|UFLAG4_ROOF) )
		{
			wBlockThis |= CAN_I_PLATFORM;
		}
	}
	if ( tiledata.m_flags & UFLAG2_CLIMBABLE )
	{
		// actual standing height is height/2
		wBlockThis |= CAN_I_CLIMB;
	}
	if ( tiledata.m_flags & UFLAG4_HOVEROVER )
		wBlockThis |= CAN_I_HOVER;
	//DEBUG_WARN(("tiledata.m_height(%d)\n",tiledata.m_height));
	return( tiledata.m_height );
}

height_t CItemBase::GetItemHeight( ITEMID_TYPE id, DWORD & wBlockThis ) // static
{
	ADDTOCALLSTACK("CItemBase::GetItemHeight");
	// Get just the height and the blocking flags for the item by id.
	// used for walk block checking.

	RESOURCE_ID rid = RESOURCE_ID( RES_ITEMDEF, id );
	size_t index = g_Cfg.m_ResHash.FindKey(rid);
	if ( index != g_Cfg.m_ResHash.BadIndex() ) // already loaded ?
	{
		CResourceDef * pBaseStub = g_Cfg.m_ResHash.GetAt( rid, index );
		ASSERT(pBaseStub);
		CItemBase * pBase = dynamic_cast <CItemBase *>(pBaseStub);
		if ( pBase )
		{
			wBlockThis = pBase->m_Can & CAN_I_MOVEMASK;
			return( pBase->GetHeight() );
		}
	}
	
	// Not already loaded.

	CUOItemTypeRec2 tiledata;
	if ( ! GetItemData( id, &tiledata ))
	{
		wBlockThis = CAN_I_MOVEMASK;
		return( UO_SIZE_Z );
	}
	if ( IsID_Chair( id ))
	{
		wBlockThis = 0;
		return 0;	// have no effective height if they don't block.
	}
	return( GetItemHeightFlags( tiledata, wBlockThis ));
}

IT_TYPE CItemBase::GetTypeBase( ITEMID_TYPE id, const CUOItemTypeRec2 &tiledata ) // static
{
	ADDTOCALLSTACK("CItemBase::GetTypeBase");
	if ( IsID_Ship( id ))
		return IT_SHIP;
	if ( IsID_Multi( id ))
		return IT_MULTI;

	if (( tiledata.m_flags & UFLAG1_BLOCK ) && (tiledata.m_flags & UFLAG1_WATER))
		return IT_WATER;
	if ( IsID_WaterFish( id )) // UFLAG1_WATER
		return IT_WATER;

	if (( tiledata.m_flags & UFLAG4_DOOR ) || IsID_Door( id ))
	{
		if ( IsID_DoorOpen( id ))	// currently open
			return IT_DOOR_OPEN;
		return IT_DOOR;
	}

	if ( tiledata.m_flags & UFLAG3_CONTAINER )
		return IT_CONTAINER;

	if ( IsID_WaterWash( id ))
		return IT_WATER_WASH;
	else if ( IsID_Chair( id ))
		return IT_CHAIR;
	else if ( IsID_Track( id ))
		return IT_FIGURINE;
	else if ( IsID_GamePiece( id ))
		return IT_GAME_PIECE;

	// Get rid of the stuff below here !

	if (( tiledata.m_flags & UFLAG1_DAMAGE ) && ! ( tiledata.m_flags & UFLAG1_BLOCK ))
		return IT_TRAP_ACTIVE;

	if ( tiledata.m_flags & UFLAG3_LIGHT )	// this may actually be a moon gate or fire ?
	{
		return IT_LIGHT_LIT;
	}

	return IT_NORMAL;	// Get from script i guess.
}

ITEMID_TYPE CItemBase::GetNextFlipID( ITEMID_TYPE id ) const
{
	ADDTOCALLSTACK("CItemBase::GetNextFlipID");
	if ( m_flip_id.GetCount() > 0 )
	{
		ITEMID_TYPE idprev = GetDispID();
		for ( size_t i = 0; i < m_flip_id.GetCount(); i++ )
		{
			ITEMID_TYPE idnext = m_flip_id[i];
			if ( idprev == id )
				return( idnext );
			idprev = idnext;
		}
	}
	return( GetDispID());
}

bool CItemBase::IsSameDispID( ITEMID_TYPE id ) const
{
	ADDTOCALLSTACK("CItemBase::IsSameDispID");
	// Does this item look like the item we want ?
	// Take into account flipped items.

	if ( ! IsValidDispID( id ))	// this should really not be here but handle it anyhow.
	{
		return( GetID() == id );
	}

	if ( id == GetDispID())
		return( true );

	for ( size_t i = 0; i < m_flip_id.GetCount(); i ++ )
	{
		if ( m_flip_id[i] == id )
			return( true );
	}
	return( false );
}

void CItemBase::Restock()
{
	ADDTOCALLSTACK("CItemBase::Restock");
	// Re-evaluate the base random value rate some time in the future.
	if ( m_values.m_iLo < 0 || m_values.m_iHi < 0 )
	{
		m_values.Init();
	}
}

int CItemBase::CalculateMakeValue( int iQualityLevel ) const
{
	ADDTOCALLSTACK("CItemBase::CalculateMakeValue");
	// Calculate the value in gold for this item based on its components.
	// NOTE: Watch out for circular RESOURCES= list in the scripts.
	// ARGS:
	//   iQualityLevel = 0-100

	static int sm_iReentrantCount = 0;
	if ( sm_iReentrantCount > 32 )
	{
		DEBUG_ERR(( "Too many RESOURCES at item '%s' to calculate a value with (circular resource list?).\n", GetResourceName() ));
		return( 0 );
	}

	sm_iReentrantCount++;
	int lValue = 0;

	// add value based on the base resources making this up.
	for ( size_t i = 0; i < m_BaseResources.GetCount(); i++ )
	{
		RESOURCE_ID rid = m_BaseResources[i].GetResourceID();
		if ( rid.GetResType() != RES_ITEMDEF )
			continue;

		CItemBase * pItemDef = CItemBase::FindItemBase( static_cast<ITEMID_TYPE>(rid.GetResIndex()) );
		if ( pItemDef == NULL )
			continue;

		lValue += pItemDef->GetMakeValue( iQualityLevel ) * m_BaseResources[i].GetResQty();
	}

	// add some value based on the skill required to create it.
	for ( size_t i = 0; i < m_SkillMake.GetCount(); i++ )
	{
		RESOURCE_ID rid = m_SkillMake[i].GetResourceID();
		if ( rid.GetResType() != RES_SKILL )
			continue;
		const CSkillDef* pSkillDef = g_Cfg.GetSkillDef(static_cast<SKILL_TYPE>(rid.GetResIndex()));
		if ( pSkillDef == NULL )
			continue;

		// this is the normal skill required.
		// if iQuality is much less than iSkillNeed then something is wrong.
		int iSkillNeed = m_SkillMake[i].GetResQty();
		if ( iQualityLevel < iSkillNeed )
			iQualityLevel = iSkillNeed;

		lValue += pSkillDef->m_Values.GetLinear( iQualityLevel );
	}

	sm_iReentrantCount--;
	return( lValue );
}

int CItemBase::GetMakeValue( int iQualityLevel )
{
	ADDTOCALLSTACK("CItemBase::GetMakeValue");
	// Set the items value based on the resources and skill used to make it.
	// ARGS:
	// iQualityLevel = 0-100

	CValueRangeDef values = m_values;

	if ( m_values.m_iLo == INT_MIN || m_values.m_iHi == INT_MIN )
	{
		values.m_iLo = CalculateMakeValue(0);		// low quality specimen
		m_values.m_iLo = -values.m_iLo;			// negative means they will float.
		values.m_iHi = CalculateMakeValue(100); 		// Top quality specimen
		m_values.m_iHi = -values.m_iHi;
	}
	else
	{
		values.m_iLo = abs(values.m_iLo);
		values.m_iHi = abs(values.m_iHi);
	}

	return values.GetLinear(iQualityLevel*10);
}

void CItemBase::ResetMakeValue()
{
	ADDTOCALLSTACK("CItemBase::ResetMakeValue");
	m_values.m_iLo = INT_MIN;
	m_values.m_iHi = INT_MIN;
	GetMakeValue(0);
}

enum IBC_TYPE
{
	#define ADD(a,b) IBC_##a,
	#include "../tables/CItemBase_props.tbl"
	#undef ADD
	IBC_QTY
};

LPCTSTR const CItemBase::sm_szLoadKeys[IBC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CItemBase_props.tbl"
	#undef ADD
	NULL
};

bool CItemBase::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pChar )
{
	ADDTOCALLSTACK("CItemBase::r_WriteVal");
	EXC_TRY("WriteVal");
	switch ( FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case IBC_AMMOANIM:
		case IBC_AMMOANIMHUE:
		case IBC_AMMOANIMRENDER:
		case IBC_AMMOCONT:
		case IBC_AMMOTYPE:
			{
				sVal = GetDefStr(pszKey, false);
			}
			break;
		case IBC_DEFNAME:
			sVal = GetResourceName();
			break;
		case IBC_DISPID:
			sVal = g_Cfg.ResourceGetName( RESOURCE_ID( RES_ITEMDEF, GetDispID()));
			break;
		case IBC_DUPELIST:
			{
				TCHAR *pszTemp = Str_GetTemp();
				size_t iLen = 0;
				*pszTemp = '\0';
				for ( size_t i = 0; i < m_flip_id.GetCount(); i++ )
				{
					if ( i > 0 )
						iLen += strcpylen( pszTemp+iLen, "," );

					iLen += sprintf( pszTemp+iLen, "0%x", m_flip_id[i] );
					ASSERT(iLen < SCRIPT_MAX_LINE_LEN);
				}
				sVal = pszTemp;
			}
			break;
		case IBC_DYE:
			sVal.FormatHex(( m_Can & CAN_I_DYE ) ? true : false );
			break;
		case IBC_ENCHANT:
			sVal.FormatHex(( m_Can & CAN_I_ENCHANT ) ? true : false );
			break;
		case IBC_EXCEPTIONAL:
			sVal.FormatHex(( m_Can & CAN_I_EXCEPTIONAL ) ? true : false );
			break;
		case IBC_FLIP:
			sVal.FormatHex(( m_Can & CAN_I_FLIP ) ? true : false );
			break;
		case IBC_ID:
			sVal.FormatHex( GetDispID() );
			break;
		case IBC_IMBUE:
			sVal.FormatHex(( m_Can & CAN_I_IMBUE ) ? true : false );
			break;
		case IBC_ISARMOR:
			sVal.FormatVal( IsTypeArmor( m_type ) );
			break;
		case IBC_ISWEAPON:
			sVal.FormatVal( IsTypeWeapon( m_type ) );
			break;
		case IBC_MAKERSMARK:
			sVal.FormatHex(( m_Can & CAN_I_MAKERSMARK ) ? true : false );
			break;
		case IBC_RECYCLE:
			sVal.FormatHex(( m_Can & CAN_I_RECYCLE ) ? true : false );
			break;
		case IBC_REFORGE:
			sVal.FormatHex(( m_Can & CAN_I_REFORGE ) ? true : false );
			break;
		case IBC_RETAINCOLOR:
			sVal.FormatHex(( m_Can & CAN_I_RETAINCOLOR ) ? true : false );
			break;
		case IBC_SKILL:		// Skill to use.
			{
				if ( m_iSkill > SKILL_NONE && m_iSkill < SKILL_MAX )
				{
					sVal.FormatVal(m_iSkill);
					break;
				}

				SKILL_TYPE skill;
				switch ( GetType() )
				{
					case IT_WEAPON_MACE_CROOK:
					case IT_WEAPON_MACE_PICK:
					case IT_WEAPON_MACE_SMITH:	// Can be used for smithing ?
					case IT_WEAPON_MACE_STAFF:
					case IT_WEAPON_MACE_SHARP:	// war axe can be used to cut/chop trees.
						skill = SKILL_MACEFIGHTING;
						break;
					case IT_WEAPON_SWORD:
					case IT_WEAPON_AXE:
						skill = SKILL_SWORDSMANSHIP;
						break;
					case IT_WEAPON_FENCE:
						skill = SKILL_FENCING;
						break;
					case IT_WEAPON_BOW:
					case IT_WEAPON_XBOW:
						skill = SKILL_ARCHERY;
						break;
					case IT_WEAPON_THROWING:
						skill = SKILL_THROWING;
						break;
					default:
						skill = SKILL_NONE;
						break;
				}
				sVal.FormatVal(skill);
				break;
			}
		case IBC_LAYER:
			sVal.FormatVal( m_layer );
			break;
		case IBC_REPAIR:
			sVal.FormatHex(( m_Can & CAN_I_REPAIR ) ? true : false );
			break;
		case IBC_REPLICATE:
			sVal.FormatHex(( m_Can & CAN_I_REPLICATE ) ? true : false );
			break;
		case IBC_REQSTR:
			// IsTypeEquippable()
			sVal.FormatVal( m_ttEquippable.m_StrReq );
			break;
		case IBC_SKILLMAKE:		// Print the resources need to make in nice format.
			{
				pszKey	+= 9;
				if ( *pszKey == '.' )
				{
					bool	fQtyOnly	= false;
					bool	fKeyOnly	= false;
					SKIP_SEPARATORS( pszKey );
					int		index	= Exp_GetVal( pszKey );
					SKIP_SEPARATORS( pszKey );

					if ( !strnicmp( pszKey, "KEY", 3 ))
						fKeyOnly	= true;
					else if ( !strnicmp( pszKey, "VAL", 3 ))
						fQtyOnly	= true;

					TCHAR *pszTmp = Str_GetTemp();
					if ( fKeyOnly || fQtyOnly )
						m_SkillMake.WriteKeys( pszTmp, index, fQtyOnly, fKeyOnly );
					else
						m_SkillMake.WriteNames( pszTmp, index );
					if ( fQtyOnly && pszTmp[0] == '\0' )
						strcpy( pszTmp, "0" );
					sVal = pszTmp;
				}
				else
				{
					TCHAR *pszTmp = Str_GetTemp();
					m_SkillMake.WriteNames( pszTmp );
					sVal = pszTmp;
				}
			}
			break;
		case IBC_RESDISPDNID:
			sVal = g_Cfg.ResourceGetName( RESOURCE_ID( RES_TYPEDEF, GetResDispDnId()));
			break;
		case IBC_RESMAKE:
			// Print the resources need to make in nice format.
			{
				TCHAR *pszTmp = Str_GetTemp();
				m_BaseResources.WriteNames( pszTmp );
				sVal = pszTmp;
			}
			break;
		case IBC_SPEED:
			sVal.FormatVal( m_speed );
			break;
		case IBC_TDATA1:
			sVal.FormatHex( m_ttNormal.m_tData1 );
			break;
		case IBC_TDATA2:
			sVal.FormatHex( m_ttNormal.m_tData2 );
			break;
		case IBC_TDATA3:
			sVal.FormatHex( m_ttNormal.m_tData3 );
			break;
		case IBC_TDATA4:
			sVal.FormatHex( m_ttNormal.m_tData4 );
			break;
		case IBC_TFLAGS:
			sVal.FormatHex( GetTFlags() );
			break;
		case IBC_TWOHANDS:
			if ( ! IsTypeEquippable())
				return( false );

			if ( ! IsTypeWeapon(GetType()) && ! IsType(IT_FISH_POLE))
				sVal.FormatVal(0);
			else
				sVal.FormatVal( m_layer == LAYER_HAND2 );
			break;
		case IBC_TYPE:
			// sVal.FormatVal( m_type );
			{
				RESOURCE_ID	rid( RES_TYPEDEF, m_type );
				CResourceDef *	pRes	= g_Cfg.ResourceGetDef( rid );
				if ( !pRes )
					sVal.FormatVal( m_type );
				else
					sVal = pRes->GetResourceName();
				
			}
			break;
		case IBC_VALUE:
			if ( m_values.GetRange())
				sVal.Format( "%d,%d", GetMakeValue(0), GetMakeValue(100) );
			else
				sVal.Format( "%d", GetMakeValue(0));
			break;
		case IBC_WEIGHT:
			sVal.FormatVal( m_weight / WEIGHT_UNITS );
			break;
		default:
			return( CBaseBaseDef::r_WriteVal( pszKey, sVal ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pChar);
	EXC_DEBUG_END;
	return false;
}

bool CItemBase::r_LoadVal( CScript &s )
{
	ADDTOCALLSTACK("CItemBase::r_LoadVal");
	EXC_TRY("LoadVal");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case IBC_AMMOANIM:
		case IBC_AMMOANIMHUE:
		case IBC_AMMOANIMRENDER:
		case IBC_AMMOCONT:
		case IBC_AMMOTYPE:
			{
				bool fQuoted = false;
				SetDefStr(s.GetKey(), s.GetArgStr( &fQuoted ), fQuoted);
			}
			break;
		case IBC_DISPID:
			// Can't set this.
			return( false );
		case IBC_DUPEITEM:
			// Just ignore these.
			return( true );

		case IBC_DUPELIST:
			{
				TCHAR * ppArgs[512];
				size_t iArgQty = Str_ParseCmds( s.GetArgStr(), ppArgs, COUNTOF(ppArgs));
				if ( iArgQty <= 0 )
					return( false );
				m_flip_id.Empty();
				for ( size_t i = 0; i < iArgQty; i++ )
				{
					ITEMID_TYPE id = static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, ppArgs[i] ));
					if ( ! IsValidDispID( id ))
						continue;
					if ( IsSameDispID(id))
						continue;
					m_flip_id.Add(id);
				}
			}
			break;
		case IBC_DYE:
			if ( ! s.HasArgs())
				m_Can |= CAN_I_DYE;
			else
				m_Can |= ( s.GetArgVal()) ? CAN_I_DYE : 0;
			break;
		case IBC_FLIP:
			if ( ! s.HasArgs())
				m_Can |= CAN_I_FLIP;
			else
				m_Can |= ( s.GetArgVal()) ? CAN_I_FLIP : 0;
			break;
		case IBC_ID:
			{
				if ( GetID() < ITEMID_MULTI )
				{
					DEBUG_ERR(( "Setting new id for base type %s not allowed\n", GetResourceName()));
					return( false );
				}

				ITEMID_TYPE id = static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, s.GetArgStr()));
				if ( ! IsValidDispID(id))
				{
					DEBUG_ERR(( "Setting invalid id=%s for base type %s\n", s.GetArgStr(), GetResourceName()));
					return( false );
				}

				CItemBase * pItemDef = FindItemBase( id );	// make sure the base is loaded.
				if ( ! pItemDef )
				{
					DEBUG_ERR(( "Setting unknown base id=0%x for %s\n", id, GetResourceName()));
					return( false );
				}

				CopyBasic( pItemDef );
				m_dwDispIndex = id;	// Might not be the default of a DUPEITEM
			}
			break;

		case IBC_LAYER:
			m_layer = static_cast<LAYER_TYPE>(s.GetArgVal());
			break;
		case IBC_PILE:
			break;
		case IBC_ENCHANT:
			m_Can |= ( s.GetArgVal()) ? CAN_I_ENCHANT : 0;
			break;
		case IBC_EXCEPTIONAL:
			m_Can |= ( s.GetArgVal()) ? CAN_I_EXCEPTIONAL : 0;
			break;
		case IBC_IMBUE:
			m_Can |= ( s.GetArgVal()) ? CAN_I_IMBUE : 0;
			break;
		case IBC_REFORGE:
			m_Can |= ( s.GetArgVal()) ? CAN_I_REFORGE : 0;
			break;
		case IBC_RETAINCOLOR:
			m_Can |= ( s.GetArgVal()) ? CAN_I_RETAINCOLOR : 0;
			break;
		case IBC_MAKERSMARK:
			m_Can |= ( s.GetArgVal()) ? CAN_I_MAKERSMARK : 0;
			break;
		case IBC_RECYCLE:
			m_Can |= ( s.GetArgVal()) ? CAN_I_RECYCLE : 0;
			break;
		case IBC_REPAIR:
			m_Can |= ( s.GetArgVal()) ? CAN_I_REPAIR : 0;
			break;
		case IBC_REPLICATE:
			m_Can |= ( s.GetArgVal()) ? CAN_I_REPLICATE : 0;
			break;
		case IBC_REQSTR:
			if ( ! IsTypeEquippable())
				return( false );
			m_ttEquippable.m_StrReq = s.GetArgVal();
			break;
		
		case IBC_RESDISPDNID:
			SetResDispDnId(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, s.GetArgStr()));
			break;

		case IBC_SPEED:
			m_speed = s.GetArgVal();
			break;

		case IBC_SKILL:		// Skill to use.
			m_iSkill = g_Cfg.FindSkillKey( s.GetArgStr() );
			break;

		case IBC_SKILLMAKE:
			// Skill required to make this.
			m_SkillMake.Load( s.GetArgStr());
			break;

		case IBC_TDATA1:
			m_ttNormal.m_tData1 = s.GetArgVal();
			break;
		case IBC_TDATA2:
			m_ttNormal.m_tData2 = s.GetArgVal();
			break;
		case IBC_TDATA3:
			m_ttNormal.m_tData3 = s.GetArgVal();
			break;
		case IBC_TDATA4:
			m_ttNormal.m_tData4 = s.GetArgVal();
			break;

		case IBC_TWOHANDS:
			// In some cases the layer is not set right.
			// override the layer here.
			if ( s.GetArgStr()[0] == '1' || s.GetArgStr()[0] == 'Y' || s.GetArgStr()[0] == 'y' )
			{
				m_layer = LAYER_HAND2;
			}
			break;
		case IBC_TYPE:
			m_type = static_cast<IT_TYPE>(g_Cfg.ResourceGetIndexType( RES_TYPEDEF, s.GetArgStr()));
			if ( m_type == IT_CONTAINER_LOCKED )
			{
				// At this level it just means to add a key for it.
				m_type = IT_CONTAINER;
			}
			break;
		case IBC_VALUE:
			m_values.Load( s.GetArgRaw() );
			break;
		case IBC_WEIGHT:
			// Read in the weight but it may not be decimalized correctly
			{
				bool fDecimal = ( strchr( s.GetArgStr(), '.' ) != NULL );
				m_weight = s.GetArgVal();
				if ( ! fDecimal )
				{
					m_weight *= WEIGHT_UNITS;
				}
			}
			break;
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

void CItemBase::ReplaceItemBase( CItemBase * pOld, CResourceDef * pNew ) // static
{
	ADDTOCALLSTACK("CItemBase::ReplaceItemBase");
	ASSERT(pOld);
	ASSERT(pOld->GetRefInstances() == 0);
	RESOURCE_ID rid = pOld->GetResourceID();
	size_t index = g_Cfg.m_ResHash.FindKey(rid);
	ASSERT( index != g_Cfg.m_ResHash.BadIndex() );
	g_Cfg.m_ResHash.SetAt( rid, index, pNew );
}

CItemBase * CItemBase::MakeDupeReplacement( CItemBase * pBase, ITEMID_TYPE idmaster ) // static
{
	ADDTOCALLSTACK("CItemBase::MakeDupeReplacement");
	ITEMID_TYPE id = pBase->GetID();
	if ( idmaster == id || ! IsValidDispID( idmaster ))
	{
		DEBUG_ERR(( "CItemBase:DUPEITEM weirdness 0%x==0%x\n", id, idmaster ));
		return( pBase );
	}

	CItemBase * pBaseNew = FindItemBase( idmaster );
	if ( pBaseNew == NULL )
	{
		DEBUG_ERR(( "CItemBase:DUPEITEM not exist 0%x==0%x\n", id, idmaster ));
		return( pBase );
	}

	if ( pBaseNew->GetID() != idmaster )
	{
		DEBUG_ERR(( "CItemBase:DUPEITEM circle 0%x==0%x\n", id, idmaster ));
		return( pBase );
	}

	if ( ! pBaseNew->IsSameDispID(id))	// already here ?!
	{
		pBaseNew->m_flip_id.Add(id);
	}

	// create the dupe stub.
	CUOItemTypeRec2 tiledata;
	CItemBaseDupe * pBaseDupe = NULL;

	memset( &tiledata, 0, sizeof(tiledata));
	pBaseDupe = new CItemBaseDupe( id, pBaseNew );
	if ( CItemBase::GetItemData( id, &tiledata ) )
	{
		pBaseDupe->SetTFlags( tiledata.m_flags );
		height_t Height = CItemBase::GetItemHeightFlags( tiledata, pBaseDupe->m_Can );
		//Height = ( pBaseDupe->GetTFlags() & 0x400 ) ? ( Height / 2 ) : ( Height ); //should not be done here
		Height = IsID_Chair( id ) ? 0 : Height;
		pBaseDupe->SetHeight( Height );
	}
	ReplaceItemBase( pBase, pBaseDupe );

	return( pBaseNew );
}

//**************************************************
// -CItemBaseMulti

CItemBase * CItemBaseMulti::MakeMultiRegion( CItemBase * pBase, CScript & s ) // static
{
	ADDTOCALLSTACK("CItemBaseMulti::MakeMultiRegion");
	// "MULTIREGION"
	// We must transform this object into a CItemBaseMulti

	ASSERT( pBase );

	if ( ! pBase->IsTypeMulti(pBase->GetType()) )
	{
		DEBUG_ERR(( "MULTIREGION defined for NON-MULTI type 0%x\n", pBase->GetID()));
		return pBase;
	}

	CItemBaseMulti * pMultiBase = dynamic_cast <CItemBaseMulti *>(pBase);
	if ( pMultiBase == NULL )
	{
		if ( pBase->GetRefInstances() > 0 )
		{
			DEBUG_ERR(( "MULTIREGION defined for IN USE NON-MULTI type 0%x\n", pBase->GetID()));
			return pBase;
		}

		pMultiBase = new CItemBaseMulti( pBase );
		ReplaceItemBase( pBase, pMultiBase );
	}

	pMultiBase->SetMultiRegion( s.GetArgStr());
	return( pMultiBase );
}

CItemBaseMulti::CItemBaseMulti( CItemBase* pBase ) :
	CItemBase( pBase->GetID())
{
	m_dwRegionFlags = REGION_FLAG_NODECAY | REGION_ANTIMAGIC_TELEPORT | REGION_ANTIMAGIC_RECALL_IN | REGION_FLAG_NOBUILDING;
	m_rect.SetRectEmpty();
	m_shipSpeed.period = 1 * TICK_PER_SEC;
	m_shipSpeed.tiles = 2;
	// copy the stuff from the pBase
	CopyTransfer(pBase);
}

bool CItemBaseMulti::AddComponent( ITEMID_TYPE id, signed short dx, signed short dy, signed char dz )
{
	ADDTOCALLSTACK("CItemBaseMulti::AddComponent");
	m_rect.UnionPoint( dx, dy );
	if ( id > 0 )
	{
		CItemBase * pItemBase = FindItemBase(id);
		if ( pItemBase == NULL )	// make sure the item is valid
		{
			DEBUG_ERR(( "Bad COMPONENT 0%x\n", id ));
			return false;
		}

		CItemBaseMulti::CMultiComponentItem comp;
		comp.m_id = id;
		comp.m_dx = dx;
		comp.m_dy = dy;
		comp.m_dz = dz;
		m_Components.Add( comp );
	}

	return( true );
}

//	TODO: (Vjaka) do i really need map plane here?
void CItemBaseMulti::SetMultiRegion( TCHAR * pArgs )
{
	ADDTOCALLSTACK("CItemBaseMulti::SetMultiRegion");
	// inclusive region.
	INT64 piArgs[5];
	size_t iQty = Str_ParseCmds( pArgs, piArgs, COUNTOF(piArgs));
	if ( iQty <= 1 )
		return;
	m_Components.Empty();	// might be after a resync
	m_rect.SetRect( piArgs[0], piArgs[1], piArgs[2]+1, piArgs[3]+1, piArgs[4] );
}

bool CItemBaseMulti::AddComponent( TCHAR * pArgs )
{
	ADDTOCALLSTACK("CItemBaseMulti::AddComponent");
	INT64 piArgs[4];
	size_t iQty = Str_ParseCmds( pArgs, piArgs, COUNTOF(piArgs));
	if ( iQty <= 1 )
		return false;
	return AddComponent(static_cast<ITEMID_TYPE>(RES_GET_INDEX(piArgs[0])), piArgs[1], piArgs[2], piArgs[3] );
}

int CItemBaseMulti::GetMaxDist() const
{
	ADDTOCALLSTACK("CItemBaseMulti::GetMaxDist");
	int iDist = abs( m_rect.m_left );
	int iDistTmp = abs( m_rect.m_top );
	if ( iDistTmp > iDist )
		iDist = iDistTmp;
	iDistTmp = abs( m_rect.m_right + 1 );
	if ( iDistTmp > iDist )
		iDist = iDistTmp;
	iDistTmp = abs( m_rect.m_bottom + 1 );
	if ( iDistTmp > iDist )
		iDist = iDistTmp;
	return( iDist+1 );
}

enum MLC_TYPE 
{
	MLC_BASECOMPONENT,
	MLC_COMPONENT,
	MLC_MULTIREGION,
	MLC_REGIONFLAGS,
	MLC_SHIPSPEED,
	MLC_TSPEECH,
	MLC_QTY
};

LPCTSTR const CItemBaseMulti::sm_szLoadKeys[] =
{
	"BASECOMPONENT",
	"COMPONENT",
	"MULTIREGION",
	"REGIONFLAGS",
	"SHIPSPEED",
	"TSPEECH",
	NULL
};

bool CItemBaseMulti::r_LoadVal( CScript &s )
{
	ADDTOCALLSTACK("CItemBaseMulti::r_LoadVal");
	EXC_TRY("LoadVal");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	case MLC_COMPONENT:
		return AddComponent( s.GetArgStr());
	case MLC_MULTIREGION:
		MakeMultiRegion( this, s );
		break;
	case MLC_REGIONFLAGS:
		m_dwRegionFlags = s.GetArgVal();
		return( true );
	case MLC_SHIPSPEED:
	{
		if (!IsType(IT_SHIP))	// only valid for ships
			return false;

		// SHIPSPEED x[,y]
		INT64 ppArgs[2];
		size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs));
		if (iQty < 1)
			return false;

		m_shipSpeed.period = ppArgs[0];

		if (iQty >= 2)
			m_shipSpeed.tiles = ppArgs[1];
	} break;
	case MLC_TSPEECH:
		return( m_Speech.r_LoadVal( s, RES_SPEECH ));
	default:
		return( CItemBase::r_LoadVal( s ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CItemBaseMulti::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pChar )
{
	ADDTOCALLSTACK("CItemBaseMulti::r_WriteVal");
	EXC_TRY("WriteVal");
	switch ( FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ) )
	{
	case MLC_BASECOMPONENT:
	{
		pszKey += 13;
		const CGrayMulti* pMulti = g_Cfg.GetMultiItemDefs(GetDispID());
		if (pMulti == NULL)
			return false;

		if (*pszKey == '\0')
		{
			sVal.FormatVal(pMulti->GetItemCount());
		}
		else if (*pszKey == '.')
		{
			SKIP_SEPARATORS( pszKey );
			size_t index = Exp_GetVal( pszKey );
			if (index >= pMulti->GetItemCount())
				return false;
			SKIP_SEPARATORS( pszKey );
			const CUOMultiItemRec2* item = pMulti->GetItem(index);

			if ( *pszKey == '\0' )						sVal.Format("%u,%i,%i,%i", item->m_wTileID, item->m_dx, item->m_dy, item->m_dz);
			else if ( !strnicmp(pszKey, "ID", 2) )		sVal.FormatVal(item->m_wTileID);
			else if ( !strnicmp(pszKey, "DX", 2) )		sVal.FormatVal(item->m_dx);
			else if ( !strnicmp(pszKey, "DY", 2) )		sVal.FormatVal(item->m_dy);
			else if ( !strnicmp(pszKey, "DZ", 2) )		sVal.FormatVal(item->m_dz);
			else if ( !strnicmp(pszKey, "D", 1) )		sVal.Format("%u,%i,%i", item->m_dx, item->m_dy, item->m_dz);
			else if ( !strnicmp(pszKey, "VISIBLE", 7) )	sVal.FormatVal(item->m_visible);
			else return false;
		}
		else
			return false;

		return true;
	}
	case MLC_COMPONENT:
		{
			pszKey += 9;
			if ( *pszKey == '\0' )
			{
				sVal.FormatVal( m_Components.GetCount() );
			}
			else if ( *pszKey == '.' )
			{
				SKIP_SEPARATORS( pszKey );
				size_t index = Exp_GetVal( pszKey );
				if ( m_Components.IsValidIndex(index) == false )
					return false;

				SKIP_SEPARATORS( pszKey );
				CMultiComponentItem item = m_Components.GetAt( index );

				if ( !strnicmp(pszKey, "ID", 2) ) sVal.FormatVal(item.m_id);
				else if ( !strnicmp(pszKey, "DX", 2) ) sVal.FormatVal(item.m_dx);
				else if ( !strnicmp(pszKey, "DY", 2) ) sVal.FormatVal(item.m_dy);
				else if ( !strnicmp(pszKey, "DZ", 2) ) sVal.FormatVal(item.m_dz);
				else if ( !strnicmp(pszKey, "D", 1) ) sVal.Format("%i,%i,%i", item.m_dx, item.m_dy, item.m_dz);
				else sVal.Format("%u,%i,%i,%i", item.m_id, item.m_dx, item.m_dy, item.m_dz);
			}
			else
				return false;
			return true;
		}
	case MLC_MULTIREGION:
		sVal.Format( "%d,%d,%d,%d", m_rect.m_left, m_rect.m_top, m_rect.m_right-1, m_rect.m_bottom-1 );
		return( true );
	case MLC_REGIONFLAGS:
		sVal.FormatHex( m_dwRegionFlags );
		return( true );
	case MLC_SHIPSPEED:
	{
		if (!IsType(IT_SHIP))
			return( false );

		pszKey += 9;
		if (*pszKey == '.')
		{
			pszKey++;
			if ( !strcmpi(pszKey, "TILES"))
			{
				sVal.FormatVal(m_shipSpeed.tiles);
				break;
			}
			else if (!strcmpi(pszKey, "PERIOD"))
			{
				sVal.FormatVal(m_shipSpeed.period);
				break;
			}
			return false;
		}

		sVal.Format("%d,%d", m_shipSpeed.period, m_shipSpeed.tiles);
		break;
	}
	default:
		return( CItemBase::r_WriteVal( pszKey, sVal, pChar ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pChar);
	EXC_DEBUG_END;
	return false;
}

//**************************************************

CItemBase * CItemBase::FindItemBase( ITEMID_TYPE id ) // static
{
	ADDTOCALLSTACK("CItemBase::FindItemBase");
	// CItemBase
	// is a like item already loaded.

	if ( id <= 0 )
	{
		return( NULL );
	}

	RESOURCE_ID rid = RESOURCE_ID( RES_ITEMDEF, id );
	size_t index = g_Cfg.m_ResHash.FindKey(rid);
	if ( index == g_Cfg.m_ResHash.BadIndex() )
	{
		return( NULL );
	}

	CResourceDef * pBaseStub = g_Cfg.m_ResHash.GetAt( rid, index );
	ASSERT(pBaseStub);

	CItemBase * pBase = dynamic_cast <CItemBase *>(pBaseStub);
	if ( pBase )
	{
		return( pBase );	// already loaded all base info.
	}

	const CItemBaseDupe * pBaseDupe = dynamic_cast <const CItemBaseDupe *>(pBaseStub);
	if ( pBaseDupe )
	{
		return( pBaseDupe->GetItemDef() );	// this is just a dupeitem
	}

	CResourceLink * pBaseLink = dynamic_cast <CResourceLink *>(pBaseStub);
	ASSERT(pBaseLink);

	pBase = new CItemBase( id );
	pBase->CResourceLink::CopyTransfer( pBaseLink );
	g_Cfg.m_ResHash.SetAt( rid, index, pBase );	// Replace with new in sorted order.

	// Find the previous one in the series if any.
	// Find it's script section offset.

	CResourceLock s;
	if ( ! pBase->ResourceLock(s))
	{
		// must be scripted. not in the artwork set.
		g_Log.Event( LOGL_ERROR, "UN-scripted item 0%0x NOT allowed.\n", id );
		return( NULL );
	}

	// Scan the item definition for keywords such as DUPEITEM and
	// MULTIREGION, as these will adjust how our definition is processed
	CScriptLineContext scriptStartContext = s.GetContext();
	while ( s.ReadKeyParse())
	{
		if ( s.IsKey( "DUPEITEM" ))
		{
			return( MakeDupeReplacement( pBase, static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, s.GetArgStr()))));
		}
		if ( s.IsKey( "MULTIREGION" ))
		{
			// Upgrade the CItemBase::pBase to the CItemBaseMulti.
			pBase = CItemBaseMulti::MakeMultiRegion( pBase, s );
			continue;
		}
		if ( s.IsKeyHead( "ON", 2 ))	// trigger scripting marks the end
		{
			break;
		}
		if ( s.IsKey( "ID" ) || s.IsKey( "TYPE" ))	// These are required for CItemBaseMulti::MakeMultiRegion to function correctly
		{
			pBase->r_LoadVal( s );
		}
	}

	// Return to the start of the item script
	s.SeekContext(scriptStartContext);

	// Read the Script file preliminary.
	while ( s.ReadKeyParse())
	{
		if ( s.IsKey( "DUPEITEM" ) || s.IsKey( "MULTIREGION" ))
			continue;
		if ( s.IsKeyHead( "ON", 2 ))	// trigger scripting marks the end
			break;

		pBase->r_LoadVal( s );
	}

	return( pBase );
}

//**************************************************
CItemBaseDupe * CItemBaseDupe::GetDupeRef( ITEMID_TYPE id ) // static
{
	ADDTOCALLSTACK("CItemBaseDupe::GetDupeRef");
	if ( id <= 0 )
		return( NULL );

	RESOURCE_ID rid = RESOURCE_ID( RES_ITEMDEF, id );
	size_t index = g_Cfg.m_ResHash.FindKey(rid);
	if ( index == g_Cfg.m_ResHash.BadIndex() )
		return( NULL );

	CResourceDef * pBaseStub = g_Cfg.m_ResHash.GetAt( rid, index );

	CItemBase * pBase = dynamic_cast <CItemBase *>(pBaseStub);
	if ( pBase )
		return( NULL ); //We want to return Dupeitem, not Baseitem

	CItemBaseDupe * pBaseDupe = dynamic_cast <CItemBaseDupe *>(pBaseStub);
	if ( pBaseDupe )
		return( pBaseDupe );	// this is just a dupeitem

	return( NULL); //we suspect item is loaded
}
