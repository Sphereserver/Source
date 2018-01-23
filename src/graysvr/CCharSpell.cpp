//
// CCharSpell.cpp
//

#include "graysvr.h"	// predef header.
#include "../network/send.h"

void CChar::Spell_Dispel(int iLevel)
{
	ADDTOCALLSTACK("CChar::Spell_Dispel");
	// ARGS: iLevel = 0-100 level of dispel caster.
	// remove all the spells. NOT if caused by objects worn !!!
	// ATTR_MAGIC && !ATTR_MOVE_NEVER

	CItem *pItemNext = NULL;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( (iLevel <= 100) && pItem->IsAttr(ATTR_MOVE_NEVER) )	// we don't lose this.
			continue;
		if ( ((pItem->GetEquipLayer() >= LAYER_SPELL_STATS) && (pItem->GetEquipLayer() <= LAYER_SPELL_Summon)) )
			pItem->Delete();
	}
}

bool CChar::Spell_Teleport(CPointMap ptNew, bool bTakePets, bool bCheckAntiMagic, bool bDisplayEffect, ITEMID_TYPE iEffect, SOUND_TYPE iSound)
{
	ADDTOCALLSTACK("CChar::Spell_Teleport");
	// Teleport you to this place.
	// This is sometimes not really a spell at all.
	// ex. ships plank.
	// RETURN: true = it worked.

	if ( !ptNew.IsValidPoint() )
		return false;

	ptNew.m_z = GetFixZ(ptNew);

	if ( g_Cfg.m_iMountHeight )
	{
		if ( !IsVerticalSpace(ptNew, false) )
		{
			SysMessageDefault(DEFMSG_MSG_MOUNT_CEILING);
			return false;
		}
	}

	if ( IsPriv(PRIV_JAILED) )
	{
		CRegionBase *pJail = g_Cfg.GetRegion("jail");
		if ( !pJail || !pJail->IsInside2d(ptNew) )
		{
			// Must be /PARDONed to leave jail area
			static LPCTSTR const sm_szPunishMsg[] =
			{
				g_Cfg.GetDefaultMsg(DEFMSG_SPELL_TELE_JAILED_1),
				g_Cfg.GetDefaultMsg(DEFMSG_SPELL_TELE_JAILED_2)
			};
			SysMessage(sm_szPunishMsg[Calc_GetRandVal(COUNTOF(sm_szPunishMsg))]);

			int iCell = 0;
			if ( m_pPlayer && m_pPlayer->m_pAccount )
				iCell = static_cast<int>(m_pPlayer->m_pAccount->m_TagDefs.GetKeyNum("JailCell"));

			if ( iCell )
			{
				TCHAR szJailName[128];
				sprintf(szJailName, "jail%d", iCell);
				pJail = g_Cfg.GetRegion(szJailName);
			}

			if ( pJail )
				ptNew = pJail->m_pt;
			else
				ptNew.InitPoint();
		}
	}

	// Is it a valid teleport location that allows this ?
	if ( bCheckAntiMagic && !IsPriv(PRIV_GM) )
	{
		CRegionBase *pArea = CheckValidMove(ptNew, NULL, DIR_QTY, NULL);
		if ( !pArea )
		{
			SysMessageDefault(DEFMSG_SPELL_TELE_CANT);
			return false;
		}
		if ( pArea->IsFlag(REGION_ANTIMAGIC_RECALL_IN|REGION_ANTIMAGIC_TELEPORT) )
		{
			SysMessageDefault(DEFMSG_SPELL_TELE_AM);
			return false;
		}
	}

	if ( bDisplayEffect && !IsStatFlag(STATF_Insubstantial) && (iEffect == ITEMID_NOTHING) && (iSound == SOUND_NONE) )
	{
		if ( m_pPlayer )
		{
			if ( IsPriv(PRIV_GM) && !IsPriv(PRIV_PRIV_NOSHOW) && !IsStatFlag(STATF_Incognito) )
			{
				iEffect = g_Cfg.m_iSpell_Teleport_Effect_Staff;
				iSound = g_Cfg.m_iSpell_Teleport_Sound_Staff;
			}
			else
			{
				iEffect = g_Cfg.m_iSpell_Teleport_Effect_Players;
				iSound = g_Cfg.m_iSpell_Teleport_Sound_Players;
			}
		}
		else
		{
			iEffect = g_Cfg.m_iSpell_Teleport_Effect_NPC;
			iSound = g_Cfg.m_iSpell_Teleport_Sound_NPC;
		}
	}

	CPointMap ptOld = GetTopPoint();
	if ( ptOld.IsValidPoint() )		// guards might have just been created
	{
		if ( bTakePets )	// look for any creatures that might be following me nearby
		{
			CWorldSearch Area(ptOld, UO_MAP_VIEW_SIGHT);
			for (;;)
			{
				CChar *pChar = Area.GetChar();
				if ( !pChar )
					break;
				if ( pChar == this )
					continue;

				if ( (pChar->Skill_GetActive() == NPCACT_FOLLOW_TARG) && (pChar->m_Act_Targ == GetUID()) )	// my pet?
				{
					if ( pChar->CanMoveWalkTo(ptOld, false, true) )
						pChar->Spell_Teleport(ptNew, bTakePets, bCheckAntiMagic, bDisplayEffect, iEffect, iSound);
				}
			}
		}
	}

	MoveTo(ptNew);		// move character

	CClient *pClientIgnore = NULL;
	if ( m_pClient && (ptNew.m_map != ptOld.m_map) )
	{
		m_pClient->addReSync();
		pClientIgnore = m_pClient;	// we don't need update this client again
	}

	UpdateMove(ptOld, pClientIgnore, true);
	Reveal();

	if ( bDisplayEffect )
	{
		if ( iEffect != ITEMID_NOTHING )
		{
			// Departing effect
			if ( ptOld.IsValidPoint() )
			{
				CItem *pItem = CItem::CreateBase(ITEMID_NODRAW);
				ASSERT(pItem);
				pItem->SetAttr(ATTR_MOVE_NEVER);
				pItem->MoveTo(ptOld);
				pItem->Effect(EFFECT_XYZ, iEffect, this, 10, 10);
				pItem->Delete();
			}

			// Entering effect
			Effect(EFFECT_XYZ, iEffect, this, 10, 10);
		}
		if ( iSound != SOUND_NONE )
			Sound(iSound);
	}

	return true;
}

bool CChar::Spell_CreateGate(CPointMap ptNew, bool bCheckAntiMagic)
{
	ADDTOCALLSTACK("CChar::Spell_CreateGate");
	// Create moongate between current pt and destination pt
	// RETURN: true = it worked.

	if ( !ptNew.IsValidPoint() )
		return false;

	ptNew.m_z = GetFixZ(ptNew);

	if ( IsPriv(PRIV_JAILED) )
	{
		// Must be /PARDONed to leave jail area
		static LPCTSTR const sm_szPunishMsg[] =
		{
			g_Cfg.GetDefaultMsg(DEFMSG_SPELL_TELE_JAILED_1),
			g_Cfg.GetDefaultMsg(DEFMSG_SPELL_TELE_JAILED_2)
		};
		SysMessage(sm_szPunishMsg[Calc_GetRandVal(COUNTOF(sm_szPunishMsg))]);
		return false;
	}

	CRegionBase *pAreaDest = ptNew.GetRegion(REGION_TYPE_AREA|REGION_TYPE_ROOM|REGION_TYPE_MULTI);
	if ( !pAreaDest )
		return false;

	if ( pAreaDest->IsFlag(REGION_FLAG_SHIP) )
	{
		SysMessageDefault(DEFMSG_SPELL_GATE_SOMETHINGBLOCKING);
		return false;
	}

	if ( bCheckAntiMagic )
	{
		if ( pAreaDest->IsFlag(REGION_ANTIMAGIC_ALL|REGION_ANTIMAGIC_GATE) )
		{
			SysMessageDefault(DEFMSG_SPELL_GATE_AM);
			if ( !IsPriv(PRIV_GM) )
				return false;
		}
	}

	if ( g_World.IsItemTypeNear(GetTopPoint(), IT_TELEPAD, 0, false, true) || g_World.IsItemTypeNear(ptNew, IT_TELEPAD, 0, false, true) )
	{
		SysMessageDefault(DEFMSG_SPELL_GATE_ALREADYTHERE);
		if ( !IsPriv(PRIV_GM) )
			return false;
	}

	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(SPELL_Gate_Travel);
	ASSERT(pSpellDef);
	int iDuration = pSpellDef->m_Duration.GetLinear(0);

	CItem *pGate = CItem::CreateBase(pSpellDef->m_idEffect);
	ASSERT(pGate);
	pGate->m_uidLink = GetUID();
	pGate->SetType(IT_TELEPAD);
	pGate->SetAttr(ATTR_MAGIC|ATTR_MOVE_NEVER|ATTR_CAN_DECAY);
	pGate->SetHue(static_cast<HUE_TYPE>(pAreaDest->IsGuarded() ? HUE_DEFAULT : HUE_RED));
	pGate->m_itTelepad.m_pntMark = ptNew;
	pGate->MoveToDecay(GetTopPoint(), iDuration);
	pGate->Sound(pSpellDef->m_sound);

	pGate = CItem::CreateDupeItem(pGate);
	ASSERT(pGate);
	pGate->SetHue(static_cast<HUE_TYPE>((m_pArea && m_pArea->IsGuarded()) ? HUE_DEFAULT : HUE_RED));
	pGate->m_itTelepad.m_pntMark = GetTopPoint();
	pGate->MoveToDecay(ptNew, iDuration);
	pGate->Sound(pSpellDef->m_sound);

	SysMessageDefault(DEFMSG_SPELL_GATE_OPEN);
	return true;
}

CChar *CChar::Spell_Summon(CREID_TYPE id, CPointMap pntTarg)
{
	ADDTOCALLSTACK("CChar::Spell_Summon");
	// Summon an NPC using summon spells.

	int skill;
	CSpellDef *pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_Spell);
	if ( !pSpellDef || !pSpellDef->GetPrimarySkill(&skill, NULL) )
		return NULL;

	if ( !pSpellDef->IsSpellType(SPELLFLAG_TARG_OBJ|SPELLFLAG_TARG_XYZ) )
		pntTarg = GetTopPoint();

	CChar *pChar = CChar::CreateBasic(id);
	if ( !pChar )
		return NULL;

	if ( !IsPriv(PRIV_GM) )
	{
		if ( IsSetMagicFlags(MAGICF_SUMMONWALKCHECK) )	// check if the target location is valid
		{
			CCharBase *pSummonDef = CCharBase::FindCharBase(id);
			WORD wCan = 0xFFFF;
			if ( pSummonDef )
				wCan = pSummonDef->m_Can & CAN_C_MOVEMASK;

			if ( wCan != 0xFFFF )
			{
				DWORD wBlockFlags = 0;
				g_World.GetHeightPoint2(pntTarg, wBlockFlags, true);

				if ( wBlockFlags & ~wCan )
				{
					SysMessageDefault(DEFMSG_MSG_SUMMON_INVALIDTARG);
					pChar->Delete();
					return NULL;
				}
			}
		}

		if ( IsSetOF(OF_PetSlots) )
		{
			if ( !FollowersUpdate(pChar, static_cast<short>(maximum(1, pChar->GetDefNum("FollowerSlots", true))), true) )
			{
				SysMessageDefault(DEFMSG_PETSLOTS_TRY_SUMMON);
				pChar->Delete();
				return NULL;
			}
		}

		if ( g_Cfg.m_iMountHeight && !pChar->IsVerticalSpace(GetTopPoint(), false) )
		{
			SysMessageDefault(DEFMSG_MSG_SUMMON_CEILING);
			pChar->Delete();
			return NULL;
		}
	}

	pChar->StatFlag_Set(STATF_Conjured);	// conjured creates have no loot
	pChar->NPC_LoadScript(false);
	pChar->MoveToChar(pntTarg);
	pChar->m_ptHome = pntTarg;
	pChar->m_pNPC->m_Home_Dist_Wander = 10;
	pChar->NPC_CreateTrigger();		// removed from NPC_LoadScript() and triggered after char placement
	pChar->NPC_PetSetOwner(this, false);
	pChar->OnSpellEffect(SPELL_Summon, this, Skill_GetAdjusted(static_cast<SKILL_TYPE>(skill)), NULL);

	pChar->Update();
	pChar->UpdateAnimate(ANIM_CAST_DIR);
	pChar->SoundChar(CRESND_GETHIT);
	m_Act_Targ = pChar->GetUID();	// for last target stuff
	return pChar;
}

bool CChar::Spell_Recall(CItem *pTarg, bool bGate)
{
	ADDTOCALLSTACK("CChar::Spell_Recall");
	if ( !pTarg )
		return false;

	if ( IsTrigUsed(TRIGGER_SPELLEFFECT) || IsTrigUsed(TRIGGER_ITEMSPELL) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = bGate ? SPELL_Gate_Travel : SPELL_Recall;

		if ( pTarg->OnTrigger(ITRIG_SPELLEFFECT, this, &Args) == TRIGRET_RET_FALSE )
			return true;
	}

	if ( pTarg->IsType(IT_RUNE) )
	{
		if ( !pTarg->m_itRune.m_pntMark.IsValidPoint() )
		{
			SysMessageDefault(DEFMSG_SPELL_RECALL_RUNENOTMARKED);
			return false;
		}
		else if ( pTarg->m_itRune.m_Charges <= 0 )
		{
			SysMessageDefault(DEFMSG_SPELL_RECALL_NOCHARGES);
			if ( !IsPriv(PRIV_GM) )
				return false;
		}

		if ( bGate )
		{
			if ( !Spell_CreateGate(pTarg->m_itRune.m_pntMark) )
				return false;
		}
		else
		{
			const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(SPELL_Recall);
			ASSERT(pSpellDef);
			if ( !Spell_Teleport(pTarg->m_itRune.m_pntMark, true, true, true, pSpellDef->m_idEffect, pSpellDef->m_sound) )
				return false;
		}

		if ( !IsPriv(PRIV_GM) )
		{
			pTarg->m_itRune.m_Charges--;
			pTarg->ResendTooltip();
		}
		return true;
	}
	else if ( pTarg->IsType(IT_KEY) )
	{
		CItemShip *pBoat = dynamic_cast<CItemShip *>(pTarg->m_uidLink.ItemFind());
		if ( pBoat )
		{
			if ( bGate )
			{
				SysMessageDefault(DEFMSG_SPELL_GATE_SOMETHINGBLOCKING);
				return false;
			}
			else
			{
				CItemContainer *pHatch = pBoat->GetShipHold();
				if ( !pHatch || !Spell_Teleport(pHatch->GetTopPoint(), true) )
					return false;
			}
			return true;
		}
	}

	SysMessageDefault(DEFMSG_SPELL_RECALL_CANTRECALLOBJ);
	return false;
}

bool CChar::Spell_Resurrection(CItemCorpse *pCorpse, CChar *pCharSrc, bool bNoFail)
{
	ADDTOCALLSTACK("CChar::Spell_Resurrection");
	if ( !IsStatFlag(STATF_DEAD) )
		return false;

	if ( IsPriv(PRIV_GM) || (pCharSrc && pCharSrc->IsPriv(PRIV_GM)) )
		bNoFail = true;

	if ( !bNoFail && m_pArea && m_pArea->IsFlag(REGION_ANTIMAGIC_ALL) )
	{
		SysMessageDefault(DEFMSG_SPELL_RES_AM);
		return false;
	}

	short hits = IMULDIV(Stat_GetMax(STAT_STR), static_cast<short>(g_Cfg.m_iHitpointPercentOnRez), 100);
	if ( !pCorpse )
		pCorpse = FindMyCorpse();

	if ( IsTrigUsed(TRIGGER_RESURRECT) )
	{
		CScriptTriggerArgs Args(hits, 0, pCorpse);
		if ( OnTrigger(CTRIG_Resurrect, pCharSrc, &Args) == TRIGRET_RET_TRUE )
			return false;
		hits = static_cast<short>(Args.m_iN1);
	}

	SetID(m_prev_id);
	SetHue(m_prev_Hue);
	StatFlag_Clear(STATF_DEAD|STATF_Insubstantial);
	Stat_SetVal(STAT_STR, maximum(hits, 1));

	if ( m_pNPC && m_pNPC->m_bonded )
		m_Can &= ~CAN_C_GHOST;

	ClientIterator it;
	for ( CClient* pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( !pClient->CanSee(this) )
			continue;

		pClient->addChar(this);
		if ( m_pNPC )
			pClient->addBondedStatus(this, false);
	}

	if ( m_pClient )
	{
		m_pClient->addPlayerView(NULL, g_Cfg.m_fDeadCannotSeeLiving ? true : false);
		CSector *pSector = GetTopPoint().GetSector();
		if ( pSector )
			m_pClient->addSeason(pSector->GetSeason());
	}

	bool bRaisedCorpse = false;
	if ( pCorpse )
	{
		if ( RaiseCorpse(pCorpse) )
		{
			SysMessageDefault(DEFMSG_SPELL_RES_REJOIN);
			bRaisedCorpse = true;
		}
	}

	if ( m_pPlayer )
	{
		CItem *pDeathShroud = ContentFind(RESOURCE_ID(RES_ITEMDEF, ITEMID_DEATHSHROUD));
		if ( pDeathShroud )
			pDeathShroud->Delete();

		if ( !bRaisedCorpse && !g_Cfg.m_fNoResRobe )
		{
			CItem *pRobe = CItem::CreateBase(ITEMID_ROBE);
			ASSERT(pRobe);
			pRobe->SetName(g_Cfg.GetDefaultMsg(DEFMSG_SPELL_RES_ROBENAME));
			LayerAdd(pRobe, LAYER_ROBE);
		}

	}

	CSpellDef *pSpellDef = g_Cfg.GetSpellDef(SPELL_Resurrection);
	if ( pSpellDef->m_idEffect )
		Effect(EFFECT_OBJ, pSpellDef->m_idEffect, this, 10, 16);
	Sound(pSpellDef->m_sound);
	return true;
}

