//
// CResource.h
//


#ifndef _INC_CRESOURCE_H
#define _INC_CRESOURCE_H
#pragma once

#include "../common/CAssoc.h"

class CAccount;
class CClient;
class CServerDef;

typedef CServerDef * CServerRef;

#define SKILL_MAX		(static_cast<SKILL_TYPE>(g_Cfg.m_iMaxSkill))
#define MAX_SKILL_SA	(static_cast<SKILL_TYPE>(58))	// The number of skills viewable by SA clients
#define MAX_SKILL_KR	(static_cast<SKILL_TYPE>(55))	// The number of skills viewable by KR clients
#define MAX_SKILL_ML	(static_cast<SKILL_TYPE>(55))	// The number of skills viewable by ML clients
#define MAX_SKILL_SE	(static_cast<SKILL_TYPE>(54))	// The number of skills viewable by SE clients
#define MAX_SKILL_AOS	(static_cast<SKILL_TYPE>(52))	// The number of skills viewable by AOS clients
#define MAX_SKILL_LBR	(static_cast<SKILL_TYPE>(49))	// The number of skills viewable by LBR clients
#define MAX_SKILL_T2A	(static_cast<SKILL_TYPE>(49))	// The number of skills viewable by T2A clients

// option flags
enum OF_TYPE
{
	OF_NoDClickTarget			= 0x0000001,
	OF_NoSmoothSailing			= 0x0000002,
	OF_ScaleDamageByDurability	= 0x0000004,
	OF_Command_Sysmsgs			= 0x0000008,
	OF_PetSlots					= 0x0000010,
	OF_OSIMultiSight			= 0x0000020,
	OF_Items_AutoName			= 0x0000040,
	OF_FileCommands				= 0x0000080,
	OF_NoItemNaming				= 0x0000100,
	OF_NoHouseMuteSpeech		= 0x0000200,
	OF_NoContextMenuLOS			= 0x0000400,
	OF_Flood_Protection			= 0x0001000,
	OF_Buffs					= 0x0002000,
	OF_NoPrefix					= 0x0004000,
	OF_DyeType					= 0x0008000,
	OF_DrinkIsFood				= 0x0010000,
	OF_DClickNoTurn				= 0x0020000,
	OF_Specific					= 0x1000000
};

enum EF_TYPE
{
	EF_NoDiagonalCheckLOS			= 0x0000001,
	EF_Unused02						= 0x0000002,
	EF_ItemStacking					= 0x0000004,
	EF_ItemStackDrop				= 0x0000008,
	EF_Unused010					= 0x0000010,
	EF_Intrinsic_Locals				= 0x0000020,
	EF_Item_Strict_Comparison		= 0x0000040,
	EF_Unused080					= 0x0000080,
	EF_Unused0100					= 0x0000100,
	EF_AllowTelnetPacketFilter		= 0x0000200,
	EF_Script_Profiler				= 0x0000400,
	EF_Size_Optimise				= 0x0000800,
	EF_Unused01000					= 0x0001000,
	EF_DamageTools					= 0x0002000,
	EF_UsePingServer				= 0x0008000,
	EF_Unused0100000				= 0x0010000,
	EF_FixCanSeeInClosedConts		= 0x0020000,
	EF_Unused0400000				= 0x0400000,
#ifndef _MTNETWORK
	EF_NetworkOutThread				= 0x0800000,
#endif
    EF_Specific						= 0x1000000,	// Specific behaviour, not completly tested
	EF_UltimaLive					= 0x2000000		//very Experimental!!!
};

enum MAGICFLAGS_TYPE
{
	MAGICF_NODIRCHANGE			= 0x0000001,	// not rotate player when casting/targeting
	MAGICF_PRECAST				= 0x0000002,	// use precasting (cast spell before targeting)
	MAGICF_IGNOREAR				= 0x0000004,	// magic ignore ar
	MAGICF_CANHARMSELF			= 0x0000008,	// i can do damage on self
	MAGICF_STACKSTATS			= 0x0000010,	// allow multiple stat spells at once
	MAGICF_FREEZEONCAST			= 0x0000020,	// disallow movement whilst casting
	MAGICF_SUMMONWALKCHECK		= 0x0000040,	// disallow summoning creatures to places they can't normally step
	MAGICF_NOFIELDSOVERWALLS	= 0x0000080,	// prevent fields from being formed over blocking objects.
	MAGICF_NOANIM				= 0x0000100,	// auto spellflag_no_anim on all spells
	MAGICF_OSIFORMULAS			= 0x0000200,	// calculated damage and duration based on OSI formulas
	MAGICF_NOCASTFROZENHANDS	= 0x0000400,		// can't cast spells if got paralyzed holding something on hands
	MAGICF_POLYMORPHSTATS		= 0x0000800		// Polymorph spells give out stats based on base chars (old behaviour backwards).
};

enum COMBATFLAGS_TYPE
{
	COMBAT_NODIRCHANGE			= 0x0000001,	// not rotate player when fighting
	COMBAT_FACECOMBAT			= 0x0000002,	// allow faced combat only
	COMBAT_PREHIT				= 0x0000004,	// allow prehit for close combat. first hit is instant (delay 0.1sec)
	COMBAT_ELEMENTAL_ENGINE		= 0x0000008,	// use DAM*/RES* to split damage/resist into Physical/Fire/Cold/Poison/Energy (AOS) instead use old AR (pre-AOS)
	COMBAT_DCLICKSELF_UNMOUNTS	= 0x0000020,	// unmount horse when dclicking self while in warmode
	COMBAT_ALLOWHITFROMSHIP		= 0x0000040,	// allow attacking opponents from ships
	COMBAT_OSIDAMAGEMOD			= 0x0000080,	// modify weapon damage OSI-style (taking anatomy, tactics, lumberjacking into account)
	COMBAT_ARCHERYCANMOVE		= 0x0000100,	// allow firing bow while moving
	COMBAT_STAYINRANGE			= 0x0000200,	// must be in range at the end of the swing or the hit will miss
	COMBAT_STACKARMOR			= 0x0001000,		// if a region is covered by more than one armor part, all AR will count
	COMBAT_NOPOISONHIT			= 0x0002000		// Uses old (55i like) poisoning style: Poisoning > 30.0 && (RAND(100.0)> Poisoning) for monsters OR weapon.morez && (RAND(100) < weapon.morez ) for poisoned weapons.
};

enum TOOLTIPMODE_TYPE
{
	TOOLTIPMODE_SENDFULL	= 0x00,	// always send full tooltip packet
	TOOLTIPMODE_SENDVERSION	= 0x01	// send version packet and wait for client to request full tooltip
};

enum BODYPART_TYPE
{
	ARMOR_HEAD = 0,
	ARMOR_NECK,
	ARMOR_BACK,
	ARMOR_CHEST,	// or thorax
	ARMOR_ARMS,
	ARMOR_HANDS,
	ARMOR_LEGS,
	ARMOR_FEET,
	ARMOR_QTY,		// All the parts that armor will cover.

