#include "graysvr.h"	// predef header
#include "../network/network.h"
#include <cmath>

CResource::CResource()
{
	m_timePeriodic.Init();

	m_fUseHTTP = 2;
	m_fUseAuthID = true;
	m_iMapCacheTime = 2 * 60 * TICK_PER_SEC;
	m_iSectorSleepMask = (1 << 10) - 1;
	m_fUseMapDiffs = false;

	m_fSecure = true;
	m_iFreezeRestartTime = 60;

#ifdef _DEBUG
	m_wDebugFlags = 0;
#endif

	// Decay
	m_iDecay_Item = 30 * 60 * TICK_PER_SEC;
	m_iDecay_CorpsePlayer = 7 * 60 * TICK_PER_SEC;
	m_iDecay_CorpseNPC = 7 * 60 * TICK_PER_SEC;

	// Save
	m_iSaveNPCSkills = 10;
	m_iSavePeriod = 20 * 60 * TICK_PER_SEC;
	m_iSaveBackupLevels = 10;
	m_iSaveBackgroundTime = 0;
	m_iSaveSectorsPerTick = 1;
	m_iSaveStepMaxComplexity = 500;
	m_fSaveGarbageCollect = true;

	// Account
	m_iDeadSocketTime = 5 * 60 * TICK_PER_SEC;
	m_iClientsMax = FD_SETSIZE - 1;
	m_iClientsMaxIP = 16;
	m_iConnectingMax = 32;
	m_iConnectingMaxIP = 8;

	m_fAutoAccountCreation = false;
	m_iClientLingerTime = 10 * 60 * TICK_PER_SEC;
	m_iMinCharDeleteTime = 7 * 24 * 60 * 60 * TICK_PER_SEC;
	m_iMaxCharsPerAccount = 5;
	m_fMd5Passwords = false;

	// Magic
	m_fReagentsRequired = false;
	m_iWordsOfPowerColor = HUE_TEXT_DEF;
	m_iWordsOfPowerFont = FONT_NORMAL;
	m_fWordsOfPowerPlayer = true;
	m_fWordsOfPowerStaff = false;
	m_fEquippedCast = true;
	m_fReagentLossFail = false;
	m_iMagicUnlockDoor = 900;
	m_iSpellTimeout = 0;

	m_iSpell_Teleport_Effect_NPC = ITEMID_FX_HEAL_EFFECT;
	m_iSpell_Teleport_Sound_NPC = SOUND_TELEPORT;
	m_iSpell_Teleport_Effect_Players = ITEMID_FX_TELE_VANISH;
	m_iSpell_Teleport_Sound_Players = SOUND_TELEPORT;
	m_iSpell_Teleport_Effect_Staff = ITEMID_FX_FLAMESTRIKE;
	m_iSpell_Teleport_Sound_Staff = 0x1F3;

	// Ingame effects
	m_iLightDay = LIGHT_BRIGHT;
	m_iLightNight = 25;
	m_iLightDungeon = 27;
	m_iGameMinuteLength = 20 * TICK_PER_SEC;
	m_fNoWeather = true;
	m_fCharTags = false;
	m_fVendorTradeTitle = true;
	m_fFlipDroppedItems = true;
	m_iItemsMaxAmount = 60000;
	m_fMonsterFight = false;
	m_fMonsterFear = false;
	m_iBankIMax = 1000;
	m_iBankWMax = 1000 * WEIGHT_UNITS;
	m_iBackpackMaxWeight = 550 * WEIGHT_UNITS;
	m_iVendorMaxSell = 255;
	m_iMaxCharComplexity = 32;
	m_iMaxItemComplexity = 25;
	m_iMaxSectorComplexity = 1024;
	m_fGenericSounds = true;
	m_fAutoNewbieKeys = true;
	m_iStamRunningPenalty = 50;
	m_iStaminaLossAtWeight = 150;
	m_iHitpointPercentOnRez = 10;
	m_iHitsHungerLoss = 0;
	m_iMaxBaseSkill = 200;
	m_iTrainSkillCost = 1;
	m_iTrainSkillMax = 420;
	m_iTrainSkillPercent = 30;
	m_fDeadCannotSeeLiving = 0;
	m_iMediumCanHearGhosts = 1000;
	m_iMountHeight = false;
	m_iMoveRate = 100;
	m_iArcheryMinDist = 2;
	m_iArcheryMaxDist = 15;
	m_iHitsUpdateRate = TICK_PER_SEC;
	m_iSpeedScaleFactor = 15000;
	m_iCombatArcheryMovementDelay = 10;
	m_iCombatDamageEra = 0;
	m_iCombatHitChanceEra = 0;
	m_iCombatSpeedEra = 0;
	m_iSkillPracticeMax = 300;
	m_iPacketDeathAnimation = true;

	// Flags for controlling pvp/pvm behaviour of players
	m_iCombatFlags = 0;
	m_iMagicFlags = 0;
	m_iRacialFlags = 0;
	m_iRevealFlags = (REVEALF_DETECTINGHIDDEN|REVEALF_LOOTINGSELF|REVEALF_LOOTINGOTHERS|REVEALF_SPEAK|REVEALF_SPELLCAST);

	// Criminal/Karma
	m_fAttackingIsACrime = true;
	m_fGuardsInstantKill = true;
	m_fGuardsOnMurderers = true;
	m_iGuardLingerTime = 3 * 60 * TICK_PER_SEC;
	m_iSnoopCriminal = 100;
	m_iTradeWindowSnooping = true;
	m_iMurderMinCount = 5;
	m_iMurderDecayTime = 8 * 60 * 60 * TICK_PER_SEC;
	m_fHelpingCriminalsIsACrime = true;
	m_fLootingIsACrime = true;
	m_iCriminalTimer = 3 * 60 * TICK_PER_SEC;
	m_iPlayerKarmaNeutral = -2000;
	m_iPlayerKarmaEvil = -8000;
	m_iMinKarma = -10000;
	m_iMaxKarma = 10000;
	m_iMaxFame = 10000;

	// Other
	m_fNoResRobe = 0;
	m_iLostNPCTeleport = 50;
	m_iExperimental = 0;
	m_iOptionFlags = (OF_NoDClickTarget|OF_Command_Sysmsgs|OF_OSIMultiSight|OF_NoHouseMuteSpeech|OF_Buffs|OF_NoDClickTurn);
	m_iWoolGrowthTime = 30 * 60 * TICK_PER_SEC;
	m_iAttackerTimeout = 300 * TICK_PER_SEC;
	m_iNotoTimeout = 30 * TICK_PER_SEC;
	m_iMaxSkill = SKILL_QTY;

	m_iDistanceWhisper = 1;
	m_iDistanceYell = UO_MAP_VIEW_RADAR;

	// Third-party tools
	m_fCUOStatus = true;
	m_fUOGStatus = true;

	m_iWalkBuffer = 75;
	m_iWalkRegen = 25;

	m_iCommandLog = 0;

	m_fUsecrypt = true;
	m_fUsenocrypt = false;

	m_fPayFromPackOnly = false;
	m_iOverSkillMultiply = 2;
	m_fSuppressCapitals = false;

	m_iAdvancedLos = 0;

	m_iFeatureT2A = (FEATURE_T2A_UPDATE|FEATURE_T2A_CHAT);
	m_iFeatureLBR = 0;
	m_iFeatureAOS = 0;
	m_iFeatureSE = 0;
	m_iFeatureML = 0;
	m_iFeatureKR = 0;
	m_iFeatureSA = 0;
	m_iFeatureTOL = 0;
	m_iFeatureExtra = 0;

	m_iMaxLoopTimes = 100000;

	m_iStatFlag = 0;

	m_iNpcAi = 0;

	// Experience system
	m_bExperienceSystem = false;
	m_iExperienceMode = 0;
	m_iExperienceKoefPVP = 100;
	m_iExperienceKoefPVM = 100;
	m_bLevelSystem = false;
	m_iLevelMode = LEVEL_MODE_DOUBLE;
	m_iLevelNextAt = 0;

	m_bAutoResDisp = true;

	m_cCommandPrefix = '.';

	m_iDefaultCommandLevel = PLEVEL_Owner;

	// Notoriety colors
	m_iColorNotoGood = 0x59;				// blue
	m_iColorNotoGuildSame = 0x3f;			// green
	m_iColorNotoNeutral = 0x3b2;			// grey (can be attacked)
	m_iColorNotoCriminal = 0x3b2;			// grey (criminal)
	m_iColorNotoGuildWar = 0x90;			// orange (enemy guild)
	m_iColorNotoEvil = 0x22;				// red
	m_iColorNotoInvul = 0x35;				// yellow
	m_iColorNotoInvulGameMaster = 0x0b;		// purple
	m_iColorNotoDefault = 0x3b2;			// grey (if not any other)

	m_iColorInvis = 0;
	m_iColorInvisSpell = 0;
	m_iColorHidden = 0;

	// Notoriety inheritance
	m_iPetsInheritNotoriety = 0;

	m_iClientLoginMaxTries = 10;
	m_iClientLoginTempBan = 3 * 60 * TICK_PER_SEC;
	m_iMaxShipPlankTeleport = UO_MAP_VIEW_SIZE;

	// Chat system
	m_sChatStaticChannels = "General, Help, Trade, Looking For Group";
	m_iChatFlags = (CHATF_AUTOJOIN|CHATF_CHANNELCREATION|CHATF_CHANNELMODERATION|CHATF_CUSTOMNAMES);

	// MySQL support
	m_bMySql = false;

	// Network settings
	m_iNetworkThreads = 0;
	m_iNetworkThreadPriority = IThread::Disabled;
	m_fUseAsyncNetwork = 0;
	m_iNetMaxPings = 15;
	m_iNetHistoryTTL = 300;
	m_iNetMaxPacketsPerTick = 50;
	m_iNetMaxLengthPerTick = 18000;
	m_iNetMaxQueueSize = 75;
	m_fUsePacketPriorities = false;
	m_fUseExtraBuffer = true;

	m_iTooltipCache = 30 * TICK_PER_SEC;
	m_iTooltipMode = TOOLTIPMODE_SENDVERSION;

	m_iRegenRate[STAT_STR] = 40 * TICK_PER_SEC;		// Seconds to heal ONE hp (before stam/food adjust)
	m_iRegenRate[STAT_INT] = 20 * TICK_PER_SEC;		// Seconds to heal ONE mn
	m_iRegenRate[STAT_DEX] = 10 * TICK_PER_SEC;		// Seconds to heal ONE stm
	m_iRegenRate[STAT_FOOD] = 60 * 60 * TICK_PER_SEC;	// Food usage (1 time per 60 minutes)
	m_iTimerCall = 0;
	m_bAllowLightOverride = true;
	m_sZeroPoint = "1323,1624,0";
	m_bAllowBuySellAgent = false;

	m_NPCNoFameTitle = true;

	m_iMaxPolyStats = 150;
}

CResource::~CResource()
{
	for ( size_t i = 0; i < COUNTOF(m_ResHash.m_Array); ++i )
	{
		for ( size_t j = 0; j < m_ResHash.m_Array[i].GetCount(); ++j )
		{
			CResourceDef *pResourceDef = m_ResHash.m_Array[i][j];
			if ( pResourceDef )
				pResourceDef->UnLink();
		}
	}
	Unload(false);
}

// SKILL ITEMDEF, etc
bool CResource::r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef)
{
	ADDTOCALLSTACK("CResource::r_GetRef");
	TCHAR *pszSep = const_cast<TCHAR *>(strchr(pszKey, '('));	// acts like const_cast
	if ( !pszSep )
	{
		pszSep = const_cast<TCHAR *>(strchr(pszKey, '.'));
		if ( !pszSep )
			return false;
	}

	char oldChar = *pszSep;
	*pszSep = '\0';

	bool fNewStyleDef = false;
	int iResType = FindTableSorted(pszKey, sm_szResourceBlocks, RES_QTY);
	if ( iResType < 0 )
	{
		if ( strcmpi(pszKey, "MULTIDEF") == 0 )
		{
			iResType = RES_ITEMDEF;
			fNewStyleDef = true;
		}
		else
		{
			*pszSep = oldChar;
			return false;
		}
	}

	*pszSep = '.';

	// Now get the index
	pszKey = pszSep + 1;
	if ( pszKey[0] == '\0' )
	{
		*pszSep = oldChar;
		return false;
	}

	pszSep = const_cast<TCHAR *>(strchr(pszKey, '.'));
	if ( pszSep )
		*pszSep = '\0';

	if ( iResType == RES_SERVERS )
		pRef = NULL;
	else if ( iResType == RES_CHARDEF )
	{
		//pRef = CCharBase::FindCharBase(static_cast<CREID_TYPE>(Exp_GetVal(pszKey)));
		pRef = CCharBase::FindCharBase(static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType(RES_CHARDEF, pszKey)));
	}
	else if ( iResType == RES_ITEMDEF )
	{
		if ( fNewStyleDef && IsDigit(pszKey[0]) )
			pRef = CItemBase::FindItemBase(static_cast<ITEMID_TYPE>(Exp_GetVal(pszKey) + ITEMID_MULTI));
		else
			pRef = CItemBase::FindItemBase(static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType(RES_ITEMDEF, pszKey)));
	}
	else if ( (iResType == RES_SPELL) && (*pszKey == '-') )
	{
		++pszKey;
		int i = Exp_GetVal(pszKey);
		pRef = ((i >= 0) && m_SpellDefs_Sorted.IsValidIndex(i)) ? m_SpellDefs_Sorted[i] : NULL;
	}
	else
	{
		// Check if the found resource type matches what we searched for
		RESOURCE_ID	rid = ResourceGetID(static_cast<RES_TYPE>(iResType), pszKey);
		if ( rid.GetResType() == iResType )
			pRef = ResourceGetDef(rid);
	}

	if ( pszSep )
	{
		*pszSep = oldChar;	//*pszSep = '.';
		pszKey = pszSep + 1;
	}
	else
		pszKey += strlen(pszKey);

	return true;
}

