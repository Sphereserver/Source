//
// CObjBase.h
//

#ifndef _INC_COBJBASE_H
#define _INC_COBJBASE_H
#pragma once

enum MEMORY_TYPE
{
	// IT_EQ_MEMORY_OBJ
	// Types of memory a CChar has about a game object. (m_wHue)
	MEMORY_NONE			= 0,
	MEMORY_SAWCRIME		= 0x0001,	// I saw them commit a crime or i was attacked criminally. I can call the guards on them. the crime may not have been against me.
	MEMORY_IPET			= 0x0002,	// I am a pet. (this link is my master) (never time out)
	MEMORY_FIGHT		= 0x0004,	// Active fight going on now. may not have done any damage. and they may not know they are fighting me.
	MEMORY_IAGGRESSOR	= 0x0008,	// I was the agressor here. (good or evil)
	MEMORY_HARMEDBY		= 0x0010,	// I was harmed by them. (but they may have been retaliating)
	MEMORY_IRRITATEDBY	= 0x0020,	// I saw them snoop from me or someone.
	MEMORY_SPEAK		= 0x0040,	// We spoke about something at some point. (or was tamed) (NPC_MEM_ACT_TYPE)
	MEMORY_AGGREIVED	= 0x0080,	// I was attacked and was the inocent party here !
	MEMORY_GUARD		= 0x0100,	// Guard this item (never time out)
	MEMORY_ISPAWNED		= 0x0200,	// UNUSED!!!! I am spawned from this item. (never time out)
	MEMORY_GUILD		= 0x0400,	// This is my guild stone. (never time out) only have 1
	MEMORY_TOWN			= 0x0800,	// This is my town stone. (never time out) only have 1
	MEMORY_FRIEND		= 0x4000,	// They can command me but not release me. (not primary blame)
};

enum NPC_MEM_ACT_TYPE	// A simgle primary memory about the object.
{
	// related to MEMORY_SPEAK
	NPC_MEM_ACT_NONE = 0,		// we spoke about something non-specific,
	NPC_MEM_ACT_SPEAK_TRAIN,	// I am speaking about training. Waiting for money
	NPC_MEM_ACT_SPEAK_HIRE,		// I am speaking about being hired. Waiting for money
	NPC_MEM_ACT_FIRSTSPEAK,		// I attempted (or could have) to speak to player. but have had no response.
	NPC_MEM_ACT_TAMED,		// I was tamed by this person previously.
	NPC_MEM_ACT_IGNORE		// I looted or looked at and discarded this item (ignore it)
};

class PacketPropertyList;

class CObjBase : public CObjBaseTemplate, public CScriptObj
{
	// All Instances of CItem or CChar have these base attributes.
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szRefKeys[];

private:
	CServTime m_timeout;		// when does this rot away ? or other action. 0 = never, else system time
	CServTime m_timestamp;
	HUE_TYPE m_wHue;			// Hue or skin color. (CItems must be < 0x4ff or so)
	LPCTSTR m_RunningTrigger;

protected:
	CResourceRef m_BaseRef;	// Pointer to the resource that describes this type.
public:
	inline bool CallPersonalTrigger(TCHAR * pArgs, CTextConsole * pSrc, TRIGRET_TYPE & trResult, bool bFull);
	static const char *m_sClassName;
	CVarDefMap m_TagDefs;		// attach extra tags here.
	CVarDefMap m_BaseDefs;		// New Variable storage system
	DWORD	m_Can;
	int m_ModMaxWeight;

	WORD	m_attackBase;	// dam for weapons
	WORD	m_attackRange;	// variable range of attack damage.

	WORD	m_defenseBase;	// Armor for IsArmor items
	WORD	m_defenseRange;	// variable range of defense.
	CGrayUID m_uidSpawnItem;		// SpawnItem for this item

	CResourceRefArray m_OEvents;
	static size_t sm_iCount;	// how many total objects in the world ?
	CVarDefMap *GetTagDefs()
	{
		return &m_TagDefs;
	}
	virtual void DeletePrepare();
	bool IsTriggerActive(LPCTSTR pszTrig) ;
	LPCTSTR GetTriggerActive();
	void SetTriggerActive(LPCTSTR pszTrig = NULL); 

public:
	BYTE RangeL() const
	{
		CVarDefCont *pRange = GetDefKey("RANGE", true);
		return static_cast<BYTE>((pRange ? pRange->GetValNum() : 0) & UCHAR_MAX);
	}

	BYTE RangeH() const
	{
		CVarDefCont *pRange = GetDefKey("RANGE", true);
		return static_cast<BYTE>(((pRange ? pRange->GetValNum() : 0) >> 8) & UCHAR_MAX);
	}

	CServTime GetTimeStamp() const
	{
		return m_timestamp;
	}
	void SetTimeStamp( UINT64 t_time )
	{
		m_timestamp.InitTime(t_time);
	}

