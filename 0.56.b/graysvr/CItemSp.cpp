//
// CItemSp.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "graysvr.h"	// predef header.

/////////////////////////////////////////////////////////////////////////////

inline CCharBase * CItem::Spawn_TryChar( CREID_TYPE &id )
{
	ADDTOCALLSTACK("CItem::Spawn_TryChar");
		CCharBase * pCharDef = CCharBase::FindCharBase( id );
		if ( pCharDef )
		{
			m_itSpawnChar.m_CharID = RESOURCE_ID( RES_CHARDEF, id );
			return( pCharDef );
		}
		return NULL;
}

inline CItemBase * CItem::Spawn_TryItem( ITEMID_TYPE &id )
{
	ADDTOCALLSTACK("CItem::Spawn_TryItem");
		CItemBase * pItemDef = CItemBase::FindItemBase( id );
		if ( pItemDef )
		{
			m_itSpawnItem.m_ItemID = RESOURCE_ID( RES_ITEMDEF, id );
			return( pItemDef );
		}
		return NULL;
}

CResourceDef * CItem::Spawn_FixDef()
{
	ADDTOCALLSTACK("CItem::Spawn_FixDef");
	// Get a proper RESOURCE_ID from the id provided.
	// RETURN: true = ok.

	RESOURCE_ID_BASE rid = ( IsType(IT_SPAWN_ITEM) ? m_itSpawnItem.m_ItemID : m_itSpawnChar.m_CharID );

	if ( rid.GetResType() != RES_UNKNOWN )
	{
		return STATIC_CAST <CResourceDef *>(g_Cfg.ResourceGetDef(rid));
	}

	// No type info here !?
	if ( IsType(IT_SPAWN_ITEM))
	{
		ITEMID_TYPE id = static_cast<ITEMID_TYPE>(rid.GetResIndex());
		if ( id < ITEMID_TEMPLATE )
		{
			return( Spawn_TryItem( id ) );
		}
		else
		{
			// try a template.
			rid = RESOURCE_ID( RES_TEMPLATE, id );
			CResourceDef * pDef = g_Cfg.ResourceGetDef(rid);
			if ( pDef )
			{
				m_itSpawnItem.m_ItemID = rid;
				return( STATIC_CAST <CResourceDef *>( pDef ));
			} //if fails
			return( Spawn_TryItem( id ) );
		}
	}
	else
	{
		CREID_TYPE id = static_cast<CREID_TYPE>(rid.GetResIndex());
		if ( id < SPAWNTYPE_START )
		{
			return( Spawn_TryChar( id ));
		}
		else
		{
			// try a spawn group.
			rid = RESOURCE_ID( RES_SPAWN, id );
			CResourceDef * pDef = g_Cfg.ResourceGetDef(rid);
			if ( pDef )
			{
				m_itSpawnChar.m_CharID = rid;
				return( STATIC_CAST <CResourceDef *>( pDef ));
			} //if fails
			return( Spawn_TryChar( id ));
		}
	}
}

int CItem::Spawn_GetName( TCHAR * pszOut ) const
{
	ADDTOCALLSTACK("CItem::Spawn_GetName");
	RESOURCE_ID_BASE rid;
	if ( IsType(IT_SPAWN_ITEM))
	{
		rid = m_itSpawnItem.m_ItemID;
	}
	else
	{
		// Name the spawn type.
		rid = m_itSpawnChar.m_CharID;
	}

	LPCTSTR pszName = NULL;
	CResourceDef * pDef = g_Cfg.ResourceGetDef( rid );
	if ( pDef != NULL )
	{
		pszName = pDef->GetName();
	}
	if ( pDef == NULL || pszName == NULL || pszName[0] == '\0' )
	{
		pszName = g_Cfg.ResourceGetName( rid );
	}
	return sprintf( pszOut, " (%s)", pszName );
}



