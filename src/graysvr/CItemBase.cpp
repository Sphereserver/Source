#include "graysvr.h"	// predef header.

//*********************************************************
// CItemBase

CItemBase::CItemBase(ITEMID_TYPE id) : CBaseBaseDef(RESOURCE_ID(RES_ITEMDEF, id))
{
	m_type = IT_NORMAL;
	m_weight = 0;
	m_layer = LAYER_NONE;
	m_uiFlags = 0;
	m_speed = 0;
	m_iSkill = SKILL_NONE;
	m_CanUse = CAN_U_ALL;
	SetDefNum("RANGE", 1);	//m_range = 1;

	m_StrengthBonus = 0;
	m_DexterityBonus = 0;
	m_IntelligenceBonus = 0;
	m_HitpointIncrease = 0;
	m_StaminaIncrease = 0;
	m_ManaIncrease = 0;
	m_MageArmor = 0;
	m_MageWeapon = 0;
	m_ArtifactRarity = 0;
	m_SelfRepair = 0;
	m_SpellChanneling = 0;
	m_LowerRequirements = 0;
	m_UseBestWeaponSkill = 0;
	m_HitPhysicalArea = 0;
	m_HitFireArea = 0;
	m_HitColdArea = 0;
	m_HitPoisonArea = 0;
	m_HitEnergyArea = 0;
	m_HitDispel = 0;
	m_HitFireball = 0;
	m_HitHarm = 0;
	m_HitLightning = 0;
	m_HitMagicArrow = 0;
	m_WeightReduction = 0;

	m_ttNormal.m_tData1 = 0;
	m_ttNormal.m_tData2 = 0;
	m_ttNormal.m_tData3 = 0;
	m_ttNormal.m_tData4 = 0;

	if ( !IsValidDispID(id) )
	{
		// There should be an ID= in scripts later
		m_dwDispIndex = ITEMID_GOLD_C1;
		return;
	}
	m_dwDispIndex = id;

	// I have it indexed but it needs to be loaded. Read it from the script and *.mul files
	CUOItemTypeRec2 tiledata;
	memset(&tiledata, 0, sizeof(tiledata));
	if ( id < ITEMID_MULTI )
		static_cast<void>(GetItemData(id, &tiledata));
	else
		tiledata.m_weight = BYTE_MAX;

	m_uiFlags = tiledata.m_flags;
	m_type = GetTypeBase(id, tiledata);

	TCHAR szName[sizeof(tiledata.m_name) + 1];
	size_t j = 0;
	for ( size_t i = 0; i < sizeof(tiledata.m_name) && tiledata.m_name[i]; i++ )
	{
		if ( (j == 0) && ISWHITESPACE(tiledata.m_name[i]) )
			continue;
		szName[j++] = tiledata.m_name[i];
	}
	szName[j] = '\0';
	m_sName = szName;

	SetHeight(GetItemHeightFlags(tiledata, m_Can));
	GetItemSpecificFlags(tiledata, m_Can, m_type, id);

	if ( (tiledata.m_weight == BYTE_MAX) || (tiledata.m_flags & UFLAG1_WATER) )	// not movable
		m_weight = WORD_MAX;
	else
		m_weight = tiledata.m_weight * WEIGHT_UNITS;

	if ( tiledata.m_flags & (UFLAG1_EQUIP|UFLAG3_EQUIP2) )
	{
		m_layer = tiledata.m_layer;
		if ( m_layer && !IsMovable() )
			m_weight = WEIGHT_UNITS;	// how I'm supposed to equip something I can't pick up?
	}

	SetResDispDnId(ITEMID_GOLD_C1);
}

CItemBase *CItemBase::FindItemBase(ITEMID_TYPE id)	// static
{
	ADDTOCALLSTACK_INTENSIVE("CItemBase::FindItemBase");
	if ( id <= ITEMID_NOTHING )
		return NULL;

	RESOURCE_ID rid = RESOURCE_ID(RES_ITEMDEF, id);
	size_t index = g_Cfg.m_ResHash.FindKey(rid);
	if ( index == g_Cfg.m_ResHash.BadIndex() )
		return NULL;

	CResourceDef *pBaseStub = g_Cfg.m_ResHash.GetAt(rid, index);
	ASSERT(pBaseStub);

	CItemBase *pBase = dynamic_cast<CItemBase *>(pBaseStub);
	if ( pBase )
		return pBase;	// already loaded all base info

	const CItemBaseDupe *pBaseDupe = dynamic_cast<const CItemBaseDupe *>(pBaseStub);
	if ( pBaseDupe )
		return pBaseDupe->GetItemDef();		// this is just a DupeItem

	CResourceLink *pBaseLink = static_cast<CResourceLink *>(pBaseStub);
	ASSERT(pBaseLink);

	pBase = new CItemBase(id);
	pBase->CResourceLink::CopyTransfer(pBaseLink);
	g_Cfg.m_ResHash.SetAt(rid, index, pBase);	// replace with new in sorted order

	// Find the previous one in the series if any
	// Find it's script section offset
	CResourceLock s;
	if ( !pBase->ResourceLock(s) )
	{
		g_Log.Event(LOGL_ERROR, "Un-scripted item 0%x not allowed\n", id);
		return NULL;
	}

	// Scan the item definition for keywords such as DUPEITEM and MULTIREGION, as these will adjust how our definition is processed
	CScriptLineContext scriptStartContext = s.GetContext();
	while ( s.ReadKeyParse() )
	{
		if ( s.IsKey("DUPEITEM") )
			return MakeDupeReplacement(pBase, static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType(RES_ITEMDEF, s.GetArgStr())));
		if ( s.IsKey("MULTIREGION") )
		{
			pBase = CItemBaseMulti::MakeMultiRegion(pBase, s);	// change CItemBase::pBase to CItemBaseMulti
			continue;
		}
		if ( s.IsKeyHead("ON", 2) )		// trigger scripting marks the end
			break;
		if ( s.IsKey("ID") || s.IsKey("TYPE") )		// these are required to make CItemBaseMulti::MakeMultiRegion work properly
			pBase->r_LoadVal(s);
	}

	// Return to the start of the item script
	s.SeekContext(scriptStartContext);

	// Read the script file preliminary
	while ( s.ReadKeyParse() )
	{
		if ( s.IsKey("DUPEITEM") || s.IsKey("MULTIREGION") )
			continue;
		if ( s.IsKeyHead("ON", 2) )		// trigger scripting marks the end
			break;

		pBase->r_LoadVal(s);
	}
	return pBase;
}

void CItemBase::CopyBasic(const CItemBase *pItemDef)
{
	ADDTOCALLSTACK("CItemBase::CopyBasic");
	m_speed = pItemDef->m_speed;
	m_weight = pItemDef->m_weight;
	m_flip_id = pItemDef->m_flip_id;
	m_type = pItemDef->m_type;
	m_layer = pItemDef->m_layer;
	SetDefNum("RANGE", pItemDef->GetDefNum("RANGE"));	// m_range = pBase->m_range;

	m_ttNormal.m_tData1 = pItemDef->m_ttNormal.m_tData1;
	m_ttNormal.m_tData2 = pItemDef->m_ttNormal.m_tData2;
	m_ttNormal.m_tData3 = pItemDef->m_ttNormal.m_tData3;
	m_ttNormal.m_tData4 = pItemDef->m_ttNormal.m_tData4;

	CBaseBaseDef::CopyBasic(pItemDef);		// this will overwrite CResourceLink
}

void CItemBase::CopyTransfer(CItemBase *pItemDef)
{
	ADDTOCALLSTACK("CItemBase::CopyTransfer");
	CopyBasic(pItemDef);

	m_values = pItemDef->m_values;
	m_SkillMake = pItemDef->m_SkillMake;

	CBaseBaseDef::CopyTransfer(pItemDef);	// this will overwrite the CResourceLink
}

void CItemBase::SetTypeName(LPCTSTR pszName)
{
	ADDTOCALLSTACK("CItemBase::SetTypeName");
	ASSERT(pszName);
	if ( !strcmp(pszName, GetTypeName()) )
		return;
	m_uiFlags |= UFLAG2_ZERO1;	// we override the name
	CBaseBaseDef::SetTypeName(pszName);
}

