#include "graysvr.h"
#include "CClient.h"
#include "network/send.h"
#include "network/network.h"
#include "common/version.h"

/////////////////////////////////////////////////////////////////
// -CClient stuff.

bool CClient::addDeleteErr( DELETE_ERR_TYPE code )
{
	// code
	if ( code == DELETE_SUCCESS )
		return true;

	DEBUG_ERR(("%x:Bad Char Delete Attempted %d\n", socketId(), code));

	CCommand cmd;
	cmd.DeleteBad.m_Cmd = XCMD_DeleteBad;
	cmd.DeleteBad.m_code = code;
	xSend(&cmd, sizeof(cmd.DeleteBad));
	return false;
}

void CClient::addExtData( EXTDATA_TYPE type, const CExtData * pData, int iSize)
{
	CCommand cmd;
	int iSizeTotal = iSize + sizeof(cmd.ExtData) - sizeof(cmd.ExtData.m_u);
	cmd.ExtData.m_Cmd = XCMD_ExtData;
	cmd.ExtData.m_len = iSizeTotal;
	cmd.ExtData.m_type = type;
	memcpy( &(cmd.ExtData.m_u), pData, iSize );
	xSend( &cmd, iSizeTotal );
}

void CClient::addTime()
{
	// Send time. (real or game time ??? why ?)
	CCommand cmd;
	long lCurrentTime = (CServTime::GetCurrentTime()).GetTimeRaw();
	cmd.Time.m_Cmd = XCMD_Time;
	cmd.Time.m_hours = ( lCurrentTime / ( 60*60*TICK_PER_SEC )) % 24;
	cmd.Time.m_min   = ( lCurrentTime / ( 60*TICK_PER_SEC )) % 60;
	cmd.Time.m_sec   = ( lCurrentTime / ( TICK_PER_SEC )) % 60;
	xSend(&cmd, sizeof(cmd.Time));
}

void CClient::addObjectRemoveCantSee( UID uid, LPCTSTR pszName )
{
	// Seems this object got out of sync some how.
	if ( pszName == NULL ) pszName = "it";
	SysMessagef( "You can't see %s", pszName );
	addObjectRemove( uid );
}

void CClient::addObjectRemove(UID uid)
{
	PacketRemoveObject *packet = new PacketRemoveObject(uid, this);
}

void CClient::addObjectRemove( const CObjBase * pObj )
{
	addObjectRemove(pObj->GetUID());
}

void CClient::addRemoveAll( bool fItems, bool fChars )
{
	if ( fItems )
	{
		// Remove any multi objects first ! or client will hang
		CWorldSearch AreaItems(GetChar()->GetTopPoint(), UO_MAP_VIEW_RADAR);
		AreaItems.SetAllShow( IsPriv( PRIV_ALLSHOW ));	// show logged out chars?
		while ( CItem *pItem = AreaItems.GetItem() )
		{
			addObjectRemove(pItem);
		}
	}
	if ( fChars )
	{
		// Remove any multi objects first ! or client will hang
		CWorldSearch AreaChars(GetChar()->GetTopPoint(), UO_MAP_VIEW_SIZE);
		AreaChars.SetAllShow(IsPriv(PRIV_ALLSHOW));
		while ( CChar *pChar = AreaChars.GetChar() )
		{
			addObjectRemove( pChar );
		}
	}
}

void CClient::addItem_OnGround( CItem * pItem ) // Send items (on ground)
{
	PacketItemWorld *cmd = new PacketItemWorld(pItem, this);

	if ( !IsPriv(PRIV_DEBUG) && 
		(( pItem->GetDispID() == ITEMID_CORPSE ) && CCharBase::IsHumanID(pItem->GetCorpseType()) )
		)
	{
		CItemCorpse *pCorpse = dynamic_cast <CItemCorpse*> (pItem);
		if ( pCorpse )
		{
			addContents(pCorpse, false, true, false);	// send all items
			addContents(pCorpse, true, true, false);	// equip proper ones
		}
	}
	addAOSTooltip(pItem);
}

void CClient::addItem_Equipped( const CItem * pItem )
{
	// Equip a single item on a CChar.
	CChar * pChar = dynamic_cast <CChar*> (pItem->GetParent());

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

	xSend( &cmd, sizeof( cmd.ItemEquip ));

	addAOSTooltip( pItem );
}

void CClient::addItem_InContainer( const CItem * pItem )
{
	CItemContainer * pCont = dynamic_cast <CItemContainer*> (pItem->GetParent());
	if ( !pCont )
		return;

	CPointBase pt = pItem->GetContainedPoint();

	// Add a single item in a container.
	CCommand cmd;
	cmd.ContAdd.m_Cmd = XCMD_ContAdd;
	cmd.ContAdd.m_UID = pItem->GetUID();
	cmd.ContAdd.m_id = pItem->GetDispID();
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
		if ( wHue > HUE_QTY )
			wHue &= HUE_MASK_LO;
	}
	cmd.ContAdd.m_wHue = wHue;

	xSend( &cmd, sizeof( cmd.ContAdd ));

	addAOSTooltip( pItem );
}

