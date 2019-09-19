// Define resources in the MUL files.
#ifndef _INC_GRAYMUL_H
#define _INC_GRAYMUL_H
#pragma once

//---------------------------MUL FILE DEFS---------------------------

// All these structures must be BYTE packed.
#if defined _WIN32 && (!__MINGW32__)
// Microsoft dependant pragma
#pragma pack(1)
#define PACK_NEEDED
#else
// GCC based compiler you can add:
#define PACK_NEEDED __attribute__ ((packed))
#endif

// NOTE: !!! ALL Multi bytes in file ASSUME big endian !!!!

#define UO_MAP_DIST_INTERACT	2	// Max distance chars can reach when interact with nearby objects (it must be '2' to match client internal value)
#define UO_MAP_VIEW_SIGHT		14	// Visibility distance for NPCs
#define UO_MAP_VIEW_SIZE		18	// Visibility distance for items (on old clients it's always 18, and since client 7.0.55.27 it's now dynamic 18~24 based on client screen resolution)
#define UO_MAP_VIEW_RADAR		36	// Visibility distance for multis (houses/boats)

////////////////////////////////////////////////////////////////////////
// Shared enum types.

// 20 colors of 10 hues and 5 brightnesses, which gives us 1000 colors.
//  plus black, and default.
// Skin color is similar, except there are 7 basic colors of 8 hues.
// For hair there are 7 colors of 7 hues.
typedef WORD HUE_TYPE;		// Index into the hues.mul table.

enum HUE_CODE
{
	HUE_DEFAULT				= 0x0,
	HUE_BLUE_LOW			= 0x2,		// lowest dyeable color
	HUE_BLUE_NAVY			= 0x3,
	HUE_RED_DARK			= 0x20,
	HUE_RED					= 0x22,
	HUE_YELLOW				= 0x35,
	HUE_GRAY				= 0x387,
	HUE_TEXT_DEF			= 0x3B2,	// light gray color
	HUE_DYE_HIGH			= 0x3E9,	// highest dyeable color
	HUE_SKIN_LOW			= 0x3EA,
	HUE_SKIN_HIGH			= 0x422,
	HUE_HAIR_LOW			= 0x44E,
	HUE_HAIR_HIGH			= 0x47D,
	HUE_WHITE				= 0x481,
	HUE_STONE				= 0x482,
	HUE_GARGSKIN_LOW		= 0x6DB,
	HUE_GARGSKIN_HIGH		= 0x6F3,
	HUE_MASK_LO				= 0x7FF,	// mask for items. (not really a valid thing to do i know)
	HUE_QTY					= 0xBB8,	// number of valid colors in hue table
	HUE_MASK_HI				= 0xFFF,
	HUE_MASK_TRANSLUCENT	= 0x4000,
	HUE_TRANSLUCENT			= 0x4001,
	HUE_MASK_UNDERWEAR		= 0x8000	// can be used only on chars
};

typedef WORD SOUND_TYPE;	// Sound ID

enum SOUND_CODE
{
	SOUND_NONE			= 0x0,
	SOUND_DRIP3			= 0x2A,
	SOUND_DROP_GOLD1	= 0x32,
	SOUND_DROP_GOLD2	= 0x37,
	SOUND_HAMMER		= 0x42,
	SOUND_LEATHER		= 0x48,
	SOUND_RUSTLE		= 0x4F,
	SOUND_USE_CLOTH		= 0x57,
	SOUND_SPELL_FIZZLE	= 0x5C,
	SOUND_SFX6			= 0xF9,
	SOUND_TELEPORT		= 0x1FE,
	SOUND_FLAMESTRIKE	= 0x208,
	SOUND_FLAME5		= 0x227,
	SOUND_FISH_SPLASH	= 0x23F,
	SOUND_LIQUID		= 0x240,
	SOUND_SNIP			= 0x248,
	SOUND_SCRIBE		= 0x249,
	SOUND_SPIRITSPEAK	= 0x24A,
	SOUND_CROSSBOW		= 0x2B1,
	SOUND_DROP_MONEY1	= 0x2E4,
	SOUND_DROP_MONEY2	= 0x2E5,
	SOUND_DROP_MONEY3	= 0x2E6,
	SOUND_GLASS_BREAK4	= 0x390,
	SOUND_SPECIAL_HUMAN = 0x900		// custom (hardcoded) sound, to be played with SoundChar
};

typedef WORD MIDI_TYPE;	// Music id

#define MULTI_QTY	0x2000	// total address space for multis.
typedef WORD MULTI_TYPE;	// define a multi (also defined by ITEMID_MULTI)

enum ITEMID_TYPE	// InsideUO is great for this stuff.
{
	ITEMID_NOTHING				= 0x0,	// used for lightning
	ITEMID_NODRAW				= 0x1,

	ITEMID_STONE_WALL			= 0x80,

	ITEMID_DOOR_SECRET_STONE_1	= 0xE8,

	ITEMID_DOOR_SECRET_STONE_2	= 0x314,
	ITEMID_DOOR_SECRET_STONE_3	= 0x324,
	ITEMID_DOOR_SECRET_WOOD_1	= 0x334,
	ITEMID_DOOR_SECRET_WOOD_2	= 0x344,
	ITEMID_DOOR_SECRET_STONE_4	= 0x354,

	ITEMID_DOOR_METAL_1			= 0x675,
	ITEMID_DOOR_METAL_BARRED_1	= 0x685,
	ITEMID_DOOR_RATTAN			= 0x695,
	ITEMID_DOOR_WOODEN_1		= 0x6A5,
	ITEMID_DOOR_WOODEN_2		= 0x6B5,
	ITEMID_DOOR_METAL_2			= 0x6C5,
	ITEMID_DOOR_WOODEN_3		= 0x6D5,
	ITEMID_DOOR_WOODEN_LIGHT	= 0x6D9,
	ITEMID_DOOR_WOODEN_4		= 0x6E5,
	ITEMID_DOOR_PORTCULLIS_1	= 0x6F5,
	ITEMID_DOOR_PORTCULLIS_2	= 0x6F6,

	ITEMID_GATE_IRON_1			= 0x824,
	ITEMID_GATE_WOODEN_1		= 0x839,
	ITEMID_GATE_IRON_2			= 0x84C,
	ITEMID_GATE_WOODEN_2		= 0x866,

	ITEMID_FOOD_FISH_RAW		= 0x97A,
	ITEMID_BANK_BOX				= 0x9B2,	// another pack really but used as bank box
	ITEMID_JAR_HONEY			= 0x9EC,

	ITEMID_BEDROLL_O_EW			= 0xA55,
	ITEMID_BEDROLL_O_NS			= 0xA56,
	ITEMID_BEDROLL_C			= 0xA57,
	ITEMID_BEDROLL_C_NS			= 0xA58,
	ITEMID_BEDROLL_C_EW			= 0xA59,

	ITEMID_WATER_TROUGH_1		= 0xB41,
	ITEMID_WATER_TROUGH_2		= 0xB44,

	ITEMID_MOONGATE_RED			= 0xDDA,
	ITEMID_KINDLING1			= 0xDE1,
	ITEMID_CAMPFIRE				= 0xDE3,
	ITEMID_EMBERS				= 0xDE9,
	ITEMID_WOOL					= 0xDF8,

	ITEMID_YARN1				= 0xE1D,
	ITEMID_BANDAGES_BLOODY1		= 0xE20,
	ITEMID_BANDAGES1			= 0xE21,	// clean
	ITEMID_BANDAGES_BLOODY2		= 0xE22,
	ITEMID_SPELLBOOK2			= 0xE3B,	// ??? looks like a spellbook but doesn open corectly !
	ITEMID_Cannon_Ball			= 0xE73,
	ITEMID_BACKPACK				= 0xE75,	// containers.
	ITEMID_CHEST_SILVER			= 0xE7C,	// all grey. BANK BOX
	ITEMID_BBOARD_MSG			= 0xEB0,	// a message on the bboard
	ITEMID_SKELETON_1			= 0xECA,
	ITEMID_SKELETON_9			= 0xED2,
	ITEMID_WEB1_1				= 0xEE3,
	ITEMID_WEB1_4				= 0xEE6,
	ITEMID_GOLD_C1				= 0xEED,	// big pile
	ITEMID_SPELLBOOK			= 0xEFA,

	ITEMID_EMPTY_BOTTLE			= 0xF0E,
	ITEMID_DAGGER				= 0xF51,
	ITEMID_MOONGATE_BLUE		= 0xF6C,
	ITEMID_REAG_SA				= 0xF8C,	// sulfurous ash
	ITEMID_CLOTH_BOLT1			= 0xF95,
	ITEMID_CLOTH_BOLT8			= 0xF9C,
	ITEMID_THREAD1				= 0xFA0,
	ITEMID_DYE_TUB				= 0xFAB,
	ITEMID_PITCHER_WATER		= 0xFF8,

	ITEMID_ARCHERYBUTTE_E		= 0x100A,
	ITEMID_ARCHERYBUTTE_S		= 0x100B,
	ITEMID_KEY_COPPER			= 0x100E,
	ITEMID_KEY_RING0			= 0x1011,
	ITEMID_KEY_MAGIC			= 0x1012,
	ITEMID_LEATHER_1			= 0x1067,
	ITEMID_LEATHER_2			= 0x1068,
	ITEMID_HIDES				= 0x1078,

	ITEMID_BLOOD1				= 0x122A,
	ITEMID_BLOOD2				= 0x122B,
	ITEMID_BLOOD3				= 0x122C,
	ITEMID_BLOOD4				= 0x122D,
	ITEMID_BLOOD5				= 0x122E,
	ITEMID_BLOOD6				= 0x122F,

	ITEMID_ROCK_B_LO			= 0x134F,	// boulder
	ITEMID_ROCK_B_HI			= 0x1362,
	ITEMID_ROCK_2_LO			= 0x1363,
	ITEMID_ROCK_2_HI			= 0x136D,

	ITEMID_BEE_WAX				= 0x1423,
	ITEMID_BONE_ARMS			= 0x144E,
	ITEMID_BONE_ARMOR			= 0x144F,
	ITEMID_BONE_GLOVES			= 0x1450,
	ITEMID_BONE_HELM			= 0x1451,
	ITEMID_BONE_LEGS			= 0x1452,

