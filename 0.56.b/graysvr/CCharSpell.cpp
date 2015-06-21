//
// CCharSpell.cpp
//

#include "graysvr.h"	// predef header.
#include "../network/send.h"

SPELL_TYPE CChar::Spell_GetIndex(SKILL_TYPE skill)	// Returns the first spell for the given skill
{
	if (skill == SKILL_NONE)	// providing no skill returns first monster's custom spell.
		return SPELL_Summon_Undead;

	if (!g_Cfg.IsSkillFlag(skill, SKF_MAGIC))
		return SPELL_NONE;

	switch (skill)
	{
	case SKILL_MAGERY:
		return SPELL_Clumsy;
	case SKILL_NECROMANCY:
		return SPELL_Animate_Dead_AOS;
	case SKILL_CHIVALRY:
		return SPELL_Cleanse_by_Fire;
	case SKILL_BUSHIDO:
		return SPELL_Honorable_Execution;
	case SKILL_NINJITSU:
		return SPELL_Focus_Attack;
	case SKILL_SPELLWEAVING:
		return SPELL_Arcane_Circle;
	case SKILL_MYSTICISM:
		return SPELL_Nether_Bolt;
	}
	return SPELL_NONE;
}

SPELL_TYPE CChar::Spell_GetMax(SKILL_TYPE skill)
{
	if (skill == SKILL_NONE)	// providing no skill returns the last spell for monsters.
		return SPELL_QTY;

	if (!g_Cfg.IsSkillFlag(skill, SKF_MAGIC))
		return SPELL_NONE;

	switch (skill)
	{
	case SKILL_MAGERY:
		return SPELL_BOOK_QTY;
	case SKILL_NECROMANCY:
		return SPELL_NECROMANCY_QTY;
	case SKILL_CHIVALRY:
		return SPELL_CHIVALRY_QTY;
	case SKILL_BUSHIDO:
		return SPELL_BUSHIDO_QTY;
	case SKILL_NINJITSU:
		return SPELL_NINJITSU_QTY;
	case SKILL_SPELLWEAVING:
		return SPELL_SPELLWEAVING_QTY;
	case SKILL_MYSTICISM:
		return SPELL_MYSTICISM_QTY;
	default:
		break;
	}
	return SPELL_NONE;
}

void CChar::Spell_Dispel(int iLevel)
{
	ADDTOCALLSTACK("CChar::Spell_Dispel");
	// ARGS: iLevel = 0-100 level of dispel caster.
	// remove all the spells. NOT if caused by objects worn !!!
	// ATTR_MAGIC && ! ATTR_MOVE_NEVER

	CItem *pItemNext;
	CItem *pItem = GetContentHead();
	for (; pItem != NULL; pItem = pItemNext)
	{
		pItemNext = pItem->GetNext();
		if (iLevel <= 100 && pItem->IsAttr(ATTR_MOVE_NEVER))	// we don't lose this.
			continue;
		if (pItem->GetEquipLayer() == LAYER_NEWLIGHT || (pItem->GetEquipLayer() >= LAYER_SPELL_STATS && pItem->GetEquipLayer() <= LAYER_SPELL_Summon))
			pItem->Delete();
	}
}

bool CChar::Spell_Teleport( CPointBase ptNew, bool fTakePets, bool fCheckAntiMagic,	ITEMID_TYPE iEffect, SOUND_TYPE iSound )
{
	ADDTOCALLSTACK("CChar::Spell_Teleport");
	// Teleport you to this place.
	// This is sometimes not really a spell at all.
	// ex. ships plank.
	// RETURN: true = it worked.

	if ( ! ptNew.IsCharValid())
		return false;

	ptNew.m_z = GetFixZ(ptNew);

	if ( g_Cfg.m_iMountHeight )
	{
		if ( ! IsVerticalSpace( ptNew, false ) )
		{
			SysMessageDefault( DEFMSG_MOUNT_CEILING );
			return false;
		}
	}

	Reveal();

	if ( IsPriv(PRIV_JAILED))
	{
		CRegionBase *pJail = g_Cfg.GetRegion( "jail" );
		if (pJail == NULL || pJail->IsInside2d(ptNew) == false)
		{
			// Must be /PARDONed to leave jail area
			static LPCTSTR const sm_szPunishMsg[] =
			{
				g_Cfg.GetDefaultMsg( DEFMSG_SPELL_TELE_JAILED_1 ),
				g_Cfg.GetDefaultMsg( DEFMSG_SPELL_TELE_JAILED_2 )
			};
			SysMessage( sm_szPunishMsg[ Calc_GetRandVal( COUNTOF( sm_szPunishMsg )) ]);

			int iCell = 0;
			if ( m_pPlayer && m_pPlayer->GetAccount() )
				iCell = static_cast<int>(m_pPlayer->GetAccount()->m_TagDefs.GetKeyNum("JailCell", true));

			if ( iCell )
			{
				TCHAR szJailName[ 128 ];
				sprintf( szJailName, "jail%d", iCell );
				pJail = g_Cfg.GetRegion( szJailName );
			}

			if ( pJail != NULL )
				ptNew = pJail->m_pt;
			else
				ptNew.InitPoint();
		}
	}

	// Is it a valid teleport location that allows this ?

	if ( IsPriv( PRIV_GM ))
	{
		fCheckAntiMagic = false;
		if ( iEffect && ! IsStatFlag( STATF_Incognito ) && ! IsPriv( PRIV_PRIV_NOSHOW ))
		{
			iEffect = g_Cfg.m_iSpell_Teleport_Effect_Staff;
			iSound = g_Cfg.m_iSpell_Teleport_Sound_Staff;
		}
	}
	else if ( fCheckAntiMagic )
	{
		CRegionBase * pArea = CheckValidMove( ptNew, NULL, DIR_QTY, NULL );
		if ( pArea == NULL )
		{
			SysMessageDefault( DEFMSG_SPELL_TELE_CANT );
			return false;
		}
		if ( pArea->IsFlag( REGION_ANTIMAGIC_RECALL_IN | REGION_ANTIMAGIC_TELEPORT ))
		{
			SysMessageDefault( DEFMSG_SPELL_TELE_AM );
			return false;
		}
	}

	if ( IsClient() )
	{
		if ( IsStatFlag( STATF_Insubstantial ))
			iEffect = ITEMID_NOTHING;
	}

	if ( iEffect == ITEMID_TEMPLATE )
	{
		if ( IsClient() )
		{
			iEffect = g_Cfg.m_iSpell_Teleport_Effect_Players;
			iSound = g_Cfg.m_iSpell_Teleport_Sound_Players;
		} else {
			iEffect = g_Cfg.m_iSpell_Teleport_Effect_NPC;
			iSound = g_Cfg.m_iSpell_Teleport_Sound_NPC;
		}
	}
	
	if (GetTopPoint().IsValidPoint())	// Guards might have just been created
	{
		if (fTakePets)
		{
			// Look for any creatures that might be following me near by.
			CWorldSearch Area(GetTopPoint(), UO_MAP_VIEW_SIGHT);
			for (;;)
			{
				CChar * pChar = Area.GetChar();
				if (pChar == NULL)
					break;
				if (pChar == this)
					continue;

				// My pets ?
				if (pChar->Skill_GetActive() == NPCACT_FOLLOW_TARG && pChar->m_Act_Targ == GetUID())
				{
					CPointMap masterp = GetTopPoint();	// it will modify the val.
					if (!pChar->CanMoveWalkTo(masterp, false))
						continue;
					pChar->Spell_Teleport(ptNew, fTakePets, fCheckAntiMagic, iEffect, iSound);
				}
			}
		}
	}

	CPointMap ptOld = GetTopPoint();
	m_fClimbUpdated = false;			// update climb height here
	MoveToChar( ptNew );				// move character
	UpdateMove( ptOld, NULL, true );	// update other characters

	if ( iEffect != ITEMID_NOTHING )
	{
		// Departing effect
		if ( ptOld.IsValidPoint())
		{
			CItem * pItem = CItem::CreateBase( ITEMID_NODRAW );
			ASSERT(pItem);
			pItem->SetAttr( ATTR_MOVE_NEVER );
			pItem->MoveTo( ptOld );
			pItem->Effect( EFFECT_XYZ, iEffect, this, 10, 10 );
			pItem->Delete();
		}

		// Entering effect
		Effect( EFFECT_XYZ, iEffect, this, 10, 10 );
		Sound( iSound );
	}

	return( true );
}

CChar * CChar::Spell_Summon( CREID_TYPE id, CPointMap pntTarg, bool fSpellSummon )
{
	ADDTOCALLSTACK("CChar::Spell_Summon");
	if ( id == CREID_INVALID )
		return( NULL );

	CChar * pChar;
	if ( !fSpellSummon )
	{
		// GM creates a char this way. "ADDNPC"
		// More specific npc type from script ?
		pChar = CreateNPC( id );
		if ( pChar == NULL )
			return NULL;
		pChar->MoveToChar( pntTarg );
	}
	else
	{
		pChar = CChar::CreateBasic(id);
		if (pChar == NULL)
			return NULL;

		if (!IsPriv(PRIV_GM))
		{
			CCharBase *pSummonDef = CCharBase::FindCharBase(id);
			if (IsSetMagicFlags(MAGICF_SUMMONWALKCHECK))	// check if the target location is valid
			{
				WORD wCan = 0xFFFF;
				if (pSummonDef != NULL)
					wCan = pSummonDef->m_Can & CAN_C_MOVEMASK;

				if (wCan != 0xFFFF)
				{
					DWORD wBlockFlags = 0;
					g_World.GetHeightPoint2(pntTarg, wBlockFlags, true);

					if (wBlockFlags &~wCan)
					{
						SysMessageDefault(DEFMSG_SUMMON_INVALIDTARG);
						return NULL;
					}
				}
			}

			if (IsSetOF(OF_PetSlots))
			{
				short iFollowerSlotsNeeded = static_cast<short>(maximum(pChar->GetDefNum("FOLLOWERSLOTS", true, true), 1));
				short iCurFollower = static_cast<short>(GetDefNum("CURFOLLOWER", true, true));
				short iMaxFollower = static_cast<short>(GetDefNum("MAXFOLLOWER", true, true));

				if ( FollowersUpdate(pChar, false, &iFollowerSlotsNeeded) == false || iFollowerSlotsNeeded + iCurFollower > iMaxFollower )
				{
					SysMessageDefault(DEFMSG_PETSLOTS_TRY_SUMMON);
					pChar->Delete();
					return NULL;
				}
			}
		}

		if (g_Cfg.m_iMountHeight && !pChar->IsVerticalSpace(GetTopPoint(), false))
		{
			SysMessageDefault(DEFMSG_SUMMON_CEILING);
			pChar->Delete();
			return NULL;
		}

		pChar->StatFlag_Set(STATF_Conjured);	// conjured creates have no loot
		pChar->NPC_LoadScript(false);
		pChar->MoveToChar(pntTarg);
		pChar->NPC_CreateTrigger();		// removed from NPC_LoadScript() and triggered after char placement
		pChar->NPC_PetSetOwner(this);

		int skill;
		const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(g_Cfg.IsSkillFlag(m_Act_SkillCurrent, SKF_MAGIC) ? m_atMagery.m_Spell : SPELL_Summon);
		if (!pSpellDef || !pSpellDef->GetPrimarySkill(&skill, NULL))
		{
			pChar->Delete();
			return NULL;
		}
		pChar->OnSpellEffect(SPELL_Summon, this, Skill_GetAdjusted(static_cast<SKILL_TYPE>(skill)), NULL);
	}

	m_Act_Targ = pChar->GetUID();	// for last target stuff.
	pChar->Update();
	pChar->UpdateAnimate( ANIM_CAST_DIR );
	pChar->SoundChar( CRESND_GETHIT );
	return pChar;
}

bool CChar::Spell_Recall(CItem * pRune, bool fGate)
{
	ADDTOCALLSTACK("CChar::Spell_Recall");
	if (pRune && (IsTrigUsed(TRIGGER_SPELLEFFECT) || IsTrigUsed(TRIGGER_ITEMSPELL)))
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = fGate ? SPELL_Gate_Travel : SPELL_Recall;

		if (pRune->OnTrigger(ITRIG_SPELLEFFECT, this, &Args) == TRIGRET_RET_FALSE)
			return true;
	}

	if (pRune == NULL || (!pRune->IsType(IT_RUNE) && !pRune->IsType(IT_TELEPAD)))
	{
		SysMessageDefault(DEFMSG_SPELL_RECALL_NOTRUNE);
		return(false);
	}
	if (!pRune->m_itRune.m_pntMark.IsValidPoint())
	{
		SysMessageDefault(DEFMSG_SPELL_RECALL_BLANK);
		return(false);
	}
	if (pRune->IsType(IT_RUNE) && pRune->m_itRune.m_Strength <= 0)
	{
		SysMessageDefault(DEFMSG_SPELL_RECALL_FADED);
		if (!IsPriv(PRIV_GM))
			return(false);
	}

	if (fGate)
	{
		CRegionBase * pArea = pRune->m_itRune.m_pntMark.GetRegion(REGION_TYPE_AREA | REGION_TYPE_MULTI | REGION_TYPE_ROOM);
		if (pArea == NULL)
			return(false);

		if (pArea->IsFlag(REGION_ANTIMAGIC_ALL | REGION_ANTIMAGIC_GATE | REGION_ANTIMAGIC_RECALL_IN | REGION_ANTIMAGIC_RECALL_OUT | REGION_ANTIMAGIC_RECALL_IN))	//anti-magic
		{
			SysMessageDefault(DEFMSG_SPELL_GATE_AM);
			if (!IsPriv(PRIV_GM))
				return(false);
		}

		const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(SPELL_Gate_Travel);
		ASSERT(pSpellDef);
		int iDuration = pSpellDef->m_Duration.GetLinear(0);

		CItem * pGate = CItem::CreateBase(pSpellDef->m_idEffect);
		ASSERT(pGate);
		pGate->m_uidLink = GetUID();
		pGate->SetType(IT_TELEPAD);
		pGate->SetAttr(ATTR_MAGIC | ATTR_MOVE_NEVER | ATTR_CAN_DECAY);	// why is this movable ?
		pGate->SetHue(static_cast<HUE_TYPE>(pArea->IsGuarded() ? HUE_DEFAULT : HUE_RED));
		pGate->m_itTelepad.m_pntMark = pRune->m_itRune.m_pntMark;
		pGate->MoveToDecay(GetTopPoint(), iDuration);
		pGate->Effect(EFFECT_OBJ, ITEMID_MOONGATE_FX_BLUE, pGate, 2);
		pGate->Sound(pSpellDef->m_sound);

		// Far end gate.
		pGate = CItem::CreateDupeItem(pGate);
		ASSERT(pGate);
		pGate->SetHue(static_cast<HUE_TYPE>((m_pArea && m_pArea->IsGuarded()) ? HUE_DEFAULT : HUE_RED));
		pGate->m_itTelepad.m_pntMark = GetTopPoint();
		pGate->MoveToDecay(pRune->m_itRune.m_pntMark, iDuration);
		pGate->Effect(EFFECT_OBJ, ITEMID_MOONGATE_FX_BLUE, pGate, 2);
		pGate->Sound(pSpellDef->m_sound);
	}
	else
	{
		if (!Spell_Teleport(pRune->m_itRune.m_pntMark, true, true, ITEMID_NOTHING))
			return(false);
	}

	if (pRune->IsType(IT_RUNE))	// wear out the rune
	{
		if (!IsPriv(PRIV_GM))
			pRune->m_itRune.m_Strength--;
		if (pRune->m_itRune.m_Strength < 10)
			SysMessageDefault(DEFMSG_SPELL_RECALL_SFADE);
		else if (!pRune->m_itRune.m_Strength)
			SysMessageDefault(DEFMSG_SPELL_RECALL_FADEC);
	}

	return(true);
}

bool CChar::Spell_Resurrection(CItemCorpse * pCorpse, CChar * pCharSrc, bool bNoFail)
{
	ADDTOCALLSTACK("CChar::Spell_Resurrection");
	if (!IsStatFlag(STATF_DEAD))
		return false;

	if (IsPriv(PRIV_GM) || (pCharSrc && pCharSrc->IsPriv(PRIV_GM)))
		bNoFail = true;

	if (!bNoFail && m_pArea && m_pArea->IsFlag(REGION_ANTIMAGIC_ALL))
	{
		SysMessageDefault(DEFMSG_SPELL_RES_AM);
		return false;
	}

	long long hits = IMULDIV(Stat_GetMax(STAT_STR), g_Cfg.m_iHitpointPercentOnRez, 100);
	if (!pCorpse)
		pCorpse = FindMyCorpse();

	if (IsTrigUsed(TRIGGER_RESURRECT))
	{
		CScriptTriggerArgs Args(hits, 0, pCorpse);
		if (OnTrigger(CTRIG_Resurrect, pCharSrc, &Args) == TRIGRET_RET_TRUE)
			return false;
		hits = Args.m_iN1;
	}

	SetID(m_prev_id);
	SetHue(m_prev_Hue);
	StatFlag_Clear(STATF_DEAD|STATF_Insubstantial);
	Stat_SetVal(STAT_STR, maximum(hits, 1));

	bool bRaisedCorpse = false;
	if (pCorpse != NULL)
	{
		if (RaiseCorpse(pCorpse))
		{
			SysMessageDefault(DEFMSG_SPELL_RES_REJOIN);
			bRaisedCorpse = true;
		}
	}

	if (m_pPlayer)
	{
		CItem *pDeathShroud = ContentFind(RESOURCE_ID(RES_ITEMDEF, ITEMID_DEATHSHROUD));
		if (pDeathShroud != NULL)
			pDeathShroud->Delete();

		if (!bRaisedCorpse && !g_Cfg.m_fNoResRobe)
		{
			CItem *pRobe = CItem::CreateBase(ITEMID_ROBE);
			ASSERT(pRobe);
			pRobe->SetName(g_Cfg.GetDefaultMsg(DEFMSG_SPELL_RES_ROBENAME));
			LayerAdd(pRobe, LAYER_ROBE);
		}

	}
	if (m_pNPC && m_pNPC->m_bonded)
	{
		m_Can &= ~CAN_C_GHOST;
		UpdateCanSee(new PacketBondedStatus(this, false));
	}
	UpdateMode();

	CSpellDef *pSpellDef = g_Cfg.GetSpellDef(SPELL_Resurrection);
	Effect(EFFECT_OBJ, pSpellDef->m_idEffect, this, 10, 16);
	Sound(pSpellDef->m_sound);
	return true;
}