void CClient::addItem( CItem * pItem )
{
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
	// NOTE: We needed to send the header for this FIRST !!!
	// 1 = equip a corpse
	// 0 = contents.
	bool fLayer[LAYER_HORSE];
	memset(fLayer, 0, sizeof(fLayer));

	CCommand cmd;

	// send all the items in the container.
	int count = 0;
	for ( CItem* pItem = pContainer->GetContentHead(); pItem ; pItem=pItem->GetNext() )
	{
		if ( count >= MAX_ITEMS_CONTENT )
		{
			g_Log.Event( LOGL_WARN, "Too many items in container '%s' uid=0%x items=%d\n", pContainer->GetName(), pContainer->GetUID(), pContainer->GetCount());
			break;
		}

		LAYER_TYPE layer;

		if ( fCorpseFilter )	// dressing a corpse is different from opening the coffin!
		{
			layer = (LAYER_TYPE) pItem->GetContainedLayer();
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
			cmd.CorpEquip.m_item[count].m_layer	= pItem->GetContainedLayer();
			cmd.CorpEquip.m_item[count].m_UID	= pItem->GetUID();
		}
		else	// Content items
		{
			if ( !fShop && pItem->IsAttr(ATTR_INVIS) && !CanSee(pItem) )
				continue;

			cmd.Content.m_item[count].m_UID		= pItem->GetUID();
			cmd.Content.m_item[count].m_id		= pItem->GetDispID();
			cmd.Content.m_item[count].m_zero6	= 0;
			cmd.Content.m_item[count].m_amount	= pItem->GetAmount();
			if ( fShop )
			{
				CItemVendable * pVendItem = dynamic_cast <CItemVendable *> (pItem);
				if ( !pVendItem || !pVendItem->GetAmount() || pVendItem->IsType(IT_GOLD) )
					continue;

				cmd.Content.m_item[count].m_amount = cmd.Content.m_item[count].m_amount;
				cmd.Content.m_item[count].m_x	= count;
				cmd.Content.m_item[count].m_y	= count;
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
				if ( wHue > HUE_QTY )
					wHue &= HUE_MASK_LO;	// restrict colors
			}
			cmd.Content.m_item[count].m_wHue = wHue;
		}
		count ++;

		addAOSTooltip(pItem);
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

	xSend(&cmd, len);
	return count;
}



void CClient::addOpenGump( const CObjBase * pContainer, GUMP_TYPE gump )
{
	// NOTE: if pContainer has not already been sent to the client
	//  this will crash client.
	CCommand cmd;
	cmd.ContOpen.m_Cmd = XCMD_ContOpen;
	cmd.ContOpen.m_UID = pContainer->GetUID();
	cmd.ContOpen.m_gump = gump;

	// we automatically get open sound for this,.
	xSend( &cmd, sizeof( cmd.ContOpen ));
}

bool CClient::addContainerSetup( const CItemContainer * pContainer ) // Send Backpack (with items)
{
	// open the conatiner with the proper GUMP.
	CItemBase * pItemDef = pContainer->Item_GetDef();
	GUMP_TYPE gump = pItemDef->IsTypeContainer();
	if ( gump <= GUMP_RESERVED )
	{
		return false;
	}

	addOpenGump( pContainer, gump );
	addContents( pContainer, false, false, false );
	return true;
}

void CClient::addSeason(SEASON_TYPE season)
{
	if ( m_pChar->IsStatFlag(STATF_DEAD) )		// everything looks like this when dead.
		season = SEASON_Desolate;
	if ( season == m_Env.m_Season )	// the season i saw last.
		return;

	m_Env.m_Season = season;

	CCommand cmd;
	cmd.Season.m_Cmd = XCMD_Season;
	cmd.Season.m_season = season;
	cmd.Season.m_cursor = 1;
	xSend(&cmd, sizeof(cmd.Season));
}

void CClient::addWeather( WEATHER_TYPE weather ) // Send new weather to player
{
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
	xSend( &cmd, sizeof(cmd.Weather));
}

void CClient::addLight(int light)
{
	if ( m_pChar->m_LocalLight )
		light = m_pChar->m_LocalLight;

	if ( light < LIGHT_BRIGHT )
		light = m_pChar->GetLightLevel();

	if ( light < LIGHT_BRIGHT )
		light = LIGHT_BRIGHT;
	if ( light > LIGHT_DARK )
		light = LIGHT_DARK;

	if ( light == m_Env.m_Light )
		return;
	m_Env.m_Light = light;

	PacketGlobalLight *cmd = new PacketGlobalLight(light, this);
}

void CClient::addArrowQuest( int x, int y )
{
	CCommand cmd;
	cmd.Arrow.m_Cmd = XCMD_Arrow;
	cmd.Arrow.m_Active = ( x && y ) ? 1 : 0;	// 1/0
	cmd.Arrow.m_x = x;
	cmd.Arrow.m_y = y;
	xSend( &cmd, sizeof( cmd.Arrow ));
}

bool CClient::addKick( CTextConsole * pSrc, bool fBlock )
{
	// Kick me out.
	if ( GetAccount() == NULL )
	{
		m_net->markClose();
		return true;
	}

	if ( ! GetAccount()->Kick( pSrc, fBlock ))
		return false;

	SysMessagef( "You have been %sed by '%s'", fBlock ? "KICK" : "DISCONNECT", pSrc->GetName());
	m_net->markClose();
	return true;
}

void CClient::addSound( SOUND_TYPE id, const CObjBaseTemplate * pBase, int iOnce )
{
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

	xSend( &cmd, sizeof(cmd.Sound));
}

void CClient::addItemDragCancel( BYTE type )
{
	CCommand cmd;
	cmd.DragCancel.m_Cmd = XCMD_DragCancel;
	cmd.DragCancel.m_type = type;
	xSend( &cmd, sizeof( cmd.DragCancel ));
}

void CClient::addBarkUNICODE( const NCHAR * pwText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang )
{
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
		cmd.SpeakUNICODE.m_id = pChar->GetDispID();
	}
	xSend( &cmd, len );
}

void CClient::addBarkLocalized( int iClilocId, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, TCHAR * pArgs )
{
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
	xSend( &cmd, len );
}

void CClient::addBarkParse( LPCTSTR pszText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, LPCTSTR name )
{
	if ( !pszText )
		return;

	WORD Args[] = { wHue, font, 0 };
	TEMPSTRING(z);

	if ( *pszText == '@' )
	{
		pszText++;
		if ( *pszText == '@' ) // @@ = just a @ symbol
			goto bark_default;

		const char *		s	= pszText;
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
	sprintf(z, "%s%s",name, pszText);

	switch ( Args[2] )
	{
		case 2:	// Localized
		{
            TCHAR * ppArgs[256];
			int iQty = Str_ParseCmds( z, ppArgs, COUNTOF(ppArgs), "," );
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
			NCHAR szBuffer[MAX_TALK_BUFFER];
			int iLen = CvtSystemToNUNICODE(szBuffer, COUNTOF(szBuffer), z, -1 );
			addBarkUNICODE(szBuffer, pSrc, (HUE_TYPE)Args[0], mode, (FONT_TYPE)Args[1], 0);
		}

		case 0:	// Ascii
		default:
		{
bark_default:
			if ( !*z )
				sprintf(z, "%s%s",name, pszText);

			addBark(z, pSrc, (HUE_TYPE)Args[0], mode, (FONT_TYPE)Args[1]);
		}
	}
}



void CClient::addBark( LPCTSTR pszText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font )
{
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
		cmd.Speak.m_id = pChar->GetDispID();
	}
	strcpylen( cmd.Speak.m_text, pszText, MAX_TALK_BUFFER );
	xSend( &cmd, len );
}

void CClient::addObjMessage( LPCTSTR pMsg, const CObjBaseTemplate * pSrc, HUE_TYPE wHue ) // The message when an item is clicked
{
	if ( !pMsg )
		return;

	//	flood protection
	if ( GetPrivLevel() <= PLEVEL_Player )
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
	// bSpeedSeconds = seconds = 0=very fast, 7=slow.
	// wHue =
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
		xSend( &cmd,  sizeof(cmd.Effect)  );
	else
		xSend( &cmd,  sizeof(cmd.Effect) - 8 );
}


void CClient::GetAdjustedItemID( const CChar * pChar, const CItem * pItem, ITEMID_TYPE & id, HUE_TYPE & wHue )
{
	// An equipped item.
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

		if (( wHue & HUE_MASK_HI ) > HUE_QTY )
			wHue &= HUE_MASK_LO | HUE_TRANSLUCENT;
		else
			wHue &= HUE_MASK_HI | HUE_TRANSLUCENT;
	}
}

void CClient::GetAdjustedCharID( const CChar * pChar, CREID_TYPE & id, HUE_TYPE & wHue )
{
	// Some clients can't see all creature artwork and colors.
	// try to do the translation here,.
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

			// allow transparency and underwear colors
			if (( wHue & HUE_MASK_HI ) > HUE_QTY )
				wHue &= HUE_MASK_LO | HUE_UNDERWEAR | HUE_TRANSLUCENT;
			else
				wHue &= HUE_MASK_HI | HUE_UNDERWEAR | HUE_TRANSLUCENT;
		}
	}
}

void CClient::addCharMove( const CChar * pChar )
{
	// This char has just moved on screen.
	// or changed in a subtle way like "hidden"
	// NOTE: If i have been turned this will NOT update myself.

	CCommand cmd;
	cmd.CharMove.m_Cmd = XCMD_CharMove;
	cmd.CharMove.m_UID = pChar->GetUID();

	CREID_TYPE id;
	HUE_TYPE wHue;
	GetAdjustedCharID( pChar, id, wHue );
	cmd.CharMove.m_id = id;
	cmd.CharMove.m_wHue = wHue;

	CPointMap pt = pChar->GetTopPoint();
	cmd.CharMove.m_x  = pt.m_x;
	cmd.CharMove.m_y  = pt.m_y;
	cmd.CharMove.m_z = pt.m_z;
	cmd.CharMove.m_dir = pChar->GetDirFlag();
	cmd.CharMove.m_mode = pChar->GetModeFlag( pChar->CanSeeTrue( m_pChar ));
	cmd.CharMove.m_noto = pChar->Noto_GetFlag( m_pChar );

	xSend( &cmd, sizeof(cmd.CharMove));
}