	ITEMID_BLOOD_SPLAT			= 0x1645,
	ITEMID_LIGHT_SRC			= 0x1647,

	ITEMID_KEY_RING1			= 0x1769,
	ITEMID_KEY_RING3			= 0x176A,
	ITEMID_KEY_RING5			= 0x176B,

	ITEMID_FIRE					= 0x19AB,
	ITEMID_ORE_1				= 0x19B7,	// can't mine this, it's just leftover from smelting
	ITEMID_ORE_3				= 0x19B8,
	ITEMID_ORE_4				= 0x19B9,
	ITEMID_ORE_2				= 0x19BA,

	ITEMID_BOARD1				= 0x1BD7,	// boards
	ITEMID_LOG_1				= 0x1BDD,

	ITEMID_Bulletin1			= 0x1E5E,	// secure trades are here also. bboard
	ITEMID_Bulletin2			= 0x1E5F,
	ITEMID_WorldGem				= 0x1EA7,	// typically an uninitialized spawn item
	ITEMID_DOOR_MAGIC_SI_NS		= 0x1ED9,
	ITEMID_DOOR_MAGIC_SI_EW		= 0x1EEC,

	ITEMID_ROBE					= 0x1F03,
	ITEMID_WorldGem_lg			= 0x1F13,	// typically an initialized t_spawn_item
	ITEMID_DOOR_METAL_BARRED_2	= 0x1FED,

	ITEMID_CORPSE				= 0x2006,	// this is all corpses
	ITEMID_MEMORY				= 0x2007,	// NonGen Marker
	ITEMID_HAIR_SHORT			= 0x203B,
	ITEMID_HAIR_LONG			= 0x203C,
	ITEMID_HAIR_PONYTAIL		= 0x203D,
	ITEMID_BEARD_LONG			= 0x203E,
	ITEMID_BEARD_SHORT			= 0x203F,
	ITEMID_BEARD_GOATEE			= 0x2040,
	ITEMID_BEARD_MUST			= 0x2041,
	ITEMID_HAIR_MOHAWK			= 0x2044,
	ITEMID_HAIR_PAGEBOY			= 0x2045,
	ITEMID_HAIR_BUNS			= 0x2046,
	ITEMID_HAIR_CURLY			= 0x2047,
	ITEMID_HAIR_RECEDING		= 0x2048,
	ITEMID_HAIR_2_TAILS			= 0x2049,
	ITEMID_HAIR_TOPKNOT			= 0x204A,
	ITEMID_BEARD_SHORT_MUST		= 0x204B,
	ITEMID_BEARD_LONG_MUST		= 0x204C,
	ITEMID_BEARD_VANDYKE		= 0x204D,
	ITEMID_DEATHSHROUD			= 0x204E,
	ITEMID_GM_ROBE				= 0x204F,
	ITEMID_RHAND_POINT_NW		= 0x2053,	// point nw on the map
	ITEMID_RHAND_POINT_W		= 0x205A,

	// Item equiv of creatures.
	ITEMID_TRACK_BEGIN			= 0x20C8,
	ITEMID_TRACK_OGRE			= 0x20DF,
	ITEMID_TRACK_WISP			= 0x2100,
	ITEMID_TRACK_MAN			= 0x2106,
	ITEMID_TRACK_WOMAN			= 0x2107,
	ITEMID_TRACK_HORSE			= 0x2120,
	ITEMID_TRACK_END			= 0x213E,

	ITEMID_VENDOR_BOX			= 0x2AF8,	// vendor container

	ITEMID_HAIR_MID_LONG		= 0x2FBF,
	ITEMID_HAIR_LONG_FEATHER	= 0x2FC0,
	ITEMID_HAIR_SHORT_ELF		= 0x2FC1,
	ITEMID_HAIR_MULLET			= 0x2FC2,
	ITEMID_HAIR_FLOWER			= 0x2FCC,
	ITEMID_HAIR_LONG_ELF		= 0x2FCD,
	ITEMID_HAIR_TOPKNOT_ELF		= 0x2FCE,
	ITEMID_HAIR_LONG_BRAID		= 0x2FCF,
	ITEMID_HAIR_BUNS_ELF		= 0x2FD0,
	ITEMID_HAIR_SPIKED			= 0x2FD1,

	ITEMID_DIRT_TILE			= 0x31F4,

	ITEMID_GAME1_CHECKER		= 0x3584,	// white
	ITEMID_GAME1_BISHOP			= 0x3585,
	ITEMID_GAME1_ROOK			= 0x3586,
	ITEMID_GAME1_QUEEN			= 0x3587,
	ITEMID_GAME1_KNIGHT			= 0x3588,
	ITEMID_GAME1_PAWN			= 0x3589,
	ITEMID_GAME1_KING			= 0x358A,
	ITEMID_GAME2_CHECKER		= 0x358B,	// brown
	ITEMID_GAME2_BISHOP			= 0x358C,
	ITEMID_GAME2_ROOK			= 0x358D,
	ITEMID_GAME2_QUEEN			= 0x358E,
	ITEMID_GAME2_KNIGHT			= 0x358F,
	ITEMID_GAME2_PAWN			= 0x3590,
	ITEMID_GAME2_KING			= 0x3591,
	ITEMID_GAME_HI				= 0x35A1,	// ?

	ITEMID_FX_EXPLODE			= 0x36B0,
	ITEMID_FX_FIRE_BALL			= 0x36D4,

	ITEMID_FX_FLAMESTRIKE		= 0x3709,
	ITEMID_FX_TELE_VANISH		= 0x3728,
	ITEMID_FX_SPELL_FAIL		= 0x3735,
	ITEMID_FX_CURSE_EFFECT		= 0x374A,
	ITEMID_FX_HEAL_EFFECT		= 0x376A,
	ITEMID_FX_SPARKLE_2			= 0x3779,
	ITEMID_FX_VORTEX			= 0x3789,
	ITEMID_FX_GLOW				= 0x37B9,

	ITEMID_FX_POISON_F_EW		= 0x3915,
	ITEMID_FX_POISON_F_NS		= 0x3920,
	ITEMID_FX_ENERGY_F_EW		= 0x3947,
	ITEMID_FX_ENERGY_F_NS		= 0x3956,
	ITEMID_FX_PARA_F_EW			= 0x3967,
	ITEMID_FX_PARA_F_NS			= 0x3979,
	ITEMID_FX_FIRE_F_EW			= 0x398C,
	ITEMID_FX_FIRE_F_NS			= 0x3996,

	ITEMID_FACE_1				= 0x3B44,
	ITEMID_FACE_2				= 0x3B45,
	ITEMID_FACE_3				= 0x3B46,
	ITEMID_FACE_4				= 0x3B47,
	ITEMID_FACE_5				= 0x3B48,
	ITEMID_FACE_6				= 0x3B49,
	ITEMID_FACE_7				= 0x3B4A,
	ITEMID_FACE_8				= 0x3B4B,
	ITEMID_FACE_9				= 0x3B4C,
	ITEMID_FACE_10				= 0x3B4D,
	ITEMID_FACE_ANIME			= 0x3B4E,
	ITEMID_FACE_HELLIAN			= 0x3B4F,
	ITEMID_FACE_JUKA			= 0x3B50,
	ITEMID_FACE_UNDEAD			= 0x3B51,
	ITEMID_FACE_MEER			= 0x3B52,
	ITEMID_FACE_ELDER			= 0x3B53,
	ITEMID_FACE_ORC				= 0x3B54,
	ITEMID_FACE_PIRATE			= 0x3B55,
	ITEMID_FACE_NATIVE_PAPUAN	= 0x3B56,
	ITEMID_FACE_VAMPIRE			= 0x3B57,

	ITEMID_MEMORY_SHIP_PILOT	= 0x3E96,

	ITEMID_HEALING_STONE		= 0x4078,

	ITEMID_GARG_HORN_1			= 0x4258,
	ITEMID_GARG_HORN_2			= 0x4259,
	ITEMID_GARG_HORN_3			= 0x425A,
	ITEMID_GARG_HORN_4			= 0x425B,
	ITEMID_GARG_HORN_5			= 0x425C,
	ITEMID_GARG_HORN_6			= 0x425D,
	ITEMID_GARG_HORN_7			= 0x425E,
	ITEMID_GARG_HORN_8			= 0x425F,
	ITEMID_GARG_HORN_FEMALE_1	= 0x4261,
	ITEMID_GARG_HORN_FEMALE_2	= 0x4262,
	ITEMID_GARG_HORN_FEMALE_3	= 0x4273,
	ITEMID_GARG_HORN_FEMALE_4	= 0x4274,
	ITEMID_GARG_HORN_FEMALE_5	= 0x4275,
	ITEMID_GARG_HORN_FEMALE_6	= 0x42AA,
	ITEMID_GARG_HORN_FEMALE_7	= 0x42AB,
	ITEMID_GARG_HORN_FACIAL_1	= 0x42AD,
	ITEMID_GARG_HORN_FACIAL_2	= 0x42AE,
	ITEMID_GARG_HORN_FACIAL_3	= 0x42AF,
	ITEMID_GARG_HORN_FACIAL_4	= 0x42B0,
	ITEMID_GARG_HORN_FEMALE_8	= 0x42B1,

	ITEMID_FACE_1_GARG			= 0x5679,
	ITEMID_FACE_2_GARG			= 0x567A,
	ITEMID_FACE_3_GARG			= 0x567B,
	ITEMID_FACE_4_GARG			= 0x567C,
	ITEMID_FACE_5_GARG			= 0x567D,
	ITEMID_FACE_6_GARG			= 0x567E,

	ITEMID_QTY					= 0x10000,

	// Multi slots
	ITEMID_MULTI_LEGACY			= 0x4000,	// ITEMID_MULTI for old clients (< 7.0.0.0)
	ITEMID_MULTI_SA				= 0x8000,	// ITEMID_MULTI for SA clients (< 7.0.8.44)
	ITEMID_MULTI				= 0x10000,

	ITEMID_SHIP_SMALL_N			= ITEMID_MULTI + 0x0,
	ITEMID_GALLEON_BRIT2_W		= ITEMID_MULTI + 0x47,

	ITEMID_HOUSE_SMALL_ST_PL	= ITEMID_MULTI + 0x64,
	ITEMID_HOUSE_SMALL_SHOP_MB	= ITEMID_MULTI + 0xA2,