enum RC_TYPE
{
	RC_ACCTFILES,					// m_sAcctBaseDir
	RC_ADVANCEDLOS,					// m_iAdvancedLos
	RC_ALLOWBUYSELLAGENT,			// m_bAllowBuySellAgent
	RC_ALLOWLIGHTOVERRIDE,			// m_bAllowLightOverride
	RC_ARCHERYMAXDIST,				// m_iArcheryMaxDist
	RC_ARCHERYMINDIST,				// m_iArcheryMinDist
	RC_ATTACKERTIMEOUT,				// m_iAttackerTimeout
	RC_ATTACKINGISACRIME,			// m_fAttackingIsACrime
	RC_AUTOACCOUNTCREATION,			// m_fAutoAccountCreation
	RC_AUTONEWBIEKEYS,				// m_fAutoNewbieKeys
	RC_AUTORESDISP,					// m_bAutoResDisp
	RC_BACKPACKMAXWEIGHT,			// m_iBackpackMaxWeight
	RC_BACKUPLEVELS,				// m_iSaveBackupLevels
	RC_BANKMAXITEMS,				// m_iBankIMax
	RC_BANKMAXWEIGHT,				// m_iBankWMax
	RC_BUILD,
	RC_CHARTAGS,					// m_fCharTags
	RC_CHATFLAGS,					// m_iChatFlags
	RC_CHATSTATICCHANNELS,			// m_sChatStaticChannels
	RC_CLIENTLINGER,				// m_iClientLingerTime
	RC_CLIENTLOGINMAXTRIES,			// m_iClientLoginMaxTries
	RC_CLIENTLOGINTEMPBAN,			// m_iClientLoginTempBan
	RC_CLIENTMAX,					// m_iClientsMax
	RC_CLIENTMAXIP,					// m_iClientsMaxIP
	RC_COLORHIDDEN,					// m_iColorHidden
	RC_COLORINVIS,					// m_iColorInvis
	RC_COLORINVISSPELL,				// m_iColorInvisSpell
	RC_COLORNOTOCRIMINAL,			// m_iColorNotoCriminal
	RC_COLORNOTODEFAULT,			// m_iColorNotoDefault
	RC_COLORNOTOEVIL,				// m_iColorNotoEvil
	RC_COLORNOTOGOOD,				// m_iColorNotoGood
	RC_COLORNOTOGUILDSAME,			// m_iColorNotoGuildSame
	RC_COLORNOTOGUILDWAR,			// m_iColorNotoGuildWar
	RC_COLORNOTOINVUL,				// m_iColorNotoInvul
	RC_COLORNOTOINVULGAMEMASTER,	// m_iColorNotoInvulGameMaster
	RC_COLORNOTONEUTRAL,			// m_iColorNotoNeutral
	RC_COMBATARCHERYMOVEMENTDELAY,	// m_iCombatArcheryMovementDelay
	RC_COMBATDAMAGEERA,				// m_iCombatDamageEra
	RC_COMBATFLAGS,					// m_iCombatFlags
	RC_COMBATHITCHANCEERA,			// m_iCombatHitChanceEra
	RC_COMBATSPEEDERA,				// m_iCombatSpeedEra
	RC_COMMANDLOG,					// m_iCommandLog
	RC_COMMANDPREFIX,				// m_cCommandPrefix
	RC_COMMANDTRIGGER,				// m_sCommandTrigger
	RC_CONNECTINGMAX,				// m_iConnectingMax
	RC_CONNECTINGMAXIP,				// m_iConnectingMaxIP
	RC_CORPSENPCDECAY,				// m_iDecay_CorpseNPC
	RC_CORPSEPLAYERDECAY,			// m_iDecay_CorpsePlayer
	RC_CRIMINALTIMER,				// m_iCriminalTimer
	RC_CUOSTATUS,					// m_fCUOStatus
	RC_DEADCANNOTSEELIVING,			// m_fDeadCannotSeeLiving
	RC_DEADSOCKETTIME,				// m_iDeadSocketTime
#ifdef _DEBUG
	RC_DEBUGFLAGS,					// m_wDebugFlags
#endif
	RC_DECAYTIMER,					// m_iDecay_Item
	RC_DEFAULTCOMMANDLEVEL,			// m_iDefaultCommandLevel
	RC_DISTANCEWHISPER,				// m_iDistanceWhisper
	RC_DISTANCEYELL,				// m_iDistanceYell
	RC_DUNGEONLIGHT,				// m_iLightDungeon
	RC_EQUIPPEDCAST,				// m_fEquippedCast
	RC_EVENTSITEM,					// m_sEventsItem
	RC_EVENTSPET,					// m_sEventsPet
	RC_EVENTSPLAYER,				// m_sEventsPlayer
	RC_EVENTSREGION,				// m_sEventsRegion
	RC_EXPERIENCEKOEFPVM,			// m_iExperienceKoefPVM
	RC_EXPERIENCEKOEFPVP,			// m_iExperienceKoefPVP
	RC_EXPERIENCEMODE,				// m_iExperienceMode
	RC_EXPERIENCESYSTEM,			// m_bExperienceSystem
	RC_EXPERIMENTAL,				// m_iExperimental
	RC_FEATURESAOS,					// m_iFeatureAOS
	RC_FEATURESEXTRA,				// m_iFeatureExtra
	RC_FEATURESKR,					// m_iFeatureKR
	RC_FEATURESLBR,					// m_iFeatureLBR
	RC_FEATURESML,					// m_iFeatureML
	RC_FEATURESSA,					// m_iFeatureSA
	RC_FEATURESSE,					// m_iFeatureSE
	RC_FEATUREST2A,					// m_iFeatureT2A
	RC_FEATURESTOL,					// m_iFeatureTOL
	RC_FLIPDROPPEDITEMS,			// m_fFlipDroppedItems
	RC_FORCEGARBAGECOLLECT,			// m_fSaveGarbageCollect
	RC_FREEZERESTARTTIME,			// m_iFreezeRestartTime
	RC_GAMEMINUTELENGTH,			// m_iGameMinuteLength
	RC_GENERICSOUNDS,				// m_fGenericSounds
	RC_GUARDLINGER,					// m_iGuardLingerTime
	RC_GUARDSINSTANTKILL,			// m_fGuardsInstantKill
	RC_GUARDSONMURDERERS,			// m_fGuardsOnMurderers
	RC_GUILDS,
	RC_HEARALL,
	RC_HELPINGCRIMINALSISACRIME,	// m_fHelpingCriminalsIsACrime
	RC_HITPOINTPERCENTONREZ,		// m_iHitpointPercentOnRez
	RC_HITSHUNGERLOSS,				// m_iHitsHungerLoss
	RC_HITSUPDATERATE,				// m_iHitsUpdateRate
	RC_ITEMSMAXAMOUNT,				// m_iItemsMaxAmount
	RC_LEVELMODE,					// m_iLevelMode
	RC_LEVELNEXTAT,					// m_iLevelNextAt
	RC_LEVELSYSTEM,					// m_bLevelSystem
	RC_LIGHTDAY,					// m_iLightDay
	RC_LIGHTNIGHT,					// m_iLightNight
	RC_LOG,
	RC_LOGMASK,
	RC_LOOTINGISACRIME,				// m_fLootingIsACrime
	RC_LOSTNPCTELEPORT,				// m_fLostNPCTeleport
	RC_MAGICFLAGS,					// m_iMagicFlags
	RC_MAGICUNLOCKDOOR,				// m_iMagicUnlockDoor
	RC_MAPCACHETIME,				// m_iMapCacheTime
	RC_MAXBASESKILL,				// m_iMaxBaseSkill
	RC_MAXCHARSPERACCOUNT,			// m_iMaxCharsPerAccount
	RC_MAXCOMPLEXITY,				// m_iMaxCharComplexity
	RC_MAXFAME,						// m_iMaxFame
	RC_MAXITEMCOMPLEXITY,			// m_iMaxItemComplexity
	RC_MAXKARMA,					// m_iMaxKarma
	RC_MAXLOOPTIMES,				// m_iMaxLoopTimes
	RC_MAXPACKETSPERTICK,			// m_iNetMaxPacketsPerTick
	RC_MAXPINGS,					// m_iNetMaxPings
	RC_MAXPOLYSTATS,				// m_iMaxPolyStats
	RC_MAXQUEUESIZE,				// m_iNetMaxQueueSize
	RC_MAXSECTORCOMPLEXITY,			// m_iMaxSectorComplexity
	RC_MAXSHIPPLANKTELEPORT,		// m_iMaxShipPlankTeleport
	RC_MAXSIZEPERTICK,				// m_iNetMaxLengthPerTick
	RC_MD5PASSWORDS,				// m_fMd5Passwords
	RC_MEDIUMCANHEARGHOSTS,			// m_iMediumCanHearGhosts
	RC_MINCHARDELETETIME,			// m_iMinCharDeleteTime
	RC_MINKARMA,					// m_iMinKarma
	RC_MONSTERFEAR,					// m_fMonsterFear
	RC_MONSTERFIGHT,				// m_fMonsterFight
	RC_MOUNTHEIGHT,					// m_iMountHeight
	RC_MOVERATE,					// m_iMoveRate
	RC_MULFILES,
	RC_MURDERDECAYTIME,				// m_iMurderDecayTime;
	RC_MURDERMINCOUNT,				// m_iMurderMinCount
	RC_MYSQL,						// m_bMySql
	RC_MYSQLDB,						// m_sMySqlDatabase
	RC_MYSQLHOST,					// m_sMySqlHost
	RC_MYSQLPASS,					// m_sMySqlPassword
	RC_MYSQLUSER,					// m_sMySqlUser
	RC_NETTTL,						// m_iNetHistoryTTL
	RC_NETWORKTHREADPRIORITY,		// m_iNetworkThreadPriority
	RC_NETWORKTHREADS,				// m_iNetworkThreads
	RC_NORESROBE,					// m_fNoResRobe
	RC_NOTOTIMEOUT,					// m_iNotoTimeout
	RC_NOWEATHER,					// m_fNoWeather
	RC_NPCAI,						// m_iNpcAi
	RC_NPCNOFAMETITLE,				// m_NPCNoFameTitle
	RC_NPCSKILLSAVE,				// m_iSaveNPCSkills
	RC_NPCTRAINCOST,				// m_iTrainSkillCost
	RC_NPCTRAINMAX,					// m_iTrainSkillMax
	RC_NPCTRAINPERCENT,				// m_iTrainSkillPercent
	RC_OPTIONFLAGS,					// m_iOptionFlags
	RC_OVERSKILLMULTIPLY,			// m_iOverSkillMultiply
	RC_PACKETDEATHANIMATION,		// m_iPacketDeathAnimation
	RC_PAYFROMPACKONLY,				// m_fPayFromPackOnly
	RC_PETSINHERITNOTORIETY,		// m_iPetsInheritNotoriety
	RC_PLAYEREVIL,					// m_iPlayerKarmaEvil
	RC_PLAYERNEUTRAL,				// m_iPlayerKarmaNeutral
	RC_PROFILE,
	RC_RACIALFLAGS,					// m_iRacialFlags
	RC_REAGENTLOSSFAIL,				// m_fReagentLossFail
	RC_REAGENTSREQUIRED,			// m_fReagentsRequired
	RC_REVEALFLAGS,					// m_iRevealFlags
	RC_RTICKS,
	RC_RTIME,
	RC_RUNNINGPENALTY,				// m_iStamRunningPenalty
	RC_SAVEBACKGROUND,				// m_iSaveBackgroundTime
	RC_SAVEPERIOD,					// m_iSavePeriod
	RC_SAVESECTORSPERTICK,			// m_iSaveSectorsPerTick
	RC_SAVESTEPMAXCOMPLEXITY,		// m_iSaveStepMaxComplexity
	RC_SCPFILES,
	RC_SECTORSLEEP,					// m_iSectorSleepMask
	RC_SECURE,						// m_fSecure
	RC_SKILLPRACTICEMAX,			// m_iSkillPracticeMax
	RC_SNOOPCRIMINAL,				// m_iSnoopCriminal
	RC_SPEECHOTHER,					// m_sSpeechOther
	RC_SPEECHPET,					// m_sSpeechPet
	RC_SPEECHSELF,					// m_sSpeechSelf
	RC_SPEEDSCALEFACTOR,			// m_iSpeedScaleFactor
	RC_SPELLTIMEOUT,				// m_iSpellTimeout
	RC_STAMINALOSSATWEIGHT,			// m_iStaminaLossAtWeight
	RC_STATSFLAGS,					// m_iStatFlag
	RC_STRIPPATH,					// m_sStripPath
	RC_SUPPRESSCAPITALS,			// m_fSuppressCapitals
	RC_TELEPORTEFFECTNPC,			// m_iSpell_Teleport_Effect_NPC
	RC_TELEPORTEFFECTPLAYERS,		// m_iSpell_Teleport_Effect_Players
	RC_TELEPORTEFFECTSTAFF,			// m_iSpell_Teleport_Effect_Staff
	RC_TELEPORTSOUNDNPC,			// m_iSpell_Teleport_Sound_NPC
	RC_TELEPORTSOUNDPLAYERS,		// m_iSpell_Teleport_Sound_Players
	RC_TELEPORTSOUNDSTAFF,			// m_iSpell_Teleport_Sound_Staff
	RC_TIMERCALL,					// m_iTimerCall
	RC_TIMEUP,
	RC_TOOLTIPCACHE,				// m_iTooltipCache
	RC_TOOLTIPMODE,					// m_iTooltipMode
	RC_TRADEWINDOWSNOOPING,			// m_iTradeWindowSnooping
	RC_UOGSTATUS,					// m_fUOGStatus
	RC_USEASYNCNETWORK,				// m_fUseAsyncNetwork
	RC_USEAUTHID,					// m_fUseAuthID
	RC_USECRYPT,					// m_Usecrypt
	RC_USEEXTRABUFFER,				// m_fUseExtraBuffer
	RC_USEHTTP,						// m_fUseHTTP
	RC_USEMAPDIFFS,					// m_fUseMapDiffs
	RC_USENOCRYPT,					// m_Usenocrypt
	RC_USEPACKETPRIORITY,			// m_fUsePacketPriorities
	RC_VENDORMAXSELL,				// m_iVendorMaxSell
	RC_VENDORTRADETITLE,			// m_fVendorTradeTitle
	RC_VERSION,
	RC_WALKBUFFER,					// m_iWalkBuffer
	RC_WALKREGEN,					// m_iWalkRegen
	RC_WOOLGROWTHTIME,				// m_iWoolGrowthTime
	RC_WOPCOLOR,					// m_iWordsOfPowerColor
	RC_WOPFONT,						// m_iWordsOfPowerFont
	RC_WOPPLAYER,					// m_fWordsOfPowerPlayer
	RC_WOPSTAFF,					// m_fWordsOfPowerStaff
	RC_WORLDSAVE,
	RC_ZEROPOINT,					// m_sZeroPoint
	RC_QTY
};

