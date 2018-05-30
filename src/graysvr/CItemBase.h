//
// CItemBase.h
//

#ifndef _INC_CITEMBASE_H
#define _INC_CITEMBASE_H
#pragma once

enum IT_TYPE		// double click type action.
{
	// NOTE: Never change this list as it will mess up the RES_ITEMDEF or RES_WORLDITEM already stored.
	// Just add stuff to end.
	IT_NORMAL = 0,
	IT_CONTAINER,			// 1 = any unlocked container or corpse. CContainer based
	IT_CONTAINER_LOCKED,	// 2 =
	IT_DOOR,				// 3 = door can be opened
	IT_DOOR_LOCKED,			// 4 = a locked door.
	IT_KEY,					// 5 =
	IT_LIGHT_LIT,			// 6 = Local Light giving object. (needs no fuel, never times out)
	IT_LIGHT_OUT,			// 7 = Can be lit.
	IT_FOOD,				// 8 = edible food. (poisoned food ?)
	IT_FOOD_RAW,			// 9 = Must cook raw meat unless your an animal.
	IT_ARMOR,				// 10 = some type of armor. (no real action)
	IT_WEAPON_MACE_SMITH,	// 11 = Can be used for smithing
	IT_WEAPON_MACE_SHARP,	// 12 = war axe can be used to cut/chop trees.
	IT_WEAPON_SWORD,		// 13 =
	IT_WEAPON_FENCE,		// 14 = can't be used to chop trees. (make kindling)
	IT_WEAPON_BOW,			// 15 = bow or xbow
	IT_WAND,			    // 16 = A magic storage item
	IT_TELEPAD,				// 17 = walk on teleport
	IT_SWITCH,				// 18 = this is a switch which effects some other object in the world.
	IT_BOOK,				// 19 = read this book. (static or dynamic text)
	IT_RUNE,				// 20 = can be marked and renamed as a recall rune.
	IT_BOOZE,				// 21 = booze	(drunk effect)
	IT_POTION,				// 22 = Some liquid in a container. Possibly Some magic effect.
	IT_FIRE,				// 23 = It will burn you.
	IT_CLOCK,				// 24 = or a wristwatch
	IT_TRAP,				// 25 = walk on trap.
	IT_TRAP_ACTIVE,			// 26 = some animation
	IT_MUSICAL,				// 27 = a musical instrument.
	IT_SPELL,				// 28 = magic spell effect.
	IT_GEM,					// 29 = no use yet
	IT_WATER,				// 30 = This is water (fishable) (Not a glass of water)
	IT_CLOTHING,			// 31 = All cloth based wearable stuff,
	IT_SCROLL,				// 32 = magic scroll
	IT_CARPENTRY,			// 33 = tool of some sort.
	IT_SPAWN_CHAR,			// 34 = spawn object. should be invis also.
	IT_GAME_PIECE,			// 35 = can't be removed from game.
	IT_PORTCULIS,			// 36 = Z delta moving gate. (open)
	IT_FIGURINE,			// 37 = magic figure that turns into a creature when activated.
	IT_SHRINE,				// 38 = can res you
	IT_MOONGATE,			// 39 = linked to other moon gates (hard coded locations)
	IT_UNUSED_40,			// 40
	IT_FORGE,				// 41 = used to smelt ore, blacksmithy etc.
	IT_ORE,					// 42 = smelt to ingots.
	IT_LOG,					// 43 = make into furniture etc. lumber,logs,
	IT_TREE,				// 44 = can be chopped.
	IT_ROCK,				// 45 = can be mined for ore.
	IT_CARPENTRY_CHOP,		// 46 = tool of some sort.
	IT_MULTI,				// 47 = multi part object like house or ship.
	IT_REAGENT,				// 48
	IT_SHIP,				// 49 = this is a SHIP MULTI
	IT_SHIP_PLANK,			// 50
	IT_SHIP_SIDE,			// 51 = Should extend to make a plank
	IT_SHIP_SIDE_LOCKED,	// 52
	IT_SHIP_TILLER,			// 53 = Tiller man on the ship.
	IT_EQ_TRADE_WINDOW,		// 54 = container for the trade window.
	IT_FISH,				// 55 = fish can be cut up.
	IT_SIGN_GUMP,			// 56 = Things like grave stones and sign plaques
	IT_STONE_GUILD,			// 57 = Guild stones
	IT_ANIM_ACTIVE,			// 58 = active anim that will recycle when done.
	IT_SAND,				// 59 = sand
	IT_CLOTH,				// 60 = bolt or folded cloth
	IT_HAIR,				// 61
	IT_BEARD,				// 62 = just for grouping purposes.
	IT_INGOT,				// 63 = Ingot of some type made from IT_ORE.
	IT_COIN,				// 64 = coin of some sort. gold or otherwise.
	IT_CROPS,				// 65 = a plant that will regrow. picked type.
	IT_DRINK,				// 66 = some sort of drink (non booze, potion or water) (ex. cider)
	IT_ANVIL,				// 67 = for repair.
	IT_PORT_LOCKED,			// 68 = this portcullis must be triggered.
	IT_SPAWN_ITEM,			// 69 = spawn other items.
	IT_UNUSED_70,			// 70
	IT_UNUSED_71,			// 71
	IT_GOLD,				// 72 = Gold Coin
	IT_MAP,					// 73 = Map object with pins.
	IT_EQ_MEMORY_OBJ,		// 74 = A Char has a memory link to some object. (I am fighting with someone. This records the fight.)
	IT_WEAPON_MACE_STAFF,	// 75 = staff type of mace. or just other type of mace.
	IT_EQ_HORSE,			// 76 = equipped horse object represents a riding horse to the client.
	IT_COMM_CRYSTAL,		// 77 = communication crystal.
	IT_GAME_BOARD,			// 78 = this is a container of pieces.
	IT_TRASH_CAN,			// 79 = delete any object dropped on it.
	IT_CANNON_MUZZLE,		// 80 = cannon muzzle. NOT the other cannon parts.
	IT_CANNON,				// 81 = the rest of the cannon.
	IT_CANNON_BALL,			// 82
	IT_ARMOR_LEATHER,		// 83 = Non metallic armor.
	IT_SEED,				// 84 = seed from fruit
	IT_JUNK,				// 85 = ring of reagents.
	IT_CRYSTAL_BALL,		// 86
	IT_SWAMP,				// 87 = swamp
	IT_MESSAGE,				// 88 = user written message item (for bboard usually)
	IT_REAGENT_RAW,			// 89 = Freshly grown reagents...not processed yet. NOT USED!
	IT_EQ_CLIENT_LINGER,	// 90 = Change player to NPC for a while.
	IT_SNOW,				// 91 = snow
	IT_UNUSED_92,			// 92
	IT_UNUSED_93,			// 93
	IT_EXPLOSION,			// 94 = async explosion.
	IT_EQ_NPC_SCRIPT,		// [OFF] 95 = Script npc actions in the form of a book.
	IT_WEB,					// 96 = walk on this and transform into some other object. (stick to it)
	IT_GRASS,				// 97 = can be eaten by grazing animals
	IT_AROCK,				// 98 = a rock or boulder. can be thrown by giants.
	IT_TRACKER,				// 99 = points to a linked object.
	IT_SOUND,				// 100 = this is a sound source.
	IT_STONE_TOWN,			// 101 = Town stones. everyone free to join.
	IT_WEAPON_MACE_CROOK,	// 102
	IT_WEAPON_MACE_PICK,	// 103
	IT_LEATHER,				// 104 = Leather or skins of some sort.(not wearable)
	IT_SHIP_OTHER,			// 105 = some other part of a ship.
	IT_BBOARD,				// 106 = a container and bboard object.
	IT_SPELLBOOK,			// 107 = spellbook (with spells)
	IT_CORPSE,				// 108 = special type of item.
	IT_UNUSED_109,			// 109
	IT_UNUSED_110,			// 110
	IT_WEAPON_ARROW,		// 111
	IT_WEAPON_BOLT,			// 112
	IT_EQ_VENDOR_BOX,		// 113 = an equipped vendor .
	IT_EQ_BANK_BOX,			// 114 = an equipped bank box
	IT_DEED,				// 115
	IT_LOOM,				// 116
	IT_BEE_HIVE,			// 117
	IT_ARCHERY_BUTTE,		// 118
	IT_EQ_MURDER_COUNT,		// 119 = my murder count flag.
	IT_EQ_STUCK,			// 120
	IT_TRAP_INACTIVE,		// 121 = a safe trap.
	IT_UNUSED_122,			// 122
	IT_BANDAGE,				// 123 = can be used for healing.
	IT_CAMPFIRE,			// 124 = this is a fire but a small one.
	IT_MAP_BLANK,			// 125 = blank map.
	IT_SPY_GLASS,			// 126
	IT_SEXTANT,				// 127
	IT_SCROLL_BLANK,		// 128
	IT_FRUIT,				// 129
	IT_WATER_WASH,			// 130 = water that will not contain fish. (for washing or drinking)
	IT_WEAPON_AXE,			// 131 = not the same as a sword. but uses swordsmanship skill
	IT_WEAPON_XBOW,			// 132
	IT_SPELLICON,			// 133
	IT_DOOR_OPEN,			// 134 = You can walk through doors that are open.
	IT_MEAT_RAW,			// 135 = raw (uncooked meat) or part of a corpse.
	IT_UNUSED_136,			// 136
	IT_KEYRING,				// 137
	IT_UNUSED_138,			// 138
	IT_FLOOR,				// 139
	IT_ROOF,				// 140
	IT_FEATHER,				// 141 = a birds feather
	IT_WOOL,				// 142 = Wool cut frm a sheep.
	IT_FUR,					// 143
	IT_BLOOD,				// 144 = blood of some creature
	IT_FOLIAGE,				// 145 = does not go invis when reaped. but will if eaten
	IT_GRAIN,				// 146
	IT_SCISSORS,			// 147
	IT_THREAD,				// 148
	IT_YARN,				// 149
	IT_SPINWHEEL,			// 150
	IT_BANDAGE_BLOOD,		// 151 = must be washed in water to get bandage back.
	IT_FISH_POLE,			// 152
	IT_SHAFT,				// 153 = used to make arrows and xbolts
	IT_LOCKPICK,			// 154
	IT_KINDLING,			// 155 = lit to make campfire
	IT_TRAIN_DUMMY,			// 156
	IT_TRAIN_PICKPOCKET,	// 157
	IT_BEDROLL,				// 158
	IT_UNUSED_159,			// 159
	IT_HIDE,				// 160 = hides are cured to make leather.
	IT_CLOTH_BOLT,			// 161 = must be cut up to make cloth squares.
	IT_BOARD,				// 162 = logs are plained into decent lumber
	IT_PITCHER,				// 163
	IT_PITCHER_EMPTY,		// 164
	IT_DYE_VAT,				// 165
	IT_DYE,					// 166
	IT_POTION_EMPTY,		// 167 = empty bottle.
	IT_MORTAR,				// 168
	IT_UNUSED_169,			// 169
	IT_SEWING_KIT,			// 170
	IT_TINKER_TOOLS,		// 171
	IT_WALL,				// 172 = wall of a structure.
	IT_WINDOW,				// 173 = window for a structure.
	IT_COTTON,				// 174 = Cotton from the plant
	IT_BONE,				// 175
	IT_EQ_SCRIPT,			// 176
	IT_SHIP_HOLD,			// 177 = ships hold.
	IT_SHIP_HOLD_LOCK,		// 178
	IT_LAVA,				// 179
	IT_SHIELD,				// 180 = equippable armor.
	IT_JEWELRY,				// 181 = equippable.
	IT_DIRT,				// 182 = a patch of dirt where i can plant something
	IT_SCRIPT,				// 183 = Scriptable item (but not equippable)
	IT_SPELLBOOK_NECRO,		// 184 = AOS Necromancy spellbook (should have MOREZ=100 by default)
	IT_SPELLBOOK_PALA,		// 185 = AOS Paladin spellbook (should have MOREZ=200 by default)
	IT_SPELLBOOK_EXTRA,		// 186 = some spellbook for script purposes
	IT_SPELLBOOK_BUSHIDO,	// 187 = SE Bushido spellbook (should have MOREZ=400 by default)
	IT_SPELLBOOK_NINJITSU,	// 188 = SE Ninjitsu spellbook (should have MOREZ=500 by default)
	IT_SPELLBOOK_ARCANIST,	// 189 = ML Spellweaver spellbook (should have MOREZ=600 by default)
	IT_MULTI_CUSTOM,		// 190 = Customisable multi
	IT_SPELLBOOK_MYSTIC,	// 191 = SA Mysticism spellbook (should have MOREX=677 by default)
	IT_HOVEROVER,			// 192 = Hover-over item (CAN_C_HOVER can hover over blocking items)
	IT_SPELLBOOK_MASTERY,	// 193 = SA/TOL Masteries spellbook (should have MOREZ=700 by default)
	IT_WEAPON_THROWING,		// 194 = Throwing Weapon
	IT_CARTOGRAPHY,			// 195 = cartography tool
	IT_COOKING,				// 196 = cooking tool
	IT_PILOT,				// 197 = ship's pilot (PacketWheelMove)
	IT_ROPE,				// 198 = t_rope (working like t_ship_plank but without id changes)
	IT_HEALING_STONE,		// 199 = t_healing_stone