void CChar::Spell_Effect_Remove(CItem *pSpell)
{
	ADDTOCALLSTACK("CChar::Spell_Effect_Remove");
	// we are removing the spell effect.
	// equipped wands do not confer effect.
	if ( !pSpell || !pSpell->IsTypeSpellable() || pSpell->IsType(IT_WAND) )
		return;

	SPELL_TYPE spell = static_cast<SPELL_TYPE>(RES_GET_INDEX(pSpell->m_itSpell.m_spell));
	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
	if ( !spell || !pSpellDef )
		return;

	short iStatEffect = static_cast<short>(pSpell->m_itSpell.m_spelllevel);

	switch ( pSpellDef->m_idLayer )	// spell effects that are common for the same layer fits here
	{
		case LAYER_NONE:
			break;
		case LAYER_FLAG_Poison:
		{
			StatFlag_Clear(STATF_Poisoned);
			UpdateModeFlag();
			if ( m_pClient )
				m_pClient->removeBuff(BI_POISON);
			return;
		}
		case LAYER_SPELL_Summon:
		{
			if ( m_pPlayer )	// summoned players ? thats odd.
				return;
			if ( !g_Serv.IsLoading() )
			{
				Effect(EFFECT_XYZ, ITEMID_FX_TELE_VANISH, this, 10, 15);
				Sound(SOUND_TELEPORT);
			}
			if ( !IsStatFlag(STATF_DEAD) )	// fix for double @Destroy trigger
				Delete();
			return;
		}
		case LAYER_SPELL_Polymorph:
		{
			SetHue(m_prev_Hue);
			SetID(m_prev_id);

			BUFF_ICONS iBuffIcon = BI_POLYMORPH;
			switch ( spell )
			{
				case SPELL_Horrific_Beast:
					iBuffIcon = BI_HORRIFICBEAST;
					SetDefNum("RegenHits", GetDefNum("RegenHits") - pSpell->m_itSpell.m_PolyStr);
					SetDefNum("IncreaseDam", GetDefNum("IncreaseDam") - pSpell->m_itSpell.m_PolyDex);
					m_attackBase -= static_cast<WORD>(pSpell->m_itSpell.m_spellcharges);
					m_attackRange -= pSpell->m_itSpell.m_spelllevel;
					break;
				case SPELL_Lich_Form:
					iBuffIcon = BI_LICHFORM;
					SetDefNum("RegenHits", GetDefNum("RegenHits") + pSpell->m_itSpell.m_PolyStr);
					SetDefNum("RegenMana", GetDefNum("RegenMana") - pSpell->m_itSpell.m_PolyDex);
					SetDefNum("ResFire", GetDefNum("ResFire") + pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResPoison", GetDefNum("ResPoison") - pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResCold", GetDefNum("ResCold") - pSpell->m_itSpell.m_spellcharges);
					break;
				case SPELL_Vampiric_Embrace:
					iBuffIcon = BI_VAMPIRICEMBRACE;
					SetDefNum("HitLeechLife", GetDefNum("HitLeechLife") - pSpell->m_itSpell.m_PolyStr);
					SetDefNum("RegenStam", GetDefNum("RegenStam") - pSpell->m_itSpell.m_PolyDex);
					SetDefNum("RegenMana", GetDefNum("RegenMana") - pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResFire", GetDefNum("ResFire") + pSpell->m_itSpell.m_spelllevel);
					break;
				case SPELL_Wraith_Form:
					iBuffIcon = BI_WRAITHFORM;
					SetDefNum("ResPhysical", GetDefNum("ResPhysical") - pSpell->m_itSpell.m_PolyStr);
					SetDefNum("ResEnergy", GetDefNum("ResEnergy") + pSpell->m_itSpell.m_PolyDex);
					SetDefNum("ResFire", GetDefNum("ResFire") + pSpell->m_itSpell.m_PolyDex);
					SetDefNum("HitLeechMana", GetDefNum("HitLeechMana") - pSpell->m_itSpell.m_spellcharges);
					break;
				case SPELL_Reaper_Form:
					iBuffIcon = BI_REAPERFORM;
					SetDefNum("IncreaseSwingSpeed", GetDefNum("IncreaseSwingSpeed") - pSpell->m_itSpell.m_PolyStr);
					SetDefNum("IncreaseSpellDam", GetDefNum("IncreaseSpellDam") - pSpell->m_itSpell.m_PolyStr);
					SetDefNum("ResPhysical", GetDefNum("ResPhysical") - pSpell->m_itSpell.m_PolyDex);
					SetDefNum("ResCold", GetDefNum("ResCold") - pSpell->m_itSpell.m_PolyDex);
					SetDefNum("ResPoison", GetDefNum("ResPoison") - pSpell->m_itSpell.m_PolyDex);
					SetDefNum("ResEnergy", GetDefNum("ResEnergy") - pSpell->m_itSpell.m_PolyDex);
					SetDefNum("ResFire", GetDefNum("ResFire") + pSpell->m_itSpell.m_spellcharges);
					break;
				case SPELL_Stone_Form:
					iBuffIcon = BI_STONEFORM;
					SetDefNum("IncreaseSwingSpeed", GetDefNum("IncreaseSwingSpeed") + pSpell->m_itSpell.m_PolyStr);
					SetDefNum("FasterCasting", GetDefNum("FasterCasting") + pSpell->m_itSpell.m_PolyDex);
					SetDefNum("ResPhysical", GetDefNum("ResPhysical") - pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResFire", GetDefNum("ResFire") - pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResCold", GetDefNum("ResCold") - pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResPoison", GetDefNum("ResPoison") - pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResEnergy", GetDefNum("ResEnergy") - pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResPhysicalMax", GetDefNum("ResPhysicalMax") - pSpell->m_itSpell.m_spelllevel);
					SetDefNum("ResFireMax", GetDefNum("ResFireMax") - pSpell->m_itSpell.m_spelllevel);
					SetDefNum("ResColdMax", GetDefNum("ResColdMax") - pSpell->m_itSpell.m_spelllevel);
					SetDefNum("ResPoisonMax", GetDefNum("ResPoisonMax") - pSpell->m_itSpell.m_spelllevel);
					SetDefNum("ResEnergyMax", GetDefNum("ResEnergyMax") - pSpell->m_itSpell.m_spelllevel);
					SetDefNum("IncreaseDam", GetDefNum("IncreaseDam") - pSpell->m_itSpell.m_PolyStr);
					break;
				default:
					break;
			}
			if ( (spell == SPELL_Polymorph) && IsSetMagicFlags(MAGICF_POLYMORPHSTATS) )
			{
				Stat_AddMod(STAT_STR, -pSpell->m_itSpell.m_PolyStr);
				Stat_AddMod(STAT_DEX, -pSpell->m_itSpell.m_PolyDex);
				Stat_SetVal(STAT_STR, minimum(Stat_GetVal(STAT_STR), Stat_GetMax(STAT_STR)));
				Stat_SetVal(STAT_DEX, minimum(Stat_GetVal(STAT_DEX), Stat_GetMax(STAT_DEX)));
			}
			StatFlag_Clear(STATF_Polymorph);
			if ( m_pClient )
				m_pClient->removeBuff(iBuffIcon);
			return;
		}
		case LAYER_SPELL_Night_Sight:
		{
			StatFlag_Clear(STATF_NightSight);
			if ( m_pClient )
			{
				m_pClient->addLight();
				m_pClient->removeBuff(BI_NIGHTSIGHT);
			}
			return;
		}
		case LAYER_SPELL_Incognito:
		{
			StatFlag_Clear(STATF_Incognito);
			SetName(pSpell->GetName());		// restore your name

			if ( !IsStatFlag(STATF_Polymorph) && IsPlayableCharacter() )
				SetHue(m_prev_Hue);

			CItem *pHair = LayerFind(LAYER_HAIR);
			if ( pHair )
				pHair->SetHue(static_cast<HUE_TYPE>(pSpell->GetTagDefs()->GetKeyNum("COLOR.HAIR")));

			CItem *pBeard = LayerFind(LAYER_BEARD);
			if ( pBeard )
				pBeard->SetHue(static_cast<HUE_TYPE>(pSpell->GetTagDefs()->GetKeyNum("COLOR.BEARD")));

			if ( m_pClient )
				m_pClient->removeBuff(BI_INCOGNITO);
			NotoSave_Update();
			return;
		}
		case LAYER_SPELL_Invis:
		{
			Reveal(STATF_Invisible);
			return;
		}
		case LAYER_SPELL_Paralyze:
		{
			StatFlag_Clear(STATF_Freeze);
			UpdateMode();	// immediately tell the client that now he's able to move (without this, it will be able to move only on next tick update)
			if ( m_pClient )
				m_pClient->removeBuff(BI_PARALYZE);
			return;
		}
		case LAYER_SPELL_Strangle:	// TO-DO: NumBuff[0] and NumBuff[1] to hold the damage range values.
		{
			if ( m_pClient )
				m_pClient->removeBuff(BI_STRANGLE);
			return;
		}
		case LAYER_SPELL_Gift_Of_Renewal:
		{
			if ( m_pClient )
				m_pClient->removeBuff(BI_GIFTOFRENEWAL);
			return;
		}
		case LAYER_SPELL_Attunement:
		{
			if ( m_pClient )
				m_pClient->removeBuff(BI_ATTUNEWEAPON);
			return;
		}
		case LAYER_SPELL_Thunderstorm:
		{
			if ( m_pClient )
				m_pClient->removeBuff(BI_THUNDERSTORM);
			return;
		}
		case LAYER_SPELL_Essence_Of_Wind:
		{
			if ( m_pClient )
				m_pClient->removeBuff(BI_ESSENCEOFWIND);
			return;
		}
		case LAYER_SPELL_Ethereal_Voyage:
		{
			if ( m_pClient )
				m_pClient->removeBuff(BI_ETHEREALVOYAGE);
			return;
		}
		case LAYER_SPELL_Gift_Of_Life:
		{
			if ( m_pClient )
				m_pClient->removeBuff(BI_GIFTOFLIFE);
			return;
		}
		case LAYER_SPELL_Arcane_Empowerment:
		{
			if ( m_pClient )
				m_pClient->removeBuff(BI_ARCANEEMPOWERMENT);
			return;
		}
		/*case LAYER_Mortal_Strike:
		{
			if ( m_pClient )
				m_pClient->removeBuff(BI_MORTALSTRIKE);
			return;
		}*/
		case LAYER_SPELL_Blood_Oath:
		{
			if ( m_pClient )
				m_pClient->removeBuff(BI_BLOODOATHCURSE);
			CChar *pSrc = pSpell->m_uidLink.CharFind();
			if ( pSrc && pSrc->m_pClient )
				pSrc->m_pClient->removeBuff(BI_BLOODOATHCASTER);
			return;
		}
		case LAYER_SPELL_Corpse_Skin:
		{
			SetDefNum("ResPhysical", GetDefNum("ResPhysical") - pSpell->m_itSpell.m_PolyStr, true);
			SetDefNum("ResFire", GetDefNum("ResFire") + pSpell->m_itSpell.m_PolyDex, true);
			SetDefNum("ResCold", GetDefNum("ResCold") - pSpell->m_itSpell.m_PolyStr, true);
			SetDefNum("ResPoison", GetDefNum("ResPoison") + pSpell->m_itSpell.m_PolyDex, true);
			if ( m_pClient )
				m_pClient->removeBuff(BI_CORPSESKIN);
			return;
		}
		case LAYER_SPELL_Pain_Spike:
		{
			if ( m_pClient )
				m_pClient->removeBuff(BI_PAINSPIKE);
			return;
		}
		default:
			break;
	}

	switch ( spell )	// the rest of the effects are handled directly by each spell
	{
		case SPELL_Clumsy:
		{
			Stat_AddMod(STAT_DEX, iStatEffect);
			if ( m_pClient )
				m_pClient->removeBuff(BI_CLUMSY);
			return;
		}
		case SPELL_Particle_Form:	// 112 // turns you into an immobile, but untargetable particle system for a while.
		case SPELL_Stone:
		{
			StatFlag_Clear(STATF_Stone);
			UpdateModeFlag();
			return;
		}
		case SPELL_Hallucination:
		{
			StatFlag_Clear(STATF_Hallucinating);
			UpdateModeFlag();
			if ( m_pClient )
			{
				m_pClient->addChar(this);
				m_pClient->addPlayerSee(NULL);
			}
			return;
		}
		case SPELL_Feeblemind:
		{
			Stat_AddMod(STAT_INT, iStatEffect);
			if ( m_pClient )
				m_pClient->removeBuff(BI_FEEBLEMIND);
			return;
		}
		case SPELL_Weaken:
		{
			Stat_AddMod(STAT_STR, iStatEffect);
			if ( m_pClient )
				m_pClient->removeBuff(BI_WEAKEN);
			return;
		}
		case SPELL_Curse:
		case SPELL_Mass_Curse:
		{
			if ( m_pPlayer && (spell == SPELL_Curse) )
			{
				SetDefNum("ResFireMax", GetDefNum("ResFireMax", true) + 10);
				SetDefNum("ResColdMax", GetDefNum("ResColdMax", true) + 10);
				SetDefNum("ResPoisonMax", GetDefNum("ResPoisonMax", true) + 10);
				SetDefNum("ResEnergyMax", GetDefNum("ResEnergyMax", true) + 10);
			}
			for ( int i = STAT_STR; i < STAT_BASE_QTY; i++ )
				Stat_AddMod(static_cast<STAT_TYPE>(i), iStatEffect);
			if ( m_pClient )
			{
				if ( spell == SPELL_Curse )
					m_pClient->removeBuff(BI_CURSE);
				else
					m_pClient->removeBuff(BI_MASSCURSE);
			}
			return;
		}
		case SPELL_Agility:
		{
			Stat_AddMod(STAT_DEX, -iStatEffect);
			if ( m_pClient )
				m_pClient->removeBuff(BI_AGILITY);
			return;
		}
		case SPELL_Cunning:
		{
			Stat_AddMod(STAT_INT, -iStatEffect);
			if ( m_pClient )
				m_pClient->removeBuff(BI_CUNNING);
			return;
		}
		case SPELL_Strength:
		{
			Stat_AddMod(STAT_STR, -iStatEffect);
			if ( m_pClient )
				m_pClient->removeBuff(BI_STRENGTH);
			return;
		}
		case SPELL_Bless:
		{
			for ( int i = STAT_STR; i < STAT_BASE_QTY; i++ )
				Stat_AddMod(static_cast<STAT_TYPE>(i), -iStatEffect);
			if ( m_pClient )
				m_pClient->removeBuff(BI_BLESS);
			return;
		}
		case SPELL_Mana_Drain:
		{
			UpdateStatVal(STAT_INT, +iStatEffect);
			Effect(EFFECT_OBJ, ITEMID_FX_SPARKLE_2, this, 10, 25);
			Sound(0x28E);
			return;
		}
		case SPELL_Reactive_Armor:
		{
			if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
			{
				SetDefNum("ResPhysical", GetDefNum("ResPhysical") - pSpell->m_itSpell.m_spelllevel);
				SetDefNum("ResFire", GetDefNum("ResFire") + 5);
				SetDefNum("ResCold", GetDefNum("ResCold") + 5);
				SetDefNum("ResPoison", GetDefNum("ResPoison") + 5);
				SetDefNum("ResEnergy", GetDefNum("ResEnergy") + 5);
			}
			else
			{
				StatFlag_Clear(STATF_Reactive);
			}
			if ( m_pClient )
				m_pClient->removeBuff(BI_REACTIVEARMOR);
			return;
		}
		case SPELL_Magic_Reflect:
		{
			StatFlag_Clear(STATF_Reflection);
			if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
			{
				SetDefNum("ResPhysical", GetDefNum("ResPhysical") + pSpell->m_itSpell.m_spelllevel);
				SetDefNum("ResFire", GetDefNum("ResFire") - 10);
				SetDefNum("ResCold", GetDefNum("ResCold") - 10);
				SetDefNum("ResPoison", GetDefNum("ResPoison") - 10);
				SetDefNum("ResEnergy", GetDefNum("ResEnergy") - 10);
			}
			if ( m_pClient )
				m_pClient->removeBuff(BI_MAGICREFLECTION);
			return;
		}
		case SPELL_Steelskin:		// 114 // turns your skin into steel, giving a boost to your AR.
		case SPELL_Stoneskin:		// 115 // turns your skin into stone, giving a boost to your AR.
		case SPELL_Protection:
		case SPELL_Arch_Prot:
		{
			Sound(0x1ed);
			Effect(EFFECT_OBJ, ITEMID_FX_SPARKLE, this, 9, 20);
			if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
			{
				SetDefNum("ResPhysical", GetDefNum("ResPhysical") + pSpell->m_itSpell.m_PolyStr);
				SetDefNum("FasterCasting", GetDefNum("FasterCasting") + 2);
				Skill_SetBase(SKILL_MAGICRESISTANCE, minimum(Skill_GetMax(SKILL_MAGICRESISTANCE, true), Skill_GetBase(SKILL_MAGICRESISTANCE) + pSpell->m_itSpell.m_PolyDex));
			}
			else
			{
				m_defense = CalcArmorDefense();
			}
			if ( m_pClient )
			{
				if ( spell == SPELL_Protection )
					m_pClient->removeBuff(BI_PROTECTION);
				else if ( spell == SPELL_Arch_Prot )
					m_pClient->removeBuff(BI_ARCHPROTECTION);
			}
			return;
		}
		//case SPELL_Chameleon:		// 106 // makes your skin match the colors of whatever is behind you.
		//	return;
		case SPELL_Trance:			// 111 // temporarily increases your meditation skill.
		{
			Skill_SetBase(SKILL_MEDITATION, Skill_GetBase(SKILL_MEDITATION) - static_cast<WORD>(g_Cfg.GetSpellEffect(spell, iStatEffect)));
			return;
		}
		//case SPELL_Shield:		// 113 // erects a temporary force field around you. Nobody approaching will be able to get within 1 tile of you, though you can move close to them if you wish.
		//	return;
		case SPELL_Mind_Rot:
		{
			SetDefNum("LowerManaCost", GetDefNum("LowerManaCost") + pSpell->m_itSpell.m_spelllevel, true);
			if ( m_pClient )
				m_pClient->removeBuff(BI_MINDROT);
			return;
		}
		case SPELL_Curse_Weapon:
		{
			CItem *pWeapon = m_uidWeapon.ItemFind();
			if ( pWeapon )
				pWeapon->SetDefNum("HitLeechLife", pWeapon->GetDefNum("HitLeechLife") - pSpell->m_itSpell.m_spelllevel, true);	// Adding 50% HitLeechLife to the weapon, since damaging with it should return 50% of the damage dealt.
			return;
		}
		default:
			return;
	}
}

// Attach the spell effect for a duration.
// Add effects which are saved in the save file here.
// Not in LayerAdd
void CChar::Spell_Effect_Add(CItem *pSpell)
{
	ADDTOCALLSTACK("CChar::Spell_Effect_Add");
	// NOTE: ATTR_MAGIC without ATTR_MOVE_NEVER is dispellable !
	// equipped wands do not confer effect.
	if ( !pSpell || !pSpell->IsTypeSpellable() || pSpell->IsType(IT_WAND) )
		return;

	SPELL_TYPE spell = static_cast<SPELL_TYPE>(RES_GET_INDEX(pSpell->m_itSpell.m_spell));
	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
	if ( !spell || !pSpellDef )
		return;

	CChar *pCaster = pSpell->m_uidLink.CharFind();
	WORD iStatEffect = pSpell->m_itSpell.m_spelllevel;
	WORD iTimerEffect = static_cast<WORD>(maximum(pSpell->GetTimerAdjusted(), 0));

	if ( IsTrigUsed(TRIGGER_EFFECTADD) )
	{
		CScriptTriggerArgs Args;
		Args.m_pO1 = pSpell;
		Args.m_iN1 = spell;
		TRIGRET_TYPE iRet = OnTrigger(CTRIG_EffectAdd, pCaster, &Args);
		if ( iRet == TRIGRET_RET_TRUE )		// we don't want nothing to happen, removing memory also
		{
			pSpell->Delete(true);
			return;
		}
		else if ( iRet == TRIGRET_RET_FALSE )	// we want the memory to be equipped but we want custom things to happen: don't remove memory but stop here
			return;
	}

	// Buffs related variables
	TCHAR NumBuff[7][8];
	LPCTSTR pNumBuff[7] = { NumBuff[0], NumBuff[1], NumBuff[2], NumBuff[3], NumBuff[4], NumBuff[5], NumBuff[6] };

	switch ( pSpellDef->m_idLayer )
	{
		case LAYER_NONE:
			break;
		case LAYER_SPELL_Polymorph:
		{
			CCharBase *pCharDef = Char_GetDef();
			switch ( spell )
			{
				case SPELL_Polymorph:
				case SPELL_BeastForm:		// 107 // polymorphs you into an animal for a while.
				case SPELL_Monster_Form:	// 108 // polymorphs you into a monster for a while.
				{
					if ( m_pClient && IsSetOF(OF_Buffs) )
					{
						CResourceDef *pCharDefNew = g_Cfg.ResourceGetDef(RESOURCE_ID(RES_CHARDEF, m_atMagery.m_SummonID));
						LPCTSTR pszName = pCharDefNew->GetName();
						_strlwr(const_cast<TCHAR *>(pszName));
						if ( pszName[0] == '#' )
							pszName = "creature";
						strcpy(NumBuff[0], Str_GetArticleAndSpace(pszName));
						strcpy(NumBuff[1], pszName);
						NumBuff[0][strlen(NumBuff[0]) - 1] = '\0';		// trim whitespace from "a " / "an " strings
						m_pClient->removeBuff(BI_POLYMORPH);
						m_pClient->addBuff(BI_POLYMORPH, 1075824, 1075823, iTimerEffect, pNumBuff, 2);
					}
					break;
				}
				case SPELL_Horrific_Beast:
				{
					m_atMagery.m_SummonID = CREID_HORRIFIC_BEAST;
					pSpell->m_itSpell.m_PolyStr = 20;						// Hitpoint Regeneration
					pSpell->m_itSpell.m_PolyDex = 25;						// Damage Increase
					pSpell->m_itSpell.m_spellcharges = 5 - m_attackBase;	// Char min base damage
					pSpell->m_itSpell.m_spelllevel = 10 - m_attackRange;	// Char max base damage
					SetDefNum("RegenHits", GetDefNum("RegenHits") + pSpell->m_itSpell.m_PolyStr);
					SetDefNum("IncreaseDam", GetDefNum("IncreaseDam") + pSpell->m_itSpell.m_PolyDex);
					m_attackBase += static_cast<WORD>(pSpell->m_itSpell.m_spellcharges);
					m_attackRange += pSpell->m_itSpell.m_spelllevel;

					if ( m_pClient && IsSetOF(OF_Buffs) )
					{
						ITOA(pSpell->m_itSpell.m_PolyStr, NumBuff[0], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, NumBuff[1], 10);
						m_pClient->removeBuff(BI_HORRIFICBEAST);
						m_pClient->addBuff(BI_HORRIFICBEAST, 1060514, 1153763, iTimerEffect, pNumBuff, 2);
					}
					break;
				}
				case SPELL_Lich_Form:
				{
					m_atMagery.m_SummonID = CREID_LICH_FORM;
					pSpell->m_itSpell.m_PolyStr = 5;		// Hitpoint Regeneration
					pSpell->m_itSpell.m_PolyDex = 13;		// Mana Regeneration
					pSpell->m_itSpell.m_spellcharges = 10;	// Fire/Poison/Cold Resist
					SetDefNum("RegenHits", GetDefNum("RegenHits") - pSpell->m_itSpell.m_PolyStr);
					SetDefNum("RegenMana", GetDefNum("RegenMana") + pSpell->m_itSpell.m_PolyDex);
					SetDefNum("ResFire", GetDefNum("ResFire") - pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResPoison", GetDefNum("ResPoison") + pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResCold", GetDefNum("ResCold") + pSpell->m_itSpell.m_spellcharges);

					if ( m_pClient && IsSetOF(OF_Buffs) )
					{
						ITOA(pSpell->m_itSpell.m_PolyStr, NumBuff[0], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, NumBuff[1], 10);
						ITOA(pSpell->m_itSpell.m_spellcharges, NumBuff[2], 10);
						ITOA(pSpell->m_itSpell.m_spellcharges, NumBuff[3], 10);
						ITOA(pSpell->m_itSpell.m_spellcharges, NumBuff[4], 10);
						m_pClient->removeBuff(BI_LICHFORM);
						m_pClient->addBuff(BI_LICHFORM, 1060515, 1153767, iTimerEffect, pNumBuff, 5);
					}
					break;
				}
				case SPELL_Vampiric_Embrace:
				{
					if ( IsGargoyle() )
						m_atMagery.m_SummonID = pCharDef->IsFemale() ? CREID_GARGWOMAN : CREID_GARGMAN;
					else
						m_atMagery.m_SummonID = pCharDef->IsFemale() ? CREID_VAMPIREWOMAN : CREID_VAMPIREMAN;

					SetHue(0x847E);
					pSpell->m_itSpell.m_PolyStr = 20;		// Hit Life Leech
					pSpell->m_itSpell.m_PolyDex = 15;		// Stamina Regeneration
					pSpell->m_itSpell.m_spellcharges = 3;	// Mana Regeneration
					pSpell->m_itSpell.m_spelllevel = 25;	// Fire Resist
					SetDefNum("HitLeechLife", GetDefNum("HitLeechLife") + pSpell->m_itSpell.m_PolyStr);
					SetDefNum("RegenStam", GetDefNum("RegenStam") + pSpell->m_itSpell.m_PolyDex);
					SetDefNum("RegenMana", GetDefNum("RegenMana") + pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResFire", GetDefNum("ResFire") - pSpell->m_itSpell.m_spelllevel);

					if ( m_pClient && IsSetOF(OF_Buffs) )
					{
						ITOA(pSpell->m_itSpell.m_PolyStr, NumBuff[0], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, NumBuff[1], 10);
						ITOA(pSpell->m_itSpell.m_spellcharges, NumBuff[2], 10);
						ITOA(pSpell->m_itSpell.m_spelllevel, NumBuff[3], 10);
						m_pClient->removeBuff(BI_VAMPIRICEMBRACE);
						m_pClient->addBuff(BI_VAMPIRICEMBRACE, 1060521, 1153768, iTimerEffect, pNumBuff, 4);
					}
					break;
				}
				case SPELL_Wraith_Form:
				{
					if ( pCharDef->IsFemale() )
						m_atMagery.m_SummonID = CREID_WAILING_BANSHEE2;
					else
					{
						m_atMagery.m_SummonID = CREID_WRAITH;
						SetHue(HUE_TRANSLUCENT);
					}

					pSpell->m_itSpell.m_PolyStr = 15;		// Physical Resist
					pSpell->m_itSpell.m_PolyDex = 5;		// Energy/Fire Resist
					pSpell->m_itSpell.m_spellcharges = 5 + (15 * pCaster->Skill_GetBase(SKILL_SPIRITSPEAK) / 1000);		// Hit Mana Drain
					SetDefNum("ResPhysical", GetDefNum("ResPhysical") + pSpell->m_itSpell.m_PolyStr);
					SetDefNum("ResEnergy", GetDefNum("ResEnergy") - pSpell->m_itSpell.m_PolyDex);
					SetDefNum("ResFire", GetDefNum("ResFire") - pSpell->m_itSpell.m_PolyDex);
					SetDefNum("HitLeechMana", GetDefNum("HitLeechMana") + pSpell->m_itSpell.m_spellcharges);

					if ( m_pClient && IsSetOF(OF_Buffs) )
					{
						ITOA(pSpell->m_itSpell.m_PolyStr, NumBuff[0], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, NumBuff[1], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, NumBuff[2], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, NumBuff[3], 10);
						m_pClient->removeBuff(BI_WRAITHFORM);
						m_pClient->addBuff(BI_WRAITHFORM, 1060524, 1153829, iTimerEffect, pNumBuff, 4);
					}
					break;
				}
				case SPELL_Reaper_Form:
				{
					m_atMagery.m_SummonID = CREID_REAPER_FORM;
					pSpell->m_itSpell.m_PolyStr = 10;		// Swing Speed Increase, Spell Damage Increase
					pSpell->m_itSpell.m_PolyDex = 5;		// Physical/Cold/Poison/Energy Resist
					pSpell->m_itSpell.m_spellcharges = 25;	// Fire Resist
					SetDefNum("IncreaseSwingSpeed", GetDefNum("IncreaseSwingSpeed") + pSpell->m_itSpell.m_PolyStr);
					SetDefNum("IncreaseSpellDam", GetDefNum("IncreaseSpellDam") + pSpell->m_itSpell.m_PolyStr);
					SetDefNum("ResPhysical", GetDefNum("ResPhysical") + pSpell->m_itSpell.m_PolyDex);
					SetDefNum("ResCold", GetDefNum("ResCold") + pSpell->m_itSpell.m_PolyDex);
					SetDefNum("ResPoison", GetDefNum("ResPoison") + pSpell->m_itSpell.m_PolyDex);
					SetDefNum("ResEnergy", GetDefNum("ResEnergy") + pSpell->m_itSpell.m_PolyDex);
					SetDefNum("ResFire", GetDefNum("ResFire") - pSpell->m_itSpell.m_spellcharges);

					if ( m_pClient && IsSetOF(OF_Buffs) )
					{
						ITOA(pSpell->m_itSpell.m_PolyStr, NumBuff[0], 10);
						ITOA(pSpell->m_itSpell.m_PolyStr, NumBuff[1], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, NumBuff[2], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, NumBuff[3], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, NumBuff[4], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, NumBuff[5], 10);
						ITOA(pSpell->m_itSpell.m_spellcharges, NumBuff[6], 10);
						m_pClient->removeBuff(BI_REAPERFORM);
						m_pClient->addBuff(BI_REAPERFORM, 1071034, 1153781, iTimerEffect, pNumBuff, 7);
					}
					break;
				}
				case SPELL_Stone_Form:
				{
					m_atMagery.m_SummonID = CREID_STONE_FORM;
					pSpell->m_itSpell.m_PolyStr = 10;		// Swing Speed Increase / Damage Increase
					pSpell->m_itSpell.m_PolyDex = 2;		// Faster Casting
					pSpell->m_itSpell.m_spellcharges = (pCaster->Skill_GetBase(SKILL_MYSTICISM) + pCaster->Skill_GetBase(SKILL_FOCUS)) / 240;		// All Resists
					pSpell->m_itSpell.m_spelllevel = maximum(2, (pCaster->Skill_GetBase(SKILL_MYSTICISM) + pCaster->Skill_GetBase(SKILL_IMBUING)) / 480);		// All Resists Max
					SetDefNum("IncreaseSwingSpeed", GetDefNum("IncreaseSwingSpeed") - pSpell->m_itSpell.m_PolyStr);
					SetDefNum("FasterCasting", GetDefNum("FasterCasting") - pSpell->m_itSpell.m_PolyDex);
					SetDefNum("ResPhysical", GetDefNum("ResPhysical") + pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResFire", GetDefNum("ResFire") + pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResCold", GetDefNum("ResCold") + pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResPoison", GetDefNum("ResPoison") + pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResEnergy", GetDefNum("ResEnergy") + pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResPhysicalMax", GetDefNum("ResPhysicalMax") + pSpell->m_itSpell.m_spelllevel);
					SetDefNum("ResFireMax", GetDefNum("ResFireMax") + pSpell->m_itSpell.m_spelllevel);
					SetDefNum("ResColdMax", GetDefNum("ResColdMax") + pSpell->m_itSpell.m_spelllevel);
					SetDefNum("ResPoisonMax", GetDefNum("ResPoisonMax") + pSpell->m_itSpell.m_spelllevel);
					SetDefNum("ResEnergyMax", GetDefNum("ResEnergyMax") + pSpell->m_itSpell.m_spelllevel);
					SetDefNum("IncreaseDam", GetDefNum("IncreaseDam") + pSpell->m_itSpell.m_PolyStr);

					if ( m_pClient && IsSetOF(OF_Buffs) )
					{
						ITOA(-pSpell->m_itSpell.m_PolyStr, NumBuff[0], 10);
						ITOA(-pSpell->m_itSpell.m_PolyDex, NumBuff[1], 10);
						ITOA(pSpell->m_itSpell.m_spellcharges, NumBuff[2], 10);
						ITOA(pSpell->m_itSpell.m_spellcharges, NumBuff[3], 10);
						ITOA(pSpell->m_itSpell.m_PolyStr, NumBuff[4], 10);
						m_pClient->removeBuff(BI_STONEFORM);
						m_pClient->addBuff(BI_STONEFORM, 1080145, 1080146, iTimerEffect, pNumBuff, 5);
					}
					break;
				}
				default:
					break;
			}

			// set to creature type stats
			if ( (spell == SPELL_Polymorph) && IsSetMagicFlags(MAGICF_POLYMORPHSTATS) )
			{
				short SPELL_MAX_POLY_STAT = static_cast<short>(g_Cfg.m_iMaxPolyStats);
				if ( pCharDef->m_Str )
				{
					short iChange = pCharDef->m_Str - Stat_GetBase(STAT_STR);
					if ( iChange > SPELL_MAX_POLY_STAT )			// Can't pass from SPELL_MAX_POLY_STAT defined in .ini (MaxPolyStats)
						iChange = SPELL_MAX_POLY_STAT;
					else if ( (iChange < 0) && (iChange * -1 > SPELL_MAX_POLY_STAT) )	// Limit it to negative values too
						iChange = -SPELL_MAX_POLY_STAT;
					if ( iChange + Stat_GetBase(STAT_STR) < 0 )
						iChange = -Stat_GetBase(STAT_STR);
					Stat_AddMod(STAT_STR, iChange);
					pSpell->m_itSpell.m_PolyStr = iChange;
				}
				if ( pCharDef->m_Dex )
				{
					short iChange = pCharDef->m_Dex - Stat_GetBase(STAT_DEX);
					if ( iChange > SPELL_MAX_POLY_STAT )
						iChange = SPELL_MAX_POLY_STAT;
					else if ( (iChange < 0) && (iChange * -1 > SPELL_MAX_POLY_STAT) )	// Limit it to negative values too
						iChange = -SPELL_MAX_POLY_STAT;
					if ( iChange + Stat_GetBase(STAT_DEX) < 0 )
						iChange = -Stat_GetBase(STAT_DEX);
					Stat_AddMod(STAT_DEX, iChange);
					pSpell->m_itSpell.m_PolyDex = iChange;
				}
			}

			SetID(m_atMagery.m_SummonID);
			StatFlag_Set(STATF_Polymorph);
			return;
		}
		case LAYER_SPELL_Night_Sight:
		{
			StatFlag_Set(STATF_NightSight);
			if ( m_pClient )
			{
				m_pClient->addLight();
				if ( IsSetOF(OF_Buffs) )
				{
					m_pClient->removeBuff(BI_NIGHTSIGHT);
					m_pClient->addBuff(BI_NIGHTSIGHT, 1075643, 1075644, iTimerEffect);
				}
			}
			return;
		}
		case LAYER_SPELL_Incognito:
		{
			const CCharBase *pCharDef = Char_GetDef();
			ASSERT(pCharDef);
			StatFlag_Set(STATF_Incognito);

			pSpell->SetName(GetName());
			SetName(pCharDef->IsFemale() ? "#NAMES_HUMANFEMALE" : "#NAMES_HUMANMALE");

			if ( IsPlayableCharacter() )
				SetHue(static_cast<HUE_TYPE>(Calc_GetRandVal2(HUE_SKIN_LOW, HUE_SKIN_HIGH)) | HUE_MASK_UNDERWEAR);

			HUE_TYPE RandomHairHue = static_cast<HUE_TYPE>(Calc_GetRandVal2(HUE_HAIR_LOW, HUE_HAIR_HIGH));
			CItem *pHair = LayerFind(LAYER_HAIR);
			if ( pHair )
			{
				pSpell->GetTagDefs()->SetNum("COLOR.HAIR", static_cast<INT64>(pHair->GetHue()));
				pHair->SetHue(RandomHairHue);
			}

			CItem *pBeard = LayerFind(LAYER_BEARD);
			if ( pBeard )
			{
				pSpell->GetTagDefs()->SetNum("COLOR.BEARD", static_cast<INT64>(pBeard->GetHue()));
				pBeard->SetHue(RandomHairHue);
			}

			NotoSave_Update();
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				m_pClient->removeBuff(BI_INCOGNITO);
				m_pClient->addBuff(BI_INCOGNITO, 1075819, 1075820, iTimerEffect);
			}
			return;
		}
		case LAYER_SPELL_Invis:
		{
			StatFlag_Set(STATF_Invisible);
			Reveal(STATF_Hidden);	// clear previous Hiding skill effect (this will not reveal the char because STATF_Invisibility still set)
			UpdateModeFlag();
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				m_pClient->removeBuff(BI_INVISIBILITY);
				m_pClient->addBuff(BI_INVISIBILITY, 1075825, 1075826, iTimerEffect);
			}
			return;
		}
		case LAYER_SPELL_Paralyze:
		{
			StatFlag_Set(STATF_Freeze);
			UpdateMode();
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				m_pClient->removeBuff(BI_PARALYZE);
				m_pClient->addBuff(BI_PARALYZE, 1075827, 1075828, iTimerEffect);
			}
			return;
		}
		case LAYER_SPELL_Summon:
		{
			StatFlag_Set(STATF_Conjured);
			return;
		}
		case LAYER_SPELL_Strangle:
		{
			iStatEffect = pCaster->Skill_GetBase(SKILL_SPIRITSPEAK) / 100;
			if ( iStatEffect < 4 )
				iStatEffect = 4;
			pSpell->m_itSpell.m_spelllevel = iStatEffect;
			pSpell->m_itSpell.m_spellcharges = iStatEffect;

			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				double iStamPenalty = 3 - ((Stat_GetVal(STAT_DEX) / Stat_GetAdjusted(STAT_DEX)) * 2);
				WORD iTimerTotal = 0;
				for ( WORD i = 0; i < iStatEffect; i++ )
					iTimerTotal += (iStatEffect - i) * TICK_PER_SEC;

				ITOA(static_cast<int>((iStatEffect - 2) * iStamPenalty), NumBuff[0], 10);
				ITOA(static_cast<int>((iStatEffect + 1) * iStamPenalty), NumBuff[1], 10);
				m_pClient->removeBuff(BI_STRANGLE);
				m_pClient->addBuff(BI_STRANGLE, 1075794, 1075795, iTimerTotal, pNumBuff, 2);
			}
			return;
		}
		case LAYER_SPELL_Gift_Of_Renewal:
		{
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(pSpell->m_itSpell.m_spelllevel, NumBuff[0], 10);
				m_pClient->removeBuff(BI_GIFTOFRENEWAL);
				m_pClient->addBuff(BI_GIFTOFRENEWAL, 1075796, 1075797, iTimerEffect, pNumBuff, 1);
			}
			return;
		}
		case LAYER_SPELL_Attunement:
		{
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(pSpell->m_itSpell.m_spelllevel, NumBuff[0], 10);
				m_pClient->removeBuff(BI_ATTUNEWEAPON);
				m_pClient->addBuff(BI_ATTUNEWEAPON, 1075798, 1075799, iTimerEffect, pNumBuff, 1);
			}
			return;
		}
		case LAYER_SPELL_Thunderstorm:
		{
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(pSpell->m_itSpell.m_spelllevel, NumBuff[0], 10);
				m_pClient->removeBuff(BI_THUNDERSTORM);
				m_pClient->addBuff(BI_THUNDERSTORM, 1075800, 1075801, iTimerEffect, pNumBuff, 1);
			}
			return;
		}
		case LAYER_SPELL_Essence_Of_Wind:
		{
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(pSpell->m_itSpell.m_spelllevel, NumBuff[0], 10);
				m_pClient->removeBuff(BI_ESSENCEOFWIND);
				m_pClient->addBuff(BI_ESSENCEOFWIND, 1075802, 1075803, iTimerEffect, pNumBuff, 1);
			}
			return;
		}
		case LAYER_SPELL_Ethereal_Voyage:
		{
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				m_pClient->removeBuff(BI_ETHEREALVOYAGE);
				m_pClient->addBuff(BI_ETHEREALVOYAGE, 1075804, 1075805, iTimerEffect);
			}
			return;
		}
		case LAYER_SPELL_Gift_Of_Life:
		{
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				m_pClient->removeBuff(BI_GIFTOFLIFE);
				m_pClient->addBuff(BI_GIFTOFLIFE, 1075806, 1075807, iTimerEffect);
			}
			return;
		}
		case LAYER_SPELL_Arcane_Empowerment:
		{
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(pSpell->m_itSpell.m_spelllevel, NumBuff[0], 10);
				ITOA(pSpell->m_itSpell.m_spellcharges, NumBuff[1], 10);
				m_pClient->removeBuff(BI_ARCANEEMPOWERMENT);
				m_pClient->addBuff(BI_ARCANEEMPOWERMENT, 1075805, 1075804, iTimerEffect, pNumBuff, 1);
			}
			return;
		}
		/*case LAYER_Mortal_Strike:
		{
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				m_pClient->removeBuff(BI_MORTALSTRIKE);
				m_pClient->addBuff(BI_MORTALSTRIKE, 1075810, 1075811, iTimerEffect);
			}
			return;
		}*/
		case LAYER_SPELL_Pain_Spike:
		{
			CItem *pPrevious = LayerFind(LAYER_SPELL_Pain_Spike);
			if ( pPrevious )
			{
				pPrevious = LayerFind(LAYER_SPELL_Pain_Spike);
				if ( pPrevious )
					pSpell->m_itSpell.m_spellcharges += 2;
				//TO-DO If the spell targets someone already affected by the Pain Spike spell, only 3 to 7 points of DIRECT damage will be inflicted.
			}
			if ( m_pNPC )
				iStatEffect = ((pCaster->Skill_GetBase(SKILL_SPIRITSPEAK) - Skill_GetBase(SKILL_MAGICRESISTANCE)) / 10) + 30;
			else
				iStatEffect = ((pCaster->Skill_GetBase(SKILL_SPIRITSPEAK) - Skill_GetBase(SKILL_MAGICRESISTANCE)) / 100) + 18;
			pSpell->m_itSpell.m_spellcharges = 10;
			pSpell->m_itSpell.m_spelllevel = iStatEffect;
			return;
		}
		case LAYER_SPELL_Blood_Oath:
		{
			iStatEffect = ((Skill_GetBase(SKILL_MAGICRESISTANCE) * 10) / 20) + 10;	// bonus of reflection
			pSpell->m_itSpell.m_spelllevel = iStatEffect;
			if ( IsSetOF(OF_Buffs) )
			{
				if ( m_pClient )
				{
					strcpy(NumBuff[0], pCaster->GetName());
					strcpy(NumBuff[1], pCaster->GetName());
					m_pClient->removeBuff(BI_BLOODOATHCURSE);
					m_pClient->addBuff(BI_BLOODOATHCURSE, 1075659, 1075660, iTimerEffect, pNumBuff, 2);
				}
				CClient *pClientCaster = pCaster->m_pClient;
				if ( pClientCaster )
				{
					strcpy(NumBuff[0], GetName());
					pClientCaster->removeBuff(BI_BLOODOATHCASTER);
					pClientCaster->addBuff(BI_BLOODOATHCASTER, 1075661, 1075662, iTimerEffect, pNumBuff, 1);
				}
			}
			return;
		}
		case LAYER_SPELL_Corpse_Skin:
		{
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				m_pClient->removeBuff(BI_CORPSESKIN);
				m_pClient->addBuff(BI_CORPSESKIN, 1075805, 1075804, iTimerEffect, pNumBuff, 1);
			}
			pSpell->m_itSpell.m_PolyDex = 15;
			pSpell->m_itSpell.m_PolyStr = 10;
			SetDefNum("ResFire", GetDefNum("ResFire") - pSpell->m_itSpell.m_PolyDex, true);
			SetDefNum("ResPoison", GetDefNum("ResPoison") - pSpell->m_itSpell.m_PolyDex, true);
			SetDefNum("ResCold", GetDefNum("ResCold") + pSpell->m_itSpell.m_PolyStr, true);
			SetDefNum("ResPhysical", GetDefNum("ResPhysical") + pSpell->m_itSpell.m_PolyStr, true);
			return;
		}
		case LAYER_SPELL_Mind_Rot:
		{
			iStatEffect = 10;	// Lower Mana Cost
			pSpell->m_itSpell.m_spelllevel = iStatEffect;
			SetDefNum("LowerManaCost", GetDefNum("LowerManaCost") - iStatEffect, true);
			return;
		}
		case LAYER_SPELL_Curse_Weapon:
		{
			CItem *pWeapon = m_uidWeapon.ItemFind();
			if ( !pWeapon )
			{
				pSpell->Delete(true);
				return;
			}
			iStatEffect = 50;	// Hit Life Leech
			pSpell->m_itSpell.m_spelllevel = iStatEffect;
			pWeapon->SetDefNum("HitLeechLife", pWeapon->GetDefNum("HitLeechLife") + pSpell->m_itSpell.m_spelllevel, true);	// Adding 50% HitLeechLife to the weapon, since damaging with it should return 50% of the damage dealt.
			return;
		}
		default:
			break;
	}

	switch ( spell )
	{
		case SPELL_Reactive_Armor:
		{
			if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
			{
				iStatEffect = 15 + (pCaster->Skill_GetBase(SKILL_INSCRIPTION) / 200);
				pSpell->m_itSpell.m_spelllevel = iStatEffect;

				SetDefNum("ResPhysical", GetDefNum("ResPhysical") + iStatEffect);
				SetDefNum("ResFire", GetDefNum("ResFire") - 5);
				SetDefNum("ResCold", GetDefNum("ResCold") - 5);
				SetDefNum("ResPoison", GetDefNum("ResPoison") - 5);
				SetDefNum("ResEnergy", GetDefNum("ResEnergy") - 5);
			}
			else
			{
				StatFlag_Set(STATF_Reactive);
			}
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				m_pClient->removeBuff(BI_REACTIVEARMOR);
				if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
				{
					ITOA(iStatEffect, NumBuff[0], 10);
					for ( int idx = 1; idx < 5; ++idx )
						ITOA(5, NumBuff[idx], 10);

					m_pClient->addBuff(BI_REACTIVEARMOR, 1075812, 1075813, iTimerEffect, pNumBuff, 5);
				}
				else
				{
					m_pClient->addBuff(BI_REACTIVEARMOR, 1075812, 1070722, iTimerEffect);
				}
			}
			return;
		}
		case SPELL_Clumsy:
		{
			if ( pCaster && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
			{
				iStatEffect = 8 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100) - (Skill_GetBase(SKILL_MAGICRESISTANCE) / 100);
				pSpell->m_itSpell.m_spelllevel = iStatEffect;
			}
			Stat_AddMod(STAT_DEX, -iStatEffect);
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(iStatEffect, NumBuff[0], 10);
				m_pClient->removeBuff(BI_CLUMSY);
				m_pClient->addBuff(BI_CLUMSY, 1075831, 1075832, iTimerEffect, pNumBuff, 1);
			}
			return;
		}
		case SPELL_Particle_Form:	// 112 // turns you into an immobile, but untargetable particle system for a while.
		case SPELL_Stone:
		{
			StatFlag_Set(STATF_Stone);
			UpdateModeFlag();
			return;
		}
		case SPELL_Hallucination:
		{
			StatFlag_Set(STATF_Hallucinating);
			UpdateModeFlag();
			if ( m_pClient )
			{
				m_pClient->addChar(this);
				m_pClient->addPlayerSee(NULL);
			}
			return;
		}
		case SPELL_Feeblemind:
		{
			if ( pCaster && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
			{
				iStatEffect = 8 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100) - (Skill_GetBase(SKILL_MAGICRESISTANCE) / 100);
				pSpell->m_itSpell.m_spelllevel = iStatEffect;
			}
			Stat_AddMod(STAT_INT, -iStatEffect);
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(iStatEffect, NumBuff[0], 10);
				m_pClient->removeBuff(BI_FEEBLEMIND);
				m_pClient->addBuff(BI_FEEBLEMIND, 1075833, 1075834, iTimerEffect, pNumBuff, 1);
			}
			return;
		}
		case SPELL_Weaken:
		{
			if ( pCaster && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
			{
				iStatEffect = 8 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100) - (Skill_GetBase(SKILL_MAGICRESISTANCE) / 100);
				pSpell->m_itSpell.m_spelllevel = iStatEffect;
			}
			Stat_AddMod(STAT_STR, -iStatEffect);
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(iStatEffect, NumBuff[0], 10);
				m_pClient->removeBuff(BI_WEAKEN);
				m_pClient->addBuff(BI_WEAKEN, 1075837, 1075838, iTimerEffect, pNumBuff, 1);
			}
			return;
		}
		case SPELL_Curse:
		case SPELL_Mass_Curse:
		{
			if ( pCaster && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
			{
				iStatEffect = 8 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100) - (Skill_GetBase(SKILL_MAGICRESISTANCE) / 100);
				pSpell->m_itSpell.m_spelllevel = iStatEffect;
			}
			if ( (spell == SPELL_Curse) && IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) && m_pPlayer )		// Curse also decrease max resistances on players (not applied to Mass Curse)
			{
				SetDefNum("ResFireMax", GetDefNum("ResFireMax", true) - 10);
				SetDefNum("ResColdMax", GetDefNum("ResColdMax", true) - 10);
				SetDefNum("ResPoisonMax", GetDefNum("ResPoisonMax", true) - 10);
				SetDefNum("ResEnergyMax", GetDefNum("ResEnergyMax", true) - 10);
			}
			for ( int i = STAT_STR; i < STAT_BASE_QTY; i++ )
				Stat_AddMod(static_cast<STAT_TYPE>(i), -iStatEffect);

			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				BUFF_ICONS BuffIcon = BI_CURSE;
				DWORD BuffCliloc = 1075835;
				if ( spell == SPELL_Mass_Curse )
				{
					BuffIcon = BI_MASSCURSE;
					BuffCliloc = 1075839;
				}

				m_pClient->removeBuff(BuffIcon);
				for ( int idx = STAT_STR; idx < STAT_BASE_QTY; ++idx )
					ITOA(iStatEffect, NumBuff[idx], 10);
				if ( (spell == SPELL_Curse) && IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
				{
					for ( int idx = 3; idx < 7; ++idx )
						ITOA(10, NumBuff[idx], 10);

					m_pClient->addBuff(BuffIcon, BuffCliloc, 1075836, iTimerEffect, pNumBuff, 7);
				}
				else
				{
					m_pClient->addBuff(BuffIcon, BuffCliloc, 1075840, iTimerEffect, pNumBuff, 3);
				}
			}
			return;
		}
		case SPELL_Agility:
		{
			if ( pCaster && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
			{
				iStatEffect = 1 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100);
				pSpell->m_itSpell.m_spelllevel = iStatEffect;
			}
			Stat_AddMod(STAT_DEX, +iStatEffect);
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(iStatEffect, NumBuff[0], 10);
				m_pClient->removeBuff(BI_AGILITY);
				m_pClient->addBuff(BI_AGILITY, 1075841, 1075842, iTimerEffect, pNumBuff, 1);
			}
			return;
		}
		case SPELL_Cunning:
		{
			if ( pCaster && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
			{
				iStatEffect = 1 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100);
				pSpell->m_itSpell.m_spelllevel = iStatEffect;
			}
			Stat_AddMod(STAT_INT, +iStatEffect);
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(iStatEffect, NumBuff[0], 10);
				m_pClient->removeBuff(BI_CUNNING);
				m_pClient->addBuff(BI_CUNNING, 1075843, 1075844, iTimerEffect, pNumBuff, 1);
			}
			return;
		}
		case SPELL_Strength:
		{
			if ( pCaster && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
			{
				iStatEffect = 1 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100);
				pSpell->m_itSpell.m_spelllevel = iStatEffect;
			}
			Stat_AddMod(STAT_STR, +iStatEffect);
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(iStatEffect, NumBuff[0], 10);
				m_pClient->removeBuff(BI_STRENGTH);
				m_pClient->addBuff(BI_STRENGTH, 1075845, 1075846, iTimerEffect, pNumBuff, 1);
			}
			return;
		}
		case SPELL_Bless:
		{
			if ( pCaster && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
			{
				iStatEffect = 1 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100);
				pSpell->m_itSpell.m_spelllevel = iStatEffect;
			}
			for ( int i = STAT_STR; i < STAT_BASE_QTY; i++ )
				Stat_AddMod(static_cast<STAT_TYPE>(i), iStatEffect);

			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				for ( int idx = STAT_STR; idx < STAT_BASE_QTY; ++idx )
					ITOA(iStatEffect, NumBuff[idx], 10);

				m_pClient->removeBuff(BI_BLESS);
				m_pClient->addBuff(BI_BLESS, 1075847, 1075848, iTimerEffect, pNumBuff, STAT_BASE_QTY);
			}
			return;
		}
		case SPELL_Mana_Drain:
		{
			if ( pCaster )
			{
				iStatEffect = (400 + pCaster->Skill_GetBase(SKILL_EVALINT) - Skill_GetBase(SKILL_MAGICRESISTANCE)) / 10;
				if ( iStatEffect < 0 )
					iStatEffect = 0;
				else if ( iStatEffect > Stat_GetVal(STAT_INT) )
					iStatEffect = static_cast<short>(Stat_GetVal(STAT_INT));

				pSpell->m_itSpell.m_spelllevel = iStatEffect;
			}
			UpdateStatVal(STAT_INT, -iStatEffect);
			return;
		}
		case SPELL_Magic_Reflect:
		{
			StatFlag_Set(STATF_Reflection);
			if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
			{
				iStatEffect = 25 - (pCaster->Skill_GetBase(SKILL_INSCRIPTION) / 200);
				pSpell->m_itSpell.m_spelllevel = iStatEffect;

				SetDefNum("ResPhysical", GetDefNum("ResPhysical") - iStatEffect);
				SetDefNum("ResFire", GetDefNum("ResFire") + 10);
				SetDefNum("ResCold", GetDefNum("ResCold") + 10);
				SetDefNum("ResPoison", GetDefNum("ResPoison") + 10);
				SetDefNum("ResEnergy", GetDefNum("ResEnergy") + 10);
			}
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				m_pClient->removeBuff(BI_MAGICREFLECTION);
				if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
				{
					ITOA(-iStatEffect, NumBuff[0], 10);
					for ( int idx = 1; idx < 5; ++idx )
						ITOA(10, NumBuff[idx], 10);

					m_pClient->addBuff(BI_MAGICREFLECTION, 1075817, 1075818, iTimerEffect, pNumBuff, 5);
				}
				else
				{
					m_pClient->addBuff(BI_MAGICREFLECTION, 1075817, 1070722, iTimerEffect);
				}
			}
			return;
		}
		case SPELL_Steelskin:		// 114 // turns your skin into steel, giving a boost to your AR.
		case SPELL_Stoneskin:		// 115 // turns your skin into stone, giving a boost to your AR.
		case SPELL_Protection:
		case SPELL_Arch_Prot:
		{
			WORD iPhysicalResist = 0;
			WORD iMagicResist = 0;
			if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
			{
				iStatEffect = minimum(75, (pCaster->Skill_GetBase(SKILL_EVALINT) + pCaster->Skill_GetBase(SKILL_MEDITATION) + pCaster->Skill_GetBase(SKILL_INSCRIPTION)) / 40);
				iPhysicalResist = 15 - (pCaster->Skill_GetBase(SKILL_INSCRIPTION) / 200);
				iMagicResist = minimum(Skill_GetBase(SKILL_MAGICRESISTANCE), 350 - (Skill_GetBase(SKILL_INSCRIPTION) / 20));

				pSpell->m_itSpell.m_spelllevel = iStatEffect;
				pSpell->m_itSpell.m_PolyStr = static_cast<short>(iPhysicalResist);
				pSpell->m_itSpell.m_PolyDex = static_cast<short>(iMagicResist);

				SetDefNum("ResPhysical", GetDefNum("ResPhysical") - iPhysicalResist);
				SetDefNum("FasterCasting", GetDefNum("FasterCasting") - 2);
				Skill_SetBase(SKILL_MAGICRESISTANCE, Skill_GetBase(SKILL_MAGICRESISTANCE) - iMagicResist);
			}
			else
			{
				m_defense = CalcArmorDefense();
			}
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				BUFF_ICONS BuffIcon = BI_PROTECTION;
				DWORD BuffCliloc = 1075814;
				if ( spell == SPELL_Arch_Prot )
				{
					BuffIcon = BI_ARCHPROTECTION;
					BuffCliloc = 1075816;
				}

				m_pClient->removeBuff(BuffIcon);
				if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
				{
					ITOA(-iPhysicalResist, NumBuff[0], 10);
					ITOA(-iMagicResist / 10, NumBuff[1], 10);
					m_pClient->addBuff(BuffIcon, BuffCliloc, 1075815, iTimerEffect, pNumBuff, 2);
				}
				else
				{
					m_pClient->addBuff(BuffIcon, BuffCliloc, 1070722, iTimerEffect);
				}
			}
			return;
		}
		case SPELL_Trance:			// 111 // temporarily increases your meditation skill.
		{
			Skill_SetBase(SKILL_MEDITATION, Skill_GetBase(SKILL_MEDITATION) + iStatEffect);
			return;
		}
		//case SPELL_Chameleon:		// 106 // makes your skin match the colors of whatever is behind you.
		//case SPELL_Shield:		// 113 // erects a temporary force field around you. Nobody approaching will be able to get within 1 tile of you, though you can move close to them if you wish.
		//	return;
	}
}

