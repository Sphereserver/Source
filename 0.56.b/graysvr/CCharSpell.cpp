//
// CCharSpell.cpp
//

#include "graysvr.h"	// predef header.


void CChar::Spell_Dispel( int iLevel )
{
	ADDTOCALLSTACK("CChar::Spell_Dispel");
	// ARGS: iLevel = 0-100 level of dispel caster.
	// remove all the spells. NOT if caused by objects worn !!!
	// ATTR_MAGIC && ! ATTR_MOVE_NEVER

	CItem* pItemNext;
	CItem* pItem=GetContentHead();
	for ( ; pItem!=NULL; pItem=pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( iLevel <= 100 && pItem->IsAttr(ATTR_MOVE_NEVER))	// we don't lose this.
			continue;
		if ( pItem->GetEquipLayer() == LAYER_NEWLIGHT ||
			( pItem->GetEquipLayer() >= LAYER_SPELL_STATS && pItem->GetEquipLayer() <= LAYER_SPELL_Summon ))
		{
			pItem->Delete();
		}
	}
}

bool CChar::Spell_Teleport( CPointBase ptNew,
	bool fTakePets, bool fCheckAntiMagic,
	ITEMID_TYPE iEffect, SOUND_TYPE iSound )
{
	ADDTOCALLSTACK("CChar::Spell_Teleport");
	// Teleport you to this place.
	// This is sometimes not really a spell at all.
	// ex. ships plank.
	// RETURN: true = it worked.

	if ( ! ptNew.IsCharValid())
		return false;

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
				iCell = m_pPlayer->GetAccount()->m_TagDefs.GetKeyNum("JailCell", true);

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
			iEffect = g_Cfg.m_iSpell_Teleport_Effect_Staff;	// drama
			iSound = g_Cfg.m_iSpell_Teleport_Sound_Staff;
		}
	}
	else if ( fCheckAntiMagic )
	{
		CRegionBase * pArea = NULL;
		if ( IsSetEF( EF_WalkCheck ) )
			pArea = CheckValidMove_New( ptNew, NULL, DIR_QTY, NULL );
		else
			pArea = CheckValidMove( ptNew, NULL );

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
	
	if ( GetTopPoint().IsValidPoint() )	// Guards might have just been created
	{
		if ( fTakePets )
		{
			// Look for any creatures that might be following me near by.
			CWorldSearch Area( GetTopPoint(), UO_MAP_VIEW_SIGHT );
			for (;;)
			{
				CChar * pChar = Area.GetChar();
				if ( pChar == NULL )
					break;
				if ( pChar == this )
					continue;

				// Hostiles ???

				// My pets ?
				if ( pChar->Skill_GetActive() == NPCACT_FOLLOW_TARG &&
					pChar->m_Act_Targ == GetUID())
				{
					CPointMap masterp = GetTopPoint();	// it will modify the val.
					if ( ! pChar->CanMoveWalkTo( masterp, false ))
						continue;
					pChar->Spell_Teleport( ptNew, fTakePets, fCheckAntiMagic, iEffect, iSound );
				}
			}
		}
	}

	CPointMap ptOld = GetTopPoint();

	m_fClimbUpdated = false; // update climb height here

	// move character
	MoveToChar( ptNew );

	// departing effect
	if ( ptOld.IsValidPoint() && iEffect != ITEMID_NOTHING )
	{
		CItem * pItem = CItem::CreateBase( iEffect );
		ASSERT(pItem);
		pItem->SetType( IT_NORMAL );
		pItem->SetAttr( ATTR_MOVE_NEVER );
		pItem->MoveToDecay( ptOld, 2 * TICK_PER_SEC );
	}

	// update other characters
	UpdateMove( ptOld, NULL, true );

	// entering effect
	if ( iEffect != ITEMID_NOTHING )
	{
		Sound( iSound );	// 0x01fe
		Effect( EFFECT_OBJ, iEffect, this, 9, 20 );
	}

	return( true );
}

CChar * CChar::Spell_Summon( CREID_TYPE id, CPointMap pntTarg, bool fSpellSummon )
{
	ADDTOCALLSTACK("CChar::Spell_Summon");
	if ( id == CREID_INVALID )
		return( NULL );

	CChar * pChar;
	if ( ! fSpellSummon )
	{
		// GM creates a char this way. "ADDNPC"
		// More specific npc type from script ?
		pChar = CreateNPC( id );
		if ( pChar == NULL )
			return( NULL );
		pChar->MoveToChar( pntTarg );
	}
	else
	{
		// Check if the target location is valid for summoning this creature
		if ( IsSetMagicFlags( MAGICF_SUMMONWALKCHECK ) && !IsPriv(PRIV_GM))
		{
			WORD wCan = 0xFFFF;
			CCharBase *pSummonDef = CCharBase::FindCharBase(id);
			if (pSummonDef != NULL)
				wCan = pSummonDef->m_Can & CAN_C_MOVEMASK;

			if (wCan != 0xFFFF)
			{
				WORD wBlockFlags = 0;
				if ( IsSetEF( EF_WalkCheck ) )
					g_World.GetHeightPoint_New(pntTarg, wBlockFlags, true);
				else
					g_World.GetHeightPoint(pntTarg, wBlockFlags, true);

				if (wBlockFlags &~ wCan)
				{
					SysMessageDefault(DEFMSG_SUMMON_INVALIDTARG);
					return NULL;
				}
			}
		}

		// These type summons make me it's master. (for a limited time)
		pChar = CChar::CreateBasic( id );
		if ( pChar == NULL )
			return( NULL );
		// Time based on magery. Add the flag first so it does not get loot.
		// conjured creates have no loot. (Mark this early)
		pChar->StatFlag_Set( STATF_Conjured );
		pChar->NPC_LoadScript(false);
		pChar->MoveToChar( pntTarg );
		if ( g_Cfg.m_iMountHeight )
		{
			if ( ! pChar->IsVerticalSpace( GetTopPoint(), false ) )
			{
				SysMessageDefault( DEFMSG_SUMMON_CEILING );
				pChar->Delete();
				return NULL;
			}
		}

#ifdef _ALPHASPHERE_PETS
		if (!IsSetEF(EF_Minimize_Triggers) && IsSetEF(EF_PetSlots))
		{
			// if we cannot control the creature it will not be tame - what might be somewhat risky :)
			CVarDefCont * pTagStorage = pChar->GetKey("FOLLOWERSLOTS", true);
			unsigned short int iFollowerSlotsNeeded = pTagStorage ? ((unsigned short int)pTagStorage->GetValNum()) : 1;
			if ((iFollowerSlotsNeeded + m_pPlayer->m_curFollower) > m_pPlayer->m_maxFollower )
			{
				SysMessage( g_Cfg.GetDefaultMsg(DEFMSG_SUMMON_NO_SLOTS_FREE) );
			} else
			{
				pChar->NPC_PetSetOwner( this );
				// now increase curfollowers property
				m_pPlayer->m_curFollower += iFollowerSlotsNeeded;
				// send an update packet for the stats
				CClient * pClient = this->GetClient();
				if (pClient)
					pClient->addCharStatWindow( this->GetUID() );
			}
		} else {
			pChar->NPC_PetSetOwner( this );
		}

#else
		pChar->NPC_PetSetOwner( this );
#endif
		int skill;
		const CSpellDef *pSpellDef = g_Cfg.GetSpellDef( CChar::IsSkillMagic(m_Act_SkillCurrent)? m_atMagery.m_Spell : SPELL_Summon );
		if (!pSpellDef || !pSpellDef->GetPrimarySkill(&skill, NULL))
		{
			pChar->Delete();
			return( NULL );
		}

		pChar->OnSpellEffect( SPELL_Summon, this, Skill_GetAdjusted( (SKILL_TYPE)skill ), NULL );

		// ASSERT(pChar->m_pNPC);
	}

	m_Act_Targ = pChar->GetUID();	// for last target stuff.
	pChar->Update();
	pChar->UpdateAnimate( ANIM_CAST_DIR );
	pChar->SoundChar( CRESND_GETHIT );
	return( pChar );
}

bool CChar::Spell_Recall( CItem * pRune, bool fGate )
{
	ADDTOCALLSTACK("CChar::Spell_Recall");
	if (pRune) {
		CScriptTriggerArgs Args;
		Args.m_iN1 = fGate ? SPELL_Gate_Travel : SPELL_Recall;

		if (pRune->OnTrigger(ITRIG_SPELLEFFECT, this, &Args) == TRIGRET_RET_FALSE)
			return true;
	}

	if ( pRune == NULL ||
		( ! pRune->IsType(IT_RUNE) && ! pRune->IsType(IT_TELEPAD)))
	{
		SysMessageDefault( DEFMSG_SPELL_RECALL_NOTRUNE );
		return( false );
	}
	if ( ! pRune->m_itRune.m_pntMark.IsValidPoint())
	{
		SysMessageDefault( DEFMSG_SPELL_RECALL_BLANK );
		return( false );
	}
	if ( pRune->IsType(IT_RUNE) && pRune->m_itRune.m_Strength <= 0 )
	{
		SysMessageDefault( DEFMSG_SPELL_RECALL_FADED );
		if ( ! IsPriv(PRIV_GM))
		{
			return( false );
		}
	}

	if ( fGate )
	{
		// Can't even open the gate ?
		CRegionBase * pArea = pRune->m_itRune.m_pntMark.GetRegion( REGION_TYPE_AREA | REGION_TYPE_MULTI | REGION_TYPE_ROOM );
		if ( pArea == NULL )
		{
			return( false );
		}
		if ( pArea->IsFlag( REGION_ANTIMAGIC_ALL | REGION_ANTIMAGIC_GATE | REGION_ANTIMAGIC_RECALL_IN | REGION_ANTIMAGIC_RECALL_OUT | REGION_ANTIMAGIC_RECALL_IN ))
		{
			// Antimagic
			SysMessageDefault( DEFMSG_SPELL_GATE_AM );
			if ( ! IsPriv(PRIV_GM))
			{
				return( false );
			}
		}

		const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(SPELL_Gate_Travel);
		ASSERT(pSpellDef);

		// Color gates to unguarded regions.
		CItem * pGate = CItem::CreateBase( pSpellDef->m_idEffect );
		ASSERT(pGate);
		pGate->SetType( IT_TELEPAD );
		pGate->SetAttr(ATTR_MAGIC|ATTR_MOVE_NEVER|ATTR_CAN_DECAY);	// why is this movable ?
		pGate->m_itTelepad.m_pntMark = pRune->m_itRune.m_pntMark;
		// pGate->m_light_pattern = LIGHT_LARGE;
		pGate->SetHue(pArea->IsGuarded() ? HUE_DEFAULT : HUE_RED );
		int iDuration = pSpellDef->m_Duration.GetLinear( 0 );
		pGate->MoveToDecay( GetTopPoint(), iDuration );
		pGate->m_uidLink = this->GetUID();
		// The open animation.
		pGate->Effect( EFFECT_OBJ, ITEMID_MOONGATE_FX_BLUE, pGate, 2 );

		// Far end gate.
		pGate = CItem::CreateDupeItem( pGate );
		ASSERT(pGate);
		pGate->m_itTelepad.m_pntMark = GetTopPoint();
		pGate->SetHue(( m_pArea && m_pArea->IsGuarded()) ? HUE_DEFAULT : HUE_RED );
		pGate->MoveToDecay( pRune->m_itRune.m_pntMark, iDuration );
		pGate->m_uidLink = this->GetUID();
		pGate->Sound( pSpellDef->m_sound );
		// The open animation.
		pGate->Effect( EFFECT_OBJ, ITEMID_MOONGATE_FX_BLUE, pGate, 2 );
	}
	else
	{
		if ( ! Spell_Teleport( pRune->m_itRune.m_pntMark, false, true, ITEMID_NOTHING ))
			return( false );
	}

	// Wear out the rune.
	if ( pRune->IsType(IT_RUNE))
	{
		if ( ! IsPriv(PRIV_GM))
		{
			pRune->m_itRune.m_Strength --;
		}
		if ( pRune->m_itRune.m_Strength < 10 )
		{
			SysMessageDefault( DEFMSG_SPELL_RECALL_SFADE );
		}
		if ( ! pRune->m_itRune.m_Strength )
		{
			SysMessageDefault( DEFMSG_SPELL_RECALL_FADEC );
		}
	}

	return( true );
}

