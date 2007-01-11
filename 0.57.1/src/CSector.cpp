#include "graysvr.h"

//#define	NEW_MAP_CACHE

////////////////////////////////////////////////////////////////////////
// -CCharsActiveList

CCharsActiveList::CCharsActiveList()
{
	m_timeLastClient.Init();
	m_iClients = 0;
}

void CCharsActiveList::OnRemoveOb( CGObListRec * pObRec )
{
	// Override this = called when removed from group.
	CChar * pChar = static_cast <CChar*>(pObRec);
	if ( pChar->IsClient())
	{
		ClientDetach();
		m_timeLastClient = CServTime::GetCurrentTime();	// mark time in case it's the last client
	}
	CGObList::OnRemoveOb(pObRec);
	pChar->SetContainerFlags(UID_O_DISCONNECT);
}

void CCharsActiveList::AddCharToSector(CChar *pChar)
{
	if ( pChar )
	{
		if ( pChar->IsClient() )
			ClientAttach();
		CGObList::InsertHead(pChar);
	}
}

void CCharsActiveList::ClientAttach()
{
	m_iClients++;
}

void CCharsActiveList::ClientDetach()
{
	m_iClients--;
}

//////////////////////////////////////////////////////////////
// -CItemList

bool CItemsList::sm_fNotAMove = false;

void CItemsList::OnRemoveOb(CGObListRec *pObRec)
{
	// Item is picked up off the ground. (may be put right back down though)
	CItem * pItem = static_cast <CItem*>(pObRec);
	if ( ! sm_fNotAMove )
	{
		pItem->OnMoveFrom();	// IT_MULTI, IT_SHIP and IT_COMM_CRYSTAL
	}

	CGObList::OnRemoveOb(pObRec);
	pItem->SetContainerFlags(UID_O_DISCONNECT);	// It is no place for the moment.
}

void CItemsList::AddItemToSector(CItem *pItem)
{
	// Add to top level.
	if ( pItem )
		CGObList::InsertHead(pItem);
}

//////////////////////////////////////////////////////////////////
// -CSector :: Environment

CSector::Environment::Environment()
{
	m_Light = LIGHT_BRIGHT;
	m_Season = SEASON_Summer;
	m_Weather = WEATHER_DRY;
}

void CSector::Environment::invalidate()
{
	// Force a resync of all this. we changed location by teleport etc.
	m_Light = -1;
	m_Season = SEASON_QTY;
	m_Weather = WEATHER_DEFAULT;
}

//////////////////////////////////////////////////////////////////
// -CSector

CSector::CSector()
{
	m_map = 0;
	m_index = 0;

	m_ListenItems = 0;

	m_RainChance = 15;		// 0 to 100%
	m_ColdChance = 1;		// Will be snow if rain chance success.

	//	set default weather
	CPointMap pt = GetBasePoint();
	int iPercent = IMULDIV(pt.m_y, 100, g_MapList.GetY(pt.m_map));	// 100 = south

	if ( iPercent < 50 )		// snow likely to north
		m_ColdChance += ( 49 - iPercent ) * 2;
	else						// rainy to the south
		m_RainChance += ( iPercent - 50 ) / 10;

	m_fSaveParity = false;
	m_bSleeping = true;
}

CSector::~CSector()
{
}

enum SC_TYPE
{
	SC_CLIENTS,
	SC_COLDCHANCE,
	SC_COMPLEXITY,
	SC_ISDARK,
	SC_ISNIGHTTIME,
    SC_ITEMCOUNT,
	SC_LIGHT,
	SC_LOCALHOUR,
	SC_LOCALTIME,
	SC_LOCALTOD,
	SC_RAINCHANCE,
	SC_SEASON,
	SC_QTY,
};

LPCTSTR const CSector::sm_szLoadKeys[SC_QTY+1] =
{
	"CLIENTS",
	"COLDCHANCE",
	"COMPLEXITY",
	"ISDARK",
	"ISNIGHTTIME",
	"ITEMCOUNT",
	"LIGHT",
	"LOCALHOUR",
	"LOCALTIME",
	"LOCALTOD",
	"RAINCHANCE",
	"SEASON",
	NULL,
};

bool CSector::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	EXC_TRY("WriteVal");
	static const CValStr sm_ComplexityTitles[] =
	{
		"HIGH", INT_MIN,	// speech can be very complex if low char count
		"MEDIUM", 5,
		"LOW", 10,
		NULL, INT_MAX,
	};
	switch ( FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case SC_CLIENTS:
			sVal.FormatVal(m_Chars_Active.HasClients());
			return true;
		case SC_COLDCHANCE:
			sVal.FormatVal( GetColdChance());
			return true;
		case SC_COMPLEXITY:
			if ( pszKey[10] == '.' )
			{
				pszKey += 11;
				sVal = ( ! strcmpi( pszKey, sm_ComplexityTitles->FindName( GetCharComplexity()))) ? "1" : "0";
				return true;
			}
			sVal.FormatVal( GetCharComplexity());
			return true;
		case SC_LIGHT:
			sVal.FormatVal(GetLight());
			return true;
		case SC_LOCALHOUR:
			sVal = GetLocalGameTime(true);
			return true;
		case SC_LOCALTIME:
			sVal = GetLocalGameTime();
			return true;
		case SC_LOCALTOD:
			sVal.FormatVal(GetLocalTime());
			return true;
		case SC_ISDARK:
			sVal.FormatVal( IsDark() );
			return true;
		case SC_ISNIGHTTIME:
			{
				int iMinutes = GetLocalTime();
				sVal = ( iMinutes < 7*60 || iMinutes > (9+12)*60 ) ? "1" : "0";
			}
			return true;
		case SC_RAINCHANCE:
			sVal.FormatVal( GetRainChance());
			return true;
		case SC_ITEMCOUNT:
			sVal.FormatVal(GetItemComplexity());
			return true;
		case SC_SEASON:
			sVal.FormatVal((int)GetSeason());
			return true;
	}
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CSector::r_LoadVal( CScript &s )
{
	EXC_TRY("LoadVal");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case SC_COLDCHANCE:
			SetWeatherChance( false, s.HasArgs() ? s.GetArgVal() : -1 );
			return true;
		case SC_LIGHT:
			if ( g_Cfg.m_bAllowLightOverride )
				m_Env.m_Light = s.GetArgVal() | LIGHT_OVERRIDE;
			return true;
		case SC_RAINCHANCE:
			SetWeatherChance( true, s.HasArgs() ? s.GetArgVal() : -1 );
			return true;
		case SC_SEASON:
			SetSeason(s.HasArgs() ? (SEASON_TYPE) s.GetArgVal() : SEASON_Summer);
			return (true);
	}
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

