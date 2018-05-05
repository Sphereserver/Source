//
// CCharBase.h
//

#ifndef _INC_CCHARBASE_H
#define _INC_CCHARBASE_H
#pragma once

class CCharBase : public CBaseBaseDef	// define basic info about each "TYPE" of monster/creature
{
	// RES_CHARDEF
public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szLoadKeys[];

	explicit CCharBase(CREID_TYPE id);
	~CCharBase() { }

public:
	ITEMID_TYPE m_trackID;			// ITEMID_TYPE what look like on tracking
	SOUND_TYPE m_soundBase;			// sounds (typically 5 sounds per creature, humans and birds have more)
	SOUND_TYPE m_soundIdle;
	SOUND_TYPE m_soundNotice;
	SOUND_TYPE m_soundHit;
	SOUND_TYPE m_soundGetHit;
	SOUND_TYPE m_soundDie;

	CResourceQtyArray m_FoodType;	// FOODTYPE=MEAT 15 (3)
	int m_MaxFood;					// Derived from foodtype, this is the max amount of food we can eat

	WORD m_defense;			// base defense (basic to body type)
	DWORD m_Anims;			// bitmask of animations available for monsters (ANIM_TYPE)
	HUE_TYPE m_wBloodHue;	// when damaged, what color is the blood (-1 = no blood)
	HUE_TYPE m_wColor;

	int m_Str;
	int m_Dex;
	int m_Int;
	short m_iMoveRate;
	short m_FollowerSlots;
	short m_FollowerMax;
	int m_Tithing;

	// NPC info
	CResourceQtyArray m_Aversions;	// traps, civilization
	CResourceQtyArray m_Desires;	// desires that are typical for the char class (see also m_sNeed)
	CResourceRefArray m_Speech;		// speech fragment list (other stuff we know)
	unsigned int m_iHireDayWage;	// gold required to hire an player vendor

public:
	static CCharBase *FindCharBase(CREID_TYPE id);
	bool SetDispID(CREID_TYPE id);
	LPCTSTR GetTradeName() const;

	bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc = NULL);
	bool r_LoadVal(CScript &s);
	bool r_Load(CScript &s);

private:
	void CopyBasic(const CCharBase *pCharDef);
	void SetFoodType(LPCTSTR pszFood);

public:
	virtual void UnLink()
	{
		// We are being reloaded
		m_FoodType.RemoveAll();
		m_Desires.RemoveAll();
		m_Speech.RemoveAll();
		CBaseBaseDef::UnLink();
	}

	CREID_TYPE GetID() const
	{
		return static_cast<CREID_TYPE>(GetResourceID().GetResIndex());
	}

	CREID_TYPE GetDispID() const
	{
		return static_cast<CREID_TYPE>(m_dwDispIndex);
	}

	static bool IsValidDispID(CREID_TYPE id)
	{
		return ((id > CREID_INVALID) && (id < CREID_QTY));
	}

	static bool IsPlayableID(CREID_TYPE id, bool bCheckGhost = false)
	{
		return (IsHumanID(id, bCheckGhost) || IsElfID(id, bCheckGhost) || IsGargoyleID(id, bCheckGhost));
	}

	static bool IsHumanID(CREID_TYPE id, bool bCheckGhost = false)
	{
		if ( bCheckGhost )
			return ((id == CREID_MAN) || (id == CREID_WOMAN) || (id == CREID_EQUIP_GM_ROBE) || (id == CREID_GHOSTMAN) || (id == CREID_GHOSTWOMAN));
		else
			return ((id == CREID_MAN) || (id == CREID_WOMAN) || (id == CREID_EQUIP_GM_ROBE));
	}

	static bool IsElfID(CREID_TYPE id, bool bCheckGhost = false)
	{
		if ( bCheckGhost )
			return ((id == CREID_ELFMAN) || (id == CREID_ELFWOMAN) || (id == CREID_ELFGHOSTMAN) || (id == CREID_ELFGHOSTWOMAN));
		else
			return ((id == CREID_ELFMAN) || (id == CREID_ELFWOMAN));
	}

	static bool IsGargoyleID(CREID_TYPE id, bool bCheckGhost = false)
	{
		if ( bCheckGhost )
			return ((id == CREID_GARGMAN) || (id == CREID_GARGWOMAN) || (id == CREID_GARGGHOSTMAN) || (id == CREID_GARGGHOSTWOMAN));
		else
			return ((id == CREID_GARGMAN) || (id == CREID_GARGWOMAN));
	}

	bool IsFemale() const
	{
		return (m_Can & CAN_C_FEMALE);
	}

private:
	CCharBase(const CCharBase &copy);
	CCharBase &operator=(const CCharBase &other);
};

#endif	// _INC_CCHARBASE_H
