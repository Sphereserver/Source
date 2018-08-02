//
// CChar.h
//

#ifndef _INC_CCHAR_H
#define _INC_CCHAR_H
#pragma once

enum NPCBRAIN_TYPE
{
	NPCBRAIN_NONE,		// 0 = This should never really happen
	NPCBRAIN_ANIMAL,	// 1 = Can be tamed
	NPCBRAIN_HUMAN,		// 2 = Generic human
	NPCBRAIN_HEALER,	// 3 = Can resurrect
	NPCBRAIN_GUARD,		// 4 = Will attack criminal/murderers
	NPCBRAIN_BANKER,	// 5 = Can open bank box for you
	NPCBRAIN_VENDOR,	// 6 = Can buy/sell items
	NPCBRAIN_STABLE,	// 7 = Can stable pets
	NPCBRAIN_MONSTER,	// 8 = Usually evil
	NPCBRAIN_BERSERK,	// 9 = Attack every nearby creature (blade spirit, energy vortex)
	NPCBRAIN_DRAGON,	// 10 = Can breath fire attacks
	NPCBRAIN_QTY
};

// Number of steps to remember for pathfinding (default to 24 steps, will have 24*4 extra bytes per char)
#define MAX_NPC_PATH_STORAGE_SIZE	UO_MAP_VIEW_SIGHT * 2

struct CCharNPC
{
	// This is basically the unique "brains" for any character
public:
	CCharNPC(CChar *pChar, NPCBRAIN_TYPE NPCBrain);
	~CCharNPC();

private:
	CCharNPC(const CCharNPC &copy);
	CCharNPC &operator=(const CCharNPC &other);

public:
	static const char *m_sClassName;
	// Stuff that is specific to an NPC character instance (not an NPC type see CCharBase for that)
	// Any NPC AI stuff will go here
	static LPCTSTR const sm_szVerbKeys[];

	NPCBRAIN_TYPE m_Brain;		// Number of the assigned basic AI block
	WORD m_Home_Dist_Wander;	// Distance to allow to "wander"
	BYTE m_Act_Motivation;		// 0-100 (100=very greatly) how bad do i want to do the current action
	bool m_bonded;				// Bonded pet

	// We respond to what we here with this
	CResourceRefArray m_Speech;	// Speech fragment list (other stuff we know)

	CResourceQty m_Need;		// What items might i need/desire? (coded as resource scripts) ex "10 gold,20 logs" etc

	static LPCTSTR const sm_szLoadKeys[];

	WORD m_nextX[MAX_NPC_PATH_STORAGE_SIZE];	// Array of X coords of the next step
	WORD m_nextY[MAX_NPC_PATH_STORAGE_SIZE];	// Array of Y coords of the next step
	CPointMap m_nextPt;							// Where the array wants to go (if changed, recount the path)
	CServTime m_timeRestock;					// When last restock happened in sell/buy container

	struct Spells {
		SPELL_TYPE id;
	};
	std::vector<Spells> m_spells;	// spells stored in this NPC

	int Spells_GetCount();
	SPELL_TYPE Spells_GetAt(BYTE id);
	bool Spells_DelAt(BYTE id);
	bool Spells_Add(SPELL_TYPE spell);
	int Spells_FindSpell(SPELL_TYPE spell);

public:
	void r_WriteChar(CChar *pChar, CScript &s);
	bool r_WriteVal(CChar *pChar, LPCTSTR pszKey, CGString &sVal);
	bool r_LoadVal(CChar *pChar, CScript &s);

	int GetNpcAiFlags(const CChar *pChar) const;
};

struct CCharPlayer
{
	// Stuff that is specific to a player character
public:
	CCharPlayer(CChar *pChar, CAccount *pAccount);
	~CCharPlayer();

private:
	CCharPlayer(const CCharPlayer &copy);
	CCharPlayer &operator=(const CCharPlayer &other);

	BYTE m_SkillLock[SKILL_QTY];	// List of skills lock state
	BYTE m_StatLock[STAT_BASE_QTY];	// List of stats lock state
	CResourceRef m_SkillClass;		// What skill class group we have selected

public:
	static const char *m_sClassName;
	CAccount *m_pAccount;	// The account index (for idle players mostly)
	static LPCTSTR const sm_szVerbKeys[];

	CServTime m_timeLastUsed;	// Time the player char was last used

	CGString m_sProfile;	// Limited to SCRIPT_MAX_LINE_LEN-16

	WORD m_wMurders;		// Murder count
	WORD m_wDeaths;			// Death count
	BYTE m_speedMode;		// speed mode (0x0 = default, 0x1 = fast, 0x2 = slow, 0x3 = hybrid)
	bool m_bRefuseTrades;
	bool m_bKrToolbarEnabled;

	static LPCTSTR const sm_szLoadKeys[];

	CResourceRefArray m_Speech;	// Speech fragment list (other stuff we know)

public:
	SKILL_TYPE Skill_GetLockType(LPCTSTR pszKey) const;
	SKILLLOCK_TYPE Skill_GetLock(SKILL_TYPE skill) const;
	void Skill_SetLock(SKILL_TYPE skill, SKILLLOCK_TYPE state);

	STAT_TYPE Stat_GetLockType(LPCTSTR pszKey) const;
	SKILLLOCK_TYPE Stat_GetLock(STAT_TYPE stat) const;
	void Stat_SetLock(STAT_TYPE stat, SKILLLOCK_TYPE state);

	void r_WriteChar(CChar *pChar, CScript &s);
	bool r_WriteVal(CChar *pChar, LPCTSTR pszKey, CGString &sVal);
	bool r_LoadVal(CChar *pChar, CScript &s);

	bool SetSkillClass(CChar *pChar, RESOURCE_ID rid);
	CSkillClassDef *GetSkillClass() const;
};

enum WAR_SWING_TYPE		// m_atFight.m_Swing_State
{
	WAR_SWING_INVALID = -1,
	WAR_SWING_EQUIPPING,		// Char is recoiling the weapon
	WAR_SWING_READY,			// Char is ready to swing
	WAR_SWING_SWINGING			// Char is swinging the weapon
};