	LPCTSTR GetDefStr(LPCTSTR pszKey, bool fZero = false, bool fDef = false) const
	{
		CVarDefCont	*pVar = GetDefKey(pszKey, fDef);
		if ( pVar == NULL )
			return fZero ? "0" : "";
		return pVar->GetValStr();
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

	void SetDefStr(LPCTSTR pszKey, LPCTSTR pszVal, bool fQuoted = false, bool fZero = true)
	{
		m_BaseDefs.SetStr(pszKey, fQuoted, pszVal, fZero);
	}

	void DeleteDef(LPCTSTR pszKey)
	{
		m_BaseDefs.DeleteKey(pszKey);
	}

	CVarDefCont * GetDefKey( LPCTSTR pszKey, bool fDef ) const
	{
		CVarDefCont	* pVar	= m_BaseDefs.GetKey( pszKey );
		if ( !fDef || pVar )	return pVar;
		if (IsItem())
		{
			CItemBase *pItemDef = static_cast<CItemBase *>(Base_GetDef());
			ASSERT(pItemDef);
			return pItemDef-> m_BaseDefs.GetKey( pszKey );
		}
		else
		{
			CCharBase *pCharDef = static_cast<CCharBase *>( Base_GetDef());
			ASSERT(pCharDef);
			return pCharDef-> m_BaseDefs.GetKey( pszKey );
		}
	}

	LPCTSTR GetKeyStr( LPCTSTR pszKey, bool fZero = false, bool fDef = false ) const
	{
		CVarDefCont	* pVar = GetKey( pszKey, fDef );
		if ( pVar == NULL )
			return fZero ? "0" : "";
		return pVar->GetValStr();
	}

	INT64 GetKeyNum( LPCTSTR pszKey, bool fDef = false ) const
	{
		CVarDefCont	*pVar = GetKey(pszKey, fDef);
		return pVar ? pVar->GetValNum() : 0;
	}

	CVarDefCont * GetKey( LPCTSTR pszKey, bool fDef ) const
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

	void SetKeyNum(LPCTSTR pszKey, INT64 iVal)
	{
		m_TagDefs.SetNum(pszKey, iVal);
	}

	void SetKeyStr(LPCTSTR pszKey, LPCTSTR pszVal)
	{
		m_TagDefs.SetStr(pszKey, false, pszVal);
	}

	void DeleteKey(LPCTSTR pszKey)
	{
		m_TagDefs.DeleteKey(pszKey);
	}

protected:
	virtual void DupeCopy( const CObjBase * pObj )
	{
		CObjBaseTemplate::DupeCopy( pObj );
		m_wHue = pObj->GetHue();
		// m_timeout = pObj->m_timeout;
		m_TagDefs.Copy( &( pObj->m_TagDefs ) );
		m_BaseDefs.Copy(&(pObj->m_BaseDefs));
	}

public:
	virtual bool OnTick() = 0;
	virtual int FixWeirdness() = 0;
	virtual int GetWeight(WORD wAmount = 0) const = 0;
	virtual bool IsResourceMatch( RESOURCE_ID_BASE rid, DWORD dwArg ) = 0;

	virtual int IsWeird() const;
	virtual void Delete(bool bforce = false);

	// Accessors

	virtual WORD GetBaseID() const = 0;
	CBaseBaseDef * Base_GetDef() const
	{
		return static_cast<CBaseBaseDef *>(m_BaseRef.GetRef());
	}

	void SetUID( DWORD dwVal, bool fItem );
	CObjBase *GetNext() const
	{
		return static_cast<CObjBase *>(CGObListRec::GetNext());
	}
	CObjBase *GetPrev() const
	{
		return static_cast<CObjBase *>(CGObListRec::GetPrev());
	}
	virtual LPCTSTR GetName() const	// resolve ambiguity w/CScriptObj
	{
		return CObjBaseTemplate::GetName();
	}
	LPCTSTR GetResourceName() const
	{
		return Base_GetDef()->GetResourceName();
	}

public:
	// Hue
	void SetHue( HUE_TYPE wHue, bool bAvoidTrigger = true, CTextConsole *pSrc = NULL, CObjBase *SourceObj = NULL, long long sound = 0 );
	HUE_TYPE GetHue() const
	{
		return m_wHue;
	}

protected:
	WORD GetHueAlt() const
	{
		// IT_EQ_MEMORY_OBJ = MEMORY_TYPE mask
		// IT_EQ_VENDOR_BOX = restock time.
		return m_wHue;
	}
	void SetHueAlt( HUE_TYPE wHue )
	{
		m_wHue = wHue;
	}

public:
	// Timer
	virtual void SetTimeout( INT64 iDelayInTicks );
	bool IsTimerSet() const
	{
		return m_timeout.IsTimeValid();
	}
	INT64 GetTimerDiff() const;	// return: < 0 = in the past ( m_timeout - CServTime::GetCurrentTime() )
	bool IsTimerExpired() const
	{
		return (GetTimerDiff() <= 0);
	}

	INT64 GetTimerAdjusted() const
	{
		// RETURN: time in seconds from now.
		if ( ! IsTimerSet())
			return -1;
		INT64 iDiffInTicks = GetTimerDiff();
		if ( iDiffInTicks < 0 )
			return 0;
		return iDiffInTicks / TICK_PER_SEC;
	}

	INT64 GetTimerDAdjusted() const
	{
		// RETURN: time in seconds from now.
		if ( ! IsTimerSet())
			return -1;
		INT64 iDiffInTicks = GetTimerDiff();
		if ( iDiffInTicks < 0 )
			return 0;
		return iDiffInTicks;
	}

public:
	// Location
	virtual bool MoveTo(CPointMap pt, bool bForceFix = false) = 0;	// Move to a location at top level.

	virtual bool MoveNear( CPointMap pt, WORD iSteps = 0 );
	virtual bool MoveNearObj( const CObjBaseTemplate *pObj, WORD iSteps = 0 );

	void inline SetNamePool_Fail( TCHAR * ppTitles );
	bool SetNamePool( LPCTSTR pszName );

	void Sound(SOUND_TYPE id, BYTE iRepeat = 1) const; // Play sound effect from this location.
	void Effect(EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBase * pSource = NULL, BYTE bspeedseconds = 5, BYTE bloop = 1, bool fexplode = false, DWORD color = 0, DWORD render = 0, WORD effectid = 0, WORD explodeid = 0, WORD explodesound = 0, DWORD effectuid = 0, BYTE type = 0) const;

	void r_WriteSafe( CScript & s );

	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual void r_Write( CScript & s );
	virtual bool r_LoadVal( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc );	// some command on this object as a target

	void Emote(LPCTSTR pszText, CClient * pClientExclude = NULL, bool fPossessive = false);
	void Emote2(LPCTSTR pszText, LPCTSTR pszText2, CClient * pClientExclude = NULL, bool fPossessive = false);

	virtual void Speak( LPCTSTR pszText, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL );
	virtual void SpeakUTF8( LPCTSTR pszText, HUE_TYPE wHue= HUE_TEXT_DEF, TALKMODE_TYPE mode= TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL, CLanguageID lang = 0 );
	virtual void SpeakUTF8Ex( const NWORD * pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang );
	
	void RemoveFromView( CClient * pClientExclude = NULL , bool fHardcoded = true );	// remove this item from all clients.
	void ResendOnEquip( bool fAllClients = false );	// Fix for Enhanced Client when equipping items via DClick, these must be removed from where they are and sent again.
	void ResendTooltip( bool bSendFull = false, bool bUseCache = false );	// force reload of tooltip for this object
	void UpdateCanSee( PacketSend * pPacket, CClient * pClientExclude = NULL ) const;
	void UpdateObjMessage( LPCTSTR pszTextThem, LPCTSTR pszTextYou, CClient * pClientExclude, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font = FONT_NORMAL, bool bUnicode = false ) const;

	TRIGRET_TYPE OnHearTrigger(CResourceLock &s, LPCTSTR pszCmd, CChar *pSrc, TALKMODE_TYPE &mode, HUE_TYPE wHue = HUE_DEFAULT);

	bool IsContainer() const;

	virtual void Update(const CClient * pClientExclude = NULL)	// send this new item to clients.
		= 0;
	virtual void Flip()
		= 0;
	virtual bool OnSpellEffect( SPELL_TYPE spell, CChar * pCharSrc, int iSkillLevel, CItem * pSourceItem, bool bReflecting = false )
		= 0;

	virtual TRIGRET_TYPE Spell_OnTrigger( SPELL_TYPE spell, SPTRIG_TYPE stage, CChar * pSrc, CScriptTriggerArgs * pArgs );

public:
	explicit CObjBase( bool fItem );
	virtual ~CObjBase();
private:
	CObjBase(const CObjBase& copy);
	CObjBase& operator=(const CObjBase& other);

public:
	//	Some global object variables
	signed int m_ModAr;

#define SU_UPDATE_HITS			0x01	// update hits to others
#define SU_UPDATE_MODE			0x02	// update mode to all
#define SU_UPDATE_TOOLTIP		0x04	// update tooltip to all
	BYTE m_fStatusUpdate;				// update flags for next tick
	virtual void OnTickStatusUpdate();

protected:
	PacketPropertyList *m_PropertyList;	// currently cached property list packet
	DWORD m_PropertyHash;				// latest property list hash
	DWORD m_PropertyRevision;			// current property list revision

public:
	PacketPropertyList *GetPropertyList(void) const { return m_PropertyList; }
	void SetPropertyList(PacketPropertyList *propertyList);
	void FreePropertyList(void);
	DWORD UpdatePropertyRevision(DWORD hash);
	void UpdatePropertyFlag(int mask);
};

enum STONEALIGN_TYPE // Types of Guild/Town stones
{
	STONEALIGN_STANDARD = 0,
	STONEALIGN_ORDER,
	STONEALIGN_CHAOS
};

enum ITRIG_TYPE
{
	// XTRIG_UNKNOWN = some named trigger not on this list.
	ITRIG_AfterClick = 1,
	ITRIG_Buy,					// Item got bought from a vendor
	ITRIG_Click,				// Item got single clicked by someone
	ITRIG_CLIENTTOOLTIP,		// Item tooltip got requested by someone
	ITRIG_ContextMenuRequest,
	ITRIG_ContextMenuSelect,
	ITRIG_Create,				// Item got created (not placed in the world yet)
	ITRIG_DAMAGE,				// Item have been damaged
	ITRIG_DCLICK,				// Item got double clicked by someone
	ITRIG_DESTROY,				// Item got destroyed (removed)
	ITRIG_DROPON_CHAR,			// Item got dropped over an char
	ITRIG_DROPON_GROUND,		// Item got dropped on ground
	ITRIG_DROPON_ITEM,			// Item got dropped over an item
	ITRIG_DROPON_SELF,			// Item got dropped inside an container itself
	ITRIG_DROPON_TRADE,			// Item got dropped on trade window
	ITRIG_EQUIP,				// Item got equipped
	ITRIG_EQUIPTEST,
	ITRIG_MemoryEquip,
	ITRIG_PICKUP_GROUND,		// Item got picked up from the ground
	ITRIG_PICKUP_PACK,			// Item got picked up from inside an container
	ITRIG_PICKUP_SELF,			// Item got picked up from inside an container itself
	ITRIG_PICKUP_STACK,			// Item got picked up from an stack
	ITRIG_Sell,					// Item got sold to a vendor
	ITRIG_Ship_Turn,
	ITRIG_SPELLEFFECT,			// Item got hit by a spell
	ITRIG_STEP,					// Item got stepped by someone
	ITRIG_TARGON_CANCEL,
	ITRIG_TARGON_CHAR,
	ITRIG_TARGON_GROUND,
	ITRIG_TARGON_ITEM,
	ITRIG_TIMER,				// Item timer got expired
	ITRIG_ToolTip,				// Item tooltip got requested by someone
	ITRIG_UNEQUIP,				// Item got unequipped
	ITRIG_QTY
};

enum ITC_TYPE	// Item Template commands
{
	ITC_BREAK,
	ITC_BUY,
	ITC_CONTAINER,
	ITC_FULLINTERP,
	ITC_ITEM,
	ITC_ITEMNEWBIE,
	ITC_NEWBIESWAP,
	ITC_SELL,
	ITC_QTY
};

class CItem : public CObjBase
{
	// RES_WORLDITEM
public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szRefKeys[];
	static LPCTSTR const sm_szTrigName[ITRIG_QTY+1];
	static LPCTSTR const sm_szTemplateTable[ITC_QTY+1];

private:
	ITEMID_TYPE m_dwDispIndex;		// The current display type. ITEMID_TYPE
	WORD m_amount;		// Amount of items in pile. 64K max (or corpse type)
	IT_TYPE m_type;		// What does this item do when dclicked ? defines dynamic_cast type
	BYTE m_containedGridIndex;	// Which grid have i been placed in ? (when in a container)
	DWORD	m_CanUse;		// Base attribute flags. can_u_all/male/female..
	WORD	m_weight;

public:
	BYTE	m_speed;
	// Attribute flags.
#define ATTR_IDENTIFIED		0x0001		// This is the identified name. ???
#define ATTR_DECAY			0x0002		// Timer currently set to decay.
#define ATTR_NEWBIE			0x0004		// Not lost on death or sellable ?
#define ATTR_MOVE_ALWAYS	0x0008		// Always movable (else Default as stored in client) (even if MUL says not movalble) NEVER DECAYS !
#define ATTR_MOVE_NEVER		0x0010		// Never movable (else Default as stored in client) NEVER DECAYS !
#define ATTR_MAGIC			0x0020		// DON'T SET THIS WHILE WORN! This item is magic as apposed to marked or markable.
#define ATTR_OWNED			0x0040		// This is owned by the town. You need to steal it. NEVER DECAYS !
#define ATTR_INVIS			0x0080		// Gray hidden item (to GM's or owners?)
#define ATTR_CURSED			0x0100
#define ATTR_CURSED2		0x0200		// cursed damned unholy
#define ATTR_BLESSED		0x0400
#define ATTR_BLESSED2		0x0800		// blessed sacred holy
#define ATTR_FORSALE		0x1000		// For sale on a vendor.
#define ATTR_STOLEN			0x2000		// The item is hot. m_uidLink = previous owner.
#define ATTR_CAN_DECAY		0x4000		// This item can decay. but it would seem that it would not (ATTR_MOVE_NEVER etc)
#define ATTR_STATIC			0x8000		// WorldForge merge marker. (used for statics saves)
#define ATTR_EXCEPTIONAL	0x10000		// Is Exceptional
#define ATTR_ENCHANTED		0x20000		// Is Enchanted
#define ATTR_IMBUED			0x40000		// Is Imbued
#define ATTR_QUESTITEM		0x80000		// Is Quest Item
#define ATTR_INSURED		0x100000	// Is Insured
#define ATTR_NODROPTRADE	0x200000	// No-drop/trade
#define ATTR_ARTIFACT		0x400000	// Artifact
#define ATTR_LOCKEDDOWN		0x800000	// Is Locked Down
#define ATTR_SECURE			0x1000000	// Is Secure
#define ATTR_REFORGED		0x2000000	// Is Runic Reforged.
#define ATTR_OPENED			0x4000000	// Is Door Opened.
	DWORD	m_Attr;
	// NOTE: If this link is set but not valid -> then delete the whole object !
	CGrayUID m_uidLink;		// Linked to this other object in the world. (owned, key, etc)

	bool IsTriggerActive(LPCTSTR pszTrig) { return static_cast<CObjBase *>(const_cast<CItem *>(this))->IsTriggerActive(pszTrig); }
	void SetTriggerActive(LPCTSTR pszTrig = NULL) { static_cast<CObjBase *>(const_cast<CItem *>(this))->SetTriggerActive(pszTrig); }

