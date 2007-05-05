//
// CResource.h
//


#ifndef _INC_CRESOURCE_H
#define _INC_CRESOURCE_H
#pragma once

#include "../common/CAssoc.h"

class CAccount;
class CClient;
class CLogIP;
class CServerDef;

typedef CServerDef * CServerRef;

#define MAX_SKILL	(g_Cfg.m_iMaxSkill)
#define MAX_SKILL_ML	55	// The number of skills viewable by ML clients
#define MAX_SKILL_SE	54	// The number of skills viewable by SE clients
#define MAX_SKILL_AOS	52	// The number of skills viewable by AOS clients
#define MAX_SKILL_LBR	49	// The number of skills viewable by LBR clients
#define MAX_SKILL_T2A	49	// The number of skills viewable by T2A clients

// option flags
enum OF_TYPE
{
	OF_Unused0001			= 0x0000001,
	OF_Unused0002			= 0x0000002,
	OF_Unused0004			= 0x0000004,
	OF_Command_Sysmsgs		= 0x0000008,
	OF_Unused0010			= 0x0000010,
	OF_OSIMultiSight		= 0x0000020,
	OF_Items_AutoName		= 0x0000040,
	OF_FileCommands			= 0x0000080,
	OF_NoItemNaming			= 0x0000100,
	OF_NoHouseMuteSpeech	= 0x0000200,
	OF_Unused0400			= 0x0000400,
	OF_Flood_Protection		= 0x0001000,
	OF_Buffs				= 0x0002000,
	OF_NoPrefix				= 0x0004000,
	OF_DyeType				= 0x0008000,
	OF_DrinkIsFood			= 0x0010000,
	OF_Specific				= 0x1000000
};

enum EF_TYPE
{
	EF_DiagonalWalkCheck		= 0x0000001,
	EF_UNICODE					= 0x0000002,
	EF_Unused04					= 0x0000004,
	EF_New_Triggers				= 0x0000008,
	EF_Unused010				= 0x0000010,
	EF_Intrinsic_Locals			= 0x0000020,
	EF_Item_Strict_Comparison	= 0x0000040,
	EF_NewPositionChecks		= 0x0000080,
	EF_WalkCheck				= 0x0000100,
	EF_AllowTelnetPacketFilter	= 0x0000200,
	EF_Script_Profiler			= 0x0000400,
	EF_Size_Optimise			= 0x0000800,
	EF_Minimize_Triggers		= 0x0001000,
	EF_DamageTools				= 0x0002000,
	EF_UsePingServer			= 0x0008000,
    EF_Specific					= 0x1000000,	// Specific behaviour, not completly tested
};

enum MAGICFLAGS_TYPE
{
	MAGICF_NODIRCHANGE		= 0x0000001,	// not rotate player when casting/targetting
	MAGICF_PRECAST			= 0x0000002,	// use precasting
	MAGICF_IGNOREAR			= 0x0000004,	// magic ignore ar
	MAGICF_CANHARMSELF		= 0x0000008,	// i can do damage on self
	MAGICF_STACKSTATS		= 0x0000010,	// allow multiple stat spells at once
	MAGICF_FREEZEONCAST		= 0x0000020,	// freeze on cast // TODO: implement it
};

enum COMBATFLAGS_TYPE
{
	COMBAT_NODIRCHANGE			= 0x0000001,	// not rotate player when fighting
	COMBAT_FACECOMBAT			= 0x0000002,	// allow faced combat only
	COMBAT_PREHIT				= 0x0000004,	// allow prehit for close combat. first hit is instant (delay 0.1sec)
	COMBAT_USE_RESISTANCE		= 0x0000008,	// use character props RES* against oldfashioned damage types
	COMBAT_SPECIALDAMAGE		= 0x0000010,	// use character tags TAG.*DAMAGE to apply additional damage
	COMBAT_DCLICKSELF_UNMOUNTS	= 0x0000020,	// unmount horse when dclicking self while in warmode
	COMBAT_ALLOWHITFROMSHIP		= 0x0000040,	// allow attacking opponents from ships
	COMBAT_OSIDAMAGEMOD			= 0x0000080,	// modify weapon damage OSI-style (taking anatomy, tactics, lumberjacking into account)
	COMBAT_ARCHERYCANMOVE		= 0x0000100,	// firing bow while moving
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