enum CTRIG_TYPE
{
	CTRIG_AAAUNUSED,
	CTRIG_AfterClick,
	CTRIG_Attack,				// Char attacked someone (SRC)
	CTRIG_CallGuards,
	CTRIG_charAttack,
	CTRIG_charClick,
	CTRIG_charClientTooltip,
	CTRIG_charContextMenuRequest,
	CTRIG_charContextMenuSelect,
	CTRIG_charDClick,
	CTRIG_charTradeAccepted,
	CTRIG_Click,				// Char got single clicked by someone
	CTRIG_ClientTooltip,		// Char tooltip got requested by someone
	CTRIG_CombatAdd,			// Char added someone on attacker list
	CTRIG_CombatDelete,			// Char deleted someone from attacker list
	CTRIG_CombatEnd,			// Char had end an combat
	CTRIG_CombatStart,			// Char had started an combat
	CTRIG_ContextMenuRequest,
	CTRIG_ContextMenuSelect,
	CTRIG_Create,				// Char got created (not placed in the world yet)
	CTRIG_Criminal,				// Char got flagged criminal for someone
	CTRIG_DClick,				// Char got double clicked by someone
	CTRIG_Death,				// Char got killed
	CTRIG_DeathCorpse,			// Char dead corpse is being created
	CTRIG_Destroy,				// Char got destroyed (removed)
	CTRIG_Dismount,
	CTRIG_Eat,
	CTRIG_EffectAdd,
	CTRIG_EnvironChange,		// Char environment has changed (region, light, weather, season)
	CTRIG_ExpChange,			// Char EXP got changed
	CTRIG_ExpLevelChange,		// Char LEVEL got changed
	CTRIG_FameChange,			// Char FAME got changed
	CTRIG_FollowersUpdate,		// Char CURFOLLOWER got changed
	CTRIG_GetHit,				// Char got hit by someone
	CTRIG_Hit,					// Char hit someone
	CTRIG_HitCheck,
	CTRIG_HitIgnore,
	CTRIG_HitMiss,
	CTRIG_HitTry,				// Char is trying to hit someone
	CTRIG_HouseDesignCommit,	// Char committed a new house design
	CTRIG_HouseDesignExit,		// Char exited house design mode
	CTRIG_itemAfterClick,
	CTRIG_itemBuy,
	CTRIG_itemClick,			// Char single clicked an item
	CTRIG_itemClientTooltip,
	CTRIG_itemContextMenuRequest,
	CTRIG_itemContextMenuSelect,
	CTRIG_itemCreate,
	CTRIG_itemDamage,
	CTRIG_itemDCLICK,			// Char double clicked an item
	CTRIG_itemDestroy,			// Char destroyed an item
	CTRIG_itemDROPON_CHAR,		// Char dropped an item over an char
	CTRIG_itemDROPON_GROUND,	// Char dropped an item on ground
	CTRIG_itemDROPON_ITEM,		// Char dropped an item over an item
	CTRIG_itemDROPON_SELF,		// Char dropped an item inside an container itself
	CTRIG_itemDROPON_TRADE,		// Char dropped an item on trade window
	CTRIG_itemEQUIP,			// Char equipped an item
	CTRIG_itemEQUIPTEST,
	CTRIG_itemMemoryEquip,
	CTRIG_itemPICKUP_GROUND,	// Char picked up an item on the ground
	CTRIG_itemPICKUP_PACK,		// Char picked up an item inside an container
	CTRIG_itemPICKUP_SELF,		// Char picked up an item inside an container itself
	CTRIG_itemPICKUP_STACK,		// Char picked up an item from an stack
	CTRIG_itemSell,
	CTRIG_itemSPELL,			// Char casted an spell on an item
	CTRIG_itemSTEP,				// Char stepped on an item
	CTRIG_itemTARGON_CANCEL,
	CTRIG_itemTARGON_CHAR,
	CTRIG_itemTARGON_GROUND,
	CTRIG_itemTARGON_ITEM,
	CTRIG_itemTimer,
	CTRIG_itemToolTip,			// Char requested an item tooltip
	CTRIG_itemUNEQUIP,			// Char unequipped an item
	CTRIG_Jailed,				// Char got jailed/forgived
	CTRIG_KarmaChange,			// Char KARMA got changed
	CTRIG_Kill,					// Char killed someone
	CTRIG_LogIn,				// Char client is logging in
	CTRIG_LogOut,				// Char client is logging out
	CTRIG_Mount,
	CTRIG_MurderDecay,			// Char got an murder count (KILL) decayed
	CTRIG_MurderMark,			// Char got flagged as murderer
	CTRIG_NotoSend,				// Char notoriety got requested by someone
	CTRIG_NPCAcceptItem,		// NPC accepted an item given by someone (according to DESIRES)
	CTRIG_NPCActFight,
	CTRIG_NPCActFollow,			// NPC decided to follow someone
	CTRIG_NPCAction,
	CTRIG_NPCHearGreeting,		// NPC heared someone for the first time
	CTRIG_NPCHearUnknown,		// NPC heared something that it doesn't understand
	CTRIG_NPCLookAtChar,
	CTRIG_NPCLookAtItem,
	CTRIG_NPCLostTeleport,		// NPC got teleported back to spawn point after walk too far
	CTRIG_NPCRefuseItem,		// NPC refused an item given by someone
	CTRIG_NPCRestock,
	CTRIG_NPCSeeNewPlayer,		// NPC saw an new player for the first time
	CTRIG_NPCSeeWantItem,		// NPC saw an wanted item when looting corpses
	CTRIG_NPCSpecialAction,
	CTRIG_PartyDisband,			// Char disbanded his party
	CTRIG_PartyInvite,			// Char invited someone to join his party
	CTRIG_PartyLeave,			// Char had left an party
	CTRIG_PartyRemove,			// Char got removed from an party
	CTRIG_PersonalSpace,		// Char got stepped by someone
	CTRIG_PetDesert,			// NPC deserted his master after get attacked by him or get starving for too long
	CTRIG_Profile,				// Char profile got requested by someone
	CTRIG_ReceiveItem,			// NPC is receiving an item from someone
	CTRIG_RegenStat,			// Char stats got regenerated (hits, mana, stam, food)
	CTRIG_RegionEnter,
	CTRIG_RegionLeave,
	CTRIG_RegionResourceFound,	// Char found an resource using an gathering skill
	CTRIG_RegionResourceGather,	// Char gathered an resource using an gathering skill
	CTRIG_Rename,				// Char got renamed
	CTRIG_Resurrect,			// Char got resurrected
	CTRIG_SeeCrime,				// Char saw someone nearby committing a crime
	CTRIG_SeeHidden,			// Char saw someone hidden
	CTRIG_SeeSnoop,				// Char saw someone nearby snooping an item
	CTRIG_SkillAbort,			// SKTRIG_ABORT
	CTRIG_SkillChange,
	CTRIG_SkillFail,			// SKTRIG_FAIL
	CTRIG_SkillGain,			// SKTRIG_GAIN
	CTRIG_SkillMakeItem,
	CTRIG_SkillMenu,
	CTRIG_SkillPreStart,		// SKTRIG_PRESTART
	CTRIG_SkillSelect,			// SKTRIG_SELECT
	CTRIG_SkillStart,			// SKTRIG_START
	CTRIG_SkillStroke,			// SKTRIG_STROKE
	CTRIG_SkillSuccess,			// SKTRIG_SUCCESS
	CTRIG_SkillTargetCancel,	// SKTRIG_TARGETCANCEL
	CTRIG_SkillUseQuick,		// SKTRIG_USEQUICK
	CTRIG_SkillWait,			// SKTRIG_WAIT
	CTRIG_SpellBook,			// Char opened a spellbook
	CTRIG_SpellCast,			// Char casted a spell
	CTRIG_SpellEffect,			// Char got hit by a spell
	CTRIG_SpellFail,			// Char failed an spell cast
	CTRIG_SpellSelect,			// Char selected a spell to cast
	CTRIG_SpellSuccess,			// Char succeeded an spell cast
	CTRIG_SpellTargetCancel,	// Char cancelled spell target
	CTRIG_StatChange,			// Char stats got changed (STR/hits, INT/mana, DEX/stam, food)
	CTRIG_StepStealth,			// Char is walking/running while hidden
	CTRIG_Targon_Cancel,		// Char cancelled current TARGETF
	CTRIG_ToggleFlying,			// Char toggled flying mode (gargoyle only)
	CTRIG_ToolTip,				// Char tooltip got requested by someone
	CTRIG_TradeAccepted,		// Char accepted a trade window
	CTRIG_TradeClose,			// Char closed a trade window
	CTRIG_TradeCreate,			// Char created a trade window
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
	CTRIG_UserUltimaStoreButton,
	CTRIG_UserVirtue,
	CTRIG_UserVirtueInvoke,
	CTRIG_UserWarmode,
	CTRIG_QTY
};

class CChar : public CObjBase, public CContainer, public CTextConsole
{
	// RES_WORLDCHAR
public:
	CChar(CREID_TYPE id);
	~CChar();

private:
	CChar(const CChar &copy);
	CChar &operator=(const CChar &other);

private:
	// Spell type effects
	#define STATF_INVUL			0x00000001	// Invulnerability
	#define STATF_DEAD			0x00000002
	#define STATF_Freeze		0x00000004	// Paralyzed (spell)
	#define STATF_Invisible		0x00000008	// Invisible (spell)
	#define STATF_Sleeping		0x00000010	// You look like a corpse ?
	#define STATF_War			0x00000020	// War mode on ?
	#define STATF_Reactive		0x00000040	// have reactive armor on
	#define STATF_Poisoned		0x00000080	// Poison level is in the poison object
	#define STATF_NightSight	0x00000100	// All is light to you
	#define STATF_Reflection	0x00000200	// Magic reflect on
	#define STATF_Polymorph		0x00000400	// We have polymorphed to another form
	#define STATF_Incognito		0x00000800	// Dont show skill titles
	#define STATF_SpiritSpeak	0x00001000	// I can hear ghosts clearly
	#define STATF_Insubstantial	0x00002000	// Ghost has not manifest (or GM hidden)
	#define STATF_EmoteAction	0x00004000	// The creature will emote its actions to it's owners
	#define STATF_COMM_CRYSTAL	0x00008000	// I have a IT_COMM_CRYSTAL or listening item on me
	#define STATF_HasShield		0x00010000	// Using a shield
	#define STATF_ArcherCanMove	0x00020000	// Can move with archery
	#define STATF_Stone			0x00040000	// turned to stone
	#define STATF_Hovering		0x00080000	// hovering (flying gargoyle)
	#define STATF_Fly			0x00100000	// Flying or running
	//							0x00200000
	#define STATF_Hallucinating	0x00400000	// eat 'shrooms or bad food
	#define STATF_Hidden		0x00800000	// Hidden (non-magical)
	#define STATF_InDoors		0x01000000	// we are covered from the rain
	#define STATF_Criminal		0x02000000	// The guards will attack me (someone has called guards)
	#define STATF_Conjured		0x04000000	// This creature is conjured and will expire (leave no corpse or loot)
	#define STATF_Pet			0x08000000	// I am a pet/hirling, check for my owner memory
	#define STATF_Spawned		0x10000000	// I am spawned by a spawn item
	#define STATF_SaveParity	0x20000000	// Has this char been saved or not ?
	#define STATF_Ridden		0x40000000	// This is the horse (don't display me) I am being ridden
	#define STATF_OnHorse		0x80000000	// Mounted on horseback
	UINT64 m_StatFlag;			// Flags above