void CClient::addChar( const CChar * pChar )
{
	// Full update about a char.

	CCommand cmd;
	cmd.Char.m_Cmd = XCMD_Char;
	cmd.Char.m_UID = pChar->GetUID();

	CREID_TYPE id;
	HUE_TYPE wHue;
	GetAdjustedCharID( pChar, id, wHue );
	cmd.Char.m_id = id;
	cmd.Char.m_wHue = wHue;

	CPointMap pt = pChar->GetTopPoint();
	pt.GetSector()->wakeUp();

	cmd.Char.m_x = pt.m_x;
	cmd.Char.m_y = pt.m_y;
	cmd.Char.m_z = pt.m_z;
	cmd.Char.m_dir = pChar->GetDirFlag();
	cmd.Char.m_mode = pChar->GetModeFlag( pChar->CanSeeTrue( m_pChar ));
	cmd.Char.m_noto = pChar->Noto_GetFlag( m_pChar );

	int len = ( sizeof( cmd.Char ) - sizeof( cmd.Char.equip ));
	CCommand * pCmd = &cmd;

	bool fLayer[LAYER_HORSE+1];
	memset( fLayer, 0, sizeof(fLayer));

	// extend the current struct for all the equipped items.
	for ( CItem* pItem=pChar->GetContentHead(); pItem!=NULL; pItem=pItem->GetNext())
	{
		LAYER_TYPE layer = pItem->GetEquipLayer();
		if ( ! CItemBase::IsVisibleLayer( layer ))
			continue;
		if ( ! m_pChar->CanSeeItem( pItem ) && m_pChar != pChar )
			continue;

		// Make certain that no more than one of each layer goes out to client....crashes otherwise!!
		if ( fLayer[layer] )
		{
			continue;
		}
		fLayer[layer] = true;
		addAOSTooltip( pItem );

		pCmd->Char.equip[0].m_UID = pItem->GetUID();
		pCmd->Char.equip[0].m_layer = layer;

		HUE_TYPE wHue;
		ITEMID_TYPE itemid;
		GetAdjustedItemID( pChar, pItem, itemid, wHue );
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

	pCmd->Char.equip[0].m_UID = 0;	// terminator.
	len += sizeof( DWORD );

	cmd.Char.m_len = len;
	xSend( &cmd, len );

	addAOSTooltip( pChar );
}

void CClient::addItemName( const CItem * pItem )
{
	// NOTE: CanSee() has already been called.
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
				RESOURCE_ID_BASE rid = pItem->IsType(IT_SPAWN_ITEM) ? pItem->m_itSpawnItem.m_ItemID : pItem->m_itSpawnChar.m_CharID;
				len += sprintf(szName + len, " (%s)", g_Cfg.ResourceGetName(rid));
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
		len += sprintf(szName+len, " [0%lx]", pItem->uid());

	addObjMessage(szName, pItem, wHue);
}

void CClient::addCharName( const CChar * pChar ) // Singleclick text for a character
{
	// Karma wHue codes ?
	HUE_TYPE wHue	= pChar->Noto_GetHue( m_pChar, true );

	TEMPSTRING(pszTemp);
	LPCTSTR prefix = pChar->GetKeyStr( "NAME.PREFIX" );
	if ( ! *prefix )
		prefix = pChar->Noto_GetFameTitle();

	strcpy( pszTemp, prefix );
	strcat( pszTemp, pChar->GetName() );
	strcat( pszTemp, pChar->GetKeyStr( "NAME.SUFFIX" ) );

	if ( !pChar->IsStatFlag(STATF_Incognito) || ( GetPrivLevel() > pChar->GetPrivLevel() ))
	{
		// guild abbriviation
		LPCTSTR pszAbrTown = pChar->Guild_Abbrev(MEMORY_TOWN);
		LPCTSTR pszAbrGuild = pChar->Guild_Abbrev(MEMORY_GUILD);

		if ( pszAbrTown || pszAbrGuild )
		{
			TEMPSTRING(z);
			if ( pszAbrTown )
			{
				sprintf(z, " [%s]", pszAbrTown);
				strcat(pszTemp, z);
			}
			if ( pszAbrGuild )
			{
				sprintf(z, " [%s]", pszAbrGuild);
				strcat(pszTemp, z);
			}
		}
	}
	else
		strcpy( pszTemp, pChar->GetName() );

	bool fAllShow = IsPriv(PRIV_DEBUG|PRIV_ALLSHOW);

	if ( fAllShow )
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
		if ( pChar->IsStatFlag( STATF_Hallucinating ))
			strcat( pszTemp, " [hallu]" );

		if ( fAllShow )
		{
			if ( pChar->IsStatFlag(STATF_Spawned) )
				strcat(pszTemp, " [spawn]");
			if ( IsPriv( PRIV_DEBUG ))
				sprintf(pszTemp+strlen(pszTemp), " [0%lx]", pChar->uid());
		}
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

void CClient::addPlayerStart( CChar * pChar )
{
	if ( m_pChar != pChar )	// death option just usese this as a reload.
	{
		CharDisconnect();
		if ( pChar->IsClient() ) pChar->GetClient()->CharDisconnect();
		m_pChar = pChar;
		m_pChar->ClientAttach(this);
	}
	CItem * clientLinger = m_pChar->LayerFind(LAYER_FLAG_ClientLinger);
	if ( clientLinger != NULL )
	{
		clientLinger->Delete();
	}

	m_Env.invalidate();
			
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

	xSend( &cmd, sizeof( cmd.Start ));
	
	ClearTargMode();	// clear death menu mode. etc. ready to walk about. cancel any previos modes
	
	addMap( NULL, true );
	
	// Here Mapdiffs

	addPlayerView(pt, true);
	
	addRedrawAll();
	addTime();

	m_pChar->MoveToChar( pt );	// Make sure we are in active list.
	m_pChar->Update();
	
	if ( pChar->m_pParty )
	{
		pChar->m_pParty->SendAddList(NULL);
	}

	addWeather(WEATHER_DEFAULT);
	addLight();
	addPlayerWarMode();
	addCharStatWindow(m_pChar->GetUID());
}

void CClient::addPlayerWarMode()
{
	CCommand cmd;
	cmd.War.m_Cmd = XCMD_War;
	cmd.War.m_warmode = ( m_pChar->IsStatFlag( STATF_War )) ? 1 : 0;
	cmd.War.m_unk2[0] = 0;
	cmd.War.m_unk2[1] = 0x32;	// ?
	cmd.War.m_unk2[2] = 0;
	xSend( &cmd, sizeof( cmd.War ));
}

bool CClient::addBookOpen( CItem * pBook )
{
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
		nPages = fWritable ? ( config.get("client.book.maxpages") ) : ( pMsgItem->GetPageCount());	// Max pages.
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
	xSend(&cmd, sizeof(cmd.BookOpen));

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

	return true;
}

void CClient::addBookPage( const CItem * pBook, int iPage )
{
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

	xSend( &cmd, length );
}

int CClient::Setup_FillCharList( CEventCharDef * pCharList, const CChar * pCharFirst )
{
	// list available chars for your account that are idle.
	CAccount * pAccount = GetAccount();
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
		UID uid( pAccount->m_Chars.GetChar(k));
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

	return iQty;
}

CLIMODE_TYPE CClient::GetTargMode() const
{
	return m_Targ_Mode;
}

void CClient::ClearTargMode()
{
	// done with the last mode.
	m_Targ_Mode = CLIMODE_NORMAL;
}

void CClient::SetTargMode( CLIMODE_TYPE targmode, LPCTSTR pPrompt )
{
	// ??? Get rid of menu stuff if previous targ mode.
	// Can i close a menu ?
	// Cancel a cursor input.

	if ( GetTargMode() == CLIMODE_TARG_USE_ITEM )
	{
		CItem * pItemUse = m_Targ_UID.ItemFind();
		if ( pItemUse )
		{
			if ( pItemUse->OnTrigger(ITRIG_TARGON_CANCEL, m_pChar) == TRIGRET_RET_TRUE )
			{
				m_Targ_Mode = targmode;
				if ( targmode != CLIMODE_NORMAL )
				{
					addSysMessage(pPrompt);
				}
				return;
			}
		}
	}

	if ( GetTargMode() == targmode ) return;

	if ( targmode == CLIMODE_NORMAL )
		addSysMessage("Targeting cancelled.");
	else if ( GetTargMode() != CLIMODE_NORMAL )
		addSysMessage("Previous targeting cancelled.");
	else
		addSysMessage(pPrompt);

	m_Targ_Mode = targmode;
}

void CClient::addPromptConsole( CLIMODE_TYPE targmode, LPCTSTR pPrompt )
{
	SetTargMode( targmode, pPrompt );

	CCommand cmd;
	cmd.Prompt.m_Cmd = XCMD_Prompt;
	cmd.Prompt.m_len = sizeof( cmd.Prompt );
	memset( cmd.Prompt.m_unk3, 0, sizeof(cmd.Prompt.m_unk3));
	cmd.Prompt.m_text[0] = '\0';

	xSend( &cmd, cmd.Prompt.m_len );
}

void CClient::addTarget( CLIMODE_TYPE targmode, LPCTSTR pPrompt, bool fAllowGround, bool fCheckCrime ) // Send targetting cursor to client
{
	// Expect XCMD_Target back.
	// ??? will this be selective for us ? objects only or chars only ? not on the ground (statics) ?
	SetTargMode( targmode, pPrompt );

	CCommand cmd;
	memset( &(cmd.Target), 0, sizeof( cmd.Target ));
	cmd.Target.m_Cmd = XCMD_Target;
	cmd.Target.m_TargType = fAllowGround; // fAllowGround;	// 1=allow xyz, 0=objects only.
	cmd.Target.m_context = targmode ;	// 5=my id code for action.
	cmd.Target.m_fCheckCrime = fCheckCrime; // // Not sure what this is. (m_checkcrimflag?)

	xSend( &cmd, sizeof( cmd.Target ));
}

void CClient::addTargetDeed( const CItem * pDeed )
{
	// Place an item from a deed. preview all the stuff
	ITEMID_TYPE iddef = (ITEMID_TYPE) RES_GET_INDEX( pDeed->m_itDeed.m_Type );
	m_tmUseItem.m_pParent = pDeed->GetParent();	// Cheat Verify.
	addTargetItems( CLIMODE_TARG_USE_ITEM, iddef );
}

bool CClient::addTargetChars( CLIMODE_TYPE mode, CREID_TYPE baseID, bool fNotoCheck )
{
	CCharBase * pBase = CCharBase::FindCharBase( baseID );
	if ( pBase == NULL )
		return false;

	TEMPSTRING(pszTemp);
	sprintf(pszTemp, "Where would you like to summon the '%s'?", (LPCTSTR) pBase->GetTradeName());

	addTarget(mode, pszTemp, true, fNotoCheck);
	return true;
}

bool CClient::addTargetItems( CLIMODE_TYPE targmode, ITEMID_TYPE id )
{
	// Add a list of items to place at target.
	// preview all the stuff

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

	TEMPSTRING(pszTemp);
	sprintf(pszTemp, "Where would you like to place the %s?", pszName);

	if ( CItemBase::IsID_Multi( id ))	// a multi we get from Multi.mul
	{
		SetTargMode(targmode, pszTemp);

		CCommand cmd;
		cmd.TargetMulti.m_Cmd = XCMD_TargetMulti;
		cmd.TargetMulti.m_fAllowGround = 1;
		cmd.TargetMulti.m_context = targmode ;	// 5=my id code for action.
		memset( cmd.TargetMulti.m_zero6, 0, sizeof(cmd.TargetMulti.m_zero6));
		cmd.TargetMulti.m_id = id - ITEMID_MULTI;

		// Add any extra stuff attached to the multi. preview this.

		memset( cmd.TargetMulti.m_zero20, 0, sizeof(cmd.TargetMulti.m_zero20));

		xSend( &cmd, sizeof( cmd.TargetMulti ));
		return true;
	}

	// preview not supported by this ver?
	addTarget(targmode, pszTemp, true);
	return true;
}

void CClient::addDyeOption( const CObjBase * pObj )
{
	// Put up the wHue chart for the client.
	// This results in a Event_Item_Dye message. CLIMODE_DYE
	ITEMID_TYPE id;
	if ( pObj->IsItem())
	{
		const CItem * pItem = dynamic_cast <const CItem*> (pObj);
		id = pItem->GetDispID();
	}
	else
	{
		// Get the item equiv for the creature.
		const CChar * pChar = dynamic_cast <const CChar*> (pObj);
		id = pChar->Char_GetDef()->m_trackID;
	}

	CCommand cmd;
	cmd.DyeVat.m_Cmd = XCMD_DyeVat;
	cmd.DyeVat.m_UID = pObj->GetUID();
	cmd.DyeVat.m_zero5 = pObj->GetHue();
	cmd.DyeVat.m_id = id;
	xSend( &cmd, sizeof( cmd.DyeVat ));
	SetTargMode( CLIMODE_DYE );
}

void CClient::addSkillWindow(SKILL_TYPE skill, bool bFromInfo) // Opens the skills list
{
	// Whos skills do we want to look at ?
	CChar *pChar = m_Prop_UID.CharFind();
	if ( !pChar )
		pChar = m_pChar;

	CCommand cmd;
	cmd.Skill.m_Cmd = XCMD_Skill;

	int len = sizeof(cmd.Skill.m_Cmd) + sizeof(cmd.Skill.m_len) + sizeof(cmd.Skill.m_single);

	if ( skill >= MAX_SKILL )
	{	// all skills
		CScriptTriggerArgs Args(-1, bFromInfo);
		if ( m_pChar->OnTrigger( CTRIG_UserSkills, pChar, &Args ) == TRIGRET_RET_TRUE )
			return;

		cmd.Skill_New.m_single = 0x02;
		
		int amount = 0;
		int i;
		for ( i = 0; i < MAX_SKILL; i++ )
		{
			if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(i) )
				continue;

			amount++;
			int iskillval = pChar->Skill_GetBase( (SKILL_TYPE) i);

			cmd.Skill_New.skills[i].m_index = i+1;
			cmd.Skill_New.skills[i].m_val = pChar->Skill_GetAdjusted( (SKILL_TYPE) i);
			cmd.Skill_New.skills[i].m_valraw = iskillval;
			cmd.Skill_New.skills[i].m_lock = pChar->Skill_GetLock( (SKILL_TYPE) i);
			cmd.Skill_New.skills[i].m_cap = (WORD) pChar->Skill_GetMax( (SKILL_TYPE) i);
		}

		cmd.Skill_New.skills[i].m_index = 0;	// terminator.
		len += (amount * sizeof(cmd.Skill_New.skills[0])) + sizeof(NWORD);
	}
	else
	{	// Just one skill update.
		CScriptTriggerArgs Args( skill );
		if ( m_pChar->OnTrigger( CTRIG_UserSkills, pChar, &Args ) == TRIGRET_RET_TRUE )
			return;

		if ( skill >= SKILL_SCRIPTED )
			return;

		len += sizeof(cmd.Skill.skills[0]);

		cmd.Skill_New.m_single = 0xdf;

		int iskillval = pChar->Skill_GetBase( skill );

		cmd.Skill_New.skills[0].m_index = skill;
		cmd.Skill_New.skills[0].m_val = pChar->Skill_GetAdjusted( skill );
		cmd.Skill_New.skills[0].m_valraw = iskillval;
		cmd.Skill_New.skills[0].m_lock = pChar->Skill_GetLock( skill );
		cmd.Skill_New.skills[0].m_cap = (WORD) pChar->Skill_GetMax( skill );
		len += sizeof(cmd.Skill_New.skills[0].m_cap);
	}

	cmd.Skill_New.m_len = len;

	xSend( &cmd, len );
}

void CClient::addPlayerSee( const CPointMap & ptold )
{
	// Adjust to my new location, what do I now see here?
	bool fAllShow = IsPriv(PRIV_ALLSHOW);

	CPointMap pt = m_pChar->GetTopPoint();
	CRegionBase * pCurrentCharRegion = pt.GetRegion(REGION_TYPE_MULTI);

	//	Items on the ground
	CWorldSearch AreaItems(pt, UO_MAP_VIEW_RADAR);
	AreaItems.SetAllShow(fAllShow);
	DWORD	dSeeItems = 0;
	IT_TYPE pCType;

	while ( CItem *pItem = AreaItems.GetItem() )
	{
		if ( !CanSee(pItem) ) continue;

		if ( pItem->GetTopLevelObj()->GetTopPoint().GetRegion(REGION_TYPE_MULTI) != pCurrentCharRegion
			&& pItem->GetType() != IT_MULTI
			&& pItem->GetType() != IT_SHIP )
		{
			if ( pItem->m_uidLink.IsValidUID() && pItem->m_uidLink.IsItem() )
			{
				pCType = pItem->m_uidLink.ItemFind()->GetType();
				if ( pCType != IT_MULTI && pCType != IT_SHIP )
				{
					addObjectRemove( pItem ); // need this to remove old items
					continue;
				}
			}
			else
			{
				addObjectRemove( pItem ); // need this to remove old items
				continue;
			}
		}
		// I didn't see it before
		if ( ptold.GetDist(pItem->GetTopPoint()) > UO_MAP_VIEW_SIZE || ptold.GetRegion(REGION_TYPE_MULTI) != pCurrentCharRegion )
		{
			if ( dSeeItems < g_Cfg.m_iMaxItemComplexity*30 )
			{
				dSeeItems++;
				addItem_OnGround(pItem);
			}
		}
	}

	//	Characters around
	CWorldSearch AreaChars(pt, UO_MAP_VIEW_SIZE, m_pChar);
	AreaChars.SetAllShow(fAllShow);
	DWORD	dSeeChars(0);
	while ( CChar *pChar = AreaChars.GetChar() )
	{
		if ( !CanSee(pChar) ) continue;

		if ( ptold.GetDist(pChar->GetTopPoint()) > UO_MAP_VIEW_SIZE )
		{
			if ( dSeeChars < g_Cfg.m_iMaxCharComplexity*5 )
			{
				dSeeChars++;
				addChar(pChar);
			}
		}
	}
}

void CClient::addPlayerView( const CPointMap & ptold, bool playerStart )
{
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

	xSend( &cmd, sizeof( cmd.View ));

	// resync this stuff.
	m_net->m_sequence = 0;

	if ( ptold == pt )
		return;	// not a real move i guess. might just have been a change in face dir.

	m_Env.invalidate();	// Must resend environ stuff.

	// What can i see here ?
	if ( !playerStart )
		addPlayerSee( ptold );
}

void CClient::addReSync(bool bForceMap)
{
	// Reloads the client with all it needs.
	CPointMap ptold;	// invalid value.
	addMap(NULL, !bForceMap);
	addPlayerView(ptold);
	addChar( m_pChar );
	addLight();		// Current light level where I am.
	addSpeedMode( m_pChar->m_pPlayer->m_speedMode );
}

void CClient::addMap( const CPointMap * pOldP, bool playerStart)
{
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

void CClient::UpdateStats()
{
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

void CClient::addCharStatWindow( UID uid, bool fRequested ) // Opens the status window
{
	CChar * pChar = uid.CharFind();
	if ( !pChar )
		return;

	if ( IsTrigUsed(TRIGGER_USERSTATS) )
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
		// 0 not valid, 1 = till weight, 2 = till statcap, 3 till follower, 4 = till the end, 5 = include maxweight+race.
		len += sizeof(cmd.StatusNew);
		cmd.Status.m_ValidStats = 5;
		bStatLocks = true;

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
		cmd.Status.m_gold = pChar->ContentCount(RESOURCE_ID(RES_TYPEDEF,IT_GOLD));	/// ??? optimize this count is too often.
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
			cmd.Status.m_resFire = pChar->m_pPlayer->m_resFire;
			cmd.Status.m_resCold = pChar->m_pPlayer->m_resCold;
			cmd.Status.m_resPoison = pChar->m_pPlayer->m_resPoison;
			cmd.Status.m_resEnergy = pChar->m_pPlayer->m_resEnergy;
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
			cmd.StatusNew.m_resEnergy	= cmd.Status.m_resEnergy;
			cmd.StatusNew.m_resPoison	= cmd.Status.m_resPoison;
			cmd.StatusNew.m_resCold		= cmd.Status.m_resCold;
			cmd.StatusNew.m_resFire		= cmd.Status.m_resFire;
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
		int iMaxHits = max(pChar->Stat_GetMax(STAT_STR),1);
		cmd.Status.m_maxhealth = 100;
		cmd.Status.m_health = (pChar->Stat_GetVal(STAT_STR) * 100) / iMaxHits;
		cmd.Status.m_len = ((BYTE*)&(cmd.Status.m_sex)) - ((BYTE*)&(cmd.Status));
	}

	xSend(&cmd, cmd.Status.m_len);

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

void CClient::addHitsUpdate( UID uid )
{
	CChar * pChar = uid.CharFind();
	if ( pChar == NULL )
		return;

	CCommand cmd;
	cmd.StatChng.m_Cmd = XCMD_StatChngStr;
	cmd.StatChng.m_UID = pChar->GetUID();
	if ( m_pChar != pChar )
	{
		cmd.StatChng.m_max = 50;
		cmd.StatChng.m_val = ( pChar->Stat_GetVal(STAT_STR) * 50 ) / pChar->Stat_GetMax(STAT_STR);
	}
	else
	{
		cmd.StatChng.m_max = pChar->Stat_GetMax(STAT_STR);
		cmd.StatChng.m_val = pChar->Stat_GetVal(STAT_STR);
	}
	xSend( &cmd, sizeof(cmd.StatChng) );

	if (( pChar->m_pParty == NULL ) || ( m_pChar != pChar ))
		return;

	pChar->m_pParty->AddStatsUpdate( pChar, &cmd, sizeof(cmd.StatChng) );
}

void CClient::addManaUpdate( UID uid )
{
	CChar * pChar = uid.CharFind();
	if ( pChar == NULL )
		return;

	CCommand cmd;
	cmd.StatChng.m_Cmd = XCMD_StatChngInt;
	cmd.StatChng.m_UID = pChar->GetUID();
	cmd.StatChng.m_max = pChar->Stat_GetMax(STAT_INT);
	cmd.StatChng.m_val = pChar->Stat_GetVal(STAT_INT);
	xSend( &cmd, sizeof(cmd.StatChng) );

	if (( pChar->m_pParty == NULL ) || ( m_pChar != pChar ))
		return;

	pChar->m_pParty->AddStatsUpdate( pChar, &cmd, sizeof(cmd.StatChng) );
}

void CClient::addStamUpdate( UID uid )
{
	CChar * pChar = uid.CharFind();
	if ( pChar == NULL )
		return;

	CCommand cmd;
	cmd.StatChng.m_Cmd = XCMD_StatChngDex;
	cmd.StatChng.m_UID = pChar->GetUID();
	cmd.StatChng.m_max = pChar->Stat_GetMax(STAT_DEX);
	cmd.StatChng.m_val = pChar->Stat_GetVal(STAT_DEX);
	xSend( &cmd, sizeof(cmd.StatChng) );

	if (( pChar->m_pParty == NULL ) || ( m_pChar != pChar ))
		return;

	pChar->m_pParty->AddStatsUpdate( pChar, &cmd, sizeof(cmd.StatChng) );
}

void CClient::addSpellbookOpen( CItem * pBook, WORD offset )
{

	if ( !m_pChar )
		return;

	if ( IsTrigUsed(TRIGGER_SPELLBOOK) )
	{
		CScriptTriggerArgs	Args(0, 0, pBook);
		if ( m_pChar->OnTrigger(CTRIG_SpellBook, m_pChar, &Args) == TRIGRET_RET_TRUE )
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

	addOpenGump(pBook, GUMP_OPEN_SPELLBOOK);

	// Handle new AOS spellbook stuff (old packets no longer work)
	CExtData data;
	data.NewSpellbook.m_Unk1 = 1;
	data.NewSpellbook.m_UID = pBook->GetUID();
	data.NewSpellbook.m_ItemId = pBook->GetDispID();
	data.NewSpellbook.m_Offset = offset; // 1 = normal, 101 = necro, 201 = paladin
	data.NewSpellbook.m_Content[0] = pBook->m_itSpellbook.m_spells1;
	data.NewSpellbook.m_Content[1] = pBook->m_itSpellbook.m_spells2;

	addExtData( EXTDATA_NewSpellbook, &data, sizeof( data.NewSpellbook ) );
}


void CClient::addCustomSpellbookOpen( CItem * pBook, DWORD gumpID )
{
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
		cmd.Content.m_item[j].m_x	= 0x48;	// may not mean anything ?
		cmd.Content.m_item[j].m_y	= 0x7D;
		cmd.Content.m_item[j].m_UIDCont = pBook->GetUID();
		cmd.Content.m_item[j].m_wHue = HUE_DEFAULT;
		j++;
	}

	xSend( &cmd, len );
}

void CClient::addShopMenuBuy(CChar *pVendor)
{
	// Try to buy stuff that the vendor has.
	if ( !pVendor || !pVendor->NPC_IsVendor() ) return;

	//	non-player vendors could be restocked on-the-fly
	if ( !pVendor->IsStatFlag(STATF_Pet) )
	{
		if ( !pVendor->m_pNPC->m_timeRestock.IsTimeValid() )
			pVendor->NPC_Vendor_Restock();

		//	and remember that I was asked during the last minutes
		pVendor->m_pNPC->m_timeRestock.SetCurrentTime();
	}

	CScriptTriggerArgs args;
	args.m_iN1 = m_pChar->ContentCount(RESOURCE_ID(RES_TYPEDEF,IT_GOLD));
	pVendor->r_Call("f_onserver_buy", this, &args);
}

void CClient::addShopMenuSell(CChar *pVendor)
{
	// Player selling to vendor.
	// What things do you have in your inventory that the vendor would want ?
	if ( !pVendor || !pVendor->NPC_IsVendor() ) return;

	//	non-player vendors could be restocked on-the-fly
	if ( !pVendor->IsStatFlag(STATF_Pet) )
	{
		if ( !pVendor->m_pNPC->m_timeRestock.IsTimeValid() )
			pVendor->NPC_Vendor_Restock();

		//	and remember that I was asked during the last minutes
		pVendor->m_pNPC->m_timeRestock.SetCurrentTime();
	}

	pVendor->r_Call("f_onsell", m_pChar, NULL);
}

void CClient::addBankOpen( CChar * pChar, LAYER_TYPE layer )
{
	// open it up for this pChar.
	CItemContainer *pBankBox = pChar->GetBank(layer);
	addItem(pBankBox);	// may crash client if we dont do this.

	pBankBox->OnOpenEvent(m_pChar, pChar);
	addContainerSetup(pBankBox);
}

void CClient::addMap( CItemMap * pMap )
{
	// Make player drawn maps possible. (m_map_type=0) ???
	if ( !pMap ) return;

	CRectMap rect;
	rect.SetRect( pMap->m_itMap.m_left, pMap->m_itMap.m_top, pMap->m_itMap.m_right, pMap->m_itMap.m_bottom, GetChar()->GetTopPoint().m_map);

	if ( pMap->IsType(IT_MAP_BLANK) || !rect.IsValid() || rect.IsRectEmpty() )
	{
		addSysMessage("This map is blank.");
		return;
	}

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
	xSend( &cmd, sizeof( cmd.MapDisplay ));

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
		xSend( &cmd, sizeof( cmd.MapEdit ));
	}
}

void CClient::addMapMode( CItemMap * pMap, MAPCMD_TYPE iType, bool fEdit )
{
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
	xSend( &cmd, sizeof( cmd.MapEdit ));
}

void CClient::addBulletinBoard( const CItemContainer * pBoard )
{
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
	xSend( &cmd, len );

	// Send Content messages for all the items on the bboard.
	// Not sure what x,y are here, date/time maybe ?
	addContents( pBoard, false, false, false );

	// The client will now ask for the headers it wants.
}

bool CClient::addBBoardMessage( const CItemContainer * pBoard, BBOARDF_TYPE flag, UID uidMsg )
{
	CItemMessage * pMsgItem = dynamic_cast <CItemMessage *> ( uidMsg.ItemFind());
	if ( ! pBoard->IsItemInside( pMsgItem ))
		return false;

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

	TEMPSTRING(pszTemp);
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
	xSend( &cmd, len );
	return true;
}

void CClient::addRedrawAll()
{
	CCommand cmd;
	cmd.ReDrawAll.m_Cmd = XCMD_ReDrawAll;
	xSend( &cmd, sizeof(cmd.ReDrawAll));
}

void CClient::addChatSystemMessage( CHATMSG_TYPE iType, LPCTSTR pszName1, LPCTSTR pszName2, CLanguageID lang )
{
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
	xSend( &cmd, len );
}

void CClient::addItemMenu( CLIMODE_TYPE mode, const CMenuItem * item, int count, CObjBase * pObj )
{
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
			g_Log.Error("Bad option length %d in menu item %d\n", lenitem, i);
			continue;
		}

		pCmd->MenuItems.m_item[0].m_id = item[i].m_id; // image next to menu.
		pCmd->MenuItems.m_item[0].m_check = 0;	// check or not ?
		pCmd->MenuItems.m_item[0].m_lentext = lenitem;
		strcpy( pCmd->MenuItems.m_item[0].m_name, item[i].m_sText );

		lenitem += sizeof( cmd.MenuItems.m_item[0] ) - 1;
		pCmd = (CCommand *)(((BYTE*)pCmd) + lenitem );
		len += lenitem;
	}

	cmd.MenuItems.m_len = len;
	xSend( &cmd, len );

	m_tmMenu.m_UID = pObj->GetUID();

	SetTargMode( mode );
}

