//
// CObjBase.h
//

#ifndef _INC_COBJBASE_H
#define _INC_COBJBASE_H
#pragma once

class PacketPropertyList;

class CObjBase : public CObjBaseTemplate, public CScriptObj
{
	// All instances of CItem or CChar have these base attributes
public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szRefKeys[];

	explicit CObjBase(bool fItem);
	virtual ~CObjBase();

private:
	CObjBase(const CObjBase &copy);
	CObjBase &operator=(const CObjBase &other);

private:
	CServTime m_timeout;
	CServTime m_timestamp;
	HUE_TYPE m_wHue;			// hue or skin color (CItems must be < 0x4FF or so)
	LPCTSTR m_RunningTrigger;

protected:
	CResourceRef m_BaseRef;

public:
	CVarDefMap m_TagDefs;		// attach extra tags here
	CVarDefMap m_BaseDefs;		// new variable storage system

	DWORD m_Can;
	int m_ModMaxWeight;

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

	CGrayUID m_uidSpawnItem;	// SpawnItem for this item

	CResourceRefArray m_OEvents;
	static size_t sm_iCount;	// how many total objects in the world?

public:
	BYTE RangeL() const
	{
		CVarDefCont *pRange = GetDefKey("RANGE", true);
		return static_cast<BYTE>((pRange ? pRange->GetValNum() : 0) & BYTE_MAX);
	}
	BYTE RangeH() const
	{
		CVarDefCont *pRange = GetDefKey("RANGE", true);
		return static_cast<BYTE>(((pRange ? pRange->GetValNum() : 0) >> 8) & BYTE_MAX);
	}

	CVarDefMap *GetTagDefs()
	{
		return &m_TagDefs;
	}

	CServTime GetTimeStamp() const
	{
		return m_timestamp;
	}
	void SetTimeStamp(UINT64 t_time)
	{
		m_timestamp.InitTime(t_time);
	}

	CVarDefCont *GetDefKey(LPCTSTR pszKey, bool fDef) const
	{
		CVarDefCont	*pVar = m_BaseDefs.GetKey(pszKey);
		if ( !fDef || pVar )
			return pVar;
		if ( IsItem() )
		{
			CItemBase *pItemDef = static_cast<CItemBase *>(Base_GetDef());
			ASSERT(pItemDef);
			return pItemDef->m_BaseDefs.GetKey(pszKey);
		}
		else
		{
			CCharBase *pCharDef = static_cast<CCharBase *>(Base_GetDef());
			ASSERT(pCharDef);
			return pCharDef->m_BaseDefs.GetKey(pszKey);
		}
	}

	LPCTSTR GetDefStr(LPCTSTR pszKey, bool fZero = false, bool fDef = false) const
	{
		CVarDefCont	*pVar = GetDefKey(pszKey, fDef);
		if ( pVar )
			return pVar->GetValStr();
		return fZero ? "0" : "";
	}
	void SetDefStr(LPCTSTR pszKey, LPCTSTR pszVal, bool fQuoted = false, bool fZero = true)
	{
		m_BaseDefs.SetStr(pszKey, fQuoted, pszVal, fZero);
	}

	INT64 GetDefNum(LPCTSTR pszKey, bool fDef = false) const
	{
		CVarDefCont	*pVar = GetDefKey(pszKey, fDef);
		return pVar ? pVar->GetValNum() : 0;
	}
	void SetDefNum(LPCTSTR pszKey, INT64 iVal, bool fZero = true)
	{
		m_BaseDefs.SetNum(pszKey, iVal, fZero);
	}

	void DeleteDef(LPCTSTR pszKey)
	{
		m_BaseDefs.DeleteKey(pszKey);
	}

	CVarDefCont *GetKey(LPCTSTR pszKey, bool fDef) const
	{
		CVarDefCont	*pVar = m_TagDefs.GetKey(pszKey);
		if ( !fDef || pVar )
			return pVar;
		if ( IsItem() )
		{
			CItemBase *pItemDef = static_cast<CItemBase *>(Base_GetDef());
			ASSERT(pItemDef);
			return pItemDef->m_TagDefs.GetKey(pszKey);
		}
		else
		{
			CCharBase *pCharDef = static_cast<CCharBase *>(Base_GetDef());
			ASSERT(pCharDef);
			return pCharDef->m_TagDefs.GetKey(pszKey);
		}
	}

	LPCTSTR GetKeyStr(LPCTSTR pszKey, bool fZero = false, bool fDef = false) const
	{
		CVarDefCont	*pVar = GetKey(pszKey, fDef);
		if ( pVar )
			return pVar->GetValStr();
		return fZero ? "0" : "";
	}
	void SetKeyStr(LPCTSTR pszKey, LPCTSTR pszVal)
	{
		m_TagDefs.SetStr(pszKey, false, pszVal);
	}

	INT64 GetKeyNum(LPCTSTR pszKey, bool fDef = false) const
	{
		CVarDefCont	*pVar = GetKey(pszKey, fDef);
		return pVar ? pVar->GetValNum() : 0;
	}
	void SetKeyNum(LPCTSTR pszKey, INT64 iVal)
	{
		m_TagDefs.SetNum(pszKey, iVal);
	}