enum SEV_TYPE
{
	#define ADD(a,b) SEV_##a,
	#include "tables/CSector_functions.tbl"
	#undef ADD
	SEV_QTY,
};

LPCTSTR const CSector::sm_szVerbKeys[SEV_QTY+1] =
{
	#define ADD(a,b) b,
	#include "tables/CSector_functions.tbl"
	#undef ADD
	NULL,
};

bool CSector::r_Verb( CScript & s, CTextConsole * pSrc )
{
	EXC_TRY("Verb");
	int index = FindTableSorted( s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys)-1 );
	switch (index)
	{
		case SEV_ALLCHARS:		// "ALLCHARS"
			v_AllChars( s, pSrc );
			break;
		case SEV_ALLCHARSIDLE:	// "ALLCHARSIDLE"
			v_AllCharsIdle( s, pSrc );
			break;
		case SEV_ALLCLIENTS:	// "ALLCLIENTS"
			v_AllClients( s, pSrc );
			break;
		case SEV_ALLITEMS:		// "ALLITEMS"
			v_AllItems( s, pSrc );
			break;
		case SEV_DRY:	// "DRY"
			SetWeather( WEATHER_DRY );
			break;
		case SEV_LIGHT:
			if ( g_Cfg.m_bAllowLightOverride )
				SetLight( (s.HasArgs()) ? s.GetArgVal() : -1 );
			break;
		case SEV_RAIN:
			SetWeather( (s.HasArgs()) ? ((WEATHER_TYPE) s.GetArgVal()) : WEATHER_RAIN );
			break;
		case SEV_RESPAWN:
			RespawnDeadNPCs();
			break;
		case SEV_RESTOCK:
			Restock();
			break;
		case SEV_SEASON:
			SetSeason( (SEASON_TYPE) s.GetArgVal() );
			break;
		case SEV_SNOW:
			SetWeather( WEATHER_SNOW );
			break;
		default:
			return( CScriptObj::r_Verb( s, pSrc ));
	}
	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

void CSector::r_Write()
{
	if ( m_fSaveParity == g_World.m_fSaveParity )
		return;	// already saved.
	CPointMap pt = GetBasePoint();

	m_fSaveParity = g_World.m_fSaveParity;

	if ( g_Cfg.m_bAllowLightOverride || !g_Cfg.m_fNoWeather )
	{
		if ( IsLightOverriden() || IsRainOverriden() || IsColdOverriden() )
		{
			g_World.m_FileWorld.WriteSection("SECTOR %d,%d,0,%d", pt.m_x, pt.m_y, pt.m_map );
			if (GetSeason() != SEASON_Summer)
				g_World.m_FileWorld.WriteKeyVal("SEASON", GetSeason());
			if ( g_Cfg.m_bAllowLightOverride )
			{
				if ( IsLightOverriden() )
					g_World.m_FileWorld.WriteKeyVal("LIGHT", GetLight());
			}
			if ( !g_Cfg.m_fNoWeather )
			{
				if ( IsRainOverriden() )
					g_World.m_FileWorld.WriteKeyVal("RAINCHANCE", GetRainChance());
				if ( IsColdOverriden() )
					g_World.m_FileWorld.WriteKeyVal("COLDCHANCE", GetColdChance());
			}
		}
	}

	// Chars in the sector.
	CChar * pCharNext;
	CChar * pChar = static_cast <CChar*>( m_Chars_Active.GetHead());
	for ( ; pChar != NULL; pChar = pCharNext )
	{
		pCharNext = pChar->GetNext();
		if ( pChar->m_pPlayer )
		{
			pChar->r_WriteParity( g_World.m_FilePlayers );
		}
		else
		{
			pChar->r_WriteParity( g_World.m_FileWorld );
		}
	}
	// Inactive Client Chars, ridden horses and dead npcs
	// NOTE: ??? Push inactive player chars out to the account files here ?
	pChar = static_cast <CChar*> (m_Chars_Disconnect.GetHead());
	for ( ; pChar != NULL; pChar = pCharNext )
	{
		pCharNext = pChar->GetNext();
		if ( pChar->m_pPlayer )
		{
			pChar->r_WriteParity( g_World.m_FilePlayers );
		}
		else
		{
			pChar->r_WriteParity( g_World.m_FileWorld );
		}
	}

	// Items on the ground.
	CItem * pItemNext;
	CItem * pItem = static_cast <CItem*> (m_Items_Inert.GetHead());
	for ( ; pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		pItem->r_WriteSafe( pItem->IsAttr(ATTR_STATIC) ? g_World.m_FileStatics : g_World.m_FileWorld );
	}
	pItem = static_cast <CItem*> (m_Items_Timer.GetHead());
	for ( ; pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		pItem->r_WriteSafe( pItem->IsAttr(ATTR_STATIC) ? g_World.m_FileStatics : g_World.m_FileWorld );
	}
}