	// Type specific info. IT_TYPE
	union // 4(more1) + 4(more2) + 6(morep: (2 morex) (2 morey) (1 morez) (1morem) ) = 14 bytes
	{
		// IT_NORMAL
		struct	// used only to save and restore all this junk.
		{
			DWORD m_more1;
			DWORD m_more2;
			CPointBase m_morep;
		} m_itNormal;

		// IT_CONTAINER
		// IT_CONTAINER_LOCKED
		// IT_DOOR
		// IT_DOOR_OPEN
		// IT_DOOR_LOCKED
		// IT_SHIP_SIDE
		// IT_SHIP_SIDE_LOCKED
		// IT_SHIP_HOLD
		// IT_SHIP_HOLD_LOCK
		struct	// IsTypeLockable()
		{
			CGrayUIDBase m_lockUID;		// more1=the lock code. normally this is the same as the uid (magic lock=non UID)
			DWORD m_lock_complexity;	// more2=0-1000 = How hard to pick or magic unlock. (conflict with door ?)
		} m_itContainer;

		// IT_SHIP_TILLER
		// IT_KEY
		// IT_SIGN_GUMP
		struct
		{
			CGrayUIDBase m_lockUID;		// more1 = the lock code. Normally this is the UID, except if uidLink is set.
		} m_itKey;

		// IT_EQ_BANK_BOX
		struct
		{
			DWORD m_Check_Amount;		// more1=Current amount of gold in account..
			DWORD m_Check_Restock;		// more2= amount to restock the bank account to
			CPointBase m_ptOpen;		// morep=point we are standing on when opened bank box.
		} m_itEqBankBox;

		// IT_EQ_VENDOR_BOX
		struct
		{
			DWORD m_junk1;
			DWORD m_junk2;
			CPointBase m_ptOpen;	// morep=point we are standing on when opened vendor box.
		} m_itEqVendorBox;

		// IT_GAME_BOARD
		struct
		{
			int m_GameType;		// more1=0=chess, 1=checkers, 2=backgammon, 3=no pieces.
		} m_itGameBoard;

		// IT_WAND
		// IT_WEAPON_*
		struct
		{
			WORD m_Hits_Cur;		// more1l=eqiv to quality of the item (armor/weapon).
			WORD m_Hits_Max;		// more1h=can only be repaired up to this level.
			int  m_spellcharges;	// more2=for a wand etc.
			WORD m_spell;			// morex=SPELL_TYPE = The magic spell cast on this. (daemons breath)(boots of strength) etc
			WORD m_spelllevel;		// morey=level of the spell. (0-1000)
			BYTE m_poison_skill;	// morez=0-100 = Is the weapon poisoned ?
			BYTE m_nonused;			// morem
		} m_itWeapon;

		// IT_ARMOR
		// IT_ARMOR_LEATHER
		// IT_SHIELD
		// IT_CLOTHING
		// IT_JEWELRY
		struct
		{
			WORD m_Hits_Cur;		// more1l= eqiv to quality of the item (armor/weapon).
			WORD m_Hits_Max;		// more1h= can only be repaired up to this level.
			int  m_spellcharges;	// more2 = ? spell charges ? not sure how used here..
			WORD m_spell;			// morex = SPELL_TYPE = The magic spell cast on this. (daemons breath)(boots of strength) etc
			WORD m_spelllevel;		// morey=level of the spell. (0-1000)
		} m_itArmor;

		// IT_SPELL = a magic spell effect. (might be equipped)
		// IT_FIRE
		// IT_SCROLL
		// IT_COMM_CRYSTAL
		// IT_CAMPFIRE
		// IT_LAVA
		struct
		{
			int  m_PolyStr;			// more1l=polymorph effect of this. (on strength)
			int  m_PolyDex;			// more1h=polymorph effect of this. (on dex)
			int  m_spellcharges;	// more2=not sure how used here..
			WORD m_spell;			// morex=SPELL_TYPE = The magic spell cast on this. (daemons breath)(boots of strength) etc
			WORD m_spelllevel;		// morey=0-1000=level of the spell.
			BYTE m_pattern;			// morez = light pattern - CAN_I_LIGHT LIGHT_QTY
		} m_itSpell;

		// IT_SPELLBOOK
		// IT_SPELLBOOK_NECRO
		// IT_SPELLBOOK_PALA
		// IT_SPELLBOOK_EXTRA
		// IT_SPELLBOOK_BUSHIDO
		// IT_SPELLBOOK_NINJITSU
		// IT_SPELLBOOK_ARCANIST
		// IT_SPELLBOOK_MYSTIC
		// IT_SPELLBOOK_MASTERY
		struct
		{
			DWORD m_spells1;	// more1=Mask of avail spells for spell book.
			DWORD m_spells2;	// more2=Mask of avail spells for spell book.
		} m_itSpellbook;

		// IT_POTION
		struct
		{
			SPELL_TYPE m_Type;		// more1 = potion effect type
			DWORD m_skillquality;	// more2 = 0-1000 Strength of the resulting spell.
			WORD m_tick;			// morex = countdown to explode purple.
			WORD m_junk4;
			BYTE m_ignited;
		} m_itPotion;

		// IT_MAP
		struct
		{
			WORD m_top;			// more1l=in world coords.
			WORD m_left;		// more1h=
			WORD m_bottom;		// more2l=
			WORD m_right;		// more2h=
			WORD m_junk3;
			WORD m_junk4;
			BYTE m_fPinsGlued;	// morez=pins are glued in place. Cannot be moved.
			BYTE m_map;			// morem=map
		} m_itMap;

		// IT_FRUIT
		// IT_FOOD
		// IT_FOOD_RAW
		// IT_MEAT_RAW
		struct
		{
			ITEMID_TYPE m_cook_id;		// more1=Cooks into this. (only if raw)
			WORD m_junk1;
			WORD m_spell;				// morex=SPELL_TYPE = The magic spell cast on this. ( effect of eating.)
			WORD m_spelllevel;			// morey=level of the spell. (0-1000)
			BYTE m_poison_skill;		// morez=0-100 = Is poisoned ?
			BYTE m_foodval;				// morem=food value to restore
		} m_itFood;

		// IT_DRINK
		struct
		{
			ITEMID_TYPE m_cook_id;		// more1=Cooks into this. (only if raw)
			WORD m_junk1;
			WORD m_spell;				// morex=SPELL_TYPE = The magic spell cast on this. ( effect of eating.)
			WORD m_spelllevel;			// morey=level of the spell. (0-1000)
			BYTE m_poison_skill;		// morez=0-100 = Is poisoned ?
			BYTE m_foodval;				// morem=food value to restore
		} m_itDrink;

		// IT_CORPSE
		struct	// might just be a sleeping person as well
		{
			DWORD			m_carved;		// more1 = Corpse is already carved? (0=not carved, 1=carved)
			CGrayUIDBase	m_uidKiller;	// more2 = Who killed this corpse, carved or looted it last. sleep=self.
			CREID_TYPE		m_BaseID;		// morex,morey = The true type of the creature who's corpse this is.
			DIR_TYPE		m_facing_dir;	// morez = Corpse dir. 0x80 = on face.
			// m_amount = the body type.
			// m_uidLink = the creatures ghost.
		} m_itCorpse;

		// IT_LIGHT_LIT
		// IT_LIGHT_OUT
		// IT_WINDOW
		struct
		{
			// CAN_I_LIGHT may be set for others as well..ie.Moon gate conflict
			DWORD	m_junk1;
			DWORD	m_junk2;
			BYTE	m_burned;	// morex = out of charges? (1=yes / 0=no)
			WORD	m_charges;	// morey = how long will the torch last ?
			BYTE	m_pattern;	// morez = light rotation pattern (LIGHT_PATTERN)
		} m_itLight;

		// IT_EQ_TRADE_WINDOW
		struct
		{
			DWORD	m_iGold;
			DWORD	m_iPlatinum;
			UINT64	m_iWaitTime;
			BYTE	m_bCheck;		// morez=Check box for trade window.
		} m_itEqTradeWindow;

		// IT_SPAWN_CHAR
		// IT_SPAWN_ITEM
		struct
		{
			RESOURCE_ID_BASE m_ItemID;	// more1=The ITEMID_* or template for items
			DWORD	m_pile;				// more2=The max # of items to spawn per interval.  If this is 0, spawn up to the total amount.
			WORD	m_TimeLoMin;		// morex=Lo time in minutes.
			WORD	m_TimeHiMin;		// morey=Hi time in minutes.
			BYTE	m_DistMax;			// morez=How far from this will it spawn?
		} m_itSpawnItem;

		// IT_SPAWN_CHAR
		struct
		{
			RESOURCE_ID_BASE m_CharID;	// more1=CREID_*,  or SPAWNTYPE_*,
			DWORD	m_junk1;		// more2=used only by IT_SPAWN_ITEM, keeping it only for mantaining the structure of the union.
			WORD	m_TimeLoMin;		// morex=Lo time in minutes.
			WORD	m_TimeHiMin;		// morey=Hi time in minutes.
			BYTE	m_DistMax;			// morez=How far from this will they wander?
		} m_itSpawnChar;

		// IT_EXPLOSION
		struct
		{
			DWORD	m_junk1;
			DWORD	m_junk2;
			WORD	m_iDamage;		// morex = damage of the explosion
			WORD	m_wFlags;		// morey = DAMAGE_TYPE = fire,magic,etc
			BYTE	m_iDist;		// morez = distance range of damage.
		} m_itExplode;	// Make this asyncronous.

		// IT_BOOK
		// IT_MESSAGE
		struct
		{
			RESOURCE_ID_BASE m_ResID;	// more1 = preconfigured book id from RES_BOOK or Time date stamp for the book/message creation. (if |0x80000000)
			//CServTime		 m_Time;	// more2= Time date stamp for the book/message creation. (Now Placed inside TIMESTAMP for INT64 support)
		} m_itBook;

		// IT_DEED
		struct
		{
			ITEMID_TYPE m_Type;		// more1 = deed for what multi, item or template ?
			DWORD		m_dwKeyCode;	// more2 = previous key code. (dry docked ship)
		} m_itDeed;

		// IT_CROPS
		// IT_FOLIAGE - the leaves of a tree normally.
		struct
		{
			int m_Respawn_Sec;		// more1 = plant respawn time in seconds. (for faster growth plants)
			ITEMID_TYPE m_ReapFruitID;	// more2 = What is the fruit of this plant.
			WORD m_ReapStages;		// morex = how many more stages of this to go til ripe.
		} m_itCrop;