void CItem::Spawn_GenerateItem( CResourceDef * pDef )
{
	ADDTOCALLSTACK("CItem::Spawn_GenerateItem");
	// Count how many items are here already.
	// This could be in a container.

	RESOURCE_ID_BASE rid = pDef->GetResourceID();
	ITEMID_TYPE id = static_cast<ITEMID_TYPE>(rid.GetResIndex());
	int iDistMax = m_itSpawnItem.m_DistMax;
	int iAmountPile = m_itSpawnItem.m_pile;

	int iCount = 0;
	CItemContainer * pCont = dynamic_cast <CItemContainer *>( GetParent());
	if ( pCont != NULL )
	{
		iCount = pCont->ContentCount( rid );
	}
	else
	{
		// If is equipped this will produce the item where you are standing.
		CPointMap pt = GetTopLevelObj()->GetTopPoint();
		CWorldSearch AreaItems( pt, iDistMax );
		for (;;)
		{
			CItem * pItem = AreaItems.GetItem();
			if ( pItem == NULL )
				break;
			if ( pItem->IsType(IT_SPAWN_ITEM))
				continue;
			if ( pItem->IsAttr( ATTR_INVIS ))
				continue;
			if ( pItem->GetID() != id )
				continue;
			// if ( pItem->m_uidLink != GetUID()) continue;
			iCount += pItem->GetAmount();
		}
	}
	if ( iCount >= GetAmount())
		return;

	CItem * pItem = CreateTemplate( id );
	if ( pItem == NULL )
		return;

	pItem->SetAttr( m_Attr & ( ATTR_OWNED | ATTR_MOVE_ALWAYS ));

	if ( iAmountPile > 1 )
	{
		CItemBase * pItemDef = pItem->Item_GetDef();
		ASSERT(pItemDef);
		if ( pItemDef->IsStackableType())
		{
			if ( iAmountPile == 0 || iAmountPile > GetAmount())
				iAmountPile = GetAmount();
			pItem->SetAmount( Calc_GetRandVal(iAmountPile) + 1 );
		}
	}

	pItem->SetKeyNum("SpawnItem", static_cast<DWORD>(GetUID()));	// This might be dangerous ?
	pItem->SetDecayTime( g_Cfg.m_iDecay_Item );	// It will decay eventually to be replaced later.
	pItem->MoveNearObj( this, iDistMax );
}



void CItem::Spawn_GenerateChar( CResourceDef * pDef )
{
	ADDTOCALLSTACK("CItem::Spawn_GenerateChar");
	if ( !IsTopLevel() || ( m_itSpawnChar.m_current >= GetAmount() ) || ( GetTopSector()->GetCharComplexity() > g_Cfg.m_iMaxCharComplexity ))
		return;

	int iDistMax = m_itSpawnChar.m_DistMax;
	RESOURCE_ID_BASE rid = pDef->GetResourceID();
	if ( rid.GetResType() == RES_SPAWN )
	{
		const CRandGroupDef * pSpawnGroup = STATIC_CAST <const CRandGroupDef *>(pDef);
		ASSERT(pSpawnGroup);
		size_t i = pSpawnGroup->GetRandMemberIndex();
		if ( i != pSpawnGroup->BadMemberIndex() )
		{
			rid = pSpawnGroup->GetMemberID(i);
		}
	}

	if (( rid.GetResType() != RES_CHARDEF ) && ( rid.GetResType() != RES_UNKNOWN ))
		return;

	CREID_TYPE id = static_cast<CREID_TYPE>(rid.GetResIndex());

	bool isBadPlaceToSpawn = false;
	CChar * pChar = CChar::CreateBasic(id);
	if( pChar == NULL )
	{
		return;
	}

	m_itSpawnChar.m_current ++;
	pChar->Memory_AddObjTypes(this, MEMORY_ISPAWNED);

	pChar->MoveTo(GetTopPoint());
	pChar->NPC_LoadScript(true);

	if( pChar->GetRegion() == NULL )
	{
		isBadPlaceToSpawn = true;
	}
	else if( pChar->GetRegion()->IsGuarded() && pChar->Noto_IsEvil() )
	{
		isBadPlaceToSpawn = true;
	}

	// Deny definitely known a bad place to spawn (like red NPCs in guarded areas)
	// Usually caused by wide range near the edge of the towns
	if( isBadPlaceToSpawn )
	{
		pChar->Delete();
		m_itSpawnChar.m_current--;
		return;
	}

	ASSERT(pChar->m_pNPC);
	if ( iDistMax )
	{
		pChar->m_ptHome = GetTopPoint();
		pChar->m_pNPC->m_Home_Dist_Wander = iDistMax;
	}
	pChar->Update();
}



