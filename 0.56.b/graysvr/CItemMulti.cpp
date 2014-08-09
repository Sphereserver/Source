//
// CItemMulti.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "graysvr.h"	// predef header.

/////////////////////////////////////////////////////////////////////////////

CItemMulti::CItemMulti( ITEMID_TYPE id, CItemBase * pItemDef ) :	// CItemBaseMulti
	CItem( id, pItemDef )
{
	m_pRegion = NULL;
}

CItemMulti::~CItemMulti()
{
	MultiUnRealizeRegion();	// unrealize before removed from ground.
	DeletePrepare();	// Must remove early because virtuals will fail in child destructor.
	// NOTE: ??? This is dangerous to iterators. The "next" item may no longer be valid !

	// Attempt to remove all the accessory junk.
	// NOTE: assume we have already been removed from Top Level

	if ( ! m_pRegion )
		return;

	CWorldSearch Area( m_pRegion->m_pt, Multi_GetMaxDist() );	// largest area.
	Area.SetSearchSquare( true );
	for (;;)
	{
		CItem * pItem = Area.GetItem();
		if ( pItem == NULL )
			break;
		if ( pItem == this )	// this gets deleted seperately.
			continue;
		if ( ! Multi_IsPartOf( pItem ))
			continue;
		pItem->Delete();	// delete the key id for the door/key/sign.
	}

	delete m_pRegion;
}

int CItemMulti::Multi_GetMaxDist() const
{
	ADDTOCALLSTACK("CItemMulti::Multi_GetMaxDist");
	const CItemBaseMulti * pMultiDef = Multi_GetDef();
	if ( pMultiDef == NULL )
		return( 0 );
	return( pMultiDef->GetMaxDist());
}

const CItemBaseMulti * CItemMulti::Multi_GetDef( ITEMID_TYPE id ) // static
{
	ADDTOCALLSTACK("CItemMulti::Multi_GetDef");
	return( dynamic_cast <const CItemBaseMulti *> ( CItemBase::FindItemBase(id)));
}

bool CItemMulti::MultiRealizeRegion()
{
	ADDTOCALLSTACK("CItemMulti::MultiRealizeRegion");
	// Add/move a region for the multi so we know when we are in it.
	// RETURN: ignored.

	if ( !IsTopLevel() )
		return false;

	const CItemBaseMulti * pMultiDef = Multi_GetDef();
	if ( pMultiDef == NULL )
	{
		DEBUG_ERR(( "Bad Multi type 0%x, uid=0%lx\n", GetID(), (DWORD) GetUID()));
		return false;
	}

	if ( m_pRegion == NULL )
	{
		RESOURCE_ID rid;
		rid.SetPrivateUID( GetUID());
		m_pRegion = new CRegionWorld( rid );
	}

	// Get Background region.
	CPointMap pt = GetTopPoint();
	const CRegionWorld * pRegionBack = dynamic_cast <CRegionWorld*> (pt.GetRegion( REGION_TYPE_AREA ));
	ASSERT( pRegionBack );
	ASSERT( pRegionBack != m_pRegion );

	// Create the new region rectangle.
	CRectMap rect;
	reinterpret_cast<CGRect&>(rect) = pMultiDef->m_rect;
	rect.m_map = pt.m_map;
	rect.OffsetRect( pt.m_x, pt.m_y );
	m_pRegion->SetRegionRect( rect );
	m_pRegion->m_pt = pt;

	DWORD dwFlags;
	if ( IsType(IT_SHIP))
	{
		dwFlags = REGION_FLAG_SHIP;
	}
	else
	{
		// Houses get some of the attribs of the land around it.
		dwFlags = pRegionBack->GetRegionFlags();
	}
	dwFlags |= pMultiDef->m_dwRegionFlags;
	m_pRegion->SetRegionFlags( dwFlags );

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "%s (%s)", static_cast<LPCTSTR>(pRegionBack->GetName()), static_cast<LPCTSTR>(GetName()));
	m_pRegion->SetName(pszTemp);

	return m_pRegion->RealizeRegion();
}