	#define SKILL_VARIANCE 150	// Difficulty modifier for determining success (15.0%)
	WORD m_Skill[SKILL_QTY];	// List of skills

// This is a character that can either be NPC or PC
// Player vs NPC Stuff

public:
	struct LastAttackers {
		DWORD charUID;
		INT64 elapsed;
		INT64 damage;
		INT64 threat;
	};
	std::vector<LastAttackers> m_lastAttackers;

	struct NotoSaves {
		DWORD charUID;
		INT64 elapsed;
		NOTO_TYPE value;
		NOTO_TYPE color;
	};
	std::vector<NotoSaves> m_notoSaves;

	static const char *m_sClassName;
	CClient *m_pClient;
	CCharPlayer *m_pPlayer;
	CCharNPC *m_pNPC;
	CPartyDef *m_pParty;
	CRegionWorld *m_pArea;
	CRegionBase *m_pRoom;

	static LPCTSTR const sm_szRefKeys[];
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szTrigName[CTRIG_QTY + 1];
	static const LAYER_TYPE sm_VendorLayers[3];

	// Combat stuff (not saved)
	CGrayUID m_uidWeapon;		// current equipped weapon
	CGrayUID m_uidWeaponLast;	// last equipped weapon (only used by 'EquipLastWeapon' client macro)
	WORD m_defense;				// calculated armor worn (NOT intrinsic armor)

	height_t m_height;			// Height set in-game or under some trigger (height=) - for both items and chars

	CGrayUID m_UIDLastNewItem;	// Last item created, used to store on this CChar the UID of the last created item via ITEM or ITEMNEWBIe in @Create and @Restock to prevent COLOR, etc properties to be called with no reference when the item was not really created, ie: ITEM=i_dagger,R5
	unsigned int m_exp;			// character experience
	unsigned int m_level;		// character experience level
	BYTE m_iVisualRange;		// Visual Range
	//DIR_TYPE m_dirClimb;		// we are standing on a CAN_I_CLIMB or UFLAG2_CLIMBABLE, DIR_QTY = not on climbable
	bool m_fClimbUpdated;		// FixClimbHeight() called?
	bool m_fIgnoreNextPetCmd;	// return 1 in speech block for this pet will make it ignore target petcmds while allowing the rest to perform them
	height_t m_zClimbHeight;	// The height at the end of the climbable

	// Saved stuff
	DIR_TYPE m_dirFace;			// facing this dir
	CGString m_sTitle;			// Special title such as "the guard" (replaces the normal skill title)
	CPointMap m_ptHome;			// What is our "home" region (towns and bounding of NPC's)
	UINT64 m_virtualGold;		// Virtual gold used by TOL clients
	short m_FollowerSlots;
	short m_FollowerCur;
	short m_FollowerMax;
	int m_Tithing;

	// Speech
	FONT_TYPE m_fonttype;		// speech font to use (client send this to server, but it's not used)
	HUE_TYPE m_SpeechHue;		// speech hue to use

	// In order to revert to original Hue and body
	CREID_TYPE m_prev_id;		// Backup of body type for ghosts and poly
	HUE_TYPE m_prev_Hue;		// Backup of skin color, in case of polymorph etc
	HUE_TYPE m_wBloodHue;		// Replicating CharDef's BloodColor on the char, or overriding it
	bool IsTriggerActive(LPCTSTR pszTrig) { return static_cast<CObjBase *>(const_cast<CChar *>(this))->IsTriggerActive(pszTrig); }
	void SetTriggerActive(LPCTSTR pszTrig = NULL) { static_cast<CObjBase *>(const_cast<CChar *>(this))->SetTriggerActive(pszTrig); }

	// Client's local light
	BYTE m_LocalLight;

	// When events happen to the char, check here for reaction scripts

	// Skills, Stats and health
	struct
	{
		int m_base;
		int m_mod;			// signed for modifier
		int m_val;			// signed for karma
		int m_max;			// max
		WORD m_regen;		// Tick time since last regen
	} m_Stat[STAT_QTY];

	CServTime m_timeLastRegen;	// When did i get my last regen tick ?
	CServTime m_timeCreate;		// When was i created ?
	CServTime m_timeLastHitsUpdate;
	UINT64 m_timeLastCallGuards;

	// Some character action in progress
	SKILL_TYPE m_Act_SkillCurrent;	// Currently using a skill
	CGrayUID m_Act_Targ;			// Current caction target
	CGrayUID m_Fight_Targ;			// Current combat target
	CGrayUID m_Act_TargPrv;			// Previous target
	int m_Act_Difficulty;			// -1 = fail skill (0-100) for skill advance calc
	CPointBase m_Act_p;				// Moving to this location, or location of forge we are working on
	int m_StepStealth;				// Max steps allowed to walk invisible while using Stealth skill

	// Args related to specific actions type (m_Act_SkillCurrent)
	union
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
			SPELL_TYPE m_Spell;			// ACTARG1 = Currently casting spell
			CREID_TYPE m_SummonID;		// ACTARG2 = A sub arg of the skill (summoned type ?)
		} m_atMagery;

		// SKILL_ALCHEMY
		// SKILL_BLACKSMITHING
		// SKILL_BOWCRAFT
		// SKILL_CARPENTRY
		// SKILL_CARTOGRAPHY
		// SKILL_INSCRIPTION
		// SKILL_TAILORING
		// SKILL_TINKERING
		struct
		{
			WORD m_Stroke_Count;		// ACTARG1 = For smithing, tinkering, etc. all requiring multi strokes
			ITEMID_TYPE m_ItemID;		// ACTARG2 = Making this item
			WORD m_Amount;				// ACTARG3 = How many of this item are we making?
		} m_atCreate;

		// SKILL_LUMBERJACKING
		// SKILL_MINING
		// SKILL_FISHING
		struct
		{
			DWORD m_ridType;			// ACTARG1 = Type of item we're harvesting
			BYTE m_bounceItem;			// ACTARG2 = Drop item on backpack (true) or drop it on ground (false)
			WORD m_Stroke_Count;		// ACTARG3 = All requiring multi strokes
		} m_atResource;

		// SKILL_TAMING
		// SKILL_MEDITATION
		struct
		{
			WORD m_Stroke_Count;		// ACTARG1 = All requiring multi strokes
		} m_atTaming;

		// SKILL_ARCHERY
		// SKILL_SWORDSMANSHIP
		// SKILL_MACEFIGHTING
		// SKILL_FENCING
		// SKILL_WRESTLING
		// SKILL_THROWING
		struct
		{
			WAR_SWING_TYPE m_Swing_State;		// ACTARG1 = State of the current swing occurring
			CServTime m_Swing_NextAction;		// ACTARG2 = Delay to wait before start another swing
			BYTE m_Swing_Delay;					// ACTARG3 = Delay of the current swing occurring (in tenths of second)
		} m_atFight;

		// NPCACT_RIDDEN
		struct
		{
			CGrayUIDBase m_FigurineUID;		// ACTARG1 = This creature is being ridden by this object link (IT_FIGURINE, IT_EQ_HORSE)
		} m_atRidden;

		// NPCACT_TALK
		// NPCACT_TALK_FOLLOW
		struct
		{
			int m_HearUnknown;			// ACTARG1 = Speaking NPC has no idea what u're saying
			int m_WaitCount;			// ACTARG2 = How long have i been waiting (xN sec)
			// m_Act_Targ = who am i talking to ?
		} m_atTalk;

		// NPCACT_FLEE
		struct
		{
			int m_iStepsMax;			// ACTARG1 = How long should it take to get there
			int m_iStepsCurrent;		// ACTARG2 = How long has it taken ?
			// m_Act_Targ = who am i fleeing from ?
		} m_atFlee;
	};