	ITEMID_FOUNDATION_7x7		= ITEMID_MULTI + 0x13EC,
	ITEMID_FOUNDATION_30x30		= ITEMID_MULTI + 0x147D,

	ITEMID_HOUSE_TRINSIC_KEEP	= ITEMID_MULTI + 0x147E,
	ITEMID_HOUSE_GRIMSWIND_SISTERS_CASTLE	= ITEMID_MULTI + 0x148F,

	ITEMID_MULTI_MAX			= ITEMID_MULTI + MULTI_QTY - 1,

	// Reserved slots
	ITEMID_SCRIPT				= ITEMID_QTY,				// safe area for server specific items defintions
	ITEMID_TEMPLATE				= ITEMID_SCRIPT + 0x9FFF	// container item templates are beyond here
};

// Door ID Attribute flags.
#define DOOR_OPENED			0x1
#define DOOR_RIGHTLEFT		0x2
#define DOOR_INOUT			0x4
#define DOOR_NORTHSOUTH		0x8

enum CREID_TYPE		// enum the creature art work. (dont allow any others !) also know as "model number"
{
	CREID_INVALID			= 0x0,

	CREID_OGRE				= 0x1,
	CREID_ETTIN				= 0x2,
	CREID_ZOMBIE			= 0x3,
	CREID_GARGOYLE			= 0x4,
	CREID_EAGLE				= 0x5,
	CREID_BIRD				= 0x6,
	CREID_ORC_LORD			= 0x7,
	CREID_CORPSER			= 0x8,
	CREID_DAEMON			= 0x9,
	CREID_DAEMON_SWORD		= 0xA,
	CREID_DRAGON_GREY		= 0xC,
	CREID_AIR_ELEM			= 0xD,
	CREID_EARTH_ELEM		= 0xE,
	CREID_FIRE_ELEM			= 0xF,

	CREID_WATER_ELEM		= 0x10,
	CREID_ORC				= 0x11,
	CREID_ETTIN_AXE			= 0x12,
	CREID_LICH				= 0x18,
	CREID_SPECTRE			= 0x1A,
	CREID_GIANT_SPIDER		= 0x1C,
	CREID_GORILLA			= 0x1D,

	CREID_LIZMAN			= 0x21,
	CREID_LIZMAN_SPEAR		= 0x23,
	CREID_LIZMAN_MACE		= 0x24,
	CREID_ORC_CLUB			= 0x29,
	CREID_RATMAN			= 0x2A,
	CREID_RATMAN_CLUB		= 0x2C,
	CREID_RATMAN_SWORD		= 0x2D,

	CREID_SKELETON			= 0x32,
	CREID_Snake				= 0x34,
	CREID_TROLL_SWORD		= 0x35,
	CREID_TROLL				= 0x36,
	CREID_TROLL_MACE		= 0x37,
	CREID_SKEL_AXE			= 0x38,
	CREID_SKEL_SW_SH		= 0x39,		// sword and sheild
	CREID_DRAGON_RED		= 0x3B,
	CREID_DRAKE_GREY		= 0x3C,
	CREID_DRAKE_RED			= 0x3D,

	CREID_Tera_Warrior		= 0x46,
	CREID_Tera_Drone		= 0x47,
	CREID_Tera_Matriarch	= 0x48,
	CREID_Cyclops			= 0x4C,

	CREID_Bull_Frog			= 0x51,
	CREID_Ophid_Mage		= 0x55,
	CREID_Ophid_Warrior		= 0x56,
	CREID_Ophid_Queen		= 0x57,
	CREID_SEA_Creature		= 0x5F,

	CREID_SERPENTINE_DRAGON	= 0x67,
	CREID_SKELETAL_DRAGON	= 0x68,

	CREID_Dolphin			= 0x97,

	CREID_VORTEX			= 0xA4,

	// Animals (Low detail critters)

	CREID_HORSE1			= 0xC8,		// white
	CREID_Pig				= 0xCB,
	CREID_HORSE4			= 0xCC,		// brown
	CREID_Sheep				= 0xCF,		// un-sheered
	CREID_Ostard_Desert		= 0xD2,
	CREID_BrownBear			= 0xD3,
	CREID_GrizzlyBear		= 0xD4,
	CREID_PolarBear			= 0xD5,
	CREID_Cow_BW			= 0xD8,
	CREID_Ostard_Frenz		= 0xDA,
	CREID_Ostard_Forest		= 0xDB,
	CREID_Llama				= 0xDC,
	CREID_Sheep_Sheered		= 0xDF,
	CREID_HORSE2			= 0xE2,
	CREID_HORSE3			= 0xE4,
	CREID_Cow2				= 0xE7,
	CREID_Bull_Brown		= 0xE8,		// light brown
	CREID_Bull2				= 0xE9,		// dark brown
	CREID_Hart				= 0xEA,
	CREID_Deer				= 0xED,

	CREID_REAPER_FORM		= 0x11D,

	CREID_Boar				= 0x122,	// large pig
	CREID_HORSE_PACK		= 0x123,
	CREID_LLAMA_PACK		= 0x124,

	// all below here are humanish or clothing
	CREID_MAN				= 0x190,
	CREID_WOMAN				= 0x191,
	CREID_GHOSTMAN			= 0x192,
	CREID_GHOSTWOMAN		= 0x193,

	CREID_BLADES			= 0x23E,

	CREID_ELFMAN			= 0x25D,
	CREID_ELFWOMAN			= 0x25E,
	CREID_ELFGHOSTMAN		= 0x25F,

	CREID_ELFGHOSTWOMAN		= 0x260,
 
	CREID_GARGMAN			= 0x29A,
	CREID_GARGWOMAN			= 0x29B,

	CREID_GARGGHOSTMAN		= 0x2B6,
	CREID_GARGGHOSTWOMAN	= 0x2B7,

	CREID_STONE_FORM		= 0x2C1,

	CREID_VAMPIREMAN		= 0x2E8,
	CREID_VAMPIREWOMAN		= 0x2E9,
	CREID_HORRIFIC_BEAST	= 0x2EA,
	CREID_WAILING_BANSHEE2	= 0x2EB,
	CREID_WRAITH			= 0x2EC,
	CREID_LICH_FORM			= 0x2ED,
	CREID_REVENANT			= 0x2EE,

	CREID_IRON_GOLEM		= 0x2F0,

	CREID_GIANT_BEETLE		= 0x317,
	CREID_SWAMP_DRAGON1		= 0x31A,
	CREID_REPTILE_LORD		= 0x31D,
	CREID_ANCIENT_WYRM		= 0x31E,
	CREID_SWAMP_DRAGON2		= 0x31F,

	CREID_EQUIP_GM_ROBE		= 0x3DB,

	CREID_QTY				= 0x800,	// max number of base character types, based on art work

	NPCID_SCRIPT			= 0x4000,	// safe area for server specific NPC defintions
	SPAWNTYPE_START			= 0x8000
};

enum ANIM_TYPE	// not all creatures animate the same for some reason.
{
	ANIM_WALK_UNARM		= 0x00,	// Walk (unarmed)

	// human anim. (supported by all humans)
	ANIM_WALK_ARM		= 0x01,	// Walk (armed) (but not war mode)

	ANIM_RUN_UNARM		= 0x02,	// Run (unarmed)
	ANIM_RUN_ARMED		= 0x03,	// Run (armed)

	ANIM_STAND			= 0x04,	// armed or unarmed.

	ANIM_FIDGET1		= 0x05,	// Look around
	ANIM_FIDGET_YAWN	= 0x06,	// Fidget, Yawn

	ANIM_STAND_WAR_1H	= 0x07,	// Stand for 1 hand attack.
	ANIM_STAND_WAR_2H	= 0x08,	// Stand for 2 hand attack.

	ANIM_ATTACK_WEAPON		= 0x09,	// 1H generic melee swing, any weapon.
	ANIM_ATTACK_1H_SLASH	= 0x09,
	ANIM_ATTACK_1H_PIERCE	= 0x0a,
	ANIM_ATTACK_1H_BASH		= 0x0b,

	ANIM_ATTACK_2H_BASH		= 0x0c,
	ANIM_ATTACK_2H_SLASH	= 0x0d,
	ANIM_ATTACK_2H_PIERCE	= 0x0e,

	ANIM_WALK_WAR		= 0x0f,	// Walk (warmode)

	ANIM_CAST_DIR		= 0x10,	// Directional spellcast
	ANIM_CAST_AREA		= 0x11,	// Area-effect spellcast

	ANIM_ATTACK_BOW		= 0x12,	// Bow attack / Mounted bow attack
	ANIM_ATTACK_XBOW	= 0x13,	// Crossbow attack
	ANIM_GET_HIT		= 0x14,	// Take a hit

	ANIM_DIE_BACK		= 0x15,	// (Die onto back)
	ANIM_DIE_FORWARD	= 0x16,	// (Die onto face)

	ANIM_BLOCK			= 0x1e,	// Dodge, Shield Block
	ANIM_ATTACK_WRESTLE	= 0x1f,	// Punch - attack while walking ?

	ANIM_BOW			= 0x20, // =32
	ANIM_SALUTE			= 0x21,
	ANIM_EAT			= 0x22,

	// don't use these directly these are just for translation.
	// Human on horseback
	ANIM_HORSE_RIDE_SLOW	= 0x17,
	ANIM_HORSE_RIDE_FAST	= 0x18,
	ANIM_HORSE_STAND		= 0x19,
	ANIM_HORSE_ATTACK		= 0x1a,
	ANIM_HORSE_ATTACK_BOW	= 0x1b,
	ANIM_HORSE_ATTACK_XBOW	= 0x1c,
	ANIM_HORSE_SLAP			= 0x1d,

	ANIM_QTY_MAN = 35,	// 0x23

	ANIM_MAN_SIT = 35,