void CItem::Spawn_OnTick( bool fExec )
{
	ADDTOCALLSTACK("CItem::Spawn_OnTick");

	INT64 iMinutes;
	if ( m_itSpawnChar.m_TimeHiMin <= 0 )
		iMinutes = Calc_GetRandLLVal(30) + 1;
	else
		iMinutes = minimum( m_itSpawnChar.m_TimeHiMin, m_itSpawnChar.m_TimeLoMin ) + Calc_GetRandLLVal( abs( m_itSpawnChar.m_TimeHiMin - m_itSpawnChar.m_TimeLoMin ));

	if ( iMinutes <= 0 )
		iMinutes = 1;

	if ( !fExec || IsTimerExpired() )
		SetTimeout( iMinutes * 60 * TICK_PER_SEC );	// set time to check again.

	if ( ! fExec )
		return;

	CResourceDef * pDef = Spawn_FixDef();
	if ( !pDef )
	{
		RESOURCE_ID_BASE rid = ( IsType(IT_SPAWN_ITEM) ? m_itSpawnItem.m_ItemID : m_itSpawnChar.m_CharID);
		DEBUG_ERR(( "Bad Spawn point uid=0%lx, id=%s\n", (DWORD) GetUID(), g_Cfg.ResourceGetName(rid) ));
		return;
	}

	if ( IsType(IT_SPAWN_ITEM) )
		Spawn_GenerateItem(pDef);
	else
		Spawn_GenerateChar(pDef);
}

void CItem::Spawn_KillChildren()
{
	ADDTOCALLSTACK("CItem::Spawn_KillChildren");
	// kill all creatures spawned from this !
	int iCurrent = m_itSpawnChar.m_current;

	for ( int j = 0; j < 256; j++ )	// loop through all maps
	{
		if ( !g_MapList.m_maps[j] ) continue;		// skip unsupported maps

		for ( int i = 0; i < g_MapList.GetSectorQty(j); i++ )
		{
			CSector * pSector = g_World.GetSector(j, i);
			ASSERT(pSector);
			CChar * pCharNext;
			CChar * pChar = STATIC_CAST <CChar*>( pSector->m_Chars_Active.GetHead());
			for ( ; pChar!=NULL; pChar = pCharNext )
			{
				pCharNext = pChar->GetNext();
				if ( pChar->IsStatFlag(STATF_Spawned) && pChar->m_pNPC &&
					pChar->Memory_FindObjTypes(this, MEMORY_ISPAWNED) )
				{
					pChar->Delete();
					iCurrent--;
				}
			}
		}
	}
	m_itSpawnChar.m_current = 0;	// Should not be necessary
	Spawn_OnTick(false);
}

CCharBase * CItem::Spawn_SetTrackID()
{
	ADDTOCALLSTACK("CItem::Spawn_SetTrackID");
	if ( ! IsType(IT_SPAWN_CHAR))
		return NULL;
	CCharBase * pCharDef = NULL;
	RESOURCE_ID_BASE rid = m_itSpawnChar.m_CharID;

	if ( rid.GetResType() == RES_CHARDEF )
	{
		CREID_TYPE id = static_cast<CREID_TYPE>(rid.GetResIndex());
		pCharDef = CCharBase::FindCharBase( id );
	}
	if ( pCharDef )
		SetAttr( ATTR_INVIS );
	if ( IsAttr(ATTR_INVIS))	// They must want it to look like this.
	{
		SetDispID( ( pCharDef == NULL ) ? ITEMID_TRACK_WISP : pCharDef->m_trackID );
		if ( GetHue() == 0 )
			SetHue( HUE_RED_DARK );	// Indicate to GM's that it is invis.
	}
	return( pCharDef );
}