	void DeleteKey(LPCTSTR pszKey)
	{
		m_TagDefs.DeleteKey(pszKey);
	}

protected:
	virtual void DupeCopy(const CObjBase *pObj)
	{
		CObjBaseTemplate::DupeCopy(pObj);
		m_wHue = pObj->GetHue();
		m_TagDefs.Copy(&pObj->m_TagDefs);
		m_BaseDefs.Copy(&pObj->m_BaseDefs);
		m_OEvents.Copy(&pObj->m_OEvents);

		m_Can = pObj->m_Can;
		m_ModMaxWeight = pObj->m_ModMaxWeight;

		m_attackBase = pObj->m_attackBase;
		m_attackRange = pObj->m_attackRange;

		m_defenseBase = pObj->m_defenseBase;
		m_defenseRange = pObj->m_defenseRange;

		m_DamPhysical = pObj->m_DamPhysical;
		m_DamFire = pObj->m_DamFire;
		m_DamCold = pObj->m_DamCold;
		m_DamPoison = pObj->m_DamPoison;
		m_DamEnergy = pObj->m_DamEnergy;

		m_ResPhysical = pObj->m_ResPhysical;
		m_ResPhysicalMax = pObj->m_ResPhysicalMax;
		m_ResFire = pObj->m_ResFire;
		m_ResFireMax = pObj->m_ResFireMax;
		m_ResCold = pObj->m_ResCold;
		m_ResColdMax = pObj->m_ResColdMax;
		m_ResPoison = pObj->m_ResPoison;
		m_ResPoisonMax = pObj->m_ResPoisonMax;
		m_ResEnergy = pObj->m_ResEnergy;
		m_ResEnergyMax = pObj->m_ResEnergyMax;

		m_Luck = pObj->m_Luck;
		m_DamIncrease = pObj->m_DamIncrease;
		m_SpellDamIncrease = pObj->m_SpellDamIncrease;
		m_HitLifeLeech = pObj->m_HitLifeLeech;
		m_HitManaDrain = pObj->m_HitManaDrain;
		m_HitManaLeech = pObj->m_HitManaLeech;
		m_HitStaminaLeech = pObj->m_HitStaminaLeech;
		m_HitChanceIncrease = pObj->m_HitChanceIncrease;
		m_DefChanceIncrease = pObj->m_DefChanceIncrease;
		m_DefChanceIncreaseMax = pObj->m_DefChanceIncreaseMax;
		m_SwingSpeedIncrease = pObj->m_SwingSpeedIncrease;
		m_FasterCasting = pObj->m_FasterCasting;
		m_FasterCastRecovery = pObj->m_FasterCastRecovery;
		m_LowerManaCost = pObj->m_LowerManaCost;
		m_LowerReagentCost = pObj->m_LowerReagentCost;
		m_EnhancePotions = pObj->m_EnhancePotions;
		m_NightSight = pObj->m_NightSight;
		m_ReflectPhysicalDamage = pObj->m_ReflectPhysicalDamage;
	}

public:
	virtual void DeletePrepare();
	virtual void Delete(bool fForce = false);

	bool IsTriggerActive(LPCTSTR pszTrig);
	LPCTSTR GetTriggerActive();
	void SetTriggerActive(LPCTSTR pszTrig = NULL);

	virtual bool OnTick() = 0;
	virtual int FixWeirdness() = 0;
	virtual int GetWeight(WORD wAmount = 0) const = 0;
	virtual bool IsResourceMatch(RESOURCE_ID_BASE rid, DWORD dwArg) = 0;

	virtual int IsWeird() const;

	// Accessors
	virtual WORD GetBaseID() const = 0;
	CBaseBaseDef *Base_GetDef() const
	{
		return static_cast<CBaseBaseDef *>(m_BaseRef.GetRef());
	}

	void SetUID(DWORD dwVal, bool fItem);
	CObjBase *GetNext() const
	{
		return static_cast<CObjBase *>(CGObListRec::GetNext());
	}
	CObjBase *GetPrev() const
	{
		return static_cast<CObjBase *>(CGObListRec::GetPrev());
	}
	virtual LPCTSTR GetName() const		// resolve ambiguity w/ CScriptObj
	{
		return CObjBaseTemplate::GetName();
	}
	LPCTSTR GetResourceName() const
	{
		return Base_GetDef()->GetResourceName();
	}

public:
	void SetHue(HUE_TYPE wHue, bool fSkipTrigger = true, CTextConsole *pSrc = NULL, CObjBase *pObj = NULL, SOUND_TYPE wSound = 0);
	HUE_TYPE GetHue() const
	{
		return m_wHue;
	}

protected:
	void SetHueAlt(HUE_TYPE wHue)
	{
		m_wHue = wHue;
	}
	WORD GetHueAlt() const
	{
		return m_wHue;
	}

public:
	virtual void SetTimeout(INT64 iDelayInTicks);
	bool IsTimerSet() const
	{
		return m_timeout.IsTimeValid();
	}
	bool IsTimerExpired() const
	{
		return (GetTimerDiff() <= 0);
	}