	// monster anim	- (not all anims are supported for each creature)
	ANIM_MON_WALK 		= 0x00,
	ANIM_MON_STAND		= 0x01,
	ANIM_MON_DIE1		= 0x02,	// back
	ANIM_MON_DIE2		= 0x03, // fore or side.
	ANIM_MON_ATTACK1	= 0x04,	// ALL creatures have at least this attack.
	ANIM_MON_ATTACK2	= 0x05,	// swimming monsteers don't have this.
	ANIM_MON_ATTACK3	= 0x06,
	ANIM_MON_AttackBow	= 0x07, // air/fire elem = flail arms.
	ANIM_MON_AttackXBow = 0x08,	// Misc Roll over,
	ANIM_MON_AttackThrow,
	ANIM_MON_GETHIT 	= 0x0a,
	ANIM_MON_PILLAGE	= 0x0b,	// 11 = Misc, Stomp, slap ground, lich conjure.
	ANIM_MON_Stomp		= 0x0c,	// Misc Cast, breath fire, elem creation.
	ANIM_MON_Cast2		= 0x0d,	// 13 = Trolls don't have this.
	ANIM_MON_Cast3,	
	ANIM_MON_BlockRight	= 0x0f,
	ANIM_MON_BlockLeft	= 0x10,
	ANIM_MON_FIDGET1	= 0x11,	// 17=Idle
	ANIM_MON_FIDGET2	= 0x12,	// 18
	ANIM_MON_FLY		= 0x13,
	ANIM_MON_LAND		= 0x14,	// TakeOff
	ANIM_MON_DIE_FLIGHT	= 0x15,	// GetHitInAir

	ANIM_QTY_MON = 22,

	// animals. (Most All animals have all anims)
	ANIM_ANI_WALK		= 0x00,
	ANIM_ANI_RUN		= 0x01,
	ANIM_ANI_STAND		= 0x02,
	ANIM_ANI_EAT		= 0x03,
	ANIM_ANI_ALERT,				// not all have this.
	ANIM_ANI_ATTACK1	= 0x05,
	ANIM_ANI_ATTACK2	= 0x06,
	ANIM_ANI_GETHIT 	= 0x07,
	ANIM_ANI_DIE1 		= 0x08,
	ANIM_ANI_FIDGET1	= 0x09,	// Idle
	ANIM_ANI_FIDGET2	= 0x0a,
	ANIM_ANI_SLEEP		= 0x0b,	// lie down (not all have this)
	ANIM_ANI_DIE2		= 0x0c,

	ANIM_QTY_ANI = 13,

	ANIM_QTY = 0x32
};

enum ANIM_TYPE_NEW	// not all creatures animate the same for some reason. http://img546.imageshack.us/img546/5439/uonewanimstable2.png
{
	NANIM_ATTACK		= 0x00,	// 8 SUB ANIMATIONS, VARIATION 0-*
	NANIM_BLOCK			= 0x01,			// VARIATION 0-1
	NANIM_BLOCK2		= 0x02,			// MONSTERS, VARIATION 0-1
	NANIM_DEATH			= 0x03,			// VARIATION 0-1
	NANIM_GETHIT		= 0x04,			// VARIATION 0-*
	NANIM_IDLE			= 0x05,
	NANIM_EAT			= 0x06,
	NANIM_EMOTE			= 0x07,			// 2 SUB ANIMATIONS
	NANIM_ANGER			= 0x08,
	NANIM_TAKEOFF		= 0x09,
	NANIM_LANDING		= 0x0a,
	NANIM_SPELL			= 0x0b,			// 2 SUB ANIMATIONS
	NANIM_UNKNOWN1		= 0x0c,			// According to RUOSI now this is StartCombat
	NANIM_UNKNOWN2		= 0x0d,			// and this one EndCombat (Maybe only for EC?)
	NANIM_PILLAGE		= 0x0e,			// Human/Animal (eat), Monster (pillage)
	NANIM_RISE			= 0x0f,			// Used on character creation (Only EC)
	
	NANIM_QTY = 16,

	NANIM_ATTACK_WRESTLING	= 0x00,
	NANIM_ATTACK_BOW		= 0x01,
	NANIM_ATTACK_CROSSBOW	= 0x02,
	NANIM_ATTACK_1H_BASH	= 0x03,
	NANIM_ATTACK_1H_SLASH	= 0x04,
	NANIM_ATTACK_1H_PIERCE	= 0x05,
	NANIM_ATTACK_2H_BASH	= 0x06,
	NANIM_ATTACK_2H_SLASH	= 0x07,
	NANIM_ATTACK_2H_PIERCE	= 0x08,
	NANIM_ATTACK_THROWING	= 0x09,

	NANIM_ATTACK_QTY = 10,

	NANIM_EMOTE_BOW		= 0x00,
	NANIM_EMOTE_SALUTE	= 0x01,
	
	NANIM_EMOTE_QTY = 2,

	NANIM_SPELL_NORMAL	= 0x00,
	NANIM_SPELL_SUMMON	= 0x01,

	NANIM_SPELL_QTY = 2
};

enum CRESND_TYPE	// Placeholders (not real sound IDs): the SoundChar method chooses the best sound for each creature
{
	CRESND_RAND = -1,	// pick up randomly CRESND_IDLE or CRESND_NOTICE
	CRESND_IDLE = 0,	// just random noise. or default "no" response
	CRESND_NOTICE,		// just random noise. or default "yes" response
	CRESND_HIT,
	CRESND_GETHIT,
	CRESND_DIE
};

enum FONT_TYPE
{
	FONT_BOLD,		// 0 - Bold Text = Large plain filled block letters.
	FONT_SHAD,		// 1 - Text with shadow = small gray
	FONT_BOLD_SHAD,	// 2 - Bold+Shadow = Large Gray block letters.
	FONT_NORMAL,	// 3 - Normal (default) = Filled block letters.
	FONT_GOTH,		// 4 - Gothic = Very large blue letters.
	FONT_ITAL,		// 5 - Italic Script
	FONT_SM_DARK,	// 6 - Small Dark Letters = small Blue
	FONT_COLOR,		// 7 - Colorful Font (Buggy?) = small Gray (hazy)
	FONT_RUNE,		// 8 - Rune font (Only use capital letters with this!)
	FONT_SM_LITE,	// 9 - Small Light Letters = small roman gray font.
	FONT_QTY
};

enum AFFIX_TYPE
{
	AFFIX_APPEND	= 0x0,	// 0 - Append affix to end of message
	AFFIX_PREPEND	= 0x1,	// 1 - Prepend affix to front of message
	AFFIX_SYSTEM	= 0x2	// 2 - Message is displayed as a system message
};

enum DIR_TYPE	// Walking directions. m_dir
{
	DIR_INVALID = -1,

	DIR_N = 0,
	DIR_NE,
	DIR_E,
	DIR_SE,
	DIR_S,
	DIR_SW,
	DIR_W,
	DIR_NW,
	DIR_QTY		// also means "center"
};

enum SKILL_TYPE	// List of skill numbers (things that can be done at a given time)
{
	SKILL_NONE = -1,

	SKILL_ALCHEMY,
	SKILL_ANATOMY,
	SKILL_ANIMALLORE,
	SKILL_ITEMID,
	SKILL_ARMSLORE,
	SKILL_PARRYING,
	SKILL_BEGGING,
	SKILL_BLACKSMITHING,
	SKILL_BOWCRAFT,
	SKILL_PEACEMAKING,
	SKILL_CAMPING,
	SKILL_CARPENTRY,
	SKILL_CARTOGRAPHY,
	SKILL_COOKING,
	SKILL_DETECTINGHIDDEN,
	SKILL_ENTICEMENT,
	SKILL_EVALINT,
	SKILL_HEALING,
	SKILL_FISHING,
	SKILL_FORENSICS,
	SKILL_HERDING,
	SKILL_HIDING,
	SKILL_PROVOCATION,
	SKILL_INSCRIPTION,
	SKILL_LOCKPICKING,
	SKILL_MAGERY,
	SKILL_MAGICRESISTANCE,
	SKILL_TACTICS,
	SKILL_SNOOPING,
	SKILL_MUSICIANSHIP,
	SKILL_POISONING,
	SKILL_ARCHERY,
	SKILL_SPIRITSPEAK,
	SKILL_STEALING,
	SKILL_TAILORING,
	SKILL_TAMING,
	SKILL_TASTEID,
	SKILL_TINKERING,
	SKILL_TRACKING,
	SKILL_VETERINARY,
	SKILL_SWORDSMANSHIP,
	SKILL_MACEFIGHTING,
	SKILL_FENCING,
	SKILL_WRESTLING,
	SKILL_LUMBERJACKING,
	SKILL_MINING,
	SKILL_MEDITATION,
	SKILL_STEALTH,
	SKILL_REMOVETRAP,
	//AOS
	SKILL_NECROMANCY,
	SKILL_FOCUS,
	SKILL_CHIVALRY,
	//SE
	SKILL_BUSHIDO,
	SKILL_NINJITSU,
	//ML
	SKILL_SPELLWEAVING,
	//SA
	SKILL_MYSTICISM,
	SKILL_IMBUING,
	SKILL_THROWING,

	/**
	 * Skill level limit. Should not used directly, most cases are covered by g_Cfg.m_iMaxSkill instead
	 */
	SKILL_QTY = 99,

	// Actions a npc will perform. (no need to track skill level for these)
	NPCACT_FOLLOW_TARG = 100,	// 100 = following a char.
	NPCACT_STAY,				// 101
	NPCACT_GOTO,				// 102 = Go to a location x,y. Pet command
	NPCACT_WANDER,				// 103 = Wander aimlessly.
	NPCACT_LOOKING,				// 104 = just look around intently.
	NPCACT_FLEE,				// 105 = Run away from target. m_Act_Targ
	NPCACT_TALK,				// 106 = Talking to my target. m_Act_Targ
	NPCACT_TALK_FOLLOW,			// 107 = m_Act_Targ / m_Fight_Targ.
	NPCACT_GUARD_TARG,			// 108 = Guard a targetted object. m_Act_Targ
	NPCACT_GO_HOME,				// 109 =
	NPCACT_BREATH,				// 110 = Using breath weapon. on m_Fight_Targ.
	NPCACT_RIDDEN,				// 111 = Being ridden or shrunk as figurine.
	NPCACT_THROWING,			// 112 = Throwing a stone at m_Fight_Targ.
	NPCACT_TRAINING,			// 113 = using a training dummy etc.
	NPCACT_FOOD,				// 114 = Searching for food
	NPCACT_RUNTO,				// 115 = Run to a location x,y.
	NPCACT_QTY
};