void CItemMulti::MultiUnRealizeRegion()
{
	ADDTOCALLSTACK("CItemMulti::MultiUnRealizeRegion");
	if ( m_pRegion == NULL )
		return;

	m_pRegion->UnRealizeRegion();

	// find all creatures in the region and remove this from them.
	CWorldSearch Area( m_pRegion->m_pt, Multi_GetMaxDist() );
	Area.SetSearchSquare(true);
	for (;;)
	{
		CChar * pChar = Area.GetChar();
		if ( pChar == NULL )
			break;
		if ( pChar->m_pArea != m_pRegion )
			continue;
		pChar->MoveToRegionReTest(REGION_TYPE_AREA);
	}
}

bool CItemMulti::Multi_CreateComponent( ITEMID_TYPE id, int dx, int dy, int dz, DWORD dwKeyCode )
{
	ADDTOCALLSTACK("CItemMulti::Multi_CreateComponent");
	CItem * pItem = CreateTemplate( id );
	ASSERT(pItem);

	CPointMap pt = GetTopPoint();
	pt.m_x += dx;
	pt.m_y += dy;
	pt.m_z += dz;

	bool fNeedKey = false;

	switch ( pItem->GetType() )
	{
		case IT_KEY:	// it will get locked down with the house ?
		case IT_SIGN_GUMP:
		case IT_SHIP_TILLER:
			pItem->m_itKey.m_lockUID.SetPrivateUID( dwKeyCode );	// Set the key id for the key/sign.
			m_uidLink.SetPrivateUID(pItem->GetUID());
			fNeedKey = true;
			break;
		case IT_DOOR:
			pItem->SetType(IT_DOOR_LOCKED);
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
		default:
			break;
	}

	if ( pItem->GetHue() == HUE_DEFAULT )
		pItem->SetHue( GetHue());

	pItem->SetAttr( ATTR_MOVE_NEVER | (m_Attr&(ATTR_MAGIC|ATTR_INVIS)));
	pItem->m_uidLink = GetUID();	// lock it down with the structure.

	if ( pItem->IsTypeLockable() || pItem->IsTypeLocked())
	{
		pItem->m_itContainer.m_lockUID.SetPrivateUID( dwKeyCode );	// Set the key id for the door/key/sign.
		pItem->m_itContainer.m_lock_complexity = 10000;	// never pickable.
	}

	pItem->MoveTo( pt );
	OnComponentCreate( pItem );
	return( fNeedKey );
}

void CItemMulti::Multi_Create( CChar * pChar, DWORD dwKeyCode )
{
	ADDTOCALLSTACK("CItemMulti::Multi_Create");
	// Create house or Ship extra stuff.
	// ARGS:
	//  dwKeyCode = set the key code for the doors/sides to this in case it's a drydocked ship.
	// NOTE: 
	//  This can only be done after the house is given location.

	const CItemBaseMulti * pMultiDef = Multi_GetDef();
	// We are top level.
	if ( pMultiDef == NULL ||
		! IsTopLevel())
		return;

	if ( dwKeyCode == UID_CLEAR )
		dwKeyCode = GetUID();

	// ??? SetTimeout( GetDecayTime()); house decay ?

	bool fNeedKey = false;
	size_t iQty = pMultiDef->m_Components.GetCount();
	for ( size_t i = 0; i < iQty; i++ )
	{
		const CItemBaseMulti::CMultiComponentItem & component = pMultiDef->m_Components.ElementAt(i);

		fNeedKey |= Multi_CreateComponent(static_cast<ITEMID_TYPE>(component.m_id),
										  component.m_dx,
										  component.m_dy,
										  component.m_dz,
										  dwKeyCode );
	}

	CItem * pKey = NULL;
	if ( fNeedKey )
	{
		// Create the key to the door.
		ITEMID_TYPE id = IsAttr(ATTR_MAGIC) ? ITEMID_KEY_MAGIC : ITEMID_KEY_COPPER ;
		pKey = CreateScript( id, pChar );
		ASSERT(pKey);
		pKey->SetType(IT_KEY);
		if ( g_Cfg.m_fAutoNewbieKeys )
			pKey->SetAttr(ATTR_NEWBIE);
		pKey->SetAttr(m_Attr&ATTR_MAGIC);
		pKey->m_itKey.m_lockUID.SetPrivateUID( dwKeyCode );
		pKey->m_uidLink = GetUID();	
	}

	Multi_GetSign();	// set the m_uidLink

	if ( pChar != NULL )
	{
		m_itShip.m_UIDCreator = pChar->GetUID();
		pChar->Memory_AddObjTypes( this, MEMORY_GUARD );

		if ( pKey )
		{
			// Put in your pack
			pChar->GetPackSafe()->ContentAdd( pKey );

			// Put dupe key in the bank.
			pKey = CreateDupeItem( pKey );
			pChar->GetBank()->ContentAdd( pKey );
			pChar->SysMessageDefault(DEFMSG_KEY_DUPEBANK);
		}
	}
}