	BODYPART_LEGS2,	// Alternate set of legs (spider)
	BODYPART_TAIL,	// Dragon, Snake, Alligator, etc. (tail attack?)
	BODYPART_WINGS,	// Dragon, Mongbat, Gargoyle
	BODYPART_CLAWS,	// can't wear any gloves here!
	BODYPART_HOOVES,	// No shoes
	BODYPART_HORNS,	// Bull, Daemon

	BODYPART_STALKS,		// Gazer or Corpser
	BODYPART_BRANCHES,	// Reaper.
	BODYPART_TRUNK,		// Reaper.
	BODYPART_PSEUDOPOD,	// Slime
	BODYPART_ABDOMEN,		// Spider or insect. asusme throax and chest are the same.

	BODYPART_QTY
};

#define DAMAGE_GOD			0x0001	// Nothing can block this.
#define DAMAGE_HIT_BLUNT	0x0002	// Physical hit of some sort.
#define DAMAGE_MAGIC		0x0004	// Magic blast of some sort. (we can be immune to magic to some extent)
#define DAMAGE_POISON		0x0008	// Or biological of some sort ? (HARM spell)
#define DAMAGE_FIRE			0x0010	// Fire damage of course.  (Some creatures are immune to fire)
#define DAMAGE_ENERGY		0x0020	// lightning.
#define DAMAGE_GENERAL		0x0080	// All over damage. As apposed to hitting just one point.
#define DAMAGE_ACIDIC		0x0100	// damages armor
#define DAMAGE_COLD			0x0200	// cold or water based damage
#define DAMAGE_HIT_SLASH	0x0400	// sword
#define DAMAGE_HIT_PIERCE	0x0800	// spear.
#define DAMAGE_NOREVEAL		0x4000	// Attacker is not revealed for this
#define DAMAGE_NOUNPARALYZE	0x8000  // victim won't be unparalyzed
#define DAMAGE_FIXED		0x10000	// already fixed damage, don't do calcs ... only create blood, anim, sounds... and update memories and attacker

typedef DWORD DAMAGE_TYPE;		// describe a type of damage.

///////////////////////////////////////

struct CValueRangeDef
{
	// Simple linearity
public:
	INT64 m_iLo;
	INT64 m_iHi;
public:
	void Init()
	{
		m_iLo = LLONG_MIN;
		m_iHi = LLONG_MIN;
	}
	int GetRange() const
	{
		return( static_cast<int>(m_iHi - m_iLo) );
	}
	int GetLinear( int iPercent ) const
	{	
		// ARGS: iPercent = 0-1000
		return( static_cast<int>(m_iLo) + IMULDIV( GetRange(), iPercent, 1000 ));
	}
	int GetRandom() const
	{	
		return( static_cast<int>(m_iLo) + Calc_GetRandVal( GetRange()));
	}
	int GetRandomLinear( int iPercent ) const;
	bool Load( TCHAR * pszDef );
	const TCHAR * Write() const;

public:
	CValueRangeDef()
	{
		Init();
	}
};

struct CValueCurveDef
{
	// Describe an arbitrary curve.
	// for a range from 0.0 to 100.0 (1000)
	// May be a list of probabilties from 0 skill to 100.0% skill.
public:
	CGTypedArray<int,int> m_aiValues;		// 0 to 100.0 skill levels

public:
	void Init()
	{
		m_aiValues.Empty();
	}
	bool Load( TCHAR * pszDef );
	const TCHAR * Write() const;
	int GetLinear( int iSkillPercent ) const;
	int GetChancePercent( int iSkillPercent ) const;
	int GetRandom() const;
	int GetRandomLinear( int iPercent ) const;

public:
	CValueCurveDef() { };

private:
	CValueCurveDef(const CValueCurveDef& copy);
	CValueCurveDef& operator=(const CValueCurveDef& other);
};

class CCharRefArray
{
private:
	// List of Players and NPC's involved in the quest/party/account etc..
	CGTypedArray< CGrayUID, CGrayUID> m_uidCharArray;

public:
	static const char *m_sClassName;
	size_t FindChar( const CChar * pChar ) const;
	bool IsCharIn( const CChar * pChar ) const
	{
		return( FindChar( pChar ) != m_uidCharArray.BadIndex() );
	}
	size_t AttachChar( const CChar * pChar );
	size_t InsertChar( const CChar * pChar, size_t i );
	void DetachChar( size_t i );
	size_t DetachChar( const CChar * pChar );
	void DeleteChars();
	size_t GetCharCount() const
	{
		return( m_uidCharArray.GetCount());
	}
	CGrayUID GetChar( size_t i ) const
	{
		return( m_uidCharArray[i] );
	}
	bool IsValidIndex( size_t i ) const
	{
		return m_uidCharArray.IsValidIndex( i );
	}
	inline size_t BadIndex() const
	{
		return m_uidCharArray.BadIndex();
	}
	void WritePartyChars( CScript & s );

public:
	CCharRefArray() { };

private:
	CCharRefArray(const CCharRefArray& copy);
	CCharRefArray& operator=(const CCharRefArray& other);
};

enum RRTRIG_TYPE
{
	// XTRIG_UNKNOWN	= some named trigger not on this list.
	RRTRIG_RESOURCEFOUND=1,
	RRTRIG_RESOURCEGATHER,
	RRTRIG_RESOURCETEST,
	RRTRIG_QTY
};

class CRegionResourceDef : public CResourceLink
{
	// RES_REGIONRESOURCE
	// When mining/lumberjacking etc. What can we get?
public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szTrigName[RRTRIG_QTY+1];

	// What item do we get when we try to mine this.
	ITEMID_TYPE m_ReapItem;	// ITEMID_ORE_1 most likely
	CValueCurveDef m_ReapAmount;	// How much can we reap at one time (based on skill)

	CValueCurveDef m_Amount;		// How is here total
	CValueCurveDef m_Skill;			// Skill levels required to mine this.
	CValueCurveDef m_iRegenerateTime;			// TICK_PER_SEC once found how long to regen this type.

public:
	explicit CRegionResourceDef( RESOURCE_ID rid );
	virtual ~CRegionResourceDef();

private:
	CRegionResourceDef(const CRegionResourceDef& copy);
	CRegionResourceDef& operator=(const CRegionResourceDef& other);

public:
	bool r_LoadVal( CScript & s );
	bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc = NULL );
	TRIGRET_TYPE OnTrigger( LPCTSTR pszTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs );
};

enum WEBPAGE_TYPE
{
	WEBPAGE_TEMPLATE,
	WEBPAGE_TEXT,
	WEBPAGE_BMP,
	WEBPAGE_GIF,
	WEBPAGE_JPG,
	WEBPAGE_QTY
};

enum WTRIG_TYPE
{
	// XTRIG_UNKNOWN	= some named trigger not on this list.
	WTRIG_Load=1,
	WTRIG_QTY
};

class CWebPageDef : public CResourceLink
{
	// RES_WEBPAGE

	// This is a single web page we are generating or serving.
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szPageType[];
	static LPCTSTR const sm_szPageExt[];
private:
	WEBPAGE_TYPE m_type;				// What basic format of file is this ? 0=text
	CGString m_sSrcFilePath;	// source template for the generated web page.
private:
	PLEVEL_TYPE m_privlevel;	// What priv level to see this page ?

