#include "graysvr.h"	// predef header.

/////////////////////////////////////////////////////////////////////////////

BYTE CItemSpawn::GetAmount()
{
	ADDTOCALLSTACK("CItemSpawn::GetAmount");
	return m_iAmount;
}

void CItemSpawn::SetAmount(BYTE iAmount)
{
	ADDTOCALLSTACK("CItemSpawn::SetAmount");
	m_iAmount = iAmount;
}

inline CCharBase *CItemSpawn::TryChar(CREID_TYPE &id)
{
	ADDTOCALLSTACK("CItemSpawn::TryChar");
	CCharBase *pCharDef = CCharBase::FindCharBase(id);
	if ( pCharDef )
	{
		m_itSpawnChar.m_CharID = RESOURCE_ID(RES_CHARDEF, id);
		return pCharDef;
	}
	return NULL;
}

inline CItemBase *CItemSpawn::TryItem(ITEMID_TYPE &id)
{
	ADDTOCALLSTACK("CItemSpawn::TryItem");
	CItemBase *pItemDef = CItemBase::FindItemBase(id);
	if ( pItemDef )
	{
		m_itSpawnItem.m_ItemID = RESOURCE_ID(RES_ITEMDEF, id);
		return pItemDef;
	}
	return NULL;
}

CResourceDef *CItemSpawn::FixDef()
{
	ADDTOCALLSTACK("CItemSpawn::FixDef");

	RESOURCE_ID_BASE rid = (IsType(IT_SPAWN_ITEM) ? m_itSpawnItem.m_ItemID : m_itSpawnChar.m_CharID);
	if ( rid.GetResType() != RES_UNKNOWN )
		return static_cast<CResourceDef *>(g_Cfg.ResourceGetDef(rid));

	// No type info here !?
	if ( IsType(IT_SPAWN_CHAR) )
	{
		CREID_TYPE id = static_cast<CREID_TYPE>(rid.GetResIndex());
		if ( id < SPAWNTYPE_START )
			return TryChar(id);

		// try a spawn group.
		rid = RESOURCE_ID(RES_SPAWN, id);
		CResourceDef *pDef = g_Cfg.ResourceGetDef(rid);
		if ( pDef )
		{
			m_itSpawnChar.m_CharID = rid;
			return pDef;
		}
		return TryChar(id);
	}
	else
	{
		ITEMID_TYPE id = static_cast<ITEMID_TYPE>(rid.GetResIndex());
		if ( id < ITEMID_TEMPLATE )
			return TryItem(id);

		// try a template.
		rid = RESOURCE_ID(RES_TEMPLATE, id);
		CResourceDef *pDef = g_Cfg.ResourceGetDef(rid);
		if ( pDef )
		{
			m_itSpawnItem.m_ItemID = rid;
			return pDef;
		}
		return TryItem(id);
	}
}

int CItemSpawn::GetName(TCHAR *pszOut) const
{
	ADDTOCALLSTACK("CItemSpawn::GetName");
	RESOURCE_ID_BASE rid;
	if ( IsType(IT_SPAWN_ITEM) )
		rid = m_itSpawnItem.m_ItemID;
	else
		rid = m_itSpawnChar.m_CharID;	// name the spawn type

	LPCTSTR pszName = NULL;
	CResourceDef *pDef = g_Cfg.ResourceGetDef(rid);
	if ( pDef )
		pszName = pDef->GetName();
	if ( !pDef || !pszName || (pszName[0] == '\0') )
		pszName = g_Cfg.ResourceGetName(rid);

	return sprintf(pszOut, " (%s)", pszName);
}

CItemSpawn::CItemSpawn(ITEMID_TYPE id, CItemBase *pItemDef) : CItem(ITEMID_WorldGem, pItemDef)
{
	ADDTOCALLSTACK("CItemSpawn::CItemSpawn");
	UNREFERENCED_PARAMETER(id);		//forced in CItem(ITEMID_WorldGem, ...)
	m_currentSpawned = 0;
	m_iAmount = 1;
}

CItemSpawn::~CItemSpawn()
{
	ADDTOCALLSTACK("CItemSpawn::~CItemSpawn");
}

BYTE CItemSpawn::GetCount()
{
	ADDTOCALLSTACK("CItemSpawn::GetCount");
	return m_currentSpawned;
}