public:
	// Status and attributes
	int IsWeird() const;
	signed char GetFixZ(CPointMap pt, DWORD dwBlockFlags = 0);
	virtual void Delete(bool fForce = false, CClient *pClient = NULL);
	virtual bool NotifyDelete(CClient *pClient = NULL);
	bool IsStatFlag(DWORD dwStatFlag) const
	{
		return (m_StatFlag & dwStatFlag);
	}
	void StatFlag_Set(DWORD dwStatFlag)
	{
		m_StatFlag |= dwStatFlag;
	}
	void StatFlag_Clear(DWORD dwStatFlag)
	{
		m_StatFlag &= ~dwStatFlag;
	}
	void StatFlag_Mod(DWORD dwStatFlag, bool fMod)
	{
		if ( fMod )
			m_StatFlag |= dwStatFlag;
		else
			m_StatFlag &= ~dwStatFlag;
	}
	bool IsPriv(WORD wPrivFlags) const
	{
		if ( m_pPlayer )
			return m_pPlayer->m_pAccount->IsPriv(wPrivFlags);
		return false;
	}
	PLEVEL_TYPE GetPrivLevel() const
	{
		if ( m_pPlayer )
			return m_pPlayer->m_pAccount->GetPrivLevel();
		return PLEVEL_Player;
	}

	CCharBase *Char_GetDef() const
	{
		return static_cast<CCharBase *>(Base_GetDef());
	}
	CRegionWorld *GetRegion() const
	{
		return m_pArea;
	}
	CRegionBase *GetRoom() const
	{
		return m_pRoom;
	}
	int GetSight() const
	{
		return static_cast<int>(m_iVisualRange);
	}
	void SetSight(BYTE newSight)
	{
		// NOTE: Client 7.0.55.27 added new screen resolutions on options menu, and it will lock
		// visual range value based on current resolution, so there's no way to change the value
		// manually anymore (but enhanced clients still allow changes). Using higher resolutions
		// will also increase max visual range (18 -> 24)

		BYTE iPreviousRange = m_iVisualRange;
		m_iVisualRange = minimum(newSight, 24);
		if ( m_pClient )
		{
			m_pClient->addVisualRange(m_iVisualRange);
			if ( m_iVisualRange > iPreviousRange )
				m_pClient->addPlayerSee(NULL);
		}
	}

	bool Can(DWORD dwCan) const
	{
		return (m_Can & dwCan);
	}
	bool IsResourceMatch(RESOURCE_ID_BASE rid, DWORD dwAmount);
	bool IsResourceMatch(RESOURCE_ID_BASE rid, DWORD dwAmount, DWORD dwArgResearch);

	bool IsSpeakAsGhost() const
	{
		return (IsStatFlag(STATF_DEAD) && !IsStatFlag(STATF_SpiritSpeak) && !IsPriv(PRIV_GM));
	}
	bool CanUnderstandGhost() const
	{
		// Can i understand player ghost speak ?
		if ( m_pNPC && (m_pNPC->m_Brain == NPCBRAIN_HEALER) )
			return true;
		if ( Skill_GetBase(SKILL_SPIRITSPEAK) >= g_Cfg.m_iMediumCanHearGhosts )
			return true;
		return (IsStatFlag(STATF_SpiritSpeak|STATF_DEAD) || IsPriv(PRIV_GM|PRIV_HEARALL));
	}
	bool IsPlayableCharacter() const
	{
		return (IsHuman() || IsElf() || IsGargoyle());
	}
	bool IsHuman() const
	{
		return CCharBase::IsHumanID(GetDispID());
	}
	bool IsElf() const
	{
		return CCharBase::IsElfID(GetDispID());
	}
	bool IsGargoyle() const
	{
		return CCharBase::IsGargoyleID(GetDispID());
	}
	int GetHealthPercent() const;
	LPCTSTR GetTradeTitle() const;

	// Information about us
	CREID_TYPE GetID() const
	{
		CCharBase *pCharDef = Char_GetDef();
		ASSERT(pCharDef);
		return pCharDef->GetID();
	}
	WORD GetBaseID() const
	{
		return static_cast<WORD>(GetID());
	}
	CREID_TYPE GetDispID() const
	{
		CCharBase *pCharDef = Char_GetDef();
		ASSERT(pCharDef);
		return pCharDef->GetDispID();
	}
	void SetID(CREID_TYPE id);

	LPCTSTR GetName() const
	{
		return GetName(true);
	}

	LPCTSTR GetNameWithoutIncognito() const
	{
		if ( IsStatFlag(STATF_Incognito) )
		{
			CItem *pSpell = LayerFind(LAYER_SPELL_Incognito);
			if ( !pSpell )
				pSpell = LayerFind(LAYER_FLAG_Potion);

			if ( pSpell && pSpell->IsType(IT_SPELL) && (pSpell->m_itSpell.m_spell == SPELL_Incognito) )
				return pSpell->GetName();
		}
		return GetName();
	}

	LPCTSTR GetName(bool fAllowAlt) const
	{
		if ( fAllowAlt )
		{
			LPCTSTR pszAltName = GetKeyStr("NAME.ALT");
			if ( pszAltName && *pszAltName )
				return pszAltName;
		}
		if ( !IsIndividualName() )			// allow some creatures to go unnamed
		{
			CCharBase *pCharDef = Char_GetDef();
			ASSERT(pCharDef);
			return pCharDef->GetTypeName();	// Just use it's type name instead
		}
		return CObjBase::GetName();
	}

	bool SetName(LPCTSTR pszName);
	height_t GetHeightMount() const;
	height_t GetHeight() const;

	bool CanSeeAsDead(const CChar *pChar = NULL) const;
	bool CanSeeInContainer(const CItemContainer *pContItem) const;
	bool CanSee(const CObjBaseTemplate *pObj) const;
	inline bool CanSeeLOS_New_Failed(CPointMap *pptBlock, CPointMap &ptNow) const;
	bool CanSeeLOS_Adv(const CPointMap &ptDst, CPointMap *pptBlock = NULL, int iMaxDist = UO_MAP_VIEW_SIGHT, WORD wFlags = 0) const;
	bool CanSeeLOS(const CPointMap &ptDst, CPointMap *pptBlock = NULL, int iMaxDist = UO_MAP_VIEW_SIGHT, WORD wFlags = 0) const;
	bool CanSeeLOS(const CObjBaseTemplate *pObj, WORD wFlags = 0) const;

	#define LOS_NB_LOCAL_TERRAIN	0x0001	// Terrain inside a region I am standing in does not block LOS
	#define LOS_NB_LOCAL_STATIC		0x0002	// Static items inside a region I am standing in do not block LOS
	#define LOS_NB_LOCAL_DYNAMIC	0x0004	// Dynamic items inside a region I am standing in do not block LOS
	#define LOS_NB_LOCAL_MULTI		0x0008	// Multi items inside a region I am standing in do not block LOS
	#define LOS_NB_TERRAIN			0x0010	// Terrain does not block LOS at all
	#define LOS_NB_STATIC			0x0020	// Static items do not block LOS at all
	#define LOS_NB_DYNAMIC			0x0040	// Dynamic items do not block LOS at all
	#define LOS_NB_MULTI			0x0080	// Multi items do not block LOS at all
	#define LOS_NB_WINDOWS			0x0100	// Windows do not block LOS (e.g. Archery + Magery)
	#define LOS_NO_OTHER_REGION		0x0200	// Do not allow LOS path checking to go out of your region
	#define LOS_NC_MULTI			0x0400	// Do not allow LOS path checking to go through (no cross) a multi region (except the one you are standing in)
	#define LOS_FISHING				0x0800	// Do not allow LOS path checking to go through objects or terrain which do not represent water
	#define LOS_NC_WATER			0x1000	// Water does not block LOS at all

	bool CanHear(const CObjBaseTemplate *pSrc, TALKMODE_TYPE mode) const;
	bool CanSeeItem(const CItem *pItem) const
	{
		ASSERT(pItem);
		if ( pItem->IsAttr(ATTR_INVIS) )
		{
			if ( IsPriv(PRIV_GM) )
				return true;

			TCHAR *uid = Str_GetTemp();
			sprintf(uid, "SeenBy_0%lx", static_cast<DWORD>(GetUID()));
			if ( !pItem->m_TagDefs.GetKeyNum(uid) )
				return false;
		}
		return true;
	}
	bool CanTouch(const CPointMap &pt) const;
	bool CanTouch(const CObjBase *pObj) const;
	IT_TYPE CanTouchStatic(CPointMap &pt, ITEMID_TYPE id, CItem *pItem);
	bool CanMove(CItem *pItem, bool fMsg = true) const;
	BYTE GetLightLevel() const;
	bool CanUse(CItem *pItem, bool fMoveOrConsume) const;
	bool IsMountCapable() const;

	WORD Food_CanEat(CObjBase *pObj) const;
	int Food_GetLevelPercent() const;
	LPCTSTR Food_GetLevelMessage(bool fPet, bool fHappy) const;

public:
	int Stat_GetAdjusted(STAT_TYPE i) const;

	void Stat_SetBase(STAT_TYPE i, int iVal);
	int Stat_GetBase(STAT_TYPE i) const;

	void Stat_AddMod(STAT_TYPE i, int iVal);
	void Stat_SetMod(STAT_TYPE i, int iVal);
	int Stat_GetMod(STAT_TYPE i) const;

	void Stat_SetVal(STAT_TYPE i, int iVal);
	int Stat_GetVal(STAT_TYPE i) const;

	void Stat_SetMax(STAT_TYPE i, int iVal);
	int Stat_GetMax(STAT_TYPE i) const;

	int Stat_GetSum() const;
	int Stat_GetLimit(STAT_TYPE i) const;

	bool Stat_Decrease(STAT_TYPE stat, SKILL_TYPE skill);
	bool Stats_Regen(INT64 iTimeDiff);
	WORD Stats_GetRegenVal(STAT_TYPE iStat, bool fGetTicks);

	SKILLLOCK_TYPE Stat_GetLock(STAT_TYPE stat)
	{
		if ( m_pPlayer )
			return m_pPlayer->Stat_GetLock(stat);
		return SKILLLOCK_UP;
	};

	void Stat_SetLock(STAT_TYPE stat, SKILLLOCK_TYPE state)
	{
		if ( m_pPlayer )
			return m_pPlayer->Stat_SetLock(stat, state);
	};

