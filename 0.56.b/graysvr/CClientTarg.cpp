//
// CClientTarg.cpp
// Copyright Menace Software (www.menasoft.com).
//
// An item is targetted.

#include "graysvr.h"	// predef header.
#include "CClient.h"
#include "../network/send.h"

////////////////////////////////////////////////////////
// Targetted GM functions.

bool CClient::OnTarg_Obj_Set( CObjBase * pObj )
{
	ADDTOCALLSTACK("CClient::OnTarg_Obj_Set");
	// CLIMODE_TARG_OBJ_SET
	// Targeted a command at an CObjBase object
	// ARGS:
	//  m_Targ_Text = new command and value.

	if ( pObj == NULL )
	{
		SysMessage( g_Cfg.GetDefaultMsg(DEFMSG_TARG_UNEXPECTED) );
		return( false );
	}

	// Parse the command.
	TCHAR * pszLogMsg = Str_GetTemp();

	if ( pObj->IsItem() )
	{
		const CItem * pItem = STATIC_CAST <CItem*> (pObj);
		if ( pItem->GetAmount() > 1 )
			sprintf(pszLogMsg, "'%s' commands uid=0%lx (%s) [amount=%u] to '%s'", static_cast<LPCTSTR>(GetName()), static_cast<DWORD>(pObj->GetUID()), static_cast<LPCTSTR>(pObj->GetName()), pItem->GetAmount(), static_cast<LPCTSTR>(m_Targ_Text));
		else
			sprintf(pszLogMsg, "'%s' commands uid=0%lx (%s) to '%s'", static_cast<LPCTSTR>(GetName()), static_cast<DWORD>(pObj->GetUID()), static_cast<LPCTSTR>(pObj->GetName()), static_cast<LPCTSTR>(m_Targ_Text));
	}
	else
		sprintf(pszLogMsg, "'%s' commands uid=0%lx (%s) to '%s'", static_cast<LPCTSTR>(GetName()), static_cast<DWORD>(pObj->GetUID()), static_cast<LPCTSTR>(pObj->GetName()), static_cast<LPCTSTR>(m_Targ_Text));

	// Check priv level for the new verb.
	if ( ! g_Cfg.CanUsePrivVerb( pObj, m_Targ_Text, this ))
	{
		SysMessageDefault( DEFMSG_CMD_LACKPRIV );
		g_Log.Event( LOGM_GM_CMDS, "%s=0\n", pszLogMsg);
		return( false );
	}

	CScript sCmd( m_Targ_Text );

	if ( sCmd.IsKey("COLOR") )
	{
		// ".xCOLOR" command without arguments displays dye option
		if ( !sCmd.HasArgs() )
		{
			addDyeOption( pObj );
			return true;
		}
	}

	bool fRet = pObj->r_Verb( sCmd, this );
	if ( ! fRet )
	{
		SysMessageDefault( DEFMSG_ERR_INVSET );
	}

	if ( GetPrivLevel() >= g_Cfg.m_iCommandLog )
		g_Log.Event( LOGM_GM_CMDS, "%s=%d\n", pszLogMsg, fRet);
	return( fRet );
}


bool CClient::OnTarg_Obj_Function( CObjBase * pObj, const CPointMap & pt, ITEMID_TYPE id )
{
	ADDTOCALLSTACK("CClient::OnTarg_Obj_Function");
	m_Targ_p	= pt;
	LPCTSTR	pSpace	= strchr( m_Targ_Text, ' ' );
	if ( !pSpace )
		pSpace	= strchr( m_Targ_Text, '\t' );
	if ( pSpace )
		GETNONWHITESPACE( pSpace );

	CScriptTriggerArgs	Args( pSpace ? pSpace : "" );
	Args.m_VarsLocal.SetNum( "ID", id, true );
	Args.m_pO1	= pObj;
	CGString sVal;
	m_pChar->r_Call( static_cast<LPCTSTR>(m_Targ_Text), this, &Args, &sVal );
	return true;
}


bool CClient::OnTarg_Obj_Info( CObjBase * pObj, const CPointMap & pt, ITEMID_TYPE id )
{
	ADDTOCALLSTACK("CClient::OnTarg_Obj_Info");
	// CLIMODE_TARG_OBJ_INFO "INFO"

	if ( pObj )
	{
		SetTargMode();
		addGumpDialogProps(pObj->GetUID());
	}
	else
	{
		TCHAR *pszTemp = Str_GetTemp();
		size_t len = 0;
		if ( id )
		{
			len = sprintf( pszTemp, "[Static z=%d, 0%x=", pt.m_z, id );

			// static items have no uid's but we can still use them.
			CItemBase * pItemDef = CItemBase::FindItemBase(id);
			if ( pItemDef )
			{
				len += sprintf( pszTemp+len, "%s->%s], ", pItemDef->GetResourceName(),
					g_Cfg.ResourceGetName( RESOURCE_ID( RES_TYPEDEF, pItemDef->GetType() )));
			}
			else
			{
				len += sprintf( pszTemp+len, "NON scripted], " );
			}
		}
		else
		{
			// tile info for location.
			len = strcpylen( pszTemp, "[No static tile], " );
		}

		const CUOMapMeter * pMeter = g_World.GetMapMeter( pt );
		if ( pMeter )
		{
			len += sprintf( pszTemp+len, "TERRAIN=0%x   TYPE=%s",
				pMeter->m_wTerrainIndex,
				g_World.GetTerrainItemTypeDef( pMeter->m_wTerrainIndex )->GetResourceName() );
		}

		SysMessage(pszTemp);
	}

	return true;
}

bool CClient::Cmd_Control( CChar * pChar2 )
{
	ADDTOCALLSTACK("CClient::Cmd_Control");
	// I wish to control pChar2
	// Leave my own body behind.

	if ( pChar2 == NULL )
		return false;
	if ( pChar2->IsDisconnected())
		return( false );	// char is not on-line. (then we should make it so !)

	if ( GetPrivLevel() < pChar2->GetPrivLevel())
		return false;

	ASSERT(m_pChar);
	CChar * pChar1 = m_pChar;

	// Put my newbie equipped items on it.
	CItem* pItemNext;
	for ( CItem* pItem=pChar1->GetContentHead(); pItem!=NULL; pItem=pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( ! pItem->IsAttr(ATTR_MOVE_NEVER))
			continue; // keep GM stuff.
		if ( ! CItemBase::IsVisibleLayer( pItem->GetEquipLayer()))
			continue;
		switch ( pItem->GetEquipLayer())
		{
			case LAYER_BEARD:
			case LAYER_HAIR:
			case LAYER_PACK:
				continue;

			default:
				break;
		}
		pChar2->LayerAdd( pItem );	// add content
	}

	// Put my GM pack stuff in it's inventory.
	CItemContainer * pPack1 = pChar1->GetPack();
	if ( pPack1 != NULL )
	{
		CItemContainer * pPack2 = pChar2->GetPackSafe();
		CItem* pItemNext;
		for ( CItem* pItem=pPack1->GetContentHead(); pItem!=NULL; pItem=pItemNext )
		{
			pItemNext = pItem->GetNext();
			if ( ! pItem->IsAttr(ATTR_MOVE_NEVER))
				continue; // keep newbie stuff.
			pPack2->ContentAdd( pItem );	// add content
		}
	}

	pChar1->ClientDetach();
	m_pChar = NULL;
	CClient * pClient2 = pChar2->GetClient();
	if ( pClient2 )	// controled char is a player/client.
	{
		pChar2->ClientDetach();
		pClient2->m_pChar = NULL;
	}

	CCharPlayer * pPlayer1 = pChar1->m_pPlayer;
	if ( pPlayer1 )
	{
		pPlayer1->GetAccount()->DetachChar(pChar1);
	}
	CCharPlayer * pPlayer2 = pChar2->m_pPlayer;
	if ( pPlayer2 )
	{
		pPlayer2->GetAccount()->DetachChar(pChar2);
	}

	// swap m_pPlayer (if they even are both players.)

	pChar1->m_pPlayer = pPlayer2;
	pChar2->m_pPlayer = pPlayer1;

	ASSERT( pChar1->m_pNPC == NULL );
	pChar1->m_pNPC = pChar2->m_pNPC;	// Turn my old body into a NPC. (if it was)
	pChar2->m_pNPC = NULL;

	if ( pPlayer1 )
	{
		pPlayer1->GetAccount()->AttachChar(pChar2);
	}
	if ( pPlayer2 )
	{
		pPlayer2->GetAccount()->AttachChar(pChar1);
	}
	if ( pClient2 )
	{
		pClient2->addPlayerStart( pChar1 );
	}
	else
	{
		// delete my ghost.
		if ( pChar1->GetID() == CREID_EQUIP_GM_ROBE ||
			pChar1->GetID() == CREID_GHOSTMAN ||
			pChar1->GetID() == CREID_GHOSTWOMAN ||
			pChar1->GetID() == CREID_ELFGHOSTMAN || 
			pChar1->GetID() == CREID_ELFGHOSTWOMAN ||
			pChar1->GetID() == CREID_GARGGHOSTMAN || 
			pChar1->GetID() == CREID_GARGGHOSTWOMAN )	// CREID_EQUIP_GM_ROBE
		{
			pChar1->Delete();
		}
		else
		{
			pChar1->SetTimeout(1);	// must kick start the npc.
		}
	}
	addPlayerStart( pChar2 );
	return true;
}

bool CClient::OnTarg_UnExtract( CObjBase * pObj, const CPointMap & pt )
{
	ADDTOCALLSTACK("CClient::OnTarg_UnExtract");
	UNREFERENCED_PARAMETER(pObj);
	// CLIMODE_TARG_UNEXTRACT
	// ??? Get rid of this in favor of a more .SCP file type approach.
	// result of the MULTI command.
	// Break a multi out of the multi.txt files and turn it into items.

	if ( ! pt.IsValidPoint())
		return( false );

	CScript s;	// It is not really a valid script type file.
	if ( ! g_Cfg.OpenResourceFind( s, m_Targ_Text ))
		return false;

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "%i template id", m_tmTile.m_id);
	if ( ! s.FindTextHeader(pszTemp))
		return false;
	if ( ! s.ReadKey())
		return false; // throw this one away
	if ( ! s.ReadKeyParse())
		return false; // this has the item count

	int iItemCount = ATOI(s.GetKey()); // item count
	while (iItemCount > 0)
	{
		if ( ! s.ReadKeyParse())
			return false; // this has the item count

		INT64 piCmd[4];		// Maximum parameters in one line
		Str_ParseCmds( s.GetArgStr(), piCmd, COUNTOF(piCmd));

		CItem * pItem = CItem::CreateTemplate(static_cast<ITEMID_TYPE>(ATOI(s.GetKey())), NULL, m_pChar);
		if ( pItem == NULL )
			return( false );

		CPointMap ptOffset( static_cast<WORD>(piCmd[0]), static_cast<WORD>(piCmd[1]), static_cast<signed char>(piCmd[2]) );
		ptOffset += pt;
		ptOffset.m_map = pt.m_map;
		pItem->MoveToUpdate( ptOffset );
		iItemCount --;
	}

	return true;
}

bool CClient::OnTarg_Item_Add( CObjBase * pObj, const CPointMap & pt )
{
	ADDTOCALLSTACK("CClient::OnTarg_Item_Add");
	// CLIMODE_ADDITEM
	// m_tmAdd.m_id = new item id

	if ( ! pt.IsValidPoint())
		return( false );
	ASSERT( m_pChar );

	CItem * pItem = CItem::CreateTemplate( m_tmAdd.m_id, NULL, m_pChar );
	if ( pItem == NULL )
		return( false );
	if ( m_tmAdd.m_fStatic == 1)
	{
		// Lock this item down
		pItem->SetAttr( ATTR_MOVE_NEVER );
	}

	if ( pItem->IsTypeMulti() )
	{
		CItem * pItemNew = OnTarg_Use_Multi( pItem->Item_GetDef(), pt, pItem->m_Attr, pItem->GetHue());
		pItem->Delete();
		if ( pItemNew == NULL )
			return( false );
		pItem = pItemNew;
	}
	else
	{
		if ( pObj &&
			pObj->IsItemInContainer() &&
			m_pChar->CanUse( STATIC_CAST<CItem*>(pObj), true ))
		{
			pItem->MoveNearObj( pObj );
		}
		else
		{
			CPointMap ptNew = pt;
			ptNew.m_z ++;
			pItem->MoveToCheck( ptNew, m_pChar );
		}
	}

	m_pChar->m_Act_Targ = pItem->GetUID();	// for last target stuff. (trigger stuff)
	return true;
}

