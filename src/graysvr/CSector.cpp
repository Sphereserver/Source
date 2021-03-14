#include "graysvr.h"	// predef header

int CSectorBase::m_iMapBlockCacheTime = 0;

CSector::CSector()
{
	m_bFlags = 0;

	m_ListenItems = 0;
	m_fSaveParity = false;
	SetDefaultWeatherChance();
}

enum SC_TYPE
{
	SC_CLIENTS,
	SC_COLDCHANCE,
	SC_COMPLEXITY,
	SC_FLAGS,
	SC_ISDARK,
	SC_ISNIGHTTIME,
	SC_ITEMCOUNT,
	SC_LIGHT,
	SC_LOCALTIME,
	SC_LOCALTOD,
	SC_NUMBER,
	SC_RAINCHANCE,
	SC_SEASON,
	SC_WEATHER,
	SC_QTY
};

const LPCTSTR CSector::sm_szLoadKeys[SC_QTY + 1] =
{
	"CLIENTS",
	"COLDCHANCE",
	"COMPLEXITY",
	"FLAGS",
	"ISDARK",
	"ISNIGHTTIME",
	"ITEMCOUNT",
	"LIGHT",
	"LOCALTIME",
	"LOCALTOD",
	"NUMBER",
	"RAINCHANCE",
	"SEASON",
	"WEATHER",
	NULL
};

