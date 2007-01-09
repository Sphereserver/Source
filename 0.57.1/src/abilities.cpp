#include "abilities.h"

//		***		***			***
//
//		CombatAbility
//
//		***		***			***

CombatAbility *CombatAbility::abils[ABIL_QTY + 1] =
{
	new CombatAbility(),
	new CombatAbility_ArmorIgnore(),
	new CombatAbility_BleedAttack(),
	new CombatAbility_ConcussionBlow(),
	new CombatAbility_CrushingBlow(),
	new CombatAbility_Disarm(),
	new CombatAbility_Dismount(),
	new CombatAbility_DoubleStrike(),
	new CombatAbility_InfectiousStrike(),
	new CombatAbility_MortalStrike(),
	new CombatAbility_MovingShot(),
	new CombatAbility_ParalyzingBlow(),
	new CombatAbility_ShadowStrike(),
	new CombatAbility_WhirlwindAttack(),
	NULL
};

CombatAbility::CombatAbility(int mana, int damagePercent) :
	m_mana(mana), m_damagePercent(damagePercent)
{
}

void CombatAbility::OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType)
{
	ClearCurrentAbility(attacker);
	damage = ( damage * m_damagePercent ) / 100;
}

void CombatAbility::OnMiss(CChar *attacker, CChar *defender)
{
	ClearCurrentAbility(attacker);
}

void CombatAbility::OnItemTick(CItem *item)
{
	item->SetAttr(ATTR_DECAY);
}

bool CombatAbility::CheckMana(CChar *from)
{
	int mana = m_mana;
	int skilltotal = from->Skill_GetAdjusted(SKILL_SWORDSMANSHIP) + from->Skill_GetAdjusted(SKILL_MACEFIGHTING) +
		from->Skill_GetAdjusted(SKILL_FENCING) + from->Skill_GetAdjusted(SKILL_ARCHERY) +
		from->Skill_GetAdjusted(SKILL_PARRYING) + from->Skill_GetAdjusted(SKILL_LUMBERJACKING) +
		from->Skill_GetAdjusted(SKILL_STEALTH) + from->Skill_GetAdjusted(SKILL_POISONING);

	if ( skilltotal >= 3000 )
		mana -= 10;
	else if ( skilltotal >= 2000 )
		mana -= 5;

	// using special move in 3 seconds after previous costs double mana
	if ( from->IsClient() )
	{
		if ( from->GetClient()->m_TagDefs.GetKeyNum("LastCombatAbility", true) >= ( g_World.GetCurrentTime().GetTimeRaw() - 3 * TICK_PER_SEC) )
			mana *= 2;
	}

	if ( from->Stat_GetVal(STAT_INT) < mana )
	{
		from->SysMessagef("You need %d mana to perform this attack", mana);
		ClearCurrentAbility(from);
		return false;
	}

	from->Stat_SetVal(STAT_INT, from->Stat_GetVal(STAT_INT) - mana);
	return true;
}

void CombatAbility::ClearCurrentAbility(CChar *from)
{
	if ( from->IsClient() )
	{
		CClient *pClient = from->GetClient();
		pClient->m_TagDefs.DeleteKey("SpecialCombatMove");
	}
	//	TODO;
	//	send 0xbf (short)5 (short)33
}

int CombatAbility::IndexOf(CombatAbility *abil)
{
	for ( int i = 0; i < ABIL_QTY; i++ )
	{
		if ( abils[i] == abil )
			return i;
	}
	return -1;
}

//		***		***			***
//
//		CombatAbility	ArmorIgnore
//
//		***		***			***

CombatAbility_ArmorIgnore::CombatAbility_ArmorIgnore() : CombatAbility(30,90)
{
}

void CombatAbility_ArmorIgnore::OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType)
{
	if ( !CheckMana(attacker) )
		return;

	CombatAbility::OnHit(attacker, defender, damage, uType);
	uType = DAMAGE_GOD;

	attacker->SysMessage("Your attack penetrates their armor!");	// 1060076
	defender->SysMessage("The blow penetrated your armor!");		// 1060077

	defender->Sound(0x56);
}

//		***		***			***
//
//		CombatAbility	BleedAttack
//
//		***		***			***

CombatAbility_BleedAttack::CombatAbility_BleedAttack() : CombatAbility(30)
{
}