bool CClient::OnTarg_Item_Link( CObjBase * pObj2 )
{
	ADDTOCALLSTACK("CClient::OnTarg_Item_Link");
	// CLIMODE_LINK

	if ( pObj2 == NULL )
	{
		SysMessage( g_Cfg.GetDefaultMsg(DEFMSG_TARG_DYNAMIC) );
		return( false );
	}

	CItem * pItem2 = dynamic_cast <CItem*>(pObj2);
	CItem * pItem1 = m_Targ_UID.ItemFind();
	if ( pItem1 == NULL )
	{
		if ( pItem2 == NULL )
		{
			m_Targ_UID.InitUID();
			addTarget( CLIMODE_TARG_LINK, g_Cfg.GetDefaultMsg( DEFMSG_TARG_LINK ) );
		}
		else
		{
			m_Targ_UID = pObj2->GetUID();
			addTarget( CLIMODE_TARG_LINK, g_Cfg.GetDefaultMsg( DEFMSG_TARG_LINK2 ) );
		}
		return true;
	}

	if ( pItem2 == pItem1 )
	{
		SysMessageDefault( DEFMSG_TARG_LINK_SAME );
		// Break any existing links.
		return false;
	}

	if ( pItem2 && ( pItem1->IsType(IT_KEY) || pItem2->IsType(IT_KEY)))
	{
		// Linking a key to something is a special case.
		if ( ! pItem1->IsType(IT_KEY))
		{
			CItem * pTmp = pItem1;
			pItem1 = pItem2;
			pItem2 = pTmp;
		}
		// pItem1 = the IT_KEY
		if ( pItem2->m_itContainer.m_lockUID )
		{
			pItem1->m_itKey.m_lockUID = pItem2->m_itContainer.m_lockUID;
		}
		else if ( pItem1->m_itKey.m_lockUID )
		{
			pItem2->m_itContainer.m_lockUID = pItem1->m_itKey.m_lockUID;
		}
		else
		{
			pItem1->m_itKey.m_lockUID = pItem2->m_itContainer.m_lockUID = pItem2->GetUID();
		}
	}
	else
	{
		pItem1->m_uidLink = pObj2->GetUID();
		if ( pItem2 && ! pItem2->m_uidLink.IsValidUID())
		{
			pItem2->m_uidLink = pItem1->GetUID();
		}
	}

	SysMessageDefault( DEFMSG_TARG_LINKS );
	return true;
}

int CClient::Cmd_Extract( CScript * pScript, CRectMap &rect, int & zlowest )
{
	ADDTOCALLSTACK("CClient::Cmd_Extract");
	// RETURN: Number of statics here.
	CPointMap ptCtr = rect.GetCenter();

	int iCount = 0;
	for ( int mx = rect.m_left; mx <= rect.m_right; mx++)
	{
		for ( int my = rect.m_top; my <= rect.m_bottom; my++)
		{
			CPointMap ptCur(static_cast<WORD>(mx), static_cast<WORD>(my), 0, static_cast<unsigned char>(rect.m_map));
			const CGrayMapBlock * pBlock = g_World.GetMapBlock( ptCur );
			if ( pBlock == NULL )
				continue;
			size_t iQty = pBlock->m_Statics.GetStaticQty();
			if ( iQty <= 0 )  // no static items here.
				continue;

			int x2 = pBlock->GetOffsetX(mx);
			int y2 = pBlock->GetOffsetY(my);
			for ( size_t i = 0; i < iQty; i++ )
			{
				if ( ! pBlock->m_Statics.IsStaticPoint( i, x2, y2 ))
					continue;
				const CUOStaticItemRec * pStatic = pBlock->m_Statics.GetStatic(i);
				ASSERT(pStatic);
				iCount ++;
				if ( pScript )
				{
					// This static is at the coordinates in question.
					pScript->Printf( "%u %i %i %i 0\n",
						pStatic->GetDispID(), mx - ptCtr.m_x, my - ptCtr.m_y, pStatic->m_z - zlowest);
				}
				else
				{
					if ( pStatic->m_z < zlowest)
					{
						zlowest = pStatic->m_z;
					}
				}
			}
		}
	}

	// Extract Multi's ???


	// Extract dynamics as well.

	int rx = 1 + abs( rect.m_right - rect.m_left ) / 2;
	int ry = 1 + abs( rect.m_bottom - rect.m_top ) / 2;

	CWorldSearch AreaItem( ptCtr, maximum( rx, ry ));
	AreaItem.SetSearchSquare( true );
	for (;;)
	{
		CItem * pItem = AreaItem.GetItem();
		if ( pItem == NULL )
			break;
		if ( ! rect.IsInside2d( pItem->GetTopPoint()))
			continue;

		CPointMap pt = pItem->GetTopPoint();
		iCount ++;
		if ( pScript )
		{
			// This static is at the coordinates in question.
			pScript->Printf( "%u %i %i %i 0\n",
				pItem->GetDispID(), pt.m_x - ptCtr.m_x, pt.m_y - ptCtr.m_y, pt.m_z - zlowest );
		}
		else
		{
			if ( pt.m_z < zlowest)
			{
				zlowest = pt.m_z;
			}
		}
	}

	return iCount;
}

bool CClient::OnTarg_Tile( CObjBase * pObj, const CPointMap & pt )
{
	ADDTOCALLSTACK("CClient::OnTarg_Tile");
	// CLIMODE_TARG_TILE
	// m_tmTile.m_Code = CV_TILE etc
	// CV_NUKE. CV_NUKECHAR, CV_EXTRACT, CV_NUDGE
	//

	ASSERT(m_pChar);

	if ( pObj && ! pObj->IsTopLevel())
		return( false );
	if ( ! pt.IsValidPoint())
		return( false );

	if ( !m_tmTile.m_ptFirst.IsValidPoint())
	{
		m_tmTile.m_ptFirst = pt;
		addTarget( CLIMODE_TARG_TILE, g_Cfg.GetDefaultMsg( DEFMSG_TARG_PC ), true );
		return true;
	}

	if ( pt == m_tmTile.m_ptFirst && m_tmTile.m_Code != CV_EXTRACT ) // Extract can work with one square
	{
		SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_TILE_SAME_POINT ) );
		addTarget( CLIMODE_TARG_TILE, g_Cfg.GetDefaultMsg( DEFMSG_TARG_PC ), true );
		return true;
	}

	CRectMap rect;
	rect.SetRect( m_tmTile.m_ptFirst.m_x, m_tmTile.m_ptFirst.m_y, pt.m_x, pt.m_y, pt.m_map);
	CPointMap ptCtr = rect.GetCenter();
	ptCtr.m_map = pt.m_map;

	int rx = 1 + abs( rect.m_right - rect.m_left ) / 2;
	int ry = 1 + abs( rect.m_bottom - rect.m_top ) / 2;
	int iRadius = maximum( rx, ry );

	int iCount = 0;

	switch ( m_tmTile.m_Code )
	{
	case CV_EXTRACT:
		{
		// "EXTRACT" all map statics in the region.
		// First find the lowest Z to use as a base
		// and count the statics
		int zlowest = UO_SIZE_Z;
		iCount = Cmd_Extract( NULL, rect, zlowest );
		if ( iCount )
		{
			CScript s;
			if ( ! s.Open( m_Targ_Text, OF_WRITE|OF_TEXT|OF_DEFAULTMODE ))
				return( false );

			// Write a header for this multi in XXX format
			// (i have no idea what most of this means)
			s.Printf("6 version\n");
			s.Printf("%d template id\n", m_tmTile.m_id );
			s.Printf("-1 item version\n");
			s.Printf("%i num components\n", iCount);

			Cmd_Extract( &s, rect, zlowest );
		}

		SysMessagef( "%d Statics Extracted to '%s', id=%d", iCount, static_cast<LPCTSTR>(m_Targ_Text), m_tmTile.m_id );
		}
		break;

	case CV_NUDGE:
		{
			TCHAR szTmp[512];
			strcpylen( szTmp, m_Targ_Text, COUNTOF(szTmp));

			INT64 piArgs[3];		// Maximum parameters in one line
			Str_ParseCmds( szTmp, piArgs, COUNTOF( piArgs ));

			CPointMap ptNudge(static_cast<WORD>(piArgs[0]),static_cast<WORD>(piArgs[1]),static_cast<signed char>(piArgs[2]) );

			CWorldSearch AreaItem( ptCtr, iRadius );
			AreaItem.SetAllShow( IsPriv( PRIV_ALLSHOW ));
			AreaItem.SetSearchSquare( true );
			for (;;)
			{
				CItem * pItem = AreaItem.GetItem();
				if ( pItem == NULL )
					break;
				if ( ! rect.IsInside2d( pItem->GetTopPoint()))
					continue;
				CPointMap ptMove = pItem->GetTopPoint();
				ptMove += ptNudge;
				pItem->MoveToCheck( ptMove );
				iCount++;
			}

			CWorldSearch AreaChar( ptCtr, iRadius );
			AreaChar.SetAllShow( IsPriv( PRIV_ALLSHOW ));
			AreaChar.SetSearchSquare( true );
			for (;;)
			{
				CChar* pChar = AreaChar.GetChar();
				if ( pChar == NULL )
					break;
				if ( ! rect.IsInside2d( pChar->GetTopPoint()))
					continue;
				CPointMap ptMove = pChar->GetTopPoint();
				ptMove += ptNudge;
				pChar->m_fClimbUpdated = false; // update climb height
				pChar->MoveToChar( ptMove );
				iCount++;
			}

			SysMessagef( "%d %s", iCount, g_Cfg.GetDefaultMsg( DEFMSG_NUDGED_OBJECTS ) );
		}
		break;

	case CV_NUKE:		// NUKE all items in the region.
		{
			CWorldSearch AreaItem( ptCtr, iRadius );
			AreaItem.SetAllShow( IsPriv( PRIV_ALLSHOW ));
			AreaItem.SetSearchSquare( true );
			for (;;)
			{
				CItem * pItem = AreaItem.GetItem();
				if ( pItem == NULL )
					break;
				if ( ! rect.IsInside2d( pItem->GetTopPoint()))
					continue;

				if ( m_Targ_Text.IsEmpty())
				{
					pItem->Delete();
				}
				else
				{
					CScript script(m_Targ_Text);
					if ( ! pItem->r_Verb( script, this ))
						continue;
				}
				iCount++;
			}
			SysMessagef( "%d %s", iCount, g_Cfg.GetDefaultMsg( DEFMSG_NUKED_ITEMS ) );
		}
		break;

	case CV_NUKECHAR:
		{
			CWorldSearch AreaChar( ptCtr, iRadius );
			AreaChar.SetAllShow( IsPriv( PRIV_ALLSHOW ));
			AreaChar.SetSearchSquare( true );
			for (;;)
			{
				CChar* pChar = AreaChar.GetChar();
				if ( pChar == NULL )
					break;
				if ( ! rect.IsInside2d( pChar->GetTopPoint()))
					continue;
				if ( pChar->m_pPlayer )
					continue;
				if ( m_Targ_Text.IsEmpty())
				{
					pChar->Delete();
				}
				else
				{
					CScript script(m_Targ_Text);
					if ( ! pChar->r_Verb( script, this ))
						continue;
				}
				iCount++;
			}
			SysMessagef( "%d %s", iCount, g_Cfg.GetDefaultMsg( DEFMSG_NUKED_CHARS ) );
		}
		break;

	case CV_TILE:
		{
			TCHAR szTmp[256];
			strcpylen( szTmp, m_Targ_Text, COUNTOF(szTmp));

			INT64 piArgs[16];		// Maximum parameters in one line
			size_t iArgQty = Str_ParseCmds( szTmp, piArgs, COUNTOF( piArgs ));

			signed char z = static_cast<signed char>(piArgs[0]);	// z height is the first arg.
			size_t iArg = 0;
			for ( int mx = rect.m_left; mx <= rect.m_right; mx++)
			{
				for (int my = rect.m_top; my <= rect.m_bottom; my++)
				{
					if ( ++iArg >= iArgQty )
						iArg = 1;
					CItem * pItem = CItem::CreateTemplate(static_cast<ITEMID_TYPE>(RES_GET_INDEX(piArgs[iArg])), NULL, m_pChar);
					ASSERT(pItem);
					pItem->SetAttr( ATTR_MOVE_NEVER );
					CPointMap ptCur(static_cast<WORD>(mx), static_cast<WORD>(my), z, pt.m_map);
					pItem->MoveToUpdate( ptCur );
					iCount++;
				}
			}

			SysMessagef( "%d %s", iCount, g_Cfg.GetDefaultMsg( DEFMSG_TILED_ITEMS ) );
		}
		break;
	}

	return true;
}