void CItemSpawn::GenerateItem(CResourceDef *pDef)
{
	ADDTOCALLSTACK("CItemSpawn::GenerateItem");

	RESOURCE_ID_BASE rid = pDef->GetResourceID();
	CItemContainer *pCont = dynamic_cast<CItemContainer *>(GetParent());
	BYTE iCount = pCont ? static_cast<BYTE>(pCont->ContentCount(rid)) : GetCount();
	if ( iCount >= GetAmount() )
		return;

	CItem *pItem = CreateTemplate(static_cast<ITEMID_TYPE>(rid.GetResIndex()));
	if ( !pItem )
		return;

	WORD wAmountPile = static_cast<WORD>(minimum(WORD_MAX, m_itSpawnItem.m_pile));
	if ( wAmountPile > 1 )
	{
		CItemBase *pItemDef = pItem->Item_GetDef();
		ASSERT(pItemDef);
		if ( pItemDef->IsStackableType() )
			pItem->SetAmount(static_cast<WORD>(Calc_GetRandVal(wAmountPile)) + 1);
	}

	pItem->SetAttr(m_Attr & (ATTR_OWNED|ATTR_MOVE_ALWAYS));
	pItem->SetDecayTime(g_Cfg.m_iDecay_Item);	// it will decay eventually to be replaced later
	pItem->MoveNearObj(this, m_itSpawnItem.m_DistMax);
	AddObj(pItem->GetUID());
}

void CItemSpawn::GenerateChar(CResourceDef *pDef)
{
	ADDTOCALLSTACK("CItemSpawn::GenerateChar");

	RESOURCE_ID_BASE rid = pDef->GetResourceID();
	if ( rid.GetResType() == RES_SPAWN )
	{
		const CRandGroupDef *pSpawnGroup = static_cast<const CRandGroupDef *>(pDef);
		ASSERT(pSpawnGroup);
		size_t i = pSpawnGroup->GetRandMemberIndex();
		if ( i != pSpawnGroup->BadMemberIndex() )
			rid = pSpawnGroup->GetMemberID(i);
	}

	if ( (rid.GetResType() != RES_CHARDEF) && (rid.GetResType() != RES_UNKNOWN) )
		return;

	CPointMap pt = GetTopPoint();
	CRegionBase *pRegion = pt.GetRegion(REGION_TYPE_AREA);
	if ( !pRegion )
		return;

	CChar *pChar = CChar::CreateBasic(static_cast<CREID_TYPE>(rid.GetResIndex()));
	if ( !pChar )
		return;

	pChar->NPC_LoadScript(true);
	pChar->StatFlag_Set(STATF_Spawned);

	// Try placing the char near the spawn
	if ( !pChar->MoveNearObj(this, m_itSpawnChar.m_DistMax) || !pChar->CanSeeLOS(pt) )
	{
		// If this fails, try placing the char over the spawn
		if ( !pChar->MoveTo(pt) )
		{
			DEBUG_ERR(("Spawn UID=0%" FMTDWORDH " is unable to move the created character to world\n", static_cast<DWORD>(GetUID())));
			pChar->Delete();
			return;
		}
	}

	AddObj(pChar->GetUID());
	pChar->NPC_CreateTrigger();		// removed from NPC_LoadScript() and triggered after char placement and attachment to the spawnitem
	pChar->Update();

	size_t iCount = GetTopSector()->GetCharComplexity();
	if ( iCount > g_Cfg.m_iMaxCharComplexity )
		g_Log.Event(LOGL_WARN, "%" FMTSIZE_T " chars at %s. Sector too complex!\n", iCount, GetTopSector()->GetBasePoint().WriteUsed());
}

void CItemSpawn::DelObj(CGrayUID uid)
{
	ADDTOCALLSTACK("CItemSpawn::DelObj");
	if ( !uid.IsValidUID() )
		return;

	BYTE iMax = GetCount();
	for ( BYTE i = 0; i < iMax; ++i )
	{
		if ( m_obj[i] != uid )
			continue;

		CObjBase *pObj = uid.ObjFind();
		pObj->m_uidSpawnItem.InitUID();

		--m_currentSpawned;
		if ( GetType() == IT_SPAWN_CHAR )
			uid.CharFind()->StatFlag_Clear(STATF_Spawned);

		while ( m_obj[i + 1].IsValidUID() )				// searching for any entry higher than this one...
		{
			m_obj[i] = m_obj[i + 1];					// and moving it 1 position to keep values 'together'.
			++i;
		}
		m_obj[i].InitUID();								// Finished moving higher entries (if any) so we free the last entry.
		break;
	}
	UpdatePropertyFlag();
}