bool CChar::Spell_Resurrection(CItemCorpse * pCorpse, CChar * pCharSrc, bool bNoFail)
{
	ADDTOCALLSTACK("CChar::Spell_Resurrection");
	if ( ! IsStatFlag( STATF_DEAD ))
		return false;

	if ( !bNoFail )
	{
		if ( pCharSrc && ( pCharSrc->IsPriv(PRIV_GM) ) )
			bNoFail = true;

		if ( IsPriv( PRIV_GM ) )
			bNoFail = true;
	}

	if ( ! bNoFail &&
		m_pArea &&
		m_pArea->IsFlag( REGION_ANTIMAGIC_ALL | REGION_ANTIMAGIC_RECALL_IN | REGION_ANTIMAGIC_TELEPORT ))
	{
		// Could be a house break in.
		// Check to see if it is.
		if ( m_pArea->GetResourceID().IsItem())
		{
			SysMessageDefault( DEFMSG_SPELL_RES_AM );
			return false;
		}
	}

	SetID( m_prev_id );
	StatFlag_Clear( STATF_DEAD | STATF_Insubstantial );
	SetHue( m_prev_Hue );
	Stat_SetVal(STAT_STR, maximum(IMULDIV( Stat_GetMax(STAT_STR), g_Cfg.m_iHitpointPercentOnRez, 100 ),1) );

	if ( m_pPlayer )
	{
		CItem * pRobe = ContentFind( RESOURCE_ID(RES_ITEMDEF,ITEMID_DEATHSHROUD));
		if ( pRobe != NULL )
		{
			pRobe->RemoveFromView();	// For some reason this does not update without remove first.
			pRobe->SetID( ITEMID_ROBE );
			pRobe->SetName( g_Cfg.GetDefaultMsg( DEFMSG_SPELL_RES_ROBENAME ) );
			pRobe->Update();
		}

		if ( pCorpse == NULL )
		{
			pCorpse = FindMyCorpse();
		}

		if ( pCorpse != NULL )
		{
			if ( RaiseCorpse( pCorpse ))
			{
				SysMessageDefault( DEFMSG_SPELL_RES_REJOIN );
				if ( pRobe != NULL )
				{
					pRobe->Delete();
					pRobe = NULL;
				}
			}
		}

		if ( pRobe != NULL )
		{
			if ( g_Cfg.m_fNoResRobe )
			{
				pRobe->Delete();
				pRobe = NULL;
			}
			else
			{
				m_defense = CalcArmorDefense();
			}
		}
	}

	Effect( EFFECT_OBJ, ITEMID_FX_HEAL_EFFECT, this, 9, 14 );
	Update();
	return( true );
}
void CChar::Spell_Effect_Remove(CItem * pSpell)
{
	ADDTOCALLSTACK("CChar::Spell_Effect_Remove");
	// we are removing the spell effect.
	ASSERT(pSpell);

	if ( ! pSpell->IsTypeSpellable())	// can this item have a spell effect ?
		return;
	if ( ! pSpell->m_itSpell.m_spell )
		return;
	if ( pSpell->IsType(IT_WAND))	// equipped wands do not confer effect.
		return;

	// m_itWeapon, m_itArmor, m_itSpell

	SPELL_TYPE spell = (SPELL_TYPE) RES_GET_INDEX( pSpell->m_itSpell.m_spell );

	short iStatEffect = g_Cfg.GetSpellEffect( spell, pSpell->m_itSpell.m_spelllevel );

	switch ( spell )
	{
		case SPELL_Clumsy:
			Stat_AddMod(STAT_DEX, iStatEffect);
			if (IsClient()) {
				GetClient()->removeBuff(BI_CLUMSY);
			}
			break;
		case SPELL_Particle_Form:	// 112 // turns you into an immobile, but untargetable particle system for a while.
		case SPELL_Stone:
			StatFlag_Clear( STATF_Stone );
			UpdateModeFlag();
			break;
		case SPELL_Hallucination:
			StatFlag_Clear( STATF_Hallucinating );
			if ( IsClient() )
			{
				m_pClient->addReSync();
			}
			UpdateModeFlag();
		case SPELL_Feeblemind:
			Stat_AddMod( STAT_INT, iStatEffect );
			if (IsClient()) {
				GetClient()->removeBuff(BI_FEEBLEMIND);
			}
			break;
		case SPELL_Weaken:
			Stat_AddMod( STAT_STR, iStatEffect );
			if (IsClient()) {
				GetClient()->removeBuff(BI_WEAKEN);
			}

			UpdateHitsFlag();
			break;
		case SPELL_Curse:
			{
				for ( int i=STAT_STR; i<STAT_BASE_QTY; i++ )
					Stat_AddMod( (STAT_TYPE) i, iStatEffect );
				if (IsClient()) {
					GetClient()->removeBuff(BI_CURSE);
				}

				UpdateHitsFlag();
			}
			break;
		case SPELL_Agility:
			Stat_AddMod( STAT_DEX, -iStatEffect );
			if (IsClient()) {
				GetClient()->removeBuff(BI_AGILITY);
			}
			break;
		case SPELL_Cunning:
			Stat_AddMod( STAT_INT, -iStatEffect );
			if (IsClient()) {
				GetClient()->removeBuff(BI_CUNNING);
			}
			break;
		case SPELL_Strength:
			Stat_AddMod( STAT_STR, -iStatEffect );
			if (IsClient()) {
				GetClient()->removeBuff(BI_STRENGTH);
			}

			UpdateHitsFlag();
			break;
		case SPELL_Bless:
			{
				for ( int i=STAT_STR; i<STAT_BASE_QTY; i++ )
					Stat_AddMod( (STAT_TYPE) i, -iStatEffect );
				if (IsClient()) {
					GetClient()->removeBuff(BI_BLESS);
				}

				UpdateHitsFlag();
			}
			break;

		case SPELL_Ale:		// 90 = drunkeness ?
		case SPELL_Wine:	// 91 = mild drunkeness ?
		case SPELL_Liquor:	// 92 = extreme drunkeness ?
			{
				for ( int i=STAT_STR; i<STAT_BASE_QTY; i++ )
					Stat_AddMod( (STAT_TYPE) i, iStatEffect );

				UpdateHitsFlag();
			}
			break;

		case SPELL_Reactive_Armor:
			StatFlag_Clear( STATF_Reactive );
			if (IsClient()) {
				GetClient()->removeBuff(BI_REACTIVEARMOR);
			}
			return;
		case SPELL_Night_Sight:
			StatFlag_Clear( STATF_NightSight );
			if ( IsClient())
			{
				m_pClient->addLight();
				if (IsSetOF(OF_Buffs)) {
					GetClient()->removeBuff(BI_NIGHTSIGHT);
				}
			}

			return;
		case SPELL_Magic_Reflect:
			StatFlag_Clear( STATF_Reflection );
			if (IsClient()) {
				GetClient()->removeBuff(BI_MAGICREFLECTION);
			}
			return;
		case SPELL_Poison:
		case SPELL_Poison_Field:
			StatFlag_Clear( STATF_Poisoned );
			UpdateModeFlag();
			if (IsClient()) {
				GetClient()->removeBuff(BI_POISON);
			}
			break;
		case SPELL_Arch_Prot:
			if (IsClient()) {
				GetClient()->removeBuff(BI_ARCHPROTECTION);
			}
			m_defense = CalcArmorDefense();
			break;
		case SPELL_Protection:
			if (IsClient()) {
				GetClient()->removeBuff(BI_PROTECTION);
			}
		case SPELL_Steelskin:		// 114 // turns your skin into steel, giving a boost to your AR.
		case SPELL_Stoneskin:		// 115 // turns your skin into stone, giving a boost to your AR.
			m_defense = CalcArmorDefense();
			break;
		case SPELL_Incognito:
			StatFlag_Clear( STATF_Incognito );
			SetName( pSpell->GetName());	// restore your name
			if ( ! IsStatFlag( STATF_Polymorph ))
			{
				SetHue( m_prev_Hue );
			}
			pSpell->SetName( "" );	// clear the name from the item (might be a worn item)
			if (IsClient()) {
				GetClient()->removeBuff(BI_INCOGNITO);
			}
			break;
		case SPELL_Invis:
			Reveal(STATF_Invisible);
			if (IsClient()) {
				GetClient()->removeBuff(BI_INVISIBILITY);
			}
			return;
		case SPELL_Paralyze:
		case SPELL_Paralyze_Field:
			StatFlag_Clear( STATF_Freeze );
			if (IsClient()) {
				GetClient()->removeBuff(BI_PARALYZE);
			}
			UpdateModeFlag();
			return;


		case SPELL_Chameleon:		// 106 // makes your skin match the colors of whatever is behind you.
			break;
		case SPELL_BeastForm:		// 107 // polymorphs you into an animal for a while.
		case SPELL_Monster_Form:	// 108 // polymorphs you into a monster for a while.
		case SPELL_Polymorph:
			{
				//  m_prev_id != GetID()
				// poly back to orig form.
				SetID( m_prev_id );
				// set back to original stats as well.
				Stat_AddMod( STAT_STR, -pSpell->m_itSpell.m_PolyStr );
				Stat_AddMod( STAT_DEX, -pSpell->m_itSpell.m_PolyDex );
				Stat_SetVal(STAT_STR,  minimum( Stat_GetVal(STAT_STR), Stat_GetMax(STAT_STR) ));
				Stat_SetVal(STAT_DEX,  minimum( Stat_GetVal(STAT_DEX), Stat_GetMax(STAT_DEX) ));
				Update();
				StatFlag_Clear( STATF_Polymorph );
				if (IsClient()) {
					GetClient()->removeBuff(BI_POLYMORPH);
				}
			}
			return;
		case SPELL_Summon:
			// Delete the creature completely.
			// ?? Drop anything it might have had ?
			if ( ! g_Serv.IsLoading())
			{
				const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(SPELL_Teleport);
				ASSERT(pSpellDef);

				CItem * pEffect = CItem::CreateBase( ITEMID_FX_TELE_VANISH );
				ASSERT(pEffect);
				pEffect->SetAttr(ATTR_MAGIC|ATTR_MOVE_NEVER|ATTR_CAN_DECAY); // why is this movable ?
				pEffect->MoveToDecay( GetTopPoint(), 2*TICK_PER_SEC );
				pEffect->Sound( pSpellDef->m_sound  );
			}
			if ( m_pPlayer )	// summoned players ? thats odd.
				return;
			Delete();
			return;
			break;

		case SPELL_Trance:			// 111 // temporarily increases your meditation skill.
			Skill_SetBase( SKILL_MEDITATION, Skill_GetBase( SKILL_MEDITATION ) - iStatEffect );
			break;

		case SPELL_Shield:			// 113 // erects a temporary force field around you. Nobody approaching will be able to get within 1 tile of you, though you can move close to them if you wish.
			break;

		default:
			return;
	}
	UpdateStatsFlag();
}