bool CSector::v_AllChars(CScript &s, CTextConsole * pSrc)
{
	CScript	script(s.GetArgStr());
	CChar	*pChar = NULL;
	bool fRet = false;
	
	// Loop through all the characters in m_Chars_Active.
	// We should start at the end incase some are removed during the loop.
	for (int i = (m_Chars_Active.GetCount() - 1); i >= 0; i--)
	{
		pChar = static_cast <CChar*>(m_Chars_Active.GetAt(i));
		if ( !pChar )
			continue;
		fRet |= pChar->r_Verb(script, pSrc);
	}
	return fRet;
}

bool CSector::v_AllCharsIdle(CScript &s, CTextConsole *pSrc)
{
	CScript	script(s.GetArgStr());
	CChar	*pChar = NULL;
	bool fRet = false;
	
	// Loop through all the characters in m_Chars_Disconnect.
	// We should start at the end incase some are removed during the loop.
	for (int i = (m_Chars_Disconnect.GetCount() - 1); i >= 0; i--)
	{
		pChar = static_cast <CChar*>(m_Chars_Disconnect.GetAt(i));
		if ( !pChar )
			continue;
		fRet |= pChar->r_Verb(script, pSrc);
	}
	return fRet;
}

bool CSector::v_AllItems(CScript &s, CTextConsole *pSrc)
{
	CScript	script(s.GetArgStr());
	CItem	*pItem = NULL;
	bool fRet = false;
	int i;

	// Loop through all the items in m_Items_Timer.
	// We should start at the end in case items are removed during the loop.
	for (i = (m_Items_Timer.GetCount() - 1); i >= 0; i--)
	{
		pItem = static_cast <CItem*>(m_Items_Timer.GetAt(i));
		if ( !pItem )
			continue;
		fRet |= pItem->r_Verb(script, pSrc);
	}
	
	// Loop through all the items in m_Items_Inert.
	// We should start at the end in case items are removed during the loop.
	for (i = (m_Items_Inert.GetCount() - 1); i >= 0; i--)
	{
		pItem = static_cast <CItem*>(m_Items_Inert.GetAt(i));
		if ( !pItem )
			continue;
		fRet |= pItem->r_Verb(script, pSrc);
	}
	return fRet;
}

bool CSector::v_AllClients(CScript &s, CTextConsole *pSrc)
{
	CScript script(s.GetArgStr());
	CChar *pChar = NULL;
	bool fRet = false;

	// Loop through all the characters in m_Chars_Active.
	// We should start at the end in case some are removed during the loop.
	for (int i = (m_Chars_Active.GetCount() - 1); i >= 0; i--)
	{
		pChar = static_cast <CChar*>(m_Chars_Active.GetAt(i));

		if ( !pChar || !pChar->IsClient() )
			continue;

		fRet |= pChar->r_Verb(script, pSrc);
	}
	return fRet;
}

int CSector::GetLocalTime() const
{
	//	Get local time of the day (in minutes)
	CPointMap	pt = GetBasePoint();
	int			iLocalTime = g_World.GetGameWorldTime();

	iLocalTime += ( pt.m_x * 24*60 ) / g_MapList.GetX(pt.m_map);
	iLocalTime %= (24*60);
	return iLocalTime;
}

LPCTSTR CSector::GetLocalGameTime(bool numeric) const
{
	return GetTimeMinDesc(GetLocalTime(), numeric);
}

BYTE CSector::GetLightCalc() const
{
	// What is the light level default here in this sector.
	if ( g_Cfg.m_bAllowLightOverride && IsLightOverriden() )
	{
		return m_Env.m_Light;
	}

	if ( IsInDungeon() )
	{
		return g_Cfg.m_iLightDungeon;
	}

	int localTime = GetLocalTime();

	//	Normalize time:
	//	convert	0=midnight	.. (23*60)+59=midnight
	//	to		0=day		.. 12*60=night
	if ( localTime < 12*60 )
		localTime = 12*60 - localTime;
	else
		localTime -= 12*60;

	//	0...	y	...lightnight
	//	0...	x	...12*60
	int targetLight = ((localTime * ( g_Cfg.m_iLightNight - g_Cfg.m_iLightDay ))/(12*60) + g_Cfg.m_iLightDay);
	targetLight = GetSeasonLightDiff(targetLight);

	if ( targetLight < LIGHT_BRIGHT ) targetLight = LIGHT_BRIGHT;
	if ( targetLight > LIGHT_DARK ) targetLight = LIGHT_DARK;

	return targetLight;
}

int CSector::GetSeasonLightDiff(int light) const
{
	long diff = 0;

	switch ( m_Env.m_Season )
	{
	case SEASON_Summer:
	case SEASON_Nice:
		diff = -6; break;	// lighter in summer
	case SEASON_Winter:
	case SEASON_Desolate:
		diff = +3; break;	// darker in winter
	}

	return ( light + diff );
}

void CSector::SetLightNow( bool fFlash )
{
	// Set the light level for all the CClients here.

	CChar * pChar = static_cast <CChar*>( m_Chars_Active.GetHead());
	for ( ; pChar != NULL; pChar = pChar->GetNext() )
	{
		if ( pChar->IsStatFlag(STATF_DEAD|STATF_NightSight) )
			continue;

		CClient *pClient = pChar->GetClient();
		if ( pClient )
		{
			if ( fFlash )
			{
				BYTE bPrvLight = m_Env.m_Light;
				m_Env.m_Light = LIGHT_BRIGHT;
				pClient->addLight();
				m_Env.m_Light = bPrvLight;	// back to previous.
			}
			pClient->addLight();
		}

		if ( !g_Serv.IsLoading() )
		{
			pChar->OnTrigger(CTRIG_EnvironChange, pChar);
		}
	}
}

