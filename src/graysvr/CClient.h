//
// CClient.h
//

#ifndef _INC_CCLIENT_H
#define _INC_CCLIENT_H
#pragma once

enum CV_TYPE
{
	#define ADD(a,b) CV_##a,
	#include "../tables/CClient_functions.tbl"
	#undef ADD
	CV_QTY
};

enum CC_TYPE
{
	#define ADD(a,b) CC_##a,
	#include "../tables/CClient_props.tbl"
	#undef ADD
	CC_QTY
};

enum CLIMODE_TYPE	// What mode is the client to server connection in? (waiting for input?)
{
	// Setup events ------------------------------------------------------------------
	CLIMODE_SETUP_CONNECTING,
	CLIMODE_SETUP_SERVERS,			// client has received the servers list
	CLIMODE_SETUP_RELAY,			// client has been relayed to the game server. wait for new login
	CLIMODE_SETUP_CHARLIST,			// client has the char list and may (select char, delete char, create new char)

	// Capture the user input for this mode ------------------------------------------
	CLIMODE_NORMAL,					// no targeting going on, we are just walking around, etc

	// Asyc events enum here ---------------------------------------------------------
	CLIMODE_DRAG,					// I'm dragging something (not quite a targeting but similar)
	CLIMODE_DYE,					// the dye dialog is up and I'm targeting something to dye
	CLIMODE_INPVAL,					// special text input dialog (for set item attrib)

	// Some sort of general gump dialog ----------------------------------------------
	CLIMODE_DIALOG,					// from RES_DIALOG

	// Client hardcoded (internal) dialogs
	CLIMODE_DIALOG_VIRTUE			= 0x1CD,
	CLIMODE_DIALOG_FACESELECTION	= 0x2B0,	// enhanced clients only

	// Making a selection from a menu ------------------------------------------------
	CLIMODE_MENU,					// from RES_MENU

	// Hard-coded (internal) menus
	CLIMODE_MENU_SKILL,				// result of some skill (Tracking, Tinkering, Blacksmithing, etc)
	CLIMODE_MENU_SKILL_TRACK_SETUP,
	CLIMODE_MENU_SKILL_TRACK,
	CLIMODE_MENU_EDIT,				// edit the contents of a container

	// Prompting for text input ------------------------------------------------------
	CLIMODE_PROMPT_NAME_RUNE,
	CLIMODE_PROMPT_NAME_KEY,		// naming a key
	CLIMODE_PROMPT_NAME_SIGN,		// naming a house sign
	CLIMODE_PROMPT_NAME_SHIP,
	CLIMODE_PROMPT_GM_PAGE_TEXT,	// allowed to enter text for GM page
	CLIMODE_PROMPT_VENDOR_PRICE,	// set item price on vendor
	CLIMODE_PROMPT_TARG_VERB,		// send message to another player
	CLIMODE_PROMPT_SCRIPT_VERB,		// script verb
	CLIMODE_PROMPT_STONE_NAME,		// prompt for text

	// Targeting mouse cursor --------------------------------------------------------
	CLIMODE_MOUSE_TYPE,				// greater than this = mouse type targeting

	// GM commands
	CLIMODE_TARG_OBJ_SET,			// set some attribute of the item I will show
	CLIMODE_TARG_OBJ_INFO,			// what item do I want props for?
	CLIMODE_TARG_OBJ_FUNC,

	CLIMODE_TARG_UNEXTRACT,			// "UNEXTRACT" command
	CLIMODE_TARG_ADDCHAR,			// "ADDNPC" command
	CLIMODE_TARG_ADDITEM,			// "ADDITEM" command
	CLIMODE_TARG_LINK,				// "LINK" command
	CLIMODE_TARG_TILE,				// "TILE" command

	// Player stuff
	CLIMODE_TARG_SKILL,				// targeting a skill or spell
	CLIMODE_TARG_SKILL_MAGERY,
	CLIMODE_TARG_SKILL_HERD_DEST,
	CLIMODE_TARG_SKILL_POISON,
	CLIMODE_TARG_SKILL_PROVOKE,

	CLIMODE_TARG_USE_ITEM,			// target for using the selected item
	CLIMODE_TARG_PET_CMD,			// targeted pet command
	CLIMODE_TARG_PET_STABLE,		// pick a creature to stable
	CLIMODE_TARG_REPAIR,			// attempt to repair an item
	CLIMODE_TARG_STONE_RECRUIT,		// recruit members for a stone
	CLIMODE_TARG_STONE_RECRUITFULL,	// recruit/make a member and set abbrev show
	CLIMODE_TARG_PARTY_ADD,
	CLIMODE_TARG_PARTY_REMOVE,
	CLIMODE_TARG_GLOBALCHAT_ADD,

	CLIMODE_TARG_QTY
};

enum LOGINFLAGS_TYPE		// Client send these flags on login, based on client version/expansion
{
	None		= 0x0,
	Felucca		= 0x1,
	Trammel		= 0x2,
	Ilshenar	= 0x4,
	Malas		= 0x8,
	Tokuno		= 0x10,
	TerMur		= 0x20,
	ClientEC	= 0x40,
	Unk			= 0x80,		// unknown, only sent by enhanced clients
	Client3D	= 0x100
};