bool CSector::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CSector::r_WriteVal");
	EXC_TRY("WriteVal");

	switch ( FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case SC_CLIENTS:
			sVal.FormatVal(m_Chars_Active.HasClients());
			return true;
		case SC_COLDCHANCE:
			sVal.FormatVal(GetColdChance());
			return true;
		case SC_COMPLEXITY:
		{
			if ( pszKey[10] == '.' )
			{
				pszKey += 11;
				static const CValStr sm_ComplexityTitles[] =
				{
					{ "HIGH",	INT_MIN },	// speech can be very complex if low char count
					{ "MEDIUM",	5 },
					{ "LOW",	10 },
					{ NULL,		INT_MAX }
				};
				sVal = !strcmpi(pszKey, sm_ComplexityTitles->FindName(GetCharComplexity())) ? "1" : "0";
				return true;
			}
			sVal.FormatVal(GetCharComplexity());
			return true;
		}
		case SC_FLAGS:
			sVal.FormatHex(m_bFlags);
			return true;
		case SC_ISDARK:
			sVal.FormatVal(IsDark());
			return true;
		case SC_ISNIGHTTIME:
			sVal.FormatVal(IsNight());
			return true;
		case SC_ITEMCOUNT:
			sVal.FormatUVal(GetItemComplexity());
			return true;
		case SC_LIGHT:
			sVal.FormatUVal(GetLight());
			return true;
		case SC_LOCALTIME:
			sVal = GetLocalGameTime();
			return true;
		case SC_LOCALTOD:
			sVal.FormatVal(GetLocalTime());
			return true;
		case SC_NUMBER:
			sVal.FormatVal(m_index);
			return true;
		case SC_RAINCHANCE:
			sVal.FormatUVal(GetRainChance());
			return true;
		case SC_SEASON:
			sVal.FormatUVal(GetSeason());
			return true;
		case SC_WEATHER:
			sVal.FormatUVal(GetWeather());
			return true;
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CSector::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CSector::r_LoadVal");
	EXC_TRY("LoadVal");
	switch ( FindTableSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case SC_COLDCHANCE:
			SetWeatherChance(false, s.HasArgs() ? s.GetArgVal() : -1);
			return true;
		case SC_FLAGS:
			m_bFlags = static_cast<BYTE>(s.GetArgVal());
			return true;
		case SC_LIGHT:
			if ( g_Cfg.m_bAllowLightOverride )
				m_Env.m_Light = static_cast<BYTE>(s.GetArgVal() | LIGHT_OVERRIDE);
			return true;
		case SC_RAINCHANCE:
			SetWeatherChance(true, s.HasArgs() ? s.GetArgVal() : -1);
			return true;
		case SC_SEASON:
			SetSeason(s.HasArgs() ? static_cast<SEASON_TYPE>(s.GetArgVal()) : SEASON_Summer);
			return true;
		case SC_WEATHER:
			SetWeather(s.HasArgs() ? static_cast<WEATHER_TYPE>(s.GetArgVal()) : WEATHER_Clear);
			return true;
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

const LPCTSTR CSector::sm_szVerbKeys[SEV_QTY + 1] =
{
	#define ADD(a,b) b,
	#include "../tables/CSector_functions.tbl"
	#undef ADD
	NULL
};

bool CSector::r_Verb(CScript &s, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CSector::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);
	int index = FindTableSorted(s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1);
	switch ( index )
	{
		case SEV_ALLCHARS:
			v_AllChars(s, pSrc);
			break;
		case SEV_ALLCHARSIDLE:
			v_AllCharsIdle(s, pSrc);
			break;
		case SEV_ALLCLIENTS:
			v_AllClients(s, pSrc);
			break;
		case SEV_ALLITEMS:
			v_AllItems(s, pSrc);
			break;
		case SEV_DRY:
			SetWeather(WEATHER_Clear);
			break;
		case SEV_LIGHT:
			if ( g_Cfg.m_bAllowLightOverride )
				SetLight((s.HasArgs()) ? s.GetArgVal() : -1);
			break;
		case SEV_RAIN:
			SetWeather(s.HasArgs() ? static_cast<WEATHER_TYPE>(s.GetArgVal()) : WEATHER_Rain);
			break;
		case SEV_RESPAWN:
			if ( toupper(s.GetArgRaw()[0]) == 'A' )
				g_World.RespawnDeadNPCs();
			else
				RespawnDeadNPCs();
			break;
		case SEV_RESTOCK:
			if ( toupper(s.GetArgRaw()[0]) == 'A' )
				g_World.Restock();
			else
				Restock();
			break;
		case SEV_SEASON:
			SetSeason(static_cast<SEASON_TYPE>(s.GetArgVal()));
			break;
		case SEV_SNOW:
			SetWeather(WEATHER_Snow);
			break;
		default:
			return CScriptObj::r_Verb(s, pSrc);
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
	if ( m_fSaveParity == g_World.m_fSaveParity )	// already saved
		return;

	CPointMap pt = GetBasePoint();
	m_fSaveParity = g_World.m_fSaveParity;
	bool fHeaderCreated = false;

	if ( m_bFlags > 0 )
	{
		g_World.m_FileWorld.WriteSection("SECTOR %hd,%hd,0,%hhu", pt.m_x, pt.m_y, pt.m_map);
		g_World.m_FileWorld.WriteKeyHex("FLAGS", m_bFlags);
		fHeaderCreated = true;
	}

	if ( g_Cfg.m_bAllowLightOverride && IsLightOverriden() )
	{
		if ( !fHeaderCreated )
		{
			g_World.m_FileWorld.WriteSection("SECTOR %hd,%hd,0,%hhu", pt.m_x, pt.m_y, pt.m_map);
			fHeaderCreated = true;
		}

		g_World.m_FileWorld.WriteKeyVal("LIGHT", GetLight());
	}

	if ( !g_Cfg.m_fNoWeather && (IsRainOverriden() || IsColdOverriden()) )
	{
		if ( !fHeaderCreated )
		{
			g_World.m_FileWorld.WriteSection("SECTOR %hd,%hd,0,%hhu", pt.m_x, pt.m_y, pt.m_map);
			fHeaderCreated = true;
		}

		if ( IsRainOverriden() )
			g_World.m_FileWorld.WriteKeyVal("RAINCHANCE", GetRainChance());

		if ( IsColdOverriden() )
			g_World.m_FileWorld.WriteKeyVal("COLDCHANCE", GetColdChance());
	}

	if ( GetSeason() != SEASON_Summer )
	{
		if ( !fHeaderCreated )
			g_World.m_FileWorld.WriteSection("SECTOR %hd,%hd,0,%hhu", pt.m_x, pt.m_y, pt.m_map);

		g_World.m_FileWorld.WriteKeyVal("SEASON", GetSeason());
	}

	// Active chars on sector
	for ( CChar *pChar = static_cast<CChar *>(m_Chars_Active.GetHead()); pChar != NULL; pChar = pChar->GetNext() )
		pChar->r_WriteParity(pChar->m_pPlayer ? g_World.m_FilePlayers : g_World.m_FileWorld);

	// Inactive chars on sector (logout players, ridden mounts, dead NPCs)
	for ( CChar *pChar = static_cast<CChar *>(m_Chars_Disconnect.GetHead()); pChar != NULL; pChar = pChar->GetNext() )
		pChar->r_WriteParity(pChar->m_pPlayer ? g_World.m_FilePlayers : g_World.m_FileWorld);

	// Items on ground
	for ( CItem *pItem = static_cast<CItem *>(m_Items_Timer.GetHead()); pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( pItem->IsTypeMulti() )
			pItem->r_WriteSafe(g_World.m_FileMultis);
		else if ( !pItem->IsAttr(ATTR_STATIC) )
			pItem->r_WriteSafe(g_World.m_FileWorld);
	}
	for ( CItem *pItem = static_cast<CItem *>(m_Items_Inert.GetHead()); pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( pItem->IsTypeMulti() )
			pItem->r_WriteSafe(g_World.m_FileMultis);
		else if ( !pItem->IsAttr(ATTR_STATIC) )
			pItem->r_WriteSafe(g_World.m_FileWorld);
	}
}

bool CSector::v_AllChars(CScript &s, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CSector::v_AllChars");
	// Loop through all active chars on sector

	CScript script(s.GetArgStr());
	bool fRet = false;

	CChar *pCharNext = NULL;
	for ( CChar *pChar = static_cast<CChar *>(m_Chars_Active.GetHead()); pChar != NULL; pChar = pCharNext )
	{
		pCharNext = pChar->GetNext();
		fRet |= pChar->r_Verb(script, pSrc);
	}
	return fRet;
}

bool CSector::v_AllCharsIdle(CScript &s, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CSector::v_AllCharsIdle");
	// Loop through all inactive chars on sector (logout players, ridden mounts, dead NPCs)

	CScript script(s.GetArgStr());
	bool fRet = false;

	CChar *pCharNext = NULL;
	for ( CChar *pChar = static_cast<CChar *>(m_Chars_Disconnect.GetHead()); pChar != NULL; pChar = pCharNext )
	{
		pCharNext = pChar->GetNext();
		fRet |= pChar->r_Verb(script, pSrc);
	}
	return fRet;
}

bool CSector::v_AllClients(CScript &s, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CSector::v_AllClients");
	// Loop through all client chars on sector

	CScript script(s.GetArgStr());
	bool fRet = false;

	CChar *pCharNext = NULL;
	for ( CChar *pChar = static_cast<CChar *>(m_Chars_Active.GetHead()); pChar != NULL; pChar = pCharNext )
	{
		pCharNext = pChar->GetNext();
		if ( pChar->m_pClient )
			fRet |= pChar->r_Verb(script, pSrc);
	}
	return fRet;
}

bool CSector::v_AllItems(CScript &s, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CSector::v_AllItems");
	// Loop through all items on sector

	CScript script(s.GetArgStr());
	bool fRet = false;

	CItem *pItemNext = NULL;
	for ( CItem *pItem = static_cast<CItem *>(m_Items_Timer.GetHead()); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		fRet |= pItem->r_Verb(script, pSrc);
	}
	for ( CItem *pItem = static_cast<CItem *>(m_Items_Inert.GetHead()); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		fRet |= pItem->r_Verb(script, pSrc);
	}
	return fRet;
}

int CSector::GetLocalTime() const
{
	ADDTOCALLSTACK("CSector::GetLocalTime");
	// Get local time of the day (in minutes)

	CPointMap pt = GetBasePoint();
	int iLocalTime = g_World.GetGameWorldTime();

	if ( g_Cfg.m_bAllowLightOverride )
	{
		int iSectorSize = g_MapList.GetSectorSize(pt.m_map);
		int iSectorCols = g_MapList.GetSectorCols(pt.m_map);
		iLocalTime += (pt.m_x / maximum(1, iSectorSize)) * ((24 * 60) / maximum(1, iSectorCols));
	}
	else
	{
		int iSizeX = g_MapList.GetX(pt.m_map);
		iLocalTime += (pt.m_x * 24 * 60) / maximum(1, iSizeX);
	}

	return iLocalTime % (24 * 60);
}

LPCTSTR CSector::GetLocalGameTime() const
{
	ADDTOCALLSTACK("CSector::GetLocalGameTime");
	return GetTimeMinDesc(GetLocalTime());
}

bool CSector::IsMoonVisible(unsigned int uPhase, int iLocalTime) const
{
	ADDTOCALLSTACK("CSector::IsMoonVisible");
	// Check if given time is between moonrise and moonset

	iLocalTime %= 24 * 60;	// just to make sure (1 day = 1440 minutes)
	switch ( uPhase )
	{
		case 0:		// new
			return ((iLocalTime > 360) && (iLocalTime < 1080));
		case 1:		// waxing crescent
			return ((iLocalTime > 540) && (iLocalTime < 1270));
		case 2:		// first quarter
			return (iLocalTime > 720);
		case 3:		// waxing gibbous
			return ((iLocalTime < 180) || (iLocalTime > 900));
		case 4:		// full
			return ((iLocalTime < 360) || (iLocalTime > 1080));
		case 5:		// waning gibbous
			return ((iLocalTime < 540) || (iLocalTime > 1270));
		case 6:		// last quarter
			return (iLocalTime < 720);
		case 7:		// waning crescent
			return ((iLocalTime > 180) && (iLocalTime < 900));
		default:
			return false;
	}
}

BYTE CSector::GetLightCalc(bool fQuickSet) const
{
	ADDTOCALLSTACK("CSector::GetLightCalc");
	// What is the light level default here in this sector.

	if ( g_Cfg.m_bAllowLightOverride && IsLightOverriden() )
		return m_Env.m_Light;

	if ( IsInDungeon() )
		return static_cast<BYTE>(g_Cfg.m_iLightDungeon);

	int iLocalTime = GetLocalTime();

	if ( !g_Cfg.m_bAllowLightOverride )
	{
		//	Normalize time:
		//	convert	0=midnight	.. (23*60)+59 = midnight
		//	to		0=day		.. (12*60)    = night
		if ( iLocalTime < 12 * 60 )
			iLocalTime = 12 * 60 - iLocalTime;
		else
			iLocalTime -= 12 * 60;

		//	0...	y	...lightnight
		//	0...	x	...12*60
		int iTargLight = ((iLocalTime * (g_Cfg.m_iLightNight - g_Cfg.m_iLightDay)) / (12 * 60) + g_Cfg.m_iLightDay);

		if ( iTargLight < LIGHT_BRIGHT )
			iTargLight = LIGHT_BRIGHT;
		else if ( iTargLight > LIGHT_DARK )
			iTargLight = LIGHT_DARK;

		return static_cast<BYTE>(iTargLight);
	}

	int iHour = (iLocalTime / 60) % 24;
	bool fNight = ((iHour < 6) || (iHour > 12 + 8));	// Is it night or day ?
	int iTargLight = fNight ? g_Cfg.m_iLightNight : g_Cfg.m_iLightDay;	// Target light level.

	// Check for clouds...if it is cloudy, then we don't even need to check for the effects of the moons...
	if ( GetWeather() )
	{
		// Clouds of some sort...
		if ( fNight )
			iTargLight += (Calc_GetRandVal(2) + 1);	// 1-2 light levels darker if cloudy at night
		else
			iTargLight += (Calc_GetRandVal(4) + 1);	// 1-4 light levels darker if cloudy during the day.
	}

	if ( fNight )
	{
		// Factor in the effects of the moons
		unsigned int uTrammelPhase = g_World.GetMoonPhase(false);
		if ( IsMoonVisible(uTrammelPhase, iLocalTime) )
		{
			static const BYTE sm_bTrammelPhaseBrightness[] =
			{
				0,									// new
				TRAMMEL_FULL_BRIGHTNESS / 4,		// waxing crescent
				TRAMMEL_FULL_BRIGHTNESS / 2,		// first quarter
				(TRAMMEL_FULL_BRIGHTNESS * 3) / 4,	// waxing gibbous
				TRAMMEL_FULL_BRIGHTNESS,			// full
				(TRAMMEL_FULL_BRIGHTNESS * 3) / 4,	// waning gibbous
				TRAMMEL_FULL_BRIGHTNESS / 2,		// last quarter
				TRAMMEL_FULL_BRIGHTNESS / 4			// waning crescent
			};
			ASSERT(uTrammelPhase < COUNTOF(sm_bTrammelPhaseBrightness));
			iTargLight -= sm_bTrammelPhaseBrightness[uTrammelPhase];
		}

		unsigned int uFeluccaPhase = g_World.GetMoonPhase(true);
		if ( IsMoonVisible(uFeluccaPhase, iLocalTime) )
		{
			static const BYTE sm_bFeluccaPhaseBrightness[] =
			{
				0,									// new
				FELUCCA_FULL_BRIGHTNESS / 4,		// waxing crescent
				FELUCCA_FULL_BRIGHTNESS / 2,		// first quarter
				(FELUCCA_FULL_BRIGHTNESS * 3) / 4,	// waxing gibbous
				FELUCCA_FULL_BRIGHTNESS,			// full
				(FELUCCA_FULL_BRIGHTNESS * 3) / 4,	// waning gibbous
				FELUCCA_FULL_BRIGHTNESS / 2,		// last quarter
				FELUCCA_FULL_BRIGHTNESS / 4			// waning crescent
			};
			ASSERT(uFeluccaPhase < COUNTOF(sm_bFeluccaPhaseBrightness));
			iTargLight -= sm_bFeluccaPhaseBrightness[uFeluccaPhase];
		}
	}

	if ( iTargLight < LIGHT_BRIGHT )
		iTargLight = LIGHT_BRIGHT;
	else if ( iTargLight > LIGHT_DARK )
		iTargLight = LIGHT_DARK;

	if ( fQuickSet || (m_Env.m_Light == iTargLight) )		// initializing the sector
		return static_cast<BYTE>(iTargLight);

	// Gradual transition to global light level
	if ( m_Env.m_Light > iTargLight )
		return m_Env.m_Light - 1;
	else
		return m_Env.m_Light + 1;
}

void CSector::SetLight(int iLight)
{
	ADDTOCALLSTACK("CSector::SetLight");
	// GM set light level command

	if ( (iLight >= LIGHT_BRIGHT) && (iLight <= LIGHT_DARK) )
		m_Env.m_Light = static_cast<BYTE>(iLight | LIGHT_OVERRIDE);
	else
	{
		m_Env.m_Light &= ~LIGHT_OVERRIDE;
		m_Env.m_Light = GetLightCalc(true);
	}
}

void CSector::SetDefaultWeatherChance()
{
	ADDTOCALLSTACK("CSector::SetDefaultWeatherChance");

	CPointMap pt = GetBasePoint();
	int iSizeY = g_MapList.GetY(pt.m_map);
	if ( iSizeY < 1 )
		iSizeY = 1;

	BYTE bPercent = static_cast<BYTE>(IMULDIV(pt.m_y, 100, iSizeY));	// 0 = north, 50 = middle, 100 = south
	if ( bPercent < 50 )
	{
		// Anywhere north of the Britain moongate is a good candidate for snow
		m_RainChance = 15;
		m_ColdChance = 1 + (49 - bPercent) * 2;
	}
	else
	{
		// Rain more likely down south
		m_RainChance = 15 + (bPercent - 50) / 10;
		m_ColdChance = 1;
	}
}

WEATHER_TYPE CSector::GetWeatherCalc() const
{
	ADDTOCALLSTACK("CSector::GetWeatherCalc");
	if ( g_Cfg.m_fNoWeather || IsInDungeon() )
		return WEATHER_Clear;

	int iPercentRoll = Calc_GetRandVal(100);
	if ( GetRainChance() > iPercentRoll )
		return (GetColdChance() > Calc_GetRandVal(100)) ? WEATHER_Snow : WEATHER_Rain;

	if ( GetRainChance() > iPercentRoll / 2 )
		return WEATHER_Cloudy;

	return WEATHER_Clear;
}

void CSector::SetWeather(WEATHER_TYPE weather)
{
	ADDTOCALLSTACK("CSector::SetWeather");
	if ( weather == m_Env.m_Weather )
		return;

	// Clients doesn't change from snow to rain, so it must be dry first
	bool fDryFirst = ((m_Env.m_Weather == WEATHER_Rain) && (weather == WEATHER_Rain));

	m_Env.m_Weather = weather;

	for ( CChar *pChar = static_cast<CChar *>(m_Chars_Active.GetHead()); pChar != NULL; pChar = pChar->GetNext() )
	{
		if ( pChar->m_pClient )
		{
			if ( fDryFirst )
				pChar->m_pClient->addWeather(WEATHER_Clear);
			pChar->m_pClient->addWeather(weather);
		}

		if ( IsTrigUsed(TRIGGER_ENVIRONCHANGE) )
			pChar->OnTrigger(CTRIG_EnvironChange, pChar);
	}
}

void CSector::SetSeason(SEASON_TYPE season)
{
	ADDTOCALLSTACK("CSector::SetSeason");
	if ( season == m_Env.m_Season )
		return;

	m_Env.m_Season = season;

	for ( CChar *pChar = static_cast<CChar *>(m_Chars_Active.GetHead()); pChar != NULL; pChar = pChar->GetNext() )
	{
		if ( pChar->m_pClient )
			pChar->m_pClient->addSeason(season);

		if ( IsTrigUsed(TRIGGER_ENVIRONCHANGE) )
			pChar->OnTrigger(CTRIG_EnvironChange, pChar);
	}
}

void CSector::SetWeatherChance(bool fRain, int iChance)
{
	ADDTOCALLSTACK("CSector::SetWeatherChance");
	if ( iChance > 100 )
		iChance = 100;

	if ( iChance < 0 )
		SetDefaultWeatherChance();	// just set back to default
	else if ( fRain )
		m_RainChance = static_cast<BYTE>(iChance | LIGHT_OVERRIDE);
	else
		m_ColdChance = static_cast<BYTE>(iChance | LIGHT_OVERRIDE);

	SetWeather(GetWeatherCalc());
}

void CSector::OnHearItem(CChar *pChar, TCHAR *pszText)
{
	ADDTOCALLSTACK("CSector::OnHearItem");
	// Report to any of the items that something was said
	ASSERT(m_ListenItems);

	CItem *pItemNext = NULL;
	for ( CItem *pItem = static_cast<CItem *>(m_Items_Timer.GetHead()); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		pItem->OnHear(pszText, pChar);
	}
	for ( CItem *pItem = static_cast<CItem *>(m_Items_Inert.GetHead()); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		pItem->OnHear(pszText, pChar);
	}
}

void CSector::MoveItemToSector(CItem *pItem, bool fActive)
{
	ADDTOCALLSTACK("CSector::MoveItemToSector");
	// Move item to this sector
	// Or change between timer/inert list when the item is not really moved and just get its timer changed
	ASSERT(pItem);

	if ( fActive )
		m_Items_Timer.AddItemToSector(pItem);
	else
		m_Items_Inert.AddItemToSector(pItem);
}

void CSector::CheckSaveParity(CChar *pChar)
{
	ADDTOCALLSTACK("CSector::CheckSaveParity");
	// Check char parity vs sector parity and save it if not saved yet
	// NOTE: char is not yet added to any char list of this sector

	if ( pChar->IsStatFlag(STATF_SaveParity) != m_fSaveParity )
	{
		if ( g_World.IsSaving() )
		{
			if ( m_fSaveParity == g_World.m_fSaveParity )
			{
				// Save only this CChar
				if ( pChar->m_pPlayer )
					pChar->r_WriteParity(g_World.m_FilePlayers);
				else
					pChar->r_WriteParity(g_World.m_FileWorld);
			}
			else
			{
				// Save CSector and everything inside it
				r_Write();
			}
		}
	}
}

bool CSector::MoveCharToSector(CChar *pChar)
{
	ADDTOCALLSTACK("CSector::MoveCharToSector");
	// Move active char to this sector
	// NOTE: char m_pt still pointing to the old location (not moved yet)

	if ( IsCharActiveIn(pChar) )
		return false;	// already here

	CheckSaveParity(pChar);

	m_Chars_Active.AddCharToSector(pChar);	// remove from previous spot
	return true;
}

bool CSector::MoveDisconnectedCharToSector(CChar *pChar)
{
	ADDTOCALLSTACK("CSector::MoveDisconnectedCharToSector");
	// Move disconnected char to this sector
	// NOTE: char m_pt still pointing to the old location (not moved yet)

	if ( IsCharDisconnectedIn(pChar) )
		return false;	// already here

	CheckSaveParity(pChar);

	m_Chars_Disconnect.InsertHead(pChar);	// remove from previous spot
	return true;
}

inline bool CSector::IsSectorSleeping() const
{
	ADDTOCALLSTACK_INTENSIVE("CSector::IsSectorSleeping");
	if ( m_bFlags & 0x1 )	// no sleep
		return false;
	if ( m_bFlags & 0x2 )	// instant sleep
		return (m_Chars_Active.HasClients() > 0);

	return (-g_World.GetTimeDiff(GetLastClientTime()) > 10 * 60 * TICK_PER_SEC);
}

void CSector::SetSectorWakeStatus()
{
	ADDTOCALLSTACK("CSector::SetSectorWakeStatus");
	// NOTE: Ships may enter a sector before it's rider (ships need working timers to move)
	m_Chars_Active.m_timeLastClient = CServTime::GetCurrentTime();
}

void CSector::Close()
{
	ADDTOCALLSTACK("CSector::Close");
	// Clear up all dynamic data for this sector
	m_Items_Timer.DeleteAll();
	m_Items_Inert.DeleteAll();
	m_Chars_Active.DeleteAll();
	m_Chars_Disconnect.DeleteAll();
}

void CSector::RespawnDeadNPCs()
{
	ADDTOCALLSTACK("CSector::RespawnDeadNPCs");
	// Respawn all dead NPCs on this sector
	CChar *pCharNext = NULL;
	for ( CChar *pChar = static_cast<CChar *>(m_Chars_Disconnect.GetHead()); pChar != NULL; pChar = pCharNext )
	{
		pCharNext = pChar->GetNext();
		if ( !pChar->m_pNPC || !pChar->IsStatFlag(STATF_DEAD) || !pChar->m_ptHome.IsValidPoint() )
			continue;

		pChar->NPC_LoadScript(true);
		pChar->MoveNear(pChar->m_ptHome, pChar->m_pNPC->m_Home_Dist_Wander);
		pChar->NPC_CreateTrigger();		// removed from NPC_LoadScript() and triggered after char placement
		pChar->Spell_Resurrection();
	}
}

void CSector::Restock()
{
	ADDTOCALLSTACK("CSector::Restock");
	// Restock all vendors and spawns on this sector

	CChar *pCharNext = NULL;
	for ( CChar *pChar = static_cast<CChar *>(m_Chars_Active.GetHead()); pChar != NULL; pChar = pCharNext )
	{
		pCharNext = pChar->GetNext();
		pChar->NPC_Vendor_Restock(true);
	}

	CItem *pItemNext = NULL;
	for ( CItem *pItem = static_cast<CItem *>(m_Items_Timer.GetHead()); pItem; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( pItem->IsType(IT_SPAWN_ITEM) || pItem->IsType(IT_SPAWN_CHAR) )
			static_cast<CItemSpawn *>(pItem)->OnTick(true);
	}
}

void CSector::OnTick(int iPulseCount)
{
	ADDTOCALLSTACK_INTENSIVE("CSector::OnTick");
	// CWorld gives OnTick() to all CSectors
	EXC_TRY("Tick");

	// Check for light change before put the sector to sleep
	EXC_SET("light change");
	bool fEnvironChange = false;
	bool fLightChange = false;

	if ( !(iPulseCount & 0x7F) )	// 30 seconds or so
	{
		// Check light changes
		BYTE bLightPrev = m_Env.m_Light;
		m_Env.m_Light = GetLightCalc(false);
		if ( m_Env.m_Light != bLightPrev )
		{
			fEnvironChange = true;
			fLightChange = true;
		}
	}

	EXC_SET("sector sleep");
	bool fSleeping = false;
	size_t iClients = m_Chars_Active.HasClients();
	if ( iClients <= 0 )
	{
		// Put the sector to sleep if there's no clients here
		fSleeping = IsSectorSleeping();
		if ( fSleeping )
		{
			if ( !g_Cfg.m_iSectorSleepMask || ((iPulseCount & g_Cfg.m_iSectorSleepMask) != (GetIndex() & g_Cfg.m_iSectorSleepMask)) )
				return;
		}
	}

	EXC_SET("sound effects");
	// Random weather noises and effects
	SOUND_TYPE iSound = 0;
	bool fWeatherChange = false;
	int iRegionPeriodic = 0;

	if ( !(iPulseCount & 0x7F) )	// 30 seconds or so
	{
		// Check weather changes
		if ( !Calc_GetRandVal(30) )		// change less often
		{
			WEATHER_TYPE bWeatherPrev = m_Env.m_Weather;
			m_Env.m_Weather = GetWeatherCalc();
			if ( m_Env.m_Weather != bWeatherPrev )
			{
				fEnvironChange = true;
				fWeatherChange = true;
			}
		}

		// Random area noises
		if ( iClients > 0 )
		{
			iRegionPeriodic = 2;
			switch ( GetWeather() )
			{
				case WEATHER_Rain:
				{
					int iVal = Calc_GetRandVal(30);
					if ( iVal < 5 )
					{
						static const SOUND_TYPE sm_SoundThunder[] = { 0x28, 0x29, 0x206 };
						iSound = sm_SoundThunder[Calc_GetRandVal(COUNTOF(sm_SoundThunder))];
					}
					else if ( iVal < 10 )
					{
						static const SOUND_TYPE sm_SoundRain[] = { 0x10, 0x11 };
						iSound = sm_SoundRain[Calc_GetRandVal(COUNTOF(sm_SoundRain))];
					}
					else if ( iVal < 15 )
					{
						static const SOUND_TYPE sm_SoundWind[] = { 0x14, 0x15, 0x16 };
						iSound = sm_SoundWind[Calc_GetRandVal(COUNTOF(sm_SoundWind))];
					}
					break;
				}
				case WEATHER_Snow:
				{
					if ( !Calc_GetRandVal(5) )
					{
						static const SOUND_TYPE sm_SoundWind[] = { 0x14, 0x15, 0x16 };
						iSound = sm_SoundWind[Calc_GetRandVal(COUNTOF(sm_SoundWind))];
					}
					break;
				}
				default:
					break;
			}
		}
	}

	// Tick all sector chars
	ProfileTask charactersTask(PROFILE_CHARS);

	CChar *pCharNext = NULL;
	for ( CChar *pChar = static_cast<CChar *>(m_Chars_Active.GetHead()); pChar != NULL; pChar = pCharNext )
	{
		EXC_TRYSUB("TickChar");
		pCharNext = pChar->GetNext();
		if ( fEnvironChange && IsTrigUsed(TRIGGER_ENVIRONCHANGE) )
			pChar->OnTrigger(CTRIG_EnvironChange, pChar);

		if ( pChar->m_pClient )
		{
			if ( iSound )
				pChar->m_pClient->addSound(iSound, pChar);

			if ( fLightChange && !pChar->IsStatFlag(STATF_DEAD|STATF_NightSight) )
				pChar->m_pClient->addLight();

			if ( fWeatherChange )
				pChar->m_pClient->addWeather(GetWeather());

			if ( iRegionPeriodic && pChar->m_pArea )
			{
				if ( (iRegionPeriodic == 2) && IsTrigUsed(TRIGGER_REGPERIODIC) )
				{
					pChar->m_pArea->OnRegionTrigger(pChar, RTRIG_REGPERIODIC);
					--iRegionPeriodic;
				}
				if ( IsTrigUsed(TRIGGER_CLIPERIODIC) )
					pChar->m_pArea->OnRegionTrigger(pChar, RTRIG_CLIPERIODIC);
			}
		}

		if ( !pChar->OnTick() )
			pChar->Delete();

		EXC_CATCHSUB("Sector");

		EXC_DEBUGSUB_START;
		CPointMap pt = GetBasePoint();
		g_Log.EventDebug("char 0%" FMTDWORDH " '%s'\n", static_cast<DWORD>(pChar->GetUID()), pChar->GetName());
		g_Log.EventDebug("sector #%d [%hd,%hd,%hhd,%hhu]\n", GetIndex(), pt.m_x, pt.m_y, pt.m_z, pt.m_map);
		EXC_DEBUGSUB_END;
	}

	// Tick all sector items
	ProfileTask itemsTask(PROFILE_ITEMS);

	CItem *pItemNext = NULL;
	for ( CItem *pItem = static_cast<CItem *>(m_Items_Timer.GetHead()); pItem != NULL; pItem = pItemNext )
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
				if ( pItem->IsTimerExpired() )	// forgot to clear the timer? strange
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
		g_Log.EventError("item 0%" FMTDWORDH " '%s' [timer=%lld, type=%d]\n", static_cast<DWORD>(pItem->GetUID()), pItem->GetName(), pItem->GetTimerAdjusted(), static_cast<int>(pItem->GetType()));
		g_Log.EventError("sector #%d [%hd,%hd,%hhd,%hhu]\n", GetIndex(), pt.m_x, pt.m_y, pt.m_z, pt.m_map);

		EXC_DEBUGSUB_END;
#else
	}
#ifndef _DEBUG
	catch ( const CGrayError &e )
	{
		PAUSECALLSTACK;
		CPointMap pt = GetBasePoint();
		g_Log.EventError("CGrayError: item 0%" FMTDWORDH " '%s' [timer=%lld, type=%d]\n", static_cast<DWORD>(pItem->GetUID()), pItem->GetName(), pItem->GetTimerAdjusted(), static_cast<int>(pItem->GetType()));
		g_Log.EventError("sector #%d [%hd,%hd,%hhd,%hhu]\n", GetIndex(), pt.m_x, pt.m_y, pt.m_z, pt.m_map);
		UNPAUSECALLSTACK;
		EXC_CATCHSUB_EVENT(&e, "Sector");
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	catch ( ... )
	{
		CPointMap pt = GetBasePoint();
		g_Log.EventError("...: item 0%" FMTDWORDH " '%s' [timer=%lld, type=%d]\n", static_cast<DWORD>(pItem->GetUID()), pItem->GetName(), pItem->GetTimerAdjusted(), static_cast<int>(pItem->GetType()));
		g_Log.EventError("sector #%d [%hd,%hd,%hhd,%hhu]\n", GetIndex(), pt.m_x, pt.m_y, pt.m_z, pt.m_map);
		EXC_CATCHSUB_EVENT(NULL, "Sector");
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
#endif
#endif
	}

	ProfileTask overheadTask(PROFILE_OVERHEAD);

	EXC_SET("check map cache");
	if ( fSleeping || !(iPulseCount & 0x7F) )	// 30 seconds or so
	{
		m_iMapBlockCacheTime = fSleeping ? 0 : g_Cfg.m_iMapCacheTime;
		CheckMapBlockCache();
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	CPointMap pt = GetBasePoint();
	g_Log.EventError("sector #%d [%hd,%hd,%hhd,%hhu]\n", GetIndex(), pt.m_x, pt.m_y, pt.m_z, pt.m_map);
	EXC_DEBUG_END;
}
