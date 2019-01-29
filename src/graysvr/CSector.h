#ifndef _INC_CSECTOR_H
#define _INC_CSECTOR_H
#pragma once

#define SECTOR_TICK_PERIOD		(TICK_PER_SEC / 4)	// pulse length

class CSector : public CScriptObj, public CSectorBase
{
	// A square region of the world
	// Eg: map0.mul dungeon sectors are 256x256 tiles
public:
	CSector();
	~CSector()
	{
		ASSERT(!HasClients());
	}

	LPCTSTR GetName() const
	{
		return "Sector";
	}

	static const char *m_sClassName;
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szLoadKeys[];
private:
	BYTE m_ListenItems;		// items on the ground that can listen (eg: communication crystal)
	bool m_fSaveParity;		// the sector has been saved relative to the char entering it?

	CSectorEnviron m_Env;	// current environment
	BYTE m_RainChance;		// rain chance (0% - 100%)
	BYTE m_ColdChance;		// cold chance (0% - 100%) - will snow if also raining

public:
	void Close();
	void RespawnDeadNPCs();
	void Restock();
	void OnTick(int iPulseCount);

	bool v_AllChars(CScript &s, CTextConsole *pSrc);
	bool v_AllCharsIdle(CScript &s, CTextConsole *pSrc);
	bool v_AllClients(CScript &s, CTextConsole *pSrc);
	bool v_AllItems(CScript &s, CTextConsole *pSrc);

	virtual bool r_LoadVal(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	virtual void r_Write();
	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);

private:
	bool IsMoonVisible(unsigned int uPhase, int iLocalTime) const;
	void CheckSaveParity(CChar *pChar);

public:
	// Time
	int GetLocalTime() const;
	LPCTSTR GetLocalGameTime() const;

	// Light
	BYTE GetLightCalc(bool fQuickSet) const;
	BYTE GetLight() const
	{
		return m_Env.m_Light & ~LIGHT_OVERRIDE;
	}
	void SetLight(int iLight);

	bool IsLightOverriden() const
	{
		return (m_Env.m_Light & LIGHT_OVERRIDE);
	}
	bool IsDark() const
	{
		return (GetLight() >= 12);
	}
	bool IsNight() const
	{
		int iMinutes = GetLocalTime();
		return ((iMinutes < 7 * 60) || (iMinutes > (9 + 12) * 60));
	}

	// Weather
	WEATHER_TYPE GetWeatherCalc() const;
	WEATHER_TYPE GetWeather() const
	{
		return m_Env.m_Weather;
	}
	void SetWeather(WEATHER_TYPE weather);

	void SetDefaultWeatherChance();
	void SetWeatherChance(bool fRain, int iChance);

	BYTE GetRainChance() const
	{
		return m_RainChance & ~LIGHT_OVERRIDE;
	}
	bool IsRainOverriden() const
	{
		return (m_RainChance & LIGHT_OVERRIDE);
	}

	BYTE GetColdChance() const
	{
		return m_ColdChance & ~LIGHT_OVERRIDE;
	}
	bool IsColdOverriden() const
	{
		return (m_ColdChance & LIGHT_OVERRIDE);
	}

	// Season
	SEASON_TYPE GetSeason() const
	{
		return m_Env.m_Season;
	}
	void SetSeason(SEASON_TYPE season);

	// Items in the sector
	void MoveItemToSector(CItem *pItem, bool fActive);
	bool IsItemInSector(const CItem *pItem) const
	{
		if ( !pItem )
			return false;

		return ((pItem->GetParent() == &m_Items_Inert) || (pItem->GetParent() == &m_Items_Timer));
	}

	size_t GetItemComplexity() const
	{
		return m_Items_Timer.GetCount() + m_Items_Inert.GetCount();
	}

	void AddListenItem()
	{
		++m_ListenItems;
	}
	void RemoveListenItem()
	{
		--m_ListenItems;
	}
	bool HasListenItems() const
	{
		return m_ListenItems ? true : false;
	}
	void OnHearItem(CChar *pChar, TCHAR *pszText);

	// Chars in the sector
	bool MoveCharToSector(CChar *pChar);
	bool MoveDisconnectedCharToSector(CChar *pChar);

	bool IsSectorSleeping() const;
	void SetSectorWakeStatus();		// ships may enter a sector before it's riders

	size_t GetCharComplexity() const
	{
		return m_Chars_Active.GetCount();
	}
	size_t GetInactiveChars() const
	{
		return m_Chars_Disconnect.GetCount();
	}

	bool IsCharActiveIn(const CChar *pChar) const
	{
		// Check if char is active
		return (pChar->GetParent() == &m_Chars_Active);
	}
	bool IsCharDisconnectedIn(const CChar *pChar) const
	{
		// Check if char is inactive (disconnected)
		return (pChar->GetParent() == &m_Chars_Disconnect);
	}

	// Clients in the sector
	size_t HasClients() const
	{
		return m_Chars_Active.HasClients();
	}
	CServTime GetLastClientTime() const
	{
		return m_Chars_Active.m_timeLastClient;
	}

	void ClientAttach(CChar *pChar)
	{
		if ( !IsCharActiveIn(pChar) )
			return;
		m_Chars_Active.ClientAttach();
	}
	void ClientDetach(CChar *pChar)
	{
		if ( !IsCharActiveIn(pChar) )
			return;
		m_Chars_Active.ClientDetach();
	}

private:
	CSector(const CSector &copy);
	CSector &operator=(const CSector &other);
};

#endif	// _INC_CSECTOR_H
