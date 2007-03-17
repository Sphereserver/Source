//
// CClientMsg.cpp
// Copyright Menace Software (www.menasoft.com).
//
// Game server messages. (No login stuff)
//

#include "graysvr.h"	// predef header.
#include "CClient.h"

/////////////////////////////////////////////////////////////////
// -CClient stuff.

void CClient::resendBuffs()
{
	// These checks are in addBuff too, but it would be useless to call it so many times
	if ( !IsSetOF(OF_Buffs) )
		return;
	if (!IsResClient(RDS_AOS))
		return;
	if ( !IsClientVersion(0x500030) )
		return;

	CContainer* Cont = dynamic_cast<CContainer*>(GetChar());
	ASSERT(Cont);

	CItem* pSpell = 0;
	BYTE WideMsg[18] = {0};
	char NumBuff[4] = {0};
	short iBuffPercent = 0;
	int iStatEffect = 0;

	for ( int i = 0; i != Cont->GetCount(); ++i )
	{
		pSpell = Cont->GetAt(i);
		if ( !pSpell )
			continue;
		if ( !(pSpell->IsType(IT_SPELL)) )
			continue;
		iStatEffect = g_Cfg.GetSpellEffect( (SPELL_TYPE)RES_GET_INDEX(pSpell->m_itSpell.m_spell), pSpell->m_itSpell.m_spelllevel );
		switch( pSpell->m_itSpell.m_spell )
		{
		case SPELL_Night_Sight:
			addBuff( BI_NIGHTSIGHT,1075643,1075644,(WORD)(pSpell->GetTimerAdjusted()) );
			break;
		case SPELL_Clumsy:
			iBuffPercent = GetStatPercentage( GetChar(), STAT_DEX, iStatEffect );
			ITOA(iBuffPercent, NumBuff, 10);
			CharToMultiByteNonNull(WideMsg, NumBuff, 3);
			addBuff( BI_CLUMSY, 1075831,1075832,(WORD)(pSpell->GetTimerAdjusted()),WideMsg  );
			break;
		case SPELL_Weaken:

			iBuffPercent = GetStatPercentage( GetChar(), STAT_STR, iStatEffect );
			ITOA(iBuffPercent, NumBuff, 10);
			CharToMultiByteNonNull(WideMsg, NumBuff, 3);
			addBuff( BI_WEAKEN, 1075837,1075838,(WORD)(pSpell->GetTimerAdjusted()),WideMsg );
			break;
		case SPELL_Feeblemind:
			iBuffPercent = GetStatPercentage( GetChar(), STAT_INT, iStatEffect );
			ITOA(iBuffPercent, NumBuff, 10);
			CharToMultiByteNonNull(WideMsg, NumBuff, 3);
			addBuff( BI_FEEBLEMIND, 1075833,1075834,(WORD)(pSpell->GetTimerAdjusted()),WideMsg  );
			break;
		case SPELL_Curse:
			for( char idx = STAT_STR; idx != STAT_BASE_QTY; ++idx)
			{
				iBuffPercent = GetStatPercentage( GetChar(), static_cast<STAT_TYPE>(idx), iStatEffect );
				ITOA(iBuffPercent, NumBuff, 10);
				CharToMultiByteNonNull(WideMsg + (idx*6), NumBuff, 3);
			}
			addBuff( BI_CURSE, 1075835,1075840,(WORD)(pSpell->GetTimerAdjusted()),WideMsg  );
			break;
		case SPELL_Strength:
			iBuffPercent = GetStatPercentage( GetChar(), STAT_STR, iStatEffect );
			ITOA(iBuffPercent, NumBuff, 10);
			CharToMultiByteNonNull(WideMsg, NumBuff, 3);
			addBuff( BI_STRENGTH, 0x106A85,0x106A86,(WORD)(pSpell->GetTimerAdjusted()),WideMsg  );
			break;
		case SPELL_Agility:
			iBuffPercent = GetStatPercentage( GetChar(), STAT_DEX, iStatEffect );
			ITOA(iBuffPercent, NumBuff, 10);
			CharToMultiByteNonNull(WideMsg, NumBuff, 3);
			addBuff( BI_AGILITY, 0x106A85,0x106A86,(WORD)(pSpell->GetTimerAdjusted()),WideMsg  );
			break;
		case SPELL_Cunning:
			iBuffPercent = GetStatPercentage( GetChar(), STAT_INT, iStatEffect );
			ITOA(iBuffPercent, NumBuff, 10);
			CharToMultiByteNonNull(WideMsg, NumBuff, 3);
			addBuff( BI_CUNNING, 0x106A85,0x106A86,(WORD)(pSpell->GetTimerAdjusted()),WideMsg  );
			break;
		case SPELL_Bless:
			for( char idx = STAT_STR; idx != STAT_BASE_QTY; ++idx)
			{
				iBuffPercent = GetStatPercentage( GetChar(), static_cast<STAT_TYPE>(idx), iStatEffect );
				ITOA(iBuffPercent, NumBuff, 10);
				CharToMultiByteNonNull(WideMsg + (idx*6), NumBuff, 3);
			}
			addBuff( BI_BLESS, 1075847,1075848,(WORD)(pSpell->GetTimerAdjusted()), WideMsg );
			break;
		case SPELL_Reactive_Armor:
			addBuff( BI_REACTIVEARMOR, 1075812,1070722,(WORD)(pSpell->GetTimerAdjusted()) );
			break;
		case SPELL_Protection:
			addBuff( BI_PROTECTION, 1075814,1070722,(WORD)(pSpell->GetTimerAdjusted()) );
			break;
		case SPELL_Arch_Prot:
			addBuff( BI_ARCHPROTECTION, 1075816,1070722,(WORD)(pSpell->GetTimerAdjusted()) );
			break;
		case SPELL_Poison:
			addBuff( BI_POISON, 1017383,1070722,(WORD)(pSpell->GetTimerAdjusted()) );
			break;
		case SPELL_Incognito:
			addBuff( BI_INCOGNITO, 1075819,1075820,(WORD)(pSpell->GetTimerAdjusted()) );
			break;
		case SPELL_Paralyze:
			addBuff( BI_PARALYZE, 1075827,1075828,(WORD)(pSpell->GetTimerAdjusted()) );
			break;
		case SPELL_Magic_Reflect:
			addBuff( BI_MAGICREFLECTION, 1075817,1070722,(WORD)(pSpell->GetTimerAdjusted()) );
			break;
		case SPELL_Invis:
			addBuff( BI_INVISIBILITY,1075825,1075826,(WORD)(pSpell->GetTimerAdjusted()) );
			break;
		//case SPELL_Mass_Curse:
		//	break;

		}

	}

}
void CClient::addBuff( const WORD IconId, const DWORD ClilocOne, const DWORD ClilocTwo, const short Time, BYTE * pText)
{
	ADDTOCALLSTACK("CClient::addBuff");
	if ( !IsSetOF(OF_Buffs) )
		return;
	if (!IsResClient(RDS_AOS))
		return;
	if ( !IsClientVersion(0x500030) )
		return;

	bool bSelfAlloc = false;
	if ( pText == 0 )
	{
		bSelfAlloc = true;
		pText = new BYTE[18];
		for ( size_t i = 0; i != 18; ++i )
		{
			pText[i] = 0;
		}
	}
	for ( BYTE* BytePtr = pText; BytePtr != pText + 18; BytePtr += 2 )
	{
		if ( *BytePtr == 0 )
		{
			*BytePtr = 1;
		}
	}

	CCommand Cmd;
	Cmd.AddBuff.m_Cmd = 0xDF;
	Cmd.AddBuff.m_Length = 72;
	Cmd.AddBuff.m_Uid = m_pChar->GetUID();
	Cmd.AddBuff.m_IconId_1 = IconId;
	Cmd.AddBuff.m_Show_1 = 0x1;
	Cmd.AddBuff.m_unk_1 = 0x0;
	Cmd.AddBuff.m_IconId_2 = IconId;
	Cmd.AddBuff.m_Show_2 = 0x1;
	Cmd.AddBuff.m_unk_2 = 0x0;
	Cmd.AddBuff.m_Time = Time;
	Cmd.AddBuff.m_unk_3 = 0x0;
	Cmd.AddBuff.m_unk_4 = 0x0;
	Cmd.AddBuff.m_ClilocOne = ClilocOne;
	Cmd.AddBuff.m_ClilocTwo = ClilocTwo;
	Cmd.AddBuff.m_unk_5 = 0x0;
	Cmd.AddBuff.m_unk_6 = 0x1;
	Cmd.AddBuff.m_unk_7 = 0x0;
	memset((void*)Cmd.AddBuff.m_MBUText_1, 0, sizeof(BYTE) * 6);
	memset((void*)Cmd.AddBuff.m_MBUText_2, 0, sizeof(BYTE) * 6);
	memset((void*)Cmd.AddBuff.m_MBUText_3, 0, sizeof(BYTE) * 6);
	memcpy((void*)Cmd.AddBuff.m_MBUText_1, (void*) pText, sizeof(BYTE) * 6);
	memcpy((void*)Cmd.AddBuff.m_MBUText_2, (void*) (pText + 6), sizeof(BYTE) * 6);
	memcpy((void*)Cmd.AddBuff.m_MBUText_3, (void*) (pText + 12), sizeof(BYTE) * 6);
	Cmd.AddBuff.m_MBUTab_1[0] = 9;
	Cmd.AddBuff.m_MBUTab_1[1] = 0;
	Cmd.AddBuff.m_MBUTab_2[0] = 9;
	Cmd.AddBuff.m_MBUTab_2[1] = 0;
	Cmd.AddBuff.m_MBUTab_3[0] = 9;
	Cmd.AddBuff.m_MBUTab_3[1] = 0;
	Cmd.AddBuff.m_unk_8 = 0x0;

	if ( bSelfAlloc )
	{
		delete[] pText;
	}

	xSendPkt(&Cmd, Cmd.AddBuff.m_Length);
}

void CClient::removeBuff (const WORD IconId)
{
	ADDTOCALLSTACK("CClient::removeBuff");
	if ( !IsSetOF(OF_Buffs) )
		return;
	if (!IsResClient(RDS_AOS))
		return;
	if ( !IsClientVersion(0x500030) )
		return;
	CCommand Cmd;
	Cmd.RemoveBuff.m_Cmd = 0xDF;
	Cmd.RemoveBuff.m_Length = 15;
	Cmd.RemoveBuff.m_Uid = m_pChar->GetUID();
	Cmd.RemoveBuff.m_IconId = IconId;
	Cmd.RemoveBuff.m_Show = 0x0;
	Cmd.RemoveBuff.m_unk_1 = 0x0;
	xSendPkt(&Cmd, Cmd.RemoveBuff.m_Length);
}


bool CClient::addDeleteErr( DELETE_ERR_TYPE code )
{
	ADDTOCALLSTACK("CClient::addDeleteErr");
	// code
	if ( code == DELETE_SUCCESS )
		return true;

	DEBUG_ERR(( "%x:Bad Char Delete Attempted %d\n", m_Socket.GetSocket(), code ));
	CCommand cmd;
	cmd.DeleteBad.m_Cmd = XCMD_DeleteBad;
	cmd.DeleteBad.m_code = code;
	xSendPkt( &cmd, sizeof( cmd.DeleteBad ));
	xFlush();
	return( false );
}

void CClient::addExtData( EXTDATA_TYPE type, const CExtData * pData, int iSize)
{
	ADDTOCALLSTACK("CClient::addExtData");
	CCommand cmd;
	int iSizeTotal = iSize + sizeof(cmd.ExtData.m_Cmd) + sizeof(cmd.ExtData.m_len) + sizeof(cmd.ExtData.m_type);
	//g_Log.EventDebug("CClient::addExtData iSize: %d, iSizeTotal: %d\n",iSize,iSizeTotal);
	cmd.ExtData.m_Cmd = XCMD_ExtData;
	cmd.ExtData.m_len = iSizeTotal;
	cmd.ExtData.m_type = type;
	memcpy( &(cmd.ExtData.m_u), pData, iSize );
	xSendPkt( &cmd, iSizeTotal );
}

void CClient::addTime( bool bCurrent )
{
	ADDTOCALLSTACK("CClient::addTime");
	// Send time. (real or game time ??? why ?)
	CCommand cmd;

	if ( bCurrent )
	{
		long lCurrentTime = (CServTime::GetCurrentTime()).GetTimeRaw();
		cmd.Time.m_Cmd = XCMD_Time;
		cmd.Time.m_hours = ( lCurrentTime / ( 60*60*TICK_PER_SEC )) % 24;
		cmd.Time.m_min   = ( lCurrentTime / ( 60*TICK_PER_SEC )) % 60;
		cmd.Time.m_sec   = ( lCurrentTime / ( TICK_PER_SEC )) % 60;
	}
	else
	{
		cmd.Time.m_Cmd = XCMD_Time;
		cmd.Time.m_hours = 0;
		cmd.Time.m_min   = 0;
		cmd.Time.m_sec   = 0;
	}

	xSendPkt( &cmd, sizeof( cmd.Time ));
}

void CClient::addObjectRemoveCantSee( CGrayUID uid, LPCTSTR pszName )
{
	ADDTOCALLSTACK("CClient::addObjectRemoveCantSee");
	// Seems this object got out of sync some how.
	if ( pszName == NULL ) pszName = "it";
	SysMessagef( "You can't see %s", pszName );
	addObjectRemove( uid );
}

void CClient::addObjectRemove( CGrayUID uid )
{
	ADDTOCALLSTACK("CClient::addObjectRemove");
	// Tell the client to remove the item or char
	CCommand cmd;
	cmd.Remove.m_Cmd = XCMD_Remove;
	cmd.Remove.m_UID = uid;
	xSendPkt( &cmd, sizeof( cmd.Remove ));
}

void CClient::addObjectRemove( const CObjBase * pObj )
{
	ADDTOCALLSTACK("CClient::addObjectRemove");
	addObjectRemove( pObj->GetUID());
}

void CClient::addRemoveAll( bool fItems, bool fChars )
{
	ADDTOCALLSTACK("CClient::addRemoveAll");
	if ( fItems )
	{
		// Remove any multi objects first ! or client will hang
		CWorldSearch AreaItems( GetChar()->GetTopPoint(), UO_MAP_VIEW_RADAR );
		AreaItems.SetAllShow( IsPriv( PRIV_ALLSHOW ));	// show logged out chars?
		while (true)
		{
			CItem * pItem = AreaItems.GetItem();
			if ( pItem == NULL )
				break;
			addObjectRemove( pItem );
		}
	}
	if ( fChars )
	{
		// Remove any multi objects first ! or client will hang
		CWorldSearch AreaChars( GetChar()->GetTopPoint(), UO_MAP_VIEW_SIZE );
		AreaChars.SetAllShow( IsPriv( PRIV_ALLSHOW ));	// show logged out chars?
		while (true)
		{
			CChar * pChar = AreaChars.GetChar();
			if ( pChar == NULL )
				break;
			addObjectRemove( pChar );
		}
	}
}

void CClient::addItem_OnGround( CItem * pItem ) // Send items (on ground)
{
	ADDTOCALLSTACK("CClient::addItem_OnGround");
	ASSERT(pItem);

	// Get base values.
	DWORD dwUID = pItem->GetUID();
	ITEMID_TYPE wID = pItem->GetDispID();
	HUE_TYPE wHue = pItem->GetHue();
	WORD wAmount = ( pItem->GetAmount() > 1 ) ? pItem->GetAmount() : 0;
	CPointMap pt = pItem->GetTopPoint();
	BYTE bFlags = 0;
	BYTE bDir = DIR_N;

	// Modify the values for the specific client/item.
	bool fHumanCorpse = ( wID == ITEMID_CORPSE && CCharBase::IsHumanID( pItem->GetCorpseType() ));

	if ( wID != ITEMID_CORPSE )
	{
		CItemBase * pItemDef = pItem->Item_GetDef();
		if ( pItemDef && ( GetResDisp() < pItemDef->GetResLevel() ) )
		{
			wID = (ITEMID_TYPE) pItemDef->GetResDispDnId();
			if ( pItemDef->GetResDispDnHue() != HUE_DEFAULT )
				wHue = pItemDef->GetResDispDnHue();
		}

		// On monster this just colors the underwaer. thats it.
		if ( wHue & HUE_UNDERWEAR )
			wHue = 0;
		else if (( wHue & HUE_MASK_HI ) > HUE_QTY )
			wHue &= HUE_MASK_LO | HUE_TRANSLUCENT;
		else
			wHue &= HUE_MASK_HI | HUE_TRANSLUCENT;
	}
	else
	{
		// Adjust amount and hue of corpse if necessary
		CCharBase * pCharDef = CCharBase::FindCharBase( pItem->m_itCorpse.m_BaseID );
		if (( pCharDef ) && ( GetResDisp() < pCharDef->GetResLevel() ))
		{
			wAmount = pCharDef->GetResDispDnId();
			if ( pCharDef->GetResDispDnHue() != HUE_DEFAULT )
				wHue = pCharDef->GetResDispDnHue();
		}

		// allow HUE_UNDERWEAR colors only on corpses
		if (( wHue & HUE_MASK_HI ) > HUE_QTY )
			wHue &= HUE_MASK_LO | HUE_UNDERWEAR | HUE_TRANSLUCENT;
		else
			wHue &= HUE_MASK_HI | HUE_UNDERWEAR | HUE_TRANSLUCENT;
	}

	if ( m_pChar->IsStatFlag( STATF_Hallucinating ))
	{
		// cmd.Put.m_id = pItem->GetDispID();	// ??? random displayable item ?
		wHue = Calc_GetRandVal( HUE_DYE_HIGH );	// restrict colors
	}

	if ( m_pChar->CanMove( pItem, false ))	// wID != ITEMID_CORPSE &&
	{
		bFlags |= ITEMF_MOVABLE;
	}

	if ( IsPriv(PRIV_DEBUG))
	{
		// just use safe numbers
		wID = ITEMID_WorldGem;	// bigger item ???
		pt.m_z = m_pChar->GetTopZ();
		wAmount = 0;
		bFlags |= ITEMF_MOVABLE;
	}
	else
	{
		if ( ! m_pChar->CanSeeItem( pItem ))
		{
			bFlags |= ITEMF_INVIS;
		}
		if ( pItem->Item_GetDef()->Can( CAN_I_LIGHT ))	// ??? Gate or other ? IT_LIGHT_LIT ?
		{
			if ( pItem->IsTypeLit())
			{
				bDir = pItem->m_itLight.m_pattern;
			}
			else
			{
				bDir = LIGHT_LARGE;
			}
		}
		else if ( wID == ITEMID_CORPSE )	// IsType( IT_CORPSE )
		{
			// If this item has direction facing use it
			bDir = pItem->m_itCorpse.m_facing_dir;	// DIR_N
		}
	}

	// Pack values in our strange format.
	CCommand cmd;
	BYTE * pData = cmd.m_Raw + 9;

	cmd.Put.m_Cmd = XCMD_Put;

	if ( wAmount ) dwUID |= UID_F_RESOURCE;	// Enable amount feild
	cmd.Put.m_UID = dwUID;

	cmd.Put.m_id = wID;
	if ( wAmount )
	{
		PACKWORD(pData,wAmount);
		pData += 2;
	}

	if ( bDir ) pt.m_x |= 0x8000;
	PACKWORD(pData,pt.m_x);
	pData += 2;

	if ( wHue ) pt.m_y |= 0x8000;	 // Enable m_wHue and m_movable
	if ( bFlags ) pt.m_y |= 0x4000;
	PACKWORD(pData,pt.m_y);
	pData += 2;

	if ( bDir )
	{
		pData[0] = bDir;
		pData++;
	}

	pData[0] = pt.m_z;
	pData++;

	if ( wHue )
	{
		PACKWORD(pData,wHue);
		pData += 2;
	}

	if ( bFlags )	// m_flags = ITEMF_MOVABLE (020, 080)
	{
		pData[0] = bFlags;
		pData++;
	}

	int iLen = pData - (cmd.m_Raw);
	ASSERT( iLen );
	cmd.Put.m_len = iLen;

	xSendPkt( &cmd, iLen );

	if ( pItem->IsType(IT_SOUND))
	{
		addSound( (SOUND_TYPE) pItem->m_itSound.m_Sound, pItem, pItem->m_itSound.m_Repeat );
	}

	if ( ! IsPriv(PRIV_DEBUG) && fHumanCorpse )	// cloths on corpse
	{
		CItemCorpse * pCorpse = dynamic_cast <CItemCorpse*> (pItem);
		ASSERT(pCorpse);

		// send all the items on the corpse.
		addContents( pCorpse, false, true, false );
		// equip the proper items on the corpse.
		addContents( pCorpse, true, true, false );
	}

	addAOSTooltip( pItem );
}

void CClient::addItem_Equipped( const CItem * pItem )
{
	ADDTOCALLSTACK("CClient::addItem_Equipped");
	ASSERT(pItem);
	// Equip a single item on a CChar.
	CChar * pChar = dynamic_cast <CChar*> (pItem->GetParent());
	ASSERT( pChar != NULL );

	if ( ! m_pChar->CanSeeItem( pItem ) && m_pChar != pChar )
		return;

	LAYER_TYPE layer = pItem->GetEquipLayer();
	HUE_TYPE wHue;
	ITEMID_TYPE id;
	GetAdjustedItemID( pChar, pItem, id, wHue );

	CCommand cmd;
	cmd.ItemEquip.m_Cmd = XCMD_ItemEquip;
	cmd.ItemEquip.m_UID = pItem->GetUID();
	cmd.ItemEquip.m_id = ( layer == LAYER_BANKBOX ) ? ITEMID_CHEST_SILVER : id;
	cmd.ItemEquip.m_zero7 = 0;
	cmd.ItemEquip.m_layer = layer;
	cmd.ItemEquip.m_UIDchar = pChar->GetUID();
	cmd.ItemEquip.m_wHue = wHue;

	xSendPkt( &cmd, sizeof( cmd.ItemEquip ));

	addAOSTooltip( pItem );
}

void CClient::addItem_InContainer( const CItem * pItem )
{
	ADDTOCALLSTACK("CClient::addItem_InContainer");
	ASSERT(pItem);
	CItemContainer * pCont = dynamic_cast <CItemContainer*> (pItem->GetParent());
	if ( pCont == NULL )
		return;

	CItemBase * pItemDef = pItem->Item_GetDef();
	CPointBase pt = pItem->GetContainedPoint();

	// Add a single item in a container.
	CCommand cmd;
	cmd.ContAdd.m_Cmd = XCMD_ContAdd;
	cmd.ContAdd.m_UID = pItem->GetUID();

	ITEMID_TYPE id = pItem->GetDispID();
	if ( pItemDef && ( GetResDisp() < pItemDef->GetResLevel() ) )
		id = (ITEMID_TYPE) pItemDef->GetResDispDnId();

	cmd.ContAdd.m_id = id;

	cmd.ContAdd.m_zero7 = 0;
	cmd.ContAdd.m_amount = pItem->GetAmount();
	cmd.ContAdd.m_x = pt.m_x;
	cmd.ContAdd.m_y = pt.m_y;
	cmd.ContAdd.m_UIDCont = pCont->GetUID();

	HUE_TYPE wHue;
	if ( m_pChar->IsStatFlag( STATF_Hallucinating ))
	{
		wHue = Calc_GetRandVal( HUE_DYE_HIGH );	// restrict colors
	}
	else
	{
		wHue = pItem->GetHue() & HUE_MASK_HI;

		if ( pItemDef && ( GetResDisp() < pItemDef->GetResLevel() ) )
			if ( pItemDef->GetResDispDnHue() != HUE_DEFAULT )
				wHue = pItemDef->GetResDispDnHue() & HUE_MASK_HI;

		if ( wHue > HUE_QTY )
			wHue &= HUE_MASK_LO;
	}
	cmd.ContAdd.m_wHue = wHue;

	xSendPkt( &cmd, sizeof( cmd.ContAdd ));

	addAOSTooltip( pItem );
}

void CClient::addItem( CItem * pItem )
{
	ADDTOCALLSTACK("CClient::addItem");
	if ( pItem == NULL )
		return;
	if ( pItem->IsTopLevel())
	{
		addItem_OnGround( pItem );
	}
	else if ( pItem->IsItemEquipped())
	{
		addItem_Equipped( pItem );
	}
	else if ( pItem->IsItemInContainer())
	{
		addItem_InContainer( pItem );
	}
}