void CombatAbility_BleedAttack::OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType)
{
	if ( !CheckMana(attacker) )
		return;

	CombatAbility::OnHit(attacker, defender, damage, uType);

	attacker->SysMessage("Your target is bleeding!");	// 1060159
	defender->SysMessage("You are bleeding!");			// 1060160

	defender->Sound(0x133);

	CItem *pSpell = CItem::CreateBase(ITEMID_RHAND_POINT_NW);
	pSpell->SetAttr(ATTR_NEWBIE|ATTR_MAGIC|ATTR_DECAY);
	pSpell->SetType(IT_SPELL);
	pSpell->m_itSpell.m_spell = -1;
	pSpell->m_itSpell.m_spelllevel = IndexOf(this);
	pSpell->m_itSpell.m_PolyStr = Calc_GetRandVal2(15, 31);
	pSpell->SetDecayTime(1);
	pSpell->m_uidLink = attacker->GetUID();
	defender->LayerAdd(pSpell, LAYER_SPECIAL);
}

void CombatAbility_BleedAttack::OnItemTick(CItem *item)
{
	CChar *defender = static_cast <CChar *>(item->GetContainer());
	CChar *attacker = item->m_uidLink.CharFind();

	if ( !defender || !attacker )
		return;

	int damtomake = Calc_GetRandVal2(1, item->m_itSpell.m_PolyStr);
	item->m_itSpell.m_PolyStr -= damtomake;

	if ( item->m_itSpell.m_PolyStr > 0 )
		item->SetDecayTime(1);

	defender->OnTakeDamage(damtomake, attacker, DAMAGE_GOD);
}

//		***		***			***
//
//		CombatAbility	ConcussionBlow
//
//		***		***			***

CombatAbility_ConcussionBlow::CombatAbility_ConcussionBlow() : CombatAbility(25)
{
}

void CombatAbility_ConcussionBlow::OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType)
{
	if ( !CheckMana(attacker) )
		return;

	CombatAbility::OnHit(attacker, defender, damage, uType);

	attacker->SysMessage("You have delivered a concussion!");	// 1060165
	defender->SysMessage("You feel disoriented!");				// 1060166

	defender->Sound(0x213);

	int healthRatio = (defender->Stat_GetVal(STAT_STR) * 100) / defender->Stat_GetMax(STAT_STR);
	int manaRatio = (defender->Stat_GetVal(STAT_INT) * 100) / defender->Stat_GetMax(STAT_INT);
	int extraDam = healthRatio - manaRatio;
	if ( extraDam < 0 )
		extraDam *= -2;
	damage += 10 + extraDam/4;
	uType = DAMAGE_GOD;
}

//		***		***			***
//
//		CombatAbility	CrushingBlow
//
//		***		***			***

CombatAbility_CrushingBlow::CombatAbility_CrushingBlow() : CombatAbility(25)
{
}

void CombatAbility_CrushingBlow::OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType)
{
	if ( !CheckMana(attacker) )
		return;

	CombatAbility::OnHit(attacker, defender, damage, uType);

	attacker->SysMessage("You have delivered a crushing blow!");			// 1060090
	defender->SysMessage("You take extra damage from the crushing attack!");// 1060091

	defender->Sound(0x1e1);

	damage += damage/2;
	uType = DAMAGE_HIT_BLUNT;
}

//		***		***			***
//
//		CombatAbility	Disarm
//
//		***		***			***

CombatAbility_Disarm::CombatAbility_Disarm() : CombatAbility(20)
{
}

void CombatAbility_Disarm::OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType)
{
	if ( !CheckMana(attacker) )
		return;

	CombatAbility::OnHit(attacker, defender, damage, uType);

	CItem *pItem = defender->LayerFind(LAYER_HAND1);
	if ( !pItem )
		pItem = defender->LayerFind(LAYER_HAND2);

	if ( !pItem )
	{
		attacker->SysMessage("Your target is already unarmed!");// 1060849
	}
	else
	{
		attacker->SysMessage("You disarm their weapon!");		// 1060092
		defender->SysMessage("Your weapon has been disarmed!");	// 1060093

		defender->Sound(0x3b9);

		defender->GetPackSafe()->ContentAdd(pItem);
	}
}

//		***		***			***
//
//		CombatAbility	Dismount
//
//		***		***			***

CombatAbility_Dismount::CombatAbility_Dismount() : CombatAbility(20)
{
}

