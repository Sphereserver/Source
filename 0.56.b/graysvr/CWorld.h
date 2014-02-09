//
// CWorld.h
//

#ifndef _INC_CWORLD_H
#define _INC_CWORLD_H
#pragma once

#include "../common/mtrand/mtrand.h"

class CSector : public CScriptObj, public CSectorBase	// square region of the world.
{
	// A square region of the world. ex: MAP0.MUL Dungeon Sectors are 256 by 256 meters
#define SECTOR_TICK_PERIOD (TICK_PER_SEC/4) // length of a pulse.

public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szLoadKeys[];
private:

	bool   m_fSaveParity;		// has the sector been saved relative to the char entering it ?
	CSectorEnviron m_Env;		// Current Environment

	BYTE m_RainChance;		// 0 to 100%
	BYTE m_ColdChance;		// Will be snow if rain chance success.
	BYTE m_ListenItems;		// Items on the ground that listen ?

private:
	WEATHER_TYPE GetWeatherCalc() const;
	BYTE GetLightCalc( bool fQuickSet ) const;
	void SetLightNow( bool fFlash = false );
	bool IsMoonVisible( unsigned int iPhase, int iLocalTime ) const;
	void SetDefaultWeatherChance();

public:
	CSector();
	~CSector()
	{
		ASSERT( ! HasClients());
	}

private:
	CSector(const CSector& copy);
	CSector& operator=(const CSector& other);

public:
	void OnTick( int iPulse );

	// Time
	int GetLocalTime() const;
	LPCTSTR GetLocalGameTime() const;

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
	bool IsNight() const
	{
		int iMinutes = GetLocalTime();
		return( iMinutes < 7*60 || iMinutes > (9+12)*60 );
	}
	void LightFlash()
	{
		SetLightNow( true );
	}
	void SetLight( int light );

	// Items in the sector

	size_t GetItemComplexity() const
	{
		return m_Items_Timer.GetCount() + m_Items_Inert.GetCount();
	}
	bool IsItemInSector( const CItem * pItem ) const
	{
		if ( !pItem )
			return false;

		return pItem->GetParent() == &m_Items_Inert ||
			   pItem->GetParent() == &m_Items_Timer;
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

	// Chars in the sector.
	bool IsCharActiveIn( const CChar * pChar ) //const
	{
		// assume the char is active (not disconnected)
		return( pChar->GetParent() == &m_Chars_Active );
	}
	bool IsCharDisconnectedIn( const CChar * pChar ) //const
	{
		// assume the char is active (not disconnected)
		return( pChar->GetParent() == &m_Chars_Disconnect );
	}
	size_t GetCharComplexity() const
	{
		return( m_Chars_Active.GetCount());
	}
	size_t HasClients() const
	{
		return( m_Chars_Active.HasClients());
	}
	CServTime GetLastClientTime() const
	{
		return( m_Chars_Active.m_timeLastClient );
	}
	bool IsSectorSleeping() const;
	void SetSectorWakeStatus();	// Ships may enter a sector before it's riders !
	void ClientAttach( CChar * pChar )
	{
		if ( ! IsCharActiveIn( pChar ))
			return;
		m_Chars_Active.ClientAttach();
	}
	void ClientDetach( CChar * pChar )
	{
		if ( ! IsCharActiveIn( pChar ))
			return;
		m_Chars_Active.ClientDetach();
	}
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

	void Close();
	LPCTSTR GetName() const { return( "Sector" ); }
};

enum IMPFLAGS_TYPE	// IMPORT and EXPORT flags.
{
	IMPFLAGS_NOTHING = 0,
	IMPFLAGS_ITEMS = 0x01,		// 0x01 = items,
	IMPFLAGS_CHARS = 0x02,		// 0x02 = characters
	IMPFLAGS_BOTH  = 0x03,		// 0x03 = both
	IMPFLAGS_PLAYERS = 0x04,
	IMPFLAGS_RELATIVE = 0x10,	// 0x10 = distance relative. dx, dy to you
	IMPFLAGS_ACCOUNT = 0x20		// 0x20 = recover just this account/char	(and all it is carrying)
};

#define FREE_UIDS_SIZE	500000	//	the list of free empty slots in the uids list should contain these elements


class CWorldThread
{
	// A regional thread of execution. hold all the objects in my region.
	// as well as those just created here. (but may not be here anymore)

protected:
	CGObArray<CObjBase*> m_UIDs;	// all the UID's in the World. CChar and CItem.
	int m_iUIDIndexLast;	// remeber the last index allocated so we have more even usage.

