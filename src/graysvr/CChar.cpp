//  CChar is either an NPC or a Player.
#include "graysvr.h"	// predef header.
#include "../network/network.h"

const LPCTSTR CChar::sm_szTrigName[CTRIG_QTY + 1] =	// static
{
	"@AAAUNUSED",
	"@AfterClick",
	"@Attack",
	"@CallGuards",
	"@charAttack",
	"@charClick",
	"@charClientTooltip",
	"@charContextMenuRequest",
	"@charContextMenuSelect",
	"@charDClick",
	"@charTradeAccepted",
	"@Click",
	"@ClientTooltip",
	"@CombatAdd",
	"@CombatDelete",
	"@CombatEnd",
	"@CombatStart",
	"@ContextMenuRequest",
	"@ContextMenuSelect",
	"@Create",
	"@Criminal",
	"@DClick",
	"@Death",
	"@DeathCorpse",
	"@Destroy",
	"@Dismount",
	"@Eat",
	"@EffectAdd",
	"@EnvironChange",
	"@ExpChange",
	"@ExpLevelChange",
	"@FameChange",
	"@FollowersUpdate",
	"@GetHit",
	"@Hit",
	"@HitCheck",
	"@HitIgnore",
	"@HitMiss",
	"@HitTry",
	"@HouseDesignCommit",
	"@HouseDesignExit",
	"@itemAfterClick",
	"@itemBuy",
	"@itemClick",
	"@itemClientTooltip",
	"@itemContextMenuRequest",
	"@itemContextMenuSelect",
	"@itemCreate",
	"@itemDamage",
	"@itemDCLICK",
	"@itemDestroy",
	"@itemDropOn_Char",
	"@itemDropOn_Ground",
	"@itemDropOn_Item",
	"@itemDropOn_Self",
	"@ItemDropOn_Trade",
	"@itemEQUIP",
	"@itemEQUIPTEST",
	"@itemMemoryEquip",
	"@itemPICKUP_GROUND",
	"@itemPICKUP_PACK",
	"@itemPICKUP_SELF",
	"@itemPICKUP_STACK",
	"@itemSELL",
	"@itemSPELL",
	"@itemSTEP",
	"@itemTARGON_CANCEL",
	"@itemTARGON_CHAR",
	"@itemTARGON_GROUND",
	"@itemTARGON_ITEM",
	"@itemTimer",
	"@itemToolTip",
	"@itemUNEQUIP",
	"@Jailed",
	"@KarmaChange",
	"@Kill",
	"@LogIn",
	"@LogOut",
	"@Mount",
	"@MurderDecay",
	"@MurderMark",
	"@NotoSend",
	"@NPCAcceptItem",
	"@NPCActFight",
	"@NPCActFollow",
	"@NPCAction",
	"@NPCHearGreeting",
	"@NPCHearUnknown",
	"@NPCLookAtChar",
	"@NPCLookAtItem",
	"@NPCLostTeleport",
	"@NPCRefuseItem",
	"@NPCRestock",
	"@NPCSeeNewPlayer",
	"@NPCSeeWantItem",
	"@NPCSpecialAction",
	"@PartyDisband",
	"@PartyInvite",
	"@PartyLeave",
	"@PartyRemove",
	"@PersonalSpace",
	"@PetDesert",
	"@Profile",
	"@ReceiveItem",
	"@RegenStat",
	"@RegionEnter",
	"@RegionLeave",
	"@RegionResourceFound",
	"@RegionResourceGather",
	"@Rename",
	"@Resurrect",
	"@SeeCrime",
	"@SeeHidden",
	"@SeeSnoop",
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
	"@SpellCast",
	"@SpellEffect",
	"@SpellFail",
	"@SpellSelect",
	"@SpellSuccess",
	"@SpellTargetCancel",
	"@StatChange",
	"@StepStealth",
	"@Targon_Cancel",
	"@ToggleFlying",
	"@ToolTip",
	"@TradeAccepted",
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
	"@UserUltimaStoreButton",
	"@UserVirtue",
	"@UserVirtueInvoke",
	"@UserWarmode",
	 NULL
};


/////////////////////////////////////////////////////////////////
// -CChar

// Create the "basic" char (not NPC or player yet)
// NOTE: NEVER return NULL
CChar *CChar::CreateBasic(CREID_TYPE id)	// static
{
	ADDTOCALLSTACK("CChar::CreateBasic");
	return new CChar(id);
}

// Create an NPC
// NOTE: NEVER return NULL
CChar *CChar::CreateNPC(CREID_TYPE id)	// static
{
	ADDTOCALLSTACK("CChar::CreateNPC");
	CChar *pChar = CreateBasic(id);
	ASSERT(pChar);

	pChar->NPC_LoadScript(true);
	pChar->NPC_CreateTrigger();
	return pChar;
}

CChar::CChar(CREID_TYPE id) : CObjBase(false)
{
	g_Serv.StatInc(SERV_STAT_CHARS);	// increase chars created count

	m_pClient = NULL;
	m_pPlayer = NULL;
	m_pNPC = NULL;
	m_pArea = NULL;
	m_pRoom = NULL;
	m_pParty = NULL;
	m_StatFlag = 0;

	if ( g_World.m_fSaveParity )
		StatFlag_Set(STATF_SaveParity);		// it will get saved next time

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

	m_timeCreate = m_timeLastRegen = m_timeLastHitsUpdate = m_timeLastCallGuards = CServTime::GetCurrentTime();

	m_prev_Hue = HUE_DEFAULT;
	m_prev_id = CREID_INVALID;
	SetID(id);
	CCharBase *pCharDef = Char_GetDef();
	ASSERT(pCharDef);
	m_attackBase = pCharDef->m_attackBase;
	m_attackRange = pCharDef->m_attackRange;
	m_defenseBase = pCharDef->m_defenseBase;
	m_defenseRange = pCharDef->m_defenseRange;

	m_DamPhysical = pCharDef->m_DamPhysical;
	m_DamFire = pCharDef->m_DamFire;
	m_DamCold = pCharDef->m_DamCold;
	m_DamPoison = pCharDef->m_DamPoison;
	m_DamEnergy = pCharDef->m_DamEnergy;

	m_ResPhysical = pCharDef->m_ResPhysical;
	m_ResPhysicalMax = pCharDef->m_ResPhysicalMax;
	m_ResFire = pCharDef->m_ResFire;
	m_ResFireMax = pCharDef->m_ResFireMax;
	m_ResCold = pCharDef->m_ResCold;
	m_ResColdMax = pCharDef->m_ResColdMax;
	m_ResPoison = pCharDef->m_ResPoison;
	m_ResPoisonMax = pCharDef->m_ResPoisonMax;
	m_ResEnergy = pCharDef->m_ResEnergy;
	m_ResEnergyMax = pCharDef->m_ResEnergyMax;

	m_Luck = pCharDef->m_Luck;
	m_DamIncrease = pCharDef->m_DamIncrease;
	m_SpellDamIncrease = pCharDef->m_SpellDamIncrease;
	m_HitLifeLeech = pCharDef->m_HitLifeLeech;
	m_HitManaDrain = pCharDef->m_HitManaDrain;
	m_HitManaLeech = pCharDef->m_HitManaLeech;
	m_HitStaminaLeech = pCharDef->m_HitStaminaLeech;
	m_HitChanceIncrease = pCharDef->m_HitChanceIncrease;
	m_DefChanceIncrease = pCharDef->m_DefChanceIncrease;
	m_DefChanceIncreaseMax = pCharDef->m_DefChanceIncreaseMax;
	m_SwingSpeedIncrease = pCharDef->m_SwingSpeedIncrease;
	m_FasterCasting = pCharDef->m_FasterCasting;
	m_FasterCastRecovery = pCharDef->m_FasterCastRecovery;
	m_LowerManaCost = pCharDef->m_LowerManaCost;
	m_LowerReagentCost = pCharDef->m_LowerReagentCost;
	m_EnhancePotions = pCharDef->m_EnhancePotions;
	m_NightSight = pCharDef->m_NightSight;
	m_ReflectPhysicalDamage = pCharDef->m_ReflectPhysicalDamage;

	m_Can = pCharDef->m_Can;
	m_wBloodHue = pCharDef->m_wBloodHue;

	m_FollowerSlots = pCharDef->m_FollowerSlots;
	m_FollowerCur = 0;
	m_FollowerMax = pCharDef->m_FollowerMax;
	m_Tithing = 0;

	SetName(pCharDef->GetName());	// set the name in case there is a name template

	Skill_Cleanup();

	g_World.m_uidLastNewChar = GetUID();	// for script access

	for ( size_t i = 0; i < STAT_QTY; ++i )
	{
		m_Stat[i].m_base = 0;
		m_Stat[i].m_mod = 0;
		m_Stat[i].m_val = 0;
		m_Stat[i].m_max = 0;
		m_Stat[i].m_regen = 0;
	}
	m_Stat[STAT_FOOD].m_base = Stat_GetMax(STAT_FOOD);

	for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; ++i )
		m_Skill[i] = 0;

	m_LocalLight = 0;
	m_fClimbUpdated = false;

	ASSERT(IsDisconnected());
}

// Delete character
CChar::~CChar()
{
	if ( m_pClient )	// this should never happen
		m_pClient->m_NetState->markReadClosed();

	DeletePrepare();	// remove me early so virtuals will work
	if ( IsStatFlag(STATF_Ridden) )
	{
		CItem *pItem = Horse_GetMountItem();
		if ( pItem )
		{
			pItem->m_itFigurine.m_UID.InitUID();	// unlink it first
			pItem->Delete();
		}
	}

	Guild_Resign(MEMORY_GUILD);
	Guild_Resign(MEMORY_TOWN);

	if ( m_pParty )
		m_pParty->RemoveMember(this);

	NPC_PetClearOwners();		// clear follower slots on pet owner
	DeleteAll();				// remove me early so virtuals will work
	ClearNPC();
	ClearPlayer();
	g_Serv.StatDec(SERV_STAT_CHARS);
}

// Client is detaching from this CChar.
void CChar::ClientDetach()
{
	ADDTOCALLSTACK("CChar::ClientDetach");

	if ( !m_pClient )
		return;

	// Close trade windows
	CItem *pItemNext = NULL;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( pItem->IsType(IT_EQ_TRADE_WINDOW) )
			pItem->Delete();
	}

	// Disband the party if I'm the master
	if ( m_pParty && m_pParty->IsPartyMaster(this) )
		m_pParty->Disband();

	CSector *pSector = GetTopSector();
	if ( pSector )
		pSector->ClientDetach(this);

	m_pClient = NULL;
}

// Client is attaching to this CChar
void CChar::ClientAttach(CClient *pClient)
{
	ADDTOCALLSTACK("CChar::ClientAttach");
	if ( pClient == m_pClient )
		return;
	if ( !SetPlayerAccount(pClient->m_pAccount) )
		return;

	ASSERT(m_pPlayer);
	m_pPlayer->m_timeLastUsed = CServTime::GetCurrentTime();
	m_pClient = pClient;

	CSector *pSector = GetTopSector();
	if ( pSector )
		pSector->ClientAttach(this);

	FixClimbHeight();
}

// Client logged out or NPC is dead
void CChar::SetDisconnected()
{
	ADDTOCALLSTACK("CChar::SetDisconnected");
	if ( m_pClient )
	{
		m_pClient->m_NetState->markReadClosed();
		return;
	}

	if ( m_pParty )
		m_pParty->RemoveMember(this);

	if ( IsDisconnected() )
		return;

	RemoveFromView();

	CSector *pSector = GetTopSector();
	if ( pSector )
		pSector->m_Chars_Disconnect.InsertHead(this);
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
		CScriptTriggerArgs args;
		args.m_pO1 = pClient;
		r_Call("f_onchar_delete", this, &args, NULL, &tr);
		if ( tr == TRIGRET_RET_TRUE )
			return false;
	}

	ContentNotifyDelete();
	return true;
}

void CChar::Delete(bool fForce, CClient *pClient)
{
	ADDTOCALLSTACK("CChar::Delete");

	if ( !NotifyDelete(pClient) && !fForce )
		return;

	// Character has been deleted
	if ( m_pClient )
	{
		m_pClient->m_NetState->markReadClosed();
		m_pClient->CharDisconnect();
	}

	// Detach from account
	ClearPlayer();

	CObjBase::Delete();
}

// Is there something wrong with this char?
// RETURN: invalid code
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

				// Make sure we are still linked back to the world
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
signed char CChar::GetFixZ(CPointMap pt, DWORD dwBlockFlags)
{
	DWORD dwCan = GetMoveBlockFlags();
	if ( !dwBlockFlags )
		dwBlockFlags = dwCan;

	if ( dwCan & CAN_C_WALK )
		dwBlockFlags |= CAN_I_CLIMB;	// if we can walk than we can climb (ignore CAN_C_FLY here)

	CGrayMapBlockState block(dwBlockFlags, pt.m_z, pt.m_z + m_zClimbHeight + GetHeightMount(), pt.m_z + m_zClimbHeight + 2, GetHeightMount());
	g_World.GetFixPoint(pt, block);

	dwBlockFlags = block.m_Bottom.m_dwBlockFlags;
	if ( block.m_Top.m_dwBlockFlags )
	{
		dwBlockFlags |= CAN_I_ROOF;		// covered by something
		if ( block.m_Top.m_z < pt.m_z + m_zClimbHeight + (block.m_Top.m_dwTile > TERRAIN_QTY ? GetHeightMount() : GetHeightMount() / 2) )
			dwBlockFlags |= CAN_I_BLOCK;	// can't fit under this
	}
	if ( (dwCan != DWORD_MAX) && (dwBlockFlags != 0x0) )
	{

		if ( (dwBlockFlags & CAN_I_DOOR) && Can(CAN_C_GHOST) )
			dwBlockFlags &= ~CAN_I_BLOCK;

		if ( (dwBlockFlags & CAN_I_WATER) && Can(CAN_C_SWIM) )
			dwBlockFlags &= ~CAN_I_BLOCK;

		if ( !Can(CAN_C_FLY) )
		{
			if ( !(dwBlockFlags & CAN_I_CLIMB) )	// can climb anywhere
			{
				if ( block.m_Bottom.m_dwTile > TERRAIN_QTY )
				{
					if ( block.m_Bottom.m_z > pt.m_z + m_zClimbHeight + 2 )		// too high to climb
						return pt.m_z;
				}
				else if ( block.m_Bottom.m_z > pt.m_z + m_zClimbHeight + GetHeightMount() + 3 )
					return pt.m_z;
			}
		}
		if ( (dwBlockFlags & CAN_I_BLOCK) && !Can(CAN_C_PASSWALLS) )
			return pt.m_z;

		if ( block.m_Bottom.m_z >= UO_SIZE_Z )
			return pt.m_z;
	}
	if ( (GetHeightMount() + pt.m_z >= block.m_Top.m_z) && g_Cfg.m_iMountHeight && !IsPriv(PRIV_GM) && !IsPriv(PRIV_ALLMOVE) )
		return pt.m_z;

	return block.m_Bottom.m_z;
}

