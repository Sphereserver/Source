//
// CSector.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "graysvr.h"	// predef header.

//////////////////////////////////////////////////////////////////
// -CSector

int CSectorBase::m_iMapBlockCacheTime = 0;

CSector::CSector()
{
	m_ListenItems = 0;

	m_RainChance = 0;		// 0 to 100%
	m_ColdChance = 0;		// Will be snow if rain chance success.
	SetDefaultWeatherChance();

	m_fSaveParity = false;
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
	SC_LOCALTIME,
	SC_LOCALTOD,
	SC_RAINCHANCE,
	SC_SEASON,
	SC_WEATHER,
	SC_QTY
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
	"LOCALTIME",
	"LOCALTOD",
	"RAINCHANCE",
	"SEASON",
	"WEATHER",
	NULL
};

bool CSector::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CSector::r_WriteVal");
	EXC_TRY("WriteVal");

	static const CValStr sm_ComplexityTitles[] =
	{
		{ "HIGH", INT_MIN },	// speech can be very complex if low char count
		{ "MEDIUM", 5 },
		{ "LOW", 10 },
		{ NULL, INT_MAX }
	};

	switch ( FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case SC_CLIENTS:
			sVal.FormatVal(m_Chars_Active.HasClients());
			return true;
		case SC_COLDCHANCE:
			sVal.FormatVal( GetColdChance());
			return( true );
		case SC_COMPLEXITY:
			if ( pszKey[10] == '.' )
			{
				pszKey += 11;
				sVal = ( ! strcmpi( pszKey, sm_ComplexityTitles->FindName( GetCharComplexity()))) ? "1" : "0";
				return( true );
			}
			sVal.FormatVal( GetCharComplexity());
			return( true );
		case SC_LIGHT:
			sVal.FormatVal(GetLight());
			return true;
		case SC_LOCALTIME:
			sVal = GetLocalGameTime();
			return( true );
		case SC_LOCALTOD:
			sVal.FormatVal( GetLocalTime());
			return( true );
		case SC_ISDARK:
			sVal.FormatVal( IsDark() );
			return( true );
		case SC_ISNIGHTTIME:
			{
				int iMinutes = GetLocalTime();
				sVal = ( iMinutes < 7*60 || iMinutes > (9+12)*60 ) ? "1" : "0";
			}
			return( true );
		case SC_RAINCHANCE:
			sVal.FormatVal( GetRainChance());
			return( true );
		case SC_ITEMCOUNT:
			sVal.FormatVal(GetItemComplexity());
			return true;
		case SC_SEASON:
			sVal.FormatVal(static_cast<int>(GetSeason()));
			return true;
		case SC_WEATHER:
			sVal.FormatVal(static_cast<int>(GetWeather()));
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
	ADDTOCALLSTACK("CSector::r_LoadVal");
	EXC_TRY("LoadVal");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case SC_COLDCHANCE:
			SetWeatherChance( false, s.HasArgs() ? s.GetArgVal() : -1 );
			return( true );
		case SC_LIGHT:
			if ( g_Cfg.m_bAllowLightOverride )
				m_Env.m_Light = s.GetArgVal() | LIGHT_OVERRIDE;
			return true;
		case SC_RAINCHANCE:
			SetWeatherChance( true, s.HasArgs() ? s.GetArgVal() : -1 );
			return( true );
		case SC_SEASON:
			SetSeason(s.HasArgs() ? static_cast<SEASON_TYPE>(s.GetArgVal()) : SEASON_Summer);
			return (true);
		case SC_WEATHER:
			SetWeather(s.HasArgs() ? static_cast<WEATHER_TYPE>(s.GetArgVal()) : WEATHER_DRY);
			return( true );
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
	#include "../tables/CSector_functions.tbl"
	#undef ADD
	SEV_QTY
};

LPCTSTR const CSector::sm_szVerbKeys[SEV_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CSector_functions.tbl"
	#undef ADD
	NULL
};