int CClient::addContents( const CItemContainer * pContainer, bool fCorpseEquip, bool fCorpseFilter, bool fShop) // Send Backpack (with items)
{
	ADDTOCALLSTACK("CClient::addContents");
	// NOTE: We needed to send the header for this FIRST !!!
	// 1 = equip a corpse
	// 0 = contents.

	bool fLayer[LAYER_HORSE];
	memset(fLayer, 0, sizeof(fLayer));

	CCommand cmd;
	CItemBase * pItemDef = NULL;

	// send all the items in the container.
	int count = 0;

//	int tContMaxI;
//	CVarDefCont * pTagTmp = pContainer->GetKey("OVERRIDE.MAXITEMS", false);
//	if ( pTagTmp )
//	{
//		tContMaxI = pTagTmp->GetValNum();
//	} else {
//		tContMaxI = MAX_ITEMS_CONT;
//	}

	for ( CItem * pItem = pContainer->GetContentHead(); pItem ; pItem = pItem->GetNext(), pItemDef = NULL )
	{
//		if ( count >= tContMaxI )
		if (count >= MAX_ITEMS_CONTENT)
		{
			g_Log.EventWarn("Too many items in container '%s' uid=0%x items=%d\n", pContainer->GetName(), (DWORD)pContainer->GetUID(), pContainer->GetCount());
			break;
		}

		LAYER_TYPE layer;

		if ( fCorpseFilter )	// dressing a corpse is different from opening the coffin!
		{
			layer = (LAYER_TYPE) pItem->GetContainedLayer();
			ASSERT( layer < LAYER_HORSE );
			switch ( layer )	// don't put these on a corpse.
			{
				case LAYER_NONE:
				case LAYER_PACK:	// these display strange.
					continue;
				case LAYER_NEWLIGHT:
					continue;
			}
			// Make certain that no more than one of each layer goes out to client....crashes otherwise!!
			if ( fLayer[layer] )
				continue;
			fLayer[layer] = true;
		}

		if ( fCorpseEquip )	// list equipped items on a corpse
		{
			ASSERT( fCorpseFilter );
			cmd.CorpEquip.m_item[count].m_layer	= pItem->GetContainedLayer();
			cmd.CorpEquip.m_item[count].m_UID	= pItem->GetUID();
		}
		else	// Content items
		{
			if ( !fShop && pItem->IsAttr(ATTR_INVIS) && !CanSee(pItem) ) // don't send invis items
				continue;

			pItemDef = pItem->Item_GetDef();

			cmd.Content.m_item[count].m_UID		= pItem->GetUID();
			cmd.Content.m_item[count].m_id		= pItem->GetDispID();
			if ( pItemDef && ( GetResDisp() < pItemDef->GetResLevel() ) )
				cmd.Content.m_item[count].m_id = (ITEMID_TYPE) pItemDef->GetResDispDnId();

			cmd.Content.m_item[count].m_zero6	= 0;
			cmd.Content.m_item[count].m_amount	= pItem->GetAmount();
			if ( fShop )
			{
				CItemVendable * pVendItem = dynamic_cast <CItemVendable *> (pItem);
				if ( !pVendItem || !pVendItem->GetAmount() || pVendItem->IsType(IT_GOLD) )
					continue;

				cmd.Content.m_item[count].m_amount = minimum(g_Cfg.m_iVendorMaxSell, cmd.Content.m_item[count].m_amount);
				cmd.Content.m_item[count].m_x	= (count + 1);
				cmd.Content.m_item[count].m_y	= 1; // count;
			}
			else
			{
				CPointBase pt = pItem->GetContainedPoint();
				cmd.Content.m_item[count].m_x	= pt.m_x;
				cmd.Content.m_item[count].m_y	= pt.m_y;
			}
			cmd.Content.m_item[count].m_UIDCont	= pContainer->GetUID();

			HUE_TYPE wHue;
			if ( m_pChar->IsStatFlag( STATF_Hallucinating ))
			{
				wHue = Calc_GetRandVal( HUE_DYE_HIGH );
			}
			else
			{
				wHue = pItem->GetHue() & HUE_MASK_HI;

				if ( pItemDef && ( GetResDisp() < pItemDef->GetResLevel() ) )
					if ( pItemDef->GetResDispDnHue() != HUE_DEFAULT )
						wHue = pItemDef->GetResDispDnHue() & HUE_MASK_HI;

				if ( wHue > HUE_QTY )
					wHue &= HUE_MASK_LO;	// restrict colors
			}
			cmd.Content.m_item[count].m_wHue = wHue;
		}
		count ++;

		addAOSTooltip(pItem, fShop);
	}

	if ( !count )
		return 0;

	int len;
	if ( fCorpseEquip )
	{
		cmd.CorpEquip.m_item[count].m_layer = LAYER_NONE;	// terminator.
		len = sizeof( cmd.CorpEquip ) - sizeof(cmd.CorpEquip.m_item) + ( count * sizeof(cmd.CorpEquip.m_item[0])) + 1;
		cmd.CorpEquip.m_Cmd = XCMD_CorpEquip;
		cmd.CorpEquip.m_len = len;
		cmd.CorpEquip.m_UID = pContainer->GetUID();
	}
	else
	{
		len = sizeof( cmd.Content ) - sizeof(cmd.Content.m_item) + ( count * sizeof(cmd.Content.m_item[0]));
		cmd.Content.m_Cmd = XCMD_Content;
		cmd.Content.m_len = len;
		cmd.Content.m_count = count;
	}

	xSendPkt(&cmd, len);
	return count;
}



void CClient::addOpenGump( const CObjBase * pContainer, GUMP_TYPE gump )
{
	ADDTOCALLSTACK("CClient::addOpenGump");
	// NOTE: if pContainer has not already been sent to the client
	//  this will crash client.
	CCommand cmd;
	cmd.ContOpen.m_Cmd = XCMD_ContOpen;
	cmd.ContOpen.m_UID = pContainer->GetUID();
	cmd.ContOpen.m_gump = gump;

	// we automatically get open sound for this,.
	xSendPkt( &cmd, sizeof( cmd.ContOpen ));
}

bool CClient::addContainerSetup( const CItemContainer * pContainer ) // Send Backpack (with items)
{
	ADDTOCALLSTACK("CClient::addContainerSetup");
	ASSERT(pContainer);
	ASSERT( pContainer->IsItem());

	// open the container with the proper GUMP.
	CItemBase * pItemDef = pContainer->Item_GetDef();
	GUMP_TYPE gump = pItemDef->IsTypeContainer();
	if ( gump <= GUMP_RESERVED )
	{
		return false;
	}

	addOpenGump(pContainer, gump);
	addContents(pContainer, false, false, false);
	CObjBaseTemplate * pTopMostContainer = pContainer->GetTopLevelObj();

	m_openedContainers[pContainer->GetUID().GetPrivateUID()] = std::make_pair(
																	std::make_pair( pContainer->GetContainer()->GetUID().GetPrivateUID(), pTopMostContainer->GetUID().GetPrivateUID() ),
																	pTopMostContainer->GetTopPoint());
	return true;
}

void CClient::addSeason(SEASON_TYPE season)
{
	ADDTOCALLSTACK("CClient::addSeason");
	if ( m_pChar->IsStatFlag(STATF_DEAD) )		// everything looks like this when dead.
		season = SEASON_Desolate;
	if ( season == m_Env.m_Season )	// the season i saw last.
		return;

	m_Env.m_Season = season;

	CCommand cmd;
	cmd.Season.m_Cmd = XCMD_Season;
	cmd.Season.m_season = season;
	cmd.Season.m_cursor = 1;
	xSendPkt(&cmd, sizeof(cmd.Season));
}

void CClient::addWeather( WEATHER_TYPE weather ) // Send new weather to player
{
	ADDTOCALLSTACK("CClient::addWeather");
	ASSERT( m_pChar );

	if ( g_Cfg.m_fNoWeather )
		return;

	if ( m_pChar->IsStatFlag( STATF_InDoors ))
	{
		// If there is a roof over our head at the moment then stop rain.
		weather = WEATHER_DRY;
	}
	else if ( weather == WEATHER_DEFAULT )
	{
		weather = m_pChar->GetTopSector()->GetWeather();
	}

	if ( weather == m_Env.m_Weather )
		return;

	CCommand cmd;
	cmd.Weather.m_Cmd = XCMD_Weather;
	cmd.Weather.m_type = weather;
	cmd.Weather.m_ext1 = 0x40;
	cmd.Weather.m_unk010 = 0x10;

	// m_ext = seems to control a very gradual fade in and out ?
	switch ( weather )
	{
		case WEATHER_RAIN:	// rain
		case WEATHER_STORM:
		case WEATHER_SNOW:	// snow
			// fix weird client transition problem.
			// May only transition from Dry.
			addWeather( WEATHER_DRY );
			break;
		default:	// dry or cloudy
			cmd.Weather.m_type = WEATHER_DRY;
			cmd.Weather.m_ext1 = 0;
			break;
	}

	m_Env.m_Weather = weather;
	xSendPkt( &cmd, sizeof(cmd.Weather));
}

void CClient::addLight( int iLight )
{
	ADDTOCALLSTACK("CClient::addLight");
	// NOTE: This could just be a flash of light.
	// Global light level.
	ASSERT(m_pChar);

	if (m_pChar->m_LocalLight)
		iLight = m_pChar->m_LocalLight;

	if ( iLight < LIGHT_BRIGHT )
	{
		iLight = m_pChar->GetLightLevel();
	}

	// Scale light level for non-t2a.
	if ( iLight < LIGHT_BRIGHT )
		iLight = LIGHT_BRIGHT;
	if ( iLight > LIGHT_DARK )
		iLight = LIGHT_DARK;

	if ( iLight == m_Env.m_Light )
		return;
	m_Env.m_Light = iLight;

	CCommand cmd;
	cmd.Light.m_Cmd = XCMD_Light;
	cmd.Light.m_level = iLight;
	xSendPkt( &cmd, sizeof( cmd.Light ));
}

void CClient::addArrowQuest( int x, int y )
{
	ADDTOCALLSTACK("CClient::addArrowQuest");
	CCommand cmd;
	cmd.Arrow.m_Cmd = XCMD_Arrow;
	cmd.Arrow.m_Active = ( x && y ) ? 1 : 0;	// 1/0
	cmd.Arrow.m_x = x;
	cmd.Arrow.m_y = y;
	xSendPkt( &cmd, sizeof( cmd.Arrow ));
}

void CClient::addMusic( WORD id )
{
	ADDTOCALLSTACK("CClient::addMusic");
	// Music is ussually appropriate for the region.
	CCommand cmd;
	cmd.PlayMusic.m_Cmd = XCMD_PlayMusic;
	cmd.PlayMusic.m_musicid = id;
	xSendPkt( &cmd, sizeof( cmd.PlayMusic ));
}

bool CClient::addKick( CTextConsole * pSrc, bool fBlock )
{
	ADDTOCALLSTACK("CClient::addKick");
	// Kick me out.
	ASSERT( pSrc );
	if ( GetAccount() == NULL )
	{
		m_fClosed	= true;
		return( true );
	}

	if ( ! GetAccount()->Kick( pSrc, fBlock ))
		return( false );

	LPCTSTR pszAction = fBlock ? "KICK" : "DISCONNECT";
	SysMessagef( "You have been %sed by '%s'", (LPCTSTR) pszAction, (LPCTSTR) pSrc->GetName());

	if ( IsConnectTypePacket() )
	{
		CCommand cmd;
		cmd.Kick.m_Cmd = XCMD_Kick;
		cmd.Kick.m_unk1 = 0;	// The kickers uid ?
		xSendPkt( &cmd, sizeof( cmd.Kick ));
	}

	m_fClosed	= true;
	return( true );
}

void CClient::addSound( SOUND_TYPE id, const CObjBaseTemplate * pBase, int iOnce )
{
	ADDTOCALLSTACK("CClient::addSound");
	if ( !g_Cfg.m_fGenericSounds )
		return;

	CPointMap pt;
	if ( pBase )
	{
		pBase = pBase->GetTopLevelObj();
		pt = pBase->GetTopPoint();
	}
	else
		pt = m_pChar->GetTopPoint();

	if (( id > 0 ) && !iOnce && !pBase )
		return;

	CCommand cmd;
	cmd.Sound.m_Cmd = XCMD_Sound;
	cmd.Sound.m_flags = iOnce;
	cmd.Sound.m_id = id;
	cmd.Sound.m_volume = 0;
	cmd.Sound.m_x = pt.m_x;
	cmd.Sound.m_y = pt.m_y;
	cmd.Sound.m_z = pt.m_z;

	xSendPkt( &cmd, sizeof(cmd.Sound));
}

void CClient::addItemDragCancel( BYTE type )
{
	ADDTOCALLSTACK("CClient::addItemDragCancel");
	CCommand cmd;
	cmd.DragCancel.m_Cmd = XCMD_DragCancel;
	cmd.DragCancel.m_type = type;
	xSendPkt( &cmd, sizeof( cmd.DragCancel ));
}

void CClient::addBarkUNICODE( const NCHAR * pwText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang )
{
	ADDTOCALLSTACK("CClient::addBarkUNICODE");
	if ( pwText == NULL )
		return;

	if ( ! IsConnectTypePacket())
	{
		// Need to convert back from unicode !
		//SysMessage(pwText);
		return;
	}

	if ( mode == TALKMODE_BROADCAST )
	{
		mode = TALKMODE_SYSTEM;
		pSrc = NULL;
	}
	CCommand cmd;
	cmd.SpeakUNICODE.m_Cmd = XCMD_SpeakUNICODE;

	// string copy unicode (til null)
	int i=0;
	for ( ; pwText[i] && i < MAX_TALK_BUFFER; i++ )
	{
		cmd.SpeakUNICODE.m_utext[i] = pwText[i];
	}
	cmd.SpeakUNICODE.m_utext[i++] = '\0';	// add for the null

	int len = sizeof(cmd.SpeakUNICODE) + (i*sizeof(NCHAR));
	cmd.SpeakUNICODE.m_len = len;
	cmd.SpeakUNICODE.m_mode = mode;		// mode = range.
	cmd.SpeakUNICODE.m_wHue = wHue;
	cmd.SpeakUNICODE.m_font = font;		// font. 3 = system message just to you !

	lang.GetStrDef(cmd.SpeakUNICODE.m_lang);

	int iNameLen;
	if ( pSrc == NULL )
	{
		cmd.SpeakUNICODE.m_UID = 0xFFFFFFFF;	// 0x01010101;
		iNameLen = strcpylen( cmd.SpeakUNICODE.m_charname, "System" );
	}
	else
	{
		cmd.SpeakUNICODE.m_UID = pSrc->GetUID();
		iNameLen = strcpylen( cmd.SpeakUNICODE.m_charname, pSrc->GetName(), sizeof(cmd.SpeakUNICODE.m_charname) );
	}
	memset( cmd.SpeakUNICODE.m_charname+iNameLen, 0, sizeof(cmd.SpeakUNICODE.m_charname)-iNameLen );

	if ( pSrc == NULL || pSrc->IsItem())
	{
		cmd.SpeakUNICODE.m_id = 0xFFFF;	// 0x0101;
	}
	else	// char id only.
	{
		const CChar * pChar = dynamic_cast <const CChar*>(pSrc);
		ASSERT(pChar);
		cmd.SpeakUNICODE.m_id = pChar->GetDispID();
	}
	xSendPkt( &cmd, len );
}

void CClient::addBarkLocalized( int iClilocId, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, TCHAR * pArgs )
{
	ADDTOCALLSTACK("CClient::addBarkLocalized");
	if ( iClilocId <= 0 )
		return;

	if ( !IsConnectTypePacket() )
		return;

	if ( mode == TALKMODE_BROADCAST )
	{
		mode = TALKMODE_SYSTEM;
		pSrc = NULL;
	}

	CCommand cmd;
	cmd.SpeakLocalized.m_Cmd = XCMD_SpeakLocalized;
	cmd.SpeakLocalized.m_mode = mode;
	cmd.SpeakLocalized.m_wHue = wHue;
	cmd.SpeakLocalized.m_font = font;

	int iNameLen;
	if ( pSrc == NULL )
	{
		cmd.SpeakLocalized.m_UID = 0xFFFFFFFF;
		iNameLen = strcpylen( cmd.SpeakLocalized.m_charname, "System" );
	}
	else
	{
		cmd.SpeakLocalized.m_UID = pSrc->GetUID();
		iNameLen = strcpylen( cmd.SpeakLocalized.m_charname, pSrc->GetName(), sizeof(cmd.SpeakLocalized.m_charname) );
	}
	memset( cmd.SpeakLocalized.m_charname+iNameLen, 0, sizeof(cmd.SpeakLocalized.m_charname)-iNameLen );

	if ( pSrc == NULL || pSrc->IsItem())
	{
		cmd.SpeakLocalized.m_id = 0xFFFF; // 0x0101;
	}
	else	// char id only.
	{
		const CChar * pChar = dynamic_cast <const CChar*>(pSrc);
		ASSERT(pChar);
		cmd.SpeakLocalized.m_id = pChar->GetDispID();
	}

	cmd.SpeakLocalized.m_clilocId = iClilocId;

	TCHAR * pArg = pArgs;
	int i = 0;
	int len = sizeof(cmd.SpeakLocalized) - 1;
	if ( pArg )
	{
		while ( *pArg )
		{
			cmd.SpeakLocalized.m_args[i++] = *pArg;
			cmd.SpeakLocalized.m_args[i++] = 0;
			pArg++;
			len += 2;
		}
	}
	cmd.SpeakLocalized.m_args[i++] = '\0';
	cmd.SpeakLocalized.m_args[i++] = '\0';
	len += 2;
	cmd.SpeakLocalized.m_len = len;
	xSendPkt( &cmd, len );
}

void CClient::addBarkParse( LPCTSTR pszText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, LPCTSTR name)
{
	ADDTOCALLSTACK("CClient::addBarkParse");
	if ( !pszText )
		return;

	WORD Args[] = { wHue, font, 0 };

	if ( *pszText == '@' )
	{
		pszText++;
		if ( *pszText == '@' ) // @@ = just a @ symbol
			goto bark_default;

		const char *s	= pszText;
		pszText		= strchr( s, ' ' );

		if ( !pszText )
			return;

		for ( int i = 0; ( s < pszText ) && ( i < 3 ); )
		{
			if ( *s == ',' ) // default value, skip it
			{
				i++;
				s++;
				continue;
			}
			Args[i] = Exp_GetVal( s );
			i++;

			if ( *s == ',' )
				s++;
			else
				break;	// no more args here!
		}
		pszText++;
		if ( Args[1] > FONT_QTY )
			Args[1]	= FONT_NORMAL;
	}


	if ( m_BarkBuffer.IsEmpty())
	{
		m_BarkBuffer.Format( "%s%s", (LPCTSTR) name, (LPCTSTR) pszText );
	}

	switch ( Args[2] )
	{
		case 2:	// Localized
		{
            TCHAR * ppArgs[256];
			int iQty = Str_ParseCmds( (TCHAR *)m_BarkBuffer.GetPtr(), ppArgs, COUNTOF(ppArgs), "," );
			int iClilocId = Exp_GetVal( ppArgs[0] );
			CGString CArgs;
			for ( int i = 1; i < iQty; i++ )
			{
				if ( CArgs.GetLength() )
					CArgs += "\t";
				CArgs += ( !strcmp(ppArgs[i], "NULL") ? " " : ppArgs[i] );
			}

			addBarkLocalized( iClilocId, pSrc, (HUE_TYPE) Args[0], mode, (FONT_TYPE) Args[1], (TCHAR *)CArgs.GetPtr());
			break;
		}

		case 1:	// Unicode
		{
			NCHAR szBuffer[ MAX_TALK_BUFFER ];
			int iLen = CvtSystemToNUNICODE( szBuffer, COUNTOF(szBuffer), m_BarkBuffer.GetPtr(), -1 );
			addBarkUNICODE( szBuffer, pSrc, (HUE_TYPE) Args[0], mode, (FONT_TYPE) Args[1], 0 );
			break;
		}

		case 0:	// Ascii
		default:
		{
bark_default:
			if ( m_BarkBuffer.IsEmpty())
			{
				m_BarkBuffer.Format( "%s%s", (LPCTSTR) name, (LPCTSTR) pszText );
			}

			addBark( m_BarkBuffer.GetPtr(), pSrc, (HUE_TYPE) Args[0], mode, (FONT_TYPE) Args[1] );
			break;
		}
	}

	// Empty the buffer.
	m_BarkBuffer.Empty();
}



void CClient::addBark( LPCTSTR pszText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font )
{
	ADDTOCALLSTACK("CClient::addBark");
	if ( pszText == NULL )
		return;

	if ( ! IsConnectTypePacket())
	{
		SysMessage(pszText);
		return;
	}

	if ( mode == TALKMODE_BROADCAST )
	{
		mode = TALKMODE_SYSTEM;
		pSrc = NULL;
	}

	CCommand cmd;
	cmd.Speak.m_Cmd = XCMD_Speak;
	int len = strlen(pszText);
	len += sizeof(cmd.Speak);
	cmd.Speak.m_len = len;
	cmd.Speak.m_mode = mode;		// mode = range.
	cmd.Speak.m_wHue = wHue;
	cmd.Speak.m_font = font;		// font. 3 = system message just to you !

	int iNameLen;
	if ( pSrc == NULL )
	{
		cmd.Speak.m_UID = 0xFFFFFFFF;
		iNameLen = strcpylen( cmd.Speak.m_charname, "System" );
	}
	else
	{
		cmd.Speak.m_UID = pSrc->GetUID();
		iNameLen = strcpylen( cmd.Speak.m_charname, pSrc->GetName(), sizeof(cmd.Speak.m_charname) );
	}
	memset( cmd.Speak.m_charname+iNameLen, 0, sizeof(cmd.Speak.m_charname)-iNameLen );

	if ( pSrc == NULL || pSrc->IsItem())
	{
		cmd.Speak.m_id = 0xFFFF; // 0x0101;
	}
	else	// char id only.
	{
		const CChar * pChar = dynamic_cast <const CChar*>(pSrc);
		ASSERT(pChar);
		cmd.Speak.m_id = pChar->GetDispID();
	}
	strcpylen( cmd.Speak.m_text, pszText, MAX_TALK_BUFFER );
	xSendPkt( &cmd, len );
}

void CClient::addObjMessage( LPCTSTR pMsg, const CObjBaseTemplate * pSrc, HUE_TYPE wHue ) // The message when an item is clicked
{
	ADDTOCALLSTACK("CClient::addObjMessage");
	if ( !pMsg )
		return;

	if ( IsSetOF(OF_Flood_Protection) && ( GetPrivLevel() <= PLEVEL_Player )  )
	{
		if ( !strcmpi(pMsg, m_zLastObjMessage) )
			return;

		if ( strlen(pMsg) < SCRIPT_MAX_LINE_LEN )
			strcpy(m_zLastObjMessage, pMsg);
	}

	addBarkParse(pMsg, pSrc, wHue, ( pSrc == m_pChar ) ? TALKMODE_OBJ : TALKMODE_ITEM, FONT_NORMAL);
}

void CClient::addEffect( EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate * pDst, const CObjBaseTemplate * pSrc, BYTE bSpeedSeconds, BYTE bLoop, bool fExplode, DWORD color, DWORD render )
{
	ADDTOCALLSTACK("CClient::addEffect");
	// bSpeedSeconds = seconds = 0=very fast, 7=slow.
	// wHue =

	ASSERT(m_pChar);
	ASSERT( pDst );
	pDst = pDst->GetTopLevelObj();
	CPointMap ptDst = pDst->GetTopPoint();

	CCommand cmd;
	if ( color || render )
		cmd.Effect.m_Cmd = XCMD_EffectEx;
	else
		cmd.Effect.m_Cmd = XCMD_Effect;
	cmd.Effect.m_motion = motion;
	cmd.Effect.m_id = id;
	cmd.Effect.m_UID = pDst->GetUID();

	CPointMap ptSrc;
	if ( pSrc != NULL && motion == EFFECT_BOLT )
	{
		pSrc = pSrc->GetTopLevelObj();
		ptSrc = pSrc->GetTopPoint();
	}
	else
	{
		ptSrc = ptDst;
	}

	cmd.Effect.m_speed = bSpeedSeconds;		// 22= 0=very fast, 7=slow.
	cmd.Effect.m_loop = bLoop;		// 23= 0 is really long.  1 is the shortest., 6 = longer
	cmd.Effect.m_unk = 	0;		// 0x300
	cmd.Effect.m_OneDir = true;		// 26=1=point in single dir else turn.
	cmd.Effect.m_explode = fExplode;	// 27=effects that explode on impact.

	cmd.Effect.m_srcx = ptSrc.m_x;
	cmd.Effect.m_srcy = ptSrc.m_y;
	cmd.Effect.m_srcz = ptSrc.m_z;
	cmd.Effect.m_dstx = ptDst.m_x;
	cmd.Effect.m_dsty = ptDst.m_y;
	cmd.Effect.m_dstz = ptDst.m_z;

	cmd.Effect.m_hue	= color;
	cmd.Effect.m_render	= render;

	switch ( motion )
	{
		case EFFECT_BOLT:	// a targetted bolt
			if ( ! pSrc )
				return;
			cmd.Effect.m_targUID = pDst->GetUID();
			cmd.Effect.m_UID = pSrc->GetUID();	// source
			cmd.Effect.m_OneDir = false;
			cmd.Effect.m_loop = 0;	// Does not apply.
			break;

		case EFFECT_LIGHTNING:	// lightning bolt.
			break;

		case EFFECT_XYZ:	// Stay at current xyz ??? not sure about this.
			break;

		case EFFECT_OBJ:	// effect at single Object.
			break;
	}

	if ( cmd.Effect.m_Cmd == XCMD_EffectEx )
		xSendPkt( &cmd,  sizeof(cmd.Effect)  );
	else
		xSendPkt( &cmd,  sizeof(cmd.Effect) - 8 );
}