private:
	// Location and movement
	bool TeleportToCli(int iType, int iArgs);
	bool TeleportToObj(int iType, TCHAR *pszArgs);

	CRegionBase *CheckValidMove(CPointBase &ptDst, DWORD *pdwBlockFlags, DIR_TYPE dir, height_t *pClimbHeight, bool fPathFinding = false) const;
	void FixClimbHeight();
	bool MoveToRegion(CRegionWorld *pNewArea, bool fAllowReject);
	bool MoveToRoom(CRegionBase *pNewRoom, bool fAllowReject);
	bool IsVerticalSpace(CPointMap ptDst, bool fForceMount = false);

public:
	CChar *GetNext() const
	{
		return static_cast<CChar *>(CObjBase::GetNext());
	}
	CObjBaseTemplate *GetTopLevelObj() const
	{
		// Get the object that has a location in the world (ground level)
		return const_cast<CChar *>(this);
	}

	bool IsSwimming() const;

	bool MoveToRegionReTest(DWORD dwType)
	{
		return MoveToRegion(dynamic_cast<CRegionWorld *>(GetTopPoint().GetRegion(dwType)), false);
	}
	bool MoveToChar(CPointMap pt, bool bForceFix = false);
	bool MoveTo(CPointMap pt, bool bForceFix = false)
	{
		m_fClimbUpdated = false;	// update climb height
		return MoveToChar(pt, bForceFix);
	}
	virtual void SetTopZ(signed char z)
	{
		CObjBaseTemplate::SetTopZ(z);
		m_fClimbUpdated = false;	// update climb height
		FixClimbHeight();
	}
	bool MoveToValidSpot(DIR_TYPE dir, int iDist, int iDistStart = 1, bool bFromShip = false);
	virtual bool MoveNearObj(const CObjBaseTemplate *pObj, WORD wSteps = 0)
	{
		return CObjBase::MoveNearObj(pObj, wSteps);
	}
	bool MoveNear(CPointMap pt, WORD wSteps = 0)
	{
		return CObjBase::MoveNear(pt, wSteps);
	}

	CRegionBase *CanMoveWalkTo(CPointBase &ptDst, bool fCheckChars = true, bool fCheckOnly = false, DIR_TYPE dir = DIR_QTY, bool fPathFinding = false);
	void CheckRevealOnMove();
	TRIGRET_TYPE CheckLocation(bool fStanding = false);

public:
	// Client/player specific stuff
	void ClientAttach(CClient *pClient);
	void ClientDetach();

	bool SetPrivLevel(CTextConsole *pSrc, LPCTSTR pszFlags);
	bool CanDisturb(const CChar *pChar) const;
	void SetDisconnected();
	bool SetPlayerAccount(CAccount *pAccount);
	bool SetPlayerAccount(LPCTSTR pszAccName);
	bool SetNPCBrain(NPCBRAIN_TYPE NPCBrain);
	NPCBRAIN_TYPE GetNPCBrain(bool fGroupTypes = true) const;
	void ClearNPC();
	void ClearPlayer();

public:
	void ObjMessage(LPCTSTR pszMsg, const CObjBase *pSrc) const
	{
		if ( m_pClient )
			m_pClient->addObjMessage(pszMsg, pSrc);
	}
	void SysMessage(LPCTSTR pszMsg) const
	{
		if ( m_pClient )
			m_pClient->SysMessage(pszMsg);
	}

	void UpdateStatsFlag() const;
	void UpdateStatVal(STAT_TYPE type, int iChange = 0, int iLimit = 0);
	void UpdateHitsFlag();
	void UpdateModeFlag();
	void UpdateManaFlag() const;
	void UpdateStamFlag() const;
	void UpdateRegenTimers(STAT_TYPE iStat, WORD wVal);
	ANIM_TYPE GenerateAnimate(ANIM_TYPE action, bool fTranslate = true, bool fBackward = false, BYTE iFrameDelay = 0, BYTE iAnimLen = 7);
	bool UpdateAnimate(ANIM_TYPE action, bool fTranslate = true, bool fBackward = false, BYTE iFrameDelay = 0, BYTE iAnimLen = 7);

	void UpdateMode(CClient *pExcludeClient = NULL, bool fFull = false);
	void UpdateSpeedMode();
	void UpdateVisualRange();
	void UpdateMove(const CPointMap &ptOld, CClient *pClientExclude = NULL, bool bFull = false);
	void UpdateDir(DIR_TYPE dir);
	void UpdateDir(const CPointMap &pt);
	void UpdateDir(const CObjBaseTemplate *pObj);
	void UpdateDrag(CItem *pItem, CObjBase *pCont = NULL, CPointMap *pt = NULL);
	void Update(const CClient *pClientExclude = NULL);

public:
	LPCTSTR GetPronoun() const;	// he
	LPCTSTR GetPossessPronoun() const;	// his

	BYTE GetModeFlag(const CClient *pViewer = NULL) const;
	BYTE GetDirFlag() const
	{
		BYTE dir = static_cast<BYTE>(m_dirFace);
		if ( IsStatFlag(STATF_Fly) )	// running/flying
			dir |= 0x80;
		return dir;
	}

	DWORD GetMoveBlockFlags(bool bIgnoreGM = false) const
	{
		// What flags can block us?
		if ( !bIgnoreGM && IsPriv(PRIV_GM|PRIV_ALLMOVE) )	// nothing can blocks us
			return ULONG_MAX;

		DWORD dwCan = m_Can;
		CCharBase *pCharDef = Char_GetDef();
		if ( pCharDef && pCharDef->Can(CAN_C_GHOST) )
			dwCan |= CAN_C_GHOST;

		if ( IsStatFlag(STATF_Hovering) )
			dwCan |= CAN_C_HOVER;

		// Inversion of MT_INDOORS, so MT_INDOORS should be named MT_NOINDOORS now
		if ( dwCan & CAN_C_INDOORS )
			dwCan &= ~CAN_C_INDOORS;
		else
			dwCan |= CAN_C_INDOORS;

		return (dwCan & CAN_C_MOVEMASK);
	}

	int FixWeirdness();
	void CreateNewCharCheck();
	bool DupeFrom(CChar *pChar, bool fNewbieItems);

private:
	// Contents stuff
	void ContentAdd(CItem *pItem)
	{
		ItemEquip(pItem);
		//LayerAdd(pItem, LAYER_QTY);
	}

protected:
	void OnRemoveOb(CGObListRec *pObRec);	// override this = called when removed from list

public:
	bool CanCarry(const CItem *pItem) const;
	bool CanEquipStr(CItem *pItem) const;
	LAYER_TYPE CanEquipLayer(CItem *pItem, LAYER_TYPE layer, CChar *pCharMsg, bool fTest);
	CItem *LayerFind(LAYER_TYPE layer) const;
	void LayerAdd(CItem *pItem, LAYER_TYPE layer = LAYER_QTY);

	TRIGRET_TYPE OnCharTrigForLayerLoop(CScript &s, CTextConsole *pSrc, CScriptTriggerArgs *pArgs, CGString *pResult, LAYER_TYPE layer);
	TRIGRET_TYPE OnCharTrigForMemTypeLoop(CScript &s, CTextConsole *pSrc, CScriptTriggerArgs *pArgs, CGString *pResult, WORD wMemType);

	void OnWeightChange(int iChange);
	int GetWeight(WORD wAmount = 0) const
	{
		UNREFERENCED_PARAMETER(wAmount);
		return CContainer::GetTotalWeight();
	}
	int GetWeightLoadPercent(int iWeight) const;

	CItem *GetSpellbook(SPELL_TYPE spell) const;
	CItemContainer *GetContainer(LAYER_TYPE layer) const
	{
		return dynamic_cast<CItemContainer *>(LayerFind(layer));
	}
	CItemContainer *GetContainerCreate(LAYER_TYPE layer);
	CItem *GetBackpackItem(ITEMID_TYPE id);
	void AddGoldToPack(DWORD dwAmount, CItemContainer *pPack = NULL, bool bSound = true);

	virtual TRIGRET_TYPE OnTrigger(LPCTSTR pszTrigName, CTextConsole *pSrc, CScriptTriggerArgs *pArgs);

