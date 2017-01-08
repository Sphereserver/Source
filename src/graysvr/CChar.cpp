//  CChar is either an NPC or a Player.
#include "graysvr.h"	// predef header.
#include "CClient.h"
#include "../network/network.h"

LPCTSTR const CChar::sm_szTrigName[CTRIG_QTY+1] =	// static
{
	"@AAAUNUSED",
	"@AfterClick",
	"@Attack",				// I am attacking someone (SRC)
	"@CallGuards",

	"@charAttack",		// Here starts @charXXX section
	"@charClick",
	"@charClientTooltip",
	"@charContextMenuRequest",
	"@charContextMenuSelect",
	"@charDClick",
	"@charTradeAccepted",

	"@Click",				// I got clicked on by someone.
	"@ClientTooltip", // Sending tooltips to someone
	"@CombatAdd",
	"@CombatDelete",
	"@CombatEnd",
	"@CombatStart",
	"@ContextMenuRequest",
	"@ContextMenuSelect",
	"@Create",				// Newly created (not in the world yet)
	"@Criminal",			// Called before somebody becomes "gray" for someone
	"@DClick",				// Someone has dclicked on me.
	"@Death",				//+I just got killed.
	"@DeathCorpse",
	"@Destroy",				//+I am nearly destroyed
	"@Dismount",			// I am trying to get rid of my ride right now
	//"@Dye",					// My color has been changed
	"@Eat",
	"@EffectAdd",
	"@EnvironChange",		// my environment changed somehow (light,weather,season,region)
	"@ExpChange",			// EXP is going to change
	"@ExpLevelChange",		// Experience LEVEL is going to change

	"@FameChange",				// Fame changed
	"@FollowersUpdate",

	"@GetHit",				// I just got hit.
	"@Hit",					// I just hit someone. (TARG)
	"@HitCheck",			// Checking if I can hit my target, overriding also default checks if set to.
	"@HitIgnore",			// I'm going to avoid a target (attacker.n.ignore=1) , should I un-ignore him?.
	"@HitMiss",				// I just missed.
	"@HitTry",				// I am trying to hit someone. starting swing.
	"@HouseDesignCommit",	// I committed a new house design.
	"@HouseDesignExit",		// I exited house design mode.

	// ITRIG_QTY
	"@itemAfterClick",
	"@itemBuy",
	"@itemClick",			// I clicked on an item
	"@itemClientTooltip", // Receiving tooltip for something
	"@itemContextMenuRequest",
	"@itemContextMenuSelect",
	"@itemCreate",			//?
	"@itemDamage",			//?
	"@itemDCLICK",			// I have dclicked item
	"@itemDestroy",			//+I am nearly destroyed
	"@itemDropOn_Char",		// I have been dropped on this char
	"@itemDropOn_Ground",	// I dropped an item on the ground
	"@itemDropOn_Item",		// I have been dropped on this item
	"@itemDropOn_Self",		// An item has been dropped on
	"@ItemDropOn_Trade",
	"@itemEQUIP",			// I have equipped an item
	"@itemEQUIPTEST",
	"@itemMemoryEquip",
	"@itemPICKUP_GROUND",
	"@itemPICKUP_PACK",		// picked up from inside some container.
	"@itemPICKUP_SELF",		// picked up from this container
	"@itemPICKUP_STACK",	// picked up from a stack
	"@itemSELL",
	"@itemSPELL",			// cast some spell on the item.
	"@itemSTEP",			// stepped on an item
	"@itemTARGON_CANCEL",
	"@itemTARGON_CHAR",
	"@itemTARGON_GROUND",
	"@itemTARGON_ITEM",		// I am being combined with an item
	"@itemTimer",			//?
	"@itemToolTip",			// Did tool tips on an item
	"@itemUNEQUIP",			// i have unequipped (or try to unequip) an item

	"@Jailed",

	"@KarmaChange",				// Karma chaged

	"@Kill",				//+I have just killed someone
	"@LogIn",				// Client logs in
	"@LogOut",				// Client logs out (21)
	"@Mount",				// I'm trying to mount my horse (or whatever)
	"@MurderDecay",			// One of my kills is gonna to be cleared
	"@MurderMark",			// I am gonna to be marked as a murderer
	"@NotoSend",			// Sending notoriety

	"@NPCAcceptItem",		// (NPC only) i've been given an item i like (according to DESIRES)
	"@NPCActFight",
	"@NPCActFollow",			// (NPC only) following someone right now
	"@NPCAction",
	"@NPCHearGreeting",		// (NPC only) i have been spoken to for the first time. (no memory of previous hearing)
	"@NPCHearUnknown",		//+(NPC only) I heard something i don't understand.
	"@NPCLookAtChar",		// (NPC only) look at a character
	"@NPCLookAtItem",		// (NPC only) look at a character
	"@NPCLostTeleport",		//+(NPC only) ready to teleport back to spawn
	"@NPCRefuseItem",		// (NPC only) i've been given an item i don't want.
	"@NPCRestock",			// (NPC only)
	"@NPCSeeNewPlayer",		//+(NPC only) i see u for the first time. (in 20 minutes) (check memory time)
	"@NPCSeeWantItem",		// (NPC only) i see something good.
	"@NPCSpecialAction",	// Idle 

	"@PartyDisband",		//I just disbanded my party
	"@PartyInvite",			//SRC invited me to join a party, so I may chose
	"@PartyLeave",
	"@PartyRemove",			//I have ben removed from the party by SRC

	"@PersonalSpace",	//+i just got stepped on.
	"@PetDesert",		// I just went wild again
	"@Profile",			// someone hit the profile button for me.
	"@ReceiveItem",		// I was just handed an item (Not yet checked if i want it)
	"@RegenStat",		//Regenerating any stat

	"@RegionEnter",
	"@RegionLeave",
	"@RegionResourceFound",	// I just discovered a resource
	"@RegionResourceGather",

	"@Rename",
	"@Resurrect",
	"@SeeCrime",		// I saw a crime
	"@SeeHidden",			// Can I see hidden chars?
	"@SeeSnoop",

	// SKTRIG_QTY
	"@SkillAbort",
	"@SkillChange",
	"@SkillFail",
	"@SkillGain",
	"@SkillMakeItem",
	"@SkillMenu",
	"@SkillPreStart",
	"@SkillSelect",
	"@SkillStart",
	"@SkillStroke",
	"@SkillSuccess",
	"@SkillTargetCancel",
	"@SkillUseQuick",
	"@SkillWait",

	"@SpellBook",
	"@SpellCast",		//+Char is casting a spell.
	"@SpellEffect",		//+A spell just hit me.
	"@SpellFail",		// The spell failed
	"@SpellSelect",		// Selected a spell
	"@SpellSuccess",	// The spell succeeded
	"@SpellTargetCancel",	// cancelled spell target
	"@StatChange",
	"@StepStealth",		//+Made a step in stealth mode
	"@Targon_Cancel",	//closing target from TARGETF*
	"@ToggleFlying",	//Flying On/Off
	"@ToolTip",			// someone did tool tips on me.
	"@TradeAccepted",	// Everything went well, and we are about to exchange trade items
	"@TradeClose",
	"@TradeCreate",

	"@UserBugReport",
	"@UserChatButton",
	"@UserExtCmd",
	"@UserExWalkLimit",
	"@UserGuildButton",
	"@UserKRToolbar",
	"@UserMailBag",
	"@UserQuestArrowClick",
	"@UserQuestButton",
	"@UserSkills",
	"@UserSpecialMove",
	"@UserStats",
	"@UserVirtue",
	"@UserVirtueInvoke",
	"@UserWarmode",

	// War mode ?
	 NULL,
};


/////////////////////////////////////////////////////////////////
// -CChar

// Create the "basic" NPC. Not NPC or player yet.
// NOTE: NEVER return NULL
CChar * CChar::CreateBasic(CREID_TYPE baseID) // static
{
	ADDTOCALLSTACK("CChar::CreateBasic");
	return new CChar(baseID);
}

// Create an NPC
// NOTE: NEVER return NULL
CChar * CChar::CreateNPC( CREID_TYPE baseID )	// static
{
	ADDTOCALLSTACK("CChar::CreateNPC");
	CChar * pChar = CreateBasic(baseID);
	ASSERT(pChar);
	pChar->NPC_LoadScript(true);
	pChar->NPC_CreateTrigger();

	return pChar;
}

CChar::CChar( CREID_TYPE baseID ) : CObjBase( false )
{
	g_Serv.StatInc( SERV_STAT_CHARS );	// Count created CChars.

	m_pArea = NULL;
	m_pParty = NULL;
	m_pClient = NULL;	// is the char a logged in player?
	m_pPlayer = NULL;	// May even be an off-line player!
	m_pNPC	  = NULL;
	m_pRoom = NULL;
	m_StatFlag = 0;


	if ( g_World.m_fSaveParity )
	{
		StatFlag_Set(STATF_SaveParity);	// It will get saved next time.
	}
	m_UIDLastNewItem.InitUID();
	m_dirFace = DIR_SE;
	m_fonttype = FONT_NORMAL;
	m_SpeechHue = HUE_TEXT_DEF;

	m_height = 0;
	m_ModMaxWeight = 0;

	m_StepStealth = 0;
	m_iVisualRange = UO_MAP_VIEW_SIZE;
	m_virtualGold = 0;

	m_exp = 0;
	m_level = 0;
	m_defense = 0;
	m_atUnk.m_Arg1 = 0;
	m_atUnk.m_Arg2 = 0;
	m_atUnk.m_Arg3 = 0;

	m_timeLastRegen = m_timeCreate = CServTime::GetCurrentTime();
	m_timeLastHitsUpdate = m_timeLastRegen;

	m_prev_Hue = HUE_DEFAULT;
	m_prev_id = CREID_INVALID;
	SetID( baseID );
	CCharBase* pCharDef = Char_GetDef();
	ASSERT(pCharDef);
	m_attackBase = pCharDef->m_attackBase;
	m_attackRange = pCharDef->m_attackRange;
	m_defenseBase = pCharDef->m_defenseBase;
	m_defenseRange = pCharDef->m_defenseRange;
	m_Can = pCharDef->m_Can;
	m_wBloodHue = pCharDef->m_wBloodHue;	// when damaged , what color is the blood (-1) = no blood

	SetName(pCharDef->GetName());	// set the name in case there is a name template.

	Skill_Cleanup();

	g_World.m_uidLastNewChar = GetUID();	// for script access.

	size_t i = 0;
	for ( ; i < STAT_QTY; i++ )
	{
		Stat_SetBase(static_cast<STAT_TYPE>(i), 0);
		Stat_SetMod(static_cast<STAT_TYPE>(i), 0);
		Stat_SetVal(static_cast<STAT_TYPE>(i), 0);
		Stat_SetMax(static_cast<STAT_TYPE>(i), 0);
		m_Stat[i].m_regen = 0;
	}
	Stat_SetVal( STAT_FOOD, Stat_GetMax(STAT_FOOD) );

	for ( i = 0; i < g_Cfg.m_iMaxSkill; i++ )
	{
		m_Skill[i] = 0;
	}

	m_LocalLight = 0;
	m_fClimbUpdated = false;

	ASSERT(IsDisconnected());
}

// Delete character
CChar::~CChar() 
{
	if ( m_pClient )	// this should never happen
		m_pClient->m_NetState->markReadClosed();

	DeletePrepare();	// remove me early so virtuals will work.
	if ( IsStatFlag( STATF_Ridden ))
	{
		CItem * pItem = Horse_GetMountItem();
		if ( pItem )
		{
			pItem->m_itFigurine.m_UID.InitUID();	// unlink it first.
			pItem->Delete();
		}
	}

	Guild_Resign(MEMORY_GUILD);
	Guild_Resign(MEMORY_TOWN);

	if ( m_pParty )
	{
		m_pParty->RemoveMember(GetUID(), GetUID());
		m_pParty = NULL;
	}
	Attacker_RemoveChar();	// Remove me from enemies attacker list (I assume that if he is on my list, I'm on his one and no one have me on their list if I dont have them)
	NPC_PetClearOwners();	// Clear follower slots on pet owner
	DeleteAll();			// Remove me early so virtuals will work
	ClearNPC();
	ClearPlayer();
	g_Serv.StatDec( SERV_STAT_CHARS );
}

// Client is detaching from this CChar.
void CChar::ClientDetach()
{
	ADDTOCALLSTACK("CChar::ClientDetach");
	
	// remove all trade windows.
	CItem *pItemNext = NULL;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( pItem->IsType(IT_EQ_TRADE_WINDOW) )
			pItem->Delete();
	}
	if ( !m_pClient )
		return;

	if ( m_pParty && m_pParty->IsPartyMaster( this ))
	{
		// Party must disband if the master is logged out.
		m_pParty->Disband(GetUID());
		m_pParty = NULL;
	}

	// If this char is on a IT_SHIP then we need to stop the ship !
	if ( m_pArea && m_pArea->IsFlag( REGION_FLAG_SHIP ))
	{
		CItemShip * pShipItem = dynamic_cast <CItemShip *>( m_pArea->GetResourceID().ItemFind());
		if ( pShipItem )
		{
			pShipItem->Ship_Stop();
		}
	}

	CSector * pSector = GetTopSector();
	pSector->ClientDetach( this );
	m_pClient = NULL;
}

// Client is Attaching to this CChar.
void CChar::ClientAttach( CClient * pClient )
{
	ADDTOCALLSTACK("CChar::ClientAttach");
	if ( pClient == m_pClient )
		return;
	if ( !SetPlayerAccount(pClient->m_pAccount) )	// i now own this char.
		return;

	ASSERT(m_pPlayer);
	m_pPlayer->m_timeLastUsed = CServTime::GetCurrentTime();

	m_pClient = pClient;
	GetTopSector()->ClientAttach( this );
	FixClimbHeight();
}

// Client logged out or NPC is dead.
void CChar::SetDisconnected()
{
	ADDTOCALLSTACK("CChar::SetDisconnected");
	if ( m_pClient )
	{
		m_pClient->m_NetState->markReadClosed();
		return;
	}

	if ( m_pParty )
	{
		m_pParty->RemoveMember(GetUID(), GetUID());
		m_pParty = NULL;
	}
	if ( IsDisconnected() )
		return;

	RemoveFromView();
	MoveToRegion(NULL, false);
	GetTopSector()->m_Chars_Disconnect.InsertHead(this);
}

// Called before Delete()
// Return true on @Destroy or f_onchar_delete can prevent the deletion
// pClient is the client deleting the char (only set when the function is triggered from client Character Selection menu)
bool CChar::NotifyDelete(CClient *pClient)
{
	if ( IsTrigUsed(TRIGGER_DESTROY) )
	{
		if ( OnTrigger(CTRIG_Destroy, &g_Serv) == TRIGRET_RET_TRUE )
			return false;
	}

	if ( m_pPlayer )
	{
		TRIGRET_TYPE tr;
		CScriptTriggerArgs Args;
		Args.m_pO1 = pClient;
		r_Call("f_onchar_delete", this, &Args, NULL, &tr);
		if ( tr == TRIGRET_RET_TRUE )
			return false;
	}

	ContentNotifyDelete();
	return true;
}

void CChar::Delete(bool bforce, CClient *pClient)
{
	ADDTOCALLSTACK("CChar::Delete");

	if ( !NotifyDelete(pClient) && !bforce )
		return;

	// Character has been deleted
	if ( m_pClient )
	{
		m_pClient->m_NetState->markReadClosed();
		m_pClient->CharDisconnect();
	}

	// Detach from account now
	ClearPlayer();

	CObjBase::Delete();
}

// Is there something wrong with this char?
// RETURN: invalid code.
int CChar::IsWeird() const
{
	ADDTOCALLSTACK_INTENSIVE("CChar::IsWeird");
	int iResultCode = CObjBase::IsWeird();
	if ( iResultCode )
		return iResultCode;

	if ( IsDisconnected() )
	{
		if ( !GetTopSector()->IsCharDisconnectedIn(this) )
			return 0x1102;

		if ( m_pNPC )
		{
			if ( IsStatFlag(STATF_Ridden) )
			{
				if ( Skill_GetActive() != NPCACT_RIDDEN )
					return 0x1103;

				// Make sure we are still linked back to the world.
				CItem *pItem = Horse_GetMountItem();
				if ( !pItem )
					return 0x1104;
				if ( pItem->m_itFigurine.m_UID != GetUID() )
					return 0x1105;
			}
			else if ( !IsStatFlag(STATF_DEAD) )
				return 0x1106;
		}
	}

	if ( !m_pPlayer && !m_pNPC )
		return 0x1107;
	if ( !GetTopPoint().IsValidPoint() )
		return 0x1108;

	return 0;
}

// Get the Z we should be at
signed char CChar::GetFixZ( CPointMap pt, DWORD dwBlockFlags)
{
	if ( !dwBlockFlags )
		dwBlockFlags = GetMoveBlockFlags();
	DWORD dwCan = GetMoveBlockFlags();
	if ( dwCan & CAN_C_WALK )
		dwBlockFlags |= CAN_I_CLIMB; // If we can walk than we can climb. Ignore CAN_C_FLY at all here

	CGrayMapBlockState block( dwBlockFlags, pt.m_z, pt.m_z + m_zClimbHeight + GetHeightMount( false ), pt.m_z + m_zClimbHeight + 2, GetHeightMount( false ) );
	g_World.GetFixPoint( pt, block );

	dwBlockFlags = block.m_Bottom.m_dwBlockFlags;
	if ( block.m_Top.m_dwBlockFlags )
	{
		dwBlockFlags |= CAN_I_ROOF;	// we are covered by something.
		if ( block.m_Top.m_z < pt.m_z + (m_zClimbHeight + (block.m_Top.m_dwTile > TERRAIN_QTY ? GetHeightMount( false ) : GetHeightMount( false )/2 )) )
			dwBlockFlags |= CAN_I_BLOCK; // we can't fit under this!
	}
	if (( dwCan != 0xFFFF ) && ( dwBlockFlags != 0x0 ))
	{

		if ( ( dwBlockFlags & CAN_I_DOOR ) && Can( CAN_C_GHOST ))
			dwBlockFlags &= ~CAN_I_BLOCK;

		if ( ( dwBlockFlags & CAN_I_WATER ) && Can( CAN_C_SWIM ))
			dwBlockFlags &= ~CAN_I_BLOCK;

		if ( ! Can( CAN_C_FLY ))
		{
			if ( ! ( dwBlockFlags & CAN_I_CLIMB ) ) // we can climb anywhere
			{
				if ( block.m_Bottom.m_dwTile > TERRAIN_QTY )
				{
					if ( block.m_Bottom.m_z > pt.m_z + m_zClimbHeight + 2) // Too high to climb.
						return pt.m_z;
				}
				else if ( block.m_Bottom.m_z > pt.m_z + m_zClimbHeight + GetHeightMount( false ) + 3)
					return pt.m_z;
			}
		}
		if (( dwBlockFlags & CAN_I_BLOCK ) && ( ! Can( CAN_C_PASSWALLS )) )
			return pt.m_z;

		if ( block.m_Bottom.m_z >= UO_SIZE_Z )
			return pt.m_z;
	}
	if (( GetHeightMount( false ) + pt.m_z >= block.m_Top.m_z ) && ( g_Cfg.m_iMountHeight ) && ( !IsPriv( PRIV_GM ) ) && ( !IsPriv( PRIV_ALLMOVE ) ))
		return pt.m_z;
	return(block.m_Bottom.m_z);
}

