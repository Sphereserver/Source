#ifndef _INC_CITEMSTONE_H
#define _INC_CITEMSTONE_H
#pragma once

enum STONEPRIV_TYPE
{
	STONEPRIV_CANDIDATE,
	STONEPRIV_MEMBER,
	STONEPRIV_MASTER,
	STONEPRIV_UNUSED,
	STONEPRIV_ACCEPTED,		// candidate has been accepted but have not DClicked on the stone to become a full member yet
	STONEPRIV_ENEMY = 100,	// enemy guild/town
	STONEPRIV_ALLY			// ally guild/town
};

class CStoneMember : public CGObListRec, public CScriptObj
{
	// Members for various stones, and links to its enemy/ally stones
	// NOTE: chars are linked to CItemStone via memory object
	friend class CItemStone;

public:
	static const char *m_sClassName;
	static const LPCTSTR sm_szLoadKeys[];
	static const LPCTSTR sm_szVerbKeys[];

	CStoneMember(CItemStone *pStone, CGrayUID uidLinkTo, STONEPRIV_TYPE priv, LPCTSTR pszTitle = "", CGrayUID uidLoyalTo = UID_CLEAR, bool fVal1 = false, bool fVal2 = false, int iAccountGold = 0);
	virtual ~CStoneMember();

private:
	STONEPRIV_TYPE m_priv;
	CGrayUID m_uidLinkTo;

	// Only apply to members
	CGrayUID m_uidLoyalTo;
	CGString m_sTitle;

	union	// depends on m_priv
	{
		struct	// unknown type
		{
			int m_iVal1;
			int m_iVal2;
			int m_iVal3;
		} m_Unk;

		struct	// STONEPRIV_ENEMY
		{
			int m_fTheyDeclared;
			int m_fWeDeclared;
		} m_Enemy;

		struct	// STONEPRIV_ALLY
		{
			int m_fTheyDeclared;
			int m_fWeDeclared;
		} m_Ally;

		struct	// char member (not STONEPRIV_ENEMY / STONEPRIV_ALLY)
		{
			int m_fAbbrev;
			int m_iVoteTally;
			int m_iAccountGold;
		} m_Member;
	};

private:
	LPCTSTR GetPrivName() const;
	bool SetLoyalTo(const CChar *pChar);

	bool IsPrivMember() const
	{
		return ((m_priv == STONEPRIV_MASTER) || (m_priv == STONEPRIV_MEMBER));
	}

	CStoneMember *GetNext() const
	{
		return static_cast<CStoneMember *>(CGObListRec::GetNext());
	}

public:
	CItemStone *GetParentStone() const;

	LPCTSTR GetName() const
	{
		return m_sClassName;
	}

	LPCTSTR GetTitle() const
	{
		return m_sTitle;
	}

	bool IsAbbrevOn() const
	{
		return m_Member.m_fAbbrev ? true : false;
	}

	virtual bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);
	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);	// execute command from script
	virtual bool r_LoadVal(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);

private:
	CStoneMember(const CStoneMember &copy);
	CStoneMember &operator=(const CStoneMember &other);
};

class CItemStone : public CItem, public CGObList
{
	// IT_STONE_GUILD
	// IT_STONE_TOWN
	friend class CStoneMember;

public:
	static const char *m_sClassName;
	static const LPCTSTR sm_szLoadKeys[];
	static const LPCTSTR sm_szVerbKeys[];

	CItemStone(ITEMID_TYPE id, CItemBase *pItemDef);
	virtual ~CItemStone();

private:
	CGString m_sCharter[6];
	CGString m_sWebPageURL;
	CGString m_sAbbrev;

private:
	MEMORY_TYPE GetMemoryType() const;
	LPCTSTR GetTypeName() const;
	CStoneMember *GetMasterMember() const;
	CChar *GetMaster() const;

	void ElectMaster();
	bool CheckValidMember(CStoneMember *pMember);
	int FixWeirdness();
	bool WeDeclareWar(CItemStone *pStone);
	void TheyDeclarePeace(CItemStone *pStone, bool fForcePeace);
	void WeDeclarePeace(CItemStone *pStone, bool fForcePeace = false);
	void SetTownName();
	virtual bool MoveTo(CPointMap pt, bool fForceFix = false);
	bool SetName(LPCTSTR pszName);

public:
	LPCTSTR GetAlignName() const;
	CStoneMember *GetMember(const CObjBase *pObj) const;
	CStoneMember *AddRecruit(const CChar *pChar, STONEPRIV_TYPE priv, bool fFull = false);
	bool IsAlliedWith(const CItemStone *pStone) const;
	bool IsAtWarWith(const CItemStone *pStone) const;

	LPCTSTR GetAbbrev() const
	{
		return m_sAbbrev;
	}

	virtual bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);
	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);	// execute command from script
	virtual void r_Write(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	virtual bool r_LoadVal(CScript &s);

private:
	CItemStone(const CItemStone &copy);
	CItemStone &operator=(const CItemStone &other);
};

#endif	// _INC_CITEMSTONE_H