bool CSector::r_Verb( CScript & s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CSector::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);
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
			SetWeather(s.HasArgs() ? static_cast<WEATHER_TYPE>(s.GetArgVal()) : WEATHER_RAIN);
			break;
		case SEV_RESPAWN:
			( toupper( s.GetArgRaw()[0] ) == 'A' ) ? g_World.RespawnDeadNPCs() : RespawnDeadNPCs();
			break;
		case SEV_RESTOCK:	// x
			// set restock time of all vendors in World, set the respawn time of all spawns in World.
			( toupper( s.GetArgRaw()[0] ) == 'A' ) ? g_World.Restock() : Restock();
			break;
		case SEV_SEASON:
			SetSeason(static_cast<SEASON_TYPE>(s.GetArgVal()));
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
	ADDTOCALLSTACK_INTENSIVE("CSector::r_Write");
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
	CChar * pChar = STATIC_CAST <CChar*>( m_Chars_Active.GetHead());
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
	pChar = STATIC_CAST <CChar*> (m_Chars_Disconnect.GetHead());
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
	CItem * pItem = STATIC_CAST <CItem*> (m_Items_Inert.GetHead());
	for ( ; pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( pItem->IsType(IT_MULTI_CUSTOM) )
			pItem->r_WriteSafe(g_World.m_FileMultis);
#ifdef _ALPHASPHERE
		else if ( !pItem->IsAttr(ATTR_STATIC) || IsSetEF(EF_Specific) )
#else
		else if ( !pItem->IsAttr(ATTR_STATIC) )
#endif
			pItem->r_WriteSafe(g_World.m_FileWorld);
	}
	pItem = STATIC_CAST <CItem*> (m_Items_Timer.GetHead());
	for ( ; pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( pItem->IsType(IT_MULTI_CUSTOM) )
			pItem->r_WriteSafe(g_World.m_FileMultis);
#ifdef _ALPHASPHERE
		else if ( !pItem->IsAttr(ATTR_STATIC) || IsSetEF(EF_Specific) )
#else
		else if ( !pItem->IsAttr(ATTR_STATIC) )
#endif
			pItem->r_WriteSafe(g_World.m_FileWorld);
	}
}

bool CSector::v_AllChars( CScript & s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CSector::v_AllChars");
	CScript	script(s.GetArgStr());
	CChar * pChar = NULL;
	bool fRet = false;
	
	// Loop through all the characters in m_Chars_Active.
	// We should start at the end incase some are removed during the loop.
	size_t i = m_Chars_Active.GetCount();
	while ( i > 0 )
	{
		pChar = STATIC_CAST <CChar*>(m_Chars_Active.GetAt(--i));

		// Check that a character was returned and keep looking if not.
		if (pChar == NULL)
			continue;

		// Execute the verb on the character
		fRet |= pChar->r_Verb(script, pSrc);
	}
	return fRet;
}

bool CSector::v_AllCharsIdle( CScript & s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CSector::v_AllCharsIdle");
	CScript	script(s.GetArgStr());
	CChar * pChar = NULL;
	bool fRet = false;
	
	// Loop through all the characters in m_Chars_Disconnect.
	// We should start at the end incase some are removed during the loop.
	size_t i = m_Chars_Disconnect.GetCount();
	while ( i > 0 )
	{
		pChar = STATIC_CAST <CChar*>(m_Chars_Disconnect.GetAt(--i));

		// Check that a character was returned and keep looking if not.
		if (pChar == NULL)
			continue;

		// Execute the verb on the character
		fRet |= pChar->r_Verb(script, pSrc);
	}
	return fRet;
}

bool CSector::v_AllItems( CScript & s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CSector::v_AllItems");
	CScript	script(s.GetArgStr());
	CItem * pItem = NULL;
	bool fRet = false;

	// Loop through all the items in m_Items_Timer.
	// We should start at the end incase items are removed during the loop.
	size_t i = m_Items_Timer.GetCount();
	while ( i > 0 )
	{
		// Get the next item
		pItem = STATIC_CAST <CItem*>(m_Items_Timer.GetAt(--i));

		// Check that an item was returned and keep looking if not.
		if (pItem == NULL)
			continue;

		// Execute the verb on the item
		fRet |= pItem->r_Verb(script, pSrc);
	}
	
	// Loop through all the items in m_Items_Inert.
	// We should start at the end incase items are removed during the loop.
	i = m_Items_Inert.GetCount();
	while ( i > 0 )
	{
		// Get the next item.
		pItem = STATIC_CAST <CItem*>(m_Items_Inert.GetAt(--i));

		// Check that an item was returned and keep looking if not.
		if (pItem == NULL)
			continue;

		// Execute the verb on the item
		fRet |= pItem->r_Verb(script, pSrc);
	}
	return fRet;
}

