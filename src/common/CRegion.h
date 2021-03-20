#ifndef _INC_CREGION_H
#define _INC_CREGION_H
#pragma once

#include "CRect.h"

inline DIR_TYPE GetDirTurn(DIR_TYPE dir, int iOffset)
{
	// Turn in a direction
	// -1 = to the left
	// +1 = to the right
	ASSERT((iOffset + DIR_QTY + dir) >= 0);
	iOffset += DIR_QTY + dir;
	iOffset %= DIR_QTY;
	return static_cast<DIR_TYPE>(iOffset);
}

///////////////////////////////////////////////////////////
// CGRegion

class CGRegion
{
	// A bunch of rectangles forming an area
public:
	static const char *m_sClassName;

	CGRegion();
	virtual ~CGRegion() { };

public:
	CGRect m_rectUnion;
	CGTypedArray<CGRect, const CGRect &> m_Rects;

public:
	virtual bool AddRegionRect(const CGRect &rect);
	CGRect &GetRegionRect(size_t i);
	const CGRect &GetRegionRect(size_t i) const;
	size_t GetRegionRectCount() const;
	CPointBase GetRegionCorner(DIR_TYPE dir = DIR_QTY) const;

	bool IsInside(const CGRect &rect) const;
	bool IsInside(const CGRegion *pRegion) const;
	bool IsInside2d(const CPointBase &pt) const;
	bool IsOverlapped(const CGRect &rect) const;
	bool IsOverlapped(const CGRegion *pRegion) const;
	bool IsEqualRegion(const CGRegion *pRegion) const;

	void EmptyRegion()
	{
		m_rectUnion.SetRectEmpty();
		m_Rects.RemoveAll();
	}
	bool IsRegionEmpty() const
	{
		return m_rectUnion.IsRectEmpty();
	}

	CSector *GetSector(int i) const
	{
		return m_rectUnion.GetSector(i);
	}

private:
	CGRegion(const CGRegion &copy);
	CGRegion &operator=(const CGRegion &other);
};

///////////////////////////////////////////////////////////
// CRegionBase

enum RTRIG_TYPE
{
	RTRIG_CLIPERIODIC = 1,
	RTRIG_ENTER,
	RTRIG_EXIT,
	RTRIG_REGPERIODIC,
	RTRIG_STEP,
	RTRIG_QTY
};

class CRegionBase : public CResourceDef, public CGRegion
{
	// Region of the world of arbitrary size and location (RES_AREA, RES_ROOM)
	// Can be made of multiple rectangles
public:
	static const char *m_sClassName;
	static const LPCTSTR sm_szLoadKeys[];
	static const LPCTSTR sm_szVerbKeys[];
	static const LPCTSTR sm_szTrigName[RTRIG_QTY + 1];

	explicit CRegionBase(RESOURCE_ID rid, LPCTSTR pszName = NULL);
	virtual ~CRegionBase();

private:
	CGString m_sName;
	CGString m_sGroup;

	#define REGION_ANTIMAGIC_ALL		0x000001	// Can't cast any magic here
	#define REGION_ANTIMAGIC_RECALL_IN	0x000002	// Can't recall in or teleport here
	#define REGION_ANTIMAGIC_RECALL_OUT	0x000004	// Can't recall out of here
	#define REGION_ANTIMAGIC_GATE		0x000008	// Can't cast gate travel here
	#define REGION_ANTIMAGIC_TELEPORT	0x000010	// Can't teleport here
	#define REGION_ANTIMAGIC_DAMAGE		0x000020	// Can't cast damage spells here
	#define REGION_FLAG_SHIP			0x000040	// Ship region (accept ship commands)
	#define REGION_FLAG_NOBUILDING		0x000080	// No building allowed here
	#define REGION_FLAG_ANNOUNCE		0x000200	// Announce to all who enter
	#define REGION_FLAG_INSTA_LOGOUT	0x000400	// Instant logout
	#define REGION_FLAG_UNDERGROUND		0x000800	// Dungeon region (no weather)
	#define REGION_FLAG_NODECAY			0x001000	// Items dropped on ground will not decay
	#define REGION_FLAG_SAFE			0x002000	// Region is safe from all harm
	#define REGION_FLAG_GUARDED			0x004000	// Region is protected by guards (guards name = REGION.TAG.GUARDOWNER)
	#define REGION_FLAG_NO_PVP			0x008000	// No PVP allowed here
	#define REGION_FLAG_ARENA			0x010000	// Everyone will have neutral notoriety to fight without being flagged as murderer/criminal
	DWORD m_dwFlags;

public:
	CPointMap m_pt;
	int m_iModified;
	int m_iLinkedSectors;
	CResourceRefArray m_Events;
	CVarDefMap m_TagDefs;
	CVarDefMap m_BaseDefs;		// new variable storage system

public:
	virtual bool RealizeRegion();
	void UnRealizeRegion();
	bool MakeRegionName();

	bool AddRegionRect(const CRectMap &rect);
	bool SetRegionRect(const CRectMap &rect)
	{
		EmptyRegion();
		return AddRegionRect(rect);
	}

	void SetName(LPCTSTR pszName);
	LPCTSTR GetName() const
	{
		return m_sName;
	}
	const CGString &GetNameStr() const
	{
		return m_sName;
	}
	virtual bool IsValid() const
	{
		return m_sName.IsValid();
	}