		// IT_TREE
		// ? IT_ROCK
		// ? IT_WATER
		// ? IT_GRASS
		struct	// Natural resources. tend to be statics.
		{
			RESOURCE_ID_BASE m_rid_res;	// more1 = base resource type. RES_REGIONRESOURCE
		} m_itResource;

		// IT_FIGURINE
		// IT_EQ_HORSE
		struct
		{
			CREID_TYPE m_ID;	// more1 = What sort of creature will this turn into.
			CGrayUIDBase m_UID;	// more2 = If stored by the stables. (offline creature)
		} m_itFigurine;

		// IT_RUNE
		struct
		{
			int m_Charges;			// more1 = How many uses til a rune will wear out ?
			DWORD m_junk2;
			CPointBase m_ptMark;	// morep = rune marked to a location or a teleport ?
		} m_itRune;

		// IT_TELEPAD
		// IT_MOONGATE
		struct
		{
			int m_fPlayerOnly;		// more1 = The gate is player only. (no npcs, xcept pets)
			int m_fQuiet;			// more2 = The gate/telepad makes no noise.
			CPointBase m_ptMark;	// morep = marked to a location or a teleport ?
		} m_itTelepad;

		// IT_EQ_MEMORY_OBJ
		struct
		{
			// m_amount = memory type mask.
			WORD m_Action;		// more1l = NPC_MEM_ACT_TYPE What sort of action is this memory about ? (1=training, 2=hire, etc)
			WORD m_Skill;		// more1h = SKILL_TYPE = training a skill ?
			DWORD m_junk2;		// more2 = When did the fight start or action take place ? (Now Placed inside TIMESTAMP for INT64 support)
			CPointBase m_pt;	// morep = Location the memory occured.
			// m_uidLink = what is this memory linked to. (must be valid)
		} m_itEqMemory;

		// IT_MULTI
		// IT_MULTI_CUSTOM
		// IT_SHIP
		struct
		{
			CGrayUIDBase m_UIDCreator;	// more1 = who created this house or ship ?
			BYTE m_fSail;		// more2.b1 = ? speed ?
			BYTE m_fAnchored;
			BYTE m_DirMove;		// DIR_TYPE
			BYTE m_DirFace;
			// uidLink = my IT_SHIP_TILLER or IT_SIGN_GUMP,
			CGrayUIDBase m_Pilot;
		} m_itShip;

		// IT_SHIP_PLANK
		struct
		{
			CGrayUIDBase m_lockUID;		// more1 = the lock code. normally this is the same as the uid (magic lock=non UID)
			DWORD m_lock_complexity;	// more2=0-1000 = How hard to pick or magic unlock. (conflict with door ?)
			WORD m_itSideType;			// morex = type to become (IT_SHIP_SIDE or IT_SHIP_SIDE_LOCKED)
		} m_itShipPlank;

		// IT_PORTCULIS
		// IT_PORT_LOCKED
		struct
		{
			int m_z1;			// more1 = The down z height.
			int m_z2;			// more2 = The up z height.
		} m_itPortculis;

		// IT_BEE_HIVE
		struct
		{
			int m_honeycount;		// more1 = How much honey has accumulated here.
		} m_itBeeHive;

		// IT_LOOM
		struct
		{
			ITEMID_TYPE m_ClothID;	// more1 = the cloth type currenctly loaded here.
			WORD m_ClothQty;		// more2 = IS the loom loaded with cloth ?
		} m_itLoom;

		// IT_ARCHERY_BUTTE
		struct
		{
			ITEMID_TYPE m_AmmoType;	// more1 = arrow or bolt currently stuck in it.
			WORD m_AmmoCount;		// more2 = how many arrows or bolts ?
		} m_itArcheryButte;

		// IT_CANNON_MUZZLE
		struct
		{
			DWORD m_junk1;
			DWORD m_Load;			// more2 = Is the cannon loaded ? Mask = 1=powder, 2=shot
		} m_itCannon;

		// IT_EQ_MURDER_COUNT
		struct
		{
			DWORD m_Decay_Balance;	// more1 = For the murder flag, how much time is left ?
		} m_itEqMurderCount;

		// IT_EQ_STUCK
		struct
		{
			// LINK = what are we stuck to ?
		} m_itEqStuck;

		// IT_WEB
		struct
		{
			DWORD m_Hits_Cur;	// more1 = how much damage the web can take.
		} m_itWeb;

		// IT_TRAP
		// IT_TRAP_ACTIVE
		// IT_TRAP_INACTIVE
		struct
		{
			ITEMID_TYPE m_AnimID;	// more1 = What does a trap do when triggered. 0=just use the next id.
			int	m_Damage;			// more2 = Base damage for a trap.
			WORD m_wAnimSec;		// morex = How long to animate as a dangerous trap.
			WORD m_wResetSec;		// morey = How long to sit idle til reset.
			BYTE m_fPeriodic;		// morez = Does the trap just cycle from active to inactive ?
		} m_itTrap;

		// IT_ANIM_ACTIVE
		struct
		{
			// NOTE: This is slightly dangerous to use as it will overwrite more1 and more2
			ITEMID_TYPE m_PrevID;	// more1 = What to turn back into after the animation.
			IT_TYPE m_PrevType;	// more2 = Any item that will animate.	??? Get rid of this !!
		} m_itAnim;

		// IT_SWITCH
		struct
		{
			ITEMID_TYPE m_SwitchID;	// more1 = the next state of this switch.
			DWORD		m_junk2;
			WORD		m_fStep;	// morex = can we just step on this to activate ?
			WORD		m_wDelay;	// morey = delay this how long before activation.
			// uidLink = the item to use when this item is thrown or used.
		} m_itSwitch;

		// IT_SOUND
		struct
		{
			DWORD	m_Sound;	// more1 = SOUND_TYPE
			BYTE	m_Repeat;	// more2 =
		} m_itSound;

		// IT_STONE_GUILD
		// IT_STONE_TOWN
		struct
		{
			STONEALIGN_TYPE m_iAlign;	// more1=Neutral, chaos, order.
			int m_iAccountGold;			// more2=How much gold has been dropped on me?
			// ATTR_OWNED = auto promote to member.
		} m_itStone;

	};	// IT_QTY

protected:
	CItem( ITEMID_TYPE id, CItemBase * pItemDef );	// only created via CreateBase()
public:
	virtual ~CItem();
private:
	CItem(const CItem& copy);
	CItem& operator=(const CItem& other);

protected:
	bool SetBase( CItemBase * pItemDef );
	virtual int FixWeirdness();

public:
	virtual bool OnTick();
	virtual void OnHear( LPCTSTR pszCmd, CChar * pSrc )
	{
		// This should never be called directly. Normal items cannot hear. IT_SHIP and IT_COMM_CRYSTAL
		UNREFERENCED_PARAMETER(pszCmd);
		UNREFERENCED_PARAMETER(pSrc);
	}

	CItemBase *Item_GetDef() const
	{
		return static_cast<CItemBase *>(Base_GetDef());
	}

	ITEMID_TYPE GetID() const
	{
		const CItemBase *pItemDef = Item_GetDef();
		ASSERT(pItemDef);
		return pItemDef->GetID();
	}
	WORD GetBaseID() const
	{
		// future: strongly typed enums will remove the need for this cast
		return static_cast<WORD>(GetID());
	}
	bool SetBaseID( ITEMID_TYPE id );
	bool SetID( ITEMID_TYPE id );
	ITEMID_TYPE GetDispID() const
	{
		// This is what the item looks like.
		// May not be the same as the item that defines it's type.
		return m_dwDispIndex;
	}
	bool IsSameDispID( ITEMID_TYPE id ) const	// account for flipped types ?
	{
		const CItemBase *pItemDef = Item_GetDef();
		ASSERT(pItemDef);
		return pItemDef->IsSameDispID(id);
	}
	bool SetDispID( ITEMID_TYPE id );
	void SetAnim( ITEMID_TYPE id, int iTime );

	int IsWeird() const;
	signed char GetFixZ(CPointMap pt, DWORD dwBlockFlags = 0);
	BYTE GetSpeed() const;
	void SetAttr( DWORD dwAttr )
	{
		m_Attr |= dwAttr;
	}
	void ClrAttr( DWORD dwAttr )
	{
		m_Attr &= ~dwAttr;
	}
	bool IsAttr( DWORD dwAttr ) const	// ATTR_DECAY
	{
		return (m_Attr & dwAttr) ? true : false;
	}

	height_t GetHeight() const;
	INT64  GetDecayTime() const;
	void SetDecayTime( INT64 iTime = 0 );
	SOUND_TYPE GetDropSound(const CObjBase *pObjOn) const;
	bool IsTopLevelMultiLocked() const;
	bool IsMovableType() const;
	bool IsMovable() const;
	bool IsStackableException() const;
	bool IsStackable( const CItem * pItem ) const;
	bool IsStackableType() const
	{
		return Can(CAN_I_PILE);
	}

	bool Can(DWORD dwCan) const
	{
		return (m_Can & dwCan) ? true : false;
	}
	virtual bool  IsSameType( const CObjBase * pObj ) const;
	virtual bool  IsIdentical( const CObjBase * pObj );
	bool Stack( CItem * pItem );
	DWORD ConsumeAmount( DWORD iQty = 1, bool fTest = false );

	CREID_TYPE GetCorpseType() const
	{
		return static_cast<CREID_TYPE>(GetAmount());	// What does the corpse look like ?
	}
	void  SetCorpseType( CREID_TYPE id )
	{
		// future: strongly typed enums will remove the need for this cast
		m_amount = static_cast<WORD>(id);	// m_corpse_DispID
	}
	void SetAmount( WORD amount );
	WORD GetMaxAmount();
	bool SetMaxAmount( WORD amount );
	void SetAmountUpdate( WORD amount );
	WORD GetAmount() const { return m_amount; }

	LPCTSTR GetName() const;	// allowed to be default name.
	LPCTSTR GetNameFull( bool fIdentified ) const;

	virtual bool SetName( LPCTSTR pszName );

	virtual int GetWeight(WORD wAmount = 0) const
	{
		WORD wWeight = m_weight * (wAmount ? wAmount : GetAmount());
		CVarDefCont *pReduction = GetDefKey("WEIGHTREDUCTION", true);
		if ( pReduction )
			wWeight -= static_cast<WORD>(IMULDIV(wWeight, maximum(pReduction->GetValNum(), 0), 100));
		return wWeight;
	}

