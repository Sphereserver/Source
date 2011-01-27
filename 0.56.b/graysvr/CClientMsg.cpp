//
// CClientMsg.cpp
// Copyright Menace Software (www.menasoft.com).
//
// Game server messages. (No login stuff)
//

#include "graysvr.h"	// predef header.
#include "CClient.h"
#include "../network/network.h"
#include "../network/send.h"

// Simple string hashing algorithm function
// Founded by D. J. Bernstein
// Original code found at: http://www.cse.yorku.ca/~oz/hash.html
unsigned long HashString(LPCTSTR str, size_t length)
{
    unsigned long hash = 5381;
    for (size_t i = 0; i < length; i++)
	    hash = ((hash << 5) + hash) + *str++;

    return hash;
}

/////////////////////////////////////////////////////////////////
// -CClient stuff.

void CClient::resendBuffs()
{
	// These checks are in addBuff too, but it would be useless to call it so many times
	if ( !IsSetOF(OF_Buffs) )
		return;
	if ( !IsResClient(RDS_AOS))
		return;
	if ( PacketBuff::CanSendTo(GetNetState()) == false )
		return;

	CContainer* Cont = dynamic_cast<CContainer*>(GetChar());
	ASSERT(Cont);

	CItem* pSpell = 0;
	TCHAR NumBuff[3][4];
	LPCTSTR pNumBuff[3] = {(LPCTSTR)NumBuff[0], (LPCTSTR)NumBuff[1], (LPCTSTR)NumBuff[2]};
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
			ITOA(iBuffPercent, NumBuff[0], 10);
			addBuff( BI_CLUMSY, 1075831,1075832,(WORD)(pSpell->GetTimerAdjusted()), pNumBuff, 1 );
			break;
		case SPELL_Weaken:

			iBuffPercent = GetStatPercentage( GetChar(), STAT_STR, iStatEffect );
			ITOA(iBuffPercent, NumBuff[0], 10);
			addBuff( BI_WEAKEN, 1075837,1075838,(WORD)(pSpell->GetTimerAdjusted()), pNumBuff, 1 );
			break;
		case SPELL_Feeblemind:
			iBuffPercent = GetStatPercentage( GetChar(), STAT_INT, iStatEffect );
			ITOA(iBuffPercent, NumBuff[0], 10);
			addBuff( BI_FEEBLEMIND, 1075833,1075834,(WORD)(pSpell->GetTimerAdjusted()), pNumBuff, 1  );
			break;
		case SPELL_Curse:
		{
			for( int idx = STAT_STR; idx != STAT_BASE_QTY; ++idx)
			{
				iBuffPercent = GetStatPercentage( GetChar(), static_cast<STAT_TYPE>(idx), iStatEffect );
				ITOA(iBuffPercent, NumBuff[idx], 10);
			}
			addBuff( BI_CURSE, 1075835,1075840,(WORD)(pSpell->GetTimerAdjusted()), pNumBuff, STAT_BASE_QTY );
			break;
		}
		case SPELL_Strength:
			iBuffPercent = GetStatPercentage( GetChar(), STAT_STR, iStatEffect );
			ITOA(iBuffPercent, NumBuff[0], 10);
			addBuff( BI_STRENGTH, 0x106A85,0x106A86,(WORD)(pSpell->GetTimerAdjusted()), pNumBuff, 1 );
			break;
		case SPELL_Agility:
			iBuffPercent = GetStatPercentage( GetChar(), STAT_DEX, iStatEffect );
			ITOA(iBuffPercent, NumBuff[0], 10);
			addBuff( BI_AGILITY, 0x106A85,0x106A86,(WORD)(pSpell->GetTimerAdjusted()), pNumBuff, 1 );
			break;
		case SPELL_Cunning:
			iBuffPercent = GetStatPercentage( GetChar(), STAT_INT, iStatEffect );
			ITOA(iBuffPercent, NumBuff[0], 10);
			addBuff( BI_CUNNING, 0x106A85,0x106A86,(WORD)(pSpell->GetTimerAdjusted()), pNumBuff, 1 );
			break;
		case SPELL_Bless:
		{
			for( int idx = STAT_STR; idx != STAT_BASE_QTY; ++idx)
			{
				iBuffPercent = GetStatPercentage( GetChar(), static_cast<STAT_TYPE>(idx), iStatEffect );
				ITOA(iBuffPercent, NumBuff[idx], 10);
			}
			addBuff( BI_BLESS, 1075847,1075848,(WORD)(pSpell->GetTimerAdjusted()), pNumBuff, STAT_BASE_QTY );
			break;
		}
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
		default:
			break;
		}

	}

}
void CClient::addBuff( const WORD IconId, const DWORD ClilocOne, const DWORD ClilocTwo, const short Time, LPCTSTR* pArgs, int iArgCount)
{
	ADDTOCALLSTACK("CClient::addBuff");
	if ( !IsSetOF(OF_Buffs) )
		return;
	if ( !IsResClient(RDS_AOS))
		return;
	if ( PacketBuff::CanSendTo(GetNetState()) == false )
		return;

	new PacketBuff(this, IconId, ClilocOne, ClilocTwo, Time, pArgs, iArgCount);
}

void CClient::removeBuff (const WORD IconId)
{
	ADDTOCALLSTACK("CClient::removeBuff");
	if ( !IsSetOF(OF_Buffs) )
		return;
	if ( !IsResClient(RDS_AOS))
		return;
	if ( PacketBuff::CanSendTo(GetNetState()) == false )
		return;

	new PacketBuff(this, IconId);
}


bool CClient::addDeleteErr(BYTE code)
{
	ADDTOCALLSTACK("CClient::addDeleteErr");
	// code
	if (code == PacketDeleteError::Success)
		return true;

	DEBUG_ERR(( "%lx:Bad Char Delete Attempted %d\n", GetSocketID(), code ));
	new PacketDeleteError(this, (PacketDeleteError::Reason)code);
	return( false );
}

void CClient::addTime( bool bCurrent )
{
	ADDTOCALLSTACK("CClient::addTime");
	// Send time. (real or game time ??? why ?)
	PacketGameTime* cmd(NULL);

	if ( bCurrent )
	{
		long lCurrentTime = (CServTime::GetCurrentTime()).GetTimeRaw();
		cmd = new PacketGameTime(this, 
								( lCurrentTime / ( 60*60*TICK_PER_SEC )) % 24,
								( lCurrentTime / ( 60*TICK_PER_SEC )) % 60,
								( lCurrentTime / ( TICK_PER_SEC )) % 60);
	}
	else
	{
		cmd = new PacketGameTime(this);
	}
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
	new PacketRemoveObject(this, uid);
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
		AreaItems.SetSearchSquare(true);
		for (;;)
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
		AreaChars.SetSearchSquare(true);
		for (;;)
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
	
	if ( PacketItemWorldNew::CanSendTo(GetNetState()) )
		new PacketItemWorldNew(this, pItem);
	else
		new PacketItemWorld(this, pItem);

	// send KR drop confirmation
	if ( PacketDropAccepted::CanSendTo(GetNetState()) )
		new PacketDropAccepted(this);

	// send item sound
	if (pItem->IsType(IT_SOUND))
	{
		addSound( (SOUND_TYPE) pItem->m_itSound.m_Sound, pItem, pItem->m_itSound.m_Repeat );
	}

	// send corpse clothing
	if (IsPriv(PRIV_DEBUG) == false && (pItem->GetDispID() == ITEMID_CORPSE && CCharBase::IsHumanID(pItem->GetCorpseType())) )	// cloths on corpse
	{
		CItemCorpse* pCorpse = dynamic_cast <CItemCorpse*> (pItem);
		if (pCorpse != NULL)
		{
			// send all the items on the corpse.
			addContents( pCorpse, false, true, false );
			// equip the proper items on the corpse.
			addContents( pCorpse, true, true, false );
		}
	}

	// send item tooltip
	addAOSTooltip(pItem);

	if (pItem->IsType(IT_MULTI_CUSTOM))
	{
		// send house design version
		CItemMultiCustom * pItemMulti = dynamic_cast <CItemMultiCustom*> (pItem);
		if (pItemMulti != NULL)
			pItemMulti->SendVersionTo(this);
	}
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

	new PacketItemEquipped(this, pItem);

	addAOSTooltip( pItem );
}