void CChar::Spell_Effect_Remove(CItem * pSpell)
{
	ADDTOCALLSTACK("CChar::Spell_Effect_Remove");
	// we are removing the spell effect.
	ASSERT(pSpell);

	if ( !pSpell->IsTypeSpellable() )	// can this item have a spell effect ?
		return;
	if ( !pSpell->m_itSpell.m_spell )
		return;
	if ( pSpell->IsType(IT_WAND))	// equipped wands do not confer effect.
		return;

	CClient *pClient = GetClient();
	SPELL_TYPE spell = static_cast<SPELL_TYPE>(RES_GET_INDEX(pSpell->m_itSpell.m_spell));
	short iStatEffect = static_cast<short>(pSpell->m_itSpell.m_spelllevel);

	switch (GetSpellLayer(spell))	// spell effects that are common for the same layer fits here
	{
		case LAYER_NONE:
			break;
		case LAYER_FLAG_Poison:
			StatFlag_Clear(STATF_Poisoned);
			UpdateMode();
			if (pClient)
				pClient->removeBuff(BI_POISON);
			break;
		case LAYER_SPELL_Summon:
		{
			if (m_pPlayer)	// summoned players ? thats odd.
				return;
			if (!g_Serv.IsLoading())
				Effect(EFFECT_XYZ, ITEMID_FX_TELE_VANISH, this, 8, 20);
			if (!IsStatFlag(STATF_DEAD))	// fix for double @Destroy trigger
				Delete();
			return;
		}
		case LAYER_SPELL_Polymorph:
		{
			BUFF_ICONS iBuffIcon = BI_START;
			switch (spell)
			{
				case SPELL_Polymorph:
				case SPELL_BeastForm:
				case SPELL_Monster_Form:
					iBuffIcon = BI_POLYMORPH;
					break;
				case SPELL_Horrific_Beast:
					iBuffIcon = BI_HORRIFICBEAST;
					SetDefNum("RegenHitsVal", GetDefNum("RegenHitsVal", true) - pSpell->m_itSpell.m_spellcharges);
					break;
				case SPELL_Lich_Form:
					iBuffIcon = BI_LICHFORM;
					SetDefNum("RegenManaVal", GetDefNum("RegenManaVal", true) - pSpell->m_itSpell.m_PolyStr);
					SetDefNum("RegenHitsVal", GetDefNum("RegenHitsVal", true) + pSpell->m_itSpell.m_PolyDex);
					SetDefNum("ResFire", GetDefNum("ResFire", true) + pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResPoison", GetDefNum("ResPoison", true) - pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResCold", GetDefNum("ResCold", true) - pSpell->m_itSpell.m_spellcharges);
					break;
				case SPELL_Vampiric_Embrace:
					iBuffIcon = BI_VAMPIRICEMBRACE;
					SetDefNum("HitLeechLife", GetDefNum("HitLeechLife", true) - pSpell->m_itSpell.m_PolyStr);
					SetDefNum("RegenStamVal", GetDefNum("RegenStamVal", true) - pSpell->m_itSpell.m_PolyDex);
					SetDefNum("RegenManaVal", GetDefNum("RegenManaVal", true) - pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResFire", GetDefNum("ResFire", true) + pSpell->m_itSpell.m_spelllevel);
					break;
				case SPELL_Wraith_Form:
					iBuffIcon = BI_WRAITHFORM;
					SetDefNum("ResPhysical", GetDefNum("ResPhysical", true) - 15);
					SetDefNum("ResFire", GetDefNum("ResFire", true) + 5);
					SetDefNum("ResEnergy", GetDefNum("ResEnergy", true) + 5);
					break;
				case SPELL_Reaper_Form:
					iBuffIcon = BI_REAPERFORM;
					break;
				case SPELL_Stone_Form:
					iBuffIcon = BI_STONEFORM;
					break;
				default:
					break;
			}

			SetID(m_prev_id);
			if (IsSetMagicFlags(MAGICF_POLYMORPHSTATS))
			{
				Stat_AddMod(STAT_STR, -pSpell->m_itSpell.m_PolyStr);
				Stat_AddMod(STAT_DEX, -pSpell->m_itSpell.m_PolyDex);
				Stat_SetVal(STAT_STR, minimum(Stat_GetVal(STAT_STR), Stat_GetMax(STAT_STR)));
				Stat_SetVal(STAT_DEX, minimum(Stat_GetVal(STAT_DEX), Stat_GetMax(STAT_DEX)));
			}
			StatFlag_Clear(STATF_Polymorph);
			if (pClient)
				pClient->removeBuff(iBuffIcon);
			UpdateStatsFlag();
			return;
		}
		case LAYER_SPELL_Night_Sight:
		{
			StatFlag_Clear(STATF_NightSight);
			if (pClient)
			{
				pClient->addLight();
				pClient->removeBuff(BI_NIGHTSIGHT);
			}
			return;
		}

		case LAYER_SPELL_Incognito:
			StatFlag_Clear(STATF_Incognito);
			SetName(pSpell->GetName());	// restore your name
			pSpell->SetName("");	// clear the name from the item (might be a worn item)
			if (!IsStatFlag(STATF_Polymorph))
				SetHue(m_prev_Hue);
			if (pClient)
				pClient->removeBuff(BI_INCOGNITO);
			NotoSave_Update();
			break;

		case LAYER_SPELL_Invis:
			Reveal(STATF_Invisible);
			if (pClient)
				pClient->removeBuff(BI_INVISIBILITY);
			return;

		case LAYER_SPELL_Paralyze:
			StatFlag_Clear(STATF_Freeze);
			if (pClient)
			{
				pClient->removeBuff(BI_PARALYZE);
				pClient->addCharMove(this);		// immediately tell the client that now he's able to move (without this, it will be able to move only on next tick update)
			}
			break;

		case LAYER_SPELL_Strangle:	// TO-DO: NumBuff[0] and NumBuff[1] to hold the damage range values.
			if (pClient)
				pClient->removeBuff(BI_STRANGLE);
			break;

		case LAYER_SPELL_Gift_Of_Renewal:
			if (pClient)
				pClient->removeBuff(BI_GIFTOFRENEWAL);
			break;

		case LAYER_SPELL_Attunement:
			if (pClient)
				pClient->removeBuff(BI_ATTUNEWEAPON);
			break;

		case LAYER_SPELL_Thunderstorm:
			if (pClient)
				pClient->removeBuff(BI_THUNDERSTORM);
			break;

		case LAYER_SPELL_Essence_Of_Wind:
			if (pClient)
				pClient->removeBuff(BI_ESSENCEOFWIND);
			break;

		case LAYER_SPELL_Ethereal_Voyage:
			if (pClient)
				pClient->removeBuff(BI_ETHEREALVOYAGE);
			break;

		case LAYER_SPELL_Gift_Of_Life:
			if (pClient)
				pClient->removeBuff(BI_GIFTOFLIFE);
			break;

		case LAYER_SPELL_Arcane_Empowerment:
			if (pClient)
				pClient->removeBuff(BI_ARCANEEMPOWERMENT);
			break;

		/*case LAYER_Mortal_Strike:
			if (pClient)
				pClient->removeBuff(BI_MORTALSTRIKE);
			break;*/

		case LAYER_SPELL_Blood_Oath:
		{
			if (pClient)
				pClient->removeBuff(BI_BLOODOATHCURSE);
			CChar * pSrc = pSpell->m_uidLink.CharFind();
			if (pSrc && pSrc->IsClient())
				pSrc->GetClient()->removeBuff(BI_BLOODOATHCASTER);
			break;
		}
		case LAYER_SPELL_Corpse_Skin:
			SetDefNum("ResPhysical", GetDefNum("ResPhysical") - pSpell->m_itSpell.m_PolyStr, true);
			SetDefNum("ResFire", GetDefNum("ResFire") + pSpell->m_itSpell.m_PolyDex, true);
			SetDefNum("ResCold", GetDefNum("ResCold") - pSpell->m_itSpell.m_PolyStr, true);
			SetDefNum("ResPoison", GetDefNum("ResPoison") + pSpell->m_itSpell.m_PolyDex, true);
			if (pClient)
				pClient->removeBuff(BI_CORPSESKIN);
			break;

		case LAYER_SPELL_Pain_Spike:
			if (pClient)
				pClient->removeBuff(BI_PAINSPIKE);
			break;

		default:
			break;
	}

	switch (spell)	// the rest of the effects are handled directly by each spell
	{
		case SPELL_Clumsy:
			Stat_AddMod(STAT_DEX, iStatEffect);
			if (pClient)
				pClient->removeBuff(BI_CLUMSY);
			return;
		case SPELL_Particle_Form:	// 112 // turns you into an immobile, but untargetable particle system for a while.
		case SPELL_Stone:
			StatFlag_Clear( STATF_Stone );
			UpdateModeFlag();
			break;
		case SPELL_Hallucination:
			StatFlag_Clear( STATF_Hallucinating );
			if (pClient)
				pClient->addReSync();
			UpdateModeFlag();
		case SPELL_Feeblemind:
			Stat_AddMod( STAT_INT, iStatEffect );
			if (pClient)
				pClient->removeBuff(BI_FEEBLEMIND);
			return;
		case SPELL_Weaken:
			Stat_AddMod( STAT_STR, iStatEffect );
			if (pClient)
				pClient->removeBuff(BI_WEAKEN);
			return;
		case SPELL_Curse:
		{
			if ( m_pPlayer )
			{
				SetDefNum("RESFIREMAX", GetDefNum("RESFIREMAX", true, true) + 10);
				SetDefNum("RESCOLDMAX", GetDefNum("RESCOLDMAX", true, true) + 10);
				SetDefNum("RESPOISONMAX", GetDefNum("RESPOISONMAX", true, true) + 10);
				SetDefNum("RESENERGYMAX", GetDefNum("RESENERGYMAX", true, true) + 10);
			}
			for (int i = STAT_STR; i < STAT_BASE_QTY; i++ )
				Stat_AddMod(static_cast<STAT_TYPE>(i), iStatEffect);
			if (pClient)
				pClient->removeBuff(BI_CURSE);
			return;
		}
		case SPELL_Agility:
			Stat_AddMod( STAT_DEX, -iStatEffect );
			if (pClient)
				pClient->removeBuff(BI_AGILITY);
			return;
		case SPELL_Cunning:
			Stat_AddMod( STAT_INT, -iStatEffect );
			if (pClient)
				pClient->removeBuff(BI_CUNNING);
			return;
		case SPELL_Strength:
			Stat_AddMod( STAT_STR, -iStatEffect );
			if (pClient)
				pClient->removeBuff(BI_STRENGTH);
			return;
		case SPELL_Bless:
		{
			for ( int i = STAT_STR; i < STAT_BASE_QTY; i++ )
				Stat_AddMod(static_cast<STAT_TYPE>(i), -iStatEffect);
			if (pClient)
				pClient->removeBuff(BI_BLESS);
			return;
		}
		case SPELL_Mana_Drain:
			UpdateStatVal( STAT_INT, +iStatEffect );
			Effect( EFFECT_OBJ, ITEMID_FX_SPARKLE_2, this, 10, 25 );
			Sound( 0x28E );
			return;
		case SPELL_Reactive_Armor:
			if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
			{
				SetDefNum("RESPHYSICAL", GetDefNum("RESPHYSICAL", true) - pSpell->m_itSpell.m_spelllevel);
				SetDefNum("RESFIRE", GetDefNum("RESFIRE", true) + 5);
				SetDefNum("RESCOLD", GetDefNum("RESCOLD", true) + 5);
				SetDefNum("RESPOISON", GetDefNum("RESPOISON", true) + 5);
				SetDefNum("RESENERGY", GetDefNum("RESENERGY", true) + 5);
			}
			else
			{
				StatFlag_Clear(STATF_Reactive);
			}
			if (pClient)
				pClient->removeBuff(BI_REACTIVEARMOR);
			return;
		case SPELL_Magic_Reflect:
			StatFlag_Clear(STATF_Reflection);
			if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
			{
				SetDefNum("RESPHYSICAL", GetDefNum("RESPHYSICAL", true) + pSpell->m_itSpell.m_spelllevel);
				SetDefNum("RESFIRE", GetDefNum("RESFIRE", true) - 10);
				SetDefNum("RESCOLD", GetDefNum("RESCOLD", true) - 10);
				SetDefNum("RESPOISON", GetDefNum("RESPOISON", true) - 10);
				SetDefNum("RESENERGY", GetDefNum("RESENERGY", true) - 10);
			}
			if (pClient)
				pClient->removeBuff(BI_MAGICREFLECTION);
			return;
		case SPELL_Steelskin:		// 114 // turns your skin into steel, giving a boost to your AR.
		case SPELL_Stoneskin:		// 115 // turns your skin into stone, giving a boost to your AR.
		case SPELL_Protection:
		case SPELL_Arch_Prot:
			Sound(0x1ed);
			Effect(EFFECT_OBJ, ITEMID_FX_SPARKLE, this, 9, 20);
			if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
			{
				SetDefNum("RESPHYSICAL", GetDefNum("RESPHYSICAL", true) + pSpell->m_itSpell.m_PolyStr);
				SetDefNum("FASTERCASTING", GetDefNum("FASTERCASTING", true) + 2);
				Skill_SetBase(SKILL_MAGICRESISTANCE, minimum(Skill_GetMax(SKILL_MAGICRESISTANCE, true), Skill_GetBase(SKILL_MAGICRESISTANCE) + pSpell->m_itSpell.m_PolyDex));
			}
			else
			{
				m_defense = static_cast<WORD>(CalcArmorDefense());
			}
			if (pClient)
			{
				if (spell == SPELL_Protection)
					pClient->removeBuff(BI_PROTECTION);
				else if (spell == SPELL_Arch_Prot)
					pClient->removeBuff(BI_ARCHPROTECTION);
			}
			break;
		/*case SPELL_Chameleon:		// 106 // makes your skin match the colors of whatever is behind you.
			break;*/
		case SPELL_Trance:			// 111 // temporarily increases your meditation skill.
			Skill_SetBase(SKILL_MEDITATION, Skill_GetBase(SKILL_MEDITATION) - g_Cfg.GetSpellEffect(spell, iStatEffect));
			break;
		/*case SPELL_Shield:			// 113 // erects a temporary force field around you. Nobody approaching will be able to get within 1 tile of you, though you can move close to them if you wish.
			break;*/
		case SPELL_Mind_Rot:
			SetDefNum("LowerManaCost", GetDefNum("LowerManaCost") + pSpell->m_itSpell.m_spelllevel, true);
			if (pClient)
				pClient->removeBuff(BI_MINDROT);
			break;
		case SPELL_Curse_Weapon:
		{
			CItem * pWeapon = m_uidWeapon.ItemFind();
			if (!pWeapon)
				break;
			pWeapon->SetDefNum("HitLeechLife", pWeapon->GetDefNum("HitLeechLife", true) - pSpell->m_itSpell.m_spelllevel, true);	// Adding 50% HitLeechLife to the weapon, since damaging with it should return 50% of the damage dealt.
		}break;
		default:
			return;
	}
	UpdateStatsFlag();
}


// Attach the spell effect for a duration.
// Add effects which are saved in the save file here.
// Not in LayerAdd
void CChar::Spell_Effect_Add( CItem * pSpell )
{
	ADDTOCALLSTACK("CChar::Spell_Effect_Add");
	// NOTE: ATTR_MAGIC without ATTR_MOVE_NEVER is dispellable !
	// equipped wands do not confer effect.
	if ( !pSpell || !pSpell->IsTypeSpellable() || pSpell->IsType(IT_WAND) )
		return;

	CClient *pClient = GetClient();
	CChar *pCaster = pSpell->m_uidLink.CharFind();
	SPELL_TYPE spell = static_cast<SPELL_TYPE>(RES_GET_INDEX(pSpell->m_itSpell.m_spell));
	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);

	if ( !pSpellDef || !spell )
		return;

	short iStatEffect = static_cast<short>(pSpell->m_itSpell.m_spelllevel);
	short iTimerEffect = static_cast<short>(pSpell->GetTimerAdjusted());

	if (IsTrigUsed(TRIGGER_EFFECTADD))
	{
		CScriptTriggerArgs Args;
		Args.m_pO1 = pSpell;
		Args.m_iN1 = spell;
		TRIGRET_TYPE iRet = OnTrigger(CTRIG_EffectAdd, this, &Args);
		if (iRet == TRIGRET_RET_TRUE)	// Return 1: We don't want nothing to happen, removing memory also.
		{
			pSpell->Delete(true);
			return;
		}
		else if (iRet == static_cast<TRIGRET_TYPE>(0))	// return 0: we want the memory to be equipped but we want custom things to happen: don't remove memory but stop here,
			return;
	}
	//Buffs related variables:
	TCHAR NumBuff[7][8];
	LPCTSTR pNumBuff[7] = { NumBuff[0], NumBuff[1], NumBuff[2], NumBuff[3], NumBuff[4], NumBuff[5], NumBuff[6] };
	//------------------------

	switch (GetSpellLayer(spell))
	{
		case LAYER_NONE:
			break;
		case LAYER_SPELL_Polymorph:
		{
			BUFF_ICONS iBuffIcon = BI_START;
			switch (spell)
			{
				case SPELL_BeastForm:		// 107 // polymorphs you into an animal for a while.
				case SPELL_Monster_Form:	// 108 // polymorphs you into a monster for a while.
				case SPELL_Polymorph:
					iBuffIcon = BI_POLYMORPH;
					break;
				case SPELL_Lich_Form:
					pSpell->m_itSpell.m_PolyStr = 13;		// +RegenManaVal
					pSpell->m_itSpell.m_PolyDex = 5;			// -RegenHitsVal
					pSpell->m_itSpell.m_spellcharges = 10;	// -ResFire, +ResPoison, +ResCold
					m_atMagery.m_SummonID = CREID_LICH;
					SetDefNum("RegenManaVal", GetDefNum("RegenManaVal", true) + pSpell->m_itSpell.m_PolyStr);	// RegenManaVal
					SetDefNum("RegenHitsVal", GetDefNum("RegenHitsVal", true) - pSpell->m_itSpell.m_PolyDex);	// RegenHitsVal
					SetDefNum("ResFire", GetDefNum("ResFire", true) - pSpell->m_itSpell.m_spellcharges);		// ResFire, ResPoison, ResCold
					SetDefNum("ResPoison", GetDefNum("ResPoison", true) + pSpell->m_itSpell.m_spellcharges);
					SetDefNum("ResCold", GetDefNum("ResCold", true) + pSpell->m_itSpell.m_spellcharges);
					iBuffIcon = BI_LICHFORM;
					break;
				case SPELL_Wraith_Form:
					pSpell->m_itSpell.m_PolyDex = 15;
					pSpell->m_itSpell.m_PolyStr = 5;
					m_atMagery.m_SummonID = CREID_SPECTRE;
					iBuffIcon = BI_WRAITHFORM;
					SetDefNum("ResPhysical", GetDefNum("ResPhysical", true) + 15);
					SetDefNum("ResFire", GetDefNum("ResFire", true) - 5);
					SetDefNum("ResEnergy", GetDefNum("ResEnergy", true) - 5);
					break;
				case SPELL_Horrific_Beast:
					pSpell->m_itSpell.m_PolyStr = 5;			// UnArmed DamLo
					pSpell->m_itSpell.m_PolyDex = 9;			// UnArmed DamHi
					pSpell->m_itSpell.m_spelllevel = 25;		// Melee Damage Increase
					pSpell->m_itSpell.m_spellcharges = 20;	// RegenHitsVal
					m_atMagery.m_SummonID = CREID_Horrific_Beast;
					SetDefNum("RegenHitsVal", GetDefNum("RegenHitsVal", true) + pSpell->m_itSpell.m_spellcharges);
					iBuffIcon = BI_HORRIFICBEAST;
					break;
				case SPELL_Vampiric_Embrace:
					pSpell->m_itSpell.m_PolyStr = 13;		// +Hit Leech Life
					pSpell->m_itSpell.m_PolyDex = 5;			// +RegenStamVal
					pSpell->m_itSpell.m_spellcharges = 3;	// +RegenManaVal
					pSpell->m_itSpell.m_spelllevel = 25;		// -ResFire
					m_atMagery.m_SummonID = CREID_Vampire_Bat;
					SetDefNum("HitLeechLife", GetDefNum("HitLeechLife", true) + pSpell->m_itSpell.m_PolyStr);		// +Hit Leech Life
					SetDefNum("RegenStamVal", GetDefNum("RegenStamVal", true) + pSpell->m_itSpell.m_PolyDex);		// +RegenStamVal
					SetDefNum("RegenManaVal", GetDefNum("RegenManaVal", true) + pSpell->m_itSpell.m_spellcharges);	// RegenManaVal
					SetDefNum("ResFire", GetDefNum("ResFire", true) - pSpell->m_itSpell.m_spelllevel);				// ResFire
					iBuffIcon = BI_VAMPIRICEMBRACE;
					break;
				case SPELL_Stone_Form:
					m_atMagery.m_SummonID = CREID_Stone_Form;
					iBuffIcon = BI_STONEFORM;
					break;
				case SPELL_Reaper_Form:
					m_atMagery.m_SummonID = CREID_Stone_Form;
					iBuffIcon = BI_REAPERFORM;
					break;
				default:
					break;
			}

			int SPELL_MAX_POLY_STAT = g_Cfg.m_iMaxPolyStats;
			SetID(m_atMagery.m_SummonID);

			CCharBase * pCharDef = Char_GetDef();
			ASSERT(pCharDef);

			// re-apply our incognito name
			if (IsStatFlag(STATF_Incognito))
				SetName(pCharDef->GetTypeName());

			// set to creature type stats
			if (IsSetMagicFlags(MAGICF_POLYMORPHSTATS))
			{
				if (pCharDef->m_Str)
				{
					int iChange = pCharDef->m_Str - Stat_GetBase(STAT_STR);
					if (iChange > SPELL_MAX_POLY_STAT)
						iChange = SPELL_MAX_POLY_STAT;
					if (iChange + Stat_GetBase(STAT_STR) < 0)
						iChange = -Stat_GetBase(STAT_STR);
					Stat_AddMod(STAT_STR, static_cast<short>(iChange));
					pSpell->m_itSpell.m_PolyStr = static_cast<short>(iChange);
				}
				else
				{
					pSpell->m_itSpell.m_PolyStr = 0;
				}
				if (pCharDef->m_Dex)
				{
					int iChange = pCharDef->m_Dex - Stat_GetBase(STAT_DEX);
					if (iChange > SPELL_MAX_POLY_STAT)
						iChange = SPELL_MAX_POLY_STAT;
					if (iChange + Stat_GetBase(STAT_DEX) < 0)
						iChange = -Stat_GetBase(STAT_DEX);
					Stat_AddMod(STAT_DEX, static_cast<short>(iChange));
					pSpell->m_itSpell.m_PolyDex = static_cast<short>(iChange);
				}
				else
				{
					pSpell->m_itSpell.m_PolyDex = 0;
				}
			}
			Update();		// show everyone I am now a new type

			StatFlag_Set(STATF_Polymorph);
			if (pClient && IsSetOF(OF_Buffs) && iBuffIcon)
			{
				pClient->removeBuff(iBuffIcon);
				pClient->addBuff(iBuffIcon, 1075824, 1070722, iTimerEffect);
			}
			return;
		}
		case LAYER_FLAG_Poison:
			StatFlag_Set(STATF_Poisoned);
			UpdateMode();
			if (pClient && IsSetOF(OF_Buffs))
			{
				pClient->removeBuff(BI_POISON);
				pClient->addBuff(BI_POISON, 1017383, 1070722, 2);
			}
			break;
		case LAYER_SPELL_Night_Sight:
			StatFlag_Set(STATF_NightSight);
			if (pClient)
			{
				pClient->addLight();
				if (IsSetOF(OF_Buffs))
				{
					pClient->removeBuff(BI_NIGHTSIGHT);
					pClient->addBuff(BI_NIGHTSIGHT, 1075643, 1075644, iTimerEffect);
				}
			}
			break;
		case LAYER_SPELL_Incognito:
			if (!IsStatFlag(STATF_Incognito))
			{
				const CCharBase * pCharDef = Char_GetDef();
				ASSERT(pCharDef);
				StatFlag_Set(STATF_Incognito);
				pSpell->SetName(GetName());	// Give it my name
				SetName(pCharDef->GetTypeName());	// Give me general name for the type
				if (!IsStatFlag(STATF_Polymorph) && IsPlayableCharacter())
					SetHue((HUE_UNDERWEAR | HUE_SKIN_LOW) + static_cast<HUE_TYPE>(Calc_GetRandVal(HUE_SKIN_HIGH - HUE_SKIN_LOW)));

				if (pClient && IsSetOF(OF_Buffs))
				{
					pClient->removeBuff(BI_INCOGNITO);
					pClient->addBuff(BI_INCOGNITO, 1075819, 1075820, iTimerEffect);
				}
			}
			break;
		case LAYER_SPELL_Invis:
			StatFlag_Set(STATF_Invisible);
			if (pClient)
			{
				new PacketCharacter(pClient, this);	//updates paperdoll with less packets then pClient->addChar(this)
				if (IsSetOF(OF_Buffs))
				{
					pClient->removeBuff(BI_INVISIBILITY);
					pClient->addBuff(BI_INVISIBILITY, 1075825, 1075826, iTimerEffect);
				}
			}
			UpdateMove(GetTopPoint());	// some will be seeing us for the first time!
			break;
		case LAYER_SPELL_Paralyze:
			StatFlag_Set(STATF_Freeze);
			if (pClient && IsSetOF(OF_Buffs))
			{
				pClient->removeBuff(BI_PARALYZE);
				pClient->addBuff(BI_PARALYZE, 1075827, 1075828, iTimerEffect);
			}
			break;
		case LAYER_SPELL_Summon:
			StatFlag_Set(STATF_Conjured);
			break;
		case LAYER_SPELL_Strangle:	// TO-DO: NumBuff[0] and NumBuff[1] to hold the damage range values.
			{
				if (pClient && IsSetOF(OF_Buffs))
				{
					pClient->removeBuff(BI_STRANGLE);
					pClient->addBuff(BI_STRANGLE, 1075794, 1075795, iTimerEffect);
				}
				iStatEffect = (pCaster->Skill_GetBase(SKILL_SPIRITSPEAK) / 100);
				if (iStatEffect < 4)
					iStatEffect = 4;
				pSpell->m_itSpell.m_spelllevel = iStatEffect;
				pSpell->m_itSpell.m_spellcharges = iStatEffect;
			}	break;
		case LAYER_SPELL_Gift_Of_Renewal:
			if (pClient && IsSetOF(OF_Buffs))
			{
				ITOA(pSpell->m_itSpell.m_spelllevel,NumBuff[0], 10);
				pClient->removeBuff(BI_GIFTOFRENEWAL);
				pClient->addBuff(BI_GIFTOFRENEWAL, 1075796, 1075797, iTimerEffect, pNumBuff, 1);
			}
			break;
		case LAYER_SPELL_Attunement:
			if (pClient && IsSetOF(OF_Buffs))
			{
				ITOA(pSpell->m_itSpell.m_spelllevel, NumBuff[0], 10);
				pClient->removeBuff(BI_ATTUNEWEAPON);
				pClient->addBuff(BI_ATTUNEWEAPON, 1075798, 1075799, iTimerEffect, pNumBuff, 1);
			}
			break;
		case LAYER_SPELL_Thunderstorm:
			if (pClient && IsSetOF(OF_Buffs))
			{
				ITOA(pSpell->m_itSpell.m_spelllevel, NumBuff[0], 10);
				pClient->removeBuff(BI_THUNDERSTORM);
				pClient->addBuff(BI_THUNDERSTORM, 1075800, 1075801, iTimerEffect, pNumBuff, 1);
			}
			break;
		case LAYER_SPELL_Essence_Of_Wind:
			if (pClient && IsSetOF(OF_Buffs))
			{
				ITOA(pSpell->m_itSpell.m_spelllevel, NumBuff[0], 10);
				pClient->removeBuff(BI_ESSENCEOFWIND);
				pClient->addBuff(BI_ESSENCEOFWIND, 1075802, 1075803, iTimerEffect, pNumBuff, 1);
			}
			break;
		case LAYER_SPELL_Ethereal_Voyage:
			if (pClient && IsSetOF(OF_Buffs))
			{
				pClient->removeBuff(BI_ETHEREALVOYAGE);
				pClient->addBuff(BI_ETHEREALVOYAGE, 1075804, 1075805, iTimerEffect);
			}
			break;
		case LAYER_SPELL_Gift_Of_Life:
			if (pClient && IsSetOF(OF_Buffs))
			{
				pClient->removeBuff(BI_GIFTOFLIFE);
				pClient->addBuff(BI_GIFTOFLIFE, 1075806, 1075807, iTimerEffect);
			}
			break;
		case LAYER_SPELL_Arcane_Empowerment:
			if (pClient && IsSetOF(OF_Buffs))
			{
				ITOA(pSpell->m_itSpell.m_spelllevel, NumBuff[0], 10);
				ITOA(pSpell->m_itSpell.m_spellcharges,NumBuff[1], 10);
				pClient->removeBuff(BI_ARCANEEMPOWERMENT);
				pClient->addBuff(BI_ARCANEEMPOWERMENT, 1075805, 1075804, iTimerEffect, pNumBuff, 1);
			}
			break;
		/*case LAYER_Mortal_Strike:
			if (pClient && IsSetOF(OF_Buffs))
			{
				pClient->removeBuff(BI_MORTALSTRIKE);
				pClient->addBuff(BI_MORTALSTRIKE, 1075810, 1075811, iTimerEffect);
			}
			break;*/
		case LAYER_SPELL_Pain_Spike:
			{
				CItem * pPrevious = LayerFind(LAYER_SPELL_Pain_Spike);
				if (pPrevious)
				{
					CItem * pPrevious = LayerFind(LAYER_SPELL_Pain_Spike);
					if (pPrevious)
						pSpell->m_itSpell.m_spellcharges += 2;
						//TO-DO If the spell targets someone already affected by the Pain Spike spell, only 3 to 7 points of DIRECT damage will be inflicted.
				}
				if (m_pNPC)
					iStatEffect = ((pCaster->Skill_GetBase(SKILL_SPIRITSPEAK) - Skill_GetBase(SKILL_MAGICRESISTANCE)) / 10) + 30;
				else
					iStatEffect = ((pCaster->Skill_GetBase(SKILL_SPIRITSPEAK) - Skill_GetBase(SKILL_MAGICRESISTANCE)) / 100) + 18;
				pSpell->m_itSpell.m_spellcharges = 10;
				pSpell->m_itSpell.m_spelllevel = iStatEffect;
			}
			break;
		case LAYER_SPELL_Blood_Oath:
			iStatEffect = ((Skill_GetBase(SKILL_MAGICRESISTANCE) * 10) / 20) + 10;	// bonus of reflection
			pSpell->m_itSpell.m_spelllevel = iStatEffect;
			if (IsSetOF(OF_Buffs))
			{
				if (pClient)
				{
					strcpy(NumBuff[0], pCaster->GetName());
					strcpy(NumBuff[1], pCaster->GetName());
					pClient->removeBuff(BI_BLOODOATHCURSE);
					pClient->addBuff(BI_BLOODOATHCURSE, 1075659, 1075660, iTimerEffect, pNumBuff, 2);
				}
				CClient *pCasterClient = pCaster->GetClient();
				if (pCasterClient)
				{
					strcpy(NumBuff[0], GetName());
					pCasterClient->removeBuff(BI_BLOODOATHCASTER);
					pCasterClient->addBuff(BI_BLOODOATHCASTER, 1075661, 1075662, iTimerEffect, pNumBuff, 1);
				}
			}
			break;
		case LAYER_SPELL_Corpse_Skin:
			if (pClient && IsSetOF(OF_Buffs))
			{
				pClient->removeBuff(BI_CORPSESKIN);
				pClient->addBuff(BI_CORPSESKIN, 1075805, 1075804, iTimerEffect, pNumBuff, 1);
			}
			pSpell->m_itSpell.m_PolyDex = 15;
			pSpell->m_itSpell.m_PolyStr = 10;
			SetDefNum("ResFire", GetDefNum("ResFire") - pSpell->m_itSpell.m_PolyDex, true);
			SetDefNum("ResPoison", GetDefNum("ResPoison") - pSpell->m_itSpell.m_PolyDex, true);
			SetDefNum("ResCold", GetDefNum("ResCold") + pSpell->m_itSpell.m_PolyStr, true);
			SetDefNum("ResPhysical", GetDefNum("ResPhysical") + pSpell->m_itSpell.m_PolyStr, true);
			break;
		case LAYER_SPELL_Mind_Rot:
			iStatEffect = 10;	// -10% LOWERMANACOST
			pSpell->m_itSpell.m_spelllevel = iStatEffect;
			SetDefNum("LowerManaCost", GetDefNum("LowerManaCost")-iStatEffect , true);
			break;
		case LAYER_SPELL_Curse_Weapon:
			{
				CItem * pWeapon = m_uidWeapon.ItemFind();
				if (!pWeapon)
				{
					pSpell->Delete(true);
					return;
				}
				iStatEffect = 50;	// +50% HitLeechLife
				pSpell->m_itSpell.m_spelllevel = iStatEffect;
				pWeapon->SetDefNum("HitLeechLife", pWeapon->GetDefNum("HitLeechLife", true) + pSpell->m_itSpell.m_spelllevel, true);	// Adding 50% HitLeechLife to the weapon, since damaging with it should return 50% of the damage dealt.
			}
			break;
		default:
			break;
	}

	switch ( spell )
	{
		case SPELL_Reactive_Armor:
			if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
			{
				iStatEffect = 15 + (pCaster->Skill_GetBase(SKILL_INSCRIPTION) / 200);
				pSpell->m_itSpell.m_spelllevel = iStatEffect;

				SetDefNum("RESPHYSICAL", GetDefNum("RESPHYSICAL", true) + iStatEffect);
				SetDefNum("RESFIRE", GetDefNum("RESFIRE", true) - 5);
				SetDefNum("RESCOLD", GetDefNum("RESCOLD", true) - 5);
				SetDefNum("RESPOISON", GetDefNum("RESPOISON", true) - 5);
				SetDefNum("RESENERGY", GetDefNum("RESENERGY", true) - 5);
			}
			else
			{
				StatFlag_Set( STATF_Reactive );
			}
			if (pClient && IsSetOF(OF_Buffs))
			{
				pClient->removeBuff(BI_REACTIVEARMOR);
				if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
				{
					ITOA(iStatEffect, NumBuff[0], 10);
					for ( int idx = 1; idx < 5; ++idx )
						ITOA(5, NumBuff[idx], 10);

					pClient->addBuff(BI_REACTIVEARMOR, 1075812, 1075813, iTimerEffect, pNumBuff, 5);
				}
				else
				{
					pClient->addBuff(BI_REACTIVEARMOR, 1075812, 1070722, iTimerEffect);
				}
			}
			break;
		case SPELL_Clumsy:
			{
				if ( pCaster != NULL && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
				{
					iStatEffect = 8 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100) - (Skill_GetBase(SKILL_MAGICRESISTANCE) / 100);
					pSpell->m_itSpell.m_spelllevel = iStatEffect;
				}
				Stat_AddMod( STAT_DEX, -iStatEffect );
				if (pClient && IsSetOF(OF_Buffs))
				{
					ITOA(iStatEffect, NumBuff[0], 10);
					pClient->removeBuff(BI_CLUMSY);
					pClient->addBuff(BI_CLUMSY, 1075831, 1075832, iTimerEffect, pNumBuff, 1);
				}
			}
			break;
		case SPELL_Particle_Form:	// 112 // turns you into an immobile, but untargetable particle system for a while.
		case SPELL_Stone:
			StatFlag_Set( STATF_Stone );
			UpdateModeFlag();
			break;
		case SPELL_Hallucination:
			StatFlag_Set( STATF_Hallucinating );
			UpdateModeFlag();
		case SPELL_Feeblemind:
			{
				if ( pCaster != NULL && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
				{
					iStatEffect = 8 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100) - (Skill_GetBase(SKILL_MAGICRESISTANCE) / 100);
					pSpell->m_itSpell.m_spelllevel = iStatEffect;
				}
				Stat_AddMod( STAT_INT, -iStatEffect );
				if (pClient && IsSetOF(OF_Buffs))
				{
					ITOA(iStatEffect, NumBuff[0], 10);
					pClient->removeBuff(BI_FEEBLEMIND);
					pClient->addBuff(BI_FEEBLEMIND, 1075833, 1075834, iTimerEffect, pNumBuff, 1);
				}
			} break;
		case SPELL_Weaken:
			{
				if ( pCaster != NULL && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
				{
					iStatEffect = 8 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100) - (Skill_GetBase(SKILL_MAGICRESISTANCE) / 100);
					pSpell->m_itSpell.m_spelllevel = iStatEffect;
				}
				Stat_AddMod( STAT_STR, -iStatEffect );
				if (pClient && IsSetOF(OF_Buffs))
				{
					ITOA(iStatEffect, NumBuff[0], 10);
					pClient->removeBuff(BI_WEAKEN);
					pClient->addBuff(BI_WEAKEN, 1075837, 1075838, iTimerEffect, pNumBuff, 1);
				}
			} break;
		case SPELL_Curse:
			{
				if ( pCaster != NULL && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
				{
					iStatEffect = 8 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100) - (Skill_GetBase(SKILL_MAGICRESISTANCE) / 100);
					pSpell->m_itSpell.m_spelllevel = iStatEffect;
				}
				if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) && m_pPlayer )		// Curse also decrease max resistances on players
				{
					SetDefNum("RESFIREMAX", GetDefNum("RESFIREMAX", true) - 10);
					SetDefNum("RESCOLDMAX", GetDefNum("RESCOLDMAX", true) - 10);
					SetDefNum("RESPOISONMAX", GetDefNum("RESPOISONMAX", true) - 10);
					SetDefNum("RESENERGYMAX", GetDefNum("RESENERGYMAX", true) - 10);
				}
				for ( int i = STAT_STR; i < STAT_BASE_QTY; i++ )
					Stat_AddMod(static_cast<STAT_TYPE>(i), -iStatEffect);

				if (pClient && IsSetOF(OF_Buffs))
				{
					pClient->removeBuff(BI_CURSE);
					for ( int idx = STAT_STR; idx < STAT_BASE_QTY; ++idx )
						ITOA(iStatEffect, NumBuff[idx], 10);
					if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
					{
						for ( int idx = 3; idx < 7; ++idx )
							ITOA(10, NumBuff[idx], 10);

						pClient->addBuff(BI_CURSE, 1075835, 1075836, iTimerEffect, pNumBuff, 7);
					}
					else
					{
						pClient->addBuff(BI_CURSE, 1075835, 1075840, iTimerEffect, pNumBuff, 3);
					}
				}
			} break;
		case SPELL_Agility:
			{
				if ( pCaster != NULL && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
				{
					iStatEffect = 1 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100);
					pSpell->m_itSpell.m_spelllevel = iStatEffect;
				}
				Stat_AddMod( STAT_DEX, +iStatEffect );
				if (pClient && IsSetOF(OF_Buffs))
				{
					ITOA(iStatEffect, NumBuff[0], 10);
					pClient->removeBuff(BI_AGILITY);
					pClient->addBuff(BI_AGILITY, 1075841, 1075842, iTimerEffect, pNumBuff, 1);
				}
			} break;
		case SPELL_Cunning:
			{
				if ( pCaster != NULL && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
				{
					iStatEffect = 1 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100);
					pSpell->m_itSpell.m_spelllevel = iStatEffect;
				}
				Stat_AddMod( STAT_INT, +iStatEffect );
				if (pClient && IsSetOF(OF_Buffs))
				{
					ITOA(iStatEffect, NumBuff[0], 10);
					pClient->removeBuff(BI_CUNNING);
					pClient->addBuff(BI_CUNNING, 1075843, 1075844, iTimerEffect, pNumBuff, 1);
				}
			} break;
		case SPELL_Strength:
			{
				if ( pCaster != NULL && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
				{
					iStatEffect = 1 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100);
					pSpell->m_itSpell.m_spelllevel = iStatEffect;
				}
				Stat_AddMod( STAT_STR, +iStatEffect );
				if (pClient && IsSetOF(OF_Buffs))
				{
					ITOA(iStatEffect, NumBuff[0], 10);
					pClient->removeBuff(BI_STRENGTH);
					pClient->addBuff(BI_STRENGTH, 1075845, 1075846, iTimerEffect, pNumBuff, 1);
				}
			} break;
		case SPELL_Bless:
			{
				if ( pCaster != NULL && IsSetMagicFlags(MAGICF_OSIFORMULAS) )
				{
					iStatEffect = 1 + (pCaster->Skill_GetBase(SKILL_EVALINT) / 100);
					pSpell->m_itSpell.m_spelllevel = iStatEffect;
				}
				for ( int i = STAT_STR; i < STAT_BASE_QTY; i++ )
					Stat_AddMod(static_cast<STAT_TYPE>(i), iStatEffect);

				if (pClient && IsSetOF(OF_Buffs))
				{
					for ( int idx = STAT_STR; idx < STAT_BASE_QTY; ++idx)
						ITOA(iStatEffect, NumBuff[idx], 10);

					pClient->removeBuff(BI_BLESS);
					pClient->addBuff(BI_BLESS, 1075847, 1075848, iTimerEffect, pNumBuff, STAT_BASE_QTY);
				}
			} break;
		case SPELL_Mana_Drain:
			{
				if ( pCaster != NULL )
				{
					iStatEffect = (400 + pCaster->Skill_GetBase(SKILL_EVALINT) - Skill_GetBase(SKILL_MAGICRESISTANCE)) / 10;
					if ( iStatEffect < 0 )
						iStatEffect = 0;
					else if ( iStatEffect > Stat_GetVal(STAT_INT) )
						iStatEffect = static_cast<short>(Stat_GetVal(STAT_INT));

					pSpell->m_itSpell.m_spelllevel = iStatEffect;
				}
				UpdateStatVal( STAT_INT, -iStatEffect );
			} break;
		
		case SPELL_Magic_Reflect:
			StatFlag_Set( STATF_Reflection );
			if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
			{
				iStatEffect = 25 - (pCaster->Skill_GetBase(SKILL_INSCRIPTION) / 200);
				pSpell->m_itSpell.m_spelllevel = iStatEffect;

				SetDefNum("RESPHYSICAL", GetDefNum("RESPHYSICAL", true) - iStatEffect);
				SetDefNum("RESFIRE", GetDefNum("RESFIRE", true) + 10);
				SetDefNum("RESCOLD", GetDefNum("RESCOLD", true) + 10);
				SetDefNum("RESPOISON", GetDefNum("RESPOISON", true) + 10);
				SetDefNum("RESENERGY", GetDefNum("RESENERGY", true) + 10);
			}
			if (pClient && IsSetOF(OF_Buffs))
			{
				pClient->removeBuff(BI_MAGICREFLECTION);
				if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
				{
					ITOA(-iStatEffect, NumBuff[0], 10);
					for ( int idx = 1; idx < 5; ++idx )
						ITOA(10, NumBuff[idx], 10);

					pClient->addBuff(BI_MAGICREFLECTION, 1075817, 1075818, iTimerEffect, pNumBuff, 5);
				}
				else
				{
					pClient->addBuff(BI_MAGICREFLECTION, 1075817, 1070722, iTimerEffect);
				}
			}
			break;
		case SPELL_Steelskin:		// 114 // turns your skin into steel, giving a boost to your AR.
		case SPELL_Stoneskin:		// 115 // turns your skin into stone, giving a boost to your AR.
		case SPELL_Protection:
		case SPELL_Arch_Prot:
			{
				int iPhysicalResist = 0;
				int iMagicResist = 0;
				if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
				{
					iStatEffect = minimum(75, (pCaster->Skill_GetBase(SKILL_EVALINT) + pCaster->Skill_GetBase(SKILL_MEDITATION) + pCaster->Skill_GetBase(SKILL_INSCRIPTION)) / 40);
					iPhysicalResist = 15 - (pCaster->Skill_GetBase(SKILL_INSCRIPTION) / 200);
					iMagicResist = minimum(Skill_GetBase(SKILL_MAGICRESISTANCE), 350 - (Skill_GetBase(SKILL_INSCRIPTION) / 20));

					pSpell->m_itSpell.m_spelllevel = iStatEffect;
					pSpell->m_itSpell.m_PolyStr = static_cast<short>(iPhysicalResist);
					pSpell->m_itSpell.m_PolyDex = static_cast<short>(iMagicResist);

					SetDefNum("RESPHYSICAL", GetDefNum("RESPHYSICAL", true) - iPhysicalResist);
					SetDefNum("FASTERCASTING", GetDefNum("FASTERCASTING", true) - 2);
					Skill_SetBase(SKILL_MAGICRESISTANCE, Skill_GetBase(SKILL_MAGICRESISTANCE) - iMagicResist);
				}
				else
				{
					m_defense = static_cast<WORD>(CalcArmorDefense());
				}
				if (pClient && IsSetOF(OF_Buffs))
				{
					BUFF_ICONS BuffIcon = BI_PROTECTION;
					unsigned long BuffCliloc = 1075814;
					if ( spell == SPELL_Arch_Prot )
					{
						BuffIcon = BI_ARCHPROTECTION;
						BuffCliloc = 1075816;
					}

					pClient->removeBuff(BuffIcon);
					if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
					{
						ITOA(-iPhysicalResist, NumBuff[0], 10);
						ITOA(-iMagicResist/10, NumBuff[1], 10);
						pClient->addBuff(BuffIcon, BuffCliloc, 1075815, iTimerEffect, pNumBuff, 2);
					}
					else
					{
						pClient->addBuff(BuffIcon, BuffCliloc, 1070722, iTimerEffect);
					}
				}
			}
			break; 
		

		case SPELL_Chameleon:		// 106 // makes your skin match the colors of whatever is behind you.
		case SPELL_BeastForm:		// 107 // polymorphs you into an animal for a while.
		case SPELL_Monster_Form:	// 108 // polymorphs you into a monster for a while.
			break;

		case SPELL_Trance:			// 111 // temporarily increases your meditation skill.
			Skill_SetBase( SKILL_MEDITATION, Skill_GetBase( SKILL_MEDITATION ) + iStatEffect );
			break;

		case SPELL_Shield:			// 113 // erects a temporary force field around you. Nobody approaching will be able to get within 1 tile of you, though you can move close to them if you wish.
			break;

		default:
			return;
	}

	UpdateStatsFlag();
}