// Clean up weird flags.
// fix Weirdness.
// NOTE:
//  Deleting a player char is VERY BAD ! Be careful !
//
// RETURN: false = i can't fix this.
int CChar::FixWeirdness()
{
	ADDTOCALLSTACK("CChar::FixWeirdness");
	// Make sure my flags are good.
	// NOTE: Stats and skills may go negative temporarily.

	int iResultCode = CObjBase::IsWeird();
	if ( iResultCode )		// Not recoverable - must try to delete the object.
		return iResultCode;

	CCharBase *pCharDef = Char_GetDef();
	if ( IsStatFlag(STATF_HasShield) )
	{
		CItem *pShield = LayerFind(LAYER_HAND2);
		if ( !pShield )
			StatFlag_Clear(STATF_HasShield);
	}
	if ( IsStatFlag(STATF_OnHorse) )
	{
		CItem *pHorse = LayerFind(LAYER_HORSE);
		if ( !pHorse )
			StatFlag_Clear(STATF_OnHorse);
	}
	if ( IsStatFlag(STATF_Spawned) )
	{
		if ( !m_uidSpawnItem.ItemFind() )
			StatFlag_Clear(STATF_Spawned);
	}
	if ( IsStatFlag(STATF_Pet) )
	{
		CItemMemory *pMemory = Memory_FindTypes(MEMORY_IPET);
		if ( !pMemory )
			StatFlag_Clear(STATF_Pet);
	}
	if ( IsStatFlag(STATF_Ridden) )
	{
		// Move the ridden creature to the same location as it's rider.
		if ( !m_pNPC || !IsDisconnected() )
			StatFlag_Clear(STATF_Ridden);
		else
		{
			if ( Skill_GetActive() != NPCACT_RIDDEN )
				return 0x1203;

			CItem *pFigurine = Horse_GetMountItem();
			if ( !pFigurine )
				return 0x1204;

			CPointMap pt = pFigurine->GetTopLevelObj()->GetTopPoint();
			if ( pt != GetTopPoint() )
			{
				MoveToChar(pt);
				SetDisconnected();
			}
		}
	}
	if ( IsStatFlag(STATF_Criminal) )
	{
		CItem *pMemory = LayerFind(LAYER_FLAG_Criminal);
		if ( !pMemory )
			StatFlag_Clear(STATF_Criminal);
	}

	if ( !IsIndividualName() && pCharDef->GetTypeName()[0] == '#' )
		SetName(pCharDef->GetTypeName());

	// Automatic transition from old to new spawn engine
	CItemMemory *pMemory = Memory_FindTypes(MEMORY_ISPAWNED);
	if ( pMemory )
	{
		CItemSpawn *pSpawn = static_cast<CItemSpawn *>(pMemory->m_uidLink.ItemFind());
		pMemory->Delete();
		if ( pSpawn )
		{
			pSpawn->AddObj(GetUID());
			m_uidSpawnItem = pSpawn->GetUID();
		}
	}

	if ( m_pPlayer )
	{
		Memory_ClearTypes(MEMORY_IPET);
		StatFlag_Clear(STATF_Ridden);

		if ( m_pPlayer->GetSkillClass() == NULL )	// this should never happen.
		{
			m_pPlayer->SetSkillClass(this, RESOURCE_ID(RES_SKILLCLASS));
			ASSERT(m_pPlayer->GetSkillClass());
		}

		// Make sure players don't get ridiculous stats.
		//		m_iOverSkillMultiply disables this check if set to < 1
		if ( (GetPrivLevel() <= PLEVEL_Player) && (g_Cfg.m_iOverSkillMultiply > 0) )
		{
			for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; i++ )
			{
				WORD iSkillMax = Skill_GetMax(static_cast<SKILL_TYPE>(i));
				WORD iSkillVal = Skill_GetBase(static_cast<SKILL_TYPE>(i));
				if ( iSkillVal < 0 )
					Skill_SetBase(static_cast<SKILL_TYPE>(i), 0);
				if ( iSkillVal > iSkillMax * g_Cfg.m_iOverSkillMultiply )
					Skill_SetBase(static_cast<SKILL_TYPE>(i), iSkillMax);
			}

			// ??? What if magically enhanced !!!
			if ( IsPlayableCharacter() && (GetPrivLevel() < PLEVEL_Counsel) && !IsStatFlag(STATF_Polymorph) )
			{
				for ( int j = STAT_STR; j < STAT_BASE_QTY; j++ )
				{
					int iStatMax = Stat_GetLimit(static_cast<STAT_TYPE>(j));
					if ( Stat_GetAdjusted(static_cast<STAT_TYPE>(j)) > iStatMax*g_Cfg.m_iOverSkillMultiply )
						Stat_SetBase(static_cast<STAT_TYPE>(j), static_cast<short>(iStatMax));
				}
			}
		}
	}
	else
	{
		if ( !m_pNPC )
			return 0x1205;

		// An NPC. Don't keep track of unused skills.
		for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; i++ )
		{
			if ( m_Skill[i] > 0 && m_Skill[i] < g_Cfg.m_iSaveNPCSkills )
				Skill_SetBase(static_cast<SKILL_TYPE>(i), 0);
		}
	}

	if ( GetTimerAdjusted() > 60 * 60 )
		SetTimeout(1);	// unreasonably long for a char?

	return IsWeird();
}

// Creating a new char. (Not loading from save file) Make sure things are set to reasonable values.
void CChar::CreateNewCharCheck()
{
	ADDTOCALLSTACK("CChar::CreateNewCharCheck");
	m_prev_id = GetID();
	m_prev_Hue = GetHue();

	Stat_SetVal(STAT_STR, Stat_GetMax(STAT_STR));
	Stat_SetVal(STAT_DEX, Stat_GetMax(STAT_DEX));
	Stat_SetVal(STAT_INT, Stat_GetMax(STAT_INT));

	if ( !m_pPlayer )	// need a starting brain tick.
	{
		//	auto-set EXP/LEVEL level
		if ( g_Cfg.m_bExperienceSystem && g_Cfg.m_iExperienceMode&EXP_MODE_AUTOSET_EXP )
		{
			if ( !m_exp )
			{
				CCharBase *pCharDef = Char_GetDef();

				int mult = (Stat_GetMax(STAT_STR) + (Stat_GetMax(STAT_DEX) / 2) + Stat_GetMax(STAT_INT)) / 3;
				m_exp = maximum(
						Skill_GetBase(SKILL_ARCHERY),
						maximum(Skill_GetBase(SKILL_THROWING),
						maximum(Skill_GetBase(SKILL_SWORDSMANSHIP),
						maximum(Skill_GetBase(SKILL_MACEFIGHTING),
						maximum(Skill_GetBase(SKILL_FENCING),
						Skill_GetBase(SKILL_WRESTLING)))))
					) +
					(Skill_GetBase(SKILL_TACTICS)     / 4) +
					(Skill_GetBase(SKILL_PARRYING)    / 4) +
					(Skill_GetBase(SKILL_MAGERY)      / 3) +
					(Skill_GetBase(SKILL_PROVOCATION) / 4) +
					(Skill_GetBase(SKILL_PEACEMAKING) / 4) +
					(Skill_GetBase(SKILL_TAMING)      / 4) +
					(pCharDef->m_defense    * 3) +
					(pCharDef->m_attackBase * 6)
					;
				m_exp = (m_exp * mult) / 100;
			}

			if ( !m_level && g_Cfg.m_bLevelSystem && ( m_exp > g_Cfg.m_iLevelNextAt ))
				ChangeExperience();
		}

		SetTimeout(1);
	}
}


bool CChar::DupeFrom( CChar * pChar, bool fNewbieItems )
{
	// CChar part
	if ( !pChar )
		return false;

	m_pArea = pChar->m_pArea;
	m_pRoom = pChar->m_pRoom;
	m_StatFlag = pChar->m_StatFlag;

	if ( g_World.m_fSaveParity )
		StatFlag_Set(STATF_SaveParity);	// It will get saved next time.

	m_dirFace = pChar->m_dirFace;
	m_fonttype = pChar->m_fonttype;
	m_SpeechHue = pChar->m_SpeechHue;

	m_height = pChar->m_height;
	m_ModMaxWeight = pChar->m_ModMaxWeight;
	
	m_StepStealth = pChar->m_StepStealth;
	m_iVisualRange = pChar->m_iVisualRange;
	m_virtualGold = pChar->m_virtualGold;

	m_exp = pChar->m_exp;
	m_level = pChar->m_level;
	m_defense = pChar->m_defense;
	m_atUnk.m_Arg1 = pChar->m_atUnk.m_Arg1;
	m_atUnk.m_Arg2 = pChar->m_atUnk.m_Arg2;
	m_atUnk.m_Arg3 = pChar->m_atUnk.m_Arg3;

	m_timeLastRegen = pChar->m_timeLastRegen;
	m_timeCreate = pChar->m_timeCreate;

	m_timeLastHitsUpdate = pChar->m_timeLastHitsUpdate;

	m_prev_Hue = pChar->m_prev_Hue;
	SetHue(pChar->GetHue());
	m_prev_id = pChar->m_prev_id;
	SetID( pChar->GetID() );
	m_Can = pChar->m_Can;
	m_wBloodHue = pChar->m_wBloodHue;
	//m_totalweight = 0;

	Skill_Cleanup();

	g_World.m_uidLastNewChar = GetUID();	// for script access.

	for ( size_t i = 0; i < STAT_QTY; i++ )
	{
		Stat_SetBase(static_cast<STAT_TYPE>(i), pChar->Stat_GetBase(static_cast<STAT_TYPE>(i)));
		Stat_SetMod(static_cast<STAT_TYPE>(i), pChar->Stat_GetMod(static_cast<STAT_TYPE>(i)));
		Stat_SetVal(static_cast<STAT_TYPE>(i), pChar->Stat_GetVal(static_cast<STAT_TYPE>(i)));
		Stat_SetMax(static_cast<STAT_TYPE>(i), pChar->Stat_GetMax(static_cast<STAT_TYPE>(i)));
		m_Stat[i].m_regen = 0;
	}

	for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; i++ )
	{
		m_Skill[i] = pChar->m_Skill[i];
	}

	m_LocalLight = pChar->m_LocalLight;
	m_fClimbUpdated = pChar->m_fClimbUpdated;
	/*if ( m_pNPC )
	{
		if (pChar->m_pNPC)
			m_pNPC->m_Speech.Copy(&(pChar->m_pNPC->m_Speech));
		else
			m_pNPC->m_Speech.Copy(&(pChar->m_pPlayer->m_Speech));
	}else{
		if (pChar->m_pNPC)
			m_pPlayer->m_Speech.Copy(&(pChar->m_pNPC->m_Speech));
		else
			m_pPlayer->m_Speech.Copy(&(pChar->m_pPlayer->m_Speech));
	}*/
	
	FixWeirdness();
	SetName( pChar->GetName());	// SetName after FixWeirdness, otherwise it can be replaced again.
	// We copy tags,etc first and place it because of NPC_LoadScript and @Create trigger, so it have information before calling it
	m_TagDefs.Copy( &( pChar->m_TagDefs ) );
	m_BaseDefs.Copy( &( pChar->m_BaseDefs ) );
	m_OEvents.Copy(&(pChar->m_OEvents));
	//NPC_LoadScript( false );	//Calling it now so everything above can be accessed and overrided in the @Create
	//Not calling NPC_LoadScript() because, in some part, it's breaking the name and looking for template names.
	// end of CChar

	// Begin copying items.
	LAYER_TYPE layer;
	for ( int i = 0 ; i < LAYER_QTY; i++)
	{
		layer = static_cast<LAYER_TYPE>(i);
		CItem * myLayer = LayerFind(layer);
		if ( myLayer )
			myLayer->Delete();

		if ( !pChar->LayerFind( layer ) )
			continue;

		CItem * pItem = CItem::CreateDupeItem(pChar->LayerFind( static_cast<LAYER_TYPE>(i) ),this,true);
		pItem->LoadSetContainer(GetUID(),static_cast<LAYER_TYPE>(i));
		if ( fNewbieItems )
		{
			pItem->SetAttr(ATTR_NEWBIE);
			if (pItem->IsType(IT_CONTAINER) )
			{
				for ( CItem *pItemCont = static_cast<CItemContainer*>(pItem)->GetContentHead(); pItemCont != NULL; pItemCont = pItemCont->GetNext() )
				{
					pItemCont->SetAttr(ATTR_NEWBIE);

					CChar *pTest = static_cast<CChar*>(static_cast<CGrayUID>(pItemCont->m_itNormal.m_more1).CharFind());
					if ( pTest && pTest == pChar )
						pItemCont->m_itNormal.m_more1 = static_cast<DWORD>(GetUID());

					CChar *pTest2 = static_cast<CChar*>(static_cast<CGrayUID>(pItemCont->m_itNormal.m_more2).CharFind());
					if ( pTest2 && pTest2 == pChar )
						pItemCont->m_itNormal.m_more2 = static_cast<DWORD>(GetUID());

					CChar *pTest3 = static_cast<CChar*>(static_cast<CGrayUID>(pItemCont->m_uidLink).CharFind());
					if ( pTest3 && pTest3 == pChar )
						pItemCont->m_uidLink = GetUID();
				}
			}
		}
		CChar * pTest = static_cast<CChar*>(static_cast<CGrayUID>(pItem->m_itNormal.m_more1).CharFind());
		if ( pTest && pTest == pChar)
			pItem->m_itNormal.m_more1 = static_cast<DWORD>(GetUID());

		CChar * pTest2 = static_cast<CChar*>(static_cast<CGrayUID>(pItem->m_itNormal.m_more2).CharFind());
		if ( pTest2)
		{
			if ( pTest2 == pChar)
				pItem->m_itNormal.m_more2 = static_cast<DWORD>(GetUID());
			else if ( pTest2->NPC_IsOwnedBy(pChar, true) )	// Mount's fix
			{
				if ( fNewbieItems )	// Removing any mount references for the memory, so when character dies or dismounts ... no mount will appear.
					pItem->m_itNormal.m_more2 = 0;
				else	// otherwise we create a full new character.
				{
					pItem->m_itNormal.m_more2 = 0;	// more2 should be cleared before removing the memory or the real npc will be removed too.
					pItem->Delete(true);
					CChar * pChar2 = CreateNPC( pTest2->GetID());
					pChar2->SetTopPoint( pChar->GetTopPoint() );	// Moving the mount again because the dupe will place it in the same place as the 'invisible & disconnected' original (usually far away from where the guy will be, so the duped char can't touch the mount).
					pChar2->DupeFrom(pTest2,fNewbieItems);
					pChar2->NPC_PetSetOwner(this);
					Horse_Mount(pChar2);
				}
			}
		}

		CChar * pTest3 = static_cast<CChar*>(static_cast<CGrayUID>(pItem->m_uidLink).CharFind());
		if ( pTest3 && pTest3 == pChar)
			pItem->m_uidLink = GetUID();
		
	}
	// End copying items.
	FixWeight();
	Update();
	return true;
}

// Reading triggers from CHARDEF
bool CChar::ReadScriptTrig(CCharBase * pCharDef, CTRIG_TYPE trig, bool bVendor)
{
	ADDTOCALLSTACK("CChar::ReadScriptTrig");
	if ( !pCharDef || !pCharDef->HasTrigger(trig) )
		return false;
	CResourceLock s;
	if ( !pCharDef->ResourceLock(s) || !OnTriggerFind(s, sm_szTrigName[trig]) )
		return false;
	return ReadScript(s, bVendor);
}

// If this is a regen they will have a pack already.
// RETURN:
//  true = default return. (mostly ignored).
bool CChar::ReadScript(CResourceLock &s, bool bVendor)
{
	ADDTOCALLSTACK("CChar::ReadScript");
	bool fFullInterp		= false;

	CItem * pItem = NULL;
	while ( s.ReadKeyParse() )
	{
		if ( s.IsKeyHead("ON", 2) )
			break;

		int iCmd = FindTableSorted(s.GetKey(), CItem::sm_szTemplateTable, COUNTOF(CItem::sm_szTemplateTable)-1);

		if ( bVendor )
		{
			if (iCmd != -1)
			{
				switch ( iCmd )
				{
					case ITC_BUY:
					case ITC_SELL:
						{
							CItemContainer *pCont = GetContainerCreate((iCmd == ITC_SELL) ? LAYER_VENDOR_STOCK : LAYER_VENDOR_BUYS);
							if ( pCont )
							{
								pItem = CItem::CreateHeader(s.GetArgRaw(), pCont, false);
								if ( pItem )
									pItem->m_TagDefs.SetNum("NOSAVE", 1);
							}
							pItem = NULL;
							continue;
						}
					default:
						pItem = NULL;
						continue;
				}
			}
		}
		else
		{
			switch ( iCmd )
			{
				case ITC_FULLINTERP:
					{
						LPCTSTR		pszArgs	= s.GetArgStr();
						GETNONWHITESPACE(pszArgs);
						fFullInterp = ( *pszArgs == '\0' ) ? true : ( s.GetArgVal() != 0);
						continue;
					}
				case ITC_NEWBIESWAP:
					{
						if ( !pItem )
							continue;

						if ( pItem->IsAttr( ATTR_NEWBIE ) )
						{
							if ( Calc_GetRandVal( s.GetArgVal() ) == 0 )
								pItem->ClrAttr(ATTR_NEWBIE);
						}
						else
						{
							if ( Calc_GetRandVal( s.GetArgVal() ) == 0 )
								pItem->SetAttr(ATTR_NEWBIE);
						}
						continue;
					}
				case ITC_ITEM:
				case ITC_CONTAINER:
				case ITC_ITEMNEWBIE:
					{
						if ( IsStatFlag( STATF_Conjured ) && iCmd != ITC_ITEMNEWBIE ) // This check is not needed.
							break; // conjured creates have no loot.

						pItem = CItem::CreateHeader( s.GetArgRaw(), this, iCmd == ITC_ITEMNEWBIE );
						if ( pItem == NULL )
						{
							m_UIDLastNewItem = GetUID();	// Setting m_UIDLastNewItem to CChar's UID to prevent calling any following functions meant to be called on that item
							continue;
						}
						m_UIDLastNewItem.InitUID();	//Clearing the attr for the next cycle

						if ( iCmd == ITC_ITEMNEWBIE )
						{
							pItem->SetAttr(ATTR_NEWBIE);
						}

						if ( !pItem->IsItemInContainer() && !pItem->IsItemEquipped())
							pItem = NULL;
						continue;
					}

				case ITC_BREAK:
				case ITC_BUY:
				case ITC_SELL:
					pItem = NULL;
					continue;
			}

		}

		if ( m_UIDLastNewItem == GetUID() )
			continue;
		if ( pItem != NULL )
		{
			if ( fFullInterp )	// Modify the item.
				pItem->r_Verb( s, &g_Serv );
			else
				pItem->r_LoadVal( s );
		}
		else
		{
			TRIGRET_TYPE tRet = OnTriggerRun( s, TRIGRUN_SINGLE_EXEC, &g_Serv, NULL, NULL );
			if ( (tRet == TRIGRET_RET_FALSE) && fFullInterp )
				;
			else if ( tRet != TRIGRET_RET_DEFAULT )
			{
				m_UIDLastNewItem.InitUID();
				return (tRet == TRIGRET_RET_FALSE);
			}
		}
	}

	return( true );
}

