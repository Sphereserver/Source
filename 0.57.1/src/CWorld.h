//
// CWorld.h
//

#ifndef _INC_CWORLD_H
#define _INC_CWORLD_H
#pragma once

#include "common/mtrand/mtrand.h"
#include "CSector.h"

#define FREE_UIDS_SIZE	500000	//	the list of free empty slots in the uids list should contain these elements

#ifndef _WIN32
#ifdef CLOCKS_PER_SEC
#undef CLOCKS_PER_SEC
#endif	// CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 1000	// must be converted from some internal clock.
#endif

class CTimedFunctionHandler
{
public:
	struct TimedFunction
	{
		int		elapsed;
		char	funcname[1024];
		UID 	uid;
	};

private:
	std::vector<TimedFunction *> m_timedFunctions[TICK_PER_SEC];
	int m_curTick;
	std::vector<TimedFunction *> m_tFrecycled;
	std::vector<TimedFunction *> m_tFqueuedToBeAdded;
	bool m_isBeingProcessed;

public:
	static const char *m_sClassName;

public:
	void OnTick();
	void r_Write( CScript & s );

	int Load( const char *pszName, bool fQuoted, const char *pszVal );
	void Add( UID uid, int numSeconds, LPCTSTR funcname );
	void Erase( UID uid );
};