void CSector::SetLight( int light )
{
	// GM set light level command
	// light =LIGHT_BRIGHT, LIGHT_DARK=dark

	if ( light < LIGHT_BRIGHT || light > LIGHT_DARK )
	{
		m_Env.m_Light &= ~LIGHT_OVERRIDE;
		m_Env.m_Light = GetLightCalc();
	}
	else
	{
		m_Env.m_Light = (BYTE) ( light | LIGHT_OVERRIDE );
	}
	SetLightNow(false);
}

WEATHER_TYPE CSector::GetWeatherCalc() const
{
	// (1 in x) chance of some kind of weather change at any given time
	if ( IsInDungeon() || g_Cfg.m_fNoWeather )
		return( WEATHER_DRY );

	int iRainRoll = Calc_GetRandVal( 100 );
	if ( ( GetRainChance() * 2) < iRainRoll )
		return( WEATHER_DRY );

	// Is it just cloudy?
	if ( iRainRoll > GetRainChance())
		return WEATHER_CLOUDY;

	// It is snowing
	if ( Calc_GetRandVal(100) <= GetColdChance()) // Can it actually snow here?
		return WEATHER_SNOW;

	// It is raining
	return WEATHER_RAIN;
}

void CSector::SetWeather( WEATHER_TYPE w )
{
	// Set the immediate weather type.
	// 0=dry, 1=rain or 2=snow.

	if ( w == m_Env.m_Weather )
		return;

	m_Env.m_Weather = w;

	CChar * pChar = static_cast <CChar*>( m_Chars_Active.GetHead());
	for ( ; pChar != NULL; pChar = pChar->GetNext())
	{
		if ( pChar->IsClient())
		{
			pChar->GetClient()->addWeather( w );
		}
		pChar->OnTrigger( CTRIG_EnvironChange, pChar );
	}
}

void CSector::SetSeason( SEASON_TYPE season )
{
	// Set the season type.

	if ( season == m_Env.m_Season )
		return;

	m_Env.m_Season = season;

	CChar * pChar = static_cast <CChar*>( m_Chars_Active.GetHead());
	for ( ; pChar != NULL; pChar = pChar->GetNext())
	{
		if ( pChar->IsClient() )
			pChar->GetClient()->addSeason(season);
		pChar->OnTrigger(CTRIG_EnvironChange, pChar);
	}
}

void CSector::SetWeatherChance( bool fRain, int iChance )
{
	// Set via the client.
	// Transfer from snow to rain does not work ! must be DRY first.

	if ( iChance > 100 )
		iChance = 100;

	else if ( fRain )
	{
		m_RainChance = iChance | LIGHT_OVERRIDE;
	}
	else if ( iChance > 0 )
	{
		m_ColdChance = iChance | LIGHT_OVERRIDE;
	}

	// Recalc the weather immediatly.
	SetWeather( GetWeatherCalc());
}

void CSector::OnHearItem( CChar * pChar, TCHAR * szText )
{
	// report to any of the items that something was said.

	CItem * pItemNext;
	CItem * pItem = static_cast <CItem*>( m_Items_Timer.GetHead());
	for ( ; pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		pItem->OnHear( szText, pChar );
	}
	pItem = static_cast <CItem*>( m_Items_Inert.GetHead());
	for ( ; pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		pItem->OnHear( szText, pChar );
	}
}

void CSector::MoveItemToSector( CItem * pItem, bool fActive )
{
	// remove from previous list and put in new.
	// May just be setting a timer. SetTimer or MoveTo()
	fActive ? m_Items_Timer.AddItemToSector(pItem) : m_Items_Inert.AddItemToSector(pItem);
}

bool CSector::MoveCharToSector( CChar * pChar )
{
	// Move a CChar into this CSector.
	// NOTE:
	//   m_pt is still the old location. Not moved yet!
	// ASSUME: called from CChar.MoveToChar() assume ACtive char.

	if ( IsCharActiveIn(pChar) )
		return false;	// already here

	// moving to sector with another parity (i not yet saved, but the sector is?)
	if (( pChar->IsStatFlag(STATF_SaveParity) != m_fSaveParity ) && ( m_fSaveParity == g_World.m_fSaveParity ))
	{
		g_World.m_LostPlayers.push_back(pChar->uid());
	}

	if ( pChar->IsClient() )
	{
		// Send new weather and light for this sector
		// Only send if different than last ???

		CClient * pClient = pChar->GetClient();

		if ( !pChar->IsStatFlag(STATF_DEAD|STATF_NightSight) )
			pClient->addLight(GetLight());

		// Provide the weather as an arg as we are not in the new location yet.
		pClient->addWeather(GetWeather());
		pClient->addSeason(GetSeason());
	}

	m_Chars_Active.AddCharToSector(pChar);		// remove from previous spot.
	return true;
}

void CSector::RespawnDeadNPCs()
{
	// skip sectors in unsupported maps
	if ( !g_MapList.m_maps[m_map] ) return;

	// Respawn dead NPC's
	m_Chars_Disconnect.Lock();
	CChar * pCharNext;
	CChar * pChar = static_cast <CChar *>( m_Chars_Disconnect.GetHead());
	for ( ; pChar != NULL; pChar = pCharNext )
	{
		pCharNext = pChar->GetNext();
		if ( !pChar->m_pNPC || !pChar->m_ptHome.IsValidPoint() || !pChar->IsStatFlag(STATF_DEAD) )
			continue;

		// Res them back to their "home".
		int iDist = pChar->m_pNPC->m_Home_Dist_Wander;
		pChar->MoveNear( pChar->m_ptHome, ( iDist < SHRT_MAX ) ? iDist : 4 );
		pChar->Spell_Resurrection();

		// Restock them with npc stuff.
		pChar->NPC_LoadScript(true);
	}
	m_Chars_Disconnect.Release();
}