/////////////////////////////////////////////////////////////////////////////

void CItem::Plant_SetTimer()
{
	ADDTOCALLSTACK("CItem::Plant_SetTimer");
	SetTimeout( GetDecayTime() );
}

bool CItem::Plant_Use( CChar * pChar )
{
	ADDTOCALLSTACK("CItem::Plant_Use");
	// Pick cotton/hay/etc...
	// use the
	//  IT_CROPS = transforms into a "ripe" variety then is used up on reaping.
	//  IT_FOLIAGE = is not consumed on reap (unless eaten then will regrow invis)
	//

	if ( !pChar )
		return false;
	if ( ! pChar->CanSeeItem(this))	// might be invis underground.
		return( false );

	const CItemBase * pItemDef = Item_GetDef();
	ITEMID_TYPE iFruitID = static_cast<ITEMID_TYPE>(RES_GET_INDEX(pItemDef->m_ttCrops.m_idFruit)); // if it's reapable at this stage.
	if ( iFruitID <= 0 )
	{
		// not ripe. (but we could just eat it if we are herbivorous ?)
		pChar->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_CROPS_NOT_RIPE ) );
		return( true );
	}
	if ( m_itCrop.m_ReapFruitID )
	{
		iFruitID = static_cast<ITEMID_TYPE>(RES_GET_INDEX(m_itCrop.m_ReapFruitID));
	}

	if ( iFruitID > 0 )
	{
		CItem * pItemFruit = CItem::CreateScript( iFruitID , pChar );
		if ( pItemFruit)
		{
			pChar->ItemBounce( pItemFruit );
		}
	}
	else
	{
		pChar->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_CROPS_NO_FRUIT ) );
	}

	Plant_CropReset();

	pChar->UpdateAnimate( ANIM_BOW );
	pChar->Sound( 0x13e );
	return true;
}

bool CItem::Plant_OnTick()
{
	ADDTOCALLSTACK("CItem::Plant_OnTick");
	ASSERT( IsType(IT_CROPS) || IsType(IT_FOLIAGE));
	// If it is in a container, kill it.
	if ( !IsTopLevel())
	{
		return false;
	}

	// Make sure the darn thing isn't moveable
	SetAttr(ATTR_MOVE_NEVER);
	Plant_SetTimer();

	// No tree stuff below here
	if ( IsAttr(ATTR_INVIS)) // If it's invis, take care of it here and return
	{
		SetHue( HUE_DEFAULT );
		ClrAttr(ATTR_INVIS);
		Update();
		return true;
	}

	const CItemBase * pItemDef = Item_GetDef();
	ITEMID_TYPE iGrowID = pItemDef->m_ttCrops.m_idGrow;

	if ( iGrowID == -1 )
	{
		// Some plants generate a fruit on the ground when ripe.
		ITEMID_TYPE iFruitID = static_cast<ITEMID_TYPE>(RES_GET_INDEX(pItemDef->m_ttCrops.m_idGrow));
		if ( m_itCrop.m_ReapFruitID )
		{
			iFruitID = static_cast<ITEMID_TYPE>(RES_GET_INDEX(m_itCrop.m_ReapFruitID));
		}
		if ( ! iFruitID )
		{
			return( true );
		}

		// put a fruit on the ground if not already here.
		CWorldSearch AreaItems( GetTopPoint() );
		for (;;)
		{
			CItem * pItem = AreaItems.GetItem();
			if ( pItem == NULL )
			{
				CItem * pItemFruit = CItem::CreateScript( iFruitID );
				ASSERT(pItemFruit);
				pItemFruit->MoveToDecay(GetTopPoint(),10*g_Cfg.m_iDecay_Item);
				break;
			}
			if ( pItem->IsType( IT_FRUIT ) || pItem->IsType( IT_REAGENT_RAW ))
				break;
		}

		// NOTE: ??? The plant should cycle here as well !
		iGrowID = pItemDef->m_ttCrops.m_idReset;
	}

	if ( iGrowID > 0 )
	{
		SetID(static_cast<ITEMID_TYPE>(RES_GET_INDEX(iGrowID)));
		Update();
		return true;
	}

	// some plants go dormant again ?

	// m_itCrop.m_Fruit_ID = iTemp;
	return true;
}