bool CChar::Spell_Equip_OnTick(CItem *pItem)
{
	ADDTOCALLSTACK("CChar::Spell_Equip_OnTick");
	// Spells that have a gradual effect over time.
	// NOTE: These are not necessarily "magical". could be something physical as well.
	// RETURN: false = kill the spell.

	ASSERT(pItem);
	SPELL_TYPE spell = static_cast<SPELL_TYPE>(RES_GET_INDEX(pItem->m_itSpell.m_spell));
	int iCharges = pItem->m_itSpell.m_spellcharges;
	int iLevel = pItem->m_itSpell.m_spelllevel;

	switch ( spell )
	{
		case SPELL_Ale:		// 90 = drunkeness ?
		case SPELL_Wine:	// 91 = mild drunkeness ?
		case SPELL_Liquor:	// 92 = extreme drunkeness ?
		{
			// Chance to get sober quickly
			if ( 10 > Calc_GetRandVal(100) )
				pItem->m_itSpell.m_spellcharges--;

			Stat_SetVal(STAT_INT, maximum(0, Stat_GetVal(STAT_INT) - 1));
			Stat_SetVal(STAT_DEX, maximum(0, Stat_GetVal(STAT_DEX) - 1));

			if ( !Calc_GetRandVal(3) )
			{
				Speak(g_Cfg.GetDefaultMsg(DEFMSG_SPELL_ALCOHOL_HIC));
				if ( !IsStatFlag(STATF_OnHorse) )
				{
					UpdateDir(static_cast<DIR_TYPE>(Calc_GetRandVal(DIR_QTY)));
					UpdateAnimate(ANIM_BOW);
				}
			}

			// We will have this effect again
			pItem->SetTimeout(5 * TICK_PER_SEC);
			break;
		}
		case SPELL_Regenerate:
		{
			if ( iCharges <= 0 || iLevel <= 0 )
				return false;

			// Gain HP.
			UpdateStatVal(STAT_STR, static_cast<short>(g_Cfg.GetSpellEffect(spell, iLevel)));
			pItem->SetTimeout(2 * TICK_PER_SEC);
			break;
		}
		case SPELL_Hallucination:
		{
			if ( iCharges <= 0 || iLevel <= 0 )
				return false;

			if ( m_pClient )
			{
				static const SOUND_TYPE sm_sounds[] = { 0x243, 0x244 };
				m_pClient->addSound(sm_sounds[Calc_GetRandVal(COUNTOF(sm_sounds))]);
				m_pClient->addChar(this);
				m_pClient->addPlayerSee(NULL);
			}
			pItem->SetTimeout(Calc_GetRandLLVal2(15, 30) * TICK_PER_SEC);
			break;
		}
		case SPELL_Poison:
		{
			// Both potions and poison spells use this.
			// m_itSpell.m_spelllevel = strength of the poison ! 0-1000

			if ( iCharges <= 0 )
				return false;

			int iDmg = 0;
			// The poison in your body is having an effect.
			if ( IsSetMagicFlags(MAGICF_OSIFORMULAS) )
			{
				//iLevel = pItem->m_itSpell.m_spelllevel;	//Osi Formulas store directly the strenght in more2
				iDmg = pItem->m_itSpell.m_pattern;
				switch ( iLevel )
				{
					default:
					case 0:	//Lesser
						iDmg = IMULDIV(Stat_GetVal(STAT_STR), Calc_GetRandVal2(4, 7), 100);	// damage is different for lesser: it gets value from current hp
						pItem->SetTimeout(20);
						break;
					case 1:	//Standard
						pItem->SetTimeout(30);
						break;
					case 2:	//Greater
						pItem->SetTimeout(40);
						break;
					case 3:	//Deadly
						pItem->SetTimeout(50);
						break;
					case 4:	//Lethal
						pItem->SetTimeout(50);
						break;
				}

				static LPCTSTR const sm_Poison_Message[] =
				{
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_LESSER),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_STANDARD),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_GREATER),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_DEADLY),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_LETHAL)

				};
				static LPCTSTR const sm_Poison_Message_Other[] =
				{
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_LESSER1),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_STANDARD1),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_GREATER1),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_DEADLY1),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_LETHAL1)

				};
				Emote2(sm_Poison_Message[iLevel], sm_Poison_Message_Other[iLevel], m_pClient);
				SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_SPELL_YOUFEEL), sm_Poison_Message[iLevel]);
			}
			else
			{
				if ( iLevel < 50 )
					return false;
				if ( iLevel < 200 )	// Lesser
					iLevel = 0;
				else if ( iLevel < 400 ) // Normal
					iLevel = 1;
				else if ( iLevel < 800 ) // Greater
					iLevel = 2;
				else	// Deadly.
					iLevel = 3;

				pItem->m_itSpell.m_spelllevel -= 50;	// gets weaker too.	Only on old formulas
				iDmg = IMULDIV(Stat_GetMax(STAT_STR), iLevel * 2, 100);
				pItem->SetTimeout((5 + Calc_GetRandLLVal(4)) * TICK_PER_SEC);

				static LPCTSTR const sm_Poison_Message[] =
				{
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_POISON_1),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_POISON_2),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_POISON_3),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_POISON_4)
				};

				TCHAR *pszMsg = Str_GetTemp();
				sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_SPELL_LOOKS), sm_Poison_Message[iLevel]);
				Emote(pszMsg, m_pClient);
				SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_SPELL_YOUFEEL), sm_Poison_Message[iLevel]);
			}

			static const int sm_iPoisonMax[] = { 2, 4, 6, 8, 10 };
			OnTakeDamage(maximum(sm_iPoisonMax[iLevel], iDmg), pItem->m_uidLink.CharFind(), DAMAGE_MAGIC|DAMAGE_POISON|DAMAGE_NODISTURB|DAMAGE_NOREVEAL|DAMAGE_NOUNPARALYZE, 0, 0, 0, 100, 0);

			if ( IsSetOF(OF_Buffs) && m_pClient )
			{
				m_pClient->removeBuff(BI_POISON);
				m_pClient->addBuff(BI_POISON, 1017383, 1070722, static_cast<WORD>(pItem->GetTimerAdjusted()));
			}
			break;
		}
		case SPELL_Strangle:
		{
			double iStamPenalty = 3 - ((Stat_GetVal(STAT_DEX) / Stat_GetAdjusted(STAT_DEX)) * 2);
			int iDmg = static_cast<int>(Calc_GetRandLLVal2(pItem->m_itSpell.m_spelllevel - 2, pItem->m_itSpell.m_spelllevel + 1) * iStamPenalty);
			int iRemainingTicks = pItem->m_itSpell.m_spelllevel - pItem->m_itSpell.m_spellcharges;

			OnTakeDamage(maximum(1, iDmg), pItem->m_uidLink.CharFind(), DAMAGE_MAGIC|DAMAGE_POISON|DAMAGE_NODISTURB|DAMAGE_NOREVEAL|DAMAGE_NOUNPARALYZE, 0, 0, 0, 100, 0);
			pItem->SetTimeout(maximum(1, iRemainingTicks * TICK_PER_SEC));
			break;
		}
		case SPELL_Pain_Spike:
		{
			// Receives x amount (stored in pItem->m_itSpell.m_spelllevel) of damage in 10 seconds, so damage each second is equal to total / 10
			OnTakeDamage(pItem->m_itSpell.m_spelllevel / 10, pItem->m_uidLink.CharFind(), DAMAGE_MAGIC|DAMAGE_FIXED);
			pItem->SetTimeout(TICK_PER_SEC);
			break;
		}
		default:
			return false;
	}

	// Total number of ticks to come back here.
	if ( --pItem->m_itSpell.m_spellcharges > 0 )
		return true;
	return false;
}