public:
	// Load/Save
	virtual bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);
	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);
	virtual bool r_LoadVal(CScript &s);
	virtual bool r_Load(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc = NULL);
	virtual void r_Write(CScript &s);

	void r_WriteParity(CScript &s);

	TRIGRET_TYPE OnTrigger(CTRIG_TYPE trigger, CTextConsole *pSrc, CScriptTriggerArgs *pArgs = NULL)
	{
		ASSERT(trigger < CTRIG_QTY);
		return OnTrigger(MAKEINTRESOURCE(trigger), pSrc, pArgs);
	}

private:
	// Fame/Karma stuff
	void Noto_Karma(int iKarmaChange, int iBottom = INT_MIN, bool bMessage = false);
	void Noto_Fame(int iFameChange);
	void Noto_ChangeNewMsg(int iPrvLevel);
	void Noto_ChangeDeltaMsg(int iDelta, LPCTSTR pszType);

public:
	// Notoriety stuff
	NOTO_TYPE Noto_GetFlag(const CChar *pCharViewer, bool bAllowInvul = false, bool bGetColor = false) const;
	NOTO_TYPE Noto_CalcFlag(const CChar *pCharViewer, bool bAllowInvul = false) const;
	HUE_TYPE Noto_GetHue(const CChar *pCharViewer) const;

	bool Noto_IsNeutral() const;
	bool Noto_IsMurderer() const;
	bool Noto_IsEvil() const;
	bool Noto_IsCriminal() const
	{
		if ( IsStatFlag(STATF_Criminal) )
			return true;
		return Noto_IsEvil();
	}

	int Noto_GetLevel() const;
	LPCTSTR Noto_GetFameTitle() const;
	LPCTSTR Noto_GetTitle() const;
	void Noto_Kill(CChar *pKill, bool fPetKill = false, int iTotalKillers = 0);
	bool Noto_Criminal(CChar *pChar = NULL);
	void Noto_Murder();

	void NotoSave_Add(CChar *pChar, NOTO_TYPE value, NOTO_TYPE color = NOTO_INVALID);
	void NotoSave_Delete(CChar *pChar);
	void NotoSave_Clear();
	void NotoSave_Update();
	int NotoSave_GetID(CChar *pChar);
	void NotoSave_CheckTimeout();

	bool IsTakeCrime(const CItem *pItem, CChar **ppCharMark = NULL) const;

	void ChangeExperience(int iDelta = 0, CChar *pCharDead = NULL);
	DWORD GetSkillTotal(int iWhat = 0, bool fHow = true);

	// Skills
	static bool IsSkillBase(SKILL_TYPE skill)
	{
		return ((skill > SKILL_NONE) && (skill < static_cast<SKILL_TYPE>(g_Cfg.m_iMaxSkill)));
	}

	static bool IsSkillNPC(SKILL_TYPE skill)
	{
		return ((skill >= NPCACT_FOLLOW_TARG) && (skill < NPCACT_QTY));
	}

	SKILL_TYPE Skill_GetBest(unsigned int uRank = 0) const;
	SKILL_TYPE Skill_GetActive() const
	{
		return m_Act_SkillCurrent;
	}
	LPCTSTR Skill_GetName(bool fUse = false) const;
	WORD Skill_GetBase(SKILL_TYPE skill) const
	{
		ASSERT(IsSkillBase(skill));
		return m_Skill[skill];
	}
	WORD Skill_GetMax(SKILL_TYPE skill, bool fIgnoreLock = false) const;
	DWORD Skill_GetSumMax() const;
	SKILLLOCK_TYPE Skill_GetLock(SKILL_TYPE skill) const
	{
		return m_pPlayer ? m_pPlayer->Skill_GetLock(skill) : SKILLLOCK_UP;
	}
	WORD Skill_GetAdjusted(SKILL_TYPE skill) const;
	SKILL_TYPE Skill_GetMagicRandom(WORD wMinValue = 0);

	bool Skill_CanUse(SKILL_TYPE skill);
	void Skill_SetBase(SKILL_TYPE skill, WORD wValue);
	bool Skill_UseQuick(SKILL_TYPE skill, int iDifficulty, bool fAllowGain = true, bool fUseBellCurve = true);
	bool Skill_CheckSuccess(SKILL_TYPE skill, int iDifficulty, bool fUseBellCurve = true) const;
	bool Skill_Wait(SKILL_TYPE skill);
	bool Skill_Start(SKILL_TYPE skill);
	void Skill_Fail(bool fCancel = false);
	int Skill_Stroke(bool fResource);
	ANIM_TYPE Skill_GetAnim(SKILL_TYPE skill);
	SOUND_TYPE Skill_GetSound(SKILL_TYPE skill);
	int Skill_Stage(SKTRIG_TYPE stage);
	TRIGRET_TYPE Skill_OnTrigger(SKILL_TYPE skill, SKTRIG_TYPE stage);
	TRIGRET_TYPE Skill_OnTrigger(SKILL_TYPE skill, SKTRIG_TYPE stage, CScriptTriggerArgs *pArgs);		// pArgs.m_iN1 will be rewritten with skill
	TRIGRET_TYPE Skill_OnCharTrigger(SKILL_TYPE skill, CTRIG_TYPE ctrig);
	TRIGRET_TYPE Skill_OnCharTrigger(SKILL_TYPE skill, CTRIG_TYPE ctrig, CScriptTriggerArgs *pArgs);	// pArgs.m_iN1 will be rewritten with skill

	bool Skill_Mining_Smelt(CItem *pItemOre, CItem *pItemTarg);
	bool Skill_Tracking(CGrayUID uidTarg, int iDistMax = SHRT_MAX);
	bool Skill_MakeItem(ITEMID_TYPE id, CGrayUID uidTarg, SKTRIG_TYPE stage, bool fSkillOnly = false, DWORD dwReplicationQty = 1);
	bool Skill_MakeItem_Success();
	bool Skill_Snoop_Check(const CItemContainer *pItem);
	void Skill_Cleanup();

	int SkillResourceTest(const CResourceQtyArray *pResources);

	void Spell_Effect_Remove(CItem *pSpell);
	void Spell_Effect_Add(CItem *pSpell);

private:
	int Skill_Done();
	void Skill_Decay();
	void Skill_Experience(SKILL_TYPE skill, int iDifficulty);

	int Skill_NaturalResource_Setup(CItem *pResBit);
	CItem *Skill_NaturalResource_Create(CItem *pResBit, SKILL_TYPE skill);
	void Skill_SetTimeout();
	INT64 Skill_GetTimeout();

	int	Skill_Scripted(SKTRIG_TYPE stage);

	int Skill_MakeItem(SKTRIG_TYPE stage);
	int Skill_Information(SKTRIG_TYPE stage);
	int Skill_Hiding(SKTRIG_TYPE stage);
	int Skill_Enticement(SKTRIG_TYPE stage);
	int Skill_Snooping(SKTRIG_TYPE stage);
	int Skill_Stealing(SKTRIG_TYPE stage);
	int Skill_Mining(SKTRIG_TYPE stage);
	int Skill_Lumberjack(SKTRIG_TYPE stage);
	int Skill_Taming(SKTRIG_TYPE stage);
	int Skill_Fishing(SKTRIG_TYPE stage);
	int Skill_DetectHidden(SKTRIG_TYPE stage);
	int Skill_Herding(SKTRIG_TYPE stage);
	int Skill_Blacksmith(SKTRIG_TYPE stage);
	int Skill_Lockpicking(SKTRIG_TYPE stage);
	int Skill_Peacemaking(SKTRIG_TYPE stage);
	int Skill_Carpentry(SKTRIG_TYPE stage);
	int Skill_Provocation(SKTRIG_TYPE stage);
	int Skill_Poisoning(SKTRIG_TYPE stage);
	int Skill_Cooking(SKTRIG_TYPE stage);
	int Skill_Healing(SKTRIG_TYPE stage);
	int Skill_Meditation(SKTRIG_TYPE stage);
	int Skill_RemoveTrap(SKTRIG_TYPE stage);
	int Skill_Begging(SKTRIG_TYPE stage);
	int Skill_SpiritSpeak(SKTRIG_TYPE stage);
	int Skill_Magery(SKTRIG_TYPE stage);
	int Skill_Tracking(SKTRIG_TYPE stage);
	int Skill_Fighting(SKTRIG_TYPE stage);
	int Skill_Musicianship(SKTRIG_TYPE stage);

	int Skill_Act_Napping(SKTRIG_TYPE stage);
	int Skill_Act_Throwing(SKTRIG_TYPE stage);
	int Skill_Act_Breath(SKTRIG_TYPE stage);
	int Skill_Act_Training(SKTRIG_TYPE stage);

	void Spell_Dispel(int iLevel);
	CChar *Spell_Summon(CREID_TYPE id, CPointMap ptTarg);
	bool Spell_Recall(CItem *pTarg, bool fGate);
	CItem *Spell_Effect_Create(SPELL_TYPE spell, LAYER_TYPE layer, int iSkillLevel, int iDuration, CObjBase *pSrc = NULL, bool fEquip = true);
	bool Spell_Equip_OnTick(CItem *pItem);

	void Spell_Field(CPointMap ptTarg, ITEMID_TYPE idEW, ITEMID_TYPE idNS, BYTE bFieldWidth, BYTE bFieldGauge, int iSkillLevel, CChar *pCharSrc = NULL, int iDuration = 0, HUE_TYPE wColor = HUE_DEFAULT);
	void Spell_Area(CPointMap ptTarg, int iDist, int iSkillLevel);
	bool Spell_TargCheck_Face();
	bool Spell_TargCheck();
	bool Spell_Unequip(LAYER_TYPE layer);

	int Spell_CastStart();
	void Spell_CastFail();