void CSector::Restock()
{
	//	restock all vendors in sector

	m_Chars_Active.Lock();
	CChar *pCharNext;
	for ( CChar *pChar = static_cast <CChar*>(m_Chars_Active.GetHead()); pChar ; pChar = pCharNext )
	{
		pCharNext = pChar->GetNext();
		pChar->NPC_Vendor_Restock(true);
	}
	m_Chars_Active.Release();
}

void CSector::OnTick(int iPulseCount)
{
	// CWorld gives OnTick() to all CSectors.
#ifdef DEBUGTIMING
	TIME_PROFILE_INIT;
	TIME_PROFILE_START;
#endif
	CChar *pCharNext, *pChar;

	EXC_TRY("Tick");
	EXC_SET("light change");

	//	do not tick sectors on maps not supported by server
	if ( !g_MapList.m_maps[m_map] )
		return;

	// Check for light change before putting the sector to sleep, since in other case the
	// world light levels will be shitty
	bool fEnvironChange = false;
	bool fLightChange = false;

	if ( ! ( iPulseCount & 0x7f ))	// 30 seconds or so.
	{
		// check for local light level change ?
		BYTE blightprv = m_Env.m_Light;
		m_Env.m_Light = GetLightCalc();
		if ( m_Env.m_Light != blightprv )
		{
			fEnvironChange = true;
			fLightChange = true;
		}
	}

	EXC_SET("sector sleeping?");
	CheckSleepingStatus();

	if ( IsSleeping() )
		return;

	// random weather noises and effects.
	SOUND_TYPE sound = 0;
	bool fWeatherChange = false;
	int iRegionPeriodic = 0;

	if ( g_Cfg.m_fGenericSounds && !( iPulseCount & 0x7f ))	// 30 seconds or so.
	{
		EXC_SET("sound effects");
		// Only do this every x minutes or so (TICK_PER_SEC)
		// check for local weather change ?
		WEATHER_TYPE weatherprv = m_Env.m_Weather;
		if ( ! Calc_GetRandVal( 30 ))	// change less often
		{
			m_Env.m_Weather = GetWeatherCalc();
			if ( weatherprv != m_Env.m_Weather )
			{
				fWeatherChange = true;
				fEnvironChange = true;
			}
		}

		// Random area noises.
		iRegionPeriodic = 2;

		static const SOUND_TYPE sm_SfxRain[] = { 0x10, 0x11 };
		static const SOUND_TYPE sm_SfxWind[] = { 0x14, 0x15, 0x16 };
		static const SOUND_TYPE sm_SfxThunder[] = { 0x28, 0x29 , 0x206 };

		switch ( GetWeather() )
		{
		case WEATHER_SNOW:
			if ( ! Calc_GetRandVal(5) )
				sound = sm_SfxWind[ Calc_GetRandVal( COUNTOF( sm_SfxWind )) ];
			break;

		case WEATHER_RAIN:
			{
				int iVal = Calc_GetRandVal(30);
				if ( iVal < 5 )
				{
					// Mess up the light levels for a sec..
					LightFlash();
					sound = sm_SfxThunder[ Calc_GetRandVal( COUNTOF( sm_SfxThunder )) ];
				}
				else if ( iVal < 10 )
					sound = sm_SfxRain[ Calc_GetRandVal( COUNTOF( sm_SfxRain )) ];
				else if ( iVal < 15 )
					sound = sm_SfxWind[ Calc_GetRandVal( COUNTOF( sm_SfxWind )) ];
			}
			break;
		}
	}

	// regenerate all creatures and do AI

	m_Chars_Active.Lock();
	pChar = static_cast <CChar*>( m_Chars_Active.GetHead());
	for ( ; pChar != NULL; pChar = pCharNext )
	{
		EXC_TRYSUB("TickChar");

		pCharNext = pChar->GetNext();
		if ( fEnvironChange )
			pChar->OnTrigger(CTRIG_EnvironChange, pChar);

		if ( pChar->IsClient())
		{
			CClient * pClient = pChar->GetClient();
			if ( sound )
				pClient->addSound(sound, pChar);

			if ( fLightChange && ! pChar->IsStatFlag( STATF_DEAD | STATF_NightSight ))
				pClient->addLight();

			if ( fWeatherChange )
				pClient->addWeather(GetWeather());

			if ( iRegionPeriodic && pChar->m_pArea )
			{
				if ( iRegionPeriodic == 2 )
				{
					if ( IsTrigUsed(TRIGGER_REGPERIODIC) )
					{
						pChar->m_pArea->OnRegionTrigger( pChar, RTRIG_REGPERIODIC );
					}
					iRegionPeriodic--;
				}
				if ( IsTrigUsed(TRIGGER_CLIPERIODIC) )
				{
					pChar->m_pArea->OnRegionTrigger( pChar, RTRIG_CLIPERIODIC );
				}
			}
		}
		// Can only die on your own tick.
		if ( !pChar->OnTick() )
			pChar->Delete();

		EXC_CATCHSUB("Sector");

		EXC_DEBUGSUB_START;
		CPointMap pt = GetBasePoint();
		g_Log.Debug("char 0%lx '%s'\n", pChar->GetUID(), pChar->GetName());
		g_Log.Debug("sector #%d [%d,%d,%d,%d]\n", m_index, pt.m_x, pt.m_y, pt.m_z, pt.m_map);
		EXC_DEBUGSUB_END;
	}
	m_Chars_Active.Release();

	// decay items on ground = time out spells / gates etc.. etc..
	// No need to check these so often !

	m_Items_Timer.Lock();
	CItem * pItemNext;
	CItem * pItem = static_cast <CItem*>( m_Items_Timer.GetHead());
	for ( ; pItem != NULL; pItem = pItemNext )
	{
		EXC_TRYSUB("TickItem");
		pItemNext = pItem->GetNext();

		if ( !pItem->IsTimerExpired() )
			continue;

		if ( !pItem->OnTick() )
			pItem->Delete();
		else
		{
			if ( pItem->IsTimerExpired() )	// forgot to clear the timer.? strange.
				pItem->SetTimeout(-1);
		}
		EXC_CATCHSUB("Sector");

		EXC_DEBUGSUB_START;
		CPointMap pt = GetBasePoint();
		g_Log.Debug("item 0%lx '%s' [timer=%d, type=%d]\n", pItem->GetUID(), pItem->GetName(), pItem->GetTimerAdjusted(), (int)pItem->GetType());
		g_Log.Debug("sector #%d [%d,%d,%d,%d]\n", m_index, pt.m_x, pt.m_y, pt.m_z, pt.m_map);
		
		EXC_DEBUGSUB_END;
	}
	m_Items_Timer.Release();

	EXC_SET("check map cache");
	if ( IsSleeping() || ! ( iPulseCount & 0x7f ))	// 30 seconds or so.
	{
		// delete the static CGrayMapBlock items that have not been used recently.
		CheckMapBlockCache( IsSleeping() ? 0 : 2*60*TICK_PER_SEC );
	}
	EXC_CATCH;
#ifdef DEBUGTIMING
	TIME_PROFILE_END;
	int	hi = TIME_PROFILE_GET_HI;
	if ( hi > 1 )
		g_Log.Debug("CSector::OnTick(%d) [ticking sector] took %d.%d to run\n", m_index, hi, TIME_PROFILE_GET_LO);
#endif
}