bool CChar::Spell_Equip_OnTick( CItem * pItem )
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
			if ( iLevel <= 0 )
				return false;

			// Change to get sober
			if ( 10 > Calc_GetRandVal(100))
				return false;

			Stat_SetVal( STAT_INT, maximum(0, Stat_GetVal(STAT_INT)-1) );
			Stat_SetVal( STAT_DEX, maximum(0, Stat_GetVal(STAT_DEX)-1) );

			if ( !Calc_GetRandVal(3))
			{
				Speak( g_Cfg.GetDefaultMsg( DEFMSG_SPELL_ALCOHOL_HIC ) );
				if ( !IsStatFlag(STATF_OnHorse))
				{
					DIR_TYPE dir = static_cast<DIR_TYPE>(Calc_GetRandVal(8));
					UpdateDir( dir );
					UpdateAnimate( ANIM_BOW );
				}
			}

			// We will have this effect again
			//pItem->m_itSpell.m_spelllevel -= 10;
			pItem->SetTimeout( 5*TICK_PER_SEC );
		}
		break;

		case SPELL_Regenerate:
		{
			if (iCharges <= 0 || iLevel <= 0)
			{
				return(false);
			}

			// Gain HP.
			UpdateStatVal(STAT_STR, g_Cfg.GetSpellEffect(spell, iLevel));
			pItem->SetTimeout(2 * TICK_PER_SEC);
		}	break;

		case SPELL_Hallucination:
		{
			if (iCharges <= 0 || iLevel <= 0)
			{
				return(false);
			}
			if (IsClient())
			{
				static const SOUND_TYPE sm_sounds[] = { 0x243, 0x244, 0x245 };
				m_pClient->addSound(sm_sounds[Calc_GetRandVal(COUNTOF(sm_sounds))]);
				m_pClient->addReSync();
			}
			// save the effect.
			pItem->SetTimeout((15 + Calc_GetRandLLVal(15))*TICK_PER_SEC);
		}
		break;

		case SPELL_Poison:
		{
			// Both potions and poison spells use this.
			// m_itSpell.m_spelllevel = strength of the poison ! 0-1000

			static const int sm_iPoisonMax[] = { 2, 4, 6, 8 };

			if (iCharges <= 0)
				return(false);

			int iDmg = 0;
			// The poison in your body is having an effect.
			if (IsSetMagicFlags(MAGICF_OSIFORMULAS))
			{
				//iLevel = pItem->m_itSpell.m_spelllevel;	//Osi Formulas store directly the strenght in more2
				iDmg = pItem->m_itSpell.m_pattern;
				switch (iLevel)
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
						pItem->SetTimeout(20);
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
				Emote2(sm_Poison_Message[iLevel], sm_Poison_Message_Other[iLevel], GetClient());
				SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_SPELL_YOUFEEL), static_cast<LPCTSTR>(sm_Poison_Message[iLevel]));
			}
			else
			{
				if (iLevel < 50)
					return false;
				if (iLevel < 200)	// Lesser
					iLevel = 0;
				else if (iLevel < 400) // Normal
					iLevel = 1;
				else if (iLevel < 800) // Greater
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

				TCHAR * pszMsg = Str_GetTemp();
				sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_SPELL_LOOKS), static_cast<LPCTSTR>(sm_Poison_Message[iLevel]));
				Emote(pszMsg, GetClient());
				SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_SPELL_YOUFEEL), static_cast<LPCTSTR>(sm_Poison_Message[iLevel]));
			}

			OnTakeDamage(maximum(sm_iPoisonMax[iLevel], iDmg), pItem->m_uidLink.CharFind(), DAMAGE_MAGIC | DAMAGE_POISON | DAMAGE_NOREVEAL, 0, 0, 0, 100, 0);

			// g_Cfg.GetSpellEffect( SPELL_Poison,

			// We will have this effect again.

			if (IsSetOF(OF_Buffs) && IsClient())
			{
				GetClient()->removeBuff(BI_POISON);
				GetClient()->addBuff(BI_POISON, 1017383, 1070722, static_cast<WORD>(pItem->GetTimerAdjusted()));
			}
			break;
		}
		case SPELL_Strangle:
		{
			int iDiff = pItem->m_itSpell.m_spelllevel - pItem->m_itSpell.m_spellcharges;	// Retrieves the total amount of ticks done substracting spellcharges from spelllevel.

			switch (iDiff) //First tick is in 5 seconds (when mem was created), second one in 4, next one in 3, 2 ... and following ones in each second.
			{
			case 0:
				pItem->SetTimeout(4 * TICK_PER_SEC);
				break;
			case 1:
				pItem->SetTimeout(3 * TICK_PER_SEC);
				break;
			case 2:
				pItem->SetTimeout(2 * TICK_PER_SEC);
				break;
			default:
				pItem->SetTimeout(TICK_PER_SEC);
				break;
			}

			int iSpellPower = static_cast<int>(Calc_GetRandLLVal2(pItem->m_itSpell.m_spelllevel - 2, pItem->m_itSpell.m_spelllevel + 1));
			int iDmg = iSpellPower * ( 3 - ( (Stat_GetBase(STAT_DEX) / Stat_GetAdjusted(STAT_DEX) ) * 2));
			/*Chokes an enemy with poison, doing more damage as their Stamina drops.The power of the effect is equal to the Caster's Spirit Speak skill divided by 10.
			The minimum power is 4. The power number determines the duration and base damage of the Strangle effect.
			Each point of power causes the Strangle effect to damage the target one time.The first round of damage is done after five seconds.
			Four seconds later, the second round hits.Each round after that comes one second more quickly than the last, until there is only 1 second between hits.
			Damage is calculated as follows : The range of damage is between power - 2 and power + 1.
			Then the damage is multiplied based on the victim's current and maximum Stamina values.
			The more the victim is fatigued, the more damage this spell deals.
			The damage is multiplied by the result of this formula: 3 - (Cur Stamina ÷ Max Stamina x 2.
			For example, suppose the base damage for a Strangle hit is 5. The target currently has 40 out of a maximum of 80 stamina. Final damage for that hit is: 5 x (3 - (40 ÷ 80 x 2) = 10.*/
			OnTakeDamage(maximum(1, iDmg), pItem->m_uidLink.CharFind(), DAMAGE_MAGIC | DAMAGE_POISON | DAMAGE_NOREVEAL, 0, 0, 0, 100, 0);
		}
		case SPELL_Pain_Spike:
		{
			// Receives x amount (stored in pItem->m_itSpell.m_spelllevel) of damage in 10 seconds, so damage each second is equal to total / 10
			OnTakeDamage(pItem->m_itSpell.m_spelllevel / 10, pItem->m_uidLink.CharFind(), DAMAGE_MAGIC | DAMAGE_GOD);	// DIRECT? damage
			pItem->SetTimeout(TICK_PER_SEC);
		}
		break;

	default:
		return( false );
	}

	// Total number of ticks to come back here.
	if ( --pItem->m_itSpell.m_spellcharges )
		return( true );
	return( false );
}

