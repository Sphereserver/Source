//
// CItemShip.cpp
//

#include "graysvr.h"	// predef header.

/////////////////////////////////////////////////////////////////////////////

CItemShip::CItemShip( ITEMID_TYPE id, CItemBase * pItemDef ) : CItemMulti( id, pItemDef )
{
}

bool CItem::Ship_Plank( bool fOpen )
{
	ADDTOCALLSTACK("CItem::Ship_Plank");
	// IT_SHIP_PLANK to IT_SHIP_SIDE and IT_SHIP_SIDE_LOCKED
	// This item is the ships plank.

	CItemBase * pItemDef = Item_GetDef();
	ITEMID_TYPE idState = (ITEMID_TYPE) RES_GET_INDEX( pItemDef->m_ttShipPlank.m_idState );
	if ( !idState )
		return( false );

	if ( IsType(IT_SHIP_PLANK))
	{
		if ( fOpen )
			return( true );
	}
	else
	{
		if ( ! fOpen )
			return( true );
	}

	IT_TYPE oldType = GetType();
	SetID( idState );

	if ( IsType(IT_SHIP_PLANK) && (oldType == IT_SHIP_SIDE || oldType == IT_SHIP_SIDE_LOCKED) )
	{
		// Save the original Type of the plank if it used to be a ship side
		m_itShipPlank.m_itSideType = oldType;
	}
	else if ( oldType == IT_SHIP_PLANK )
	{
		// Restore the type of the ship side
		if ( m_itShipPlank.m_itSideType == IT_SHIP_SIDE || m_itShipPlank.m_itSideType == IT_SHIP_SIDE_LOCKED )
			SetType( (IT_TYPE)m_itShipPlank.m_itSideType );

		m_itShipPlank.m_itSideType = IT_NORMAL;
	}

	Update();
	return( true );
}

void CItemShip::Ship_Stop()
{
	ADDTOCALLSTACK("CItemShip::Ship_Stop");
	// Make sure we have stopped.
	m_itShip.m_fSail = false;
	SetTimeout( -1 );
}

bool CItemShip::Ship_SetMoveDir( DIR_TYPE dir )
{
	ADDTOCALLSTACK("CItemShip::Ship_SetMoveDir");
	// Set the direction we will move next time we get a tick.
	int iSpeed = 1;
	if ( m_itShip.m_DirMove == dir && m_itShip.m_fSail )
	{
		if ( m_itShip.m_DirFace == m_itShip.m_DirMove &&
			m_itShip.m_fSail == 1 )
		{
			iSpeed = 2;
		}
		else return( false );
	}

	if ( ! IsAttr(ATTR_MAGIC ))	// make sound.
	{
		if ( ! Calc_GetRandVal(10))
		{
			Sound( Calc_GetRandVal(2)?0x12:0x13 );
		}
	}

	m_itShip.m_DirMove = dir;
	m_itShip.m_fSail = iSpeed;
	GetTopSector()->SetSectorWakeStatus();	// may get here b4 my client does.
	SetTimeout(( m_itShip.m_fSail == 1 ) ? 1*TICK_PER_SEC : (TICK_PER_SEC/5));
	return( true );
}

#define MAX_MULTI_LIST_OBJS 128

