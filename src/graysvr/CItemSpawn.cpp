#include "graysvr.h"	// predef header.

/////////////////////////////////////////////////////////////////////////////


inline CCharBase *CItemSpawn::TryChar(CREID_TYPE &id)
{
	ADDTOCALLSTACK("CitemSpawn:TryChar");
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
	ADDTOCALLSTACK("CitemSpawn:TryItem");
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
	ADDTOCALLSTACK("CitemSpawn:FixDef");

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
	ADDTOCALLSTACK("CitemSpawn:GetName");
	RESOURCE_ID_BASE rid;
	if ( IsType(IT_SPAWN_ITEM) )
		rid = m_itSpawnItem.m_ItemID;
	else
		rid = m_itSpawnChar.m_CharID;	// name the spawn type

	LPCTSTR pszName = NULL;
	CResourceDef *pDef = g_Cfg.ResourceGetDef(rid);
	if ( pDef != NULL )
		pszName = pDef->GetName();
	if ( pDef == NULL || pszName == NULL || pszName[0] == '\0' )
		pszName = g_Cfg.ResourceGetName(rid);

	return sprintf(pszOut, " (%s)", pszName);
}

CItemSpawn::CItemSpawn(ITEMID_TYPE id, CItemBase *pDef) : CItem(ITEMID_WorldGem, pDef)
{
	ADDTOCALLSTACK("CItemSpawn::CItemSpawn");
	UNREFERENCED_PARAMETER(id);	//forced in CItem(ITEMID_WorldGem , )
}

CItemSpawn::~CItemSpawn()
{
	ADDTOCALLSTACK("CItemSpawn::~CItemSpawn");
}

unsigned char CItemSpawn::GetCount()
{
	ADDTOCALLSTACK("CitemSpawn:GetCount");
	if ( GetType() == IT_SPAWN_CHAR )
		return static_cast<unsigned char>(m_itSpawnChar.m_current);
	else if ( GetType() == IT_SPAWN_ITEM )
	{
		unsigned char iCount = 0;
		unsigned char i = 0;
		while ( m_obj[i].ItemFind() )
		{
			CGrayUID spawn = m_obj[i].ItemFind()->m_uidSpawnItem;
			if ( spawn.IsValidUID() && (spawn == GetUID()) )
				iCount++;
			i++;
		}
		return iCount;
	}
	return 0;
}

void CItemSpawn::GenerateItem(CResourceDef *pDef)
{
	ADDTOCALLSTACK("CitemSpawn:GenerateItem");

	RESOURCE_ID_BASE rid = pDef->GetResourceID();
	ITEMID_TYPE id = static_cast<ITEMID_TYPE>(rid.GetResIndex());

	CItemContainer *pCont = dynamic_cast<CItemContainer *>(GetParent());
	int iCount = pCont ? pCont->ContentCount(rid) : GetCount();
	if ( iCount >= GetAmount() )
		return;

	CItem *pItem = CreateTemplate(id);
	if ( pItem == NULL )
		return;

	int iAmountPile = m_itSpawnItem.m_pile;
	if ( iAmountPile > 1 )
	{
		CItemBase *pItemDef = pItem->Item_GetDef();
		ASSERT(pItemDef);
		if ( pItemDef->IsStackableType() )
		{
			if ( iAmountPile == 0 || iAmountPile > GetAmount() )
				iAmountPile = GetAmount();
			pItem->SetAmount(Calc_GetRandVal(iAmountPile) + 1);
		}
	}

	pItem->SetAttr(m_Attr & (ATTR_OWNED | ATTR_MOVE_ALWAYS));
	pItem->SetDecayTime(g_Cfg.m_iDecay_Item);	// it will decay eventually to be replaced later
	pItem->MoveNearObj(this, m_itSpawnItem.m_DistMax);
	AddObj(pItem->GetUID());
}

void CItemSpawn::GenerateChar(CResourceDef *pDef)
{
	ADDTOCALLSTACK("CitemSpawn:GenerateChar");
	if ( !IsTopLevel() )
		return;

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

	CChar *pChar = CChar::CreateBasic(static_cast<CREID_TYPE>(rid.GetResIndex()));
	if ( !pChar )
		return;

	CPointMap pt = GetTopPoint();
	pChar->NPC_LoadScript(true);
	pChar->StatFlag_Set(STATF_Spawned);
	pChar->MoveTo(pt);

	// Check if the NPC can spawn in this region
	CRegionBase *pRegion = GetTopPoint().GetRegion(REGION_TYPE_AREA);
	if ( !pRegion || (pRegion->IsGuarded() && pChar->Noto_IsEvil()) )
	{
		pChar->Delete();
		return;
	}

	AddObj(pChar->GetUID());
	pChar->NPC_CreateTrigger();		// removed from NPC_LoadScript() and triggered after char placement and attachment to the spawnitem
	pChar->Update();

	size_t iCount = GetTopSector()->GetCharComplexity();
	if ( iCount > g_Cfg.m_iMaxCharComplexity )
		g_Log.Event(LOGL_WARN, "%d chars at %s. Sector too complex!\n", iCount, GetTopSector()->GetBasePoint().WriteUsed());
}