	void SetTimeout( INT64 iDelay );

	virtual void OnMoveFrom()	// Moving from current location.
	{
	}
	virtual bool MoveTo(CPointMap pt, bool bForceFix = false); // Put item on the ground here.
	bool MoveToUpdate(CPointMap pt, bool bForceFix = false)
	{
		bool bReturn = MoveTo(pt, bForceFix);
		Update();
		return bReturn;
	}
	bool MoveToDecay(const CPointMap & pt, INT64 iDecayTime, bool bForceFix = false)
	{
		SetDecayTime( iDecayTime );
		return MoveToUpdate( pt, bForceFix);
	}
	bool MoveToCheck( const CPointMap & pt, CChar * pCharMover = NULL );
	virtual bool MoveNearObj( const CObjBaseTemplate *pItem, WORD iSteps = 0 );

	CItem *GetNext() const
	{
		return static_cast<CItem *>(CObjBase::GetNext());
	}
	CItem *GetPrev() const
	{
		return static_cast<CItem *>(CObjBase::GetPrev());
	}
	CObjBase *GetParentObj() const
	{
		// What is this CItem contained in ?
		// Container should be a CChar or CItemContainer
		return dynamic_cast<CObjBase *>(GetParent());
	}
	CObjBaseTemplate *GetTopLevelObj() const
	{
		// recursively get the item that is at "top" level.
		const CObjBase *pObj = GetParentObj();
		if ( !pObj )
			return const_cast<CItem *>(this);
		if ( pObj == this )		// to avoid script errors setting same CONT
			return const_cast<CItem *>(this);
		return pObj->GetTopLevelObj();
	}

	BYTE GetContainedGridIndex() const
	{
		return m_containedGridIndex;
	}

	void SetContainedGridIndex(BYTE index)
	{
		m_containedGridIndex = index;
	}

	void  Update( const CClient * pClientExclude = NULL );		// send this new item to everyone.
	void  Flip();
	bool  LoadSetContainer( CGrayUID uid, LAYER_TYPE layer );

	void WriteUOX( CScript & s, int index );

	void r_WriteMore1( CGString & sVal );
	void r_WriteMore2( CGString & sVal );

	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual void  r_Write( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & s, CTextConsole * pSrc );
	virtual bool  r_LoadVal( CScript & s  );
	virtual bool  r_Load( CScript & s ); // Load an item from script
	virtual bool  r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script

private:
	TRIGRET_TYPE OnTrigger( LPCTSTR pszTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs );
	TRIGRET_TYPE OnTriggerCreate(CTextConsole * pSrc, CScriptTriggerArgs * pArgs );

public:
	TRIGRET_TYPE OnTrigger( ITRIG_TYPE trigger, CTextConsole * pSrc, CScriptTriggerArgs * pArgs = NULL )
	{
		ASSERT( trigger < ITRIG_QTY );
		return OnTrigger(MAKEINTRESOURCE(trigger), pSrc, pArgs);
	}

	// Item type specific stuff.
	bool IsType( IT_TYPE type ) const
	{
		return (m_type == type);
	}
	IT_TYPE GetType() const
	{
		return m_type;
	}
	CItem * SetType( IT_TYPE type );
	bool IsTypeLit() const
	{
		// has m_pattern arg
		switch(m_type)
		{
			case IT_SPELL: // a magic spell effect. (might be equipped)
			case IT_FIRE:
			case IT_LIGHT_LIT:
			case IT_CAMPFIRE:
			case IT_LAVA:
			case IT_WINDOW:
				return true;
			default:
				return false;
		}
	}
	bool IsTypeBook() const
	{
		switch( m_type )
		{
			case IT_BOOK:
			case IT_MESSAGE:
				return true;
			default:
				return false;
		}
	}
	bool IsTypeSpellbook() const
	{
		return CItemBase::IsTypeSpellbook(m_type);
	}
	bool IsTypeArmor() const
	{
		return CItemBase::IsTypeArmor(m_type);
	}
	bool IsTypeWeapon() const
	{
		return CItemBase::IsTypeWeapon(m_type);
	}
	bool IsTypeMulti() const
	{
		return CItemBase::IsTypeMulti(m_type);
	}
	bool IsTypeArmorWeapon() const
	{
		// Armor or weapon.
		return (IsTypeArmor() || IsTypeWeapon());
	}
	bool IsTypeLocked() const
	{
		switch ( m_type )
		{
			case IT_SHIP_SIDE_LOCKED:
			case IT_CONTAINER_LOCKED:
			case IT_SHIP_HOLD_LOCK:
			case IT_DOOR_LOCKED:
				return true;
			default:
				return false;
		}
	}
	bool IsTypeLockable() const
	{
		switch( m_type )
		{
			case IT_CONTAINER:
			case IT_DOOR:
			case IT_DOOR_OPEN:
			case IT_SHIP_SIDE:
			case IT_SHIP_PLANK:
			case IT_SHIP_HOLD:
			//case IT_ROPE:
				return true;
			default:
				return IsTypeLocked();
		}
	}
	bool IsTypeSpellable() const
	{
		// m_itSpell
		switch( m_type )
		{
			case IT_SCROLL:
			case IT_SPELL:
			case IT_FIRE:
				return true;
			default:
				return IsTypeArmorWeapon();
		}
	}

	bool IsResourceMatch( RESOURCE_ID_BASE rid, DWORD dwArg );

	bool IsValidLockLink( CItem * pItemLock ) const;
	bool IsValidLockUID() const;
	bool IsKeyLockFit( DWORD dwLockUID ) const
	{
		return (m_itKey.m_lockUID == dwLockUID);
	}

	void ConvertBolttoCloth();

	// Spells
	SPELL_TYPE GetScrollSpell() const;
	bool IsSpellInBook( SPELL_TYPE spell ) const;
	int GetSpellcountInBook() const;
	int  AddSpellbookScroll( CItem * pItem );
	int AddSpellbookSpell( SPELL_TYPE spell, bool fUpdate );

	//Doors
	bool IsDoorOpen() const;
	bool Use_Door( bool bJustOpen );
	bool Use_DoorNew( bool bJustOpen );
	bool Use_Portculis();
	SOUND_TYPE Use_Music( bool fWell ) const;

	bool SetMagicLock( CChar * pCharSrc, int iSkillLevel );
	void SetSwitchState();
	void SetTrapState( IT_TYPE state, ITEMID_TYPE id, int iTimeSec );
	int Use_Trap();
	bool Use_Light();
	int Use_LockPick( CChar * pCharSrc, bool fTest, bool fFail );
	LPCTSTR Use_SpyGlass( CChar * pUser ) const;
	LPCTSTR Use_Sextant( CPointMap pntCoords ) const;

	bool IsBookWritable() const
	{
		return (m_itBook.m_ResID.GetPrivateUID() == 0 && GetTimeStamp().GetTimeRaw() == 0);
	}
	bool IsBookSystem() const	// stored in RES_BOOK
	{
		return (m_itBook.m_ResID.GetResType() == RES_BOOK);
	}

	void OnExplosion();
	bool OnSpellEffect( SPELL_TYPE spell, CChar * pCharSrc, int iSkillLevel, CItem * pSourceItem, bool bReflecting = false );
	int OnTakeDamage( int iDmg, CChar * pSrc, DAMAGE_TYPE uType = DAMAGE_HIT_BLUNT );

	int Armor_GetRepairPercent() const;
	LPCTSTR Armor_GetRepairDesc() const;
	bool Armor_IsRepairable() const;
	int Armor_GetDefense() const;
	int Weapon_GetAttack(bool bGetRange = true) const;
	SKILL_TYPE Weapon_GetSkill() const;
	SOUND_TYPE Weapon_GetSoundHit() const;
	SOUND_TYPE Weapon_GetSoundMiss() const;
	void Weapon_GetRangedAmmoAnim(ITEMID_TYPE &id, DWORD &hue, DWORD &render);
	RESOURCE_ID_BASE Weapon_GetRangedAmmoRes();
	CItem *Weapon_FindRangedAmmo(RESOURCE_ID_BASE id);

	bool IsMemoryTypes( WORD wType ) const
	{
		// MEMORY_FIGHT
		if ( !IsType(IT_EQ_MEMORY_OBJ) )
			return false;
		return (GetHueAlt() & wType) ? true : false;
	}

	bool Ship_Plank( bool fOpen );

	void Plant_SetTimer();
	bool Plant_OnTick();
	void Plant_CropReset();
	bool Plant_Use( CChar * pChar );

	virtual void DupeCopy( const CItem * pItem );
	CItem * UnStackSplit( WORD amount, CChar * pCharSrc = NULL );

	static CItem * CreateBase( ITEMID_TYPE id );
	static CItem * CreateHeader( TCHAR * pArg, CObjBase * pCont = NULL, bool fDupeCheck = false, CChar * pSrc = NULL );
	static CItem * CreateScript(ITEMID_TYPE id, CChar * pSrc = NULL);
	CItem * GenerateScript(CChar * pSrc = NULL);
	static CItem * CreateDupeItem( const CItem * pItem, CChar * pSrc = NULL, bool fSetNew = false );
	static CItem * CreateTemplate( ITEMID_TYPE id, CObjBase* pCont = NULL, CChar * pSrc = NULL );

	static CItem * ReadTemplate( CResourceLock & s, CObjBase * pCont );
	
	virtual void Delete(bool bforce = false);
	virtual bool NotifyDelete();
};

class CItemSpawn : public CItem
{
private:
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];
	CGrayUID m_obj[UCHAR_MAX];	///< Storing UIDs of the created items/chars.

public:
	static const char *m_sClassName;

	/* I don't want to inherit SetAmount, GetAmount and m_iAmount from the parent CItem class. I need to redefine them for CItemSpawn class
	*	so that when i set AMOUNT to the spawn item, i don't really set the "item amount/quantity" property, but the "spawn item AMOUNT" property.
	*	This way, even if there is a stackable spawn item (default in Enhanced Client), i won't increase the item stack quantity and i can't pick
	*	from pile the spawn item. Plus, since the max amount of spawnable objects per single spawn item is the max size of a BYTE, we can change
	*	the data type accepted/returned.
	*/
	void SetAmount(BYTE iAmount);
	BYTE GetAmount();
	BYTE m_iAmount;				// Amount of objects to spawn.
	BYTE m_currentSpawned;		// Amount of current objects already spawned. Get it from scripts via COUNT property (read-only).

