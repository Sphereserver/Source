#include "graysvr.h"	// predef header.
#include "../common/grayver.h"
#include "../common/CFileList.h"
#include "../network/network.h"

CResource::CResource()
{
	m_timePeriodic.Init();

	m_fUseNTService = false;
	m_fUseHTTP		= 2;
	m_fUseAuthID	= true;
	m_iMapCacheTime = 2*60*TICK_PER_SEC;
	m_iSectorSleepMask = (1<<10)-1;
	m_fUseMapDiffs = false;

	m_wDebugFlags = 0; //DEBUGF_NPC_EMOTE
	m_fSecure = true;
	m_iFreezeRestartTime = 60;
	m_bAgree = false;
	m_fMd5Passwords = false;

	//Magic
	m_fReagentsRequired = false;
	m_fReagentLossFail = false;
	m_iWordsOfPowerColor = HUE_TEXT_DEF;
	m_iWordsOfPowerFont = FONT_NORMAL;
	m_fWordsOfPowerPlayer = true;
	m_fWordsOfPowerStaff = false;
	m_fEquippedCast = true;
	m_iMagicUnlockDoor = 900;
	m_iSpellTimeout = 0;

	m_iSpell_Teleport_Effect_Staff = ITEMID_FX_FLAMESTRIKE;	// drama
	m_iSpell_Teleport_Sound_Staff = 0x1f3;
	m_iSpell_Teleport_Effect_Players = ITEMID_FX_TELE_VANISH;
	m_iSpell_Teleport_Sound_Players = 0x01fe;
	m_iSpell_Teleport_Effect_NPC = ITEMID_FX_HEAL_EFFECT;
	m_iSpell_Teleport_Sound_NPC = 0x01fe;

	// Decay
	m_iDecay_Item = 30*60*TICK_PER_SEC;
	m_iDecay_CorpsePlayer = 7*60*TICK_PER_SEC;
	m_iDecay_CorpseNPC = 7*60*TICK_PER_SEC;

	// Accounts
	m_iClientsMax		= FD_SETSIZE-1;
	m_iClientsMaxIP		= 16;
	m_iConnectingMax	= 32;
	m_iConnectingMaxIP	= 8;

	m_iGuestsMax = 0;
	m_iArriveDepartMsg = 1;
	m_iClientLingerTime = 10*60*TICK_PER_SEC;
	m_iDeadSocketTime = 5*60*TICK_PER_SEC;
	m_iMinCharDeleteTime = 7*24*60*60*TICK_PER_SEC;
	m_iMaxCharsPerAccount = 5;
	m_fLocalIPAdmin = true;

	// Save
	m_iSaveNPCSkills = 10;
	m_iSaveBackupLevels = 10;
	m_iSaveBackgroundTime = 0;		// Use the new background save.
	m_fSaveGarbageCollect = true;	// Always force a full garbage collection.
	m_iSavePeriod = 20*60*TICK_PER_SEC;
	m_iSaveSectorsPerTick = 1;
	m_iSaveStepMaxComplexity = 500;

	// In game effects.
	m_fCanUndressPets	= true;
	m_fMonsterFight		= false;
	m_fMonsterFear		= false;
	m_iLightDungeon		= 27;
	m_iLightNight		= 25;	// dark before t2a.
	m_iLightDay		= LIGHT_BRIGHT;
	m_iBankIMax		= 1000;
	m_iBankWMax		= 1000 * WEIGHT_UNITS;
	m_fAttackingIsACrime	= true;
	m_fGuardsInstantKill	= true;
	m_fGuardsOnMurderers	= true;
	m_iSnoopCriminal	= 100;
	m_iTradeWindowSnooping = true;
	m_iTrainSkillCost = 1;
	m_iTrainSkillMax = 420;
	m_iTrainSkillPercent = 30;
	m_fDeadCannotSeeLiving = 0;
	m_iMediumCanHearGhosts	= 1000;
	m_iSkillPracticeMax	= 300;
	m_iPacketDeathAnimation = true;
	m_fCharTags			= false;
	m_fVendorTradeTitle	= true;
	m_iVendorMaxSell	= 255;
	m_iGameMinuteLength	= 20*TICK_PER_SEC;
	m_fNoWeather		= true;
	m_fFlipDroppedItems	= true;
	m_iItemsMaxAmount	= 60000;
	m_iMurderMinCount	= 5;
	m_iMurderDecayTime	= 8*60*60*TICK_PER_SEC;
	m_iMaxCharComplexity	= 32;
	m_iMaxItemComplexity	= 25;
	m_iMaxSectorComplexity	= 1024;
	m_iPlayerKarmaNeutral	= -2000; // How much bad karma makes a player neutral?
	m_iPlayerKarmaEvil	= -8000;
	m_iMinKarma = -10000;
	m_iMaxKarma = 10000;
	m_iMaxFame = 10000;
	m_iGuardLingerTime	= 3*60*TICK_PER_SEC;
	m_iCriminalTimer	= 3*60*TICK_PER_SEC;
	m_iHitpointPercentOnRez	= 10;
	m_iHitsHungerLoss	= 0;
	m_fLootingIsACrime	= true;
	m_fHelpingCriminalsIsACrime = true;
	m_fGenericSounds	= true;
	m_fAutoNewbieKeys 	= true;
	m_iMaxBaseSkill		= 200;
	m_iStamRunningPenalty 	= 50;
	m_iStaminaLossAtWeight 	= 150;
	m_iMountHeight		= false;
	m_iMoveRate			= 100;
	m_iArcheryMinDist	= 2;
	m_iArcheryMaxDist	= 15;
	m_iHitsUpdateRate	= TICK_PER_SEC;
	m_iSpeedScaleFactor	= 80000;
	m_iCombatFlags		= 0;
	m_iCombatDamageEra	= 0;
	m_iCombatHitChanceEra = 0;
	m_iCombatSpeedEra	= 0;
	m_iMagicFlags		= 0;
	m_iMaxPolyStats		= 150;
	m_iRacialFlags		= 0;
	m_iRevealFlags		= (REVEALF_DETECTINGHIDDEN|REVEALF_LOOTINGSELF|REVEALF_LOOTINGOTHERS|REVEALF_SPEAK|REVEALF_SPELLCAST);

	m_fNoResRobe		= 0;
	m_iLostNPCTeleport	= 50;
	m_iExperimental		= 0;
	m_iDistanceYell		= UO_MAP_VIEW_RADAR;
	m_iDistanceWhisper	= 3;
	m_iDistanceTalk		= UO_MAP_VIEW_SIZE;
	m_iOptionFlags		= (OF_Command_Sysmsgs|OF_NoHouseMuteSpeech);

	m_iMaxSkill			= SKILL_QTY;
	m_iWalkBuffer		= 75;
	m_iWalkRegen		= 25;
	m_iWoolGrowthTime	= 30*60*TICK_PER_SEC;
	m_iAttackerTimeout	= 300;

	m_iCommandLog		= 0;
	m_fTelnetLog		= true;

	m_fUsecrypt 		= true; // Server want crypt client ?
	m_fUsenocrypt		= false; // Server want un-crypt client ? (version guessed by cliver)
	m_fPayFromPackOnly	= false; // pay vendors from packs only

	m_iOverSkillMultiply = 2;
	m_fSuppressCapitals = false;

	m_iAdvancedLos		= 0;

	// New ones
	m_iFeatureT2A		= (FEATURE_T2A_UPDATE|FEATURE_T2A_CHAT);
	m_iFeatureLBR		= 0;
	m_iFeatureAOS		= 0;
	m_iFeatureSE		= 0;
	m_iFeatureML		= 0;
	m_iFeatureKR		= 0;
	m_iFeatureSA		= 0;
	m_iFeatureTOL		= 0;
	m_iFeatureExtra		= 0;

	m_iStatFlag = 0;

	m_iNpcAi = 0;
	m_iMaxLoopTimes = 100000;

	m_bAutoResDisp = true;
	m_iAutoPrivFlags = PRIV_DETAIL;

	// Third Party Tools
	m_fCUOStatus = true;
	m_fUOGStatus = true;

	//	Experience
	m_bExperienceSystem = false;
	m_iExperienceMode = 0;
	m_iExperienceKoefPVP = 100;
	m_iExperienceKoefPVM = 100;
	m_bLevelSystem = false;
	m_iLevelMode = LEVEL_MODE_DOUBLE;
	m_iLevelNextAt = 0;

	//	MySQL support
	m_bMySql = false;

	m_cCommandPrefix = '.';

	m_iDefaultCommandLevel = 7;	//PLevel 7 default for command levels.

	m_iRegenRate[STAT_STR] = 40*TICK_PER_SEC;		// Seconds to heal ONE hp (before stam/food adjust)
	m_iRegenRate[STAT_INT] = 20*TICK_PER_SEC;		// Seconds to heal ONE mn
	m_iRegenRate[STAT_DEX] = 10*TICK_PER_SEC;		// Seconds to heal ONE stm
	m_iRegenRate[STAT_FOOD] = 60*60*TICK_PER_SEC;	// Food usage (1 time per 60 minutes)

	m_iTimerCall = 0;
	m_bAllowLightOverride = true;
	m_bAllowNewbTransfer = false;
	m_sZeroPoint= "1323,1624,0";
	m_bAllowBuySellAgent = false;

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
	
	m_iNotoTimeout = 30;					// seconds to remove this character from notoriety list.

	m_iPetsInheritNotoriety = 0;

#ifdef _MTNETWORK
	m_iNetworkThreads = 0;
	m_iNetworkThreadPriority = IThread::Disabled;
#endif
	m_fUseAsyncNetwork = 0;
	m_iNetMaxPings = 15;
	m_iNetHistoryTTL = 300;
	m_iNetMaxPacketsPerTick = 50;
	m_iNetMaxLengthPerTick = 18000;
	m_iNetMaxQueueSize = 75;
	m_fUsePacketPriorities = false;
	m_fUseExtraBuffer = true;

	m_iTooltipCache = 30*TICK_PER_SEC;
	m_iTooltipMode = TOOLTIPMODE_SENDVERSION;
	m_iAutoTooltipResend = (AUTOTOOLTIP_FLAG_NAME|AUTOTOOLTIP_FLAG_AMOUNT|AUTOTOOLTIP_FLAG_WEIGHT|AUTOTOOLTIP_FLAG_DURABILITY|AUTOTOOLTIP_FLAG_POISON|AUTOTOOLTIP_FLAG_WANDCHARGES|AUTOTOOLTIP_FLAG_SPELLBOOK);
	m_iContextMenuLimit = 15;

	m_iClientLoginMaxTries = 0;		// maximum bad password tries before a temp ip ban
	m_iClientLoginTempBan = 3*60*TICK_PER_SEC;
	m_iMaxShipPlankTeleport = UO_MAP_VIEW_SIZE;

	m_NPCNoFameTitle = false;
}

CResource::~CResource()
{
	for ( size_t i = 0; i < COUNTOF(m_ResHash.m_Array); i++ )
	{
		for ( size_t j = 0; j < m_ResHash.m_Array[i].GetCount(); j++ )
		{
			CResourceDef* pResDef = m_ResHash.m_Array[i][j];
			if ( pResDef != NULL )
			{
				pResDef->UnLink();
			}
		}
	}

	Unload(false);
}


// SKILL ITEMDEF, etc
bool CResource::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CResource::r_GetRef");
	TCHAR * pszSep = const_cast<TCHAR*>(strchr( pszKey, '(' ));	// acts like const_cast
	if ( pszSep == NULL )
	{
		pszSep = const_cast<TCHAR*>(strchr( pszKey, '.' ));
		if ( pszSep == NULL )
			return( false );
	}

	char oldChar = *pszSep;
	*pszSep = '\0';

	int iResType = FindTableSorted( pszKey, sm_szResourceBlocks, RES_QTY );
	bool fNewStyleDef = false;

	if ( iResType < 0 )
	{
		if (strcmpi(pszKey, "MULTIDEF") == 0)
		{
			iResType = RES_ITEMDEF;
			fNewStyleDef = true;
		}
		else
		{
			*pszSep = oldChar;
			return( false );
		}
	}

	*pszSep = '.';

	// Now get the index.
	pszKey = pszSep + 1;
	if ( pszKey[0] == '\0' )
		return( false );

	pszSep = const_cast<TCHAR*>(strchr( pszKey, '.' ));
	if ( pszSep != NULL )
	{
		*pszSep = '\0';
	}

	if ( iResType == RES_SERVERS )
	{
		pRef = NULL;
	}
	else if ( iResType == RES_CHARDEF )
	{
		//pRef = CCharBase::FindCharBase(static_cast<CREID_TYPE>(Exp_GetVal(pszKey)));
		pRef = CCharBase::FindCharBase(static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType(RES_CHARDEF, pszKey)));
	}
	else if ( iResType == RES_ITEMDEF )
	{
		if (fNewStyleDef && IsDigit(pszKey[0]))
			pRef = CItemBase::FindItemBase(static_cast<ITEMID_TYPE>(Exp_GetVal(pszKey) + ITEMID_MULTI));
		else
			pRef = CItemBase::FindItemBase(static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType(RES_ITEMDEF, pszKey)));
	}
	else if ( iResType == RES_SPELL && *pszKey == '-' )
	{
		pszKey++;
		size_t iOrder = Exp_GetVal( pszKey );
		if ( !m_SpellDefs_Sorted.IsValidIndex( iOrder ) )
			pRef = NULL;
		else
			pRef = m_SpellDefs_Sorted[iOrder];
	}
	else
	{
		RESOURCE_ID	rid	= ResourceGetID(static_cast<RES_TYPE>(iResType), pszKey);

		// check the found resource type matches what we searched for
		if ( rid.GetResType() == iResType )
		{
			pRef = ResourceGetDef( rid );
		}
	}

	if ( pszSep != NULL )
	{
		*pszSep = oldChar; //*pszSep = '.';
		pszKey = pszSep+1;
	}
	else
	{
		pszKey += strlen(pszKey);
	}
	return( true );
}


enum RC_TYPE
{
	RC_ACCTFILES,				// m_sAcctBaseDir
	RC_ADVANCEDLOS,				// m_iAdvancedLos
	RC_AGREE,
	RC_ALLOWBUYSELLAGENT,		// m_bAllowBuySellAgent
	RC_ALLOWLIGHTOVERRIDE,		// m_bAllowLightOverride
	RC_ALLOWNEWBTRANSFER,		// m_bAllowNewbTransfer
	RC_ARCHERYMAXDIST,			// m_iArcheryMaxDist
	RC_ARCHERYMINDIST,			// m_iArcheryMinDist
	RC_ARRIVEDEPARTMSG,
	RC_ATTACKERTIMEOUT,			// m_iAttackerTimeout
	RC_ATTACKINGISACRIME,		// m_fAttackingIsACrime
	RC_AUTONEWBIEKEYS,			// m_fAutoNewbieKeys
	RC_AUTOPRIVFLAGS,			// m_iAutoPrivFlags
	RC_AUTORESDISP,				// m_bAutoResDisp
	RC_AUTOTOOLTIPRESEND,		// m_iAutoTooltipResend
	RC_BACKUPLEVELS,			// m_iSaveBackupLevels
	RC_BANKMAXITEMS,
	RC_BANKMAXWEIGHT,
	RC_BUILD,
	RC_CANUNDRESSPETS,			// m_fCanUndressPets
	RC_CHARTAGS,				// m_fCharTags
	RC_CLIENTLINGER,
	RC_CLIENTLOGINMAXTRIES,		// m_iClientLoginMaxTries
	RC_CLIENTLOGINTEMPBAN,		// m_iClientLoginTempBan
	RC_CLIENTMAX,				// m_iClientsMax
	RC_CLIENTMAXIP,				// m_iClientsMaxIP
	RC_CLIENTS,
	RC_COLORHIDDEN,
	RC_COLORINVIS,
	RC_COLORINVISSPELL,
	RC_COLORNOTOCRIMINAL,		// m_iColorNotoCriminal
	RC_COLORNOTODEFAULT,		// m_iColorNotoDefault
	RC_COLORNOTOEVIL,			// m_iColorNotoEvil
	RC_COLORNOTOGOOD,			// m_iColorNotoGood
	RC_COLORNOTOGUILDSAME,		// m_iColorNotoGuildSame
	RC_COLORNOTOGUILDWAR,		// m_iColorNotoGuildWar
	RC_COLORNOTOINVUL,			// m_iColorNotoInvul
	RC_COLORNOTOINVULGAMEMASTER,// m_iColorNotoInvulGameMaster
	RC_COLORNOTONEUTRAL,		// m_iColorNotoNeutral
	RC_COMBATDAMAGEERA,			// m_iCombatDamageEra
	RC_COMBATFLAGS,				// m_iCombatFlags
	RC_COMBATHITCHANCEERA,		// m_iCombatHitChanceEra
	RC_COMBATSPEEDERA,			// m_iCombatSpeedEra
	RC_COMMANDLOG,
	RC_COMMANDPREFIX,
	RC_COMMANDTRIGGER,			// m_sCommandTrigger
	RC_CONNECTINGMAX,			// m_iConnectingMax
	RC_CONNECTINGMAXIP,			// m_iConnectingMaxIP
	RC_CONTEXTMENULIMIT,		// m_iContextMenuLimit
	RC_CORPSENPCDECAY,
	RC_CORPSEPLAYERDECAY,
	RC_CRIMINALTIMER,			// m_iCriminalTimer
	RC_CUOSTATUS,				// m_fCUOStatus
	RC_DEADCANNOTSEELIVING,
	RC_DEADSOCKETTIME,
	RC_DEBUGFLAGS,
	RC_DECAYTIMER,
	RC_DEFAULTCOMMANDLEVEL,		//m_iDefaultCommandLevel
	RC_DISTANCETALK,
	RC_DISTANCEWHISPER,
	RC_DISTANCEYELL,
#ifdef _DUMPSUPPORT
	RC_DUMPPACKETSFORACC,
#endif
	RC_DUNGEONLIGHT,
	RC_EQUIPPEDCAST,			// m_fEquippedCast
	RC_EVENTSITEM,				// m_sEventsItem
	RC_EVENTSPET,				// m_sEventsPet
	RC_EVENTSPLAYER,			// m_sEventsPlayer
	RC_EVENTSREGION,			// m_sEventsRegion
	RC_EXPERIENCEKOEFPVM,		// m_iExperienceKoefPVM
	RC_EXPERIENCEKOEFPVP,		// m_iExperienceKoefPVP
	RC_EXPERIENCEMODE,			// m_iExperienceMode
	RC_EXPERIENCESYSTEM,		// m_bExperienceSystem
	RC_EXPERIMENTAL,			// m_iExperimental
	RC_FEATURESAOS,
	RC_FEATURESEXTRA,
	RC_FEATURESKR,
	RC_FEATURESLBR,
	RC_FEATURESML,
	RC_FEATURESSA,
	RC_FEATURESSE,
	RC_FEATUREST2A,
	RC_FEATURESTOL,
	RC_FLIPDROPPEDITEMS,		// m_fFlipDroppedItems
	RC_FORCEGARBAGECOLLECT,		// m_fSaveGarbageCollect
	RC_FREEZERESTARTTIME,		// m_iFreezeRestartTime
	RC_GAMEMINUTELENGTH,		// m_iGameMinuteLength
	RC_GENERICSOUNDS,			// m_fGenericSounds
	RC_GUARDLINGER,				// m_iGuardLingerTime
	RC_GUARDSINSTANTKILL,
	RC_GUARDSONMURDERERS,
	RC_GUESTSMAX,
	RC_GUILDS,
	RC_HEARALL,
	RC_HELPINGCRIMINALSISACRIME,// m_fHelpingCriminalsIsACrime
	RC_HITPOINTPERCENTONREZ,	// m_iHitpointPercentOnRez
	RC_HITSHUNGERLOSS,			// m_iHitsHungerLoss
	RC_HITSUPDATERATE,
	RC_ITEMSMAXAMOUNT,			// m_iItemsMaxAmount
	RC_LEVELMODE,				// m_iLevelMode
	RC_LEVELNEXTAT,				// m_iLevelNextAt
	RC_LEVELSYSTEM,				// m_bLevelSystem
	RC_LIGHTDAY,				// m_iLightDay
	RC_LIGHTNIGHT,				// m_iLightNight
	RC_LOCALIPADMIN,			// m_fLocalIPAdmin
	RC_LOG,
	RC_LOGMASK,					// GetLogMask
	RC_LOOTINGISACRIME,			// m_fLootingIsACrime
	RC_LOSTNPCTELEPORT,			// m_fLostNPCTeleport
	RC_MAGICFLAGS,
	RC_MAGICUNLOCKDOOR,			// m_iMagicUnlockDoor
	RC_MAPCACHETIME,
	RC_MAXBASESKILL,			// m_iMaxBaseSkill
	RC_MAXCHARSPERACCOUNT,		// m_iMaxCharsPerAccount
	RC_MAXCOMPLEXITY,			// m_iMaxCharComplexity
	RC_MAXFAME,					// m_iMaxFame
	RC_MAXITEMCOMPLEXITY,		// m_iMaxItemComplexity
	RC_MAXKARMA,				// m_iMaxKarma
	RC_MAXLOOPTIMES,			// m_iMaxLoopTimes
	RC_MAXPACKETSPERTICK,		// m_iNetMaxPacketsPerTick
	RC_MAXPINGS,				// m_iNetMaxPings
	RC_MAXPOLYSTATS,			// m_iMaxPolyStats
	RC_MAXQUEUESIZE,			// m_iNetMaxQueueSize
	RC_MAXSECTORCOMPLEXITY,		// m_iMaxSectorComplexity
	RC_MAXSHIPPLANKTELEPORT,	// m_iMaxShipPlankTeleport
	RC_MAXSIZEPERTICK,			// m_iNetMaxLengthPerTick
	RC_MD5PASSWORDS,			// m_fMd5Passwords
	RC_MEDIUMCANHEARGHOSTS,		// m_iMediumCanHearGhosts
	RC_MINCHARDELETETIME,
	RC_MINKARMA,				// m_iMinKarma
	RC_MONSTERFEAR,				// m_fMonsterFear
	RC_MONSTERFIGHT,
	RC_MOUNTHEIGHT,				// m_iMountHeight
	RC_MOVERATE,				// m_iMoveRate
	RC_MULFILES,
	RC_MURDERDECAYTIME,			// m_iMurderDecayTime;
	RC_MURDERMINCOUNT,			// m_iMurderMinCount
	RC_MYSQL,					// m_bMySql
	RC_MYSQLDB,					// m_sMySqlDatabase
	RC_MYSQLHOST,				// m_sMySqlHost
	RC_MYSQLPASS,				// m_sMySqlPassword
	RC_MYSQLUSER,				// m_sMySqlUser
	RC_NETTTL,					// m_iNetHistoryTTL
#ifdef _MTNETWORK
	RC_NETWORKTHREADPRIORITY,	// m_iNetworkThreadPriority
	RC_NETWORKTHREADS,			// m_iNetworkThreads
#endif
	RC_NORESROBE,
	RC_NOTOTIMEOUT,
	RC_NOWEATHER,				// m_fNoWeather
	RC_NPCAI,					// m_iNpcAi
	RC_NPCNOFAMETITLE,			// m_NPCNoFameTitle
	RC_NPCSKILLSAVE,			// m_iSaveNPCSkills
	RC_NPCTRAINCOST,			// m_iTrainSkillCost
	RC_NPCTRAINMAX,				// m_iTrainSkillMax
	RC_NPCTRAINPERCENT,			// m_iTrainSkillPercent
	RC_NTSERVICE,				// m_fUseNTService
	RC_OPTIONFLAGS,				// m_iOptionFlags
	RC_OVERSKILLMULTIPLY,		// m_iOverSkillMultiply
	RC_PACKETDEATHANIMATION,	// m_iPacketDeathAnimation
	RC_PAYFROMPACKONLY,			// m_fPayFromPackOnly
	RC_PETSINHERITNOTORIETY,	// m_iPetsInheritNotoriety
	RC_PLAYEREVIL,				// m_iPlayerKarmaEvil
	RC_PLAYERNEUTRAL,			// m_iPlayerKarmaNeutral
	RC_PROFILE,
	RC_RACIALFLAGS,				// m_iRacialFlags
	RC_REAGENTLOSSFAIL,			// m_fReagentLossFail
	RC_REAGENTSREQUIRED,
	RC_REVEALFLAGS,				// m_iRevealFlags
	RC_RTICKS,
	RC_RTIME,
	RC_RUNNINGPENALTY,			// m_iStamRunningPenalty
	RC_SAVEBACKGROUND,			// m_iSaveBackgroundTime
	RC_SAVEPERIOD,
	RC_SAVESECTORSPERTICK,		// m_iSaveSectorsPerTick
	RC_SAVESTEPMAXCOMPLEXITY,	// m_iSaveStepMaxComplexity
	RC_SCPFILES,
	RC_SECTORSLEEP,				// m_iSectorSleepMask
	RC_SECURE,
	RC_SKILLPRACTICEMAX,		// m_iSkillPracticeMax
	RC_SNOOPCRIMINAL,
	RC_SPEECHOTHER,
	RC_SPEECHPET,
	RC_SPEECHSELF,
	RC_SPEEDSCALEFACTOR,
	RC_SPELLTIMEOUT,
	RC_STAMINALOSSATWEIGHT,		// m_iStaminaLossAtWeight
	RC_STATSFLAGS,				// m_iStatFlag
	RC_STRIPPATH,				// for TNG
	RC_SUPPRESSCAPITALS,
	RC_TELEPORTEFFECTNPC,		// m_iSpell_Teleport_Effect_NPC
	RC_TELEPORTEFFECTPLAYERS,	// m_iSpell_Teleport_Effect_Players
	RC_TELEPORTEFFECTSTAFF,		// m_iSpell_Teleport_Effect_Staff
	RC_TELEPORTSOUNDNPC,		// m_iSpell_Teleport_Sound_NPC
	RC_TELEPORTSOUNDPLAYERS,	// m_iSpell_Teleport_Sound_Players
	RC_TELEPORTSOUNDSTAFF,		// m_iSpell_Teleport_Sound_Staff
	RC_TELNETLOG,				// m_fTelnetLog
	RC_TIMERCALL,				// m_iTimerCall
	RC_TIMEUP,
	RC_TOOLTIPCACHE,			// m_iTooltipCache
	RC_TOOLTIPMODE,				// m_iTooltipMode
	RC_TRADEWINDOWSNOOPING,		// m_iTradeWindowSnooping
	RC_UOGSTATUS,				// m_fUOGStatus
	RC_USEASYNCNETWORK,			// m_fUseAsyncNetwork
	RC_USEAUTHID,				// m_fUseAuthID
	RC_USECRYPT,				// m_Usecrypt
	RC_USEEXTRABUFFER,			// m_fUseExtraBuffer
	RC_USEHTTP,					// m_fUseHTTP
	RC_USEMAPDIFFS,				// m_fUseMapDiffs
	RC_USENOCRYPT,				// m_Usenocrypt
	RC_USEPACKETPRIORITY,		// m_fUsePacketPriorities
	RC_VENDORMAXSELL,			// m_iVendorMaxSell
	RC_VENDORTRADETITLE,		// m_fVendorTradeTitle
	RC_VERSION,
	RC_WALKBUFFER,
	RC_WALKREGEN,
	RC_WOOLGROWTHTIME,			// m_iWoolGrowthTime
	RC_WOPCOLOR,
	RC_WOPFONT,
	RC_WOPPLAYER,
	RC_WOPSTAFF,
	RC_WORLDSAVE,
	RC_ZEROPOINT,				// m_sZeroPoint
	RC_QTY
}; 