void CClient::addItem_InContainer( const CItem * pItem )
{
	ADDTOCALLSTACK("CClient::addItem_InContainer");
	ASSERT(pItem);
	CItemContainer * pCont = dynamic_cast <CItemContainer*> (pItem->GetParent());
	if ( pCont == NULL )
		return;

	new PacketItemContainer(this, pItem);
	
	if ( PacketDropAccepted::CanSendTo(GetNetState()) )
		new PacketDropAccepted(this);

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

void CClient::addContents( const CItemContainer * pContainer, bool fCorpseEquip, bool fCorpseFilter, bool fShop) // Send Backpack (with items)
{
	ADDTOCALLSTACK("CClient::addContents");
	// NOTE: We needed to send the header for this FIRST !!!
	// 1 = equip a corpse
	// 0 = contents.

	if (fCorpseEquip == true)
	{
		new PacketCorpseEquipment(this, pContainer);
	}
	else
	{
		new PacketItemContents(this, pContainer, fShop, fCorpseFilter);
	}

	return;
}



void CClient::addOpenGump( const CObjBase * pContainer, GUMP_TYPE gump )
{
	ADDTOCALLSTACK("CClient::addOpenGump");
	// NOTE: if pContainer has not already been sent to the client
	//  this will crash client.
	new PacketContainerOpen(this, pContainer, gump);
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

	OpenPacketTransaction transaction(this, PacketSend::PRI_NORMAL);

	addOpenGump(pContainer, gump);
	addContents(pContainer, false, false, false);

	LogOpenedContainer(pContainer);
	return true;
}

void CClient::LogOpenedContainer(const CItemContainer* pContainer) // record a container in opened container list
{
	ADDTOCALLSTACK("CClient::LogOpenedContainer");
	if (pContainer == NULL)
		return;

	CObjBaseTemplate * pTopMostContainer = pContainer->GetTopLevelObj();
	CObjBase * pTopContainer = pContainer->GetContainer();

	DWORD dwTopMostContainerUID = pTopMostContainer->GetUID().GetPrivateUID();
	DWORD dwTopContainerUID = 0;
	
	if ( pTopContainer != NULL )
		dwTopContainerUID = pTopContainer->GetUID().GetPrivateUID();
	else
		dwTopContainerUID = dwTopMostContainerUID;

	m_openedContainers[pContainer->GetUID().GetPrivateUID()] = std::make_pair(
																std::make_pair( dwTopContainerUID, dwTopMostContainerUID ),
																pTopMostContainer->GetTopPoint()
															    );
}

void CClient::addSeason(SEASON_TYPE season)
{
	ADDTOCALLSTACK("CClient::addSeason");
	if ( m_pChar->IsStatFlag(STATF_DEAD) )		// everything looks like this when dead.
		season = SEASON_Desolate;
	if ( season == m_Env.m_Season )	// the season i saw last.
		return;

	m_Env.m_Season = season;

	new PacketSeason(this, season, true);

	// client resets light level on season change, so resend light here too
	m_Env.m_Light = UCHAR_MAX;
	addLight();
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

	m_Env.m_Weather = weather;
	new PacketWeather(this, weather, 0x40, 0x10);
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

	new PacketGlobalLight(this, iLight);
}

void CClient::addArrowQuest( int x, int y, int id )
{
	ADDTOCALLSTACK("CClient::addArrowQuest");

	new PacketArrowQuest(this, x, y, id);
}

void CClient::addMusic( WORD id )
{
	ADDTOCALLSTACK("CClient::addMusic");
	// Music is ussually appropriate for the region.
	new PacketPlayMusic(this, id);
}

bool CClient::addKick( CTextConsole * pSrc, bool fBlock )
{
	ADDTOCALLSTACK("CClient::addKick");
	// Kick me out.
	ASSERT( pSrc );
	if ( GetAccount() == NULL )
	{
		GetNetState()->markReadClosed();
		return( true );
	}

	if ( ! GetAccount()->Kick( pSrc, fBlock ))
		return( false );

	LPCTSTR pszAction = fBlock ? "KICK" : "DISCONNECT";
	SysMessagef( "You have been %sed by '%s'", (LPCTSTR) pszAction, (LPCTSTR) pSrc->GetName());

	if ( IsConnectTypePacket() )
	{
		new PacketKick(this);
	}

	GetNetState()->markReadClosed();
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

	new PacketPlaySound(this, id, iOnce, 0, pt);
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

	new PacketMessageUNICODE(this, pwText, pSrc, wHue, mode, font, lang);
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

	new PacketMessageLocalised(this, iClilocId, pSrc, wHue, mode, font, pArgs);
}

void CClient::addBarkLocalizedEx( int iClilocId, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, AFFIX_TYPE affix, TCHAR * pAffix, TCHAR * pArgs )
{
	ADDTOCALLSTACK("CClient::addBarkLocalizedEx");
	if ( iClilocId <= 0 )
		return;

	if ( !IsConnectTypePacket() )
		return;

	if ( mode == TALKMODE_BROADCAST )
	{
		mode = TALKMODE_SYSTEM;
		pSrc = NULL;
	}

	new PacketMessageLocalisedEx(this, iClilocId, pSrc, wHue, mode, font, affix, pAffix, pArgs);
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
		case 3:	// Extended localized message (with affixed ASCII text)
		{
            TCHAR * ppArgs[256];
			int iQty = Str_ParseCmds( (TCHAR *)m_BarkBuffer.GetPtr(), ppArgs, COUNTOF(ppArgs), "," );
			int iClilocId = Exp_GetVal( ppArgs[0] );
			int iAffixType = Exp_GetVal( ppArgs[1] );
			CGString CArgs;
			for ( int i = 3; i < iQty; i++ )
			{
				if ( CArgs.GetLength() )
					CArgs += "\t";
				CArgs += ( !strcmp(ppArgs[i], "NULL") ? " " : ppArgs[i] );
			}

			addBarkLocalizedEx( iClilocId, pSrc, (HUE_TYPE) Args[0], mode, (FONT_TYPE) Args[1], (AFFIX_TYPE)iAffixType, ppArgs[2], (TCHAR *)CArgs.GetPtr());
			break;
		}

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
			CvtSystemToNUNICODE( szBuffer, COUNTOF(szBuffer), m_BarkBuffer.GetPtr(), -1 );
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

	new PacketMessageASCII(this, pszText, pSrc, wHue, mode, font);
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
	ASSERT(pDst);

	if (pSrc == NULL && motion == EFFECT_BOLT) // source required for bolt effect
		return;

	PacketSend* cmd(NULL);
	if (color || render)
		cmd = new PacketEffect(this, motion, id, pDst, pSrc, bSpeedSeconds, bLoop, fExplode, color, render);
	else
		cmd = new PacketEffect(this, motion, id, pDst, pSrc, bSpeedSeconds, bLoop, fExplode);
}


void CClient::GetAdjustedItemID( const CChar * pChar, const CItem * pItem, ITEMID_TYPE & id, HUE_TYPE & wHue ) const
{
	ADDTOCALLSTACK("CClient::GetAdjustedItemID");
	// An equipped item.
	ASSERT( pChar );

	id = pItem->GetDispID();
	wHue = pItem->GetHue();
	CItemBase * pItemDef = pItem->Item_GetDef();

	if ( pItem->IsType(IT_EQ_HORSE) )
	{
		// check the reslevel of the ridden horse
		CREID_TYPE idHorse = pItem->m_itFigurine.m_ID;
		CCharBase * pCharDef = CCharBase::FindCharBase(idHorse);
		if ( pCharDef && ( GetResDisp() < pCharDef->GetResLevel() ) )
		{
			idHorse = (CREID_TYPE) pCharDef->GetResDispDnId();
			wHue = pCharDef->GetResDispDnHue();

			// adjust the item to display the mount item associated with
			// the resdispdnid of the mount's chardef
			if ( idHorse != pItem->m_itFigurine.m_ID )
			{
				TCHAR * sMountDefname = Str_GetTemp();
				sprintf(sMountDefname, "mount_0x%x", idHorse);
				ITEMID_TYPE idMountItem = (ITEMID_TYPE)g_Exp.m_VarDefs.GetKeyNum(sMountDefname);
				if ( idMountItem > ITEMID_NOTHING )
				{
					id = idMountItem;
					pItemDef = CItemBase::FindItemBase(id);
				}
			}
		}
	}

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

void CClient::GetAdjustedCharID( const CChar * pChar, CREID_TYPE & id, HUE_TYPE & wHue ) const
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

	addCharMove(pChar, pChar->GetDirFlag());
}

void CClient::addCharMove( const CChar * pChar, BYTE bCharDir )
{
	ADDTOCALLSTACK("CClient::addCharMove");
	// This char has just moved on screen.
	// or changed in a subtle way like "hidden"
	// NOTE: If i have been turned this will NOT update myself.

	new PacketCharacterMove(this, pChar, bCharDir);
}

void CClient::addChar( const CChar * pChar )
{
	ADDTOCALLSTACK("CClient::addChar");
	// Full update about a char.
	EXC_TRY("addChar");
	new PacketCharacter(this, pChar);

	EXC_SET("Wake sector");
	pChar->GetTopPoint().GetSector()->SetSectorWakeStatus();	// if it can be seen then wake it.

	EXC_SET("Health bar colour");
	addHealthBarUpdate( pChar );

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
	size_t len = strcpylen( szName, pszNameFull, COUNTOF(szName) );

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
			len += sprintf( szName+len, " (%d gp)", pVendItem->GetBasePrice());
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

			default:
				break;
		}
	}
	if ( IsPriv(PRIV_DEBUG) )
		len += sprintf(szName+len, " [0%lx]", (DWORD) pItem->GetUID());

	if ( IsSetEF(EF_New_Triggers) )
	{
		CScriptTriggerArgs Args( this );
		Args.m_VarsLocal.SetStrNew("ClickMsgText", &szName[0]);
		Args.m_VarsLocal.SetNumNew("ClickMsgHue", (int)wHue);

		TRIGRET_TYPE ret = dynamic_cast<CObjBase*>(const_cast<CItem*>(pItem))->OnTrigger( "@AfterClick", m_pChar, &Args );	// CTRIG_AfterClick, ITRIG_AfterClick

		if ( ret == TRIGRET_RET_TRUE )
			return;

		LPCTSTR pNewStr = Args.m_VarsLocal.GetKeyStr("ClickMsgText");

		if ( pNewStr != NULL )
			strcpylen(szName, pNewStr, COUNTOF(szName));

		wHue = (HUE_TYPE)Args.m_VarsLocal.GetKeyNum("ClickMsgHue", true);
	}

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
				strcat( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_TAME) );
			else
				strcat( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_NPC) );
		}
		if ( pChar->IsStatFlag( STATF_INVUL ) && ! pChar->IsStatFlag( STATF_Incognito ) && ! pChar->IsPriv( PRIV_PRIV_NOSHOW ))
			strcat( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_INVUL) );
		if ( pChar->IsStatFlag( STATF_Stone ))
			strcat( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_STONE) );
		else if ( pChar->IsStatFlag( STATF_Freeze ))
			strcat( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_FROZEN) );
		if ( pChar->IsStatFlag( STATF_Insubstantial | STATF_Invisible | STATF_Hidden ))
			strcat( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_HIDDEN) );
		if ( pChar->IsStatFlag( STATF_Sleeping ))
			strcat( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_SLEEPING) );
		if ( pChar->IsStatFlag( STATF_Hallucinating ))
			strcat( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_HALLU) );

		if ( fAllShow )
		{
			if ( pChar->IsStatFlag(STATF_Spawned) )
				strcat(pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_SPAWN));
			if ( IsPriv( PRIV_DEBUG ))
				sprintf(pszTemp+strlen(pszTemp), " [0%lx]", (DWORD) pChar->GetUID());
		}
	}
	if ( ! fAllShow && pChar->Skill_GetActive() == NPCACT_Napping )
	{
		strcat( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_AFK) );
	}
	if ( pChar->GetPrivLevel() <= PLEVEL_Guest )
	{
		strcat( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_GUEST) );
	}
	if ( pChar->IsPriv( PRIV_JAILED ))
	{
		strcat( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_JAILED) );
	}
	if ( pChar->IsDisconnected())
	{
		strcat( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_LOGOUT) );
	}
	if (( fAllShow || pChar == m_pChar ) && pChar->IsStatFlag( STATF_Criminal ))
	{
		strcat( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_CRIMINAL) );
	}
	if ( fAllShow || ( IsPriv(PRIV_GM) && ( g_Cfg.m_wDebugFlags & DEBUGF_NPC_EMOTE )))
	{
		strcat( pszTemp, " [" );
		strcat( pszTemp, pChar->Skill_GetName());
		strcat( pszTemp, "]" );
	}

	if ( IsSetEF(EF_New_Triggers) )
	{
		CScriptTriggerArgs Args( this );
		Args.m_VarsLocal.SetStrNew("ClickMsgText", pszTemp);
		Args.m_VarsLocal.SetNumNew("ClickMsgHue", (int)wHue);

		TRIGRET_TYPE ret = dynamic_cast<CObjBase*>(const_cast<CChar*>(pChar))->OnTrigger( "@AfterClick", m_pChar, &Args );	// CTRIG_AfterClick, ITRIG_AfterClick

		if ( ret == TRIGRET_RET_TRUE )
			return;

		LPCTSTR pNewStr = Args.m_VarsLocal.GetKeyStr("ClickMsgText");

		if ( pNewStr != NULL )
			strcpy(pszTemp, pNewStr);

		wHue = (HUE_TYPE)Args.m_VarsLocal.GetKeyNum("ClickMsgHue", true);
	}

	addObjMessage( pszTemp, pChar, wHue );
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

	new PacketPlayerStart(this);

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

	// the client doesn't appear to load expansion maps when logging in during spring, so tell
	// them it's summer to begin with
	addSeason(SEASON_Summer);
	addWeather(WEATHER_DEFAULT);
	addLight();

	// now send the correct season
	CSector* pSector = pt.GetSector();
	if (pSector != NULL)
		addSeason(pSector->GetSeason());

	addPlayerWarMode();
	addKRToolbar( pChar->m_pPlayer->getKrToolbarStatus() );
}

