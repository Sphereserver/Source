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
	MEMORY_ISPAWNED		= 0x0200,	// I am spawned from this item. (never time out)
	MEMORY_GUILD		= 0x0400,	// This is my guild stone. (never time out) only have 1
	MEMORY_TOWN			= 0x0800,	// This is my town stone. (never time out) only have 1
	MEMORY_UNUSED		= 0x1000,	// UNUSED!!!! I am following this Object (never time out)
	MEMORY_WAR_TARG		= 0x2000,	// This is one of my current war targets.
	MEMORY_FRIEND		= 0x4000,	// They can command me but not release me. (not primary blame)
	MEMORY_UNUSED2		= 0x8000	// UNUSED!!!! Gump record memory (More1 = Context, More2 = Uid)
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

class PacketSend;
class PacketHouseDesign;
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

protected:
	CResourceRef m_BaseRef;	// Pointer to the resource that describes this type.
public:
	inline bool CallPersonalTrigger(TCHAR * pArgs, CTextConsole * pSrc, TRIGRET_TYPE & trResult, bool bFull);
	static const char *m_sClassName;
	CVarDefMap m_TagDefs;		// attach extra tags here.
	CVarDefMap m_BaseDefs;		// New Variable storage system
	DWORD	m_Can;
	
	WORD	m_attackBase;	// dam for weapons
	WORD	m_attackRange;	// variable range of attack damage.

	WORD	m_defenseBase;	// Armor for IsArmor items
	WORD	m_defenseRange;	// variable range of defense.

	CResourceRefArray m_OEvents;
	static size_t sm_iCount;	// how many total objects in the world ?
	CVarDefMap * GetTagDefs()
	{
		return( &m_TagDefs );
	}

	virtual void DeletePrepare();

public:
	BYTE	RangeL() const
	{
		CVarDefCont * pRange = GetDefKey("RANGE", true);
		return static_cast<BYTE>((pRange ? pRange->GetValNum() : 0) & 0xff);
	}

	BYTE	RangeH() const
	{
		CVarDefCont * pRange = GetDefKey("RANGE", true);
		return static_cast<BYTE>(((pRange ? pRange->GetValNum() : 0)>>8) & 0xff);
	}

	CServTime GetTimeStamp() const
	{
		return m_timestamp;
	}
	void SetTimeStamp( INT64 t_time)
	{
		m_timestamp.InitTime(t_time);
	}

	LPCTSTR GetDefStr( LPCTSTR pszKey, bool fZero = false, bool fDef = false ) const
	{
		CVarDefCont	* pVar = GetDefKey( pszKey, fDef );
		if ( pVar == NULL )
			return (fZero ? "0" : "");
		return pVar->GetValStr();
	}

	INT64 GetDefNum( LPCTSTR pszKey, bool fZero = false, bool fDef = false ) const
	{
		CVarDefCont	* pVar = GetDefKey( pszKey, fDef );
		if ( pVar == NULL )
			return (fZero ? 0 : NULL);
		return pVar->GetValNum();
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
			CItemBase * pItemDef = STATIC_CAST <CItemBase*>( Base_GetDef());
			ASSERT(pItemDef);
			return pItemDef-> m_BaseDefs.GetKey( pszKey );
		}
		else
		{
			CCharBase * pCharDef = STATIC_CAST <CCharBase*>( Base_GetDef());
			ASSERT(pCharDef);
			return pCharDef-> m_BaseDefs.GetKey( pszKey );
		}
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
	virtual int GetWeight(WORD amount = 0) const = 0;
	virtual bool IsResourceMatch( RESOURCE_ID_BASE rid, DWORD dwArg ) = 0;

	virtual int IsWeird() const;
	virtual void Delete(bool bforce = false);

	// Accessors

	virtual WORD GetBaseID() const = 0;
	CBaseBaseDef * Base_GetDef() const
	{
		return( STATIC_CAST <CBaseBaseDef *>( m_BaseRef.GetRef() ));
	}

	void SetUID( DWORD dwVal, bool fItem );
	CObjBase* GetNext() const
	{
		return( STATIC_CAST <CObjBase*>( CGObListRec::GetNext()));
	}
	CObjBase* GetPrev() const
	{
		return( STATIC_CAST <CObjBase*>( CGObListRec::GetPrev()));
	}
	virtual LPCTSTR GetName() const	// resolve ambiguity w/CScriptObj
	{
		return( CObjBaseTemplate::GetName());
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
		return( m_wHue );
	}

protected:
	WORD GetHueAlt() const
	{
		// IT_EQ_MEMORY_OBJ = MEMORY_TYPE mask
		// IT_EQ_VENDOR_BOX = restock time.
		return( m_wHue );
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
		return( m_timeout.IsTimeValid());
	}
	INT64 GetTimerDiff() const;	// return: < 0 = in the past ( m_timeout - CServTime::GetCurrentTime() )
	bool IsTimerExpired() const
	{
		return( GetTimerDiff() <= 0 );
	}

	INT64 GetTimerAdjusted() const
	{
		// RETURN: time in seconds from now.
		if ( ! IsTimerSet())
			return( -1 );
		INT64 iDiffInTicks = GetTimerDiff();
		if ( iDiffInTicks < 0 )
			return( 0 );
		return( iDiffInTicks / TICK_PER_SEC );
	}

	INT64 GetTimerDAdjusted() const
	{
		// RETURN: time in seconds from now.
		if ( ! IsTimerSet())
			return( -1 );
		INT64 iDiffInTicks = GetTimerDiff();
		if ( iDiffInTicks < 0 )
			return( 0 );
		return( iDiffInTicks );
	}

public:
	// Location
	virtual bool MoveTo(CPointMap pt, bool bForceFix = false) = 0;	// Move to a location at top level.

	virtual bool MoveNear( CPointMap pt, int iSteps = 0, DWORD dwCan = CAN_C_WALK );
	virtual bool MoveNearObj( const CObjBaseTemplate * pObj, int iSteps = 0, DWORD dwCan = CAN_C_WALK );

	void inline SetNamePool_Fail( TCHAR * ppTitles );
	bool SetNamePool( LPCTSTR pszName );

	void Sound( SOUND_TYPE id, int iRepeat = 1 ) const; // Play sound effect from this location.
	void Effect( EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBase * pSource = NULL, BYTE bspeedseconds = 5, BYTE bloop = 1, bool fexplode = false, DWORD color = 0, DWORD render = 0 ) const;

	void r_WriteSafe( CScript & s );

	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual void r_Write( CScript & s );
	virtual bool r_LoadVal( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc );	// some command on this object as a target

	void Emote( LPCTSTR pText, CClient * pClientExclude = NULL, bool fPossessive = false );

	virtual void Speak( LPCTSTR pText, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL );
	virtual void SpeakUTF8( LPCTSTR pText, HUE_TYPE wHue= HUE_TEXT_DEF, TALKMODE_TYPE mode= TALKMODE_SAY, FONT_TYPE font= FONT_NORMAL, CLanguageID lang = 0 );
	virtual void SpeakUTF8Ex( const NWORD * pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang );
	
	void RemoveFromView( CClient * pClientExclude = NULL , bool fHardcoded = true );	// remove this item from all clients.
	void ResendOnEquip( bool fAllClients = false );	// Fix for Enhanced Client when equipping items via DClick, these must be removed from where they are and sent again.
	void ResendTooltip( bool bSendFull = false, bool bUseCache = false );	// force reload of tooltip for this object
	void UpdateCanSee( PacketSend * pPacket, CClient * pClientExclude = NULL ) const;
	void UpdateObjMessage( LPCTSTR pTextThem, LPCTSTR pTextYou, CClient * pClientExclude, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, bool bUnicode = false ) const;

	TRIGRET_TYPE OnHearTrigger(CResourceLock &s, LPCTSTR pCmd, CChar *pSrc, TALKMODE_TYPE &mode, HUE_TYPE wHue = HUE_DEFAULT);

	bool IsContainer() const;

	virtual void Update(const CClient * pClientExclude = NULL)	// send this new item to clients.
		= 0;
	virtual void Flip()
		= 0;
	virtual int OnTakeDamage( int iDmg, CChar * pSrc, DAMAGE_TYPE uType = DAMAGE_HIT_BLUNT )
		= 0;
	virtual bool OnSpellEffect( SPELL_TYPE spell, CChar * pCharSrc, int iSkillLevel, CItem * pSourceItem )
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
	unsigned char m_fStatusUpdate;	// update flags for next tick
	virtual void OnTickStatusUpdate();

protected:
	PacketPropertyList* m_PropertyList;	// currently cached property list packet
	DWORD m_PropertyHash;				// latest property list hash
	DWORD m_PropertyRevision;			// current property list revision

public:
	PacketPropertyList* GetPropertyList(void) const { return m_PropertyList; }
	void SetPropertyList(PacketPropertyList* propertyList);
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
	ITRIG_AfterClick=1,
	ITRIG_Buy,
	ITRIG_Click,
	ITRIG_CLIENTTOOLTIP, // Sending tooltip to client for this item
	ITRIG_ContextMenuRequest,
	ITRIG_ContextMenuSelect,
	ITRIG_Create,		// Item is being created.
	ITRIG_DAMAGE,		// I have been damaged in some way
#ifdef _ALPHASPHERE	
	ITRIG_DAMAGEGIVEN,	// I gave some kind of damage to someone
#endif
	ITRIG_DCLICK,		// I have been dclicked.
	ITRIG_DESTROY,		//+I am nearly destroyed
	ITRIG_DROPON_CHAR,		// I have been dropped on this char
	ITRIG_DROPON_GROUND,		// I have been dropped on the ground here
	ITRIG_DROPON_ITEM,		// An item has been 
	ITRIG_DROPON_SELF,		// An item has been dropped upon me
	ITRIG_DROPON_TRADE,		// Droping an item in a trade window
	//ITRIG_DYE,
	ITRIG_EQUIP,		// I have been equipped.
	ITRIG_EQUIPTEST,
	ITRIG_MemoryEquip,
	ITRIG_PICKUP_GROUND,
	ITRIG_PICKUP_PACK,	// picked up from inside some container.
	ITRIG_PICKUP_SELF,	// picked up from this container
	ITRIG_PICKUP_STACK,	// picked up from a stack (ARGO)
	ITRIG_Sell,
	ITRIG_SPELLEFFECT,		// cast some spell on me.
	ITRIG_STEP,			// I have been walked on. (or shoved)
	ITRIG_TARGON_CANCEL,
	ITRIG_TARGON_CHAR,
	ITRIG_TARGON_GROUND,
	ITRIG_TARGON_ITEM,	// I am being combined with an item
	ITRIG_TIMER,		// My timer has expired.
	ITRIG_ToolTip,
	ITRIG_UNEQUIP,
	ITRIG_QTY
};

enum ITC_TYPE	// Item Template commands
{
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
	unsigned char m_containedGridIndex;	// Which grid have i been placed in ? (when in a container)
	DWORD	m_CanUse;		// Base attribute flags. can_u_all/male/female..
	WORD	m_weight;

public:
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