const CAssocReg CResource::sm_szLoadKeys[RC_QTY+1] =
{
	{ "ACCTFILES",				{ ELEM_CSTRING,	OFFSETOF(CResource,m_sAcctBaseDir),			0 }},
	{ "ADVANCEDLOS",			{ ELEM_INT,		OFFSETOF(CResource,m_iAdvancedLos),			0 }},
	{ "AGREE",					{ ELEM_BOOL,	OFFSETOF(CResource,m_bAgree),				0 }},
	{ "ALLOWBUYSELLAGENT",		{ ELEM_BOOL,	OFFSETOF(CResource,m_bAllowBuySellAgent),	0 }},
	{ "ALLOWLIGHTOVERRIDE",		{ ELEM_BOOL,	OFFSETOF(CResource,m_bAllowLightOverride),	0 }},
	{ "ALLOWNEWBTRANSFER",		{ ELEM_BOOL,	OFFSETOF(CResource,m_bAllowNewbTransfer),	0 }},
	{ "ARCHERYMAXDIST",			{ ELEM_INT,		OFFSETOF(CResource,m_iArcheryMaxDist),		0 }},
	{ "ARCHERYMINDIST",			{ ELEM_INT,		OFFSETOF(CResource,m_iArcheryMinDist),		0 }},
	{ "ARRIVEDEPARTMSG",		{ ELEM_INT,		OFFSETOF(CResource,m_iArriveDepartMsg),		0 }},
	{ "ATTACKERTIMEOUT",		{ ELEM_INT,		OFFSETOF(CResource,m_iAttackerTimeout),		0 }},
	{ "ATTACKINGISACRIME",		{ ELEM_BOOL,	OFFSETOF(CResource,m_fAttackingIsACrime),	0 }},
	{ "AUTONEWBIEKEYS",			{ ELEM_BOOL,	OFFSETOF(CResource,m_fAutoNewbieKeys),		0 }},
	{ "AUTOPRIVFLAGS",			{ ELEM_INT,		OFFSETOF(CResource,m_iAutoPrivFlags),		0 }},
	{ "AUTORESDISP",			{ ELEM_BOOL,	OFFSETOF(CResource,m_bAutoResDisp),			0 }},
	{ "AUTOTOOLTIPRESEND",		{ ELEM_INT,		OFFSETOF(CResource,m_iAutoTooltipResend),	0 }},
	{ "BACKUPLEVELS",			{ ELEM_INT,		OFFSETOF(CResource,m_iSaveBackupLevels),	0 }},
	{ "BANKMAXITEMS",			{ ELEM_INT,		OFFSETOF(CResource,m_iBankIMax),			0 }},
	{ "BANKMAXWEIGHT",			{ ELEM_INT,		OFFSETOF(CResource,m_iBankWMax),			0 }},
	{ "BUILD",					{ ELEM_VOID,	0,											0 }},
	{ "CANUNDRESSPETS",			{ ELEM_BOOL,	OFFSETOF(CResource,m_fCanUndressPets),		0 }},
	{ "CHARTAGS",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fCharTags),			0 }},
	{ "CLIENTLINGER",			{ ELEM_INT,		OFFSETOF(CResource,m_iClientLingerTime),	0 }},
	{ "CLIENTLOGINMAXTRIES",	{ ELEM_INT,		OFFSETOF(CResource,m_iClientLoginMaxTries),	0 }},
	{ "CLIENTLOGINTEMPBAN",		{ ELEM_INT,		OFFSETOF(CResource,m_iClientLoginTempBan),	0 }},
	{ "CLIENTMAX",				{ ELEM_INT,		OFFSETOF(CResource,m_iClientsMax),			0 }},
	{ "CLIENTMAXIP",			{ ELEM_INT,		OFFSETOF(CResource,m_iClientsMaxIP),		0 }},
	{ "CLIENTS",				{ ELEM_VOID,	0,											0 }},	// duplicate
	{ "COLORHIDDEN",			{ ELEM_VOID,	OFFSETOF(CResource,m_iColorHidden),			0 }},
	{ "COLORINVIS",				{ ELEM_VOID,	OFFSETOF(CResource,m_iColorInvis),			0 }},
	{ "COLORINVISSPELL",		{ ELEM_VOID,	OFFSETOF(CResource,m_iColorInvisSpell),		0 }},
	{ "COLORNOTOCRIMINAL",		{ ELEM_WORD,	OFFSETOF(CResource,m_iColorNotoCriminal),	0 }},
	{ "COLORNOTODEFAULT",		{ ELEM_WORD,	OFFSETOF(CResource,m_iColorNotoDefault),	0 }},
	{ "COLORNOTOEVIL",			{ ELEM_WORD,	OFFSETOF(CResource,m_iColorNotoEvil),		0 }},
	{ "COLORNOTOGOOD",			{ ELEM_WORD,	OFFSETOF(CResource,m_iColorNotoGood),		0 }},
	{ "COLORNOTOGUILDSAME",		{ ELEM_WORD,	OFFSETOF(CResource,m_iColorNotoGuildSame),	0 }},
	{ "COLORNOTOGUILDWAR",		{ ELEM_WORD,	OFFSETOF(CResource,m_iColorNotoGuildWar),	0 }},
	{ "COLORNOTOINVUL",			{ ELEM_WORD,	OFFSETOF(CResource,m_iColorNotoInvul),		0 }},
	{ "COLORNOTOINVULGAMEMASTER",{ ELEM_WORD,	OFFSETOF(CResource,m_iColorNotoInvulGameMaster),	0 }},
	{ "COLORNOTONEUTRAL",		{ ELEM_WORD,	OFFSETOF(CResource,m_iColorNotoNeutral),	0 }},
	{ "COMBATDAMAGEERA",		{ ELEM_BYTE,	OFFSETOF(CResource,m_iCombatDamageEra),		0 }},
	{ "COMBATFLAGS",			{ ELEM_INT,		OFFSETOF(CResource,m_iCombatFlags),			0 }},
	{ "COMBATHITCHANCEERA",		{ ELEM_BYTE,	OFFSETOF(CResource,m_iCombatHitChanceEra),	0 }},
	{ "COMBATSPEEDERA",			{ ELEM_BYTE,	OFFSETOF(CResource,m_iCombatSpeedEra),		0 }},
	{ "COMMANDLOG",				{ ELEM_INT,		OFFSETOF(CResource,m_iCommandLog),			0 }},
	{ "COMMANDPREFIX",			{ ELEM_BYTE,	OFFSETOF(CResource,m_cCommandPrefix),		0 }},
	{ "COMMANDTRIGGER",			{ ELEM_CSTRING,	OFFSETOF(CResource,m_sCommandTrigger),		0 }},
	{ "CONNECTINGMAX",			{ ELEM_INT,		OFFSETOF(CResource,m_iConnectingMax),		0 }},
	{ "CONNECTINGMAXIP",		{ ELEM_INT,		OFFSETOF(CResource,m_iConnectingMaxIP),		0 }},
	{ "CONTEXTMENULIMIT",		{ ELEM_INT,		OFFSETOF(CResource,m_iContextMenuLimit),	0 }},
	{ "CORPSENPCDECAY",			{ ELEM_INT,		OFFSETOF(CResource,m_iDecay_CorpseNPC),		0 }},
	{ "CORPSEPLAYERDECAY",		{ ELEM_INT,		OFFSETOF(CResource,m_iDecay_CorpsePlayer),	0 }},
	{ "CRIMINALTIMER",			{ ELEM_INT,		OFFSETOF(CResource,m_iCriminalTimer),		0 }},
	{ "CUOSTATUS",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fCUOStatus),			0 }},
	{ "DEADCANNOTSEELIVING",	{ ELEM_INT,		OFFSETOF(CResource,m_fDeadCannotSeeLiving),	0 }},
	{ "DEADSOCKETTIME",			{ ELEM_INT,		OFFSETOF(CResource,m_iDeadSocketTime),		0 }},
	{ "DEBUGFLAGS",				{ ELEM_WORD,	OFFSETOF(CResource,m_wDebugFlags),			0 }},
	{ "DECAYTIMER",				{ ELEM_INT,		OFFSETOF(CResource,m_iDecay_Item),			0 }},
	{ "DEFAULTCOMMANDLEVEL",	{ ELEM_INT,		OFFSETOF(CResource,m_iDefaultCommandLevel),	0 }},
	{ "DISTANCETALK",			{ ELEM_INT,		OFFSETOF(CResource,m_iDistanceTalk ),		0 }},
	{ "DISTANCEWHISPER",		{ ELEM_INT,		OFFSETOF(CResource,m_iDistanceWhisper ),	0 }},
	{ "DISTANCEYELL",			{ ELEM_INT,		OFFSETOF(CResource,m_iDistanceYell ),		0 }},
#ifdef _DUMPSUPPORT
	{ "DUMPPACKETSFORACC",		{ ELEM_CSTRING,	OFFSETOF(CResource,m_sDumpAccPackets),		0 }},
#endif
	{ "DUNGEONLIGHT",			{ ELEM_INT,		OFFSETOF(CResource,m_iLightDungeon),		0 }},
	{ "EQUIPPEDCAST",			{ ELEM_BOOL,	OFFSETOF(CResource,m_fEquippedCast),		0 }},
	{ "EVENTSITEM",				{ ELEM_CSTRING, OFFSETOF(CResource,m_sEventsItem),			0 }},
	{ "EVENTSPET",				{ ELEM_CSTRING,	OFFSETOF(CResource,m_sEventsPet),			0 }},
	{ "EVENTSPLAYER",			{ ELEM_CSTRING,	OFFSETOF(CResource,m_sEventsPlayer),		0 }},
	{ "EVENTSREGION",			{ ELEM_CSTRING,	OFFSETOF(CResource,m_sEventsRegion),		0 }},
	{ "EXPERIENCEKOEFPVM",		{ ELEM_INT,		OFFSETOF(CResource,m_iExperienceKoefPVM),	0 }},
	{ "EXPERIENCEKOEFPVP",		{ ELEM_INT,		OFFSETOF(CResource,m_iExperienceKoefPVP),	0 }},
	{ "EXPERIENCEMODE",			{ ELEM_INT,		OFFSETOF(CResource,m_iExperienceMode),		0 }},
	{ "EXPERIENCESYSTEM",		{ ELEM_BOOL,	OFFSETOF(CResource,m_bExperienceSystem),	0 }},
	{ "EXPERIMENTAL",			{ ELEM_INT,		OFFSETOF(CResource,m_iExperimental),		0 }},
	{ "FEATUREAOS",				{ ELEM_INT,		OFFSETOF(CResource,m_iFeatureAOS),			0 }},
	{ "FEATUREEXTRA",			{ ELEM_INT,		OFFSETOF(CResource,m_iFeatureExtra),		0 }},
	{ "FEATUREKR",				{ ELEM_INT,		OFFSETOF(CResource,m_iFeatureKR),			0 }},
	{ "FEATURELBR",				{ ELEM_INT,		OFFSETOF(CResource,m_iFeatureLBR),			0 }},
	{ "FEATUREML",				{ ELEM_INT,		OFFSETOF(CResource,m_iFeatureML),			0 }},
	{ "FEATURESA",				{ ELEM_INT,		OFFSETOF(CResource,m_iFeatureSA),			0 }},
	{ "FEATURESE",				{ ELEM_INT,		OFFSETOF(CResource,m_iFeatureSE),			0 }},
	{ "FEATURET2A",				{ ELEM_INT,		OFFSETOF(CResource,m_iFeatureT2A),			0 }},
	{ "FEATURETOL",				{ ELEM_INT,		OFFSETOF(CResource,m_iFeatureTOL),			0 }},
	{ "FLIPDROPPEDITEMS",		{ ELEM_BOOL,	OFFSETOF(CResource,m_fFlipDroppedItems),	0 }},
	{ "FORCEGARBAGECOLLECT",	{ ELEM_BOOL,	OFFSETOF(CResource,m_fSaveGarbageCollect),	0 }},
	{ "FREEZERESTARTTIME",		{ ELEM_INT,		OFFSETOF(CResource,m_iFreezeRestartTime),	0 }},
	{ "GAMEMINUTELENGTH",		{ ELEM_INT,		OFFSETOF(CResource,m_iGameMinuteLength),	0 }},
	{ "GENERICSOUNDS",			{ ELEM_BOOL,	OFFSETOF(CResource,m_fGenericSounds),		0 }},
	{ "GUARDLINGER",			{ ELEM_INT,		OFFSETOF(CResource,m_iGuardLingerTime),		0 }},
	{ "GUARDSINSTANTKILL",		{ ELEM_BOOL,	OFFSETOF(CResource,m_fGuardsInstantKill),	0 }},
	{ "GUARDSONMURDERERS",		{ ELEM_BOOL,	OFFSETOF(CResource,m_fGuardsOnMurderers),	0 }},
	{ "GUESTSMAX",				{ ELEM_INT,		OFFSETOF(CResource,m_iGuestsMax),			0 }},
	{ "GUILDS",					{ ELEM_VOID,	0,											0 }},
	{ "HEARALL",				{ ELEM_VOID,	0,											0 }},
	{ "HELPINGCRIMINALSISACRIME",{ ELEM_BOOL,	OFFSETOF(CResource,m_fHelpingCriminalsIsACrime),	0 }},
	{ "HITPOINTPERCENTONREZ",	{ ELEM_INT,		OFFSETOF(CResource,m_iHitpointPercentOnRez),0 }},
	{ "HITSHUNGERLOSS",			{ ELEM_INT,		OFFSETOF(CResource,m_iHitsHungerLoss),		0 }},
	{ "HITSUPDATERATE",			{ ELEM_VOID,	0,											0 }},
	{ "ITEMSMAXAMOUNT",			{ ELEM_INT,		OFFSETOF(CResource,m_iItemsMaxAmount),		0 }},
	{ "LEVELMODE",				{ ELEM_INT,		OFFSETOF(CResource,m_iLevelMode),			0 }},
	{ "LEVELNEXTAT",			{ ELEM_INT,		OFFSETOF(CResource,m_iLevelNextAt),			0 }},
	{ "LEVELSYSTEM",			{ ELEM_BOOL,	OFFSETOF(CResource,m_bLevelSystem),			0 }},
	{ "LIGHTDAY",				{ ELEM_INT,		OFFSETOF(CResource,m_iLightDay),			0 }},
	{ "LIGHTNIGHT",				{ ELEM_INT,		OFFSETOF(CResource,m_iLightNight),			0 }},
	{ "LOCALIPADMIN",			{ ELEM_BOOL,	OFFSETOF(CResource,m_fLocalIPAdmin),		0 }}, // The local ip is assumed to be the admin.
	{ "LOG",					{ ELEM_VOID,	0,											0 }},
	{ "LOGMASK",				{ ELEM_VOID,	0,											0 }}, // GetLogMask
	{ "LOOTINGISACRIME",		{ ELEM_BOOL,	OFFSETOF(CResource,m_fLootingIsACrime),		0 }},
	{ "LOSTNPCTELEPORT",		{ ELEM_INT,		OFFSETOF(CResource,m_iLostNPCTeleport),		0 }},
	{ "MAGICFLAGS",				{ ELEM_INT,		OFFSETOF(CResource,m_iMagicFlags),			0 }},
	{ "MAGICUNLOCKDOOR",		{ ELEM_INT,		OFFSETOF(CResource,m_iMagicUnlockDoor),		0 }},
	{ "MAPCACHETIME",			{ ELEM_INT,		OFFSETOF(CResource,m_iMapCacheTime),		0 }},
	{ "MAXBASESKILL",			{ ELEM_INT,		OFFSETOF(CResource,m_iMaxBaseSkill),		0 }},
	{ "MAXCHARSPERACCOUNT",		{ ELEM_BYTE,	OFFSETOF(CResource,m_iMaxCharsPerAccount),	0 }},
	{ "MAXCOMPLEXITY",			{ ELEM_INT,		OFFSETOF(CResource,m_iMaxCharComplexity),	0 }},
	{ "MAXFAME",				{ ELEM_INT,		OFFSETOF(CResource,m_iMaxFame),				0 }},
	{ "MAXITEMCOMPLEXITY",		{ ELEM_INT,		OFFSETOF(CResource,m_iMaxItemComplexity),	0 }},
	{ "MAXKARMA",				{ ELEM_INT,		OFFSETOF(CResource,m_iMaxKarma),			0 }},
	{ "MAXLOOPTIMES",			{ ELEM_INT,		OFFSETOF(CResource,m_iMaxLoopTimes),		0 }},
	{ "MAXPACKETSPERTICK",		{ ELEM_INT,		OFFSETOF(CResource,m_iNetMaxPacketsPerTick),0 }},
	{ "MAXPINGS",				{ ELEM_INT,		OFFSETOF(CResource,m_iNetMaxPings),			0 }},
	{ "MAXPOLYSTATS",			{ ELEM_INT,		OFFSETOF(CResource,m_iMaxPolyStats),		0 }},
	{ "MAXQUEUESIZE",			{ ELEM_INT,		OFFSETOF(CResource,m_iNetMaxQueueSize),		0 }},
	{ "MAXSECTORCOMPLEXITY",	{ ELEM_INT,		OFFSETOF(CResource,m_iMaxSectorComplexity),	0 }},
	{ "MAXSHIPPLANKTELEPORT",	{ ELEM_INT,		OFFSETOF(CResource,m_iMaxShipPlankTeleport),0 }},
	{ "MAXSIZEPERTICK",			{ ELEM_INT,		OFFSETOF(CResource,m_iNetMaxLengthPerTick),	0 }},
	{ "MD5PASSWORDS",			{ ELEM_BOOL,	OFFSETOF(CResource,m_fMd5Passwords),		0 }},
	{ "MEDIUMCANHEARGHOSTS",	{ ELEM_INT,		OFFSETOF(CResource,m_iMediumCanHearGhosts),	0 }},
	{ "MINCHARDELETETIME",		{ ELEM_INT,		OFFSETOF(CResource,m_iMinCharDeleteTime),	0 }},
	{ "MINKARMA",				{ ELEM_INT,		OFFSETOF(CResource,m_iMinKarma),			0 }},
	{ "MONSTERFEAR",			{ ELEM_BOOL,	OFFSETOF(CResource,m_fMonsterFear),			0 }},
	{ "MONSTERFIGHT",			{ ELEM_BOOL,	OFFSETOF(CResource,m_fMonsterFight),		0 }},
	{ "MOUNTHEIGHT",			{ ELEM_BOOL,	OFFSETOF(CResource,m_iMountHeight),			0 }},
	{ "MOVERATE",				{ ELEM_INT,		OFFSETOF(CResource,m_iMoveRate),			0 }},
	{ "MULFILES",				{ ELEM_VOID,	0,											0 }},
	{ "MURDERDECAYTIME",		{ ELEM_INT,		OFFSETOF(CResource,m_iMurderDecayTime),		0 }},
	{ "MURDERMINCOUNT",			{ ELEM_INT,		OFFSETOF(CResource,m_iMurderMinCount),		0 }}, // amount of murders before we get title.
	{ "MYSQL",					{ ELEM_BOOL,	OFFSETOF(CResource,m_bMySql),				0 }},
	{ "MYSQLDATABASE",			{ ELEM_CSTRING,	OFFSETOF(CResource,m_sMySqlDB),				0 }},
	{ "MYSQLHOST",				{ ELEM_CSTRING, OFFSETOF(CResource,m_sMySqlHost),			0 }},
	{ "MYSQLPASSWORD",			{ ELEM_CSTRING,	OFFSETOF(CResource,m_sMySqlPass),			0 }},
	{ "MYSQLUSER",				{ ELEM_CSTRING,	OFFSETOF(CResource,m_sMySqlUser),			0 }},
	{ "NETTTL",					{ ELEM_INT,		OFFSETOF(CResource,m_iNetHistoryTTL),		0 }},