void CClient::GetAdjustedItemID( const CChar * pChar, const CItem * pItem, ITEMID_TYPE & id, HUE_TYPE & wHue )
{
	ADDTOCALLSTACK("CClient::GetAdjustedItemID");
	// An equipped item.
	ASSERT( pChar );

	id = pItem->GetDispID();
	CItemBase * pItemDef = pItem->Item_GetDef();

	if ( m_pChar->IsStatFlag( STATF_Hallucinating ))
	{
		wHue = Calc_GetRandVal( HUE_DYE_HIGH );
	}
	else if ( pChar->IsStatFlag(STATF_Stone))
	{
		wHue = HUE_STONE;
	}
	else
	{
		wHue = pItem->GetHue();

		if ( pItemDef && ( GetResDisp() < pItemDef->GetResLevel() ) )
			if ( pItemDef->GetResDispDnHue() != HUE_DEFAULT )
				wHue = pItemDef->GetResDispDnHue();

		if (( wHue & HUE_MASK_HI ) > HUE_QTY )
			wHue &= HUE_MASK_LO | HUE_TRANSLUCENT;
		else
			wHue &= HUE_MASK_HI | HUE_TRANSLUCENT;
	}

	if ( pItemDef && ( GetResDisp() < pItemDef->GetResLevel() ) )
		id = (ITEMID_TYPE) pItemDef->GetResDispDnId();
}

void CClient::GetAdjustedCharID( const CChar * pChar, CREID_TYPE & id, HUE_TYPE & wHue )
{
	ADDTOCALLSTACK("CClient::GetAdjustedCharID");
	// Some clients can't see all creature artwork and colors.
	// try to do the translation here,.

	ASSERT( GetAccount() );
	ASSERT( pChar );

	if ( IsPriv(PRIV_DEBUG))
	{
		id = CREID_MAN;
		wHue = 0;
		return;
	}

	id = pChar->GetDispID();
	CCharBase * pCharDef = pChar->Char_GetDef();

	if ( m_pChar->IsStatFlag( STATF_Hallucinating )) // viewer is Hallucinating.
	{
		if ( pChar != m_pChar )
		{
			// Get a random creature from the artwork.
			id = (CREID_TYPE) Calc_GetRandVal(CREID_EQUIP_GM_ROBE);
			pCharDef = CCharBase::FindCharBase( id );

			for ( int iTries = 0; iTries < CREID_EQUIP_GM_ROBE; iTries ++ )
			{
				if ( pCharDef && ( id != CREID_SEA_Creature ) )
					break;
				id = (CREID_TYPE) ( id + 1 );
				if ( id >= CREID_EQUIP_GM_ROBE )
					id = (CREID_TYPE) 1;
				pCharDef = CCharBase::FindCharBase( id );
			}
		}

		wHue = Calc_GetRandVal( HUE_DYE_HIGH );
	}
	else
	{
		if ( pChar->IsStatFlag(STATF_Stone))	// turned to stone.
		{
			wHue = HUE_STONE;
		}
		else
		{
			wHue = pChar->GetHue();
			if ( pCharDef && ( GetResDisp() < pCharDef->GetResLevel() ) )
				if ( pCharDef->GetResDispDnHue() != HUE_DEFAULT )
					wHue = pCharDef->GetResDispDnHue();

			// allow transparency and underwear colors
			if (( wHue & HUE_MASK_HI ) > HUE_QTY )
				wHue &= HUE_MASK_LO | HUE_UNDERWEAR | HUE_TRANSLUCENT;
			else
				wHue &= HUE_MASK_HI | HUE_UNDERWEAR | HUE_TRANSLUCENT;
		}
	}

	if ( pCharDef && ( GetResDisp() < pCharDef->GetResLevel() ) )
		id = (CREID_TYPE) pCharDef->GetResDispDnId();
}

void CClient::addCharMove( const CChar * pChar )
{
	ADDTOCALLSTACK("CClient::addCharMove");
	// This char has just moved on screen.
	// or changed in a subtle way like "hidden"
	// NOTE: If i have been turned this will NOT update myself.

	CCommand cmd;
	EXC_TRY("addCharMove");
	cmd.CharMove.m_Cmd = XCMD_CharMove;
	cmd.CharMove.m_UID = pChar->GetUID();

	CREID_TYPE id;
	HUE_TYPE wHue;
	EXC_SET("GetAdjustChar");
	GetAdjustedCharID( pChar, id, wHue );
	cmd.CharMove.m_id = id;
	cmd.CharMove.m_wHue = wHue;

	EXC_SET("GetTopPoint");
	CPointMap pt = pChar->GetTopPoint();
	cmd.CharMove.m_x  = pt.m_x;
	cmd.CharMove.m_y  = pt.m_y;
	cmd.CharMove.m_z = pt.m_z;
	EXC_SET("GetDirFlag");
	cmd.CharMove.m_dir = pChar->GetDirFlag();
	EXC_SET("GetModeFlag");
	cmd.CharMove.m_mode = pChar->GetModeFlag( pChar->CanSeeTrue( m_pChar ));
	EXC_SET("Noto_GetFlag");
	cmd.CharMove.m_noto = pChar->Noto_GetFlag( m_pChar, false, true );

	EXC_SET("xsendpkt");
	xSendPkt( &cmd, sizeof(cmd.CharMove));
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("m_dirFace (0%x)\n",pChar->m_dirFace);
	EXC_DEBUG_END;
}

void CClient::addChar( const CChar * pChar )
{
	ADDTOCALLSTACK("CClient::addChar");
	// Full update about a char.

	CCommand cmd;
	EXC_TRY("addChar");
	cmd.Char.m_Cmd = XCMD_Char;
	cmd.Char.m_UID = pChar->GetUID();

	CREID_TYPE id;
	HUE_TYPE wHue;
	EXC_SET("GetAdjustChar");
	GetAdjustedCharID( pChar, id, wHue );
	cmd.Char.m_id = id;
	cmd.Char.m_wHue = wHue;

	EXC_SET("GetTopPoint");
	CPointMap pt = pChar->GetTopPoint();
	EXC_SET("Wake sector");
	pt.GetSector()->SetSectorWakeStatus();	// if it can be seen then wake it.

	cmd.Char.m_x = pt.m_x;
	cmd.Char.m_y = pt.m_y;
	cmd.Char.m_z = pt.m_z;
	EXC_SET("GetDirFlag");
	cmd.Char.m_dir = pChar->GetDirFlag();
	EXC_SET("GetModeFlag");
	cmd.Char.m_mode = pChar->GetModeFlag( pChar->CanSeeTrue( m_pChar ));
	EXC_SET("Noto_GetFlag");
	cmd.Char.m_noto = pChar->Noto_GetFlag( m_pChar, false, true );

	int len = ( sizeof( cmd.Char ) - sizeof( cmd.Char.equip ));
	CCommand * pCmd = &cmd;

	EXC_SET("Array creating");
	bool fLayer[LAYER_HORSE+1];

	EXC_SET("Array clearing");
#ifndef _WIN32
	for ( int i_tmpN=0; i_tmpN < (LAYER_HORSE + 1); ++i_tmpN )
	{
		fLayer[i_tmpN] = 0;
	}
#else
	memset( fLayer, 0, sizeof(fLayer));
#endif

	if ( ! pChar->IsStatFlag( STATF_Sleeping ))
	{
		// extend the current struct for all the equipped items.
		EXC_SET("Getting CItem pointer");
		CItem* pItem=pChar->GetContentHead();
		EXC_SET("Starting loop");
		for ( ; pItem!=NULL; pItem=pItem->GetNext())
		{
			LAYER_TYPE layer = pItem->GetEquipLayer();
			if ( ! CItemBase::IsVisibleLayer( layer ))
				continue;
			if ( ! m_pChar->CanSeeItem( pItem ) && m_pChar != pChar )
				continue;

			EXC_SET("Array testing & setting");
			// Make certain that no more than one of each layer goes out to client....crashes otherwise!!
			if ( fLayer[layer] )
			{
				continue;
			}
			fLayer[layer] = true;
			EXC_SET("Tooltip adding");
			addAOSTooltip( pItem );

			pCmd->Char.equip[0].m_UID = pItem->GetUID();
			pCmd->Char.equip[0].m_layer = layer;

			HUE_TYPE wHue;
			ITEMID_TYPE itemid;
			EXC_SET("GetAdjustItem");
			GetAdjustedItemID( pChar, pItem, itemid, wHue );
			EXC_SET("Finishing the packet");
			if ( wHue )
			{
				pCmd->Char.equip[0].m_id = itemid | 0x8000;	// include wHue.
				pCmd->Char.equip[0].m_wHue = wHue;
				len += sizeof(pCmd->Char.equip[0]);
				pCmd = (CCommand*)(((BYTE*)pCmd)+sizeof(pCmd->Char.equip[0]));
			}
			else
			{
				pCmd->Char.equip[0].m_id = itemid;
				len += sizeof(pCmd->Char.equip[0]) - sizeof(pCmd->Char.equip[0].m_wHue);
				pCmd = (CCommand*)(((BYTE*)pCmd)+sizeof(pCmd->Char.equip[0])-sizeof(pCmd->Char.equip[0].m_wHue));
			}
		}
	}

	pCmd->Char.equip[0].m_UID = 0;	// terminator.
	len += sizeof( DWORD );

	cmd.Char.m_len = len;
	EXC_SET("Packet sending");
	xSendPkt( &cmd, len );

	EXC_SET("AOSToolTip adding (end)");
	addAOSTooltip( pChar );

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("m_dirFace (0%x)\n",pChar->m_dirFace);
	EXC_DEBUG_END;
}

void CClient::addItemName( const CItem * pItem )
{
	ADDTOCALLSTACK("CClient::addItemName");
	// NOTE: CanSee() has already been called.
	ASSERT(pItem);

	bool fIdentified = ( IsPriv(PRIV_GM) || pItem->IsAttr( ATTR_IDENTIFIED ));
	LPCTSTR pszNameFull = pItem->GetNameFull( fIdentified );

	TCHAR szName[ MAX_ITEM_NAME_SIZE + 256 ];
	int len = strcpylen( szName, pszNameFull, sizeof(szName) );

	const CContainer* pCont = dynamic_cast<const CContainer*>(pItem);
	if ( pCont != NULL )
	{
		// ??? Corpses show hair as an item !!
		len += sprintf( szName+len, " (%d items)", pCont->GetCount());
	}

	// obviously damaged ?
	else if ( pItem->IsTypeArmorWeapon())
	{
		int iPercent = pItem->Armor_GetRepairPercent();
		if ( iPercent < 50 &&
			( m_pChar->Skill_GetAdjusted( SKILL_ARMSLORE ) / 10 > iPercent ))
		{
			len += sprintf( szName+len, " (%s)", pItem->Armor_GetRepairDesc());
		}
	}

	// Show the priced value
	CItemContainer * pMyCont = dynamic_cast <CItemContainer *>( pItem->GetParent());
	if ( pMyCont != NULL && pMyCont->IsType(IT_EQ_VENDOR_BOX))
	{
		const CItemVendable * pVendItem = dynamic_cast <const CItemVendable *> (pItem);
		if ( pVendItem )
		{
			len += sprintf( szName+len, " (%d gp)",	pVendItem->GetBasePrice());
		}
	}

	HUE_TYPE wHue = HUE_TEXT_DEF;
	const CItemCorpse * pCorpseItem = dynamic_cast <const CItemCorpse *>(pItem);
	if ( pCorpseItem )
	{
		CChar * pCharCorpse = pCorpseItem->m_uidLink.CharFind();
		if ( pCharCorpse )
		{
			wHue = pCharCorpse->Noto_GetHue( m_pChar, true );
		}
	}

	if ( IsPriv( PRIV_GM ))
	{
		if ( pItem->IsAttr(ATTR_INVIS ))
		{
			len += strcpylen( szName+len, " (invis)" );
		}
		// Show the restock count
		if ( pMyCont != NULL && pMyCont->IsType(IT_EQ_VENDOR_BOX) )
		{
			len += sprintf( szName+len, " (%d restock)", pItem->GetContainedLayer());
		}
		switch ( pItem->GetType() )
		{
		case IT_SPAWN_CHAR:
		case IT_SPAWN_ITEM:
			{
				len += pItem->Spawn_GetName( szName + len );
			}
			break;

		case IT_TREE:
		case IT_GRASS:
		case IT_ROCK:
		case IT_WATER:
			{
			const CResourceDef * pResDef = g_Cfg.ResourceGetDef( pItem->m_itResource.m_rid_res );
			if ( pResDef)
			{
				len += sprintf( szName+len, " (%s)", pResDef->GetName());
			}
			}
			break;
		}
	}
	if ( IsPriv(PRIV_DEBUG) )
		len += sprintf(szName+len, " [0%lx]", (DWORD) pItem->GetUID());

	addObjMessage(szName, pItem, wHue);
}

void CClient::addCharName( const CChar * pChar ) // Singleclick text for a character
{
	ADDTOCALLSTACK("CClient::addCharName");
	// Karma wHue codes ?
	ASSERT( pChar );

	HUE_TYPE wHue	= pChar->Noto_GetHue( m_pChar, true );

	TCHAR *pszTemp = Str_GetTemp();
	LPCTSTR prefix = pChar->GetKeyStr( "NAME.PREFIX" );
	if ( ! *prefix )
		prefix = pChar->Noto_GetFameTitle();

	strcpy( pszTemp, prefix );
	strcat( pszTemp, pChar->GetName() );
	strcat( pszTemp, pChar->GetKeyStr( "NAME.SUFFIX" ) );

	if ( !pChar->IsStatFlag(STATF_Incognito) || ( GetPrivLevel() > pChar->GetPrivLevel() ))
	{
		// Guild abbrev.
		LPCTSTR pAbbrev = pChar->Guild_AbbrevBracket(MEMORY_TOWN);
		if ( pAbbrev )
		{
			strcat( pszTemp, pAbbrev );
		}
		pAbbrev = pChar->Guild_AbbrevBracket(MEMORY_GUILD);
		if ( pAbbrev )
		{
			strcat( pszTemp, pAbbrev );
		}
	}
	else
		strcpy( pszTemp, pChar->GetName() );

	bool fAllShow = IsPriv(PRIV_DEBUG|PRIV_ALLSHOW);

	if ( g_Cfg.m_fCharTags || fAllShow )
	{
		if ( pChar->m_pArea && pChar->m_pArea->IsGuarded() && pChar->m_pNPC )
		{
			if ( pChar->IsStatFlag( STATF_Pet ))
				strcat( pszTemp, " [tame]" );
			else
				strcat( pszTemp, " [npc]" );
		}
		if ( pChar->IsStatFlag( STATF_INVUL ) && ! pChar->IsStatFlag( STATF_Incognito ) && ! pChar->IsPriv( PRIV_PRIV_NOSHOW ))
			strcat( pszTemp, " [invul]" );
		if ( pChar->IsStatFlag( STATF_Stone ))
			strcat( pszTemp, " [stone]" );
		else if ( pChar->IsStatFlag( STATF_Freeze ))
			strcat( pszTemp, " [frozen]" );
		if ( pChar->IsStatFlag( STATF_Insubstantial | STATF_Invisible | STATF_Hidden ))
			strcat( pszTemp, " [hidden]" );
		if ( pChar->IsStatFlag( STATF_Sleeping ))
			strcat( pszTemp, " [sleeping]" );
		if ( pChar->IsStatFlag( STATF_Hallucinating ))
			strcat( pszTemp, " [hallu]" );

		if ( fAllShow )
		{
			if ( pChar->IsStatFlag(STATF_Spawned) )
				strcat(pszTemp, " [spawn]");
			if ( IsPriv( PRIV_DEBUG ))
				sprintf(pszTemp+strlen(pszTemp), " [0%lx]", (DWORD) pChar->GetUID());
		}
	}
	if ( ! fAllShow && pChar->Skill_GetActive() == NPCACT_Napping )
	{
		strcat( pszTemp, " [afk]" );
	}
	if ( pChar->GetPrivLevel() <= PLEVEL_Guest )
	{
		strcat( pszTemp, " [guest]" );
	}
	if ( pChar->IsPriv( PRIV_JAILED ))
	{
		strcat( pszTemp, " [jailed]" );
	}
	if ( pChar->IsDisconnected())
	{
		strcat( pszTemp, " [logout]" );
	}
	if (( fAllShow || pChar == m_pChar ) && pChar->IsStatFlag( STATF_Criminal ))
	{
		strcat( pszTemp, " [criminal]" );
	}
	if ( fAllShow || ( IsPriv(PRIV_GM) && ( g_Cfg.m_wDebugFlags & DEBUGF_NPC_EMOTE )))
	{
		strcat( pszTemp, " [" );
		strcat( pszTemp, pChar->Skill_GetName());
		strcat( pszTemp, "]" );
	}

	addObjMessage( pszTemp, pChar, wHue );
}

void CClient::addPlayerWalkCancel()
{
	ADDTOCALLSTACK("CClient::addPlayerWalkCancel");
	addPlayerWalkCancel(m_pChar->GetTopPoint(),m_pChar->GetDirFlag());
}

void CClient::addPlayerWalkCancel(const CPointMap & pt, BYTE bDir)
{
	ADDTOCALLSTACK("CClient::addPlayerWalkCancel");
	// Resync CChar client back to a previous move.
	CCommand cmd;
	cmd.WalkCancel.m_Cmd = XCMD_WalkCancel;
	cmd.WalkCancel.m_count = m_wWalkCount;	// sequence #

	cmd.WalkCancel.m_x = pt.m_x;
	cmd.WalkCancel.m_y = pt.m_y;
	cmd.WalkCancel.m_dir = bDir;
	cmd.WalkCancel.m_z = pt.m_z;
	xSendPkt( &cmd, sizeof( cmd.WalkCancel ));
	m_wWalkCount = -1;
}

void CClient::addPlayerStart( CChar * pChar )
{
	ADDTOCALLSTACK("CClient::addPlayerStart");
	if ( m_pChar != pChar )	// death option just usese this as a reload.
	{
		// This must be a CONTROL command ?
		CharDisconnect();
		if ( pChar->IsClient())	// not sure why this would happen but take care of it anyhow.
		{
			pChar->GetClient()->CharDisconnect();
			ASSERT(!pChar->IsClient());
		}
		m_pChar = pChar;
		m_pChar->ClientAttach( this );
	}

	ASSERT( m_pChar->IsClient());
	ASSERT( m_pChar->m_pPlayer );

	CItem * pItemChange = m_pChar->LayerFind(LAYER_FLAG_ClientLinger);
	if ( pItemChange != NULL )
	{
		pItemChange->Delete();
	}

	m_Env.SetInvalid();
/*
	CExtData ExtData;
	ExtData.Party_Enable.m_state = 1;
	addExtData( EXTDATA_Party_Enable, &ExtData, sizeof(ExtData.Party_Enable));
*/

	CPointMap pt = m_pChar->GetTopPoint();

	CCommand cmd;
	cmd.Start.m_Cmd = XCMD_Start;
	cmd.Start.m_UID = m_pChar->GetUID();
	cmd.Start.m_unk_5_8 = 0x00;
	cmd.Start.m_id = m_pChar->GetDispID();
	cmd.Start.m_x = pt.m_x;
	cmd.Start.m_y = pt.m_y;
	cmd.Start.m_z = pt.m_z;
	cmd.Start.m_dir = m_pChar->GetDirFlag();
	cmd.Start.m_unk_18 = 0x00;
	cmd.Start.m_unk_19_22 = 0xffffffff;
	cmd.Start.m_boundX = 0x0000;
	cmd.Start.m_boundY = 0x0000;
	// cmd.Start.m_mode = m_pChar->GetModeFlag();

	bool bMap = pt.m_map > 0;

	cmd.Start.m_mode = bMap ? g_MapList.GetX(pt.m_map) : 0x1800;
	cmd.Start.m_boundH = bMap ? g_MapList.GetY(pt.m_map) : 0x1000;
	cmd.Start.m_zero_31 = 0x0000;
	cmd.Start.m_zero_33 = 0x00000000;

	xSendPkt( &cmd, sizeof( cmd.Start ));

	ClearTargMode();	// clear death menu mode. etc. ready to walk about. cancel any previos modes

	addMap( NULL, true );

	addChangeServer();

	addMapDiff();

	addPlayerView( pt, true );

	addRedrawAll();
	addTime( true );

	m_pChar->MoveToChar( pt );	// Make sure we are in active list.
	m_pChar->Update();

	if ( pChar->m_pParty )
	{
		pChar->m_pParty->SendAddList(NULL);
	}

	addWeather(WEATHER_DEFAULT);
	addLight();
	addPlayerWarMode();
}

void CClient::addPlayerWarMode()
{
	ADDTOCALLSTACK("CClient::addPlayerWarMode");
	CCommand cmd;
	cmd.War.m_Cmd = XCMD_War;
	cmd.War.m_warmode = ( m_pChar->IsStatFlag( STATF_War )) ? 1 : 0;
	cmd.War.m_unk2[0] = 0;
	cmd.War.m_unk2[1] = 0x32;	// ?
	cmd.War.m_unk2[2] = 0;
	xSendPkt( &cmd, sizeof( cmd.War ));
}

void CClient::addToolTip( const CObjBase * pObj, LPCTSTR pszText )
{
	ADDTOCALLSTACK("CClient::addToolTip");
	if ( pObj == NULL )
		return;
	if ( pObj->IsChar())
		return; // no tips on chars.

	CCommand cmd;
	int i = CvtSystemToNUNICODE( cmd.ToolTip.m_utext, MAX_TALK_BUFFER, pszText, -1 );
	int len = ((i + 1) * sizeof(NCHAR)) + ( sizeof(cmd.ToolTip) - sizeof(cmd.ToolTip.m_utext));

	cmd.ToolTip.m_Cmd = XCMD_ToolTip;
	cmd.ToolTip.m_len = len;
	cmd.ToolTip.m_UID = pObj->GetUID();

	xSendPkt( &cmd, len );
}