enum BUFF_ICONS
{
	BI_NOREMOUNT = 0x3E9,
	BI_NOREARM,
	BI_NIGHTSIGHT = 0x3ED,
	BI_DEATHSTRIKE,
	BI_EVILOMEN,
	BI_HEALINGTHROTTLE,
	BI_STAMINATHROTTLE,
	BI_DIVINEFURY,
	BI_ENEMYOFONE,
	BI_HIDDEN,
	BI_ACTIVEMEDITATION,
	BI_BLOODOATHCASTER,
	BI_BLOODOATHCURSE,
	BI_CORPSESKIN,
	BI_MINDROT,
	BI_PAINSPIKE,
	BI_STRANGLE,
	BI_GIFTOFRENEWAL,
	BI_ATTUNEWEAPON,
	BI_THUNDERSTORM,
	BI_ESSENCEOFWIND,
	BI_ETHEREALVOYAGE,
	BI_GIFTOFLIFE,
	BI_ARCANEEMPOWERMENT,
	BI_MORTALSTRIKE,
	BI_REACTIVEARMOR,
	BI_PROTECTION,
	BI_ARCHPROTECTION,
	BI_MAGICREFLECTION,
	BI_INCOGNITO,
	BI_DISGUISED,
	BI_ANIMALFORM,
	BI_POLYMORPH,
	BI_INVISIBILITY,
	BI_PARALYZE,
	BI_POISON,
	BI_BLEED,
	BI_CLUMSY,
	BI_FEEBLEMIND,
	BI_WEAKEN,
	BI_CURSE,
	BI_MASSCURSE,
	BI_AGILITY,
	BI_CUNNING,
	BI_STRENGTH,
	BI_BLESS,
	BI_SLEEP,
	BI_STONEFORM,
	BI_SPELLPLAGUE,
	BI_GARGOYLEBERSERK,
	BI_GARGOYLEFLY = 0x41E,
	BI_INSPIRE,
	BI_INVIGORATE,
	BI_RESILIENCE,
	BI_PERSEVERANCE,
	BI_TRIBULATIONDEBUFF,
	BI_DESPAIR,
	BI_FISHPIE = 0x426,
	BI_HITLOWERATTACK,
	BI_HITLOWERDEFENSE,
	BI_HITDUALWIELD,
	BI_BLOCK,
	BI_DEFENSEMASTERY,
	BI_DESPAIRDEBUFF,
	BI_HEALINGEFFECT,
	BI_SPELLFOCUSING,
	BI_SPELLFOCUSINGDEBUFF,
	BI_RAGEFOCUSINGDEBUFF,
	BI_RAGEFOCUSING,
	BI_WARDING,
	BI_TRIBULATION,
	BI_FORCEARROW,
	BI_DISARM,
	BI_SURGE,
	BI_FEINT,
	BI_TALONSTRIKE,
	BI_PHYSICATTACK,
	BI_CONSECRATE,
	BI_GRAPESOFWRATH,
	BI_ENEMYOFONEDEBUFF,
	BI_HORRIFICBEAST,
	BI_LICHFORM,
	BI_VAMPIRICEMBRACE,
	BI_CURSEWEAPON,
	BI_REAPERFORM,
	BI_INMOLATINGWEAPON,
	BI_ENCHANT,
	BI_HONORABLEEXECUTION,
	BI_CONFIDENCE,
	BI_EVASION,
	BI_COUNTERATTACK,
	BI_LIGHTNINGSTRIKE,
	BI_MOMENTUMSTRIKE,
	BI_ORANGEPETALS,
	BI_ROSEOFTRINSIC,
	BI_POISONIMMUNITY,
	BI_VETERINARY,
	BI_PERFECTION,
	BI_HONORED,
	BI_MANAPHASE,
	BI_FANDANCERFANFIRE,
	BI_RAGE,
	BI_WEBBING,
	BI_MEDUSASTONE,
	BI_DRAGONSLASHERFEAR,
	BI_AURAOFNAUSEA,
	BI_HOWLOFCACOPHONY,
	BI_GAZEDESPAIR,
	BI_HIRYUPHYSICALRESISTANCE,
	BI_RUNEBEETLECORRUPTION,
	BI_BLOODWORMANEMIA,
	BI_ROTWORMBLOODDISEASE,
	BI_SKILLUSEDELAY,
	BI_FACTIONSTATLOSS,
	BI_HEATOFBATTLE,
	BI_CRIMINALSTATUS,
	BI_ARMORPIERCE,
	BI_SPLINTERINGEFFECT,
	BI_SWINGSPEEDDEBUFF,
	BI_WRAITHFORM,
	BI_CITYTRADEDEAL = 0x466,
	BI_HUMILITYDEBUFF,
	BI_SPIRITUALITY,
	BI_HUMILITY,
	BI_RAMPAGE,
	BI_STAGGERDEBUFF,
	BI_TOUGHNESS,
	BI_THRUST,
	BI_PIERCEDEBUFF,
	BI_PLAYINGTHEODDS,
	BI_FOCUSEDEYE,
	BI_ONSLAUGHTDEBUFF,
	BI_ELEMENTALFURY,
	BI_ELEMENTALFURYDEBUFF,
	BI_CALLEDSHOT,
	BI_KNOCKOUT,
	BI_WARRIORSGIFTS,		// previously known as Saving Throw
	BI_CONDUIT,
	BI_ETHEREALBURST,
	BI_MYSTICWEAPON,
	BI_MANASHIELD,
	BI_ANTICIPATEHIT,
	BI_WARCRY,
	BI_SHADOW,
	BI_WHITETIGERFORM,
	BI_BODYGUARD,
	BI_HEIGHTENEDSENSES,
	BI_TOLERANCE,
	BI_DEATHRAY,
	BI_DEATHRAYDEBUFF,
	BI_INTUITION,
	BI_ENCHANTEDSUMMONING,
	BI_SHIELDBASH,
	BI_WHISPERING,
	BI_COMBATTRAINING,
	BI_INJECTEDSTRIKEDEBUFF,
	BI_INJECTEDSTRIKE,
	BI_UNKNOWNTOMATO,
	BI_PLAYINGTHEODDSDEBUFF,
	BI_DRAGONTURTLEDEBUFF,
	BI_BOARDING,
	BI_POTENCY,
	BI_THRUSTDEBUFF,
	BI_FISTSOFFURY,
	BI_BARRABHEMOLYMPHCONCENTRATE,
	BI_JUKARIBURNPOULTICE,
	BI_KURAKAMBUSHERSESSENCE,
	BI_BARAKODRAFTOFMIGHT,
	BI_URALITRANCETONIC,
	BI_SAKKHRAPROPHYLAXIS,
	BI_SPARKSDEBUFF,
	BI_SWARMDEBUFF,
	BI_BONEBREAKERDEBUFF,
	BI_SPARKS,
	BI_SWARM,
	BI_BONEBREAKER,
	BI_STATUETRANSFORMATION,
	BI_CORASMANARAGE,
	BI_VIRTUESHIELD,
	BI_FEINTDEBUFF,
	BI_CADDELLITEINFUSED,
	BI_POTIONOFGLORIOUSFORTUNE,
	BI_MYSTICALPOLYMORPHTOTEM,
	BI_DISCORDANCEDEBUFF
};