// Clean up weird flags
// NOTE:
//	Deleting a player char is VERY BAD! Be careful!
//
// RETURN: false = can't fix this
int CChar::FixWeirdness()
{
	ADDTOCALLSTACK("CChar::FixWeirdness");
	// Make sure my flags are good
	// NOTE: Stats and skills may go negative temporarily

	int iResultCode = CObjBase::IsWeird();
	if ( iResultCode )		// not recoverable - must try to delete the object
		return iResultCode;

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
		if ( !m_pNPC || !IsDisconnected() )
			StatFlag_Clear(STATF_Ridden);
		else
		{
			if ( Skill_GetActive() != NPCACT_RIDDEN )
				return 0x1203;

			CItem *pFigurine = Horse_GetMountItem();
			if ( !pFigurine )
				return 0x1204;

			// Move ridden creature to the same location as it's rider
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

	CCharBase *pCharDef = Char_GetDef();
	if ( !IsIndividualName() && (pCharDef->GetTypeName()[0] == '#') )
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

		if ( !m_pPlayer->GetSkillClass() )	// this should never happen
		{
			m_pPlayer->SetSkillClass(this, RESOURCE_ID(RES_SKILLCLASS));
			ASSERT(m_pPlayer->GetSkillClass());
		}

		// Make sure players don't get ridiculous skills/stats
		if ( (GetPrivLevel() <= PLEVEL_Player) && (g_Cfg.m_iOverSkillMultiply > 0) )
		{
			for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; ++i )
			{
				WORD wSkillVal = Skill_GetBase(static_cast<SKILL_TYPE>(i));
				WORD wSkillMax = Skill_GetMax(static_cast<SKILL_TYPE>(i));
				if ( wSkillVal > wSkillMax * g_Cfg.m_iOverSkillMultiply )
					Skill_SetBase(static_cast<SKILL_TYPE>(i), wSkillMax);
			}

			if ( IsPlayableCharacter() && !IsStatFlag(STATF_Polymorph) )
			{
				for ( int i = STAT_STR; i < STAT_BASE_QTY; ++i )
				{
					int iStatMax = Stat_GetLimit(static_cast<STAT_TYPE>(i));
					if ( Stat_GetAdjusted(static_cast<STAT_TYPE>(i)) > iStatMax * g_Cfg.m_iOverSkillMultiply )
						Stat_SetBase(static_cast<STAT_TYPE>(i), iStatMax);
				}
			}
		}
	}
	else
	{
		if ( !m_pNPC )
			return 0x1205;

		// Don't keep track of unused skills
		for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; ++i )
		{
			if ( (m_Skill[i] > 0) && (m_Skill[i] < g_Cfg.m_iSaveNPCSkills) )
				Skill_SetBase(static_cast<SKILL_TYPE>(i), 0);
		}
	}

	if ( GetTimerAdjusted() > 60 * 60 )		// timer is too long for a char?
		SetTimeout(1);

	return IsWeird();
}

// Creating a new char (not loading from save file)
void CChar::CreateNewCharCheck()
{
	ADDTOCALLSTACK("CChar::CreateNewCharCheck");
	m_prev_id = GetID();
	m_prev_Hue = GetHue();

	Stat_SetVal(STAT_STR, Stat_GetMax(STAT_STR));
	Stat_SetVal(STAT_DEX, Stat_GetMax(STAT_DEX));
	Stat_SetVal(STAT_INT, Stat_GetMax(STAT_INT));

	if ( m_pNPC )
	{
		if ( g_Cfg.m_bExperienceSystem && (g_Cfg.m_iExperienceMode & EXP_MODE_AUTOSET_EXP) )
		{
			if ( !m_exp )
			{
				CCharBase *pCharDef = Char_GetDef();
				int iMult = (Stat_GetMax(STAT_STR) + (Stat_GetMax(STAT_DEX) / 2) + Stat_GetMax(STAT_INT)) / 3;
				m_exp = maximum(Skill_GetBase(SKILL_ARCHERY),
						maximum(Skill_GetBase(SKILL_THROWING),
						maximum(Skill_GetBase(SKILL_SWORDSMANSHIP),
						maximum(Skill_GetBase(SKILL_MACEFIGHTING),
						maximum(Skill_GetBase(SKILL_FENCING),
						Skill_GetBase(SKILL_WRESTLING))))))
						+
						(Skill_GetBase(SKILL_TACTICS) / 4) +
						(Skill_GetBase(SKILL_PARRYING) / 4) +
						(Skill_GetBase(SKILL_MAGERY) / 3) +
						(Skill_GetBase(SKILL_PROVOCATION) / 4) +
						(Skill_GetBase(SKILL_PEACEMAKING) / 4) +
						(Skill_GetBase(SKILL_TAMING) / 4) +
						(pCharDef->m_defense * 3) +
						(pCharDef->m_attackBase * 6);
				m_exp = (m_exp * iMult) / 100;
			}

			if ( !m_level && g_Cfg.m_bLevelSystem && (m_exp > g_Cfg.m_iLevelNextAt) )
				ChangeExperience();
		}

		// Need a starting brain tick
		SetTimeout(1);
	}
}

bool CChar::DupeFrom(CChar *pChar, bool fNewbieItems)
{
	ADDTOCALLSTACK("CChar::DupeFrom");
	if ( !pChar )
		return false;

	// Copy CObjBase
	m_Can = pChar->m_Can;
	m_ModMaxWeight = pChar->m_ModMaxWeight;

	m_attackBase = pChar->m_attackBase;
	m_attackRange = pChar->m_attackRange;

	m_defenseBase = pChar->m_defenseBase;
	m_defenseRange = pChar->m_defenseRange;

	m_DamPhysical = pChar->m_DamPhysical;
	m_DamFire = pChar->m_DamFire;
	m_DamCold = pChar->m_DamCold;
	m_DamPoison = pChar->m_DamPoison;
	m_DamEnergy = pChar->m_DamEnergy;

	m_ResPhysical = pChar->m_ResPhysical;
	m_ResPhysicalMax = pChar->m_ResPhysicalMax;
	m_ResFire = pChar->m_ResFire;
	m_ResFireMax = pChar->m_ResFireMax;
	m_ResCold = pChar->m_ResCold;
	m_ResColdMax = pChar->m_ResColdMax;
	m_ResPoison = pChar->m_ResPoison;
	m_ResPoisonMax = pChar->m_ResPoisonMax;
	m_ResEnergy = pChar->m_ResEnergy;
	m_ResEnergyMax = pChar->m_ResEnergyMax;

	m_Luck = pChar->m_Luck;
	m_DamIncrease = pChar->m_DamIncrease;
	m_SpellDamIncrease = pChar->m_SpellDamIncrease;
	m_HitLifeLeech = pChar->m_HitLifeLeech;
	m_HitManaDrain = pChar->m_HitManaDrain;
	m_HitManaLeech = pChar->m_HitManaLeech;
	m_HitStaminaLeech = pChar->m_HitStaminaLeech;
	m_HitChanceIncrease = pChar->m_HitChanceIncrease;
	m_DefChanceIncrease = pChar->m_DefChanceIncrease;
	m_DefChanceIncreaseMax = pChar->m_DefChanceIncreaseMax;
	m_SwingSpeedIncrease = pChar->m_SwingSpeedIncrease;
	m_FasterCasting = pChar->m_FasterCasting;
	m_FasterCastRecovery = pChar->m_FasterCastRecovery;
	m_LowerManaCost = pChar->m_LowerManaCost;
	m_LowerReagentCost = pChar->m_LowerReagentCost;
	m_EnhancePotions = pChar->m_EnhancePotions;
	m_NightSight = pChar->m_NightSight;
	m_ReflectPhysicalDamage = pChar->m_ReflectPhysicalDamage;

	m_TagDefs.Copy(&pChar->m_TagDefs);
	m_BaseDefs.Copy(&pChar->m_BaseDefs);
	m_OEvents.Copy(&pChar->m_OEvents);

	// Copy CChar
	m_pArea = pChar->m_pArea;
	m_pRoom = pChar->m_pRoom;

	m_StatFlag = pChar->m_StatFlag;
	if ( g_World.m_fSaveParity )
		StatFlag_Set(STATF_SaveParity);		// it will get saved next time

	m_dirFace = pChar->m_dirFace;
	m_fonttype = pChar->m_fonttype;
	m_SpeechHue = pChar->m_SpeechHue;

	m_height = pChar->m_height;

	m_StepStealth = pChar->m_StepStealth;
	m_iVisualRange = pChar->m_iVisualRange;
	m_virtualGold = pChar->m_virtualGold;
	m_FollowerSlots = pChar->m_FollowerSlots;
	m_FollowerCur = pChar->m_FollowerCur;
	m_FollowerMax = pChar->m_FollowerMax;
	m_Tithing = pChar->m_Tithing;

	m_exp = pChar->m_exp;
	m_level = pChar->m_level;
	m_defense = pChar->m_defense;
	m_atUnk.m_Arg1 = pChar->m_atUnk.m_Arg1;
	m_atUnk.m_Arg2 = pChar->m_atUnk.m_Arg2;
	m_atUnk.m_Arg3 = pChar->m_atUnk.m_Arg3;

	m_prev_Hue = pChar->m_prev_Hue;
	m_prev_id = pChar->m_prev_id;
	SetHue(pChar->GetHue());
	SetID(pChar->GetID());

	m_wBloodHue = pChar->m_wBloodHue;

	SetName(pChar->GetName());
	Skill_Cleanup();

	g_World.m_uidLastNewChar = GetUID();	// for script access

	for ( size_t i = 0; i < STAT_QTY; ++i )
		m_Stat[i] = pChar->m_Stat[i];

	for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; ++i )
		m_Skill[i] = pChar->m_Skill[i];

	m_LocalLight = pChar->m_LocalLight;
	m_fClimbUpdated = pChar->m_fClimbUpdated;

	m_sTitle = pChar->m_sTitle;
	m_ptHome = pChar->m_ptHome;
	m_timeCreate = pChar->m_timeCreate;

	// Copy CCharNPC
	if ( m_pNPC && pChar->m_pNPC )
	{
		m_pNPC->m_Brain = pChar->m_pNPC->m_Brain;
		m_pNPC->m_Home_Dist_Wander = pChar->m_pNPC->m_Home_Dist_Wander;
		m_pNPC->m_Act_Motivation = pChar->m_pNPC->m_Act_Motivation;
		m_pNPC->m_bonded = pChar->m_pNPC->m_bonded;
		m_pNPC->m_spells = pChar->m_pNPC->m_spells;
	}

	// Copy items
	for ( int i = 0; i < LAYER_QTY; ++i )
	{
		LAYER_TYPE layer = static_cast<LAYER_TYPE>(i);
		CItem *pLayer = LayerFind(layer);
		if ( pLayer )
			pLayer->Delete();

		if ( !pChar->LayerFind(layer) )
			continue;

		CItem *pItem = CItem::CreateDupeItem(pChar->LayerFind(layer), this, true);
		pItem->LoadSetContainer(GetUID(), layer);
		if ( fNewbieItems )
		{
			pItem->SetAttr(ATTR_NEWBIE);
			if ( pItem->IsType(IT_CONTAINER) )
			{
				for ( CItem *pItemCont = static_cast<CItemContainer *>(pItem)->GetContentHead(); pItemCont != NULL; pItemCont = pItemCont->GetNext() )
				{
					pItemCont->SetAttr(ATTR_NEWBIE);

					CChar *pTest1 = static_cast<CGrayUID>(pItemCont->m_itNormal.m_more1).CharFind();
					if ( pTest1 && (pTest1 == pChar) )
						pItemCont->m_itNormal.m_more1 = static_cast<DWORD>(GetUID());

					CChar *pTest2 = static_cast<CGrayUID>(pItemCont->m_itNormal.m_more2).CharFind();
					if ( pTest2 && (pTest2 == pChar) )
						pItemCont->m_itNormal.m_more2 = static_cast<DWORD>(GetUID());

					CChar *pTest3 = pItemCont->m_uidLink.CharFind();
					if ( pTest3 && (pTest3 == pChar) )
						pItemCont->m_uidLink = GetUID();
				}
			}
		}

		CChar *pTest1 = static_cast<CGrayUID>(pItem->m_itNormal.m_more1).CharFind();
		if ( pTest1 && (pTest1 == pChar) )
			pItem->m_itNormal.m_more1 = static_cast<DWORD>(GetUID());

		CChar *pTest2 = static_cast<CGrayUID>(pItem->m_itNormal.m_more2).CharFind();
		if ( pTest2 )
		{
			if ( pTest2 == pChar )
				pItem->m_itNormal.m_more2 = static_cast<DWORD>(GetUID());
			else if ( pTest2->NPC_IsOwnedBy(pChar) )
			{
				if ( fNewbieItems )
				{
					// Remove any mount references for the memory, so no mount will appear when character die/dismount
					pItem->m_itNormal.m_more2 = 0;
				}
				else
				{
					// Otherwise we create a new mount
					pItem->m_itNormal.m_more2 = 0;	// MORE2 should be cleared before remove the memory, or the NPC will be removed too
					pItem->Delete(true);
					CChar *pMount = CreateNPC(pTest2->GetID());
					pMount->SetTopPoint(pChar->GetTopPoint());	// move the mount again because the dupe will place it in the same place as the 'invisible & disconnected' original (usually far away from where the guy will be, so the duped char can't touch the mount)
					pMount->DupeFrom(pTest2, fNewbieItems);
					pMount->NPC_PetSetOwner(this, false);
					Horse_Mount(pMount);
				}
			}
		}

		CChar *pTest3 = pItem->m_uidLink.CharFind();
		if ( pTest3 && (pTest3 == pChar) )
			pItem->m_uidLink = GetUID();
	}

	FixWeirdness();
	FixWeight();
	return true;
}