bool CSector::v_AllClients( CScript & s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CSector::v_AllClients");
	CScript script(s.GetArgStr());
	CChar * pChar = NULL;
	bool fRet = false;

	// Loop through all the characters in m_Chars_Active.
	// We should start at the end incase some are removed during the loop.
	size_t i = m_Chars_Active.GetCount();
	while ( i > 0 )
	{
		pChar = STATIC_CAST <CChar*>(m_Chars_Active.GetAt(--i));

		// Check that a character was returned and keep looking if not.
		if (pChar == NULL)
			continue;

		// Check that the character is a client (we only want to affect
		// clients with this)
		if ( ! pChar->IsClient())
			continue;

		// Execute the verb on the client
		fRet |= pChar->r_Verb(script, pSrc);
	}
	return fRet;
}

int CSector::GetLocalTime() const
{
	ADDTOCALLSTACK("CSector::GetLocalTime");
	//	Get local time of the day (in minutes)
	CPointMap pt = GetBasePoint();
	int iLocalTime = g_World.GetGameWorldTime();

	if ( !g_Cfg.m_bAllowLightOverride )
	{
		iLocalTime += ( pt.m_x * 24*60 ) / g_MapList.GetX(pt.m_map);
	}
	else
	{
		// Time difference between adjacent sectors in minutes
		int iSectorTimeDiff = (24*60) / g_MapList.GetSectorCols(pt.m_map);

		// Calculate the # of columns between here and Castle Britannia ( x = 1400 )
		//int iSectorOffset = ( pt.m_x / g_MapList.GetX(pt.m_map) ) - ( (24*60) / g_MapList.GetSectorSize(pt.m_map));
		int iSectorOffset = ( pt.m_x / g_MapList.GetSectorSize(pt.m_map));

		// Calculate the time offset from global time
		int iTimeOffset = iSectorOffset * iSectorTimeDiff;

		// Calculate the local time
		iLocalTime += iTimeOffset;
	}
	return (iLocalTime % (24*60));
}

LPCTSTR CSector::GetLocalGameTime() const
{
	ADDTOCALLSTACK("CSector::GetLocalGameTime");
	return( GetTimeMinDesc( GetLocalTime()));
}

bool CSector::IsMoonVisible(unsigned int iPhase, int iLocalTime) const
{
	ADDTOCALLSTACK("CSector::IsMoonVisible");
	// When is moon rise and moon set ?
	iLocalTime %= (24*60);		// just to make sure (day time in minutes) 1440
	switch (iPhase)
	{
		case 0:	// new moon
			return( (iLocalTime > 360) && (iLocalTime < 1080));
		case 1:	// waxing crescent
			return( (iLocalTime > 540) && (iLocalTime < 1270));
		case 2:	// first quarter
			return( iLocalTime > 720 );
		case 3:	// waxing gibbous
			return( (iLocalTime < 180) || (iLocalTime > 900));
		case 4:	// full moon
			return( (iLocalTime < 360) || (iLocalTime > 1080));
		case 5:	// waning gibbous
			return( (iLocalTime < 540) || (iLocalTime > 1270));
		case 6:	// third quarter
			return( iLocalTime < 720 );
		case 7:	// waning crescent
			return( (iLocalTime > 180) && (iLocalTime < 900));
		default: // How'd we get here?
			return false;
	}
}