//-----------------------------------------------------------------------
// Targetted Informational skills

int CClient::OnSkill_AnimalLore( CGrayUID uid, int iSkillLevel, bool fTest )
{
	ADDTOCALLSTACK("CClient::OnSkill_AnimalLore");
	UNREFERENCED_PARAMETER(iSkillLevel);
	// SKILL_ANIMALLORE
	// The creature is a "human" etc..
	// How happy.
	// Well trained.
	// Who owns it ?
	// What it eats.

	// Other "lore" type things about a creature ?
	// ex. liche = the remnants of a powerful wizard

	CChar * pChar = uid.CharFind();
	if ( pChar == NULL )
	{
		SysMessageDefault( DEFMSG_NON_ALIVE );
		return( -1 );
	}

	if ( fTest )
	{
		if ( pChar == m_pChar )
			return( 2 );
		if ( m_pChar->IsStatFlag( STATF_OnHorse ) )
		{ 
			CItem * pItem = m_pChar->LayerFind( LAYER_HORSE );
			if ( pItem && pItem->m_itFigurine.m_UID == uid)
				return( 1 );
		}
		if ( pChar->IsPlayableCharacter())
			return( Calc_GetRandVal(10));
		return Calc_GetRandVal(60);
	}

	LPCTSTR pszHe = pChar->GetPronoun();
	LPCTSTR pszHis = pChar->GetPossessPronoun();

	TCHAR *pszTemp = Str_GetTemp();

	// What kind of animal.
	if ( pChar->IsIndividualName())
	{
		sprintf(pszTemp, g_Cfg.GetDefaultMsg( DEFMSG_ANIMALLORE_RESULT ), static_cast<LPCTSTR>(pChar->GetName()), static_cast<LPCTSTR>(pChar->Char_GetDef()->GetTradeName()));
		addObjMessage(pszTemp, pChar);
	}

	// Who is master ?
	CChar * pCharOwner = pChar->NPC_PetGetOwner();
	if ( pCharOwner == NULL )
	{
		sprintf(pszTemp, g_Cfg.GetDefaultMsg( DEFMSG_ANIMALLORE_FREE ), static_cast<LPCTSTR>(pszHe), static_cast<LPCTSTR>(pszHis));
	}
	else
	{
		sprintf(pszTemp, g_Cfg.GetDefaultMsg( DEFMSG_ANIMALLORE_MASTER ), pszHe, ( pCharOwner == m_pChar ) ? g_Cfg.GetDefaultMsg( DEFMSG_ANIMALLORE_MASTER_YOU ) : static_cast<LPCTSTR>(pCharOwner->GetName()));
		// How loyal to master ?
	}
	addObjMessage(pszTemp, pChar );

	// How well fed ?
	// Food count = 30 minute intervals.
	LPCTSTR pszText = pChar->IsStatFlag(STATF_Conjured) ?
						g_Cfg.GetDefaultMsg(DEFMSG_ANIMALLORE_CONJURED) : 
						pChar->Food_GetLevelMessage(pCharOwner ? true : false, true);

	sprintf(pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_ANIMALLORE_FOOD), pszHe, pszText);
	addObjMessage(pszTemp, pChar);

	return 0;
}

int CClient::OnSkill_ItemID( CGrayUID uid, int iSkillLevel, bool fTest )
{
	ADDTOCALLSTACK("CClient::OnSkill_ItemID");
	// SKILL_ITEMID

	CObjBase * pObj = uid.ObjFind();
	if ( pObj == NULL )
	{
		return( -1 );
	}

	if ( pObj->IsChar())
	{
		CChar * pChar = STATIC_CAST <CChar*>(pObj);
		ASSERT(pChar);
		if ( fTest )
		{
			return( 1 );
		}

		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_ITEMID_RESULT ), static_cast<LPCTSTR>(pChar->GetName()));
		return( 1 );
	}

	CItem * pItem = STATIC_CAST <CItem*>(pObj);
	ASSERT( pItem );

	if ( fTest )
	{
		if ( pItem->IsAttr( ATTR_IDENTIFIED ))
		{
			// already identified so easier.
			return Calc_GetRandVal(20);
		}
		return Calc_GetRandVal(60);
	}

	pItem->SetAttr(ATTR_IDENTIFIED);

	// ??? Estimate it's worth ?

	CItemVendable * pItemVend = dynamic_cast <CItemVendable *>(pItem);
	if ( pItemVend == NULL )
	{
		SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_ITEMID_NOVAL ));
	}
	else
	{
		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_ITEMID_GOLD ),
			(pItemVend->GetVendorPrice(-15) * pItem->GetAmount()), static_cast<LPCTSTR>(pItemVend->GetNameFull(true)));
	}

	// Whats it made of ?

	CItemBase * pItemDef = pItem->Item_GetDef();
	ASSERT(pItemDef);

	if ( iSkillLevel > 40 && pItemDef->m_BaseResources.GetCount() > 0)
	{
		TCHAR *pszTemp = Str_GetTemp();
		strcpy(pszTemp, g_Cfg.GetDefaultMsg( DEFMSG_ITEMID_MADEOF ));

		pItemDef->m_BaseResources.WriteNames( pszTemp + strlen(pszTemp) );
		SysMessage( pszTemp );
	}

	// It required what skills to make ?
	// "It requires lots of mining skill"

	return iSkillLevel;
}

int CClient::OnSkill_EvalInt( CGrayUID uid, int iSkillLevel, bool fTest )
{
	ADDTOCALLSTACK("CClient::OnSkill_EvalInt");
	// SKILL_EVALINT
	// iSkillLevel = 0 to 1000
	CChar * pChar = uid.CharFind();
	if ( pChar == NULL )
	{
		SysMessageDefault( DEFMSG_NON_ALIVE );
		return( -1 );
	}

	if ( fTest )
	{
		if ( pChar == m_pChar )
			return( 2 );
		return Calc_GetRandVal(60);
	}

	static LPCTSTR const sm_szIntDesc[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_3 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_4 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_5 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_6 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_7 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_8 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_9 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_10 )
	};

	int iIntVal = pChar->Stat_GetAdjusted(STAT_INT);
	int iIntEntry = (iIntVal-1) / 10;
	if ( iIntEntry < 0 )
		iIntEntry = 0;
	if ( static_cast<unsigned int>(iIntEntry) >= COUNTOF( sm_szIntDesc ))
		iIntEntry = COUNTOF( sm_szIntDesc )-1;

	SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_RESULT ), static_cast<LPCTSTR>(pChar->GetName()), static_cast<LPCTSTR>(sm_szIntDesc[iIntEntry]));

	static LPCTSTR const sm_szMagicDesc[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAG_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAG_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAG_3 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAG_4 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAG_5 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAG_6 )
	};

	static LPCTSTR const sm_szManaDesc[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAN_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAN_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAN_3 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAN_4 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAN_5 ),	// 100 %
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAN_6 )
	};

	if ( iSkillLevel > 400 )	// magery skill and mana level ?
	{
		int iMagerySkill = pChar->Skill_GetAdjusted(SKILL_MAGERY);
		int iNecroSkill = pChar->Skill_GetAdjusted(SKILL_NECROMANCY);
		int iMagicSkill = maximum(iMagerySkill,iNecroSkill);

		int iMagicEntry = iMagicSkill / 200;
		if ( iMagicEntry < 0 )
			iMagicEntry = 0;
		if ( static_cast<unsigned int>(iMagicEntry) >= COUNTOF(sm_szMagicDesc))
			iMagicEntry = COUNTOF(sm_szMagicDesc)-1;

		int iManaEntry = 0;
		if ( iIntVal )
			iManaEntry = IMULDIV( pChar->Stat_GetVal(STAT_INT), COUNTOF(sm_szManaDesc)-1, iIntVal );

		if ( iManaEntry < 0 )
			iManaEntry = 0;
		if ( static_cast<unsigned int>(iManaEntry) >= COUNTOF(sm_szManaDesc))
			iManaEntry = COUNTOF(sm_szManaDesc)-1;

		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_RESULT_2 ), static_cast<LPCTSTR>(sm_szMagicDesc[iMagicEntry]), static_cast<LPCTSTR>(sm_szManaDesc[iManaEntry]));
	}

	return iSkillLevel;
}
static LPCTSTR const sm_szPoisonMessages[] =
{
	g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_PSN_1 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_PSN_2 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_PSN_3 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_PSN_4 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_PSN_5 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_PSN_6 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_PSN_7 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_PSN_8 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_PSN_9 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_PSN_10 )
};

int CClient::OnSkill_ArmsLore( CGrayUID uid, int iSkillLevel, bool fTest )
{
	ADDTOCALLSTACK("CClient::OnSkill_ArmsLore");
	// SKILL_ARMSLORE
	CItem * pItem = uid.ItemFind();
	if ( pItem == NULL || ! pItem->IsTypeArmorWeapon())
	{
		SysMessageDefault( DEFMSG_ARMSLORE_UNABLE );
		return( -SKTRIG_QTY );
	}

	/* todo: why aren't these being used? remove?
	static LPCTSTR const sm_szAttackMessages[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DAM_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DAM_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DAM_3 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DAM_4 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DAM_5 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DAM_6 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DAM_7 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DAM_8 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DAM_9 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DAM_10 )
	};
	
	static LPCTSTR const sm_szDefenseMessages[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DEF_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DEF_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DEF_3 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DEF_4 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DEF_5 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DEF_6 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DEF_7 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DEF_8 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DEF_9 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DEF_10 )
	};
	*/

	if ( fTest )
	{
		return Calc_GetRandVal(60);
	}

	TCHAR *pszTemp = Str_GetTemp();
	size_t len = 0;
	bool fWeapon;
	int iHitsCur;
	int iHitsMax;

	switch ( pItem->GetType() )
	{
		case IT_ARMOR:				// some type of armor. (no real action)
		case IT_SHIELD:
		case IT_ARMOR_LEATHER:
		case IT_CLOTHING:
		case IT_JEWELRY:
			fWeapon = false;
			iHitsCur = pItem->m_itArmor.m_Hits_Cur;
			iHitsMax = pItem->m_itArmor.m_Hits_Max;
			len += sprintf( pszTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DEF ), pItem->Armor_GetDefense());
			break;
		case IT_WEAPON_MACE_CROOK:
		case IT_WEAPON_MACE_PICK:
		case IT_WEAPON_MACE_SMITH:	// Can be used for smithing ?
		case IT_WEAPON_MACE_STAFF:
		case IT_WEAPON_MACE_SHARP:	// war axe can be used to cut/chop trees.
		case IT_WEAPON_SWORD:
		case IT_WEAPON_AXE:
		case IT_WEAPON_FENCE:
		case IT_WEAPON_BOW:
		case IT_WEAPON_XBOW:
		case IT_WEAPON_THROWING:
			fWeapon = true;
			iHitsCur = pItem->m_itWeapon.m_Hits_Cur;
			iHitsMax = pItem->m_itWeapon.m_Hits_Max;
			len += sprintf( pszTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DAM ), pItem->Weapon_GetAttack());
			break;
		default:
			SysMessageDefault( DEFMSG_ARMSLORE_UNABLE );
			return( -SKTRIG_QTY );
	}

	len += sprintf( pszTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_REP ), pItem->Armor_GetRepairDesc());

	if ( iHitsCur <= 3 || iHitsMax <= 3 )
	{
		len += strcpylen( pszTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_REP_0 ) );
	}

	// Magical effects ?
	if ( pItem->IsAttr(ATTR_MAGIC))
	{
		len += strcpylen( pszTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ITEM_MAGIC ) );
	}
	else if ( pItem->IsAttr(ATTR_NEWBIE|ATTR_MOVE_NEVER))
	{
		len += strcpylen( pszTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ITEM_NEWBIE ) );
	}

	// Repairable ?
	if ( ! pItem->Armor_IsRepairable())
	{
		len += strcpylen( pszTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ITEM_REPAIR ) );
	}

	// Poisoned ?
	if ( fWeapon && pItem->m_itWeapon.m_poison_skill )
	{
		unsigned int iLevel = IMULDIV( pItem->m_itWeapon.m_poison_skill, COUNTOF(sm_szPoisonMessages), 100 );
		if ( iLevel >= COUNTOF(sm_szPoisonMessages))
			iLevel = COUNTOF(sm_szPoisonMessages) - 1;
		len += sprintf( pszTemp+len, " %s", sm_szPoisonMessages[iLevel] );
	}

	SysMessage(pszTemp);
	return iSkillLevel;
}