	INT64 GetTimerDiff() const;		// return < 0 = in the past

	INT64 GetTimerAdjusted() const
	{
		// Return time in seconds from now
		if ( !IsTimerSet() )
			return -1;
		INT64 iDiffInTicks = GetTimerDiff();
		if ( iDiffInTicks < 0 )
			return 0;
		return iDiffInTicks / TICK_PER_SEC;
	}

	INT64 GetTimerDAdjusted() const
	{
		// Return time in seconds from now
		if ( !IsTimerSet() )
			return -1;
		INT64 iDiffInTicks = GetTimerDiff();
		if ( iDiffInTicks < 0 )
			return 0;
		return iDiffInTicks;
	}

public:
	virtual bool MoveTo(CPointMap pt, bool fForceFix = false) = 0;

	virtual bool MoveNear(CPointMap pt, WORD wSteps = 0);
	virtual bool MoveNearObj(const CObjBaseTemplate *pObj, WORD wSteps = 0);

	bool SetNamePool(LPCTSTR pszName);

	void Sound(SOUND_TYPE id, BYTE bRepeat = 1) const;
	void Effect(EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate *pSrc = NULL, BYTE bSpeed = 0, BYTE bFrames = 0, bool fExplode = false, DWORD dwColor = 0, DWORD dwRender = 0, WORD wEffectID = 0, WORD wExplodeID = 0, WORD wExplodeSound = 0, DWORD dwItemUID = 0, BYTE bLayer = 0) const;
	void Effect(EFFECT_TYPE motion, ITEMID_TYPE id, CPointMap ptSrc = CPointMap(-1, -1), CPointMap ptDest = CPointMap(-1, -1), BYTE bSpeed = 0, BYTE bFrames = 0, bool fExplode = false, DWORD dwColor = 0, DWORD dwRender = 0, WORD wEffectID = 0, WORD wExplodeID = 0, WORD wExplodeSound = 0, DWORD dwItemUID = 0, BYTE bLayer = 0) const;

	void r_WriteSafe(CScript &s);

	virtual bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);
	virtual void r_Write(CScript &s);
	virtual bool r_LoadVal(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);

	void Emote(LPCTSTR pszTextYou, LPCTSTR pszTextThem = NULL, CClient *pClientExclude = NULL);

	virtual void Speak(LPCTSTR pszText, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL);
	virtual void SpeakUTF8(LPCTSTR pszText, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL, CLanguageID lang = 0);
	virtual void SpeakUTF8Ex(const NWORD *pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang);

	void RemoveFromView(CClient *pClientExclude = NULL, bool fHardcoded = true);
	void ResendOnEquip(bool fAllClients = false);	// fix for Enhanced Client when equipping items via DClick, these must be removed from where they are and sent again
	void ResendTooltip(bool fSendFull = false, bool fUseCache = false);
	void UpdateCanSee(PacketSend *pPacket, CClient *pClientExclude = NULL) const;
	void UpdateObjMessage(LPCTSTR pszTextThem, LPCTSTR pszTextYou, CClient *pClientExclude, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font = FONT_NORMAL, bool fUnicode = false) const;

	TRIGRET_TYPE OnHearTrigger(CResourceLock &s, LPCTSTR pszCmd, CChar *pSrc, TALKMODE_TYPE &mode, HUE_TYPE wHue = HUE_DEFAULT);

	bool IsContainer() const;

	virtual void Update(bool fFull = true, CClient *pClientExclude = NULL) = 0;
	virtual void Flip() = 0;
	virtual bool OnSpellEffect(SPELL_TYPE spell, CChar *pCharSrc, int iSkillLevel, CItem *pSourceItem, bool fReflecting = false) = 0;

	virtual TRIGRET_TYPE Spell_OnTrigger(SPELL_TYPE spell, SPTRIG_TYPE stage, CChar *pSrc, CScriptTriggerArgs *pArgs);
	inline bool CallPersonalTrigger(TCHAR *pszArgs, CTextConsole *pSrc, TRIGRET_TYPE &tr, bool fFull);

public:
	// Some global object variables
	signed int m_ModAr;

	#define SU_UPDATE_HITS		0x1	// update hits to others
	#define SU_UPDATE_MODE		0x2	// update mode to all
	#define SU_UPDATE_TOOLTIP	0x4	// update tooltip to all
	BYTE m_fStatusUpdate;		// update flags for next tick
	virtual void OnTickStatusUpdate();

protected:
	PacketPropertyList *m_PropertyList;	// currently cached property list packet
	DWORD m_PropertyHash;				// latest property list hash
	DWORD m_PropertyRevision;			// current property list revision

public:
	PacketPropertyList *GetPropertyList() const { return m_PropertyList; }
	void SetPropertyList(PacketPropertyList *pPropertyList);
	DWORD GetPropertyHash() const { return m_PropertyHash; }
	DWORD UpdatePropertyRevision(DWORD dwHash);
	void UpdatePropertyFlag();
	void FreePropertyList();
};

#endif	// _INC_COBJBASE_H