CItem *CChar::Spell_Effect_Create(SPELL_TYPE spell, LAYER_TYPE layer, int iSkillLevel, int iDuration, CObjBase *pSrc, bool bEquip)
{
	ADDTOCALLSTACK("CChar::Spell_Effect_Create");
	// Attach an effect to the Character.
	//
	// ARGS:
	// spell = SPELL_Invis, etc.
	// layer == LAYER_FLAG_Potion, etc.
	// iSkillLevel = 0-1000 = skill level or other spell specific value.
	// iDuration = TICK_PER_SEC
	// bEquip automatically equips the memory, false requires manual equipment... usefull to setup everything before calling @MemoryEquip
	//
	// NOTE:
	//   ATTR_MAGIC without ATTR_MOVE_NEVER is dispellable !

	// Check if there's any previous effect to clear before apply the new effect
	for ( CItem *pSpellPrev = GetContentHead(); pSpellPrev != NULL; pSpellPrev = pSpellPrev->GetNext() )
	{
		if ( layer != pSpellPrev->GetEquipLayer() )
			continue;

		// Some spells create the memory using TIMER=-1 to make the effect last until cast again,
		// death or logout. So casting this same spell again will just remove the current effect.
		if ( pSpellPrev->GetTimerAdjusted() == -1 )
		{
			pSpellPrev->Delete();
			return NULL;
		}

		// Check if stats spells can stack
		if ( (layer == LAYER_SPELL_STATS) && (spell != pSpellPrev->m_itSpell.m_spell) && IsSetMagicFlags(MAGICF_STACKSTATS) )
			continue;

		pSpellPrev->Delete();
		break;
	}

	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
	CItem *pSpell = CItem::CreateBase(pSpellDef ? pSpellDef->m_idSpell : ITEMID_RHAND_POINT_NW);
	ASSERT(pSpell);

	switch ( layer )
	{
		case LAYER_FLAG_Criminal:		pSpell->SetName("Criminal Timer");			break;
		case LAYER_FLAG_Drunk:			pSpell->SetName("Drunk Effect");			break;
		case LAYER_FLAG_Hallucination:	pSpell->SetName("Hallucination Effect");	break;
		case LAYER_FLAG_PotionUsed:		pSpell->SetName("Potion Cooldown");			break;
		case LAYER_FLAG_Murders:		pSpell->SetName("Murder Decay");			break;
		default:						break;
	}

	g_World.m_uidNew = pSpell->GetUID();
	pSpell->SetAttr(pSpellDef ? ATTR_NEWBIE|ATTR_MAGIC : ATTR_NEWBIE);
	pSpell->SetType(IT_SPELL);
	pSpell->SetDecayTime(iDuration);
	pSpell->m_itSpell.m_spell = static_cast<WORD>(spell);
	pSpell->m_itSpell.m_spelllevel = static_cast<WORD>(g_Cfg.GetSpellEffect(spell, iSkillLevel));
	pSpell->m_itSpell.m_spellcharges = 1;
	if ( pSrc )
		pSpell->m_uidLink = pSrc->GetUID();

	if ( bEquip )
		LayerAdd(pSpell, layer);

	Spell_Effect_Add(pSpell);
	return pSpell;
}