BYTE CSector::GetLightCalc( bool fQuickSet ) const
{
	ADDTOCALLSTACK("CSector::GetLightCalc");
	// What is the light level default here in this sector.

	if ( g_Cfg.m_bAllowLightOverride && IsLightOverriden() )
		return m_Env.m_Light;

	if ( IsInDungeon() )
		return g_Cfg.m_iLightDungeon;

	int localtime = GetLocalTime();

	if ( !g_Cfg.m_bAllowLightOverride )
	{
		//	Normalize time:
		//	convert	0=midnight	.. (23*60)+59=midnight
		//	to		0=day		.. 12*60=night
		if ( localtime < 12*60 )
			localtime = 12*60 - localtime;
		else
			localtime -= 12*60;

		//	0...	y	...lightnight
		//	0...	x	...12*60
		int iTargLight = ((localtime * ( g_Cfg.m_iLightNight - g_Cfg.m_iLightDay ))/(12*60) + g_Cfg.m_iLightDay);

		if ( iTargLight < LIGHT_BRIGHT ) iTargLight = LIGHT_BRIGHT;
		if ( iTargLight > LIGHT_DARK ) iTargLight = LIGHT_DARK;

		return iTargLight;
	}

	int hour = ( localtime / ( 60)) % 24;
	bool fNight = ( hour < 6 || hour > 12+8 );	// Is it night or day ?
	int iTargLight = (fNight) ? g_Cfg.m_iLightNight : g_Cfg.m_iLightDay;	// Target light level.

	// Check for clouds...if it is cloudy, then we don't even need to check for the effects of the moons...
	if ( GetWeather())
	{
		// Clouds of some sort...
		if (fNight)
			iTargLight += ( Calc_GetRandVal( 2 ) + 1 );	// 1-2 light levels darker if cloudy at night
		else
			iTargLight += ( Calc_GetRandVal( 4 ) + 1 );	// 1-4 light levels darker if cloudy during the day.
	}

	if ( fNight )
	{
		// Factor in the effects of the moons
		// Trammel
		unsigned int iTrammelPhase = g_World.GetMoonPhase( false );
		// Check to see if Trammel is up here...

		if ( IsMoonVisible( iTrammelPhase, localtime ))
		{
static const BYTE sm_TrammelPhaseBrightness[] =
{
	0, // New Moon
	TRAMMEL_FULL_BRIGHTNESS / 4,	// Crescent Moon
	TRAMMEL_FULL_BRIGHTNESS / 2, 	// Quarter Moon
	( TRAMMEL_FULL_BRIGHTNESS * 3) / 4, // Gibbous Moon
	TRAMMEL_FULL_BRIGHTNESS,		// Full Moon
	( TRAMMEL_FULL_BRIGHTNESS * 3) / 4, // Gibbous Moon
	TRAMMEL_FULL_BRIGHTNESS / 2, 	// Quarter Moon
	TRAMMEL_FULL_BRIGHTNESS / 4		// Crescent Moon
};
			ASSERT( iTrammelPhase < COUNTOF(sm_TrammelPhaseBrightness));
			iTargLight -= sm_TrammelPhaseBrightness[iTrammelPhase];
		}

		// Felucca
		unsigned int iFeluccaPhase = g_World.GetMoonPhase( true );
		if ( IsMoonVisible( iFeluccaPhase, localtime ))
		{
static const BYTE sm_FeluccaPhaseBrightness[] =
{
	0, // New Moon
	FELUCCA_FULL_BRIGHTNESS / 4,	// Crescent Moon
	FELUCCA_FULL_BRIGHTNESS / 2, 	// Quarter Moon
	( FELUCCA_FULL_BRIGHTNESS * 3) / 4, // Gibbous Moon
	FELUCCA_FULL_BRIGHTNESS,		// Full Moon
	( FELUCCA_FULL_BRIGHTNESS * 3) / 4, // Gibbous Moon
	FELUCCA_FULL_BRIGHTNESS / 2, 	// Quarter Moon
	FELUCCA_FULL_BRIGHTNESS / 4		// Crescent Moon
};
			ASSERT( iFeluccaPhase < COUNTOF(sm_FeluccaPhaseBrightness));
			iTargLight -= sm_FeluccaPhaseBrightness[iFeluccaPhase];
		}
	}

	if ( iTargLight < LIGHT_BRIGHT ) iTargLight = LIGHT_BRIGHT;
	if ( iTargLight > LIGHT_DARK ) iTargLight = LIGHT_DARK;

	if ( fQuickSet || m_Env.m_Light == iTargLight )		// Initializing the sector
		return iTargLight;

	// Gradual transition to global light level.
	if ( m_Env.m_Light > iTargLight )
		return( m_Env.m_Light - 1 );
	else
		return( m_Env.m_Light + 1 );
}

void CSector::SetLightNow( bool fFlash )
{
	ADDTOCALLSTACK("CSector::SetLightNow");
	// Set the light level for all the CClients here.

	CChar * pChar = STATIC_CAST <CChar*>( m_Chars_Active.GetHead());
	for ( ; pChar != NULL; pChar = pChar->GetNext())
	{
		if ( pChar->IsStatFlag( STATF_DEAD | STATF_NightSight ))
			continue;

		if ( pChar->IsClient())
		{
			CClient * pClient = pChar->GetClient();
			ASSERT(pClient);

			if ( fFlash )	// This does not seem to work predicably !
			{
				BYTE bPrvLight = m_Env.m_Light;
				m_Env.m_Light = LIGHT_BRIGHT;	// full bright.
				pClient->addLight();
				m_Env.m_Light = bPrvLight;	// back to previous.
			}
			pClient->addLight();
		}

		// don't fire trigger when server is loading or light is flashing
		if ( ! g_Serv.IsLoading() && fFlash == false )
		{
			pChar->OnTrigger( CTRIG_EnvironChange, pChar );
		}
	}
}