void CItemSpawn::AddObj(CGrayUID uid)
{
	ADDTOCALLSTACK("CItemSpawn::AddObj");
	// NOTE: This function is also called when loading spawn items
	// on server startup. In this case, some objs UID still invalid
	// (not loaded yet) so just proceed without any checks.

	bool bIsSpawnChar = IsType(IT_SPAWN_CHAR);
	if ( !g_Serv.IsLoading() )
	{
		if ( !uid.IsValidUID() )
			return;

		if ( bIsSpawnChar )				// IT_SPAWN_CHAR can only spawn NPCs
		{
			CChar *pChar = uid.CharFind();
			if ( !pChar || !pChar->m_pNPC )
				return;
		}
		else if ( !uid.ItemFind() )		// IT_SPAWN_ITEM can only spawn items
			return;

		CItemSpawn *pPrevSpawn = static_cast<CItemSpawn *>(uid.ObjFind()->m_uidSpawnItem.ItemFind());
		if ( pPrevSpawn )
		{
			if ( pPrevSpawn == this )		// obj already linked to this spawn
				return;
			pPrevSpawn->DelObj(uid);		// obj linked to other spawn, remove the link before proceed
		}
	}

	BYTE iMax = maximum(GetAmount(), 1);
	for ( BYTE i = 0; i < iMax; ++i )
	{
		if ( !m_obj[i].IsValidUID() )
		{
			m_obj[i] = uid;
			++m_currentSpawned;

			// objects are linked to the spawn at each server start
			if ( !g_Serv.IsLoading() )
			{
				uid.ObjFind()->m_uidSpawnItem = GetUID();
				if ( bIsSpawnChar )
				{
					CChar *pChar = uid.CharFind();
					ASSERT(pChar->m_pNPC);
					pChar->StatFlag_Set(STATF_Spawned);
					pChar->m_ptHome = GetTopPoint();
					pChar->m_pNPC->m_Home_Dist_Wander = static_cast<WORD>(m_itSpawnChar.m_DistMax);
				}
			}
			break;
		}
	}
	UpdatePropertyFlag();
}

void CItemSpawn::OnTick(bool fExec)
{
	ADDTOCALLSTACK("CItemSpawn::OnTick");
	EXC_TRY("Tick");

	INT64 iMinutes;
	if ( m_itSpawnChar.m_TimeHiMin <= 0 )
		iMinutes = Calc_GetRandLLVal(30) + 1;
	else
		iMinutes = minimum(m_itSpawnChar.m_TimeHiMin, m_itSpawnChar.m_TimeLoMin) + Calc_GetRandLLVal(abs(m_itSpawnChar.m_TimeHiMin - m_itSpawnChar.m_TimeLoMin));

	if ( iMinutes <= 0 )
		iMinutes = 1;

	if ( !fExec || IsTimerExpired() )
		SetTimeout(iMinutes * 60 * TICK_PER_SEC);	// set time to check again.

	if ( !fExec || (m_currentSpawned >= GetAmount()) )
		return;
	if ( !IsTopLevel() )
		return;

	CResourceDef *pDef = FixDef();
	if ( !pDef )
	{
		RESOURCE_ID_BASE rid = IsType(IT_SPAWN_ITEM) ? m_itSpawnItem.m_ItemID : m_itSpawnChar.m_CharID;
		DEBUG_ERR(("Bad Spawn point UID=0%" FMTDWORDH ", id=%s\n", static_cast<DWORD>(GetUID()), g_Cfg.ResourceGetName(rid)));
		return;
	}

	if ( IsType(IT_SPAWN_ITEM) )
		GenerateItem(pDef);
	else
		GenerateChar(pDef);

	EXC_CATCH;
}

void CItemSpawn::KillChildren()
{
	ADDTOCALLSTACK("CItemSpawn::KillChildren");
	if ( m_currentSpawned <= 0 )
		return;

	for ( BYTE i = 0; i < m_currentSpawned; ++i )
	{
		CObjBase *pObj = m_obj[i].ObjFind();
		if ( !pObj )
			continue;

		if ( pObj->IsChar() )
			static_cast<CChar *>(pObj)->Delete(true);
		else if ( pObj->IsItem() )
			static_cast<CItem *>(pObj)->Delete(true);

		m_obj[i].InitUID();
	}

	m_currentSpawned = 0;
	UpdatePropertyFlag();
	OnTick(false);
}