// Create an NPC from script.
void CChar::NPC_LoadScript( bool fRestock )
{
	ADDTOCALLSTACK("CChar::NPC_LoadScript");
	if ( m_pNPC == NULL )
		// Set a default brian type til we get the real one from scripts.
		SetNPCBrain(GetNPCBrain(false));	// should have a default brain. watch out for override vendor.

	CCharBase * pCharDef = Char_GetDef();

	// 1) CHARDEF trigger
	if ( m_pPlayer == NULL ) //	CHARDEF triggers (based on body type)
	{
		CChar * pChar = GetChar();
		if ( pChar != NULL )
		{
			CGrayUID uidOldAct = pChar->m_Act_Targ;
			pChar->m_Act_Targ = GetUID();
			pChar->ReadScriptTrig(pCharDef, CTRIG_Create);
			pChar->m_Act_Targ = uidOldAct;
		}
	}
	//This remains untouched but moved after the chardef's section
	if (( fRestock ) && ( IsTrigUsed(TRIGGER_NPCRESTOCK) ))
		ReadScriptTrig(pCharDef, CTRIG_NPCRestock);

	CreateNewCharCheck();	//This one is giving stats, etc to the char, so we can read/set them in the next triggers.
}

// @Create trigger, NPC version
void CChar::NPC_CreateTrigger()
{
	ADDTOCALLSTACK("CChar::NPC_CreateTrigger");
	if (!m_pNPC)
		return;

	CCharBase *pCharDef = Char_GetDef();
	TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;
	LPCTSTR pszTrigName = "@Create";
	CTRIG_TYPE iAction = (CTRIG_TYPE)FindTableSorted(pszTrigName, sm_szTrigName, COUNTOF(sm_szTrigName) - 1);

	// 2) TEVENTS
	for (size_t i = 0; i < pCharDef->m_TEvents.GetCount(); ++i)
	{
		CResourceLink * pLink = pCharDef->m_TEvents[i];
		if (!pLink || !pLink->HasTrigger(iAction))
			continue;
		CResourceLock s;
		if (!pLink->ResourceLock(s))
			continue;
		iRet = CScriptObj::OnTriggerScript(s, pszTrigName, this, 0);
		if (iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT)
			return;
	}

	// 4) EVENTSPET triggers
	for (size_t i = 0; i < g_Cfg.m_pEventsPetLink.GetCount(); ++i)
	{
		CResourceLink * pLink = g_Cfg.m_pEventsPetLink[i];
		if (!pLink || !pLink->HasTrigger(iAction))
			continue;
		CResourceLock s;
		if (!pLink->ResourceLock(s))
			continue;
		iRet = CScriptObj::OnTriggerScript(s, pszTrigName, this, 0);
		if (iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT)
			return;
	}
}

void CChar::OnWeightChange( int iChange )
{
	ADDTOCALLSTACK("CChar::OnWeightChange");
	CContainer::OnWeightChange( iChange );
	UpdateStatsFlag();
}

bool CChar::SetName( LPCTSTR pszName )
{
	ADDTOCALLSTACK("CChar::SetName");
	return SetNamePool( pszName );
}

height_t CChar::GetHeightMount( bool fEyeSubstract ) const
{
	ADDTOCALLSTACK("CChar::GetHeightMount");
	height_t height = GetHeight();
	if ( IsStatFlag(STATF_OnHorse|STATF_Hovering) )
		height += 4;
	if ( fEyeSubstract )
		--height;
	return ( height ); //if mounted +4, if not -1 (let's say it's eyes' height)
}

height_t CChar::GetHeight() const
{
	ADDTOCALLSTACK("CChar::GetHeight");
	if ( m_height ) //set by a dynamic variable (On=@Create  Height=10)
		return m_height;

	height_t tmpHeight;

	CCharBase * pCharDef = Char_GetDef();
	tmpHeight = pCharDef->GetHeight();
	if ( tmpHeight ) //set by a chardef variable ([CHARDEF 10]  Height=10)
		return tmpHeight;

	char * heightDef = Str_GetTemp();

	sprintf(heightDef, "height_0%x", static_cast<unsigned int>(pCharDef->GetDispID()));
	tmpHeight = static_cast<height_t>(g_Exp.m_VarDefs.GetKeyNum(heightDef));
	if ( tmpHeight ) //set by a defname ([DEFNAME charheight]  height_0a)
		return tmpHeight;

	sprintf(heightDef, "height_%u", static_cast<unsigned int>(pCharDef->GetDispID()));
	tmpHeight = static_cast<height_t>(g_Exp.m_VarDefs.GetKeyNum(heightDef));
	if ( tmpHeight ) //set by a defname ([DEFNAME charheight]  height_10)
		return tmpHeight;

	return PLAYER_HEIGHT; //if everything fails
}

// Just set the base id and not the actual display id.
// NOTE: Never return NULL
void CChar::SetID( CREID_TYPE id )
{
	ADDTOCALLSTACK("CChar::SetID");

	CCharBase * pCharDef = CCharBase::FindCharBase(id);
	if ( pCharDef == NULL )
	{
		if ( id != -1 && id != CREID_INVALID )
			DEBUG_ERR(("Create Invalid Char 0%x\n", id));

		id = static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType(RES_CHARDEF, "DEFAULTCHAR"));
		if ( id < 0 )
			id = CREID_OGRE;

		pCharDef = CCharBase::FindCharBase(id);
	}

	if ( pCharDef == Char_GetDef() )
		return;

	m_BaseRef.SetRef(pCharDef);
	if ( m_prev_id == CREID_INVALID )
		m_prev_id = GetID();

	if ( !IsMountCapable() )	// new body may not be capable of riding mounts
		Horse_UnMount();

	if ( !pCharDef->Can(CAN_C_EQUIP) )	// new body may not be capable of equipping items (except maybe on hands)
	{
		UnEquipAllItems(NULL, pCharDef->Can(CAN_C_USEHANDS));
	}
	else if ( !pCharDef->Can(CAN_C_USEHANDS) )
	{
		CItem *pHand = LayerFind(LAYER_HAND1);
		if ( pHand )
			GetContainerCreate(LAYER_PACK)->ContentAdd(pHand);

		pHand = LayerFind(LAYER_HAND2);
		if ( pHand )
			GetContainerCreate(LAYER_PACK)->ContentAdd(pHand);
	}
	UpdateMode(NULL, true);
}

// Create a brand new Player char. Called directly from the packet.
void CChar::InitPlayer(CClient *pClient, const char *pszCharname, bool bFemale, RACE_TYPE rtRace, short wStr, short wDex, short wInt, PROFESSION_TYPE prProf, SKILL_TYPE skSkill1, int iSkillVal1, SKILL_TYPE skSkill2, int iSkillVal2, SKILL_TYPE skSkill3, int iSkillVal3, SKILL_TYPE skSkill4, int iSkillVal4, HUE_TYPE wSkinHue, ITEMID_TYPE idHair, HUE_TYPE wHairHue, ITEMID_TYPE idBeard, HUE_TYPE wBeardHue, HUE_TYPE wShirtHue, HUE_TYPE wPantsHue, ITEMID_TYPE idFace, int iStartLoc)
{
	ADDTOCALLSTACK("CChar::InitPlayer");
	ASSERT(pClient);

	CAccount *pAccount = pClient->m_pAccount;
	if ( pAccount )
		SetPlayerAccount(pAccount);

	switch ( rtRace )
	{
		default:
		case RACETYPE_HUMAN:
			SetID(bFemale ? CREID_WOMAN : CREID_MAN);
			break;
		case RACETYPE_ELF:
			SetID(bFemale ? CREID_ELFWOMAN : CREID_ELFMAN);
			break;
		case RACETYPE_GARGOYLE:
			SetID(bFemale ? CREID_GARGWOMAN : CREID_GARGMAN);
			break;
	}

	// Set name
	bool bNameIsAccepted = true;
	TCHAR *zCharName = Str_GetTemp();
	strcpylen(zCharName, pszCharname, MAX_NAME_SIZE);

	if ( !strlen(zCharName) || g_Cfg.IsObscene(zCharName) || Str_CheckName(zCharName) || !strnicmp(zCharName, "lord ", 5) || !strnicmp(zCharName, "lady ", 5) || !strnicmp(zCharName, "seer ", 5) || !strnicmp(zCharName, "gm ", 3) || !strnicmp(zCharName, "admin ", 6) || !strnicmp(zCharName, "counselor ", 10) )
		bNameIsAccepted = false;

	if ( bNameIsAccepted && IsTrigUsed(TRIGGER_RENAME) )
	{
		CScriptTriggerArgs args;
		args.m_s1 = zCharName;
		args.m_pO1 = this;
		if ( OnTrigger(CTRIG_Rename, this, &args) == TRIGRET_RET_TRUE )
			bNameIsAccepted = false;
	}

	if ( bNameIsAccepted )
		SetName(zCharName);
	else
		SetNamePool(bFemale ? "#NAMES_HUMANFEMALE" : "#NAMES_HUMANMALE");

	if ( g_Cfg.m_StartDefs.IsValidIndex(iStartLoc) )
		m_ptHome = g_Cfg.m_StartDefs[iStartLoc]->m_pt;
	else
		m_ptHome.InitPoint();

	if ( !m_ptHome.IsValidPoint() )
		DEBUG_ERR(("Invalid start location '%d' for character!\n", iStartLoc));

	SetUnkPoint(m_ptHome);	// don't actually put me in the world yet.

	// Set stats
	if ( wStr > 60 )		wStr = 60;
	if ( wDex > 60 )		wDex = 60;
	if ( wInt > 60 )		wInt = 60;
	if ( iSkillVal1 > 50 )	iSkillVal1 = 50;
	if ( iSkillVal2 > 50 )	iSkillVal2 = 50;
	if ( iSkillVal3 > 50 )	iSkillVal3 = 50;
	if ( iSkillVal4 > 50 )	iSkillVal4 = 50;

	if ( skSkill4 != SKILL_NONE )
	{
		if ( (wStr + wDex + wInt) > 90 )
			wStr = wDex = wInt = 30;

		if ( (iSkillVal1 + iSkillVal2 + iSkillVal3 + iSkillVal4) > 120 )
			iSkillVal4 = 1;
	}
	else
	{
		if ( (wStr + wDex + wInt) > 80 )
			wStr = wDex = wInt = 26;

		if ( (iSkillVal1 + iSkillVal2 + iSkillVal3) > 100 )
			iSkillVal3 = 1;
	}
	Stat_SetBase(STAT_STR, wStr);
	Stat_SetBase(STAT_DEX, wDex);
	Stat_SetBase(STAT_INT, wInt);

	// Set skills
	for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; i++ )
	{
		if ( g_Cfg.m_SkillIndexDefs.IsValidIndex(i) )
			Skill_SetBase(static_cast<SKILL_TYPE>(i), static_cast<WORD>(Calc_GetRandVal(g_Cfg.m_iMaxBaseSkill)));
	}
	if ( IsSkillBase(skSkill1) && g_Cfg.m_SkillIndexDefs.IsValidIndex(skSkill1) )
		Skill_SetBase(skSkill1, iSkillVal1 * 10);
	if ( IsSkillBase(skSkill2) && g_Cfg.m_SkillIndexDefs.IsValidIndex(skSkill2) )
		Skill_SetBase(skSkill2, iSkillVal2 * 10);
	if ( IsSkillBase(skSkill3) && g_Cfg.m_SkillIndexDefs.IsValidIndex(skSkill3) )
		Skill_SetBase(skSkill3, iSkillVal3 * 10);
	if ( IsSkillBase(skSkill4) && g_Cfg.m_SkillIndexDefs.IsValidIndex(skSkill4) )
		Skill_SetBase(skSkill4, iSkillVal4 * 10);

	// Validate skin hue
	switch ( rtRace )
	{
		default:
		case RACETYPE_HUMAN:
			if ( wSkinHue < HUE_SKIN_LOW )
				wSkinHue = static_cast<HUE_TYPE>(HUE_SKIN_LOW);
			if ( wSkinHue > HUE_SKIN_HIGH )
				wSkinHue = static_cast<HUE_TYPE>(HUE_SKIN_HIGH);
			break;

		case RACETYPE_ELF:
		{
			static const HUE_TYPE sm_ElfSkinHues[] = { 0xBF, 0x24D, 0x24E, 0x24F, 0x353, 0x361, 0x367, 0x374, 0x375, 0x376, 0x381, 0x382, 0x383, 0x384, 0x385, 0x389, 0x3DE, 0x3E5, 0x3E6, 0x3E8, 0x3E9, 0x430, 0x4A7, 0x4DE, 0x51D, 0x53F, 0x579, 0x76B, 0x76C, 0x76D, 0x835, 0x903 };
			bool bValid = false;
			for ( size_t i = 0; i < COUNTOF(sm_ElfSkinHues); i++ )
			{
				if ( sm_ElfSkinHues[i] == wSkinHue )
				{
					bValid = true;
					break;
				}
			}
			if ( !bValid )
				wSkinHue = sm_ElfSkinHues[0];
			break;
		}

		case RACETYPE_GARGOYLE:
			if ( wSkinHue < HUE_GARGSKIN_LOW )
				wSkinHue = static_cast<HUE_TYPE>(HUE_GARGSKIN_LOW);
			if ( wSkinHue > HUE_GARGSKIN_HIGH )
				wSkinHue = static_cast<HUE_TYPE>(HUE_GARGSKIN_HIGH);
			break;
	}
	SetHue(wSkinHue|HUE_MASK_UNDERWEAR);

	// Create basic resources
	GetContainerCreate(LAYER_BANKBOX);
	GetContainerCreate(LAYER_PACK);
	CreateNewCharCheck();

	// Create hair
	switch ( rtRace )
	{
		default:
		case RACETYPE_HUMAN:
			if ( !(((idHair >= ITEMID_HAIR_SHORT) && (idHair <= ITEMID_HAIR_PONYTAIL)) || ((idHair >= ITEMID_HAIR_MOHAWK) && (idHair <= ITEMID_HAIR_TOPKNOT))) )
				idHair = ITEMID_NOTHING;
			if ( bFemale )
			{
				if ( idHair == ITEMID_HAIR_RECEDING )
					idHair = ITEMID_NOTHING;
			}
			else
			{
				if ( idHair == ITEMID_HAIR_BUNS )
					idHair = ITEMID_NOTHING;
			}
			break;

		case RACETYPE_ELF:
			if ( !(((idHair >= ITEMID_HAIR_ML_ELF) && (idHair <= ITEMID_HAIR_ML_MULLET)) || ((idHair >= ITEMID_HAIR_ML_FLOWER) && (idHair <= ITEMID_HAIR_ML_SPYKE))) )
				idHair = ITEMID_NOTHING;
			if ( bFemale )
			{
				if ( (idHair == ITEMID_HAIR_ML_LONG2) || (idHair == ITEMID_HAIR_ML_ELF) )
					idHair = ITEMID_NOTHING;
			}
			else
			{
				if ( (idHair == ITEMID_HAIR_ML_FLOWER) || (idHair == ITEMID_HAIR_ML_LONG4) )
					idHair = ITEMID_NOTHING;
			}
			break;

		case RACETYPE_GARGOYLE:
			if ( bFemale )
			{
				if ( !((idHair == ITEMID_GARG_HORN_FEMALE_1) || (idHair == ITEMID_GARG_HORN_FEMALE_2) || ((idHair >= ITEMID_GARG_HORN_FEMALE_3) && (idHair <= ITEMID_GARG_HORN_FEMALE_5)) || (idHair == ITEMID_GARG_HORN_FEMALE_6) || (idHair == ITEMID_GARG_HORN_FEMALE_7) || (idHair == ITEMID_GARG_HORN_FEMALE_8)) )
					idHair = ITEMID_NOTHING;
			}
			else
			{
				if ( !((idHair >= ITEMID_GARG_HORN_1) && (idHair <= ITEMID_GARG_HORN_8)) )
					idHair = ITEMID_NOTHING;
			}
			break;
	}

	if ( idHair )
	{
		CItem *pHair = CItem::CreateScript(idHair, this);
		ASSERT(pHair);
		if ( !pHair->IsType(IT_HAIR) )
			pHair->Delete();
		else
		{
			switch ( rtRace )
			{
				default:
				case RACETYPE_HUMAN:
					if ( wHairHue < HUE_HAIR_LOW )
						wHairHue = static_cast<HUE_TYPE>(HUE_HAIR_LOW);
					if ( wHairHue > HUE_HAIR_HIGH )
						wHairHue = static_cast<HUE_TYPE>(HUE_HAIR_HIGH);
					break;

				case RACETYPE_ELF:
				{
					static const HUE_TYPE sm_ElfHairHues[] = { 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x58, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x101, 0x159, 0x15A, 0x15B, 0x15C, 0x15D, 0x15E, 0x128, 0x12F, 0x1BD, 0x1E4, 0x1F3, 0x207, 0x211, 0x239, 0x251, 0x26C, 0x2C3, 0x2C9, 0x31D, 0x31E, 0x31F, 0x320, 0x321, 0x322, 0x323, 0x324, 0x325, 0x326, 0x369, 0x386, 0x387, 0x388, 0x389, 0x38A, 0x59D, 0x6B8, 0x725, 0x853 };
					bool bValid = false;
					for ( size_t i = 0; i < COUNTOF(sm_ElfHairHues); i++ )
					{
						if ( sm_ElfHairHues[i] == wHairHue )
						{
							bValid = true;
							break;
						}
					}
					if ( !bValid )
						wHairHue = sm_ElfHairHues[0];
					break;
				}

				case RACETYPE_GARGOYLE:
				{
					static const HUE_TYPE sm_GargoyleHornHues[] = { 0x709, 0x70B, 0x70D, 0x70F, 0x711, 0x763, 0x765, 0x768, 0x76B, 0x6F3, 0x6F1, 0x6EF, 0x6E4, 0x6E2, 0x6E0, 0x709, 0x70B, 0x70D };
					bool bValid = false;
					for ( size_t i = 0; i < COUNTOF(sm_GargoyleHornHues); i++ )
					{
						if ( sm_GargoyleHornHues[i] == wHairHue )
						{
							bValid = true;
							break;
						}
					}
					if ( !bValid )
						wHairHue = sm_GargoyleHornHues[0];
					break;
				}
			}
			pHair->SetHue(wHairHue);
			pHair->SetAttr(ATTR_NEWBIE|ATTR_MOVE_NEVER);
			LayerAdd(pHair);
		}
	}

	// Create beard
	if ( bFemale )
		idBeard = ITEMID_NOTHING;
	else
	{
		switch ( rtRace )
		{
			default:
			case RACETYPE_HUMAN:
				if ( !(((idBeard >= ITEMID_BEARD_LONG) && (idBeard <= ITEMID_BEARD_MOUSTACHE)) || ((idBeard >= ITEMID_BEARD_SH_M) && (idBeard <= ITEMID_BEARD_GO_M))) )
					idBeard = ITEMID_NOTHING;
				break;

			case RACETYPE_ELF:
				idBeard = ITEMID_NOTHING;
				break;

			case RACETYPE_GARGOYLE:
				if ( !((idBeard >= ITEMID_GARG_HORN_FACIAL_1) && (idBeard <= ITEMID_GARG_HORN_FACIAL_4)) )
					idBeard = ITEMID_NOTHING;
				break;
		}
	}

	if ( idBeard )
	{
		CItem *pBeard = CItem::CreateScript(idBeard, this);
		ASSERT(pBeard);
		if ( !pBeard->IsType(IT_BEARD) )
			pBeard->Delete();
		else
		{
			switch ( rtRace )
			{
				case RACETYPE_HUMAN:
					if ( wBeardHue < HUE_HAIR_LOW )
						wBeardHue = static_cast<HUE_TYPE>(HUE_HAIR_LOW);
					if ( wBeardHue > HUE_HAIR_HIGH )
						wBeardHue = static_cast<HUE_TYPE>(HUE_HAIR_HIGH);
					break;

				case RACETYPE_GARGOYLE:
				{
					static const HUE_TYPE sm_GargoyleBeardHues[] = { 0x709, 0x70B, 0x70D, 0x70F, 0x711, 0x763, 0x765, 0x768, 0x76B, 0x6F3, 0x6F1, 0x6EF, 0x6E4, 0x6E2, 0x6E0, 0x709, 0x70B, 0x70D };
					bool bValid = false;
					for ( size_t i = 0; i < COUNTOF(sm_GargoyleBeardHues); i++ )
					{
						if ( sm_GargoyleBeardHues[i] == wHairHue )
						{
							bValid = true;
							break;
						}
					}
					if ( !bValid )
						wHairHue = sm_GargoyleBeardHues[0];
					break;
				}

				default:
					break;
			}
			pBeard->SetHue(wBeardHue);
			pBeard->SetAttr(ATTR_NEWBIE|ATTR_MOVE_NEVER);
			LayerAdd(pBeard);
		}
	}

	// Create face (enhanced clients only)
	if ( idFace )
	{
		switch ( rtRace )
		{
			case RACETYPE_GARGOYLE:
				if ( !((idFace >= ITEMID_FACE_1_GARG) && (idFace <= ITEMID_FACE_6_GARG)) )
					idFace = ITEMID_NOTHING;
				break;

			default:
				if ( !(((idFace >= ITEMID_FACE_1) && (idFace <= ITEMID_FACE_10)) || ((idFace >= ITEMID_FACE_ANIME) && (idFace <= ITEMID_FACE_VAMPIRE))) )
					idFace = ITEMID_NOTHING;
				break;
		}

		CItem *pFace = CItem::CreateScript(idFace, this);
		ASSERT(pFace);
		pFace->SetHue(wSkinHue);
		pFace->SetAttr(ATTR_NEWBIE|ATTR_MOVE_NEVER);
		LayerAdd(pFace);
	}

	// Get starting items for the profession / skills.
	int iProfession = INT_MAX;
	bool bCreateSkillItems = true;
	switch ( prProf )
	{
		case PROFESSION_ADVANCED:
			iProfession = RES_NEWBIE_PROF_ADVANCED;
			break;
		case PROFESSION_WARRIOR:
			iProfession = RES_NEWBIE_PROF_WARRIOR;
			break;
		case PROFESSION_MAGE:
			iProfession = RES_NEWBIE_PROF_MAGE;
			break;
		case PROFESSION_BLACKSMITH:
			iProfession = RES_NEWBIE_PROF_BLACKSMITH;
			break;
		case PROFESSION_NECROMANCER:
			iProfession = RES_NEWBIE_PROF_NECROMANCER;
			bCreateSkillItems = false;
			break;
		case PROFESSION_PALADIN:
			iProfession = RES_NEWBIE_PROF_PALADIN;
			bCreateSkillItems = false;
			break;
		case PROFESSION_SAMURAI:
			iProfession = RES_NEWBIE_PROF_SAMURAI;
			bCreateSkillItems = false;
			break;
		case PROFESSION_NINJA:
			iProfession = RES_NEWBIE_PROF_NINJA;
			bCreateSkillItems = false;
			break;
	}

	CResourceLock s;
	if ( g_Cfg.ResourceLock(s, RESOURCE_ID(RES_NEWBIE, bFemale ? RES_NEWBIE_FEMALE_DEFAULT : RES_NEWBIE_MALE_DEFAULT, rtRace)) )
		ReadScript(s);
	else if ( g_Cfg.ResourceLock(s, RESOURCE_ID(RES_NEWBIE, bFemale ? RES_NEWBIE_FEMALE_DEFAULT : RES_NEWBIE_MALE_DEFAULT)) )
		ReadScript(s);

	if ( g_Cfg.ResourceLock(s, RESOURCE_ID(RES_NEWBIE, iProfession, rtRace)) )
		ReadScript(s);
	else if ( g_Cfg.ResourceLock(s, RESOURCE_ID(RES_NEWBIE, iProfession)) )
		ReadScript(s);

	if ( bCreateSkillItems )
	{
		for ( int i = 1; i < 5; i++ )
		{
			int iSkill = INT_MAX;
			switch ( i )
			{
				case 1:
					iSkill = skSkill1;
					break;
				case 2:
					iSkill = skSkill2;
					break;
				case 3:
					iSkill = skSkill3;
					break;
				case 4:
					iSkill = skSkill4;
					break;
			}

			if ( g_Cfg.ResourceLock(s, RESOURCE_ID(RES_NEWBIE, iSkill, rtRace)) )
				ReadScript(s);
			else if ( g_Cfg.ResourceLock(s, RESOURCE_ID(RES_NEWBIE, iSkill)) )
				ReadScript(s);
		}
	}

	CItem *pLayer = LayerFind(LAYER_SHIRT);
	if ( pLayer )
	{
		if ( wShirtHue < HUE_BLUE_LOW )
			wShirtHue = HUE_BLUE_LOW;
		if ( wShirtHue > HUE_DYE_HIGH )
			wShirtHue = HUE_DYE_HIGH;
		pLayer->SetHue(wShirtHue);
	}
	pLayer = bFemale ? LayerFind(LAYER_SKIRT) : LayerFind(LAYER_PANTS);
	if ( pLayer )
	{
		if ( wPantsHue < HUE_BLUE_LOW )
			wPantsHue = HUE_BLUE_LOW;
		if ( wPantsHue > HUE_DYE_HIGH )
			wPantsHue = HUE_DYE_HIGH;
		pLayer->SetHue(wPantsHue);
	}
	CreateNewCharCheck();
}