public:
	bool Spell_CastDone();
	bool OnSpellEffect(SPELL_TYPE spell, CChar *pCharSrc, int iSkillLevel, CItem *pSourceItem, bool fReflecting = false);
	bool Spell_Resurrection(CItemCorpse *pCorpse = NULL, CChar *pCharSrc = NULL, bool fNoFail = false);
	bool Spell_Teleport(CPointMap ptDest, bool fTakePets = false, bool fCheckAntiMagic = true, bool fDisplayEffect = true, ITEMID_TYPE iEffect = ITEMID_NOTHING, SOUND_TYPE iSound = SOUND_NONE);
	bool Spell_CreateGate(CPointMap ptDest, bool fCheckAntiMagic = true);
	bool Spell_CanCast(SPELL_TYPE &spell, bool fTest, CObjBase *pSrc, bool fFailMsg, bool fCheckAntiMagic = true);
	int	GetSpellDuration(SPELL_TYPE spell, int iSkillLevel, CChar *pCharSrc = NULL);

	// Memories about objects in the world
	bool Memory_OnTick(CItemMemory *pMemory);
	bool Memory_UpdateFlags(CItemMemory *pMemory);
	bool Memory_UpdateClearTypes(CItemMemory *pMemory, WORD wMemTypes);
	void Memory_AddTypes(CItemMemory *pMemory, WORD wMemTypes);
	bool Memory_ClearTypes(CItemMemory *pMemory, WORD wMemTypes);
	CItemMemory *Memory_CreateObj(CGrayUID uid, WORD wMemTypes);
	CItemMemory *Memory_CreateObj(const CObjBase *pObj, WORD wMemTypes)
	{
		ASSERT(pObj);
		return Memory_CreateObj(pObj->GetUID(), wMemTypes);
	}

public:
	void Memory_ClearTypes(WORD wMemTypes);
	CItemMemory *Memory_FindObj(CGrayUID uid) const;
	CItemMemory *Memory_FindObj(const CObjBase *pObj) const
	{
		if ( !pObj )
			return NULL;
		return Memory_FindObj(pObj->GetUID());
	}

	CItemMemory *Memory_AddObjTypes(CGrayUID uid, WORD wMemTypes);
	CItemMemory *Memory_AddObjTypes(const CObjBase *pObj, WORD wMemTypes)
	{
		ASSERT(pObj);
		return Memory_AddObjTypes(pObj->GetUID(), wMemTypes);
	}

	CItemMemory *Memory_FindTypes(WORD wMemTypes) const;
	CItemMemory *Memory_FindObjTypes(const CObjBase *pObj, WORD wMemTypes) const
	{
		CItemMemory *pMemory = Memory_FindObj(pObj);
		if ( !pMemory || !pMemory->IsMemoryTypes(wMemTypes) )
			return NULL;
		return pMemory;
	}

public:
	void SoundChar(CRESND_TYPE type);
	void Action_StartSpecial(CREID_TYPE id);

private:
	void OnNoticeCrime(CChar *pCriminal, const CChar *pCharMark);

public:
	bool CheckCrimeSeen(SKILL_TYPE skill, CChar *pCharMark, const CObjBase *pItem, LPCTSTR pszAction);

private:
	// Armor, weapons and combat
	int	CalcFightRange(CItem *pWeapon = NULL);

	bool Fight_IsActive() const;
	bool Fight_IsAttackable() const;

public:
	WORD CalcArmorDefense() const;

	void Memory_Fight_Start(const CChar *pTarg);
	bool Memory_Fight_OnTick(CItemMemory *pMemory);

	bool Fight_Attack(const CChar *pCharTarg, bool bToldByMaster = false);
	void Fight_Clear();
	void Fight_HitTry();
	WAR_SWING_TYPE Fight_Hit(CChar *pCharTarg);
	SKILL_TYPE Fight_GetWeaponSkill() const;
	int Fight_CalcDamage(const CItem *pWeapon, bool bNoRandom = false, bool bGetMax = true) const;

	// Attacker system
	enum ATTACKER_CLEAR_TYPE
	{
		ATTACKER_CLEAR_FORCED,
		ATTACKER_CLEAR_ELAPSED,
		ATTACKER_CLEAR_DISTANCE,
		ATTACKER_CLEAR_REMOVEDCHAR,
		ATTACKER_CLEAR_SCRIPT
	};

	bool Attacker_Add(CChar *pChar, INT64 threat = 0);
	bool Attacker_Delete(int id, bool bForced = false, ATTACKER_CLEAR_TYPE type = ATTACKER_CLEAR_FORCED);
	bool Attacker_Delete(CChar *pChar, bool bForced = false, ATTACKER_CLEAR_TYPE type = ATTACKER_CLEAR_FORCED);
	int Attacker_GetID(CChar *pChar);
	CChar *Attacker_GetHighestDam();
	CChar *Attacker_GetLowestElapsed();
	void Attacker_SetElapsed(int id, INT64 value);
	void Attacker_SetDamage(int id, INT64 value);
	void Attacker_SetThreat(int id, INT64 value);
	void Attacker_CheckTimeout();

	//
	bool Player_OnVerb(CScript &s, CTextConsole *pSrc);
	bool ReadScriptTrig(CCharBase *pCharDef, CTRIG_TYPE trig, bool fVendor = false);
	bool ReadScript(CResourceLock &s, bool fVendor = false);
	void NPC_LoadScript(bool fRestock);
	void NPC_CreateTrigger();

	// Mounting and figurines
	bool Horse_Mount(CChar *pHorse);
	bool Horse_UnMount();

private:
	CItem *Horse_GetMountItem() const;
	CChar *Horse_GetMountChar() const;

public:
	CChar *Use_Figurine(CItem *pItem, bool bCheckFollowerSlots = true);
	CItem *Make_Figurine(CGrayUID uidOwner = (UID_F_ITEM|UID_O_INDEX_MASK), ITEMID_TYPE id = ITEMID_NOTHING);
	CItem *NPC_Shrink();
	bool FollowersUpdate(CChar *pChar, short iFollowerSlots = 0, bool bCheckOnly = false);

	int ItemPickup(CItem *pItem, WORD wAmount);
	bool ItemEquip(CItem *pItem, CChar *pCharMsg = NULL, bool fFromDClick = false);
	bool ItemEquipWeapon(bool fForce);
	bool ItemEquipArmor(bool fForce);
	bool ItemBounce(CItem *pItem, bool bDisplayMsg = true);
	bool ItemDrop(CItem *pItem, const CPointMap &pt);

	void Flip();
	bool SetPoison(int iSkill, int iTicks, CChar *pCharSrc);
	bool SetPoisonCure(int iSkill, bool fExtra);
	bool CheckCorpseCrime(const CItemCorpse *pCorpse, bool fLooting, bool fTest);
	CItemCorpse *FindMyCorpse(bool fIgnoreLOS = false, int iRadius = 2) const;
	CItemCorpse *MakeCorpse(bool fFrontFall);
	bool RaiseCorpse(CItemCorpse *pCorpse);
	bool Death();
	bool Reveal(DWORD dwFlags = 0);
	void Jail(CTextConsole *pSrc, bool fSet, int iCell);
	void EatAnim(LPCTSTR pszName, int iQty);

	void CallGuards();
	void CallGuards(CChar *pCriminal);

	#define DEATH_NOFAMECHANGE		0x01
	#define DEATH_NOCORPSE			0x02
	#define DEATH_NOLOOTDROP		0x04
	#define DEATH_NOCONJUREDEFFECT	0x08
	#define DEATH_HASCORPSE			0x10

	virtual void Speak(LPCTSTR pszText, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL);
	virtual void SpeakUTF8(LPCTSTR pszText, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL, CLanguageID lang = 0);
	virtual void SpeakUTF8Ex(const NWORD *pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang);

	bool OnFreezeCheck();
	void ToggleFlying();
	void DropAll(CItemContainer *pDest = NULL, DWORD dwAttr = 0);
	void UnEquipAllItems(CItemContainer *pDest = NULL, bool bLeaveHands = false);
	void Wake();
	void SleepStart(bool fFrontFall);

	void Guild_Resign(MEMORY_TYPE MemType);
	CItemStone *Guild_Find(MEMORY_TYPE MemType) const;
	CStoneMember *Guild_FindMember(MEMORY_TYPE MemType) const;
	LPCTSTR Guild_Abbrev(MEMORY_TYPE MemType) const;
	LPCTSTR Guild_AbbrevBracket(MEMORY_TYPE MemType) const;

	bool Use_EatQty(CItem *pItem, WORD wQty = 1);
	bool Use_Eat(CItem *pItem, WORD wQty = 1);
	bool Use_MultiLockDown(CItem *pItem);
	bool Use_CarveCorpse(CItemCorpse *pCorpse);
	bool Use_Repair(CItem *pItem);
	int Use_PlayMusic(CItem *pItem, int iDifficultyToPlay);
	bool Use_Drink(CItem *pItem);
	bool Use_Cannon_Feed(CItem *pItem, CItem *pFeed);
	bool Use_Item_Web(CItem *pItem);
	bool Use_MoonGate(CItem *pItem);
	bool Use_Kindling(CItem *pItem);
	bool Use_BedRoll(CItem *pItem);
	bool Use_Seed(CItem *pItem, CPointMap *pPoint);
	bool Use_Key(CItem *pItem, CItem *pItemTarg);
	bool Use_KeyChange(CItem *pItem);
	bool Use_Train_PickPocketDip(CItem *pItem, bool fSetup);
	bool Use_Train_Dummy(CItem *pItem, bool fSetup);
	bool Use_Train_ArcheryButte(CItem *pItem, bool fSetup);

	bool Use_Item(CItem *pItem, bool fLink = false);
	bool Use_Obj(CObjBase *pObj, bool fTestTouch, bool fScript = false);