	// Type specific info. IT_TYPE
	union // 4(more1) + 4(more2) + 5(morep) = 13 bytes
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
			CPointBase m_pntOpen;	// morep=point we are standing on when opened bank box.
		} m_itEqBankBox;

		// IT_EQ_VENDOR_BOX
		struct
		{
			DWORD m_junk1;
			DWORD m_junk2;
			CPointBase m_pntOpen;	// morep=point we are standing on when opened vendor box.
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
			short int m_dmgcold;
			short int m_dmgenergy;
			short int m_dmgfire;
			short int m_dmgpoison;
		} m_itWeapon;

		// IT_ARMOR
		// IT_ARMOR_LEATHER
		// IT_SHIELD:
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
			int  m_spellcharges; // more2=not sure how used here..
			WORD m_spell;		// morex=SPELL_TYPE = The magic spell cast on this. (daemons breath)(boots of strength) etc
			WORD m_spelllevel;	// morey=0-1000=level of the spell.
			BYTE m_pattern;		// morez = light pattern - CAN_I_LIGHT LIGHT_QTY
		} m_itSpell;

		// IT_SPELLBOOK
		struct	// Spellbook extra spells.
		{
			DWORD m_spells1;	// more1=Mask of avail spells for spell book.
			DWORD m_spells2;	// more2=Mask of avail spells for spell book.
			DWORD m_spells3;	// morex,morey=necro type spells.
			WORD  m_baseid;		// morez
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
			CREID_TYPE m_MeatType;		// more2= Meat from what type of creature ?
			WORD m_spell;				// morex=SPELL_TYPE = The magic spell cast on this. ( effect of eating.)
			WORD m_spelllevel;			// morey=level of the spell. (0-1000)
			BYTE m_poison_skill;		// morez=0-100 = Is poisoned ?
			BYTE m_foodval;	
		} m_itFood;

		// IT_DRINK
		struct
		{
			ITEMID_TYPE m_cook_id;		// more1=Cooks into this. (only if raw)
			CREID_TYPE m_MeatType;		// more2= Meat from what type of creature ?
			WORD m_spell;				// morex=SPELL_TYPE = The magic spell cast on this. ( effect of eating.)
			WORD m_spelllevel;			// morey=level of the spell. (0-1000)
			BYTE m_poison_skill;		// morez=0-100 = Is poisoned ?
			BYTE m_foodval;	
		} m_itDrink;

		// IT_CORPSE
		struct	// might just be a sleeping person as well
		{
			DWORD		m_junk1;		// more1=Death time of corpse object. 0=sleeping or carved (Now Placed inside TIMESTAMP for INT64 support)
			CGrayUIDBase m_uidKiller;	// more2=Who killed this corpse, carved or looted it last. sleep=self.
			CREID_TYPE	m_BaseID;		// morex,morey=The true type of the creature who's corpse this is.
			BYTE		m_facing_dir;	// morez=corpse dir. 0x80 = on face. DIR_TYPE
			// m_amount = the body type.
			// m_uidLink = the creatures ghost.
		} m_itCorpse;

		// IT_LIGHT_LIT
		// IT_LIGHT_OUT
		// IT_WINDOW
		struct
		{
			// CAN_I_LIGHT may be set for others as well..ie.Moon gate conflict
			DWORD   m_junk1;
			DWORD	m_junk2;
			WORD	m_junk3;
			WORD	m_charges;	// morey = how long will the torch last ?
			BYTE	m_pattern;	// morez = light rotation pattern (LIGHT_PATTERN)
		} m_itLight;

		// IT_EQ_TRADE_WINDOW
		struct
		{
			DWORD	m_junk1;
			DWORD	m_junk2;
			DWORD	m_junk3;
			BYTE	m_fCheck;	// morez=Check box for trade window.
		} m_itEqTradeWindow;

		// IT_SPAWN_ITEM
		struct
		{
			RESOURCE_ID_BASE m_ItemID;	// more1=The ITEMID_* or template for items)
			DWORD	m_pile;			// more2=The max # of items to spawn per interval.  If this is 0, spawn up to the total amount
			WORD	m_TimeLoMin;	// morex=Lo time in minutes.
			WORD	m_TimeHiMin;	// morey=
			BYTE	m_DistMax;		// morez=How far from this will it spawn?
		} m_itSpawnItem;

		// IT_SPAWN_CHAR
		struct
		{
			RESOURCE_ID_BASE m_CharID;	// more1=CREID_*,  or (SPAWNTYPE_*,
			DWORD	m_current;		// more2=The current spawned from here. m_amount = the max count.
			WORD	m_TimeLoMin;	// morex=Lo time in minutes.
			WORD	m_TimeHiMin;	// morey=
			BYTE	m_DistMax;		// morez=How far from this will they wander ?
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
			//CServTime   	 m_Time;	// more2= Time date stamp for the book/message creation. (Now Placed inside TIMESTAMP for INT64 support)
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
			int m_Strength;			// more1 = How many uses til a rune will wear out ?
			DWORD m_junk2;
			CPointBase m_pntMark;	// morep = rune marked to a location or a teleport ?
		} m_itRune;

		// IT_TELEPAD
		// IT_MOONGATE
		struct
		{
			int m_fPlayerOnly;		// more1 = The gate is player only. (no npcs, xcept pets)
			int m_fQuiet;			// more2 = The gate/telepad makes no noise.
			CPointBase m_pntMark;	// morep = marked to a location or a teleport ?
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
			int m_ClothQty;			// more2 = IS the loom loaded with cloth ?
		} m_itLoom;

		// IT_ARCHERY_BUTTE
		struct
		{
			ITEMID_TYPE m_AmmoType;	// more1 = arrow or bolt currently stuck in it.
			int m_AmmoCount;		// more2 = how many arrows or bolts ?
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

		// IT_ITEM_STONE
		struct
		{
			ITEMID_TYPE m_ItemID;	// more1= generate this item or template.
			int m_iPrice;			// more2= ??? gold to purchase / sellback. (vending machine)
			WORD m_wRegenTime;		// morex=regen time in seconds. 0 = no regen required.
			WORD m_wAmount;			// morey=Total amount to deliver. 0 = infinite, 0xFFFF=none left
		} m_itItemStone;

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

		// IT_DREAM_GATE
		struct
		{
			int m_index;	// more1 = how much damage the web can take.
		} m_itDreamGate;

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
			int		m_Repeat;	// more2 =
		} m_itSound;

		// IT_STONE_GUILD
		// IT_STONE_TOWN
		struct
		{
			STONEALIGN_TYPE m_iAlign;	// more1=Neutral, chaos, order.
			int m_iAccountGold;			// more2=How much gold has been dropped on me?
			// ATTR_OWNED = auto promote to member.
		} m_itStone;

		// IT_LEATHER
		// IT_FEATHER
		// IT_FUR
		// IT_WOOL
		// IT_BLOOD
		// IT_BONE
	 	struct
		{
			int m_junk1;
			CREID_TYPE m_creid;	// more2=the source creature id. (CREID_TYPE)
		} m_itSkin;

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

	CItemBase * Item_GetDef() const
	{
		return( STATIC_CAST <CItemBase*>( Base_GetDef()));
	}

	ITEMID_TYPE GetID() const
	{
		const CItemBase * pItemDef = Item_GetDef();
		ASSERT(pItemDef);
		return( pItemDef->GetID());
	}
	WORD GetBaseID() const
	{
		// future: strongly typed enums will remove the need for this cast
		return( static_cast<WORD>(GetID()));
	}
	bool SetBaseID( ITEMID_TYPE id );
	bool SetID( ITEMID_TYPE id );
	ITEMID_TYPE GetDispID() const
	{
		// This is what the item looks like.
		// May not be the same as the item that defines it's type.
		return( m_dwDispIndex );
	}
	bool IsSameDispID( ITEMID_TYPE id ) const	// account for flipped types ?
	{
		const CItemBase * pItemDef = Item_GetDef();
		ASSERT(pItemDef);
		return( pItemDef->IsSameDispID( id ));
	}
	bool SetDispID( ITEMID_TYPE id );
	void SetAnim( ITEMID_TYPE id, int iTime );

	int IsWeird() const;
	signed char GetFixZ(CPointMap pt, unsigned long wBlockFlags = 0 );
	
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
		return(( m_Attr & dwAttr ) ? true : false );
	}
	void SetCanUse( DWORD dwCanUse )
	{
		m_CanUse |= dwCanUse;
	}
	void ClrCanUse( DWORD dwCanUse )
	{
		m_CanUse &= ~dwCanUse;
	}
	bool IsCanUse( DWORD dwCanUse ) const	// CanUse_None
	{
		return(( m_CanUse & dwCanUse ) ? true : false );
	}

	height_t GetHeight() const;
	INT64  GetDecayTime() const;
	void SetDecayTime( INT64 iTime = 0 );
	SOUND_TYPE GetDropSound( const CObjBase * pObjOn ) const;
	bool IsTopLevelMultiLocked() const;
	bool IsMovableType() const;
	bool IsMovable() const;
	int GetVisualRange() const	// virtual
	{
		if ( GetDispID() >= ITEMID_MULTI ) // ( IsTypeMulti() ) why not this?
			return( UO_MAP_VIEW_RADAR );
		return( UO_MAP_VIEW_SIZE );
	}

	bool  IsStackableException() const;
	bool  IsStackable( const CItem * pItem ) const;
	virtual bool  IsSameType( const CObjBase * pObj ) const;
	bool  Stack( CItem * pItem );
	int ConsumeAmount( int iQty = 1, bool fTest = false );

	CREID_TYPE GetCorpseType() const
	{
		return static_cast<CREID_TYPE>(GetAmount());	// What does the corpse look like ?
	}
	void  SetCorpseType( CREID_TYPE id )
	{
		// future: strongly typed enums will remove the need for this cast
		m_amount = static_cast<WORD>(id);	// m_corpse_DispID
	}
	void SetAmount( unsigned int amount );
	void SetAmountUpdate( unsigned int amount );
	WORD GetAmount() const { return( m_amount ); }

	LPCTSTR GetName() const;	// allowed to be default name.
	LPCTSTR GetNameFull( bool fIdentified ) const;

	virtual bool SetName( LPCTSTR pszName );

	virtual int GetWeight(WORD amount = 0) const
	{
		const CItemBase * pItemDef = Item_GetDef();
		ASSERT(pItemDef);
		
		int iWeight = m_weight * (amount ? amount : GetAmount());
		CVarDefCont * pReduction = GetDefKey("WEIGHTREDUCTION", true);
		if (pReduction)
		{
			iWeight -= static_cast<int>(IMULDIV( iWeight, pReduction->GetValNum(), 100 ));
			if ( iWeight < 0)
				iWeight = 0;
		}
		return( iWeight );
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
	virtual bool MoveNearObj( const CObjBaseTemplate * pItem, int iSteps = 0, WORD wCan = CAN_C_WALK );

	CItem* GetNext() const
	{
		return( STATIC_CAST <CItem*>( CObjBase::GetNext()));
	}
	CItem* GetPrev() const
	{
		return( STATIC_CAST <CItem*>( CObjBase::GetPrev()));
	}
	CObjBase * GetContainer() const
	{
		// What is this CItem contained in ?
		// Container should be a CChar or CItemContainer
		return( dynamic_cast <CObjBase*> (GetParent()));
	}
	CObjBaseTemplate * GetTopLevelObj() const
	{
		// recursively get the item that is at "top" level.
		const CObjBase* pObj = GetContainer();
		if ( !pObj )
			return const_cast <CItem*>(this);
		else if ( pObj == this )		// to avoid script errors setting same CONT
			return const_cast <CItem*>(this);
		return pObj->GetTopLevelObj();
	}

	unsigned char GetContainedGridIndex() const
	{
		return m_containedGridIndex;
	}

	void SetContainedGridIndex(unsigned char index)
	{
		m_containedGridIndex = index;
	}

	LPCTSTR GetKeyStr( LPCTSTR pszKey ) const
	{
		return m_TagDefs.GetKeyStr( pszKey );
	}

	INT64 GetKeyNum( LPCTSTR pszKey, bool fZero = false ) const
	{
		return m_TagDefs.GetKeyNum( pszKey, fZero );
	}

	CVarDefCont * GetKey( LPCTSTR pszKey, bool fDef ) const
	{
		CVarDefCont	* pVar	= m_TagDefs.GetKey( pszKey );
		if ( !fDef || pVar )	return pVar;
		CItemBase * pCharDef = Item_GetDef();
		ASSERT(pCharDef);
		return pCharDef->m_TagDefs.GetKey( pszKey );
	}

	void SetKeyNum(LPCTSTR pszKey, int iVal)
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
		return( OnTrigger( MAKEINTRESOURCE(trigger), pSrc, pArgs ));
	}

	// Item type specific stuff.
	bool IsType( IT_TYPE type ) const
	{
		return( m_type == type );
	}
	IT_TYPE GetType() const
	{
		return( m_type );
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
				return( true );
			default:
				return( false );
		}
	}
	bool IsTypeBook() const
	{
		switch( m_type )
		{
			case IT_BOOK:
			case IT_MESSAGE:
				return( true );
			default:
				return( false );
		}
	}
	bool IsTypeSpellbook() const
	{
		return( CItemBase::IsTypeSpellbook(m_type));
	}
	bool IsTypeArmor() const
	{
		return( CItemBase::IsTypeArmor(m_type));
	}
	bool IsTypeWeapon() const
	{
		return( CItemBase::IsTypeWeapon(m_type));
	}
	bool IsTypeMulti() const
	{
		return( CItemBase::IsTypeMulti(m_type));
	}
	bool IsTypeArmorWeapon() const
	{
		// Armor or weapon.
		return( IsTypeArmor() || IsTypeWeapon());
	}
	bool IsTypeLocked() const
	{
		switch ( m_type )
		{
			case IT_SHIP_SIDE_LOCKED:
			case IT_CONTAINER_LOCKED:
			case IT_SHIP_HOLD_LOCK:
			case IT_DOOR_LOCKED:
				return( true );
			default:
				return( false );
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
				return( true );
			default:
				return( IsTypeLocked() );
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
				return( true );
			default:
				return( IsTypeArmorWeapon() );
		}
	}

	bool IsResourceMatch( RESOURCE_ID_BASE rid, DWORD dwArg );

	bool IsValidLockLink( CItem * pItemLock ) const;
	bool IsValidLockUID() const;
	bool IsKeyLockFit( DWORD dwLockUID ) const
	{
		return( m_itKey.m_lockUID == dwLockUID );
	}

	void ConvertBolttoCloth();
	SPELL_TYPE GetScrollSpell() const;
	bool IsSpellInBook( SPELL_TYPE spell ) const;
	int GetSpellcountInBook() const;
	int  AddSpellbookScroll( CItem * pItem );
	int AddSpellbookSpell( SPELL_TYPE spell, bool fUpdate );

	bool IsDoorOpen() const;
	bool Use_Door( bool fJustOpen );
	bool Use_DoorNew( bool bJustOpen );
	bool Use_Portculis();
	SOUND_TYPE Use_Music( bool fWell ) const;

	bool SetMagicLock( CChar * pCharSrc, int iSkillLevel );
	void SetSwitchState();
	void SetTrapState( IT_TYPE state, ITEMID_TYPE id, int iTimeSec );
	int Use_Trap();
	bool Use_Light();
	int Light_GetOverride(const CItemBase * pBase = NULL) const;
	int Use_LockPick( CChar * pCharSrc, bool fTest, bool fFail );
	LPCTSTR Use_SpyGlass( CChar * pUser ) const;
	LPCTSTR Use_Sextant( CPointMap pntCoords ) const;

	bool IsBookWritable() const
	{
		return( m_itBook.m_ResID.GetPrivateUID() == 0 && GetTimeStamp().GetTimeRaw() == 0 );
	}
	bool IsBookSystem() const	// stored in RES_BOOK
	{
		return( m_itBook.m_ResID.GetResType() == RES_BOOK );
	}

	bool OnExplosion();
	bool OnSpellEffect( SPELL_TYPE spell, CChar * pCharSrc, int iSkillLevel, CItem * pSourceItem );
	int OnTakeDamage( int iDmg, CChar * pSrc, DAMAGE_TYPE uType = DAMAGE_HIT_BLUNT );

	int Armor_GetRepairPercent() const;
	LPCTSTR Armor_GetRepairDesc() const;
	bool Armor_IsRepairable() const;
	int Armor_GetDefense() const;
	int Weapon_GetAttack(bool bNoRandom = false) const;
	SKILL_TYPE Weapon_GetSkill() const;

	void Spawn_OnTick( bool fExec );
	void Spawn_KillChildren();
	CCharBase * Spawn_SetTrackID();
	void Spawn_GenerateItem( CResourceDef * pDef );
	void Spawn_GenerateChar( CResourceDef * pDef );

	inline CCharBase * Spawn_TryChar( CREID_TYPE &id );
	inline CItemBase * Spawn_TryItem( ITEMID_TYPE &id );
	CResourceDef * Spawn_FixDef();

	int Spawn_GetName( TCHAR * pszOut ) const;

	bool IsMemoryTypes( WORD wType ) const
	{
		// MEMORY_FIGHT
		if ( ! IsType( IT_EQ_MEMORY_OBJ ))
			return( false );
		return(( GetHueAlt() & wType ) ? true : false );
	}

	bool Ship_Plank( bool fOpen );

	void Plant_SetTimer();
	bool Plant_OnTick();
	void Plant_CropReset();
	bool Plant_Use( CChar * pChar );

	virtual void DupeCopy( const CItem * pItem );
	CItem * UnStackSplit( int amount, CChar * pCharSrc = NULL );

	static CItem * CreateBase( ITEMID_TYPE id );
	static CItem * CreateHeader( TCHAR * pArg, CObjBase * pCont = NULL, bool fDupeCheck = false, CChar * pSrc = NULL );
	static CItem * CreateScript( ITEMID_TYPE id, CChar * pSrc = NULL );
	static CItem * CreateDupeItem( const CItem * pItem, CChar * pSrc = NULL, bool fSetNew = false );
	static CItem * CreateTemplate( ITEMID_TYPE id, CObjBase* pCont = NULL, CChar * pSrc = NULL );

	static CItem * ReadTemplate( CResourceLock & s, CObjBase * pCont );

	int GetAbilityFlags() const;

	virtual void Delete(bool bforce = false);
	virtual bool NotifyDelete();
};

class CItemVendable : public CItem
{
	// Any item that can be sold and has value.
private:
	static LPCTSTR const sm_szLoadKeys[];
	WORD m_quality;		// 0-100 quality.
	LONG m_price;		// The price of this item if on a vendor. (allow random (but remembered) pluctuations)

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

	void SetPlayerVendorPrice( LONG dwVal );
	LONG GetBasePrice() const;
	LONG GetVendorPrice( int iConvertFactor );

	bool  IsValidSaleItem( bool fBuyFromVendor ) const;
	bool  IsValidNPCSaleItem() const;

	virtual void DupeCopy( const CItem * pItem );

	void	Restock( bool fSellToPlayers );
	virtual void  r_Write( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	virtual bool  r_LoadVal( CScript & s  );
};

class CContainer : public CGObList	// This class contains a list of items but may or may not be an item itself.
{
private:
	int	m_totalweight;	// weight of all the items it has. (1/WEIGHT_UNITS pound)
protected:
	virtual void OnRemoveOb( CGObListRec* pObRec );	// Override this = called when removed from list.
	void ContentAddPrivate( CItem * pItem );

	void r_WriteContent( CScript & s ) const;