	BODYPART_QTY,
};

#define DAMAGE_GOD			0x0001	// Nothing can block this.
#define DAMAGE_HIT_BLUNT	0x0002	// Physical hit of some sort.
#define DAMAGE_MAGIC		0x0004	// Magic blast of some sort. (we can be immune to magic to some extent)
#define DAMAGE_POISON		0x0008	// Or biological of some sort ? (HARM spell)
#define DAMAGE_FIRE			0x0010	// Fire damage of course.  (Some creatures are immune to fire)
#define DAMAGE_ELECTRIC		0x0020	// lightning.
#define DAMAGE_ENERGY		0x0020	// lightning.
#define DAMAGE_GENERAL		0x0080	// All over damage. As apposed to hitting just one point.
#define DAMAGE_ACIDIC		0x0100	// damages armor
#define DAMAGE_COLD			0x0200	// cold or water based damage
#define DAMAGE_HIT_SLASH	0x0400	// sword
#define DAMAGE_HIT_PIERCE	0x0800	// spear.
#define DAMAGE_NOREVEAL		0x4000	// Attacker is not revealed for this
#define DAMAGE_NOUNPARALYZE	0x8000  // victim won't be unparalyzed

typedef WORD DAMAGE_TYPE;		// describe a type of damage.

///////////////////////////////////////

struct CValueRangeDef
{
	// Simple linearity
public:
	int m_iLo;
	int m_iHi;
public:
	void Init()
	{
		m_iLo = INT_MIN;
		m_iHi = INT_MIN;
	}
	int GetRange() const
	{
		return( m_iHi - m_iLo );
	}
	int GetLinear( int iPercent ) const
	{	
		// ARGS: iPercent = 0-1000
		return( m_iLo + IMULDIV( GetRange(), iPercent, 1000 ));
	}
	int GetRandom() const
	{	
		return( m_iLo + Calc_GetRandVal( GetRange()));
	}
	int GetRandomLinear( int iPercent ) const;
	bool Load( TCHAR * pszDef );
	const TCHAR * Write() const;
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
};

class CCharRefArray
{
private:
	// List of Players and NPC's involved in the quest/party/account etc..
	CGTypedArray< CGrayUID, CGrayUID> m_uidCharArray;
public:
	static const char *m_sClassName;
	int FindChar( const CChar * pChar ) const;
	bool IsCharIn( const CChar * pChar ) const
	{
		return( FindChar( pChar ) >= 0 );
	}
	int AttachChar( const CChar * pChar );
	void DetachChar( int i );
	int DetachChar( const CChar * pChar );
	void DeleteChars();
	int GetCharCount() const
	{
		return( m_uidCharArray.GetCount());
	}
	CGrayUID GetChar( int i ) const
	{
		return( m_uidCharArray[i] );
	}
	bool IsValidIndex( int i )
	{
		return m_uidCharArray.IsValidIndex( i );
	}
	void WritePartyChars( CScript & s );
};

class CRegionResourceDef : public CResourceLink
{
	// RES_REGIONRESOURCE
	// When mining/lumberjacking etc. What can we get?
public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szLoadKeys[];

	// What item do we get when we try to mine this.
	ITEMID_TYPE m_ReapItem;	// ITEMID_ORE_1 most likely
	CValueCurveDef m_ReapAmount;	// How much can we reap at one time (based on skill)

	CValueCurveDef m_Amount;		// How is here total
	CValueCurveDef m_Skill;			// Skill levels required to mine this.
	int m_iRegenerateTime;			// TICK_PER_SEC once found how long to regen this type.

public:
	CRegionResourceDef( RESOURCE_ID rid );
	virtual ~CRegionResourceDef();
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
	WEBPAGE_QTY,
};

enum WTRIG_TYPE
{
	// XTRIG_UNKNOWN	= some named trigger not on this list.
	WTRIG_Load=1,
	WTRIG_QTY,
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

