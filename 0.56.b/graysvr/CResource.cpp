//
// CResource.cpp
// Copyright Menace Software (www.menasoft.com).
//


#include "graysvr.h"	// predef header.
#include "../common/grayver.h"
#include "../common/CFileList.h"

CResource::CResource()
{
	m_timePeriodic.Init();

	m_fUseNTService = false;
	m_fUseHTTP		= 2;
	m_fUseAuthID	= false;
	m_iMapCacheTime = 2 * 60 * TICK_PER_SEC;
	m_iSectorSleepMask = (1<<10)-1;

	m_wDebugFlags = 0; //DEBUGF_NPC_EMOTE
	m_fSecure = true;
	m_iFreezeRestartTime = 10;
	m_bAgree = false;

	//Magic
	m_fReagentsRequired = true;
	m_fReagentLossFail = true;
	m_iWordsOfPowerColor = HUE_TEXT_DEF;
	m_iWordsOfPowerFont = FONT_NORMAL;
	m_fWordsOfPowerPlayer = true;
	m_fWordsOfPowerStaff = false;
	m_fEquippedCast = true;
	m_iMagicUnlockDoor = 1000;

	m_iSpell_Teleport_Effect_Staff = ITEMID_FX_FLAMESTRIKE;	// drama
	m_iSpell_Teleport_Sound_Staff = 0x1f3;
	m_iSpell_Teleport_Effect_Players = ITEMID_FX_TELE_VANISH;
	m_iSpell_Teleport_Sound_Players = 0x01fe;

	// Decay
	m_iDecay_Item = 30*60*TICK_PER_SEC;
	m_iDecay_CorpsePlayer = 45*60*TICK_PER_SEC;
	m_iDecay_CorpseNPC = 15*60*TICK_PER_SEC;

	// Accounts
	m_iClientsMax		= FD_SETSIZE-1;
	m_iConnectingMax	= 24;
	m_iConnectingMaxIP	= 8;
	m_iClientsMaxIP		= 0;

	m_iGuestsMax = 0;
	m_iArriveDepartMsg = 1;
	m_iClientLingerTime = 60 * TICK_PER_SEC;
	m_iDeadSocketTime = 5*60*TICK_PER_SEC;
	m_iMinCharDeleteTime = 3*24*60*60*TICK_PER_SEC;
	m_iMaxCharsPerAccount = MAX_CHARS_PER_ACCT;
	m_fLocalIPAdmin = true;

	// Save
	m_iSaveBackupLevels = 3;
	m_iSaveBackgroundTime = 5*60*TICK_PER_SEC;	// Use the new background save.
	m_fSaveGarbageCollect = false;	// Always force a full garbage collection.
	m_iSavePeriod = 15*60*TICK_PER_SEC;

	// In game effects.
	m_fMonsterFight		= true;
	m_fMonsterFear		= true;
	m_iLightDungeon		= 17;
	m_iLightNight		= 17;	// dark before t2a.
	m_iLightDay		= LIGHT_BRIGHT;
	m_iBankIMax		= 1000;
	m_iBankWMax		= 400 * WEIGHT_UNITS;
	m_fGuardsInstantKill	= true;
	m_iSnoopCriminal	= 500;
	m_iTrainSkillPercent	= 50;
	m_iTrainSkillMax	= 500;
	m_iSkillPracticeMax	= 300;
	m_fCharTags		= true;
	m_iVendorMaxSell	= 30;
	m_iGameMinuteLength	= 8 * TICK_PER_SEC;
	m_fNoWeather		= false;
	m_fFlipDroppedItems	= true;
	m_iMurderMinCount	= 5;
	m_iMurderDecayTime	= 8*60*60* TICK_PER_SEC;
	m_iMaxCharComplexity	= 16;
	m_iMaxItemComplexity	= 25;
	m_iMaxSectorComplexity	= 1024;
	m_iPlayerKarmaNeutral	= -2000; // How much bad karma makes a player neutral?
	m_iPlayerKarmaEvil	= -8000;
	m_iGuardLingerTime	= 1*60*TICK_PER_SEC; // "GUARDLINGER",
	m_iCriminalTimer	= 3*60*TICK_PER_SEC;
	m_iHitpointPercentOnRez	= 10;
	m_iHitsHungerLoss	= 0;
	m_fLootingIsACrime	= true;
	m_fHelpingCriminalsIsACrime = true;
	m_fGenericSounds	= true;
	m_fAutoNewbieKeys 	= true;
	m_iMaxBaseSkill		= 250;
	m_fInitHiddenSkills		= false;
	m_iStamRunningPenalty 	= 50;
	m_iStaminaLossAtWeight 	= 100;
	m_iMountHeight		= PLAYER_HEIGHT + 5;
	m_iMoveRate			= 100;
	m_iArcheryMinDist	= 2;
	m_iArcheryMaxDist	= 12;
	m_iHitsUpdateRate	= TICK_PER_SEC;
	m_iSpeedScaleFactor 	= 15000;
	m_iCombatFlags		= 0;
	m_iMagicFlags		= 0;

	m_fNoResRobe		= 0;
	m_iLostNPCTeleport	= 0;
	m_iExperimental		= 0;
	m_iDistanceYell		= UO_MAP_VIEW_RADAR;
	m_iDistanceWhisper	= 3;
	m_iDistanceTalk		= UO_MAP_VIEW_SIZE;
	m_iOptionFlags		= 0;

	m_iMaxSkill		= SKILL_SCRIPTED;
	m_iWalkBuffer		= 50;
	m_iWalkRegen		= 25;
	m_iWoolGrowthTime	= 30*60*TICK_PER_SEC;

	m_iCommandLog		= 0;
	m_pEventsPetLink 	= NULL;
	m_pEventsPlayerLink	= NULL;
	m_pEventsRegionLink	= NULL;

	m_fUsecrypt 		= true; // Server want crypt client ?
	m_fUsenocrypt		= false; // Server want un-crypt client ? (version guessed by cliver)
	m_fPayFromPackOnly	= 0; // pay vendors from packs only

	m_iOverSkillMultiply 	= 0;
	m_fSuppressCapitals = false;

	m_iAdvancedLos		= 0;

	// New ones
	m_iFeatureT2A		= (FEATURE_T2A_UPDATE|FEATURE_T2A_CHAT);
	m_iFeatureLBR		= 0;
	m_iFeatureAOS		= 0;
	m_iFeatureSE		= 0;
	m_iFeatureML		= 0;

	m_iStatFlag = 0;

	m_iNpcAi = 0;
	m_iMaxLoopTimes = 10000;

	m_iAutoResDisp = RDS_T2A;
	m_iAutoPrivFlags = PRIV_DETAIL;

	//	Experience
	m_bExperienceSystem = false;
	m_iExperienceMode = 0;
	m_iExperienceKoefPVP = 100;
	m_iExperienceKoefPVM = 100;
	m_bLevelSystem = false;
	m_iLevelMode = 1;
	m_iLevelNextAt = 0;

#ifndef _EXTERNAL_DLL
	//	mySQL support
	m_bMySql = false;
#else
	m_sDbDll.Empty();
	m_iDbDllQueryBuffer = DEFAULT_RESULT_SIZE;
#endif

	m_cCommandPrefix = '.';
	m_iMaxTooltipForTick = 7;

	m_iRegenRate[STAT_STR] = 6*TICK_PER_SEC;		// Seconds to heal ONE hp (before stam/food adjust)
	m_iRegenRate[STAT_INT] = 5*TICK_PER_SEC;		// Seconds to heal ONE mn
	m_iRegenRate[STAT_DEX] = 3*TICK_PER_SEC;		// Seconds to heal ONE stm
	m_iRegenRate[STAT_FOOD] = 30*60*TICK_PER_SEC;	// Food usage (1 time per 30 minutes)
	m_iRegenRate[STAT_KARMA] = 0;					// Karma doesn't drop
	m_iRegenRate[STAT_FAME] = 12*60*60*TICK_PER_SEC;// Fame drop (1 time per x minutes)

	m_iTimerCall = 0;
	m_bAllowLightOverride = true;
	m_sZeroPoint= "1323,1624,0";
	m_bAllowBuySellAgent = false;

	m_iColorNotoCriminal = 0x03b2;		// grey (criminal)
	m_iColorNotoDefault = 0x03b2;		// grey (if not any other)
	m_iColorNotoEvil = 0x0026;			// red
	m_iColorNotoGood = 0x0063;			// blue
	m_iColorNotoGuildSame = 0x0044;		// green
	m_iColorNotoGuildWar = 0x002b;		// orange (enemy guild)
	m_iColorNotoNeutral = 0x03b2;		// grey (can be attacked)
}

