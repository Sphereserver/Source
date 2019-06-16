#ifndef _INC_CWORLD_H
#define _INC_CWORLD_H
#pragma once

#include "../common/mtrand/mtrand.h"

///////////////////////////////////////////////////////////
// CWorldThread

#define FREE_UIDS_SIZE	500000	// list of free empty slots in the UIDs list should contain these elements

class CWorldThread
{
	// A regional thread of execution. Hold all the objects in my region
	// as well as those just created here (but may not be here anymore)

public:
	static const char *m_sClassName;

	CWorldThread();
	virtual ~CWorldThread();

protected:
	CGObArray<CObjBase *> m_UIDs;	// all UID's placed on world (CChar/CItem)
	DWORD m_dwUIDIndexLast;			// last UID index allocated

	DWORD *m_dwFreeUIDs;			// list of free UIDs available
	DWORD m_dwFreeOffset;			// offset of the first free element

public:
	CGObList m_ObjNew;				// Object created but not yet placed in the world
	CGObList m_ObjDelete;			// Objects to be deleted

	// Background save (does this belong here?)
	CScript m_FileData;				// global vars
	CScript m_FileWorld;			// objects placed on world
	CScript m_FilePlayers;			// players chars
	CScript m_FileMultis;			// custom multis
	bool m_fSaveParity;				// the sector has been saved relative to the char entering it?
	MTRand m_Rand;

public:
	// Backgound save
	bool IsSaving() const;

	// UID managenent
	#define UID_PLACE_HOLDER	reinterpret_cast<CObjBase *>(UID_UNUSED)
	CObjBase *FindUID(DWORD dwIndex) const;
	DWORD GetUIDCount() const;
	void FreeUID(DWORD dwIndex);
	DWORD AllocUID(DWORD dwIndex, CObjBase *pObj);

	int FixObjTry(CObjBase *pObj, DWORD uid = 0);
	int FixObj(CObjBase *pObj, DWORD uid = 0);

	void SaveThreadClose();
	void GarbageCollection_UIDs();
	void GarbageCollection_New();

	void CloseAllUIDs();

private:
	CWorldThread(const CWorldThread &copy);
	CWorldThread &operator=(const CWorldThread &other);
};

///////////////////////////////////////////////////////////
// CWorldClock

#ifndef _WIN32
	#ifdef CLOCKS_PER_SEC
		#undef CLOCKS_PER_SEC
	#endif
	#define CLOCKS_PER_SEC	1000	// must be converted from some internal clock
#endif

class CWorldClock
{
public:
	static const char *m_sClassName;

	CWorldClock()
	{
		Init();
	}

private:
	UINT64 m_SystemClock_Prev;		// system time of the last OnTick() (in CLOCKS_PER_SEC)
	CServTime m_TimeClock;			// the current relative tick time (in TICK_PER_SEC)

public:
	void Init();
	void InitTime(UINT64 uTimeBase);
	bool Advance();

	static UINT64 GetSystemClock();		// time in CLOCKS_PER_SEC
	CServTime GetCurrentTime() const	// time in TICK_PER_SEC
	{
		return m_TimeClock;
	}

private:
	CWorldClock(const CWorldClock &copy);
	CWorldClock &operator=(const CWorldClock &other);
};

///////////////////////////////////////////////////////////
// CTimedFunctionHandler

class CTimedFunctionHandler
{
public:
	static const char *m_sClassName;

	CTimedFunctionHandler();

private:
	struct TimedFunction
	{
		int elapsed;
		char funcname[128];
		CGrayUID uid;
	};

	int m_iCurTick;
	bool m_fBeingProcessed;
	std::vector<TimedFunction *> m_timedFunctions[TICK_PER_SEC];
	std::vector<TimedFunction *> m_tFrecycled;
	std::vector<TimedFunction *> m_tFqueuedToBeAdded;

public:
	void OnTick();
	void r_Write(CScript &s);

	int Load(const char *pszName, bool fQuoted, const char *pszVal);
	void Add(CGrayUID uid, int iNumSeconds, LPCTSTR pszFuncName);
	void Erase(CGrayUID uid);
	void Stop(CGrayUID uid, LPCTSTR pszFuncName);
	TRIGRET_TYPE Loop(LPCTSTR pszFuncName, int iLoopsMade, CScriptLineContext StartContext, CScript &s, CTextConsole *pSrc, CScriptTriggerArgs *pArgs, CGString *sResult);
	int IsTimer(CGrayUID uid, LPCTSTR pszFuncName);

private:
	CTimedFunctionHandler(const CTimedFunctionHandler &copy);
	CTimedFunctionHandler &operator=(const CTimedFunctionHandler &other);
};