struct CSectorEnviron		// When these change it is an CTRIG_EnvironChange
{
	#define LIGHT_OVERRIDE 0x80
public:
	BYTE m_Light;				// calculated light level in this sector (|0x80 = override)
	SEASON_TYPE m_Season;		// season in this sector
	WEATHER_TYPE m_Weather;		// weather in this sector

public:
	CSectorEnviron()
	{
		m_Light = LIGHT_BRIGHT;	// set based on time later
		m_Season = SEASON_Summer;
		m_Weather = WEATHER_Clear;
	}
	void SetInvalid()
	{
		// We changed location by teleport etc. Force a resync of all this
		m_Light = UCHAR_MAX;	// set based on time later
		m_Season = SEASON_Summer;
		m_Weather = WEATHER_Default;
	}
};

struct CMenuItem	// Describe a menu item
{
public:
	WORD m_id;		// ITEMID_TYPE in base set
	WORD m_color;
	CGString m_sText;

public:
	bool ParseLine(TCHAR *pszArgs, CScriptObj *pObjBase, CTextConsole *pSrc);
};

class CClientTooltip;
class CDialogResponseArgs;
class CGMPage;
class CItemBase;
class CItemContainer;
class CItemMap;
class CItemMultiCustom;
class NetState;
class Packet;
class PacketDisplayPopup;
class PacketSend;
class PacketServerRelay;
struct VendorItem;

class CClient : public CGObListRec, public CScriptObj, public CChatMember, public CGlobalChat, public CTextConsole
{
	// TCP/IP connection to the player or console
public:
	explicit CClient(NetState *state);
	~CClient();

	static const char *m_sClassName;
	static LPCTSTR const sm_szRefKeys[];
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];

private:
	static LPCTSTR const sm_szCmd_Redirect[];		// default to redirect these

private:
	CChar *m_pChar;

	// Client last know state stuff
	CSectorEnviron m_Env;		// cache of last environment info sent to avoid server keep resending it to client when its not changed

	BYTE m_fUpdateStats;		// update char status when done with the cycle

	// Walk limiting code
	int	m_iWalkTimeAvg;
	int m_iWalkStepCount;		// count the actual steps (turning does not count)
	ULONGLONG m_timeWalkStep;	// the last %8 walk step time

	// Screensize
	struct __screensize
	{
		WORD x;
		WORD y;
	} m_ScreenSize;

	// Promptconsole
	CLIMODE_TYPE m_Prompt_Mode;	// type
	CGrayUID m_Prompt_Uid;		// context uid
	CGString m_Prompt_Text;		// text (i.e. callback function)

public:
	CONNECT_TYPE m_iConnectType;	// connection type
	NetState *m_NetState;
	CAccount *m_pAccount;

	DWORD m_FeatureFlags;			// features enabled on this client
	DWORD m_CharacterListFlags;		// character list features enabled on this client
	bool m_TooltipEnabled;			// is tooltip feature enabled on this client?
	bool m_ContainerGridEnabled;	// is container grid feature enabled on this client?
	bool m_UseNewChatSystem;		// is this client compatible with new SA+ chat system?

	CServTime m_timeLogin;					// world clock of login time ("LASTCONNECTTIME")
	CServTime m_timeLastEvent;				// last time client sent an event
	CServTime m_timeLastEventItemPickup;	// last time client sent item pickup request (used to prevent fastloot)
	CServTime m_timeLastEventWarMode;		// last time client sent warmode request (used to prevent excessive warmode requests)
	CServTime m_timeLastEventWalk;			// last time client sent walk request (used to handle STATF_Fly char flag)
	UINT64 m_timeNextEventWalk;				// next time client can send walk request (used to prevent fastwalk)

	// GM stuff
	CGMPage *m_pGMPage;			// current GM page being handled by this client
	CGrayUID m_Prop_UID;		// the object of /props (used for skills list as well!)

	// Gump stuff
	typedef std::map<int, int> OpenedGumpsMap_t;
	OpenedGumpsMap_t m_mapOpenedGumps;

	// Throwing weapons stuff (used to play weapon returning anim after throw it)
	CServTime m_timeLastSkillThrowing;		// last time we throw the weapon
	CObjBase *m_pSkillThrowingTarg;			// object from where the anim will return from
	ITEMID_TYPE m_SkillThrowingAnimID;		// weapon anim ID ("AMMOANIM")
	DWORD m_SkillThrowingAnimHue;			// weapon anim hue ("AMMOANIMHUE")
	DWORD m_SkillThrowingAnimRender;		// weapon anim render ("AMMOANIMRENDER")

	// Current operation context args for modal async operations
private:
	CLIMODE_TYPE m_Targ_Mode;	// type of async operation under way