enum CHR_TYPE
{
	CHR_ACCOUNT,
	CHR_ACT,
	CHR_FINDLAYER,
	CHR_MEMORYFIND,
	CHR_MEMORYFINDTYPE,
	CHR_OWNER,
	CHR_REGION,
	CHR_WEAPON,
	CHR_QTY
};

LPCTSTR const CChar::sm_szRefKeys[CHR_QTY+1] =
{
	"ACCOUNT",
	"ACT",
	"FINDLAYER",
	"MEMORYFIND",
	"MEMORYFINDTYPE",
	"OWNER",
	"REGION",
	"WEAPON",
	NULL
};

bool CChar::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CChar::r_GetRef");
	int i = FindTableHeadSorted( pszKey, sm_szRefKeys, COUNTOF(sm_szRefKeys)-1 );
	if ( i >= 0 )
	{
		pszKey += strlen( sm_szRefKeys[i] );
		SKIP_SEPARATORS(pszKey);
		switch (i)
		{
			case CHR_ACCOUNT:
				if ( pszKey[-1] != '.' )	// only used as a ref !
					break;
				pRef = m_pPlayer ? m_pPlayer->m_pAccount : NULL;
				return true;
			case CHR_ACT:
				if ( pszKey[-1] != '.' )	// only used as a ref !
					break;
				pRef = m_Act_Targ.ObjFind();
				return( true );
			case CHR_FINDLAYER:	// Find equipped layers.
				pRef = LayerFind( (LAYER_TYPE) Exp_GetSingle( pszKey ));
				SKIP_SEPARATORS(pszKey);
				return( true );
			case CHR_MEMORYFINDTYPE:	// FInd a type of memory.
				pRef = Memory_FindTypes(static_cast<WORD>(Exp_GetSingle(pszKey)));
				SKIP_SEPARATORS(pszKey);
				return( true );
			case CHR_MEMORYFIND:	// Find a memory of a UID
				pRef = Memory_FindObj( (CGrayUID) Exp_GetSingle( pszKey ));
				SKIP_SEPARATORS(pszKey);
				return( true );
			case CHR_OWNER:
				pRef	= NPC_PetGetOwner();
				return( true );
			case CHR_WEAPON:
				{
					pRef	= m_uidWeapon.ObjFind();
					return( true );
				}
			case CHR_REGION:
				pRef = m_pArea;
				return( true );
		}
	}

	if ( r_GetRefContainer( pszKey, pRef ))
	{
		return( true );
	}

	return( CObjBase::r_GetRef( pszKey, pRef ));
}

enum CHC_TYPE
{
	#define ADD(a,b) CHC_##a,
	#include "../tables/CChar_props.tbl"
	#undef ADD
	CHC_QTY
};

LPCTSTR const CChar::sm_szLoadKeys[CHC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CChar_props.tbl"
	#undef ADD
	NULL
};