CItem * CChar::Spell_Effect_Create( SPELL_TYPE spell, LAYER_TYPE layer, int iSkillLevel, int iDuration, CObjBase * pSrc, bool bEquip )
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

	CItem * pSpell;
	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
	pSpell = CItem::CreateBase( pSpellDef ? ( pSpellDef->m_idSpell ) : ITEMID_RHAND_POINT_NW );
	ASSERT(pSpell);

	switch ( layer )
	{
		case LAYER_FLAG_Criminal:		pSpell->SetName("Criminal Timer");			break;
		case LAYER_FLAG_Potion:			pSpell->SetName("Potion Cooldown");			break;
		case LAYER_FLAG_Drunk:			pSpell->SetName("Drunk Effect");			break;
		case LAYER_FLAG_ClientLinger:	pSpell->SetName("Client Linger");			break;
		case LAYER_FLAG_Hallucination:	pSpell->SetName("Hallucination Effect");	break;
		case LAYER_FLAG_Murders:		pSpell->SetName("Murder Decay");			break;
		default:						break;
	}

//	if (( IsSetMagicFlags( MAGICF_STACKSTATS ) && (layer == LAYER_SPELL_STATS) ) || (layer == LAYER_NONE))
//		layer = pSpell->Item_GetDef()->GetEquipLayer();

	pSpell->SetAttr(ATTR_NEWBIE);
	if ( pSpellDef )
		pSpell->SetAttr(ATTR_MAGIC);

	pSpell->SetType(IT_SPELL);
	pSpell->SetDecayTime(iDuration);
	pSpell->m_itSpell.m_spell = static_cast<WORD>(spell);
	pSpell->m_itSpell.m_spelllevel = static_cast<WORD>(g_Cfg.GetSpellEffect(spell, iSkillLevel));
	pSpell->m_itSpell.m_spellcharges = 1;

	if ( pSrc )
		pSpell->m_uidLink = pSrc->GetUID();

	g_World.m_uidNew = pSpell->GetUID();
	Spell_Effect_Add(pSpell);
	if (bEquip)
		LayerAdd( pSpell, layer );	// Remove any competing effect first.
	return( pSpell );
}