void CClient::addPlayerWarMode()
{
	ADDTOCALLSTACK("CClient::addPlayerWarMode");

	new PacketWarMode(this, m_pChar);
}

void CClient::addToolTip( const CObjBase * pObj, LPCTSTR pszText )
{
	ADDTOCALLSTACK("CClient::addToolTip");
	if ( pObj == NULL )
		return;
	if ( pObj->IsChar())
		return; // no tips on chars.

	new PacketTooltip(this, pObj, pszText);
}

bool CClient::addBookOpen( CItem * pBook )
{
	ADDTOCALLSTACK("CClient::addBookOpen");
	// word wrap is done when typed in the client. (it has a var size font)
	if (pBook == NULL)
		return false;

	int iPagesNow = 0;
	bool bNewPacket = PacketDisplayBookNew::CanSendTo(GetNetState());

	if (pBook->IsBookSystem() == false)
	{
		// User written book.
		CItemMessage* pMsgItem = dynamic_cast<CItemMessage*>(pBook);
		if (pMsgItem == NULL)
			return false;

		if (pMsgItem->IsBookWritable())
			iPagesNow = pMsgItem->GetPageCount(); // for some reason we must send them now
	}

	if (bNewPacket)
		new PacketDisplayBookNew(this, pBook);
	else
		new PacketDisplayBook(this, pBook);

	// We could just send all the pages now if we want.
	if (iPagesNow)
		addBookPage(pBook, 1, iPagesNow);

	return( true );
}

void CClient::addBookPage( const CItem * pBook, int iPage, int iCount )
{
	ADDTOCALLSTACK("CClient::addBookPage");
	// ARGS:
	//  iPage = 1 based page.
	if ( iPage <= 0 )
		return;
	if ( iCount < 1 )
		iCount = 1;

	new PacketBookPageContent(this, pBook, iPage, iCount );
}

int CClient::Setup_FillCharList(Packet* pPacket, const CChar * pCharFirst)
{
	ADDTOCALLSTACK("CClient::Setup_FillCharList");
	// list available chars for your account that are idle.
	CAccount * pAccount = GetAccount();
	ASSERT( pAccount );

	int count = 0;

	if ( pCharFirst && pAccount->IsMyAccountChar( pCharFirst ))
	{
		m_tmSetupCharList[0] = pCharFirst->GetUID();

		pPacket->writeStringFixedASCII(pCharFirst->GetName(), MAX_NAME_SIZE);
		pPacket->writeStringFixedASCII("", MAX_NAME_SIZE);

		count++;
	}

	
	int iMax = minimum(maximum(pAccount->m_Chars.GetCharCount(), pAccount->GetMaxChars()), MAX_CHARS_PER_ACCT);

	int iQty = pAccount->m_Chars.GetCharCount();
	if (iQty > iMax)
		iQty = iMax;

	for (int i = 0; i < iQty; i++)
	{
		CGrayUID uid(pAccount->m_Chars.GetChar(i));
		CChar* pChar = uid.CharFind();
		if ( pChar == NULL )
			continue;
		if ( pCharFirst == pChar )
			continue;
		if ( count >= iMax )
			break;

		m_tmSetupCharList[count] = uid;

		pPacket->writeStringFixedASCII(pChar->GetName(), MAX_NAME_SIZE);
		pPacket->writeStringFixedASCII("", MAX_NAME_SIZE);

		count++;
	}

	// always show max count for some stupid reason. (client bug)
	// pad out the rest of the chars.
	int iClientMin = 5;
	if (GetNetState()->isClientVersion(MINCLIVER_PADCHARLIST) || !GetNetState()->getCryptVersion())
		iClientMin = maximum(iQty, 5);

	for ( ; count < iClientMin; count++)
	{
		pPacket->writeStringFixedASCII("", MAX_NAME_SIZE);
		pPacket->writeStringFixedASCII("", MAX_NAME_SIZE);
	}

	return count;
}

void CClient::SetTargMode( CLIMODE_TYPE targmode, LPCTSTR pPrompt, int iTimeout )
{
	ADDTOCALLSTACK("CClient::SetTargMode");
	// ??? Get rid of menu stuff if previous targ mode.
	// Can i close a menu ?
	// Cancel a cursor input.

	bool bSuppressCancelMessage = false;

	switch ( GetTargMode() )
	{
		case CLIMODE_TARG_USE_ITEM:
		{
			CItem * pItemUse = m_Targ_UID.ItemFind();
			if ( pItemUse != NULL && !IsSetEF(EF_Minimize_Triggers) )
			{
				if ( pItemUse->OnTrigger( ITRIG_TARGON_CANCEL, m_pChar ) == TRIGRET_RET_TRUE )
					bSuppressCancelMessage = true;
			}
		} break;

		case CLIMODE_TARG_SKILL_MAGERY:
		{
			const CSpellDef* pSpellDef = g_Cfg.GetSpellDef(m_tmSkillMagery.m_Spell);
			if (m_pChar != NULL && pSpellDef != NULL && !IsSetEF(EF_Minimize_Triggers))
			{
				CScriptTriggerArgs Args(m_tmSkillMagery.m_Spell, 0, m_Targ_PrvUID.ObjFind());
				if ( m_pChar->OnTrigger( CTRIG_SpellTargetCancel, this, &Args ) == TRIGRET_RET_TRUE )
					bSuppressCancelMessage = true;
				else if ( m_pChar->Spell_OnTrigger( m_tmSkillMagery.m_Spell, SPTRIG_TARGETCANCEL, m_pChar, &Args ) == TRIGRET_RET_TRUE )
					bSuppressCancelMessage = true;
			}
		} break;

		case CLIMODE_TARG_SKILL:
		case CLIMODE_TARG_SKILL_HERD_DEST:
		case CLIMODE_TARG_SKILL_PROVOKE:
		case CLIMODE_TARG_SKILL_POISON:
		{
			SKILL_TYPE action = SKILL_NONE;
			switch (GetTargMode())
			{
				case CLIMODE_TARG_SKILL:
					action = m_tmSkillTarg.m_Skill;
					break;
				case CLIMODE_TARG_SKILL_HERD_DEST:
					action = SKILL_HERDING;
					break;
				case CLIMODE_TARG_SKILL_PROVOKE:
					action = SKILL_PROVOCATION;
					break;
				case CLIMODE_TARG_SKILL_POISON:
					action = SKILL_POISONING;
					break;
				default:
					break;
			}

			if (action != SKILL_NONE && m_pChar != NULL)
			{
				if ( m_pChar->Skill_OnTrigger(action, SKTRIG_TARGETCANCEL) == TRIGRET_RET_TRUE )
					bSuppressCancelMessage = true;
			}
		} break;

		default:
			break;
	}

	// determine timeout time
	if (iTimeout > 0)
		m_Targ_Timeout = CServTime::GetCurrentTime() + iTimeout;
	else
		m_Targ_Timeout.Init();

	if ( GetTargMode() == targmode )
		return;

	if ( GetTargMode() != CLIMODE_NORMAL && targmode != CLIMODE_NORMAL )
	{
		// Just clear the old target mode
		if (bSuppressCancelMessage == false)
			addSysMessage( g_Cfg.GetDefaultMsg(DEFMSG_TARGET_CANCEL_2) );
	}

	m_Targ_Mode = targmode;

	if ( targmode == CLIMODE_NORMAL && bSuppressCancelMessage == false )
		addSysMessage( g_Cfg.GetDefaultMsg(DEFMSG_TARGET_CANCEL_1) );
	else if ( pPrompt && *pPrompt ) // Check that the message is not blank.
		addSysMessage( pPrompt );
}