	// For files that are being translated and updated.
	CGString m_sDstFilePath;	// where is the page served from ?
	int  m_iUpdatePeriod;	// How often to update the web page. 0 = never.
	int  m_iUpdateLog;		// create a daily log of the page.
	CServTime  m_timeNextUpdate;

public:
	static const char *m_sClassName;
	static int sm_iListIndex;
	static LPCTSTR const sm_szTrigName[WTRIG_QTY+1];
private:
	int ServPageRequest( CClient * pClient, LPCTSTR pszURLArgs, CGTime * pdateLastMod );
public:
	LPCTSTR GetName() const
	{
		return( m_sSrcFilePath );
	}
	LPCTSTR GetDstName() const
	{
		return( m_sDstFilePath );
	}
	bool IsMatch( LPCTSTR IsMatchPage ) const;

	bool SetSourceFile( LPCTSTR pszName, CClient * pClient );
	bool ServPagePost( CClient * pClient, LPCTSTR pszURLArgs, TCHAR * pPostData, int iContentLength );

	virtual bool r_LoadVal( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc = NULL );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc );	// some command on this object as a target

	void WebPageLog();
	bool WebPageUpdate( bool fNow, LPCTSTR pszDstName, CTextConsole * pSrc );

	static bool ServPage( CClient * pClient, TCHAR * pszPage, CGTime * pdateLastMod );

public:
	explicit CWebPageDef( RESOURCE_ID id );
	virtual ~CWebPageDef() { };

private:
	CWebPageDef(const CWebPageDef& copy);
	CWebPageDef& operator=(const CWebPageDef& other);
};

enum SPTRIG_TYPE
{
	SPTRIG_EFFECT	= 1,
	SPTRIG_FAIL,
	SPTRIG_SELECT,
	SPTRIG_START,
	SPTRIG_SUCCESS,
	SPTRIG_TARGETCANCEL,
	SPTRIG_QTY
};

class CSpellDef : public CResourceLink	// 1 based spells. See SPELL_*
{
	// RES_SPELL
private:
	DWORD	m_dwFlags;
	DWORD	m_dwGroup;

#define SPELLFLAG_DIR_ANIM			0x0000001 // Evoke type cast or directed. (animation)
#define SPELLFLAG_TARG_ITEM			0x0000002 // Need to target an object
#define SPELLFLAG_TARG_CHAR			0x0000004 // Needs to target a living thing
#define SPELLFLAG_TARG_OBJ			(SPELLFLAG_TARG_ITEM|SPELLFLAG_TARG_CHAR)
#define SPELLFLAG_TARG_XYZ			0x0000008 // Can just target a location.

#define SPELLFLAG_HARM				0x0000010 // The spell is in some way harmfull.
#define SPELLFLAG_FX_BOLT			0x0000020 // Effect is a bolt to the target.
#define SPELLFLAG_FX_TARG			0x0000040 // Effect is at the target.
#define SPELLFLAG_FIELD				0x0000080 // create a field of stuff. (fire,poison,wall)
#define SPELLFLAG_SUMMON			0x0000100 // summon a creature.
#define SPELLFLAG_GOOD				0x0000200 // The spell is a good spell. u intend to help to receiver.
#define SPELLFLAG_RESIST			0x0000400 // Allowed to resist this.	
#define SPELLFLAG_TARG_NOSELF		0x0000800
#define SPELLFLAG_DISABLED			0x0008000
#define SPELLFLAG_SCRIPTED			0x0010000
#define	SPELLFLAG_PLAYERONLY		0x0020000 // casted by players only
#define	SPELLFLAG_NOUNPARALYZE		0x0040000 // casted by players only
#define SPELLFLAG_NO_CASTANIM		0x0080000 // play no anim while casting (also override SPELLFLAG_DIR_ANIM)
#define SPELLFLAG_TARG_NO_PLAYER	0x0100000 // if a char may be targetted, it may not be a player
#define SPELLFLAG_TARG_NO_NPC		0x0200000 // if a char may be targetted, it may not be an NPC
#define SPELLFLAG_NOPRECAST			0x0400000 // disable precasting for this spell
#define SPELLFLAG_NOFREEZEONCAST	0x0800000 // disable freeze on cast for this spell
#define SPELLFLAG_AREA				0x1000000 // area effect (uses local.arearadius)
#define SPELLFLAG_POLY				0x2000000
#define SPELLFLAG_TARG_DEAD			0x4000000 // allowed to targ dead chars
#define SPELLFLAG_DAMAGE			0x8000000 // damage intended
#define SPELLFLAG_BLESS				0x10000000	//Benefitial spells like Bless,Agility,etc.
#define SPELLFLAG_CURSE				0x20000000	//Curses just like Weaken,Purge Magic,Curse,etc.
#define SPELLFLAG_HEAL				0x40000000	// Healing spell

	CGString m_sName;	// spell name

public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szTrigName[SPTRIG_QTY+1];
	static LPCTSTR const sm_szLoadKeys[];
	CGString m_sTargetPrompt;	// targetting prompt. (if needed)
	SOUND_TYPE m_sound;			// What noise does it make when done.
	CGString m_sRunes;			// Letter Runes for Words of power.
	CResourceQtyArray m_Reags;	// What reagents does it take ?
	CResourceQtyArray m_SkillReq;	// What skills/unused reagents does it need to cast.
	ITEMID_TYPE m_idSpell;		// The rune graphic for this.
	ITEMID_TYPE m_idScroll;		// The scroll graphic item for this.
	ITEMID_TYPE m_idEffect;		// Animation effect ID
	WORD m_wManaUse;			// How much mana does it need.
	LAYER_TYPE m_idLayer;		// Where the layer buff/debuff/data is stored.

	CValueCurveDef	m_CastTime;		// In TICK_PER_SEC.
	CValueCurveDef	m_Effect;		// Damage or effect level based on skill of caster.100% magery
	CValueCurveDef	m_Duration;		// length of effect. in TICK_PER_SEC
	CValueCurveDef	m_Interrupt;	// chance to interrupt a spell
	
public:
	bool IsSpellType( DWORD wFlags ) const
	{
		return(( m_dwFlags & wFlags ) ? true : false );
	}

public:
	explicit CSpellDef( SPELL_TYPE id );
	virtual ~CSpellDef()
	{
	}

private:
	CSpellDef(const CSpellDef& copy);
	CSpellDef& operator=(const CSpellDef& other);

public:
	LPCTSTR GetName() const { return( m_sName ); }
	bool r_LoadVal( CScript & s );
	bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );

	bool GetPrimarySkill( int * piSkill = NULL, int * piQty = NULL ) const;
};