void CItemSpawn::DelObj(CGrayUID uid)
{
	ADDTOCALLSTACK("CitemSpawn:DelObj");
	if ( !uid.IsValidUID() )
		return;

	unsigned char iMax = GetCount();
	for ( unsigned char i = 0; i < iMax; i++ )
	{
		if ( m_obj[i] != uid )
			continue;

		CObjBase *pObj = uid.ObjFind();
		pObj->m_uidSpawnItem.InitUID();

		if ( GetType() == IT_SPAWN_CHAR )				// IT_SPAWN_ITEM uses MORE2 to store how much items to spawn at once, so we must not touch it.
		{
			uid.CharFind()->StatFlag_Clear(STATF_Spawned);
			m_itSpawnChar.m_current--;
		}

		while ( m_obj[i + 1].IsValidUID() )				// searching for any entry higher than this one...
		{
			m_obj[i] = m_obj[i + 1];					// and moving it 1 position to keep values 'together'.
			i++;
		}
		m_obj[i].InitUID();								// Finished moving higher entries (if any) so we free the last entry.

		if ( !g_Serv.IsLoading() )
			ResendTooltip();
		break;
	}
}

void CItemSpawn::AddObj(CGrayUID uid)
{
	ADDTOCALLSTACK("CitemSpawn:AddObj");
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

		CItemSpawn *pPrevSpawn = static_cast<CItemSpawn*>(uid.ObjFind()->m_uidSpawnItem.ItemFind());
		if ( pPrevSpawn )
		{
			if ( pPrevSpawn == this )		// obj already linked to this spawn
				return;
			pPrevSpawn->DelObj(uid);		// obj linked to other spawn, remove the link before proceed
		}
	}

	unsigned char iMax = (GetAmount() > 0) ? static_cast<unsigned char>(GetAmount()) : 1;
	for ( unsigned char i = 0; i < iMax; i++ )
	{
		if ( !m_obj[i].IsValidUID() )
		{
			m_obj[i] = uid;
			if ( !g_Serv.IsLoading() )
			{
				uid.ObjFind()->m_uidSpawnItem = GetUID();
				if ( bIsSpawnChar )
				{
					m_itSpawnChar.m_current++;
					CChar *pChar = uid.CharFind();
					ASSERT(pChar->m_pNPC);
					pChar->StatFlag_Set(STATF_Spawned);
					pChar->m_ptHome = GetTopPoint();
					pChar->m_pNPC->m_Home_Dist_Wander = static_cast<WORD>(m_itSpawnChar.m_DistMax);
					ResendTooltip();
				}
			}
			break;
		}
	}
}

void CItemSpawn::OnTick(bool fExec)
{
	ADDTOCALLSTACK("CitemSpawn:OnTick");

	INT64 iMinutes;
	if ( m_itSpawnChar.m_TimeHiMin <= 0 )
		iMinutes = Calc_GetRandLLVal(30) + 1;
	else
		iMinutes = minimum(m_itSpawnChar.m_TimeHiMin, m_itSpawnChar.m_TimeLoMin) + Calc_GetRandLLVal(abs(m_itSpawnChar.m_TimeHiMin - m_itSpawnChar.m_TimeLoMin));

	if ( iMinutes <= 0 )
		iMinutes = 1;

	if ( !fExec || IsTimerExpired() )
		SetTimeout(iMinutes * 60 * TICK_PER_SEC);	// set time to check again.

	if ( !fExec || m_itSpawnChar.m_current >= GetAmount() )
		return;

	CResourceDef *pDef = FixDef();
	if ( !pDef )
	{
		RESOURCE_ID_BASE rid = IsType(IT_SPAWN_ITEM) ? m_itSpawnItem.m_ItemID : m_itSpawnChar.m_CharID;
		DEBUG_ERR(("Bad Spawn point uid=0%lx, id=%s\n", (DWORD)GetUID(), g_Cfg.ResourceGetName(rid)));
		return;
	}

	if ( IsType(IT_SPAWN_ITEM) )
		GenerateItem(pDef);
	else
		GenerateChar(pDef);
}