int CClient::OnSkill_Anatomy( CGrayUID uid, int iSkillLevel, bool fTest )
{
	ADDTOCALLSTACK("CClient::OnSkill_Anatomy");
	// SKILL_ANATOMY
	CChar * pChar = uid.CharFind();
	if ( pChar == NULL )
	{
		addObjMessage( g_Cfg.GetDefaultMsg( DEFMSG_NON_ALIVE ), pChar );
		return( -1 );
	}

	if ( fTest )
	{
		// based on rareity ?
		if ( pChar == m_pChar )
			return( 2 );
		return Calc_GetRandVal(60);
	}

	// Add in error cased on your skill level.

	static LPCTSTR const sm_szStrEval[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_3 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_4 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_5 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_6 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_7 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_8 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_9 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_10 )
	};
	static LPCTSTR const sm_szDexEval[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_3 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_4 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_5 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_6 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_7 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_8 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_9 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_10 )
	};

	int iStrVal = pChar->Stat_GetAdjusted(STAT_STR);
	int iStrEntry = (iStrVal-1)/10;
	if ( iStrEntry < 0 )
		iStrEntry = 0;
	if ( static_cast<unsigned int>(iStrEntry) >= COUNTOF( sm_szStrEval ))
		iStrEntry = COUNTOF( sm_szStrEval )-1;

	int iDexVal = pChar->Stat_GetAdjusted(STAT_DEX);
	int iDexEntry = (iDexVal-1)/10;
	if ( iDexEntry < 0 )
		iDexEntry = 0;
	if ( static_cast<unsigned int>(iDexEntry) >= COUNTOF( sm_szDexEval ))
		iDexEntry = COUNTOF( sm_szDexEval )-1;

	TCHAR * pszTemp = Str_GetTemp();
	sprintf(pszTemp, g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_RESULT ), static_cast<LPCTSTR>(pChar->GetName()),
		sm_szStrEval[iStrEntry], sm_szDexEval[iDexEntry]);
	addObjMessage(pszTemp, pChar);

	if ( pChar->IsStatFlag(STATF_Conjured) )
	{
		addObjMessage(g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_MAGIC), pChar);
	}

	// ??? looks hungry ?
	return iSkillLevel;
}

int CClient::OnSkill_Forensics( CGrayUID uid, int iSkillLevel, bool fTest )
{
	ADDTOCALLSTACK("CClient::OnSkill_Forensics");
	// SKILL_FORENSICS
	// ! weird client issue targetting corpses !

	CItemCorpse * pCorpse = dynamic_cast<CItemCorpse *>(uid.ItemFind());
	if ( !pCorpse )
	{
		SysMessageDefault( DEFMSG_FORENSICS_CORPSE );
		return -SKTRIG_QTY;
	}
	if ( !m_pChar->CanTouch(pCorpse) )
	{
		SysMessageDefault( DEFMSG_FORENSICS_REACH );
		return -SKTRIG_QTY;
	}

	if (fTest)
		return (pCorpse->m_uidLink == m_pChar->GetUID()) ? 2 : Calc_GetRandVal(60);

	CChar * pCharKiller = pCorpse->m_itCorpse.m_uidKiller.CharFind();
	LPCTSTR pName = pCharKiller ? pCharKiller->GetName() : NULL;

	if ( pCorpse->IsCorpseSleeping() )
	{
		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_FORENSICS_ALIVE ), pName ? pName : "It" );
		return 1;
	}

	TCHAR * pszTemp = Str_GetTemp();
	if ( pCorpse->m_itCorpse.m_carved )
	{
		int len = sprintf( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_FORENSICS_CARVE_1), pCorpse->GetName() );
		if ( pName )
			sprintf( pszTemp + len, g_Cfg.GetDefaultMsg(DEFMSG_FORENSICS_CARVE_2), pName );
		else
			strcpy( pszTemp + len, g_Cfg.GetDefaultMsg(DEFMSG_FORENSICS_FAILNAME) );

	}
	else if ( pCorpse->GetTimeStamp().IsTimeValid() )
	{
		int len = sprintf( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_FORENSICS_TIMER), pCorpse->GetName(), -g_World.GetTimeDiff(pCorpse->GetTimeStamp()) / TICK_PER_SEC );
		if ( pName )
			sprintf( pszTemp + len, g_Cfg.GetDefaultMsg(DEFMSG_FORENSICS_NAME), pName );
		else
			strcpy( pszTemp + len, g_Cfg.GetDefaultMsg(DEFMSG_FORENSICS_FAILNAME) );
	}
	SysMessage( pszTemp );
	return iSkillLevel;
}

int CClient::OnSkill_TasteID( CGrayUID uid, int iSkillLevel, bool fTest )
{
	ADDTOCALLSTACK("CClient::OnSkill_TasteID");
	// SKILL_TASTEID
	// Try to taste for poisons I assume.
	// Maybe taste what it is made of for ingredients ?
	// Differntiate potion types ?

	CItem * pItem = uid.ItemFind();
	if ( pItem == NULL )
	{
		if ( uid == m_pChar->GetUID())
		{
			SysMessageDefault( DEFMSG_TASTEID_SELF );
		}
		else
		{
			SysMessageDefault( DEFMSG_TASTEID_CHAR );
		}
		return( -SKTRIG_QTY );
	}

	if ( ! m_pChar->CanUse( pItem, true ))
	{
		SysMessageDefault( DEFMSG_TASTEID_UNABLE );
		return( -SKTRIG_QTY );
	}

	unsigned int iPoisonLevel = 0;
	switch ( pItem->GetType())
	{
		case IT_POTION:
			if ( RES_GET_INDEX(pItem->m_itPotion.m_Type) == SPELL_Poison )
			{
				iPoisonLevel = pItem->m_itPotion.m_skillquality;
			}
			break;
		case IT_FRUIT:
		case IT_FOOD:
		case IT_FOOD_RAW:
		case IT_MEAT_RAW:
			iPoisonLevel = pItem->m_itFood.m_poison_skill * 10;
			break;
		case IT_WEAPON_MACE_SHARP:
		case IT_WEAPON_SWORD:		// 13 =
		case IT_WEAPON_FENCE:		// 14 = can't be used to chop trees. (make kindling)
		case IT_WEAPON_AXE:
			// pItem->m_itWeapon.m_poison_skill = pPoison->m_itPotion.m_skillquality / 10;
			iPoisonLevel = pItem->m_itWeapon.m_poison_skill * 10;
			break;
		default:
			if ( ! fTest )
			{
				SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_TASTEID_RESULT ), static_cast<LPCTSTR>(pItem->GetNameFull(false)));
			}
			return 1;
	}

	if ( fTest )
	{
		return Calc_GetRandVal(60);
	}

	if ( iPoisonLevel )
	{
		unsigned int iLevel = IMULDIV( iPoisonLevel, COUNTOF(sm_szPoisonMessages), 1000 );
		if ( iLevel >= COUNTOF(sm_szPoisonMessages))
			iLevel = COUNTOF(sm_szPoisonMessages) - 1;
		SysMessage( sm_szPoisonMessages[iLevel] );
	}
	else
	{
		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_TASTEID_RESULT ), static_cast<LPCTSTR>(pItem->GetNameFull(false)));
	}

	return iSkillLevel;
}

int CClient::OnSkill_Info( SKILL_TYPE skill, CGrayUID uid, int iSkillLevel, bool fTest )
{
	ADDTOCALLSTACK("CClient::OnSkill_Info");
	// Skill timer has expired.
	// RETURN: difficulty credit. 0-100
	//  <0 = immediate failure.
	switch ( skill )
	{
		case SKILL_ANIMALLORE:	return OnSkill_AnimalLore( uid, iSkillLevel, fTest );
		case SKILL_ARMSLORE:	return OnSkill_ArmsLore( uid, iSkillLevel, fTest );
		case SKILL_ANATOMY:		return OnSkill_Anatomy( uid, iSkillLevel, fTest );
		case SKILL_ITEMID:		return OnSkill_ItemID( uid, iSkillLevel, fTest );
		case SKILL_EVALINT:		return OnSkill_EvalInt( uid, iSkillLevel, fTest );
		case SKILL_FORENSICS:	return OnSkill_Forensics( uid, iSkillLevel, fTest );
		case SKILL_TASTEID:		return OnSkill_TasteID( uid, iSkillLevel, fTest );
		default:				return -SKTRIG_QTY;
	}
}

////////////////////////////////////////
// Targeted skills and actions.

bool CClient::OnTarg_Skill( CObjBase * pObj )
{
	ADDTOCALLSTACK("CClient::OnTarg_Skill");
	// targetted skill now has it's target.
	// response to CLIMODE_TARG_SKILL
	// from Event_Skill_Use() select button from skill window

	if ( pObj == NULL )
		return( false );

	SetTargMode();	// just make sure last targ mode is gone.
	m_Targ_UID = pObj->GetUID();	// keep for 'last target' info.

	// did we target a scripted skill ?
	if ( g_Cfg.IsSkillFlag( m_tmSkillTarg.m_Skill, SKF_SCRIPTED ) )
	{
		// is this scripted skill a targeted skill ?
		const CSkillDef * pSkillDef = g_Cfg.GetSkillDef(m_tmSkillTarg.m_Skill);
		if (pSkillDef != NULL && pSkillDef->m_sTargetPrompt.IsEmpty() == false)
		{
			m_pChar->m_Act_Targ = m_Targ_UID;
			return m_pChar->Skill_Start( m_tmSkillTarg.m_Skill );
		}
		else
		{
			return true;
		}
	}

	// targetting what skill ?
	switch ( m_tmSkillTarg.m_Skill )
	{
		// Delayed response type skills.
	case SKILL_BEGGING:
	case SKILL_STEALING:
	case SKILL_TAMING:
	case SKILL_ENTICEMENT:
	case SKILL_STEALTH:
	case SKILL_REMOVETRAP:

		// Informational skills. (instant return)
	case SKILL_ANIMALLORE:
	case SKILL_ARMSLORE:
	case SKILL_ANATOMY:
	case SKILL_ITEMID:
	case SKILL_EVALINT:
	case SKILL_FORENSICS:
	case SKILL_TASTEID:
		m_pChar->m_Act_Targ = m_Targ_UID;
		return( m_pChar->Skill_Start( m_tmSkillTarg.m_Skill ));

	case SKILL_PROVOCATION:
		if ( !pObj->IsChar() )
		{
			SysMessageDefault(DEFMSG_PROVOKE_UNABLE);
			return false;
		}
		addTarget(CLIMODE_TARG_SKILL_PROVOKE, g_Cfg.GetDefaultMsg(DEFMSG_PROVOKE_SELECT), false, true);
		break;

	case SKILL_POISONING:
		// We now have to find the poison.
		addTarget( CLIMODE_TARG_SKILL_POISON, g_Cfg.GetDefaultMsg( DEFMSG_POISONING_SELECT_1 ), false, true );
		break;

	default:
		// This is not a targetted skill !
		break;
	}

	return true;
}