bool CClient::addBookOpen( CItem * pBook )
{
	ADDTOCALLSTACK("CClient::addBookOpen");
	// word wrap is done when typed in the client. (it has a var size font)
	if ( !pBook )
		return false;

	int iPagesNow = 0;
	bool fWritable;
	WORD nPages = 0;
	CGString sTitle;
	CGString sAuthor;

	if ( pBook->IsBookSystem())
	{
		fWritable = false;

		CResourceLock s;
		if ( ! g_Cfg.ResourceLock( s, pBook->m_itBook.m_ResID ))
			return false;

		while ( s.ReadKeyParse())
		{
			switch ( FindTableSorted( s.GetKey(), CItemMessage::sm_szLoadKeys, COUNTOF( CItemMessage::sm_szLoadKeys )-1 ))
			{
				case CIC_AUTHOR:
					sAuthor = s.GetArgStr();
					break;
				case CIC_PAGES:
					nPages = s.GetArgVal();
					break;
				case CIC_TITLE:
					sTitle = s.GetArgStr();
					break;
			}
		}
		if ( ! sTitle.IsEmpty())
		{
			pBook->SetName( sTitle );	// Make sure the book is named.
		}
	}
	else
	{
		// User written book.
		CItemMessage * pMsgItem = dynamic_cast <CItemMessage *> (pBook);
		if ( pMsgItem == NULL )
			return false;

		fWritable = pMsgItem->IsBookWritable() ? true : false;	// Not sealed
		nPages = fWritable ? ( MAX_BOOK_PAGES ) : ( pMsgItem->GetPageCount());	// Max pages.
		sTitle = pMsgItem->GetName();
		sAuthor = (pMsgItem->m_sAuthor.IsEmpty()) ? "unknown" : (LPCTSTR)( pMsgItem->m_sAuthor );

		if ( fWritable )	// For some reason we must send them now.
		{
			iPagesNow = pMsgItem->GetPageCount();
		}
	}

	CCommand cmd;
	cmd.BookOpen.m_Cmd = XCMD_BookOpen;
	cmd.BookOpen.m_UID = pBook->GetUID();
	cmd.BookOpen.m_writable = fWritable ? 1 : 0;
	cmd.BookOpen.m_NEWunk1 = fWritable ? 1 : 0;
	cmd.BookOpen.m_pages = nPages;
	strcpy(cmd.BookOpen.m_title, sTitle);
	strcpy(cmd.BookOpen.m_author, sAuthor);
	xSendPkt(&cmd, sizeof(cmd.BookOpen));

	// We could just send all the pages now if we want.
	if ( iPagesNow )
	{
		if ( iPagesNow>nPages )
			iPagesNow=nPages;
		for ( int i=0; i<iPagesNow; i++ )
		{
			addBookPage( pBook, i+1 );
		}
	}

	return( true );
}

void CClient::addBookPage( const CItem * pBook, int iPage )
{
	ADDTOCALLSTACK("CClient::addBookPage");
	// ARGS:
	//  iPage = 1 based page.
	if ( iPage <= 0 )
		return;

	CCommand cmd;
	cmd.BookPage.m_Cmd = XCMD_BookPage;
	cmd.BookPage.m_UID = pBook->GetUID();
	cmd.BookPage.m_pages = 1;	// we can send multiple pages if we wish.
	cmd.BookPage.m_page[0].m_pagenum = iPage;	// 1 based page.

	int lines=0;
	int length=0;

	if ( pBook->IsBookSystem())
	{
		CResourceLock s;
		if ( ! g_Cfg.ResourceLock( s, RESOURCE_ID( RES_BOOK, pBook->m_itBook.m_ResID.GetResIndex(), iPage )))
			return;

		while (s.ReadKey(false))
		{
			lines++;
			length += strcpylen( cmd.BookPage.m_page[0].m_text+length, s.GetKey()) + 1;
		}
	}
	else
	{
		// User written book pages.
		const CItemMessage * pMsgItem = dynamic_cast <const CItemMessage *> (pBook);
		if ( pMsgItem == NULL )
			return;
		iPage --;
		if ( iPage < pMsgItem->GetPageCount())
		{
			// Copy the pages to the book
			LPCTSTR pszText = pMsgItem->GetPageText(iPage);
			if ( pszText )
			{
				while (true)
				{
					TCHAR ch = pszText[ length ];
					if ( ch == '\t' )
					{
						ch = '\0';
						lines++;
					}
					cmd.BookPage.m_page[0].m_text[ length ] = ch;
					if ( pszText[ length ++ ] == '\0' )
					{
						if ( length > 1 ) lines ++;
						break;
					}
				}
			}
		}
	}

	length += sizeof( cmd.BookPage ) - sizeof( cmd.BookPage.m_page[0].m_text );
	cmd.BookPage.m_len = length;
	cmd.BookPage.m_page[0].m_lines = lines;

	xSendPkt( &cmd, length );
}

int CClient::Setup_FillCharList( CEventCharDef * pCharList, const CChar * pCharFirst )
{
	ADDTOCALLSTACK("CClient::Setup_FillCharList");
	// list available chars for your account that are idle.
	CAccount * pAccount = GetAccount();
	ASSERT( pAccount );
	int j=0;

	if ( pCharFirst && pAccount->IsMyAccountChar( pCharFirst ))
	{
		m_tmSetupCharList[0] = pCharFirst->GetUID();
		strcpylen( pCharList[0].m_charname, pCharFirst->GetName(), sizeof( pCharList[0].m_charname ));
		pCharList[0].m_charpass[0] = '\0';
		j++;
	}

	int iQty = pAccount->m_Chars.GetCharCount();
	for ( int k=0; k<iQty; k++ )
	{
		CGrayUID uid( pAccount->m_Chars.GetChar(k));
		CChar * pChar = uid.CharFind();
		if ( pChar == NULL )
			continue;
		if ( pCharFirst == pChar )
			continue;
		if ( j >= MAX_CHARS_PER_ACCT )
			break;

		// if ( j >= g_Cfg.m_iMaxCharsPerAccount && ! IsPriv(PRIV_GM)) break;

		m_tmSetupCharList[j] = uid;
		strcpylen( pCharList[j].m_charname, pChar->GetName(), sizeof( pCharList[0].m_charname ));
		pCharList[j].m_charpass[0] = '\0';
		j++;
	}

	// always show max count for some stupid reason. (client bug)
	// pad out the rest of the chars.
	for ( ;j<MAX_CHARS_PER_ACCT;j++)
	{
		m_tmSetupCharList[j].InitUID();
		pCharList[j].m_charname[0] = '\0';
		pCharList[j].m_charpass[0] = '\0';
	}

	if ( IsClientVer( 0x300001 ) || IsNoCryptVer(0x300001) || (!m_Crypt.GetClientVer()) )	// the 2nd or wiil fail at this stage
		return iQty;
	else
		return( MAX_CHARS_PER_ACCT );
}

void CClient::SetTargMode( CLIMODE_TYPE targmode, LPCTSTR pPrompt )
{
	ADDTOCALLSTACK("CClient::SetTargMode");
	// ??? Get rid of menu stuff if previous targ mode.
	// Can i close a menu ?
	// Cancel a cursor input.

	if ( GetTargMode() == CLIMODE_TARG_USE_ITEM )
	{
		CItem * pItemUse = m_Targ_UID.ItemFind();
		if ( pItemUse )
		{
			if ( !IsSetEF(EF_Minimize_Triggers) )
			{
				if ( pItemUse->OnTrigger( ITRIG_TARGON_CANCEL, m_pChar ) == TRIGRET_RET_TRUE )
				{
					m_Targ_Mode = targmode;
					if ( targmode != CLIMODE_NORMAL )
						addSysMessage( pPrompt );
					return;
				}
			}
		}
	}

	if ( GetTargMode() == targmode )
		return;

	if ( GetTargMode() != CLIMODE_NORMAL && targmode != CLIMODE_NORMAL )
	{
		// Just clear the old target mode
		addSysMessage( g_Cfg.GetDefaultMsg(DEFMSG_TARGET_CANCEL_2) );
	}

	m_Targ_Mode = targmode;

	if ( targmode == CLIMODE_NORMAL )
		addSysMessage( g_Cfg.GetDefaultMsg(DEFMSG_TARGET_CANCEL_1) );
	else if ( pPrompt && *pPrompt ) // Check that the message is not blank.
		addSysMessage( pPrompt );
}

void CClient::addPromptConsole( CLIMODE_TYPE targmode, LPCTSTR pPrompt )
{
	ADDTOCALLSTACK("CClient::addPromptConsole");
	SetTargMode( targmode, pPrompt );

	CCommand cmd;
	cmd.Prompt.m_Cmd = XCMD_Prompt;
	cmd.Prompt.m_len = sizeof( cmd.Prompt );
	memset( cmd.Prompt.m_unk3, 0, sizeof(cmd.Prompt.m_unk3));
	cmd.Prompt.m_text[0] = '\0';

	xSendPkt( &cmd, cmd.Prompt.m_len );
}

void CClient::addTarget( CLIMODE_TYPE targmode, LPCTSTR pPrompt, bool fAllowGround, bool fCheckCrime ) // Send targetting cursor to client
{
	ADDTOCALLSTACK("CClient::addTarget");
	// Expect XCMD_Target back.
	// ??? will this be selective for us ? objects only or chars only ? not on the ground (statics) ?

	SetTargMode( targmode, pPrompt );

	CCommand cmd;
	memset( &(cmd.Target), 0, sizeof( cmd.Target ));
	cmd.Target.m_Cmd = XCMD_Target;
	cmd.Target.m_TargType = fAllowGround; // fAllowGround;	// 1=allow xyz, 0=objects only.
	cmd.Target.m_context = targmode ;	// 5=my id code for action.
	cmd.Target.m_fCheckCrime = fCheckCrime; // // Not sure what this is. (m_checkcrimflag?)

	xSendPkt( &cmd, sizeof( cmd.Target ));
}

void CClient::addTargetDeed( const CItem * pDeed )
{
	ADDTOCALLSTACK("CClient::addTargetDeed");
	// Place an item from a deed. preview all the stuff

	ASSERT( m_Targ_UID == pDeed->GetUID());
	ITEMID_TYPE iddef = (ITEMID_TYPE) RES_GET_INDEX( pDeed->m_itDeed.m_Type );
	m_tmUseItem.m_pParent = pDeed->GetParent();	// Cheat Verify.
	addTargetItems( CLIMODE_TARG_USE_ITEM, iddef );
}

bool CClient::addTargetChars( CLIMODE_TYPE mode, CREID_TYPE baseID, bool fNotoCheck )
{
	ADDTOCALLSTACK("CClient::addTargetChars");
	CCharBase * pBase = CCharBase::FindCharBase( baseID );
	if ( pBase == NULL )
		return( false );

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "%s '%s'?", g_Cfg.GetDefaultMsg(DEFMSG_WHERE_TO_SUMMON), (LPCTSTR) pBase->GetTradeName());

	addTarget(mode, pszTemp, true, fNotoCheck);
	return true;
}

bool CClient::addTargetItems( CLIMODE_TYPE targmode, ITEMID_TYPE id, bool fGround )
{
	ADDTOCALLSTACK("CClient::addTargetItems");
	// Add a list of items to place at target.
	// preview all the stuff

	ASSERT(m_pChar);

	LPCTSTR pszName;
	CItemBase * pItemDef;
	if ( id < ITEMID_TEMPLATE )
	{
		pItemDef = CItemBase::FindItemBase( id );
		if ( pItemDef == NULL )
			return false;
		pszName = pItemDef->GetName();

		if ( pItemDef->IsType(IT_STONE_GUILD) )
		{
			// Check if they are already in a guild first
			CItemStone * pStone = m_pChar->Guild_Find(MEMORY_GUILD);
			if (pStone)
			{
				addSysMessage( "You are already a member of a guild. Resign first!");
				return false;
			}
		}
	}
	else
	{
		pItemDef = NULL;
		pszName = "template";
	}

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "%s %s?", g_Cfg.GetDefaultMsg(DEFMSG_WHERE_TO_PLACE), pszName);

	if ( CItemBase::IsID_Multi( id ))	// a multi we get from Multi.mul
	{
		SetTargMode(targmode, pszTemp);

		CCommand cmd;
		cmd.TargetMulti.m_Cmd = XCMD_TargetMulti;
		cmd.TargetMulti.m_fAllowGround = fGround;
		cmd.TargetMulti.m_context = targmode ;	// 5=my id code for action.
		memset( cmd.TargetMulti.m_zero6, 0, sizeof(cmd.TargetMulti.m_zero6));
		cmd.TargetMulti.m_id = id - ITEMID_MULTI;

		// Add any extra stuff attached to the multi. preview this.

		memset( cmd.TargetMulti.m_zero20, 0, sizeof(cmd.TargetMulti.m_zero20));

		xSendPkt( &cmd, sizeof( cmd.TargetMulti ));
		return true;
	}

	// preview not supported by this ver?
	addTarget(targmode, pszTemp, true);
	return true;
}

void CClient::addDyeOption( const CObjBase * pObj )
{
	ADDTOCALLSTACK("CClient::addDyeOption");
	// Put up the wHue chart for the client.
	// This results in a Event_Item_Dye message. CLIMODE_DYE
	ITEMID_TYPE id;
	if ( pObj->IsItem())
	{
		const CItem * pItem = dynamic_cast <const CItem*> (pObj);
		ASSERT(pItem);
		id = pItem->GetDispID();
	}
	else
	{
		// Get the item equiv for the creature.
		const CChar * pChar = dynamic_cast <const CChar*> (pObj);
		ASSERT(pChar);
		id = pChar->Char_GetDef()->m_trackID;
	}

	CCommand cmd;
	cmd.DyeVat.m_Cmd = XCMD_DyeVat;
	cmd.DyeVat.m_UID = pObj->GetUID();
	cmd.DyeVat.m_zero5 = pObj->GetHue();
	cmd.DyeVat.m_id = id;
	xSendPkt( &cmd, sizeof( cmd.DyeVat ));
	SetTargMode( CLIMODE_DYE );
}

void CClient::addSkillWindow(SKILL_TYPE skill, bool bFromInfo) // Opens the skills list
{
	ADDTOCALLSTACK("CClient::addSkillWindow");
	// Whos skills do we want to look at ?
	CChar *pChar = m_Prop_UID.CharFind();
	if ( !pChar )
		pChar = m_pChar;

	bool bNewCaps = ( IsClientVer(0x400000) || IsNoCryptVer(0x400000) );

	CCommand cmd;
	cmd.Skill.m_Cmd = XCMD_Skill;

	int len = sizeof(cmd.Skill.m_Cmd) + sizeof(cmd.Skill.m_len) + sizeof(cmd.Skill.m_single);

	if ( skill >= MAX_SKILL )
	{	// all skills
		if ( !IsSetEF(EF_Minimize_Triggers) )
		{
			CScriptTriggerArgs Args(-1, bFromInfo);
			if ( m_pChar->OnTrigger( CTRIG_UserSkills, pChar, &Args ) == TRIGRET_RET_TRUE )
				return;
		}

		if ( bNewCaps )
			cmd.Skill_New.m_single = 0x02;
		else
			cmd.Skill.m_single = 0x00;

		int amount = 0, i = 0;
		for ( i = 0; i < MAX_SKILL; i++ )
		{
			if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(i) )
				continue;

			amount++;
			int iskillval = pChar->Skill_GetBase( (SKILL_TYPE) i);

			if ( bNewCaps )
			{
				cmd.Skill_New.skills[i].m_index = i+1;
				cmd.Skill_New.skills[i].m_val = pChar->Skill_GetAdjusted( (SKILL_TYPE) i);
				cmd.Skill_New.skills[i].m_valraw = iskillval;
				cmd.Skill_New.skills[i].m_lock = pChar->Skill_GetLock( (SKILL_TYPE) i);
				cmd.Skill_New.skills[i].m_cap = (WORD) pChar->Skill_GetMax( (SKILL_TYPE) i);
			}
			else
			{
				cmd.Skill.skills[i].m_index = i+1;
				cmd.Skill.skills[i].m_val = pChar->Skill_GetAdjusted( (SKILL_TYPE) i);
				cmd.Skill.skills[i].m_valraw = iskillval;
				cmd.Skill.skills[i].m_lock = pChar->Skill_GetLock( (SKILL_TYPE) i);
			}
		}

		if ( bNewCaps )
		{
			cmd.Skill_New.skills[i].m_index = 0;	// terminator.
			len += (amount * sizeof(cmd.Skill_New.skills[0])) + sizeof(NWORD);
		}
		else
		{
			cmd.Skill.skills[i].m_index = 0;	// terminator.
			len += (amount * sizeof(cmd.Skill.skills[0])) + sizeof(NWORD);
		}
	}
	else
	{	// Just one skill update.
		if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(skill) )
			return;

		if ( !IsSetEF(EF_Minimize_Triggers) )
		{
			CScriptTriggerArgs Args( skill );
			if ( m_pChar->OnTrigger( CTRIG_UserSkills, pChar, &Args ) == TRIGRET_RET_TRUE )
				return;
		}

		if ( skill >= SKILL_SCRIPTED )
			return;

		len += sizeof(cmd.Skill.skills[0]);

		if ( bNewCaps )
			cmd.Skill_New.m_single = 0xdf;
		else
			cmd.Skill.m_single = 0xff;

		int iskillval = pChar->Skill_GetBase( skill );

		if ( bNewCaps )
		{
			cmd.Skill_New.skills[0].m_index = skill;
			cmd.Skill_New.skills[0].m_val = pChar->Skill_GetAdjusted( skill );
			cmd.Skill_New.skills[0].m_valraw = iskillval;
			cmd.Skill_New.skills[0].m_lock = pChar->Skill_GetLock( skill );
			cmd.Skill_New.skills[0].m_cap = (WORD) pChar->Skill_GetMax( skill );
			len += sizeof(cmd.Skill_New.skills[0].m_cap);
		}
		else
		{
			cmd.Skill.skills[0].m_index = skill;
			cmd.Skill.skills[0].m_val = pChar->Skill_GetAdjusted( skill );
			cmd.Skill.skills[0].m_valraw = iskillval;
			cmd.Skill.skills[0].m_lock = pChar->Skill_GetLock( skill );
		}
	}

	if ( bNewCaps )
		cmd.Skill_New.m_len = len;
	else
		cmd.Skill.m_len = len;

	xSendPkt( &cmd, len );
}

void CClient::addAOSPlayerSeeNoCrypt()
{
	ADDTOCALLSTACK("CClient::addAOSPlayerSeeNoCrypt");
	// Adjust to my new location, what do I now see here?
	bool fAllShow = IsPriv(PRIV_ALLSHOW);
	bool fOsiSight = IsSetOF(OF_OSIMultiSight);
	CRegionBase * pCurrentCharRegion = m_pChar->GetTopPoint().GetRegion(REGION_TYPE_MULTI);

	//	Items on the ground
	CWorldSearch AreaItems(m_pChar->GetTopPoint(), UO_MAP_VIEW_SIZE);
	AreaItems.SetAllShow(fAllShow);
	DWORD	dSeeItems = 0;

	while (true)
	{
		CItem *pItem = AreaItems.GetItem();
		if ( !pItem )
			break;
		if ( !CanSee(pItem) )
			continue;
		if (fOsiSight)
		{
			if (( !pItem->GetTopLevelObj()->GetTopPoint().GetRegion(REGION_TYPE_MULTI) || ( pItem->m_TagDefs.GetKeyNum("ALWAYSSEND", true) ) || ( pItem->GetType() == IT_MULTI ) || ( pItem->GetType() == IT_SHIP ) || (( pItem->m_uidLink.IsValidUID() ) && ( pItem->m_uidLink.IsItem() ) && (( pItem->m_uidLink.ItemFind()->GetType() == IT_MULTI ) || ( pItem->m_uidLink.ItemFind()->GetType() == IT_SHIP )))
				|| ( pItem->GetTopLevelObj()->GetTopPoint().GetRegion(REGION_TYPE_MULTI) == pCurrentCharRegion )))
			{
				if ( dSeeItems < g_Cfg.m_iMaxItemComplexity*30 )
				{
					++dSeeItems;
					addAOSTooltip(pItem);
				}
				else
					break;
			}
		}
		else
		{
			if ( dSeeItems < g_Cfg.m_iMaxItemComplexity*30 )
			{
				++dSeeItems;
				addAOSTooltip(pItem);
			}
			else
				break;
		}
	}

	//	Characters around
	CWorldSearch AreaChars(m_pChar->GetTopPoint(), UO_MAP_VIEW_SIZE);
	AreaChars.SetAllShow(fAllShow);
	DWORD	dSeeChars(0);
	while ( true )
	{
		CChar	*pChar = AreaChars.GetChar();
		if ( !pChar )
			break;
		if ( !CanSee(pChar) )
			continue;
		if ( dSeeChars < g_Cfg.m_iMaxCharComplexity*5 )
		{
			++dSeeChars;
			addAOSTooltip(pChar);
		}
		else
			break;
	}
}

void CClient::addPlayerSee( const CPointMap & ptold )
{
	ADDTOCALLSTACK("CClient::addPlayerSee");
	// Adjust to my new location, what do I now see here?
	bool fAllShow = IsPriv(PRIV_ALLSHOW);
	bool fOsiSight = IsSetOF(OF_OSIMultiSight);
	CRegionBase * pCurrentCharRegion = m_pChar->GetTopPoint().GetRegion(REGION_TYPE_MULTI);

	CChar * tMe = GetChar();
	BYTE tViewDist;
	if ( tMe )
		tViewDist = tMe->GetSight();
	else
		// dunno if this _can_ happen, but to be on the safe side ... Nazghul
		tViewDist = UO_MAP_VIEW_SIZE;

	//	Items on the ground
	CWorldSearch AreaItems(m_pChar->GetTopPoint(), UO_MAP_VIEW_RADAR);
	AreaItems.SetAllShow(fAllShow);
	DWORD	dSeeItems = 0;

	while (true)
	{
		CItem *pItem = AreaItems.GetItem();
		if ( !pItem )
			break;
		if ( !CanSee(pItem) )
			continue;

		if (fOsiSight)
		{
			if (( !pItem->GetTopLevelObj()->GetTopPoint().GetRegion(REGION_TYPE_MULTI) ) || ( pItem->m_TagDefs.GetKeyNum("ALWAYSSEND", true) ) || ( pItem->GetType() == IT_MULTI ) || ( pItem->GetType() == IT_SHIP ) || (( pItem->m_uidLink.IsValidUID() ) && ( pItem->m_uidLink.IsItem() ) && (( pItem->m_uidLink.ItemFind()->GetType() == IT_MULTI ) || ( pItem->m_uidLink.ItemFind()->GetType() == IT_SHIP )))
				|| ((( ptold.GetRegion(REGION_TYPE_MULTI) != pCurrentCharRegion ) || ( ptold.GetDist(pItem->GetTopPoint()) > UO_MAP_VIEW_SIZE )) && ( pItem->GetType() != IT_MULTI ) && ( pItem->GetType() != IT_SHIP ) && ( pItem->GetTopLevelObj()->GetTopPoint().GetRegion(REGION_TYPE_MULTI) == pCurrentCharRegion )))
			{
				if ((( m_pChar->GetTopPoint().GetDist(pItem->GetTopPoint()) <= UO_MAP_VIEW_SIZE ) && ( ptold.GetDist(pItem->GetTopPoint()) > UO_MAP_VIEW_SIZE )) || (( ptold.GetDist(pItem->GetTopPoint()) > tViewDist ) && (( pItem->GetType() == IT_SHIP ) || ( pItem->GetType() == IT_MULTI ))) || ((( ptold.GetRegion(REGION_TYPE_MULTI) != pCurrentCharRegion ) || ( ptold.GetDist(pItem->GetTopPoint()) > tViewDist )) && ( pItem->GetType() != IT_MULTI ) && ( pItem->GetType() != IT_SHIP ) && ( pItem->GetTopLevelObj()->GetTopPoint().GetRegion(REGION_TYPE_MULTI) == pCurrentCharRegion )))
				{
					if ( dSeeItems < g_Cfg.m_iMaxItemComplexity*30 )
					{
						++dSeeItems;
						addItem_OnGround(pItem);
					}
					else
						break;
				}
			}
		}
		else
		{
			if ((( m_pChar->GetTopPoint().GetDist(pItem->GetTopPoint()) <= tViewDist ) && ( ptold.GetDist(pItem->GetTopPoint()) > tViewDist )) || (( ptold.GetDist(pItem->GetTopPoint()) > tViewDist ) && (( pItem->GetType() == IT_SHIP ) || ( pItem->GetType() == IT_MULTI ))))
			{
				if ( dSeeItems < g_Cfg.m_iMaxItemComplexity*30 )
				{
					++dSeeItems;
					addItem_OnGround(pItem);
				}
				else
					break;
			}
		}
	}

	//	Characters around
	CWorldSearch AreaChars(m_pChar->GetTopPoint(), UO_MAP_VIEW_SIZE);
	AreaChars.SetAllShow(fAllShow);
	DWORD	dSeeChars(0);
	while ( true )
	{
		CChar	*pChar = AreaChars.GetChar();
		if ( !pChar )
			break;
		if (( m_pChar == pChar ) || !CanSee(pChar) )
			continue;

		if ( ptold.GetDist( pChar->GetTopPoint()) > tViewDist )
		{
			if ( dSeeChars < g_Cfg.m_iMaxCharComplexity*5 )
			{
				++dSeeChars;
				addChar(pChar);
			}
			else
				break;
		}
	}
}