void CChar::Spell_Effect_Add( CItem * pSpell )
{
	ADDTOCALLSTACK("CChar::Spell_Effect_Add");
	// Attach the spell effect for a duration.
	// Add effects which are saved in the save file here.
	// Not in LayerAdd
	// NOTE: ATTR_MAGIC without ATTR_MOVE_NEVER is dispellable !

	// equipped wands do not confer effect.
	if ( !pSpell || !pSpell->IsTypeSpellable() || pSpell->IsType(IT_WAND) )
		return;

	SPELL_TYPE spell = (SPELL_TYPE) RES_GET_INDEX(pSpell->m_itSpell.m_spell);
	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);

	if ( !pSpellDef || !spell )
		return;

	short iStatEffect = g_Cfg.GetSpellEffect( spell, pSpell->m_itSpell.m_spelllevel );

	//Buffs related variables:
	int iBuffPercent;
	TCHAR NumBuff[3][4];
	LPCTSTR pNumBuff[3] = {(LPCTSTR)NumBuff[0], (LPCTSTR)NumBuff[1], (LPCTSTR)NumBuff[2]};
	//------------------------

	switch ( spell )
	{
		case SPELL_Poison:
		case SPELL_Poison_Field:
			StatFlag_Set( STATF_Poisoned );
			UpdateModeFlag();
			if ( IsSetOF(OF_Buffs) && IsClient() )
			{
				GetClient()->removeBuff( BI_POISON );
				GetClient()->addBuff( BI_POISON,1017383,1070722,2 );
			}
			break;
		case SPELL_Reactive_Armor:
			StatFlag_Set( STATF_Reactive );
			if ( IsSetOF(OF_Buffs) && IsClient() )
			{
				GetClient()->removeBuff( BI_REACTIVEARMOR );
				GetClient()->addBuff( BI_REACTIVEARMOR,1075812,1070722,(WORD)(pSpell->GetTimerAdjusted()) );
			}
			break;
		case SPELL_Night_Sight:
			StatFlag_Set( STATF_NightSight );
			if ( IsClient())
			{
				m_pClient->addLight();
				if (IsSetOF(OF_Buffs))
				{
					GetClient()->removeBuff(BI_NIGHTSIGHT);
					GetClient()->addBuff(BI_NIGHTSIGHT,1075643,1075644,(WORD)(pSpell->GetTimerAdjusted()) );
				}
			}
			break;
		case SPELL_Clumsy:
			Stat_AddMod( STAT_DEX, -iStatEffect );
			if ( IsSetOF(OF_Buffs) && IsClient() )
			{
				GetClient()->removeBuff(BI_CLUMSY);
				iBuffPercent = GetStatPercentage( this, STAT_DEX, iStatEffect );
				ITOA(iBuffPercent, NumBuff[0], 10);
				GetClient()->addBuff(BI_CLUMSY,1075831,1075832,(WORD)(pSpell->GetTimerAdjusted()), pNumBuff, 1);
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
			// NOTE: Allow stats to go negative !
			Stat_AddMod( STAT_INT, -iStatEffect );
			if ( IsSetOF(OF_Buffs) && IsClient() )
			{
				GetClient()->removeBuff(BI_FEEBLEMIND);
				iBuffPercent = GetStatPercentage( this, STAT_INT, iStatEffect );
				ITOA(iBuffPercent, NumBuff[0], 10);
				GetClient()->addBuff(BI_FEEBLEMIND,1075833,1075834,(WORD)(pSpell->GetTimerAdjusted()), pNumBuff, 1);
			}
			break;
		case SPELL_Weaken:
			// NOTE: Allow stats to go negative !
			Stat_AddMod( STAT_STR, -iStatEffect );
			if ( IsSetOF(OF_Buffs) && IsClient() )
			{
				GetClient()->removeBuff(BI_WEAKEN);
				iBuffPercent = GetStatPercentage( this, STAT_STR, iStatEffect );
				ITOA(iBuffPercent, NumBuff[0], 10);
				GetClient()->addBuff(BI_WEAKEN,1075837,1075838,(WORD)(pSpell->GetTimerAdjusted()), pNumBuff, 1);
			}

			UpdateHitsFlag();
			break;
		case SPELL_Curse:
			// NOTE: Allow stats to go negative !
			{
				for ( int i=STAT_STR; i<STAT_BASE_QTY; i++ )
					Stat_AddMod( (STAT_TYPE) i, -iStatEffect );

				if ( IsSetOF(OF_Buffs) && IsClient() )
				{
					GetClient()->removeBuff(BI_CURSE);
					for ( int idx = STAT_STR; idx != STAT_BASE_QTY; ++idx)
					{
						iBuffPercent = GetStatPercentage( this, static_cast<STAT_TYPE>(idx), iStatEffect );
						ITOA(iBuffPercent, NumBuff[idx], 10);
					}

					GetClient()->addBuff(BI_CURSE,1075835,1075840,(WORD)(pSpell->GetTimerAdjusted()), pNumBuff, STAT_BASE_QTY);
				}

				UpdateHitsFlag();
			} break;
		case SPELL_Agility:
			Stat_AddMod( STAT_DEX, +iStatEffect );
			if ( IsSetOF(OF_Buffs) && IsClient() )
			{
				GetClient()->removeBuff(BI_AGILITY);
				iBuffPercent = GetStatPercentage( this, STAT_DEX, iStatEffect );
				ITOA(iBuffPercent, NumBuff[0], 10);
				GetClient()->addBuff(BI_AGILITY,1075841,1075842,(WORD)(pSpell->GetTimerAdjusted()), pNumBuff, 1);
			}
			break;
		case SPELL_Cunning:
			Stat_AddMod( STAT_INT, +iStatEffect );
			if ( IsSetOF(OF_Buffs) && IsClient() )
			{
				GetClient()->removeBuff(BI_CUNNING);
				iBuffPercent = GetStatPercentage( this, STAT_INT, iStatEffect );
				ITOA(iBuffPercent, NumBuff[0], 10);
				GetClient()->addBuff(BI_CUNNING,1075843,1075844,(WORD)(pSpell->GetTimerAdjusted()), pNumBuff, 1);
			}
			break;
		case SPELL_Strength:
			{
				Stat_AddMod( STAT_STR, +iStatEffect );
				if ( IsSetOF(OF_Buffs) && IsClient() )
				{
					GetClient()->removeBuff(BI_STRENGTH);
					iBuffPercent = GetStatPercentage( this, STAT_STR, iStatEffect );
					ITOA(iBuffPercent, NumBuff[0], 10);
					GetClient()->addBuff(BI_STRENGTH,1075845,1075846,(WORD)(pSpell->GetTimerAdjusted()), pNumBuff, 1);
				}

				UpdateHitsFlag();
			} break;
		case SPELL_Bless:
			{
				for ( int i=STAT_STR; i<STAT_BASE_QTY; i++ )
					Stat_AddMod( (STAT_TYPE) i, iStatEffect );

				if ( IsSetOF(OF_Buffs) && IsClient() )
				{
					GetClient()->removeBuff(BI_BLESS);
					for ( int idx = STAT_STR; idx != STAT_BASE_QTY; ++idx)
					{
						iBuffPercent = GetStatPercentage( this, static_cast<STAT_TYPE>(idx), iStatEffect );
						ITOA(iBuffPercent, NumBuff[idx], 10);
					}

					GetClient()->addBuff(BI_BLESS,1075847,1075848,(WORD)(pSpell->GetTimerAdjusted()), pNumBuff, STAT_BASE_QTY);
				}

				UpdateHitsFlag();
			} break;
		case SPELL_Ale:		// 90 = drunkeness ?
		case SPELL_Wine:	// 91 = mild drunkeness ?
		case SPELL_Liquor:	// 92 = extreme drunkeness ?
			{
				// NOTE: Allow stats to go negative !
				int i = STAT_STR;
				for ( i=STAT_STR; i<STAT_BASE_QTY; i++ )
					Stat_AddMod( (STAT_TYPE) i, -iStatEffect );

				UpdateHitsFlag();
			}
			break;
		case SPELL_Incognito:
			if ( ! IsStatFlag( STATF_Incognito ))
			{
				CCharBase * pCharDef = Char_GetDef();
				ASSERT(pCharDef);
				StatFlag_Set( STATF_Incognito );
				pSpell->SetName( GetName());	// Give it my name
				SetName( pCharDef->GetTypeName());	// Give me general name for the type
				if ( ! IsStatFlag( STATF_Polymorph ) && IsHuman())
				{
					SetHue((HUE_UNDERWEAR|HUE_SKIN_LOW) + Calc_GetRandVal(HUE_SKIN_HIGH-HUE_SKIN_LOW));
				}
				if ( IsSetOF(OF_Buffs) && IsClient() )
				{
					GetClient()->removeBuff(BI_INCOGNITO);
					GetClient()->addBuff(BI_INCOGNITO,1075819,1075820,(WORD)(pSpell->GetTimerAdjusted()) );
				}
			}
			break;
		case SPELL_Magic_Reflect:
			StatFlag_Set( STATF_Reflection );
			if ( IsSetOF(OF_Buffs) && IsClient() )
			{
				GetClient()->removeBuff(BI_MAGICREFLECTION);
				GetClient()->addBuff(BI_MAGICREFLECTION,1075817,1070722,(WORD)(pSpell->GetTimerAdjusted()) );
			}
			break;
		case SPELL_Protection:
			if ( IsSetOF(OF_Buffs) && IsClient() )
			{
				GetClient()->removeBuff(BI_PROTECTION);
				GetClient()->addBuff(BI_PROTECTION, 1075814,1070722,(WORD)(pSpell->GetTimerAdjusted()) );
			}
			m_defense = CalcArmorDefense();
			break;
		case SPELL_Arch_Prot:
			if ( IsSetOF(OF_Buffs) && IsClient() )
			{
				GetClient()->removeBuff(BI_ARCHPROTECTION);
				GetClient()->addBuff(BI_ARCHPROTECTION, 1075816,1070722,(WORD)(pSpell->GetTimerAdjusted()) );
			}
		case SPELL_Steelskin:		// 114 // turns your skin into steel, giving a boost to your AR.
		case SPELL_Stoneskin:		// 115 // turns your skin into stone, giving a boost to your AR.

			m_defense = CalcArmorDefense();
			break;
		case SPELL_Invis:
			StatFlag_Set( STATF_Invisible );
			// m_wHue = HUE_TRANSLUCENT;
			UpdateMove(GetTopPoint());	// Some will be seeing us for the first time !
			if ( IsSetOF(OF_Buffs) && IsClient() )
			{
				GetClient()->removeBuff(BI_INVISIBILITY);
				GetClient()->addBuff(BI_INVISIBILITY,1075825,1075826,(WORD)(pSpell->GetTimerAdjusted()) );
			}
			break;
		case SPELL_Paralyze:
		case SPELL_Paralyze_Field:
			StatFlag_Set( STATF_Freeze );
			if ( IsSetOF(OF_Buffs) && IsClient() )
			{
				GetClient()->removeBuff(BI_PARALYZE);
				GetClient()->addBuff(BI_PARALYZE, 1075827,1075828,(WORD)(pSpell->GetTimerAdjusted()) );
			}
			UpdateModeFlag();
			break;
		case SPELL_Polymorph:
			StatFlag_Set( STATF_Polymorph );
			if ( IsSetOF(OF_Buffs) && IsClient() )
			{
				GetClient()->removeBuff(BI_POLYMORPH);
				GetClient()->addBuff(BI_POLYMORPH, 1075824,1070722,(WORD)(pSpell->GetTimerAdjusted()) );
			}
			break;
		case SPELL_Summon:
			// LAYER_SPELL_Summon
			StatFlag_Set( STATF_Conjured );
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

	SPELL_TYPE spell = (SPELL_TYPE)	RES_GET_INDEX(pItem->m_itSpell.m_spell);

	int iCharges = pItem->m_itSpell.m_spellcharges;
	int iLevel = pItem->m_itSpell.m_spelllevel;

	static LPCTSTR const sm_Poison_Message[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_SPELL_POISON_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_SPELL_POISON_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_SPELL_POISON_3 ),
		g_Cfg.GetDefaultMsg( DEFMSG_SPELL_POISON_4 )
	};
	static const int sm_iPoisonMax[] = { 2, 4, 6, 8 };

	switch ( spell )
	{
	case SPELL_Ale:		// 90 = drunkeness ?
	case SPELL_Wine:	// 91 = mild drunkeness ?
	case SPELL_Liquor:	// 92 = extreme drunkeness ?
		// Degrades over time.
		// LAYER_FLAG_Drunk

		if ( iCharges <= 0 || iLevel <= 0 )
		{
			return( false );
		}
		if ( iLevel > 50 )
		{
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_SPELL_ALCOHOL_HIC ) );
			if ( iLevel > 200 && Calc_GetRandVal(2))
			{
				UpdateAnimate( ANIM_BOW );
			}
		}
		if ( Calc_GetRandVal(2))
		{
			Spell_Effect_Remove(pItem);
			pItem->m_itSpell.m_spelllevel-=10;	// weaken the effect.
			Spell_Effect_Add( pItem );
		}

		// We will have this effect again.
		pItem->SetTimeout( Calc_GetRandVal(10)*TICK_PER_SEC );
		break;

	case SPELL_Regenerate:
		if ( iCharges <= 0 || iLevel <= 0 )
		{
			return( false );
		}

		// Gain HP.
		UpdateStatVal( STAT_STR, g_Cfg.GetSpellEffect( spell, iLevel ));
		pItem->SetTimeout( 2*TICK_PER_SEC );
		break;

	case SPELL_Hallucination:

		if ( iCharges <= 0 || iLevel <= 0 )
		{
			return( false );
		}
		if ( IsClient())
		{
			static const SOUND_TYPE sm_sounds[] = { 0x243, 0x244, 0x245 };
			m_pClient->addSound( sm_sounds[ Calc_GetRandVal( COUNTOF( sm_sounds )) ] );
			m_pClient->addReSync();
		}
		// save the effect.
		pItem->SetTimeout( (15+Calc_GetRandVal(15))*TICK_PER_SEC );
		break;

	case SPELL_Poison:
		// Both potions and poison spells use this.
		// m_itSpell.m_spelllevel = strength of the poison ! 0-1000

		if ( iCharges <= 0 || iLevel < 50 )
			return( false );

		// The poison in your body is having an effect.

		if ( iLevel < 200 )	// Lesser
			iLevel = 0;
		else if ( iLevel < 400 ) // Normal
			iLevel = 1;
		else if ( iLevel < 800 ) // Greater
			iLevel = 2;
		else	// Deadly.
			iLevel = 3;

		{
			TCHAR	*pszMsg = Str_GetTemp();
			sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_SPELL_LOOKS ), (LPCTSTR) sm_Poison_Message[iLevel]);
			Emote(pszMsg, GetClient());
			SysMessagef(g_Cfg.GetDefaultMsg( DEFMSG_SPELL_YOUFEEL ), (LPCTSTR) sm_Poison_Message[iLevel]);

			int iDmg = IMULDIV( Stat_GetMax(STAT_STR), iLevel * 2, 100 );
			OnTakeDamage( maximum( sm_iPoisonMax[iLevel], iDmg ), pItem->m_uidLink.CharFind(), DAMAGE_POISON | DAMAGE_GENERAL | DAMAGE_NOREVEAL );
		}

		pItem->m_itSpell.m_spelllevel -= 50;	// gets weaker too.

		// g_Cfg.GetSpellEffect( SPELL_Poison,

		// We will have this effect again.
		pItem->SetTimeout((5+Calc_GetRandVal(4))*TICK_PER_SEC);

		if (IsClient() && IsSetOF(OF_Buffs))
		{
			GetClient()->removeBuff( BI_POISON );
			GetClient()->addBuff( BI_POISON,1017383,1070722,(WORD)(pItem->GetTimerAdjusted()) );
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

CItem * CChar::Spell_Effect_Create( SPELL_TYPE spell, LAYER_TYPE layer, int iSkillLevel, int iDuration, CObjBase * pSrc )
{
	ADDTOCALLSTACK("CChar::Spell_Effect_Create");
	// Attach an effect to the Character.
	//
	// ARGS:
	// spell = SPELL_Invis, etc.
	// layer == LAYER_FLAG_Potion, etc.
	// iSkillLevel = 0-1000 = skill level or other spell specific value.
	// iDuration = TICK_PER_SEC
	//
	// NOTE:
	//   ATTR_MAGIC without ATTR_MOVE_NEVER is dispellable !

	CItem * pSpell;
	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);

	pSpell = CItem::CreateBase( pSpellDef ? ( pSpellDef->m_idSpell ) : ITEMID_RHAND_POINT_NW );
	ASSERT(pSpell);

//	if (( IsSetMagicFlags( MAGICF_STACKSTATS ) && (layer == LAYER_SPELL_STATS) ) || (layer == LAYER_NONE))
//		layer = pSpell->Item_GetDef()->GetEquipLayer();

	pSpell->SetAttr(ATTR_NEWBIE);
	if ( pSpellDef )
	{
		pSpell->SetAttr(ATTR_MAGIC);
	}
	pSpell->SetType(IT_SPELL);
	pSpell->m_itSpell.m_spell = spell;
	pSpell->m_itSpell.m_spelllevel = iSkillLevel;	// 0 - 1000
	pSpell->m_itSpell.m_spellcharges = 1;

	if ( iDuration <= 0 )	// use default script duration.
	{
		if (pSpellDef != NULL) // may not be a valid spell
			iDuration = pSpellDef->m_Duration.GetLinear( iSkillLevel );
		if ( iDuration <= 0 )
			iDuration = 1;
	}
	pSpell->SetDecayTime( iDuration );

	if ( pSrc )
	{
		pSpell->m_uidLink = pSrc->GetUID();
	}

	LayerAdd( pSpell, layer );	// Remove any competing effect first.
	Spell_Effect_Add( pSpell );
	return( pSpell );
}