int CItemShip::Ship_ListObjs( CObjBase ** ppObjList )
{
	ADDTOCALLSTACK("CItemShip::Ship_ListObjs");
	// List all the objects in the structure.
	// Move the ship and everything on the deck
	// If too much stuff. then some will fall overboard. hehe.

	if ( ! IsTopLevel())
		return 0;

	int iMaxDist = Multi_GetMaxDist();

	// always list myself first. All other items must see my new region !
	int iCount = 0;
	ppObjList[iCount++] = this;

	CWorldSearch AreaChar( GetTopPoint(), iMaxDist );
	AreaChar.SetAllShow( true );
	while ( iCount < MAX_MULTI_LIST_OBJS )
	{
		CChar * pChar = AreaChar.GetChar();
		if ( pChar == NULL )
			break;
		if ( ! m_pRegion->IsInside2d( pChar->GetTopPoint()))
			continue;
		if ( pChar->IsDisconnected() && pChar->m_pNPC )
			continue;
		int zdiff = pChar->GetTopZ() - GetTopZ();
		if ( abs( zdiff ) > 3 )
			continue;
		ppObjList[iCount++] = pChar;
	}

	CWorldSearch AreaItem( GetTopPoint(), iMaxDist );
	while ( iCount < MAX_MULTI_LIST_OBJS )
	{
		CItem * pItem = AreaItem.GetItem();
		if ( pItem == NULL )
			break;
		if ( pItem == this )	// already listed.
			continue;
		if ( ! Multi_IsPartOf( pItem ))
		{
			if ( ! m_pRegion->IsInside2d( pItem->GetTopPoint()))
				continue;
			if ( ! pItem->IsMovable())
				continue;
			int zdiff = pItem->GetTopZ() - GetTopZ();
			if ( abs( zdiff ) > 3 )
				continue;
		}
		ppObjList[iCount++] = pItem;
	}
	return( iCount );
}

bool CItemShip::Ship_MoveDelta( CPointBase pdelta )
{
	ADDTOCALLSTACK("CItemShip::Ship_MoveDelta");
	// Move the ship one space in some direction.

	ASSERT( m_pRegion->m_iLinkedSectors );

	int znew = GetTopZ() + pdelta.m_z;
	if ( pdelta.m_z > 0 )
	{
		if ( znew >= (UO_SIZE_Z - PLAYER_HEIGHT )-1 )
			return( false );
	}
	else if ( pdelta.m_z < 0 )
	{
		if ( znew <= (UO_SIZE_MIN_Z + 3 ))
			return( false );
	}

	// Move the ship and everything on the deck
	CObjBase * ppObjs[MAX_MULTI_LIST_OBJS+1];
	int iCount = Ship_ListObjs( ppObjs );

	for ( int i=0; i <iCount; i++ )
	{
		CObjBase * pObj = ppObjs[i];
		ASSERT(pObj);
		CPointMap pt = pObj->GetTopPoint();
		pt += pdelta;
		pt.m_map = pObj->GetTopPoint().m_map;
		if ( ! pt.IsValidPoint())  // boat goes out of bounds !
		{
			DEBUG_ERR(( "Ship uid=0%x out of bounds\n", (DWORD) GetUID()));
			continue;
		}
		pObj->MoveTo( pt );
		if ( pObj->IsChar() && !pObj->IsDisconnected() )
		{
			ASSERT( m_pRegion->m_iLinkedSectors );
			pObj->RemoveFromView(); // Get rid of the blink/walk
			pObj->Update();
		}
	}

	return( true );
}

bool CItemShip::Ship_CanMoveTo( const CPointMap & pt ) const
{
	ADDTOCALLSTACK("CItemShip::Ship_CanMoveTo");
	// Can we move to the new location ? all water type ?
	if ( IsAttr(ATTR_MAGIC ))
		return( true );

	WORD wBlockFlags = CAN_I_WATER;
	signed char z = g_World.GetHeightPoint( pt, wBlockFlags, true );
	if ( ! ( wBlockFlags & CAN_I_WATER ))
		return false;

	return true;
}

static const DIR_TYPE sm_Ship_FaceDir[] =
{
	DIR_N,
	DIR_E,
	DIR_S,
	DIR_W,
};