class CRandGroupDef	: public CResourceLink // A spawn group.
{
	// RES_SPAWN or RES_REGIONTYPE
private:
	static LPCTSTR const sm_szLoadKeys[];
	int m_iTotalWeight;
	CResourceQtyArray m_Members;
private:
	int CalcTotalWeight();
public:
	static const char *m_sClassName;
	explicit CRandGroupDef( RESOURCE_ID rid ) :
		CResourceLink( rid )
	{
		m_iTotalWeight = 0;
	}
	virtual ~CRandGroupDef()
	{
	}

private:
	CRandGroupDef(const CRandGroupDef& copy);
	CRandGroupDef& operator=(const CRandGroupDef& other);

public:
	virtual bool r_LoadVal( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pKey, CGString &sVal, CTextConsole * pSrc = NULL );
	size_t GetRandMemberIndex( CChar * pCharSrc = NULL, bool bTrigger = true ) const;
	CResourceQty GetMember( size_t i ) const
	{
		return m_Members[i];
	}
	RESOURCE_ID GetMemberID( size_t i ) const
	{
		return m_Members[i].GetResourceID();
	}
	size_t BadMemberIndex() const
	{
		return m_Members.BadIndex();
	}
};

enum STAT_TYPE	// Character stats
{
	STAT_NONE = -1,
	STAT_STR = 0,
	STAT_INT,
	STAT_DEX,
	STAT_BASE_QTY,
	STAT_FOOD = 3,	// just used as a regen rate. (as karma does not decay)

	// Notoriety.
	STAT_KARMA = 4,		// g_Cfg.m_iMinKarma to g_Cfg.m_iMaxKarma - also used as the food consumption main timer.
	STAT_FAME,			// 0 to g_Cfg.m_iMaxFame
	STAT_QTY
};

class CSkillClassDef : public CResourceLink // For skill def table
{
	// Similar to character class.
	// RES_SKILLCLASS
	static LPCTSTR const sm_szLoadKeys[];
public:
	static const char *m_sClassName;
	CGString m_sName;	// The name of this skill class.

	WORD m_StatSumMax;
	DWORD m_SkillSumMax;

	WORD m_StatMax[STAT_BASE_QTY];	// STAT_BASE_QTY
	WORD m_SkillLevelMax[ SKILL_QTY ];

private:
	void Init();
public:
	explicit CSkillClassDef( RESOURCE_ID rid ) :
		CResourceLink( rid )
	{
		// If there was none defined in scripts.
		Init();
	}
	virtual ~CSkillClassDef()
	{
	}

private:
	CSkillClassDef(const CSkillClassDef& copy);
	CSkillClassDef& operator=(const CSkillClassDef& other);

public:
	LPCTSTR GetName() const { return( m_sName ); }

	bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	bool r_LoadVal( CScript & s );
};

enum SKTRIG_TYPE
{
	// All skills may be scripted.
	// XTRIG_UNKNOWN	= some named trigger not on this list.
	SKTRIG_ABORT=1,		// Some odd thing went wrong.
	SKTRIG_FAIL,		// we failed the skill check.
	SKTRIG_GAIN,		// called when there is a chance to gain skill
	SKTRIG_PRESTART,	// called before any hardcoded messages
	SKTRIG_SELECT,		// just selecting params for the skill
	SKTRIG_START,		// params for skill are done. (stroke)
	SKTRIG_STROKE,		// Not really a trigger! Just a stage.
	SKTRIG_SUCCESS,		// we passed the skill check
	SKTRIG_TARGETCANCEL,// called when a target cursor is cancelled
	SKTRIG_USEQUICK,	// called when a 'quick' usage of the skill is made
	SKTRIG_WAIT,		// called when a test is made to see if the character must wait before starting
	SKTRIG_QTY
};


enum SKF_TYPE
{
	SKF_SCRIPTED		= 0x0001,		// fully scripted, no hardcoded behaviour
	SKF_FIGHT			= 0x0002,		// considered a fight skill, maintains fight active
	SKF_MAGIC			= 0x0004,		// considered a magic skill
	SKF_CRAFT			= 0x0008,		// considered a crafting skill, compatible with MAKEITEM function
	SKF_IMMOBILE		= 0x0010,		// fails skill if character moves
	SKF_SELECTABLE		= 0x0020,		// allows skill to be selected from the skill menu
	SKF_NOMINDIST		= 0x0040,		// you can mine, fish, chop, hack on the same point you are standing on
	SKF_NOANIM			= 0x0080,		// prevents hardcoded animation from playing
	SKF_NOSFX			= 0x0100,		// prevents hardcoded sound from playing
	SKF_RANGED			= 0x0200,		// Considered a ranged skill (combine with SKF_FIGHT)
	SKF_GATHER			= 0x0400		// Considered a gathering skill, using SkillStrokes as SKF_CRAFT
};

struct CSkillDef : public CResourceLink // For skill def table
{
	// RES_SKILL
	static LPCTSTR const sm_szTrigName[SKTRIG_QTY+1];
	static LPCTSTR const sm_szLoadKeys[];
private:
	CGString m_sKey;	// script key word for skill.
public:
	CGString m_sTitle;	// title one gets if it's your specialty.
	CGString m_sName;	// fancy skill name
	CGString m_sTargetPrompt;	// targetting prompt. (if needed)

	CValueCurveDef m_Delay;	//	The base delay for the skill. (tenth of seconds)
	CValueCurveDef m_Effect;	// depends on skill

	// Stat effects.
	// You will tend toward these stat vals if you use this skill a lot.
	BYTE m_Stat[STAT_BASE_QTY];	// STAT_STR, STAT_INT, STAT_DEX
	BYTE m_StatPercent; // BONUS_STATS = % of success depending on stats
	BYTE m_StatBonus[STAT_BASE_QTY]; // % of each stat toward success at skill, total 100

	CValueCurveDef	m_AdvRate;		// ADV_RATE defined "skill curve" 0 to 100 skill levels.
	CValueCurveDef	m_Values;	// VALUES= influence for items made with 0 to 100 skill levels.
	int			m_GainRadius; // GAINRADIUS= max. amount of skill above the necessary skill for a task to gain from it
	int			m_Range;

	DWORD			m_dwFlags;
	DWORD			m_dwGroup;
	
	// Delay before skill complete. modified by skill level of course !
public:
	explicit CSkillDef( SKILL_TYPE iSkill );
	virtual ~CSkillDef() { };

private:
	CSkillDef(const CSkillDef& copy);
	CSkillDef& operator=(const CSkillDef& other);

public:
	LPCTSTR GetKey() const
	{
		return( m_sKey );
	}

	LPCTSTR GetName() const { return( GetKey()); }
	bool r_LoadVal( CScript & s );
	bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
};

class CSkillKeySortArray : public CGObSortArray< CValStr*, LPCTSTR >
{
public:
	int CompareKey( LPCTSTR pszKey, CValStr * pVal, bool fNoSpaces ) const
	{
		UNREFERENCED_PARAMETER(fNoSpaces);
		ASSERT( pszKey );
		ASSERT( pVal->m_pszName );
		return( strcmpi( pszKey, pVal->m_pszName ));
	}

public:
	CSkillKeySortArray() { };

private:
	CSkillKeySortArray(const CSkillKeySortArray& copy);
	CSkillKeySortArray& operator=(const CSkillKeySortArray& other);
};

struct CMultiDefArray : public CGObSortArray< CGrayMulti*, MULTI_TYPE >
{
	// store the static components of a IT_MULTI
	// Sorted array
	int CompareKey( MULTI_TYPE id, CGrayMulti* pBase, bool fNoSpaces ) const
	{
		UNREFERENCED_PARAMETER(fNoSpaces);
		ASSERT( pBase );
		return( id - pBase->GetMultiID());
	}
};

