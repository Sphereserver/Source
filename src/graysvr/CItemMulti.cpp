#include "graysvr.h"	// predef header.

CItemMulti::CItemMulti(ITEMID_TYPE id, CItemBase *pItemDef) : CItem(id, pItemDef)		// CItemBaseMulti
{
	CItemBaseMulti *pItemBase = static_cast<CItemBaseMulti *>(Base_GetDef());
	m_pRegion = NULL;
	m_shipSpeed.period = pItemBase->m_shipSpeed.period;
	m_shipSpeed.tiles = pItemBase->m_shipSpeed.tiles;
	m_SpeedMode = pItemBase->m_SpeedMode;
}

CItemMulti::~CItemMulti()
{
	MultiUnRealizeRegion();		// unrealize before remove from ground
	DeletePrepare();			// must remove early because virtuals will fail in child destructor

	if ( !m_pRegion )
		return;

	// Remove all the accessory junk
	// NOTE: Assume the multi is already been removed from top level
	CWorldSearch Area(m_pRegion->m_pt, Multi_GetMaxDist());
	for (;;)
	{
		CItem *pItem = Area.GetItem();
		if ( !pItem )
			break;
		if ( pItem == this )	// this must be deleted seperately
			continue;
		if ( !Multi_IsPartOf(pItem) )
			continue;
		pItem->Delete();
		Area.RestartSearch();	// the removed item will mess the loop sequence, so restart the loop
	}
	delete m_pRegion;
}

const CItemBaseMulti *CItemMulti::Multi_GetDef(ITEMID_TYPE id)	// static
{
	ADDTOCALLSTACK("CItemMulti::Multi_GetDef");
	return static_cast<const CItemBaseMulti *>(CItemBase::FindItemBase(id));
}

int CItemMulti::Multi_GetMaxDist() const
{
	ADDTOCALLSTACK("CItemMulti::Multi_GetMaxDist");
	const CItemBaseMulti *pMultiDef = Multi_GetDef();
	return pMultiDef ? pMultiDef->GetMaxDist() : 0;
}

void CItemMulti::MultiRealizeRegion()
{
	ADDTOCALLSTACK("CItemMulti::MultiRealizeRegion");
	if ( !IsTopLevel() )
		return;

	const CItemBaseMulti *pMultiDef = Multi_GetDef();
	if ( !pMultiDef )
	{
		DEBUG_ERR(("Bad multi type 0%x (UID=0%" FMTDWORDH ")\n", GetID(), static_cast<DWORD>(GetUID())));
		return;
	}

	if ( !m_pRegion )
	{
		RESOURCE_ID rid;
		rid.SetPrivateUID(GetUID());
		m_pRegion = new CRegionWorld(rid);
	}

	// Get background region
	CPointMap pt = GetTopPoint();
	const CRegionWorld *pRegionBack = static_cast<CRegionWorld *>(pt.GetRegion(REGION_TYPE_AREA));
	if ( !pRegionBack || (pRegionBack == m_pRegion) )
		return;

	// Create the new region rectangle
	CRectMap rect;
	reinterpret_cast<CGRect &>(rect) = pMultiDef->m_rect;
	rect.OffsetRect(pt.m_x, pt.m_y);
	rect.m_map = pt.m_map;

	m_pRegion->SetRegionRect(rect);
	m_pRegion->m_pt = pt;

	DWORD dwFlags = pMultiDef->m_dwRegionFlags;
	if ( !IsType(IT_SHIP) )		// ships are multis that can be moved, so to avoid region flag conflicts they won't inherit parent region flags
		dwFlags |= pRegionBack->GetRegionFlags();
	m_pRegion->SetRegionFlags(dwFlags);

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "%s (%s)", pRegionBack->GetName(), GetName());
	m_pRegion->SetName(pszTemp);

	m_pRegion->RealizeRegion();
}