bool CItemShip::Ship_Face( DIR_TYPE dir )
{
	ADDTOCALLSTACK("CItemShip::Ship_Face");
	// Change the direction of the ship.

	if ( !IsTopLevel() || !m_pRegion ) {
		return false;
	}

	int i = 0;
	for ( ; true; ++i )
	{
		if ( i >= COUNTOF(sm_Ship_FaceDir))
			return( false );
		if ( dir == sm_Ship_FaceDir[i] )
			break;
	}

	int iFaceOffset = Ship_GetFaceOffset();
	ITEMID_TYPE idnew = (ITEMID_TYPE) ( GetID() - iFaceOffset + i );
	const CItemBaseMulti * pMultiNew = Multi_GetDef( idnew );
	if ( pMultiNew == NULL ) {
		return false;
	}

	const CItemBaseMulti * pMultiDef = Multi_GetDef();
	ASSERT( pMultiDef);

	int iTurn = dir - sm_Ship_FaceDir[ iFaceOffset ];

	// ?? Are there blocking items in the way of the turn ?

	// Acquire the CRectMap for the new direction of the ship
	CRectMap rect;
	reinterpret_cast<CGRect&>(rect) = pMultiNew->m_rect;
	rect.m_map = GetTopPoint().m_map;
	rect.OffsetRect( GetTopPoint().m_x, GetTopPoint().m_y );

	// Check that we can fit into this space.
	CPointMap ptTmp;
	DIR_TYPE dirTmp;
	for( i = -3; i < 3; ++i ) {
		if (i == 0) // We don't need to check forwards
			continue;

		dirTmp = GetDirTurn((DIR_TYPE)this->m_itShip.m_DirFace, i);
		ptTmp = rect.GetRectCorner(dirTmp);
		ptTmp.m_z = GetTopZ();

		// If the ship already overlaps a point then we must
		// already be allowed there.
		if( !m_pRegion->IsInside2d(ptTmp) && !Ship_CanMoveTo(ptTmp) ) {
			CItem *pTiller = Multi_GetSign();
			ASSERT(pTiller);
			pTiller->Speak("We cannot turn that way Cap'n", 0, TALKMODE_SAY, FONT_NORMAL);
			return false;
		}
	}

	// Reorient everything on the deck
	CObjBase * ppObjs[MAX_MULTI_LIST_OBJS+1];
	int iCount = Ship_ListObjs( ppObjs );
	for( i = 0; i < iCount; ++i ) {
		CObjBase *pObj = ppObjs[i];

		if( pObj->IsItem() ) {
			CItem * pItem = STATIC_CAST<CItem*>(pObj);
			//	change to new view of this ship
			if ( pItem == this ) {
				m_pRegion->UnRealizeRegion();
				pItem->SetID(idnew);
				// Adjust the region to be the new shape/area.
				MultiRealizeRegion();
				pItem->Update();
				//	create all needed components
				DWORD privateUID = Multi_GetSign()->m_itKey.m_lockUID.GetPrivateUID();
				for( int j = 0; j < pMultiNew->m_Components.GetCount(); j++ ) {
					Multi_CreateComponent((ITEMID_TYPE)pMultiNew->m_Components[j].m_id,
						pMultiNew->m_Components[j].m_dx,
						pMultiNew->m_Components[j].m_dy,
						pMultiNew->m_Components[j].m_dz,
						privateUID);
				}
				continue;
			}
			else if ( Multi_IsPartOf(pItem) ) {
				//	this will be deleted later
			}
		}
		else if( pObj->IsChar() ) {
			CChar *pChar = STATIC_CAST<CChar*>(pObj);
			pChar->m_dirFace = GetDirTurn(pChar->m_dirFace, iTurn);
			pChar->RemoveFromView();
		}
		CPointMap pt = pObj->GetTopPoint();
		
		int iTmp;
		int xdiff = GetTopPoint().m_x - pt.m_x;
		int ydiff = GetTopPoint().m_y - pt.m_y;
		switch ( iTurn )
		{
		case 2: // right
		case (2-DIR_QTY):
			iTmp = xdiff;
			xdiff = ydiff;
			ydiff = -iTmp;
			break;
		case -2: // left.
		case (DIR_QTY-2):
			iTmp = xdiff;
			xdiff = -ydiff;
			ydiff = iTmp;
			break;
		default: // u turn.
			xdiff = -xdiff;
			ydiff = -ydiff;
			break;
		}
		pt.m_x = GetTopPoint().m_x + xdiff;
		pt.m_y = GetTopPoint().m_y + ydiff;
		pObj->MoveTo(pt);
		if ( !pObj->IsDisconnected() )
			pObj->Update();
	}

	//	delete old components
	for ( int j = iCount-1; j > 0; j-- )
	{
		if ( !ppObjs[j]->IsItem() )
			continue;

		CItem * pItem = dynamic_cast<CItem*>( ppObjs[j] );
		if ( !Multi_IsPartOf( pItem ) )
			continue;

		if ( pItem->IsContainer() )
		{
			CItemContainer * pItemCont = dynamic_cast<CItemContainer*>( pItem );
			pItemCont->ContentsTransfer( GetShipHold(), false );
		}

		pItem->Delete();
	}
	m_itShip.m_DirFace = dir;
	return true;
}