	IT_QTY,
	IT_TRIGGER = 1000		// custom triggers starts from here
};

class CItemBase : public CBaseBaseDef
{
	// RES_ITEMDEF
	// Describe basic stuff about all items.
	// Partly based on CUOItemTypeRec/CUOItemTypeRec2
public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szLoadKeys[];

	explicit CItemBase(ITEMID_TYPE id);
	virtual ~CItemBase() { };

private:
	CGTypedArray<ITEMID_TYPE, ITEMID_TYPE> m_flip_id;
	CValueRangeDef m_values;
	IT_TYPE m_type;
	WORD m_weight;				// weight in WEIGHT_UNITS (USHRT_MAX=not movable). Defaults from .mul files
	BYTE m_layer;
	DWORD m_dwFlags;			// UFLAG_* from CUOItemTypeRec/CUOItemTypeRec2
	BYTE m_speed;

public:
	SKILL_TYPE m_iSkill;
	CResourceQtyArray m_SkillMake;
	DWORD m_CanUse;

	int m_StrengthBonus;
	int m_DexterityBonus;
	int m_IntelligenceBonus;
	int m_HitpointIncrease;
	int m_StaminaIncrease;
	int m_ManaIncrease;
	int m_SpellChanneling;
	int m_LowerRequirements;
	int m_UseBestWeaponSkill;
	int m_WeightReduction;