// Reading triggers from CHARDEF
bool CChar::ReadScriptTrig(CCharBase *pCharDef, CTRIG_TYPE trig, bool fVendor)
{
	ADDTOCALLSTACK("CChar::ReadScriptTrig");
	if ( !pCharDef || !pCharDef->HasTrigger(trig) )
		return false;
	CResourceLock s;
	if ( !pCharDef->ResourceLock(s) || !OnTriggerFind(s, sm_szTrigName[trig]) )
		return false;
	return ReadScript(s, fVendor);
}

// If this is a regen they will have a pack already
// RETURN:
//	true = default return (mostly ignored)
bool CChar::ReadScript(CResourceLock &s, bool fVendor)
{
	ADDTOCALLSTACK("CChar::ReadScript");
	bool fFullInterp = false;

	CItem *pItem = NULL;
	while ( s.ReadKeyParse() )
	{
		if ( s.IsKeyHead("ON", 2) )
			break;

		int iCmd = FindTableSorted(s.GetKey(), CItem::sm_szTemplateTable, COUNTOF(CItem::sm_szTemplateTable) - 1);

		if ( fVendor )
		{
			if ( iCmd != -1 )
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
					{
						pItem = NULL;
						continue;
					}
				}
			}
		}
		else
		{
			switch ( iCmd )
			{
				case ITC_FULLINTERP:
				{
					LPCTSTR pszArgs = s.GetArgStr();
					GETNONWHITESPACE(pszArgs);
					fFullInterp = (*pszArgs == '\0') ? true : (s.GetArgVal() != 0);
					continue;
				}
				case ITC_NEWBIESWAP:
				{
					if ( !pItem )
						continue;

					if ( pItem->IsAttr(ATTR_NEWBIE) )
					{
						if ( Calc_GetRandVal(s.GetArgVal()) == 0 )
							pItem->ClrAttr(ATTR_NEWBIE);
					}
					else
					{
						if ( Calc_GetRandVal(s.GetArgVal()) == 0 )
							pItem->SetAttr(ATTR_NEWBIE);
					}
					continue;
				}
				case ITC_ITEM:
				case ITC_CONTAINER:
				case ITC_ITEMNEWBIE:
				{
					pItem = CItem::CreateHeader(s.GetArgRaw(), this, (iCmd == ITC_ITEMNEWBIE));
					if ( !pItem )
					{
						m_UIDLastNewItem = GetUID();	// set m_UIDLastNewItem to CChar's UID to prevent calling any following functions meant to be called on that item
						continue;
					}
					m_UIDLastNewItem.InitUID();		// clear attr for the next cycle

					if ( iCmd == ITC_ITEMNEWBIE )
						pItem->SetAttr(ATTR_NEWBIE);

					if ( !pItem->IsItemInContainer() && !pItem->IsItemEquipped() )
						pItem = NULL;
					continue;
				}
				case ITC_BREAK:
				case ITC_BUY:
				case ITC_SELL:
				{
					pItem = NULL;
					continue;
				}
			}
		}

		if ( m_UIDLastNewItem == GetUID() )
			continue;

		if ( pItem )
		{
			if ( fFullInterp )	// modify the item
				pItem->r_Verb(s, &g_Serv);
			else
				pItem->r_LoadVal(s);
		}
		else
		{
			TRIGRET_TYPE tRet = OnTriggerRun(s, TRIGRUN_SINGLE_EXEC, &g_Serv, NULL, NULL);
			if ( (tRet == TRIGRET_RET_FALSE) && fFullInterp )
				continue;
			else if ( tRet != TRIGRET_RET_DEFAULT )
			{
				m_UIDLastNewItem.InitUID();
				return (tRet == TRIGRET_RET_FALSE);
			}
		}
	}

	return true;
}

// Create an NPC from script
void CChar::NPC_LoadScript(bool fRestock)
{
	ADDTOCALLSTACK("CChar::NPC_LoadScript");
	if ( !m_pNPC )
		SetNPCBrain(GetNPCBrain(false));	// should have a default brain (watch out for override vendor)

	CCharBase *pCharDef = Char_GetDef();

	// 1) CHARDEF triggers
	if ( !m_pPlayer )
	{
		CChar *pChar = GetChar();
		if ( pChar )
		{
			CGrayUID uidOldAct = pChar->m_Act_Targ;
			pChar->m_Act_Targ = GetUID();
			pChar->ReadScriptTrig(pCharDef, CTRIG_Create);
			pChar->m_Act_Targ = uidOldAct;
		}
	}

	// This remains untouched but moved after the chardef's section
	if ( fRestock && IsTrigUsed(TRIGGER_NPCRESTOCK) )
		ReadScriptTrig(pCharDef, CTRIG_NPCRestock);

	CreateNewCharCheck();	// this one is giving stats, etc to the char, so we can read/set them in the next triggers
}

// @Create trigger (NPC version)
void CChar::NPC_CreateTrigger()
{
	ADDTOCALLSTACK("CChar::NPC_CreateTrigger");
	if ( !m_pNPC )
		return;

	LPCTSTR pszTrigName = "@Create";
	CTRIG_TYPE iAction = static_cast<CTRIG_TYPE>(FindTableSorted(pszTrigName, sm_szTrigName, COUNTOF(sm_szTrigName) - 1));
	TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;

	// 2) TEVENTS triggers
	CCharBase *pCharDef = Char_GetDef();
	for ( size_t i = 0; i < pCharDef->m_TEvents.GetCount(); ++i )
	{
		CResourceLink *pLink = pCharDef->m_TEvents[i];
		if ( !pLink || !pLink->HasTrigger(iAction) )
			continue;
		CResourceLock s;
		if ( !pLink->ResourceLock(s) )
			continue;
		iRet = CScriptObj::OnTriggerScript(s, pszTrigName, this, 0);
		if ( (iRet != TRIGRET_RET_FALSE) && (iRet != TRIGRET_RET_DEFAULT) )
			return;
	}

	// 4) EVENTSPET triggers
	for ( size_t i = 0; i < g_Cfg.m_pEventsPetLink.GetCount(); ++i )
	{
		CResourceLink *pLink = g_Cfg.m_pEventsPetLink[i];
		if ( !pLink || !pLink->HasTrigger(iAction) )
			continue;
		CResourceLock s;
		if ( !pLink->ResourceLock(s) )
			continue;
		iRet = CScriptObj::OnTriggerScript(s, pszTrigName, this, 0);
		if ( (iRet != TRIGRET_RET_FALSE) && (iRet != TRIGRET_RET_DEFAULT) )
			return;
	}
}

void CChar::OnWeightChange(int iChange)
{
	ADDTOCALLSTACK("CChar::OnWeightChange");
	CContainer::OnWeightChange(iChange);
	UpdateStatsFlag();
}

bool CChar::SetName(LPCTSTR pszName)
{
	ADDTOCALLSTACK("CChar::SetName");
	return SetNamePool(pszName);
}

height_t CChar::GetHeightMount() const
{
	ADDTOCALLSTACK("CChar::GetHeightMount");
	height_t height = GetHeight();
	if ( IsStatFlag(STATF_OnHorse|STATF_Hovering) )
		height += 4;
	return height;
}

height_t CChar::GetHeight() const
{
	ADDTOCALLSTACK("CChar::GetHeight");
	if ( m_height )		// set by a dynamic variable (On=@Create  Height=10)
		return m_height;

	CCharBase *pCharDef = Char_GetDef();
	height_t tmpHeight = pCharDef->GetHeight();
	if ( tmpHeight )	// set by a chardef variable ([CHARDEF 10]  Height=10)
		return tmpHeight;

	char *heightDef = Str_GetTemp();
	sprintf(heightDef, "height_0%x", static_cast<unsigned int>(pCharDef->GetDispID()));
	tmpHeight = static_cast<height_t>(g_Exp.m_VarDefs.GetKeyNum(heightDef));
	if ( tmpHeight )	// set by a defname ([DEFNAME charheight]  height_0a)
		return tmpHeight;

	sprintf(heightDef, "height_%u", static_cast<unsigned int>(pCharDef->GetDispID()));
	tmpHeight = static_cast<height_t>(g_Exp.m_VarDefs.GetKeyNum(heightDef));
	if ( tmpHeight )	// set by a defname ([DEFNAME charheight]  height_10)
		return tmpHeight;

	return PLAYER_HEIGHT;	// if everything fails
}

// Just set the base id and not the actual display id
// NOTE: Never return NULL
void CChar::SetID(CREID_TYPE id)
{
	ADDTOCALLSTACK("CChar::SetID");

	CCharBase *pCharDef = CCharBase::FindCharBase(id);
	if ( !pCharDef )
	{
		DEBUG_ERR(("Creating invalid chardef 0%x\n", id));
		id = static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType(RES_CHARDEF, "DEFAULTCHAR"));
		if ( id <= CREID_INVALID )
			id = CREID_MAN;

		pCharDef = CCharBase::FindCharBase(id);
		if ( !pCharDef )
			return;
	}

	if ( pCharDef == Char_GetDef() )
		return;

	m_BaseRef.SetRef(pCharDef);
	if ( m_prev_id == CREID_INVALID )
		m_prev_id = GetID();

	if ( !IsMountCapable() )	// new body may not be capable of ride mounts
		Horse_UnMount();

	if ( IsStatFlag(STATF_Hovering) && !IsGargoyle() )	// new body may not be capable of use gargoyle fly ability
		ToggleFlying();

	if ( !pCharDef->Can(CAN_C_EQUIP) )	// new body may not be capable of equip items (except maybe on hands)
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
	Update();
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

const LPCTSTR CChar::sm_szRefKeys[CHR_QTY + 1] =
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

bool CChar::r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef)
{
	ADDTOCALLSTACK("CChar::r_GetRef");
	int index = FindTableHeadSorted(pszKey, sm_szRefKeys, COUNTOF(sm_szRefKeys) - 1);
	if ( index >= 0 )
	{
		pszKey += strlen(sm_szRefKeys[index]);
		SKIP_SEPARATORS(pszKey);
		switch ( index )
		{
			case CHR_ACCOUNT:
				if ( pszKey[-1] != '.' )	// only used as a ref
					break;
				pRef = m_pPlayer ? m_pPlayer->m_pAccount : NULL;
				return true;
			case CHR_ACT:
				if ( pszKey[-1] != '.' )	// only used as a ref
					break;
				pRef = m_Act_Targ.ObjFind();
				return true;
			case CHR_FINDLAYER:				// find equipped layers
				pRef = LayerFind(static_cast<LAYER_TYPE>(Exp_GetLLSingle(pszKey)));
				SKIP_SEPARATORS(pszKey);
				return true;
			case CHR_MEMORYFINDTYPE:		// find a type of memory
				pRef = Memory_FindTypes(static_cast<WORD>(Exp_GetLLSingle(pszKey)));
				SKIP_SEPARATORS(pszKey);
				return true;
			case CHR_MEMORYFIND:			// find memory related to a given UID
				pRef = Memory_FindObj(static_cast<CGrayUID>(Exp_GetLLSingle(pszKey)));
				SKIP_SEPARATORS(pszKey);
				return true;
			case CHR_OWNER:
				pRef = NPC_PetGetOwner();
				return true;
			case CHR_WEAPON:
				pRef = m_uidWeapon.ObjFind();
				return true;
			case CHR_REGION:
				pRef = m_pArea;
				return true;
		}
	}

	if ( r_GetRefContainer(pszKey, pRef) )
		return true;

	return CObjBase::r_GetRef(pszKey, pRef);
}

enum CHC_TYPE
{
	#define ADD(a,b) CHC_##a,
	#include "../tables/CChar_props.tbl"
	#undef ADD
	CHC_QTY
};

const LPCTSTR CChar::sm_szLoadKeys[CHC_QTY + 1] =
{
	#define ADD(a,b) b,
	#include "../tables/CChar_props.tbl"
	#undef ADD
	NULL
};