bool CItemShip::Ship_Move( DIR_TYPE dir )
{
	ADDTOCALLSTACK("CItemShip::Ship_Move");
	if ( dir >= DIR_QTY )
		return( false );

	if ( m_pRegion == NULL )
	{
		DEBUG_ERR(( "Ship bad region\n" ));
		return false;
	}

	CPointMap ptDelta;
	ptDelta.ZeroPoint();
	ptDelta.Move( dir );

	CPointMap ptForePrv = m_pRegion->GetRegionCorner(dir);
	CPointMap ptFore = ptForePrv;
	ptFore.Move( dir );
	ptFore.m_z = GetTopZ();

	if ( ! ptFore.IsValidPoint() ||
		( ptForePrv.m_x < UO_SIZE_X_REAL && ptFore.m_x >= UO_SIZE_X_REAL && ( ptFore.m_map <= 1 )))
	{
		// Circle the globe
		// Fall off edge of world ?
		CItem * pTiller = Multi_GetSign();
		ASSERT(pTiller);
		pTiller->Speak( "Turbulent waters Cap'n", 0, TALKMODE_SAY, FONT_NORMAL );
		return false;
	}

	// We should check all relevant corners.
	if ( ! Ship_CanMoveTo( ptFore ))
	{
cantmove:
		CItem * pTiller = Multi_GetSign();
		ASSERT(pTiller);
		pTiller->Speak( "We've stopped Cap'n", 0, TALKMODE_SAY, FONT_NORMAL );
		return false;
	}

	// left side
	CPointMap ptTmp = m_pRegion->GetRegionCorner(GetDirTurn(dir,-1));
	ptTmp.Move( dir );
	ptTmp.m_z = GetTopZ();
	if ( ! Ship_CanMoveTo( ptTmp ))
		goto cantmove;

	// right side.
	ptTmp = m_pRegion->GetRegionCorner(GetDirTurn(dir,+1));
	ptTmp.Move( dir );
	ptTmp.m_z = GetTopZ();
	if ( ! Ship_CanMoveTo( ptTmp ))
		goto cantmove;

	Ship_MoveDelta( ptDelta );

	// Move again
	GetTopSector()->SetSectorWakeStatus();	// may get here b4 my client does.
	return true;
}

bool CItemShip::Ship_OnMoveTick()
{
	ADDTOCALLSTACK("CItemShip::Ship_OnMoveTick");
	// We just got a move tick.
	// RETURN: false = delete the boat.

	if ( ! m_itShip.m_fSail )	// decay the ship instead ???
		return( true );

	// Calculate the leading point.
	DIR_TYPE dir = (DIR_TYPE) m_itShip.m_DirMove;
	if ( ! Ship_Move( dir ))
	{
		Ship_Stop();
		return( true );
	}

	SetTimeout(( m_itShip.m_fSail == 1 ) ? 1*TICK_PER_SEC : (TICK_PER_SEC/2));
	return( true );
}

bool CItemShip::OnTick()
{
	ADDTOCALLSTACK("CItemShip::OnTick");

	// Ships move on their tick.
	Ship_OnMoveTick();
	return true;
}

