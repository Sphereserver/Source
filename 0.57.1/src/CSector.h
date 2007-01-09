//
// CSector.h
//

#ifndef _INC_CSECTOR_H
#define _INC_CSECTOR_H
#pragma once

class CCharsDisconnectList : public CGObList
{
public:
	static const char *m_sClassName;
};

class CCharsActiveList : public CGObList
{
private:
	int	   m_iClients;			// How many clients in this sector now?
public:
	static const char *m_sClassName;
	CServTime m_timeLastClient;	// age the sector based on last client here.
protected:
	void OnRemoveOb( CGObListRec* pObRec );	// Override this = called when removed from list.
public:
	int HasClients() const { return( m_iClients ); }
	void ClientAttach();
	void ClientDetach();
	void AddCharToSector( CChar * pChar );
	CCharsActiveList();
};

class CItemsList : public CGObList
{
	// Top level list of items.
public:
	static bool sm_fNotAMove;	// hack flag to prevent items from bouncing around too much.
protected:
	void OnRemoveOb( CGObListRec* pObRec );	// Override this = called when removed from list.
public:
	static const char *m_sClassName;
	void AddItemToSector( CItem * pItem );
};

class CObPointSortArray : public CGObSortArray< CPointSort*, long >
{
public:
	static const char *m_sClassName;

	// Find a point fast.
	int CompareKey( long id, CPointSort* pBase, bool fNoSpaces ) const
	{
		return (id - pBase->GetPointSortIndex());
	}
};

class CSector : public CScriptObj		// square region of the world.
{
public:
	struct Environment
	{
		#define LIGHT_OVERRIDE 0x80
		BYTE m_Light;		// the calculated light level in this area. |0x80 = override.
		SEASON_TYPE m_Season;		// What is the season for this sector.
		WEATHER_TYPE m_Weather;		// the weather in this area now.
		Environment();
		void invalidate();
	};

protected:
	int	m_index;		// sector index
	int m_map;			// sector map

#ifdef NEW_MAP_CACHE
	struct TerrainRec			//	map terain record in x,y,z,map
	{
		char	m_name[20];
		DWORD	m_flags;
		int		m_terrain;
	};

	struct StaticRec			//	map statics record in x,y,z,map
	{
		int		m_tile;
		int		m_hue;
	};

	struct PointRec				//	point record in map
	{
		int					m_x;
		int					m_y;
		int					m_z;
		vector<TerrainRec>	m_terrains;
		vector<StaticRec>	m_statics;
		vector<UIDBase>		m_chars;
		vector<UIDBase>		m_items;
	};
								//	sector points information cache
	vector<PointRec>		m_cache;
#endif

public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szLoadKeys[];

	CObPointSortArray	m_Teleports;		//	CTeleport array
	CRegionLinks		m_RegionLinks;		//	CRegionBase(s) in this CSector

	CCharsActiveList		m_Chars_Active;		// CChar(s) activte in this CSector.
	CCharsDisconnectList	m_Chars_Disconnect;	// dead NPCs, etc
	CItemsList m_Items_Timer;				// CItem(s) in this CSector that need timers.
	CItemsList m_Items_Inert;				// CItem(s) in this CSector. (no timer required)

private:
	CObPointSortArray	m_MapBlockCache;	//	CGrayMapBlock map cache. 

	bool   m_fSaveParity;		// has the sector been saved relative to the char entering it ?
	Environment	m_Env;		// Current Environment

	BYTE m_RainChance;		// 0 to 100%
	BYTE m_ColdChance;		// Will be snow if rain chance success.
	WORD m_ListenItems;		// Items on the ground that listen ?

private:
	WEATHER_TYPE GetWeatherCalc() const;
	BYTE GetLightCalc() const;
	int GetSeasonLightDiff(int light) const;
	void SetLightNow( bool fFlash = false );

public:
	CSector();
	~CSector();
	void OnTick( int iPulse );

	void Init(int index, int map);