	DWORD	*m_FreeUIDs;		//	list of free uids available
	DWORD	m_FreeOffset;		//	offset of the first free element

public:
	static const char *m_sClassName;
	//int m_iThreadNum;	// Thread number to id what range of UID's i own.
	//CRectMap m_ThreadRect;	// the World area that this thread owns.

	//int m_iUIDIndexBase;		// The start of the uid range that i will allocate from.
	//int m_iUIDIndexMaxQty;	// The max qty of UIDs i can allocate.

	CGObList m_ObjNew;			// Obj created but not yet placed in the world.
	CGObList m_ObjDelete;		// Objects to be deleted.

	// Background save. Does this belong here ?
	CScript m_FileData;			// Save or Load file.
	CScript m_FileWorld;		// Save or Load file.
	CScript m_FilePlayers;		// Save of the players chars.
	CScript m_FileMultis;		// Save of the custom multis.
	bool	m_fSaveParity;		// has the sector been saved relative to the char entering it ?
	MTRand	m_Rand;

public:
	// Backgound Save
	bool IsSaving() const;

	// UID Managenent
	DWORD GetUIDCount() const;
#define UID_PLACE_HOLDER ((CObjBase*)0xFFFFFFFF)
	CObjBase * FindUID(DWORD dwIndex) const;
	void FreeUID(DWORD dwIndex);
	DWORD AllocUID( DWORD dwIndex, CObjBase * pObj );

	int FixObjTry( CObjBase * pObj, DWORD dwUID = 0 );
	int FixObj( CObjBase * pObj, DWORD dwUID = 0 );

	void SaveThreadClose();
	void GarbageCollection_UIDs();
	void GarbageCollection_New();

	void CloseAllUIDs();

public:
	CWorldThread();
	virtual ~CWorldThread();

private:
	CWorldThread(const CWorldThread& copy);
	CWorldThread& operator=(const CWorldThread& other);
};

class CWorldClock
{
#ifndef _WIN32
#ifdef CLOCKS_PER_SEC
#undef CLOCKS_PER_SEC
#endif	// CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 1000	// must be converted from some internal clock.
#endif

private:
	CServTime m_timeClock;		// the current relative tick time  (in TICK_PER_SEC)
	DWORD  m_Clock_PrevSys;		// System time of the last OnTick(). (CLOCKS_PER_SEC)
public:
	static const char *m_sClassName;
	CWorldClock()
	{
		Init();
	}

private:
	CWorldClock(const CWorldClock& copy);
	CWorldClock& operator=(const CWorldClock& other);

public:
	void Init();
	void InitTime( INT64 lTimeBase );
	bool Advance();
	CServTime GetCurrentTime() const	// TICK_PER_SEC
	{
		return( m_timeClock );
	}
	static DWORD GetSystemClock();	// CLOCKS_PER_SEC
};

class CTimedFunctionHandler
{
	public:
		struct TimedFunction
		{
			int		elapsed;
			char	funcname[1024];
			CGrayUID 	uid;
		};

	private:
		std::vector<TimedFunction *> m_timedFunctions[TICK_PER_SEC];
		int m_curTick;
		std::vector<TimedFunction *> m_tFrecycled;
		std::vector<TimedFunction *> m_tFqueuedToBeAdded;
		bool m_isBeingProcessed;

	public:
		static const char *m_sClassName;
		CTimedFunctionHandler();

	private:
		CTimedFunctionHandler(const CTimedFunctionHandler& copy);
		CTimedFunctionHandler& operator=(const CTimedFunctionHandler& other);

	public:
		void OnTick();
		void r_Write( CScript & s );
		