enum
{
	SHV_SHIPANCHORDROP,
	SHV_SHIPANCHORRAISE,
	SHV_SHIPBACK,
	SHV_SHIPBACKLEFT,
	SHV_SHIPBACKRIGHT,
	SHV_SHIPDOWN,
	SHV_SHIPDRIFTLEFT,
	SHV_SHIPDRIFTRIGHT,
	SHV_SHIPFACE,
	SHV_SHIPFORE,
	SHV_SHIPFORELEFT,
	SHV_SHIPFORERIGHT,
	SHV_SHIPGATE,
	SHV_SHIPLAND,
	SHV_SHIPMOVE,
	SHV_SHIPSTOP,
	SHV_SHIPTURN,
	SHV_SHIPTURNLEFT,
	SHV_SHIPTURNRIGHT,
	SHV_SHIPUP,
	SHV_QTY,
};

LPCTSTR const CItemShip::sm_szVerbKeys[SHV_QTY+1] =
{
	"SHIPANCHORDROP",
	"SHIPANCHORRAISE",
	"SHIPBACK",
	"SHIPBACKLEFT",
	"SHIPBACKRIGHT",
	"SHIPDOWN",		// down one space.
	"SHIPDRIFTLEFT",
	"SHIPDRIFTRIGHT",
	"SHIPFACE",		// set the ships facing direction.
	"SHIPFORE",
	"SHIPFORELEFT",
	"SHIPFORERIGHT",
	"SHIPGATE",		// Moves the whole ship to some new point location.
	"SHIPLAND",
	"SHIPMOVE",		// move in a specified direction.
	"SHIPSTOP",
	"SHIPTURN",
	"SHIPTURNLEFT",
	"SHIPTURNRIGHT",
	"SHIPUP",		// up one space.
	NULL,
};

bool CItemShip::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CItemShip::r_GetRef");

	if ( !strnicmp( pszKey, "HATCH.", 6 ) )
	{
		pszKey += 6;
		pRef = GetShipHold();
		return true;
	}
	else if ( !strnicmp( pszKey, "TILLER.", 7 ) )
	{
		pszKey += 7;
		pRef = Multi_GetSign();
		return true;
	}
	else if ( !strnicmp( pszKey, "PLANK.", 6 ) )
	{
		pszKey += 6;
		int i = Exp_GetVal( pszKey );
		SKIP_SEPARATORS(pszKey);
		pRef = GetShipPlank(i);
		return true;
	}

	return( CItemMulti::r_GetRef( pszKey, pRef ));
}