void CItemMulti::MultiUnRealizeRegion()
{
	ADDTOCALLSTACK("CItemMulti::MultiUnRealizeRegion");
	EXC_TRY("MultiUnRealizeRegion");
	if ( !m_pRegion )
		return;

	m_pRegion->UnRealizeRegion();

	// Move all chars in this region to background region
	CWorldSearch Area(m_pRegion->m_pt, Multi_GetMaxDist());
	for (;;)
	{
		CChar *pChar = Area.GetChar();
		if ( !pChar )
			break;
		if ( pChar->m_pArea != m_pRegion )
			continue;
		pChar->MoveToRegionReTest(REGION_TYPE_AREA);
	}
	EXC_CATCH;
}

bool CItemMulti::Multi_CreateComponent(ITEMID_TYPE id, signed short x, signed short y, signed char z, DWORD dwKeyCode)
{
	ADDTOCALLSTACK("CItemMulti::Multi_CreateComponent");
	CItem *pItem = CreateTemplate(id);
	ASSERT(pItem);

	CPointMap pt = GetTopPoint();
	pt.m_x += x;
	pt.m_y += y;
	pt.m_z += z;

	bool fNeedKey = false;
	switch ( pItem->GetType() )
	{
		case IT_KEY:
		case IT_SIGN_GUMP:
		case IT_SHIP_TILLER:
			pItem->m_itKey.m_lockUID.SetPrivateUID(dwKeyCode);	// set key id for the key/sign
			m_uidLink.SetPrivateUID(pItem->GetUID());
			fNeedKey = true;
			break;
		case IT_DOOR:
			pItem->SetType(IT_DOOR_LOCKED);
			fNeedKey = true;
			break;
		case IT_PORTCULIS:
			pItem->SetType(IT_PORT_LOCKED);
			fNeedKey = true;
			break;
		case IT_CONTAINER:
			pItem->SetType(IT_CONTAINER_LOCKED);
			fNeedKey = true;
			break;
		case IT_SHIP_SIDE:
			pItem->SetType(IT_SHIP_SIDE_LOCKED);
			break;
		case IT_SHIP_HOLD:
			pItem->SetType(IT_SHIP_HOLD_LOCK);
			break;
	}

	if ( pItem->GetHue() == HUE_DEFAULT )
		pItem->SetHue(GetHue());

	pItem->SetAttr(ATTR_MOVE_NEVER | (m_Attr & (ATTR_MAGIC|ATTR_INVIS)));
	pItem->m_uidLink = GetUID();

	if ( pItem->IsTypeLockable() )
	{
		pItem->m_itContainer.m_lockUID.SetPrivateUID(dwKeyCode);	// set key id for the door/key/sign
		pItem->m_itContainer.m_lock_complexity = 10000;				// never pickable
	}

	pItem->MoveToUpdate(pt);
	OnComponentCreate(pItem);
	return fNeedKey;
}

void CItemMulti::Multi_Create(CChar *pChar)
{
	ADDTOCALLSTACK("CItemMulti::Multi_Create");
	// Create multi components
	// ARGS:
	//  dwKeyCode = set the key code for the doors/sides to this
	// NOTE: 
	//  This can only be done after multi item is already placed on world

	const CItemBaseMulti *pMultiDef = Multi_GetDef();
	if ( !pMultiDef || !IsTopLevel() )
		return;

	bool fNeedKey = false;
	for ( size_t i = 0; i < pMultiDef->m_Components.GetCount(); ++i )
	{
		const CItemBaseMulti::CMultiComponentItem &component = pMultiDef->m_Components.ElementAt(i);
		fNeedKey |= Multi_CreateComponent(component.m_id, component.m_dx, component.m_dy, component.m_dz, GetUID());
	}

	Multi_GetSign();	// set the m_uidLink

	if ( pChar )
	{
		m_itShip.m_UIDCreator = pChar->GetUID();
		pChar->Memory_AddObjTypes(this, MEMORY_GUARD);

		if ( fNeedKey )
		{
			CItem *pKey = CreateScript(IsAttr(ATTR_MAGIC) ? ITEMID_KEY_MAGIC : ITEMID_KEY_COPPER, pChar);
			if ( pKey )
			{
				pKey->SetType(IT_KEY);
				pKey->SetAttr(g_Cfg.m_fAutoNewbieKeys ? ATTR_NEWBIE|ATTR_MAGIC : ATTR_MAGIC);
				pKey->m_itKey.m_lockUID.SetPrivateUID(GetUID());
				pKey->m_uidLink = GetUID();

				// Put primary key on backpack
				pChar->GetContainerCreate(LAYER_PACK)->ContentAdd(pKey);

				// Put secondary key on bankbox
				pKey = CreateDupeItem(pKey);
				pChar->GetContainerCreate(LAYER_BANKBOX)->ContentAdd(pKey);
				pChar->SysMessageDefault(DEFMSG_MSG_KEY_DUPEBANK);
			}
		}
	}
}