enum LAYER_TYPE		// defined by UO. Only one item can be in a slot.
{
	LAYER_NONE = 0,	// spells that are layed on the CChar ?
	LAYER_HAND1,	// 1 = spellbook or weapon.
	LAYER_HAND2,	// 2 = other hand or 2 handed thing. = shield (also used for light halo 'ITEMID_LIGHT_SRC')
	LAYER_SHOES,	// 3
	LAYER_PANTS,	// 4 = bone legs + pants.
	LAYER_SHIRT,
	LAYER_HELM,		// 6
	LAYER_GLOVES,	// 7
	LAYER_RING,
	LAYER_TALISMAN,	// 9 = talisman item
	LAYER_COLLAR,	// 10 = gorget or necklace.
	LAYER_HAIR,		// 11 = 0x0b =
	LAYER_HALF_APRON,// 12 = 0x0c =
	LAYER_CHEST,	// 13 = 0x0d = armor chest
	LAYER_WRIST,	// 14 = 0x0e = watch
	LAYER_FACE,		// 15 = character face style on enhanced clients
	LAYER_BEARD,	// 16 = try to have only men have this.
	LAYER_TUNIC,	// 17 = jester suit or full apron.
	LAYER_EARS,		// 18 = earrings
	LAYER_ARMS,		// 19 = armor
	LAYER_CAPE,		// 20 = cape
	LAYER_PACK,		// 21 = 0x15 = only used by ITEMID_BACKPACK
	LAYER_ROBE,		// 22 = robe over all.
	LAYER_SKIRT,	// 23 = skirt or kilt.
	LAYER_LEGS,		// 24= 0x18 = plate legs.

	// These are not part of the paperdoll (but get sent to the client)
	LAYER_HORSE,		// 25 = 0x19 = ride this object. (horse objects are strange?)
	LAYER_VENDOR_STOCK,	// 26 = 0x1a = the stuff the vendor will restock and sell to the players
	LAYER_VENDOR_EXTRA,	// 27 = 0x1b = the stuff the vendor will resell to players but is not restocked. (bought from players)
	LAYER_VENDOR_BUYS,	// 28 = 0x1c = the stuff the vendor can buy from players but does not stock.
	LAYER_BANKBOX,		// 29 = 0x1d = contents of my bank box.

	// Internally used layers - Don't bother sending these to client.
	LAYER_SPECIAL,		// 30 =	Can be multiple of these. memories
	LAYER_DRAGGING,

	// Spells that are effecting us go here.
	LAYER_SPELL_STATS,			// 32 = Stats effecting spell. These cancel each other out.
	LAYER_SPELL_Reactive,		// 33 =
	LAYER_SPELL_Night_Sight,
	LAYER_SPELL_Protection,		// 35
	LAYER_SPELL_Incognito,
	LAYER_SPELL_Magic_Reflect,
	LAYER_SPELL_Paralyze,		// or turned to stone.
	LAYER_SPELL_Invis,
	LAYER_SPELL_Polymorph,		// 40
	LAYER_SPELL_Summon,			// 41 = magical summoned creature.

	LAYER_FLAG_Poison,			// 42
	LAYER_FLAG_Criminal,		// criminal or murderer ? decay over time
	LAYER_FLAG_Potion,			// Some magic type effect done by a potion. (they cannot be dispelled)
	LAYER_FLAG_SpiritSpeak,		// 45
	LAYER_FLAG_Wool,			// regrowing wool.
	LAYER_FLAG_Drunk,			// Booze effect.
	LAYER_FLAG_ClientLinger,	// 48
	LAYER_FLAG_Hallucination,	// shrooms etc.
	LAYER_FLAG_PotionUsed,		// 50 = track the time till we can use a potion again.
	LAYER_FLAG_Stuck,			// In a trap or web.
	LAYER_FLAG_Murders,			// How many murders do we have ? decay over time
	LAYER_FLAG_Bandage,			// 53 = Bandages go here for healing

	LAYER_AUCTION,				// Auction layer

	//Necro
	LAYER_SPELL_Blood_Oath,
	LAYER_SPELL_Curse_Weapon,
	LAYER_SPELL_Corpse_Skin,
	LAYER_SPELL_Evil_Omen,
	LAYER_SPELL_Pain_Spike,
	LAYER_SPELL_Mind_Rot,
	LAYER_SPELL_Strangle,

	//Ninjitsu
	//LAYER_SPELL_Surprise_Attack,

	//Chivalry
	LAYER_SPELL_Consecrate_Weapon,
	LAYER_SPELL_Divine_Fury,
	LAYER_SPELL_Enemy_Of_One,

	//SpellWeaving
	LAYER_SPELL_Attunement,
	LAYER_SPELL_Gift_Of_Renewal,
	LAYER_SPELL_Immolating_Weapon,
	LAYER_SPELL_Thunderstorm,
	LAYER_SPELL_Arcane_Empowerment,
	LAYER_SPELL_Ethereal_Voyage,
	LAYER_SPELL_Gift_Of_Life,
	LAYER_SPELL_Dryad_Allure,
	LAYER_SPELL_Essence_Of_Wind,

	//Mysticism
	LAYER_SPELL_Sleep,
	LAYER_SPELL_Bombard,
	LAYER_SPELL_Spell_Plague,
	LAYER_SPELL_Nether_Cyclone,
	
	LAYER_QTY
};

enum SPELL_TYPE	// List of spell numbers in spell book.
{
	SPELL_NONE = 0,

	// Magery
	SPELL_Clumsy = 1,		// 1st circle
	SPELL_Create_Food,
	SPELL_Feeblemind,
	SPELL_Heal,
	SPELL_Magic_Arrow,
	SPELL_Night_Sight,
	SPELL_Reactive_Armor,
	SPELL_Weaken,
	SPELL_Agility,			// 2nd circle
	SPELL_Cunning,
	SPELL_Cure,
	SPELL_Harm,
	SPELL_Magic_Trap,
	SPELL_Magic_Untrap,
	SPELL_Protection,
	SPELL_Strength,
	SPELL_Bless,			// 3rd circle
	SPELL_Fireball,
	SPELL_Magic_Lock,
	SPELL_Poison,
	SPELL_Telekin,
	SPELL_Teleport,
	SPELL_Unlock,
	SPELL_Wall_of_Stone,
	SPELL_Arch_Cure,		// 4th circle
	SPELL_Arch_Prot,
	SPELL_Curse,
	SPELL_Fire_Field,
	SPELL_Great_Heal,
	SPELL_Lightning,
	SPELL_Mana_Drain,
	SPELL_Recall,
	SPELL_Blade_Spirit,		// 5th circle
	SPELL_Dispel_Field,
	SPELL_Incognito,
	SPELL_Magic_Reflect,
	SPELL_Mind_Blast,
	SPELL_Paralyze,
	SPELL_Poison_Field,
	SPELL_Summon,
	SPELL_Dispel,			// 6th circle
	SPELL_Energy_Bolt,
	SPELL_Explosion,
	SPELL_Invis,
	SPELL_Mark,
	SPELL_Mass_Curse,
	SPELL_Paralyze_Field,
	SPELL_Reveal,
	SPELL_Chain_Lightning,	// 7th circle
	SPELL_Energy_Field,
	SPELL_Flame_Strike,
	SPELL_Gate_Travel,
	SPELL_Mana_Vamp,
	SPELL_Mass_Dispel,
	SPELL_Meteor_Swarm,
	SPELL_Polymorph,
	SPELL_Earthquake,		// 8th circle
	SPELL_Vortex,
	SPELL_Resurrection,
	SPELL_Air_Elem,
	SPELL_Daemon,
	SPELL_Earth_Elem,
	SPELL_Fire_Elem,
	SPELL_Water_Elem,
	SPELL_MAGERY_QTY = SPELL_Water_Elem,

	// Necromancy (AOS)
	SPELL_Animate_Dead_AOS = 101,
	SPELL_Blood_Oath,
	SPELL_Corpse_Skin,
	SPELL_Curse_Weapon,
	SPELL_Evil_Omen,
	SPELL_Horrific_Beast,
	SPELL_Lich_Form,
	SPELL_Mind_Rot,
	SPELL_Pain_Spike,
	SPELL_Poison_Strike,
	SPELL_Strangle,
	SPELL_Summon_Familiar,
	SPELL_Vampiric_Embrace,
	SPELL_Vengeful_Spirit,
	SPELL_Wither,
	SPELL_Wraith_Form,
	SPELL_Exorcism,
	SPELL_NECROMANCY_QTY = SPELL_Exorcism,

	// Chivalry (AOS)
	SPELL_Cleanse_by_Fire = 201,
	SPELL_Close_Wounds,
	SPELL_Consecrate_Weapon,
	SPELL_Dispel_Evil,
	SPELL_Divine_Fury,
	SPELL_Enemy_of_One,
	SPELL_Holy_Light,
	SPELL_Noble_Sacrifice,
	SPELL_Remove_Curse,
	SPELL_Sacred_Journey,
	SPELL_CHIVALRY_QTY = SPELL_Sacred_Journey,

	// Bushido (SE)
	SPELL_Honorable_Execution = 401,
	SPELL_Confidence,
	SPELL_Evasion,
	SPELL_Counter_Attack,
	SPELL_Lightning_Strike,
	SPELL_Momentum_Strike,
	SPELL_BUSHIDO_QTY = SPELL_Momentum_Strike,

	// Ninjitsu (SE)
	SPELL_Focus_Attack = 501,
	SPELL_Death_Strike,
	SPELL_Animal_Form,
	SPELL_Ki_Attack,
	SPELL_Surprise_Attack,
	SPELL_Backstab,
	SPELL_Shadowjump,
	SPELL_Mirror_Image,
	SPELL_NINJITSU_QTY = SPELL_Mirror_Image,

	// Spellweaving (ML)
	SPELL_Arcane_Circle = 601,
	SPELL_Gift_of_Renewal,
	SPELL_Immolating_Weapon,
	SPELL_Attunement,
	SPELL_Thunderstorm,
	SPELL_Natures_Fury,
	SPELL_Summon_Fey,
	SPELL_Summon_Fiend,
	SPELL_Reaper_Form,
	SPELL_Wildfire,
	SPELL_Essence_of_Wind,
	SPELL_Dryad_Allure,
	SPELL_Ethereal_Voyage,
	SPELL_Word_of_Death,
	SPELL_Gift_of_Life,
	SPELL_Arcane_Empowerment,
	SPELL_SPELLWEAVING_QTY = SPELL_Arcane_Empowerment,