const CAssocReg CResource::sm_szLoadKeys[RC_QTY + 1] =
{
	{"ACCTFILES",					{ELEM_CSTRING,	OFFSETOF(CResource, m_sAcctBaseDir),					0}},
	{"ADVANCEDLOS",					{ELEM_INT,		OFFSETOF(CResource, m_iAdvancedLos),					0}},
	{"ALLOWBUYSELLAGENT",			{ELEM_BOOL,		OFFSETOF(CResource, m_bAllowBuySellAgent),				0}},
	{"ALLOWLIGHTOVERRIDE",			{ELEM_BOOL,		OFFSETOF(CResource, m_bAllowLightOverride),				0}},
	{"ARCHERYMAXDIST",				{ELEM_INT,		OFFSETOF(CResource, m_iArcheryMaxDist),					0}},
	{"ARCHERYMINDIST",				{ELEM_INT,		OFFSETOF(CResource, m_iArcheryMinDist),					0}},
	{"ATTACKERTIMEOUT",				{ELEM_INT,		OFFSETOF(CResource, m_iAttackerTimeout),				0}},
	{"ATTACKINGISACRIME",			{ELEM_BOOL,		OFFSETOF(CResource, m_fAttackingIsACrime),				0}},
	{"AUTOACCOUNTCREATION",			{ELEM_BOOL,		OFFSETOF(CResource, m_fAutoAccountCreation),			0}},
	{"AUTONEWBIEKEYS",				{ELEM_BOOL,		OFFSETOF(CResource, m_fAutoNewbieKeys),					0}},
	{"AUTORESDISP",					{ELEM_BOOL,		OFFSETOF(CResource, m_bAutoResDisp),					0}},
	{"BACKPACKMAXWEIGHT",			{ELEM_INT,		OFFSETOF(CResource, m_iBackpackMaxWeight),				0}},
	{"BACKUPLEVELS",				{ELEM_INT,		OFFSETOF(CResource, m_iSaveBackupLevels),				0}},
	{"BANKMAXITEMS",				{ELEM_DWORD,	OFFSETOF(CResource, m_iBankIMax),						0}},
	{"BANKMAXWEIGHT",				{ELEM_INT,		OFFSETOF(CResource, m_iBankWMax),						0}},
	{"BUILD",						{ELEM_VOID,		0,														0}},
	{"CHARTAGS",					{ELEM_BOOL,		OFFSETOF(CResource, m_fCharTags),						0}},
	{"CHATFLAGS",					{ELEM_INT,		OFFSETOF(CResource, m_iChatFlags),						0}},
	{"CHATSTATICCHANNELS",			{ELEM_CSTRING,	OFFSETOF(CResource, m_sChatStaticChannels),				0}},
	{"CLIENTLINGER",				{ELEM_INT,		OFFSETOF(CResource, m_iClientLingerTime),				0}},
	{"CLIENTLOGINMAXTRIES",			{ELEM_INT,		OFFSETOF(CResource, m_iClientLoginMaxTries),			0}},
	{"CLIENTLOGINTEMPBAN",			{ELEM_INT,		OFFSETOF(CResource, m_iClientLoginTempBan),				0}},
	{"CLIENTMAX",					{ELEM_INT,		OFFSETOF(CResource, m_iClientsMax),						0}},
	{"CLIENTMAXIP",					{ELEM_INT,		OFFSETOF(CResource, m_iClientsMaxIP),					0}},
	{"COLORHIDDEN",					{ELEM_VOID,		OFFSETOF(CResource, m_iColorHidden),					0}},
	{"COLORINVIS",					{ELEM_VOID,		OFFSETOF(CResource, m_iColorInvis),						0}},
	{"COLORINVISSPELL",				{ELEM_VOID,		OFFSETOF(CResource, m_iColorInvisSpell),				0}},
	{"COLORNOTOCRIMINAL",			{ELEM_WORD,		OFFSETOF(CResource, m_iColorNotoCriminal),				0}},
	{"COLORNOTODEFAULT",			{ELEM_WORD,		OFFSETOF(CResource, m_iColorNotoDefault),				0}},
	{"COLORNOTOEVIL",				{ELEM_WORD,		OFFSETOF(CResource, m_iColorNotoEvil),					0}},
	{"COLORNOTOGOOD",				{ELEM_WORD,		OFFSETOF(CResource, m_iColorNotoGood),					0}},
	{"COLORNOTOGUILDSAME",			{ELEM_WORD,		OFFSETOF(CResource, m_iColorNotoGuildSame),				0}},
	{"COLORNOTOGUILDWAR",			{ELEM_WORD,		OFFSETOF(CResource, m_iColorNotoGuildWar),				0}},
	{"COLORNOTOINVUL",				{ELEM_WORD,		OFFSETOF(CResource, m_iColorNotoInvul),					0}},
	{"COLORNOTOINVULGAMEMASTER",	{ELEM_WORD,		OFFSETOF(CResource, m_iColorNotoInvulGameMaster),		0}},
	{"COLORNOTONEUTRAL",			{ELEM_WORD,		OFFSETOF(CResource, m_iColorNotoNeutral),				0}},
	{"COMBATARCHERYMOVEMENTDELAY",	{ELEM_BYTE,		OFFSETOF(CResource, m_iCombatArcheryMovementDelay),		0}},
	{"COMBATDAMAGEERA",				{ELEM_BYTE,		OFFSETOF(CResource, m_iCombatDamageEra),				0}},
	{"COMBATFLAGS",					{ELEM_INT,		OFFSETOF(CResource, m_iCombatFlags),					0}},
	{"COMBATHITCHANCEERA",			{ELEM_BYTE,		OFFSETOF(CResource, m_iCombatHitChanceEra),				0}},
	{"COMBATSPEEDERA",				{ELEM_BYTE,		OFFSETOF(CResource, m_iCombatSpeedEra),					0}},
	{"COMMANDLOG",					{ELEM_INT,		OFFSETOF(CResource, m_iCommandLog),						0}},
	{"COMMANDPREFIX",				{ELEM_BYTE,		OFFSETOF(CResource, m_cCommandPrefix),					0}},
	{"COMMANDTRIGGER",				{ELEM_CSTRING,	OFFSETOF(CResource, m_sCommandTrigger),					0}},
	{"CONNECTINGMAX",				{ELEM_INT,		OFFSETOF(CResource, m_iConnectingMax),					0}},
	{"CONNECTINGMAXIP",				{ELEM_INT,		OFFSETOF(CResource, m_iConnectingMaxIP),				0}},
	{"CORPSENPCDECAY",				{ELEM_INT,		OFFSETOF(CResource, m_iDecay_CorpseNPC),				0}},
	{"CORPSEPLAYERDECAY",			{ELEM_INT,		OFFSETOF(CResource, m_iDecay_CorpsePlayer),				0}},
	{"CRIMINALTIMER",				{ELEM_INT,		OFFSETOF(CResource, m_iCriminalTimer),					0}},
	{"CUOSTATUS",					{ELEM_BOOL,		OFFSETOF(CResource, m_fCUOStatus),						0}},
	{"DEADCANNOTSEELIVING",			{ELEM_INT,		OFFSETOF(CResource, m_fDeadCannotSeeLiving),			0}},
	{"DEADSOCKETTIME",				{ELEM_INT,		OFFSETOF(CResource, m_iDeadSocketTime),					0}},
#ifdef _DEBUG
	{"DEBUGFLAGS",					{ELEM_WORD,		OFFSETOF(CResource, m_wDebugFlags),						0}},
#endif
	{"DECAYTIMER",					{ELEM_INT,		OFFSETOF(CResource, m_iDecay_Item),						0}},
	{"DEFAULTCOMMANDLEVEL",			{ELEM_INT,		OFFSETOF(CResource, m_iDefaultCommandLevel),			0}},
	{"DISTANCEWHISPER",				{ELEM_INT,		OFFSETOF(CResource, m_iDistanceWhisper),				0}},
	{"DISTANCEYELL",				{ELEM_INT,		OFFSETOF(CResource, m_iDistanceYell),					0}},
	{"DUNGEONLIGHT",				{ELEM_INT,		OFFSETOF(CResource, m_iLightDungeon),					0}},
	{"EQUIPPEDCAST",				{ELEM_BOOL,		OFFSETOF(CResource, m_fEquippedCast),					0}},
	{"EVENTSITEM",					{ELEM_CSTRING,	OFFSETOF(CResource, m_sEventsItem),						0}},
	{"EVENTSPET",					{ELEM_CSTRING,	OFFSETOF(CResource, m_sEventsPet),						0}},
	{"EVENTSPLAYER",				{ELEM_CSTRING,	OFFSETOF(CResource, m_sEventsPlayer),					0}},
	{"EVENTSREGION",				{ELEM_CSTRING,	OFFSETOF(CResource, m_sEventsRegion),					0}},
	{"EXPERIENCEKOEFPVM",			{ELEM_INT,		OFFSETOF(CResource, m_iExperienceKoefPVM),				0}},
	{"EXPERIENCEKOEFPVP",			{ELEM_INT,		OFFSETOF(CResource, m_iExperienceKoefPVP),				0}},
	{"EXPERIENCEMODE",				{ELEM_INT,		OFFSETOF(CResource, m_iExperienceMode),					0}},
	{"EXPERIENCESYSTEM",			{ELEM_BOOL,		OFFSETOF(CResource, m_bExperienceSystem),				0}},
	{"EXPERIMENTAL",				{ELEM_INT,		OFFSETOF(CResource, m_iExperimental),					0}},
	{"FEATUREAOS",					{ELEM_INT,		OFFSETOF(CResource, m_iFeatureAOS),						0}},
	{"FEATUREEXTRA",				{ELEM_INT,		OFFSETOF(CResource, m_iFeatureExtra),					0}},
	{"FEATUREKR",					{ELEM_INT,		OFFSETOF(CResource, m_iFeatureKR),						0}},
	{"FEATURELBR",					{ELEM_INT,		OFFSETOF(CResource, m_iFeatureLBR),						0}},
	{"FEATUREML",					{ELEM_INT,		OFFSETOF(CResource, m_iFeatureML),						0}},
	{"FEATURESA",					{ELEM_INT,		OFFSETOF(CResource, m_iFeatureSA),						0}},
	{"FEATURESE",					{ELEM_INT,		OFFSETOF(CResource, m_iFeatureSE),						0}},
	{"FEATURET2A",					{ELEM_INT,		OFFSETOF(CResource, m_iFeatureT2A),						0}},
	{"FEATURETOL",					{ELEM_INT,		OFFSETOF(CResource, m_iFeatureTOL),						0}},
	{"FLIPDROPPEDITEMS",			{ELEM_BOOL,		OFFSETOF(CResource, m_fFlipDroppedItems),				0}},
	{"FORCEGARBAGECOLLECT",			{ELEM_BOOL,		OFFSETOF(CResource, m_fSaveGarbageCollect),				0}},
	{"FREEZERESTARTTIME",			{ELEM_INT,		OFFSETOF(CResource, m_iFreezeRestartTime),				0}},
	{"GAMEMINUTELENGTH",			{ELEM_INT,		OFFSETOF(CResource, m_iGameMinuteLength),				0}},
	{"GENERICSOUNDS",				{ELEM_BOOL,		OFFSETOF(CResource, m_fGenericSounds),					0}},
	{"GUARDLINGER",					{ELEM_INT,		OFFSETOF(CResource, m_iGuardLingerTime),				0}},
	{"GUARDSINSTANTKILL",			{ELEM_BOOL,		OFFSETOF(CResource, m_fGuardsInstantKill),				0}},
	{"GUARDSONMURDERERS",			{ELEM_BOOL,		OFFSETOF(CResource, m_fGuardsOnMurderers),				0}},
	{"GUILDS",						{ELEM_VOID,		0,														0}},
	{"HEARALL",						{ELEM_VOID,		0,														0}},
	{"HELPINGCRIMINALSISACRIME",	{ELEM_BOOL,		OFFSETOF(CResource, m_fHelpingCriminalsIsACrime),		0}},
	{"HITPOINTPERCENTONREZ",		{ELEM_INT,		OFFSETOF(CResource, m_iHitpointPercentOnRez),			0}},
	{"HITSHUNGERLOSS",				{ELEM_INT,		OFFSETOF(CResource, m_iHitsHungerLoss),					0}},
	{"HITSUPDATERATE",				{ELEM_INT,		OFFSETOF(CResource, m_iHitsUpdateRate),					0}},
	{"ITEMSMAXAMOUNT",				{ELEM_WORD,		OFFSETOF(CResource, m_iItemsMaxAmount),					0}},
	{"LEVELMODE",					{ELEM_INT,		OFFSETOF(CResource, m_iLevelMode),						0}},
	{"LEVELNEXTAT",					{ELEM_INT,		OFFSETOF(CResource, m_iLevelNextAt),					0}},
	{"LEVELSYSTEM",					{ELEM_BOOL,		OFFSETOF(CResource, m_bLevelSystem),					0}},
	{"LIGHTDAY",					{ELEM_INT,		OFFSETOF(CResource, m_iLightDay),						0}},
	{"LIGHTNIGHT",					{ELEM_INT,		OFFSETOF(CResource, m_iLightNight),						0}},
	{"LOG",							{ELEM_VOID,		0,														0}},
	{"LOGMASK",						{ELEM_VOID,		0,														0}},
	{"LOOTINGISACRIME",				{ELEM_BOOL,		OFFSETOF(CResource, m_fLootingIsACrime),				0}},
	{"LOSTNPCTELEPORT",				{ELEM_INT,		OFFSETOF(CResource, m_iLostNPCTeleport),				0}},
	{"MAGICFLAGS",					{ELEM_INT,		OFFSETOF(CResource, m_iMagicFlags),						0}},
	{"MAGICUNLOCKDOOR",				{ELEM_INT,		OFFSETOF(CResource, m_iMagicUnlockDoor),				0}},
	{"MAPCACHETIME",				{ELEM_INT,		OFFSETOF(CResource, m_iMapCacheTime),					0}},
	{"MAXBASESKILL",				{ELEM_WORD,		OFFSETOF(CResource, m_iMaxBaseSkill),					0}},
	{"MAXCHARSPERACCOUNT",			{ELEM_BYTE,		OFFSETOF(CResource, m_iMaxCharsPerAccount),				0}},
	{"MAXCOMPLEXITY",				{ELEM_INT,		OFFSETOF(CResource, m_iMaxCharComplexity),				0}},
	{"MAXFAME",						{ELEM_INT,		OFFSETOF(CResource, m_iMaxFame),						0}},
	{"MAXITEMCOMPLEXITY",			{ELEM_INT,		OFFSETOF(CResource, m_iMaxItemComplexity),				0}},
	{"MAXKARMA",					{ELEM_INT,		OFFSETOF(CResource, m_iMaxKarma),						0}},
	{"MAXLOOPTIMES",				{ELEM_INT,		OFFSETOF(CResource, m_iMaxLoopTimes),					0}},
	{"MAXPACKETSPERTICK",			{ELEM_INT,		OFFSETOF(CResource, m_iNetMaxPacketsPerTick),			0}},
	{"MAXPINGS",					{ELEM_INT,		OFFSETOF(CResource, m_iNetMaxPings),					0}},
	{"MAXPOLYSTATS",				{ELEM_INT,		OFFSETOF(CResource, m_iMaxPolyStats),					0}},
	{"MAXQUEUESIZE",				{ELEM_INT,		OFFSETOF(CResource, m_iNetMaxQueueSize),				0}},
	{"MAXSECTORCOMPLEXITY",			{ELEM_INT,		OFFSETOF(CResource, m_iMaxSectorComplexity),			0}},
	{"MAXSHIPPLANKTELEPORT",		{ELEM_INT,		OFFSETOF(CResource, m_iMaxShipPlankTeleport),			0}},
	{"MAXSIZEPERTICK",				{ELEM_INT,		OFFSETOF(CResource, m_iNetMaxLengthPerTick),			0}},
	{"MD5PASSWORDS",				{ELEM_BOOL,		OFFSETOF(CResource, m_fMd5Passwords),					0}},
	{"MEDIUMCANHEARGHOSTS",			{ELEM_INT,		OFFSETOF(CResource, m_iMediumCanHearGhosts),			0}},
	{"MINCHARDELETETIME",			{ELEM_INT,		OFFSETOF(CResource, m_iMinCharDeleteTime),				0}},
	{"MINKARMA",					{ELEM_INT,		OFFSETOF(CResource, m_iMinKarma),						0}},
	{"MONSTERFEAR",					{ELEM_BOOL,		OFFSETOF(CResource, m_fMonsterFear),					0}},
	{"MONSTERFIGHT",				{ELEM_BOOL,		OFFSETOF(CResource, m_fMonsterFight),					0}},
	{"MOUNTHEIGHT",					{ELEM_BOOL,		OFFSETOF(CResource, m_iMountHeight),					0}},
	{"MOVERATE",					{ELEM_INT,		OFFSETOF(CResource, m_iMoveRate),						0}},
	{"MULFILES",					{ELEM_VOID,		0,														0}},
	{"MURDERDECAYTIME",				{ELEM_INT,		OFFSETOF(CResource, m_iMurderDecayTime),				0}},
	{"MURDERMINCOUNT",				{ELEM_INT,		OFFSETOF(CResource, m_iMurderMinCount),					0}},
	{"MYSQL",						{ELEM_BOOL,		OFFSETOF(CResource, m_bMySql),							0}},
	{"MYSQLDATABASE",				{ELEM_CSTRING,	OFFSETOF(CResource, m_sMySqlDB),						0}},
	{"MYSQLHOST",					{ELEM_CSTRING,	OFFSETOF(CResource, m_sMySqlHost),						0}},
	{"MYSQLPASSWORD",				{ELEM_CSTRING,	OFFSETOF(CResource, m_sMySqlPass),						0}},
	{"MYSQLUSER",					{ELEM_CSTRING,	OFFSETOF(CResource, m_sMySqlUser),						0}},
	{"NETTTL",						{ELEM_INT,		OFFSETOF(CResource, m_iNetHistoryTTL),					0}},
	{"NETWORKTHREADPRIORITY",		{ELEM_INT,		OFFSETOF(CResource, m_iNetworkThreadPriority),			0}},
	{"NETWORKTHREADS",				{ELEM_INT,		OFFSETOF(CResource, m_iNetworkThreads),					0}},
	{"NORESROBE",					{ELEM_BOOL,		OFFSETOF(CResource, m_fNoResRobe),						0}},
	{"NOTOTIMEOUT",					{ELEM_INT,		OFFSETOF(CResource, m_iNotoTimeout),					0}},
	{"NOWEATHER",					{ELEM_BOOL,		OFFSETOF(CResource, m_fNoWeather),						0}},
	{"NPCAI",						{ELEM_INT,		OFFSETOF(CResource, m_iNpcAi),							0}},
	{"NPCNOFAMETITLE",				{ELEM_BOOL,		OFFSETOF(CResource, m_NPCNoFameTitle),					0}},
	{"NPCSKILLSAVE",				{ELEM_INT,		OFFSETOF(CResource, m_iSaveNPCSkills),					0}},
	{"NPCTRAINCOST",				{ELEM_WORD,		OFFSETOF(CResource, m_iTrainSkillCost),					0}},
	{"NPCTRAINMAX",					{ELEM_WORD,		OFFSETOF(CResource, m_iTrainSkillMax),					0}},
	{"NPCTRAINPERCENT",				{ELEM_INT,		OFFSETOF(CResource, m_iTrainSkillPercent),				0}},
	{"OPTIONFLAGS",					{ELEM_INT,		OFFSETOF(CResource, m_iOptionFlags),					0}},
	{"OVERSKILLMULTIPLY",			{ELEM_INT,		OFFSETOF(CResource, m_iOverSkillMultiply),				0}},
	{"PACKETDEATHANIMATION",		{ELEM_BOOL,		OFFSETOF(CResource, m_iPacketDeathAnimation),			0}},
	{"PAYFROMPACKONLY",				{ELEM_BOOL,		OFFSETOF(CResource, m_fPayFromPackOnly),				0}},
	{"PETSINHERITNOTORIETY",		{ELEM_INT,		OFFSETOF(CResource, m_iPetsInheritNotoriety),			0}},
	{"PLAYEREVIL",					{ELEM_INT,		OFFSETOF(CResource, m_iPlayerKarmaEvil),				0}},
	{"PLAYERNEUTRAL",				{ELEM_INT,		OFFSETOF(CResource, m_iPlayerKarmaNeutral),				0}},
	{"PROFILE",						{ELEM_VOID,		0,														0}},
	{"RACIALFLAGS",					{ELEM_INT,		OFFSETOF(CResource, m_iRacialFlags),					0}},
	{"REAGENTLOSSFAIL",				{ELEM_BOOL,		OFFSETOF(CResource, m_fReagentLossFail),				0}},
	{"REAGENTSREQUIRED",			{ELEM_BOOL,		OFFSETOF(CResource, m_fReagentsRequired),				0}},
	{"REVEALFLAGS",					{ELEM_INT,		OFFSETOF(CResource, m_iRevealFlags),					0}},
	{"RTICKS",						{ELEM_VOID,		0,														0}},
	{"RTIME",						{ELEM_VOID,		0,														0}},
	{"RUNNINGPENALTY",				{ELEM_INT,		OFFSETOF(CResource, m_iStamRunningPenalty),				0}},
	{"SAVEBACKGROUND",				{ELEM_INT,		OFFSETOF(CResource, m_iSaveBackgroundTime),				0}},
	{"SAVEPERIOD",					{ELEM_INT,		OFFSETOF(CResource, m_iSavePeriod),						0}},
	{"SAVESECTORSPERTICK",			{ELEM_INT,		OFFSETOF(CResource, m_iSaveSectorsPerTick),				0}},
	{"SAVESTEPMAXCOMPLEXITY",		{ELEM_INT,		OFFSETOF(CResource, m_iSaveStepMaxComplexity),			0}},
	{"SCPFILES",					{ELEM_CSTRING,	OFFSETOF(CResource, m_sSCPBaseDir),						0}},
	{"SECTORSLEEP",					{ELEM_INT,		OFFSETOF(CResource, m_iSectorSleepMask),				0}},
	{"SECURE",						{ELEM_BOOL,		OFFSETOF(CResource, m_fSecure),							0}},
	{"SKILLPRACTICEMAX",			{ELEM_WORD,		OFFSETOF(CResource, m_iSkillPracticeMax),				0}},
	{"SNOOPCRIMINAL",				{ELEM_INT,		OFFSETOF(CResource, m_iSnoopCriminal),					0}},
	{"SPEECHOTHER",					{ELEM_CSTRING,	OFFSETOF(CResource, m_sSpeechOther),					0}},
	{"SPEECHPET",					{ELEM_CSTRING,	OFFSETOF(CResource, m_sSpeechPet),						0}},
	{"SPEECHSELF",					{ELEM_CSTRING,	OFFSETOF(CResource, m_sSpeechSelf),						0}},
	{"SPEEDSCALEFACTOR",			{ELEM_INT,		OFFSETOF(CResource, m_iSpeedScaleFactor),				0}},
	{"SPELLTIMEOUT",				{ELEM_INT,		OFFSETOF(CResource, m_iSpellTimeout),					0}},
	{"STAMINALOSSATWEIGHT",			{ELEM_INT,		OFFSETOF(CResource, m_iStaminaLossAtWeight),			0}},
	{"STATSFLAGS",					{ELEM_INT,		OFFSETOF(CResource, m_iStatFlag),						0}},
	{"STRIPPATH",					{ELEM_INT,		OFFSETOF(CResource, m_sStripPath),						0}},
	{"SUPPRESSCAPITALS",			{ELEM_BOOL,		OFFSETOF(CResource, m_fSuppressCapitals),				0}},
	{"TELEPORTEFFECTNPC",			{ELEM_INT,		OFFSETOF(CResource, m_iSpell_Teleport_Effect_NPC),		0}},
	{"TELEPORTEFFECTPLAYERS",		{ELEM_INT,		OFFSETOF(CResource, m_iSpell_Teleport_Effect_Players),	0}},
	{"TELEPORTEFFECTSTAFF",			{ELEM_INT,		OFFSETOF(CResource, m_iSpell_Teleport_Effect_Staff),	0}},
	{"TELEPORTSOUNDNPC",			{ELEM_INT,		OFFSETOF(CResource, m_iSpell_Teleport_Sound_NPC),		0}},
	{"TELEPORTSOUNDPLAYERS",		{ELEM_INT,		OFFSETOF(CResource, m_iSpell_Teleport_Sound_Players),	0}},
	{"TELEPORTSOUNDSTAFF",			{ELEM_INT,		OFFSETOF(CResource, m_iSpell_Teleport_Sound_Staff),		0}},
	{"TIMERCALL",					{ELEM_INT,		OFFSETOF(CResource, m_iTimerCall),						0}},
	{"TIMEUP",						{ELEM_VOID,		0,														0}},
	{"TOOLTIPCACHE",				{ELEM_INT,		OFFSETOF(CResource, m_iTooltipCache),					0}},
	{"TOOLTIPMODE",					{ELEM_INT,		OFFSETOF(CResource, m_iTooltipMode),					0}},
	{"TRADEWINDOWSNOOPING",			{ELEM_BOOL,		OFFSETOF(CResource, m_iTradeWindowSnooping),			0}},
	{"UOGSTATUS",					{ELEM_BOOL,		OFFSETOF(CResource, m_fUOGStatus),						0}},
	{"USEASYNCNETWORK",				{ELEM_INT,		OFFSETOF(CResource, m_fUseAsyncNetwork),				0}},
	{"USEAUTHID",					{ELEM_BOOL,		OFFSETOF(CResource, m_fUseAuthID),						0}},
	{"USECRYPT",					{ELEM_BOOL,		OFFSETOF(CResource, m_fUsecrypt),						0}},
	{"USEEXTRABUFFER",				{ELEM_BOOL,		OFFSETOF(CResource, m_fUseExtraBuffer),					0}},
	{"USEHTTP",						{ELEM_INT,		OFFSETOF(CResource, m_fUseHTTP),						0}},
	{"USEMAPDIFFS",					{ELEM_BOOL,		OFFSETOF(CResource, m_fUseMapDiffs),					0}},
	{"USENOCRYPT",					{ELEM_BOOL,		OFFSETOF(CResource, m_fUsenocrypt),						0}},
	{"USEPACKETPRIORITY",			{ELEM_BOOL,		OFFSETOF(CResource, m_fUsePacketPriorities),			0}},
	{"VENDORMAXSELL",				{ELEM_INT,		OFFSETOF(CResource, m_iVendorMaxSell),					0}},
	{"VENDORTRADETITLE",			{ELEM_BOOL,		OFFSETOF(CResource, m_fVendorTradeTitle),				0}},
	{"VERSION",						{ELEM_VOID,		0,														0}},
	{"WALKBUFFER",					{ELEM_INT,		OFFSETOF(CResource, m_iWalkBuffer),						0}},
	{"WALKREGEN",					{ELEM_INT,		OFFSETOF(CResource, m_iWalkRegen),						0}},
	{"WOOLGROWTHTIME",				{ELEM_INT,		OFFSETOF(CResource, m_iWoolGrowthTime),					0}},
	{"WOPCOLOR",					{ELEM_INT,		OFFSETOF(CResource, m_iWordsOfPowerColor),				0}},
	{"WOPFONT",						{ELEM_INT,		OFFSETOF(CResource, m_iWordsOfPowerFont),				0}},
	{"WOPPLAYER",					{ELEM_BOOL,		OFFSETOF(CResource, m_fWordsOfPowerPlayer),				0}},
	{"WOPSTAFF",					{ELEM_BOOL,		OFFSETOF(CResource, m_fWordsOfPowerStaff),				0}},
	{"WORLDSAVE",					{ELEM_CSTRING,	OFFSETOF(CResource, m_sWorldBaseDir),					0}},
	{"ZEROPOINT",					{ELEM_CSTRING,	OFFSETOF(CResource, m_sZeroPoint),						0}},
	{NULL,							{ELEM_VOID,		0,														0}}
};