bool CChar::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CChar::r_WriteVal");

	if ( m_pClient && m_pClient->r_WriteVal(pszKey, sVal, pSrc) )
		return true;

	EXC_TRY("WriteVal");

	CCharBase * pCharDef = Char_GetDef();
	ASSERT(pCharDef);
	CChar * pCharSrc = pSrc->GetChar();

	CHC_TYPE iKeyNum = (CHC_TYPE) FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );
	if ( iKeyNum < 0 )
	{
do_default:
		if ( m_pPlayer )
		{
			if ( m_pPlayer->r_WriteVal( this, pszKey, sVal ))
				return( true );
		}
		if ( m_pNPC )
		{
			if ( m_pNPC->r_WriteVal( this, pszKey, sVal ))
				return( true );
		}

		if ( r_WriteValContainer(pszKey, sVal, pSrc))
		{
			return( true );
		}

		// special write values
		int i;

		// Adjusted stats
		i = g_Cfg.FindStatKey( pszKey );
		if ( i >= 0 )
		{
			sVal.FormatVal(Stat_GetAdjusted(static_cast<STAT_TYPE>(i)));
			return( true );
		}

		if ( !strnicmp( pszKey, "O", 1 ) )
		{
			i = g_Cfg.FindStatKey( pszKey+1 );
			if ( i >= 0 )
			{
				sVal.FormatVal(Stat_GetBase(static_cast<STAT_TYPE>(i)));
				return( true );
			}
		}

		if ( !strnicmp( pszKey, "MOD", 3 ) )
		{
			i = g_Cfg.FindStatKey( pszKey+3 );
			if ( i >= 0 )
			{
				sVal.FormatVal(Stat_GetMod(static_cast<STAT_TYPE>(i)));
				return( true );
			}
		}

		i = g_Cfg.FindSkillKey( pszKey );
		if ( IsSkillBase(static_cast<SKILL_TYPE>(i)))
		{
			// Check some skill name.
			WORD iVal = Skill_GetBase( static_cast<SKILL_TYPE>(i) );
			sVal.Format( "%hu.%hu", iVal/10, iVal%10 );
			return( true );
		}

		return( CObjBase::r_WriteVal( pszKey, sVal, pSrc ));
	}

	switch ( iKeyNum )
	{
		//On these ones, check BaseDef if not found on dynamic
		case CHC_FASTERCASTRECOVERY:
		case CHC_FASTERCASTING:
		case CHC_INCREASEHITCHANCE:
		case CHC_INCREASESWINGSPEED:
		case CHC_INCREASEDAM:
		case CHC_INCREASEDEFCHANCE:
		case CHC_INCREASEDEFCHANCEMAX:
		case CHC_INCREASESPELLDAM:
		case CHC_LOWERMANACOST:
		case CHC_LOWERREAGENTCOST:
		case CHC_LUCK:
		case CHC_CURFOLLOWER:
		case CHC_MAXFOLLOWER:
		case CHC_RESPHYSICAL:
		case CHC_RESPHYSICALMAX:
		case CHC_RESFIRE:
		case CHC_RESFIREMAX:
		case CHC_RESCOLD:
		case CHC_RESCOLDMAX:
		case CHC_RESPOISON:
		case CHC_RESPOISONMAX:
		case CHC_RESENERGY:
		case CHC_RESENERGYMAX:
		case CHC_REGENFOOD:
		case CHC_REGENHITS:
		case CHC_REGENSTAM:
		case CHC_REGENMANA:
		case CHC_REGENVALFOOD:
		case CHC_REGENVALHITS:
		case CHC_REGENVALSTAM:
		case CHC_REGENVALMANA:
		case CHC_SPELLTIMEOUT:
		case CHC_TITHING:
			sVal.FormatLLVal(GetDefNum(pszKey));
			break;
		case CHC_ATTACKER:
			{
				if ( strlen( pszKey ) == 8 )
				{
					sVal.FormatVal(m_lastAttackers.size());
					return true;
				}

				sVal.FormatVal(0);
				pszKey += 8;

				if ( *pszKey == '.' )
				{
					pszKey++;
					if ( !strnicmp(pszKey, "ID", 2 ) )
					{
						pszKey += 3;	// ID + whitspace
						CChar * pChar = static_cast<CChar*>(static_cast<CGrayUID>(Exp_GetSingle(pszKey)).CharFind());
						sVal.FormatVal(Attacker_GetID(pChar));
						return true;
					}else if ( !strnicmp(pszKey, "TARGET", 6 ) )
					{
						pszKey += 6;
						if ( m_Act_Targ )
							sVal.FormatHex(static_cast<DWORD>(m_Fight_Targ));
						else
							sVal.FormatVal(-1);
						return true;
					}
					if ( m_lastAttackers.size() )
					{
						size_t attackerIndex = m_lastAttackers.size();
						if( !strnicmp(pszKey, "MAX", 3) )
						{
							pszKey += 3;
							INT64 iMaxDmg = -1, iCurDmg = 0;

							for ( size_t iAttacker = 0; iAttacker < m_lastAttackers.size(); ++iAttacker )
							{
								iCurDmg = m_lastAttackers.at(iAttacker).amountDone;
								if ( iCurDmg > iMaxDmg )
								{
									iMaxDmg = iCurDmg;
									attackerIndex = iAttacker;
								}
							}
						}
						else if( !strnicmp(pszKey, "LAST", 4) )
						{
							pszKey += 4;
							INT64 dwLastTime = INT_MAX, dwCurTime = 0;

							for ( size_t iAttacker = 0; iAttacker < m_lastAttackers.size(); ++iAttacker )
							{
								dwCurTime = m_lastAttackers.at(iAttacker).elapsed;
								if ( dwCurTime <= dwLastTime )
								{
									dwLastTime = dwCurTime;
									attackerIndex = iAttacker;
								}
							}
						}
						else
						{
							attackerIndex = Exp_GetVal(pszKey);
						}

						SKIP_SEPARATORS(pszKey);
						if ( attackerIndex < m_lastAttackers.size() )
						{
							LastAttackers & refAttacker = m_lastAttackers.at(attackerIndex);

							if( !strnicmp(pszKey, "DAM", 3) )
							{
								sVal.FormatLLVal(refAttacker.amountDone);
								return true;
							}
							else if( !strnicmp(pszKey, "ELAPSED", 7) )
							{
								sVal.FormatLLVal(refAttacker.elapsed);
								return true;
							}
							else if (( !strnicmp(pszKey, "UID", 3) ) || ( *pszKey == '\0' ))
							{
								CGrayUID uid = refAttacker.charUID;
								sVal.FormatHex( uid.CharFind() ? refAttacker.charUID : 0 );
								return true;
							}
							else if ((!strnicmp(pszKey, "THREAT", 6)))
							{
								sVal.FormatLLVal(refAttacker.threat);
								return true;
							}
						}
					}
				}

				return true;
			}
		case CHC_BREATH:
			{
				if( !strnicmp(pszKey, "BREATH.DAM", 10) )
				{
					CVarDefCont * pVar = GetDefKey(pszKey, true);
					sVal.FormatLLVal(pVar ? pVar->GetValNum() : 0);
					return true;
				}
				else if ( !strnicmp(pszKey, "BREATH.HUE", 10) || !strnicmp(pszKey, "BREATH.ANIM", 11) || !strnicmp(pszKey, "BREATH.TYPE", 11))
				{
					CVarDefCont * pVar = GetDefKey(pszKey, true);
					sVal.FormatHex(pVar ? static_cast<DWORD>(pVar->GetValNum()) : 0);
					return true;
				}
				return false;
			}
		case CHC_NOTOSAVE:
			{
				if ( strlen( pszKey ) == 8 )
				{
					sVal.FormatVal(m_notoSaves.size());
					return true;
				}

				sVal.FormatVal(0);
				pszKey += 8;

				if ( *pszKey == '.' )
				{
					pszKey++;
					if ( !strnicmp(pszKey, "ID", 2 ) )
					{
						pszKey += 2;	// ID + whitspace
						CChar * pChar = static_cast<CChar*>(static_cast<CGrayUID>(Exp_GetSingle(pszKey)).CharFind());
						if ( !NotoSave_GetID(pChar) )
							sVal.FormatVal( -1 );
						else
							sVal.FormatVal(NotoSave_GetID(pChar));
						return true;
					}
					if ( m_notoSaves.size() )
					{
						size_t notoIndex = Exp_GetVal(pszKey);
						SKIP_SEPARATORS(pszKey);
						if ( notoIndex < m_notoSaves.size() )
						{
							NotoSaves & refnoto = m_notoSaves.at(notoIndex);

							if ( !strnicmp(pszKey, "VALUE", 5) )
							{
								sVal.FormatVal(refnoto.value);
								return true;
							}
							else if ( !strnicmp(pszKey, "ELAPSED", 7) )
							{
								sVal.FormatVal(static_cast<long>(refnoto.time));
								return true;
							}
							else if (( !strnicmp(pszKey, "UID", 3) ) || ( *pszKey == '\0' ))
							{
								CGrayUID uid = refnoto.charUID;
								sVal.FormatHex( uid.CharFind() ? refnoto.charUID : 0 );
								return true;
							}
							else if (!strnicmp(pszKey, "COLOR", 5))
							{
								sVal.FormatVal(refnoto.color);
								return true;
							}
							return false;
						}
					}
				}

				return true;
			}

		case CHC_FIGHTRANGE: //RANGE is now writable so this is changed to FIGHTRANGE as readable only
			sVal.FormatVal( CalcFightRange( m_uidWeapon.ItemFind() ) );
			return true;
		case CHC_BLOODCOLOR:
			sVal.FormatHex( m_wBloodHue );
			break;
		case CHC_FAME:
			// How much respect do i give this person ?
			// Fame is never negative !
			{
				if ( pszKey[4] != '.' )
					goto do_default;

				if ( g_Cfg.m_Fame.GetCount() <= 0 )
				{
					DEBUG_ERR(("FAME ranges have not been defined.\n"));
					sVal.FormatVal( 0 );
					return( true );
				}

				TCHAR * ppLevel_sep[100];
				const CGString* pFameAt0 = g_Cfg.m_Fame.GetAt(0);

				TCHAR * pszFameAt0 = new TCHAR[pFameAt0->GetLength() + 1];
				strcpylen(pszFameAt0, pFameAt0->GetPtr());

				int iFame = Stat_GetAdjusted(STAT_FAME);
				size_t i = Str_ParseCmds( pszFameAt0, ppLevel_sep, COUNTOF(ppLevel_sep), "," ) - 1; //range
				for (;;)
				{
					if ( !IsStrNumeric( ppLevel_sep[i] ) )
					{
						DEBUG_ERR(("'%s' is not a valid fame value.\n", ppLevel_sep[i]));
					}
					else if ( iFame >= ATOI(ppLevel_sep[ i ]) )
					{
						sVal = ( ! g_Cfg.m_Fame.GetAt(i + 1)->CompareNoCase( pszKey + 5 )) ? "1" : "0";
						delete[] pszFameAt0;
						return( true );
					}

					if ( i == 0 )
						break;

					i--;
				}

				sVal = 0;
				delete[] pszFameAt0;
				return( true );
			}
		case CHC_SKILLCHECK:	// odd way to get skills checking into the triggers.
			pszKey += 10;
			SKIP_SEPARATORS(pszKey);
			{
				TCHAR * ppArgs[2];
				Str_ParseCmds(const_cast<TCHAR *>(pszKey), ppArgs, COUNTOF( ppArgs ));
				SKILL_TYPE iSkill = g_Cfg.FindSkillKey( ppArgs[0] );
				if ( iSkill == SKILL_NONE )
					return( false );
				sVal.FormatVal( Skill_CheckSuccess( iSkill, Exp_GetVal( ppArgs[1] )));
			}
			return( true );
		case CHC_SKILLBEST:
			// Get the top skill.
			pszKey += 9;
			{
				unsigned int iRank = 0;
				if ( *pszKey == '.' )
				{
					SKIP_SEPARATORS(pszKey);
					iRank = Exp_GetSingle(pszKey);
				}
				sVal.FormatVal(Skill_GetBest(iRank));
			}
			return true;
		case CHC_SEX:	// <SEX milord/milady>	sep chars are :,/
			pszKey += 3;
			SKIP_SEPARATORS(pszKey);
			{
				TCHAR * ppArgs[2];
				Str_ParseCmds(const_cast<TCHAR *>(pszKey), ppArgs, COUNTOF(ppArgs), ":,/" );
				sVal = ( pCharDef->IsFemale()) ? ppArgs[1] : ppArgs[0];
			}
			return( true );
		case CHC_KARMA:
			// What do i think of this person.
			{
				if ( pszKey[5] != '.' )
					goto do_default;

				if ( g_Cfg.m_Karma.GetCount() <= 0 )
				{
					DEBUG_ERR(("KARMA ranges have not been defined.\n"));
					sVal.FormatVal( 0 );
					return( true );
				}

				TCHAR * ppLevel_sep[100];
				const CGString* pKarmaAt0 = g_Cfg.m_Karma.GetAt(0);

				TCHAR * pszKarmaAt0 = new TCHAR[pKarmaAt0->GetLength() + 1];
				strcpylen(pszKarmaAt0, pKarmaAt0->GetPtr());

				int iKarma = Stat_GetAdjusted(STAT_KARMA);

				size_t i = Str_ParseCmds( pszKarmaAt0, ppLevel_sep, COUNTOF(ppLevel_sep), "," ) - 1; //range
				for (;;)
				{
					if ( ppLevel_sep[i][0] != '-' && !IsStrNumeric( ppLevel_sep[i] ) )
					{
						DEBUG_ERR(("'%s' is not a valid karma value.\n", ppLevel_sep[i]));
					}
					else if ( iKarma >= ATOI(ppLevel_sep[ i ]) )
					{
						sVal = ( ! g_Cfg.m_Karma.GetAt(i + 1)->CompareNoCase( pszKey + 6 )) ? "1" : "0";
						delete[] pszKarmaAt0;
						return( true );
					}

					if ( i == 0 )
						break;
					i--;
				}

				sVal = 0;
				delete[] pszKarmaAt0;
				return( true );
			}
		case CHC_AR:
		case CHC_AC:
			sVal.FormatVal( m_defense + pCharDef->m_defense );
			return( true );
		case CHC_AGE:
			sVal.FormatLLVal( -( g_World.GetTimeDiff(m_timeCreate) / ( TICK_PER_SEC * 60 * 60 *24 ) )); //displayed in days
			return( true );
		case CHC_BANKBALANCE:
			sVal.FormatUVal(GetContainerCreate(LAYER_BANKBOX)->ContentCount(RESOURCE_ID(RES_TYPEDEF, IT_GOLD)));
			return true;
		case CHC_CANCAST:
			{
				pszKey += 7;
				GETNONWHITESPACE(pszKey);

				TCHAR * ppArgs[2];
				size_t iQty = Str_ParseCmds(const_cast<TCHAR *>(pszKey), ppArgs, COUNTOF( ppArgs ));

				// Check that we have at least the first argument
				if ( iQty <= 0 )
					return false;

				// Lookup the spell ID to ensure it's valid
				SPELL_TYPE spell = static_cast<SPELL_TYPE>(g_Cfg.ResourceGetIndexType( RES_SPELL, ppArgs[0] ));
				bool fCheckAntiMagic = true; // AntiMagic check is enabled by default

				// Set AntiMagic check if second argument has been provided
				if ( iQty == 2 )
					fCheckAntiMagic = ( Exp_GetVal( ppArgs[1] ) >= 1 );

				sVal.FormatVal( Spell_CanCast( spell, true, this, false, fCheckAntiMagic ) );
			}
			return true;
		case CHC_CANMAKE:
			{
				// use m_Act_Targ ?
				pszKey += 7;
				ITEMID_TYPE id = static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, pszKey ));
				sVal.FormatVal( Skill_MakeItem( id,	UID_CLEAR, SKTRIG_SELECT ) );
			}
			return true;
		case CHC_CANMAKESKILL:
			{
				pszKey += 12;
				ITEMID_TYPE id = static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, pszKey ));
				sVal.FormatVal( Skill_MakeItem( id,	UID_CLEAR, SKTRIG_SELECT, true ) );
			}
			return true;
		case CHC_SKILLUSEQUICK:
			{
				pszKey += 13;
				GETNONWHITESPACE( pszKey );

				if ( *pszKey )
				{
					TCHAR * ppArgs[2];
					size_t iQty = Str_ParseCmds(const_cast<TCHAR *>(pszKey), ppArgs, COUNTOF( ppArgs ));
					if ( iQty == 2 )
					{
						SKILL_TYPE iSkill = g_Cfg.FindSkillKey( ppArgs[0] );
						if ( iSkill == SKILL_NONE )
							return( false );

						sVal.FormatVal( Skill_UseQuick( iSkill, Exp_GetVal( ppArgs[1] )));
						return true;
					}
				}
			} return false;
		case CHC_SKILLTEST:
			{
				pszKey += 9;

				if ( *pszKey )
				{
					CResourceQtyArray Resources;
					if ( Resources.Load(pszKey) > 0 && SkillResourceTest( &Resources ) )
					{
						sVal.FormatVal(1);
						return true;
					}
				}
			}
			sVal.FormatVal(0);
			return true;
		case CHC_CANMOVE:
			{
				pszKey += 7;
				GETNONWHITESPACE(pszKey);

				CPointBase	ptDst	= GetTopPoint();
				DIR_TYPE	dir = GetDirStr(pszKey);
				ptDst.Move( dir );
				WORD		wBlockFlags	= 0;
				CRegionBase	*	pArea;
				pArea = CheckValidMove( ptDst, &wBlockFlags, dir, NULL );
				sVal.FormatHex( pArea ? pArea->GetResourceID() : 0 );
			}
			return true;

		case CHC_GOLD:
			sVal.FormatUVal(ContentCount(RESOURCE_ID(RES_TYPEDEF, IT_GOLD)));
			break;

		case CHC_MOUNT:
			{
				CChar *pChar = Horse_GetMountChar();
				if ( pChar == NULL )
					sVal.FormatVal(0);
				else
					sVal.FormatVal(pChar->GetUID());
				return true;
			}
		case CHC_MOVE:
			{
				pszKey += 4;
				GETNONWHITESPACE(pszKey);

				CPointBase	ptDst	= GetTopPoint();
				ptDst.Move( GetDirStr( pszKey ) );
				CRegionBase * pArea = ptDst.GetRegion( REGION_TYPE_MULTI | REGION_TYPE_AREA );
				if ( !pArea )
					sVal.FormatHex( ULONG_MAX );
				else
				{
					DWORD		wBlockFlags	= 0;
					g_World.GetHeightPoint2( ptDst, wBlockFlags, true );
					sVal.FormatHex( wBlockFlags );
				}
			}
			return true;
		case CHC_DISPIDDEC:	// for properties dialog.
			sVal.FormatVal( pCharDef->m_trackID );
			return true;
		case CHC_GUILDABBREV:
			{
				LPCTSTR pszAbbrev = Guild_Abbrev(MEMORY_GUILD);
				sVal = ( pszAbbrev ) ? pszAbbrev : "";
			}
			return true;
		case CHC_ID:
			sVal = g_Cfg.ResourceGetName( pCharDef->GetResourceID());
			return true;
		case CHC_ISGM:
			sVal.FormatVal( IsPriv(PRIV_GM));
			return( true );
		case CHC_ISINPARTY:
			if ( m_pPlayer != NULL )
				sVal = ( m_pParty != NULL ) ? "1" : "0";
			else
				sVal = "0";
			return( true );
		case CHC_ISMYPET:
			sVal = NPC_IsOwnedBy( pCharSrc, true ) ? "1" : "0";
			return( true );
		case CHC_ISONLINE:
			if ( m_pPlayer != NULL )
			{
				sVal = m_pClient ? "1" : "0";
				return ( true );
			}
			if ( m_pNPC != NULL )
			{
				sVal = IsDisconnected() ? "0" : "1";
				return ( true );
			}
			sVal = 0;
			return( true );
		case CHC_ISSTUCK:
			{
				CPointBase	pt = GetTopPoint();

				if ( OnFreezeCheck() )
					sVal.FormatVal(1);
				else if ( CanMoveWalkTo(pt, true, true, DIR_N) || CanMoveWalkTo(pt, true, true, DIR_E) || CanMoveWalkTo(pt, true, true, DIR_S) || CanMoveWalkTo(pt, true, true, DIR_W) )
					sVal.FormatVal(0);
				else
					sVal.FormatVal(1);
				return true;
			}
		case CHC_ISVENDOR:
			sVal.FormatVal(NPC_IsVendor() ? 1 : 0);
			return true;
		case CHC_ISVERTICALSPACE:
			{
			pszKey += 15;
			CPointMap pt;
			if ( strlen( pszKey ) )
			{
				pt = g_Cfg.GetRegionPoint(pszKey);
				if ( ! pt.IsValidPoint() )
				{
					DEBUG_ERR(("An invalid point passed as an argument to the function IsVerticalSpace.\n"));
					return false;
				}
			}
			else
				pt = GetTopPoint();
			sVal.FormatVal( IsVerticalSpace( pt, false ) );
			return true;
			}
		case CHC_MEMORY:
			// What is our memory flags about this pSrc person.
			{
				DWORD dwFlags	= 0;
				CItemMemory *	pMemory;
				pszKey += 6;
				if ( *pszKey == '.' )
				{
					pszKey++;
					CGrayUID		uid	= Exp_GetVal( pszKey );
					pMemory	= Memory_FindObj( uid );
				}
				else
					pMemory	= Memory_FindObj( pCharSrc );
				if ( pMemory != NULL )
				{
					dwFlags = pMemory->GetMemoryTypes();
				}
				sVal.FormatHex( dwFlags );
			}
			return( true );
		case CHC_NAME:
			sVal = GetName(false);
			break;
		case CHC_SKILLTOTAL:
			{
				pszKey += 10;
				SKIP_SEPARATORS(pszKey);
				GETNONWHITESPACE(pszKey);

				int		iVal	= 0;
				bool	fComp	= true;
				if ( *pszKey == '\0' )
					;
				else if ( *pszKey == '+' )
					iVal	= Exp_GetVal( ++pszKey );
				else if ( *pszKey == '-' )
					iVal	= - Exp_GetVal( ++pszKey );
				else
				{
					iVal	= Exp_GetVal( pszKey );
					fComp	= false;
				}

				sVal.FormatVal( GetSkillTotal(iVal,fComp) );
			}
			return( true );
		case CHC_SWING:
			sVal.FormatVal(m_atFight.m_Swing_State);
			break;
		case CHC_TOWNABBREV:
			{
				LPCTSTR pszAbbrev = Guild_Abbrev(MEMORY_TOWN);
				sVal = ( pszAbbrev ) ? pszAbbrev : "";
			}
			return true;
		case CHC_MAXWEIGHT:
			sVal.FormatVal( g_Cfg.Calc_MaxCarryWeight(this));
			return( true );
		case CHC_ACCOUNT:
			if ( pszKey[7] == '.' )	// used as a ref ?
			{
				if ( m_pPlayer != NULL )
				{
					pszKey += 7;
					SKIP_SEPARATORS(pszKey);

					CScriptObj * pRef = m_pPlayer->m_pAccount;
					if ( pRef )
					{
						if ( pRef->r_WriteVal( pszKey, sVal, pSrc ) )
							break;
						return ( false );
					}
				}
			}
			if ( m_pPlayer == NULL )
				sVal.Empty();
			else
				sVal = m_pPlayer->m_pAccount->GetName();
			break;
		case CHC_ACT:
			if ( pszKey[3] == '.' )	// used as a ref ?
				goto do_default;
			sVal.FormatHex( m_Act_Targ.GetObjUID());	// uid
			break;
		case CHC_ACTP:
			sVal = m_Act_p.WriteUsed();
			break;
		case CHC_ACTPRV:
			sVal.FormatHex( m_Act_TargPrv.GetObjUID());
			break;
		case CHC_ACTDIFF:
			sVal.FormatVal( m_Act_Difficulty * 10 );
			break;
		case CHC_ACTARG1:
			sVal.FormatHex( m_atUnk.m_Arg1);
			break;
		case CHC_ACTARG2:
			sVal.FormatHex( m_atUnk.m_Arg2 );
			break;
		case CHC_ACTARG3:
			sVal.FormatHex( m_atUnk.m_Arg3 );
			break;
		case CHC_ACTION:
			sVal = g_Cfg.ResourceGetName( RESOURCE_ID( RES_SKILL, Skill_GetActive()));
			break;
		case CHC_BODY:
			sVal = g_Cfg.ResourceGetName( RESOURCE_ID( RES_CHARDEF, GetDispID()));
			break;
		case CHC_CREATE:
			sVal.FormatLLVal( -( g_World.GetTimeDiff(m_timeCreate) / TICK_PER_SEC ));
			break;
		case CHC_DIR:
			{
				pszKey +=3;
				CChar * pChar = static_cast<CChar*>(static_cast<CGrayUID>(Exp_GetSingle(pszKey)).CharFind());
				if ( pChar )
					sVal.FormatVal( GetDir(pChar));
				else
					sVal.FormatVal( m_dirFace );
			}break;
		case CHC_EMOTEACT:
			sVal.FormatVal( IsStatFlag( STATF_EmoteAction ));
			break;
		case CHC_FLAGS:
			sVal.FormatLLHex( m_StatFlag );
			break;
		case CHC_FONT:
			sVal.FormatVal( m_fonttype );
			break;
		case CHC_SPEECHCOLOR:
			sVal.FormatVal( m_SpeechHue );
			break;
		case CHC_FOOD:
			sVal.FormatVal( Stat_GetVal( STAT_FOOD ) );
			break;
		case CHC_HEIGHT:
			sVal.FormatVal( GetHeight() );
			break;
		case CHC_HITPOINTS:
		case CHC_HITS:
			sVal.FormatVal( Stat_GetVal(STAT_STR) );
			break;
		case CHC_STAM:
		case CHC_STAMINA:
			sVal.FormatVal( Stat_GetVal(STAT_DEX) );
			break;
		case CHC_STEPSTEALTH:
			sVal.FormatVal( m_StepStealth );
			break;
		case CHC_MANA:
			sVal.FormatVal( Stat_GetVal(STAT_INT) );
			break;
		case CHC_MAXFOOD:
			sVal.FormatVal( Stat_GetMax( STAT_FOOD ) );
			break;
		case CHC_MAXHITS:
			sVal.FormatVal( Stat_GetMax(STAT_STR) );
			break;
		case CHC_MAXMANA:
			sVal.FormatVal( Stat_GetMax(STAT_INT) );
			break;
		case CHC_MAXSTAM:
			sVal.FormatVal( Stat_GetMax(STAT_DEX) );
			break;
		case CHC_HIT:
		case CHC_HITTRY:
			break;
		case CHC_HOME:
			sVal = m_ptHome.WriteUsed();
			break;
		case CHC_NIGHTSIGHT:
		{
			CVarDefCont *pVar = GetDefKey(pszKey, true);
			sVal.FormatLLVal(pVar ? pVar->GetValNum() : 0);
			break;
		}
		case CHC_NOTOGETFLAG:
		{
			pszKey += 11;
			GETNONWHITESPACE(pszKey);

			CGrayUID uid = Exp_GetVal(pszKey);
			SKIP_ARGSEP(pszKey);
			bool bAllowInvul = (Exp_GetVal(pszKey) >= 1);

			CChar *pChar = NULL;
			if ( !uid.IsValidUID() )
				pChar = pCharSrc;
			else
			{
				pChar = uid.CharFind();
				if ( !pChar )
					pChar = pCharSrc;
			}
			sVal.FormatVal(Noto_GetFlag(pChar, bAllowInvul));
			break;
		}

		case CHC_NPC:
			goto do_default;

		case CHC_OBODY:
			sVal = g_Cfg.ResourceGetName( RESOURCE_ID( RES_CHARDEF, m_prev_id ));
			break;

		case CHC_OSKIN:
			sVal.FormatHex( m_prev_Hue );
			break;
		case CHC_P:
			goto do_default;
		case CHC_STONE:
			sVal.FormatVal( IsStatFlag( STATF_Stone ));
			break;
		case CHC_TITLE:
			{
				if (strlen(pszKey) == 5)
				{
					sVal = m_sTitle; //GetTradeTitle
				}
				else
					sVal = GetTradeTitle();
			}break;
		case CHC_LIGHT:
			sVal.FormatHex(m_LocalLight);
			break;
		case CHC_EXP:
			sVal.FormatVal(m_exp);
			break;
		case CHC_LEVEL:
			sVal.FormatVal(m_level);
			break;
		case CHC_VIRTUALGOLD:
			sVal.FormatULLVal(m_virtualGold);
			break;
		case CHC_VISUALRANGE:
			sVal.FormatVal(GetSight());
			break;
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}