#ifdef _MTNETWORK
	{ "NETWORKTHREADPRIORITY",	{ ELEM_INT,		OFFSETOF(CResource,m_iNetworkThreadPriority),	0 }},
	{ "NETWORKTHREADS",			{ ELEM_INT,		OFFSETOF(CResource,m_iNetworkThreads),		0 }},
#endif
	{ "NORESROBE",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fNoResRobe),			0 }},
	{ "NOTOTIMEOUT",			{ ELEM_INT,		OFFSETOF(CResource,m_iNotoTimeout),			0 }},
	{ "NOWEATHER",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fNoWeather),			0 }},
	{ "NPCAI",					{ ELEM_INT,		OFFSETOF(CResource,m_iNpcAi),				0 }},
	{ "NPCNOFAMETITLE",			{ ELEM_BOOL,	OFFSETOF(CResource,m_NPCNoFameTitle),		0 }},
	{ "NPCSKILLSAVE",			{ ELEM_INT,		OFFSETOF(CResource,m_iSaveNPCSkills),		0 }},
	{ "NPCTRAINCOST",			{ ELEM_INT,		OFFSETOF(CResource,m_iTrainSkillCost),		0 }},
	{ "NPCTRAINMAX",			{ ELEM_INT,		OFFSETOF(CResource,m_iTrainSkillMax),		0 }},
	{ "NPCTRAINPERCENT",		{ ELEM_INT,		OFFSETOF(CResource,m_iTrainSkillPercent),	0 }},
	{ "NTSERVICE",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fUseNTService),		0 }},
	{ "OPTIONFLAGS",			{ ELEM_INT,		OFFSETOF(CResource,m_iOptionFlags),			0 }},
	{ "OVERSKILLMULTIPLY",		{ ELEM_INT,		OFFSETOF(CResource,m_iOverSkillMultiply),	0 }},
	{ "PACKETDEATHANIMATION",	{ ELEM_BOOL,	OFFSETOF(CResource,m_iPacketDeathAnimation),0 }},
	{ "PAYFROMPACKONLY",		{ ELEM_BOOL,	OFFSETOF(CResource,m_fPayFromPackOnly),		0 }},
	{ "PETSINHERITNOTORIETY",	{ ELEM_INT,		OFFSETOF(CResource,m_iPetsInheritNotoriety),0 }},
	{ "PLAYEREVIL",				{ ELEM_INT,		OFFSETOF(CResource,m_iPlayerKarmaEvil),		0 }},
	{ "PLAYERNEUTRAL",			{ ELEM_INT,		OFFSETOF(CResource,m_iPlayerKarmaNeutral),	0 }},
	{ "PROFILE",				{ ELEM_VOID,	0,											0 }},
	{ "RACIALFLAGS",			{ ELEM_INT,		OFFSETOF(CResource,m_iRacialFlags),			0 }},
	{ "REAGENTLOSSFAIL",		{ ELEM_BOOL,	OFFSETOF(CResource,m_fReagentLossFail),		0 }},
	{ "REAGENTSREQUIRED",		{ ELEM_BOOL,	OFFSETOF(CResource,m_fReagentsRequired),	0 }},
	{ "REVEALFLAGS",			{ ELEM_INT,		OFFSETOF(CResource,m_iRevealFlags),			0 }},
	{ "RTICKS",					{ ELEM_VOID,	0,											0 }},
	{ "RTIME",					{ ELEM_VOID,	0,											0 }},
	{ "RUNNINGPENALTY",			{ ELEM_INT,		OFFSETOF(CResource,m_iStamRunningPenalty),	0 }},
	{ "SAVEBACKGROUND",			{ ELEM_INT,		OFFSETOF(CResource,m_iSaveBackgroundTime),	0 }},
	{ "SAVEPERIOD",				{ ELEM_INT,		OFFSETOF(CResource,m_iSavePeriod),			0 }},
	{ "SAVESECTORSPERTICK",		{ ELEM_INT,		OFFSETOF(CResource,m_iSaveSectorsPerTick),	0 }},
	{ "SAVESTEPMAXCOMPLEXITY",	{ ELEM_INT,		OFFSETOF(CResource,m_iSaveStepMaxComplexity),	0 }},
	{ "SCPFILES",				{ ELEM_CSTRING,	OFFSETOF(CResource,m_sSCPBaseDir),			0 }},
	{ "SECTORSLEEP",			{ ELEM_INT,		OFFSETOF(CResource,m_iSectorSleepMask),		0 }},
	{ "SECURE",					{ ELEM_BOOL,	OFFSETOF(CResource,m_fSecure),				0 }},
	{ "SKILLPRACTICEMAX",		{ ELEM_INT,		OFFSETOF(CResource,m_iSkillPracticeMax),	0 }},
	{ "SNOOPCRIMINAL",			{ ELEM_INT,		OFFSETOF(CResource,m_iSnoopCriminal),		0 }},
	{ "SPEECHOTHER",			{ ELEM_CSTRING,	OFFSETOF(CResource,m_sSpeechOther),			0 }},
	{ "SPEECHPET",				{ ELEM_CSTRING,	OFFSETOF(CResource,m_sSpeechPet),			0 }},
	{ "SPEECHSELF",				{ ELEM_CSTRING,	OFFSETOF(CResource,m_sSpeechSelf),			0 }},
	{ "SPEEDSCALEFACTOR",		{ ELEM_INT,		OFFSETOF(CResource,m_iSpeedScaleFactor),	0 }},
	{ "SPELLTIMEOUT",			{ ELEM_INT,		OFFSETOF(CResource,m_iSpellTimeout),		0 }},
	{ "STAMINALOSSATWEIGHT",	{ ELEM_INT,		OFFSETOF(CResource,m_iStaminaLossAtWeight),	0 }},
	{ "STATSFLAGS",				{ ELEM_INT,		OFFSETOF(CResource,m_iStatFlag),			0 }},
	{ "STRIPPATH",				{ ELEM_INT,		OFFSETOF(CResource,m_sStripPath),			0 }},
	{ "SUPPRESSCAPITALS",		{ ELEM_BOOL,	OFFSETOF(CResource,m_fSuppressCapitals),	0 }},
	{ "TELEPORTEFFECTNPC",		{ ELEM_INT,		OFFSETOF(CResource,m_iSpell_Teleport_Effect_NPC),		0 }},
	{ "TELEPORTEFFECTPLAYERS",	{ ELEM_INT,		OFFSETOF(CResource,m_iSpell_Teleport_Effect_Players),	0 }},
	{ "TELEPORTEFFECTSTAFF",	{ ELEM_INT,		OFFSETOF(CResource,m_iSpell_Teleport_Effect_Staff),		0 }},
	{ "TELEPORTSOUNDNPC",		{ ELEM_INT,		OFFSETOF(CResource,m_iSpell_Teleport_Sound_NPC),		0 }},
	{ "TELEPORTSOUNDPLAYERS",	{ ELEM_INT,		OFFSETOF(CResource,m_iSpell_Teleport_Sound_Players),	0 }},
	{ "TELEPORTSOUNDSTAFF",		{ ELEM_INT,		OFFSETOF(CResource,m_iSpell_Teleport_Sound_Staff),		0 }},
	{ "TELNETLOG",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fTelnetLog),			0 }},
	{ "TIMERCALL",				{ ELEM_INT,		OFFSETOF(CResource,m_iTimerCall),			0 }},
	{ "TIMEUP",					{ ELEM_VOID,	0,											0 }},
	{ "TOOLTIPCACHE",			{ ELEM_INT,		OFFSETOF(CResource,m_iTooltipCache),		0 }},
	{ "TOOLTIPMODE",			{ ELEM_INT,		OFFSETOF(CResource,m_iTooltipMode),			0 }},
	{ "TRADEWINDOWSNOOPING",	{ ELEM_BOOL,	OFFSETOF(CResource,m_iTradeWindowSnooping),	0 }},
	{ "UOGSTATUS",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fUOGStatus),			0 }},
	{ "USEASYNCNETWORK",		{ ELEM_INT,		OFFSETOF(CResource,m_fUseAsyncNetwork),		0 }},
	{ "USEAUTHID",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fUseAuthID),			0 }},	// we use authid like osi
	{ "USECRYPT",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fUsecrypt),			0 }},	// we don't want crypt clients
	{ "USEEXTRABUFFER",			{ ELEM_BOOL,	OFFSETOF(CResource,m_fUseExtraBuffer),		0 }},
	{ "USEHTTP",				{ ELEM_INT,		OFFSETOF(CResource,m_fUseHTTP),				0 }},
	{ "USEMAPDIFFS",			{ ELEM_BOOL,	OFFSETOF(CResource,m_fUseMapDiffs),			0 }},
	{ "USENOCRYPT",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fUsenocrypt),			0 }},	// we don't want no-crypt clients
	{ "USEPACKETPRIORITY",		{ ELEM_BOOL,	OFFSETOF(CResource,m_fUsePacketPriorities),	0 }},
	{ "VENDORMAXSELL",			{ ELEM_INT,		OFFSETOF(CResource,m_iVendorMaxSell),		0 }},
	{ "VENDORTRADETITLE",		{ ELEM_BOOL,	OFFSETOF(CResource,m_fVendorTradeTitle),	0 }},
	{ "VERSION",				{ ELEM_VOID,	0,											0 }},
	{ "WALKBUFFER",				{ ELEM_INT,		OFFSETOF(CResource,m_iWalkBuffer),			0 }},
	{ "WALKREGEN",				{ ELEM_INT,		OFFSETOF(CResource,m_iWalkRegen),			0 }},
	{ "WOOLGROWTHTIME",			{ ELEM_INT,		OFFSETOF(CResource,m_iWoolGrowthTime),		0 }},
	{ "WOPCOLOR",				{ ELEM_INT,		OFFSETOF(CResource,m_iWordsOfPowerColor),	0 }},
	{ "WOPFONT",				{ ELEM_INT,		OFFSETOF(CResource,m_iWordsOfPowerFont),	0 }},
	{ "WOPPLAYER",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fWordsOfPowerPlayer),	0 }},
	{ "WOPSTAFF",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fWordsOfPowerStaff),	0 }},
	{ "WORLDSAVE",				{ ELEM_CSTRING,	OFFSETOF(CResource,m_sWorldBaseDir),		0 }},
	{ "ZEROPOINT",				{ ELEM_CSTRING,	OFFSETOF(CResource,m_sZeroPoint),			0 }},
	{ NULL,						{ ELEM_VOID,	0,											0 }}
};