	/**
	* @brief Overrides onTick for this class.
	*
	* Setting time again
	* stoping if more2 >= amount
	* more1 Resource Check
	* resource (item/char) generation
	*/
	void OnTick( bool fExec );

	/**
	* @brief Removes everything created by this spawn, if still belongs to the spawn.
	*/
	void KillChildren();

	/**
	* @brief Setting display ID based on Character's Figurine or the default display ID if this is an IT_SPAWN_ITEM.
	*/
	CCharBase * SetTrackID();

	/**
	* @brief Generate a *pDef item from this spawn.
	*
	* @param pDef resource to create
	*/
	void GenerateItem( CResourceDef * pDef );

	/**
	* @brief Generate a *pDef char from this spawn.
	*
	* @param pDef resource to create
	*/
	void GenerateChar( CResourceDef * pDef );

	/**
	* @brief Gets the total count of items or chars created which are still considered as 'spawned' by this spawn.
	*
	* @return the count of items/chars.
	*/
	BYTE GetCount();

	/**
	* @brief Removing one UID in Spawn's m_obj[].
	*
	* @param UID of the obj to remove.
	*/
	void DelObj( CGrayUID uid );

	/**
	* @brief Storing one UID in Spawn's m_obj[].
	*
	* @param UID of the obj to add.
	*/
	void AddObj( CGrayUID uid );

	/**
	* @brief Test if the character from more1 exists.
	*
	* @param ID of the char to check.
	*/
	inline CCharBase * TryChar( CREID_TYPE &id );

	/**
	* @brief Test if the item from more1 exists.
	*
	* @param ID of the item to check.
	*/
	inline CItemBase * TryItem( ITEMID_TYPE &id );

	/**
	* @brief Get a proper RESOURCE_ID from the id provided.
	*
	* @return a valid RESOURCE_ID.
	*/
	CResourceDef * FixDef();

	/**
	* @brief Gets the name of the resource created (item or char).
	*
	* @return the name of the resource.
	*/
	int GetName(TCHAR * pszOut) const;

	CItemSpawn(ITEMID_TYPE id , CItemBase * pItemDef);
	virtual ~CItemSpawn();
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc);
	virtual bool  r_LoadVal(CScript & s);
	virtual void  r_Write(CScript & s);
};

class CItemVendable : public CItem
{
	// Any item that can be sold and has value.
private:
	static LPCTSTR const sm_szLoadKeys[];
	WORD m_quality;		// 0-100 quality.
	DWORD m_price;		// The price of this item if on a vendor. (allow random (but remembered) pluctuations)

public:
	static const char *m_sClassName;
	CItemVendable( ITEMID_TYPE id, CItemBase * pItemDef );
	virtual ~CItemVendable();

private:
	CItemVendable(const CItemVendable& copy);
	CItemVendable& operator=(const CItemVendable& other);

public:

	WORD	GetQuality() const {return m_quality;}
	void	SetQuality(WORD quality = 0)
	{
		m_quality = quality;
	}

	void SetPlayerVendorPrice( DWORD dwVal );
	DWORD GetBasePrice() const;
	DWORD GetVendorPrice( int iConvertFactor );

	bool  IsValidSaleItem( bool fBuyFromVendor ) const;
	bool  IsValidNPCSaleItem() const;

	virtual void DupeCopy( const CItem * pItem );

	void	Restock( bool fSellToPlayers );
	virtual void  r_Write( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	virtual bool  r_LoadVal( CScript & s  );
};

class CContainer : public CGObList	// this class contains a list of items but may or may not be an item itself
{
public:
	static const char *m_sClassName;
	CContainer()
	{
		m_totalweight = 0;
	}
	virtual ~CContainer()
	{
		DeleteAll();	// call this early so the virtuals will work
	}

private:
	int	m_totalweight;	// weight of all the items it has (1 / WEIGHT_UNITS stones)

protected:
	virtual void OnRemoveOb(CGObListRec *pObRec);	// override this = called when removed from list
	void ContentAddPrivate(CItem *pItem);

	void r_WriteContent(CScript &s) const;

	bool r_WriteValContainer(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	bool r_GetRefContainer(LPCTSTR &pszKey, CScriptObj *&pRef);

private:
	CContainer(const CContainer &copy);
	CContainer &operator=(const CContainer &other);

public:
	CItem *GetAt(size_t index) const
	{
		return dynamic_cast<CItem *>(CGObList::GetAt(index));
	}
	CItem *GetContentHead() const
	{
		return static_cast<CItem *>(GetHead());
	}
	CItem *GetContentTail() const
	{
		return static_cast<CItem *>(GetTail());
	}
	int	GetTotalWeight() const
	{
		return m_totalweight;
	}
	int FixWeight();

	bool ContentFindKeyFor(CItem *pLocked) const;

	void ContentsDump(const CPointMap &pt, DWORD dwAttrLeave = 0);
	void ContentsTransfer(CItemContainer *pCont, bool fNoNewbie);
	void ContentAttrMod(DWORD dwAttr, bool fSet);
	void ContentNotifyDelete();

	// For resource usage and gold.
	CItem *ContentFind(RESOURCE_ID_BASE rid, DWORD dwArg = 0, int iDecendLevels = 255) const;
	TRIGRET_TYPE OnContTriggerForLoop(CScript &s, CTextConsole *pSrc, CScriptTriggerArgs *pArgs, CGString *pResult, CScriptLineContext &StartContext, CScriptLineContext &EndContext, RESOURCE_ID_BASE rid, DWORD dwArg = 0, int iDecendLevels = 255);
	TRIGRET_TYPE OnGenericContTriggerForLoop(CScript &s, CTextConsole *pSrc, CScriptTriggerArgs *pArgs, CGString *pResult, CScriptLineContext &StartContext, CScriptLineContext &EndContext, int iDecendLevels = 255);
	DWORD ContentCount(RESOURCE_ID_BASE rid, DWORD dwArg = 0);
	DWORD ContentCountAll() const;
	DWORD ContentConsume(RESOURCE_ID_BASE rid, DWORD dwQty = 1, bool fTest = false, DWORD dwArg = 0);

	DWORD ResourceConsume(const CResourceQtyArray *pResources, DWORD dwReplicationQty, bool fTest = false, DWORD dwArg = 0);
	size_t ResourceConsumePart(const CResourceQtyArray *pResources, DWORD dwReplicationQty, int iDamagePercent, bool fTest = false, DWORD dwArg = 0);

	virtual void OnWeightChange(int iChange);
	virtual void ContentAdd(CItem *pItem) = 0;
};

class CItemContainer : public CItemVendable, public CContainer
{
	// This item has other items inside it
	static LPCTSTR const sm_szVerbKeys[];
public:
	static const char *m_sClassName;
	CItemContainer(ITEMID_TYPE id, CItemBase *pItemDef) : CItemVendable(id, pItemDef)
	{
	}
	virtual ~CItemContainer()
	{
		DeleteAll();	// get rid of my contents first to protect against weight calc errors
		DeletePrepare();
	}

private:
	CItemContainer(const CItemContainer &copy);
	CItemContainer &operator=(const CItemContainer &other);

public:
	bool NotifyDelete();
	void DeletePrepare()
	{
		if ( IsType(IT_EQ_TRADE_WINDOW) )
			Trade_Delete();
		CItem::DeletePrepare();
	}

	bool IsWeighed() const
	{
		if ( IsType(IT_EQ_BANK_BOX) || IsType(IT_EQ_VENDOR_BOX) || IsAttr(ATTR_MAGIC) )		// magic containers have no weight
			return false;
		return true;
	}
	bool IsSearchable() const
	{
		if ( IsType(IT_EQ_BANK_BOX) || IsType(IT_EQ_VENDOR_BOX) || IsType(IT_CONTAINER_LOCKED) || IsType(IT_EQ_TRADE_WINDOW) )
			return false;
		return true;
	}

	bool IsItemInside(const CItem *pItem) const;
	bool CanContainerHold(const CItem *pItem, const CChar *pCharMsg);

	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);
	virtual void r_Write(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	virtual bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);

	virtual int GetWeight(WORD wAmount = 0) const
	{
		// True weight = container weight + contents weight
		return CItem::GetWeight(wAmount) + CContainer::GetTotalWeight();
	}
	void OnWeightChange(int iChange);

	void ContentAdd(CItem *pItem);
	void ContentAdd(CItem *pItem, CPointMap pt, BYTE gridIndex = 0);

protected:
	void OnRemoveOb(CGObListRec *pObRec);	// override this = called when removed from list

public:
	bool IsItemInTrade();
	void Trade_Status(bool bCheck);
	void Trade_UpdateGold(DWORD dwPlatinum, DWORD dwGold);
	void Trade_Delete();

	void MakeKey();
	void SetKeyRing();
	void Game_Create();
	void Restock();
	bool OnTick();

	virtual void DupeCopy(const CItem *pItem);

	CPointMap GetRandContainerLoc() const;
	SOUND_TYPE GetDropSound() const;

	void OnOpenEvent(CChar *pCharOpener, const CObjBaseTemplate *pObjTop);
};

class CItemScript : public CItemVendable	// A message for a bboard or book text.
{
	// IT_SCRIPT, IT_EQ_SCRIPT
public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];
public:
	CItemScript( ITEMID_TYPE id, CItemBase * pItemDef ) : CItemVendable( id, pItemDef )
	{
	}
	virtual ~CItemScript()
	{
		DeletePrepare();	// Must remove early because virtuals will fail in child destructor.
	}

private:
	CItemScript(const CItemScript& copy);
	CItemScript& operator=(const CItemScript& other);

public:
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc );	// some command on this object as a target
	virtual void r_Write( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc = NULL );
	virtual bool r_LoadVal( CScript & s );
	virtual void DupeCopy( const CItem * pItem );
};

class CItemCorpse : public CItemContainer
{
	// A corpse is a special type of item (IT_CORPSE)
public:
	static const char *m_sClassName;
	CItemCorpse(ITEMID_TYPE id, CItemBase *pItemDef) : CItemContainer(id, pItemDef)
	{
	}
	virtual ~CItemCorpse()
	{
		DeletePrepare();	// must remove early because virtuals will fail in child destructor
	}

private:
	CItemCorpse(const CItemCorpse &copy);
	CItemCorpse &operator=(const CItemCorpse &other);

public:
	CChar *IsCorpseSleeping() const;