	CWebPageDef( RESOURCE_ID id );
	virtual ~CWebPageDef()
	{
	}
};

enum SPTRIG_TYPE
{
	SPTRIG_EFFECT	= 1,
	SPTRIG_FAIL,
	SPTRIG_SELECT,
	SPTRIG_START,
	SPTRIG_SUCCESS,
	SPTRIG_QTY,
};

class CSpellDef : public CResourceLink	// 1 based spells. See SPELL_*
{
	// RES_SPELL
private:
	DWORD	m_dwFlags;
	DWORD	m_dwGroup;

#define SPELLFLAG_DIR_ANIM  0x00001	// Evoke type cast or directed. (animation)
#define SPELLFLAG_TARG_ITEM 0x00002	// Need to target an object
#define SPELLFLAG_TARG_CHAR 0x00004	// Needs to target a living thing
#define SPELLFLAG_TARG_OBJ	(SPELLFLAG_TARG_ITEM|SPELLFLAG_TARG_CHAR)

#define SPELLFLAG_TARG_XYZ  0x00008	// Can just target a location.
#define SPELLFLAG_HARM		0x00010	// The spell is in some way harmfull.
#define SPELLFLAG_FX_BOLT	0x00020	// Effect is a bolt to the target.
#define SPELLFLAG_FX_TARG	0x00040	// Effect is at the target.
#define SPELLFLAG_FIELD		0x00080	// create a field of stuff. (fire,poison,wall)
#define SPELLFLAG_SUMMON	0x00100	// summon a creature.
#define SPELLFLAG_GOOD		0x00200	// The spell is a good spell. u intend to help to receiver.
#define SPELLFLAG_RESIST	0x00400	// Allowed to resist this.	
#define SPELLFLAG_TARG_NOSELF	0x00800
#define SPELLFLAG_DISABLED	0x08000
#define SPELLFLAG_SCRIPTED	0x10000
#define	SPELLFLAG_PLAYERONLY	0x20000	// casted by players only
#define	SPELLFLAG_NOUNPARALYZE	0x40000	// casted by players only
#define SPELLFLAG_NO_CASTANIM	0x80000	// play no anim while casting (also override SPELLFLAG_DIR_ANIM)
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

	CValueCurveDef	m_CastTime;		// In TICK_PER_SEC.
	CValueCurveDef	m_Effect;		// Damage or effect level based on skill of caster.100% magery
	CValueCurveDef	m_Duration;		// length of effect. in TICK_PER_SEC
	CValueCurveDef	m_Interrupt;	// chance to interrupt a spell
	
public:
	bool IsSpellType( DWORD wFlags ) const
	{
		return(( m_dwFlags & wFlags ) ? true : false );
	}
	CSpellDef( SPELL_TYPE id );
	virtual ~CSpellDef()
	{
	}
	LPCTSTR GetName() const { return( m_sName ); }
	bool r_LoadVal( CScript & s );
	bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );

	
	bool GetPrimarySkill( int * iSkill = NULL, int * iQty = NULL ) const;
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
	CRandGroupDef( RESOURCE_ID rid ) :
		CResourceLink( rid )
	{
		m_iTotalWeight = 0;
	}
	virtual ~CRandGroupDef()
	{
	}
	virtual bool r_LoadVal( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pKey, CGString &sVal, CTextConsole * pSrc = NULL );
	int GetRandMemberIndex( CChar * pCharSrc = NULL, bool bTrigger = true ) const;
	CResourceQty GetMember( int i ) const
	{
		return( m_Members[i] );
	}
	RESOURCE_ID GetMemberID( int i ) const
	{
		return( m_Members[i].GetResourceID() );
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
	STAT_KARMA = 4,		// -10000 to 10000 - also used as the food consumption main timer.
	STAT_FAME,
	STAT_QTY,
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
	CSkillClassDef( RESOURCE_ID rid ) :
		CResourceLink( rid )
	{
		// If there was none defined in scripts.
		Init();
	}
	virtual ~CSkillClassDef()
	{
	}

	LPCTSTR GetName() const { return( m_sName ); }

	bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	bool r_LoadVal( CScript & s );
};