	union
	{
		// IT_NORMAL
		struct	// used only to script ref all this junk.
		{
			DWORD m_tData1;	// TDATA1=
			DWORD m_tData2;	// TDATA2=
			DWORD m_tData3;	// TDATA3=
			DWORD m_tData4;	// TDATA4=
		} m_ttNormal;

		// IT_WAND
		// IT_WEAPON_*
		// IT_ARMOR
		// IT_ARMOR_LEATHER
		// IT_SHIELD
		// IT_CLOTHING
		// IT_JEWELRY
		// IT_EQ_SCRIPT
		// Container pack is the only exception here. IT_CONTAINER
		struct	// ALL equippable items ex. Weapons and armor
		{
			int m_junk1;
			int m_StrReq;			// REQSTR= Strength required to weild weapons/armor.
		} m_ttEquippable;

		// IT_LIGHT_OUT
		// IT_LIGHT_LIT
		struct
		{
			int	m_junk1;
			int m_junk2;					// REQSTR= Strength required to weild weapons/armor.
			RESOURCE_ID_BASE m_Light_ID;	// TDATA3=Change light state to on/off
		} m_ttLightSource;

		// IT_WEAPON_BOW
		// IT_WEAPON_XBOW
		struct
		{
			int m_junk1;
			int m_StrReq;				// REQSTR= Strength required to weild weapons/armor.
			RESOURCE_ID_BASE m_idAmmo;	// TDATA3= required source ammo.
			RESOURCE_ID_BASE m_idAmmoX;	// TDATA4= fired ammo fx.
		} m_ttWeaponBow;