	int GetWeight(WORD wAmount = 0) const
	{
		UNREFERENCED_PARAMETER(wAmount);
		// GetAmount is messed up
		// True weight = corpse weight + contents weight
		return 1 + CContainer::GetTotalWeight();
	}
};

#define MAX_MULTI_LIST_OBJS 128
#define MAX_MULTI_CONTENT 1024

class CItemMulti : public CItem
{
	// IT_MULTI IT_SHIP
	// A ship or house etc.
private:
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];

protected:
	bool Multi_IsPartOf( const CItem * pItem ) const;
	CItem * Multi_FindItemType( IT_TYPE type ) const;
	CItem * Multi_FindItemComponent( int iComp ) const;

	const CItemBaseMulti *Multi_GetDef() const
	{
		return static_cast<const CItemBaseMulti *>(Base_GetDef());
	}
	bool Multi_CreateComponent( ITEMID_TYPE id, signed short dx, signed short dy, signed char dz, DWORD dwKeyCode );

public:
	CRegionWorld *m_pRegion;	// we own this region
	bool MultiRealizeRegion();
	void MultiUnRealizeRegion();
	int Multi_GetMaxDist() const;
	struct ShipSpeed // speed of a ship
	{
		BYTE period;	// time between movement
		BYTE tiles;		// distance to move
	};
	ShipSpeed m_shipSpeed; // Speed of ships (IT_SHIP)
	BYTE m_SpeedMode;

protected:
	virtual void OnComponentCreate( const CItem * pComponent )
	{
		UNREFERENCED_PARAMETER(pComponent);
	};


public:
	static const char *m_sClassName;
	CItemMulti( ITEMID_TYPE id, CItemBase * pItemDef );
	virtual ~CItemMulti();

private:
	CItemMulti(const CItemMulti& copy);
	CItemMulti& operator=(const CItemMulti& other);

public:
	virtual bool OnTick();
	virtual bool MoveTo(CPointMap pt, bool bForceFix = false); // Put item on the ground here.
	virtual void OnMoveFrom();	// Moving from current location.
	void OnHearRegion( LPCTSTR pszCmd, CChar * pSrc );
	CItem * Multi_GetSign();	// or Tiller

	void Multi_Create( CChar * pChar, DWORD dwKeyCode );
	static const CItemBaseMulti * Multi_GetDef( ITEMID_TYPE id );

	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script

	virtual void  r_Write( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	virtual bool  r_LoadVal( CScript & s  );
	virtual void DupeCopy( const CItem * pItem );
};

class PacketHouseDesign;

class CItemMultiCustom : public CItemMulti
{
	// IT_MULTI_CUSTOM
	// A customizable multi
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
		DWORD m_iRevision;
		ComponentsContainer m_vectorComponents;
		PacketHouseDesign *m_pData;
		DWORD m_iDataRevision;
	};
	
	class CGrayMultiCustom : public CGrayMulti
	{
	public:
		void LoadFrom(DesignDetails * pDesign);

	public:
		CGrayMultiCustom() { };
	private:
		CGrayMultiCustom(const CGrayMultiCustom& copy);
		CGrayMultiCustom& operator=(const CGrayMultiCustom& other);
	};

private:
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];

	DesignDetails m_designMain;
	DesignDetails m_designWorking;
	DesignDetails m_designBackup;
	DesignDetails m_designRevert;

	CClient *m_pArchitect;
	CRectMap m_rectDesignArea;
	CGrayMultiCustom *m_pGrayMulti;

	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual void r_Write( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	virtual bool r_LoadVal( CScript & s  );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script

	const CPointMap GetComponentPoint(Component * pComponent) const;
	const CPointMap GetComponentPoint(signed short dx, signed short dy, signed char dz) const;
	void CopyDesign(DesignDetails * designFrom, DesignDetails * designTo);

private:
	typedef std::map<ITEMID_TYPE,int> ValidItemsContainer;	// ItemID, FeatureMask
	static ValidItemsContainer sm_mapValidItems;
	static bool LoadValidItems();

public:
	static const char *m_sClassName;
	CItemMultiCustom( ITEMID_TYPE id, CItemBase * pItemDef );
	virtual ~CItemMultiCustom();

private:
	CItemMultiCustom(const CItemMultiCustom& copy);
	CItemMultiCustom& operator=(const CItemMultiCustom& other);

public:
	void BeginCustomize( CClient * pClientSrc );
	void EndCustomize( bool bForce = false );
	void SwitchToLevel( CClient * pClientSrc, BYTE iLevel );
	void CommitChanges( CClient * pClientSrc = NULL );
	void AddItem( CClient * pClientSrc, ITEMID_TYPE id, signed short x, signed short y, signed char z = SCHAR_MIN, short iStairID = 0 );
	void AddStairs( CClient * pClientSrc, ITEMID_TYPE id, signed short x, signed short y, signed char z = SCHAR_MIN, short iStairID = -1 );
	void AddRoof( CClient * pClientSrc, ITEMID_TYPE id, signed short x, signed short y, signed char z );
	void RemoveItem( CClient * pClientSrc, ITEMID_TYPE id, signed short x, signed short y, signed char z );
	bool RemoveStairs( Component * pStairComponent );
	void RemoveRoof( CClient * pClientSrc, ITEMID_TYPE id, signed short x, signed short y, signed char z );
	void SendVersionTo( CClient * pClientSrc );
	void SendStructureTo( CClient * pClientSrc );
	void BackupStructure();
	void RestoreStructure( CClient * pClientSrc = NULL );
	void RevertChanges( CClient * pClientSrc = NULL );
	void ResetStructure( CClient * pClientSrc = NULL );

	const CGrayMultiCustom * GetMultiItemDefs();
	const CGRect GetDesignArea();
	size_t GetFixtureCount(DesignDetails * pDesign = NULL);
	size_t GetComponentsAt(signed short dx, signed short dy, signed char dz, Component ** pComponents, DesignDetails * pDesign = NULL);
	DWORD GetRevision(const CClient * pClientSrc = NULL) const;
	BYTE GetLevelCount();
	WORD GetStairID();

	static BYTE GetPlane( signed char z );
	static BYTE GetPlane( Component * pComponent );
	static signed char GetPlaneZ( BYTE plane );
	static bool IsValidItem( ITEMID_TYPE id, CClient * pClientSrc, bool bMulti );
};

class CItemShip : public CItemMulti
{
	// IT_SHIP
	// A ship
private:
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];

	CGrayUID m_uidHold;
	std::vector<CGrayUID> m_uidPlanks;
	CServTime m_NextMove;


	int Ship_GetFaceOffset() const
	{
		return (GetID() & 3);
	}
	size_t  Ship_ListObjs( CObjBase ** ppObjList );
	bool Ship_CanMoveTo( const CPointMap & pt ) const;
	bool Ship_MoveDelta( CPointBase pdelta );
	bool Ship_OnMoveTick();

	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual void r_Write( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	virtual bool r_LoadVal( CScript & s  );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script
	virtual int FixWeirdness();
	virtual void OnComponentCreate( const CItem * pComponent );

public:
	bool Ship_SetMoveDir(DIR_TYPE dir, BYTE speed = 0, bool bWheelMove = false);
	bool Ship_Face(DIR_TYPE dir);
	bool Ship_Move(DIR_TYPE dir, int distance);
	static const char *m_sClassName;
	CItemShip( ITEMID_TYPE id, CItemBase * pItemDef );
	virtual ~CItemShip();

private:
	CItemShip(const CItemShip& copy);
	CItemShip& operator=(const CItemShip& other);

public:
	virtual bool OnTick();
	void Ship_Stop();
	CItemContainer * GetShipHold();
	size_t GetShipPlankCount();
	CItem * GetShipPlank(size_t index);
	//CItemBaseMulti::ShipSpeed GetShipSpeed();
};

class CItemStone;

class CItemMemory : public CItem
{
	// IT_EQ_MEMORY
	// Allow extra tags for the memory
public:
	static const char *m_sClassName;
	CItemMemory( ITEMID_TYPE id, CItemBase * pItemDef ) :
		CItem( ITEMID_MEMORY, pItemDef )
	{
		UNREFERENCED_PARAMETER(id);
	}
	
	virtual ~CItemMemory()
	{
		DeletePrepare();	// Must remove early because virtuals will fail in child destructor.
	}

private:
	CItemMemory(const CItemMemory& copy);
	CItemMemory& operator=(const CItemMemory& other);
	
public:
	WORD SetMemoryTypes( WORD wType )	// For memory type objects.
	{
		SetHueAlt(wType);
		return wType;
	}
	
	WORD GetMemoryTypes() const
	{
		return GetHueAlt();		// MEMORY_FIGHT
	}
	
	bool Guild_IsAbbrevOn() const;
	void Guild_SetAbbrev( bool fAbbrevShow );
	WORD Guild_GetVotes() const;
	void Guild_SetVotes( WORD wVotes );
	int Guild_SetLoyalTo( CGrayUID uid );
	CGrayUID Guild_GetLoyalTo() const;
	int Guild_SetTitle( LPCTSTR pszTitle );
	LPCTSTR Guild_GetTitle() const;
	CItemStone * Guild_GetLink();

	virtual int FixWeirdness();
};

struct CMapPinRec // Pin on a map
{
	WORD m_x;
	WORD m_y;

public:
	CMapPinRec(WORD x, WORD y) : m_x(x), m_y(y)
	{
	}
};

class CItemMap : public CItemVendable
{
	// IT_MAP
public:
	static const char *m_sClassName;
	enum
	{
		MAX_PINS = 128,
		DEFAULT_SIZE = 200
	};

	bool m_fPlotMode;	// should really be per-client based but oh well.
	CGTypedArray<CMapPinRec,CMapPinRec&> m_Pins;

public:
	CItemMap( ITEMID_TYPE id, CItemBase * pItemDef ) : CItemVendable( id, pItemDef )
	{
		m_fPlotMode = false;
	}
	virtual ~CItemMap()
	{
		DeletePrepare();	// Must remove early because virtuals will fail in child destructor.
	}

private:
	CItemMap(const CItemMap& copy);
	CItemMap& operator=(const CItemMap& other);

public:
	virtual bool IsSameType( const CObjBase * pObj ) const;
	virtual void r_Write( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc = NULL );
	virtual bool r_LoadVal( CScript & s );
	virtual void DupeCopy( const CItem * pItem );
};