void CItem::Plant_CropReset()
{
	ADDTOCALLSTACK("CItem::Plant_CropReset");
	// Animals will eat crops before they are ripe, so we need a way to reset them prematurely

	if ( ! IsType(IT_CROPS) && ! IsType(IT_FOLIAGE))
	{
		// This isn't a crop, and since it just got eaten, we should delete it
		Delete();
		return;
	}

	const CItemBase * pItemDef = Item_GetDef();
	ITEMID_TYPE iResetID = static_cast<ITEMID_TYPE>(RES_GET_INDEX(pItemDef->m_ttCrops.m_idReset));
	if ( iResetID != ITEMID_NOTHING )
	{
		SetID(iResetID);
	}

	Plant_SetTimer();
	RemoveFromView();	// remove from most screens.
	SetHue( HUE_RED_DARK );	// Indicate to GM's that it is growing.
	SetAttr(ATTR_INVIS);	// regrown invis.
}

/////////////////////////////////////////////////////////////////////////////
// -CItemMap

bool CItemMap::IsSameType(const CObjBase *pObj) const
{
	const CItemMap *pItemMap = dynamic_cast<const CItemMap *>( pObj );
	if ( pItemMap )
	{
		// maps can only stack on top of each other if the pins match
		if ( m_Pins.GetCount() != pItemMap->m_Pins.GetCount() )
			return false;

		// check individual pins are in the same place
		for (size_t i = 0; i < m_Pins.GetCount(); i++)
		{
			if ( m_Pins[i].m_x != pItemMap->m_Pins[i].m_x )
				return false;
			else if ( m_Pins[i].m_y != pItemMap->m_Pins[i].m_y )
				return false;
		}
	}

	return CItemVendable::IsSameType( pObj );
}

