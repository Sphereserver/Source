#ifndef _INC_CSECTORTEMPLATE_H
#define _INC_CSECTORTEMPLATE_H
#pragma once

class CCharsDisconnectList : public CGObList
{
public:
	static const char *m_sClassName;

	CCharsDisconnectList() { };

private:
	CCharsDisconnectList(const CCharsDisconnectList &copy);
	CCharsDisconnectList &operator=(const CCharsDisconnectList &other);
};

class CCharsActiveList : public CGObList
{
public:
	static const char *m_sClassName;

	CCharsActiveList();

private:
	size_t m_iClients;	// how many clients in this sector

protected:
	void OnRemoveOb(CGObListRec *pObRec);	// override this = called when removed from list

public:
	CServTime m_timeLastClient;		// age the sector based on last client here

	size_t HasClients() const { return m_iClients; }
	void AddCharToSector(CChar *pChar);
	void ClientAttach();
	void ClientDetach();

private:
	CCharsActiveList(const CCharsActiveList &copy);
	CCharsActiveList &operator=(const CCharsActiveList &other);
};

class CItemsList : public CGObList
{
	// Top level list of items
public:
	static const char *m_sClassName;

	CItemsList() { };

protected:
	void OnRemoveOb(CGObListRec *pObRec);	// override this = called when removed from list

public:
	static bool sm_fNotAMove;	// hack flag to prevent items from bouncing around too much

	void AddItemToSector(CItem *pItem);

private:
	CItemsList(const CItemsList &copy);
	CItemsList &operator=(const CItemsList &other);
};

class CObPointSortArray : public CGObSortArray<CPointSort *, long>
{
public:
	static const char *m_sClassName;

	CObPointSortArray() { };

public:
	int CompareKey(long id, CPointSort *pBase, bool fNoSpaces) const
	{
		UNREFERENCED_PARAMETER(fNoSpaces);
		ASSERT(pBase);
		return id - pBase->GetPointSortIndex();
	}

private:
	CObPointSortArray(const CObPointSortArray &copy);
	CObPointSortArray &operator=(const CObPointSortArray &other);
};

class CSectorBase		// world sector
{
public:
	static const char *m_sClassName;

	CSectorBase();
	virtual ~CSectorBase();

protected:
	int m_index;
	int m_map;

private:
	typedef std::map<long, CGrayMapBlock *> MapBlockCache;
	MapBlockCache m_MapBlockCache;

public:
	CCharsActiveList m_Chars_Active;			// CChar(s) active in this CSector
	CCharsDisconnectList m_Chars_Disconnect;	// Dead NPCs, etc
	CItemsList m_Items_Timer;					// CItem(s) in this CSector (timed)
	CItemsList m_Items_Inert;					// CItem(s) in this CSector (not timed)
	CRegionLinks m_RegionLinks;					// CRegionBase(s) in this CSector
	CObPointSortArray m_Teleports;				// CTeleport array
	static int m_iMapBlockCacheTime;
	BYTE m_bFlags;

public:
	void Init(int index, int newmap);

	bool static CheckMapBlockTime(const MapBlockCache::value_type &elem);
	void ClearMapBlockCache();
	void CheckMapBlockCache();

	const CGrayMapBlock *GetMapBlock(const CPointMap &pt);
	bool IsInDungeon() const;
	CRegionBase *GetRegion(const CPointBase &pt, BYTE bType) const;
	size_t GetRegions(const CPointBase &pt, BYTE bType, CRegionLinks &rList) const;
	bool UnLinkRegion(CRegionBase *pRegionOld);
	bool LinkRegion(CRegionBase *pRegionNew);

	CTeleport *GetTeleport(const CPointMap &pt) const;
	bool AddTeleport(CTeleport *pTeleport);

	CPointMap GetBasePoint() const;
	CRectMap GetRect() const;

	int GetIndex() const { return m_index; }
	int GetMap() const { return m_map; }

#define SECF_NoSleep	0x1
#define SECF_InstaSleep	0x2

	bool IsFlagSet(BYTE bFlag) const
	{
		return (m_bFlags & bFlag);
	}

private:
	CSectorBase(const CSectorBase &copy);
	CSectorBase &operator=(const CSectorBase &other);
};

#endif // _INC_CSECTORTEMPLATE_H