void CombatAbility_Dismount::OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType)
{
	if ( !CheckMana(attacker) )
		return;

	CombatAbility::OnHit(attacker, defender, damage, uType);

	if ( attacker->IsStatFlag(STATF_OnHorse) )
	{
		attacker->SysMessage("You cannot perform that attack while mounted!");	// 1061283
	}
	else if ( !defender->IsStatFlag(STATF_OnHorse) )
	{
		attacker->SysMessage("This attack only works on mounted targets");		// 1060848
	}
	else
	{
		attacker->SysMessage("The force of your attack has dislodged them from their mount!");	// 1060082
		defender->Sound(0x140);
		defender->Horse_UnMount();

		damage += Calc_GetRandVal2(15, 25);
	}
}

//		***		***			***
//
//		CombatAbility	DoubleStrike
//
//		***		***			***

CombatAbility_DoubleStrike::CombatAbility_DoubleStrike() : CombatAbility(20)
{
}

void CombatAbility_DoubleStrike::OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType)
{
	if ( !CheckMana(attacker) )
		return;

	CombatAbility::OnHit(attacker, defender, damage, uType);

	damage *= 2;

	attacker->SysMessage("You attack with lightning speed!");			// 1060084
	defender->SysMessage("Your attacker strikes with lightning speed!");// 1060085

	defender->Sound(0x3bb);
}

//		***		***			***
//
//		CombatAbility	InfectiousStrike
//
//		***		***			***

CombatAbility_InfectiousStrike::CombatAbility_InfectiousStrike() : CombatAbility(15)
{
}

void CombatAbility_InfectiousStrike::OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType)
{
	if ( !CheckMana(attacker) )
		return;

	CombatAbility::OnHit(attacker, defender, damage, uType);

	defender->SetPoison(Calc_GetRandVal2(200, 800), 30, attacker);

	attacker->SysMessage("You have poisoned the target!");
	defender->SysMessage("You have been poisoned by attacker!");

	defender->Sound(0xdd);
}

//		***		***			***
//
//		CombatAbility	MortalStrike
//
//		***		***			***

CombatAbility_MortalStrike::CombatAbility_MortalStrike() : CombatAbility(30)
{
}

void CombatAbility_MortalStrike::OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType)
{
	if ( !CheckMana(attacker) )
		return;

	CombatAbility::OnHit(attacker, defender, damage, uType);

	attacker->SysMessage("You deliver a mortal wound!");
	defender->SysMessage("You have been mortally wounded!");

	defender->Sound(0x1e1);

	CItem *pSpell = CItem::CreateBase(ITEMID_RHAND_POINT_NW);
	pSpell->SetAttr(ATTR_NEWBIE|ATTR_MAGIC|ATTR_DECAY);
	pSpell->SetType(IT_SPELL);
	pSpell->m_itSpell.m_spell = -1;
	pSpell->m_itSpell.m_spelllevel = IndexOf(this);
	pSpell->m_itSpell.m_PolyStr = defender->IsClient() ? 6 : 12;
	pSpell->m_itSpell.m_PolyDex = defender->Stat_GetVal(STAT_STR);
	pSpell->SetDecayTime(1);
	pSpell->m_uidLink = attacker->GetUID();
	defender->LayerAdd(pSpell, LAYER_SPECIAL);
}

void CombatAbility_MortalStrike::OnItemTick(CItem *item)
{
	CChar *defender = static_cast <CChar *>(item->GetContainer());

	if ( !defender )
		return;

	item->m_itSpell.m_PolyStr--;

	if ( item->m_itSpell.m_PolyStr > 0 )
		item->SetDecayTime(1);

	//	keep hits level of the defender not higher than it was in hit time
	int hits = defender->Stat_GetVal(STAT_STR);
	if ( hits > item->m_itSpell.m_PolyDex )
		defender->Stat_SetVal(STAT_STR, item->m_itSpell.m_PolyDex);
}

//		***		***			***
//
//		CombatAbility	MortalStrike
//
//		***		***			***

CombatAbility_MovingShot::CombatAbility_MovingShot() : CombatAbility(15)
{
}

void CombatAbility_MovingShot::OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType)
{
	if ( !CheckMana(attacker) )
		return;

	CombatAbility::OnHit(attacker, defender, damage, uType);

	attacker->StatFlag_Set(STATF_ArcherCanMove);

	CItem *pSpell = CItem::CreateBase(ITEMID_RHAND_POINT_NW);
	pSpell->SetAttr(ATTR_NEWBIE|ATTR_MAGIC|ATTR_DECAY);
	pSpell->SetType(IT_SPELL);
	pSpell->m_itSpell.m_spell = -1;
	pSpell->m_itSpell.m_spelllevel = IndexOf(this);
	pSpell->SetDecayTime(Calc_GetRandVal2(10, 15));
	attacker->LayerAdd(pSpell, LAYER_SPECIAL);
}