public:
	CGrayUID m_Targ_Last;		// the last object targeted by the client
	CGrayUID m_Targ_UID;		// the object of interest to apply to the target
	CGrayUID m_Targ_PrvUID;		// the object of interest before this
	CGString m_Targ_Text;		// text transfered up from client
	CPointMap m_Targ_p;			// for script targeting
	CServTime m_Targ_Timeout;	// target timeout

	// Context of the targetting setup (depends on m_Targ_Mode)
	union
	{
		// CLIMODE_SETUP_CHARLIST
		CGrayUIDBase m_tmSetupCharList[MAX_CHARS_PER_ACCT];

		// CLIMODE_MENU_*
		// CLIMODE_MENU_SKILL
		struct
		{
			CGrayUIDBase m_UID;
			RESOURCE_ID_BASE m_ResourceID;	// what menu is this?
			DWORD m_Item[MAX_MENU_ITEMS];	// this saves the inrange tracking targets or other context
		} m_tmMenu;

		// CLIMODE_TARG_PET_CMD
		struct
		{
			int	m_iCmd;
			bool m_fAllPets;
		} m_tmPetCmd;

		// CLIMODE_TARG_TILE
		// CLIMODE_TARG_UNEXTRACT
		struct
		{
			CPointBase m_ptFirst;	// multi-stage targetting
			int m_Code;
			int m_id;
		} m_tmTile;

		// CLIMODE_TARG_ADDCHAR
		// CLIMODE_TARG_ADDITEM
		struct
		{
			int m_id;
			WORD m_amount;
		} m_tmAdd;

		// CLIMODE_TARG_SKILL
		struct
		{
			SKILL_TYPE m_Skill;		// targetting what spell?
		} m_tmSkillTarg;

		// CLIMODE_TARG_SKILL_MAGERY
		struct
		{
			SPELL_TYPE m_Spell;		// targetting what spell?
			CREID_TYPE m_SummonID;
		} m_tmSkillMagery;

		// CLIMODE_TARG_USE_ITEM
		struct
		{
			CGObList *m_pParent;	// parent of the item being targetted
		} m_tmUseItem;
	};

private:
	// Encryption stuff
	CCrypt m_Crypt;			// client source communications are always encrypted
	static CHuffman m_Comp;

private:
	bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);

	bool OnRxConsoleLoginComplete();
	bool OnRxConsole(const BYTE *pData, size_t iLen);
	bool OnRxAxis(const BYTE *pData, size_t iLen);
	bool OnRxPing(const BYTE *pData, size_t iLen);
	bool OnRxWebPageRequest(BYTE *pRequest, size_t iLen);

	BYTE LogIn(LPCTSTR pszAccount, LPCTSTR pszPassword, CGString &sMsg);
	BYTE LogIn(CAccount *pAccount, CGString &sMsg);
	bool CanInstantLogOut() const;
	void Announce(bool fArrive) const;

	// GM stuff
	bool OnTarg_Obj_Set(CObjBase *pObj);
	bool OnTarg_Obj_Info(CObjBase *pObj, const CPointMap &pt, ITEMID_TYPE id);
	bool OnTarg_Obj_Function(CObjBase *pObj, const CPointMap &pt, ITEMID_TYPE id);

	bool OnTarg_UnExtract(CObjBase *pObj, const CPointMap &pt);
	bool OnTarg_Stone_Recruit(CChar *pChar, bool fFull = false);
	bool OnTarg_Char_Add(CObjBase *pObj, const CPointMap &pt);
	bool OnTarg_Item_Add(CObjBase *pObj, CPointMap &pt);
	bool OnTarg_Item_Link(CObjBase *pObj);
	bool OnTarg_Tile(CObjBase *pObj, const CPointMap &pt);

	// Player stuff
	bool OnTarg_Use_Deed(CItem *pDeed, CPointMap &pt);
	bool OnTarg_Use_Item(CObjBase *pObjTarg, CPointMap &pt, ITEMID_TYPE id);
	bool OnTarg_Party_Add(CChar *pChar);
	bool OnTarg_Party_Remove(CChar *pChar);
	bool OnTarg_GlobalChat_Add(CChar *pChar);
	CItem *OnTarg_Use_Multi(const CItemBase *pItemDef, CPointMap &pt, DWORD dwAttr, HUE_TYPE wHue);

	int OnSkill_AnimalLore(CGrayUID uid, int iSkillLevel, bool fTest);
	int OnSkill_Anatomy(CGrayUID uid, int iSkillLevel, bool fTest);
	int OnSkill_Forensics(CGrayUID uid, int iSkillLevel, bool fTest);
	int OnSkill_EvalInt(CGrayUID uid, int iSkillLevel, bool fTest);
	int OnSkill_ArmsLore(CGrayUID uid, int iSkillLevel, bool fTest);
	int OnSkill_ItemID(CGrayUID uid, int iSkillLevel, bool fTest);
	int OnSkill_TasteID(CGrayUID uid, int iSkillLevel, bool fTest);

	bool OnTarg_Pet_Command(CObjBase *pObj, const CPointMap &pt);
	bool OnTarg_Pet_Stable(CChar *pCharPet);

	// Commands from client
	void Event_Talk_Common(TCHAR *szText);	// PC speech
	bool Event_Command(LPCTSTR pszCommand, TALKMODE_TYPE mode = TALKMODE_SYSTEM);	// client entered a '/' command like /ADD