		// IT_CONTAINER
		// IT_SIGN_GUMP
		// IT_SHIP_HOLD
		// IT_BBOARD
		// IT_CORPSE
		// IT_TRASH_CAN
		// IT_GAME_BOARD
		// IT_EQ_BANK_BOX
		// IT_EQ_VENDOR_BOX
		// IT_KEYRING
		struct
		{
			int	m_junk1;
			GUMP_TYPE m_gumpid;	// TDATA2= the gump that comes up when this container is opened.
			DWORD m_dwMinXY;	// TDATA3= Gump size used.
			DWORD m_dwMaxXY;	// TDATA4=
		} m_ttContainer;

		// IT_FIGURINE
		// IT_EQ_HORSE
		struct
		{
			int m_junk1;
			int m_StrReq;				// REQSTR= Strength required to mount
			RESOURCE_ID_BASE m_charid;	// TDATA3= (CREID_TYPE)
		} m_ttFigurine;

		// IT_SPELLBOOK
		// IT_SPELLBOOK_NECRO
		// IT_SPELLBOOK_PALA
		// IT_SPELLBOOK_BUSHIDO
		// IT_SPELLBOOK_NINJITSU
		// IT_SPELLBOOK_ARCANIST
		// IT_SPELLBOOK_MYSTIC
		// IT_SPELLBOOK_MASTERY
		struct
		{
			int	m_junk1;
			int	m_junk2;
			DWORD m_Offset;		// TDATA3= First spell number of this book type
			DWORD m_MaxSpells;	// TDATA4= Max spells that this book type can handle
		} m_ttSpellbook;

