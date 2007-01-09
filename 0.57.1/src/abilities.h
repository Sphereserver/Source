#ifndef ABILITIES_H
#define ABILITIES_H

#include "graysvr.h"

#define ABIL_QTY 14

class CombatAbility
{
public:
	static CombatAbility *abils[ABIL_QTY + 1];

	int	m_mana;
	int m_damagePercent;

	CombatAbility(int mana = 0, int damagePercent = 100);
	virtual void OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType);
	virtual void OnMiss(CChar *attacker, CChar *defender);
	virtual void OnItemTick(CItem *item);

	bool CheckMana(CChar *from);
	int IndexOf(CombatAbility *abil);

	void ClearCurrentAbility(CChar *from);
};

class CombatAbility_ArmorIgnore : public CombatAbility
{
public:
	CombatAbility_ArmorIgnore();

	virtual void OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType);
};

class CombatAbility_BleedAttack : public CombatAbility
{
public:
	CombatAbility_BleedAttack();

	virtual void OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType);
	virtual void OnItemTick(CItem *item);
};

class CombatAbility_ConcussionBlow : public CombatAbility
{
public:
	CombatAbility_ConcussionBlow();

	virtual void OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType);
};

class CombatAbility_CrushingBlow : public CombatAbility
{
public:
	CombatAbility_CrushingBlow();

	virtual void OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType);
};

class CombatAbility_Disarm : public CombatAbility
{
public:
	CombatAbility_Disarm();

	virtual void OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType);
};

class CombatAbility_Dismount : public CombatAbility
{
public:
	CombatAbility_Dismount();

	virtual void OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType);
};

class CombatAbility_DoubleStrike : public CombatAbility
{
public:
	CombatAbility_DoubleStrike();

	virtual void OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType);
};

class CombatAbility_InfectiousStrike : public CombatAbility
{
public:
	CombatAbility_InfectiousStrike();

	virtual void OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType);
};

class CombatAbility_MortalStrike : public CombatAbility
{
public:
	CombatAbility_MortalStrike();

	virtual void OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType);
	virtual void OnItemTick(CItem *item);
};

class CombatAbility_MovingShot : public CombatAbility
{
public:
	CombatAbility_MovingShot();

	virtual void OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType);
	virtual void OnItemTick(CItem *item);
};

class CombatAbility_ParalyzingBlow : public CombatAbility
{
public:
	CombatAbility_ParalyzingBlow();

	virtual void OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType);
	virtual void OnItemTick(CItem *item);
};

class CombatAbility_ShadowStrike : public CombatAbility
{
public:
	CombatAbility_ShadowStrike();

	virtual void OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType);
	virtual void OnItemTick(CItem *item);
};

class CombatAbility_WhirlwindAttack : public CombatAbility
{
public:
	CombatAbility_WhirlwindAttack();

	virtual void OnHit(CChar *attacker, CChar *defender, int &damage, DAMAGE_TYPE &uType);
};

#endif