public:
	bool OnTarg_Skill(CObjBase *pObj);
	bool OnTarg_Skill_Provoke(CObjBase *pObj);
	bool OnTarg_Skill_Poison(CObjBase *pObj);
	bool OnTarg_Skill_Herd_Dest(CObjBase *pObj, const CPointMap &pt);
	bool OnTarg_Skill_Magery(CObjBase *pObj, const CPointMap &pt);

	void GetAdjustedCharID(const CChar *pChar, CREID_TYPE &id, HUE_TYPE &wHue) const;
	void GetAdjustedItemID(const CChar *pChar, const CItem *pItem, ITEMID_TYPE &id, HUE_TYPE &wHue) const;

	void Event_Attack(CGrayUID uid);
	void Event_Book_Title(CItem *pItem, LPCTSTR pszTitle, LPCTSTR pszAuthor);
	void Event_BugReport(const TCHAR *pszText, int len, BUGREPORT_TYPE type, CLanguageID lang = 0);
	void Event_CharRename(CChar *pChar, LPCTSTR pszName);
	void Event_ChatButton(const NCHAR *pszName = NULL);
	void Event_ChatText(const NCHAR *pszText, int len, CLanguageID lang = 0);
	void Event_CombatAbilitySelect(DWORD dwAbility);
	void Event_CombatMode(bool fWar);
	bool Event_DoubleClick(CGrayUID uid, bool fMacro, bool fTestTouch, bool fScript = false);
	void Event_ExtCmd(EXTCMD_TYPE type, TCHAR *pszArgs);
	void Event_Item_Drop(CItem *pItem, CPointMap pt, CGrayUID uidOn, BYTE gridIndex = 0);
	void Event_Item_Drop_Fail(CItem *pItem);
	void Event_Item_Dye(CGrayUID uid, HUE_TYPE wHue);
	void Event_Item_Pickup(CGrayUID uid, int amount);
	void Event_MailMsg(CChar *pChar);
	void Event_Profile(bool fWrite, CChar *pChar, TCHAR *pszText);
	void Event_PromptResp(LPCTSTR pszText, size_t iTextLen, CGrayUID uidChar, CGrayUID uidPrompt, DWORD dwType);
	void Event_PromptResp_GMPage(LPCTSTR pszReason);
	void Event_SingleClick(CGrayUID uid);
	void Event_Skill_Use(SKILL_TYPE skill);
	void Event_Talk(LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, bool bNoStrip = false);
	void Event_TalkUNICODE(NWORD *wszText, int iTextLen, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, LPCTSTR pszLang);
	void Event_Target(CLIMODE_TYPE context, CGrayUID uid, CPointMap pt, BYTE flags = 0, ITEMID_TYPE id = ITEMID_NOTHING);
	void Event_Tips(WORD i);
	void Event_ToolTip(CGrayUID uid);
	void Event_UseToolbar(BYTE bType, DWORD dwArg);
	void Event_VendorBuy(CChar *pVendor, const VendorItem *items, size_t itemCount);
	void Event_VendorBuy_Cheater(int iCode = 0);
	void Event_VendorSell(CChar *pVendor, const VendorItem *items, size_t itemCount);
	void Event_VendorSell_Cheater(int iCode = 0);
	void Event_VirtueSelect(DWORD dwVirtue, CChar *pCharTarg);
	bool Event_Walk(BYTE rawdir, BYTE sequence = 0);
	bool Event_CheckWalkBuffer();

	TRIGRET_TYPE Menu_OnSelect(RESOURCE_ID_BASE rid, int iSelect, CObjBase *pObj);
	TRIGRET_TYPE Dialog_OnButton(RESOURCE_ID_BASE rid, DWORD dwButtonID, CObjBase *pObj, CDialogResponseArgs *pArgs);

	bool Login_Relay(WORD wRelay);		// relay player to a certain IP
	BYTE Login_ServerList(LPCTSTR pszAccount, LPCTSTR pszPassword);		// initial login (Login on "loginserver", new format)

	BYTE Setup_FillCharList(Packet *pPacket);	// write character list to packet
	BYTE Setup_ListReq(LPCTSTR pszAccount, LPCTSTR pszPassword, bool fTest);	// gameserver login and character listing
	BYTE Setup_Delete(DWORD dwSlot);	// delete character
	BYTE Setup_Play(DWORD dwSlot);		// after hitting "Play Character" button
	BYTE Setup_Start(CChar *pChar);		// send character startup stuff to player

	// Translated commands
private:
	int Cmd_Extract(CScript *pScript, CRectMap &rect, signed char &zLowest);
	size_t Cmd_Skill_Menu_Build(RESOURCE_ID_BASE rid, int iSelect, CMenuItem *item, size_t iMaxSize, bool &fShowMenu, bool &fLimitReached);

public:
	bool Cmd_Skill_Menu(RESOURCE_ID_BASE rid, int iSelect = -1);
	bool Cmd_Skill_Smith(CItem *pIngots);
	bool Cmd_Skill_Magery(SPELL_TYPE iSpell, CObjBase *pSrc);
	bool Cmd_Skill_Tracking(WORD wTrackType = USHRT_MAX, bool bExec = false);
	bool Cmd_Skill_Inscription();
	bool Cmd_SecureTrade(CChar *pChar, CItem *pItem);
	bool Cmd_Control(CChar *pChar);

public:
	long GetSocketID() const;			// get socket id
	CSocketAddress &GetPeer() const;	// get peer address
	LPCTSTR GetPeerStr() const;			// get string representation of the peer address