		// IT_MUSICAL
		struct
		{
			int m_iSoundGood;	// TDATA1= SOUND_TYPE if played well.
			int m_iSoundBad;	// TDATA2=sound if played poorly.
		} m_ttMusical;

		// IT_ORE
		struct
		{
			ITEMID_TYPE m_IngotID;	// tdata1= what ingot is this to be made into.
		} m_ttOre;

		// IT_INGOT
		struct
		{
			int m_iSkillMin;	// tdata1= what is the lowest skill
			int m_iSkillMax;	// tdata2= what is the highest skill for max yield
		} m_ttIngot;

		// IT_DOOR
		// IT_DOOR_OPEN
		struct
		{
			SOUND_TYPE m_iSoundClose;	// tdata1(low)= sound to close. SOUND_TYPE
			SOUND_TYPE m_iSoundOpen;	// tdata1(high)= sound to open. SOUND_TYPE
			ITEMID_TYPE m_idSwitch; // tdata2 ID to change into when open/close
			int m_iXChange;		// tdata3= X position to change
			int m_iYChange;		// tdata4= Y position to change
		} m_ttDoor;

		// IT_FOLIAGE - is not consumed on reap (unless eaten then will regrow invis)
		// IT_CROPS	- is consumed and will regrow invis.
		struct
		{
			ITEMID_TYPE m_idReset;	// tdata1= what will it be reset to regrow from ? 0=nothing
			ITEMID_TYPE m_idGrow;	// tdata2= what will it grow further into ? 0=fully mature.
			ITEMID_TYPE m_idFruit;	// tdata3= what can it be reaped for ? 0=immature can't be reaped
		} m_ttCrops;

		// IT_SEED
		// IT_FRUIT
		struct
		{
			ITEMID_TYPE m_idReset;	// tdata1= what will it be reset to regrow from ? 0=nothing
			ITEMID_TYPE m_idSeed;	// tdata2= what does the seed look like ? Copper coin = default.
		} m_ttFruit;

		// IT_DRINK
		// IT_BOOZE
		// IT_POTION
		// IT_PITCHER
		// IT_WATER_WASH
		struct
		{
			ITEMID_TYPE m_idEmpty;	// tdata1= the empty pitcher. IT_POTION_EMPTY IT_PITCHER_EMPTY
		} m_ttDrink;

		// IT_SHIP_PLANK
		// IT_SHIP_SIDE
		// IT_SHIP_SIDE_LOCKED
		struct
		{
			ITEMID_TYPE m_idState;	// tdata1= next state open/close for the Ship Side
		} m_ttShipPlank;

		// IT_MAP
		struct
		{
			int m_iGumpWidth;	// tdata1= map gump width
			int m_iGumpHeight;	// tdata2= map gump height
		} m_ttMap;
	};

private:
	int CalculateMakeValue(int iSkillLevel) const;
	static CItemBase *MakeDupeReplacement(CItemBase *pBase, ITEMID_TYPE iddupe);

protected:
	static void ReplaceItemBase(CItemBase *pOld, CResourceDef *pNew);

public:
	static CItemBase *FindItemBase(ITEMID_TYPE id);
	void CopyBasic(const CItemBase *pItemDef);
	void CopyTransfer(CItemBase *pItemDef);