bool CClient::OnTarg_Skill_Provoke( CObjBase * pObj )
{
	ADDTOCALLSTACK("CClient::OnTarg_Skill_Provoke");
	// CLIMODE_TARG_SKILL_PROVOKE
	if ( !pObj || !pObj->IsChar() )
	{
		SysMessageDefault(DEFMSG_PROVOKE_UNABLE);
		return false;
	}
	m_pChar->m_Act_TargPrv = m_Targ_UID;	// provoke him
	m_pChar->m_Act_Targ = pObj->GetUID();	// against him
	return m_pChar->Skill_Start(SKILL_PROVOCATION);
}

bool CClient::OnTarg_Skill_Poison( CObjBase * pObj )
{
	ADDTOCALLSTACK("CClient::OnTarg_Skill_Poison");
	// CLIMODE_TARG_SKILL_POISON
	if ( pObj == NULL )
		return( false );
	m_pChar->m_Act_TargPrv = m_Targ_UID;	// poison this
	m_pChar->m_Act_Targ = pObj->GetUID();				// with this poison
	return( m_pChar->Skill_Start( SKILL_POISONING ));
}

bool CClient::OnTarg_Skill_Herd_Dest( CObjBase * pObj, const CPointMap & pt )
{
	ADDTOCALLSTACK("CClient::OnTarg_Skill_Herd_Dest");
	UNREFERENCED_PARAMETER(pObj);
	// CLIMODE_TARG_SKILL_HERD_DEST

	m_pChar->m_Act_p = pt;
	m_pChar->m_Act_Targ = m_Targ_UID;	// Who to herd?
	m_pChar->m_Act_TargPrv = m_Targ_PrvUID; // crook ?

	return( m_pChar->Skill_Start( SKILL_HERDING ));
}

bool CClient::OnTarg_Skill_Magery( CObjBase * pObj, const CPointMap & pt )
{
	ADDTOCALLSTACK("CClient::OnTarg_Skill_Magery");
	// The client player has targeted a spell.
	// CLIMODE_TARG_SKILL_MAGERY

	const CSpellDef *		pSpell	= g_Cfg.GetSpellDef( m_tmSkillMagery.m_Spell );

	if ( ! pSpell )		return false;

	if ( pObj )
	{
		if ( !pSpell->IsSpellType( SPELLFLAG_TARG_OBJ ) )
		{
			SysMessageDefault( DEFMSG_MAGERY_4 );
			return true;
		}

		if ( pObj->IsItem() && !pSpell->IsSpellType( SPELLFLAG_TARG_ITEM ) )
		{
			SysMessageDefault( DEFMSG_MAGERY_1 );
			return true;
		}

		if ( pObj->IsChar() )
		{
			if (!pSpell->IsSpellType( SPELLFLAG_TARG_CHAR ) )
			{
				SysMessageDefault( DEFMSG_MAGERY_2 );
				return true;
			}
			CChar * pChar = dynamic_cast<CChar*>(pObj);
			if ( pSpell->IsSpellType( SPELLFLAG_TARG_NO_PLAYER ) && pChar->m_pPlayer )
			{
				SysMessageDefault( DEFMSG_MAGERY_7 );
				return true;
			}
			if ( pSpell->IsSpellType( SPELLFLAG_TARG_NO_NPC ) && !pChar->m_pPlayer )
			{
				SysMessageDefault( DEFMSG_MAGERY_8 );
				return true;
			}
		}

		if (pObj == m_pChar && pSpell->IsSpellType( SPELLFLAG_TARG_NOSELF ) && !IsPriv(PRIV_GM) )
		{
			SysMessageDefault( DEFMSG_MAGERY_3 );
			return true;
		}
	}

	if ( m_tmSkillMagery.m_Spell == SPELL_Polymorph )
	{
		if ( IsTrigUsed(TRIGGER_SKILLMENU) )
		{
			CScriptTriggerArgs args("sm_polymorph");
			if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
				return true;
		}
		return Cmd_Skill_Menu( g_Cfg.ResourceGetIDType( RES_SKILLMENU, "sm_polymorph" ));
	}

	m_pChar->m_atMagery.m_Spell			= m_tmSkillMagery.m_Spell;
	m_pChar->m_atMagery.m_SummonID		= m_tmSkillMagery.m_SummonID;
	m_pChar->m_atMagery.m_fSummonPet	= m_tmSkillMagery.m_fSummonPet;

	m_pChar->m_Act_TargPrv				= m_Targ_PrvUID;	// Source (wand or you?)
	m_pChar->m_Act_Targ					= pObj ? (DWORD) pObj->GetUID() : UID_CLEAR ;
	m_pChar->m_Act_p					= pt;
	m_Targ_p							= pt;

	if ( IsSetMagicFlags( MAGICF_PRECAST ) && !pSpell->IsSpellType( SPELLFLAG_NOPRECAST ) && m_pChar->IsClient() )
	{
		if ( g_Cfg.IsSkillFlag(m_pChar->m_Act_SkillCurrent, SKF_MAGIC) )
		{
			this->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_MAGERY_5 ) ); // You have not yet finished preparing the spell.
			return false;
		}

		return( m_pChar->Spell_CastDone() );
	}

	int skill;
	if (!pSpell->GetPrimarySkill(&skill, NULL))
		return false;

	return( m_pChar->Skill_Start(static_cast<SKILL_TYPE>(skill)));
}

bool CClient::OnTarg_Pet_Command( CObjBase * pObj, const CPointMap & pt )
{
	ADDTOCALLSTACK("CClient::OnTarg_Pet_Command");
	// CLIMODE_TARG_PET_CMD
	// Any pet command requiring a target.
	// m_Targ_UID = the pet i was instructing.
	// m_tmPetCmd = command index.
	// m_Targ_Text = the args to the command that got us here.

	if ( m_tmPetCmd.m_fAllPets )
	{
		// All the pets that could hear me.
		bool fGhostSpeak = m_pChar->IsSpeakAsGhost();

		CWorldSearch AreaChars( m_pChar->GetTopPoint(), UO_MAP_VIEW_SIGHT );
		for (;;)
		{
			CChar * pCharPet = AreaChars.GetChar();
			if ( pCharPet == NULL )
				break;
			if ( pCharPet == m_pChar )
				continue;
			if ( fGhostSpeak && ! pCharPet->CanUnderstandGhost())
				continue;
			if ( ! pCharPet->NPC_IsOwnedBy( GetChar(), true ))
				continue;
			pCharPet->NPC_OnHearPetCmdTarg( m_tmPetCmd.m_iCmd, GetChar(), pObj, pt, m_Targ_Text );
		}
		return( true );
	}
	else
	{
		CChar * pCharPet = m_Targ_UID.CharFind();
		if ( pCharPet == NULL )
			return false;
		return pCharPet->NPC_OnHearPetCmdTarg( m_tmPetCmd.m_iCmd, GetChar(), pObj, pt, m_Targ_Text );
	}
}

bool CClient::OnTarg_Pet_Stable( CChar * pCharPet )
{
	ADDTOCALLSTACK("CClient::OnTarg_Pet_Stable");
	// CLIMODE_PET_STABLE
	// NOTE: You are only allowed to stable x creatures here.
	// m_Targ_PrvUID = stable master.

	CChar * pCharMaster = m_Targ_PrvUID.CharFind();
	if ( pCharMaster == NULL )
		return( false );

	if ( pCharPet == NULL || pCharPet->m_pPlayer || pCharMaster == pCharPet )
	{
		pCharMaster->Speak( g_Cfg.GetDefaultMsg( DEFMSG_STABLEMASTER_TARG_FAIL ) );
		return( false );
	}

	if ( ! pCharMaster->CanSeeLOS( pCharPet ))
	{
		pCharMaster->Speak( g_Cfg.GetDefaultMsg( DEFMSG_STABLEMASTER_TARG_LOS ) );
		return( false );
	}

	if ( ! pCharPet->NPC_IsOwnedBy( m_pChar ))
	{
		pCharMaster->Speak( g_Cfg.GetDefaultMsg( DEFMSG_STABLEMASTER_TARG_OWNER ) );
		return( false );
	}

	if ( pCharPet->IsStatFlag(STATF_Conjured))
	{
		pCharMaster->Speak( g_Cfg.GetDefaultMsg( DEFMSG_STABLEMASTER_TARG_SUMMON ) );
		return( false );
	}

	CItemContainer * pPack = pCharPet->GetPack();
	if ( pPack )
	{
		if ( ! pPack->IsEmpty() )
		{
			pCharMaster->Speak( g_Cfg.GetDefaultMsg( DEFMSG_STABLEMASTER_TARG_UNLOAD ) );
			return( false );
		}
	}

	// Shrink the pet and put it in the bank box of the stable master.
	CItem * pPetItem = pCharPet->Make_Figurine( m_pChar->GetUID());
	if ( pPetItem == NULL )
	{
		pCharMaster->Speak( g_Cfg.GetDefaultMsg( DEFMSG_STABLEMASTER_TARG_FAIL ) );
		return( false );
	}

	if ( IsSetOF(OF_PetSlots) )
	{
		short int iFollowerSlotsNeeded = static_cast<short>(maximum(pCharPet->GetDefNum("FOLLOWERSLOTS", true, true),1));
		short int iCurFollower = static_cast<short>(m_pChar->GetDefNum("CURFOLLOWER", true, true));
		short int iSetFollower = iCurFollower - iFollowerSlotsNeeded;
		if ( iSetFollower < 0 )
			iSetFollower = 0;
		m_pChar->FollowersUpdate(pCharPet, true, &iFollowerSlotsNeeded);
		// Send an update packet for the stats
		m_pChar->SetDefNum("CURFOLLOWER", iSetFollower);
		CClient * pClient = m_pChar->GetClient();
		if ( pClient )
			pClient->addCharStatWindow( m_pChar->GetUID() );
	}

	pCharMaster->GetBank()->ContentAdd( pPetItem );
	pCharMaster->Speak( g_Cfg.GetDefaultMsg( DEFMSG_STABLEMASTER_CLAIM ) );
	return true;
}

//-----------------------------------------------------------------------
// Targetted items with special props.

bool CClient::OnTarg_Use_Deed( CItem * pDeed, const CPointMap & pt )
{
	ADDTOCALLSTACK("CClient::OnTarg_Use_Deed");
	// Place the house/ship here. (if possible)
	// Can the structure go here ?
	// IT_DEED
	//

	if ( ! m_pChar->CanUse(pDeed, true ))
		return( false );

	const CItemBase * pItemDef = CItemBase::FindItemBase(static_cast<ITEMID_TYPE>(RES_GET_INDEX(pDeed->m_itDeed.m_Type)));

	if ( ! OnTarg_Use_Multi( pItemDef, pt, pDeed->m_Attr, pDeed->GetHue() ))
		return( false );

	pDeed->Delete();	// consume the deed.
	return true;
}