void CClient::addCharPaperdoll( CChar * pChar )
{
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

	xSend( &cmd, sizeof( cmd.PaperDoll ));
}

void CClient::addAOSTooltip( const CObjBase * pObj, bool bRealTooltip )
{
	if ( !pObj )
		return;

	if ( !IsAosFlagEnabled(FEATURE_AOS_UPDATE_B) )
		return;

	// We check here if we are sending a tooltip for a static/non-movable items
	// (client doesn't expect us to) but only in the world
	if ( pObj->IsItem() )
	{
		CItem * pItem = (CItem *) dynamic_cast <const CItem *> ( pObj );

		if ( !pItem->GetContainer() && pItem->IsAttr(ATTR_STATIC) )
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

	DEBUG_MSG(("Sending tooltip for 0%x (%s)\n", pObj->GetUID(), pObj->GetName()));

	TRIGRET_TYPE iRet = TRIGRET_RET_FALSE;
	if ( IsTrigUsed(TRIGGER_CLIENTTOOLTIP) )
	{
		CScriptTriggerArgs args((CScriptObj *)pObj);
		if ( pItem )
			iRet = pItem->OnTrigger(ITRIG_CLIENTTOOLTIP, this->GetChar(), &args);
		else if ( pChar )
			iRet = pChar->OnTrigger(CTRIG_ClientTooltip, this->GetChar(), &args);
	}

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

			if ( ! *lpPrefix )
				lpPrefix = pChar->Noto_GetFameTitle();

			if ( ! *lpPrefix )
				lpPrefix = " ";

			TEMPSTRING(lpSuffix);
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

	xSend(&pack, len);
}

void CClient::addSpeedMode( int speedMode )
{
	CExtData cmdData;
	cmdData.SpeedMode.m_speed = speedMode;
	addExtData( EXTDATA_SpeedMode, &cmdData, sizeof( cmdData.SpeedMode ) );
}

void CClient::addIdleWarning( bool bSameChar )
{
	CCommand cmd;
	cmd.IdleWarning.m_Cmd = XCMD_IdleWarning;
	cmd.IdleWarning.m_Value = (bSameChar) ? 0x05 : 0x07;
	xSend(&cmd, sizeof(cmd.IdleWarning));
}


// --------------------------------------------------------------------
void CClient::SendPacket( TCHAR * pszKey )
{
	BYTE	*pszTmp = (BYTE *)CThread::Thread()->TempString();
	int	iLen;
	DWORD	iVal;

	iLen	= -1;
	while ( *pszKey )
	{
		if ( iLen > SCRIPT_MAX_LINE_LEN - 4 )
		{	// we won't get here because this lenght is enforced in all scripts
			g_Log.Error("SENDPACKET too big.\n");
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

	xSend( (void*) &pszTmp[0], iLen+1 );
}

// ---------------------------------------------------------------------
// Login type stuff.

bool CClient::Setup_Start( CChar * pChar ) // Send character startup stuff to player
{
	// Play this char.
	TEMPSTRING(z);
	CharDisconnect();	// I'm already logged in as someone else ?

	g_Log.Event(LOGM_CLIENTS_LOG, "%x:Setup_Start acct='%s', char='%s'\n",
		socketId(), GetAccount()->GetName(), pChar->GetName());

	bool fQuickLogIn = false;
	bool fNoMessages = false;
	if ( !pChar->IsDisconnected() )
	{
		// The players char is already in game ! Client linger time re-login.
		fQuickLogIn = true;
	}

	addPlayerStart( pChar );
	// Gump memory cleanup, we don't want them from logged out players
	m_pChar->Memory_ClearTypes(MEMORY_GUMPRECORD);

	//	gms should login with invul and without allshow flag set
	if ( GetPrivLevel() >= PLEVEL_Counsel )
	{
		if ( IsPriv(PRIV_ALLSHOW) ) ClearPrivFlags(PRIV_ALLSHOW);
		if ( !pChar->IsStatFlag(STATF_INVUL) ) pChar->StatFlag_Set(STATF_INVUL);
	}

	CScriptTriggerArgs	Args( fNoMessages, fQuickLogIn, NULL );

	if ( pChar->OnTrigger( CTRIG_LogIn, pChar, &Args ) == TRIGRET_RET_TRUE )
	{
		m_pChar->ClientDetach();
		pChar->SetDisconnected();
		addLoginErr(PacketLoginError::Blocked);
		return false;
	}

	fNoMessages	= (Args.m_iN1 != 0);
	fQuickLogIn	= (Args.m_iN2 != 0);

	if ( !fQuickLogIn )
	{
		if ( !fNoMessages )
		{
			addBark(SPHERE_FULL, NULL, HUE_YELLOW, TALKMODE_SYSTEM, FONT_NORMAL);

			sprintf(z, (g_Serv.StatGet(SERV_STAT_CLIENTS) <= 2) ?
				g_Cfg.GetDefaultMsg( DEFMSG_LOGIN_PLAYER ) : g_Cfg.GetDefaultMsg( DEFMSG_LOGIN_PLAYERS ),
				g_Serv.StatGet(SERV_STAT_CLIENTS)-1 );
			addSysMessage(z);

			sprintf(z, "Last logged: %s", GetAccount()->m_TagDefs.GetKeyStr("LastLogged"));
			addSysMessage(z);
		}
		if ( m_pChar->m_pArea && m_pChar->m_pArea->IsGuarded() && !m_pChar->m_pArea->IsFlag(REGION_FLAG_ANNOUNCE) )
		{
			VariableList::Variable *pVarStr = m_pChar->m_pArea->m_TagDefs.GetKey("GUARDOWNER");
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
		sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_GMPAGES), g_World.m_GMPages.GetCount());
		addSysMessage(z);
	}
	if ( IsPriv(PRIV_JAILED) )
	{
		m_pChar->Jail(&g_Serv, true, 0);
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
		addSysMessage(g_Cfg.GetDefaultMsg( (i < 100 ) ? DEFMSG_REGION_WATER_1 : DEFMSG_REGION_WATER_2 ));
	}

	DEBUG_MSG(("%x:Setup_Start done\n", socketId()));
	return true;
}

void CClient::Setup_CreateDialog( const CEvent * pEvent ) // All the character creation stuff
{
	if ( m_pChar != NULL )
	{
		// Loggin in as a new player while already on line !
		addSysMessage( g_Cfg.GetDefaultMsg( DEFMSG_ALREADYONLINE ) );
		g_Log.Error("%x:Setup_CreateDialog acct='%s' already on line!\n",
			socketId(), GetAccount()->GetName());
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
			addLoginErr(PacketLoginError::Other);
			return;
		}
	}

	CChar * pChar = CChar::CreateBasic( CREID_MAN );
	pChar->InitPlayer( pEvent, this );

	g_Log.Event(LOGM_CLIENTS_LOG, "%x:Setup_CreateDialog acct='%s', char='%s'\n",
		socketId(), GetAccount()->GetName(), pChar->GetName());

	enum TRIGRET_TYPE	tr;
	CScriptTriggerArgs createArgs;
	createArgs.m_iN1 = (DWORD) pEvent->Create.m_flags;
	createArgs.m_iN2 = (int) pEvent->Create.m_prof;
	createArgs.m_iN3 = ((pEvent->Create.m_sex - 2) >= 0);
	createArgs.m_s1 = GetAccount()->GetName();

	r_Call("f_onchar_create", pChar, &createArgs, NULL, &tr);

	Setup_Start( pChar );
}

bool CClient::Setup_Play( int iSlot ) // After hitting "Play Character" button
{
	// Mode == CLIMODE_SETUP_CHARLIST

	DEBUG_MSG(("%x:Setup_Play slot %d\n", socketId(), iSlot));

	if ( ! GetAccount())
		return false;
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
		return false;
	}

	return Setup_Start( pChar );
}