bool CItemMulti::Multi_IsPartOf( const CItem * pItem ) const
{
	ADDTOCALLSTACK("CItemMulti::Multi_IsPartOf");
	// Assume it is in my area test already.
	// IT_MULTI
	// IT_SHIP
	ASSERT( pItem );
	if ( pItem == this )
		return( true );
	return ( pItem->m_uidLink == GetUID());
}

CItem * CItemMulti::Multi_FindItemComponent( int iComp ) const
{
	ADDTOCALLSTACK("CItemMulti::Multi_FindItemComponent");
	UNREFERENCED_PARAMETER(iComp);
	const CItemBaseMulti * pMultiDef = Multi_GetDef();
	if ( pMultiDef == NULL )
		return( NULL );

	return( NULL );
}

CItem * CItemMulti::Multi_FindItemType( IT_TYPE type ) const
{
	ADDTOCALLSTACK("CItemMulti::Multi_FindItemType");
	// Find a part of this multi nearby.
	if ( ! IsTopLevel())
		return( NULL );

	CWorldSearch Area( GetTopPoint(), Multi_GetMaxDist() );
	Area.SetSearchSquare(true);
	for (;;)
	{
		CItem * pItem = Area.GetItem();
		if ( pItem == NULL )
			return( NULL );
		if ( ! Multi_IsPartOf( pItem ))
			continue;
		if ( pItem->IsType( type ))
			return( pItem );
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
	// Being removed from the top level.
	// Might just be moving.

	ASSERT( m_pRegion );
	m_pRegion->UnRealizeRegion();
}

bool CItemMulti::MoveTo( CPointMap pt, bool bForceFix ) // Put item on the ground here.
{
	ADDTOCALLSTACK("CItemMulti::MoveTo");
	// Move this item to it's point in the world. (ground/top level)
	if ( ! CItem::MoveTo(pt))
		return false;

	// Multis need special region info to track when u are inside them.
	// Add new region info.
	MultiRealizeRegion();
	return( true );
}

CItem * CItemMulti::Multi_GetSign()
{
	ADDTOCALLSTACK("CItemMulti::Multi_GetSign");
	// Get my sign or tiller link.
	CItem * pTiller = m_uidLink.ItemFind();
	if ( pTiller == NULL )
	{
		pTiller = Multi_FindItemType( IsType(IT_SHIP) ? IT_SHIP_TILLER : IT_SIGN_GUMP );
		if ( pTiller == NULL )
			return( this );
		m_uidLink = pTiller->GetUID();
	}
	return( pTiller );
}

void CItemMulti::OnHearRegion( LPCTSTR pszCmd, CChar * pSrc )
{
	ADDTOCALLSTACK("CItemMulti::OnHearRegion");
	// IT_SHIP or IT_MULTI

	const CItemBaseMulti * pMultiDef = Multi_GetDef();
	if ( pMultiDef == NULL )
		return;
	TALKMODE_TYPE		mode	= TALKMODE_SAY;

	for ( size_t i = 0; i < pMultiDef->m_Speech.GetCount(); i++ )
	{
		CResourceLink * pLink = pMultiDef->m_Speech[i];
		ASSERT(pLink);
		CResourceLock s;
		if ( ! pLink->ResourceLock( s ))
			continue;
		TRIGRET_TYPE iRet = OnHearTrigger( s, pszCmd, pSrc, mode );
		if ( iRet == TRIGRET_ENDIF || iRet == TRIGRET_RET_FALSE )
			continue;
		break;
	}
}

enum
{
	SHV_MULTICREATE,
	SHV_QTY
};

LPCTSTR const CItemMulti::sm_szVerbKeys[SHV_QTY+1] =
{
	"MULTICREATE",
	NULL
};

bool CItemMulti::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CItemMulti::r_GetRef");
	// COMP(x).

	if ( ! strnicmp( pszKey, "COMP(", 4 ))
	{
		pszKey += 5;
		int i = Exp_GetVal(pszKey);
		SKIP_SEPARATORS(pszKey);
		pRef = Multi_FindItemComponent(i);
		return( true );
	}
	if ( ! strnicmp( pszKey, "REGION", 6 ))
	{
		pszKey += 6;
		SKIP_SEPARATORS(pszKey);
		pRef = m_pRegion;
		return( true );
	}

	return( CItem::r_GetRef( pszKey, pRef ));
}