bool CChar::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CChar::r_WriteVal");

	if ( m_pClient && m_pClient->r_WriteVal(pszKey, sVal, pSrc) )
		return true;

	EXC_TRY("WriteVal");

	CCharBase *pCharDef = Char_GetDef();
	ASSERT(pCharDef);
	CChar *pCharSrc = pSrc->GetChar();

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	if ( index < 0 )
	{
	do_default:
		if ( m_pPlayer && m_pPlayer->r_WriteVal(this, pszKey, sVal) )
			return true;
		if ( m_pNPC && m_pNPC->r_WriteVal(this, pszKey, sVal) )
			return true;
		if ( r_WriteValContainer(pszKey, sVal, pSrc) )
			return true;

		// Stats
		STAT_TYPE stat = g_Cfg.FindStatKey(pszKey);
		if ( stat != STAT_NONE )
		{
			sVal.FormatVal(Stat_GetAdjusted(stat));
			return true;
		}
		if ( !strnicmp(pszKey, "O", 1) )
		{
			stat = g_Cfg.FindStatKey(pszKey + 1);
			if ( stat != STAT_NONE )
			{
				sVal.FormatVal(Stat_GetBase(stat));
				return true;
			}
		}
		if ( !strnicmp(pszKey, "MOD", 3) )
		{
			stat = g_Cfg.FindStatKey(pszKey + 3);
			if ( stat != STAT_NONE )
			{
				sVal.FormatVal(Stat_GetMod(stat));
				return true;
			}
		}

		// Skills
		SKILL_TYPE skill = g_Cfg.FindSkillKey(pszKey);
		if ( IsSkillBase(skill) )
		{
			WORD wVal = Skill_GetBase(skill);
			sVal.Format("%hu.%hu", wVal / 10, wVal % 10);
			return true;
		}

		return CObjBase::r_WriteVal(pszKey, sVal, pSrc);
	}

	switch ( index )
	{
		case CHC_REGENFOOD:
		case CHC_REGENHITS:
		case CHC_REGENSTAM:
		case CHC_REGENMANA:
		case CHC_REGENVALFOOD:
		case CHC_REGENVALHITS:
		case CHC_REGENVALSTAM:
		case CHC_REGENVALMANA:
		case CHC_SPELLTIMEOUT:
			sVal.FormatLLVal(GetDefNum(pszKey));
			break;
		case CHC_ATTACKER:
		{
			if ( strlen(pszKey) == 8 )
			{
				sVal.FormatVal(m_lastAttackers.size());
				return true;
			}

			sVal.FormatVal(0);
			pszKey += 8;

			if ( *pszKey == '.' )
			{
				++pszKey;
				if ( !strnicmp(pszKey, "ID", 2) )
				{
					pszKey += 2;
					sVal.FormatVal(Attacker_GetID(static_cast<CGrayUID>(Exp_GetLLSingle(pszKey)).CharFind()));
					return true;
				}
				else if ( !strnicmp(pszKey, "TARGET", 6) )
				{
					pszKey += 6;
					if ( m_Act_Targ )
						sVal.FormatHex(m_Fight_Targ);
					else
						sVal.FormatVal(-1);
					return true;
				}
				if ( m_lastAttackers.size() )
				{
					size_t id;
					if ( !strnicmp(pszKey, "MAX", 3) )
					{
						pszKey += 3;
						id = Attacker_GetID(Attacker_GetHighestDam());
					}
					else if ( !strnicmp(pszKey, "LAST", 4) )
					{
						pszKey += 4;
						id = Attacker_GetID(Attacker_GetLowestElapsed());
					}
					else
					{
						id = Exp_GetVal(pszKey);
					}

					SKIP_SEPARATORS(pszKey);
					if ( id < m_lastAttackers.size() )
					{
						LastAttackers &refAttacker = m_lastAttackers.at(id);
						if ( !strnicmp(pszKey, "UID", 3) || (*pszKey == '\0') )
						{
							sVal.FormatHex(refAttacker.charUID);
							return true;
						}
						else if ( !strnicmp(pszKey, "ELAPSED", 7) )
						{
							sVal.FormatLLVal(refAttacker.elapsed);
							return true;
						}
						else if ( !strnicmp(pszKey, "DAM", 3) )
						{
							sVal.FormatLLVal(refAttacker.damage);
							return true;
						}
						else if ( (!strnicmp(pszKey, "THREAT", 6)) )
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
			if ( !strnicmp(pszKey, "BREATH.DAM", 10) )
			{
				CVarDefCont *pVar = GetDefKey(pszKey, true);
				sVal.FormatLLVal(pVar ? pVar->GetValNum() : 0);
				return true;
			}
			else if ( !strnicmp(pszKey, "BREATH.HUE", 10) || !strnicmp(pszKey, "BREATH.ANIM", 11) || !strnicmp(pszKey, "BREATH.TYPE", 11) )
			{
				CVarDefCont *pVar = GetDefKey(pszKey, true);
				sVal.FormatHex(pVar ? static_cast<DWORD>(pVar->GetValNum()) : 0);
				return true;
			}
			return false;
		}
		case CHC_NOTOSAVE:
		{
			if ( strlen(pszKey) == 8 )
			{
				sVal.FormatVal(m_notoSaves.size());
				return true;
			}

			sVal.FormatVal(0);
			pszKey += 8;

			if ( *pszKey == '.' )
			{
				++pszKey;
				if ( !strnicmp(pszKey, "ID", 2) )
				{
					pszKey += 2;
					sVal.FormatVal(NotoSave_GetID(static_cast<CGrayUID>(Exp_GetLLSingle(pszKey)).CharFind()));
					return true;
				}
				if ( m_notoSaves.size() )
				{
					size_t id = Exp_GetVal(pszKey);
					SKIP_SEPARATORS(pszKey);
					if ( id < m_notoSaves.size() )
					{
						NotoSaves refNoto = m_notoSaves.at(id);
						if ( !strnicmp(pszKey, "UID", 3) || (*pszKey == '\0') )
						{
							sVal.FormatHex(refNoto.charUID);
							return true;
						}
						else if ( !strnicmp(pszKey, "ELAPSED", 7) )
						{
							sVal.FormatLLVal(refNoto.elapsed);
							return true;
						}
						else if ( !strnicmp(pszKey, "VALUE", 5) )
						{
							sVal.FormatVal(refNoto.value);
							return true;
						}
						else if ( !strnicmp(pszKey, "COLOR", 5) )
						{
							sVal.FormatVal(refNoto.color);
							return true;
						}
						return false;
					}
				}
			}
			return true;
		}
		case CHC_FIGHTRANGE:	// RANGE is now writable so this got changed to FIGHTRANGE as read-only
			sVal.FormatVal(CalcFightRange(m_uidWeapon.ItemFind()));
			return true;
		case CHC_BLOODCOLOR:
			sVal.FormatHex(m_wBloodHue);
			break;
		case CHC_FAME:
		{
			if ( pszKey[4] != '.' )
				goto do_default;

			if ( g_Cfg.m_Fame.GetCount() <= 0 )
			{
				DEBUG_ERR(("FAME ranges have not been defined\n"));
				sVal.FormatVal(0);
				return true;
			}

			TCHAR *ppLevel_sep[100];
			const CGString *pFameAt0 = g_Cfg.m_Fame.GetAt(0);

			TCHAR *pszFameAt0 = new TCHAR[pFameAt0->GetLength() + 1];
			strcpylen(pszFameAt0, pFameAt0->GetPtr());

			size_t iArgQty = Str_ParseCmds(pszFameAt0, ppLevel_sep, COUNTOF(ppLevel_sep), ",");
			if ( iArgQty > 0 )
			{
				int iFame = Stat_GetAdjusted(STAT_FAME);
				for ( size_t i = iArgQty - 1; i > 0; --i )
				{
					if ( !IsStrNumeric(ppLevel_sep[i]) )
					{
						DEBUG_ERR(("'%s' is not a valid fame value\n", ppLevel_sep[i]));
					}
					else if ( iFame >= ATOI(ppLevel_sep[i]) )
					{
						sVal.FormatVal(!g_Cfg.m_Fame.GetAt(i + 1)->CompareNoCase(pszKey + 5));
						delete[] pszFameAt0;
						return true;
					}
				}
			}

			sVal.FormatVal(0);
			delete[] pszFameAt0;
			return true;
		}
		case CHC_FOLLOWERSLOTS:
			sVal.FormatVal(m_FollowerSlots);
			break;
		case CHC_CURFOLLOWER:
			sVal.FormatVal(m_FollowerCur);
			break;
		case CHC_MAXFOLLOWER:
			sVal.FormatVal(m_FollowerMax);
			break;
		case CHC_TITHING:
			sVal.FormatVal(m_Tithing);
			break;
		case CHC_SKILLCHECK:	// odd way to get skills checking into the triggers
		{
			pszKey += 10;
			SKIP_SEPARATORS(pszKey);

			TCHAR *ppArgs[2];
			Str_ParseCmds(const_cast<TCHAR *>(pszKey), ppArgs, COUNTOF(ppArgs));

			SKILL_TYPE skill = g_Cfg.FindSkillKey(ppArgs[0]);
			if ( skill == SKILL_NONE )
				return false;
			sVal.FormatVal(Skill_CheckSuccess(skill, Exp_GetVal(ppArgs[1])));
			return true;
		}
		case CHC_SKILLBEST:
		{
			pszKey += 9;
			size_t uRank = 0;
			if ( *pszKey == '.' )
			{
				SKIP_SEPARATORS(pszKey);
				uRank = static_cast<size_t>(Exp_GetLLSingle(pszKey));
			}
			sVal.FormatVal(Skill_GetBest(uRank));
			return true;
		}
		case CHC_SEX:
		{
			pszKey += 3;
			SKIP_SEPARATORS(pszKey);

			TCHAR *ppArgs[2];
			Str_ParseCmds(const_cast<TCHAR *>(pszKey), ppArgs, COUNTOF(ppArgs), ":,/");

			sVal = pCharDef->IsFemale() ? ppArgs[1] : ppArgs[0];
			return true;
		}
		case CHC_KARMA:
		{
			if ( pszKey[5] != '.' )
				goto do_default;

			if ( g_Cfg.m_Karma.GetCount() <= 0 )
			{
				DEBUG_ERR(("KARMA ranges have not been defined\n"));
				sVal.FormatVal(0);
				return true;
			}

			TCHAR *ppLevel_sep[100];
			const CGString *pKarmaAt0 = g_Cfg.m_Karma.GetAt(0);

			TCHAR *pszKarmaAt0 = new TCHAR[pKarmaAt0->GetLength() + 1];
			strcpylen(pszKarmaAt0, pKarmaAt0->GetPtr());

			size_t iArgQty = Str_ParseCmds(pszKarmaAt0, ppLevel_sep, COUNTOF(ppLevel_sep), ",");
			if ( iArgQty > 0 )
			{
				int iKarma = Stat_GetAdjusted(STAT_KARMA);
				for ( size_t i = iArgQty - 1; i > 0; --i )
				{
					if ( (ppLevel_sep[i][0] != '-') && !IsStrNumeric(ppLevel_sep[i]) )
					{
						DEBUG_ERR(("'%s' is not a valid karma value\n", ppLevel_sep[i]));
					}
					else if ( iKarma >= ATOI(ppLevel_sep[i]) )
					{
						sVal.FormatVal(!g_Cfg.m_Karma.GetAt(i + 1)->CompareNoCase(pszKey + 6));
						delete[] pszKarmaAt0;
						return true;
					}
				}
			}

			sVal.FormatVal(0);
			delete[] pszKarmaAt0;
			return true;
		}
		case CHC_AR:
			sVal.FormatVal(m_defense + pCharDef->m_defense);
			return true;
		case CHC_AGE:
			sVal.FormatLLVal(-(g_World.GetTimeDiff(m_timeCreate) / (TICK_PER_SEC * 60 * 60 * 24)));		// in days
			return true;
		case CHC_BANKBALANCE:
			sVal.FormatUVal(GetContainerCreate(LAYER_BANKBOX)->ContentCount(RESOURCE_ID(RES_TYPEDEF, IT_GOLD)));
			return true;
		case CHC_CANCAST:
		{
			pszKey += 7;
			GETNONWHITESPACE(pszKey);

			TCHAR *ppArgs[2];
			size_t iArgQty = Str_ParseCmds(const_cast<TCHAR *>(pszKey), ppArgs, COUNTOF(ppArgs));
			if ( iArgQty <= 0 )
				return false;

			SPELL_TYPE spell = static_cast<SPELL_TYPE>(g_Cfg.ResourceGetIndexType(RES_SPELL, ppArgs[0]));

			bool fCheckAntiMagic = true;
			if ( iArgQty == 2 )
				fCheckAntiMagic = (Exp_GetVal(ppArgs[1]) >= 1);

			sVal.FormatVal(Spell_CanCast(spell, true, this, false, fCheckAntiMagic));
			return true;
		}
		case CHC_CANMAKE:
		{
			pszKey += 7;
			ITEMID_TYPE id = static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType(RES_ITEMDEF, pszKey));
			sVal.FormatVal(Skill_MakeItem(id, UID_CLEAR, SKTRIG_SELECT));
			return true;
		}
		case CHC_CANMAKESKILL:
		{
			pszKey += 12;
			ITEMID_TYPE id = static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType(RES_ITEMDEF, pszKey));
			sVal.FormatVal(Skill_MakeItem(id, UID_CLEAR, SKTRIG_SELECT, true));
			return true;
		}
		case CHC_INCREASEDAM:
			sVal.FormatVal(m_DamIncrease);
			return true;
		case CHC_INCREASEDEFCHANCE:
			sVal.FormatVal(m_DefChanceIncrease);
			return true;
		case CHC_INCREASEDEFCHANCEMAX:
			sVal.FormatVal(m_DefChanceIncreaseMax);
			return true;
		case CHC_INCREASEHITCHANCE:
			sVal.FormatVal(m_HitChanceIncrease);
			return true;
		case CHC_INCREASESPELLDAM:
			sVal.FormatVal(m_SpellDamIncrease);
			return true;
		case CHC_INCREASESWINGSPEED:
			sVal.FormatVal(m_SwingSpeedIncrease);
			return true;
		case CHC_FASTERCASTING:
			sVal.FormatVal(m_FasterCasting);
			return true;
		case CHC_FASTERCASTRECOVERY:
			sVal.FormatVal(m_FasterCastRecovery);
			return true;
		case CHC_HITLEECHLIFE:
			sVal.FormatVal(m_HitLifeLeech);
			return true;
		case CHC_HITLEECHMANA:
			sVal.FormatVal(m_HitManaLeech);
			return true;
		case CHC_HITLEECHSTAM:
			sVal.FormatVal(m_HitStaminaLeech);
			return true;
		case CHC_HITMANADRAIN:
			sVal.FormatVal(m_HitManaDrain);
			return true;
		case CHC_LOWERMANACOST:
			sVal.FormatVal(m_LowerManaCost);
			return true;
		case CHC_LOWERREAGENTCOST:
			sVal.FormatVal(m_LowerReagentCost);
			return true;
		case CHC_SKILLUSEQUICK:
		{
			pszKey += 13;
			GETNONWHITESPACE(pszKey);

			if ( *pszKey )
			{
				TCHAR *ppArgs[2];
				size_t iArgQty = Str_ParseCmds(const_cast<TCHAR *>(pszKey), ppArgs, COUNTOF(ppArgs));
				if ( iArgQty == 2 )
				{
					SKILL_TYPE skill = g_Cfg.FindSkillKey(ppArgs[0]);
					if ( skill == SKILL_NONE )
						return false;

					sVal.FormatVal(Skill_UseQuick(skill, Exp_GetVal(ppArgs[1])));
					return true;
				}
			}
			return false;
		}
		case CHC_SKILLTEST:
		{
			pszKey += 9;
			if ( *pszKey )
			{
				CResourceQtyArray Resources;
				if ( (Resources.Load(pszKey) > 0) && SkillResourceTest(&Resources) )
				{
					sVal.FormatVal(1);
					return true;
				}
			}
			sVal.FormatVal(0);
			return true;
		}
		case CHC_CANMOVE:
		{
			pszKey += 7;
			GETNONWHITESPACE(pszKey);

			DIR_TYPE dir = GetDirStr(pszKey);
			CPointBase pt = GetTopPoint();
			pt.Move(dir);

			DWORD dwBlockFlags = 0;
			sVal.FormatVal(CheckValidMove(pt, &dwBlockFlags, dir, NULL) != NULL);
			return true;
		}
		case CHC_GOLD:
			sVal.FormatUVal(ContentCount(RESOURCE_ID(RES_TYPEDEF, IT_GOLD)));
			break;
		case CHC_MOUNT:
		{
			CItem *pItem = LayerFind(LAYER_HORSE);
			if ( pItem )
				sVal.FormatHex(pItem->m_itFigurine.m_UID);
			else
				sVal.FormatVal(0);
			return true;
		}
		case CHC_MOVE:
		{
			pszKey += 4;
			GETNONWHITESPACE(pszKey);

			CPointBase pt = GetTopPoint();
			pt.Move(GetDirStr(pszKey));

			CRegionBase *pArea = pt.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA);
			if ( pArea )
			{
				DWORD dwBlockFlags = 0;
				g_World.GetHeightPoint2(pt, dwBlockFlags, true);
				sVal.FormatHex(dwBlockFlags);
			}
			else
				sVal.FormatHex(DWORD_MAX);
			return true;
		}
		case CHC_DISPIDDEC:
			sVal.FormatVal(pCharDef->m_trackID);
			return true;
		case CHC_GUILDABBREV:
			sVal = Guild_Abbrev(MEMORY_GUILD);
			return true;
		case CHC_ID:
			sVal = g_Cfg.ResourceGetName(pCharDef->GetResourceID());
			return true;
		case CHC_ISGM:
			sVal.FormatVal(IsPriv(PRIV_GM));
			return true;
		case CHC_ISINPARTY:
			sVal.FormatVal(m_pParty != NULL);
			return true;
		case CHC_ISMYPET:
			sVal.FormatVal(NPC_IsOwnedBy(pCharSrc, true));
			return true;
		case CHC_ISONLINE:
		{
			if ( m_pPlayer )
			{
				sVal.FormatVal(m_pClient != NULL);
				return true;
			}
			if ( m_pNPC )
			{
				sVal.FormatVal(!IsDisconnected());
				return true;
			}
			sVal.FormatVal(0);
			return true;
		}
		case CHC_ISSTUCK:
		{
			CPointBase pt = GetTopPoint();
			if ( OnFreezeCheck() )
				sVal.FormatVal(1);
			else if ( CanMoveWalkTo(pt, true, true, DIR_N) || CanMoveWalkTo(pt, true, true, DIR_E) || CanMoveWalkTo(pt, true, true, DIR_S) || CanMoveWalkTo(pt, true, true, DIR_W) )
				sVal.FormatVal(0);
			else
				sVal.FormatVal(1);
			return true;
		}
		case CHC_ISVENDOR:
			sVal.FormatVal(NPC_IsVendor());
			return true;
		case CHC_ISVERTICALSPACE:
		{
			pszKey += 15;
			CPointMap pt;
			if ( strlen(pszKey) )
			{
				pt = g_Cfg.GetRegionPoint(pszKey);
				if ( !pt.IsValidPoint() )
				{
					DEBUG_ERR(("%s: invalid point passed as argument\n", sm_szLoadKeys[index]));
					return false;
				}
			}
			else
				pt = GetTopPoint();
			sVal.FormatVal(IsVerticalSpace(pt, false));
			return true;
		}
		case CHC_MEMORY:
		{
			CItemMemory *pMemory;
			pszKey += 6;
			if ( *pszKey == '.' )
			{
				++pszKey;
				pMemory = Memory_FindObj(static_cast<CGrayUID>(Exp_GetLLVal(pszKey)));
			}
			else
				pMemory = Memory_FindObj(pCharSrc);

			WORD wFlags = 0;
			if ( pMemory )
				wFlags = pMemory->GetMemoryTypes();
			sVal.FormatHex(wFlags);
			return true;
		}
		case CHC_NAME:
			sVal = GetName(false);
			break;
		case CHC_SKILLTOTAL:
		{
			pszKey += 10;
			SKIP_SEPARATORS(pszKey);
			GETNONWHITESPACE(pszKey);

			int iVal = 0;
			bool fComp = true;
			if ( *pszKey != '\0' )
			{
				if ( *pszKey == '+' )
					iVal = Exp_GetVal(++pszKey);
				else if ( *pszKey == '-' )
					iVal = -Exp_GetVal(++pszKey);
				else
				{
					iVal = Exp_GetVal(pszKey);
					fComp = false;
				}
			}
			sVal.FormatUVal(GetSkillTotal(iVal, fComp));
			return true;
		}
		case CHC_SWING:
			sVal.FormatVal(m_atFight.m_Swing_State);
			break;
		case CHC_TOWNABBREV:
			sVal = Guild_Abbrev(MEMORY_TOWN);
			return true;
		case CHC_MAXWEIGHT:
			sVal.FormatVal(g_Cfg.Calc_MaxCarryWeight(this));
			return true;
		case CHC_ACCOUNT:
		{
			if ( m_pPlayer )
			{
				pszKey += 7;
				if ( *pszKey == '.' )		// used as a ref
				{
					++pszKey;
					SKIP_SEPARATORS(pszKey);

					CScriptObj *pRef = m_pPlayer->m_pAccount;
					if ( pRef )
					{
						if ( pRef->r_WriteVal(pszKey, sVal, pSrc) )
							break;
						return false;
					}
				}
				sVal = m_pPlayer->m_pAccount->GetName();
			}
			else
				sVal.Empty();
			break;
		}
		case CHC_ACT:
		{
			if ( pszKey[3] == '.' )		// used as a ref
				goto do_default;
			sVal.FormatHex(m_Act_Targ.GetObjUID());
			break;
		}
		case CHC_ACTP:
			sVal = m_Act_p.WriteUsed();
			break;
		case CHC_ACTPRV:
			sVal.FormatHex(m_Act_TargPrv.GetObjUID());
			break;
		case CHC_ACTDIFF:
			sVal.FormatVal(m_Act_Difficulty * 10);
			break;
		case CHC_ACTARG1:
			sVal.FormatHex(m_atUnk.m_Arg1);
			break;
		case CHC_ACTARG2:
			sVal.FormatHex(m_atUnk.m_Arg2);
			break;
		case CHC_ACTARG3:
			sVal.FormatHex(m_atUnk.m_Arg3);
			break;
		case CHC_ACTION:
		{
			if ( Skill_GetActive() == SKILL_NONE )
				sVal.FormatVal(SKILL_NONE);
			else
				sVal = g_Cfg.ResourceGetName(RESOURCE_ID(RES_SKILL, Skill_GetActive()));
			break;
		}
		case CHC_BODY:
			sVal = g_Cfg.ResourceGetName(RESOURCE_ID(RES_CHARDEF, GetDispID()));
			break;
		case CHC_CREATE:
			sVal.FormatLLVal(-(g_World.GetTimeDiff(m_timeCreate) / TICK_PER_SEC));
			break;
		case CHC_DIR:
		{
			pszKey += 3;
			CChar *pChar = static_cast<CGrayUID>(Exp_GetLLSingle(pszKey)).CharFind();
			if ( pChar )
				sVal.FormatVal(GetDir(pChar));
			else
				sVal.FormatVal(m_dirFace);
			break;
		}
		case CHC_EMOTEACT:
			sVal.FormatVal(IsStatFlag(STATF_EmoteAction));
			break;
		case CHC_FLAGS:
			sVal.FormatULLHex(m_StatFlag);
			break;
		case CHC_FONT:
			sVal.FormatVal(m_fonttype);
			break;
		case CHC_SPEECHCOLOR:
			sVal.FormatVal(m_SpeechHue);
			break;
		case CHC_FOOD:
			sVal.FormatVal(Stat_GetVal(STAT_FOOD));
			break;
		case CHC_HEIGHT:
			sVal.FormatVal(GetHeight());
			break;
		case CHC_HITPOINTS:
		case CHC_HITS:
			sVal.FormatVal(Stat_GetVal(STAT_STR));
			break;
		case CHC_STAM:
		case CHC_STAMINA:
			sVal.FormatVal(Stat_GetVal(STAT_DEX));
			break;
		case CHC_STEPSTEALTH:
			sVal.FormatVal(m_StepStealth);
			break;
		case CHC_MANA:
			sVal.FormatVal(Stat_GetVal(STAT_INT));
			break;
		case CHC_MAXFOOD:
			sVal.FormatVal(Stat_GetMax(STAT_FOOD));
			break;
		case CHC_MAXHITS:
			sVal.FormatVal(Stat_GetMax(STAT_STR));
			break;
		case CHC_MAXMANA:
			sVal.FormatVal(Stat_GetMax(STAT_INT));
			break;
		case CHC_MAXSTAM:
			sVal.FormatVal(Stat_GetMax(STAT_DEX));
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

			CChar *pChar = static_cast<CGrayUID>(Exp_GetLLVal(pszKey)).CharFind();
			if ( !pChar )
				pChar = pCharSrc;

			SKIP_ARGSEP(pszKey);
			sVal.FormatVal(Noto_GetFlag(pChar, (Exp_GetVal(pszKey) >= 1)));
			break;
		}
		case CHC_NPC:
			goto do_default;
		case CHC_OBODY:
			sVal = g_Cfg.ResourceGetName(RESOURCE_ID(RES_CHARDEF, m_prev_id));
			break;
		case CHC_OSKIN:
			sVal.FormatHex(m_prev_Hue);
			break;
		case CHC_P:
			goto do_default;
		case CHC_STONE:
			sVal.FormatVal(IsStatFlag(STATF_Stone));
			break;
		case CHC_TITLE:
			sVal = (strlen(pszKey) == 5) ? m_sTitle : static_cast<CGString>(GetTradeTitle());
			break;
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