enum SKTRIG_TYPE
{
	// All skills may be scripted.
	// XTRIG_UNKNOWN	= some named trigger not on this list.
	SKTRIG_ABORT=1,	// Some odd thing went wrong.
	SKTRIG_FAIL,	// we failed the skill check.
	SKTRIG_GAIN,
	SKTRIG_PRESTART, //called before any hardcoded messages
	SKTRIG_SELECT,	// just selecting params for the skill
	SKTRIG_START,	// params for skill are done. (stroke)
	SKTRIG_STROKE,	
	SKTRIG_SUCCESS,
	SKTRIG_USEQUICK,
	SKTRIG_QTY,
};


enum	SKF_TYPE
{
	SKF_SCRIPTED		= 0x0001,		// fully scripted, no hardcoded behaviour
	SKF_FIGHT			= 0x0002,		// considered a fight skill, maintains fight active
	SKF_MAGIC			= 0x0004,		// considered a magic skill
	SKF_CRAFT			= 0x0008,		// considered a crafting skill, compatible with MAKEITEM function
	SKF_IMMOBILE		= 0x0010,		// fails skill if character moves
	SKF_SELECTABLE		= 0x0020,		// allows skill to be selected from the skill menu
	SKF_NOMINDIST		= 0x0040,		// you can mine, fish, chop, hack on the same point you are standing on
	SKF_NOANIM			= 0x0080,		// prevents hardcoded animation from playing
	SKF_NOSFX			= 0x0100		// prevents hardcoded sound from playing
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

	DWORD			m_dwFlags;
	DWORD			m_dwGroup;
	


	// Delay before skill complete. modified by skill level of course !
public:
	CSkillDef( SKILL_TYPE iSkill );
	virtual ~CSkillDef()
	{
	}
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
	int CompareKey( LPCTSTR pszKey, CValStr * pVal, bool fNoSpaces ) const
	{
		ASSERT( pszKey );
		ASSERT( pVal->m_pszName );
		return( strcmpi( pszKey, pVal->m_pszName ));
	}
};