CItem * CClient::OnTarg_Use_Multi( const CItemBase * pItemDef, const CPointMap & pt, DWORD dwAttr, HUE_TYPE wHue )
{
	ADDTOCALLSTACK("CClient::OnTarg_Use_Multi");
	// Might be a IT_MULTI or it might not. place it anyhow.

	if ( pItemDef == NULL )
		return( NULL );

	bool fShip = ( pItemDef->IsType(IT_SHIP));	// must be in water.

	const CItemBaseMulti * pMultiDef = dynamic_cast <const CItemBaseMulti *> ( pItemDef );

	// Check water/mountains/etc.
	if ( pMultiDef != NULL && ! (dwAttr&ATTR_MAGIC))
	{
		// Check for items in the way and bumpy terrain.

		CGRect rect = pMultiDef->m_rect;
		rect.m_map = pt.m_map;
		rect.OffsetRect( pt.m_x, pt.m_y );
		CPointMap ptn = pt;

		int x = rect.m_left;
		for ( ; x <rect.m_right; ++x )
		{
			ptn.m_x = static_cast<short>(x);
			int y = rect.m_top;
			for ( ; y < rect.m_bottom; ++y )
			{
				ptn.m_y = static_cast<short>(y);

				if ( !ptn.IsValidPoint() )
				{
					SysMessageDefault( DEFMSG_ITEMUSE_MULTI_FAIL );
					return( NULL );
				}

				CRegionBase * pRegion = ptn.GetRegion( REGION_TYPE_MULTI | REGION_TYPE_AREA | REGION_TYPE_ROOM );
				if ( pRegion == NULL || ( pRegion->IsFlag(REGION_FLAG_NOBUILDING|REGION_FLAG_UNDERGROUND|REGION_FLAG_GUARDED|REGION_FLAG_SAFE) && ! fShip ))
				{
					SysMessageDefault( DEFMSG_ITEMUSE_MULTI_FAIL );
					if ( ! IsPriv( PRIV_GM )) return( NULL );
				}

				DWORD wBlockFlags = ( fShip ) ? CAN_C_SWIM : CAN_C_WALK;
				ptn.m_z = g_World.GetHeightPoint2( ptn, wBlockFlags, true ); //hm...should really use the 2nd function ? it does fixed #2373
				if ( abs( ptn.m_z - pt.m_z ) > 4 )
				{
					SysMessageDefault( DEFMSG_ITEMUSE_MULTI_BUMP );
					if ( ! IsPriv( PRIV_GM )) return( NULL );
				}
				if ( fShip )
				{
					if ( ! ( wBlockFlags & CAN_I_WATER ))
					{
						SysMessageDefault( DEFMSG_ITEMUSE_MULTI_SHIPW );
						if ( ! IsPriv( PRIV_GM )) return( NULL );
					}
				}
				else if ( wBlockFlags & ( CAN_I_WATER | CAN_I_BLOCK | CAN_I_CLIMB ))
				{
					SysMessageDefault( DEFMSG_ITEMUSE_MULTI_BLOCKED );
					if ( ! IsPriv( PRIV_GM )) return( NULL );
				}
			}
		}

		// Check for chars in the way.

		CWorldSearch Area( pt, UO_MAP_VIEW_SIZE );
		Area.SetSearchSquare( true );
		for (;;)
		{
			CChar * pChar = Area.GetChar();
			if ( pChar == NULL )
				break;
			if ( pChar == m_pChar )
				continue;
			if ( ! rect.IsInside2d( pChar->GetTopPoint()))
				continue;
			if ( pChar->GetPrivLevel() >= PLEVEL_Counsel && !CanSee( pChar ) )
			{
				// Don't reveal the presence of hidden staff
				pChar->Spell_Teleport(m_pChar->GetTopPoint(), false, false);
				continue;
			}

			SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_MULTI_INTWAY ), static_cast<LPCTSTR>(pChar->GetName()));
			if ( IsPriv(PRIV_GM) )
			{
				//	Teleport the char to self. At least I will be able to move him to someplace
				pChar->Spell_Teleport(m_pChar->GetTopPoint(), false, false);
			}
			else return( NULL );
		}
	}

	CItem * pItemNew = CItem::CreateTemplate( pItemDef->GetID(), NULL, m_pChar );
	if ( pItemNew == NULL )
	{
		SysMessageDefault( DEFMSG_ITEMUSE_MULTI_COLLAPSE );
		return( NULL );
	}

	pItemNew->SetAttr( dwAttr & ( ATTR_MAGIC | ATTR_INVIS ));
	pItemNew->SetHue( wHue );
	pItemNew->MoveToUpdate( pt );

	CItemMulti * pMultiItem = dynamic_cast <CItemMulti*>(pItemNew);
	if ( pMultiItem )
	{
		pMultiItem->Multi_Create( m_pChar, UID_CLEAR );
	}

	if ( pItemDef->IsType(IT_STONE_GUILD))
	{
		// Now name the guild
		CItemStone * pStone = dynamic_cast <CItemStone*>(pItemNew);
		pStone->AddRecruit( m_pChar, STONEPRIV_MASTER );
		addPromptConsole( CLIMODE_PROMPT_STONE_NAME, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_GUILDSTONE_NEW ), pItemNew->GetUID() );
	}
	else if ( fShip )
	{
		pItemNew->Sound( Calc_GetRandVal(2)? 0x12:0x13 );
	}

	return pItemNew;
}