	// Mysticism (SA)
	SPELL_Nether_Bolt = 678,
	SPELL_Healing_Stone,
	SPELL_Purge_Magic,
	SPELL_Enchant_Weapon,
	SPELL_Sleep,
	SPELL_Eagle_Strike,
	SPELL_Animated_Weapon,
	SPELL_Stone_Form,
	SPELL_Spell_Trigger,
	SPELL_Mass_Sleep,
	SPELL_Cleansing_Winds,
	SPELL_Bombard,
	SPELL_Spell_Plague,
	SPELL_Hail_Storm,
	SPELL_Nether_Cyclone,
	SPELL_Rising_Colossus,
	SPELL_MYSTICISM_QTY = SPELL_Rising_Colossus,

	// Bard Masteries (SA)
	SPELL_Inspire = 701,
	SPELL_Invigorate,
	SPELL_Resilience,
	SPELL_Perseverance,
	SPELL_Tribulation,
	SPELL_Despair,
	SPELL_BARDMASTERIES_QTY = SPELL_Despair,

	// Skill Masteries (TOL)
	SPELL_Death_Ray = 707,
	SPELL_Ethereal_Burst,
	SPELL_Nether_Blast,
	SPELL_Mystic_Weapon,
	SPELL_Command_Undead,
	SPELL_Conduit,
	SPELL_Mana_Shield,
	SPELL_Summon_Reaper,
	SPELL_Enchanted_Summoning,	// Passive
	SPELL_Anticipate_Hit,		// Passive
	SPELL_Warcry,
	SPELL_Intuition,			// Passive
	SPELL_Rejuvinate,
	SPELL_Holy_Fist,
	SPELL_Shadow,
	SPELL_White_Tiger_Form,
	SPELL_Flaming_Shot,
	SPELL_Playing_The_Odds,
	SPELL_Thrust,
	SPELL_Pierce,
	SPELL_Stagger,
	SPELL_Toughness,
	SPELL_Onslaught,
	SPELL_Focused_Eye,
	SPELL_Elemental_Fury,
	SPELL_Called_Shot,
	SPELL_Warriors_Gifts,		// Passive (previously known as Saving Throw)
	SPELL_Shield_Bash,
	SPELL_Body_Guard,
	SPELL_Heightened_Senses,
	SPELL_Tolerance,
	SPELL_Injected_Strike,
	SPELL_Potency,				// Passive
	SPELL_Rampage,
	SPELL_Fists_Of_Fury,
	SPELL_Knockout,				// Passive
	SPELL_Whispering,
	SPELL_Combat_Training,
	SPELL_Boarding,				// Passive
	SPELL_SKILLMASTERIES_QTY = SPELL_Boarding,

	// Custom Sphere spells (used by some monsters)
	SPELL_Summon_Undead = 1000,
	SPELL_Animate_Dead,
	SPELL_Bone_Armor,
	SPELL_Light,
	SPELL_Fire_Bolt,
	SPELL_Hallucination,
	SPELL_CUSTOM_QTY = SPELL_Hallucination,

	// Custom extra special spells (can be used as potion effects as well). Commented value = old index.
	SPELL_Stone,			// 71 = Turn to stone (permanent).
	SPELL_Shrink,			// 72 = turn pet into icon.
	SPELL_Refresh,			// 73 = stamina
	SPELL_Restore,			// 74 = This potion increases both your hit points and your stamina.
	SPELL_Mana,				// 75 = restone mana
	SPELL_Sustenance,		// 76 = serves to fill you up. (Remember, healing rate depends on how well fed you are!)
	SPELL_Chameleon,		// 77 = makes your skin match the colors of whatever is behind you.
	SPELL_BeastForm,		// 78 = polymorphs you into an animal for a while.
	SPELL_Monster_Form,		// 79 = polymorphs you into a monster for a while.
	SPELL_Gender_Swap,		// 81 = permanently changes your gender.
	SPELL_Trance,			// 82 = temporarily increases your meditation skill.
	SPELL_Particle_Form,	// 83 = turns you into an immobile, but untargetable particle system for a while.
	SPELL_Shield,			// 84 = erects a temporary force field around you. Nobody approaching will be able to get within 1 tile of you, though you can move close to them if you wish.
	SPELL_Steelskin,		// 85 = turns your skin into steel, giving a boost to your AR.
	SPELL_Stoneskin,		// 86 = turns your skin into stone, giving a boost to your AR.
	SPELL_Regenerate,		// 87 = regen hitpoints at a fast rate.
	SPELL_Enchant,			// 88 = Enchant an item (weapon or armor)
	SPELL_Forget,			// 89 = only existed in sphere_spells.scp before
	SPELL_Ale,				// 90 = drunkeness ?
	SPELL_Wine,				// 91 = mild drunkeness ?
	SPELL_Liquor,			// 92 = extreme drunkeness ?
	SPELL_QTY = SPELL_Liquor
};

enum LIGHT_PATTERN	// What pattern (m_light_pattern) does the light source (CAN_LIGHT) take.
{
	LIGHT_LARGE		= 1,
	LIGHT_SMALL		= 2,
	LIGHT_LARGEST	= 29,
	LIGHT_QTY		= 56	// this makes it go black
};

enum GUMP_TYPE	// The gumps. (most of these are not useful to the server.)
{
	GUMP_NONE					= -1,
	GUMP_SCROLL					= 0x7,
	GUMP_CORPSE					= 0x9,
	GUMP_VENDOR_RECT			= 0x30,
	GUMP_BACKPACK				= 0x3c,
	GUMP_BAG					= 0x3d,
	GUMP_BARREL					= 0x3e,
	GUMP_BASKET_SQUARE			= 0x3f,
	GUMP_BOX_WOOD				= 0x40,
	GUMP_BASKET_ROUND			= 0x41,
	GUMP_CHEST_METAL_GOLD		= 0x42,
	GUMP_BOX_WOOD_ORNATE		= 0x43,
	GUMP_CRATE					= 0x44,
	GUMP_LEATHER				= 0x47,
	GUMP_DRAWER_DARK			= 0x48,
	GUMP_CHEST_WOOD				= 0x49,
	GUMP_CHEST_METAL			= 0x4a,
	GUMP_BOX_METAL				= 0x4b,
	GUMP_SHIP_HATCH				= 0x4c,
	GUMP_BOOK_SHELF				= 0x4d,
	GUMP_CABINET_DARK			= 0x4e,
	GUMP_CABINET_LIGHT			= 0x4f,
	GUMP_DRAWER_LIGHT			= 0x51,
	GUMP_BULLETIN_BOARD			= 0x52,
	GUMP_GIFT_BOX				= 0x102,
	GUMP_STOCKING				= 0x103,
	GUMP_ARMOIRE_ELVEN_WASH		= 0x104,
	GUMP_ARMOIRE_RED			= 0x105,
	GUMP_ARMOIRE_MAPLE			= 0x106,
	GUMP_ARMOIRE_CHERRY			= 0x107,
	GUMP_BASKET_TALL			= 0x108,
	GUMP_CHEST_WOOD_PLAIN		= 0x109,
	GUMP_CHEST_WOOD_GILDED		= 0x10a,
	GUMP_CHEST_WOOD_ORNATE		= 0x10b,
	GUMP_TALL_CABINET			= 0x10c,
	GUMP_CHEST_WOOD_FINISH		= 0x10d,
	GUMP_CHEST_WOOD_FINISH2		= 0x10e,
	GUMP_BLESSED_STATUE			= 0x116,
	GUMP_MAILBOX				= 0x11a,
	GUMP_GIFT_BOX_CUBE			= 0x11b,
	GUMP_GIFT_BOX_CYLINDER		= 0x11c,
	GUMP_GIFT_BOX_OCTOGON		= 0x11d,
	GUMP_GIFT_BOX_RECTANGLE		= 0x11e,
	GUMP_GIFT_BOX_ANGEL			= 0x11f,
	GUMP_GIFT_BOX_HEART_SHAPED	= 0x120,
	GUMP_GIFT_BOX_TALL			= 0x121,
	GUMP_GIFT_BOX_CHRISTMAS		= 0x122,
	GUMP_WALL_SAFE				= 0x123,
	GUMP_CHEST_PIRATE			= 0x423,
	GUMP_FOUNTAIN_LIFE			= 0x484,
	GUMP_SECRET_CHEST			= 0x58e,
	GUMP_MAILBOX_DOLPHIN		= 0x6d3,
	GUMP_MAILBOX_SQUIRREL		= 0x6d4,
	GUMP_MAILBOX_BARREL			= 0x6d5,
	GUMP_MAILBOX_LANTERN		= 0x6d6,
	GUMP_CABINET_LIGHT_LARGE	= 0x6e5,
	GUMP_CABINET_DARK_LARGE		= 0x6e6,
	GUMP_DRAWER_LIGHT_LARGE		= 0x6e7,
	GUMP_DRAWER_DARK_LARGE		= 0x6e8,
	GUMP_BARREL_LARGE			= 0x6e9,
	GUMP_BOOK_SHELF_LARGE		= 0x6ea,
	GUMP_SECURE_TRADE			= 0x866,
	GUMP_SECURE_TRADE_TOL		= 0x88a,
	GUMP_BOARD_CHECKER			= 0x91a,
	GUMP_BOARD_BACKGAMMON		= 0x92e,
	GUMP_MAP_2_NORTH			= 0x139d,
	GUMP_CHEST_WEDDING			= 0x266a,
	GUMP_STONE_BASE				= 0x266b,
	GUMP_PLAGUE_BEAST			= 0x2a63,
	GUMP_KING_COLLECTION_BOX	= 0x4d0c,
	GUMP_BACKPACK_SUEDE			= 0x775e,
	GUMP_BACKPACK_POLAR_BEAR	= 0x7760,
	GUMP_BACKPACK_GHOUL_SKIN	= 0x7762,
	GUMP_GIFT_BOX_SQUARE		= 0x777a,
	GUMP_WALL_SAFE2				= 0x9bf2,
	GUMP_CRATE_FLETCHING		= 0x9bfe,
	GUMP_CHEST_WOODEN			= 0x9cd9,
	GUMP_PILLOW_HEART			= 0x9cda,
	GUMP_CHEST_METAL_LARGE		= 0x9cdb,
	GUMP_CHEST_METAL_GOLD_LARGE	= 0x9cdd,
	GUMP_CHEST_WOOD_LARGE		= 0x9cdf,
	GUMP_CHEST_CRATE_LARGE		= 0x9ce3,
	GUMP_MINERS_SATCHEL			= 0x9ce4,
	GUMP_LUMBERJACKS_SATCHEL	= 0x9ce5,
	GUMP_SHIP_CANNON			= 0x9ce7,
	GUMP_CHEST_METAL2			= 0xefe7
};