DELETE_ERR_TYPE CClient::Setup_Delete( int iSlot ) // Deletion of character
{
	DEBUG_MSG(("%x:Setup_Delete slot=%d\n", socketId(), iSlot));
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
	pChar->r_Call("f_onchar_delete", pChar, NULL, NULL, &tr);
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
	xSend( &cmd, len );

	return( DELETE_SUCCESS );
}

BYTE CClient::Setup_ListReq( const char * pszAccName, const char * pszPassword, bool fTest )
{
	// XCMD_CharListReq
	// Gameserver login and request character listing

	if ( GetConnectType() != CONNECT_GAME )	// Not a game connection ?
	{
		return PacketLoginError::Other;
	}

	switch ( GetTargMode())
	{
		case CLIMODE_SETUP_RELAY:
			ClearTargMode();
			break;
	}

	CGString sMsg;
	BYTE lErr = PacketLoginError::Other;

	lErr = LogIn( pszAccName, pszPassword, sMsg );

	if ( lErr != PacketLoginError::Success )
	{
		if ( fTest && lErr != PacketLoginError::Other )
		{
			if ( ! sMsg.IsEmpty())
			{
				SysMessage( sMsg );
			}
			addLoginErr(lErr);
		}
		return( lErr );
	}

	CCommand cmd;
	cmd.FeaturesEnable.m_Cmd = XCMD_Features;
	cmd.FeaturesEnable.m_enable = g_Cfg.GetPacketFlag(false);
	xSend(&cmd, sizeof(cmd.FeaturesEnable));

	cmd.CharList.m_Cmd = XCMD_CharList;
	int len = sizeof( cmd.CharList ) - sizeof(cmd.CharList.m_start) + ( g_Cfg.m_StartDefs.GetCount() * sizeof(cmd.CharList.m_start[0]));
	cmd.CharList.m_len = len;
	NDWORD *	flags	= (NDWORD*) (&(cmd.CharList.m_Cmd) + len - sizeof(NDWORD));

	*flags	= g_Cfg.GetPacketFlag(true);

	// list chars to your account that may still be logged in !
	// "LASTCHARUID" = list this one first.
	cmd.CharList.m_count = Setup_FillCharList(cmd.CharList.m_char, GetAccount()->m_uidLastChar.CharFind());

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
	xSend(&cmd, len);
	m_Targ_Mode = CLIMODE_SETUP_CHARLIST;
	return PacketLoginError::Success;
}