void CChar::Spell_Bolt( CObjBase * pObjTarg, ITEMID_TYPE idBolt, int iSkillLevel )
{
	ADDTOCALLSTACK("CChar::Spell_Bolt");
	// I am casting a bolt spell.
	// ARGS:
	// iSkillLevel = 0-1000
	//

	if ( pObjTarg == NULL )
		return;

	bool fExplode = true;
	if ( g_Cfg.GetSpellDef(m_atMagery.m_Spell)->IsSpellType(SPELLFLAG_GOOD) )
		fExplode = false;

	pObjTarg->Effect( EFFECT_BOLT, idBolt, this, 5, 1, fExplode );
	// Take damage !
	pObjTarg->OnSpellEffect( m_atMagery.m_Spell, this, iSkillLevel, NULL );
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
		if ( !IsSetMagicFlags( MAGICF_CANHARMSELF ) && pChar == this ) // not harm the caster.
		{
			if ( pSpellDef->IsSpellType( SPELLFLAG_HARM ))
				continue;
		}
		pChar->OnSpellEffect( spelltype, this, iSkillLevel, NULL );
	}
	CWorldSearch AreaItem( pntTarg, iDist );
	for (;;)
	{
		CItem * pItem = AreaItem.GetItem();
		if ( pItem == NULL )
			break;
		pItem->OnSpellEffect( spelltype, this, iSkillLevel, NULL );
	}
}

void CChar::Spell_Field( CPointMap pntTarg, ITEMID_TYPE idEW, ITEMID_TYPE idNS, unsigned int fieldWidth, unsigned int fieldGauge, int iSkillLevel )
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

	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_Spell);
	ASSERT(pSpellDef);

	if ( m_pArea && m_pArea->IsGuarded() && pSpellDef->IsSpellType(SPELLFLAG_HARM) )
		Noto_Criminal();

	// get the dir of the field.
	int dx = abs( pntTarg.m_x - GetTopPoint().m_x );
	int dy = abs( pntTarg.m_y - GetTopPoint().m_y );
	ITEMID_TYPE id = ( dx > dy ) ? idNS : idEW;

	int minX = (int)((fieldWidth-1)/2) - (fieldWidth-1);
	int maxX = minX+(fieldWidth-1);

	int minY = (int)((fieldGauge-1)/2) - (fieldGauge-1);
	int maxY = minY+(fieldGauge-1);

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
					ptg.m_y += ix;
					ptg.m_x += iy;
				}
				else
				{
					ptg.m_x += ix;
					ptg.m_y += iy;
				}

				WORD wBlockFlags = 0;
				g_World.GetHeightPoint(ptg, wBlockFlags, true);
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

	for ( int ix=minX; ix<=maxX; ix++ )
	{
		for ( int iy=minY; iy<=maxY; iy++) 
		{
			bool fGoodLoc = true;

			// Where is this ?
			CPointMap ptg = pntTarg;
			if ( dx > dy ) {
				ptg.m_y += ix;
				ptg.m_x += iy;
			}
			else {
				ptg.m_x += ix;
				ptg.m_y += iy;
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

			// Check for direct cast on an item.
			CWorldSearch AreaItem( ptg );
			for (;;)
			{
				CItem * pItem = AreaItem.GetItem();
				if ( pItem == NULL )
					break;
				pItem->OnSpellEffect( m_atMagery.m_Spell, this, iSkillLevel, NULL );
			}

			if ( fGoodLoc)
			{
				CItem * pSpell = CItem::CreateScript( id, this );
				ASSERT(pSpell);
				pSpell->SetType(IT_SPELL);
				pSpell->SetAttr(ATTR_MAGIC);
				pSpell->m_itSpell.m_spell = m_atMagery.m_Spell;
				pSpell->m_itSpell.m_spelllevel = iSkillLevel;
				pSpell->m_itSpell.m_spellcharges = 1;
				pSpell->m_uidLink = GetUID();	// Link it back to you

				// Add some random element.
				int iDuration = pSpellDef->m_Duration.GetLinear(iSkillLevel);

				pSpell->MoveToDecay( ptg, iDuration + Calc_GetRandVal( iDuration/2 ));
			}
		}
	}
}