CCharBase *CItemSpawn::SetTrackID()
{
	ADDTOCALLSTACK("CItemSpawn::SetTrackID");
	SetAttr(ATTR_INVIS);	// Indicate to GM's that it is invis.
	if ( GetHue() == 0 )
		SetHue(HUE_RED_DARK);

	if ( !IsType(IT_SPAWN_CHAR) )
	{
		SetDispID(ITEMID_WorldGem_lg);
		return NULL;
	}

	CCharBase *pCharDef = NULL;
	RESOURCE_ID_BASE rid = m_itSpawnChar.m_CharID;
	if ( rid.GetResType() == RES_CHARDEF )
		pCharDef = CCharBase::FindCharBase(static_cast<CREID_TYPE>(rid.GetResIndex()));

	SetDispID(pCharDef ? pCharDef->m_trackID : ITEMID_TRACK_WISP);
	return pCharDef;
}

enum ISPW_TYPE
{
	ISPW_ADDOBJ,
	ISPW_AMOUNT,
	ISPW_AT,
	ISPW_COUNT,
	ISPW_DELOBJ,
	ISPW_RESET,
	ISPW_START,
	ISPW_STOP,
	ISPW_QTY
};

LPCTSTR const CItemSpawn::sm_szLoadKeys[ISPW_QTY + 1] =
{
	"ADDOBJ",
	"AMOUNT",
	"AT",
	"COUNT",
	"DELOBJ",
	"RESET",
	"START",
	"STOP",
	NULL
};

bool CItemSpawn::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CItemSpawn::r_WriteVal");
	EXC_TRY("WriteVal");
	if ( !strnicmp(pszKey, "AMOUNT", 6) )
	{
		sVal.FormatVal(GetAmount());
		return true;
	}
	else if ( !strnicmp(pszKey, "AT.", 3) )
	{
		pszKey += 3;
		int i = Exp_GetVal(pszKey);
		if ( (i >= 0) && (i <= BYTE_MAX - 1) )
		{
			CObjBase *pObj = m_obj[i].ObjFind();
			if ( pObj )
				pObj->r_WriteVal(pszKey, sVal, pSrc);
		}
		return true;
	}
	else if ( !strnicmp(pszKey, "COUNT", 5) )
	{
		sVal.FormatVal(GetCount());
		return true;
	}
	EXC_CATCH;
	return CItem::r_WriteVal(pszKey, sVal, pSrc);
}

bool CItemSpawn::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CItemSpawn::r_LoadVal");
	EXC_TRY("LoadVal");

	int iCmd = FindTableSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	if ( iCmd < 0 )
		return CItem::r_LoadVal(s);

	switch ( iCmd )
	{
		case ISPW_ADDOBJ:
		{
			AddObj(static_cast<CGrayUID>(s.GetArgVal()));
			return true;
		}
		case ISPW_AMOUNT:
		{
			SetAmount(static_cast<BYTE>(s.GetArgVal()));
			return true;
		}
		case ISPW_DELOBJ:
		{
			DelObj(static_cast<CGrayUID>(s.GetArgVal()));
			return true;
		}
		case ISPW_RESET:
			KillChildren();
			return true;
		case ISPW_START:
			SetTimeout(0);
			return true;
		case ISPW_STOP:
			KillChildren();
			SetTimeout(-1);
			return true;
		default:
			break;
	}
	EXC_CATCH;
	return false;
}

void CItemSpawn::r_Write(CScript &s)
{
	ADDTOCALLSTACK("CItemSpawn::r_Write");
	EXC_TRY("Write");
	CItem::r_Write(s);

	if ( GetAmount() != 1 )
		s.WriteKeyVal("AMOUNT", GetAmount());

	WORD wTotal = GetCount();
	if ( wTotal <= 0 )
		return;

	for ( WORD w = 0; w < wTotal; ++w )
	{
		if ( !m_obj[w].IsValidUID() )
			continue;
		CObjBase *pObj = m_obj[w].ObjFind();
		if ( pObj )
			s.WriteKeyHex("ADDOBJ", pObj->GetUID());
	}

	EXC_CATCH;
}

/////////////////////////////////////////////////////////////////////////////

void CItem::Plant_SetTimer()
{
	ADDTOCALLSTACK("CItem::Plant_SetTimer");
	SetTimeout(GetDecayTime());
}

