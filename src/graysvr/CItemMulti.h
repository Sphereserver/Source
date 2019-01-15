#ifndef _INC_CITEMMULTI_H
#define _INC_CITEMMULTI_H
#pragma once

class CItemMulti : public CItem
{
	// IT_MULTI
	// IT_MULTI_CUSTOM
	// IT_SHIP
public:
	static const char *m_sClassName;

	CItemMulti(ITEMID_TYPE id, CItemBase *pItemDef);
	virtual ~CItemMulti();

private:
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];

public:
	CRegionWorld *m_pRegion;

	struct ShipSpeed
	{
		BYTE period;	// time between movement
		BYTE tiles;		// distance to move
	};
	ShipSpeed m_shipSpeed;
	BYTE m_SpeedMode;

public:
	static const CItemBaseMulti *Multi_GetDef(ITEMID_TYPE id);
	int Multi_GetMaxDist() const;
	void MultiRealizeRegion();
	void MultiUnRealizeRegion();
	void Multi_Create(CChar *pChar);

	CItem *Multi_GetSign();
	void OnHearRegion(LPCTSTR pszCmd, CChar *pSrc);

	virtual bool OnTick();
	virtual void OnMoveFrom();
	virtual bool MoveTo(CPointMap pt, bool fForceFix = false);

	virtual bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);
	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);	// execute command from script
	virtual void r_Write(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	virtual bool r_LoadVal(CScript &s);
	virtual void DupeCopy(const CItem *pItem);

protected:
	bool Multi_CreateComponent(ITEMID_TYPE id, signed short x, signed short y, signed char z, DWORD dwKeyCode);
	bool Multi_IsPartOf(const CItem *pItem) const;

	CItem *Multi_FindItemType(IT_TYPE type) const;

	const CItemBaseMulti *Multi_GetDef() const
	{
		return dynamic_cast<const CItemBaseMulti *>(Base_GetDef());
	}

	virtual void OnComponentCreate(const CItem *pComponent)
	{
		UNREFERENCED_PARAMETER(pComponent);
	}

private:
	CItemMulti(const CItemMulti &copy);
	CItemMulti &operator=(const CItemMulti &other);
};

#endif	// _INC_CITEMMULTI_H