BYTE CClient::LogIn( CAccountRef pAccount, CGString & sMsg )
{
	if ( !pAccount )
		return PacketLoginError::Invalid;

	if ( pAccount->IsPriv( PRIV_BLOCKED ))
	{
		sMsg.Format( g_Cfg.GetDefaultMsg( DEFMSG_ACC_BLOCKED ), (LPCTSTR) g_Serv.m_sEMail );
		return PacketLoginError::Blocked;
	}

	// Look for this account already in use.
	CClient * pClientPrev = pAccount->FindClient( this );
	if ( pClientPrev != NULL )
	{
		// Only if it's from a diff ip ?
		bool bInUse = false;

		//	different ip - no reconnect
		if ( !peer().IsSameIP(pClientPrev->peer()) )
			bInUse = true;
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
					pClientPrev->m_net->markClose();
				}
				else if ( GetConnectType() == pClientPrev->GetConnectType() ) bInUse = true;
			}
		}

		if ( bInUse )
		{
			sMsg = "Account already in use.";
			return PacketLoginError::InUse;
		}
	}

	if ( pAccount->GetPrivLevel() < PLEVEL_GM && g_Serv.StatGet(SERV_STAT_CLIENTS) > config.get("network.client.max") )
	{
		// Give them a polite goodbye.
		sMsg = g_Cfg.GetDefaultMsg( DEFMSG_SERV_FULL );
		return PacketLoginError::Blocked;
	}
	//	Do the scripts allow to login this account?
	pAccount->m_Last_IP = peer();
	CScriptTriggerArgs Args;
	Args.Init(pAccount->GetName());
	Args.m_iN1 = GetConnectType();
	enum TRIGRET_TYPE tr;
	g_Serv.r_Call("f_onaccount_login", &g_Serv, &Args, NULL, &tr);
	if ( tr == TRIGRET_RET_TRUE )
	{
		sMsg = g_Cfg.GetDefaultMsg( DEFMSG_ACC_DENIED );
		return PacketLoginError::Blocked;
	}

	m_pAccount = pAccount;
	pAccount->OnLogin( this );

	return PacketLoginError::Success;
}

