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
		int m_iStairID;
		bool m_fFloor;
	};

private:
	typedef std::vector<Component *> ComponentsContainer;
	struct DesignDetails
	{
		DWORD m_dwRevision;
		ComponentsContainer m_vectorComponents;
		PacketHouseDesign *m_pPacket;
		DWORD m_dwPacketRevision;
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

	CClient *m_pArchitect;
	CGrayMultiCustom *m_pGrayMulti;
	CRectMap m_rectDesignArea;

	typedef std::map<ITEMID_TYPE, int> ValidItemsContainer;		// ItemID, FeatureMask
	static ValidItemsContainer sm_mapValidItems;

public:
	void BeginCustomize(CClient *pClient);
	void EndCustomize(bool fForce = false);

	void AddItem(CClient *pClient, ITEMID_TYPE id, signed short x, signed short y, signed char z = SCHAR_MIN, int iStairID = 0);
	void RemoveItem(CClient *pClient, ITEMID_TYPE id, signed short x, signed short y, signed char z);

	void AddStairs(CClient *pClient, ITEMID_TYPE id, signed short x, signed short y, signed char z = SCHAR_MIN);

	void AddRoof(CClient *pClient, ITEMID_TYPE id, signed short x, signed short y, signed char z);
	void RemoveRoof(CClient *pClient, ITEMID_TYPE id, signed short x, signed short y, signed char z);

	void SwitchToLevel(CClient *pClient, BYTE bLevel);
	void BackupStructure();
	void RestoreStructure(CClient *pClient = NULL);
	void RevertChanges(CClient *pClient = NULL);
	void ResetStructure(CClient *pClient = NULL);
	void CommitChanges(CClient *pClient = NULL);

	void SendVersionTo(CClient *pClient);
	void SendStructureTo(CClient *pClient);

	DWORD GetRevision(const CClient *pClient = NULL) const;
	size_t GetFixtureCount(DesignDetails *pDesign);
	size_t GetComponentsAt(signed short x, signed short y, signed char z, Component **ppComponentList, DesignDetails *pDesign);

	const CGrayMultiCustom *GetMultiItemDefs();
	const CGRect GetDesignArea();
	static BYTE GetLevel(signed char z);
	static signed char GetLevelZ(BYTE bLevel);

private:
	const CPointMap GetComponentPoint(Component *pComponent) const;
	void CopyDesign(DesignDetails *pDesignFrom, DesignDetails *pDesignTo);

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