///////////////////////////////////////////////////////////
// CWorld

enum IMPFLAGS_TYPE	// IMPORT/EXPORT flags
{
	IMPFLAGS_NOTHING	= 0x00,
	IMPFLAGS_ITEMS		= 0x01,
	IMPFLAGS_CHARS		= 0x02,
	IMPFLAGS_SELF		= 0x04,
	IMPFLAGS_RELATIVE	= 0x10,		// relative distance to object (x,y)
	IMPFLAGS_ACCOUNT	= 0x20		// import/export just this account (all chars and everything they're carrying)
};

extern class CWorld : public CScriptObj, public CWorldThread
{
	// Stuff saved in *world.scp

public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szLoadKeys[];
	LPCTSTR GetName() const
	{
		return "World";
	}

	CWorld();
	~CWorld();

private:
	ULONGLONG m_savetimer;					// time it took to finish world save
	int m_iSaveStage;						// current background save stage
	bool m_fSaveNotificationSent;			// notify all clients before start an world save

	CServTime m_timeSector;					// time to start next sector stuff
	CServTime m_timeSave;					// time to start next world save
	CServTime m_timeRespawn;				// time to resurrect dead NPCs on sector
	CServTime m_timeCallUserFunc;			// time to call next user functions

	CSector **m_Sectors;
	size_t m_SectorsQty;
	size_t m_Sector_Pulse;					// slow down some stuff that doesn't need constant processing

	CWorldClock m_Clock;					// current relative time (in TICK_PER_SEC)

public:
	CServTime m_timeStartup;				// time since the server had started

	long m_iSaveCountID;					// current world save numeric ID ("SAVECOUNT")
	long m_iPrevBuild;						// previous server build version ("PREVBUILD")
	long m_iLoadVersion;					// current server build version ("VERSION")

	CGrayUID m_uidLastNewItem;				// last new created item (for script access)
	CGrayUID m_uidLastNewChar;				// last new created char (for script access)
	CGrayUID m_uidNew;						// current obj stored on NEW ref (for script access)
	CGrayUID m_uidObj;						// current obj stored on OBJ ref (for script access)

	CGPtrTypeArray<CItemTypeDef *> m_TileTypes;		// list of terrain typedefs
	CGPtrTypeArray<CItemStone *> m_Stones;			// list of guild/town stones (not saved array)
	CGObList m_Parties;								// list of active parties (CPartyDef)
	CGObList m_GMPages;								// list of GM pages (CGMPage)

	CTimedFunctionHandler m_TimedFunctions;			// list of timed functions to be called ("TIMERF")
	CGPtrTypeArray<CObjBase *> m_ObjStatusUpdates;	// list of objects that doesn't receive OnTickStatusUpdate() ticks (eg: placed inside containers) and need to call it manually

private:
	static void GetBackupName(CGString &sBuffer, LPCTSTR pszBaseDir, TCHAR pszType, int iSaveCount);
	bool LoadFile(LPCTSTR pszFileName);
	bool LoadWorld();

	bool SaveTry(bool fForceImmediate);
	bool SaveStage();
	bool SaveForce();

public:
	void Init();

	CSector *GetSector(int iMap, int iSector);

	// Time
	CServTime GetCurrentTime() const
	{
		return m_Clock.GetCurrentTime();	// time in TICK_PER_SEC
	}
	INT64 GetTimeDiff(CServTime time) const
	{
		// How long till this event (negative = in the past)
		return time.GetTimeDiff(GetCurrentTime());		// time in TICK_PER_SEC
	}

	#define TRAMMEL_SYNODIC_PERIOD		105	// in game world minutes
	#define FELUCCA_SYNODIC_PERIOD		840	// in game world minutes
	#define TRAMMEL_FULL_BRIGHTNESS		2	// light units LIGHT_BRIGHT
	#define FELUCCA_FULL_BRIGHTNESS		6	// light units LIGHT_BRIGHT
	unsigned int GetMoonPhase(bool fMoonIndex = false) const;
	CServTime GetNextNewMoon(bool fMoonIndex) const;

	DWORD GetGameWorldTime(CServTime basetime) const;
	DWORD GetGameWorldTime() const
	{
		// Return game world minutes
		return GetGameWorldTime(GetCurrentTime());
	}

	// CSector world map stuff
	void GetHeightPoint(const CPointMap &pt, CGrayMapBlockState &block, bool fHouseCheck = false);
	void GetHeightPoint2(const CPointMap &pt, CGrayMapBlockState &block, bool fHouseCheck = false);
	signed char GetHeightPoint(const CPointBase &pt, DWORD &dwBlockFlags, bool fHouseCheck = false);
	signed char GetHeightPoint2(const CPointBase &pt, DWORD &dwBlockFlags, bool fHouseCheck = false);

	void GetFixPoint(const CPointMap &pt, CGrayMapBlockState &block);

	CItemTypeDef *GetTerrainItemTypeDef(DWORD dwIndex);
	IT_TYPE GetTerrainItemType(DWORD dwIndex);

	const CGrayMapBlock *GetMapBlock(const CPointMap &pt)
	{
		return pt.GetSector()->GetMapBlock(pt);
	}
	const CUOMapMeter *GetMapMeter(const CPointMap &pt) const
	{
		// Get height of given map coordinates
		const CGrayMapBlock *pMapBlock = pt.GetSector()->GetMapBlock(pt);
		if ( !pMapBlock )
			return NULL;
		return pMapBlock->GetTerrain(UO_BLOCK_OFFSET(pt.m_x), UO_BLOCK_OFFSET(pt.m_y));
	}

	CPointMap FindItemTypeNearby(const CPointMap &pt, IT_TYPE type, int iDist = 0, bool fCheckMulti = false, bool fLimitZ = false);
	bool IsItemTypeNear(const CPointMap &pt, IT_TYPE type, int iDist = 0, bool fCheckMulti = false, bool fLimitZ = false);

	CPointMap FindTypeNear_Top(const CPointMap &pt, IT_TYPE type, int iDist = 0);
	bool IsTypeNear_Top(const CPointMap &pt, IT_TYPE type, int iDist = 0);

	CItem *CheckNaturalResource(const CPointMap &pt, IT_TYPE type, bool fTest = true, CChar *pCharSrc = NULL);

	static bool OpenScriptBackup(CScript &s, LPCTSTR pszBaseDir, LPCTSTR pszBaseName, int iSaveCount);

	void r_Write(CScript &s);
	bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	bool r_LoadVal(CScript &s);
	bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);

	void OnTick();

	void GarbageCollection();
	void Restock();
	void RespawnDeadNPCs();
	void ReloadMultis();

	void Speak(const CObjBaseTemplate *pSrc, LPCTSTR pszText, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD);
	void SpeakUNICODE(const CObjBaseTemplate *pSrc, const NCHAR *pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang);

	void Broadcast(LPCTSTR pszMsg);
	void __cdecl Broadcastf(LPCTSTR pszMsg, ...) __printfargs(2, 3);

	bool Export(LPCTSTR pszFileName, const CChar *pSrc, WORD wModeFlags = IMPFLAGS_ITEMS, int iDist = SHRT_MAX, int dx = 0, int dy = 0);
	bool Import(LPCTSTR pszFileName, const CChar *pSrc, WORD wModeFlags = IMPFLAGS_ITEMS, int iDist = SHRT_MAX, TCHAR *pszArg1 = NULL, TCHAR *pszArg2 = NULL);
	bool Save(bool fForceImmediate);
	void SaveStatics();
	bool LoadAll();
	bool DumpAreas(CTextConsole *pSrc, LPCTSTR pszFileName);
	void Close();