typedef WORD		TERRAIN_TYPE;
enum
{
	// Terrain samples
	TERRAIN_HOLE	= 0x0002,	// "NODRAW" we can pas thru this.
	TERRAIN_WATER1	= 0x00a8,
	TERRAIN_WATER2	= 0x00a9,
	TERRAIN_WATER3	= 0x00aa,
	TERRAIN_WATER4	= 0x00ab,
	TERRAIN_WATER5	= 0x0136,
	TERRAIN_WATER6	= 0x0137,
	TERRAIN_NULL	= 0x0244,	// impassible interdungeon
	TERRAIN_QTY		= 0x4000
};

/////////////////////////////////////////////////////////////////
// File blocks

#define VERDATA_MAKE_INDEX( f, i ) ((f+1)<< 26 | (i))

enum VERFILE_TYPE		// skew list. (verdata.mul)
{
	VERFILE_ARTIDX		= 0x00,	// "artidx.mul" = Index to ART
	VERFILE_ART			= 0x01, // "art.mul" = Artwork such as ground, objects, etc.
	VERFILE_ANIMIDX		= 0x02,	// "anim.idx" = 2454ah animations.
	VERFILE_ANIM		= 0x03,	// "anim.mul" = Animations such as monsters, people, and armor.
	VERFILE_SOUNDIDX	= 0x04, // "soundidx.mul" = Index into SOUND
	VERFILE_SOUND		= 0x05, // "sound.mul" = Sampled sounds
	VERFILE_TEXIDX		= 0x06, // "texidx.mul" = Index into TEXMAPS
	VERFILE_TEXMAPS		= 0x07,	// "texmaps.mul" = Texture map data (the ground).
	VERFILE_GUMPIDX		= 0x08, // "gumpidx.mul" = Index to GUMPART
	VERFILE_GUMPART		= 0x09, // "gumpart.mul" = Gumps. Stationary controller bitmaps such as windows, buttons, paperdoll pieces, etc.
	VERFILE_MULTIIDX	= 0x0A,	// "multi.idx" = Index to MULTI
	VERFILE_MULTI		= 0x0B,	// "multi.mul" = Groups of art (houses, castles, etc)
	VERFILE_SKILLSIDX	= 0x0C, // "skills.idx" =
	VERFILE_SKILLS		= 0x0D, // "skills.mul" =
	VERFILE_VERDATA		= 0x0E, // ? "verdata.mul" = This version file.
	//	maps
	VERFILE_MAP			= 0x0F, // MAP*.mul(s)
	VERFILE_STAIDX		= 0x13, // STAIDX*.mul(s)
	VERFILE_STATICS		= 0x17,	// STATICS*.mul(s)
	// empty			= 0x10, // "map2.mul"
	// empty			= 0x11, // "map3.mul"
	// empty			= 0x12, // "map4.mul"
	// empty			= 0x14, // "staidx2.mul"
	// empty			= 0x15, // "staidx3.mul"
	// empty			= 0x16, // "staidx4.mul"
	// empty			= 0x18, // "statics2.mul"
	// empty			= 0x19, // "statics3.mul"
	// empty			= 0x1A, // "statics4.mul"
	// empty			= 0x1B space for new map
	// empty			= 0x1C space for new map
	// empty			= 0x1D space for new map

	VERFILE_TILEDATA	= 0x1E, // "tiledata.mul" = Data about tiles in ART. name and flags, etc
	VERFILE_ANIMDATA	= 0x1F, // "animdata.mul" = ? no idea, might be item animation ?.
	VERFILE_HUES		= 0x20, // ? "hues.mul"
	VERFILE_QTY					// NOTE: 021 is used for something ?!
};

enum VERFILE_FORMAT	// mul formats
{
	VERFORMAT_ORIGINAL = 0x01,	// original mul format
	VERFORMAT_HIGHSEAS = 0x02	// high seas mul format
};

struct CUOVersionBlock	// skew list. (verdata.mul)
{
	// First 4 bytes of this file = the qty of records.
private:
	DWORD m_file;		// file type id. VERFILE_TYPE (ex.tiledata = 0x1E)
	DWORD m_block;		// tile number. ( items = itemid + 0x200 )
public:
	DWORD m_filepos;	// pos in this file to find the patch block.
	DWORD m_length;

	WORD m_wVal3;		// stuff that would have been in CUOIndexRec
	WORD m_wVal4;
public:
	DWORD GetIndex() const	// a single sortable index.
	{
		return( VERDATA_MAKE_INDEX( m_file, m_block ));
	}
	VERFILE_TYPE GetFileIndex() const
	{
		return( static_cast<VERFILE_TYPE>(m_file) );
	}
	DWORD GetBlockIndex() const
	{
		return( m_block );
	}
	// This stuff is for GrayPatch
	void SetBlock(DWORD dwBlock)
	{
		m_block = dwBlock;
	}
	void SetFile(VERFILE_TYPE dwFile)
	{
		m_file = dwFile;
	}

} PACK_NEEDED;

struct CUOIndexRec	// 12 byte block = used for table indexes. (staidx0.mul,multi.idx,anim.idx)
{
private:
	DWORD	m_dwOffset;	// 0xFFFFFFFF = nothing here ! else pointer to something (CUOStaticItemRec possibly)
	DWORD 	m_dwLength; // Length of the object in question.
public:
	WORD 	m_wVal3;	// Varied uses. ex. GumpSizey
	WORD 	m_wVal4;	// Varied uses. ex. GumpSizex

public:
	DWORD GetFileOffset() const
	{
		return( m_dwOffset );
	}
	DWORD GetBlockLength() const
	{
		return( m_dwLength &~ 0x80000000 );
	}
	bool IsVerData() const
	{
		return( ( m_dwLength & 0x80000000 ) ? true : false );
	}
	bool HasData() const
	{
		return( m_dwOffset != 0xFFFFFFFF && m_dwLength != 0 );
	}
	void Init()
	{
		m_dwLength = 0;
	}
	void CopyIndex( const CUOVersionBlock * pVerData )
	{
		// Get an index rec from the verdata rec.
		m_dwOffset = pVerData->m_filepos;
		m_dwLength = pVerData->m_length | 0x80000000;
		m_wVal3 = pVerData->m_wVal3;
		m_wVal4 = pVerData->m_wVal4;
	}
	void SetupIndex( DWORD dwOffset, DWORD dwLength )
	{
		m_dwOffset = dwOffset;
		m_dwLength = dwLength;
	}

} PACK_NEEDED;

// Map definition.

#define MAP_QTY				40
#define UO_BLOCK_SIZE		8			// Base width/height size of a block
#define UO_BLOCK_ALIGN(i) 	((i) & ~7)
#define UO_BLOCK_OFFSET(i)	((i) & 7)	// i % UO_BLOCK_SIZE
#define UO_SIZE_Z			127
#define UO_SIZE_MIN_Z		-127

typedef unsigned char height_t;

class CMapDiffCollection;

extern class CMapList
{
public:
	int m_sizex[MAP_QTY];
	int m_sizey[MAP_QTY];
	int m_sectorsize[MAP_QTY];
	bool m_maps[MAP_QTY];			// list of supported maps
	int m_mapnum[MAP_QTY];			// real map number (0 for 0 and 1, 2 for 2, and so on) - file name
	int m_mapid[MAP_QTY];			// map id used by the client
	CMapDiffCollection * m_pMapDiffCollection;

protected:
	bool m_mapsinitalized[MAP_QTY];
	bool DetectMapSize(int map);

public:
	CMapList();

private:
	CMapList(const CMapList& copy);
	CMapList& operator=(const CMapList& other);

public:
	bool Load(int map, char *args);
	bool Load(int map, int maxx, int maxy, int sectorsize, int realmapnum, int mapid);

	void Init();

	bool IsMapSupported(int map) const
	{
		if ( (map < 0) || (map >= MAP_QTY) )
			return false;
		return m_maps[map];
	}

	int GetX(int map) const
	{
		if ( (map < 0) || (map >= MAP_QTY) )
			return 0;
		return m_sizex[map];
	}
	int GetY(int map) const
	{
		if ( (map < 0) || (map >= MAP_QTY) )
			return 0;
		return m_sizey[map];
	}

	int GetSectorSize(int map) const
	{
		if ( (map < 0) || (map >= MAP_QTY) )
			return 0;
		return m_sectorsize[map];
	}
	int GetSectorCols(int map) const
	{
		if ( (map < 0) || (map >= MAP_QTY) || (m_sectorsize[map] <= 0) )
			return 0;
		return m_sizex[map] / m_sectorsize[map];
	}
	int GetSectorRows(int map) const
	{
		if ( (map < 0) || (map >= MAP_QTY) || (m_sectorsize[map] <= 0) )
			return 0;
		return m_sizey[map] / m_sectorsize[map];
	}
	int GetSectorQty(int map) const
	{
		if ( (map < 0) || (map >= MAP_QTY) || (m_sectorsize[map] <= 0) )
			return 0;
		return (m_sizex[map] / m_sectorsize[map]) * (m_sizey[map] / m_sectorsize[map]);
	}

	bool IsInitialized(int map) const
	{
		return m_mapsinitalized[map];
	}
} g_MapList;

#define UO_SIZE_X_REAL		0x1400	// 640*UO_BLOCK_SIZE = 5120 = The actual world is only this big

// This should depend on height of players char.
#define PLAYER_HEIGHT 15	// We need x units of room to walk under something. (human) ??? this should vary based on creature type.