void CClient::addPlayerView( const CPointMap & ptold, bool playerStart )
{
	ADDTOCALLSTACK("CClient::addPlayerView");
	// I moved = Change my point of view. Teleport etc..

	CCommand cmd;
	cmd.View.m_Cmd = XCMD_View;
	cmd.View.m_UID = m_pChar->GetUID();

	CREID_TYPE id;
	HUE_TYPE wHue;
	GetAdjustedCharID( m_pChar, id, wHue );
	cmd.View.m_id = id;
	cmd.View.m_zero7 = 0;
	cmd.View.m_wHue = wHue;
	cmd.View.m_mode = m_pChar->GetModeFlag();

	CPointMap pt = m_pChar->GetTopPoint();
	cmd.View.m_x = pt.m_x;
	cmd.View.m_y = pt.m_y;
	cmd.View.m_zero15 = 0;
	cmd.View.m_dir = m_pChar->GetDirFlag();
	cmd.View.m_z = pt.m_z;

	xSendPkt( &cmd, sizeof( cmd.View ));

	// resync this stuff.
	m_wWalkCount = -1;

	if ( ptold == pt )
		return;	// not a real move i guess. might just have been a change in face dir.

	m_Env.SetInvalid();	// Must resend environ stuff.

	// What can i see here ?
	if ( !playerStart )
		addPlayerSee( ptold );
}

void CClient::addReSync(bool bForceMap)
{
	ADDTOCALLSTACK("CClient::addReSync");
	// Reloads the client with all it needs.
	CPointMap ptold;	// invalid value.
	addMap(NULL, !bForceMap);
	addPlayerView(ptold);
	addChar( m_pChar );
	addLight();		// Current light level where I am.
	addWeather();	// if any ...
	addSpeedMode( m_pChar->m_pPlayer->m_speedMode );
}

void CClient::addMap( const CPointMap * pOldP, bool playerStart)
{
	ADDTOCALLSTACK("CClient::addMap");
	CPointMap pt = m_pChar->GetTopPoint();

	if ( !playerStart && pOldP && pOldP->m_map == pt.m_map )
		return;

	CExtData ExtData;
	ExtData.MapChange.m_state = g_MapList.m_mapid[pt.m_map];
	addExtData( EXTDATA_Map_Change, &ExtData, sizeof(ExtData.MapChange) );
	if ( !playerStart )
	{
		CPointMap	ptold;
		addPlayerView(ptold);
		addChar(m_pChar);
		addLight();		// Current light level where I am.
	}
}

void CClient::addMapDiff()
{
	ADDTOCALLSTACK("CClient::addMapDiff");
	// Enables map diff usage on the client. If the client is told to
	// enable diffs, and then logs back in without restarting, it will
	// continue to use the diffs even if not told to enable them - so
	// this packet should always be sent even if empty.

	CExtData ExtData;

	int iMapCount = 1;
	for ( int map = 255; map >= 0; map-- )
	{
		if ( g_MapList.m_maps[map] )
		{
			iMapCount = (map + 1);
			break;
		}
	}

	ExtData.Map_Diff.m_maps_number = iMapCount;
	for ( int map = 0; map < iMapCount; map++ )
	{
		unsigned int tileCount = 0;
		unsigned int staticCount = 0;
		if ( IsSetEF( EF_Mapdiff_Support ) && g_MapList.m_maps[map] )
		{
			if ( g_Install.m_Mapdifl[map].IsFileOpen() )
				tileCount = g_Install.m_Mapdifl[map].GetLength() / 4;

			if ( g_Install.m_Stadifl[map].IsFileOpen() )
				staticCount = g_Install.m_Stadifl[map].GetLength() / 4;
		}

		ExtData.Map_Diff.m_patch[map].m_map_patch = tileCount;
		ExtData.Map_Diff.m_patch[map].m_static_patch = staticCount;
	}

	addExtData( EXTDATA_Map_Diff, &ExtData, 4 + (8 * iMapCount) );
}

void CClient::addChangeServer()
{
	ADDTOCALLSTACK("CClient::addChangeServer");
	CPointMap pt = m_pChar->GetTopPoint();

	CCommand cmd;

	cmd.ZoneChange.m_Cmd = XCMD_ZoneChange;
	cmd.ZoneChange.m_x = pt.m_x;
	cmd.ZoneChange.m_y = pt.m_y;
	cmd.ZoneChange.m_z = pt.m_z;
	cmd.ZoneChange.m_unk7_zero = 0x00;
	cmd.ZoneChange.m_serv_boundX = 0x0000;
	cmd.ZoneChange.m_serv_boundY = 0x0000;

	cmd.ZoneChange.m_serv_boundW = g_MapList.GetX(pt.m_map);
	cmd.ZoneChange.m_serv_boundH = g_MapList.GetY(pt.m_map);

	xSend( &cmd, sizeof(cmd.ZoneChange) );
}

void CClient::UpdateStats()
{
	ADDTOCALLSTACK("CClient::UpdateStats");
	if ( !m_fUpdateStats || !m_pChar )
		return;

	if ( m_fUpdateStats & SF_UPDATE_STATUS )
	{
		addCharStatWindow( m_pChar->GetUID());
		m_fUpdateStats = 0;
	}
	else
	{
		if ( m_fUpdateStats & SF_UPDATE_HITS )
		{
			addHitsUpdate( m_pChar->GetUID() );
			m_fUpdateStats &= ~SF_UPDATE_HITS;
		}
		if ( m_fUpdateStats & SF_UPDATE_MANA )
		{
			addManaUpdate( m_pChar->GetUID() );
			m_fUpdateStats &= ~SF_UPDATE_MANA;
		}

		if ( m_fUpdateStats & SF_UPDATE_STAM )
		{
			addStamUpdate( m_pChar->GetUID() );
			m_fUpdateStats &= ~SF_UPDATE_STAM;
		}
	}
}

void CClient::addCharStatWindow( CGrayUID uid, bool fRequested ) // Opens the status window
{
	ADDTOCALLSTACK("CClient::addCharStatWindow");
	CChar * pChar = uid.CharFind();
	if ( !pChar )
		return;

	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		CScriptTriggerArgs	Args(0, 0, uid.ObjFind());
		Args.m_iN3	= fRequested;
		if ( m_pChar->OnTrigger( CTRIG_UserStats, pChar, &Args ) == TRIGRET_RET_TRUE )
			return;
	}

	bool bStatLocks = false;

	CCommand cmd;
	cmd.Status.m_Cmd = XCMD_Status;
	cmd.Status.m_UID = pChar->GetUID();

	strcpylen(cmd.Status.m_charname, pChar->GetName(), sizeof(cmd.Status.m_charname));

	// renamable ?
	if ( m_pChar != pChar && pChar->NPC_IsOwnedBy(m_pChar) && !pChar->Char_GetDef()->GetHireDayWage() )
		cmd.Status.m_perm = 0xFF;	// I can rename them
	else
		cmd.Status.m_perm = 0x00;

	// Dont bother sending the rest of this if not my self.
	if ( pChar == m_pChar )
	{
		m_fUpdateStats = false;

		int len = ((BYTE*)&(cmd.Status.m_statcap)) - ((BYTE*)&(cmd.Status));
		// 0 not valid, 1 = till weight, 2 = till statcap, 3 till follower, 4 = till the end., 5 = include maxweight+race
		if ( IsClientLessVer(0x300080) || IsNoCryptLessVer(0x300080) )
		{
			cmd.Status.m_ValidStats = 1;	// valid stats
		}
		else if ( IsClientLessVer(0x400000) || IsNoCryptLessVer(0x400000) )
		{
			len += sizeof(cmd.Status.m_statcap);
			cmd.Status.m_ValidStats = 2;
		}
		else if ( IsClientLessVer(0x400010) || IsNoCryptLessVer(0x400010) )
		{
			len += sizeof(cmd.Status.m_statcap) + sizeof(cmd.Status.m_curFollower) + sizeof(cmd.Status.m_maxFollower);
			cmd.Status.m_ValidStats = 3;
		}
		else if ( IsClientLessVer(0x500000) || IsNoCryptLessVer(0x500000) )
		{
			len += sizeof(cmd.Status);
			cmd.Status.m_ValidStats = 4;
			bStatLocks = true;
		}
		else
		{
			len += sizeof(cmd.StatusNew);
			cmd.Status.m_ValidStats = 5;
			bStatLocks = true;
		}

		cmd.Status.m_len = len;
		cmd.Status.m_sex = ( pChar->Char_GetDef()->IsFemale()) ? 1 : 0;

		int iStr = pChar->Stat_GetAdjusted(STAT_STR);
		if ( iStr < 0 ) iStr = 0;
		cmd.Status.m_str = iStr;

		int iDex = pChar->Stat_GetAdjusted(STAT_DEX);
		if ( iDex < 0 ) iDex = 0;
		cmd.Status.m_dex = iDex;

		int iInt = pChar->Stat_GetAdjusted(STAT_INT);
		if ( iInt < 0 ) iInt = 0;
		cmd.Status.m_int = iInt;

		cmd.Status.m_health		= pChar->Stat_GetVal(STAT_STR);
		cmd.Status.m_maxhealth	= pChar->Stat_GetMax(STAT_STR);
		cmd.Status.m_stam		= pChar->Stat_GetVal(STAT_DEX);
		cmd.Status.m_maxstam	= pChar->Stat_GetMax(STAT_DEX);
		cmd.Status.m_mana		= pChar->Stat_GetVal(STAT_INT);
		cmd.Status.m_maxmana	= pChar->Stat_GetMax(STAT_INT);

		if ( !g_Cfg.m_fPayFromPackOnly )
			cmd.Status.m_gold = pChar->ContentCount( RESOURCE_ID(RES_TYPEDEF,IT_GOLD));	/// ??? optimize this count is too often.
		else
			cmd.Status.m_gold = pChar->GetPackSafe()->ContentCount( RESOURCE_ID(RES_TYPEDEF,IT_GOLD));

		cmd.Status.m_armor = pChar->m_defense + pChar->Char_GetDef()->m_defense;
		cmd.Status.m_weight = pChar->GetTotalWeight() / WEIGHT_UNITS;

		if (cmd.Status.m_ValidStats >= 2)
		{
			int iCap = pChar->Stat_GetLimit(STAT_QTY);
			if ( iCap < 0 ) iCap = 0;
			cmd.Status.m_statcap = iCap;
		}

		if (cmd.Status.m_ValidStats >= 3)
		{
			cmd.Status.m_curFollower = pChar->m_pPlayer->m_curFollower;
			cmd.Status.m_maxFollower = pChar->m_pPlayer->m_maxFollower;
		}

		if (cmd.Status.m_ValidStats >= 4)
		{
			cmd.Status.m_ResFire = pChar->m_ResFire;
			cmd.Status.m_ResCold = pChar->m_ResCold;
			cmd.Status.m_ResPoison = pChar->m_ResPoison;
			cmd.Status.m_ResEnergy = pChar->m_ResEnergy;
			cmd.Status.m_Luck = pChar->m_pPlayer->m_luck;

			CItem * pWeapon = dynamic_cast<CItem *>(pChar->m_uidWeapon.ObjFind());
			if ( pWeapon )
			{
				cmd.Status.m_minDamage = pWeapon->Item_GetDef()->m_attackBase;
				cmd.Status.m_maxDamage = pChar->Fight_CalcDamage(pWeapon, pWeapon->Weapon_GetSkill(), true);
			}
			else
			{
				cmd.Status.m_minDamage = pChar->Char_GetDef()->m_attackBase;
				cmd.Status.m_maxDamage = pChar->Fight_CalcDamage(NULL, SKILL_WRESTLING, true);
			}

			cmd.Status.m_Tithing = pChar->m_pPlayer->m_iTithingPoints;
		}

		if (cmd.Status.m_ValidStats >= 5)
		{
			// MaxWeight and Race data is inserted before other information, so it
			// must be transferred to a different struct
			cmd.StatusNew.m_Tithing		= cmd.Status.m_Tithing;
			cmd.StatusNew.m_maxDamage	= cmd.Status.m_maxDamage;
			cmd.StatusNew.m_minDamage	= cmd.Status.m_minDamage;
			cmd.StatusNew.m_Luck		= cmd.Status.m_Luck;
			cmd.StatusNew.m_ResEnergy	= cmd.Status.m_ResEnergy;
			cmd.StatusNew.m_ResPoison	= cmd.Status.m_ResPoison;
			cmd.StatusNew.m_ResCold		= cmd.Status.m_ResCold;
			cmd.StatusNew.m_ResFire		= cmd.Status.m_ResFire;
			cmd.StatusNew.m_maxFollower	= cmd.Status.m_maxFollower;
			cmd.StatusNew.m_curFollower	= cmd.Status.m_curFollower;
			cmd.StatusNew.m_statcap		= cmd.Status.m_statcap;

			cmd.StatusNew.m_maxWeight = g_Cfg.Calc_MaxCarryWeight(m_pChar) / WEIGHT_UNITS;
			cmd.StatusNew.m_race = (((m_pChar->GetDispID() == CREID_ELFMAN) || (m_pChar->GetDispID() == CREID_ELFWOMAN))? 1:0);
		}
	}
	else
	{
		cmd.Status.m_ValidStats = 0;
		cmd.Status.m_maxhealth = 100;
		cmd.Status.m_health = (pChar->Stat_GetVal(STAT_STR) * 100) / maximum(pChar->Stat_GetMax(STAT_STR),1);
		cmd.Status.m_len = ((BYTE*)&(cmd.Status.m_sex)) - ((BYTE*)&(cmd.Status));
	}

	xSendPkt(&cmd, cmd.Status.m_len);

	if ( ( pChar != m_pChar ) && ( pChar->m_pParty != NULL ) && ( pChar->m_pParty->IsInParty( m_pChar ) ) )
	{
		// Send mana and stamina info to party members
		addManaUpdate( pChar->GetUID() );
		addStamUpdate( pChar->GetUID() );
	}

	if ( bStatLocks && ( pChar == m_pChar ) && (pChar->m_pPlayer))
	{
		CExtData cStats;
		cStats.Stats_Enable.m_type = 2;
		cStats.Stats_Enable.m_UID = pChar->GetUID();
		cStats.Stats_Enable.m_unk = 0;

		int iLocks = 0;
		iLocks |= (int)pChar->m_pPlayer->Stat_GetLock(STAT_INT);
		iLocks |= (int)pChar->m_pPlayer->Stat_GetLock(STAT_DEX) << 2;
		iLocks |= (int)pChar->m_pPlayer->Stat_GetLock(STAT_STR) << 4;
		cStats.Stats_Enable.m_lockbits = iLocks;

		addExtData(EXTDATA_Stats_Enable, &cStats, 7);
	}

}

void CClient::addHitsUpdate( CGrayUID uid )
{
	ADDTOCALLSTACK("CClient::addHitsUpdate");
	CChar * pChar = uid.CharFind();
	if ( pChar == NULL )
		return;

	CCommand cmd;
	cmd.StatChng.m_Cmd = XCMD_StatChngStr;
	cmd.StatChng.m_UID = pChar->GetUID();
	if ( m_pChar != pChar )
	{
		cmd.StatChng.m_max = 50;
		cmd.StatChng.m_val = ( pChar->Stat_GetVal(STAT_STR) * 50 ) / maximum(pChar->Stat_GetMax(STAT_STR),1);
	}
	else
	{
		cmd.StatChng.m_max = pChar->Stat_GetMax(STAT_STR);
		cmd.StatChng.m_val = pChar->Stat_GetVal(STAT_STR);
	}
	xSendPkt( &cmd, sizeof(cmd.StatChng) );

	if (( pChar->m_pParty == NULL ) || ( m_pChar != pChar ))
		return;

	pChar->m_pParty->AddStatsUpdate( pChar, &cmd, sizeof(cmd.StatChng) );
}

void CClient::addManaUpdate( CGrayUID uid )
{
	ADDTOCALLSTACK("CClient::addManaUpdate");
	CChar * pChar = uid.CharFind();
	if ( pChar == NULL )
		return;

	CCommand cmd;
	cmd.StatChng.m_Cmd = XCMD_StatChngInt;
	cmd.StatChng.m_UID = pChar->GetUID();
	cmd.StatChng.m_max = pChar->Stat_GetMax(STAT_INT);
	cmd.StatChng.m_val = pChar->Stat_GetVal(STAT_INT);
	xSendPkt( &cmd, sizeof(cmd.StatChng) );

	if (( pChar->m_pParty == NULL ) || ( m_pChar != pChar ))
		return;

	pChar->m_pParty->AddStatsUpdate( pChar, &cmd, sizeof(cmd.StatChng) );
}

void CClient::addStamUpdate( CGrayUID uid )
{
	ADDTOCALLSTACK("CClient::addStamUpdate");
	CChar * pChar = uid.CharFind();
	if ( pChar == NULL )
		return;

	CCommand cmd;
	cmd.StatChng.m_Cmd = XCMD_StatChngDex;
	cmd.StatChng.m_UID = pChar->GetUID();
	cmd.StatChng.m_max = pChar->Stat_GetMax(STAT_DEX);
	cmd.StatChng.m_val = pChar->Stat_GetVal(STAT_DEX);
	xSendPkt( &cmd, sizeof(cmd.StatChng) );

	if (( pChar->m_pParty == NULL ) || ( m_pChar != pChar ))
		return;

	pChar->m_pParty->AddStatsUpdate( pChar, &cmd, sizeof(cmd.StatChng) );
}

void CClient::addSpellbookOpen( CItem * pBook, WORD offset )
{
	ADDTOCALLSTACK("CClient::addSpellbookOpen");

	if ( !m_pChar )
		return;

	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		CScriptTriggerArgs	Args( 0, 0, pBook );
		if ( m_pChar->OnTrigger( CTRIG_SpellBook, m_pChar, &Args ) == TRIGRET_RET_TRUE )
			return;
	}

	// NOTE: if the spellbook item is not present on the client it will crash.
	// count what spells I have.

	if ( pBook->GetDispID() == ITEMID_SPELLBOOK2 )
	{
		// weird client bug.
		pBook->SetDispID( ITEMID_SPELLBOOK );
		pBook->Update();
		return;
	}

	int count = pBook->GetSpellcountInBook();
	if ( count == -1 )
		return;

	addOpenGump( pBook, GUMP_OPEN_SPELLBOOK );

	//
	// New AOS spellbook packet required by client 4.0.0 and above.
	// Old packet is still required if both FEATURE_AOS_TOOLTIP and FEATURE_AOS_UPDATE aren't sent.
	//
	if ( (IsClientVer(0x400000) || IsNoCryptVer(0x400000)) && ( IsResClient( RDS_AOS ) && IsAosFlagEnabled(FEATURE_AOS_UPDATE_B) )) // IsResClient( RDS_AOS ) && g_Cfg.m_iFeatureAOS
	{
		// Handle new AOS spellbook stuff (old packets no longer work)

		CExtData data;
		data.NewSpellbook.m_Unk1 = 1;
		data.NewSpellbook.m_UID = pBook->GetUID();
		data.NewSpellbook.m_ItemId = pBook->GetDispID();
		data.NewSpellbook.m_Offset = offset; // 1 = normal, 101 = necro, 201 = paladin
#ifdef _WIN32
		data.NewSpellbook.m_Content0 = pBook->m_itSpellbook.m_spells1;
		data.NewSpellbook.m_Content1 = pBook->m_itSpellbook.m_spells2;
#else
		data.NewSpellbook.m_Content0 = ((pBook->m_itSpellbook.m_spells1 >> 24) & 0xFF)
									 | ((pBook->m_itSpellbook.m_spells1 >> 8 ) & 0xFF00)
									 | ((pBook->m_itSpellbook.m_spells1 << 8 ) & 0xFF0000)
									 | ((pBook->m_itSpellbook.m_spells1 << 24 ) & 0xFF000000);
		data.NewSpellbook.m_Content1 = ((pBook->m_itSpellbook.m_spells2 >> 24) & 0xFF)
									 | ((pBook->m_itSpellbook.m_spells2 >> 8 ) & 0xFF00)
									 | ((pBook->m_itSpellbook.m_spells2 << 8 ) & 0xFF0000)
									 | ((pBook->m_itSpellbook.m_spells2 << 24 ) & 0xFF000000);
#endif
		addExtData( EXTDATA_NewSpellbook, &data, sizeof( data.NewSpellbook ) );
		return;
	}

	if (!count)
		return;

	CCommand cmd;
	int len = sizeof( cmd.Content ) - sizeof(cmd.Content.m_item) + ( count * sizeof(cmd.Content.m_item[0]));
	cmd.Content.m_Cmd = XCMD_Content;
	cmd.Content.m_len = len;
	cmd.Content.m_count = count;

	int j=0;
	for ( int i = SPELL_Clumsy; i < SPELL_BOOK_QTY; i++ )
	{
		if ( ! pBook->IsSpellInBook( (SPELL_TYPE) i ) )
			continue;

		/*const CSpellDef * pSpellDef = g_Cfg.GetSpellDef( (SPELL_TYPE) i );
		ASSERT(pSpellDef); */

		cmd.Content.m_item[j].m_UID = UID_F_ITEM + UID_O_INDEX_FREE + i; // just some unused uid.
		cmd.Content.m_item[j].m_id = 0x1F2E; //pSpellDef->m_idScroll;	// scroll id. (0x1F2E)
		cmd.Content.m_item[j].m_zero6 = 0;
		cmd.Content.m_item[j].m_amount = i;
		cmd.Content.m_item[j].m_x = 0x0; // 0x48
		cmd.Content.m_item[j].m_y = 0x0; // 0x7D;
		cmd.Content.m_item[j].m_UIDCont = pBook->GetUID();
		cmd.Content.m_item[j].m_wHue = HUE_DEFAULT;
		j++;
	}

	xSendPkt( &cmd, len );
}


void CClient::addCustomSpellbookOpen( CItem * pBook, DWORD gumpID )
{
	ADDTOCALLSTACK("CClient::addCustomSpellbookOpen");
	const CItemContainer * pContainer = dynamic_cast <CItemContainer *> (pBook);
	CItem	* pItem;
	if ( !pContainer )
		return;

	int count=0;
	for ( pItem=pContainer->GetContentHead(); pItem!=NULL; pItem=pItem->GetNext() )
	{
		if ( !pItem->IsType( IT_SCROLL ) )
			continue;
		count++;
	}

	addOpenGump( pBook, (GUMP_TYPE) gumpID );
	if ( !count )
		return;

	CCommand cmd;
	int len = sizeof( cmd.Content ) - sizeof(cmd.Content.m_item) + ( count * sizeof(cmd.Content.m_item[0]));
	cmd.Content.m_Cmd = XCMD_Content;
	cmd.Content.m_len = len;
	cmd.Content.m_count = count;

	int j=0;
	for ( pItem=pContainer->GetContentHead(); pItem!=NULL; pItem=pItem->GetNext() )
	{
		if ( !pItem->IsType( IT_SCROLL ) )
			continue;

		const CSpellDef * pSpellDef = g_Cfg.GetSpellDef( (SPELL_TYPE) pItem->m_itSpell.m_spell );
		if ( !pSpellDef )
			continue;

		cmd.Content.m_item[j].m_UID	= pItem->GetUID();
		cmd.Content.m_item[j].m_id	= pSpellDef->m_idScroll;	// scroll id. (0x1F2E)
		cmd.Content.m_item[j].m_zero6	= 0;
		cmd.Content.m_item[j].m_amount	= pItem->m_itSpell.m_spell;
		cmd.Content.m_item[j].m_x = 0x0; // 0x48;
		cmd.Content.m_item[j].m_y = 0x0; // 0x7D;
		cmd.Content.m_item[j].m_UIDCont = pBook->GetUID();
		cmd.Content.m_item[j].m_wHue = HUE_DEFAULT;
		j++;
	}

	xSendPkt( &cmd, len );
}

void CClient::addScrollScript( CResourceLock &s, SCROLL_TYPE type, DWORD context, LPCTSTR pszHeader )
{
	ADDTOCALLSTACK("CClient::addScrollScript");
	CCommand cmd;
	cmd.Scroll.m_Cmd = XCMD_Scroll;
	cmd.Scroll.m_type = type;
	cmd.Scroll.m_context = context;	// for ScrollClose ?

	int length=0;

	if ( pszHeader )
	{
		length = strcpylen( &cmd.Scroll.m_text[0], pszHeader );
		cmd.Scroll.m_text[length++] = 0x0d;
		length += strcpylen( &cmd.Scroll.m_text[length], "  " );
		cmd.Scroll.m_text[length++] = 0x0d;
	}

	while (s.ReadKey(false))
	{
		length += strcpylen( &cmd.Scroll.m_text[length], s.GetKey());
		cmd.Scroll.m_text[length++] = 0x0d;
	}

	cmd.Scroll.m_lentext = length;
	length += sizeof( cmd.Scroll ) - sizeof( cmd.Scroll.m_text );
	cmd.Scroll.m_len = length;

	xSendPkt( &cmd, length );
}