private:
	CWorld(const CWorld &copy);
	CWorld &operator=(const CWorld &other);
} g_World;

///////////////////////////////////////////////////////////
// CWorldSearch

class CWorldSearch
{
	// Create an search area on world to find objects

public:
	static const char *m_sClassName;
	explicit CWorldSearch(const CPointMap &pt, int iDist = 0);

private:
	const CPointMap m_pt;		// base search point (center)
	const int m_iDist;			// max distance to search
	bool m_fAllShow;			// include inert items and disconnected chars

	CObjBase *m_pObj;			// current object
	CObjBase *m_pObjNext;		// next object (in case the current object get deleted)
	bool m_fInertToggle;		// search on inert items

	CSector *m_pSectorBase;		// store the base sector to avoid search this sector twice on loop
	CSector *m_pSector;			// current sector
	CRectMap m_rectSector;		// rectangle containing sectors to search
	int m_iSectorCur;			// current sector index in m_rectSector

private:
	bool GetNextSector();

public:
	CChar * GetChar();
	CItem *GetItem();

	void SetAllShow(bool fAllShow)
	{
		m_fAllShow = fAllShow;
	}
	void RestartSearch()
	{
		// Setting current obj to NULL will restart the search
		m_pObj = NULL;
	}

private:
	CWorldSearch(const CWorldSearch &copy);
	CWorldSearch &operator=(const CWorldSearch &other);
};

inline CServTime CServTime::GetCurrentTime()	// static
{
	return g_World.GetCurrentTime();
}

#endif	// _INC_CWORLD_H