	// Location map units.
	CPointMap GetBasePoint() const;
	CRectMap GetRect() const;
	bool IsInDungeon() const;

	void CheckMapBlockCache( int iTime );
	const CGrayMapBlock * GetMapBlock( const CPointMap & pt );

	// CRegionBase
	CRegionBase * GetRegion( const CPointBase & pt, DWORD dwType ) const;
	int GetRegions( const CPointBase & pt, DWORD dwType, CRegionLinks & rlist ) const;

	bool UnLinkRegion( CRegionBase * pRegionOld );
	bool LinkRegion( CRegionBase * pRegionNew );

	// CTeleport(s) in the region.
	CTeleport * GetTeleport( const CPointMap & pt ) const;
	bool AddTeleport( CTeleport * pTeleport );

	// Time
	int GetLocalTime() const;
	LPCTSTR GetLocalGameTime(bool numeric = false) const;

	SEASON_TYPE GetSeason() const
	{
		return m_Env.m_Season;
	}
	void SetSeason( SEASON_TYPE season );

	// Weather
	WEATHER_TYPE GetWeather() const	// current weather.
	{
		return m_Env.m_Weather;
	}
	bool IsRainOverriden() const
	{
		return(( m_RainChance & LIGHT_OVERRIDE ) ? true : false );
	}
	BYTE GetRainChance() const
	{
		return( m_RainChance &~ LIGHT_OVERRIDE );
	}
	bool IsColdOverriden() const
	{
		return(( m_ColdChance & LIGHT_OVERRIDE ) ? true : false );
	}
	BYTE GetColdChance() const
	{
		return( m_ColdChance &~ LIGHT_OVERRIDE );
	}
	void SetWeather( WEATHER_TYPE w );
	void SetWeatherChance( bool fRain, int iChance );

	// Light
	bool IsLightOverriden() const
	{
		return(( m_Env.m_Light & LIGHT_OVERRIDE ) ? true : false );
	}
	BYTE GetLight() const
	{
		return( m_Env.m_Light &~ LIGHT_OVERRIDE );
	}
	bool IsDark() const
	{
		return( GetLight() > 6 );
	}
	void LightFlash()
	{
		SetLightNow( true );
	}
	void SetLight( int light );

	// Items in the sector

	int GetItemComplexity() const
	{
		return m_Items_Timer.GetCount() + m_Items_Inert.GetCount();
	}
	void MoveItemToSector( CItem * pItem, bool fActive );

	void AddListenItem()
	{
		m_ListenItems++;
	}
	void RemoveListenItem()
	{
		m_ListenItems--;
	}
	bool HasListenItems() const
	{
		return m_ListenItems ? true : false;
	}
	void OnHearItem( CChar * pChar, TCHAR * szText );

	//	sector caching
protected:
	bool m_bSleeping;

public:
	bool isWalkable(CPointMap pt, CChar *person);

	bool IsSleeping() const;
	void wakeUp();				// wakes up
	void sleep();				// stats sleeping
	void CheckSleepingStatus();	// check whatever to wake up / start sleeping

	// Chars in the sector.
	bool IsCharActiveIn( const CChar * pChar );
	bool IsCharDisconnectedIn( const CChar * pChar );
	int GetCharComplexity() const;
	int HasClients() const;
	void ClientAttach( CChar * pChar );
	void ClientDetach( CChar * pChar );
	bool MoveCharToSector( CChar * pChar );

	// General.
	virtual bool r_LoadVal( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	virtual void r_Write();
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc );

	// AllThings Verbs
	bool v_AllChars( CScript & s, CTextConsole * pSrc );
	bool v_AllCharsIdle( CScript & s, CTextConsole * pSrc );
	bool v_AllItems( CScript & s, CTextConsole * pSrc );
	bool v_AllClients( CScript & s, CTextConsole * pSrc );

	// Other resources.
	void Restock();
	void RespawnDeadNPCs();

	LPCTSTR GetName() const { return( "Sector" ); }
};
#endif