struct CUOMapMeter	// 3 bytes (map0.mul)
{
	WORD m_wTerrainIndex;	// TERRAIN_TYPE index to Radarcol and CUOTerrainTypeRec/CUOTerrainTypeRec2
	signed char m_z;
	static bool IsTerrainNull( WORD wTerrainIndex )
	{
		return (wTerrainIndex == 0x244) ? true : false;
	}
} PACK_NEEDED;

struct CUOMapBlock	// 196 byte block = 8x8 meters, (map0.mul)
{
	WORD m_wID1;	// ?
	WORD m_wID2;
	CUOMapMeter m_Meter[ UO_BLOCK_SIZE * UO_BLOCK_SIZE ];
} PACK_NEEDED;

struct CUOStaticItemRec	// 7 byte block = static items on the map (statics0.mul)
{
	WORD	m_wTileID;		// ITEMID_TYPE = Index to tile CUOItemTypeRec/CUOItemTypeRec2
	BYTE	m_x;		// x <= 7 = offset from block.
	BYTE 	m_y;		// y <= 7
	signed char m_z;	//
	WORD 	m_wHue;		// HUE_TYPE modifier for the item

	// For internal caching purposes only. overload this.
	// LOBYTE(m_wColor) = Blocking flags for this item. (CAN_I_BLOCK)
	// HIBYTE(m_wColor) = Height of this object.

	ITEMID_TYPE GetDispID() const
	{
		return static_cast<ITEMID_TYPE>(m_wTileID);
	}

} PACK_NEEDED;

#define UOTILE_BLOCK_QTY	32	// Come in blocks of 32.

struct CUOTerrainTypeRec	// size = 0x1a = 26 (tiledata.mul)
{	
	// First half of tiledata.mul file is for terrain tiles.
	DWORD m_flags;	// 0xc0=water, 0x40=dirt or rock, 0x60=lava, 0x50=cave, 0=floor
	WORD m_index;	// just counts up.  0 = unused.
	char m_name[20];

} PACK_NEEDED;

struct CUOTerrainTypeRec2	// size = 0x1e = 30 (tiledata.mul, High Seas+)
{
	// First half of tiledata.mul file is for terrain tiles.
	DWORD m_flags;	// 0xc0=water, 0x40=dirt or rock, 0x60=lava, 0x50=cave, 0=floor
	DWORD m_unknown;
	WORD m_index;	// just counts up.  0 = unused.
	char m_name[20];
} PACK_NEEDED;

struct CGrayTerrainInfo : public CUOTerrainTypeRec2
{
	CGrayTerrainInfo( TERRAIN_TYPE id );
};

	// 0x68800 = (( 0x4000 / 32 ) * 4 ) + ( 0x4000 * 26 )
#define UOTILE_TERRAIN_SIZE ((( TERRAIN_QTY / UOTILE_BLOCK_QTY ) * 4 ) + ( TERRAIN_QTY * sizeof( CUOTerrainTypeRec )))

	// 0x78800 = (( 0x4000 / 32 ) * 4 ) + ( 0x4000 * 30 )
#define UOTILE_TERRAIN_SIZE2 ((( TERRAIN_QTY / UOTILE_BLOCK_QTY ) * 4 ) + ( TERRAIN_QTY * sizeof( CUOTerrainTypeRec2 )))

struct CUOItemTypeRec	// size = 37 (tiledata.mul)
{	// Second half of tiledata.mul file is for item tiles (ITEMID_TYPE).
	// if all entries are 0 then this is unused and undisplayable.
#define UFLAG1_FLOOR		0x00000001	// 0= floor (Walkable at base position)
#define UFLAG1_EQUIP		0x00000002	// 1= equipable. m_layer is LAYER_TYPE
#define UFLAG1_NONBLOCKING	0x00000004	// 2= Signs and railings that do not block.
#define UFLAG1_LIQUID		0x00000008	// 3= blood,Wave,Dirt,webs,stain, (translucent items)
#define UFLAG1_WALL			0x00000010	// 4= wall type = wall/door/fireplace
#define UFLAG1_DAMAGE		0x00000020	// 5= damaging. (Sharp, hot or poisonous)
#define UFLAG1_BLOCK		0x00000040	// 6= blocked for normal human. (big and heavy)
#define UFLAG1_WATER		0x00000080	// 7= water/wet (blood/water)
#define UFLAG2_ZERO1		0x00000100	// 8= NOT USED (i checked)
#define UFLAG2_PLATFORM		0x00000200	// 9= platform/flat (can stand on, bed, floor, )
#define UFLAG2_CLIMBABLE	0x00000400	// a= climbable (stairs). m_height /= 2(For Stairs+Ladders)
#define UFLAG2_STACKABLE	0x00000800	// b= pileable/stackable (m_dwUnk7 = stack size ?)
#define UFLAG2_WINDOW		0x00001000	// c= window/arch/door can walk thru it
#define UFLAG2_WALL2		0x00002000	// d= another type of wall. (not sure why there are 2)
#define UFLAG2_A			0x00004000	// e= article a
#define UFLAG2_AN			0x00008000	// f= article an
#define UFLAG3_DESCRIPTION	0x00010000	//10= descriptional tile. (Regions, professions, ...)
#define UFLAG3_TRANSPARENT	0x00020000	//11= Transparent (Is used for leaves and sails)
#define UFLAG3_CLOTH		0x00040000	//12= Probably dyeable ? effects the way the client colors the item. color gray scale stuff else must be converted to grayscale
#define UFLAG3_ZERO8		0x00080000	//13= 0 NOT USED (i checked)
#define UFLAG3_MAP			0x00100000	//14= map
#define UFLAG3_CONTAINER	0x00200000	//15= container.
#define UFLAG3_EQUIP2		0x00400000	//16= equipable (not sure why there are 2 of these)
#define UFLAG3_LIGHT		0x00800000	//17= light source
#define UFLAG4_ANIM			0x01000000	//18= animation with next several object frames.
#define UFLAG4_HOVEROVER	0x02000000	//19= item can be hovered over (SA tiledata) (older tiledata has this applied to archway, easel, fountain - unknown purpose)
#define UFLAG4_WALL3		0x04000000	//1a= tend to be types of walls ? I have no idea.
#define UFLAG4_BODYITEM		0x08000000	//1b= Whole body item (ex.British", "Blackthorne", "GM Robe" and "Death shroud")
#define UFLAG4_ROOF			0x10000000	//1c=
#define UFLAG4_DOOR			0x20000000	//1d= door
#define UFLAG4_STAIRS		0x40000000	//1e=
#define UFLAG4_WALKABLE		0x80000000	//1f= We can walk here.

	DWORD m_flags;
	BYTE m_weight;		// 255 = unmovable.
	BYTE m_layer;		// LAYER_TYPE for UFLAG1_EQUIP, UFLAG3_EQUIP2 or light index for UFLAG3_LIGHT
	DWORD m_dwUnk6;		// ? qty in the case of UFLAG2_STACKABLE, Spell icons use this as well.
	WORD m_wAnim;		// equipable items animation index. (50000 = male offset, 60000=female) Gump base as well
	WORD m_wHue;
	WORD m_wLight;
	BYTE m_height;		// z height but may not be blocking. ex.UFLAG2_WINDOW
	char m_name[20];	// sometimes legit not to have a name
} PACK_NEEDED;

struct CUOItemTypeRec2	// size = 41 (tiledata.mul, High Seas+)
{
	UINT64 m_flags;
	BYTE m_weight;		// 255 = unmovable.
	BYTE m_layer;		// LAYER_TYPE for UFLAG1_EQUIP, UFLAG3_EQUIP2 or light index for UFLAG3_LIGHT
	DWORD m_dwUnk11;	// ? qty in the case of UFLAG2_STACKABLE, Spell icons use this as well.
	WORD m_wAnim;		// equipable items animation index. (50000 = male offset, 60000=female) Gump base as well
	WORD m_wHue;
	WORD m_wLight;
	BYTE m_height;		// z height but may not be blocking. ex.UFLAG2_WINDOW
	char m_name[20];	// sometimes legit not to have a name
} PACK_NEEDED;

struct CGrayItemInfo : public CUOItemTypeRec2
{
	explicit CGrayItemInfo( ITEMID_TYPE id );

	static ITEMID_TYPE GetMaxTileDataItem();
};

struct CUOMultiItemRec // (Multi.mul)
{
	// Describe multi's like houses and boats. One single tile.
	// From Multi.Idx and Multi.mul files.
	WORD m_wTileID;	// ITEMID_TYPE = Index to tile CUOItemTypeRec/CUOItemTypeRec2
	signed short m_dx;	// signed delta.
	signed short m_dy;
	signed short m_dz;
	DWORD m_visible;	// 0 or 1 (non-visible items are things like doors and signs)

	ITEMID_TYPE GetDispID() const
	{
		return static_cast<ITEMID_TYPE>(m_wTileID);
	}

} PACK_NEEDED;

struct CUOMultiItemRec2 // (Multi.mul, High Seas+)
{
	// Describe multi's like houses and boats. One single tile.
	// From Multi.Idx and Multi.mul files
	WORD m_wTileID;	// ITEMID_TYPE = Index to tile CUOItemTypeRec/CUOItemTypeRec2
	signed short m_dx;	// signed delta.
	signed short m_dy;
	signed short m_dz;
	DWORD m_visible;	// 0 or 1 (non-static item, like doors and signs)
	DWORD m_shipAccess;	// 0 or 1 (rope item used to enter/exit galleons)

	ITEMID_TYPE GetDispID() const
	{
		return static_cast<ITEMID_TYPE>(m_wTileID);
	}
} PACK_NEEDED;

struct CUOHuesRec // (Hues.mul)
{
	short m_color[34];
	char m_name[20];

	BYTE GetRGB( int rgb ) const
	{
		short sColor = m_color[31];
		if ( rgb == 0 ) // R
			return static_cast<BYTE>(((sColor & 0x7C00) >> 7));
		else if ( rgb == 1 )
			return static_cast<BYTE>(((sColor & 0x3E0) >> 2));
		else if ( rgb == 3 )
			return static_cast<BYTE>(((sColor & 0x1F) << 3));

		return 0;
	}

} PACK_NEEDED;

#define LIGHT_BRIGHT	0
#define LIGHT_DARK		30

// Turn off structure packing.
#if defined _WIN32 && (!__MINGW32__)
#pragma pack()
#endif

#endif // _INC_GRAYMUL_H