bool CChar::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CChar::r_LoadVal");
	EXC_TRY("LoadVal");

	LPCTSTR	pszKey = s.GetKey();
	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	if ( index < 0 )
	{
	do_default:
		if ( m_pPlayer && m_pPlayer->r_LoadVal(this, s) )
			return true;
		if ( m_pNPC && m_pNPC->r_LoadVal(this, s) )
			return true;

		// Stats
		STAT_TYPE stat = g_Cfg.FindStatKey(pszKey);
		if ( stat != STAT_NONE )
		{
			Stat_SetBase(stat, static_cast<int>(s.GetArgVal()));
			return true;
		}
		if ( !strnicmp(pszKey, "O", 1) )
		{
			stat = g_Cfg.FindStatKey(pszKey + 1);
			if ( stat != STAT_NONE )
			{
				Stat_SetBase(stat, static_cast<int>(s.GetArgVal()));
				return true;
			}
		}
		if ( !strnicmp(pszKey, "MOD", 3) )
		{
			stat = g_Cfg.FindStatKey(pszKey + 3);
			if ( stat != STAT_NONE )
			{
				Stat_SetMod(stat, static_cast<int>(s.GetArgVal()));
				return true;
			}
		}

		// Skills
		SKILL_TYPE skill = g_Cfg.FindSkillKey(pszKey);
		if ( skill != SKILL_NONE )
		{
			Skill_SetBase(skill, static_cast<WORD>(s.GetArgVal()));
			return true;
		}

		return CObjBase::r_LoadVal(s);
	}

	switch ( index )
	{
		case CHC_REGENFOOD:
		case CHC_REGENVALFOOD:
		case CHC_REGENVALHITS:
		case CHC_REGENVALSTAM:
		case CHC_REGENVALMANA:
			SetDefNum(s.GetKey(), s.GetArgVal(), false);
			UpdateStatsFlag();
			break;
		// Set as numeric
		case CHC_SPELLTIMEOUT:
			SetDefNum(s.GetKey(), s.GetArgVal(), false);
			break;
		case CHC_BLOODCOLOR:
			m_wBloodHue = static_cast<HUE_TYPE>(s.GetArgVal());
			break;
		case CHC_MAXFOOD:
			Stat_SetMax(STAT_FOOD, static_cast<int>(s.GetArgVal()));
			break;
		case CHC_MAXHITS:
			Stat_SetMax(STAT_STR, static_cast<int>(s.GetArgVal()));
			break;
		case CHC_MAXMANA:
			Stat_SetMax(STAT_INT, static_cast<int>(s.GetArgVal()));
			break;
		case CHC_MAXSTAM:
			Stat_SetMax(STAT_DEX, static_cast<int>(s.GetArgVal()));
			break;
		case CHC_ACCOUNT:
			return SetPlayerAccount(s.GetArgStr());
		case CHC_ACT:
			m_Act_Targ = s.GetArgVal();
			break;
		case CHC_ACTP:
		{
			if ( s.HasArgs() )
				m_Act_p.Read(s.GetArgStr());
			else
				m_Act_p = GetTopPoint();
			break;
		}
		case CHC_ACTPRV:
			m_Act_TargPrv = static_cast<CGrayUID>(s.GetArgVal());
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
			return Skill_Start(g_Cfg.FindSkillKey(s.GetArgStr()));
		case CHC_ATTACKER:
		{
			if ( strlen(pszKey) > 8 )
			{
				pszKey += 8;
				int id = m_lastAttackers.size();
				if ( *pszKey == '.' )
				{
					++pszKey;
					if ( !strnicmp(pszKey, "ADD", 3) )
					{
						CChar *pChar = static_cast<CGrayUID>(s.GetArgVal()).CharFind();
						if ( !pChar )
							return false;
						Fight_Attack(pChar, true);
						return true;
					}
					else if ( !strnicmp(pszKey, "DELETE", 6) )
					{
						CChar *pChar = static_cast<CGrayUID>(s.GetArgVal()).CharFind();
						if ( !pChar )
							return false;
						Attacker_Delete(pChar, false, ATTACKER_CLEAR_SCRIPT);
						return true;
					}
					else if ( !strnicmp(pszKey, "TARGET", 6) )
					{
						CChar *pChar = static_cast<CGrayUID>(s.GetArgVal()).CharFind();
						if ( !pChar || (pChar == this) )	// can't set ourself as target
							return false;
						m_Fight_Targ = pChar->GetUID();
						return true;
					}
					else if ( !strnicmp(pszKey, "CLEAR", 5) )
					{
						if ( m_lastAttackers.size() )
							Fight_Clear();
						return true;
					}

					id = Exp_GetVal(pszKey);
					if ( id < 0 )
						return false;

					SKIP_SEPARATORS(pszKey);
					if ( id < static_cast<int>(m_lastAttackers.size()) )
					{
						if ( !strnicmp(pszKey, "ELAPSED", 7) )
						{
							Attacker_SetElapsed(id, s.GetArgLLVal());
							return true;
						}
						else if ( !strnicmp(pszKey, "DAM", 3) )
						{
							Attacker_SetDamage(id, s.GetArgLLVal());
							return true;
						}
						else if ( !strnicmp(pszKey, "THREAT", 6) )
						{
							Attacker_SetThreat(id, s.GetArgLLVal());
							return true;
						}
						else if ( !strnicmp(pszKey, "DELETE", 6) )
						{
							Attacker_Delete(id, false, ATTACKER_CLEAR_SCRIPT);
							return true;
						}
					}
				}
			}
			return false;
		}
		case CHC_BODY:
			SetID(static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType(RES_CHARDEF, s.GetArgStr())));
			break;
		case CHC_BREATH:
		{
			if ( !strnicmp(pszKey, "BREATH.DAM", 10) || !strnicmp(pszKey, "BREATH.HUE", 10) || !strnicmp(pszKey, "BREATH.ANIM", 11) || !strnicmp(pszKey, "BREATH.TYPE", 11) )
			{
				SetDefNum(s.GetKey(), s.GetArgLLVal());
				return true;
			}
			return false;
		}
		case CHC_CREATE:
			m_timeCreate = CServTime::GetCurrentTime() - (s.GetArgLLVal() * TICK_PER_SEC);
			break;
		case CHC_DIR:
		{
			DIR_TYPE dir = static_cast<DIR_TYPE>(s.GetArgVal());
			if ( (dir <= DIR_INVALID) || (dir >= DIR_QTY) )
				dir = DIR_SE;
			m_dirFace = dir;
			UpdateDir(dir);
			break;
		}
		case CHC_DISMOUNT:
			Horse_UnMount();
			break;
		case CHC_EMOTEACT:
		{
			bool fSet = IsStatFlag(STATF_EmoteAction);
			if ( s.HasArgs() )
				fSet = s.GetArgVal() ? true : false;
			else
				fSet = !fSet;
			StatFlag_Mod(STATF_EmoteAction, fSet);
			break;
		}
		case CHC_FLAGS:
		{
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
		}
		case CHC_FOLLOWERSLOTS:
			m_FollowerSlots = static_cast<short>(s.GetArgVal());
			break;
		case CHC_CURFOLLOWER:
			m_FollowerCur = static_cast<short>(s.GetArgVal());
			UpdateStatsFlag();
			break;
		case CHC_MAXFOLLOWER:
			m_FollowerMax = static_cast<short>(s.GetArgVal());
			UpdateStatsFlag();
			break;
		case CHC_TITHING:
			m_Tithing = static_cast<int>(s.GetArgVal());
			UpdateStatsFlag();
			break;
		case CHC_FONT:
		{
			m_fonttype = static_cast<FONT_TYPE>(s.GetArgVal());
			if ( (m_fonttype < FONT_BOLD) || (m_fonttype >= FONT_QTY) )
				m_fonttype = FONT_NORMAL;
			break;
		}
		case CHC_INCREASEDAM:
			m_DamIncrease = static_cast<int>(s.GetArgVal());
			UpdateStatsFlag();
			break;
		case CHC_INCREASEDEFCHANCE:
			m_DefChanceIncrease = static_cast<int>(s.GetArgVal());
			UpdateStatsFlag();
			break;
		case CHC_INCREASEDEFCHANCEMAX:
			m_DefChanceIncreaseMax = static_cast<int>(s.GetArgVal());
			UpdateStatsFlag();
			break;
		case CHC_INCREASEHITCHANCE:
			m_HitChanceIncrease = static_cast<int>(s.GetArgVal());
			UpdateStatsFlag();
			break;
		case CHC_INCREASESPELLDAM:
			m_SpellDamIncrease = static_cast<int>(s.GetArgVal());
			UpdateStatsFlag();
			break;
		case CHC_INCREASESWINGSPEED:
			m_SwingSpeedIncrease = static_cast<int>(s.GetArgVal());
			UpdateStatsFlag();
			break;
		case CHC_FASTERCASTING:
			m_FasterCasting = static_cast<int>(s.GetArgVal());
			UpdateStatsFlag();
			break;
		case CHC_FASTERCASTRECOVERY:
			m_FasterCastRecovery = static_cast<int>(s.GetArgVal());
			UpdateStatsFlag();
			break;
		case CHC_HITLEECHLIFE:
			m_HitLifeLeech = static_cast<int>(s.GetArgVal());
			break;
		case CHC_HITLEECHMANA:
			m_HitManaLeech = static_cast<int>(s.GetArgVal());
			break;
		case CHC_HITLEECHSTAM:
			m_HitStaminaLeech = static_cast<int>(s.GetArgVal());
			break;
		case CHC_HITMANADRAIN:
			m_HitManaDrain = static_cast<int>(s.GetArgVal());
			break;
		case CHC_LOWERMANACOST:
			m_LowerManaCost = static_cast<int>(s.GetArgVal());
			UpdateStatsFlag();
			break;
		case CHC_LOWERREAGENTCOST:
			m_LowerReagentCost = static_cast<int>(s.GetArgVal());
			UpdateStatsFlag();
			break;
		case CHC_SPEECHCOLOR:
		{
			if ( m_pPlayer )	// read-only on players
				return false;
			m_SpeechHue = static_cast<HUE_TYPE>(s.GetArgVal());
			break;
		}
		case CHC_FOOD:
			Stat_SetVal(STAT_FOOD, static_cast<int>(s.GetArgVal()));
			break;
		case CHC_GOLD:
		{
			DWORD dwAmount = s.GetArgVal();
			if ( dwAmount <= 0 )
				dwAmount = 0;

			DWORD dwCurrentGold = ContentCount(RESOURCE_ID(RES_TYPEDEF, IT_GOLD));
			DWORD dwNewGold = dwAmount;
			if ( dwNewGold < dwCurrentGold )
			{
				ContentConsume(RESOURCE_ID(RES_TYPEDEF, IT_GOLD), dwCurrentGold - dwNewGold);
			}
			else if ( dwNewGold > dwCurrentGold )
			{
				CItemContainer *pBank = GetContainerCreate(LAYER_BANKBOX);
				if ( !pBank )
					return false;
				AddGoldToPack(dwNewGold - dwCurrentGold, pBank, false);
			}
			UpdateStatsFlag();
			break;
		}
		case CHC_HITPOINTS:
		case CHC_HITS:
			Stat_SetVal(STAT_STR, static_cast<int>(s.GetArgVal()));
			UpdateHitsFlag();
			break;
		case CHC_MANA:
			Stat_SetVal(STAT_INT, static_cast<int>(s.GetArgVal()));
			UpdateManaFlag();
			break;
		case CHC_STAM:
		case CHC_STAMINA:
			Stat_SetVal(STAT_DEX, static_cast<int>(s.GetArgVal()));
			UpdateStamFlag();
			break;
		case CHC_STEPSTEALTH:
			m_StepStealth = static_cast<int>(s.GetArgVal());
			break;
		case CHC_HEIGHT:
			m_height = static_cast<height_t>(s.GetArgVal());
			break;
		case CHC_HOME:
		{
			if ( s.HasArgs() )
				m_ptHome.Read(s.GetArgStr());
			else
				m_ptHome = GetTopPoint();
			break;
		}
		case CHC_NAME:
		{
			if ( IsTrigUsed(TRIGGER_RENAME) )
			{
				CScriptTriggerArgs args;
				args.m_s1 = s.GetArgStr();
				args.m_pO1 = this;
				if ( OnTrigger(CTRIG_Rename, this, &args) == TRIGRET_RET_TRUE )
					return false;

				SetName(args.m_s1);
			}
			else
				SetName(s.GetArgStr());
			UpdateStatsFlag();
			break;
		}
		case CHC_FAME:
		case CHC_KARMA:
			goto do_default;
		case CHC_SKILLUSEQUICK:
		{
			if ( s.GetArgStr() )
			{
				TCHAR *ppArgs[2];
				size_t iArgQty = Str_ParseCmds(const_cast<TCHAR *>(s.GetArgStr()), ppArgs, COUNTOF(ppArgs));
				if ( iArgQty == 2 )
				{
					SKILL_TYPE skill = g_Cfg.FindSkillKey(ppArgs[0]);
					if ( skill == SKILL_NONE )
						return false;

					Skill_UseQuick(skill, Exp_GetVal(ppArgs[1]));
					return true;
				}
			}
			return false;
		}
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
			break;
		}
		case CHC_NIGHTSIGHT:
		{
			bool fSet;
			bool fChange = IsStatFlag(STATF_NightSight);
			if ( s.HasArgs() )
			{
				fSet = s.GetArgVal() ? true : false;
				fChange = (fSet != fChange);
			}
			else
			{
				fSet = !fChange;
				fChange = true;
			}
			SetDefNum(s.GetKey(), fSet, false);
			StatFlag_Mod(STATF_NightSight, fSet);
			if ( m_pClient )
			{
				m_pClient->addLight();
				if ( fSet )
					m_pClient->addBuff(BI_NIGHTSIGHT, 1075643, 1075644);
				else
					m_pClient->removeBuff(BI_NIGHTSIGHT);
			}
			break;
		}
		case CHC_NPC:
			return SetNPCBrain(static_cast<NPCBRAIN_TYPE>(s.GetArgVal()));
		case CHC_OBODY:
		{
			CREID_TYPE id = static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType(RES_CHARDEF, s.GetArgStr()));
			if ( !CCharBase::FindCharBase(id) )
			{
				DEBUG_ERR(("OBODY Invalid Char 0%x\n", id));
				return false;
			}
			m_prev_id = id;
			break;
		}
		case CHC_OSKIN:
			m_prev_Hue = static_cast<HUE_TYPE>(s.GetArgVal());
			break;
		case CHC_P:
		{
			CPointMap pt;
			pt.Read(s.GetArgStr());
			MoveTo(pt);
			break;
		}
		case CHC_REGENHITS:
			SetDefNum(s.GetKey(), s.GetArgVal(), false);
			UpdateRegenTimers(STAT_STR, static_cast<WORD>(s.GetArgVal()));
			break;
		case CHC_REGENMANA:
			SetDefNum(s.GetKey(), s.GetArgVal(), false);
			UpdateRegenTimers(STAT_INT, static_cast<WORD>(s.GetArgVal()));
			break;
		case CHC_REGENSTAM:
			SetDefNum(s.GetKey(), s.GetArgVal(), false);
			UpdateRegenTimers(STAT_DEX, static_cast<WORD>(s.GetArgVal()));
			break;
		case CHC_STONE:
		{
			bool fSet;
			bool fChange = IsStatFlag(STATF_Stone);
			if ( s.HasArgs() )
			{
				fSet = s.GetArgVal() ? true : false;
				fChange = (fSet != fChange);
			}
			else
			{
				fSet = !fChange;
				fChange = true;
			}
			StatFlag_Mod(STATF_Stone, fSet);
			if ( fChange )
			{
				Update();
				if ( m_pClient )
					m_pClient->addCharMove(this);
			}
			break;
		}
		case CHC_SWING:
		{
			if ( s.GetArgVal() && ((s.GetArgVal() < WAR_SWING_INVALID) || (s.GetArgVal() > WAR_SWING_SWINGING)) )
				return false;
			m_atFight.m_Swing_State = static_cast<WAR_SWING_TYPE>(s.GetArgVal());
			break;
		}
		case CHC_TITLE:
			m_sTitle = s.GetArgStr();
			break;
		case CHC_LIGHT:
			m_LocalLight = static_cast<BYTE>(minimum(maximum(s.GetArgVal(), LIGHT_BRIGHT), LIGHT_DARK));
			break;
		case CHC_EXP:
			m_exp = s.GetArgVal();
			ChangeExperience();
			break;
		case CHC_LEVEL:
			m_level = s.GetArgVal();
			ChangeExperience();
			break;
		case CHC_VIRTUALGOLD:
			m_virtualGold = static_cast<UINT64>(s.GetArgLLVal());
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