bool CClient::OnTarg_Use_Item( CObjBase * pObjTarg, CPointMap & pt, ITEMID_TYPE id )
{
	ADDTOCALLSTACK("CClient::OnTarg_Use_Item");
	// CLIMODE_TARG_USE_ITEM started from Event_DoubleClick()
	// Targetted an item to be used on some other object (char or item).
	// Solve for the intersection of the items.
	// ARGS:
	//  id = static item id
	//  uid = the target.
	//  m_Targ_UID = what is the used object on the target
	//
	// NOTE:
  	//  Assume we can see the target.
	//
	// RETURN:
	//  true = success.

	CItem * pItemUse = m_Targ_UID.ItemFind();
	if ( pItemUse == NULL )
	{
		SysMessage( g_Cfg.GetDefaultMsg(DEFMSG_TARG_GONE) );
		return false;
	}
	if ( pItemUse->GetParent() != m_tmUseItem.m_pParent )
	{
		// Watch out for cheating.
		// Is the source item still in the same place as it was.
		SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_TARG_MOVED));
		return false;
	}

	m_Targ_PrvUID = m_Targ_UID;	// used item.
	m_Targ_p = pt;

	ITRIG_TYPE trigtype;
	if ( pObjTarg == NULL )
	{
		m_Targ_UID.ClearUID();
		if ( pt.IsCharValid())
		{
			if ( !IsSetOF(OF_DClickNoTurn) )
				m_pChar->UpdateDir(pt);
		}
		trigtype = ITRIG_TARGON_GROUND;
	}
	else
	{
		m_Targ_UID = pObjTarg->GetUID();
		if ( !IsSetOF(OF_DClickNoTurn) )
			m_pChar->UpdateDir(pObjTarg);

		if ( pObjTarg->IsChar() )
		{
			trigtype = ITRIG_TARGON_CHAR;
		}
		else
		{
			trigtype = ITRIG_TARGON_ITEM;
		}
	}

	if (( IsTrigUsed(CItem::sm_szTrigName[trigtype]) ) || ( IsTrigUsed(CChar::sm_szTrigName[(CTRIG_itemAfterClick - 1) + trigtype]) )) //ITRIG_TARGON_GROUND, ITRIG_TARGON_CHAR, ITRIG_TARGON_ITEM
	{
		CScriptTriggerArgs	Args( id, 0, pObjTarg );
		if ( pItemUse->OnTrigger( trigtype, m_pChar, &Args ) == TRIGRET_RET_TRUE )
			return true;
	}

	// NOTE: We have NOT checked to see if the targetted object is within reasonable distance.
	// Call CanUse( pItemTarg )

	// What did i target it on ? this could be null if ground is the target.
	CChar * pCharTarg = dynamic_cast <CChar*>(pObjTarg);
	CItem * pItemTarg = dynamic_cast <CItem*>(pObjTarg);

	switch ( pItemUse->GetType() )
	{
	case IT_COMM_CRYSTAL:
		if ( pItemTarg == NULL )
			return( false );
		if ( ! pItemTarg->IsType(IT_COMM_CRYSTAL))
			return( false );
		pItemUse->m_uidLink = pItemTarg->GetUID();
		pItemUse->Speak( "Linked" );
		return( true );

	case IT_POTION:
		// Use a potion on something else.
		if ( RES_GET_INDEX(pItemUse->m_itPotion.m_Type) == SPELL_Poison )
		{
			// ??? If the item is a poison ask them what they want to use the poison on ?
			// Poisoning or poison self ?
		}
		else if ( RES_GET_INDEX(pItemUse->m_itPotion.m_Type) == SPELL_Explosion )
		{
			// Throw explode potion.
			if ( ! pItemUse->IsItemEquipped() || pItemUse->GetEquipLayer() != LAYER_DRAGGING )
				return( false );

			// Put at destination location.
			CPointMap ptBlock;
			if ( m_pChar->CanSeeLOS( pt, &ptBlock, UO_MAP_VIEW_SIZE, LOS_NB_WINDOWS ))	// Get the block point; flag means that we can throw an explosion potion outta a window
				ptBlock = pt;
			pItemUse->MoveToUpdate( ptBlock );	// leave the decay as it is.
			pItemUse->Effect( EFFECT_BOLT, pItemUse->GetDispID(), m_pChar, 7, 0, false );
		}
		return( true );

	case IT_KEY:
		return( m_pChar->Use_Key( pItemUse, pItemTarg ));

	case IT_FORGE:
		// target the ore.
		return( m_pChar->Skill_Mining_Smelt( pItemTarg, pItemUse ));

	case IT_CANNON_MUZZLE:
		// We have targetted the cannon to something.
		if ( ( pItemUse->m_itCannon.m_Load & 3 ) != 3 )
		{
			if ( m_pChar->Use_Cannon_Feed( pItemUse, pItemTarg ))
			{
				pItemTarg->ConsumeAmount();
			}
			return( true );
		}

		// Fire!
		pItemUse->m_itCannon.m_Load = 0;
		pItemUse->Sound( 0x207 );
		pItemUse->Effect( EFFECT_OBJ, ITEMID_FX_TELE_VANISH, pItemUse, 9, 6 );

		// just explode on the ground ?
		if ( pObjTarg != NULL )
		{
			// Check distance and LOS.
			if ( pItemUse->GetDist( pObjTarg ) > UO_MAP_VIEW_SIGHT )
			{
				SysMessageDefault( DEFMSG_ITEMUSE_TOOFAR );
				return( true );
			}

			// Hit the Target !
			pObjTarg->Sound( 0x207 );
			pObjTarg->Effect( EFFECT_BOLT, ITEMID_Cannon_Ball, pItemUse, 8, 0, true );

			CChar *pChar = dynamic_cast<CChar *>(this);
			CItem *pItem = dynamic_cast<CItem *>(this);
			if ( pChar )
				pChar->OnTakeDamage( 80 + Calc_GetRandVal(150), m_pChar, DAMAGE_HIT_BLUNT|DAMAGE_FIRE );
			else if ( pItem )
				pItem->OnTakeDamage( 80 + Calc_GetRandVal(150), m_pChar, DAMAGE_HIT_BLUNT|DAMAGE_FIRE );
		}
		return( true );

	case IT_WEAPON_MACE_PICK:
		// Mine at the location. (shovel)
		m_pChar->m_Act_p = pt;
		m_pChar->m_Act_TargPrv = m_Targ_PrvUID;
		return( m_pChar->Skill_Start( SKILL_MINING ));

	case IT_WEAPON_MACE_CROOK:
		// SKILL_HERDING
		// Selected a creature to herd
		// Move the selected item or char to this location.
		if ( pCharTarg == NULL )
		{
			// hit it ?
			SysMessageDefault( DEFMSG_ITEMUSE_CROOK_TRY );
			return( false );
		}
		addTarget(CLIMODE_TARG_SKILL_HERD_DEST, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_CROOK_TARGET), true);
		return( true );

	case IT_WEAPON_MACE_SMITH:		// Can be used for smithing.
		// Near a forge ? smithing ?
		if ( pItemTarg == NULL )
			break;
		if ( pItemTarg->IsType(IT_INGOT) )
		{
			return Cmd_Skill_Smith( pItemTarg );
		}
		else if ( pItemTarg->Armor_IsRepairable())
		{
			// Near an anvil ? repair ?
			m_pChar->Use_Repair( pItemTarg );
			return( true );
		}
		break;

	case IT_CARPENTRY_CHOP:		// Carpentry type tool
	case IT_WEAPON_MACE_SHARP:// 22 = war axe can be used to cut/chop trees.
	case IT_WEAPON_FENCE:		// 24 = can't be used to chop trees.
	case IT_WEAPON_AXE:
	case IT_WEAPON_SWORD:		// 23 =

		// Use sharp weapon on something.
		if ( pCharTarg != NULL )
		{
			// on some person ?
			if ( ! m_pChar->CanTouch(pCharTarg) )
				return( false );
			switch ( pCharTarg->GetID())
			{
				case CREID_Sheep: // Sheep have wool.
					// Get the wool.
					{
						CItem *pWool = CItem::CreateBase( ITEMID_WOOL );
						ASSERT(pWool);
						m_pChar->ItemBounce(pWool);
						pCharTarg->SetID(CREID_Sheep_Sheered);
						// Set wool to regrow.
						pWool = CItem::CreateBase( ITEMID_WOOL );
						ASSERT(pWool);
						pWool->SetTimeout( g_Cfg.m_iWoolGrowthTime );
						pCharTarg->LayerAdd( pWool, LAYER_FLAG_Wool );
						pCharTarg->Update();
					}
					return true;
				case CREID_Sheep_Sheered:
					SysMessageDefault( DEFMSG_ITEMUSE_WEAPON_WWAIT );
					return true;
				default:
					// I suppose this is an attack ?
					break;
			}
			break;
		}


		switch ( m_pChar->CanTouchStatic( pt, id, pItemTarg ))
		{
		case IT_JUNK:
			SysMessageDefault( DEFMSG_ITEMUSE_JUNK_REACH );
			return false;
		case IT_FOLIAGE:
		case IT_TREE:
			// Just targetted a tree type

			m_pChar->m_Act_TargPrv = m_Targ_PrvUID;
			m_pChar->m_Act_Targ = m_Targ_UID;
			m_pChar->m_Act_p = pt;
			return( m_pChar->Skill_Start( SKILL_LUMBERJACKING ));

		case IT_LOG:

			if ( ! m_pChar->CanUse( pItemTarg, true ))
			{
				SysMessageDefault( DEFMSG_ITEMUSE_LOG_UNABLE );
				return( false );
			}
			if ( pItemUse->IsType(IT_CARPENTRY_CHOP) )
			{
				if ( IsTrigUsed(TRIGGER_SKILLMENU) )
				{
					CScriptTriggerArgs args("sm_carpentry");
					if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
						return true;
				}
				return Cmd_Skill_Menu( g_Cfg.ResourceGetIDType( RES_SKILLMENU, "sm_carpentry" ));
			}
			if ( pItemUse->IsSameDispID( ITEMID_DAGGER ))
			{
				// set the target item
				m_Targ_UID = pItemTarg->GetUID();
				if ( IsTrigUsed(TRIGGER_SKILLMENU) )
				{
					CScriptTriggerArgs args("sm_bowcraft");
					if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
						return true;
				}
				return Cmd_Skill_Menu( g_Cfg.ResourceGetIDType( RES_SKILLMENU, "sm_bowcraft" ) );
			}
			SysMessageDefault( DEFMSG_ITEMUSE_LOG_USE );
			return false;

		case IT_FISH:
			if ( ! m_pChar->CanUse( pItemTarg, true ))
			{
				SysMessageDefault( DEFMSG_ITEMUSE_FISH_UNABLE );
				return( false );
			}

			// Carve up Fish parts.
			pItemTarg->SetID( ITEMID_FOOD_FISH_RAW );
			pItemTarg->SetAmount( 4 * pItemTarg->GetAmount());
			pItemTarg->Update();
			return true;

		case IT_CORPSE:
			if ( ! m_pChar->CanUse( pItemTarg, false ))
				return( false );
			m_pChar->Use_CarveCorpse( dynamic_cast <CItemCorpse*>( pItemTarg ));
			return true;

		case IT_FRUIT:
		case IT_REAGENT_RAW:
			// turn the fruit into a seed.
			if ( ! m_pChar->CanUse( pItemTarg, true ))
				return( false );
			{
			RESOURCE_ID defaultseed = g_Cfg.ResourceGetIDType( RES_ITEMDEF, "DEFAULTSEED" );
			pItemTarg->SetDispID(static_cast<ITEMID_TYPE>(defaultseed.GetResIndex()));
			pItemTarg->SetType(IT_SEED);
			TCHAR *pszTemp = Str_GetTemp();
			sprintf(pszTemp, "%s seed", pItemTarg->GetName());
			pItemTarg->SetName(pszTemp);
			pItemTarg->Update();
			}
			return( true );

		// case IT_FOLIAGE: // trim back ?
		case IT_CROPS:
			if ( pItemTarg )
				pItemTarg->Plant_CropReset();
			return( true );

		default:
			// Item to smash ? furniture ???

			if ( ! m_pChar->CanMove(pItemTarg) )
			{
				SysMessageDefault( DEFMSG_ITEMUSE_WEAPON_IMMUNE );
				return( false );	// ? using does not imply moving in all cases ! such as reading ?
			}

			// Is breaking this a crime ?
			if ( m_pChar->IsTakeCrime( pItemTarg ))
			{
				SysMessageDefault( DEFMSG_ITEMUSE_STEAL );
				return( false );
			}

			if ( pItemTarg )
				pItemTarg->OnTakeDamage( 1, m_pChar, DAMAGE_HIT_BLUNT );
			return( true );
		}
		break;

	case IT_BANDAGE:	// SKILL_HEALING, or SKILL_VETERINARY
		// Use bandages on some creature.
		if ( pCharTarg == NULL )
			return( false );
		m_pChar->m_Act_TargPrv = m_Targ_PrvUID;
		m_pChar->m_Act_Targ = m_Targ_UID;
		return( m_pChar->Skill_Start( (pCharTarg->GetNPCBrain() == NPCBRAIN_ANIMAL) ? SKILL_VETERINARY : SKILL_HEALING ));

	case IT_SEED:
		return m_pChar->Use_Seed( pItemUse, &pt );

	case IT_DEED:
		return( OnTarg_Use_Deed( pItemUse, pt ));

	case IT_WOOL:
	case IT_COTTON:
		// Use on a spinning wheel.
		if ( pItemTarg == NULL )
			break;
		if ( ! pItemTarg->IsType(IT_SPINWHEEL))
			break;
		if ( ! m_pChar->CanUse( pItemTarg, false ))
			return( false );

		pItemTarg->SetAnim(static_cast<ITEMID_TYPE>( pItemTarg->GetID() + 1 ), 2 * TICK_PER_SEC);
		pItemUse->ConsumeAmount( 1 );

		{
			CItem * pNewItem;
			if ( pItemUse->IsType(IT_WOOL))
			{
				// 1 pile of wool yields three balls of yarn
				SysMessageDefault( DEFMSG_ITEMUSE_WOOL_CREATE );
				pNewItem = CItem::CreateScript( ITEMID_YARN1, m_pChar );
				if ( pNewItem->GetAmount() == 1 )
					pNewItem->SetAmountUpdate( 3 );
			}
			else
			{
				// 1 pile of cotton yields six spools of thread
				SysMessageDefault( DEFMSG_ITEMUSE_COTTON_CREATE );
				pNewItem = CItem::CreateScript( ITEMID_THREAD1, m_pChar );
				if ( pNewItem->GetAmount() == 1 )
					pNewItem->SetAmountUpdate( 6 );
			}
			m_pChar->ItemBounce( pNewItem );
		}
		return true;

	case IT_KEYRING:
		// it acts as a key.
		{
		if ( pItemTarg == NULL )
			return( false );
		CItemContainer* pKeyRing = dynamic_cast <CItemContainer*>(pItemUse);
		if ( pKeyRing == NULL )
			return( false );

		if ( pItemTarg == pItemUse )
		{
			// Use a keyring on self = remove all keys.
			pKeyRing->ContentsTransfer( m_pChar->GetPack(), false );
			return( true );
		}

		CItem * pKey = NULL;
		bool fLockable = pItemTarg->IsTypeLockable();

		if ( fLockable && pItemTarg->m_itContainer.m_lockUID )
		{
			// try all the keys on the object.
			pKey = pKeyRing->ContentFind( RESOURCE_ID(RES_TYPEDEF,IT_KEY), pItemTarg->m_itContainer.m_lockUID );
		}
		if ( pKey == NULL )
		{
			// are we trying to lock it down ?
			if ( m_pChar->m_pArea->GetResourceID().IsItem())
			{
				pKey = pKeyRing->ContentFind( RESOURCE_ID(RES_TYPEDEF,IT_KEY), m_pChar->m_pArea->GetResourceID() );
				if ( pKey )
				{
					if ( m_pChar->Use_MultiLockDown( pItemTarg ))
					{
						return( true );
					}
				}
			}

			if ( ! fLockable || ! pItemTarg->m_itContainer.m_lockUID )
			{
				SysMessageDefault( DEFMSG_ITEMUSE_KEY_NOLOCK );
			}
			else
			{
				SysMessageDefault( DEFMSG_ITEMUSE_KEY_NOKEY );
			}
			return false;
		}

		return( m_pChar->Use_Key( pKey, pItemTarg ));
		}

	case IT_SCISSORS:
		if ( pItemTarg != NULL )
		{
			if ( ! m_pChar->CanUse( pItemTarg, true ))
				return( false );

			// Cut up cloth.
			int iOutQty = 0;
			ITEMID_TYPE iOutID = ITEMID_BANDAGES1;

			switch ( pItemTarg->GetType())
			{
				case IT_CLOTH_BOLT:
					// Just make cut cloth here !
					pItemTarg->ConvertBolttoCloth();
					m_pChar->Sound( SOUND_SNIP );	// snip noise.
					return( true );
				case IT_CLOTH:
					iOutQty = pItemTarg->GetAmount();
					break;
				case IT_CLOTHING:
					// Cut up for bandages.
					iOutQty = pItemTarg->GetWeight()/WEIGHT_UNITS;
					break;
				case IT_HIDE:
					// IT_LEATHER
					// Cut up the hides and create strips of leather
					iOutID = static_cast<ITEMID_TYPE>(RES_GET_INDEX(pItemTarg->Item_GetDef()->m_ttNormal.m_tData1));
					if ( ! iOutID )
						iOutID = ITEMID_LEATHER_1;
					iOutQty = pItemTarg->GetAmount();
					break;
				default:
					break;
			}
			if ( iOutQty )
			{
				CItem * pItemNew = CItem::CreateBase( iOutID );
				ASSERT(pItemNew);
				pItemNew->SetHue( pItemTarg->GetHue());
				pItemNew->SetAmount( iOutQty );
				m_pChar->ItemBounce( pItemNew );
				m_pChar->Sound( SOUND_SNIP );	// snip noise.
				pItemTarg->Delete();
				return( true );
			}
		}
		SysMessageDefault( DEFMSG_ITEMUSE_SCISSORS_USE );
		return false;

	case IT_YARN:
	case IT_THREAD:
		// Use this on a loom.
		// use on a spinning wheel.
		if ( pItemTarg == NULL )
			break;
		if ( ! pItemTarg->IsType( IT_LOOM ))
			break;
		if ( ! m_pChar->CanUse( pItemTarg, false ))
			return( false );

		{
static LPCTSTR const sm_Txt_LoomUse[] =
{
	g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_BOLT_1 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_BOLT_2 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_BOLT_3 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_BOLT_4 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_BOLT_5 )
};

		// pItemTarg->SetAnim(static_cast<ITEMID_TYPE>(pItemTarg->GetID() + 1), 2 * TICK_PER_SEC );

		// Use more1 to record the type of resource last used on this object
		// Use more2 to record the number of resources used so far
		// Check what was used last.
		if ( pItemTarg->m_itLoom.m_ClothID != pItemUse->GetDispID() &&
			pItemTarg->m_itLoom.m_ClothID )
		{
			// throw away what was on here before
			SysMessageDefault( DEFMSG_ITEMUSE_LOOM_REMOVE );
			CItem * pItemCloth = CItem::CreateTemplate( pItemTarg->m_itLoom.m_ClothID, NULL, m_pChar );
			pItemCloth->SetAmount( pItemTarg->m_itLoom.m_ClothQty );
			pItemTarg->m_itLoom.m_ClothQty = 0;
			pItemTarg->m_itLoom.m_ClothID = ITEMID_NOTHING;
			m_pChar->ItemBounce( pItemCloth );
			return true;
		}

		pItemTarg->m_itLoom.m_ClothID = pItemUse->GetDispID();

		int iUsed = 0;
		int iNeed = COUNTOF( sm_Txt_LoomUse ) - 1;
		int iHave = pItemTarg->m_itLoom.m_ClothQty;
		if ( iHave < iNeed )
		{
			iNeed -= iHave;
			iUsed = pItemUse->ConsumeAmount( iNeed );
		}

		if ( static_cast<unsigned int>(iHave  + iUsed) < (COUNTOF( sm_Txt_LoomUse ) - 1) )
		{
			pItemTarg->m_itLoom.m_ClothQty += iUsed;
			SysMessage( sm_Txt_LoomUse[ pItemTarg->m_itLoom.m_ClothQty ] );
		}
		else
		{
			SysMessage( sm_Txt_LoomUse[ COUNTOF( sm_Txt_LoomUse ) - 1 ] );
			pItemTarg->m_itLoom.m_ClothQty = 0;
			pItemTarg->m_itLoom.m_ClothID = ITEMID_NOTHING;

/*
			CItemBase * pItemDef = pItemTarg->Item_GetDef(); 

			if ( pItemDef->m_ttNormal.m_tData3 != 0 )
			{
				for ( int clothcount=1; clothcount < pItemDef->m_ttNormal.m_tData3; clothcount++)
				{
					m_pChar->ItemBounce( CItem::CreateScript(ITEMID_CLOTH_BOLT1, m_pChar ));
				}
			} else {
*/

				m_pChar->ItemBounce( CItem::CreateScript(ITEMID_CLOTH_BOLT1, m_pChar ));
/*			}	
*/
		}
		}
		return true;

	case IT_BANDAGE_BLOOD:
		// Use these on water to clean them.
		switch ( m_pChar->CanTouchStatic( pt, id, pItemTarg ))
		{
			case IT_WATER:
			case IT_WATER_WASH:
				// Make clean.
				pItemUse->SetID( ITEMID_BANDAGES1 );
				pItemUse->Update();
				return( true );
			case IT_JUNK:
				SysMessageDefault( DEFMSG_ITEMUSE_BANDAGE_REACH );
				break;
			default:
				SysMessageDefault( DEFMSG_ITEMUSE_BANDAGE_CLEAN );
				break;
		}
		return( false );

	case IT_FISH_POLE:
		m_pChar->m_Act_p = pt;
		return( m_pChar->Skill_Start( SKILL_FISHING ));

	case IT_LOCKPICK:
		// Using a lock pick on something.
		if ( pItemTarg== NULL )
			return( false );
		m_pChar->m_Act_Targ = m_Targ_UID;	// the locked item to be picked
		m_pChar->m_Act_TargPrv = m_Targ_PrvUID;	// the pick
		if ( ! m_pChar->CanUse( pItemTarg, false ))
			return( false );
		return( m_pChar->Skill_Start( SKILL_LOCKPICKING ));

	case IT_CANNON_BALL:
		if ( m_pChar->Use_Cannon_Feed( pItemTarg, pItemUse ))
		{
			pItemUse->Delete();
			return( true );
		}
		break;

	case IT_DYE:
		if (( pItemTarg != NULL && pItemTarg->IsType(IT_DYE_VAT)) ||
			( pCharTarg != NULL && ( pCharTarg == m_pChar || IsPriv( PRIV_GM ))))	// Change skin color.
		{
			addDyeOption( pObjTarg );
			return true;
		}
		SysMessageDefault( DEFMSG_ITEMUSE_DYE_FAIL );
		return false;

	case IT_DYE_VAT:
		{
		// Use the dye vat on some object.
		if ( pObjTarg == NULL )
			return false;

		if ( pObjTarg->GetTopLevelObj() != m_pChar &&
			! IsPriv( PRIV_GM ))	// Change hair wHue.
		{
			SysMessageDefault( DEFMSG_ITEMUSE_DYE_REACH );
			return false;
		}
		if ( pCharTarg != NULL )
		{
			// Dye hair.
			pObjTarg = pCharTarg->LayerFind( LAYER_HAIR );
			if ( pObjTarg != NULL )
			{
				pObjTarg->SetHue( pItemUse->GetHue(), false, this->GetChar(), pItemUse, 0x23e);
				pObjTarg->Update();
			}
			pObjTarg = pCharTarg->LayerFind( LAYER_BEARD );
			if ( pObjTarg == NULL )
				return true;
			// fall through
		}
		else
		{
			if ( ! m_pChar->CanUse( pItemTarg, false ))
				return( false );
			if ( ! IsPriv( PRIV_GM ) &&
				! pItemTarg->Item_GetDef()->Can( CAN_I_DYE ) &&
				! pItemTarg->IsType(IT_CLOTHING) )
			{
				SysMessageDefault( DEFMSG_ITEMUSE_DYE_FAIL );
				return false;
			}
		}

		pObjTarg->SetHue( pItemUse->GetHue(), false, this->GetChar(), pItemUse, 0x23e );
		pObjTarg->Update();
		return true;
		}
		break;

	case IT_PITCHER_EMPTY:
		// Fill it up with water.
		switch ( m_pChar->CanTouchStatic( pt, id, pItemTarg ))
		{
			case IT_JUNK:
				SysMessageDefault( DEFMSG_ITEMUSE_PITCHER_REACH );
				return( false );
			case IT_WATER:
			case IT_WATER_WASH:
				pItemUse->SetID( ITEMID_PITCHER_WATER );
				pItemUse->Update();
				return( true );
			default:
				SysMessageDefault( DEFMSG_ITEMUSE_PITCHER_FILL );
				return( false );
		}
		break;

	case IT_SEWING_KIT:
		// Use on cloth or hides
		if ( pItemTarg == NULL)
			break;
		if ( ! m_pChar->CanUse( pItemTarg, true ))
			return( false );

		switch ( pItemTarg->GetType())
		{
			case IT_LEATHER:
			case IT_HIDE:
			{
				if ( IsTrigUsed(TRIGGER_SKILLMENU) )
				{
					CScriptTriggerArgs args("sm_tailor_leather");
					if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
						return true;
				}
				return Cmd_Skill_Menu( g_Cfg.ResourceGetIDType( RES_SKILLMENU, "sm_tailor_leather" ) );
			}
			case IT_CLOTH:
			{
				if ( IsTrigUsed(TRIGGER_SKILLMENU) )
				{
					CScriptTriggerArgs args("sm_tailor_cloth");
					if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
						return true;
				}
				return Cmd_Skill_Menu( g_Cfg.ResourceGetIDType( RES_SKILLMENU, "sm_tailor_cloth" ) );
			}
			default:
				break;
		}

		SysMessageDefault( DEFMSG_ITEMUSE_SKIT_UNABLE );
		return (false);

	default:
		break;
	}

	SysMessageDefault( DEFMSG_ITEMUSE_UNABLE );
	return false;
}