bool CItemMap::r_LoadVal( CScript & s ) // Load an item Script
{
	ADDTOCALLSTACK("CItemMap::r_LoadVal");
	EXC_TRY("LoadVal");
	if ( s.IsKeyHead("PIN", 3 ))
	{
		CPointMap pntTemp;
		pntTemp.Read( s.GetArgStr());
		CMapPinRec pin( pntTemp.m_x, pntTemp.m_y );
		m_Pins.Add( pin );
		return( true );
	}
	return CItem::r_LoadVal(s);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CItemMap::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CItemMap::r_WriteVal");
	EXC_TRY("WriteVal");
	if ( ! strnicmp( pszKey, "PINS", 4 ))
	{
		sVal.FormatVal(m_Pins.GetCount());
		return true;
	}
	if ( ! strnicmp( pszKey, "PIN.", 4 ))
	{
		pszKey += 4;
		size_t i = Exp_GetVal(pszKey) - 1;
		if ( m_Pins.IsValidIndex(i) )
		{
			sVal.Format( "%i,%i", m_Pins[i].m_x, m_Pins[i].m_y );
			return( true );
		}
	}
	return CItemVendable::r_WriteVal(pszKey, sVal, pSrc);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

void CItemMap::r_Write( CScript & s )
{
	ADDTOCALLSTACK_INTENSIVE("CItemMap::r_Write");
	CItemVendable::r_Write( s );
	for ( size_t i = 0; i < m_Pins.GetCount(); i++ )
	{
		s.WriteKeyFormat( "PIN", "%i,%i", m_Pins[i].m_x, m_Pins[i].m_y );
	}
}

void CItemMap::DupeCopy( const CItem * pItem )
{
	ADDTOCALLSTACK("CItemMap::DupeCopy");
	CItemVendable::DupeCopy(pItem);

	const CItemMap * pMapItem = dynamic_cast <const CItemMap *>(pItem);
	if ( pMapItem == NULL )
		return;
	m_Pins.Copy( &(pMapItem->m_Pins));
}

/////////////////////////////////////////////////////////////////////////////
// -CItemMessage

void CItemMessage::r_Write( CScript & s )
{
	ADDTOCALLSTACK_INTENSIVE("CItemMessage::r_Write");
	CItemVendable::r_Write( s );

	s.WriteKey( "AUTHOR", m_sAuthor );

	TemporaryString pszTemp;
	// Store the message body lines. MAX_BOOK_PAGES
	for ( size_t i = 0; i < GetPageCount(); ++i )
	{
		sprintf(pszTemp, "BODY.%" FMTSIZE_T, i);
		LPCTSTR pszText = GetPageText(i);
		s.WriteKey(pszTemp, pszText != NULL ? pszText : "" );
	}
}

LPCTSTR const CItemMessage::sm_szLoadKeys[CIC_QTY+1] = // static
{
	"AUTHOR",
	"BODY",
	"PAGES",	// (W)
	"TITLE",	// same as name
	NULL,
};

bool CItemMessage::r_LoadVal( CScript &s )
{
	ADDTOCALLSTACK("CItemMessage::r_LoadVal");
	EXC_TRY("LoadVal");
	// Load the message body for a book or a bboard message.
	if ( s.IsKeyHead( "BODY", 4 ))
	{
		AddPageText( s.GetArgStr());
		return( true );
	}
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	case CIC_AUTHOR:
		if ( s.GetArgStr()[0] != '0' )
		{
			m_sAuthor = s.GetArgStr();
		}
		return( true );
	case CIC_BODY:	// handled above.
		return( false );
	case CIC_PAGES:	// not settable. (used for resource stuff)
		return( false );
	case CIC_TITLE:
		SetName( s.GetArgStr());
		return( true );
	}
	return CItemVendable::r_LoadVal(s);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CItemMessage::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CItemMessage::r_WriteVal");
	EXC_TRY("WriteVal");
	// Load the message body for a book or a bboard message.
	if ( ! strnicmp( pszKey, "BODY", 4 ))
	{
		pszKey += 4;
		size_t iPage = Exp_GetVal(pszKey);
		if ( m_sBodyLines.IsValidIndex(iPage) == false )
			return( false );
		sVal = *m_sBodyLines[iPage];
		return( true );
	}
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	case CIC_AUTHOR:
		sVal = m_sAuthor;
		return( true );
	case CIC_BODY:	// handled above.
		return( false );
	case CIC_PAGES:	// not settable. (used for resource stuff)
		sVal.FormatVal( m_sBodyLines.GetCount());
		return( true );
	case CIC_TITLE:
		sVal = GetName();
		return( true );
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

bool CItemMessage::r_Verb( CScript & s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CItemMessage::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);
	if ( s.IsKey( sm_szVerbKeys[0] ))
	{
		// 1 based pages.
		size_t iPage = ( s.GetArgStr()[0] && toupper( s.GetArgStr()[0] ) != 'A' ) ? s.GetArgVal() : 0;
		if ( iPage <= 0 )
		{
			m_sBodyLines.RemoveAll();
			return( true );
		}
		else if ( iPage <= m_sBodyLines.GetCount())
		{
			m_sBodyLines.RemoveAt( iPage - 1 );
			return( true );
		}
	}
	if ( s.IsKeyHead( "PAGE", 4 ))
	{
		size_t iPage =  ATOI( s.GetKey() + 4 );
		if ( iPage <= 0 )
			return( false );

		SetPageText( iPage - 1, s.GetArgStr());
		return( true );
	}
	return CItemVendable::r_Verb(s, pSrc);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

void CItemMessage::DupeCopy( const CItem * pItem )
{
	ADDTOCALLSTACK("CItemMessage::DupeCopy");
	CItemVendable::DupeCopy( pItem );

	const CItemMessage * pMsgItem = dynamic_cast <const CItemMessage *>(pItem);
	if ( pMsgItem == NULL )
		return;

	m_sAuthor = pMsgItem->m_sAuthor;
	for ( size_t i = 0; i < pMsgItem->GetPageCount(); i++ )
	{
		SetPageText( i, pMsgItem->GetPageText(i));
	}
}

/////////////////////////////////////////////////////////////////////////////
// -CItemMemory

CItemStone * CItemMemory::Guild_GetLink()
{
	ADDTOCALLSTACK("CItemMemory::Guild_GetLink");
	if ( ! IsMemoryTypes(MEMORY_TOWN|MEMORY_GUILD))
		return NULL;
	return dynamic_cast<CItemStone*>(m_uidLink.ItemFind());
}

bool CItemMemory::Guild_IsAbbrevOn() const
{
	ADDTOCALLSTACK("CItemMemory::Guild_IsAbbrevOn");
	return( m_itEqMemory.m_Action != 0 );
}

void CItemMemory::Guild_SetAbbrev( bool fAbbrevShow )
{
	ADDTOCALLSTACK("CItemMemory::Guild_SetAbbrev");
	m_itEqMemory.m_Action = fAbbrevShow;
}

WORD CItemMemory::Guild_GetVotes() const
{
	ADDTOCALLSTACK("CItemMemory::Guild_GetVotes");
	return m_itEqMemory.m_Skill;
}

void CItemMemory::Guild_SetVotes( WORD wVotes )
{
	ADDTOCALLSTACK("CItemMemory::Guild_SetVotes");
	m_itEqMemory.m_Skill = wVotes;
}

int CItemMemory::Guild_SetLoyalTo( CGrayUID uid )
{
	ADDTOCALLSTACK("CItemMemory::Guild_SetLoyalTo");
	// Some other place checks to see if this is a valid member.
	return GetTagDefs()->SetNum("LoyalTo", (DWORD) uid , false);
}

CGrayUID CItemMemory::Guild_GetLoyalTo() const
{
	ADDTOCALLSTACK("CItemMemory::Guild_GetLoyalTo");
	CItemMemory * pObj = const_cast<CItemMemory *>(this);
	CGrayUID iUid(static_cast<DWORD>(pObj->GetTagDefs()->GetKeyNum("LoyalTo", true)));
	return iUid;
}

int CItemMemory::Guild_SetTitle( LPCTSTR pszTitle )
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
	{
		return( iResultCode );
	}

	if ( ! IsItemEquipped() ||
		GetEquipLayer() != LAYER_SPECIAL ||
		! GetMemoryTypes())	// has to be a memory of some sort.
	{
		iResultCode = 0x4222;
		return( iResultCode );	// get rid of it.
	}

	CChar * pChar = dynamic_cast <CChar*>( GetParent());
	if ( pChar == NULL )
	{
		iResultCode = 0x4223;
		return( iResultCode );	// get rid of it.
	}

	// If it is my guild make sure i am linked to it correctly !
	if ( IsMemoryTypes(MEMORY_GUILD|MEMORY_TOWN))
	{
		const CItemStone * pStone = pChar->Guild_Find(static_cast<MEMORY_TYPE>(GetMemoryTypes()));
		if ( pStone == NULL || pStone->GetUID() != m_uidLink )
		{
			iResultCode = 0x4224;
			return( iResultCode );	// get rid of it.
		}
		if ( ! pStone->GetMember( pChar ))
		{
			iResultCode = 0x4225;
			return( iResultCode );	// get rid of it.
		}
	}
	return( 0 );
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

bool CItemCommCrystal::MoveTo(CPointMap pt, bool bForceFix ) // Put item on the ground here.
{
	ADDTOCALLSTACK("CItemCommCrystal::MoveTo");
	// Move this item to it's point in the world. (ground/top level)
	CSector * pSector = pt.GetSector();
	ASSERT(pSector);
	pSector->AddListenItem();
	return CItem::MoveTo(pt, bForceFix);
}

void CItemCommCrystal::OnHear( LPCTSTR pszCmd, CChar * pSrc )
{
	ADDTOCALLSTACK("CItemCommCrystal::OnHear");
	// IT_COMM_CRYSTAL
	// STATF_COMM_CRYSTAL = if i am on a person.
	TALKMODE_TYPE mode = TALKMODE_SAY;

	for ( size_t i = 0; i < m_Speech.GetCount(); i++ )
	{
		CResourceLink * pLink = m_Speech[i];
		ASSERT(pLink);
		CResourceLock s;
		if ( ! pLink->ResourceLock( s ))
			continue;
		TRIGRET_TYPE iRet = OnHearTrigger( s, pszCmd, pSrc, mode );
		if ( iRet == TRIGRET_ENDIF || iRet == TRIGRET_RET_FALSE )
			continue;
		break;
	}

	// That's prevent @ -1 crash speech :P
	if ( *pszCmd == '@' )
		return;

	if ( m_uidLink.IsValidUID())
	{
		// I am linked to something ?
		// Transfer the sound.
		CItem * pItem = m_uidLink.ItemFind();
		if ( pItem != NULL && pItem->IsType(IT_COMM_CRYSTAL))
		{
			pItem->Speak( pszCmd );
		}
	}
	else if ( m_Speech.GetCount() <= 0 )
	{
		Speak( pszCmd );
	}
}

void CItemCommCrystal::r_Write( CScript & s )
{
	ADDTOCALLSTACK_INTENSIVE("CItemCommCrystal::r_Write");
	CItemVendable::r_Write(s);
	m_Speech.r_Write( s, "SPEECH" );
}

bool CItemCommCrystal::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CItemCommCrystal::r_WriteVal");
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case 0:
			m_Speech.WriteResourceRefList( sVal );
			break;
		default:
			return CItemVendable::r_WriteVal(pszKey,sVal,pSrc);
	}
	return( true );
}