void CChar::Spell_Area( CPointMap pntTarg, int iDist, int iSkillLevel )
{
	ADDTOCALLSTACK("CChar::Spell_Area");
	// Effects all creatures in the area. (but not us)
	// ARGS:
	// iSkillLevel = 0-1000
	//

	SPELL_TYPE spelltype = m_atMagery.m_Spell;
	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spelltype);
	if ( pSpellDef == NULL )
		return;

	CWorldSearch AreaChar( pntTarg, iDist );
	for (;;)
	{
		CChar * pChar = AreaChar.GetChar();
		if ( pChar == NULL )
			break;
		if ( pChar == this )
		{
			if ( pSpellDef->IsSpellType(SPELLFLAG_HARM) && !IsSetMagicFlags(MAGICF_CANHARMSELF) )
				continue;
		}
		pChar->OnSpellEffect( spelltype, this, iSkillLevel, NULL );
	}

	if ( !pSpellDef->IsSpellType( SPELLFLAG_DAMAGE ))	// prevent damage nearby items on ground
	{
		CWorldSearch AreaItem( pntTarg, iDist );
		for (;;)
		{
			CItem * pItem = AreaItem.GetItem();
			if ( pItem == NULL )
				break;
			pItem->OnSpellEffect( spelltype, this, iSkillLevel, NULL );
		}
	}
}

void CChar::Spell_Field(CPointMap pntTarg, ITEMID_TYPE idEW, ITEMID_TYPE idNS, unsigned int fieldWidth, unsigned int fieldGauge, int iSkillLevel, CChar * pCharSrc, ITEMID_TYPE idnewEW, ITEMID_TYPE idnewNS, int iDuration, HUE_TYPE iColor)
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

	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_Spell);
	ASSERT(pSpellDef);

	if ( m_pArea && m_pArea->IsGuarded() && pSpellDef->IsSpellType(SPELLFLAG_HARM) )
		Noto_Criminal();

	// get the dir of the field.
	int dx = abs( pntTarg.m_x - GetTopPoint().m_x );
	int dy = abs( pntTarg.m_y - GetTopPoint().m_y );
	ITEMID_TYPE id = (dx > dy) ? idnewNS ? idnewNS : idNS: idnewEW ? idnewEW : idEW;

	int minX = static_cast<int>((fieldWidth - 1) / 2) - (fieldWidth - 1);
	int maxX = minX + (fieldWidth - 1);

	int minY = static_cast<int>((fieldGauge - 1) / 2) - (fieldGauge - 1);
	int maxY = minY+(fieldGauge - 1);

	if (iDuration <= 0)
		iDuration = GetSpellDuration( m_atMagery.m_Spell, iSkillLevel, 1000, pCharSrc );

	if ( IsSetMagicFlags( MAGICF_NOFIELDSOVERWALLS ) )
	{
		// check if anything is blocking the field from fully extending to its desired width

		// first checks center piece, then left direction (minX), and finally right direction (maxX)
		// (structure of the loop looks a little odd but it should be more effective for wide fields (we don't really
		// want to be testing the far left or right of the field when it has been blocked towards the center))
		for (int ix = 0; ; ix <= 0? ix-- : ix++)
		{
			if (ix < minX)
				ix = 1;	// start checking right extension
			if (ix > maxX)
				break; // all done

			// check the whole width of the field for anything that would block this placement
			for (int iy = minY; iy <= maxY; iy++)
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
				if ( wBlockFlags & ( CAN_I_BLOCK | CAN_I_DOOR ) )
				{
					if (ix < 0)	// field cannot extend fully to the left
						minX = ix + 1;
					else if (ix > 0) // field cannot extend fully to the right
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
		for ( int iy = minY; iy <= maxY; iy++) 
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
			CWorldSearch AreaChar( ptg );
			for (;;)
			{
				CChar * pChar = AreaChar.GetChar();
				if ( pChar == NULL )
					break;

				if ( pChar->GetPrivLevel() > GetPrivLevel() )	// skip higher priv characters
					continue;

				if (( pSpellDef->IsSpellType(SPELLFLAG_HARM) ) && ( !pChar->OnAttackedBy( this, 1, false ) ))	// they should know they where attacked.
					continue;

				if ( !pSpellDef->IsSpellType( SPELLFLAG_NOUNPARALYZE ) )
				{
					if (pChar->LayerFind( LAYER_FLAG_Stuck ))
						pChar->LayerFind( LAYER_FLAG_Stuck )->Delete();
					pChar->StatFlag_Clear( STATF_Freeze );
				}

				if (( idEW == ITEMID_STONE_WALL ) || ( idEW == ITEMID_FX_ENERGY_F_EW ) || ( idEW == ITEMID_FX_ENERGY_F_NS ))	// don't place stone wall over characters
				{
					fGoodLoc = false;
					break;
				}
			}

			if (!fGoodLoc)
				continue;

			// Check for direct cast on an item.
			CWorldSearch AreaItem( ptg );
			for (;;)
			{
				CItem * pItem = AreaItem.GetItem();
				if ( pItem == NULL )
					break;
				pItem->OnSpellEffect( m_atMagery.m_Spell, this, iSkillLevel, NULL );
			}

			CItem * pSpell = CItem::CreateBase( id );
			ASSERT(pSpell);
			pSpell->m_itSpell.m_spell = static_cast<WORD>(m_atMagery.m_Spell);
			pSpell->m_itSpell.m_spelllevel = static_cast<WORD>(iSkillLevel);
			pSpell->m_itSpell.m_spellcharges = 1;
			pSpell->m_uidLink = GetUID();	// link it back to you
			pSpell->SetType(IT_SPELL);
			pSpell->SetAttr(ATTR_MAGIC);
			pSpell->SetHue(iColor);
			pSpell->GenerateScript(this);
			pSpell->MoveToDecay( ptg, iDuration, true);
		}
	}
}

bool CChar::Spell_CanCast( SPELL_TYPE &spell, bool fTest, CObjBase * pSrc, bool fFailMsg, bool fCheckAntiMagic )
{
	ADDTOCALLSTACK("CChar::Spell_CanCast");
	// ARGS:
	//  pSrc = possible scroll or wand source.
	if ( spell <= SPELL_NONE || pSrc == NULL )
		return( false );

	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
	if ( pSpellDef == NULL )
		return( false );
	if ( pSpellDef->IsSpellType( SPELLFLAG_DISABLED ))
		return( false );

	int wManaUse = pSpellDef->m_wManaUse * (100 - minimum(static_cast<int>(GetDefNum("LOWERMANACOST", true, true)), 40)) / 100;

	if (pSrc != this)
	{
		CItem * pItem = dynamic_cast <CItem*> (pSrc);
		if (pItem)
		{
			if (pItem->GetType() == IT_WAND)
				wManaUse = 0;
			else if(pItem->GetType() == IT_SCROLL)
				wManaUse /= 2;
		}
	}


	CScriptTriggerArgs Args( spell, wManaUse, pSrc );
	if ( fTest )
		Args.m_iN3 |= 0x0001;
	if ( fFailMsg )
		Args.m_iN3 |= 0x0002;

	if ( IsTrigUsed(TRIGGER_SELECT) )
	{
		TRIGRET_TYPE iRet = Spell_OnTrigger( spell, SPTRIG_SELECT, this, &Args );
		if ( iRet == TRIGRET_RET_TRUE )
			return false;

		if ( iRet == TRIGRET_RET_FALSE )
			return true;

		if ( iRet == TRIGRET_RET_HALFBAKED )		// just for compatibility with @SPELLSELECT
			return true;
	}

	if ( IsTrigUsed(TRIGGER_SPELLSELECT) )
	{
		TRIGRET_TYPE iRet = OnTrigger(CTRIG_SpellSelect, this, &Args );
		if ( iRet == TRIGRET_RET_TRUE )
			return false;

		if ( iRet == TRIGRET_RET_HALFBAKED )
			return true;
	}

	if ( spell != Args.m_iN1 )
	{
		pSpellDef = g_Cfg.GetSpellDef(spell);
		if ( pSpellDef == NULL )
			return( false );
		spell = static_cast<SPELL_TYPE>(Args.m_iN1);
	}
	wManaUse = static_cast<int>(Args.m_iN2);

	if ( pSrc != this )
	{
		// Cast spell using magic items (wand/scroll)
		CItem * pItem = dynamic_cast <CItem*> (pSrc);
		if ( !pItem )
			return false;
		if ( ! pItem->IsAttr( ATTR_MAGIC ))
		{
			if ( fFailMsg )
				SysMessageDefault( DEFMSG_SPELL_ENCHANT_LACK );
			return( false );
		}
		CObjBaseTemplate * pObjTop = pSrc->GetTopLevelObj();
		if ( pObjTop != this )		// magic items must be on your person to use.
		{
			if ( fFailMsg )
				SysMessageDefault( DEFMSG_SPELL_ENCHANT_ACTIVATE );
			return( false );
		}
		if ( pItem->IsType(IT_WAND))
		{
			// Must have charges.
			if ( pItem->m_itWeapon.m_spellcharges <= 0 )
			{
				if ( fFailMsg )
					SysMessageDefault( DEFMSG_SPELL_WAND_NOCHARGE );
				return false;
			}
			//wManaUse = 0;
			if ( ! fTest && pItem->m_itWeapon.m_spellcharges != 255 )
			{
				pItem->m_itWeapon.m_spellcharges --;
				pItem->UpdatePropertyFlag(AUTOTOOLTIP_FLAG_WANDCHARGES);
			}
		}
		else	// Scroll
		{
			//wManaUse /= 2;
			if ( ! fTest )
			{
				 pItem->ConsumeAmount();
			}
		}
	}
	else
	{
		// Raw cast from spellbook.
		if ( IsPriv( PRIV_GM ))
			return( true );

		if ( m_pPlayer )
		{
			if ( IsStatFlag( STATF_DEAD|STATF_Sleeping ) || ! pSpellDef->m_SkillReq.IsResourceMatchAll(this))
			{
				if ( fFailMsg )
					SysMessageDefault( DEFMSG_SPELL_TRY_DEAD );
				return( false );
			}

			// check the spellbook for it.
			CItem * pBook = GetSpellbook( spell );
			if ( pBook == NULL )
			{
				if ( fFailMsg )
					SysMessageDefault( DEFMSG_SPELL_TRY_NOBOOK );
				return( false );
			}
			if ( ! pBook->IsSpellInBook( spell ))
			{
				if ( fFailMsg )
					SysMessageDefault( DEFMSG_SPELL_TRY_NOTYOURBOOK );
				return( false );
			}

			// check for reagents
			if ( g_Cfg.m_fReagentsRequired && ! m_pNPC && pSrc == this )
			{
				if ( GetDefNum("LOWERREAGENTCOST", true, true) <= Calc_GetRandVal(100))
				{
					const CResourceQtyArray * pRegs = &(pSpellDef->m_Reags);
					CItemContainer * pPack = GetPack();
					size_t iMissing = pPack->ResourceConsumePart( pRegs, 1, 100, fTest );
					if ( iMissing != pRegs->BadIndex() )
					{
						if ( fFailMsg )
						{
							CResourceDef * pReagDef = g_Cfg.ResourceGetDef( pRegs->GetAt(iMissing).GetResourceID() );
							SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_SPELL_TRY_NOREGS ), pReagDef ? pReagDef->GetName() : g_Cfg.GetDefaultMsg( DEFMSG_SPELL_TRY_THEREG ) );
						}
						return( false );
					}
				}
			}
		}
	}

	if ( fCheckAntiMagic )
	{
		if ( ! IsPriv(PRIV_GM) && m_pArea && m_pArea->CheckAntiMagic( spell ))
		{
			if ( fFailMsg )
				SysMessageDefault( DEFMSG_MAGERY_6 ); // An anti-magic field disturbs the spells.
			m_Act_Difficulty = -1;	// Give very little credit for failure !
			return( false );
		}
	}

	// Check for mana
	if ( Stat_GetVal(STAT_INT) < wManaUse )
	{
		if ( fFailMsg )
			SysMessageDefault( DEFMSG_SPELL_TRY_NOMANA );
		return( false );
	}
	if ( ! fTest && wManaUse )
	{
		// Consume mana.
		if ( m_Act_Difficulty < 0 )	// use diff amount of mana if we fail.
		{
			wManaUse = wManaUse/2 + Calc_GetRandVal( wManaUse/2 + wManaUse/4 );
		}
		UpdateStatVal( STAT_INT, -wManaUse );
	}

	return( true );
}

bool CChar::Spell_TargCheck_Face()
{
	ADDTOCALLSTACK("CChar::Spell_TargCheck_Face");
	if ( !IsSetMagicFlags(MAGICF_NODIRCHANGE) )
		UpdateDir(m_Act_p);

		// Is my target in an anti magic feild.
	CRegionWorld * pArea = dynamic_cast <CRegionWorld *> ( m_Act_p.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA));
	if ( ! IsPriv(PRIV_GM) && pArea && pArea->CheckAntiMagic( m_atMagery.m_Spell ))
	{
		SysMessageDefault( DEFMSG_SPELL_TRY_AM );
		m_Act_Difficulty = -1;	// Give very little credit for failure !
		return( false );
	}
	return( true );
}