	void SetTypeName(LPCTSTR pszName);
	LPCTSTR GetArticleAndSpace() const;
	static TCHAR *GetNamePluralize(LPCTSTR pszNameBase, bool fPluralize);
	static CREID_TYPE FindCharTrack(ITEMID_TYPE id);

	static bool IsTypeArmor(IT_TYPE type);
	static bool IsTypeWeapon(IT_TYPE type);
	static bool IsTypeSpellbook(IT_TYPE type);
	static bool IsTypeMulti(IT_TYPE type);
	bool IsTypeEquippable() const;

	static int IsID_Door(ITEMID_TYPE id);
	static bool IsID_DoorOpen(ITEMID_TYPE id);

	static bool GetItemData(ITEMID_TYPE id, CUOItemTypeRec2 *pTiledata);
	static void GetItemSpecificFlags(const CUOItemTypeRec2 &tiledata, DWORD &dwBlockThis, IT_TYPE type, ITEMID_TYPE id);
	static void GetItemTiledataFlags(DWORD &dwBlockThis, ITEMID_TYPE id);
	static height_t GetItemHeight(ITEMID_TYPE id, DWORD &dwBlockThis);
	static height_t GetItemHeightFlags(const CUOItemTypeRec2 &tiledata, DWORD &dwBlockThis);

	static IT_TYPE GetTypeBase(ITEMID_TYPE id, const CUOItemTypeRec2 &tiledata);
	GUMP_TYPE GetContainerGumpID() const;
	ITEMID_TYPE GetNextFlipID(ITEMID_TYPE id) const;

	bool IsSameDispID(ITEMID_TYPE id) const;
	void Restock();

	BYTE GetSpeed() const;

	WORD GetMaxAmount();
	bool SetMaxAmount(WORD wAmount);

	int GetMakeValue(int iSkillLevel);
	void ResetMakeValue();

	bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc = NULL);
	virtual bool r_LoadVal(CScript &s);

public:
	virtual void UnLink()
	{
		m_flip_id.RemoveAll();
		m_SkillMake.RemoveAll();
		CBaseBaseDef::UnLink();
	}

	LPCTSTR GetName() const
	{
		return GetNamePluralize(GetTypeName(), false);
	}

	LAYER_TYPE GetEquipLayer() const
	{
		return static_cast<LAYER_TYPE>(m_layer);
	}

	static bool IsVisibleLayer(LAYER_TYPE layer)
	{
		return ((layer > LAYER_NONE) && (layer <= LAYER_HORSE));
	}

	ITEMID_TYPE GetID() const
	{
		return static_cast<ITEMID_TYPE>(GetResourceID().GetResIndex());
	}

	ITEMID_TYPE GetDispID() const
	{
		return static_cast<ITEMID_TYPE>(m_dwDispIndex);
	}

	DWORD GetTFlags() const
	{
		return m_dwFlags;
	}

	IT_TYPE GetType() const
	{
		return m_type;
	}

#define WEIGHT_UNITS 10

	WORD GetWeight() const
	{
		// Get weight in tenths of a stone
		if ( IsMovableType() )
			return m_weight;
		return WEIGHT_UNITS;	// if we can pick them up then we should be able to move them
	}

	WORD GetVolume() const
	{
		return m_weight / WEIGHT_UNITS;
	}

	bool IsStackableType() const
	{
		return Can(CAN_I_PILE);
	}

	bool IsType(IT_TYPE type) const
	{
		return (type == m_type);
	}

	bool IsMovableType() const
	{
		return (m_weight != USHRT_MAX);
	}

	static bool IsValidDispID(ITEMID_TYPE id)
	{
		return ((id > ITEMID_NOTHING) && (id < ITEMID_MULTI_MAX));
	}

	static bool IsID_Multi(ITEMID_TYPE id)
	{
		// IT_MULTI
		// IT_MULTI_CUSTOM
		return ((id >= ITEMID_MULTI) && (id < ITEMID_MULTI_MAX));
	}

	static bool IsID_Ship(ITEMID_TYPE id)
	{
		// IT_SHIP
		return ((id >= ITEMID_SHIP_SMALL_N) && (id <= ITEMID_GALLEON_BRIT2_W));
	}

	static bool IsID_GamePiece(ITEMID_TYPE id)
	{
		// IT_GAME_PIECE
		return ((id >= ITEMID_GAME1_CHECKER) && (id <= ITEMID_GAME_HI));
	}

	static bool IsID_Track(ITEMID_TYPE id)
	{
		// IT_FIGURINE
		return ((id >= ITEMID_TRACK_BEGIN) && (id <= ITEMID_TRACK_END));
	}

	static bool IsID_WaterFish(ITEMID_TYPE id)
	{
		// IT_WATER
		return ((id == 0x1559) || ((id >= 0x1796) && (id <= 0x17B2)));
	}

	static bool IsID_WaterWash(ITEMID_TYPE id)
	{
		// IT_WATER_WASH
		if ( (id >= ITEMID_WATER_TROUGH_1) && (id <= ITEMID_WATER_TROUGH_2) )
			return true;
		return IsID_WaterFish(id);
	}