public:
	void CharDisconnect();

	CClient *GetNext() const
	{
		return static_cast<CClient *>(CGObListRec::GetNext());
	}

	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);	// execute script type command on me
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	virtual bool r_LoadVal(CScript &s);

	// Low level message traffic
	static size_t xCompress(BYTE *pOutput, const BYTE *pInput, size_t iInputLen);

	bool xProcessClientSetup(CEvent *pEvent, size_t iLen);
	bool xPacketFilter(const BYTE *pData, size_t iLen = 0);
	bool xOutPacketFilter(const BYTE *pData, size_t iLen = 0);
	bool xCanEncLogin(bool bCheckCliver = false);	// login crypt check

	// Low level push world data to the client
	bool addRelay(const CServerDef *pServ);
	bool addLoginErr(BYTE bCode);

	#define SF_UPDATE_HITS		0x1
	#define SF_UPDATE_MANA		0x2
	#define SF_UPDATE_STAM		0x4
	#define SF_UPDATE_STATUS	0x8

	void addUpdateStatsFlag()
	{
		m_fUpdateStats |= SF_UPDATE_STATUS;
	}
	void addUpdateHitsFlag()
	{
		m_fUpdateStats |= SF_UPDATE_HITS;
	}
	void addUpdateManaFlag()
	{
		m_fUpdateStats |= SF_UPDATE_MANA;
	}
	void addUpdateStamFlag()
	{
		m_fUpdateStats |= SF_UPDATE_STAM;
	}

	void UpdateStats();
	void UpdateFeatureFlags();
	void UpdateCharacterListFlags();
	bool addDeleteErr(BYTE bCode, DWORD dwSlot);
	void addSeason(SEASON_TYPE season);
	void addTime(bool fCurrent = false);
	void addObjectRemoveCantSee(CGrayUID uid, LPCTSTR pszName = NULL);
	void closeContainer(const CObjBase *pObj);
	void closeUIWindow(const CChar *pChar, DWORD dwCmd);
	void addObjectRemove(CGrayUID uid);
	void addObjectRemove(const CObjBase *pObj);
	void addRemoveAll(bool fItems, bool fChars);

	void addItem_OnGround(CItem *pItem);
	void addItem_Equipped(const CItem *pItem);
	void addItem_InContainer(const CItem *pItem);
	void addItem(CItem *pItem);

	void addBuff(const BUFF_ICONS IconID, const DWORD dwClilocOne, const DWORD dwClilocTwo, const WORD wTime = 0, LPCTSTR *pszArgs = 0, size_t iArgCount = 0);
	void removeBuff(const BUFF_ICONS IconID);
	void resendBuffs();

	void addOpenGump(const CObjBase *pContainer, GUMP_TYPE gump);
	void addContents(const CItemContainer *pContainer, bool fCorpseEquip = false, bool fCorpseFilter = false, bool fShop = false);
	bool addContainerSetup(const CItemContainer *pContainer);

	void addPlayerStart(CChar *pChar);
	void addPlayerSee(const CPointMap &ptOld, bool fIgnoreSelfRegion = false);
	void addPlayerView(const CPointMap &ptOld, bool fFull = true);
	void addPlayerWarMode();

	void addCharMove(const CChar *pChar);
	void addCharMove(const CChar *pChar, BYTE bDir);
	void addChar(const CChar *pChar);
	void addCharName(const CChar *pChar);
	void addItemName(const CItem *pItem);

	void addKick(CTextConsole *pSrc, bool fBlock = true);
	void addWeather(WEATHER_TYPE weather = WEATHER_Default);
	void addLight();
	void addMusic(MIDI_TYPE id);
	void addArrowQuest(WORD x, WORD y, DWORD id);
	void addEffect(EFFECT_TYPE motion, const CObjBaseTemplate *pSrc, CPointMap ptSrc, const CObjBaseTemplate *pDest, CPointMap ptDest, ITEMID_TYPE id, BYTE bSpeed = 0, BYTE bFrames = 0, bool fExplode = false, DWORD dwColor = 0, DWORD dwRender = 0, WORD wEffectID = 0, WORD wExplodeID = 0, WORD wExplodeSound = 0, DWORD dwItemUID = 0, BYTE bLayer = 0);
	void addSound(SOUND_TYPE id, const CObjBaseTemplate *pSrc = NULL, BYTE bRepeat = 1);
	void addReSync();
	void addMap();
	void addMapDiff();
	void addMapWaypoint(CObjBase *pObj, MAPWAYPOINT_TYPE type);
	void addChangeServer();
	void addPlayerUpdate();

	void addBark(LPCTSTR pszText, const CObjBaseTemplate *pSrc, HUE_TYPE wHue = HUE_DEFAULT, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD);
	void addBarkUNICODE(const NCHAR *pszText, const CObjBaseTemplate *pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang = 0);
	void addBarkLocalized(DWORD dwClilocID, const CObjBaseTemplate *pSrc, HUE_TYPE wHue = HUE_DEFAULT, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD, LPCTSTR pszArgs = NULL);
	void addBarkLocalizedEx(DWORD dwClilocID, const CObjBaseTemplate *pSrc, HUE_TYPE wHue = HUE_DEFAULT, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD, AFFIX_TYPE affix = AFFIX_APPEND, LPCTSTR pszAffix = NULL, LPCTSTR pszArgs = NULL);
	void addBarkParse(LPCTSTR pszText, const CObjBaseTemplate *pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font = FONT_NORMAL, bool fUnicode = false);
	void addSysMessage(LPCTSTR pszMsg);
	void addObjMessage(LPCTSTR pszMsg, const CObjBaseTemplate *pSrc, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_OBJ);

	void addCodexOfWisdom(DWORD dwTopicID, bool fForceOpen = false);
	void addDyeOption(const CObjBase *pObj);
	void addWebLaunch(LPCTSTR pszURL);

	void addPromptConsole(CLIMODE_TYPE mode, LPCTSTR pszPrompt, CGrayUID context1 = UID_CLEAR, CGrayUID context2 = UID_CLEAR, bool fUnicode = false);
	void addTarget(CLIMODE_TYPE mode, LPCTSTR pszPrompt, bool fAllowGround = false, bool fCheckCrime = false, int iTimeout = 0);
	void addTargetDeed(const CItem *pDeed);
	bool addTargetItems(CLIMODE_TYPE mode, ITEMID_TYPE id, HUE_TYPE wHue = HUE_DEFAULT, bool fAllowGround = true);
	bool addTargetChars(CLIMODE_TYPE mode, CREID_TYPE id, bool fCheckCrime, int iTimeout = 0);
	void addTargetVerb(LPCTSTR pszCmd, LPCTSTR pszArgs);
	void addTargetFunctionMulti(LPCTSTR pszFunction, ITEMID_TYPE itemid, HUE_TYPE wHue = HUE_DEFAULT, bool fAllowGround = true);
	void addTargetFunction(LPCTSTR pszFunction, bool fAllowGround, bool fCheckCrime);
	void addTargetCancel();
	void addPromptConsoleFunction(LPCTSTR pszFunction, LPCTSTR pszArgs, bool fUnicode = false);

	void addScrollScript(CResourceLock &s, SCROLL_TYPE type, DWORD dwContext = 0, LPCTSTR pszHeader = NULL);
	void addScrollResource(LPCTSTR pszName, SCROLL_TYPE type, DWORD dwScrollID = 0);

	void addVendorClose(const CChar *pVendor);
	void addShopMenuBuy(CChar *pVendor);
	void addShopMenuSell(CChar *pVendor);
	void addBankOpen(CChar *pChar, LAYER_TYPE layer = LAYER_BANKBOX);

	void addSpellbookOpen(CItem *pBook);
	void addCustomSpellbookOpen(CItem *pBook, GUMP_TYPE gumpID);
	bool addBookOpen(CItem *pBook);
	void addBookPage(const CItem *pBook, WORD wPage, WORD wCount);
	void addHealthBarInfo(CObjBase *pObj, bool fRequested = false);
	void addHitsUpdate(CChar *pChar);
	void addManaUpdate(CChar *pChar);
	void addStamUpdate(CChar *pChar);
	void addHealthBarUpdate(const CChar *pChar);
	void addBondedStatus(const CChar *pChar, bool fGhost);
	void addSkillWindow(SKILL_TYPE skill, bool fFromInfo = false);
	void addBulletinBoard(const CItemContainer *pBoard);
	bool addBBoardMessage(const CItemContainer *pBoard, BULLETINBOARD_TYPE action, CGrayUID uidMsg);

	void addToolTip(const CObjBase *pObj, LPCTSTR pszText);
	void addDrawMap(CItemMap *pMap);
	void addMapMode(CItemMap *pMap, MAPCMD_TYPE iType, bool fEdit = false);

	void addGumpTextDisp(const CObjBase *pObj, GUMP_TYPE gump, LPCTSTR pszName, LPCTSTR pszText);
	void addGumpInpVal(bool fCancel, INPVAL_TYPE type, DWORD iMaxLength, LPCTSTR pszText1, LPCTSTR pszText2, CObjBase *pObj);

	void addItemMenu(CLIMODE_TYPE mode, const CMenuItem *item, size_t count, CObjBase *pObj = NULL);
	void addGumpDialog(CLIMODE_TYPE mode, const CGString *psControls, size_t iControls, const CGString *psText, size_t iTexts, DWORD x, DWORD y, CObjBase *pObj = NULL, DWORD rid = 0);

	bool addGumpDialogProps(CObjBase *pObj);

	void addLoginComplete();
	void addChatSystemMessage(CHATMSG_TYPE type, LPCTSTR pszName1 = NULL, LPCTSTR pszName2 = NULL, CLanguageID lang = 0);

	void addGlobalChatConnect();
	void addGlobalChatStatusToggle();

	void addCharPaperdoll(const CChar *pChar);

	void addAOSTooltip(const CObjBase *pObj, bool fRequested = false, bool fShop = false);