		int Load( const char *pszName, bool fQuoted, const char *pszVal );
		void Add( CGrayUID uid, int numSeconds, LPCTSTR funcname );
		void Erase( CGrayUID uid );
		void Stop( CGrayUID uid, LPCTSTR funcname );
		int IsTimer( CGrayUID uid, LPCTSTR funcname );
};

extern class CWorld : public CScriptObj, public CWorldThread
{
	// the world. Stuff saved in *World.SCP
private:
	// Clock stuff. how long have we been running ? all i care about.
	CWorldClock m_Clock;		// the current relative tick time  (in TICK_PER_SEC)

	// Special purpose timers.
	CServTime	m_timeSector;		// next time to do sector stuff.
	CServTime	m_timeSave;		// when to auto save ?
	bool m_bSaveNotificationSent;	// has notification been sent?
	CServTime	m_timeRespawn;	// when to res dead NPC's ?
	CServTime	m_timeCallUserFunc;	// when to call next user func
	int		m_Sector_Pulse;		// Slow some stuff down that doesn't need constant processing.

	int		m_iSaveStage;	// Current stage of the background save.
	LONGLONG m_savetimer; // Time it takes to save

public:
	static const char *m_sClassName;
	// World data.
	CSector	**m_Sectors;
	int		m_SectorsQty;

public:
	int		m_iSaveCountID;	// Current archival backup id. Whole World must have this same stage id
	int		m_iLoadVersion;		// Previous load version. (only used during load of course)
	CServTime m_timeStartup;		// When did the system restore load/save ?

	CGrayUID m_uidLastNewItem;	// for script access.
	CGrayUID m_uidLastNewChar;	// for script access.
	CGrayUID m_uidObj;			// for script access - auxiliary obj
	CGrayUID m_uidNew;			// for script access - auxiliary obj

	CGObList m_GMPages;		// Current outstanding GM pages. (CGMPage)

	
	CGPtrTypeArray<CItemStone*> m_Stones;	// links to leige/town stones. (not saved array)
	CGObList m_Parties;	// links to all active parties. CPartyDef

	static LPCTSTR const sm_szLoadKeys[];
	CGPtrTypeArray <CItemTypeDef *> m_TileTypes;

	// TimedFunction Container/Wrapper
	CTimedFunctionHandler m_TimedFunctions;
	CGPtrTypeArray<CObjBase*> m_ObjStatusUpdates; // objects that need OnTickStatusUpdate called

private:
	bool LoadFile( LPCTSTR pszName, bool fError = true );
	bool LoadWorld();

	bool SaveTry(bool fForceImmediate); // Save world state
	bool SaveStage();
	static void GetBackupName( CGString & sArchive, LPCTSTR pszBaseDir, TCHAR chType, int savecount );
	bool SaveForce(); // Save world state

public:
	CWorld();
	~CWorld();

private:
	CWorld(const CWorld& copy);
	CWorld& operator=(const CWorld& other);

public:
	void Init();

	CSector *GetSector( int map, int i );	// gets sector # from one map

	// Time

	CServTime GetCurrentTime() const
	{
		return m_Clock.GetCurrentTime();  // Time in TICK_PER_SEC
	}
	INT64 GetTimeDiff( CServTime time ) const
	{
		// How long till this event
		// negative = in the past.
		return( time.GetTimeDiff( GetCurrentTime() )); // Time in TICK_PER_SEC
	}

#define TRAMMEL_SYNODIC_PERIOD 105 // in game world minutes
#define FELUCCA_SYNODIC_PERIOD 840 // in game world minutes
#define TRAMMEL_FULL_BRIGHTNESS 2 // light units LIGHT_BRIGHT
#define FELUCCA_FULL_BRIGHTNESS 6 // light units LIGHT_BRIGHT
	unsigned int GetMoonPhase( bool bMoonIndex = false ) const;
	CServTime GetNextNewMoon( bool bMoonIndex ) const;

	DWORD GetGameWorldTime( CServTime basetime ) const;
	DWORD GetGameWorldTime() const	// return game world minutes
	{
		return( GetGameWorldTime( GetCurrentTime()));
	}

	// CSector World Map stuff.

	void GetHeightPoint( const CPointMap & pt, CGrayMapBlockState & block, bool fHouseCheck = false );
	signed char GetHeightPoint( const CPointBase & pt, DWORD & wBlockFlags, bool fHouseCheck = false ); // Height of player who walked to X/Y/OLDZ

	void GetHeightPoint_New( const CPointMap & pt, CGrayMapBlockState & block, bool fHouseCheck = false );
	signed char GetHeightPoint_New( const CPointBase & pt, DWORD & wBlockFlags, bool fHouseCheck = false );

	CItemTypeDef *	GetTerrainItemTypeDef( DWORD dwIndex );
	IT_TYPE			GetTerrainItemType( DWORD dwIndex );

	const CGrayMapBlock * GetMapBlock( const CPointMap & pt )
	{
		return( pt.GetSector()->GetMapBlock(pt));
	}
	const CUOMapMeter * GetMapMeter( const CPointMap & pt ) const // Height of MAP0.MUL at given coordinates
	{
		const CGrayMapBlock * pMapBlock = pt.GetSector()->GetMapBlock(pt);
		if ( !pMapBlock )
			return NULL;
		return( pMapBlock->GetTerrain( UO_BLOCK_OFFSET(pt.m_x), UO_BLOCK_OFFSET(pt.m_y)));
	}

	CPointMap FindItemTypeNearby( const CPointMap & pt, IT_TYPE iType, int iDistance = 0, bool bCheckMulti = false, bool bLimitZ = false );
	bool IsItemTypeNear( const CPointMap & pt, IT_TYPE iType, int iDistance, bool bCheckMulti );

	CPointMap FindTypeNear_Top( const CPointMap & pt, IT_TYPE iType, int iDistance = 0 );
	bool IsTypeNear_Top( const CPointMap & pt, IT_TYPE iType, int iDistance = 0 );
	CItem * CheckNaturalResource( const CPointMap & pt, IT_TYPE Type, bool fTest = true, CChar * pCharSrc = NULL );

	static bool OpenScriptBackup( CScript & s, LPCTSTR pszBaseDir, LPCTSTR pszBaseName, int savecount );

	void r_Write( CScript & s );
	bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );
	bool r_LoadVal( CScript & s ) ;
	bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );

	void OnTick();

	void GarbageCollection();
	void Restock();
	void RespawnDeadNPCs();

	void Speak( const CObjBaseTemplate * pSrc, LPCTSTR pText, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD );
	void SpeakUNICODE( const CObjBaseTemplate * pSrc, const NCHAR * pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang );

	void Broadcast( LPCTSTR pMsg );
	void __cdecl Broadcastf( LPCTSTR pMsg, ...) __printfargs(2,3);
	void Explode( CChar * pSrc, CPointMap pt, int iDist, int iDamage, WORD wFlags = DAMAGE_GENERAL | DAMAGE_HIT_BLUNT );

	bool Export( LPCTSTR pszFilename, const CChar* pSrc, WORD iModeFlags = IMPFLAGS_ITEMS, int iDist = SHRT_MAX, int dx = 0, int dy = 0 );
	bool Import( LPCTSTR pszFilename, const CChar* pSrc, WORD iModeFlags = IMPFLAGS_ITEMS, int iDist = SHRT_MAX, TCHAR *pszAgs1 = NULL, TCHAR *pszAgs2 = NULL );
	bool Save( bool fForceImmediate ); // Save world state
	void SaveStatics();
	bool LoadAll();
	bool DumpAreas( CTextConsole * pSrc, LPCTSTR pszFilename );
	void Close();

	LPCTSTR GetName() const { return( "World" ); }

} g_World;