void CChar::Spell_Area(CPointMap pntTarg, int iDist, int iSkillLevel)
{
	ADDTOCALLSTACK("CChar::Spell_Area");
	// Effects all creatures in the area. (but not us)
	// ARGS:
	// iSkillLevel = 0-1000

	SPELL_TYPE spelltype = m_atMagery.m_Spell;
	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spelltype);
	if ( !pSpellDef )
		return;

	CWorldSearch AreaChar(pntTarg, iDist);
	for (;;)
	{
		CChar *pChar = AreaChar.GetChar();
		if ( !pChar )
			break;
		if ( pChar == this )
		{
			if ( pSpellDef->IsSpellType(SPELLFLAG_HARM) && !IsSetMagicFlags(MAGICF_CANHARMSELF) )
				continue;
		}
		pChar->OnSpellEffect(spelltype, this, iSkillLevel, NULL);
	}

	if ( !pSpellDef->IsSpellType(SPELLFLAG_DAMAGE) )	// prevent damage nearby items on ground
	{
		CWorldSearch AreaItem(pntTarg, iDist);
		for (;;)
		{
			CItem *pItem = AreaItem.GetItem();
			if ( !pItem )
				break;
			pItem->OnSpellEffect(spelltype, this, iSkillLevel, NULL);
		}
	}
}

void CChar::Spell_Field(CPointMap pntTarg, ITEMID_TYPE idEW, ITEMID_TYPE idNS, unsigned int fieldWidth, unsigned int fieldGauge, int iSkillLevel, CChar *pCharSrc, ITEMID_TYPE idnewEW, ITEMID_TYPE idnewNS, int iDuration, HUE_TYPE iColor)
{
	ADDTOCALLSTACK("CChar::Spell_Field");
	// Cast the field spell to here.
	// ARGS:
	// pntTarg = target
	// idEW = ID of EW aligned spell object
	// idNS = ID of NS aligned spell object
	// fieldWidth = width of the field (looking from char's point of view)
	// fieldGauge = thickness of the field
	// iSkillLevel = 0-1000
	// idnewEW and idnewNS are the overriders created in @Success trigger, passed as another arguments because checks are made using default items

	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_Spell);
	ASSERT(pSpellDef);

	if ( m_pArea && m_pArea->IsGuarded() && pSpellDef->IsSpellType(SPELLFLAG_HARM) )
		Noto_Criminal();

	// get the dir of the field.
	int dx = abs(pntTarg.m_x - GetTopPoint().m_x);
	int dy = abs(pntTarg.m_y - GetTopPoint().m_y);
	ITEMID_TYPE id = (dx > dy) ? (idnewNS ? idnewNS : idNS) : (idnewEW ? idnewEW : idEW);

	int minX = static_cast<int>((fieldWidth - 1) / 2) - (fieldWidth - 1);
	int maxX = minX + (fieldWidth - 1);

	int minY = static_cast<int>((fieldGauge - 1) / 2) - (fieldGauge - 1);
	int maxY = minY + (fieldGauge - 1);

	if ( iDuration <= 0 )
		iDuration = GetSpellDuration(m_atMagery.m_Spell, iSkillLevel, pCharSrc);

	if ( IsSetMagicFlags(MAGICF_NOFIELDSOVERWALLS) )
	{
		// check if anything is blocking the field from fully extending to its desired width

		// first checks center piece, then left direction (minX), and finally right direction (maxX)
		// (structure of the loop looks a little odd but it should be more effective for wide fields (we don't really
		// want to be testing the far left or right of the field when it has been blocked towards the center))
		for ( int ix = 0; ; ix <= 0 ? ix-- : ix++ )
		{
			if ( ix < minX )
				ix = 1;	// start checking right extension
			if ( ix > maxX )
				break; // all done

			// check the whole width of the field for anything that would block this placement
			for ( int iy = minY; iy <= maxY; iy++ )
			{
				CPointMap ptg = pntTarg;
				if ( dx > dy )
				{
					ptg.m_y += static_cast<short>(ix);
					ptg.m_x += static_cast<short>(iy);
				}
				else
				{
					ptg.m_x += static_cast<short>(ix);
					ptg.m_y += static_cast<short>(iy);
				}

				DWORD wBlockFlags = 0;
				g_World.GetHeightPoint2(ptg, wBlockFlags, true);
				if ( wBlockFlags & (CAN_I_BLOCK|CAN_I_DOOR) )
				{
					if ( ix < 0 )	// field cannot extend fully to the left
						minX = ix + 1;
					else if ( ix > 0 ) // field cannot extend fully to the right
						maxX = ix - 1;
					else	// center piece is blocked, field cannot be created at all
						return;
					break;
				}
			}
		}
	}

	for ( int ix = minX; ix <= maxX; ix++ )
	{
		for ( int iy = minY; iy <= maxY; iy++ )
		{
			bool fGoodLoc = true;

			// Where is this ?
			CPointMap ptg = pntTarg;
			if ( dx > dy )
			{
				ptg.m_y += static_cast<short>(ix);
				ptg.m_x += static_cast<short>(iy);
			}
			else
			{
				ptg.m_x += static_cast<short>(ix);
				ptg.m_y += static_cast<short>(iy);
			}

			// Check for direct cast on a creature.
			CWorldSearch AreaChar(ptg);
			for (;;)
			{
				CChar *pChar = AreaChar.GetChar();
				if ( !pChar )
					break;

				if ( pChar->GetPrivLevel() > GetPrivLevel() )	// skip higher priv characters
					continue;

				if ( (pSpellDef->IsSpellType(SPELLFLAG_HARM)) && (!pChar->OnAttackedBy(this, false)) )	// they should know they where attacked.
					continue;

				if ( !pSpellDef->IsSpellType(SPELLFLAG_NOUNPARALYZE) )
				{
					CItem *pParalyze = pChar->LayerFind(LAYER_SPELL_Paralyze);
					if ( pParalyze )
						pParalyze->Delete();

					CItem *pStuck = pChar->LayerFind(LAYER_FLAG_Stuck);
					if ( pStuck )
						pStuck->Delete();
				}

				if ( (idEW == ITEMID_STONE_WALL) || (idEW == ITEMID_FX_ENERGY_F_EW) || (idEW == ITEMID_FX_ENERGY_F_NS) )	// don't place stone wall over characters
				{
					fGoodLoc = false;
					break;
				}
			}

			if ( !fGoodLoc )
				continue;

			// Check for direct cast on an item.
			CWorldSearch AreaItem(ptg);
			for (;;)
			{
				CItem *pItem = AreaItem.GetItem();
				if ( !pItem )
					break;
				if ( pItem->IsType(IT_SPELL) && IsSetMagicFlags(MAGICF_OVERRIDEFIELDS) )
				{
					pItem->Delete();
					continue;
				}
				pItem->OnSpellEffect(m_atMagery.m_Spell, this, iSkillLevel, NULL);
			}

			CItem *pSpell = CItem::CreateBase(id);
			ASSERT(pSpell);
			pSpell->m_itSpell.m_spell = static_cast<WORD>(m_atMagery.m_Spell);
			pSpell->m_itSpell.m_spelllevel = static_cast<WORD>(iSkillLevel);
			pSpell->m_itSpell.m_spellcharges = 1;
			pSpell->m_uidLink = GetUID();	// link it back to you
			pSpell->SetType(IT_SPELL);
			pSpell->SetAttr(ATTR_MAGIC);
			pSpell->SetHue(iColor);
			pSpell->GenerateScript(this);
			pSpell->MoveToDecay(ptg, iDuration, true);
		}
	}
}

bool CChar::Spell_CanCast(SPELL_TYPE &spell, bool fTest, CObjBase *pSrc, bool fFailMsg, bool fCheckAntiMagic)
{
	ADDTOCALLSTACK("CChar::Spell_CanCast");
	// ARGS:
	//  pSrc = possible scroll or wand source.
	if ( !pSrc || (spell <= SPELL_NONE) )
		return false;

	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
	if ( !pSpellDef )
		return false;
	if ( pSpellDef->IsSpellType(SPELLFLAG_DISABLED) )
		return false;

	int skill = SKILL_NONE;
	int iSkillTest = 0;
	if ( !pSpellDef->GetPrimarySkill(&iSkillTest, NULL) )
		iSkillTest = SKILL_MAGERY;
	skill = static_cast<SKILL_TYPE>(iSkillTest);

	if ( !Skill_CanUse(static_cast<SKILL_TYPE>(skill)) )
		return false;

	short wManaUse = static_cast<short>(pSpellDef->m_wManaUse * (100 - minimum(GetDefNum("LowerManaCost", true), 40)) / 100);
	short wTithingUse = static_cast<short>(pSpellDef->m_wTithingUse * (100 - minimum(GetDefNum("LowerReagentCost", true), 40)) / 100);

	if ( pSrc != this )
	{
		CItem *pItem = dynamic_cast<CItem *>(pSrc);
		if ( pItem )
		{
			if ( pItem->GetType() == IT_WAND )
			{
				wManaUse = 0;
				wTithingUse = 0;
			}
			else if ( pItem->GetType() == IT_SCROLL )
			{
				wManaUse /= 2;
				wTithingUse /= 2;
			}
		}
	}

	CScriptTriggerArgs Args(spell, wManaUse, pSrc);
	if ( fTest )
		Args.m_iN3 |= 0x0001;
	if ( fFailMsg )
		Args.m_iN3 |= 0x0002;
	Args.m_VarsLocal.SetNum("TithingUse", wTithingUse);

	if ( IsTrigUsed(TRIGGER_SELECT) )
	{
		TRIGRET_TYPE iRet = Spell_OnTrigger(spell, SPTRIG_SELECT, this, &Args);
		if ( iRet == TRIGRET_RET_TRUE )
			return false;
		if ( iRet == TRIGRET_RET_FALSE )
			return true;
		if ( iRet == TRIGRET_RET_HALFBAKED )		// just for compatibility with @SpellSelect
			return true;
	}

	if ( IsTrigUsed(TRIGGER_SPELLSELECT) )
	{
		TRIGRET_TYPE iRet = OnTrigger(CTRIG_SpellSelect, this, &Args);
		if ( iRet == TRIGRET_RET_TRUE )
			return false;
		if ( iRet == TRIGRET_RET_HALFBAKED )
			return true;
	}

	if ( spell != Args.m_iN1 )
	{
		pSpellDef = g_Cfg.GetSpellDef(spell);
		if ( !pSpellDef )
			return false;
		spell = static_cast<SPELL_TYPE>(Args.m_iN1);
	}
	wManaUse = static_cast<short>(Args.m_iN2);
	wTithingUse = static_cast<short>(Args.m_VarsLocal.GetKeyNum("TithingUse"));

	if ( pSrc != this )
	{
		// Cast spell using magic items (wand/scroll)
		CItem *pItem = dynamic_cast<CItem *>(pSrc);
		if ( !pItem )
			return false;
		if ( !pItem->IsAttr(ATTR_MAGIC) )
		{
			if ( fFailMsg )
				SysMessageDefault(DEFMSG_SPELL_ENCHANT_LACK);
			return false;
		}
		CObjBaseTemplate *pObjTop = pSrc->GetTopLevelObj();
		if ( pObjTop != this )		// magic items must be on your person to use.
		{
			if ( fFailMsg )
				SysMessageDefault(DEFMSG_SPELL_ENCHANT_ACTIVATE);
			return false;
		}
		if ( pItem->IsType(IT_WAND) )
		{
			// Must have charges.
			if ( pItem->m_itWeapon.m_spellcharges <= 0 )
			{
				if ( fFailMsg )
					SysMessageDefault(DEFMSG_SPELL_WAND_NOCHARGE);
				return false;
			}
			//wManaUse = 0;
			if ( !fTest && (pItem->m_itWeapon.m_spellcharges != 255) )
			{
				pItem->m_itWeapon.m_spellcharges--;
				pItem->UpdatePropertyFlag(AUTOTOOLTIP_FLAG_WANDCHARGES);
			}
		}
		else	// Scroll
		{
			//wManaUse /= 2;
			if ( !fTest )
				pItem->ConsumeAmount();
		}
	}
	else
	{
		// Raw cast from spellbook
		if ( IsPriv(PRIV_GM) )
			return true;

		if ( m_pPlayer )
		{
			if ( IsStatFlag(STATF_DEAD|STATF_Sleeping) || !pSpellDef->m_SkillReq.IsResourceMatchAll(this) )
			{
				if ( fFailMsg )
					SysMessageDefault(DEFMSG_SPELL_TRY_DEAD);
				return false;
			}

			// check the spellbook for it.
			CItem *pBook = GetSpellbook(spell);
			if ( !pBook )
			{
				if ( fFailMsg )
					SysMessageDefault(DEFMSG_SPELL_TRY_NOBOOK);
				return false;
			}
			if ( !pBook->IsSpellInBook(spell) )
			{
				if ( fFailMsg )
					SysMessageDefault(DEFMSG_SPELL_TRY_NOTYOURBOOK);
				return false;
			}

			// check for reagents
			if ( g_Cfg.m_fReagentsRequired && !m_pNPC && (pSrc == this) )
			{
				if ( GetDefNum("LowerReagentCost", true) <= Calc_GetRandVal(100) )
				{
					const CResourceQtyArray *pRegs = &(pSpellDef->m_Reags);
					CItemContainer *pPack = GetContainer(LAYER_PACK);
					size_t iMissing = pPack->ResourceConsumePart(pRegs, 1, 100, fTest);
					if ( iMissing != pRegs->BadIndex() )
					{
						if ( fFailMsg )
						{
							CResourceDef *pReagDef = g_Cfg.ResourceGetDef(pRegs->GetAt(iMissing).GetResourceID());
							SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_SPELL_TRY_NOREGS), pReagDef ? pReagDef->GetName() : g_Cfg.GetDefaultMsg(DEFMSG_SPELL_TRY_THEREG));
						}
						return false;
					}
				}
			}
		}
	}

	if ( fCheckAntiMagic )
	{
		if ( !IsPriv(PRIV_GM) && m_pArea && m_pArea->CheckAntiMagic(spell) )
		{
			if ( fFailMsg )
				SysMessageDefault(DEFMSG_MAGERY_6); // An anti-magic field disturbs the spells.
			m_Act_Difficulty = -1;	// Give very little credit for failure !
			return false;
		}
	}

	// Check required mana
	if ( Stat_GetVal(STAT_INT) < wManaUse )
	{
		if ( fFailMsg )
			SysMessageDefault(DEFMSG_SPELL_TRY_NOMANA);
		return false;
	}
	if ( !fTest && wManaUse )
	{
		if ( m_Act_Difficulty < 0 )	// use diff amount of mana if we fail.
			wManaUse = wManaUse / 2 + static_cast<short>(Calc_GetRandVal(wManaUse / 2 + wManaUse / 4));
		UpdateStatVal(STAT_INT, -wManaUse);
	}

	// Check Tithing
	int wTithing = static_cast<int>(GetDefNum("Tithing"));
	if ( wTithing < wTithingUse )
	{
		if ( fFailMsg )
			SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_SPELL_TRY_NOTITHING), wTithingUse);
		return false;
	}
	if ( !fTest && wTithingUse )
	{
		// Consume points
		if ( m_Act_Difficulty < 0 )	// use diff amount of points if we fail.
			wTithingUse = wTithingUse / 2 + static_cast<short>(Calc_GetRandVal(wTithingUse / 2 + wTithingUse / 4));
		SetDefNum("Tithing", wTithing - wTithingUse);
	}
	return true;
}

