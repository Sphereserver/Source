//
// CSectorTemplate.h
//
#ifndef _INC_CSECTOR_H
#define _INC_CSECTOR_H
#pragma once

class CCharsDisconnectList : public CGObList
{
public:
	static const char *m_sClassName;

public:
	CCharsDisconnectList() { };

private:
	CCharsDisconnectList(const CCharsDisconnectList& copy);
	CCharsDisconnectList& operator=(const CCharsDisconnectList& other);
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

public:
	CCharsActiveList();

private:
	CCharsActiveList(const CCharsActiveList& copy);
	CCharsActiveList& operator=(const CCharsActiveList& other);
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

public:
	CItemsList() { };

private:
	CItemsList(const CItemsList& copy);
	CItemsList& operator=(const CItemsList& other);
};

class CObPointSortArray : public CGObSortArray< CPointSort*, long >
{
public:
	static const char *m_sClassName;

	// Find a point fast.
	int CompareKey( long id, CPointSort* pBase, bool fNoSpaces ) const
	{
		UNREFERENCED_PARAMETER(fNoSpaces);
		ASSERT( pBase );
		return( id - pBase->GetPointSortIndex());
	}

public:
	CObPointSortArray() { };

private:
	CObPointSortArray(const CObPointSortArray& copy);
	CObPointSortArray& operator=(const CObPointSortArray& other);
};

class CSectorBase		// world sector
{
protected:
	int	m_index;		// sector index
	int m_map;			// sector map

private:
	typedef std::map<long, CGrayMapBlock*>	MapBlockCache;
	MapBlockCache							m_MapBlockCache;
public:
	static const char *m_sClassName;
	CObPointSortArray	m_Teleports;		//	CTeleport array
	CRegionLinks		m_RegionLinks;		//	CRegionBase(s) in this CSector
public:
	CCharsActiveList		m_Chars_Active;		// CChar(s) activte in this CSector.
	CCharsDisconnectList	m_Chars_Disconnect;	// dead NPCs, etc
	CItemsList m_Items_Timer;				// CItem(s) in this CSector that need timers.
	CItemsList m_Items_Inert;				// CItem(s) in this CSector. (no timer required)
public:
	CSectorBase();
	virtual ~CSectorBase();

private:
	CSectorBase(const CSectorBase& copy);
	CSectorBase& operator=(const CSectorBase& other);

public:
	void Init(int index, int newmap);

	// Location map units.
	int GetIndex() const { return m_index; }
	int GetMap() const { return m_map; }
	CPointMap GetBasePoint() const;
	CRectMap GetRect() const;
	bool IsInDungeon() const;

	bool static CheckMapBlockTime( const MapBlockCache::value_type& Elem );
	void ClearMapBlockCache();
	void CheckMapBlockCache();
	static int m_iMapBlockCacheTime;
	const CGrayMapBlock * GetMapBlock( const CPointMap & pt );

	// CRegionBase
	CRegionBase * GetRegion( const CPointBase & pt, DWORD dwType ) const;
	int GetRegions( const CPointBase & pt, DWORD dwType, CRegionLinks & rlist ) const;

	bool UnLinkRegion( CRegionBase * pRegionOld );
	bool LinkRegion( CRegionBase * pRegionNew );

	// CTeleport(s) in the region.
	CTeleport * GetTeleport( const CPointMap & pt ) const;
	CTeleport * GetTeleport2d( const CPointMap & pt ) const;
	bool AddTeleport( CTeleport * pTeleport );
};

#endif // _INC_CSECTOR_H