void CItemSpawn::KillChildren()
{
	ADDTOCALLSTACK("CitemSpawn:KillChildren");
	WORD iTotal = (GetType() == IT_SPAWN_CHAR) ? static_cast<WORD>(m_itSpawnChar.m_current) : GetAmount();	//m_itSpawnItem doesn't have m_current, it uses MORE2 to set the amount of items spawned in each tick, so i'm using its amount to perform the loop
	if ( iTotal <= 0 )
		return;

	for ( WORD i = 0; i < iTotal; i++ )
	{
		CObjBase *pObj = m_obj[i].ObjFind();
		if ( !pObj )
			continue;
		pObj->m_uidSpawnItem.InitUID();
		pObj->Delete();
		m_obj[i].InitUID();

	}
	m_itSpawnChar.m_current = 0;
	OnTick(false);
}

CCharBase *CItemSpawn::SetTrackID()
{
	ADDTOCALLSTACK("CitemSpawn:SetTrackID");
	if ( !IsType(IT_SPAWN_CHAR) )
		return NULL;

	CCharBase *pCharDef = NULL;
	RESOURCE_ID_BASE rid = m_itSpawnChar.m_CharID;

	if ( rid.GetResType() == RES_CHARDEF )
	{
		CREID_TYPE id = static_cast<CREID_TYPE>(rid.GetResIndex());
		pCharDef = CCharBase::FindCharBase(id);
	}
	if ( pCharDef )
		SetAttr(ATTR_INVIS);
	if ( IsAttr(ATTR_INVIS) )	// They must want it to look like this.
	{
		SetDispID(pCharDef ? pCharDef->m_trackID : ITEMID_TRACK_WISP);
		if ( GetHue() == 0 )
			SetHue(HUE_RED_DARK);	// Indicate to GM's that it is invis.
	}
	return pCharDef;
}

enum ISPW_TYPE
{
	ISPW_ADDOBJ,
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
	"AT",
	"COUNT",
	"DELOBJ",
	"RESET",
	"START",
	"STOP",
	NULL
};

bool CItemSpawn::r_WriteVal(LPCTSTR pszKey, CGString & sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CitemSpawn:r_WriteVal");
	EXC_TRY("WriteVal");
	if ( !strnicmp(pszKey, "at.", 3) )
	{
		pszKey += 3;
		int objIndex = Exp_GetVal(pszKey);
		if ( m_obj[objIndex].ItemFind() )
			return m_obj[objIndex].ItemFind()->r_WriteVal(pszKey, sVal, pSrc);
		else if ( m_obj[objIndex].CharFind() )
			return m_obj[objIndex].CharFind()->r_WriteVal(pszKey, sVal, pSrc);
		return true;
	}
	else if ( !strnicmp(pszKey, "count", 5) )
	{
		sVal.FormatVal(GetCount());
		return true;
	}
	EXC_CATCH;
	return CItem::r_WriteVal(pszKey, sVal, pSrc);
}