void CSector::SetLight( int light )
{
	ADDTOCALLSTACK("CSector::SetLight");
	// GM set light level command
	// light =LIGHT_BRIGHT, LIGHT_DARK=dark

	if ( light < LIGHT_BRIGHT || light > LIGHT_DARK )
	{
		m_Env.m_Light &= ~LIGHT_OVERRIDE;
		m_Env.m_Light = (BYTE) GetLightCalc( true );
	}
	else
	{
		m_Env.m_Light = (BYTE) ( light | LIGHT_OVERRIDE );
	}
	SetLightNow(false);
}

void CSector::SetDefaultWeatherChance()
{
	ADDTOCALLSTACK("CSector::SetDefaultWeatherChance");
	CPointMap pt = GetBasePoint();
	int iPercent = IMULDIV( pt.m_y, 100, g_MapList.GetY(pt.m_map) );	// 100 = south
	if ( iPercent < 50 )
	{
		// Anywhere north of the Britain Moongate is a good candidate for snow
		m_ColdChance = 1 + ( 49 - iPercent ) * 2;
		m_RainChance = 15;
	}
	else
	{
		// warmer down south
		m_ColdChance = 1;
		// rain more likely down south.
		m_RainChance = 15 + ( iPercent - 50 ) / 10;
	}
}

WEATHER_TYPE CSector::GetWeatherCalc() const
{
	ADDTOCALLSTACK("CSector::GetWeatherCalc");
	// (1 in x) chance of some kind of weather change at any given time
	if ( IsInDungeon() || g_Cfg.m_fNoWeather )
		return( WEATHER_DRY );

	int iRainRoll = Calc_GetRandVal( 100 );
	if ( ( GetRainChance() * 2) < iRainRoll )
		return( WEATHER_DRY );

	// Is it just cloudy?
	if ( iRainRoll > GetRainChance())
		return WEATHER_CLOUDY;

	// Rain chance also controls the chance of snow. If it isn't possible to rain then it cannot snow either
	if ( !GetRainChance() )
		return WEATHER_DRY;

	// Is it snowing?
	if ( GetColdChance() && Calc_GetRandVal(100) <= GetColdChance()) // Can it actually snow here?
		return WEATHER_SNOW;

	// It is raining
	return WEATHER_RAIN;
}

void CSector::SetWeather( WEATHER_TYPE w )
{
	ADDTOCALLSTACK("CSector::SetWeather");
	// Set the immediate weather type.
	// 0=dry, 1=rain or 2=snow.

	if ( w == m_Env.m_Weather )
		return;

	m_Env.m_Weather = w;

	CChar * pChar = STATIC_CAST <CChar*>( m_Chars_Active.GetHead());
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
	ADDTOCALLSTACK("CSector::SetSeason");
	// Set the season type.

	if ( season == m_Env.m_Season )
		return;

	m_Env.m_Season = season;

	CChar * pChar = STATIC_CAST <CChar*>( m_Chars_Active.GetHead());
	for ( ; pChar != NULL; pChar = pChar->GetNext())
	{
		if ( pChar->IsClient() )
			pChar->GetClient()->addSeason(season);
		pChar->OnTrigger(CTRIG_EnvironChange, pChar);
	}
}

void CSector::SetWeatherChance( bool fRain, int iChance )
{
	ADDTOCALLSTACK("CSector::SetWeatherChance");
	// Set via the client.
	// Transfer from snow to rain does not work ! must be DRY first.

	if ( iChance > 100 )
		iChance = 100;
	if ( iChance < 0 )
	{
		// just set back to defaults.
		SetDefaultWeatherChance();
	}
	else if ( fRain )
	{
		m_RainChance = iChance | LIGHT_OVERRIDE;
	}
	else
	{
		m_ColdChance = iChance | LIGHT_OVERRIDE;
	}

	// Recalc the weather immediatly.
	SetWeather( GetWeatherCalc());
}

void CSector::OnHearItem( CChar * pChar, TCHAR * szText )
{
	ADDTOCALLSTACK("CSector::OnHearItem");
	// report to any of the items that something was said.

	ASSERT(m_ListenItems);

	CItem * pItemNext;
	CItem * pItem = STATIC_CAST <CItem*>( m_Items_Timer.GetHead());
	for ( ; pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		pItem->OnHear( szText, pChar );
	}
	pItem = STATIC_CAST <CItem*>( m_Items_Inert.GetHead());
	for ( ; pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		pItem->OnHear( szText, pChar );
	}
}