// Pick cotton/hay/etc...
// use the
//  IT_CROPS = transforms into a "ripe" variety then is used up on reaping.
//  IT_FOLIAGE = is not consumed on reap (unless eaten then will regrow invis)

bool CItem::Plant_Use(CChar *pChar)
{
	ADDTOCALLSTACK("CItem::Plant_Use");

	if ( !pChar )
		return false;
	if ( !pChar->CanSeeItem(this) )		// might be invis underground
		return false;

	const CItemBase *pItemDef = Item_GetDef();
	ITEMID_TYPE iFruitID = static_cast<ITEMID_TYPE>(RES_GET_INDEX(pItemDef->m_ttCrops.m_idFruit));	// if it's reapable at this stage
	if ( iFruitID <= 0 )
	{
		// not ripe. (but we could just eat it if we are herbivorous ?)
		pChar->SysMessageDefault(DEFMSG_CROPS_NOT_RIPE);
		return true;
	}

	if ( m_itCrop.m_ReapFruitID )
		iFruitID = static_cast<ITEMID_TYPE>(RES_GET_INDEX(m_itCrop.m_ReapFruitID));
	if ( iFruitID )
	{
		CItem *pItemFruit = CItem::CreateScript(iFruitID, pChar);
		if ( pItemFruit )
			pChar->ItemBounce(pItemFruit);
	}
	else
		pChar->SysMessageDefault(DEFMSG_CROPS_NO_FRUIT);

	Plant_CropReset();
	pChar->UpdateAnimate(ANIM_BOW);
	pChar->Sound(0x13e);
	return true;
}

// timer expired, should I grow?
bool CItem::Plant_OnTick()
{
	ADDTOCALLSTACK("CItem::Plant_OnTick");
	ASSERT(IsType(IT_CROPS) || IsType(IT_FOLIAGE));
	// If it is in a container, kill it.
	if ( !IsTopLevel() )
		return false;

	// Make sure the darn thing isn't moveable
	SetAttr(ATTR_MOVE_NEVER);
	Plant_SetTimer();

	// No tree stuff below here
	if ( IsAttr(ATTR_INVIS) )	// if it's invis, take care of it here and return
	{
		SetHue(HUE_DEFAULT);
		ClrAttr(ATTR_INVIS);
		Update();
		return true;
	}

	const CItemBase *pItemDef = Item_GetDef();
	ITEMID_TYPE iGrowID = pItemDef->m_ttCrops.m_idGrow;

	if ( iGrowID == -1 )
	{
		// Some plants generate a fruit on the ground when ripe.
		ITEMID_TYPE iFruitID = static_cast<ITEMID_TYPE>(RES_GET_INDEX(pItemDef->m_ttCrops.m_idGrow));
		if ( m_itCrop.m_ReapFruitID )
			iFruitID = static_cast<ITEMID_TYPE>(RES_GET_INDEX(m_itCrop.m_ReapFruitID));
		if ( !iFruitID )
			return true;

		// Put a fruit on the ground if not already here.
		CWorldSearch AreaItems(GetTopPoint());
		for (;;)
		{
			CItem *pItem = AreaItems.GetItem();
			if ( !pItem )
			{
				CItem *pItemFruit = CItem::CreateScript(iFruitID);
				ASSERT(pItemFruit);
				pItemFruit->MoveToDecay(GetTopPoint(), g_Cfg.m_iDecay_Item);
				break;
			}
			if ( pItem->IsType(IT_FRUIT) || pItem->IsType(IT_REAGENT_RAW) )
				break;
		}

		// NOTE: ??? The plant should cycle here as well !
		iGrowID = pItemDef->m_ttCrops.m_idReset;
	}

	if ( iGrowID )
	{
		SetID(static_cast<ITEMID_TYPE>(RES_GET_INDEX(iGrowID)));
		Update();
		return true;
	}

	// some plants go dormant again ?
	// m_itCrop.m_Fruit_ID = iTemp;
	return true;
}

// Animals will eat crops before they are ripe, so we need a way to reset them prematurely
void CItem::Plant_CropReset()
{
	ADDTOCALLSTACK("CItem::Plant_CropReset");

	if ( !IsType(IT_CROPS) && !IsType(IT_FOLIAGE) )
	{
		// This isn't a crop, and since it just got eaten, we should delete it
		Delete();
		return;
	}

	const CItemBase *pItemDef = Item_GetDef();
	ITEMID_TYPE iResetID = static_cast<ITEMID_TYPE>(RES_GET_INDEX(pItemDef->m_ttCrops.m_idReset));
	if ( iResetID != ITEMID_NOTHING )
		SetID(iResetID);

	Plant_SetTimer();
	RemoveFromView();		// remove from most screens.
	SetHue(HUE_RED_DARK);	// Indicate to GM's that it is growing.
	SetAttr(ATTR_INVIS);	// regrown invis.
}

