//
// CBase.h
//

#ifndef _INC_CBASE_H
#define _INC_CBASE_H
#pragma once

// CCharBase defs
#define CAN_C_GHOST			0x00001		// Moves thru doors etc.
#define CAN_C_SWIM			0x00002		// dolphin, elemental or is water
#define CAN_C_WALK			0x00004		// Can walk on land, climbed on walked over else Frozen by nature(Corpser) or can just swim
#define CAN_C_PASSWALLS		0x00008		// Walk thru walls.
#define CAN_C_FLY			0x00010		// Mongbat etc.
#define CAN_C_FIRE_IMMUNE	0x00020		// Has some immunity to fire ? (will walk into it (lava))
#define CAN_C_INDOORS		0x00040		// Can go under roof. Not really used except to mask.
#define CAN_C_HOVER			0x00080		// Can hover.
#define CAN_C_EQUIP			0x00100		// Can equip stuff. (humanoid)
#define CAN_C_USEHANDS		0x00200		// Can wield weapons (INT dependant), open doors ?, pick up/manipulate things
#define CAN_C_MOUNT			0x00400		// can mount rides
#define CAN_C_FEMALE		0x00800		// It is female by nature.
#define CAN_C_NONHUMANOID	0x01000		// Body type for combat messages.
#define CAN_C_RUN			0x02000		// Can run (not needed if they can fly)
#define CAN_C_DCIGNORELOS	0x04000		// when dclicking sth., ignore LOS checks
#define CAN_C_DCIGNOREDIST	0x08000		// when dclicking sth., ignore distance checks
#define CAN_C_NONMOVER		0x10000		// Just stay in place, avoid movement actions.

// CItemBase defs
#define CAN_I_DOOR			0x0000001	// Is a door UFLAG4_DOOR
#define CAN_I_WATER			0x0000002	// Need to swim in it. UFLAG1_WATER
#define CAN_I_PLATFORM		0x0000004	// we can walk on top of it. (even tho the item itself might block) UFLAG2_PLATFORM
#define CAN_I_BLOCK			0x0000008	// need to walk thru walls or fly over. UFLAG1_BLOCK
#define CAN_I_CLIMB			0x0000010	// step up on it, UFLAG2_CLIMBABLE
#define CAN_I_FIRE			0x0000020	// Is a fire. Ussually blocks as well. UFLAG1_DAMAGE
#define CAN_I_ROOF			0x0000040	// We are under a roof. can't rain on us. UFLAG4_ROOF
#define CAN_I_HOVER			0x0000080	// We are hovering. UFLAG4_HOVEROVER
#define CAN_I_PILE			0x0000100	// Can item be piled UFLAG2_STACKABLE (*.mul)
#define CAN_I_DYE			0x0000200	// Can item be dyed UFLAG3_CLOTH? (sort of)
#define CAN_I_FLIP			0x0000400	// will flip by default.
#define CAN_I_LIGHT			0x0000800	// UFLAG3_LIGHT
#define CAN_I_REPAIR		0x0001000	// Is it repairable (difficulty based on value)
#define CAN_I_REPLICATE		0x0002000	// Things like arrows are pretty much all the same.
#define CAN_I_DCIGNORELOS	0x0004000	// when dclicked, ignore LOS checks
#define CAN_I_DCIGNOREDIST	0x0008000	// when dclicked, ignore distance checks
#define CAN_I_BLOCKLOS		0x0010000	// blocks LOS without blocking walkchecks
#define CAN_I_EXCEPTIONAL	0x0020000	// can items be exceptional
#define CAN_I_MAKERSMARK	0x0040000	// can items hold makers mark
#define CAN_I_RETAINCOLOR	0x0080000	// can items retain material colors
#define CAN_I_ENCHANT		0x0100000	// can items be enchanted (runic)
#define CAN_I_IMBUE			0x0200000	// can items be imbued (imbuing)
#define CAN_I_RECYCLE		0x0400000	// Can items be recycled.
#define CAN_I_REFORGE		0x0800000	// Can items be Runic Reforged.
#define CAN_I_FORCEDC		0x1000000	// Can force DClick skipping other checks (LOS,Distance, Cont...).
#define CAN_I_DAMAGEABLE	0x2000000	// Display item health bar on HS clients >= 7.0.30.0 (MORE1L = cur hitpoints / MORE1H = max hitpoints)

#define CAN_U_ALL			0x00
#define CAN_U_MALE			0x01
#define CAN_U_FEMALE		0x02
#define CAN_U_HUMAN			0x04
#define CAN_U_ELF			0x08
#define CAN_U_GARGOYLE		0x10
#define CAN_U_NONE			0x20