bool CItemMulti::Multi_IsPartOf(const CItem *pItem) const
{
	ADDTOCALLSTACK("CItemMulti::Multi_IsPartOf");
	if ( !pItem )
		return false;
	if ( pItem == this )
		return true;
	return (pItem->m_uidLink == GetUID());
}

CItem *CItemMulti::Multi_FindItemType(IT_TYPE type) const
{
	ADDTOCALLSTACK("CItemMulti::Multi_FindItemType");
	// Find an multi component with given type
	if ( !IsTopLevel() )
		return NULL;

	CWorldSearch Area(GetTopPoint(), Multi_GetMaxDist());
	for (;;)
	{
		CItem *pItem = Area.GetItem();
		if ( !pItem )
			break;
		if ( !Multi_IsPartOf(pItem) || !pItem->IsType(type) )
			continue;
		return pItem;
	}
	return NULL;
}

CItem *CItemMulti::Multi_GetSign()
{
	ADDTOCALLSTACK("CItemMulti::Multi_GetSign");
	// Get multi sign (houses) or tiller (ships)
	CItem *pItem = m_uidLink.ItemFind();
	if ( !pItem )
	{
		pItem = Multi_FindItemType(IsType(IT_SHIP) ? IT_SHIP_TILLER : IT_SIGN_GUMP);
		if ( !pItem )
			return this;
		m_uidLink = pItem->GetUID();
	}
	return pItem;
}

void CItemMulti::OnHearRegion(LPCTSTR pszCmd, CChar *pSrc)
{
	ADDTOCALLSTACK("CItemMulti::OnHearRegion");
	// Someone is saying something inside multi region
	// Check if the multi have an speech filter script

	const CItemBaseMulti *pMultiDef = Multi_GetDef();
	if ( !pMultiDef )
		return;

	TALKMODE_TYPE mode = TALKMODE_SAY;
	for ( size_t i = 0; i < pMultiDef->m_Speech.GetCount(); ++i )
	{
		CResourceLink *pLink = pMultiDef->m_Speech[i];
		ASSERT(pLink);
		CResourceLock s;
		if ( !pLink->ResourceLock(s) )
			continue;
		TRIGRET_TYPE tr = OnHearTrigger(s, pszCmd, pSrc, mode);
		if ( (tr == TRIGRET_ENDIF) || (tr == TRIGRET_RET_FALSE) )
			continue;
		break;
	}
}

bool CItemMulti::OnTick()
{
	ADDTOCALLSTACK("CItemMulti::OnTick");
	return true;
}

void CItemMulti::OnMoveFrom()
{
	ADDTOCALLSTACK("CItemMulti::OnMoveFrom");
	// Multi item is being removed from background region
	// Or might just get moved (ships)

	ASSERT(m_pRegion);
	m_pRegion->UnRealizeRegion();
}