private:
	#define POPUPFLAG_DISABLED			0x01
	#define POPUPFLAG_ARROW				0x02	// classic clients only
	#define POPUPFLAG_HIGHLIGHT			0x04	// enhanced clients only
	#define POPUPFLAG_COLOR				0x20	// classic clients only (< 6.0.0.0)

	#define POPUP_OPEN_PAPERDOLL		11
	#define POPUP_OPEN_BACKPACK			12
	#define POPUP_PARTY_ADD				13
	#define POPUP_PARTY_REMOVE			14
	#define POPUP_TRADE_ALLOW			15
	#define POPUP_TRADE_REFUSE			16
	#define POPUP_TRADE_OPEN			17
	#define POPUP_GLOBALCHAT_ALLOW		18
	#define POPUP_GLOBALCHAT_REFUSE		19
	#define POPUP_OPEN_BANKBOX			21
	#define POPUP_BUY					31
	#define POPUP_SELL					32
	#define POPUP_TAME					40
	#define POPUP_PETCMD_GUARD			41
	#define POPUP_PETCMD_FOLLOW			42
	#define POPUP_PETCMD_DROP			43
	#define POPUP_PETCMD_KILL			44
	#define POPUP_PETCMD_STOP			45
	#define POPUP_PETCMD_STAY			46
	#define POPUP_PETCMD_FRIEND_ADD		47
	#define POPUP_PETCMD_FRIEND_REMOVE	48
	#define POPUP_PETCMD_TRANSFER		49
	#define POPUP_PETCMD_RELEASE		50
	#define POPUP_STABLE_PET			51
	#define POPUP_CLAIM_PETS			52
	#define POPUP_TRAIN_SKILL			10000

	PacketDisplayPopup *m_pPopupPacket;

public:
	void Event_AOSPopupMenuSelect(CGrayUID uid, WORD EntryTag);
	void Event_AOSPopupMenuRequest(CGrayUID uid);


	void addShowDamage(CGrayUID uid, int iDamage);
	void addSpeedMode(BYTE bMode = 0);
	void addVisualRange(BYTE bRange);
	void addIdleWarning(BYTE bCode);
	void addKRToolbar(bool fEnable);

	void SendPacket(TCHAR *pszKey);
	void LogOpenedContainer(const CItemContainer *pContainer);

	bool IsPriv(WORD wPrivFlag) const
	{
		if ( m_pAccount )
			return m_pAccount->IsPriv(wPrivFlag);
		return false;
	}
	void SetPrivFlags(WORD wPrivFlags)
	{
		if ( m_pAccount )
			m_pAccount->SetPrivFlags(wPrivFlags);
	}
	void ClearPrivFlags(WORD wPrivFlags)
	{
		if ( m_pAccount )
			m_pAccount->ClearPrivFlags(wPrivFlags);
	}

	BYTE GetResDisp() const
	{
		if ( m_pAccount )
			return m_pAccount->GetResDisp();
		return UCHAR_MAX;
	}
	bool SetResDisp(BYTE bResDisp)
	{
		if ( m_pAccount )
			return m_pAccount->SetResDisp(bResDisp);
		return false;
	}

	void SetScreenSize(WORD x, WORD y)
	{
		m_ScreenSize.x = x;
		m_ScreenSize.y = y;
	}

	PLEVEL_TYPE GetPrivLevel() const
	{
		if ( m_pAccount )
			return m_pAccount->GetPrivLevel();
		return PLEVEL_Guest;
	}
	LPCTSTR GetName() const
	{
		if ( m_pAccount )
			return m_pAccount->GetName();
		return "NA";
	}
	CChar *GetChar() const
	{
		return m_pChar;
	}

	void SysMessage(LPCTSTR pszMsg) const;
	bool CanSee(const CObjBaseTemplate *pObj) const;
	bool CanHear(const CObjBaseTemplate *pSrc, TALKMODE_TYPE mode) const;

	bool Dialog_Setup(CLIMODE_TYPE mode, RESOURCE_ID_BASE rid, int iPage, CObjBase *pObj, LPCTSTR pszArgs = "");
	bool Dialog_Close(CObjBase *pObj, DWORD rid, DWORD dwButtonID);
	void Menu_Setup(RESOURCE_ID_BASE rid, CObjBase *pObj = NULL);

	int OnSkill_Info(SKILL_TYPE skill, CGrayUID uid, int iSkillLevel, bool fTest);

	bool Cmd_Use_Item(CItem *pItem, bool fTestTouch, bool fScript = false);
	void Cmd_EditItem(CObjBase *pObj, int iSelect);

	bool IsConnectTypePacket() const
	{
		// This is a game or login server
		// m_Crypt.IsInit()
		return ((m_iConnectType == CONNECT_CRYPT) || (m_iConnectType == CONNECT_LOGIN) || (m_iConnectType == CONNECT_GAME));
	}

	CONNECT_TYPE GetConnectType() const
	{
		return m_iConnectType;
	}
	void SetConnectType(CONNECT_TYPE iType);

	CLIMODE_TYPE GetTargMode() const
	{
		return m_Targ_Mode;
	}
	void SetTargMode(CLIMODE_TYPE targmode = CLIMODE_NORMAL, LPCTSTR pszPrompt = NULL, int iTimeout = 0);
	void ClearTargMode()
	{
		m_Targ_Mode = CLIMODE_NORMAL;
		m_Targ_Timeout.Init();
	}