private:
	CItemBase(const CItemBase &copy);
	CItemBase &operator=(const CItemBase &other);
};

class CItemBaseDupe : public CResourceDef
{
	// RES_ITEMDEF
public:
	static const char *m_sClassName;

	CItemBaseDupe(ITEMID_TYPE id, CItemBase *pMasterItem) : CResourceDef(RESOURCE_ID(RES_ITEMDEF, id)), m_MasterItem(pMasterItem), m_Can(0), m_dwFlags(0), m_Height(0)
	{
		ASSERT(pMasterItem);
		ASSERT(pMasterItem->GetResourceID().GetResIndex() != id);
	}
	virtual	~CItemBaseDupe() { }

public:
	CResourceRef m_MasterItem;
	DWORD m_Can;

private:
	DWORD m_dwFlags;	// UFLAG_* from CUOItemTypeRec/CUOItemTypeRec2
	height_t m_Height;

public:
	virtual void UnLink()
	{
		m_MasterItem.SetRef(NULL);
		CResourceDef::UnLink();
	}

	CItemBase *GetItemDef() const
	{
		CResourceLink *pLink = m_MasterItem;
		ASSERT(pLink);
		CItemBase *pItemDef = dynamic_cast<CItemBase *>(pLink);
		ASSERT(pItemDef);
		return pItemDef;
	}

	static CItemBaseDupe *GetDupeRef(ITEMID_TYPE id);

	DWORD GetTFlags() const
	{
		return m_dwFlags;
	}
	void SetTFlags(DWORD dwFlags)
	{
		m_dwFlags = dwFlags;
	}

	height_t GetHeight() const
	{
		return m_Height;
	}
	void SetHeight(height_t Height)
	{
		m_Height = Height;
	}

private:
	CItemBaseDupe(const CItemBaseDupe &copy);
	CItemBaseDupe &operator=(const CItemBaseDupe &other);
};

class CItemBaseMulti : public CItemBase
{
	// This item is really a multi with other items associated
	// Define the list of objects it is made of
	// NOTE: It does not have to be a true multi item ITEMID_MULTI
public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szLoadKeys[];

	explicit CItemBaseMulti(CItemBase *pBase);
	virtual ~CItemBaseMulti() { }

public:
	struct CMultiComponentItem
	{
		ITEMID_TYPE m_id;
		signed short m_dx;
		signed short m_dy;
		signed char m_dz;
	};
	CGTypedArray<CMultiComponentItem, CMultiComponentItem &> m_Components;
	CGRect m_rect;
	DWORD m_dwRegionFlags;			// base region flags (REGION_FLAG_*)
	CResourceRefArray m_Speech;		// speech fragment list (other stuff we know)

	struct ShipSpeed
	{
		BYTE period;	// time between movements
		BYTE tiles;		// distance to move
	};
	ShipSpeed m_shipSpeed;
	BYTE m_SpeedMode;

public:
	static CItemBase *MakeMultiRegion(CItemBase *pBase, CScript &s);
	void SetMultiRegion(TCHAR *pszArgs);
	bool AddComponent(ITEMID_TYPE id, signed short dx, signed short dy, signed char dz);
	bool AddComponent(TCHAR *pszArgs);
	int GetMaxDist() const;

	bool r_LoadVal(CScript &s);
	bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);

private:
	CItemBaseMulti(const CItemBaseMulti &copy);
	CItemBaseMulti &operator=(const CItemBaseMulti &other);
};

#endif	// _INC_CITEMBASE_H