void CChar::r_Write(CScript &s)
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

	if ( m_height )
		s.WriteKeyVal("HEIGHT", m_height);
	if ( m_ptHome.IsValidPoint() )
		s.WriteKey("HOME", m_ptHome.WriteUsed());
	if ( m_StepStealth )
		s.WriteKeyVal("STEPSTEALTH", m_StepStealth);
	if ( m_Tithing )
		s.WriteKeyVal("TITHING", m_Tithing);

	CCharBase *pCharDef = Char_GetDef();
	ASSERT(pCharDef);

	if ( m_FollowerSlots != pCharDef->m_FollowerSlots )
		s.WriteKeyVal("FOLLOWERSLOTS", m_FollowerSlots);
	if ( m_FollowerCur )
		s.WriteKeyVal("CURFOLLOWER", m_FollowerCur);
	if ( m_FollowerMax != pCharDef->m_FollowerMax )
		s.WriteKeyVal("MAXFOLLOWER", m_FollowerMax);

	TCHAR szTmp[8];
	for ( size_t i = 0; i < STAT_QTY; ++i )
	{
		// Save MOD first (this is VERY important)
		if ( Stat_GetMod(static_cast<STAT_TYPE>(i)) )
		{
			sprintf(szTmp, "MOD%s", g_Stat_Name[i]);
			s.WriteKeyVal(szTmp, Stat_GetMod(static_cast<STAT_TYPE>(i)));
		}
		if ( Stat_GetBase(static_cast<STAT_TYPE>(i)) )
		{
			sprintf(szTmp, "O%s", g_Stat_Name[i]);
			s.WriteKeyVal(szTmp, Stat_GetBase(static_cast<STAT_TYPE>(i)));
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

	for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; ++i )
	{
		if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(i) || (m_Skill[i] == 0) )
			continue;
		s.WriteKeyVal(g_Cfg.GetSkillDef(static_cast<SKILL_TYPE>(i))->GetKey(), m_Skill[i]);
	}

	r_WriteContent(s);
	EXC_CATCH;
}