bool CChar::Spell_TargCheck()
{
	ADDTOCALLSTACK("CChar::Spell_TargCheck");
	// Is the spells target or target pos valid ?

	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_Spell);
	if ( pSpellDef == NULL )
	{
		DEBUG_ERR(( "Bad Spell %d, uid 0%0lx\n", m_atMagery.m_Spell, (DWORD) GetUID()));
		return( false );
	}

	CObjBase * pObj = m_Act_Targ.ObjFind();
	CObjBaseTemplate * pObjTop = NULL;
	if ( pObj != NULL )
	{
		pObjTop = pObj->GetTopLevelObj();
	}

	// NOTE: Targeting a field spell directly on a char should not be allowed ?
	if ( pSpellDef->IsSpellType(SPELLFLAG_FIELD) && !pSpellDef->IsSpellType(SPELLFLAG_TARG_CHAR) )
	{
		if ( m_Act_Targ.IsValidUID() && m_Act_Targ.IsChar())
		{
			SysMessageDefault( DEFMSG_SPELL_TARG_FIELDC );
			return false;
		}
	}

	// Need a target.
	if ( pSpellDef->IsSpellType( SPELLFLAG_TARG_OBJ ) && !( !pObj && pSpellDef->IsSpellType( SPELLFLAG_TARG_XYZ ) ) )
	{
		if ( pObj == NULL || pObjTop == NULL )
		{
			SysMessageDefault( DEFMSG_SPELL_TARG_OBJ );
			return( false );
		}
		if ( !CanSee(pObj) || !CanSeeLOS(pObj, LOS_NB_WINDOWS) ) //we should be able to cast through a window
		{
			SysMessageDefault(DEFMSG_SPELL_TARG_LOS);
			return false;
		}
		if ( !IsPriv(PRIV_GM) && pObjTop != this && pObjTop != pObj && pObjTop->IsChar() )
		{
			SysMessageDefault( DEFMSG_SPELL_TARG_CONT );
			return( false );
		}

		m_Act_p = pObjTop->GetTopPoint();

		if ( ! Spell_TargCheck_Face() )
			return( false );

	}
	else if ( pSpellDef->IsSpellType( SPELLFLAG_TARG_XYZ ))
	{
		if ( pObj )
		{
			m_Act_p = pObjTop->GetTopPoint();
		}
		if ( ! CanSeeLOS( m_Act_p, NULL, UO_MAP_VIEW_SIGHT, LOS_NB_WINDOWS )) //we should be able to cast through a window
		{
			SysMessageDefault( DEFMSG_SPELL_TARG_LOS );
			return( false );
		}
		if ( ! Spell_TargCheck_Face() )
			return( false );
	}

	return( true );
}

bool CChar::Spell_Unequip( LAYER_TYPE layer )
{
	ADDTOCALLSTACK("CChar::Spell_Unequip");
	CItem * pItemPrev = LayerFind( layer );
	if ( pItemPrev != NULL )
	{
		if ( IsSetMagicFlags(MAGICF_NOCASTFROZENHANDS) && IsStatFlag( STATF_Freeze ))
		{
			SysMessageDefault( DEFMSG_SPELL_TRY_FROZENHANDS );
			return( false );
		}
		else if ( ! CanMove( pItemPrev ))
		{
			return( false );
		}
		else if ( ! pItemPrev->IsTypeSpellbook() && ! pItemPrev->IsType(IT_WAND) && ! pItemPrev->GetDefKey("SPELLCHANNELING",true))
		{
			ItemBounce( pItemPrev );
		}
	}
	return( true );
}

inline bool CChar::Spell_SimpleEffect( CObjBase * pObj, CObjBase * pObjSrc, SPELL_TYPE &spell, int &iSkillLevel )
{
	ADDTOCALLSTACK("CChar::Spell_SimpleEffect");
	if ( pObj == NULL )
		return( false );
	pObj->OnSpellEffect( spell, this, iSkillLevel, dynamic_cast <CItem*>( pObjSrc ));
	return( true );
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
	//

	if (!Spell_TargCheck())
		return(false);

	CObjBase * pObj = m_Act_Targ.ObjFind();	// dont always need a target.
	CObjBase * pObjSrc = m_Act_TargPrv.ObjFind();
	ITEMID_TYPE iT1 = ITEMID_NOTHING;
	ITEMID_TYPE iT2 = ITEMID_NOTHING;
	CREID_TYPE iC1 = CREID_INVALID;
	HUE_TYPE iColor = HUE_DEFAULT;

	unsigned int fieldWidth = 0;
	unsigned int fieldGauge = 0;
	unsigned int areaRadius = 0;

	SPELL_TYPE spell = m_atMagery.m_Spell;
	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
	if (pSpellDef == NULL)
		return(false);

	bool bIsSpellField = pSpellDef->IsSpellType(SPELLFLAG_FIELD);

	int iSkill, iDifficulty;
	if (!pSpellDef->GetPrimarySkill(&iSkill, &iDifficulty))
		return(false);

	int iSkillLevel;
	if (pObjSrc != this)
	{
		// Get the strength of the item. IT_SCROLL or IT_WAND
		CItem * pItem = dynamic_cast <CItem*>(pObjSrc);
		if (pItem == NULL)
			return(false);
		if (!pItem->m_itWeapon.m_spelllevel)
			iSkillLevel = Calc_GetRandVal(500);
		else
			iSkillLevel = pItem->m_itWeapon.m_spelllevel;
	}
	else
	{
		iSkillLevel = Skill_GetAdjusted(static_cast<SKILL_TYPE>(iSkill));
	}

	CScriptTriggerArgs	Args(spell, iSkillLevel, pObjSrc);
	Args.m_VarsLocal.SetNum("fieldWidth", 0);
	Args.m_VarsLocal.SetNum("fieldGauge", 0);
	Args.m_VarsLocal.SetNum("areaRadius", 0);
	Args.m_VarsLocal.SetNum("duration", GetSpellDuration(spell, iSkillLevel, 1000, this), true);

	if (bIsSpellField)
	{
		switch (spell)	// Only setting ids and locals for field spells
		{
		case SPELL_Wall_of_Stone: 	iT1 = ITEMID_STONE_WALL;				iT2 = ITEMID_STONE_WALL;		break;
		case SPELL_Fire_Field: 		iT1 = ITEMID_FX_FIRE_F_EW; 				iT2 = ITEMID_FX_FIRE_F_NS;		break;
		case SPELL_Poison_Field:	iT1 = ITEMID_FX_POISON_F_EW;			iT2 = ITEMID_FX_POISON_F_NS;	break;
		case SPELL_Paralyze_Field:	iT1 = ITEMID_FX_PARA_F_EW;				iT2 = ITEMID_FX_PARA_F_NS;		break;
		case SPELL_Energy_Field:	iT1 = ITEMID_FX_ENERGY_F_EW;			iT2 = ITEMID_FX_ENERGY_F_NS;	break;
		default: break;
		}

		Args.m_VarsLocal.SetNum("CreateObject1", iT1, false);
		Args.m_VarsLocal.SetNum("CreateObject2", iT2, false);
	}

	if (IsTrigUsed(TRIGGER_SPELLSUCCESS))
	{
		if (OnTrigger(CTRIG_SpellSuccess, this, &Args) == TRIGRET_RET_TRUE)
			return false;
	}

	if (IsTrigUsed(TRIGGER_SUCCESS))
	{
		if (Spell_OnTrigger(spell, SPTRIG_SUCCESS, this, &Args) == TRIGRET_RET_TRUE)
			return false;
	}

	iSkillLevel = static_cast<int>(Args.m_iN2);

	ITEMID_TYPE it1test = ITEMID_NOTHING;
	ITEMID_TYPE it2test = ITEMID_NOTHING;

	if (bIsSpellField)
	{
		//Setting new IDs as another variables to pass as different arguments to the field function.
		it1test = static_cast<ITEMID_TYPE>(RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("CreateObject1", true)));
		it2test = static_cast<ITEMID_TYPE>(RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("CreateObject2", true)));
		//Can't be < 0, so max it to 0
		fieldWidth = static_cast<unsigned int>(maximum(0, Args.m_VarsLocal.GetKeyNum("fieldWidth", true)));
		fieldGauge = static_cast<unsigned int>(maximum(0, Args.m_VarsLocal.GetKeyNum("fieldGauge", true)));

	}

	iC1 = static_cast<CREID_TYPE>(Args.m_VarsLocal.GetKeyNum("CreateObject1", true) & 0xFFFF);
	areaRadius = static_cast<unsigned int>(maximum(0, Args.m_VarsLocal.GetKeyNum("areaRadius", true)));
	unsigned int iDuration = static_cast<unsigned int>(maximum(0, Args.m_VarsLocal.GetKeyNum("duration", true)));
	iColor = static_cast<HUE_TYPE>(maximum(0, Args.m_VarsLocal.GetKeyNum("EffectColor", true)));

	// Consume the reagents/mana/scroll/charge
	if (!Spell_CanCast(spell, false, pObjSrc, true))
		return(false);

	if (pSpellDef->IsSpellType(SPELLFLAG_SCRIPTED))
	{
		if (pSpellDef->IsSpellType(SPELLFLAG_SUMMON))
		{
			if (iC1)
			{
				m_atMagery.m_SummonID = iC1;
				m_atMagery.m_fSummonPet = true;
				Spell_Summon(m_atMagery.m_SummonID, m_Act_p, m_atMagery.m_fSummonPet != 0);
			}
		}
		else if (bIsSpellField)
		{
			if (iT1 && iT2)
			{
				if (!fieldWidth)
					fieldWidth = 5;
				if (!fieldGauge)
					fieldGauge = 1;

				Spell_Field(m_Act_p, iT1, iT2, fieldWidth, fieldGauge, iSkillLevel, this, it1test, it2test, iDuration, iColor);
			}
		}
		else if (pSpellDef->IsSpellType(SPELLFLAG_AREA))
		{
			if (!areaRadius)
				areaRadius = 4;

			if (!pSpellDef->IsSpellType(SPELLFLAG_TARG_OBJ | SPELLFLAG_TARG_XYZ))
				Spell_Area(GetTopPoint(), areaRadius, iSkillLevel);
			else
				Spell_Area(m_Act_p, areaRadius, iSkillLevel);
		}
		else if (pSpellDef->IsSpellType(SPELLFLAG_POLY))
		{
			return(false);
		}
		else
		{
			if (pObj)
			{
				pObj->OnSpellEffect(spell, this, iSkillLevel, dynamic_cast <CItem*>(pObjSrc));
			}
		}
	}
	else if (bIsSpellField)
	{
		if (!fieldWidth)
			fieldWidth = 3;
		if (!fieldGauge)
			fieldGauge = 1;

		Spell_Field(m_Act_p, iT1, iT2, fieldWidth, fieldGauge, iSkillLevel, this, it1test, it2test, iDuration, iColor);
	}
	else if (pSpellDef->IsSpellType(SPELLFLAG_AREA))
	{
		if (!areaRadius)
		{
			switch (spell)
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
			default: areaRadius = 4;											break;
			}
		}

		if (!pSpellDef->IsSpellType(SPELLFLAG_TARG_OBJ | SPELLFLAG_TARG_XYZ))
			Spell_Area(GetTopPoint(), areaRadius, iSkillLevel);
		else
			Spell_Area(m_Act_p, areaRadius, iSkillLevel);

	}
	else if (pSpellDef->IsSpellType(SPELLFLAG_SUMMON))
	{
		if (spell == SPELL_Summon)
		{
			if (iC1)
				m_atMagery.m_SummonID = iC1;
			Spell_Summon(m_atMagery.m_SummonID, m_Act_p, m_atMagery.m_fSummonPet != 0);
		}
		else
		{
			if (!iC1)
			{
				switch (spell)
				{
				case SPELL_Blade_Spirit:	m_atMagery.m_SummonID = CREID_BLADES;		break;
				case SPELL_Vortex:			m_atMagery.m_SummonID = CREID_VORTEX;		break;
				case SPELL_Air_Elem:		m_atMagery.m_SummonID = CREID_AIR_ELEM;		break;
				case SPELL_Daemon:			m_atMagery.m_SummonID = CREID_DAEMON;		break;
				case SPELL_Earth_Elem:		m_atMagery.m_SummonID = CREID_EARTH_ELEM;	break;
				case SPELL_Fire_Elem:		m_atMagery.m_SummonID = CREID_FIRE_ELEM;	break;
				case SPELL_Water_Elem:		m_atMagery.m_SummonID = CREID_WATER_ELEM;	break;
				case SPELL_Summon_Undead:
					switch (Calc_GetRandVal(15))
					{
					case 1:					m_atMagery.m_SummonID = CREID_LICH;			break;
					case 3:
					case 5:
					case 7:
					case 9:					m_atMagery.m_SummonID = CREID_SKELETON;		break;
					default:				m_atMagery.m_SummonID = CREID_ZOMBIE;		break;
					}
				case SPELL_Vengeful_Spirit:	m_atMagery.m_SummonID = CREID_Revenant;		break;
				default: break;
				}
			}
			else
				m_atMagery.m_SummonID = iC1;

			m_atMagery.m_fSummonPet = true;
			Spell_Summon(m_atMagery.m_SummonID, m_Act_p, m_atMagery.m_fSummonPet != 0);
		}
	}
	else
	{
		iT1 = it1test;	// Set iT1 to it1test here because spell_field() needed both values to be passed.

		switch (spell)
		{

			// Magery
			case SPELL_Create_Food:
			{
				RESOURCE_ID food = g_Cfg.ResourceGetIDType(RES_ITEMDEF, "DEFFOOD");
				CItem * pItem = CItem::CreateScript((iT1 ? iT1 : static_cast<ITEMID_TYPE>(food.GetResIndex())), this);
				if (pSpellDef->IsSpellType(SPELLFLAG_TARG_OBJ | SPELLFLAG_TARG_XYZ))
				{
					pItem->MoveToCheck(m_Act_p, this);
				}
				else
				{
					TCHAR * pMsg = Str_GetTemp();
					sprintf(pMsg, g_Cfg.GetDefaultMsg(DEFMSG_SPELL_CREATE_FOOD), pItem->GetName());
					SysMessage(pMsg);
					CItemContainer * pPack = GetPackSafe();
					pPack->ContentAdd(pItem);
				}
			}
			break;

			case SPELL_Magic_Trap:
			case SPELL_Magic_Untrap:
				/* Create the trap object and link it to the target.
				   A container is diff from door or stationary object */
				break;

			case SPELL_Telekin:	// Act as DClick on the object.
				Use_Obj(pObj, false);
				break;

			case SPELL_Teleport:
				Spell_Teleport(m_Act_p);
				break;

			case SPELL_Recall:
				if (!Spell_Recall(dynamic_cast <CItem*> (pObj), false))
					return(false);
				break;

			case SPELL_Dispel_Field:
			{
				CItem * pItem = dynamic_cast <CItem*> (pObj);
				if (pItem == NULL || pItem->IsAttr(ATTR_MOVE_NEVER) || (!pItem->IsType(IT_CAMPFIRE) && !pItem->IsType(IT_SPELL) && !pItem->IsType(IT_FIRE)))
				{
					SysMessageDefault(DEFMSG_SPELL_DISPELLF_WT);
					return(false);
				}
				pItem->OnSpellEffect(SPELL_Dispel_Field, this, iSkillLevel, NULL);
				break;
			}

			case SPELL_Mind_Blast:
				if (pObj->IsChar())
				{
					CChar * pChar = dynamic_cast <CChar*> (pObj);
					ASSERT(pChar);
					int iDiff = (Stat_GetAdjusted(STAT_INT) - pChar->Stat_GetAdjusted(STAT_INT)) / 2;
					if (iDiff < 0)
					{
						pChar = this;	// spell revereses !
						iDiff = -iDiff;
					}
					int iMax = pChar->Stat_GetMax(STAT_STR) / 2;
					pChar->OnSpellEffect(spell, this, minimum(iDiff, iMax), NULL);
				}
				break;

				/*case SPELL_Summon: //not needed with SPELLFLAG_SUMMON
					{
					if ( iC1 )
					m_atMagery.m_SummonID = iC1;

					Spell_Summon( m_atMagery.m_SummonID, m_Act_p, m_atMagery.m_fSummonPet != 0 );
					break;
					}*/

			case SPELL_Flame_Strike:
			{
				// Display spell.
				if (!iT1)
					iT1 = ITEMID_FX_FLAMESTRIKE;

				if (pObj == NULL)
				{
					CItem * pItem = CItem::CreateBase(iT1);
					ASSERT(pItem);
					pItem->SetType(IT_SPELL);
					pItem->m_itSpell.m_spell = SPELL_Flame_Strike;
					pItem->MoveToDecay(m_Act_p, 2 * TICK_PER_SEC);
				}
				else
				{
					// Burn person at location.
					//pObj->Effect(EFFECT_OBJ, iT1, pObj, 10, 30, false, iColor, iRender);
					if (!Spell_SimpleEffect(pObj, pObjSrc, spell, iSkillLevel))
						return(false);
				}
				break;
			}

			case SPELL_Gate_Travel:
				if (!Spell_Recall(dynamic_cast <CItem*> (pObj), true))
					return(false);
				break;

			case SPELL_Polymorph:
			case SPELL_Wraith_Form:
			case SPELL_Horrific_Beast:
			case SPELL_Lich_Form:
			case SPELL_Vampiric_Embrace:
			case SPELL_Stone_Form:
			case SPELL_Reaper_Form:
				// This has a menu select for client.
				if (GetPrivLevel() < PLEVEL_Seer)
				{
					if (pObj != this)
						return(false);
				}
				if (!Spell_SimpleEffect(pObj, pObjSrc, spell, iSkillLevel))
					return(false);
				break;

			case SPELL_Animate_Dead:
			{
				CItemCorpse * pCorpse = dynamic_cast <CItemCorpse*> (pObj);
				if (pCorpse == NULL)
				{
					SysMessageDefault(DEFMSG_SPELL_ANIMDEAD_NC);
					return(false);
				}
				if (IsPriv(PRIV_GM))
				{
					m_atMagery.m_SummonID = pCorpse->m_itCorpse.m_BaseID;
				}
				else if (CCharBase::IsPlayableID(pCorpse->GetCorpseType())) 	// Must be a human corpse ?
				{
					m_atMagery.m_SummonID = CREID_ZOMBIE;
				}
				else
				{
					m_atMagery.m_SummonID = pCorpse->GetCorpseType();
				}
				m_atMagery.m_fSummonPet = true;

				if (!pCorpse->IsTopLevel())
				{
					return(false);
				}
				CChar * pChar = Spell_Summon(m_atMagery.m_SummonID, pCorpse->GetTopPoint(), true);
				ASSERT(pChar);
				if (!pChar->RaiseCorpse(pCorpse))
				{
					SysMessageDefault(DEFMSG_SPELL_ANIMDEAD_FAIL);
					pChar->Delete();
				}
				break;
			}

			case SPELL_Bone_Armor:
			{
				CItemCorpse * pCorpse = dynamic_cast <CItemCorpse*> (pObj);
				if (pCorpse == NULL)
				{
					SysMessage("That is not a corpse!");
					return(false);
				}
				if (!pCorpse->IsTopLevel() ||
					pCorpse->GetCorpseType() != CREID_SKELETON) 	// Must be a skeleton corpse
				{
					SysMessage("The body stirs for a moment");
					return(false);
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
				for (size_t i = 0; i < COUNTOF(sm_Item_Bone); i++)
				{
					if (!Calc_GetRandVal(2 + iGet))
						break;
					CItem * pItem = CItem::CreateScript(static_cast<ITEMID_TYPE>(sm_Item_Bone[i]), this);
					pItem->MoveToCheck(m_Act_p, this);
					iGet++;
				}
				if (!iGet)
				{
					SysMessage("The bones shatter into dust!");
					break;
				}
			}
			break;

			default:
				if (!Spell_SimpleEffect(pObj, pObjSrc, spell, iSkillLevel))
					return(false);
				break;
		}
	}

	if ( g_Cfg.m_fHelpingCriminalsIsACrime && pSpellDef->IsSpellType(SPELLFLAG_GOOD) && pObj != NULL && pObj->IsChar() && pObj != this )
	{
		CChar * pChar = dynamic_cast <CChar*> ( pObj );
		ASSERT( pChar );
		switch ( pChar->Noto_GetFlag( this, false ))
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
	if ( !IsStatFlag( STATF_Insubstantial) )
		Sound( pSpellDef->m_sound );
	
	// At this point we should gain skill if precasting is enabled
	if ( IsClient() && IsSetMagicFlags(MAGICF_PRECAST) && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST) )
	{
		iDifficulty /= 10;
		Skill_Experience(static_cast<SKILL_TYPE>(iSkill), iDifficulty);
	}
	return(true);
}