	bool r_WriteValContainer(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	bool r_GetRefContainer( LPCTSTR & pszKey, CScriptObj * & pRef );

public:
	static const char *m_sClassName;
	CContainer()
	{
		m_totalweight = 0;
	}
	virtual ~CContainer()
	{
		DeleteAll(); // call this early so the virtuals will work.
	}

private:
	CContainer(const CContainer& copy);
	CContainer& operator=(const CContainer& other);

public:
	CItem * GetAt( size_t index ) const
	{
		return( dynamic_cast <CItem*>( CGObList::GetAt( index )));
	}
	int	GetTotalWeight() const
	{
		return( m_totalweight );
	}
	CItem* GetContentHead() const
	{
		return( STATIC_CAST <CItem*>( GetHead()));
	}
	CItem* GetContentTail() const
	{
		return( STATIC_CAST <CItem*>( GetTail()));
	}
	int FixWeight();

	bool ContentFindKeyFor( CItem * pLocked ) const;
	// bool IsItemInside( CItem * pItem ) const;

	CItem * ContentFindRandom() const;
	void ContentsDump( const CPointMap & pt, DWORD dwAttr = 0 );
	void ContentsTransfer( CItemContainer * pCont, bool fNoNewbie );
	void ContentAttrMod( DWORD dwAttr, bool fSet );
	void ContentNotifyDelete();

	// For resource usage and gold.
	CItem * ContentFind( RESOURCE_ID_BASE rid, DWORD dwArg = 0, int iDecendLevels = 255 ) const;
	TRIGRET_TYPE OnContTriggerForLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * pResult, CScriptLineContext & StartContext, CScriptLineContext & EndContext, RESOURCE_ID_BASE rid, DWORD dwArg = 0, int iDecendLevels = 255 );
	TRIGRET_TYPE OnGenericContTriggerForLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * pResult, CScriptLineContext & StartContext, CScriptLineContext & EndContext, int iDecendLevels = 255 );
	int ContentCount( RESOURCE_ID_BASE rid, DWORD dwArg = 0 );
	int ContentCountAll() const;
	int ContentConsume( RESOURCE_ID_BASE rid, int iQty = 1, bool fTest = false, DWORD dwArg = 0 );

	int ResourceConsume( const CResourceQtyArray * pResources, int iReplicationQty, bool fTest = false, DWORD dwArg = 0 );
	size_t ResourceConsumePart( const CResourceQtyArray * pResources, int iReplicationQty, int iFailPercent, bool fTest = false, DWORD dwArg = 0 );

	virtual void OnWeightChange( int iChange );
	virtual void ContentAdd( CItem * pItem ) = 0;
};

class CItemContainer : public CItemVendable, public CContainer
{
	// This item has other items inside it.
	static LPCTSTR const sm_szVerbKeys[];
public:
	static const char *m_sClassName;
	// bool m_fTinkerTrapped;	// magic trap is diff.
	bool NotifyDelete();
	void DeletePrepare()
	{
		if ( IsType( IT_EQ_TRADE_WINDOW ))
		{
			Trade_Delete();
		}
		CItem::DeletePrepare();
	}

public:
	CItemContainer( ITEMID_TYPE id, CItemBase * pItemDef );
	virtual ~CItemContainer()
	{
		DeleteAll();	// get rid of my contents first to protect against weight calc errors.
		DeletePrepare();
	}

private:
	CItemContainer(const CItemContainer& copy);
	CItemContainer& operator=(const CItemContainer& other);

public:
	bool IsWeighed() const
	{
		if ( IsType(IT_EQ_BANK_BOX ))
			return false;
		if ( IsType(IT_EQ_VENDOR_BOX))
			return false;
		if ( IsAttr(ATTR_MAGIC))	// magic containers have no weight.
			return false;
		return true;
	}
	bool IsSearchable() const
	{
		if ( IsType(IT_EQ_BANK_BOX) )
			return false;
		if ( IsType(IT_EQ_VENDOR_BOX) )
			return false;
		if ( IsType(IT_CONTAINER_LOCKED) )
			return false;
		if ( IsType(IT_EQ_TRADE_WINDOW) )
			return false;
		return true;
	}

	bool IsItemInside(const CItem * pItem) const;
	bool CanContainerHold(const CItem * pItem, const CChar * pCharMsg );

	virtual bool r_Verb( CScript & s, CTextConsole * pSrc );
	virtual void  r_Write( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );

	virtual int GetWeight(WORD amount = 0) const
	{	// true weight == container item + contents.
		return( CItem::GetWeight(amount) + CContainer::GetTotalWeight());
	}
	void OnWeightChange( int iChange );

	void ContentAdd( CItem * pItem );
	void ContentAdd( CItem * pItem, CPointMap pt, unsigned char gridIndex = 0 );
protected:
	void OnRemoveOb( CGObListRec* pObRec );	// Override this = called when removed from list.
public:
	bool IsItemInTrade();
	void Trade_Status( bool fCheck );
	void Trade_Delete();

	void MakeKey();
	void SetKeyRing();
	void Game_Create();
	void Restock();
	bool OnTick();

	virtual void DupeCopy( const CItem * pItem );

	CPointMap GetRandContainerLoc() const;

	void OnOpenEvent( CChar * pCharOpener, const CObjBaseTemplate * pObjTop );
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
	// IT_CORPSE
	// A corpse is a special type of item.
public:
	static const char *m_sClassName;
	CItemCorpse( ITEMID_TYPE id, CItemBase * pItemDef ) :
		CItemContainer( id, pItemDef )
	{
	}
	virtual ~CItemCorpse()
	{
		DeletePrepare();	// Must remove early because virtuals will fail in child destructor.
	}

private:
	CItemCorpse(const CItemCorpse& copy);
	CItemCorpse& operator=(const CItemCorpse& other);

public:
	CChar * IsCorpseSleeping() const;

	int GetWeight(WORD amount = 0) const
	{
		// GetAmount is messed up.
		// true weight == container item + contents.
		return( 1 + CContainer::GetTotalWeight());
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
	CRegionWorld * m_pRegion;		// we own this region.
	bool Multi_IsPartOf( const CItem * pItem ) const;
	void MultiUnRealizeRegion();
	bool MultiRealizeRegion();

	CItem * Multi_FindItemType( IT_TYPE type ) const;
	CItem * Multi_FindItemComponent( int iComp ) const;


	const CItemBaseMulti * Multi_GetDef() const
	{
		return( STATIC_CAST <const CItemBaseMulti *>( Base_GetDef()));
	}
	bool Multi_CreateComponent( ITEMID_TYPE id, int dx, int dy, int dz, DWORD dwKeyCode );

public:
	int Multi_GetMaxDist() const;
	size_t  Multi_ListObjs(CObjBase ** ppObjList);
	struct ShipSpeed // speed of a ship
	{
		unsigned short period;	// time between movement
		unsigned short tiles;	// distance to move
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
	typedef std::vector<Component*> ComponentsContainer;
	struct DesignDetails
	{
		int m_iRevision;
		ComponentsContainer m_vectorComponents;
		PacketHouseDesign* m_pData;
		int m_iDataRevision;
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

	CClient * m_pArchitect;
	CRectMap m_rectDesignArea;
	CGrayMultiCustom * m_pGrayMulti;

	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual void r_Write( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	virtual bool r_LoadVal( CScript & s  );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script

	const CPointMap GetComponentPoint(Component * pComponent) const;
	const CPointMap GetComponentPoint(int dx, int dy, int dz) const;
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
	void EndCustomize(bool bForce = false);
	void SwitchToLevel( CClient * pClientSrc, int iLevel );
	void CommitChanges( CClient * pClientSrc = NULL );
	void AddItem( CClient * pClientSrc, ITEMID_TYPE id, short x, short y, signed char z = -128, int iStairID = 0);
	void AddStairs( CClient * pClientSrc, ITEMID_TYPE id, short x, short y, signed char z = -128, int iStairID = -1 );
	void AddRoof( CClient * pClientSrc, ITEMID_TYPE id, short x, short y, signed char z);
	void RemoveItem( CClient * pClientSrc, ITEMID_TYPE id, short x, short y, signed char z);
	bool RemoveStairs( Component * pStairComponent );
	void RemoveRoof( CClient * pClientSrc, ITEMID_TYPE id, short x, short y, signed char z);
	void SendVersionTo( CClient * pClientSrc );
	void SendStructureTo( CClient * pClientSrc );
	void BackupStructure( CClient * pClientSrc = NULL );
	void RestoreStructure( CClient * pClientSrc = NULL );
	void RevertChanges( CClient * pClientSrc = NULL );
	void ResetStructure( CClient * pClientSrc = NULL );

	const CGrayMultiCustom * GetMultiItemDefs();
	const CGRect GetDesignArea();
	size_t GetFixtureCount(DesignDetails * pDesign = NULL);
	size_t GetComponentsAt(short dx, short dy, signed char dz, Component ** pComponents, DesignDetails * pDesign = NULL);
	int GetRevision(const CClient * pClientSrc = NULL) const;
	int GetLevelCount();
	int GetStairCount();

	static unsigned char GetPlane( signed char z );
	static unsigned char GetPlane( Component * pComponent );
	static signed char GetPlaneZ( unsigned char plane );
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
		return( GetID() & 3 );
	}
	size_t  Ship_ListObjs( CObjBase ** ppObjList );
	bool Ship_CanMoveTo( const CPointMap & pt ) const;
	bool Ship_SetMoveDir( DIR_TYPE dir );
	bool Ship_MoveDelta( CPointBase pdelta );
	bool Ship_OnMoveTick();
	double Ship_GetMovePeriod();

	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual void r_Write( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	virtual bool r_LoadVal( CScript & s  );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script
	virtual int FixWeirdness();
	virtual void OnComponentCreate( const CItem * pComponent );

public:
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
		SetHueAlt( wType );
		return( wType );
	}
	
	WORD GetMemoryTypes() const
	{
		return( GetHueAlt());	// MEMORY_FIGHT
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
	short m_x;
	short m_y;

public:
	CMapPinRec( short x, short y )
		: m_x(x), m_y(y)
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
	STONEPRIV_ACCEPTED,	// The candidate has been accepted. But they have not dclicked on the stone yet.
	STONEPRIV_ENEMY = 100	// this is an enemy town/guild.
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

		struct	// a char member (NOT STONEPRIV_ENEMY)
		{
			int m_fAbbrev;			// Do they have their guild abbrev on or not ?
			int m_iVoteTally;		// Temporary space to calculate votes for me.
			int m_iAccountGold;		// how much i still owe to the guild or have surplus (Normally negative).
		} m_Member;
	};

public:
	static const char *m_sClassName;
	CStoneMember( CItemStone * pStone, CGrayUID uid, STONEPRIV_TYPE iType, LPCTSTR pTitle = "", CGrayUID loyaluidLink = 0, bool fArg1 = false, bool fArg2 = false, int nAccountGold = 0);
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
	void SetWeDeclared(bool f);
	bool GetWeDeclared() const;
	void SetTheyDeclared(bool f);
	bool GetTheyDeclared() const;

	// Member
	bool IsAbbrevOn() const;
	void ToggleAbbrev();
	void SetAbbrev(bool mode);

	LPCTSTR GetTitle() const;
	void SetTitle( LPCTSTR pTitle );
	CGrayUID GetLoyalToUID() const;
	bool SetLoyalTo( const CChar * pChar);
	int GetAccountGold() const;
	void SetAccountGold( int iGold );
	// ---------------------------------

	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];

	LPCTSTR GetName() const { return m_sClassName; }
	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual bool r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc );
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
	void SetCharter( unsigned int iLine, LPCTSTR pCharter );
	LPCTSTR GetWebPageURL() const;
	void SetWebPage( LPCTSTR pWebPage );

#ifndef _NEWGUILDSYSTEM
	bool IsInMenu( STONEDISP_TYPE iStoneMenu, const CItemStone * pOtherStone ) const;
	bool IsInMenu( STONEDISP_TYPE iStoneMenu, const CStoneMember * pMember ) const;

	// Client interaction.
	size_t  addStoneListSetup( STONEDISP_TYPE iStoneMenu, CGString * psText, size_t iTexts );
	void addStoneList( CClient * pClient, STONEDISP_TYPE iStoneMenu );
	void addStoneSetViewCharter( CClient * pClient, STONEDISP_TYPE iStoneMenu );
	void addStoneDialog( CClient * pClient, STONEDISP_TYPE menuid );
	
	void SetupMenu( CClient * pClient, bool fMasterFunc = false );
#endif

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
	static bool IsUniqueName( LPCTSTR pName );
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
	void SetAbbrev( LPCTSTR pAbbrev );
	
#ifndef _NEWGUILDSYSTEM
	bool OnPromptResp( CClient * pClient, CLIMODE_TYPE TargMode, LPCTSTR pszText, CGString & sMsg, CGrayUID context );
	bool OnDialogButton( CClient * pClient, STONEDISP_TYPE type, CDialogResponseArgs & resp );
	void Use_Item( CClient * pClient );
#endif
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

	size_t GetPageCount() const
	{
		return m_sBodyLines.GetCount();
	}
	LPCTSTR GetPageText( size_t iPage ) const
	{
		if ( m_sBodyLines.IsValidIndex(iPage) == false )
			return NULL;
		if ( m_sBodyLines[iPage] == NULL )
			return NULL;
		return m_sBodyLines[iPage]->GetPtr();
	}
	void SetPageText( size_t iPage, LPCTSTR pszText )
	{
		if ( pszText == NULL )
			return;
		m_sBodyLines.SetAtGrow( iPage, new CGString( pszText ));
	}
	void AddPageText( LPCTSTR pszText )
	{
		m_sBodyLines.Add( new CGString( pszText ));
	}

	virtual void DupeCopy( const CItem * pItem );
	void UnLoadSystemPages()
	{
		m_sAuthor.Empty();
		m_sBodyLines.RemoveAll();
	}
};

enum NPCBRAIN_TYPE	// General AI type.
{
	NPCBRAIN_NONE = 0,	// 0 = This should never really happen.
	NPCBRAIN_ANIMAL,	// 1 = can be tamed.
	NPCBRAIN_HUMAN,		// 2 = generic human.
	NPCBRAIN_HEALER,	// 3 = can res.
	NPCBRAIN_GUARD,		// 4 = inside cities
	NPCBRAIN_BANKER,	// 5 = can open your bank box for you
	NPCBRAIN_VENDOR,	// 6 = will sell from vendor boxes.
	NPCBRAIN_BEGGAR,	// 7 = begs.
	NPCBRAIN_STABLE,	// 8 = will store your animals for you.
	NPCBRAIN_THIEF,		// 9 = should try to steal ?
	NPCBRAIN_MONSTER,	// 10 = not tamable. normally evil.
	NPCBRAIN_BERSERK,	// 11 = attack closest (blades, vortex)
	NPCBRAIN_UNDEAD,	// 12 = disapears in the light.
	NPCBRAIN_DRAGON,	// 13 = we can breath fire. may be tamable ? hirable ?
	NPCBRAIN_VENDOR_OFFDUTY,	// 14 = "Sorry i'm not working right now. come back when my shop is open.
	NPCBRAIN_QTY
};

//	number of steps to remember for pathfinding
//	default to 24 steps, will have 24*4 extra bytes per char
#define MAX_NPC_PATH_STORAGE_SIZE	UO_MAP_VIEW_SIGHT*2

struct CCharNPC
{
	// This is basically the unique "brains" for any character.
public:
	static const char *m_sClassName;
	// Stuff that is specific to an NPC character instance (not an NPC type see CCharBase for that).
	// Any NPC AI stuff will go here.
	static LPCTSTR const sm_szVerbKeys[];