bool CChar::Spell_CanCast( SPELL_TYPE spell, bool fTest, CObjBase * pSrc, bool fFailMsg, bool fCheckAntiMagic )
{
	ADDTOCALLSTACK("CChar::Spell_CanCast");
	// ARGS:
	//  pSrc = possible scroll or wand source.
	// Do we have enough mana to start ?
	if ( spell <= SPELL_NONE ||
		pSrc == NULL )
		return( false );

	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
	if ( pSpellDef == NULL )
		return( false );
	if ( pSpellDef->IsSpellType( SPELLFLAG_DISABLED ))
		return( false );

	// if ( ! fTest || m_pNPC )
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

	int wManaUse = pSpellDef->m_wManaUse;

	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		CScriptTriggerArgs		Args( spell, wManaUse, pSrc );
		if ( fTest )
			Args.m_iN3	|= 0x0001;
		if ( fFailMsg )
			Args.m_iN3	|= 0x0002;

		TRIGRET_TYPE	iRet	= Spell_OnTrigger( spell, SPTRIG_SELECT, this, &Args );
		if ( iRet == TRIGRET_RET_TRUE )
		{
			//DEBUG_ERR(( "@SELECT returned TRUE (%d)" , iRet ));
			return false;
		}
		if ( iRet == TRIGRET_RET_FALSE )
		{
			//DEBUG_ERR(( "@SELECT returned FALSE (%d)" , iRet ));
			return true;
		}
		if ( iRet == TRIGRET_RET_HALFBAKED )		// just for compatibility with @SPELLSELECT
		{
			//DEBUG_ERR(( "@SELECT returned TRIGRET_RET_HALFBAKED (%d)" , iRet ));
			return true;
		}

		iRet = OnTrigger(CTRIG_SpellSelect, this, &Args );
		if ( iRet == TRIGRET_RET_TRUE )
		{
			//DEBUG_ERR(( "@SPELLSELECT returned TRUE (%d)" , iRet ));
			return false;
		}
		if ( iRet == TRIGRET_RET_HALFBAKED )
		{
			//DEBUG_ERR(( "@SPELLSELECT returned TRIGRET_RET_HALFBAKED (%d)" , iRet ));
			return true;
		}

		//DEBUG_ERR(( "@SPELLSELECT returned SOMETHING (%d)" , iRet ));

		if ( spell != Args.m_iN1 )
		{
			pSpellDef = g_Cfg.GetSpellDef(spell);
			if ( pSpellDef == NULL )
				return( false );
			spell		= (SPELL_TYPE) Args.m_iN1;
		}
		wManaUse	= Args.m_iN2;
	}

	// The magic item must be on your person to use.
	if ( pSrc != this )
	{
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
		if ( pObjTop != this )
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
				// ??? May explode !!
				if ( fFailMsg )
					SysMessageDefault( DEFMSG_SPELL_WAND_NOCHARGE );
				return false;
			}
			wManaUse = 0;	// magic items need no mana.
			if ( ! fTest && pItem->m_itWeapon.m_spellcharges != 255 )
			{
				pItem->m_itWeapon.m_spellcharges --;
				pItem->UpdatePropertyFlag(AUTOTOOLTIP_FLAG_WANDCHARGES);
			}
		}
		else	// Scroll
		{
			wManaUse /= 2;
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
			if ( IsStatFlag( STATF_DEAD|STATF_Sleeping ) ||
				! pSpellDef->m_SkillReq.IsResourceMatchAll(this))
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
		}
	}

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

	if ( m_pNPC ||	// NPC's don't need regs.
		pSrc != this )	// wands and scrolls have there own reags source.
		return( true );

	// Check for regs ?
	if ( g_Cfg.m_fReagentsRequired )
	{
		CItemContainer * pPack = GetPack();
		if ( pPack )
		{
			const CResourceQtyArray * pRegs = &(pSpellDef->m_Reags);
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
		else
		{
			if( fFailMsg )
			{
				SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_SPELL_TRY_NOREGS), g_Cfg.GetDefaultMsg(DEFMSG_SPELL_TRY_THEREG));
			}
			return false;
		}
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
	if (	pSpellDef->IsSpellType( SPELLFLAG_TARG_OBJ )
		&& !( !pObj && pSpellDef->IsSpellType( SPELLFLAG_TARG_XYZ ) ) )
	{
		if ( pObj == NULL )
		{
			SysMessageDefault( DEFMSG_SPELL_TARG_OBJ );
			return( false );
		}
		if ( !CanSee(pObj) || !CanSeeLOS(pObj, LOS_NB_WINDOWS) ) //we should be able to cast through a window
		{
			SysMessageDefault(DEFMSG_SPELL_TARG_LOS);
			return false;
		}
		if ( ! IsPriv( PRIV_GM ) && pObjTop != pObj && pObjTop->IsChar() && pObjTop != this )
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
		if ( ! CanMove( pItemPrev ))
			return( false );
		if ( ! pItemPrev->IsTypeSpellbook() && ! pItemPrev->IsType(IT_WAND))
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

	if ( ! Spell_TargCheck())
		return( false );

	CObjBase * pObj		= m_Act_Targ.ObjFind();	// dont always need a target.
	CObjBase * pObjSrc	= m_Act_TargPrv.ObjFind();
    ITEMID_TYPE iT1 = ITEMID_NOTHING;
    ITEMID_TYPE iT2 = ITEMID_NOTHING;
	CREID_TYPE iC1 = CREID_INVALID;

	unsigned int fieldWidth = 0;
	unsigned int fieldGauge = 0;
	unsigned int areaRadius = 0;

	SPELL_TYPE spell = m_atMagery.m_Spell;
	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
	if ( pSpellDef == NULL )
		return( false );

	int iSkill, iDifficulty;
	if (!pSpellDef->GetPrimarySkill(&iSkill, &iDifficulty))
		return( false );

	int iSkillLevel;
	if ( pObjSrc != this )
	{
		// Get the strength of the item. IT_SCROLL or IT_WAND
		CItem * pItem = dynamic_cast <CItem*>(pObjSrc);
		if ( pItem == NULL )
			return( false );
		if ( ! pItem->m_itWeapon.m_spelllevel )
			iSkillLevel = Calc_GetRandVal( 500 );
		else
			iSkillLevel = pItem->m_itWeapon.m_spelllevel;
	}
	else
	{
		iSkillLevel = Skill_GetAdjusted( (SKILL_TYPE)iSkill );
	}

	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		CScriptTriggerArgs	Args( spell, iSkillLevel, pObjSrc );
		Args.m_VarsLocal.SetNum("CreateObject1",0);
		Args.m_VarsLocal.SetNum("CreateObject2",0);

		Args.m_VarsLocal.SetNum("fieldWidth",0);
		Args.m_VarsLocal.SetNum("fieldGauge",0);
		Args.m_VarsLocal.SetNum("areaRadius",0);

		if ( OnTrigger( CTRIG_SpellSuccess, this, &Args ) == TRIGRET_RET_TRUE )
			return false;

		if ( Spell_OnTrigger( spell, SPTRIG_SUCCESS, this, &Args ) == TRIGRET_RET_TRUE )
			return false;

		iSkillLevel		= Args.m_iN2;
		
		iT1 = (ITEMID_TYPE) RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("CreateObject1",true));
		iT2 = (ITEMID_TYPE) RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("CreateObject2",true));
		iC1 = (CREID_TYPE)  (Args.m_VarsLocal.GetKeyNum("CreateObject1",true) & 0xFFFF);

		//Can't be < 0, so max it to 0
		fieldWidth = maximum(0,Args.m_VarsLocal.GetKeyNum("fieldWidth",true));
		fieldGauge = maximum(0,Args.m_VarsLocal.GetKeyNum("fieldGauge",true));
		areaRadius = maximum(0,Args.m_VarsLocal.GetKeyNum("areaRadius",true));

		// DEBUG_ERR(( "1: iT1=%d, iT2=%d, iC1=%d\n", iT2, iT2, iC1 ));
		// DEBUG_ERR(( "2: iT1=%s, iT2=%s, iC1=%s\n", Args.m_VarsLocal.GetKeyStr("CreateObject1",true), Args.m_VarsLocal.GetKeyStr("CreateObject2",true), Args.m_VarsLocal.GetKeyStr("CreateObject1") ));
	}

	// Consume the reagents/mana/scroll/charge
	if ( ! Spell_CanCast( spell, false, pObjSrc, true ) )
		return( false );

	if ( pSpellDef->IsSpellType(SPELLFLAG_SCRIPTED) )
	{
		if ( pSpellDef->IsSpellType(SPELLFLAG_SUMMON) )
		{
			if ( iC1 )
			{
				m_atMagery.m_SummonID = iC1;
				m_atMagery.m_fSummonPet = true;
				Spell_Summon( m_atMagery.m_SummonID, m_Act_p, m_atMagery.m_fSummonPet != 0 );
			}
		} 
		else if ( pSpellDef->IsSpellType(SPELLFLAG_FIELD) )
		{
			if ( iT1 && iT2 )
			{
				if ( !fieldWidth )
					fieldWidth = 7;
				if ( !fieldGauge )
					fieldGauge = 1;

				Spell_Field( m_Act_p, iT1, iT2, fieldWidth, fieldGauge, iSkillLevel );
			}
		}
		else if ( pSpellDef->IsSpellType( SPELLFLAG_AREA ) )
		{
			if ( !areaRadius )
				areaRadius = 4;

			Spell_Area( m_Act_p, areaRadius, iSkillLevel);
		}
		else if ( pSpellDef->IsSpellType(SPELLFLAG_FX_BOLT) )
		{
			Spell_Bolt( pObj, pSpellDef->m_idEffect, iSkillLevel );
		}
		else
		{
			if ( pObj )
			{
				pObj->OnSpellEffect( spell, this, iSkillLevel, dynamic_cast <CItem*>( pObjSrc ));
			}
		}
	}
	else
	switch ( spell )
	{
	
	// Magery
	// 1st Circle
	case SPELL_Create_Food:
		if ( !pObj )
		{
			RESOURCE_ID food = g_Cfg.ResourceGetIDType( RES_ITEMDEF, "DEFFOOD" );
			CItem * pItem = CItem::CreateScript( (ITEMID_TYPE) food.GetResIndex(), this );
			pItem->MoveToCheck( m_Act_p, this );
		}
		break;

	case SPELL_Magic_Arrow:
		Spell_Bolt( pObj, ITEMID_FX_MAGIC_ARROW, iSkillLevel );
		break;

	case SPELL_Heal:
	case SPELL_Night_Sight:
	case SPELL_Reactive_Armor:

	case SPELL_Clumsy:
	case SPELL_Feeblemind:
	case SPELL_Weaken:
		if ( ! Spell_SimpleEffect( pObj, pObjSrc, spell, iSkillLevel ) )
			return( false );
		break;

	// 2nd Circle
	case SPELL_Agility:
	case SPELL_Cunning:
	case SPELL_Cure:
	case SPELL_Protection:
	case SPELL_Strength:

	case SPELL_Harm:
		if ( ! Spell_SimpleEffect( pObj, pObjSrc, spell, iSkillLevel ) )
			return( false );
		break;

	case SPELL_Magic_Trap:
	case SPELL_Magic_Untrap:
		// Create the trap object and link it to the target. ???
		// A container is diff from door or stationary object
		break;

	// 3rd Circle
	case SPELL_Bless:

	case SPELL_Poison:
		if ( ! Spell_SimpleEffect( pObj, pObjSrc, spell, iSkillLevel ) )
			return( false );
		break;
	case SPELL_Fireball:
		Spell_Bolt( pObj, ITEMID_FX_FIRE_BALL, iSkillLevel );
		break;

	case SPELL_Magic_Lock:
	case SPELL_Unlock:
		if ( ! Spell_SimpleEffect( pObj, pObjSrc, spell, iSkillLevel ) )
			return( false );
		break;

	case SPELL_Telekin:
		// Act as dclick on the object.
		Use_Obj( pObj, false );
		break;
	case SPELL_Teleport:
		Spell_Teleport( m_Act_p );
		break;
	case SPELL_Wall_of_Stone:

		if ( ! iT1 )
			iT1 = ITEMID_STONE_WALL;
		if ( ! iT2 )
			iT2 = ITEMID_STONE_WALL;

		if ( !fieldWidth )
			fieldWidth = 7;
		if ( !fieldGauge )
			fieldGauge = 1;

		Spell_Field( m_Act_p, iT1, iT2, fieldWidth, fieldGauge, iSkillLevel );

		break;

	// 4th Circle
	case SPELL_Arch_Cure:
	case SPELL_Arch_Prot:
	{
		if ( !areaRadius )
			areaRadius = 5;
		Spell_Area( m_Act_p, areaRadius, iSkillLevel );
		break;
	}

	case SPELL_Great_Heal:
	case SPELL_Curse:
	case SPELL_Lightning:
		if ( ! Spell_SimpleEffect( pObj, pObjSrc, spell, iSkillLevel ) )
			return( false );
		break;
	case SPELL_Fire_Field:
	{
		if ( ! iT1 )
			iT1 = ITEMID_FX_FIRE_F_EW;
		if ( ! iT2 )
			iT2 = ITEMID_FX_FIRE_F_NS;

		if ( !fieldWidth )
			fieldWidth = 7;
		if ( !fieldGauge )
			fieldGauge = 1;

		Spell_Field( m_Act_p, iT1, iT2, fieldWidth, fieldGauge, iSkillLevel );
		break;
	}
	case SPELL_Recall:
		if ( ! Spell_Recall( dynamic_cast <CItem*> (pObj), false ))
			return( false );
		break;

	// 5th Circle

	case SPELL_Blade_Spirit:
		if ( ! iC1 )
			iC1 = CREID_BLADES;
		m_atMagery.m_SummonID = iC1;
		m_atMagery.m_fSummonPet = true;
		Spell_Summon( m_atMagery.m_SummonID, m_Act_p, m_atMagery.m_fSummonPet != 0 );
		break;

	case SPELL_Dispel_Field:
		{
			CItem * pItem = dynamic_cast <CItem*> (pObj);
			if ( pItem == NULL ||
				pItem->IsAttr( ATTR_MOVE_NEVER ) ||
				( ! pItem->IsType(IT_CAMPFIRE) &&
				! pItem->IsType(IT_SPELL) &&
				! pItem->IsType(IT_FIRE)))
			{
				SysMessageDefault( DEFMSG_SPELL_DISPELLF_WT );
				return( false );
			}
			pItem->OnSpellEffect( SPELL_Dispel_Field, this, iSkillLevel, NULL );
			break;
		}

	case SPELL_Mind_Blast:
		if ( pObj->IsChar())
		{
			CChar * pChar = dynamic_cast <CChar*> ( pObj );
			ASSERT( pChar );
			int iDiff = ( Stat_GetAdjusted(STAT_INT) - pChar->Stat_GetAdjusted(STAT_INT) ) / 2;
			if ( iDiff < 0 )
			{
				pChar = this;	// spell revereses !
				iDiff = -iDiff;
			}
			int iMax = pChar->Stat_GetMax(STAT_STR) / 2;
			pChar->OnSpellEffect( spell, this, minimum( iDiff, iMax ), NULL );
		}
		break;

	case SPELL_Magic_Reflect:

	case SPELL_Paralyze:
	case SPELL_Incognito:
		if ( ! Spell_SimpleEffect( pObj, pObjSrc, spell, iSkillLevel ) )
			return( false );
		break;

	case SPELL_Poison_Field:

		if ( ! iT1 )
			iT1 = (ITEMID_TYPE) 0x3915;
		if ( ! iT2 )
			iT2 = ITEMID_FX_POISON_F_NS;

		if ( !fieldWidth )
			fieldWidth = 7;
		if ( !fieldGauge )
			fieldGauge = 1;

		Spell_Field( m_Act_p, iT1, iT2, fieldWidth, fieldGauge, iSkillLevel );
		break;

	case SPELL_Summon:
 		if ( iC1 )
			m_atMagery.m_SummonID = iC1;
		Spell_Summon( m_atMagery.m_SummonID, m_Act_p, m_atMagery.m_fSummonPet != 0 );
		break;

	// 6th Circle

	case SPELL_Invis:

	case SPELL_Dispel:
		if ( ! Spell_SimpleEffect( pObj, pObjSrc, spell, iSkillLevel ) )
			return( false );
		break;

	case SPELL_Energy_Bolt:
		Spell_Bolt( pObj, ITEMID_FX_ENERGY_BOLT, iSkillLevel );
		break;

	case SPELL_Explosion:
		if ( !areaRadius )
			areaRadius = 2;
		Spell_Area( m_Act_p, areaRadius, iSkillLevel );
		break;

	case SPELL_Mark:
		if ( ! Spell_SimpleEffect( pObj, pObjSrc, spell, iSkillLevel ) )
			return( false );
		break;

	case SPELL_Mass_Curse:
		if ( !areaRadius )
			areaRadius = 5;
		Spell_Area( m_Act_p, areaRadius, iSkillLevel );
		break;

	case SPELL_Paralyze_Field:

		if ( ! iT1 )
			iT1 = ITEMID_FX_PARA_F_EW;
		if ( ! iT2 )
			iT2 = ITEMID_FX_PARA_F_NS;

		if ( !fieldWidth )
			fieldWidth = 7;
		if ( !fieldGauge )
			fieldGauge = 1;

		Spell_Field( m_Act_p, iT1, iT2, fieldWidth, fieldGauge, iSkillLevel );
		break;

	case SPELL_Reveal:
		if ( !areaRadius )
			areaRadius = UO_MAP_VIEW_SIGHT;
		Spell_Area( m_Act_p, areaRadius, iSkillLevel );
		break;

	// 7th Circle
	case SPELL_Chain_Lightning:
		if ( !areaRadius )
			areaRadius = 5;
		Spell_Area( m_Act_p, areaRadius, iSkillLevel );
		break;

	case SPELL_Energy_Field:

		if ( ! iT1 )
			iT1 = ITEMID_FX_ENERGY_F_EW;
		if ( ! iT2 )
			iT2 = ITEMID_FX_ENERGY_F_NS;

		if ( !fieldWidth )
			fieldWidth = 7;
		if ( !fieldGauge )
			fieldGauge = 1;

		Spell_Field( m_Act_p, iT1, iT2, fieldWidth, fieldGauge, iSkillLevel );
		break;

	case SPELL_Flame_Strike:
		// Display spell.
		if ( pObj == NULL )
		{
			CItem * pItem = CItem::CreateBase( ITEMID_FX_FLAMESTRIKE );
			ASSERT(pItem);
			pItem->SetType( IT_SPELL );
			pItem->m_itSpell.m_spell = SPELL_Flame_Strike;
			pItem->MoveToDecay( m_Act_p, 2*TICK_PER_SEC );
		}
		else
		{
			pObj->Effect( EFFECT_OBJ, ITEMID_FX_FLAMESTRIKE, pObj, 6, 15 );
			// Burn person at location.
			if ( ! Spell_SimpleEffect( pObj, pObjSrc, spell, iSkillLevel ) )
				return( false );
		}
		break;

	case SPELL_Gate_Travel:
		if ( ! Spell_Recall( dynamic_cast <CItem*> (pObj), true ))
			return( false );
		break;

	case SPELL_Mana_Drain:
	case SPELL_Mana_Vamp:
		// Take the mana from the target.
		if ( pObj->IsChar() && this != pObj )
		{
			CChar * pChar = dynamic_cast <CChar*> ( pObj );
			ASSERT( pChar );
			if ( ! pChar->IsStatFlag( STATF_Reflection ))
			{
				int iMax = pChar->Stat_GetAdjusted(STAT_INT);
				int iDiff = Stat_GetAdjusted(STAT_INT) - iMax;
				if ( iDiff < 0 )
					iDiff = 0;
				else
					iDiff = Calc_GetRandVal( iDiff );
				iDiff += Calc_GetRandVal( 25 );
				if ( pChar->OnSpellEffect( spell, this, iDiff, NULL ) == false )
					break;

				if ( spell == SPELL_Mana_Vamp )
				{
					// Give some back to me.
					UpdateStatVal( STAT_INT, minimum( iDiff, Stat_GetAdjusted(STAT_INT)));
				}
				break;
			}
		}
		if ( ! Spell_SimpleEffect( pObj, pObjSrc, spell, iSkillLevel ) )
			return( false );
		break;

	case SPELL_Mass_Dispel:
		if ( !areaRadius )
			areaRadius = 15;
		Spell_Area( m_Act_p, areaRadius, iSkillLevel );
		break;

	case SPELL_Meteor_Swarm:
		// Multi explosion ??? 0x36b0
		if ( !areaRadius )
			areaRadius = 4;
		Spell_Area( m_Act_p, areaRadius, iSkillLevel );
		break;

	case SPELL_Polymorph:
		// This has a menu select for client.
		if ( GetPrivLevel() < PLEVEL_Seer )
		{
			if ( pObj != this )
				return( false );
		}
		if ( ! Spell_SimpleEffect( pObj, pObjSrc, spell, iSkillLevel ) )
			return( false );
		break;

	// 8th Circle
	case SPELL_Earthquake:
		if ( !areaRadius )
			areaRadius = UO_MAP_VIEW_SIGHT;
		Spell_Area( GetTopPoint(), areaRadius, iSkillLevel );
		break;

	case SPELL_Vortex:
 		if ( ! iC1 )
			iC1 = CREID_VORTEX;
		m_atMagery.m_SummonID = iC1;
		m_atMagery.m_fSummonPet = true;
		Spell_Summon( m_atMagery.m_SummonID, m_Act_p, m_atMagery.m_fSummonPet != 0 );
		break;

	case SPELL_Resurrection:
		if ( ! Spell_SimpleEffect( pObj, pObjSrc, spell, iSkillLevel ) )
			return( false );
		break;

	case SPELL_Air_Elem:
 		if ( ! iC1 )
			iC1 = CREID_AIR_ELEM;
		m_atMagery.m_SummonID = iC1;
		m_atMagery.m_fSummonPet = true;
		Spell_Summon( m_atMagery.m_SummonID, m_Act_p, m_atMagery.m_fSummonPet != 0 );
		break;
	case SPELL_Daemon:
 		if ( ! iC1 )
			iC1 = ( Calc_GetRandVal( 2 )) ? CREID_DAEMON_SWORD : CREID_DAEMON;
		m_atMagery.m_SummonID = iC1;
		m_atMagery.m_fSummonPet = true;
		Spell_Summon( m_atMagery.m_SummonID, m_Act_p, m_atMagery.m_fSummonPet != 0 );
		break;
	case SPELL_Earth_Elem:
 		if ( ! iC1 )
			iC1 = CREID_EARTH_ELEM;
		m_atMagery.m_SummonID = iC1;
		m_atMagery.m_fSummonPet = true;
		Spell_Summon( m_atMagery.m_SummonID, m_Act_p, m_atMagery.m_fSummonPet != 0 );
		break;
	case SPELL_Fire_Elem:
 		if ( ! iC1 )
			iC1 = CREID_FIRE_ELEM;
		m_atMagery.m_SummonID = iC1;
		m_atMagery.m_fSummonPet = true;
		Spell_Summon( m_atMagery.m_SummonID, m_Act_p, m_atMagery.m_fSummonPet != 0 );
		break;
	case SPELL_Water_Elem:
 		if ( ! iC1 )
			iC1 = CREID_WATER_ELEM;
		m_atMagery.m_SummonID = iC1;
		m_atMagery.m_fSummonPet = true;
		Spell_Summon( m_atMagery.m_SummonID, m_Act_p, m_atMagery.m_fSummonPet != 0 );
		break;

		// Necro
	case SPELL_Summon_Undead:
		switch (Calc_GetRandVal(15))
		{
		case 1:
			m_atMagery.m_SummonID = CREID_LICH;
			break;
		case 3:
		case 5:
		case 7:
		case 9:
			m_atMagery.m_SummonID = CREID_SKELETON;
			break;
		default:
			m_atMagery.m_SummonID = CREID_ZOMBIE;
			break;
		}
		m_atMagery.m_fSummonPet = true;
		Spell_Summon( m_atMagery.m_SummonID, m_Act_p, m_atMagery.m_fSummonPet != 0 );
		break;

	case SPELL_Animate_Dead:
		{
			CItemCorpse * pCorpse = dynamic_cast <CItemCorpse*> (pObj);
			if ( pCorpse == NULL )
			{
				SysMessageDefault( DEFMSG_SPELL_ANIMDEAD_NC );
				return( false );
			}
			if ( IsPriv( PRIV_GM ))
			{
				m_atMagery.m_SummonID = pCorpse->m_itCorpse.m_BaseID;
			}
			else if ( CCharBase::IsHumanID( pCorpse->GetCorpseType())) 	// Must be a human corpse ?
			{
				m_atMagery.m_SummonID = CREID_ZOMBIE;
			}
			else
			{
				m_atMagery.m_SummonID = pCorpse->GetCorpseType();
			}
			m_atMagery.m_fSummonPet = true;

			if ( ! pCorpse->IsTopLevel())
			{
				return( false );
			}
			CChar * pChar = Spell_Summon( m_atMagery.m_SummonID, pCorpse->GetTopPoint(), true );
			ASSERT( pChar );
			if ( ! pChar->RaiseCorpse( pCorpse ))
			{
				SysMessageDefault( DEFMSG_SPELL_ANIMDEAD_FAIL );
				pChar->Delete();
			}
			break;
		}

	case SPELL_Bone_Armor:
		{
			CItemCorpse * pCorpse = dynamic_cast <CItemCorpse*> (pObj);
			if ( pCorpse == NULL )
			{
				SysMessage( "That is not a corpse!" );
				return( false );
			}
			if ( ! pCorpse->IsTopLevel() ||
				pCorpse->GetCorpseType() != CREID_SKELETON ) 	// Must be a skeleton corpse
			{
				SysMessage( "The body stirs for a moment" );
				return( false );
			}
			// Dump any stuff on corpse
			pCorpse->ContentsDump( pCorpse->GetTopPoint());
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
				if ( ! Calc_GetRandVal( 2+iGet ))
					break;
				CItem * pItem = CItem::CreateScript( (ITEMID_TYPE) sm_Item_Bone[i], this );
				pItem->MoveToCheck( m_Act_p, this );
				iGet++;
			}
			if ( ! iGet )
			{
				SysMessage( "The bones shatter into dust!" );
				break;
			}
		}
		break;

	case SPELL_Light:
		if ( ! Spell_SimpleEffect( pObj, pObjSrc, spell, iSkillLevel ) )
			return( false );
		break;

	case SPELL_Fire_Bolt:
		Spell_Bolt( pObj, ITEMID_FX_FIRE_BOLT, iSkillLevel );
		break;

	case SPELL_Ale:		// 90 = drunkeness ?
	case SPELL_Wine:	// 91 = mild drunkeness ?
	case SPELL_Liquor:	// 92 = extreme drunkeness ?
	case SPELL_Hallucination:
	case SPELL_Stone:
	case SPELL_Shrink:
	case SPELL_Mana:
	case SPELL_Refresh:
	case SPELL_Restore:			// increases both your hit points and your stamina.
	case SPELL_Sustenance:		// serves to fill you up. (Remember, healing rate depends on how well fed you are!)
	case SPELL_Gender_Swap:		// permanently changes your gender.
	case SPELL_Chameleon:		// makes your skin match the colors of whatever is behind you.
	case SPELL_BeastForm:		// polymorphs you into an animal for a while.
	case SPELL_Monster_Form:	// polymorphs you into a monster for a while.
	case SPELL_Trance:			// temporarily increases your meditation skill.
	case SPELL_Particle_Form:	// turns you into an immobile, but untargetable particle system for a while.
	case SPELL_Shield:			// erects a temporary force field around you. Nobody approaching will be able to get within 1 tile of you, though you can move close to them if you wish.
	case SPELL_Steelskin:		// turns your skin into steel, giving a boost to your AR.
	case SPELL_Stoneskin:		// turns your skin into stone, giving a boost to your AR.
	default:
		if ( ! Spell_SimpleEffect( pObj, pObjSrc, spell, iSkillLevel ) )
			return( false );

		// No effect on creatures it seems.
		break;
	}

	if ( g_Cfg.m_fHelpingCriminalsIsACrime &&
		pObj != NULL &&
		pObj->IsChar() &&
		pObj != this &&
		pSpellDef->IsSpellType(SPELLFLAG_GOOD))
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

	// Make noise.
	if ( ! IsStatFlag( STATF_Insubstantial ))
	{
		Sound( pSpellDef->m_sound );
	}

	// At this point we should gain skill if precasting is enabled
	if ( IsClient() && IsSetMagicFlags( MAGICF_PRECAST ) && !pSpellDef->IsSpellType( SPELLFLAG_NOPRECAST ))
	{
		iDifficulty /= 10;
		Skill_Experience( (SKILL_TYPE)iSkill, iDifficulty );
	}
	return( true );
}