void CChar::Spell_CastFail()
{
	ADDTOCALLSTACK("CChar::Spell_CastFail");
	ITEMID_TYPE iT1 = ITEMID_FX_SPELL_FAIL;
	CScriptTriggerArgs	Args( m_atMagery.m_Spell, 0, m_Act_TargPrv.ObjFind() );
	Args.m_VarsLocal.SetNum("CreateObject1",iT1);
	if ( IsTrigUsed(TRIGGER_SPELLFAIL) )
	{
		if ( OnTrigger( CTRIG_SpellFail, this, &Args ) == TRIGRET_RET_TRUE )
			return;
	}

	if ( IsTrigUsed(TRIGGER_FAIL) )
	{
		if ( Spell_OnTrigger( m_atMagery.m_Spell, SPTRIG_FAIL, this, &Args ) == TRIGRET_RET_TRUE )
			return;
	}

	HUE_TYPE iColor = static_cast<HUE_TYPE>(maximum(0, Args.m_VarsLocal.GetKeyNum("EffectColor", true)));
	DWORD iRender = static_cast<DWORD>(maximum(0, Args.m_VarsLocal.GetKeyNum("EffectRender", true)));

	iT1 = static_cast<ITEMID_TYPE>(RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("CreateObject1", true)));
	if (iT1)
		Effect(EFFECT_OBJ, iT1, this, 1, 30, false, iColor, iRender);
	Sound( SOUND_SPELL_FIZZLE );

	if ( IsClient() )
		GetClient()->addObjMessage( g_Cfg.GetDefaultMsg( DEFMSG_SPELL_GEN_FIZZLES ), this );

	if ( g_Cfg.m_fReagentLossFail )
	{
		// consume the regs.
		Spell_CanCast( m_atMagery.m_Spell, false, m_Act_TargPrv.ObjFind(), false );
	}
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
	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_Spell);
	if ( pSpellDef == NULL )
		return( -1 );

	if ( IsClient() && IsSetMagicFlags(MAGICF_PRECAST) && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST) )
	{
		m_Act_p = GetTopPoint();
		m_Act_Targ = GetClient()->m_Targ_UID;
		m_Act_TargPrv = GetClient()->m_Targ_PrvUID;

		if ( !Spell_CanCast(m_atMagery.m_Spell, true, m_Act_TargPrv.ObjFind(), true) )
			return(-1);
	}
	else
	{
		if ( !Spell_TargCheck() )
			return(-1);
	}

	bool fWOP = ( GetPrivLevel() >= PLEVEL_Counsel ) ? g_Cfg.m_fWordsOfPowerStaff : g_Cfg.m_fWordsOfPowerPlayer;
	if ( !NPC_CanSpeak() || IsStatFlag(STATF_Insubstantial) )
		fWOP = false;

	int iDifficulty;
	int iSkill;
	if ( !pSpellDef->GetPrimarySkill( &iSkill, &iDifficulty ) )
		return -1;

	// Adjust to 0 - 100
	iDifficulty	/= 10;

	CGrayUID uid(m_Act_TargPrv);
	CItem * pItem = uid.ItemFind();
	bool fAllowEquip = false;
	if ( pItem != NULL )
	{
		if ( pItem->IsType(IT_WAND))
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

	INT64 iWaitTime = IsPriv(PRIV_GM) ? 1 : pSpellDef->m_CastTime.GetLinear(Skill_GetBase(static_cast<SKILL_TYPE>(iSkill)));
	iWaitTime -= GetDefNum("FASTERCASTING", true, true) * 2;	//correct value is 0.25, but sphere can handle only 0.2
	if ( iWaitTime < 1 )
		iWaitTime = 1;

	CScriptTriggerArgs Args(static_cast<int>(m_atMagery.m_Spell), iDifficulty, pItem);
	Args.m_iN3 = iWaitTime;
	Args.m_VarsLocal.SetNum("WOP",fWOP);
	Args.m_VarsLocal.SetNum("WOPColor", g_Cfg.m_iWordsOfPowerColor, true);
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
			return( -1 );
		if ( !Spell_Unequip(LAYER_HAND2) )
			return( -1 );
	}

	// Animate casting.
	if ( !pSpellDef->IsSpellType(SPELLFLAG_NO_CASTANIM) && !IsSetMagicFlags(MAGICF_NOANIM) )
		UpdateAnimate( (pSpellDef->IsSpellType(SPELLFLAG_DIR_ANIM)) ? ANIM_CAST_DIR : ANIM_CAST_AREA );

	m_atMagery.m_Spell = static_cast<SPELL_TYPE>(Args.m_iN1);
	iDifficulty = static_cast<int>(Args.m_iN2);
	iWaitTime = static_cast<int>(Args.m_iN3);
	fWOP = Args.m_VarsLocal.GetKeyNum("WOP",true) > 0 ? true : false;
	int WOPColor = static_cast<int>(Args.m_VarsLocal.GetKeyNum("WOPColor", true));
	if (WOPColor < 0)
		WOPColor = g_Cfg.m_iWordsOfPowerColor;
	int WOPFont = static_cast<int>(Args.m_VarsLocal.GetKeyNum("WOPFont", true));
	if (WOPFont < 0)
		WOPFont = g_Cfg.m_iWordsOfPowerFont;

	if ( fWOP )
	{
		pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_Spell);
		if ( !pSpellDef )
			return -1;

		if ( pSpellDef->m_sRunes[0] == '.' )
		{
			Speak((pSpellDef->m_sRunes.GetPtr()) + 1);
		}
		else
		{
			size_t len = 0;
			TCHAR * pszTemp = Str_GetTemp();

			size_t i;
			for ( i = 0; ; i++ )
			{
				TCHAR ch = pSpellDef->m_sRunes[i];
				if ( !ch )
					break;
				len += strcpylen(pszTemp+len, g_Cfg.GetRune(ch));
				pszTemp[len++] = ' ';
			}
			if ( i > 0 )
			{
				pszTemp[len] = 0;
				Speak(pszTemp, static_cast<HUE_TYPE>(WOPColor), TALKMODE_SAY, static_cast<FONT_TYPE>(WOPFont));
			}
		}
	}

	SetTimeout(iWaitTime);
	return iDifficulty;
}