bool CChar::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CChar::r_LoadVal");
	EXC_TRY("LoadVal");
	LPCTSTR	pszKey	=  s.GetKey();
	CHC_TYPE iKeyNum = (CHC_TYPE) FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );
	if ( iKeyNum < 0 )
	{
do_default:
		if ( m_pPlayer )
		{
			if ( m_pPlayer->r_LoadVal( this, s ))
				return( true );
		}
		if ( m_pNPC )
		{
			if ( m_pNPC->r_LoadVal( this, s ))
				return( true );
		}

		{
			int i = g_Cfg.FindSkillKey( pszKey );
			if ( i != SKILL_NONE )
			{
				// Check some skill name.
				Skill_SetBase(static_cast<SKILL_TYPE>(i), static_cast<WORD>(s.GetArgVal()));
				return true;
			}

			i = g_Cfg.FindStatKey( pszKey );
			if ( i >= 0 )
			{
				int iVal = s.GetArgVal();
				if (iVal > SHRT_MAX)
					iVal = SHRT_MAX;
				else if (iVal < SHRT_MIN)
					iVal = SHRT_MIN;

				Stat_SetBase(static_cast<STAT_TYPE>(i), static_cast<short>(iVal));
				return true;
			}

			if ( !strnicmp( pszKey, "O", 1 ) )
			{
				i = g_Cfg.FindStatKey( pszKey+1 );
				if ( i >= 0 )
				{
					Stat_SetBase(static_cast<STAT_TYPE>(i), static_cast<short>(s.GetArgVal()));
					return true;
				}
			}
			if ( !strnicmp( pszKey, "MOD", 3 ) )
			{
				i = g_Cfg.FindStatKey( pszKey+3 );
				if ( i >= 0 )
				{
					Stat_SetMod(static_cast<STAT_TYPE>(i), static_cast<short>(s.GetArgVal()));
					return true;
				}
			}
		}

		return( CObjBase::r_LoadVal( s ));
	}

	switch (iKeyNum)
	{
		//Status Update Variables
		case CHC_REGENHITS:
			{
				SetDefNum(s.GetKey(), s.GetArgVal(), false);
				UpdateRegenTimers(STAT_STR, static_cast<short>(s.GetArgVal()));
			}
			break;
		case CHC_REGENSTAM:
			{
				SetDefNum(s.GetKey(), s.GetArgVal(), false);
				UpdateRegenTimers(STAT_DEX, static_cast<short>(s.GetArgVal()));
			}
			break;
		case CHC_REGENMANA:
			{
				SetDefNum(s.GetKey(), s.GetArgVal(), false);
				UpdateRegenTimers(STAT_INT, static_cast<short>(s.GetArgVal()));
			}
			break;
		case CHC_INCREASEHITCHANCE:
		case CHC_INCREASESWINGSPEED:
		case CHC_INCREASEDAM:
		case CHC_LOWERREAGENTCOST:
		case CHC_LOWERMANACOST:
		case CHC_FASTERCASTRECOVERY:
		case CHC_FASTERCASTING:
		case CHC_INCREASEDEFCHANCE:
		case CHC_INCREASEDEFCHANCEMAX:
		case CHC_INCREASESPELLDAM:
		case CHC_RESPHYSICAL:
		case CHC_RESPHYSICALMAX:
		case CHC_RESFIRE:
		case CHC_RESFIREMAX:
		case CHC_RESCOLD:
		case CHC_RESCOLDMAX:
		case CHC_RESPOISON:
		case CHC_RESPOISONMAX:
		case CHC_RESENERGY:
		case CHC_RESENERGYMAX:
		case CHC_LUCK:
		case CHC_CURFOLLOWER:
		case CHC_MAXFOLLOWER:		
		case CHC_REGENFOOD:
		case CHC_REGENVALFOOD:
		case CHC_REGENVALHITS:
		case CHC_REGENVALSTAM:
		case CHC_REGENVALMANA:
		case CHC_TITHING:
			{
				SetDefNum(s.GetKey(), s.GetArgVal(), false);
				UpdateStatsFlag();
			}
			break;
		//Set as numbers only
		case CHC_SPELLTIMEOUT:
			{
				SetDefNum(s.GetKey(), s.GetArgVal(), false);
			}
			break;
			
		case CHC_BLOODCOLOR:
			m_wBloodHue = static_cast<HUE_TYPE>(s.GetArgVal());
			break;
		case CHC_MAXFOOD:
			Stat_SetMax(STAT_FOOD, static_cast<short>(s.GetArgVal()));
			break;
		case CHC_MAXHITS:
			Stat_SetMax(STAT_STR, static_cast<short>(s.GetArgVal()));
			break;
		case CHC_MAXMANA:
			Stat_SetMax(STAT_INT, static_cast<short>(s.GetArgVal()));
			break;
		case CHC_MAXSTAM:
			Stat_SetMax(STAT_DEX, static_cast<short>(s.GetArgVal()));
			break;
		case CHC_ACCOUNT:
			return SetPlayerAccount( s.GetArgStr());
		case CHC_ACT:
			m_Act_Targ = s.GetArgVal();
			break;
		case CHC_ACTP:
			if ( ! s.HasArgs())
				m_Act_p = GetTopPoint();
			else
				m_Act_p.Read( s.GetArgStr());
			break;
		case CHC_ACTPRV:
			m_Act_TargPrv = s.GetArgVal();
			break;
		case CHC_ACTDIFF:
			m_Act_Difficulty = (s.GetArgVal() / 10);
			break;
		case CHC_ACTARG1:
			m_atUnk.m_Arg1 = s.GetArgVal();
			break;
		case CHC_ACTARG2:
			m_atUnk.m_Arg2 = s.GetArgVal();
			break;
		case CHC_ACTARG3:
			m_atUnk.m_Arg3 = s.GetArgVal();
			break;
		case CHC_ACTION:
			return Skill_Start( g_Cfg.FindSkillKey( s.GetArgStr()));
		case CHC_ATTACKER:
		{
			if ( strlen(pszKey) > 8 )
			{
				pszKey += 8;
				int attackerIndex = m_lastAttackers.size();
				if ( *pszKey == '.' )
				{
					pszKey++;
					if ( !strnicmp(pszKey, "CLEAR", 5) )
					{
						if ( m_lastAttackers.size() )
							Fight_Clear();
						return true;
					}
					else if ( !strnicmp(pszKey, "DELETE", 6) )
					{
						if ( m_lastAttackers.size() )
						{
							CChar *pChar = static_cast<CChar *>(static_cast<CGrayUID>(s.GetArgVal()).CharFind());
							Attacker_Delete(pChar, false, ATTACKER_CLEAR_SCRIPT);
						}
						return true;
					}
					else if ( !strnicmp(pszKey, "ADD", 3) )
					{
						CChar *pChar = static_cast<CChar *>(static_cast<CGrayUID>(s.GetArgVal()).CharFind());
						if ( !pChar )
							return false;
						Fight_Attack(pChar);
						return true;
					}
					else if ( !strnicmp(pszKey, "TARGET", 6) )
					{
						CChar *pChar = static_cast<CChar *>(static_cast<CGrayUID>(s.GetArgVal()).CharFind());
						if ( !pChar || pChar == this )	// can't set ourself as target
						{
							m_Fight_Targ.InitUID();
							return false;
						}
						m_Fight_Targ = pChar->GetUID();
						return true;
					}

					attackerIndex = Exp_GetVal(pszKey);
					if ( attackerIndex < 0 )
						return false;

					SKIP_SEPARATORS(pszKey);
					if ( attackerIndex < Attacker() )
					{
						CChar *pChar = Attacker_GetUID(attackerIndex);
						if ( !strnicmp(pszKey, "DAM", 3) )
						{
							Attacker_SetDam(pChar, s.GetArgVal());
							return true;
						}
						else if ( !strnicmp(pszKey, "ELAPSED", 7) )
						{
							Attacker_SetElapsed(pChar, s.GetArgVal());
							return true;
						}
						else if ( !strnicmp(pszKey, "THREAT", 6) )
						{
							Attacker_SetThreat(pChar, s.GetArgVal());
							return true;
						}
						else if ( !strnicmp(pszKey, "DELETE", 6) )
						{
							Attacker_Delete(pChar, false, ATTACKER_CLEAR_SCRIPT);
							return true;
						}
					}
				}
			}
			return false;
		}
		case CHC_BODY:
			SetID(static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType( RES_CHARDEF, s.GetArgStr())));
			break;
		case CHC_BREATH:
			{				
				if ( !strnicmp(pszKey, "BREATH.DAM", 10) || !strnicmp(pszKey, "BREATH.HUE", 10) || !strnicmp(pszKey, "BREATH.ANIM", 11) || !strnicmp(pszKey, "BREATH.TYPE", 11) )
				{
					SetDefNum(s.GetKey(), s.GetArgLLVal());
					return true;
				}
				return false;
			}break;
		case CHC_CREATE:
			m_timeCreate = CServTime::GetCurrentTime() - ( s.GetArgLLVal() * TICK_PER_SEC );
			break;
		case CHC_DIR:
			{
				DIR_TYPE dir = static_cast<DIR_TYPE>(s.GetArgVal());
				if (dir <= DIR_INVALID || dir >= DIR_QTY)
					dir = DIR_SE;
				m_dirFace = dir;
				UpdateDir( dir );
			}
			break;
		case CHC_DISMOUNT:
			Horse_UnMount();
			break;
		case CHC_EMOTEACT:
			{
				bool fSet = IsStatFlag(STATF_EmoteAction);
				if ( s.HasArgs())
				{
					fSet = s.GetArgVal() ? true : false;
				}
				else
				{
					fSet = ! fSet;
				}
				StatFlag_Mod(STATF_EmoteAction,fSet);
			}
			break;
		case CHC_FLAGS:
			if ( g_Serv.IsLoading() )
			{
				// Don't set STATF_SaveParity at server startup, otherwise the first worldsave will not save these chars
				m_StatFlag = s.GetArgLLVal() & ~STATF_SaveParity;
				break;
			}
			// Don't modify STATF_SaveParity, STATF_Pet, STATF_Spawned here
			m_StatFlag = (m_StatFlag & (STATF_SaveParity|STATF_Pet|STATF_Spawned)) | (s.GetArgLLVal() & ~(STATF_SaveParity|STATF_Pet|STATF_Spawned));
			NotoSave_Update();
			break;
		case CHC_FONT:
			m_fonttype = static_cast<FONT_TYPE>(s.GetArgVal());
			if ( m_fonttype < 0 || m_fonttype >= FONT_QTY )
				m_fonttype = FONT_NORMAL;
			break;
		case CHC_SPEECHCOLOR:
			if ( m_pPlayer )	// read-only on players
				return false;
			m_SpeechHue = static_cast<HUE_TYPE>(s.GetArgVal());
			break;
		case CHC_FOOD:
			Stat_SetVal(STAT_FOOD, static_cast<short>(s.GetArgVal()));
			break;

		case CHC_GOLD:
		{
			DWORD currentGold = ContentCount(RESOURCE_ID(RES_TYPEDEF, IT_GOLD));
			DWORD newGold = static_cast<DWORD>(s.GetArgVal());

			if ( newGold >= 0 )
			{
				if ( newGold < currentGold )
				{
					ContentConsume(RESOURCE_ID(RES_TYPEDEF, IT_GOLD), currentGold - newGold);
				}
				else if ( newGold > currentGold )
				{
					CItemContainer *pBank = GetContainerCreate(LAYER_BANKBOX);
					if ( !pBank )
						return false;
					AddGoldToPack(newGold - currentGold, pBank);
				}
				UpdateStatsFlag();
			}
			break;
		}

		case CHC_HITPOINTS:
		case CHC_HITS:
			Stat_SetVal(STAT_STR, static_cast<short>(s.GetArgVal()));
			UpdateHitsFlag();
			break;
		case CHC_MANA:
			Stat_SetVal(STAT_INT, static_cast<short>(s.GetArgVal()));
			UpdateManaFlag();
			break;
		case CHC_STAM:
		case CHC_STAMINA:
			Stat_SetVal(STAT_DEX, static_cast<short>(s.GetArgVal()));
			UpdateStamFlag();
			break;
		case CHC_STEPSTEALTH:
			m_StepStealth =  static_cast<int>(s.GetArgVal());
			break;
		case CHC_HEIGHT:
			m_height = static_cast<height_t>(s.GetArgVal());
			break;
		case CHC_HOME:
			if ( ! s.HasArgs())
				m_ptHome = GetTopPoint();
			else
				m_ptHome.Read( s.GetArgStr());
			break;
		case CHC_NAME:
			{
				if ( IsTrigUsed(TRIGGER_RENAME) )
				{
					CScriptTriggerArgs args;
					args.m_s1 = s.GetArgStr();
					args.m_pO1 = this;
					if ( OnTrigger(CTRIG_Rename, this, &args) == TRIGRET_RET_TRUE )
						return( false );

					SetName( args.m_s1 );
				}
				else
					SetName( s.GetArgStr() );
			}
			break;
		case CHC_FAME:
		case CHC_KARMA:
			goto do_default;
		case CHC_SKILLUSEQUICK:
			{

				if ( s.GetArgStr() )
				{
					TCHAR * ppArgs[2];
					size_t iQty = Str_ParseCmds(const_cast<TCHAR *>(s.GetArgStr()), ppArgs, COUNTOF( ppArgs ));
					if ( iQty == 2 )
					{
						SKILL_TYPE iSkill = g_Cfg.FindSkillKey( ppArgs[0] );
						if ( iSkill == SKILL_NONE )
							return( false );

						 Skill_UseQuick( iSkill, Exp_GetVal( ppArgs[1] ));
						return true;
					}
				}
			} return false;
		case CHC_MEMORY:
			{
				INT64 piCmd[2];
				size_t iArgQty = Str_ParseCmds(s.GetArgStr(), piCmd, COUNTOF(piCmd));
				if ( iArgQty < 2 )
					return false;

				CGrayUID uid = static_cast<CGrayUID>(piCmd[0]);
				WORD wFlags = static_cast<WORD>(piCmd[1]);

				CItemMemory *pMemory = Memory_FindObj(uid);
				if ( pMemory )
					pMemory->SetMemoryTypes(wFlags);
				else
					pMemory = Memory_AddObjTypes(uid, wFlags);
			}
			break;
		case CHC_NIGHTSIGHT:
			{
				long fNightsight = s.GetArgVal();
				if ( !fNightsight )		// keep old 'switch' from 0 to 1 and viceversa behaviour while no args are given
					fNightsight = !IsStatFlag(STATF_NightSight);

				SetDefNum(s.GetKey(), fNightsight, false);
				StatFlag_Mod(STATF_NightSight, fNightsight > 0 ? true : false);
				if ( m_pClient )
				{
					m_pClient->addLight();
					if ( fNightsight )
						m_pClient->addBuff(BI_NIGHTSIGHT, 1075643, 1075644);
					else
						m_pClient->removeBuff(BI_NIGHTSIGHT);
				}
			}
			break;
		case CHC_NPC:
			return SetNPCBrain(static_cast<NPCBRAIN_TYPE>(s.GetArgVal()));
		case CHC_OBODY:
			{
				CREID_TYPE id = static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType( RES_CHARDEF, s.GetArgStr()));
				if ( ! CCharBase::FindCharBase( id ))
				{
					DEBUG_ERR(( "OBODY Invalid Char 0%x\n", id ));
					return( false );
				}
				m_prev_id = id;
			}
			break;
		case CHC_OSKIN:
			m_prev_Hue = static_cast<HUE_TYPE>(s.GetArgVal());
			break;
		case CHC_P:
			{
				CPointMap pt;
				pt.Read(s.GetArgStr());
				MoveTo(pt);
			}
			break;
		case CHC_STONE:
			{
				bool fSet;
				bool fChange = IsStatFlag(STATF_Stone);
				if ( s.HasArgs())
				{
					fSet = s.GetArgVal() ? true : false;
					fChange = ( fSet != fChange );
				}
				else
				{
					fSet = ! fChange;
					fChange = true;
				}
				StatFlag_Mod(STATF_Stone,fSet);
				if ( fChange )
				{
					UpdateMode(NULL, true);
					if ( m_pClient )
						m_pClient->addCharMove(this);
				}
			}
			break;
		case CHC_SWING:
			{
				if ( s.GetArgVal() && ((s.GetArgVal() < WAR_SWING_INVALID) || (s.GetArgVal() > WAR_SWING_SWINGING)) )
					return false;
				m_atFight.m_Swing_State = static_cast<WAR_SWING_TYPE>(s.GetArgVal());
			}
			break;
		case CHC_TITLE:
			m_sTitle = s.GetArgStr();
			break;
		case CHC_LIGHT:
			m_LocalLight = static_cast<BYTE>(s.GetArgVal());
			break;
		case CHC_EXP:
			m_exp = s.GetArgVal();
			ChangeExperience();			//	auto-update level if applicable
			break;
		case CHC_LEVEL:
			m_level = s.GetArgVal();
			ChangeExperience();
			break;
		case CHC_VIRTUALGOLD:
			m_virtualGold = static_cast<unsigned long long>(s.GetArgLLVal());
			UpdateStatsFlag();
			break;
		case CHC_VISUALRANGE:
			SetSight(static_cast<BYTE>(s.GetArgVal()));
			break;
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