void CChar::Spell_CastFail()
{
	ADDTOCALLSTACK("CChar::Spell_CastFail");
	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		CScriptTriggerArgs	Args( m_atMagery.m_Spell, 0, m_Act_TargPrv.ObjFind() );
		if ( OnTrigger( CTRIG_SpellFail, this, &Args ) == TRIGRET_RET_TRUE )
			return;
		if ( Spell_OnTrigger( m_atMagery.m_Spell, SPTRIG_FAIL, this, &Args ) == TRIGRET_RET_TRUE )
			return;
	}

	Effect( EFFECT_OBJ, ITEMID_FX_SPELL_FAIL, this, 1, 30 );
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

	if ( IsClient() && IsSetMagicFlags( MAGICF_PRECAST ) && ! pSpellDef->IsSpellType( SPELLFLAG_NOPRECAST ) )
	{
		m_Act_p = GetTopPoint();
		m_Act_Targ = GetClient()->m_Targ_UID;
		m_Act_TargPrv = GetClient()->m_Targ_PrvUID;

		if ( ! Spell_CanCast(m_atMagery.m_Spell, true, m_Act_TargPrv.ObjFind(), true))
			return( -1 );
	}
	else
	{
		if ( ! Spell_TargCheck())
			return( -1 );
	}

	// Animate casting.
	if (!pSpellDef->IsSpellType( SPELLFLAG_NO_CASTANIM ))
		UpdateAnimate(( pSpellDef->IsSpellType( SPELLFLAG_DIR_ANIM )) ? ANIM_CAST_DIR : ANIM_CAST_AREA );

	bool fWOP = ( GetPrivLevel() >= PLEVEL_Counsel ) ? g_Cfg.m_fWordsOfPowerStaff : g_Cfg.m_fWordsOfPowerPlayer;

	if ( ! NPC_CanSpeak() || IsStatFlag( STATF_Insubstantial ))
	{
		fWOP = false;
	}

	int iDifficulty;
	int iSkill;
	if ( !pSpellDef->GetPrimarySkill( &iSkill, &iDifficulty ) )
		return -1;
	iDifficulty	/= 10;	// adjust to 0-100

	CGrayUID uid( m_Act_TargPrv );
	CItem * pItem = uid.ItemFind();
	if ( pItem != NULL )
	{
		if ( pItem->IsType(IT_WAND))
		{
			// Wand use no words of power. and require no magery.
			fWOP = false;
			iDifficulty = 1;
		}
		else
		{
			// Scroll
			iDifficulty /= 2;
		}
	}

	if ( ! g_Cfg.m_fEquippedCast && fWOP )
	{
		// Attempt to Unequip stuff before casting.
		// Except not wands and spell books !
		if ( ! Spell_Unequip( LAYER_HAND1 ))
			return( -1 );
		if ( ! Spell_Unequip( LAYER_HAND2 ))
			return( -1 );
	}

	int iWaitTime = IsPriv(PRIV_GM) ? 1 : pSpellDef->m_CastTime.GetLinear(Skill_GetBase((SKILL_TYPE)iSkill));

	CScriptTriggerArgs Args((int)m_atMagery.m_Spell, iDifficulty, pItem);
	Args.m_iN3		= iWaitTime;

	if ( OnTrigger(CTRIG_SpellCast, this, &Args) == TRIGRET_RET_TRUE )
		return -1;
	if ( Spell_OnTrigger((SPELL_TYPE)Args.m_iN1, SPTRIG_START, this, &Args) == TRIGRET_RET_TRUE )
		return -1;

	m_atMagery.m_Spell		= (SPELL_TYPE) Args.m_iN1;
	iDifficulty				= Args.m_iN2;
	iWaitTime				= Args.m_iN3;

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
			TCHAR *pszTemp = Str_GetTemp();

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
				Speak(pszTemp, g_Cfg.m_iWordsOfPowerColor, TALKMODE_SAY, (enum FONT_TYPE)g_Cfg.m_iWordsOfPowerFont);
			}
		}
	}

	SetTimeout(iWaitTime);
	return iDifficulty;
}