bool CResource::r_LoadVal( CScript &s )
{
	ADDTOCALLSTACK("CResource::r_LoadVal");
	EXC_TRY("LoadVal");

	int i = FindTableHeadSorted( s.GetKey(), reinterpret_cast<LPCTSTR const *>(sm_szLoadKeys), COUNTOF( sm_szLoadKeys )-1, sizeof(sm_szLoadKeys[0]));
	if ( i < 0 )
	{
		if ( s.IsKeyHead( "REGEN", 5 ))			//	REGENx=<stat regeneration rate>
		{
			int index = ATOI(s.GetKey()+5);
			if (index < 0 || index >= STAT_QTY)
				return false;
			g_Cfg.m_iRegenRate[index] = (s.GetArgVal() * TICK_PER_SEC);
			return true;
		}
		else if ( s.IsKeyHead("MAP", 3) )		//	MAPx=settings
		{
			bool ok = true;
			std::string str = s.GetKey()+3;
			for ( size_t j = 0; j < str.size(); ++j )
			{
				if ( IsDigit(str[j]) )
					continue;

				ok = false;
				break;
			}

			if ( ok && str.size() > 0 )
			{
				return g_MapList.Load(ATOI(str.c_str()), s.GetArgRaw());
			}
			else
			{
				size_t length = str.size();

				if ( length >= 2/*at least .X*/ && str[0] == '.' && isdigit(str[1]) )
				{
					LPCTSTR pszStr = &(str[1]);
					int nMapNumber = Exp_GetVal(pszStr);

					if ( g_MapList.IsMapSupported(nMapNumber) )
					{
						SKIP_SEPARATORS(pszStr);

						if ( strcmpi(pszStr, "ALLSECTORS") == 0 )
						{
							int nSectors = g_MapList.GetSectorQty(nMapNumber);
							pszStr = s.GetArgRaw();

							if ( pszStr && *pszStr )
							{
								CScript scp(pszStr);

								for ( int nIndex = 0; nIndex < nSectors; ++nIndex )
									g_World.GetSector(nMapNumber, nIndex)->r_Verb(scp, &g_Serv);
							}

							return true;
						}
						else if ( !strnicmp( pszStr, "SECTOR.",7) )
						{
							pszStr = pszStr + 7;
							int iSecNumber = Exp_GetVal(pszStr);
							int nSectors = g_MapList.GetSectorQty(nMapNumber);
							SKIP_SEPARATORS(pszStr);

							if ((iSecNumber > 0) && (iSecNumber <=  nSectors))
							{
								pszStr = s.GetArgRaw();

								if ( pszStr && *pszStr )
								{
									CScript scp(pszStr);
									g_World.GetSector(nMapNumber, iSecNumber-1)->r_Verb(scp, &g_Serv);
								}
							}
							else
								g_Log.EventError("Invalid Sector #%d for Map %d\n", iSecNumber, nMapNumber);

							return true;
						}
					}
				}
	
				DEBUG_ERR(("Bad usage of MAPx. Check your " GRAY_FILE ".ini or scripts (SERV.MAP is a read only property)\n"));
				return false;
			}
		}
		else if ( s.IsKeyHead("PACKET", 6) )	//	PACKETx=<function name to execute upon packet>
		{
			int index = ATOI(s.GetKey() + 6);
			if (( index >= 0 ) && ( index < 255 )) // why XCMD_QTY? let's them hook every possible custom packet
			{
				char *args = s.GetArgRaw();
				if ( !args || ( strlen(args) >= 31 ))
					g_Log.EventError("Invalid function name for packet filtering (limit is 30 chars).\n");
				else
				{
					strcpy(g_Serv.m_PacketFilter[index], args);
					DEBUG_MSG(("PACKET FILTER: Hooked packet 0x%x with function %s.\n", index, args));
					return true;
				}
			}
			else
				g_Log.EventError("Packet filtering index %d out of range [0..254]\n", index);
		}
		else if ( s.IsKeyHead("OUTPACKET", 9) )	//	OUTPACKETx=<function name to execute upon packet>
		{
			int index = ATOI(s.GetKey() + 9);
			if (( index >= 0 ) && ( index < 255 ))
			{
				char *args = s.GetArgRaw();
				if ( !args || ( strlen(args) >= 31 ))
					g_Log.EventError("Invalid function name for outgoing packet filtering (limit is 30 chars).\n");
				else
				{
					strcpy(g_Serv.m_OutPacketFilter[index], args);
					DEBUG_MSG(("OUTGOING PACKET FILTER: Hooked packet 0x%x with function %s.\n", index, args));
					return true;
				}
			}
			else
				g_Log.EventError("Outgoing packet filtering index %d out of range [0..254]\n", index);
		}

		return(false);
	}

	switch (i)
	{
		case RC_AGREE:
			m_bAgree = (s.GetArgVal() != 0);
			break;
		case RC_ACCTFILES:	// Put acct files here.
			m_sAcctBaseDir = CGFile::GetMergedFileName( s.GetArgStr(), "" );
			break;
		case RC_ATTACKERTIMEOUT:
			m_iAttackerTimeout = s.GetArgVal();
			break;
		case RC_BANKMAXWEIGHT:
			m_iBankWMax = s.GetArgVal() * WEIGHT_UNITS;
			break;
		case RC_CLIENTLINGER:
			m_iClientLingerTime = s.GetArgVal() * TICK_PER_SEC;
			break;
		case RC_CLIENTLOGINMAXTRIES:
			{
				m_iClientLoginMaxTries = s.GetArgVal();
				if ( m_iClientLoginMaxTries < 0 )
					m_iClientLoginMaxTries = 0;
			} break;
		case RC_CLIENTLOGINTEMPBAN:
			m_iClientLoginTempBan = s.GetArgVal() * 60 * TICK_PER_SEC;
			break;
		case RC_CLIENTMAX:
		case RC_CLIENTS:
			m_iClientsMax = s.GetArgVal();
			if ( m_iClientsMax > FD_SETSIZE-1 )	// Max number we can deal with. compile time thing.
			{
				m_iClientsMax = FD_SETSIZE-1;
			}
			break;
		case RC_COLORHIDDEN:
			m_iColorHidden = static_cast<HUE_TYPE>(s.GetArgVal());
			break;
		case RC_COLORINVIS:
			m_iColorInvis = static_cast<HUE_TYPE>(s.GetArgVal());
			break;
		case RC_COLORINVISSPELL:
			m_iColorInvisSpell = static_cast<HUE_TYPE>(s.GetArgVal());
			break;
		case RC_CORPSENPCDECAY:
			m_iDecay_CorpseNPC = s.GetArgVal()*60*TICK_PER_SEC;
			break;
		case RC_CORPSEPLAYERDECAY:
			m_iDecay_CorpsePlayer = s.GetArgVal()*60*TICK_PER_SEC ;
			break;
		case RC_CRIMINALTIMER:
			m_iCriminalTimer = s.GetArgVal() * 60 * TICK_PER_SEC;
			break;
		case RC_STRIPPATH:	// Put TNG stripped files here.
			m_sStripPath = CGFile::GetMergedFileName( s.GetArgStr(), "" );
			break;
		case RC_DEADSOCKETTIME:
			m_iDeadSocketTime = s.GetArgVal()*60*TICK_PER_SEC;
			break;
		case RC_DECAYTIMER:
			m_iDecay_Item = s.GetArgVal() *60*TICK_PER_SEC;
			break;
		case RC_FREEZERESTARTTIME:
			m_iFreezeRestartTime = s.GetArgVal() * TICK_PER_SEC;
			break;
		case RC_GAMEMINUTELENGTH:
			m_iGameMinuteLength = s.GetArgVal() * TICK_PER_SEC;
			break;
		case RC_GUARDLINGER:
			m_iGuardLingerTime = s.GetArgVal() * 60 * TICK_PER_SEC;
			break;
		case RC_HEARALL:
			g_Log.SetLogMask( s.GetArgFlag( g_Log.GetLogMask(), LOGM_PLAYER_SPEAK ));
			break;
		case RC_HITSUPDATERATE:
			m_iHitsUpdateRate = s.GetArgVal() * TICK_PER_SEC;
			break;
		case RC_LOG:
			g_Log.OpenLog( s.GetArgStr());
			break;
		case RC_LOGMASK:
			g_Log.SetLogMask( s.GetArgVal());
			break;
		case RC_MULFILES:
			g_Install.SetPreferPath( CGFile::GetMergedFileName( s.GetArgStr(), "" ));
			break;
		case RC_MAPCACHETIME:
			m_iMapCacheTime = s.GetArgVal() * TICK_PER_SEC;
			break;
		case RC_MAXCHARSPERACCOUNT:
			m_iMaxCharsPerAccount = static_cast<unsigned char>(s.GetArgVal());
			if ( m_iMaxCharsPerAccount > MAX_CHARS_PER_ACCT )
				m_iMaxCharsPerAccount = MAX_CHARS_PER_ACCT;
			break;
		case RC_MAXFAME:
			m_iMaxFame = s.GetArgVal();
			if (m_iMaxFame < 0)
				m_iMaxFame = 0;
			break;
		case RC_MAXKARMA:
			m_iMaxKarma = s.GetArgVal();
			if (m_iMaxKarma < m_iMinKarma)
				m_iMaxKarma = m_iMinKarma;
			break;
		case RC_MINCHARDELETETIME:
			m_iMinCharDeleteTime = s.GetArgVal()*TICK_PER_SEC;
			break;
		case RC_MINKARMA:
			m_iMinKarma = s.GetArgVal();
			if (m_iMinKarma > m_iMaxKarma)
				m_iMinKarma = m_iMaxKarma;
			break;
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
				int seconds = s.GetArgVal();
				size_t threadCount = ThreadHolder::getActiveThreads();
				for (size_t j = 0; j < threadCount; j++)
				{
					AbstractSphereThread *thread = static_cast<AbstractSphereThread *>(ThreadHolder::getThreadAt(j));
					if (thread != NULL)
						thread->m_profile.SetActive(seconds);
				}
			}
			break;

		case RC_PLAYEREVIL:	// How much bad karma makes a player evil?
			m_iPlayerKarmaEvil = s.GetArgVal();
			if ( m_iPlayerKarmaNeutral < m_iPlayerKarmaEvil )
				m_iPlayerKarmaNeutral = m_iPlayerKarmaEvil;
			break;

		case RC_PLAYERNEUTRAL:	// How much bad karma makes a player neutral?
			m_iPlayerKarmaNeutral = s.GetArgVal();
			if ( m_iPlayerKarmaEvil > m_iPlayerKarmaNeutral )
				m_iPlayerKarmaEvil = m_iPlayerKarmaNeutral;
			break;

		case RC_GUARDSINSTANTKILL:
			m_fGuardsInstantKill = s.GetArgVal() ? true : false;
			break;

		case RC_GUARDSONMURDERERS:
			m_fGuardsOnMurderers = s.GetArgVal() ? true : false;
			break;

		case RC_CONTEXTMENULIMIT:
			m_iContextMenuLimit = s.GetArgVal();
			break;

		case RC_SCPFILES: // Get SCP files from here.
			m_sSCPBaseDir = CGFile::GetMergedFileName( s.GetArgStr(), "" );
			break;

		case RC_SECURE:
			m_fSecure = (s.GetArgVal() != 0);
			if ( !g_Serv.IsLoading() )
				g_Serv.SetSignals();
			break;

		case RC_PACKETDEATHANIMATION:
			m_iPacketDeathAnimation = s.GetArgVal() > 0 ? true : false;
			break;

		case RC_SKILLPRACTICEMAX:
			m_iSkillPracticeMax = s.GetArgVal();
			break;

		case RC_SAVEPERIOD:
			m_iSavePeriod = s.GetArgVal()*60*TICK_PER_SEC;
			break;
		case RC_SPELLTIMEOUT:
			m_iSpellTimeout = s.GetArgVal() * TICK_PER_SEC;
			break;

		case RC_SECTORSLEEP:
			{
				int sleep = s.GetArgVal();
				m_iSectorSleepMask = sleep ? (( 1 << sleep) - 1) : 0;
			}
			break;

		case RC_SAVEBACKGROUND:
			m_iSaveBackgroundTime = s.GetArgVal() * 60 * TICK_PER_SEC;
			break;

		case RC_SAVESECTORSPERTICK:
			m_iSaveSectorsPerTick = s.GetArgVal();
			if ( m_iSaveSectorsPerTick <= 0 )
				m_iSaveSectorsPerTick = 1;
			break;

		case RC_SAVESTEPMAXCOMPLEXITY:
			m_iSaveStepMaxComplexity = s.GetArgVal();
			break;

		case RC_WORLDSAVE: // Put save files here.
			m_sWorldBaseDir = CGFile::GetMergedFileName( s.GetArgStr(), "" );
			break;

		case RC_COMMANDPREFIX:
			m_cCommandPrefix = *s.GetArgStr();
			break;

		case RC_EXPERIMENTAL:
			g_Cfg.m_iExperimental = s.GetArgVal();
			//PrintEFOFFlags(true, false);
			break;

		case RC_OPTIONFLAGS:
			g_Cfg.m_iOptionFlags = s.GetArgVal();
			//PrintEFOFFlags(false, true);
			break;
		case RC_TIMERCALL:
			m_iTimerCall = s.GetArgVal() * 60 * TICK_PER_SEC;
			break;

		case RC_TOOLTIPCACHE:
			g_Cfg.m_iTooltipCache = s.GetArgVal() * TICK_PER_SEC;
			break;
			
#ifdef _MTNETWORK
		case RC_NETWORKTHREADS:
			if (g_Serv.IsLoading())
				g_Cfg.m_iNetworkThreads = s.GetArgVal();
			else
				g_Log.EventError("The value of NetworkThreads cannot be modified after the server has started\n");
			break;

		case RC_NETWORKTHREADPRIORITY:
			{
				int priority = s.GetArgVal();
				if (priority < IThread::Idle)
					priority = IThread::Idle;
				else if (priority > IThread::RealTime)
					priority = IThread::Disabled;

				m_iNetworkThreadPriority = priority;
			}
			break;
#endif
		case RC_WALKBUFFER:
			m_iWalkBuffer = s.GetArgVal() * TICK_PER_SEC;
			break;


		default:
			return( sm_szLoadKeys[i].m_elem.SetValStr( this, s.GetArgRaw()));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}


const CSkillDef * CResource::SkillLookup( LPCTSTR pszKey )
{
	ADDTOCALLSTACK("CResource::SkillLookup");

	size_t iLen = strlen(pszKey);
	const CSkillDef *pDef;
	for ( size_t i = 0; i < m_SkillIndexDefs.GetCount(); ++i )
	{
		pDef = static_cast<const CSkillDef *>(m_SkillIndexDefs[i]);
		if ( pDef->m_sName.IsEmpty() ? !strnicmp(pszKey, pDef->GetKey(), iLen) : !strnicmp(pszKey, pDef->m_sName, iLen) )
			return pDef;
	}
	return NULL;
}