void CChar::r_WriteParity(CScript &s)
{
	ADDTOCALLSTACK("CChar::r_WriteParity");
	// Overload virtual for world save

	if ( g_World.m_fSaveParity == IsStatFlag(STATF_SaveParity) )
		return;		// already saved

	StatFlag_Mod(STATF_SaveParity, g_World.m_fSaveParity);
	if ( IsWeird() )
		return;
	r_WriteSafe(s);
}

bool CChar::r_Load(CScript &s)	// load a character from script
{
	ADDTOCALLSTACK("CChar::r_Load");
	CScriptObj::r_Load(s);

	if ( m_pNPC )
		NPC_GetAllSpellbookSpells();

	// Init STATF_SaveParity flag
	//StatFlag_Mod(STATF_SaveParity, g_World.m_fSaveParity);

	// Make sure everything is ok
	if ( (m_pPlayer && !m_pClient) || (m_pNPC && IsStatFlag(STATF_Ridden)) )
		SetDisconnected();

	int iResultCode = CObjBase::IsWeird();
	if ( iResultCode )
	{
		DEBUG_ERR(("Char 0%" FMTDWORDH " Invalid, id='%s', code=0%x\n", static_cast<DWORD>(GetUID()), GetResourceName(), iResultCode));
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

const LPCTSTR CChar::sm_szVerbKeys[CHV_QTY + 1] =
{
	#define ADD(a,b) b,
	#include "../tables/CChar_functions.tbl"
	#undef ADD
	NULL
};

bool CChar::r_Verb(CScript &s, CTextConsole *pSrc)	// execute command from script
{
	ADDTOCALLSTACK("CChar::r_Verb");
	if ( !pSrc )
		return false;
	if ( m_pClient && m_pClient->r_Verb(s, pSrc) )
		return true;

	EXC_TRY("Verb");

	int index = FindTableSorted(s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1);
	if ( index < 0 )
		return (NPC_OnVerb(s, pSrc) || Player_OnVerb(s, pSrc) || CObjBase::r_Verb(s, pSrc));

	CChar *pCharSrc = pSrc->GetChar();

	switch ( index )
	{
		case CHV_ALLSKILLS:
		{
			WORD wVal = static_cast<WORD>(s.GetArgVal());
			for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; ++i )
			{
				if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(i) )
					continue;
				Skill_SetBase(static_cast<SKILL_TYPE>(i), wVal);
			}
			break;
		}
		case CHV_ANIM:
		{
			INT64 piCmd[3];		// maximum parameters in one line
			size_t iArgQty = Str_ParseCmds(s.GetArgRaw(), piCmd, COUNTOF(piCmd));
			return UpdateAnimate(static_cast<ANIM_TYPE>(piCmd[0]), true, false, (iArgQty > 1) ? static_cast<BYTE>(piCmd[1]) : 1, (iArgQty > 2) ? static_cast<BYTE>(piCmd[2]) : 1);
		}
		case CHV_ATTACK:
			Fight_Attack(static_cast<CGrayUID>(s.GetArgVal()).CharFind(), true);
			break;
		case CHV_BANK:
		{
			if ( !pCharSrc || !pCharSrc->m_pClient )
				return false;
			pCharSrc->m_pClient->addBankOpen(this, s.HasArgs() ? static_cast<LAYER_TYPE>(s.GetArgVal()) : LAYER_BANKBOX);
			break;
		}
		case CHV_BARK:
		{
			CRESND_TYPE id = CRESND_RAND;
			if ( s.HasArgs() )
			{
				id = static_cast<CRESND_TYPE>(s.GetArgVal());
				if ( (id < CRESND_RAND) || (id > CRESND_DIE) )
				{
					DEBUG_ERR(("Invalid Bark args '%d'\n", static_cast<int>(id)));
					return false;
				}
			}
			SoundChar(id);
			break;
		}
		case CHV_BOUNCE:
			return ItemBounce(static_cast<CGrayUID>(s.GetArgVal()).ItemFind());
		case CHV_BOW:
			UpdateDir(static_cast<CGrayUID>(s.GetArgVal()).ObjFind());
			UpdateAnimate(ANIM_BOW);
			break;
		case CHV_CONTROL:
		{
			if ( !pCharSrc || !pCharSrc->m_pClient )
				return false;
			return pCharSrc->m_pClient->Cmd_Control(this);
		}
		case CHV_CONSUME:
		{
			CResourceQtyArray Resources;
			Resources.Load(s.GetArgStr());
			ResourceConsume(&Resources, 1, false);
			break;
		}
		case CHV_CRIMINAL:
		{
			if ( s.HasArgs() && !s.GetArgVal() )
			{
				CItem *pMemory = LayerFind(LAYER_FLAG_Criminal);
				if ( pMemory )
				{
					// Removing criminal memory will already clear flag, noto and buff
					pMemory->Delete();
				}
				else
				{
					// Otherwise clear it manually if there's no memory set
					StatFlag_Clear(STATF_Criminal);
					NotoSave_Update();
					if ( m_pClient )
					{
						m_pClient->removeBuff(BI_CRIMINALSTATUS);
						if ( !(g_Cfg.m_fGuardsOnMurderers && Noto_IsEvil()) )
							SysMessageDefault(DEFMSG_MSG_GUARDS_NOLONGER);
					}
				}
			}
			else
				Noto_Criminal();
			break;
		}
		case CHV_DISCONNECT:
		{
			if ( m_pClient )
				m_pClient->addKick(pSrc, false);
			else
				SetDisconnected();
			break;
		}
		case CHV_DROP:
			return ItemDrop(static_cast<CGrayUID>(s.GetArgVal()).ItemFind(), GetTopPoint());
		case CHV_DUPE:
		{
			CChar *pChar = CreateNPC(GetID());
			ASSERT(pChar);
			pChar->MoveTo(GetTopPoint());
			pChar->DupeFrom(this, (s.GetArgVal() < 1) ? true : false);
			break;
		}
		case CHV_EQUIP:
			return ItemEquip(static_cast<CGrayUID>(s.GetArgVal()).ItemFind());
		case CHV_EQUIPHALO:
		{
			CItem *pItem = CItem::CreateScript(ITEMID_LIGHT_SRC, this);
			ASSERT(pItem);
			if ( s.HasArgs() )
			{
				pItem->SetTimeout(s.GetArgLLVal());
				pItem->Item_GetDef()->m_ttNormal.m_tData4 = 0;
			}
			pItem->SetAttr(ATTR_MOVE_NEVER);
			LayerAdd(pItem, LAYER_HAND2);
			return true;
		}
		case CHV_EQUIPARMOR:
			return ItemEquipArmor(false);
		case CHV_EQUIPWEAPON:
			return ItemEquipWeapon(false);
		case CHV_FACE:
			UpdateDir(static_cast<CGrayUID>(s.GetArgVal()).ObjFind());
			break;
		case CHV_FIXWEIGHT:
			FixWeight();
			break;
		case CHV_FORGIVE:
			Jail(pSrc, false, 0);
			break;
		case CHV_GOCHAR:
			return TeleportToObj(1, s.GetArgStr());
		case CHV_GOCHARID:
			return TeleportToObj(3, s.GetArgStr());
		case CHV_GOCLI:
			return TeleportToCli(1, s.GetArgVal());
		case CHV_GOITEMID:
			return TeleportToObj(4, s.GetArgStr());
		case CHV_GONAME:
			return TeleportToObj(0, s.GetArgStr());
		case CHV_GO:
			return Spell_Teleport(g_Cfg.GetRegionPoint(s.GetArgStr()), true, false);
		case CHV_GOSOCK:
			return TeleportToCli(0, s.GetArgVal());
		case CHV_GOTYPE:
			return TeleportToObj(2, s.GetArgStr());
		case CHV_GOUID:
		{
			if ( s.HasArgs() )
			{
				CObjBaseTemplate *pObj = static_cast<CGrayUID>(s.GetArgVal()).ObjFind();
				if ( !pObj )
					return false;
				pObj = pObj->GetTopLevelObj();
				Spell_Teleport(pObj->GetTopPoint(), true, false);
				return true;
			}
			return false;
		}
		case CHV_HEAR:
		{
			if ( m_pPlayer )
				SysMessage(s.GetArgStr());
			else
				NPC_OnHear(s.GetArgStr(), pSrc->GetChar());
			break;
		}
		case CHV_HUNGRY:
		{
			if ( pCharSrc )
			{
				char *z = Str_GetTemp();
				if ( pCharSrc == this )
					sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_SELF), Food_GetLevelMessage());
				else
					sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_OTHER), GetName(), Food_GetLevelMessage());
				pCharSrc->ObjMessage(z, this);
			}
			break;
		}
		case CHV_INVIS:
		{
			if ( pSrc )
			{
				m_StatFlag = s.GetArgFlag(m_StatFlag, STATF_Insubstantial);
				Update();
				if ( IsStatFlag(STATF_Insubstantial) )
				{
					if ( m_pClient )
						m_pClient->addBuff(BI_HIDDEN, 1075655, 1075656);
					if ( IsSetOF(OF_Command_Sysmsgs) )
						pSrc->SysMessagef("Invis %s", g_Cfg.GetDefaultMsg(DEFMSG_CMD_TOGGLE_ON));
				}
				else
				{
					if ( m_pClient && !IsStatFlag(STATF_Hidden) )
						m_pClient->removeBuff(BI_HIDDEN);
					if ( IsSetOF(OF_Command_Sysmsgs) )
						pSrc->SysMessagef("Invis %s", g_Cfg.GetDefaultMsg(DEFMSG_CMD_TOGGLE_OFF));
				}
			}
			break;
		}
		case CHV_INVUL:
		{
			if ( pSrc )
			{
				m_StatFlag = s.GetArgFlag(m_StatFlag, STATF_INVUL);
				NotoSave_Update();
				if ( IsSetOF(OF_Command_Sysmsgs) )
					pSrc->SysMessagef("Invul %s", g_Cfg.GetDefaultMsg(IsStatFlag(STATF_INVUL) ? DEFMSG_CMD_TOGGLE_ON : DEFMSG_CMD_TOGGLE_OFF));
			}
			break;
		}
		case CHV_JAIL:
			Jail(pSrc, true, s.GetArgVal());
			break;
		case CHV_KILL:
		{
			Effect(EFFECT_LIGHTNING, ITEMID_NOTHING, pCharSrc);
			OnTakeDamage(Stat_GetVal(STAT_STR), pCharSrc, DAMAGE_GOD);
			break;
		}
		case CHV_MAKEITEM:
		{
			TCHAR *pszTmp = Str_GetTemp();
			strncpy(pszTmp, s.GetArgRaw(), MAX_ITEM_NAME_SIZE);
			GETNONWHITESPACE(pszTmp);

			WORD wReplicationQty = 1;

			TCHAR *ppArgs[2];
			size_t iArgQty = Str_ParseCmds(pszTmp, ppArgs, COUNTOF(ppArgs), " ,\t");
			if ( iArgQty == 2 )
			{
				if ( IsDigit(ppArgs[1][0]) )
					wReplicationQty = static_cast<WORD>(ATOI(ppArgs[1]));
			}

			if ( m_pClient )
				m_Act_Targ = m_pClient->m_Targ_UID;

			return Skill_MakeItem(static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType(RES_ITEMDEF, ppArgs[0])), m_Act_Targ, SKTRIG_START, false, wReplicationQty);
		}
		case CHV_MOUNT:
		{
			CChar *pChar = static_cast<CGrayUID>(s.GetArgVal()).CharFind();
			if ( pChar )
				Horse_Mount(pChar);
			break;
		}
		case CHV_NEWBIESKILL:
		{
			CResourceLock t;
			if ( !g_Cfg.ResourceLock(t, RES_NEWBIE, s.GetArgStr()) )
				return false;
			ReadScript(t);
			break;
		}
		case CHV_NEWGOLD:
		{
			long lAmount = s.GetArgVal();
			if ( lAmount > 0 )
			{
				AddGoldToPack(static_cast<DWORD>(lAmount), NULL, false);
				UpdateStatsFlag();
			}
			break;
		}
		case CHV_NEWLOOT:
		{
			if ( m_pNPC && !IsStatFlag(STATF_Conjured) )
			{
				CItem *pItem = CItem::CreateHeader(s.GetArgStr(), NULL, false, this);
				if ( pItem )
				{
					ItemEquip(pItem);
					g_World.m_uidNew = pItem->GetUID();
				}
				else
					g_World.m_uidNew = static_cast<CGrayUID>(UID_CLEAR);
			}
			break;
		}
		case CHV_NOTOCLEAR:
			NotoSave_Clear();
			break;
		case CHV_NOTOUPDATE:
			NotoSave_Update();
			break;
		case CHV_PACK:
		{
			if ( !pCharSrc || !pCharSrc->m_pClient )
				return false;
			pCharSrc->m_pClient->addBankOpen(this, LAYER_PACK);
			break;
		}
		case CHV_POISON:
		{
			int iSkill = static_cast<int>(s.GetArgVal());
			int iTicks = iSkill / 50;
			INT64 piCmd[2];
			if ( Str_ParseCmds(s.GetArgRaw(), piCmd, COUNTOF(piCmd)) > 1 )
				iTicks = static_cast<int>(piCmd[1]);

			SetPoison(iSkill, iTicks, pSrc->GetChar());
			break;
		}
		case CHV_POLY:
		{
			CSpellDef *pSpellDef = g_Cfg.GetSpellDef(SPELL_Polymorph);
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
			return SetPrivLevel(pSrc, s.GetArgStr());
		case CHV_RELEASE:
			NPC_PetRelease();
			break;
		case CHV_REMOVE:	// remove this char from the world instantly
		case CHV_DESTROY:	// remove this char from the world and bypass trigger return value
		{
			if ( m_pPlayer )
			{
				if ( (s.GetArgRaw()[0] != '1') || (pSrc->GetPrivLevel() < PLEVEL_Admin) )
				{
					pSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_CMD_REMOVE_PLAYER));
					return false;
				}
			}
			Delete((index == CHV_DESTROY));
			break;
		}
		case CHV_RESURRECT:
		{
			if ( s.GetArgVal() )
				return Spell_Resurrection(NULL, pCharSrc, true);
			else
				return OnSpellEffect(SPELL_Resurrection, pCharSrc, 1000, NULL);
		}
		case CHV_REVEAL:
			Reveal(static_cast<DWORD>(s.GetArgVal()));
			break;
		case CHV_SALUTE:
		{
			UpdateDir(static_cast<CGrayUID>(s.GetArgVal()).ObjFind());
			UpdateAnimate(ANIM_SALUTE);
			break;
		}
		case CHV_SKILL:
			Skill_Start(g_Cfg.FindSkillKey(s.GetArgStr()));
			break;
		case CHV_SKILLGAIN:
		{
			if ( s.HasArgs() )
			{
				TCHAR *ppArgs[2];
				if ( Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs)) > 0 )
				{
					SKILL_TYPE skill = g_Cfg.FindSkillKey(ppArgs[0]);
					if ( skill == SKILL_NONE )
						return false;
					Skill_Experience(skill, Exp_GetVal(ppArgs[1]));
				}
			}
			return true;
		}
		case CHV_SUICIDE:
		{
			m_lastAttackers.clear();	// clear the list of people who get credit for your death
			Memory_ClearTypes(MEMORY_FIGHT);
			UpdateAnimate(ANIM_SALUTE);
			Stat_SetVal(STAT_STR, 0);
			break;
		}
		case CHV_SUMMONCAGE:
		{
			if ( pCharSrc )
			{
				ITEMID_TYPE id = static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType(RES_ITEMDEF, "i_multi_cage"));
				if ( id <= ITEMID_NOTHING )
					return false;
				CItemMulti *pItem = static_cast<CItemMulti *>(CItem::CreateBase(id));
				if ( !pItem )
					return false;
				CPointMap pt = pCharSrc->GetTopPoint();
				pt.MoveN(pCharSrc->m_dirFace, 3);
				pItem->MoveToDecay(pt, 10 * 60 * TICK_PER_SEC);		// make the cage vanish after 10 minutes
				pItem->Multi_Create(NULL);
				Spell_Teleport(pt, true, false);
				break;
			}
			return false;
		}
		case CHV_SUMMONTO:
		{
			if ( pCharSrc )
				Spell_Teleport(pCharSrc->GetTopPoint(), true, false);
			break;
		}
		case CHV_SMSG:
		case CHV_SMSGL:
		case CHV_SMSGLEX:
		case CHV_SMSGU:
		case CHV_SYSMESSAGE:
		case CHV_SYSMESSAGELOC:
		case CHV_SYSMESSAGELOCEX:
		case CHV_SYSMESSAGEUA:
			break;		// just eat this if it's not a client
		case CHV_TARGETCLOSE:
		{
			if ( m_pClient )
				m_pClient->addTargetCancel();
			break;
		}
		case CHV_UNDERWEAR:
		{
			if ( !IsPlayableCharacter() )
				return false;
			SetHue(GetHue() ^ HUE_MASK_UNDERWEAR);
			Update(false);
			break;
		}
		case CHV_UNEQUIP:
			return ItemBounce(static_cast<CGrayUID>(s.GetArgVal()).ItemFind());
		case CHV_WHERE:
		{
			if ( pCharSrc )
			{
				char *z = Str_GetTemp();
				if ( m_pArea )
				{
					if ( m_pArea->GetResourceID().IsItem() )
						sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_MSG_WHERE_AREA), m_pArea->GetName(), GetTopPoint().WriteUsed());
					else
					{
						const CRegionBase *pRoom = GetTopPoint().GetRegion(REGION_TYPE_ROOM);
						if ( pRoom )
							sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_MSG_WHERE_ROOM), m_pArea->GetName(), pRoom->GetName(), GetTopPoint().WriteUsed());
						else
							sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_MSG_WHERE_AREA), m_pArea->GetName(), GetTopPoint().WriteUsed());
					}
				}
				else
					sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_MSG_WHERE), GetTopPoint().WriteUsed());
				pCharSrc->ObjMessage(z, this);
			}
			break;
		}
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