void CSector::MoveItemToSector( CItem * pItem, bool fActive )
{
	ADDTOCALLSTACK("CSector::MoveItemToSector");
	// remove from previous list and put in new.
	// May just be setting a timer. SetTimer or MoveTo()
	ASSERT( pItem );
	if ( fActive )
	{
		m_Items_Timer.AddItemToSector( pItem );
	}
	else
	{
		m_Items_Inert.AddItemToSector( pItem );
	}
}

bool CSector::MoveCharToSector( CChar * pChar )
{
	ADDTOCALLSTACK("CSector::MoveCharToSector");
	// Move a CChar into this CSector.
	// NOTE:
	//   m_pt is still the old location. Not moved yet!
	// ASSUME: called from CChar.MoveToChar() assume ACtive char.

	if ( IsCharActiveIn( pChar ))
		return false;	// already here

	// Check my save parity vs. this sector's
	if ( pChar->IsStatFlag( STATF_SaveParity ) != m_fSaveParity )
	{
		if ( g_World.IsSaving())
		{
			if ( m_fSaveParity == g_World.m_fSaveParity )
			{
				// Save out the CChar now. the sector has already been saved.
				if ( pChar->m_pPlayer )
					pChar->r_WriteParity(g_World.m_FilePlayers);
				else
					pChar->r_WriteParity(g_World.m_FileWorld);
			}
			else
			{
				// We need to write out this CSector now. (before adding client)
				r_Write();
			}
		}
	}

	if ( pChar->IsClient())
	{
		// Send new weather and light for this sector
		// Only send if different than last ???

		CClient * pClient = pChar->GetClient();
		if ( ! pChar->IsStatFlag( STATF_DEAD | STATF_NightSight | STATF_Sleeping ))
		{
			pClient->addLight(GetLight());
		}
		// Provide the weather as an arg as we are not in the new location yet.
		pClient->addWeather(GetWeather());
		pClient->addSeason(GetSeason());
	}

	m_Chars_Active.AddCharToSector( pChar );	// remove from previous spot.
	return( true );
}

inline bool CSector::IsSectorSleeping() const
{
	ADDTOCALLSTACK_INTENSIVE("CSector::IsSectorSleeping");
	long iAge = - g_World.GetTimeDiff( GetLastClientTime());
	return( iAge > 10*60*TICK_PER_SEC );
}

void CSector::SetSectorWakeStatus()
{
	ADDTOCALLSTACK("CSector::SetSectorWakeStatus");
	// Ships may enter a sector before it's riders ! ships need working timers to move !
	m_Chars_Active.m_timeLastClient = CServTime::GetCurrentTime();
}

void CSector::Close()
{
	ADDTOCALLSTACK("CSector::Close");
	// Clear up all dynamic data for this sector.
	m_Items_Timer.DeleteAll();
	m_Items_Inert.DeleteAll();
	m_Chars_Active.DeleteAll();
	m_Chars_Disconnect.DeleteAll();

	// These are resource type things.
	// m_Teleports.RemoveAll();
	// m_RegionLinks.Empty();	// clear the link list.
}

void CSector::RespawnDeadNPCs()
{
	ADDTOCALLSTACK("CSector::RespawnDeadNPCs");
	// skip sectors in unsupported maps
	if ( !g_MapList.m_maps[m_map] ) return;

	// Respawn dead NPC's
	CChar * pCharNext;
	CChar * pChar = STATIC_CAST <CChar *>( m_Chars_Disconnect.GetHead());
	for ( ; pChar != NULL; pChar = pCharNext )
	{
		pCharNext = pChar->GetNext();
		if ( ! pChar->m_pNPC )
			continue;
		if ( ! pChar->m_ptHome.IsValidPoint())
			continue;
		if ( ! pChar->IsStatFlag( STATF_DEAD ))
			continue;

		// Res them back to their "home".
		int iDist = pChar->m_pNPC->m_Home_Dist_Wander;
		pChar->MoveNear( pChar->m_ptHome, ( iDist < SHRT_MAX ) ? iDist : 4 );
		pChar->Spell_Resurrection();

		// Restock them with npc stuff.
		pChar->NPC_LoadScript(true);
	}
}

void CSector::Restock()
{
	ADDTOCALLSTACK("CSector::Restock");
	// ARGS: iTime = time in seconds
	// set restock time of all vendors in Sector.
	// set the respawn time of all spawns in Sector.

	CChar * pCharNext;
	CChar * pChar = dynamic_cast <CChar*>( m_Chars_Active.GetHead());
	for ( ; pChar; pChar = pCharNext )
	{
		pCharNext = pChar->GetNext();
		pChar->NPC_Vendor_Restock(true);
	}

	CItem * pItemNext;
	CItem * pItem = dynamic_cast <CItem*>( m_Items_Timer.GetHead());
	for ( ; pItem; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( pItem->IsType(IT_SPAWN_ITEM) || pItem->IsType(IT_SPAWN_CHAR) )
			pItem->Spawn_OnTick( true );
	}
}