void CSector::Init(int index, int map)
{
	if (( map < 0 ) || ( map >= 256 ) || !g_MapList.m_maps[map] )
	{
		g_Log.Error("Trying to initalize a sector %d in unsupported map #%d. Defaulting to 0,0.\n", index, map);
	}
	else if (( index < 0 ) || ( index >= g_MapList.GetSectorQty(map) ))
	{
		m_map = map;
		g_Log.Error("Trying to initalize a sector by sector number %d out-of-range for map #%d. Defaulting to 0,%d.\n", index, map, map);
	}
	else
	{
		m_index = index;
		m_map = map;
	}
}

void CSector::CheckMapBlockCache( int iTime )
{
	// Clean out the sectors map cache if it has not been used recently.
	// iTime == 0 = delete all.
	for ( int i = 0; i < m_MapBlockCache.GetCount(); i++ )
	{
		EXC_TRY("CheckMapBlockCache");
		CGrayMapBlock	*pMapBlock = static_cast <CGrayMapBlock *>(m_MapBlockCache[i]);
		// NOTE: Experimental thing... if we can't cast it to CGrayMapBlock
		// let's remove it, it'll be reloaded if necessary.
		if ( pMapBlock == NULL )
		{
			m_MapBlockCache.DeleteAt(i);
			i--;
			continue;
		}
		if (( iTime <= 0 ) || ( pMapBlock->m_CacheTime.GetCacheAge() >= iTime ))
		{
			m_MapBlockCache.DeleteAt(i);
			i--;
		}
		EXC_CATCH;
		EXC_DEBUG_START;
		CPointMap pt = GetBasePoint();
		g_Log.Debug("check time %d, index %d/%d\n", iTime, i, m_MapBlockCache.GetCount());
		g_Log.Debug("sector #%d [%d,%d,%d,%d]\n", m_index, pt.m_x, pt.m_y, pt.m_z, pt.m_map);
		EXC_DEBUG_END;
	}
}


const CGrayMapBlock * CSector::GetMapBlock( const CPointMap & pt )
{
	// Get a map block from the cache. load it if not.
	if ( !pt.IsValidXY() )
	{
		g_Log.Warn("Attempting to access invalid memory block at %s.\n", pt.WriteUsed());
		return NULL;
	}

	CPointMap pntBlock( UO_BLOCK_ALIGN(pt.m_x), UO_BLOCK_ALIGN(pt.m_y), 0, pt.m_map);

	CGrayMapBlock * pMapBlock;

	// Find it in cache.
	int i = m_MapBlockCache.FindKey(pntBlock.GetPointSortIndex());
	if ( i >= 0 )
	{
		pMapBlock = static_cast <CGrayMapBlock *>(m_MapBlockCache[i]);
		pMapBlock->m_CacheTime.HitCacheTime();
		return pMapBlock;
	}

	// else load it.
	try
	{
		pMapBlock = new CGrayMapBlock(pntBlock);
		ASSERT(pMapBlock);
	}
	catch (CError& e)
	{
		g_Log.Error("Exception creating new memory block at %s. (%s)\n", pntBlock.WriteUsed(), e.m_descr);
		return NULL;
	}
	catch (...)
	{
		g_Log.Error("Exception creating new memory block at %s.\n", pntBlock.WriteUsed());
		return NULL;
	}

	// Add it to the cache.
	m_MapBlockCache.AddSortKey( pMapBlock, pntBlock.GetPointSortIndex() );
	return( pMapBlock );
}

bool CSector::IsInDungeon() const
{
	// What part of the maps are filled with dungeons.
	// Used for light / weather calcs.
	CPointMap pt = GetBasePoint();
	CRegionBase *pRegion = GetRegion(pt, REGION_TYPE_AREA);

	return ( pRegion && pRegion->IsFlag(REGION_FLAG_UNDERGROUND) );
}