void CClient::addPromptConsole( CLIMODE_TYPE mode, LPCTSTR pPrompt, CGrayUID context1, CGrayUID context2, bool bUnicode )
{
	ADDTOCALLSTACK("CClient::addPromptConsole");

	m_Prompt_Uid = context1;
	m_Prompt_Mode = mode;

	if ( pPrompt && *pPrompt ) // Check that the message is not blank.
		addSysMessage( pPrompt );

	new PacketAddPrompt(this, context1, context2, bUnicode);
}

void CClient::addTarget( CLIMODE_TYPE targmode, LPCTSTR pPrompt, bool fAllowGround, bool fCheckCrime, int iTimeout ) // Send targetting cursor to client
{
	ADDTOCALLSTACK("CClient::addTarget");
	// Expect XCMD_Target back.
	// ??? will this be selective for us ? objects only or chars only ? not on the ground (statics) ?

	SetTargMode( targmode, pPrompt, iTimeout );

	new PacketAddTarget(this,
						fAllowGround? PacketAddTarget::Ground : PacketAddTarget::Object,
						targmode,
						fCheckCrime? PacketAddTarget::Harmful : PacketAddTarget::None);
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

bool CClient::addTargetChars( CLIMODE_TYPE mode, CREID_TYPE baseID, bool fNotoCheck, int iTimeout )
{
	ADDTOCALLSTACK("CClient::addTargetChars");
	CCharBase * pBase = CCharBase::FindCharBase( baseID );
	if ( pBase == NULL )
		return( false );

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "%s '%s'?", g_Cfg.GetDefaultMsg(DEFMSG_WHERE_TO_SUMMON), (LPCTSTR) pBase->GetTradeName());

	addTarget(mode, pszTemp, true, fNotoCheck, iTimeout);
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
				addSysMessage( g_Cfg.GetDefaultMsg(DEFMSG_GUILD_ALREADY_MEMBER));
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

		new PacketAddTarget(this, fGround? PacketAddTarget::Ground : PacketAddTarget::Object, targmode, PacketAddTarget::None, id);
		return true;
	}

	// preview not supported by this ver?
	addTarget(targmode, pszTemp, true);
	return true;
}

void CClient::addTargetCancel()
{
	ADDTOCALLSTACK("CClient::addTargetCancel");

	// handle the cancellation now, in an ideal world the client would normally respond to
	// the cancel target packet as if the user had pressed ESC, but we shouldn't rely on
	// this happening (older clients for example don't support the cancel command and will
	// bring up a new target cursor)
	SetTargMode();

	// tell the client to cancel their cursor
	new PacketAddTarget(this, PacketAddTarget::Object, 0, PacketAddTarget::Cancel);
}

void CClient::addDyeOption( const CObjBase * pObj )
{
	ADDTOCALLSTACK("CClient::addDyeOption");
	// Put up the wHue chart for the client.
	// This results in a Event_Item_Dye message. CLIMODE_DYE

	new PacketShowDyeWindow(this, pObj);

	SetTargMode( CLIMODE_DYE );
}