bool CChar::Spell_TargCheck_Face()
{
	ADDTOCALLSTACK("CChar::Spell_TargCheck_Face");
	if ( !IsSetMagicFlags(MAGICF_NODIRCHANGE) )
		UpdateDir(m_Act_p);

	// Check if target in on anti-magic region
	CRegionBase *pArea = m_Act_p.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA);
	if ( !IsPriv(PRIV_GM) && pArea && pArea->CheckAntiMagic(m_atMagery.m_Spell) )
	{
		SysMessageDefault(DEFMSG_SPELL_TRY_AM);
		m_Act_Difficulty = -1;		// give very little credit for failure
		return false;
	}
	return true;
}

bool CChar::Spell_TargCheck()
{
	ADDTOCALLSTACK("CChar::Spell_TargCheck");
	// Is the spells target or target pos valid ?

	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_Spell);
	if ( !pSpellDef )
	{
		DEBUG_ERR(("Bad Spell %d, uid 0%0lx\n", m_atMagery.m_Spell, static_cast<DWORD>(GetUID())));
		return false;
	}

	CObjBase *pObj = m_Act_Targ.ObjFind();
	CObjBaseTemplate *pObjTop = NULL;
	if ( pObj )
		pObjTop = pObj->GetTopLevelObj();

	// NOTE: Targeting a field spell directly on a char should not be allowed ?
	if ( pSpellDef->IsSpellType(SPELLFLAG_FIELD) && !pSpellDef->IsSpellType(SPELLFLAG_TARG_CHAR) )
	{
		if ( m_Act_Targ.IsValidUID() && m_Act_Targ.IsChar() )
		{
			SysMessageDefault(DEFMSG_SPELL_TARG_FIELDC);
			return false;
		}
	}

	// Need a target.
	if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_OBJ) && !(!pObj && pSpellDef->IsSpellType(SPELLFLAG_TARG_XYZ)) )
	{
		if ( !pObj || !pObjTop )
		{
			SysMessageDefault(DEFMSG_SPELL_TARG_OBJ);
			return false;
		}
		if ( !CanSee(pObj) || !CanSeeLOS(pObj, LOS_NB_WINDOWS) ) //we should be able to cast through a window
		{
			SysMessageDefault(DEFMSG_SPELL_TARG_LOS);
			return false;
		}
		if ( (pObjTop != this) && (pObjTop != pObj) && pObjTop->IsChar() && !IsPriv(PRIV_GM) )
		{
			SysMessageDefault(DEFMSG_SPELL_TARG_CONT);
			return false;
		}

		m_Act_p = pObjTop->GetTopPoint();
		if ( !Spell_TargCheck_Face() )
			return false;
	}
	else if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_XYZ) )
	{
		if ( pObj )
			m_Act_p = pObjTop->GetTopPoint();

		if ( !CanSeeLOS(m_Act_p, NULL, UO_MAP_VIEW_SIGHT, LOS_NB_WINDOWS) )		// we should be able to cast through a window
		{
			SysMessageDefault(DEFMSG_SPELL_TARG_LOS);
			return false;
		}
		if ( !Spell_TargCheck_Face() )
			return false;
	}
	return true;
}

bool CChar::Spell_Unequip(LAYER_TYPE layer)
{
	ADDTOCALLSTACK("CChar::Spell_Unequip");
	CItem *pItem = LayerFind(layer);
	if ( pItem )
	{
		if ( IsSetMagicFlags(MAGICF_NOCASTFROZENHANDS) && IsStatFlag(STATF_Freeze) )
		{
			SysMessageDefault(DEFMSG_SPELL_TRY_FROZENHANDS);
			return false;
		}
		else if ( !CanMove(pItem) )
			return false;
		else if ( !pItem->IsTypeSpellbook() && !pItem->IsType(IT_WAND) && !pItem->GetDefNum("SpellChanneling", true) )
			ItemBounce(pItem);
	}
	return true;
}

bool CChar::Spell_CastDone()
{
	ADDTOCALLSTACK("CChar::Spell_CastDone");
	// Spell_CastDone
	// Ready for the spell effect.
	// m_Act_TargPrv = spell was magic item or scroll ?
	// RETURN:
	//  false = fail.
	// ex. magery skill goes up FAR less if we use a scroll or magic device !

	if ( !Spell_TargCheck() )
		return false;

	CObjBase *pObj = m_Act_Targ.ObjFind();	// dont always need a target.
	CObjBase *pObjSrc = m_Act_TargPrv.ObjFind();
	ITEMID_TYPE iT1 = ITEMID_NOTHING;
	ITEMID_TYPE iT2 = ITEMID_NOTHING;
	CREID_TYPE iC1 = CREID_INVALID;
	HUE_TYPE iColor = HUE_DEFAULT;

	unsigned int fieldWidth = 0;
	unsigned int fieldGauge = 0;
	unsigned int areaRadius = 0;

	SPELL_TYPE spell = m_atMagery.m_Spell;
	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
	if ( !pSpellDef )
		return false;

	bool bIsSpellField = pSpellDef->IsSpellType(SPELLFLAG_FIELD);

	int iSkill, iDifficulty;
	if ( !pSpellDef->GetPrimarySkill(&iSkill, &iDifficulty) )
		return false;

	int iSkillLevel;
	if ( pObjSrc != this )
	{
		// Get the strength of the item. IT_SCROLL or IT_WAND
		CItem *pItem = dynamic_cast<CItem *>(pObjSrc);
		if ( !pItem )
			return false;
		if ( !pItem->m_itWeapon.m_spelllevel )
			iSkillLevel = Calc_GetRandVal(500);
		else
			iSkillLevel = pItem->m_itWeapon.m_spelllevel;
	}
	else
		iSkillLevel = Skill_GetAdjusted(static_cast<SKILL_TYPE>(iSkill));

	if ( (iSkill == SKILL_MYSTICISM) && (g_Cfg.m_iRacialFlags & RACIALF_GARG_MYSTICINSIGHT) && (iSkillLevel < 300) && IsGargoyle() )
		iSkillLevel = 300;	// Racial trait (Mystic Insight). Gargoyles always have a minimum of 30.0 Mysticism.

	CScriptTriggerArgs	Args(spell, iSkillLevel, pObjSrc);
	Args.m_VarsLocal.SetNum("FieldWidth", 0);
	Args.m_VarsLocal.SetNum("FieldGauge", 0);
	Args.m_VarsLocal.SetNum("AreaRadius", 0);
	Args.m_VarsLocal.SetNum("Duration", GetSpellDuration(spell, iSkillLevel, this), true);

	if ( bIsSpellField )
	{
		switch ( spell )	// Only setting ids and locals for field spells
		{
			case SPELL_Wall_of_Stone: 	iT1 = ITEMID_STONE_WALL;		iT2 = ITEMID_STONE_WALL;		break;
			case SPELL_Fire_Field: 		iT1 = ITEMID_FX_FIRE_F_EW; 		iT2 = ITEMID_FX_FIRE_F_NS;		break;
			case SPELL_Poison_Field:	iT1 = ITEMID_FX_POISON_F_EW;	iT2 = ITEMID_FX_POISON_F_NS;	break;
			case SPELL_Paralyze_Field:	iT1 = ITEMID_FX_PARA_F_EW;		iT2 = ITEMID_FX_PARA_F_NS;		break;
			case SPELL_Energy_Field:	iT1 = ITEMID_FX_ENERGY_F_EW;	iT2 = ITEMID_FX_ENERGY_F_NS;	break;
			default: break;
		}

		Args.m_VarsLocal.SetNum("CreateObject1", iT1, false);
		Args.m_VarsLocal.SetNum("CreateObject2", iT2, false);
	}

	if ( IsTrigUsed(TRIGGER_SPELLSUCCESS) )
	{
		if ( OnTrigger(CTRIG_SpellSuccess, this, &Args) == TRIGRET_RET_TRUE )
			return false;
	}

	if ( IsTrigUsed(TRIGGER_SUCCESS) )
	{
		if ( Spell_OnTrigger(spell, SPTRIG_SUCCESS, this, &Args) == TRIGRET_RET_TRUE )
			return false;
	}

	iSkillLevel = static_cast<int>(Args.m_iN2);
	ITEMID_TYPE it1test = ITEMID_NOTHING;
	ITEMID_TYPE it2test = ITEMID_NOTHING;

	if ( bIsSpellField )
	{
		//Setting new IDs as another variables to pass as different arguments to the field function.
		it1test = static_cast<ITEMID_TYPE>(RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("CreateObject1")));
		it2test = static_cast<ITEMID_TYPE>(RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("CreateObject2")));
		//Can't be < 0, so max it to 0
		fieldWidth = static_cast<unsigned int>(maximum(0, Args.m_VarsLocal.GetKeyNum("FieldWidth")));
		fieldGauge = static_cast<unsigned int>(maximum(0, Args.m_VarsLocal.GetKeyNum("FieldGauge")));

	}

	iC1 = static_cast<CREID_TYPE>(Args.m_VarsLocal.GetKeyNum("CreateObject1") & 0xFFFF);
	areaRadius = static_cast<unsigned int>(maximum(0, Args.m_VarsLocal.GetKeyNum("AreaRadius")));
	int iDuration = maximum(0, static_cast<int>(Args.m_VarsLocal.GetKeyNum("Duration")));
	iColor = static_cast<HUE_TYPE>(maximum(0, Args.m_VarsLocal.GetKeyNum("EffectColor")));

	// Consume the reagents/mana/scroll/charge
	if ( !Spell_CanCast(spell, false, pObjSrc, true) )
		return false;

	if ( pSpellDef->IsSpellType(SPELLFLAG_SCRIPTED) )
	{
		if ( pSpellDef->IsSpellType(SPELLFLAG_SUMMON) )
		{
			if ( iC1 )
			{
				m_atMagery.m_SummonID = iC1;
				Spell_Summon(m_atMagery.m_SummonID, m_Act_p);
			}
		}
		else if ( bIsSpellField )
		{
			if ( iT1 && iT2 )
			{
				if ( !fieldWidth )
					fieldWidth = 3;
				if ( !fieldGauge )
					fieldGauge = 1;

				Spell_Field(m_Act_p, iT1, iT2, fieldWidth, fieldGauge, iSkillLevel, this, it1test, it2test, iDuration, iColor);
			}
		}
		else if ( pSpellDef->IsSpellType(SPELLFLAG_AREA) )
		{
			if ( !areaRadius )
				areaRadius = 4;

			if ( !pSpellDef->IsSpellType(SPELLFLAG_TARG_OBJ|SPELLFLAG_TARG_XYZ) )
				Spell_Area(GetTopPoint(), areaRadius, iSkillLevel);
			else
				Spell_Area(m_Act_p, areaRadius, iSkillLevel);
		}
		else if ( pSpellDef->IsSpellType(SPELLFLAG_POLY) )
			return false;
		else if ( pObj )
			pObj->OnSpellEffect(spell, this, iSkillLevel, dynamic_cast<CItem *>(pObjSrc));
	}
	else if ( bIsSpellField )
	{
		if ( !fieldWidth )
			fieldWidth = 3;
		if ( !fieldGauge )
			fieldGauge = 1;

		Spell_Field(m_Act_p, iT1, iT2, fieldWidth, fieldGauge, iSkillLevel, this, it1test, it2test, iDuration, iColor);
	}
	else if ( pSpellDef->IsSpellType(SPELLFLAG_AREA) )
	{
		if ( !areaRadius )
		{
			switch ( spell )
			{
				case SPELL_Arch_Cure:		areaRadius = 2;							break;
				case SPELL_Arch_Prot:		areaRadius = 3;							break;
				case SPELL_Mass_Curse:		areaRadius = 2;							break;
				case SPELL_Reveal:			areaRadius = 1 + (iSkillLevel / 200);	break;
				case SPELL_Chain_Lightning: areaRadius = 2;							break;
				case SPELL_Mass_Dispel:		areaRadius = 8;							break;
				case SPELL_Meteor_Swarm:	areaRadius = 2;							break;
				case SPELL_Earthquake:		areaRadius = 1 + (iSkillLevel / 150);	break;
				case SPELL_Poison_Strike:	areaRadius = 2;							break;
				case SPELL_Wither:			areaRadius = 4;							break;
				default:					areaRadius = 4;							break;
			}
		}

		if ( !pSpellDef->IsSpellType(SPELLFLAG_TARG_OBJ|SPELLFLAG_TARG_XYZ) )
			Spell_Area(GetTopPoint(), areaRadius, iSkillLevel);
		else
			Spell_Area(m_Act_p, areaRadius, iSkillLevel);
	}
	else if ( pSpellDef->IsSpellType(SPELLFLAG_SUMMON) )
	{
		if ( iC1 )
			m_atMagery.m_SummonID = iC1;
		else
		{
			switch ( spell )
			{
				case SPELL_Blade_Spirit:	m_atMagery.m_SummonID = CREID_BLADES;		break;
				case SPELL_Vortex:			m_atMagery.m_SummonID = CREID_VORTEX;		break;
				case SPELL_Air_Elem:		m_atMagery.m_SummonID = CREID_AIR_ELEM;		break;
				case SPELL_Daemon:			m_atMagery.m_SummonID = CREID_DAEMON;		break;
				case SPELL_Earth_Elem:		m_atMagery.m_SummonID = CREID_EARTH_ELEM;	break;
				case SPELL_Fire_Elem:		m_atMagery.m_SummonID = CREID_FIRE_ELEM;	break;
				case SPELL_Water_Elem:		m_atMagery.m_SummonID = CREID_WATER_ELEM;	break;
				case SPELL_Vengeful_Spirit:	m_atMagery.m_SummonID = CREID_REVENANT;		break;
				case SPELL_Summon_Undead:
				{
					switch ( Calc_GetRandVal(15) )
					{
						case 1:				m_atMagery.m_SummonID = CREID_LICH;			break;
						case 3:
						case 5:
						case 7:
						case 9:				m_atMagery.m_SummonID = CREID_SKELETON;		break;
						default:			m_atMagery.m_SummonID = CREID_ZOMBIE;		break;
					}
				}
				default: break;
			}
		}
		Spell_Summon(m_atMagery.m_SummonID, m_Act_p);
	}
	else
	{
		iT1 = it1test;	// Set iT1 to it1test here because spell_field() needed both values to be passed.

		switch ( spell )
		{
			case SPELL_Create_Food:
			{
				RESOURCE_ID food = g_Cfg.ResourceGetIDType(RES_ITEMDEF, "DEFFOOD");
				CItem *pItem = CItem::CreateScript((iT1 ? iT1 : static_cast<ITEMID_TYPE>(food.GetResIndex())), this);
				ASSERT(pItem);
				if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_OBJ|SPELLFLAG_TARG_XYZ) )
					pItem->MoveToCheck(m_Act_p, this);
				else
				{
					ItemBounce(pItem, false);
					SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_SPELL_CREATE_FOOD), pItem->GetName());
				}
				break;
			}
			case SPELL_Magic_Trap:
			case SPELL_Magic_Untrap:
			{
				// Create the trap object and link it to the target.
				// A container is diff from door or stationary object
				break;
			}
			case SPELL_Telekin:	// Act as DClick on the object.
			{
				CItemCorpse *pCorpse = dynamic_cast<CItemCorpse *>(pObj->GetTopLevelObj());
				if ( pCorpse && pCorpse->m_uidLink != GetUID() )
				{
					CheckCorpseCrime(pCorpse, true, false);
					Reveal();
				}
				Use_Obj(pObj, false);
				break;
			}
			case SPELL_Teleport:
			{
				Spell_Teleport(m_Act_p);
				break;
			}
			case SPELL_Recall:
			{
				if ( !Spell_Recall(dynamic_cast<CItem *>(pObj), false) )
					return false;
				break;
			}
			case SPELL_Dispel_Field:
			{
				CItem *pItem = dynamic_cast<CItem *>(pObj);
				if ( !pItem || pItem->IsAttr(ATTR_MOVE_NEVER) || !pItem->IsType(IT_SPELL) )
				{
					SysMessageDefault(DEFMSG_SPELL_DISPELLF_WT);
					return false;
				}
				pItem->OnSpellEffect(SPELL_Dispel_Field, this, iSkillLevel, NULL);
				break;
			}
			case SPELL_Mind_Blast:
			{
				if ( pObj->IsChar() )
				{
					CChar *pChar = dynamic_cast<CChar *>(pObj);
					ASSERT(pChar);
					int iDiff = (Stat_GetAdjusted(STAT_INT) - pChar->Stat_GetAdjusted(STAT_INT)) / 2;
					if ( iDiff < 0 )
					{
						pChar = this;	// spell revereses !
						iDiff = -iDiff;
					}
					int iMax = pChar->Stat_GetMax(STAT_STR) / 2;
					pChar->OnSpellEffect(spell, this, minimum(iDiff, iMax), NULL);
				}
				break;
			}
			case SPELL_Flame_Strike:
			{
				// Display spell.
				if ( pObj )
					pObj->OnSpellEffect(spell, this, iSkillLevel, dynamic_cast<CItem *>(pObjSrc));
				else
				{
					CItem *pItem = CItem::CreateBase(iT1 ? iT1 : ITEMID_FX_FLAMESTRIKE);
					ASSERT(pItem);
					pItem->SetType(IT_SPELL);
					pItem->m_itSpell.m_spell = SPELL_Flame_Strike;
					pItem->MoveToDecay(m_Act_p, 2 * TICK_PER_SEC);
				}
				break;
			}
			case SPELL_Gate_Travel:
			{
				if ( !Spell_Recall(dynamic_cast<CItem *>(pObj), true) )
					return false;
				break;
			}
			case SPELL_Polymorph:
			case SPELL_Wraith_Form:
			case SPELL_Horrific_Beast:
			case SPELL_Lich_Form:
			case SPELL_Vampiric_Embrace:
			case SPELL_Stone_Form:
			case SPELL_Reaper_Form:
			{
				// This has a menu select for client.
				if ( !pObj || ((pObj != this) && (GetPrivLevel() < PLEVEL_Seer)) )
					return false;
				pObj->OnSpellEffect(spell, this, iSkillLevel, dynamic_cast<CItem *>(pObjSrc));
				break;
			}
			case SPELL_Animate_Dead:
			{
				CItemCorpse *pCorpse = dynamic_cast<CItemCorpse *>(pObj);
				if ( !pCorpse )
				{
					SysMessageDefault(DEFMSG_SPELL_ANIMDEAD_NC);
					return false;
				}
				if ( !pCorpse->IsTopLevel() )
					return false;

				if ( IsPriv(PRIV_GM) )
					m_atMagery.m_SummonID = pCorpse->m_itCorpse.m_BaseID;
				else if ( CCharBase::IsPlayableID(pCorpse->GetCorpseType()) ) 	// must be a human corpse?
					m_atMagery.m_SummonID = CREID_ZOMBIE;
				else
					m_atMagery.m_SummonID = pCorpse->GetCorpseType();

				CChar *pChar = Spell_Summon(m_atMagery.m_SummonID, pCorpse->GetTopPoint());
				ASSERT(pChar);
				if ( !pChar->RaiseCorpse(pCorpse) )
				{
					SysMessageDefault(DEFMSG_SPELL_ANIMDEAD_FAIL);
					pChar->Delete();
				}
				break;
			}
			case SPELL_Bone_Armor:
			{
				CItemCorpse *pCorpse = dynamic_cast<CItemCorpse *>(pObj);
				if ( !pCorpse )
				{
					SysMessage("That is not a corpse!");
					return false;
				}
				if ( !pCorpse->IsTopLevel() || (pCorpse->GetCorpseType() != CREID_SKELETON) ) 	// Must be a skeleton corpse
				{
					SysMessage("The body stirs for a moment");
					return false;
				}
				// Dump any stuff on corpse
				pCorpse->ContentsDump(pCorpse->GetTopPoint());
				pCorpse->Delete();

				static const ITEMID_TYPE sm_Item_Bone[] =
				{
					ITEMID_BONE_ARMS,
					ITEMID_BONE_ARMOR,
					ITEMID_BONE_GLOVES,
					ITEMID_BONE_HELM,
					ITEMID_BONE_LEGS
				};

				int iGet = 0;
				for ( size_t i = 0; i < COUNTOF(sm_Item_Bone); i++ )
				{
					if ( !Calc_GetRandVal(2 + iGet) )
						break;
					CItem *pItem = CItem::CreateScript(sm_Item_Bone[i], this);
					pItem->MoveToCheck(m_Act_p, this);
					iGet++;
				}
				if ( !iGet )
				{
					SysMessage("The bones shatter into dust!");
					break;
				}
				break;
			}
			default:
			{
				if ( !pObj )
					return false;
				pObj->OnSpellEffect(spell, this, iSkillLevel, dynamic_cast<CItem *>(pObjSrc));
				break;
			}
		}
	}

	if ( g_Cfg.m_fHelpingCriminalsIsACrime && pSpellDef->IsSpellType(SPELLFLAG_GOOD) && pObj && pObj->IsChar() && (pObj != this) )
	{
		CChar *pChar = dynamic_cast<CChar *>(pObj);
		ASSERT(pChar);
		switch ( pChar->Noto_GetFlag(this) )
		{
			case NOTO_CRIMINAL:
			case NOTO_GUILD_WAR:
			case NOTO_EVIL:
				Noto_Criminal();
				break;
			default:
				break;
		}
	}

	// If we are visible, play sound.
	if ( !IsStatFlag(STATF_Insubstantial) )
		Sound(pSpellDef->m_sound);

	// At this point we should gain skill if precasting is enabled
	if ( m_pClient && IsSetMagicFlags(MAGICF_PRECAST) && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST) )
	{
		iDifficulty /= 10;
		Skill_Experience(static_cast<SKILL_TYPE>(iSkill), iDifficulty);
	}
	return true;
}