CResource::~CResource()
{
	for ( int i=0; i<COUNTOF(m_ResHash.m_Array); i++ )
	for ( int j=0; j<m_ResHash.m_Array[i].GetCount(); j++ )
	{
		CResourceDef* pResDef = m_ResHash.m_Array[i][j];
		if ( pResDef )
		{
			pResDef->UnLink();
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
	if ( iResType < 0 )
	{
		*pszSep = oldChar;
		return( false );
	}

	*pszSep = '.';

	// Now get the index.
	pszKey = pszSep+1;
	if ( pszKey[0] == ' \0' )
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
		//pRef = CCharBase::FindCharBase((CREID_TYPE)Exp_GetVal(pszKey));
		pRef = CCharBase::FindCharBase((CREID_TYPE)g_Cfg.ResourceGetIndexType(RES_CHARDEF, pszKey));
	}
	else if ( iResType == RES_ITEMDEF )
	{
		pRef = CItemBase::FindItemBase((ITEMID_TYPE)g_Cfg.ResourceGetIndexType(RES_ITEMDEF, pszKey));
	}
	else if ( iResType == RES_SPELL && *pszKey == '-' )
	{
		pszKey++;
		int		iOrder	= Exp_GetVal( pszKey );
		if ( !m_SpellDefs_Sorted.IsValidIndex( iOrder ) )
			pRef	= NULL;
		else
			pRef	= m_SpellDefs_Sorted[iOrder];
	}
	else
	{
		RESOURCE_ID	rid	= ResourceGetID( (RES_TYPE) iResType, pszKey );
		pRef = ResourceGetDef( rid );
	}

	if ( pszSep != NULL )
	{
		*pszSep = '.';
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
	RC_ACCTFILES,			// m_sAcctBaseDir
	RC_ADVANCEDLOS,			// m_iAdvancedLos
	RC_AGREE,
	RC_ALLOWBUYSELLAGENT,	// m_bAllowBuySellAgent
	RC_ALLOWLIGHTOVERRIDE,	// m_bAllowLightOverride
	RC_ARCHERYMAXDIST,		// m_iArcheryMaxDist
	RC_ARCHERYMINDIST,		// m_iArcheryMinDist
	RC_ARRIVEDEPARTMSG,
	RC_AUTONEWBIEKEYS,		// m_fAutoNewbieKeys
	RC_AUTOPRIVFLAGS,		// m_iAutoPrivFlags
	RC_AUTORESDISP,			// m_iAutoResDisp
	RC_BACKUPLEVELS,		// m_iSaveBackupLevels
	RC_BANKMAXITEMS,
	RC_BANKMAXWEIGHT,
	RC_CHARTAGS,			// m_fCharTags
	RC_CLIENTLINGER,
	RC_CLIENTMAX,			// m_iClientsMax
	RC_CLIENTMAXIP,			// m_iClientsMaxIP
	RC_CLIENTS,
	RC_COLORNOTOCRIMINAL,	// m_iColorNotoCriminal
	RC_COLORNOTODEFAULT,	// m_iColorNotoDefault
	RC_COLORNOTOEVIL,		// m_iColorNotoEvil
	RC_COLORNOTOGOOD,		// m_iColorNotoGood
	RC_COLORNOTOGUILDSAME,	// m_iColorNotoGuildSame
	RC_COLORNOTOGUILDWAR,	// m_iColorNotoGuildWar
	RC_COLORNOTONEUTRAL,	// m_iColorNotoNeutral
	RC_COMBATFLAGS,			// m_iCombatFlags
	RC_COMMANDLOG,
	RC_COMMANDPREFIX,
	RC_COMMANDTRIGGER,		// m_sCommandTrigger
	RC_CONNECTINGMAX,		// m_iConnectingMax
	RC_CONNECTINGMAXIP,		// m_iConnectingMaxIP
	RC_CORPSENPCDECAY,
	RC_CORPSEPLAYERDECAY,
	RC_CRIMINALTIMER,		// m_iCriminalTimer
#ifdef _EXTERNAL_DLL
	RC_DBDLL,				//	m_sDbDll
	RC_DBDLLDATABASE,		//	m_sDbDllDB
	RC_DBDLLHOST,			//	m_sDbDllHost
	RC_DBDLLPASSWORD,		//	m_sDbDllPass
	RC_DBDLLQUERYBUFFER,	//	m_iDbDllQueryBuffer
	RC_DBDLLUSER,			//	m_sDbDllUser
#endif
	RC_DEADSOCKETTIME,
	RC_DEBUGFLAGS,
	RC_DECAYTIMER,
	RC_DISTANCETALK,
	RC_DISTANCEWHISPER,
	RC_DISTANCEYELL,
#ifdef _DUMPSUPPORT
	RC_DUMPPACKETSFORACC,
#endif
	RC_DUNGEONLIGHT,
	RC_EQUIPPEDCAST,		// m_fEquippedCast
	RC_EVENTSPET,			// m_sEventsPet
	RC_EVENTSPLAYER,		// m_sEventsPlayer
	RC_EVENTSREGION,		// m_sEventsRegion
	RC_EXPERIENCEKOEFPVM,	// m_iExperienceKoefPVM
	RC_EXPERIENCEKOEFPVP,	// m_iExperienceKoefPVP
	RC_EXPERIENCEMODE,		// m_iExperienceMode
	RC_EXPERIENCESYSTEM,	// m_bExperienceSystem
	RC_EXPERIMENTAL,		// m_iExperimental
	RC_FEATURESAOS,
	RC_FEATURESLBR,
	RC_FEATURESML,
	RC_FEATURESSE,
	RC_FEATUREST2A,
	RC_FLIPDROPPEDITEMS,	// m_fFlipDroppedItems
	RC_FORCEGARBAGECOLLECT,	// m_fSaveGarbageCollect
	RC_FREEZERESTARTTIME,	// m_iFreezeRestartTime
	RC_GAMEMINUTELENGTH,	// m_iGameMinuteLength
	RC_GENERICSOUNDS,		// m_fGenericSounds
	RC_GUARDLINGER,			// m_iGuardLingerTime
	RC_GUARDSINSTANTKILL,
	RC_GUESTSMAX,
	RC_GUILDS,
	RC_HEARALL,
	RC_HELPINGCRIMINALSISACRIME,	// m_fHelpingCriminalsIsACrime
	RC_HITPOINTPERCENTONREZ, // m_iHitpointPercentOnRez
	RC_HITSHUNGERLOSS,		// m_iHitsHungerLoss
	RC_HITSUPDATERATE,
	RC_INITHIDDENSKILLS,	// m_fInitHiddenSkills
	RC_LEVELMODE,			// m_iLevelMode
	RC_LEVELNEXTAT,			// m_iLevelNextAt
	RC_LEVELSYSTEM,			// m_bLevelSystem
	RC_LIGHTDAY,			// m_iLightDay
	RC_LIGHTNIGHT,			// m_iLightNight
	RC_LOCALIPADMIN,		// m_fLocalIPAdmin
	RC_LOG,
	RC_LOGMASK,				// GetLogMask
	RC_LOOTINGISACRIME,		// m_fLootingIsACrime
	RC_LOSTNPCTELEPORT,		// m_fLostNPCTeleport
	RC_MAGICFLAGS,
	RC_MAGICUNLOCKDOOR,		// m_iMagicUnlockDoor
	RC_MAPCACHETIME,
	RC_MAXBASESKILL,			// m_iMaxBaseSkill
	RC_MAXCHARSPERACCOUNT,	// m_iMaxCharsPerAccount
	RC_MAXCOMPLEXITY,			// m_iMaxCharComplexity
	RC_MAXITEMCOMPLEXITY,		// m_iMaxItemComplexity
	RC_MAXLOOPTIMES,			// m_iMaxLoopTimes
	RC_MAXSECTORCOMPLEXITY,		// m_iMaxSectorComplexity
	RC_MAXSKILL,
	RC_MAXTOOLTIPFORTICK,
	RC_MD5PASSWORDS,			// m_fMd5Passwords
	RC_MINCHARDELETETIME,
	RC_MONSTERFEAR,			// m_fMonsterFear
	RC_MONSTERFIGHT,
	RC_MOUNTHEIGHT,			// m_iMountHeight
	RC_MOVERATE,				//	m_iMoveRate
	RC_MULFILES,
	RC_MURDERDECAYTIME,		// m_iMurderDecayTime;
	RC_MURDERMINCOUNT,			//	m_iMurderMinCount
#ifndef _EXTERNAL_DLL
	RC_MYSQL,					//	m_bMySql
	RC_MYSQLDB,					//	m_sMySqlDatabase
	RC_MYSQLHOST,				//	m_sMySqlHost
	RC_MYSQLPASS,				//	m_sMySqlPassword
	RC_MYSQLUSER,				//	m_sMySqlUser
#endif
	RC_NORESROBE,
	RC_NOWEATHER,				// m_fNoWeather
	RC_NPCAI,					// m_iNpcAi
	RC_NPCTRAINMAX,			// m_iTrainSkillMax
	RC_NPCTRAINPERCENT,			// m_iTrainSkillPercent
	RC_NTSERVICE,				// m_fUseNTService
	RC_OPTIONFLAGS,			// m_iOptionFlags
	RC_OVERSKILLMULTIPLY,		//	m_iOverSkillMultiply
	RC_PAYFROMPACKONLY,			//	m_fPayFromPackOnly
	RC_PLAYERNEUTRAL,		// m_iPlayerKarmaNeutral
	RC_PROFILE,
	RC_REAGENTLOSSFAIL,			// m_fReagentLossFail
	RC_REAGENTSREQUIRED,
	RC_RTIME,
	RC_RUNNINGPENALTY,		// m_iStamRunningPenalty
	RC_SAVEBACKGROUND,			// m_iSaveBackgroundTime
	RC_SAVEPERIOD,
	RC_SCPFILES,
	RC_SECTORSLEEP,				// m_iSectorSleepMask
	RC_SECURE,
	RC_SKILLPRACTICEMAX,	// m_iSkillPracticeMax
	RC_SNOOPCRIMINAL,
	RC_SPEECHOTHER,
	RC_SPEECHPET,
	RC_SPEECHSELF,
	RC_SPEEDSCALEFACTOR,
	RC_STAMINALOSSATWEIGHT,	// m_iStaminaLossAtWeight
	RC_STATSFLAGS,				//	m_iStatFlag
	RC_STRIPPATH,				// for TNG
	RC_SUPPRESSCAPITALS,
	RC_TELEPORTEFFECTPLAYERS,	//	m_iSpell_Teleport_Effect_Players
	RC_TELEPORTEFFECTSTAFF,		//	m_iSpell_Teleport_Effect_Staff
	RC_TIMERCALL,				//	m_iTimerCall
	RC_TIMEUP,
	RC_USEAUTHID,				// m_fUseAuthID
	RC_USECRYPT,				// m_Usecrypt
	RC_USEHTTP,					// m_fUseHTTP
	RC_USENOCRYPT,				// m_Usenocrypt
	RC_VENDORMAXSELL,			// m_iVendorMaxSell
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
	RC_QTY,
};

const CAssocReg CResource::sm_szLoadKeys[RC_QTY+1] =
{
	{ "ACCTFILES",				{ ELEM_CSTRING,	OFFSETOF(CResource,m_sAcctBaseDir)	}},
	{ "ADVANCEDLOS",			{ ELEM_INT,		OFFSETOF(CResource,m_iAdvancedLos)	}},
	{ "AGREE",					{ ELEM_BOOL,	OFFSETOF(CResource,m_bAgree) }},
	{ "ALLOWBUYSELLAGENT",		{ ELEM_BOOL,	OFFSETOF(CResource,m_bAllowBuySellAgent)	}},
	{ "ALLOWLIGHTOVERRIDE",		{ ELEM_BOOL,	OFFSETOF(CResource,m_bAllowLightOverride)	}},
	{ "ARCHERYMAXDIST",			{ ELEM_INT,		OFFSETOF(CResource,m_iArcheryMaxDist) }},
	{ "ARCHERYMINDIST",			{ ELEM_INT,		OFFSETOF(CResource,m_iArcheryMinDist) }},
	{ "ARRIVEDEPARTMSG",		{ ELEM_INT,		OFFSETOF(CResource,m_iArriveDepartMsg)	}},
	{ "AUTONEWBIEKEYS",			{ ELEM_BOOL,	OFFSETOF(CResource,m_fAutoNewbieKeys)	}},
	{ "AUTOPRIVFLAGS",			{ ELEM_INT,		OFFSETOF(CResource,m_iAutoPrivFlags)	}},
	{ "AUTORESDISP",			{ ELEM_INT,		OFFSETOF(CResource,m_iAutoResDisp)	}},
	{ "BACKUPLEVELS",			{ ELEM_INT,		OFFSETOF(CResource,m_iSaveBackupLevels)	}},
	{ "BANKMAXITEMS",			{ ELEM_INT,		OFFSETOF(CResource,m_iBankIMax)	}},
	{ "BANKMAXWEIGHT",			{ ELEM_INT,		OFFSETOF(CResource,m_iBankWMax)	}},
	{ "CHARTAGS",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fCharTags)	}},
	{ "CLIENTLINGER",			{ ELEM_INT,		OFFSETOF(CResource,m_iClientLingerTime)	}},
	{ "CLIENTMAX",				{ ELEM_INT,		OFFSETOF(CResource,m_iClientsMax)	}},
	{ "CLIENTMAXIP",			{ ELEM_INT,		OFFSETOF(CResource,m_iClientsMaxIP)	}},
	{ "CLIENTS" },	// duplicate
	{ "COLORNOTOCRIMINAL",		{ ELEM_INT,		OFFSETOF(CResource,m_iColorNotoCriminal)	}},
	{ "COLORNOTODEFAULT",		{ ELEM_INT,		OFFSETOF(CResource,m_iColorNotoDefault)		}},
	{ "COLORNOTOEVIL",			{ ELEM_INT,		OFFSETOF(CResource,m_iColorNotoEvil)		}},
	{ "COLORNOTOGOOD",			{ ELEM_INT,		OFFSETOF(CResource,m_iColorNotoGood)		}},
	{ "COLORNOTOGUILDSAME",		{ ELEM_INT,		OFFSETOF(CResource,m_iColorNotoGuildSame)	}},
	{ "COLORNOTOGUILDWAR",		{ ELEM_INT,		OFFSETOF(CResource,m_iColorNotoGuildWar)	}},
	{ "COLORNOTONEUTRAL",		{ ELEM_INT,		OFFSETOF(CResource,m_iColorNotoNeutral)		}},
	{ "COMBATFLAGS",			{ ELEM_INT,		OFFSETOF(CResource,m_iCombatFlags)		}},
	{ "COMMANDLOG",				{ ELEM_INT,		OFFSETOF(CResource,m_iCommandLog)		}},
	{ "COMMANDPREFIX",			{ ELEM_BYTE,	OFFSETOF(CResource,m_cCommandPrefix)	}},
	{ "COMMANDTRIGGER",			{ ELEM_CSTRING,	OFFSETOF(CResource,m_sCommandTrigger)	}},
	{ "CONNECTINGMAX",			{ ELEM_INT,		OFFSETOF(CResource,m_iConnectingMax)	}},
	{ "CONNECTINGMAXIP",		{ ELEM_INT,		OFFSETOF(CResource,m_iConnectingMaxIP)	}},
	{ "CORPSENPCDECAY",			{ ELEM_INT,		OFFSETOF(CResource,m_iDecay_CorpseNPC)	}},
	{ "CORPSEPLAYERDECAY",		{ ELEM_INT,		OFFSETOF(CResource,m_iDecay_CorpsePlayer) }},
	{ "CRIMINALTIMER",			{ ELEM_INT,		OFFSETOF(CResource,m_iCriminalTimer)	}},
#ifdef _EXTERNAL_DLL
	{ "DBDLL",					{ ELEM_CSTRING,	OFFSETOF(CResource,m_sDbDll)		}},
	{ "DBDLLDATABASE",			{ ELEM_CSTRING,	OFFSETOF(CResource,m_sDbDllDB)		}},
	{ "DBDLLHOST",				{ ELEM_CSTRING, OFFSETOF(CResource,m_sDbDllHost)	}},
	{ "DBDLLPASSWORD",			{ ELEM_CSTRING,	OFFSETOF(CResource,m_sDbDllPass)	}},
	{ "DBDLLQUERYBUFFER",		{ ELEM_INT,		OFFSETOF(CResource,m_iDbDllQueryBuffer)	}},
	{ "DBDLLUSER",				{ ELEM_CSTRING,	OFFSETOF(CResource,m_sDbDllUser)	}},
#endif
	{ "DEADSOCKETTIME",			{ ELEM_INT,		OFFSETOF(CResource,m_iDeadSocketTime)	}},
	{ "DEBUGFLAGS",				{ ELEM_WORD,	OFFSETOF(CResource,m_wDebugFlags)	}},
	{ "DECAYTIMER",				{ ELEM_INT,		OFFSETOF(CResource,m_iDecay_Item)	}},
	{ "DISTANCETALK",			{ ELEM_INT,		OFFSETOF(CResource,m_iDistanceTalk )	}},
	{ "DISTANCEWHISPER",		{ ELEM_INT,		OFFSETOF(CResource,m_iDistanceWhisper )	}},
	{ "DISTANCEYELL",			{ ELEM_INT,		OFFSETOF(CResource,m_iDistanceYell )	}},
#ifdef _DUMPSUPPORT
	{ "DUMPPACKETSFORACC",		{ ELEM_CSTRING,	OFFSETOF(CResource,m_sDumpAccPackets)	}},
#endif
	{ "DUNGEONLIGHT",			{ ELEM_INT,		OFFSETOF(CResource,m_iLightDungeon)	}},
	{ "EQUIPPEDCAST",			{ ELEM_BOOL,	OFFSETOF(CResource,m_fEquippedCast)	}},
	{ "EVENTSPET",				{ ELEM_CSTRING,	OFFSETOF(CResource,m_sEventsPet)	}},
	{ "EVENTSPLAYER",			{ ELEM_CSTRING,	OFFSETOF(CResource,m_sEventsPlayer)	}},
	{ "EVENTSREGION",			{ ELEM_CSTRING,	OFFSETOF(CResource,m_sEventsRegion)	}},
	{ "EXPERIENCEKOEFPVM",		{ ELEM_INT,		OFFSETOF(CResource,m_iExperienceKoefPVM)}},
	{ "EXPERIENCEKOEFPVP",		{ ELEM_INT,		OFFSETOF(CResource,m_iExperienceKoefPVP)}},
	{ "EXPERIENCEMODE",			{ ELEM_INT,		OFFSETOF(CResource,m_iExperienceMode)	}},
	{ "EXPERIENCESYSTEM",		{ ELEM_BOOL,	OFFSETOF(CResource,m_bExperienceSystem)	}},
	{ "EXPERIMENTAL",			{ ELEM_INT,		OFFSETOF(CResource,m_iExperimental)	}},
	{ "FEATUREAOS",				{ ELEM_INT,		OFFSETOF(CResource,m_iFeatureAOS)	}},
	{ "FEATURELBR",				{ ELEM_INT,		OFFSETOF(CResource,m_iFeatureLBR)	}},
	{ "FEATUREML",				{ ELEM_INT,		OFFSETOF(CResource,m_iFeatureML)	}},
	{ "FEATURESE",				{ ELEM_INT,		OFFSETOF(CResource,m_iFeatureSE)	}},
	{ "FEATURET2A",				{ ELEM_INT,		OFFSETOF(CResource,m_iFeatureT2A)	}},
	{ "FLIPDROPPEDITEMS",		{ ELEM_BOOL,	OFFSETOF(CResource,m_fFlipDroppedItems)	}},
	{ "FORCEGARBAGECOLLECT",	{ ELEM_BOOL,	OFFSETOF(CResource,m_fSaveGarbageCollect) }},
	{ "FREEZERESTARTTIME",		{ ELEM_INT,		OFFSETOF(CResource,m_iFreezeRestartTime)	}},
	{ "GAMEMINUTELENGTH",		{ ELEM_INT,		OFFSETOF(CResource,m_iGameMinuteLength)	}},
	{ "GENERICSOUNDS",			{ ELEM_BOOL,	OFFSETOF(CResource,m_fGenericSounds)	}},
	{ "GUARDLINGER",			{ ELEM_INT,		OFFSETOF(CResource,m_iGuardLingerTime)	}},
	{ "GUARDSINSTANTKILL",		{ ELEM_BOOL,	OFFSETOF(CResource,m_fGuardsInstantKill)	}},
	{ "GUESTSMAX",				{ ELEM_INT,		OFFSETOF(CResource,m_iGuestsMax)	}},
	{ "GUILDS" },
	{ "HEARALL"	},
	{ "HELPINGCRIMINALSISACRIME",{ ELEM_BOOL,	OFFSETOF(CResource,m_fHelpingCriminalsIsACrime)	}},
	{ "HITPOINTPERCENTONREZ",	{ ELEM_INT,		OFFSETOF(CResource,m_iHitpointPercentOnRez)	} },
	{ "HITSHUNGERLOSS",			{ ELEM_INT,		OFFSETOF(CResource,m_iHitsHungerLoss) }},
	{ "HITSUPDATERATE" },
	{ "INITHIDDENSKILLS",		{ ELEM_BOOL,	OFFSETOF(CResource,m_fInitHiddenSkills)	}},
	{ "LEVELMODE",				{ ELEM_INT,		OFFSETOF(CResource,m_iLevelMode)	}},
	{ "LEVELNEXTAT",			{ ELEM_INT,		OFFSETOF(CResource,m_iLevelNextAt)	}},
	{ "LEVELSYSTEM",			{ ELEM_BOOL,	OFFSETOF(CResource,m_bLevelSystem)	}},
	{ "LIGHTDAY",				{ ELEM_INT,		OFFSETOF(CResource,m_iLightDay)	} },
	{ "LIGHTNIGHT",				{ ELEM_INT,		OFFSETOF(CResource,m_iLightNight)	} },
	{ "LOCALIPADMIN",			{ ELEM_BOOL,	OFFSETOF(CResource,m_fLocalIPAdmin)	} }, // The local ip is assumed to be the admin.
	{ "LOG" },
	{ "LOGMASK" },		// GetLogMask
	{ "LOOTINGISACRIME",		{ ELEM_BOOL,	OFFSETOF(CResource,m_fLootingIsACrime)	}},
	{ "LOSTNPCTELEPORT",		{ ELEM_INT,		OFFSETOF(CResource,m_iLostNPCTeleport)	}},
	{ "MAGICFLAGS",				{ ELEM_INT,		OFFSETOF(CResource,m_iMagicFlags)	}},
	{ "MAGICUNLOCKDOOR",		{ ELEM_INT,		OFFSETOF(CResource,m_iMagicUnlockDoor)	}},
	{ "MAPCACHETIME",			{ ELEM_INT,		OFFSETOF(CResource,m_iMapCacheTime)	}},
	{ "MAXBASESKILL",			{ ELEM_INT,		OFFSETOF(CResource,m_iMaxBaseSkill)	}},
	{ "MAXCHARSPERACCOUNT",		{ ELEM_INT,		OFFSETOF(CResource,m_iMaxCharsPerAccount)	}},
	{ "MAXCOMPLEXITY",			{ ELEM_INT,		OFFSETOF(CResource,m_iMaxCharComplexity)	}},
	{ "MAXITEMCOMPLEXITY",		{ ELEM_INT,		OFFSETOF(CResource,m_iMaxItemComplexity)	}},
	{ "MAXLOOPTIMES",			{ ELEM_INT,		OFFSETOF(CResource,m_iMaxLoopTimes)			}},
	{ "MAXSECTORCOMPLEXITY",	{ ELEM_INT,		OFFSETOF(CResource,m_iMaxSectorComplexity)	}},
	{ "MAXSKILL",				{ ELEM_INT,		OFFSETOF(CResource,m_iMaxSkill)	}},
	{ "MAXTOOLTIPFORTICK",		{ ELEM_INT,		OFFSETOF(CResource,m_iMaxTooltipForTick)	}},
	{ "MD5PASSWORDS",			{ ELEM_BOOL,	OFFSETOF(CResource,m_fMd5Passwords) }},
	{ "MINCHARDELETETIME",		{ ELEM_INT,		OFFSETOF(CResource,m_iMinCharDeleteTime)	}},
	{ "MONSTERFEAR",			{ ELEM_BOOL,	OFFSETOF(CResource,m_fMonsterFear)	}},
	{ "MONSTERFIGHT",			{ ELEM_BOOL,	OFFSETOF(CResource,m_fMonsterFight)	}},
	{ "MOUNTHEIGHT",			{ ELEM_INT,		OFFSETOF(CResource,m_iMountHeight)	}},
	{ "MOVERATE",				{ ELEM_INT,		OFFSETOF(CResource,m_iMoveRate)		}},
	{ "MULFILES" },
	{ "MURDERDECAYTIME",		{ ELEM_INT,		OFFSETOF(CResource,m_iMurderDecayTime)	}},
	{ "MURDERMINCOUNT",			{ ELEM_INT,		OFFSETOF(CResource,m_iMurderMinCount)	}}, // amount of murders before we get title.
#ifndef _EXTERNAL_DLL
	{ "MYSQL",					{ ELEM_BOOL,	OFFSETOF(CResource,m_bMySql)		}},
	{ "MYSQLDATABASE",			{ ELEM_CSTRING,	OFFSETOF(CResource,m_sMySqlDB)		}},
	{ "MYSQLHOST",				{ ELEM_CSTRING, OFFSETOF(CResource,m_sMySqlHost)	}},
	{ "MYSQLPASSWORD",			{ ELEM_CSTRING,	OFFSETOF(CResource,m_sMySqlPass)	}},
	{ "MYSQLUSER",				{ ELEM_CSTRING,	OFFSETOF(CResource,m_sMySqlUser)	}},
#endif
	{ "NORESROBE",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fNoResRobe)	}},
	{ "NOWEATHER",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fNoWeather)	}},
	{ "NPCAI",					{ ELEM_INT,		OFFSETOF(CResource,m_iNpcAi)		}},
	{ "NPCTRAINMAX",			{ ELEM_INT,		OFFSETOF(CResource,m_iTrainSkillMax)	}},
	{ "NPCTRAINPERCENT",		{ ELEM_INT,		OFFSETOF(CResource,m_iTrainSkillPercent) }},
	{ "NTSERVICE",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fUseNTService)	}},
	{ "OPTIONFLAGS",			{ ELEM_INT,		OFFSETOF(CResource,m_iOptionFlags)	}},
	{ "OVERSKILLMULTIPLY",		{ ELEM_INT,		OFFSETOF(CResource,m_iOverSkillMultiply)	}},
	{ "PAYFROMPACKONLY",		{ ELEM_BOOL,	OFFSETOF(CResource,m_fPayFromPackOnly)	}},
	{ "PLAYERNEUTRAL",			{ ELEM_INT,		OFFSETOF(CResource,m_iPlayerKarmaNeutral)	}},
	{ "PROFILE" },
	{ "REAGENTLOSSFAIL",		{ ELEM_BOOL,	OFFSETOF(CResource,m_fReagentLossFail)	}},
	{ "REAGENTSREQUIRED",		{ ELEM_BOOL,	OFFSETOF(CResource,m_fReagentsRequired)	}},
	{ "RTIME" },
	{ "RUNNINGPENALTY",			{ ELEM_INT,		OFFSETOF(CResource,m_iStamRunningPenalty)	}},
	{ "SAVEBACKGROUND",			{ ELEM_INT,		OFFSETOF(CResource,m_iSaveBackgroundTime)	}},
	{ "SAVEPERIOD",				{ ELEM_INT,		OFFSETOF(CResource,m_iSavePeriod)	}},
	{ "SCPFILES",				{ ELEM_CSTRING,	OFFSETOF(CResource,m_sSCPBaseDir)	}},
	{ "SECTORSLEEP",			{ ELEM_INT,		OFFSETOF(CResource,m_iSectorSleepMask)	}},
	{ "SECURE",					{ ELEM_BOOL,	OFFSETOF(CResource,m_fSecure)	}},
	{ "SKILLPRACTICEMAX",		{ ELEM_INT,		OFFSETOF(CResource,m_iSkillPracticeMax) }},
	{ "SNOOPCRIMINAL",			{ ELEM_INT,		OFFSETOF(CResource,m_iSnoopCriminal)	}},
	{ "SPEECHOTHER",			{ ELEM_CSTRING,	OFFSETOF(CResource,m_sSpeechOther )	}},
	{ "SPEECHPET",				{ ELEM_CSTRING,	OFFSETOF(CResource,m_sSpeechPet )	}},
	{ "SPEECHSELF",				{ ELEM_CSTRING,	OFFSETOF(CResource,m_sSpeechSelf)	}},
	{ "SPEEDSCALEFACTOR",		{ ELEM_INT,		OFFSETOF(CResource,m_iSpeedScaleFactor)	}},
	{ "STAMINALOSSATWEIGHT",	{ ELEM_INT,		OFFSETOF(CResource,m_iStaminaLossAtWeight)	}},
	{ "STATSFLAGS",				{ ELEM_INT,		OFFSETOF(CResource,m_iStatFlag) }},
	{ "STRIPPATH",				{ ELEM_INT,		OFFSETOF(CResource,m_sStripPath) }},
	{ "SUPPRESSCAPITALS",		{ ELEM_BOOL,	OFFSETOF(CResource,m_fSuppressCapitals) }},
	{ "TELEPORTEFFECTPLAYERS",	{ ELEM_INT,		OFFSETOF(CResource,m_iSpell_Teleport_Effect_Players) }},
	{ "TELEPORTEFFECTSTAFF",	{ ELEM_INT,		OFFSETOF(CResource,m_iSpell_Teleport_Effect_Staff) }},
	{ "TIMERCALL",				{ ELEM_INT,		OFFSETOF(CResource,m_iTimerCall) }},
	{ "TIMEUP" },
	{ "USEAUTHID",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fUseAuthID)	}},	// we use authid like osi
	{ "USECRYPT",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fUsecrypt)	}},	// we don't want crypt clients
	{ "USEHTTP",				{ ELEM_INT,		OFFSETOF(CResource,m_fUseHTTP)	}},
	{ "USENOCRYPT",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fUsenocrypt)	}},	// we don't want no-crypt clients
	{ "VENDORMAXSELL",			{ ELEM_INT,		OFFSETOF(CResource,m_iVendorMaxSell) }},
	{ "VERSION" },
	{ "WALKBUFFER",				{ ELEM_INT,		OFFSETOF(CResource,m_iWalkBuffer) }},
	{ "WALKREGEN",				{ ELEM_INT,		OFFSETOF(CResource,m_iWalkRegen) }},
	{ "WOOLGROWTHTIME",			{ ELEM_INT,		OFFSETOF(CResource,m_iWoolGrowthTime) }},
	{ "WOPCOLOR",				{ ELEM_INT,		OFFSETOF(CResource,m_iWordsOfPowerColor)	}},
	{ "WOPFONT",				{ ELEM_INT,		OFFSETOF(CResource,m_iWordsOfPowerFont)	}},
	{ "WOPPLAYER",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fWordsOfPowerPlayer)	}},
	{ "WOPSTAFF",				{ ELEM_BOOL,	OFFSETOF(CResource,m_fWordsOfPowerStaff)	}},
	{ "WORLDSAVE",				{ ELEM_CSTRING,	OFFSETOF(CResource,m_sWorldBaseDir)	}},
	{ "ZEROPOINT",				{ ELEM_CSTRING,	OFFSETOF(CResource,m_sZeroPoint) }},
	{ NULL },
};