void CClient::addScrollResource( LPCTSTR pszSec, SCROLL_TYPE type, DWORD scrollID )
{
	ADDTOCALLSTACK("CClient::addScrollResource");
	//
	// type = 0 = TIPS
	// type = 2 = UPDATES

	CResourceLock s;
	if ( ! g_Cfg.ResourceLock( s, RES_SCROLL, pszSec ))
		return;
	addScrollScript( s, type, scrollID );
}

void CClient::addVendorClose( const CChar * pVendor )
{
	ADDTOCALLSTACK("CClient::addVendorClose");
	// Clear the vendor display.
	CCommand cmd;
	cmd.VendorBuy.m_Cmd = XCMD_VendorBuy;
	cmd.VendorBuy.m_len = sizeof( cmd.VendorBuy );
	cmd.VendorBuy.m_UIDVendor = pVendor->GetUID();
	cmd.VendorBuy.m_flag = 0;	// 0x00 = no items following, 0x02 - items following
	xSendPkt( &cmd, sizeof( cmd.VendorBuy ));
}

int CClient::addShopItems(CChar * pVendor, LAYER_TYPE layer, bool bReal)
{
	ADDTOCALLSTACK("CClient::addShopItems");
	// Player buying from vendor.
	// Show the Buy menu for the contents of this container
	// RETURN: the number of items in container.
	//   < 0 = error.
	CItemContainer * pContainer = pVendor->GetBank( layer );
	if ( pContainer == NULL )
		return( -1 );

	addItem(pContainer);
	if ( bReal )
		addContents(pContainer, false, false, true);

	// add the names and prices for stuff.
	CCommand cmd;
	cmd.VendOpenBuy.m_Cmd = XCMD_VendOpenBuy;
	cmd.VendOpenBuy.m_VendorUID = pContainer->GetUID();
	int len = sizeof( cmd.VendOpenBuy ) - sizeof(cmd.VendOpenBuy.m_item);

	int iConvertFactor = pVendor->NPC_GetVendorMarkup(m_pChar );

	CCommand * pCur = &cmd;
	int count = 0;
	if ( bReal )
	{
		for ( CItem* pItem = pContainer->GetContentHead(); pItem!=NULL; pItem = pItem->GetNext())
		{
			if ( ! pItem->GetAmount() )
				continue;

			CItemVendable * pVendItem = dynamic_cast <CItemVendable *> (pItem);
			if ( pVendItem == NULL )
				continue;

			long lPrice = pVendItem->GetVendorPrice(iConvertFactor);
			if ( ! lPrice )
			{
				pVendItem->Item_GetDef()->ResetMakeValue();
				lPrice = pVendItem->GetVendorPrice(iConvertFactor);

				if ( !lPrice && pVendItem->IsValidNPCSaleItem() )
				{
					lPrice = pVendItem->GetBasePrice();
				}

				if ( !lPrice )
					lPrice = 100000;
			}

			pCur->VendOpenBuy.m_item[0].m_price = lPrice;

			int	lenname	= 0;
			lenname	+= strcpylen( pCur->VendOpenBuy.m_item[0].m_text + lenname, pVendItem->GetName() );

			pCur->VendOpenBuy.m_item[0].m_len = lenname + 1;
			lenname += sizeof( cmd.VendOpenBuy.m_item[0] );
			len += lenname;
			pCur = (CCommand *)( ((BYTE*) pCur ) + lenname );

			// Client will only accept (MAX_ITEMS_CONT-1) item descriptions, so no need
			// to add more to this packet.
			if ( ++count >= minimum(MAX_ITEMS_VENDOR,MAX_ITEMS_CONT) )
				break;
		}

		// Send a warning if the vendor somehow has more stock than the allowed limit
		if ( pContainer->GetCount() > minimum(MAX_ITEMS_VENDOR,MAX_ITEMS_CONT) )
		{
			g_Log.Event( LOGL_WARN, "Vendor 0%lx '%s' has exceeded their stock limit! (%d/%d items)\n", (DWORD) pVendor->GetUID(), (LPCTSTR) pVendor->GetName(), pContainer->GetCount(), minimum(MAX_ITEMS_VENDOR,MAX_ITEMS_CONT));
		}
	}
	cmd.VendOpenBuy.m_len = len;
	cmd.VendOpenBuy.m_count = count;
	xSendPkt( &cmd, len );
	return count;
}

bool CClient::addShopMenuBuy( CChar * pVendor )
{
	ADDTOCALLSTACK("CClient::addShopMenuBuy");
	// Try to buy stuff that the vendor has.
	if ( !pVendor || !pVendor->NPC_IsVendor() )
		return false;

	//	non-player vendors could be restocked on-the-fly
	if ( !pVendor->IsStatFlag(STATF_Pet) )
	{
		pVendor->NPC_Vendor_Restock(false, true);
	}

	addChar(pVendor);

	int iTotal = 0;
	int iRes = addShopItems(pVendor, LAYER_VENDOR_STOCK);
	if ( iRes < 0 )
		return false;
	iTotal += iRes;
	if ( iTotal <= 0 )
		return false;

		//	since older clients like 2.0.3 will crash without extra packets, let's provide
		//	some empty packets specialy for them
	addShopItems(pVendor, LAYER_VENDOR_EXTRA, false);

	addOpenGump( pVendor, GUMP_VENDOR_RECT );
	addCharStatWindow( m_pChar->GetUID());	// Make sure the gold total has been updated.
	return( true );
}

int CClient::addShopMenuSellFind( CItemContainer * pSearch, CItemContainer * pVend1, CItemContainer * pVend2, int iConvertFactor, CCommand * & pCur )
{
	ADDTOCALLSTACK("CClient::addShopMenuSellFind");
	// What things do you have in your inventory that the vendor would want ?
	// Search sub containers if necessary.
	// RETURN: How many items did we find.

	ASSERT(pSearch);
	int iCount = 0;

	CItem * pItem = pSearch->GetContentHead();
	if ( !pItem )
		return 0;

	CItemContainer * pSearchBox = pSearch;
	bool bSearch = true;

	std::deque<CItemContainer *> dOtherBox;

	while( bSearch )
	{
		if ( pItem != NULL )
		{
			CItemContainer * pContItem = dynamic_cast <CItemContainer*>(pItem);
			if ( pContItem != NULL && pContItem->GetCount() )
			{
				if ( pContItem->IsSearchable())
				{
					dOtherBox.push_back(pContItem);
				}
			}
			else
			{
				CItemVendable * pVendItem = dynamic_cast <CItemVendable *> (pItem);
				if ( pVendItem != NULL )
				{
					CItemVendable * pItemSell = CChar::NPC_FindVendableItem( pVendItem, pVend1, pVend2 );
					if ( pItemSell != NULL )
					{
						pCur->VendOpenSell.m_item[0].m_UID = pVendItem->GetUID();
						pCur->VendOpenSell.m_item[0].m_id = pVendItem->GetDispID();

						HUE_TYPE wHue = pVendItem->GetHue() & HUE_MASK_HI;
						if ( wHue > HUE_QTY )
							wHue &= HUE_MASK_LO;

						pCur->VendOpenSell.m_item[0].m_wHue = wHue;
						pCur->VendOpenSell.m_item[0].m_amount = pVendItem->GetAmount();
						pCur->VendOpenSell.m_item[0].m_price = pItemSell->GetVendorPrice(iConvertFactor);

						int lenname = strcpylen( pCur->VendOpenSell.m_item[0].m_text, pVendItem->GetName());
						pCur->VendOpenSell.m_item[0].m_len = lenname + 1;
						pCur = (CCommand *)( ((BYTE*) pCur ) + lenname + sizeof( pCur->VendOpenSell.m_item[0] ));
						addAOSTooltip( pVendItem, true );
						if ( ++iCount >= MAX_ITEMS_CONT )
						{
							break;
						}
					}
				}
			}
		}

		pItem = pItem->GetNext();
		if ( pItem == NULL )
		{
			if ( ! dOtherBox.empty() )
			{
				pSearch = dOtherBox.front();
				dOtherBox.pop_front();
				pItem = pSearch->GetContentHead();
			}
			else
			{
				bSearch = false;
			}
		}
	}

	return( iCount );
}

bool CClient::addShopMenuSell( CChar * pVendor )
{
	ADDTOCALLSTACK("CClient::addShopMenuSell");
	// Player selling to vendor.
	// What things do you have in your inventory that the vendor would want ?
	// Should end with a returned Event_VendorSell()

	if ( !pVendor || !pVendor->NPC_IsVendor() )
		return false;

	//	non-player vendors could be restocked on-the-fly
	if ( !pVendor->IsStatFlag(STATF_Pet) )
	{
		pVendor->NPC_Vendor_Restock(false, true);
	}

	int iConvertFactor		= - pVendor->NPC_GetVendorMarkup( m_pChar );

	// TODO: Here we should add the container empty, not required to send items.
	CItemContainer * pContainer1 = pVendor->GetBank( LAYER_VENDOR_BUYS );
	addItem( pContainer1 );
	CItemContainer * pContainer2 = pVendor->GetBank( LAYER_VENDOR_STOCK );
	addItem( pContainer2 );

	if ( IsClientLessVer(0x300000) || IsNoCryptLessVer(0x300000) )
	{
		// This avoid client crashes
		CItemContainer * pContainer3 = pVendor->GetBank( LAYER_VENDOR_EXTRA );
		addItem( pContainer3 );
	}

	if ( pVendor->IsStatFlag( STATF_Pet ))	// Player vendor.
	{
		pContainer2 = NULL; // no stock
	}

	CCommand cmd;
	cmd.VendOpenSell.m_Cmd = XCMD_VendOpenSell;
	cmd.VendOpenSell.m_UIDVendor = pVendor->GetUID();

	CCommand * pCur = (CCommand *)((BYTE*) &cmd );
	int iCount = addShopMenuSellFind( m_pChar->GetPackSafe(), pContainer1, pContainer2, iConvertFactor, pCur );
	if ( ! iCount )
		return( false );

	cmd.VendOpenSell.m_len = (((BYTE*)pCur) - ((BYTE*) &cmd )) + sizeof( cmd.VendOpenSell ) - sizeof(cmd.VendOpenSell.m_item);
	cmd.VendOpenSell.m_count = iCount;

	xSendPkt( &cmd, cmd.VendOpenSell.m_len );
	return( true );
}

void CClient::addBankOpen( CChar * pChar, LAYER_TYPE layer )
{
	ADDTOCALLSTACK("CClient::addBankOpen");
	// open it up for this pChar.
	ASSERT( pChar );

	CItemContainer * pBankBox = pChar->GetBank(layer);
	ASSERT(pBankBox);
	addItem( pBankBox );	// may crash client if we dont do this.

	if ( pChar != GetChar())
	{
		// xbank verb on others needs this ?
		// addChar( pChar );
	}

	pBankBox->OnOpenEvent( m_pChar, pChar );
	addContainerSetup( pBankBox );
}

void CClient::addMap( CItemMap * pMap )
{
	ADDTOCALLSTACK("CClient::addMap");
	// Make player drawn maps possible. (m_map_type=0) ???

	if ( pMap == NULL )
	{
blank_map:
		addSysMessage( g_Cfg.GetDefaultMsg(DEFMSG_MAP_IS_BLANK) );
		return;
	}
	if ( pMap->IsType(IT_MAP_BLANK))
		goto blank_map;

	CRectMap rect;
	rect.SetRect( pMap->m_itMap.m_left,
		pMap->m_itMap.m_top,
		pMap->m_itMap.m_right,
		pMap->m_itMap.m_bottom,
		GetChar()->GetTopPoint().m_map);

	if ( ! rect.IsValid())
		goto blank_map;
	if ( rect.IsRectEmpty())
		goto blank_map;

	CCommand cmd;
	cmd.MapDisplay.m_Cmd = XCMD_MapDisplay;
	cmd.MapDisplay.m_UID = pMap->GetUID();
	cmd.MapDisplay.m_Gump_Corner = GUMP_MAP_2_NORTH;
	cmd.MapDisplay.m_x_ul = rect.m_left;
	cmd.MapDisplay.m_y_ul = rect.m_top;
	cmd.MapDisplay.m_x_lr = rect.m_right;
	cmd.MapDisplay.m_y_lr = rect.m_bottom;
	cmd.MapDisplay.m_xsize = 0xc8;	// ??? we could make bigger maps ?
	cmd.MapDisplay.m_ysize = 0xc8;
	xSendPkt( &cmd, sizeof( cmd.MapDisplay ));

	addMapMode( pMap, MAP_UNSENT, false );

	// Now show all the pins
	cmd.MapEdit.m_Cmd = XCMD_MapEdit;
	cmd.MapEdit.m_UID = pMap->GetUID();
	cmd.MapEdit.m_Mode = 0x1;	// MAP_PIN?
	cmd.MapEdit.m_Req = 0x00;

	for ( int i=0; i < pMap->m_Pins.GetCount(); i++ )
	{
		cmd.MapEdit.m_pin_x = pMap->m_Pins[i].m_x;
		cmd.MapEdit.m_pin_y = pMap->m_Pins[i].m_y;
		xSendPkt( &cmd, sizeof( cmd.MapEdit ));
	}
}

void CClient::addMapMode( CItemMap * pMap, MAPCMD_TYPE iType, bool fEdit )
{
	ADDTOCALLSTACK("CClient::addMapMode");
	// NOTE: MAPMODE_* depends on who is looking. Multi clients could interfere with each other ?
	if ( !pMap )
		return;

	pMap->m_fPlotMode = fEdit;

	CCommand cmd;
	cmd.MapEdit.m_Cmd = XCMD_MapEdit;
	cmd.MapEdit.m_UID = pMap->GetUID();
	cmd.MapEdit.m_Mode = iType;
	cmd.MapEdit.m_Req = fEdit;
	cmd.MapEdit.m_pin_x = 0;
	cmd.MapEdit.m_pin_y = 0;
	xSendPkt( &cmd, sizeof( cmd.MapEdit ));
}

void CClient::addBulletinBoard( const CItemContainer * pBoard )
{
	ADDTOCALLSTACK("CClient::addBulletinBoard");
	// Open up the bulletin board and all it's messages
	// Event_BBoardRequest
	if ( !pBoard )
		return;

	CCommand cmd;

	// Give the bboard name.
	cmd.BBoard.m_Cmd = XCMD_BBoard;
	int len = strcpylen( (TCHAR *) cmd.BBoard.m_data, pBoard->GetName(), MAX_ITEM_NAME_SIZE );
	len += sizeof(cmd.BBoard);
	cmd.BBoard.m_len = len;
	cmd.BBoard.m_flag = BBOARDF_NAME;
	cmd.BBoard.m_UID = pBoard->GetUID(); // 4-7 = UID for the bboard.
	xSendPkt( &cmd, len );

	// Send Content messages for all the items on the bboard.
	// Not sure what x,y are here, date/time maybe ?
	addContents( pBoard, false, false, false );

	// The client will now ask for the headers it wants.
}

bool CClient::addBBoardMessage( const CItemContainer * pBoard, BBOARDF_TYPE flag, CGrayUID uidMsg )
{
	ADDTOCALLSTACK("CClient::addBBoardMessage");
	ASSERT(pBoard);

	CItemMessage * pMsgItem = dynamic_cast <CItemMessage *> ( uidMsg.ItemFind());
	if ( ! pBoard->IsItemInside( pMsgItem ))
		return( false );

	// Send back the message header and/or body.
	CCommand cmd;
	cmd.BBoard.m_Cmd = XCMD_BBoard;
	cmd.BBoard.m_flag = ( flag == BBOARDF_REQ_FULL ) ? BBOARDF_MSG_BODY : BBOARDF_MSG_HEAD;
	cmd.BBoard.m_UID = pBoard->GetUID();	// 4-7 = UID for the bboard.

	int len = 4;
	PACKDWORD(cmd.BBoard.m_data+0,pMsgItem->GetUID());

	if ( flag == BBOARDF_REQ_HEAD )
	{
		// just the header has this ? (replied to message?)
		PACKDWORD(cmd.BBoard.m_data+4,0);
		len += 4;
	}

	// author name. if it has one.
	if ( pMsgItem->m_sAuthor.IsEmpty())
	{
		cmd.BBoard.m_data[len++] = 0x01;
		cmd.BBoard.m_data[len++] = 0;
	}
	else
	{
		CChar * pChar = pMsgItem->m_uidLink.CharFind();
		if ( pChar == NULL )	// junk it if bad author. (deleted?)
		{
			pMsgItem->Delete();
			return false;
		}
		LPCTSTR pszAuthor = pMsgItem->m_sAuthor;
		if ( IsPriv(PRIV_GM))
		{
			pszAuthor = m_pChar->GetName();
		}
		int lenstr = strlen(pszAuthor) + 1;
		cmd.BBoard.m_data[len++] = lenstr;
		strcpy( (TCHAR*) &cmd.BBoard.m_data[len], pszAuthor);
		len += lenstr;
	}

	// Pad this out with spaces to indent next field.
	int lenstr = strlen( pMsgItem->GetName()) + 1;
	cmd.BBoard.m_data[len++] = lenstr;
	strcpy( (TCHAR*) &cmd.BBoard.m_data[len], pMsgItem->GetName());
	len += lenstr;

	// Get the BBoard message time stamp. m_itBook.m_Time
	CServTime time;
	time = pMsgItem->m_itBook.m_Time;

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "Day %d", ( g_World.GetGameWorldTime(time) / (24*60)) % 365 );
	lenstr = strlen(pszTemp) + 1;
	cmd.BBoard.m_data[len++] = lenstr;
	strcpy((TCHAR*)&cmd.BBoard.m_data[len], pszTemp);
	len += lenstr;

	if ( flag == BBOARDF_REQ_FULL )
	{
		// request for full message body
		//
		PACKDWORD(&(cmd.BBoard.m_data[len]),0);
		len += 4;

		// Pack the text into seperate lines.
		int lines = pMsgItem->GetPageCount();

		// number of lines.
		PACKWORD(&(cmd.BBoard.m_data[len]),lines);
		len += 2;

		// Now pack all the lines
		for ( int i=0; i<lines; i++ )
		{
			LPCTSTR pszText = pMsgItem->GetPageText(i);
			if ( pszText == NULL )
				continue;
			lenstr = strlen(pszText)+2;
			cmd.BBoard.m_data[len++] = lenstr;
			strcpy( (TCHAR*) &cmd.BBoard.m_data[len], pszText );
			len += lenstr;
		}
	}

	len = sizeof( cmd.BBoard ) - sizeof( cmd.BBoard.m_data ) + len;
	cmd.BBoard.m_len = len;
	xSendPkt( &cmd, len );
	return( true );
}

void CClient::addPing( BYTE bCode )
{
	ADDTOCALLSTACK("CClient::addPing");
	if ( ! IsConnectTypePacket())
	{
		// Not sure how to ping anything but a game client.
		return;
	}
	// Should normally be in response to a client ping packet, where
	// m_bCode matches what the client sent
	CCommand cmd;
	cmd.Ping.m_Cmd = XCMD_Ping;
	cmd.Ping.m_bCode = bCode;
	xSendPkt( &cmd, sizeof(cmd.Ping));
}

void CClient::addRedrawAll()
{
	ADDTOCALLSTACK("CClient::addRedrawAll");
	CCommand cmd;
	cmd.ReDrawAll.m_Cmd = XCMD_ReDrawAll;
	xSendPkt( &cmd, sizeof(cmd.ReDrawAll));
}

void CClient::addChatSystemMessage( CHATMSG_TYPE iType, LPCTSTR pszName1, LPCTSTR pszName2, CLanguageID lang )
{
	ADDTOCALLSTACK("CClient::addChatSystemMessage");
	CCommand cmd;
	cmd.ChatReq.m_Cmd = XCMD_ChatReq;
	cmd.ChatReq.m_subcmd = iType;

	if ( iType >= CHATMSG_PlayerTalk && iType <= CHATMSG_PlayerPrivate) // These need the language stuff
		lang.GetStrDef( cmd.ChatReq.m_lang ); // unicode support: pszLang
	else
		memset( cmd.ChatReq.m_lang, 0, sizeof(cmd.ChatReq.m_lang));

	// Convert internal UTF8 to UNICODE for client.
	// ? If we're sending out player names, prefix name with moderator status

	if ( pszName1 == NULL )
		pszName1 = "";
	int len1 = CvtSystemToNUNICODE( cmd.ChatReq.m_uname, MAX_TALK_BUFFER, pszName1, -1 );

	if ( pszName2 == NULL )
		pszName2 = "";
	int len2 = CvtSystemToNUNICODE( cmd.ChatReq.m_uname+len1+1, MAX_TALK_BUFFER, pszName2, -1 );

	int len = sizeof(cmd.ChatReq) + (len1*2) + (len2*2);
	cmd.ChatReq.m_len = len;
	xSendPkt( &cmd, len );
}

void CClient::addGumpTextDisp( const CObjBase * pObj, GUMP_TYPE gump, LPCTSTR pszName, LPCTSTR pszText )
{
	ADDTOCALLSTACK("CClient::addGumpTextDisp");
	// ??? how do we control where exactly the text goes ??

	// display a gump with some text on it.
	int lenname = pszName ? strlen( pszName ) : 0 ;
	lenname++;
	int lentext = pszText ? strlen( pszText ) : 0 ;
	lentext++;

	CCommand cmd;

	int len = ( sizeof(cmd.GumpTextDisp) - 2 ) + lenname + lentext;

	cmd.GumpTextDisp.m_Cmd = XCMD_GumpTextDisp;
	cmd.GumpTextDisp.m_len = len;
	cmd.GumpTextDisp.m_UID = pObj ? ((DWORD)( pObj->GetUID())) : UID_CLEAR;
	cmd.GumpTextDisp.m_gump = gump;
	cmd.GumpTextDisp.m_len_unktext = lenname;
	cmd.GumpTextDisp.m_unk11 = 10;	// ? not HUE_TYPE, not x,
	strcpy( cmd.GumpTextDisp.m_unktext, ( pszName ) ? pszName : "" );

	CCommand * pCmd = (CCommand *)(((BYTE*)(&cmd))+lenname-1);
	strcpy( pCmd->GumpTextDisp.m_text, ( pszText ) ? pszText : "" );
	xSendPkt( &cmd, len );
}

void CClient::addItemMenu( CLIMODE_TYPE mode, const CMenuItem * item, int count, CObjBase * pObj )
{
	ADDTOCALLSTACK("CClient::addItemMenu");
	// We must set GetTargMode() to show what mode we are in for menu select.
	// Will result in Event_MenuChoice()
	// cmd.ItemMenu.

	if ( ! count )
		return;
	if ( pObj == NULL )
		pObj = m_pChar;

	CCommand cmd;
	cmd.MenuItems.m_Cmd = XCMD_MenuItems;
	cmd.MenuItems.m_UID = pObj->GetUID();
	cmd.MenuItems.m_context = mode;

	int len = sizeof( cmd.MenuItems ) - sizeof( cmd.MenuItems.m_item );
	int lenttitle = item[0].m_sText.GetLength();
	cmd.MenuItems.m_lenname = lenttitle;
	strcpy( cmd.MenuItems.m_name, item[0].m_sText );

	lenttitle --;
	len += lenttitle;
	CCommand * pCmd = (CCommand *)(((BYTE*)&cmd) + lenttitle );
	pCmd->MenuItems.m_count = count;

	// Strings in here and NOT null terminated.
	for ( int i=1; i<=count; i++ )
	{
		int lenitem = item[i].m_sText.GetLength();
		if ( lenitem <= 0 || lenitem >= 256 )
		{
			DEBUG_ERR(("Bad option length %d in menu item %d\n", lenitem, i ));
			continue;
		}

		pCmd->MenuItems.m_item[0].m_id = item[i].m_id; // image next to menu.
		pCmd->MenuItems.m_item[0].m_color = item[i].m_color;	// color of the image
		pCmd->MenuItems.m_item[0].m_lentext = lenitem;
		strcpy( pCmd->MenuItems.m_item[0].m_name, item[i].m_sText );

		lenitem += sizeof( cmd.MenuItems.m_item[0] ) - 1;
		pCmd = (CCommand *)(((BYTE*)pCmd) + lenitem );
		len += lenitem;
	}

	cmd.MenuItems.m_len = len;
	xSendPkt( &cmd, len );

	m_tmMenu.m_UID = pObj->GetUID();

	SetTargMode( mode );
}