bool CResource::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CResource::r_WriteVal");
	EXC_TRY("WriteVal");
	// Just do stats values for now.
	int index = FindTableHeadSorted( pszKey, reinterpret_cast<LPCTSTR const *>(sm_szLoadKeys), COUNTOF(sm_szLoadKeys) - 1, sizeof(sm_szLoadKeys[0]) );
	if ( index < 0 )
	{
		if ( !strnicmp( pszKey, "REGEN", 5 ))
		{
			index = ATOI(pszKey+5);
			if (( index < 0 ) || ( index >= STAT_QTY ))
				return false;
			sVal.FormatVal(g_Cfg.m_iRegenRate[index] / TICK_PER_SEC);
			return true;
		}

		if ( !strnicmp( pszKey, "LOOKUPSKILL", 11 ) )
		{
			pszKey	+= 12;
			SKIP_SEPARATORS( pszKey );
			GETNONWHITESPACE( pszKey );

			const CSkillDef *	pSkillDef	= SkillLookup( pszKey );
			if ( !pSkillDef )
				sVal.FormatVal( -1 );
			else
				sVal.FormatVal( pSkillDef->GetResourceID().GetResIndex() );
			return true;
		}

		if ( !strnicmp(pszKey, "MAP(", 4) )
		{
			pszKey += 4;
			TCHAR * pszArgsNext;
			Str_Parse( const_cast<TCHAR*>(pszKey), &(pszArgsNext), ")" );
			sVal.FormatVal(0);

			CPointMap pt;
			if ( IsDigit( pszKey[0] ) || pszKey[0] == '-' )
			{
				pt.m_map = 0; pt.m_z = 0;
				TCHAR * ppVal[4];
				size_t iArgs = Str_ParseCmds( const_cast<TCHAR*>(pszKey), ppVal, COUNTOF( ppVal ), "," );

				switch ( iArgs )
				{
					default:
					case 4:
						if ( IsDigit(ppVal[3][0]) )
						{
							pt.m_map = static_cast<unsigned char>(ATOI(ppVal[3]));
						}
					case 3:
						if ( IsDigit(ppVal[2][0]) || (( iArgs == 4 ) && ( ppVal[2][0] == '-' )) )
						{
							pt.m_z = static_cast<signed char>(( iArgs == 4 ) ? ATOI(ppVal[2]) : 0);
							if ( iArgs == 3 )
							{
								pt.m_map = static_cast<unsigned char>(ATOI(ppVal[2]));
							}
						}
					case 2:
						pt.m_y = static_cast<short>(ATOI(ppVal[1]));
					case 1:
						pt.m_x = static_cast<short>(ATOI(ppVal[0]));
					case 0:
						break;
				}
			}

			pszKey = pszArgsNext;
			SKIP_SEPARATORS(pszKey);
			return pt.r_WriteVal(pszKey, sVal);
		} 

		if ( !strnicmp( pszKey, "MAPLIST.",8) )
		{
			LPCTSTR pszCmd = pszKey + 8;
			int iNumber = Exp_GetVal(pszCmd);
			SKIP_SEPARATORS(pszCmd);
			sVal.FormatVal(0);

			if ( !*pszCmd )
			{
				sVal.FormatVal( g_MapList.IsMapSupported(iNumber) );
			}
			else
			{
				if ( g_MapList.IsMapSupported(iNumber) )
				{
					if (!strcmpi(pszCmd,"BOUND.X"))
						sVal.FormatVal( g_MapList.GetX(iNumber) );
					else if (!strcmpi(pszCmd,"BOUND.Y"))
						sVal.FormatVal( g_MapList.GetY(iNumber) );
					else if (!strcmpi(pszCmd,"CENTER.X"))
						sVal.FormatVal( g_MapList.GetCenterX(iNumber) );
					else if (!strcmpi(pszCmd,"CENTER.Y"))
						sVal.FormatVal( g_MapList.GetCenterY(iNumber) );
					else if (!strcmpi(pszCmd,"SECTOR.SIZE"))
						sVal.FormatVal( g_MapList.GetSectorSize(iNumber) );
					else if (!strcmpi(pszCmd,"SECTOR.ROWS"))
						sVal.FormatVal( g_MapList.GetSectorRows(iNumber) );
					else if (!strcmpi(pszCmd,"SECTOR.COLS"))
						sVal.FormatVal( g_MapList.GetSectorCols(iNumber) );
					else if (!strcmpi(pszCmd,"SECTOR.QTY"))
						sVal.FormatVal( g_MapList.GetSectorQty(iNumber) );
				}
			}

			return true;
		}

		if ( !strnicmp( pszKey, "MAP", 3 ))
		{
			pszKey = pszKey + 3;
			int iMapNumber = Exp_GetVal(pszKey);
			SKIP_SEPARATORS(pszKey);
			sVal.FormatVal(0);

			if ( g_MapList.IsMapSupported(iMapNumber) )
			{
				if ( !strnicmp( pszKey, "SECTOR", 6 ))
				{ 
					pszKey = pszKey + 6;
					int iSecNumber = Exp_GetVal(pszKey);
					SKIP_SEPARATORS(pszKey);
					int nSectors = g_MapList.GetSectorQty(iMapNumber);

					if ((iSecNumber > 0) && (iSecNumber <=  nSectors))
						return( g_World.GetSector(iMapNumber, iSecNumber-1)->r_WriteVal(pszKey, sVal, pSrc) );
					else
					{
						g_Log.EventError("Invalid Sector #%d for Map %d\n", iSecNumber, iMapNumber);
						return false;
					}
				}
			}
			g_Log.EventError("Unsupported Map %d\n", iMapNumber);
			return false;
		} 

		if (!strnicmp( pszKey, "FUNCTIONS.", 10))
		{
			LPCTSTR pszCmd = pszKey + 10;

			if ( !strnicmp( pszCmd, "COUNT", 5 ))
			{
				sVal.FormatVal(static_cast<size_t>(m_Functions.GetCount()));
				return( true );
			}
			else if ( m_Functions.ContainsKey(pszCmd) )
			{
				sVal.FormatVal(static_cast<int>(GetPrivCommandLevel(pszCmd)));
				return true;
			}

			size_t iNumber = Exp_GetVal(pszCmd);
			SKIP_SEPARATORS(pszCmd);
			sVal.FormatVal(0);

			if (iNumber < 0 || iNumber >= m_Functions.GetCount()) //invalid index can potentially crash the server, this check is strongly needed
			{
				g_Log.EventError("Invalid command index %" FMTSIZE_T "\n", iNumber);
				return false;
			}

			if ( *pszCmd == '\0')
			{
				sVal = m_Functions.GetAt(iNumber)->GetName();
				return true;
			}
			else if ( !strnicmp( pszCmd, "PLEVEL", 5 ))
			{
				sVal.FormatVal(static_cast<int>(GetPrivCommandLevel(m_Functions.GetAt(iNumber)->GetName())));
				return true;
			}
		}

		if ( ( !strnicmp( pszKey, "GUILDSTONES.", 12) ) || ( !strnicmp( pszKey, "TOWNSTONES.", 11) ) )
		{
			bool bGuild = !strnicmp( pszKey, "GUILDSTONES.",12);
			LPCTSTR pszCmd = pszKey + 11 + ( (bGuild) ? 1 : 0 );
			CItemStone * pStone = NULL;
			size_t x = 0;

			if (!strcmpi(pszCmd,"COUNT"))
			{
				for ( size_t i = 0; i < g_World.m_Stones.GetCount(); i++ )
				{
					pStone = g_World.m_Stones[i];
					if ( pStone == NULL )
						continue;

					if (( pStone->GetType() == IT_STONE_GUILD ) && bGuild )
						x++;
					else if (( pStone->GetType() == IT_STONE_TOWN ) && !bGuild )
						x++;
				}

				sVal.FormatVal(x);
				return( true );
			}

			size_t iNumber = Exp_GetVal(pszCmd);
			SKIP_SEPARATORS(pszCmd);
			sVal.FormatVal(0);

			for ( size_t i = 0; i < g_World.m_Stones.GetCount(); i++ )
			{
				pStone = g_World.m_Stones[i];
				if ( pStone == NULL )
					continue;

				if (( pStone->GetType() == IT_STONE_GUILD ) && bGuild )
				{
					if ( iNumber == x )
					{
						return( pStone->r_WriteVal(pszCmd, sVal, pSrc) );
					}
					x++;
				}
				else if (( pStone->GetType() == IT_STONE_TOWN ) && !bGuild )
				{
					if ( iNumber == x )
					{
						return( pStone->r_WriteVal(pszCmd, sVal, pSrc) );
					}
					x++;
				}
			}

			return( true );
		}

		if ( !strnicmp( pszKey, "CLIENT.",7))
		{
			pszKey += 7;
			unsigned int cli_num = static_cast<unsigned int>(Exp_GetVal(pszKey));
			unsigned int i = 0;
			SKIP_SEPARATORS(pszKey);

			if (cli_num >= g_Serv.StatGet( SERV_STAT_CLIENTS ))
				return false;

			sVal.FormatVal(0);
			ClientIterator it;
			for (CClient* pClient = it.next(); pClient != NULL; pClient = it.next())
			{
				if ( cli_num == i )
				{
					CChar * pChar = pClient->GetChar();

					if ( *pszKey == '\0' ) // checking reference
						sVal.FormatVal( pChar != NULL? 1 : 0 );
					else if ( pChar != NULL )
						return pChar->r_WriteVal(pszKey, sVal, pSrc);
					else
						return pClient->r_WriteVal(pszKey, sVal, pSrc);
				}
				i++;
			}
			return true;
		}
		return( false );
	}

	switch (index)
	{
		case RC_ATTACKERTIMEOUT:
			sVal.FormatVal(m_iAttackerTimeout);
			break;
		case RC_BANKMAXWEIGHT:
			sVal.FormatVal( m_iBankWMax / WEIGHT_UNITS );
			break;
		case RC_BUILD:
			#ifdef GRAY_VER_BUILD
			 sVal.FormatVal(GRAY_VER_BUILD);
			#else
			 sVal = __DATE__;
			#endif
			break;
		case RC_CLIENTLINGER:
			sVal.FormatVal( m_iClientLingerTime / TICK_PER_SEC );
			break;
		case RC_COLORHIDDEN:
			sVal.FormatHex( m_iColorHidden );
			break;
		case RC_COLORINVIS:
			sVal.FormatHex( m_iColorInvis );
			break;
		case RC_COLORINVISSPELL:
			sVal.FormatHex( m_iColorInvisSpell );
			break;
		case RC_CORPSENPCDECAY:
			sVal.FormatVal( m_iDecay_CorpseNPC / (60*TICK_PER_SEC));
			break;
		case RC_CORPSEPLAYERDECAY:
			sVal.FormatVal( m_iDecay_CorpsePlayer / (60*TICK_PER_SEC));
			break;
		case RC_CRIMINALTIMER:
			sVal.FormatVal( m_iCriminalTimer / (60*TICK_PER_SEC));
			break;
		case RC_DEADSOCKETTIME:
			sVal.FormatVal( m_iDeadSocketTime / (60*TICK_PER_SEC));
			break;
		case RC_DECAYTIMER:
			sVal.FormatVal( m_iDecay_Item / (60*TICK_PER_SEC));
			break;
		case RC_FREEZERESTARTTIME:
			sVal.FormatVal(m_iFreezeRestartTime / TICK_PER_SEC);
			break;
		case RC_GAMEMINUTELENGTH:
			sVal.FormatVal(m_iGameMinuteLength / TICK_PER_SEC);
			break;
		case RC_GUARDLINGER:
			sVal.FormatVal( m_iGuardLingerTime / (60*TICK_PER_SEC));
			break;
		case RC_HEARALL:
			sVal.FormatVal( g_Log.GetLogMask() & LOGM_PLAYER_SPEAK );
			break;
		case RC_HITSUPDATERATE:
			sVal.FormatVal( m_iHitsUpdateRate / TICK_PER_SEC );
			break;
		case RC_LOG:
			sVal = g_Log.GetLogDir();
			break;
		case RC_LOGMASK:
			sVal.FormatHex( g_Log.GetLogMask());
			break;
		case RC_MULFILES:
			sVal = g_Install.GetPreferPath(NULL);
			break;
		case RC_MAPCACHETIME:
			sVal.FormatVal( m_iMapCacheTime / TICK_PER_SEC );
			break;
		case RC_NOTOTIMEOUT:
			sVal.FormatVal(m_iNotoTimeout / TICK_PER_SEC);
			break;
		case RC_MAXFAME:
			sVal.FormatVal( m_iMaxFame );
			break;
		case RC_MAXKARMA:
			sVal.FormatVal( m_iMaxKarma );
			break;
		case RC_MINCHARDELETETIME:
			sVal.FormatVal( m_iMinCharDeleteTime /TICK_PER_SEC );
			break;
		case RC_MINKARMA:
			sVal.FormatVal( m_iMinKarma );
			break;
		case RC_MURDERDECAYTIME:
			sVal.FormatVal( m_iMurderDecayTime / (TICK_PER_SEC ));
			break;
		case RC_WOOLGROWTHTIME:
			sVal.FormatVal( m_iWoolGrowthTime /( 60*TICK_PER_SEC ));
			break;
		case RC_PROFILE:
			sVal.FormatVal(CurrentProfileData.GetActiveWindow());
			break;
		case RC_RTICKS:
			{
				if ( pszKey[6] != '.' )
					sVal.FormatUVal(static_cast<unsigned long>(CGTime::GetCurrentTime().GetTime()));
				else
				{
					pszKey += 6;
					SKIP_SEPARATORS(pszKey);
					if ( !strnicmp("FROMTIME", pszKey, 8) )
					{
						pszKey += 8;
						GETNONWHITESPACE(pszKey);
						INT64 piVal[6];

						// year, month, day, hour, minute, second
						size_t iQty = Str_ParseCmds(const_cast<TCHAR*>(pszKey), piVal, COUNTOF(piVal));
						if ( iQty != 6 )
							return false;

						CGTime datetime(static_cast<int>(piVal[0]), static_cast<int>(piVal[1]), static_cast<int>(piVal[2]), static_cast<int>(piVal[3]), static_cast<int>(piVal[4]), static_cast<int>(piVal[5]));
						if ( datetime.GetTime() == -1 )
							sVal.FormatVal(-1);
						else
							sVal.FormatUVal(static_cast<unsigned long>(datetime.GetTime()));
					}
					else if ( !strnicmp("FORMAT", pszKey, 6) )
					{
						pszKey += 6;
						GETNONWHITESPACE( pszKey );
						TCHAR *ppVal[2];

						// timestamp, formatstr
						size_t iQty = Str_ParseCmds(const_cast<TCHAR*>(pszKey), ppVal, COUNTOF(ppVal));
						if ( iQty < 1 )
							return false;

						time_t lTime = Exp_GetVal(ppVal[0]);

						CGTime datetime(lTime);
						sVal = datetime.Format(iQty > 1? ppVal[1]: NULL);
					}
				}
			}
			break;
		case RC_RTIME:
			{
				if ( pszKey[5] != '.' )
					sVal = CGTime::GetCurrentTime().Format(NULL);
				else
				{
					pszKey += 5;
					SKIP_SEPARATORS(pszKey);
					if (!strnicmp("GMTFORMAT",pszKey,9))
					{
						pszKey += 9;
						GETNONWHITESPACE( pszKey );
						sVal = CGTime::GetCurrentTime().FormatGmt(pszKey);
					}
					if (!strnicmp("FORMAT",pszKey,6))
					{
						pszKey += 6;
						GETNONWHITESPACE( pszKey );
						sVal = CGTime::GetCurrentTime().Format(pszKey);
					}
				}
			} break;
		case RC_SAVEPERIOD:
			sVal.FormatVal( m_iSavePeriod / (60*TICK_PER_SEC));
			break;
		case RC_SECTORSLEEP:
			sVal.FormatVal(m_iSectorSleepMask ? (Calc_GetLog2(m_iSectorSleepMask+1)-1) : 0);
			break;
		case RC_SAVEBACKGROUND:
			sVal.FormatVal( m_iSaveBackgroundTime / (60 * TICK_PER_SEC));
			break;
		case RC_SAVESECTORSPERTICK:
			sVal.FormatVal(m_iSaveSectorsPerTick);
			break;
		case RC_SAVESTEPMAXCOMPLEXITY:
			sVal.FormatVal(m_iSaveStepMaxComplexity);
			break;
		case RC_SPELLTIMEOUT:
			sVal.FormatVal(m_iSpellTimeout / TICK_PER_SEC);
			break;
		case RC_GUILDS:
			sVal.FormatVal( g_World.m_Stones.GetCount());
			return( true );
		case RC_TIMEUP:
			sVal.FormatLLVal( ( - g_World.GetTimeDiff( g_World.m_timeStartup )) / TICK_PER_SEC );
			return( true );
		case RC_TIMERCALL:
			sVal.FormatVal(m_iTimerCall / (60 * TICK_PER_SEC));
			break;
		case RC_VERSION:
			sVal = g_szServerDescription;
			break;
		case RC_EXPERIMENTAL:
			sVal.FormatHex( g_Cfg.m_iExperimental );
			//PrintEFOFFlags(true, false, pSrc);
			break;
		case RC_OPTIONFLAGS:
			sVal.FormatHex( g_Cfg.m_iOptionFlags );
			//PrintEFOFFlags(false, true, pSrc);
			break;
		case RC_CLIENTS:		// this is handled by CServerDef as SV_CLIENTS
			return false;
		case RC_PLAYEREVIL:
			sVal.FormatVal( m_iPlayerKarmaEvil );
			break;
		case RC_PLAYERNEUTRAL:
			sVal.FormatVal( m_iPlayerKarmaNeutral );
			break;
		case RC_TOOLTIPCACHE:
			sVal.FormatVal( m_iTooltipCache / (TICK_PER_SEC) );
			break;

		case RC_GUARDSINSTANTKILL:
			sVal.FormatVal(g_Cfg.m_fGuardsInstantKill);
			break;

		case RC_GUARDSONMURDERERS:
			sVal.FormatVal(g_Cfg.m_fGuardsOnMurderers);
			break;

		case RC_CONTEXTMENULIMIT:
			sVal.FormatVal(g_Cfg.m_iContextMenuLimit);
			break;
		case RC_WALKBUFFER:
			sVal.FormatVal(m_iWalkBuffer / TICK_PER_SEC);
			break;
		default:
			return( sm_szLoadKeys[index].m_elem.GetValStr( this, sVal ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

//*************************************************************

bool CResource::IsConsoleCmd( TCHAR ch ) const
{
	ADDTOCALLSTACK("CResource::IsConsoleCmd");
	return (ch == '.' || ch == '/' );
}

SKILL_TYPE CResource::FindSkillKey( LPCTSTR pszKey ) const
{
	ADDTOCALLSTACK("CResource::FindSkillKey");
	// Find the skill name in the alpha sorted list.
	// RETURN: SKILL_NONE = error.

	if ( IsDigit( pszKey[0] ))
	{
		SKILL_TYPE skill = static_cast<SKILL_TYPE>(Exp_GetVal(pszKey));
		if ( (!CChar::IsSkillBase(skill) || !g_Cfg.m_SkillIndexDefs.IsValidIndex(skill)) &&
			! CChar::IsSkillNPC(skill))
		{
			return( SKILL_NONE );
		}
		return( skill );
	}

	const CSkillDef * pSkillDef = FindSkillDef( pszKey );
	if ( pSkillDef == NULL )
		return( SKILL_NONE );
	return static_cast<SKILL_TYPE>(pSkillDef->GetResourceID().GetResIndex());
}

STAT_TYPE CResource::FindStatKey( LPCTSTR pszKey ) // static
{
	ADDTOCALLSTACK("CResource::FindStatKey");
	return (STAT_TYPE) FindTable( pszKey, g_Stat_Name, COUNTOF( g_Stat_Name ));
}

int CResource::GetSpellEffect( SPELL_TYPE spell, int iSkillVal ) const
{
	ADDTOCALLSTACK("CResource::GetSpellEffect");
	// NOTE: Any randomizing of the effect must be done by varying the skill level .
	// iSkillVal = 0-1000
	if ( !spell )
		return 0;
	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef( spell );
	if ( pSpellDef == NULL )
		return( 0 );
	return( pSpellDef->m_Effect.GetLinear( iSkillVal ));
}

LPCTSTR CResource::GetNotoTitle( int iLevel, bool bFemale ) const
{
	ADDTOCALLSTACK("CResource::GetNotoTitle");
	// Retrieve the title used for the given noto level and gender

	if ( !m_NotoTitles.IsValidIndex(iLevel) )
	{
		return "";
	}
	else
	{
		// check if a female title is present
		LPCTSTR pFemaleTitle = strchr(m_NotoTitles[iLevel]->GetPtr(), ',');
		if (pFemaleTitle == NULL)
			return m_NotoTitles[iLevel]->GetPtr();

		pFemaleTitle++;
		if (bFemale)
			return pFemaleTitle;
		
		// copy string so that it can be null-terminated without modifying m_NotoTitles
		TCHAR* pTitle = Str_GetTemp();
		strcpylen(pTitle, m_NotoTitles[iLevel]->GetPtr(), m_NotoTitles[iLevel]->GetLength() - strlen(pFemaleTitle));
		return pTitle;
	}
}

bool CResource::IsValidEmailAddressFormat( LPCTSTR pszEmail ) // static
{
	ADDTOCALLSTACK("CResource::IsValidEmailAddressFormat");
	// what are the invalid email name chars ?
	// Valid characters are, numbers, letters, underscore "_", dash "-" and the dot ".").

	size_t len1 = strlen( pszEmail );
	if ( len1 <= 0 || len1 > 128 )
		return( false );

	TCHAR szEmailStrip[256];
	size_t len2 = Str_GetBare( szEmailStrip, pszEmail,
		sizeof(szEmailStrip),
		" !\"#%&()*,/:;<=>?[\\]^{|}'`+" );
	if ( len2 != len1 )
		return( false );

	TCHAR * pszAt = const_cast<TCHAR*>(strchr( pszEmail, '@' ));
	if ( ! pszAt )
		return( false );
	if ( pszAt == pszEmail )
		return( false );
	if ( ! strchr( pszAt, '.' ))
		return( false );

	return( true );
}

CServerRef CResource::Server_GetDef( size_t index )
{
	ADDTOCALLSTACK("CResource::Server_GetDef");
	if ( ! m_Servers.IsValidIndex(index))
		return( NULL );
	return CServerRef(static_cast<CServerDef *>(m_Servers[index]));
}

CWebPageDef * CResource::FindWebPage( LPCTSTR pszPath ) const
{
	ADDTOCALLSTACK("CResource::FindWebPage");
	if ( pszPath == NULL )
	{
		if ( m_WebPages.GetCount() <= 0 )
			return( NULL );
		// Take this as the default page.
		return static_cast<CWebPageDef *>(m_WebPages[0]);
	}

	LPCTSTR pszTitle = CGFile::GetFilesTitle(pszPath);

	if ( pszTitle == NULL || pszTitle[0] == '\0' )
	{
		// This is just the root index.
		if ( m_WebPages.GetCount() <= 0 )
			return( NULL );
		// Take this as the default page.
		return static_cast<CWebPageDef *>(m_WebPages[0]);
	}

	for ( size_t i = 0; i < m_WebPages.GetCount(); i++ )
	{
		if ( m_WebPages[i] == NULL )	// not sure why this would happen
			continue;
		CWebPageDef *pWeb = static_cast<CWebPageDef *>(m_WebPages[i]);
		ASSERT(pWeb);
		if ( pWeb->IsMatch(pszTitle))
			return( pWeb );
	}

	return( NULL );
}

bool CResource::IsObscene( LPCTSTR pszText ) const
{
	ADDTOCALLSTACK("CResource::IsObscene");
	// does this text contain obscene content?
	// NOTE: allow partial match syntax *fuck* or ass (alone)

	for ( size_t i = 0; i < m_Obscene.GetCount(); i++ )
	{
		TCHAR* match = new TCHAR[ strlen(m_Obscene[i])+3 ];
		sprintf(match,"%s%s%s","*",m_Obscene[i],"*");
		MATCH_TYPE ematch = Str_Match( match , pszText );
		delete[] match;

		if ( ematch == MATCH_VALID )
			return( true );
	}
	return( false );
}

bool CResource::SetKRDialogMap(DWORD rid, DWORD idKRDialog)
{
	ADDTOCALLSTACK("CResource::SetKRDialogMap");
	// Defines a link between the given ResourceID and KR DialogID, so that
	// the dialogs of KR clients can be handled in scripts.
	KRGumpsMap::iterator it;

	// prevent double mapping of same dialog
	it = m_mapKRGumps.find(rid);
	if ( it != m_mapKRGumps.end() )
	{
		if ( it->second == idKRDialog )	// already mapped to this kr dialog
			return true;

		g_Log.Event( LOGL_WARN, "Dialog '%s' is already mapped to KR dialog '%lu'.\n", ResourceGetName(RESOURCE_ID(RES_DIALOG, rid)), it->second);
	}

	// prevent double mapping of KR dialog
	for (it = m_mapKRGumps.begin(); it != m_mapKRGumps.end(); ++it)
	{
		if (it->second != idKRDialog)
			continue;

		DEBUG_ERR(("KR Dialog '%lu' is already mapped to dialog '%s'.\n", idKRDialog, ResourceGetName(RESOURCE_ID(RES_DIALOG, it->first))));
		return false;
	}

	m_mapKRGumps[rid] = idKRDialog;
	return true;
}

DWORD CResource::GetKRDialogMap(DWORD idKRDialog)
{
	ADDTOCALLSTACK("CResource::GetKRDialogMap");
	// Translates the given KR DialogID into the ResourceID of its scripted dialog.
	// Returns 0 on failure
	for (KRGumpsMap::iterator it = m_mapKRGumps.begin(); it != m_mapKRGumps.end(); ++it)
	{
		if (it->second != idKRDialog)
			continue;

		return it->first;
	}

	return 0;
}

DWORD CResource::GetKRDialog(DWORD rid)
{
	ADDTOCALLSTACK("CResource::GetKRDialog");
	// Translates the given ResourceID into it's equivalent KR DialogID.
	// Returns 0 on failure
	KRGumpsMap::iterator it = m_mapKRGumps.find(rid);
	if (it != m_mapKRGumps.end())
		return it->second;

	return 0;
}

const CGrayMulti * CResource::GetMultiItemDefs( CItem * pItem )
{
	ADDTOCALLSTACK("CResource::GetMultiItemDefs(CItem*)");
	if ( !pItem )
		return NULL;

	CItemMultiCustom *pItemMultiCustom = dynamic_cast<CItemMultiCustom*>(pItem);
	if ( pItemMultiCustom )
		return pItemMultiCustom->GetMultiItemDefs();	// customized multi

	return GetMultiItemDefs(pItem->GetDispID());		// multi.mul multi
}

const CGrayMulti * CResource::GetMultiItemDefs( ITEMID_TYPE itemid )
{
	ADDTOCALLSTACK("CResource::GetMultiItemDefs(ITEMID_TYPE)");
	if ( !CItemBase::IsID_Multi(itemid) )
		return NULL;

	MULTI_TYPE id = static_cast<MULTI_TYPE>(itemid - ITEMID_MULTI);
	size_t index = m_MultiDefs.FindKey(id);
	if ( index == m_MultiDefs.BadIndex() )
		index = m_MultiDefs.AddSortKey(new CGrayMulti(id), id);
	else
		m_MultiDefs[index]->HitCacheTime();

	const CGrayMulti *pMulti = m_MultiDefs[index];
	return pMulti;
}

PLEVEL_TYPE CResource::GetPrivCommandLevel( LPCTSTR pszCmd ) const
{
	ADDTOCALLSTACK("CResource::GetPrivCommandLevel");
	// What is this commands plevel ?
	// NOTE: This does not attempt to parse anything.

	// Is this command avail for your priv level (or lower) ?
	unsigned int ilevel = PLEVEL_QTY;
	while ( ilevel > 0 )
	{
		--ilevel;
		LPCTSTR const * pszTable = m_PrivCommands[ilevel].GetBasePtr();
		size_t iCount = m_PrivCommands[ilevel].GetCount();
		if ( FindTableHeadSorted( pszCmd, pszTable, iCount ) >= 0 )
			return( static_cast<PLEVEL_TYPE>(ilevel) );
	}

	// A GM will default to use all commands.
	// xcept those that are specifically named that i can't use.
	return ( static_cast<PLEVEL_TYPE>(m_iDefaultCommandLevel) ); // default level.
}

bool CResource::CanUsePrivVerb( const CScriptObj * pObjTarg, LPCTSTR pszCmd, CTextConsole * pSrc ) const
{
	ADDTOCALLSTACK("CResource::CanUsePrivVerb");
	// can i use this verb on this object ?
	// Check just at entry points where commands are entered or targetted.
	// NOTE:
	//  Call this each time we change pObjTarg such as r_GetRef()
	// RETURN:
	//  true = i am ok to use this command.

	if ( !pSrc || !pObjTarg )
		return false;

	// Are they more privleged than me ?

	const CChar * pChar = dynamic_cast <const CChar*> (pObjTarg);
	if ( pChar )
	{
		if ( pSrc->GetChar() == pChar )
			return( true );
		if ( pSrc->GetPrivLevel() < pChar->GetPrivLevel())
		{
			pSrc->SysMessage( "Target is more privileged than you\n" );
			return false;
		}
	}

	// can i use this verb on this object ?

	if ( pSrc->GetChar() == NULL )
	{
		// I'm not a cchar. what am i ?
		CClient * pClient = dynamic_cast <CClient *>(pSrc);
		if ( pClient )
		{
			// We are not logged in as a player char ? so we cannot do much !
			if ( !pClient->m_pAccount )
			{
				// must be a console or web page ?
				// I guess we can just login !
				if ( ! strnicmp( pszCmd, "LOGIN", 5 ))
					return( true );
				return( false );
			}
		}
		else
		{
			// we might be the g_Serv or web page ?
		}
	}

	// Is this command avail for your priv level (or lower) ?
	PLEVEL_TYPE ilevel;
	TCHAR *myCmd = Str_GetTemp();

	size_t pOs = strcspn(pszCmd," "); //position of space :)
	strncpy ( myCmd, pszCmd, pOs );
	myCmd[pOs] = '\0';

	TCHAR * pOd; //position of dot :)
	while ( (pOd = strchr(myCmd, '.')) != NULL )
	{
		ilevel = GetPrivCommandLevel( myCmd );
		if ( ilevel > pSrc->GetPrivLevel())
			return( false );
		myCmd = pOd + 1; //skip dot
	}


	ilevel = GetPrivCommandLevel( myCmd );
	if ( ilevel > pSrc->GetPrivLevel())
		return( false );

	return( true );
}

//*************************************************************

CPointMap CResource::GetRegionPoint( LPCTSTR pCmd ) const // Decode a teleport location number into X/Y/Z
{
	ADDTOCALLSTACK("CResource::GetRegionPoint");
	// get a point from a name. (probably the name of a region)
	// Might just be a point coord number ?

	GETNONWHITESPACE( pCmd );
	if ( pCmd[0] == '-' && !strchr( pCmd, ',' ) )	// Get location from start list.
	{
		size_t i = ( - ATOI(pCmd)) - 1;
		if ( ! m_StartDefs.IsValidIndex( i ))
		{
			if ( m_StartDefs.GetCount() <= 0 )
				return CPointMap();

			i = 0;
		}
		return( m_StartDefs[i]->m_pt );
	}

	CPointMap pt;	// invalid point
	if ( IsDigit( pCmd[0] ) || pCmd[0] == '-' )
	{
		TCHAR *pszTemp = Str_GetTemp();
		strcpy( pszTemp, pCmd );
		size_t iCount = pt.Read( pszTemp );
		if ( iCount >= 2 )
		{
			return( pt );
		}
	}
	else
	{
		// Match the region name with global regions.
		CRegionBase * pRegion = GetRegion(pCmd);
		if ( pRegion != NULL )
		{
			return( pRegion->m_pt );
		}
	}
	// no match.
	return( pt );
}

CRegionBase * CResource::GetRegion( LPCTSTR pKey ) const
{
	ADDTOCALLSTACK("CResource::GetRegion");
	// get a region from a name or areadef.

	GETNONWHITESPACE( pKey );
	for ( size_t i = 0; i < COUNTOF(m_ResHash.m_Array); i++ )
	{
		for ( size_t j = 0; j < m_ResHash.m_Array[i].GetCount(); j++ )
		{
			CResourceDef * pResDef = m_ResHash.m_Array[i][j];
			ASSERT(pResDef);

			CRegionBase * pRegion = dynamic_cast <CRegionBase*> (pResDef);
			if ( pRegion == NULL )
				continue;

			if ( ! pRegion->GetNameStr().CompareNoCase( pKey ) ||
				! strcmpi( pRegion->GetResourceName(), pKey ))
			{
				return( pRegion );
			}
		}
	}

	// no match.
	return( NULL );
}

void CResource::LoadSortSpells()
{
	size_t iQtySpells = m_SpellDefs.GetCount();
	if ( iQtySpells <= 0 )
		return;

	m_SpellDefs_Sorted.RemoveAll();
	m_SpellDefs_Sorted.Add( m_SpellDefs[0] );		// the null spell

	for ( size_t i = 1; i < iQtySpells; i++ )
	{
		if ( !m_SpellDefs.IsValidIndex( i ) )
			continue;

		int	iVal = 0;
		m_SpellDefs[i]->GetPrimarySkill( NULL, &iVal );

		size_t iQty = m_SpellDefs_Sorted.GetCount();
		size_t k = 1;
		while ( k < iQty )
		{
			int	iVal2 = 0;
			m_SpellDefs_Sorted[k]->GetPrimarySkill( NULL, &iVal2 );
			if ( iVal2 > iVal )
				break;
			++k;
		}
		m_SpellDefs_Sorted.InsertAt( k, m_SpellDefs[i] );
	}
}


//*************************************************************

bool CResource::LoadResourceSection( CScript * pScript )
{
	ADDTOCALLSTACK("CResource::LoadResourceSection");
	bool	fNewStyleDef	= false;

	// Index or read any resource blocks we know how to handle.
	ASSERT(pScript);
	CScriptFileContext FileContext( pScript );	// set this as the context.
	CVarDefContNum * pVarNum = NULL;
	RESOURCE_ID rid;
	LPCTSTR		pszSection	= pScript->GetSection();

	RES_TYPE restype;
	if ( !strnicmp( pszSection, "DEFMESSAGE", 10 ) )
	{
		restype			= RES_DEFNAME;
		fNewStyleDef	= true;
	}
	else if ( !strnicmp( pszSection, "AREADEF", 7 ) )
	{
		restype			= RES_AREA;
		fNewStyleDef	= true;
		if ( g_Serv.m_fResyncPause )
			g_Serv.m_fResyncMultiRegions = true;
	}
	else if ( !strnicmp( pszSection, "ROOMDEF", 7 ) )
	{
		restype			= RES_ROOM;
		fNewStyleDef	= true;
		if ( g_Serv.m_fResyncPause )
			g_Serv.m_fResyncMultiRegions = true;
	}
	else if ( !strnicmp( pszSection, "GLOBALS", 7 ) )
	{
		restype			= RES_WORLDVARS;
	}
	else if ( !strnicmp( pszSection, "LIST", 4) )
	{
		restype			= RES_WORLDLISTS;
	}
	else if ( !strnicmp( pszSection, "TIMERF", 7 ) )
	{
		restype			= RES_TIMERF;
	}
	else if ( !strnicmp( pszSection, "FAME", 7 ) )
	{
		restype			= RES_FAME;
	}
	else if ( !strnicmp( pszSection, "KARMA", 7 ) )
	{
		restype			= RES_KARMA;
	}
	else if ( !strnicmp( pszSection, "MULTIDEF", 8 ) )
	{
		restype			= RES_ITEMDEF;
		fNewStyleDef	= true;
	}
	else
		restype	= (RES_TYPE) FindTableSorted( pszSection, sm_szResourceBlocks, COUNTOF( sm_szResourceBlocks ));


	if (( restype == RES_WORLDSCRIPT ) || ( restype == RES_WS ))
	{
		LPCTSTR	pszDef = pScript->GetArgStr();
		CVarDefCont * pVarBase = g_Exp.m_VarDefs.GetKey( pszDef );
		pVarNum = NULL;
		if ( pVarBase )
			pVarNum = static_cast<CVarDefContNum *>(pVarBase);
		if ( !pVarNum )
		{
			g_Log.Event( LOGL_WARN|LOGM_INIT, "Resource '%s' not found\n", pszDef );
			return false;
		}

		rid.SetPrivateUID( static_cast<DWORD>(pVarNum->GetValNum()) );
		restype	= rid.GetResType();

		CResourceDef *pRes = NULL;
		size_t index = m_ResHash.FindKey(rid);
		if ( index != m_ResHash.BadIndex() )
			pRes = m_ResHash.GetAt(rid, index);

		if ( pRes == NULL )
		{
			g_Log.Event( LOGL_WARN|LOGM_INIT, "Resource '%s' not found\n", pszDef );
			return false;
		}

		pRes->r_Load( *pScript );
		return true;
	}

	if ( restype < 0 )
	{
		g_Log.Event( LOGL_WARN|LOGM_INIT, "Unknown section '%s' in '%s'\n", static_cast<LPCTSTR>(pScript->GetKey()), static_cast<LPCTSTR>(pScript->GetFileTitle()));
		return false;
	}
	else
	{
		// Create a new index for the block.
		// NOTE: rid is not created for all types.
		// NOTE: GetArgStr() is not always the DEFNAME
		rid = ResourceGetNewID( restype, pScript->GetArgStr(), &pVarNum, fNewStyleDef );
	}

	if ( !rid.IsValidUID() )
	{
		DEBUG_ERR(( "Invalid %s block index '%s'\n", pszSection, static_cast<LPCTSTR>(pScript->GetArgStr())));
		return( false );
	}

	EXC_TRY("LoadResourceSection");

	// NOTE: It is possible to be replacing an existing entry !!! Check for this.

	CResourceLink * pNewLink = NULL;
	CResourceDef * pNewDef = NULL;
	CResourceDef * pPrvDef = NULL;

	if ( m_ResourceList.ContainsKey( const_cast<TCHAR *>(pszSection) ))
	{

		CListDefCont* pListBase = g_Exp.m_ListInternals.GetKey(pszSection);
			if ( !pListBase )
				pListBase = g_Exp.m_ListInternals.AddList(pszSection);

			if ( pListBase )
				pListBase->r_LoadVal(pScript->GetArgStr());
	}

	switch ( restype )
	{
	case RES_SPHERE:
		// Define main global config info.
		g_Serv.r_Load(*pScript);
		return( true );

	case RES_SPHERECRYPT:
		CCrypt::LoadKeyTable(*pScript);
		return( true );

	case RES_ACCOUNT:	// NOTE: ArgStr is not the DEFNAME
		// Load the account now. Not normal method !
		return g_Accounts.Account_Load( pScript->GetArgStr(), *pScript, false );

	case RES_ADVANCE:
		// Stat advance rates.
		while ( pScript->ReadKeyParse())
		{
			int i = FindStatKey( pScript->GetKey());
			if ( i >= STAT_BASE_QTY )
				continue;
			m_StatAdv[i].Load( pScript->GetArgStr());
		}
		return( true );

	case RES_BLOCKIP:
		{
			TCHAR* ipBuffer = Str_GetTemp();
			while ( pScript->ReadKeyParse())
			{
				strcpy(ipBuffer, pScript->GetKey());
#ifndef _MTNETWORK
				HistoryIP& history = g_NetworkIn.getIPHistoryManager().getHistoryForIP(ipBuffer);
#else
				HistoryIP& history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(ipBuffer);
#endif
				history.setBlocked(true);
			}
		}
		return( true );

	case RES_COMMENT:
		// Just toss this.
		return( true );

	case RES_DEFNAME:
		// just get a block of defs.
		while ( pScript->ReadKeyParse())
		{
			LPCTSTR	pszKey = pScript->GetKey();
			if ( fNewStyleDef )
			{
				//	search for this.
				size_t l;
				for ( l = 0; l < DEFMSG_QTY; l++ )
				{
					if ( !strcmpi(pszKey, g_Exp.sm_szMsgNames[l]) )
					{
						strcpy(g_Exp.sm_szMessages[l], pScript->GetArgStr());
						break;
					}
				}
				if ( l == DEFMSG_QTY )
					g_Log.Event(LOGM_INIT|LOGL_ERROR, "Setting not used message override named '%s'\n", pszKey);
				continue;
			}
			else
			{
				g_Exp.m_VarDefs.SetStr(pszKey, false, pScript->GetArgStr());
			}
		}
		return( true );
	case RES_RESOURCELIST:
		{
			while ( pScript->ReadKey())
			{
				LPCTSTR pName = pScript->GetKeyBuffer();	
				m_ResourceList.AddSortString( pName );
			}
		}
		return( true );
	case RES_FAME:
		{
			size_t i = 0;
			while ( pScript->ReadKey())
			{
				LPCTSTR pName = pScript->GetKeyBuffer();
				if ( *pName == '<' )
					pName = "";
				
				m_Fame.SetAtGrow( i, new CGString(pName) );
				++i;
			}
		}
		return( true );
	case RES_KARMA:
		{
			size_t i = 0;
			while ( pScript->ReadKey())
			{
				LPCTSTR pName = pScript->GetKeyBuffer();
				if ( *pName == '<' )
					pName = "";
				
				m_Karma.SetAtGrow( i, new CGString(pName) );
				++i;
			}
		}
		return( true );
	case RES_NOTOTITLES:
		{
			if (pScript->ReadKey() == false)
			{
				g_Log.Event(LOGM_INIT|LOGL_ERROR, "NOTOTITLES section is missing the list of karma levels.\n");
				return true;
			}

			INT64 piNotoLevels[64];
			size_t i = 0, iQty = 0;

			// read karma levels
			iQty = Str_ParseCmds(pScript->GetKeyBuffer(), piNotoLevels, COUNTOF(piNotoLevels));
			for (i = 0; i < iQty; i++)
				m_NotoKarmaLevels.SetAtGrow(i, static_cast<int>(piNotoLevels[i]));

			m_NotoKarmaLevels.SetCount(i);

			if (pScript->ReadKey() == false)
			{
				g_Log.Event(LOGM_INIT|LOGL_ERROR, "NOTOTITLES section is missing the list of fame levels.\n");
				return true;
			}

			// read fame levels
			iQty = Str_ParseCmds(pScript->GetKeyBuffer(), piNotoLevels, COUNTOF(piNotoLevels));
			for (i = 0; i < iQty; i++)
				m_NotoFameLevels.SetAtGrow(i, static_cast<int>(piNotoLevels[i]));

			m_NotoFameLevels.SetCount(i);

			// read noto titles
			i = 0;
			while ( pScript->ReadKey())
			{
				LPCTSTR pName = pScript->GetKeyBuffer();
				if ( *pName == '<' )
					pName = "";

				m_NotoTitles.SetAtGrow( i, new CGString(pName) );
				++i;
			}

			if (m_NotoTitles.GetCount() != ((m_NotoKarmaLevels.GetCount() + 1) * (m_NotoFameLevels.GetCount() + 1)))
				g_Log.Event(LOGM_INIT|LOGL_WARN, "Expected %" FMTSIZE_T " titles in NOTOTITLES section but found %" FMTSIZE_T ".\n", (m_NotoKarmaLevels.GetCount() + 1) * (m_NotoFameLevels.GetCount() + 1), m_NotoTitles.GetCount());
		}
		return( true );
	case RES_OBSCENE:
		while ( pScript->ReadKey())
		{
			m_Obscene.AddSortString( pScript->GetKey() );
		}
		return( true );
	case RES_PLEVEL:
		{
			int index = rid.GetResIndex();
			if ( index < 0 || static_cast<unsigned int>(index) >= COUNTOF(m_PrivCommands) )
				return false;
			while ( pScript->ReadKey() )
			{
				LPCTSTR	key = pScript->GetKey();
				m_PrivCommands[index].AddSortString(key);
			}
		}
		return true;
	case RES_RESOURCES:
		// Add these all to the list of files we need to include.
		while ( pScript->ReadKey())
		{
			AddResourceFile( pScript->GetKey());
		}
		return( true );
	case RES_RUNES:
		// The full names of the magic runes.
		m_Runes.RemoveAll();
		while ( pScript->ReadKey())
		{
			m_Runes.Add( new CGString(pScript->GetKey()) );
		}
		return( true );
	case RES_SECTOR: // saved in world file.
		{
			CPointMap pt = GetRegionPoint( pScript->GetArgStr() ); // Decode a teleport location number into X/Y/Z
			return( pt.GetSector()->r_Load(*pScript));
		}
	case RES_SPELL:
		{
			CSpellDef * pSpell;
			pPrvDef = ResourceGetDef( rid );
			if ( pPrvDef )
			{
				pSpell = dynamic_cast<CSpellDef*>(pPrvDef);
			}
			else
			{
				pSpell = new CSpellDef(static_cast<SPELL_TYPE>(rid.GetResIndex()));
			}
			ASSERT(pSpell);
			pNewLink = pSpell;

			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load(*pScript);
			pScript->SeekContext( LineContext );

			if ( !pPrvDef )
			{
				m_SpellDefs.SetAtGrow( rid.GetResIndex(), pSpell );
			}
		}
		break;

	case RES_SKILL:
		{
			CSkillDef * pSkill;
			pPrvDef = ResourceGetDef( rid );
			if ( pPrvDef )
			{
				pSkill = dynamic_cast <CSkillDef*>(pPrvDef);
			}
			else
			{
				if ( rid.GetResIndex() >= static_cast<int>(g_Cfg.m_iMaxSkill) )
					g_Cfg.m_iMaxSkill = rid.GetResIndex() + 1;

				// Just replace any previous CSkillDef
				pSkill = new CSkillDef(static_cast<SKILL_TYPE>(rid.GetResIndex()));
			}

			ASSERT(pSkill);
			pNewLink = pSkill;

			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load(*pScript);
			pScript->SeekContext( LineContext );

			if ( !pPrvDef )
			{
				// build a name sorted list.
				m_SkillNameDefs.AddSortKey( pSkill, pSkill->GetKey());
				// Hard coded value for skill index.
				m_SkillIndexDefs.SetAtGrow( rid.GetResIndex(), pSkill );
			}
		}
		break;

	case RES_TYPEDEF:
	{
		// Just index this for access later.
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef )
		{
			CItemTypeDef * pTypeDef	= dynamic_cast <CItemTypeDef*>(pPrvDef);
			ASSERT( pTypeDef );
			pNewLink = pTypeDef;
			ASSERT(pNewLink);

			// clear old tile links to this type
			size_t iQty = g_World.m_TileTypes.GetCount();
			for ( size_t i = 0; i < iQty; i++ )
			{
				if ( g_World.m_TileTypes.GetAt(i) == pTypeDef )
					g_World.m_TileTypes.SetAt( i, NULL );
			}

		}
		else
		{
			pNewLink = new CItemTypeDef( rid );
			ASSERT(pNewLink);
			m_ResHash.AddSortKey( rid, pNewLink );
		}

		CScriptLineContext LineContext = pScript->GetContext();
		pNewLink->r_Load(*pScript);
		pScript->SeekContext( LineContext );
		break;
	}

	//*******************************************************************
	// Might need to check if the link already exists ?

	case RES_BOOK:
	case RES_EVENTS:
	case RES_MENU:
	case RES_NAMES:
	case RES_NEWBIE:
	case RES_TIP:
	case RES_SPEECH:
	case RES_SCROLL:
	case RES_SKILLMENU:
		// Just index this for access later.
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast <CResourceLink*>(pPrvDef);
			ASSERT(pNewLink);
		}
		else
		{
			pNewLink = new CResourceLink( rid );
			ASSERT(pNewLink);
			m_ResHash.AddSortKey( rid, pNewLink );
		}
		break;
	case RES_DIALOG:
		// Just index this for access later.
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast <CDialogDef*>(pPrvDef);
			ASSERT(pNewLink);
		}
		else
		{
			pNewLink = new CDialogDef( rid );
			ASSERT(pNewLink);
			m_ResHash.AddSortKey( rid, pNewLink );
		}
		break;

	case RES_REGIONRESOURCE:
		// No need to Link to this really .
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast <CRegionResourceDef*>( pPrvDef );
			ASSERT(pNewLink);
		}
		else
		{
			pNewLink = new CRegionResourceDef( rid );
			ASSERT(pNewLink);
			m_ResHash.AddSortKey( rid, pNewLink );
		}
		{
			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load(*pScript);
			pScript->SeekContext( LineContext ); // set the pos back so ScanSection will work.
		}

		break;
	case RES_AREA:
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef && fNewStyleDef )
		{
			CRegionWorld *	pRegion = dynamic_cast <CRegionWorld*>( pPrvDef );
			pNewDef	= pRegion;
			ASSERT(pNewDef);
			pRegion->UnRealizeRegion();
			pRegion->r_Load(*pScript);
			pRegion->RealizeRegion();
		}
		else
		{
			CRegionWorld * pRegion = new CRegionWorld( rid, pScript->GetArgStr());
			pNewDef = pRegion;
			ASSERT(pNewDef);
			pRegion->r_Load( *pScript );
			if ( ! pRegion->RealizeRegion() )
				delete pRegion; // might be a dupe ?
			else
			{
				m_ResHash.AddSortKey( rid, pRegion );
				// if it's old style but has a defname, it's already set via r_Load,
				// so this will do nothing, which is good
				// if ( !fNewStyleDef )
				//	pRegion->MakeRegionName();
				m_RegionDefs.Add( pRegion );
			}
		}
		break;
	case RES_ROOM:
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef && fNewStyleDef )
		{
			CRegionBase *	pRegion = dynamic_cast <CRegionBase*>( pPrvDef );
			pNewDef	= pRegion;
			ASSERT(pNewDef);
			pRegion->UnRealizeRegion();
			pRegion->r_Load(*pScript);
			pRegion->RealizeRegion();
		}
		else
		{
			CRegionBase * pRegion = new CRegionBase( rid, pScript->GetArgStr());
			pNewDef = pRegion;
			ASSERT(pNewDef);
			pRegion->r_Load(*pScript);
			if ( !pRegion->RealizeRegion() )
				delete pRegion; // might be a dupe ?
			else
			{
				m_ResHash.AddSortKey( rid, pRegion );
				// if it's old style but has a defname, it's already set via r_Load,
				// so this will do nothing, which is good
				// if ( !fNewStyleDef )
				//	pRegion->MakeRegionName();
				m_RegionDefs.Add( pRegion );
			}
		}
		break;
	case RES_REGIONTYPE:
	case RES_SPAWN:
	case RES_TEMPLATE:
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast <CRandGroupDef*>(pPrvDef);
			ASSERT(pNewLink);
		}
		else
		{
			pNewLink = new CRandGroupDef( rid );
			ASSERT(pNewLink);
			m_ResHash.AddSortKey( rid, pNewLink );
		}
		{
			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load( *pScript );
			pScript->SeekContext( LineContext );
		}
		break;

	case RES_SKILLCLASS:
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast <CSkillClassDef*>(pPrvDef);
			ASSERT(pNewLink);
		}
		else
		{
			pNewLink = new CSkillClassDef( rid );
			ASSERT(pNewLink);
			m_ResHash.AddSortKey( rid, pNewLink );
		}
		{
			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load( *pScript );
			pScript->SeekContext( LineContext );
		}
		break;

	case RES_CHARDEF:
	case RES_ITEMDEF:
		// ??? existing hard pointers to RES_CHARDEF ?
		// ??? existing hard pointers to RES_ITEMDEF ?
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast<CResourceLink*>(pPrvDef);
			if ( !pNewLink )
				return true;

			CBaseBaseDef * pBaseDef = dynamic_cast <CBaseBaseDef*> (pNewLink);
			if ( pBaseDef )
			{
				pBaseDef->UnLink();
				CScriptLineContext LineContext = pScript->GetContext();
				pBaseDef->r_Load(*pScript);
				pScript->SeekContext( LineContext );
			}
		}
		else
		{
			pNewLink = new CResourceLink(rid);
			ASSERT(pNewLink);
			m_ResHash.AddSortKey( rid, pNewLink );
		}
		break;

	// Map stuff that could be duplicated !!!
	// NOTE: ArgStr is NOT the DEFNAME in this case
	// ??? duplicate areas ?
	// ??? existing hard pointers to areas ?
	case RES_WEBPAGE:
		// Read a web page entry.
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast <CWebPageDef *>(pPrvDef);
			ASSERT(pNewLink);
		}
		else
		{
			pNewLink = new CWebPageDef( rid );
			ASSERT(pNewLink);
			m_WebPages.AddSortKey( pNewLink, rid );
		}
		{
			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load(*pScript);
			pScript->SeekContext( LineContext ); // set the pos back so ScanSection will work.
		}
		break;

	case RES_FUNCTION:
		// Define a char macro. (Name is NOT DEFNAME)
		pNewLink = new CResourceNamed( rid, pScript->GetArgStr());
		m_Functions.AddSortKey( pNewLink, pNewLink->GetName());
		break;

	case RES_SERVERS:	// Old way to define a block of servers.
		{
			bool fReadSelf = false;

			while ( pScript->ReadKey())
			{
				// Does the name already exist ?
				bool fAddNew = false;
				CServerRef pServ;
				size_t i = m_Servers.FindKey( pScript->GetKey());
				if ( i == m_Servers.BadIndex() )
				{
					pServ = new CServerDef( pScript->GetKey(), CSocketAddressIP( SOCKET_LOCAL_ADDRESS ));
					fAddNew = true;
				}
				else
				{
					pServ = Server_GetDef(i);
				}
				ASSERT(pServ != NULL);
				if ( pScript->ReadKey())
				{
					pServ->m_ip.SetHostPortStr( pScript->GetKey());
					if ( pScript->ReadKey())
					{
						pServ->m_ip.SetPort( static_cast<WORD>(pScript->GetArgVal()));
					}
				}
				if ( ! strcmpi( pServ->GetName(), g_Serv.GetName()))
				{
					fReadSelf = true;
				}
				if ( g_Serv.m_ip == pServ->m_ip )
				{
					fReadSelf = true;
				}
				if ( fReadSelf )
				{
					// I can be listed first here. (old way)
					g_Serv.SetName( pServ->GetName());
					g_Serv.m_ip = pServ->m_ip;
					delete pServ;
					fReadSelf = false;
					continue;
				}
				if ( fAddNew )
				{
					m_Servers.AddSortKey( pServ, pServ->GetName());
				}
			}
		}
		return( true );

	case RES_TYPEDEFS:
		// just get a block of defs.
		while ( pScript->ReadKeyParse())
		{
			RESOURCE_ID ridnew( RES_TYPEDEF, pScript->GetArgVal() );
			pPrvDef = ResourceGetDef( ridnew );
			if ( pPrvDef )
			{
				pPrvDef->SetResourceName( pScript->GetKey() );
			}
			else
			{
				CResourceDef * pResDef = new CItemTypeDef( ridnew );
				pResDef->SetResourceName( pScript->GetKey() );
				ASSERT(pResDef);
				m_ResHash.AddSortKey( ridnew, pResDef );
			}
		}
		return( true );

	case RES_STARTS:
		{
			int iStartVersion = pScript->GetArgVal();
			m_StartDefs.RemoveAll();
			while ( pScript->ReadKey())
			{
				CStartLoc * pStart = new CStartLoc( pScript->GetKey());
				if ( pScript->ReadKey())
				{
					pStart->m_sName = pScript->GetKey();
					if ( pScript->ReadKey())
						pStart->m_pt.Read( pScript->GetKeyBuffer());

					if (iStartVersion == 2)
					{
						if ( pScript->ReadKey())
							pStart->iClilocDescription = ATOI(pScript->GetKey());
					}
				}
				m_StartDefs.Add( pStart );
			}

			return( true );
		}
	case RES_MOONGATES:
		m_MoonGates.RemoveAll();
		while ( pScript->ReadKey())
		{
			CPointMap pt = GetRegionPoint( pScript->GetKey());
			m_MoonGates.Add( pt );
		}
		return( true );
	case RES_WORLDVARS:
		while ( pScript->ReadKeyParse() )
		{
			bool fQuoted = false;
			LPCTSTR pszKey = pScript->GetKey();
			if ( strstr(pszKey, "VAR.") )  // This is for backward compatibility from Rcs
				pszKey = pszKey + 4;

			g_Exp.m_VarGlobals.SetStr( pszKey, fQuoted, pScript->GetArgStr( &fQuoted ) );
		}
		return true;
	case RES_WORLDLISTS:
		{
			CListDefCont* pListBase = g_Exp.m_ListGlobals.AddList(pScript->GetArgStr());

			if ( !pListBase )
			{
				DEBUG_ERR(("Unable to create list '%s'...\n", pScript->GetArgStr()));

				return false;
			}

			while ( pScript->ReadKeyParse() )
			{
				if ( !pListBase->r_LoadVal(*pScript) )
					DEBUG_ERR(("Unable to add element '%s' to list '%s'...\n", pScript->GetArgStr(), pListBase->GetKey()));
			}
		}
		return true;
	case RES_TIMERF:
		while ( pScript->ReadKeyParse() )
		{
			bool fQuoted = false;
			g_World.m_TimedFunctions.Load( pScript->GetKey(), fQuoted, pScript->GetArgStr( &fQuoted ) );
		}
		return true;
	case RES_TELEPORTERS:
		while ( pScript->ReadKey())
		{
			// Add the teleporter to the CSector.
			CTeleport * pTeleport = new CTeleport( pScript->GetKeyBuffer());
			ASSERT(pTeleport);
			// make sure this is not a dupe.
			if ( ! pTeleport->RealizeTeleport())
			{
				delete pTeleport;
			}
		}
		return( true );
	case RES_KRDIALOGLIST:
		while ( pScript->ReadKeyParse())
		{
			CDialogDef *pDef = dynamic_cast<CDialogDef *>( g_Cfg.ResourceGetDefByName(RES_DIALOG, pScript->GetKey()) );
			if ( pDef != NULL )
			{
				g_Cfg.SetKRDialogMap( (DWORD)pDef->GetResourceID(), pScript->GetArgVal());
			}
			else
			{
				DEBUG_ERR(("Dialog '%s' not found...\n", pScript->GetKey()));
			}
		}
		return true;

	// Saved in the world file.

	case RES_GMPAGE:	// saved in world file. (Name is NOT DEFNAME)
		{
			CGMPage * pGMPage = new CGMPage( pScript->GetArgStr());
			return( pGMPage->r_Load( *pScript ));
		}
	case RES_WC:
	case RES_WORLDCHAR:	// saved in world file.
		if ( ! rid.IsValidUID())
		{
			g_Log.Event(LOGL_ERROR|LOGM_INIT, "Undefined char type '%s'\n", static_cast<LPCTSTR>(pScript->GetArgStr()));
			return( false );
		}
		return( CChar::CreateBasic(static_cast<CREID_TYPE>(rid.GetResIndex()))->r_Load(*pScript));
	case RES_WI:
	case RES_WORLDITEM:	// saved in world file.
		if ( ! rid.IsValidUID())
		{
			g_Log.Event(LOGL_ERROR|LOGM_INIT, "Undefined item type '%s'\n", static_cast<LPCTSTR>(pScript->GetArgStr()));
			return( false );
		}
		return( CItem::CreateBase(static_cast<ITEMID_TYPE>(rid.GetResIndex()))->r_Load(*pScript));

	default:
		break;
	}

	if ( pNewLink )
	{
		pNewLink->SetResourceVar( pVarNum );

		// NOTE: we should not be linking to stuff in the *WORLD.SCP file.
		CResourceScript* pResScript = dynamic_cast <CResourceScript*>(pScript);
		if ( pResScript == NULL )	// can only link to it if it's a CResourceScript !
		{
			DEBUG_ERR(( "Can't link resources in the world save file\n" ));
			return( false );
		}
		// Now scan it for DEFNAME= or DEFNAME2= stuff ?
		pNewLink->SetLink(pResScript);
		pNewLink->ScanSection( restype );
	}
	else if ( pNewDef && pVarNum )
	{
		// Not linked but still may have a var name
		pNewDef->SetResourceVar( pVarNum );
	}
	return( true );

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("section '%s' args '%s'\n", pszSection, pScript ? pScript->GetArgStr() : "");
	EXC_DEBUG_END;
	return false;
}