bool CResource::r_LoadVal( CScript &s )
{
	ADDTOCALLSTACK("CResource::r_LoadVal");
	EXC_TRY("LoadVal");

	int i = FindTableHeadSorted( s.GetKey(), (LPCTSTR const *) sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1, sizeof(sm_szLoadKeys[0]));
	if ( i < 0 )
	{
		if ( s.IsKeyHead( "REGEN", 5 ))			//	REGENx=<stat regeneration rate>
		{
			int index = ATOI(s.GetKey()+5);
			if (( index >= 0 ) || ( index < STAT_QTY ))
			{
				g_Cfg.m_iRegenRate[index] = (s.GetArgVal() * TICK_PER_SEC);
				return true;
			}
			else
				return false;
		}
		else if ( s.IsKeyHead("MAP", 3) )		//	MAPx=settings
		{
			return g_MapList.Load(ATOI(s.GetKey() + 3), s.GetArgRaw());
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

		return(false);
	}

	if ( i == RC_MAXSKILL && !g_Serv.IsLoading() )
		return false;

	switch (i)
	{
		case RC_AGREE:
			m_bAgree = s.GetArgVal();
			break;
		case RC_ACCTFILES:	// Put acct files here.
			m_sAcctBaseDir = CGFile::GetMergedFileName( s.GetArgStr(), "" );
			break;
		case RC_BANKMAXWEIGHT:
			m_iBankWMax = s.GetArgVal() * WEIGHT_UNITS;
			break;
		case RC_CLIENTLINGER:
			m_iClientLingerTime = s.GetArgVal() * TICK_PER_SEC;
			break;
		case RC_CLIENTMAX:
		case RC_CLIENTS:
			m_iClientsMax = s.GetArgVal();
			if ( m_iClientsMax > FD_SETSIZE-1 )	// Max number we can deal with. compile time thing.
			{
				m_iClientsMax = FD_SETSIZE-1;
			}
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
#ifdef _EXTERNAL_DLL
		case RC_DBDLL:
			{
				m_sDbDll.Empty();
				if ( s.HasArgs() )
				{
					TCHAR * pTmpName = Str_TrimWhitespace( s.GetArgStr() );
					if ( pTmpName && *pTmpName )
					{
						m_sDbDll = pTmpName;
					}

					cDatabaseLoader::ForceIstanceReload();
				}
			} break;

		case RC_DBDLLQUERYBUFFER:
			{
				if ( s.HasArgs() )
				{
					int iTempValue = s.GetArgVal();
					m_iDbDllQueryBuffer = ( iTempValue >= 0 ) ? iTempValue : DEFAULT_RESULT_SIZE;
				}
			} break;
#endif
		case RC_DEADSOCKETTIME:
			m_iDeadSocketTime = s.GetArgVal()*60*TICK_PER_SEC;
			break;
		case RC_DECAYTIMER:
			m_iDecay_Item = s.GetArgVal() *60*TICK_PER_SEC;
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
			m_iMaxCharsPerAccount = s.GetArgVal();
			if ( m_iMaxCharsPerAccount > MAX_CHARS_PER_ACCT )
				m_iMaxCharsPerAccount = MAX_CHARS_PER_ACCT;
			break;
		case RC_MINCHARDELETETIME:
			m_iMinCharDeleteTime = s.GetArgVal()*60*TICK_PER_SEC;
			break;
		case RC_MURDERDECAYTIME:
			m_iMurderDecayTime = s.GetArgVal() * TICK_PER_SEC;
			break;
		case RC_WOOLGROWTHTIME:
			m_iWoolGrowthTime = s.GetArgVal() * 60 * TICK_PER_SEC;
			break;
		case RC_PROFILE:
			g_Serv.m_Profile.SetActive(s.GetArgVal());
			break;
		case RC_PLAYERNEUTRAL:	// How much bad karma makes a player neutral?
			m_iPlayerKarmaNeutral = s.GetArgVal();
			if ( m_iPlayerKarmaEvil > m_iPlayerKarmaNeutral )
				m_iPlayerKarmaEvil = m_iPlayerKarmaNeutral;
			break;
		case RC_SCPFILES: // Get SCP files from here.
			m_sSCPBaseDir = CGFile::GetMergedFileName( s.GetArgStr(), "" );
			break;
		case RC_SECURE:
			m_fSecure = s.GetArgVal();
			if ( !g_Serv.IsLoading() )
				g_Serv.SetSignals();
			break;
		case RC_SKILLPRACTICEMAX:
			m_iSkillPracticeMax = s.GetArgVal();
			break;
		case RC_SAVEPERIOD:
			m_iSavePeriod = s.GetArgVal()*60*TICK_PER_SEC;
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

		case RC_WORLDSAVE: // Put save files here.
			m_sWorldBaseDir = CGFile::GetMergedFileName( s.GetArgStr(), "" );
			break;

		case RC_COMMANDPREFIX:
			m_cCommandPrefix = *s.GetArgStr();
			break;

		case RC_EXPERIMENTAL:
			g_Cfg.m_iExperimental = s.GetArgVal();
			PrintEFOFFlags(true, false);
			break;

		case RC_OPTIONFLAGS:
			g_Cfg.m_iOptionFlags = s.GetArgVal();
			PrintEFOFFlags(true, false);
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

	int		iLen	= strlen( pszKey );
    const CSkillDef *		pDef;
	for ( int i = 0; i < m_SkillIndexDefs.GetCount(); ++i )
	{
		pDef	= STATIC_CAST <const CSkillDef*>(m_SkillIndexDefs[i]);
		if ( pDef->m_sName.IsEmpty() ?
				!strnicmp( pszKey, pDef->GetKey(), iLen )
			:	!strnicmp( pszKey, pDef->m_sName, iLen ) )
			return pDef;
	}
	return NULL;
}



bool CResource::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CResource::r_WriteVal");
	EXC_TRY("WriteVal");
	// Just do stats values for now.
	int i = FindTableHeadSorted( pszKey, (LPCTSTR const *) sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1, sizeof(sm_szLoadKeys[0]) );
	if ( i<0 )
	{
		if ( !strnicmp( pszKey, "REGEN", 5 ))
		{
			int index = ATOI(pszKey+5);
			if (( index < 0 ) || ( index >= STAT_QTY ))
				return false;
			sVal.FormatVal(g_Cfg.m_iRegenRate[index]);
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

		// NOTE: When using SERV.xxxx CResource::r_GetRef is called, it transform MAP( in
		// MAP. dunno why (need to test to see what it breakes). So for a temp fix also MAP.
		// is caught.
		if ( !strnicmp(pszKey, "MAP(", 4) || !strnicmp(pszKey, "MAP.", 4) )
		{
			pszKey += 4;
			TCHAR * pszArgsNext;
			Str_Parse( const_cast<TCHAR*>(pszKey), &(pszArgsNext), ")" );

			CPointMap	pt;
			int			x = 0;
			int			iArgs = 0;

			if ( isdigit( pszKey[0] ) || pszKey[0] == '-' )
			{
				pt.m_map = 0; pt.m_z = 0;
				TCHAR * ppVal[4];
				iArgs = Str_ParseCmds( const_cast<TCHAR*>(pszKey), ppVal, COUNTOF( ppVal ), "," );

				switch ( iArgs )
				{
					default:
					case 4:
						if ( isdigit(ppVal[3][0]) )
						{
							pt.m_map = ATOI(ppVal[3]);
						}
					case 3:
						if ( isdigit(ppVal[2][0]) || (( iArgs == 4 ) && ( ppVal[2][0] == '-' )) )
						{
							pt.m_z = ( iArgs == 4 ) ? ATOI(ppVal[2]) : 0;
							if ( iArgs == 3 )
							{
								pt.m_map = ATOI(ppVal[2]);
							}
						}
					case 2:
						pt.m_y = ATOI(ppVal[1]);
					case 1:
						pt.m_x = ATOI(ppVal[0]);
					case 0:
						break;
				}
			}

			pszKey = pszArgsNext;

			if (( iArgs < 2 ) || !g_MapList.IsMapSupported(pt.m_map) || !pt.IsValidPoint() )
			{
				sVal.FormatVal(0);
				return false;
			}

			SKIP_SEPARATORS(pszKey);
			return pt.r_WriteVal(pszKey, sVal);
		}

		if ( ( !strnicmp( pszKey, "GUILDSTONES.",12) ) || ( !strnicmp( pszKey, "TOWNSTONES.",11) ) )
		{
			bool bGuild = !strnicmp( pszKey, "GUILDSTONES.",12);
			LPCTSTR pszCmd = pszKey + 11 + ( (bGuild) ? 1 : 0 );
			CItemStone * pStone = NULL;
			int x(0);

			if (!strcmpi(pszCmd,"COUNT"))
			{
				for ( int i=0; i<g_World.m_Stones.GetCount(); i++ )
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

			int iNumber = Exp_GetVal(pszCmd);
			SKIP_SEPARATORS(pszCmd);
			sVal.FormatVal(0);

			for ( int i=0; i<g_World.m_Stones.GetCount(); i++ )
			{
				pStone = g_World.m_Stones[i];
				if ( pStone == NULL )
					continue;

				if (( pStone->GetType() == IT_STONE_GUILD ) && bGuild )
				{
					if ( iNumber == x )
					{
						return( pStone->r_WriteVal(pszCmd,sVal,pSrc) );
					}
					x++;
				}
				else if (( pStone->GetType() == IT_STONE_TOWN ) && !bGuild )
				{
					if ( iNumber == x )
					{
						return( pStone->r_WriteVal(pszCmd,sVal,pSrc) );
					}
					x++;
				}
			}

			return( true );
		}

		if ( !strnicmp( pszKey, "CLIENT.",7))
		{
			pszKey += 7;
			int cli_num = Exp_GetVal(pszKey);
			int i(0);
			SKIP_SEPARATORS(pszKey);

			if ((cli_num < 0) || (cli_num >= g_Serv.StatGet( SERV_STAT_CLIENTS )))
				return false;

			sVal.FormatVal(0);
			for ( CClient * pClient = g_Serv.GetClientHead(); pClient != NULL; pClient = pClient->GetNext())
			{
				if ( cli_num == i )
				{
					CChar * pChar = pClient->GetChar();

					if ( pChar == NULL ) // Not an ingame client
						sVal.FormatVal(0);
					else if (!*pszKey)   // Checking reference
						sVal.FormatVal(1);
					else                 // Function of some kind
						return pChar->r_WriteVal(pszKey, sVal, pSrc);
					break;
				}
				i++;
			}
			return true;
		}
		return( false );
	}

	switch (i)
	{
		case RC_BANKMAXWEIGHT:
			sVal.FormatVal( m_iBankWMax / WEIGHT_UNITS );
			break;
		case RC_CLIENTLINGER:
			sVal.FormatVal( m_iClientLingerTime / TICK_PER_SEC );
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
		case RC_MINCHARDELETETIME:
			sVal.FormatVal( m_iMinCharDeleteTime /( 60*TICK_PER_SEC ));
			break;
		case RC_MURDERDECAYTIME:
			sVal.FormatVal( m_iMurderDecayTime / (TICK_PER_SEC ));
			break;
		case RC_WOOLGROWTHTIME:
			sVal.FormatVal( m_iWoolGrowthTime /( 60*TICK_PER_SEC ));
			break;
		case RC_PROFILE:
			sVal.FormatVal(g_Serv.m_Profile.GetActiveWindow());
			break;
		case RC_RTIME:
			{
				if ( pszKey[5] != '.' )
					sVal = CGTime::GetCurrentTime().Format(NULL);
				else
				{
					pszKey += 5;
					SKIP_SEPARATORS(pszKey);
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
		case RC_GUILDS:
			sVal.FormatVal( g_World.m_Stones.GetCount());
			return( true );
		case RC_TIMEUP:
			sVal.FormatVal( ( - g_World.GetTimeDiff( g_World.m_timeStartup )) / TICK_PER_SEC );
			return( true );
		case RC_VERSION:
			sVal = g_szServerDescription;
			break;
		case RC_EXPERIMENTAL:
			sVal.FormatHex( g_Cfg.m_iExperimental );
			PrintEFOFFlags(true, false, pSrc);
			break;
		case RC_OPTIONFLAGS:
			sVal.FormatHex( g_Cfg.m_iOptionFlags );
			PrintEFOFFlags(false, true, pSrc);
			break;
		case RC_CLIENTS:		// this is handled by CServerDef as SV_CLIENTS
			return false;
		default:
			return( sm_szLoadKeys[i].m_elem.GetValStr( this, sVal ));
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

	if ( isdigit( pszKey[0] ))
	{
		SKILL_TYPE skill = (SKILL_TYPE) Exp_GetVal(pszKey);
		if ( (!CChar::IsSkillBase(skill) || !g_Cfg.m_SkillIndexDefs.IsValidIndex(skill)) &&
			! CChar::IsSkillNPC(skill))
		{
			return( SKILL_NONE );
		}
		return( skill );
	}

	const CSkillDef* pSkillDef = FindSkillDef( pszKey );
	if ( pSkillDef == NULL )
		return( SKILL_NONE );
	return( (SKILL_TYPE)( pSkillDef->GetResourceID().GetResIndex()));
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

bool CResource::IsValidEmailAddressFormat( LPCTSTR pszEmail ) // static
{
	ADDTOCALLSTACK("CResource::IsValidEmailAddressFormat");
	// what are the invalid email name chars ?
	// Valid characters are, numbers, letters, underscore "_", dash "-" and the dot ".").

	int len1 = strlen( pszEmail );
	if ( len1 <= 0 || len1 > 128 )
		return( false );

	TCHAR szEmailStrip[256];
	int len2 = Str_GetBare( szEmailStrip, pszEmail,
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

CServerRef CResource::Server_GetDef( int index )
{
	ADDTOCALLSTACK("CResource::Server_GetDef");
	if ( ! m_Servers.IsValidIndex(index))
		return( NULL );
	return( CServerRef( STATIC_CAST <CServerDef*>( m_Servers[index] )));
}

CWebPageDef * CResource::FindWebPage( LPCTSTR pszPath ) const
{
	ADDTOCALLSTACK("CResource::FindWebPage");
	if ( pszPath == NULL )
	{
		if ( ! m_WebPages.GetCount())
			return( NULL );
		// Take this as the default page.
		return( STATIC_CAST <CWebPageDef*>( m_WebPages[0] ));
	}

	LPCTSTR pszTitle = CGFile::GetFilesTitle(pszPath);

	if ( pszTitle == NULL || pszTitle[0] == '\0' )
	{
		// This is just the root index.
		if ( ! m_WebPages.GetCount())
			return( NULL );
		// Take this as the default page.
		return( STATIC_CAST <CWebPageDef*>( m_WebPages[0] ));
	}

	for ( int i=0; i<m_WebPages.GetCount(); i++ )
	{
		if ( m_WebPages[i] == NULL )	// not sure why this would happen
			continue;
		CWebPageDef * pWeb = STATIC_CAST <CWebPageDef*>(m_WebPages[i] );
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

	for ( int i=0; i<m_Obscene.GetCount(); i++ )
	{
		char * match = new char[ strlen(m_Obscene[i])+3 ];
		sprintf(match,"%s%s%s","*",m_Obscene[i],"*");
		MATCH_TYPE ematch = Str_Match( match , pszText );
		delete[] match;
		if ( ematch == MATCH_VALID )
			return( true );
	}
	return( false );
}

CLogIP * CResource::FindLogIP( CSocketAddressIP IP, bool fCreate )
{
	ADDTOCALLSTACK("CResource::FindLogIP");
	if ( ! IP.IsValidAddr() )
	{
		// this address is always blocked.
		return( NULL );
	}

	// Will create if not found.
	for ( int i=0; i<m_LogIP.GetCount(); i++ )
	{
		CLogIP * pLogIP = m_LogIP[i];

		if ( pLogIP->IsSameIP( IP ))
		{
			if ( pLogIP->IsTimeDecay())	// time to decay the whole record ?
			{
				pLogIP->InitTimes();	// it has decayed but we didn't get rid of it yet.
			}
			return( pLogIP );
		}
		if ( pLogIP->IsTimeDecay())
		{
			// remove it.
			m_LogIP.DeleteAt( i );
			i--;
		}
	}

	// Create a new 1.
	if ( ! fCreate )
		return( NULL );

	CLogIP * pLogIP = new CLogIP( IP );
	m_LogIP.Add( pLogIP );
	return( pLogIP );
}

bool CResource::SetLogIPBlock( LPCTSTR szIP, bool fBlock, int iTimeDecay )
{
	ADDTOCALLSTACK("CResource::SetLogIPBlock");
	// Block or unblock an IP.
	// RETURN: true = set, false = already that state.
	CSocketAddressIP dwIP;
	dwIP.SetAddrStr( szIP );

	CLogIP * pLogIP = FindLogIP( dwIP, fBlock );
	if ( pLogIP == NULL )
		return( false );

	bool fPrevBlock = pLogIP->IsBlocked();
	if ( ! fBlock )
	{
		if ( ! fPrevBlock )
			return( false );	// already unblocked.
		m_LogIP.DeleteOb( pLogIP );
	}
	else
	{
		if ( fPrevBlock )
			return( false );	// already blocked.
		pLogIP->SetBlocked( true, iTimeDecay );
	}
	return( true );
}

const CGrayMulti * CResource::GetMultiItemDefs( ITEMID_TYPE itemid )
{
	ADDTOCALLSTACK("CResource::GetMultiItemDefs");
	if ( ! CItemBase::IsID_Multi(itemid))
		return( NULL );

	MULTI_TYPE id = itemid - ITEMID_MULTI;
	int index = m_MultiDefs.FindKey( id );
	if ( index < 0 )
	{
		index = m_MultiDefs.AddSortKey( new CGrayMulti( id ), id );
	}
	else
	{
		m_MultiDefs[index]->HitCacheTime();
	}
	const CGrayMulti * pMulti = m_MultiDefs[index];
	ASSERT(pMulti);
	return( pMulti );
}

PLEVEL_TYPE CResource::GetPrivCommandLevel( LPCTSTR pszCmd ) const
{
	ADDTOCALLSTACK("CResource::GetPrivCommandLevel");
	// What is this commands plevel ?
	// NOTE: This doe snot attempt to parse anything.
	int ilevel = PLEVEL_QTY-1;

	// Is this command avail for your priv level (or lower) ?
	for ( ; ilevel >= 0; ilevel-- )
	{
		LPCTSTR const * pszTable = m_PrivCommands[ilevel].GetBasePtr();
		int iCount = m_PrivCommands[ilevel].GetCount();
		if ( FindTableHeadSorted( pszCmd, pszTable, iCount ) >= 0 )
			return( (PLEVEL_TYPE)ilevel );
	}

	// A GM will default to use all commands.
	// xcept those that are specifically named that i can't use.
	return PLEVEL_GM;	// default level.
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
			if ( pClient->GetAccount() == NULL )
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
	char *myCmd = Str_GetTemp();

	int pOs; //position of space :)
	pOs = strcspn(pszCmd," ");
	strncpy ( myCmd, pszCmd, pOs );
	myCmd[pOs] = '\0';

	char * pOd; //position of dot :)
	while (pOd=strchr(myCmd,'.'))
	{
		ilevel = GetPrivCommandLevel( myCmd );
		if ( ilevel > pSrc->GetPrivLevel())
			return( false );
        myCmd=pOd+1; //skip dot
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
		int i = ( - ATOI(pCmd)) - 1;
		if ( ! m_StartDefs.IsValidIndex( i ))
			i = 0;
		return( m_StartDefs[i]->m_pt );
	}

	CPointMap pt;	// invalid point
	if ( isdigit( pCmd[0] ) || pCmd[0] == '-' )
	{
		TCHAR *pszTemp = Str_GetTemp();
		strcpy( pszTemp, pCmd );
		int iCount = pt.Read( pszTemp );
		if ( iCount >= 2 )
		{
			return( pt );
		}
	}
	else
	{
		// Match the region name with global regions.

		for ( int i=0; i<COUNTOF(m_ResHash.m_Array); i++ )
		for ( int j=0; j<m_ResHash.m_Array[i].GetCount(); j++ )
		{
			CResourceDef* pResDef = m_ResHash.m_Array[i][j];
			ASSERT(pResDef);
			CRegionBase * pRegion = dynamic_cast <CRegionBase*> (pResDef);
			if ( pRegion == NULL )
				continue;

			if ( ! pRegion->GetNameStr().CompareNoCase( pCmd ) ||
				! strcmpi( pRegion->GetResourceName(), pCmd ))
			{
				return( pRegion->m_pt );
			}
		}
	}
	// no match.
	return( pt );
}




void	CResource::LoadSortSpells()
{
	int		iQtySpells	= m_SpellDefs.GetCount();
	m_SpellDefs_Sorted.RemoveAll();
	m_SpellDefs_Sorted.Add( m_SpellDefs[0] );		// the null spell

	for ( int i = 1; i < iQtySpells; i++ )
	{
		if ( !m_SpellDefs.IsValidIndex( i ) )
			continue;

		int	iVal	= 0;
		m_SpellDefs[i]->GetPrimarySkill( NULL, &iVal );

		int		iQty	= m_SpellDefs_Sorted.GetCount();
		int		k;
		for ( k = 1; k < iQty; k++ )
		{
			int	iVal2	= 0;
			m_SpellDefs_Sorted[k]->GetPrimarySkill( NULL, &iVal2 );
			if ( iVal2 > iVal )
				break;
		}
		m_SpellDefs_Sorted.InsertAt( k, m_SpellDefs[i] );
	}
}


//*************************************************************

int CResource::GetPacketFlag( bool bCharlist, RESDISPLAY_VERSION res )
{
	int retValue = 0;
	bool bResOk = false;

	if ( bCharlist )
	{
		//	BYTE[4] Flags
		//		0x01	= unknown
		//		0x02	= send config/req logout (IGR?)
		//		0x04	= single character (siege) (alternative seen, Limit Characters)
		//		0x08	= enable npcpopup menus
		//		0x10	= unknown, (alternative seen, single character)
		//		0x20	= enable common AOS features (tooltip thing/fight system book, but not AOS monsters/map/skills)
		//		0x40	= Sixth Character Slot?
		//		8x80	= Samurai Empire?
		//		0x100	= Elf races?

		// T2A - LBR don't have char list flags
		bResOk = ( res >= RDS_AOS );
		if ( bResOk )
		{
			retValue |= ( (this->m_iFeatureAOS & FEATURE_AOS_UPDATE_A) && (this->m_iFeatureAOS & FEATURE_AOS_UPDATE_B) ) ? 0x020 : 0x00;
			retValue |= ( this->m_iFeatureAOS & FEATURE_AOS_POPUP ) ? 0x008 : 0x00;
			retValue |= ( (this->m_iFeatureAOS & FEATURE_AOS_UPDATE_A) && (this->m_iFeatureAOS & FEATURE_AOS_UPDATE_B) && (this->m_iMaxCharsPerAccount == 1) ) ? ( 0x010 | 0x004 ) : 0x00;
			retValue |= ( (this->m_iFeatureAOS & FEATURE_AOS_UPDATE_A) && (this->m_iFeatureAOS & FEATURE_AOS_UPDATE_B) && (this->m_iMaxCharsPerAccount == 6) ) ? 0x040 : 0x00;
		}

		bResOk = ( res >= RDS_SE );
		if ( bResOk )
		{
			retValue |= ( this->m_iFeatureSE ) ? 0x080 : 0x00;
		}

		bResOk = ( res >= RDS_ML );
		if ( bResOk )
		{
			retValue |= ( this->m_iFeatureML ) ? 0x0100 : 0x00;
		}
	}
	else
	{
		//         BYTE[2] feature#
		//	0x01	T2A upgrade, enables chatbutton
		//	0x02	Enables LBR update.  (of course LBR installation is required)
		//			(plays MP3 instead of midis, 2D LBR client shows new LBR monsters, ... )
		//	0x04	Unknown, never seen it set	(single char?)
		//	0x08	Unknown, set on OSI servers that have AOS code - no matter of account status (doesn�t seem to �unlock/lock� anything on client side)
		//	0x10	Enables AOS update (necro/paladin skills for all clients, malas map/AOS monsters if AOS installation present)
		//	0x20	Sixth Character Slot
		//	0x40	Samurai Empire?
		//	0x80	Elves?
		//	0x100
		//	0x200
		//	0x400
		//	0x800
		//	0x1000
		//	0x2000
		//	0x4000
		//	0x8000	Since client 4.0 this bit has to be set, otherwise bits 3..14 are ignored.
		// Thus	0		neither T2A NOR LBR, equal to not sending it at all,
		//		1		is T2A, chatbutton,
		//		2		is LBR without chatbutton,
		//		3		is LBR with chatbutton�
		//		8013	LBR + chatbutton + AOS enabled
		// Note1: this message is send immediately after login.
		// Note2: on OSI  servers this controls features OSI enables/disables via �upgrade codes.�
		// Note3: a 3 doesn�t seem to �hurt� older (NON LBR) clients.

		bResOk = ( res >= RDS_T2A );
		if ( bResOk )
		{
			retValue |= ( this->m_iFeatureT2A & FEATURE_T2A_UPDATE ) ? CLI_FEAT_T2A_FULL : 0x00;
			retValue |= ( this->m_iFeatureT2A & FEATURE_T2A_CHAT ) ? CLI_FEAT_T2A_CHAT : 0x00;
		}

		bResOk = ( res >= RDS_LBR );
		if ( bResOk )
		{
			retValue |= ( this->m_iFeatureLBR & FEATURE_LBR_UPDATE ) ? CLI_FEAT_LBR_FULL : 0x00;
			retValue |= ( this->m_iFeatureLBR & FEATURE_LBR_SOUND  ) ? CLI_FEAT_LBR_SOUND : 0x00;
		}

		bResOk = ( res >= RDS_AOS );
		if ( bResOk )
		{
			retValue |= ( this->m_iFeatureAOS & FEATURE_AOS_UPDATE_A ) ? 0x08010 : 0x00;
			retValue |= ( (this->m_iFeatureAOS & FEATURE_AOS_UPDATE_A) && (this->m_iFeatureAOS & FEATURE_AOS_UPDATE_B) && (this->m_iMaxCharsPerAccount == 6) ) ? 0x020 : 0x00;
		}

		bResOk = ( res >= RDS_SE );
		if ( bResOk )
		{
			retValue |= ( this->m_iFeatureSE & FEATURE_SE_NINJASAM ) ? 0x040 : 0x00;
		}

		bResOk = ( res >= RDS_ML );
		if ( bResOk )
		{
			retValue |= ( this->m_iFeatureML ) ? 0x080 : 0x00;
		}
	}

	return( retValue );
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
	}
	else if ( !strnicmp( pszSection, "ROOMDEF", 7 ) )
	{
		restype			= RES_ROOM;
		fNewStyleDef	= true;
	}
	else if ( !strnicmp( pszSection, "GLOBALS", 7 ) )
	{
		restype			= RES_WORLDVARS;
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
	else
		restype	= (RES_TYPE) FindTableSorted( pszSection, sm_szResourceBlocks, COUNTOF( sm_szResourceBlocks ));


	if (( restype == RES_WORLDSCRIPT ) || ( restype == RES_WS ))
	{
		LPCTSTR	pszDef			= pScript->GetArgStr();
		CVarDefCont * pVarBase	= g_Exp.m_VarDefs.GetKey( pszDef );
		CVarDefCont * pVarNum	= NULL;
		if ( pVarBase )
			pVarNum				= dynamic_cast <CVarDefContNum*>( pVarBase );
		if ( !pVarNum )
		{
			g_Log.Event( LOGL_WARN|LOGM_INIT, "Resource '%s' not found\n", pszDef );
			return false;
		}

		rid.SetPrivateUID( pVarNum->GetValNum() );
		restype	= rid.GetResType();

		int		index	= m_ResHash.FindKey( rid );


		CResourceDef *	pRes	= NULL;
		if ( index >= 0 )
			pRes	= dynamic_cast <CResourceDef*> (m_ResHash.GetAt( rid, index ) );
		if ( !pRes )
		{
			g_Log.Event( LOGL_WARN|LOGM_INIT, "Resource '%s' not found\n", pszDef );
			return false;
		}

		pRes->r_Load( *pScript );
		return true;
	}

	if ( restype < 0 )
	{
		g_Log.Event( LOGL_WARN|LOGM_INIT, "Unknown section '%s' in '%s'\n", (LPCTSTR) pScript->GetKey(), (LPCTSTR) pScript->GetFileTitle());
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
		DEBUG_ERR(( "Invalid %s block index '%s'\n", pszSection, (LPCTSTR) pScript->GetArgStr()));
		return( false );
	}

	EXC_TRY("LoadResourceSection");

	// NOTE: It is possible to be replacing an existing entry !!! Check for this.

	CResourceLink * pNewLink = NULL;
	CResourceDef * pNewDef = NULL;
	CResourceDef * pPrvDef = NULL;

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
		while ( pScript->ReadKeyParse())
		{
			SetLogIPBlock( pScript->GetKey(), true );
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
				long	l;
				for ( l = 0; l < DEFMSG_QTY; l++ )
				{
					if ( !strcmpi(pszKey, (const char *)g_Exp.sm_szMsgNames[l]) )
					{
						strcpy(g_Exp.sm_szMessages[l], pScript->GetArgStr());
						break;
					}
				}
				if ( l == DEFMSG_QTY )
					g_Log.Event(LOGM_INIT|LOGL_ERROR, "Setting not used message override named '%s'\n", pszKey);
				continue;
			}
			else g_Exp.m_VarDefs.SetStr(pszKey, false, pScript->GetArgStr());
		}
		return( true );
	case RES_FAME:
		{
			int i = 0;
			while ( pScript->ReadKey())
			{
				TCHAR * pName = pScript->GetKeyBuffer();
				if ( * pName == '<' )
					pName = "";
				TCHAR * pNew = new TCHAR [ strlen( pName ) + 1 ];
				strcpy( pNew, pName );
				m_Fame.SetAtGrow( i, pNew );
				++i;
			}
		}
		return( true );
	case RES_KARMA:
		{
			int i = 0;
			while ( pScript->ReadKey())
			{
				TCHAR * pName = pScript->GetKeyBuffer();
				if ( * pName == '<' )
					pName = "";
				TCHAR * pNew = new TCHAR [ strlen( pName ) + 1 ];
				strcpy( pNew, pName );
				m_Karma.SetAtGrow( i, pNew );
				++i;
			}
		}
		return( true );
	case RES_NOTOTITLES:
		{
			int i = 0;
			while ( pScript->ReadKey())
			{
				TCHAR * pName = pScript->GetKeyBuffer();
				if ( * pName == '<' )
					pName = "";
				TCHAR * pNew = new TCHAR [ strlen( pName ) + 1 ];
				strcpy( pNew, pName );
				m_NotoTitles.SetAtGrow( i, pNew );
				++i;
			}
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
			if ( index >= COUNTOF(m_PrivCommands) )
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
			TCHAR * pNew = new TCHAR [ strlen( pScript->GetKey()) + 1 ];
			strcpy( pNew, pScript->GetKey());
			m_Runes.Add( pNew );
		}
		return( true );
	case RES_SECTOR: // saved in world file.
		{
			CPointMap pt = GetRegionPoint( pScript->GetArgStr() ); // Decode a teleport location number into X/Y/Z
			return( pt.GetSector()->r_Load(*pScript));
		}
		return( true );
	case RES_SPELL:
		{
			CSpellDef * pSpell;
			pPrvDef = ResourceGetDef( rid );
			if ( pPrvDef )
			{
				pSpell = dynamic_cast <CSpellDef*>(pPrvDef);
			}
			else
			{
				pSpell = new CSpellDef( (SPELL_TYPE) rid.GetResIndex() );
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
				if ( rid.GetResIndex() >= g_Cfg.m_iMaxSkill )
					g_Cfg.m_iMaxSkill	= rid.GetResIndex() +1 ;

				// Just replace any previous CSkillDef
				pSkill = new CSkillDef( (SKILL_TYPE) rid.GetResIndex());
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
			CItemTypeDef	* pTypeDef	= dynamic_cast <CItemTypeDef*>(pPrvDef);
			ASSERT( pTypeDef );
			pNewLink = pTypeDef;
			ASSERT(pNewLink);

			// clear old tile links to this type
			int iQty	= g_World.m_TileTypes.GetCount();
			for ( int i = 0; i < iQty; i++ )
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
	case RES_TEMPLATE:
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
				int i = m_Servers.FindKey( pScript->GetKey());
				if ( i < 0 )
				{
					pServ = new CServerDef( pScript->GetKey(), CSocketAddressIP( SOCKET_LOCAL_ADDRESS ));
					fAddNew = true;
				}
				else
				{
					pServ = Server_GetDef(i);
				}
				if ( pScript->ReadKey())
				{
					pServ->m_ip.SetHostPortStr( pScript->GetKey());
					if ( pScript->ReadKey())
					{
						pServ->m_ip.SetPort( pScript->GetArgVal());
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
		m_StartDefs.RemoveAll();
		while ( pScript->ReadKey())
		{
			CStartLoc * pStart = new CStartLoc( pScript->GetKey());
			if ( pScript->ReadKey())
			{
				pStart->m_sName = pScript->GetKey();
				if ( pScript->ReadKey())
				{
					pStart->m_pt.Read( pScript->GetKeyBuffer());
				}
			}
			m_StartDefs.Add( pStart );
		}
		return( true );

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

	// Saved in the world file.

	case RES_GMPAGE:	// saved in world file. (Name is NOT DEFNAME)
		{
			CGMPage * pGMPage = new CGMPage( pScript->GetArgStr());
			return( pGMPage->r_Load( *pScript ));
		}
		return( true );
	case RES_WC:
	case RES_WORLDCHAR:	// saved in world file.
		if ( ! rid.IsValidUID())
		{
			g_Log.Event( LOGL_ERROR|LOGM_INIT, "Undefined char type '%s'\n", (LPCTSTR) pScript->GetArgStr() );
			return( false );
		}
		return( CChar::CreateBasic((CREID_TYPE)rid.GetResIndex())->r_Load(*pScript));
	case RES_WI:
	case RES_WORLDITEM:	// saved in world file.
		if ( ! rid.IsValidUID())
		{
			g_Log.Event( LOGL_ERROR|LOGM_INIT, "Undefined item type '%s'\n", (LPCTSTR) pScript->GetArgStr() );
			return( false );
		}
		return( CItem::CreateBase((ITEMID_TYPE)rid.GetResIndex())->r_Load(*pScript));
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
	g_Log.EventDebug("section '%s' args '%s'", pszSection, pScript ? pScript->GetArgStr() : "");
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
		if ( ! strcmpi( pszName, "MALE_DEFAULT" ))
			return( RESOURCE_ID( RES_NEWBIE, RES_NEWBIE_MALE_DEFAULT ));
		if ( ! strcmpi( pszName, "FEMALE_DEFAULT" ))
			return( RESOURCE_ID( RES_NEWBIE, RES_NEWBIE_FEMALE_DEFAULT ));
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
		if ( isdigit(pszName[0]))	// Its just an index.
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
			case RES_ITEMDEF:		// Define an item type
			case RES_TEMPLATE:		// Define lists of items. (for filling loot etc)
				break;
			default:
				return( rid );
			}
#ifdef _DEBUG
			if ( g_Serv.m_iModeCode != SERVMODE_ResyncLoad )	// this really is ok.
			{
				// Warn of  duplicates.
				index = m_ResHash.FindKey( rid );
				if ( index >= 0 )	// i found it. So i have to find something else.
					ASSERT(m_ResHash.GetAt(rid, index));
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
				DEBUG_ERR(( "Re-Using name '%s' to define block\n", (LPCTSTR) pszName ));
				return( ridinvalid );
			}
			rid.SetPrivateUID( pVarNum->GetValNum());
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
					DEBUG_ERR(( "Redefined name '%s' from %s to %s\n", (LPCTSTR) pszName, (LPCTSTR) GetResourceBlockName(rid.GetResType()), (LPCTSTR) GetResourceBlockName(restype) ));
					return( ridinvalid );
				}
			}
			else if ( fNewStyleDef && pVarNum->GetValNum() != rid.GetPrivateUID() )
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
						g_pLog->EventWarn( "Redef resource '%s'\n", (LPCTSTR) pszName );
					else
						DEBUG_WARN(( "Redef resource '%s'\n", (LPCTSTR) pszName ));
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
		index = NPCID_SCRIPT2 + 0x2000;	// add another offset to avoid Sphere ranges.
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
		while(true)
		{
			if ( m_ResHash.FindKey( rid ) < 0 )
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

	int index = rid.GetResIndex();
	switch ( rid.GetResType() )
	{
	case RES_WEBPAGE:
		index = m_WebPages.FindKey( rid );
		if ( index < 0 )
			return( NULL );
		return( m_WebPages.GetAt( index ));

	case RES_SKILL:
		if ( ! m_SkillIndexDefs.IsValidIndex(index))
			return( NULL );
		return( const_cast <CSkillDef *>( m_SkillIndexDefs[ index ] ));

	case RES_SPELL:
		if ( ! m_SpellDefs.IsValidIndex(index))
			return( NULL );
		return( const_cast <CSpellDef *>( m_SpellDefs[ index ] ));

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
		for ( int i = 0; i < m_WebPages.GetCount(); i++ )
		{
			EXC_TRY("WebTick");
			if ( !m_WebPages[i] )
				continue;
			CWebPageDef * pWeb = STATIC_CAST <CWebPageDef *>(m_WebPages[i]);
			if ( pWeb )
			{
				pWeb->WebPageUpdate(fNow, NULL, &g_Serv);
				pWeb->WebPageLog();
			}
			EXC_CATCH;

			EXC_DEBUG_START;
			CWebPageDef * pWeb = STATIC_CAST <CWebPageDef *>(m_WebPages[i]);
			g_Log.EventDebug("web '%s' dest '%s' now '%d' index '%d'/'%d'\n",
				pWeb ? pWeb->GetName() : "", pWeb ? pWeb->GetDstName() : "",
				(int)fNow, i, m_WebPages.GetCount());
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
		char	zOptionFlags[512];
		zOptionFlags[0] = 0;

		if ( IsSetOF(OF_Command_Sysmsgs) ) catresname(zOptionFlags, "CommandSysmessages");
		if ( IsSetOF(OF_OSIMultiSight) ) catresname(zOptionFlags, "OSIMultiSight");
		if ( IsSetOF(OF_Items_AutoName) ) catresname(zOptionFlags, "ItemsAutoName");
		if ( IsSetOF(OF_FileCommands) ) catresname(zOptionFlags, "FileCommands");
		if ( IsSetOF(OF_NoItemNaming) ) catresname(zOptionFlags, "NoItemNaming");
		if ( IsSetOF(OF_NoHouseMuteSpeech) ) catresname(zOptionFlags, "NoHouseMuteSpeech");
		if ( IsSetOF(OF_Flood_Protection) ) catresname(zOptionFlags, "FloodProtection");
		if ( IsSetOF(OF_Buffs) ) catresname(zOptionFlags, "Buffs");
		if ( IsSetOF(OF_NoPrefix) ) catresname(zOptionFlags, "NoPrefix");
		if ( IsSetOF(OF_DyeType) ) catresname(zOptionFlags, "DyeType");

		if ( zOptionFlags[0] )
		{
			if ( pSrc ) pSrc->SysMessagef("Option flags: %s\n", zOptionFlags);
			else g_Log.Event(LOGM_INIT, "Option flags: %s\n", zOptionFlags);
		}
	}
	if ( bEF )
	{
		char	zExperimentalFlags[512];
		zExperimentalFlags[0] = 0;

		if ( IsSetEF(EF_DiagonalWalkCheck) ) catresname(zExperimentalFlags, "DiagonalWalkCheck");
		if ( IsSetEF(EF_UNICODE) ) catresname(zExperimentalFlags, "Unicode");
		if ( IsSetEF(EF_New_Triggers) ) catresname(zExperimentalFlags, "NewTriggersEnable");
		if ( IsSetEF(EF_Intrinsic_Locals) ) catresname(zExperimentalFlags, "IntrinsicLocals");
		if ( IsSetEF(EF_Item_Strict_Comparison) ) catresname(zExperimentalFlags, "ItemStrictComparison");
		if ( IsSetEF(EF_NewPositionChecks) ) catresname(zExperimentalFlags, "NewPositionChecks");
		if ( IsSetEF(EF_WalkCheck) ) catresname(zExperimentalFlags, "WalkCheck");
		if ( IsSetEF(EF_AllowTelnetPacketFilter) ) catresname(zExperimentalFlags, "TelnetPacketFilter");
		if ( IsSetEF(EF_Script_Profiler) ) catresname(zExperimentalFlags, "ScriptProfiler");
		if ( IsSetEF(EF_Size_Optimise) ) catresname(zExperimentalFlags, "SizeOptimize");
		if ( IsSetEF(EF_Minimize_Triggers) ) catresname(zExperimentalFlags, "MinimizeTriggers");
		if ( IsSetEF(EF_DamageTools) ) catresname(zExperimentalFlags, "DamageTools");
		if ( IsSetEF(EF_Mapdiff_Support) ) catresname(zExperimentalFlags, "MapdiffSupport");

		if ( zExperimentalFlags[0] )
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
			g_Log.Event(LOGL_FATAL|LOGM_INIT, GRAY_FILE ".ini has not been found, server probably would be not usable.\n");
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
		g_Log.Event( LOGL_WARN|LOGM_INIT, "Could not open " GRAY_FILE "Crypt.ini, encryption might not be available\n");

		return( false );
	}

	LoadResourcesOpen(&m_scpCryptIni);
	m_scpCryptIni.Close();
	m_scpCryptIni.CloseForce();

	g_Log.Event( LOGM_INIT, "Loaded %d client encryption keys.\n", CCrypt::client_keys.size() );

	return( true );
}

void CResource::Unload( bool fResync )
{
	ADDTOCALLSTACK("CResource::Unload");
	if ( fResync )
	{
		// Unlock all the SCP and MUL files.
		g_Install.CloseFiles();
		for ( int j=0; true; j++ )
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

	// m_LogIP
	m_Obscene.RemoveAll();
	m_Fame.RemoveAll();
	m_Karma.RemoveAll();
	m_NotoTitles.RemoveAll();
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
}

bool CResource::Load( bool fResync )
{
	ADDTOCALLSTACK("CResource::Load");
	// ARGS:
	//  fResync = just look for changes.

	if ( ! fResync )
	{
		g_Install.FindInstall();
	}
	else
	{
		m_scpIni.ReSync();
		m_scpIni.CloseForce();
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
      g_Log.Event( LOGL_FATAL|LOGM_INIT, "The " GRAY_FILE ".INI file is corrupt or missing\n" );
		g_Log.Event( LOGL_FATAL|LOGM_INIT, "MUL File '%s' not found...\n", (LPCTSTR) g_Install.GetBaseFileName(i));
		return( false );
	}

	// Load the optional verdata cache. (modifies MUL stuff)
	try
	{
		g_VerData.Load( g_Install.m_File[VERFILE_VERDATA] );
	}
	catch ( CGrayError &e )
	{
      g_Log.Event( LOGL_FATAL|LOGM_INIT, "The " GRAY_FILE ".INI file is corrupt or missing\n" );
		g_Log.CatchEvent( &e, "g_VerData.Load" );
		return( false );
	}
	catch(...)
	{
      g_Log.Event( LOGL_FATAL|LOGM_INIT, "The " GRAY_FILE ".INI file is corrupt or missing\n" );
		g_Log.CatchEvent( NULL, "g_VerData.Load" );
		return( false );
	}

	// Now load the *TABLES.SCP file.
	if ( ! fResync )
	{
		if ( ! OpenResourceFind( m_scpTables, GRAY_FILE "tables" ))
		{
         		g_Log.Event( LOGL_FATAL|LOGM_INIT, "The " GRAY_FILE ".INI file is corrupt or missing\n" );
			g_Log.Event( LOGL_FATAL|LOGM_INIT, "Error opening table definitions file...\n" );
			return false;
		}

		LoadResourcesOpen(&m_scpTables);
		m_scpTables.Close();
	}
	else
	{
		m_scpTables.ReSync();
	}
	m_scpTables.CloseForce();

	//	Initialize the world sectors
	g_World.Init();

	// open and index all my script files i'm going to use.
	AddResourceDir( m_sSCPBaseDir );		// if we want to get *.SCP files from elsewhere.

	int count = m_ResourceFiles.GetCount();
	g_Log.Event(LOGM_INIT, "Indexing %d scripts...\n", count);

	for ( int j=0; true; j++ )
	{
		CResourceScript * pResFile = GetResourceFile(j);
		if ( !pResFile )
			break;

		if ( !fResync )
			LoadResources( pResFile );
		else
			pResFile->ReSync();

#ifdef _WIN32
		NTWindow_OnTick(0);
#endif
		g_Serv.PrintPercent(j+1, count);
	}

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
	g_Serv.SysMessage( "Done loading scripts.\n" );

	if ( !m_StartDefs.GetCount() )
	{
		g_Log.Event(LOGM_INIT|LOGL_ERROR, "No any START locations specifyed. Add them and try again.\n");
		return false;
	}

	// Make region DEFNAMEs
	{
		int			iMax	= g_Cfg.m_RegionDefs.GetCount();
		for ( int i = 0; i < iMax; i++ )
		{
			CRegionBase * pRegion = dynamic_cast <CRegionBase*> (g_Cfg.m_RegionDefs.GetAt(i));
			if ( !pRegion )
				continue;
			pRegion->MakeRegionName();
		}
	}
	if ( ! m_sEventsPet.IsEmpty() )
	{
		m_pEventsPetLink = dynamic_cast<CResourceLink *>( g_Cfg.ResourceGetDefByName( RES_EVENTS, m_sEventsPet ) );

		if ( m_pEventsPetLink == NULL )
			g_Log.Event( LOGM_INIT|LOGL_ERROR, "Can't find definition for '%s' (EVENTSPET)\n", (LPCTSTR) m_sEventsPet );
	}
	else if ( fResync )
		m_pEventsRegionLink = NULL;

	if ( ! m_sEventsPlayer.IsEmpty() )
	{
		m_pEventsPlayerLink = dynamic_cast<CResourceLink *>( g_Cfg.ResourceGetDefByName( RES_EVENTS, m_sEventsPlayer ) );

		if ( m_pEventsPlayerLink == NULL )
			g_Log.Event( LOGM_INIT|LOGL_ERROR, "Can't find definition for '%s' (EVENTSPLAYER)\n", (LPCTSTR) m_sEventsPlayer );
	}
	else if ( fResync )
		m_pEventsRegionLink = NULL;

	if ( ! m_sEventsRegion.IsEmpty() )
	{
		m_pEventsRegionLink = dynamic_cast<CResourceLink *>( g_Cfg.ResourceGetDefByName( RES_REGIONTYPE, m_sEventsRegion ) );

		if ( m_pEventsRegionLink == NULL )
			g_Log.Event( LOGM_INIT|LOGL_ERROR, "Can't find definition for '%s' (EVENTSREGION)\n", (LPCTSTR) m_sEventsRegion );
	}
	else if ( fResync )
		m_pEventsRegionLink = NULL;

	LoadSortSpells();
	g_Serv.SysMessage("\n");


	// Load crypt keys from SphereCrypt.ini
	if ( ! fResync )
	{
		LoadCryptIni();
	}
	else
	{
		m_scpCryptIni.ReSync();
		m_scpCryptIni.CloseForce();
	}

	// Yay for crypt version
	g_Serv.SetCryptVersion();

	return true;
}

LPCTSTR CResource::GetDefaultMsg(long lKeyNum)
{
	ADDTOCALLSTACK("CResource::GetDefaultMsg");
	if (( lKeyNum < 0 ) || ( lKeyNum > DEFMSG_QTY ))
	{
		g_Log.EventError("Defmessage %d out of range [0..%d]\n", lKeyNum, DEFMSG_QTY-1);
		return "";
	}
	return g_Exp.sm_szMessages[lKeyNum];
}

int CItemTypeDef::GetItemType()
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
		int		iLo;
		int		iHi;
		iLo	= Exp_GetVal( pszArgs );
		GETNONWHITESPACE( pszArgs );
		if ( *pszArgs == ',' )
		{
			pszArgs++;
			GETNONWHITESPACE( pszArgs );
		}
		if ( !*pszArgs )
			iHi	= iLo;
		else
			iHi	= Exp_GetVal( pszArgs );

		if ( iLo > iHi )		// swap
		{
			int	iTmp	= iHi;
			iHi	= iLo;
			iLo	= iTmp;
		}

		for ( int i = iLo; i <= iHi; i++ )
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