void CSector::OnTick(int iPulseCount)
{
	ADDTOCALLSTACK_INTENSIVE("CSector::OnTick");
	// CWorld gives OnTick() to all CSectors.
	TIME_PROFILE_INIT;
	if ( IsSetSpecific )
		TIME_PROFILE_START;

	EXC_TRY("Tick");
	EXC_SET("light change");

	//	do not tick sectors on maps not supported by server
	if ( !g_MapList.m_maps[m_map] ) return;

	// Check for light change before putting the sector to sleep, since in other case the
	// world light levels will be shitty
	bool fEnvironChange = false;
	bool fLightChange = false;
	bool fSleeping = false;

	if ( ! ( iPulseCount & 0x7f ))	// 30 seconds or so.
	{
		// check for local light level change ?
		BYTE blightprv = m_Env.m_Light;
		m_Env.m_Light = GetLightCalc( false );
		if ( m_Env.m_Light != blightprv )
		{
			fEnvironChange = true;
			fLightChange = true;
		}
	}

	EXC_SET("sector sleeping?");
	size_t clients = m_Chars_Active.HasClients();

	if ( clients <= 0 ) // having no clients inside
	{
		// Put the sector to sleep if no clients been here in a while.
		fSleeping = IsSectorSleeping();
		if ( fSleeping )
		{
			if ( !g_Cfg.m_iSectorSleepMask )
				return;
			if (( iPulseCount & g_Cfg.m_iSectorSleepMask ) != ( GetIndex() & g_Cfg.m_iSectorSleepMask ))
				return;
		}
	}

	EXC_SET("sound effects");
	// random weather noises and effects.
	SOUND_TYPE sound = 0;
	bool fWeatherChange = false;
	int iRegionPeriodic = 0;

	if ( ! ( iPulseCount & 0x7f ))	// 30 seconds or so.
	{
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

		// Random area noises. Only do if clients about.
		if ( clients > 0 )
		{
			iRegionPeriodic = 2;

			static const SOUND_TYPE sm_SfxRain[] = { 0x10, 0x11 };
			static const SOUND_TYPE sm_SfxWind[] = { 0x14, 0x15, 0x16 };
			static const SOUND_TYPE sm_SfxThunder[] = { 0x28, 0x29 , 0x206 };

			// Lightning ?	// wind, rain,
			switch ( GetWeather() )
			{
				case WEATHER_CLOUDY:
					break;

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

				default:
					break;
			}
		}
	}

	// regen all creatures and do AI

	ProfileTask charactersTask(PROFILE_CHARS);

	//pChar = STATIC_CAST <CChar*>( m_Chars_Active.GetHead());
	CChar * pCharNext = NULL;
	CChar * pChar = dynamic_cast <CChar*>( m_Chars_Active.GetHead());
	for ( ; pChar != NULL; pChar = pCharNext )
	{
		EXC_TRYSUB("TickChar");

		pCharNext = pChar->GetNext();
		if ( fEnvironChange )
			pChar->OnTrigger(CTRIG_EnvironChange, pChar);

		if ( pChar->IsClient())
		{
			CClient * pClient = pChar->GetClient();
			ASSERT( pClient );
			if ( sound )
				pClient->addSound(sound, pChar);

			if ( fLightChange && ! pChar->IsStatFlag( STATF_DEAD | STATF_NightSight ))
				pClient->addLight();

			if ( fWeatherChange )
				pClient->addWeather(GetWeather());

			if ( !IsSetEF(EF_Minimize_Triggers) && ( iRegionPeriodic && pChar->m_pArea ))
			{
				if ( iRegionPeriodic == 2 )
				{
					pChar->m_pArea->OnRegionTrigger( pChar, RTRIG_REGPERIODIC );
					iRegionPeriodic--;
				}
				pChar->m_pArea->OnRegionTrigger( pChar, RTRIG_CLIPERIODIC );
			}
		}
		// Can only die on your own tick.
		if ( !pChar->OnTick() )
			pChar->Delete();

		EXC_CATCHSUB("Sector");

		EXC_DEBUGSUB_START;
		CPointMap pt = GetBasePoint();
		g_Log.EventDebug("char 0%lx '%s'\n", static_cast<DWORD>(pChar->GetUID()), pChar->GetName());
		g_Log.EventDebug("sector #%d [%d,%d,%d,%d]\n", GetIndex(),  pt.m_x, pt.m_y, pt.m_z, pt.m_map);
		EXC_DEBUGSUB_END;
	}

	// decay items on ground = time out spells / gates etc.. etc..
	// No need to check these so often !

	ProfileTask itemsTask(PROFILE_ITEMS);

	CItem * pItemNext = NULL;
	CItem * pItem = dynamic_cast <CItem*>( m_Items_Timer.GetHead());
	for ( ; pItem != NULL; pItem = pItemNext )
	{
		EXC_TRYSUB("TickItem");
		pItemNext = pItem->GetNext();

		EXC_SETSUB("TimerExpired");
		if ( pItem->IsTimerExpired() )
		{
			EXC_SETSUB("ItemTick");
			if ( !pItem->OnTick() )
			{
				EXC_SETSUB("ItemDelete");
				pItem->Delete();
			}
			else
			{
				EXC_SETSUB("TimerExpired2");
				if ( pItem->IsTimerExpired() )	// forgot to clear the timer.? strange.
				{
					EXC_SETSUB("SetTimeout");
					pItem->SetTimeout(-1);
				}
			}
		}

		EXC_SETSUB("UpdateFlags");
		pItem->OnTickStatusUpdate();

#ifdef _WIN32
		EXC_CATCHSUB("Sector");

		EXC_DEBUGSUB_START;
		CPointMap pt = GetBasePoint();
		g_Log.EventError("item 0%lx '%s' [timer=%d, type=%d]\n", static_cast<DWORD>(pItem->GetUID()), pItem->GetName(), pItem->GetTimerAdjusted(), static_cast<int>(pItem->GetType()));
		g_Log.EventError("sector #%d [%d,%d,%d,%d]\n", GetIndex(),  pt.m_x, pt.m_y, pt.m_z, pt.m_map);
		
		EXC_DEBUGSUB_END;
#else
		}
#ifndef _DEBUG
		catch ( const CGrayError& e )
		{
			PAUSECALLSTACK;
			CPointMap pt = GetBasePoint();
			g_Log.EventError("CGrayError: item 0%lx '%s' [timer=%d, type=%d]\n", static_cast<DWORD>(pItem->GetUID()), pItem->GetName(), pItem->GetTimerAdjusted(), static_cast<int>(pItem->GetType()));
			g_Log.EventError("sector #%d [%d,%d,%d,%d]\n", GetIndex(),  pt.m_x, pt.m_y, pt.m_z, pt.m_map);
			UNPAUSECALLSTACK;
			EXC_CATCH_SUB(&e, "Sector");
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
		catch (...)
		{
			CPointMap pt = GetBasePoint();
			g_Log.EventError("...: item 0%lx '%s' [timer=%d, type=%d]\n", static_cast<DWORD>(pItem->GetUID()), pItem->GetName(), pItem->GetTimerAdjusted(), static_cast<int>(pItem->GetType()));\
			g_Log.EventError("sector #%d [%d,%d,%d,%d]\n", GetIndex(),  pt.m_x, pt.m_y, pt.m_z, pt.m_map);
			EXC_CATCH_SUB(NULL, "Sector");
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
#endif
#endif
	}

	ProfileTask overheadTask(PROFILE_OVERHEAD);

	EXC_SET("check map cache");
	if ( fSleeping || ! ( iPulseCount & 0x7f ))	// 30 seconds or so.
	{
		// delete the static CGrayMapBlock items that have not been used recently.
		m_iMapBlockCacheTime = ( fSleeping ? 0 : g_Cfg.m_iMapCacheTime );
		CheckMapBlockCache();
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	CPointMap pt = GetBasePoint();
	g_Log.EventError("sector #%d [%d,%d,%d,%d]\n", GetIndex(),  pt.m_x, pt.m_y, pt.m_z, pt.m_map);
	EXC_DEBUG_END;

	if ( IsSetSpecific )
	{
		TIME_PROFILE_END;
		LONGLONG hi = TIME_PROFILE_GET_HI;
		if ( hi > 1L )
		{
			DEBUG_ERR(("CSector::OnTick(%d) [ticking sector] took %lld.%lld to run\n", GetIndex(), static_cast<INT64>(hi), static_cast<INT64>(TIME_PROFILE_GET_LO)));
		}
	}
}