BYTE CClient::LogIn( LPCTSTR pszAccName, LPCTSTR pszPassword, CGString & sMsg )
{
	// Try to validate this account.
	// Do not send output messages as this might be a console or web page or game client.
	// NOTE: addLoginErr() will get called after this.

	if ( GetAccount() ) // already logged in.
		return PacketLoginError::Success;

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
		return PacketLoginError::Other;
	}

	iLen3 = Str_GetBare( szTmp, pszPassword, MAX_NAME_SIZE );
	if ( iLen2 != iLen3 )	// a corrupt message.
		goto badformat;


	TCHAR szName[ MAX_ACCOUNT_NAME_SIZE ];
	if ( !CAccount::NameStrip(szName, pszAccName) || Str_Check(pszAccName) || Str_Check(pszPassword) )
	{
		return PacketLoginError::Other;
	}

	bool fGuestAccount = ! strnicmp( pszAccName, "GUEST", 5 );
	if ( fGuestAccount )
	{
		// trying to log in as some sort of guest.
		// Find or create a new guest account.
		TEMPSTRING(pszTemp);
		for ( int i=0; 1; i++ )
		{
			if ( i>=g_Cfg.m_iGuestsMax )
			{
				sMsg = g_Cfg.GetDefaultMsg( DEFMSG_ACC_GUSED );
				return PacketLoginError::Blocked;
			}

			sprintf(pszTemp, "GUEST%d", i);
			CAccountRef pAccount = g_Accounts.Account_FindCreate(pszTemp, true );
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
			return PacketLoginError::BadPass;
		}
	}

	CAccountRef pAccount = g_Accounts.Account_FindCreate(pszAccName);
	if ( ! pAccount )
	{
		g_Log.Event(LOGM_CLIENTS_LOG, "%x:ERR Login NO Account '%s'\n", socketId(), pszAccName);
		sMsg.Format(g_Cfg.GetDefaultMsg(DEFMSG_ACC_UNK), pszAccName);
		return PacketLoginError::Invalid;
	}

	if ( ! fGuestAccount && ! pAccount->IsPriv(PRIV_BLOCKED) )
	{
		if ( ! pAccount->CheckPassword(pszPassword))
		{
			g_Log.Event(LOGM_CLIENTS_LOG, "%x: '%s' bad password\n", socketId(), pAccount->GetName());
			sMsg = g_Cfg.GetDefaultMsg(DEFMSG_ACC_BADPASS);
			return PacketLoginError::BadPass;
		}
	}

	return LogIn(pAccount, sMsg);
}