struct CMultiDefArray : public CGObSortArray< CGrayMulti*, MULTI_TYPE >
{
	// store the static components of a IT_MULTI
	// Sorted array
	int CompareKey( MULTI_TYPE id, CGrayMulti* pBase, bool fNoSpaces ) const
	{
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
	WORD m_wDebugFlags;			// DEBUG In game effects to turn on and off.

	// Decay
	int  m_iDecay_Item;			// Base decay time in minutes.
	int  m_iDecay_CorpsePlayer;	// Time for a playercorpse to decay (mins).
	int  m_iDecay_CorpseNPC;	// Time for a NPC corpse to decay.

	// Save
	int  m_iSavePeriod;			// Minutes between saves.
	int  m_iSaveBackupLevels;	// How many backup levels.
	int  m_iSaveBackgroundTime;	// Speed of the background save in minutes.
	bool m_fSaveGarbageCollect;	// Always force a full garbage collection.

	// Account
	int  m_iDeadSocketTime;
	int	 m_iArriveDepartMsg;    // General switch to turn on/off arrival/depart messages.
	int  m_iClientsMax;		// Maximum (FD_SETSIZE) open connections to server
	int  m_iClientsMaxIP;		// Maximum (FD_SETSIZE) open connections to server per IP
	int  m_iConnectingMax;		// max clients connecting
	int  m_iConnectingMaxIP;		// max clients connecting

	int  m_iGuestsMax;		// Allow guests who have no accounts ?
	int  m_iClientLingerTime;	// How long logged out clients linger in seconds.
	int  m_iMinCharDeleteTime;	// How old must a char be ? (minutes)
	int  m_iMaxCharsPerAccount;	// MAX_CHARS_PER_ACCT
	bool m_fLocalIPAdmin;		// The local ip is the admin ?
	bool m_fMd5Passwords;		// Should MD5 hashed passwords be used?

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

	// In Game Effects
	int	 m_iLightDungeon;
	int  m_iLightDay;		// Outdoor light level.
	int  m_iLightNight;		// Outdoor light level.
	int  m_iGameMinuteLength;	// Length of the game world minute in real world (TICK_PER_SEC) seconds.
	bool m_fNoWeather;			// Turn off all weather.
	bool m_fCharTags;			// Put [NPC] tags over chars.
	bool m_fFlipDroppedItems;	// Flip dropped items.
	bool m_fMonsterFight;	// Will creatures fight amoung themselves.
	bool m_fMonsterFear;	// will they run away if hurt ?
	int	 m_iBankIMax;	// Maximum number of items allowed in bank.
	int  m_iBankWMax;	// Maximum weight in WEIGHT_UNITS stones allowed in bank.
	int  m_iVendorMaxSell;		// Max things a vendor will sell in one shot.
	int  m_iMaxCharComplexity;		// How many chars per sector.
	int  m_iMaxItemComplexity;		// How many items per meter.
	int  m_iMaxSectorComplexity;	// How many items per sector.
	bool m_fGenericSounds;		// Do players receive generic (not them-devoted) sounds
	bool m_fAutoNewbieKeys;		// Are house and boat keys newbied automatically?
	int  m_iStamRunningPenalty;		// Weight penalty for running (+N% of max carry weight)
	int  m_iStaminaLossAtWeight;	// %Weight at which characters begin to lose stamina
	int  m_iHitpointPercentOnRez;// How many hitpoints do they get when they are rez'd?
	int  m_iHitsHungerLoss;		// How many % of HP will loose char on starving
	int  m_iMaxBaseSkill;		// Maximum value for base skills at char creation
	bool m_fInitHiddenSkills;	// Hidden skills will be initialised at char creation
	int  m_iTrainSkillPercent;	// How much can NPC's train up to ?
	int	 m_iTrainSkillMax;
	int	 m_iMountHeight;		// The height at which a mounted person clears ceilings.
	int	 m_iMoveRate;			// The percent rate of NPC movement
	int  m_iArcheryMaxDist;
	int  m_iArcheryMinDist;
	int  m_iHitsUpdateRate;		// how often send my hits updates to visible clients
	int  m_iSpeedScaleFactor;	// fight skill delay = m_iSpeedScaleFactor / ( (dex + 100) * Weapon Speed )
	int  m_iSkillPracticeMax;	// max skill level a player can practice on dummies/targets upto

	// Flags for controlling pvp/pvm behaviour of players
	int  m_iCombatFlags;		// combat flags
	int  m_iMagicFlags;			// magic flags

	// Criminal/Karma
	bool m_fGuardsInstantKill;	// Will guards kill instantly or follow normal combat rules?
	int	 m_iGuardLingerTime;	// How long do guards linger about.
	int  m_iSnoopCriminal;		// 1 in # chance of getting criminalflagged when succesfully snooping.
	int  m_iMurderMinCount;		// amount of murders before we get title.
	int	 m_iMurderDecayTime;	// (minutes) Roll murder counts off this often.
	bool m_fHelpingCriminalsIsACrime;// If I help (rez, heal, etc) a criminal, do I become one too?
	bool m_fLootingIsACrime;	// Looting a blue corpse is bad.
	int  m_iCriminalTimer;		// How many minutes are criminals flagged for?
	int	 m_iPlayerKarmaNeutral;	// How much bad karma makes a player neutral?
	int	 m_iPlayerKarmaEvil;


	// other
	bool	m_fNoResRobe;
	int		m_iLostNPCTeleport;
	int		m_iExperimental;
	int		m_iOptionFlags;
	int		m_iWoolGrowthTime;	// how long till wool grows back on sheared sheep, in minutes

	int		m_iDistanceYell;
	int		m_iDistanceWhisper;
	int		m_iDistanceTalk;

	int		m_iMaxSkill;
	CGString	m_sSpeechSelf;
	CGString	m_sSpeechPet;
	CGString	m_sSpeechOther;
	CGString	m_sCommandTrigger;

#ifdef _DUMPSUPPORT
	CGString	m_sDumpAccPackets;
#endif

	CGString	m_sEventsPet;
	CResourceLink *	m_pEventsPetLink;

	CGString	m_sEventsPlayer;
	CResourceLink *	m_pEventsPlayerLink;

	CGString	m_sEventsRegion;
	CResourceLink *	m_pEventsRegionLink;

	CGString	m_sStripPath;

	int		m_iWalkBuffer;
	int		m_iWalkRegen;

	int		m_iCommandLog;
	
	bool 		m_fUsecrypt;
	bool 		m_fUsenocrypt;

	bool		m_fPayFromPackOnly;	// Pay only from main pack?
	int		m_iOverSkillMultiply;	// multiplyer to get over skillclass
	bool	m_fSuppressCapitals;	// Enable/Disable capital letters suppression

#define ADVANCEDLOS_DISABLED		0x00
#define	ADVANCEDLOS_PLAYER			0x01
#define	ADVANCEDLOS_NPC				0x02
	int		m_iAdvancedLos;		// AdvancedLOS

	// New ones
	int		m_iFeatureT2A;
	int		m_iFeatureLBR;
	int		m_iFeatureAOS;
	int		m_iFeatureSE;
	int		m_iFeatureML;
	
	int		m_iMaxLoopTimes;
#define	STAT_FLAG_NORMAL	0x00	//	MAX* status allowed (default)
#define STAT_FLAG_DENYMAX	0x01	//	MAX* denied
#define STAT_FLAG_DENYMAXP	0x02	//		.. for players
#define STAT_FLAG_DENYMAXN	0x04	//		.. for npcs
	int		m_iStatFlag;

#define NPC_AI_PATH		0x0001		//	NPC pathfinding
#define	NPC_AI_FOOD		0x0002		//	NPC food search (objects + grass)
#define	NPC_AI_EXTRA	0x0004		//	NPC magics, combat, etc
#define NPC_AI_ALWAYSINT 0x0008		//	NPC pathfinding does not check int, always smart
#define NPC_AI_INTFOOD	0x00010		//  NPC food search (more intelligent and trusworthy)
//#define NPC_AI_MAGICS	0x00020
#define NPC_AI_COMBAT	0x00040
#define NPC_AI_VEND_TIME		0x00080
#define NPC_AI_LOOTING	0x00100
#define	NPC_AI_MOVEOBSTACLES	0x00200
	int		m_iNpcAi;

	//	Experience system
	bool	m_bExperienceSystem;
#define EXP_MODE_RAISE_COMBAT	0x0001
#define	EXP_MODE_RAISE_CRAFT	0x0002
#define	EXP_MODE_ALLOW_DOWN		0x0004
#define	EXP_MODE_DOWN_NOLEVEL	0x0008
#define	EXP_MODE_AUTOSET_EXP	0x0010
#define EXP_MODE_TRIGGER_EXP	0x0020
#define EXP_MODE_TRIGGER_LEVEL	0x0040
	int		m_iExperienceMode;
	int		m_iExperienceKoefPVM;
	int		m_iExperienceKoefPVP;
	bool	m_bLevelSystem;
#define LEVEL_MODE_LINEAR		0
#define	LEVEL_MODE_DOUBLE		1
	int		m_iLevelMode;
	int		m_iLevelNextAt;

	int		m_iAutoResDisp;
	int		m_iAutoPrivFlags;

	char	m_cCommandPrefix;

	int		m_iMaxTooltipForTick;

	//	color noto flag
	int		m_iColorNotoCriminal;
	int		m_iColorNotoDefault;
	int		m_iColorNotoEvil;
	int		m_iColorNotoGood;
	int		m_iColorNotoGuildSame;
	int		m_iColorNotoGuildWar;
	int		m_iColorNotoNeutral;

#ifndef _EXTERNAL_DLL
	//	mySQL features
	bool		m_bMySql;
	CGString	m_sMySqlHost;
	CGString	m_sMySqlUser;
	CGString	m_sMySqlPass;
	CGString	m_sMySqlDB;
#else
	CGString	m_sDbDll;
	CGString	m_sDbDllHost;
	CGString	m_sDbDllUser;
	CGString	m_sDbDllPass;
	CGString	m_sDbDllDB;
	int			m_iDbDllQueryBuffer;
#endif

	int			m_iRegenRate[STAT_QTY];
	int			m_iTimerCall;
	bool		m_bAllowLightOverride;
	CGString	m_sZeroPoint;
	bool		m_bAllowBuySellAgent;

	bool		m_bAgree;

	// End INI file options.
	
	CResourceScript m_scpIni;	// Keep this around so we can link to it.
	CResourceScript m_scpCryptIni; // Encryption keys are in here
	CGObArray< CLogIP *> m_LogIP;	// Block these IP numbers

public:
	CResourceScript m_scpTables;

	CStringSortArray m_Obscene;	// Bad Names/Words etc.
	CGObArray< TCHAR* > m_Fame;	// fame titles (fame.famous)
	CGObArray< TCHAR* > m_Karma;	// karma titles (karma.wicked)
	CGObArray< TCHAR* > m_NotoTitles;	// Noto titles.
	CGObArray< TCHAR* > m_Runes;	// Words of power. (A-Z)

	CMultiDefArray m_MultiDefs;	// read from the MUL files. Cached here on demand.

	CObNameSortArray m_SkillNameDefs;	// const CSkillDef* Name sorted
	CGPtrTypeArray< CSkillDef* > m_SkillIndexDefs;	// Defined Skills indexed by number
	CGObArray< CSpellDef* > m_SpellDefs;	// Defined Spells
	CGObArray< CSpellDef* > m_SpellDefs_Sorted;	// Defined Spells, in skill order

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
	int GetPacketFlag( bool bCharlist, RESDISPLAY_VERSION res = RDS_T2A );

	// Specialized resource accessors.

	bool CanUsePrivVerb( const CScriptObj * pObjTarg, LPCTSTR pszCmd, CTextConsole * pSrc ) const;
	PLEVEL_TYPE GetPrivCommandLevel( LPCTSTR pszCmd ) const;

	CLogIP * FindLogIP( CSocketAddressIP dwIP, bool fCreate );
	bool SetLogIPBlock( LPCTSTR pszIP, bool fBlock, int iTimeDecay = -1 );

	static STAT_TYPE FindStatKey( LPCTSTR pszKey );
	static bool IsValidEmailAddressFormat( LPCTSTR pszText );
	bool IsObscene( LPCTSTR pszText ) const;

	CWebPageDef * FindWebPage( LPCTSTR pszPath ) const;

	CServerRef Server_GetDef( int index );

	const CSpellDef* GetSpellDef( SPELL_TYPE index ) const
	{
		if ( ! index || ! m_SpellDefs.IsValidIndex(index))
			return( NULL );
		return( m_SpellDefs[index] );
	}

	CSpellDef* GetSpellDef( SPELL_TYPE index ) 
	{
		if ( ! index || ! m_SpellDefs.IsValidIndex(index))
			return( NULL );
		return( m_SpellDefs[index] );
	}

	LPCTSTR GetSkillKey( SKILL_TYPE index ) const
	{
		return( m_SkillIndexDefs[index]->GetKey());
	}

	const bool IsSkillFlag( SKILL_TYPE index, SKF_TYPE skf ) const
	{
		if ( !m_SkillIndexDefs.IsValidIndex(index) )
			return false;
		const CSkillDef *	pSkillDef	= GetSkillDef( index );
		return ( pSkillDef && (pSkillDef->m_dwFlags & skf) );
	}

	const CSkillDef* GetSkillDef( SKILL_TYPE index ) const
	{
		return( m_SkillIndexDefs[index] );
	}
	
	CSkillDef* GetSkillDef( SKILL_TYPE index )
	{
		return( m_SkillIndexDefs[index] );
	}

	const CSkillDef* FindSkillDef( LPCTSTR pszKey ) const
	{
		// Find the skill name in the alpha sorted list.
		// RETURN: SKILL_NONE = error.
		int i = m_SkillNameDefs.FindKey( pszKey );
		if ( i < 0 )
			return( NULL );
		return( STATIC_CAST <const CSkillDef*>(m_SkillNameDefs[i]));
	}
	const CSkillDef* SkillLookup( LPCTSTR pszKey );
	SKILL_TYPE FindSkillKey( LPCTSTR pszKey ) const;

	int GetSpellEffect( SPELL_TYPE spell, int iSkillval ) const;

	LPCTSTR GetRune( TCHAR ch ) const
	{
		ch = toupper(ch) - 'A';
		if ( ! m_Runes.IsValidIndex(ch))
			return "?";
		return( m_Runes[ ch ] );
	}
	LPCTSTR GetNotoTitle( int iLevel, bool bFemale ) const;

	const CGrayMulti * GetMultiItemDefs( ITEMID_TYPE itemid );

	bool IsConsoleCmd( TCHAR ch ) const;

	CPointMap GetRegionPoint( LPCTSTR pCmd ) const; // Decode a teleport location number into X/Y/Z

	int Calc_MaxCarryWeight( const CChar * pChar ) const;
	int Calc_DropStamWhileMoving( CChar * pChar, int iWeightLoadPercent );
	int Calc_WalkThroughChar( CChar * pCharMove, CChar * pCharObstacle );
	int Calc_CombatAttackSpeed( CChar * pChar, CItem * pWeapon );
	int Calc_CombatChanceToHit( CChar * pChar, SKILL_TYPE skill, CChar * pCharTarg, CItem * pWeapon );
	bool Calc_SkillCheck( int iSkillLevel, int iDifficulty );
	int  Calc_StealingItem( CChar * pCharThief, CItem * pItem, CChar * pCharMark );
	bool Calc_CrimeSeen( CChar * pCharThief, CChar * pCharViewer, SKILL_TYPE SkillToSee, bool fBonus );
	int Calc_FameKill( CChar * pKill );
	int Calc_FameScale( int iFame, int iFameChange );
	int Calc_KarmaKill( CChar * pKill, NOTO_TYPE NotoThem );
	int Calc_KarmaScale( int iKarma, int iKarmaChange );
	LPCTSTR Calc_MaptoSextant( CPointMap pntCoords );
	
#define SysMessageDefault( msg )	SysMessage( g_Cfg.GetDefaultMsg( msg ) )
	LPCTSTR CResource::GetDefaultMsg(LPCTSTR pszKey);
	LPCTSTR	GetDefaultMsg(long lKeyNum);
} g_Cfg;



class CDialogDef : public CResourceLink
{
	static LPCTSTR const sm_szLoadKeys[];

public:
	static const char *m_sClassName;
	bool		GumpSetup( int iPage, CClient * pClientSrc, CObjBase * pObj, LPCTSTR Arguments = "" );
	int			GumpAddText( LPCTSTR pszText );		// add text to the text section, return insertion index
	bool		r_Verb( CScript &s, CTextConsole * pSrc );
	bool		r_LoadVal( CScript & s );
	bool		r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );

	CDialogDef( RESOURCE_ID rid );
	virtual ~CDialogDef()
	{
	}

public:	
	// temporary placeholders - valid only during dialog setup
	CObjBase *	m_pObj;
	CGString	m_sControls[1024];
	CGString	m_sText[512];
	int			m_iTexts;
	int			m_iControls;
	int			m_x;
	int			m_y;

	int			m_iOriginX;	// keep track of position when parsing
	int			m_iOriginY;
	WORD		m_iPage;		// page to open the dialog in
};




class CItemTypeDef : public CResourceLink
{
public:
	static const char *m_sClassName;
	CItemTypeDef( RESOURCE_ID rid ) : CResourceLink( rid )
	{	
	}

	bool		r_LoadVal( CScript & s );

	int			GetItemType();
};

#define IsSetEF(ef)		(g_Cfg.m_iExperimental & ef)
#define IsSetOF(of)		(g_Cfg.m_iOptionFlags & of)
#define IsSetSpecific	((g_Cfg.m_iExperimental & EF_Specific) && (g_Cfg.m_iOptionFlags & OF_Specific))

#define IsSetCombatFlags(of)		(g_Cfg.m_iCombatFlags & of)
#define IsSetMagicFlags(of)		(g_Cfg.m_iMagicFlags & of)

#endif	// _INC_CRESOURCE_H