// Masks for movement-affecting flags
#define CAN_C_MOVEMASK		(CAN_C_GHOST|CAN_C_SWIM|CAN_C_WALK|CAN_C_PASSWALLS|CAN_C_FLY|CAN_C_FIRE_IMMUNE|CAN_C_INDOORS|CAN_C_HOVER)
#define CAN_I_MOVEMASK		(CAN_I_DOOR|CAN_I_WATER|CAN_I_PLATFORM|CAN_I_BLOCK|CAN_I_CLIMB|CAN_I_FIRE|CAN_I_ROOF|CAN_I_HOVER)

struct CBaseBaseDef : public CResourceLink
{
	// Base type of both CItemBase and CCharBase
	// Minimal amount of common info to define RES_ITEMDEF or RES_CHARDEF (it might just be a DUPE)
public:
	static LPCTSTR const sm_szLoadKeys[];

	explicit CBaseBaseDef(RESOURCE_ID id);
	virtual ~CBaseBaseDef() { }

protected:
	DWORD m_dwDispIndex;	// base artwork id (may be the same as GetResourceID() in base set, but can also be "flipped")
	CGString m_sName;		// default type name ("human" vs specific "Dennis")

public:
	CVarDefMap m_TagDefs;
	CVarDefMap m_BaseDefs;				// new variable storage system
	CResourceRefArray m_TEvents;		// action or motivation type indexes (NPC only)
	CResourceQtyArray m_BaseResources;	// what is this made of (RESOURCES=10 i_meat, etc)
	//WORD m_range;

	WORD m_attackBase;
	WORD m_attackRange;

	WORD m_defenseBase;
	WORD m_defenseRange;

	int m_DamPhysical;
	int m_DamFire;
	int m_DamCold;
	int m_DamPoison;
	int m_DamEnergy;

	int m_ResPhysical;
	int m_ResPhysicalMax;
	int m_ResFire;
	int m_ResFireMax;
	int m_ResCold;
	int m_ResColdMax;
	int m_ResPoison;
	int m_ResPoisonMax;
	int m_ResEnergy;
	int m_ResEnergyMax;

	int m_Luck;
	int m_DamIncrease;
	int m_SpellDamIncrease;
	int m_HitLifeLeech;
	int m_HitManaDrain;
	int m_HitManaLeech;
	int m_HitStaminaLeech;
	int m_HitChanceIncrease;
	int m_DefChanceIncrease;
	int m_DefChanceIncreaseMax;
	int m_SwingSpeedIncrease;
	int m_FasterCasting;
	int m_FasterCastRecovery;
	int m_LowerManaCost;
	int m_LowerReagentCost;
	int m_EnhancePotions;
	int m_NightSight;
	int m_ReflectPhysicalDamage;

	DWORD m_Can;

private:
	height_t m_Height;

	BYTE m_ResLevel;
	WORD m_ResDispDnId;
	HUE_TYPE m_ResDispDnHue;

public:
	void CopyBasic(const CBaseBaseDef *pBaseDef);
	void CopyTransfer(CBaseBaseDef *pBaseDef);

	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc = NULL);
	virtual bool r_LoadVal(CScript &s);

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

public:
	virtual void UnLink()
	{
		m_BaseResources.RemoveAll();
		m_TEvents.RemoveAll();
		CResourceLink::UnLink();
	}

	bool IsValid() const
	{
		return m_sName.IsValid();
	}

	virtual LPCTSTR GetName() const
	{
		return GetTypeName();
	}

	LPCTSTR GetTypeName() const
	{
		return m_sName;
	}
	virtual void SetTypeName(LPCTSTR pszName)
	{
		GETNONWHITESPACE(pszName);
		m_sName = pszName;
	}

	bool HasTypeName() const
	{
		return !m_sName.IsEmpty();
	}

	bool Can(DWORD dwCan) const
	{
		return (m_Can & dwCan);
	}

	BYTE GetRangeL() const
	{
		return static_cast<BYTE>(GetDefNum("RANGE") & BYTE_MAX);
	}
	BYTE GetRangeH() const
	{
		return static_cast<BYTE>((GetDefNum("RANGE") >> 8) & BYTE_MAX);
	}

	height_t GetHeight() const
	{
		return m_Height;
	}
	void SetHeight(height_t Height)
	{
		m_Height = Height;
	}

	BYTE GetResLevel() const
	{
		return m_ResLevel;
	}
	bool SetResLevel(BYTE bResLevel)
	{
		m_ResLevel = bResLevel;
		return true;
	}

	WORD GetResDispDnId() const
	{
		return m_ResDispDnId;
	}
	void SetResDispDnId(WORD ResDispDnId)
	{
		m_ResDispDnId = ResDispDnId;
	}

	HUE_TYPE GetResDispDnHue() const
	{
		return m_ResDispDnHue;
	}
	void SetResDispDnHue(HUE_TYPE ResDispDnHue)
	{
		m_ResDispDnHue = ResDispDnHue;
	}

private:
	CBaseBaseDef(const CBaseBaseDef &copy);
	CBaseBaseDef &operator=(const CBaseBaseDef &other);
};

#endif	// _INC_CBASE_H