	#define REGMOD_FLAGS	0x01
	#define REGMOD_EVENTS	0x02
	#define REGMOD_TAGS		0x04
	#define REGMOD_NAME		0x08
	#define REGMOD_GROUP	0x10
	void SetModified(int iFlag);

	void SetRegionFlags(DWORD dwFlags)
	{
		m_dwFlags |= dwFlags;
	}
	DWORD GetRegionFlags() const
	{
		return m_dwFlags;
	}
	bool IsFlag(DWORD dwFlags) const
	{
		return (m_dwFlags & dwFlags);
	}
	void ToggleRegionFlags(DWORD dwFlags, bool fSet)
	{
		if ( fSet )
			m_dwFlags |= dwFlags;
		else
			m_dwFlags &= ~dwFlags;
		SetModified(REGMOD_FLAGS);
	}

	bool IsGuarded() const;
	bool CheckAntiMagic(SPELL_TYPE spell) const;
	TRIGRET_TYPE OnRegionTrigger(CTextConsole *pChar, RTRIG_TYPE trig);

	void r_WriteBase(CScript &s);
	virtual void r_WriteBody(CScript &s, LPCTSTR pszPrefix);
	virtual void r_WriteModified(CScript &s);
	virtual void r_Write(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	virtual bool r_LoadVal(CScript &s);
	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);	// execute command from script

public:
	LPCTSTR GetDefStr(LPCTSTR pszKey, bool fZero = false) const
	{
		return m_BaseDefs.GetKeyStr(pszKey, fZero);
	}
	void SetDefStr(LPCTSTR pszKey, LPCTSTR pszVal, bool fQuoted = false, bool fZero = true)
	{
		m_BaseDefs.SetStr(pszKey, fQuoted, pszVal, fZero);
	}

	INT64 GetDefNum(LPCTSTR pszKey) const
	{
		return m_BaseDefs.GetKeyNum(pszKey);
	}
	void SetDefNum(LPCTSTR pszKey, INT64 iVal, bool fZero = true)
	{
		m_BaseDefs.SetNum(pszKey, iVal, fZero);
	}

	void DeleteDef(LPCTSTR pszKey)
	{
		m_BaseDefs.DeleteKey(pszKey);
	}

private:
	bool SendSectorsVerb(LPCTSTR pszVerb, LPCTSTR pszArgs, CTextConsole *pSrc);

private:
	CRegionBase(const CRegionBase &copy);
	CRegionBase &operator=(const CRegionBase &other);
};

///////////////////////////////////////////////////////////
// CRegionLinks

class CRegionLinks : public CGPtrTypeArray<CRegionBase *>
{
	// Just named class for this, maybe something here later
public:
	CRegionLinks() { };

private:
	CRegionLinks(const CRegionLinks &copy);
	CRegionLinks &operator=(const CRegionLinks &other);
};

///////////////////////////////////////////////////////////
// CRegionWorld

class CRandGroupDef;

class CRegionWorld : public CRegionBase
{
	// A region with extra properties
public:
	static const char *m_sClassName;
	static const LPCTSTR sm_szLoadKeys[];
	static const LPCTSTR sm_szVerbKeys[];

	explicit CRegionWorld(RESOURCE_ID rid, LPCTSTR pszName = NULL);
	virtual ~CRegionWorld() { };

public:
	const CRandGroupDef *FindNaturalResource(/*IT_TYPE*/ int iType) const;

public:
	virtual bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);
	virtual void r_WriteBody(CScript &s, LPCTSTR pszPrefix);
	virtual void r_WriteModified(CScript &s);
	virtual void r_Write(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	virtual bool r_LoadVal(CScript &s);
	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);	// execute command from script

private:
	CRegionWorld(const CRegionWorld &copy);
	CRegionWorld &operator=(const CRegionWorld &other);
};

///////////////////////////////////////////////////////////
// CTeleport

class CTeleport : public CPointSort
{
	// World static teleporters (RES_TELEPORTERS)
public:
	static const char *m_sClassName;

	explicit CTeleport(TCHAR *pszArgs);
	explicit CTeleport(const CPointMap &pt) : CPointSort(pt)
	{
		ASSERT(pt.IsValidPoint());
		m_ptDst = pt;
		m_fNPC = false;
	}
	virtual ~CTeleport() { };

public:
	CPointMap m_ptDst;
	bool m_fNPC;

public:
	bool RealizeTeleport();

private:
	CTeleport(const CTeleport &copy);
	CTeleport &operator=(const CTeleport &other);
};

///////////////////////////////////////////////////////////
// CStartLoc

class CStartLoc
{
	// Starting locations to show on client character creation menu (RES_STARTS)
public:
	static const char *m_sClassName;

	explicit CStartLoc(LPCTSTR pszCity)
	{
		m_sCity = pszCity;
		iClilocDescription = 1149559;	// empty cliloc
	}

public:
	CGString m_sCity;
	CGString m_sLocation;
	CPointMap m_pt;
	int iClilocDescription;

private:
	CStartLoc(const CStartLoc &copy);
	CStartLoc &operator=(const CStartLoc &other);
};

#endif // _INC_CREGION_H