void CChar::r_Write( CScript & s )
{
	ADDTOCALLSTACK_INTENSIVE("CChar::r_Write");
	EXC_TRY("r_Write");

	s.WriteSection("WORLDCHAR %s", GetResourceName());
	s.WriteKeyVal("CREATE", -(g_World.GetTimeDiff(m_timeCreate) / TICK_PER_SEC));

	CObjBase::r_Write(s);
	if ( m_pPlayer )
		m_pPlayer->r_WriteChar(this, s);
	if ( m_pNPC )
		m_pNPC->r_WriteChar(this, s);

	if ( GetTopPoint().IsValidPoint() )
		s.WriteKey("P", GetTopPoint().WriteUsed());
	if ( !m_sTitle.IsEmpty() )
		s.WriteKey("TITLE", m_sTitle);
	if ( m_fonttype != FONT_NORMAL )
		s.WriteKeyVal("FONT", m_fonttype);
	if ( m_SpeechHue != HUE_TEXT_DEF )
		s.WriteKeyVal("SPEECHCOLOR", m_SpeechHue);
	if ( m_dirFace != DIR_SE )
		s.WriteKeyVal("DIR", m_dirFace);
	if ( m_prev_id != GetID() )
		s.WriteKey("OBODY", g_Cfg.ResourceGetName(RESOURCE_ID(RES_CHARDEF, m_prev_id)));
	if ( m_prev_Hue != HUE_DEFAULT )
		s.WriteKeyHex("OSKIN", m_prev_Hue);
	if ( m_StatFlag )
		s.WriteKeyHex("FLAGS", m_StatFlag);
	if ( m_LocalLight )
		s.WriteKeyHex("LIGHT", m_LocalLight);
	if ( m_attackBase )
		s.WriteKeyFormat("DAM", "%hu,%hu", m_attackBase, m_attackBase + m_attackRange);
	if ( m_defense )
		s.WriteKeyVal("ARMOR", m_defense);

	if ( Skill_GetActive() != SKILL_NONE )
	{
		s.WriteKey("ACTION", g_Cfg.ResourceGetName(RESOURCE_ID(RES_SKILL, Skill_GetActive())));
		if ( m_atUnk.m_Arg1 )
			s.WriteKeyHex("ACTARG1", m_atUnk.m_Arg1);
		if ( m_atUnk.m_Arg2 )
			s.WriteKeyHex("ACTARG2", m_atUnk.m_Arg2);
		if ( m_atUnk.m_Arg3 )
			s.WriteKeyHex("ACTARG3", m_atUnk.m_Arg3);
	}

	if ( m_virtualGold )
		s.WriteKeyVal("VIRTUALGOLD", m_virtualGold);

	if ( m_exp )
		s.WriteKeyVal("EXP", m_exp);
	if ( m_level )
		s.WriteKeyVal("LEVEL", m_level);

	if ( m_ModMaxWeight )
		s.WriteKeyVal("MODMAXWEIGHT", m_ModMaxWeight);
	if ( m_height )
		s.WriteKeyVal("HEIGHT", m_height);
	if ( m_ptHome.IsValidPoint() )
		s.WriteKey("HOME", m_ptHome.WriteUsed());
	if ( m_StepStealth )
		s.WriteKeyVal("STEPSTEALTH", m_StepStealth);

	TCHAR szTmp[100];
	size_t j = 0;
	for ( j = 0; j < STAT_QTY; j++ )
	{
		// this is VERY important, saving the MOD first
		if ( Stat_GetMod(static_cast<STAT_TYPE>(j)) )
		{
			sprintf(szTmp, "MOD%s", g_Stat_Name[j]);
			s.WriteKeyVal(szTmp, Stat_GetMod(static_cast<STAT_TYPE>(j)));
		}
		if ( Stat_GetBase(static_cast<STAT_TYPE>(j)) )
		{
			sprintf(szTmp, "O%s", g_Stat_Name[j]);
			s.WriteKeyVal(szTmp, Stat_GetBase(static_cast<STAT_TYPE>(j)));
		}
	}

	if ( Stat_GetAdjusted(STAT_STR) != Stat_GetMax(STAT_STR) )
		s.WriteKeyVal("MAXHITS", Stat_GetMax(STAT_STR));
	if ( Stat_GetAdjusted(STAT_DEX) != Stat_GetMax(STAT_DEX) )
		s.WriteKeyVal("MAXSTAM", Stat_GetMax(STAT_DEX));
	if ( Stat_GetAdjusted(STAT_INT) != Stat_GetMax(STAT_INT) )
		s.WriteKeyVal("MAXMANA", Stat_GetMax(STAT_INT));

	s.WriteKeyVal("HITS", Stat_GetVal(STAT_STR));
	s.WriteKeyVal("STAM", Stat_GetVal(STAT_DEX));
	s.WriteKeyVal("MANA", Stat_GetVal(STAT_INT));
	s.WriteKeyVal("FOOD", Stat_GetVal(STAT_FOOD));

	for ( j = 0; j < g_Cfg.m_iMaxSkill; j++ )
	{
		if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(static_cast<SKILL_TYPE>(j)) || Skill_GetBase(static_cast<SKILL_TYPE>(j)) == 0 )
			continue;
		s.WriteKeyVal(g_Cfg.GetSkillDef(static_cast<SKILL_TYPE>(j))->GetKey(), Skill_GetBase(static_cast<SKILL_TYPE>(j)));
	}

	r_WriteContent(s);
	EXC_CATCH;
}

void CChar::r_WriteParity( CScript & s )
{
	ADDTOCALLSTACK("CChar::r_WriteParity");
	// overload virtual for world save.

	// if ( GetPrivLevel() <= PLEVEL_Guest ) return;

	if ( g_World.m_fSaveParity == IsStatFlag(STATF_SaveParity))
	{
		return; // already saved.
	}

	StatFlag_Mod( STATF_SaveParity, g_World.m_fSaveParity );
	if ( IsWeird() )
		return;
	r_WriteSafe(s);
}

bool CChar::r_Load( CScript & s ) // Load a character from script
{
	ADDTOCALLSTACK("CChar::r_Load");
	CScriptObj::r_Load(s);

	if ( m_pNPC )
		NPC_GetAllSpellbookSpells();

	// Init the STATF_SaveParity flag.
	// StatFlag_Mod(STATF_SaveParity, g_World.m_fSaveParity);

	// Make sure everything is ok.
	if ( (m_pPlayer && !m_pClient) || (m_pNPC && IsStatFlag(STATF_Ridden)) )	// ridden npc
		SetDisconnected();

	int iResultCode = CObjBase::IsWeird();
	if ( iResultCode )
	{
		DEBUG_ERR(("Char 0%lx Invalid, id='%s', code=0%x\n", static_cast<DWORD>(GetUID()), GetResourceName(), iResultCode));
		Delete();
	}

	return true;
}

enum CHV_TYPE
{
	#define ADD(a,b) CHV_##a,
	#include "../tables/CChar_functions.tbl"
	#undef ADD
	CHV_QTY
};

LPCTSTR const CChar::sm_szVerbKeys[CHV_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CChar_functions.tbl"
	#undef ADD
	NULL
};

bool CChar::r_Verb( CScript &s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CChar::r_Verb");
	if ( !pSrc )
		return false;
	if ( m_pClient && m_pClient->r_Verb(s, pSrc) )
		return true;

	EXC_TRY("Verb");

	int index = FindTableSorted( s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys)-1 );
	if ( index < 0 )
		return ( NPC_OnVerb(s, pSrc) || Player_OnVerb(s, pSrc) || CObjBase::r_Verb(s, pSrc) );

	CChar * pCharSrc = pSrc->GetChar();

	switch ( index )
	{
		case CHV_AFK:
			// toggle ?
			{
				bool fAFK = ( Skill_GetActive() == NPCACT_Napping );
				bool fMode;
				if ( s.HasArgs())
				{
					fMode = ( s.GetArgVal() != 0 );
				}
				else
				{
					fMode = ! fAFK;
				}
				if ( fMode != fAFK )
				{
					if ( fMode )
					{
						SysMessageDefault(DEFMSG_CMDAFK_ENTER);
						m_Act_p = GetTopPoint();
						Skill_Start( NPCACT_Napping );
					}
					else
					{
						SysMessageDefault(DEFMSG_CMDAFK_LEAVE);
						Skill_Start( SKILL_NONE );
					}
				}
			}
			break;

		case CHV_ALLSKILLS:
			{
				WORD iVal = static_cast<WORD>(s.GetArgVal());
				for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; i++ )
				{
					if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(static_cast<SKILL_TYPE>(i)) )
						continue;

					Skill_SetBase(static_cast<SKILL_TYPE>(i), iVal);
				}
			}
			break;
		case CHV_ANIM:
			// ANIM, ANIM_TYPE action, bool fBackward = false, BYTE iFrameDelay = 1
			{
				INT64 Arg_piCmd[3];		// Maximum parameters in one line
				size_t Arg_Qty = Str_ParseCmds(s.GetArgRaw(), Arg_piCmd, COUNTOF(Arg_piCmd));
				return UpdateAnimate(static_cast<ANIM_TYPE>(Arg_piCmd[0]), true, false,
					(Arg_Qty > 1) ? static_cast<BYTE>(Arg_piCmd[1]) : 1,
					(Arg_Qty > 2) ? static_cast<BYTE>(Arg_piCmd[2]) : 1);
			}
			break;
		case CHV_ATTACK:
		{
			Fight_Attack(CGrayUID(s.GetArgVal()).CharFind());
			break;
		}
		case CHV_BANK:
			// Open the bank box for this person
			if ( !pCharSrc || !pCharSrc->m_pClient )
				return( false );
			pCharSrc->m_pClient->addBankOpen(this, s.HasArgs() ? static_cast<LAYER_TYPE>(s.GetArgVal()) : LAYER_BANKBOX);
			break;
		case CHV_BARK:
			SoundChar( ( s.HasArgs() ? static_cast<CRESND_TYPE>(s.GetArgVal()) : ( Calc_GetRandVal(2) ? CRESND_RAND1 : CRESND_RAND2 )));
			break;
		case CHV_BOUNCE: // uid
			return ItemBounce( CGrayUID( s.GetArgVal()).ItemFind());
		case CHV_BOW:
		{
			UpdateDir(CGrayUID(s.GetArgVal()).ObjFind());
			UpdateAnimate(ANIM_BOW);
			break;
		}
		case CHV_CONTROL: // Possess
			if ( !pCharSrc || !pCharSrc->m_pClient )
				return( false );
			return pCharSrc->m_pClient->Cmd_Control(this);

		case CHV_CONSUME:
			{
			CResourceQtyArray Resources;
			Resources.Load( s.GetArgStr() );
			ResourceConsume( &Resources, 1, false );
			}
			break;
		case CHV_CRIMINAL:
			if ( s.HasArgs() && !s.GetArgVal() )
			{
				CItem *pCriminal = LayerFind(LAYER_FLAG_Criminal);
				if ( pCriminal )
				{
					// Removing criminal memory will already clear flag, noto and buff
					pCriminal->Delete();
				}
				else
				{
					// Otherwise clear it manually if there's no memory set
					StatFlag_Clear(STATF_Criminal);
					NotoSave_Update();
					if ( m_pClient )
						m_pClient->removeBuff(BI_CRIMINALSTATUS);
				}
			}
			else
			{
				Noto_Criminal();
			}
			break;
		case CHV_DISCONNECT:
			// Push a player char off line. CLIENTLINGER thing
			if ( m_pClient )
				return m_pClient->addKick(pSrc, false);
			SetDisconnected();
			break;
		case CHV_DROP:	// uid
			return ItemDrop( CGrayUID( s.GetArgVal()).ItemFind(), GetTopPoint());
		case CHV_DUPE:	// = dupe a creature !
			{
			CChar * pChar = CreateNPC( GetID());
			pChar->MoveTo( GetTopPoint() );
			pChar->DupeFrom(this,s.GetArgVal() < 1 ? true : false);
			}break;
		case CHV_EQUIP:	// uid
			return ItemEquip( CGrayUID( s.GetArgVal()).ItemFind());
		case CHV_EQUIPHALO:
			{
				// equip a halo light
				CItem * pItem = CItem::CreateScript(ITEMID_LIGHT_SRC, this);
				ASSERT( pItem);
				if ( s.HasArgs())	// how long to last ?
				{
					INT64 iTimer = s.GetArgLLVal();
					if ( iTimer > 0 )
						pItem->SetTimeout(iTimer);

					pItem->Item_GetDef()->m_ttNormal.m_tData4 = 0;
				}
				pItem->SetAttr(ATTR_MOVE_NEVER);
				LayerAdd(pItem, LAYER_HAND2);
			}
			return( true );
		case CHV_EQUIPARMOR:
			return ItemEquipArmor(false);
		case CHV_EQUIPWEAPON:
			// find my best waepon for my skill and equip it.
			return ItemEquipWeapon(false);
		case CHV_FACE:
		{
			UpdateDir(CGrayUID(s.GetArgVal()).ObjFind());
			break;
		}
		case CHV_FIXWEIGHT:
			FixWeight();
			break;
		case CHV_FORGIVE:
			Jail( pSrc, false, 0 );
			break;
		case CHV_GOCHAR:	// uid
			return TeleportToObj( 1, s.GetArgStr());
		case CHV_GOCHARID:
			return TeleportToObj( 3, s.GetArgStr());
		case CHV_GOCLI:	// enum clients
			return TeleportToCli( 1, s.GetArgVal());
		case CHV_GOITEMID:
			return TeleportToObj( 4, s.GetArgStr());
		case CHV_GONAME:
			return TeleportToObj( 0, s.GetArgStr());
		case CHV_GO:
			return Spell_Teleport( g_Cfg.GetRegionPoint(s.GetArgStr()), true, false );
		case CHV_GOSOCK:	// sockid
			return TeleportToCli( 0, s.GetArgVal());
		case CHV_GOTYPE:
			return TeleportToObj( 2, s.GetArgStr());
		case CHV_GOUID:	// uid
			if ( s.HasArgs())
			{
				CGrayUID uid( s.GetArgVal());
				CObjBaseTemplate * pObj = uid.ObjFind();
				if ( pObj == NULL )
					return( false );
				pObj = pObj->GetTopLevelObj();
				Spell_Teleport( pObj->GetTopPoint(), true, false );
				return( true );
			}
			return( false );
		case CHV_HEAR:
			// NPC will hear this command but no-one else.
			if ( m_pPlayer )
				SysMessage(s.GetArgStr());
			else
				NPC_OnHear(s.GetArgStr(), pSrc->GetChar());
			break;
		case CHV_HUNGRY:	// How hungry are we ?
			if ( pCharSrc )
			{
				char *z = Str_GetTemp();
				if ( pCharSrc == this )
					sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_SELF), Food_GetLevelMessage( false, false ));
				else
					sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_OTHER), GetName(), Food_GetLevelMessage( false, false ));
				pCharSrc->ObjMessage(z, this);
			}
			break;
		case CHV_INVIS:
			if ( pSrc )
			{
				m_StatFlag = s.GetArgFlag( m_StatFlag, STATF_Insubstantial );
				UpdateMode(NULL, true);
				if ( IsStatFlag(STATF_Insubstantial) )
				{
					if ( m_pClient )
						m_pClient->addBuff(BI_HIDDEN, 1075655, 1075656);
					if ( IsSetOF(OF_Command_Sysmsgs) )
						pSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_INVIS_ON));
				}
				else
				{
					if ( m_pClient && !IsStatFlag(STATF_Hidden) )
						m_pClient->removeBuff(BI_HIDDEN);
					if ( IsSetOF(OF_Command_Sysmsgs) )
						pSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_INVIS_OFF));
				}
			}
			break;
		case CHV_INVUL:
			if ( pSrc )
			{
				m_StatFlag = s.GetArgFlag( m_StatFlag, STATF_INVUL );
				NotoSave_Update();
				if ( IsSetOF( OF_Command_Sysmsgs ) )
					pSrc->SysMessage( IsStatFlag( STATF_INVUL )? g_Cfg.GetDefaultMsg(DEFMSG_MSG_INVUL_ON) : g_Cfg.GetDefaultMsg(DEFMSG_MSG_INVUL_OFF) );
			}
			break;
		case CHV_JAIL:	// i am being jailed
			Jail( pSrc, true, s.GetArgVal() );
			break;
		case CHV_KILL:
			{
				Effect( EFFECT_LIGHTNING, ITEMID_NOTHING, pCharSrc );
				OnTakeDamage( 10000, pCharSrc, DAMAGE_GOD );
				Stat_SetVal( STAT_STR, 0 );
				g_Log.Event( LOGL_EVENT|LOGM_KILLS|LOGM_GM_CMDS, "'%s' was KILLed by '%s'\n", GetName(), pSrc->GetName());
			}
			break;
		case CHV_MAKEITEM:
		{
			TCHAR *psTmp = Str_GetTemp();
			strcpy( psTmp, s.GetArgRaw() );
			GETNONWHITESPACE( psTmp );
			TCHAR * ttVal[2];
			DWORD iReplicationQty = 1;
			size_t iArg = Str_ParseCmds( psTmp, ttVal, COUNTOF( ttVal ), " ,\t" );
			if ( iArg == 2 )
			{
				if ( IsDigit(ttVal[1][0]) )
					iReplicationQty = static_cast<DWORD>(ATOI(ttVal[1]));
			}
 
			if ( m_pClient )
				m_Act_Targ = m_pClient->m_Targ_UID;

			return Skill_MakeItem(static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType(RES_ITEMDEF, ttVal[0])), m_Act_Targ, SKTRIG_START, false, iReplicationQty);
		}

		case CHV_MOUNT:
			{
				CChar * pChar = dynamic_cast <CChar*> (pSrc);
				if ( pChar )
					pChar->Horse_Mount(this);
			}
			break;

		case CHV_NEWBIESKILL:
			{
				CResourceLock t;
				if ( !g_Cfg.ResourceLock( t, RES_NEWBIE, s.GetArgStr()) )
					return false;
				ReadScript(t);
			}
			break;

		case CHV_NEWGOLD:
			{
				CItemContainer *pPack = GetContainerCreate(LAYER_PACK);
				CItem *pGold = NULL;
				WORD iGoldStack = 0;
				DWORD iAmount = s.GetArgLLVal();
				while ( iAmount > 0 )
				{
					iGoldStack = minimum(iAmount, g_Cfg.m_iItemsMaxAmount);
					pGold = CItem::CreateScript(ITEMID_GOLD_C1, this);
					pGold->SetAmount(iGoldStack);
					pPack->ContentAdd(pGold);
					iAmount -= iGoldStack;
				}
				UpdateStatsFlag();
			} break;

		case CHV_NEWLOOT:
			{
				if ( m_pNPC && !m_pPlayer && !IsStatFlag(STATF_Conjured) )
				{
					CItem *pItem = CItem::CreateHeader(s.GetArgStr(), NULL, false, this);
					if ( !pItem )
						g_World.m_uidNew = static_cast<CGrayUID>(UID_CLEAR);
					else
					{
						ItemEquip(pItem);
						g_World.m_uidNew = pItem->GetUID();
					}
				}
			} break;
		case CHV_NOTOCLEAR:
			NotoSave_Clear();
			break;
		case CHV_NOTOUPDATE:
			NotoSave_Update();
			break;

		case CHV_PACK:
			if ( !pCharSrc || !pCharSrc->m_pClient )
				return( false );
			pCharSrc->m_pClient->addBankOpen( this, LAYER_PACK ); // Send Backpack (with items)
			break;

		case CHV_POISON:
		{		
			int iSkill = s.GetArgVal();
			int iTicks = iSkill / 50; 
			INT64		piCmd[2];
			if (Str_ParseCmds(s.GetArgRaw(), piCmd, COUNTOF(piCmd)) > 1)
				iTicks = static_cast<int>(piCmd[1]);

			SetPoison(iSkill, iTicks, pSrc->GetChar());
		}
		break;

		case CHV_POLY:	// result of poly spell script choice. (casting a spell)
		{
			const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(SPELL_Polymorph);
			if ( !pSpellDef )
				return false;

			m_atMagery.m_Spell = SPELL_Polymorph;
			m_atMagery.m_SummonID = static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType(RES_CHARDEF, s.GetArgStr()));
			m_Act_Targ = GetUID();
			m_Act_TargPrv = GetUID();

			if ( m_pClient && IsSetMagicFlags(MAGICF_PRECAST) && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST) )
			{
				Spell_CastDone();
				break;
			}

			int skill;
			if ( !pSpellDef->GetPrimarySkill(&skill, NULL) )
				return false;

			Skill_Start(static_cast<SKILL_TYPE>(skill));
			break;
		}
		case CHV_PRIVSET:
			return( SetPrivLevel( pSrc, s.GetArgStr()));
		case CHV_RELEASE:
			Skill_Start( SKILL_NONE );
			NPC_PetClearOwners();
			SoundChar( CRESND_RAND2 );	// No noise
			return( true );
		case CHV_REMOVE:	// remove this char from the world instantly.
			if ( m_pPlayer )
			{
				if ( s.GetArgRaw()[0] != '1' || pSrc->GetPrivLevel() < PLEVEL_Admin )
				{
					pSrc->SysMessage( g_Cfg.GetDefaultMsg(DEFMSG_CMD_REMOVE_PLAYER) );
					return( false );
				}
			}
			Delete();
			break;
		case CHV_DESTROY:	// remove this char from the world and bypass trigger's return value.
			if ( m_pPlayer )
			{
				if ( s.GetArgRaw()[0] != '1' || pSrc->GetPrivLevel() < PLEVEL_Admin )
				{
					pSrc->SysMessage( g_Cfg.GetDefaultMsg(DEFMSG_CMD_REMOVE_PLAYER) );
					return( false );
				}
			}
			Delete(true);
			break;
		case CHV_RESURRECT:
			{
				if ( !s.GetArgVal() )
					return OnSpellEffect( SPELL_Resurrection, pCharSrc, 1000, NULL );
				else
					return Spell_Resurrection( NULL, pCharSrc, true );
			}
		case CHV_REVEAL:
			Reveal(static_cast<DWORD>(s.GetArgVal()));
			break;
		case CHV_SALUTE:	//	salute to player
		{
			UpdateDir(CGrayUID(s.GetArgVal()).ObjFind());
			UpdateAnimate(ANIM_SALUTE);
			break;
		}
		case CHV_SKILL:
			Skill_Start( g_Cfg.FindSkillKey( s.GetArgStr()));
			break;
		case CHV_SKILLGAIN:
			{
				if ( s.HasArgs() )
				{
					TCHAR * ppArgs[2];
					if ( Str_ParseCmds( s.GetArgRaw(), ppArgs, COUNTOF( ppArgs )) > 0 )
					{
						SKILL_TYPE iSkill = g_Cfg.FindSkillKey( ppArgs[0] );
						if ( iSkill == SKILL_NONE )
							return( false );
						Skill_Experience( iSkill, Exp_GetVal( ppArgs[1] ));
					}
				}
			}
			return( true );
		case CHV_SLEEP:
			SleepStart( s.GetArgVal() != 0 );
			break;
		case CHV_SUICIDE:
			Memory_ClearTypes( MEMORY_FIGHT ); // Clear the list of people who get credit for your death
			UpdateAnimate( ANIM_SALUTE );
			Stat_SetVal( STAT_STR, 0 );
			break;
		case CHV_SUMMONCAGE: // i just got summoned
			if ( pCharSrc != NULL )
			{
				// Let's make a cage to put the player in
				ITEMID_TYPE id = static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, "i_multi_cage" ));
				if ( id < 0 )
					return( false );
				CItemMulti * pItem = dynamic_cast <CItemMulti*>( CItem::CreateBase( id ));
				if ( pItem == NULL )
					return( false );
				CPointMap pt = pCharSrc->GetTopPoint();
				pt.MoveN( pCharSrc->m_dirFace, 3 );
				pItem->MoveToDecay( pt, 10*60*TICK_PER_SEC );	// make the cage vanish after 10 minutes.
				pItem->Multi_Create( NULL, UID_CLEAR );
				Spell_Teleport( pt, true, false );
				break;
			}
			return( false );
		case CHV_SUMMONTO:	// i just got summoned
			if ( pCharSrc != NULL )
			{
				Spell_Teleport( pCharSrc->GetTopPoint(), true, false );
			}
			break;
		case CHV_SMSG:
		case CHV_SMSGL:
		case CHV_SMSGLEX:
		case CHV_SMSGU:
		case CHV_SYSMESSAGE:
		case CHV_SYSMESSAGELOC:
		case CHV_SYSMESSAGELOCEX:
		case CHV_SYSMESSAGEUA:
			// just eat this if it's not a client.
			break;
		case CHV_TARGETCLOSE:
			if ( m_pClient )
				m_pClient->addTargetCancel();
			break;
		case CHV_UNDERWEAR:
			if ( ! IsPlayableCharacter())
				return( false );
			SetHue( GetHue() ^ HUE_MASK_UNDERWEAR /*, false, pSrc*/ ); //call @Dye on underwear?
			UpdateMode();
			break;
		case CHV_UNEQUIP:	// uid
			return ItemBounce( CGrayUID( s.GetArgVal()).ItemFind());
		case CHV_WHERE:
			if ( pCharSrc )
			{
				char *z = Str_GetTemp();
				if ( m_pArea )
				{
					if ( m_pArea->GetResourceID().IsItem())
					{
						sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_MSG_WHERE_AREA), m_pArea->GetName(), GetTopPoint().WriteUsed());
					}
					else
					{
						const CRegionBase * pRoom = GetTopPoint().GetRegion( REGION_TYPE_ROOM );
						if ( ! pRoom )
						{
							sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_MSG_WHERE_AREA), m_pArea->GetName(), GetTopPoint().WriteUsed());
						} else
						{
							sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_MSG_WHERE_ROOM), m_pArea->GetName(), pRoom->GetName(), GetTopPoint().WriteUsed());
						}
					}
				}
				else
					sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_MSG_WHERE), GetTopPoint().WriteUsed());
				pCharSrc->ObjMessage(z, this);
			}
			break;
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