bool CChar::OnTriggerSpeech(bool fPet, LPCTSTR pszText, CChar *pSrc, TALKMODE_TYPE &mode, HUE_TYPE wHue)
{
	ADDTOCALLSTACK("CChar::OnTriggerSpeech");

	LPCTSTR pszName = NULL;
	if ( fPet && !g_Cfg.m_sSpeechPet.IsEmpty() )
		pszName = g_Cfg.m_sSpeechPet;
	else if ( !fPet && !g_Cfg.m_sSpeechSelf.IsEmpty() )
		pszName = g_Cfg.m_sSpeechSelf;

	if ( pszName )
	{
		CScriptObj *pDef = g_Cfg.ResourceGetDefByName(RES_SPEECH, pszName);
		if ( pDef )
		{
			CResourceLink *pLink = dynamic_cast<CResourceLink *>(pDef);
			if ( pLink )
			{
				CResourceLock s;
				if ( pLink->ResourceLock(s) && pLink->HasTrigger(XTRIG_UNKNOWN) )
				{
					TRIGRET_TYPE iRet = OnHearTrigger(s, pszText, pSrc, mode, wHue);
					if ( iRet == TRIGRET_RET_TRUE )
						return true;
					else if ( iRet == TRIGRET_RET_HALFBAKED )
						return false;
				}
				else
					DEBUG_ERR(("Couldn't run script for speech %s\n", pszName));
			}
			else
				DEBUG_ERR(("Couldn't find speech %s\n", pszName));
		}
		else
			DEBUG_ERR(("Couldn't find speech resource %s\n", pszName));
	}

	if ( fPet || !m_pPlayer )
		return false;

	if ( m_pPlayer->m_Speech.GetCount() > 0 )
	{
		for ( size_t i = 0; i < m_pPlayer->m_Speech.GetCount(); ++i )
		{
			CResourceLink *pLinkDSpeech = m_pPlayer->m_Speech[i];
			if ( !pLinkDSpeech )
				continue;

			CResourceLock sDSpeech;
			if ( !pLinkDSpeech->ResourceLock(sDSpeech) )
				continue;

			TRIGRET_TYPE iRet = OnHearTrigger(sDSpeech, pszText, pSrc, mode, wHue);
			if ( iRet == TRIGRET_RET_TRUE )
				return true;
			else if ( iRet == TRIGRET_RET_HALFBAKED )
				break;
		}
	}
	return false;
}

// Increase EXP
unsigned int Calc_ExpGet_Exp(unsigned int level)
{
	unsigned int exp = 0;
	switch ( g_Cfg.m_iLevelMode )
	{
		case LEVEL_MODE_LINEAR:
		{
			exp = g_Cfg.m_iLevelNextAt * maximum(1, level);
			break;
		}
		case LEVEL_MODE_DOUBLE:
		default:
		{
			for ( unsigned int i = 1; i <= level; ++i )
				exp += g_Cfg.m_iLevelNextAt * (i + 1);
			break;
		}
	}
	return exp;
}

// Increase LEVEL
unsigned int Calc_ExpGet_Level(unsigned int exp)
{
	unsigned int level = 0;		// current level
	unsigned int req = g_Cfg.m_iLevelNextAt;	// required exp for next level

	if ( req < 1 )	// must do this check in case ini's LevelNextAt is not set or server will freeze because exp will never decrease in the while
		return 0;

	while ( exp >= req )
	{
		// Reduce exp and raise level
		exp -= req;
		++level;

		// Calculate requirement for next level
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

void CChar::ChangeExperience(int iDelta, CChar *pCharDead)
{
	ADDTOCALLSTACK("CChar::ChangeExperience");
	if ( !g_Cfg.m_iExperienceMode )
		return;

	if ( (iDelta != 0) || pCharDead )	// zero call will sync the exp level
	{
		if ( iDelta < 0 )
		{
			if ( !(g_Cfg.m_iExperienceMode & EXP_MODE_ALLOW_DOWN) )
				return;
			
			if ( g_Cfg.m_bLevelSystem && (g_Cfg.m_iExperienceMode & EXP_MODE_DOWN_NOLEVEL) )
			{
				unsigned int exp = Calc_ExpGet_Exp(m_level);
				if ( m_exp + iDelta < exp )
					iDelta = m_exp - exp;
			}
		}

#ifdef _DEBUG
		if ( g_Cfg.m_wDebugFlags & DEBUGF_EXP )
			DEBUG_ERR(("%s %s experience change (was %u, delta %d, now %u)\n", m_pNPC ? "NPC" : "Player", GetName(), m_exp, iDelta, m_exp + iDelta));
#endif

		bool fShowMsg = (m_pClient != NULL);

		if ( IsTrigUsed(TRIGGER_EXPCHANGE) )
		{
			CScriptTriggerArgs args(iDelta, fShowMsg);
			args.m_pO1 = pCharDead;
			if ( OnTrigger(CTRIG_ExpChange, this, &args) == TRIGRET_RET_TRUE )
				return;
			iDelta = static_cast<int>(args.m_iN1);
			fShowMsg = (args.m_iN2 != 0);
		}

		// Do not allow an underflow due to negative exp change
		if ( (iDelta < 0) && (abs(iDelta) > static_cast<int>(m_exp)) )
			m_exp = 0;
		else
			m_exp += iDelta;

		if ( m_pClient && fShowMsg && iDelta )
		{
			static const LPCTSTR sm_szExpDelta[] =
			{
				g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_CHANGE_1),
				g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_CHANGE_2),
				g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_CHANGE_3),
				g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_CHANGE_4),
				g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_CHANGE_5),
				g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_CHANGE_6),
				g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_CHANGE_7),
				g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_CHANGE_8)
			};

			int iVal = abs(iDelta);
			int iMaxVal = (g_Cfg.m_bLevelSystem && g_Cfg.m_iLevelNextAt) ? maximum(g_Cfg.m_iLevelNextAt, 1000) : 1000;
			int iDegree = 0;

			if ( iVal >= iMaxVal )					// 100%
				iDegree = 7;
			else if ( iVal >= (iMaxVal * 2) / 3 )	// 66%
				iDegree = 6;
			else if ( iVal >= iMaxVal / 2 )			// 50%
				iDegree = 5;
			else if ( iVal >= iMaxVal / 3 )			// 33%
				iDegree = 4;
			else if ( iVal >= iMaxVal / 5 )			// 20%
				iDegree = 3;
			else if ( iVal >= iMaxVal / 7 )			// 14%
				iDegree = 2;
			else if ( iVal >= iMaxVal / 14 )		// 7%
				iDegree = 1;
			m_pClient->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_CHANGE_0), g_Cfg.GetDefaultMsg((iDelta > 0) ? DEFMSG_MSG_EXP_CHANGE_GAIN : DEFMSG_MSG_EXP_CHANGE_LOST), sm_szExpDelta[iDegree]);
		}
	}

	if ( g_Cfg.m_bLevelSystem )
	{
		unsigned int level = Calc_ExpGet_Level(m_exp);
		if ( level != m_level )
		{
			iDelta = level - m_level;
			bool fShowMsg = (m_pClient != NULL);

			if ( IsTrigUsed(TRIGGER_EXPLEVELCHANGE) )
			{
				CScriptTriggerArgs args(iDelta);
				if ( OnTrigger(CTRIG_ExpLevelChange, this, &args) == TRIGRET_RET_TRUE )
					return;
				iDelta = static_cast<int>(args.m_iN1);
				fShowMsg = (args.m_iN2 != 0);
			}

			// Prevent integer underflow due to negative level change
			level = m_level + iDelta;
			if ( (iDelta < 0) && (abs(iDelta) > static_cast<int>(m_level)) )
				level = 0;

#ifdef _DEBUG
			if ( g_Cfg.m_wDebugFlags & DEBUGF_LEVEL )
				DEBUG_ERR(("%s %s level change (was %u, delta %d, now %u)\n", m_pNPC ? "NPC" : "Player", GetName(), m_level, iDelta, level));
#endif

			m_level = level;

			if ( m_pClient && fShowMsg )
				m_pClient->SysMessagef(g_Cfg.GetDefaultMsg((abs(iDelta) == 1) ? DEFMSG_MSG_EXP_LVLCHANGE_0 : DEFMSG_MSG_EXP_LVLCHANGE_1), g_Cfg.GetDefaultMsg((iDelta > 0) ? DEFMSG_MSG_EXP_LVLCHANGE_GAIN : DEFMSG_MSG_EXP_LVLCHANGE_LOST));
		}
	}
}

DWORD CChar::GetSkillTotal(int iWhat, bool fHow)
{
	ADDTOCALLSTACK("CChar::GetSkillTotal");
	DWORD dwTotal = 0;
	WORD wBase = 0;

	for ( unsigned int i = 0; i < g_Cfg.m_iMaxSkill; ++i )
	{
		wBase = Skill_GetBase(static_cast<SKILL_TYPE>(i));
		if ( fHow )
		{
			if ( iWhat < 0 )
			{
				if ( wBase >= -iWhat )
					continue;
			}
			else if ( wBase < iWhat )
				continue;
		}
		else
		{
			const CSkillDef *pSkill = g_Cfg.GetSkillDef(static_cast<SKILL_TYPE>(i));
			if ( !pSkill )
				continue;
			if ( !(pSkill->m_dwGroup & iWhat) )
				continue;
		}
		dwTotal += wBase;
	}
	return dwTotal;
}