bool CItemMulti::MoveTo(CPointMap pt, bool fForceFix)
{
	ADDTOCALLSTACK("CItemMulti::MoveTo");
	// Move multi to given pt on world
	if ( !CItem::MoveTo(pt, fForceFix) )
		return false;

	MultiRealizeRegion();
	return true;
}

enum
{
	SHV_MULTICREATE,
	SHV_QTY
};

const LPCTSTR CItemMulti::sm_szVerbKeys[SHV_QTY + 1] =
{
	"MULTICREATE",
	NULL
};

bool CItemMulti::r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef)
{
	ADDTOCALLSTACK("CItemMulti::r_GetRef");

	if ( !strnicmp(pszKey, "REGION", 6) )
	{
		pszKey += 6;
		SKIP_SEPARATORS(pszKey);
		pRef = m_pRegion;
		return true;
	}
	return CItem::r_GetRef(pszKey, pRef);
}

bool CItemMulti::r_Verb(CScript &s, CTextConsole *pSrc)		// execute command from script
{
	ADDTOCALLSTACK("CItemMulti::r_Verb");
	EXC_TRY("Verb");

	int index = FindTableSorted(s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1);
	switch ( index )
	{
		case SHV_MULTICREATE:
		{
			Multi_Create(static_cast<CGrayUID>(s.GetArgVal()).CharFind());
			return true;
		}
		default:
			return CItem::r_Verb(s, pSrc);
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return true;
}

void CItemMulti::r_Write(CScript &s)
{
	ADDTOCALLSTACK_INTENSIVE("CItemMulti::r_Write");
	CItem::r_Write(s);
	if ( m_pRegion )
		m_pRegion->r_WriteBody(s, "REGION.");
}

bool CItemMulti::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CItemMulti::r_WriteVal");
	if ( !strnicmp(pszKey, "COMP", 4) )
	{
		pszKey += 4;
		const CItemBaseMulti *pMultiDef = Multi_GetDef();

		if ( *pszKey == '\0' )
		{
			sVal.FormatVal(pMultiDef->m_Components.GetCount());
			return true;
		}
		else if ( *pszKey == '.' )
		{
			++pszKey;
			size_t iQty = Exp_GetVal(pszKey);
			if ( !pMultiDef->m_Components.IsValidIndex(iQty) )
				return false;

			SKIP_SEPARATORS(pszKey);
			CItemBaseMulti::CMultiComponentItem component = pMultiDef->m_Components.GetAt(iQty);

			if ( !strnicmp(pszKey, "ID", 2) )
				sVal.FormatVal(component.m_id);
			else if ( !strnicmp(pszKey, "DX", 2) )
				sVal.FormatVal(component.m_dx);
			else if ( !strnicmp(pszKey, "DY", 2) )
				sVal.FormatVal(component.m_dy);
			else if ( !strnicmp(pszKey, "DZ", 2) )
				sVal.FormatVal(component.m_dz);
			else if ( !strnicmp(pszKey, "D", 1) )
				sVal.Format("%hd,%hd,%hhd", component.m_dx, component.m_dy, component.m_dz);
			else
				sVal.Format("%d,%hd,%hd,%hhd", component.m_id, component.m_dx, component.m_dy, component.m_dz);
			return true;
		}
		return false;
	}
	return CItem::r_WriteVal(pszKey, sVal, pSrc);
}

bool CItemMulti::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CItemMulti::r_LoadVal");
	EXC_TRY("LoadVal");
	if ( s.IsKeyHead("REGION.", 7) )
	{
		if ( !IsTopLevel() )
		{
			MoveTo(GetTopPoint());	// put item on the ground here
			Update();
		}
		ASSERT(m_pRegion);
		CScript script(s.GetKey() + 7, s.GetArgStr());
		return m_pRegion->r_LoadVal(script);
	}
	return CItem::r_LoadVal(s);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

void CItemMulti::DupeCopy(const CItem *pItem)
{
	ADDTOCALLSTACK("CItemMulti::DupeCopy");
	CItem::DupeCopy(pItem);
}