CRegionBase * CSector::GetRegion( const CPointBase & pt, DWORD dwType ) const
{
	// Does it match the mask of types we care about ?
	// Assume sorted so that the smallest are first.
	//
	// REGION_TYPE_AREA => RES_AREA = World region area only = CRegionWorld
	// REGION_TYPE_ROOM => RES_ROOM = NPC House areas only = CRegionBase.
	// REGION_TYPE_MULTI => RES_WORLDITEM = UID linked types in general = CRegionWorld

	int iQty = m_RegionLinks.GetCount();
	for ( int i = 0; i < iQty; i++ )
	{
		CRegionBase * pRegion = m_RegionLinks[i];
		if ( !pRegion || !pRegion->GetResourceID().IsValidUID() )
			continue;

		if ( pRegion->GetResourceID().IsItem() )
		{
			if ( ! ( dwType & REGION_TYPE_MULTI ))
				continue;
		}
		else if ( pRegion->GetResourceID().GetResType() == RES_AREA )
		{
			if ( ! ( dwType & REGION_TYPE_AREA ))
				continue;
		}
		else
		{
			if ( ! ( dwType & REGION_TYPE_ROOM ))
				continue;
		}

		if ( !pRegion->IsInside2d(pt) )
			continue;
		return pRegion;
	}
	return NULL;
}

// Balkon: get regions list (to cicle through intercepted house regions)
int CSector::GetRegions( const CPointBase & pt, DWORD dwType, CRegionLinks & rlist ) const
{
	int iQty = m_RegionLinks.GetCount();
	for ( int i=0; i<iQty; i++ )
	{
		CRegionBase * pRegion = m_RegionLinks[i];
		if ( !pRegion || !pRegion->GetResourceID().IsValidUID() )
			continue;

		if ( pRegion->GetResourceID().IsItem())
		{
			if ( ! ( dwType & REGION_TYPE_MULTI ))
				continue;
		}
		else if ( pRegion->GetResourceID().GetResType() == RES_AREA )
		{
			if ( ! ( dwType & REGION_TYPE_AREA ))
				continue;
		}
		else
		{
			if ( ! ( dwType & REGION_TYPE_ROOM ))
				continue;
		}

		if ( ! pRegion->m_pt.IsSameMap(pt.m_map))
			continue;
		if ( ! pRegion->IsInside2d( pt ))
			continue;
		rlist.Add( pRegion );
	}
	return( rlist.GetCount() );
}

bool CSector::UnLinkRegion( CRegionBase * pRegionOld )
{
	if ( !pRegionOld )
		return false;
	return m_RegionLinks.RemovePtr(pRegionOld);
}

bool CSector::LinkRegion( CRegionBase * pRegionNew )
{
	// link in a region. may have just moved !
	// Make sure the smaller regions are first in the array !
	// Later added regions from the MAP file should be the smaller ones, 
	//  according to the old rules.
	if ( !pRegionNew || !pRegionNew->IsOverlapped(GetRect()) )
		return false;

	int iQty = m_RegionLinks.GetCount();
	for ( int i=0; i<iQty; i++ )
	{
		CRegionBase * pRegion = m_RegionLinks[i];
		if ( pRegionNew == pRegion )
		{
			g_Log.Error("region already linked!\n");
			return false;
		}

		if ( pRegion->IsOverlapped(pRegionNew))
		{
			// NOTE : We should use IsInside() but my version isn't completely accurate for it's FALSE return
			if ( pRegion->IsEqualRegion( pRegionNew ))
			{
				g_Log.Error("Conflicting region!\n");
				return false;
			}
			if ( pRegionNew->IsInside(pRegion))	// it is accurate in the TRUE case.
				continue;

			// must insert before this.
			m_RegionLinks.InsertAt(i, pRegionNew);
			return true;
		}
	}

	m_RegionLinks.Add(pRegionNew);
	return true;
}

CTeleport * CSector::GetTeleport(const CPointMap &pt) const
{
	// Any teleports here at this point ?

	int i = m_Teleports.FindKey(pt.GetPointSortIndex());
	if ( i < 0 )
		return NULL;

	CTeleport *pTeleport = static_cast <CTeleport *>(m_Teleports[i]);

	if ( !pTeleport->IsSameMap(pt.m_map) || ( abs(pt.m_z - pTeleport->m_z) > 5 ))
		return NULL;

	return pTeleport;
}

bool CSector::AddTeleport( CTeleport * pTeleport )
{
	// NOTE: can't be 2 teleports from the same place !
	if ( m_Teleports.FindKey(pTeleport->GetPointSortIndex()) >= 0 )
	{
		g_Log.Error("Conflicting teleport %s!\n", pTeleport->WriteUsed());
		return false;
	}
	m_Teleports.AddSortKey(pTeleport, pTeleport->GetPointSortIndex());
	return true;
}

bool CSector::IsCharActiveIn(const CChar *pChar)
{
	// assume the char is active (not disconnected)
	return ( pChar->GetParent() == &m_Chars_Active );
}

bool CSector::IsCharDisconnectedIn(const CChar *pChar)
{
	// assume the char is active (not disconnected)
	return ( pChar->GetParent() == &m_Chars_Disconnect );
}

int CSector::GetCharComplexity() const
{
	return m_Chars_Active.GetCount();
}

int CSector::HasClients() const
{
	return m_Chars_Active.HasClients();
}

void CSector::ClientAttach(CChar *pChar)
{
	if ( !IsCharActiveIn(pChar) )
		return;

	m_Chars_Active.ClientAttach();
}

void CSector::ClientDetach(CChar *pChar)
{
	if ( !IsCharActiveIn(pChar) )
		return;

	m_Chars_Active.ClientDetach();
}

CPointMap CSector::GetBasePoint() const
{
	// What is the coord base of this sector. upper left point.
	CPointMap pt(( m_index % g_MapList.GetSectorCols(m_map)) * g_MapList.GetSectorSize(m_map),
		( m_index / g_MapList.GetSectorCols(m_map) ) * g_MapList.GetSectorSize(m_map),
		0,
		m_map);
	return pt;
}

CRectMap CSector::GetRect() const
{
	// Get a rectangle for the sector.
	CPointMap pt = GetBasePoint();
	CRectMap rect;
	rect.m_left = pt.m_x;
	rect.m_top = pt.m_y;
	rect.m_right = pt.m_x + g_MapList.GetSectorSize(pt.m_map);	// East
	rect.m_bottom = pt.m_y + g_MapList.GetSectorSize(pt.m_map);	// South
	rect.m_map = pt.m_map;
	return( rect );
}