bool CResource::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CResource::r_LoadVal");
	EXC_TRY("LoadVal");

	int index = FindTableHeadSortedRes(s.GetKey(), reinterpret_cast<const LPCTSTR *>(sm_szLoadKeys), COUNTOF(sm_szLoadKeys) - 1);
	if ( index < 0 )
	{
		if ( s.IsKeyHead("REGEN", 5) )	// REGENx=<stat regeneration rate>
		{
			int iStat = ATOI(s.GetKey() + 5);
			if ( (iStat < 0) || (iStat >= STAT_QTY) )
				return false;

			g_Cfg.m_iRegenRate[iStat] = s.GetArgVal() * TICK_PER_SEC;
			return true;
		}
		else if ( s.IsKeyHead("MAP", 3) )	// MAPx=settings
		{
			bool fOk = true;
			std::string sArgs = s.GetKey() + 3;
			for ( size_t i = 0; i < sArgs.size(); ++i )
			{
				if ( IsDigit(sArgs[i]) )
					continue;

				fOk = false;
				break;
			}

			if ( fOk && (sArgs.size() > 0) )
				return g_MapList.Load(ATOI(sArgs.c_str()), s.GetArgRaw());

			if ( (sArgs.size() >= 2) /*at least .X*/ && (sArgs[0] == '.') && isdigit(sArgs[1]) )
			{
				LPCTSTR pszArgs = &sArgs[1];
				int iMap = Exp_GetVal(pszArgs);

				if ( g_MapList.IsMapSupported(iMap) )
				{
					SKIP_SEPARATORS(pszArgs);
					if ( strcmpi(pszArgs, "ALLSECTORS") == 0 )
					{
						pszArgs = s.GetArgRaw();
						if ( pszArgs && *pszArgs )
						{
							CScript scp(pszArgs);
							int iSectorQty = g_MapList.GetSectorQty(iMap);
							for ( int i = 0; i < iSectorQty; ++i )
								g_World.GetSector(iMap, i)->r_Verb(scp, &g_Serv);
						}
						return true;
					}
					else if ( !strnicmp(pszArgs, "SECTOR.", 7) )
					{
						pszArgs = pszArgs + 7;
						int iSector = Exp_GetVal(pszArgs);
						SKIP_SEPARATORS(pszArgs);

						if ( (iSector < 0) || (iSector >= g_MapList.GetSectorQty(iMap)) )
						{
							g_Log.EventError("Invalid sector #%d for map %d\n", iSector, iMap);
							return false;
						}

						pszArgs = s.GetArgRaw();
						if ( pszArgs && *pszArgs )
						{
							CScript scp(pszArgs);
							g_World.GetSector(iMap, iSector - 1)->r_Verb(scp, &g_Serv);
						}
						return true;
					}
				}
			}

			DEBUG_ERR(("Bad usage of MAPx. Check your " SPHERE_FILE ".ini or scripts (SERV.MAP is a read only property)\n"));
			return false;
		}
		else if ( s.IsKeyHead("PACKET", 6) )	// PACKETx=<function name to execute upon packet>
		{
			int iPacket = ATOI(s.GetKey() + 6);
			if ( (iPacket < 0) || (iPacket >= PACKET_QTY) )
			{
				g_Log.EventError("Packet filtering index %d out of range [0~%d]\n", iPacket, PACKET_QTY - 1);
				return false;
			}

			char *pchArgs = s.GetArgRaw();
			if ( !pchArgs || (strlen(pchArgs) > 30) )
			{
				g_Log.EventError("Invalid function name for packet filtering (limit is 30 chars)\n");
				return false;
			}

			strcpy(g_Serv.m_PacketFilter[iPacket], pchArgs);
			DEBUG_MSG(("PACKET FILTER: Hooked packet '0x%x' with function '%s'\n", iPacket, pchArgs));
			return true;
		}
		else if ( s.IsKeyHead("OUTPACKET", 9) )		// OUTPACKETx=<function name to execute upon packet>
		{
			int iPacket = ATOI(s.GetKey() + 9);
			if ( (iPacket < 0) || (iPacket >= PACKET_QTY) )
			{
				g_Log.EventError("Outgoing packet filtering index %d out of range [0~%d]\n", iPacket, PACKET_QTY - 1);
				return false;
			}

			char *pchArgs = s.GetArgRaw();
			if ( !pchArgs || (strlen(pchArgs) > 30) )
			{
				g_Log.EventError("Invalid function name for outgoing packet filtering (limit is 30 chars)\n");
				return false;
			}

			strcpy(g_Serv.m_OutPacketFilter[iPacket], pchArgs);
			DEBUG_MSG(("OUTGOING PACKET FILTER: Hooked packet '0x%x' with function '%s'\n", iPacket, pchArgs));
			return true;
		}
		return false;
	}

	switch ( index )
	{
		case RC_ACCTFILES:
			m_sAcctBaseDir = CGFile::GetMergedFileName(s.GetArgStr(), "");
			break;
		case RC_ATTACKERTIMEOUT:
			m_iAttackerTimeout = s.GetArgVal() * TICK_PER_SEC;
			break;
		case RC_BACKPACKMAXWEIGHT:
			m_iBackpackMaxWeight = s.GetArgVal() * WEIGHT_UNITS;
			break;
		case RC_BANKMAXWEIGHT:
			m_iBankWMax = s.GetArgVal() * WEIGHT_UNITS;
			break;
		case RC_CHATFLAGS:
			m_iChatFlags = s.GetArgVal();
			break;
		case RC_CHATSTATICCHANNELS:
			m_sChatStaticChannels = s.GetArgStr();
			break;
		case RC_CLIENTLINGER:
			m_iClientLingerTime = s.GetArgVal() * TICK_PER_SEC;
			break;
		case RC_CLIENTLOGINMAXTRIES:
		{
			int iVal = s.GetArgVal();
			m_iClientLoginMaxTries = maximum(0, iVal);
			break;
		}
		case RC_CLIENTLOGINTEMPBAN:
			m_iClientLoginTempBan = s.GetArgVal() * 60 * TICK_PER_SEC;
			break;
		case RC_CLIENTMAX:
		{
			int iVal = s.GetArgVal();
			m_iClientsMax = minimum(maximum(0, iVal), FD_SETSIZE - 1);
			break;
		}
		case RC_COLORHIDDEN:
			m_iColorHidden = static_cast<HUE_TYPE>(s.GetArgLLVal());
			break;
		case RC_COLORINVIS:
			m_iColorInvis = static_cast<HUE_TYPE>(s.GetArgLLVal());
			break;
		case RC_COLORINVISSPELL:
			m_iColorInvisSpell = static_cast<HUE_TYPE>(s.GetArgLLVal());
			break;
		case RC_CORPSENPCDECAY:
			m_iDecay_CorpseNPC = s.GetArgVal() * 60 * TICK_PER_SEC;
			break;
		case RC_CORPSEPLAYERDECAY:
			m_iDecay_CorpsePlayer = s.GetArgVal() * 60 * TICK_PER_SEC;
			break;
		case RC_CRIMINALTIMER:
			m_iCriminalTimer = s.GetArgVal() * 60 * TICK_PER_SEC;
			break;
		case RC_STRIPPATH:
			m_sStripPath = CGFile::GetMergedFileName(s.GetArgStr(), "");
			break;
		case RC_DEADSOCKETTIME:
		{
			int iVal = s.GetArgVal();
			m_iDeadSocketTime = maximum(1, iVal) * 60 * TICK_PER_SEC;
			break;
		}
		case RC_DECAYTIMER:
			m_iDecay_Item = s.GetArgVal() * 60 * TICK_PER_SEC;
			break;
		case RC_FREEZERESTARTTIME:
		{
			int iVal = s.GetArgVal();
			m_iFreezeRestartTime = maximum(10, iVal) * TICK_PER_SEC;
			break;
		}
		case RC_GAMEMINUTELENGTH:
			m_iGameMinuteLength = s.GetArgVal() * TICK_PER_SEC;
			break;
		case RC_GUARDLINGER:
			m_iGuardLingerTime = s.GetArgVal() * 60 * TICK_PER_SEC;
			break;
		case RC_HEARALL:
			g_Log.SetLogMask(static_cast<DWORD>(s.GetArgFlag(g_Log.GetLogMask(), LOGM_PLAYER_SPEAK)));
			break;
		case RC_HITSUPDATERATE:
			m_iHitsUpdateRate = s.GetArgVal() * TICK_PER_SEC;
			break;
		case RC_LOG:
			g_Log.OpenLog(s.GetArgStr());
			break;
		case RC_LOGMASK:
			g_Log.SetLogMask(static_cast<DWORD>(s.GetArgLLVal()));
			break;
		case RC_MULFILES:
			g_Install.SetMulPath(CGFile::GetMergedFileName(s.GetArgStr(), ""));
			break;
		case RC_MAPCACHETIME:
			m_iMapCacheTime = s.GetArgVal() * TICK_PER_SEC;
			break;
		case RC_MAXCHARSPERACCOUNT:
		{
			BYTE bVal = static_cast<BYTE>(s.GetArgLLVal());
			m_iMaxCharsPerAccount = minimum(bVal, MAX_CHARS_PER_ACCT);
			break;
		}
		case RC_MAXFAME:
		{
			int iVal = s.GetArgVal();
			m_iMaxFame = maximum(0, iVal);
			break;
		}
		case RC_MAXKARMA:
		{
			int iVal = s.GetArgVal();
			m_iMaxKarma = maximum(m_iMinKarma, iVal);
			break;
		}
		case RC_MINCHARDELETETIME:
			m_iMinCharDeleteTime = s.GetArgVal() * TICK_PER_SEC;
			break;
		case RC_MINKARMA:
		{
			int iVal = s.GetArgVal();
			m_iMinKarma = minimum(iVal, m_iMaxKarma);
			break;
		}
		case RC_MURDERDECAYTIME:
			m_iMurderDecayTime = s.GetArgVal() * TICK_PER_SEC;
			break;
		case RC_NOTOTIMEOUT:
			m_iNotoTimeout = s.GetArgVal() * TICK_PER_SEC;
			break;
		case RC_WOOLGROWTHTIME:
			m_iWoolGrowthTime = s.GetArgVal() * 60 * TICK_PER_SEC;
			break;
		case RC_PROFILE:
		{
			int iSeconds = s.GetArgVal();
			size_t iThreadCount = ThreadHolder::getActiveThreads();
			for ( size_t j = 0; j < iThreadCount; ++j )
			{
				AbstractSphereThread *pThread = static_cast<AbstractSphereThread *>(ThreadHolder::getThreadAt(j));
				if ( pThread )
					pThread->m_profile.SetActive(iSeconds);
			}
			break;
		}
		case RC_PLAYEREVIL:
		{
			int iVal = s.GetArgVal();
			m_iPlayerKarmaEvil = minimum(iVal, m_iPlayerKarmaNeutral);
			break;
		}
		case RC_PLAYERNEUTRAL:
		{
			int iVal = s.GetArgVal();
			m_iPlayerKarmaNeutral = maximum(m_iPlayerKarmaEvil, iVal);
			break;
		}
		case RC_GUARDSINSTANTKILL:
			m_fGuardsInstantKill = s.GetArgVal() ? true : false;
			break;
		case RC_GUARDSONMURDERERS:
			m_fGuardsOnMurderers = s.GetArgVal() ? true : false;
			break;
		case RC_SCPFILES:
			m_sSCPBaseDir = CGFile::GetMergedFileName(s.GetArgStr(), "");
			break;
		case RC_SECURE:
			m_fSecure = (s.GetArgVal() > 0);
#ifndef _WIN32
			if ( !g_Serv.IsLoading() )
				SetSignals(g_Cfg.m_fSecure);
#endif
			break;
		case RC_PACKETDEATHANIMATION:
			m_iPacketDeathAnimation = (s.GetArgVal() > 0);
			break;
		case RC_SKILLPRACTICEMAX:
			m_iSkillPracticeMax = static_cast<WORD>(s.GetArgLLVal());
			break;
		case RC_SAVEPERIOD:
			m_iSavePeriod = s.GetArgVal() * 60 * TICK_PER_SEC;
			break;
		case RC_SPELLTIMEOUT:
			m_iSpellTimeout = s.GetArgVal() * TICK_PER_SEC;
			break;
		case RC_SECTORSLEEP:
		{
			int iVal = s.GetArgVal();
			m_iSectorSleepMask = (1 << minimum(maximum(0, iVal), 31)) - 1;
			break;
		}
		case RC_SAVEBACKGROUND:
			m_iSaveBackgroundTime = s.GetArgVal() * 60 * TICK_PER_SEC;
			break;
		case RC_SAVESECTORSPERTICK:
		{
			int iVal = s.GetArgVal();
			m_iSaveSectorsPerTick = maximum(1, iVal);
			break;
		}
		case RC_SAVESTEPMAXCOMPLEXITY:
			m_iSaveStepMaxComplexity = s.GetArgVal();
			break;
		case RC_WORLDSAVE:
			m_sWorldBaseDir = CGFile::GetMergedFileName(s.GetArgStr(), "");
			break;
		case RC_COMMANDPREFIX:
			m_cCommandPrefix = *s.GetArgStr();
			break;
		case RC_EXPERIMENTAL:
			m_iExperimental = s.GetArgVal();
			break;
		case RC_OPTIONFLAGS:
			m_iOptionFlags = s.GetArgVal();
			break;
		case RC_TIMERCALL:
			m_iTimerCall = s.GetArgVal() * 60 * TICK_PER_SEC;
			break;
		case RC_TOOLTIPCACHE:
			m_iTooltipCache = s.GetArgVal() * TICK_PER_SEC;
			break;
		case RC_NETWORKTHREADS:
			if ( g_Serv.IsLoading() )
				g_Cfg.m_iNetworkThreads = s.GetArgVal();
			else
				g_Log.EventError("NetworkThreads setting can't be changed after the server has started\n");
			break;
		case RC_NETWORKTHREADPRIORITY:
		{
			int iVal = s.GetArgVal();
			m_iNetworkThreadPriority = minimum(maximum(IThread::Idle, iVal), IThread::RealTime);
			break;
		}
		case RC_WALKBUFFER:
			m_iWalkBuffer = s.GetArgVal() * TICK_PER_SEC;
			break;
		default:
			return sm_szLoadKeys[index].m_elem.SetValStr(this, s.GetArgRaw());
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

const CSkillDef *CResource::SkillLookup(LPCTSTR pszKey)
{
	ADDTOCALLSTACK("CResource::SkillLookup");

	const CSkillDef *pDef;
	size_t iLen = strlen(pszKey);
	size_t iMax = m_SkillIndexDefs.GetCount();

	for ( size_t i = 0; i < iMax; ++i )
	{
		pDef = static_cast<const CSkillDef *>(m_SkillIndexDefs[i]);
		if ( !pDef->m_sName.IsEmpty() )
		{
			if ( !strnicmp(pszKey, pDef->m_sName, iLen) )
				return pDef;
		}
		else
		{
			if ( !strnicmp(pszKey, pDef->GetKey(), iLen) )
				return pDef;
		}
	}
	return NULL;
}

bool CResource::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CResource::r_WriteVal");
	EXC_TRY("WriteVal");

	// Just do stats values for now
	int index = FindTableHeadSortedRes(pszKey, reinterpret_cast<const LPCTSTR *>(sm_szLoadKeys), COUNTOF(sm_szLoadKeys) - 1);
	if ( index < 0 )
	{
		if ( !strnicmp(pszKey, "REGEN", 5) )
		{
			int i = ATOI(pszKey + 5);
			if ( (i < STAT_STR) || (i >= STAT_QTY) )
				return false;

			sVal.FormatVal(g_Cfg.m_iRegenRate[i] / TICK_PER_SEC);
			return true;
		}
		else if ( !strnicmp(pszKey, "LOOKUPSKILL", 11) )
		{
			pszKey += 11;
			SKIP_SEPARATORS(pszKey);
			GETNONWHITESPACE(pszKey);

			const CSkillDef *pSkillDef = SkillLookup(pszKey);
			sVal.FormatVal(pSkillDef ? pSkillDef->GetResourceID().GetResIndex() : -1);
			return true;
		}
		else if ( !strnicmp(pszKey, "MAP(", 4) )
		{
			pszKey += 4;
			TCHAR *pszArgsNext;
			Str_Parse(const_cast<TCHAR *>(pszKey), &pszArgsNext, ")");

			CPointMap pt;
			if ( IsDigit(pszKey[0]) || (pszKey[0] == '-') )
			{
				TCHAR *ppVal[4];
				size_t iArgQty = Str_ParseCmds(const_cast<TCHAR *>(pszKey), ppVal, COUNTOF(ppVal), ",");
				switch ( iArgQty )
				{
					default:
					case 4:
						if ( IsDigit(ppVal[3][0]) )
							pt.m_map = static_cast<BYTE>(ATOI(ppVal[3]));
					case 3:
						if ( IsDigit(ppVal[2][0]) || ((iArgQty == 4) && (ppVal[2][0] == '-')) )
						{
							pt.m_z = static_cast<signed char>((iArgQty == 4) ? ATOI(ppVal[2]) : 0);
							if ( iArgQty == 3 )
								pt.m_map = static_cast<BYTE>(ATOI(ppVal[2]));
						}
					case 2:
						pt.m_y = static_cast<signed short>(ATOI(ppVal[1]));
					case 1:
						pt.m_x = static_cast<signed short>(ATOI(ppVal[0]));
					case 0:
						break;
				}
			}

			pszKey = pszArgsNext;
			SKIP_SEPARATORS(pszKey);

			sVal.FormatVal(0);
			return pt.r_WriteVal(pszKey, sVal);
		}
		else if ( !strnicmp(pszKey, "MAPLIST.", 8) )
		{
			LPCTSTR pszCmd = pszKey + 8;
			int iMap = Exp_GetVal(pszCmd);
			SKIP_SEPARATORS(pszCmd);

			if ( !*pszCmd )
			{
				sVal.FormatVal(g_MapList.IsMapSupported(iMap));
				return true;
			}

			sVal.FormatVal(0);
			if ( !g_MapList.IsMapSupported(iMap) )
			{
				g_Log.EventError("Unsupported map %d\n", iMap);
				return false;
			}
			else if ( !strcmpi(pszCmd, "BOUND.X") )
				sVal.FormatVal(g_MapList.GetX(iMap));
			else if ( !strcmpi(pszCmd, "BOUND.Y") )
				sVal.FormatVal(g_MapList.GetY(iMap));
			else if ( !strcmpi(pszCmd, "CENTER.X") )
				sVal.FormatVal(g_MapList.GetX(iMap) / 2);
			else if ( !strcmpi(pszCmd, "CENTER.Y") )
				sVal.FormatVal(g_MapList.GetY(iMap) / 2);
			else if ( !strcmpi(pszCmd, "SECTOR.SIZE") )
				sVal.FormatVal(g_MapList.GetSectorSize(iMap));
			else if ( !strcmpi(pszCmd, "SECTOR.ROWS") )
				sVal.FormatVal(g_MapList.GetSectorRows(iMap));
			else if ( !strcmpi(pszCmd, "SECTOR.COLS") )
				sVal.FormatVal(g_MapList.GetSectorCols(iMap));
			else if ( !strcmpi(pszCmd, "SECTOR.QTY") )
				sVal.FormatVal(g_MapList.GetSectorQty(iMap));
			return true;
		}
		else if ( !strnicmp(pszKey, "MAP", 3) )
		{
			pszKey = pszKey + 3;
			int iMap = Exp_GetVal(pszKey);
			SKIP_SEPARATORS(pszKey);

			sVal.FormatVal(0);
			if ( !g_MapList.IsMapSupported(iMap) )
			{
				g_Log.EventError("Unsupported map %d\n", iMap);
				return false;
			}
			else if ( !strnicmp(pszKey, "SECTOR", 6) )
			{
				pszKey = pszKey + 6;
				int iSector = Exp_GetVal(pszKey);
				SKIP_SEPARATORS(pszKey);

				if ( (iSector < 0) || (iSector >= g_MapList.GetSectorQty(iMap)) )
				{
					g_Log.EventError("Invalid sector #%d for map %d\n", iSector, iMap);
					return false;
				}
				return g_World.GetSector(iMap, iSector - 1)->r_WriteVal(pszKey, sVal, pSrc);
			}
		}
		else if ( !strnicmp(pszKey, "FUNCTIONS.", 10) )
		{
			LPCTSTR pszCmd = pszKey + 10;
			if ( !strnicmp(pszCmd, "COUNT", 5) )
			{
				sVal.FormatUVal(m_Functions.GetCount());
				return true;
			}
			else if ( m_Functions.ContainsKey(pszCmd) )
			{
				sVal.FormatVal(static_cast<long>(GetPrivCommandLevel(pszCmd)));
				return true;
			}

			size_t iNumber = Exp_GetVal(pszCmd);
			SKIP_SEPARATORS(pszCmd);

			sVal.FormatVal(0);
			if ( iNumber >= m_Functions.GetCount() )
			{
				g_Log.EventError("Invalid command index %" FMTSIZE_T "\n", iNumber);
				return false;
			}
			else if ( !*pszCmd )
			{
				sVal = m_Functions.GetAt(iNumber)->GetName();
				return true;
			}
			else if ( !strnicmp(pszCmd, "PLEVEL", 5) )
			{
				sVal.FormatVal(static_cast<long>(GetPrivCommandLevel(m_Functions.GetAt(iNumber)->GetName())));
				return true;
			}
		}
		else if ( !strnicmp(pszKey, "GUILDSTONES.", 12) || !strnicmp(pszKey, "TOWNSTONES.", 11) )
		{
			bool fGuild = !strnicmp(pszKey, "GUILDSTONES.", 12);
			LPCTSTR pszCmd = pszKey + (fGuild ? 12 : 11);
			CItemStone *pStone = NULL;

			if ( !strcmpi(pszCmd, "COUNT") )
			{
				size_t iCount = 0;
				size_t iMax = g_World.m_Stones.GetCount();
				for ( size_t i = 0; i < iMax; ++i )
				{
					pStone = g_World.m_Stones[i];
					if ( !pStone )
						continue;

					if ( (fGuild && (pStone->GetType() == IT_STONE_GUILD)) || (!fGuild && (pStone->GetType() == IT_STONE_TOWN)) )
						++iCount;
				}

				sVal.FormatUVal(iCount);
				return true;
			}

			size_t iNumber = static_cast<size_t>(Exp_GetLLVal(pszCmd));
			SKIP_SEPARATORS(pszCmd);

			sVal.FormatVal(0);
			if ( iNumber >= g_World.m_Stones.GetCount() )
				return true;

			pStone = g_World.m_Stones[iNumber];
			if ( !*pszCmd )
			{
				sVal.FormatVal(pStone ? 1 : 0);
				return true;
			}
			if ( pStone && ((fGuild && (pStone->GetType() == IT_STONE_GUILD)) || (!fGuild && (pStone->GetType() == IT_STONE_TOWN))) )
				return pStone->r_WriteVal(pszCmd, sVal, pSrc);

			return true;
		}
		else if ( !strnicmp(pszKey, "CLIENT.", 7) )
		{
			pszKey += 7;
			size_t iNumber = static_cast<size_t>(Exp_GetLLVal(pszKey));
			SKIP_SEPARATORS(pszKey);

			sVal.FormatVal(0);
			if ( iNumber >= g_Serv.StatGet(SERV_STAT_CLIENTS) )
				return true;

			size_t i = 0;
			ClientIterator it;
			for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next(), ++i )
			{
				if ( iNumber != i )
					continue;

				CChar *pChar = pClient->GetChar();
				if ( !*pszKey )
					sVal.FormatVal(pChar ? 1 : 0);
				else if ( pChar )
					return pChar->r_WriteVal(pszKey, sVal, pSrc);
				else
					return pClient->r_WriteVal(pszKey, sVal, pSrc);
			}
			return true;
		}
		return false;
	}

	switch ( index )
	{
		case RC_ATTACKERTIMEOUT:
			sVal.FormatUVal(m_iAttackerTimeout / TICK_PER_SEC);
			break;
		case RC_BACKPACKMAXWEIGHT:
			sVal.FormatVal(m_iBackpackMaxWeight / WEIGHT_UNITS);
			break;
		case RC_BANKMAXWEIGHT:
			sVal.FormatVal(m_iBankWMax / WEIGHT_UNITS);
			break;
		case RC_BUILD:
#ifdef __GITREVISION__
			sVal.FormatVal(__GITREVISION__);
#else
			sVal = g_szCompiledDate;
#endif
			break;
		case RC_CHATFLAGS:
			sVal.FormatHex(m_iChatFlags);
			break;
		case RC_CHATSTATICCHANNELS:
			sVal = m_sChatStaticChannels;
			break;
		case RC_CLIENTLINGER:
			sVal.FormatVal(m_iClientLingerTime / TICK_PER_SEC);
			break;
		case RC_CLIENTLOGINTEMPBAN:
			sVal.FormatVal(m_iClientLoginTempBan / (60 * TICK_PER_SEC));
			break;
		case RC_COLORHIDDEN:
			sVal.FormatHex(m_iColorHidden);
			break;
		case RC_COLORINVIS:
			sVal.FormatHex(m_iColorInvis);
			break;
		case RC_COLORINVISSPELL:
			sVal.FormatHex(m_iColorInvisSpell);
			break;
		case RC_CORPSENPCDECAY:
			sVal.FormatVal(m_iDecay_CorpseNPC / (60 * TICK_PER_SEC));
			break;
		case RC_CORPSEPLAYERDECAY:
			sVal.FormatVal(m_iDecay_CorpsePlayer / (60 * TICK_PER_SEC));
			break;
		case RC_CRIMINALTIMER:
			sVal.FormatVal(m_iCriminalTimer / (60 * TICK_PER_SEC));
			break;
		case RC_DEADSOCKETTIME:
			sVal.FormatVal(m_iDeadSocketTime / (60 * TICK_PER_SEC));
			break;
		case RC_DECAYTIMER:
			sVal.FormatVal(m_iDecay_Item / (60 * TICK_PER_SEC));
			break;
		case RC_FREEZERESTARTTIME:
			sVal.FormatVal(m_iFreezeRestartTime / TICK_PER_SEC);
			break;
		case RC_GAMEMINUTELENGTH:
			sVal.FormatVal(m_iGameMinuteLength / TICK_PER_SEC);
			break;
		case RC_GUARDLINGER:
			sVal.FormatVal(m_iGuardLingerTime / (60 * TICK_PER_SEC));
			break;
		case RC_HEARALL:
			sVal.FormatUVal(g_Log.GetLogMask() & LOGM_PLAYER_SPEAK);
			break;
		case RC_HITSUPDATERATE:
			sVal.FormatVal(m_iHitsUpdateRate / TICK_PER_SEC);
			break;
		case RC_LOG:
			sVal = g_Log.GetLogDir();
			break;
		case RC_LOGMASK:
			sVal.FormatHex(g_Log.GetLogMask());
			break;
		case RC_MULFILES:
			sVal = g_Install.GetMulPath();
			break;
		case RC_MAPCACHETIME:
			sVal.FormatVal(m_iMapCacheTime / TICK_PER_SEC);
			break;
		case RC_NOTOTIMEOUT:
			sVal.FormatUVal(m_iNotoTimeout / TICK_PER_SEC);
			break;
		case RC_MAXFAME:
			sVal.FormatVal(m_iMaxFame);
			break;
		case RC_MAXKARMA:
			sVal.FormatVal(m_iMaxKarma);
			break;
		case RC_MINCHARDELETETIME:
			sVal.FormatVal(m_iMinCharDeleteTime / TICK_PER_SEC);
			break;
		case RC_MINKARMA:
			sVal.FormatVal(m_iMinKarma);
			break;
		case RC_MURDERDECAYTIME:
			sVal.FormatVal(m_iMurderDecayTime / (TICK_PER_SEC));
			break;
		case RC_WOOLGROWTHTIME:
			sVal.FormatVal(m_iWoolGrowthTime / (60 * TICK_PER_SEC));
			break;
		case RC_PROFILE:
			sVal.FormatVal(CurrentProfileData.IsActive());
			break;
		case RC_RTICKS:
		{
			if ( pszKey[6] != '.' )
			{
				sVal.FormatULLVal(static_cast<UINT64>(CGTime::GetCurrentTime().GetTime()));
				break;
			}

			pszKey += 6;
			SKIP_SEPARATORS(pszKey);

			if ( !strnicmp("FROMTIME", pszKey, 8) )
			{
				pszKey += 8;
				GETNONWHITESPACE(pszKey);

				// Syntax: year, month, day, hour, minute, second
				INT64 piVal[6];
				size_t iArgQty = Str_ParseCmds(const_cast<TCHAR *>(pszKey), piVal, COUNTOF(piVal));
				if ( iArgQty != 6 )
					return false;

				CGTime datetime(static_cast<int>(piVal[0]), static_cast<int>(piVal[1]), static_cast<int>(piVal[2]), static_cast<int>(piVal[3]), static_cast<int>(piVal[4]), static_cast<int>(piVal[5]));
				sVal.FormatULLVal(static_cast<UINT64>(datetime.GetTime()));
				break;
			}
			else if ( !strnicmp("FORMAT", pszKey, 6) )
			{
				pszKey += 6;
				GETNONWHITESPACE(pszKey);

				// Syntax: timestamp, formatstr
				TCHAR *ppVal[2];
				size_t iArgQty = Str_ParseCmds(const_cast<TCHAR *>(pszKey), ppVal, COUNTOF(ppVal));
				if ( iArgQty < 1 )
					return false;

				CGTime datetime(static_cast<time_t>(Exp_GetLLVal(ppVal[0])));
				sVal = datetime.Format((iArgQty > 1) ? ppVal[1] : NULL);
				break;
			}
			return false;
		}
		case RC_RTIME:
		{
			if ( pszKey[5] != '.' )
			{
				sVal = CGTime::GetCurrentTime().Format(NULL);
				break;
			}

			pszKey += 5;
			SKIP_SEPARATORS(pszKey);

			if ( !strnicmp("FORMAT", pszKey, 6) )
			{
				pszKey += 6;
				GETNONWHITESPACE(pszKey);
				sVal = CGTime::GetCurrentTime().Format(pszKey);
				break;
			}
			else if ( !strnicmp("GMTFORMAT", pszKey, 9) )
			{
				pszKey += 9;
				GETNONWHITESPACE(pszKey);
				sVal = CGTime::GetCurrentTime().FormatGmt(pszKey);
				break;
			}
			return false;
		}
		case RC_SAVEPERIOD:
			sVal.FormatVal(m_iSavePeriod / (60 * TICK_PER_SEC));
			break;
		case RC_SECTORSLEEP:
			sVal.FormatVal(m_iSectorSleepMask ? static_cast<long>(log(static_cast<double>(m_iSectorSleepMask) + 1) / log(static_cast<double>(2))) : 0);
			break;
		case RC_SAVEBACKGROUND:
			sVal.FormatVal(m_iSaveBackgroundTime / (60 * TICK_PER_SEC));
			break;
		case RC_SAVESECTORSPERTICK:
			sVal.FormatUVal(m_iSaveSectorsPerTick);
			break;
		case RC_SAVESTEPMAXCOMPLEXITY:
			sVal.FormatUVal(m_iSaveStepMaxComplexity);
			break;
		case RC_SPELLTIMEOUT:
			sVal.FormatVal(m_iSpellTimeout / TICK_PER_SEC);
			break;
		case RC_GUILDS:
			sVal.FormatUVal(g_World.m_Stones.GetCount());
			return true;
		case RC_TIMEUP:
			sVal.FormatLLVal(-g_World.GetTimeDiff(g_World.m_timeStartup) / TICK_PER_SEC);
			return true;
		case RC_TIMERCALL:
			sVal.FormatVal(m_iTimerCall / (60 * TICK_PER_SEC));
			break;
		case RC_VERSION:
			sVal = SPHERE_TITLE_VER;
			break;
		case RC_EXPERIMENTAL:
			sVal.FormatHex(g_Cfg.m_iExperimental);
			break;
		case RC_OPTIONFLAGS:
			sVal.FormatHex(g_Cfg.m_iOptionFlags);
			break;
		case RC_PLAYEREVIL:
			sVal.FormatVal(m_iPlayerKarmaEvil);
			break;
		case RC_PLAYERNEUTRAL:
			sVal.FormatVal(m_iPlayerKarmaNeutral);
			break;
		case RC_TOOLTIPCACHE:
			sVal.FormatVal(m_iTooltipCache / TICK_PER_SEC);
			break;
		case RC_GUARDSINSTANTKILL:
			sVal.FormatVal(g_Cfg.m_fGuardsInstantKill);
			break;
		case RC_GUARDSONMURDERERS:
			sVal.FormatVal(g_Cfg.m_fGuardsOnMurderers);
			break;
		case RC_WALKBUFFER:
			sVal.FormatVal(m_iWalkBuffer / TICK_PER_SEC);
			break;
		default:
			return sm_szLoadKeys[index].m_elem.GetValStr(this, sVal);
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

///////////////////////////////////////////////////////////

bool CResource::IsConsoleCmd(TCHAR ch) const
{
	ADDTOCALLSTACK("CResource::IsConsoleCmd");
	return ((ch == '.') || (ch == '/'));
}

SKILL_TYPE CResource::FindSkillKey(LPCTSTR pszKey) const
{
	ADDTOCALLSTACK("CResource::FindSkillKey");
	// Find the skill name in the sorted list
	// RETURN:
	//  SKILL_NONE = error

	if ( IsDigit(pszKey[0]) )
	{
		SKILL_TYPE skill = static_cast<SKILL_TYPE>(Exp_GetLLVal(pszKey));
		if ( CChar::IsSkillBase(skill) || CChar::IsSkillNPC(skill) )
			return skill;
	}
	else
	{
		const CSkillDef *pSkillDef = FindSkillDef(pszKey);
		if ( pSkillDef )
			return static_cast<SKILL_TYPE>(pSkillDef->GetResourceID().GetResIndex());
	}
	return SKILL_NONE;
}

STAT_TYPE CResource::FindStatKey(LPCTSTR pszKey)	// static
{
	ADDTOCALLSTACK("CResource::FindStatKey");
	return static_cast<STAT_TYPE>(FindTable(pszKey, g_Stat_Name, COUNTOF(g_Stat_Name) - 1));
}

int CResource::GetSpellEffect(SPELL_TYPE spell, int iSkillVal) const
{
	ADDTOCALLSTACK("CResource::GetSpellEffect");
	// NOTE: Any randomizing of the effect must be done by varying the skill level .
	// iSkillVal = 0-1000

	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
	if ( pSpellDef )
		return pSpellDef->m_Effect.GetLinear(iSkillVal);

	return 0;
}

LPCTSTR CResource::GetNotoTitle(int index, bool fFemale) const
{
	ADDTOCALLSTACK("CResource::GetNotoTitle");
	// Retrieve the title used for the given noto level

	if ( (index < 0) || !m_NotoTitles.IsValidIndex(index) )
		return "";

	if ( fFemale )
	{
		LPCTSTR pszFemaleTitle = strchr(m_NotoTitles[index]->GetPtr(), ',');
		if ( pszFemaleTitle )
			return ++pszFemaleTitle;
	}

	return m_NotoTitles[index]->GetPtr();
}

CServerRef CResource::Server_GetDef(size_t index)
{
	ADDTOCALLSTACK("CResource::Server_GetDef");
	if ( !m_Servers.IsValidIndex(index) )
		return NULL;

	return CServerRef(static_cast<CServerDef *>(m_Servers[index]));
}

CWebPageDef *CResource::FindWebPage(LPCTSTR pszPath) const
{
	ADDTOCALLSTACK("CResource::FindWebPage");
	if ( !pszPath )
	{
		if ( m_WebPages.GetCount() <= 0 )
			return NULL;

		return static_cast<CWebPageDef *>(m_WebPages[0]);	// default page
	}

	LPCTSTR pszTitle = CGFile::GetFilesTitle(pszPath);
	if ( !pszTitle || (pszTitle[0] == '\0') )
	{
		if ( m_WebPages.GetCount() <= 0 )
			return NULL;

		return static_cast<CWebPageDef *>(m_WebPages[0]);	// default page
	}

	size_t iMax = m_WebPages.GetCount();
	for ( size_t i = 0; i < iMax; ++i )
	{
		if ( !m_WebPages[i] )
			continue;

		CWebPageDef *pWeb = static_cast<CWebPageDef *>(m_WebPages[i]);
		ASSERT(pWeb);
		if ( pWeb->IsMatch(pszTitle) )
			return pWeb;
	}
	return NULL;
}

bool CResource::IsObscene(LPCTSTR pszText) const
{
	ADDTOCALLSTACK("CResource::IsObscene");
	// Does this text contain obscene word?

	size_t iMax = m_Obscene.GetCount();
	for ( size_t i = 0; i < iMax; ++i )
	{
		TCHAR *pszMatch = new TCHAR[strlen(m_Obscene[i]) + 3];
		sprintf(pszMatch, "%s%s%s", "*", m_Obscene[i], "*");
		MATCH_TYPE iResult = Str_Match(pszMatch, pszText);
		delete[] pszMatch;

		if ( iResult == MATCH_VALID )
			return true;
	}
	return false;
}

bool CResource::SetKRDialogMap(DWORD rid, DWORD dwKRDialogID)
{
	ADDTOCALLSTACK("CResource::SetKRDialogMap");
	// Link ResourceID to an KR DialogID, allowing KR dialogs get handled by scripts

	KRGumpsMap::iterator it = m_mapKRGumps.find(rid);
	if ( it != m_mapKRGumps.end() )
	{
		if ( it->second == dwKRDialogID )	// already linked to this KR dialog
			return true;

		g_Log.Event(LOGL_WARN, "Dialog '%s' is already linked to KR dialog '%" FMTDWORD "'\n", ResourceGetName(RESOURCE_ID(RES_DIALOG, rid)), it->second);
	}

	for ( it = m_mapKRGumps.begin(); it != m_mapKRGumps.end(); ++it )
	{
		if ( it->second != dwKRDialogID )
			continue;

		DEBUG_ERR(("KR Dialog '%" FMTDWORD "' is already linked to dialog '%s'\n", dwKRDialogID, ResourceGetName(RESOURCE_ID(RES_DIALOG, it->first))));
		return false;
	}

	m_mapKRGumps[rid] = dwKRDialogID;
	return true;
}

DWORD CResource::GetKRDialogMap(DWORD dwKRDialogID)
{
	ADDTOCALLSTACK("CResource::GetKRDialogMap");
	// Translates the given KR DialogID into ResourceID of its scripted dialog

	for ( KRGumpsMap::iterator it = m_mapKRGumps.begin(); it != m_mapKRGumps.end(); ++it )
	{
		if ( it->second != dwKRDialogID )
			continue;

		return it->first;
	}
	return 0;
}

DWORD CResource::GetKRDialog(DWORD rid)
{
	ADDTOCALLSTACK("CResource::GetKRDialog");
	// Translates the given ResourceID into it's equivalent KR DialogID.

	KRGumpsMap::iterator it = m_mapKRGumps.find(rid);
	if ( it != m_mapKRGumps.end() )
		return it->second;

	return 0;
}

const CGrayMulti *CResource::GetMultiItemDefs(CItem *pItem)
{
	ADDTOCALLSTACK("CResource::GetMultiItemDefs");
	if ( !pItem )
		return NULL;

	CItemMultiCustom *pItemMultiCustom = dynamic_cast<CItemMultiCustom *>(pItem);
	if ( pItemMultiCustom )
		return pItemMultiCustom->GetMultiItemDefs();	// customized multi

	return GetMultiItemDefs(pItem->GetDispID());		// multi.mul multi
}

const CGrayMulti *CResource::GetMultiItemDefs(ITEMID_TYPE itemid)
{
	ADDTOCALLSTACK("CResource::GetMultiItemDefs(2)");
	if ( !CItemBase::IsID_Multi(itemid) )
		return NULL;

	MULTI_TYPE id = static_cast<MULTI_TYPE>(itemid - ITEMID_MULTI);
	size_t index = m_MultiDefs.FindKey(id);

	if ( index != m_MultiDefs.BadIndex() )
		m_MultiDefs[index]->HitCacheTime();
	else
		index = m_MultiDefs.AddSortKey(new CGrayMulti(id), id);

	return static_cast<const CGrayMulti *>(m_MultiDefs[index]);
}

PLEVEL_TYPE CResource::GetPrivCommandLevel(LPCTSTR pszCmd) const
{
	ADDTOCALLSTACK("CResource::GetPrivCommandLevel");
	// Get required plevel to use given command
	// NOTE: This does not attempt to parse anything

	size_t iPlevel = PLEVEL_QTY;
	while ( --iPlevel > 0 )
	{
		if ( FindTableHeadSorted(pszCmd, m_PrivCommands[iPlevel].GetBasePtr(), m_PrivCommands[iPlevel].GetCount()) >= 0 )
			return static_cast<PLEVEL_TYPE>(iPlevel);
	}
	return m_iDefaultCommandLevel;
}

bool CResource::CanUsePrivVerb(const CScriptObj *pObjTarg, LPCTSTR pszCmd, CTextConsole *pSrc) const
{
	ADDTOCALLSTACK("CResource::CanUsePrivVerb");
	// Can I use this verb on this object?
	// Check just at entry points where commands are entered or targetted
	// NOTE:
	//  Call this each time we change pObjTarg such as r_GetRef()
	// RETURN:
	//  true = I am ok to use this command

	if ( !pSrc || !pObjTarg )
		return false;

	const CChar *pChar = dynamic_cast<const CChar *>(pObjTarg);
	if ( pChar )
	{
		if ( pSrc->GetChar() == pChar )
			return true;

		if ( pSrc->GetPrivLevel() < pChar->GetPrivLevel() )
		{
			pSrc->SysMessageDefault(DEFMSG_MSG_ACC_PRIV);
			return false;
		}
	}

	if ( !pSrc->GetChar() )
	{
		CClient *pClient = dynamic_cast<CClient *>(pSrc);
		if ( pClient && !pClient->m_pAccount )	// must be a console or web page
			return !strnicmp(pszCmd, "LOGIN", 5);
	}

	size_t iSpace = strcspn(pszCmd, " ");	// position of space
	TCHAR *pszMyCmd = Str_GetTemp();
	strncpy(pszMyCmd, pszCmd, iSpace);
	pszMyCmd[iSpace] = '\0';

	TCHAR *pszDot;	// position of dot
	while ( (pszDot = strchr(pszMyCmd, '.')) != NULL )
	{
		if ( pSrc->GetPrivLevel() < GetPrivCommandLevel(pszMyCmd) )
			return false;

		pszMyCmd = pszDot + 1;	// skip dot
	}

	if ( pSrc->GetPrivLevel() < GetPrivCommandLevel(pszMyCmd) )
		return false;

	return true;
}

///////////////////////////////////////////////////////////

CPointMap CResource::GetRegionPoint(LPCTSTR pszCmd) const
{
	ADDTOCALLSTACK("CResource::GetRegionPoint");
	// Get a point from a name (probably the name of a region)
	// Might just be a point coord number?

	GETNONWHITESPACE(pszCmd);
	if ( (pszCmd[0] == '-') && !strchr(pszCmd, ',') )	// Get location from start list.
	{
		size_t i = static_cast<size_t>(abs(ATOI(pszCmd))) - 1;
		if ( !m_StartDefs.IsValidIndex(i) )
		{
			if ( m_StartDefs.GetCount() <= 0 )
				return CPointMap();

			i = 0;
		}
		return m_StartDefs[i]->m_pt;
	}

	CPointMap pt;
	if ( IsDigit(pszCmd[0]) || (pszCmd[0] == '-') )
	{
		TCHAR szTemp[32];
		strncpy(szTemp, pszCmd, sizeof(szTemp) - 1);

		size_t iCount = pt.Read(szTemp);
		if ( iCount >= 2 )
			return pt;
	}
	else
	{
		// Match the region name with global regions
		CRegionBase *pRegion = GetRegion(pszCmd);
		if ( pRegion )
			return pRegion->m_pt;
	}
	return pt;	// no match
}

CRegionBase *CResource::GetRegion(LPCTSTR pszKey) const
{
	ADDTOCALLSTACK("CResource::GetRegion");
	// Get a region from a name or areadef

	GETNONWHITESPACE(pszKey);
	size_t iMax;
	for ( size_t i = 0; i < COUNTOF(m_ResHash.m_Array); ++i )
	{
		iMax = m_ResHash.m_Array[i].GetCount();
		for ( size_t j = 0; j < iMax; ++j )
		{
			CResourceDef *pResDef = m_ResHash.m_Array[i][j];
			ASSERT(pResDef);

			CRegionBase *pRegion = dynamic_cast<CRegionBase *>(pResDef);
			if ( !pRegion )
				continue;

			if ( !pRegion->GetNameStr().CompareNoCase(pszKey) || !strcmpi(pRegion->GetResourceName(), pszKey) )
				return pRegion;
		}
	}
	return NULL;	// no match
}

void CResource::LoadSortSpells()
{
	size_t iMaxSpells = m_SpellDefs.GetCount();
	if ( iMaxSpells <= 0 )
		return;

	m_SpellDefs_Sorted.RemoveAll();
	m_SpellDefs_Sorted.Add(m_SpellDefs[0]);

	for ( size_t i = 1; i < iMaxSpells; ++i )
	{
		if ( !m_SpellDefs.IsValidIndex(i) )
			continue;

		int	iVal1 = 0;
		if ( !m_SpellDefs[i]->GetPrimarySkill(NULL, &iVal1) )
			continue;

		size_t j = 1;
		size_t iMaxSpellsSorted = m_SpellDefs_Sorted.GetCount();
		for ( ; j < iMaxSpellsSorted; ++j )
		{
			int	iVal2 = 0;
			if ( m_SpellDefs_Sorted[j]->GetPrimarySkill(NULL, &iVal2) )
			{
				if ( iVal2 > iVal1 )
					break;
			}
		}
		m_SpellDefs_Sorted.InsertAt(j, m_SpellDefs[i]);
	}
}

///////////////////////////////////////////////////////////

bool CResource::LoadResourceSection(CScript *pScript)
{
	ADDTOCALLSTACK("CResource::LoadResourceSection");
	// Index or read any resource blocks we know how to handle

	ASSERT(pScript);
	CScriptFileContext FileContext(pScript);	// set this as context
	
	LPCTSTR pszSection = pScript->GetSection();
	RES_TYPE restype;
	bool fNewStyleDef = false;

	if ( !strnicmp(pszSection, "DEFMESSAGE", 10) )
	{
		restype = RES_DEFNAME;
		fNewStyleDef = true;
	}
	else if ( !strnicmp(pszSection, "AREADEF", 7) )
	{
		restype = RES_AREA;
		fNewStyleDef = true;
		if ( g_Serv.m_fResyncPause )
			g_Serv.m_fReloadMultis = true;
	}
	else if ( !strnicmp(pszSection, "ROOMDEF", 7) )
	{
		restype = RES_ROOM;
		fNewStyleDef = true;
		if ( g_Serv.m_fResyncPause )
			g_Serv.m_fReloadMultis = true;
	}
	else if ( !strnicmp(pszSection, "GLOBALS", 7) )
		restype = RES_WORLDVARS;
	else if ( !strnicmp(pszSection, "LIST", 4) )
		restype = RES_WORLDLISTS;
	else if ( !strnicmp(pszSection, "TIMERF", 7) )
		restype = RES_TIMERF;
	else if ( !strnicmp(pszSection, "FAME", 7) )
		restype = RES_FAME;
	else if ( !strnicmp(pszSection, "KARMA", 7) )
		restype = RES_KARMA;
	else if ( !strnicmp(pszSection, "MULTIDEF", 8) )
	{
		restype = RES_ITEMDEF;
		fNewStyleDef = true;
	}
	else
		restype = static_cast<RES_TYPE>(FindTableSorted(pszSection, sm_szResourceBlocks, COUNTOF(sm_szResourceBlocks)));

	CVarDefContNum *pVarNum = NULL;
	RESOURCE_ID rid;

	if ( (restype == RES_WORLDSCRIPT) || (restype == RES_WS) )
	{
		LPCTSTR pszDef = pScript->GetArgStr();
		CVarDefCont *pVarBase = g_Exp.m_VarDefs.GetKey(pszDef);
		if ( pVarBase )
			pVarNum = static_cast<CVarDefContNum *>(pVarBase);

		if ( !pVarNum )
		{
			g_Log.Event(LOGL_WARN, "Resource '%s' not found\n", pszDef);
			return false;
		}

		rid.SetPrivateUID(static_cast<DWORD>(pVarNum->GetValNum()));
		restype = rid.GetResType();

		CResourceDef *pRes = NULL;
		size_t index = m_ResHash.FindKey(rid);
		if ( index != m_ResHash.BadIndex() )
			pRes = m_ResHash.GetAt(rid, index);

		if ( !pRes )
		{
			g_Log.Event(LOGL_WARN, "Resource '%s' not found\n", pszDef);
			return false;
		}

		pRes->r_Load(*pScript);
		return true;
	}

	if ( restype < 0 )
	{
		g_Log.Event(LOGL_WARN, "Unknown section '%s' in '%s'\n", pScript->GetKey(), static_cast<LPCTSTR>(pScript->GetFileTitle()));
		return false;
	}
	else
	{
		// Create a new index for the block
		// NOTE: rid is not created for all types, and GetArgStr() is not always the DEFNAME
		rid = ResourceGetNewID(restype, pScript->GetArgStr(), &pVarNum, fNewStyleDef);
	}

	if ( !rid.IsValidUID() )
	{
		DEBUG_ERR(("Invalid %s block index '%s'\n", pszSection, static_cast<LPCTSTR>(pScript->GetArgStr())));
		return false;
	}

	EXC_TRY("LoadResourceSection");
	// It is possible to be replacing an existing entry! Check for this

	if ( m_ResourceList.ContainsKey(const_cast<TCHAR *>(pszSection)) )
	{
		CListDefCont *pListBase = g_Exp.m_ListInternals.GetKey(pszSection);
		if ( !pListBase )
			pListBase = g_Exp.m_ListInternals.AddList(pszSection);

		if ( pListBase )
			pListBase->r_LoadVal(pScript->GetArgStr());
	}

	CResourceLink *pNewLink = NULL;
	CResourceDef *pPrvDef = NULL;
	CResourceDef *pNewDef = NULL;

	switch ( restype )
	{
		case RES_SPHERE:
		{
			g_Serv.r_Load(*pScript);
			return true;
		}
		case RES_SPHERECRYPT:
		{
			CCrypt::LoadKeyTable(*pScript);
			return true;
		}
		case RES_ACCOUNT:
		{
			// Load the account now (not normal method)
			// NOTE: ArgStr is not the DEFNAME
			return g_Accounts.Account_Load(pScript->GetArgStr(), *pScript, false);
		}
		case RES_ADVANCE:
		{
			// Stat advance rates
			while ( pScript->ReadKeyParse() )
			{
				STAT_TYPE iStat = FindStatKey(pScript->GetKey());
				if ( (iStat < STAT_STR) || (iStat >= STAT_BASE_QTY) )
					continue;
				m_StatAdv[iStat].Load(pScript->GetArgStr());
			}
			return true;
		}
		case RES_BLOCKIP:
		{
			TCHAR szIP[16];
			while ( pScript->ReadKeyParse() )
			{
				strncpy(szIP, pScript->GetKey(), sizeof(szIP) - 1);
				HistoryIP &history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(szIP);
				history.setBlocked(true);
			}
			return true;
		}
		case RES_COMMENT:
		{
			// Just skip it
			return true;
		}
		case RES_DEFNAME:
		{
			// Get defname block
			while ( pScript->ReadKeyParse() )
			{
				LPCTSTR pszKey = pScript->GetKey();
				if ( fNewStyleDef )
				{
					// Search for this
					size_t i;
					for ( i = 0; i < DEFMSG_QTY; ++i )
					{
						if ( !strcmpi(pszKey, g_Exp.sm_szMsgNames[i]) )
						{
							strncpy(g_Exp.sm_szMessages[i], pScript->GetArgStr(), EXPRESSION_MAX_KEY_LEN - 1);
							break;
						}
					}
					if ( i == DEFMSG_QTY )
						g_Log.Event(LOGL_ERROR, "Unknown message '%s'\n", pszKey);
					continue;
				}
				else
					g_Exp.m_VarDefs.SetStr(pszKey, false, pScript->GetArgStr());
			}
			return true;
		}
		case RES_RESOURCELIST:
		{
			while ( pScript->ReadKey() )
				m_ResourceList.AddSortString(static_cast<LPCTSTR>(pScript->GetKeyBuffer()));
			return true;
		}
		case RES_FAME:
		{
			size_t i = 0;
			while ( pScript->ReadKey() )
			{
				LPCTSTR pszName = pScript->GetKeyBuffer();
				if ( *pszName == '<' )
					pszName = "";

				m_Fame.SetAtGrow(i, new CGString(pszName));
				++i;
			}
			return true;
		}
		case RES_KARMA:
		{
			size_t i = 0;
			while ( pScript->ReadKey() )
			{
				LPCTSTR pszName = pScript->GetKeyBuffer();
				if ( *pszName == '<' )
					pszName = "";

				m_Karma.SetAtGrow(i, new CGString(pszName));
				++i;
			}
			return true;
		}
		case RES_NOTOTITLES:
		{
			if ( !pScript->ReadKey() )
			{
				g_Log.Event(LOGL_ERROR, "NOTOTITLES section is missing the list of karma levels\n");
				return false;
			}

			// Read karma levels
			INT64 piNotoLevels[64];
			size_t iArgQty = Str_ParseCmds(pScript->GetKeyBuffer(), piNotoLevels, COUNTOF(piNotoLevels));

			size_t i = 0;
			for ( i = 0; i < iArgQty; ++i )
				m_NotoKarmaLevels.SetAtGrow(i, static_cast<int>(piNotoLevels[i]));

			m_NotoKarmaLevels.SetCount(i);

			if ( !pScript->ReadKey() )
			{
				g_Log.Event(LOGL_ERROR, "NOTOTITLES section is missing the list of fame levels\n");
				return false;
			}

			// Read fame levels
			iArgQty = Str_ParseCmds(pScript->GetKeyBuffer(), piNotoLevels, COUNTOF(piNotoLevels));
			for ( i = 0; i < iArgQty; ++i )
				m_NotoFameLevels.SetAtGrow(i, static_cast<int>(piNotoLevels[i]));

			m_NotoFameLevels.SetCount(i);

			// Read noto titles
			i = 0;
			while ( pScript->ReadKey() )
			{
				LPCTSTR pszName = pScript->GetKeyBuffer();
				if ( *pszName == '<' )
					pszName = "";

				m_NotoTitles.SetAtGrow(i, new CGString(pszName));
				++i;
			}

			if ( m_NotoTitles.GetCount() != ((m_NotoKarmaLevels.GetCount() + 1) * (m_NotoFameLevels.GetCount() + 1)) )
				g_Log.Event(LOGL_WARN, "Expected %" FMTSIZE_T " titles in NOTOTITLES section but found %" FMTSIZE_T "\n", (m_NotoKarmaLevels.GetCount() + 1) * (m_NotoFameLevels.GetCount() + 1), m_NotoTitles.GetCount());
			return true;
		}
		case RES_OBSCENE:
		{
			while ( pScript->ReadKey() )
				m_Obscene.AddSortString(pScript->GetKey());
			return true;
		}
		case RES_PLEVEL:
		{
			int iPlevel = rid.GetResIndex();
			if ( (iPlevel < PLEVEL_Player) || (iPlevel >= PLEVEL_QTY) )
				return false;
			while ( pScript->ReadKey() )
				m_PrivCommands[iPlevel].AddSortString(pScript->GetKey());
			return true;
		}
		case RES_RESOURCES:
		{
			// Add these all to the list of files we need to include
			while ( pScript->ReadKey() )
				AddResourceFile(pScript->GetKey());
			return true;
		}
		case RES_RUNES:
		{
			// The full names of the magic runes
			m_Runes.RemoveAll();
			while ( pScript->ReadKey() )
				m_Runes.Add(new CGString(pScript->GetKey()));
			return true;
		}
		case RES_SECTOR:
		{
			CPointMap pt = GetRegionPoint(pScript->GetArgStr());
			return pt.GetSector()->r_Load(*pScript);
		}
		case RES_SPELL:
		{
			CSpellDef *pSpell;
			pPrvDef = ResourceGetDef(rid);
			if ( pPrvDef )
			{
				pSpell = dynamic_cast<CSpellDef *>(pPrvDef);
				if ( !pSpell )
					return false;
			}
			else
				pSpell = new CSpellDef(static_cast<SPELL_TYPE>(rid.GetResIndex()));

			ASSERT(pSpell);
			pNewLink = pSpell;

			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load(*pScript);
			pScript->SeekContext(LineContext);

			if ( !pPrvDef )
				m_SpellDefs.SetAtGrow(rid.GetResIndex(), pSpell);
			break;
		}
		case RES_SKILL:
		{
			CSkillDef *pSkill;
			pPrvDef = ResourceGetDef(rid);
			if ( pPrvDef )
			{
				pSkill = dynamic_cast<CSkillDef *>(pPrvDef);
				if ( !pSkill )
					return false;
			}
			else
			{
				if ( rid.GetResIndex() >= static_cast<int>(g_Cfg.m_iMaxSkill) )
					g_Cfg.m_iMaxSkill = rid.GetResIndex() + 1;

				pSkill = new CSkillDef(static_cast<SKILL_TYPE>(rid.GetResIndex()));
			}

			ASSERT(pSkill);
			pNewLink = pSkill;

			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load(*pScript);
			pScript->SeekContext(LineContext);

			if ( !pPrvDef )
			{
				m_SkillNameDefs.AddSortKey(pSkill, pSkill->GetKey());		// build a name sorted list
				m_SkillIndexDefs.SetAtGrow(rid.GetResIndex(), pSkill);		// hardcoded value for skill index
			}
			break;
		}
		case RES_TYPEDEF:
		{
			// Just index this for access later
			pPrvDef = ResourceGetDef(rid);
			if ( pPrvDef )
			{
				CItemTypeDef *pTypeDef = dynamic_cast<CItemTypeDef *>(pPrvDef);
				if ( !pTypeDef )
					return false;

				pNewLink = pTypeDef;
				ASSERT(pNewLink);

				// Clear old tile links to this type
				size_t iMax = g_World.m_TileTypes.GetCount();
				for ( size_t i = 0; i < iMax; ++i )
				{
					if ( g_World.m_TileTypes.GetAt(i) == pTypeDef )
						g_World.m_TileTypes.SetAt(i, NULL);
				}

			}
			else
			{
				pNewLink = new CItemTypeDef(rid);
				ASSERT(pNewLink);
				m_ResHash.AddSortKey(rid, pNewLink);
			}

			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load(*pScript);
			pScript->SeekContext(LineContext);
			break;
		}

		// Might need to check if the link already exists?

		case RES_BOOK:
		case RES_EVENTS:
		case RES_MENU:
		case RES_NAMES:
		case RES_NEWBIE:
		case RES_TIP:
		case RES_SPEECH:
		case RES_SCROLL:
		case RES_SKILLMENU:
		{
			// Just index this for access later
			pPrvDef = ResourceGetDef(rid);
			if ( pPrvDef )
			{
				pNewLink = dynamic_cast<CResourceLink *>(pPrvDef);
				ASSERT(pNewLink);
			}
			else
			{
				pNewLink = new CResourceLink(rid);
				ASSERT(pNewLink);
				m_ResHash.AddSortKey(rid, pNewLink);
			}
			break;
		}
		case RES_DIALOG:
		{
			// Just index this for access later
			pPrvDef = ResourceGetDef(rid);
			if ( pPrvDef )
			{
				pNewLink = dynamic_cast<CDialogDef *>(pPrvDef);
				ASSERT(pNewLink);
			}
			else
			{
				pNewLink = new CDialogDef(rid);
				ASSERT(pNewLink);
				m_ResHash.AddSortKey(rid, pNewLink);
			}
			break;
		}
		case RES_REGIONRESOURCE:
		{
			// No need to link to this really
			pPrvDef = ResourceGetDef(rid);
			if ( pPrvDef )
			{
				pNewLink = dynamic_cast<CRegionResourceDef *>(pPrvDef);
				if ( !pNewLink )
					return false;
			}
			else
			{
				pNewLink = new CRegionResourceDef(rid);
				ASSERT(pNewLink);
				m_ResHash.AddSortKey(rid, pNewLink);
			}
			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load(*pScript);
			pScript->SeekContext(LineContext);		// set the pos back so ScanSection will work
			break;
		}
		case RES_AREA:
		{
			pPrvDef = ResourceGetDef(rid);
			if ( pPrvDef && fNewStyleDef )
			{
				CRegionWorld *pRegion = dynamic_cast<CRegionWorld *>(pPrvDef);
				if ( !pRegion )
					return false;
				pNewDef = pRegion;
				pRegion->UnRealizeRegion();
				pRegion->r_Load(*pScript);
				pRegion->RealizeRegion();
			}
			else
			{
				CRegionWorld *pRegion = new CRegionWorld(rid, pScript->GetArgStr());
				pRegion->r_Load(*pScript);
				if ( !pRegion->RealizeRegion() )
					delete pRegion;		// might be a dupe?
				else
				{
					pNewDef = pRegion;
					ASSERT(pNewDef);
					m_ResHash.AddSortKey(rid, pRegion);
					//if ( !fNewStyleDef )		// if it's old style but has a defname, it's already set via r_Load, so this will do nothing, which is good
					//	pRegion->MakeRegionName();
					m_RegionDefs.Add(pRegion);
				}
			}
			break;
		}
		case RES_ROOM:
		{
			pPrvDef = ResourceGetDef(rid);
			if ( pPrvDef && fNewStyleDef )
			{
				CRegionBase *pRegion = dynamic_cast<CRegionBase *>(pPrvDef);
				if ( !pRegion )
					return false;
				pNewDef = pRegion;
				pRegion->UnRealizeRegion();
				pRegion->r_Load(*pScript);
				pRegion->RealizeRegion();
			}
			else
			{
				CRegionBase *pRegion = new CRegionBase(rid, pScript->GetArgStr());
				pRegion->r_Load(*pScript);
				if ( !pRegion->RealizeRegion() )
					delete pRegion;		// might be a dupe?
				else
				{
					pNewDef = pRegion;
					ASSERT(pNewDef);
					m_ResHash.AddSortKey(rid, pRegion);
					//if ( !fNewStyleDef )		// if it's old style but has a defname, it's already set via r_Load, so this will do nothing, which is good
					//	pRegion->MakeRegionName();
					m_RegionDefs.Add(pRegion);
				}
			}
			break;
		}
		case RES_REGIONTYPE:
		case RES_SPAWN:
		case RES_TEMPLATE:
		{
			pPrvDef = ResourceGetDef(rid);
			if ( pPrvDef )
			{
				pNewLink = dynamic_cast<CRandGroupDef *>(pPrvDef);
				if ( !pNewLink )
					return false;
			}
			else
			{
				pNewLink = new CRandGroupDef(rid);
				ASSERT(pNewLink);
				m_ResHash.AddSortKey(rid, pNewLink);
			}
			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load(*pScript);
			pScript->SeekContext(LineContext);
			break;
		}
		case RES_SKILLCLASS:
		{
			pPrvDef = ResourceGetDef(rid);
			if ( pPrvDef )
			{
				pNewLink = dynamic_cast<CSkillClassDef *>(pPrvDef);
				if ( !pNewLink )
					return false;
			}
			else
			{
				pNewLink = new CSkillClassDef(rid);
				ASSERT(pNewLink);
				m_ResHash.AddSortKey(rid, pNewLink);
			}
			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load(*pScript);
			pScript->SeekContext(LineContext);
			break;
		}
		case RES_CHARDEF:
		case RES_ITEMDEF:
		{
			// Existing hard pointers to RES_CHARDEF/RES_ITEMDEF?
			pPrvDef = ResourceGetDef(rid);
			if ( pPrvDef )
			{
				pNewLink = dynamic_cast<CResourceLink *>(pPrvDef);
				if ( !pNewLink )
					return false;

				CBaseBaseDef *pBaseDef = dynamic_cast<CBaseBaseDef *>(pNewLink);
				if ( pBaseDef )
				{
					pBaseDef->UnLink();
					CScriptLineContext LineContext = pScript->GetContext();
					pBaseDef->r_Load(*pScript);
					pScript->SeekContext(LineContext);
				}
			}
			else
			{
				pNewLink = new CResourceLink(rid);
				ASSERT(pNewLink);
				m_ResHash.AddSortKey(rid, pNewLink);
			}
			break;
		}
		case RES_WEBPAGE:
		{
			// Read a web page entry
			pPrvDef = ResourceGetDef(rid);
			if ( pPrvDef )
			{
				pNewLink = dynamic_cast<CWebPageDef *>(pPrvDef);
				if ( !pNewLink )
					return false;
			}
			else
			{
				pNewLink = new CWebPageDef(rid);
				ASSERT(pNewLink);
				m_WebPages.AddSortKey(pNewLink, rid);
			}
			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load(*pScript);
			pScript->SeekContext(LineContext);		// set the pos back so ScanSection will work
			break;
		}
		case RES_FUNCTION:
		{
			pNewLink = new CResourceNamed(rid, pScript->GetArgStr());
			m_Functions.AddSortKey(pNewLink, pNewLink->GetName());
			break;
		}
		case RES_SERVERS:
		{
			// Old way to define a block of servers
			while ( pScript->ReadKey() )
			{
				CServerRef pServ;
				bool fAddNew = false;

				size_t i = m_Servers.FindKey(pScript->GetKey());
				if ( i == m_Servers.BadIndex() )
				{
					pServ = new CServerDef(pScript->GetKey(), CSocketAddressIP(SOCKET_LOCAL_ADDRESS));
					fAddNew = true;
				}
				else
					pServ = Server_GetDef(i);

				ASSERT(pServ);
				if ( pScript->ReadKey() )
				{
					pServ->m_ip.SetHostPortStr(pScript->GetKey());
					if ( pScript->ReadKey() )
						pServ->m_ip.SetPort(static_cast<WORD>(pScript->GetArgVal()));
				}

				if ( !strcmpi(pServ->GetName(), g_Serv.GetName()) || (g_Serv.m_ip == pServ->m_ip) )
				{
					// I can be listed first here (old way)
					g_Serv.SetName(pServ->GetName());
					g_Serv.m_ip = pServ->m_ip;
					delete pServ;
					continue;
				}

				if ( fAddNew )
					m_Servers.AddSortKey(pServ, pServ->GetName());
			}
			return true;
		}
		case RES_TYPEDEFS:
		{
			// Just get a block of defs
			while ( pScript->ReadKeyParse() )
			{
				RESOURCE_ID ridNew(RES_TYPEDEF, pScript->GetArgVal());
				pPrvDef = ResourceGetDef(ridNew);
				if ( pPrvDef )
					pPrvDef->SetResourceName(pScript->GetKey());
				else
				{
					CResourceDef *pResDef = new CItemTypeDef(ridNew);
					pResDef->SetResourceName(pScript->GetKey());
					ASSERT(pResDef);
					m_ResHash.AddSortKey(ridNew, pResDef);
				}
			}
			return true;
		}
		case RES_STARTS:
		{
			m_StartDefs.RemoveAll();
			while ( pScript->ReadKey() )
			{
				CStartLoc *pStartLoc = new CStartLoc(pScript->GetKey());
				if ( pScript->ReadKey() )
					pStartLoc->m_sName = pScript->GetKey();
				if ( pScript->ReadKey() )
					pStartLoc->m_pt.Read(pScript->GetKeyBuffer());
				if ( pScript->ReadKey() )
					pStartLoc->iClilocDescription = ATOI(pScript->GetKey());

				m_StartDefs.Add(pStartLoc);
			}
			return true;
		}
		case RES_MOONGATES:
		{
			m_MoonGates.RemoveAll();
			while ( pScript->ReadKey() )
			{
				CPointMap pt = GetRegionPoint(pScript->GetKey());
				m_MoonGates.Add(pt);
			}
			return true;
		}
		case RES_WORLDVARS:
		{
			while ( pScript->ReadKeyParse() )
			{
				bool fQuoted = false;
				LPCTSTR pszKey = pScript->GetKey();
				if ( strstr(pszKey, "VAR.") )	// this is for backward compatibility from RCs
					pszKey = pszKey + 4;

				g_Exp.m_VarGlobals.SetStr(pszKey, fQuoted, pScript->GetArgStr(&fQuoted));
			}
			return true;
		}
		case RES_WORLDLISTS:
		{
			CListDefCont *pListBase = g_Exp.m_ListGlobals.AddList(pScript->GetArgStr());
			if ( !pListBase )
			{
				DEBUG_ERR(("Unable to create list '%s'\n", pScript->GetArgStr()));
				return false;
			}

			while ( pScript->ReadKeyParse() )
			{
				if ( !pListBase->r_LoadVal(*pScript) )
					DEBUG_ERR(("Unable to add element '%s' to list '%s'\n", pScript->GetArgStr(), pListBase->GetKey()));
			}
			return true;
		}
		case RES_TIMERF:
		{
			while ( pScript->ReadKeyParse() )
			{
				bool fQuoted = false;
				g_World.m_TimedFunctions.Load(pScript->GetKey(), fQuoted, pScript->GetArgStr(&fQuoted));
			}
			return true;
		}
		case RES_TELEPORTERS:
		{
			// Add the teleporter to CSector
			while ( pScript->ReadKey() )
			{
				CTeleport *pTeleport = new CTeleport(pScript->GetKeyBuffer());
				ASSERT(pTeleport);
				if ( !pTeleport->RealizeTeleport() )
					delete pTeleport;
			}
			return true;
		}
		case RES_KRDIALOGLIST:
		{
			while ( pScript->ReadKeyParse() )
			{
				CDialogDef *pDialogDef = dynamic_cast<CDialogDef *>(g_Cfg.ResourceGetDefByName(RES_DIALOG, pScript->GetKey()));
				if ( pDialogDef )
					g_Cfg.SetKRDialogMap(static_cast<DWORD>(pDialogDef->GetResourceID()), static_cast<DWORD>(pScript->GetArgVal()));
				else
					DEBUG_ERR(("KR dialog '%s' not found\n", pScript->GetKey()));
			}
			return true;
		}

		// Saved in world file

		case RES_GMPAGE:
		{
			CGMPage *pGMPage = new CGMPage(pScript->GetArgStr());
			return pGMPage->r_Load(*pScript);
		}
		case RES_WC:
		case RES_WORLDCHAR:
		{
			if ( !rid.IsValidUID() )
			{
				g_Log.Event(LOGL_ERROR, "Undefined char type '%s'\n", static_cast<LPCTSTR>(pScript->GetArgStr()));
				return false;
			}
			return CChar::CreateBasic(static_cast<CREID_TYPE>(rid.GetResIndex()))->r_Load(*pScript);
		}
		case RES_WI:
		case RES_WORLDITEM:
		{
			if ( !rid.IsValidUID() )
			{
				g_Log.Event(LOGL_ERROR, "Undefined item type '%s'\n", static_cast<LPCTSTR>(pScript->GetArgStr()));
				return false;
			}
			return CItem::CreateBase(static_cast<ITEMID_TYPE>(rid.GetResIndex()))->r_Load(*pScript);
		}
		default:
			break;
	}

	if ( pNewLink )
	{
		pNewLink->SetResourceVar(pVarNum);

		CResourceScript *pResScript = dynamic_cast<CResourceScript *>(pScript);
		if ( !pResScript )
		{
			DEBUG_ERR(("Can't link resources in the world save file\n"));
			return false;
		}

		// Now scan it for DEFNAME= or DEFNAME2= stuff
		pNewLink->SetLink(pResScript);
		pNewLink->ScanSection(restype);
	}
	else if ( pNewDef && pVarNum )
	{
		// Not linked but still may have a var name
		pNewDef->SetResourceVar(pVarNum);
	}
	return true;

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("section '%s' args '%s'\n", pszSection, pScript->GetArgStr());
	EXC_DEBUG_END;
	return false;
}

///////////////////////////////////////////////////////////

RESOURCE_ID CResource::ResourceGetNewID(RES_TYPE restype, LPCTSTR pszName, CVarDefContNum **ppVarNum, bool fNewStyleDef)
{
	ADDTOCALLSTACK("CResource::ResourceGetNewID");
	// We are reading in a script block.
	// We may be creating a new id or replacing an old one.
	// ARGS:
	//  restype = the data type we are reading in.
	//  pszName = MAy or may not be the DEFNAME depending on type.

	ASSERT(pszName);
	ASSERT(ppVarNum);

	RESOURCE_ID rid;
	const RESOURCE_ID ridInvalid;	// Linux wants this for some reason
	int iPage = 0;	// sub page

	// Some types don't use named indexes at all (single instance)
	switch ( restype )
	{
		case RES_UNKNOWN:
		{
			return ridInvalid;
		}
		case RES_ADVANCE:
		case RES_BLOCKIP:
		case RES_COMMENT:
		case RES_DEFNAME:
		case RES_KRDIALOGLIST:
		case RES_MOONGATES:
		case RES_NOTOTITLES:
		case RES_OBSCENE:
		case RES_RESOURCES:
		case RES_RUNES:
		case RES_SERVERS:
		case RES_SPHERE:
		case RES_STARTS:
		case RES_TELEPORTERS:
		case RES_TYPEDEFS:
		case RES_WORLDVARS:
		case RES_WORLDLISTS:
		{
			// Single instance stuff (fully read in)
			// Ignore any resource name
			return RESOURCE_ID(restype);
		}
		case RES_ACCOUNT:
		case RES_FUNCTION:
		case RES_GMPAGE:
		case RES_SECTOR:
		{
			// Multiple instance stuff, but name is not a RESOURCE_ID
			// Must have a resource name but do not use true RESOURCE_ID format
			if ( pszName[0] == '\0' )
				return ridInvalid;

			return RESOURCE_ID(restype);
		}
		case RES_BOOK:
		case RES_DIALOG:
		case RES_REGIONTYPE:
		{
			if ( pszName[0] == '\0' )
				return ridInvalid;

			TCHAR *pszArg1 = Str_GetTemp();
			strncpy(pszArg1, pszName, SCRIPT_MAX_LINE_LEN - 1);
			pszName = pszArg1;

			TCHAR *pszArg2;
			Str_Parse(pszArg1, &pszArg2);

			if ( !strcmpi(pszArg2, "TEXT") )
				iPage = RES_DIALOG_TEXT;
			else if ( !strcmpi(pszArg2, "BUTTON") )
				iPage = RES_DIALOG_BUTTON;
			else
			{
				iPage = RES_GET_INDEX(Exp_GetVal(pszArg2));
				if ( iPage > 255 )
					DEBUG_ERR(("Bad resource index page %d\n", iPage));
			}
			break;
		}
		case RES_NEWBIE:
		{
			if ( pszName[0] == '\0' )
				return ridInvalid;

			TCHAR *pszArg1 = Str_GetTemp();
			strncpy(pszArg1, pszName, SCRIPT_MAX_LINE_LEN - 1);
			pszName = pszArg1;

			TCHAR *pszArg2;
			Str_Parse(pszArg1, &pszArg2);

			if ( !strcmpi(pszArg2, "GARG") )
				iPage = RACETYPE_GARGOYLE;
			else if ( !strcmpi(pszArg2, "ELF") )
				iPage = RACETYPE_ELF;
			else
				iPage = RACETYPE_HUMAN;

			if ( !strcmpi(pszName, "MALE_DEFAULT") )
				return RESOURCE_ID(RES_NEWBIE, RES_NEWBIE_MALE_DEFAULT, iPage);
			else if ( !strcmpi(pszName, "FEMALE_DEFAULT") )
				return RESOURCE_ID(RES_NEWBIE, RES_NEWBIE_FEMALE_DEFAULT, iPage);

			if ( !strcmpi(pszName, "PROFESSION_ADVANCED") )
				return RESOURCE_ID(RES_NEWBIE, RES_NEWBIE_PROF_ADVANCED, iPage);
			else if ( !strcmpi(pszName, "PROFESSION_WARRIOR") )
				return RESOURCE_ID(RES_NEWBIE, RES_NEWBIE_PROF_WARRIOR, iPage);
			else if ( !strcmpi(pszName, "PROFESSION_MAGE") )
				return RESOURCE_ID(RES_NEWBIE, RES_NEWBIE_PROF_MAGE, iPage);
			else if ( !strcmpi(pszName, "PROFESSION_BLACKSMITH") )
				return RESOURCE_ID(RES_NEWBIE, RES_NEWBIE_PROF_BLACKSMITH, iPage);
			else if ( !strcmpi(pszName, "PROFESSION_NECROMANCER") )
				return RESOURCE_ID(RES_NEWBIE, RES_NEWBIE_PROF_NECROMANCER, iPage);
			else if ( !strcmpi(pszName, "PROFESSION_PALADIN") )
				return RESOURCE_ID(RES_NEWBIE, RES_NEWBIE_PROF_PALADIN, iPage);
			else if ( !strcmpi(pszName, "PROFESSION_SAMURAI") )
				return RESOURCE_ID(RES_NEWBIE, RES_NEWBIE_PROF_SAMURAI, iPage);
			else if ( !strcmpi(pszName, "PROFESSION_NINJA") )
				return RESOURCE_ID(RES_NEWBIE, RES_NEWBIE_PROF_NINJA, iPage);
			break;
		}
		case RES_AREA:
		case RES_ROOM:
		{
			if ( !fNewStyleDef )
			{
				// Name is not the defname or id, just find a free id
				pszName = NULL;
				break;
			}
			// Otherwise, passthrough to default
		}
		default:
		{
			// The name is a DEFNAME or id number
			ASSERT(restype < RES_QTY);
			break;
		}
	}

	int index;
	if ( pszName )
	{
		if ( pszName[0] == '\0' )
			return RESOURCE_ID(restype, 0, iPage);		// this might be ok

		if ( IsDigit(pszName[0]) )
		{
			index = Exp_GetVal(pszName);
			rid = RESOURCE_ID(restype, index);
			switch ( restype )
			{
				case RES_BOOK:
				case RES_DIALOG:
				case RES_REGIONTYPE:
					rid = RESOURCE_ID(restype, index, iPage);
					break;
				case RES_SKILLMENU:
				case RES_MENU:
				case RES_EVENTS:
				case RES_SPEECH:
				case RES_NAMES:
				case RES_SCROLL:
				case RES_TIP:
				case RES_TYPEDEF:
				case RES_CHARDEF:
				case RES_TEMPLATE:
					break;
				case RES_ITEMDEF:
					if ( fNewStyleDef )		// this is a multi and should have appropriate offset
						rid = RESOURCE_ID(restype, ITEMID_MULTI|index);
					break;
				default:
					return rid;
			}
#ifdef _DEBUG
			if ( g_Serv.m_iModeCode != SERVMODE_ResyncLoad )
			{
				// Warn of duplicates
				size_t iDuplicateIndex = m_ResHash.FindKey(rid);
				if ( iDuplicateIndex != m_ResHash.BadIndex() )
					ASSERT(m_ResHash.GetAt(rid, iDuplicateIndex));
			}
#endif
			return rid;
		}

		CVarDefCont *pVarBase = g_Exp.m_VarDefs.GetKey(pszName);
		if ( pVarBase )
		{
			// An existing VarDef with the same name?
			// Creating a new block but using an old name? weird
			// Just check to see if this is a strange type conflict
			CVarDefContNum *pVarNum = dynamic_cast<CVarDefContNum *>(pVarBase);
			if ( !pVarNum )
			{
				switch ( restype )
				{
					case RES_WC:
					case RES_WI:
					case RES_WS:
					case RES_WORLDCHAR:
					case RES_WORLDITEM:
					case RES_WORLDSCRIPT:
					{
						const CVarDefContStr *pVarStr = dynamic_cast<CVarDefContStr *>(pVarBase);
						if ( pVarStr )
							return ResourceGetNewID(restype, pVarStr->GetValStr(), ppVarNum, fNewStyleDef);
						// fall through
					}
					default:
					{
						DEBUG_ERR(("Re-using resource '%s' to define block\n", pszName));
						return ridInvalid;
					}
				}
			}

			rid.SetPrivateUID(static_cast<DWORD>(pVarNum->GetValNum()));
			if ( restype != rid.GetResType() )
			{
				switch ( restype )
				{
					case RES_WC:
					case RES_WI:
					case RES_WORLDCHAR:
					case RES_WORLDITEM:
					case RES_NEWBIE:
					case RES_PLEVEL:
					{
						// These are not truly defining a new DEFNAME
						break;
					}
					default:
					{
						DEBUG_ERR(("Can't change resource '%s' from %s to %s\n", pszName, GetResourceBlockName(rid.GetResType()), GetResourceBlockName(restype)));
						return ridInvalid;
					}
				}
			}
			else if ( fNewStyleDef && (static_cast<DWORD>(pVarNum->GetValNum()) != rid.GetPrivateUID()) )
			{
				g_pLog->EventWarn("Redefining DEFNAME '%s' for another region\n", pszName);
			}
			else if ( iPage == rid.GetResPage() )	// books and dialogs have pages
			{
				// We are redefining an item we have already read in? Why do this unless it's a resync?
				if ( g_Serv.m_iModeCode != SERVMODE_ResyncLoad )
				{
					if ( restype != RES_TYPEDEF )
						g_pLog->EventWarn("Redefining resource '%s'\n", pszName);
				}
			}
			rid = RESOURCE_ID(restype, rid.GetResIndex(), iPage);
			*ppVarNum = pVarNum;
			return rid;
		}
	}

	// Must define this as a new unique entry
	// Find a new free entry

	int iHashRange = 0;
	switch ( restype )
	{
		case RES_SKILL:
		case RES_SPELL:
		case RES_WC:
		case RES_WI:
		case RES_WORLDCHAR:
		case RES_WORLDITEM:
		case RES_NEWBIE:
		case RES_PLEVEL:
		{
			// Don't allow new named indexes for these resource types
			return ridInvalid;
		}
		case RES_CHARDEF:
		{
			iHashRange = 2000;
			index = NPCID_SCRIPT|0x2000;	// add another offset to avoid Sphere ranges
			break;
		}
		case RES_ITEMDEF:
		{
			iHashRange = 2000;
			index = ITEMID_SCRIPT|0x4000;	// add another offset to avoid Sphere ranges
			break;
		}
		case RES_TEMPLATE:
		{
			iHashRange = 2000;
			index = ITEMID_TEMPLATE|100000;
			break;
		}
		case RES_BOOK:
		case RES_DIALOG:
		{
			if ( iPage )	// must define the main section first
				return ridInvalid;
			// fall through
		}
		case RES_REGIONTYPE:
		{
			iHashRange = 100;
			index = 1000;
			break;
		}
		case RES_AREA:
		{
			iHashRange = 1000;
			index = 10000;
			break;
		}
		case RES_ROOM:
		case RES_SKILLMENU:
		case RES_MENU:
		case RES_EVENTS:
		case RES_SPEECH:
		case RES_NAMES:
		case RES_SCROLL:
		case RES_TIP:
		case RES_TYPEDEF:
		case RES_SKILLCLASS:
		case RES_REGIONRESOURCE:
		{
			iHashRange = 1000;
			index = 10000;
			break;
		}
		case RES_SPAWN:
		{
			iHashRange = 1000;
			index = SPAWNTYPE_START|100000;
			break;
		}
		case RES_WEBPAGE:
		{
			index = m_WebPages.GetCount() + 1;
			break;
		}
		default:
		{
			ASSERT(0);
			return ridInvalid;
		}
	}

	rid = RESOURCE_ID(restype, index, iPage);

	if ( iHashRange )
	{
		// Find a new free entry starting here
		rid.SetPrivateUID(rid.GetPrivateUID() + Calc_GetRandVal(iHashRange));
		for (;;)
		{
			if ( m_ResHash.FindKey(rid) == m_ResHash.BadIndex() )
				break;
			rid.SetPrivateUID(rid.GetPrivateUID() + 1);
		}
	}
	else
	{
		// Find a new free entry starting here
		if ( !index )
			rid.SetPrivateUID(rid.GetPrivateUID() + 1);
	}

	if ( pszName )
	{
		CVarDefContNum *pVarNum = g_Exp.m_VarDefs.SetNum(pszName, rid.GetPrivateUID());
		if ( pVarNum )
			*ppVarNum = pVarNum;
	}

	return rid;
}

CResourceDef *CResource::ResourceGetDef(RESOURCE_ID_BASE rid) const
{
	ADDTOCALLSTACK("CResource::ResourceGetDef");
	// Get a CResourceDef from the given RESOURCE_ID
	// ARGS:
	//  rid = id must be this type

	if ( !rid.IsValidUID() )
		return NULL;

	size_t index = rid.GetResIndex();
	switch ( rid.GetResType() )
	{
		case RES_WEBPAGE:
			index = m_WebPages.FindKey(rid);
			return m_WebPages.IsValidIndex(index) ? m_WebPages.GetAt(index) : NULL;
		case RES_SKILL:
			return m_SkillIndexDefs.IsValidIndex(index) ? m_SkillIndexDefs[index] : NULL;
		case RES_SPELL:
			return m_SpellDefs.IsValidIndex(index) ? m_SpellDefs[index] : NULL;
		case RES_UNKNOWN:
			return NULL;
		case RES_BOOK:
		case RES_EVENTS:
		case RES_DIALOG:
		case RES_MENU:
		case RES_NAMES:
		case RES_NEWBIE:
		case RES_REGIONRESOURCE:
		case RES_REGIONTYPE:
		case RES_SCROLL:
		case RES_SPEECH:
		case RES_TIP:
		case RES_TYPEDEF:
		case RES_TEMPLATE:
		case RES_SKILLMENU:
		case RES_ITEMDEF:
		case RES_CHARDEF:
		case RES_SPAWN:
		case RES_SKILLCLASS:
		case RES_AREA:
		case RES_ROOM:
			return CResourceBase::ResourceGetDef(rid);
		default:
			return NULL;
	}
}

///////////////////////////////////////////////////////////

void CResource::OnTick(bool fNow)
{
	ADDTOCALLSTACK("CResource::OnTick");
	// Give a tick to the less critical stuff

	if ( !fNow && (g_Serv.IsLoading() || (m_timePeriodic > CServTime::GetCurrentTime())) )
		return;

	if ( m_fUseHTTP )
	{
		// Update WEBPAGES resources
		size_t iMax = m_WebPages.GetCount();
		for ( size_t i = 0; i < iMax; ++i )
		{
			EXC_TRY("WebTick");
			if ( !m_WebPages[i] )
				continue;
			CWebPageDef *pWeb = static_cast<CWebPageDef *>(m_WebPages[i]);
			if ( pWeb )
			{
				pWeb->WebPageUpdate(fNow, NULL, &g_Serv);
				pWeb->WebPageLog();
			}
			EXC_CATCH;

			EXC_DEBUG_START;
			CWebPageDef *pWeb = static_cast<CWebPageDef *>(m_WebPages[i]);
			g_Log.EventDebug("web '%s' dest '%s' now '%d' index '%" FMTSIZE_T "'/'%" FMTSIZE_T "'\n", pWeb ? pWeb->GetName() : "", pWeb ? pWeb->GetDstName() : "", fNow ? 1 : 0, i, m_WebPages.GetCount());
			EXC_DEBUG_END;
		}
	}

	m_timePeriodic = CServTime::GetCurrentTime() + (60 * TICK_PER_SEC);
}

#define catresname(a, b)	\
{							\
	if ( *(a) )				\
		strcat(a, " + ");	\
	strcat(a, b);			\
}

bool CResource::LoadIni()
{
	ADDTOCALLSTACK("CResource::LoadIni");

	if ( OpenResourceFind(m_scpCryptIni, SPHERE_FILE "Crypt.ini", false) )
	{
		LoadResourcesOpen(&m_scpCryptIni);
		m_scpCryptIni.Close();
		m_scpCryptIni.CloseForce();
	}
	else
		g_Log.Event(LOGL_WARN, "File '" SPHERE_FILE "Crypt.ini' not found\n");

	if ( OpenResourceFind(m_scpIni, SPHERE_FILE ".ini", false) )
	{
		LoadResourcesOpen(&m_scpIni);
		m_scpIni.Close();
		m_scpIni.CloseForce();
		return true;
	}
	else
	{
		g_Log.Event(LOGL_FATAL, "File '" SPHERE_FILE ".ini' not found\n");
		return false;
	}
}

void CResource::Unload(bool fResync)
{
	ADDTOCALLSTACK("CResource::Unload");
	if ( fResync )
	{
		// Unlock all SCP and MUL files
		g_Install.CloseFiles();
		for ( size_t i = 0; ; ++i )
		{
			CResourceScript *pResFile = GetResourceFile(i);
			if ( !pResFile )
				break;
			pResFile->CloseForce();
		}
		m_scpIni.CloseForce();
		m_scpTables.CloseForce();
		return;
	}

	m_Functions.RemoveAll();
	m_Fame.RemoveAll();
	m_Karma.RemoveAll();
	m_MoonGates.Empty();
	m_NotoFameLevels.RemoveAll();
	m_NotoKarmaLevels.RemoveAll();
	m_NotoTitles.RemoveAll();
	m_Obscene.RemoveAll();
	m_PrivCommands->RemoveAll();
	m_ResourceFiles.RemoveAll();
	m_Runes.RemoveAll();
	m_SkillIndexDefs.RemoveAll();
	m_SkillNameDefs.RemoveAll();
	m_SpellDefs.RemoveAll();
	m_SpellDefs_Sorted.RemoveAll();
	m_StartDefs.RemoveAll();
	//m_MultiDefs.RemoveAll();
	//m_ResHash.RemoveAll();
	//m_Servers.RemoveAll();
	//m_WebPages.RemoveAll();
	CCrypt::ClearKeyTable();
}

bool CResource::Load(bool fResync)
{
	ADDTOCALLSTACK("CResource::Load");
	// ARGS:
	//  fResync = just look for changes.

	if ( fResync )
	{
		m_scpCryptIni.ReSync();
		m_scpCryptIni.CloseForce();

		m_scpIni.ReSync();
		m_scpIni.CloseForce();
	}
	else
	{
		CGString sMulPath = g_Install.GetMulPath();
#ifdef _WIN32
		if ( sMulPath.IsEmpty() )
		{
			// Try get UO install path from Windows registry
			g_Install.FindInstall();
			sMulPath = g_Install.GetMulPath();
		}
#endif
		if ( sMulPath.IsEmpty() )
		{
			g_Log.Event(LOGL_FATAL, "Unable to find Ultima Online MUL files automatically, please set the 'MulFiles' path manually on " SPHERE_FILE ".ini\n");
			return false;
		}
		g_Log.Event(LOGL_EVENT, "Loading MUL files from path: '%s'\n", static_cast<LPCTSTR>(sMulPath));
	}

	// Open MUL files
	VERFILE_TYPE i = g_Install.OpenFiles((1 << VERFILE_MULTIIDX)|(1 << VERFILE_MULTI)|(1 << VERFILE_VERDATA)|(1 << VERFILE_MAP)|(1 << VERFILE_STAIDX)|(1 << VERFILE_STATICS)|(1 << VERFILE_TILEDATA));
	if ( i != VERFILE_QTY )
	{
		g_Log.Event(LOGL_FATAL, "MUL file '%s' not found\n", g_Install.GetBaseFileName(i));
		return false;
	}

	// Load optional verdata file (modifies MUL stuff)
	try
	{
		g_VerData.Load(g_Install.m_File[VERFILE_VERDATA]);
	}
	catch ( const CGrayError &e )
	{
		g_Log.Event(LOGL_FATAL, "File '" SPHERE_FILE ".ini' not found\n");
		g_Log.CatchEvent(&e, "g_VerData.Load");
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		return false;
	}
	catch ( ... )
	{
		g_Log.Event(LOGL_FATAL, "File '" SPHERE_FILE ".ini' not found\n");
		g_Log.CatchEvent(NULL, "g_VerData.Load");
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		return false;
	}

	// Load *tables.scp file
	if ( fResync )
	{
		m_scpTables.ReSync();
	}
	else
	{
		if ( !OpenResourceFind(m_scpTables, SPHERE_FILE "tables") )
		{
			g_Log.Event(LOGL_FATAL, "File '" SPHERE_FILE "tables" SPHERE_SCRIPT "' not found\n");
			return false;
		}

		LoadResourcesOpen(&m_scpTables);
		m_scpTables.Close();
	}
	m_scpTables.CloseForce();

	// Initialize world sectors on server startup
	if ( !fResync )
		g_World.Init();

	// Index all script files
	AddResourceDir(m_sSCPBaseDir);
	size_t iCount = m_ResourceFiles.GetCount();
	g_Log.Event(LOGL_EVENT, "Indexing %" FMTSIZE_T " scripts...\n", iCount);

	for ( size_t j = 0; ; ++j )
	{
		CResourceScript *pResFile = GetResourceFile(j);
		if ( !pResFile )
			break;

		if ( fResync )
			pResFile->ReSync();
		else
			LoadResources(pResFile);

#ifdef _WIN32
		NTWindow_OnTick(0);
#endif
		g_Serv.PrintPercent(j + 1, iCount);
	}

	if ( fResync )
		g_World.Init();

	// Make sure we have the basics
	if ( g_Serv.GetName()[0] == '\0' )
		g_Serv.SetName(SPHERE_TITLE);

	if ( !g_Cfg.ResourceGetDef(RESOURCE_ID(RES_SKILLCLASS, 0)) )
	{
		// Must have at least 1 skill class
		CSkillClassDef *pSkillClass = new CSkillClassDef(RESOURCE_ID(RES_SKILLCLASS));
		ASSERT(pSkillClass);
		m_ResHash.AddSortKey(RESOURCE_ID(RES_SKILLCLASS, 0), pSkillClass);
	}

	if ( m_StartDefs.GetCount() <= 0 )
		g_Log.Event(LOGL_ERROR, "Map template [STARTS] not found on scripts, clients will not be able to create new characters\n");

	// Make region DEFNAMEs
	size_t iMax = g_Cfg.m_RegionDefs.GetCount();
	for ( size_t j = 0; j < iMax; ++j )
	{
		CRegionBase *pRegion = dynamic_cast<CRegionBase *>(g_Cfg.m_RegionDefs.GetAt(j));
		if ( !pRegion )
			continue;
		pRegion->MakeRegionName();
	}

	if ( fResync && g_Serv.m_fReloadMultis )
	{
		g_World.ReloadMultis();
		g_Serv.m_fReloadMultis = false;
	}

	m_iEventsItemLink.Empty();
	if ( !m_sEventsItem.IsEmpty() )
	{
		CScript script("EVENTSITEM", m_sEventsItem);
		m_iEventsItemLink.r_LoadVal(script, RES_EVENTS);
	}

	m_pEventsPetLink.Empty();
	if ( !m_sEventsPet.IsEmpty() )
	{
		CScript script("EVENTSPET", m_sEventsPet);
		m_pEventsPetLink.r_LoadVal(script, RES_EVENTS);
	}

	m_pEventsPlayerLink.Empty();
	if ( !m_sEventsPlayer.IsEmpty() )
	{
		CScript script("EVENTSPLAYER", m_sEventsPlayer);
		m_pEventsPlayerLink.r_LoadVal(script, RES_EVENTS);
	}

	m_pEventsRegionLink.Empty();
	if ( !m_sEventsRegion.IsEmpty() )
	{
		CScript script("EVENTSREGION", m_sEventsRegion);
		m_pEventsRegionLink.r_LoadVal(script, RES_REGIONTYPE);
	}

	if ( !m_sChatStaticChannels.IsEmpty() )
	{
		TCHAR *ppArgs[32];
		size_t iChannels = Str_ParseCmds(const_cast<TCHAR *>(g_Cfg.m_sChatStaticChannels.GetPtr()), ppArgs, COUNTOF(ppArgs), ",");
		for ( size_t n = 0; n < iChannels; ++n )
			g_Serv.m_Chats.CreateChannel(ppArgs[n]);
	}

	LoadSortSpells();

	long lTotal, lUsed;
	Triglist(lTotal, lUsed);
	g_Log.Event(LOGL_EVENT, "Done loading scripts (%ld of %ld triggers used)\n", lUsed, lTotal);
	return true;
}

LPCTSTR CResource::GetDefaultMsg(int iKeyNum)
{
	ADDTOCALLSTACK("CResource::GetDefaultMsg");
	if ( (iKeyNum < 0) || (iKeyNum >= DEFMSG_QTY) )
		return "";

	return g_Exp.sm_szMessages[iKeyNum];
}

LPCTSTR CResource::GetDefaultMsg(LPCTSTR pszKey)
{
	ADDTOCALLSTACK("CResource::GetDefaultMsg");
	for ( int i = 0; i < DEFMSG_QTY; ++i )
	{
		if ( !strcmpi(pszKey, g_Exp.sm_szMsgNames[i]) )
			return g_Exp.sm_szMessages[i];
	}
	return "";
}

bool CResource::GenerateDefname(TCHAR *pszObjectName, size_t iInputLength, LPCTSTR pszPrefix, TCHAR *pszOutput, bool fCheckConflict, CVarDefMap *vDefnames)
{
	ADDTOCALLSTACK("CResource::GenerateDefname");
	if ( !pszOutput )
		return false;

	if ( !pszObjectName || !pszObjectName[0] )
	{
		pszOutput[0] = '\0';
		return false;
	}

	size_t iOut = 0;

	if ( pszPrefix )
	{
		// Write prefix
		for ( size_t i = 0; pszPrefix[i] != '\0'; ++i )
			pszOutput[iOut++] = pszPrefix[i];
	}

	// Write object name
	for ( size_t i = 0; i < iInputLength; ++i )
	{
		if ( pszObjectName[i] == '\0' )
			break;

		if ( ISWHITESPACE(pszObjectName[i]) )
		{
			if ( (iOut > 0) && (pszOutput[iOut - 1] != '_') )	// avoid double '_'
				pszOutput[iOut++] = '_';
		}
		else if ( _ISCSYMF(pszObjectName[i]) )
		{
			if ( (pszObjectName[i] != '_') || ((iOut > 0) && (pszOutput[iOut - 1] != '_')) )	// avoid double '_'
				pszOutput[iOut++] = static_cast<TCHAR>(tolower(pszObjectName[i]));
		}
	}

	// Remove trailing '_'
	while ( (iOut > 0) && (pszOutput[iOut - 1] == '_') )
		--iOut;

	pszOutput[iOut] = '\0';
	if ( iOut <= 0 )
		return false;

	if ( fCheckConflict )
	{
		size_t iEnd = iOut;
		int iAttempts = 1;

		for (;;)
		{
			bool fValid = true;
			if ( g_Exp.m_VarDefs.GetKey(pszOutput) || (vDefnames && vDefnames->GetKey(pszOutput)) )
				fValid = false;

			if ( fValid )
				break;

			// attach suffix
			iOut = iEnd;
			pszOutput[iOut++] = '_';
			ITOA(++iAttempts, &pszOutput[iOut], 10);
		}
	}

	// Set defname
	if ( vDefnames )
		vDefnames->SetNum(pszOutput, 1);

	return true;
}

bool CResource::DumpUnscriptedItems(CTextConsole *pSrc, LPCTSTR pszFilename)
{
	ADDTOCALLSTACK("CResource::DumpUnscriptedItems");
	if ( !pSrc )
		return false;

	if ( !pszFilename || (pszFilename[0] == '\0') )
		pszFilename = "unscripted_items" SPHERE_SCRIPT;
	else if ( strlen(pszFilename) <= 4 )
		return false;

	// Open file
	CScript s;
	if ( !s.Open(pszFilename, OF_WRITE|OF_TEXT|OF_DEFAULTMODE) )
		return false;

	s.Printf("// Unscripted items, generated by " SPHERE_TITLE " at %s\n", CGTime::GetCurrentTime().Format(NULL));

	TCHAR szItemName[21];	// should be always sizeof(CUOItemTypeRec2::m_name) + 1
	TemporaryString sDefnameBuffer;
	TCHAR *pszDefnameBuffer = static_cast<TCHAR *>(sDefnameBuffer);
	CVarDefMap vDefnames;

	ITEMID_TYPE idMaxItem = CGrayItemInfo::GetMaxTileDataItem();
	if ( idMaxItem > ITEMID_MULTI )
		idMaxItem = ITEMID_MULTI;

	for ( int i = 0; i < idMaxItem; ++i )
	{
		if ( !(i % 0xFF) )
			g_Serv.PrintPercent(i, idMaxItem);

		RESOURCE_ID rid = RESOURCE_ID(RES_ITEMDEF, i);
		if ( g_Cfg.m_ResHash.FindKey(rid) != g_Cfg.m_ResHash.BadIndex() )
			continue;

		// Check item in tiledata
		CUOItemTypeRec2 tiledata;
		if ( !CItemBase::GetItemData(static_cast<ITEMID_TYPE>(i), &tiledata) )
			continue;

		s.WriteSection("ITEMDEF 0%x", i);

		strcpylen(szItemName, tiledata.m_name, sizeof(szItemName));
		if ( GenerateDefname(szItemName, sizeof(szItemName), "i_", pszDefnameBuffer, true, &vDefnames) )
		{
			s.Printf("//%s\n", szItemName);
			s.WriteKey("DEFNAME", pszDefnameBuffer);
		}
		else
			s.Printf("//unnamed object\n");

		IT_TYPE type = CItemBase::GetTypeBase(static_cast<ITEMID_TYPE>(i), tiledata);
		if ( type != IT_NORMAL )
			s.WriteKey("TYPE", ResourceGetName(RESOURCE_ID(RES_TYPEDEF, type)));
	}

	s.WriteSection("EOF");
	s.Close();

#ifdef _WIN32
	// This is needed to clear g_Serv.PrintPercent()
	NTWindow_SetWindowTitle();
#endif
	return true;
}

int CItemTypeDef::GetItemType() const
{
	ADDTOCALLSTACK("CItemTypeDef::GetItemType");
	return GETINTRESOURCE(GetResourceID());
}

bool CItemTypeDef::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CItemTypeDef::r_LoadVal");
	EXC_TRY("LoadVal");

	LPCTSTR pszKey = s.GetKey();
	if ( !strnicmp(pszKey, "TERRAIN", 7) )
	{
		LPCTSTR pszArgs = s.GetArgStr();
		int iVal = Exp_GetVal(pszArgs);
		if ( iVal < 0 )
			return false;
		int iLo = iVal;

		GETNONWHITESPACE(pszArgs);
		if ( *pszArgs == ',' )
		{
			++pszArgs;
			GETNONWHITESPACE(pszArgs);
		}

		if ( pszArgs )
		{
			iVal = Exp_GetVal(pszArgs);
			if ( iVal < 0 )
				return false;

			if ( (iVal > 0) && (iVal < iLo) )	// swap
			{
				int iTemp = iLo;
				iLo = iVal;
				iVal = iTemp;
			}
		}

		int iHi = iVal;
		for ( int i = iLo; i <= iHi; ++i )
			g_World.m_TileTypes.SetAtGrow(static_cast<size_t>(i), this);

		return true;
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}