bool CItemSpawn::r_LoadVal(CScript & s)
{
	ADDTOCALLSTACK("CitemSpawn:r_LoadVal");
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

void  CItemSpawn::r_Write(CScript & s)
{
	ADDTOCALLSTACK("CitemSpawn:r_Write");
	EXC_TRY("Write");
	CItem::r_Write(s);
	WORD iTotal = GetCount();
	if ( iTotal <= 0 )
		return;

	for ( WORD i = 0; i < iTotal; i++ )
	{
		if ( !m_obj[i].IsValidUID() )
			continue;
		CObjBase *pObj = m_obj[i].ObjFind();
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
//
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
				pItemFruit->MoveToDecay(GetTopPoint(), 10 * g_Cfg.m_iDecay_Item);
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
		for ( size_t i = 0; i < m_Pins.GetCount(); i++ )
		{
			if ( m_Pins[i].m_x != pItemMap->m_Pins[i].m_x )
				return false;
			if ( m_Pins[i].m_y != pItemMap->m_Pins[i].m_y )
				return false;
		}
	}

	return CItemVendable::IsSameType(pObj);
}

bool CItemMap::r_LoadVal(CScript & s)	// load an item script
{
	ADDTOCALLSTACK("CItemMap::r_LoadVal");
	EXC_TRY("LoadVal");
	if ( s.IsKeyHead("PIN", 3) )
	{
		CPointMap pntTemp;
		pntTemp.Read(s.GetArgStr());
		CMapPinRec pin(pntTemp.m_x, pntTemp.m_y);
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
		size_t i = Exp_GetVal(pszKey) - 1;
		if ( m_Pins.IsValidIndex(i) )
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

void CItemMap::r_Write(CScript & s)
{
	ADDTOCALLSTACK_INTENSIVE("CItemMap::r_Write");
	CItemVendable::r_Write(s);
	for ( size_t i = 0; i < m_Pins.GetCount(); i++ )
		s.WriteKeyFormat("PIN", "%i,%i", m_Pins[i].m_x, m_Pins[i].m_y);
}

void CItemMap::DupeCopy(const CItem *pItem)
{
	ADDTOCALLSTACK("CItemMap::DupeCopy");
	CItemVendable::DupeCopy(pItem);

	const CItemMap *pMapItem = dynamic_cast<const CItemMap *>(pItem);
	if ( pMapItem == NULL )
		return;
	m_Pins.Copy(&(pMapItem->m_Pins));
}

/////////////////////////////////////////////////////////////////////////////
// -CItemMessage

void CItemMessage::r_Write(CScript & s)
{
	ADDTOCALLSTACK_INTENSIVE("CItemMessage::r_Write");
	CItemVendable::r_Write(s);
	s.WriteKey("AUTHOR", m_sAuthor);

	// Store the message body lines. MAX_BOOK_PAGES
	TemporaryString pszTemp;
	for ( size_t i = 0; i < GetPageCount(); ++i )
	{
		sprintf(pszTemp, "BODY.%" FMTSIZE_T, i);
		LPCTSTR pszText = GetPageText(i);
		s.WriteKey(pszTemp, pszText != NULL ? pszText : "");
	}
}

LPCTSTR const CItemMessage::sm_szLoadKeys[CIC_QTY+1] =	// static
{
	"AUTHOR",
	"BODY",
	"PAGES",	// (W)
	"TITLE",	// same as name
	NULL,
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
		size_t iPage = Exp_GetVal(pszKey);
		if ( m_sBodyLines.IsValidIndex(iPage) == false )
			return false;
		sVal = *m_sBodyLines[iPage];
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
			sVal.FormatVal(m_sBodyLines.GetCount());
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
	NULL,
};

bool CItemMessage::r_Verb(CScript & s, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CItemMessage::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);
	if ( s.IsKey(sm_szVerbKeys[0]) )
	{
		// 1 based pages
		size_t iPage = (s.GetArgStr()[0] && toupper(s.GetArgStr()[0]) != 'A') ? s.GetArgVal() : 0;
		if ( iPage <= 0 )
		{
			m_sBodyLines.RemoveAll();
			return true;
		}
		else if ( iPage <= m_sBodyLines.GetCount() )
		{
			m_sBodyLines.RemoveAt(iPage - 1);
			return true;
		}
	}
	if ( s.IsKeyHead("PAGE", 4) )
	{
		size_t iPage = ATOI(s.GetKey() + 4);
		if ( iPage <= 0 )
			return false;

		SetPageText(iPage - 1, s.GetArgStr());
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
	if ( pMsgItem == NULL )
		return;

	m_sAuthor = pMsgItem->m_sAuthor;
	for ( size_t i = 0; i < pMsgItem->GetPageCount(); i++ )
		SetPageText(i, pMsgItem->GetPageText(i));
}

/////////////////////////////////////////////////////////////////////////////
// -CItemMemory

CItemStone *CItemMemory::Guild_GetLink()
{
	ADDTOCALLSTACK("CItemMemory::Guild_GetLink");
	if ( !IsMemoryTypes(MEMORY_TOWN|MEMORY_GUILD) )
		return NULL;
	return dynamic_cast<CItemStone*>(m_uidLink.ItemFind());
}

bool CItemMemory::Guild_IsAbbrevOn() const
{
	ADDTOCALLSTACK("CItemMemory::Guild_IsAbbrevOn");
	return (m_itEqMemory.m_Action != 0);
}

void CItemMemory::Guild_SetAbbrev(bool fAbbrevShow)
{
	ADDTOCALLSTACK("CItemMemory::Guild_SetAbbrev");
	m_itEqMemory.m_Action = fAbbrevShow;
}

WORD CItemMemory::Guild_GetVotes() const
{
	ADDTOCALLSTACK("CItemMemory::Guild_GetVotes");
	return m_itEqMemory.m_Skill;
}

void CItemMemory::Guild_SetVotes(WORD wVotes)
{
	ADDTOCALLSTACK("CItemMemory::Guild_SetVotes");
	m_itEqMemory.m_Skill = wVotes;
}

int CItemMemory::Guild_SetLoyalTo(CGrayUID uid)
{
	ADDTOCALLSTACK("CItemMemory::Guild_SetLoyalTo");
	// Some other place checks to see if this is a valid member.
	return GetTagDefs()->SetNum("LoyalTo", (DWORD)uid, false);
}

CGrayUID CItemMemory::Guild_GetLoyalTo() const
{
	ADDTOCALLSTACK("CItemMemory::Guild_GetLoyalTo");
	CItemMemory *pObj = const_cast<CItemMemory *>(this);
	CGrayUID iUid(static_cast<DWORD>(pObj->GetTagDefs()->GetKeyNum("LoyalTo", true)));
	return iUid;
}

int CItemMemory::Guild_SetTitle(LPCTSTR pszTitle)
{
	ADDTOCALLSTACK("CItemMemory::Guild_SetTitle");
	return GetTagDefs()->SetStr("Title", false, pszTitle);
}

LPCTSTR CItemMemory::Guild_GetTitle() const
{
	ADDTOCALLSTACK("CItemMemory::Guild_GetTitle");
	return m_TagDefs.GetKeyStr("Title", false);
}

int CItemMemory::FixWeirdness()
{
	ADDTOCALLSTACK("CItemMemory::FixWeirdness");
	int iResultCode = CItem::FixWeirdness();
	if ( iResultCode )
		return iResultCode;

	if ( !IsItemEquipped() || GetEquipLayer() != LAYER_SPECIAL || !GetMemoryTypes() )	// has to be a memory of some sort.
	{
		iResultCode = 0x4222;
		return iResultCode;	// get rid of it.
	}

	CChar *pChar = dynamic_cast<CChar*>(GetParent());
	if ( !pChar )
	{
		iResultCode = 0x4223;
		return iResultCode;	// get rid of it.
	}

	// If it is my guild make sure I am linked to it correctly !
	if ( IsMemoryTypes(MEMORY_GUILD|MEMORY_TOWN) )
	{
		const CItemStone *pStone = pChar->Guild_Find(static_cast<MEMORY_TYPE>(GetMemoryTypes()));
		if ( !pStone || pStone->GetUID() != m_uidLink )
		{
			iResultCode = 0x4224;
			return iResultCode;	// get rid of it.
		}
		if ( !pStone->GetMember(pChar) )
		{
			iResultCode = 0x4225;
			return iResultCode;	// get rid of it.
		}
	}
	return 0;
}

//*********************************************************
// CItemCommCrystal

LPCTSTR const CItemCommCrystal::sm_szLoadKeys[] =
{
	"SPEECH",
	NULL,
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
	for ( size_t i = 0; i < m_Speech.GetCount(); i++ )
	{
		CResourceLink *pLink = m_Speech[i];
		ASSERT(pLink);
		CResourceLock s;
		if ( !pLink->ResourceLock(s) )
			continue;
		TRIGRET_TYPE iRet = OnHearTrigger(s, pszCmd, pSrc, mode);
		if ( iRet == TRIGRET_ENDIF || iRet == TRIGRET_RET_FALSE )
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

void CItemCommCrystal::r_Write(CScript & s)
{
	ADDTOCALLSTACK_INTENSIVE("CItemCommCrystal::r_Write");
	CItemVendable::r_Write(s);
	m_Speech.r_Write(s, "SPEECH");
}

bool CItemCommCrystal::r_WriteVal(LPCTSTR pszKey, CGString & sVal, CTextConsole *pSrc)
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

bool CItemCommCrystal::r_LoadVal(CScript & s)
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
	NULL,
};

LPCTSTR const CItemScript::sm_szVerbKeys[] =
{
	NULL,
};

void CItemScript::r_Write(CScript & s)
{
	ADDTOCALLSTACK_INTENSIVE("CItemScript::r_Write");
	CItemVendable::r_Write(s);
}

bool CItemScript::r_WriteVal(LPCTSTR pszKey, CGString & sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CItemScript::r_WriteVal");
	return CItemVendable::r_WriteVal(pszKey, sVal, pSrc);
}

bool CItemScript::r_LoadVal(CScript & s)
{
	ADDTOCALLSTACK("CItemScript::r_LoadVal");
	return CItemVendable::r_LoadVal(s);
}

bool CItemScript::r_Verb(CScript & s, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CItemScript::r_Verb");
	return CItemVendable::r_Verb(s, pSrc);
}

void CItemScript::DupeCopy(const CItem *pItem)
{
	ADDTOCALLSTACK("CItemScript::DupeCopy");
	CItemVendable::DupeCopy(pItem);
}
