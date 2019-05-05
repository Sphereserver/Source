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

bool CChar::Spell_Teleport(CPointMap ptDest, bool fTakePets, bool fCheckAntiMagic, bool fDisplayEffect, ITEMID_TYPE iEffect, SOUND_TYPE iSound)
{
	ADDTOCALLSTACK("CChar::Spell_Teleport");
	// Teleport you to this place.
	// This is sometimes not really a spell at all.
	// ex. ships plank.
	// RETURN: true = it worked.

	if ( !ptDest.IsValidXY() )
		return false;

	ptDest.m_z = GetFixZ(ptDest);

	if ( !IsPriv(PRIV_GM) )
	{
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

		if ( fCheckAntiMagic )
		{
			CRegionBase *pArea = CheckValidMove(ptDest, NULL, DIR_QTY, NULL);
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

		if ( g_Cfg.m_iMountHeight && !IsVerticalSpace(ptDest, false) )
		{
			SysMessageDefault(DEFMSG_MSG_MOUNT_CEILING);
			return false;
		}
	}

	if ( fDisplayEffect && !IsStatFlag(STATF_Insubstantial) && (iEffect == ITEMID_NOTHING) && (iSound == SOUND_NONE) )
	{
		if ( m_pPlayer )
		{
			if ( IsPriv(PRIV_GM) )
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
	if ( fTakePets )	// look for any creatures that might be following me nearby
	{
		if ( ptOld.IsValidPoint() )
		{
			CWorldSearch Area(ptOld, UO_MAP_VIEW_SIGHT);
			for (;;)
			{
				CChar *pChar = Area.GetChar();
				if ( !pChar )
					break;
				if ( pChar == this )
					continue;

				if ( (pChar->Skill_GetActive() == NPCACT_FOLLOW_TARG) && (pChar->m_Act_Targ == GetUID()) )
				{
					if ( pChar->CanMoveWalkTo(ptOld, false, true) )
						pChar->Spell_Teleport(ptDest, fTakePets, fCheckAntiMagic, fDisplayEffect, iEffect, iSound);
				}
			}
		}
	}

	MoveTo(ptDest);		// move character

	CClient *pClientExclude = NULL;
	if ( m_pClient )
	{
		if ( ptDest.m_map == ptOld.m_map )
			m_pClient->addPlayerView(NULL);
		else
		{
			m_pClient->addReSync();
			pClientExclude = m_pClient;	// don't update this client again
		}
	}

	UpdateMove(ptOld, pClientExclude);
	Reveal();

	if ( fDisplayEffect )
	{
		if ( iEffect != ITEMID_NOTHING )
		{
			Effect(EFFECT_XYZ, iEffect, ptOld, ptOld, 10, 10);
			Effect(EFFECT_XYZ, iEffect, ptDest, ptDest, 10, 10);
		}
		if ( iSound != SOUND_NONE )
			Sound(iSound);
	}

	return true;
}

bool CChar::Spell_CreateGate(CPointMap ptDest, bool fCheckAntiMagic)
{
	ADDTOCALLSTACK("CChar::Spell_CreateGate");
	// Create moongate between current pt and destination pt
	// RETURN: true = it worked.

	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(SPELL_Gate_Travel);
	if ( !pSpellDef || !m_pArea || !ptDest.IsValidPoint() )
		return false;

	CRegionBase *pAreaDest = ptDest.GetRegion(REGION_TYPE_AREA|REGION_TYPE_ROOM|REGION_TYPE_MULTI);
	if ( !pAreaDest )
		return false;

	if ( !IsPriv(PRIV_GM) )
	{
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

		if ( pAreaDest->IsFlag(REGION_FLAG_SHIP) )
		{
			SysMessageDefault(DEFMSG_SPELL_GATE_SOMETHINGBLOCKING);
			return false;
		}

		if ( fCheckAntiMagic )
		{
			if ( pAreaDest->IsFlag(REGION_ANTIMAGIC_ALL|REGION_ANTIMAGIC_GATE) )
			{
				SysMessageDefault(DEFMSG_SPELL_GATE_AM);
				return false;
			}
		}

		if ( g_World.IsItemTypeNear(GetTopPoint(), IT_TELEPAD, 0, false, true) || g_World.IsItemTypeNear(ptDest, IT_TELEPAD, 0, false, true) )
		{
			SysMessageDefault(DEFMSG_SPELL_GATE_ALREADYTHERE);
			return false;
		}
	}

	ptDest.m_z = GetFixZ(ptDest);
	ITEMID_TYPE id = pSpellDef->m_idEffect;
	int iDuration = pSpellDef->m_Duration.GetLinear(0);

	CItem *pGateOrig = CItem::CreateBase((id != ITEMID_NOTHING) ? id : (pAreaDest->IsFlag(REGION_FLAG_SAFE|REGION_FLAG_GUARDED|REGION_FLAG_NO_PVP) ? ITEMID_MOONGATE_BLUE : ITEMID_MOONGATE_RED));
	ASSERT(pGateOrig);
	pGateOrig->SetType(IT_TELEPAD);
	pGateOrig->SetAttr(ATTR_MOVE_NEVER);
	pGateOrig->m_itNormal.m_more1 = static_cast<DWORD>(GetUID());
	pGateOrig->m_itTelepad.m_ptMark = ptDest;
	pGateOrig->MoveToDecay(GetTopPoint(), iDuration);
	pGateOrig->Sound(pSpellDef->m_sound);

	CItem *pGateDest = CItem::CreateBase((id != ITEMID_NOTHING) ? id : (m_pArea->IsFlag(REGION_FLAG_SAFE|REGION_FLAG_GUARDED|REGION_FLAG_NO_PVP) ? ITEMID_MOONGATE_BLUE : ITEMID_MOONGATE_RED));
	ASSERT(pGateDest);
	pGateDest->SetType(IT_TELEPAD);
	pGateDest->SetAttr(ATTR_MOVE_NEVER);
	pGateDest->m_itNormal.m_more1 = static_cast<DWORD>(GetUID());
	pGateDest->m_itTelepad.m_ptMark = GetTopPoint();
	pGateDest->MoveToDecay(ptDest, iDuration);
	pGateDest->Sound(pSpellDef->m_sound);

	pGateOrig->m_uidLink = pGateDest->GetUID();
	pGateDest->m_uidLink = pGateOrig->GetUID();

	SysMessageDefault(DEFMSG_SPELL_GATE_OPEN);
	return true;
}

CChar *CChar::Spell_Summon(CREID_TYPE id, CPointMap ptTarg)
{
	ADDTOCALLSTACK("CChar::Spell_Summon");
	// Summon an NPC using summon spells.

	int iSkill;
	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_Spell);
	if ( !pSpellDef || !pSpellDef->GetPrimarySkill(&iSkill, NULL) )
		return NULL;

	if ( !pSpellDef->IsSpellType(SPELLFLAG_TARG_OBJ|SPELLFLAG_TARG_XYZ) )
		ptTarg = GetTopPoint();

	CChar *pChar = CChar::CreateBasic(id);
	if ( !pChar )
		return NULL;

	if ( !IsPriv(PRIV_GM) )
	{
		if ( IsSetMagicFlags(MAGICF_SUMMONWALKCHECK) )	// check if the target location is valid
		{
			CCharBase *pSummonDef = CCharBase::FindCharBase(id);
			DWORD dwCan = ULONG_MAX;
			if ( pSummonDef )
				dwCan = pSummonDef->m_Can & CAN_C_MOVEMASK;

			if ( dwCan != ULONG_MAX )
			{
				DWORD dwBlockFlags = 0;
				g_World.GetHeightPoint2(ptTarg, dwBlockFlags, true);

				if ( dwBlockFlags & ~dwCan )
				{
					SysMessageDefault(DEFMSG_MSG_SUMMON_INVALIDTARG);
					pChar->Delete();
					return NULL;
				}
			}
		}

		if ( IsSetOF(OF_PetSlots) )
		{
			if ( !FollowersUpdate(pChar, pChar->m_FollowerSlots, true) )
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
	pChar->MoveToChar(ptTarg);
	pChar->m_ptHome = ptTarg;
	pChar->m_pNPC->m_Home_Dist_Wander = 10;
	pChar->NPC_CreateTrigger();		// removed from NPC_LoadScript() and triggered after char placement
	pChar->NPC_PetSetOwner(this, false);
	pChar->OnSpellEffect(SPELL_Summon, this, Skill_GetAdjusted(static_cast<SKILL_TYPE>(iSkill)), NULL);

	pChar->Update();
	pChar->UpdateAnimate(ANIM_CAST_DIR);
	pChar->SoundChar(CRESND_GETHIT);
	m_Act_Targ = pChar->GetUID();	// for last target stuff
	return pChar;
}

bool CChar::Spell_Recall(CItem *pTarg, bool fGate)
{
	ADDTOCALLSTACK("CChar::Spell_Recall");
	if ( !pTarg )
		return false;

	if ( IsTrigUsed(TRIGGER_SPELLEFFECT) || IsTrigUsed(TRIGGER_ITEMSPELL) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = fGate ? SPELL_Gate_Travel : SPELL_Recall;

		if ( pTarg->OnTrigger(ITRIG_SPELLEFFECT, this, &Args) == TRIGRET_RET_FALSE )
			return true;
	}

	if ( pTarg->IsType(IT_RUNE) )
	{
		if ( !pTarg->m_itRune.m_ptMark.IsValidPoint() )
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

		if ( fGate )
		{
			if ( !Spell_CreateGate(pTarg->m_itRune.m_ptMark) )
				return false;
		}
		else
		{
			const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(SPELL_Recall);
			if ( !pSpellDef || !Spell_Teleport(pTarg->m_itRune.m_ptMark, true, true, true, pSpellDef->m_idEffect, pSpellDef->m_sound) )
				return false;
		}

		if ( !IsPriv(PRIV_GM) )
		{
			--pTarg->m_itRune.m_Charges;
			pTarg->UpdatePropertyFlag();
		}
		return true;
	}
	else if ( pTarg->IsType(IT_KEY) )
	{
		CItemShip *pBoat = dynamic_cast<CItemShip *>(pTarg->m_uidLink.ItemFind());
		if ( pBoat )
		{
			if ( fGate )
			{
				SysMessageDefault(DEFMSG_SPELL_GATE_SOMETHINGBLOCKING);
				return false;
			}
			else
			{
				CItemContainer *pHatch = pBoat->GetShipHold();
				if ( !pHatch )
					return false;

				CPointMap pt = pHatch->GetTopPoint();
				pt.m_z += pHatch->GetHeight();
				if ( !Spell_Teleport(pt, true) )
					return false;
			}
			return true;
		}
	}

	SysMessageDefault(DEFMSG_SPELL_RECALL_CANTRECALLOBJ);
	return false;
}

bool CChar::Spell_Resurrection(CItemCorpse *pCorpse, CChar *pCharSrc, bool fNoFail)
{
	ADDTOCALLSTACK("CChar::Spell_Resurrection");
	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(SPELL_Resurrection);
	if ( !pSpellDef || !IsStatFlag(STATF_DEAD) )
		return false;

	if ( IsPriv(PRIV_GM) || (pCharSrc && pCharSrc->IsPriv(PRIV_GM)) )
		fNoFail = true;

	if ( !fNoFail && m_pArea && m_pArea->IsFlag(REGION_ANTIMAGIC_ALL) )
	{
		SysMessageDefault(DEFMSG_SPELL_RES_AM);
		return false;
	}

	int iHits = IMULDIV(Stat_GetMax(STAT_STR), g_Cfg.m_iHitpointPercentOnRez, 100);
	if ( !pCorpse )
		pCorpse = FindMyCorpse();

	if ( IsTrigUsed(TRIGGER_RESURRECT) )
	{
		CScriptTriggerArgs Args(iHits, 0, pCorpse);
		if ( OnTrigger(CTRIG_Resurrect, pCharSrc, &Args) == TRIGRET_RET_TRUE )
			return false;
		iHits = static_cast<int>(Args.m_iN1);
	}

	SetID(m_prev_id);
	SetHue(m_prev_Hue);
	StatFlag_Clear(STATF_DEAD|STATF_Insubstantial);
	Stat_SetVal(STAT_STR, maximum(iHits, 1));

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

	bool fRaisedCorpse = false;
	if ( pCorpse )
	{
		if ( RaiseCorpse(pCorpse) )
		{
			SysMessageDefault(DEFMSG_SPELL_RES_REJOIN);
			fRaisedCorpse = true;
		}
	}

	if ( m_pPlayer )
	{
		CItem *pDeathShroud = ContentFind(RESOURCE_ID(RES_ITEMDEF, ITEMID_DEATHSHROUD));
		if ( pDeathShroud )
			pDeathShroud->Delete();

		if ( !fRaisedCorpse && !g_Cfg.m_fNoResRobe )
		{
			CItem *pRobe = CItem::CreateBase(ITEMID_ROBE);
			ASSERT(pRobe);
			pRobe->SetName(g_Cfg.GetDefaultMsg(DEFMSG_SPELL_RES_ROBENAME));
			LayerAdd(pRobe, LAYER_ROBE);
		}

	}

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

	WORD wStatEffect = pSpell->m_itSpell.m_spelllevel;

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
					m_DamIncrease -= pSpell->m_itSpell.m_PolyDex;
					m_attackBase -= static_cast<WORD>(pSpell->m_itSpell.m_spellcharges);
					m_attackRange -= wStatEffect;
					break;
				case SPELL_Lich_Form:
					iBuffIcon = BI_LICHFORM;
					SetDefNum("RegenHits", GetDefNum("RegenHits") + pSpell->m_itSpell.m_PolyStr);
					SetDefNum("RegenMana", GetDefNum("RegenMana") - pSpell->m_itSpell.m_PolyDex);
					m_ResFire += pSpell->m_itSpell.m_spellcharges;
					m_ResPoison -= pSpell->m_itSpell.m_spellcharges;
					m_ResCold -= pSpell->m_itSpell.m_spellcharges;
					break;
				case SPELL_Vampiric_Embrace:
					iBuffIcon = BI_VAMPIRICEMBRACE;
					m_HitLifeLeech -= pSpell->m_itSpell.m_PolyStr;
					SetDefNum("RegenStam", GetDefNum("RegenStam") - pSpell->m_itSpell.m_PolyDex);
					SetDefNum("RegenMana", GetDefNum("RegenMana") - pSpell->m_itSpell.m_spellcharges);
					m_ResFire += wStatEffect;
					break;
				case SPELL_Wraith_Form:
					iBuffIcon = BI_WRAITHFORM;
					m_ResPhysical -= pSpell->m_itSpell.m_PolyStr;
					m_ResEnergy += pSpell->m_itSpell.m_PolyDex;
					m_ResFire += pSpell->m_itSpell.m_PolyDex;
					m_HitManaLeech -= pSpell->m_itSpell.m_spellcharges;
					break;
				case SPELL_Reaper_Form:
					iBuffIcon = BI_REAPERFORM;
					m_SwingSpeedIncrease -= pSpell->m_itSpell.m_PolyStr;
					m_SpellDamIncrease -= pSpell->m_itSpell.m_PolyStr;
					m_ResPhysical -= pSpell->m_itSpell.m_PolyDex;
					m_ResCold -= pSpell->m_itSpell.m_PolyDex;
					m_ResPoison -= pSpell->m_itSpell.m_PolyDex;
					m_ResEnergy -= pSpell->m_itSpell.m_PolyDex;
					m_ResFire += pSpell->m_itSpell.m_spellcharges;
					break;
				case SPELL_Stone_Form:
					iBuffIcon = BI_STONEFORM;
					m_SwingSpeedIncrease += pSpell->m_itSpell.m_PolyStr;
					m_FasterCasting += pSpell->m_itSpell.m_PolyDex;
					m_ResPhysical -= pSpell->m_itSpell.m_spellcharges;
					m_ResPhysicalMax -= wStatEffect;
					m_ResFire -= pSpell->m_itSpell.m_spellcharges;
					m_ResFireMax -= wStatEffect;
					m_ResCold -= pSpell->m_itSpell.m_spellcharges;
					m_ResColdMax -= wStatEffect;
					m_ResPoison -= pSpell->m_itSpell.m_spellcharges;
					m_ResPoisonMax -= wStatEffect;
					m_ResEnergy -= pSpell->m_itSpell.m_spellcharges;
					m_ResEnergyMax -= wStatEffect;
					m_DamIncrease -= pSpell->m_itSpell.m_PolyStr;
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
			if ( m_pClient )
			{
				m_pClient->addCharMove(this);	// immediately tell the client that now he's unparalyzed (without this, it will be unparalyzed only on next tick update)
				m_pClient->removeBuff(BI_PARALYZE);
			}
			return;
		}
		case LAYER_SPELL_Strangle:
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
			m_ResPhysical -= pSpell->m_itSpell.m_PolyStr;
			m_ResFire += pSpell->m_itSpell.m_PolyDex;
			m_ResCold -= pSpell->m_itSpell.m_PolyStr;
			m_ResPoison += pSpell->m_itSpell.m_PolyDex;
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
			Stat_AddMod(STAT_DEX, wStatEffect);
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
			Stat_AddMod(STAT_INT, wStatEffect);
			if ( m_pClient )
				m_pClient->removeBuff(BI_FEEBLEMIND);
			return;
		}
		case SPELL_Weaken:
		{
			Stat_AddMod(STAT_STR, wStatEffect);
			if ( m_pClient )
				m_pClient->removeBuff(BI_WEAKEN);
			return;
		}
		case SPELL_Curse:
		case SPELL_Mass_Curse:
		{
			if ( (spell == SPELL_Curse) && IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) && m_pPlayer )
			{
				m_ResFireMax += 10;
				m_ResColdMax += 10;
				m_ResPoisonMax += 10;
				m_ResEnergyMax += 10;
			}
			for ( int i = STAT_STR; i < STAT_BASE_QTY; ++i )
				Stat_AddMod(static_cast<STAT_TYPE>(i), wStatEffect);
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
			Stat_AddMod(STAT_DEX, -wStatEffect);
			if ( m_pClient )
				m_pClient->removeBuff(BI_AGILITY);
			return;
		}
		case SPELL_Cunning:
		{
			Stat_AddMod(STAT_INT, -wStatEffect);
			if ( m_pClient )
				m_pClient->removeBuff(BI_CUNNING);
			return;
		}
		case SPELL_Strength:
		{
			Stat_AddMod(STAT_STR, -wStatEffect);
			if ( m_pClient )
				m_pClient->removeBuff(BI_STRENGTH);
			return;
		}
		case SPELL_Bless:
		{
			for ( int i = STAT_STR; i < STAT_BASE_QTY; ++i )
				Stat_AddMod(static_cast<STAT_TYPE>(i), -wStatEffect);
			if ( m_pClient )
				m_pClient->removeBuff(BI_BLESS);
			return;
		}
		case SPELL_Mana_Drain:
		{
			UpdateStatVal(STAT_INT, +wStatEffect);
			return;
		}
		case SPELL_Reactive_Armor:
		{
			if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
			{
				m_ResPhysical -= wStatEffect;
				m_ResFire += 5;
				m_ResCold += 5;
				m_ResPoison += 5;
				m_ResEnergy += 5;
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
				m_ResPhysical += wStatEffect;
				m_ResFire -= 10;
				m_ResCold -= 10;
				m_ResPoison -= 10;
				m_ResEnergy -= 10;
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
			if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
			{
				m_ResPhysical += pSpell->m_itSpell.m_PolyStr;
				m_FasterCasting += 2;
				Skill_SetBase(SKILL_MAGICRESISTANCE, minimum(Skill_GetMax(SKILL_MAGICRESISTANCE, true), Skill_GetBase(SKILL_MAGICRESISTANCE) + static_cast<WORD>(pSpell->m_itSpell.m_PolyDex)));
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
			Skill_SetBase(SKILL_MEDITATION, Skill_GetBase(SKILL_MEDITATION) - static_cast<WORD>(g_Cfg.GetSpellEffect(spell, wStatEffect)));
			return;
		}
		//case SPELL_Shield:		// 113 // erects a temporary force field around you. Nobody approaching will be able to get within 1 tile of you, though you can move close to them if you wish.
		//	return;
		case SPELL_Mind_Rot:
		{
			m_LowerManaCost += wStatEffect;
			if ( m_pClient )
				m_pClient->removeBuff(BI_MINDROT);
			return;
		}
		case SPELL_Curse_Weapon:
		{
			CItem *pWeapon = m_uidWeapon.ItemFind();
			if ( pWeapon )
				pWeapon->m_HitLifeLeech -= wStatEffect;		// add 50% hit life leech to the weapon, since damaging with it should return 50% of the damage dealt
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
	WORD wStatEffect = pSpell->m_itSpell.m_spelllevel;
	WORD wTimerEffect = static_cast<WORD>(maximum(pSpell->GetTimerAdjusted(), 0));

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
	TCHAR szNumBuff[7][MAX_NAME_SIZE];
	LPCTSTR pszNumBuff[7] = { szNumBuff[0], szNumBuff[1], szNumBuff[2], szNumBuff[3], szNumBuff[4], szNumBuff[5], szNumBuff[6] };

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
						LPCTSTR pszName = "creature";
						CCharBase *pPolyCharDef = CCharBase::FindCharBase(m_atMagery.m_SummonID);
						if ( pPolyCharDef && (pPolyCharDef->GetName()[0] != '#') )
						{
							pszName = pPolyCharDef->GetName();
							_strlwr(const_cast<TCHAR *>(pszName));
						}

						strcpy(szNumBuff[0], Str_GetArticleAndSpace(pszName));
						strncpy(szNumBuff[1], pszName, sizeof(szNumBuff[1]) - 1);
						szNumBuff[0][strlen(szNumBuff[0]) - 1] = '\0';		// trim whitespace from "a " / "an " strings
						m_pClient->removeBuff(BI_POLYMORPH);
						m_pClient->addBuff(BI_POLYMORPH, 1075824, 1075823, wTimerEffect, pszNumBuff, 2);
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
					m_DamIncrease += pSpell->m_itSpell.m_PolyDex;
					m_attackBase += static_cast<WORD>(pSpell->m_itSpell.m_spellcharges);
					m_attackRange += pSpell->m_itSpell.m_spelllevel;

					if ( m_pClient && IsSetOF(OF_Buffs) )
					{
						ITOA(pSpell->m_itSpell.m_PolyStr, szNumBuff[0], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, szNumBuff[1], 10);
						m_pClient->removeBuff(BI_HORRIFICBEAST);
						m_pClient->addBuff(BI_HORRIFICBEAST, 1060514, 1153763, wTimerEffect, pszNumBuff, 2);
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
					m_ResFire -= pSpell->m_itSpell.m_spellcharges;
					m_ResPoison += pSpell->m_itSpell.m_spellcharges;
					m_ResCold += pSpell->m_itSpell.m_spellcharges;

					if ( m_pClient && IsSetOF(OF_Buffs) )
					{
						ITOA(pSpell->m_itSpell.m_PolyStr, szNumBuff[0], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, szNumBuff[1], 10);
						ITOA(pSpell->m_itSpell.m_spellcharges, szNumBuff[2], 10);
						ITOA(pSpell->m_itSpell.m_spellcharges, szNumBuff[3], 10);
						ITOA(pSpell->m_itSpell.m_spellcharges, szNumBuff[4], 10);
						m_pClient->removeBuff(BI_LICHFORM);
						m_pClient->addBuff(BI_LICHFORM, 1060515, 1153767, wTimerEffect, pszNumBuff, 5);
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
					m_HitLifeLeech += pSpell->m_itSpell.m_PolyStr;
					SetDefNum("RegenStam", GetDefNum("RegenStam") + pSpell->m_itSpell.m_PolyDex);
					SetDefNum("RegenMana", GetDefNum("RegenMana") + pSpell->m_itSpell.m_spellcharges);
					m_ResFire -= pSpell->m_itSpell.m_spelllevel;

					if ( m_pClient && IsSetOF(OF_Buffs) )
					{
						ITOA(pSpell->m_itSpell.m_PolyStr, szNumBuff[0], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, szNumBuff[1], 10);
						ITOA(pSpell->m_itSpell.m_spellcharges, szNumBuff[2], 10);
						ITOA(pSpell->m_itSpell.m_spelllevel, szNumBuff[3], 10);
						m_pClient->removeBuff(BI_VAMPIRICEMBRACE);
						m_pClient->addBuff(BI_VAMPIRICEMBRACE, 1060521, 1153768, wTimerEffect, pszNumBuff, 4);
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
					m_ResPhysical += pSpell->m_itSpell.m_PolyStr;
					m_ResEnergy -= pSpell->m_itSpell.m_PolyDex;
					m_ResFire -= pSpell->m_itSpell.m_PolyDex;
					m_HitManaLeech += pSpell->m_itSpell.m_spellcharges;

					if ( m_pClient && IsSetOF(OF_Buffs) )
					{
						ITOA(pSpell->m_itSpell.m_PolyStr, szNumBuff[0], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, szNumBuff[1], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, szNumBuff[2], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, szNumBuff[3], 10);
						m_pClient->removeBuff(BI_WRAITHFORM);
						m_pClient->addBuff(BI_WRAITHFORM, 1060524, 1153829, wTimerEffect, pszNumBuff, 4);
					}
					break;
				}
				case SPELL_Reaper_Form:
				{
					m_atMagery.m_SummonID = CREID_REAPER_FORM;
					pSpell->m_itSpell.m_PolyStr = 10;		// Swing Speed Increase, Spell Damage Increase
					pSpell->m_itSpell.m_PolyDex = 5;		// Physical/Cold/Poison/Energy Resist
					pSpell->m_itSpell.m_spellcharges = 25;	// Fire Resist
					m_SwingSpeedIncrease += pSpell->m_itSpell.m_PolyStr;
					m_SpellDamIncrease += pSpell->m_itSpell.m_PolyStr;
					m_ResPhysical += pSpell->m_itSpell.m_PolyDex;
					m_ResCold += pSpell->m_itSpell.m_PolyDex;
					m_ResPoison += pSpell->m_itSpell.m_PolyDex;
					m_ResEnergy += pSpell->m_itSpell.m_PolyDex;
					m_ResFire -= pSpell->m_itSpell.m_spellcharges;

					if ( m_pClient && IsSetOF(OF_Buffs) )
					{
						ITOA(pSpell->m_itSpell.m_PolyStr, szNumBuff[0], 10);
						ITOA(pSpell->m_itSpell.m_PolyStr, szNumBuff[1], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, szNumBuff[2], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, szNumBuff[3], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, szNumBuff[4], 10);
						ITOA(pSpell->m_itSpell.m_PolyDex, szNumBuff[5], 10);
						ITOA(pSpell->m_itSpell.m_spellcharges, szNumBuff[6], 10);
						m_pClient->removeBuff(BI_REAPERFORM);
						m_pClient->addBuff(BI_REAPERFORM, 1071034, 1153781, wTimerEffect, pszNumBuff, 7);
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
					m_SwingSpeedIncrease -= pSpell->m_itSpell.m_PolyStr;
					m_FasterCasting -= pSpell->m_itSpell.m_PolyDex;
					m_ResPhysical += pSpell->m_itSpell.m_spellcharges;
					m_ResPhysicalMax += pSpell->m_itSpell.m_spelllevel;
					m_ResFire += pSpell->m_itSpell.m_spellcharges;
					m_ResFireMax += pSpell->m_itSpell.m_spelllevel;
					m_ResCold += pSpell->m_itSpell.m_spellcharges;
					m_ResColdMax += pSpell->m_itSpell.m_spelllevel;
					m_ResPoison += pSpell->m_itSpell.m_spellcharges;
					m_ResPoisonMax += pSpell->m_itSpell.m_spelllevel;
					m_ResEnergy += pSpell->m_itSpell.m_spellcharges;
					m_ResEnergyMax += pSpell->m_itSpell.m_spelllevel;
					m_DamIncrease += pSpell->m_itSpell.m_PolyStr;

					if ( m_pClient && IsSetOF(OF_Buffs) )
					{
						ITOA(-pSpell->m_itSpell.m_PolyStr, szNumBuff[0], 10);
						ITOA(-pSpell->m_itSpell.m_PolyDex, szNumBuff[1], 10);
						ITOA(pSpell->m_itSpell.m_spellcharges, szNumBuff[2], 10);
						ITOA(pSpell->m_itSpell.m_spellcharges, szNumBuff[3], 10);
						ITOA(pSpell->m_itSpell.m_PolyStr, szNumBuff[4], 10);
						m_pClient->removeBuff(BI_STONEFORM);
						m_pClient->addBuff(BI_STONEFORM, 1080145, 1080146, wTimerEffect, pszNumBuff, 5);
					}
					break;
				}
				default:
					break;
			}

			// set to creature type stats
			if ( (spell == SPELL_Polymorph) && IsSetMagicFlags(MAGICF_POLYMORPHSTATS) )
			{
				if ( pCharDef->m_Str )
				{
					int iChange = pCharDef->m_Str - Stat_GetBase(STAT_STR);
					if ( iChange > g_Cfg.m_iMaxPolyStats )
						iChange = g_Cfg.m_iMaxPolyStats;
					else if ( iChange < -g_Cfg.m_iMaxPolyStats )
						iChange = -g_Cfg.m_iMaxPolyStats;
					if ( iChange + Stat_GetBase(STAT_STR) < 0 )
						iChange = -Stat_GetBase(STAT_STR);
					Stat_AddMod(STAT_STR, iChange);
					pSpell->m_itSpell.m_PolyStr = iChange;
				}
				if ( pCharDef->m_Dex )
				{
					int iChange = pCharDef->m_Dex - Stat_GetBase(STAT_DEX);
					if ( iChange > g_Cfg.m_iMaxPolyStats )
						iChange = g_Cfg.m_iMaxPolyStats;
					else if ( iChange < -g_Cfg.m_iMaxPolyStats )
						iChange = -g_Cfg.m_iMaxPolyStats;
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
					m_pClient->addBuff(BI_NIGHTSIGHT, 1075643, 1075644, wTimerEffect);
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
				pSpell->GetTagDefs()->SetNum("COLOR.HAIR", pHair->GetHue());
				pHair->SetHue(RandomHairHue);
			}

			CItem *pBeard = LayerFind(LAYER_BEARD);
			if ( pBeard )
			{
				pSpell->GetTagDefs()->SetNum("COLOR.BEARD", pBeard->GetHue());
				pBeard->SetHue(RandomHairHue);
			}

			NotoSave_Update();
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				m_pClient->removeBuff(BI_INCOGNITO);
				m_pClient->addBuff(BI_INCOGNITO, 1075819, 1075820, wTimerEffect);
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
				m_pClient->addBuff(BI_INVISIBILITY, 1075825, 1075826, wTimerEffect);
			}
			return;
		}
		case LAYER_SPELL_Paralyze:
		{
			StatFlag_Set(STATF_Freeze);
			if ( m_pClient )
			{
				m_pClient->addCharMove(this);	// immediately tell the client that now he's paralyzed (without this, it will be paralyzed only on next tick update)
				if ( IsSetOF(OF_Buffs) )
				{
					m_pClient->removeBuff(BI_PARALYZE);
					m_pClient->addBuff(BI_PARALYZE, 1075827, 1075828, wTimerEffect);
				}
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
			wStatEffect = pCaster->Skill_GetBase(SKILL_SPIRITSPEAK) / 100;
			if ( wStatEffect < 4 )
				wStatEffect = 4;
			pSpell->m_itSpell.m_spelllevel = wStatEffect;
			pSpell->m_itSpell.m_spellcharges = wStatEffect;

			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				double dStamPenalty = 3 - (static_cast<double>(Stat_GetVal(STAT_DEX) / maximum(1, Stat_GetAdjusted(STAT_DEX))) * 2);
				WORD wTimerTotal = 0;
				for ( WORD w = 0; w < wStatEffect; ++w )
					wTimerTotal += (wStatEffect - w) * TICK_PER_SEC;

				ITOA(static_cast<int>((wStatEffect - 2) * dStamPenalty), szNumBuff[0], 10);
				ITOA(static_cast<int>((wStatEffect + 1) * dStamPenalty), szNumBuff[1], 10);
				m_pClient->removeBuff(BI_STRANGLE);
				m_pClient->addBuff(BI_STRANGLE, 1075794, 1075795, wTimerTotal, pszNumBuff, 2);
			}
			return;
		}
		case LAYER_SPELL_Gift_Of_Renewal:
		{
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(pSpell->m_itSpell.m_spelllevel, szNumBuff[0], 10);
				m_pClient->removeBuff(BI_GIFTOFRENEWAL);
				m_pClient->addBuff(BI_GIFTOFRENEWAL, 1075796, 1075797, wTimerEffect, pszNumBuff, 1);
			}
			return;
		}
		case LAYER_SPELL_Attunement:
		{
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(pSpell->m_itSpell.m_spelllevel, szNumBuff[0], 10);
				m_pClient->removeBuff(BI_ATTUNEWEAPON);
				m_pClient->addBuff(BI_ATTUNEWEAPON, 1075798, 1075799, wTimerEffect, pszNumBuff, 1);
			}
			return;
		}
		case LAYER_SPELL_Thunderstorm:
		{
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(pSpell->m_itSpell.m_spelllevel, szNumBuff[0], 10);
				m_pClient->removeBuff(BI_THUNDERSTORM);
				m_pClient->addBuff(BI_THUNDERSTORM, 1075800, 1075801, wTimerEffect, pszNumBuff, 1);
			}
			return;
		}
		case LAYER_SPELL_Essence_Of_Wind:
		{
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(pSpell->m_itSpell.m_spelllevel, szNumBuff[0], 10);
				m_pClient->removeBuff(BI_ESSENCEOFWIND);
				m_pClient->addBuff(BI_ESSENCEOFWIND, 1075802, 1075803, wTimerEffect, pszNumBuff, 1);
			}
			return;
		}
		case LAYER_SPELL_Ethereal_Voyage:
		{
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				m_pClient->removeBuff(BI_ETHEREALVOYAGE);
				m_pClient->addBuff(BI_ETHEREALVOYAGE, 1075804, 1075805, wTimerEffect);
			}
			return;
		}
		case LAYER_SPELL_Gift_Of_Life:
		{
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				m_pClient->removeBuff(BI_GIFTOFLIFE);
				m_pClient->addBuff(BI_GIFTOFLIFE, 1075806, 1075807, wTimerEffect);
			}
			return;
		}
		case LAYER_SPELL_Arcane_Empowerment:
		{
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(pSpell->m_itSpell.m_spelllevel, szNumBuff[0], 10);
				ITOA(pSpell->m_itSpell.m_spellcharges, szNumBuff[1], 10);
				m_pClient->removeBuff(BI_ARCANEEMPOWERMENT);
				m_pClient->addBuff(BI_ARCANEEMPOWERMENT, 1075805, 1075804, wTimerEffect, pszNumBuff, 1);
			}
			return;
		}
		/*case LAYER_Mortal_Strike:
		{
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				m_pClient->removeBuff(BI_MORTALSTRIKE);
				m_pClient->addBuff(BI_MORTALSTRIKE, 1075810, 1075811, wTimerEffect);
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
				wStatEffect = ((pCaster->Skill_GetBase(SKILL_SPIRITSPEAK) - Skill_GetBase(SKILL_MAGICRESISTANCE)) / 10) + 30;
			else
				wStatEffect = ((pCaster->Skill_GetBase(SKILL_SPIRITSPEAK) - Skill_GetBase(SKILL_MAGICRESISTANCE)) / 100) + 18;
			pSpell->m_itSpell.m_spellcharges = 10;
			pSpell->m_itSpell.m_spelllevel = wStatEffect;
			return;
		}
		case LAYER_SPELL_Blood_Oath:
		{
			wStatEffect = ((Skill_GetBase(SKILL_MAGICRESISTANCE) * 10) / 20) + 10;	// bonus of reflection
			pSpell->m_itSpell.m_spelllevel = wStatEffect;
			if ( IsSetOF(OF_Buffs) )
			{
				if ( m_pClient )
				{
					strncpy(szNumBuff[0], pCaster->GetName(), sizeof(szNumBuff[0]) - 1);
					strncpy(szNumBuff[1], pCaster->GetName(), sizeof(szNumBuff[1]) - 1);
					m_pClient->removeBuff(BI_BLOODOATHCURSE);
					m_pClient->addBuff(BI_BLOODOATHCURSE, 1075659, 1075660, wTimerEffect, pszNumBuff, 2);
				}
				CClient *pClientCaster = pCaster->m_pClient;
				if ( pClientCaster )
				{
					strncpy(szNumBuff[0], GetName(), sizeof(szNumBuff[0]) - 1);
					pClientCaster->removeBuff(BI_BLOODOATHCASTER);
					pClientCaster->addBuff(BI_BLOODOATHCASTER, 1075661, 1075662, wTimerEffect, pszNumBuff, 1);
				}
			}
			return;
		}
		case LAYER_SPELL_Corpse_Skin:
		{
			pSpell->m_itSpell.m_PolyStr = 10;
			pSpell->m_itSpell.m_PolyDex = 15;
			m_ResPhysical += pSpell->m_itSpell.m_PolyStr;
			m_ResFire -= pSpell->m_itSpell.m_PolyDex;
			m_ResCold += pSpell->m_itSpell.m_PolyStr;
			m_ResPoison -= pSpell->m_itSpell.m_PolyDex;

			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				m_pClient->removeBuff(BI_CORPSESKIN);
				m_pClient->addBuff(BI_CORPSESKIN, 1075663, 1075664, wTimerEffect);
			}
			return;
		}
		case LAYER_SPELL_Mind_Rot:
		{
			wStatEffect = 10;	// Lower Mana Cost
			pSpell->m_itSpell.m_spelllevel = wStatEffect;
			m_LowerManaCost -= wStatEffect;
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
			wStatEffect = 50;	// Hit Life Leech
			pSpell->m_itSpell.m_spelllevel = wStatEffect;
			pWeapon->m_HitLifeLeech += wStatEffect;		// add 50% hit life leech to the weapon, since damaging with it should return 50% of the damage dealt
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
				wStatEffect = 15 + (pCaster->Skill_GetBase(SKILL_INSCRIPTION) / 200);
				pSpell->m_itSpell.m_spelllevel = wStatEffect;

				m_ResPhysical += wStatEffect;
				m_ResFire -= 5;
				m_ResCold -= 5;
				m_ResPoison -= 5;
				m_ResEnergy -= 5;
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
					ITOA(wStatEffect, szNumBuff[0], 10);
					for ( int idx = 1; idx < 5; ++idx )
						ITOA(5, szNumBuff[idx], 10);

					m_pClient->addBuff(BI_REACTIVEARMOR, 1075812, 1075813, wTimerEffect, pszNumBuff, 5);
				}
				else
				{
					m_pClient->addBuff(BI_REACTIVEARMOR, 1075812, 1070722, wTimerEffect);
				}
			}
			return;
		}
		case SPELL_Clumsy:
		{
			if ( pCaster && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
			{
				wStatEffect = 8 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100) - (Skill_GetBase(SKILL_MAGICRESISTANCE) / 100);
				pSpell->m_itSpell.m_spelllevel = wStatEffect;
			}
			Stat_AddMod(STAT_DEX, -wStatEffect);
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(wStatEffect, szNumBuff[0], 10);
				m_pClient->removeBuff(BI_CLUMSY);
				m_pClient->addBuff(BI_CLUMSY, 1075831, 1075832, wTimerEffect, pszNumBuff, 1);
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
				wStatEffect = 8 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100) - (Skill_GetBase(SKILL_MAGICRESISTANCE) / 100);
				pSpell->m_itSpell.m_spelllevel = wStatEffect;
			}
			Stat_AddMod(STAT_INT, -wStatEffect);
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(wStatEffect, szNumBuff[0], 10);
				m_pClient->removeBuff(BI_FEEBLEMIND);
				m_pClient->addBuff(BI_FEEBLEMIND, 1075833, 1075834, wTimerEffect, pszNumBuff, 1);
			}
			return;
		}
		case SPELL_Weaken:
		{
			if ( pCaster && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
			{
				wStatEffect = 8 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100) - (Skill_GetBase(SKILL_MAGICRESISTANCE) / 100);
				pSpell->m_itSpell.m_spelllevel = wStatEffect;
			}
			Stat_AddMod(STAT_STR, -wStatEffect);
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(wStatEffect, szNumBuff[0], 10);
				m_pClient->removeBuff(BI_WEAKEN);
				m_pClient->addBuff(BI_WEAKEN, 1075837, 1075838, wTimerEffect, pszNumBuff, 1);
			}
			return;
		}
		case SPELL_Curse:
		case SPELL_Mass_Curse:
		{
			if ( pCaster && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
			{
				wStatEffect = 8 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100) - (Skill_GetBase(SKILL_MAGICRESISTANCE) / 100);
				pSpell->m_itSpell.m_spelllevel = wStatEffect;
			}
			if ( (spell == SPELL_Curse) && IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) && m_pPlayer )		// Curse also decrease max resistances on players (not applied to Mass Curse)
			{
				m_ResFireMax -= 10;
				m_ResColdMax -= 10;
				m_ResPoisonMax -= 10;
				m_ResEnergyMax -= 10;
			}
			for ( int i = STAT_STR; i < STAT_BASE_QTY; ++i )
				Stat_AddMod(static_cast<STAT_TYPE>(i), -wStatEffect);

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
					ITOA(wStatEffect, szNumBuff[idx], 10);
				if ( (spell == SPELL_Curse) && IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
				{
					for ( int idx = 3; idx < 7; ++idx )
						ITOA(10, szNumBuff[idx], 10);

					m_pClient->addBuff(BuffIcon, BuffCliloc, 1075836, wTimerEffect, pszNumBuff, 7);
				}
				else
				{
					m_pClient->addBuff(BuffIcon, BuffCliloc, 1075840, wTimerEffect, pszNumBuff, 3);
				}
			}
			return;
		}
		case SPELL_Agility:
		{
			if ( pCaster && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
			{
				wStatEffect = 1 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100);
				pSpell->m_itSpell.m_spelllevel = wStatEffect;
			}
			Stat_AddMod(STAT_DEX, +wStatEffect);
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(wStatEffect, szNumBuff[0], 10);
				m_pClient->removeBuff(BI_AGILITY);
				m_pClient->addBuff(BI_AGILITY, 1075841, 1075842, wTimerEffect, pszNumBuff, 1);
			}
			return;
		}
		case SPELL_Cunning:
		{
			if ( pCaster && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
			{
				wStatEffect = 1 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100);
				pSpell->m_itSpell.m_spelllevel = wStatEffect;
			}
			Stat_AddMod(STAT_INT, +wStatEffect);
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(wStatEffect, szNumBuff[0], 10);
				m_pClient->removeBuff(BI_CUNNING);
				m_pClient->addBuff(BI_CUNNING, 1075843, 1075844, wTimerEffect, pszNumBuff, 1);
			}
			return;
		}
		case SPELL_Strength:
		{
			if ( pCaster && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
			{
				wStatEffect = 1 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100);
				pSpell->m_itSpell.m_spelllevel = wStatEffect;
			}
			Stat_AddMod(STAT_STR, +wStatEffect);
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				ITOA(wStatEffect, szNumBuff[0], 10);
				m_pClient->removeBuff(BI_STRENGTH);
				m_pClient->addBuff(BI_STRENGTH, 1075845, 1075846, wTimerEffect, pszNumBuff, 1);
			}
			return;
		}
		case SPELL_Bless:
		{
			if ( pCaster && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
			{
				wStatEffect = 1 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100);
				pSpell->m_itSpell.m_spelllevel = wStatEffect;
			}
			for ( int i = STAT_STR; i < STAT_BASE_QTY; ++i )
				Stat_AddMod(static_cast<STAT_TYPE>(i), wStatEffect);

			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				for ( int idx = STAT_STR; idx < STAT_BASE_QTY; ++idx )
					ITOA(wStatEffect, szNumBuff[idx], 10);

				m_pClient->removeBuff(BI_BLESS);
				m_pClient->addBuff(BI_BLESS, 1075847, 1075848, wTimerEffect, pszNumBuff, STAT_BASE_QTY);
			}
			return;
		}
		case SPELL_Mana_Drain:
		{
			if ( pCaster )
			{
				int iChange = (400 + pCaster->Skill_GetBase(SKILL_EVALINT) - Skill_GetBase(SKILL_MAGICRESISTANCE)) / 10;
				if ( iChange < 0 )
					iChange = 0;
				else if ( iChange > Stat_GetVal(STAT_INT) )
					iChange = Stat_GetVal(STAT_INT);

				wStatEffect = static_cast<WORD>(iChange);
				pSpell->m_itSpell.m_spelllevel = wStatEffect;
			}
			UpdateStatVal(STAT_INT, -wStatEffect);
			return;
		}
		case SPELL_Magic_Reflect:
		{
			StatFlag_Set(STATF_Reflection);
			if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
			{
				wStatEffect = 25 - (pCaster->Skill_GetBase(SKILL_INSCRIPTION) / 200);
				pSpell->m_itSpell.m_spelllevel = wStatEffect;

				m_ResPhysical -= wStatEffect;
				m_ResFire += 10;
				m_ResCold += 10;
				m_ResPoison += 10;
				m_ResEnergy += 10;
			}
			if ( m_pClient && IsSetOF(OF_Buffs) )
			{
				m_pClient->removeBuff(BI_MAGICREFLECTION);
				if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
				{
					ITOA(-wStatEffect, szNumBuff[0], 10);
					for ( int idx = 1; idx < 5; ++idx )
						ITOA(10, szNumBuff[idx], 10);

					m_pClient->addBuff(BI_MAGICREFLECTION, 1075817, 1075818, wTimerEffect, pszNumBuff, 5);
				}
				else
				{
					m_pClient->addBuff(BI_MAGICREFLECTION, 1075817, 1070722, wTimerEffect);
				}
			}
			return;
		}
		case SPELL_Steelskin:		// 114 // turns your skin into steel, giving a boost to your AR.
		case SPELL_Stoneskin:		// 115 // turns your skin into stone, giving a boost to your AR.
		case SPELL_Protection:
		case SPELL_Arch_Prot:
		{
			WORD wPhysicalResist = 0;
			WORD wMagicResist = 0;
			if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
			{
				wStatEffect = minimum(75, (pCaster->Skill_GetBase(SKILL_EVALINT) + pCaster->Skill_GetBase(SKILL_MEDITATION) + pCaster->Skill_GetBase(SKILL_INSCRIPTION)) / 40);
				wPhysicalResist = 15 - (pCaster->Skill_GetBase(SKILL_INSCRIPTION) / 200);
				wMagicResist = minimum(Skill_GetBase(SKILL_MAGICRESISTANCE), 350 - (Skill_GetBase(SKILL_INSCRIPTION) / 20));

				pSpell->m_itSpell.m_spelllevel = wStatEffect;
				pSpell->m_itSpell.m_PolyStr = static_cast<int>(wPhysicalResist);
				pSpell->m_itSpell.m_PolyDex = static_cast<int>(wMagicResist);

				m_ResPhysical -= wPhysicalResist;
				m_FasterCasting -= 2;
				Skill_SetBase(SKILL_MAGICRESISTANCE, Skill_GetBase(SKILL_MAGICRESISTANCE) - wMagicResist);
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
					ITOA(-wPhysicalResist, szNumBuff[0], 10);
					ITOA(-wMagicResist / 10, szNumBuff[1], 10);
					m_pClient->addBuff(BuffIcon, BuffCliloc, 1075815, wTimerEffect, pszNumBuff, 2);
				}
				else
				{
					m_pClient->addBuff(BuffIcon, BuffCliloc, 1070722, wTimerEffect);
				}
			}
			return;
		}
		case SPELL_Trance:			// 111 // temporarily increases your meditation skill.
		{
			Skill_SetBase(SKILL_MEDITATION, Skill_GetBase(SKILL_MEDITATION) + wStatEffect);
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
				--pItem->m_itSpell.m_spellcharges;

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
			if ( (iCharges <= 0) || (iLevel <= 0) )
				return false;

			// Gain HP.
			UpdateStatVal(STAT_STR, g_Cfg.GetSpellEffect(spell, iLevel));
			pItem->SetTimeout(2 * TICK_PER_SEC);
			break;
		}
		case SPELL_Hallucination:
		{
			if ( (iCharges <= 0) || (iLevel <= 0) )
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

				static LPCTSTR const sm_szPoisonMsg[] =
				{
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_LESSER),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_STANDARD),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_GREATER),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_DEADLY),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_LETHAL)
				};
				static LPCTSTR const sm_szPoisonMsg_Other[] =
				{
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_LESSER1),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_STANDARD1),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_GREATER1),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_DEADLY1),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_OSIPOISON_LETHAL1)
				};
				Emote(NULL, sm_szPoisonMsg_Other[iLevel], m_pClient);
				SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_SPELL_YOUFEEL), sm_szPoisonMsg[iLevel]);
			}
			else
			{
				if ( iLevel < 50 )
					return false;
				if ( iLevel < 200 )			// Lesser
					iLevel = 0;
				else if ( iLevel < 400 )	// Normal
					iLevel = 1;
				else if ( iLevel < 800 )	// Greater
					iLevel = 2;
				else						// Deadly
					iLevel = 3;

				pItem->m_itSpell.m_spelllevel -= 50;	// gets weaker too.	Only on old formulas
				iDmg = IMULDIV(Stat_GetMax(STAT_STR), static_cast<LONGLONG>(iLevel) * 2, 100);
				pItem->SetTimeout((5 + Calc_GetRandLLVal(4)) * TICK_PER_SEC);

				static LPCTSTR const sm_szPoisonMsg[] =
				{
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_POISON_1),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_POISON_2),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_POISON_3),
					g_Cfg.GetDefaultMsg(DEFMSG_SPELL_POISON_4)
				};

				TCHAR *pszMsg = Str_GetTemp();
				sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_SPELL_LOOKS), sm_szPoisonMsg[iLevel]);
				Emote(NULL, pszMsg, m_pClient);
				SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_SPELL_YOUFEEL), sm_szPoisonMsg[iLevel]);
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
			double dStamPenalty = 3 - (static_cast<double>(Stat_GetVal(STAT_DEX) / maximum(1, Stat_GetAdjusted(STAT_DEX))) * 2);
			int iDmg = static_cast<int>(Calc_GetRandVal2(pItem->m_itSpell.m_spelllevel - 2, pItem->m_itSpell.m_spelllevel + 1) * dStamPenalty);
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

CItem *CChar::Spell_Effect_Create(SPELL_TYPE spell, LAYER_TYPE layer, int iSkillLevel, int iDuration, CObjBase *pSrc, bool fEquip)
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
	//  ATTR_MAGIC without ATTR_MOVE_NEVER is dispellable !

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

	if ( fEquip )
		LayerAdd(pSpell, layer);

	Spell_Effect_Add(pSpell);
	return pSpell;
}

void CChar::Spell_Area(CPointMap ptTarg, int iDist, int iSkillLevel)
{
	ADDTOCALLSTACK("CChar::Spell_Area");
	// Effects all creatures in the area. (but not us)
	// ARGS:
	// iSkillLevel = 0-1000

	SPELL_TYPE spelltype = m_atMagery.m_Spell;
	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spelltype);
	if ( !pSpellDef )
		return;

	CWorldSearch AreaChar(ptTarg, iDist);
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
		CWorldSearch AreaItem(ptTarg, iDist);
		for (;;)
		{
			CItem *pItem = AreaItem.GetItem();
			if ( !pItem )
				break;
			pItem->OnSpellEffect(spelltype, this, iSkillLevel, NULL);
		}
	}
}

void CChar::Spell_Field(CPointMap ptTarg, ITEMID_TYPE idEW, ITEMID_TYPE idNS, BYTE bFieldWidth, BYTE bFieldGauge, int iSkillLevel, CChar *pCharSrc, int iDuration, HUE_TYPE wColor)
{
	ADDTOCALLSTACK("CChar::Spell_Field");
	// Cast the field spell to here.
	// ARGS:
	// ptTarg = target
	// idEW = ID of EW aligned spell object
	// idNS = ID of NS aligned spell object
	// bFieldWidth = width of the field (looking from char's point of view)
	// bFieldGauge = thickness of the field
	// iSkillLevel = 0-1000
	// idnewEW and idnewNS are the overriders created in @Success trigger, passed as another arguments because checks are made using default items

	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_Spell);
	if ( !pSpellDef )
		return;

	if ( m_pArea && m_pArea->IsGuarded() && pSpellDef->IsSpellType(SPELLFLAG_HARM) )
		Noto_Criminal();

	// get the dir of the field.
	int dx = abs(ptTarg.m_x - GetTopPoint().m_x);
	int dy = abs(ptTarg.m_y - GetTopPoint().m_y);
	ITEMID_TYPE id = (dx > dy) ? idNS : idEW;

	int minX = static_cast<int>((bFieldWidth - 1) / 2) - (bFieldWidth - 1);
	int maxX = minX + (bFieldWidth - 1);

	int minY = static_cast<int>((bFieldGauge - 1) / 2) - (bFieldGauge - 1);
	int maxY = minY + (bFieldGauge - 1);

	if ( iDuration <= 0 )
		iDuration = GetSpellDuration(m_atMagery.m_Spell, iSkillLevel, pCharSrc);

	if ( IsSetMagicFlags(MAGICF_NOFIELDSOVERWALLS) )
	{
		// check if anything is blocking the field from fully extending to its desired width

		// first checks center piece, then left direction (minX), and finally right direction (maxX)
		// (structure of the loop looks a little odd but it should be more effective for wide fields (we don't really
		// want to be testing the far left or right of the field when it has been blocked towards the center))
		for ( int ix = 0; ; (ix <= 0) ? --ix : ++ix )
		{
			if ( ix < minX )
				ix = 1;	// start checking right extension
			if ( ix > maxX )
				break;	// all done

			// check the whole width of the field for anything that would block this placement
			for ( int iy = minY; iy <= maxY; ++iy )
			{
				CPointMap pt = ptTarg;
				if ( dx > dy )
				{
					pt.m_x += static_cast<signed short>(iy);
					pt.m_y += static_cast<signed short>(ix);
				}
				else
				{
					pt.m_x += static_cast<signed short>(ix);
					pt.m_y += static_cast<signed short>(iy);
				}

				DWORD dwBlockFlags = 0;
				g_World.GetHeightPoint2(pt, dwBlockFlags, true);
				if ( dwBlockFlags & (CAN_I_BLOCK|CAN_I_DOOR) )
				{
					if ( ix < 0 )	// field cannot extend fully to the left
						minX = ix + 1;
					else if ( ix > 0 )	// field cannot extend fully to the right
						maxX = ix - 1;
					else	// center piece is blocked, field cannot be created at all
						return;
					break;
				}
			}
		}
	}

	for ( int ix = minX; ix <= maxX; ++ix )
	{
		for ( int iy = minY; iy <= maxY; ++iy )
		{
			bool fGoodLoc = true;

			// Where is this ?
			CPointMap pt = ptTarg;
			if ( dx > dy )
			{
				pt.m_y += static_cast<signed short>(ix);
				pt.m_x += static_cast<signed short>(iy);
			}
			else
			{
				pt.m_x += static_cast<signed short>(ix);
				pt.m_y += static_cast<signed short>(iy);
			}

			// Check for direct cast on a creature.
			CWorldSearch AreaChar(pt);
			for (;;)
			{
				CChar *pChar = AreaChar.GetChar();
				if ( !pChar )
					break;
				if ( pChar->GetPrivLevel() > GetPrivLevel() )	// skip higher priv characters
					continue;
				if ( pSpellDef->IsSpellType(SPELLFLAG_HARM) && !pChar->OnAttackedBy(this, false) )	// they should know they where attacked
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
			CWorldSearch AreaItem(pt);
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

			CItem *pSpell = CItem::CreateScript(id);
			ASSERT(pSpell);
			pSpell->m_itSpell.m_spell = static_cast<WORD>(m_atMagery.m_Spell);
			pSpell->m_itSpell.m_spelllevel = static_cast<WORD>(iSkillLevel);
			pSpell->m_itSpell.m_spellcharges = 1;
			pSpell->m_uidLink = GetUID();	// link it back to you
			pSpell->SetType(IT_SPELL);
			pSpell->SetAttr(ATTR_MAGIC);
			pSpell->SetHue(wColor);
			pSpell->MoveToDecay(pt, iDuration, true);
		}
	}
}

bool CChar::Spell_CanCast(SPELL_TYPE &spell, bool fTest, CObjBase *pSrc, bool fFailMsg, bool fCheckAntiMagic)
{
	ADDTOCALLSTACK("CChar::Spell_CanCast");
	// ARGS:
	//  pSrc = possible scroll or wand source.
	if ( !pSrc || (spell <= SPELL_NONE) || (m_pPlayer && (spell > SPELL_SKILLMASTERIES_QTY)) )
		return false;

	CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
	if ( !pSpellDef )
		return false;
	if ( pSpellDef->IsSpellType(SPELLFLAG_DISABLED) )
		return false;

	int iSkill = SKILL_NONE;
	if ( !pSpellDef->GetPrimarySkill(&iSkill, NULL) )
		iSkill = SKILL_MAGERY;
	if ( !Skill_CanUse(static_cast<SKILL_TYPE>(iSkill)) )
		return false;

	int iManaUse = pSpellDef->m_wManaUse * (100 - minimum(m_LowerManaCost, 40)) / 100;
	int iTithingUse = 0;
	if ( m_LowerReagentCost <= Calc_GetRandVal(100) )
		iTithingUse = pSpellDef->m_wTithingUse;

	if ( pSrc != this )
	{
		CItem *pItem = dynamic_cast<CItem *>(pSrc);
		if ( pItem )
		{
			if ( pItem->IsType(IT_WAND) )
			{
				iManaUse = 0;
				iTithingUse = 0;
			}
			else if ( pItem->IsType(IT_SCROLL) )
			{
				iManaUse /= 2;
				iTithingUse /= 2;
			}
		}
	}

	CScriptTriggerArgs Args(spell, iManaUse, pSrc);
	if ( fTest )
		Args.m_iN3 |= 0x1;
	if ( fFailMsg )
		Args.m_iN3 |= 0x2;
	Args.m_VarsLocal.SetNum("TithingUse", iTithingUse);

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
			if ( !fTest && (pItem->m_itWeapon.m_spellcharges != 255) )
			{
				--pItem->m_itWeapon.m_spellcharges;
				pItem->UpdatePropertyFlag();
			}
		}
		else if ( pItem->IsType(IT_SCROLL) )
		{
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
				if ( m_LowerReagentCost <= Calc_GetRandVal(100) )
				{
					const CResourceQtyArray *pRegs = &pSpellDef->m_Reags;
					CItemContainer *pPack = GetContainerCreate(LAYER_PACK);
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
				SysMessageDefault(DEFMSG_MAGERY_6);
			m_Act_Difficulty = -1;	// give very little credit for failure
			return false;
		}
	}

	// Check required mana
	iManaUse = static_cast<int>(Args.m_iN2);
	if ( Stat_GetVal(STAT_INT) < iManaUse )
	{
		if ( fFailMsg )
			SysMessageDefault(DEFMSG_SPELL_TRY_NOMANA);
		return false;
	}
	if ( iManaUse && !fTest )
	{
		if ( m_Act_Difficulty < 0 )	// use diff amount of mana if we fail.
			iManaUse = iManaUse / 2 + Calc_GetRandVal(iManaUse / 2 + iManaUse / 4);
		UpdateStatVal(STAT_INT, -iManaUse);
	}

	// Check required tithing points
	iTithingUse = static_cast<int>(Args.m_VarsLocal.GetKeyNum("TithingUse"));
	if ( m_Tithing < iTithingUse )
	{
		if ( fFailMsg )
			SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_SPELL_TRY_NOTITHING), iTithingUse);
		return false;
	}
	if ( iTithingUse && !fTest )
	{
		if ( m_Act_Difficulty < 0 )	// use diff amount of points if we fail.
			iTithingUse = iTithingUse / 2 + Calc_GetRandVal(iTithingUse / 2 + iTithingUse / 4);
		m_Tithing -= iTithingUse;
	}
	return true;
}

bool CChar::Spell_TargCheck_Face()
{
	ADDTOCALLSTACK("CChar::Spell_TargCheck_Face");
	if ( !IsSetMagicFlags(MAGICF_NODIRCHANGE) )
		UpdateDir(m_Act_p);

	// Check if target is on anti-magic region
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
		if ( !CanSeeLOS(pObj, LOS_NB_WINDOWS) )		// we should be able to cast through a window
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
		else if ( !pItem->IsTypeSpellbook() && !pItem->IsType(IT_WAND) && !pItem->m_SpellChanneling )
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

	SPELL_TYPE spell = m_atMagery.m_Spell;
	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
	if ( !pSpellDef )
		return false;

	int iSkill, iDifficulty;
	if ( !pSpellDef->GetPrimarySkill(&iSkill, &iDifficulty) )
		return false;

	CObjBase *pObjSrc = m_Act_TargPrv.ObjFind();

	int iSkillLevel;
	if ( pObjSrc == this )
	{
		iSkillLevel = Skill_GetAdjusted(static_cast<SKILL_TYPE>(iSkill));

		if ( (iSkill == SKILL_MYSTICISM) && (iSkillLevel < 300) && (g_Cfg.m_iRacialFlags & RACIALF_GARG_MYSTICINSIGHT) && IsGargoyle() )
			iSkillLevel = 300;	// gargoyles always have a minimum of 30.0 Mysticism (Mystic Insight racial trait)
	}
	else
	{
		// Get the strength of the item (IT_SCROLL or IT_WAND)
		CItem *pItem = dynamic_cast<CItem *>(pObjSrc);
		if ( !pItem )
			return false;

		iSkillLevel = pItem->m_itWeapon.m_spelllevel ? pItem->m_itWeapon.m_spelllevel : Calc_GetRandVal(500);
	}

	CScriptTriggerArgs Args(spell, iSkillLevel, pObjSrc);
	Args.m_VarsLocal.SetNum("Duration", GetSpellDuration(spell, iSkillLevel, this) / TICK_PER_SEC, true);

	bool fFieldSpell = pSpellDef->IsSpellType(SPELLFLAG_FIELD);
	ITEMID_TYPE iT1 = ITEMID_NOTHING;
	ITEMID_TYPE iT2 = ITEMID_NOTHING;

	if ( fFieldSpell )
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

	// Consume the reagents/mana/scroll/charge
	if ( !Spell_CanCast(spell, false, pObjSrc, true) )
		return false;

	CObjBase *pObj = m_Act_Targ.ObjFind();	// dont always need a target
	CREID_TYPE iC1 = static_cast<CREID_TYPE>(Args.m_VarsLocal.GetKeyNum("CreateObject1") & 0xFFFF);
	BYTE bAreaRadius = static_cast<BYTE>(maximum(0, Args.m_VarsLocal.GetKeyNum("AreaRadius")));
	int iDuration = maximum(0, static_cast<int>(Args.m_VarsLocal.GetKeyNum("Duration") * TICK_PER_SEC));
	HUE_TYPE wColor = static_cast<HUE_TYPE>(maximum(0, Args.m_VarsLocal.GetKeyNum("EffectColor")));
	iSkillLevel = static_cast<int>(Args.m_iN2);

	BYTE bFieldWidth = 0;
	BYTE bFieldGauge = 0;

	if ( fFieldSpell )
	{
		// Setting new IDs as another variables to pass as different arguments to the field function
		iT1 = static_cast<ITEMID_TYPE>(RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("CreateObject1")));
		iT2 = static_cast<ITEMID_TYPE>(RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("CreateObject2")));

		bFieldWidth = static_cast<BYTE>(maximum(0, Args.m_VarsLocal.GetKeyNum("FieldWidth")));
		bFieldGauge = static_cast<BYTE>(maximum(0, Args.m_VarsLocal.GetKeyNum("FieldGauge")));
	}

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
		else if ( fFieldSpell )
		{
			if ( iT1 && iT2 )
			{
				if ( !bFieldWidth )
					bFieldWidth = 3;
				if ( !bFieldGauge )
					bFieldGauge = 1;

				Spell_Field(m_Act_p, iT1, iT2, bFieldWidth, bFieldGauge, iSkillLevel, this, iDuration, wColor);
			}
		}
		else if ( pSpellDef->IsSpellType(SPELLFLAG_AREA) )
		{
			if ( !bAreaRadius )
				bAreaRadius = 4;

			if ( !pSpellDef->IsSpellType(SPELLFLAG_TARG_OBJ|SPELLFLAG_TARG_XYZ) )
				Spell_Area(GetTopPoint(), bAreaRadius, iSkillLevel);
			else
				Spell_Area(m_Act_p, bAreaRadius, iSkillLevel);
		}
		else if ( pSpellDef->IsSpellType(SPELLFLAG_POLY) )
			return false;
		else if ( pObj )
			pObj->OnSpellEffect(spell, this, iSkillLevel, dynamic_cast<CItem *>(pObjSrc));
	}
	else if ( fFieldSpell )
	{
		if ( !bFieldWidth )
			bFieldWidth = 3;
		if ( !bFieldGauge )
			bFieldGauge = 1;

		Spell_Field(m_Act_p, iT1, iT2, bFieldWidth, bFieldGauge, iSkillLevel, this, iDuration, wColor);
	}
	else if ( pSpellDef->IsSpellType(SPELLFLAG_AREA) )
	{
		if ( !bAreaRadius )
		{
			switch ( spell )
			{
				case SPELL_Arch_Cure:		bAreaRadius = 2;						break;
				case SPELL_Arch_Prot:		bAreaRadius = 3;						break;
				case SPELL_Mass_Curse:		bAreaRadius = 2;						break;
				case SPELL_Reveal:			bAreaRadius = 1 + (iSkillLevel / 200);	break;
				case SPELL_Chain_Lightning: bAreaRadius = 2;						break;
				case SPELL_Mass_Dispel:		bAreaRadius = 8;						break;
				case SPELL_Meteor_Swarm:	bAreaRadius = 2;						break;
				case SPELL_Earthquake:		bAreaRadius = 1 + (iSkillLevel / 150);	break;
				case SPELL_Poison_Strike:	bAreaRadius = 2;						break;
				case SPELL_Wither:			bAreaRadius = 4;						break;
				default:					bAreaRadius = 4;						break;
			}
		}

		if ( !pSpellDef->IsSpellType(SPELLFLAG_TARG_OBJ|SPELLFLAG_TARG_XYZ) )
			Spell_Area(GetTopPoint(), bAreaRadius, iSkillLevel);
		else
			Spell_Area(m_Act_p, bAreaRadius, iSkillLevel);
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
		switch ( spell )
		{
			case SPELL_Create_Food:
			{
				RESOURCE_ID rid = g_Cfg.ResourceGetIDType(RES_ITEMDEF, "DEFFOOD");
				CItem *pItem = CItem::CreateScript(static_cast<ITEMID_TYPE>(rid.GetResIndex()), this);
				ASSERT(pItem);
				if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_OBJ|SPELLFLAG_TARG_XYZ) )
					pItem->MoveToCheck(m_Act_p, this, true);
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
					if ( !pChar )
						return false;
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
					CItem *pItem = CItem::CreateBase(ITEMID_FX_FLAMESTRIKE);
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
				if ( !pObj || ((pObj != this) && !IsPriv(PRIV_GM)) )
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
				for ( size_t i = 0; i < COUNTOF(sm_Item_Bone); ++i )
				{
					if ( !Calc_GetRandVal(2 + iGet) )
						break;
					CItem *pItem = CItem::CreateScript(sm_Item_Bone[i], this);
					pItem->MoveToCheck(m_Act_p, this, true);
					++iGet;
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
		CChar *pChar = static_cast<CChar *>(pObj);
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
	CScriptTriggerArgs Args(m_atMagery.m_Spell, 0, m_Act_TargPrv.ObjFind());
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

	HUE_TYPE wColor = static_cast<HUE_TYPE>(maximum(0, Args.m_VarsLocal.GetKeyNum("EffectColor")));
	DWORD dwRender = static_cast<DWORD>(maximum(0, Args.m_VarsLocal.GetKeyNum("EffectRender")));

	iT1 = static_cast<ITEMID_TYPE>(RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("CreateObject1")));
	if ( iT1 != ITEMID_NOTHING )
		Effect(EFFECT_OBJ, iT1, this, 1, 30, false, wColor, dwRender);
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

	CSpellDef *pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_Spell);
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
	bool fWOP = IsPriv(PRIV_GM) ? g_Cfg.m_fWordsOfPowerStaff : g_Cfg.m_fWordsOfPowerPlayer;
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
	iWaitTime -= static_cast<INT64>(m_FasterCasting) * 2;	// correct value is 0.25, but sphere can handle only 0.2
	if ( (iWaitTime < 1) || IsPriv(PRIV_GM) )
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
			for ( size_t i = 0; ; ++i )
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
				pszTemp[len] = '\0';
				Speak(pszTemp, WOPColor, TALKMODE_SAY, WOPFont);
			}
		}
	}
	SetTimeout(iWaitTime);
	return iDifficulty;
}

bool CChar::OnSpellEffect(SPELL_TYPE spell, CChar *pCharSrc, int iSkillLevel, CItem *pSourceItem, bool fReflecting)
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
	if ( (spell == SPELL_Paralyze_Field) && IsStatFlag(STATF_Freeze) )
		return false;
	if ( (spell == SPELL_Poison_Field) && IsStatFlag(STATF_Poisoned) )
		return false;

	iSkillLevel = iSkillLevel / 2 + Calc_GetRandVal(iSkillLevel / 2);	// randomize the potency
	int iEffect = g_Cfg.GetSpellEffect(spell, iSkillLevel);
	int iDuration = pSpellDef->m_idLayer ? GetSpellDuration(spell, iSkillLevel, pCharSrc) : 0;
	SOUND_TYPE iSound = pSpellDef->m_sound;
	bool fExplode = (pSpellDef->IsSpellType(SPELLFLAG_FX_BOLT) && !pSpellDef->IsSpellType(SPELLFLAG_GOOD));		// bolt (chasing) spells have explode = 1 by default (if not good spell)
	bool fPotion = (pSourceItem && pSourceItem->IsType(IT_POTION));
	if ( fPotion )
	{
		static const SOUND_TYPE sm_DrinkSounds[] = { 0x30, 0x31 };
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

	if ( pSpellDef->IsSpellType(SPELLFLAG_DAMAGE) && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
	{
		if ( pCharSrc )
		{
			// Evaluating Intelligence mult
			iEffect *= ((pCharSrc->Skill_GetBase(SKILL_EVALINT) * 3) / 1000) + 1;

			// Spell Damage Increase bonus
			int iDamageBonus = pCharSrc->m_SpellDamIncrease;
			if ( (iDamageBonus > 15) && m_pPlayer && pCharSrc->m_pPlayer )		// Spell Damage Increase is capped at 15% on PvP
				iDamageBonus = 15;

			// INT bonus
			iDamageBonus += pCharSrc->Stat_GetAdjusted(STAT_INT) / 10;

			// Inscription bonus
			iDamageBonus += pCharSrc->Skill_GetBase(SKILL_INSCRIPTION) / 100;

			// Racial Bonus (Berserk), gargoyles gains +3% Spell Damage Increase per each 20 HP lost
			if ( (g_Cfg.m_iRacialFlags & RACIALF_GARG_BERSERK) && IsGargoyle() )
				iDamageBonus += minimum(3 * ((Stat_GetMax(STAT_STR) - Stat_GetVal(STAT_STR)) / 20), 12);		// value is capped at 12%

			iEffect += iEffect * iDamageBonus / 100;
		}
		else
			iEffect *= ((iSkillLevel * 3) / 1000) + 1;
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

	if ( pSpellDef->IsSpellType(SPELLFLAG_HARM) )
	{
		if ( (pCharSrc == this) && !IsSetMagicFlags(MAGICF_CANHARMSELF) && !fReflecting )
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

		if ( !OnAttackedBy(pCharSrc, false, !pSpellDef->IsSpellType(SPELLFLAG_FIELD)) && !fReflecting )
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

	ITEMID_TYPE iEffectID = static_cast<ITEMID_TYPE>(RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("CreateObject1")));
	HUE_TYPE wColor = static_cast<HUE_TYPE>(maximum(0, Args.m_VarsLocal.GetKeyNum("EffectColor")));
	DWORD dwRender = static_cast<DWORD>(maximum(0, Args.m_VarsLocal.GetKeyNum("EffectRender")));
	fExplode = (Args.m_VarsLocal.GetKeyNum("Explode") > 0);
	iSound = static_cast<SOUND_TYPE>(Args.m_VarsLocal.GetKeyNum("Sound"));
	iEffect = static_cast<int>(Args.m_VarsLocal.GetKeyNum("Effect"));
	iDuration = static_cast<int>(Args.m_VarsLocal.GetKeyNum("Duration"));

	if ( pSpellDef->IsSpellType(SPELLFLAG_DAMAGE) )
	{
		iResist = static_cast<int>(Args.m_VarsLocal.GetKeyNum("Resist"));
		if ( iResist > 0 )
		{
			SysMessageDefault(DEFMSG_RESISTMAGIC);
			iEffect -= iEffect * iResist / 100;
			if ( iEffect < 0 )
				iEffect = 0;	//May not do damage, but aversion should be created from the target.
		}

		DAMAGE_TYPE iDmgType = static_cast<DAMAGE_TYPE>(RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("DamageType")));
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
			UpdateStatVal(STAT_STR, iEffect);
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
			{
				iEffectID = ITEMID_NOTHING;
				iSound = SOUND_NONE;
			}
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
			UpdateStatVal(STAT_INT, -iSkillLevel);
			if ( pCharSrc )
				pCharSrc->UpdateStatVal(STAT_INT, +iSkillLevel);
			break;
		}

		case SPELL_Lightning:
		case SPELL_Chain_Lightning:
			Effect(EFFECT_LIGHTNING, ITEMID_NOTHING, pCharSrc);
			break;

		case SPELL_Resurrection:
			return Spell_Resurrection(NULL, pCharSrc, (pSourceItem && pSourceItem->IsType(IT_SHRINE)));

		case SPELL_Light:
			Spell_Effect_Create(spell, LAYER_FLAG_Potion, iSkillLevel, iDuration, pCharSrc);
			break;

		case SPELL_Hallucination:
		{
			CItem *pItem = Spell_Effect_Create(spell, LAYER_FLAG_Hallucination, iSkillLevel, 10 * TICK_PER_SEC, pCharSrc);
			if ( pItem )
				pItem->m_itSpell.m_spellcharges = Calc_GetRandVal(30);
			break;
		}

		case SPELL_Shrink:
		{
			if ( m_pPlayer )
				break;
			if ( fPotion )
				pSourceItem->Delete();

			CItem *pItem = NPC_Shrink();
			if ( pCharSrc && pItem )
				pCharSrc->m_Act_Targ = pItem->GetUID();
			break;
		}

		case SPELL_Mana:
			UpdateStatVal(STAT_INT, iEffect);
			break;

		case SPELL_Refresh:
			UpdateStatVal(STAT_DEX, iEffect);
			break;

		case SPELL_Restore:		// increases both your hit points and your stamina.
			UpdateStatVal(STAT_DEX, iEffect);
			UpdateStatVal(STAT_STR, iEffect);
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

	if ( (iEffectID > ITEMID_NOTHING) && (iEffectID < ITEMID_QTY) )
	{
		if ( pSpellDef->IsSpellType(SPELLFLAG_FX_BOLT) )
			Effect(EFFECT_BOLT, iEffectID, pCharSrc, 5, 1, fExplode, wColor, dwRender);
		if ( pSpellDef->IsSpellType(SPELLFLAG_FX_TARG) )
			Effect(EFFECT_OBJ, iEffectID, this, 0, 15, fExplode, wColor, dwRender);
	}

	if ( iSound )
		Sound(iSound);

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
				break;
			}

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
		if ( pSpellDef )
			iDuration = pSpellDef->m_Duration.GetLinear(iSkillLevel) / 10;
	}
	return iDuration * TICK_PER_SEC;
}
