#ifndef _INC_CITEMSHIP_H
#define _INC_CITEMSHIP_H
#pragma once

class CItemShip : public CItemMulti
{
	// IT_SHIP
public:
	static const char *m_sClassName;

	CItemShip(ITEMID_TYPE id, CItemBase *pItemDef);
	virtual ~CItemShip();

private:
	static const LPCTSTR sm_szLoadKeys[];
	static const LPCTSTR sm_szVerbKeys[];

	CGrayUID m_uidHold;
	std::vector<CGrayUID> m_uidPlanks;
	CServTime m_NextMove;

public:
	void Ship_Stop();
	void Ship_SetPilot(CChar *pChar);
	bool Ship_SetMoveDir(DIR_TYPE dir, BYTE bSpeed = 2, bool fWheelMove = false);
	bool Ship_Face(DIR_TYPE dir);
	bool Ship_Move(DIR_TYPE dir, int iDist);

	CItemContainer *GetShipHold();
	size_t GetShipPlankCount();
	CItem *GetShipPlank(size_t index);

	virtual bool OnTick();

private:
	size_t Ship_ListObjs(CObjBase **ppObjList);
	bool Ship_MoveDelta(CPointBase ptDelta, bool fMapBoundary = false);
	bool Ship_CanMoveTo(const CPointMap &pt) const;
	bool Ship_OnMoveTick();

	int Ship_GetFaceOffset() const
	{
		return (GetID() & 3);
	}

	virtual bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);
	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);	// execute command from script
	virtual void r_Write(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	virtual bool r_LoadVal(CScript &s);

	virtual int FixWeirdness();
	virtual void OnComponentCreate(const CItem *pComponent);

private:
	CItemShip(const CItemShip &copy);
	CItemShip &operator=(const CItemShip &other);
};

#endif	// _INC_CITEMSHIP_H