bool CSector::IsSleeping() const
{
	return m_bSleeping;
}

void CSector::wakeUp()
{
	if ( !m_bSleeping )
		return;

	Lock();

	m_Chars_Active.m_timeLastClient = CServTime::GetCurrentTime();

	EXC_TRY("wakeUp()");

#ifdef NEW_MAP_CACHE
	//	read all points information
	CPointMap	pt = GetBasePoint();
	CUOMapBlock	mapBlock;
	CUOStaticItemRec	statics[256];
	for ( int x = pt.m_x; x < pt.m_x + g_MapList.m_sectorsize[m_map]; x++ )
	{
		for ( int y = pt.m_y; y < pt.m_y + g_MapList.m_sectorsize[m_map]; y++ )
		{
			CPointMap	pntBlock(UO_BLOCK_ALIGN(x), UO_BLOCK_ALIGN(y));
			int			bx = pt.m_x/UO_BLOCK_SIZE;
			int			by = pt.m_y/UO_BLOCK_SIZE;
			long		blockIndex = (bx*(g_MapList.GetY(m_map)/UO_BLOCK_SIZE) + by);
			CGFile		*file = &g_Install.m_Maps[g_MapList.m_mapnum[m_map]];
			CUOIndexRec	index;

			//	read map file

			index.SetupIndex(blockIndex * sizeof(CUOMapBlock), sizeof(CUOMapBlock));

			file->Seek(index.GetFileOffset(), SEEK_SET);
			file->Read(&mapBlock, sizeof(mapBlock));

			//	read statics file

			file = &g_Install.m_Staidx[g_MapList.m_mapnum[m_map]];
			g_Install.ReadMulIndex(*file, blockIndex, index);
			int staticsCount = index.GetBlockLength()/sizeof(CUOStaticItemRec);
			g_Install.ReadMulData(*file, index, statics);

			//	TODO:
		}
	}
#endif

	//	TODO:

	m_bSleeping = false;

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.Debug("sector '#%d' at map '%d'\n", m_index, m_map);
	EXC_DEBUG_END;

	Release();
}

void CSector::sleep()
{
	if ( m_bSleeping )
		return;

	Lock();

	EXC_TRY("sleep()");

#ifdef NEW_MAP_CACHE
	//	clear the cache
	m_cache.clear();
#endif

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.Debug("sector '#%d' at map '%d'\n", m_index, m_map);
	EXC_DEBUG_END;

	m_bSleeping = true;
	Release();
}

void CSector::CheckSleepingStatus()
{
	int timeSinceLastClient = -g_World.GetTimeDiff(m_Chars_Active.m_timeLastClient);

	if ( m_bSleeping )
	{
		//	sector has been visited in last 10 minutes - wake it up
		if ( timeSinceLastClient < 10*60*TICK_PER_SEC )
			wakeUp();
	}
	else
	{
		//	nobody been here for some time, so go on sleeping
		if ( timeSinceLastClient > 10*60*TICK_PER_SEC )
			sleep();
	}
}

bool CSector::isWalkable(CPointMap pt, CChar *person)
{
	if ( !person )
		return false;

	//	sector is still sleeping
	if ( m_bSleeping )
	{
		//	we wake it up since player gonna to walk here
		if ( person->m_pPlayer )
			wakeUp();
		else
			return false;
	}

	EXC_TRY("isWalkable");

	//	TODO: check walkability for this point

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.Debug("point '%d,%d,%d,%d' source '0%lx' name '%s'\n",
		pt.m_x, pt.m_y, pt.m_z, pt.m_map, person->uid(), person->GetName());
	EXC_DEBUG_END;
	return false;
}

/*
//	create initial block basing on statics
TMapCachePoint point;
point.m_itemBlock = point.m_charBlock = 0;
for ( int i = 0; i < maxX; i++ )
{
for ( int j = 0; j < maxY; j++ )
{
	CPointMap pt(i, j, 0, map);
	point.m_baseZ = 127;
	point.m_tileBlock = 0;

	const CGrayMapBlock	*pMapBlock = g_World.GetMapBlock(pt);
	int	iQty = pMapBlock->m_Statics.GetStaticQty();

	if ( iQty )
	{
		int x2 = pMapBlock->GetOffsetX(pt.m_x);
		int y2 = pMapBlock->GetOffsetY(pt.m_y);

		for ( int k = 0; k < iQty; k++ )
		{
			if ( !pMapBlock->m_Statics.IsStaticPoint(i, x2, y2) )
				continue;

			const CUOStaticItemRec *pStatic = pMapBlock->m_Statics.GetStatic(i);
			WORD wBlockThis = 0;
			CItemBase::GetItemHeight(pStatic->GetDispID(), wBlockThis);

			if (( pStatic->m_z < point.m_baseZ ) && ( wBlockThis & (CAN_I_CLIMB|CAN_I_PLATFORM|CAN_I_ROOF) ))
			{
				point.m_baseZ = pStatic->m_z;
			}

			if (( (wBlockThis & CAN_I_BLOCK|CAN_I_PLATFORM) == CAN_I_BLOCK ) ||
				( wBlockThis & CAN_I_FIRE ) ||
				( wBlockThis & CAN_I_WATER ) )
			{
				//	interested flags for us are set, thus record this statics
				point.m_tileBlock |= wBlockThis & (CAN_I_BLOCK|CAN_I_FIRE|CAN_I_WATER);
			}
		}
	}

	if ( point.m_baseZ == 127 )
		point.m_baseZ = mapDefaultHeight;

	fwrite(&point, sizeof(point), 1, m_hFile);

*/