	NPCBRAIN_TYPE m_Brain;	// For NPCs: Number of the assigned basic AI block
	WORD	m_Home_Dist_Wander;	// Distance to allow to "wander".
	BYTE    m_Act_Motivation;		// 0-100 (100=very greatly) how bad do i want to do the current action.

	// We respond to what we here with this.
	CResourceRefArray m_Speech;	// Speech fragment list (other stuff we know)
	HUE_TYPE m_SpeechHue;

	CResourceQty m_Need;	// What items might i need/Desire ? (coded as resource scripts) ex "10 gold,20 logs" etc.

	static LPCTSTR const sm_szLoadKeys[];

	WORD	m_nextX[MAX_NPC_PATH_STORAGE_SIZE];	// array of X coords of the next step
	WORD	m_nextY[MAX_NPC_PATH_STORAGE_SIZE];	// array of Y coords of the next step
	CPointMap m_nextPt;							// where the array(^^) wants to go, if changed, recount the path
	CServTime	m_timeRestock;		//	when last restock happened in sell/buy container

public:
	void r_WriteChar( CChar * pChar, CScript & s );
	bool r_WriteVal( CChar * pChar, LPCTSTR pszKey, CGString & s );
	bool r_LoadVal( CChar * pChar, CScript & s );

	bool IsVendor() const
	{
		return( m_Brain == NPCBRAIN_HEALER || m_Brain == NPCBRAIN_STABLE || m_Brain == NPCBRAIN_VENDOR );
	}

public:
	CCharNPC( CChar * pChar, NPCBRAIN_TYPE NPCBrain );
	~CCharNPC();

private:
	CCharNPC(const CCharNPC& copy);
	CCharNPC& operator=(const CCharNPC& other);
};

#define IS_SKILL_BASE(sk) ((sk) > SKILL_NONE && (sk) < SKILL_MAX )

struct CCharPlayer
{
	// Stuff that is specific to a player character.
private:
	BYTE m_SkillLock[SKILL_QTY];	// SKILLLOCK_TYPE List of skill lock states for this player character
	BYTE m_StatLock[STAT_BASE_QTY]; // SKILLLOCK_TYPE Applied to stats
	CResourceRef m_SkillClass;	// RES_SKILLCLASS CSkillClassDef What skill class group have we selected.
	bool m_bKrToolbarEnabled;

public:
	static const char *m_sClassName;
	CAccount * m_pAccount;	// The account index. (for idle players mostly)
	static LPCTSTR const sm_szVerbKeys[];

	CServTime m_timeLastUsed;	// Time the player char was last used.

	CGString m_sProfile;	// limited to SCRIPT_MAX_LINE_LEN-16

	WORD m_wMurders;		// Murder count.
	WORD m_wDeaths;		// How many times have i died ?
	unsigned short int		m_speedMode;		// speed mode (0x0 = Normal movement, 0x1 = Fast movement, 0x2 = Slow movement, 0x3 and above = Hybrid movement)
	DWORD			m_pflag;			// PFLAG

	static LPCTSTR const sm_szLoadKeys[];

	CResourceRefArray m_Speech;	// Speech fragment list (other stuff we know)

public:
	SKILL_TYPE Skill_GetLockType( LPCTSTR pszKey ) const;
	SKILLLOCK_TYPE Skill_GetLock( SKILL_TYPE skill ) const;
	void Skill_SetLock( SKILL_TYPE skill, SKILLLOCK_TYPE state );

	STAT_TYPE Stat_GetLockType( LPCTSTR pszKey ) const;
	SKILLLOCK_TYPE Stat_GetLock( STAT_TYPE stat ) const;
	void Stat_SetLock( STAT_TYPE stat, SKILLLOCK_TYPE state );

	void r_WriteChar( CChar * pChar, CScript & s );
	bool r_WriteVal( CChar * pChar, LPCTSTR pszKey, CGString & s );
	bool r_LoadVal( CChar * pChar, CScript & s );

	bool SetSkillClass( CChar * pChar, RESOURCE_ID rid );
	CSkillClassDef * GetSkillClass() const;

	bool getKrToolbarStatus();

	CAccountRef GetAccount() const;

public:
	CCharPlayer( CChar * pChar, CAccount * pAccount );
	~CCharPlayer();

private:
	CCharPlayer(const CCharPlayer& copy);
	CCharPlayer& operator=(const CCharPlayer& other);
};

enum WAR_SWING_TYPE	// m_Act_War_Swing_State
{
	WAR_SWING_INVALID = -1,
	WAR_SWING_EQUIPPING = 0,	// we are recoiling our weapon.
	WAR_SWING_READY,			// we can swing at any time.
	WAR_SWING_SWINGING			// we are swinging our weapon.
};

enum CTRIG_TYPE
{
	CTRIG_AAAUNUSED		= 0,
	CTRIG_AfterClick,
	CTRIG_Attack,			// I am attacking someone (SRC)
	CTRIG_CallGuards,

	CTRIG_charAttack,		// Here starts @charXXX section
	CTRIG_charClick,
	CTRIG_charClientTooltip,
	CTRIG_charDClick,
	CTRIG_charTradeAccepted,

	CTRIG_Click,			// I got clicked on by someone.
	CTRIG_ClientTooltip,	 // Sending tooltips for me to someone
	CTRIG_CombatAdd,		// I add someone to my attacker list
	CTRIG_CombatDelete,		// delete someone from my list
	CTRIG_CombatEnd,		// I finished fighting
	CTRIG_CombatStart,		// I begin fighting
	CTRIG_ContextMenuRequest,
	CTRIG_ContextMenuSelect,
	CTRIG_Create,			// Newly created (not in the world yet)
	CTRIG_Criminal,			// Called before someone becomes 'gray' for someone
	CTRIG_DClick,			// Someone has dclicked on me.
	CTRIG_Death,			//+I just got killed.
	CTRIG_DeathCorpse,		// Corpse
	CTRIG_Destroy,			//+I am nearly destroyed
	CTRIG_Dismount,
	//CTRIG_DYE,
	CTRIG_EnvironChange,	// my environment changed somehow (light,weather,season,region)
	CTRIG_ExpChange,		// EXP is going to change
	CTRIG_ExpLevelChange,	// Experience LEVEL is going to change

	CTRIG_FameChange,				// Fame chaged

	CTRIG_GetHit,			// I just got hit.
	CTRIG_Hit,				// I just hit someone. (TARG)
	CTRIG_HitCheck,
	CTRIG_HitMiss,			// I just missed.
	CTRIG_HitTry,			// I am trying to hit someone. starting swing.,
	CTRIG_HouseDesignCommit,	// I committed a new house design
	CTRIG_HouseDesignExit,	// I exited house design mode

	CTRIG_itemAfterClick,
	CTRIG_itemBuy,
	CTRIG_itemClick,		// I clicked on an item
	CTRIG_itemClientTooltip,
	CTRIG_itemContextMenuRequest,
	CTRIG_itemContextMenuSelect,
	CTRIG_itemCreate,		//?
	CTRIG_itemDamage,		//?
#ifdef _ALPHASPHERE
	CTRIG_itemDamageGiven,
#endif
	CTRIG_itemDCLICK,		// I have dclicked item
	CTRIG_itemDestroy,		//+Item is nearly destroyed
	CTRIG_itemDROPON_CHAR,		// I have been dropped on this char
	CTRIG_itemDROPON_GROUND,	// I dropped an item on the ground
	CTRIG_itemDROPON_ITEM,		// I have been dropped on this item
	CTRIG_itemDROPON_SELF,		// I have been dropped on this item
	CTRIG_itemDROPON_TRADE,
	CTRIG_itemEQUIP,		// I have equipped an item
	CTRIG_itemEQUIPTEST,
	CTRIG_itemMemoryEquip,
	CTRIG_itemPICKUP_GROUND,
	CTRIG_itemPICKUP_PACK,	// picked up from inside some container.
	CTRIG_itemPICKUP_SELF,	// picked up from this (ACT) container.
	CTRIG_itemPICKUP_STACK,	// was picked up from a stack
	CTRIG_itemSell,
	CTRIG_itemSPELL,		// cast some spell on the item.
	CTRIG_itemSTEP,			// stepped on an item
	CTRIG_itemTARGON_CANCEL,
	CTRIG_itemTARGON_CHAR,
	CTRIG_itemTARGON_GROUND,
	CTRIG_itemTARGON_ITEM,	// I am being combined with an item
	CTRIG_itemTimer,		//?
	CTRIG_itemToolTip,		// Did tool tips on an item
	CTRIG_itemUNEQUIP,		// i have unequipped (or try to unequip) an item

	CTRIG_Jailed,			// I'm up to be send to jail, or to be forgiven

	CTRIG_KarmaChange,			// Karma chaged

	CTRIG_Kill,				//+I have just killed someone
	CTRIG_LogIn,			// Client logs in
	CTRIG_LogOut,			// Client logs out (21)
	CTRIG_Mount,
	CTRIG_MurderDecay,		// I have decayed one of my kills
	CTRIG_MurderMark,		// I am gonna to be marked as a murder
	CTRIG_NotoSend,			// sending notoriety

	CTRIG_NPCAcceptItem,		// (NPC only) i've been given an item i like (according to DESIRES)
	CTRIG_NPCActFight,
	CTRIG_NPCActFollow,		// (NPC only) decided to follow someone
	CTRIG_NPCAction,
	CTRIG_NPCHearGreeting,		// (NPC only) i have been spoken to for the first time. (no memory of previous hearing)
	CTRIG_NPCHearUnknown,		//+(NPC only) I heard something i don't understand.
	CTRIG_NPCLookAtChar,		//
	CTRIG_NPCLookAtItem,		//
	CTRIG_NPCLostTeleport,		//+(NPC only) ready to teleport back to spawn
	CTRIG_NPCRefuseItem,		// (NPC only) i've been given an item i don't want.
	CTRIG_NPCRestock,			// (NPC only) 
	CTRIG_NPCSeeNewPlayer,		//+(NPC only) i see u for the first time. (in 20 minutes) (check memory time)
	CTRIG_NPCSeeWantItem,		// (NPC only) i see something good.
	CTRIG_NPCSpecialAction,
	
	CTRIG_PartyDisband,			//I just disbanded my party
	CTRIG_PartyInvite,			//SRC invited me to join a party, so I may chose
	CTRIG_PartyLeave,
	CTRIG_PartyRemove,			//I have ben removed from the party by SRC

	CTRIG_PersonalSpace,	//+i just got stepped on.
	CTRIG_PetDesert,
	CTRIG_Profile,			// someone hit the profile button for me.
	CTRIG_ReceiveItem,		// I was just handed an item (Not yet checked if i want it)
	CTRIG_RegenStat,		// Hits/mana/stam/food regeneration

	CTRIG_RegionEnter,
	CTRIG_RegionLeave,
	CTRIG_RegionResourceFound,	// I just discovered a resource
	CTRIG_RegionResourceGather,

	CTRIG_Rename,			// Changing my name or pets one

	CTRIG_Resurrect,		// I'm going to resurrect via function or spell.

	CTRIG_SeeCrime,			// I am seeing a crime
	CTRIG_SeeHidden,		// I'm about to see a hidden char

	// SKTRIG_QTY
	CTRIG_SkillAbort,			// SKTRIG_ABORT
	CTRIG_SkillChange,
	CTRIG_SkillFail,			// SKTRIG_FAIL
	CTRIG_SkillGain,			// SKTRIG_GAIN
	CTRIG_SkillMakeItem,
	CTRIG_SkillMemu,
	CTRIG_SkillPreStart,		// SKTRIG_PRESTART
	CTRIG_SkillSelect,			// SKTRIG_SELECT
	CTRIG_SkillStart,			// SKTRIG_START
	CTRIG_SkillStroke,			// SKTRIG_STROKE
	CTRIG_SkillSuccess,			// SKTRIG_SUCCESS
	CTRIG_SkillTargetCancel,	// SKTRIG_TARGETCANCEL
	CTRIG_SkillUseQuick,		// SKTRIG_USEQUICK
	CTRIG_SkillWait,			// SKTRIG_WAIT

	CTRIG_SpellBook,
	CTRIG_SpellCast,		//+Char is casting a spell.
	CTRIG_SpellEffect,		//+A spell just hit me.
	CTRIG_SpellFail,		// The spell failed
	CTRIG_SpellSelect,		// selected a spell
	CTRIG_SpellSuccess,		// The spell succeeded
	CTRIG_SpellTargetCancel,	//  cancelled spell target
	CTRIG_StatChange,
	CTRIG_StepStealth,		//+Made a step while being in stealth 
	CTRIG_ToggleFlying,
	CTRIG_ToolTip,			// someone did tool tips on me.
	CTRIG_TradeAccepted,	// Everything went well, and we are about to exchange trade items
	CTRIG_TradeClose,		// Fired when a Trade Window is being deleted, no returns
	CTRIG_TradeCreate,		// Trade window is going to be created

	// Packet related triggers
	CTRIG_UserBugReport,
	CTRIG_UserChatButton,
	CTRIG_UserExtCmd,
	CTRIG_UserExWalkLimit,
	CTRIG_UserGuildButton,
	CTRIG_UserKRToolbar,
	CTRIG_UserMailBag,
	CTRIG_UserQuestArrowClick,
	CTRIG_UserQuestButton,
	CTRIG_UserSkills,
	CTRIG_UserSpecialMove,
	CTRIG_UserStats,
	CTRIG_UserVirtue,
	CTRIG_UserVirtueInvoke,
	CTRIG_UserWarmode,

	CTRIG_QTY				// 130
};

class CPartyDef;

class CChar : public CObjBase, public CContainer, public CTextConsole
{
	// RES_WORLDCHAR
private:
	// Spell type effects.
#define STATF_INVUL			0x00000001	// Invulnerability
#define STATF_DEAD			0x00000002
#define STATF_Freeze		0x00000004	// Paralyzed. (spell)
#define STATF_Invisible		0x00000008	// Invisible (spell).
#define STATF_Sleeping		0x00000010	// You look like a corpse ?
#define STATF_War			0x00000020	// War mode on ?
#define STATF_Reactive		0x00000040	// have reactive armor on.
#define STATF_Poisoned		0x00000080	// Poison level is in the poison object
#define STATF_NightSight	0x00000100	// All is light to you
#define STATF_Reflection	0x00000200	// Magic reflect on.
#define STATF_Polymorph		0x00000400	// We have polymorphed to another form.
#define STATF_Incognito		0x00000800	// Dont show skill titles
#define STATF_SpiritSpeak	0x00001000	// I can hear ghosts clearly.
#define STATF_Insubstantial	0x00002000	// Ghost has not manifest. or GM hidden
#define STATF_EmoteAction	0x00004000	// The creature will emote its actions to it's owners.
#define STATF_COMM_CRYSTAL	0x00008000	// I have a IT_COMM_CRYSTAL or listening item on me.
#define STATF_HasShield		0x00010000	// Using a shield
#define STATF_ArcherCanMove	0x00020000	// Can move with archery
#define STATF_Stone			0x00040000	// turned to stone.
#define STATF_Hovering		0x00080000	// hovering (flying gargoyle)
#define STATF_Fly			0x00100000	// Flying or running ? (anim)
//							0x00200000
#define STATF_Hallucinating	0x00400000	// eat 'shrooms or bad food.
#define STATF_Hidden		0x00800000	// Hidden (non-magical)
#define STATF_InDoors		0x01000000	// we are covered from the rain.
#define STATF_Criminal		0x02000000	// The guards will attack me. (someone has called guards)
#define STATF_Conjured		0x04000000	// This creature is conjured and will expire. (leave no corpse or loot)
#define STATF_Pet			0x08000000	// I am a pet/hirling. check for my owner memory.
#define STATF_Spawned		0x10000000	// I am spawned by a spawn item.
#define STATF_SaveParity	0x20000000	// Has this char been saved or not ?
#define STATF_Ridden		0x40000000	// This is the horse. (don't display me) I am being ridden
#define STATF_OnHorse		0x80000000	// Mounted on horseback.