extern class CResource : public CResourceBase
{
	// Script defined resources (not saved in world file)
	static const CAssocReg sm_szLoadKeys[];

public:
	static const char *m_sClassName;
	CServTime m_timePeriodic;	// When to perform the next periodic update

	// Begin INI file options.
	bool m_fUseNTService;
	int	 m_fUseHTTP;
	bool m_fUseAuthID;
	int  m_iMapCacheTime;		// Time in sec to keep unused map data.
	int	 m_iSectorSleepMask;	// The mask for how long sectors will sleep.
	bool m_fUseMapDiffs;			// Whether or not to use map diff files.

	CGString m_sWorldBaseDir;	// "e:\graysvr\worldsave\" = world files go here.
	CGString m_sAcctBaseDir;	// Where do the account files go/come from ?

	bool m_fSecure;				// Secure mode. (will trap exceptions)
	int  m_iFreezeRestartTime;	// # seconds before restarting.
#define DEBUGF_NPC_EMOTE		0x0001
#define DEBUGF_ADVANCE_STATS	0x0002
#define DEBUGF_WALKCODES		0x0080	// try the new walk code checking stuff.
#define DEBUGF_EXP				0x0200	// experience gain/loss
#define DEBUGF_LEVEL			0x0400	// experience level changes
#define	DEBUGF_SCRIPTS			0x0800	// debug flags for scripts
#define DEBUGF_LOS				0x1000	// debug flags for AdvancedLOS
#define DEBUGF_WALK				0x2000	// debug flags for Walking stuff
#define DEBUGF_PACKETS			0x4000	// log packets to file
#define DEBUGF_NETWORK			0x8000	// debug flags for networking
	WORD m_wDebugFlags;			// DEBUG In game effects to turn on and off.

	// Decay
	int  m_iDecay_Item;			// Base decay time in minutes.
	int  m_iDecay_CorpsePlayer;	// Time for a playercorpse to decay (mins).
	int  m_iDecay_CorpseNPC;	// Time for a NPC corpse to decay.

	// Save
	int  m_iSaveNPCSkills;		// Only save NPC skills above this
	int  m_iSavePeriod;			// Minutes between saves.
	int  m_iSaveBackupLevels;	// How many backup levels.
	int  m_iSaveBackgroundTime;	// Speed of the background save in minutes.
	bool m_fSaveGarbageCollect;	// Always force a full garbage collection.

	// Account
	int  m_iDeadSocketTime;
	int	 m_iArriveDepartMsg;		// General switch to turn on/off arrival/depart messages.
	unsigned int  m_iClientsMax;	// Maximum (FD_SETSIZE) open connections to server
	int  m_iClientsMaxIP;			// Maximum (FD_SETSIZE) open connections to server per IP
	int  m_iConnectingMax;			// max clients connecting
	int  m_iConnectingMaxIP;		// max clients connecting

	int  m_iGuestsMax;				// Allow guests who have no accounts ?
	int  m_iClientLingerTime;		// How long logged out clients linger in seconds.
	int  m_iMinCharDeleteTime;		// How old must a char be ? (minutes)
	BYTE  m_iMaxCharsPerAccount;	// Maximum characters allowed on an account.
	bool m_fLocalIPAdmin;			// The local ip is the admin ?
	bool m_fMd5Passwords;			// Should MD5 hashed passwords be used?

	// Magic
	bool m_fReagentsRequired;
	int m_iWordsOfPowerColor;
	int m_iWordsOfPowerFont;
	bool m_fWordsOfPowerPlayer; // Words of Power for players
	bool m_fWordsOfPowerStaff;	// Words of Power for staff
	bool m_fEquippedCast;		// Allow casting while equipped.
	bool m_fReagentLossFail;	// ??? Lose reags when failed.
	int m_iMagicUnlockDoor;  // 1 in N chance of magic unlock working on doors -- 0 means never
	ITEMID_TYPE m_iSpell_Teleport_Effect_NPC;
	SOUND_TYPE m_iSpell_Teleport_Sound_NPC;
	ITEMID_TYPE m_iSpell_Teleport_Effect_Players;
	SOUND_TYPE m_iSpell_Teleport_Sound_Players;
	ITEMID_TYPE m_iSpell_Teleport_Effect_Staff;
	SOUND_TYPE m_iSpell_Teleport_Sound_Staff;
	int m_iSpellTimeout; // Timeout for spell targeting

	// In Game Effects
	int	 m_iLightDungeon;
	int  m_iLightDay;		// Outdoor light level.
	int  m_iLightNight;		// Outdoor light level.
	int  m_iGameMinuteLength;	// Length of the game world minute in real world (TICK_PER_SEC) seconds.
	bool m_fNoWeather;			// Turn off all weather.
	bool m_fCharTags;			// Show [NPC] tags over chars.
	bool m_fVendorTradeTitle;	// Show job title on vendor names.
	bool m_fFlipDroppedItems;	// Flip dropped items.
	bool m_fCanUndressPets;		// Can players undress their pets?
	bool m_fMonsterFight;		// Will creatures fight amoung themselves.
	bool m_fMonsterFear;		// will they run away if hurt ?
	int	 m_iBankIMax;			// Maximum number of items allowed in bank.
	int  m_iBankWMax;			// Maximum weight in WEIGHT_UNITS stones allowed in bank.
	int  m_iVendorMaxSell;		// Max things a vendor will sell in one shot.
	unsigned int  m_iMaxCharComplexity;		// How many chars per sector.
	unsigned int  m_iMaxItemComplexity;		// How many items per meter.
	unsigned int  m_iMaxSectorComplexity;	// How many items per sector.
	bool m_fGenericSounds;		// Do players receive generic (not them-devoted) sounds
	bool m_fAutoNewbieKeys;		// Are house and boat keys newbied automatically?
	int  m_iStamRunningPenalty;		// Weight penalty for running (+N% of max carry weight)
	int  m_iStaminaLossAtWeight;	// %Weight at which characters begin to lose stamina
	int  m_iHitpointPercentOnRez;	// How many hitpoints do they get when they are rez'd?
	int  m_iHitsHungerLoss;		// How many % of HP will loose char on starving
	int  m_iMaxBaseSkill;		// Maximum value for base skills at char creation
	bool m_fInitHiddenSkills;	// Hidden skills will be initialised at char creation
	int  m_iTrainSkillPercent;	// How much can NPC's train up to ?
	int	 m_iTrainSkillMax;
	int  m_fDeadCannotSeeLiving;
	int  m_iMediumCanHearGhosts;	// At this Spirit Speak skill level players can understand ghosts speech instead hear 'oOOoO ooO'
#ifdef _ALPHASPHERE
	int m_iTrainSkillCost;
#endif
	int	 m_iMountHeight;		// The height at which a mounted person clears ceilings.
	int	 m_iMoveRate;			// The percent rate of NPC movement
	int  m_iArcheryMaxDist;
	int  m_iArcheryMinDist;
	int  m_iHitsUpdateRate;		// how often send my hits updates to visible clients
	int  m_iSpeedScaleFactor;	// fight skill delay = m_iSpeedScaleFactor / ( (dex + 100) * Weapon Speed )
	int  m_iSkillPracticeMax;	// max skill level a player can practice on dummies/targets upto
	bool m_iPacketDeathAnimation;	// packet 02c

	// Flags for controlling pvp/pvm behaviour of players
	int  m_iCombatFlags;		// combat flags
	int  m_iMagicFlags;			// magic flags

	// Criminal/Karma
	bool m_fAttackingIsACrime;		// Is attacking (even before hitting) a crime?
	bool m_fGuardsInstantKill;	// Will guards kill instantly or follow normal combat rules?
	bool m_fGuardsOnMurderers;	// should guards be only called on criminals ?
	int	 m_iGuardLingerTime;	// How long do guards linger about.
	int  m_iSnoopCriminal;		// 1 in # chance of getting criminalflagged when succesfully snooping.
	bool m_iTradeWindowSnooping;// 1 means opening a container in trade window needs to use snooping, 0 direct open.
	int  m_iMurderMinCount;		// amount of murders before we get title.
	int	 m_iMurderDecayTime;	// (minutes) Roll murder counts off this often.
	bool m_fHelpingCriminalsIsACrime;// If I help (rez, heal, etc) a criminal, do I become one too?
	bool m_fLootingIsACrime;	// Looting a blue corpse is bad.
	int  m_iCriminalTimer;		// How many minutes are criminals flagged for?
	int	 m_iPlayerKarmaNeutral;	// How much bad karma makes a player neutral?
	int	 m_iPlayerKarmaEvil;	// How much bad karma makes a player evil?
	int m_iMinKarma;			// Minimum karma level
	int m_iMaxKarma;			// Maximum karma level
	int m_iMaxFame;				// Maximum fame level

	// other
	bool	m_fNoResRobe;
	int		m_iLostNPCTeleport;
	int		m_iExperimental;
	int		m_iOptionFlags;
	int		m_iWoolGrowthTime;	// how long till wool grows back on sheared sheep, in minutes
	unsigned int m_iAttackerTimeout;	// Timeout for attacker.*
	unsigned int m_iNotoTimeout;	// Timeout for NOTOriety checks.*

	int		m_iDistanceYell;
	int		m_iDistanceWhisper;
	int		m_iDistanceTalk;

	unsigned int m_iMaxSkill;

	CGString	m_sSpeechSelf;
	CGString	m_sSpeechPet;
	CGString	m_sSpeechOther;
	CGString	m_sCommandTrigger;

#ifdef _DUMPSUPPORT
	CGString	m_sDumpAccPackets;
#endif

	CGString	m_sEventsPet;
	CResourceRefArray m_pEventsPetLink;

	CGString	m_sEventsPlayer;
	CResourceRefArray m_pEventsPlayerLink;

	CGString	m_sEventsRegion;
	CResourceRefArray m_pEventsRegionLink;

	CGString	m_sEventsItem;
	CResourceRefArray m_iEventsItemLink;

	// Third Party Tools
	CGString	m_sStripPath;
	bool	m_fCUOStatus;
	bool	m_fUOGStatus;

	int		m_iWalkBuffer;
	int		m_iWalkRegen;

	int		m_iCommandLog;
	bool	m_fTelnetLog;
	
	bool 		m_fUsecrypt;
	bool 		m_fUsenocrypt;

	bool		m_fPayFromPackOnly;	// Pay only from main pack?
	int		m_iOverSkillMultiply;	// multiplyer to get over skillclass
	bool	m_fSuppressCapitals;	// Enable/Disable capital letters suppression

#define ADVANCEDLOS_DISABLED		0x00
#define	ADVANCEDLOS_PLAYER			0x01
#define	ADVANCEDLOS_NPC				0x02
	int		m_iAdvancedLos;		// AdvancedLOS
	int		m_iWrapX;

	// New ones
	int		m_iFeatureT2A;
	int		m_iFeatureLBR;
	int		m_iFeatureAOS;
	int		m_iFeatureSE;
	int		m_iFeatureML;
	int		m_iFeatureKR;
	int		m_iFeatureSA;
	int		m_iFeatureExtra;
	
	int		m_iMaxLoopTimes;
#define	STAT_FLAG_NORMAL	0x00	//	MAX* status allowed (default)
#define STAT_FLAG_DENYMAX	0x01	//	MAX* denied
#define STAT_FLAG_DENYMAXP	0x02	//		.. for players
#define STAT_FLAG_DENYMAXN	0x04	//		.. for npcs
	int		m_iStatFlag;

#define NPC_AI_PATH				0x00001		//	NPC pathfinding
#define	NPC_AI_FOOD				0x00002		//	NPC food search (objects + grass)
#define	NPC_AI_EXTRA			0x00004		//	NPC magics, combat, etc
#define NPC_AI_ALWAYSINT		0x00008		//	NPC pathfinding does not check int, always smart
#define NPC_AI_INTFOOD			0x00010		//  NPC food search (more intelligent and trusworthy)
#define NPC_AI_MAGICS			0x00020
#define NPC_AI_COMBAT			0x00040
#define NPC_AI_VEND_TIME		0x00080
#define NPC_AI_LOOTING			0x00100
#define	NPC_AI_MOVEOBSTACLES	0x00200
#define NPC_AI_PERSISTENTPATH	0x00400		// 
#define NPC_AI_THREAT			0x00800
	int		m_iNpcAi;

	//	Experience system
	bool	m_bExperienceSystem;
#define EXP_MODE_RAISE_COMBAT	0x0001
#define	EXP_MODE_RAISE_CRAFT	0x0002
#define	EXP_MODE_ALLOW_DOWN		0x0004
#define	EXP_MODE_DOWN_NOLEVEL	0x0008
#define	EXP_MODE_AUTOSET_EXP	0x0010
	int		m_iExperienceMode;
	int		m_iExperienceKoefPVM;
	int		m_iExperienceKoefPVP;
	bool	m_bLevelSystem;
#define LEVEL_MODE_LINEAR		0
#define	LEVEL_MODE_DOUBLE		1
	int		m_iLevelMode;
	unsigned int m_iLevelNextAt;

	int		m_iAutoResDisp;
	int		m_iAutoPrivFlags;

	char	m_cCommandPrefix;

	int		m_iDefaultCommandLevel;

	//	color noto flag
	HUE_TYPE	m_iColorNotoGood;
	HUE_TYPE	m_iColorNotoGuildSame;
	HUE_TYPE	m_iColorNotoNeutral;
	HUE_TYPE	m_iColorNotoCriminal;
	HUE_TYPE	m_iColorNotoGuildWar;
	HUE_TYPE	m_iColorNotoEvil;
	HUE_TYPE	m_iColorNotoInvul;
	HUE_TYPE	m_iColorNotoInvulGameMaster;
	HUE_TYPE	m_iColorNotoDefault;
	
	HUE_TYPE	m_iColorInvis;
	HUE_TYPE	m_iColorInvisSpell;
	HUE_TYPE	m_iColorHidden;

	// notoriety inheritance
	int     m_iPetsInheritNotoriety;

	int		m_iMaxAccountLoginTries;
	int		m_iMaxShipPlankTeleport;

#ifndef _DBPLUGIN
	//	mySQL features
	bool		m_bMySql;
	CGString	m_sMySqlHost;
	CGString	m_sMySqlUser;
	CGString	m_sMySqlPass;
	CGString	m_sMySqlDB;
#else
	CGString	m_sDbDll;
	CGString	m_sDbHost;
	CGString	m_sDbUser;
	CGString	m_sDbPass;
	CGString	m_sDbDatabase;
	int			m_iDbQueryBuffer;
#endif

	// network settings
#ifdef _MTNETWORK
	unsigned int m_iNetworkThreads;			// number of network threads to create
	unsigned int m_iNetworkThreadPriority;	// priority of network threads
#endif
	int			m_fUseAsyncNetwork;			// 0=normal send, 1=async send, 2=async send for 4.0.0+ only
	int			m_iNetMaxPings;				// max pings before blocking an ip
	int			m_iNetHistoryTTL;			// time to remember an ip
	int			m_iNetMaxPacketsPerTick;	// max packets to send per tick (per queue)
	unsigned int m_iNetMaxLengthPerTick;		// max packet length to send per tick (per queue) (also max length of individual packets)
	int			m_iNetMaxQueueSize;			// max packets to hold per queue (comment out for unlimited)
	bool		m_fUsePacketPriorities;		// true to prioritise sending packets
	bool		m_fUseExtraBuffer;			// true to queue packet data in an extra buffer

	int			m_iTooltipCache;			// time to cache tooltip for
	int			m_iTooltipMode;				// tooltip mode (TOOLTIP_TYPE)
	int			m_iContextMenuLimit;		// max amount of options per context menu
#define AUTOTOOLTIP_FLAG_NAME          0x0001
#define AUTOTOOLTIP_FLAG_AMOUNT        0x0002
#define AUTOTOOLTIP_FLAG_WEIGHT        0x0004
#define AUTOTOOLTIP_FLAG_DURABILITY    0x0008
#define AUTOTOOLTIP_FLAG_POISON        0x0010
#define AUTOTOOLTIP_FLAG_WANDCHARGES   0x0020
#define AUTOTOOLTIP_FLAG_SPELLBOOK     0x0040
	int			m_iAutoTooltipResend;		// automatically resend tooltip

	int			m_iRegenRate[STAT_QTY];
	int			m_iTimerCall;
	bool		m_bAllowLightOverride;
	CGString	m_sZeroPoint;
	bool		m_bAllowBuySellAgent;

	bool		m_bAllowNewbTransfer;

	bool		m_NPCNoFameTitle;

	bool		m_bAgree;
	int			m_iMaxPolyStats;

	// End INI file options.
	
	CResourceScript m_scpIni;	// Keep this around so we can link to it.
	CResourceScript m_scpCryptIni; // Encryption keys are in here

public:
	CResourceScript m_scpTables;

	CStringSortArray m_ResourceList;	// Sections lists

	CStringSortArray m_Obscene;	// Bad Names/Words etc.
	CGObArray< CGString* > m_Fame;	// fame titles (fame.famous)
	CGObArray< CGString* > m_Karma;	// karma titles (karma.wicked)
	CGObArray< CGString* > m_Runes;	// Words of power. (A-Z)

	CGTypedArray< int, int > m_NotoKarmaLevels; // karma levels for noto titles
	CGTypedArray< int, int > m_NotoFameLevels; // fame levels for noto titles
	CGObArray< CGString* > m_NotoTitles;	// Noto titles.

	CMultiDefArray m_MultiDefs;	// read from the MUL files. Cached here on demand.

	CObNameSortArray m_SkillNameDefs;	// const CSkillDef* Name sorted
	CGPtrTypeArray< CSkillDef* > m_SkillIndexDefs;	// Defined Skills indexed by number
	CGObArray< CSpellDef* > m_SpellDefs;	// Defined Spells
	CGPtrTypeArray< CSpellDef* > m_SpellDefs_Sorted; // Defined Spells, in skill order

	CStringSortArray m_PrivCommands[PLEVEL_QTY];	// what command are allowed for a priv level?

public:
	CObNameSortArray m_Servers;		// Servers list. we act like the login server with this.
	CObNameSortArray m_Functions;	// subroutines that can be used in scripts.
	CRegionLinks m_RegionDefs;

	// static definition stuff from *TABLE.SCP mostly.
	CGObArray< const CStartLoc* > m_StartDefs; // Start points list
	CValueCurveDef m_StatAdv[STAT_BASE_QTY]; // "skill curve"
	CGTypedArray<CPointBase,CPointBase&> m_MoonGates;	// The array of moongates.

	CResourceHashArray m_WebPages;	// These can be linked back to the script.

private:
	RESOURCE_ID ResourceGetNewID( RES_TYPE restype, LPCTSTR pszName, CVarDefContNum ** ppVarNum, bool fNewStyleDef );

public:
	CResource();
	~CResource();
	
private:
	CResource(const CResource& copy);
	CResource& operator=(const CResource& other);

public:
	bool r_LoadVal( CScript &s );
	bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	void r_Write( CScript &s );

	bool LoadIni( bool fTest );
	bool LoadCryptIni( void );
	bool Load( bool fResync );
	void Unload( bool fResync );
	void OnTick( bool fNow );

	bool LoadResourceSection( CScript * pScript );
	void LoadSortSpells();
	CResourceDef * ResourceGetDef( RESOURCE_ID_BASE rid ) const;
	
	// Print EF/OF Flags
	
	void PrintEFOFFlags( bool bEF = true, bool bOF = true, CTextConsole *pSrc = NULL );

	// ResDisp Flag
	int GetPacketFlag( bool bCharlist, RESDISPLAY_VERSION res = RDS_T2A, unsigned char chars = 5 );

	// Specialized resource accessors.

	bool CanUsePrivVerb( const CScriptObj * pObjTarg, LPCTSTR pszCmd, CTextConsole * pSrc ) const;
	PLEVEL_TYPE GetPrivCommandLevel( LPCTSTR pszCmd ) const;

	static STAT_TYPE FindStatKey( LPCTSTR pszKey );
	static bool IsValidEmailAddressFormat( LPCTSTR pszText );
	bool IsObscene( LPCTSTR pszText ) const;

	CWebPageDef * FindWebPage( LPCTSTR pszPath ) const;

	CServerRef Server_GetDef( size_t index );

	const CSpellDef * GetSpellDef( SPELL_TYPE index ) const
	{
		// future: underlying type for SPELL_TYPE to avoid casts
		if (index == SPELL_NONE || m_SpellDefs.IsValidIndex(static_cast<size_t>(index)) == false)
			return NULL;
		return m_SpellDefs[static_cast<size_t>(index)];
	}

	CSpellDef * GetSpellDef( SPELL_TYPE index ) 
	{
		// future: underlying type for SPELL_TYPE to avoid casts
		if (index == SPELL_NONE || m_SpellDefs.IsValidIndex(static_cast<size_t>(index)) == false)
			return NULL;
		return m_SpellDefs[static_cast<size_t>(index)];
	}

	LPCTSTR GetSkillKey( SKILL_TYPE index ) const
	{
		// future: underlying type for SPELL_TYPE to avoid casts
		if (m_SkillIndexDefs.IsValidIndex(static_cast<size_t>(index)) == false)
			return NULL;
		return( m_SkillIndexDefs[static_cast<size_t>(index)]->GetKey());
	}

	bool IsSkillFlag( SKILL_TYPE index, SKF_TYPE skf ) const
	{
		const CSkillDef * pSkillDef = GetSkillDef( index );
		return ( pSkillDef != NULL && (pSkillDef->m_dwFlags & skf) );
	}

	bool IsSkillRanged( SKILL_TYPE index ) const
	{
		return IsSkillFlag( index, SKF_RANGED );
	}

	const CSkillDef* GetSkillDef( SKILL_TYPE index ) const
	{
		if (m_SkillIndexDefs.IsValidIndex(static_cast<size_t>(index)) == false)
			return NULL;
		return( m_SkillIndexDefs[static_cast<size_t>(index)] );
	}
	
	CSkillDef* GetSkillDef( SKILL_TYPE index )
	{
		if (m_SkillIndexDefs.IsValidIndex(static_cast<size_t>(index)) == false )
			return NULL;
		return( m_SkillIndexDefs[static_cast<size_t>(index)] );
	}

	const CSkillDef* FindSkillDef( LPCTSTR pszKey ) const
	{
		// Find the skill name in the alpha sorted list.
		// RETURN: SKILL_NONE = error.
		size_t i = m_SkillNameDefs.FindKey( pszKey );
		if ( i == m_SkillNameDefs.BadIndex() )
			return( NULL );
		return( STATIC_CAST <const CSkillDef*>(m_SkillNameDefs[i]));
	}
	const CSkillDef* SkillLookup( LPCTSTR pszKey );
	SKILL_TYPE FindSkillKey( LPCTSTR pszKey ) const;

	int GetSpellEffect( SPELL_TYPE spell, int iSkillval ) const;

	LPCTSTR GetRune( TCHAR ch ) const
	{
		size_t index = static_cast<size_t>(toupper(ch) - 'A');
		if ( ! m_Runes.IsValidIndex(index))
			return "?";
		return( m_Runes[index]->GetPtr() );
	}
	LPCTSTR GetNotoTitle( int iLevel, bool bFemale ) const;

	const CGrayMulti * GetMultiItemDefs( CItem * pItem );
	const CGrayMulti * GetMultiItemDefs( ITEMID_TYPE itemid );

	bool IsConsoleCmd( TCHAR ch ) const;

	CPointMap GetRegionPoint( LPCTSTR pCmd ) const; // Decode a teleport location number into X/Y/Z
	CRegionBase * GetRegion( LPCTSTR pKey ) const; // Find a region with the given name/defname

	int Calc_MaxCarryWeight( const CChar * pChar ) const;
	int Calc_DropStamWhileMoving( CChar * pChar, int iWeightLoadPercent );
	int Calc_WalkThroughChar( CChar * pCharMove, CChar * pCharObstacle );
	int Calc_CombatAttackSpeed( CChar * pChar, CItem * pWeapon );
	int Calc_CombatChanceToHit( CChar * pChar, SKILL_TYPE skill, CChar * pCharTarg, CItem * pWeapon );
	int  Calc_StealingItem( CChar * pCharThief, CItem * pItem, CChar * pCharMark );
	bool Calc_CrimeSeen( CChar * pCharThief, CChar * pCharViewer, SKILL_TYPE SkillToSee, bool fBonus );
	int Calc_FameKill( CChar * pKill );
	int Calc_KarmaKill( CChar * pKill, NOTO_TYPE NotoThem );
	int Calc_KarmaScale( int iKarma, int iKarmaChange );
	LPCTSTR Calc_MaptoSextant( CPointMap pntCoords );
	
#define SysMessageDefault( msg )	SysMessage( g_Cfg.GetDefaultMsg( msg ) )
	LPCTSTR GetDefaultMsg(LPCTSTR pszKey);
	LPCTSTR	GetDefaultMsg(long lKeyNum);

typedef std::map<DWORD,DWORD> KRGumpsMap;
	KRGumpsMap m_mapKRGumps;

	bool SetKRDialogMap(DWORD rid, DWORD idKRDialog);
	DWORD GetKRDialogMap(DWORD idKRDialog);
	DWORD GetKRDialog(DWORD rid);

	bool GenerateDefname(TCHAR *pObjectName, size_t iInputLength, LPCTSTR pPrefix, TCHAR *pOutput, bool bCheckConflict = true, CVarDefMap* vDefnames = NULL);
	bool DumpUnscriptedItems(CTextConsole * pSrc, LPCTSTR pszFilename);
} g_Cfg;