bool CClient::OnTarg_Stone_Recruit(CChar* pChar, bool bFull)
{
	ADDTOCALLSTACK("CClient::OnTarg_Stone_Recruit");
	// CLIMODE_TARG_STONE_RECRUIT / CLIMODE_TARG_STONE_RECRUITFULL
	CItemStone * pStone = dynamic_cast <CItemStone*> (m_Targ_UID.ItemFind());
	if ( !pStone )
		return false;
	return ( pStone->AddRecruit(pChar, STONEPRIV_CANDIDATE, bFull) != NULL );
}

bool CClient::OnTarg_Party_Add( CChar * pChar )
{
	ADDTOCALLSTACK("CClient::OnTarg_Party_Add");
	// CLIMODE_TARG_PARTY_ADD
	// Invite this person to join our party. PARTYMSG_Add

	if ( pChar == NULL )
	{
		SysMessageDefault( DEFMSG_PARTY_SELECT );
		return( false );
	}

	if ( pChar == m_pChar )
	{
		SysMessageDefault( DEFMSG_PARTY_NOSELFADD );
		return( false );
	}

	if ( !pChar->IsClient() )
	{
		SysMessageDefault( DEFMSG_PARTY_NONPCADD );
		return( false );
	}

	if ( m_pChar->m_pParty )
	{
		if ( !(m_pChar->m_pParty->IsPartyMaster(m_pChar)) )
		{
			SysMessageDefault( DEFMSG_PARTY_ADD_NOTLEADER );
			return( false );
		}

		if ( m_pChar->m_pParty->IsPartyFull() )
		{
			SysMessageDefault( DEFMSG_PARTY_IS_FULL );
			return( false );
		}
	}

	if (IsPriv(PRIV_GM) && (pChar->GetClient()->GetPrivLevel() < GetPrivLevel()))
	{
		CPartyDef::AcceptEvent(pChar, m_pChar->GetUID(), true);
		return true;
	}

	if ( pChar->m_pParty != NULL )	// Aready in a party !
	{
		if ( m_pChar->m_pParty == pChar->m_pParty )	// already in this party
		{
			SysMessageDefault( DEFMSG_PARTY_ALREADY_IN_THIS );
			return( true );
		}

		SysMessageDefault( DEFMSG_PARTY_ALREADY_IN );
		return( false );
	}

	if ( pChar->GetKeyNum("PARTY_AUTODECLINEINVITE", true) )
	{
		SysMessageDefault( DEFMSG_PARTY_AUTODECLINE );
		return( false );
	}

	CVarDefCont * pTagInvitetime = m_pChar->m_TagDefs.GetKey("PARTY_LASTINVITETIME");
	if ( pTagInvitetime && ( g_World.GetCurrentTime().GetTimeRaw() < pTagInvitetime->GetValNum() ) )
	{
		SysMessageDefault( DEFMSG_PARTY_ADD_TOO_FAST );
		return( false );
	}

	if ( IsTrigUsed(TRIGGER_PARTYINVITE) )
	{
		CScriptTriggerArgs args;
		if ( pChar->OnTrigger(CTRIG_PartyInvite, m_pChar, &args) == TRIGRET_RET_TRUE )
			return( false );
	}

	TCHAR * sTemp = Str_GetTemp();
	sprintf(sTemp, g_Cfg.GetDefaultMsg( DEFMSG_PARTY_INVITE ), static_cast<LPCTSTR>(pChar->GetName()));
	m_pChar->SysMessage( sTemp );
	sTemp = Str_GetTemp();
	sprintf(sTemp, g_Cfg.GetDefaultMsg( DEFMSG_PARTY_INVITE_TARG ), static_cast<LPCTSTR>(m_pChar->GetName()));
	pChar->SysMessage( sTemp );

	m_pChar->SetKeyNum("PARTY_LASTINVITE", static_cast<DWORD>(pChar->GetUID()));
	m_pChar->SetKeyNum("PARTY_LASTINVITETIME", g_World.GetCurrentTime().GetTimeRaw() + (Calc_GetRandVal2(2,5) * TICK_PER_SEC));

	new PacketPartyInvite(pChar->GetClient(), m_pChar);

	// Now up to them to decide to accept.
	return( true );
}
