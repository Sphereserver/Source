#ifndef _INC_CITEMMULTICUSTOM_H
#define _INC_CITEMMULTICUSTOM_H
#pragma once

class PacketHouseDesign;

class CItemMultiCustom : public CItemMulti
{
	// IT_MULTI_CUSTOM
public:
	static const char *m_sClassName;

	CItemMultiCustom(ITEMID_TYPE id, CItemBase *pItemDef);
	virtual ~CItemMultiCustom();

private:
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];

public:
	struct Component
	{
		CUOMultiItemRec2 m_item;
		short m_isStair;
		bool m_isFloor;
	};

private:
	typedef std::vector<Component *> ComponentsContainer;
	struct DesignDetails
	{
		DWORD m_dwRevision;
		ComponentsContainer m_vectorComponents;
		PacketHouseDesign *m_pData;
		DWORD m_dwDataRevision;
	};

	class CGrayMultiCustom : public CGrayMulti
	{
	public:
		CGrayMultiCustom() { };
		void LoadFrom(DesignDetails *pDesign);

	private:
		CGrayMultiCustom(const CGrayMultiCustom &copy);
		CGrayMultiCustom &operator=(const CGrayMultiCustom &other);
	};

private:
	DesignDetails m_designMain;
	DesignDetails m_designWorking;
	DesignDetails m_designBackup;
	DesignDetails m_designRevert;

	CClient *m_pArchitect;
	CGrayMultiCustom *m_pGrayMulti;
	CRectMap m_rectDesignArea;

	typedef std::map<ITEMID_TYPE, int> ValidItemsContainer;		// ItemID, FeatureMask
	static ValidItemsContainer sm_mapValidItems;

public:
	void BeginCustomize(CClient *pClient);
	void EndCustomize(bool fForce = false);

	void SwitchToLevel(CClient *pClient, BYTE bLevel);
	void CommitChanges(CClient *pClient = NULL);

	void AddItem(CClient *pClient, ITEMID_TYPE id, signed short x, signed short y, signed char z = SCHAR_MIN, short iStairID = 0);
	void AddStairs(CClient *pClient, ITEMID_TYPE id, signed short x, signed short y, signed char z = SCHAR_MIN, short iStairID = -1);
	void AddRoof(CClient *pClient, ITEMID_TYPE id, signed short x, signed short y, signed char z);

	void RemoveItem(CClient *pClient, ITEMID_TYPE id, signed short x, signed short y, signed char z);
	bool RemoveStairs(Component *pComponent);
	void RemoveRoof(CClient *pClient, ITEMID_TYPE id, signed short x, signed short y, signed char z);

	void SendVersionTo(CClient *pClient);
	void SendStructureTo(CClient *pClient);

	void BackupStructure();
	void RestoreStructure(CClient *pClient = NULL);
	void RevertChanges(CClient *pClient = NULL);
	void ResetStructure(CClient *pClient = NULL);

	DWORD GetRevision(const CClient *pClient = NULL) const;
	BYTE GetLevelCount();
	WORD GetStairID();
	size_t GetFixtureCount(DesignDetails *pDesign = NULL);
	size_t GetComponentsAt(signed short x, signed short y, signed char z, Component **ppComponents, DesignDetails *pDesign = NULL);

	const CGrayMultiCustom *GetMultiItemDefs();
	const CGRect GetDesignArea();
	static BYTE GetPlane(signed char z);
	static BYTE GetPlane(Component *pComponent);
	static signed char GetPlaneZ(BYTE bPlane);

private:
	const CPointMap GetComponentPoint(Component *pComponent) const;
	const CPointMap GetComponentPoint(signed short x, signed short y, signed char z) const;

	void CopyDesign(DesignDetails *designFrom, DesignDetails *designTo);

	static bool IsValidItem(ITEMID_TYPE id, CClient *pClient, bool fMulti);
	static bool LoadValidItems();

	virtual bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);
	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);	// execute command from script
	virtual void r_Write(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	virtual bool r_LoadVal(CScript &s);

private:
	CItemMultiCustom(const CItemMultiCustom &copy);
	CItemMultiCustom &operator=(const CItemMultiCustom &other);
};

#endif	// _INC_CITEMMULTICUSTOM_H