class CDialogDef : public CResourceLink
{
	static LPCTSTR const sm_szLoadKeys[];

public:
	static const char *m_sClassName;
	bool GumpSetup( int iPage, CClient * pClientSrc, CObjBase * pObj, LPCTSTR Arguments = "" );
	size_t GumpAddText( LPCTSTR pszText );		// add text to the text section, return insertion index
	bool r_Verb( CScript &s, CTextConsole * pSrc );
	bool r_LoadVal( CScript & s );
	bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );

public:
	explicit CDialogDef( RESOURCE_ID rid );
	virtual ~CDialogDef() { };

private:
	CDialogDef(const CDialogDef& copy);
	CDialogDef& operator=(const CDialogDef& other);

public:	
	// temporary placeholders - valid only during dialog setup
	CObjBase *	m_pObj;
	CGString	m_sControls[1024];
	CGString	m_sText[512];
	size_t		m_iTexts;
	size_t		m_iControls;
	int			m_x;
	int			m_y;

	int			m_iOriginX;	// keep track of position when parsing
	int			m_iOriginY;
	WORD		m_iPage;		// page to open the dialog in

	bool		m_bNoDispose;	// contains 'nodispose' control
};



class CItemTypeDef : public CResourceLink
{
public:
	static const char *m_sClassName;
	explicit CItemTypeDef( RESOURCE_ID rid ) : CResourceLink( rid )
	{	
	}

private:
	CItemTypeDef(const CItemTypeDef& copy);
	CItemTypeDef& operator=(const CItemTypeDef& other);

public:
	bool r_LoadVal( CScript & s );
	int GetItemType() const;
};


#define IsSetEF(ef)				((g_Cfg.m_iExperimental & ef) != 0)
#define IsSetOF(of)				((g_Cfg.m_iOptionFlags & of) != 0)
#define IsSetSpecific			((g_Cfg.m_iExperimental & EF_Specific) != 0 && (g_Cfg.m_iOptionFlags & OF_Specific) != 0)
#define IsSetCombatFlags(of)	((g_Cfg.m_iCombatFlags & of) != 0)
#define IsSetMagicFlags(of)		((g_Cfg.m_iMagicFlags & of) != 0)

#endif	// _INC_CRESOURCE_H