extern class CWorld : public CScriptObj
{
	// the world. Stuff saved in *World.SCP
private:
	// Clock stuff. how long have we been running ? all i care about.
	CServTime m_timeClock;		// the current relative tick time  (in TICK_PER_SEC)
	DWORD  m_Clock_PrevSys;		// System time of the last OnTick(). (CLOCKS_PER_SEC)

	// Special purpose timers.
	CServTime	m_timeSector;		// next time to do sector stuff.
	CServTime	m_timeCallUserFunc;	// when to call next user func
	int		m_Sector_Pulse;		// Slow some stuff down that doesn't need constant processing.

protected:
	CGObArray<CObjBase*> m_UIDs;	// all the UID's in the World. CChar and CItem.

	DWORD	*m_FreeUIDs;		//	list of free uids available
	DWORD	m_FreeOffset;		//	offset of the first free element

public:
	int			m_iSaveStage;	// Current stage of the background save.
	CServTime	m_timeRespawn;	// when to res dead NPC's ?
	CServTime	m_timeSave;		// when to auto save ?

	CGObList m_ObjNew;			// Obj created but not yet placed in the world.
	CGObList m_ObjDelete;		// Objects to be deleted.

	CScript m_FileData;			// Save or Load file.
	CScript m_FileWorld;		// Save or Load file.
	CScript m_FilePlayers;		// Save of the players chars.
	CScript	m_FileStatics;		// statics data
	CScript m_FileAccounts;		// accounts data
	vector<long>	m_LostPlayers;	// lost of saved players moved to already saved sectors, thus not saved
	bool	m_fSaveParity;		// has the sector been saved relative to the char entering it ?
	MTRand	m_Rand;

public:
	static const char *m_sClassName;
	// World data.
	CSector	**m_Sectors;
	int		m_SectorsQty;

public:
	int		m_iSaveCountID;	// Current archival backup id. Whole World must have this same stage id
	CServTime m_timeStartup;		// When did the system restore load/save ?

	UID m_uidLastNewItem;	// for script access.
	UID m_uidLastNewChar;	// for script access.
	UID m_uidObj;			// for script access - auxiliary obj
	UID m_uidNew;			// for script access - auxiliary obj

	CGObList m_GMPages;		// Current outstanding GM pages. (CGMPage)

	
	CGPtrTypeArray<CItemStone*> m_Stones;	// links to leige/town stones. (not saved array)
	CGObList m_Parties;	// links to all active parties. CPartyDef

	static LPCTSTR const sm_szLoadKeys[];
	CGPtrTypeArray <CItemTypeDef *> m_TileTypes;

	// TimedFunction Container/Wrapper
	CTimedFunctionHandler m_TimedFunctions;

private:
	bool LoadFile( LPCTSTR pszName, bool fError = true );

public:
	// time
	DWORD GetSystemClock();
	void InitTime(long lTimeBase);
	bool Advance();

	// UID Managenent
	DWORD GetUIDCount() const;
	CObjBase * FindUID(DWORD dwIndex) const;
	void FreeUID(DWORD dwIndex);
	DWORD AllocUID( DWORD dwIndex, CObjBase * pObj );

	int FixObjTry( CObjBase * pObj, int iUID = 0 );
	int  FixObj( CObjBase * pObj, int iUID = 0 );

	bool SaveStage();

	CWorld();
	~CWorld();
	void Init();

	CSector *GetSector( int map, int i );	// gets sector # from one map

	// Time
	CServTime GetCurrentTime() const;
	int GetTimeDiff( CServTime time ) const;

	DWORD GetGameWorldTime( CServTime basetime ) const;
	DWORD GetGameWorldTime() const	// return game world minutes
	{
		return( GetGameWorldTime( GetCurrentTime()));
	}

	// CSector World Map stuff.

	void GetHeightPoint( const CPointMap & pt, CGrayMapBlockState & block, bool fHouseCheck = false );
	signed char GetHeightPoint( const CPointBase & pt, WORD & wBlockFlags, bool fHouseCheck = false ); // Height of player who walked to X/Y/OLDZ

	void GetHeightPoint_New( const CPointMap & pt, CGrayMapBlockState & block, bool fHouseCheck = false );
	signed char GetHeightPoint_New( const CPointBase & pt, WORD & wBlockFlags, bool fHouseCheck = false );

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

	CPointMap FindItemTypeNearby( const CPointMap & pt, IT_TYPE iType, int iDistance = 0, bool bLimitZ = false );
	bool IsItemTypeNear( const CPointMap & pt, IT_TYPE iType, int iDistance = 0 );
	CItem * CheckNaturalResource( const CPointMap & pt, IT_TYPE Type, bool fTest = true, CChar * pCharSrc = NULL );

	void r_Write( CScript & s );
	bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );
	bool r_LoadVal( CScript & s ) ;
	bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );

	void OnTick();
	void GarbageCollection();

	void Speak( const CObjBaseTemplate * pSrc, LPCTSTR pText, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD );
	void SpeakUNICODE( const CObjBaseTemplate * pSrc, const NCHAR * pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang );

	void __cdecl Broadcast( LPCTSTR pMsg, ...);
	void Explode( CChar * pSrc, CPointMap pt, int iDist, int iDamage, WORD wFlags = DAMAGE_GENERAL | DAMAGE_HIT_BLUNT );

	bool LoadAll();

	LPCTSTR GetName() const { return( "World" ); }
} g_World;

class CObjBase;
class CWorldSearch	// define a search of the world.
{
private:
	const CPointMap m_pt;		// Base point of our search.
	const int m_iDist;			// How far from the point are we interested in
	bool m_fAllShow;		// Include Even inert items.

	CObjBase * m_pObj;	// The current object of interest.
	CObjBase * m_pObjNext;	// In case the object get deleted.
	bool m_fInertToggle;		// We are now doing the inert items

	CSector * m_pSectorBase;	// Don't search the center sector 2 times.
	CSector * m_pSector;	// current Sector
	CRectMap m_rectSector;		// A rectangle containing our sectors we can search.
	int		m_iSectorCur;		// What is the current Sector index in m_rectSector

public:
	static const char *m_sClassName;
	CWorldSearch(const CPointMap &pt, int iDist = 0, const CObjBase *skipObject = NULL);
	void SetAllShow( bool fView ) { m_fAllShow = fView; }
	CChar * GetChar();
	CItem * GetItem();

private:
	bool GetNextSector();

	const CObjBase *m_skipObject;
};

inline CServTime CServTime::GetCurrentTime()	// static
{
	return( g_World.GetCurrentTime());
}

#endif