bool CItemShip::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CItemShip::r_Verb");
	EXC_TRY("Verb");
	// Speaking in this ships region.
	// return: true = command for the ship.

	//"One (direction*)", " (Direction*), one" Moves ship one tile in desired direction and stops.
	//"Slow (direction*)" Moves ship slowly in desired direction (see below for possible directions).

	int iCmd = FindTableSorted( s.GetKey(), sm_szVerbKeys, COUNTOF( sm_szVerbKeys )-1 );
	if ( iCmd < 0 )
	{
		return( CItemMulti::r_Verb( s, pSrc ));
	}

	if ( ! pSrc )
		return( false );

	if ( IsAttr(ATTR_MOVE_NEVER) || ! IsTopLevel() )
		return false;

	CChar * pChar = pSrc->GetChar();

	// Only key holders can command the ship ???
	// if ( pChar && pChar->ContentFindKeyFor( pItem ))

	// Find the tiller man object.
	CItem * pTiller = Multi_GetSign();
	ASSERT( pTiller );

	// Get current facing dir.
	DIR_TYPE DirFace = sm_Ship_FaceDir[ Ship_GetFaceOffset() ];
	int DirMoveChange;
	LPCTSTR pszSpeak = NULL;

	switch ( iCmd )
	{
		case SHV_SHIPSTOP:
		{
			// "Furl sail"
			// "Stop" Stops current ship movement.
			if ( ! m_itShip.m_fSail )
				return( false );
			Ship_Stop();
			break;
		}

		case SHV_SHIPFACE:
		{
			// Face this direction. do not change the direction of movement.
			if ( ! s.HasArgs())
				return( false );
			return Ship_Face( GetDirStr( s.GetArgStr()));
		}

		case SHV_SHIPMOVE:
		{
			// Move one space in this direction.
			// Does NOT protect against exploits !
			if ( ! s.HasArgs())
				return( false );
			m_itShip.m_DirMove = GetDirStr( s.GetArgStr());
			return Ship_Move( (DIR_TYPE) m_itShip.m_DirMove );
		}

		case SHV_SHIPGATE:
		{
			// Move the whole ship and contents to another place.
			if ( ! s.HasArgs())
				return( false );

			CPointMap ptdelta = g_Cfg.GetRegionPoint( s.GetArgStr());
			if ( ! ptdelta.IsValidPoint())
				return( false );
			ptdelta -= GetTopPoint();
			return Ship_MoveDelta( ptdelta );
		}

		case SHV_SHIPTURNLEFT:
		{
			// "Port",
			// "Turn Left",
			DirMoveChange = -2;
doturn:
			if ( m_itShip.m_fAnchored )
			{
anchored:
				pszSpeak = "The anchor is down <SEX Sir/Mam>!";
				break;
			}
			m_itShip.m_DirMove = GetDirTurn( DirFace, DirMoveChange );
			if (! Ship_Face( (DIR_TYPE) m_itShip.m_DirMove ) )
				return false;
			break;
		}

		case SHV_SHIPTURNRIGHT:
		{
			// "Starboard",
			// "Turn Right",
			DirMoveChange = 2;
			goto doturn;
		}

		case SHV_SHIPDRIFTLEFT:
		{
			// "Left",
			// "Drift Left",
			DirMoveChange = -2;
dodirmovechange:
			if ( m_itShip.m_fAnchored )
				goto anchored;
			if ( ! Ship_SetMoveDir( GetDirTurn( DirFace, DirMoveChange )))
				return( false );
			break;
		}

		case SHV_SHIPDRIFTRIGHT:
		{
			// "Right",
			// "Drift Right",
			DirMoveChange = 2;
			goto dodirmovechange;
		}

		case SHV_SHIPBACK: 
		{
			// "Back",			// Move ship backwards
			// "Backward",		// Move ship backwards
			// "Backwards",	// Move ship backwards
			DirMoveChange = 4;
			goto dodirmovechange;
		}

		case SHV_SHIPFORE:
		{
			// "Forward"
			// "Foreward",		// Moves ship forward.
			// "Unfurl sail",	// Moves ship forward.
			DirMoveChange = 0;
			goto dodirmovechange;
		}

		case SHV_SHIPFORELEFT: // "Forward left",
		{
			DirMoveChange = -1;
			goto dodirmovechange;
		}

		case SHV_SHIPFORERIGHT: // "forward right",
		{
			DirMoveChange = 1;
			goto dodirmovechange;
		}

		case SHV_SHIPBACKLEFT:
		{
			// "backward left",
			// "back left",
			DirMoveChange = -3;
			goto dodirmovechange;
		}

		case SHV_SHIPBACKRIGHT:
		{
			// "backward right",
			// "back right",
			DirMoveChange = 3;
			goto dodirmovechange;
		}

		case SHV_SHIPANCHORRAISE: // "Raise Anchor",
		{
			if ( ! m_itShip.m_fAnchored )
			{
				pszSpeak = "The anchor is already up <SEX Sir/Mam>";
				break;
			}
			m_itShip.m_fAnchored = false;
			break;
		}

		case SHV_SHIPANCHORDROP: // "Drop Anchor",
		{
			if ( m_itShip.m_fAnchored )
			{
				pszSpeak = "The anchor is already down <SEX Sir/Mam>";
				break;
			}
			m_itShip.m_fAnchored = true;
			Ship_Stop();
			break;
		}

		case SHV_SHIPTURN:
		{
			//	"Turn around",	// Turns ship around and proceeds.
			// "Come about",	// Turns ship around and proceeds.
			DirMoveChange = 4;
			goto doturn;
		}

		case SHV_SHIPUP: // "Up"
		{
			if ( ! IsAttr(ATTR_MAGIC ))
				return( false );

			CPointMap pt;
			pt.m_z = PLAYER_HEIGHT;
			if ( Ship_MoveDelta( pt ))
			{
				pszSpeak = "As you command <SEX Sir/Mam>";
			}
			else
			{
				pszSpeak = "Can't do that <SEX Sir/Mam>";
			}
			break;
		}

		case SHV_SHIPDOWN: // "Down"
		{
			if ( ! IsAttr(ATTR_MAGIC ))
				return( false );
			CPointMap pt;
			pt.m_z = -PLAYER_HEIGHT;
			if ( Ship_MoveDelta( pt ))
			{
				pszSpeak = "As you command <SEX Sir/Mam>";
			}
			else
			{
				pszSpeak = "Can't do that <SEX Sir/Mam>";
			}
			break;
		}

		case SHV_SHIPLAND: // "Land"
		{
			if ( ! IsAttr(ATTR_MAGIC ))
				return( false );
			signed char zold = GetTopZ();
			CPointMap pt = GetTopPoint();
			pt.m_z = zold;
			SetTopZ( -UO_SIZE_Z );	// bottom of the world where i won't get in the way.
			WORD wBlockFlags = CAN_I_WATER;
			signed char z = g_World.GetHeightPoint( pt, wBlockFlags );
			SetTopZ( zold );	// restore z for now.
			pt.InitPoint();
			pt.m_z = z - zold;
			if ( pt.m_z )
			{
				Ship_MoveDelta( pt );
				pszSpeak = "As you command <SEX Sir/Mam>";
			}
			else
			{
				pszSpeak = "We have landed <SEX Sir/Mam>";
			}
			break;
		}

		default:
		{
			return false ;
		}
	}

	if ( pChar )
	{
		if ( pszSpeak == NULL )
		{
			static LPCTSTR const sm_pszAye[] =
			{
				"Aye",
				"Aye Cap'n",
				"Aye <SEX Sir/Mam>",
			};
			pszSpeak = sm_pszAye[ Calc_GetRandVal( COUNTOF( sm_pszAye )) ];
		}

		TCHAR szText[ MAX_TALK_BUFFER ];
		strcpy( szText, pszSpeak );
		pChar->ParseText( szText, &g_Serv );
		pTiller->Speak( szText, 0, TALKMODE_SAY, FONT_NORMAL );
	}
	return false;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return true;
}