//*************************************************************

RESOURCE_ID CResource::ResourceGetNewID( RES_TYPE restype, LPCTSTR pszName, CVarDefContNum ** ppVarNum, bool fNewStyleDef )
{
	ADDTOCALLSTACK("CResource::ResourceGetNewID");
	// We are reading in a script block.
	// We may be creating a new id or replacing an old one.
	// ARGS:
	//	restype = the data type we are reading in.
	//  pszName = MAy or may not be the DEFNAME depending on type.

	ASSERT(pszName);
	ASSERT(ppVarNum);

	const RESOURCE_ID ridinvalid;	// LINUX wants this for some reason.
	RESOURCE_ID rid;
	int iPage = 0;	// sub page

	// Some types don't use named indexes at all. (Single instance)
	switch ( restype )
	{
	case RES_UNKNOWN:
		return( ridinvalid);
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
		// Single instance stuff. (fully read in)
		// Ignore any resource name.
		return( RESOURCE_ID( restype ));
	case RES_FUNCTION:		// Define a new command verb script that applies to a char.
	case RES_ACCOUNT:
	case RES_GMPAGE:
	case RES_SECTOR:
		// These must have a resource name but do not use true RESOURCE_ID format.
		// These are multiple instance but name is not a RESOURCE_ID
		if ( pszName[0] == '\0' )
			return( ridinvalid );	// invalid
		return( RESOURCE_ID( restype ));
	// Extra args are allowed.
	case RES_BOOK:	// BOOK BookName page
	case RES_DIALOG:	// DIALOG GumpName ./TEXT/BUTTON
	case RES_REGIONTYPE:
		{
			if ( pszName[0] == '\0' )
				return( ridinvalid );
			TCHAR * pArg1 = Str_GetTemp();
			strcpy( pArg1, pszName );
			pszName = pArg1;
			TCHAR * pArg2;
			Str_Parse( pArg1, &pArg2 );
			if ( ! strcmpi( pArg2, "TEXT" ))
				iPage = RES_DIALOG_TEXT;
			else if ( ! strcmpi( pArg2, "BUTTON" ))
				iPage = RES_DIALOG_BUTTON;
			else
				iPage = RES_GET_INDEX( Exp_GetVal( pArg2 ));
			if ( iPage > 255 )
			{
				DEBUG_ERR(( "Bad resource index page %d\n", iPage ));
			}
		}
		break;
	case RES_NEWBIE:	// MALE_DEFAULT, FEMALE_DEFAULT, Skill
		{
			if ( pszName[0] == '\0' )
				return( ridinvalid );
			TCHAR * pArg1 = Str_GetTemp();
			strcpy( pArg1, pszName );
			pszName = pArg1;
			TCHAR * pArg2;
			Str_Parse( pArg1, &pArg2 );
			if ( pArg2[0] == '\0' || ! strcmpi( pArg2, "HUMAN" ))
				iPage = RACETYPE_HUMAN;
			else if ( ! strcmpi( pArg2, "ELF" ))
				iPage = RACETYPE_ELF;
			else if ( ! strcmpi( pArg2, "GARGOYLE" ))
				iPage = RACETYPE_GARGOYLE;

			if ( ! strcmpi( pszName, "MALE_DEFAULT" ))
				return ( RESOURCE_ID( RES_NEWBIE, RES_NEWBIE_MALE_DEFAULT, iPage ));
			else if ( ! strcmpi( pszName, "FEMALE_DEFAULT" ))
				return ( RESOURCE_ID( RES_NEWBIE, RES_NEWBIE_FEMALE_DEFAULT, iPage ));

			if ( ! strcmpi( pszName, "PROFESSION_ADVANCED" ))
				return ( RESOURCE_ID( RES_NEWBIE, RES_NEWBIE_PROF_ADVANCED, iPage ));
			else if ( ! strcmpi( pszName, "PROFESSION_WARRIOR" ))
				return ( RESOURCE_ID( RES_NEWBIE, RES_NEWBIE_PROF_WARRIOR, iPage ));
			else if ( ! strcmpi( pszName, "PROFESSION_MAGE" ))
				return ( RESOURCE_ID( RES_NEWBIE, RES_NEWBIE_PROF_MAGE, iPage ));
			else if ( ! strcmpi( pszName, "PROFESSION_BLACKSMITH" ))
				return ( RESOURCE_ID( RES_NEWBIE, RES_NEWBIE_PROF_BLACKSMITH, iPage ));
			else if ( ! strcmpi( pszName, "PROFESSION_NECROMANCER" ))
				return ( RESOURCE_ID( RES_NEWBIE, RES_NEWBIE_PROF_NECROMANCER, iPage ));
			else if ( ! strcmpi( pszName, "PROFESSION_PALADIN" ))
				return ( RESOURCE_ID( RES_NEWBIE, RES_NEWBIE_PROF_PALADIN, iPage ));
			else if ( ! strcmpi( pszName, "PROFESSION_SAMURAI" ))
				return ( RESOURCE_ID( RES_NEWBIE, RES_NEWBIE_PROF_SAMURAI, iPage ));
			else if ( ! strcmpi( pszName, "PROFESSION_NINJA" ))
				return ( RESOURCE_ID( RES_NEWBIE, RES_NEWBIE_PROF_NINJA, iPage ));
		}
		break;
	case RES_AREA:
	case RES_ROOM:
		if ( !fNewStyleDef )
		{
			// Name is not the defname or id, just find a free id.
			pszName = NULL;	// fake it out for now.
			break;
		}
		// otherwise, passthrough to default
	default:
		// The name is a DEFNAME or id number
		ASSERT( restype < RES_QTY );
		break;
	}


	int index;
	if ( pszName )
	{
		if ( pszName[0] == '\0' )	// absense of resourceid = index 0
		{
			// This might be ok.
			return( RESOURCE_ID( restype, 0, iPage ) );
		}
		if ( IsDigit(pszName[0]))	// Its just an index.
		{
			index = Exp_GetVal(pszName);
			rid = RESOURCE_ID( restype, index );
			switch ( restype )
			{
			case RES_BOOK:			// A book or a page from a book.
			case RES_DIALOG:			// A scriptable gump dialog: text or handler block.
			case RES_REGIONTYPE:	// Triggers etc. that can be assinged to a RES_AREA
				rid = RESOURCE_ID( restype, index, iPage );
				break;
			case RES_SKILLMENU:
			case RES_MENU:			// General scriptable menus.
			case RES_EVENTS:		// An Event handler block with the trigger type in it. ON=@Death etc.
			case RES_SPEECH:		// A speech block with ON=*blah* in it.
			case RES_NAMES:			// A block of possible names for a NPC type. (read as needed)
			case RES_SCROLL:		// SCROLL_GUEST=message scroll sent to player at guest login. SCROLL_MOTD: SCROLL_NEWBIE
			case RES_TIP:			// Tips (similar to RES_SCROLL) that can come up at startup.
			case RES_TYPEDEF:			// Define a trigger block for a RES_WORLDITEM m_type.
			case RES_CHARDEF:		// Define a char type.
			case RES_TEMPLATE:		// Define lists of items. (for filling loot etc)
				break;
			case RES_ITEMDEF:		// Define an item type
				if (fNewStyleDef)	// indicates this is a multi and should have an appropriate offset applied
					rid = RESOURCE_ID( restype, index + ITEMID_MULTI);
				break;
			default:
				return( rid );
			}
#ifdef _DEBUG
			if ( g_Serv.m_iModeCode != SERVMODE_ResyncLoad )	// this really is ok.
			{
				// Warn of  duplicates.
				size_t duplicateIndex = m_ResHash.FindKey( rid );
				if ( duplicateIndex != m_ResHash.BadIndex() )	// i found it. So i have to find something else.
					ASSERT(m_ResHash.GetAt(rid, duplicateIndex));
			}
#endif
			return( rid );
		}


		CVarDefCont * pVarBase = g_Exp.m_VarDefs.GetKey( pszName );
		if ( pVarBase )
		{
			// An existing VarDef with the same name ?
			// We are creating a new Block but using an old name ? weird.
			// just check to see if this is a strange type conflict ?
			CVarDefContNum * pVarNum = dynamic_cast <CVarDefContNum*>( pVarBase );
			if ( pVarNum == NULL )
			{
				switch (restype)
				{
					case RES_WC:
					case RES_WI:
					case RES_WS:
					case RES_WORLDCHAR:
					case RES_WORLDITEM:
					case RES_WORLDSCRIPT:
					{
						const CVarDefContStr * pVarStr = dynamic_cast <CVarDefContStr*>( pVarBase );
						if ( pVarStr != NULL )
							return( ResourceGetNewID(restype, pVarStr->GetValStr(), ppVarNum, fNewStyleDef) );
					}
					default:
						DEBUG_ERR(( "Re-Using name '%s' to define block\n", static_cast<LPCTSTR>(pszName) ));
						return( ridinvalid );
				}
			}
			rid.SetPrivateUID( static_cast<unsigned long>(pVarNum->GetValNum()));
			if ( restype != rid.GetResType())
			{
				switch ( restype )
				{
					case RES_WC:
					case RES_WI:
					case RES_WORLDCHAR:
					case RES_WORLDITEM:
					case RES_NEWBIE:
					case RES_PLEVEL:
						// These are not truly defining a new DEFNAME
						break;
					default:
						DEBUG_ERR(( "Redefined name '%s' from %s to %s\n", static_cast<LPCTSTR>(pszName), static_cast<LPCTSTR>(GetResourceBlockName(rid.GetResType())), static_cast<LPCTSTR>(GetResourceBlockName(restype)) ));
						return( ridinvalid );
				}
			}
			else if ( fNewStyleDef && static_cast<DWORD>(pVarNum->GetValNum()) != rid.GetPrivateUID() )
			{
				DEBUG_ERR(( "WARNING: region redefines DEFNAME '%s' for another region!\n", pszName ));
			}
			else if ( iPage == rid.GetResPage())	// Books and dialogs have pages.
			{
				// We are redefining an item we have already read in ?
				// Why do this unless it's a Resync ?
				if ( g_Serv.m_iModeCode != SERVMODE_ResyncLoad )
				{
					if ( g_Cfg.m_wDebugFlags & DEBUGF_SCRIPTS )
					{
						g_pLog->EventWarn("Redef resource '%s'\n", static_cast<LPCTSTR>(pszName));
					}
					else
					{
						DEBUG_WARN(( "Redef resource '%s'\n", static_cast<LPCTSTR>(pszName) ));
					}
				}
			}
			rid = RESOURCE_ID( restype, rid.GetResIndex(), iPage );
			*ppVarNum = pVarNum;
			return( rid );
		}
	}

	// we must define this as a new unique entry.
	// Find a new free entry.

	int iHashRange = 0;
	switch ( restype )
	{

	// Some cannot create new id's
	// Do not allow NEW named indexs for some types for now.

	case RES_SKILL:			// Define attributes for a skill (how fast it raises etc)
		// rid = m_SkillDefs.GetCount();
	case RES_SPELL:			// Define a magic spell. (0-64 are reserved)
		// rid = m_SpellDefs.GetCount();
		return( ridinvalid );

	// These MUST exist !

	case RES_NEWBIE:	// MALE_DEFAULT, FEMALE_DEFAULT, Skill
		return( ridinvalid );
	case RES_PLEVEL:	// 0-7
		return( ridinvalid );
	case RES_WC:
	case RES_WI:
	case RES_WORLDCHAR:
	case RES_WORLDITEM:
		return( ridinvalid );

	// Just find a free entry in proper range.

	case RES_CHARDEF:		// Define a char type.
		iHashRange = 2000;
		index = NPCID_SCRIPT + 0x2000;	// add another offset to avoid Sphere ranges.
		break;
	case RES_ITEMDEF:		// Define an item type
		iHashRange = 2000;
		index = ITEMID_SCRIPT2 + 0x4000;	// add another offset to avoid Sphere ranges.
		break;
	case RES_TEMPLATE:		// Define lists of items. (for filling loot etc)
		iHashRange = 2000;
		index = ITEMID_TEMPLATE + 100000;
		break;

	case RES_BOOK:			// A book or a page from a book.
	case RES_DIALOG:			// A scriptable gump dialog: text or handler block.
		if ( iPage )	// We MUST define the main section FIRST !
			return( ridinvalid );

	case RES_REGIONTYPE:	// Triggers etc. that can be assinged to a RES_AREA
		iHashRange = 100;
		index = 1000;
		break;

	case RES_AREA:
		iHashRange = 1000;
		index = 10000;
		break;
	case RES_ROOM:
	case RES_SKILLMENU:
	case RES_MENU:			// General scriptable menus.
	case RES_EVENTS:			// An Event handler block with the trigger type in it. ON=@Death etc.
	case RES_SPEECH:			// A speech block with ON=*blah* in it.
	case RES_NAMES:			// A block of possible names for a NPC type. (read as needed)
	case RES_SCROLL:		// SCROLL_GUEST=message scroll sent to player at guest login. SCROLL_MOTD: SCROLL_NEWBIE
	case RES_TIP:			// Tips (similar to RES_SCROLL) that can come up at startup.
	case RES_TYPEDEF:			// Define a trigger block for a RES_WORLDITEM m_type.
	case RES_SKILLCLASS:		// Define specifics for a char with this skill class. (ex. skill caps)
	case RES_REGIONRESOURCE:
		iHashRange = 1000;
		index = 10000;
		break;
	case RES_SPAWN:			// Define a list of NPC's and how often they may spawn.
		iHashRange = 1000;
		index = SPAWNTYPE_START + 100000;
		break;
	case RES_WEBPAGE:		// Define a web page template.
		index = m_WebPages.GetCount() + 1;
		break;

	default:
		ASSERT(0);
		return( ridinvalid );
	}

	if ( iPage )
	{
		rid = RESOURCE_ID( restype, index, iPage );
	}
	else
	{
		rid = RESOURCE_ID( restype, index );
	}

	if ( iHashRange )
	{
		// find a new FREE entry starting here
		rid.SetPrivateUID( rid.GetPrivateUID() + Calc_GetRandVal( iHashRange ));
		for (;;)
		{
			if ( m_ResHash.FindKey(rid) == m_ResHash.BadIndex())
				break;
			rid.SetPrivateUID( rid.GetPrivateUID()+1 );
		}
	}
	else
	{
		// find a new FREE entry starting here
		if ( ! index )
		{
			rid.SetPrivateUID( rid.GetPrivateUID()+1 );
		}
	}

	if ( pszName )
	{
		int iVarNum = g_Exp.m_VarDefs.SetNum( pszName, rid.GetPrivateUID() );
		if ( iVarNum >= 0 )
		{
			*ppVarNum = dynamic_cast <CVarDefContNum*>( g_Exp.m_VarDefs.GetAt(iVarNum));
		}
	}

	return( rid );
}