void CChar::Spell_CastFail()
{
	ADDTOCALLSTACK("CChar::Spell_CastFail");
	ITEMID_TYPE iT1 = ITEMID_FX_SPELL_FAIL;
	CScriptTriggerArgs	Args(m_atMagery.m_Spell, 0, m_Act_TargPrv.ObjFind());
	Args.m_VarsLocal.SetNum("CreateObject1", iT1);
	if ( IsTrigUsed(TRIGGER_SPELLFAIL) )
	{
		if ( OnTrigger(CTRIG_SpellFail, this, &Args) == TRIGRET_RET_TRUE )
			return;
	}

	if ( IsTrigUsed(TRIGGER_FAIL) )
	{
		if ( Spell_OnTrigger(m_atMagery.m_Spell, SPTRIG_FAIL, this, &Args) == TRIGRET_RET_TRUE )
			return;
	}

	HUE_TYPE iColor = static_cast<HUE_TYPE>(maximum(0, Args.m_VarsLocal.GetKeyNum("EffectColor")));
	DWORD iRender = static_cast<DWORD>(maximum(0, Args.m_VarsLocal.GetKeyNum("EffectRender")));

	iT1 = static_cast<ITEMID_TYPE>(RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("CreateObject1")));
	if ( iT1 )
		Effect(EFFECT_OBJ, iT1, this, 1, 30, false, iColor, iRender);
	Sound(SOUND_SPELL_FIZZLE);

	if ( m_pClient )
		m_pClient->addObjMessage(g_Cfg.GetDefaultMsg(DEFMSG_SPELL_GEN_FIZZLES), this);
	if ( g_Cfg.m_fReagentLossFail )
		Spell_CanCast(m_atMagery.m_Spell, false, m_Act_TargPrv.ObjFind(), false);	// consume reagents
}

int CChar::Spell_CastStart()
{
	ADDTOCALLSTACK("CChar::Spell_CastStart");
	// Casting time goes up with difficulty
	// but down with skill, int and dex
	// ARGS:
	//  m_Act_p = location to cast to.
	//  m_atMagery.m_Spell = the spell.
	//  m_Act_TargPrv = the source of the spell.
	//  m_Act_Targ = target for the spell.
	// RETURN:
	//  0-100
	//  -1 = instant failure.

	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_Spell);
	if ( !pSpellDef )
		return -1;

	if ( m_pClient && IsSetMagicFlags(MAGICF_PRECAST) && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST) )
	{
		m_Act_p = GetTopPoint();
		m_Act_Targ = m_pClient->m_Targ_UID;
		m_Act_TargPrv = m_pClient->m_Targ_PrvUID;

		if ( !Spell_CanCast(m_atMagery.m_Spell, true, m_Act_TargPrv.ObjFind(), true) )
			return -1;
	}
	else
	{
		if ( !Spell_TargCheck() )
			return -1;
	}

	int iSkill, iDifficulty;
	if ( !pSpellDef->GetPrimarySkill(&iSkill, &iDifficulty) )
		return -1;

	iDifficulty /= 10;		// adjust to 0 - 100
	bool fWOP = (GetPrivLevel() >= PLEVEL_Counsel) ? g_Cfg.m_fWordsOfPowerStaff : g_Cfg.m_fWordsOfPowerPlayer;
	if ( !NPC_CanSpeak() || IsStatFlag(STATF_Insubstantial) )
		fWOP = false;

	bool fAllowEquip = false;
	CItem *pItem = m_Act_TargPrv.ItemFind();
	if ( pItem )
	{
		if ( pItem->IsType(IT_WAND) )
		{
			// Wand use no words of power. and require no magery.
			fAllowEquip = true;
			fWOP = false;
			iDifficulty = 1;
		}
		else
		{
			// Scroll
			iDifficulty /= 2;
		}
	}

	INT64 iWaitTime = pSpellDef->m_CastTime.GetLinear(Skill_GetBase(static_cast<SKILL_TYPE>(iSkill)));
	iWaitTime -= GetDefNum("FasterCasting", true) * 2;	//correct value is 0.25, but sphere can handle only 0.2.
	if ( iWaitTime < 1 || IsPriv(PRIV_GM) )
		iWaitTime = 1;

	CScriptTriggerArgs Args(static_cast<int>(m_atMagery.m_Spell), iDifficulty, pItem);
	Args.m_iN3 = iWaitTime;
	Args.m_VarsLocal.SetNum("WOP", fWOP);
	Args.m_VarsLocal.SetNum("WOPColor", g_Cfg.m_iWordsOfPowerColor > 0 ? g_Cfg.m_iWordsOfPowerColor : m_SpeechHue, true);
	Args.m_VarsLocal.SetNum("WOPFont", g_Cfg.m_iWordsOfPowerFont, true);

	if ( IsTrigUsed(TRIGGER_SPELLCAST) )
	{
		if ( OnTrigger(CTRIG_SpellCast, this, &Args) == TRIGRET_RET_TRUE )
			return -1;
	}

	if ( IsTrigUsed(TRIGGER_START) )
	{
		if ( Spell_OnTrigger(static_cast<SPELL_TYPE>(Args.m_iN1), SPTRIG_START, this, &Args) == TRIGRET_RET_TRUE )
			return -1;
	}

	// Attempt to unequip stuff before casting (except wands, spellbooks and items with SPELLCHANNELING property set)
	if ( !g_Cfg.m_fEquippedCast && !fAllowEquip )
	{
		if ( !Spell_Unequip(LAYER_HAND1) )
			return -1;
		if ( !Spell_Unequip(LAYER_HAND2) )
			return -1;
	}

	m_atMagery.m_Spell = static_cast<SPELL_TYPE>(Args.m_iN1);
	iDifficulty = static_cast<int>(Args.m_iN2);
	iWaitTime = Args.m_iN3;

	pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_Spell);
	if ( !pSpellDef )
		return -1;

	if ( g_Cfg.m_iRevealFlags & REVEALF_SPELLCAST )
		Reveal(STATF_Hidden|STATF_Invisible);
	else
		Reveal(STATF_Hidden);

	// Animate casting
	if ( !pSpellDef->IsSpellType(SPELLFLAG_NO_CASTANIM) && !IsSetMagicFlags(MAGICF_NOANIM) )
		UpdateAnimate(pSpellDef->IsSpellType(SPELLFLAG_DIR_ANIM) ? ANIM_CAST_DIR : ANIM_CAST_AREA);

	fWOP = (Args.m_VarsLocal.GetKeyNum("WOP") > 0);
	if ( fWOP )
	{
		HUE_TYPE WOPColor = static_cast<HUE_TYPE>(Args.m_VarsLocal.GetKeyNum("WOPColor"));
		FONT_TYPE WOPFont = static_cast<FONT_TYPE>(Args.m_VarsLocal.GetKeyNum("WOPFont"));

		// Correct talk mode for spells WOP is TALKMODE_SPELL, but since sphere doesn't have any delay between spell casts this can allow WOP flood on screen.
		// So to avoid this problem we must use TALKMODE_SAY, which is not the correct type but with this type the client only show last 3 messages on screen.
		if ( pSpellDef->m_sRunes[0] == '.' )
			Speak((pSpellDef->m_sRunes.GetPtr()) + 1, WOPColor, TALKMODE_SAY, WOPFont);
		else
		{
			TCHAR *pszTemp = Str_GetTemp();
			size_t len = 0;
			for ( size_t i = 0; ; i++ )
			{
				TCHAR ch = pSpellDef->m_sRunes[i];
				if ( !ch )
					break;
				len += strcpylen(pszTemp + len, g_Cfg.GetRune(ch));
				if ( pSpellDef->m_sRunes[i + 1] )
					pszTemp[len++] = ' ';
			}
			if ( len > 0 )
			{
				pszTemp[len] = 0;
				Speak(pszTemp, WOPColor, TALKMODE_SAY, WOPFont);
			}
		}
	}
	SetTimeout(iWaitTime);
	return iDifficulty;
}