void CItemShip::r_Write( CScript & s )
{
	ADDTOCALLSTACK("CItemShip::r_Write");
	CItemMulti::r_Write(s);
	if ( m_uidHold )
		s.WriteKeyHex("HATCH", m_uidHold );
	if ( GetShipPlankCount() )
	{
		for ( int i = 0; i < m_uidPlanks.size(); i++ )
			s.WriteKeyHex("PLANK", m_uidPlanks.at(i));
	}
}
bool CItemShip::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CItemShip::r_WriteVal");

	if ( !strcmpi( pszKey, "HATCH" ) )
	{
		pszKey += 5;
		CItem * pItemHold = GetShipHold();
		if ( pItemHold )
			sVal.FormatHex( pItemHold->GetUID() );
		else
			sVal.FormatVal( 0 );
		return true;
	}
	else if ( !strcmpi( pszKey, "TILLER" ) )
	{
		pszKey += 6;
		CItem * pTiller = Multi_GetSign();
		if ( pTiller )
			sVal.FormatHex( pTiller->GetUID() );
		else
			sVal.FormatVal( 0 );
		return true;
	}
	else if ( !strcmpi( pszKey, "PLANKS" ) )
	{
		pszKey += 6;
		sVal.FormatVal( GetShipPlankCount() );
		return true;
	}
	return( CItemMulti::r_WriteVal(pszKey, sVal, pSrc));
}