	DWORD m_StatFlag;		// Flags above

#define SKILL_VARIANCE 100		// Difficulty modifier for determining success. 10.0 %
	unsigned short m_Skill[SKILL_QTY];	// List of skills ( skill * 10 )

	// This is a character that can either be NPC or PC.
	// Player vs NPC Stuff
	CClient * m_pClient;	// is the char a logged in m_pPlayer ?

public:
	struct LastAttackers {
		INT64	elapsed;
		DWORD	charUID;
		INT64	amountDone;
		INT64	threat;
		bool	ignore;
	};
	std::vector<LastAttackers> m_lastAttackers;
	
	struct NotoSaves {
		INT64		time;
		DWORD		charUID;
		NOTO_TYPE	value;
	};
	std::vector<NotoSaves> m_notoSaves;

	static const char *m_sClassName;
	CCharPlayer * m_pPlayer;	// May even be an off-line player !
	CCharNPC * m_pNPC;			// we can be both a player and an NPC if "controlled" ?
	CPartyDef * m_pParty;		// What party am i in ?
	CRegionWorld * m_pArea; // What region are we in now. (for guarded message)
	CRegionBase * m_pRoom; // What room we are in now.

	static LPCTSTR const sm_szRefKeys[];
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szTrigName[CTRIG_QTY+1];
	static const LAYER_TYPE sm_VendorLayers[3];

	// Combat stuff. cached data. (not saved)
	CGrayUID m_uidWeapon;		// current Wielded weapon.	(could just get rid of this ?)
	WORD m_defense;			// calculated armor worn (NOT intrinsic armor)

	height_t m_height;		//Height set in-game or under some trigger (height=) - for both items and chars

	signed int m_ModMaxWeight;
	unsigned int m_exp;			//	character experience
	unsigned int m_level;		//	character experience level
	BYTE m_iVisualRange;		//  Visual Range
//	short int m_ResPhysical;
//	short int m_ResFire;
//	short int m_ResCold;
//	short int m_ResPoison;
//	short int m_ResEnergy;
	//DIR_TYPE m_dirClimb;	// we are standing on a CAN_I_CLIMB or UFLAG2_CLIMBABLE, DIR_QTY = not on climbable
	bool m_fClimbUpdated;	// FixClimbHeight() called?
	bool m_fIgnoreNextPetCmd;	// return 1 in speech block for this pet will make it ignore target petcmds while allowing the rest to perform them
	height_t m_zClimbHeight;	// The height at the end of the climbable.

	// Saved stuff.
	DIR_TYPE m_dirFace;	// facing this dir.
	CGString m_sTitle;		// Special title such as "the guard" (replaces the normal skill title).
	CPointMap m_ptHome;			// What is our "home" region. (towns and bounding of NPC's)
	FONT_TYPE m_fonttype;	// Speech font to use // can client set this ?

	// In order to revert to original Hue and body.
	CREID_TYPE m_prev_id;		// Backup of body type for ghosts and poly
	HUE_TYPE m_prev_Hue;	// Backup of skin color. in case of polymorph etc.
	HUE_TYPE m_wBloodHue;	// Replicating CharDef's BloodColor on the char, or overriding it.

	// Client's local light
	BYTE m_LocalLight;

	// When events happen to the char. check here for reaction scripts.

	// Skills, Stats and health
	struct
	{
		short	m_base;
		short	m_mod;			// signed for modifier
		int		m_val;			// signed for karma
		int		m_max;			// max
		unsigned short m_regen;	// Tick time since last regen.
	} m_Stat[STAT_QTY];

	CServTime m_timeLastRegen;	// When did i get my last regen tick ?
	CServTime m_timeCreate;		// When was i created ?

	CServTime m_timeLastHitsUpdate;

	// Some character action in progress.
	SKILL_TYPE	m_Act_SkillCurrent;	// Currently using a skill. Could be combat skill.
	CGrayUID	m_Act_Targ;			// Current combat/action target
	CGrayUID	m_Act_TargPrv;		// Previous target.
	int			m_Act_Difficulty;	// -1 = fail skill. (0-100) for skill advance calc.
	CPointBase  m_Act_p;			// Moving to this location. or location of forge we are working on.

	union	// arg specific to the action type.(m_Act_SkillCurrent)
	{
		struct
		{
			DWORD m_Arg1;	// "ACTARG1"
			DWORD m_Arg2;	// "ACTARG2"
			DWORD m_Arg3;	// "ACTARG3"
		} m_atUnk;

		// SKILL_MAGERY
		// SKILL_NECROMANCY
		// SKILL_CHIVALRY
		// SKILL_BUSHIDO
		// SKILL_NINJITSU
		// SKILL_SPELLWEAVING
		struct
		{
			SPELL_TYPE	m_Spell;		// ACTARG1=Currently casting spell.
			CREID_TYPE	m_SummonID;		// ACTARG2=A sub arg of the skill. (summoned type ?)
			WORD	m_fSummonPet;		// ACTARG3=
		} m_atMagery;

		// SKILL_MUSICIANSHIP
		// SKILL_ENTICEMENT
		// SKILL_PROVOCATION
		// SKILL_PEACEMAKING
		struct
		{
			int m_iMusicDifficulty;		// ACTARG1=Base music diff, (whole thing won't work if this fails)
		} m_atMusician;

		// SKILL_ALCHEMY
		// SKILL_BLACKSMITHING
		// SKILL_BOWCRAFT
		// SKILL_CARPENTRY
		// SKILL_CARTOGRAPHY:
		// SKILL_INSCRIPTION
		// SKILL_TAILORING:
		// SKILL_TINKERING,

		struct	// creation type skill.
		{
			ITEMID_TYPE m_ItemID;		// ACTARG1=Making this item.
			WORD m_Stroke_Count;		// ACTARG2=For smithing, tinkering, etc. all requiring multi strokes.
			WORD m_Amount;				// How many of this item are we making?
		} m_atCreate;

		// SKILL_LUMBERJACKING,
		// SKILL_MINING
		// SKILL_FISHING
		struct
		{
			DWORD		m_junk1;
			DWORD		m_Stroke_Count;		// all requiring multi strokes.
			DWORD		m_ridType;			// type of item we're harvesting
		} m_atResource;

		// SKILL_TAMING
		// SKILL_MEDITATION
		struct
		{
			DWORD m_junk1;
			WORD m_Stroke_Count;		// all requiring multi strokes.
		} m_atTaming;

		// SKILL_SWORDSMANSHIP
		// SKILL_MACEFIGHTING,
		// SKILL_FENCING,
		// SKILL_WRESTLING
		struct
		{
			WAR_SWING_TYPE	m_War_Swing_State;	// We are in the war mode swing.
			// m_Act_Targ = who are we currently attacking?
			WORD			m_fMoved;
		} m_atFight;

		// SKILL_TRACKING
		struct
		{
			DIR_TYPE	m_PrvDir; // Previous direction of tracking target, used for when to notify player
		} m_atTracking;

		// NPCACT_FOLLOW_TARG
		struct
		{
			int	m_DistMin;		// Try to force this distance.
			int	m_DistMax;		// Try to force this distance.
			// m_Act_Targ = what am i folloiwng ?
		} m_atFollowTarg;

		// NPCACT_RIDDEN
		struct
		{
			CGrayUIDBase m_FigurineUID;	// This creature is being ridden by this object link. IT_FIGURINE IT_EQ_HORSE
		} m_atRidden;

		// NPCACT_TALK
		// NPCACT_TALK_FOLLOW
		struct
		{
			int	 m_HearUnknown;	// Speaking NPC has no idea what u're saying.
			int  m_WaitCount;	// How long have i been waiting (xN sec)
			// m_Act_Targ = who am i talking to ?
		} m_atTalk;

		// NPCACT_FLEE
		// m_Act_Targ = who am i fleeing from ?
		struct
		{
			int	 m_iStepsMax;	// how long should it take to get there.
			int	 m_iStepsCurrent;	// how long has it taken ?
		} m_atFlee;

		// NPCACT_LOOTING
		// m_Act_Targ = what am i looting ? (Corpse)
		// m_Act_TargPrv = targets parent
		// m_Act_p = location of the target.
		struct
		{
			int	 m_iDistEstimate;	// how long should it take to get there.
			int	 m_iDistCurrent;	// how long has it taken ?
		} m_atLooting;

		// NPCACT_TRAINING
		// m_Act_Targ = what am i training on
		// m_Act_TargPrv = weapon
		//
	};

public:
	CChar( CREID_TYPE id );
	virtual ~CChar(); // Delete character
	bool DupeFrom( CChar * pChar, bool fNewbieItems);

private:
	CChar(const CChar& copy);
	CChar& operator=(const CChar& other);

public:
	// Status and attributes ------------------------------------
	int IsWeird() const;
	signed char GetFixZ( CPointMap pt, unsigned long wBlockFlags = 0);
	virtual void Delete(bool bforce = false);
	virtual bool NotifyDelete();
	bool IsStatFlag( DWORD dwStatFlag ) const
	{
		return(( m_StatFlag & dwStatFlag) ? true : false );
	}
	void StatFlag_Set( DWORD dwStatFlag )
	{
		m_StatFlag |= dwStatFlag;
	}
	void StatFlag_Clear( DWORD dwStatFlag )
	{
		m_StatFlag &= ~dwStatFlag;
	}
	void StatFlag_Mod( DWORD dwStatFlag, bool fMod )
	{
		if ( fMod )
			m_StatFlag |= dwStatFlag;
		else
			m_StatFlag &= ~dwStatFlag;
	}
	bool IsPriv( WORD flag ) const
	{	// PRIV_GM flags
		if ( m_pPlayer == NULL )
			return( false );	// NPC's have no privs.
		return( m_pPlayer->GetAccount()->IsPriv( flag ));
	}
	PLEVEL_TYPE GetPrivLevel() const
	{
		// The higher the better. // PLEVEL_Counsel
		if ( ! m_pPlayer )
			return( PLEVEL_Player );
		return( m_pPlayer->GetAccount()->GetPrivLevel());
	}

	CCharBase * Char_GetDef() const
	{
		return( STATIC_CAST <CCharBase*>( Base_GetDef()));
	}
	CRegionWorld * GetRegion() const
	{
		return m_pArea; // What region are we in now. (for guarded message)
	}
	CRegionBase * GetRoom() const
	{
		return m_pRoom; // What room are we in now.
	}
	int GetSight() const
	{
		return static_cast<int>(m_iVisualRange);
	}
	void SetSight(BYTE newSight)
	{
		m_iVisualRange = newSight;
		if ( IsClient() )
			GetClient()->addVisualRange(m_iVisualRange);
	}
	
	bool Can( WORD wCan ) const
	{
		return(( m_Can & wCan ) ? true : false );
	}
	bool IsResourceMatch( RESOURCE_ID_BASE rid, DWORD dwArg );
	bool IsResourceMatch( RESOURCE_ID_BASE rid, DWORD dwArg, DWORD dwArgResearch );

	bool IsSpeakAsGhost() const
	{
		return( IsStatFlag(STATF_DEAD) &&
			! IsStatFlag( STATF_SpiritSpeak ) &&
			! IsPriv( PRIV_GM ));
	}
	bool CanUnderstandGhost() const
	{
		// Can i understand player ghost speak ?
		if ( m_pNPC && m_pNPC->m_Brain == NPCBRAIN_HEALER )
			return( true );
		if ( Skill_GetBase( SKILL_SPIRITSPEAK ) >= g_Cfg.m_iMediumCanHearGhosts )
			return( true );
		return( IsStatFlag( STATF_SpiritSpeak|STATF_DEAD ) || IsPriv( PRIV_GM|PRIV_HEARALL ));
	}
	bool IsPlayableCharacter() const
	{
		return( IsHuman() || IsElf() || IsGargoyle() );
	}
	bool IsHuman() const
	{
		return( CCharBase::IsHumanID( GetDispID()));
	}
	bool IsElf() const
	{
		return( CCharBase::IsElfID( GetDispID()));
	}
	bool IsGargoyle() const
	{
		return( CCharBase::IsGargoyleID( GetDispID()));
	}
	int	 GetHealthPercent() const;
	LPCTSTR GetTradeTitle() const; // Paperdoll title for character p (2)

	// Information about us.
	CREID_TYPE GetID() const
	{
		CCharBase * pCharDef = Char_GetDef();
		ASSERT(pCharDef);
		return( pCharDef->GetID());
	}
	WORD GetBaseID() const
	{
		// future: strongly typed enums will remove the need for this cast
		return( static_cast<WORD>(GetID()));
	}
	CREID_TYPE GetDispID() const
	{
		CCharBase * pCharDef = Char_GetDef();
		ASSERT(pCharDef);
		return( pCharDef->GetDispID());
	}
	void SetID( CREID_TYPE id );

	LPCTSTR GetKeyStr( LPCTSTR pszKey ) const
	{
		return m_TagDefs.GetKeyStr( pszKey );
	}

	INT64 GetKeyNum( LPCTSTR pszKey, bool fZero = false ) const
	{
		return m_TagDefs.GetKeyNum( pszKey, fZero );
	}