LPCTSTR CItemBase::GetArticleAndSpace() const
{
	ADDTOCALLSTACK("CItemBase::GetArticleAndSpace");
	if ( IsSetOF(OF_NoPrefix) )
		return "";
	if ( m_uiFlags & UFLAG2_ZERO1 )	// name has been changed from tiledata.mul
		return Str_GetArticleAndSpace(GetTypeName());
	if ( m_uiFlags & UFLAG2_AN )
		return "an ";
	if ( m_uiFlags & UFLAG2_A )
		return "a ";
	return "";
}

TCHAR *CItemBase::GetNamePluralize(LPCTSTR pszNameBase, bool fPluralize)	// static
{
	ADDTOCALLSTACK("CItemBase::GetNamePluralize");
	// Get rid of the strange %s type stuff for pluralize rules of names

	TCHAR *pszName = Str_GetTemp();
	size_t j = 0;
	bool fInside = false;
	bool fPlural = false;
	for ( size_t i = 0; pszNameBase[i]; i++ )
	{
		if ( pszNameBase[i] == '%' )
		{
			fInside = !fInside;
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
				if ( !fPlural )
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
	return pszName;
}

CREID_TYPE CItemBase::FindCharTrack(ITEMID_TYPE id)	// static
{
	ADDTOCALLSTACK("CItemBase::FindCharTrack");
	// For figurines. Convert to a creature
	// IT_FIGURINE
	// IT_EQ_HORSE

	CItemBase *pItemDef = FindItemBase(id);
	if ( pItemDef && (pItemDef->IsType(IT_FIGURINE) || pItemDef->IsType(IT_EQ_HORSE)) )
		return static_cast<CREID_TYPE>(pItemDef->m_ttFigurine.m_charid.GetResIndex());
	return CREID_INVALID;
}

bool CItemBase::IsTypeArmor(IT_TYPE type)  // static
{
	ADDTOCALLSTACK("CItemBase::IsTypeArmor");
	switch ( type )
	{
		case IT_CLOTHING:
		case IT_ARMOR:
		case IT_ARMOR_LEATHER:
		case IT_SHIELD:
			return true;
		default:
			return false;
	}
}
bool CItemBase::IsTypeWeapon(IT_TYPE type)  // static
{
	ADDTOCALLSTACK("CItemBase::IsTypeWeapon");
	switch ( type )
	{
		case IT_WEAPON_MACE_STAFF:
		case IT_WEAPON_MACE_CROOK:
		case IT_WEAPON_MACE_PICK:
		case IT_WEAPON_AXE:
		case IT_WEAPON_XBOW:
		case IT_WEAPON_THROWING:
		case IT_WEAPON_MACE_SMITH:
		case IT_WEAPON_MACE_SHARP:
		case IT_WEAPON_SWORD:
		case IT_WEAPON_FENCE:
		case IT_WEAPON_BOW:
		case IT_WEAPON_WHIP:
		case IT_WAND:
			return true;
		default:
			return false;
	}
}

bool CItemBase::IsTypeSpellbook(IT_TYPE type)  // static
{
	ADDTOCALLSTACK("CItemBase::IsTypeSpellbook");
	switch ( type )
	{
		case IT_SPELLBOOK:
		case IT_SPELLBOOK_NECRO:
		case IT_SPELLBOOK_PALA:
		case IT_SPELLBOOK_EXTRA:
		case IT_SPELLBOOK_BUSHIDO:
		case IT_SPELLBOOK_NINJITSU:
		case IT_SPELLBOOK_ARCANIST:
		case IT_SPELLBOOK_MYSTIC:
		case IT_SPELLBOOK_MASTERY:
			return true;
		default:
			return false;
	}
}

bool CItemBase::IsTypeMulti(IT_TYPE type)	// static
{
	ADDTOCALLSTACK("CItemBase::IsTypeMulti");
	switch ( type )
	{
		case IT_MULTI:
		case IT_MULTI_CUSTOM:
		case IT_SHIP:
			return true;
		default:
			return false;
	}
}

bool CItemBase::IsTypeEquippable() const
{
	ADDTOCALLSTACK("CItemBase::IsTypeEquippable");
	// Check visible layers
	switch ( m_type )
	{
		case IT_LIGHT_LIT:
		case IT_LIGHT_OUT:
		case IT_HAIR:
		case IT_BEARD:
		case IT_EQ_HORSE:
		case IT_FISH_POLE:
		case IT_JEWELRY:
			return true;
		default:
			break;
	}
	if ( IsTypeArmor(m_type) )
		return true;
	if ( IsTypeWeapon(m_type) )
		return true;
	if ( IsTypeSpellbook(m_type) )
		return true;

	// Check not visible layers
	switch ( m_type )
	{
		case IT_EQ_TRADE_WINDOW:
		case IT_EQ_MEMORY_OBJ:
		case IT_EQ_CLIENT_LINGER:
		case IT_EQ_VENDOR_BOX:
		case IT_EQ_BANK_BOX:
		case IT_EQ_MURDER_COUNT:
		case IT_EQ_STUCK:
		case IT_EQ_SCRIPT:
			return !IsVisibleLayer(static_cast<LAYER_TYPE>(m_layer));
		default:
			break;
	}
	return false;
}

int CItemBase::IsID_Door(ITEMID_TYPE id)	// static
{
	ADDTOCALLSTACK("CItemBase::IsID_Door");
	// IT_DOOR
	// IT_DOOR_LOCKED
	static const ITEMID_TYPE sm_Item_DoorBase[] =
	{
		ITEMID_DOOR_SECRET_STONE_1,
		ITEMID_DOOR_SECRET_STONE_2,
		ITEMID_DOOR_SECRET_STONE_3,
		ITEMID_DOOR_SECRET_WOOD_1,
		ITEMID_DOOR_SECRET_WOOD_2,
		ITEMID_DOOR_SECRET_STONE_4,
		ITEMID_DOOR_METAL_1,
		ITEMID_DOOR_METAL_BARRED_1,
		ITEMID_DOOR_RATTAN,
		ITEMID_DOOR_WOODEN_1,
		ITEMID_DOOR_WOODEN_2,
		ITEMID_DOOR_METAL_2,
		ITEMID_DOOR_WOODEN_3,
		ITEMID_DOOR_WOODEN_4,
		ITEMID_GATE_IRON_1,
		ITEMID_GATE_WOODEN_1,
		ITEMID_GATE_IRON_2,
		ITEMID_GATE_WOODEN_2,
		ITEMID_DOOR_METAL_BARRED_2
	};

	for ( size_t i = 0; i < COUNTOF(sm_Item_DoorBase); ++i )
	{
		int iDoorId = id - sm_Item_DoorBase[i];
		if ( (iDoorId >= 0) && (iDoorId <= 15) )
			return iDoorId + 1;
	}
	return 0;
}

bool CItemBase::IsID_DoorOpen(ITEMID_TYPE id)	// static
{
	ADDTOCALLSTACK("CItemBase::IsID_DoorOpen");
	int iDoorDir = IsID_Door(id) - 1;
	if ( iDoorDir < 0 )
		return false;
	if ( iDoorDir & DOOR_OPENED )
		return true;
	return false;
}

bool CItemBase::GetItemData(ITEMID_TYPE id, CUOItemTypeRec2 *pTiledata)	// static
{
	ADDTOCALLSTACK("CItemBase::GetItemData");
	// Read from g_Install.m_fTileData
	// Get an Item tiledata def data

	if ( !IsValidDispID(id) )
		return false;

	try
	{
		CGrayItemInfo info(id);
		*pTiledata = *(static_cast<CUOItemTypeRec2 *>(&info));
	}
	catch ( const CGrayError &e )
	{
		g_Log.CatchEvent(&e, "GetItemData");
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		return false;
	}
	catch ( ... )
	{
		g_Log.CatchEvent(NULL, "GetItemData");
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		return false;
	}

	// Unused tiledata I guess. Don't create it
	if ( !pTiledata->m_flags && !pTiledata->m_weight && !pTiledata->m_layer && !pTiledata->m_dwUnk11 && !pTiledata->m_wAnim && !pTiledata->m_wHue && !pTiledata->m_wLight && !pTiledata->m_height && !pTiledata->m_name[0] )
		return ((id == ITEMID_BBOARD_MSG) || IsID_GamePiece(id) || IsID_Track(id));		// what are the exceptions to the rule?

	return true;
}

inline void CItemBase::GetItemSpecificFlags(const CUOItemTypeRec2 &tiledata, DWORD &dwBlockThis, IT_TYPE type, ITEMID_TYPE id)	// static
{
	ADDTOCALLSTACK("CItemBase::GetItemSpecificFlags");
	if ( type == IT_DOOR )
	{
		dwBlockThis &= ~CAN_I_BLOCK;
		if ( IsID_DoorOpen(id) )
			dwBlockThis &= ~CAN_I_DOOR;
		else
			dwBlockThis |= CAN_I_DOOR;
	}

	if ( (tiledata.m_flags & UFLAG2_STACKABLE) || (type == IT_REAGENT) || (id == ITEMID_EMPTY_BOTTLE) )
		dwBlockThis |= CAN_I_PILE;
	if ( tiledata.m_flags & UFLAG3_LIGHT )
		dwBlockThis |= CAN_I_LIGHT;
}

void CItemBase::GetItemTiledataFlags(DWORD &dwBlockThis, ITEMID_TYPE id)	// static
{
	ADDTOCALLSTACK("CItemBase::GetItemTiledataFlags");

	CUOItemTypeRec2 tiledata;
	memset(&tiledata, 0, sizeof(tiledata));
	if ( !GetItemData(id, &tiledata) )
	{
		dwBlockThis = 0;
		return;
	}

	if ( tiledata.m_flags & UFLAG4_DOOR )
		dwBlockThis |= CAN_I_DOOR;
	if ( tiledata.m_flags & UFLAG1_WATER )
		dwBlockThis |= CAN_I_WATER;
	if ( tiledata.m_flags & UFLAG2_PLATFORM )
		dwBlockThis |= CAN_I_PLATFORM;
	if ( tiledata.m_flags & UFLAG1_BLOCK )
		dwBlockThis |= CAN_I_BLOCK;
	if ( tiledata.m_flags & UFLAG2_CLIMBABLE )
		dwBlockThis |= CAN_I_CLIMB;
	if ( tiledata.m_flags & UFLAG1_DAMAGE )
		dwBlockThis |= CAN_I_FIRE;
	if ( tiledata.m_flags & UFLAG4_ROOF )
		dwBlockThis |= CAN_I_ROOF;
	if ( tiledata.m_flags & UFLAG4_HOVEROVER )
		dwBlockThis |= CAN_I_HOVER;
}

inline height_t CItemBase::GetItemHeightFlags(const CUOItemTypeRec2 &tiledata, DWORD &dwBlockThis)	// static
{
	ADDTOCALLSTACK("CItemBase::GetItemHeightFlags");

	if ( tiledata.m_flags & UFLAG4_DOOR )
	{
		dwBlockThis = CAN_I_DOOR;
		return tiledata.m_height;
	}

	if ( tiledata.m_flags & UFLAG1_BLOCK )
	{
		if ( tiledata.m_flags & UFLAG1_WATER )
		{
			dwBlockThis = CAN_I_WATER;
			return tiledata.m_height;
		}
		dwBlockThis = CAN_I_BLOCK;
	}
	else
	{
		dwBlockThis = 0;
		if ( !(tiledata.m_flags & (UFLAG2_PLATFORM|UFLAG4_ROOF|UFLAG4_HOVEROVER)) )
			return 0;	// have no effective height if it doesn't block
	}

	if ( tiledata.m_flags & UFLAG4_ROOF )
		dwBlockThis |= CAN_I_ROOF;
	else if ( tiledata.m_flags & UFLAG2_PLATFORM )
		dwBlockThis |= CAN_I_PLATFORM;

	if ( tiledata.m_flags & UFLAG2_CLIMBABLE )
		dwBlockThis |= CAN_I_CLIMB;		// actual standing height is height/2
	if ( tiledata.m_flags & UFLAG4_HOVEROVER )
		dwBlockThis |= CAN_I_HOVER;

	return tiledata.m_height;
}

height_t CItemBase::GetItemHeight(ITEMID_TYPE id, DWORD &dwBlockThis)	// static
{
	ADDTOCALLSTACK("CItemBase::GetItemHeight");
	// Get just the height and the blocking flags for the item by id
	// Used for walk block checking

	RESOURCE_ID rid = RESOURCE_ID(RES_ITEMDEF, id);
	size_t index = g_Cfg.m_ResHash.FindKey(rid);
	if ( index != g_Cfg.m_ResHash.BadIndex() )	// already loaded
	{
		CResourceDef *pBaseStub = g_Cfg.m_ResHash.GetAt(rid, index);
		ASSERT(pBaseStub);
		CItemBase *pBase = dynamic_cast<CItemBase *>(pBaseStub);
		if ( pBase )
		{
			dwBlockThis = pBase->m_Can & CAN_I_MOVEMASK;
			return pBase->GetHeight();
		}
	}

	// Not already loaded
	CUOItemTypeRec2 tiledata;
	if ( !GetItemData(id, &tiledata) )
	{
		dwBlockThis = CAN_I_MOVEMASK;
		return UO_SIZE_Z;
	}

	return GetItemHeightFlags(tiledata, dwBlockThis);
}

IT_TYPE CItemBase::GetTypeBase(ITEMID_TYPE id, const CUOItemTypeRec2 &tiledata)	// static
{
	ADDTOCALLSTACK("CItemBase::GetTypeBase");
	if ( IsID_Ship(id) )
		return IT_SHIP;
	if ( IsID_Multi(id) )
		return IT_MULTI;
	if ( (tiledata.m_flags & UFLAG1_BLOCK) && (tiledata.m_flags & UFLAG1_WATER) )
		return IT_WATER;
	if ( IsID_WaterFish(id) )	// UFLAG1_WATER
		return IT_WATER;
	if ( (tiledata.m_flags & UFLAG4_DOOR) || IsID_Door(id) )
	{
		if ( IsID_DoorOpen(id) )
			return IT_DOOR_OPEN;
		return IT_DOOR;
	}
	if ( tiledata.m_flags & UFLAG3_CONTAINER )
		return IT_CONTAINER;
	if ( IsID_WaterWash(id) )
		return IT_WATER_WASH;
	else if ( IsID_Track(id) )
		return IT_FIGURINE;
	else if ( IsID_GamePiece(id) )
		return IT_GAME_PIECE;

	// Get rid of the stuff below here
	if ( (tiledata.m_flags & UFLAG1_DAMAGE) && !(tiledata.m_flags & UFLAG1_BLOCK) )
		return IT_TRAP_ACTIVE;
	if ( tiledata.m_flags & UFLAG3_LIGHT )	// this may actually be a moongate or fire?
		return IT_LIGHT_LIT;

	return IT_NORMAL;
}

GUMP_TYPE CItemBase::GetContainerGumpID() const
{
	ADDTOCALLSTACK("CItemBase::GetContainerGumpID");
	switch ( m_type )
	{
		case IT_CONTAINER:
		case IT_CONTAINER_LOCKED:
		case IT_SIGN_GUMP:
		case IT_GAME_BOARD:
		case IT_TRASH_CAN:
		case IT_BBOARD:
		case IT_CORPSE:
		case IT_EQ_VENDOR_BOX:
		case IT_EQ_BANK_BOX:
		case IT_KEYRING:
		case IT_SHIP_HOLD:
		case IT_SHIP_HOLD_LOCK:
			return m_ttContainer.m_gumpid;
		default:
			return GUMP_NONE;
	}
}

ITEMID_TYPE CItemBase::GetNextFlipID(ITEMID_TYPE id) const
{
	ADDTOCALLSTACK("CItemBase::GetNextFlipID");
	if ( m_flip_id.GetCount() > 0 )
	{
		ITEMID_TYPE idprev = GetDispID();
		for ( size_t i = 0; i < m_flip_id.GetCount(); i++ )
		{
			ITEMID_TYPE idnext = m_flip_id[i];
			if ( idprev == id )
				return idnext;
			idprev = idnext;
		}
	}
	return GetDispID();
}

bool CItemBase::IsSameDispID(ITEMID_TYPE id) const
{
	ADDTOCALLSTACK("CItemBase::IsSameDispID");
	// Check if item dispid is the same as the requested

	if ( GetDispID() == id )
		return true;

	for ( size_t i = 0; i < m_flip_id.GetCount(); i++ )
	{
		if ( m_flip_id[i] == id )
			return true;
	}
	return false;
}

void CItemBase::Restock()
{
	ADDTOCALLSTACK("CItemBase::Restock");
	// Re-evaluate the base random value rate some time in the future
	if ( (m_values.m_iLo < 0) || (m_values.m_iHi < 0) )
		m_values.Init();
}

DWORD CItemBase::CalculateMakeValue(int iQualityLevel) const
{
	ADDTOCALLSTACK("CItemBase::CalculateMakeValue");
	// Calculate the value in gold for this item based on its components
	// NOTE: Watch out for circular RESOURCES= list in the scripts
	// ARGS:
	//  iQualityLevel = 0-100

	static int sm_iReentrantCount = 0;
	if ( sm_iReentrantCount > 32 )
	{
		DEBUG_ERR(("Too many RESOURCES at item '%s' to calculate a value with (circular resource list?)\n", GetResourceName()));
		return 0;
	}

	sm_iReentrantCount++;
	DWORD dwValue = 0;

	// Add value based on base resources
	for ( size_t i = 0; i < m_BaseResources.GetCount(); i++ )
	{
		RESOURCE_ID rid = m_BaseResources[i].GetResourceID();
		if ( rid.GetResType() != RES_ITEMDEF )
			continue;

		CItemBase *pItemDef = FindItemBase(static_cast<ITEMID_TYPE>(rid.GetResIndex()));
		if ( !pItemDef )
			continue;

		dwValue += pItemDef->GetMakeValue(iQualityLevel) * static_cast<DWORD>(m_BaseResources[i].GetResQty());
	}

	// Add some value based on the skill required to craft it
	for ( size_t i = 0; i < m_SkillMake.GetCount(); i++ )
	{
		RESOURCE_ID rid = m_SkillMake[i].GetResourceID();
		if ( rid.GetResType() != RES_SKILL )
			continue;
		const CSkillDef *pSkillDef = g_Cfg.GetSkillDef(static_cast<SKILL_TYPE>(rid.GetResIndex()));
		if ( !pSkillDef )
			continue;

		// This is the normal skill required. If iQuality is much less than iSkillReq then something is wrong
		int iSkillReq = m_SkillMake[i].GetResQty();
		if ( iQualityLevel < iSkillReq )
			iQualityLevel = iSkillReq;

		dwValue += pSkillDef->m_Values.GetLinear(iQualityLevel);
	}

	sm_iReentrantCount--;
	return dwValue;
}

BYTE CItemBase::GetSpeed() const
{
	ADDTOCALLSTACK("CItemBase::GetSpeed");
	BYTE iSpeed = static_cast<BYTE>(m_TagDefs.GetKeyNum("OVERRIDE.SPEED"));
	return iSpeed ? iSpeed : m_speed;
}

WORD CItemBase::GetMaxAmount()
{
	ADDTOCALLSTACK("CItemBase::GetMaxAmount");
	if ( !IsStackableType() )
		return 0;

	WORD wMax = static_cast<WORD>(GetDefNum("MaxAmount"));
	return wMax ? wMax : g_Cfg.m_iItemsMaxAmount;
};

bool CItemBase::SetMaxAmount(WORD wAmount)
{
	ADDTOCALLSTACK("CItemBase::SetMaxAmount");
	if ( !IsStackableType() )
		return false;

	SetDefNum("MaxAmount", wAmount, false);
	return true;
}

DWORD CItemBase::GetMakeValue(int iQualityLevel)
{
	ADDTOCALLSTACK("CItemBase::GetMakeValue");
	// Set the items value based on the resources and skill used to craft it
	// ARGS:
	//  iQualityLevel = 0-100

	CValueRangeDef values = m_values;
	if ( (m_values.m_iLo == LLONG_MIN) || (m_values.m_iHi == LLONG_MIN) )
	{
		values.m_iLo = CalculateMakeValue(0);		// low quality specimen
		m_values.m_iLo = -values.m_iLo;				// negative means they will float
		values.m_iHi = CalculateMakeValue(100);		// top quality specimen
		m_values.m_iHi = -values.m_iHi;
	}
	else
	{
		values.m_iLo = llabs(values.m_iLo);
		values.m_iHi = llabs(values.m_iHi);
	}
	return static_cast<DWORD>(values.GetLinear(iQualityLevel * 10));
}

void CItemBase::ResetMakeValue()
{
	ADDTOCALLSTACK("CItemBase::ResetMakeValue");
	m_values.Init();
	GetMakeValue(0);
}

enum IBC_TYPE
{
	#define ADD(a,b) IBC_##a,
	#include "../tables/CItemBase_props.tbl"
	#undef ADD
	IBC_QTY
};

LPCTSTR const CItemBase::sm_szLoadKeys[IBC_QTY + 1] =
{
	#define ADD(a,b) b,
	#include "../tables/CItemBase_props.tbl"
	#undef ADD
	NULL
};

bool CItemBase::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CItemBase::r_WriteVal");
	EXC_TRY("WriteVal");
	switch ( FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		// Return as string or hex number (NULL if not set)
		case IBC_AMMOANIM:
		case IBC_AMMOANIMHUE:
		case IBC_AMMOANIMRENDER:
		case IBC_AMMOCONT:
		case IBC_AMMOTYPE:
		case IBC_AMMOSOUNDHIT:
		case IBC_AMMOSOUNDMISS:
		case IBC_DROPSOUND:
		case IBC_EQUIPSOUND:
		case IBC_BONUSSKILL1:
		case IBC_BONUSSKILL2:
		case IBC_BONUSSKILL3:
		case IBC_BONUSSKILL4:
		case IBC_BONUSSKILL5:
		case IBC_OCOLOR:
			sVal = GetDefStr(pszKey);
			break;
		// Return as decimal number (0 if not set)
		case IBC_BONUSSKILL1AMT:
		case IBC_BONUSSKILL2AMT:
		case IBC_BONUSSKILL3AMT:
		case IBC_BONUSSKILL4AMT:
		case IBC_BONUSSKILL5AMT:
		case IBC_LIFESPAN:
		case IBC_USESCUR:
		case IBC_USESMAX:
		case IBC_BONUSHITSMAX:
		case IBC_BONUSSTAMMAX:
		case IBC_BONUSMANAMAX:
			sVal.FormatLLVal(GetDefNum(pszKey));
			break;
		case IBC_MAXAMOUNT:
			sVal.FormatVal(GetMaxAmount());
			break;
		case IBC_SPEEDMODE:
		{
			if ( !IsType(IT_SHIP) )
				return false;
			CItemBaseMulti *pItemMulti = dynamic_cast<CItemBaseMulti *>(this);
			if ( !pItemMulti )
				return false;

			sVal.FormatVal(pItemMulti->m_SpeedMode);
			break;
		}
		case IBC_SHIPSPEED:
		{
			if ( !IsType(IT_SHIP) )
				return false;
			CItemBaseMulti *pItemMulti = dynamic_cast<CItemBaseMulti *>(this);
			if ( !pItemMulti )
				return false;

			pszKey += 9;
			if ( *pszKey == '.' )
			{
				pszKey++;
				if ( !strcmpi(pszKey, "TILES") )
				{
					sVal.FormatVal(pItemMulti->m_shipSpeed.tiles);
					break;
				}
				else if ( !strcmpi(pszKey, "PERIOD") )
				{
					sVal.FormatVal(pItemMulti->m_shipSpeed.period);
					break;
				}
				return false;
			}
			sVal.Format("%hhu,%hhu", pItemMulti->m_shipSpeed.period, pItemMulti->m_shipSpeed.tiles);
			break;
		}
		case IBC_DISPID:
			sVal = g_Cfg.ResourceGetName(RESOURCE_ID(RES_ITEMDEF, GetDispID()));
			break;
		case IBC_DUPELIST:
		{
			TCHAR *pszTemp = Str_GetTemp();
			size_t iLen = 0;
			*pszTemp = '\0';
			for ( size_t i = 0; i < m_flip_id.GetCount(); i++ )
			{
				if ( i > 0 )
					iLen += strcpylen(pszTemp + iLen, ",");

				iLen += sprintf(pszTemp + iLen, "0%x", static_cast<unsigned int>(m_flip_id[i]));
				ASSERT(iLen < SCRIPT_MAX_LINE_LEN);
			}
			sVal = pszTemp;
			break;
		}
		case IBC_BONUSSTR:
			sVal.FormatVal(m_StrengthBonus);
			break;
		case IBC_BONUSDEX:
			sVal.FormatVal(m_DexterityBonus);
			break;
		case IBC_BONUSINT:
			sVal.FormatVal(m_IntelligenceBonus);
			break;
		case IBC_BONUSHITS:
			sVal.FormatVal(m_HitpointIncrease);
			break;
		case IBC_BONUSSTAM:
			sVal.FormatVal(m_StaminaIncrease);
			break;
		case IBC_BONUSMANA:
			sVal.FormatVal(m_ManaIncrease);
			break;
		case IBC_MAGEARMOR:
			sVal.FormatVal(m_MageArmor);
			break;
		case IBC_MAGEWEAPON:
			sVal.FormatVal(m_MageWeapon);
			break;
		case IBC_RARITY:
			sVal.FormatVal(m_ArtifactRarity);
			break;
		case IBC_SELFREPAIR:
			sVal.FormatVal(m_SelfRepair);
			break;
		case IBC_SPELLCHANNELING:
			sVal.FormatVal(m_SpellChanneling);
			break;
		case IBC_LOWERREQ:
			sVal.FormatVal(m_LowerRequirements);
			break;
		case IBC_USEBESTWEAPONSKILL:
			sVal.FormatVal(m_UseBestWeaponSkill);
			break;
		case IBC_HITAREAPHYSICAL:
			sVal.FormatVal(m_HitPhysicalArea);
			break;
		case IBC_HITAREAFIRE:
			sVal.FormatVal(m_HitFireArea);
			break;
		case IBC_HITAREACOLD:
			sVal.FormatVal(m_HitColdArea);
			break;
		case IBC_HITAREAPOISON:
			sVal.FormatVal(m_HitPoisonArea);
			break;
		case IBC_HITAREAENERGY:
			sVal.FormatVal(m_HitEnergyArea);
			break;
		case IBC_HITDISPEL:
			sVal.FormatVal(m_HitDispel);
			break;
		case IBC_HITFIREBALL:
			sVal.FormatVal(m_HitFireball);
			break;
		case IBC_HITHARM:
			sVal.FormatVal(m_HitHarm);
			break;
		case IBC_HITLIGHTNING:
			sVal.FormatVal(m_HitLightning);
			break;
		case IBC_HITMAGICARROW:
			sVal.FormatVal(m_HitMagicArrow);
			break;
		case IBC_WEIGHTREDUCTION:
			sVal.FormatVal(m_WeightReduction);
			break;
		case IBC_CANUSE:
			sVal.FormatHex(m_CanUse);
			break;
		case IBC_DYE:
			sVal.FormatVal((m_Can & CAN_I_DYE) ? true : false);
			break;
		case IBC_ENCHANT:
			sVal.FormatVal((m_Can & CAN_I_ENCHANT) ? true : false);
			break;
		case IBC_EXCEPTIONAL:
			sVal.FormatVal((m_Can & CAN_I_EXCEPTIONAL) ? true : false);
			break;
		case IBC_FLIP:
			sVal.FormatHex((m_Can & CAN_I_FLIP) ? true : false);
			break;
		case IBC_ID:
			sVal.FormatHex(GetDispID());
			break;
		case IBC_IMBUE:
			sVal.FormatVal((m_Can & CAN_I_IMBUE) ? true : false);
			break;
		case IBC_ISARMOR:
			sVal.FormatVal(IsTypeArmor(m_type));
			break;
		case IBC_ISWEAPON:
			sVal.FormatVal(IsTypeWeapon(m_type));
			break;
		case IBC_MAKERSMARK:
			sVal.FormatVal((m_Can & CAN_I_MAKERSMARK) ? true : false);
			break;
		case IBC_RECYCLE:
			sVal.FormatVal((m_Can & CAN_I_RECYCLE) ? true : false);
			break;
		case IBC_REFORGE:
			sVal.FormatVal((m_Can & CAN_I_REFORGE) ? true : false);
			break;
		case IBC_RETAINCOLOR:
			sVal.FormatVal((m_Can & CAN_I_RETAINCOLOR) ? true : false);
			break;
		case IBC_SKILL:
		{
			if ( (m_iSkill > SKILL_NONE) && (m_iSkill < static_cast<SKILL_TYPE>(g_Cfg.m_iMaxSkill)) )
			{
				sVal.FormatVal(m_iSkill);
				break;
			}

			SKILL_TYPE skill;
			switch ( GetType() )
			{
				case IT_WEAPON_BOW:
				case IT_WEAPON_XBOW:
					skill = SKILL_ARCHERY;
					break;
				case IT_WEAPON_SWORD:
				case IT_WEAPON_AXE:
					skill = SKILL_SWORDSMANSHIP;
					break;
				case IT_WEAPON_MACE_SMITH:
				case IT_WEAPON_MACE_SHARP:
				case IT_WEAPON_MACE_STAFF:
				case IT_WEAPON_MACE_CROOK:
				case IT_WEAPON_MACE_PICK:
				case IT_WEAPON_WHIP:
					skill = SKILL_MACEFIGHTING;
					break;
				case IT_WEAPON_FENCE:
					skill = SKILL_FENCING;
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
			sVal.FormatVal(m_layer);
			break;
		case IBC_REPAIR:
			sVal.FormatHex((m_Can & CAN_I_REPAIR) ? true : false);
			break;
		case IBC_REPLICATE:
			sVal.FormatHex((m_Can & CAN_I_REPLICATE) ? true : false);
			break;
		case IBC_REQSTR:
			sVal.FormatVal(m_ttEquippable.m_StrReq);
			break;
		case IBC_SKILLMAKE:
		{
			pszKey += 9;
			if ( *pszKey == '.' )
			{
				SKIP_SEPARATORS(pszKey);
				int index = Exp_GetVal(pszKey);
				SKIP_SEPARATORS(pszKey);

				bool fQtyOnly = false;
				bool fKeyOnly = false;
				if ( !strnicmp(pszKey, "KEY", 3) )
					fKeyOnly = true;
				else if ( !strnicmp(pszKey, "VAL", 3) )
					fQtyOnly = true;

				TCHAR *pszTmp = Str_GetTemp();
				if ( fKeyOnly || fQtyOnly )
					m_SkillMake.WriteKeys(pszTmp, index, fQtyOnly, fKeyOnly);
				else
					m_SkillMake.WriteNames(pszTmp, index);
				if ( fQtyOnly && (pszTmp[0] == '\0') )
					strcpy(pszTmp, "0");
				sVal = pszTmp;
			}
			else
			{
				TCHAR *pszTmp = Str_GetTemp();
				m_SkillMake.WriteNames(pszTmp);
				sVal = pszTmp;
			}
			break;
		}
		case IBC_RESDISPDNID:
			sVal = g_Cfg.ResourceGetName(RESOURCE_ID(RES_TYPEDEF, GetResDispDnId()));
			break;
		case IBC_RESMAKE:
		{
			TCHAR *pszTmp = Str_GetTemp();
			m_BaseResources.WriteNames(pszTmp);
			sVal = pszTmp;
			break;
		}
		case IBC_SPEED:
			sVal.FormatVal(m_speed);
			break;
		case IBC_TDATA1:
			sVal.FormatHex(m_ttNormal.m_tData1);
			break;
		case IBC_TDATA2:
			sVal.FormatHex(m_ttNormal.m_tData2);
			break;
		case IBC_TDATA3:
			sVal.FormatHex(m_ttNormal.m_tData3);
			break;
		case IBC_TDATA4:
			sVal.FormatHex(m_ttNormal.m_tData4);
			break;
		case IBC_TFLAGS:
			sVal.FormatULLHex(GetTFlags());
			break;
		case IBC_TWOHANDS:
			if ( !IsTypeEquippable() )
				return false;
			if ( !IsTypeWeapon(GetType()) && !IsType(IT_FISH_POLE) )
				sVal.FormatVal(0);
			else
				sVal.FormatVal(m_layer == LAYER_HAND2);
			break;
		case IBC_TYPE:
		{
			RESOURCE_ID rid(RES_TYPEDEF, m_type);
			CResourceDef *pRes = g_Cfg.ResourceGetDef(rid);
			if ( !pRes )
				sVal.FormatVal(m_type);
			else
				sVal = pRes->GetResourceName();
			break;
		}
		case IBC_VALUE:
			if ( m_values.GetRange() )
				sVal.Format("%" FMTDWORD ",%" FMTDWORD, GetMakeValue(0), GetMakeValue(100));
			else
				sVal.Format("%" FMTDWORD, GetMakeValue(0));
			break;
		case IBC_WEIGHT:
			sVal.FormatVal(m_weight / WEIGHT_UNITS);
			break;
		default:
			return CBaseBaseDef::r_WriteVal(pszKey, sVal);
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CItemBase::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CItemBase::r_LoadVal");
	EXC_TRY("LoadVal");
	LPCTSTR pszKey = s.GetKey();
	switch ( FindTableSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		// Set as string
		case IBC_AMMOANIM:
		case IBC_AMMOANIMHUE:
		case IBC_AMMOANIMRENDER:
		case IBC_AMMOCONT:
		case IBC_AMMOTYPE:
		case IBC_AMMOSOUNDHIT:
		case IBC_AMMOSOUNDMISS:
		case IBC_DROPSOUND:
		case IBC_EQUIPSOUND:
		case IBC_BONUSSKILL1:
		case IBC_BONUSSKILL2:
		case IBC_BONUSSKILL3:
		case IBC_BONUSSKILL4:
		case IBC_BONUSSKILL5:
		case IBC_OCOLOR:
		{
			bool fQuoted = false;
			SetDefStr(s.GetKey(), s.GetArgStr(&fQuoted), fQuoted);
			break;
		}
		// Set as numeric
		case IBC_BONUSSKILL1AMT:
		case IBC_BONUSSKILL2AMT:
		case IBC_BONUSSKILL3AMT:
		case IBC_BONUSSKILL4AMT:
		case IBC_BONUSSKILL5AMT:
		case IBC_LIFESPAN:
		case IBC_USESCUR:
		case IBC_USESMAX:
		case IBC_BONUSHITSMAX:
		case IBC_BONUSSTAMMAX:
		case IBC_BONUSMANAMAX:
			SetDefNum(s.GetKey(), s.GetArgVal(), false);
			break;
		case IBC_MAXAMOUNT:
			if ( !SetMaxAmount(static_cast<WORD>(s.GetArgVal())) )
				return false;
			break;
		case IBC_SPEEDMODE:
		{
			if ( !IsType(IT_SHIP) )
				return false;
			CItemBaseMulti *pItemMulti = dynamic_cast<CItemBaseMulti *>(this);
			if ( !pItemMulti )
				return false;

			pItemMulti->m_SpeedMode = static_cast<BYTE>(minimum(maximum(s.GetArgVal(), 1), 4));
			break;
		}
		case IBC_SHIPSPEED:
		{
			pszKey += 9;
			if ( *pszKey == '.' )
			{
				pszKey++;
				if ( !IsType(IT_SHIP) )
					return false;
				CItemBaseMulti *pItemMulti = dynamic_cast<CItemBaseMulti *>(this);
				if ( !pItemMulti )
					return false;

				if ( !strcmpi(pszKey, "TILES") )
				{
					pItemMulti->m_shipSpeed.tiles = static_cast<BYTE>(s.GetArgVal());
					return true;
				}
				else if ( !strcmpi(pszKey, "PERIOD") )
				{
					pItemMulti->m_shipSpeed.tiles = static_cast<BYTE>(s.GetArgVal());
					return true;
				}
				INT64 piVal[2];
				size_t iQty = Str_ParseCmds(s.GetArgStr(), piVal, COUNTOF(piVal));
				if ( iQty == 2 )
				{
					pItemMulti->m_shipSpeed.period = static_cast<BYTE>(piVal[0]);
					pItemMulti->m_shipSpeed.tiles = static_cast<BYTE>(piVal[1]);
					return true;
				}
				return false;
			}
			break;
		}
		case IBC_BONUSSTR:
			m_StrengthBonus = static_cast<int>(s.GetArgVal());
			break;
		case IBC_BONUSDEX:
			m_DexterityBonus = static_cast<int>(s.GetArgVal());
			break;
		case IBC_BONUSINT:
			m_IntelligenceBonus = static_cast<int>(s.GetArgVal());
			break;
		case IBC_BONUSHITS:
			m_HitpointIncrease = static_cast<int>(s.GetArgVal());
			break;
		case IBC_BONUSSTAM:
			m_StaminaIncrease = static_cast<int>(s.GetArgVal());
			break;
		case IBC_BONUSMANA:
			m_ManaIncrease = static_cast<int>(s.GetArgVal());
			break;
		case IBC_MAGEARMOR:
			m_MageArmor = static_cast<int>(s.GetArgVal());
			break;
		case IBC_MAGEWEAPON:
			m_MageWeapon = static_cast<int>(s.GetArgVal());
			break;
		case IBC_RARITY:
			m_ArtifactRarity = static_cast<int>(s.GetArgVal());
			break;
		case IBC_SELFREPAIR:
			m_SelfRepair = static_cast<int>(s.GetArgVal());
			break;
		case IBC_SPELLCHANNELING:
			m_SpellChanneling = static_cast<int>(s.GetArgVal());
			break;
		case IBC_LOWERREQ:
			m_LowerRequirements = static_cast<int>(s.GetArgVal());
			break;
		case IBC_USEBESTWEAPONSKILL:
			m_UseBestWeaponSkill = static_cast<int>(s.GetArgVal());
			break;
		case IBC_HITAREAPHYSICAL:
			m_HitPhysicalArea = static_cast<int>(s.GetArgVal());
			break;
		case IBC_HITAREAFIRE:
			m_HitFireArea = static_cast<int>(s.GetArgVal());
			break;
		case IBC_HITAREACOLD:
			m_HitColdArea = static_cast<int>(s.GetArgVal());
			break;
		case IBC_HITAREAPOISON:
			m_HitPoisonArea = static_cast<int>(s.GetArgVal());
			break;
		case IBC_HITAREAENERGY:
			m_HitEnergyArea = static_cast<int>(s.GetArgVal());
			break;
		case IBC_HITDISPEL:
			m_HitDispel = static_cast<int>(s.GetArgVal());
			break;
		case IBC_HITFIREBALL:
			m_HitFireball = static_cast<int>(s.GetArgVal());
			break;
		case IBC_HITHARM:
			m_HitHarm = static_cast<int>(s.GetArgVal());
			break;
		case IBC_HITLIGHTNING:
			m_HitLightning = static_cast<int>(s.GetArgVal());
			break;
		case IBC_HITMAGICARROW:
			m_HitMagicArrow = static_cast<int>(s.GetArgVal());
			break;
		case IBC_WEIGHTREDUCTION:
			m_WeightReduction = static_cast<int>(s.GetArgVal());
			break;
		case IBC_CANUSE:
			m_CanUse = s.GetArgVal();
			break;
		case IBC_DISPID:
			return false;	// can't set this
		case IBC_DUPEITEM:
			return true;	// just ignore these
		case IBC_DUPELIST:
		{
			TCHAR *ppArgs[512];
			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), ppArgs, COUNTOF(ppArgs));
			if ( iArgQty <= 0 )
				return false;
			m_flip_id.Empty();
			for ( size_t i = 0; i < iArgQty; i++ )
			{
				ITEMID_TYPE id = static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType(RES_ITEMDEF, ppArgs[i]));
				if ( !IsValidDispID(id) )
					continue;
				if ( IsSameDispID(id) )
					continue;
				m_flip_id.Add(id);
			}
			break;
		}
		case IBC_DYE:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_DYE;
			else
			{
				if ( s.GetArgVal() )
					m_Can |= CAN_I_DYE;
				else
					m_Can &= ~CAN_I_DYE;
			}
			break;
		case IBC_FLIP:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_FLIP;
			else
			{
				if ( s.GetArgVal() )
					m_Can |= CAN_I_FLIP;
				else
					m_Can &= ~CAN_I_FLIP;
			}
			break;
		case IBC_ENCHANT:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_ENCHANT;
			else
			{
				if ( s.GetArgVal() )
					m_Can |= CAN_I_ENCHANT;
				else
					m_Can &= ~CAN_I_ENCHANT;
			}
			break;
		case IBC_EXCEPTIONAL:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_EXCEPTIONAL;
			else
			{
				if ( s.GetArgVal() )
					m_Can |= CAN_I_EXCEPTIONAL;
				else
					m_Can &= ~CAN_I_EXCEPTIONAL;
			}
			break;
		case IBC_IMBUE:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_IMBUE;
			else
			{
				if ( s.GetArgVal() )
					m_Can |= CAN_I_IMBUE;
				else
					m_Can &= ~CAN_I_IMBUE;
			}
			break;
		case IBC_REFORGE:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_REFORGE;
			else
			{
				if ( s.GetArgVal() )
					m_Can |= CAN_I_REFORGE;
				else
					m_Can &= ~CAN_I_REFORGE;
			}
			break;
		case IBC_RETAINCOLOR:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_RETAINCOLOR;
			else
			{
				if ( s.GetArgVal() )
					m_Can |= CAN_I_RETAINCOLOR;
				else
					m_Can &= ~CAN_I_RETAINCOLOR;
			}
			break;
		case IBC_MAKERSMARK:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_MAKERSMARK;
			else
			{
				if ( s.GetArgVal() )
					m_Can |= CAN_I_MAKERSMARK;
				else
					m_Can &= ~CAN_I_MAKERSMARK;
			}
			break;
		case IBC_RECYCLE:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_RECYCLE;
			else
			{
				if ( s.GetArgVal() )
					m_Can |= CAN_I_RECYCLE;
				else
					m_Can &= ~CAN_I_RECYCLE;
			}
			break;
		case IBC_REPAIR:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_REPAIR;
			else
			{
				if ( s.GetArgVal() )
					m_Can |= CAN_I_REPAIR;
				else
					m_Can &= ~CAN_I_REPAIR;
			}
			break;
		case IBC_REPLICATE:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_REPLICATE;
			else
			{
				if ( s.GetArgVal() )
					m_Can |= CAN_I_REPLICATE;
				else
					m_Can &= ~CAN_I_REPLICATE;
			}
			break;
		case IBC_ID:
		{
			if ( GetID() < ITEMID_MULTI )
			{
				DEBUG_ERR(("Setting new id for base type %s not allowed\n", GetResourceName()));
				return false;
			}

			ITEMID_TYPE id = static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType(RES_ITEMDEF, s.GetArgStr()));
			if ( !IsValidDispID(id) )
			{
				DEBUG_ERR(("Setting invalid id=%s for base type %s\n", s.GetArgStr(), GetResourceName()));
				return false;
			}

			CItemBase *pItemDef = FindItemBase(id);
			if ( !pItemDef )
			{
				DEBUG_ERR(("Setting unknown base id=0%x for %s\n", id, GetResourceName()));
				return false;
			}

			CopyBasic(pItemDef);
			m_dwDispIndex = id;		// might not be the default of a DUPEITEM
			break;
		}
		case IBC_LAYER:
			m_layer = static_cast<BYTE>(s.GetArgVal());
			break;
		case IBC_PILE:
			break;
		case IBC_REQSTR:
			if ( !IsTypeEquippable() )
				return false;
			m_ttEquippable.m_StrReq = s.GetArgVal();
			break;
		case IBC_RESDISPDNID:
			SetResDispDnId(static_cast<WORD>(g_Cfg.ResourceGetIndexType(RES_ITEMDEF, s.GetArgStr())));
			break;
		case IBC_SPEED:
			m_speed = static_cast<BYTE>(s.GetArgVal());
			break;
		case IBC_SKILL:
			m_iSkill = g_Cfg.FindSkillKey(s.GetArgStr());
			break;
		case IBC_SKILLMAKE:
			m_SkillMake.Load(s.GetArgStr());
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
			if ( (s.GetArgStr()[0] == '1') || (s.GetArgStr()[0] == 'Y') || (s.GetArgStr()[0] == 'y') )
				m_layer = LAYER_HAND2;
			break;
		case IBC_TYPE:
			m_type = static_cast<IT_TYPE>(g_Cfg.ResourceGetIndexType(RES_TYPEDEF, s.GetArgStr()));
			if ( m_type == IT_CONTAINER_LOCKED )	// at this level it just means to add a key for it
				m_type = IT_CONTAINER;
			break;
		case IBC_VALUE:
			m_values.Load(s.GetArgRaw());
			break;
		case IBC_WEIGHT:
			m_weight = static_cast<WORD>(s.GetArgVal());
			if ( strchr(s.GetArgStr(), '.') )
				m_weight *= WEIGHT_UNITS;
			break;
		default:
			return CBaseBaseDef::r_LoadVal(s);
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

void CItemBase::ReplaceItemBase(CItemBase *pOld, CResourceDef *pNew)	// static
{
	ADDTOCALLSTACK("CItemBase::ReplaceItemBase");
	ASSERT(pOld);
	ASSERT(pOld->GetRefInstances() == 0);
	RESOURCE_ID rid = pOld->GetResourceID();
	size_t index = g_Cfg.m_ResHash.FindKey(rid);
	ASSERT(index != g_Cfg.m_ResHash.BadIndex());
	g_Cfg.m_ResHash.SetAt(rid, index, pNew);
}

CItemBase *CItemBase::MakeDupeReplacement(CItemBase *pBase, ITEMID_TYPE iddupe)	// static
{
	ADDTOCALLSTACK("CItemBase::MakeDupeReplacement");
	ITEMID_TYPE id = pBase->GetID();
	if ( (iddupe == id) || !IsValidDispID(iddupe) )
	{
		DEBUG_ERR(("CItemBase:DUPEITEM weirdness 0%x==0%x\n", id, iddupe));
		return pBase;
	}

	CItemBase *pBaseNew = FindItemBase(iddupe);
	if ( !pBaseNew )
	{
		DEBUG_ERR(("CItemBase:DUPEITEM not exist 0%x==0%x\n", id, iddupe));
		return pBase;
	}

	if ( pBaseNew->GetID() != iddupe )
	{
		DEBUG_ERR(("CItemBase:DUPEITEM circle 0%x==0%x\n", id, iddupe));
		return pBase;
	}

	if ( !pBaseNew->IsSameDispID(id) )
		pBaseNew->m_flip_id.Add(id);

	// Create the dupe stub
	CUOItemTypeRec2 tiledata;
	memset(&tiledata, 0, sizeof(tiledata));

	CItemBaseDupe *pBaseDupe = new CItemBaseDupe(id, pBaseNew);
	if ( GetItemData(id, &tiledata) )
	{
		pBaseDupe->SetTFlags(tiledata.m_flags);
		pBaseDupe->SetHeight(GetItemHeightFlags(tiledata, pBaseDupe->m_Can));
	}
	ReplaceItemBase(pBase, pBaseDupe);
	return pBaseNew;
}

//*********************************************************
// CItemBaseDupe

CItemBaseDupe *CItemBaseDupe::GetDupeRef(ITEMID_TYPE id)	// static
{
	ADDTOCALLSTACK("CItemBaseDupe::GetDupeRef");
	if ( id <= ITEMID_NOTHING )
		return NULL;

	RESOURCE_ID rid = RESOURCE_ID(RES_ITEMDEF, id);
	size_t index = g_Cfg.m_ResHash.FindKey(rid);
	if ( index == g_Cfg.m_ResHash.BadIndex() )
		return NULL;

	CResourceDef *pBaseStub = g_Cfg.m_ResHash.GetAt(rid, index);
	CItemBase *pBase = dynamic_cast<CItemBase *>(pBaseStub);
	if ( pBase )
		return NULL;	// we want to return DupeItem, not BaseItem

	CItemBaseDupe *pBaseDupe = dynamic_cast<CItemBaseDupe *>(pBaseStub);
	if ( pBaseDupe )
		return pBaseDupe;	// this is just a DupeItem

	return NULL;	// we suspect item is loaded
}

//*********************************************************
// CItemBaseMulti

CItemBaseMulti::CItemBaseMulti(CItemBase *pBase) : CItemBase(pBase->GetID())
{
	m_Components.Empty();
	m_rect.SetRectEmpty();
	m_dwRegionFlags = REGION_FLAG_NOBUILDING;
	m_Speech.Empty();
	m_shipSpeed.period = TICK_PER_SEC / 2;
	m_shipSpeed.tiles = 1;
	m_SpeedMode = 3;
	CopyTransfer(pBase);	// copy the stuff from pBase
}

CItemBase *CItemBaseMulti::MakeMultiRegion(CItemBase *pBase, CScript &s)	// static
{
	ADDTOCALLSTACK("CItemBaseMulti::MakeMultiRegion");
	// MULTIREGION
	// We must transform this object into a CItemBaseMulti

	if ( !pBase )
		return NULL;

	if ( !pBase->IsTypeMulti(pBase->GetType()) )
	{
		DEBUG_ERR(("MULTIREGION defined for NON-MULTI type 0%x\n", pBase->GetID()));
		return pBase;
	}

	CItemBaseMulti *pBaseMulti = dynamic_cast<CItemBaseMulti *>(pBase);
	if ( !pBaseMulti )
	{
		if ( pBase->GetRefInstances() > 0 )
		{
			DEBUG_ERR(("MULTIREGION defined for IN USE NON-MULTI type 0%x\n", pBase->GetID()));
			return pBase;
		}

		pBaseMulti = new CItemBaseMulti(pBase);
		ReplaceItemBase(pBase, pBaseMulti);
	}
	pBaseMulti->SetMultiRegion(s.GetArgStr());
	return pBaseMulti;
}

void CItemBaseMulti::SetMultiRegion(TCHAR *pszArgs)
{
	ADDTOCALLSTACK("CItemBaseMulti::SetMultiRegion");
	INT64 piArgs[5];
	size_t iQty = Str_ParseCmds(pszArgs, piArgs, COUNTOF(piArgs));
	if ( iQty <= 1 )
		return;
	m_Components.Empty();	// might be after a resync
	m_rect.SetRect(static_cast<int>(piArgs[0]), static_cast<int>(piArgs[1]), static_cast<int>(piArgs[2] + 1), static_cast<int>(piArgs[3] + 1), static_cast<int>(piArgs[4]));
}

bool CItemBaseMulti::AddComponent(ITEMID_TYPE id, signed short dx, signed short dy, signed char dz)
{
	ADDTOCALLSTACK("CItemBaseMulti::AddComponent");
	m_rect.UnionPoint(dx, dy);
	if ( id > ITEMID_NOTHING )
	{
		CItemBase *pItemBase = FindItemBase(id);
		if ( !pItemBase )
		{
			DEBUG_ERR(("Bad COMPONENT 0%x\n", id));
			return false;
		}

		CMultiComponentItem comp;
		comp.m_id = id;
		comp.m_dx = dx;
		comp.m_dy = dy;
		comp.m_dz = dz;
		m_Components.Add(comp);
	}
	return true;
}

bool CItemBaseMulti::AddComponent(TCHAR *pszArgs)
{
	ADDTOCALLSTACK("CItemBaseMulti::AddComponent");
	INT64 piArgs[4];
	size_t iQty = Str_ParseCmds(pszArgs, piArgs, COUNTOF(piArgs));
	if ( iQty <= 1 )
		return false;
	return AddComponent(static_cast<ITEMID_TYPE>(RES_GET_INDEX(piArgs[0])), static_cast<short>(piArgs[1]), static_cast<short>(piArgs[2]), static_cast<char>(piArgs[3]));
}

int CItemBaseMulti::GetMaxDist() const
{
	ADDTOCALLSTACK("CItemBaseMulti::GetMaxDist");
	int iDist = abs(m_rect.m_left);
	int iDistTmp = abs(m_rect.m_top);
	if ( iDistTmp > iDist )
		iDist = iDistTmp;
	iDistTmp = abs(m_rect.m_right + 1);
	if ( iDistTmp > iDist )
		iDist = iDistTmp;
	iDistTmp = abs(m_rect.m_bottom + 1);
	if ( iDistTmp > iDist )
		iDist = iDistTmp;
	return iDist + 1;
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

bool CItemBaseMulti::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CItemBaseMulti::r_LoadVal");
	EXC_TRY("LoadVal");
	switch ( FindTableSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case MLC_COMPONENT:
			return AddComponent(s.GetArgStr());
		case MLC_MULTIREGION:
			MakeMultiRegion(this, s);
			break;
		case MLC_REGIONFLAGS:
			m_dwRegionFlags = s.GetArgVal();
			return true;
		case MLC_SHIPSPEED:
		{
			if ( !IsType(IT_SHIP) )
				return false;

			INT64 piArgs[2];
			size_t iQty = Str_ParseCmds(s.GetArgRaw(), piArgs, COUNTOF(piArgs));
			if ( iQty < 1 )
				return false;

			m_shipSpeed.period = static_cast<BYTE>(piArgs[0]);

			if ( iQty >= 2 )
				m_shipSpeed.tiles = static_cast<BYTE>(piArgs[1]);
			break;
		}
		case MLC_TSPEECH:
			return m_Speech.r_LoadVal(s, RES_SPEECH);
		default:
			return CItemBase::r_LoadVal(s);
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CItemBaseMulti::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CItemBaseMulti::r_WriteVal");
	EXC_TRY("WriteVal");
	switch ( FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case MLC_BASECOMPONENT:
		{
			pszKey += 13;
			const CGrayMulti *pMulti = g_Cfg.GetMultiItemDefs(GetDispID());
			if ( !pMulti )
				return false;

			if ( *pszKey == '\0' )
				sVal.FormatVal(pMulti->GetItemCount());
			else if ( *pszKey == '.' )
			{
				SKIP_SEPARATORS(pszKey);
				size_t index = Exp_GetVal(pszKey);
				if ( index >= pMulti->GetItemCount() )
					return false;
				SKIP_SEPARATORS(pszKey);
				const CUOMultiItemRecHS *pItem = pMulti->GetItem(index);

				if ( *pszKey == '\0' )
					sVal.Format("%hu,%hd,%hd,%hd", pItem->m_wTileID, pItem->m_dx, pItem->m_dy, pItem->m_dz);
				else if ( !strnicmp(pszKey, "ID", 2) )
					sVal.FormatVal(pItem->m_wTileID);
				else if ( !strnicmp(pszKey, "DX", 2) )
					sVal.FormatVal(pItem->m_dx);
				else if ( !strnicmp(pszKey, "DY", 2) )
					sVal.FormatVal(pItem->m_dy);
				else if ( !strnicmp(pszKey, "DZ", 2) )
					sVal.FormatVal(pItem->m_dz);
				else if ( !strnicmp(pszKey, "D", 1) )
					sVal.Format("%hd,%hd,%hd", pItem->m_dx, pItem->m_dy, pItem->m_dz);
				else if ( !strnicmp(pszKey, "VISIBLE", 7) )
					sVal.FormatVal(pItem->m_visible);
				else
					return false;
			}
			else
				return false;
			return true;
		}
		case MLC_COMPONENT:
		{
			pszKey += 9;
			if ( *pszKey == '\0' )
				sVal.FormatVal(m_Components.GetCount());
			else if ( *pszKey == '.' )
			{
				SKIP_SEPARATORS(pszKey);
				size_t index = Exp_GetVal(pszKey);
				if ( !m_Components.IsValidIndex(index) )
					return false;

				SKIP_SEPARATORS(pszKey);
				CMultiComponentItem item = m_Components.GetAt(index);

				if ( !strnicmp(pszKey, "ID", 2) )
					sVal.FormatVal(item.m_id);
				else if ( !strnicmp(pszKey, "DX", 2) )
					sVal.FormatVal(item.m_dx);
				else if ( !strnicmp(pszKey, "DY", 2) )
					sVal.FormatVal(item.m_dy);
				else if ( !strnicmp(pszKey, "DZ", 2) )
					sVal.FormatVal(item.m_dz);
				else if ( !strnicmp(pszKey, "D", 1) )
					sVal.Format("%hd,%hd,%hhd", item.m_dx, item.m_dy, item.m_dz);
				else
					sVal.Format("%u,%hd,%hd,%hhd", item.m_id, item.m_dx, item.m_dy, item.m_dz);
			}
			else
				return false;
			return true;
		}
		case MLC_MULTIREGION:
			sVal.Format("%d,%d,%d,%d", m_rect.m_left, m_rect.m_top, m_rect.m_right - 1, m_rect.m_bottom - 1);
			return true;
		case MLC_REGIONFLAGS:
			sVal.FormatHex(m_dwRegionFlags);
			return true;
		case MLC_SHIPSPEED:
		{
			if ( !IsType(IT_SHIP) )
				return false;

			pszKey += 9;
			if ( *pszKey == '.' )
			{
				pszKey++;
				if ( !strcmpi(pszKey, "TILES") )
				{
					sVal.FormatVal(m_shipSpeed.tiles);
					break;
				}
				else if ( !strcmpi(pszKey, "PERIOD") )
				{
					sVal.FormatVal(m_shipSpeed.period);
					break;
				}
				return false;
			}

			sVal.Format("%hhu,%hhu", m_shipSpeed.period, m_shipSpeed.tiles);
			break;
		}
		default:
			return CItemBase::r_WriteVal(pszKey, sVal, pSrc);
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}