bool CItemCommCrystal::r_LoadVal( CScript & s  )
{
	ADDTOCALLSTACK("CItemCommCrystal::r_LoadVal");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case 0:
			return( m_Speech.r_LoadVal( s, RES_SPEECH ));
		default:
			return CItemVendable::r_LoadVal(s);
	}
}

void CItemCommCrystal::DupeCopy( const CItem * pItem )
{
	ADDTOCALLSTACK("CItemCommCrystal::DupeCopy");
	CItemVendable::DupeCopy(pItem);

	const CItemCommCrystal * pItemCrystal = dynamic_cast <const CItemCommCrystal *>(pItem);
	if ( pItemCrystal == NULL )
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

void CItemScript::r_Write( CScript & s )
{
	ADDTOCALLSTACK_INTENSIVE("CItemScript::r_Write");
	CItemVendable::r_Write(s);
}

bool CItemScript::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CItemScript::r_WriteVal");
	return CItemVendable::r_WriteVal(pszKey,sVal,pSrc);
}
bool CItemScript::r_LoadVal( CScript & s  )
{
	ADDTOCALLSTACK("CItemScript::r_LoadVal");
	return CItemVendable::r_LoadVal(s);
}

bool CItemScript::r_Verb( CScript & s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CItemScript::r_Verb");
	return( CItemVendable::r_Verb(s,pSrc));
}

void CItemScript::DupeCopy( const CItem * pItem )
{
	ADDTOCALLSTACK("CItemScript::DupeCopy");
	CItemVendable::DupeCopy(pItem);
}