	CVarDefCont * GetKey( LPCTSTR pszKey, bool fDef ) const
	{
		CVarDefCont	* pVar	= m_TagDefs.GetKey( pszKey );
		if ( !fDef || pVar )	return pVar;
		CCharBase * pCharDef = Char_GetDef();
		ASSERT(pCharDef);
		return pCharDef->m_TagDefs.GetKey( pszKey );
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

	LPCTSTR GetName() const
	{
		return GetName( true );
	}

	LPCTSTR GetNameWithoutIncognito() const
	{
		if ( IsStatFlag( STATF_Incognito ) )
		{
			CItem * pSpell = NULL;
			pSpell = LayerFind(LAYER_SPELL_Incognito);
			if ( pSpell == NULL )
				pSpell = LayerFind(LAYER_FLAG_Potion);

			if ( pSpell && pSpell->IsType(IT_SPELL) && (pSpell->m_itSpell.m_spell == SPELL_Incognito))
				return pSpell->GetName();
		}

		return GetName();
	}

	LPCTSTR GetName( bool fAllowAlt ) const
	{
		if ( fAllowAlt )
		{
			LPCTSTR pAltName = GetKeyStr( "NAME.ALT" );
			if ( pAltName && *pAltName )
				return pAltName;
		}
		if ( ! IsIndividualName())			// allow some creatures to go unnamed.
		{
			CCharBase * pCharDef = Char_GetDef();
			ASSERT(pCharDef);
			return( pCharDef->GetTypeName());	// Just use it's type name instead.
		}
		return( CObjBase::GetName());
	}

	bool SetName( LPCTSTR pName );
	height_t GetHeightMount( bool fEyeSubstract = false ) const;
	height_t GetHeight() const;

	bool CanSeeTrue( const CChar * pChar = NULL ) const;
	bool CanSeeAsDead( const CChar * pChar = NULL ) const;
	bool CanSeeInContainer( const CItemContainer * pContItem ) const;
	bool CanSee( const CObjBaseTemplate * pObj ) const;
	inline bool CanSeeLOS_New_Failed( CPointMap * pptBlock, CPointMap &ptNow ) const;
	bool CanSeeLOS_New( const CPointMap & pd, CPointMap * pBlock = NULL, int iMaxDist = UO_MAP_VIEW_SIGHT, WORD wFlags = 0 ) const;
	bool CanSeeLOS( const CPointMap & pd, CPointMap * pBlock = NULL, int iMaxDist = UO_MAP_VIEW_SIGHT, WORD wFlags = 0 ) const;
	bool CanSeeLOS( const CObjBaseTemplate * pObj, WORD wFlags = 0  ) const;

	#define LOS_NB_LOCAL_TERRAIN	0x00001 // Terrain inside a region I am standing in does not block LOS
	#define LOS_NB_LOCAL_STATIC		0x00002 // Static items inside a region I am standing in do not block LOS
	#define LOS_NB_LOCAL_DYNAMIC	0x00004 // Dynamic items inside a region I am standing in do not block LOS
	#define LOS_NB_LOCAL_MULTI		0x00008 // Multi items inside a region I am standing in do not block LOS
	#define LOS_NB_TERRAIN			0x00010 // Terrain does not block LOS at all
	#define LOS_NB_STATIC			0x00020 // Static items do not block LOS at all
	#define LOS_NB_DYNAMIC			0x00040 // Dynamic items do not block LOS at all
	#define LOS_NB_MULTI			0x00080 // Multi items do not block LOS at all
	#define LOS_NB_WINDOWS			0x00100 // Windows do not block LOS (e.g. Archery + Magery)
	#define LOS_NO_OTHER_REGION		0x00200 // Do not allow LOS path checking to go out of your region
	#define LOS_NC_MULTI			0x00400 // Do not allow LOS path checking to go through (no cross) a multi region (except the one you are standing in)
	#define LOS_FISHING				0x00800 // Do not allow LOS path checking to go through objects or terrain which do not represent water
	#define LOS_NC_WATER			0x01000	// Water does not block LOS at all.



	bool CanHear( const CObjBaseTemplate * pSrc, TALKMODE_TYPE mode ) const;
	bool CanSeeItem( const CItem * pItem ) const
	{
		ASSERT(pItem);
		if ( ! IsPriv( PRIV_GM ))
		{
			if ( pItem->IsAttr( ATTR_INVIS ))
				return( false );
		}
		return( true );
	}
	bool CanTouch( const CPointMap & pt ) const;
	bool CanTouch( const CObjBase * pObj ) const;
	IT_TYPE CanTouchStatic( CPointMap & pt, ITEMID_TYPE id, CItem * pItem );
	bool CanMove( CItem * pItem, bool fMsg = true ) const;
	BYTE GetLightLevel() const;
	bool CanUse( CItem * pItem, bool fMoveOrConsume ) const;
	bool IsMountCapable() const;

	int  Food_CanEat( CObjBase * pObj ) const;
	int  Food_GetLevelPercent() const;
	LPCTSTR Food_GetLevelMessage( bool fPet, bool fHappy ) const;

public:
	short	Stat_GetAdjusted( STAT_TYPE i ) const;

	void	Stat_SetBase( STAT_TYPE i, short iVal );
	short	Stat_GetBase( STAT_TYPE i ) const;
	void	Stat_AddBase( STAT_TYPE i, short iVal );

	void	Stat_AddMod( STAT_TYPE i, short iVal );
	void	Stat_SetMod( STAT_TYPE i, short iVal );
	short	Stat_GetMod( STAT_TYPE i ) const;

	void	Stat_SetVal( STAT_TYPE i, int iVal );
	int		Stat_GetVal( STAT_TYPE i ) const;

	void	Stat_SetMax( STAT_TYPE i, int iVal );
	int		Stat_GetMax( STAT_TYPE i ) const;
	int		Stat_GetSum() const;

	short	Stat_GetLimit( STAT_TYPE i ) const;

	void Stat_Experience(STAT_TYPE stat, int difficulty);

	bool Stat_Decrease( STAT_TYPE stat, SKILL_TYPE skill = (SKILL_TYPE)NULL);

	SKILLLOCK_TYPE Stat_GetLock(STAT_TYPE stat)
	{
		if (!m_pPlayer)
			return SKILLLOCK_UP;	// Always raising status for NPCs.
		return m_pPlayer->Stat_GetLock(stat);
	};

	void Stat_SetLock(STAT_TYPE stat, SKILLLOCK_TYPE state)
	{
		if (!m_pPlayer)
			return;
		return m_pPlayer->Stat_SetLock(stat,state);
	};


	// Location and movement ------------------------------------
private:
	bool TeleportToCli( int iType, int iArgs );
	bool TeleportToObj( int iType, TCHAR * pszArgs );
private:
	CRegionBase * CheckValidMove( CPointBase & ptDest, WORD * pwBlockFlags, DIR_TYPE dir, height_t * ClimbHeight, bool fPathFinding = false ) const;
	void FixClimbHeight();
	bool MoveToRegion( CRegionWorld * pNewArea, bool fAllowReject);
	bool MoveToRoom( CRegionBase * pNewRoom, bool fAllowReject);
	bool IsVerticalSpace( CPointMap ptDest, bool fForceMount = false );

public:
	CChar* GetNext() const
	{
		return( STATIC_CAST <CChar*>( CObjBase::GetNext()));
	}
	CObjBaseTemplate * GetTopLevelObj() const
	{
		// Get the object that has a location in the world. (Ground level)
		return const_cast<CChar*>(this);
	}

	bool IsSwimming() const;

	bool MoveToRegionReTest( DWORD dwType )
	{
		return( MoveToRegion( dynamic_cast <CRegionWorld *>( GetTopPoint().GetRegion( dwType )), false));
	}
	bool MoveToChar(CPointMap pt, bool bForceFix = false);
	bool MoveTo(CPointMap pt, bool bForceFix = false)
	{
		m_fClimbUpdated = false; // update climb height
		return MoveToChar( pt, bForceFix);
	}
	virtual void SetTopZ( signed char z )
	{
		CObjBaseTemplate::SetTopZ( z );
		m_fClimbUpdated = false; // update climb height
		FixClimbHeight();
	}
	bool MoveToValidSpot(DIR_TYPE dir, int iDist, int iDistStart = 1, bool bFromShip = false);
	virtual bool MoveNearObj( const CObjBaseTemplate * pObj, int iSteps = 0, DWORD dwCan = CAN_C_WALK )
	{
		UNREFERENCED_PARAMETER(dwCan);
		return CObjBase::MoveNearObj( pObj, iSteps, GetMoveBlockFlags());
	}
	bool MoveNear( CPointMap pt, int iSteps = 0, DWORD dwCan = CAN_C_WALK )
	{
		UNREFERENCED_PARAMETER(dwCan);
		return CObjBase::MoveNear( pt, iSteps, GetMoveBlockFlags());
	}

	CRegionBase * CanMoveWalkTo( CPointBase & pt, bool fCheckChars = true, bool fCheckOnly = false, DIR_TYPE dir = DIR_QTY, bool fPathFinding = false );
	void CheckRevealOnMove();
	bool CheckLocation( bool fStanding = false );

public:
	// Client Player specific stuff. -------------------------
	void ClientAttach( CClient * pClient );
	void ClientDetach();
	bool IsClient() const { return( m_pClient != NULL ); }
	CClient * GetClient() const
	{
		return( m_pClient );
	}

	bool SetPrivLevel( CTextConsole * pSrc, LPCTSTR pszFlags );
	bool CanDisturb( const CChar * pChar ) const;
	void SetDisconnected();
	bool SetPlayerAccount( CAccount * pAccount );
	bool SetPlayerAccount( LPCTSTR pszAccount );
	bool SetNPCBrain( NPCBRAIN_TYPE NPCBrain );
	NPCBRAIN_TYPE GetNPCBrain( bool fDefault = true ) const; // return 1 for animal, 2 for monster, 3 for NPC humans and PCs
	void ClearNPC();
	void ClearPlayer();

public:
	void ObjMessage( LPCTSTR pMsg, const CObjBase * pSrc ) const
	{
		if ( ! IsClient())
			return;
		GetClient()->addObjMessage( pMsg, pSrc );
	}
	void SysMessage( LPCTSTR pMsg ) const	// Push a message back to the client if there is one.
	{
		if ( ! IsClient())
			return;
		GetClient()->SysMessage( pMsg );
	}

	void UpdateStatsFlag() const;
	void UpdateStatVal( STAT_TYPE x, int iChange = 0, int iLimit = 0 );
	void UpdateHitsFlag();
	void UpdateModeFlag();
	void UpdateManaFlag() const;
	void UpdateStamFlag() const;
	void UpdateHitsForOthers() const;
	ANIM_TYPE GenerateAnimate(ANIM_TYPE action, bool fTranslate = true, bool fBackward = false, BYTE iFrameDelay = 0, BYTE iAnimLen = 7);
	bool UpdateAnimate(ANIM_TYPE action, bool fTranslate = true, bool fBackward = false, BYTE iFrameDelay = 0, BYTE iAnimLen = 7);

	void UpdateMode(  CClient * pExcludeClient = NULL, bool fFull= false );
	void UpdateSpeedMode();
	void UpdateVisualRange();
	void UpdateMove( const CPointMap & pold, CClient * pClientExclude = NULL, bool fFull = false, bool fOnlydir = false );
	void UpdateDir( DIR_TYPE dir );
	void UpdateDir( const CPointMap & pt );
	void UpdateDir( const CObjBaseTemplate * pObj );
	void UpdateDrag( CItem * pItem, CObjBase * pCont = NULL, CPointMap * pp = NULL );
	void Update(const CClient * pClientExclude = NULL);

public:
	LPCTSTR GetPronoun() const;	// he
	LPCTSTR GetPossessPronoun() const;	// his
	BYTE GetModeFlag( bool fTrueSight = false, const CClient* pViewer = NULL ) const;
	BYTE GetDirFlag(bool fSquelchForwardStep = false) const
	{
		// future: strongly typed enums will remove the need for this cast
		BYTE dir = static_cast<BYTE>(m_dirFace);
		ASSERT( dir<DIR_QTY );

		if ( fSquelchForwardStep )
		{
			// not so sure this is an intended 'feature' but it seems to work (5.0.2d)
			switch ( dir )
			{
				case DIR_S:
					return 0x2a;	// 0x32; 0x5a; 0x5c; all also work
				case DIR_SW:
					return 0x1d;	// 0x29; 0x5d; 0x65; all also work
				case DIR_W:
					return 0x60;
				default:
					return ( dir | 0x08 );
			}
		}

		if ( IsStatFlag( STATF_Fly ))
			dir |= 0x80; // running/flying ?
		
		return( dir );
	}
	DWORD GetMoveBlockFlags(bool bIgnoreGM = false) const
	{
		// What things block us ?
		if ( IsPriv(PRIV_GM|PRIV_ALLMOVE) && !bIgnoreGM)	// nothing blocks us.
			return( 0xFFFF );
		
		DWORD dwCan = m_Can;
		CCharBase * pCharDef = Char_GetDef();
		if ((pCharDef) && (pCharDef->Can(CAN_C_GHOST)))
			dwCan |= CAN_C_GHOST;

		if ( IsStatFlag(STATF_Hovering) )
			dwCan |= CAN_C_HOVER;

		// Inversion of MT_INDOORS, so MT_INDOORS should be named MT_NOINDOORS now.
		if (dwCan & CAN_C_INDOORS)
			dwCan &= ~CAN_C_INDOORS;
		else
			dwCan |= CAN_C_INDOORS;

		return( dwCan & CAN_C_MOVEMASK );
	}

	int FixWeirdness();
	void CreateNewCharCheck();

private:
	// Contents/Carry stuff. ---------------------------------
	void ContentAdd( CItem * pItem )
	{
		LayerAdd( pItem, LAYER_QTY );
	}
protected:
	void OnRemoveOb( CGObListRec* pObRec );	// Override this = called when removed from list.
public:
	LAYER_TYPE CanEquipLayer( CItem * pItem, LAYER_TYPE layer, CChar * pCharMsg, bool fTest );
	CItem * LayerFind( LAYER_TYPE layer ) const;
	void LayerAdd( CItem * pItem, LAYER_TYPE layer = LAYER_QTY );
	
	TRIGRET_TYPE OnCharTrigForLayerLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * pResult, LAYER_TYPE layer );
	TRIGRET_TYPE OnCharTrigForMemTypeLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * pResult, WORD wMemType );

	void OnWeightChange( int iChange );
	int GetWeight(WORD amount = 0) const
	{
		return( CContainer::GetTotalWeight());
	}
	int GetWeightLoadPercent( int iWeight ) const;
	bool CanCarry( const CItem * pItem ) const;

	CItem * GetSpellbook( SPELL_TYPE iSpell = SPELL_Clumsy ) const;
	CItemContainer * GetPack() const
	{
		return( dynamic_cast <CItemContainer *>(LayerFind( LAYER_PACK )));
	}
	CItemContainer * GetBank( LAYER_TYPE layer = LAYER_BANKBOX );
	CItemContainer * GetPackSafe()
	{
		return( GetBank(LAYER_PACK));
	}
	void AddGoldToPack( int iAmount, CItemContainer * pPack=NULL );

//private:
	virtual TRIGRET_TYPE OnTrigger( LPCTSTR pTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs );

public:
	// Load/Save----------------------------------

	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc );
	virtual bool r_LoadVal( CScript & s );
	virtual bool r_Load( CScript & s );  // Load a character from Script
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & s, CTextConsole * pSrc = NULL );
	virtual void r_Write( CScript & s );

	void r_WriteParity( CScript & s );	

	TRIGRET_TYPE OnTrigger( CTRIG_TYPE trigger, CTextConsole * pSrc, CScriptTriggerArgs * pArgs = NULL )
	{
		ASSERT( trigger < CTRIG_QTY );
		return( OnTrigger( MAKEINTRESOURCE(trigger), pSrc, pArgs ));
	}

private:
	// Noto/Karma stuff. --------------------------------
	void Noto_Karma( int iKarma, int iBottom=INT_MIN, bool bMessage = false );
	void Noto_Fame( int iFameChange );
	void Noto_ChangeNewMsg( int iPrv );
	void Noto_ChangeDeltaMsg( int iDelta, LPCTSTR pszType );