private:
	CGString m_BarkBuffer;

public:
	char m_zLastMessage[SCRIPT_MAX_LINE_LEN];
	char m_zLastObjMessage[SCRIPT_MAX_LINE_LEN];
	char m_zLogin[64];
	CVarDefMap m_TagDefs;
	CVarDefMap m_BaseDefs;		// new variable storage system

	typedef std::map<DWORD, std::pair<std::pair<DWORD, DWORD>, CPointMap> > OpenedContainerMap_t;
	OpenedContainerMap_t m_openedContainers;	// UID list of all containers opened by the client

	CGObArray<CClientTooltip *> m_TooltipData;	// storage for tooltip data while in trigger
	CItemMultiCustom *m_pHouseDesign;	// the building this client is designing

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

#ifndef _MTNETWORK
	friend class NetworkIn;
	friend class NetworkOut;
#else
	friend class NetworkInput;
	friend class NetworkOutput;
#endif
	friend class PacketServerRelay;

private:
	CClient(const CClient &copy);
	CClient &operator=(const CClient &other);
};

class CClientTooltip
{
	// Storage for Tooltip data while in trigger on an item
public:
	explicit CClientTooltip(DWORD dwClilocID, LPCTSTR pszArgs = NULL)
	{
		m_clilocid = dwClilocID;
		if ( pszArgs )
			strcpylen(m_args, pszArgs, SCRIPT_MAX_LINE_LEN);
		else
			m_args[0] = '\0';
	}

	static const char *m_sClassName;

public:
	DWORD m_clilocid;
	TCHAR m_args[SCRIPT_MAX_LINE_LEN];

public:
	void __cdecl FormatArgs(LPCTSTR pszFormat, ...) __printfargs(2, 3)
	{
		va_list vargs;
		va_start(vargs, pszFormat);

		if ( !_vsnprintf(m_args, SCRIPT_MAX_LINE_LEN, pszFormat, vargs) )
			strcpylen(m_args, pszFormat, SCRIPT_MAX_LINE_LEN);

		va_end(vargs);
	}

private:
	CClientTooltip(const CClientTooltip &copy);
	CClientTooltip &operator=(const CClientTooltip &other);
};

class CPartyDef : public CGObListRec, public CScriptObj
{
	// List of characters in party
public:
	CPartyDef(CChar *pCharInviter, CChar *pCharAccept);

	static const char *m_sClassName;
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szLoadKeysM[];

private:
	CCharRefArray m_Chars;
	CGString m_sName;
	CVarDefMap m_TagDefs;
	CVarDefMap m_BaseDefs;		// new variable storage system
	CGString m_pSpeechFunction;

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

private:
	bool SendMemberMsg(CChar *pCharDest, PacketSend *pPacket);
	void SendAll(PacketSend *pPacket);

	// List manipulation
	size_t AttachChar(CChar *pChar);
	size_t DetachChar(CChar *pChar);

public:
	static bool AcceptEvent(CChar *pCharAccept, CChar *pCharInviter, bool fForced = false);
	static bool DeclineEvent(CChar *pCharDecline, CChar *pCharInviter);

	bool IsPartyFull() const
	{
		return (m_Chars.GetCharCount() >= 10);
	}
	bool IsInParty(const CChar *pChar) const
	{
		return m_Chars.IsCharIn(pChar);
	}
	bool IsPartyMaster(const CChar *pChar) const
	{
		return (m_Chars.FindChar(pChar) == 0);
	}

	LPCTSTR GetName() const
	{
		return static_cast<LPCTSTR>(m_sName);
	}
	CChar *FindMaster() const
	{
		return m_Chars.GetChar(0).CharFind();
	}

	// Functions sent to all party members
	void StatsUpdateAll(CChar *pCharSrc, PacketSend *pPacket);
	void SysMessageAll(LPCTSTR pszMsg);
	void UpdateWaypointAll(CChar *pCharSrc, MAPWAYPOINT_TYPE type);

	// List sending wrappers
	bool SendRemoveList(CChar *pCharRemove);
	bool SendAddList(CChar *pCharDest);

	// Party message sending wrappers
	bool MessageEvent(CChar *pCharDest, CChar *pCharSrc, const NCHAR *pszMsg, int iMsgLen);

	// Commands
	bool Disband();
	bool RemoveMember(CChar *pChar, CChar *pCharSrc = NULL);
	void AcceptMember(CChar *pChar);
	void SetLootFlag(CChar *pChar, bool fSet);
	bool GetLootFlag(const CChar *pChar);
	bool SetMaster(CChar *pChar);

	bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);
	bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	bool r_Verb(CScript &s, CTextConsole *pSrc); // Execute command from script
	bool r_LoadVal(CScript &s);
	bool r_Load(CScript &s);

private:
	CPartyDef(const CPartyDef &copy);
	CPartyDef &operator=(const CPartyDef &other);
};

#endif	// _INC_CCLIENT_H