class CItemCommCrystal : public CItemVendable
{
	// STATF_COMM_CRYSTAL and IT_COMM_CRYSTAL
	// What speech blocks does it like ?
protected:
	static LPCTSTR const sm_szLoadKeys[];
	CResourceRefArray m_Speech;	// Speech fragment list (other stuff we know)
public:
	static const char *m_sClassName;
	CItemCommCrystal( ITEMID_TYPE id, CItemBase * pItemDef ) : CItemVendable( id, pItemDef )
	{
	}
	virtual ~CItemCommCrystal()
	{
		DeletePrepare();	// Must remove early because virtuals will fail in child destructor.
	}

private:
	CItemCommCrystal(const CItemCommCrystal& copy);
	CItemCommCrystal& operator=(const CItemCommCrystal& other);

public:
	virtual void OnMoveFrom();
	virtual bool MoveTo(CPointMap pt, bool bForceFix = false);

	virtual void OnHear( LPCTSTR pszCmd, CChar * pSrc );
	virtual void  r_Write( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	virtual bool  r_LoadVal( CScript & s  );
	virtual void DupeCopy( const CItem * pItem );
};

enum STONEPRIV_TYPE // Priv level for this char
{
	STONEPRIV_CANDIDATE = 0,
	STONEPRIV_MEMBER,
	STONEPRIV_MASTER,
	STONEPRIV_UNUSED,
	STONEPRIV_ACCEPTED,		// The candidate has been accepted. But they have not dclicked on the stone yet.
	STONEPRIV_ENEMY = 100,	// This is an enemy town/guild.
	STONEPRIV_ALLY			// This is an ally town/guild.
};

class CStoneMember : public CGObListRec, public CScriptObj	// Members for various stones, and links to stones at war with
{
	// NOTE: Chars are linked to the CItemStone via a memory object.
	friend class CItemStone;
private:
	STONEPRIV_TYPE m_iPriv;	// What is my status level in the guild ?
	CGrayUID m_uidLinkTo;			// My char uid or enemy stone UID

	// Only apply to members.
	CGrayUID m_uidLoyalTo;	// Who am i loyal to? invalid value = myself.
	CGString m_sTitle;		// What is my title in the guild?

	union	// Depends on m_iPriv
	{
		struct	// Unknown type.
		{
			int m_Val1;
			int m_Val2;
			int m_Val3;
		} m_UnDef;

		struct // STONEPRIV_ENEMY
		{
			int m_fTheyDeclared;
			int m_fWeDeclared;
		} m_Enemy;

		struct // STONEPRIV_ALLY
		{
			int m_fTheyDeclared;
			int m_fWeDeclared;
		} m_Ally;

		struct	// a char member (not STONEPRIV_ENEMY / STONEPRIV_ALLY)
		{
			int m_fAbbrev;			// Do they have their guild abbrev on or not ?
			int m_iVoteTally;		// Temporary space to calculate votes for me.
			int m_iAccountGold;		// how much i still owe to the guild or have surplus (Normally negative).
		} m_Member;
	};

public:
	static const char *m_sClassName;
	CStoneMember( CItemStone * pStone, CGrayUID uid, STONEPRIV_TYPE iType, LPCTSTR pszTitle = "", CGrayUID loyaluidLink = 0, bool fArg1 = false, bool fArg2 = false, int nAccountGold = 0);
	virtual ~CStoneMember();

private:
	CStoneMember(const CStoneMember& copy);
	CStoneMember& operator=(const CStoneMember& other);

public:
	CStoneMember* GetNext() const;
	CItemStone * GetParentStone() const;

	CGrayUID GetLinkUID() const;

	STONEPRIV_TYPE GetPriv() const;
	void SetPriv(STONEPRIV_TYPE iPriv);
	bool IsPrivMaster() const;
	bool IsPrivMember() const;
	LPCTSTR GetPrivName() const;

	// If the member is really a war flag (STONEPRIV_ENEMY)
	void SetWeDeclaredWar(bool f);
	bool GetWeDeclaredWar() const;
	void SetTheyDeclaredWar(bool f);
	bool GetTheyDeclaredWar() const;

	// If the member is really a ally flag (STONEPRIV_ALLY)
	void SetWeDeclaredAlly(bool f);
	bool GetWeDeclaredAlly() const;
	void SetTheyDeclaredAlly(bool f);
	bool GetTheyDeclaredAlly() const;

	// Member
	bool IsAbbrevOn() const;
	void ToggleAbbrev();
	void SetAbbrev(bool mode);

	LPCTSTR GetTitle() const;
	void SetTitle( LPCTSTR pszTitle );
	CGrayUID GetLoyalToUID() const;
	bool SetLoyalTo( const CChar * pChar);
	int GetAccountGold() const;
	void SetAccountGold( int iGold );
	// ---------------------------------

	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];

	LPCTSTR GetName() const { return m_sClassName; }
	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script
	virtual bool r_LoadVal( CScript & s );
};

enum STONEDISP_TYPE	// Hard coded Menus
{
	STONEDISP_NONE = 0,
	STONEDISP_ROSTER,
	STONEDISP_CANDIDATES,
	STONEDISP_FEALTY,
	STONEDISP_ACCEPTCANDIDATE,
	STONEDISP_REFUSECANDIDATE,
	STONEDISP_DISMISSMEMBER,
	STONEDISP_VIEWCHARTER,
	STONEDISP_SETCHARTER,
	STONEDISP_VIEWENEMYS,
	STONEDISP_VIEWTHREATS,
	STONEDISP_DECLAREWAR,
	STONEDISP_DECLAREPEACE,
	STONEDISP_GRANTTITLE,
	STONEDISP_VIEWBANISHED,
	STONEDISP_BANISHMEMBER
};

class CItemStone : public CItem, public CGObList
{
	// IT_STONE_GUILD
	// IT_STONE_TOWN
	// ATTR_OWNED = auto promote to member.

	friend class CStoneMember;
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szLoadKeysM[];
	static LPCTSTR const sm_szLoadKeysG[];
private:
	CGString m_sCharter[6];
	CGString m_sWebPageURL;
	CGString m_sAbbrev;

private:

	void SetTownName();
	bool SetName( LPCTSTR pszName );
	virtual bool MoveTo(CPointMap pt, bool bForceFix = false);

	MEMORY_TYPE GetMemoryType() const;

	LPCTSTR GetCharter(unsigned int iLine) const;
	void SetCharter( unsigned int iLine, LPCTSTR pszCharter );
	LPCTSTR GetWebPageURL() const;
	void SetWebPageURL( LPCTSTR pszWebPage );
	void ElectMaster();
public:
	static const char *m_sClassName;
	CStoneMember * AddRecruit(const CChar * pChar, STONEPRIV_TYPE iPriv, bool bFull = false);

	// War
private:
	void TheyDeclarePeace( CItemStone* pEnemyStone, bool fForcePeace );
	bool WeDeclareWar(CItemStone * pEnemyStone);
	void WeDeclarePeace(CGrayUID uidEnemy, bool fForcePeace = false);
	void AnnounceWar( const CItemStone * pEnemyStone, bool fWeDeclare, bool fWar );
public:
	bool IsAtWarWith( const CItemStone * pStone ) const;
	bool IsAlliedWith( const CItemStone * pStone ) const;

	bool CheckValidMember(CStoneMember * pMember);
	int FixWeirdness();

public:
	CItemStone( ITEMID_TYPE id, CItemBase * pItemDef );
	virtual ~CItemStone();

private:
	CItemStone(const CItemStone& copy);
	CItemStone& operator=(const CItemStone& other);

public:
	virtual void r_Write( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & s, CTextConsole * pSrc );
	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual bool r_LoadVal( CScript & s );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script

	LPCTSTR GetTypeName() const;
	static bool IsUniqueName( LPCTSTR pszName );
	CChar * GetMaster() const;
	bool NoMembers() const;
	CStoneMember * GetMasterMember() const;
	CStoneMember * GetMember( const CObjBase * pObj) const;
	bool IsPrivMember( const CChar * pChar ) const;

	// Simple accessors.
	STONEALIGN_TYPE GetAlignType() const;
	void SetAlignType(STONEALIGN_TYPE iAlign);
	LPCTSTR GetAlignName() const;
	LPCTSTR GetAbbrev() const;
	void SetAbbrev( LPCTSTR pszAbbrev );
};

enum CIC_TYPE
{
	CIC_AUTHOR,
	CIC_BODY,
	CIC_PAGES,
	CIC_TITLE,
	CIC_QTY
};

class CItemMessage : public CItemVendable	// A message for a bboard or book text.
{
	// IT_BOOK, IT_MESSAGE = can be written into.
	// the name is the title for the message. (ITEMID_BBOARD_MSG)
protected:
	static LPCTSTR const sm_szVerbKeys[];
private:
	CGObArray<CGString*> m_sBodyLines;	// The main body of the text for bboard message or book.
public:
	static const char *m_sClassName;
	CGString m_sAuthor;					// Should just have author name !
	static LPCTSTR const sm_szLoadKeys[CIC_QTY+1];

public:
	CItemMessage( ITEMID_TYPE id, CItemBase * pItemDef ) : CItemVendable( id, pItemDef )
	{
	}
	virtual ~CItemMessage()
	{
		DeletePrepare();	// Must remove early because virtuals will fail in child destructor.
		UnLoadSystemPages();
	}

private:
	CItemMessage(const CItemMessage& copy);
	CItemMessage& operator=(const CItemMessage& other);

public:
	virtual void r_Write( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );
	virtual bool r_LoadVal( CScript & s );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script

	WORD GetPageCount() const
	{
		return static_cast<WORD>(m_sBodyLines.GetCount());
	}
	LPCTSTR GetPageText(WORD wPage) const
	{
		if ( !m_sBodyLines.IsValidIndex(wPage) || !m_sBodyLines[wPage] )
			return NULL;
		return m_sBodyLines[wPage]->GetPtr();
	}
	void SetPageText(WORD wPage, LPCTSTR pszText)
	{
		if ( !pszText )
			return;
		m_sBodyLines.SetAtGrow(wPage, new CGString(pszText));
	}
	void AddPageText(LPCTSTR pszText)
	{
		m_sBodyLines.Add(new CGString(pszText));
	}

	virtual void DupeCopy( const CItem * pItem );
	void UnLoadSystemPages()
	{
		m_sAuthor.Empty();
		m_sBodyLines.RemoveAll();
	}
};

#endif