bool CItemMulti::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CItemMulti::r_Verb");
	EXC_TRY("Verb");
	// Speaking in this multis region.
	// return: true = command for the multi.

	int iCmd = FindTableSorted( s.GetKey(), sm_szVerbKeys, COUNTOF( sm_szVerbKeys )-1 );
	switch ( iCmd )
	{
		case SHV_MULTICREATE:
		{
			CGrayUID	uid( s.GetArgVal() );
			CChar *	pSrc	= uid.CharFind();
			Multi_Create( pSrc, 0 );
			return true;
		}

		default:
		{
			return CItem::r_Verb( s, pSrc );
		}
	}

	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return true;
}

void CItemMulti::r_Write( CScript & s )
{
	ADDTOCALLSTACK_INTENSIVE("CItemMulti::r_Write");
	CItem::r_Write(s);
	if ( m_pRegion )
	{
		m_pRegion->r_WriteBody( s, "REGION." );
	}
}
bool CItemMulti::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CItemMulti::r_WriteVal");
	if ( !strnicmp(pszKey, "COMP", 4) )
	{
		const CItemBaseMulti *pMultiDef = Multi_GetDef();
		pszKey += 4;

		// no component uid
		if ( *pszKey == '\0' )
		{
			sVal.FormatVal(pMultiDef->m_Components.GetCount());
		}
		else if ( *pszKey == '.' )
		{
			CItemBaseMulti::CMultiComponentItem	item;

			SKIP_SEPARATORS(pszKey);
			size_t iQty = Exp_GetVal(pszKey);
			if ( pMultiDef->m_Components.IsValidIndex(iQty) == false )
				return false;

			SKIP_SEPARATORS(pszKey);
			item = pMultiDef->m_Components.GetAt(iQty);

			if ( !strnicmp(pszKey, "ID", 2) ) sVal.FormatVal(item.m_id);
			else if ( !strnicmp(pszKey, "DX", 2) ) sVal.FormatVal(item.m_dx);
			else if ( !strnicmp(pszKey, "DY", 2) ) sVal.FormatVal(item.m_dy);
			else if ( !strnicmp(pszKey, "DZ", 2) ) sVal.FormatVal(item.m_dz);
			else if ( !strnicmp(pszKey, "D", 1) ) sVal.Format("%i,%i,%i", item.m_dx, item.m_dy, item.m_dz);
			else sVal.Format("%u,%i,%i,%i", item.m_id, item.m_dx, item.m_dy, item.m_dz);
		}
		else return false;
		return true;
	}
	return( CItem::r_WriteVal(pszKey, sVal, pSrc));
}

bool CItemMulti::r_LoadVal( CScript & s  )
{
	ADDTOCALLSTACK("CItemMulti::r_LoadVal");
	EXC_TRY("LoadVal");
	if ( s.IsKeyHead( "REGION.", 7 ))
	{
		if ( ! IsTopLevel())
		{
			MoveTo( GetTopPoint()); // Put item on the ground here.
		}
		ASSERT( m_pRegion );
		CScript script( s.GetKey()+7, s.GetArgStr());
		return( m_pRegion->r_LoadVal( script ) );
	}
	return CItem::r_LoadVal(s);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}
void CItemMulti::DupeCopy( const CItem * pItem )
{
	ADDTOCALLSTACK("CItemMulti::DupeCopy");
	CItem::DupeCopy(pItem);
}