bool CClient::addWalkCode( EXTDATA_TYPE iType, int iCodes )
{
	ADDTOCALLSTACK("CClient::addWalkCode");
	// Fill up the walk code buffer.
	// RETURN: true = new codes where sent.

	if ( ! m_Crypt.IsInit() )	// This is not even a game client ! IsConnectTypePacket()
		return false;

	if ( IsNoCryptLessVer(0x126000) )
		return( false );

	if ( IsClientLessVer( 0x126000 ) )
		return false;

	if ( ! ( g_Cfg.m_wDebugFlags & DEBUGF_WALKCODES ))
		return( false );

	if ( iType == EXTDATA_WalkCode_Add )
	{
		if ( m_Walk_InvalidEchos >= 0 )
			return false;					// they are stuck til they give a valid echo!
		// On a timer tick call this.
		if ( m_Walk_CodeQty >= COUNTOF(m_Walk_LIFO))	// They are appearently not moving fast.
			return false;
	}
	else
	{
		// Fill the buffer at start.
		ASSERT( m_Walk_CodeQty < 0 );
		m_Walk_CodeQty = 0;
	}

	ASSERT( iCodes <= COUNTOF(m_Walk_LIFO));

	// make a new code and send it out
	CExtData ExtData;
	int i;
	for ( i=0; i < iCodes && m_Walk_CodeQty < COUNTOF(m_Walk_LIFO); m_Walk_CodeQty++, i++ )
	{
		DWORD dwCode = 0x88ca0000 + Calc_GetRandVal( 0xffff );
		m_Walk_LIFO[m_Walk_CodeQty] = dwCode;
		ExtData.WalkCode[i] = dwCode;
	}

	addExtData( iType, &ExtData, i*sizeof(DWORD));
	return( true );
}

void CClient::addCharPaperdoll( CChar * pChar )
{
	ADDTOCALLSTACK("CClient::addCharPaperdoll");
	if ( !pChar )
		return;

	CCommand cmd;
	cmd.PaperDoll.m_Cmd = XCMD_PaperDoll;
	cmd.PaperDoll.m_UID = pChar->GetUID();

	if ( pChar->IsStatFlag( STATF_Incognito ))
	{
		strcpy( cmd.PaperDoll.m_text, pChar->GetName());
	}
	else
	{
		int len = 0;
		CStoneMember * pGuildMember = pChar->Guild_FindMember(MEMORY_GUILD);
		if ( pGuildMember && pGuildMember->IsAbbrevOn() && pGuildMember->GetParentStone()->GetAbbrev()[0] )
		{
			len = sprintf( cmd.PaperDoll.m_text, "%s [%s], %s",
				pChar->Noto_GetTitle(), pGuildMember->GetParentStone()->GetAbbrev(),
				pGuildMember->GetTitle()[0] ? pGuildMember->GetTitle() : pChar->GetTradeTitle());
		}
		if ( ! len )
		{
			sprintf( cmd.PaperDoll.m_text, "%s, %s", pChar->Noto_GetTitle(), pChar->GetTradeTitle());
		}
	}

	unsigned char mode = 0;
	if ( pChar->IsStatFlag( STATF_War ) )
		mode |= 0x1;
	if ( pChar == m_pChar || (m_pChar->IsPriv( PRIV_GM ) && ( m_pChar->GetPrivLevel() > pChar->GetPrivLevel() )) )
		mode |= 0x2;

	cmd.PaperDoll.m_text[ sizeof(cmd.PaperDoll.m_text)-1 ] = '\0';
	cmd.PaperDoll.m_mode = mode;	// 0=normal, 0x1=warmode, 0x2=canunequip

	xSendPkt( &cmd, sizeof( cmd.PaperDoll ));
}

void CClient::addAOSTooltip( const CObjBase * pObj, bool bShop )
{
	ADDTOCALLSTACK("CClient::addAOSTooltip");
	if ( !pObj )
		return;

	if ( !IsClientVersion(0x400000) )
		return;

	if (!IsResClient(RDS_AOS))
		return;

	bool bNameOnly = false;
	if (!IsAosFlagEnabled(FEATURE_AOS_UPDATE_B))
	{
		if ( !bShop )
			return;

		// shop items use tooltips whether they're disabled or not,
		// so we can just send a basic tooltip with the item name
		bNameOnly = true;
	}

	//DEBUG_MSG(("(( m_pChar->GetTopPoint().GetDist(pObj->GetTopPoint()) (%x) > UO_MAP_VIEW_SIZE (%x) ) && ( !bShop ) (%x) )", m_pChar->GetTopPoint().GetDist(pObj->GetTopPoint()), UO_MAP_VIEW_SIZE, ( !bShop )));
	if (( m_pChar->GetTopPoint().GetDist(pObj->GetTopPoint()) > UO_MAP_VIEW_SIZE ) && ( m_pChar->GetTopPoint().GetDist(pObj->GetTopPoint()) <= UO_MAP_VIEW_RADAR ) && ( !bShop ) ) //we do not need to send tooltips for items not in LOS (multis/ships)
		return;

	// We check here if we are sending a tooltip for a static/non-movable items
	// (client doesn't expect us to) but only in the world
	if ( pObj->IsItem() )
	{
		CItem * pItem = (CItem *) dynamic_cast <const CItem *> ( pObj );

		if ( !pItem->GetContainer() && pItem->IsAttr(/*ATTR_MOVE_NEVER|*/ATTR_STATIC) )
		{
			if ( ( ! this->GetChar()->IsPriv( PRIV_GM ) ) && ( ! this->GetChar()->IsPriv( PRIV_ALLMOVE ) ) )
				return;
		}
	}

#define DOHASH( value ) hash ^= ((value) & 0x3FFFFFF); \
						hash ^= ((value) >> 26) & 0x3F;

	int hash = 0;
	DOHASH( (pObj->GetUID() & UID_O_INDEX_MASK) );
	DOHASH( pObj->GetBaseID() );
	DOHASH( (Calc_GetRandVal( pObj->GetBaseID() ) + 1) );

	CItem	*pItem = ( pObj->IsItem() ? (CItem *) dynamic_cast <const CItem *> (pObj) : NULL );
	CChar	*pChar = ( pObj->IsChar() ? (CChar *) dynamic_cast <const CChar *> (pObj) : NULL );
	CClientTooltip	*t;

	this->m_TooltipData.Clean(true);

	//DEBUG_MSG(("Preparing tooltip for 0%x (%s)\n", pObj->GetUID(), pObj->GetName()));

	if (bNameOnly) // if we only want to display the name (FEATURE_AOS_UPDATE_B disabled)
	{
		this->m_TooltipData.InsertAt(0, t = new CClientTooltip(1050045));
		t->FormatArgs(" \t%s\t ", pObj->GetName());
	}
	else // we have FEATURE_AOS_UPDATE_B enabled
	{
		TRIGRET_TYPE iRet = TRIGRET_RET_FALSE;
		CScriptTriggerArgs args((CScriptObj *)pObj);
		if ( pItem )
			iRet = pItem->OnTrigger(ITRIG_CLIENTTOOLTIP, this->GetChar(), &args);
		else if ( pChar )
			iRet = pChar->OnTrigger(CTRIG_ClientTooltip, this->GetChar(), &args);

		if ( iRet != TRIGRET_RET_TRUE )
		{
			if ( pItem )
			{
				this->m_TooltipData.InsertAt(0, t = new CClientTooltip(1050045));
				t->FormatArgs(" \t%s\t ", pObj->GetName()); // ~1_PREFIX~~2_NAME~~3_SUFFIX~
			}
			else if ( pChar )
			{
				LPCTSTR lpPrefix = pChar->GetKeyStr("NAME.PREFIX");
				// HUE_TYPE wHue = m_pChar->Noto_GetHue( pChar, true );

				if ( ! *lpPrefix )
					lpPrefix = pChar->Noto_GetFameTitle();

				if ( ! *lpPrefix )
					lpPrefix = " ";

				TCHAR * lpSuffix = Str_GetTemp();
				strcpy(lpSuffix, pChar->GetKeyStr("NAME.SUFFIX"));

				CStoneMember * pGuildMember = pChar->Guild_FindMember(MEMORY_GUILD);
				if ( !pChar->IsStatFlag(STATF_Incognito) || ( GetPrivLevel() > pChar->GetPrivLevel() ))
				{
					if ( pGuildMember && pGuildMember->IsAbbrevOn() && pGuildMember->GetParentStone()->GetAbbrev()[0] )
					{
						sprintf( lpSuffix, "%s [%s]", lpSuffix, pGuildMember->GetParentStone()->GetAbbrev() );
					}
				}

				if ( *lpSuffix == '\0' )
					strcpy( lpSuffix, " " );

				// The name
				this->m_TooltipData.InsertAt(0, t = new CClientTooltip(1050045));
				t->FormatArgs("%s\t%s\t%s", lpPrefix, pObj->GetName(), lpSuffix); // ~1_PREFIX~~2_NAME~~3_SUFFIX~

				// Need to find a way to get the ushort inside hues.mul for index wHue to get this working.
				// t->FormatArgs("<basefont color=\"#%02x%02x%02x\">%s\t%s\t%s</basefont>",
				//	(BYTE)((((int)wHue) & 0x7C00) >> 7), (BYTE)((((int)wHue) & 0x3E0) >> 2),
				//	(BYTE)((((int)wHue) & 0x1F) << 3),lpPrefix, pObj->GetName(), lpSuffix); // ~1_PREFIX~~2_NAME~~3_SUFFIX~

				if ( !pChar->IsStatFlag(STATF_Incognito) || ( GetPrivLevel() > pChar->GetPrivLevel() ))
				{
					if ( pGuildMember && pGuildMember->IsAbbrevOn() )
					{
						if ( pGuildMember->GetTitle()[0] )
						{
							this->m_TooltipData.Add(t = new CClientTooltip(1060776));
							t->FormatArgs( "%s\t%s", pGuildMember->GetTitle(), pGuildMember->GetParentStone()->GetName()); // ~1_val~, ~2_val~
						}
						else
						{
							this->m_TooltipData.Add(new CClientTooltip(1070722, pGuildMember->GetParentStone()->GetName())); // ~1_NOTHING~
						}
					}
				}
			}


			// Some default tooltip info if RETURN 0 or no script
			if ( pChar )
			{
				// Character specific stuff
				if ( ( pChar->IsPriv( PRIV_GM ) ) && ( ! pChar->IsPriv( PRIV_PRIV_NOSHOW ) ) )
					this->m_TooltipData.Add( new CClientTooltip( 1018085 ) ); // Game Master
			}

			if ( pItem )
			{
				if ( pItem->IsAttr( ATTR_BLESSED ) )
					this->m_TooltipData.Add( new CClientTooltip( 1038021 ) ); // Blessed
				if ( pItem->IsAttr( ATTR_CURSED ) )
					this->m_TooltipData.Add( new CClientTooltip( 1049643 ) ); // Cursed
				if ( pItem->IsAttr( ATTR_NEWBIE ) )
					this->m_TooltipData.Add( new CClientTooltip( 1070722, "Newbie" ) ); // ~1_NOTHING~
				if ( pItem->IsAttr( ATTR_MAGIC ) )
					this->m_TooltipData.Add( new CClientTooltip( 3010064 ) ); // Magic

				if ( ( pItem->GetAmount() != 1 ) && ( pItem->GetType() != IT_CORPSE ) ) // Negative amount?
				{
					this->m_TooltipData.Add( t = new CClientTooltip( 1060663 ) ); // ~1_val~: ~2_val~
					t->FormatArgs( "Amount\t%d", pItem->GetAmount() );
				}

				// Some type specific default stuff
				switch ( pItem->GetType() )
				{
					case IT_CONTAINER_LOCKED:
						this->m_TooltipData.Add( new CClientTooltip( 3005142 ) ); // Locked
					case IT_CONTAINER:
					case IT_CORPSE:
					case IT_TRASH_CAN:
						if ( pItem->IsContainer() )
						{
							CContainer * pContainer = dynamic_cast <CContainer *> ( pItem );
							this->m_TooltipData.Add( t = new CClientTooltip( 1050044 ) );
							t->FormatArgs( "%d\t%d", pContainer->GetCount(), pContainer->GetTotalWeight() ); // ~1_COUNT~ items, ~2_WEIGHT~ stones
						}
						break;

					case IT_ARMOR_LEATHER:
					case IT_ARMOR:
					case IT_CLOTHING:
					case IT_SHIELD:
						this->m_TooltipData.Add( t = new CClientTooltip( 1060658 ) ); // ~1_val~: ~2_val~
						t->FormatArgs( "Armor\t%d", pItem->Armor_GetDefense() );
						this->m_TooltipData.Add( t = new CClientTooltip( 1061170 ) ); // strength requirement ~1_val~
						t->FormatArgs( "%d", pItem->Item_GetDef()->m_ttEquippable.m_StrReq );
						this->m_TooltipData.Add( t = new CClientTooltip( 1060639 ) ); // durability ~1_val~ / ~2_val~
						t->FormatArgs( "%d\t%d", pItem->m_itArmor.m_Hits_Cur, pItem->m_itArmor.m_Hits_Max );
						break;

					case IT_WEAPON_MACE_SMITH:
					case IT_WEAPON_MACE_SHARP:
					case IT_WEAPON_MACE_STAFF:
					case IT_WEAPON_MACE_CROOK:
					case IT_WEAPON_SWORD:
					case IT_WEAPON_FENCE:
					case IT_WEAPON_BOW:
					case IT_WAND:
					case IT_WEAPON_AXE:
					case IT_WEAPON_XBOW:
						if ( pItem->GetType() == IT_WAND )
						{
							this->m_TooltipData.Add( t = new CClientTooltip( 1054132 ) ); // [charges: ~1_charges~]
							t->FormatArgs( "%d", pItem->m_itWeapon.m_spellcharges );
						}
						else
						{
							if ( pItem->Item_GetDef()->GetEquipLayer() == LAYER_HAND2 )
								this->m_TooltipData.Add( new CClientTooltip( 1061171 ) );
							else
								this->m_TooltipData.Add( new CClientTooltip( 1061824 ) );
						}

						this->m_TooltipData.Add( t = new CClientTooltip( 1061168 ) ); // weapon damage ~1_val~ - ~2_val~
						t->FormatArgs( "%d\t%d", pItem->Item_GetDef()->m_attackBase, ( pItem->Weapon_GetAttack(true) ) );
						this->m_TooltipData.Add( t = new CClientTooltip( 1061170 ) ); // strength requirement ~1_val~
						t->FormatArgs( "%d", pItem->Item_GetDef()->m_ttEquippable.m_StrReq );
						this->m_TooltipData.Add( t = new CClientTooltip( 1060639 ) ); // durability ~1_val~ / ~2_val~
						t->FormatArgs( "%d\t%d", pItem->m_itWeapon.m_Hits_Cur, pItem->m_itWeapon.m_Hits_Max );

						if ( pItem->m_itWeapon.m_poison_skill )
							this->m_TooltipData.Add( new CClientTooltip( 1017383 ) ); // Poisoned
						break;

					case IT_WEAPON_MACE_PICK:
						this->m_TooltipData.Add( t = new CClientTooltip( 1060639 ) ); // durability ~1_val~ / ~2_val~
						t->FormatArgs( "%d\t%d", pItem->m_itWeapon.m_Hits_Cur, pItem->m_itWeapon.m_Hits_Max );
						break;

					case IT_TELEPAD:
					case IT_MOONGATE:
						if ( this->IsPriv( PRIV_GM ) )
						{
							this->m_TooltipData.Add( t = new CClientTooltip( 1060658 ) ); // ~1_val~: ~2_val~
							t->FormatArgs( "Destination\t%s", pItem->m_itTelepad.m_pntMark.WriteUsed() );
						}
						break;

					case IT_BOOK:
						// Author?
						break;

					case IT_SPELLBOOK:
					case IT_SPELLBOOK_NECRO:
					case IT_SPELLBOOK_PALA:
					case IT_SPELLBOOK_BUSHIDO:
					case IT_SPELLBOOK_NINJITSU:
					case IT_SPELLBOOK_ARCANIST:
						{
							int count = pItem->GetSpellcountInBook();
							if ( count > 0 )
							{
								this->m_TooltipData.Add( t = new CClientTooltip( 1042886 ) ); // ~1_NUMBERS_OF_SPELLS~ Spells
								t->FormatArgs( "%d", count );
							}
						} break;

					case IT_SPAWN_CHAR:
						{
							CResourceDef * pSpawnCharDef = g_Cfg.ResourceGetDef( pItem->m_itSpawnChar.m_CharID );
							this->m_TooltipData.Add( t = new CClientTooltip( 1060658 ) ); // ~1_val~: ~2_val~
							t->FormatArgs( "Character\t%s", pSpawnCharDef ? pSpawnCharDef->GetName() : "none" );
							this->m_TooltipData.Add( t = new CClientTooltip( 1061169 ) ); // range ~1_val~
							t->FormatArgs( "%d", pItem->m_itSpawnChar.m_DistMax );
							this->m_TooltipData.Add( t = new CClientTooltip( 1074247 ) );
							t->FormatArgs( "%d\t%d", pItem->m_itSpawnChar.m_current, pItem->GetAmount() );
							this->m_TooltipData.Add( t = new CClientTooltip( 1060659 ) ); // ~1_val~: ~2_val~
							t->FormatArgs( "Min/max time\t%d min / %d min", pItem->m_itSpawnChar.m_TimeLoMin, pItem->m_itSpawnChar.m_TimeHiMin );
							this->m_TooltipData.Add( t = new CClientTooltip( 1060660 ) ); // ~1_val~: ~2_val~
							t->FormatArgs( "Time until next spawn\t%d sec", pItem->GetTimerAdjusted() );
						} break;

					case IT_SPAWN_ITEM:
						{
							CResourceDef * pSpawnItemDef = g_Cfg.ResourceGetDef( pItem->m_itSpawnItem.m_ItemID );
							this->m_TooltipData.Add( t = new CClientTooltip( 1060658 ) ); // ~1_val~: ~2_val~
							t->FormatArgs( "Item\t%s", pSpawnItemDef ? pSpawnItemDef->GetName() : "none" );
							this->m_TooltipData.Add( t = new CClientTooltip( 1060656 ) ); // amount to make: ~1_val~
							t->FormatArgs( "%d", pItem->m_itSpawnItem.m_pile );
							this->m_TooltipData.Add( t = new CClientTooltip( 1074247 ) );
							t->FormatArgs( "??\t%d", pItem->GetAmount() );
							this->m_TooltipData.Add( t = new CClientTooltip( 1060659 ) ); // ~1_val~: ~2_val~
							t->FormatArgs( "Min/max time\t%d min / %d min", pItem->m_itSpawnItem.m_TimeLoMin, pItem->m_itSpawnItem.m_TimeHiMin );
							this->m_TooltipData.Add( t = new CClientTooltip( 1060660 ) ); // ~1_val~: ~2_val~
							t->FormatArgs( "Time until next spawn\t%d sec", pItem->GetTimerAdjusted() );
						} break;

					case IT_COMM_CRYSTAL:
						this->m_TooltipData.Add( t = new CClientTooltip( 1060658 ) ); // ~1_val~: ~2_val~
						t->FormatArgs( "Linked\t%s", ( ( (DWORD) pItem->m_uidLink == 0x4FFFFFFF ) ? "No" : "Yes" ) );
						break;

					case IT_STONE_GUILD:
						{
							this->m_TooltipData.Clean(true);
							this->m_TooltipData.Add( t = new CClientTooltip( 1041429 ) );
							CItemStone * thisStone = dynamic_cast<CItemStone *>(pItem);
							if ( thisStone )
							{
								if ( thisStone->GetAbbrev()[0] )
								{
									this->m_TooltipData.Add( t = new CClientTooltip( 1060802 ) ); // Guild name: ~1_val~
									t->FormatArgs( "%s [%s]", thisStone->GetName(), thisStone->GetAbbrev() );
								}
								else
								{
									this->m_TooltipData.Add( t = new CClientTooltip( 1060802 ) ); // Guild name: ~1_val~
									t->FormatArgs( "%s", thisStone->GetName() );
								}
							}
						} break;
				}
			}
		}
	}

	if ( !this->m_TooltipData.GetCount() )
		return;

	CCommand pack;
	memset( &pack, 0, sizeof( pack ) );
	int len = 0;

	pack.AOSTooltip.m_Cmd = XCMD_AOSTooltip;
	len += sizeof( pack.AOSTooltip.m_Cmd );
	pack.AOSTooltip.m_Unk1 = 1;
	len += sizeof( pack.AOSTooltip.m_Unk1 );

	pack.AOSTooltip.m_UID = pObj->GetUID();
	len += sizeof( pack.AOSTooltip.m_UID );

	pack.AOSTooltip.m_Unk2 = 0;
	len += sizeof( pack.AOSTooltip.m_Unk2 );

	pack.AOSTooltip.m_ListID = hash;
	len += sizeof( pack.AOSTooltip.m_ListID );

	// Set later
	pack.AOSTooltip.m_len = 0;
	len += sizeof( pack.AOSTooltip.m_len );

	int x = 0;
	for ( ; this->m_TooltipData.GetCount() > x; x++ )
	{
		CClientTooltip * tipEntry = this->m_TooltipData.GetAt(x);

		NDWORD *locID = (NDWORD *)( &pack.AOSTooltip.m_Cmd + len );
		*locID = tipEntry->m_clilocid;
		len += sizeof( pack.AOSTooltip.m_list[0].m_LocID );

		NWORD *locSize = (NWORD *)( &pack.AOSTooltip.m_Cmd + len );
		*locSize = (WORD)( strlen( tipEntry->m_args ) * 2 );
		len += sizeof( pack.AOSTooltip.m_list[0].m_textlen );

		TCHAR *locText = (TCHAR *)( &pack.AOSTooltip.m_Cmd + len );
		TCHAR *pLoc = (TCHAR *) tipEntry->m_args;

		while ( *pLoc )
		{
			*locText = *pLoc;
			locText++;
			pLoc++;

			*locText = 0;
			locText++;

			len += 2;
		}
	}

	// Terminate with a 0 locid
	NDWORD *locID = (NDWORD *)( &pack.AOSTooltip.m_Cmd + len );
	locID = 0;
	len += sizeof( pack.AOSTooltip.m_list[0].m_LocID );

	pack.AOSTooltip.m_len = len;
	pack.AOSTooltip.m_ListID = hash;

#undef DOHASH

	// Enqueue the data
	m_TooltipQueue.push_back( new CTooltipData(&(pack.m_Raw), len,
							(DWORD)pObj->GetUID(), g_World.GetCurrentTime().GetTimeRaw()) );

	// Old send
	// xSend(&pack, len);
}

void CClient::addShowDamage( int damage, DWORD uid_damage )
{
	ADDTOCALLSTACK("CClient::addShowDamage");
	if ( damage < 0 )
		damage = 0;

	if ( IsClientVer(0x400070) || IsNoCryptVer(0x400070) )
	{
		if ( damage > 65535 )
			damage = 65535;

		CCommand cmdDamage;
		cmdDamage.DamagePacket.m_Cmd = XCMD_DamagePacket;
		cmdDamage.DamagePacket.m_UID = uid_damage;
		cmdDamage.DamagePacket.m_dmg = damage;
		xSend(&cmdDamage, sizeof(cmdDamage.DamagePacket));
		return;
	}

	if ( IsClientVer(0x400000) || IsNoCryptVer(0x400000) )
	{
		if ( damage > 255 )
			damage = 255;

		CExtData cmdData;
		cmdData.DamagePacketOld.m_dmg = damage;
		cmdData.DamagePacketOld.m_unk1 = 1;
		cmdData.DamagePacketOld.m_UID = uid_damage;
		addExtData(EXTDATA_DamagePacketOld, &cmdData, sizeof( cmdData.DamagePacketOld ) );
	}
}

void CClient::addSpeedMode( int speedMode )
{
	ADDTOCALLSTACK("CClient::addSpeedMode");
	CExtData cmdData;
	cmdData.SpeedMode.m_speed = speedMode;
	addExtData( EXTDATA_SpeedMode, &cmdData, sizeof( cmdData.SpeedMode ) );
}