public:
	NOTO_TYPE Noto_GetFlag( const CChar * pChar, bool fIncog = false, bool fInvul = false ) const;
	NOTO_TYPE Noto_CalcFlag( const CChar * pChar, bool fIncog = false, bool fInvul = false ) const;
	HUE_TYPE Noto_GetHue( const CChar * pChar, bool fIncog = false ) const;
	bool Noto_IsNeutral() const;
	bool Noto_IsMurderer() const;
	bool Noto_IsEvil() const;
	bool Noto_IsCriminal() const
	{
		// do the guards hate me ?
		if ( IsStatFlag( STATF_Criminal ))
			return( true );
		return Noto_IsEvil();
	}
	int Noto_GetLevel() const;
	LPCTSTR Noto_GetFameTitle() const;
	LPCTSTR Noto_GetTitle() const;

	void Noto_Kill(CChar * pKill, bool fPetKill = false, int iOtherKillers = 0);
	bool Noto_Criminal( CChar * pChar = NULL);
	void Noto_Murder();
	void Noto_KarmaChangeMessage( int iKarmaChange, int iLimit );
	int NotoSave() { return static_cast<int>(m_notoSaves.size()); }
	void NotoSave_Add( CChar * pChar, NOTO_TYPE value );
	NOTO_TYPE NotoSave_GetValue( int id );
	NOTO_TYPE NotoSave_GetValue( CChar * pChar );
	INT64 NotoSave_GetTime( int id );
	void NotoSave_SetValue( CChar * pChar, NOTO_TYPE value );
	void NotoSave_SetValue( int pChar, NOTO_TYPE value);
	void NotoSave_Clear();
	void NotoSave_Update();
	void NotoSave_Resend( int id );
	int NotoSave_GetID( CChar * pChar );
	int NotoSave_GetID( CGrayUID pChar );
	CChar * NotoSave_GetUID( int index );
	bool NotoSave_Delete( CChar * pChar, bool bForced );
	void NotoSave_CheckTimeout();

	bool IsTakeCrime( const CItem * pItem, CChar ** ppCharMark = NULL ) const;

	void ChangeExperience(int delta = 0, CChar *pCharDead = NULL);
	int GetSkillTotal(int what = 0, bool how = true);

public:
	// skills and actions. -------------------------------------------
	static bool IsSkillBase( SKILL_TYPE skill );
	static bool IsSkillNPC( SKILL_TYPE skill );
	static bool IsSkillMagic( SKILL_TYPE skill );
	static bool IsSkillCraft( SKILL_TYPE skill );
	static bool IsSkillGather( SKILL_TYPE skill );

	SKILL_TYPE Skill_GetBest( unsigned int iRank = 0 ) const; // Which skill is the highest for character p
	SKILL_TYPE Skill_GetActive() const
	{
		return( m_Act_SkillCurrent );
	}
	LPCTSTR Skill_GetName( bool fUse = false ) const;
	unsigned short Skill_GetBase( SKILL_TYPE skill ) const
	{
		ASSERT( IsSkillBase(skill));
		return( m_Skill[skill] );
	}
	int Skill_GetMax( SKILL_TYPE skill ) const;
	int Skill_GetSum() const;
	SKILLLOCK_TYPE Skill_GetLock( SKILL_TYPE skill ) const
	{
		if ( ! m_pPlayer )
			return( SKILLLOCK_UP );
		return( m_pPlayer->Skill_GetLock(skill));
	}
	unsigned short Skill_GetAdjusted( SKILL_TYPE skill ) const;
	void Skill_SetBase( SKILL_TYPE skill, int iValue );
	bool Skill_UseQuick( SKILL_TYPE skill, INT64 difficulty, bool bAllowGain = true );

	bool Skill_CheckSuccess( SKILL_TYPE skill, int difficulty ) const;
	bool Skill_Wait( SKILL_TYPE skilltry );
	bool Skill_Start( SKILL_TYPE skill, int iDifficulty = 0 ); // calc skill progress.
	void Skill_Fail( bool fCancel = false );
	int Skill_Stroke( bool fResource);				// Strokes in crafting skills, calling for SkillStroke trig
	ANIM_TYPE Skill_GetAnim( SKILL_TYPE skill);
	int Skill_GetSound( SKILL_TYPE skill);
	int Skill_Stage( SKTRIG_TYPE stage );
	TRIGRET_TYPE	Skill_OnTrigger( SKILL_TYPE skill, SKTRIG_TYPE  stage);
	TRIGRET_TYPE	Skill_OnTrigger( SKILL_TYPE skill, SKTRIG_TYPE  stage, CScriptTriggerArgs * pArgs); //pArgs.m_iN1 will be rewritten with skill

	TRIGRET_TYPE	Skill_OnCharTrigger( SKILL_TYPE skill, CTRIG_TYPE ctrig);
	TRIGRET_TYPE	Skill_OnCharTrigger( SKILL_TYPE skill, CTRIG_TYPE ctrig, CScriptTriggerArgs * pArgs); //pArgs.m_iN1 will be rewritten with skill

	bool Skill_Mining_Smelt( CItem * pItemOre, CItem * pItemTarg );
	bool Skill_Tracking( CGrayUID uidTarg, DIR_TYPE & dirPrv, int iDistMax = SHRT_MAX );
	bool Skill_MakeItem( ITEMID_TYPE id, CGrayUID uidTarg, SKTRIG_TYPE stage, bool fSkillOnly = false, int iReplicationQty = 1 );
	bool Skill_MakeItem( ITEMID_TYPE id, ITEMID_TYPE material, CGrayUID uidTarg, SKTRIG_TYPE stage, bool fSkillOnly = false, int iReplicationQty = 1 );
	bool Skill_MakeItem_Success();
	bool Skill_Snoop_Check( const CItemContainer * pItem );
	void Skill_Cleanup();	 // may have just cancelled targetting.

	// test for skill towards making an item
	int SkillResourceTest( const CResourceQtyArray * pResources );

	void Spell_Effect_Remove(CItem * pSpell);
	void Spell_Effect_Add( CItem * pSpell );

private:
	int Skill_Done();	 // complete skill (async)
	void Skill_Decay();
	void Skill_Experience( SKILL_TYPE skill, int difficulty );

	int Skill_NaturalResource_Setup( CItem * pResBit );
	CItem * Skill_NaturalResource_Create( CItem * pResBit, SKILL_TYPE skill );
	void Skill_SetTimeout();
	INT64 Skill_GetTimeout();

	int	Skill_Scripted( SKTRIG_TYPE stage );

	int Skill_Inscription( SKTRIG_TYPE stage );
	int Skill_MakeItem( SKTRIG_TYPE stage );
	int Skill_Information( SKTRIG_TYPE stage );
	int Skill_Hiding( SKTRIG_TYPE stage );
	int Skill_Enticement( SKTRIG_TYPE stage );
	int Skill_Bowcraft( SKTRIG_TYPE stage );
	int Skill_Snooping( SKTRIG_TYPE stage );
	int Skill_Stealing( SKTRIG_TYPE stage );
	int Skill_Mining( SKTRIG_TYPE stage );
	int Skill_Lumberjack( SKTRIG_TYPE stage );
	int Skill_Taming( SKTRIG_TYPE stage );
	int Skill_Fishing( SKTRIG_TYPE stage );
	int Skill_Cartography(SKTRIG_TYPE stage);
	int Skill_DetectHidden(SKTRIG_TYPE stage);
	int Skill_Herding(SKTRIG_TYPE stage);
	int Skill_Blacksmith(SKTRIG_TYPE stage);
	int Skill_Lockpicking(SKTRIG_TYPE stage);
	int Skill_Peacemaking(SKTRIG_TYPE stage);
	int Skill_Alchemy(SKTRIG_TYPE stage);
	int Skill_Carpentry(SKTRIG_TYPE stage);
	int Skill_Provocation(SKTRIG_TYPE stage);
	int Skill_Poisoning(SKTRIG_TYPE stage);
	int Skill_Cooking(SKTRIG_TYPE stage);
	int Skill_Healing(SKTRIG_TYPE stage);
	int Skill_Meditation(SKTRIG_TYPE stage);
	int Skill_RemoveTrap(SKTRIG_TYPE stage);
	int Skill_Begging( SKTRIG_TYPE stage );
	int Skill_SpiritSpeak( SKTRIG_TYPE stage );
	int Skill_Magery( SKTRIG_TYPE stage );
	int Skill_Tracking( SKTRIG_TYPE stage );
	int Skill_Fighting( SKTRIG_TYPE stage );
	int Skill_Musicianship( SKTRIG_TYPE stage );
	int Skill_Tailoring( SKTRIG_TYPE stage );

	int Skill_Act_Napping(SKTRIG_TYPE stage);
	int Skill_Act_Throwing(SKTRIG_TYPE stage);
	int Skill_Act_Breath(SKTRIG_TYPE stage);
	int Skill_Act_Training( SKTRIG_TYPE stage );
	int Skill_Act_Looting( SKTRIG_TYPE stage );

	void Spell_Dispel( int iskilllevel );
CChar * Spell_Summon( CREID_TYPE id, CPointMap pt, bool fPet );
	bool Spell_Recall( CItem * pRune, bool fGate );
	CItem * Spell_Effect_Create( SPELL_TYPE spell, LAYER_TYPE layer, int iSkillLevel, int iDuration, CObjBase * pSrc = NULL );
	bool Spell_Equip_OnTick( CItem * pItem );

	void Spell_Bolt( CObjBase * pObj, ITEMID_TYPE idBolt, int iSkill );
	void Spell_Field( CPointMap pt, ITEMID_TYPE idEW, ITEMID_TYPE idNS, unsigned int fieldWidth, unsigned int fieldGauge, int iSkill );
	void Spell_Area( CPointMap pt, int iDist, int iSkill );
	bool Spell_TargCheck_Face();
	bool Spell_TargCheck();
	bool Spell_Unequip( LAYER_TYPE layer );

	int  Spell_CastStart();
	void Spell_CastFail();

public:
	inline bool Spell_SimpleEffect( CObjBase * pObj, CObjBase * pObjSrc, SPELL_TYPE &spell, int &iSkillLevel );
	bool Spell_CastDone();
	bool OnSpellEffect( SPELL_TYPE spell, CChar * pCharSrc, int iSkillLevel, CItem * pSourceItem );
	bool Spell_Resurrection(CItemCorpse * pCorpse = NULL, CChar * pCharSrc = NULL, bool bNoFail = false);
	bool Spell_Teleport( CPointBase pt, bool fTakePets = false, bool fCheckAntiMagic = true, ITEMID_TYPE iEffect = ITEMID_TEMPLATE, SOUND_TYPE iSound = 0x01fe );
	bool Spell_CanCast( SPELL_TYPE spell, bool fTest, CObjBase * pSrc, bool fFailMsg, bool fCheckAntiMagic = true );
	int	GetSpellEffect( SPELL_TYPE spell, int iSkillLevel, int iEffectMult );
	int	GetSpellDuration( SPELL_TYPE spell, int iSkillLevel, int iEffectMult );
	// Memories about objects in the world. -------------------
private:
	bool Memory_OnTick( CItemMemory * pMemory );
	bool Memory_UpdateFlags( CItemMemory * pMemory );
	bool Memory_UpdateClearTypes( CItemMemory * pMemory, WORD MemTypes );
	void Memory_AddTypes( CItemMemory * pMemory, WORD MemTypes );
	bool Memory_ClearTypes( CItemMemory * pMemory, WORD MemTypes );
	CItemMemory * Memory_CreateObj( CGrayUID uid, WORD MemTypes );
	CItemMemory * Memory_CreateObj( const CObjBase * pObj, WORD MemTypes )
	{
		ASSERT(pObj);
		return Memory_CreateObj( pObj->GetUID(), MemTypes );
	}

public:
	void Memory_ClearTypes( WORD MemTypes );
	CItemMemory * Memory_FindObj( CGrayUID uid ) const;
	CItemMemory * Memory_FindObj( const CObjBase * pObj ) const
	{
		if ( pObj == NULL )
			return( NULL );
		return Memory_FindObj( pObj->GetUID());
	}
	CItemMemory * Memory_AddObjTypes( CGrayUID uid, WORD MemTypes );
	CItemMemory * Memory_AddObjTypes( const CObjBase * pObj, WORD MemTypes )
	{
		ASSERT(pObj);
		return Memory_AddObjTypes( pObj->GetUID(), MemTypes );
	}
	CItemMemory * Memory_FindTypes( WORD MemTypes ) const;
	CItemMemory * Memory_FindObjTypes( const CObjBase * pObj, WORD MemTypes ) const
	{
		CItemMemory * pMemory = Memory_FindObj(pObj);
		if ( pMemory == NULL )
			return( NULL );
		if ( ! pMemory->IsMemoryTypes( MemTypes ))
			return( NULL );
		return( pMemory );
	}
	CItemMemory * Memory_FindGump( DWORD gump, DWORD uid = 0, bool bSdialog = false ) const;
	// -------- Public alias for MemoryCreateObj ------------------
	CItemMemory * Memory_AddObj( CGrayUID uid, WORD MemTypes )
	{
		return Memory_CreateObj( uid, MemTypes );
	}
	CItemMemory * Memory_AddObj( const CObjBase * pObj, WORD MemTypes )
	{
		return Memory_CreateObj( pObj, MemTypes );
	}
	// ------------------------------------------------------------

public:
	void SoundChar(CRESND_TYPE type);
	void Action_StartSpecial(CREID_TYPE id);

private:
	void OnNoticeCrime( CChar * pCriminal, const CChar * pCharMark );
public:
	bool CheckCrimeSeen( SKILL_TYPE SkillToSee, CChar * pCharMark, const CObjBase * pItem, LPCTSTR pAction );

private:
	// Armor, weapons and combat ------------------------------------
	int	CalcFightRange( CItem * pWeapon = NULL );

	SKILL_TYPE Fight_GetWeaponSkill() const;
	void Fight_ResetWeaponSwingTimer();
	int Fight_GetWeaponSwingTimer();
	bool Fight_IsActive() const;