bool CChar::OnSpellEffect(SPELL_TYPE spell, CChar *pCharSrc, int iSkillLevel, CItem *pSourceItem, bool bReflecting)
{
	ADDTOCALLSTACK("CChar::OnSpellEffect");
	// Spell has a direct effect on this char.
	// This should effect noto of source.
	// ARGS:
	//  pSourceItem = the potion, wand, scroll etc. NULL = cast (IT_SPELL)
	//  iSkillLevel = 0-1000 = difficulty. may be slightly larger .
	// RETURN:
	//  false = the spell did not work. (should we get credit ?)

	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
	if ( !pSpellDef )
		return false;
	if ( iSkillLevel <= 0 )		// spell died or fizzled
		return false;
	if ( IsStatFlag(STATF_DEAD) && !pSpellDef->IsSpellType(SPELLFLAG_TARG_DEAD) )
		return false;
	if ( spell == SPELL_Paralyze_Field && IsStatFlag(STATF_Freeze) )
		return false;
	if ( spell == SPELL_Poison_Field && IsStatFlag(STATF_Poisoned) )
		return false;

	iSkillLevel = iSkillLevel / 2 + Calc_GetRandVal(iSkillLevel / 2);	// randomize the potency
	int iEffect = g_Cfg.GetSpellEffect(spell, iSkillLevel);
	int iDuration = pSpellDef->m_idLayer ? GetSpellDuration(spell, iSkillLevel, pCharSrc) : 0;
	SOUND_TYPE iSound = pSpellDef->m_sound;
	bool fExplode = (pSpellDef->IsSpellType(SPELLFLAG_FX_BOLT) && !pSpellDef->IsSpellType(SPELLFLAG_GOOD));		// bolt (chasing) spells have explode = 1 by default (if not good spell)
	bool fPotion = (pSourceItem && pSourceItem->IsType(IT_POTION));
	if ( fPotion )
	{
		static const SOUND_TYPE sm_DrinkSounds[] = { 0x030, 0x031 };
		iSound = sm_DrinkSounds[Calc_GetRandVal(COUNTOF(sm_DrinkSounds))];
	}

	// Check if the spell is being resisted
	int iResist = 0;
	if ( pSpellDef->IsSpellType(SPELLFLAG_RESIST) && pCharSrc && !fPotion )
	{
		iResist = Skill_GetBase(SKILL_MAGICRESISTANCE);
		int iFirst = iResist / 50;
		int iSecond = iResist - (((pCharSrc->Skill_GetBase(SKILL_MAGERY) - 200) / 50) + (1 + (spell / 8)) * 50);
		int iResistChance = maximum(iFirst, iSecond) / 30;
		iResist = Skill_UseQuick(SKILL_MAGICRESISTANCE, iResistChance, true, false) ? 25 : 0;	// If we successfully resist then we have a 25% damage reduction, 0 if we don't.

		if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B )
		{
			CItem *pEvilOmen = LayerFind(LAYER_SPELL_Evil_Omen);
			if ( pEvilOmen )
				iResist /= 2;	// Effect 3: Only 50% of magic resistance used in next resistable spell.
		}
	}

	if ( pSpellDef->IsSpellType(SPELLFLAG_DAMAGE) )
	{
		if ( IsSetMagicFlags(MAGICF_OSIFORMULAS) )
		{
			if ( !pCharSrc )
				iEffect *= ((iSkillLevel * 3) / 1000) + 1;
			else
			{
				// Evaluating Intelligence mult
				iEffect *= ((pCharSrc->Skill_GetBase(SKILL_EVALINT) * 3) / 1000) + 1;

				// Spell Damage Increase bonus
				int DamageBonus = static_cast<int>(pCharSrc->GetDefNum("IncreaseSpellDam"));
				if ( (DamageBonus > 15) && m_pPlayer && pCharSrc->m_pPlayer )		// Spell Damage Increase is capped at 15% on PvP
					DamageBonus = 15;

				// INT bonus
				DamageBonus += pCharSrc->Stat_GetAdjusted(STAT_INT) / 10;

				// Inscription bonus
				DamageBonus += pCharSrc->Skill_GetBase(SKILL_INSCRIPTION) / 100;

				// Racial Bonus (Berserk), gargoyles gains +3% Spell Damage Increase per each 20 HP lost
				if ( (g_Cfg.m_iRacialFlags & RACIALF_GARG_BERSERK) && IsGargoyle() )
					DamageBonus += minimum(3 * ((Stat_GetMax(STAT_STR) - Stat_GetVal(STAT_STR)) / 20), 12);		// value is capped at 12%

				iEffect += iEffect * DamageBonus / 100;
			}
		}
	}

	CScriptTriggerArgs Args(static_cast<int>(spell), iSkillLevel, pSourceItem);
	Args.m_VarsLocal.SetNum("DamageType", 0);
	Args.m_VarsLocal.SetNum("CreateObject1", pSpellDef->m_idEffect);
	Args.m_VarsLocal.SetNum("Explode", fExplode);
	Args.m_VarsLocal.SetNum("Sound", iSound);
	Args.m_VarsLocal.SetNum("Effect", iEffect);
	Args.m_VarsLocal.SetNum("Resist", iResist);
	Args.m_VarsLocal.SetNum("Duration", iDuration);

	if ( IsTrigUsed(TRIGGER_SPELLEFFECT) )
	{
		switch ( OnTrigger(CTRIG_SpellEffect, pCharSrc ? pCharSrc : this, &Args) )
		{
			case TRIGRET_RET_TRUE:	return false;
			case TRIGRET_RET_FALSE:	if ( pSpellDef->IsSpellType(SPELLFLAG_SCRIPTED) ) return true;
			default:				break;
		}
	}

	if ( IsTrigUsed(TRIGGER_EFFECT) )
	{
		switch ( Spell_OnTrigger(spell, SPTRIG_EFFECT, pCharSrc ? pCharSrc : this, &Args) )
		{
			case TRIGRET_RET_TRUE:	return false;
			case TRIGRET_RET_FALSE:	if ( pSpellDef->IsSpellType(SPELLFLAG_SCRIPTED) ) return true;
			default:				break;
		}
	}

	spell = static_cast<SPELL_TYPE>(Args.m_iN1);
	iSkillLevel = static_cast<int>(Args.m_iN2);		// remember that effect/duration is calculated before triggers
	DAMAGE_TYPE iDmgType = static_cast<DAMAGE_TYPE>(RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("DamageType")));
	ITEMID_TYPE iEffectID = static_cast<ITEMID_TYPE>(RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("CreateObject1")));
	fExplode = (Args.m_VarsLocal.GetKeyNum("EffectExplode") > 0) ? true : false;
	iSound = static_cast<SOUND_TYPE>(Args.m_VarsLocal.GetKeyNum("Sound"));
	iEffect = static_cast<int>(Args.m_VarsLocal.GetKeyNum("Effect"));
	iResist = static_cast<int>(Args.m_VarsLocal.GetKeyNum("Resist"));
	iDuration = static_cast<int>(Args.m_VarsLocal.GetKeyNum("Duration"));

	HUE_TYPE iColor = static_cast<HUE_TYPE>(maximum(0, Args.m_VarsLocal.GetKeyNum("EffectColor")));
	DWORD iRender = static_cast<DWORD>(maximum(0, Args.m_VarsLocal.GetKeyNum("EffectRender")));

	if ( iEffectID > ITEMID_QTY )
		iEffectID = pSpellDef->m_idEffect;

	if ( pSpellDef->IsSpellType(SPELLFLAG_HARM) )
	{
		if ( (pCharSrc == this) && !IsSetMagicFlags(MAGICF_CANHARMSELF) && !bReflecting )
			return false;

		if ( IsStatFlag(STATF_INVUL) )
		{
			Effect(EFFECT_OBJ, ITEMID_FX_GLOW, this, 10, 16);
			return false;
		}
		else if ( GetPrivLevel() == PLEVEL_Guest )
		{
			if ( pCharSrc )
				pCharSrc->SysMessageDefault(DEFMSG_MSG_ACC_GUESTHIT);
			Effect(EFFECT_OBJ, ITEMID_FX_GLOW, this, 10, 16);
			return false;
		}

		if ( !OnAttackedBy(pCharSrc, false, !pSpellDef->IsSpellType(SPELLFLAG_FIELD)) && !bReflecting )
			return false;

		// Check if the spell can be reflected
		if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_CHAR) && pCharSrc && (pCharSrc != this) )	// only spells with direct target can be reflected
		{
			if ( IsStatFlag(STATF_Reflection) )
			{
				Effect(EFFECT_OBJ, ITEMID_FX_GLOW, this, 10, 5);
				CItem *pMagicReflect = LayerFind(LAYER_SPELL_Magic_Reflect);
				if ( pMagicReflect )
					pMagicReflect->Delete();

				if ( pCharSrc->IsStatFlag(STATF_Reflection) )		// caster is under reflection effect too, so the spell will reflect back to default target
				{
					pCharSrc->Effect(EFFECT_OBJ, ITEMID_FX_GLOW, pCharSrc, 10, 5);
					pMagicReflect = pCharSrc->LayerFind(LAYER_SPELL_Magic_Reflect);
					if ( pMagicReflect )
						pMagicReflect->Delete();
				}
				else
				{
					pCharSrc->OnSpellEffect(spell, pCharSrc, iSkillLevel, pSourceItem, true);
					return true;
				}
			}
		}
	}

	if ( pSpellDef->IsSpellType(SPELLFLAG_SCRIPTED) )
		return true;

	if ( iEffectID )
	{
		if ( pSpellDef->IsSpellType(SPELLFLAG_FX_BOLT) )
			Effect(EFFECT_BOLT, iEffectID, pCharSrc, 5, 1, fExplode, iColor, iRender);
		if ( pSpellDef->IsSpellType(SPELLFLAG_FX_TARG) )
			Effect(EFFECT_OBJ, iEffectID, this, 0, 15, fExplode, iColor, iRender);
	}
	if ( iSound )
		Sound(iSound);

	if ( pSpellDef->IsSpellType(SPELLFLAG_DAMAGE) )
	{
		if ( iResist > 0 )
		{
			SysMessageDefault(DEFMSG_RESISTMAGIC);
			iEffect -= iEffect * iResist / 100;
			if ( iEffect < 0 )
				iEffect = 0;	//May not do damage, but aversion should be created from the target.
		}
		if ( !iDmgType )
		{
			switch ( spell )
			{
				case SPELL_Magic_Arrow:
				case SPELL_Fireball:
				case SPELL_Fire_Field:
				case SPELL_Explosion:
				case SPELL_Flame_Strike:
				case SPELL_Meteor_Swarm:
				case SPELL_Fire_Bolt:
					iDmgType = DAMAGE_MAGIC|DAMAGE_FIRE|DAMAGE_NOREVEAL;
					break;
				case SPELL_Harm:
				case SPELL_Mind_Blast:
					iDmgType = DAMAGE_MAGIC|DAMAGE_COLD|DAMAGE_NOREVEAL;
					break;
				case SPELL_Lightning:
				case SPELL_Energy_Bolt:
				case SPELL_Chain_Lightning:
					iDmgType = DAMAGE_MAGIC|DAMAGE_ENERGY|DAMAGE_NOREVEAL;
					break;
				default:
					iDmgType = DAMAGE_MAGIC|DAMAGE_GENERAL|DAMAGE_NOREVEAL;
					break;
			}
		}

		// AOS damage types (used by COMBAT_ELEMENTAL_ENGINE)
		int iDmgPhysical = 0, iDmgFire = 0, iDmgCold = 0, iDmgPoison = 0, iDmgEnergy = 0;
		if ( iDmgType & DAMAGE_FIRE )
			iDmgFire = 100;
		else if ( iDmgType & DAMAGE_COLD )
			iDmgCold = 100;
		else if ( iDmgType & DAMAGE_POISON )
			iDmgPoison = 100;
		else if ( iDmgType & DAMAGE_ENERGY )
			iDmgEnergy = 100;
		else
			iDmgPhysical = 100;

		OnTakeDamage(iEffect, pCharSrc, iDmgType, iDmgPhysical, iDmgFire, iDmgCold, iDmgPoison, iDmgEnergy);
	}

	switch ( spell )
	{
		case SPELL_Clumsy:
		case SPELL_Feeblemind:
		case SPELL_Weaken:
		case SPELL_Curse:
		case SPELL_Agility:
		case SPELL_Cunning:
		case SPELL_Strength:
		case SPELL_Bless:
		case SPELL_Mana_Drain:
		case SPELL_Mass_Curse:
			Spell_Effect_Create(spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_STATS, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Heal:
		case SPELL_Great_Heal:
			UpdateStatVal(STAT_STR, static_cast<short>(iEffect));
			break;

		case SPELL_Night_Sight:
			Spell_Effect_Create(spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Night_Sight, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Reactive_Armor:
			Spell_Effect_Create(spell, LAYER_SPELL_Reactive, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Magic_Reflect:
			Spell_Effect_Create(spell, LAYER_SPELL_Magic_Reflect, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Poison:
		case SPELL_Poison_Field:
			if ( pCharSrc && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
				iSkillLevel = (pCharSrc->Skill_GetBase(SKILL_MAGERY) + pCharSrc->Skill_GetBase(SKILL_POISONING)) / 2;
			SetPoison(iSkillLevel, iSkillLevel / 50, pCharSrc);
			break;

		case SPELL_Cure:
			SetPoisonCure(iSkillLevel, iSkillLevel > 900);
			break;

		case SPELL_Arch_Cure:
			SetPoisonCure(iSkillLevel, true);
			break;

		case SPELL_Protection:
		case SPELL_Arch_Prot:
			Spell_Effect_Create(spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Protection, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Summon:
			Spell_Effect_Create(spell, LAYER_SPELL_Summon, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Dispel:
		case SPELL_Mass_Dispel:
			Spell_Dispel((pCharSrc && pCharSrc->IsPriv(PRIV_GM)) ? 150 : 50);	// should be difficult to dispel summoned creatures?
			break;

		case SPELL_Reveal:
			if ( !Reveal() )
				break;
			if ( iEffectID )
				Effect(EFFECT_OBJ, iEffectID, this, 0, 15, fExplode, iColor, iRender);
			break;

		case SPELL_Invis:
			Spell_Effect_Create(spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Invis, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Incognito:
			Spell_Effect_Create(spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Incognito, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Paralyze:
		case SPELL_Paralyze_Field:
		case SPELL_Stone:
		case SPELL_Particle_Form:
			Spell_Effect_Create(spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Paralyze, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Mana_Vamp:
		{
			int iMax = Stat_GetVal(STAT_INT);
			if ( pCharSrc && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
			{
				// AOS formula
				iSkillLevel = (pCharSrc->Skill_GetBase(SKILL_EVALINT) - Skill_GetBase(SKILL_MAGICRESISTANCE)) / 10;
				if ( !m_pPlayer )
					iSkillLevel /= 2;

				if ( iSkillLevel < 0 )
					iSkillLevel = 0;
				else if ( iSkillLevel > iMax )
					iSkillLevel = iMax;
			}
			else
			{
				// Pre-AOS formula
				iSkillLevel = iMax;
			}
			UpdateStatVal(STAT_INT, static_cast<short>(-iSkillLevel));
			if ( pCharSrc )
				pCharSrc->UpdateStatVal(STAT_INT, static_cast<short>(+iSkillLevel));
			break;
		}

		case SPELL_Meteor_Swarm:
			if ( iEffectID )
				Effect(EFFECT_BOLT, iEffectID, pCharSrc, 9, 6, fExplode, iColor, iRender);
			break;

		case SPELL_Lightning:
		case SPELL_Chain_Lightning:
			GetTopSector()->LightFlash();
			Effect(EFFECT_LIGHTNING, ITEMID_NOTHING, pCharSrc);
			break;

		case SPELL_Resurrection:
			return Spell_Resurrection(NULL, pCharSrc, (pSourceItem && pSourceItem->IsType(IT_SHRINE)));

		case SPELL_Light:
			if ( iEffectID )
				Effect(EFFECT_OBJ, iEffectID, this, 9, 6, fExplode, iColor, iRender);
			Spell_Effect_Create(spell, LAYER_FLAG_Potion, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Hallucination:
		{
			CItem *pItem = Spell_Effect_Create(spell, LAYER_FLAG_Hallucination, iSkillLevel, 10 * TICK_PER_SEC, pCharSrc);
			pItem->m_itSpell.m_spellcharges = Calc_GetRandVal(30);
			break;
		}

		case SPELL_Shrink:
		{
			if ( m_pPlayer )
				break;
			if ( fPotion )
				pSourceItem->Delete();

			CItem *pItem = NPC_Shrink(); // this delete's the char !!!
			if ( pCharSrc && pItem )
				pCharSrc->m_Act_Targ = pItem->GetUID();
			break;
		}

		case SPELL_Mana:
			UpdateStatVal(STAT_INT, static_cast<short>(iEffect));
			break;

		case SPELL_Refresh:
			UpdateStatVal(STAT_DEX, static_cast<short>(iEffect));
			break;

		case SPELL_Restore:		// increases both your hit points and your stamina.
			UpdateStatVal(STAT_DEX, static_cast<short>(iEffect));
			UpdateStatVal(STAT_STR, static_cast<short>(iEffect));
			break;

		case SPELL_Sustenance:		// 105 // serves to fill you up. (Remember, healing rate depends on how well fed you are!)
			Stat_SetVal(STAT_FOOD, Stat_GetAdjusted(STAT_FOOD));
			break;

		case SPELL_Gender_Swap:		// 110 // permanently changes your gender.
			if ( IsPlayableCharacter() )
			{
				CCharBase *pCharDef = Char_GetDef();
				ASSERT(pCharDef);

				if ( IsHuman() )
					SetID(pCharDef->IsFemale() ? CREID_MAN : CREID_WOMAN);
				else if ( IsElf() )
					SetID(pCharDef->IsFemale() ? CREID_ELFMAN : CREID_ELFWOMAN);
				else if ( IsGargoyle() )
					SetID(pCharDef->IsFemale() ? CREID_GARGMAN : CREID_GARGWOMAN);
				m_prev_id = GetID();
			}
			break;

		case SPELL_Wraith_Form:
		case SPELL_Horrific_Beast:
		case SPELL_Lich_Form:
		case SPELL_Vampiric_Embrace:
		case SPELL_Stone_Form:
		case SPELL_Reaper_Form:
		case SPELL_Polymorph:
			Spell_Effect_Create(spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Polymorph, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Chameleon:		// 106 // makes your skin match the colors of whatever is behind you.
		case SPELL_BeastForm:		// 107 // polymorphs you into an animal for a while.
		case SPELL_Monster_Form:	// 108 // polymorphs you into a monster for a while.
			Spell_Effect_Create(spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Polymorph, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Trance:			// 111 // temporarily increases your meditation skill.
			Spell_Effect_Create(spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_STATS, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Shield:			// 113 // erects a temporary force field around you. Nobody approaching will be able to get within 1 tile of you, though you can move close to them if you wish.
		case SPELL_Steelskin:		// 114 // turns your skin into steel, giving a boost to your AR.
		case SPELL_Stoneskin:		// 115 // turns your skin into stone, giving a boost to your AR.
			Spell_Effect_Create(spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Protection, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Regenerate:		// Set number of charges based on effect level.
		{
			iDuration /= (2 * TICK_PER_SEC);
			if ( iDuration <= 0 )
				iDuration = 1;
			CItem *pSpell = Spell_Effect_Create(spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_STATS, iSkillLevel, iDuration, pCharSrc);
			ASSERT(pSpell);
			pSpell->m_itSpell.m_spellcharges = iDuration;
			break;
		}

		case SPELL_Blood_Oath:		// Blood Oath is a pact created between the casted and the target, memory is stored on the caster because one caster can have only 1 enemy, but one target can have the effect from various spells.
			if ( pCharSrc )
				pCharSrc->Spell_Effect_Create(spell, LAYER_SPELL_Blood_Oath, iSkillLevel, iDuration, this);
			break;

		case SPELL_Corpse_Skin:
			Spell_Effect_Create(spell, LAYER_SPELL_Corpse_Skin, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Evil_Omen:
			Spell_Effect_Create(spell, LAYER_SPELL_Evil_Omen, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Mind_Rot:
			Spell_Effect_Create(spell, LAYER_SPELL_Mind_Rot, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Pain_Spike:
			Spell_Effect_Create(spell, LAYER_SPELL_Pain_Spike, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Strangle:
			Spell_Effect_Create(spell, LAYER_SPELL_Strangle, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Curse_Weapon:
			Spell_Effect_Create(spell, LAYER_SPELL_Curse_Weapon, iSkillLevel, iDuration, pCharSrc);
			break;

		default:
			break;
	}
	return true;
}

int CChar::GetSpellDuration(SPELL_TYPE spell, int iSkillLevel, CChar *pCharSrc)
{
	ADDTOCALLSTACK("CChar::GetSpellDuration");
	int iDuration = -1;
	if ( pCharSrc && (IsSetMagicFlags(MAGICF_OSIFORMULAS) || (spell >= SPELL_Animate_Dead_AOS)) )
	{
		switch ( spell )
		{
			case SPELL_Clumsy:
			case SPELL_Feeblemind:
			case SPELL_Weaken:
			case SPELL_Agility:
			case SPELL_Cunning:
			case SPELL_Strength:
			case SPELL_Bless:
			case SPELL_Curse:
				iDuration = 1 + ((6 * pCharSrc->Skill_GetBase(SKILL_EVALINT)) / 50);
				break;

			case SPELL_Protection:
			{
				iDuration = (2 * pCharSrc->Skill_GetBase(SKILL_MAGERY)) / 10;
				if ( iDuration < 15 )
					iDuration = 15;
				else if ( iDuration > 240 )
					iDuration = 240;
			}
			break;

			case SPELL_Wall_of_Stone:
				iDuration = 10;
				break;

			case SPELL_Arch_Prot:
			{
				iDuration = pCharSrc->Skill_GetBase(SKILL_MAGERY) * 12 / 100;
				if ( iDuration > 144 )
					iDuration = 144;
				break;
			}

			case SPELL_Fire_Field:
				iDuration = (15 + (pCharSrc->Skill_GetBase(SKILL_MAGERY) / 5)) / 4;
				break;

			case SPELL_Mana_Drain:
				iDuration = 5;
				break;

			case SPELL_Blade_Spirit:
				iDuration = 120;
				break;

			case SPELL_Incognito:
			{
				iDuration = 1 + ((6 * pCharSrc->Skill_GetBase(SKILL_MAGERY)) / 50);
				if ( iDuration > 144 )
					iDuration = 144;
				break;
			}

			case SPELL_Paralyze:
				// Pre-AOS formula
				iDuration = 7 + (pCharSrc->Skill_GetBase(SKILL_MAGERY) / 50);
				break;

				// AOS formula (it only works well on servers with skillcap)
				/*iDuration = (pCharSrc->Skill_GetBase(SKILL_EVALINT) / 10) - (Skill_GetBase(SKILL_MAGICRESISTANCE) / 10);
				if ( m_pNPC )
					iDuration *= 3;
				if ( iDuration < 0 )
					iDuration = 0;*/

			case SPELL_Poison_Field:
				iDuration = 3 + (pCharSrc->Skill_GetBase(SKILL_MAGERY) / 25);
				break;

			case SPELL_Invis:
				iDuration = pCharSrc->Skill_GetBase(SKILL_MAGERY) * 12 / 100;
				break;

			case SPELL_Paralyze_Field:
				iDuration = 3 + (pCharSrc->Skill_GetBase(SKILL_MAGERY) / 30);
				break;

			case SPELL_Energy_Field:
				iDuration = (15 + (pCharSrc->Skill_GetBase(SKILL_MAGERY) / 5)) / 7;
				break;

			case SPELL_Polymorph:
			{
				iDuration = pCharSrc->Skill_GetBase(SKILL_MAGERY) / 10;
				if ( iDuration > 120 )
					iDuration = 120;
				break;
			}

			case SPELL_Vortex:
				iDuration = 90;
				break;

			case SPELL_Summon:
			case SPELL_Air_Elem:
			case SPELL_Daemon:
			case SPELL_Earth_Elem:
			case SPELL_Fire_Elem:
			case SPELL_Water_Elem:
				iDuration = (2 * pCharSrc->Skill_GetBase(SKILL_MAGERY)) / 5;
				break;

			case SPELL_Blood_Oath:
				iDuration = 8 + ((pCharSrc->Skill_GetBase(SKILL_SPIRITSPEAK) - Skill_GetBase(SKILL_MAGICRESISTANCE)) / 80);
				break;

			case SPELL_Corpse_Skin:
				iDuration = 40 + ((pCharSrc->Skill_GetBase(SKILL_SPIRITSPEAK) - Skill_GetBase(SKILL_MAGICRESISTANCE)) / 25);
				break;

			case SPELL_Curse_Weapon:
				iDuration = 1 + (pCharSrc->Skill_GetBase(SKILL_SPIRITSPEAK) / 34);
				break;

			case SPELL_Mind_Rot:
				iDuration = 20 + ((pCharSrc->Skill_GetBase(SKILL_SPIRITSPEAK) - Skill_GetBase(SKILL_MAGICRESISTANCE)) / 50);
				break;

			case SPELL_Pain_Spike:
				iDuration = 1;		// timer is 1, but using 10 charges
				break;

			case SPELL_Strangle:
				iDuration = 5;
				break;

			default:
				break;
		}
	}

	if ( iDuration == -1 )
	{
		const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
		ASSERT(pSpellDef);
		iDuration = pSpellDef->m_Duration.GetLinear(iSkillLevel) / 10;
	}
	return iDuration * TICK_PER_SEC;
}