/////////////////////////////////////////////////////////////////////////////
// -CItemMap

bool CItemMap::IsSameType(const CObjBase *pObj) const
{
	const CItemMap *pItemMap = dynamic_cast<const CItemMap *>(pObj);
	if ( pItemMap )
	{
		// Maps can only stack on top of each other if the pins match
		if ( m_Pins.GetCount() != pItemMap->m_Pins.GetCount() )
			return false;

		// Check individual pins in the same place
		for ( size_t i = 0; i < m_Pins.GetCount(); ++i )
		{
			if ( m_Pins[i].m_x != pItemMap->m_Pins[i].m_x )
				return false;
			if ( m_Pins[i].m_y != pItemMap->m_Pins[i].m_y )
				return false;
		}
	}

	return CItemVendable::IsSameType(pObj);
}

bool CItemMap::r_LoadVal(CScript &s)	// load an item script
{
	ADDTOCALLSTACK("CItemMap::r_LoadVal");
	EXC_TRY("LoadVal");
	if ( s.IsKeyHead("PIN", 3) )
	{
		CPointMap ptTemp;
		ptTemp.Read(s.GetArgStr());
		CMapPinRec pin(ptTemp.m_x, ptTemp.m_y);
		m_Pins.Add(pin);
		return true;
	}
	return CItem::r_LoadVal(s);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CItemMap::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CItemMap::r_WriteVal");
	EXC_TRY("WriteVal");
	if ( !strnicmp(pszKey, "PINS", 4) )
	{
		sVal.FormatVal(m_Pins.GetCount());
		return true;
	}
	if ( !strnicmp(pszKey, "PIN.", 4) )
	{
		pszKey += 4;
		int i = Exp_GetVal(pszKey) - 1;
		if ( (i >= 0) && m_Pins.IsValidIndex(i) )
		{
			sVal.Format("%i,%i", m_Pins[i].m_x, m_Pins[i].m_y);
			return true;
		}
	}
	return CItemVendable::r_WriteVal(pszKey, sVal, pSrc);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

void CItemMap::r_Write(CScript &s)
{
	ADDTOCALLSTACK_INTENSIVE("CItemMap::r_Write");
	CItemVendable::r_Write(s);
	for ( size_t i = 0; i < m_Pins.GetCount(); ++i )
		s.WriteKeyFormat("PIN", "%i,%i", m_Pins[i].m_x, m_Pins[i].m_y);
}

void CItemMap::DupeCopy(const CItem *pItem)
{
	ADDTOCALLSTACK("CItemMap::DupeCopy");
	CItemVendable::DupeCopy(pItem);

	const CItemMap *pMapItem = dynamic_cast<const CItemMap *>(pItem);
	if ( !pMapItem )
		return;
	m_Pins.Copy(&pMapItem->m_Pins);
}

/////////////////////////////////////////////////////////////////////////////
// -CItemMessage

void CItemMessage::r_Write(CScript &s)
{
	ADDTOCALLSTACK_INTENSIVE("CItemMessage::r_Write");
	CItemVendable::r_Write(s);
	s.WriteKey("AUTHOR", m_sAuthor);

	// Store the message body lines. MAX_BOOK_PAGES
	TemporaryString pszTemp;
	for ( WORD w = 0; w < GetPageCount(); ++w )
	{
		sprintf(pszTemp, "BODY.%hu", w);
		LPCTSTR pszText = GetPageText(w);
		s.WriteKey(pszTemp, pszText ? pszText : "");
	}
}

LPCTSTR const CItemMessage::sm_szLoadKeys[CIC_QTY + 1] =	// static
{
	"AUTHOR",
	"BODY",
	"PAGES",	// (W)
	"TITLE",	// same as name
	NULL
};

bool CItemMessage::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CItemMessage::r_LoadVal");
	EXC_TRY("LoadVal");
	// Load the message body for a book or a bboard message.
	if ( s.IsKeyHead("BODY", 4) )
	{
		AddPageText(s.GetArgStr());
		return true;
	}

	switch ( FindTableSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case CIC_AUTHOR:
			if ( s.GetArgStr()[0] != '0' )
				m_sAuthor = s.GetArgStr();
			return true;
		case CIC_BODY:		// handled above
			return false;
		case CIC_PAGES:		// not settable (used for resource stuff)
			return false;
		case CIC_TITLE:
			SetName(s.GetArgStr());
			return true;
	}
	return CItemVendable::r_LoadVal(s);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CItemMessage::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CItemMessage::r_WriteVal");
	EXC_TRY("WriteVal");
	// Load the message body for a book or a bboard message.
	if ( !strnicmp(pszKey, "BODY", 4) )
	{
		pszKey += 4;
		int i = Exp_GetVal(pszKey);
		if ( (i < 0) || !m_sBodyLines.IsValidIndex(i) )
			return false;
		sVal = *m_sBodyLines[i];
		return true;
	}

	switch ( FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case CIC_AUTHOR:
			sVal = m_sAuthor;
			return true;
		case CIC_BODY:		// handled above
			return false;
		case CIC_PAGES:		// not settable (used for resource stuff)
			sVal.FormatUVal(GetPageCount());
			return true;
		case CIC_TITLE:
			sVal = GetName();
			return true;
	}
	return CItemVendable::r_WriteVal(pszKey, sVal, pSrc);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

LPCTSTR const CItemMessage::sm_szVerbKeys[] =
{
	"ERASE",
	"PAGE",
	NULL
};

bool CItemMessage::r_Verb(CScript &s, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CItemMessage::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);
	if ( s.IsKey(sm_szVerbKeys[0]) )
	{
		// 1 based pages
		WORD wPage = (s.GetArgStr()[0] && (toupper(s.GetArgStr()[0]) != 'A')) ? static_cast<WORD>(s.GetArgVal()) : 0;
		if ( wPage <= 0 )
		{
			m_sBodyLines.RemoveAll();
			return true;
		}
		else if ( wPage <= GetPageCount() )
		{
			m_sBodyLines.RemoveAt(wPage - 1);
			return true;
		}
	}
	if ( s.IsKeyHead("PAGE", 4) )
	{
		WORD wPage = static_cast<WORD>(ATOI(s.GetKey() + 4));
		if ( wPage <= 0 )
			return false;

		SetPageText(wPage - 1, s.GetArgStr());
		return true;
	}
	return CItemVendable::r_Verb(s, pSrc);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

void CItemMessage::DupeCopy(const CItem *pItem)
{
	ADDTOCALLSTACK("CItemMessage::DupeCopy");
	CItemVendable::DupeCopy(pItem);

	const CItemMessage *pMsgItem = dynamic_cast<const CItemMessage *>(pItem);
	if ( !pMsgItem )
		return;

	m_sAuthor = pMsgItem->m_sAuthor;
	for ( WORD w = 0; w < pMsgItem->GetPageCount(); ++w )
		SetPageText(w, pMsgItem->GetPageText(w));
}

/////////////////////////////////////////////////////////////////////////////
// -CItemMemory

int CItemMemory::FixWeirdness()
{
	ADDTOCALLSTACK("CItemMemory::FixWeirdness");
	int iResultCode = CItem::FixWeirdness();
	if ( iResultCode )
		return iResultCode;

	if ( !IsItemEquipped() || (GetEquipLayer() != LAYER_SPECIAL) || !GetMemoryTypes() )	// has to be a memory of some sort.
		return 0x4222;	// get rid of it.

	CChar *pChar = dynamic_cast<CChar *>(GetParent());
	if ( !pChar )
		return 0x4223;	// get rid of it.

	// Make sure guild/town memories are linked correctly
	if ( IsMemoryTypes(MEMORY_GUILD|MEMORY_TOWN) )
	{
		const CItemStone *pStone = pChar->Guild_Find(static_cast<MEMORY_TYPE>(GetMemoryTypes()));
		if ( !pStone || (pStone->GetUID() != m_uidLink) )
			return 0x4224;	// get rid of it.
		if ( !pStone->GetMember(pChar) )
			return 0x4225;	// get rid of it.
	}

	// Make sure guard memories are linked correctly (this is not an ERROR, just make the item decay on next tick)
	if ( IsMemoryTypes(MEMORY_GUARD) && !m_uidLink.ObjFind() )
	{
		SetAttr(ATTR_DECAY);
		SetTimeout(0);
	}
	return 0;
}

//*********************************************************
// CItemCommCrystal

LPCTSTR const CItemCommCrystal::sm_szLoadKeys[] =
{
	"SPEECH",
	NULL
};

void CItemCommCrystal::OnMoveFrom()
{
	ADDTOCALLSTACK("CItemCommCrystal::OnMoveFrom");
	// Being removed from the top level.
	CSector *pSector = GetTopSector();
	if ( pSector )
		pSector->RemoveListenItem();
}

// Move this item to it's point in the world. (ground/top level)
bool CItemCommCrystal::MoveTo(CPointMap pt, bool bForceFix)
{
	ADDTOCALLSTACK("CItemCommCrystal::MoveTo");
	CSector *pSector = pt.GetSector();
	ASSERT(pSector);
	pSector->AddListenItem();
	return CItem::MoveTo(pt, bForceFix);
}

void CItemCommCrystal::OnHear(LPCTSTR pszCmd, CChar *pSrc)
{
	ADDTOCALLSTACK("CItemCommCrystal::OnHear");
	// IT_COMM_CRYSTAL
	// STATF_COMM_CRYSTAL = if i am on a person.
	TALKMODE_TYPE mode = TALKMODE_SAY;
	for ( size_t i = 0; i < m_Speech.GetCount(); ++i )
	{
		CResourceLink *pLink = m_Speech[i];
		ASSERT(pLink);
		CResourceLock s;
		if ( !pLink->ResourceLock(s) )
			continue;
		TRIGRET_TYPE iRet = OnHearTrigger(s, pszCmd, pSrc, mode);
		if ( (iRet == TRIGRET_ENDIF) || (iRet == TRIGRET_RET_FALSE) )
			continue;
		break;
	}

	// That's prevent @ -1 crash speech :P
	if ( *pszCmd == '@' )
		return;

	if ( m_uidLink.IsValidUID() )
	{
		// I am linked to something ?
		// Transfer the sound.
		CItem *pItem = m_uidLink.ItemFind();
		if ( pItem && pItem->IsType(IT_COMM_CRYSTAL) )
			pItem->Speak(pszCmd);
	}
	else if ( m_Speech.GetCount() <= 0 )
		Speak(pszCmd);
}

void CItemCommCrystal::r_Write(CScript &s)
{
	ADDTOCALLSTACK_INTENSIVE("CItemCommCrystal::r_Write");
	CItemVendable::r_Write(s);
	m_Speech.r_Write(s, "SPEECH");
}

bool CItemCommCrystal::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CItemCommCrystal::r_WriteVal");
	switch ( FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case 0:
			m_Speech.WriteResourceRefList(sVal);
			break;
		default:
			return CItemVendable::r_WriteVal(pszKey, sVal, pSrc);
	}
	return true;
}