bool CChar::OnSpellEffect( SPELL_TYPE spell, CChar * pCharSrc, int iSkillLevel, CItem * pSourceItem )
{
	ADDTOCALLSTACK("CChar::OnSpellEffect");
	// Spell has a direct effect on this char.
	// This should effect noto of source.
	// ARGS:
	//  pSourceItem = the potion, wand, scroll etc. NULL = cast (IT_SPELL)
	//  iSkillLevel = 0-1000 = difficulty. may be slightly larger .
	// RETURN:
	//  false = the spell did not work. (should we get credit ?)

	int		iEffectMult	= 1000;

	if ( this == NULL )
		return( false );
	ASSERT( ! IsItem());

	if ( iSkillLevel <= 0 )	// spell died (fizzled?).
		return( false );
	if ( IsStatFlag(STATF_DEAD) )
	{
		//	bug with gaining reflection skill by affecting by some spells while dead
		if ( spell != SPELL_Resurrection )
			return false;
	}


	CScriptTriggerArgs Args( (int) spell, iSkillLevel, pSourceItem );
	Args.m_iN3		= iEffectMult;
	TRIGRET_TYPE	iRet	= OnTrigger( CTRIG_SpellEffect, pCharSrc ? pCharSrc : this, &Args );
	spell			= (SPELL_TYPE) Args.m_iN1;
	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);

	switch ( iRet )
	{
		case TRIGRET_RET_TRUE:	return( false );
		case TRIGRET_RET_FALSE:	if ( pSpellDef && pSpellDef->IsSpellType( SPELLFLAG_SCRIPTED ) ) return true;
		default:				break;
	}

	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		iRet			= Spell_OnTrigger( spell, SPTRIG_EFFECT, pCharSrc ? pCharSrc : this, &Args );
	}
	spell			= (SPELL_TYPE) Args.m_iN1;
	iSkillLevel		= Args.m_iN2;
    iEffectMult		= Args.m_iN3;
	pSpellDef		= g_Cfg.GetSpellDef(spell);

	switch ( iRet )
	{
		case TRIGRET_RET_TRUE:	return( false );
		case TRIGRET_RET_FALSE:	if ( pSpellDef && pSpellDef->IsSpellType( SPELLFLAG_SCRIPTED ) ) return true;
		default:				break;
	}


	if ( pSpellDef == NULL )
		return( false );

	// Most spells don't work on ghosts.
	if ( IsStatFlag(STATF_DEAD) && !(spell == SPELL_Resurrection) ) // || pSpellDef->IsSpellType( SPELLFLAG_TARG_DEAD ) ) )
		return false;

	bool fResistAttempt = true;
	switch ( spell )	// just strengthen the effect.
	{
		case SPELL_Poison:
		case SPELL_Poison_Field:
			if ( IsStatFlag(STATF_Poisoned))
			{
				fResistAttempt = false;
			}	// no further effect. don't count resist effect.
			break;
		case SPELL_Paralyze_Field:
		case SPELL_Paralyze:
			if ( IsStatFlag(STATF_Freeze))
				return false;	// no further effect.
			break;
		default:
			break;
	}

	bool fPotion = ( pSourceItem != NULL && pSourceItem->IsType( IT_POTION ));
	if ( fPotion )
		fResistAttempt = false;
	if ( pCharSrc == this )
		fResistAttempt = false;

	if ( pSpellDef->IsSpellType( SPELLFLAG_HARM ))
	{
		// Can't harm yourself directly ?
		if ( !IsSetMagicFlags( MAGICF_CANHARMSELF ) && (pCharSrc == this) )
			return( false );

		if ( IsStatFlag( STATF_INVUL ))
		{
			Effect( EFFECT_OBJ, ITEMID_FX_GLOW, this, 9, 30, false );
			return false;
		}

		if ( ! fPotion && fResistAttempt )
		{
			if ( pCharSrc != NULL && GetPrivLevel() > PLEVEL_Guest )
			{
				if ( pCharSrc->GetPrivLevel() <= PLEVEL_Guest )
				{
					pCharSrc->SysMessage( "The guest curse strikes you." );
					goto reflectit;
				}
			}

			// Check resistance to magic ?
			if ( pSpellDef->IsSpellType( SPELLFLAG_RESIST ))
			{
			if ( Skill_UseQuick( SKILL_MAGICRESISTANCE, iSkillLevel ))
			{
				SysMessage( "You feel yourself resisting magic" );

				// iSkillLevel
				iSkillLevel /= 2;	// ??? reduce effect of spell.
			}

			// Check magic reflect.
			if ( IsStatFlag( STATF_Reflection ))	// reflected.
			{
				StatFlag_Clear( STATF_Reflection );
reflectit:
				Effect( EFFECT_OBJ, ITEMID_FX_GLOW, this, 9, 30, false );
				if ( pCharSrc != NULL )
				{
					pCharSrc->OnSpellEffect( spell, NULL, iSkillLevel/2, pSourceItem );
				}
				return false;
			}
		}
		}

		if ( ! OnAttackedBy( pCharSrc, 1, false, !(pSpellDef->IsSpellType(SPELLFLAG_FIELD)) ))
			return false;
	}

	if ( pSpellDef->IsSpellType( SPELLFLAG_FX_TARG ) &&	pSpellDef->m_idEffect )
	{
		Effect( EFFECT_OBJ, pSpellDef->m_idEffect, this, 0, 15 ); // 9, 14
	}

	iSkillLevel = iSkillLevel/2 + Calc_GetRandVal(iSkillLevel/2);	// randomize the effect.

	if ( pSpellDef->IsSpellType( SPELLFLAG_SCRIPTED ) == false )
	{
		switch ( spell )
		{

		case SPELL_Ale:		// 90 = drunkeness ?
		case SPELL_Wine:	// 91 = mild drunkeness ?
		case SPELL_Liquor:	// 92 = extreme drunkeness ?

		case SPELL_Clumsy:
		case SPELL_Feeblemind:
		case SPELL_Weaken:
		case SPELL_Curse:
		case SPELL_Agility:
		case SPELL_Cunning:
		case SPELL_Strength:
		case SPELL_Bless:
			Spell_Effect_Create( spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_STATS, iSkillLevel,
					GetSpellDuration( spell, iSkillLevel, iEffectMult ), pCharSrc );
			break;

		case SPELL_Mass_Curse:
			Spell_Effect_Create( SPELL_Curse, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_STATS, iSkillLevel,
					GetSpellDuration( spell, iSkillLevel, iEffectMult ), pCharSrc );
			break;

		case SPELL_Heal:
		case SPELL_Great_Heal:
			UpdateStatVal( STAT_STR, GetSpellEffect( spell, iSkillLevel, iEffectMult ) );
			break;

		case SPELL_Night_Sight:
			Spell_Effect_Create( SPELL_Night_Sight, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Night_Sight, iSkillLevel,
					GetSpellDuration( spell, iSkillLevel, iEffectMult ), pCharSrc );
			break;

		case SPELL_Reactive_Armor:
			Spell_Effect_Create( SPELL_Reactive_Armor, LAYER_SPELL_Reactive, iSkillLevel,
					GetSpellDuration( spell, iSkillLevel, iEffectMult ), pCharSrc );
			break;

		case SPELL_Magic_Reflect:
			Spell_Effect_Create( SPELL_Magic_Reflect, LAYER_SPELL_Magic_Reflect, iSkillLevel,
					GetSpellDuration( spell, iSkillLevel, iEffectMult ), pCharSrc );
			break;

		case SPELL_Poison:
		case SPELL_Poison_Field:
			if ( ! fPotion )
			{
				Effect( EFFECT_OBJ, ITEMID_FX_CURSE_EFFECT, this, 0, 15 );
			}
			SetPoison( iSkillLevel, iSkillLevel/50, pCharSrc );
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
				GetSpellDuration( spell, iSkillLevel, iEffectMult ), pCharSrc );
			break;

		case SPELL_Summon:
			Spell_Effect_Create( spell,	LAYER_SPELL_Summon, iSkillLevel,
				GetSpellDuration( spell, iSkillLevel, iEffectMult ), pCharSrc );
			break;

		case SPELL_Dispel:
		case SPELL_Mass_Dispel:
			// ??? should be difficult to dispel SPELL_Summon creatures
			Spell_Dispel( (pCharSrc != NULL && pCharSrc->IsPriv(PRIV_GM)) ? 150 : 50);
			break;

		case SPELL_Reveal:
			if ( ! Reveal())
				break;
			Effect( EFFECT_OBJ, ITEMID_FX_BLESS_EFFECT, this, 0, 15 );
			break;

		case SPELL_Invis:
			Spell_Effect_Create( SPELL_Invis, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Invis, iSkillLevel,
				GetSpellDuration( spell, iSkillLevel, iEffectMult ), pCharSrc );
			break;

		case SPELL_Incognito:
			Spell_Effect_Create( SPELL_Incognito, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Incognito, iSkillLevel,
				GetSpellDuration( spell, iSkillLevel, iEffectMult ), pCharSrc );
			break;

		case SPELL_Particle_Form:	// 112 // turns you into an immobile, but untargetable particle system for a while.
		case SPELL_Stone:
		case SPELL_Paralyze_Field:
		case SPELL_Paralyze:
			// Effect( EFFECT_OBJ, ITEMID_FX_CURSE_EFFECT, this, 0, 15 );
			Spell_Effect_Create( spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Paralyze, iSkillLevel,
				GetSpellDuration( spell, iSkillLevel, iEffectMult ), pCharSrc );
			break;

		case SPELL_Mana_Drain:
		case SPELL_Mana_Vamp:
			UpdateStatVal( STAT_INT, -iSkillLevel );
			break;

		case SPELL_Harm:
		case SPELL_Mind_Blast:
			OnTakeDamage( GetSpellEffect( spell, iSkillLevel, iEffectMult ), pCharSrc, DAMAGE_MAGIC | DAMAGE_POISON | DAMAGE_GENERAL | DAMAGE_NOREVEAL );
			break;
		case SPELL_Explosion:
			OnTakeDamage( GetSpellEffect( spell, iSkillLevel, iEffectMult ), pCharSrc, DAMAGE_MAGIC | DAMAGE_HIT_BLUNT | DAMAGE_GENERAL | DAMAGE_NOREVEAL );
			break;
		case SPELL_Energy_Bolt:
		case SPELL_Magic_Arrow:
			OnTakeDamage( GetSpellEffect( spell, iSkillLevel, iEffectMult ), pCharSrc, DAMAGE_MAGIC | DAMAGE_HIT_PIERCE | DAMAGE_NOREVEAL );
			break;
		case SPELL_Fireball:
		case SPELL_Fire_Bolt:
			OnTakeDamage( GetSpellEffect( spell, iSkillLevel, iEffectMult ), pCharSrc, DAMAGE_MAGIC | DAMAGE_HIT_BLUNT | DAMAGE_FIRE | DAMAGE_NOREVEAL );
			break;
		case SPELL_Fire_Field:
		case SPELL_Flame_Strike:
			// Burn whoever is there.
			OnTakeDamage( GetSpellEffect( spell, iSkillLevel, iEffectMult ), pCharSrc, DAMAGE_MAGIC | DAMAGE_FIRE | DAMAGE_GENERAL | DAMAGE_NOREVEAL );
			break;
		case SPELL_Meteor_Swarm:
			Effect( EFFECT_OBJ, ITEMID_FX_EXPLODE_3, this, 9, 6 );
			OnTakeDamage( GetSpellEffect( spell, iSkillLevel, iEffectMult ), pCharSrc, DAMAGE_MAGIC | DAMAGE_HIT_BLUNT | DAMAGE_FIRE | DAMAGE_NOREVEAL );
			break;
		case SPELL_Earthquake:
			OnTakeDamage( GetSpellEffect( spell, iSkillLevel, iEffectMult ), pCharSrc, DAMAGE_MAGIC | DAMAGE_HIT_BLUNT | DAMAGE_GENERAL | DAMAGE_NOREVEAL );
			break;
		case SPELL_Lightning:
		case SPELL_Chain_Lightning:
			GetTopSector()->LightFlash();
			Effect( EFFECT_LIGHTNING, ITEMID_NOTHING, pCharSrc );
			OnTakeDamage( GetSpellEffect( spell, iSkillLevel, iEffectMult ), pCharSrc, DAMAGE_MAGIC | DAMAGE_ELECTRIC | DAMAGE_GENERAL | DAMAGE_NOREVEAL );
			break;

		case SPELL_Resurrection:
			return Spell_Resurrection( NULL, pCharSrc );

		case SPELL_Light:
			Effect( EFFECT_OBJ, ITEMID_FX_HEAL_EFFECT, this, 9, 6 );
			Spell_Effect_Create( spell, fPotion ? LAYER_FLAG_Potion : LAYER_NEWLIGHT, iSkillLevel,
				GetSpellDuration( spell, iSkillLevel, iEffectMult ), pCharSrc );
			break;

		case SPELL_Hallucination:
			{
			CItem * pItem = Spell_Effect_Create( SPELL_Hallucination, LAYER_FLAG_Hallucination, iSkillLevel, 10*TICK_PER_SEC, pCharSrc );
			pItem->m_itSpell.m_spellcharges = Calc_GetRandVal(30);
			}
			break;
		case SPELL_Polymorph:
			{
				CREID_TYPE creid = m_atMagery.m_SummonID;
	#define SPELL_MAX_POLY_STAT 150

				CItem * pSpell = Spell_Effect_Create( SPELL_Polymorph, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Polymorph, iSkillLevel,
					GetSpellDuration( spell, iSkillLevel, iEffectMult ), pCharSrc );
				SetID(creid);

				CCharBase * pCharDef = Char_GetDef();
				ASSERT(pCharDef);
			
				// re-apply our incognito name
				if ( IsStatFlag( STATF_Incognito ) )
				{
					SetName( pCharDef->GetTypeName() );
				}

				// set to creature type stats.
				if ( pCharDef->m_Str )
				{
					int iChange = pCharDef->m_Str - Stat_GetBase(STAT_STR);
					if ( iChange > SPELL_MAX_POLY_STAT )
						iChange = SPELL_MAX_POLY_STAT;
					if ( iChange + Stat_GetBase(STAT_STR) < 0 )
						iChange = -Stat_GetBase(STAT_STR);
					Stat_AddMod( STAT_STR, iChange );
					pSpell->m_itSpell.m_PolyStr = iChange;
				}
				else
				{
					pSpell->m_itSpell.m_PolyStr = 0;
				}
				if ( pCharDef->m_Dex )
				{
					int iChange = pCharDef->m_Dex - Stat_GetBase(STAT_DEX);
					if ( iChange > SPELL_MAX_POLY_STAT )
						iChange = SPELL_MAX_POLY_STAT;
					if ( iChange + Stat_GetBase(STAT_DEX) < 0 )
						iChange = -Stat_GetBase(STAT_DEX);
					Stat_AddMod( STAT_DEX, iChange );
					pSpell->m_itSpell.m_PolyDex = iChange;
				}
				else
				{
					pSpell->m_itSpell.m_PolyDex = 0;
				}
				Update();		// show everyone I am now a new type
			}
			break;

		case SPELL_Shrink:
			// Getting a pet to drink this is funny.
			if ( m_pPlayer )
				break;
			if ( fPotion && pSourceItem )
			{
				pSourceItem->Delete();
			}
			NPC_Shrink();	// this delete's the char !!!
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
			if ( IsHuman())
			{
				CCharBase * pCharDef = Char_GetDef();
				ASSERT(pCharDef);

				SetID( pCharDef->IsFemale() ? CREID_MAN : CREID_WOMAN );
				m_prev_id = GetID();
				Update();
			}
			break;

		case SPELL_Chameleon:		// 106 // makes your skin match the colors of whatever is behind you.
		case SPELL_BeastForm:		// 107 // polymorphs you into an animal for a while.
		case SPELL_Monster_Form:	// 108 // polymorphs you into a monster for a while.
			Spell_Effect_Create( spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Polymorph, iSkillLevel,
				GetSpellDuration( spell, iSkillLevel, iEffectMult ), pCharSrc );
			break;

		case SPELL_Trance:			// 111 // temporarily increases your meditation skill.
			Spell_Effect_Create( spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_STATS, iSkillLevel,
				GetSpellDuration( spell, iSkillLevel, iEffectMult ), pCharSrc );
			break;

		case SPELL_Shield:			// 113 // erects a temporary force field around you. Nobody approaching will be able to get within 1 tile of you, though you can move close to them if you wish.
		case SPELL_Steelskin:		// 114 // turns your skin into steel, giving a boost to your AR.
		case SPELL_Stoneskin:		// 115 // turns your skin into stone, giving a boost to your AR.
			Spell_Effect_Create( spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_Protection, iSkillLevel,
				GetSpellDuration( spell, iSkillLevel, iEffectMult ), pCharSrc );
			break;

		case SPELL_Regenerate:
			// Set number of charges based on effect level.
			//
			{
				int iDuration = GetSpellDuration( spell, iSkillLevel, iEffectMult );
				iDuration /= (2*TICK_PER_SEC);
				if ( iDuration <= 0 )
					iDuration = 1;
				CItem * pSpell = Spell_Effect_Create( spell, fPotion ? LAYER_FLAG_Potion : LAYER_SPELL_STATS, iSkillLevel, 2*TICK_PER_SEC, pCharSrc );
				ASSERT(pSpell);
				pSpell->m_itSpell.m_spellcharges = iDuration;
			}
			break;

		default:
			break;
		}
	}
	return( true );
}



int CChar::GetSpellEffect( SPELL_TYPE spell, int iSkillLevel, int iEffectMult )
{
	ADDTOCALLSTACK("CChar::GetSpellEffect");
	long	iEffect = g_Cfg.GetSpellEffect( spell, iSkillLevel );
	return (iEffect * iEffectMult ) / 1000;
}



int CChar::GetSpellDuration( SPELL_TYPE spell, int iSkillLevel, int iEffectMult )
{
	ADDTOCALLSTACK("CChar::GetSpellDuration");
	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
	ASSERT(pSpellDef);

	int	iDuration	= pSpellDef->m_Duration.GetLinear( iSkillLevel );
	return (iDuration * iEffectMult) / 1000;
}