private:
	int Do_Use_Item(CItem *pItem, bool fLink = false);

private:
	// NPC AI
	static CREID_TYPE NPC_GetAllyGroupType(CREID_TYPE idTest);

	bool NPC_StablePetRetrieve(CChar *pCharPlayer);
	bool NPC_StablePetSelect(CChar *pCharPlayer);

	int NPC_WantThisItem(CItem *pItem) const;
	int NPC_GetWeaponUseScore(CItem *pWeapon);

	int NPC_GetHostilityLevelToward(const CChar *pCharTarg) const;
	int NPC_GetAttackMotivation(CChar *pChar) const;
	bool NPC_CheckHirelingStatus();
	WORD NPC_GetTrainMax(const CChar *pStudent, SKILL_TYPE skill) const;

	bool NPC_OnVerb(CScript &s, CTextConsole *pSrc = NULL);
	void NPC_OnHirePayMore(CItem *pGold, bool bHire = false);

public:
	bool NPC_OnHirePay(CChar *pCharSrc, CItemMemory *pMemory, CItem *pGold);
	bool NPC_OnHireHear(CChar *pCharSrc);
	WORD NPC_OnTrainCheck(CChar *pCharSrc, SKILL_TYPE skill);
	bool NPC_OnTrainPay(CChar *pCharSrc, CItemMemory *pMemory, CItem *pGold);
	bool NPC_OnTrainHear(CChar *pCharSrc, LPCTSTR pszCmd);
	bool NPC_TrainSkill(CChar *pCharSrc, SKILL_TYPE skill, WORD wVal);

private:
	bool NPC_CheckWalkHere(const CPointBase &pt, const CRegionBase *pArea) const;
	void NPC_OnNoticeSnoop(CChar *pCharThief, CChar *pCharMark);

	void NPC_LootMemory(CItem *pItem);
	bool NPC_LookAtCharGuard(CChar *pChar);
	bool NPC_LookAtCharHealer(CChar *pChar);
	bool NPC_LookAtCharHuman(CChar *pChar);
	bool NPC_LookAtCharMonster(CChar *pChar);
	bool NPC_LookAtChar(CChar *pChar);
	bool NPC_LookAtItem(CItem *pItem, int iDist);
	bool NPC_LookAround(bool fForceCheckItems = false);
	int NPC_WalkToPoint(bool fRun = false);
	CChar *NPC_FightFindBestTarget();
	bool NPC_FightMagery(CChar *pChar);
	bool NPC_FightCast(CObjBase *&pTarg, CObjBase *pSrc, SPELL_TYPE &spell, SKILL_TYPE skill = SKILL_NONE);
	bool NPC_FightArchery(CChar *pChar);
	bool NPC_FightMayCast(bool fCheckSkill = true) const;
	void NPC_GetAllSpellbookSpells();

	bool NPC_Act_Follow(bool fFlee = false, int maxDistance = 1, bool fMoveAway = false);
	void NPC_Act_Guard();
	void NPC_Act_GoHome();
	bool NPC_Act_Talk();
	void NPC_Act_Wander();
	void NPC_Act_Fight();
	void NPC_Act_Idle();
	void NPC_Act_Looting();
	void NPC_Act_Flee();
	void NPC_Act_Goto(int iDist = 30);
	void NPC_Act_Runto(int iDist = 30);
	bool NPC_Act_Food();

	void NPC_ActStart_SpeakTo(CChar *pSrc);
	void NPC_OnTickAction();

public:
	void NPC_Pathfinding();
	void NPC_Food();
	void NPC_ExtraAI();
	void NPC_AddSpellsFromBook(CItem *pBook);

	bool NPC_PetCheckAccess(int iCmd, CChar *pChar);
	void NPC_PetConfirmCommand(bool bSuccess, CChar *pMaster);
	void NPC_PetDesert();
	void NPC_PetClearOwners(bool bResendTooltip = true);
	bool NPC_PetSetOwner(CChar *pChar, bool bResendTooltip = true);
	CChar *NPC_PetGetOwner() const;
	bool NPC_IsOwnedBy(const CChar *pChar, bool fAllowGM = true) const;
	bool NPC_CanSpeak() const;

	static CItemVendable *NPC_FindVendableItem(CItemVendable *pVendItem, CItemContainer *pContBuy);

	bool NPC_IsVendor() const
	{
		return (m_pNPC && ((m_pNPC->m_Brain == NPCBRAIN_HEALER) || (m_pNPC->m_Brain == NPCBRAIN_BANKER) || (m_pNPC->m_Brain == NPCBRAIN_VENDOR) || (m_pNPC->m_Brain == NPCBRAIN_STABLE)));
	}

	bool NPC_IsMonster() const
	{
		return (m_pNPC && ((m_pNPC->m_Brain == NPCBRAIN_MONSTER) || (m_pNPC->m_Brain == NPCBRAIN_BERSERK) || (m_pNPC->m_Brain == NPCBRAIN_DRAGON)));
	}

	int NPC_GetAiFlags()
	{
		if ( m_pNPC )
			return m_pNPC->GetNpcAiFlags(this);
		return 0;
	}
	bool NPC_Vendor_Restock(bool bForce = false, bool bFillStock = false);
	int NPC_GetVendorMarkup() const;

	bool NPC_OnHearPetCmd(LPCTSTR pszCmd, CChar *pSrc, bool bAllPets = false);
	bool NPC_OnHearPetCmdTarg(int iCmd, CChar *pSrc, CObjBase *pObj, const CPointMap &pt, LPCTSTR pszArgs);
	size_t NPC_OnHearName(LPCTSTR pszText) const;
	void NPC_OnHear(LPCTSTR pszCmd, CChar *pSrc, bool fAllPets = false);
	bool NPC_OnItemGive(CChar *pCharSrc, CItem *pItem);
	bool NPC_SetVendorPrice(CItem *pItem, int iPrice);
	bool OnTriggerSpeech(bool fPet, LPCTSTR pszText, CChar *pSrc, TALKMODE_TYPE &mode, HUE_TYPE wHue = HUE_DEFAULT);

	// Outside events that occur to us
	int OnTakeDamage(int iDmg, CChar *pSrc, DAMAGE_TYPE uType, int iDmgPhysical = 0, int iDmgFire = 0, int iDmgCold = 0, int iDmgPoison = 0, int iDmgEnergy = 0);
	void OnTakeDamageArea(int iDmg, CChar *pSrc, DAMAGE_TYPE uType, int iDmgPhysical = 0, int iDmgFire = 0, int iDmgCold = 0, int iDmgPoison = 0, int iDmgEnergy = 0, HUE_TYPE effectHue = HUE_DEFAULT, SOUND_TYPE effectSound = SOUND_NONE);
	void OnHarmedBy(CChar *pCharSrc);
	bool OnAttackedBy(CChar *pCharSrc, bool bPetsCommand = false, bool bShouldReveal = true);

	bool OnTickEquip(CItem *pItem);
	void OnTickFood(int iVal, int iHitsHungerLoss);
	void OnTickStatusUpdate();
	bool OnTick();

	static CChar *CreateBasic(CREID_TYPE id);
	static CChar *CreateNPC(CREID_TYPE id);
};

#endif	// _INC_CCHAR_H