bool CItemCommCrystal::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CItemCommCrystal::r_LoadVal");
	switch ( FindTableSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case 0:
			return m_Speech.r_LoadVal(s, RES_SPEECH);
		default:
			return CItemVendable::r_LoadVal(s);
	}
}

void CItemCommCrystal::DupeCopy(const CItem *pItem)
{
	ADDTOCALLSTACK("CItemCommCrystal::DupeCopy");
	CItemVendable::DupeCopy(pItem);

	const CItemCommCrystal *pItemCrystal = dynamic_cast<const CItemCommCrystal *>(pItem);
	if ( !pItemCrystal )
		return;

	m_Speech.Copy(&pItemCrystal->m_Speech);
}

//////////////////////////////////////
// -CItemScript

LPCTSTR const CItemScript::sm_szLoadKeys[] =
{
	NULL
};

LPCTSTR const CItemScript::sm_szVerbKeys[] =
{
	NULL
};

void CItemScript::r_Write(CScript &s)
{
	ADDTOCALLSTACK_INTENSIVE("CItemScript::r_Write");
	CItemVendable::r_Write(s);
}

bool CItemScript::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CItemScript::r_WriteVal");
	return CItemVendable::r_WriteVal(pszKey, sVal, pSrc);
}

bool CItemScript::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CItemScript::r_LoadVal");
	return CItemVendable::r_LoadVal(s);
}

bool CItemScript::r_Verb(CScript &s, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CItemScript::r_Verb");
	return CItemVendable::r_Verb(s, pSrc);
}

void CItemScript::DupeCopy(const CItem *pItem)
{
	ADDTOCALLSTACK("CItemScript::DupeCopy");
	CItemVendable::DupeCopy(pItem);
}