CResourceDef * CResource::ResourceGetDef( RESOURCE_ID_BASE rid ) const
{
	ADDTOCALLSTACK("CResource::ResourceGetDef");
	// Get a CResourceDef from the RESOURCE_ID.
	// ARGS:
	//	restype = id must be this type.

	if ( ! rid.IsValidUID())
		return( NULL );

	size_t index = rid.GetResIndex();
	switch ( rid.GetResType() )
	{
		case RES_WEBPAGE:
			index = m_WebPages.FindKey( rid );
			if ( ! m_WebPages.IsValidIndex(index))
				return( NULL );
			return( m_WebPages.GetAt( index ));

		case RES_SKILL:
			if ( ! m_SkillIndexDefs.IsValidIndex(index))
				return( NULL );
			return( m_SkillIndexDefs[ index ] );

		case RES_SPELL:
			if ( ! m_SpellDefs.IsValidIndex(index))
				return( NULL );
			return( m_SpellDefs[ index ] );

		case RES_UNKNOWN:	// legal to use this as a ref but it is unknown
			return( NULL );

		case RES_BOOK:			// A book or a page from a book.
		case RES_EVENTS:
		case RES_DIALOG:			// A scriptable gump dialog: text or handler block.
		case RES_MENU:
		case RES_NAMES:			// A block of possible names for a NPC type. (read as needed)
		case RES_NEWBIE:	// MALE_DEFAULT, FEMALE_DEFAULT, Skill
		case RES_REGIONRESOURCE:
		case RES_REGIONTYPE:		// Triggers etc. that can be assinged to a RES_AREA
		case RES_SCROLL:		// SCROLL_GUEST=message scroll sent to player at guest login. SCROLL_MOTD: SCROLL_NEWBIE
		case RES_SPEECH:
		case RES_TIP:			// Tips (similar to RES_SCROLL) that can come up at startup.
		case RES_TYPEDEF:			// Define a trigger block for a RES_WORLDITEM m_type.
		case RES_TEMPLATE:
		case RES_SKILLMENU:
		case RES_ITEMDEF:
		case RES_CHARDEF:
		case RES_SPAWN:	// the char spawn tables
		case RES_SKILLCLASS:
		case RES_AREA:
		case RES_ROOM:
			break;

		default:
			return NULL;
	}

	return CResourceBase::ResourceGetDef( rid );
}

////////////////////////////////////////////////////////////////////////////////