public:
	int CalcArmorDefense() const;

	void Memory_Fight_Retreat( CChar * pTarg, CItemMemory * pFight );
	void Memory_Fight_Start( const CChar * pTarg );
	bool Memory_Fight_OnTick( CItemMemory * pMemory );

	bool Fight_Attack( const CChar * pCharTarg, bool toldByMaster = false );
	bool Fight_Clear( const CChar * pCharTarg , bool bForced = false );
	void Fight_ClearAll();
	CChar * Fight_FindBestTarget();
	bool Fight_AttackNext();
	void Fight_HitTry();
	WAR_SWING_TYPE Fight_Hit( CChar * pCharTarg );
	int  Fight_CalcDamage( const CItem * pWeapon, SKILL_TYPE skill, bool bNoRandom = false ) const;

	// Attacker System

	enum ATTACKER_CLEAR_TYPE
	{
		ATTACKER_CLEAR_FORCED		= 0,
		ATTACKER_CLEAR_ELAPSED		= 1,
		ATTACKER_CLEAR_DISTANCE		= 2,
		ATTACKER_CLEAR_REMOVEDCHAR	= 3,
		ATTACKER_CLEAR_SCRIPT		= 4,
		//ATTACKER_CLEAR_DEATH		= 3,
	};

	int	 Attacker() { return static_cast<int>(m_lastAttackers.size()); }
	//bool Attacker_Add( CChar * pChar);
	bool Attacker_Add( CChar * pChar, INT64 threat = 0);
	bool Attacker_Delete(CChar * pChar, bool bForced = false, ATTACKER_CLEAR_TYPE type = ATTACKER_CLEAR_FORCED);
	bool Attacker_Delete(int id, bool bForced = false, ATTACKER_CLEAR_TYPE type = ATTACKER_CLEAR_FORCED);
	void Attacker_RemoveChar();
	void Attacker_Clear();
	void Attacker_CheckTimeout();
	INT64 Attacker_GetDam( int attacker );
	void  Attacker_SetDam( CChar * pChar, INT64 value );
	void  Attacker_SetDam( int attacker, INT64 value );
	CChar * Attacker_GetUID( int attacker);
	INT64  Attacker_GetElapsed( int attacker );
	void  Attacker_SetElapsed( CChar * pChar, INT64 value );
	void  Attacker_SetElapsed( int pChar, INT64 value );
	INT64  Attacker_GetThreat( int attacker );
	void  Attacker_SetThreat( CChar * pChar, INT64 value );
	void  Attacker_SetThreat(int pChar, INT64 value);
	bool Attacker_GetIgnore(int pChar);
	bool Attacker_GetIgnore(CChar * pChar);
	void Attacker_SetIgnore(int pChar, bool fIgnore);
	void Attacker_SetIgnore(CChar * pChar, bool fIgnore);
	INT64 Attacker_GetHighestThreat();
	int  Attacker_GetID( CChar * pChar );
	int  Attacker_GetID( LPCTSTR pChar );
	int  Attacker_GetID( CGrayUID pChar );
	int  IsAttackedBy( CChar * pChar ) { return Attacker_GetID( pChar); } 
	CChar * Attacker_FindBestTarget( bool bUseThreat = false );
	//

	bool Player_OnVerb( CScript &s, CTextConsole * pSrc );
	void InitPlayer( CClient * pClient, const char * pszCharname, bool bFemale, RACE_TYPE rtRace, short wStr, short wDex, short wInt, PROFESSION_TYPE iProf, SKILL_TYPE skSkill1, int iSkillVal1, SKILL_TYPE skSkill2, int iSkillVal2, SKILL_TYPE skSkill3, int iSkillVal3, SKILL_TYPE skSkill4, int iSkillVal4, HUE_TYPE wSkinHue, ITEMID_TYPE idHair, HUE_TYPE wHairHue, ITEMID_TYPE idBeard, HUE_TYPE wBeardHue, HUE_TYPE wShirtHue, HUE_TYPE wPantsHue, int iStartLoc  );
	bool ReadScriptTrig(CCharBase * pCharDef, CTRIG_TYPE trig, bool bVendor = false);
	bool ReadScript(CResourceLock &s, bool bVendor = false);
	void NPC_LoadScript( bool fRestock );

	// Mounting and figurines
	bool Horse_Mount( CChar * pHorse ); // Remove horse char and give player a horse item
	bool Horse_UnMount(); // Remove horse char and give player a horse item

private:
	CItem * Horse_GetMountItem() const;
	CChar * Horse_GetMountChar() const;
public:
	CChar * Use_Figurine( CItem * pItem, int iPaces = 0 );
	CItem * Make_Figurine( CGrayUID uidOwner, ITEMID_TYPE id = ITEMID_NOTHING );
	CItem * NPC_Shrink();

	int  ItemPickup( CItem * pItem, int amount );
	bool ItemEquip( CItem * pItem, CChar * pCharMsg = NULL, bool fFromDClick = false );
	bool ItemEquipWeapon( bool fForce );
	bool ItemEquipArmor( bool fForce );
	bool ItemBounce( CItem * pItem );
	bool ItemDrop( CItem * pItem, const CPointMap & pt );

	void Flip();
	bool SetPoison( int iLevel, int iTicks, CChar * pCharSrc );
	bool SetPoisonCure( int iLevel, bool fExtra );
	bool CheckCorpseCrime( const CItemCorpse *pCorpse, bool fLooting, bool fTest );
	CItemCorpse * FindMyCorpse( bool ignoreLOS = false, int iRadius = 2) const;
	bool MakeCorpse_Fail();
	CItemCorpse * MakeCorpse( bool fFrontFall );
	bool RaiseCorpse( CItemCorpse * pCorpse );
	bool Death();
	bool Reveal( DWORD dwFlags = ( STATF_Invisible | STATF_Hidden | STATF_Sleeping ));
	void Jail( CTextConsole * pSrc, bool fSet, int iCell );
	void EatAnim( LPCTSTR pszName, int iQty );
	void CallGuards( CChar * pCriminal );

	#define DEATH_NOFAMECHANGE 0x01
	#define DEATH_NOCORPSE 0x02
	#define DEATH_NOLOOTDROP 0x04
	#define DEATH_NOCONJUREDEFFECT 0x08
	#define DEATH_HASCORPSE 0x010

	virtual void Speak( LPCTSTR pText, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL );
	virtual void SpeakUTF8( LPCTSTR pText, HUE_TYPE wHue= HUE_TEXT_DEF, TALKMODE_TYPE mode= TALKMODE_SAY, FONT_TYPE font= FONT_NORMAL, CLanguageID lang = 0 );
	virtual void SpeakUTF8Ex( const NWORD * pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang );

	bool OnFreezeCheck();
	void DropAll( CItemContainer * pCorpse = NULL, DWORD dwAttr = 0 );
	void UnEquipAllItems( CItemContainer * pCorpse = NULL, bool bLeaveHands = false );
	void Wake();
	void SleepStart( bool fFrontFall );

	void Guild_Resign( MEMORY_TYPE memtype );
	CItemStone * Guild_Find( MEMORY_TYPE memtype ) const;
	CStoneMember * Guild_FindMember( MEMORY_TYPE memtype ) const;
	LPCTSTR Guild_Abbrev( MEMORY_TYPE memtype ) const;
	LPCTSTR Guild_AbbrevBracket( MEMORY_TYPE memtype ) const;

	void Use_EatQty( CItem * pFood, int iQty = 1 );
	bool Use_Eat( CItem * pItem, int iQty = 1 );
	bool Use_MultiLockDown( CItem * pItemTarg );
	void Use_CarveCorpse( CItemCorpse * pCorpse );
	bool Use_Repair( CItem * pItem );
	int  Use_PlayMusic( CItem * pInstrument, int iDifficultyToPlay );
	void Use_Drink(CItem *pItem);
	bool Use_Cannon_Feed( CItem * pCannon, CItem * pFeed );
	bool Use_Item_Web( CItem * pItem );
	void Use_MoonGate( CItem * pItem );
	bool Use_Kindling( CItem * pKindling );
	bool Use_BedRoll( CItem * pItem );
	bool Use_Seed( CItem * pItem, CPointMap * pPoint );
	bool Use_Key( CItem * pKey, CItem * pItemTarg );
	bool Use_KeyChange( CItem * pItemTarg );
	bool Use_Train_PickPocketDip( CItem * pItem, bool fSetup );
	bool Use_Train_Dummy( CItem * pItem, bool fSetup );
	bool Use_Train_ArcheryButte( CItem * pButte, bool fSetup );
	bool Use_Item( CItem * pItem, bool fLink = false );
	bool Use_Obj( CObjBase * pObj, bool fTestTouch, bool fScript = false );

	// NPC AI -----------------------------------------
private:
	static CREID_TYPE NPC_GetAllyGroupType(CREID_TYPE idTest);

	bool NPC_StablePetRetrieve( CChar * pCharPlayer );
	bool NPC_StablePetSelect( CChar * pCharPlayer );

	int NPC_WantThisItem( CItem * pItem ) const;
	int NPC_GetWeaponUseScore( CItem * pItem );

	int  NPC_GetHostilityLevelToward( const CChar * pCharTarg ) const;
	int	 NPC_GetAttackContinueMotivation( CChar * pChar, int iMotivation = 0 ) const;
	int  NPC_GetAttackMotivation( CChar * pChar, int iMotivation = 0 ) const;
	bool NPC_CheckHirelingStatus();
	int  NPC_GetTrainMax( const CChar * pStudent, SKILL_TYPE Skill ) const;

	bool NPC_OnVerb( CScript &s, CTextConsole * pSrc = NULL );
	void NPC_OnHirePayMore( CItem * pGold, bool fHire = false );
public:
	bool NPC_OnHirePay( CChar * pCharSrc, CItemMemory * pMemory, CItem * pGold );
	bool NPC_OnHireHear( CChar * pCharSrc );
	int	 NPC_OnTrainCheck( CChar * pCharSrc, SKILL_TYPE Skill );
	bool NPC_OnTrainPay( CChar * pCharSrc, CItemMemory * pMemory, CItem * pGold );
	bool NPC_OnTrainHear( CChar * pCharSrc, LPCTSTR pCmd );
	bool NPC_TrainSkill( CChar * pCharSrc, SKILL_TYPE skill, int toTrain );
private:
	bool NPC_CheckWalkHere( const CPointBase & pt, const CRegionBase * pArea, WORD wBlockFlags ) const;
	void NPC_OnNoticeSnoop( CChar * pCharThief, CChar * pCharMark );

	bool NPC_LootContainer( CItemContainer * pItem );
	void NPC_LootMemory( CItem * pItem );
	bool NPC_LookAtCharGuard( CChar * pChar, bool bFromTrigger = false );
	bool NPC_LookAtCharHealer( CChar * pChar );
	bool NPC_LookAtCharHuman( CChar * pChar );
	bool NPC_LookAtCharMonster( CChar * pChar );
	bool NPC_LookAtChar( CChar * pChar, int iDist );
	bool NPC_LookAtItem( CItem * pItem, int iDist );
	bool NPC_LookAround( bool fForceCheckItems = false );
	int  NPC_WalkToPoint( bool fRun = false );
	bool NPC_FightMagery( CChar * pChar );
	bool NPC_FightArchery( CChar * pChar );
	bool NPC_FightMayCast() const;

	bool NPC_Act_Follow( bool fFlee = false, int maxDistance = 1, bool forceDistance = false );
	void NPC_Act_Guard();
	void NPC_Act_GoHome();
	bool NPC_Act_Talk();
	void NPC_Act_Wander();
	bool NPC_Act_Begging( CChar * pChar );
	void NPC_Act_Fight();
	void NPC_Act_Idle();
	inline void NPC_Act_Looting_CantGetItem( CItem * pItem );
	void NPC_Act_Looting();
	void NPC_Act_Flee();
	void NPC_Act_Goto(int iDist = 30);
	void NPC_Act_Runto(int iDist = 30);
	bool NPC_Act_Food();

	void NPC_ActStart_SpeakTo( CChar * pSrc );

	void NPC_OnTickAction();
	bool CHAR_OnTickFood( int nHungerPercent , int HitsHungerLoss);

public:
	void NPC_Pathfinding();		//	NPC thread AI - pathfinding
	void NPC_Food();			//	NPC thread AI - search for food
	void NPC_AI();				//	NPC thread AI - some general operations

	void NPC_PetDesert();	
	void NPC_PetClearOwners();
	bool NPC_PetSetOwner( CChar * pChar );
	CChar * NPC_PetGetOwner() const;
	bool NPC_IsOwnedBy( const CChar * pChar, bool fAllowGM = true ) const;
	bool NPC_CanSpeak() const;

	static CItemVendable * NPC_FindVendableItem( CItemVendable * pVendItem, CItemContainer * pVend1, CItemContainer * pVend2 );

	bool NPC_IsVendor() const
	{
		return (m_pNPC && m_pNPC->IsVendor());
	}
	bool NPC_Vendor_Restock(bool bForce = false, bool bFillStock = false);
	int NPC_GetVendorMarkup( const CChar * pChar ) const;

	void NPC_OnPetCommand( bool fSuccess, CChar * pMaster );
	bool NPC_OnHearPetCmd( LPCTSTR pszCmd, CChar * pSrc, bool fAllPets = false );
	bool NPC_OnHearPetCmdTarg( int iCmd, CChar * pSrc, CObjBase * pObj, const CPointMap & pt, LPCTSTR pszArgs );
	size_t  NPC_OnHearName( LPCTSTR pszText ) const;
	void NPC_OnHear( LPCTSTR pCmd, CChar * pSrc, bool fAllPets = false );
	bool NPC_OnItemGive( CChar * pCharSrc, CItem * pItem );
	bool NPC_SetVendorPrice( CItem * pItem, int iPrice );
	bool OnTriggerSpeech(bool bIsPet, LPCTSTR pszText, CChar * pSrc, TALKMODE_TYPE & mode, HUE_TYPE wHue = HUE_DEFAULT);

	// Outside events that occur to us.
	int  OnTakeDamage( int iDmg, CChar * pSrc, DAMAGE_TYPE uType );
	int  OnTakeDamageHitPoint( int iDmg, CChar * pSrc, DAMAGE_TYPE uType );
	void OnHarmedBy( CChar * pCharSrc, int iHarmQty );
	bool OnAttackedBy( CChar * pCharSrc, int iHarmQty, bool fPetsCommand = false, bool fShouldReveal = true );

	bool OnTickEquip( CItem * pItem );
	void OnTickFood( int iVal, int HitsHungerLoss );
	void OnTickStatusUpdate();
	bool OnTick();

	static CChar * CreateBasic( CREID_TYPE baseID );
	static CChar * CreateNPC( CREID_TYPE id );

	int GetAbilityFlags() const;

};

inline bool CChar::IsSkillBase( SKILL_TYPE skill ) // static
{
	// Is this in the base set of skills.
	return( IS_SKILL_BASE(skill));
}

inline bool CChar::IsSkillMagic( SKILL_TYPE skill ) // static
{
	if (g_Cfg.IsSkillFlag(skill, SKF_SCRIPTED))
		return false;
	if (g_Cfg.IsSkillFlag(skill, SKF_MAGIC))
		return true;
	switch(skill)
	{
		case SKILL_MAGERY:
		case SKILL_NECROMANCY:
		case SKILL_CHIVALRY:
		case SKILL_BUSHIDO:
		case SKILL_NINJITSU:
		case SKILL_SPELLWEAVING:
		case SKILL_MYSTICISM:
			return true;
		default:
			return false;
	}
}

inline bool CChar::IsSkillCraft( SKILL_TYPE skill ) // static
{
	if (g_Cfg.IsSkillFlag(skill, SKF_SCRIPTED))
		return false;
	if (g_Cfg.IsSkillFlag(skill, SKF_CRAFT))
		return true;
	/*switch (skill)
	{
		case SKILL_ALCHEMY:
		case SKILL_BLACKSMITHING:
		case SKILL_BOWCRAFT:
		case SKILL_CARPENTRY:
		case SKILL_COOKING:
		case SKILL_INSCRIPTION:
		case SKILL_TAILORING:
		case SKILL_TINKERING:
			return true;
		default:
			return false;
	}*/
	return false;
}

inline bool CChar::IsSkillGather( SKILL_TYPE skill ) // static
{
	if (g_Cfg.IsSkillFlag(skill, SKF_SCRIPTED))
		return false;
	if (g_Cfg.IsSkillFlag(skill, SKF_GATHER))
		return true;
	switch (skill)
	{
		case SKILL_MINING:
		case SKILL_FISHING:
		case SKILL_LUMBERJACKING:
			return true;
		default:
			return false;
	}
}

inline bool CChar::IsSkillNPC( SKILL_TYPE skill )  // static
{
	// Is this in the NPC set of skills.
	return( skill >= NPCACT_FOLLOW_TARG && skill < NPCACT_QTY );
}

#endif