class CWorldSearch	// define a search of the world.
{
private:
	const CPointMap m_pt;		// Base point of our search.
	const int m_iDist;			// How far from the point are we interested in
	bool m_fAllShow;		// Include Even inert items.
	bool m_fSearchSquare;		// Search in a square (uo-sight distance) rather than a circle (standard distance).

	CObjBase * m_pObj;	// The current object of interest.
	CObjBase * m_pObjNext;	// In case the object get deleted.
	bool m_fInertToggle;		// We are now doing the inert items

	CSector * m_pSectorBase;	// Don't search the center sector 2 times.
	CSector * m_pSector;	// current Sector
	CRectMap m_rectSector;		// A rectangle containing our sectors we can search.
	int		m_iSectorCur;		// What is the current Sector index in m_rectSector
private:
	bool GetNextSector();
public:
	static const char *m_sClassName;
	explicit CWorldSearch( const CPointMap & pt, int iDist = 0 );
private:
	CWorldSearch(const CWorldSearch& copy);
	CWorldSearch& operator=(const CWorldSearch& other);

public:
	void SetAllShow( bool fView ) { m_fAllShow = fView; }
	void SetSearchSquare( bool fSquareSearch ) { m_fSearchSquare = fSquareSearch; }
	CChar * GetChar();
	CItem * GetItem();
};

inline CServTime CServTime::GetCurrentTime()	// static
{
	return( g_World.GetCurrentTime());
}

#endif