void CResource::OnTick( bool fNow )
{
	ADDTOCALLSTACK("CResource::OnTick");
	// Give a tick to the less critical stuff.
	if ( !fNow && ( g_Serv.IsLoading() || ( m_timePeriodic > CServTime::GetCurrentTime() )))
		return;

	if ( this->m_fUseHTTP )
	{
		// Update WEBPAGES resources
		for ( size_t i = 0; i < m_WebPages.GetCount(); i++ )
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
			g_Log.EventDebug("web '%s' dest '%s' now '%d' index '%" FMTSIZE_T "'/'%" FMTSIZE_T "'\n",
				pWeb ? pWeb->GetName() : "", pWeb ? pWeb->GetDstName() : "",
				fNow? 1 : 0, i, m_WebPages.GetCount());
			EXC_DEBUG_END;
		}
	}

	m_timePeriodic = CServTime::GetCurrentTime() + ( 60 * TICK_PER_SEC );
}

#define catresname(a,b)	\
{	\
	if ( *(a) ) strcat(a, " + "); \
	strcat(a, b); \
}

void CResource::PrintEFOFFlags(bool bEF, bool bOF, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CResource::PrintEFOFFlags");
	if ( g_Serv.IsLoading() ) return;
	if ( bOF )
	{
		TCHAR zOptionFlags[512];
		zOptionFlags[0] = '\0';

		if ( IsSetOF(OF_NoDClickTarget) ) catresname(zOptionFlags, "NoDClickTarget");
		if ( IsSetOF(OF_NoSmoothSailing) ) catresname(zOptionFlags, "NoSmoothSailing");
		if ( IsSetOF(OF_ScaleDamageByDurability) ) catresname(zOptionFlags, "ScaleDamageByDurability");
		if ( IsSetOF(OF_Command_Sysmsgs) ) catresname(zOptionFlags, "CommandSysmessages");
		if ( IsSetEF(OF_PetSlots) ) catresname(zOptionFlags, "PetSlots");
		if ( IsSetOF(OF_OSIMultiSight) ) catresname(zOptionFlags, "OSIMultiSight");
		if ( IsSetOF(OF_Items_AutoName) ) catresname(zOptionFlags, "ItemsAutoName");
		if ( IsSetOF(OF_FileCommands) ) catresname(zOptionFlags, "FileCommands");
		if ( IsSetOF(OF_NoItemNaming) ) catresname(zOptionFlags, "NoItemNaming");
		if ( IsSetOF(OF_NoHouseMuteSpeech) ) catresname(zOptionFlags, "NoHouseMuteSpeech");
		if ( IsSetOF(OF_NoContextMenuLOS) ) catresname(zOptionFlags, "NoContextMenuLOS");
		if ( IsSetOF(OF_Flood_Protection) ) catresname(zOptionFlags, "FloodProtection");
		if ( IsSetOF(OF_Buffs) ) catresname(zOptionFlags, "Buffs");
		if ( IsSetOF(OF_NoPrefix) ) catresname(zOptionFlags, "NoPrefix");
		if ( IsSetOF(OF_DyeType) ) catresname(zOptionFlags, "DyeType");
		if ( IsSetOF(OF_DrinkIsFood) ) catresname(zOptionFlags, "DrinkIsFood");
		if ( IsSetOF(OF_DClickNoTurn) ) catresname(zOptionFlags, "DClickNoTurn");

		if ( zOptionFlags[0] != '\0' )
		{
			if ( pSrc ) pSrc->SysMessagef("Option flags: %s\n", zOptionFlags);
			else g_Log.Event(LOGM_INIT, "Option flags: %s\n", zOptionFlags);
		}
	}
	if ( bEF )
	{
		TCHAR zExperimentalFlags[512];
		zExperimentalFlags[0] = '\0';

		if ( IsSetEF(EF_NoDiagonalCheckLOS) ) catresname(zExperimentalFlags, "NoDiagonalCheckLOS");
		if ( IsSetEF(EF_ItemStacking) ) catresname(zExperimentalFlags, "ItemStacking");
		if ( IsSetEF(EF_ItemStackDrop) ) catresname(zExperimentalFlags, "ItemStackDrop");
		if ( IsSetEF(EF_Intrinsic_Locals) ) catresname(zExperimentalFlags, "IntrinsicLocals");
		if ( IsSetEF(EF_Item_Strict_Comparison) ) catresname(zExperimentalFlags, "ItemStrictComparison");
		if ( IsSetEF(EF_AllowTelnetPacketFilter) ) catresname(zExperimentalFlags, "TelnetPacketFilter");
		if ( IsSetEF(EF_Script_Profiler) ) catresname(zExperimentalFlags, "ScriptProfiler");
		if ( IsSetEF(EF_DamageTools) ) catresname(zExperimentalFlags, "DamageTools");
		if ( IsSetEF(EF_UsePingServer) ) catresname(zExperimentalFlags, "UsePingServer");
		if ( IsSetEF(EF_FixCanSeeInClosedConts) ) catresname(zExperimentalFlags, "FixCanSeeInClosedConts");
#ifndef _MTNETWORK
		if ( IsSetEF(EF_NetworkOutThread) ) catresname(zExperimentalFlags, "NetworkOutThread");
#endif

		if ( zExperimentalFlags[0] != '\0' )
		{
			if ( pSrc ) pSrc->SysMessagef("Experimental flags: %s\n", zExperimentalFlags);
			else g_Log.Event(LOGM_INIT, "Experimental flags: %s\n", zExperimentalFlags);
		}
	}
}

bool CResource::LoadIni( bool fTest )
{
	ADDTOCALLSTACK("CResource::LoadIni");
	// Load my INI file first.
	if ( ! OpenResourceFind( m_scpIni, GRAY_FILE ".ini", !fTest )) // Open script file
	{
		if( !fTest )
		{
			g_Log.Event(LOGL_FATAL|LOGM_INIT, "File " GRAY_FILE ".ini is corrupt or missing, server probably would be not usable.\n");
			g_Log.Event(LOGL_FATAL|LOGM_INIT, "Navigate to http://prerelease.sphereserver.net/ to download sample config.\n");
		}
		return( false );
	}

	LoadResourcesOpen(&m_scpIni);
	m_scpIni.Close();
	m_scpIni.CloseForce();

	return true;
}

bool CResource::LoadCryptIni( void )
{
	ADDTOCALLSTACK("CResource::LoadCryptIni");
	if ( ! OpenResourceFind( m_scpCryptIni, GRAY_FILE "Crypt.ini", false ))
	{
		g_Log.Event(LOGL_WARN|LOGM_INIT, "File " GRAY_FILE "Crypt.ini is corrupt or missing, client encryption list might not be available.\n");
		return( false );
	}

	LoadResourcesOpen(&m_scpCryptIni);
	m_scpCryptIni.Close();
	m_scpCryptIni.CloseForce();

	g_Log.Event( LOGM_INIT, "Loaded %" FMTSIZE_T " client encryption keys.\n", CCrypt::client_keys.size() );

	return( true );
}

void CResource::Unload( bool fResync )
{
	ADDTOCALLSTACK("CResource::Unload");
	if ( fResync )
	{
		// Unlock all the SCP and MUL files.
		g_Install.CloseFiles();
		for ( size_t j = 0; ; j++ )
		{
			CResourceScript * pResFile = GetResourceFile(j);
			if ( pResFile == NULL )
				break;
			pResFile->CloseForce();
		}
		m_scpIni.CloseForce();
		m_scpTables.CloseForce();
		return;
	}

	m_ResourceFiles.RemoveAll();

	// m_ResHash.RemoveAll();

	m_Obscene.RemoveAll();
	m_Fame.RemoveAll();
	m_Karma.RemoveAll();
	m_NotoTitles.RemoveAll();
	m_NotoKarmaLevels.RemoveAll();
	m_NotoFameLevels.RemoveAll();
	m_Runes.RemoveAll();	// Words of power. (A-Z)
	// m_MultiDefs
	m_SkillNameDefs.RemoveAll();	// Defined Skills
	m_SkillIndexDefs.RemoveAll();
	// m_Servers
	m_Functions.RemoveAll();
	m_StartDefs.RemoveAll(); // Start points list
	// m_StatAdv
	for ( int j=0; j<PLEVEL_QTY; j++ )
	{
		m_PrivCommands[j].RemoveAll();
	}
	m_MoonGates.Empty();
	// m_WebPages
	m_SpellDefs.RemoveAll();	// Defined Spells
	m_SpellDefs_Sorted.RemoveAll();

	CCrypt::ClearKeyTable();
}

bool CResource::Load( bool fResync )
{
	ADDTOCALLSTACK("CResource::Load");
	// ARGS:
	//  fResync = just look for changes.

	if ( fResync )
	{
		m_scpIni.ReSync();
		m_scpIni.CloseForce();
	}
	else
	{
		g_Install.FindInstall();
	}

	// Open the MUL files I need.
	VERFILE_TYPE i = g_Install.OpenFiles(
		(1<<VERFILE_MAP)|
		(1<<VERFILE_STAIDX)|
		(1<<VERFILE_STATICS)|
		(1<<VERFILE_TILEDATA)|
		(1<<VERFILE_MULTIIDX)|
		(1<<VERFILE_MULTI)|
		(1<<VERFILE_VERDATA)
		);
	if ( i != VERFILE_QTY )
	{
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "File " GRAY_FILE ".ini is corrupt or missing.\n");
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "MUL File '%s' not found...\n", static_cast<LPCTSTR>(g_Install.GetBaseFileName(i)));
		return( false );
	}

	// Load the optional verdata cache. (modifies MUL stuff)
	try
	{
		g_VerData.Load( g_Install.m_File[VERFILE_VERDATA] );
	}
	catch ( const CGrayError& e )
	{
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "File " GRAY_FILE ".ini is corrupt or missing.\n");
		g_Log.CatchEvent( &e, "g_VerData.Load" );
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		return( false );
	}
	catch(...)
	{
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "File " GRAY_FILE ".ini is corrupt or missing.\n");
		g_Log.CatchEvent( NULL, "g_VerData.Load" );
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		return( false );
	}

	// Now load the *TABLES.SCP file.
	if ( fResync )
	{
		m_scpTables.ReSync();
	}
	else
	{
		if ( !OpenResourceFind(m_scpTables, GRAY_FILE "tables") )
		{
			g_Log.Event(LOGL_FATAL|LOGM_INIT, "File " GRAY_FILE ".ini is corrupt or missing.\n");
			g_Log.Event(LOGL_FATAL|LOGM_INIT, "Error opening table definitions file...\n");
			return( false );
		}

		LoadResourcesOpen(&m_scpTables);
		m_scpTables.Close();
	}
	m_scpTables.CloseForce();

	// Initialize world sectors on server startup
	if ( !fResync )
		g_World.Init();

	// open and index all my script files i'm going to use.
	AddResourceDir( m_sSCPBaseDir );		// if we want to get *.SCP files from elsewhere.

	size_t count = m_ResourceFiles.GetCount();
	g_Log.Event(LOGM_INIT, "Indexing %" FMTSIZE_T " scripts...\n", count);

	for ( size_t j = 0; ; j++ )
	{
		CResourceScript * pResFile = GetResourceFile(j);
		if ( !pResFile )
			break;

		if ( fResync )
			pResFile->ReSync();
		else
			LoadResources(pResFile);

#ifdef _WIN32
		NTWindow_OnTick(0);
#endif
		g_Serv.PrintPercent(j + 1, count);
	}

	if ( fResync )
		g_World.Init();

	// Make sure we have the basics.
	if ( g_Serv.GetName()[0] == '\0' )	// make sure we have a set name
	{
		TCHAR szName[ MAX_SERVER_NAME_SIZE ];
		int iRet = gethostname( szName, sizeof( szName ));
		g_Serv.SetName(( ! iRet && szName[0] ) ? szName : GRAY_TITLE );
	}

	if ( ! g_Cfg.ResourceGetDef( RESOURCE_ID( RES_SKILLCLASS, 0 )))
	{
		// must have at least 1 skill class.
		CSkillClassDef * pSkillClass = new CSkillClassDef( RESOURCE_ID( RES_SKILLCLASS ));
		ASSERT(pSkillClass);
		m_ResHash.AddSortKey( RESOURCE_ID( RES_SKILLCLASS, 0 ), pSkillClass );
	}

	if ( m_StartDefs.GetCount() <= 0 )
	{
		g_Log.Event(LOGM_INIT|LOGL_ERROR, "No START locations specified. Add them and try again.\n");
		return false;
	}

	// Make region DEFNAMEs
	size_t iMax = g_Cfg.m_RegionDefs.GetCount();
	for ( size_t k = 0; k < iMax; k++ )
	{
		CRegionBase *pRegion = dynamic_cast<CRegionBase *>(g_Cfg.m_RegionDefs.GetAt(i));
		if ( !pRegion )
			continue;
		pRegion->MakeRegionName();
	}

	if ( fResync && g_Serv.m_fResyncMultiRegions )
	{
		g_World.ResyncMultiRegions();
		g_Serv.m_fResyncMultiRegions = false;
	}

	// parse eventsitem
	m_iEventsItemLink.Empty();
	if ( ! m_sEventsItem.IsEmpty() )
	{
		CScript script("EVENTSITEM", m_sEventsItem);
		m_iEventsItemLink.r_LoadVal(script, RES_EVENTS);
	}
	
	// parse eventspet
	m_pEventsPetLink.Empty();
	if ( ! m_sEventsPet.IsEmpty() )
	{
		CScript script("EVENTSPET", m_sEventsPet);
		m_pEventsPetLink.r_LoadVal(script, RES_EVENTS);
	}

	// parse eventsplayer
	m_pEventsPlayerLink.Empty();
	if ( ! m_sEventsPlayer.IsEmpty() )
	{
		CScript script("EVENTSPLAYER", m_sEventsPlayer);
		m_pEventsPlayerLink.r_LoadVal(script, RES_EVENTS);
	}

	// parse eventsregion
	m_pEventsRegionLink.Empty();
	if ( ! m_sEventsRegion.IsEmpty() )
	{
		CScript script("EVENTSREGION", m_sEventsRegion);
		m_pEventsRegionLink.r_LoadVal(script, RES_REGIONTYPE);
	}

	LoadSortSpells();

	long total, used;
	Triglist(total, used);
	g_Serv.SysMessagef("Done loading scripts (%ld of %ld triggers used)\n\n", used, total);

	// Load crypt keys from SphereCrypt.ini
	if ( fResync )
	{
		m_scpCryptIni.ReSync();
		m_scpCryptIni.CloseForce();
	}
	else
	{
		LoadCryptIni();
	}

	// Yay for crypt version
	g_Serv.SetCryptVersion();

	return true;
}

LPCTSTR CResource::GetDefaultMsg(long lKeyNum)
{
	ADDTOCALLSTACK("CResource::GetDefaultMsg");
	if (( lKeyNum < 0 ) || ( lKeyNum >= DEFMSG_QTY ))
	{
		g_Log.EventError("Defmessage %ld out of range [0..%d]\n", lKeyNum, DEFMSG_QTY-1);
		return "";
	}
	return g_Exp.sm_szMessages[lKeyNum];
}

LPCTSTR CResource::GetDefaultMsg(LPCTSTR pszKey)
{
	ADDTOCALLSTACK("CResource::GetDefaultMsg");
	for ( long l = 0; l < DEFMSG_QTY; ++l )
	{
		if ( !strcmpi(pszKey, g_Exp.sm_szMsgNames[l]) )
			return g_Exp.sm_szMessages[l];

	}
	g_Log.EventError("Defmessage \"%s\" non existent\n", pszKey);
	return "";
}

bool CResource::GenerateDefname(TCHAR *pObjectName, size_t iInputLength, LPCTSTR pPrefix, TCHAR *pOutput, bool bCheckConflict, CVarDefMap* vDefnames)
{
	ADDTOCALLSTACK("CResource::GenerateDefname");
	if ( !pOutput )
		return false;

	if ( !pObjectName || !pObjectName[0] )
	{
		pOutput[0] = '\0';
		return false;
	}

	size_t iOut = 0;

	if (pPrefix)
	{
		// write prefix
		for (size_t i = 0; pPrefix[i] != '\0'; i++)
			pOutput[iOut++] = pPrefix[i];
	}

	// write object name
	for (size_t i = 0; i < iInputLength; i++)
	{
		if (pObjectName[i] == '\0')
			break;

		if ( ISWHITESPACE(pObjectName[i]) )
		{
			if (iOut > 0 && pOutput[iOut - 1] != '_') // avoid double '_'
				pOutput[iOut++] = '_';
		}
		else if ( _ISCSYMF(pObjectName[i]) )
		{
			if (pObjectName[i] != '_' || (iOut > 0 && pOutput[iOut - 1] != '_')) // avoid double '_'
				pOutput[iOut++] = static_cast<TCHAR>(tolower(pObjectName[i]));
		}
	}

	// remove trailing _
	while (iOut > 0 && pOutput[iOut - 1] == '_')
		iOut--;

	pOutput[iOut] = '\0';
	if (iOut == 0)
		return false;

	if (bCheckConflict == true)
	{
		// check for conflicts
		size_t iEnd = iOut;
		int iAttempts = 1;

		for (;;)
		{
			bool isValid = true;
			if (g_Exp.m_VarDefs.GetKey(pOutput) != NULL)
			{
				// check loaded defnames
				isValid = false;
			}
			else if (vDefnames && vDefnames->GetKey(pOutput) != NULL)
			{
				isValid = false;
			}

			if (isValid == true)
				break;

			// attach suffix
			iOut = iEnd;
			pOutput[iOut++] = '_';
			ITOA(++iAttempts, &pOutput[iOut], 10);
		}
	}

	// record defname
	if (vDefnames != NULL)
		vDefnames->SetNum(pOutput, 1);

	return true;
}

bool CResource::DumpUnscriptedItems( CTextConsole * pSrc, LPCTSTR pszFilename )
{
	ADDTOCALLSTACK("CResource::DumpUnscriptedItems");
	if ( pSrc == NULL )
		return false;

	if ( pszFilename == NULL || pszFilename[0] == '\0' )
		pszFilename	= "unscripted_items" GRAY_SCRIPT;
	else if ( strlen( pszFilename ) <= 4 )
		return false;
	
	// open file
	CScript s;
	if ( ! s.Open( pszFilename, OF_WRITE|OF_TEXT|OF_DEFAULTMODE ))
		return false;

	TCHAR sItemName[21];
	TemporaryString sDefnameBuffer;
	TCHAR * pDefnameBuffer = static_cast<TCHAR *>(sDefnameBuffer);
	CVarDefMap defnames;

	s.Printf("// Unscripted items, generated by " GRAY_TITLE " at %s\n", CGTime::GetCurrentTime().Format(NULL));

	ITEMID_TYPE idMaxItem = CGrayItemInfo::GetMaxTileDataItem();
	if (idMaxItem > ITEMID_MULTI)
		idMaxItem = ITEMID_MULTI;

	for (int i = 0; i < idMaxItem; i++)
	{
		if ( !( i % 0xff ))
			g_Serv.PrintPercent(i, idMaxItem);

		RESOURCE_ID rid = RESOURCE_ID(RES_ITEMDEF, i);
		if (g_Cfg.m_ResHash.FindKey(rid) != g_Cfg.m_ResHash.BadIndex())
			continue;

		// check item in tiledata
		CUOItemTypeRec2 tiledata;
		if (CItemBase::GetItemData(static_cast<ITEMID_TYPE>(i), &tiledata) == false)
			continue;

		// ensure there is actually some data here, treat "MissingName" as blank since some tiledata.muls
		// have this name set in blank slots
		if ( !tiledata.m_flags && !tiledata.m_weight && !tiledata.m_layer &&
			 !tiledata.m_dwUnk11 && !tiledata.m_dwAnim && !tiledata.m_wUnk19 && !tiledata.m_height && 
			 (!tiledata.m_name[0] || strcmp(tiledata.m_name, "MissingName") == 0))
			 continue;

		s.WriteSection("ITEMDEF 0%04x", i);
		strcpylen(sItemName, tiledata.m_name, COUNTOF(sItemName));

		// generate a suitable defname
		if (GenerateDefname(sItemName, COUNTOF(sItemName), "i_", pDefnameBuffer, true, &defnames))
		{
			s.Printf("// %s\n", sItemName);
			s.WriteKey("DEFNAME", pDefnameBuffer);
		}
		else
		{
			s.Printf("// (unnamed object)\n");
		}

		s.WriteKey("TYPE", ResourceGetName(RESOURCE_ID(RES_TYPEDEF, CItemBase::GetTypeBase(static_cast<ITEMID_TYPE>(i), tiledata))));
	}

	s.WriteSection("EOF");
	s.Close();

#ifdef _WIN32
	NTWindow_SetWindowTitle();
#endif
	return true;
}

int CItemTypeDef::GetItemType() const
{
	ADDTOCALLSTACK("CItemTypeDef::GetItemType");
	return GETINTRESOURCE(GetResourceID());
}

bool CItemTypeDef::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CItemTypeDef::r_LoadVal");
	EXC_TRY("LoadVal");
	LPCTSTR		pszKey	= s.GetKey();
	LPCTSTR		pszArgs	= s.GetArgStr();

	if ( !strnicmp( pszKey, "TERRAIN", 7 ) )
	{
		size_t iLo = Exp_GetVal( pszArgs );
		GETNONWHITESPACE( pszArgs );

		if ( *pszArgs == ',' )
		{
			pszArgs++;
			GETNONWHITESPACE( pszArgs );
		}

		size_t iHi;
		if ( *pszArgs == '\0' )
			iHi	= iLo;
		else
			iHi	= Exp_GetVal( pszArgs );

		if ( iLo > iHi )		// swap
		{
			size_t iTmp = iHi;
			iHi	= iLo;
			iLo	= iTmp;
		}

		for ( size_t i = iLo; i <= iHi; i++ )
		{
			g_World.m_TileTypes.SetAtGrow( i, this );
		}
		return true;
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}