bool CChar::OnTriggerSpeech( bool bIsPet, LPCTSTR pszText, CChar * pSrc, TALKMODE_TYPE & mode, HUE_TYPE wHue)
{
	ADDTOCALLSTACK("CChar::OnTriggerSpeech");

	LPCTSTR pszName;
	
	if ( bIsPet && !g_Cfg.m_sSpeechPet.IsEmpty() )
	{
		pszName = static_cast<LPCTSTR>(g_Cfg.m_sSpeechPet);
	}
	else if ( !bIsPet && !g_Cfg.m_sSpeechSelf.IsEmpty() )
	{
		pszName = static_cast<LPCTSTR>(g_Cfg.m_sSpeechSelf);
	}
	else
	{
		goto lbl_cchar_ontriggerspeech;
	}

	{
		CScriptObj * pDef = g_Cfg.ResourceGetDefByName( RES_SPEECH, pszName );
		if ( pDef )
		{
			CResourceLink * pLink	= dynamic_cast <CResourceLink *>( pDef );
			if ( pLink )
			{
				CResourceLock	s;
				if ( pLink->ResourceLock(s) && pLink->HasTrigger(XTRIG_UNKNOWN) )
				{
					TRIGRET_TYPE iRet = OnHearTrigger(s, pszText, pSrc, mode, wHue);
					if ( iRet == TRIGRET_RET_TRUE )
						return true;
					else if ( iRet == TRIGRET_RET_HALFBAKED )
						return false;
				}
				else
				{
					DEBUG_ERR(("TriggerSpeech: couldn't run script for speech %s\n", pszName));
				}
			}
			else
			{
				DEBUG_ERR(("TriggerSpeech: couldn't find speech %s\n", pszName));
			}
		}
		else
		{
			DEBUG_ERR(("TriggerSpeech: couldn't find speech resource %s\n", pszName));
		}
	}


lbl_cchar_ontriggerspeech:
	if ( bIsPet )
		return false;

	if ( !m_pPlayer )
		return false;

	if ( m_pPlayer->m_Speech.GetCount() > 0 )
	{
		for ( size_t i = 0; i < m_pPlayer->m_Speech.GetCount(); i++ )
		{
			CResourceLink * pLinkDSpeech = m_pPlayer->m_Speech[i];
			if ( !pLinkDSpeech )
				continue;

			CResourceLock sDSpeech;
			if ( !pLinkDSpeech->ResourceLock(sDSpeech) )
				continue;

			TRIGRET_TYPE iRet = OnHearTrigger( sDSpeech, pszText, pSrc, mode, wHue );
			if ( iRet == TRIGRET_RET_TRUE )
				return true;
			else if ( iRet == TRIGRET_RET_HALFBAKED )
				break;
		}
	}

	return false;
}

// Gaining exp
unsigned int Calc_ExpGet_Exp(unsigned int level)
{
	unsigned int exp = 0;
	for ( unsigned int lev = 1; lev <= level; lev++ )
	{
		switch ( g_Cfg.m_iLevelMode )
		{
			case LEVEL_MODE_LINEAR:
				exp += g_Cfg.m_iLevelNextAt;
				break;
			case LEVEL_MODE_DOUBLE:
			default:
				exp += (g_Cfg.m_iLevelNextAt * (lev + 1));
				break;
		}
	}
	return exp;
}

// Increasing level
unsigned int Calc_ExpGet_Level(unsigned int exp)
{
	unsigned int level = 0; // current level
	unsigned int req = g_Cfg.m_iLevelNextAt; // required xp for next level

	if (req < 1)	//Must do this check in case ini's LevelNextAt is not set or server will freeze because exp will never decrease in the while.
		return 0;

	while (exp >= req)
	{
		// reduce xp and raise level
		exp -= req;
		level++;

		// calculate requirement for next level
		switch ( g_Cfg.m_iLevelMode )
		{
			case LEVEL_MODE_LINEAR:
				break;
			case LEVEL_MODE_DOUBLE:
			default:
				req += g_Cfg.m_iLevelNextAt;
				break;
		}
	}

	return level;
}

void CChar::ChangeExperience(int delta, CChar *pCharDead)
{
	ADDTOCALLSTACK("CChar::ChangeExperience");

	if (!g_Cfg.m_iExperienceMode)
		return;

	static UINT const keyWords[] =
	{
		DEFMSG_MSG_EXP_CHANGE_1,		// 0
		DEFMSG_MSG_EXP_CHANGE_2,
		DEFMSG_MSG_EXP_CHANGE_3,
		DEFMSG_MSG_EXP_CHANGE_4,
		DEFMSG_MSG_EXP_CHANGE_5,
		DEFMSG_MSG_EXP_CHANGE_6,		// 5
		DEFMSG_MSG_EXP_CHANGE_7,
		DEFMSG_MSG_EXP_CHANGE_8
	};

	if (delta != 0 || pCharDead)//	zero call will sync the exp level
	{
		if (delta < 0)
		{
			if (!(g_Cfg.m_iExperienceMode&EXP_MODE_ALLOW_DOWN))	// do not allow changes to minus
				return;
			// limiting delta to current level? check if delta goes out of level
			if (g_Cfg.m_bLevelSystem && g_Cfg.m_iExperienceMode&EXP_MODE_DOWN_NOLEVEL)
			{
				unsigned int exp = Calc_ExpGet_Exp(m_level);
				if (m_exp + delta < exp)
					delta = m_exp - exp;
			}
		}

		if (g_Cfg.m_wDebugFlags&DEBUGF_EXP)
		{
			DEBUG_ERR(("%s %s experience change (was %u, delta %d, now %u)\n",
				(m_pNPC ? "NPC" : "Player"), GetName(), m_exp, delta, m_exp + delta));
		}

		bool bShowMsg = (m_pClient != NULL);

		if (IsTrigUsed(TRIGGER_EXPCHANGE))
		{
			CScriptTriggerArgs	args(delta, bShowMsg);
			args.m_pO1 = pCharDead;
			if (OnTrigger(CTRIG_ExpChange, this, &args) == TRIGRET_RET_TRUE)
				return;
			delta = static_cast<int>(args.m_iN1);
			bShowMsg = (args.m_iN2 != 0);
		}

		// Do not allow an underflow due to negative Exp Change.
		if( delta < 0 && static_cast<int>(m_exp) < abs(delta) )
			m_exp = 0;
		else
			m_exp += delta;
		
		if (m_pClient && bShowMsg && delta)
		{
			int iWord = 0;
			int absval = abs(delta);
			int maxval = (g_Cfg.m_bLevelSystem && g_Cfg.m_iLevelNextAt) ? maximum(g_Cfg.m_iLevelNextAt, 1000) : 1000;

			if (absval >= maxval)				// 100%
				iWord = 7;
			else if (absval >= (maxval * 2) / 3)	//  66%
				iWord = 6;
			else if (absval >= maxval / 2)		//  50%
				iWord = 5;
			else if (absval >= maxval / 3)		//  33%
				iWord = 4;
			else if (absval >= maxval / 5)		//  20%
				iWord = 3;
			else if (absval >= maxval / 7)		//  14%
				iWord = 2;
			else if (absval >= maxval / 14)		//   7%
				iWord = 1;

			m_pClient->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_CHANGE_0),
				(delta > 0) ? g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_CHANGE_GAIN) : g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_CHANGE_LOST),
				g_Cfg.GetDefaultMsg(keyWords[iWord]));
		}
	}

	if (g_Cfg.m_bLevelSystem)
	{
		unsigned int level = Calc_ExpGet_Level(m_exp);

		if (level != m_level)
		{
			delta = level - m_level;

			bool bShowMsg = (m_pClient != NULL);

			if (IsTrigUsed(TRIGGER_EXPLEVELCHANGE))
			{
				CScriptTriggerArgs	args(delta);
				if (OnTrigger(CTRIG_ExpLevelChange, this, &args) == TRIGRET_RET_TRUE)
					return;
				delta = static_cast<int>(args.m_iN1);
				bShowMsg = (args.m_iN2 != 0);
			}

			level = m_level + delta;
			// Prevent integer underflow due to negative level change
			if( delta < 0 && abs(delta) > static_cast<int>(m_level) )
				level = 0;
			if (g_Cfg.m_wDebugFlags&DEBUGF_LEVEL)
			{
				DEBUG_ERR(("%s %s level change (was %u, delta %d, now %u)\n",
					(m_pNPC ? "NPC" : "Player"), GetName(), m_level, delta, level));
			}
			m_level = level;

			if (m_pClient && bShowMsg)
			{
				m_pClient->SysMessagef((abs(delta) == 1) ? g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_LVLCHANGE_0) : g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_LVLCHANGE_1),
					(delta > 0) ? g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_LVLCHANGE_GAIN) : g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_LVLCHANGE_LOST));
			}
		}
	}
}

// returns <SkillTotal>
WORD CChar::GetSkillTotal(int what, bool how)
{
	ADDTOCALLSTACK("CChar::GetSkillTotal");
	int iTotal = 0;
	int	iBase;

	for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; i++ )
	{
		iBase = Skill_GetBase(static_cast<SKILL_TYPE>(i));
		if ( how )
		{
			if ( what < 0 )
			{
				if ( iBase >= -what )
				continue;
			}
			else if ( iBase < what )
				continue;
		}
		else
		{
			// check group flags
			const CSkillDef * pSkill = g_Cfg.GetSkillDef(static_cast<SKILL_TYPE>(i));
			if ( !pSkill )
				continue;
			if ( !( pSkill->m_dwGroup & what ) )
				continue;
		}
		iTotal += iBase;
	}

	return iTotal;
}

int CChar::GetAbilityFlags() const
{
	CCharBase * pCharBase = Char_GetDef();
	return pCharBase->m_Can;
}