void CClient::addSkillWindow(SKILL_TYPE skill, bool bFromInfo) // Opens the skills list
{
	ADDTOCALLSTACK("CClient::addSkillWindow");
	// Whos skills do we want to look at ?
	CChar* pChar = m_Prop_UID.CharFind();
	if (pChar == NULL)
		pChar = m_pChar;

	bool bAllSkills = (skill >= MAX_SKILL);
	if (bAllSkills == false && g_Cfg.m_SkillIndexDefs.IsValidIndex(skill) == false)
		return;

	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		CScriptTriggerArgs Args(bAllSkills? -1 : skill, bFromInfo);
		if (m_pChar->OnTrigger(CTRIG_UserSkills, pChar, &Args) == TRIGRET_RET_TRUE)
			return;
	}

	if (bAllSkills == false && skill >= SKILL_SCRIPTED)
		return;

	new PacketSkills(this, pChar, skill);
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
	AreaItems.SetSearchSquare(true);
	DWORD	dSeeItems = 0;

	for (;;)
	{
		CItem *pItem = AreaItems.GetItem();
		if ( !pItem )
			break;
		if ( !CanSee(pItem) )
			continue;
		if (fOsiSight)
		{
			if (( !pItem->GetTopLevelObj()->GetTopPoint().GetRegion(REGION_TYPE_MULTI) || ( pItem->m_TagDefs.GetKeyNum("ALWAYSSEND", true) ) || ( pItem->IsTypeMulti() ) || (( pItem->m_uidLink.IsValidUID() ) && ( pItem->m_uidLink.IsItem() ) && ( pItem->m_uidLink.ItemFind()->IsTypeMulti() ))
				|| ( pItem->GetTopLevelObj()->GetTopPoint().GetRegion(REGION_TYPE_MULTI) == pCurrentCharRegion )))
			{
				if ( dSeeItems < (g_Cfg.m_iMaxItemComplexity*30) )
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
			if ( dSeeItems < (g_Cfg.m_iMaxItemComplexity*30) )
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
	AreaChars.SetSearchSquare(true);
	DWORD	dSeeChars(0);
	for (;;)
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
	AreaItems.SetSearchSquare(true);
	DWORD	dSeeItems = 0;

	for (;;)
	{
		CItem *pItem = AreaItems.GetItem();
		if ( !pItem )
			break;
		if ( !CanSee(pItem) )
			continue;

		if (fOsiSight)
		{
			if (( !pItem->GetTopLevelObj()->GetTopPoint().GetRegion(REGION_TYPE_MULTI) ) || ( pItem->m_TagDefs.GetKeyNum("ALWAYSSEND", true) ) || ( pItem->IsTypeMulti() ) || (( pItem->m_uidLink.IsValidUID() ) && ( pItem->m_uidLink.IsItem() ) && ( pItem->m_uidLink.ItemFind()->IsTypeMulti() ))
				|| ((( ptold.GetRegion(REGION_TYPE_MULTI) != pCurrentCharRegion ) || ( ptold.GetDistSight(pItem->GetTopPoint()) > UO_MAP_VIEW_SIZE )) && ( !pItem->IsTypeMulti() ) && ( pItem->GetTopLevelObj()->GetTopPoint().GetRegion(REGION_TYPE_MULTI) == pCurrentCharRegion )))
			{
				if ((( m_pChar->GetTopPoint().GetDistSight(pItem->GetTopPoint()) <= UO_MAP_VIEW_SIZE ) && ( ptold.GetDistSight(pItem->GetTopPoint()) > UO_MAP_VIEW_SIZE )) || (( ptold.GetDistSight(pItem->GetTopPoint()) > tViewDist ) && ( pItem->IsTypeMulti() )) || ((( ptold.GetRegion(REGION_TYPE_MULTI) != pCurrentCharRegion ) || ( ptold.GetDistSight(pItem->GetTopPoint()) > tViewDist )) && ( !pItem->IsTypeMulti() ) && ( pItem->GetTopLevelObj()->GetTopPoint().GetRegion(REGION_TYPE_MULTI) == pCurrentCharRegion )))
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
			if ((( m_pChar->GetTopPoint().GetDistSight(pItem->GetTopPoint()) <= tViewDist ) && ( ptold.GetDistSight(pItem->GetTopPoint()) > tViewDist )) || (( ptold.GetDistSight(pItem->GetTopPoint()) > tViewDist ) && ( pItem->IsTypeMulti() )))
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
	AreaChars.SetSearchSquare(true);
	DWORD	dSeeChars(0);
	for (;;)
	{
		CChar	*pChar = AreaChars.GetChar();
		if ( !pChar )
			break;
		if (( m_pChar == pChar ) || !CanSee(pChar) )
			continue;

		if ( ptold.GetDistSight( pChar->GetTopPoint()) > tViewDist )
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

	new PacketPlayerPosition(this);

	// resync this stuff.
	m_net->m_sequence = 0;

	if ( ptold == m_pChar->GetTopPoint() )
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
	addChar( m_pChar );
	addPlayerView(ptold);
	addLight();		// Current light level where I am.
	addWeather();	// if any ...
	addSpeedMode( m_pChar->m_pPlayer->m_speedMode );
	addCharStatWindow( m_pChar->GetUID() );
}

void CClient::addMap( const CPointMap * pOldP, bool playerStart)
{
	ADDTOCALLSTACK("CClient::addMap");
	CPointMap pt = m_pChar->GetTopPoint();

	if ( !playerStart && pOldP && pOldP->m_map == pt.m_map )
		return;

	new PacketMapChange(this, g_MapList.m_mapid[pt.m_map]);

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

	new PacketEnableMapDiffs(this);
}

void CClient::addChangeServer()
{
	ADDTOCALLSTACK("CClient::addChangeServer");
	CPointMap pt = m_pChar->GetTopPoint();

	new PacketZoneChange(this, pt);
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

	new PacketCharacterStatus(this, pChar);
	if ( pChar == m_pChar )
		m_fUpdateStats = 0;

	if ( ( pChar != m_pChar ) && ( pChar->m_pParty != NULL ) && ( pChar->m_pParty->IsInParty( m_pChar ) ) )
	{
		// Send mana and stamina info to party members
		addManaUpdate( pChar->GetUID() );
		addStamUpdate( pChar->GetUID() );
	}

	if ( (pChar == m_pChar) && (pChar->m_pPlayer != NULL) && (PacketStatLocks::CanSendTo(GetNetState())) )
	{
		new PacketStatLocks(this, pChar);
	}

}

void CClient::addHitsUpdate( CGrayUID uid )
{
	ADDTOCALLSTACK("CClient::addHitsUpdate");
	CChar * pChar = uid.CharFind();
	if ( pChar == NULL )
		return;

	PacketHealthUpdate cmd(pChar, pChar == m_pChar);
	cmd.send(this);

	if (( pChar->m_pParty == NULL ) || ( m_pChar != pChar ))
		return;

	pChar->m_pParty->AddStatsUpdate( pChar, &cmd );
}

void CClient::addManaUpdate( CGrayUID uid )
{
	ADDTOCALLSTACK("CClient::addManaUpdate");
	CChar * pChar = uid.CharFind();
	if ( pChar == NULL )
		return;

	PacketManaUpdate cmd(pChar);
	cmd.send(this);

	if (( pChar->m_pParty == NULL ) || ( m_pChar != pChar ))
		return;

	pChar->m_pParty->AddStatsUpdate( pChar, &cmd );
}

void CClient::addStamUpdate( CGrayUID uid )
{
	ADDTOCALLSTACK("CClient::addStamUpdate");
	CChar * pChar = uid.CharFind();
	if ( pChar == NULL )
		return;

	PacketStaminaUpdate cmd(pChar);
	cmd.send(this);

	if (( pChar->m_pParty == NULL ) || ( m_pChar != pChar ))
		return;

	pChar->m_pParty->AddStatsUpdate( pChar, &cmd );
}

void CClient::addHealthBarUpdate( const CChar * pChar )
{
	ADDTOCALLSTACK("CClient::addHealthBarUpdate");
	if ( pChar == NULL )
		return;

	if ( PacketHealthBarUpdate::CanSendTo(GetNetState()) )
		new PacketHealthBarUpdate(this, pChar);
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

	OpenPacketTransaction transaction(this, PacketSend::PRI_NORMAL);
	addOpenGump( pBook, GUMP_OPEN_SPELLBOOK );

	//
	// New AOS spellbook packet required by client 4.0.0 and above.
	// Old packet is still required if both FEATURE_AOS_TOOLTIP and FEATURE_AOS_UPDATE aren't sent.
	//
	if ( PacketSpellbookContent::CanSendTo(GetNetState()) && IsResClient( RDS_AOS ) && IsAosFlagEnabled(FEATURE_AOS_UPDATE_B) ) // IsResClient( RDS_AOS ) && g_Cfg.m_iFeatureAOS
	{
		// Handle new AOS spellbook stuff (old packets no longer work)
		new PacketSpellbookContent(this, pBook, offset);
		return;
	}

	if (count <= 0)
		return;

	new PacketItemContents(this, pBook);
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

	OpenPacketTransaction transaction(this, PacketSend::PRI_NORMAL);
	addOpenGump( pBook, (GUMP_TYPE) gumpID );
	if (count <= 0)
		return;

	new PacketItemContents(this, pContainer);
}

void CClient::addScrollScript( CResourceLock &s, SCROLL_TYPE type, DWORD context, LPCTSTR pszHeader )
{
	ADDTOCALLSTACK("CClient::addScrollScript");

	new PacketOpenScroll(this, s, type, context, pszHeader);
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

	new PacketCloseVendor(this, pVendor);
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

	int iConvertFactor = pVendor->NPC_GetVendorMarkup(m_pChar );
	PacketVendorBuyList* cmd = new PacketVendorBuyList();
	int count = cmd->fillContainer(pContainer, iConvertFactor, bReal? 115 : 0);
	cmd->push(this);

	// Send a warning if the vendor somehow has more stock than the allowed limit
	if ( pContainer->GetCount() > minimum(MAX_ITEMS_VENDOR,MAX_ITEMS_CONT) )
		g_Log.Event( LOGL_WARN, "Vendor 0%lx '%s' has exceeded their stock limit! (%d/%d items)\n", (DWORD) pVendor->GetUID(), (LPCTSTR) pVendor->GetName(), pContainer->GetCount(), minimum(MAX_ITEMS_VENDOR,MAX_ITEMS_CONT));

	return count;
}

bool CClient::addShopMenuBuy( CChar * pVendor )
{
	ADDTOCALLSTACK("CClient::addShopMenuBuy");
	// Try to buy stuff that the vendor has.
	if ( !pVendor || !pVendor->NPC_IsVendor() )
		return false;

	OpenPacketTransaction transaction(this, PacketSend::PRI_LOW);

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

bool CClient::addShopMenuSell( CChar * pVendor )
{
	ADDTOCALLSTACK("CClient::addShopMenuSell");
	// Player selling to vendor.
	// What things do you have in your inventory that the vendor would want ?
	// Should end with a returned Event_VendorSell()

	if ( !pVendor || !pVendor->NPC_IsVendor() )
		return false;

	OpenPacketTransaction transaction(this, PacketSend::PRI_LOW);

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

	if ( GetNetState()->isClientLessVersion(MAXCLIVER_EXTRASHOPLAYER) )
	{
		// This avoid client crashes
		CItemContainer * pContainer3 = pVendor->GetBank( LAYER_VENDOR_EXTRA );
		addItem( pContainer3 );
	}

	if ( pVendor->IsStatFlag( STATF_Pet ))	// Player vendor.
	{
		pContainer2 = NULL; // no stock
	}

	PacketVendorSellList cmd(pVendor);
	size_t count = cmd.searchContainer(this, m_pChar->GetPackSafe(), pContainer1, pContainer2, iConvertFactor);
	if (count <= 0)
		return false;
	
	cmd.send(this);
	return true;
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
		g_MapList.m_mapid[pMap->m_itMap.m_map]);

	if ( ! rect.IsValid())
		goto blank_map;
	if ( rect.IsRectEmpty())
		goto blank_map;

	if ( PacketDisplayMapNew::CanSendTo(GetNetState()))
		new PacketDisplayMapNew(this, pMap, rect);
	else
		new PacketDisplayMap(this, pMap, rect);

	addMapMode( pMap, MAP_UNSENT, false );

	// Now show all the pins
	PacketMapPlot plot(pMap, MAP_ADD, false);
	for ( int i=0; i < pMap->m_Pins.GetCount(); i++ )
	{
		plot.setPin(pMap->m_Pins[i].m_x, pMap->m_Pins[i].m_y);
		plot.send(this);
	}
}

void CClient::addMapMode( CItemMap * pMap, MAPCMD_TYPE iType, bool fEdit )
{
	ADDTOCALLSTACK("CClient::addMapMode");
	// NOTE: MAPMODE_* depends on who is looking. Multi clients could interfere with each other ?
	if ( !pMap )
		return;

	pMap->m_fPlotMode = fEdit;

	new PacketMapPlot(this, pMap, iType, fEdit);
}

void CClient::addBulletinBoard( const CItemContainer * pBoard )
{
	ADDTOCALLSTACK("CClient::addBulletinBoard");
	// Open up the bulletin board and all it's messages
	// PacketBulletinBoardReq::onReceive
	if (pBoard == NULL)
		return;

	// Give the bboard name.
	new PacketBulletinBoard(this, pBoard);

	// Send Content messages for all the items on the bboard.
	// Not sure what x,y are here, date/time maybe ?
	addContents( pBoard, false, false, false );

	// The client will now ask for the headers it wants.
}

bool CClient::addBBoardMessage( const CItemContainer * pBoard, BBOARDF_TYPE flag, CGrayUID uidMsg )
{
	ADDTOCALLSTACK("CClient::addBBoardMessage");
	ASSERT(pBoard);

	CItemMessage* pMsgItem = dynamic_cast<CItemMessage *>(uidMsg.ItemFind());
	if (pBoard->IsItemInside( pMsgItem ) == false)
		return( false );

	// check author is properly linked
	if (pMsgItem->m_sAuthor.IsEmpty() == false && pMsgItem->m_uidLink.CharFind() == NULL)
	{
		pMsgItem->Delete();
		return( false );
	}

	// Send back the message header and/or body.
	new PacketBulletinBoard(this, flag, pBoard, pMsgItem);
	return( true );
}

void CClient::addRedrawAll()
{
	ADDTOCALLSTACK("CClient::addRedrawAll");
	new PacketRedrawAll(this);
}

void CClient::addChatSystemMessage( CHATMSG_TYPE iType, LPCTSTR pszName1, LPCTSTR pszName2, CLanguageID lang )
{
	ADDTOCALLSTACK("CClient::addChatSystemMessage");

	new PacketChatMessage(this, iType, pszName1, pszName2, lang);
}

void CClient::addGumpTextDisp( const CObjBase * pObj, GUMP_TYPE gump, LPCTSTR pszName, LPCTSTR pszText )
{
	ADDTOCALLSTACK("CClient::addGumpTextDisp");
	// ??? how do we control where exactly the text goes ??

	new PacketSignGump(this, pObj, gump, pszName, pszText);
}

void CClient::addItemMenu( CLIMODE_TYPE mode, const CMenuItem * item, int count, CObjBase * pObj )
{
	ADDTOCALLSTACK("CClient::addItemMenu");
	// We must set GetTargMode() to show what mode we are in for menu select.
	// Will result in PacketMenuChoice::onReceive()
	// cmd.ItemMenu.

	if (count <= 0)
		return;

	if (pObj == NULL)
		pObj = m_pChar;

	new PacketDisplayMenu(this, mode, item, count, pObj);

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

	if ( GetNetState()->isClientLessVersion(MINCLIVER_CHECKWALKCODE) )
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
	int i;
	for (i = 0; i < iCodes && m_Walk_CodeQty < COUNTOF(m_Walk_LIFO); m_Walk_CodeQty++, i++)
		m_Walk_LIFO[m_Walk_CodeQty] = 0x88ca0000 + Calc_GetRandVal(0xffff);

	new PacketFastWalk(this, m_Walk_LIFO, m_Walk_CodeQty, i);
	return( true );
}

void CClient::addCharPaperdoll( CChar * pChar )
{
	ADDTOCALLSTACK("CClient::addCharPaperdoll");
	if ( !pChar )
		return;

	new PacketPaperdoll(this, pChar);
}

void CClient::addAOSTooltip( const CObjBase * pObj, bool bRequested, bool bShop )
{
	ADDTOCALLSTACK("CClient::addAOSTooltip");
	if ( !pObj )
		return;

	if ( PacketPropertyList::CanSendTo(GetNetState()) == false )
		return;

	bool bNameOnly = false;
	if (!IsResClient(RDS_AOS) || !IsAosFlagEnabled(FEATURE_AOS_UPDATE_B))
	{
		if ( !bShop )
			return;

		// shop items use tooltips whether they're disabled or not,
		// so we can just send a basic tooltip with the item name
		bNameOnly = true;
	}

	//DEBUG_MSG(("(( m_pChar->GetTopPoint().GetDistSight(pObj->GetTopPoint()) (%x) > UO_MAP_VIEW_SIZE (%x) ) && ( !bShop ) (%x) )", m_pChar->GetTopPoint().GetDistSight(pObj->GetTopPoint()), UO_MAP_VIEW_SIZE, ( !bShop )));
	if (( m_pChar->GetTopPoint().GetDistSight(pObj->GetTopPoint()) > UO_MAP_VIEW_SIZE ) && ( m_pChar->GetTopPoint().GetDistSight(pObj->GetTopPoint()) <= UO_MAP_VIEW_RADAR ) && ( !bShop ) ) //we do not need to send tooltips for items not in LOS (multis/ships)
		return;

	// We check here if we are sending a tooltip for a static/non-movable items
	// (client doesn't expect us to) but only in the world
	if ( pObj->IsItem() )
	{
		const CItem * pItem = dynamic_cast<const CItem *>( pObj );

		if ( !pItem->GetContainer() && pItem->IsAttr(/*ATTR_MOVE_NEVER|*/ATTR_STATIC) )
		{
			if ( ( ! this->GetChar()->IsPriv( PRIV_GM ) ) && ( ! this->GetChar()->IsPriv( PRIV_ALLMOVE ) ) )
				return;
		}
	}

	PacketPropertyList* propertyList = pObj->GetPropertyList();

	if (propertyList == NULL || propertyList->hasExpired(g_Cfg.m_iTooltipCache))
	{
		CItem	*pItem = ( pObj->IsItem() ? (CItem *) dynamic_cast <const CItem *> (pObj) : NULL );
		CChar	*pChar = ( pObj->IsChar() ? (CChar *) dynamic_cast <const CChar *> (pObj) : NULL );

		if (pItem != NULL)
			pItem->FreePropertyList();
		else if (pChar != NULL)
			pChar->FreePropertyList();

		CClientTooltip* t = NULL;
		this->m_TooltipData.Clean(true);

		//DEBUG_MSG(("Preparing tooltip for 0%lx (%s)\n", (DWORD)pObj->GetUID(), pObj->GetName()));

		if (bNameOnly) // if we only want to display the name (FEATURE_AOS_UPDATE_B disabled)
		{
			this->m_TooltipData.InsertAt(0, t = new CClientTooltip(1050045));
			t->FormatArgs(" \t%s\t ", pObj->GetName());
		}
		else // we have FEATURE_AOS_UPDATE_B enabled
		{
			TRIGRET_TYPE iRet = TRIGRET_RET_FALSE;
			CScriptTriggerArgs args((CScriptObj *)pObj);
			args.m_iN1 = bRequested;
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
						this->m_TooltipData.Add( new CClientTooltip( 1070722, g_Cfg.GetDefaultMsg(DEFMSG_TOOLTIP_TAG_NEWBIE) ) ); // ~1_NOTHING~
					if ( pItem->IsAttr( ATTR_MAGIC ) )
						this->m_TooltipData.Add( new CClientTooltip( 3010064 ) ); // Magic

					if ( ( pItem->GetAmount() != 1 ) && ( pItem->GetType() != IT_CORPSE ) ) // Negative amount?
					{
						this->m_TooltipData.Add( t = new CClientTooltip( 1060663 ) ); // ~1_val~: ~2_val~
						t->FormatArgs( "%s\t%u", g_Cfg.GetDefaultMsg(DEFMSG_TOOLTIP_TAG_AMOUNT), pItem->GetAmount() );
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
								const CContainer * pContainer = dynamic_cast <const CContainer *> ( pItem );
								this->m_TooltipData.Add( t = new CClientTooltip( 1050044 ) );
								t->FormatArgs( "%d\t%d.%d", pContainer->GetCount(), pContainer->GetTotalWeight() / WEIGHT_UNITS, pContainer->GetTotalWeight() % WEIGHT_UNITS ); // ~1_COUNT~ items, ~2_WEIGHT~ stones
							}
							break;

						case IT_ARMOR_LEATHER:
						case IT_ARMOR:
						case IT_CLOTHING:
						case IT_SHIELD:
							this->m_TooltipData.Add( t = new CClientTooltip( 1060658 ) ); // ~1_val~: ~2_val~
							t->FormatArgs( "%s\t%d", g_Cfg.GetDefaultMsg(DEFMSG_TOOLTIP_TAG_ARMOR), pItem->Armor_GetDefense() );
							this->m_TooltipData.Add( t = new CClientTooltip( 1061170 ) ); // strength requirement ~1_val~
							t->FormatArgs( "%d", pItem->Item_GetDef()->m_ttEquippable.m_StrReq );
							this->m_TooltipData.Add( t = new CClientTooltip( 1060639 ) ); // durability ~1_val~ / ~2_val~
							t->FormatArgs( "%u\t%u", pItem->m_itArmor.m_Hits_Cur, pItem->m_itArmor.m_Hits_Max );
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
							t->FormatArgs( "%d\t%d", pItem->Item_GetDef()->m_attackBase + pItem->m_ModAr, ( pItem->Weapon_GetAttack(true) ) );
							this->m_TooltipData.Add( t = new CClientTooltip( 1061170 ) ); // strength requirement ~1_val~
							t->FormatArgs( "%d", pItem->Item_GetDef()->m_ttEquippable.m_StrReq );
							this->m_TooltipData.Add( t = new CClientTooltip( 1060639 ) ); // durability ~1_val~ / ~2_val~
							t->FormatArgs( "%u\t%u", pItem->m_itWeapon.m_Hits_Cur, pItem->m_itWeapon.m_Hits_Max );

							if ( pItem->m_itWeapon.m_poison_skill )
								this->m_TooltipData.Add( new CClientTooltip( 1017383 ) ); // Poisoned
							break;

						case IT_WEAPON_MACE_PICK:
							this->m_TooltipData.Add( t = new CClientTooltip( 1060639 ) ); // durability ~1_val~ / ~2_val~
							t->FormatArgs( "%u\t%u", pItem->m_itWeapon.m_Hits_Cur, pItem->m_itWeapon.m_Hits_Max );
							break;

						case IT_TELEPAD:
						case IT_MOONGATE:
							if ( this->IsPriv( PRIV_GM ) )
							{
								this->m_TooltipData.Add( t = new CClientTooltip( 1060658 ) ); // ~1_val~: ~2_val~
								t->FormatArgs( "%s\t%s", g_Cfg.GetDefaultMsg(DEFMSG_TOOLTIP_TAG_DESTINATION), pItem->m_itTelepad.m_pntMark.WriteUsed() );
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
						case IT_SPELLBOOK_MYSTIC:
						case IT_SPELLBOOK_BARD:
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
								LPCTSTR pszName = NULL;
								if ( pSpawnCharDef )
								{
									CCharBase *pCharBase = dynamic_cast<CCharBase*>( pSpawnCharDef );
									if ( pCharBase )
										pszName = pCharBase->GetTradeName();
									else
										pszName = pSpawnCharDef->GetName();

									while (*pszName == '#')
										pszName++;
								}

								this->m_TooltipData.Add( t = new CClientTooltip( 1060658 ) ); // ~1_val~: ~2_val~
								t->FormatArgs( "Character\t%s", pszName ? pszName : "none" );
								this->m_TooltipData.Add( t = new CClientTooltip( 1061169 ) ); // range ~1_val~
								t->FormatArgs( "%d", pItem->m_itSpawnChar.m_DistMax );
								this->m_TooltipData.Add( t = new CClientTooltip( 1074247 ) );
								t->FormatArgs( "%lu\t%u", pItem->m_itSpawnChar.m_current, pItem->GetAmount() );
								this->m_TooltipData.Add( t = new CClientTooltip( 1060659 ) ); // ~1_val~: ~2_val~
								t->FormatArgs( "Min/max time\t%u min / %u min", pItem->m_itSpawnChar.m_TimeLoMin, pItem->m_itSpawnChar.m_TimeHiMin );
								this->m_TooltipData.Add( t = new CClientTooltip( 1060660 ) ); // ~1_val~: ~2_val~
								t->FormatArgs( "Time until next spawn\t%d sec", pItem->GetTimerAdjusted() );
							} break;

						case IT_SPAWN_ITEM:
							{
								CResourceDef * pSpawnItemDef = g_Cfg.ResourceGetDef( pItem->m_itSpawnItem.m_ItemID );
								this->m_TooltipData.Add( t = new CClientTooltip( 1060658 ) ); // ~1_val~: ~2_val~
								t->FormatArgs( "Item\t%s", pSpawnItemDef ? pSpawnItemDef->GetName() : "none" );
								this->m_TooltipData.Add( t = new CClientTooltip( 1060656 ) ); // amount to make: ~1_val~
								t->FormatArgs( "%lu", pItem->m_itSpawnItem.m_pile );
								this->m_TooltipData.Add( t = new CClientTooltip( 1074247 ) );
								t->FormatArgs( "??\t%u", pItem->GetAmount() );
								this->m_TooltipData.Add( t = new CClientTooltip( 1060659 ) ); // ~1_val~: ~2_val~
								t->FormatArgs( "Min/max time\t%u min / %u min", pItem->m_itSpawnItem.m_TimeLoMin, pItem->m_itSpawnItem.m_TimeHiMin );
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
								const CItemStone * thisStone = dynamic_cast<const CItemStone *>(pItem);
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

						default:
							break;
					}
				}
			}
		}
	
#define DOHASH( value ) hash ^= ((value) & 0x3FFFFFF); \
						hash ^= ((value) >> 26) & 0x3F;

		// build a hash value from the tooltip entries
		DWORD hash = 0;
		DWORD argumentHash = 0;
		for (int i = 0; i < m_TooltipData.GetCount(); i++)
		{
			CClientTooltip* tipEntry = m_TooltipData.GetAt(i);
			argumentHash = HashString(tipEntry->m_args, strlen(tipEntry->m_args));

			DOHASH(tipEntry->m_clilocid);
			DOHASH(argumentHash);
		}
		hash |= UID_F_ITEM;

#undef DOHASH

		// clients actually expect to use an incremental revision number and not a
		// hash to check if a tooltip needs updating - the client will not request
		// updated tooltip data if the hash happens to be less than the previous one
		//
		// we still want to generate a hash though, so we don't have to increment
		// the revision number if the tooltip hasn't actually been changed
		DWORD revision = 0;
		if (pItem != NULL)
			revision = pItem->UpdatePropertyRevision(hash);
		else if (pChar != NULL)
			revision = pChar->UpdatePropertyRevision(hash);

		propertyList = new PacketPropertyList(pObj, revision, &m_TooltipData);

		// cache the property list for next time, unless property list is
		// incomplete (name only) or caching is disabled
		if (bNameOnly == false && g_Cfg.m_iTooltipCache > 0)
		{
			if (pItem != NULL)
				pItem->SetPropertyList(propertyList);
			else if (pChar != NULL)
				pChar->SetPropertyList(propertyList);
		}
	}

	if (propertyList->isEmpty() == false)
	{
		switch (g_Cfg.m_iTooltipMode)
		{
			case TOOLTIPMODE_SENDVERSION:
				if (bRequested == false && bShop == false)
				{
					// send property list version (client will send a request for the full tooltip if needed)
					if ( PacketPropertyListVersion::CanSendTo(GetNetState()) == false )
						new PacketPropertyListVersionOld(this, pObj, propertyList->getVersion());
					else
						new PacketPropertyListVersion(this, pObj, propertyList->getVersion());

					break;
				}

				// fall through to send full list

			case TOOLTIPMODE_SENDFULL:
			default:
				// send full property list
				new PacketPropertyList(this, propertyList);
				break;
		}
	}
	
	// delete the original packet, as long as it doesn't belong
	// to the object (i.e. wasn't cached)
	if (propertyList != pObj->GetPropertyList())
		delete propertyList;
}

void CClient::addShowDamage( int damage, DWORD uid_damage )
{
	ADDTOCALLSTACK("CClient::addShowDamage");
	if ( damage < 0 )
		damage = 0;

	if ( PacketCombatDamage::CanSendTo(GetNetState()) )
		new PacketCombatDamage(this, damage, CGrayUID(uid_damage));
	else if ( PacketCombatDamageOld::CanSendTo(GetNetState()) )
		new PacketCombatDamageOld(this, damage, CGrayUID(uid_damage));
}

void CClient::addSpeedMode( int speedMode )
{
	ADDTOCALLSTACK("CClient::addSpeedMode");

	new PacketSpeedMode(this, speedMode);
}

void CClient::addVisualRange( BYTE visualRange )
{
	ADDTOCALLSTACK("CClient::addVisualRange");

	//DEBUG_ERR(("addVisualRange called with argument %d\n", visualRange));

	new PacketVisualRange(this, visualRange);
}

void CClient::addIdleWarning( BYTE message )
{
	ADDTOCALLSTACK("CClient::addIdleWarning");

	new PacketWarningMessage(this, (PacketWarningMessage::Message)message);
}

void CClient::addKRToolbar( bool bEnable )
{
	ADDTOCALLSTACK("CClient::addKRToolbar");
	if ( PacketToggleHotbar::CanSendTo(GetNetState()) == false || !IsResClient(RDS_KR) || ( GetConnectType() != CONNECT_GAME ))
		return;

	new PacketToggleHotbar(this, bEnable);
}


// --------------------------------------------------------------------
void CClient::SendPacket( TCHAR * pszKey )
{
	ADDTOCALLSTACK("CClient::SendPacket");
	PacketSend* packet = new PacketSend(0, 0, PacketSend::PRI_NORMAL);
	packet->seek();

	while ( *pszKey )
	{
		if ( packet->getLength() > SCRIPT_MAX_LINE_LEN - 4 )
		{	// we won't get here because this lenght is enforced in all scripts
			DEBUG_ERR(("SENDPACKET too big.\n"));

			delete packet;
			return;
		}

		GETNONWHITESPACE( pszKey );

		if ( toupper(*pszKey) == 'D' )
		{
			++pszKey;
			DWORD iVal = Exp_GetVal(pszKey);

			packet->writeInt32(iVal);
		}
		else if ( toupper(*pszKey) == 'W' )
		{
			++pszKey;
			WORD iVal = Exp_GetVal(pszKey);

			packet->writeInt16(iVal);
		}
		else
		{
			if ( toupper(*pszKey) == 'B' )
				pszKey++;
			BYTE iVal = Exp_GetVal(pszKey);

			packet->writeByte(iVal);
		}
	}

	packet->trim();
	packet->push(this);
}

// ---------------------------------------------------------------------
// Login type stuff.

BYTE CClient::Setup_Start( CChar * pChar ) // Send character startup stuff to player
{
	ADDTOCALLSTACK("CClient::Setup_Start");
	// Play this char.
	ASSERT( GetAccount() );
	ASSERT( pChar );
	char	*z = Str_GetTemp();

	CharDisconnect();	// I'm already logged in as someone else ?

	g_Log.Event( LOGM_CLIENTS_LOG, "%lx:Setup_Start acct='%s', char='%s', IP='%s'\n", 
		GetSocketID(), (LPCTSTR) GetAccount()->GetName(), (LPCTSTR) pChar->GetName(), GetPeerStr() );

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
	m_mapOpenedGumps.clear();

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
		return PacketLoginError::Blocked;
	}

	pChar->SetKeyNum("LastHit", 0);

	fNoMessages	= (Args.m_iN1 != 0);
	fQuickLogIn	= (Args.m_iN2 != 0);

	if ( !fQuickLogIn )
	{
		if ( !fNoMessages )
		{
			addBark(g_szServerDescription, NULL, HUE_YELLOW, TALKMODE_SYSTEM, FONT_NORMAL);

			sprintf(z, (g_Serv.StatGet(SERV_STAT_CLIENTS)==2) ?
				g_Cfg.GetDefaultMsg( DEFMSG_LOGIN_PLAYER ) : g_Cfg.GetDefaultMsg( DEFMSG_LOGIN_PLAYERS ),
				g_Serv.StatGet(SERV_STAT_CLIENTS)-1 );
			addSysMessage(z);

			sprintf(z, g_Cfg.GetDefaultMsg( DEFMSG_LOGIN_LASTLOGGED ), GetAccount()->m_TagDefs.GetKeyStr("LastLogged"));
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
		m_pChar->Jail(&g_Serv, true, GetAccount()->m_TagDefs.GetKeyNum("JailCell", true));
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

	DEBUG_MSG(( "%lx:Setup_Start done\n", GetSocketID()));
	return PacketLoginError::Success;
}

BYTE CClient::Setup_Play( int iSlot ) // After hitting "Play Character" button
{
	ADDTOCALLSTACK("CClient::Setup_Play");
	// Mode == CLIMODE_SETUP_CHARLIST

	DEBUG_MSG(( "%lx:Setup_Play slot %d\n", GetSocketID(), iSlot ));

	if ( ! GetAccount())
		return( PacketLoginError::Invalid );
	if ( iSlot < 0 || iSlot >= COUNTOF(m_tmSetupCharList))
		return( PacketLoginError::BadCharacter );

	CChar * pChar = m_tmSetupCharList[ iSlot ].CharFind();
	if ( ! GetAccount()->IsMyAccountChar( pChar ))
		return( PacketLoginError::BadCharacter );

	CChar * pCharLast = GetAccount()->m_uidLastChar.CharFind();
	if ( pCharLast && GetAccount()->IsMyAccountChar( pCharLast ) && GetAccount()->GetPrivLevel() <= PLEVEL_GM &&
		! pCharLast->IsDisconnected() && (pChar->GetUID() != pCharLast->GetUID()))
	{
		addIdleWarning(PacketWarningMessage::CharacterInWorld);
		return(PacketLoginError::CharIdle);
	}

	return Setup_Start( pChar );
}

BYTE CClient::Setup_Delete( int iSlot ) // Deletion of character
{
	ADDTOCALLSTACK("CClient::Setup_Delete");
	ASSERT( GetAccount() );
	DEBUG_MSG(( "%lx:Setup_Delete slot=%d\n", GetSocketID(), iSlot ));
	if ( iSlot < 0 || iSlot >= COUNTOF(m_tmSetupCharList))
		return PacketDeleteError::NotExist;

	CChar * pChar = m_tmSetupCharList[iSlot].CharFind();
	if ( ! GetAccount()->IsMyAccountChar( pChar ))
		return PacketDeleteError::BadPass;

	if ( ! pChar->IsDisconnected())
	{
		return PacketDeleteError::InUse;
	}

	// Make sure the char is at least x days old.
	if ( g_Cfg.m_iMinCharDeleteTime &&
		(- g_World.GetTimeDiff( pChar->m_timeCreate )) < g_Cfg.m_iMinCharDeleteTime )
	{
		if ( GetPrivLevel() < PLEVEL_Seer )
		{
			return PacketDeleteError::NotOldEnough;
		}
	}

	//	Do the scripts allow to delete the char?
	enum TRIGRET_TYPE	tr;
	CScriptTriggerArgs Args;
	Args.m_pO1 = this;
	pChar->r_Call("f_onchar_delete", pChar, &Args, NULL, &tr);
	if ( tr == TRIGRET_RET_TRUE )
	{
		return PacketDeleteError::NotOldEnough;
	}

	// pChar->Delete();
	delete pChar;
	// refill the list.

	new PacketCharacterListUpdate(this, GetAccount()->m_uidLastChar.CharFind());

	return PacketDeleteError::Success;
}

BYTE CClient::Setup_ListReq( const char * pszAccName, const char * pszPassword, bool fTest )
{
	ADDTOCALLSTACK("CClient::Setup_ListReq");
	// XCMD_CharListReq
	// Gameserver login and request character listing

	if ( GetConnectType() != CONNECT_GAME )	// Not a game connection ?
	{
		return(PacketLoginError::Other);
	}

	switch ( GetTargMode())
	{
		case CLIMODE_SETUP_RELAY:
			ClearTargMode();
			break;

		default:
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
		}
		return( lErr );
	}

	CAccountRef pAcc = GetAccount();
	ASSERT( pAcc );

	CChar * pCharLast = pAcc->m_uidLastChar.CharFind();

/*	if ( pCharLast &&
		GetAccount()->IsMyAccountChar( pCharLast ) &&
		GetAccount()->GetPrivLevel() <= PLEVEL_GM &&
		! pCharLast->IsDisconnected())
	{
		// If the last char is lingering then log back into this char instantly.
		// m_iClientLingerTime
		if ( Setup_Start(pCharLast) )
			return PacketLoginError::Success;
		return PacketLoginError::Blocked; //Setup_Start() returns false only when login blocked by Return 1 in @Login
	} */

	new PacketEnableFeatures(this, g_Cfg.GetPacketFlag(false, (RESDISPLAY_VERSION)pAcc->GetResDisp(), maximum(pAcc->GetMaxChars(), pAcc->m_Chars.GetCharCount())));
	new PacketCharacterList(this, pCharLast);

	m_Targ_Mode = CLIMODE_SETUP_CHARLIST;
	return PacketLoginError::Success;
}

BYTE CClient::LogIn( CAccountRef pAccount, CGString & sMsg )
{
	ADDTOCALLSTACK("CClient::LogIn");
	if ( pAccount == NULL )
		return( PacketLoginError::Invalid );

	if ( pAccount->IsPriv( PRIV_BLOCKED ))
	{
		g_Log.Event(LOGM_CLIENTS_LOG, "%lx: Account '%s' is blocked.\n", GetSocketID(), (LPCTSTR) pAccount->GetName());
		sMsg.Format( g_Cfg.GetDefaultMsg( DEFMSG_ACC_BLOCKED ), (LPCTSTR) g_Serv.m_sEMail );
		return( PacketLoginError::Blocked );
	}

	// Look for this account already in use.
	CClient * pClientPrev = pAccount->FindClient( this );
	if ( pClientPrev != NULL )
	{
		// Only if it's from a diff ip ?
		ASSERT( pClientPrev != this );

		bool bInUse = false;

		//	different ip - no reconnect
		if ( ! GetPeer().IsSameIP( pClientPrev->GetPeer() )) bInUse = true;
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
					pClientPrev->GetNetState()->markReadClosed();
				}
				else if ( GetConnectType() == pClientPrev->GetConnectType() ) bInUse = true;
			}
		}

		if ( bInUse )
		{
			g_Log.Event(LOGM_CLIENTS_LOG, "%lx: Account '%s' already in use.\n", GetSocketID(), (LPCTSTR) pAccount->GetName());
			sMsg = "Account already in use.";
			return PacketLoginError::InUse;
		}
	}

	if ( g_Cfg.m_iClientsMax <= 0 )
	{
		// Allow no one but locals on.
		CSocketAddress SockName = GetPeer();
		if ( ! GetPeer().IsLocalAddr() && SockName.GetAddrIP() != GetPeer().GetAddrIP() )
		{
			g_Log.Event(LOGM_CLIENTS_LOG, "%lx: Account '%s', maximum clients reached (only local connections allowed).\n", GetSocketID(), (LPCTSTR) pAccount->GetName());
			sMsg = g_Cfg.GetDefaultMsg( DEFMSG_SERV_LD );
			return( PacketLoginError::MaxClients );
		}
	}
	if ( g_Cfg.m_iClientsMax <= 1 )
	{
		// Allow no one but Administrator on.
		if ( pAccount->GetPrivLevel() < PLEVEL_Admin )
		{
			g_Log.Event(LOGM_CLIENTS_LOG, "%lx: Account '%s', maximum clients reached (only administrators allowed).\n", GetSocketID(), (LPCTSTR) pAccount->GetName());
			sMsg = g_Cfg.GetDefaultMsg( DEFMSG_SERV_AO );
			return( PacketLoginError::MaxClients );
		}
	}
	if ( pAccount->GetPrivLevel() < PLEVEL_GM &&
		g_Serv.StatGet(SERV_STAT_CLIENTS) > g_Cfg.m_iClientsMax  )
	{
		// Give them a polite goodbye.
		g_Log.Event(LOGM_CLIENTS_LOG, "%lx: Account '%s', maximum clients reached.\n", GetSocketID(), (LPCTSTR) pAccount->GetName());
		sMsg = g_Cfg.GetDefaultMsg( DEFMSG_SERV_FULL );
		return( PacketLoginError::MaxClients );
	}
	//	Do the scripts allow to login this account?
	pAccount->m_Last_IP = GetPeer();
	CScriptTriggerArgs Args;
	Args.Init(pAccount->GetName());
	Args.m_iN1 = GetConnectType();
	Args.m_pO1 = this;
	enum TRIGRET_TYPE tr;
	g_Serv.r_Call("f_onaccount_login", &g_Serv, &Args, NULL, &tr);
	if ( tr == TRIGRET_RET_TRUE )
	{
		sMsg = g_Cfg.GetDefaultMsg( DEFMSG_ACC_DENIED );
		return (PacketLoginError::Blocked);
	}

	m_pAccount = pAccount;
	pAccount->OnLogin( this );

	return( PacketLoginError::Success );
}

BYTE CClient::LogIn( LPCTSTR pszAccName, LPCTSTR pszPassword, CGString & sMsg )
{
	ADDTOCALLSTACK("CClient::LogIn");
	// Try to validate this account.
	// Do not send output messages as this might be a console or web page or game client.
	// NOTE: addLoginErr() will get called after this.

	if ( GetAccount()) // already logged in.
		return( PacketLoginError::Success );

	TCHAR szTmp[ MAX_NAME_SIZE ];
	int iLen1 = strlen( pszAccName );
	int iLen2 = strlen( pszPassword );
	int iLen3 = Str_GetBare( szTmp, pszAccName, MAX_NAME_SIZE );
	if ( iLen1 == 0 ||
		iLen1 != iLen3 ||
		iLen1 > MAX_NAME_SIZE )	// a corrupt message.
	{
		TCHAR szVersion[ 256 ];
		sMsg.Format( g_Cfg.GetDefaultMsg( DEFMSG_ACC_WCLI ), (LPCTSTR) m_Crypt.WriteClientVer( szVersion ));
		return( PacketLoginError::BadAccount );
	}

	iLen3 = Str_GetBare( szTmp, pszPassword, MAX_NAME_SIZE );
	if ( iLen2 != iLen3 )	// a corrupt message.
	{
		TCHAR szVersion[ 256 ];
		sMsg.Format( g_Cfg.GetDefaultMsg( DEFMSG_ACC_WCLI ), (LPCTSTR) m_Crypt.WriteClientVer( szVersion ));
		return( PacketLoginError::BadPassword );
	}


	TCHAR szName[ MAX_ACCOUNT_NAME_SIZE ];
	if ( !CAccount::NameStrip(szName, pszAccName) || Str_Check(pszAccName) )
	{
		return( PacketLoginError::BadAccount );
	}
	else if ( Str_Check(pszPassword) )
	{
		return( PacketLoginError::BadPassword );
	}

	bool fGuestAccount = ! strnicmp( pszAccName, "GUEST", 5 );
	if ( fGuestAccount )
	{
		// trying to log in as some sort of guest.
		// Find or create a new guest account.
		TCHAR *pszTemp = Str_GetTemp();
		for ( int i = 0; ; i++ )
		{
			if ( i>=g_Cfg.m_iGuestsMax )
			{
				sMsg = g_Cfg.GetDefaultMsg( DEFMSG_ACC_GUSED );
				return( PacketLoginError::MaxGuests );
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
			return( PacketLoginError::BadPassword );
		}
	}

	bool fAutoCreate = ( g_Serv.m_eAccApp == ACCAPP_Free || g_Serv.m_eAccApp == ACCAPP_GuestAuto || g_Serv.m_eAccApp == ACCAPP_GuestTrial );

	CAccountRef pAccount = g_Accounts.Account_FindCreate(pszAccName, fAutoCreate);
	if ( ! pAccount )
	{
		g_Log.Event(LOGM_CLIENTS_LOG, "%lx:ERR Login NO Account '%s'\n", GetSocketID(), pszAccName);
		sMsg.Format(g_Cfg.GetDefaultMsg(DEFMSG_ACC_UNK), pszAccName);
		return PacketLoginError::Invalid;
	}

	if ( g_Cfg.m_iMaxAccountLoginTries && !pAccount->CheckPasswordTries(GetPeer()))
	{
		g_Log.Event(LOGM_CLIENTS_LOG, "%lx: '%s' exceeded password tries in time lapse\n", GetSocketID(), (LPCTSTR) pAccount->GetName());
		sMsg = g_Cfg.GetDefaultMsg(DEFMSG_ACC_BADPASS);
		return PacketLoginError::MaxPassTries;
	}

	if ( ! fGuestAccount && ! pAccount->IsPriv(PRIV_BLOCKED) )
	{
		if ( ! pAccount->CheckPassword(pszPassword))
		{
			g_Log.Event(LOGM_CLIENTS_LOG, "%lx: '%s' bad password\n", GetSocketID(), (LPCTSTR) pAccount->GetName());
			sMsg = g_Cfg.GetDefaultMsg(DEFMSG_ACC_BADPASS);
			return PacketLoginError::BadPass;
		}
	}

	return LogIn(pAccount, sMsg);
}