bool CChar::OnSpellEffect( SPELL_TYPE spell, CChar * pCharSrc, int iSkillLevel, CItem * pSourceItem )	// adicionar novo arg: bool bPlaySound
{
	ADDTOCALLSTACK("CChar::OnSpellEffect");
	// Spell has a direct effect on this char.
	// This should effect noto of source.
	// ARGS:
	//  pSourceItem = the potion, wand, scroll etc. NULL = cast (IT_SPELL)
	//  iSkillLevel = 0-1000 = difficulty. may be slightly larger .
	// RETURN:
	//  false = the spell did not work. (should we get credit ?)

	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
	if ( pSpellDef == NULL )
		return false;
	if ( iSkillLevel <= 0 )		// spell died or fizzled
		return false;
	if ( IsStatFlag(STATF_DEAD) && !pSpellDef->IsSpellType(SPELLFLAG_TARG_DEAD) )
		return false;
	if ( spell == SPELL_Paralyze_Field && LayerFind(LAYER_SPELL_Paralyze) )	//IsStatFlag(STATF_Freeze)
		return false;
	if ( spell == SPELL_Poison_Field && LayerFind(LAYER_FLAG_Poison) )	//IsStatFlag(STATF_Poisoned)
		return false;

	static const SOUND_TYPE sm_DrinkSounds[] = { 0x030, 0x031 };
	SOUND_TYPE iSound = pSpellDef->m_sound;
	if (pSourceItem && pSourceItem->IsType(IT_POTION))
		iSound = sm_DrinkSounds[Calc_GetRandVal(COUNTOF(sm_DrinkSounds))];

	bool fExplode = false;
	if (pSpellDef->IsSpellType(SPELLFLAG_FX_BOLT) && !pSpellDef->IsSpellType(SPELLFLAG_GOOD))	// bolt (chasing) spells have explode = 1 by default (if not good spell)
		fExplode = true;

	int iEffectMult = 1000;
	CScriptTriggerArgs Args(static_cast<int>(spell), iSkillLevel, pSourceItem);
	Args.m_VarsLocal.SetNum("DamageType", 0);
	Args.m_VarsLocal.SetNum("CreateObject1", pSpellDef->m_idEffect);
	Args.m_VarsLocal.SetNum("Explode", fExplode);
	Args.m_VarsLocal.SetNum("Sound", iSound);
	Args.m_iN3 = iEffectMult;
	TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;

	DAMAGE_TYPE iD1 = 0;
	if ( IsTrigUsed(TRIGGER_SPELLEFFECT) )
	{
		TRIGRET_TYPE iRet = OnTrigger( CTRIG_SpellEffect, pCharSrc ? pCharSrc : this, &Args );
		spell = static_cast<SPELL_TYPE>(Args.m_iN1);
		iD1 = static_cast<DAMAGE_TYPE>(RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("DamageType", true)));

		switch ( iRet )
		{
			case TRIGRET_RET_TRUE:	return false;
			case TRIGRET_RET_FALSE:	if ( pSpellDef->IsSpellType(SPELLFLAG_SCRIPTED) ) return true;
			default:				break;
		}
	}

	if ( IsTrigUsed(TRIGGER_EFFECT) )
	{
		iRet = Spell_OnTrigger( spell, SPTRIG_EFFECT, pCharSrc ? pCharSrc : this, &Args );
		spell = static_cast<SPELL_TYPE>(Args.m_iN1);
		iSkillLevel = static_cast<int>(Args.m_iN2);
		iEffectMult = static_cast<int>(Args.m_iN3);
		iD1 = static_cast<DAMAGE_TYPE>(RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("DamageType", true)));

		switch ( iRet )
		{
			case TRIGRET_RET_TRUE:	return false;
			case TRIGRET_RET_FALSE:	if ( pSpellDef->IsSpellType(SPELLFLAG_SCRIPTED) ) return true;
			default:				break;
		}
	}
	HUE_TYPE iColor = static_cast<HUE_TYPE>(maximum(0, Args.m_VarsLocal.GetKeyNum("EffectColor", true)));
	DWORD iRender = static_cast<DWORD>(maximum(0, Args.m_VarsLocal.GetKeyNum("EffectRender", true)));
	fExplode = Args.m_VarsLocal.GetKeyNum("EffectExplode", true) > 0 ? true : false;
	ITEMID_TYPE iT1 = static_cast<ITEMID_TYPE>(RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("CreateObject1", true)));
	iSound = static_cast<SOUND_TYPE>(Args.m_VarsLocal.GetKeyNum("Sound", true));

	if (iT1 > ITEMID_QTY)
		iT1 = pSpellDef->m_idEffect;

	if ( pSpellDef->IsSpellType(SPELLFLAG_HARM) )
	{
		if ( pCharSrc == this && !IsSetMagicFlags(MAGICF_CANHARMSELF) )
			return false;

		if ( IsStatFlag(STATF_INVUL) )
		{
			Effect(EFFECT_OBJ, ITEMID_FX_GLOW, this, 10, 16);
			return false;
		}
		else if ( GetPrivLevel() == PLEVEL_Guest )
		{
			pCharSrc->SysMessageDefault( DEFMSG_ACC_GUESTHIT );
			Effect(EFFECT_OBJ, ITEMID_FX_GLOW, this, 10, 16);
			return false;
		}

		if ( !OnAttackedBy(pCharSrc, 1, false, !(pSpellDef->IsSpellType(SPELLFLAG_FIELD))) )
			return false;

		// Check if the spell can be reflected
		if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_CHAR) && pCharSrc != NULL && pCharSrc != this )		//only spells with direct target can be reflected
		{
			if ( IsStatFlag(STATF_Reflection) )
			{
				CItem *pMagicReflect = LayerFind(LAYER_SPELL_Magic_Reflect);
				pMagicReflect->Delete();
				Effect(EFFECT_OBJ, ITEMID_FX_GLOW, this, 10, 5);
				if ( pCharSrc->IsStatFlag(STATF_Reflection) )		//caster is under reflection effect too, so the spell will reflect back to default target
				{
					CItem *pMagicReflect = pCharSrc->LayerFind(LAYER_SPELL_Magic_Reflect);
					pMagicReflect->Delete();
					pCharSrc->Effect(EFFECT_OBJ, ITEMID_FX_GLOW, pCharSrc, 10, 5);
				}
				else
				{
					pCharSrc->OnSpellEffect( spell, pCharSrc, iSkillLevel, pSourceItem );
					return false;
				}
			}
		}
	}

	// Check if the spell can be resisted
	bool fPotion = ( pSourceItem != NULL && pSourceItem->IsType(IT_POTION) );
	bool fResistAttempt = true;
	bool fResisted = false;
	if ( pSpellDef->IsSpellType(SPELLFLAG_RESIST) && fResistAttempt && !fPotion && pCharSrc != NULL )
	{
		int iResist = Skill_GetBase(SKILL_MAGICRESISTANCE);
		if (IsAosFlagEnabled(FEATURE_AOS_UPDATE_B))
		{
			CItem * pEvilOmen = LayerFind(LAYER_SPELL_Evil_Omen);
			if (pEvilOmen)
			{
				iResist /= 2;	// Effect 3: Only 50% of magic resistance used in next resistable spell.
				pEvilOmen->Delete();
			}
		}
		int iFirst = iResist / 50;
		int iSecond = iResist - (((pCharSrc->Skill_GetBase(SKILL_MAGERY) - 200) / 50) + (1 + (spell / 8)) * 50);
		int iResistChance = maximum(iFirst, iSecond) / 30;
		if ( Skill_UseQuick( SKILL_MAGICRESISTANCE, iResistChance, true, false ))
		{
			SysMessageDefault( DEFMSG_RESISTMAGIC );
			fResisted = true;
		}
	}

	if (pSpellDef->IsSpellType(SPELLFLAG_SCRIPTED))
		return true;

	if (pSpellDef->IsSpellType(SPELLFLAG_FX_BOLT) && iT1)
		Effect(EFFECT_BOLT, iT1, pCharSrc, 5, 1, fExplode, iColor, iRender);
	if ( pSpellDef->IsSpellType(SPELLFLAG_FX_TARG) && iT1 )
		Effect(EFFECT_OBJ, iT1, this, 0, 15, fExplode, iColor, iRender); // 9, 14
	if (iSound)
		Sound(iSound);

	iSkillLevel = iSkillLevel/2 + Calc_GetRandVal(iSkillLevel/2);	// randomize the effect.

	if ( pSpellDef->IsSpellType(SPELLFLAG_DAMAGE) )
	{
		int iDmg = GetSpellEffect(spell, iSkillLevel, iEffectMult);
		if ( IsSetMagicFlags(MAGICF_OSIFORMULAS) )
		{
			if (pCharSrc == NULL)
				iDmg *= ((iSkillLevel * 3) / 1000) + 1;
			else
			{
				// Evaluating Intelligence mult
				iDmg *= ((pCharSrc->Skill_GetBase(SKILL_EVALINT) * 3) / 1000) + 1;

				// Spell Damage Increase bonus
				int DamageBonus = static_cast<int>(pCharSrc->GetDefNum("INCREASESPELLDAM",true));
				if ( m_pPlayer && pCharSrc->m_pPlayer && DamageBonus > 15 )		// Spell Damage Increase is capped at 15% on PvP
					DamageBonus = 15;

				// INT bonus
				DamageBonus += pCharSrc->Stat_GetAdjusted(STAT_INT) / 10;

				// Inscription bonus
				if ( pCharSrc->Skill_GetBase(SKILL_INSCRIPTION) >= 1000 )
					DamageBonus += 10;

				iDmg += iDmg * DamageBonus / 100;
			}
		}

		if ( fResisted )
			iDmg = iDmg * 75 / 100;

		if ( !iD1 )
		{
			switch ( spell )
			{
				case SPELL_Magic_Arrow:		iD1 = DAMAGE_MAGIC | DAMAGE_FIRE | DAMAGE_NOREVEAL;		break;
				case SPELL_Fireball:		iD1 = DAMAGE_MAGIC | DAMAGE_FIRE | DAMAGE_NOREVEAL;		break;
				case SPELL_Fire_Field:		iD1 = DAMAGE_MAGIC | DAMAGE_FIRE | DAMAGE_NOREVEAL;		break;
				case SPELL_Explosion:		iD1 = DAMAGE_MAGIC | DAMAGE_FIRE | DAMAGE_NOREVEAL;		break;
				case SPELL_Flame_Strike:	iD1 = DAMAGE_MAGIC | DAMAGE_FIRE | DAMAGE_NOREVEAL;		break;
				case SPELL_Meteor_Swarm:	iD1 = DAMAGE_MAGIC | DAMAGE_FIRE | DAMAGE_NOREVEAL;		break;
				case SPELL_Fire_Bolt:		iD1 = DAMAGE_MAGIC | DAMAGE_FIRE | DAMAGE_NOREVEAL;		break;
				case SPELL_Harm:			iD1 = DAMAGE_MAGIC | DAMAGE_COLD | DAMAGE_NOREVEAL;		break;
				case SPELL_Mind_Blast:		iD1 = DAMAGE_MAGIC | DAMAGE_COLD | DAMAGE_NOREVEAL;		break;
				case SPELL_Lightning:		iD1 = DAMAGE_MAGIC | DAMAGE_ENERGY | DAMAGE_NOREVEAL;	break;
				case SPELL_Energy_Bolt:		iD1 = DAMAGE_MAGIC | DAMAGE_ENERGY | DAMAGE_NOREVEAL;	break;
				case SPELL_Chain_Lightning:	iD1 = DAMAGE_MAGIC | DAMAGE_ENERGY | DAMAGE_NOREVEAL;	break;
				case SPELL_Earthquake:		iD1 = DAMAGE_MAGIC | DAMAGE_GENERAL | DAMAGE_NOREVEAL;	break;
				default:					iD1 = DAMAGE_MAGIC | DAMAGE_GENERAL | DAMAGE_NOREVEAL;	break;
			}
		}

		// AOS damage types (used by COMBAT_ELEMENTAL_ENGINE)
		int iDmgPhysical = 0;
		int iDmgFire = 0;
		int iDmgCold = 0;
		int iDmgPoison = 0;
		int iDmgEnergy = 0;
		if ( iD1 & DAMAGE_FIRE )
			iDmgFire = 100;
		else if ( iD1 & DAMAGE_COLD )
			iDmgCold = 100;
		else if ( iD1 & DAMAGE_POISON )
			iDmgPoison = 100;
		else if ( iD1 & DAMAGE_ENERGY )
			iDmgEnergy = 100;
		else
			iDmgPhysical = 100;

		OnTakeDamage(iDmg, pCharSrc, iD1, iDmgPhysical, iDmgFire, iDmgCold, iDmgPoison, iDmgEnergy);
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
			Spell_Effect_Create( spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_STATS, iSkillLevel,
					GetSpellDuration( spell, iSkillLevel, iEffectMult, pCharSrc ), pCharSrc );
			break;

		case SPELL_Heal:
		case SPELL_Great_Heal:
			UpdateStatVal( STAT_STR, GetSpellEffect( spell, iSkillLevel, iEffectMult ) );
			break;

		case SPELL_Night_Sight:
			Spell_Effect_Create( spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Night_Sight, iSkillLevel,
					GetSpellDuration( spell, iSkillLevel, iEffectMult, pCharSrc ), pCharSrc );
			break;

		case SPELL_Reactive_Armor:
			Spell_Effect_Create( spell, LAYER_SPELL_Reactive, iSkillLevel,
					GetSpellDuration( spell, iSkillLevel, iEffectMult, pCharSrc ), pCharSrc );
			break;

		case SPELL_Magic_Reflect:
			Spell_Effect_Create( spell, LAYER_SPELL_Magic_Reflect, iSkillLevel,
					GetSpellDuration( spell, iSkillLevel, iEffectMult, pCharSrc ), pCharSrc );
			break;

		case SPELL_Poison:
		case SPELL_Poison_Field:
			if ( !fPotion )
				Effect(EFFECT_OBJ, iT1, this, 0, 15, fExplode, iColor, iRender);
			SetPoison((pCharSrc->Skill_GetBase(SKILL_MAGERY) + pCharSrc->Skill_GetBase(SKILL_POISONING)) / 2, iSkillLevel / 50, pCharSrc);
			break;

		case SPELL_Cure:
			SetPoisonCure( iSkillLevel, iSkillLevel > 900 );
			break;

		case SPELL_Arch_Cure:
			SetPoisonCure( iSkillLevel, true );
			break;

		case SPELL_Protection:
		case SPELL_Arch_Prot:
			Spell_Effect_Create( spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Protection, iSkillLevel,
				GetSpellDuration( spell, iSkillLevel, iEffectMult, pCharSrc ), pCharSrc );
			break;

		case SPELL_Summon:
			Spell_Effect_Create( spell,	LAYER_SPELL_Summon, iSkillLevel,
				GetSpellDuration( spell, iSkillLevel, iEffectMult, pCharSrc ), pCharSrc );
			break;

		case SPELL_Dispel:
		case SPELL_Mass_Dispel:
			// ??? should be difficult to dispel SPELL_Summon creatures
			Spell_Dispel( (pCharSrc != NULL && pCharSrc->IsPriv(PRIV_GM)) ? 150 : 50);
			break;

		case SPELL_Reveal:
			if ( ! Reveal())
				break;
			Effect(EFFECT_OBJ, iT1, this, 0, 15, fExplode, iColor, iRender);
			break;

		case SPELL_Invis:
			Spell_Effect_Create( spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Invis, iSkillLevel,
				GetSpellDuration( spell, iSkillLevel, iEffectMult, pCharSrc ), pCharSrc );
			break;

		case SPELL_Incognito:
			Spell_Effect_Create( spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Incognito, iSkillLevel,
				GetSpellDuration( spell, iSkillLevel, iEffectMult, pCharSrc ), pCharSrc );
			break;

		case SPELL_Paralyze:
		case SPELL_Paralyze_Field:
		case SPELL_Stone:
		case SPELL_Particle_Form:
			Spell_Effect_Create( spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Paralyze, iSkillLevel,
				GetSpellDuration( spell, iSkillLevel, iEffectMult, pCharSrc ), pCharSrc );
			break;

		case SPELL_Mana_Vamp:
			{
				int iMax = Stat_GetVal(STAT_INT);
				if ( IsSetMagicFlags(MAGICF_OSIFORMULAS) )
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
				UpdateStatVal( STAT_INT, -iSkillLevel );
				pCharSrc->UpdateStatVal( STAT_INT, +iSkillLevel );
			}
			break;

		case SPELL_Meteor_Swarm:
			Effect(EFFECT_BOLT, iT1, pCharSrc, 9, 6, fExplode, iColor, iRender);
			break;
	
		case SPELL_Lightning:
		case SPELL_Chain_Lightning:
			GetTopSector()->LightFlash();
			Effect(EFFECT_LIGHTNING, ITEMID_NOTHING, pCharSrc);
			break;

		case SPELL_Resurrection:
			return Spell_Resurrection( NULL, pCharSrc );

		case SPELL_Light:
			Effect(EFFECT_OBJ, iT1, this, 9, 6, fExplode, iColor, iRender);
			Spell_Effect_Create( spell, fPotion ? LAYER_FLAG_Potion : LAYER_NEWLIGHT, iSkillLevel,
				GetSpellDuration( spell, iSkillLevel, iEffectMult, pCharSrc ), pCharSrc );
			break;

		case SPELL_Hallucination:
			{
				CItem * pItem = Spell_Effect_Create( spell, LAYER_FLAG_Hallucination, iSkillLevel, 10*TICK_PER_SEC, pCharSrc );
				pItem->m_itSpell.m_spellcharges = Calc_GetRandVal(30);
			}
			break;

		case SPELL_Shrink:
			{
				if ( m_pPlayer )
					break;
				if ( fPotion && pSourceItem )
					pSourceItem->Delete();

				CItem * pItem = NPC_Shrink(); // this delete's the char !!!
				if ( pItem )
					pCharSrc->m_Act_Targ = pItem->GetUID();
			}
			break;

		case SPELL_Mana:
			UpdateStatVal( STAT_INT, GetSpellEffect( spell, iSkillLevel, iEffectMult ) );
			break;

		case SPELL_Refresh:
			UpdateStatVal( STAT_DEX, GetSpellEffect( spell, iSkillLevel, iEffectMult ) );
			break;

		case SPELL_Restore:		// increases both your hit points and your stamina.
			UpdateStatVal( STAT_DEX, GetSpellEffect( spell, iSkillLevel, iEffectMult ) );
			UpdateStatVal( STAT_STR, GetSpellEffect( spell, iSkillLevel, iEffectMult ) );
			break;

		case SPELL_Sustenance:		// 105 // serves to fill you up. (Remember, healing rate depends on how well fed you are!)
			Stat_SetVal( STAT_FOOD, Stat_GetAdjusted(STAT_FOOD) );
			break;

		case SPELL_Gender_Swap:		// 110 // permanently changes your gender.
			if ( IsPlayableCharacter())
			{
				CCharBase * pCharDef = Char_GetDef();
				ASSERT(pCharDef);

				if ( IsHuman() )
					SetID( pCharDef->IsFemale() ? CREID_MAN : CREID_WOMAN );
				else if ( IsElf() )
					SetID( pCharDef->IsFemale() ? CREID_ELFMAN : CREID_ELFWOMAN );
				else if ( IsGargoyle() )
					SetID( pCharDef->IsFemale() ? CREID_GARGMAN : CREID_GARGWOMAN );
				m_prev_id = GetID();
				Update();
			}
			break;

		case SPELL_Wraith_Form:
		case SPELL_Horrific_Beast:
		case SPELL_Lich_Form:
		case SPELL_Vampiric_Embrace:
		case SPELL_Stone_Form:
		case SPELL_Reaper_Form:
		case SPELL_Polymorph:
			{
				Spell_Effect_Create(spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Polymorph, iSkillLevel,
					GetSpellDuration(spell, iSkillLevel, iEffectMult, pCharSrc), pCharSrc);
			}
			break;

		case SPELL_Chameleon:		// 106 // makes your skin match the colors of whatever is behind you.
		case SPELL_BeastForm:		// 107 // polymorphs you into an animal for a while.
		case SPELL_Monster_Form:	// 108 // polymorphs you into a monster for a while.
			Spell_Effect_Create( spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Polymorph, iSkillLevel,
				GetSpellDuration( spell, iSkillLevel, iEffectMult, pCharSrc ), pCharSrc );
			break;

		case SPELL_Trance:			// 111 // temporarily increases your meditation skill.
			Spell_Effect_Create( spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_STATS, iSkillLevel,
				GetSpellDuration( spell, iSkillLevel, iEffectMult, pCharSrc ), pCharSrc );
			break;

		case SPELL_Shield:			// 113 // erects a temporary force field around you. Nobody approaching will be able to get within 1 tile of you, though you can move close to them if you wish.
		case SPELL_Steelskin:		// 114 // turns your skin into steel, giving a boost to your AR.
		case SPELL_Stoneskin:		// 115 // turns your skin into stone, giving a boost to your AR.
			Spell_Effect_Create( spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Protection, iSkillLevel,
				GetSpellDuration( spell, iSkillLevel, iEffectMult, pCharSrc ), pCharSrc );
			break;

		case SPELL_Regenerate:		// Set number of charges based on effect level.
			{
				int iDuration = GetSpellDuration( spell, iSkillLevel, iEffectMult, pCharSrc );
				iDuration /= (2*TICK_PER_SEC);
				if ( iDuration <= 0 )
					iDuration = 1;
				CItem * pSpell = Spell_Effect_Create( spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_STATS, iSkillLevel, iDuration, pCharSrc );
				ASSERT(pSpell);
				pSpell->m_itSpell.m_spellcharges = iDuration;
			}
			break;

		case SPELL_Blood_Oath:	// Blood Oath is a pact created between the casted and the target, memory is stored on the caster because one caster can have only 1 enemy, but one target can have the effect from various spells.
			pCharSrc->Spell_Effect_Create(spell, LAYER_SPELL_Blood_Oath, iSkillLevel, GetSpellDuration(spell, iSkillLevel, iEffectMult, pCharSrc) , this);
			break;

		case SPELL_Corpse_Skin:
			Spell_Effect_Create(spell, LAYER_SPELL_Corpse_Skin, iSkillLevel, GetSpellDuration(spell, iSkillLevel, iEffectMult, pCharSrc), pCharSrc);
			break;

		case SPELL_Evil_Omen:
			Spell_Effect_Create(spell, LAYER_SPELL_Evil_Omen, iSkillLevel, GetSpellDuration(spell, iSkillLevel, iEffectMult, pCharSrc), pCharSrc);
			break;

		case SPELL_Mind_Rot:
			Spell_Effect_Create(spell, LAYER_SPELL_Mind_Rot, iSkillLevel, GetSpellDuration(spell, iSkillLevel, iEffectMult, pCharSrc), pCharSrc);
			break;

		case SPELL_Pain_Spike:
			Spell_Effect_Create(spell, LAYER_SPELL_Pain_Spike, iSkillLevel, GetSpellDuration(spell, iSkillLevel, iEffectMult, pCharSrc), pCharSrc);
			break;

		case SPELL_Strangle:
			Spell_Effect_Create(spell, LAYER_SPELL_Strangle, iSkillLevel, GetSpellDuration(spell, iSkillLevel, iEffectMult, pCharSrc), pCharSrc);
			break;

		case SPELL_Curse_Weapon:
			Spell_Effect_Create(spell, LAYER_SPELL_Curse_Weapon, iSkillLevel, GetSpellDuration(spell, iSkillLevel, iEffectMult, pCharSrc), pCharSrc);
			break;

		/*case SPELL_Animate_Dead_AOS:
		case SPELL_Poison_Strike:
		case SPELL_Summon_Familiar:
		case SPELL_Vengeful_Spirit:
		case SPELL_Wither:
		case SPELL_Exorcism:*/
		default:
			break;
	}
	return true;
}



int CChar::GetSpellEffect( SPELL_TYPE spell, int iSkillLevel, int iEffectMult )
{
	ADDTOCALLSTACK("CChar::GetSpellEffect");
	int	iEffect = g_Cfg.GetSpellEffect( spell, iSkillLevel );
	return (iEffect * iEffectMult ) / 1000;
}

// Quick retrieve of the LAYER_TYPE for the given spell
LAYER_TYPE CChar::GetSpellLayer(SPELL_TYPE spell)
{
	ADDTOCALLSTACK("CChar::GetSpellEffect");
	CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
	ASSERT(pSpellDef);
	if (pSpellDef->IsSpellType(SPELLFLAG_POLY))
		return LAYER_SPELL_Polymorph;
	if (pSpellDef->IsSpellType(SPELLFLAG_SUMMON))
		return LAYER_SPELL_Summon;
	switch (spell)
	{
		case SPELL_Cunning:
		case SPELL_Strength:
		case SPELL_Agility:
		case SPELL_Bless:
		case SPELL_Weaken:
		case SPELL_Clumsy:
		case SPELL_Feeblemind:
			return LAYER_SPELL_STATS;			// 32 = Stats effecting spell. These cancel each other out.
		case SPELL_Reactive_Armor:
			return LAYER_SPELL_Reactive;
		case SPELL_Night_Sight:
		case SPELL_Light:
			return LAYER_SPELL_Night_Sight;
		case SPELL_Protection:
		case SPELL_Arch_Prot:
			return LAYER_SPELL_Protection;
		case SPELL_Incognito:
			return LAYER_SPELL_Incognito;
		case SPELL_Magic_Reflect:
			return LAYER_SPELL_Magic_Reflect;
		case SPELL_Paralyze:
		case SPELL_Paralyze_Field:
			return LAYER_SPELL_Paralyze;		// or turned to stone.
		case SPELL_Invis:
		case SPELL_Ethereal_Voyage:
			return LAYER_SPELL_Invis;
		case SPELL_Poison:
		case SPELL_Poison_Field:
			return LAYER_FLAG_Poison;
		default:
			break;
	}
	if (pSpellDef->m_idLayer)
		return pSpellDef->m_idLayer;
	return LAYER_NONE;
}

int CChar::GetSpellDuration( SPELL_TYPE spell, int iSkillLevel, int iEffectMult, CChar * pCharSrc )
{
	ADDTOCALLSTACK("CChar::GetSpellDuration");
	int iDuration = -1;
	if (pCharSrc != NULL && (IsSetMagicFlags(MAGICF_OSIFORMULAS) || spell >= SPELL_Animate_Dead_AOS))
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
				}
				break;

			case SPELL_Fire_Field:
				iDuration = (15 + (pCharSrc->Skill_GetBase(SKILL_MAGERY) / 5)) / 4;
				break;

			case SPELL_Mana_Drain:
				iDuration = 5;
				break;

			case SPELL_Blade_Spirit:
				iDuration = 120;
				break;

			case SPELL_Paralyze:
				iDuration = 7 + (pCharSrc->Skill_GetBase(SKILL_MAGERY) / 50);	// pre-AOS formula
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
		const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
		ASSERT(pSpellDef);
		iDuration = pSpellDef->m_Duration.GetLinear(iSkillLevel) / 10;
	}

	return ((iDuration * iEffectMult) / 1000) * TICK_PER_SEC;
}