void CombatAbility_MovingShot::OnItemTick(CItem *item)
{
	CChar *attacker = static_cast <CChar *>(item->GetContainer());

	if ( !attacker )
		return;
	attacker->StatFlag_Clear(STATF_ArcherCanMove);
}

//		***		***			***
//
//		CombatAbility	ParalyzingBlow
//
//		***		***			***

CombatAbility_ParalyzingBlow::CombatAbility_ParalyzingBlow() : CombatAbility(30)
{
}

void CombatAbility_ParalyzingBlow::OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType)
{
	if ( !CheckMana(attacker) )
		return;

	CombatAbility::OnHit(attacker, defender, damage, uType);

	attacker->SysMessage("You deliver a paralyzing blow!");				// 1060163
	defender->SysMessage("The attack has temporarily paralyzed you!");	// 1060164

	defender->Sound(0x204);

	CItem *pSpell = CItem::CreateBase(ITEMID_RHAND_POINT_NW);
	pSpell->SetAttr(ATTR_NEWBIE|ATTR_MAGIC|ATTR_DECAY);
	pSpell->SetType(IT_SPELL);
	pSpell->m_itSpell.m_spell = -1;
	pSpell->m_itSpell.m_spelllevel = IndexOf(this);
	pSpell->m_itSpell.m_PolyStr = defender->IsClient() ? 3 : 6;
	pSpell->SetDecayTime(1);
	pSpell->m_uidLink = attacker->GetUID();
	defender->LayerAdd(pSpell, LAYER_SPECIAL);
}

void CombatAbility_ParalyzingBlow::OnItemTick(CItem *item)
{
	CChar *defender = static_cast <CChar *>(item->GetContainer());

	if ( !defender )
		return;

	item->m_itSpell.m_PolyStr--;

	if ( item->m_itSpell.m_PolyStr > 0 )
	{
		defender->StatFlag_Set(STATF_Freeze);
		item->SetDecayTime(1);
	}
	else
		defender->StatFlag_Clear(STATF_Freeze);
}

//		***		***			***
//
//		CombatAbility	ShadowStrike
//
//		***		***			***

CombatAbility_ShadowStrike::CombatAbility_ShadowStrike() : CombatAbility(20, 125)
{
}

void CombatAbility_ShadowStrike::OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType)
{
	if ( !CheckMana(attacker) )
		return;

	CombatAbility::OnHit(attacker, defender, damage, uType);

	if ( attacker->Skill_GetAdjusted(SKILL_STEALTH) < 800 )
	{
		attacker->SysMessage("You lack the required stealth to perform that attack");	// 1060183
	}
	else
	{
		attacker->SysMessage("You strike and hide in the shadows!");					// 1060078
		defender->SysMessage("You are dazed by the attack and your attacker vanishes!");// 1060079

		defender->Sound(0x482);

		CItem *pSpell = CItem::CreateBase(ITEMID_RHAND_POINT_NW);
		pSpell->SetAttr(ATTR_NEWBIE|ATTR_MAGIC|ATTR_DECAY);
		pSpell->SetType(IT_SPELL);
		pSpell->m_itSpell.m_spell = -1;
		pSpell->m_itSpell.m_spelllevel = IndexOf(this);
		pSpell->SetDecayTime(1);
		attacker->LayerAdd(pSpell, LAYER_SPECIAL);
	}
}

void CombatAbility_ShadowStrike::OnItemTick(CItem *item)
{
	CChar *attacker = static_cast <CChar *>(item->GetContainer());

	if ( !attacker )
		return;

	attacker->Skill_Start(SKILL_NONE);
	attacker->StatFlag_Set(STATF_Hidden);
	attacker->Update(attacker->GetClient());
}

//		***		***			***
//
//		CombatAbility	WhirlwindAttack
//
//		***		***			***

CombatAbility_WhirlwindAttack::CombatAbility_WhirlwindAttack() : CombatAbility(15)
{
}

void CombatAbility_WhirlwindAttack::OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType)
{
	if ( !CheckMana(attacker) )
		return;

	CombatAbility::OnHit(attacker, defender, damage, uType);

	attacker->Sound(0x2a1);

	CWorldSearch AreaChars(attacker->GetTopPoint(), 1, attacker);
	while ( CChar *pChar = AreaChars.GetChar() )
	{
		CItemMemory * pMemory = pChar->Memory_FindObjTypes(attacker, MEMORY_FIGHT);
		pChar->OnTakeDamage(damage/2, attacker, uType);
	}
}