bool CItemShip::r_LoadVal( CScript & s  )
{
	ADDTOCALLSTACK("CItemShip::r_LoadVal");
	EXC_TRY("LoadVal");

	if ( g_Serv.IsLoading() )
	{
		if ( s.IsKey("HATCH") )
		{
			m_uidHold = s.GetArgVal();
			return true;
		}
		else if ( s.IsKey("TILLER") )
		{
			m_uidLink = s.GetArgVal();
			return true;
		}
		else if ( s.IsKey("PLANK") )
		{
			CGrayUID uid = s.GetArgVal();
			m_uidPlanks.push_back( uid );
			return true;
		}
	}
	return CItemMulti::r_LoadVal(s);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

int CItemShip::FixWeirdness()
{
	ADDTOCALLSTACK("CItemShip::FixWeirdness");
	int iResultCode = CItemMulti::FixWeirdness();
	if ( iResultCode )
	{
		return iResultCode;
	}

	// CItemShip::GetShipHold() updates/corrects the hold uid
	GetShipHold();

	// CItemShip::GetShipPlank() updates/corrects the list of planks
	GetShipPlank(-1);
	return iResultCode;
}

CItemContainer * CItemShip::GetShipHold()
{
	ADDTOCALLSTACK("CItemShip::GetShipHold");
	CItem * pItem = m_uidHold.ItemFind();
	if ( !pItem || pItem->IsDeleted() )
	{
		pItem = Multi_FindItemType( IT_SHIP_HOLD );
		if ( !pItem || pItem->IsDeleted() )
			pItem = Multi_FindItemType( IT_SHIP_HOLD_LOCK );

		if ( !pItem || pItem->IsDeleted() )
			return NULL;

		m_uidHold = pItem->GetUID();
	}

	CItemContainer * pItemHold = dynamic_cast<CItemContainer *>( pItem );
	if ( !pItemHold )
		return NULL;

	return pItemHold;
}

int CItemShip::GetShipPlankCount()
{
	ADDTOCALLSTACK("CItemShip::GetShipPlankCount");
	// CItemShip::GetShipPlank() updates the list of planks, so
	// calling this first will get an accurate result
	GetShipPlank(-1);
	return m_uidPlanks.size();
}

CItem * CItemShip::GetShipPlank(int index)
{
	ADDTOCALLSTACK("CItemShip::GetShipPlank");
	// Check the current list of planks is valid
	for ( vector<CGrayUID>::iterator i = m_uidPlanks.begin(); i != m_uidPlanks.end(); ++i )
	{
		CItem * pItem = (*i).ItemFind();
		if ( pItem && Multi_IsPartOf( pItem ) )
			continue;

		// If an invalid plank uid was found, then wipe the whole list
		// and rebuild it
		m_uidPlanks.clear();
		break;
	}

	// Find plank(s) if the list is empty
	if ( m_uidPlanks.empty() )
	{
		CWorldSearch Area( GetTopPoint(), Multi_GetMaxDist() );
		while (true)
		{
			CItem * pItem = Area.GetItem();
			if ( pItem == NULL )
				break;

			if ( pItem->IsDeleted() )
				continue;

			if ( !Multi_IsPartOf( pItem ) )
				continue;

			if ( pItem->IsType( IT_SHIP_PLANK ) || pItem->IsType( IT_SHIP_SIDE ) || pItem->IsType( IT_SHIP_SIDE_LOCKED ) )
				m_uidPlanks.push_back( pItem->GetUID() );
		}
	}

	if (( index < 0 ) || ( index >= m_uidPlanks.size() ))
		return NULL;

	CGrayUID uid = m_uidPlanks.at(index);
	return uid.ItemFind();
}

void CItemShip::OnComponentCreate( const CItem * pComponent )
{
	ADDTOCALLSTACK("CItemShip::OnComponentCreate");
	switch ( pComponent->GetType() )
	{
		case IT_SHIP_TILLER:
			// Tillerman is already stored as m_uidLink (Multi_GetSign())
			break;
		case IT_SHIP_HOLD:
		case IT_SHIP_HOLD_LOCK:
			m_uidHold = pComponent->GetUID();
			break;
		case IT_SHIP_PLANK:
		case IT_SHIP_SIDE:
		case IT_SHIP_SIDE_LOCKED:
			m_uidPlanks.push_back( pComponent->GetUID() );
			break;
	}

	CItemMulti::OnComponentCreate( pComponent );
	return;
}