void CClient::addVisualRange( BYTE visualRange )
{
	ADDTOCALLSTACK("CClient::addVisualRange");

	DEBUG_ERR(("addVisualRange called with argument %d\n", visualRange));

	CCommand cmd;
	cmd.VisualRange.m_Cmd = XCMD_ViewRange;
	cmd.VisualRange.m_Value = visualRange;
	xSend(&cmd, sizeof(cmd.VisualRange));
}

void CClient::addIdleWarning( bool bSameChar )
{
	ADDTOCALLSTACK("CClient::addIdleWarning");
	CCommand cmd;
	cmd.IdleWarning.m_Cmd = XCMD_IdleWarning;
	cmd.IdleWarning.m_Value = (bSameChar) ? 0x05 : 0x07;
	xSend(&cmd, sizeof(cmd.IdleWarning));
}


// --------------------------------------------------------------------
void CClient::SendPacket( TCHAR * pszKey )
{
	ADDTOCALLSTACK("CClient::SendPacket");
	BYTE	*pszTmp = (BYTE *)Str_GetTemp();
	int	iLen = -1;

	while ( *pszKey )
	{
		if ( iLen > SCRIPT_MAX_LINE_LEN - 4 )
		{	// we won't get here because this lenght is enforced in all scripts
			DEBUG_ERR(("SENDPACKET too big.\n"));
			return;
		}
		GETNONWHITESPACE( pszKey );
		if ( 0 ) ;
		else if ( toupper(*pszKey) == 'D' )
		{
			++pszKey;
			NDWORD		iVal;
			iVal		= Exp_GetVal(pszKey);
			pszTmp[++iLen]	= (BYTE) ((iVal >> 24) & 0xFF);
			pszTmp[++iLen]	= (BYTE) ((iVal >> 16) & 0xFF);
			pszTmp[++iLen]	= (BYTE) ((iVal >>  8) & 0xFF);
			pszTmp[++iLen]	= (BYTE) ((iVal      ) & 0xFF);
		}
		else if ( toupper(*pszKey) == 'W' )
		{
			++pszKey;
			NWORD		iVal;
			iVal		= Exp_GetVal(pszKey);
			pszTmp[++iLen]	= (BYTE) ((iVal >>  8) & 0xFF);
			pszTmp[++iLen]	= (BYTE) ((iVal      ) & 0xFF);
		}
		else
		{
			if ( toupper(*pszKey) == 'B' )
				pszKey++;
			int		iVal	= Exp_GetVal(pszKey);
			pszTmp[++iLen]		= (BYTE) iVal;
		}
	}

	xSendReady( (void*) &pszTmp[0], iLen+1 );
}

// ---------------------------------------------------------------------
// Login type stuff.

bool CClient::Setup_Start( CChar * pChar ) // Send character startup stuff to player
{
	ADDTOCALLSTACK("CClient::Setup_Start");
	// Play this char.
	ASSERT( GetAccount() );
	ASSERT( pChar );
	char	*z = Str_GetTemp();

	CharDisconnect();	// I'm already logged in as someone else ?

	g_Log.Event( LOGM_CLIENTS_LOG, "%x:Setup_Start acct='%s', char='%s'\n", m_Socket.GetSocket(), (LPCTSTR) GetAccount()->GetName(), (LPCTSTR) pChar->GetName());

	bool fQuickLogIn = false;
	bool fNoMessages = false;
	if ( !pChar->IsDisconnected() )
	{
		// The players char is already in game ! Client linger time re-login.
		fQuickLogIn = true;
	}

	//	gms should login with invul and without allshow flag set
	if ( GetPrivLevel() >= PLEVEL_Counsel )
	{
		if ( IsPriv(PRIV_ALLSHOW) ) ClearPrivFlags(PRIV_ALLSHOW);
		if ( !pChar->IsStatFlag(STATF_INVUL) ) pChar->StatFlag_Set(STATF_INVUL);
	}

	addPlayerStart( pChar );
	ASSERT(m_pChar);

	// Gump memory cleanup, we don't want them from logged out players
	m_pChar->Memory_ClearTypes(MEMORY_GUMPRECORD);

	// Resend hiding buff if hidden
	if ( m_pChar->IsStatFlag( STATF_Hidden ) )
	{
		GetClient()->addBuff( BI_HIDDEN , 1075655, 1075656, 0 );
	}
	//Resend buff icons
	resendBuffs();

	CScriptTriggerArgs	Args( fNoMessages, fQuickLogIn, NULL );

	if ( pChar->OnTrigger( CTRIG_LogIn, pChar, &Args ) == TRIGRET_RET_TRUE )
	{
		m_pChar->ClientDetach();
		pChar->SetDisconnected();
		addLoginErr( LOGIN_ERR_BLOCKED );
		return false;
	}

	pChar->SetKeyNum("LastHit", 0);

	fNoMessages	= (Args.m_iN1 != 0);
	fQuickLogIn	= (Args.m_iN2 != 0);

	if ( !fQuickLogIn )
	{
		if ( !fNoMessages )
		{
			addBark(g_szServerDescription, NULL, HUE_YELLOW, TALKMODE_SYSTEM, FONT_NORMAL);

			sprintf(z, (g_Serv.m_Clients.GetCount()==2) ?
				g_Cfg.GetDefaultMsg( DEFMSG_LOGIN_PLAYER ) : g_Cfg.GetDefaultMsg( DEFMSG_LOGIN_PLAYERS ),
				g_Serv.m_Clients.GetCount()-1 );
			addSysMessage(z);

			sprintf(z, "Last logged: %s", GetAccount()->m_TagDefs.GetKeyStr("LastLogged"));
			addSysMessage(z);
		}
		if ( m_pChar->m_pArea && m_pChar->m_pArea->IsGuarded() && !m_pChar->m_pArea->IsFlag(REGION_FLAG_ANNOUNCE) )
		{
			CVarDefContStr	*pVarStr = dynamic_cast <CVarDefContStr *>( m_pChar->m_pArea->m_TagDefs.GetKey("GUARDOWNER"));
			SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_REGION_GUARDSP),
				( pVarStr ) ? (LPCTSTR) pVarStr->GetValStr() : g_Cfg.GetDefaultMsg(DEFMSG_REGION_GUARDSPT));
			if ( m_pChar->m_pArea->m_TagDefs.GetKeyNum("RED", true) )
			{
				SysMessage("You are in the red region.");
			}
		}
	}

	if ( GetAccount()->m_TagDefs.GetKey("LastLogged") )
		GetAccount()->m_TagDefs.DeleteKey("LastLogged");

	if ( IsPriv(PRIV_GM_PAGE) && g_World.m_GMPages.GetCount() )
	{
		sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_GMPAGES), g_World.m_GMPages.GetCount(), g_Cfg.m_cCommandPrefix);
		addSysMessage(z);
	}
	if ( IsPriv(PRIV_JAILED) )
	{
		m_pChar->Jail(&g_Serv, true, 0);
	}
	if ( g_Serv.m_timeShutdown.IsTimeValid() )
	{
		addBark( g_Cfg.GetDefaultMsg( DEFMSG_SERV_SHUTDOWN_SOON ), NULL, HUE_TEXT_DEF, TALKMODE_SYSTEM, FONT_BOLD);
	}
	// Announce you to the world.
	Announce(true);
	m_pChar->Update(this);



	// don't login on the water ! (unless i can swim)
	if ( !m_pChar->Char_GetDef()->Can(CAN_C_SWIM) && !IsPriv(PRIV_GM) && m_pChar->IsSwimming() )
	{
		// bring to the nearest shore.
		int iDist = 1;
		int i;
		for ( i=0; i<20; i++)
		{
			// try diagonal in all directions
			int iDistNew = iDist + 20;
			for ( int iDir = DIR_NE; iDir <= DIR_NW; iDir += 2 )
			{
				if ( m_pChar->MoveToValidSpot((DIR_TYPE)iDir, iDistNew, iDist) )
				{
					i = 100;	// breakout
					break;
				}
			}
			iDist = iDistNew;
		}
		if ( i < 100 )
		{
			addSysMessage( g_Cfg.GetDefaultMsg( DEFMSG_REGION_WATER_1 ) );
		}
		else
		{
			addSysMessage( g_Cfg.GetDefaultMsg( DEFMSG_REGION_WATER_2 ) );
		}
	}

	DEBUG_MSG(( "%x:Setup_Start done\n", m_Socket.GetSocket()));
	return true;
}

void CClient::Setup_CreateDialog( const CEvent * pEvent ) // All the character creation stuff
{
	ADDTOCALLSTACK("CClient::Setup_CreateDialog");
	ASSERT( GetAccount());
	if ( m_pChar != NULL )
	{
		// Loggin in as a new player while already online !
		addSysMessage( g_Cfg.GetDefaultMsg( DEFMSG_ALREADYONLINE ) );
		DEBUG_ERR(( "%x:Setup_CreateDialog acct='%s' already online!\n", m_Socket.GetSocket(), GetAccount()->GetName()));
		return;
	}

	// Make sure they don't already have too many chars !
	int iMaxChars = ( IsPriv( PRIV_GM )) ? MAX_CHARS_PER_ACCT : ( g_Cfg.m_iMaxCharsPerAccount );
	int iQtyChars = GetAccount()->m_Chars.GetCharCount();
	if ( iQtyChars >= iMaxChars )
	{
		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_MAXCHARS ), iQtyChars );
		if ( GetPrivLevel() < PLEVEL_Seer )
		{
			addLoginErr( LOGIN_ERR_OTHER );
			return;
		}
	}

	CChar * pChar = CChar::CreateBasic( CREID_MAN );
	ASSERT(pChar);
	pChar->InitPlayer( pEvent, this );

	g_Log.Event( LOGM_CLIENTS_LOG, "%x:Setup_CreateDialog acct='%s', char='%s'\n",
		m_Socket.GetSocket(), (LPCTSTR)GetAccount()->GetName(), (LPCTSTR)pChar->GetName());

	enum TRIGRET_TYPE	tr;
	CScriptTriggerArgs createArgs;
	createArgs.m_iN1 = (DWORD) pEvent->Create.m_flags;
	createArgs.m_iN2 = (int) pEvent->Create.m_prof;
	createArgs.m_iN3 = ((pEvent->Create.m_sex - 2) >= 0);
	createArgs.m_s1 = GetAccount()->GetName();
	createArgs.m_pO1 = this;

	r_Call("f_onchar_create", pChar, &createArgs, NULL, &tr);

	Setup_Start( pChar );
}

bool CClient::Setup_Play( int iSlot ) // After hitting "Play Character" button
{
	ADDTOCALLSTACK("CClient::Setup_Play");
	// Mode == CLIMODE_SETUP_CHARLIST

	DEBUG_MSG(( "%x:Setup_Play slot %d\n", m_Socket.GetSocket(), iSlot ));

	if ( ! GetAccount())
		return( false );
	if ( iSlot >= COUNTOF(m_tmSetupCharList))
		return false;

	CChar * pChar = m_tmSetupCharList[ iSlot ].CharFind();
	if ( ! GetAccount()->IsMyAccountChar( pChar ))
		return false;

	CChar * pCharLast = GetAccount()->m_uidLastChar.CharFind();
	if ( pCharLast && GetAccount()->IsMyAccountChar( pCharLast ) && GetAccount()->GetPrivLevel() <= PLEVEL_GM &&
		! pCharLast->IsDisconnected() && (pChar->GetUID() != pCharLast->GetUID()))
	{
		addIdleWarning(true);
		return( false );
	}

	return Setup_Start( pChar );
}

DELETE_ERR_TYPE CClient::Setup_Delete( int iSlot ) // Deletion of character
{
	ADDTOCALLSTACK("CClient::Setup_Delete");
	ASSERT( GetAccount() );
	DEBUG_MSG(( "%x:Setup_Delete slot=%d\n", m_Socket.GetSocket(), iSlot ));
	if ( iSlot >= COUNTOF(m_tmSetupCharList))
		return DELETE_ERR_NOT_EXIST;

	CChar * pChar = m_tmSetupCharList[iSlot].CharFind();
	if ( ! GetAccount()->IsMyAccountChar( pChar ))
		return DELETE_ERR_BAD_PASS;

	if ( ! pChar->IsDisconnected())
	{
		return DELETE_ERR_IN_USE;
	}

	// Make sure the char is at least x days old.
	if ( g_Cfg.m_iMinCharDeleteTime &&
		(- g_World.GetTimeDiff( pChar->m_timeCreate )) < g_Cfg.m_iMinCharDeleteTime )
	{
		if ( GetPrivLevel() < PLEVEL_Seer )
		{
			return DELETE_ERR_NOT_OLD_ENOUGH;
		}
	}

	//	Do the scripts allow to delete the char?
	enum TRIGRET_TYPE	tr;
	CScriptTriggerArgs Args;
	Args.m_pO1 = this;
	pChar->r_Call("f_onchar_delete", pChar, &Args, NULL, &tr);
	if ( tr == TRIGRET_RET_TRUE )
	{
		return DELETE_ERR_NOT_OLD_ENOUGH;
	}

	// pChar->Delete();
	delete pChar;
	// refill the list.

	CCommand cmd;
	cmd.CharList2.m_Cmd = XCMD_CharList2;
	int len = sizeof( cmd.CharList2 );
	cmd.CharList2.m_len = len;
	cmd.CharList2.m_count = Setup_FillCharList( cmd.CharList2.m_char, GetAccount()->m_uidLastChar.CharFind());
	xSendPkt( &cmd, len );

	return( DELETE_SUCCESS );
}

LOGIN_ERR_TYPE CClient::Setup_ListReq( const char * pszAccName, const char * pszPassword, bool fTest )
{
	ADDTOCALLSTACK("CClient::Setup_ListReq");
	// XCMD_CharListReq
	// Gameserver login and request character listing

	if ( GetConnectType() != CONNECT_GAME )	// Not a game connection ?
	{
		return(LOGIN_ERR_OTHER);
	}

	switch ( GetTargMode())
	{
		case CLIMODE_SETUP_RELAY:
			ClearTargMode();
			break;
	}

	CGString sMsg;
	LOGIN_ERR_TYPE lErr = LOGIN_ERR_OTHER;

	lErr = LogIn( pszAccName, pszPassword, sMsg );

	if ( lErr != LOGIN_SUCCESS )
	{
		if ( fTest && lErr != LOGIN_ERR_OTHER )
		{
			if ( ! sMsg.IsEmpty())
			{
				SysMessage( sMsg );
			}
			addLoginErr(lErr);
		}
		return( lErr );
	}

	ASSERT( GetAccount() );

	CChar * pCharLast = GetAccount()->m_uidLastChar.CharFind();
/*	if ( pCharLast &&
		GetAccount()->IsMyAccountChar( pCharLast ) &&
		GetAccount()->GetPrivLevel() <= PLEVEL_GM &&
		! pCharLast->IsDisconnected())
	{
		// If the last char is lingering then log back into this char instantly.
		// m_iClientLingerTime
		if ( Setup_Start(pCharLast) )
			return LOGIN_SUCCESS;
		return LOGIN_ERR_BLOCKED; //Setup_Start() returns false only when login blocked by Return 1 in @Login
	} */


	{
		CCommand cmd;
		cmd.FeaturesEnable.m_Cmd = XCMD_Features;
		cmd.FeaturesEnable.m_enable = g_Cfg.GetPacketFlag(false, (RESDISPLAY_VERSION)GetAccount()->GetResDisp());
		// Here always use xSendPktNow, since this packet has to be separated from the next one
		xSendPktNow( &cmd, sizeof( cmd.FeaturesEnable ));
	}

	CCommand cmd;
	cmd.CharList.m_Cmd = XCMD_CharList;
	int len = sizeof( cmd.CharList ) - sizeof(cmd.CharList.m_start) + ( g_Cfg.m_StartDefs.GetCount() * sizeof(cmd.CharList.m_start[0]));
	cmd.CharList.m_len = len;
	NDWORD *	flags	= (NDWORD*) (&(cmd.CharList.m_Cmd) + len - sizeof(NDWORD));

	*flags	= g_Cfg.GetPacketFlag(true, (RESDISPLAY_VERSION)GetAccount()->GetResDisp());

	// list chars to your account that may still be logged in !
	// "LASTCHARUID" = list this one first.
	cmd.CharList.m_count = Setup_FillCharList( cmd.CharList.m_char, pCharLast );

	// now list all the starting locations. (just in case we create a new char.)
	// NOTE: New Versions of the client just ignore all this stuff.

	int iCount = g_Cfg.m_StartDefs.GetCount();
	cmd.CharList.m_startcount = iCount;
	for ( int i=0;i<iCount;i++)
	{
		cmd.CharList.m_start[i].m_id = i+1;
		strcpylen( cmd.CharList.m_start[i].m_area, g_Cfg.m_StartDefs[i]->m_sArea, sizeof(cmd.CharList.m_start[i].m_area));
		strcpylen( cmd.CharList.m_start[i].m_name, g_Cfg.m_StartDefs[i]->m_sName, sizeof(cmd.CharList.m_start[i].m_name));
	}


	xSendPkt( &cmd, len );
	m_Targ_Mode = CLIMODE_SETUP_CHARLIST;
	return LOGIN_SUCCESS;
}

LOGIN_ERR_TYPE CClient::LogIn( CAccountRef pAccount, CGString & sMsg )
{
	ADDTOCALLSTACK("CClient::LogIn");
	if ( pAccount == NULL )
		return( LOGIN_ERR_NONE );

	if ( pAccount->IsPriv( PRIV_BLOCKED ))
	{
		sMsg.Format( g_Cfg.GetDefaultMsg( DEFMSG_ACC_BLOCKED ), (LPCTSTR) g_Serv.m_sEMail );
		return( LOGIN_ERR_BLOCKED );
	}

	// Look for this account already in use.
	CClient * pClientPrev = pAccount->FindClient( this );
	if ( pClientPrev != NULL )
	{
		// Only if it's from a diff ip ?
		ASSERT( pClientPrev != this );

		bool bInUse = false;

		//	different ip - no reconnect
		if ( ! m_PeerName.IsSameIP( pClientPrev->m_PeerName )) bInUse = true;
		else
		{
			//	from same ip - allow reconnect if the old char is lingering out
			CChar *pCharOld = pClientPrev->GetChar();
			if ( pCharOld )
			{
				CItem	*pItem = pCharOld->LayerFind(LAYER_FLAG_ClientLinger);
				if ( !pItem ) bInUse = true;
			}

			if ( !bInUse )
			{
				if ( IsConnectTypePacket() && pClientPrev->IsConnectTypePacket())
				{
					pClientPrev->CharDisconnect();
					pClientPrev->m_fClosed = true;
				}
				else if ( GetConnectType() == pClientPrev->GetConnectType() ) bInUse = true;
			}
		}

		if ( bInUse )
		{
			sMsg = "Account already in use.";
			return LOGIN_ERR_USED;
		}
	}

	if ( g_Cfg.m_iClientsMax <= 0 )
	{
		// Allow no one but locals on.
		CSocketAddress SockName = m_Socket.GetSockName();
		if ( ! m_PeerName.IsLocalAddr() && SockName.GetAddrIP() != m_PeerName.GetAddrIP() )
		{
			sMsg = g_Cfg.GetDefaultMsg( DEFMSG_SERV_LD );
			return( LOGIN_ERR_BLOCKED );
		}
	}
	if ( g_Cfg.m_iClientsMax <= 1 )
	{
		// Allow no one but Administrator on.
		if ( pAccount->GetPrivLevel() < PLEVEL_Admin )
		{
			sMsg = g_Cfg.GetDefaultMsg( DEFMSG_SERV_AO );
			return( LOGIN_ERR_BLOCKED );
		}
	}
	if ( pAccount->GetPrivLevel() < PLEVEL_GM &&
		g_Serv.m_Clients.GetCount() > g_Cfg.m_iClientsMax  )
	{
		// Give them a polite goodbye.
		sMsg = g_Cfg.GetDefaultMsg( DEFMSG_SERV_FULL );
		return( LOGIN_ERR_BLOCKED );
	}
	//	Do the scripts allow to login this account?
	pAccount->m_Last_IP = m_PeerName;
	CScriptTriggerArgs Args;
	Args.Init(pAccount->GetName());
	Args.m_iN1 = GetConnectType();
	Args.m_pO1 = this;
	enum TRIGRET_TYPE tr;
	g_Serv.r_Call("f_onaccount_login", &g_Serv, &Args, NULL, &tr);
	if ( tr == TRIGRET_RET_TRUE )
	{
		sMsg = g_Cfg.GetDefaultMsg( DEFMSG_ACC_DENIED );
		return (LOGIN_ERR_BLOCKED);
	}

	m_pAccount = pAccount;
	pAccount->OnLogin( this );

	return( LOGIN_SUCCESS );
}

LOGIN_ERR_TYPE CClient::LogIn( LPCTSTR pszAccName, LPCTSTR pszPassword, CGString & sMsg )
{
	ADDTOCALLSTACK("CClient::LogIn");
	// Try to validate this account.
	// Do not send output messages as this might be a console or web page or game client.
	// NOTE: addLoginErr() will get called after this.

	if ( GetAccount()) // already logged in.
		return( LOGIN_SUCCESS );

	TCHAR szTmp[ MAX_NAME_SIZE ];
	int iLen1 = strlen( pszAccName );
	int iLen2 = strlen( pszPassword );
	int iLen3 = Str_GetBare( szTmp, pszAccName, MAX_NAME_SIZE );
	if ( iLen1 == 0 ||
		iLen1 != iLen3 ||
		iLen1 > MAX_NAME_SIZE )	// a corrupt message.
	{
badformat:
		TCHAR szVersion[ 256 ];
		sMsg.Format( g_Cfg.GetDefaultMsg( DEFMSG_ACC_WCLI ), (LPCTSTR) m_Crypt.WriteClientVer( szVersion ));
		return( LOGIN_ERR_OTHER );
	}

	iLen3 = Str_GetBare( szTmp, pszPassword, MAX_NAME_SIZE );
	if ( iLen2 != iLen3 )	// a corrupt message.
		goto badformat;


	TCHAR szName[ MAX_ACCOUNT_NAME_SIZE ];
	if ( !CAccount::NameStrip(szName, pszAccName) || Str_Check(pszAccName) || Str_Check(pszPassword) )
	{
		return( LOGIN_ERR_OTHER );
	}

	bool fGuestAccount = ! strnicmp( pszAccName, "GUEST", 5 );
	if ( fGuestAccount )
	{
		// trying to log in as some sort of guest.
		// Find or create a new guest account.
		TCHAR *pszTemp = Str_GetTemp();
		for ( int i=0; 1; i++ )
		{
			if ( i>=g_Cfg.m_iGuestsMax )
			{
				sMsg = g_Cfg.GetDefaultMsg( DEFMSG_ACC_GUSED );
				return( LOGIN_ERR_BLOCKED );
			}

			sprintf(pszTemp, "GUEST%d", i);
			CAccountRef pAccount = g_Accounts.Account_FindCreate(pszTemp, true );
			ASSERT( pAccount );

			if ( pAccount->FindClient() == NULL )
			{
				pszAccName = pAccount->GetName();
				break;
			}
		}
	}
	else
	{
		if ( pszPassword[0] == '\0' )
		{
			sMsg = g_Cfg.GetDefaultMsg( DEFMSG_ACC_NEEDPASS );
			return( LOGIN_ERR_BAD_PASS );
		}
	}

	bool fAutoCreate = ( g_Serv.m_eAccApp == ACCAPP_Free || g_Serv.m_eAccApp == ACCAPP_GuestAuto || g_Serv.m_eAccApp == ACCAPP_GuestTrial );

	CAccountRef pAccount = g_Accounts.Account_FindCreate(pszAccName, fAutoCreate);
	if ( ! pAccount )
	{
		g_Log.Event(LOGM_CLIENTS_LOG, "%x:ERR Login NO Account '%s'\n", m_Socket.GetSocket(), pszAccName);
		sMsg.Format(g_Cfg.GetDefaultMsg(DEFMSG_ACC_UNK), pszAccName);
		return LOGIN_ERR_NONE;
	}

	if ( ! fGuestAccount && ! pAccount->IsPriv(PRIV_BLOCKED) )
	{
		if ( ! pAccount->CheckPassword(pszPassword))
		{
			g_Log.Event(LOGM_CLIENTS_LOG, "%x: '%s' bad password\n", m_Socket.GetSocket(), (LPCTSTR) pAccount->GetName());
			sMsg = g_Cfg.GetDefaultMsg(DEFMSG_ACC_BADPASS);
			return LOGIN_ERR_BAD_PASS;
		}
	}

	return LogIn(pAccount, sMsg);
}


